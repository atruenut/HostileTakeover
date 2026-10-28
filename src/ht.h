/* HOSTILE TAKEOVER - master header. Plain C99, no external deps. */
#ifndef HT_H
#define HT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define SCR_W 640
#define SCR_H 480
#define ARRAY_LEN(a) ((int)(sizeof(a)/sizeof((a)[0])))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):((v)>(hi)?(hi):(v)))

/* The Windows 16-colour palette */
enum { C_BLACK=0, C_MAROON, C_GREEN, C_OLIVE, C_NAVY, C_PURPLE, C_TEAL, C_SILVER,
       C_GRAY, C_RED, C_LIME, C_YELLOW, C_BLUE, C_FUCHSIA, C_AQUA, C_WHITE };

typedef struct { int x, y, w, h; } Rect;
static inline Rect R(int x, int y, int w, int h) { Rect r; r.x=x; r.y=y; r.w=w; r.h=h; return r; }
static inline int pt_in(Rect r, int x, int y) { return x>=r.x && y>=r.y && x<r.x+r.w && y<r.y+r.h; }

/* ---------------------------------------------------------------- RNG */
typedef struct { unsigned int s; } Rng;
static inline void rng_seed(Rng *r, unsigned int seed) {
    unsigned int x = seed * 2654435761u + 0x9E3779B9u;
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    if (!x) x = 0x1234567u;
    r->s = x;
}
static inline unsigned int rng_u32(Rng *r) {
    unsigned int x = r->s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    r->s = x; return x;
}
static inline int rng_n(Rng *r, int n) { return n <= 1 ? 0 : (int)((rng_u32(r) >> 4) % (unsigned)n); }
static inline int rng_range(Rng *r, int a, int b) { return a + rng_n(r, b - a + 1); }
static inline int rng_pct(Rng *r, int p) { return rng_n(r, 100) < p; }
extern Rng g_rng; /* cosmetic randomness */

/* ---------------------------------------------------------------- GFX */
extern unsigned char g_fb[SCR_W * SCR_H];      /* 8-bit indexed frame buffer */
extern const unsigned char g_pal[16][3];

typedef struct { short w, h; const char *const *rows; } Spr;

void gfx_clip(Rect r);
void gfx_unclip(void);
Rect gfx_getclip(void);
void gfx_pixel(int x, int y, int c);
void gfx_rect(int x, int y, int w, int h, int c);
void gfx_frame(int x, int y, int w, int h, int c);
void gfx_hline(int x, int y, int w, int c);
void gfx_vline(int x, int y, int h, int c);
void gfx_line(int x0, int y0, int x1, int y1, int c);
void gfx_circle(int cx, int cy, int r, int c, int fill);
void gfx_dither(int x, int y, int w, int h, int c1, int c2, int mode); /* mode 0 checker 1 sparse 2 dense */
void gfx_bevel(int x, int y, int w, int h, int tl, int br);
int  gfx_text_w(const char *s);
int  gfx_text(int x, int y, const char *s, int c);       /* returns end x */
int  gfx_text_b(int x, int y, const char *s, int c);     /* bold */
int  gfx_text_bw(const char *s);
void gfx_text_c(Rect r, const char *s, int c, int bold); /* centred in rect */
void gfx_text_sh(int x, int y, const char *s, int c, int shadow);
void gfx_text_big(int x, int y, const char *s, int c, int scale);
int  gfx_text_big_w(const char *s, int scale);
void gfx_spr(int x, int y, const Spr *s);
void gfx_spr_scaled(int x, int y, const Spr *s, int sc);
void gfx_spr_mono(int x, int y, const Spr *s, int c);
void gfx_spr_flip(int x, int y, const Spr *s);

/* ---------------------------------------------------------------- MODEL */
#define MAXG 12
#define MAXA 14
#define MAXINV 8

enum { A_FERN, A_COOLER, A_PRINTER, A_CUBICLE, A_VENDING, A_DESK, A_RACK, A_MAINFRAME,
       A_TABLE, A_SHREDDER, A_BOARDROOM, N_ASSET };
typedef struct {
    const char *name; int n; signed char dx[6], dy[6];
    unsigned char color; const char *perk; const char *perkdesc; int price;
} AssetDef;
extern const AssetDef g_adef[N_ASSET];

