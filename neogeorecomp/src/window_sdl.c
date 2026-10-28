/*
 * window_sdl.c — the SDL2 window: presentation, sound, keyboard, gamepads,
 * frame pacing. Only built when SDL2 is found; see platform.c for the
 * headless stubs.
 *
 * Keys: arrows, Z/X/C/V = A/B/C/D, 1/2 = start P1/P2, 3 = select,
 * 5/6 = coin 1/2, 9 = service, F2 = test switch, Esc = quit.
 *
 * Gamepads (SDL's GameController layout, so any pad SDL knows): the first
 * connected is player 1, the second player 2, plugged in at any time.
 * D-pad or left stick; south/east/west/north (Xbox A/B/X/Y) = Neo Geo
 * A/B/C/D; Start = start; Back = coin; left shoulder = select. Keyboard
 * and pads work together. A pad SDL doesn't know can be described with the
 * SDL_GAMECONTROLLERCONFIG environment variable.
 */
#include "ng_internal.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>

static SDL_Window *s_win;
static SDL_Renderer *s_ren;
static SDL_Texture *s_tex;
static int s_fullscreen;
static uint64_t s_next_tick;
static SDL_AudioDeviceID s_audio;
static SDL_AudioStream *s_audio_stream;   /* YM2610 rate -> device rate */
static SDL_AudioSpec s_audio_spec;
static Uint32 s_audio_frame_bytes;        /* one device sample frame */
static int s_audio_started;
static unsigned s_audio_underruns, s_audio_stalls;
static int s_audio_warned;
static SDL_GameController *s_pad[2];     /* player 1, player 2 */

/* Queue thresholds, in milliseconds of device audio.  The stream is fed
 * once per video frame (~17 ms), so a cushion above one frame is needed to
 * absorb scheduler jitter.  If the queue actually runs dry after playback
 * started, silence is inserted only up to the target (an underrun
 * recovery), never as routine padding.  A much larger queue means the
 * producer stalled; only then is stale audio discarded and the cushion
 * rebuilt. */
#define AUDIO_LOW_MS     60
#define AUDIO_TARGET_MS  90
#define AUDIO_HIGH_MS   250

/* ponytail: a fixed layout; remapping belongs to the frontend (ROADMAP.md) */
static void pad_added(int index) {
    for (int i = 0; i < 2; i++)
        if (!s_pad[i]) {
            s_pad[i] = SDL_GameControllerOpen(index);
            if (s_pad[i]) fprintf(stderr, "gamepad: %s is player %d\n", SDL_GameControllerName(s_pad[i]), i + 1);
            return;
        }
}

static void pad_removed(SDL_JoystickID id) {
    for (int i = 0; i < 2; i++)
        if (s_pad[i] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(s_pad[i])) == id) {
            SDL_GameControllerClose(s_pad[i]);
            s_pad[i] = NULL;
        }
}

int window_open(const char *title, int scale, int fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "error: SDL_Init: %s\n", SDL_GetError());
        return -1;
    }
    Uint32 flags = SDL_WINDOW_RESIZABLE;
    int w = NG_SCREEN_W * scale, h = NG_SCREEN_H * scale;
    if (fullscreen) { flags |= SDL_WINDOW_FULLSCREEN_DESKTOP; w = 0; h = 0; }
    s_fullscreen = fullscreen;
    s_win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             w, h, flags);
    s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_ACCELERATED);
    s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                              NG_SCREEN_W, NG_SCREEN_H);
    s_next_tick = SDL_GetPerformanceCounter();
    /* The YM2610's 55,555 Hz is not a rate sound cards commonly accept.
     * Open the device at whatever it supports and convert with SDL's
     * audio stream, instead of letting SDL_OpenAudioDevice fail (which
     * left the game silent on some Windows and SteamOS setups). */
    SDL_AudioSpec want = {0}, have;
    want.freq = NG_AUDIO_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0)
        s_audio = SDL_OpenAudioDevice(NULL, 0, &want, &have,
                                      SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                                      SDL_AUDIO_ALLOW_FORMAT_CHANGE |
                                      SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (s_audio) {
        s_audio_spec = have;
        s_audio_frame_bytes = (Uint32)SDL_AUDIO_BITSIZE(have.format) / 8u * have.channels;
        s_audio_stream = SDL_NewAudioStream(AUDIO_S16SYS, 2, NG_AUDIO_RATE,
                                            have.format, have.channels, have.freq);
        if (!s_audio_stream) {
            fprintf(stderr, "warning: no sound: %s\n", SDL_GetError());
            SDL_CloseAudioDevice(s_audio);
            s_audio = 0;
        }
    } else {
        fprintf(stderr, "warning: no sound: %s\n", SDL_GetError());
    }
    /* Pads already connected arrive as SDL_CONTROLLERDEVICEADDED events. */
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        fprintf(stderr, "warning: no gamepads: %s\n", SDL_GetError());
    return 0;
}

