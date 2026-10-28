/* Procedural edutainment question generator. */
#include "ht.h"

const char *g_modname[N_MODULE] = {"Number Crunch", "Word Works", "Ledger Lab", "World Office", "Typing 101"};
const char *g_modblurb[N_MODULE] = {
    "Arithmetic, percentages and sequences.", "Spelling, vocabulary and corporate jargon.",
    "Accounts, debits, credits and payroll.", "Capitals of the world and of Canada.", "Type the memo before the timer runs out." };

static int rpick(Rng *r, int n) { return rng_n(r, n); }
static int g_qd = 2;   /* difficulty of the question being built */
/* easy = wide, obvious distractors; insane = distractors hugging the answer */
static long sp(long spread) { long v = spread * (6 - g_qd) / 3; return v < 2 ? 2 : v; }

/* Put correct answer in opt[0], distractors in 1..3 (already distinct), then shuffle. */
static void finish_mc(Question *q, Rng *r) {
    int i, j, corr = 0;
    char tmp[56];
    q->nopt = 4; q->kind = 0;
    for (i = 3; i > 0; i--) {
        j = rng_n(r, i + 1);
        if (i != j) {
            strcpy(tmp, q->opt[i]); strcpy(q->opt[i], q->opt[j]); strcpy(q->opt[j], tmp);
            if (corr == i) corr = j; else if (corr == j) corr = i;
        }
    }
    q->correct = corr;
    snprintf(q->ans, sizeof q->ans, "%s", q->opt[corr]);
}

static int dup(const Question *q, int n, const char *s) {
    int i;
    for (i = 0; i < n; i++) if (!strcmp(q->opt[i], s)) return 1;
    return 0;
}

/* numeric multiple choice. prefix/suffix wrap the number. */
static void mc_num(Question *q, Rng *r, long ans, long spread, const char *pre, const char *suf) {
    int n = 1, guard = 0;
    snprintf(q->opt[0], sizeof q->opt[0], "%s%ld%s", pre, ans, suf);
    spread = sp(spread);
    while (n < 4 && guard++ < 200) {
        long d = rng_range(r, 1, (int)spread) * (rng_n(r, 2) ? 1 : -1), v = ans + d;
        char s[56];
        if (guard % 5 == 0 && ans > 9) { long t = ans / 10 + (ans % 10) * 10; if (t != ans) v = t; }
        if (v < 0 && ans >= 0) continue;
        snprintf(s, sizeof s, "%s%ld%s", pre, v, suf);
        if (!dup(q, n, s)) { strcpy(q->opt[n], s); n++; }
    }
    while (n < 4) { snprintf(q->opt[n], sizeof q->opt[n], "%s%ld%s", pre, ans + 7 * n + 3, suf); n++; }
    finish_mc(q, r);
}

/* money with cents: value in cents */
static void mc_money(Question *q, Rng *r, long cents, long spread) {
    int n = 1, guard = 0;
    spread = sp(spread) + 10;
    snprintf(q->opt[0], sizeof q->opt[0], "$%ld.%02ld", cents / 100, labs(cents) % 100);
    while (n < 4 && guard++ < 300) {
        long d = rng_range(r, 1, (int)spread) * (rng_n(r, 2) ? 1 : -1), v = cents + d;
        char s[56];
        if (v < 0) continue;
        snprintf(s, sizeof s, "$%ld.%02ld", v / 100, v % 100);
        if (!dup(q, n, s)) { strcpy(q->opt[n], s); n++; }
    }
    while (n < 4) { snprintf(q->opt[n], sizeof q->opt[n], "$%ld.%02ld", cents / 100 + n, cents % 100); n++; }
    finish_mc(q, r);
}

/* text multiple choice: correct from list index c, distractors from other entries of same list */
static void mc_list(Question *q, Rng *r, const char *const *list, int n, int c) {
    int k = 1, guard = 0;
    snprintf(q->opt[0], sizeof q->opt[0], "%s", list[c]);
    while (k < 4 && guard++ < 300) {
        int j = rng_n(r, n);
        if (j == c || dup(q, k, list[j])) continue;
        snprintf(q->opt[k], sizeof q->opt[k], "%s", list[j]); k++;
    }
    while (k < 4) { snprintf(q->opt[k], sizeof q->opt[k], "None of these %d", k); k++; }
    finish_mc(q, r);
}

