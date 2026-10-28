/* UI core: input, Windows 3.1 widgets, modal dialogs, menu bar, desktop, misc screens. */
#include "ui.h"
#include <time.h>
#include <math.h>

int g_mx = -999, g_my = -999, g_down, g_press, g_release, g_rpress, g_dbl;
unsigned int g_time;
int g_dt = 16, g_frame;
Run g_run; Battle g_bt; Shop g_shop;
Screen g_scr = SC_DESKTOP;
int g_in_run;

static int in_mx = 320, in_my = 240, in_down, in_press, in_release, in_rpress, in_dbl;
static unsigned int last_click_t; static int last_cx, last_cy;
static int keyq[16], keyn, charq[16], charn;
static int active_id;
static int quit_flag, boss_on, busy_until;
static int ret_scr = SC_DESKTOP, resume_scr = -1, overlay_on;
static int pending_action;
static int shake_amt;

/* ------------------------------------------------------------------ input entry points */
void ht_mouse_move(int x, int y) { in_mx = CLAMP(x, 0, SCR_W - 1); in_my = CLAMP(y, 0, SCR_H - 1); }
void ht_mouse_button(int which, int down) {
    if (which == 0) {
        if (down) {
            in_press = 1; in_down = 1;
            if (g_time - last_click_t < 450 && abs(in_mx - last_cx) < 6 && abs(in_my - last_cy) < 6) { in_dbl = 1; last_click_t = 0; }
            else { last_click_t = g_time; last_cx = in_mx; last_cy = in_my; }
        } else { in_release = 1; in_down = 0; }
    } else if (which == 1 && down) in_rpress = 1;
}
void ht_key(int vk, int down) {
    if (!down) return;
    if (vk == KEY_F9) { boss_on = !boss_on; audio_enable(boss_on ? 0 : g_cfg.sound, boss_on ? 0 : g_cfg.music); return; }
    if (boss_on) { boss_on = 0; audio_enable(g_cfg.sound, g_cfg.music); return; }
    if (vk == KEY_F11) { plat_fullscreen(); return; }
    if (keyn < 16) keyq[keyn++] = vk;
}
void ht_char(int ch) { if (ch >= 32 && ch < 127 && charn < 16) charq[charn++] = ch; }
void ht_wheel(int d) { (void)d; }
int ht_cursor_hidden(void) { return 1; }
int ht_want_quit(void) { return quit_flag; }

int ui_key(int vk) {
    int i;
    for (i = 0; i < keyn; i++) if (keyq[i] == vk) { memmove(keyq + i, keyq + i + 1, sizeof(int) * (size_t)(keyn - i - 1)); keyn--; return 1; }
    return 0;
}
int ui_char(void) {
    int c;
    if (!charn) return 0;
    c = charq[0]; memmove(charq, charq + 1, sizeof(int) * (size_t)(charn - 1)); charn--;
    return c;
}
int ui_hover(Rect r) { return pt_in(r, g_mx, g_my); }

/* ------------------------------------------------------------------ chrome */
void ui_raised(Rect r) {
    gfx_rect(r.x, r.y, r.w, r.h, COL_FACE);
    gfx_hline(r.x, r.y, r.w, COL_HI); gfx_vline(r.x, r.y, r.h, COL_HI);
    gfx_hline(r.x, r.y + r.h - 1, r.w, COL_SH); gfx_vline(r.x + r.w - 1, r.y, r.h, COL_SH);
}
void ui_sunken(Rect r) {
    gfx_hline(r.x, r.y, r.w, COL_SH); gfx_vline(r.x, r.y, r.h, COL_SH);
    gfx_hline(r.x, r.y + r.h - 1, r.w, COL_HI); gfx_vline(r.x + r.w - 1, r.y, r.h, COL_HI);
}
void ui_groove(Rect r) {
    gfx_frame(r.x, r.y, r.w, r.h, COL_SH);
    gfx_frame(r.x + 1, r.y + 1, r.w - 2, r.h - 2, COL_HI);
}

void ui_desktop_bg(void) {
    int x, y;
    switch (g_cfg.wallpaper) {
    case 1: /* Arcade: dark teal tiles */
        gfx_rect(0, 0, SCR_W, SCR_H, C_TEAL);
        for (y = 0; y < SCR_H; y += 16) for (x = 0; x < SCR_W; x += 16) {
            if (((x + y) >> 4) & 1) gfx_dither(x, y, 16, 16, C_TEAL, C_NAVY, 1);
        }
        break;
    case 2: /* Bricks */
        gfx_rect(0, 0, SCR_W, SCR_H, C_MAROON);
        for (y = 0; y < SCR_H; y += 8) {
            gfx_hline(0, y, SCR_W, C_BLACK);
            for (x = ((y >> 3) & 1) * 8; x < SCR_W; x += 16) gfx_vline(x, y, 8, C_BLACK);
        }
        break;
    case 3: /* Chess */
        for (y = 0; y < SCR_H; y += 24) for (x = 0; x < SCR_W; x += 24) gfx_rect(x, y, 24, 24, (((x + y) / 24) & 1) ? C_GRAY : C_SILVER);
        break;
    default: gfx_rect(0, 0, SCR_W, SCR_H, C_TEAL);
    }
}

static void minmax_buttons(int x, int y) {
    /* Win 3.1 had one-piece up/down arrow buttons at the right of the title bar */
    Rect up = R(x, y, 18, 18), dn = R(x - 18, y, 18, 18);
    int i;
    ui_raised(up); ui_raised(dn);
    gfx_frame(up.x, up.y, 18, 18, C_BLACK); gfx_frame(dn.x, dn.y, 18, 18, C_BLACK);
    ui_raised(R(up.x + 1, up.y + 1, 16, 16)); ui_raised(R(dn.x + 1, dn.y + 1, 16, 16));
    for (i = 0; i < 4; i++) { gfx_hline(up.x + 9 - i, up.y + 6 + i, 1 + 2 * i, C_BLACK); gfx_hline(dn.x + 9 - (3 - i), dn.y + 6 + i, 1 + 2 * (3 - i), C_BLACK); }
    gfx_hline(up.x + 6, up.y + 11, 7, C_BLACK);
    (void)i;
}

Rect ui_window(Rect r, const char *title, int active) {
    int tb = active ? C_NAVY : C_WHITE, fg = active ? C_WHITE : C_BLACK, w;
    gfx_rect(r.x, r.y, r.w, r.h, COL_FACE);
    gfx_frame(r.x, r.y, r.w, r.h, C_BLACK);
    gfx_hline(r.x + 1, r.y + 1, r.w - 2, COL_HI); gfx_vline(r.x + 1, r.y + 1, r.h - 2, COL_HI);
    gfx_hline(r.x + 1, r.y + r.h - 2, r.w - 2, COL_SH); gfx_vline(r.x + r.w - 2, r.y + 1, r.h - 2, COL_SH);
    /* title bar */
    gfx_rect(r.x + 4, r.y + 4, r.w - 8, 18, tb);
    gfx_frame(r.x + 4, r.y + 4, r.w - 8, 18, C_BLACK);
    /* control-menu box */
    ui_raised(R(r.x + 4, r.y + 4, 18, 18)); gfx_frame(r.x + 4, r.y + 4, 18, 18, C_BLACK);
    ui_raised(R(r.x + 5, r.y + 5, 16, 16));
    gfx_rect(r.x + 8, r.y + 11, 10, 3, C_WHITE); gfx_frame(r.x + 8, r.y + 11, 10, 3, C_BLACK);
    minmax_buttons(r.x + r.w - 22, r.y + 4);
    w = gfx_text_bw(title);
    { int cx0 = r.x + 22, cx1 = r.x + r.w - 40, tx = cx0 + (cx1 - cx0 - w) / 2;
      gfx_clip(R(cx0, r.y + 4, cx1 - cx0, 18));
      gfx_text_b(MAX(tx, cx0 + 2), r.y + 9, title, fg);
      gfx_unclip(); }
    return R(r.x + 4, r.y + 23, r.w - 8, r.h - 27);
}

