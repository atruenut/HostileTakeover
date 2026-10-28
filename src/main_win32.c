/* Win32 platform layer: window, 640x480x8 software frame buffer blitted with StretchDIBits,
   mouse/keyboard, waveOut audio, INI persistence next to the .exe. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include "ht.h"

#define BUFSAMPLES 1024
#define NBUF 4

static HWND g_hwnd;
static HINSTANCE g_inst;
static int g_quit, g_full;
static WINDOWPLACEMENT g_wp;
static DWORD g_style;
static HWAVEOUT g_wave;
static WAVEHDR g_hdr[NBUF];
static short g_abuf[NBUF][BUFSAMPLES];
static int g_audio_ok;

static struct { BITMAPINFOHEADER h; RGBQUAD pal[16]; } g_bmi;

/* ------------------------------------------------------------------ platform API used by the core */
unsigned int plat_time_ms(void) { return (unsigned int)GetTickCount(); }
void plat_quit(void) { g_quit = 1; if (g_hwnd) PostMessage(g_hwnd, WM_CLOSE, 0, 0); }

static void ini_path(const char *fname, char *out, int max) {
    char *p;
    GetModuleFileNameA(NULL, out, (DWORD)max);
    p = strrchr(out, '\\');
    if (p) *(p + 1) = 0; else out[0] = 0;
    strncat(out, fname, (size_t)(max - (int)strlen(out) - 1));
}
int plat_load(const char *fname, char *buf, int max) {
    char path[MAX_PATH];
    HANDLE h; DWORD got = 0;
    ini_path(fname, path, sizeof path);
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (!ReadFile(h, buf, (DWORD)max, &got, NULL)) got = 0;
    CloseHandle(h);
    return (int)got;
}
int plat_save(const char *fname, const char *buf, int len) {
    char path[MAX_PATH];
    HANDLE h; DWORD wr = 0;
    ini_path(fname, path, sizeof path);
    h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return -1;
    WriteFile(h, buf, (DWORD)len, &wr, NULL);
    CloseHandle(h);
    return 0;
}
void plat_fullscreen(void) {
    if (!g_full) {
        MONITORINFO mi;
        memset(&mi, 0, sizeof mi); mi.cbSize = sizeof mi;
        g_style = (DWORD)GetWindowLong(g_hwnd, GWL_STYLE);
        g_wp.length = sizeof g_wp;
        GetWindowPlacement(g_hwnd, &g_wp);
        GetMonitorInfo(MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLong(g_hwnd, GWL_STYLE, (LONG)(g_style & ~(WS_OVERLAPPEDWINDOW)));
        SetWindowPos(g_hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_full = 1;
    } else {
        SetWindowLong(g_hwnd, GWL_STYLE, (LONG)g_style);
        SetWindowPlacement(g_hwnd, &g_wp);
        SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_full = 0;
    }
}

/* ------------------------------------------------------------------ audio */
static void audio_start(void) {
    WAVEFORMATEX wf;
    int i;
    memset(&wf, 0, sizeof wf);
    wf.wFormatTag = WAVE_FORMAT_PCM; wf.nChannels = 1; wf.nSamplesPerSec = AUDIO_RATE; wf.wBitsPerSample = 16;
    wf.nBlockAlign = 2; wf.nAvgBytesPerSec = AUDIO_RATE * 2;
    if (waveOutOpen(&g_wave, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) { g_audio_ok = 0; return; }
    for (i = 0; i < NBUF; i++) {
        memset(&g_hdr[i], 0, sizeof g_hdr[i]);
        g_hdr[i].lpData = (LPSTR)g_abuf[i]; g_hdr[i].dwBufferLength = BUFSAMPLES * 2;
        audio_mix(g_abuf[i], BUFSAMPLES);
        waveOutPrepareHeader(g_wave, &g_hdr[i], sizeof g_hdr[i]);
        waveOutWrite(g_wave, &g_hdr[i], sizeof g_hdr[i]);
    }
    g_audio_ok = 1;
}
static void audio_pump(void) {
    int i;
    if (!g_audio_ok) return;
    for (i = 0; i < NBUF; i++) {
        if (g_hdr[i].dwFlags & WHDR_DONE) {
            audio_mix(g_abuf[i], BUFSAMPLES);
            g_hdr[i].dwFlags &= ~(DWORD)WHDR_DONE;
            waveOutWrite(g_wave, &g_hdr[i], sizeof g_hdr[i]);
        }
    }
}
static void audio_stop(void) {
    int i;
    if (!g_audio_ok) return;
    waveOutReset(g_wave);
    for (i = 0; i < NBUF; i++) waveOutUnprepareHeader(g_wave, &g_hdr[i], sizeof g_hdr[i]);
    waveOutClose(g_wave);
    g_audio_ok = 0;
}

/* ------------------------------------------------------------------ drawing */
static void present(HDC dc) {
    RECT rc;
    int cw, ch, scale, w, h, x, y;
    GetClientRect(g_hwnd, &rc);
    cw = rc.right - rc.left; ch = rc.bottom - rc.top;
    if (cw <= 0 || ch <= 0) return;
    scale = MIN(cw / SCR_W, ch / SCR_H);
    if (scale < 1) {            /* tiny window: fit, non-integer */
        w = cw; h = cw * SCR_H / SCR_W;
        if (h > ch) { h = ch; w = ch * SCR_W / SCR_H; }
    } else { w = SCR_W * scale; h = SCR_H * scale; }
    x = (cw - w) / 2; y = (ch - h) / 2;
    if (x > 0 || y > 0) {       /* letterbox in black */
        HBRUSH b = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RECT r1 = { 0, 0, cw, y }, r2 = { 0, y + h, cw, ch }, r3 = { 0, y, x, y + h }, r4 = { x + w, y, cw, y + h };
        FillRect(dc, &r1, b); FillRect(dc, &r2, b); FillRect(dc, &r3, b); FillRect(dc, &r4, b);
    }
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, x, y, w, h, 0, 0, SCR_W, SCR_H, g_fb, (BITMAPINFO *)&g_bmi, DIB_RGB_COLORS, SRCCOPY);
}

static int to_logical(int *x, int *y) {
    RECT rc;
    int cw, ch, scale, w, h, ox, oy;
    GetClientRect(g_hwnd, &rc);
    cw = rc.right; ch = rc.bottom;
    if (cw <= 0 || ch <= 0) return 0;
    scale = MIN(cw / SCR_W, ch / SCR_H);
    if (scale < 1) { w = cw; h = cw * SCR_H / SCR_W; if (h > ch) { h = ch; w = ch * SCR_W / SCR_H; } }
    else { w = SCR_W * scale; h = SCR_H * scale; }
    ox = (cw - w) / 2; oy = (ch - h) / 2;
    *x = (*x - ox) * SCR_W / MAX(1, w);
    *y = (*y - oy) * SCR_H / MAX(1, h);
    return 1;
}

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_MOUSEMOVE: { int x = (short)LOWORD(lp), y = (short)HIWORD(lp); if (to_logical(&x, &y)) ht_mouse_move(x, y); return 0; }
    case WM_LBUTTONDOWN: { int x = (short)LOWORD(lp), y = (short)HIWORD(lp); if (to_logical(&x, &y)) ht_mouse_move(x, y); SetCapture(hwnd); ht_mouse_button(0, 1); return 0; }
    case WM_LBUTTONUP: { int x = (short)LOWORD(lp), y = (short)HIWORD(lp); if (to_logical(&x, &y)) ht_mouse_move(x, y); ht_mouse_button(0, 0); ReleaseCapture(); return 0; }
    case WM_RBUTTONDOWN: ht_mouse_button(1, 1); return 0;
    case WM_RBUTTONUP: ht_mouse_button(1, 0); return 0;
    case WM_MOUSEWHEEL: ht_wheel((short)HIWORD(wp)); return 0;
    case WM_KEYDOWN: case WM_SYSKEYDOWN: {
        int vk = (int)wp;
        if (vk == VK_RETURN) vk = KEY_RETURN; else if (vk == VK_ESCAPE) vk = KEY_ESC; else if (vk == VK_SPACE) vk = KEY_SPACE;
        else if (vk == VK_BACK) vk = KEY_BACK; else if (vk == VK_TAB) vk = KEY_TAB;
        if (msg == WM_SYSKEYDOWN && vk == VK_F10) return 0;   /* don't let F10 activate the system menu */
        ht_key(vk, 1);
        if (msg == WM_SYSKEYDOWN) return 0;                   /* swallow Alt combos and F10 */
        break; }
    case WM_KEYUP: case WM_SYSKEYUP: ht_key((int)wp, 0); break;
    case WM_CHAR: if (wp >= 32 && wp < 127) ht_char((int)wp); return 0;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT && ht_cursor_hidden()) { SetCursor(NULL); return TRUE; }
        break;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps); present(dc); EndPaint(hwnd, &ps); return 0; }
    case WM_CLOSE: g_quit = 1; DestroyWindow(hwnd); return 0;
    case WM_DESTROY: g_quit = 1; PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {
    WNDCLASSA wc;
    RECT rc = { 0, 0, SCR_W * 2, SCR_H * 2 };
    DWORD style = WS_OVERLAPPEDWINDOW;
    MSG m;
    DWORD last, acc = 0;
    int i;
    (void)prev; (void)cmd;
    g_inst = inst;
    memset(&g_bmi, 0, sizeof g_bmi);
    g_bmi.h.biSize = sizeof(BITMAPINFOHEADER); g_bmi.h.biWidth = SCR_W; g_bmi.h.biHeight = -SCR_H;
    g_bmi.h.biPlanes = 1; g_bmi.h.biBitCount = 8; g_bmi.h.biCompression = BI_RGB; g_bmi.h.biClrUsed = 16;
    for (i = 0; i < 16; i++) { g_bmi.pal[i].rgbRed = g_pal[i][0]; g_bmi.pal[i].rgbGreen = g_pal[i][1]; g_bmi.pal[i].rgbBlue = g_pal[i][2]; }

    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = wndproc; wc.hInstance = inst; wc.lpszClassName = "HostileTakeoverWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hIcon = LoadIcon(inst, MAKEINTRESOURCE(1)); if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassA(&wc);
    AdjustWindowRect(&rc, style, FALSE);
    g_hwnd = CreateWindowA("HostileTakeoverWnd", "Hostile Takeover - The Office Warfare Simulator", style | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, inst, NULL);
    if (!g_hwnd) return 1;
    ShowWindow(g_hwnd, show);
    timeBeginPeriod(1);
    ht_init((unsigned int)GetTickCount() ^ (unsigned int)GetCurrentProcessId());
    audio_start();
    last = GetTickCount();
    while (!g_quit) {
        DWORD now, dt;
        HDC dc;
        while (PeekMessage(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_QUIT) g_quit = 1;
            TranslateMessage(&m); DispatchMessage(&m);
        }
        if (g_quit || ht_want_quit()) break;
        now = GetTickCount(); dt = now - last; last = now;
        if (dt > 100) dt = 100;
        acc += dt;
        audio_pump();
        if (acc >= 16) {
            ht_frame((int)acc); acc = 0;
            dc = GetDC(g_hwnd); present(dc); ReleaseDC(g_hwnd, dc);
        } else Sleep(1);
    }
    ht_shutdown();
    audio_stop();
    timeEndPeriod(1);
    return 0;
}