/* ============================================================ MATH */
static void q_math(Question *q, Rng *r, int d) {
    /* tiers: 0-2 easy arithmetic, 3-4 two-digit, 5-7 percent/money/sequences, 8-9 multi-step */
    static const int lo[6] = {0, 0, 0, 1, 3, 5}, hi[6] = {0, 3, 5, 8, 10, 10};
    int t = lo[d] + rpick(r, hi[d] - lo[d]);
    int a, b, big = d * d * 2;      /* 2, 8, 18, 32, 50 */
    long ans;
    switch (t) {
    case 0: a = rng_range(r, 2, 6 + big); b = rng_range(r, 2, 5 + big); ans = a + b;
        snprintf(q->q, sizeof q->q, "Quick! What is %d + %d?", a, b); snprintf(q->expl, sizeof q->expl, "%d + %d = %ld.", a, b, ans); mc_num(q, r, ans, 5, "", ""); break;
    case 1: a = rng_range(r, 6, 12 + big * 2); b = rng_range(r, 2, a - 1); ans = a - b;
        snprintf(q->q, sizeof q->q, "You have %d pens. Karen takes %d. How many left?", a, b); snprintf(q->expl, sizeof q->expl, "%d - %d = %ld. Karen always takes the pens.", a, b, ans); mc_num(q, r, ans, 6, "", ""); break;
    case 2: a = rng_range(r, 2, 4 + d * 2); b = rng_range(r, 2, 3 + d * 2); ans = (long)a * b;
        snprintf(q->q, sizeof q->q, "A box holds %d staplers. How many in %d boxes?", a, b); snprintf(q->expl, sizeof q->expl, "%d x %d = %ld.", a, b, ans); mc_num(q, r, ans, 9, "", ""); break;
    case 3: a = rng_range(r, 12, 40 + d * 40); b = rng_range(r, 12, 40 + d * 40); ans = a + b;
        snprintf(q->q, sizeof q->q, "%d + %d = ?", a, b); snprintf(q->expl, sizeof q->expl, "%d + %d = %ld.", a, b, ans); mc_num(q, r, ans, 12, "", ""); break;
    case 4: a = rng_range(r, 6, 10 + d * 3); b = rng_range(r, 6, 10 + d * 3); ans = (long)a * b;
        snprintf(q->q, sizeof q->q, "Multiply: %d x %d", a, b); snprintf(q->expl, sizeof q->expl, "%d x %d = %ld.", a, b, ans); mc_num(q, r, ans, 14, "", ""); break;
    case 5: { static const int pc[] = {10, 20, 25, 50, 75}; int p = pc[rpick(r, 5)]; a = rng_range(r, 2, 40) * 4;
        ans = (long)a * p / 100;
        snprintf(q->q, sizeof q->q, "What is %d%% of %d?", p, a); snprintf(q->expl, sizeof q->expl, "%d%% of %d = %ld.", p, a, ans); mc_num(q, r, ans, 8, "", ""); break; }
    case 6: { int items = rng_range(r, 3, 9), price = rng_range(r, 2, 15) * 5; ans = (long)items * price;
        snprintf(q->q, sizeof q->q, "%d desk toys at $%d each. Total cost?", items, price); snprintf(q->expl, sizeof q->expl, "%d x $%d = $%ld.", items, price, ans); mc_num(q, r, ans, 20, "$", ""); break; }
    case 7: { int start = rng_range(r, 2, 9), step = rng_range(r, 2, 8); int i; char buf[80]; int len = 0;
        for (i = 0; i < 4; i++) len += snprintf(buf + len, sizeof buf - (size_t)len, "%d, ", start + step * i);
        ans = start + step * 4;
        snprintf(q->q, sizeof q->q, "What comes next? %s?", buf); snprintf(q->expl, sizeof q->expl, "Add %d each time: next is %ld.", step, ans); mc_num(q, r, ans, 6, "", ""); break; }
    case 8: { int hrs = rng_range(r, 3, 8), wage = rng_range(r, 14, 30); ans = (long)hrs * wage + 8 * wage;
        snprintf(q->q, sizeof q->q, "You work %d hours Mon and 8 hours Tue at $%d/h. Pay?", hrs, wage); snprintf(q->expl, sizeof q->expl, "(%d + 8) h x $%d = $%ld.", hrs, wage, ans); mc_num(q, r, ans, 24, "$", ""); break; }
    default: { int price = rng_range(r, 4, 40) * 5, pc = rng_range(r, 1, 5) * 10; ans = (long)price * (100 - pc) / 100;
        snprintf(q->q, sizeof q->q, "A $%d chair is %d%% off. Sale price?", price, pc); snprintf(q->expl, sizeof q->expl, "$%d less %d%% = $%ld.", price, pc, ans); mc_num(q, r, ans, 12, "$", ""); break; }
    }
    /* advanced extras */
    if (d >= 4 && rng_pct(r, d == 5 ? 60 : 30)) {
        long cents; int p = rng_range(r, 4, 60) * 5;
        cents = (long)p * 115;   /* HST 15% */
        snprintf(q->q, sizeof q->q, "NB HST is 15%%. A $%d.00 desk lamp costs how much with tax?", p);
        snprintf(q->expl, sizeof q->expl, "$%d.00 x 1.15 = $%ld.%02ld.", p, cents / 100, cents % 100);
        mc_money(q, r, cents, 400);
    }
}

