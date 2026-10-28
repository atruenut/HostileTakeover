/* Break Room Arcade: four tiny office-themed mini games with persistent best scores. */
#include "ui.h"

enum { G_SWEEP, G_CABLE, G_FILE, G_REPLY, N_GAMES };
typedef struct { const char *name, *blurb, *keys; int lower_better; } GameDef;
static const GameDef gdef[N_GAMES] = {
    {"PaperSweeper", "Clear the cubicle farm without touching a paper jam.", "Left-click: dig.  Right-click: flag.  Double-click a number: clear around it.", 1},
    {"Cable Management", "Untangle the server room. Plug in every connector.", "Arrow keys or WASD steer. Don't hit walls, coffee spills or yourself.", 0},
    {"Filing Cabinet", "Match every pair of office supplies in as few tries as you can.", "Click two drawers to flip them. Fewest moves wins.", 1},
    {"Reply-All Panic", "Delete the URGENT emails before the inbox overflows!", "Click red URGENT mails. Do NOT click the CEO's mails. 3 overflows and you're out.", 0},
};
static int cur_game = G_SWEEP, g_over, g_score, g_started;
static char g_msg[100];

/* ================================================================== PaperSweeper */
#define SW 9
#define SH 9
#define SMINES 10
static struct { unsigned char mine[SH][SW], open[SH][SW], flag[SH][SW]; int placed, dead, won, t; int boom_x, boom_y; } sw;

