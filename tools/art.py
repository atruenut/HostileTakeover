#!/usr/bin/env python3
"""Sprite generator for HOSTILE TAKEOVER.  Writes src/art.c and tools/preview.png.
Palette chars: 0 black 1 maroon 2 green 3 olive 4 navy 5 purple 6 teal 7 silver 8 gray
               9 red a lime b yellow c blue d fuchsia e aqua f white   ('.' = transparent)"""
import sys, os, math
HERE = os.path.dirname(os.path.abspath(__file__))
PAL = [(0,0,0),(128,0,0),(0,128,0),(128,128,0),(0,0,128),(128,0,128),(0,128,128),(192,192,192),
       (128,128,128),(255,0,0),(0,255,0),(255,255,0),(0,0,255),(255,0,255),(0,255,255),(255,255,255)]

class Canvas:
    def __init__(s, w, h):
        s.w, s.h = w, h
        s.g = [['.'] * w for _ in range(h)]
    def px(s, x, y, c):
        if 0 <= x < s.w and 0 <= y < s.h: s.g[y][x] = c
    def rect(s, x, y, w, h, c):
        for j in range(h):
            for i in range(w): s.px(x + i, y + j, c)
    def frame(s, x, y, w, h, c):
        for i in range(w): s.px(x + i, y, c); s.px(x + i, y + h - 1, c)
        for j in range(h): s.px(x, y + j, c); s.px(x + w - 1, y + j, c)
    def line(s, x0, y0, x1, y1, c):
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx + dy
        while True:
            s.px(x0, y0, c)
            if x0 == x1 and y0 == y1: break
            e2 = 2 * err
            if e2 >= dy: err += dy; x0 += sx
            if e2 <= dx: err += dx; y0 += sy
    def disc(s, cx, cy, r, c):
        for y in range(s.h):
            for x in range(s.w):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r: s.px(x, y, c)
    def ring(s, cx, cy, r, c, t=0.9):
        for y in range(s.h):
            for x in range(s.w):
                d = math.hypot(x - cx, y - cy)
                if r - t <= d <= r + 0.35: s.px(x, y, c)
    def blit(s, x, y, txt):
        for j, row in enumerate(txt.strip('\n').split('\n')):
            for i, ch in enumerate(row):
                if ch != '.' and ch != ' ': s.px(x + i, y + j, ch)
    def rows(s): return [''.join(r) for r in s.g]

def ascii_spr(txt, w, h, name):
    rows = txt.strip('\n').split('\n')
    out = []
    for j in range(h):
        r = rows[j] if j < len(rows) else ''
        r = r.replace(' ', '0')   # stray typo spaces -> outline colour
        if len(r) != w and r:
            print('WARN %s row %d len %d != %d' % (name, j, len(r), w))
        r = (r + '.' * w)[:w]
        out.append(r)
    if len(rows) != h: print('WARN %s has %d rows, wanted %d' % (name, len(rows), h))
    return out

SPR = {}   # name -> (w,h,rows)
def add(name, rows): SPR[name] = (len(rows[0]), len(rows), rows)
def add_ascii(name, txt, w=16, h=16): add(name, ascii_spr(txt, w, h, name))