/* ============================================================ WORDS */
static const char *const spell[] = {"accommodate","occurrence","necessary","separate","definitely","receive","calendar","committee","embarrass","environment",
    "government","independent","knowledge","maintenance","millennium","noticeable","privilege","recommend","rhythm","schedule","unnecessary","withhold",
    "acknowledge","bureaucracy","conscientious","discrepancy","equipment","guarantee","inventory","liaison","negotiate","perseverance","questionnaire","reimburse","supervisor"};
static const char *const vocab_w[] = {"Ubiquitous","Redundant","Delegate","Liaison","Liability","Compliance","Incentive","Tenure","Ambiguous","Lucrative","Obsolete","Mandate","Concise","Frugal","Prerequisite"};
static const char *const vocab_m[] = {"Found everywhere","No longer needed","Hand a task to someone else","A go-between","A legal responsibility","Following the rules","A reward that motivates","Length of time in a job","Unclear in meaning","Producing a lot of money","Out of date","An official order","Short and clear","Careful with money","Required beforehand"};
static const char *const jarg_p[] = {"Circle back","Touch base","Low-hanging fruit","Move the needle","Deep dive","Take it offline","Synergy","Bandwidth","Ping me","Going forward","Drill down","Reach out"};
static const char *const jarg_m[] = {"Discuss again later","Talk briefly","The easy tasks","Make real progress","Detailed analysis","Discuss privately","Working together","Available time","Send me a message","From now on","Examine in detail","Contact someone"};

static void mutate(char *out, const char *w, Rng *r, int kind) {
    int n = (int)strlen(w), i;
    strcpy(out, w);
    switch (kind) {
    case 0: i = rng_range(r, 1, n - 3); { char t = out[i]; out[i] = out[i + 1]; out[i + 1] = t; } break;           /* swap */
    case 1: i = rng_range(r, 1, n - 2); memmove(out + i, out + i + 1, (size_t)(n - i)); break;                         /* delete */
    case 2: i = rng_range(r, 1, n - 2); memmove(out + i + 1, out + i, (size_t)(n - i + 1)); break;                     /* double */
    default: for (i = 1; i < n; i++) { char c = out[i]; if (strchr("aeiou", c)) { const char *v = "aeiou"; do { out[i] = v[rng_n(r, 5)]; } while (out[i] == c); break; } }
    }
}

static void q_words(Question *q, Rng *r, int d) {
    int t = rpick(r, 3);
    if (d <= 1 && t == 2) t = 1;
    if (d >= 4 && t == 1) t = rng_n(r, 2) ? 0 : 2;
    if (t == 0) {
        int c, k = 1, guard = 0, maxl = d <= 1 ? 8 : d == 2 ? 10 : d == 3 ? 12 : 99;
        do c = rpick(r, ARRAY_LEN(spell)); while ((int)strlen(spell[c]) > maxl && guard++ < 100);
        guard = 0;
        snprintf(q->q, sizeof q->q, "Which spelling is CORRECT?");
        snprintf(q->opt[0], sizeof q->opt[0], "%s", spell[c]);
        while (k < 4 && guard++ < 200) {
            char m[56];
            mutate(m, spell[c], r, rng_n(r, 4));
            if (strcmp(m, spell[c]) && !dup(q, k, m)) { strcpy(q->opt[k], m); k++; }
        }
        while (k < 4) { snprintf(q->opt[k], sizeof q->opt[k], "%sx%d", spell[c], k); k++; }
        snprintf(q->expl, sizeof q->expl, "It's spelled '%s'. Spellcheck has left the building.", spell[c]);
        finish_mc(q, r);
    } else if (t == 1) {
        int c = rpick(r, ARRAY_LEN(vocab_w));
        snprintf(q->q, sizeof q->q, "Vocabulary: what does '%s' mean?", vocab_w[c]);
        snprintf(q->expl, sizeof q->expl, "%s: %s.", vocab_w[c], vocab_m[c]);
        mc_list(q, r, vocab_m, ARRAY_LEN(vocab_m), c);
    } else {
        int c = rpick(r, ARRAY_LEN(jarg_p));
        snprintf(q->q, sizeof q->q, "Translate the jargon: '%s' means...", jarg_p[c]);
        snprintf(q->expl, sizeof q->expl, "'%s' really means: %s.", jarg_p[c], jarg_m[c]);
        mc_list(q, r, jarg_m, ARRAY_LEN(jarg_m), c);
    }
}

