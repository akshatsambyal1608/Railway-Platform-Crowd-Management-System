/*
 * storage.c
 * ---------------------------------------------------------------------
 * Sample data, loading and saving the text files, the crowd history log
 * and the report export.
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "railway.h"

/* ===================================================================== */
/* Sample data                                                           */
/* ===================================================================== */

/* Fills the station with built-in demonstration data (fictional). */
void loadSampleData(Station *station)
{
    /* Only the first three fields are set; train info is filled by
     * syncPlatformTrainInfo(). Passenger counts follow the pattern
     * 100 + 150*i, except platform 5 which is raised to 950 so that the
     * demo shows every crowd level, including an overcrowded platform. */
    static const Platform samplePlatforms[] = {
        {.platformNumber = 1, .passengerCount = 100, .maxCapacity = 1000},  /* 10.0% LOW         */
        {.platformNumber = 2, .passengerCount = 250, .maxCapacity = 1000},  /* 25.0% LOW         */
        {.platformNumber = 3, .passengerCount = 450, .maxCapacity = 1000},  /* 45.0% MODERATE    */
        {.platformNumber = 4, .passengerCount = 700, .maxCapacity = 1000},  /* 70.0% HIGH        */
        {.platformNumber = 5, .passengerCount = 950, .maxCapacity = 1000}   /* 95.0% OVERCROWDED */
    };

    static const Train sampleTrains[] = {
        {"12001", "Capital Express",    "Delhi",      "10:30", 1, 0},
        {"12002", "City Beautiful Exp", "Chandigarh", "10:45", 2, 0},
        {"12003", "Jammu Mail",         "Jammu",      "11:00", 3, 10},
        {"12004", "Golden Temple Mail", "Amritsar",   "11:20", 4, 0},
        {"12005", "Valley Express",     "Srinagar",   "11:40", 5, 25}
    };

    int keepColor = station->colorEnabled;
    memset(station, 0, sizeof *station);
    station->colorEnabled = keepColor;

    station->platformCount = (int)(sizeof samplePlatforms / sizeof samplePlatforms[0]);
    for (int i = 0; i < station->platformCount; i++) {
        station->platforms[i] = samplePlatforms[i];
    }

    station->trainCount = (int)(sizeof sampleTrains / sizeof sampleTrains[0]);
    for (int i = 0; i < station->trainCount; i++) {
        station->trains[i] = sampleTrains[i];
    }

    syncPlatformTrainInfo(station);
}

/* ===================================================================== */
/* Loading                                                               */
/* ===================================================================== */

/* Reads the next useful line from a file: removes the line ending, trims
 * spaces, and skips blank lines and comment lines starting with '#'.
 * Returns 1 if a line was read, 0 at end of file. */
static int readDataLine(FILE *fp, char *line, size_t size)
{
    while (fgets(line, (int)size, fp) != NULL) {
        line[strcspn(line, "\r\n")] = '\0';
        trimWhitespace(line);
        if (line[0] != '\0' && line[0] != '#') {
            return 1;
        }
    }
    return 0;
}

/* Loads platforms.txt. Returns 0 if the file cannot be opened, else 1.
 * Bad lines are counted in *skipped and ignored. */
static int loadPlatforms(Station *station, int *skipped)
{
    FILE *fp = fopen(PLATFORMS_FILE, "r");
    if (fp == NULL) {
        return 0;
    }

    char line[LINE_LEN];
    while (readDataLine(fp, line, sizeof line)) {
        Platform p;
        memset(&p, 0, sizeof p);

        /* Format: platformNumber|passengerCount|maxCapacity */
        int fields = sscanf(line, "%d|%d|%d",
                            &p.platformNumber, &p.passengerCount, &p.maxCapacity);

        if (fields != 3 ||
            p.platformNumber < 1 || p.platformNumber > MAX_PLATFORM_NO ||
            p.maxCapacity < 1 || p.maxCapacity > MAX_CAPACITY ||
            p.passengerCount < 0 || p.passengerCount > p.maxCapacity ||
            station->platformCount >= MAX_PLATFORMS ||
            findPlatformIndex(station, p.platformNumber) >= 0) {
            (*skipped)++;
            continue;
        }
        station->platforms[station->platformCount++] = p;
    }

    fclose(fp);
    return 1;
}

