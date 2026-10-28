/* Software renderer: 8-bit indexed 640x480, Windows 16-colour palette. */
#include "ht.h"

int font_advance(int ch);
int font_draw(int x, int y, int ch, int c, int sc);

unsigned char g_fb[SCR_W * SCR_H];

const unsigned char g_pal[16][3] = {
    {0,0,0},{128,0,0},{0,128,0},{128,128,0},{0,0,128},{128,0,128},{0,128,128},{192,192,192},
    {128,128,128},{255,0,0},{0,255,0},{255,255,0},{0,0,255},{255,0,255},{0,255,255},{255,255,255}
};

static int cx0 = 0, cy0 = 0, cx1 = SCR_W, cy1 = SCR_H;

void gfx_clip(Rect r) {
    cx0 = MAX(0, r.x); cy0 = MAX(0, r.y);
    cx1 = MIN(SCR_W, r.x + r.w); cy1 = MIN(SCR_H, r.y + r.h);
}
void gfx_unclip(void) { cx0 = 0; cy0 = 0; cx1 = SCR_W; cy1 = SCR_H; }
Rect gfx_getclip(void) { return R(cx0, cy0, cx1 - cx0, cy1 - cy0); }

void gfx_pixel(int x, int y, int c) {
    if (x < cx0 || y < cy0 || x >= cx1 || y >= cy1) return;
    g_fb[y * SCR_W + x] = (unsigned char)c;
}

void gfx_rect(int x, int y, int w, int h, int c) {
    int x1 = x + w, y1 = y + h;
    if (x < cx0) x = cx0;
    if (y < cy0) y = cy0;
    if (x1 > cx1) x1 = cx1;
    if (y1 > cy1) y1 = cy1;
    if (x >= x1 || y >= y1) return;
    for (; y < y1; y++) memset(g_fb + y * SCR_W + x, c, (size_t)(x1 - x));
}

void gfx_hline(int x, int y, int w, int c) { gfx_rect(x, y, w, 1, c); }
void gfx_vline(int x, int y, int h, int c) { gfx_rect(x, y, 1, h, c); }

void gfx_frame(int x, int y, int w, int h, int c) {
    if (w <= 0 || h <= 0) return;
    gfx_hline(x, y, w, c); gfx_hline(x, y + h - 1, w, c);
    gfx_vline(x, y, h, c); gfx_vline(x + w - 1, y, h, c);
}

void gfx_line(int x0, int y0, int x1, int y1, int c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        int e2;
        gfx_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_circle(int cx, int cy, int r, int c, int fill) {
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        if (fill) {
            gfx_hline(cx - x, cy + y, 2 * x + 1, c); gfx_hline(cx - x, cy - y, 2 * x + 1, c);
            gfx_hline(cx - y, cy + x, 2 * y + 1, c); gfx_hline(cx - y, cy - x, 2 * y + 1, c);
        } else {
            gfx_pixel(cx + x, cy + y, c); gfx_pixel(cx - x, cy + y, c);
            gfx_pixel(cx + x, cy - y, c); gfx_pixel(cx - x, cy - y, c);
            gfx_pixel(cx + y, cy + x, c); gfx_pixel(cx - y, cy + x, c);
            gfx_pixel(cx + y, cy - x, c); gfx_pixel(cx - y, cy - x, c);
        }
        y++;
        if (err < 0) err += 2 * y + 1; else { x--; err += 2 * (y - x) + 1; }
    }
}

void gfx_dither(int x, int y, int w, int h, int c1, int c2, int mode) {
    int i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++) {
            int on;
            int px = x + i, py = y + j;
            if (mode == 0) on = (px + py) & 1;
            else if (mode == 1) on = ((px & 1) == 0 && (py & 1) == 0);
            else on = !((px & 1) == 0 && (py & 1) == 0);
            gfx_pixel(px, py, on ? c1 : c2);
        }
}

void gfx_bevel(int x, int y, int w, int h, int tl, int br) {
    gfx_hline(x, y, w, tl); gfx_vline(x, y, h, tl);
    gfx_hline(x, y + h - 1, w, br); gfx_vline(x + w - 1, y, h, br);
    gfx_pixel(x + w - 1, y, br); gfx_pixel(x, y + h - 1, br);
}

/* ------------------------------------------------------------ text */
int gfx_text_w(const char *s) {
    int w = 0;
    for (; *s; s++) {
        if (*s == ' ') w += 3; else w += font_advance((unsigned char)*s);
    }
    return w > 0 ? w - 1 : 0;
}
int gfx_text_bw(const char *s) { return gfx_text_w(s) + 1; }

int gfx_text(int x, int y, const char *s, int c) {
    for (; *s; s++) {
        if (*s == ' ') x += 3; else x += font_draw(x, y, (unsigned char)*s, c, 1);
    }
    return x;
}
int gfx_text_b(int x, int y, const char *s, int c) {
    gfx_text(x + 1, y, s, c);
    return gfx_text(x, y, s, c) + 1;
}
void gfx_text_sh(int x, int y, const char *s, int c, int shadow) {
    gfx_text(x + 1, y + 1, s, shadow);
    gfx_text(x, y, s, c);
}
void gfx_text_c(Rect r, const char *s, int c, int bold) {
    int w = bold ? gfx_text_bw(s) : gfx_text_w(s);
    int x = r.x + (r.w - w) / 2, y = r.y + (r.h - 7) / 2;
    if (bold) gfx_text_b(x, y, s, c); else gfx_text(x, y, s, c);
}
int gfx_text_big_w(const char *s, int scale) {
    int w = 0;
    for (; *s; s++) w += (*s == ' ') ? 3 * scale : font_advance((unsigned char)*s) * scale;
    return w;
}
void gfx_text_big(int x, int y, const char *s, int c, int scale) {
    for (; *s; s++) {
        if (*s == ' ') x += 3 * scale; else x += font_draw(x, y, (unsigned char)*s, c, scale) * scale;
    }
}

/* ------------------------------------------------------------ sprites */
static int spr_col(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
    if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
    return -1;
}

void gfx_spr_scaled(int x, int y, const Spr *s, int sc) {
    int i, j;
    for (j = 0; j < s->h; j++) {
        const char *row = s->rows[j];
        for (i = 0; i < s->w && row[i]; i++) {
            int c = spr_col(row[i]);
            if (c < 0) continue;
            if (sc == 1) gfx_pixel(x + i, y + j, c);
            else gfx_rect(x + i * sc, y + j * sc, sc, sc, c);
        }
    }
}
void gfx_spr(int x, int y, const Spr *s) { gfx_spr_scaled(x, y, s, 1); }

void gfx_spr_mono(int x, int y, const Spr *s, int col) {
    int i, j;
    for (j = 0; j < s->h; j++) {
        const char *row = s->rows[j];
        for (i = 0; i < s->w && row[i]; i++)
            if (spr_col(row[i]) >= 0) gfx_pixel(x + i, y + j, col);
    }
}

void gfx_spr_flip(int x, int y, const Spr *s) {
    int i, j;
    for (j = 0; j < s->h; j++) {
        const char *row = s->rows[j];
        int len = (int)strlen(row);
        for (i = 0; i < s->w && i < len; i++) {
            int c = spr_col(row[i]);
            if (c >= 0) gfx_pixel(x + (s->w - 1 - i), y + j, c);
        }
    }
}