/* menu bar placement (set by ui_app / desktop) */
static int mb_on, mb_seen, mb_x, mb_y, mb_w;

Rect ui_app(const char *title, int menubar) {
    Rect c = ui_window(R(0, 0, SCR_W, SCR_H), title, 1);
    if (menubar) {
        mb_on = 1; mb_x = c.x; mb_y = c.y; mb_w = c.w;
        c.y += 19; c.h -= 19;
    }
    return c;
}

int ui_button_def(Rect r, const char *label, int enabled) {
    int id = r.x * 1024 + r.y + 1, hover = ui_hover(r), pressed, click = 0;
    if (enabled && hover && g_press) { active_id = id; audio_sfx(SFX_CLICK); }
    pressed = (active_id == id && g_down && hover);
    gfx_rect(r.x, r.y, r.w, r.h, COL_FACE);
    gfx_frame(r.x, r.y, r.w, r.h, C_BLACK);
    if (pressed) {
        gfx_hline(r.x + 1, r.y + 1, r.w - 2, COL_SH); gfx_vline(r.x + 1, r.y + 1, r.h - 2, COL_SH);
        gfx_text_c(R(r.x + 1, r.y + 1, r.w, r.h), label, C_BLACK, 0);
    } else {
        gfx_hline(r.x + 1, r.y + 1, r.w - 2, COL_HI); gfx_vline(r.x + 1, r.y + 1, r.h - 2, COL_HI);
        gfx_hline(r.x + 1, r.y + r.h - 2, r.w - 2, COL_SH); gfx_vline(r.x + r.w - 2, r.y + 1, r.h - 2, COL_SH);
        gfx_hline(r.x + 2, r.y + r.h - 3, r.w - 4, COL_SH); gfx_vline(r.x + r.w - 3, r.y + 2, r.h - 4, COL_SH);
        if (enabled) gfx_text_c(r, label, C_BLACK, 0);
    }
    if (!enabled) { gfx_text_c(R(r.x + 1, r.y + 1, r.w, r.h), label, C_WHITE, 0); gfx_text_c(r, label, C_GRAY, 0); }
    if (enabled && g_release && active_id == id && hover) click = 1;
    return click;
}
int ui_button(Rect r, const char *label, int enabled) { return ui_button_def(r, label, enabled); }

void ui_bar(Rect r, int val, int max, int color, const char *label) {
    int w = max > 0 ? (r.w - 4) * CLAMP(val, 0, max) / max : 0;
    gfx_rect(r.x, r.y, r.w, r.h, C_WHITE);
    ui_sunken(r);
    gfx_rect(r.x + 2, r.y + 2, w, r.h - 4, color);
    gfx_frame(r.x + 1, r.y + 1, r.w - 2, r.h - 2, C_BLACK);
    gfx_rect(r.x + 2, r.y + 2, w, r.h - 4, color);
    if (label) {
        Rect lr = R(r.x, r.y, r.w, r.h);
        gfx_text_c(R(lr.x + 1, lr.y + 1, lr.w, lr.h), label, C_WHITE, 1);
        gfx_text_c(lr, label, C_BLACK, 1);
    }
}

int ui_wrap(Rect r, const char *text, int color, int draw) {
    char line[200];
    const char *p = text;
    int y = r.y, lw = 0, ll = 0;
    line[0] = 0;
    while (*p) {
        const char *e = p;
        int ww, wl;
        char word[80];
        if (*p == '\n') {
            if (draw) gfx_text(r.x, y, line, color);
            y += 10; line[0] = 0; ll = 0; lw = 0; p++; continue;
        }
        while (*e && *e != ' ' && *e != '\n') e++;
        wl = (int)(e - p); if (wl > 78) wl = 78;
        memcpy(word, p, (size_t)wl); word[wl] = 0;
        ww = gfx_text_w(word);
        if (ll > 0 && lw + 3 + ww > r.w) {
            if (draw) gfx_text(r.x, y, line, color);
            y += 10; line[0] = 0; ll = 0; lw = 0;
        }
        if (ll > 0) { line[ll++] = ' '; lw += 4; }
        if (ll + wl < (int)sizeof line - 2) { memcpy(line + ll, word, (size_t)wl); ll += wl; line[ll] = 0; lw += ww + 1; }
        p = e;
        if (*p == ' ') p++;
    }
    if (ll > 0) { if (draw) gfx_text(r.x, y, line, color); y += 10; }
    return y - r.y;
}

void ui_icon_label(int x, int y, const Spr *s, int scale, const char *label, int selected) {
    int w = gfx_text_w(label), iw = s->w * scale;
    gfx_spr_scaled(x + (80 - iw) / 2, y, s, scale);
    if (selected) { gfx_rect(x + (80 - w) / 2 - 2, y + s->h * scale + 3, w + 4, 11, C_NAVY); gfx_text(x + (80 - w) / 2, y + s->h * scale + 5, label, C_WHITE); }
    else gfx_text(x + (80 - w) / 2, y + s->h * scale + 5, label, C_BLACK);
}

void ui_textfield(Rect r, char *buf, int max, int focus, int numeric) {
    int len = (int)strlen(buf), c;
    gfx_rect(r.x, r.y, r.w, r.h, C_WHITE);
    ui_sunken(r); gfx_frame(r.x + 1, r.y + 1, r.w - 2, r.h - 2, C_WHITE);
    if (focus) {
        while ((c = ui_char()) != 0) {
            if (numeric && (c < '0' || c > '9')) continue;
            if (len < max - 1) { buf[len++] = (char)c; buf[len] = 0; audio_sfx(SFX_TYPE); }
        }
        if (ui_key(KEY_BACK) && len > 0) { buf[--len] = 0; audio_sfx(SFX_TYPE); }
    }
    gfx_clip(R(r.x + 2, r.y + 1, r.w - 4, r.h - 2));
    { int tw = gfx_text_w(buf), tx = r.x + 4;
      if (tw > r.w - 10) tx -= tw - (r.w - 10);
      gfx_text(tx, r.y + (r.h - 7) / 2, buf, C_BLACK);
      if (focus && ((g_time / 400) & 1)) gfx_rect(tx + tw + 1, r.y + 3, 1, r.h - 6, C_BLACK); }
    gfx_unclip();
}

/* ------------------------------------------------------------------ Stan the Stapler */
void ui_stan(int x, int y, int mood, int blink) {
    int lift = 0, cy, i;
    unsigned int t = g_time;
    if (mood == 1) lift = 2; else if (mood == 2) lift = 0; else lift = ((t / 700) & 3) == 0 ? 1 : 0;
    gfx_spr(x, y + 17, &S_STAN_BASE);
    gfx_spr(x, y, &S_STAN_ARM[lift]);
    for (i = 0; i < 2; i++) {
        int ex = x + 9 + i * 11, off = (int)((lift * (ex - x)) / 39);
        int ey, px, py, dx, dy;
        cy = y + 8 + 6 + lift - off;
        ey = cy - 2; px = ex; py = ey;
        dx = CLAMP((g_mx - ex) / 60, -1, 1); dy = CLAMP((g_my - ey) / 60, -1, 1);
        if (mood == 2) dy = 1;
        gfx_circle(ex, ey, 3, C_WHITE, 1); gfx_circle(ex, ey, 3, C_BLACK, 0);
        if (blink && ((t / 100) % 40) == 0) { gfx_hline(ex - 2, ey, 5, C_BLACK); }
        else gfx_rect(px + dx, py + dy, 2, 2, C_BLACK);
    }
    cy = y + 8 + 6 + lift - (lift * 15) / 39;
    if (mood == 1) { gfx_hline(x + 14, cy + 3, 9, C_BLACK); gfx_pixel(x + 13, cy + 2, C_BLACK); gfx_pixel(x + 23, cy + 2, C_BLACK); }
    else if (mood == 2) { gfx_hline(x + 15, cy + 3, 7, C_BLACK); gfx_pixel(x + 14, cy + 4, C_BLACK); gfx_pixel(x + 22, cy + 4, C_BLACK); }
    else gfx_hline(x + 15, cy + 3, 8, C_BLACK);
}