enum { CS_UNKNOWN, CS_MISS, CS_HIT, CS_SUNK };
enum { HZ_NONE, HZ_WET, HZ_DONUT, HZ_CLOSET, HZ_REPLYALL, HZ_COFFEE, N_HAZ };
#define K_HAZ    1   /* hazard revealed */
#define K_STICKY 2   /* sticky note: asset here */
#define K_FLAG   4   /* player flag */
#define K_SHIELD 8   /* shield revealed (firewall) */
#define K_HIGH   16  /* highlighter */

typedef struct {
    signed char asset, part;
    unsigned char state, haz, known, lock;
    signed char scan;           /* scanner distance, -1 none */
} Cell;

typedef struct {
    int def, orient, n, placed, sunk, nhit, owner;
    unsigned char x[6], y[6], hit[6], shield[6];
} Asset;

typedef struct {
    int w, h, na;
    Cell c[MAXG][MAXG];
    Asset a[MAXA];
    signed char rowhint[MAXG], colhint[MAXG];
} Board;

enum { SH_NONE, SH_MISS, SH_HIT, SH_SUNK, SH_ALREADY, SH_LOCKED, SH_SHIELD, SH_OOB };
typedef struct { int res, asset, part, rx, ry; } ShotRes;

void asset_shape(int def, int orient, int *dx, int *dy);
void board_init(Board *b, int w, int h);
int  board_add(Board *b, int def, int owner);
int  board_can_place(const Board *b, int def, int orient, int x, int y, int ignore);
int  board_place(Board *b, int ai, int x, int y, int orient);
void board_unplace(Board *b, int ai);
int  board_random_place(Board *b, int ai, Rng *r);
int  board_autoplace(Board *b, Rng *r);
int  board_all_sunk(const Board *b);
int  board_alive(const Board *b);
int  board_cells_alive(const Board *b);
int  board_shoot(Board *b, int x, int y, ShotRes *sr);

enum { AI_RANDOM, AI_HUNT, AI_PARITY, AI_PROB, AI_SWEEP };
#define RULE_FIREWALL   1
#define RULE_REORG      2
#define RULE_DOUBLE     4
#define RULE_INJUNCTION 8
#define RULE_AUDIT      16

typedef struct {
    const char *dept, *tag, *boss;  /* boss==NULL for normal floors */
    int label, size, ai, skill, rules, reorg_n, double_n, inj_n, dmg;
    int haz[5];                     /* weights: wet donut closet replyall coffee */
    int w[N_ASSET];                 /* enemy asset pool weights */
    int cells_pct;                  /* enemy fleet cells as % of grid */
} FloorDef;
#define N_FLOORS 13
extern const FloorDef g_floors[N_FLOORS];

enum { P_CHAIR, P_STANDING, P_HEADSET, P_LANYARD, P_CORPCARD, P_KEYBOARD, P_DUALMON, P_LUCKYMUG,
       P_POSTER, P_MACRO, P_MOUSE, P_EXTING, P_PLAQUE, P_STRESSBALL, P_PARACHUTE, P_INTERN, N_PERK };
typedef struct { const char *name, *desc; int rare; } PerkDef;
extern const PerkDef g_perk[N_PERK];

enum { I_STAPLER, I_HIGHLIGHT, I_STICKY, I_FAX, I_SCANNER, I_WHITEOUT, I_ENERGY, I_JAM, I_CALC,
       I_ALLHANDS, I_AIRPLANE, I_MUG, N_ITEM };
typedef struct { const char *name, *desc; int target, ap, price, rarity; } ItemDef;
extern const ItemDef g_item[N_ITEM];

enum { CAR_INTERN, CAR_ACCOUNTANT, CAR_IT, CAR_SALES, CAR_JANITOR, N_CAREER };
extern const char *g_career_name[N_CAREER];
extern const char *g_career_desc[N_CAREER];

enum { M_MATH, M_WORDS, M_LEDGER, M_WORLD, M_TYPING, N_MODULE };

