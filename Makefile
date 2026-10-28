# HOSTILE TAKEOVER - build with MinGW gcc on Windows (mingw32-make) or cross-compile from Linux.
SRCS = src/main_win32.c src/ui.c src/ui_battle.c src/ui_screens.c src/data.c src/board.c \
       src/battle.c src/quiz.c src/gfx.c src/font.c src/art.c src/audio.c \
       src/ui_save.c src/ui_tutorial.c src/ui_games.c
CFLAGS = -std=gnu99 -O2 -Wall -mwindows
LIBS = -lwinmm -lgdi32 -luser32 -lm

# native MinGW gcc on Windows:
res/hostile.o: res/hostile.rc res/stan.ico
	windres res/hostile.rc -O coff -o res/hostile.o

HostileTakeover.exe: $(SRCS) src/ht.h src/ui.h res/hostile.o
	gcc $(CFLAGS) -static -s -o $@ $(SRCS) res/hostile.o $(LIBS)

# cross-compile from Linux (apt install gcc-mingw-w64):
cross:
	x86_64-w64-mingw32-windres res/hostile.rc -O coff -o res/hostile64.o
	i686-w64-mingw32-windres res/hostile.rc -O coff -o res/hostile32.o
	x86_64-w64-mingw32-gcc $(CFLAGS) -static -s -o HostileTakeover.exe $(SRCS) res/hostile64.o $(LIBS)
	i686-w64-mingw32-gcc $(CFLAGS) -static -s -o HostileTakeover32.exe $(SRCS) res/hostile32.o $(LIBS)

# regenerate sprites (needs Python + Pillow for the preview sheet)
art:
	python tools/art.py

# headless logic fuzz test (any native gcc)
test:
	gcc -std=gnu99 -O1 -Wall -o ht_test src/main_test.c src/data.c src/board.c src/battle.c src/quiz.c && ./ht_test 300

clean:
	del HostileTakeover.exe
