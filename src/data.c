/* Static game data: assets, departments, perks, items, careers, events, text generators. */
#include "ht.h"

const AssetDef g_adef[N_ASSET] = {
 {"Potted Fern",      1, {0},               {0},               C_GREEN,   "Photosynthesis",  "+1 Morale at the start of each of your turns", 20},
 {"Water Cooler",     2, {0,0},             {0,1},             C_AQUA,    "Water-Cooler Talk","+5 Synergy at the start of each turn", 30},
 {"Laser Printer",    3, {0,1,2},           {0,0,0},           C_SILVER,  "Paper Jam",       "25% of enemy hits do no damage", 40},
 {"Cubicle",          3, {0,0,1},           {0,1,1},           C_BLUE,    "Soundproof Walls","Enemy hits do 1 less damage (min 1)", 50},
 {"Vending Machine",  4, {1,2,0,1,0,0},     {0,0,1,1,0,0},    C_RED,     "Snack Run",       "+$5 at the start of each turn", 60},
 {"Executive Desk",   4, {0,1,2,1,0,0},     {0,0,0,1,0,0},    C_OLIVE,   "Power Lunch",     "+50% Synergy gain", 60},
 {"Server Rack",      4, {0,1,2,3,0,0},     {0,0,0,0,0,0},   C_GRAY,    "Cloud Backup",    "Once per floor, one sinking is undone", 80},
 {"Mainframe",        4, {0,1,0,1,0,0},     {0,0,1,1,0,0},    C_PURPLE,  "Overclock",       "+1 Action every 4th turn", 90},
 {"Conference Table", 6, {0,1,2,0,1,2},     {0,0,0,1,1,1},   C_MAROON,  "Synergy Meeting", "+$10 and +5 Morale on floor clear", 120},
 {"Paper Shredder",   1, {0},               {0},               C_WHITE,   "Shred Evidence",  "Ignores the first enemy damage each floor", 30},
 {"Mahogany Boardroom",5,{1,0,1,2,1,0},     {0,1,1,1,2,0},   C_YELLOW,  "Golden Parachute","(Executives only)", 999},
};

const FloorDef g_floors[N_FLOORS] = {
/* dept, tag, boss, label, size, ai, skill, rules, reorg, dbl, inj, dmg, haz{wet,donut,closet,replyall,coffee}, weights, cells% */
 {"Mailroom","Return to sender. Repeatedly.",NULL, 1, 8, AI_RANDOM,1, 0,0,0,0, 2, {1,4,2,0,1}, {3,3,2,2,0,0,0,0,0,3,0}, 10},
 {"Accounts Payable","Your invoice is in another castle.",NULL, 2, 8, AI_PARITY,2, 0,0,0,0, 2, {1,2,2,3,1}, {2,2,3,3,1,1,0,0,0,2,0}, 12},
 {"Customer Service","Your call is important to us.",NULL, 3, 9, AI_HUNT,2, 0,0,0,0, 3, {2,2,2,2,1}, {2,3,3,3,2,1,1,0,0,2,0}, 13},
 {"Human Resources","We are all family here.","Brenda from HR", 4, 9, AI_HUNT,3, RULE_REORG,4,0,0, 3, {2,2,2,2,1}, {1,2,2,3,2,2,2,1,0,2,0}, 14},
 {"Information Technology","Have you tried turning it off?",NULL, 5, 10, AI_PROB,3, RULE_FIREWALL,0,0,0, 3, {1,2,2,2,2}, {1,1,2,2,2,2,3,2,1,1,0}, 15},
 {"Sales","Always. Be. Closing.",NULL, 6, 10, AI_HUNT,3, RULE_DOUBLE,0,4,0, 4, {2,3,2,1,2}, {1,2,2,2,3,3,1,1,1,2,0}, 15},
 {"Marketing","Let's circle back on this.",NULL, 7, 10, AI_HUNT,3, RULE_REORG,5,0,0, 4, {4,2,2,2,1}, {2,2,2,2,2,3,1,1,1,2,0}, 16},
 {"Research & Development","Fail fast. Fail often. Fail loudly.","The CTO", 8, 11, AI_PROB,4, RULE_FIREWALL|RULE_DOUBLE,0,4,0, 4, {1,2,2,2,2}, {0,1,1,2,2,2,3,3,2,1,0}, 16},
 {"Facilities","Please mind the wet floor.",NULL, 9, 11, AI_SWEEP,3, 0,0,0,0, 5, {6,2,2,1,1}, {1,1,2,3,3,2,2,1,1,2,0}, 16},
 {"Procurement","Please submit form 27-B/6.",NULL, 10, 11, AI_PARITY,4, RULE_INJUNCTION,0,0,6, 5, {2,2,2,2,2}, {0,1,2,2,2,3,3,2,2,2,0}, 17},
 {"Finance","Everything is a rounding error.",NULL, 11, 12, AI_PROB,4, RULE_AUDIT,0,0,0, 5, {2,2,3,2,2}, {0,1,1,2,2,3,3,3,2,1,0}, 17},
 {"Legal","Our attorneys will be in touch.","General Counsel Von Sue", 12, 12, AI_PROB,5, RULE_INJUNCTION|RULE_REORG,6,0,8, 6, {2,2,2,3,1}, {0,0,1,2,2,3,3,3,3,1,0}, 18},
 {"The Executive Suite","Synergy, but make it violent.","THE CEO", 14, 12, AI_PROB,5, RULE_REORG|RULE_DOUBLE|RULE_FIREWALL,6,3,0, 7, {2,2,2,2,2}, {0,0,0,2,2,3,3,3,3,0,0}, 19},
};

