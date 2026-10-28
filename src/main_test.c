/* Headless logic harness: fuzzes procgen, AI, battle rules, quiz engine. */
#include "ht.h"
#include <assert.h>

void fx_log(int color, const char *fmt, ...) { (void)color; (void)fmt; }
void fx_float(int side, int x, int y, int color, const char *txt) { (void)side; (void)x; (void)y; (void)color; (void)txt; }
void fx_boom(int side, int x, int y, int kind) { (void)side; (void)x; (void)y; (void)kind; }
void fx_banner(const char *txt) { (void)txt; }
void fx_shake(int amt) { (void)amt; }
int plat_load(const char *f, char *b, int m) { (void)f; (void)b; (void)m; return -1; }
int plat_save(const char *f, const char *b, int l) { (void)f; (void)b; (void)l; return 0; }

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; if (fails < 20) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)

static void check_board(const Board *b, const char *who) {
    int x, y, i, j, cells = 0;
    for (i = 0; i < b->na; i++) {
        const Asset *a = &b->a[i];
        if (!a->placed) { CHECK(0, "%s asset %d unplaced", who, i); continue; }
        for (j = 0; j < a->n; j++) {
            CHECK(a->x[j] < b->w && a->y[j] < b->h, "%s asset oob", who);
            CHECK(b->c[a->y[j]][a->x[j]].asset == i, "%s cell/asset mismatch", who);
            cells++;
        }
    }
    for (y = 0; y < b->h; y++) for (x = 0; x < b->w; x++) if (b->c[y][x].asset >= 0) cells--;
    CHECK(cells == 0, "%s cell count mismatch %d", who, cells);
}

static int bot_pick_foe(Battle *b, int *ox, int *oy, Rng *r) {
    /* smart-ish player bot: hunt neighbours of hits, else random */
    Board *f = &b->foe;
    int x, y, n = 0, tx = -1, ty = -1;
    static const int dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
    for (y = 0; y < f->h; y++) for (x = 0; x < f->w; x++) if (f->c[y][x].state == CS_HIT) {
        int d;
        for (d = 0; d < 4; d++) { int nx = x + dx[d], ny = y + dy[d];
            if (nx >= 0 && ny >= 0 && nx < f->w && ny < f->h && f->c[ny][nx].state == CS_UNKNOWN) { n++; if (rng_n(r, n) == 0) { tx = nx; ty = ny; } } }
    }
    if (n) { *ox = tx; *oy = ty; return 1; }
    n = 0;
    for (y = 0; y < f->h; y++) for (x = 0; x < f->w; x++) if (f->c[y][x].state == CS_UNKNOWN || f->c[y][x].lock) { n++; if (rng_n(r, n) == 0) { tx = x; ty = y; } }
    if (!n) return 0;
    *ox = tx; *oy = ty; return 1;
}

static long play_run(unsigned seed, int career, int *floors, int verbose, long *turns) {
    Run run; Battle b; Rng br; int guard = 0;
    run_new(&run, "Test", seed, career);
    rng_seed(&br, seed ^ 0xabcdef);
    while (!run.dead && run.floor < N_FLOORS && guard++ < 200) {
        int st = 0, steps = 0;
        battle_new(&b, &run);
        check_board(&b.you, "you"); check_board(&b.foe, "foe");
        CHECK(b.foe.na > 0, "foe has assets");
        battle_begin(&b, &run);
        while (steps++ < 2500) {
            int x, y, n;
            if (b.phase == PH_PLAYER) {
                /* sometimes use items / train / synergize */
                if (rng_n(&br, 6) == 0) { int s; for (s = 0; s < MAXINV; s++) if (run.inv[s]) { bt_use_item(&b, &run, s, rng_n(&br, b.foe.w), rng_n(&br, b.foe.h)); break; } }
                if (rng_n(&br, 5) == 0 && bt_can_train(&b)) { Question q; int m = rng_n(&br, N_MODULE); int d = run_quiz_diff(&run, m);
                    quiz_make(&q, &br, m, d); CHECK(q.kind == 1 ? q.ans[0] != 0 : (q.correct >= 0 && q.correct < 4), "quiz sane");
                    bt_train_result(&b, &run, m, rng_n(&br, 3) != 0, d); }
                if (run.synergy >= 100 && rng_n(&br, 2) == 0) bt_synergize(&b, &run);
                if (b.pend_quiz) bt_replyall_result(&b, &run, rng_n(&br, 2));
                while (b.ap > 0 && b.phase == PH_PLAYER) {
                    if (!bot_pick_foe(&b, &x, &y, &br)) break;
                    n = bt_fire(&b, &run, x, y);
                    if (b.pend_quiz) bt_replyall_result(&b, &run, rng_n(&br, 2));
                    if (n == -1) break;
                    if (bt_status(&b, &run) != 0) break;
                }
                st = bt_status(&b, &run);
                if (st != 0) break;
                bt_end_turn(&b, &run);
            }
            if (b.phase == PH_ENEMY) {
                int k;
                for (k = 0; k < b.enemy_shots; k++) {
                    bt_ai_pick(&b, &x, &y);
                    CHECK(x >= 0 && y >= 0 && x < b.you.w && y < b.you.h, "ai oob (%d,%d)", x, y);
                    CHECK(b.you.c[y][x].state == CS_UNKNOWN || board_cells_alive(&b.you) == 0, "ai shot at known cell");
                    bt_enemy_resolve(&b, &run, x, y);
                    if (bt_status(&b, &run) != 0) break;
                }
                st = bt_status(&b, &run);
                if (st != 0) break;
                bt_enemy_done(&b, &run);
                st = bt_status(&b, &run);
                if (st != 0) break;
            }
            if (b.phase == PH_WIN || b.phase == PH_LOSE) { st = bt_status(&b, &run); break; }
        }
        CHECK(steps < 2500, "battle did not terminate (floor %d)", run.floor);
        *turns += b.turn;
        if (st == 1) {
            int sc, ca, he;
            bt_floor_reward(&b, &run, &sc, &ca, &he);
            run.floor++;
            if (run.floor >= N_FLOORS) { run.won = 1; break; }
        } else if (st == -2) { run.dead = 1; }
        /* st == -1: retry same floor (morale already reduced) */
        CHECK(run.morale >= 0 && run.morale <= run_max_morale(&run), "morale out of range %d", run.morale);
    }
    *floors = run.floor;
    (void)verbose;
    return run.score;
}

