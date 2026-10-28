/* Boards, asset placement, shot resolution and the enemy AIs. */
#include "ht.h"

Rng g_rng;

void asset_shape(int def, int orient, int *dx, int *dy) {
    const AssetDef *d = &g_adef[def];
    int i, k, mnx = 99, mny = 99;
    for (i = 0; i < d->n; i++) { dx[i] = d->dx[i]; dy[i] = d->dy[i]; }
    for (k = 0; k < (orient & 3); k++)
        for (i = 0; i < d->n; i++) { int x = dx[i], y = dy[i]; dx[i] = -y; dy[i] = x; }
    for (i = 0; i < d->n; i++) { if (dx[i] < mnx) mnx = dx[i]; if (dy[i] < mny) mny = dy[i]; }
    for (i = 0; i < d->n; i++) { dx[i] -= mnx; dy[i] -= mny; }
}

void board_init(Board *b, int w, int h) {
    int x, y;
    memset(b, 0, sizeof *b);
    b->w = w; b->h = h;
    for (y = 0; y < MAXG; y++)
        for (x = 0; x < MAXG; x++) { b->c[y][x].asset = -1; b->c[y][x].part = -1; b->c[y][x].scan = -1; }
    for (x = 0; x < MAXG; x++) { b->rowhint[x] = -1; b->colhint[x] = -1; }
}

int board_add(Board *b, int def, int owner) {
    Asset *a;
    if (b->na >= MAXA) return -1;
    a = &b->a[b->na];
    memset(a, 0, sizeof *a);
    a->def = def; a->n = g_adef[def].n; a->owner = owner;
    return b->na++;
}

int board_can_place(const Board *b, int def, int orient, int x, int y, int ignore) {
    int dx[6], dy[6], i;
    asset_shape(def, orient, dx, dy);
    for (i = 0; i < g_adef[def].n; i++) {
        int cx = x + dx[i], cy = y + dy[i];
        if (cx < 0 || cy < 0 || cx >= b->w || cy >= b->h) return 0;
        if (b->c[cy][cx].asset >= 0 && b->c[cy][cx].asset != ignore) return 0;
    }
    return 1;
}

int board_place(Board *b, int ai, int x, int y, int orient) {
    Asset *a = &b->a[ai];
    int dx[6], dy[6], i;
    if (a->placed) board_unplace(b, ai);
    if (!board_can_place(b, a->def, orient, x, y, ai)) return 0;
    asset_shape(a->def, orient, dx, dy);
    for (i = 0; i < a->n; i++) {
        a->x[i] = (unsigned char)(x + dx[i]); a->y[i] = (unsigned char)(y + dy[i]);
        a->hit[i] = 0; a->shield[i] = 0;
        b->c[y + dy[i]][x + dx[i]].asset = (signed char)ai;
        b->c[y + dy[i]][x + dx[i]].part = (signed char)i;
    }
    a->orient = orient & 3; a->placed = 1; a->sunk = 0; a->nhit = 0;
    return 1;
}

void board_unplace(Board *b, int ai) {
    Asset *a = &b->a[ai];
    int i;
    if (!a->placed) return;
    for (i = 0; i < a->n; i++) {
        Cell *c = &b->c[a->y[i]][a->x[i]];
        c->asset = -1; c->part = -1;
    }
    a->placed = 0;
}

int board_random_place(Board *b, int ai, Rng *r) {
    int t;
    for (t = 0; t < 300; t++) {
        int o = rng_n(r, 4), x = rng_n(r, b->w), y = rng_n(r, b->h);
        if (board_can_place(b, b->a[ai].def, o, x, y, -1)) return board_place(b, ai, x, y, o);
    }
    return 0;
}

int board_autoplace(Board *b, Rng *r) {
    int tries, i, j, order[MAXA];
    for (tries = 0; tries < 80; tries++) {
        int ok = 1;
        for (i = 0; i < b->na; i++) board_unplace(b, i);
        for (i = 0; i < b->na; i++) order[i] = i;
        /* biggest first (stable enough) */
        for (i = 0; i < b->na; i++)
            for (j = i + 1; j < b->na; j++)
                if (b->a[order[j]].n > b->a[order[i]].n) { int t = order[i]; order[i] = order[j]; order[j] = t; }
        for (i = 0; i < b->na && ok; i++) if (!board_random_place(b, order[i], r)) ok = 0;
        if (ok) return 1;
    }
    return 0;
}

int board_all_sunk(const Board *b) {
    int i;
    if (b->na == 0) return 0;
    for (i = 0; i < b->na; i++) if (b->a[i].placed && !b->a[i].sunk) return 0;
    return 1;
}
int board_alive(const Board *b) {
    int i, n = 0;
    for (i = 0; i < b->na; i++) if (b->a[i].placed && !b->a[i].sunk) n++;
    return n;
}
int board_cells_alive(const Board *b) {
    int i, j, n = 0;
    for (i = 0; i < b->na; i++)
        if (b->a[i].placed) for (j = 0; j < b->a[i].n; j++) if (!b->a[i].hit[j]) n++;
    return n;
}

