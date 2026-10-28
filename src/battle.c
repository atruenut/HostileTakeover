/* Run state, battle rules, perks, items, shop, events, config. */
#include "ht.h"

Config g_cfg;

/* ------------------------------------------------------------------ run */
int run_max_morale(const Run *r) { return r->maxmorale + (HAS(r, P_CHAIR) ? 20 : 0); }

void run_new(Run *r, const char *name, unsigned int seed, int career) {
    memset(r, 0, sizeof *r);
    snprintf(r->name, sizeof r->name, "%s", name && *name ? name : "Intern #4471");
    r->seed = seed; r->career = career;
    rng_seed(&r->rng, seed);
    r->maxmorale = 100; r->morale = 100; r->cash = 40; r->maxinv = 4;
    r->fleet[0] = A_FERN; r->fleet[1] = A_COOLER; r->fleet[2] = A_PRINTER; r->fleet[3] = A_CUBICLE; r->fleet[4] = A_SHREDDER;
    r->nfleet = 5;
    switch (career) {
    case CAR_INTERN:     run_item_add(r, I_STAPLER); run_item_add(r, I_STICKY); run_item_add(r, I_MUG); break;
    case CAR_ACCOUNTANT: run_item_add(r, I_CALC); run_item_add(r, I_HIGHLIGHT); break;
    case CAR_IT:         run_item_add(r, I_SCANNER); run_item_add(r, I_SCANNER); break;
    case CAR_SALES:      run_item_add(r, I_FAX); r->cash += 100; break;
    case CAR_JANITOR:    run_item_add(r, I_WHITEOUT); run_item_add(r, I_STICKY); break;
    }
}

int run_item_count(const Run *r) {
    int i, n = 0;
    for (i = 0; i < MAXINV; i++) if (r->inv[i] > 0) n++;
    return n;
}
static int inv_cap(const Run *r) { return MIN(MAXINV, r->maxinv + (HAS(r, P_DUALMON) ? 2 : 0)); }
int run_item_add(Run *r, int item) {
    int i, cap = inv_cap(r);
    for (i = 0; i < cap; i++) if (r->inv[i] == 0) { r->inv[i] = item + 1; return 1; }
    return 0;
}
void run_heal(Run *r, int amt) { r->morale = MIN(run_max_morale(r), r->morale + amt); }
void run_hurt(Run *r, int amt) { r->morale -= amt; if (r->morale < 0) r->morale = 0; }
void run_synergy(Run *r, int amt) { r->synergy = CLAMP(r->synergy + amt, 0, 100); }

int floor_label(int idx) { return g_floors[CLAMP(idx, 0, N_FLOORS - 1)].label; }

/* ------------------------------------------------------------------ helpers */
int bt_alive_perk(const Board *bd, int def) {
    int i;
    for (i = 0; i < bd->na; i++) if (bd->a[i].def == def && bd->a[i].placed && !bd->a[i].sunk) return 1;
    return 0;
}

static void bt_syn(Run *r, int amt) {
    run_synergy(r, amt);
}

static int bt_synp(Battle *b, Run *r, int amt) {
    if (bt_alive_perk(&b->you, A_DESK)) amt = amt * 3 / 2;
    bt_syn(r, amt);
    return amt;
}

/* ------------------------------------------------------------------ battle setup */
static int place_ok_unknown(const Board *b, int def, int orient, int x, int y) {
    int dx[6], dy[6], i;
    asset_shape(def, orient, dx, dy);
    for (i = 0; i < g_adef[def].n; i++) {
        int cx = x + dx[i], cy = y + dy[i];
        if (cx < 0 || cy < 0 || cx >= b->w || cy >= b->h) return 0;
        if (b->c[cy][cx].asset >= 0 || b->c[cy][cx].state != CS_UNKNOWN || b->c[cy][cx].lock) return 0;
    }
    return 1;
}

static void foe_gen(Battle *bt, Run *r) {
    Board *f = &bt->foe;
    const FloorDef *fd = &bt->fd;
    int target = fd->size * fd->size * fd->cells_pct / 100, cells = 0, i, tw = 0;
    int tries;
    (void)r;
    for (tries = 0; tries < 40; tries++) {
        int ok = 1;
        board_init(f, fd->size, fd->size);
        cells = 0;
        if (fd->boss) { int ai = board_add(f, A_BOARDROOM, 1); (void)ai; cells += g_adef[A_BOARDROOM].n; }
        for (i = 0; i < N_ASSET; i++) tw += fd->w[i];
        while (cells < target && f->na < 9) {
            int roll = rng_n(&bt->rng, tw), k = 0, def = A_FERN;
            for (k = 0; k < N_ASSET; k++) { roll -= fd->w[k]; if (roll < 0) { def = k; break; } }
            board_add(f, def, 1); cells += g_adef[def].n;
        }
        tw = 0;
        if (board_autoplace(f, &bt->rng)) break;
        ok = 0; (void)ok;
    }
    if (!board_all_sunk(f) && f->na == 0) { board_add(f, A_PRINTER, 1); board_autoplace(f, &bt->rng); }
    for (i = 0; i < f->na; i++) {
        char nm[12];
        gen_name(&bt->rng, nm, sizeof nm);
        snprintf(bt->foe_names[i], sizeof bt->foe_names[i], "%s's", nm);
    }
}

