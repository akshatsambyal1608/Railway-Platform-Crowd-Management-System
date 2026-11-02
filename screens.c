/*
 * screens.c
 * ---------------------------------------------------------------------
 * Everything that DISPLAYS information: dashboard, platform and train
 * tables, search, recommendations, analytics, history and forecast.
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "railway.h"

/* ===================================================================== */
/* Shared tables                                                         */
/* ===================================================================== */

/* Platform-wise crowd table (used by dashboard, report and export).
 * Colours are only used when printing to the screen. */
static void printStatusTable(FILE *out, const Station *station)
{
    const int widths[] = {4, 9, 6, BAR_WIDTH, 11};
    int color = wantColor(out, station);
    char bar[BAR_WIDTH + 1];

    printSeparator(out, widths, 5);
    fprintf(out, "| %-4s | %-9s | %-6s | %-*s | %-11s |\n",
            "Plat", "Pass/Cap", "Occ %", BAR_WIDTH, "Crowd Bar", "Status");
    printSeparator(out, widths, 5);

    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[i];
        double percent = getOccupancyPercent(p);
        makeBar(percent, bar, BAR_WIDTH);

        fprintf(out, "| %-4d | %4d/%-4d | %5.1f%% | %-*s | ",
                p->platformNumber, p->passengerCount, p->maxCapacity,
                percent, BAR_WIDTH, bar);
        fprintLevelCell(out, color, getCrowdLevel(p), 11);
        fprintf(out, " |\n");
    }
    printSeparator(out, widths, 5);
}

/* Platform table with next train, delay and status, in a chosen order. */
void printPlatformTable(const Station *station, PlatformSort mode)
{
    const int widths[] = {4, 6, 10, 5, 5, 9, 11};
    int order[MAX_PLATFORMS];
    char delay[16];
    int color = wantColor(stdout, station);

    sortPlatformIndices(station, order, mode);

    printSeparator(stdout, widths, 7);
    printf("| %-4s | %-6s | %-10s | %-5s | %-5s | %-9s | %-11s |\n",
           "Plat", "Train", "Dest.", "Time", "Delay", "Pass/Cap", "Status");
    printSeparator(stdout, widths, 7);

    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[order[i]];
        formatDelay(p->delayMinutes, delay, sizeof delay);

        printf("| %-4d | %-6.6s | %-10.10s | %-5s | %-5s | %4d/%-4d | ",
               p->platformNumber, p->trainNumber, p->destination,
               p->arrivalTime, delay, p->passengerCount, p->maxCapacity);
        fprintLevelCell(stdout, color, getCrowdLevel(p), 11);
        printf(" |\n");
    }
    printSeparator(stdout, widths, 7);
}

/* Prints a table of the trains whose indices are listed in indices[]. */
void printTrainTable(const Station *station, const int indices[], int count)
{
    const int widths[] = {8, 19, 12, 5, 5, 4};
    char delay[16];

    printSeparator(stdout, widths, 6);
    printf("| %-8s | %-19s | %-12s | %-5s | %-5s | %-4s |\n",
           "Train No", "Train Name", "Destination", "Time", "Delay", "Plat");
    printSeparator(stdout, widths, 6);

    for (int i = 0; i < count; i++) {
        const Train *t = &station->trains[indices[i]];
        formatDelay(t->delayMinutes, delay, sizeof delay);

        printf("| %-8.8s | %-19.19s | %-12.12s | %-5s | %-5s | %-4d |\n",
               t->trainNumber, t->trainName, t->destination,
               t->arrivalTime, delay, t->platformNumber);
    }
    printSeparator(stdout, widths, 6);
}

/* Prints a one-line description of the next train on a platform, for
 * example "12001 - Delhi". */
static void formatNextTrain(const Platform *p, char *out, size_t size)
{
    if (strcmp(p->trainNumber, "-") == 0) {
        snprintf(out, size, "%s", "No train scheduled");
    } else {
        snprintf(out, size, "%s - %s", p->trainNumber, p->destination);
    }
}

