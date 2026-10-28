/* Tiny software synth: procedural SFX and seeded music, mixed to 16-bit mono 22050 Hz. */
#include "ht.h"
#include <math.h>

typedef struct { float f0, f1, dur, vol; int wave; } Seg;   /* wave: 0 square 1 tri 2 noise 3 sine */
typedef struct { Seg s[6]; int n; } Sfx;

#define PI 3.14159265f
static const Sfx sfx_tab[N_SFX] = {
 /* CLICK  */ {{{900,500,0.025f,0.5f,0}}, 1},
 /* FIRE   */ {{{1200,200,0.18f,0.6f,0},{300,60,0.1f,0.4f,2}}, 2},
 /* HIT    */ {{{200,40,0.22f,0.8f,2},{400,80,0.2f,0.6f,0}}, 2},
 /* MISS   */ {{{500,250,0.12f,0.35f,3},{250,120,0.12f,0.3f,3}}, 2},
 /* SUNK   */ {{{300,50,0.35f,0.9f,2},{110,40,0.4f,0.7f,0},{440,880,0.12f,0.5f,0},{660,1320,0.2f,0.5f,0}}, 4},
 /* CORRECT*/ {{{660,660,0.08f,0.5f,0},{880,880,0.08f,0.5f,0},{1320,1320,0.18f,0.5f,0}}, 3},
 /* WRONG  */ {{{220,200,0.18f,0.6f,0},{150,110,0.3f,0.6f,0}}, 2},
 /* COIN   */ {{{990,990,0.06f,0.5f,0},{1480,1480,0.2f,0.5f,0}}, 2},
 /* DING   */ {{{1760,1760,0.35f,0.4f,3}}, 1},
 /* FANFARE*/ {{{523,523,0.14f,0.5f,0},{659,659,0.14f,0.5f,0},{784,784,0.14f,0.5f,0},{1047,1047,0.35f,0.55f,0}}, 4},
 /* ERROR  */ {{{440,440,0.1f,0.5f,0},{0,0,0.03f,0,0},{440,440,0.18f,0.5f,0}}, 3},
 /* POWERUP*/ {{{330,330,0.06f,0.45f,0},{440,440,0.06f,0.45f,0},{660,660,0.06f,0.45f,0},{880,1320,0.2f,0.5f,0}}, 4},
 /* ALARM  */ {{{800,800,0.12f,0.5f,0},{600,600,0.12f,0.5f,0},{800,800,0.12f,0.5f,0},{600,600,0.12f,0.5f,0}}, 4},
 /* BOSS   */ {{{110,110,0.3f,0.7f,0},{103,103,0.3f,0.7f,0},{98,98,0.5f,0.7f,0}}, 3},
 /* TYPE   */ {{{1400,1000,0.02f,0.3f,2}}, 1},
 /* GAMEOVR*/ {{{392,392,0.2f,0.5f,1},{370,370,0.2f,0.5f,1},{349,349,0.2f,0.5f,1},{262,131,0.8f,0.6f,1}}, 4},
 /* SHUFFLE*/ {{{0,0,0.05f,0.4f,2},{0,0,0.05f,0.4f,2},{0,0,0.05f,0.4f,2}}, 3},
 /* SPLASH */ {{{600,300,0.15f,0.35f,2}}, 1},
};

typedef struct { int active, seg; float t, phase, dur_acc; const Sfx *sfx; } Voice;
#define NVOICE 8
static Voice voices[NVOICE];
static int en_sfx = 1, en_mus = 1;
static unsigned int noise_s = 22222u;

typedef struct { int style, step, bpm, root, nscale; int scale[8]; int prog[4]; int bar; Rng rng;
    float lead_f, lead_t, lead_ph, bass_f, bass_ph, bass_t, perc_t; int lead_wave; int lead_len; int started; float master; } Music;
static Music mus;
static int mus_samples_to_step;