static int sw_cnt(int x, int y) { int dx, dy, n = 0; for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) { int nx = x + dx, ny = y + dy; if (nx >= 0 && ny >= 0 && nx < SW && ny < SH && sw.mine[ny][nx]) n++; } return n; }
static void sw_init(void) { memset(&sw, 0, sizeof sw); sw.boom_x = -1; }
static void sw_flood(int x, int y) {
    int dx, dy;
    if (x < 0 || y < 0 || x >= SW || y >= SH || sw.open[y][x] || sw.flag[y][x]) return;
    sw.open[y][x] = 1;
    if (sw.mine[y][x]) { sw.dead = 1; sw.boom_x = x; sw.boom_y = y; return; }
    if (sw_cnt(x, y) == 0) for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) sw_flood(x + dx, y + dy);
}
static void sw_place(int sx, int sy) {
    int n = 0, guard = 0;
    while (n < SMINES && guard++ < 5000) {
        int x = rng_n(&g_rng, SW), y = rng_n(&g_rng, SH);
        if (sw.mine[y][x] || (abs(x - sx) <= 1 && abs(y - sy) <= 1)) continue;
        sw.mine[y][x] = 1; n++;
    }
    sw.placed = 1;
}
static int sw_frame(Rect c) {
    static const int ncol[9] = {C_BLACK, C_BLUE, C_GREEN, C_RED, C_NAVY, C_MAROON, C_TEAL, C_BLACK, C_GRAY};
    int cs = 34, bx = c.x + (c.w - SW * cs) / 2, by = c.y + 70, x, y, flags = 0, closed = 0;
    char b[40];
    if (!sw.dead && !sw.won && sw.placed) sw.t += g_dt;
    for (y = 0; y < SH; y++) for (x = 0; x < SW; x++) {
        int px = bx + x * cs, py = by + y * cs, hot = ui_hover(R(px, py, cs, cs));
        if (sw.flag[y][x]) flags++;
        if (!sw.open[y][x]) closed++;
        if (sw.open[y][x]) {
            gfx_rect(px, py, cs, cs, (sw.dead && x == sw.boom_x && y == sw.boom_y) ? C_RED : C_SILVER); gfx_frame(px, py, cs, cs, C_GRAY);
            if (sw.mine[y][x]) { gfx_circle(px + cs / 2, py + cs / 2, 9, C_WHITE, 1); gfx_circle(px + cs / 2, py + cs / 2, 9, C_BLACK, 0); gfx_line(px + 9, py + 12, px + cs - 9, py + cs - 12, C_BLACK); gfx_line(px + 12, py + 24, px + cs - 12, py + 9, C_BLACK); }
            else { int n = sw_cnt(x, y); if (n) { snprintf(b, sizeof b, "%d", n); gfx_text_big(px + cs / 2 - 4, py + cs / 2 - 7, b, ncol[n], 2); } }
        } else {
            gfx_rect(px, py, cs, cs, C_SILVER); gfx_bevel(px, py, cs, cs, C_WHITE, C_GRAY); gfx_bevel(px + 1, py + 1, cs - 2, cs - 2, C_WHITE, C_GRAY); gfx_frame(px, py, cs, cs, C_BLACK);
            if (sw.flag[y][x]) gfx_spr_scaled(px + 5, py + 5, &S_FLAG, 1 + (cs > 28));
            if (sw.dead && sw.mine[y][x] && !sw.flag[y][x]) { gfx_circle(px + cs / 2, py + cs / 2, 7, C_WHITE, 1); gfx_circle(px + cs / 2, py + cs / 2, 7, C_BLACK, 0); }
            if (hot && !sw.dead && !sw.won) gfx_dither(px + 2, py + 2, cs - 4, cs - 4, C_WHITE, C_SILVER, 1);
        }
        if (hot && !sw.dead && !sw.won) {
            if (g_press && !sw.flag[y][x] && !sw.open[y][x]) { if (!sw.placed) sw_place(x, y); sw_flood(x, y); audio_sfx(sw.dead ? SFX_SUNK : SFX_CLICK); }
            else if (g_dbl && sw.open[y][x] && sw_cnt(x, y) > 0) {
                int dx, dy, fl = 0;
                for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) { int nx = x + dx, ny = y + dy; if (nx >= 0 && ny >= 0 && nx < SW && ny < SH && sw.flag[ny][nx]) fl++; }
                if (fl == sw_cnt(x, y)) for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) sw_flood(x + dx, y + dy);
            }
            else if (g_rpress && !sw.open[y][x]) { sw.flag[y][x] ^= 1; audio_sfx(SFX_CLICK); }
        }
    }
    snprintf(b, sizeof b, "Jams left: %d", SMINES - flags); gfx_text_b(bx, c.y + 40, b, C_MAROON);
    snprintf(b, sizeof b, "Time: %d", sw.t / 1000); gfx_text_b(bx + SW * cs - gfx_text_bw(b), c.y + 40, b, C_NAVY);
    if (!sw.dead && sw.placed && closed == SMINES) sw.won = 1;
    if (sw.dead) { snprintf(g_msg, sizeof g_msg, "CRUNCH! You found a paper jam. The printer weeps."); g_score = 0; return 1; }
    if (sw.won) { snprintf(g_msg, sizeof g_msg, "Cubicle farm cleared in %d seconds!", sw.t / 1000); g_score = MAX(1, sw.t / 1000); return 2; }
    return 0;
}

