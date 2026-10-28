# HOSTILE TAKEOVER — The Office Warfare Simulator (for Windows 3.1™-style desktops)

*Publisher: Synergex Edu-Soft, Inc.  "Learn! Compete! Leverage!"  Ages 8–80.  Requires 1 MB RAM and a will to live.*

## 1. The pitch
It's 1993. You are Intern #4471 at **Vertex Global Industrial**. The 13 floors of the tower have been
fortified by rival Departments, who have hidden their **Assets** (printers, server racks, boardrooms...)
inside the cubicle farm. You fire **memos** at their floor plan Battleship-style, repossess every Asset,
and ride the elevator to the Executive Suite for the **Hostile Takeover**.

Roguelike: permadeath, procedural floors, random loot, perks, events, shop, seeds ("Employee ID").
Edutainment: **Mandatory Training** is the power-up engine — you answer math, spelling, bookkeeping and
geography questions from Stan the Stapler (your cheerful, deeply unsettling mascot) to earn extra
actions, cash, defuse traps and unlock certifications.
Everything runs inside a faithful, fake **Windows 3.1 desktop** (16 colours, bevels, Program Manager,
menus, GPF dialogs, a Boss Key).

## 2. Core loop (one Floor = one Battleship duel)
1. **Elevator** – animated ride, procedurally generated elevator music, department briefing.
2. **Space Planning** – drag your Assets onto your own floor (R rotates, Auto-Arrange rerolls).
3. **Battle** – alternate turns. You fire at *Their Floor* (hidden fleet); they fire at *Your Office*.
4. **Performance Review** – score, cash, heal, then pick **1 of 3 Corporate Perks**.
5. Maybe a **Random Office Event** (text-adventure with risky choices).
6. **Break Room Shop** – supplies, new Assets, perks, healing, mystery box. Discounts for answering questions.
7. Next floor. 13 floors (1–12 and **14**; there is no 13). Bosses on 4, 8, 12 and 14. Then endless **Overtime**.

Lose condition: **Morale** hits 0 (it is your HP), or your whole office is sunk (-40 Morale, retake the floor).
Death = **YOU'RE FIRED!** pink slip, severance package = score, saved to the Hall of Fame (TAKEOVER.INI).

## 3. Systems
### Assets (the "ships") — non-linear shapes, 11 kinds
Fern(1) · Water Cooler(2) · Laser Printer(3) · Cubicle(L3) · Vending Machine(S4) · Executive Desk(T4) ·
Server Rack(4) · Mainframe(2x2) · Conference Table(2x3) · Shredder(1) · Mahogany Boardroom(+5, enemy only).
**Your** Assets carry perks while alive (regen, paper-jam shield, damage reduction, cloud-backup revive,
synergy boost, bonus shots, passive income...). More Assets = more perks but a bigger target.

### Action economy
Action Points: 1/turn. Fire = 1 AP. Coffee, Energy Drinks, perks and **Training** add AP.
**Training button**: once a turn, answer a question → correct: +1 AP, +Synergy, +cash; wrong: -Morale.
**Synergy meter** fills from hits/sinks/right answers; at 100 press **SYNERGIZE!** for a random absurd
super-move (Pivot-To-Video, Disrupt The Paradigm, Leverage, Bandwidth, Team Building, Thought Shower).

### Office Supplies (12 consumables)
Stapler (cross) · Highlighter (row+column counts) · Sticky Note (reveal a cell) · Fax Machine (3 in a line) ·
Badge Scanner (distance ping) · Whiteout (heal) · Energy Drink (+2 AP, crash) · Printer Jam (enemy skips) ·
Calculator (auto-pass next quiz) · All-Hands Meeting (3x3) · Paper Airplane (homing) · Coffee Mug (+1 AP).