/* ============================================================ LEDGER */
static const struct { const char *n; int t; } accts[] = {
    {"Cash",0},{"Accounts Receivable",0},{"Inventory",0},{"Office Equipment",0},{"Prepaid Insurance",0},{"Vehicles",0},
    {"Accounts Payable",1},{"Bank Loan",1},{"Wages Payable",1},{"Unearned Revenue",1},{"Taxes Payable",1},
    {"Owner's Capital",2},{"Retained Earnings",2},{"Owner's Drawings",2},
    {"Sales Revenue",3},{"Service Revenue",3},{"Interest Income",3},
    {"Rent Expense",4},{"Wages Expense",4},{"Utilities Expense",4},{"Advertising Expense",4},{"Supplies Expense",4}};
static const char *const atype[] = {"Asset","Liability","Equity","Revenue","Expense"};

static void q_ledger(Question *q, Rng *r, int d) {
    int t = d >= 4 ? 2 + rpick(r, 3) : rpick(r, d <= 1 ? 2 : d == 2 ? 3 : 5);
    if (t <= 1) {
        int c = rpick(r, ARRAY_LEN(accts));
        if (t == 0) {
            snprintf(q->q, sizeof q->q, "Classify the account: %s", accts[c].n);
            snprintf(q->expl, sizeof q->expl, "%s is a(n) %s account.", accts[c].n, atype[accts[c].t]);
            mc_list(q, r, atype, 5, accts[c].t);
        } else {
            int debit = (accts[c].t == 0 || accts[c].t == 4 || !strcmp(accts[c].n, "Owner's Drawings"));
            static const char *const dc[] = {"Debit","Credit","Both equally","Neither"};
            snprintf(q->q, sizeof q->q, "What is the normal balance of %s?", accts[c].n);
            snprintf(q->expl, sizeof q->expl, "%s normally carries a %s balance. (DEALER: Drawings, Expenses, Assets, Losses, Dividends = debit.)", accts[c].n, debit ? "debit" : "credit");
            mc_list(q, r, dc, 4, debit ? 0 : 1);
        }
    } else if (t == 2) {
        int a = rng_range(r, 20, 90) * 100, l = rng_range(r, 5, 40) * 100, e = a - l;
        int which = rpick(r, 3);
        if (which == 0) { snprintf(q->q, sizeof q->q, "Assets $%d, Liabilities $%d. What is Equity? (A = L + E)", a, l); snprintf(q->expl, sizeof q->expl, "E = A - L = $%d.", e); mc_num(q, r, e, 800, "$", ""); }
        else if (which == 1) { snprintf(q->q, sizeof q->q, "Liabilities $%d, Equity $%d. What are Assets?", l, e); snprintf(q->expl, sizeof q->expl, "A = L + E = $%d.", a); mc_num(q, r, a, 800, "$", ""); }
        else { snprintf(q->q, sizeof q->q, "Assets $%d, Equity $%d. What are Liabilities?", a, e); snprintf(q->expl, sizeof q->expl, "L = A - E = $%d.", l); mc_num(q, r, l, 600, "$", ""); }
    } else if (t == 3) {
        int rate = rng_range(r, 15, 30), ot = rng_range(r, 2, 10);
        long cents = (long)(40 * rate + ot * rate * 3 / 2) * 100 + (ot * rate % 2) * 50;
        snprintf(q->q, sizeof q->q, "Gross pay: 40 h regular + %d h overtime (1.5x) at $%d/h?", ot, rate);
        snprintf(q->expl, sizeof q->expl, "40 x $%d + %d x $%d x 1.5 = $%ld.%02ld.", rate, ot, rate, cents / 100, cents % 100);
        mc_money(q, r, cents, 900);
    } else {
        int sales = rng_range(r, 40, 300) * 50, cost = rng_range(r, 10, 35) * 50 + 100;
        long gp = sales - cost;
        snprintf(q->q, sizeof q->q, "Sales are $%d and cost of goods sold is $%d. Gross profit?", sales, cost);
        snprintf(q->expl, sizeof q->expl, "Gross profit = Sales - COGS = $%ld.", gp);
        mc_num(q, r, gp, 500, "$", "");
    }
}