/* ===================================================================== */
/* Alerts and dashboard                                                  */
/* ===================================================================== */

/* Shows an alert for every overcrowded platform, with an alternative. */
void displayAlerts(const Station *station)
{
    int anyAlert = 0;

    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[i];
        if (getCrowdLevel(p) != LEVEL_OVERCROWDED) {
            continue;
        }

        anyAlert = 1;
        printf("  !!! ALERT: Platform %d is OVERCROWDED (%.1f%% - %d of %d passengers)\n",
               p->platformNumber, getOccupancyPercent(p),
               p->passengerCount, p->maxCapacity);

        int alt = getLeastCrowdedIndex(station, i);
        if (alt >= 0 && getCrowdLevel(&station->platforms[alt]) != LEVEL_OVERCROWDED) {
            printf("      -> Suggested alternative: Platform %d (%.1f%%, %s)\n",
                   station->platforms[alt].platformNumber,
                   getOccupancyPercent(&station->platforms[alt]),
                   getLevelName(getCrowdLevel(&station->platforms[alt])));
        } else {
            printf("      -> No uncrowded alternative available. Please follow staff guidance.\n");
        }
    }

    if (!anyAlert) {
        printf("  [OK] No platform is overcrowded right now.\n");
    }
}

/* Main dashboard: summary figures, status table and alerts. */
void displayDashboard(const Station *station)
{
    CrowdStats stats;
    char timestamp[TIMESTAMP_LEN];

    computeStats(station, &stats);
    getTimestamp(timestamp, sizeof timestamp);

    printScreenHeader("DASHBOARD");
    printf("\n  Date and time         : %s\n", timestamp);
    printf("  Total platforms       : %d\n", station->platformCount);
    printf("  Total trains          : %d\n", station->trainCount);
    printf("  Total passengers      : %d (capacity %d)\n",
           stats.totalPassengers, stats.totalCapacity);
    printf("  Average occupancy     : %.1f%%\n", stats.averageOccupancy);
    printf("  Overcrowded platforms : %d\n\n", stats.levelCount[LEVEL_OVERCROWDED]);

    printStatusTable(stdout, station);

    printf("\n  ALERTS\n");
    displayAlerts(station);
}

/* Platform details screen: asks how to sort, then shows the table. */
void displayPlatformDetails(const Station *station)
{
    printScreenHeader("PLATFORM DETAILS");
    printf("\n  Sort platforms by:\n");
    printf("  1. Platform number\n");
    printf("  2. Most crowded first\n\n");

    int choice = readIntInRange("Enter your choice (1-2): ", 1, 2);
    PlatformSort mode = (choice == 2) ? SORT_MOST_CROWDED_FIRST : SORT_PLATFORM_NUMBER;

    printf("\n  (Train columns show the next scheduled train on each platform)\n\n");
    printPlatformTable(station, mode);
}

/* ===================================================================== */
/* Trains                                                                */
/* ===================================================================== */

/* Train list screen: asks how to sort, then shows every train. */
void displayTrains(const Station *station)
{
    int order[MAX_TRAINS];

    printScreenHeader("TRAIN INFORMATION");

    if (station->trainCount == 0) {
        printf("\n  No trains are available. Add trains from the Manage menu.\n");
        return;
    }

    printf("\n  Sort trains by:\n");
    printf("  1. Arrival time\n");
    printf("  2. Train number\n");
    printf("  3. Destination\n\n");

    int choice = readIntInRange("Enter your choice (1-3): ", 1, 3);
    TrainSort mode = SORT_TRAIN_TIME;
    if (choice == 2) {
        mode = SORT_TRAIN_NUMBER;
    } else if (choice == 3) {
        mode = SORT_TRAIN_DESTINATION;
    }

    sortTrainIndices(station, order, mode);
    printf("\n  %d trains (delay column shows minutes late)\n\n", station->trainCount);
    printTrainTable(station, order, station->trainCount);
}