### Floor hazards (procedural, hidden on Their Floor)
Wet Floor (shot slides) · Free Donuts (heal) · Supply Closet (loot) · **Reply-All Trap** (answer a question
fast or lose Morale) · Coffee Machine (+1 AP).

### Departments = enemy AIs + rule mods
| Floor | Department | AI | Rule |
|---|---|---|---|
| 1 | Mailroom | Random | tutorial, lots of donuts |
| 2 | Accounts Payable | Auditor (parity hunt) | Reply-All traps |
| 3 | Customer Service | Hunt/Target | — |
| 4 | **HR (boss)** | Hunt | *Reorg*: unhit Assets relocate |
| 5 | IT | Probability density | *Firewall*: shielded cells take 2 hits |
| 6 | Sales | Aggressive hunt | *Double*: extra shot every few turns |
| 7 | Marketing | Hunt | Reorg + wet floors |
| 8 | **R&D / CTO (boss)** | Probability | Firewall + Double |
| 9 | Facilities | Sweeper | wet floors everywhere |
| 10 | Procurement | Auditor | *Injunction*: cells locked by red tape |
| 11 | Finance | Probability | *Audit*: hit combos snowball damage |
| 12 | **Legal (boss)** | Probability | Injunction + Reorg |
| 14 | **The Executive Suite (CEO)** | Probability+ | Reorg + Double + Firewall |

### Careers (starting kits)
Intern · Accountant (quiz cash +50%, Calculator) · IT Wizard (2 Badge Scanners) · Sales Shark (Fax, +$) ·
Night Janitor (sees hazards, wet-proof).

### Mandatory Training (edutainment, fully procedural)
* **Number Crunch** – arithmetic, percentages, sequences, meeting-time arithmetic, discount word problems.
* **Word Works** – misspelling detector (mutation-generated distractors), vocabulary, corporate-jargon translator.
* **Ledger Lab** – account classification, debit/credit, the accounting equation, payroll & overtime maths.
* **World Office** – capitals of the world and of every Canadian province.
* **Typing 101** – type the corporate phrase before the timer bar runs out.
Difficulty scales with floor and your Certification level; 3 correct in a row earns a **Certificate**.
Training Center on the desktop = no-risk practice.

### Random Office Incidents (mid-battle chaos)
Coffee spill (new wet floor), Fire alarm (both sides lose a turn), Free bagels (heal), Printer jam (enemy skips).

## 4. Procedural content
Floor layouts, fleets (weighted by department), orientations, hazards, shields, carpet styles, enemy
names ("Gary's Laser Printer"), corporate-speak memos (grammar generator), elevator music (seeded
scale/chord generator), shop stock, perk offers, events, every quiz question and every wrong answer.

## 5. Look & feel
* 640x480, **strict 16-colour Windows palette**, dithering for mid-tones, 5x7 proportional bitmap font.
* Win 3.1 chrome: bevelled buttons, navy title bars, control-menu boxes, menu bar, Program Manager group
  window with icons, hourglass cursor, MessageBox icons, GPF crash dialog, Exit Windows dialog.
* Software-rendered framebuffer blitted with GDI (StretchDIBits, integer-scaled, nearest-neighbour).
* Solitaire-style bouncing-card victory cascade. Boss Key (F9) shows a fake spreadsheet.
* Procedural square/triangle/noise synth: SFX + music through waveOut.

## 6. Tech
Plain C99, Win32 API, MinGW gcc. Portable core (`game.c`, `quiz.c`, `ui*.c`, `gfx.c`, `audio.c`) runs
against a tiny platform layer (`main_win32.c`: window, message loop, DIB blit, mouse/keyboard, waveOut,
INI persistence). The same core also builds natively with a headless harness (`main_test.c`) for fuzzing,
balance bots and screenshots.

Controls: mouse for everything · 1-4 answer quiz · R rotate · Esc cancel · F1 help · F2 sound ·
F9 Boss Key · F11 fullscreen · right-click on Their Floor = flag a suspect cell.
