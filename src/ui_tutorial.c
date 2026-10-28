/* Orientation Video: two CPU players play a real round on the real battle screen while Stan explains.
   The tutorial drives the genuine Space Planning / Battle screens with synthetic mouse input, using its
   own throw-away Run/Battle (the player's career is backed up and restored). */
#include "ui.h"

enum { T_SAY, T_CLICK_BTN, T_CLICK_PL, T_CLICK_CELL, T_DELAY, T_WAIT_IDLE, T_WAIT_QUIZ, T_FN, T_END };
enum { SEL_MISS, SEL_SCAN, SEL_ASSET_FIRST, SEL_ASSET_NEXT };
enum { HL_NONE, HL_FOE, HL_YOU, HL_STATUS, HL_END, HL_TRAIN, HL_SYN, HL_INTEL, HL_SUPPLY, HL_PL_BOARD, HL_PL_PANEL, HL_PL_SHUF, HL_PL_READY, HL_ITEM_SCAN, HL_ITEM_ENERGY, HL_INFO };
enum { COND_NONE, COND_SUNK, COND_TRAIN_OK, COND_HIT };
enum { FN_SYN100, FN_SYN_BACK };
enum { IT_BTN_END = 0, IT_BTN_TRAIN = 1, IT_BTN_SYN = 2 };

typedef struct { int type, a, hl, mood, cond; const char *text, *alt; } TOp;

#define SAY(hl, mood, txt) { T_SAY, 0, hl, mood, COND_NONE, txt, NULL }
#define SAYC(hl, mood, c, txt, alt) { T_SAY, 0, hl, mood, c, txt, alt }
#define CBTN(id) { T_CLICK_BTN, id, 0, 0, 0, NULL, NULL }
#define CITEM(it) { T_CLICK_BTN, 100 + (it), 0, 0, 0, NULL, NULL }
#define CPL(id) { T_CLICK_PL, id, 0, 0, 0, NULL, NULL }
#define CCELL(sel) { T_CLICK_CELL, sel, 0, 0, 0, NULL, NULL }
#define DELAY(ms) { T_DELAY, ms, 0, 0, 0, NULL, NULL }
#define WAITIDLE { T_WAIT_IDLE, 0, 0, 0, 0, NULL, NULL }
#define WAITQUIZ { T_WAIT_QUIZ, 0, 0, 0, 0, NULL, NULL }
#define FN(f) { T_FN, f, 0, 0, 0, NULL, NULL }