static void foe_hazards(Battle *bt, Run *r) {
    Board *f = &bt->foe;
    const FloorDef *fd = &bt->fd;
    int n = f->w * f->h / 14, tw = 0, i, placed = 0, guard = 0;
    for (i = 0; i < 5; i++) tw += fd->haz[i];
    if (fd->haz[0] >= 4) n += 3;
    while (placed < n && guard++ < 500 && tw > 0) {
        int x = rng_n(&bt->rng, f->w), y = rng_n(&bt->rng, f->h), roll = rng_n(&bt->rng, tw), k, h = HZ_WET;
        Cell *c = &f->c[y][x];
        if (c->asset >= 0 || c->haz) continue;
        for (k = 0; k < 5; k++) { roll -= fd->haz[k]; if (roll < 0) { h = k + 1; break; } }
        c->haz = (unsigned char)h;
        if (r->career == CAR_JANITOR) c->known |= K_HAZ;
        placed++;
    }
}

void battle_new(Battle *bt, Run *r) {
    int i, idx = CLAMP(r->floor, 0, N_FLOORS - 1);
    memset(bt, 0, sizeof *bt);
    bt->fd = g_floors[idx];
    if (r->overtime > 0) {
        bt->fd.skill = 5; bt->fd.dmg += r->overtime; bt->fd.cells_pct = MIN(30, bt->fd.cells_pct + r->overtime * 2);
        bt->fd.rules |= RULE_REORG | RULE_DOUBLE | RULE_FIREWALL;
        if (!bt->fd.reorg_n) bt->fd.reorg_n = 5;
        if (!bt->fd.double_n) bt->fd.double_n = 4;
    }
    bt->floor_idx = idx;
    rng_seed(&bt->rng, r->seed * 7919u + (unsigned)(r->floor + 1) * 104729u + (unsigned)r->overtime * 1299709u + (unsigned)(r->turns + r->floors_cleared * 131 + r->correct * 7 + r->wrong * 3));
    { int ys = idx < 6 ? 9 : 10;   /* your office is smaller & denser than their floor */
      board_init(&bt->you, ys, ys); }
    for (i = 0; i < r->nfleet; i++) board_add(&bt->you, r->fleet[i], 0);
    foe_gen(bt, r);
    if (bt->fd.rules & RULE_FIREWALL) {
        Board *f = &bt->foe;
        for (i = 0; i < f->na; i++) {
            int j, any = 0;
            for (j = 0; j < f->a[i].n; j++) if (rng_pct(&bt->rng, 33)) { f->a[i].shield[j] = 1; any = 1; }
            if (!any && f->a[i].n > 1) f->a[i].shield[rng_n(&bt->rng, f->a[i].n)] = 1;
        }
    }
    foe_hazards(bt, r);
    if (bt->fd.rules & RULE_INJUNCTION) {
        int n = bt->fd.inj_n, g = 0;
        while (n > 0 && g++ < 400) {
            int x = rng_n(&bt->rng, bt->foe.w), y = rng_n(&bt->rng, bt->foe.h);
            if (!bt->foe.c[y][x].lock) { bt->foe.c[y][x].lock = 1; n--; }
        }
    }
    bt->ai.type = bt->fd.ai; bt->ai.skill = bt->fd.skill;
    bt->carpet = rng_n(&bt->rng, 4); bt->carpet2 = rng_n(&bt->rng, 4); bt->carpet_pat = rng_n(&bt->rng, 4);
    if (bt->fd.boss) snprintf(bt->boss_name, sizeof bt->boss_name, "%s", bt->fd.boss);
    else bt->boss_name[0] = 0;
    bt->phase = PH_PLACE;
    board_autoplace(&bt->you, &bt->rng);
}

static void start_player_turn(Battle *b, Run *r);

void battle_begin(Battle *b, Run *r) {
    int i;
    for (i = 0; i < b->you.na; i++) if (!b->you.a[i].placed) board_random_place(&b->you, i, &b->rng);
    b->turn = 0; b->phase = PH_PLAYER; b->bonus_ap_next = 0;
    r->calc = r->calc; r->parachute_used = r->parachute_used;
    b->macro_used = 0; b->backup_used = 0; b->shred_used = 0;
    start_player_turn(b, r);
}

/* ------------------------------------------------------------------ shooting */
static void log_asset(Battle *b, Run *r, const char *verb, int ai) {
    (void)r;
    fx_log(C_YELLOW, "%s %s %s.", verb, b->foe_names[ai], g_adef[b->foe.a[ai].def].name);
}