/* ------------------------------------------------------------------ dialogs */
static struct { int active, id, icon, nb, shown_at; char title[40], text[300], lbl[3][16]; } dlg;
static int dlg_done_id = -1, dlg_done_res = -1;

void ui_dialog(int id, const char *title, const char *text, int icon, const char *b0, const char *b1, const char *b2) {
    memset(&dlg, 0, sizeof dlg);
    dlg.active = 1; dlg.id = id; dlg.icon = icon; dlg.shown_at = (int)g_time;
    snprintf(dlg.title, sizeof dlg.title, "%s", title);
    snprintf(dlg.text, sizeof dlg.text, "%s", text);
    snprintf(dlg.lbl[0], sizeof dlg.lbl[0], "%s", b0 ? b0 : "OK"); dlg.nb = 1;
    if (b1) { snprintf(dlg.lbl[1], sizeof dlg.lbl[1], "%s", b1); dlg.nb = 2; }
    if (b2) { snprintf(dlg.lbl[2], sizeof dlg.lbl[2], "%s", b2); dlg.nb = 3; }
    audio_sfx(icon == 3 ? SFX_ERROR : SFX_DING);
}
int ui_dialog_active(void) { return dlg.active; }
int ui_dialog_result(int id) {
    if (dlg_done_id == id) { int r = dlg_done_res; dlg_done_id = -1; dlg_done_res = -1; return r; }
    return -1;
}
static void dialog_frame(void) {
    int tw = 250, th, w, h, x, y, i, bw = 74, total;
    Rect c, tr;
    if (!dlg.active) return;
    th = ui_wrap(R(0, 0, tw, 0), dlg.text, C_BLACK, 0);
    if (th < 32) th = 32;
    w = 340; h = th + 96; x = (SCR_W - w) / 2; y = (SCR_H - h) / 2 - 10;
    gfx_rect(x + 5, y + 5, w, h, C_BLACK);
    c = ui_window(R(x, y, w, h), dlg.title, 1);
    if (dlg.icon > 0) gfx_spr_scaled(c.x + 14, c.y + 14, &S_MB[CLAMP(dlg.icon - 1, 0, 3)], 2);
    tr = R(c.x + (dlg.icon > 0 ? 62 : 16), c.y + 16, tw, th);
    ui_wrap(tr, dlg.text, C_BLACK, 1);
    total = dlg.nb * bw + (dlg.nb - 1) * 10;
    for (i = 0; i < dlg.nb; i++) {
        Rect br = R(c.x + (c.w - total) / 2 + i * (bw + 10), c.y + c.h - 34, bw, 24);
        if (ui_button_def(br, dlg.lbl[i], 1)) { dlg_done_id = dlg.id; dlg_done_res = i; dlg.active = 0; return; }
    }
    if (ui_key(KEY_RETURN)) { dlg_done_id = dlg.id; dlg_done_res = 0; dlg.active = 0; }
    else if (ui_key(KEY_ESC)) { dlg_done_id = dlg.id; dlg_done_res = dlg.nb - 1; dlg.active = 0; }
}

/* ------------------------------------------------------------------ menu bar */
typedef struct { const char *label; int act, en, chk, sep; } MItem;
static int menu_open = -1, menu_hot = -1;
static const char *menu_titles[2] = {"Game", "Help"};

static int menu_build(int m, MItem *it) {
    int n = 0;
    if (m == 0) {
        it[n++] = (MItem){"New Game...", ACT_NEW, 1, 0, 0};
        it[n++] = (MItem){"Hall of Fame", ACT_HOF, 1, 0, 0};
        it[n++] = (MItem){"", 0, 0, 0, 1};
        it[n++] = (MItem){"Sound Effects", ACT_SND, 1, g_cfg.sound, 0};
        it[n++] = (MItem){"Music", ACT_MUS, 1, g_cfg.music, 0};
        it[n++] = (MItem){"Change Wallpaper", ACT_WALL, 1, 0, 0};
        it[n++] = (MItem){"", 0, 0, 0, 1};
        it[n++] = (MItem){"Back to Program Manager", ACT_DESKTOP, g_scr != SC_DESKTOP, 0, 0};
        it[n++] = (MItem){"Exit Windows...", ACT_EXIT, 1, 0, 0};
    } else {
        it[n++] = (MItem){"How to Play      F1", ACT_HELP, 1, 0, 0};
        it[n++] = (MItem){"Orientation Video (tutorial)", ACT_TUT, 1, 0, 0};
        it[n++] = (MItem){"Break Room Arcade", ACT_ARCADE, !tut_active(), 0, 0};
        it[n++] = (MItem){"", 0, 0, 0, 1};
        it[n++] = (MItem){"About Hostile Takeover...", ACT_ABOUT, 1, 0, 0};
    }
    return n;
}
static Rect menu_title_rect(int i) {
    int x = mb_x + 2, k;
    for (k = 0; k < i; k++) x += gfx_text_w(menu_titles[k]) + 16;
    return R(x, mb_y, gfx_text_w(menu_titles[i]) + 12, 18);
}
static Rect menu_drop_rect(int m, int n, MItem *it) {
    int w = 80, i, h = 6;
    Rect t = menu_title_rect(m);
    for (i = 0; i < n; i++) { w = MAX(w, gfx_text_w(it[i].label) + 36); h += it[i].sep ? 6 : 14; }
    return R(t.x, t.y + 18, w, h);
}

static void menu_input(void) {
    MItem it[14];
    int n, i;
    if (!mb_seen) { menu_open = -1; return; }
    if (in_press) {
        for (i = 0; i < 2; i++) if (pt_in(menu_title_rect(i), in_mx, in_my)) {
            menu_open = (menu_open == i) ? -1 : i; in_press = 0; in_release = 0; in_dbl = 0; audio_sfx(SFX_CLICK); return;
        }
    }
    if (menu_open >= 0) {
        int y;
        Rect d;
        n = menu_build(menu_open, it);
        d = menu_drop_rect(menu_open, n, it);
        menu_hot = -1;
        y = d.y + 3;
        for (i = 0; i < n; i++) {
            int ih = it[i].sep ? 6 : 14;
            if (!it[i].sep && pt_in(R(d.x + 2, y, d.w - 4, ih), in_mx, in_my)) menu_hot = i;
            y += ih;
        }
        /* hovering over another title switches menus */
        for (i = 0; i < 2; i++) if (i != menu_open && pt_in(menu_title_rect(i), in_mx, in_my) && in_down) menu_open = i;
        if (in_press || in_release) {
            if (menu_hot >= 0 && it[menu_hot].en && (in_press || g_time - last_click_t > 150)) { pending_action = it[menu_hot].act; menu_open = -1; audio_sfx(SFX_CLICK); }
            else if (in_press && menu_hot < 0 && !pt_in(d, in_mx, in_my)) menu_open = -1;
            in_press = 0; in_release = 0; in_dbl = 0; in_rpress = 0;
        }
        if (keyn > 0) { int j; for (j = 0; j < keyn; j++) if (keyq[j] == KEY_ESC) { menu_open = -1; memmove(keyq + j, keyq + j + 1, sizeof(int) * (size_t)(keyn - j - 1)); keyn--; break; } }
    }
}