/* ================================================================== Cable Management (snake) */
#define CW 30
#define CH 19
static struct { int x[CW * CH], y[CW * CH], len, dx, dy, qdx[2], qdy[2], nq, fx, fy, t, speed, eaten; unsigned char spill[CH][CW]; } cb;
static void cb_food(void) {
    int guard = 0, i;
    do {
        cb.fx = rng_n(&g_rng, CW); cb.fy = rng_n(&g_rng, CH);
        for (i = 0; i < cb.len; i++) if (cb.x[i] == cb.fx && cb.y[i] == cb.fy) break;
    } while ((i < cb.len || cb.spill[cb.fy][cb.fx]) && guard++ < 1000);
}
static void cb_init(void) {
    memset(&cb, 0, sizeof cb);
    cb.len = 3; cb.x[0] = 12; cb.y[0] = 9; cb.x[1] = 11; cb.y[1] = 9; cb.x[2] = 10; cb.y[2] = 9; cb.dx = 1; cb.speed = 150;
    cb_food();
}
static int cb_frame(Rect c) {
    int cs = 18, bx = c.x + (c.w - CW * cs) / 2, by = c.y + 40, i, x, y, dead = 0, want_dx = 0, want_dy = 0, got = 0;
    char b[60];
    if (ui_key(KEY_LEFT) || ui_key('A')) { want_dx = -1; got = 1; }
    else if (ui_key(KEY_RIGHT) || ui_key('D')) { want_dx = 1; got = 1; }
    else if (ui_key(KEY_UP) || ui_key('W')) { want_dy = -1; got = 1; }
    else if (ui_key(KEY_DOWN) || ui_key('S')) { want_dy = 1; got = 1; }
    if (got && cb.nq < 2) {
        int ldx = cb.nq ? cb.qdx[cb.nq - 1] : cb.dx, ldy = cb.nq ? cb.qdy[cb.nq - 1] : cb.dy;
        if (!(want_dx == -ldx && want_dy == -ldy) && !(want_dx == ldx && want_dy == ldy)) { cb.qdx[cb.nq] = want_dx; cb.qdy[cb.nq] = want_dy; cb.nq++; }
    }
    cb.t += g_dt;
    while (cb.t >= cb.speed && !dead) {
        int nx, ny;
        cb.t -= cb.speed;
        if (cb.nq) { cb.dx = cb.qdx[0]; cb.dy = cb.qdy[0]; cb.qdx[0] = cb.qdx[1]; cb.qdy[0] = cb.qdy[1]; cb.nq--; }
        nx = cb.x[0] + cb.dx; ny = cb.y[0] + cb.dy;
        if (nx < 0 || ny < 0 || nx >= CW || ny >= CH || cb.spill[ny][nx]) { dead = 1; break; }
        for (i = 0; i < cb.len - 1; i++) if (cb.x[i] == nx && cb.y[i] == ny) { dead = 1; break; }
        if (dead) break;
        { int eat = (nx == cb.fx && ny == cb.fy);
          if (eat) cb.len++;
          for (i = cb.len - 1; i > 0; i--) { cb.x[i] = cb.x[i - 1]; cb.y[i] = cb.y[i - 1]; }
          cb.x[0] = nx; cb.y[0] = ny;
          if (eat) {
              cb.eaten++; audio_sfx(SFX_COIN);
              if (cb.speed > 70) cb.speed -= 4;
              if (cb.eaten % 4 == 0) { int tries = 0; while (tries++ < 200) { int sx = rng_n(&g_rng, CW), sy = rng_n(&g_rng, CH), k, ok = 1; for (k = 0; k < cb.len; k++) if (abs(cb.x[k] - sx) + abs(cb.y[k] - sy) < 4) ok = 0; if (ok && !(sx == cb.fx && sy == cb.fy)) { cb.spill[sy][sx] = 1; break; } } }
              cb_food();
          } }
    }
    gfx_rect(bx, by, CW * cs, CH * cs, C_BLACK); gfx_frame(bx - 1, by - 1, CW * cs + 2, CH * cs + 2, C_GRAY);
    for (y = 0; y < CH; y++) for (x = 0; x < CW; x++) {
        if ((x + y) & 1) gfx_dither(bx + x * cs, by + y * cs, cs, cs, C_NAVY, C_BLACK, 1);
        if (cb.spill[y][x]) { gfx_rect(bx + x * cs + 2, by + y * cs + 3, cs - 4, cs - 6, C_MAROON); gfx_rect(bx + x * cs + 4, by + y * cs + 5, cs - 8, 3, C_OLIVE); }
    }
    gfx_rect(bx + cb.fx * cs + 3, by + cb.fy * cs + 3, cs - 6, cs - 6, C_YELLOW); gfx_frame(bx + cb.fx * cs + 3, by + cb.fy * cs + 3, cs - 6, cs - 6, C_WHITE);
    gfx_rect(bx + cb.fx * cs + 6, by + cb.fy * cs, 2, 4, C_SILVER); gfx_rect(bx + cb.fx * cs + 10, by + cb.fy * cs, 2, 4, C_SILVER);
    for (i = cb.len - 1; i >= 0; i--) {
        int px = bx + cb.x[i] * cs, py = by + cb.y[i] * cs;
        if (i == 0) { gfx_rect(px + 1, py + 1, cs - 2, cs - 2, C_LIME); gfx_frame(px + 1, py + 1, cs - 2, cs - 2, C_BLACK); gfx_rect(px + 5, py + 5, 3, 3, C_BLACK); gfx_rect(px + cs - 8, py + 5, 3, 3, C_BLACK); }
        else { gfx_rect(px + 2, py + 2, cs - 4, cs - 4, (i & 1) ? C_AQUA : C_TEAL); gfx_frame(px + 2, py + 2, cs - 4, cs - 4, C_BLACK); }
    }
    snprintf(b, sizeof b, "Connectors plugged in: %d", cb.eaten); gfx_text_b(bx, c.y + 24, b, C_NAVY);
    gfx_text(bx + CW * cs - 150, c.y + 24, "Yellow = connector. Brown = spill.", C_GRAY);
    if (dead) { g_score = cb.eaten * 10; snprintf(g_msg, sizeof g_msg, "Cable tangle! You plugged in %d connectors.", cb.eaten); return 1; }
    return 0;
}