static int player_shot(Battle *b, Run *r, int x, int y, int depth) {
    Board *f = &b->foe;
    Cell *c;
    ShotRes sr;
    int res;
    if (x < 0 || y < 0 || x >= f->w || y >= f->h) return SH_OOB;
    c = &f->c[y][x];
    if (c->state == CS_UNKNOWN && !c->lock && c->asset < 0 && c->haz == HZ_WET && depth == 0 &&
        r->career != CAR_JANITOR && !HAS(r, P_EXTING)) {
        static const int ddx[8] = {-1,0,1,-1,1,-1,0,1}, ddy[8] = {-1,-1,-1,0,0,1,1,1};
        int cand[8], n = 0, k;
        for (k = 0; k < 8; k++) {
            int nx = x + ddx[k], ny = y + ddy[k];
            if (nx >= 0 && ny >= 0 && nx < f->w && ny < f->h && f->c[ny][nx].state == CS_UNKNOWN && !f->c[ny][nx].lock) cand[n++] = k;
        }
        c->haz = HZ_NONE; c->state = CS_MISS; c->known |= K_HAZ;
        fx_float(1, x, y, C_YELLOW, "WET!");
        fx_log(C_YELLOW, "Wet floor! Your memo slid away...");
        if (n > 0) {
            k = cand[rng_n(&b->rng, n)];
            return player_shot(b, r, x + ddx[k], y + ddy[k], 1);
        }
        b->last_x = x; b->last_y = y;
        return SH_MISS;
    }
    res = board_shoot(f, x, y, &sr);
    if (res == SH_ALREADY || res == SH_OOB) return res;
    b->last_x = x; b->last_y = y;
    b->shots++; r->shots++;
    switch (res) {
    case SH_LOCKED:
        fx_float(1, x, y, C_RED, "RED TAPE");
        fx_log(C_RED, "Injunction! Red tape cut. (shot wasted)");
        break;
    case SH_SHIELD:
        fx_float(1, x, y, C_AQUA, "FIREWALL");
        fx_log(C_AQUA, "Firewall absorbed the shot - it's down now.");
        break;
    case SH_HIT: case SH_SUNK: {
        int gain = (HAS(r, P_POSTER) ? 2 : 0);
        b->hits++; r->hits++; r->score += 10;
        bt_synp(b, r, res == SH_SUNK ? 25 : 10);
        if (gain) run_heal(r, gain);
        if (res == SH_HIT) { fx_float(1, x, y, C_RED, "HIT!"); fx_log(C_LIME, "Direct hit on %s %s!", b->foe_names[sr.asset], g_adef[f->a[sr.asset].def].name); }
        else { r->sunk++; r->score += 50; fx_float(1, x, y, C_YELLOW, "REPOSSESSED!"); log_asset(b, r, "REPOSSESSED", sr.asset); }
        break; }
    case SH_MISS: {
        int h = c->haz;
        fx_float(1, x, y, C_SILVER, "miss");
        if (h) {
            c->known &= (unsigned char)~K_HAZ; c->haz = HZ_NONE;
            if (h == HZ_DONUT) { run_heal(r, 8); fx_float(1, x, y, C_LIME, "+8 MORALE"); fx_log(C_LIME, "Free donuts! +8 Morale."); }
            else if (h == HZ_COFFEE) { b->ap++; fx_float(1, x, y, C_AQUA, "+1 ACTION"); fx_log(C_AQUA, "Coffee machine! +1 Action."); }
            else if (h == HZ_CLOSET) {
                if (rng_pct(&b->rng, 55)) { int it = rng_n(&b->rng, N_ITEM); if (run_item_add(r, it)) fx_log(C_YELLOW, "Supply closet: found a %s!", g_item[it].name); else { r->cash += 20; fx_log(C_YELLOW, "Supply closet: bag full, sold it for $20."); } }
                else { int c2 = 20 + rng_n(&b->rng, 30); r->cash += c2; fx_log(C_YELLOW, "Supply closet: petty cash $%d!", c2); }
                fx_float(1, x, y, C_YELLOW, "LOOT!");
            }
            else if (h == HZ_REPLYALL) { b->pend_quiz = 1; fx_float(1, x, y, C_RED, "REPLY-ALL!"); fx_log(C_RED, "REPLY-ALL TRAP! Answer fast or take the heat."); }
        }
        break; }
    }
    fx_boom(1, x, y, res);
    return res;
}

int bt_fire(Battle *b, Run *r, int x, int y) {
    int res;
    if (b->phase != PH_PLAYER || b->ap < 1 || b->pend_quiz) return -1;
    res = player_shot(b, r, x, y, 0);
    if (res == SH_ALREADY || res == SH_OOB) return res;
    b->ap--;
    if (res == SH_MISS && HAS(r, P_MOUSE) && !b->refund_used) { b->refund_used = 1; b->ap++; fx_log(C_AQUA, "Gaming Mouse: Action refunded."); }
    fx_banner(NULL);
    return res;
}

/* ------------------------------------------------------------------ items */
static int count_row_col(const Board *f, int x, int y, int *row, int *col) {
    int i; *row = 0; *col = 0;
    for (i = 0; i < f->w; i++) if (f->c[y][i].asset >= 0 && f->c[y][i].state == CS_UNKNOWN) (*row)++;
    for (i = 0; i < f->h; i++) if (f->c[i][x].asset >= 0 && f->c[i][x].state == CS_UNKNOWN) (*col)++;
    return 1;
}

static int shots_at(Battle *b, Run *r, const int *xs, const int *ys, int n) {
    int i, any = 0;
    for (i = 0; i < n; i++) { int res = player_shot(b, r, xs[i], ys[i], 1); if (res != SH_ALREADY && res != SH_OOB) any = 1; }
    return any;
}