static float noise(void) { noise_s = noise_s * 1664525u + 1013904223u; return ((noise_s >> 9) & 0xFFFF) / 32768.0f - 1.0f; }
static float osc(int wave, float ph) {
    float p = ph - floorf(ph);
    switch (wave) {
    case 0: return p < 0.5f ? 0.6f : -0.6f;
    case 1: return (p < 0.5f ? p * 4 - 1 : 3 - p * 4) * 0.8f;
    case 2: return noise() * 0.7f;
    default: return sinf(p * 2 * PI) * 0.7f;
    }
}

void audio_enable(int sfx, int music) { en_sfx = sfx; en_mus = music; if (!music) mus.style = MUS_NONE; }
void audio_init(void) { memset(voices, 0, sizeof voices); memset(&mus, 0, sizeof mus); en_sfx = 1; en_mus = 1; }

void audio_sfx(int id) {
    int i, slot = 0;
    if (!en_sfx || id < 0 || id >= N_SFX) return;
    for (i = 0; i < NVOICE; i++) if (!voices[i].active) { slot = i; break; }
    voices[slot].active = 1; voices[slot].seg = 0; voices[slot].t = 0; voices[slot].phase = 0; voices[slot].sfx = &sfx_tab[id];
}

/* scales as semitone offsets */
static const int sc_major_pent[8] = {0, 2, 4, 7, 9, 12, 14, 16};
static const int sc_minor[8]      = {0, 2, 3, 5, 7, 8, 10, 12};
static const int sc_phrygian[8]   = {0, 1, 3, 5, 7, 8, 10, 12};
static const int sc_major[8]      = {0, 2, 4, 5, 7, 9, 11, 12};

static float mtof(int midi) { return 440.0f * powf(2.0f, (midi - 69) / 12.0f); }

void audio_music(int style, unsigned int seed) {
    int i;
    if (style == mus.style && mus.started) return;
    memset(&mus, 0, sizeof mus);
    mus.style = style; mus.started = 1;
    rng_seed(&mus.rng, seed * 31u + (unsigned)style * 977u);
    mus.master = 1.0f;
    switch (style) {
    case MUS_ELEVATOR: mus.bpm = 84;  mus.root = 60 + rng_n(&mus.rng, 5); memcpy(mus.scale, sc_major_pent, sizeof mus.scale); mus.lead_wave = 3; break;
    case MUS_BATTLE:   mus.bpm = 128; mus.root = 45 + rng_n(&mus.rng, 5); memcpy(mus.scale, sc_minor, sizeof mus.scale); mus.lead_wave = 0; break;
    case MUS_BOSS:     mus.bpm = 152; mus.root = 40 + rng_n(&mus.rng, 4); memcpy(mus.scale, sc_phrygian, sizeof mus.scale); mus.lead_wave = 0; break;
    case MUS_SHOP:     mus.bpm = 100; mus.root = 55 + rng_n(&mus.rng, 5); memcpy(mus.scale, sc_major, sizeof mus.scale); mus.lead_wave = 1; break;
    case MUS_MENU:     mus.bpm = 76;  mus.root = 57 + rng_n(&mus.rng, 4); memcpy(mus.scale, sc_major_pent, sizeof mus.scale); mus.lead_wave = 1; break;
    case MUS_VICTORY:  mus.bpm = 132; mus.root = 60; memcpy(mus.scale, sc_major, sizeof mus.scale); mus.lead_wave = 0; break;
    case MUS_DEAD:     mus.bpm = 60;  mus.root = 50; memcpy(mus.scale, sc_minor, sizeof mus.scale); mus.lead_wave = 1; break;
    default: mus.style = MUS_NONE; return;
    }
    for (i = 0; i < 4; i++) mus.prog[i] = (int[]){0, 3, 4, 0, 5, 2, 4, 1}[rng_n(&mus.rng, 8)];
    mus.prog[0] = 0;
    mus_samples_to_step = 0;
}