static const TOp script[] = {
    SAY(HL_NONE, 1, "Welcome to Vertex Global Industrial, new hire! I'm Stan the Stapler, HR's official mascot. I have never stapled anything. Today two CPUs play a full round while I explain EVERYTHING. Press NEXT (or Space)."),
    SAY(HL_PL_BOARD, 0, "First: SPACE PLANNING. This grid is YOUR OFFICE and the coloured blobs are your ASSETS (printers, cubicles, one very sad fern). The enemy will shoot at them. Click an Asset to pick it up, click to drop it, R or right-click rotates."),
    CPL(0),
    SAY(HL_PL_BOARD, 1, "CPU-1 pressed SHUFFLE to randomise the layout. Spread your Assets out! If they're clumped together, one lucky shot hurts a lot more."),
    SAY(HL_PL_PANEL, 0, "This panel is INTEL on the floor you're about to fight on: the Department, how smart its AI is, special rules, and how many enemy Assets are hiding. Floor 1 is the Mailroom: its AI just fires at random. Adorable."),
    CPL(1),
    SAY(HL_FOE, 0, "This is THEIR FLOOR, hidden in fog. Somewhere in there are enemy Assets. You click a square to fire a memo at it. Your goal: hit EVERY square of EVERY enemy Asset to repossess them. It's Battleship, but with printers."),
    SAY(HL_STATUS, 0, "The top bar: MORALE is your health (hit zero and you're fired). SYNERGY charges a super-move. ACTIONS are the shots you may take this turn (usually just ONE, so choose wisely). Then your cash and score."),
    SAY(HL_FOE, 2, "CPU-1 has no idea where anything is, so it just picks a square. Watch the pretend mouse (the one labelled CPU-1)."),
    CCELL(SEL_MISS), DELAY(900),
    SAY(HL_FOE, 2, "MISS! That square was just carpet. Misses are marked with a little splash so you never waste a shot twice. That was CPU-1's only Action, so the turn is over."),
    CBTN(IT_BTN_END), DELAY(500),
    SAY(HL_YOU, 0, "Now it's MANAGEMENT BOT's turn. It fires at YOUR OFFICE (the red crosshair). The Mailroom AI shoots randomly; later floors hunt around hits, calculate odds, and cheat a little. Every hit costs you Morale (see 'Dmg/hit' in the INTEL box)."),
    WAITIDLE,
    SAY(HL_SUPPLY, 1, "Turn 2! SUPPLIES are one-use gadgets you find, win and buy. The BADGE SCANNER pings a square and tells you how many squares away the nearest Asset is. It costs no Action. CPU-1 will click the scanner, then a square."),
    CITEM(I_SCANNER), CCELL(SEL_SCAN), DELAY(900),
    SAY(HL_FOE, 0, "See the number? That's the distance to the nearest enemy Asset. Small number = hot. Big number = cold. Now CPU-1 uses its regular shot on a square it suspects."),
    CCELL(SEL_ASSET_FIRST), DELAY(900),
    SAYC(HL_FOE, 1, COND_HIT, "HIT! Assets are blobs of connected squares in odd shapes (L, T, 2x2...). One hit won't repossess it: you need every square, so look at the neighbours. Out of Actions again: END TURN.", "Hm, a miss! It happens. Out of Actions, so END TURN."),
    CBTN(IT_BTN_END), DELAY(500),
    SAY(HL_YOU, 2, "Management Bot returns fire. Don't worry about the damage numbers yet: you heal between floors, and Whiteout and the vending machine can restore Morale too."),
    WAITIDLE,
    SAY(HL_SUPPLY, 1, "Turn 3. CPU-1 drinks an ENERGY DRINK: +2 Actions this turn (you pay 5 Morale for the crash, but Actions are precious). Then it finishes off the Asset it found."),
    CITEM(I_ENERGY), DELAY(300), CCELL(SEL_ASSET_NEXT), DELAY(500), CCELL(SEL_ASSET_NEXT), DELAY(500), CCELL(SEL_ASSET_NEXT), DELAY(900),
    SAYC(HL_FOE, 1, COND_SUNK, "REPOSSESSED! Every square of that Asset is hit, so it's ours: bonus cash, score and Synergy. Clear ALL the enemy's Assets to win the floor.", "Still standing! You'd keep hunting its neighbours next turn. Assets are never more than 5 squares, so it won't take long."),
    SAY(HL_SYN, 1, "SYNERGY fills as you hit, sink and answer questions. At 100% the button lights up and SYNERGIZE! triggers a random absurd super-move like 'Pivot to Video' (5 free shots). We'll cheat and pretend it's full."),
    FN(FN_SYN100),
    SAY(HL_TRAIN, 0, "And now the EDUTAINMENT. Once per turn you can press TRAINING and answer one of Stan's questions: maths, spelling, bookkeeping, world capitals, typing. RIGHT = +1 Action, cash and Synergy. WRONG = you lose Morale. Brains are the best weapon."),
    FN(FN_SYN_BACK),
    CBTN(IT_BTN_TRAIN), WAITQUIZ, DELAY(400),
    SAYC(HL_STATUS, 1, COND_TRAIN_OK, "Correct! Notice the extra Action and cash. (If you answer it yourself next time, the difficulty scales with the floor and your certifications. The Training Center on the desktop lets you practise for free.)", "Wrong! See the Morale drop? Better luck next time. Don't panic: practise for free in the Training Center on the desktop."),
    SAY(HL_FOE, 0, "Not every empty square is boring: some hide HAZARDS and freebies. Free Donuts heal you, Coffee Machines give an Action, Supply Closets drop loot, Wet Floors make your shot slide, and Reply-All Traps ambush you with a pop quiz. Right-click a square to flag it."),
    SAY(HL_INTEL, 0, "INTEL shows damage per hit, how many enemy Assets are left (clear them all to win) and any special RULE twists like Re-org (Assets move!), Firewalls and Overtime."),
    SAY(HL_NONE, 1, "After a win: PERFORMANCE REVIEW (grade, cash, heal, pick a perk), sometimes a random OFFICE EVENT, then the BREAK ROOM SHOP (supplies, new Assets, perks, haggling by quiz) and the ELEVATOR to the next floor. Bosses wait on floors 4, 8, 12 and 14."),
    SAY(HL_NONE, 2, "Lose all Morale, or have your whole office repossessed, and you're FIRED: pink slip, severance package = your score, and a spot in the Hall of Fame. Roguelike rules: no do-overs. Your game saves automatically when you quit, though!"),
    SAY(HL_NONE, 1, "Cheat sheet: Space = End Turn, T = Training, S = Synergize, 1-8 = Supplies, right-click = flag, F1 = help, F9 = boss key. Spread your Assets, train every turn, and never trust the fern. Press FINISH and go get 'em, Intern!"),
    { T_END, 0, 0, 0, 0, NULL, NULL }
};
#define NOPS ((int)(sizeof script / sizeof script[0]))

