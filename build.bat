@echo off
rem HOSTILE TAKEOVER - build with MinGW gcc (needs gcc on PATH)
windres res\hostile.rc -O coff -o res\hostile.o
gcc -std=gnu99 -O2 -Wall -mwindows -static -s -o HostileTakeover.exe ^
  src\main_win32.c src\ui.c src\ui_battle.c src\ui_screens.c src\data.c src\board.c ^
  src\battle.c src\quiz.c src\gfx.c src\font.c src\art.c src\audio.c ^
  src\ui_save.c src\ui_tutorial.c src\ui_games.c res\hostile.o ^
  -lwinmm -lgdi32 -luser32 -lm
if errorlevel 1 (echo BUILD FAILED) else (echo Built HostileTakeover.exe - run it!)