/* ================================================================== Filing Cabinet (memory) */
static struct { int card[16], up[16], done[16], first, second, wait, moves, found; } fc;
static const int fc_icon[8] = {I_STAPLER, I_HIGHLIGHT, I_STICKY, I_FAX, I_SCANNER, I_ENERGY, I_JAM, I_MUG};
static void fc_init(void) {
    int i;
    memset(&fc, 0, sizeof fc);
    fc.first = fc.second = -1;
    for (i = 0; i < 16; i++) fc.card[i] = i / 2;
    for (i = 15; i > 0; i--) { int j = rng_n(&g_rng, i + 1), t = fc.card[i]; fc.card[i] = fc.card[j]; fc.card[j] = t; }
}
static int fc_frame(Rect c) {
    int cs = 76, gap = 8, bx = c.x + (c.w - (4 * cs + 3 * gap)) / 2, by = c.y + 44, i;
    char b[60];
    if (fc.wait > 0) {
        fc.wait -= g_dt;
        if (fc.wait <= 0) { fc.up[fc.first] = fc.up[fc.second] = 0; fc.first = fc.second = -1; }
    }
    for (i = 0; i < 16; i++) {
        int px = bx + (i % 4) * (cs + gap), py = by + (i / 4) * (cs + gap);
        Rect r = R(px, py, cs, cs);
        if (fc.up[i] || fc.done[i]) {
            gfx_rect(px, py, cs, cs, fc.done[i] ? C_LIME : C_WHITE); gfx_frame(px, py, cs, cs, C_BLACK);
            if (fc.done[i]) gfx_dither(px, py, cs, cs, C_LIME, C_WHITE, 1);
            gfx_spr_scaled(px + (cs - S_ITEM[fc_icon[fc.card[i]]].w * 3) / 2, py + (cs - S_ITEM[fc_icon[fc.card[i]]].h * 3) / 2, &S_ITEM[fc_icon[fc.card[i]]], 3);
        } else {
            gfx_rect(px, py, cs, cs, C_GRAY); gfx_bevel(px, py, cs, cs, C_SILVER, C_BLACK); gfx_frame(px, py, cs, cs, C_BLACK);
            gfx_rect(px + 12, py + 14, cs - 24, 16, C_WHITE); gfx_frame(px + 12, py + 14, cs - 24, 16, C_BLACK);
            gfx_text(px + 17, py + 19, "FILES", C_BLACK);
            gfx_rect(px + cs / 2 - 14, py + 46, 28, 8, C_SILVER); gfx_frame(px + cs / 2 - 14, py + 46, 28, 8, C_BLACK);
            if (ui_hover(r) && fc.wait <= 0) gfx_frame(px + 2, py + 2, cs - 4, cs - 4, C_YELLOW);
            if (ui_hover(r) && g_press && fc.wait <= 0) {
                fc.up[i] = 1; audio_sfx(SFX_CLICK);
                if (fc.first < 0) fc.first = i;
                else {
                    fc.second = i; fc.moves++;
                    if (fc.card[fc.first] == fc.card[i]) { fc.done[fc.first] = fc.done[i] = 1; fc.found++; fc.first = fc.second = -1; audio_sfx(SFX_CORRECT); }
                    else { fc.wait = 750; audio_sfx(SFX_WRONG); }
                }
            }
        }
    }
    snprintf(b, sizeof b, "Moves: %d     Pairs: %d/8", fc.moves, fc.found); gfx_text_b(bx, c.y + 26, b, C_NAVY);
    if (fc.found >= 8) { g_score = fc.moves; snprintf(g_msg, sizeof g_msg, "All filed in %d moves. Perfect is 8!", fc.moves); return 2; }
    return 0;
}