typedef struct {
    char name[24];
    unsigned int seed;
    int career, floor, overtime, won, dead;
    int morale, maxmorale, cash, synergy;
    int inv[MAXINV], maxinv;
    unsigned int perks;
    int fleet[MAXA], nfleet;
    long score;
    int shots, hits, sunk, correct, wrong, turns, streak, best_streak, floors_cleared;
    int train_xp[N_MODULE];
    unsigned int certs;
    int calc, disc, parachute_used;
    Rng rng;
} Run;
#define HAS(run, p) (((run)->perks >> (p)) & 1u)

enum { PH_PLACE, PH_PLAYER, PH_ENEMY, PH_WIN, PH_LOSE };

typedef struct {
    int type, skill, ntq, sweep, parity;
    int tq[96][2];
} AI;

typedef struct {
    Board you, foe;
    FloorDef fd;
    int phase, turn, ap, enemy_turns, enemy_skip, enemy_shots, audit_combo;
    int jam_used, backup_used, pend_quiz, bonus_ap_next, banner_reorg;
    int refund_used, shred_used, trained, macro_used, incident;
    int shots, hits, acc_hits, sunk_you, cause;
    int last_x, last_y, floor_idx;
    char foe_names[MAXA][20];
    char boss_name[40];
    int carpet, carpet2, carpet_pat;
    AI ai;
    Rng rng;
} Battle;

void run_new(Run *r, const char *name, unsigned int seed, int career);
int  run_max_morale(const Run *r);
int  run_item_add(Run *r, int item);
int  run_item_count(const Run *r);
void run_heal(Run *r, int amt);
void run_hurt(Run *r, int amt);
void run_synergy(Run *r, int amt);
int  floor_label(int idx);
void battle_new(Battle *b, Run *r);
void battle_begin(Battle *b, Run *r);            /* after placement */
int  bt_fire(Battle *b, Run *r, int x, int y);   /* player direct shot; returns SH_* */
int  bt_use_item(Battle *b, Run *r, int slot, int x, int y); /* 1 = used */
int  bt_synergize(Battle *b, Run *r);
void bt_end_turn(Battle *b, Run *r);             /* player ends turn -> enemy phase */
void bt_ai_pick(Battle *b, int *x, int *y);
int  bt_enemy_resolve(Battle *b, Run *r, int x, int y);  /* 1 if hit */
void bt_enemy_done(Battle *b, Run *r);           /* end of enemy turn bookkeeping */
void bt_replyall_result(Battle *b, Run *r, int ok);
int  bt_status(Battle *b, Run *r);               /* 0 playing, 1 won, -1 office sunk, -2 burnout */
void bt_floor_reward(Battle *b, Run *r, int *score, int *cash, int *heal);
int  bt_next_enemy_shots(Battle *b);
/* training result: returns cash gained (>=0) or morale lost as negative */
int  run_train_result(Run *r, int module, int ok, int diff);
int  bt_train_result(Battle *b, Run *r, int module, int ok, int diff);
int  run_quiz_diff(const Run *r, int module);
int  bt_can_train(const Battle *b);
int  bt_alive_perk(const Board *bd, int def);
int  run_free_pass(Battle *b, Run *r);   /* calculator / macro: 1 if next quiz auto-passes (consumes) */
const char *cert_name(int module, int level);
int  run_cert_level(const Run *r, int module);

/* perks, events, shop */
void perk_offer(Run *r, int out[3]);
typedef struct { int item[3], iprice[3], isold[3], asset, aprice, asold, perk, pprice, psold, mystery_sold, rerolls; } Shop;
void shop_make(Shop *s, Run *r, Rng *g);
int  item_price(const Run *r, int base);
int  event_pick(Run *r);
typedef struct { const char *label, *good, *bad; int pct, gfx, gamt, bfx, bamt; } EvOpt;
typedef struct { const char *title, *text; int nopt; EvOpt opt[3]; } EventDef;
extern const EventDef g_event[];
extern const int g_nevent;
const char *event_apply(Run *r, int ev, int opt, int *ok);
enum { FX_NONE, FX_MORALE, FX_CASH, FX_ITEM, FX_PERK, FX_SYNERGY, FX_MAXMORALE };

/* flavour text */
void gen_memo(Rng *r, char *out, int max);
void gen_name(Rng *r, char *out, int max);
const char *quip_win(Rng *r);
const char *quip_lose(Rng *r);