static void menu_draw(void) {
    MItem it[12];
    int n, i, y;
    Rect d;
    mb_seen = mb_on;
    if (!mb_on) return;
    gfx_rect(mb_x, mb_y, mb_w, 19, COL_FACE);
    gfx_hline(mb_x, mb_y + 18, mb_w, C_BLACK);
    for (i = 0; i < 2; i++) {
        Rect t = menu_title_rect(i);
        if (menu_open == i) { gfx_rect(t.x, t.y, t.w, 18, C_NAVY); gfx_text(t.x + 6, t.y + 6, menu_titles[i], C_WHITE); }
        else gfx_text(t.x + 6, t.y + 6, menu_titles[i], C_BLACK);
    }
    if (menu_open < 0) { mb_on = 0; return; }
    n = menu_build(menu_open, it);
    d = menu_drop_rect(menu_open, n, it);
    gfx_rect(d.x + 2, d.y + 2, d.w, d.h, C_BLACK);
    gfx_rect(d.x, d.y, d.w, d.h, C_WHITE); gfx_frame(d.x, d.y, d.w, d.h, C_BLACK);
    y = d.y + 3;
    for (i = 0; i < n; i++) {
        if (it[i].sep) { gfx_hline(d.x + 2, y + 3, d.w - 4, C_GRAY); y += 6; continue; }
        if (i == menu_hot && it[i].en) gfx_rect(d.x + 2, y, d.w - 4, 14, C_NAVY);
        { int fg = !it[i].en ? C_GRAY : (i == menu_hot ? C_WHITE : C_BLACK);
          if (it[i].chk) { gfx_line(d.x + 7, y + 7, d.x + 9, y + 9, fg); gfx_line(d.x + 9, y + 9, d.x + 14, y + 3, fg); gfx_line(d.x + 7, y + 8, d.x + 9, y + 10, fg); gfx_line(d.x + 9, y + 10, d.x + 14, y + 4, fg); }
          gfx_text(d.x + 20, y + 4, it[i].label, fg); }
        y += 14;
    }
    mb_on = 0;
}

int ui_take_action(void) { int a = pending_action; pending_action = 0; return a; }
void ui_menu_request(int act) { pending_action = act; }

/* input swallowing for scripted (tutorial) drive of a real screen */
static int sw_on, sw_g[7], sw_keyn, sw_keys[16], sw_charn, sw_chars[16];
void ui_swallow(int on) {
    if (on && !sw_on) {
        sw_g[0] = g_mx; sw_g[1] = g_my; sw_g[2] = g_down; sw_g[3] = g_press; sw_g[4] = g_release; sw_g[5] = g_rpress; sw_g[6] = g_dbl;
        g_mx = g_my = -999; g_down = g_press = g_release = g_rpress = g_dbl = 0;
        sw_keyn = keyn; memcpy(sw_keys, keyq, sizeof keyq); keyn = 0;
        sw_charn = charn; memcpy(sw_chars, charq, sizeof charq); charn = 0;
        sw_on = 1;
    } else if (!on && sw_on) {
        g_mx = sw_g[0]; g_my = sw_g[1]; g_down = sw_g[2]; g_press = sw_g[3]; g_release = sw_g[4]; g_rpress = sw_g[5]; g_dbl = sw_g[6];
        keyn = sw_keyn; memcpy(keyq, sw_keys, sizeof keyq); charn = sw_charn; memcpy(charq, sw_chars, sizeof charq);
        sw_on = 0;
    }
}
void ui_active_reset(void) { active_id = 0; }
void ui_raw_input(int *mx, int *my, int *press, int *release, int *down) { *mx = in_mx; *my = in_my; *press = in_press; *release = in_release; *down = in_down; }

/* ------------------------------------------------------------------ screen switching */
static int is_run_screen(int s) { return s == SC_ELEVATOR || s == SC_PLACE || s == SC_BATTLE || s == SC_REVIEW || s == SC_EVENT || s == SC_SHOP; }
static int disk_pending;
int ui_game_screen(void) {
    if (!g_in_run) return -1;
    if (is_run_screen(g_scr)) return g_scr;
    if (overlay_on && is_run_screen(ret_scr)) return ret_scr;
    if (is_run_screen(resume_scr)) return resume_scr;
    return -1;
}
void ui_set_resume(int scr, int from_disk) { resume_scr = scr; disk_pending = from_disk; }
int ui_can_resume(void) { return g_in_run && is_run_screen(resume_scr); }
static void restore_music(void) {
    switch (g_scr) {
    case SC_ELEVATOR: audio_music(MUS_ELEVATOR, g_run.seed + (unsigned)g_run.floor); break;
    case SC_PLACE: case SC_BATTLE: audio_music(g_bt.fd.boss ? MUS_BOSS : MUS_BATTLE, g_run.seed + (unsigned)g_run.floor); break;
    case SC_REVIEW: audio_music(MUS_SHOP, g_run.seed + 99u); break;
    case SC_EVENT: audio_music(MUS_MENU, g_run.seed + 7u); break;
    case SC_SHOP: audio_music(MUS_SHOP, g_run.seed + (unsigned)g_run.floor); break;
    default: break;
    }
}
void ui_resume_game(void) {
    if (!ui_can_resume()) return;
    g_scr = (Screen)resume_scr; resume_scr = -1; overlay_on = 0;
    if (disk_pending) {
        disk_pending = 0;
        if (g_scr == SC_ELEVATOR) elevator_enter();
        else if (g_scr == SC_PLACE || g_scr == SC_BATTLE) battle_resume();
    }
    restore_music();
    busy_until = (int)g_time + 150;
}
void scr_go(Screen s) {
    g_scr = s; overlay_on = 0;
    if (s != SC_TRAIN && s != SC_DESKTOP && s != SC_TUTORIAL && s != SC_ARCADE && s != SC_GAME && s != SC_HOF && s != SC_HELP && s != SC_ABOUT) resume_scr = -1;
    disk_pending = 0;
    busy_until = (int)g_time + 250;
    switch (s) {
    case SC_ELEVATOR: elevator_enter(); break;
    case SC_BATTLE: case SC_PLACE: battle_enter(0); break;
    case SC_REVIEW: review_enter(); break;
    case SC_EVENT: event_enter(); break;
    case SC_SHOP: shop_enter(); break;
    case SC_DEAD: dead_enter(); break;
    case SC_WIN: win_enter(); break;
    case SC_DESKTOP: case SC_NEWGAME: case SC_TRAIN: case SC_TUTORIAL: case SC_ARCADE: case SC_GAME: audio_music(MUS_MENU, g_time); break;
    default: break;
    }
    if (is_run_screen(s) && g_in_run) game_save();
}
static void overlay(Screen s) {
    if (!overlay_on) ret_scr = g_scr;
    overlay_on = 1; g_scr = s; busy_until = (int)g_time + 120;
}
static void overlay_close(void) { if (overlay_on) { g_scr = (Screen)ret_scr; overlay_on = 0; } else g_scr = SC_DESKTOP; }