static Uint32 audio_ms(double ms) {
    if (!s_audio_frame_bytes) return 0;
    Uint32 v = (Uint32)((double)s_audio_spec.freq * s_audio_frame_bytes * (ms / 1000.0));
    return v - v % s_audio_frame_bytes;
}

/* Convert the YM2610's samples to the device's rate and queue them.  The
 * old path queued at 55,555 Hz and inserted silence whenever the queue
 * dipped below 30 ms, then threw a whole 17 ms frame away above 120 ms:
 * scheduling jitter made that happen regularly, which is audible as clicks
 * and dropouts.  Here the stream resamples and the queue is not padded in
 * normal operation; if an underrun does happen, silence is inserted only
 * to recover the cushion. */
void window_audio(const int16_t *samples, size_t frames) {
    if (!s_audio || !s_audio_stream || !s_audio_frame_bytes || frames == 0) return;
    if (SDL_AudioStreamPut(s_audio_stream, samples, (int)(frames * 4)) < 0) {
        if (!s_audio_warned++)
            fprintf(stderr, "warning: audio stream input: %s\n", SDL_GetError());
        return;
    }
    int avail = SDL_AudioStreamAvailable(s_audio_stream);
    if (avail <= 0) return;

    Uint32 high = audio_ms(AUDIO_HIGH_MS);
    Uint32 low = audio_ms(AUDIO_LOW_MS);
    Uint32 target = audio_ms(AUDIO_TARGET_MS);

    if (SDL_GetQueuedAudioSize(s_audio) > high && s_audio_started) {
        /* The producer stalled for longer than any normal frame jitter.
         * Drop stale device audio and rebuild the cushion.  The converted
         * data for the current frame is kept and queued below. */
        SDL_ClearQueuedAudio(s_audio);
        s_audio_started = 0;
        s_audio_stalls++;
    }

    Uint8 buf[16384];
    while (avail > 0) {
        int want = avail < (int)sizeof(buf) ? avail : (int)sizeof(buf);
        want -= want % (int)s_audio_frame_bytes;
        if (want <= 0) break;
        int got = SDL_AudioStreamGet(s_audio_stream, buf, want);
        if (got <= 0) break;
        if (SDL_QueueAudio(s_audio, buf, (Uint32)got) < 0) {
            if (!s_audio_warned++)
                fprintf(stderr, "warning: audio queue: %s\n", SDL_GetError());
            break;
        }
        avail -= got;
    }

    Uint32 queued = SDL_GetQueuedAudioSize(s_audio);
    if (!s_audio_started) {
        if (queued >= low) {
            SDL_PauseAudioDevice(s_audio, 0);
            s_audio_started = 1;
        }
    } else if (queued < low) {
        /* The device is about to run dry.  Top the cushion up with silence,
         * which is preferable to an audible underrun; no real samples are
         * dropped. */
        Uint32 pad = target - queued;
        pad -= pad % s_audio_frame_bytes;
        static const Uint8 silence[4096];
        while (pad > 0) {
            Uint32 n = pad < sizeof(silence) ? pad : (Uint32)sizeof(silence);
            n -= n % s_audio_frame_bytes;
            if (!n) break;
            if (SDL_QueueAudio(s_audio, silence, n) < 0) break;
            pad -= n;
        }
        s_audio_underruns++;
    }
}

static void read_keys(ng_input_t *in) {
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    if (k[SDL_SCANCODE_UP]) in->p1 |= NG_UP;
    if (k[SDL_SCANCODE_DOWN]) in->p1 |= NG_DOWN;
    if (k[SDL_SCANCODE_LEFT]) in->p1 |= NG_LEFT;
    if (k[SDL_SCANCODE_RIGHT]) in->p1 |= NG_RIGHT;
    if (k[SDL_SCANCODE_Z]) in->p1 |= NG_A;
    if (k[SDL_SCANCODE_X]) in->p1 |= NG_B;
    if (k[SDL_SCANCODE_C]) in->p1 |= NG_C;
    if (k[SDL_SCANCODE_V]) in->p1 |= NG_D;
    in->start1 = k[SDL_SCANCODE_1];
    in->start2 = k[SDL_SCANCODE_2];
    in->select1 = k[SDL_SCANCODE_3];
    in->coin1 = k[SDL_SCANCODE_5];
    in->coin2 = k[SDL_SCANCODE_6];
    in->service = k[SDL_SCANCODE_9];
    in->test = k[SDL_SCANCODE_F2];
}