static struct {
    int on, then_new, op, sub, t, reveal;
    float cx, cy, sx, sy; int tx, ty;
    int fake, fmx, fmy, fpress, fdown, frel;
    int asset, last_x, last_y, quiz_t, train_before, train_ok;
    int bk_in_run, bk_scr, bk_ready;
    char say[400]; int say_mood, say_hl, say_cond;
} T;
static Run bk_run; static Battle bk_bt;

int tut_active(void) { return T.on; }

static void begin(int then_new) {
    int i;
    if (T.on) return;
    if (g_in_run) game_save();
    bk_run = g_run; bk_bt = g_bt; T.bk_in_run = g_in_run; T.bk_scr = ui_game_screen(); T.bk_ready = 1;
    memset(&T, 0, sizeof T);
    T.on = 1; T.then_new = then_new; T.cx = 320; T.cy = 300; T.asset = -1;
    T.bk_in_run = g_in_run; T.bk_scr = ui_game_screen();
    g_cfg.tut_seen = 1; cfg_save();
    run_new(&g_run, "CPU-1", 4242u, CAR_INTERN);
    for (i = 0; i < MAXINV; i++) g_run.inv[i] = 0;
    run_item_add(&g_run, I_SCANNER); run_item_add(&g_run, I_ENERGY);
    g_in_run = 0;
    g_run.floor = 0;
    quiz_cancel();
    scr_go(SC_PLACE);
}
void tut_start(void) { begin(0); }
void tut_start_then_new(void) { begin(1); }

void tut_exit(void) {
    int scr, then_new;
    if (!T.on) return;
    scr = T.bk_scr; then_new = T.then_new;
    g_run = bk_run; g_bt = bk_bt; g_in_run = T.bk_in_run;
    T.on = 0;
    quiz_cancel();
    battle_reset_fx();
    scr_go(then_new ? SC_NEWGAME : SC_DESKTOP);
    if (g_in_run && scr >= 0) ui_set_resume(scr, 1);
    audio_music(MUS_MENU, g_time);
}

/* ------------------------------------------------------------------ targets */
static int slot_of(int item) { int i; for (i = 0; i < MAXINV; i++) if (g_run.inv[i] - 1 == item) return i; return -1; }

