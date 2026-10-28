/* Battle UI: Space Planning (placement), the two boards, effects, enemy turn, outcomes. */
#include "ui.h"

int g_battle_busy;

/* ------------------------------------------------------------------ effects */
typedef struct { int on, side, gx, gy, color; float y, vy; int life; char txt[20]; } Float;
typedef struct { int on, side, gx, gy, kind, t; } Boom;
static Float floats[24];
static Boom booms[24];
static char logbuf[12][160];
static int logcol[12], lognum;
static char banner_txt[40];
static int banner_t, shake_t, shake_amt, shake_ox, shake_oy;

static int pend_end;          /* 0 none, 1 win, -1 office sunk, -2 burnout */
static int end_timer;
enum { ES_NONE, ES_WAIT, ES_AIM };
static int es_state, es_timer, es_shots, es_x, es_y;
static int waiting_reply, waiting_train, train_module;
static int armed = -1;
static int auto_timer;
static int pick = -1, pick_rot;
static int hint_scr_shown;

/* layout */
static int cs_foe(void) { int s = g_bt.foe.w; return s <= 8 ? 28 : s <= 9 ? 26 : s <= 10 ? 24 : s <= 11 ? 22 : 20; }
static int cs_you(void) { int s = g_bt.you.w; return s <= 8 ? 24 : s <= 9 ? 22 : 20; }
#define FOE_X 26
#define FOE_Y 96
static int you_x(void) { return 626 - g_bt.you.w * cs_you(); }
#define YOU_Y 96

static void cell_px(int side, int gx, int gy, int *px, int *py) {
    if (side == 1) { *px = FOE_X + gx * cs_foe() + cs_foe() / 2; *py = FOE_Y + gy * cs_foe() + cs_foe() / 2; }
    else { *px = you_x() + gx * cs_you() + cs_you() / 2; *py = YOU_Y + gy * cs_you() + cs_you() / 2; }
}

void fx_log(int color, const char *fmt, ...) {
    va_list ap;
    int i;
    if (lognum >= 12) { for (i = 1; i < 12; i++) { memcpy(logbuf[i - 1], logbuf[i], sizeof logbuf[0]); logcol[i - 1] = logcol[i]; } lognum = 11; }
    va_start(ap, fmt); vsnprintf(logbuf[lognum], sizeof logbuf[0], fmt, ap); va_end(ap);
    logcol[lognum++] = color;
}
void fx_float(int side, int x, int y, int color, const char *txt) {
    int i;
    for (i = 0; i < 24; i++) if (!floats[i].on) {
        Float *f = &floats[i];
        f->on = 1; f->side = side; f->gx = x; f->gy = y; f->color = color; f->life = 1100; f->vy = 0; f->y = 0;
        snprintf(f->txt, sizeof f->txt, "%s", txt); return;
    }
}
void fx_boom(int side, int x, int y, int kind) {
    int i;
    for (i = 0; i < 24; i++) if (!booms[i].on) { booms[i].on = 1; booms[i].side = side; booms[i].gx = x; booms[i].gy = y; booms[i].kind = kind; booms[i].t = 0; break; }
    switch (kind) {
    case SH_HIT: audio_sfx(SFX_HIT); shake_t = 160; shake_amt = 2; break;
    case SH_SUNK: audio_sfx(SFX_SUNK); shake_t = 380; shake_amt = 5; break;
    case SH_MISS: audio_sfx(SFX_MISS); break;
    case SH_SHIELD: case SH_LOCKED: audio_sfx(SFX_ERROR); break;
    default: break;
    }
}
void fx_banner(const char *txt) { if (!txt) return; snprintf(banner_txt, sizeof banner_txt, "%s", txt); banner_t = 1700; audio_sfx(SFX_POWERUP); }
void fx_shake(int amt) { shake_t = 250; shake_amt = amt; }

void battle_reset_fx(void) {
    memset(floats, 0, sizeof floats); memset(booms, 0, sizeof booms);
    lognum = 0; banner_t = 0; shake_t = 0; pend_end = 0; end_timer = 0; es_state = ES_NONE; es_shots = 0;
    waiting_reply = waiting_train = 0; armed = -1; auto_timer = 0; pick = -1;
}

void battle_update_fx(int dt) {
    int i;
    for (i = 0; i < 24; i++) {
        if (floats[i].on) { floats[i].life -= dt; floats[i].y -= dt * 0.03f; if (floats[i].life <= 0) floats[i].on = 0; }
        if (booms[i].on) { booms[i].t += dt; if (booms[i].t > 520) booms[i].on = 0; }
    }
    if (banner_t > 0) banner_t -= dt;
    if (shake_t > 0) { shake_t -= dt; shake_ox = (int)(g_rng.s >> 3) % (2 * shake_amt + 1) - shake_amt; rng_u32(&g_rng); shake_oy = (int)(g_rng.s >> 5) % (2 * shake_amt + 1) - shake_amt; rng_u32(&g_rng); }
    else { shake_ox = shake_oy = 0; }
}

/* ------------------------------------------------------------------ drawing cells */
static void carpet(int px, int py, int cs, int style, int pat) {
    static const int ca[4] = {C_TEAL, C_MAROON, C_OLIVE, C_GRAY}, cb[4] = {C_NAVY, C_BLACK, C_GREEN, C_BLACK};
    gfx_rect(px, py, cs, cs, ca[style & 3]);
    switch (pat & 3) {
    case 0: gfx_dither(px, py, cs, cs, cb[style & 3], ca[style & 3], 0); break;
    case 1: gfx_dither(px, py, cs, cs, ca[style & 3], cb[style & 3], 1); break;
    case 2: { int k; for (k = 0; k < cs; k += 4) gfx_line(px, py + k, px + k, py, cb[style & 3]); } break;
    default: gfx_pixel(px + cs / 2, py + cs / 2, cb[style & 3]); gfx_pixel(px + 2, py + 2, cb[style & 3]); gfx_pixel(px + cs - 3, py + cs - 3, cb[style & 3]); break;
    }
}
static void sprc(int px, int py, int cs, const Spr *s) { gfx_spr(px + (cs - s->w) / 2, py + (cs - s->h) / 2, s); }