/* Search trains by number (exact), destination (partial) or platform. */
void searchTrains(const Station *station)
{
    int matches[MAX_TRAINS];
    int matchCount = 0;
    char query[INPUT_LEN];

    printScreenHeader("SEARCH TRAINS");
    printf("\n  1. Search by train number\n");
    printf("  2. Search by destination\n");
    printf("  3. Search by platform number\n");
    printf("  0. Back to main menu\n\n");

    int choice = readIntInRange("Enter your choice (0-3): ", 0, 3);
    if (choice == 0) {
        return;
    }

    if (choice == 1) {
        readFieldString("Enter train number: ", query, sizeof query);
        int index = findTrainIndex(station, query);
        if (index >= 0) {
            matches[matchCount++] = index;
        }
    } else if (choice == 2) {
        readFieldString("Enter destination (full or partial name): ", query, sizeof query);
        for (int i = 0; i < station->trainCount; i++) {
            if (containsIgnoreCase(station->trains[i].destination, query)) {
                matches[matchCount++] = i;
            }
        }
    } else {
        int number = readIntInRange("Enter platform number: ", 1, MAX_PLATFORM_NO);
        snprintf(query, sizeof query, "platform %d", number);
        for (int i = 0; i < station->trainCount; i++) {
            if (station->trains[i].platformNumber == number) {
                matches[matchCount++] = i;
            }
        }
    }

    if (matchCount == 0) {
        printf("\n[INFO] No trains found for \"%s\".\n", query);
        return;
    }

    printf("\n[OK] %d train(s) found for \"%s\":\n\n", matchCount, query);
    printTrainTable(station, matches, matchCount);
}

/* ===================================================================== */
/* Smart platform recommendation                                         */
/* ===================================================================== */

/* Prints the disclaimer that must accompany every recommendation. */
static void printDisclaimer(void)
{
    printf("\n  NOTE: These recommendations are SIMULATED from sample data.\n");
    printf("  They must NOT replace official railway announcements or\n");
    printf("  instructions from railway staff.\n");
}

/* Option 1: the single least crowded platform. */
static void showLeastCrowded(const Station *station)
{
    int index = getLeastCrowdedIndex(station, -1);
    if (index < 0) {
        printf("\n[INFO] No platforms available.\n");
        return;
    }

    const Platform *p = &station->platforms[index];
    char nextTrain[64];
    formatNextTrain(p, nextTrain, sizeof nextTrain);

    printf("\n  Recommended platform : Platform %d\n", p->platformNumber);
    printf("  Occupancy            : %.1f%% (%d of %d passengers)\n",
           getOccupancyPercent(p), p->passengerCount, p->maxCapacity);
    printf("  Crowd level          : %s\n", getLevelName(getCrowdLevel(p)));
    printf("  Next train           : %s\n", nextTrain);

    if (getCrowdLevel(p) == LEVEL_OVERCROWDED) {
        printf("\n  [WARNING] Every platform is overcrowded. Please follow staff guidance.\n");
    }
}

/* Option 2: all platforms ranked from least to most crowded. */
static void showRanking(const Station *station)
{
    const int widths[] = {4, 4, 6, 11, 31};
    int order[MAX_PLATFORMS];
    char nextTrain[64];
    int color = wantColor(stdout, station);

    buildRanking(station, order);

    printf("\n");
    printSeparator(stdout, widths, 5);
    printf("| %-4s | %-4s | %-6s | %-11s | %-31s |\n",
           "Rank", "Plat", "Occ %", "Status", "Next Train");
    printSeparator(stdout, widths, 5);

    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[order[i]];
        formatNextTrain(p, nextTrain, sizeof nextTrain);

        printf("| %-4d | %-4d | %5.1f%% | ", i + 1, p->platformNumber,
               getOccupancyPercent(p));
        fprintLevelCell(stdout, color, getCrowdLevel(p), 11);
        printf(" | %-31.31s |\n", nextTrain);
    }
    printSeparator(stdout, widths, 5);
    printf("  (Rank 1 = least crowded)\n");
}