static int pick_cell(int sel, int *ox, int *oy) {
    Board *f = &g_bt.foe;
    int x, y, best = 9999, bx = -1, by = -1;
    if (T.asset < 0) {
        int i, bn = 99;
        for (i = 0; i < f->na; i++) if (f->a[i].n >= 2 && f->a[i].n < bn) { bn = f->a[i].n; T.asset = i; }
        if (T.asset < 0) T.asset = 0;
    }
    if (sel == SEL_ASSET_FIRST || sel == SEL_ASSET_NEXT) {
        const Asset *a = &f->a[T.asset];
        int j;
        for (j = 0; j < a->n; j++) if (f->c[a->y[j]][a->x[j]].state == CS_UNKNOWN) { *ox = a->x[j]; *oy = a->y[j]; return 1; }
        return 0;
    }
    for (y = 0; y < f->h; y++) for (x = 0; x < f->w; x++) {
        const Cell *c = &f->c[y][x];
        int d;
        if (c->state != CS_UNKNOWN || c->asset >= 0 || c->haz != 0 || c->lock) continue;
        d = sel == SEL_MISS ? abs(x - f->w / 2) + abs(y - f->h / 2) : abs(x - (f->w - 2)) + abs(y - (f->h - 2));
        /* scan: prefer a square at least 2 away from the tracked asset so the number is interesting */
        if (sel == SEL_SCAN) { const Asset *a = &f->a[T.asset]; int j, md = 99; for (j = 0; j < a->n; j++) md = MIN(md, abs(a->x[j] - x) + abs(a->y[j] - y)); if (md < 2) continue; }
        if (d < best) { best = d; bx = x; by = y; }
    }
    if (bx < 0) return 0;
    *ox = bx; *oy = by; return 1;
}

static int cond_eval(int c) {
    switch (c) {
    case COND_SUNK: return T.asset >= 0 && g_bt.foe.a[T.asset].sunk;
    case COND_TRAIN_OK: return T.train_ok;
    case COND_HIT: return T.last_x >= 0 && (g_bt.foe.c[T.last_y][T.last_x].state == CS_HIT || g_bt.foe.c[T.last_y][T.last_x].state == CS_SUNK);
    default: return 1;
    }
}

static int hl_rect(int hl, Rect *r) {
    switch (hl) {
    case HL_FOE: return battle_ui_rect(3, r);
    case HL_YOU: return battle_ui_rect(4, r);
    case HL_STATUS: return battle_ui_rect(5, r);
    case HL_END: return battle_ui_rect(0, r);
    case HL_TRAIN: return battle_ui_rect(1, r);
    case HL_SYN: return battle_ui_rect(2, r);
    case HL_INTEL: return battle_ui_rect(6, r);
    case HL_SUPPLY: return battle_ui_rect(7, r);
    case HL_PL_BOARD: return place_ui_rect(2, r);
    case HL_PL_PANEL: return place_ui_rect(3, r);
    case HL_PL_SHUF: return place_ui_rect(0, r);
    case HL_PL_READY: return place_ui_rect(1, r);
    default: return 0;
    }
}

/* ------------------------------------------------------------------ per-frame engine */
static int resolve_target(const TOp *op) {
    Rect r;
    int gx, gy, px, py;
    switch (op->type) {
    case T_CLICK_BTN:
        if (op->a >= 100) { int s = slot_of(op->a - 100); if (s < 0 || !battle_ui_rect(20 + s, &r)) return 0; }
        else if (!battle_ui_rect(op->a, &r)) return 0;
        T.tx = r.x + r.w / 2; T.ty = r.y + r.h / 2; return 1;
    case T_CLICK_PL:
        if (!place_ui_rect(op->a, &r)) return 0;
        T.tx = r.x + r.w / 2; T.ty = r.y + r.h / 2; return 1;
    case T_CLICK_CELL:
        if (!pick_cell(op->a, &gx, &gy)) return 0;
        if (!ui_dbg_foe_px(gx, gy, &px, &py)) return 0;
        T.last_x = gx; T.last_y = gy; T.tx = px; T.ty = py; return 1;
    }
    return 0;
}