int bt_use_item(Battle *b, Run *r, int slot, int x, int y) {
    int it, xs[9], ys[9], n = 0, i, j;
    Board *f = &b->foe;
    if (slot < 0 || slot >= MAXINV || r->inv[slot] <= 0 || b->phase != PH_PLAYER || b->pend_quiz) return 0;
    it = r->inv[slot] - 1;
    if (g_item[it].ap > b->ap) return 0;
    if (g_item[it].target && (x < 0 || y < 0 || x >= f->w || y >= f->h)) return 0;
    switch (it) {
    case I_STAPLER:
        xs[n] = x; ys[n++] = y; xs[n] = x+1; ys[n++] = y; xs[n] = x-1; ys[n++] = y; xs[n] = x; ys[n++] = y+1; xs[n] = x; ys[n++] = y-1;
        if (!shots_at(b, r, xs, ys, n)) { return 0; }
        break;
    case I_FAX:
        xs[0] = x-1; xs[1] = x; xs[2] = x+1; ys[0] = ys[1] = ys[2] = y; n = 3;
        if (!shots_at(b, r, xs, ys, n)) { return 0; }
        break;
    case I_ALLHANDS:
        for (j = -1; j <= 1; j++) for (i = -1; i <= 1; i++) { xs[n] = x + i; ys[n++] = y + j; }
        if (!shots_at(b, r, xs, ys, n)) { return 0; }
        break;
    case I_HIGHLIGHT: { int row, col; count_row_col(f, x, y, &row, &col); f->rowhint[y] = (signed char)row; f->colhint[x] = (signed char)col;
        fx_log(C_YELLOW, "Highlighter: row %d has %d, column %d has %d.", y + 1, row, x + 1, col); break; }
    case I_STICKY: {
        Cell *c = &f->c[y][x];
        if (c->state != CS_UNKNOWN) return 0;
        c->known |= K_STICKY;
        fx_log(C_YELLOW, c->asset >= 0 ? "Sticky note: there's an Asset at (%d,%d)!" : "Sticky note: (%d,%d) is empty.", x + 1, y + 1);
        break; }
    case I_SCANNER: {
        int best = 99;
        for (j = 0; j < f->h; j++) for (i = 0; i < f->w; i++)
            if (f->c[j][i].asset >= 0 && f->c[j][i].state == CS_UNKNOWN) { int d = abs(i - x) + abs(j - y); if (d < best) best = d; }
        f->c[y][x].scan = (signed char)(best == 99 ? 0 : best);
        fx_log(C_AQUA, "Badge scan: nearest Asset is %d cell%s away.", best == 99 ? 0 : best, best == 1 ? "" : "s");
        break; }
    case I_WHITEOUT: run_heal(r, 25); fx_log(C_LIME, "Whiteout: +25 Morale."); break;
    case I_ENERGY: b->ap += 2; run_hurt(r, 5); fx_log(C_AQUA, "Energy drink: +2 Actions, -5 Morale."); break;
    case I_JAM: b->enemy_skip = 1; fx_log(C_YELLOW, "Printer Jam: the enemy will skip their next turn."); break;
    case I_CALC: r->calc = 1; fx_log(C_YELLOW, "Calculator ready: next training is auto-correct."); break;
    case I_MUG: b->ap++; fx_log(C_AQUA, "Coffee: +1 Action."); break;
    case I_AIRPLANE: {
        int cx[144], cy[144], m = 0, a2;
        for (j = 0; j < f->h; j++) for (i = 0; i < f->w; i++)
            if (f->c[j][i].asset >= 0 && f->c[j][i].state == CS_UNKNOWN && !f->a[f->c[j][i].asset].sunk) { cx[m] = i; cy[m] = j; m++; }
        if (m == 0 || rng_pct(&b->rng, 35)) {
            m = 0;
            for (j = 0; j < f->h; j++) for (i = 0; i < f->w; i++) if (f->c[j][i].state == CS_UNKNOWN) { cx[m] = i; cy[m] = j; m++; }
        }
        if (m == 0) return 0;
        a2 = rng_n(&b->rng, m);
        fx_log(C_WHITE, "Paper airplane launched!");
        player_shot(b, r, cx[a2], cy[a2], 1);
        break; }
    default: return 0;
    }
    b->ap -= g_item[it].ap;
    r->inv[slot] = 0;
    return 1;
}

int bt_synergize(Battle *b, Run *r) {
    Board *f = &b->foe;
    int k, i, j;
    if (r->synergy < 100 || b->phase != PH_PLAYER || b->pend_quiz) return 0;
    r->synergy = 0;
    k = rng_n(&b->rng, 6);
    switch (k) {
    case 0: {
        int n = 5, guard = 0;
        fx_banner("PIVOT TO VIDEO!");
        while (n > 0 && guard++ < 600) { int x = rng_n(&b->rng, f->w), y = rng_n(&b->rng, f->h);
            if (f->c[y][x].state == CS_UNKNOWN && !f->c[y][x].lock) { player_shot(b, r, x, y, 1); n--; } }
        break; }
    case 1: {
        int idx[MAXA], m = 0;
        fx_banner("DISRUPT THE PARADIGM!");
        for (i = 0; i < f->na; i++) if (f->a[i].placed && !f->a[i].sunk) idx[m++] = i;
        if (m) { Asset *a = &f->a[idx[rng_n(&b->rng, m)]]; for (j = 0; j < a->n; j++) f->c[a->y[j]][a->x[j]].known |= K_STICKY;
            fx_log(C_YELLOW, "Disrupted: an entire %s is now visible!", g_adef[a->def].name); }
        break; }
    case 2: {
        int rows[MAXG], m = 0, row;
        fx_banner("LEVERAGE!");
        for (j = 0; j < f->h; j++) { int has = 0; for (i = 0; i < f->w; i++) if (f->c[j][i].asset >= 0 && f->c[j][i].state == CS_UNKNOWN) has = 1; if (has) rows[m++] = j; }
        row = m ? rows[rng_n(&b->rng, m)] : rng_n(&b->rng, f->h);
        for (i = 0; i < f->w; i++) if (f->c[row][i].state == CS_UNKNOWN) player_shot(b, r, i, row, 1);
        break; }
    case 3: fx_banner("BANDWIDTH!"); b->ap += 3; fx_log(C_AQUA, "+3 Actions."); break;
    case 4: fx_banner("TEAM BUILDING!"); run_heal(r, 30); b->ap += 1; fx_log(C_LIME, "+30 Morale, +1 Action."); break;
    default: {
        int cx[144], cy[144], m = 0, a2;
        fx_banner("THOUGHT SHOWER!");
        for (j = 0; j < f->h; j++) for (i = 0; i < f->w; i++) if (f->c[j][i].asset >= 0 && f->c[j][i].state == CS_UNKNOWN) { cx[m] = i; cy[m] = j; m++; }
        a2 = m ? rng_n(&b->rng, m) : 0;
        { int ox = m ? cx[a2] : f->w / 2, oy = m ? cy[a2] : f->h / 2;
          for (j = -1; j <= 1; j++) for (i = -1; i <= 1; i++) player_shot(b, r, ox + i, oy + j, 1); }
        break; }
    }
    return 1;
}

