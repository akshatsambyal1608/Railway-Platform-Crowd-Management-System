/*
 * utils.c
 * ---------------------------------------------------------------------
 * Input helpers, string helpers and terminal drawing helpers.
 * ---------------------------------------------------------------------
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include "railway.h"

#define COLOR_RESET "\033[0m"

/* ===================================================================== */
/* Terminal and input                                                    */
/* ===================================================================== */

/* Clears the terminal. Works on Windows ("cls") and Linux/macOS ("clear"). */
void clearScreen(void)
{
    int result;
#ifdef _WIN32
    result = system("cls");
#else
    result = system("clear");
#endif
    (void)result; /* the return value is not important here */
}

/* Reads one line from the keyboard safely (never overflows the buffer).
 * The newline is removed and any extra characters that did not fit are
 * thrown away. Returns 1 if the whole line fitted, 0 if it was too long.
 * If the input stream is closed (Ctrl+D / Ctrl+Z), the program exits
 * politely instead of looping forever. */
int readLine(char *buffer, size_t size)
{
    if (fgets(buffer, (int)size, stdin) == NULL) {
        printf("\n[INFO] Input closed. Exiting program.\n");
        exit(EXIT_SUCCESS);
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        return 1;
    }

    /* No newline: either the line was too long, or it is the last line. */
    int ch;
    int truncated = 0;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        truncated = 1;
    }
    return !truncated;
}

/* Removes spaces/tabs/newlines from both ends of a string (in place). */
void trimWhitespace(char *text)
{
    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }

    size_t start = 0;
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        start++;
    }
    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

/* Keeps asking until the user types a whole number between min and max. */
int readIntInRange(const char *prompt, int min, int max)
{
    char buffer[INPUT_LEN];

    for (;;) {
        printf("%s", prompt);
        int complete = readLine(buffer, sizeof buffer);

        char *end = NULL;
        long value = strtol(buffer, &end, 10);

        /* Skip trailing spaces, then the whole text must have been used. */
        while (end != NULL && *end != '\0' && isspace((unsigned char)*end)) {
            end++;
        }

        if (!complete || buffer[0] == '\0' || end == buffer || *end != '\0') {
            printf("[ERROR] Please enter a valid whole number.\n");
        } else if (value < min || value > max) {
            printf("[ERROR] Please enter a number between %d and %d.\n", min, max);
        } else {
            return (int)value;
        }
    }
}

/* Keeps asking until the user types non-empty text that fits the buffer
 * and does not contain '|' (which is the file field separator). */
void readFieldString(const char *prompt, char *buffer, size_t size)
{
    for (;;) {
        printf("%s", prompt);
        int complete = readLine(buffer, size);
        trimWhitespace(buffer);

        if (!complete) {
            printf("[ERROR] Text is too long (maximum %d characters).\n", (int)size - 1);
        } else if (buffer[0] == '\0') {
            printf("[ERROR] Input cannot be empty.\n");
        } else if (strchr(buffer, '|') != NULL) {
            printf("[ERROR] The '|' character is not allowed.\n");
        } else {
            return;
        }
    }
}

/* Keeps asking until the user types a valid 24-hour time "HH:MM". */
void readTimeString(const char *prompt, char *buffer, size_t size)
{
    for (;;) {
        printf("%s", prompt);
        int complete = readLine(buffer, size);
        trimWhitespace(buffer);

        if (complete && isValidTime(buffer)) {
            return;
        }
        printf("[ERROR] Use 24-hour HH:MM format, for example 09:45.\n");
    }
}

/* Asks a yes/no question. Returns 1 for yes, 0 for no. */
int confirmAction(const char *question)
{
    char prompt[INPUT_LEN];
    snprintf(prompt, sizeof prompt, "%s (1 = Yes, 0 = No): ", question);
    return readIntInRange(prompt, 0, 1);
}

/* Waits for the user to press Enter. */
void pauseForEnter(void)
{
    char buffer[INPUT_LEN];
    printf("\nPress Enter to continue...");
    readLine(buffer, sizeof buffer);
}

/* ===================================================================== */
/* Validation and string helpers                                         */
/* ===================================================================== */

/* Returns 1 if text looks like a valid 24-hour time "HH:MM", else 0. */
int isValidTime(const char *text)
{
    if (strlen(text) != 5 || text[2] != ':') {
        return 0;
    }
    if (!isdigit((unsigned char)text[0]) || !isdigit((unsigned char)text[1]) ||
        !isdigit((unsigned char)text[3]) || !isdigit((unsigned char)text[4])) {
        return 0;
    }
    int hours   = (text[0] - '0') * 10 + (text[1] - '0');
    int minutes = (text[3] - '0') * 10 + (text[4] - '0');
    return hours < 24 && minutes < 60;
}

/* A train number is 1-9 letters or digits (no spaces or symbols). */
int isValidTrainNumber(const char *text)
{
    size_t length = strlen(text);
    if (length == 0 || length >= TRAIN_NUM_LEN) {
        return 0;
    }
    for (size_t i = 0; i < length; i++) {
        if (!isalnum((unsigned char)text[i])) {
            return 0;
        }
    }
    return 1;
}

