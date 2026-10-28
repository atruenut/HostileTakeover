/* Internal UI header shared by ui*.c */
#ifndef UI_H
#define UI_H
#include "ht.h"

typedef enum { SC_DESKTOP, SC_NEWGAME, SC_ELEVATOR, SC_PLACE, SC_BATTLE, SC_REVIEW, SC_EVENT, SC_SHOP,
               SC_DEAD, SC_WIN, SC_TRAIN, SC_HOF, SC_HELP, SC_ABOUT, SC_BOSSKEY, SC_TUTORIAL, SC_ARCADE, SC_GAME } Screen;

/* menu actions */
enum { ACT_NONE, ACT_NEW, ACT_HOF, ACT_SND, ACT_MUS, ACT_EXIT, ACT_HELP, ACT_ABOUT, ACT_QUITRUN, ACT_DESKTOP, ACT_WALL, ACT_TRAIN, ACT_TUT, ACT_ARCADE };

/* input (per frame) */
extern int g_mx, g_my, g_down, g_press, g_release, g_rpress, g_dbl;
extern unsigned int g_time;
extern int g_dt, g_frame;
int  ui_key(int vk);                 /* consume a key-down event if present */
int  ui_char(void);                  /* next typed char or 0 */
int  ui_hover(Rect r);

/* game state */
extern Run g_run;
extern Battle g_bt;
extern Shop g_shop;
extern Screen g_scr;
extern int g_in_run;
void scr_go(Screen s);
int  ui_game_screen(void);            /* run screen to resume, or -1 */
void ui_set_resume(int scr, int from_disk);
void ui_resume_game(void);
int  ui_can_resume(void);

/* persistence (ui_save.c) */
void game_save(void);                 /* write current run (or clear if none) */
void save_clear(void);
int  game_load(void);                 /* 1 if a run was restored */
int  flow_pack(void *buf, int max);
int  flow_unpack(const void *buf, int n);
void battle_resume(void);
void ht_shutdown(void);

/* tutorial (ui_tutorial.c) */
int  tut_active(void);
void tut_start(void);
void tut_start_then_new(void);
void tut_exit(void);
void tut_frame(void);
int  battle_idle(void);
int  battle_ui_rect(int which, Rect *r);
int  place_ui_rect(int which, Rect *r);
int  ui_dbg_foe_px(int gx, int gy, int *px, int *py);
void quiz_autoanswer(void);
void quiz_cancel(void);
void ui_swallow(int on);
void ui_active_reset(void);
void ui_raw_input(int *mx, int *my, int *press, int *release, int *down);

/* arcade (ui_games.c) */
void arcade_draw(void);
void game_draw(void);
void arcade_open(void);

/* drawing helpers */
void ui_desktop_bg(void);
Rect ui_window(Rect r, const char *title, int active);   /* returns client rect */
Rect ui_app(const char *title, int menubar);              /* maximised window; returns client */
void ui_raised(Rect r);
void ui_sunken(Rect r);
void ui_groove(Rect r);
int  ui_button(Rect r, const char *label, int enabled);
int  ui_button_def(Rect r, const char *label, int enabled);
void ui_bar(Rect r, int val, int max, int color, const char *label);
int  ui_wrap(Rect r, const char *text, int color, int draw);
void ui_icon_label(int x, int y, const Spr *s, int scale, const char *label, int selected);
void ui_dialog(int id, const char *title, const char *text, int icon, const char *b0, const char *b1, const char *b2);
int  ui_dialog_result(int id);
int  ui_dialog_active(void);
void ui_textfield(Rect r, char *buf, int max, int focus, int numeric);
void ui_stan(int x, int y, int mood, int blink);          /* Stan the Stapler mascot; mood 0 idle 1 happy 2 sad */
void ui_menu_request(int act);
int  ui_take_action(void);
void ui_title_card(Rect r, const char *title, const char *sub);

/* palette helpers */
#define COL_FACE C_SILVER
#define COL_HI C_WHITE
#define COL_SH C_GRAY
#define COL_DK C_BLACK

/* battle */
void battle_enter(int floor_idx_changed);
void place_draw(void);
void battle_draw(void);
void battle_update_fx(int dt);
void battle_reset_fx(void);
extern int g_battle_busy;

/* quiz */
enum { QM_TRAIN, QM_REPLY, QM_PRACTICE, QM_HAGGLE };
void quiz_open(int mode, int module);
int  quiz_active(void);
void quiz_frame(void);
int  quiz_last_ok(void);
int  quiz_last_module(void);
int  quiz_last_diff(void);

/* other screens */
void desktop_draw(void);
void newgame_draw(void);
void elevator_enter(void);
void elevator_draw(void);
void review_enter(void);
void review_draw(void);
void event_enter(void);
void event_draw(void);
void shop_enter(void);
void shop_draw(void);
void dead_enter(void);
void dead_draw(void);
void win_enter(void);
void win_draw(void);
void train_draw(void);
void hof_draw(void);
void help_draw(void);
void about_draw(void);
void bosskey_draw(void);
void start_run(const char *name, unsigned int seed, int career);
void next_floor(void);
void after_battle_win(void);
void go_dead(void);
void clock_draw(Rect r);
void draw_boss_portrait(int x, int y, int scale, unsigned int seed);

#endif