/* hall of fame / config (INI) */
typedef struct { char name[24]; long score; int floor, career, won; unsigned int seed; } HofEntry;
#define HOF_N 10
typedef struct { HofEntry e[HOF_N]; int n; int sound, music, wallpaper, fast; char lastname[24];
    int pdiff, pxp[N_MODULE], pright, pwrong, pbest, tut_seen, hs[8]; } Config;
extern Config g_cfg;
void cfg_load(void);
void cfg_save(void);
int  hof_add(const char *name, long score, int floor, int career, int won, unsigned int seed);

/* callbacks from model into UI (implemented in ui.c; stubbed in tests) */
void fx_log(int color, const char *fmt, ...);
void fx_float(int side, int x, int y, int color, const char *txt);
void fx_boom(int side, int x, int y, int kind);
void fx_banner(const char *txt);
void fx_shake(int amt);

/* ---------------------------------------------------------------- QUIZ */
typedef struct {
    int module, diff, kind;      /* kind 0 multiple choice, 1 typing */
    char q[200], opt[4][56], ans[48], expl[130];
    int nopt, correct, time_ms;
} Question;
extern const char *g_modname[N_MODULE];
extern const char *g_modblurb[N_MODULE];
void quiz_make(Question *q, Rng *r, int module, int diff);

/* ---------------------------------------------------------------- ART */
extern const Spr S_ASSET[N_ASSET];
extern const Spr S_HAZ[N_HAZ];
extern const Spr S_ITEM[N_ITEM];
extern const Spr S_PM[9];
extern const Spr S_MB[4];
extern const Spr S_BOOM, S_MISS, S_FLAG, S_NOTE, S_SHIELD, S_LOCK, S_PLANE, S_SMILE, S_COIN, S_STAR;
extern const Spr S_CURSOR, S_HOURGLASS, S_STAN_BASE;
extern const Spr S_STAN_ARM[3];
void art_check(void);

/* ---------------------------------------------------------------- AUDIO */
enum { SFX_CLICK, SFX_FIRE, SFX_HIT, SFX_MISS, SFX_SUNK, SFX_CORRECT, SFX_WRONG, SFX_COIN, SFX_DING,
       SFX_FANFARE, SFX_ERROR, SFX_POWERUP, SFX_ALARM, SFX_BOSS, SFX_TYPE, SFX_GAMEOVER, SFX_SHUFFLE,
       SFX_SPLASH, N_SFX };
enum { MUS_NONE, MUS_ELEVATOR, MUS_BATTLE, MUS_BOSS, MUS_SHOP, MUS_MENU, MUS_VICTORY, MUS_DEAD, N_MUS };
#define AUDIO_RATE 22050
void audio_init(void);
void audio_sfx(int id);
void audio_music(int style, unsigned int seed);
void audio_enable(int sfx, int music);
void audio_mix(short *out, int n);

/* ---------------------------------------------------------------- PLATFORM (implemented per OS) */
int  plat_load(const char *fname, char *buf, int max);   /* bytes read or -1 */
int  plat_save(const char *fname, const char *buf, int len);
void plat_quit(void);
void plat_fullscreen(void);
unsigned int plat_time_ms(void);

/* ---------------------------------------------------------------- UI ENTRY */
#define KEY_BACK   8
#define KEY_TAB    9
#define KEY_RETURN 13
#define KEY_ESC    27
#define KEY_SPACE  32
#define KEY_LEFT   0x25
#define KEY_UP     0x26
#define KEY_RIGHT  0x27
#define KEY_DOWN   0x28
#define KEY_F1     0x70
#define KEY_F2     0x71
#define KEY_F9     0x78
#define KEY_F11    0x7A
#define KEY_DELETE 0x2E

void ht_init(unsigned int seed);
void ht_shutdown(void);
void ht_frame(int dt_ms);
void ht_mouse_move(int x, int y);
void ht_mouse_button(int which, int down);   /* 0 left 1 right 2 middle */
void ht_key(int vk, int down);
void ht_char(int ch);
void ht_wheel(int delta);
int  ht_cursor_hidden(void);
int  ht_want_quit(void);

#endif