static void draw_foe_cell(int gx, int gy, int px, int py, int cs) {
    const Cell *c = &g_bt.foe.c[gy][gx];
    carpet(px, py, cs, g_bt.carpet, g_bt.carpet_pat);
    switch (c->state) {
    case CS_MISS: gfx_rect(px, py, cs, cs, C_SILVER); gfx_dither(px, py, cs, cs, C_SILVER, C_WHITE, 1); sprc(px, py, cs, &S_MISS); break;
    case CS_HIT: gfx_rect(px, py, cs, cs, C_MAROON); gfx_dither(px, py, cs, cs, C_RED, C_MAROON, 0); sprc(px, py, cs, &S_BOOM); break;
    case CS_SUNK: {
        const Asset *a = &g_bt.foe.a[c->asset];
        gfx_rect(px, py, cs, cs, g_adef[a->def].color); gfx_dither(px, py, cs, cs, C_BLACK, g_adef[a->def].color, 1);
        gfx_frame(px, py, cs, cs, C_RED);
        if (c->part == 0) sprc(px, py, cs, &S_ASSET[a->def]); else { gfx_line(px + 3, py + 3, px + cs - 4, py + cs - 4, C_BLACK); gfx_line(px + cs - 4, py + 3, px + 3, py + cs - 4, C_BLACK); }
        break; }
    default:
        if (c->known & K_HAZ) sprc(px, py, cs, &S_HAZ[c->haz]);
        if (c->known & K_STICKY) { if (c->asset >= 0) sprc(px, py, cs, &S_NOTE); else { gfx_line(px + 5, py + 5, px + cs - 6, py + cs - 6, C_YELLOW); gfx_line(px + cs - 6, py + 5, px + 5, py + cs - 6, C_YELLOW); } }
        if (c->known & K_SHIELD) gfx_frame(px + 2, py + 2, cs - 4, cs - 4, C_AQUA);
        if (c->lock) { sprc(px, py, cs, &S_LOCK); gfx_dither(px, py, cs, cs, C_RED, C_BLACK, 1); sprc(px, py, cs, &S_LOCK); }
        if (c->scan >= 0) { char b[12]; snprintf(b, sizeof b, "%d", c->scan); gfx_text_big(px + cs / 2 - 3, py + cs / 2 - 6, b, C_BLACK, 2); gfx_text_big(px + cs / 2 - 4, py + cs / 2 - 7, b, C_WHITE, 2); }
        if (c->known & K_FLAG) sprc(px, py, cs, &S_FLAG);
        break;
    }
    gfx_frame(px, py, cs, cs, C_BLACK);
}

static void draw_you_cell(int gx, int gy, int px, int py, int cs) {
    const Board *bd = &g_bt.you;
    const Cell *c = &bd->c[gy][gx];
    carpet(px, py, cs, g_bt.carpet2, 3 - (g_bt.carpet_pat & 3));
    if (c->asset >= 0) {
        const Asset *a = &bd->a[c->asset];
        int col = g_adef[a->def].color;
        gfx_rect(px + 1, py + 1, cs - 2, cs - 2, col);
        gfx_bevel(px + 1, py + 1, cs - 2, cs - 2, C_WHITE, C_BLACK);
        if (c->part == 0) sprc(px, py, cs, &S_ASSET[a->def]);
        if (c->state == CS_HIT || c->state == CS_SUNK) { gfx_dither(px, py, cs, cs, C_RED, col, 1); sprc(px, py, cs, &S_BOOM); }
        if (c->state == CS_SUNK) { gfx_frame(px + 1, py + 1, cs - 2, cs - 2, C_RED); }
    } else if (c->state == CS_MISS) sprc(px, py, cs, &S_MISS);
    gfx_frame(px, py, cs, cs, C_BLACK);
}

/* ------------------------------------------------------------------ shared small UI */
static void draw_status(Rect c) {
    char b[80];
    int i;
    Rect r = R(c.x + 2, c.y + 2, c.w - 4, 22);
    ui_sunken(R(r.x, r.y, r.w, r.h + 0));
    snprintf(b, sizeof b, "Morale %d/%d", g_run.morale, run_max_morale(&g_run));
    ui_bar(R(r.x + 3, r.y + 3, 150, 16), g_run.morale, run_max_morale(&g_run), g_run.morale * 3 < run_max_morale(&g_run) ? C_RED : C_GREEN, b);
    snprintf(b, sizeof b, "Synergy %d%%", g_run.synergy);
    ui_bar(R(r.x + 160, r.y + 3, 110, 16), g_run.synergy, 100, (g_run.synergy >= 100 && ((g_time / 250) & 1)) ? C_YELLOW : C_FUCHSIA, b);
    gfx_text_b(r.x + 280, r.y + 8, "Actions:", C_BLACK);
    for (i = 0; i < 6; i++) { int on = i < g_bt.ap; gfx_rect(r.x + 336 + i * 12, r.y + 5, 10, 12, on ? C_NAVY : C_WHITE); gfx_frame(r.x + 336 + i * 12, r.y + 5, 10, 12, C_BLACK); }
    if (g_bt.ap > 6) { snprintf(b, sizeof b, "+%d", g_bt.ap - 6); gfx_text(r.x + 336 + 72, r.y + 8, b, C_BLACK); }
    gfx_spr(r.x + 420, r.y + 3, &S_COIN);
    snprintf(b, sizeof b, "$%d", g_run.cash); gfx_text_b(r.x + 438, r.y + 8, b, C_BLACK);
    snprintf(b, sizeof b, "Score %ld", g_run.score); gfx_text(r.x + 480, r.y + 8, b, C_NAVY);
    snprintf(b, sizeof b, "T%d", g_bt.turn); gfx_text(r.x + 580, r.y + 8, b, C_GRAY);
}