int board_shoot(Board *b, int x, int y, ShotRes *sr) {
    Cell *c;
    ShotRes tmp;
    if (!sr) sr = &tmp;
    memset(sr, 0, sizeof *sr);
    sr->asset = -1; sr->part = -1; sr->rx = x; sr->ry = y;
    if (x < 0 || y < 0 || x >= b->w || y >= b->h) { sr->res = SH_OOB; return SH_OOB; }
    c = &b->c[y][x];
    if (c->lock) { c->lock = 0; sr->res = SH_LOCKED; return SH_LOCKED; }
    if (c->state != CS_UNKNOWN) { sr->res = SH_ALREADY; return SH_ALREADY; }
    if (c->asset >= 0) {
        Asset *a = &b->a[c->asset];
        int p = c->part, i;
        sr->asset = c->asset; sr->part = p;
        if (a->shield[p]) { a->shield[p] = 0; c->known |= K_SHIELD; sr->res = SH_SHIELD; return SH_SHIELD; }
        c->state = CS_HIT; a->hit[p] = 1; a->nhit++;
        if (a->nhit >= a->n) {
            a->sunk = 1;
            for (i = 0; i < a->n; i++) b->c[a->y[i]][a->x[i]].state = CS_SUNK;
            sr->res = SH_SUNK; return SH_SUNK;
        }
        sr->res = SH_HIT; return SH_HIT;
    }
    c->state = CS_MISS;
    sr->res = SH_MISS; return SH_MISS;
}

/* ------------------------------------------------------------------ AI */
static int unk(const Board *b, int x, int y) {
    return x >= 0 && y >= 0 && x < b->w && y < b->h && b->c[y][x].state == CS_UNKNOWN;
}

static void pick_random_unknown(const Board *b, Rng *r, int parity, int *ox, int *oy) {
    int cnt = 0, x, y, k;
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++)
        if (unk(b, x, y) && (parity < 0 || ((x + y) & 1) == parity)) cnt++;
    if (cnt == 0 && parity >= 0) { pick_random_unknown(b, r, -1, ox, oy); return; }
    if (cnt == 0) { *ox = *oy = 0; return; }
    k = rng_n(r, cnt);
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++)
        if (unk(b, x, y) && (parity < 0 || ((x + y) & 1) == parity)) { if (k-- == 0) { *ox = x; *oy = y; return; } }
}

/* targeting: neighbours of unresolved hits; prefer extending a line of two */
static int pick_target(const Board *b, Rng *r, int *ox, int *oy) {
    static const int dx4[4] = {1, -1, 0, 0}, dy4[4] = {0, 0, 1, -1};
    int best = 0, bc = 0, x, y, d;
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++) {
        if (b->c[y][x].state != CS_HIT) continue;
        for (d = 0; d < 4; d++) {
            int nx = x + dx4[d], ny = y + dy4[d], sc;
            if (!unk(b, nx, ny)) continue;
            sc = 1;
            { int px = x - dx4[d], py = y - dy4[d];
              if (px >= 0 && py >= 0 && px < b->w && py < b->h && b->c[py][px].state == CS_HIT) sc = 4; }
            if (sc > best) { best = sc; bc = 0; }
            if (sc == best) { bc++; if (rng_n(r, bc) == 0) { *ox = nx; *oy = ny; } }
        }
    }
    return best > 0;
}

static void pick_prob(const Board *b, Rng *r, int skill, int *ox, int *oy) {
    int w[MAXG][MAXG], x, y, i, o, maxw = 0, bestc = 0, bx = 0, by = 0;
    memset(w, 0, sizeof w);
    for (i = 0; i < b->na; i++) {
        const Asset *a = &b->a[i];
        if (!a->placed || a->sunk) continue;
        for (o = 0; o < 4; o++) {
            int dx[6], dy[6];
            asset_shape(a->def, o, dx, dy);
            for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++) {
                int k, ok = 1, hits = 0;
                for (k = 0; k < a->n && ok; k++) {
                    int cx = x + dx[k], cy = y + dy[k];
                    if (cx >= b->w || cy >= b->h) { ok = 0; break; }
                    if (b->c[cy][cx].state == CS_UNKNOWN) continue;
                    if (b->c[cy][cx].state == CS_HIT) hits++; else ok = 0;
                }
                if (!ok) continue;
                for (k = 0; k < a->n; k++) {
                    int cx = x + dx[k], cy = y + dy[k];
                    if (b->c[cy][cx].state == CS_UNKNOWN) w[cy][cx] += 1 + 25 * hits;
                }
            }
        }
    }
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++) if (unk(b, x, y) && w[y][x] > maxw) maxw = w[y][x];
    if (skill < 5 && maxw > 0)
        for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++)
            if (unk(b, x, y)) w[y][x] += rng_n(r, 1 + maxw * (5 - skill) / 8);
    maxw = 0;
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++) if (unk(b, x, y) && w[y][x] > maxw) maxw = w[y][x];
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++)
        if (unk(b, x, y) && w[y][x] == maxw) { bestc++; if (rng_n(r, bestc) == 0) { bx = x; by = y; } }
    if (bestc == 0) pick_random_unknown(b, r, -1, ox, oy); else { *ox = bx; *oy = by; }
}

void bt_ai_pick(Battle *bt, int *ox, int *oy) {
    const Board *b = &bt->you;
    AI *ai = &bt->ai;
    int x, y;
    static const int noise[6] = {100, 55, 28, 14, 5, 0};
    if (ai->type != AI_RANDOM && !(rng_n(&bt->rng, 100) < noise[CLAMP(ai->skill, 0, 5)])) {
        if (ai->type == AI_PROB) { pick_prob(b, &bt->rng, ai->skill, ox, oy); return; }
        if (pick_target(b, &bt->rng, ox, oy)) return;
        if (ai->type == AI_PARITY) { pick_random_unknown(b, &bt->rng, 0, ox, oy); return; }
        if (ai->type == AI_SWEEP) {
            for (; ai->sweep < b->w * b->h; ai->sweep++) {
                int row = ai->sweep / b->w, col = ai->sweep % b->w;
                x = (row & 1) ? b->w - 1 - col : col; y = row;
                if (unk(b, x, y)) { *ox = x; *oy = y; ai->sweep++; return; }
            }
        }
    }
    pick_random_unknown(b, &bt->rng, -1, ox, oy);
}