/* Option 3: suggest alternatives for one specific platform. */
static void showAlternatives(const Station *station)
{
    int number = readIntInRange("\nEnter your platform number: ", 1, MAX_PLATFORM_NO);
    int index = findPlatformIndex(station, number);

    if (index < 0) {
        printf("[ERROR] Platform %d does not exist.\n", number);
        return;
    }

    const Platform *current = &station->platforms[index];
    CrowdLevel level = getCrowdLevel(current);
    double currentPercent = getOccupancyPercent(current);

    printf("\n  Platform %d is %s (%.1f%%).\n", number, getLevelName(level), currentPercent);

    if (level != LEVEL_HIGH && level != LEVEL_OVERCROWDED) {
        printf("  This platform is comfortable. No change is needed.\n");
        return;
    }

    int order[MAX_PLATFORMS];
    int shown = 0;
    buildRanking(station, order);

    printf("\n  Less crowded alternatives:\n");
    for (int i = 0; i < station->platformCount && shown < 3; i++) {
        const Platform *p = &station->platforms[order[i]];
        if (order[i] == index || getCrowdLevel(p) == LEVEL_OVERCROWDED ||
            getOccupancyPercent(p) >= currentPercent) {
            continue;
        }
        char nextTrain[64];
        formatNextTrain(p, nextTrain, sizeof nextTrain);
        printf("   %d. Platform %d - %.1f%% (%s), next train: %s\n",
               ++shown, p->platformNumber, getOccupancyPercent(p),
               getLevelName(getCrowdLevel(p)), nextTrain);
    }

    if (shown == 0) {
        printf("   None available - all other platforms are as crowded or more.\n");
    }
}

/* Option 4: where should a passenger for a train wait? */
static void showTrainAdvice(const Station *station)
{
    char number[INPUT_LEN];

    readFieldString("\nEnter your train number: ", number, sizeof number);
    int trainIndex = findTrainIndex(station, number);
    if (trainIndex < 0) {
        printf("[ERROR] Train %s was not found.\n", number);
        return;
    }

    const Train *t = &station->trains[trainIndex];
    int platformIndex = findPlatformIndex(station, t->platformNumber);
    if (platformIndex < 0) {
        printf("[ERROR] The train's platform %d does not exist.\n", t->platformNumber);
        return;
    }

    const Platform *p = &station->platforms[platformIndex];
    char expected[TIME_LEN];
    formatExpectedTime(t->arrivalTime, t->delayMinutes, expected, sizeof expected);

    printf("\n  Train %s (%s) to %s\n", t->trainNumber, t->trainName, t->destination);
    printf("  Scheduled %s, expected %s", t->arrivalTime, expected);
    if (t->delayMinutes > 0) {
        printf(" (delayed by %d min)", t->delayMinutes);
    }
    printf("\n  Arrives on Platform %d, currently %s (%.1f%%).\n",
           p->platformNumber, getLevelName(getCrowdLevel(p)), getOccupancyPercent(p));

    CrowdLevel level = getCrowdLevel(p);
    if (level == LEVEL_HIGH || level == LEVEL_OVERCROWDED) {
        int alt = getLeastCrowdedIndex(station, platformIndex);
        printf("  The platform is crowded - wait where there is more room, and\n");
        printf("  move to Platform %d (%.1f%%) only if staff announce it.\n",
               station->platforms[alt].platformNumber,
               getOccupancyPercent(&station->platforms[alt]));
    } else {
        printf("  The platform is comfortable.\n");
    }
}