enum { DLG_ABANDON = 1, DLG_EXIT, DLG_QUITRUN, DLG_GPF, DLG_EMPTY, DLG_NEWRUN, DLG_TUTQ };
static char pend_name[24]; static unsigned pend_seed; static int pend_career;
static void handle_action(int act) {
    if (tut_active() && (act == ACT_DESKTOP || act == ACT_QUITRUN || act == ACT_NEW || act == ACT_EXIT)) { tut_exit(); return; }
    switch (act) {
    case ACT_NEW: if (g_in_run) ui_dialog(DLG_ABANDON, "New Game", "Abandon your current career and start over?\nYour severance package will be $0.", 4, "OK", "Cancel", NULL); else scr_go(SC_NEWGAME); break;
    case ACT_TUT: if (!tut_active()) tut_start(); break;
    case ACT_ARCADE: if (!tut_active()) scr_go(SC_ARCADE); break;
    case ACT_HOF: overlay(SC_HOF); break;
    case ACT_HELP: overlay(SC_HELP); break;
    case ACT_ABOUT: overlay(SC_ABOUT); break;
    case ACT_SND: g_cfg.sound = !g_cfg.sound; audio_enable(g_cfg.sound, g_cfg.music); cfg_save(); if (g_cfg.sound) audio_sfx(SFX_DING); break;
    case ACT_MUS: g_cfg.music = !g_cfg.music; audio_enable(g_cfg.sound, g_cfg.music); cfg_save(); break;
    case ACT_WALL: g_cfg.wallpaper = (g_cfg.wallpaper + 1) % 4; cfg_save(); break;
    case ACT_EXIT: ui_dialog(DLG_EXIT, "Exit Windows", "This will end your Windows session.", 2, "OK", "Cancel", NULL); break;
    case ACT_DESKTOP: case ACT_QUITRUN:
        if (g_scr != SC_DESKTOP) { if (is_run_screen(g_scr)) resume_scr = g_scr; else if (overlay_on && is_run_screen(ret_scr)) resume_scr = ret_scr; g_scr = SC_DESKTOP; overlay_on = 0; }
        break;
    default: break;
    }
}
static void handle_dialog_results(void) {
    int r;
    if ((r = ui_dialog_result(DLG_ABANDON)) == 0) { g_in_run = 0; scr_go(SC_NEWGAME); }
    if ((r = ui_dialog_result(DLG_EXIT)) == 0) { ht_shutdown(); quit_flag = 1; plat_quit(); }
    if (ui_dialog_result(DLG_EMPTY) >= 0) { }
    if ((r = ui_dialog_result(DLG_TUTQ)) >= 0) {
        g_cfg.tut_seen = 1; cfg_save();
        if (r == 0) tut_start_then_new(); else start_run(pend_name, pend_seed, pend_career);
    }
    if ((r = ui_dialog_result(DLG_GPF)) >= 0) { }
    (void)r;
}

/* ------------------------------------------------------------------ desktop */
static int icon_sel = -1;
static char memo_text[160];
static unsigned int memo_t;

void clock_draw(Rect r) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    int cx = r.x + r.w / 2, cy = r.y + r.h / 2 - 6, rad = MIN(r.w, r.h) / 2 - 12, k;
    int h = tm ? tm->tm_hour : 10, m = tm ? tm->tm_min : 9, s = tm ? tm->tm_sec : 0;
    gfx_rect(r.x, r.y, r.w, r.h, C_WHITE);
    gfx_circle(cx, cy, rad, C_BLACK, 0);
    for (k = 0; k < 12; k++) {
        float a = (float)k * 3.14159265f / 6.0f;
        int x1 = cx + (int)((rad - 1) * (k % 3 == 0 ? 0.80f : 0.88f) * (float)sinf(a)), y1 = cy - (int)((rad - 1) * (k % 3 == 0 ? 0.80f : 0.88f) * (float)cosf(a));
        int x2 = cx + (int)((rad - 1) * (float)sinf(a)), y2 = cy - (int)((rad - 1) * (float)cosf(a));
        gfx_line(x1, y1, x2, y2, C_BLACK);
    }
    { float am = (m + s / 60.0f) * 3.14159265f / 30.0f, ah = ((h % 12) + m / 60.0f) * 3.14159265f / 6.0f, as = s * 3.14159265f / 30.0f;
      gfx_line(cx, cy, cx + (int)(rad * 0.55f * (float)sinf(ah)), cy - (int)(rad * 0.55f * (float)cosf(ah)), C_BLACK);
      gfx_line(cx + 1, cy, cx + 1 + (int)(rad * 0.55f * (float)sinf(ah)), cy - (int)(rad * 0.55f * (float)cosf(ah)), C_BLACK);
      gfx_line(cx, cy, cx + (int)(rad * 0.85f * (float)sinf(am)), cy - (int)(rad * 0.85f * (float)cosf(am)), C_BLACK);
      gfx_line(cx, cy, cx + (int)(rad * 0.9f * (float)sinf(as)), cy - (int)(rad * 0.9f * (float)cosf(as)), C_RED); }
    gfx_rect(cx - 1, cy - 1, 3, 3, C_BLACK);
}

void desktop_draw(void) {
    Rect pm, c, grp, gc, ck, memo, mc;
    static const char *labels[8] = {"New Game", "Resume Game", "Orientation", "Arcade", "Training", "Hall of Fame", "README.TXT", "Exit Windows"};
    static const int icon_spr[8] = {0, 6, 7, 8, 1, 2, 3, 5};
    int i, launch = -1;
    ui_desktop_bg();
    pm = R(14, 10, 612, 456);
    c = ui_window(pm, "Program Manager", 1);
    mb_on = 1; mb_x = c.x; mb_y = c.y; mb_w = c.w;
    c.y += 19; c.h -= 19;
    gfx_dither(c.x, c.y, c.w, c.h, C_SILVER, C_WHITE, 0);
    gfx_rect(c.x, c.y, c.w, c.h, C_SILVER); gfx_dither(c.x, c.y, c.w, c.h, C_SILVER, C_GRAY, 1);

    grp = R(c.x + 16, c.y + 14, 420, 250);
    gc = ui_window(grp, "Hostile Takeover", 1);
    gfx_rect(gc.x, gc.y, gc.w, gc.h, C_WHITE);
    for (i = 0; i < 8; i++) {
        int col = i % 4, row = i / 4, off = (i == 1 && !ui_can_resume());
        Rect ir = R(gc.x + 8 + col * 100, gc.y + 14 + row * 100, 90, 80);
        const char *lab = labels[i];
        const Spr *sp = &S_PM[icon_spr[i]];
        gfx_spr_scaled(ir.x + 29, ir.y + 4, sp, 2);
        if (off) gfx_dither(ir.x + 29, ir.y + 4, 32, 32, C_WHITE, C_SILVER, 0);
        { int w = gfx_text_w(lab), lx = ir.x + (90 - w) / 2;
          if (icon_sel == i) { gfx_rect(lx - 2, ir.y + 40, w + 4, 11, C_NAVY); gfx_text(lx, ir.y + 42, lab, C_WHITE); }
          else gfx_text(lx, ir.y + 42, lab, off ? C_GRAY : C_BLACK); }
        if (i == 2 && !g_cfg.tut_seen && ((g_time / 500) & 1)) { gfx_rect(ir.x + 60, ir.y, 24, 11, C_RED); gfx_text_b(ir.x + 62, ir.y + 2, "NEW", C_WHITE); }
        if (ui_hover(ir) && g_press) { if (icon_sel == i || g_dbl) launch = i; icon_sel = i; }
    }
    gfx_text(gc.x + 10, gc.y + gc.h - 14, "Click an icon, then click again (or press Enter) to run it.", C_GRAY);
    if (ui_key(KEY_RETURN) && icon_sel >= 0) launch = icon_sel;
    if (ui_key(KEY_LEFT) && icon_sel > 0) icon_sel--;
    if (ui_key(KEY_RIGHT) && icon_sel < 7) icon_sel++;
    if (ui_key(KEY_DOWN)) icon_sel = icon_sel < 0 ? 0 : MIN(7, icon_sel + 4);
    if (ui_key(KEY_UP)) icon_sel = icon_sel < 0 ? 0 : MAX(0, icon_sel - 4);

    /* clock */
    ck = ui_window(R(c.x + 448, c.y + 14, 140, 150), "Clock", 0);
    clock_draw(R(ck.x, ck.y, ck.w, ck.h));
    /* memo */
    if (!memo_text[0] || g_time - memo_t > 20000) { Rng r; rng_seed(&r, g_time); gen_memo(&r, memo_text, sizeof memo_text); memo_t = g_time; }
    memo = ui_window(R(c.x + 448, c.y + 174, 140, 128), "Notepad - MEMO.TXT", 0);
    gfx_rect(memo.x, memo.y, memo.w, memo.h, C_WHITE);
    mc = R(memo.x + 4, memo.y + 4, memo.w - 8, memo.h - 8);
    ui_wrap(mc, memo_text, C_BLACK, 1);
    /* stats panel */
    {
        Rect st = ui_window(R(c.x + 16, c.y + 276, 420, 106), "System Resources", 0);
        char b[140];
        gfx_rect(st.x, st.y, st.w, st.h, C_WHITE);
        snprintf(b, sizeof b, "Free memory: %d KB    System resources: %d%%", 4096 - 2 * (int)(g_frame % 40), 71 + (int)((g_time / 2000) % 4));
        gfx_text(st.x + 8, st.y + 8, b, C_BLACK);
        if (g_cfg.n > 0) { snprintf(b, sizeof b, "Best career so far: %s - $%ld severance", g_cfg.e[0].name, g_cfg.e[0].score); gfx_text(st.x + 8, st.y + 22, b, C_BLACK); }
        else gfx_text(st.x + 8, st.y + 22, "No careers on record. Your cubicle awaits.", C_BLACK);
        if (ui_can_resume()) { snprintf(b, sizeof b, "Career in progress: %s, floor %d, Morale %d. Double-click Resume Game.", g_run.name, floor_label(g_run.floor), g_run.morale); gfx_text_b(st.x + 8, st.y + 38, b, C_MAROON); }
        else if (!g_cfg.tut_seen) gfx_text_b(st.x + 8, st.y + 38, "First day? Double-click ORIENTATION - Stan explains it all!", C_RED);
        else gfx_text(st.x + 8, st.y + 38, "Synergex Edu-Soft Inc.  Learn! Compete! Leverage!", C_NAVY);
        gfx_text(st.x + 8, st.y + 54, "Your progress is saved automatically when you exit.", C_GRAY);
        gfx_text(st.x + 8, st.y + 70, "Press F1 for help, F9 if the boss walks by.", C_GRAY);
    }
    /* minimised groups */
    {
        static const char *gl[4] = {"Main", "Accessories", "Games", "StartUp"};
        for (i = 0; i < 4; i++) {
            Rect gr = R(c.x + 448 + (i % 2) * 70, c.y + 316 + (i / 2) * 36, 64, 32);
            gfx_rect(gr.x + 24, gr.y, 16, 14, C_WHITE); gfx_frame(gr.x + 24, gr.y, 16, 14, C_BLACK); gfx_rect(gr.x + 24, gr.y, 16, 3, C_NAVY);
            gfx_text(gr.x + (64 - gfx_text_w(gl[i])) / 2, gr.y + 18, gl[i], C_BLACK);
            if (ui_hover(gr) && g_dbl && i == 2) scr_go(SC_ARCADE);
            else if (ui_hover(gr) && g_dbl) ui_dialog(DLG_EMPTY, "Program Manager", "This group is empty.\nIt has always been empty.\nIt will always be empty.", 1, "OK", NULL, NULL);
        }
    }
    if (launch >= 0) {
        switch (launch) {
        case 0: if (g_in_run) ui_menu_request(ACT_NEW); else scr_go(SC_NEWGAME); break;
        case 1: if (ui_can_resume()) ui_resume_game(); else ui_dialog(DLG_EMPTY, "Resume Game", "No career in progress.\nStart a New Game first!", 1, "OK", NULL, NULL); break;
        case 2: tut_start(); break;
        case 3: scr_go(SC_ARCADE); break;
        case 4: scr_go(SC_TRAIN); break;
        case 5: overlay(SC_HOF); break;
        case 6: overlay(SC_HELP); break;
        default: ui_menu_request(ACT_EXIT); break;
        }
    }
}