static void draw_log(Rect r) {
    int i;
    gfx_rect(r.x, r.y, r.w, r.h, C_BLACK); ui_sunken(r);
    gfx_clip(R(r.x + 2, r.y + 2, r.w - 4, r.h - 4));
    for (i = 0; i < lognum; i++) {
        int y = r.y + r.h - 4 - (lognum - i) * 10;
        gfx_text(r.x + 6, y, logbuf[i], logcol[i]);
    }
    gfx_unclip();
}

/* ------------------------------------------------------------------ enemy intel text */
static const char *ai_desc[5] = {"fires at random", "hunts around hits", "hunts on a checkerboard", "calculates probabilities", "sweeps row by row"};
static void rules_text(const FloorDef *fd, char *out, int max) {
    out[0] = 0;
    if (fd->rules & RULE_REORG) strncat(out, "RE-ORG ", (size_t)max - strlen(out) - 1);
    if (fd->rules & RULE_FIREWALL) strncat(out, "FIREWALL ", (size_t)max - strlen(out) - 1);
    if (fd->rules & RULE_DOUBLE) strncat(out, "OVERTIME ", (size_t)max - strlen(out) - 1);
    if (fd->rules & RULE_INJUNCTION) strncat(out, "INJUNCTION ", (size_t)max - strlen(out) - 1);
    if (fd->rules & RULE_AUDIT) strncat(out, "AUDIT ", (size_t)max - strlen(out) - 1);
    if (!out[0]) strncat(out, "none", (size_t)max - 1);
}

/* ------------------------------------------------------------------ battle entry */
void battle_enter(int unused) {
    (void)unused;
    if (g_scr == SC_PLACE) {
        battle_new(&g_bt, &g_run);
        battle_reset_fx();
        pick = -1; pick_rot = 0;
        audio_music(g_bt.fd.boss ? MUS_BOSS : MUS_BATTLE, g_run.seed + (unsigned)g_run.floor);
        if (g_bt.fd.boss) audio_sfx(SFX_BOSS);
        fx_log(C_YELLOW, "Floor %d: %s.", floor_label(g_run.floor), g_bt.fd.dept);
        { Rng r; char memo[120]; rng_seed(&r, g_run.seed + (unsigned)g_run.floor * 13u); gen_memo(&r, memo, sizeof memo); fx_log(C_SILVER, "%s", memo); }
        fx_log(C_WHITE, "Arrange your Assets, then press READY.");
    }
}

static void finish_check(int st);
void battle_resume(void) {
    battle_reset_fx();
    pick = -1; pick_rot = 0; armed = -1; waiting_reply = waiting_train = 0; 
    fx_log(C_YELLOW, "Welcome back to Floor %d: %s.", floor_label(g_run.floor), g_bt.fd.dept);
    if (g_bt.phase == PH_PLACE) fx_log(C_WHITE, "Arrange your Assets, then press READY.");
    else fx_log(C_WHITE, "--- Turn %d: you have %d Action(s) ---", g_bt.turn, g_bt.ap);
    if (g_bt.phase != PH_PLACE) { int st = bt_status(&g_bt, &g_run); if (st) finish_check(st); }
}

/* ------------------------------------------------------------------ Space Planning */
void draw_boss_portrait(int x, int y, int scale, unsigned int seed) {
    Rng r;
    int skin = (rng_seed(&r, seed), rng_n(&r, 3)), hair = rng_n(&r, 4), tie = rng_n(&r, 4), s = scale;
    static const int skins[3] = {C_YELLOW, C_SILVER, C_WHITE};
    static const int hairs[4] = {C_BLACK, C_GRAY, C_MAROON, C_WHITE};
    static const int ties[4] = {C_RED, C_BLUE, C_FUCHSIA, C_GREEN};
    gfx_rect(x, y, 32 * s, 40 * s, C_NAVY); gfx_frame(x, y, 32 * s, 40 * s, C_BLACK);
    gfx_rect(x + 3 * s, y + 30 * s, 26 * s, 10 * s, C_BLACK);                     /* suit */
    gfx_rect(x + 12 * s, y + 30 * s, 8 * s, 10 * s, C_WHITE);                    /* shirt */
    gfx_rect(x + 15 * s, y + 31 * s, 2 * s, 9 * s, ties[tie]);                    /* tie */
    gfx_rect(x + 11 * s, y + 24 * s, 10 * s, 7 * s, skins[skin]);                 /* neck */
    gfx_rect(x + 8 * s, y + 8 * s, 16 * s, 18 * s, skins[skin]);                  /* head */
    gfx_rect(x + 7 * s, y + 6 * s, 18 * s, 5 * s, hairs[hair]);                   /* hair */
    gfx_rect(x + 10 * s, y + 14 * s, 4 * s, 3 * s, C_WHITE); gfx_rect(x + 18 * s, y + 14 * s, 4 * s, 3 * s, C_WHITE);
    gfx_rect(x + 11 * s, y + 15 * s, 2 * s, 2 * s, C_BLACK); gfx_rect(x + 19 * s, y + 15 * s, 2 * s, 2 * s, C_BLACK);
    gfx_frame(x + 9 * s, y + 13 * s, 6 * s, 5 * s, C_BLACK); gfx_frame(x + 17 * s, y + 13 * s, 6 * s, 5 * s, C_BLACK); gfx_hline(x + 15 * s, y + 15 * s, 2 * s, C_BLACK);
    gfx_rect(x + 11 * s, y + 21 * s, 10 * s, s, C_MAROON);                        /* smirk */
    gfx_rect(x + 20 * s, y + 20 * s, s, s, C_MAROON);
}

