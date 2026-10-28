/* Headless UI simulator for Linux: scripted input, PPM screenshots, autopilot. */
#include "ui.h"
#include <ctype.h>

int plat_load(const char *f, char *b, int m) { FILE *fp = fopen(f, "rb"); int n; if (!fp) return -1; n = (int)fread(b, 1, (size_t)m, fp); fclose(fp); return n; }
int plat_save(const char *f, const char *b, int l) { FILE *fp = fopen(f, "wb"); if (!fp) return -1; fwrite(b, 1, (size_t)l, fp); fclose(fp); return 0; }
void plat_quit(void) {}
void plat_fullscreen(void) {}
unsigned int plat_time_ms(void) { return 0; }

int ui_dbg_foe_px(int gx, int gy, int *px, int *py);

static void shot(const char *name) {
    FILE *f = fopen(name, "wb");
    int i;
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
    for (i = 0; i < SCR_W * SCR_H; i++) fwrite(g_pal[g_fb[i]], 1, 3, f);
    fclose(f);
}

static void frames(int n, int dt) { int i; for (i = 0; i < n; i++) ht_frame(dt); }
static void click(int x, int y) { ht_mouse_move(x, y); frames(1, 16); ht_mouse_button(0, 1); frames(1, 16); ht_mouse_button(0, 0); frames(1, 16); }
static void key(int vk) { ht_key(vk, 1); ht_key(vk, 0); frames(1, 16); }

static int keyname(const char *s) {
    if (!strcmp(s, "ESC")) { return KEY_ESC; }
    if (!strcmp(s, "RET")) { return KEY_RETURN; }
    if (!strcmp(s, "SPACE")) { return KEY_SPACE; }
    if (!strcmp(s, "F1")) { return KEY_F1; }
    if (!strcmp(s, "F9")) { return KEY_F9; }
    if (!strcmp(s, "TAB")) { return KEY_TAB; }
    if (!strcmp(s, "LEFT")) { return KEY_LEFT; }
    if (!strcmp(s, "RIGHT")) { return KEY_RIGHT; }
    return toupper((unsigned char)s[0]);
}

/* one frame of autopilot; returns 1 when the run ended */
static int autopilot(int *stuck, Screen *last) {
    static int cnt;
    if (g_scr != *last) { *last = g_scr; cnt = 0; }
    cnt++;
    if (ui_dialog_active()) { key(KEY_RETURN); return 0; }
    if (quiz_active()) { cnt = 0; ht_char('x'); key('1' + (int)(g_frame % 4)); key(KEY_RETURN); return 0; }
    switch (g_scr) {
    case SC_ELEVATOR: key(KEY_RETURN); break;
    case SC_PLACE: key(KEY_RETURN); break;
    case SC_REVIEW: key('1'); key(KEY_RETURN); break;
    case SC_EVENT: key('1'); frames(2, 16); key(KEY_RETURN); break;
    case SC_SHOP: if (g_run.cash > 60 && cnt == 1) { click(60, 150); } key(KEY_RETURN); break;
    case SC_WIN: g_run.overtime = 0; go_dead(); break;
    case SC_DEAD: return 1;
    case SC_BATTLE: {
        int x, y, px, py;
        if (g_battle_busy) { frames(1, 40); break; }
        if (g_bt.phase == PH_PLAYER && g_bt.ap > 0) {
            int tries = 0;
            do { x = rng_n(&g_rng, g_bt.foe.w); y = rng_n(&g_rng, g_bt.foe.h); tries++; } while (g_bt.foe.c[y][x].state != CS_UNKNOWN && tries < 400);
            if (ui_dbg_foe_px(x, y, &px, &py)) click(px, py);
            if (rng_n(&g_rng, 4) == 0) key('T');
            if (g_run.synergy >= 100) key('S');
        } else { key(KEY_SPACE); frames(1, 40); }
        break; }
    default: frames(1, 40); break;
    }
    if (++*stuck > 400000) return 2;
    return 0;
}

