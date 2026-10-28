/* Elevator, review, events, shop, game over, victory, training hub and the quiz modal. */
#include "ui.h"

enum { DLG_GPF_ID = 4 };

/* ================================================================== flow */
static int rv_s, rv_c, rv_h, rv_grade, rv_bonus_c, rv_bonus_h, rv_offer[3], rv_sel, rv_done_flag;

void next_floor(void) {
    if (g_run.floor < 12) g_run.floor++;
    scr_go(SC_ELEVATOR);
}

void after_battle_win(void) {
    int s, c, h, par, ratio;
    bt_floor_reward(&g_bt, &g_run, &s, &c, &h);
    rv_bonus_c = rv_bonus_h = 0;
    if (g_bt.fd.boss) { rv_bonus_c = 60; rv_bonus_h = 25; g_run.cash += 60; run_heal(&g_run, 25); }
    rv_s = s; rv_c = c + rv_bonus_c; rv_h = h + rv_bonus_h;
    par = 22 + g_bt.fd.size; ratio = g_bt.turn * 100 / MAX(1, par);
    rv_grade = ratio <= 60 ? 0 : ratio <= 80 ? 1 : ratio <= 100 ? 2 : ratio <= 130 ? 3 : 4;
    if (g_run.morale < run_max_morale(&g_run) / 4 && rv_grade < 4) rv_grade++;
    scr_go(SC_REVIEW);
}

void go_dead(void) {
    g_in_run = 0; if (!tut_active()) save_clear();
    scr_go(SC_DEAD);
}

/* ================================================================== elevator */
static int el_t, el_from, el_to;
static char el_memo[120], el_stan[100];

static const char *stan_lines[] = {
    "Going up! Please keep all limbs and ambitions inside the cab.",
    "I'm contractually obligated to say: synergy!",
    "Fun fact: I have never once been used to staple anything.",
    "Press 13? There's no 13. Don't ask Facilities.",
    "The elevator is 63% safer than the stairs! (Source: the elevator.)",
    "Remember: a stapler is just a tiny crocodile for paper."};

void elevator_enter(void) {
    Rng r;
    rng_seed(&r, g_run.seed + (unsigned)g_run.floor * 101u + (unsigned)g_run.overtime * 7u);
    gen_memo(&r, el_memo, sizeof el_memo);
    snprintf(el_stan, sizeof el_stan, "%s", stan_lines[rng_n(&r, ARRAY_LEN(stan_lines))]);
    el_t = 0;
    el_to = floor_label(g_run.floor);
    el_from = g_run.floor == 0 ? 0 : floor_label(g_run.floor - 1);
    if (g_run.overtime > 0) { el_from = 14; el_to = 14; }
    audio_music(MUS_ELEVATOR, g_run.seed + (unsigned)g_run.floor);
    audio_sfx(SFX_DING);
}