static void test_shapes(void) {
    int d, o, i;
    for (d = 0; d < N_ASSET; d++) for (o = 0; o < 4; o++) {
        int dx[6], dy[6], mnx = 99, mny = 99, j;
        asset_shape(d, o, dx, dy);
        for (i = 0; i < g_adef[d].n; i++) { if (dx[i] < mnx) mnx = dx[i]; if (dy[i] < mny) mny = dy[i];
            for (j = i + 1; j < g_adef[d].n; j++) CHECK(!(dx[i] == dx[j] && dy[i] == dy[j]), "dup cell in shape %d/%d", d, o); }
        CHECK(mnx == 0 && mny == 0, "shape %d/%d not normalised", d, o);
    }
}

static void test_quiz(void) {
    Rng r; int m, d, i, bad = 0;
    rng_seed(&r, 99);
    for (m = 0; m < N_MODULE; m++) for (d = 1; d <= 5; d++) for (i = 0; i < 400; i++) {
        Question q; int a, b2;
        quiz_make(&q, &r, m, d);
        CHECK(q.q[0] && q.expl[0], "empty question text m%d d%d", m, d);
        if (q.kind == 0) {
            CHECK(q.nopt == 4 && q.correct >= 0 && q.correct < 4, "bad correct idx");
            CHECK(!strcmp(q.opt[q.correct], q.ans), "ans mismatch");
            for (a = 0; a < 4; a++) for (b2 = a + 1; b2 < 4; b2++) if (!strcmp(q.opt[a], q.opt[b2])) { bad++; if (bad < 4) printf("  dup option: %s | %s | q=%s\n", q.opt[a], q.opt[b2], q.q); }
        } else CHECK(q.ans[0] && q.time_ms > 0, "typing q");
    }
    CHECK(bad == 0, "%d duplicate options", bad);
}

static void print_sample_quiz(void) {
    Rng r; int m;
    rng_seed(&r, 4242);
    for (m = 0; m < N_MODULE; m++) {
        Question q; int k;
        quiz_make(&q, &r, m, 3);
        printf("[%s] %s\n", g_modname[m], q.q);
        for (k = 0; k < q.nopt; k++) printf("   %c %s%s\n", 'A' + k, q.opt[k], k == q.correct ? "   <-- correct" : "");
        if (q.kind) printf("   (type: %s)\n", q.ans);
    }
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 300, i, c, hist[N_FLOORS + 1];
    long totscore = 0, turns = 0;
    int wins = 0;
    memset(hist, 0, sizeof hist);
    g_rng.s = 12345;
    test_shapes();
    test_quiz();
    for (i = 0; i < n; i++) {
        int fl = 0;
        c = i % N_CAREER;
        totscore += play_run(1000u + (unsigned)i * 77u, c, &fl, 0, &turns);
        hist[CLAMP(fl, 0, N_FLOORS)]++;
        if (fl >= N_FLOORS) wins++;
    }
    printf("runs=%d avg_score=%ld avg_turns/floor_attempt~%ld wins=%d\n", n, totscore / n, turns / (n * 4), wins);
    printf("floor reached histogram (random-ish bot, no shop):\n");
    for (i = 0; i <= N_FLOORS; i++) printf("  floor %2d: %d\n", i + (i < N_FLOORS ? 1 : 0), hist[i]);
    print_sample_quiz();
    printf(fails ? "\n%d FAILURES\n" : "\nALL CHECKS PASSED\n", fails);
    return fails ? 1 : 0;
}