/* ------------------------------------------------------------------ new game */
static char ng_name[24] = "Intern #4471", ng_seed[12];
static int ng_career = 0, ng_focus = 0, ng_init;

void start_run(const char *name, unsigned int seed, int career) {
    if (!g_cfg.tut_seen && !tut_active()) {
        snprintf(pend_name, sizeof pend_name, "%s", name); pend_seed = seed; pend_career = career;
        ui_dialog(DLG_TUTQ, "First day?", "Never played Hostile Takeover?\nStan can show you a full round played\nby two CPUs, explained step by step.\nIt takes about 3 minutes.", 4, "Show me!", "Skip it", NULL);
        return;
    }
    run_new(&g_run, name, seed, career);
    g_in_run = 1;
    snprintf(g_cfg.lastname, sizeof g_cfg.lastname, "%s", name);
    cfg_save();
    scr_go(SC_ELEVATOR);
}

void newgame_draw(void) {
    Rect w, c;
    int i;
    ui_desktop_bg();
    w = R(90, 50, 460, 380);
    c = ui_window(w, "New Employee Orientation", 1);
    if (!ng_init) { snprintf(ng_name, sizeof ng_name, "%s", g_cfg.lastname); snprintf(ng_seed, sizeof ng_seed, "%u", (unsigned)(g_time * 7u + 4471u) % 90000u + 10000u); ng_init = 1; }
    gfx_text_b(c.x + 14, c.y + 12, "Welcome to Vertex Global Industrial!", C_NAVY);
    gfx_text(c.x + 14, c.y + 28, "Please complete the following form in blue or black ink.", C_BLACK);
    gfx_text(c.x + 14, c.y + 50, "Employee name:", C_BLACK);
    { Rect f = R(c.x + 110, c.y + 46, 160, 16); if (ui_hover(f) && g_press) ng_focus = 0; ui_textfield(f, ng_name, 22, ng_focus == 0, 0); }
    gfx_text(c.x + 14, c.y + 72, "Employee ID:", C_BLACK);
    { Rect f = R(c.x + 110, c.y + 68, 100, 16); if (ui_hover(f) && g_press) ng_focus = 1; ui_textfield(f, ng_seed, 11, ng_focus == 1, 1); }
    if (ui_button(R(c.x + 220, c.y + 66, 60, 20), "Random", 1)) snprintf(ng_seed, sizeof ng_seed, "%u", (unsigned)(g_time * 31u + (unsigned)g_frame * 17u) % 900000u + 100000u);
    gfx_text_b(c.x + 14, c.y + 98, "Choose your career path:", C_BLACK);
    for (i = 0; i < N_CAREER; i++) {
        Rect rr = R(c.x + 14, c.y + 114 + i * 36, c.w - 28, 34);
        int sel = ng_career == i;
        if (ui_hover(rr) && g_press) { ng_career = i; audio_sfx(SFX_CLICK); }
        if (sel) { gfx_rect(rr.x, rr.y, rr.w, rr.h, C_NAVY); }
        gfx_circle(rr.x + 10, rr.y + 10, 5, C_WHITE, 1); gfx_circle(rr.x + 10, rr.y + 10, 5, C_BLACK, 0);
        if (sel) gfx_circle(rr.x + 10, rr.y + 10, 2, C_BLACK, 1);
        gfx_text_b(rr.x + 24, rr.y + 5, g_career_name[i], sel ? C_WHITE : C_BLACK);
        ui_wrap(R(rr.x + 24, rr.y + 16, rr.w - 30, 18), g_career_desc[i], sel ? C_YELLOW : C_GRAY, 1);
    }
    if (ui_key(KEY_TAB)) ng_focus ^= 1;
    if (ui_button_def(R(c.x + c.w - 190, c.y + c.h - 34, 84, 24), "Start Job", 1) || ui_key(KEY_RETURN)) {
        unsigned int sd = (unsigned int)strtoul(ng_seed, NULL, 10);
        if (!ng_name[0]) snprintf(ng_name, sizeof ng_name, "Intern #4471");
        ng_init = 0;
        start_run(ng_name, sd ? sd : 4471u, ng_career);
    }
    if (ui_button(R(c.x + c.w - 98, c.y + c.h - 34, 84, 24), "Cancel", 1) || ui_key(KEY_ESC)) { ng_init = 0; g_scr = SC_DESKTOP; }
}