int main(int argc, char **argv) {
    char line[256];
    FILE *in = argc > 1 ? fopen(argv[1], "r") : stdin;
    int n;
    if (!in) { fprintf(stderr, "cannot open script\n"); return 1; }
    ht_init(1234);
    while (fgets(line, sizeof line, in)) {
        char cmd[32], a1[160]; int a, b, c2;
        cmd[0] = 0; a1[0] = 0;
        if (line[0] == '#' || line[0] == '\n') continue;
        n = sscanf(line, "%31s %159s", cmd, a1);
        if (n < 1) continue;
        if (!strcmp(cmd, "frames")) { a = 1; b = 16; sscanf(line, "%*s %d %d", &a, &b); frames(a, b); }
        else if (!strcmp(cmd, "move")) { sscanf(line, "%*s %d %d", &a, &b); ht_mouse_move(a, b); frames(1, 16); }
        else if (!strcmp(cmd, "click")) { sscanf(line, "%*s %d %d", &a, &b); click(a, b); }
        else if (!strcmp(cmd, "dbl")) { sscanf(line, "%*s %d %d", &a, &b); click(a, b); click(a, b); }
        else if (!strcmp(cmd, "rclick")) { sscanf(line, "%*s %d %d", &a, &b); ht_mouse_move(a, b); frames(1, 16); ht_mouse_button(1, 1); frames(1, 16); ht_mouse_button(1, 0); frames(1, 16); }
        else if (!strcmp(cmd, "key")) key(keyname(a1));
        else if (!strcmp(cmd, "text")) { char *p = strchr(line, ' '); if (p) for (p++; *p && *p != '\n'; p++) { ht_char(*p); frames(1, 16); } }
        else if (!strcmp(cmd, "shot")) shot(a1);
        else if (!strcmp(cmd, "run")) { unsigned sd = 1; a = 0; sscanf(line, "%*s %u %d", &sd, &a); g_cfg.tut_seen = 1; start_run("Tom", sd, a); frames(2, 16); }
        else if (!strcmp(cmd, "floor")) { sscanf(line, "%*s %d", &a); g_run.floor = a; }
        else if (!strcmp(cmd, "give")) { sscanf(line, "%*s %d %d", &a, &b); if (a == 0) g_run.cash += b; else if (a == 1) g_run.synergy = b; else if (a == 2) run_item_add(&g_run, b); else if (a == 3) g_run.perks |= 1u << b; else if (a == 4) g_run.morale = b; }
        else if (!strcmp(cmd, "goto")) { sscanf(line, "%*s %d", &a); scr_go((Screen)a); frames(2, 16); }
        else if (!strcmp(cmd, "win")) { /* force-clear current battle */ int i; for (i = 0; i < g_bt.foe.na; i++) { int j; for (j = 0; j < g_bt.foe.a[i].n; j++) { g_bt.foe.a[i].hit[j] = 1; g_bt.foe.c[g_bt.foe.a[i].y[j]][g_bt.foe.a[i].x[j]].state = CS_SUNK; } g_bt.foe.a[i].nhit = g_bt.foe.a[i].n; g_bt.foe.a[i].sunk = 1; } frames(5, 16); }
        else if (!strcmp(cmd, "autopilot")) {
            int stuck = 0, r2; Screen last = (Screen)-1; int maxrun = 1; sscanf(line, "%*s %d", &maxrun); (void)c2;
            while (!(r2 = autopilot(&stuck, &last))) { }
            printf("autopilot ended: code=%d floor=%d score=%ld morale=%d\n", r2, g_run.floor + 1, g_run.score, g_run.morale);
        }
        else if (!strcmp(cmd, "tutorial")) { tut_start(); frames(2, 16); }
        else if (!strcmp(cmd, "save")) ht_shutdown();
        else if (!strcmp(cmd, "load")) { memset(&g_run, 0, sizeof g_run); memset(&g_bt, 0, sizeof g_bt); g_in_run = 0; g_scr = SC_DESKTOP; printf("load=%d\n", game_load()); frames(2, 16); }
        else if (!strcmp(cmd, "resume")) { ui_resume_game(); frames(2, 16); }
        else if (!strcmp(cmd, "state")) printf("scr=%d phase=%d ap=%d morale=%d cash=%d floor=%d dialog=%d quiz=%d\n", g_scr, g_bt.phase, g_bt.ap, g_run.morale, g_run.cash, g_run.floor, ui_dialog_active(), quiz_active());
    }
    return 0;
}