/* Smart recommendation menu. */
void recommendPlatforms(const Station *station)
{
    printScreenHeader("SMART PLATFORM RECOMMENDATION");

    if (station->platformCount == 0) {
        printf("\n[INFO] No platforms available.\n");
        return;
    }

    printf("\n  1. Find the least crowded platform\n");
    printf("  2. Rank all platforms by crowd percentage\n");
    printf("  3. Suggest alternatives for a specific platform\n");
    printf("  4. Advice for a specific train\n");
    printf("  0. Back to main menu\n\n");

    int choice = readIntInRange("Enter your choice (0-4): ", 0, 4);

    switch (choice) {
        case 1: showLeastCrowded(station); break;
        case 2: showRanking(station);      break;
        case 3: showAlternatives(station); break;
        case 4: showTrainAdvice(station);  break;
        default: return;
    }
    printDisclaimer();
}

/* ===================================================================== */
/* Analytics                                                             */
/* ===================================================================== */

/* Writes the full analytics report to 'out' (the screen or a file). */
void writeAnalytics(FILE *out, const Station *station)
{
    CrowdStats stats;
    computeStats(station, &stats);

    if (station->platformCount == 0) {
        fprintf(out, "\n  No platform data available.\n");
        return;
    }

    fprintf(out, "\n  PLATFORM-WISE REPORT\n");
    printStatusTable(out, station);

    const Platform *most  = &station->platforms[stats.mostCrowdedIndex];
    const Platform *least = &station->platforms[stats.leastCrowdedIndex];

    fprintf(out, "\n  SUMMARY\n");
    fprintf(out, "  Total passengers      : %d\n", stats.totalPassengers);
    fprintf(out, "  Total capacity        : %d\n", stats.totalCapacity);
    fprintf(out, "  Average occupancy     : %.1f%% (mean of all platforms)\n",
            stats.averageOccupancy);
    fprintf(out, "  Overall station load  : %.1f%% (passengers / capacity)\n",
            stats.overallOccupancy);
    fprintf(out, "  Most crowded platform : Platform %d (%.1f%%)\n",
            most->platformNumber, getOccupancyPercent(most));
    fprintf(out, "  Least crowded platform: Platform %d (%.1f%%)\n",
            least->platformNumber, getOccupancyPercent(least));

    fprintf(out, "\n  OVERCROWDING STATISTICS\n");
    for (int level = LEVEL_LOW; level <= LEVEL_OVERCROWDED; level++) {
        fprintf(out, "  %-12s platforms : %d\n", getLevelName((CrowdLevel)level),
                stats.levelCount[level]);
    }

    double overcrowdedShare = 100.0 * stats.levelCount[LEVEL_OVERCROWDED] / station->platformCount;
    fprintf(out, "  Share of platforms overcrowded : %.1f%%\n", overcrowdedShare);

    if (stats.levelCount[LEVEL_OVERCROWDED] > 0) {
        fprintf(out, "  Overcrowded platform numbers   :");
        for (int i = 0; i < station->platformCount; i++) {
            if (getCrowdLevel(&station->platforms[i]) == LEVEL_OVERCROWDED) {
                fprintf(out, " %d", station->platforms[i].platformNumber);
            }
        }
        fprintf(out, "\n");
    }
}

void displayAnalyticsReport(const Station *station)
{
    printScreenHeader("CROWD ANALYTICS REPORT");
    writeAnalytics(stdout, station);
}

/* ===================================================================== */
/* History and forecast                                                  */
/* ===================================================================== */