/* ------------------------------------------------------------------ help / about / hall of fame */
static int help_page;
static const char *help_title[6] = {"The Goal", "Battle", "Training & Synergy", "Supplies & Perks", "Hazards", "Departments"};
static const char *help_text[6] = {
 "You are Intern #4471 at Vertex Global Industrial. Fight your way up 13 floors of the tower to the Executive Suite for the HOSTILE TAKEOVER.\n\nEach floor is a Battleship duel: you fire MEMOS at THEIR FLOOR, hunting hidden office Assets (printers, server racks, boardrooms...). The enemy fires at YOUR OFFICE. Sink every Asset to clear the floor.\n\nMORALE is your health. If it hits zero, YOU'RE FIRED and the run ends. Permadeath! Floors, fleets, hazards, events and quiz questions are all procedurally generated from your Employee ID.",
 "Click a cell on Their Floor to fire (costs 1 Action). Right-click to drop a flag. Before the battle, drag your Assets into place (click to pick up, click to drop, R or right-click rotates).\n\nWhen you run out of Actions, press END TURN (or Space). The enemy then fires back. Every hit costs Morale; sinking one of your Assets costs double. If your whole office is sunk you lose 40 Morale and must retake the floor.\n\nKeys: 1-4 answer quiz, R rotate, Esc cancel, Space end turn, T training, S synergize, F9 boss key, F11 fullscreen.",
 "Press MANDATORY TRAINING once per turn to answer a question from Stan the Stapler. Correct: +1 Action, cash and Synergy. Wrong: you lose Morale. Modules: Number Crunch, Word Works, Ledger Lab, World Office and Typing 101. Certifications increase the difficulty (and the payout!).\n\nWhen the SYNERGY meter hits 100, press SYNERGIZE! for a random absurd super-move: Pivot To Video, Disrupt The Paradigm, Leverage, Bandwidth, Team Building or a Thought Shower.",
 "The Break Room Shop sells Office Supplies (consumables), new Assets for your office, Corporate Perks and Mystery Boxes. You can Haggle (answer a question for a discount).\n\nYour own Assets give bonuses while they survive: the Fern heals, the Cooler builds Synergy, the Printer jams enemy shots, the Server Rack backs up one sinking per floor... Perks are permanent for the run. After each floor you pick 1 of 3.",
 "Hazards hide on Their Floor, revealed only when you shoot them:\n WET FLOOR - your memo slides to a neighbouring cell.\n FREE DONUTS - heals Morale.\n SUPPLY CLOSET - free loot.\n REPLY-ALL TRAP - answer fast or take damage.\n COFFEE MACHINE - +1 Action.\n\nThe Night Janitor can see them all. Random Office Incidents also strike mid-battle: coffee spills, surprise inspections, bagels and printer jams.",
 "Each department has its own AI and rule twist:\n RE-ORG: unhit Assets relocate every few turns.\n FIREWALL: some cells absorb the first shot.\n OVERTIME: the enemy gets extra shots.\n INJUNCTION: cells are locked in red tape.\n AUDIT: consecutive enemy hits snowball in damage.\n\nBosses wait on floors 4, 8, 12 and the Executive Suite (14). There is no floor 13. Beat the CEO and you may continue into endless OVERTIME."
};

void help_draw(void) {
    Rect c;
    int i;
    ui_desktop_bg();
    c = ui_window(R(60, 36, 520, 410), "Hostile Takeover Help - How to Play", 1);
    for (i = 0; i < 6; i++) {
        Rect t = R(c.x + 8 + i * 84, c.y + 8, 82, 20);
        if (ui_button(t, help_title[i] ? (i == 2 ? "Training" : i == 3 ? "Supplies" : help_title[i]) : "", 1)) help_page = i;
        if (i == help_page) gfx_frame(t.x + 2, t.y + 2, t.w - 4, t.h - 4, C_NAVY);
    }
    gfx_rect(c.x + 8, c.y + 36, c.w - 16, c.h - 78, C_WHITE); ui_sunken(R(c.x + 8, c.y + 36, c.w - 16, c.h - 78));
    gfx_text_b(c.x + 18, c.y + 46, help_title[help_page], C_NAVY);
    ui_wrap(R(c.x + 18, c.y + 62, c.w - 36, c.h - 100), help_text[help_page], C_BLACK, 1);
    if (ui_button(R(c.x + c.w - 98, c.y + c.h - 34, 84, 24), "Close", 1) || ui_key(KEY_ESC) || ui_key(KEY_F1)) overlay_close();
    if (ui_key(KEY_RIGHT) && help_page < 5) help_page++;
    if (ui_key(KEY_LEFT) && help_page > 0) help_page--;
}

void about_draw(void) {
    Rect c;
    char b[80];
    ui_desktop_bg();
    c = ui_window(R(120, 70, 400, 320), "About Hostile Takeover", 1);
    gfx_spr_scaled(c.x + 20, c.y + 16, &S_PM[0], 3);
    gfx_text_big(c.x + 84, c.y + 14, "HOSTILE", C_MAROON, 2);
    gfx_text_big(c.x + 84, c.y + 32, "TAKEOVER", C_NAVY, 2);
    gfx_text(c.x + 20, c.y + 72, "The Office Warfare Simulator for Windows(TM)", C_BLACK);
    gfx_text(c.x + 20, c.y + 86, "Version 1.0  (c) 1993 Synergex Edu-Soft, Inc.", C_BLACK);
    gfx_hline(c.x + 14, c.y + 104, c.w - 28, C_GRAY);
    gfx_text(c.x + 20, c.y + 114, "Roguelike. Battleship. Edutainment.", C_BLACK);
    gfx_text(c.x + 20, c.y + 128, "Learn! Compete! Leverage!", C_BLACK);
    gfx_text(c.x + 20, c.y + 148, "Ages 8-80.  Requires 1 MB RAM and a will to live.", C_GRAY);
    gfx_text(c.x + 20, c.y + 168, "Stan the Stapler is a trademark of nobody.", C_GRAY);
    snprintf(b, sizeof b, "Free memory: %d KB   Mode: 640x480x16", 3968 - (g_frame % 7));
    gfx_text(c.x + 20, c.y + 196, b, C_BLACK);
    gfx_text(c.x + 20, c.y + 210, "Written in C with the Win32 API. Compiled with GCC.", C_NAVY);
    if (g_in_run) { snprintf(b, sizeof b, "Current Employee ID: %u", g_run.seed); gfx_text(c.x + 20, c.y + 232, b, C_MAROON); }
    if (ui_button_def(R(c.x + c.w - 98, c.y + c.h - 34, 84, 24), "OK", 1) || ui_key(KEY_RETURN) || ui_key(KEY_ESC)) overlay_close();
}

void hof_draw(void) {
    Rect c;
    int i;
    char b[96];
    ui_desktop_bg();
    c = ui_window(R(60, 40, 520, 400), "Hall of Fame - Severance Packages", 1);
    gfx_spr_scaled(c.x + 12, c.y + 8, &S_PM[2], 2);
    gfx_text_big(c.x + 56, c.y + 12, "EMPLOYEES OF THE CENTURY", C_NAVY, 1);
    gfx_text(c.x + 56, c.y + 26, "(Past employees who briefly mattered)", C_GRAY);
    gfx_rect(c.x + 12, c.y + 50, c.w - 24, 280, C_WHITE); ui_sunken(R(c.x + 12, c.y + 50, c.w - 24, 280));
    gfx_text_b(c.x + 20, c.y + 56, "#  Name", C_BLACK); gfx_text_b(c.x + 230, c.y + 56, "Career", C_BLACK); gfx_text_b(c.x + 340, c.y + 56, "Floor", C_BLACK); gfx_text_b(c.x + 400, c.y + 56, "Severance", C_BLACK);
    gfx_hline(c.x + 16, c.y + 66, c.w - 32, C_BLACK);
    if (g_cfg.n == 0) gfx_text(c.x + 20, c.y + 80, "Nobody has been fired yet. Give it time.", C_GRAY);
    for (i = 0; i < g_cfg.n; i++) {
        HofEntry *e = &g_cfg.e[i];
        int y = c.y + 72 + i * 24, col = e->won ? C_MAROON : C_BLACK;
        snprintf(b, sizeof b, "%2d. %s", i + 1, e->name); gfx_text(c.x + 20, y, b, col);
        gfx_text(c.x + 230, y, g_career_name[CLAMP(e->career, 0, N_CAREER - 1)], col);
        snprintf(b, sizeof b, "%d%s", e->floor, e->won ? " (CEO!)" : ""); gfx_text(c.x + 340, y, b, col);
        snprintf(b, sizeof b, "$%ld", e->score); gfx_text(c.x + 400, y, b, col);
        snprintf(b, sizeof b, "ID %u", e->seed); gfx_text(c.x + 230, y + 9, b, C_GRAY);
    }
    if (ui_button_def(R(c.x + c.w - 98, c.y + c.h - 34, 84, 24), "Close", 1) || ui_key(KEY_ESC) || ui_key(KEY_RETURN)) overlay_close();
}