static void advance(void) { T.op++; T.sub = 0; T.t = 0; }

static void run_engine(int dt) {
    const TOp *op = &script[T.op];
    float k;
    T.fake = T.fpress = T.fdown = T.frel = 0;
    switch (op->type) {
    case T_SAY:
        if (T.sub == 0) {
            const char *txt = (op->alt && !cond_eval(op->cond)) ? op->alt : op->text;
            snprintf(T.say, sizeof T.say, "%s", txt); T.say_mood = op->mood; T.say_hl = op->hl; T.reveal = 0; T.sub = 1;
        }
        T.reveal += dt * 7 / 100 + 1;
        break;
    case T_CLICK_BTN: case T_CLICK_PL: case T_CLICK_CELL:
        if (T.sub == 0) {
            if (!resolve_target(op)) { advance(); return; }
            T.sx = T.cx; T.sy = T.cy; T.t = 0; T.sub = 1;
        }
        T.t += dt;
        T.fake = 1;
        if (T.sub == 1) {
            k = (float)T.t / 520.0f; if (k > 1) k = 1; k = k * k * (3 - 2 * k);
            T.cx = T.sx + (float)(T.tx - T.sx) * k; T.cy = T.sy + (float)(T.ty - T.sy) * k;
            if (T.t >= 620) T.sub = 2;
        } else if (T.sub == 2) { T.fpress = 1; T.fdown = 1; T.sub = 3; }
        else if (T.sub == 3) { T.fdown = 1; T.sub = 4; }
        else if (T.sub == 4) { T.frel = 1; T.sub = 5; T.t = 0; }
        else { if (T.t >= 380) advance(); }
        T.fmx = (int)T.cx; T.fmy = (int)T.cy;
        break;
    case T_DELAY: T.t += dt; if (T.t >= op->a) advance(); break;
    case T_WAIT_IDLE: T.t += dt; if (T.t > 300 && battle_idle()) advance(); break;
    case T_WAIT_QUIZ:
        T.t += dt;
        if (T.t < 350) break;          /* give the quiz a moment to open */
        if (quiz_active()) {
            T.quiz_t += dt;
            if (T.quiz_t > 9000) { quiz_autoanswer(); T.quiz_t = 0; }
        } else { T.train_ok = quiz_last_ok(); advance(); }
        break;
    case T_FN:
        if (op->a == FN_SYN100) g_run.synergy = 100; else g_run.synergy = 45;
        advance(); break;
    default: break;
    }
}

/* ------------------------------------------------------------------ overlay */
static void draw_highlight(const Rect *r) {
    int c1 = ((g_time / 220) & 1) ? C_YELLOW : C_RED;
    gfx_frame(r->x - 3, r->y - 3, r->w + 6, r->h + 6, c1);
    gfx_frame(r->x - 4, r->y - 4, r->w + 8, r->h + 8, C_BLACK);
    gfx_frame(r->x - 2, r->y - 2, r->w + 4, r->h + 4, C_BLACK);
}

