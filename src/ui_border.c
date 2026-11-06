/* The window-margin pattern: office carpet tiles scattered with desk supplies.
   Drawn once with the normal gfx primitives into a scratch copy of the frame buffer,
   then handed to the platform layer to tile around the game. */
#include "ui.h"

static void dot_clip(int x, int y) { gfx_pixel(x, y, C_SILVER); gfx_pixel(x + 1, y, C_SILVER); gfx_pixel(x, y + 1, C_GRAY); gfx_pixel(x + 1, y + 1, C_GRAY); }

void ht_border_tile(unsigned char *out) {
    static unsigned char save[SCR_W * SCR_H];
    int x, y, N = BORDER_TILE;
    memcpy(save, g_fb, sizeof save);
    gfx_unclip();
    /* carpet: dark navy with a woven lighter grid, 48px carpet squares */
    gfx_rect(0, 0, N, N, C_NAVY);
    for (y = 0; y < N; y += 2) for (x = (y / 2) % 2 ? 1 : 0; x < N; x += 2) gfx_pixel(x, y, C_BLUE);
    for (x = 0; x < N; x += 48) { gfx_vline(x, 0, N, C_BLACK); gfx_vline(x + 1, 0, N, C_BLUE); }
    for (y = 0; y < N; y += 48) { gfx_hline(0, y, N, C_BLACK); gfx_hline(0, y + 1, N, C_BLUE); }
    /* supplies, one per carpet square, nudged so the repeat is not obvious */
    gfx_spr(11, 12, &S_ITEM[I_STAPLER]);
    gfx_spr(62, 14, &S_ITEM[I_MUG]);
    gfx_spr(16, 62, &S_ITEM[I_STICKY]);
    gfx_spr(60, 60, &S_ITEM[I_CALC]);
    /* loose paperclips and a stray staple or two */
    dot_clip(38, 30); dot_clip(40, 31); dot_clip(82, 40); dot_clip(30, 84); dot_clip(32, 86); dot_clip(88, 88);
    gfx_pixel(52, 58, C_YELLOW); gfx_pixel(53, 58, C_YELLOW); gfx_pixel(8, 40, C_RED); gfx_pixel(9, 40, C_RED);
    for (y = 0; y < N; y++) memcpy(out + y * N, g_fb + y * SCR_W, (size_t)N);
    memcpy(g_fb, save, sizeof save);
}