# ------------------------------------------------------------------ ASSETS
add_ascii('A_FERN', """
................
.....22222......
...2222a22222...
..222a2222a222..
..22a222a2222a2.
..2222a222a2222.
...222a22222a2..
....22222a222...
.....2222222....
.......22.......
...1999999991...
....19999991....
....19999991....
.....199991.....
.....111111.....
................
""")
add_ascii('A_COOLER', """
................
....88888888....
....8effeee8....
....8ecccce8....
....8cccccc8....
....8cccccc8....
....8cccccc8....
....88888888....
...0000000000...
...0ffffffff0...
...0f7c779770...
...0f77777770...
...0f77777770...
...0888888880...
...0000000000...
................
""")
add_ascii('A_PRINTER', """
................
.....ffffff.....
.....f0f0ff.....
.....ffffff.....
..000000000000..
..077777777770..
..07ffffffff70..
..0777777a7770..
..077777777770..
..000000000000..
...0ffffffff0...
...0f0f0f0ff0...
...0ffffffff0...
...0000000000...
................
................
""")
add_ascii('A_CUBICLE', """
................
..cccccccccccc..
..c0000000000c..
..c0eeeeeeee0c..
..c0eeffeeee0c..
..c0eeeeeeee0c..
..c0eeeeeeee0c..
..c0000000000c..
..ccccc00ccccc..
..cccc0880cccc..
3333333333333333
3bbbbbbbbbbbbbb3
3333333333333333
..00........00..
..00........00..
................
""")
add_ascii('A_VENDING', """
.00000000000000.
.09999999999990.
.09eeeeeeee9990.
.09ebecebed9990.
.09eeeeeeee9b90.
.09ecbeabde9c90.
.09eeeeeeee9990.
.09999999999990.
.09900000000990.
.09900000000990.
.09999999999990.
.09999999999990.
.00000000000000.
..00........00..
................
................
""")
add_ascii('A_DESK', """
................
................
................
.00000000000000.
.0bbbbbbbbbbbb0.
.0bbbbbbbbbbbb0.
.00000000000000.
.03333333333330.
.0333b33333b330.
.03333333333330.
.0333b33333b330.
.03333333333330.
.00000000000000.
.00..........00.
.00..........00.
................
""")
add_ascii('A_RACK', """
..000000000000..
..088888888880..
..0a9a80000080..
..000000000000..
..088888888880..
..09a98000000 0.
..000000000000..
..088888888880..
..0a9a80000080..
..000000000000..
..088888888880..
..09a9800000 80.
..000000000000..
..00........00..
..00........00..
................
""")
add_ascii('A_MAINFRAME', """
................
.00000000000000.
.07777777777770.
.07000077000070.
.070ff0770ff070.
.070ff0770ff070.
.07000077000070.
.07777777777770.
.07a9b7777777 0.
.07777777777770.
.07888888888870.
.07888888888870.
.00000000000000.
.00..........00.
................
................
""")
add_ascii('A_TABLE', """
................
...8.8.8.8.8....
................
..000000000000..
.03333333333330.
.03bbbbbbbbbb30.
.03333333333330.
.03333333333330.
.03333333333330.
..000000000000..
................
...8.8.8.8.8....
................
................
................
................
""")
add_ascii('A_SHREDDER', """
................
......ffff......
......ffff......
...0000000000...
...0888888880...
...0000000000...
...0777777770...
...0777777770...
...0888888880...
...0f8f8f8f80...
...0f8f8f8f80...
...0000000000...
................
................
................
................
""")
add_ascii('A_BOARDROOM', """
................
..b....b....b...
..bb..bbb..bb...
..bbbbbbbbbbb...
..bbb9bbb9bbb...
..bbbbbbbbbbb...
..000000000000..
................
..000000000000..
.01111111111110.
.01111111111110.
.00000000000000.
..00........00..
................
................
................
""")

# ------------------------------------------------------------------ HAZARDS
add_ascii('H_WET', """
................
.......00.......
......0bb0......
.....0bbbb0.....
.....0b00b0.....
....0bb00bb0....
....0bb00bb0....
...0bbb00bbb0...
...0bbbbbbbb0...
...0bbb00bbb0...
..0bbbb00bbbb0..
..0bbbbbbbbbb0..
..000000000000..
.cceeeeeeeeeecc.
..cceeeeeeeecc..
................
""")
c = Canvas(16, 16)
c.disc(7.5, 8, 6.4, '0'); c.disc(7.5, 8, 5.6, '3'); c.disc(7.5, 8, 5.0, 'd')
for (x, y, ch) in [(4, 5, 'f'), (9, 4, 'a'), (11, 7, 'e'), (5, 10, 'b'), (9, 11, 'f'), (7, 6, 'b')]: c.px(x, y, ch)
c.disc(7.5, 8, 1.7, '.'); c.ring(7.5, 8, 1.9, '3', 0.6)
c.disc(7.5, 8, 1.2, '.')
add('H_DONUT', c.rows())
add_ascii('H_CLOSET', """
................
................
..000000000000..
.03333bb3333330.
.03333bb3333330.
.00000bb0000000.
..03333bb333330.
..03ff3bb333330.
..03f03bb333330.
..03ff3bb333330.
..03333bb333330.
..000000000000..
................
................
................
................
""")
c = Canvas(16, 16)
c.blit(0, 3, """
.00000000000000.
.0ffffffffffff0.
.0f8ffffffff8f0.
.0ff8ffffff8ff0.
.0fff8ffff8fff0.
.0ffff8888ffff0.
.0ffffffffffff0.
.0ffffffffffff0.
.00000000000000.
""")
c.disc(12, 4, 3.3, '0'); c.disc(12, 4, 2.6, '9')
c.px(12, 2, 'f'); c.px(12, 3, 'f'); c.px(12, 5, 'f')
add('H_REPLYALL', c.rows())
add_ascii('H_COFFEE', """
................
......8..8......
.......8..8.....
......8..8......
................
..000000000000..
..0ffffffffff0..
..0f777777777000
..0f1111111117 0.
..0f1111111117 0.
..0f11111111700.
...0f1111111 70.
...00fffffff00..
....000000000...
................
................
""")