void elevator_draw(void) {
    Rect c;
    const FloorDef *fd = &g_floors[CLAMP(g_run.floor, 0, 12)];
    int ride = 2800, open_t = 800, i, cur, dw, cx, cabx = 14, caby = 8, cabw = 330, cabh = 380;
    int open;
    char buf[100], fl[16];
    el_t += g_dt;
    c = ui_app("Vertex Global Industrial - Elevator Bank B", 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    cabx += c.x; caby += c.y;
    /* cab */
    gfx_rect(cabx, caby, cabw, cabh, C_GRAY); gfx_dither(cabx, caby, cabw, cabh, C_GRAY, C_SILVER, 1); gfx_frame(cabx, caby, cabw, cabh, C_BLACK);
    /* floor indicator */
    cur = el_t < ride ? el_from + (el_to - el_from) * el_t / ride : el_to;
    if (cur == 13) cur = 12;
    if (g_run.overtime > 0) snprintf(fl, sizeof fl, "OT%d", g_run.overtime); else snprintf(fl, sizeof fl, "%d", cur);
    gfx_rect(cabx + cabw / 2 - 50, caby + 8, 100, 40, C_BLACK); gfx_frame(cabx + cabw / 2 - 50, caby + 8, 100, 40, C_YELLOW);
    gfx_text_big(cabx + cabw / 2 - gfx_text_big_w(fl, 4) / 2, caby + 14, fl, (el_t < ride && ((el_t / 150) & 1)) ? C_RED : C_YELLOW, 4);
    gfx_text(cabx + cabw / 2 - 40, caby + 52, el_t < ride ? "going up..." : "ding!", C_BLACK);
    /* doors */
    open = el_t < ride ? 0 : CLAMP((el_t - ride) * 100 / open_t, 0, 100);
    dw = (cabw - 40) / 2; cx = cabx + 20;
    { int dy = caby + 70, dh = cabh - 80;
      /* hallway behind */
      gfx_rect(cx, dy, dw * 2, dh, C_NAVY);
      gfx_rect(cx, dy + dh - 50, dw * 2, 50, C_TEAL); gfx_dither(cx, dy + dh - 50, dw * 2, 50, C_TEAL, C_NAVY, 0);
      for (i = 0; i < 4; i++) gfx_rect(cx + 14 + i * 64, dy + 8, 40, 4, C_YELLOW);
      gfx_spr_scaled(cx + 8, dy + dh - 98, &S_ASSET[A_CUBICLE], 3);
      gfx_spr_scaled(cx + dw * 2 - 56, dy + dh - 98, &S_ASSET[A_COOLER], 3);
      if (fd->boss && open > 40) draw_boss_portrait(cx + dw - 32, dy + dh - 128, 2, g_run.seed + (unsigned)g_run.floor);
      gfx_text_b(cx + dw - gfx_text_bw(fd->dept) / 2, dy + 22, fd->dept, C_WHITE);
      { int lw = dw * (100 - open) / 100;
        gfx_rect(cx, dy, lw, dh, C_SILVER); gfx_dither(cx, dy, lw, dh, C_SILVER, C_WHITE, 1); gfx_frame(cx, dy, lw + 1, dh, C_BLACK);
        gfx_rect(cx + dw * 2 - lw, dy, lw, dh, C_SILVER); gfx_dither(cx + dw * 2 - lw, dy, lw, dh, C_SILVER, C_WHITE, 1); gfx_frame(cx + dw * 2 - lw - 1, dy, lw + 1, dh, C_BLACK);
        if (lw > 6) { gfx_vline(cx + lw - 3, dy + 4, dh - 8, C_GRAY); gfx_vline(cx + dw * 2 - lw + 2, dy + 4, dh - 8, C_GRAY); } }
      gfx_frame(cx, dy, dw * 2, dh, C_BLACK);
    }
    /* right panel */
    { int rx = cabx + cabw + 14, rw = c.x + c.w - rx - 8, ry = c.y + 8;
      Rect p = R(rx, ry, rw, 250);
      gfx_rect(p.x, p.y, p.w, p.h, C_WHITE); ui_sunken(p);
      if (g_run.overtime > 0) snprintf(buf, sizeof buf, "OVERTIME ROUND %d", g_run.overtime); else snprintf(buf, sizeof buf, "FLOOR %d", floor_label(g_run.floor));
      gfx_text_big(p.x + 10, p.y + 8, buf, C_NAVY, 2);
      gfx_text_b(p.x + 10, p.y + 30, fd->dept, fd->boss ? C_RED : C_MAROON);
      if (fd->boss) { snprintf(buf, sizeof buf, "BOSS FLOOR: %s", fd->boss); gfx_text_b(p.x + 10, p.y + 44, buf, C_RED); }
      gfx_text(p.x + 10, p.y + 58, fd->tag, C_GRAY);
      gfx_hline(p.x + 8, p.y + 72, p.w - 16, C_GRAY);
      gfx_text_b(p.x + 10, p.y + 80, "BRIEFING", C_BLACK);
      ui_wrap(R(p.x + 10, p.y + 94, p.w - 20, 40), el_memo, C_BLACK, 1);
      { char rt[80] = "";
        if (fd->rules & RULE_REORG) { strcat(rt, "RE-ORG "); }
        if (fd->rules & RULE_FIREWALL) { strcat(rt, "FIREWALL "); }
        if (fd->rules & RULE_DOUBLE) { strcat(rt, "OVERTIME "); }
        if (fd->rules & RULE_INJUNCTION) { strcat(rt, "INJUNCTION "); }
        if (fd->rules & RULE_AUDIT) { strcat(rt, "AUDIT "); }
        if (!rt[0]) { strcpy(rt, "none"); }
        snprintf(buf, sizeof buf, "Rule twists: %s", rt); ui_wrap(R(p.x + 10, p.y + 138, p.w - 20, 20), buf, C_PURPLE, 1); }
      snprintf(buf, sizeof buf, "Floor size %dx%d. Damage per hit: %d.", fd->size, fd->size, fd->dmg + g_run.overtime); ui_wrap(R(p.x + 10, p.y + 164, p.w - 20, 20), buf, C_BLACK, 1);
      snprintf(buf, sizeof buf, "Morale %d/%d   Cash $%d   Score %ld", g_run.morale, run_max_morale(&g_run), g_run.cash, g_run.score);
      gfx_text(p.x + 10, p.y + 190, buf, C_NAVY);
      snprintf(buf, sizeof buf, "Employee: %s (%s)", g_run.name, g_career_name[g_run.career]); gfx_text(p.x + 10, p.y + 204, buf, C_GRAY);
      snprintf(buf, sizeof buf, "Employee ID: %u", g_run.seed); gfx_text(p.x + 10, p.y + 218, buf, C_GRAY);
      /* Stan */
      ui_stan(rx + 8, ry + 272, 0, 1);
      { Rect bub = R(rx + 56, ry + 258, rw - 64, 66); gfx_rect(bub.x, bub.y, bub.w, bub.h, C_WHITE); gfx_frame(bub.x, bub.y, bub.w, bub.h, C_BLACK);
        gfx_pixel(bub.x - 1, bub.y + 30, C_WHITE); gfx_line(bub.x, bub.y + 24, bub.x - 8, bub.y + 34, C_BLACK);
        ui_wrap(R(bub.x + 6, bub.y + 6, bub.w - 12, bub.h - 10), el_stan, C_BLACK, 1); }
      if (ui_button_def(R(rx + rw - 150, c.y + c.h - 40, 140, 30), el_t >= ride + open_t ? "Step Out >>" : "Skip ride >>", 1) || ui_key(KEY_RETURN) || ui_key(KEY_SPACE)) {
          if (el_t >= ride + open_t) scr_go(SC_PLACE); else el_t = ride + open_t;
      }
    }
}

/* ================================================================== review */
static const char *grade_s[5] = {"A+", "B", "C", "D", "F"};
void review_enter(void) {
    int i;
    perk_offer(&g_run, rv_offer);
    rv_sel = -1; rv_done_flag = 0;
    for (i = 0; i < 3; i++) if (rv_offer[i] >= 0) rv_done_flag++;
    audio_music(MUS_SHOP, g_run.seed + 99u);
}

static void review_finish(void) {
    if (g_bt.floor_idx >= 12) {
        if (!g_run.won) { g_run.won = 1; g_run.score += 5000; scr_go(SC_WIN); return; }
        g_run.overtime++;
    }
    if (rng_pct(&g_run.rng, 35)) scr_go(SC_EVENT); else scr_go(SC_SHOP);
}

void review_draw(void) {
    Rect c;
    char buf[100];
    int i, hits, acc;
    c = ui_app("Performance Review", 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    { Rect rp = R(c.x + 8, c.y + 8, 300, 228);
      gfx_rect(rp.x, rp.y, rp.w, rp.h, C_WHITE); ui_sunken(rp);
      gfx_text_b(rp.x + 10, rp.y + 8, "PERFORMANCE REVIEW", C_NAVY);
      snprintf(buf, sizeof buf, "Employee: %s", g_run.name); gfx_text(rp.x + 10, rp.y + 22, buf, C_BLACK);
      snprintf(buf, sizeof buf, "Floor %d - %s: CLEARED", floor_label(g_bt.floor_idx), g_bt.fd.dept); gfx_text(rp.x + 10, rp.y + 34, buf, C_GREEN);
      hits = g_bt.hits; acc = g_bt.shots ? hits * 100 / g_bt.shots : 0;
      snprintf(buf, sizeof buf, "Turns taken: %d   Shots: %d   Accuracy: %d%%", g_bt.turn, g_bt.shots, acc); gfx_text(rp.x + 10, rp.y + 52, buf, C_BLACK);
      snprintf(buf, sizeof buf, "Assets repossessed: %d   Lost: %d", board_alive(&g_bt.foe) == 0 ? g_bt.foe.na : 0, g_bt.sunk_you); gfx_text(rp.x + 10, rp.y + 64, buf, C_BLACK);
      snprintf(buf, sizeof buf, "Floor score:  +%d", rv_s); gfx_text(rp.x + 10, rp.y + 86, buf, C_NAVY);
      snprintf(buf, sizeof buf, "Bonus cash:   +$%d", rv_c); gfx_text(rp.x + 10, rp.y + 98, buf, C_OLIVE);
      snprintf(buf, sizeof buf, "Morale healed: +%d", rv_h); gfx_text(rp.x + 10, rp.y + 110, buf, C_GREEN);
      if (rv_bonus_c) gfx_text(rp.x + 10, rp.y + 122, "(Boss bonus included!)", C_RED);
      snprintf(buf, sizeof buf, "Total score: %ld   Cash: $%d", g_run.score, g_run.cash); gfx_text_b(rp.x + 10, rp.y + 144, buf, C_BLACK);
      snprintf(buf, sizeof buf, "Morale %d/%d", g_run.morale, run_max_morale(&g_run)); gfx_text(rp.x + 10, rp.y + 158, buf, C_BLACK);
      gfx_text(rp.x + 10, rp.y + 180, quip_win(&g_rng), C_GRAY);
      gfx_rect(rp.x + 226, rp.y + 24, 60, 56, C_YELLOW); gfx_frame(rp.x + 226, rp.y + 24, 60, 56, C_BLACK);
      gfx_text_big(rp.x + 256 - gfx_text_big_w(grade_s[rv_grade], 3) / 2, rp.y + 38, grade_s[rv_grade], C_RED, 3);
      gfx_text(rp.x + 232, rp.y + 84, "GRADE", C_BLACK);
    }
    gfx_text_b(c.x + 324, c.y + 10, "Choose a Corporate Perk:", C_NAVY);
    if (rv_done_flag == 0) {
        gfx_text(c.x + 324, c.y + 28, "You own every perk. Here is $100.", C_BLACK);
        if (ui_button_def(R(c.x + 324, c.y + 50, 150, 28), "Take the money", 1)) { g_run.cash += 100; review_finish(); }
    }
    for (i = 0; i < 3; i++) {
        Rect cr = R(c.x + 324 + (i % 1) * 0, c.y + 28 + i * 70, 300, 64);
        const PerkDef *pd;
        if (rv_offer[i] < 0) continue;
        pd = &g_perk[rv_offer[i]];
        gfx_rect(cr.x, cr.y, cr.w, cr.h, rv_sel == i ? C_NAVY : C_WHITE); ui_sunken(cr);
        gfx_rect(cr.x + 8, cr.y + 8, 32, 32, C_YELLOW); gfx_bevel(cr.x + 8, cr.y + 8, 32, 32, C_WHITE, C_OLIVE);
        { char l[2] = { pd->name[0], 0 }; gfx_text_big(cr.x + 18, cr.y + 14, l, C_BLACK, 2); }
        gfx_text_b(cr.x + 48, cr.y + 8, pd->name, rv_sel == i ? C_YELLOW : C_NAVY);
        if (pd->rare) gfx_text(cr.x + 240, cr.y + 8, "RARE", rv_sel == i ? C_WHITE : C_RED);
        ui_wrap(R(cr.x + 48, cr.y + 22, cr.w - 56, 36), pd->desc, rv_sel == i ? C_WHITE : C_BLACK, 1);
        snprintf(buf, sizeof buf, "%d", i + 1); gfx_text(cr.x + 4, cr.y + 52, buf, C_GRAY);
        if (ui_hover(cr) && g_press) { rv_sel = i; audio_sfx(SFX_CLICK); }
        if (ui_key('1' + i)) rv_sel = i;
    }
    if (ui_button_def(R(c.x + 324, c.y + 242, 144, 28), "Take Perk", rv_sel >= 0 && rv_offer[rv_sel] >= 0) || (rv_sel >= 0 && ui_key(KEY_RETURN))) {
        g_run.perks |= 1u << rv_offer[rv_sel]; audio_sfx(SFX_POWERUP);
        fx_log(C_YELLOW, "Perk acquired: %s", g_perk[rv_offer[rv_sel]].name);
        if (rv_offer[rv_sel] == P_CHAIR) run_heal(&g_run, 20);
        review_finish();
    }
    if (ui_button(R(c.x + 478, c.y + 242, 146, 28), "Skip (+$30)", rv_done_flag > 0)) { g_run.cash += 30; audio_sfx(SFX_COIN); review_finish(); }
    gfx_text(c.x + 8, c.y + 250, "Perks last for the whole run.  Keys: 1-3 pick, Enter confirm.", C_GRAY);
    /* current perks */
    gfx_text_b(c.x + 8, c.y + 272, "YOUR PERKS", C_BLACK);
    { int n = 0;
      for (i = 0; i < N_PERK; i++) if (HAS(&g_run, i)) {
        Rect pr = R(c.x + 8 + (n % 5) * 124, c.y + 288 + (n / 5) * 14, 120, 12);
        gfx_text(pr.x, pr.y, g_perk[i].name, C_BLACK); n++;
      }
      if (!n) gfx_text(c.x + 8, c.y + 288, "None yet. Corporate is watching.", C_GRAY);
    }
    { Rect sb = R(c.x + 8, c.y + c.h - 40, c.w - 16, 32);
      gfx_text(sb.x, sb.y + 4, "Next stop: the Break Room, where cash becomes advantage.", C_BLACK); }
}

/* ================================================================== events */
static int ev, ev_state, ev_ok, ev_choice; static const char *ev_text;
void event_enter(void) { ev = event_pick(&g_run); ev_state = 0; ev_text = ""; audio_music(MUS_MENU, g_run.seed + 7u); audio_sfx(SFX_ALARM); }

void event_draw(void) {
    Rect c, w;
    const EventDef *e = &g_event[ev];
    int i;
    ui_desktop_bg();
    w = R(60, 60, 520, 360);
    c = ui_window(w, "Random Office Event", 1);
    gfx_spr_scaled(c.x + 14, c.y + 14, &S_MB[1], 3);
    gfx_text_big(c.x + 74, c.y + 14, "!!! OFFICE EVENT !!!", C_RED, 1);
    gfx_text_b(c.x + 74, c.y + 30, e->title, C_NAVY);
    ui_wrap(R(c.x + 74, c.y + 48, c.w - 90, 50), e->text, C_BLACK, 1);
    gfx_hline(c.x + 14, c.y + 100, c.w - 28, C_GRAY);
    if (ev_state == 0) {
        for (i = 0; i < e->nopt; i++) {
            Rect b = R(c.x + 20, c.y + 114 + i * 46, c.w - 40, 38);
            char lab[120];
            snprintf(lab, sizeof lab, "%d. %s  (%d%% chance)", i + 1, e->opt[i].label, e->opt[i].pct);
            if (ui_button(b, lab, 1) || ui_key('1' + i)) { ev_choice = i; ev_text = event_apply(&g_run, ev, i, &ev_ok); ev_state = 1; audio_sfx(ev_ok ? SFX_FANFARE : SFX_WRONG); }
        }
        gfx_text(c.x + 20, c.y + c.h - 24, "Risk it? Choose wisely. Or don't. HR is watching either way.", C_GRAY);
    } else {
        gfx_text_b(c.x + 20, c.y + 116, ev_ok ? "IT WORKED!" : "IT DID NOT WORK.", ev_ok ? C_GREEN : C_RED);
        ui_wrap(R(c.x + 20, c.y + 134, c.w - 40, 60), ev_text, C_BLACK, 1);
        { char b[80]; snprintf(b, sizeof b, "Morale %d/%d   Cash $%d   Synergy %d%%", g_run.morale, run_max_morale(&g_run), g_run.cash, g_run.synergy); gfx_text(c.x + 20, c.y + 200, b, C_NAVY); }
        if (g_run.morale <= 0) {
            if (HAS(&g_run, P_PARACHUTE) && !g_run.parachute_used) { g_run.parachute_used = 1; g_run.morale = 30; }
            else { gfx_text_b(c.x + 20, c.y + 218, "That finished you off.", C_RED); if (ui_button_def(R(c.x + c.w - 130, c.y + c.h - 40, 110, 28), "Ouch", 1)) { g_run.dead = 1; go_dead(); } return; }
        }
        if (ui_button_def(R(c.x + c.w - 130, c.y + c.h - 40, 110, 28), "Continue", 1) || ui_key(KEY_RETURN)) scr_go(SC_SHOP);
    }
}

/* ================================================================== shop */
static int sh_cash_flash, sh_mystery_msg_t, sh_haggled, sh_hover;
static char sh_msg[120];
static Rng sh_rng;

static int fleet_cells(void) { int i, n = 0; for (i = 0; i < g_run.nfleet; i++) n += g_adef[g_run.fleet[i]].n; return n; }
static int next_you_size(void) { int idx = MIN(12, g_run.floor + 1); return idx < 6 ? 9 : 10; }

void shop_enter(void) {
    rng_seed(&sh_rng, g_run.seed * 33u + (unsigned)g_run.floor * 7u + (unsigned)g_run.cash);
    g_run.disc = 0; sh_haggled = 0;
    shop_make(&g_shop, &g_run, &sh_rng);
    snprintf(sh_msg, sizeof sh_msg, "Welcome to the Break Room. Nothing here is free. Except the donuts. They are not free either.");
    audio_music(MUS_SHOP, g_run.seed + (unsigned)g_run.floor);
    sh_mystery_msg_t = 0;
}

static void reprice(void) {
    int k;
    for (k = 0; k < 3; k++) g_shop.iprice[k] = item_price(&g_run, g_item[g_shop.item[k]].price);
    g_shop.aprice = item_price(&g_run, g_adef[g_shop.asset].price);
    g_shop.pprice = item_price(&g_run, 110);
}
static int pay(int amt) {
    if (g_run.cash < amt) { audio_sfx(SFX_ERROR); snprintf(sh_msg, sizeof sh_msg, "Insufficient funds. Have you tried selling a kidney? (Not supported.)"); return 0; }
    g_run.cash -= amt; audio_sfx(SFX_COIN); return 1;
}

void shop_draw(void) {
    Rect c;
    int k, i;
    char b[120];
    c = ui_app("Break Room - Vending, Vendors & Vice", 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    if (quiz_active()) { /* modal handles input */ }
    /* poll haggle result */
    { static int was_active; if (was_active && !quiz_active()) { if (sh_haggled == 1) { if (quiz_last_ok()) { g_run.disc = 25; reprice(); snprintf(sh_msg, sizeof sh_msg, "Haggle success! Everything is 25%% off this visit."); audio_sfx(SFX_FANFARE); } else { run_hurt(&g_run, 5); snprintf(sh_msg, sizeof sh_msg, "Haggle failed. The vendor sneers. -5 Morale."); } sh_haggled = 2; } }
      was_active = quiz_active(); }
    snprintf(b, sizeof b, "Cash: $%d    Morale %d/%d    Synergy %d%%    Floor cleared: %d", g_run.cash, g_run.morale, run_max_morale(&g_run), g_run.synergy, floor_label(g_bt.floor_idx));
    gfx_rect(c.x + 4, c.y + 4, c.w - 8, 18, C_WHITE); ui_sunken(R(c.x + 4, c.y + 4, c.w - 8, 18));
    gfx_spr(c.x + 8, c.y + 5, &S_COIN); gfx_text_b(c.x + 28, c.y + 10, b, C_BLACK);
    /* supplies */
    gfx_text_b(c.x + 8, c.y + 30, "OFFICE SUPPLIES", C_NAVY);
    for (k = 0; k < 3; k++) {
        Rect r = R(c.x + 8 + k * 207, c.y + 44, 200, 84);
        int it = g_shop.item[k], sold = g_shop.isold[k];
        gfx_rect(r.x, r.y, r.w, r.h, C_WHITE); ui_sunken(r);
        gfx_spr_scaled(r.x + 8, r.y + 8, &S_ITEM[it], 2);
        gfx_text_b(r.x + 46, r.y + 8, g_item[it].name, sold ? C_GRAY : C_BLACK);
        ui_wrap(R(r.x + 46, r.y + 20, r.w - 52, 36), g_item[it].desc, sold ? C_GRAY : C_BLACK, 1);
        snprintf(b, sizeof b, sold ? "SOLD" : "Buy $%d", g_shop.iprice[k]);
        if (ui_button(R(r.x + 8, r.y + 58, 90, 20), b, !sold && run_item_count(&g_run) < MIN(MAXINV, g_run.maxinv + (HAS(&g_run, P_DUALMON) ? 2 : 0)) ) ) {
            if (pay(g_shop.iprice[k])) { run_item_add(&g_run, it); g_shop.isold[k] = 1; snprintf(sh_msg, sizeof sh_msg, "Purchased: %s.", g_item[it].name); }
        }
        if (!sold && run_item_count(&g_run) >= MIN(MAXINV, g_run.maxinv + (HAS(&g_run, P_DUALMON) ? 2 : 0))) gfx_text(r.x + 104, r.y + 62, "Bag full!", C_RED);
    }
    /* asset */
    { Rect r = R(c.x + 8, c.y + 136, 308, 100);
      const AssetDef *ad = &g_adef[g_shop.asset];
      int dx[6], dy[6], sold = g_shop.asold;
      int canfit = g_run.nfleet < 10 && fleet_cells() + ad->n <= next_you_size() * next_you_size() * 40 / 100;
      gfx_rect(r.x, r.y, r.w, r.h, C_WHITE); ui_sunken(r);
      gfx_text_b(r.x + 8, r.y + 6, "ASSET FOR YOUR OFFICE", C_NAVY);
      gfx_spr_scaled(r.x + 8, r.y + 22, &S_ASSET[g_shop.asset], 2);
      gfx_text_b(r.x + 46, r.y + 24, ad->name, sold ? C_GRAY : C_BLACK);
      snprintf(b, sizeof b, "%s: %s", ad->perk, ad->perkdesc); ui_wrap(R(r.x + 46, r.y + 36, 170, 30), b, sold ? C_GRAY : C_BLACK, 1);
      asset_shape(g_shop.asset, 0, dx, dy);
      for (i = 0; i < ad->n; i++) { gfx_rect(r.x + 236 + dx[i] * 10, r.y + 28 + dy[i] * 10, 9, 9, ad->color); gfx_frame(r.x + 236 + dx[i] * 10, r.y + 28 + dy[i] * 10, 9, 9, C_BLACK); }
      snprintf(b, sizeof b, sold ? "SOLD" : "Buy $%d", g_shop.aprice);
      if (ui_button(R(r.x + 8, r.y + 74, 90, 20), b, !sold && canfit)) {
          if (pay(g_shop.aprice)) { g_run.fleet[g_run.nfleet++] = g_shop.asset; g_shop.asold = 1; snprintf(sh_msg, sizeof sh_msg, "Your office expands: new %s!", ad->name); }
      }
      snprintf(b, sizeof b, "Fleet %d/10   Cells %d", g_run.nfleet, fleet_cells()); gfx_text(r.x + 104, r.y + 78, b, canfit ? C_GRAY : C_RED);
    }
    /* perk */
    { Rect r = R(c.x + 324, c.y + 136, 300, 100);
      int sold = g_shop.psold || g_shop.perk < 0;
      gfx_rect(r.x, r.y, r.w, r.h, C_WHITE); ui_sunken(r);
      gfx_text_b(r.x + 8, r.y + 6, "CORPORATE PERK", C_NAVY);
      if (g_shop.perk >= 0) {
          gfx_rect(r.x + 8, r.y + 22, 28, 28, C_YELLOW); gfx_bevel(r.x + 8, r.y + 22, 28, 28, C_WHITE, C_OLIVE);
          { char l[2] = { g_perk[g_shop.perk].name[0], 0 }; gfx_text_big(r.x + 16, r.y + 28, l, C_BLACK, 2); }
          gfx_text_b(r.x + 44, r.y + 24, g_perk[g_shop.perk].name, sold ? C_GRAY : C_BLACK);
          ui_wrap(R(r.x + 44, r.y + 36, 248, 36), g_perk[g_shop.perk].desc, sold ? C_GRAY : C_BLACK, 1);
      } else gfx_text(r.x + 8, r.y + 30, "No perks left to sell.", C_GRAY);
      snprintf(b, sizeof b, sold ? "SOLD" : "Buy $%d", g_shop.pprice);
      if (ui_button(R(r.x + 8, r.y + 74, 90, 20), b, !sold)) { if (pay(g_shop.pprice)) { g_run.perks |= 1u << g_shop.perk; g_shop.psold = 1; audio_sfx(SFX_POWERUP); snprintf(sh_msg, sizeof sh_msg, "Perk acquired: %s.", g_perk[g_shop.perk].name); if (g_shop.perk == P_CHAIR) run_heal(&g_run, 20); } }
    }
    /* services */
    gfx_text_b(c.x + 8, c.y + 244, "SERVICES", C_NAVY);
    if (ui_button(R(c.x + 8, c.y + 258, 190, 26), "Vending Heal +25 Morale ($30)", g_run.morale < run_max_morale(&g_run))) { if (pay(item_price(&g_run, 30))) { run_heal(&g_run, 25); snprintf(sh_msg, sizeof sh_msg, "Mmm, vending machine nachos. +25 Morale."); } }
    { int mp = item_price(&g_run, 40 + 20 * g_shop.mystery_sold), left = 2 - g_shop.mystery_sold;
      if (left > 0) snprintf(b, sizeof b, "Mystery Box $%d (%d left)", mp, left); else snprintf(b, sizeof b, "Box: sold out");
      if (ui_button(R(c.x + 204, c.y + 258, 140, 26), b, left > 0)) {
        if (pay(mp)) {
            int roll = rng_n(&sh_rng, 100);
            g_shop.mystery_sold++;
            if (roll < 38) { int it = rng_n(&sh_rng, N_ITEM); if (run_item_add(&g_run, it)) snprintf(sh_msg, sizeof sh_msg, "Mystery Box: a %s!", g_item[it].name); else { g_run.cash += mp; snprintf(sh_msg, sizeof sh_msg, "Mystery Box: a %s... but your bag is full. Full refund.", g_item[it].name); } }
            else if (roll < 63) { int cc = 10 + rng_n(&sh_rng, mp > 20 ? mp - 10 : 10); g_run.cash += cc; snprintf(sh_msg, sizeof sh_msg, "Mystery Box: an envelope with $%d!", cc); }
            else if (roll < 78) { int o[3]; perk_offer(&g_run, o); if (o[0] >= 0) { g_run.perks |= 1u << o[0]; snprintf(sh_msg, sizeof sh_msg, "Mystery Box: a PERK - %s!", g_perk[o[0]].name); audio_sfx(SFX_POWERUP); } else { g_run.cash += mp; snprintf(sh_msg, sizeof sh_msg, "Mystery Box: no perks left. Refund.");} }
            else if (roll < 90) { run_heal(&g_run, 40); snprintf(sh_msg, sizeof sh_msg, "Mystery Box: a casserole! +40 Morale."); }
            else { run_synergy(&g_run, 50); snprintf(sh_msg, sizeof sh_msg, "Mystery Box: a single paperclip. +50 Synergy somehow."); }
        }
      } }
    if (ui_button(R(c.x + 350, c.y + 258, 130, 26), sh_haggled ? "Haggled" : "Haggle (quiz)", !sh_haggled)) { sh_haggled = 1; quiz_open(QM_HAGGLE, rng_n(&g_rng, N_MODULE)); }
    snprintf(b, sizeof b, "Reroll Stock ($15)");
    if (ui_button(R(c.x + 486, c.y + 258, 138, 26), b, 1)) {
        if (pay(15)) { int old = g_run.disc, mb = g_shop.mystery_sold; shop_make(&g_shop, &g_run, &sh_rng); g_run.disc = old; g_shop.mystery_sold = mb; reprice(); snprintf(sh_msg, sizeof sh_msg, "Fresh stock! (Same dust.)"); audio_sfx(SFX_SHUFFLE); }
    }
    /* inventory */
    gfx_text_b(c.x + 8, c.y + 294, "YOUR SUPPLIES", C_NAVY);
    sh_hover = -1;
    for (i = 0; i < MAXINV; i++) {
        Rect sr = R(c.x + 8 + i * 38, c.y + 308, 34, 34);
        int it = g_run.inv[i] - 1, cap = MIN(MAXINV, g_run.maxinv + (HAS(&g_run, P_DUALMON) ? 2 : 0));
        if (i >= cap) { gfx_rect(sr.x, sr.y, 34, 34, C_GRAY); gfx_frame(sr.x, sr.y, 34, 34, C_BLACK); continue; }
        gfx_rect(sr.x, sr.y, 34, 34, C_WHITE); ui_sunken(sr);
        if (it >= 0) { gfx_spr_scaled(sr.x + 1, sr.y + 1, &S_ITEM[it], 2); if (ui_hover(sr)) sh_hover = it; }
    }
    if (sh_hover >= 0) { snprintf(b, sizeof b, "%s: %s", g_item[sh_hover].name, g_item[sh_hover].desc); }
    else b[0] = 0;
    gfx_text(c.x + 8, c.y + 346, b, C_BLACK);
    /* fleet */
    gfx_text_b(c.x + 330, c.y + 294, "YOUR OFFICE FLEET", C_NAVY);
    for (i = 0; i < g_run.nfleet; i++) { gfx_spr(c.x + 330 + (i % 10) * 22, c.y + 308 + (i / 10) * 20, &S_ASSET[g_run.fleet[i]]); }
    /* perks */
    { int n = 0; gfx_text_b(c.x + 330, c.y + 346, "PERKS:", C_NAVY);
      for (i = 0; i < N_PERK; i++) if (HAS(&g_run, i)) { Rect pr = R(c.x + 380 + (n % 12) * 20, c.y + 342 + (n / 12) * 20, 18, 18); char l[2] = { g_perk[i].name[0], 0 };
        gfx_rect(pr.x, pr.y, 18, 18, C_YELLOW); gfx_bevel(pr.x, pr.y, 18, 18, C_WHITE, C_OLIVE); gfx_text_b(pr.x + 6, pr.y + 6, l, C_BLACK);
        if (ui_hover(pr)) { snprintf(b, sizeof b, "%s: %s", g_perk[i].name, g_perk[i].desc); gfx_rect(c.x + 8, c.y + 346, 316, 10, C_SILVER); ui_wrap(R(c.x + 8, c.y + 346, 316, 20), b, C_BLACK, 1); } n++; } }
    /* message + leave */
    gfx_rect(c.x + 4, c.y + c.h - 52, c.w - 8, 20, C_BLACK);
    gfx_clip(R(c.x + 6, c.y + c.h - 50, c.w - 12, 16)); gfx_text(c.x + 8, c.y + c.h - 45, sh_msg, C_LIME); gfx_unclip();
    if (ui_button_def(R(c.x + c.w - 190, c.y + c.h - 28, 182, 24), "Back to Work (Elevator) >>", 1) || (ui_key(KEY_RETURN))) next_floor();
    (void)sh_cash_flash; (void)sh_mystery_msg_t;
}

/* ================================================================== game over */
static int dead_t, dead_gpf, dead_rank;
static char dead_quip[80];
void dead_enter(void) {
    dead_t = 0;
    dead_gpf = (g_run.seed + (unsigned)g_run.score) % 5 == 0 && !g_run.won;
    dead_rank = hof_add(g_run.name, g_run.score, floor_label(g_run.floor) + (g_run.overtime ? g_run.overtime : 0), g_run.career, g_run.won, g_run.seed);
    snprintf(dead_quip, sizeof dead_quip, "%s", g_run.won ? "You retired with dignity (and severance)." : quip_lose(&g_rng));
    audio_music(MUS_DEAD, g_run.seed);
    g_in_run = 0;
}
void dead_draw(void) {
    Rect c;
    char b[100];
    int i, x, y;
    dead_t += g_dt;
    ui_desktop_bg();
    c = ui_window(R(70, 18, 500, 444), g_run.won ? "Retirement Notice" : "Notice of Termination", 1);
    gfx_rect(c.x + 10, c.y + 8, c.w - 20, c.h - 52, C_WHITE); gfx_frame(c.x + 10, c.y + 8, c.w - 20, c.h - 52, C_BLACK);
    gfx_rect(c.x + 10, c.y + 8, c.w - 20, 40, g_run.won ? C_YELLOW : C_FUCHSIA); gfx_hline(c.x + 10, c.y + 48, c.w - 20, C_BLACK);
    gfx_text_big(c.x + c.w / 2 - gfx_text_big_w(g_run.won ? "PROMOTED TO CEO!" : "YOU'RE FIRED!", 3) / 2, c.y + 15, g_run.won ? "PROMOTED TO CEO!" : "YOU'RE FIRED!", g_run.won ? C_NAVY : C_BLACK, 3);
    gfx_text(c.x + 24, c.y + 60, "VERTEX GLOBAL INDUSTRIAL - HUMAN RESOURCES", C_GRAY);
    snprintf(b, sizeof b, "Employee: %s (%s)   ID %u", g_run.name, g_career_name[g_run.career], g_run.seed); gfx_text(c.x + 24, c.y + 74, b, C_BLACK);
    if (g_run.won) snprintf(b, sizeof b, "Cleared all floors%s.", g_run.overtime ? " and kept going into Overtime" : "");
    else snprintf(b, sizeof b, "Cause: %s on floor %d (%s).", g_bt.cause == 1 ? "office repossessed" : "burnout", floor_label(g_run.floor), g_floors[CLAMP(g_run.floor, 0, 12)].dept);
    gfx_text(c.x + 24, c.y + 88, b, C_MAROON);
    gfx_text(c.x + 24, c.y + 104, dead_quip, C_NAVY);
    gfx_hline(c.x + 20, c.y + 122, c.w - 40, C_GRAY);
    gfx_text_b(c.x + 24, c.y + 130, "SEVERANCE PACKAGE", C_BLACK);
    snprintf(b, sizeof b, "$%ld", g_run.score); gfx_text_big(c.x + 24, c.y + 144, b, C_GREEN, 3);
    if (dead_rank >= 0 && dead_rank < HOF_N) { snprintf(b, sizeof b, "Hall of Fame rank: #%d !", dead_rank + 1); gfx_text_b(c.x + 250, c.y + 150, b, C_RED); }
    else gfx_text(c.x + 250, c.y + 150, "Didn't place in the Hall of Fame.", C_GRAY);
    { int acc = g_run.shots ? g_run.hits * 100 / g_run.shots : 0, ca = g_run.correct + g_run.wrong ? g_run.correct * 100 / (g_run.correct + g_run.wrong) : 0;
      x = c.x + 24; y = c.y + 188;
      snprintf(b, sizeof b, "Floors cleared: %d     Turns played: %d", g_run.floors_cleared, g_run.turns); gfx_text(x, y, b, C_BLACK);
      snprintf(b, sizeof b, "Shots fired: %d   Hits: %d   Accuracy: %d%%", g_run.shots, g_run.hits, acc); gfx_text(x, y + 12, b, C_BLACK);
      snprintf(b, sizeof b, "Assets repossessed: %d", g_run.sunk); gfx_text(x, y + 24, b, C_BLACK);
      snprintf(b, sizeof b, "Training: %d right, %d wrong (%d%%)   Best streak: %d", g_run.correct, g_run.wrong, ca, g_run.best_streak); gfx_text(x, y + 36, b, C_BLACK);
      snprintf(b, sizeof b, "Perks collected: %d   Cash left: $%d", __builtin_popcount(g_run.perks), g_run.cash); gfx_text(x, y + 48, b, C_BLACK); }
    gfx_text_b(c.x + 24, c.y + 258, "CERTIFICATIONS", C_BLACK);
    for (i = 0; i < N_MODULE; i++) {
        int lv = run_cert_level(&g_run, i);
        snprintf(b, sizeof b, "%s: %s", g_modname[i], lv ? cert_name(i, lv) : "(none)");
        gfx_text(c.x + 24, c.y + 272 + i * 12, b, lv ? C_NAVY : C_GRAY);
    }
    gfx_spr_scaled(c.x + c.w - 90, c.y + 268, g_run.won ? &S_STAR : &S_SMILE, 3);
    if (ui_button_def(R(c.x + 20, c.y + c.h - 38, 140, 28), "New Game", 1)) { g_in_run = 0; scr_go(SC_NEWGAME); }
    if (ui_button(R(c.x + 170, c.y + c.h - 38, 160, 28), "Program Manager", 1)) { g_in_run = 0; scr_go(SC_DESKTOP); }
    if (ui_button(R(c.x + 340, c.y + c.h - 38, 140, 28), "Hall of Fame", 1)) ui_menu_request(ACT_HOF);
    if (dead_gpf && dead_t > 1500 && dead_t < 1500 + 40) { ui_dialog(DLG_GPF_ID, "HOSTILE", "HOSTILE.EXE has caused a General Protection Fault in module HOSTILE.EXE at 0001:0042.\n\n(Just kidding. You were simply fired.)", 3, "Close", "Ignore", NULL); }
}

/* ================================================================== victory */
typedef struct { float x, y, vx, vy; int suit, face; } Card;
static Card cards[26];
static int win_t, win_init;
static unsigned char trail[SCR_W * SCR_H];

void win_enter(void) {
    int i;
    win_t = 0; win_init = 0;
    for (i = 0; i < 26; i++) { cards[i].x = -200; cards[i].y = 0; cards[i].vx = 0; cards[i].vy = 0; cards[i].suit = i % 4; cards[i].face = i % 13; }
    audio_music(MUS_VICTORY, g_run.seed);
    audio_sfx(SFX_FANFARE);
}

static void draw_card(int x, int y, int suit, int face) {
    static const char *fn[13] = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
    int col = (suit & 1) ? C_RED : C_BLACK, cx = x + 18, cy = y + 28;
    gfx_rect(x, y, 36, 48, C_WHITE); gfx_frame(x, y, 36, 48, C_BLACK);
    gfx_text_b(x + 3, y + 3, fn[face], col);
    switch (suit) {
    case 0: gfx_circle(cx, cy - 4, 4, col, 1); gfx_circle(cx - 5, cy + 2, 4, col, 1); gfx_circle(cx + 5, cy + 2, 4, col, 1); gfx_rect(cx - 1, cy + 3, 3, 8, col); break;      /* club */
    case 1: { int k; for (k = 0; k < 9; k++) { gfx_hline(cx - k, cy - 8 + k, 2 * k + 1, col); } for (k = 1; k < 9; k++) { gfx_hline(cx - (8 - k), cy + k, 2 * (8 - k) + 1, col); } } break;   /* diamond */
    case 2: gfx_circle(cx - 4, cy - 3, 4, col, 1); gfx_circle(cx + 4, cy - 3, 4, col, 1); { int k; for (k = 0; k < 9; k++) gfx_hline(cx - 8 + k, cy - 1 + k, 17 - 2 * k > 0 ? 17 - 2 * k : 1, col); } break;   /* heart */
    default: { int k; for (k = 0; k < 8; k++) { gfx_hline(cx - k, cy - 9 + k, 2 * k + 1, col); } gfx_circle(cx - 4, cy - 1, 4, col, 1); gfx_circle(cx + 4, cy - 1, 4, col, 1); gfx_rect(cx - 1, cy + 2, 3, 8, col); } break;   /* spade-ish */
    }
}

void win_draw(void) {
    Rect c;
    int i;
    char b[100];
    win_t += g_dt;
    if (!win_init) { gfx_rect(0, 0, SCR_W, SCR_H, C_GREEN); gfx_dither(0, 0, SCR_W, SCR_H, C_GREEN, C_TEAL, 1); memcpy(trail, g_fb, sizeof trail); win_init = 1; }
    memcpy(g_fb, trail, sizeof trail);
    for (i = 0; i < 26; i++) {
        Card *k = &cards[i];
        if (win_t > i * 380 && k->x < -100) { k->x = (float)(100 + (i * 97) % 440); k->y = 40.0f; k->vx = (float)((i & 1) ? -2.2f - (i % 5) * 0.4f : 2.2f + (i % 5) * 0.4f); k->vy = -1.0f; }
        if (k->x > -100) {
            int px, py;
            k->vy += 0.34f * (g_dt / 16.0f); k->x += k->vx * (g_dt / 16.0f); k->y += k->vy * (g_dt / 16.0f);
            if (k->y > SCR_H - 48) { k->y = SCR_H - 48; k->vy = -k->vy * 0.82f; if (k->vy > -2.0f) k->vy = -6.0f - (i % 4); }
            if (k->x < -40 || k->x > SCR_W + 4) { k->x = (float)(60 + (i * 53) % 480); k->vx = -k->vx; }
            px = (int)k->x; py = (int)k->y;
            draw_card(px, py, k->suit, k->face);
            { int yy, w2 = MIN(36, SCR_W - px), h2 = MIN(48, SCR_H - py);
              if (px >= 0 && py >= 0 && w2 > 0 && h2 > 0) for (yy = 0; yy < h2; yy++) memcpy(trail + (py + yy) * SCR_W + px, g_fb + (py + yy) * SCR_W + px, (size_t)w2); }
        }
    }
    c = ui_window(R(120, 110, 400, 270), "Congratulations!", 1);
    gfx_text_big(c.x + c.w / 2 - gfx_text_big_w("HOSTILE TAKEOVER", 2) / 2, c.y + 12, "HOSTILE TAKEOVER", C_MAROON, 2);
    gfx_text_big(c.x + c.w / 2 - gfx_text_big_w("COMPLETE!", 3) / 2, c.y + 36, "COMPLETE!", C_NAVY, 3);
    gfx_text(c.x + 20, c.y + 80, "The CEO has been escorted from the building by", C_BLACK);
    gfx_text(c.x + 20, c.y + 92, "a very polite security guard named Doug.", C_BLACK);
    snprintf(b, sizeof b, "%s is the new CEO of Vertex Global Industrial.", g_run.name); gfx_text_b(c.x + 20, c.y + 112, b, C_BLACK);
    snprintf(b, sizeof b, "Score: %ld   (+5000 Takeover bonus)", g_run.score); gfx_text(c.x + 20, c.y + 132, b, C_GREEN);
    gfx_text(c.x + 20, c.y + 148, "Your first act as CEO: restore the stapler budget.", C_GRAY);
    gfx_spr_scaled(c.x + c.w - 70, c.y + 80, &S_STAR, 3);
    if (ui_button_def(R(c.x + 16, c.y + c.h - 40, 180, 28), "Continue: OVERTIME", 1)) { g_run.overtime = 1; scr_go(SC_SHOP); }
    if (ui_button(R(c.x + 210, c.y + c.h - 40, 170, 28), "Retire (cash out)", 1)) { g_bt.cause = 0; go_dead(); }
}

/* ================================================================== training hub + quiz */
static struct {
    int active, mode, module, diff, state, ok, sel, tlen, t_left, t_total, last_ok, last_module, last_diff, hint_shown, free_pass;
    Question q; char typed[56]; unsigned int t0; int flash;
} qz;
#define prac_diff g_cfg.pdiff
#define prac_right g_cfg.pright
#define prac_wrong g_cfg.pwrong
#define prac_xp g_cfg.pxp
static int prac_streak;
int quiz_active(void) { return qz.active; }
void quiz_cancel(void) { qz.active = 0; }
int quiz_last_ok(void) { return qz.last_ok; }
int quiz_last_module(void) { return qz.last_module; }
int quiz_last_diff(void) { return qz.last_diff; }

void quiz_open(int mode, int module) {
    memset(&qz.q, 0, sizeof qz.q);
    qz.active = 1; qz.mode = mode; qz.module = CLAMP(module, 0, N_MODULE - 1); qz.state = 0; qz.sel = -1; qz.typed[0] = 0; qz.tlen = 0; qz.flash = 0;
    qz.diff = mode == QM_PRACTICE ? prac_diff : mode == QM_HAGGLE ? 2 : run_quiz_diff(&g_run, qz.module);
    quiz_make(&qz.q, &g_rng, qz.module, qz.diff);
    qz.t_total = mode == QM_REPLY ? 8000 : (mode == QM_PRACTICE && qz.q.kind == 0) ? 0 : qz.q.time_ms;
    if (mode == QM_REPLY && qz.q.kind == 1) qz.t_total = qz.q.time_ms;
    qz.t_left = qz.t_total;
    audio_sfx(mode == QM_REPLY ? SFX_ALARM : SFX_DING);
}

static void quiz_answer(int ok) {
    qz.ok = ok; qz.state = 1;
    qz.last_ok = ok; qz.last_module = qz.module; qz.last_diff = qz.diff;
    audio_sfx(ok ? SFX_CORRECT : SFX_WRONG);
    if (qz.mode == QM_PRACTICE) {
        if (ok) { prac_right++; prac_streak++; prac_xp[qz.module]++; if (prac_streak > g_cfg.pbest) g_cfg.pbest = prac_streak; } else { prac_wrong++; prac_streak = 0; }
        cfg_save();
    }
}

static void quiz_answer(int ok);
void quiz_autoanswer(void) {
    if (!qz.active || qz.state != 0) return;
    if (qz.q.kind == 0) { qz.sel = qz.q.correct; quiz_answer(1); }
    else { snprintf(qz.typed, sizeof qz.typed, "%s", qz.q.ans); quiz_answer(1); }
}

void quiz_frame(void) {
    Rect c, qb;
    int i, x, y, w = 540, h = 340, mood;
    char b[160];
    const char *ttl;
    if (!qz.active) return;
    x = (SCR_W - w) / 2; y = (SCR_H - h) / 2 - 8;
    ttl = qz.mode == QM_REPLY ? "REPLY-ALL TRAP!" : qz.mode == QM_HAGGLE ? "Haggle - Quick Quiz" : qz.mode == QM_PRACTICE ? "Training Center - Practice" : "Mandatory Training";
    gfx_rect(x + 6, y + 6, w, h, C_BLACK);
    c = ui_window(R(x, y, w, h), ttl, 1);
    gfx_rect(c.x, c.y, c.w, c.h, qz.mode == QM_REPLY ? C_MAROON : C_SILVER);
    mood = qz.state == 0 ? 0 : (qz.ok ? 1 : 2);
    ui_stan(c.x + 10, c.y + 10, mood, 1);
    gfx_text(c.x + 8, c.y + 52, "Stan the Stapler", qz.mode == QM_REPLY ? C_WHITE : C_NAVY);
    snprintf(b, sizeof b, "%s  (%s)", g_modname[qz.module], qz.diff <= 1 ? "Easy" : qz.diff == 2 ? "Normal" : qz.diff == 3 ? "Tough" : qz.diff == 4 ? "Hard" : "Insane");
    gfx_text_b(c.x + 8, c.y + 64, b, qz.mode == QM_REPLY ? C_YELLOW : C_MAROON);
    /* speech bubble */
    qb = R(c.x + 160, c.y + 10, c.w - 172, 78);
    gfx_rect(qb.x, qb.y, qb.w, qb.h, C_WHITE); gfx_frame(qb.x, qb.y, qb.w, qb.h, C_BLACK);
    gfx_line(qb.x, qb.y + 20, qb.x - 12, qb.y + 28, C_BLACK); gfx_line(qb.x, qb.y + 34, qb.x - 12, qb.y + 28, C_BLACK);
    ui_wrap(R(qb.x + 8, qb.y + 8, qb.w - 16, qb.h - 12), qz.q.q, C_BLACK, 1);
    /* timer */
    if (qz.t_total > 0 && qz.state == 0) {
        qz.t_left -= g_dt;
        ui_bar(R(c.x + 8, c.y + 98, c.w - 16, 14), qz.t_left > 0 ? qz.t_left : 0, qz.t_total, qz.t_left * 3 < qz.t_total ? C_RED : C_BLUE, NULL);
        if (qz.t_left <= 0) { if (qz.q.kind == 0) qz.sel = -1; quiz_answer(0); }
    } else if (qz.t_total == 0 && qz.state == 0) gfx_text(c.x + 8, c.y + 100, "No time limit in practice.", C_GRAY);
    if (qz.state == 0) {
        if (qz.q.kind == 0) {
            for (i = 0; i < qz.q.nopt; i++) {
                Rect r = R(c.x + 20 + (i % 2) * 255, c.y + 126 + (i / 2) * 50, 240, 40);
                int hit = ui_button(r, "", 1);
                gfx_rect(r.x + 6, r.y + 8, 24, 24, C_NAVY); gfx_frame(r.x + 6, r.y + 8, 24, 24, C_BLACK);
                snprintf(b, sizeof b, "%d", i + 1); gfx_text_c(R(r.x + 6, r.y + 8, 24, 24), b, C_YELLOW, 1);
                gfx_text_b(r.x + 38, r.y + 16, qz.q.opt[i], C_BLACK);
                if (hit || ui_key('1' + i)) { qz.sel = i; quiz_answer(i == qz.q.correct); }
            }
            gfx_text(c.x + 20, c.y + 232, "Click an answer, or press the number key (1-4) on the blue tile.", qz.mode == QM_REPLY ? C_WHITE : C_GRAY);
        } else {
            int cc;
            gfx_text_b(c.x + 20, c.y + 124, "Type exactly:", qz.mode == QM_REPLY ? C_WHITE : C_BLACK);
            gfx_rect(c.x + 20, c.y + 140, c.w - 40, 22, C_YELLOW); gfx_frame(c.x + 20, c.y + 140, c.w - 40, 22, C_BLACK);
            gfx_text_big(c.x + 28, c.y + 144, qz.q.ans, C_BLACK, 1);
            ui_textfield(R(c.x + 20, c.y + 172, c.w - 130, 22), qz.typed, 50, 1, 0);
            if (ui_button_def(R(c.x + c.w - 100, c.y + 170, 80, 26), "Submit", qz.typed[0] != 0)) quiz_answer(strcmp(qz.typed, qz.q.ans) == 0);
            cc = (int)strlen(qz.typed);
            { int k, good = 1; for (k = 0; k < cc; k++) if (qz.typed[k] != qz.q.ans[k]) { good = 0; break; }
              gfx_text(c.x + 20, c.y + 200, good ? "Looking good..." : "Typo! Backspace!", good ? C_GREEN : C_RED); }
            if (qz.state == 0 && ui_key(KEY_RETURN)) quiz_answer(strcmp(qz.typed, qz.q.ans) == 0);
        }
    } else {
        /* result */
        gfx_text_big(c.x + 20, c.y + 124, qz.ok ? "CORRECT!" : (qz.t_total > 0 && qz.t_left <= 0 ? "TIME'S UP!" : "WRONG!"), qz.ok ? C_GREEN : C_RED, 3);
        if (qz.q.kind == 0) { snprintf(b, sizeof b, "Answer: %s", qz.q.opt[qz.q.correct]); gfx_text_b(c.x + 20, c.y + 158, b, C_BLACK); }
        ui_wrap(R(c.x + 20, c.y + 174, c.w - 40, 44), qz.q.expl, C_NAVY, 1);
        if (qz.mode == QM_TRAIN) gfx_text(c.x + 20, c.y + 222, qz.ok ? "+1 Action, bonus cash and Synergy!" : "Morale lost. Pay attention, Intern.", qz.ok ? C_GREEN : C_RED);
        else if (qz.mode == QM_REPLY) gfx_text(c.x + 20, c.y + 222, qz.ok ? "Reply-all averted!" : "Reply-all SENT. -12 Morale.", qz.ok ? C_LIME : C_YELLOW);
        else if (qz.mode == QM_HAGGLE) gfx_text(c.x + 20, c.y + 222, qz.ok ? "The vendor reluctantly discounts everything 25%." : "The vendor laughs. -5 Morale.", C_BLACK);
        else { snprintf(b, sizeof b, "Practice: %d right, %d wrong, streak %d", prac_right, prac_wrong, prac_streak); gfx_text(c.x + 20, c.y + 222, b, C_BLACK); }
        if (ui_button_def(R(c.x + c.w - 150, c.y + c.h - 40, 130, 28), "Continue", 1) || ui_key(KEY_RETURN) || ui_key(KEY_SPACE)) {
            qz.active = 0;
        }
    }
}

void train_draw(void) {
    Rect c;
    int i;
    char b[100];
    c = ui_app("Training Center - Mandatory Fun Division", 1);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER);
    gfx_text_b(c.x + 10, c.y + 8, "Practice with no risk. Questions scale with the difficulty you pick.", C_NAVY);
    { static const char *dn[4] = {"Easy", "Normal", "Hard", "Insane"}; static const int dv[4] = {1, 2, 4, 5};
      static const char *dd[4] = {"Small numbers, short words, wide-apart answers.", "Everyday office questions.", "Bigger numbers, tax, tricky spelling, close answers.", "Multi-step maths, HST, ledgers, punctuation. Good luck."};
      gfx_text_b(c.x + 10, c.y + 27, "Difficulty:", C_BLACK);
      for (i = 0; i < 4; i++) { Rect r = R(c.x + 84 + i * 70, c.y + 22, 66, 22);
        if (prac_diff == dv[i]) { gfx_rect(r.x, r.y, r.w, r.h, C_NAVY); gfx_frame(r.x, r.y, r.w, r.h, C_BLACK); gfx_text_c(r, dn[i], C_YELLOW, 1); }
        else if (ui_button(r, dn[i], 1)) prac_diff = dv[i]; }
      for (i = 0; i < 4; i++) if (prac_diff == dv[i]) gfx_text(c.x + 10, c.y + 46, dd[i], C_MAROON); }
    for (i = 0; i < N_MODULE; i++) {
        Rect r = R(c.x + 10, c.y + 60 + i * 50, c.w - 20, 46);
        int lv = prac_xp[i] >= 20 ? 3 : prac_xp[i] >= 10 ? 2 : prac_xp[i] >= 4 ? 1 : 0;
        gfx_rect(r.x, r.y, r.w, r.h, C_WHITE); ui_sunken(r);
        gfx_spr_scaled(r.x + 6, r.y + 6, &S_PM[1], 2);
        gfx_text_b(r.x + 46, r.y + 6, g_modname[i], C_NAVY);
        gfx_text(r.x + 46, r.y + 20, g_modblurb[i], C_BLACK);
        snprintf(b, sizeof b, "Cert: %s   (%d correct)", lv ? cert_name(i, lv) : "none yet", prac_xp[i]); gfx_text(r.x + 46, r.y + 32, b, C_GRAY);
        if (ui_button(R(r.x + r.w - 110, r.y + 10, 100, 26), "Practice", 1)) quiz_open(QM_PRACTICE, i);
    }
    snprintf(b, sizeof b, "Lifetime: %d right, %d wrong. Streak %d (best %d)", prac_right, prac_wrong, prac_streak, g_cfg.pbest); gfx_text(c.x + 10, c.y + 322, b, C_BLACK);
    gfx_text(c.x + 10, c.y + 336, "In the real game, training grants Actions, cash and Synergy.", C_GRAY);
    if (ui_button_def(R(c.x + c.w - 130, c.y + c.h - 38, 120, 28), "Close", 1) || ui_key(KEY_ESC)) g_scr = SC_DESKTOP;
}

/* ================================================================== save support */
typedef struct {
    int rv[10], rv_offer[3], ev, ev_state, ev_ok, ev_choice, haggled;
    char msg[120]; Rng rng; Shop shop;
} Flow;
int flow_pack(void *buf, int max) {
    Flow f;
    if ((int)sizeof f > max) return 0;
    memset(&f, 0, sizeof f);
    f.rv[0] = rv_s; f.rv[1] = rv_c; f.rv[2] = rv_h; f.rv[3] = rv_grade; f.rv[4] = rv_bonus_c; f.rv[5] = rv_bonus_h; f.rv[6] = rv_sel; f.rv[7] = rv_done_flag;
    memcpy(f.rv_offer, rv_offer, sizeof rv_offer);
    f.ev = ev; f.ev_state = ev_state; f.ev_ok = ev_ok; f.ev_choice = ev_choice; f.haggled = sh_haggled;
    memcpy(f.msg, sh_msg, sizeof f.msg); f.rng = sh_rng; f.shop = g_shop;
    memcpy(buf, &f, sizeof f);
    return (int)sizeof f;
}
int flow_unpack(const void *buf, int n) {
    Flow f;
    int i;
    if (n != (int)sizeof f) return 0;
    memcpy(&f, buf, sizeof f);
    if (f.ev < 0 || f.ev >= g_nevent || f.ev_choice < 0 || f.ev_choice >= g_event[f.ev].nopt) return 0;
    rv_s = f.rv[0]; rv_c = f.rv[1]; rv_h = f.rv[2]; rv_grade = f.rv[3]; rv_bonus_c = f.rv[4]; rv_bonus_h = f.rv[5]; rv_sel = f.rv[6]; rv_done_flag = f.rv[7];
    memcpy(rv_offer, f.rv_offer, sizeof rv_offer);
    for (i = 0; i < 3; i++) if (rv_offer[i] < -1 || rv_offer[i] >= N_PERK) rv_offer[i] = -1;
    ev = f.ev; ev_state = f.ev_state ? 1 : 0; ev_ok = f.ev_ok; ev_choice = f.ev_choice; sh_haggled = f.haggled;
    ev_text = ev_state ? (ev_ok ? g_event[ev].opt[ev_choice].good : g_event[ev].opt[ev_choice].bad) : "";
    memcpy(sh_msg, f.msg, sizeof sh_msg); sh_msg[sizeof sh_msg - 1] = 0; sh_rng = f.rng; g_shop = f.shop;
    return 1;
}