/* ------------------------------------------------------------------ turns */
static void start_player_turn(Battle *b, Run *r) {
    b->turn++;
    b->phase = PH_PLAYER;
    b->ap = 1 + b->bonus_ap_next; b->bonus_ap_next = 0;
    b->refund_used = 0; b->trained = 0;
    if (b->turn == 1 && HAS(r, P_STANDING)) b->ap++;
    if (HAS(r, P_LUCKYMUG) && rng_pct(&b->rng, 15)) { b->ap++; fx_log(C_AQUA, "Lucky Mug: +1 Action!"); }
    if (bt_alive_perk(&b->you, A_FERN)) run_heal(r, 1);
    if (bt_alive_perk(&b->you, A_COOLER)) run_synergy(r, 5);
    if (bt_alive_perk(&b->you, A_VENDING)) r->cash += 5;
    if (bt_alive_perk(&b->you, A_MAINFRAME) && b->turn % 4 == 0) { b->ap++; fx_log(C_AQUA, "Mainframe overclock: +1 Action."); }
    if (HAS(r, P_INTERN) && b->turn % 3 == 0) {
        int tries = 0;
        while (tries++ < 300) { int x = rng_n(&b->rng, b->foe.w), y = rng_n(&b->rng, b->foe.h);
            if (b->foe.c[y][x].state == CS_UNKNOWN && !b->foe.c[y][x].lock) { fx_log(C_WHITE, "The Intern fires a free shot!"); player_shot(b, r, x, y, 1); break; } }
    }
    r->turns++;
}

int bt_next_enemy_shots(Battle *b) {
    int n = 1;
    if (b->enemy_skip) return 0;
    if ((b->fd.rules & RULE_DOUBLE) && b->fd.double_n > 0 && (b->enemy_turns + 1) % b->fd.double_n == 0) n++;
    return n;
}

void bt_end_turn(Battle *b, Run *r) {
    (void)r;
    if (b->phase != PH_PLAYER) return;
    b->enemy_shots = bt_next_enemy_shots(b);
    if (b->enemy_skip) { fx_log(C_YELLOW, "The enemy is busy with a printer jam. Turn skipped!"); b->enemy_skip = 0; }
    else if (b->enemy_shots > 1) fx_log(C_RED, "Enemy is working overtime: %d shots this turn!", b->enemy_shots);
    b->phase = PH_ENEMY;
}

int bt_enemy_resolve(Battle *b, Run *r, int x, int y) {
    Board *p = &b->you;
    ShotRes sr;
    int res, dmg;
    res = board_shoot(p, x, y, &sr);
    b->last_x = x; b->last_y = y;
    if (res != SH_HIT && res != SH_SUNK) {
        b->audit_combo = 0;
        if (res == SH_MISS) fx_float(0, x, y, C_SILVER, "miss");
        fx_boom(0, x, y, res);
        return 0;
    }
    dmg = b->fd.dmg;
    if (b->fd.rules & RULE_AUDIT) { dmg += b->audit_combo; b->audit_combo++; }
    /* cloud backup: undo a sinking */
    if (res == SH_SUNK && !b->backup_used && bt_alive_perk(p, A_RACK) && p->a[sr.asset].def != A_RACK) {
        Asset *a = &p->a[sr.asset];
        int i;
        a->sunk = 0; a->nhit--; a->hit[sr.part] = 0;
        for (i = 0; i < a->n; i++) p->c[a->y[i]][a->x[i]].state = a->hit[i] ? CS_HIT : CS_UNKNOWN;
        b->backup_used = 1; b->audit_combo = 0;
        fx_float(0, x, y, C_AQUA, "BACKED UP!");
        fx_log(C_AQUA, "Cloud Backup restored your %s!", g_adef[a->def].name);
        fx_boom(0, x, y, SH_MISS);
        return 0;
    }
    if (res == SH_SUNK) { dmg += b->fd.dmg; b->sunk_you++; }
    if (bt_alive_perk(p, A_CUBICLE) && p->a[sr.asset].def != A_CUBICLE) dmg--;
    if (HAS(r, P_STRESSBALL)) dmg--;
    if (dmg < 1) dmg = 1;
    if (bt_alive_perk(p, A_SHREDDER) && !b->shred_used && p->a[sr.asset].def != A_SHREDDER) { b->shred_used = 1; dmg = 0; fx_float(0, x, y, C_WHITE, "SHREDDED!"); fx_log(C_WHITE, "Shredder ate the evidence. No damage!"); }
    else if (bt_alive_perk(p, A_PRINTER) && rng_pct(&b->rng, 25)) { dmg = 0; fx_float(0, x, y, C_SILVER, "PAPER JAM"); fx_log(C_SILVER, "Paper jam! The attack is jammed. No damage."); }
    if (dmg > 0) { char tt[12]; run_hurt(r, dmg); snprintf(tt, sizeof tt, "-%d", dmg); fx_float(0, x, y, C_RED, tt); }
    if (res == SH_SUNK) fx_log(C_RED, "Your %s was repossessed! (-%d Morale)", g_adef[p->a[sr.asset].def].name, dmg);
    else fx_log(C_MAROON, "Your %s took a hit. (-%d Morale)", g_adef[p->a[sr.asset].def].name, dmg);
    fx_boom(0, x, y, res);
    return 1;
}