void place_draw(void) {
    Rect c;
    Board *bd = &g_bt.you;
    int cs = bd->w <= 8 ? 30 : bd->w <= 9 ? 28 : 26, ox = 24, oy = 84, i, x, y, ok = 1, gx, gy;
    char buf[100];
    c = ui_app("Hostile Takeover - Space Planning", 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    gfx_text_b(c.x + 10, c.y + 6, "SPACE PLANNING:  arrange your office before the enemy opens fire.", C_NAVY);
    snprintf(buf, sizeof buf, "Floor %d - %s", floor_label(g_run.floor), g_bt.fd.dept);
    gfx_text(c.x + 10, c.y + 20, buf, C_BLACK);
    /* board */
    for (y = 0; y < bd->h; y++) for (x = 0; x < bd->w; x++) draw_you_cell(x, y, ox + x * cs, oy + y * cs, cs);
    for (i = 0; i < bd->w; i++) { char t[4]; snprintf(t, sizeof t, "%c", 'A' + i); gfx_text(ox + i * cs + cs / 2 - 2, oy - 11, t, C_BLACK); }
    for (i = 0; i < bd->h; i++) { char t[12]; snprintf(t, sizeof t, "%d", i + 1); gfx_text(ox - 12 + (i + 1 < 10 ? 4 : 0), oy + i * cs + cs / 2 - 3, t, C_BLACK); }
    gx = (g_mx - ox) >= 0 ? (g_mx - ox) / cs : -1; gy = (g_my - oy) >= 0 ? (g_my - oy) / cs : -1;
    if (gx >= bd->w || gy >= bd->h) { gx = -1; gy = -1; }
    /* pick / place logic */
    if (pick >= 0) {
        int dx[6], dy[6], valid = 0, k;
        asset_shape(bd->a[pick].def, pick_rot, dx, dy);
        if (gx >= 0) {
            valid = board_can_place(bd, bd->a[pick].def, pick_rot, gx, gy, pick);
            for (k = 0; k < bd->a[pick].n; k++) {
                int cx = gx + dx[k], cy = gy + dy[k];
                if (cx >= 0 && cy >= 0 && cx < bd->w && cy < bd->h) { gfx_dither(ox + cx * cs, oy + cy * cs, cs, cs, valid ? C_LIME : C_RED, g_adef[bd->a[pick].def].color, 0); gfx_frame(ox + cx * cs, oy + cy * cs, cs, cs, valid ? C_LIME : C_RED); }
            }
        }
        if (g_press && gx >= 0) {
            if (valid) { board_place(bd, pick, gx, gy, pick_rot); audio_sfx(SFX_CLICK); pick = -1; }
            else audio_sfx(SFX_ERROR);
        }
        if (g_rpress || ui_key('R')) { pick_rot = (pick_rot + 1) & 3; audio_sfx(SFX_CLICK); }
        if (ui_key(KEY_ESC)) { if (!bd->a[pick].placed) board_random_place(bd, pick, &g_bt.rng); pick = -1; }
    } else if (g_press && gx >= 0 && bd->c[gy][gx].asset >= 0) {
        pick = bd->c[gy][gx].asset; pick_rot = bd->a[pick].orient; board_unplace(bd, pick); audio_sfx(SFX_CLICK);
    }
    for (i = 0; i < bd->na; i++) if (!bd->a[i].placed && i != pick) ok = 0;
    if (pick >= 0) ok = 0;
    /* right panel */
    { int px = 24 + bd->w * cs + 24, pw = c.x + c.w - px - 8, py = c.y + 34;
      Rect pr = R(px, py, pw, 150);
      char rt[80];
      gfx_rect(pr.x, pr.y, pr.w, pr.h, C_WHITE); ui_sunken(pr);
      gfx_text_b(pr.x + 8, pr.y + 6, "YOUR FLEET (click to pick up)", C_NAVY);
      for (i = 0; i < bd->na; i++) {
        int yy = pr.y + 20 + i * 12;
        if (yy > pr.y + pr.h - 12) break;
        if (i == pick) gfx_rect(pr.x + 4, yy - 1, pr.w - 8, 11, C_NAVY);
        gfx_spr(pr.x + 6, yy - 4, &S_ASSET[bd->a[i].def]);
        snprintf(buf, sizeof buf, "%s (%d)%s", g_adef[bd->a[i].def].name, bd->a[i].n, bd->a[i].placed ? "" : "  <-- place me");
        gfx_text(pr.x + 24, yy, buf, i == pick ? C_WHITE : C_BLACK);
      }
      py += 156;
      pr = R(px, py, pw, 128);
      gfx_rect(pr.x, pr.y, pr.w, pr.h, C_WHITE); ui_sunken(pr);
      snprintf(buf, sizeof buf, "INTEL: %s", g_bt.fd.dept); gfx_text_b(pr.x + 8, pr.y + 6, buf, C_MAROON);
      if (g_bt.fd.boss) { snprintf(buf, sizeof buf, "BOSS: %s", g_bt.boss_name); gfx_text_b(pr.x + 8, pr.y + 18, buf, C_RED); draw_boss_portrait(pr.x + pr.w - 74, pr.y + 8, 2, g_run.seed + (unsigned)g_run.floor); }
      gfx_text(pr.x + 8, pr.y + 32, g_bt.fd.tag, C_GRAY);
      snprintf(buf, sizeof buf, "Floor size: %dx%d   AI: %s", g_bt.foe.w, g_bt.foe.h, ai_desc[g_bt.fd.ai]);
      ui_wrap(R(pr.x + 8, pr.y + 46, pr.w - (g_bt.fd.boss ? 86 : 16), 20), buf, C_BLACK, 1);
      rules_text(&g_bt.fd, rt, sizeof rt); snprintf(buf, sizeof buf, "Rules: %s", rt);
      ui_wrap(R(pr.x + 8, pr.y + 70, pr.w - 16, 20), buf, C_PURPLE, 1);
      snprintf(buf, sizeof buf, "Enemy assets: %d   Damage per hit: %d", g_bt.foe.na, g_bt.fd.dmg);
      gfx_text(pr.x + 8, pr.y + 94, buf, C_BLACK);
      gfx_text(pr.x + 8, pr.y + 108, "Hazards lurk. Shoot carefully.", C_GRAY);
      if (ui_button(R(px, py + 138, 110, 24), "Shuffle (S)", 1) || ui_key('S')) { board_autoplace(bd, &g_bt.rng); pick = -1; audio_sfx(SFX_SHUFFLE); }
      if (ui_button_def(R(px + 120, py + 138, 140, 24), "READY! (Enter)", ok) || (ok && ui_key(KEY_RETURN))) {
        battle_begin(&g_bt, &g_run);
        g_scr = SC_BATTLE; fx_log(C_WHITE, "--- Turn 1: you have %d Action(s) ---", g_bt.ap);
        auto_timer = 0; es_state = ES_NONE;
      }
    }
    draw_log(R(c.x + 8, c.y + c.h - 50, c.w - 16, 42));
}

/* ------------------------------------------------------------------ battle */
static void finish_check(int st) {
    if (st == 0 || pend_end) return;
    pend_end = st; end_timer = 1500; es_state = ES_NONE; es_shots = 0;
    if (st == 1) { fx_banner("FLOOR CLEARED!"); audio_sfx(SFX_FANFARE); }
    else if (st == -1) { fx_banner("OFFICE REPOSSESSED!"); audio_sfx(SFX_GAMEOVER); }
    else { fx_banner("BURNOUT!"); audio_sfx(SFX_GAMEOVER); }
}

static int have_free_action(void) {
    int i;
    if (g_bt.ap > 0) return 1;
    if (bt_can_train(&g_bt) || g_run.synergy >= 100) return 1;
    for (i = 0; i < MAXINV; i++) if (g_run.inv[i] > 0 && g_item[g_run.inv[i] - 1].ap == 0) return 1;
    return 0;
}

static void start_enemy_turn(void) {
    bt_end_turn(&g_bt, &g_run);
    es_state = ES_WAIT; es_timer = 550; es_shots = g_bt.enemy_shots; armed = -1;
}

static void enemy_update(void) {
    if (es_state == ES_NONE || pend_end) return;
    es_timer -= g_dt;
    if (es_timer > 0) return;
    if (es_state == ES_WAIT) {
        if (es_shots > 0) { bt_ai_pick(&g_bt, &es_x, &es_y); es_state = ES_AIM; es_timer = 420; }
        else {
            bt_enemy_done(&g_bt, &g_run); es_state = ES_NONE; if (g_bt.phase == PH_PLAYER) game_save();
            finish_check(bt_status(&g_bt, &g_run));
            if (!pend_end && g_bt.phase == PH_PLAYER) fx_log(C_WHITE, "--- Turn %d: you have %d Action(s) ---", g_bt.turn, g_bt.ap);
        }
    } else if (es_state == ES_AIM) {
        bt_enemy_resolve(&g_bt, &g_run, es_x, es_y);
        es_shots--;
        { int st = bt_status(&g_bt, &g_run); if (st != 0) { finish_check(st); return; } }
        es_state = ES_WAIT; es_timer = 320;
    }
}

static void do_end_turn(void) {
    if (g_bt.phase != PH_PLAYER || pend_end || waiting_reply || waiting_train) return;
    start_enemy_turn();
}

static void do_training(void) {
    int m, d;
    if (!bt_can_train(&g_bt)) return;
    m = rng_n(&g_rng, N_MODULE);
    train_module = m; d = run_quiz_diff(&g_run, m);
    if (run_free_pass(&g_bt, &g_run)) {
        bt_train_result(&g_bt, &g_run, m, 1, d);
        fx_log(C_YELLOW, "Auto-correct! Your Calculator/Macro answered for you.");
        return;
    }
    quiz_open(QM_TRAIN, m); waiting_train = 1;
}

void battle_draw(void) {
    Rect c;
    Board *fb = &g_bt.foe, *yb = &g_bt.you;
    int cf = cs_foe(), cy = cs_you(), yx = you_x();
    int x, y, i, gx = -1, gy = -1, over_foe = 0, ox = shake_ox, oy = shake_oy;
    char buf[120], tipbuf[200];
    const char *tip = NULL;
    char title[100];

    snprintf(title, sizeof title, "Hostile Takeover - Floor %d: %s", floor_label(g_run.floor), g_bt.fd.dept);
    c = ui_app(title, 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    draw_status(c);

    /* resolve pending quiz results */
    if (waiting_reply && !quiz_active()) { waiting_reply = 0; bt_replyall_result(&g_bt, &g_run, quiz_last_ok()); finish_check(bt_status(&g_bt, &g_run)); }
    if (waiting_train && !quiz_active()) { waiting_train = 0; bt_train_result(&g_bt, &g_run, quiz_last_module(), quiz_last_ok(), quiz_last_diff()); finish_check(bt_status(&g_bt, &g_run)); }
    if (g_bt.pend_quiz && !waiting_reply && !quiz_active() && !pend_end) { waiting_reply = 1; quiz_open(QM_REPLY, rng_n(&g_rng, N_MODULE)); }

    /* board titles */
    snprintf(buf, sizeof buf, "THEIR FLOOR%s%s", g_bt.fd.boss ? " - BOSS: " : "", g_bt.fd.boss ? g_bt.boss_name : "");
    gfx_text_b(FOE_X, 74, buf, g_bt.fd.boss ? C_RED : C_MAROON);
    gfx_text_b(yx, 74, "YOUR OFFICE", C_NAVY);
    for (i = 0; i < fb->w; i++) { char t[4]; snprintf(t, sizeof t, "%c", 'A' + i); gfx_text(FOE_X + i * cf + cf / 2 - 2 + ox, FOE_Y - 10 + oy, t, C_BLACK); }
    for (i = 0; i < fb->h; i++) { char t[12]; snprintf(t, sizeof t, "%d", i + 1); gfx_text(FOE_X - 12 + (i + 1 < 10 ? 4 : 0) + ox, FOE_Y + i * cf + cf / 2 - 3 + oy, t, C_BLACK); }
    for (i = 0; i < yb->w; i++) { char t[4]; snprintf(t, sizeof t, "%c", 'A' + i); gfx_text(yx + i * cy + cy / 2 - 2, YOU_Y - 10, t, C_BLACK); }
    for (i = 0; i < yb->h; i++) { char t[12]; snprintf(t, sizeof t, "%d", i + 1); gfx_text(yx - 12 + (i + 1 < 10 ? 4 : 0), YOU_Y + i * cy + cy / 2 - 3, t, C_BLACK); }

    /* hover */
    if (g_mx >= FOE_X && g_my >= FOE_Y) { gx = (g_mx - FOE_X) / cf; gy = (g_my - FOE_Y) / cf; if (gx < fb->w && gy < fb->h) over_foe = 1; else { gx = gy = -1; } }

    /* boards */
    for (y = 0; y < fb->h; y++) for (x = 0; x < fb->w; x++) draw_foe_cell(x, y, FOE_X + x * cf + ox, FOE_Y + y * cf + oy, cf);
    for (y = 0; y < yb->h; y++) for (x = 0; x < yb->w; x++) draw_you_cell(x, y, yx + x * cy, YOU_Y + y * cy, cy);
    /* row/col hints */
    for (i = 0; i < fb->h; i++) if (fb->rowhint[i] >= 0) { char t[12]; snprintf(t, sizeof t, "%d", fb->rowhint[i]); gfx_text_b(FOE_X + fb->w * cf + 4 + ox, FOE_Y + i * cf + cf / 2 - 3 + oy, t, C_FUCHSIA); }
    for (i = 0; i < fb->w; i++) if (fb->colhint[i] >= 0) { char t[12]; snprintf(t, sizeof t, "%d", fb->colhint[i]); gfx_text_b(FOE_X + i * cf + cf / 2 - 2 + ox, FOE_Y + fb->h * cf + 3 + oy, t, C_FUCHSIA); }

    /* armed-item preview + crosshair */
    if (over_foe && g_bt.phase == PH_PLAYER && !pend_end) {
        int it = armed >= 0 ? g_run.inv[armed] - 1 : -1, k;
        int offs[9][2], n = 0, j;
        if (it == I_STAPLER) { int o[5][2] = {{0,0},{1,0},{-1,0},{0,1},{0,-1}}; for (k = 0; k < 5; k++) { offs[n][0] = o[k][0]; offs[n][1] = o[k][1]; n++; } }
        else if (it == I_FAX) { for (k = -1; k <= 1; k++) { offs[n][0] = k; offs[n][1] = 0; n++; } }
        else if (it == I_ALLHANDS) { for (j = -1; j <= 1; j++) for (k = -1; k <= 1; k++) { offs[n][0] = k; offs[n][1] = j; n++; } }
        else { offs[0][0] = 0; offs[0][1] = 0; n = 1; }
        for (k = 0; k < n; k++) {
            int cx = gx + offs[k][0], cyy = gy + offs[k][1];
            if (cx < 0 || cyy < 0 || cx >= fb->w || cyy >= fb->h) continue;
            gfx_frame(FOE_X + cx * cf + ox, FOE_Y + cyy * cf + oy, cf, cf, ((g_time / 160) & 1) ? C_YELLOW : C_WHITE);
            gfx_frame(FOE_X + cx * cf + 1 + ox, FOE_Y + cyy * cf + 1 + oy, cf - 2, cf - 2, C_BLACK);
        }
        snprintf(tipbuf, sizeof tipbuf, "Cell %c%d", 'A' + gx, gy + 1);
        tip = tipbuf;
        if (fb->c[gy][gx].known & K_HAZ) { tip = "Known hazard here."; }
    }
    /* enemy crosshair on your board */
    if (es_state == ES_AIM) {
        int px = yx + es_x * cy, py = YOU_Y + es_y * cy;
        gfx_frame(px, py, cy, cy, ((g_time / 90) & 1) ? C_RED : C_YELLOW); gfx_frame(px + 1, py + 1, cy - 2, cy - 2, C_RED);
        gfx_line(px - 6, py + cy / 2, px + cy + 6, py + cy / 2, C_RED); gfx_line(px + cy / 2, py - 6, px + cy / 2, py + cy + 6, C_RED);
    }
    /* booms and floats */
    for (i = 0; i < 24; i++) if (booms[i].on) {
        int px, py, r;
        cell_px(booms[i].side, booms[i].gx, booms[i].gy, &px, &py);
        if (booms[i].side == 1) { px += ox; py += oy; }
        r = booms[i].t / 26 + 2;
        if (booms[i].kind == SH_HIT || booms[i].kind == SH_SUNK) {
            gfx_circle(px, py, r, booms[i].t < 200 ? C_YELLOW : C_RED, 0);
            gfx_circle(px, py, MAX(1, r - 3), booms[i].t < 100 ? C_WHITE : C_RED, 0);
            if (booms[i].kind == SH_SUNK) { gfx_circle(px, py, r * 2, C_YELLOW, 0); gfx_circle(px, py, r * 2 + 3, C_RED, 0); }
        } else if (booms[i].kind == SH_MISS) gfx_circle(px, py, r, C_AQUA, 0);
    }
    for (i = 0; i < 24; i++) if (floats[i].on) {
        int px, py, w;
        cell_px(floats[i].side, floats[i].gx, floats[i].gy, &px, &py);
        w = gfx_text_bw(floats[i].txt);
        px = CLAMP(px - w / 2, 4, SCR_W - w - 4); py = (int)(py - 10 + floats[i].y);
        gfx_text_b(px + 1, py + 1, floats[i].txt, C_BLACK); gfx_text_b(px, py, floats[i].txt, floats[i].color);
    }

    /* middle column */
    { int mx0 = FOE_X + fb->w * cf + 22, mw = yx - mx0 - 14;
      int can = g_bt.phase == PH_PLAYER && !pend_end && !waiting_reply && !waiting_train && es_state == ES_NONE;
      int ap0 = (g_bt.ap == 0);
      Rect eb = R(mx0, 104, mw, 34);
      if (mw >= 70) {
        snprintf(buf, sizeof buf, "%s", ap0 && can && ((g_time / 400) & 1) ? "END TURN <<" : "END TURN");
        if (ui_button_def(eb, buf, can) || (can && ui_key(KEY_SPACE))) do_end_turn();
        if (ui_button(R(mx0, 146, mw, 28), "TRAINING (T)", can && bt_can_train(&g_bt)) || (can && ui_key('T'))) do_training();
        if (ui_button(R(mx0, 180, mw, 28), g_run.synergy >= 100 ? "SYNERGIZE! (S)" : "Synergize (S)", can && g_run.synergy >= 100) || (can && ui_key('S') && g_run.synergy >= 100)) {
            if (bt_synergize(&g_bt, &g_run)) { audio_sfx(SFX_POWERUP); finish_check(bt_status(&g_bt, &g_run)); }
        }
        gfx_text_b(mx0, 220, "INTEL", C_NAVY);
        snprintf(buf, sizeof buf, "Dmg/hit: %d", g_bt.fd.dmg); gfx_text(mx0, 232, buf, C_BLACK);
        snprintf(buf, sizeof buf, "Enemy Assets left: %d", board_alive(fb)); ui_wrap(R(mx0, 244, mw, 20), buf, C_BLACK, 1);
        snprintf(buf, sizeof buf, "Your Assets: %d", board_alive(yb)); gfx_text(mx0, 268, buf, C_BLACK);
        { char rt[60]; rules_text(&g_bt.fd, rt, sizeof rt); ui_wrap(R(mx0, 282, mw, 30), rt, C_PURPLE, 1); }
        if (g_bt.fd.rules & RULE_DOUBLE) { snprintf(buf, sizeof buf, "Overtime in %d", g_bt.fd.double_n - (g_bt.enemy_turns % g_bt.fd.double_n)); gfx_text(mx0, 304, buf, C_RED); }
        if ((g_bt.fd.rules & RULE_REORG) && g_bt.fd.reorg_n) { snprintf(buf, sizeof buf, "Re-org in %d", g_bt.fd.reorg_n - (g_bt.enemy_turns % g_bt.fd.reorg_n)); gfx_text(mx0, 316, buf, C_FUCHSIA); }
      }
      /* input on foe board */
      if (can && over_foe && !ui_dialog_active()) {
          if (g_press) {
              int res;
              if (armed >= 0) {
                  if (bt_use_item(&g_bt, &g_run, armed, gx, gy)) { audio_sfx(SFX_FIRE); finish_check(bt_status(&g_bt, &g_run)); } else audio_sfx(SFX_ERROR);
                  armed = -1;
              } else {
                  res = bt_fire(&g_bt, &g_run, gx, gy);
                  if (res == -1) audio_sfx(SFX_ERROR); else if (res == SH_ALREADY) audio_sfx(SFX_ERROR); else audio_sfx(SFX_FIRE);
                  finish_check(bt_status(&g_bt, &g_run));
              }
          }
          if (g_rpress && fb->c[gy][gx].state == CS_UNKNOWN) { fb->c[gy][gx].known ^= K_FLAG; audio_sfx(SFX_CLICK); }
      }
      if (can && ui_key(KEY_ESC)) armed = -1;
      /* items */
      gfx_text_b(12, 350, "SUPPLIES", C_BLACK);
      for (i = 0; i < MAXINV; i++) {
          Rect sr = R(74 + i * 38, 342, 34, 34);
          int it = g_run.inv[i] - 1, cap = MIN(MAXINV, g_run.maxinv + (HAS(&g_run, P_DUALMON) ? 2 : 0)), hov = ui_hover(sr);
          if (i >= cap) { gfx_rect(sr.x, sr.y, sr.w, sr.h, C_GRAY); gfx_dither(sr.x, sr.y, sr.w, sr.h, C_SILVER, C_GRAY, 0); gfx_frame(sr.x, sr.y, sr.w, sr.h, C_BLACK); continue; }
          gfx_rect(sr.x, sr.y, sr.w, sr.h, C_WHITE); ui_sunken(sr);
          if (it >= 0) {
              gfx_spr_scaled(sr.x + 1, sr.y + 1, &S_ITEM[it], 2);
              { char k[12]; snprintf(k, sizeof k, "%d", i + 1); gfx_text(sr.x + 3, sr.y + 3, k, C_GRAY); }
              if (armed == i) { gfx_frame(sr.x, sr.y, sr.w, sr.h, C_RED); gfx_frame(sr.x + 1, sr.y + 1, sr.w - 2, sr.h - 2, C_RED); }
              if (hov) { snprintf(tipbuf, sizeof tipbuf, "%s: %s (%d Action%s)", g_item[it].name, g_item[it].desc, g_item[it].ap, g_item[it].ap == 1 ? "" : "s"); tip = tipbuf; }
              if (can && ((hov && g_press) || ui_key('1' + i))) {
                  if (g_item[it].target) { armed = (armed == i) ? -1 : i; audio_sfx(SFX_CLICK); }
                  else { if (bt_use_item(&g_bt, &g_run, i, 0, 0)) { audio_sfx(SFX_POWERUP); finish_check(bt_status(&g_bt, &g_run)); } else audio_sfx(SFX_ERROR); }
              }
          } else if (hov) tip = "Empty supply slot. Buy supplies in the Break Room.";
      }
      /* perks */
      { int n = 0;
        gfx_text_b(392, 350, "PERKS", C_BLACK);
        for (i = 0; i < N_PERK; i++) if (HAS(&g_run, i)) {
            Rect pr = R(430 + (n % 10) * 20, 344 + (n / 10) * 20, 18, 18);
            char l[2] = { g_perk[i].name[0], 0 };
            gfx_rect(pr.x, pr.y, pr.w, pr.h, C_YELLOW); gfx_bevel(pr.x, pr.y, pr.w, pr.h, C_WHITE, C_OLIVE);
            gfx_text_b(pr.x + 6, pr.y + 6, l, C_BLACK);
            if (ui_hover(pr)) { snprintf(tipbuf, sizeof tipbuf, "%s: %s", g_perk[i].name, g_perk[i].desc); tip = tipbuf; }
            n++;
        }
        if (n == 0) gfx_text(430, 352, "(none yet)", C_GRAY);
      }
      /* own assets perk tooltips */
      if (g_mx >= yx && g_my >= YOU_Y) {
          int ax = (g_mx - yx) / cy, ay = (g_my - YOU_Y) / cy;
          if (ax < yb->w && ay < yb->h && yb->c[ay][ax].asset >= 0) {
              const Asset *a = &yb->a[yb->c[ay][ax].asset];
              snprintf(tipbuf, sizeof tipbuf, "%s - %s: %s%s", g_adef[a->def].name, g_adef[a->def].perk, g_adef[a->def].perkdesc, a->sunk ? " (REPOSSESSED)" : "");
              tip = tipbuf;
          }
      }
    }
    /* info line */
    gfx_rect(c.x + 4, 382, c.w - 8, 12, C_SILVER);
    if (!tip && g_bt.phase == PH_PLAYER && g_bt.ap == 0 && !pend_end) tip = "Out of Actions! Press END TURN (Space) - or use Training / free supplies.";
    if (!tip && g_bt.phase == PH_PLAYER && g_bt.ap > 0 && !pend_end && es_state == ES_NONE)
        tip = armed >= 0 ? "Supply armed: click a square on THEIR FLOOR to use it (Esc cancels)." : "YOUR TURN: click a square on THEIR FLOOR to fire. Goal: hit every square of every enemy Asset.";
    if (!tip && es_state != ES_NONE) tip = "The enemy is taking its turn...";
    if (tip) gfx_text(c.x + 8, 384, tip, C_BLACK);
    draw_log(R(c.x + 4, 398, c.w - 8, 76));

    /* banner */
    if (banner_t > 0) {
        int w = gfx_text_big_w(banner_txt, 3), x0 = (SCR_W - w) / 2, y0 = 190;
        int slide = banner_t > 1400 ? (banner_t - 1400) * 2 : (banner_t < 300 ? (300 - banner_t) * 2 : 0);
        gfx_rect(0, y0 - 8, SCR_W, 46, C_NAVY); gfx_hline(0, y0 - 8, SCR_W, C_YELLOW); gfx_hline(0, y0 + 37, SCR_W, C_YELLOW);
        gfx_text_big(x0 + 3 + slide, y0 + 3, banner_txt, C_BLACK, 3); gfx_text_big(x0 + slide, y0, banner_txt, C_YELLOW, 3);
    }

    /* auto end of turn when nothing is left to do */
    if (g_bt.phase == PH_PLAYER && !pend_end && es_state == ES_NONE && !waiting_reply && !waiting_train && !quiz_active()) {
        if (!have_free_action()) { auto_timer += g_dt; if (auto_timer > 900) { auto_timer = 0; do_end_turn(); } }
        else auto_timer = 0;
    } else auto_timer = 0;
    if (g_bt.phase == PH_ENEMY && es_state == ES_NONE && !pend_end) { es_state = ES_WAIT; es_timer = 300; es_shots = g_bt.enemy_shots; }
    enemy_update();

    /* outcomes */
    if (pend_end) {
        end_timer -= g_dt;
        if (end_timer <= 0) {
            int st = pend_end; pend_end = 0;
            if (st == 1) after_battle_win();
            else if (st == -1) { ui_dialog(77, "Office Repossessed", "Your entire office was repossessed!\nYou lose 40 Morale and must retake this floor.", 3, "Retake Floor", NULL, NULL); pend_end = -1; end_timer = 1 << 30; }
            else go_dead();
        }
        if (pend_end == -1 && end_timer > 1000000 && ui_dialog_result(77) >= 0) { pend_end = 0; scr_go(SC_PLACE); }
    }
    g_battle_busy = (es_state != ES_NONE) || pend_end;
    (void)hint_scr_shown;
}

/* test hook: pixel centre of a cell on Their Floor */
int ui_dbg_foe_px(int gx, int gy, int *px, int *py) {
    if (gx < 0 || gy < 0 || gx >= g_bt.foe.w || gy >= g_bt.foe.h) return 0;
    cell_px(1, gx, gy, px, py);
    return 1;
}

/* ------------------------------------------------------------------ hooks for the tutorial (ui_tutorial.c) */
int battle_idle(void) { return g_bt.phase == PH_PLAYER && es_state == ES_NONE && !pend_end && !waiting_reply && !waiting_train && !quiz_active() && banner_t <= 0; }
int battle_ui_rect(int which, Rect *r) {
    int cf = cs_foe(), cy = cs_you(), yx = you_x();
    int mx0 = FOE_X + g_bt.foe.w * cf + 22, mw = yx - mx0 - 14;
    switch (which) {
    case 0: *r = R(mx0, 104, mw, 34); return 1;        /* END TURN */
    case 1: *r = R(mx0, 146, mw, 28); return 1;        /* TRAINING */
    case 2: *r = R(mx0, 180, mw, 28); return 1;        /* SYNERGIZE */
    case 3: *r = R(FOE_X, FOE_Y, g_bt.foe.w * cf, g_bt.foe.h * cf); return 1;
    case 4: *r = R(yx, YOU_Y, g_bt.you.w * cy, g_bt.you.h * cy); return 1;
    case 5: *r = R(6, 44, 628, 24); return 1;          /* status strip */
    case 6: *r = R(mx0 - 2, 216, mw + 6, 108); return 1; /* intel */
    case 7: *r = R(8, 340, 300, 38); return 1;         /* supplies */
    case 8: *r = R(4, 380, 632, 14); return 1;         /* info line */
    default: if (which >= 20 && which < 20 + MAXINV) { *r = R(74 + (which - 20) * 38, 342, 34, 34); return 1; } break;
    }
    return 0;
}
int place_ui_rect(int which, Rect *r) {
    Board *bd = &g_bt.you;
    int cs = bd->w <= 8 ? 30 : bd->w <= 9 ? 28 : 26, px = 24 + bd->w * cs + 24, py = 42 + 34 + 156, pw = 636 - px - 8;
    switch (which) {
    case 0: *r = R(px, py + 138, 110, 24); return 1;   /* shuffle */
    case 1: *r = R(px + 120, py + 138, 140, 24); return 1; /* ready */
    case 2: *r = R(24, 84, bd->w * cs, bd->h * cs); return 1;
    case 3: *r = R(px, py, pw, 128); return 1;
    }
    return 0;
}
