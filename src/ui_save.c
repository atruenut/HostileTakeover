/* Save / resume: the whole run (Run + Battle + shop/review/event flow state) in TAKEOVER.SAV next to the exe. */
#include "ui.h"

#define SAVE_NAME "TAKEOVER.SAV"
#define SAVE_MAGIC 0x56534854u  /* 'THSV' */
#define SAVE_VER 2u
#define SAVE_MAX 65536

typedef struct {
    unsigned magic, ver, szrun, szbt, szflow, scr, di, ti, bi, crc, szhdr;
} SaveHdr;

static unsigned char sbuf[SAVE_MAX];

static unsigned crc_of(const unsigned char *p, int n) {
    unsigned h = 2166136261u;
    int i;
    for (i = 0; i < n; i++) { h ^= p[i]; h *= 16777619u; }
    return h;
}
static int str_index(const char *p, int which) {
    int i;
    if (!p) return -1;
    for (i = 0; i < N_FLOORS; i++) {
        const char *q = which == 0 ? g_floors[i].dept : which == 1 ? g_floors[i].tag : g_floors[i].boss;
        if (q == p) return i;
    }
    return 0;
}

void save_clear(void) { plat_save(SAVE_NAME, "", 0); }

void game_save(void) {
    SaveHdr h;
    Battle bt;
    int scr = ui_game_screen(), n, fl;
    unsigned char *p = sbuf + sizeof h;
    if (!g_in_run || scr < 0 || g_run.dead) return;
    memset(&h, 0, sizeof h);
    bt = g_bt;
    h.di = (unsigned)str_index(bt.fd.dept, 0); h.ti = (unsigned)str_index(bt.fd.tag, 1); h.bi = (unsigned)(str_index(bt.fd.boss, 2) + 1);
    bt.fd.dept = bt.fd.tag = bt.fd.boss = NULL;
    memcpy(p, &g_run, sizeof g_run); p += sizeof g_run;
    memcpy(p, &bt, sizeof bt); p += sizeof bt;
    fl = flow_pack(p, (int)(sbuf + SAVE_MAX - p));
    if (!fl) return;
    p += fl;
    h.magic = SAVE_MAGIC; h.ver = SAVE_VER; h.szrun = sizeof g_run; h.szbt = sizeof bt; h.szflow = (unsigned)fl; h.scr = (unsigned)scr; h.szhdr = sizeof h;
    n = (int)(p - (sbuf + sizeof h));
    h.crc = crc_of(sbuf + sizeof h, n);
    memcpy(sbuf, &h, sizeof h);
    plat_save(SAVE_NAME, (const char *)sbuf, (int)sizeof h + n);
}

int game_load(void) {
    SaveHdr h;
    int n = plat_load(SAVE_NAME, (char *)sbuf, SAVE_MAX), body;
    static Run run; static Battle bt;
    const unsigned char *p;
    if (n < (int)sizeof h) return 0;
    memcpy(&h, sbuf, sizeof h);
    if (h.magic != SAVE_MAGIC || h.ver != SAVE_VER || h.szhdr != sizeof h || h.szrun != sizeof run || h.szbt != sizeof bt) return 0;
    body = (int)(h.szrun + h.szbt + h.szflow);
    if (n != (int)sizeof h + body || crc_of(sbuf + sizeof h, body) != h.crc) return 0;
    if (h.scr != SC_ELEVATOR && h.scr != SC_PLACE && h.scr != SC_BATTLE && h.scr != SC_REVIEW && h.scr != SC_EVENT && h.scr != SC_SHOP) return 0;
    if (h.di >= N_FLOORS || h.ti >= N_FLOORS || h.bi > N_FLOORS) return 0;
    p = sbuf + sizeof h;
    memcpy(&run, p, sizeof run); p += sizeof run;
    memcpy(&bt, p, sizeof bt); p += sizeof bt;
    if (run.floor < 0 || run.floor > 12 || run.career < 0 || run.career >= N_CAREER || run.morale > 9999 || run.nfleet < 0 || run.nfleet > MAXA) return 0;
    if ((h.scr == SC_PLACE || h.scr == SC_BATTLE) && (bt.you.w < 1 || bt.you.w > MAXG || bt.you.h < 1 || bt.you.h > MAXG || bt.foe.w < 1 || bt.foe.w > MAXG || bt.foe.h < 1 || bt.foe.h > MAXG ||
        bt.you.na < 0 || bt.you.na > MAXA || bt.foe.na < 0 || bt.foe.na > MAXA || bt.phase < 0 || bt.phase > PH_LOSE)) return 0;
    if (!flow_unpack(p, (int)h.szflow)) return 0;
    bt.fd.dept = g_floors[h.di].dept; bt.fd.tag = g_floors[h.ti].tag; bt.fd.boss = h.bi ? g_floors[h.bi - 1].boss : NULL;
    run.name[sizeof run.name - 1] = 0;
    g_run = run; g_bt = bt; g_in_run = 1;
    ui_set_resume((int)h.scr, 1);
    return 1;
}
