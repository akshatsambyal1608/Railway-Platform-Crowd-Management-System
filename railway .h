/*
 * railway.h
 * ---------------------------------------------------------------------
 * Railway Platform Crowd Management System
 * Shared constants, data structures and function prototypes.
 *
 * NOTE: This is a SIMULATION built on sample data. It does not use any
 * real-time railway information.
 * ---------------------------------------------------------------------
 */

#ifndef RAILWAY_H
#define RAILWAY_H

#include <stddef.h>
#include <stdio.h>

/* ----------------------------- Limits ------------------------------- */
#define MAX_PLATFORMS    10     /* maximum platforms the program can hold */
#define MAX_TRAINS       20     /* maximum trains the program can hold    */
#define MAX_PLATFORM_NO  99     /* highest allowed platform number        */
#define MAX_CAPACITY     9999   /* highest allowed platform capacity      */
#define MAX_DELAY        600    /* highest allowed train delay (minutes)  */

#define TRAIN_NUM_LEN    10     /* train number text (including '\0')     */
#define NAME_LEN         40     /* train name / destination text          */
#define TIME_LEN         6      /* "HH:MM" plus '\0'                      */
#define TIMESTAMP_LEN    20     /* "YYYY-MM-DD HH:MM:SS" plus '\0'        */
#define INPUT_LEN        100    /* size of the keyboard input buffer      */
#define LINE_LEN         256    /* size of one line read from a file      */

#define FORECAST_WINDOW    5    /* history records used for a forecast    */
#define HISTORY_VIEW_ROWS  10   /* history rows shown on screen           */

/* ------------------------- Crowd thresholds ------------------------- */
#define MODERATE_START   30     /* LOW        : below 30%                 */
#define HIGH_START       60     /* MODERATE   : 30% to below 60%          */
#define OVERCROWD_START  90     /* HIGH       : 60% to below 90%          */
                                /* OVERCROWDED: 90% and above             */

/* ------------------------------ Files ------------------------------- */
#define PLATFORMS_FILE   "data/platforms.txt"
#define TRAINS_FILE      "data/trains.txt"
#define HISTORY_FILE     "data/history.txt"
#define REPORT_FILE      "data/crowd_report.txt"

/* ----------------------------- Display ------------------------------ */
#define TABLE_WIDTH      72     /* width of every banner and table        */
#define BAR_WIDTH        26     /* characters in the crowd bar            */

/* ------------------------- Data structures -------------------------- */

/* The four crowd levels, in increasing order of crowding. */
typedef enum {
    LEVEL_LOW = 0,
    LEVEL_MODERATE,
    LEVEL_HIGH,
    LEVEL_OVERCROWDED
} CrowdLevel;

/* Result of loading the data files. */
typedef enum {
    LOAD_OK = 0,        /* everything loaded correctly                  */
    LOAD_PARTIAL,       /* loaded, but some bad lines were skipped      */
    LOAD_FAILED         /* files missing/unusable -> use sample data    */
} LoadStatus;

/* How the platform table can be ordered. */
typedef enum {
    SORT_PLATFORM_NUMBER = 0,
    SORT_MOST_CROWDED_FIRST
} PlatformSort;

/* How the train table can be ordered. */
typedef enum {
    SORT_TRAIN_TIME = 0,
    SORT_TRAIN_NUMBER,
    SORT_TRAIN_DESTINATION
} TrainSort;

/* One railway platform.
 * trainNumber, destination, arrivalTime and delayMinutes describe the
 * next scheduled train for the platform. They are filled automatically
 * from the train list (see syncPlatformTrainInfo), so they are never
 * saved twice. */
typedef struct {
    int  platformNumber;
    int  passengerCount;
    int  maxCapacity;
    char trainNumber[TRAIN_NUM_LEN];
    char destination[NAME_LEN];
    char arrivalTime[TIME_LEN];
    int  delayMinutes;
} Platform;

/* One scheduled train. */
typedef struct {
    char trainNumber[TRAIN_NUM_LEN];
    char trainName[NAME_LEN];
    char destination[NAME_LEN];
    char arrivalTime[TIME_LEN];      /* scheduled time, "HH:MM"         */
    int  platformNumber;
    int  delayMinutes;               /* 0 = on time                     */
} Train;

/* Everything the program keeps in memory. It is passed to functions by
 * pointer, so no global variables are needed. */