/* ------------------------------------------------------------------ boss key */
void bosskey_draw(void) {
    int r, c;
    Rect cl = ui_window(R(0, 0, SCR_W, SCR_H), "Microsoft Excel - BUDGET93.XLS", 1);
    gfx_rect(cl.x, cl.y, cl.w, cl.h, C_WHITE);
    gfx_text(cl.x + 8, cl.y + 6, "File  Edit  Formula  Format  Data  Options  Macro  Window  Help", C_BLACK);
    gfx_hline(cl.x, cl.y + 18, cl.w, C_BLACK);
    for (c = 0; c < 8; c++) { char b[4]; snprintf(b, sizeof b, "%c", 'A' + c); gfx_rect(cl.x + 30 + c * 72, cl.y + 24, 72, 12, C_SILVER); gfx_frame(cl.x + 30 + c * 72, cl.y + 24, 73, 12, C_GRAY); gfx_text(cl.x + 62 + c * 72, cl.y + 27, b, C_BLACK); }
    for (r = 0; r < 28; r++) {
        char b[24];
        snprintf(b, sizeof b, "%d", r + 1);
        gfx_rect(cl.x, cl.y + 36 + r * 13, 30, 13, C_SILVER); gfx_frame(cl.x, cl.y + 36 + r * 13, 31, 14, C_GRAY); gfx_text(cl.x + 8, cl.y + 39 + r * 13, b, C_BLACK);
        for (c = 0; c < 8; c++) {
            Rng g; rng_seed(&g, (unsigned)(r * 37 + c * 11 + 5));
            gfx_frame(cl.x + 30 + c * 72, cl.y + 36 + r * 13, 73, 14, C_SILVER);
            if (r == 0) { static const char *hd[8] = {"Q1","Q2","Q3","Q4","Total","Avg","Var","Notes"}; gfx_text_b(cl.x + 34 + c * 72, cl.y + 39, hd[c], C_BLACK); }
            else if (c < 7) { snprintf(b, sizeof b, "%d.%02d", 1000 + rng_n(&g, 90000) / 10, rng_n(&g, 100)); gfx_text(cl.x + 34 + c * 72, cl.y + 39 + r * 13, b, C_BLACK); }
        }
    }
    gfx_text(cl.x + 8, cl.y + cl.h - 12, "Ready", C_BLACK);
}

/* ------------------------------------------------------------------ main frame */
void ht_init(unsigned int seed) {
    g_rng.s = seed ? seed : 1;
    g_time = 0;
    cfg_load();
    audio_init(); audio_enable(g_cfg.sound, g_cfg.music);
    art_check();
    g_scr = SC_DESKTOP;
    game_load();
    audio_music(MUS_MENU, seed);
}

void ht_shutdown(void) {
    static int done;
    if (tut_active()) tut_exit();
    if (done) return;
    done = 1;
    cfg_save();
    if (g_in_run && ui_game_screen() >= 0) game_save(); else if (!g_in_run) save_clear();
    done = 0;
}

static void draw_cursor(void) {
    int x = in_mx, y = in_my;
    gfx_unclip();
    if ((int)g_time < busy_until) gfx_spr(x - 5, y - 6, &S_HOURGLASS);
    else gfx_spr(x, y, &S_CURSOR);
}

void ht_frame(int dt) {
    int raw_down, raw_press, raw_release, raw_rpress, raw_dbl, raw_mx, raw_my, saved_keyn = 0, saved_keys[16], saved_charn = 0, saved_chars[16], modal, i;
    g_dt = CLAMP(dt, 1, 100);
    g_time += (unsigned)g_dt; g_frame++;
    if (boss_on) { gfx_unclip(); bosskey_draw(); return; }
    gfx_unclip();
    { int a = ui_take_action(); if (a) handle_action(a); }
    modal = ui_dialog_active() || quiz_active();
    if (!modal) menu_input();
    raw_down = in_down; raw_press = in_press; raw_release = in_release; raw_rpress = in_rpress; raw_dbl = in_dbl; raw_mx = in_mx; raw_my = in_my;
    modal = modal || menu_open >= 0;
    if (modal) {
        g_press = g_release = g_rpress = g_dbl = g_down = 0; g_mx = g_my = -999;
        saved_keyn = keyn; memcpy(saved_keys, keyq, sizeof keyq); keyn = 0;
        saved_charn = charn; memcpy(saved_chars, charq, sizeof charq); charn = 0;
    } else {
        g_press = in_press; g_release = in_release; g_rpress = in_rpress; g_dbl = in_dbl; g_down = in_down; g_mx = in_mx; g_my = in_my;
    }
    if (!ui_dialog_active()) {
        /* global shortcuts (only when no modal) */
        if (!modal && ui_key(KEY_F1)) { if (g_scr == SC_HELP) overlay_close(); else ui_menu_request(ACT_HELP); }
        if (!modal && ui_key(KEY_F2)) ui_menu_request(ACT_SND);
    }
    battle_update_fx(g_dt);
    switch (g_scr) {
    case SC_DESKTOP: desktop_draw(); break;
    case SC_NEWGAME: newgame_draw(); break;
    case SC_ELEVATOR: elevator_draw(); break;
    case SC_PLACE: if (tut_active()) tut_frame(); else place_draw(); break;
    case SC_BATTLE: if (tut_active()) tut_frame(); else battle_draw(); break;
    case SC_ARCADE: arcade_draw(); break;
    case SC_GAME: game_draw(); break;
    case SC_REVIEW: review_draw(); break;
    case SC_EVENT: event_draw(); break;
    case SC_SHOP: shop_draw(); break;
    case SC_DEAD: dead_draw(); break;
    case SC_WIN: win_draw(); break;
    case SC_TRAIN: train_draw(); break;
    case SC_HOF: hof_draw(); break;
    case SC_HELP: help_draw(); break;
    case SC_ABOUT: about_draw(); break;
    default: desktop_draw(); break;
    }
    if (modal) {
        keyn = saved_keyn; memcpy(keyq, saved_keys, sizeof keyq);
        charn = saved_charn; memcpy(charq, saved_chars, sizeof charq);
    }
    /* menu overlay uses raw coordinates */
    g_mx = raw_mx; g_my = raw_my;
    menu_draw();
    /* restore raw input for modal layers */
    g_down = raw_down; g_press = raw_press; g_release = raw_release; g_rpress = raw_rpress; g_dbl = raw_dbl;
    if (menu_open >= 0 && !ui_dialog_active() && !quiz_active()) { /* menu consumed input in menu_input */ g_press = g_release = g_rpress = g_dbl = 0; }
    if (quiz_active()) quiz_frame();
    else if (ui_dialog_active()) dialog_frame();
    handle_dialog_results();
    /* end of frame bookkeeping */
    if (in_release) active_id = 0;
    in_press = in_release = in_rpress = in_dbl = 0;
    keyn = 0; charn = 0;
    (void)i; (void)shake_amt;
    draw_cursor();
}

void art_check(void) { }