const PerkDef g_perk[N_PERK] = {
 {"Ergonomic Chair",   "+20 max Morale.", 0},
 {"Standing Desk",     "+1 Action on your first turn of every floor.", 0},
 {"Noise-Cancelling Headset","Wrong training answers cost half Morale.", 0},
 {"Company Lanyard",   "+$25 on every floor clear.", 0},
 {"Corporate Card",    "Everything in the Break Room is 15% off.", 0},
 {"Mechanical Keyboard","Correct training answers give +15 extra Synergy.", 0},
 {"Dual Monitors",     "+2 inventory slots.", 0},
 {"Lucky Mug",         "15% chance of +1 Action at the start of each turn.", 0},
 {"Motivational Poster","+2 Morale for every hit you land.", 0},
 {"Excel Macro",       "The first training question each floor is auto-correct.", 1},
 {"Gaming Mouse",      "First missed shot each turn refunds its Action.", 1},
 {"Fire Extinguisher", "Wet Floor hazards no longer slide your shots.", 0},
 {"Employee of the Month","+10 Morale on every floor clear.", 0},
 {"Stress Ball",       "Enemy hits do 1 less damage (min 1).", 0},
 {"Golden Parachute",  "Once per run: survive burnout with 30 Morale.", 1},
 {"Unpaid Intern",     "Every 3rd turn the Intern fires a free shot.", 1},
};

const ItemDef g_item[N_ITEM] = {
 {"Stapler",         "Fire at a cell and its 4 neighbours (cross).", 1, 1, 55, 2},
 {"Highlighter",     "Count asset cells in a row AND column.", 1, 0, 25, 3},
 {"Sticky Note",     "Reveal whether one cell hides an Asset.", 1, 0, 20, 4},
 {"Fax Machine",     "Fire along 3 cells in a line (row).", 1, 1, 45, 3},
 {"Badge Scanner",   "Ping a cell: distance to nearest Asset.", 1, 0, 30, 3},
 {"Whiteout",        "Heal 25 Morale.", 0, 0, 35, 3},
 {"Energy Drink",    "+2 Actions now. -5 Morale (crash).", 0, 0, 30, 3},
 {"Printer Jam",     "The enemy skips their next turn.", 0, 0, 50, 2},
 {"Calculator",      "Your next training question is auto-correct.", 0, 0, 25, 3},
 {"All-Hands Meeting","Fire at a 3x3 block.", 1, 1, 90, 1},
 {"Paper Airplane",  "Homing shot: likely finds an Asset.", 0, 1, 70, 2},
 {"Coffee Mug",      "+1 Action.", 0, 0, 20, 4},
};