# ------------------------------------------------------------------ ITEMS
add_ascii('I_STAPLER', """
................
................
................
..000000000000..
.0999999999999 0
.09ff99999999 90
.09999999999990.
..000000000000..
.07777777777770.
.07888888888870.
..000000000000..
................
................
................
................
................
""")
c = Canvas(16, 16)
c.line(3, 12, 11, 4, '0'); c.line(4, 12, 12, 4, '0'); c.line(3, 11, 11, 3, '0'); c.line(4, 13, 12, 5, '0')
c.line(4, 12, 11, 5, 'b'); c.line(4, 11, 11, 4, 'b'); c.line(5, 12, 11, 6, 'b')
c.line(11, 3, 13, 1, '0'); c.line(12, 4, 14, 2, '0'); c.line(12, 3, 14, 1, '0')
c.line(11, 4, 13, 2, 'a'); c.line(12, 4, 13, 3, 'a')
c.px(2, 13, '0'); c.px(3, 14, '0'); c.px(2, 14, '0'); c.px(1, 13, 'b')
c.line(1, 14, 5, 14, 'b'); c.line(1, 15, 6, 15, 'b')
add('I_HIGHLIGHT', c.rows())
add_ascii('I_STICKY', """
................
..000000000000..
..0bbbbbbbbbb0..
..0bbbbbbbbbb0..
..0b00b0b0bbb0..
..0bbbbbbbbbb0..
..0b0b00bb0bb0..
..0bbbbbbbbbb0..
..0b000b00bbb0..
..0bbbbbbbbbb0..
..0bbbbbbb0000..
..0bbbbbbb08 0..
..000000000 00..
................
................
................
""")
add_ascii('I_FAX', """
................
.....000000.....
.....0ffff0.....
.....0f0f00.....
..0000ffff00000.
..07777777777 70
..07888888887 70
..0a7777777777 0
..07700000007770
..0777f0f0f77770
..07777777777770
..00000000000000
................
................
................
................
""")
c = Canvas(16, 16)
c.rect(5, 1, 6, 2, 'c'); c.line(5, 3, 7, 5, 'c'); c.line(10, 3, 8, 5, 'c')
c.rect(3, 5, 10, 10, '0'); c.rect(4, 6, 8, 8, 'f')
c.rect(5, 7, 3, 3, 'e'); c.px(6, 7, '0'); c.px(5, 8, '0'); c.px(7, 8, '0'); c.px(6, 9, '0')
c.rect(9, 7, 2, 1, '0'); c.rect(9, 9, 2, 1, '0'); c.rect(5, 11, 6, 1, '8'); c.rect(5, 12, 4, 1, '8')
add('I_SCANNER', c.rows())
add_ascii('I_WHITEOUT', """
................
.....cccc.......
....c0cc0c......
....cccccc......
....000000......
...0ffffff0.....
...0f9999f0.....
...0f0000f0.....
...0f9999f0.....
...0ffffff0.....
...0ffffff0.....
...0f7777f0.....
...0f7777f0.....
...00000000.....
................
................
""")
c = Canvas(16, 16)
c.rect(4, 3, 8, 12, '0'); c.rect(5, 4, 6, 10, 'e'); c.rect(5, 4, 2, 10, 'f'); c.rect(10, 4, 1, 10, 'c')
c.rect(5, 2, 6, 2, '8'); c.rect(6, 1, 4, 1, '8'); c.rect(5, 2, 6, 1, '7')
c.line(9, 5, 7, 8, 'b'); c.line(7, 8, 9, 9, 'b'); c.line(9, 9, 7, 12, 'b')
c.line(10, 5, 8, 8, 'b'); c.line(8, 8, 10, 9, 'b'); c.line(10, 9, 8, 12, 'b')
add('I_ENERGY', c.rows())
c = Canvas(16, 16)
c.blit(1, 3, """
..0000000...
.0fffffff00.
0ff0ffffff0f0
0f0fff0fffff0
.0ffff0ffff0.
.0f0ffff0fff0
..0ffffffff0.
...0000000...
""")
c.disc(11.5, 11.5, 3.6, '0'); c.disc(11.5, 11.5, 2.8, '9')
c.line(10, 10, 13, 13, 'f'); c.line(13, 10, 10, 13, 'f')
add('I_JAM', c.rows())
add_ascii('I_CALC', """
................
..000000000000..
..077777777770..
..0a0aaaaaaa00..
..0a0aaaaaaa00..
..077777777770..
..07f7f7f7f770..
..077777777770..
..07f7f7f7f970..
..077777777770..
..07f7f7f7f970..
..077777777770..
..07f7f7f7f970..
..000000000000..
................
................
""")
c = Canvas(16, 16)
c.rect(2, 6, 3, 5, '0'); c.rect(3, 7, 2, 3, '8')
c.line(5, 6, 12, 2, '0'); c.line(5, 10, 12, 14, '0'); c.line(12, 2, 12, 14, '0')
for k in range(6, 11): c.line(5, k, 12, 2 + (k - 6) * 3 // 1 if False else 8 + (k - 8) * 3, '9')
c.rect(5, 6, 7, 5, '9'); c.line(5, 6, 12, 3, '9'); c.line(5, 10, 12, 13, '9')
c.rect(5, 6, 7, 1, '9'); c.rect(5, 6, 3, 5, 'f')
c.line(5, 6, 12, 3, '0'); c.line(5, 10, 12, 13, '0'); c.line(12, 3, 12, 13, '0')
c.px(14, 6, 'b'); c.px(14, 8, 'b'); c.px(14, 10, 'b'); c.line(13, 5, 15, 4, 'b'); c.line(13, 11, 15, 12, 'b')
add('I_ALLHANDS', c.rows())
add_ascii('I_AIRPLANE', """
................
................
................
.............ff.
.........fffffc.
.....ffffffffc..
.fffffffffffcc..
..0ffffffffcc...
...0ffffffcc....
....0ffff0c.....
.....0ff0c......
......00........
................
................
................
................
""")
add_ascii('I_MUG', """
................
.....8...8......
......8...8.....
.....8...8......
................
..00000000000...
..0fffffffff0...
..0f1111111f000.
..0f1111111f0f0.
..0f1111111f0f0.
..0f1111111f00..
..0fffffffff0...
...0ffffffff0...
....00000000....
................
................
""")

# ------------------------------------------------------------------ MISC
c = Canvas(16, 16)
pts = [(7.5, 0.5), (9.2, 5), (14.5, 3.5), (11, 8), (15, 12.5), (9.5, 10.8), (7.5, 15.5), (5.5, 10.8), (0.5, 12.5), (4, 8), (0.5, 3.5), (5.8, 5)]
def poly(cv, pts, ch):
    for y in range(cv.h):
        for x in range(cv.w):
            inside = False
            j = len(pts) - 1
            for i in range(len(pts)):
                xi, yi = pts[i]; xj, yj = pts[j]
                if ((yi > y) != (yj > y)) and (x < (xj - xi) * (y - yi) / (yj - yi + 1e-9) + xi): inside = not inside
                j = i
            if inside: cv.px(x, y, ch)
poly(c, pts, '9')
sm = [(7.5, 3.5), (8.6, 6.5), (11.5, 6), (9.5, 8.3), (11, 11.5), (7.5, 9.7), (4, 11.5), (5.5, 8.3), (3.5, 6), (6.4, 6.5)]
poly(c, sm, 'b'); c.px(7, 7, 'f'); c.px(8, 7, 'f'); c.px(7, 8, 'f'); c.px(8, 8, 'f')
add('S_BOOM', c.rows())
c = Canvas(16, 16)
c.disc(7.5, 8, 5.2, '8'); c.disc(7.5, 8, 4.6, 'f'); c.disc(6.5, 7, 2.3, '7')
c.line(4, 5, 8, 9, '8'); c.line(10, 5, 6, 11, '8'); c.line(5, 10, 11, 8, '8'); c.line(7, 4, 9, 11, '7')
c.px(10, 6, '7'); c.px(5, 8, '7')
add('S_MISS', c.rows())
add_ascii('S_FLAG', """
................
.......0........
.......009......
.......09999....
.......0999999..
.......09999....
.......099......
.......0........
.......0........
.......0........
.......0........
......000.......
.....00000......
................
................
................
""")
add_ascii('S_NOTE', """
................
..000000000000..
..0bbbbbbbbbb0..
..0bbbb99bbbb0..
..0bbbb99bbbb0..
..0bbbb99bbbb0..
..0bbbb99bbbb0..
..0bbbbbbbbbb0..
..0bbbb99bbbb0..
..0bbbb99bbbb0..
..0bbbbbbbbbb0..
..0bbbbbbbbb00..
..0000000000 8..
................
................
................
""")
add_ascii('S_SHIELD', """
................
..000000000000..
..0cccccccccc0..
..0cccffffccc0..
..0cccffffccc0..
..0cffffffffc0..
..0cffffffffc0..
..0cccffffccc0..
..0cccffffccc0..
...0ccccccc c0..
...0ccccccc0....
....0ccccc0.....
.....0ccc0......
......000.......
................
................
""")
add_ascii('S_LOCK', """
................
......0000......
.....088880.....
.....08..80.....
.....08..80.....
....00000000....
....0bbbbbb0....
....0bbbbbb0....
....0bb00bb0....
....0bb00bb0....
....0bbbbbb0....
....0bbbbbb0....
....00000000....
................
................
................
""")
add_ascii('S_PLANE', """
................
................
.............ff.
.........fffffc.
.....ffffffffc..
.fffffffffffcc..
..0ffffffffcc...
...0ffffffcc....
....0ffff0c.....
.....0ff0c......
......00........
................
................
................
................
................
""")
c = Canvas(16, 16)
c.disc(7.5, 7.5, 7, '0'); c.disc(7.5, 7.5, 6.2, 'b')
c.rect(4, 5, 2, 3, '0'); c.rect(10, 5, 2, 3, '0')
for (x, y) in [(3, 10), (4, 11), (5, 12), (6, 12), (7, 12), (8, 12), (9, 12), (10, 12), (11, 11), (12, 10)]: c.px(x, y, '0')
c.px(5, 5, 'f') if False else None
add('S_SMILE', c.rows())
c = Canvas(16, 16)
c.disc(7.5, 7.5, 7, '0'); c.disc(7.5, 7.5, 6.2, '3'); c.disc(7.5, 7.5, 5.4, 'b')
c.blit(5, 3, """
..00.
.0000
0.00.
.000.
..00.
.000.
.00.0
0000.
..00.
""")
c.px(9, 3, '.')
add('S_COIN', c.rows())
c = Canvas(16, 16)
pts = [(7.5, 0.5), (9.6, 5.6), (15, 6), (10.8, 9.6), (12.3, 15), (7.5, 11.8), (2.7, 15), (4.2, 9.6), (0, 6), (5.4, 5.6)]
poly(c, pts, '0'); pts2 = [(7.5, 2.4), (9, 6.2), (13, 6.6), (10, 9.3), (11, 13), (7.5, 10.7), (4, 13), (5, 9.3), (2, 6.6), (6, 6.2)]
poly(c, pts2, 'b')
add('S_STAR', c.rows())

# ------------------------------------------------------------------ PROGRAM MANAGER ICONS (16x16, shown at 2x)
c = Canvas(16, 16)   # briefcase
c.rect(5, 2, 6, 2, '0'); c.rect(6, 3, 4, 1, '.'); c.rect(5, 3, 1, 3, '0'); c.rect(10, 3, 1, 3, '0')
c.rect(1, 5, 14, 10, '0'); c.rect(2, 6, 12, 8, '3'); c.rect(2, 6, 12, 1, 'b'); c.rect(2, 9, 12, 1, '0')
c.rect(7, 8, 2, 3, 'b'); c.rect(7, 8, 2, 3, 'b'); c.frame(7, 8, 2, 3, '0')
c.rect(2, 13, 12, 1, '1')
add('P_NEW', c.rows())
c = Canvas(16, 16)   # graduation cap
c.blit(0, 3, """
.......00.......
.....0000000....
...0001111000...
.00011111111000.
0001111111111100
.00011111111000.
...0001111000...
.....0000000....
.......00.......
""")
c.rect(3, 9, 1, 5, 'b'); c.rect(2, 13, 3, 2, 'b'); c.px(3, 15, 'b')
c.rect(5, 10, 6, 3, '0'); c.rect(6, 11, 4, 1, '1')
c = c
for y in range(16):
    for x in range(16):
        if c.g[y][x] == '1': c.g[y][x] = '4'
add('P_TRAIN', c.rows())
add_ascii('P_HOF', """
................
..000000000000..
..0bbbbbbbbbb0..
.00b0bbbbbb0b00.
.0b0bbfbbbb0b0b0
.0b00bbfbbb00b0.
..000bbbbbbb000.
....00bbbbb00...
.....00bbb00....
......00b0......
.......0b0......
......0bbb0.....
.....0bbbbb0....
.....0000000....
................
................
""")
add_ascii('P_README', """
................
..00000000000...
.0f0f0f0f0f0f0..
.0ffffffffffff0.
.0f000000fffff0.
.0ffffffffffff0.
.0f00000000fff0.
.0ffffffffffff0.
.0f0000000ffff0.
.0ffffffffffff0.
.0f00000000fff0.
.0ffffffffffff0.
.00000000000000.
................
................
................
""")
c = Canvas(16, 16)
c.disc(7.5, 7.5, 7, '0'); c.disc(7.5, 7.5, 6.2, 'c')
c.rect(7, 3, 2, 2, 'f'); c.rect(7, 6, 2, 6, 'f'); c.rect(6, 6, 1, 1, 'f'); c.rect(6, 11, 4, 1, 'f')
add('P_ABOUT', c.rows())
add_ascii('P_EXIT', """
................
....00000000....
...0333333330...
...0333333330...
...0338888330...
...0338888330...
...0333333330...
...0333333330...
...033333b330...
...033333b330...
...0333333330...
...0333333330...
...0333333330...
..000000000000..
................
................
""")

# ------------------------------------------------------------------ MESSAGEBOX ICONS
c = Canvas(16, 16)
c.disc(7.5, 7.5, 7, '0'); c.disc(7.5, 7.5, 6.2, 'c'); c.rect(7, 3, 2, 2, 'f'); c.rect(7, 6, 2, 6, 'f'); c.rect(6, 6, 1, 1, 'f'); c.rect(6, 11, 4, 1, 'f')
add('M_INFO', c.rows())
c = Canvas(16, 16)
poly(c, [(7.5, 0.5), (15.5, 15), (-0.5, 15)], '0'); poly(c, [(7.5, 2.8), (13.4, 13.6), (1.6, 13.6)], 'b')
c.rect(7, 6, 2, 4, '0'); c.rect(7, 11, 2, 2, '0')
add('M_WARN', c.rows())
c = Canvas(16, 16)
c.disc(7.5, 7.5, 7, '0'); c.disc(7.5, 7.5, 6.2, '9')
c.line(5, 5, 10, 10, 'f'); c.line(10, 5, 5, 10, 'f'); c.line(5, 6, 9, 10, 'f'); c.line(9, 5, 5, 9, 'f'); c.line(6, 5, 10, 9, 'f'); c.line(10, 6, 6, 10, 'f')
add('M_STOP', c.rows())
c = Canvas(16, 16)
c.disc(7.5, 6.5, 6.5, '0'); c.disc(7.5, 6.5, 5.7, 'e'); poly(c, [(3, 11), (3, 15), (8, 11)], '0'); poly(c, [(4, 11), (4, 13.5), (7, 11)], 'e')
c.blit(5, 3, """
.000.
0...0
....0
...0.
..0..
.....
..0..
""")
add('M_QUESTION', c.rows())

# ------------------------------------------------------------------ CURSORS
add_ascii('CURSOR', """
0...........
00..........
0f0.........
0ff0........
0fff0.......
0ffff0......
0fffff0.....
0ffffff0....
0fffffff0...
0ffffffff0..
0fffffffff0.
0ffffff00000
0ff0ff0.....
0f0.0ff0....
00..0ff0....
0....0ff0...
.....0ff0...
......00....
""", 12, 18)
add_ascii('HOURGLASS', """
0000000000.
0ffffffff0.
0ffffffff0.
.0f7bb7f0..
..0f7b70...
...0f70....
....00.....
....00.....
...0b70....
..0bbb70...
.0bbbbb70..
0bbbbbbb70.
0bbbbbbb70.
0000000000.
""", 11, 14)

# ------------------------------------------------------------------ STAN THE STAPLER
def stan_base():
    c = Canvas(40, 12)
    c.rect(1, 3, 38, 8, '0')                      # outline body
    c.rect(2, 4, 36, 6, '7')
    c.rect(2, 4, 36, 1, 'f')                       # highlight
    c.rect(2, 8, 36, 2, '8')                       # shadow
    c.rect(26, 5, 11, 2, '0')                      # staple slot
    c.rect(3, 10, 5, 2, '0'); c.rect(32, 10, 5, 2, '0')   # feet
    c.px(1, 3, '.'); c.px(38, 3, '.'); c.px(1, 10, '.'); c.px(38, 10, '.')
    return c
add('STAN_BASE', stan_base().rows())

def stan_arm(lift):
    W, H = 40, 22
    c = Canvas(W, H)
    base_y = 10 + lift
    # arm shape: from hinge (x=2) to nose (x=38), thickness 9 with rounded nose
    tmp = Canvas(W, 14)
    tmp.rect(1, 1, 38, 11, '0'); tmp.rect(2, 2, 36, 9, '9')
    tmp.rect(2, 2, 36, 2, 'f') if False else None
    tmp.rect(3, 3, 32, 1, 'f')
    tmp.rect(2, 9, 36, 2, '1')
    tmp.px(1, 1, '.'); tmp.px(38, 1, '.'); tmp.px(38, 11, '.'); tmp.px(1, 11, '.')
    tmp.rect(34, 8, 5, 4, '0'); tmp.rect(35, 9, 3, 2, '7')   # nose clamp
    for x in range(W):
        off = round(lift * x / (W - 1))
        for y in range(14):
            ch = tmp.g[y][x]
            if ch != '.': c.px(x, (H - 14 - lift) + y - 0 + (lift - off), ch)
    return c
for i, lift in enumerate((0, 3, 7)):
    add('STAN_ARM%d' % i, stan_arm(lift).rows())


# ------------------------------------------------------------------ extra program-manager icons
def p_resume():
    c = Canvas(16, 16)
    c.rect(1, 1, 14, 14, '0'); c.rect(2, 2, 12, 12, '7')
    c.rect(2, 2, 12, 1, 'f'); c.rect(2, 2, 1, 12, 'f'); c.rect(13, 3, 1, 11, '8'); c.rect(3, 13, 11, 1, '8')
    for x in range(5, 12):
        h = 6 - (x - 5)
        c.rect(x, 8 - h, 1, h * 2, '0')
    for x in range(6, 11):
        h = 5 - (x - 6)
        c.rect(x, 8 - h + 0, 1, max(h * 2 - 0, 1), 'a')
    return c
add('P_RESUME', p_resume().rows())

def p_tutorial():
    c = Canvas(16, 16)
    c.line(5, 4, 3, 0, '0'); c.line(10, 4, 12, 0, '0')
    c.rect(1, 3, 14, 11, '0'); c.rect(2, 4, 12, 9, '7'); c.rect(2, 4, 12, 1, 'f')
    c.rect(3, 5, 8, 7, '0'); c.rect(4, 6, 6, 5, 'e')
    c.rect(4, 8, 6, 2, '9'); c.rect(5, 8, 1, 1, 'f'); c.rect(8, 8, 1, 1, 'f'); c.rect(4, 6, 6, 1, 'f')
    c.rect(12, 6, 2, 2, '8'); c.rect(12, 9, 2, 2, '8')
    c.rect(3, 14, 2, 1, '0'); c.rect(11, 14, 2, 1, '0')
    return c
add('P_TUTORIAL', p_tutorial().rows())

def p_arcade():
    c = Canvas(16, 16)
    c.rect(1, 10, 14, 5, '0'); c.rect(2, 11, 12, 3, '7'); c.rect(2, 11, 12, 1, 'f')
    c.rect(10, 12, 3, 1, '9'); c.rect(3, 12, 3, 1, '8')
    c.rect(7, 6, 2, 5, '0'); c.rect(7, 6, 1, 5, '8')
    c.disc(8, 4, 4, '0'); c.disc(8, 4, 3, '9'); c.px(7, 3, 'f'); c.px(6, 4, 'f')
    return c
add('P_ARCADE', p_arcade().rows())

# ------------------------------------------------------------------ the application icon (Stan, front view)
def stan_icon():
    c = Canvas(32, 32)
    # base
    c.rect(2, 19, 28, 9, '0'); c.rect(3, 20, 26, 7, '7'); c.rect(3, 20, 26, 1, 'f'); c.rect(3, 25, 26, 2, '8')
    c.rect(4, 28, 5, 2, '0'); c.rect(23, 28, 5, 2, '0')
    c.px(2, 19, '.'); c.px(29, 19, '.')
    # arm / head
    c.rect(1, 4, 30, 15, '0'); c.rect(2, 5, 28, 13, '9')
    c.rect(3, 6, 24, 2, 'f'); c.rect(2, 15, 28, 3, '1')
    for (x, y) in ((1, 4), (30, 4)): c.px(x, y, '.')
    # eyes
    for ex in (10, 21):
        c.disc(ex, 11, 4, '0'); c.disc(ex, 11, 3, 'f'); c.rect(ex, 11, 2, 2, '0')
    # grin: the staple slot
    c.rect(8, 16, 16, 1, '0')
    for x in range(9, 23, 2): c.rect(x, 17, 1, 2, 'f')
    c.rect(8, 18, 16, 1, '0')
    return c
STAN_ICON = stan_icon()
add('ICON_STAN', STAN_ICON.rows())

def write_ico(path):
    import struct
    def img(c32, size):
        if size == 32: rows = c32.rows()
        else:
            rows = []
            for y in range(0, 32, 2):
                r = ''
                for x in range(0, 32, 2):
                    blk = [c32.g[y + j][x + i] for j in (0, 1) for i in (0, 1)]
                    solid = [b for b in blk if b != '.']
                    if len(solid) < 2: r += '.'
                    else:
                        # prefer dark outline, else most common
                        r += '0' if blk.count('0') >= 2 else max(set(solid), key=solid.count)
                rows.append(r)
        xor = bytearray(); andm = bytearray()
        for y in range(size - 1, -1, -1):
            r = rows[y]
            vals = [0 if ch == '.' else int(ch, 16) for ch in r]
            for i in range(0, size, 2): xor.append((vals[i] << 4) | vals[i + 1])
            bits = 0; bl = bytearray()
            for i in range(size):
                bits = (bits << 1) | (1 if r[i] == '.' else 0)
                if i % 8 == 7: bl.append(bits); bits = 0
            while len(bl) % 4: bl.append(0)
            andm += bl
        hdr = struct.pack('<IiiHHIIiiII', 40, size, size * 2, 1, 4, 0, len(xor) + len(andm), 0, 0, 16, 0)
        pal = b''.join(bytes((b, g, r, 0)) for (r, g, b) in PAL)
        return hdr + pal + bytes(xor) + bytes(andm)
    imgs = [(32, img(STAN_ICON, 32)), (16, img(STAN_ICON, 16))]
    out = struct.pack('<HHH', 0, 1, len(imgs))
    off = 6 + 16 * len(imgs)
    for size, data in imgs:
        out += struct.pack('<BBBBHHII', size, size, 16, 0, 1, 4, len(data), off); off += len(data)
    for _, data in imgs: out += data
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'wb').write(out)