/* Shows the most recent saved snapshots for one platform or all. */
static void showHistory(const Station *station)
{
    const int widths[] = {19, 4, 9, 6, 18};
    HistoryRecord records[HISTORY_VIEW_ROWS];
    int color = wantColor(stdout, station);

    int number = readIntInRange("\nPlatform number (0 = all platforms): ", 0, MAX_PLATFORM_NO);
    if (number != 0 && findPlatformIndex(station, number) < 0) {
        printf("[ERROR] Platform %d does not exist.\n", number);
        return;
    }

    int count = loadRecentHistory(number, records, HISTORY_VIEW_ROWS);
    if (count == 0) {
        printf("\n[INFO] No history saved yet. Update counts or run a simulation first.\n");
        return;
    }

    printf("\n  Last %d record(s), oldest first:\n\n", count);
    printSeparator(stdout, widths, 5);
    printf("| %-19s | %-4s | %-9s | %-6s | %-18s |\n",
           "Time", "Plat", "Pass/Cap", "Occ %", "Status");
    printSeparator(stdout, widths, 5);

    for (int i = 0; i < count; i++) {
        Platform snapshot;
        memset(&snapshot, 0, sizeof snapshot);
        snapshot.passengerCount = records[i].passengerCount;
        snapshot.maxCapacity    = records[i].capacity;

        printf("| %-19s | %-4d | %4d/%-4d | %5.1f%% | ", records[i].timestamp,
               records[i].platformNumber, records[i].passengerCount,
               records[i].capacity, getOccupancyPercent(&snapshot));
        fprintLevelCell(stdout, color, getCrowdLevel(&snapshot), 18);
        printf(" |\n");
    }
    printSeparator(stdout, widths, 5);
}

/* Forecasts the next crowd level of every platform from its history. */
static void showForecast(const Station *station)
{
    const int widths[] = {4, 7, 10, 9, 7, 16};
    HistoryRecord records[FORECAST_WINDOW];
    int color = wantColor(stdout, station);
    int warnings = 0;
    char change[24];

    printf("\n  Forecast = current count + average change per saved record\n");
    printf("  (uses the last %d history records of each platform)\n\n", FORECAST_WINDOW);

    printSeparator(stdout, widths, 6);
    printf("| %-4s | %-7s | %-10s | %-9s | %-7s | %-16s |\n",
           "Plat", "Now %", "Avg change", "Predicted", "Pred %", "Forecast level");
    printSeparator(stdout, widths, 6);

    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[i];
        Forecast f;

        int count = loadRecentHistory(p->platformNumber, records, FORECAST_WINDOW);
        computeForecast(p, records, count, &f);

        printf("| %-4d | %6.1f%% | ", p->platformNumber, getOccupancyPercent(p));
        if (f.valid) {
            snprintf(change, sizeof change, "%+.1f", f.averageChange);
            printf("%-10s | %-9d | %6.1f%% | ", change, f.predictedCount, f.predictedPercent);
            fprintLevelCell(stdout, color, f.predictedLevel, 16);
            printf(" |\n");

            if (f.predictedLevel == LEVEL_OVERCROWDED &&
                getCrowdLevel(p) != LEVEL_OVERCROWDED) {
                warnings++;
            }
        } else {
            printf("%-10s | %-9s | %-7s | %-16s |\n", "-", "-", "-", "Not enough data");
        }
    }
    printSeparator(stdout, widths, 6);

    /* Early warnings: platforms heading for overcrowding. */
    for (int i = 0; i < station->platformCount && warnings > 0; i++) {
        const Platform *p = &station->platforms[i];
        Forecast f;
        int count = loadRecentHistory(p->platformNumber, records, FORECAST_WINDOW);
        computeForecast(p, records, count, &f);

        if (f.valid && f.predictedLevel == LEVEL_OVERCROWDED &&
            getCrowdLevel(p) != LEVEL_OVERCROWDED) {
            printf("  !!! EARLY WARNING: Platform %d may become OVERCROWDED soon.\n",
                   p->platformNumber);
        }
    }

    printf("\n  NOTE: A forecast is a simple straight-line estimate from SIMULATED\n");
    printf("  data. It is not a real prediction and must NOT replace official\n");
    printf("  railway announcements or instructions from railway staff.\n");
}

/* Menu for the history and forecast features. */
void historyMenu(const Station *station)
{
    printScreenHeader("CROWD HISTORY AND FORECAST");
    printf("\n  1. View crowd history\n");
    printf("  2. Forecast next crowd levels\n");
    printf("  0. Back to main menu\n\n");

    int choice = readIntInRange("Enter your choice (0-2): ", 0, 2);
    if (choice == 1) {
        showHistory(station);
    } else if (choice == 2) {
        showForecast(station);
    }
}
