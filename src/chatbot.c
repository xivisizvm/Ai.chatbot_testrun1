/*
 * Chip — a tiny rule-based chatbot in pure C (C99).
 *
 * Build:  gcc -std=c99 -Wall -Wextra -O2 -o chatbot src/chatbot.c
 * Run:    ./chatbot
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_INPUT 512
#define MAX_NAME  64

/* ------------------------------------------------------------------ */
/* Utilities                                                          */
/* ------------------------------------------------------------------ */

static void str_trim(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
        s[--len] = '\0';
    size_t start = 0;
    while (s[start] && isspace((unsigned char)s[start]))
        start++;
    if (start > 0)
        memmove(s, s + start, len - start + 1);
}

static void str_lower(char *s)
{
    for (; *s; s++)
        *s = (char)tolower((unsigned char)*s);
}

/* Case-insensitive substring search. */
static const char *str_istr(const char *hay, const char *needle)
{
    if (!*needle)
        return hay;
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (!*n)
            return hay;
    }
    return NULL;
}

/* Does the sentence contain ANY of the comma-separated keywords? */
static int has_any(const char *input, const char *keywords)
{
    char buf[256];
    strncpy(buf, keywords, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    char *save = NULL;
    char *kw = strtok_r(buf, ",", &save);
    while (kw) {
        if (str_istr(input, kw))
            return 1;
        kw = strtok_r(NULL, ",", &save);
    }
    return 0;
}

static void pick_from(const char *list)
{
    /* list format: "option one|option two|option three" */
    const char *start = list;
    int count = 1;
    for (const char *p = list; *p; p++)
        if (*p == '|')
            count++;

    srand((unsigned)time(NULL) ^ (unsigned)rand());
    int choice = rand() % count;

    for (int i = 0; i < choice; i++) {
        start = strchr(start, '|');
        start++;
    }
    const char *end = strchr(start, '|');
    printf("Bot: %.*s\n", end ? (int)(end - start) : (int)strlen(start), start);
}

/* ------------------------------------------------------------------ */
/* Arithmetic evaluator:  calc 12 * (3 + 4)                           */
/* A tiny recursive-descent parser: expr -> term ( +/- term )*        */
/* term -> factor ( * / % factor )*       factor -> number | (expr)   */
/* ------------------------------------------------------------------ */

static const char *g_parse_error;
static const char *g_expr; /* current position */

static void skip_ws(void)
{
    while (*g_expr && isspace((unsigned char)*g_expr))
        g_expr++;
}

static double parse_expr(void);

static double parse_factor(void)
{
    skip_ws();
    if (*g_expr == '(') {
        g_expr++;
        double v = parse_expr();
        skip_ws();
        if (*g_expr != ')') {
            g_parse_error = "missing ')'";
            return 0;
        }
        g_expr++;
        return v;
    }
    if (*g_expr == '-') { /* unary minus */
        g_expr++;
        return -parse_factor();
    }
    if (*g_expr == '+') {
        g_expr++;
        return parse_factor();
    }
    char *end;
    double v = strtod(g_expr, &end);
    if (end == g_expr) {
        g_parse_error = "expected a number";
        return 0;
    }
    g_expr = end;
    return v;
}

static double parse_term(void)
{
    double v = parse_factor();
    while (!g_parse_error) {
        skip_ws();
        if (*g_expr == '*') { g_expr++; v *= parse_factor(); }
        else if (*g_expr == '/') {
            g_expr++;
            double d = parse_factor();
            if (d == 0) { g_parse_error = "division by zero"; return 0; }
            v /= d;
        }
        else if (*g_expr == '%') {
            g_expr++;
            long a = (long)v, b = (long)parse_factor();
            if (b == 0) { g_parse_error = "division by zero"; return 0; }
            v = (double)(a % b);
        }
        else break;
    }
    return v;
}

static double parse_expr(void)
{
    double v = parse_term();
    while (!g_parse_error) {
        skip_ws();
        if (*g_expr == '+') { g_expr++; v += parse_term(); }
        else if (*g_expr == '-') { g_expr++; v -= parse_term(); }
        else break;
    }
    return v;
}

static void handle_calc(const char *args)
{
    g_parse_error = NULL;
    g_expr = args;
    double result = parse_expr();
    skip_ws();
    if (g_parse_error || *g_expr != '\0') {
        printf("Bot: Sorry, I couldn't understand that math (%s).\n",
               g_parse_error ? g_parse_error : "extra characters");
        return;
    }
    /* Print whole numbers without decimals. */
    if (result == (long long)result)
        printf("Bot: %s = %lld\n", args, (long long)result);
    else
        printf("Bot: %s = %g\n", args, result);
}

/* ------------------------------------------------------------------ */
/* Help text                                                          */
/* ------------------------------------------------------------------ */

static void print_help(void)
{
    printf("Bot: Here's what I can do:\n"
           "     - chat about greetings, feelings, food, jokes\n"
           "     - calc <expression>   e.g.  calc 12 * (3 + 4)\n"
           "     - time / date\n"
           "     - tell me your name and I'll remember it\n"
           "     - 'fact' for a fun fact, 'joke' for a joke\n"
           "     - 'bye', 'quit' or 'exit' to leave\n");
}

/* ------------------------------------------------------------------ */
/* The rule engine                                                    */
/* ------------------------------------------------------------------ */

static void respond(const char *raw, char *name, int *name_known)
{
    char input[MAX_INPUT];
    strncpy(input, raw, sizeof input - 1);
    input[sizeof input - 1] = '\0';
    str_trim(input);
    str_lower(input);

    if (input[0] == '\0') {
        printf("Bot: Say something! (or type 'help')\n");
        return;
    }

    /* Exact exit commands. */
    if (strcmp(input, "bye") == 0 || strcmp(input, "quit") == 0 ||
        strcmp(input, "exit") == 0 || strcmp(input, "goodbye") == 0) {
        if (*name_known)
            printf("Bot: Goodbye, %s! Come back soon.\n", name);
        else
            pick_from("Goodbye!|See you!|Take care!");
        strcpy(input, "__QUIT__");
        return;
    }

    /* Math: "calc ..." or bare arithmetic-looking input. */
    if (strncmp(input, "calc ", 5) == 0 || strncmp(input, "calculate ", 10) == 0) {
        const char *args = strchr(input, ' ') + 1;
        char pretty[MAX_INPUT];
        strncpy(pretty, args, sizeof pretty - 1);
        pretty[sizeof pretty - 1] = '\0';
        handle_calc(pretty);
        return;
    }

    /* Help. */
    if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
        print_help();
        return;
    }

    /* Time / date. */
    if (has_any(input, "time,what time,clock") && !has_any(input, "times")) {
        time_t now = time(NULL);
        char buf[64];
        strftime(buf, sizeof buf, "%H:%M", localtime(&now));
        printf("Bot: It's %s right now.\n", buf);
        return;
    }
    if (has_any(input, "date,today,day is it,what day")) {
        time_t now = time(NULL);
        char buf[96];
        strftime(buf, sizeof buf, "%A, %B %d, %Y", localtime(&now));
        printf("Bot: Today is %s.\n", buf);
        return;
    }

    /* Name handling. */
    if (has_any(input, "my name is,i am ,i'm ,call me")) {
        const char *marker = strstr(input, "my name is");
        const char *nm = NULL;
        if (marker)
            nm = marker + strlen("my name is");
        else if ((marker = strstr(input, "call me")))
            nm = marker + strlen("call me");
        else if ((marker = strstr(input, "i am ")))
            nm = marker + strlen("i am ");
        else if ((marker = strstr(input, "i'm ")))
            nm = marker + strlen("i'm ");
        if (nm && *nm) {
            strncpy(name, nm, MAX_NAME - 1);
            name[MAX_NAME - 1] = '\0';
            str_trim(name);
            /* drop trailing punctuation */
            size_t l = strlen(name);
            while (l > 0 && strchr("!?.,", name[l - 1]))
                name[--l] = '\0';
            *name_known = 1;
            printf("Bot: Nice to meet you, %s!\n", name);
            return;
        }
        printf("Bot: Hmm, what IS your name then?\n");
        return;
    }
    if (has_any(input, "what is my name,who am i,do you know my name")) {
        if (*name_known)
            printf("Bot: You're %s, of course!\n", name);
        else
            printf("Bot: I don't know yet — tell me! (e.g. \"my name is Sam\")\n");
        return;
    }

    /* Greetings. */
    if (has_any(input, "hello,hi,hey,good morning,good evening,good afternoon,howdy,sup,yo")) {
        if (!*name_known) {
            printf("Bot: %s\n", "Hey there! What's your name?");
            return;
        }
        pick_from("Hello again!|Hey!|Hi there!");
        return;
    }
    if (has_any(input, "how are you,how's it going,how do you feel,whats up,what's up")) {
        pick_from("Running at 100%% CPU and feeling great!|I'm just code, but functioning fine!|All systems go. How about you?");
        return;
    }
    if (has_any(input, "thank,thanks,thx")) {
        pick_from("You're welcome!|Anytime!|Happy to help!");
        return;
    }

    /* Feelings. */
    if (has_any(input, "sad,depressed,unhappy,miserable")) {
        pick_from("I'm sorry to hear that. Want to hear a joke to cheer you up? (type 'joke')|That's rough. Hopefully some code and a joke will help.");
        return;
    }
    if (has_any(input, "happy,excited,great,awesome,good")) {
        pick_from("That's wonderful to hear!|Love the energy!");
        return;
    }

    /* Jokes & facts. */
    if (has_any(input, "joke,funny,make me laugh")) {
        static const char *jokes[] = {
            "Why do C programmers never get Halloween mixed up with Christmas? Because OCT 31 == DEC 25.",
            "A programmer's spouse says: 'Go to the store and buy a loaf of bread. If they have eggs, buy a dozen.' He came back with 12 loaves of bread.",
            "There are 10 kinds of people: those who understand binary and those who don't.",
            "Why did the programmer quit his job? Because he didn't get arrays (a raise)."
        };
        printf("Bot: %s\n", jokes[rand() % (sizeof jokes / sizeof jokes[0])]);
        return;
    }
    if (has_any(input, "fact,fun fact, trivia")) {
        static const char *facts[] = {
            "The first computer bug was an actual moth, found inside a Harvard Mark II in 1947.",
            "C was created by Dennis Ritchie in 1972 at Bell Labs.",
            "The original name of C was 'NB' (New B), as it was a successor to the B language.",
            "In C, 'a[i]' is literally defined as '*(a + i)' — array indexing is pointer arithmetic."
        };
        printf("Bot: %s\n", facts[rand() % (sizeof facts / sizeof facts[0])]);
        return;
    }

    /* Favorites / small talk. */
    if (has_any(input, "your name,who are you,what are you")) {
        printf("Bot: I'm Chip, a chatbot written in pure C. About 300 lines of if-statements with a dream.\n");
        return;
    }
    if (has_any(input, "how old,age")) {
        printf("Bot: I compile fresh every time, so I'm always 0 seconds old.\n");
        return;
    }
    if (has_any(input, "weather")) {
        printf("Bot: I live inside your terminal — no windows here. But I hope it's nice out!\n");
        return;
    }
    if (has_any(input, "love you,marry me")) {
        printf("Bot: Aww. I'm flattered, but I'm committed... to this codebase.\n");
        return;
    }
    if (has_any(input, "python")) {
        printf("Bot: Python is nice, but I was written in C — the language that compiles Python itself!\n");
        return;
    }
    if (has_any(input, "what can you do,capabilities,commands")) {
        print_help();
        return;
    }

    /* Fuzzy fallback: score a few topics by keyword hits. */
    if (has_any(input, "food,eat,hungry,pizza,coffee"))
        pick_from("Food talk! My favorite data type is 'byte'.|I run on electricity, but pizza sounds great.");
    else if (has_any(input, "code,program,write,c language,programming"))
        printf("Bot: If you can write it in C, you can write it anywhere. What are you building?\n");
    else if (has_any(input, "no,yes,ok,fine,sure,maybe"))
        pick_from("Got it.|Understood!|Cool. Anything else?");
    else
        pick_from("Hmm, I don't know about that yet. Type 'help' to see what I can do!"
                  "|I'm just a rule-based bot — rephrase that? (try 'help')"
                  "|That's beyond my if-statements. Ask me for 'help' to see my tricks.");
}

/* ------------------------------------------------------------------ */
/* Main loop                                                          */
/* ------------------------------------------------------------------ */

int main(void)
{
    char name[MAX_NAME] = "";
    int name_known = 0;
    char input[MAX_INPUT];

    srand((unsigned)time(NULL));

    printf("=============================================\n");
    printf("  Chip v1.0 — a chatbot in pure C\n");
    printf("=============================================\n");
    printf("Bot: Hi! I'm Chip. Type 'help' to see what I can do, 'bye' to leave.\n");

    while (1) {
        printf("You: ");
        fflush(stdout);

        if (!fgets(input, sizeof input, stdin)) {
            printf("\nBot: Bye!\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        respond(input, name, &name_known);
        if (strcmp(input, "__QUIT__") == 0)
            break;
    }
    return 0;
}