static void read_pad(SDL_GameController *c, uint8_t *bits, uint8_t *start, uint8_t *coin, uint8_t *select) {
    enum { DEAD = 12000 };   /* about a third of the stick's travel */
    int x = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
    int y = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
#define PAD(b) SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_##b)
    if (PAD(DPAD_UP) || y < -DEAD) *bits |= NG_UP;
    if (PAD(DPAD_DOWN) || y > DEAD) *bits |= NG_DOWN;
    if (PAD(DPAD_LEFT) || x < -DEAD) *bits |= NG_LEFT;
    if (PAD(DPAD_RIGHT) || x > DEAD) *bits |= NG_RIGHT;
    if (PAD(A)) *bits |= NG_A;
    if (PAD(B)) *bits |= NG_B;
    if (PAD(X)) *bits |= NG_C;
    if (PAD(Y)) *bits |= NG_D;
    *start |= PAD(START);
    *coin |= PAD(BACK);
    *select |= PAD(LEFTSHOULDER);
#undef PAD
}

bool window_frame(const uint32_t *fb, ng_input_t *in) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) return false;
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11) {
            s_fullscreen = !s_fullscreen;
            SDL_SetWindowFullscreen(s_win, s_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        }
        if (e.type == SDL_CONTROLLERDEVICEADDED) pad_added(e.cdevice.which);
        if (e.type == SDL_CONTROLLERDEVICEREMOVED) pad_removed(e.cdevice.which);
    }
    read_keys(in);
    if (s_pad[0]) read_pad(s_pad[0], &in->p1, &in->start1, &in->coin1, &in->select1);
    if (s_pad[1]) read_pad(s_pad[1], &in->p2, &in->start2, &in->coin2, &in->select2);
    if (fb) {
        SDL_UpdateTexture(s_tex, NULL, fb, NG_SCREEN_W * 4);
        SDL_RenderClear(s_ren);
        int ow = 0, oh = 0;
        SDL_GetRendererOutputSize(s_ren, &ow, &oh);
        SDL_Rect dst = {0, 0, ow, oh};
        if (s_fullscreen && ow > 0 && oh > 0) {
            /* The Neo Geo's 320x224 is a 4:3 display on a CRT. Fit that into
             * the 16:10 panel and pillarbox; never stretch to the panel. */
            int vw = oh * 4 / 3, vh = oh;
            if (vw > ow) { vw = ow; vh = ow * 3 / 4; }
            dst.x = (ow - vw) / 2; dst.y = (oh - vh) / 2;
            dst.w = vw; dst.h = vh;
        }
        SDL_RenderCopy(s_ren, s_tex, NULL, &dst);
        if (getenv("NG_WINDOW_TRACE")) {
            static int traced;
            if (!traced++)
                fprintf(stderr, "[window] output %dx%d dst %d,%d %dx%d fullscreen=%d\n",
                        ow, oh, dst.x, dst.y, dst.w, dst.h, s_fullscreen);
        }
        SDL_RenderPresent(s_ren);
    }
    /* Pace to the MVS refresh rate, 59.1856 Hz. */
    uint64_t freq = SDL_GetPerformanceFrequency();
    s_next_tick += (uint64_t)((double)freq / 59.185606);
    uint64_t now = SDL_GetPerformanceCounter();
    if (now < s_next_tick) SDL_Delay((Uint32)((s_next_tick - now) * 1000 / freq));
    else if (now - s_next_tick > freq / 10) s_next_tick = now;
    return true;
}

void window_close(void) {
    for (int i = 0; i < 2; i++)
        if (s_pad[i]) SDL_GameControllerClose(s_pad[i]);
    if (s_audio_stream) SDL_FreeAudioStream(s_audio_stream);
    if (s_audio) SDL_CloseAudioDevice(s_audio);
    if (s_tex) SDL_DestroyTexture(s_tex);
    if (s_ren) SDL_DestroyRenderer(s_ren);
    if (s_win) SDL_DestroyWindow(s_win);
    SDL_Quit();
}