/* ================================================================== Reply-All Panic (whack-a-mole) */
typedef struct { int cell, type, life, maxlife, hit; } Mail;
static struct { Mail m[9]; int score, lives, t, spawn, combo, best_combo, flash; } rp;
static void rp_init(void) { memset(&rp, 0, sizeof rp); rp.lives = 3; rp.t = 45000; rp.spawn = 600; }
static int rp_frame(Rect c) {
    int cs = 118, gap = 10, bx = c.x + (c.w - (3 * cs + 2 * gap)) / 2, by = c.y + 48, i;
    char b[80];
    rp.t -= g_dt; rp.spawn -= g_dt;
    if (rp.spawn <= 0) {
        int cell = rng_n(&g_rng, 9), busy = 0, k;
        for (k = 0; k < 9; k++) if (rp.m[k].life > 0 && rp.m[k].cell == cell) busy = 1;
        if (!busy) for (k = 0; k < 9; k++) if (rp.m[k].life <= 0) {
            int el = 45000 - rp.t, life = MAX(550, 1300 - el / 45);
            rp.m[k].cell = cell; rp.m[k].type = rng_pct(&g_rng, 26) ? 1 : 0; rp.m[k].life = rp.m[k].maxlife = life; rp.m[k].hit = 0; break;
        }
        rp.spawn = MAX(260, 700 - (45000 - rp.t) / 80);
    }
    for (i = 0; i < 9; i++) {
        int px = bx + (i % 3) * (cs + gap), py = by + (i / 3) * (cs - 14 + gap);
        gfx_rect(px, py, cs, cs - 14, C_NAVY); gfx_bevel(px, py, cs, cs - 14, C_BLACK, C_BLUE); gfx_frame(px, py, cs, cs - 14, C_BLACK);
        gfx_text(px + 6, py + 4, "INBOX", C_AQUA);
    }
    for (i = 0; i < 9; i++) {
        Mail *m = &rp.m[i];
        int px, py;
        if (m->life <= 0) continue;
        px = bx + (m->cell % 3) * (cs + gap); py = by + (m->cell / 3) * (cs - 14 + gap);
        m->life -= g_dt;
        gfx_rect(px + 14, py + 20, cs - 28, cs - 48, m->type ? C_NAVY : C_WHITE); gfx_frame(px + 14, py + 20, cs - 28, cs - 48, C_BLACK);
        gfx_line(px + 14, py + 20, px + cs / 2, py + 20 + (cs - 48) / 2, C_BLACK); gfx_line(px + cs - 14, py + 20, px + cs / 2, py + 20 + (cs - 48) / 2, C_BLACK);
        gfx_text_b(px + 24, py + 30 + (cs - 48) / 2 - 2, m->type ? "FROM: CEO" : "URGENT!!", m->type ? C_YELLOW : C_RED);
        gfx_rect(px + 14, py + 14, (cs - 28) * m->life / m->maxlife, 3, m->type ? C_YELLOW : C_RED);
        if (ui_hover(R(px, py, cs, cs - 14)) && g_press) {
            if (m->type == 0) { rp.combo++; rp.best_combo = MAX(rp.best_combo, rp.combo); rp.score += 10 + MIN(rp.combo, 10); audio_sfx(SFX_CORRECT); }
            else { rp.score = MAX(0, rp.score - 25); rp.combo = 0; audio_sfx(SFX_ERROR); rp.flash = 300; }
            m->life = 0; m->hit = 1;
        }
        if (m->life <= 0 && !m->hit && m->type == 0) { rp.lives--; rp.combo = 0; audio_sfx(SFX_WRONG); rp.flash = 300; }
    }
    if (rp.flash > 0) { rp.flash -= g_dt; gfx_frame(c.x, c.y, c.w, c.h, C_RED); }
    snprintf(b, sizeof b, "Score: %d    Combo: x%d", rp.score, rp.combo); gfx_text_b(bx, c.y + 22, b, C_NAVY);
    snprintf(b, sizeof b, "Inbox overflows left: %d    Time: %d", rp.lives, MAX(0, rp.t / 1000)); gfx_text_b(bx + 190, c.y + 22, b, rp.lives <= 1 ? C_RED : C_BLACK);
    if (rp.lives <= 0 || rp.t <= 0) { g_score = rp.score; snprintf(g_msg, sizeof g_msg, rp.lives <= 0 ? "Inbox overflow! Score %d (best combo x%d)." : "Shift over! Score %d (best combo x%d).", rp.score, rp.best_combo); return 1; }
    return 0;
}