/* Case-insensitive "are these two strings equal?" */
int equalsIgnoreCase(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

/* Case-insensitive "does text contain part?" (simple substring search). */
int containsIgnoreCase(const char *text, const char *part)
{
    size_t partLength = strlen(part);
    if (partLength == 0) {
        return 1;
    }
    for (; *text != '\0'; text++) {
        size_t i = 0;
        while (i < partLength && text[i] != '\0' &&
               tolower((unsigned char)text[i]) == tolower((unsigned char)part[i])) {
            i++;
        }
        if (i == partLength) {
            return 1;
        }
    }
    return 0;
}

/* Writes the current local date and time as "YYYY-MM-DD HH:MM:SS". */
void getTimestamp(char *out, size_t size)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    if (local == NULL || strftime(out, size, "%Y-%m-%d %H:%M:%S", local) == 0) {
        snprintf(out, size, "%s", "unknown");
    }
}

/* Delay text for tables: "-" when on time, otherwise "+15m". */
void formatDelay(int minutes, char *out, size_t size)
{
    if (minutes <= 0) {
        snprintf(out, size, "%s", "-");
    } else {
        snprintf(out, size, "+%dm", minutes);
    }
}

/* Scheduled "HH:MM" plus a delay, wrapping past midnight. */
void formatExpectedTime(const char *scheduled, int delayMinutes,
                        char *out, size_t size)
{
    int hours   = (scheduled[0] - '0') * 10 + (scheduled[1] - '0');
    int minutes = (scheduled[3] - '0') * 10 + (scheduled[4] - '0');
    int total   = (hours * 60 + minutes + delayMinutes) % (24 * 60);

    snprintf(out, size, "%02d:%02d", total / 60, total % 60);
}

/* ===================================================================== */
/* Colours and drawing                                                   */
/* ===================================================================== */

/* Returns 1 if the terminal probably understands ANSI colour codes. */
int detectColorSupport(void)
{
    if (getenv("NO_COLOR") != NULL) {
        return 0;
    }
#ifdef _WIN32
    /* Windows 10+ understands ANSI codes once this mode is switched on. */
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (console == INVALID_HANDLE_VALUE || !GetConsoleMode(console, &mode)) {
        return 0;
    }
    return SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    const char *term = getenv("TERM");
    return term != NULL && strcmp(term, "dumb") != 0;
#endif
}

/* Colours are used only on the screen (never in exported files). */
int wantColor(FILE *out, const Station *station)
{
    return out == stdout && station->colorEnabled;
}

/* Prints a crowd-level name padded to 'width' characters. The padding is
 * added by hand so that colour codes do not disturb the table columns. */
void fprintLevelCell(FILE *out, int useColor, CrowdLevel level, int width)
{
    static const char *const colors[] = {
        "\033[32m",    /* LOW         - green  */
        "\033[33m",    /* MODERATE    - yellow */
        "\033[35m",    /* HIGH        - magenta */
        "\033[1;31m"   /* OVERCROWDED - bold red */
    };
    const char *name = getLevelName(level);
    int length = (int)strlen(name);

    if (useColor) {
        fputs(colors[level], out);
    }
    fputs(name, out);
    if (useColor) {
        fputs(COLOR_RESET, out);
    }
    for (int i = length; i < width; i++) {
        fputc(' ', out);
    }
}

/* Prints a line of one repeated character, TABLE_WIDTH long. */
void printRule(FILE *out, char ch)
{
    for (int i = 0; i < TABLE_WIDTH; i++) {
        fputc(ch, out);
    }
    fputc('\n', out);
}

/* Prints text centred within TABLE_WIDTH. */
static void printCentered(const char *text)
{
    int padding = (TABLE_WIDTH - (int)strlen(text)) / 2;
    if (padding < 0) {
        padding = 0;
    }
    printf("%*s%s\n", padding, "", text);
}

/* Prints a table border such as +------+-------+ for the given widths. */
void printSeparator(FILE *out, const int widths[], int columns)
{
    fputc('+', out);
    for (int i = 0; i < columns; i++) {
        for (int j = 0; j < widths[i] + 2; j++) {
            fputc('-', out);
        }
        fputc('+', out);
    }
    fputc('\n', out);
}

/* Prints the program banner followed by the name of the current screen. */
void printScreenHeader(const char *screenName)
{
    printRule(stdout, '=');
    printCentered("RAILWAY PLATFORM CROWD MANAGEMENT SYSTEM");
    printCentered("(Simulation - sample data only)");
    printRule(stdout, '-');
    printCentered(screenName);
    printRule(stdout, '=');
}

/* Builds a text bar such as "#########.........." (needs width + 1 bytes). */
void makeBar(double percent, char *out, int width)
{
    int filled = (int)(percent / 100.0 * width + 0.5);
    if (filled < 0) {
        filled = 0;
    }
    if (filled > width) {
        filled = width;
    }
    for (int i = 0; i < width; i++) {
        out[i] = (i < filled) ? '#' : '.';
    }
    out[width] = '\0';
}