# ================================================================== EMIT
def emit_c():
    out = ['/* GENERATED by tools/art.py - do not edit by hand */', '#include "ht.h"', '']
    for name in SPR:
        if name == 'ICON_STAN': continue
        w, h, rows = SPR[name]
        out.append('static const char *const R_%s[] = {' % name)
        for r in rows: out.append('    "%s",' % r)
        out.append('};')
    out.append('')
    def sp(n): w, h, _ = SPR[n]; return '{%d,%d,R_%s}' % (w, h, n)
    assets = ['A_FERN','A_COOLER','A_PRINTER','A_CUBICLE','A_VENDING','A_DESK','A_RACK','A_MAINFRAME','A_TABLE','A_SHREDDER','A_BOARDROOM']
    haz = [None, 'H_WET', 'H_DONUT', 'H_CLOSET', 'H_REPLYALL', 'H_COFFEE']
    items = ['I_STAPLER','I_HIGHLIGHT','I_STICKY','I_FAX','I_SCANNER','I_WHITEOUT','I_ENERGY','I_JAM','I_CALC','I_ALLHANDS','I_AIRPLANE','I_MUG']
    pm = ['P_NEW','P_TRAIN','P_HOF','P_README','P_ABOUT','P_EXIT','P_RESUME','P_TUTORIAL','P_ARCADE']
    mb = ['M_INFO','M_WARN','M_STOP','M_QUESTION']
    out.append('const Spr S_ASSET[N_ASSET] = {' + ', '.join(sp(n) for n in assets) + '};')
    out.append('const Spr S_HAZ[N_HAZ] = {{1,1,R_S_MISS}, ' + ', '.join(sp(n) for n in haz[1:]) + '};')
    out.append('const Spr S_ITEM[N_ITEM] = {' + ', '.join(sp(n) for n in items) + '};')
    out.append('const Spr S_PM[9] = {' + ', '.join(sp(n) for n in pm) + '};')
    out.append('const Spr S_MB[4] = {' + ', '.join(sp(n) for n in mb) + '};')
    for sym, n in [('S_BOOM','S_BOOM'),('S_MISS','S_MISS'),('S_FLAG','S_FLAG'),('S_NOTE','S_NOTE'),('S_SHIELD','S_SHIELD'),
                   ('S_LOCK','S_LOCK'),('S_PLANE','S_PLANE'),('S_SMILE','S_SMILE'),('S_COIN','S_COIN'),('S_STAR','S_STAR'),
                   ('S_CURSOR','CURSOR'),('S_HOURGLASS','HOURGLASS'),('S_STAN_BASE','STAN_BASE')]:
        out.append('const Spr %s = %s;' % (sym, sp(n)))
    out.append('const Spr S_STAN_ARM[3] = {' + ', '.join(sp('STAN_ARM%d' % i) for i in range(3)) + '};')
    open(os.path.join(HERE, '..', 'src', 'art.c'), 'w').write('\n'.join(out) + '\n')