/* ================================================================== hub and game shell */
void arcade_open(void) { scr_go(SC_ARCADE); }

static void game_begin(int id) {
    cur_game = id; g_over = 0; g_score = 0; g_msg[0] = 0; g_started = 1;
    switch (id) { case G_SWEEP: sw_init(); break; case G_CABLE: cb_init(); break; case G_FILE: fc_init(); break; default: rp_init(); break; }
    g_scr = SC_GAME;
}

static void icon_for(int id, int x, int y) {
    switch (id) {
    case G_SWEEP: gfx_spr_scaled(x, y, &S_ITEM[I_JAM], 3); break;
    case G_CABLE: gfx_spr_scaled(x, y, &S_ASSET[A_RACK], 3); break;
    case G_FILE: gfx_spr_scaled(x, y, &S_ASSET[A_DESK], 3); break;
    default: gfx_spr_scaled(x, y, &S_ITEM[I_ALLHANDS], 3); break;
    }
}

void arcade_draw(void) {
    Rect c;
    int i;
    char b[100];
    c = ui_app("Break Room Arcade - Synergex Recreation Suite", 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    gfx_text_b(c.x + 12, c.y + 8, "On your break? Four tiny games, zero productivity.", C_NAVY);
    gfx_text(c.x + 12, c.y + 22, "Best scores are saved automatically.", C_GRAY);
    for (i = 0; i < N_GAMES; i++) {
        Rect r = R(c.x + 12 + (i % 2) * 308, c.y + 42 + (i / 2) * 170, 300, 160);
        int best = g_cfg.hs[i];
        gfx_rect(r.x, r.y, r.w, r.h, C_WHITE); ui_sunken(r);
        gfx_rect(r.x + 8, r.y + 8, 56, 56, C_TEAL); gfx_frame(r.x + 8, r.y + 8, 56, 56, C_BLACK);
        icon_for(i, r.x + 8 + 4, r.y + 8 + 4);
        gfx_text_b(r.x + 74, r.y + 10, gdef[i].name, C_NAVY);
        ui_wrap(R(r.x + 74, r.y + 26, r.w - 82, 40), gdef[i].blurb, C_BLACK, 1);
        ui_wrap(R(r.x + 10, r.y + 74, r.w - 20, 30), gdef[i].keys, C_GRAY, 1);
        if (best) snprintf(b, sizeof b, i == G_SWEEP ? "Best: %d seconds" : i == G_FILE ? "Best: %d moves" : i == G_CABLE ? "Best: %d points" : "Best: %d points", best);
        else snprintf(b, sizeof b, "Best: --");
        gfx_text_b(r.x + 10, r.y + 112, b, C_MAROON);
        if (ui_button_def(R(r.x + r.w - 110, r.y + r.h - 34, 100, 26), "Play!", 1) || (ui_hover(R(r.x, r.y, r.w, r.h - 40)) && g_dbl)) game_begin(i);
    }
    if (ui_button_def(R(c.x + c.w - 130, c.y + c.h - 36, 120, 28), "Close", 1) || ui_key(KEY_ESC)) scr_go(SC_DESKTOP);
}

void game_draw(void) {
    Rect c, cl;
    int r = 0;
    char b[100];
    c = ui_app(gdef[cur_game].name, 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    cl = R(c.x, c.y + 4, c.w, c.h - 40);
    if (!g_over) {
        switch (cur_game) { case G_SWEEP: r = sw_frame(cl); break; case G_CABLE: r = cb_frame(cl); break; case G_FILE: r = fc_frame(cl); break; default: r = rp_frame(cl); break; }
        if (r) {
            int better = 0, won = (r == 2) || (!gdef[cur_game].lower_better && g_score > 0);
            g_over = 1;
            if (gdef[cur_game].lower_better) { if (r == 2 && (g_cfg.hs[cur_game] == 0 || g_score < g_cfg.hs[cur_game])) { g_cfg.hs[cur_game] = g_score; better = 1; } }
            else if (g_score > g_cfg.hs[cur_game]) { g_cfg.hs[cur_game] = g_score; better = 1; }
            if (better) { cfg_save(); snprintf(g_msg + strlen(g_msg), sizeof g_msg - strlen(g_msg), "  NEW BEST!"); }
            audio_sfx(won ? SFX_FANFARE : SFX_GAMEOVER);
        }
        if (ui_key(KEY_ESC)) g_scr = SC_ARCADE;
    } else {
        /* frozen playfield is gone: show result card */
        Rect card = R(c.x + (c.w - 420) / 2, c.y + 90, 420, 170);
        gfx_rect(card.x + 5, card.y + 5, card.w, card.h, C_BLACK);
        gfx_rect(card.x, card.y, card.w, card.h, C_WHITE); gfx_frame(card.x, card.y, card.w, card.h, C_BLACK);
        gfx_rect(card.x, card.y, card.w, 22, C_NAVY);
        gfx_text_b(card.x + 8, card.y + 7, "GAME OVER", C_WHITE);
        ui_stan(card.x + 14, card.y + 36, g_score > 0 ? 1 : 2, 1);
        ui_wrap(R(card.x + 64, card.y + 36, card.w - 76, 50), g_msg, C_BLACK, 1);
        snprintf(b, sizeof b, g_cfg.hs[cur_game] ? "Your best: %d" : "No best yet", g_cfg.hs[cur_game]);
        gfx_text_b(card.x + 64, card.y + 92, b, C_MAROON);
        if (ui_button_def(R(card.x + 20, card.y + card.h - 40, 120, 28), "Play again", 1) || ui_key(KEY_RETURN)) { game_begin(cur_game); return; }
        if (ui_button(R(card.x + card.w - 140, card.y + card.h - 40, 120, 28), "Arcade", 1) || ui_key(KEY_ESC)) { g_scr = SC_ARCADE; return; }
    }
    if (ui_button(R(c.x + 8, c.y + c.h - 30, 110, 24), "< Arcade (Esc)", 1)) g_scr = SC_ARCADE;
    if (!g_over && ui_button(R(c.x + 128, c.y + c.h - 30, 110, 24), "Restart", 1)) game_begin(cur_game);
    gfx_text(c.x + 250, c.y + c.h - 24, gdef[cur_game].keys, C_GRAY);
}