/* Loads trains.txt (platforms must be loaded first). The delay field is
 * optional, so older 5-field files still work. Returns 0 if the file
 * cannot be opened, else 1. */
static int loadTrains(Station *station, int *skipped)
{
    FILE *fp = fopen(TRAINS_FILE, "r");
    if (fp == NULL) {
        return 0;
    }

    char line[LINE_LEN];
    while (readDataLine(fp, line, sizeof line)) {
        Train t;
        memset(&t, 0, sizeof t);

        /* Format: number|name|destination|HH:MM|platform[|delayMinutes] */
        int fields = sscanf(line, "%9[^|]|%39[^|]|%39[^|]|%5[^|]|%d|%d",
                            t.trainNumber, t.trainName, t.destination,
                            t.arrivalTime, &t.platformNumber, &t.delayMinutes);
        if (fields == 5) {
            t.delayMinutes = 0;
        }

        if ((fields != 5 && fields != 6) ||
            !isValidTrainNumber(t.trainNumber) ||
            !isValidTime(t.arrivalTime) ||
            t.delayMinutes < 0 || t.delayMinutes > MAX_DELAY ||
            findPlatformIndex(station, t.platformNumber) < 0 ||
            station->trainCount >= MAX_TRAINS ||
            findTrainIndex(station, t.trainNumber) >= 0 ||
            hasPlatformClash(station, t.platformNumber, t.arrivalTime, -1)) {
            (*skipped)++;
            continue;
        }
        station->trains[station->trainCount++] = t;
    }

    fclose(fp);
    return 1;
}

/* Loads both data files. If they are missing or unusable, the caller
 * should fall back to loadSampleData(). */
LoadStatus loadStationData(Station *station)
{
    int skipped = 0;
    int keepColor = station->colorEnabled;

    memset(station, 0, sizeof *station);
    station->colorEnabled = keepColor;

    int platformsOpened = loadPlatforms(station, &skipped);
    int trainsOpened    = loadTrains(station, &skipped);

    /* At least one platform is required; having no trains is allowed. */
    if (!platformsOpened || !trainsOpened || station->platformCount == 0) {
        return LOAD_FAILED;
    }

    syncPlatformTrainInfo(station);
    return (skipped > 0) ? LOAD_PARTIAL : LOAD_OK;
}

/* ===================================================================== */
/* Saving                                                                */
/* ===================================================================== */

/* Writes both data files. Returns 1 on success, 0 on failure. */
int saveStationData(const Station *station)
{
    int ok = 1;

    FILE *fp = fopen(PLATFORMS_FILE, "w");
    if (fp == NULL) {
        return 0;
    }
    fprintf(fp, "# platformNumber|passengerCount|maxCapacity\n");
    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[i];
        fprintf(fp, "%d|%d|%d\n", p->platformNumber, p->passengerCount, p->maxCapacity);
    }
    if (fclose(fp) != 0) {
        ok = 0;
    }

    fp = fopen(TRAINS_FILE, "w");
    if (fp == NULL) {
        return 0;
    }
    fprintf(fp, "# trainNumber|trainName|destination|arrivalTime(HH:MM)|platformNumber|delayMinutes\n");
    for (int i = 0; i < station->trainCount; i++) {
        const Train *t = &station->trains[i];
        fprintf(fp, "%s|%s|%s|%s|%d|%d\n", t->trainNumber, t->trainName,
                t->destination, t->arrivalTime, t->platformNumber, t->delayMinutes);
    }
    if (fclose(fp) != 0) {
        ok = 0;
    }

    return ok;
}

/* ===================================================================== */
/* Crowd history log                                                     */
/* ===================================================================== */

/* Adds one line (timestamp|platform|passengers|capacity) to the history
 * file. Returns 1 on success, 0 on failure. */