def preview():
    from PIL import Image, ImageDraw
    names = list(SPR)
    cols, S, pad = 8, 6, 8
    cellw = 44 * S // 2 + pad
    rows_n = (len(names) + cols - 1) // cols
    img = Image.new('RGB', (cols * (40 * S // 2 + pad) + pad, rows_n * (24 * S // 2 + 20) + pad), (0, 128, 128))
    d = ImageDraw.Draw(img)
    for k, n in enumerate(names):
        w, h, rows = SPR[n]
        ox = pad + (k % cols) * (40 * S // 2 + pad)
        oy = pad + (k // cols) * (24 * S // 2 + 20)
        d.rectangle([ox - 1, oy - 1, ox + w * S // 2 * 1 + 0, oy + h * S // 2 + 0], outline=(0, 90, 90))
        sc = S // 2
        for j, r in enumerate(rows):
            for i, ch in enumerate(r):
                if ch == '.': continue
                col = PAL[int(ch, 16)]
                d.rectangle([ox + i * sc, oy + j * sc, ox + i * sc + sc - 1, oy + j * sc + sc - 1], fill=col)
        d.text((ox, oy + h * sc + 2), n, fill=(255, 255, 255))
    img.save(os.path.join(HERE, 'preview.png'))

if __name__ == '__main__':
    emit_c()
    write_ico(os.path.join(HERE, '..', 'res', 'stan.ico'))
    preview()
    print('sprites:', len(SPR))