static void reorg(Battle *b) {
    Board *f = &b->foe;
    int i, moved = 0;
    for (i = 0; i < f->na; i++) {
        Asset *a = &f->a[i];
        int ox, oy, oo, t, ok = 0;
        if (!a->placed || a->sunk || a->nhit > 0) continue;
        ox = a->x[0]; oy = a->y[0]; oo = a->orient;
        { int dx[6], dy[6], k, mnx = 99, mny = 99;
          asset_shape(a->def, oo, dx, dy);
          for (k = 0; k < a->n; k++) { if (a->x[k] - dx[k] < mnx) mnx = a->x[k] - dx[k]; if (a->y[k] - dy[k] < mny) mny = a->y[k] - dy[k]; }
          ox = mnx; oy = mny; }
        board_unplace(f, i);
        for (t = 0; t < 200 && !ok; t++) {
            int o = rng_n(&b->rng, 4), x = rng_n(&b->rng, f->w), y = rng_n(&b->rng, f->h);
            if (place_ok_unknown(f, a->def, o, x, y)) {
                int sh[6], k;
                for (k = 0; k < a->n; k++) sh[k] = a->shield[k];
                board_place(f, i, x, y, o);
                for (k = 0; k < a->n; k++) a->shield[k] = (unsigned char)sh[k];
                ok = 1;
            }
        }
        if (!ok) { int sh[6], k; for (k = 0; k < a->n; k++) sh[k] = a->shield[k]; board_place(f, i, ox, oy, oo); for (k = 0; k < a->n; k++) a->shield[k] = (unsigned char)sh[k]; }
        else moved++;
    }
    { int x, y; for (y = 0; y < f->h; y++) for (x = 0; x < f->w; x++) f->c[y][x].known &= (unsigned char)~K_STICKY; }
    if (moved) { b->banner_reorg = 1; fx_banner("REORGANIZATION!"); fx_log(C_FUCHSIA, "RE-ORG! %d department%s moved desks.", moved, moved == 1 ? "" : "s"); }
}

static void incident(Battle *b, Run *r) {
    Board *f = &b->foe;
    int k = rng_n(&b->rng, 4), x, y, g = 0;
    b->incident = 0;
    if (k == 0) {
        int n = 2;
        while (n > 0 && g++ < 300) { x = rng_n(&b->rng, f->w); y = rng_n(&b->rng, f->h);
            if (f->c[y][x].state == CS_UNKNOWN && f->c[y][x].asset < 0 && !f->c[y][x].haz) { f->c[y][x].haz = HZ_WET; n--; } }
        fx_log(C_YELLOW, "INCIDENT: someone spilled coffee. Wet floors have appeared!"); b->incident = 1;
    } else if (k == 1) {
        while (g++ < 300) { x = rng_n(&b->rng, f->w); y = rng_n(&b->rng, f->h);
            if (f->c[y][x].asset >= 0 && f->c[y][x].state == CS_UNKNOWN) { f->c[y][x].known |= K_STICKY; fx_log(C_YELLOW, "INCIDENT: surprise inspection reveals an Asset at (%d,%d)!", x + 1, y + 1); b->incident = 1; break; } }
    } else if (k == 2) { run_heal(r, 10); fx_log(C_LIME, "INCIDENT: free bagels in the break room! +10 Morale."); b->incident = 1; }
    else { b->enemy_skip = 1; fx_log(C_YELLOW, "INCIDENT: the enemy printer jams. They'll skip a turn!"); b->incident = 1; }
}

void bt_enemy_done(Battle *b, Run *r) {
    b->enemy_turns++;
    if ((b->fd.rules & RULE_REORG) && b->fd.reorg_n > 0 && b->enemy_turns % b->fd.reorg_n == 0) reorg(b);
    if (b->turn >= 2 && rng_pct(&b->rng, 10)) incident(b, r);
    if (bt_status(b, r) == 0) start_player_turn(b, r);
}

void bt_replyall_result(Battle *b, Run *r, int ok) {
    b->pend_quiz = 0;
    if (ok) { bt_synp(b, r, 10); fx_log(C_LIME, "Reply-all averted. Synergy +10."); }
    else { run_hurt(r, 12); fx_log(C_RED, "Reply-all sent to the entire company! -12 Morale."); }
}

int bt_status(Battle *b, Run *r) {
    if (b->phase == PH_WIN) return 1;
    if (board_all_sunk(&b->foe)) { b->phase = PH_WIN; return 1; }
    if (board_all_sunk(&b->you)) {
        run_hurt(r, 40);
        b->phase = PH_LOSE; b->cause = 1;
        if (r->morale <= 0 && !(HAS(r, P_PARACHUTE) && !r->parachute_used)) return -2;
        if (r->morale <= 0) { r->morale = 30; r->parachute_used = 1; fx_log(C_YELLOW, "Golden Parachute deployed!"); }
        return -1;
    }
    if (r->morale <= 0) {
        if (HAS(r, P_PARACHUTE) && !r->parachute_used) { r->parachute_used = 1; r->morale = 30; fx_banner("GOLDEN PARACHUTE!"); fx_log(C_YELLOW, "Golden Parachute deployed! Morale restored to 30."); return 0; }
        b->phase = PH_LOSE; b->cause = 2;
        return -2;
    }
    return 0;
}