static void music_step(void) {
    int st = mus.step & 15, bar = (mus.step >> 4) & 3;
    int chord = mus.prog[bar];
    int bass_midi = mus.root - 12 + mus.scale[chord % 8] % 12;
    int density = mus.style == MUS_BOSS || mus.style == MUS_BATTLE ? 60 : mus.style == MUS_DEAD ? 25 : 45;
    /* bass on beats / off-beats */
    if (st % 4 == 0 || (mus.style == MUS_BATTLE && st % 4 == 2) || (mus.style == MUS_BOSS && st % 2 == 0)) {
        mus.bass_f = mtof(bass_midi); mus.bass_t = 0.9f;
        if (mus.style == MUS_BATTLE || mus.style == MUS_BOSS) mus.bass_t = 0.55f;
    }
    /* lead */
    if (rng_pct(&mus.rng, density) || st == 0) {
        int deg = rng_n(&mus.rng, 8);
        if (st == 0 || st == 8) deg = (chord + (rng_n(&mus.rng, 2) ? 0 : 2)) % 8;
        mus.lead_f = mtof(mus.root + 12 + mus.scale[deg]);
        mus.lead_t = 1.0f; mus.lead_len = 2 + rng_n(&mus.rng, 3);
    }
    /* percussion tick */
    if (mus.style != MUS_ELEVATOR && mus.style != MUS_DEAD && mus.style != MUS_MENU) {
        if (st % 4 == 0) mus.perc_t = 1.0f;            /* kick-ish */
        else if (st % 4 == 2 && mus.style != MUS_SHOP) mus.perc_t = 0.6f;
    } else if (st % 8 == 0 && mus.style != MUS_DEAD) mus.perc_t = 0.4f;
    if (mus.style == MUS_VICTORY && st == 0 && bar == 0) mus.lead_t = 1.0f;
    mus.step++;
}

void audio_mix(short *out, int n) {
    int i, v;
    for (i = 0; i < n; i++) {
        float s = 0;
        /* music */
        if (en_mus && mus.style != MUS_NONE) {
            int step_len = AUDIO_RATE * 60 / mus.bpm / 4;
            if (mus_samples_to_step <= 0) { music_step(); mus_samples_to_step = step_len; }
            mus_samples_to_step--;
            if (mus.lead_t > 0) {
                mus.lead_ph += mus.lead_f / AUDIO_RATE;
                s += osc(mus.lead_wave, mus.lead_ph) * mus.lead_t * 0.16f;
                mus.lead_t -= 1.0f / (step_len * (float)mus.lead_len);
            }
            if (mus.bass_t > 0) {
                mus.bass_ph += mus.bass_f / AUDIO_RATE;
                s += osc(1, mus.bass_ph) * mus.bass_t * 0.22f;
                mus.bass_t -= 1.0f / (step_len * 3.0f);
            }
            if (mus.perc_t > 0) {
                s += noise() * mus.perc_t * 0.10f;
                mus.perc_t -= 1.0f / (AUDIO_RATE * 0.05f);
            }
        }
        /* sfx voices */
        for (v = 0; v < NVOICE; v++) {
            Voice *vo = &voices[v];
            const Seg *sg;
            float u, f;
            if (!vo->active) continue;
            sg = &vo->sfx->s[vo->seg];
            u = vo->t / sg->dur;
            f = sg->f0 + (sg->f1 - sg->f0) * u;
            vo->phase += f / AUDIO_RATE;
            if (sg->vol > 0) s += osc(sg->wave, vo->phase) * sg->vol * (1.0f - 0.3f * u) * 0.5f;
            vo->t += 1.0f / AUDIO_RATE;
            if (vo->t >= sg->dur) { vo->t = 0; vo->seg++; if (vo->seg >= vo->sfx->n) vo->active = 0; }
        }
        if (s > 1.0f) s = 1.0f; else if (s < -1.0f) s = -1.0f;
        out[i] = (short)(s * 20000.0f);
    }
}