typedef struct {
    Platform platforms[MAX_PLATFORMS];
    int      platformCount;
    Train    trains[MAX_TRAINS];
    int      trainCount;
    int      colorEnabled;           /* 1 = use ANSI colours            */
} Station;

/* Summary numbers used by the dashboard and analytics report. */
typedef struct {
    int    totalPassengers;
    int    totalCapacity;
    double averageOccupancy;     /* mean of the platform percentages     */
    double overallOccupancy;     /* total passengers / total capacity    */
    int    mostCrowdedIndex;     /* index into platforms[], -1 if none   */
    int    leastCrowdedIndex;
    int    levelCount[4];        /* platforms per CrowdLevel             */
} CrowdStats;

/* One saved snapshot of a platform's crowd (one line of history.txt). */
typedef struct {
    char timestamp[TIMESTAMP_LEN];
    int  platformNumber;
    int  passengerCount;
    int  capacity;
} HistoryRecord;

/* Result of the simple trend-based crowd forecast. */
typedef struct {
    int        valid;            /* 0 if there is too little history     */
    double     averageChange;    /* passengers added per record          */
    int        predictedCount;
    double     predictedPercent;
    CrowdLevel predictedLevel;
} Forecast;

/* ============================ utils.c ================================ */
void clearScreen(void);
int  readLine(char *buffer, size_t size);
void trimWhitespace(char *text);
int  readIntInRange(const char *prompt, int min, int max);
void readFieldString(const char *prompt, char *buffer, size_t size);
void readTimeString(const char *prompt, char *buffer, size_t size);
int  confirmAction(const char *question);
void pauseForEnter(void);
int  isValidTime(const char *text);
int  isValidTrainNumber(const char *text);
int  equalsIgnoreCase(const char *a, const char *b);
int  containsIgnoreCase(const char *text, const char *part);
void getTimestamp(char *out, size_t size);
void formatDelay(int minutes, char *out, size_t size);
void formatExpectedTime(const char *scheduled, int delayMinutes,
                        char *out, size_t size);
int  detectColorSupport(void);
int  wantColor(FILE *out, const Station *station);
void fprintLevelCell(FILE *out, int useColor, CrowdLevel level, int width);
void printRule(FILE *out, char ch);
void printSeparator(FILE *out, const int widths[], int columns);
void printScreenHeader(const char *screenName);
void makeBar(double percent, char *out, int width);

/* ============================ crowd.c ================================ */
double      getOccupancyPercent(const Platform *platform);
CrowdLevel  getCrowdLevel(const Platform *platform);
const char *getLevelName(CrowdLevel level);
void        computeStats(const Station *station, CrowdStats *stats);
int         findPlatformIndex(const Station *station, int platformNumber);
int         findTrainIndex(const Station *station, const char *trainNumber);
int         hasPlatformClash(const Station *station, int platformNumber,
                             const char *arrivalTime, int ignoreIndex);
void        syncPlatformTrainInfo(Station *station);
int         getLeastCrowdedIndex(const Station *station, int excludeIndex);
void        buildRanking(const Station *station, int order[]);
void        sortPlatformIndices(const Station *station, int order[],
                                PlatformSort mode);
void        sortTrainIndices(const Station *station, int order[],
                             TrainSort mode);
void        computeForecast(const Platform *platform,
                            const HistoryRecord records[], int count,
                            Forecast *forecast);

/* =========================== storage.c =============================== */
void       loadSampleData(Station *station);
LoadStatus loadStationData(Station *station);
int        saveStationData(const Station *station);
int        appendHistory(const Platform *platform);
int        loadRecentHistory(int platformNumber, HistoryRecord out[], int max);
int        clearHistory(void);
int        writeSampleHistory(void);
int        exportReport(const Station *station);

/* =========================== screens.c =============================== */
void displayDashboard(const Station *station);
void displayAlerts(const Station *station);
void displayPlatformDetails(const Station *station);
void printPlatformTable(const Station *station, PlatformSort mode);
void printTrainTable(const Station *station, const int indices[], int count);
void displayTrains(const Station *station);
void searchTrains(const Station *station);
void recommendPlatforms(const Station *station);
void displayAnalyticsReport(const Station *station);
void writeAnalytics(FILE *out, const Station *station);
void historyMenu(const Station *station);

/* ============================ manage.c =============================== */
void updatePassengerCount(Station *station);
void simulateCrowdChange(Station *station);
void manageMenu(Station *station);

#endif /* RAILWAY_H */



