void bt_floor_reward(Battle *b, Run *r, int *score, int *cash, int *heal) {
    int lab = floor_label(b->floor_idx) + r->overtime * 2;
    int par = 22 + b->fd.size, bonus;
    int s, c, h;
    bonus = MAX(0, par - b->turn) * 15;
    s = 400 + lab * 100 + bonus + r->morale * 2;
    c = 40 + lab * 8;
    h = 10;
    if (HAS(r, P_LANYARD)) c += 25;
    if (HAS(r, P_PLAQUE)) h += 10;
    if (bt_alive_perk(&b->you, A_TABLE)) { c += 10; h += 5; }
    if (HAS(r, P_CORPCARD)) c += 0;
    r->score += s; r->cash += c; run_heal(r, h);
    r->floors_cleared++;
    if (score) *score = s;
    if (cash) *cash = c;
    if (heal) *heal = h;
}

/* ------------------------------------------------------------------ training */
int run_cert_level(const Run *r, int module) {
    int xp = r->train_xp[module];
    return xp >= 20 ? 3 : xp >= 10 ? 2 : xp >= 4 ? 1 : 0;
}
const char *cert_name(int module, int level) {
    static const char *names[N_MODULE][4] = {
        {"-","Number Cruncher","Spreadsheet Sorcerer","Chief Arithmetic Officer"},
        {"-","Proofreader","Grammar Enforcer","Master of Memos"},
        {"-","Bookkeeper","Controller","Certified Ledger Lord"},
        {"-","Mail Courier","Regional Manager","Global Logistics Baron"},
        {"-","Hunt-and-Pecker","Touch Typist","Keyboard Samurai"}};
    return names[module][CLAMP(level, 0, 3)];
}
int run_quiz_diff(const Run *r, int module) {
    int d = 1 + r->floor / 3 + run_cert_level(r, module) / 2;
    return CLAMP(d, 1, 5);
}
int bt_can_train(const Battle *b) { return b->phase == PH_PLAYER && !b->trained && !b->pend_quiz; }

int run_free_pass(Battle *b, Run *r) {
    if (r->calc) { r->calc = 0; return 1; }
    if (HAS(r, P_MACRO) && b && !b->macro_used) { b->macro_used = 1; return 1; }
    return 0;
}

int run_train_result(Run *r, int module, int ok, int diff) {
    int cash, lost;
    if (ok) {
        int before = run_cert_level(r, module);
        r->correct++; r->streak++; if (r->streak > r->best_streak) r->best_streak = r->streak;
        r->train_xp[module]++;
        cash = 8 + diff * 4 + (r->streak >= 3 ? 5 : 0);
        if (r->career == CAR_ACCOUNTANT) cash = cash * 3 / 2;
        r->cash += cash; r->score += 25 + diff * 5;
        run_synergy(r, 15 + (HAS(r, P_KEYBOARD) ? 15 : 0));
        if (run_cert_level(r, module) > before) { r->certs |= 1u << module; fx_log(C_YELLOW, "CERTIFICATION EARNED: %s!", cert_name(module, run_cert_level(r, module))); }
        return cash;
    }
    r->wrong++; r->streak = 0;
    lost = 6 + diff * 2;
    if (HAS(r, P_HEADSET)) lost /= 2;
    run_hurt(r, lost);
    return -lost;
}

int bt_train_result(Battle *b, Run *r, int module, int ok, int diff) {
    int v = run_train_result(r, module, ok, diff);
    b->trained = 1;
    if (ok) { b->ap++; fx_log(C_LIME, "Training passed! +1 Action, +$%d.", v); }
    else fx_log(C_RED, "Training failed! %d Morale.", v);
    return v;
}

/* ------------------------------------------------------------------ perks, shop, events */
void perk_offer(Run *r, int out[3]) {
    int pool[N_PERK], n = 0, i, k;
    for (i = 0; i < N_PERK; i++) if (!HAS(r, i)) pool[n++] = i;
    for (k = 0; k < 3; k++) out[k] = -1;
    for (k = 0; k < 3 && n > 0; k++) {
        int j;
        for (;;) {
            j = rng_n(&r->rng, n);
            if (g_perk[pool[j]].rare && rng_pct(&r->rng, 55) && n > 3) continue;
            break;
        }
        out[k] = pool[j]; pool[j] = pool[--n];
    }
}

int item_price(const Run *r, int base) {
    int p = base * (100 - r->disc) / 100;
    if (HAS(r, P_CORPCARD)) p = p * 85 / 100;
    return MAX(1, p);
}

void shop_make(Shop *s, Run *r, Rng *g) {
    int k, tw = 0, i;
    memset(s, 0, sizeof *s);
    for (i = 0; i < N_ITEM; i++) tw += g_item[i].rarity;
    for (k = 0; k < 3; k++) {
        int roll = rng_n(g, tw), it = 0;
        for (i = 0; i < N_ITEM; i++) { roll -= g_item[i].rarity; if (roll < 0) { it = i; break; } }
        s->item[k] = it; s->iprice[k] = item_price(r, g_item[it].price);
    }
    s->asset = rng_n(g, A_BOARDROOM);
    s->aprice = item_price(r, g_adef[s->asset].price);
    { int o[3]; perk_offer(r, o); s->perk = o[0]; s->pprice = item_price(r, 110); if (o[0] < 0) s->psold = 1; }
    s->rerolls = 0;
}

int event_pick(Run *r) { return rng_n(&r->rng, g_nevent); }

const char *event_apply(Run *r, int ev, int opt, int *okp) {
    const EvOpt *o = &g_event[ev].opt[opt];
    int ok = rng_pct(&r->rng, o->pct);
    int fx = ok ? o->gfx : o->bfx, amt = ok ? o->gamt : o->bamt;
    *okp = ok;
    switch (fx) {
    case FX_MORALE: if (amt >= 0) run_heal(r, amt); else run_hurt(r, -amt); break;
    case FX_CASH: r->cash = MAX(0, r->cash + amt); break;
    case FX_ITEM: { int it = rng_n(&r->rng, N_ITEM); if (!run_item_add(r, it)) r->cash += 25; break; }
    case FX_PERK: { int o3[3]; perk_offer(r, o3); if (o3[0] >= 0) r->perks |= 1u << o3[0]; else r->cash += 50; break; }
    case FX_SYNERGY: run_synergy(r, amt); break;
    case FX_MAXMORALE: r->maxmorale += amt; run_heal(r, amt); break;
    }
    return ok ? o->good : o->bad;
}