static void draw_overlay(void) {
    const TOp *op = &script[T.op];
    Rect bub = R(66, 394, 452, 78), r, nb, sb;
    char buf[420];
    int n, say_active = (op->type == T_SAY), i, total = 0;
    /* highlight */
    if (T.say_hl && hl_rect(T.say_hl, &r)) draw_highlight(&r);
    /* quiz waiting note */
    if (op->type == T_WAIT_QUIZ && quiz_active()) { gfx_rect(110, 60, 420, 14, C_YELLOW); gfx_frame(110, 60, 420, 14, C_BLACK); gfx_text_c(R(110, 60, 420, 14), "CPU-1 is thinking... answer it yourself, or I'll do it in a few seconds.", C_BLACK, 0); }
    /* CPU cursor */
    if (T.fake || op->type == T_CLICK_BTN || op->type == T_CLICK_PL || op->type == T_CLICK_CELL || T.sub == 5) {
        gfx_spr((int)T.cx, (int)T.cy, &S_CURSOR);
        gfx_rect((int)T.cx + 12, (int)T.cy + 12, 32, 10, C_FUCHSIA); gfx_frame((int)T.cx + 12, (int)T.cy + 12, 32, 10, C_BLACK);
        gfx_text((int)T.cx + 14, (int)T.cy + 14, "CPU-1", C_WHITE);
    }
    /* bubble */
    gfx_rect(bub.x + 4, bub.y + 4, bub.w, bub.h, C_BLACK);
    gfx_rect(bub.x, bub.y, bub.w, bub.h, C_WHITE); gfx_frame(bub.x, bub.y, bub.w, bub.h, C_BLACK);
    gfx_line(bub.x, bub.y + 24, bub.x - 10, bub.y + 34, C_BLACK); gfx_line(bub.x, bub.y + 42, bub.x - 10, bub.y + 34, C_BLACK);
    gfx_rect(4, 392, 60, 80, C_SILVER);
    ui_stan(14, bub.y + 14, T.say_mood, 1);
    n = (int)strlen(T.say);
    if (T.reveal < n) { memcpy(buf, T.say, (size_t)T.reveal); buf[T.reveal] = 0; } else snprintf(buf, sizeof buf, "%s", T.say);
    ui_wrap(R(bub.x + 8, bub.y + 6, bub.w - 16, bub.h - 10), buf, C_BLACK, 1);
    for (i = 0; i < NOPS; i++) if (script[i].type == T_SAY) total++;
    { int idx = 0; for (i = 0; i < T.op && i < NOPS; i++) if (script[i].type == T_SAY) idx++;
      snprintf(buf, sizeof buf, "Orientation %d/%d", MIN(idx + (say_active ? 1 : 0), total), total);
      gfx_text(bub.x + bub.w - 100, bub.y + bub.h - 11, buf, C_GRAY); }
    /* buttons */
    nb = R(526, 396, 104, 30); sb = R(526, 436, 104, 24);
    { int last = (op->type == T_END) || (say_active && T.op + 1 < NOPS && script[T.op + 1].type == T_END);
      int enabled = say_active || op->type == T_END;
      if ((ui_button_def(nb, last ? "FINISH" : "Next >", enabled) || (enabled && (ui_key(KEY_SPACE) || ui_key(KEY_RETURN) || ui_key(KEY_RIGHT))))) {
          if (op->type == T_END) { tut_exit(); return; }
          if (T.reveal < n) T.reveal = n; else advance();
      }
    }
    if (ui_button(sb, "Skip tour", 1) || ui_key(KEY_ESC)) { tut_exit(); return; }
}

void tut_frame(void) {
    int dt = g_dt, ok;
    if (!T.on) return;
    if (T.op >= NOPS) T.op = NOPS - 1;
    if (script[T.op].type != T_END) run_engine(dt);
    else { T.fake = T.fpress = T.fdown = T.frel = 0; }
    ui_swallow(1);
    if (T.fake) { g_mx = T.fmx; g_my = T.fmy; g_press = T.fpress; g_down = T.fdown; g_release = T.frel; }
    if (g_scr == SC_PLACE) place_draw(); else battle_draw();
    ui_swallow(0);
    if (T.frel) ui_active_reset();
    ok = (g_scr == SC_PLACE || g_scr == SC_BATTLE);
    if (!ok) { tut_exit(); return; }
    if (g_scr == SC_BATTLE && (g_bt.phase == PH_WIN || g_bt.phase == PH_LOSE || bt_status(&g_bt, &g_run) != 0 || g_run.morale <= 0)) { tut_exit(); return; }
    draw_overlay();
}