const char *g_career_name[N_CAREER] = {"Intern","Accountant","IT Wizard","Sales Shark","Night Janitor"};
const char *g_career_desc[N_CAREER] = {
 "Eager. Underpaid. Starts with a Stapler, a Sticky Note and hope.",
 "Training pays +50%. Starts with a Calculator. Loves a balanced ledger.",
 "Starts with 2 Badge Scanners. Has seen things in the server room.",
 "Starts with $100 and a Fax Machine. Closes deals; sometimes people.",
 "Sees every Hazard on the floor. Wet floors never slide your shots."
};

/* ------------------------------------------------------------ events */
const EventDef g_event[] = {
 {"The Vending Machine Ate Your Dollar", "Your dollar is visible behind the glass. The machine hums at you, smugly.", 2, {
   {"Shake the machine", "Three items tumble out! The machine respects you now.", "The machine falls on you. Facilities will hear about this.", 60, FX_ITEM,-1, FX_MORALE,-10},
   {"Accept your loss", "You grow as a person. Synergy +10.", "", 100, FX_SYNERGY,10, FX_NONE,0}}},
 {"Mandatory Fun Day", "A banner reads 'FUN IS NOT OPTIONAL'. Someone has brought a parachute.", 2, {
   {"Participate enthusiastically", "You win the three-legged race. Morale soars.", "You fall. In front of Finance.", 55, FX_MORALE,20, FX_MORALE,-12},
   {"Hide in the stairwell", "Nobody noticed. You found $25 on the stairs.", "You are found. Written warning.", 65, FX_CASH,25, FX_MORALE,-8}}},
 {"A Wallet In The Hallway", "Fat leather wallet. A photo of someone's cat, and $60 in cash.", 2, {
   {"Return it to HR", "The owner is delighted. Synergy +30.", "HR loses it. Again.", 80, FX_SYNERGY,30, FX_NONE,0},
   {"Pocket the cash", "You are $60 richer and mostly unobserved.", "Security camera. Morale -20.", 50, FX_CASH,60, FX_MORALE,-20}}},
 {"Free Pizza In Conference Room C", "A single pizza box. A single sticky note: 'FOR EVERYONE'.", 2, {
   {"Take three slices", "Delicious. Morale +20.", "Gary saw you. Gary is going to tell everyone.", 65, FX_MORALE,20, FX_MORALE,-10},
   {"Take the whole box", "You are a legend. Max Morale +10.", "It was the CEO's pizza. Morale -25.", 40, FX_MAXMORALE,10, FX_MORALE,-25}}},
 {"The CEO Wants A Word", "The CEO's assistant is smiling in a way that suggests no mercy.", 2, {
   {"Project total confidence", "He is impressed. You are handed a Corporate Perk.", "Eye contact was a mistake. Morale -30.", 35, FX_PERK,0, FX_MORALE,-30},
   {"Mumble something about Q3", "He nods. You gain $20.", "He asks a follow-up. Morale -5.", 85, FX_CASH,20, FX_MORALE,-5}}},
 {"Mysterious Door Marked 'B14'", "There is no B14. There is no basement. The door is warm.", 2, {
   {"Open it", "A cache of abandoned office supplies!", "A sound like a fax machine screaming. Morale -15.", 50, FX_ITEM,-1, FX_MORALE,-15},
   {"Knock first", "A voice says 'come in'. You are handed $30.", "Nobody answers. You feel watched. Morale -5.", 70, FX_CASH,30, FX_MORALE,-5}}},
 {"The Server Room Is Lightly On Fire", "Alarms. Small flames. A man in a hoodie shrugs.", 2, {
   {"Fix it yourself", "Heroic. The Board of Directors sends a Perk.", "Burns. Morale -20.", 45, FX_PERK,0, FX_MORALE,-20},
   {"Post a photo on LinkedIn", "'Thrilled to announce.' Synergy +40.", "Nobody likes it. Morale -5.", 80, FX_SYNERGY,40, FX_MORALE,-5}}},
 {"Surprise Performance Review", "Your manager has a laminated folder. This is bad.", 2, {
   {"Argue your case", "You win! Max Morale +15.", "You lose. Morale -20.", 35, FX_MAXMORALE,15, FX_MORALE,-20},
   {"Agree to everything", "A $40 'bonus' (it's a coupon).", "They add a bullet point. Morale -5.", 85, FX_CASH,40, FX_MORALE,-5}}},
 {"Karaoke Night", "The microphone has been placed in your hands. How did that happen.", 2, {
   {"Sing 'Livin' On A Prayer'", "Standing ovation. Synergy +50.", "Your voice cracks. Forever. Morale -15.", 55, FX_SYNERGY,50, FX_MORALE,-15},
   {"Run the sound board", "Quiet satisfaction. +$20.", "", 100, FX_CASH,20, FX_NONE,0}}},
 {"The Anonymous Tip Box", "Someone has left a tip box on every desk. Suspicious.", 2, {
   {"Leave a tip", "You receive a reward for 'engagement'.", "", 100, FX_ITEM,-1, FX_NONE,0},
   {"Read the tips", "Gossip! Synergy +20, but Morale -5.", "", 100, FX_SYNERGY,20, FX_NONE,0}}},
 {"A Colleague Asks For A Favour", "'Quick question' is never a quick question.", 3, {
   {"Help out", "They owe you one. +$25.", "It took four hours. Morale -10.", 60, FX_CASH,25, FX_MORALE,-10},
   {"Delegate to the intern", "Perfect. +1 item.", "The intern quit. Morale -8.", 55, FX_ITEM,-1, FX_MORALE,-8},
   {"Reply-all with 'please remove me'", "Chaos. Synergy +35.", "Chaos. You were the CEO's reply-all target. Morale -25.", 40, FX_SYNERGY,35, FX_MORALE,-25}}},
};
const int g_nevent = (int)(sizeof(g_event) / sizeof(g_event[0]));