/* ============================================================ WORLD */
static const char *const countries[] = {"France","Germany","Italy","Spain","Portugal","Norway","Sweden","Poland","Greece","Ireland","Japan","China","India","Egypt","Kenya","Nigeria","Brazil","Argentina","Chile","Peru","Mexico","Australia","New Zealand","Turkey","Thailand","Vietnam","Canada","Finland","Austria","Morocco"};
static const char *const caps[] = {"Paris","Berlin","Rome","Madrid","Lisbon","Oslo","Stockholm","Warsaw","Athens","Dublin","Tokyo","Beijing","New Delhi","Cairo","Nairobi","Abuja","Brasilia","Buenos Aires","Santiago","Lima","Mexico City","Canberra","Wellington","Ankara","Bangkok","Hanoi","Ottawa","Helsinki","Vienna","Rabat"};
static const char *const prov[] = {"Ontario","Quebec","Nova Scotia","New Brunswick","Manitoba","British Columbia","Prince Edward Island","Saskatchewan","Alberta","Newfoundland and Labrador"};
static const char *const pcap[] = {"Toronto","Quebec City","Halifax","Fredericton","Winnipeg","Victoria","Charlottetown","Regina","Edmonton","St. John's"};

static void q_world(Question *q, Rng *r, int d) {
    if (rng_pct(r, d <= 2 ? 45 : 35)) {
        int c = rpick(r, d <= 2 ? 4 : (int)ARRAY_LEN(prov));
        snprintf(q->q, sizeof q->q, "What is the capital of %s?", prov[c]);
        snprintf(q->expl, sizeof q->expl, "The capital of %s is %s.", prov[c], pcap[c]);
        mc_list(q, r, pcap, ARRAY_LEN(pcap), c);
    } else {
        int c = rpick(r, d <= 1 ? 10 : d == 2 ? 20 : (int)ARRAY_LEN(countries));
        snprintf(q->q, sizeof q->q, "What is the capital of %s?", countries[c]);
        snprintf(q->expl, sizeof q->expl, "The capital of %s is %s.", countries[c], caps[c]);
        mc_list(q, r, caps, ARRAY_LEN(caps), c);
    }
}

/* ============================================================ TYPING */
static const char *const phrases[] = {"per my last email","please advise","let us circle back","synergy is a mindset","leverage the paradigm","reply all for pizza","the stapler is mine",
    "actions speak louder","low hanging fruit","touch base offline","move the needle","quarterly stakeholder review","please see attached","best regards and thanks",
    "mandatory fun begins at noon","do not feed the printer","who took my lunch","hostile takeover in progress","balance the ledger daily","accounts payable is closed"};
static void q_typing(Question *q, Rng *r, int d) {
    int c = rpick(r, ARRAY_LEN(phrases)), guard = 0;
    while (d <= 2 && strlen(phrases[c]) > 20 && guard++ < 50) c = rpick(r, ARRAY_LEN(phrases));
    snprintf(q->q, sizeof q->q, "Type this memo EXACTLY, then press ENTER or click Submit:");
    if (d >= 5) snprintf(q->ans, sizeof q->ans, "%c%s. Ref #%d", phrases[c][0] - 32, phrases[c] + 1, rng_range(r, 100, 999));
    else if (d >= 4) snprintf(q->ans, sizeof q->ans, "%c%s.", phrases[c][0] - 32, phrases[c] + 1);
    else snprintf(q->ans, sizeof q->ans, "%s", phrases[c]);
    snprintf(q->expl, sizeof q->expl, "'%s'. Excellent keyboarding, Intern.", q->ans);
    q->kind = 1; q->nopt = 0; q->correct = -1;
    q->time_ms = (int)strlen(q->ans) * (700 - d * 70) + 4000;
}

void quiz_make(Question *q, Rng *r, int module, int diff) {
    memset(q, 0, sizeof *q);
    q->module = module; q->diff = CLAMP(diff, 1, 5); g_qd = q->diff;
    q->time_ms = 15000 - q->diff * 1500;
    switch (module) {
    case M_MATH: q_math(q, r, q->diff); break;
    case M_WORDS: q_words(q, r, q->diff); break;
    case M_LEDGER: q_ledger(q, r, q->diff); break;
    case M_WORLD: q_world(q, r, q->diff); break;
    default: q_typing(q, r, q->diff); break;
    }
}