int appendHistory(const Platform *platform)
{
    FILE *fp = fopen(HISTORY_FILE, "a");
    if (fp == NULL) {
        return 0;
    }

    char timestamp[TIMESTAMP_LEN];
    getTimestamp(timestamp, sizeof timestamp);
    fprintf(fp, "%s|%d|%d|%d\n", timestamp, platform->platformNumber,
            platform->passengerCount, platform->maxCapacity);

    return fclose(fp) == 0;
}

/* Reads the most recent records (up to max) from the history file, oldest
 * first. platformNumber 0 means "every platform". Invalid lines are
 * ignored. Returns how many records were stored in out[]. */
int loadRecentHistory(int platformNumber, HistoryRecord out[], int max)
{
    FILE *fp = fopen(HISTORY_FILE, "r");
    if (fp == NULL || max <= 0) {
        if (fp != NULL) {
            fclose(fp);
        }
        return 0;
    }

    int count = 0;
    char line[LINE_LEN];

    while (readDataLine(fp, line, sizeof line)) {
        HistoryRecord r;
        memset(&r, 0, sizeof r);

        int fields = sscanf(line, "%19[^|]|%d|%d|%d", r.timestamp,
                            &r.platformNumber, &r.passengerCount, &r.capacity);
        if (fields != 4 || r.capacity < 1 || r.passengerCount < 0 ||
            r.passengerCount > r.capacity) {
            continue;
        }
        if (platformNumber != 0 && r.platformNumber != platformNumber) {
            continue;
        }

        if (count == max) {               /* keep only the newest 'max' */
            memmove(out, out + 1, (size_t)(max - 1) * sizeof out[0]);
            count--;
        }
        out[count++] = r;
    }

    fclose(fp);
    return count;
}

/* Deletes all history. Returns 1 on success. */
int clearHistory(void)
{
    FILE *fp = fopen(HISTORY_FILE, "w");
    if (fp == NULL) {
        return 0;
    }
    fprintf(fp, "# timestamp|platformNumber|passengerCount|capacity\n");
    return fclose(fp) == 0;
}

/* Writes four fictional history snapshots that match the sample data, so
 * the forecast works straight away in a demonstration. */
int writeSampleHistory(void)
{
    static const char *const times[4] = {
        "2026-11-01 09:00:00", "2026-11-01 09:15:00",
        "2026-11-01 09:30:00", "2026-11-01 09:45:00"
    };
    static const int counts[4][5] = {
        { 40, 120, 300, 520, 700},
        { 60, 170, 360, 590, 790},
        { 80, 210, 410, 650, 880},
        {100, 250, 450, 700, 950}
    };

    FILE *fp = fopen(HISTORY_FILE, "w");
    if (fp == NULL) {
        return 0;
    }
    fprintf(fp, "# timestamp|platformNumber|passengerCount|capacity\n");
    for (int t = 0; t < 4; t++) {
        for (int p = 0; p < 5; p++) {
            fprintf(fp, "%s|%d|%d|1000\n", times[t], p + 1, counts[t][p]);
        }
    }
    return fclose(fp) == 0;
}

/* ===================================================================== */
/* Report export                                                         */
/* ===================================================================== */

/* Saves a plain-text crowd report to REPORT_FILE. Returns 1 on success. */
int exportReport(const Station *station)
{
    FILE *fp = fopen(REPORT_FILE, "w");
    if (fp == NULL) {
        return 0;
    }

    char timestamp[TIMESTAMP_LEN];
    getTimestamp(timestamp, sizeof timestamp);

    printRule(fp, '=');
    fprintf(fp, "  RAILWAY PLATFORM CROWD MANAGEMENT SYSTEM - CROWD REPORT\n");
    fprintf(fp, "  Generated: %s\n", timestamp);
    fprintf(fp, "  SIMULATED sample data - not real railway information.\n");
    printRule(fp, '=');

    writeAnalytics(fp, station);

    fprintf(fp, "\n  NOTE: This report is simulated and must NOT replace official\n");
    fprintf(fp, "  railway announcements or instructions from railway staff.\n");

    return fclose(fp) == 0;
}