/* ------------------------------------------------------------ text gen */
static const char *fn[] = {"Gary","Brenda","Doug","Karen","Steve","Linda","Todd","Marge","Chad","Gladys","Phil","Denise","Kevin","Barb","Walt","Trish","Norm","Cheryl","Bob","Janice","Randy","Pam","Lou","Mitch"};
void gen_name(Rng *r, char *out, int max) {
    snprintf(out, (size_t)max, "%s", fn[rng_n(r, ARRAY_LEN(fn))]);
}

static const char *m_subj[] = {"Management","The Synergy Task Force","Facilities","Corporate","The Third Floor","Legal","Human Resources","The Committee","Accounting","Your Manager","Karen from Payroll","The Reorg"};
static const char *m_verb[] = {"will be leveraging","is proactively revisiting","has circled back on","is aligning on","will be sunsetting","is double-clicking on","is operationalizing","is right-sizing","is paradigm-shifting","has bandwidth-optimized"};
static const char *m_obj[] = {"our core deliverables","the stapler situation","next quarter's synergies","your cubicle","low-hanging fruit","the thing from Tuesday","a robust paradigm","going forward","all stakeholders","the mission-critical coffee"};
static const char *m_act[] = {"please advise","please acknowledge receipt","do not reply-all","see attached (nothing attached)","kindly circle back","per my last email","no action needed (action needed)","bring a pen","take the survey","reach out offline"};
void gen_memo(Rng *r, char *out, int max) {
    snprintf(out, (size_t)max, "MEMO: %s %s %s - %s.", m_subj[rng_n(r, ARRAY_LEN(m_subj))], m_verb[rng_n(r, ARRAY_LEN(m_verb))],
             m_obj[rng_n(r, ARRAY_LEN(m_obj))], m_act[rng_n(r, ARRAY_LEN(m_act))]);
}

static const char *q_win[] = {"Synergy achieved.","Leveraged successfully.","That's a wrap on Q3.","Stakeholders are delighted.","Paradigm: shifted.","Pizza party approved.","Your bonus: a handshake.","Corporate is proud (unsettlingly)."};
static const char *q_lose[] = {"Your badge has been deactivated.","Security will escort you out.","Please clear your desk by 5.","We're going in a different direction.","Your fern has been reassigned.","Thank you for your service.","Your stapler has been repossessed.","HR will see you in an hour."};
const char *quip_win(Rng *r) { return q_win[rng_n(r, ARRAY_LEN(q_win))]; }
const char *quip_lose(Rng *r) { return q_lose[rng_n(r, ARRAY_LEN(q_lose))]; }