/* ------------------------------------------------------------------ config / hall of fame */
void cfg_load(void) {
    char buf[4096];
    int n;
    char *p, *line;
    memset(&g_cfg, 0, sizeof g_cfg);
    g_cfg.sound = 1; g_cfg.music = 1; g_cfg.wallpaper = 0; g_cfg.pdiff = 2;
    snprintf(g_cfg.lastname, sizeof g_cfg.lastname, "Intern #4471");
    n = plat_load("TAKEOVER.INI", buf, (int)sizeof buf - 1);
    if (n <= 0) return;
    buf[n] = 0;
    for (line = strtok(buf, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        p = strchr(line, '=');
        if (!p) continue;
        *p++ = 0;
        if (!strcmp(line, "sound")) g_cfg.sound = atoi(p);
        else if (!strcmp(line, "music")) g_cfg.music = atoi(p);
        else if (!strcmp(line, "wallpaper")) g_cfg.wallpaper = atoi(p);
        else if (!strcmp(line, "pdiff")) g_cfg.pdiff = CLAMP(atoi(p), 1, 5);
        else if (!strcmp(line, "pxp")) sscanf(p, "%d,%d,%d,%d,%d", &g_cfg.pxp[0], &g_cfg.pxp[1], &g_cfg.pxp[2], &g_cfg.pxp[3], &g_cfg.pxp[4]);
        else if (!strcmp(line, "prac")) sscanf(p, "%d,%d,%d", &g_cfg.pright, &g_cfg.pwrong, &g_cfg.pbest);
        else if (!strcmp(line, "tut")) g_cfg.tut_seen = atoi(p);
        else if (!strcmp(line, "hs")) sscanf(p, "%d,%d,%d,%d,%d,%d,%d,%d", &g_cfg.hs[0], &g_cfg.hs[1], &g_cfg.hs[2], &g_cfg.hs[3], &g_cfg.hs[4], &g_cfg.hs[5], &g_cfg.hs[6], &g_cfg.hs[7]);
        else if (!strcmp(line, "name")) snprintf(g_cfg.lastname, sizeof g_cfg.lastname, "%s", p);
        else if (!strcmp(line, "hof") && g_cfg.n < HOF_N) {
            HofEntry *e = &g_cfg.e[g_cfg.n];
            char nm[40]; long sc; int fl, ca, wo; unsigned int sd;
            char *bar = strchr(p, '|');
            if (!bar) continue;
            *bar++ = 0;
            snprintf(nm, sizeof nm, "%s", p);
            if (sscanf(bar, "%ld|%d|%d|%d|%u", &sc, &fl, &ca, &wo, &sd) == 5) {
                snprintf(e->name, sizeof e->name, "%.23s", nm); e->score = sc; e->floor = fl; e->career = ca; e->won = wo; e->seed = sd;
                g_cfg.n++;
            }
        }
    }
}

void cfg_save(void) {
    char buf[4096];
    int len = 0, i;
    len += snprintf(buf + len, sizeof buf - (size_t)len, "sound=%d\nmusic=%d\nwallpaper=%d\nname=%s\n", g_cfg.sound, g_cfg.music, g_cfg.wallpaper, g_cfg.lastname);
    len += snprintf(buf + len, sizeof buf - (size_t)len, "pdiff=%d\npxp=%d,%d,%d,%d,%d\nprac=%d,%d,%d\ntut=%d\nhs=%d,%d,%d,%d,%d,%d,%d,%d\n", g_cfg.pdiff, g_cfg.pxp[0], g_cfg.pxp[1], g_cfg.pxp[2], g_cfg.pxp[3], g_cfg.pxp[4],
        g_cfg.pright, g_cfg.pwrong, g_cfg.pbest, g_cfg.tut_seen, g_cfg.hs[0], g_cfg.hs[1], g_cfg.hs[2], g_cfg.hs[3], g_cfg.hs[4], g_cfg.hs[5], g_cfg.hs[6], g_cfg.hs[7]);
    for (i = 0; i < g_cfg.n; i++) {
        HofEntry *e = &g_cfg.e[i];
        len += snprintf(buf + len, sizeof buf - (size_t)len, "hof=%s|%ld|%d|%d|%d|%u\n", e->name, e->score, e->floor, e->career, e->won, e->seed);
    }
    plat_save("TAKEOVER.INI", buf, len);
}

int hof_add(const char *name, long score, int floor, int career, int won, unsigned int seed) {
    int i, pos = g_cfg.n;
    HofEntry e;
    char *bar;
    memset(&e, 0, sizeof e);
    snprintf(e.name, sizeof e.name, "%s", name);
    while ((bar = strchr(e.name, '|'))) *bar = '/';
    e.score = score; e.floor = floor; e.career = career; e.won = won; e.seed = seed;
    for (i = 0; i < g_cfg.n; i++) if (score > g_cfg.e[i].score) { pos = i; break; }
    if (pos >= HOF_N) return -1;
    if (g_cfg.n < HOF_N) g_cfg.n++;
    for (i = g_cfg.n - 1; i > pos; i--) g_cfg.e[i] = g_cfg.e[i - 1];
    g_cfg.e[pos] = e;
    cfg_save();
    return pos;
}
