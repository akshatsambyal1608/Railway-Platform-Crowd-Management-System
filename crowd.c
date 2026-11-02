/*
 * crowd.c
 * ---------------------------------------------------------------------
 * Crowd logic: levels, statistics, lookups, sorting, recommendation
 * helpers and the simple trend-based forecast. Nothing here prints.
 * ---------------------------------------------------------------------
 */

#include <string.h>

#include "railway.h"

/* Occupancy as a percentage (0-100) of the platform capacity. */
double getOccupancyPercent(const Platform *platform)
{
    if (platform->maxCapacity <= 0) {
        return 0.0;
    }
    return 100.0 * platform->passengerCount / platform->maxCapacity;
}

/* Decides the crowd level. Integer maths is used so that exact boundaries
 * such as 30%, 60% and 90% are never affected by rounding errors:
 *   LOW         : below 30%
 *   MODERATE    : 30% to below 60%
 *   HIGH        : 60% to below 90%
 *   OVERCROWDED : 90% or above                                          */
CrowdLevel getCrowdLevel(const Platform *platform)
{
    long used     = (long)platform->passengerCount * 100;
    long capacity = platform->maxCapacity;

    if (used >= capacity * OVERCROWD_START) {
        return LEVEL_OVERCROWDED;
    }
    if (used >= capacity * HIGH_START) {
        return LEVEL_HIGH;
    }
    if (used >= capacity * MODERATE_START) {
        return LEVEL_MODERATE;
    }
    return LEVEL_LOW;
}

/* Text name for a crowd level. */
const char *getLevelName(CrowdLevel level)
{
    static const char *const names[] = {"LOW", "MODERATE", "HIGH", "OVERCROWDED"};
    return names[level];
}

/* Fills a CrowdStats structure with totals, averages and extremes. */
void computeStats(const Station *station, CrowdStats *stats)
{
    memset(stats, 0, sizeof *stats);
    stats->mostCrowdedIndex  = -1;
    stats->leastCrowdedIndex = -1;

    if (station->platformCount == 0) {
        return;
    }

    double percentSum = 0.0;
    double highest = 0.0;
    double lowest  = 0.0;

    for (int i = 0; i < station->platformCount; i++) {
        const Platform *p = &station->platforms[i];
        double percent = getOccupancyPercent(p);

        stats->totalPassengers += p->passengerCount;
        stats->totalCapacity   += p->maxCapacity;
        stats->levelCount[getCrowdLevel(p)]++;
        percentSum += percent;

        if (stats->mostCrowdedIndex < 0 || percent > highest) {
            stats->mostCrowdedIndex = i;
            highest = percent;
        }
        if (stats->leastCrowdedIndex < 0 || percent < lowest) {
            stats->leastCrowdedIndex = i;
            lowest = percent;
        }
    }

    stats->averageOccupancy = percentSum / station->platformCount;
    if (stats->totalCapacity > 0) {
        stats->overallOccupancy = 100.0 * stats->totalPassengers / stats->totalCapacity;
    }
}

/* ------------------------------ Lookups ----------------------------- */

/* Returns the array index of a platform number, or -1 if not found. */
int findPlatformIndex(const Station *station, int platformNumber)
{
    for (int i = 0; i < station->platformCount; i++) {
        if (station->platforms[i].platformNumber == platformNumber) {
            return i;
        }
    }
    return -1;
}

/* Returns the array index of a train number (exact, ignoring case),
 * or -1 if not found. */
int findTrainIndex(const Station *station, const char *trainNumber)
{
    for (int i = 0; i < station->trainCount; i++) {
        if (equalsIgnoreCase(station->trains[i].trainNumber, trainNumber)) {
            return i;
        }
    }
    return -1;
}

/* Returns 1 if another train already arrives on the same platform at the
 * same scheduled time (ignoreIndex lets a train skip itself; use -1 for
 * none). */
int hasPlatformClash(const Station *station, int platformNumber,
                     const char *arrivalTime, int ignoreIndex)
{
    for (int i = 0; i < station->trainCount; i++) {
        if (i == ignoreIndex) {
            continue;
        }
        if (station->trains[i].platformNumber == platformNumber &&
            strcmp(station->trains[i].arrivalTime, arrivalTime) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Copies each platform's NEXT train (the earliest scheduled arrival among
 * the trains assigned to it) into the platform record, so the platform
 * table can show train number, destination, time and delay. */
void syncPlatformTrainInfo(Station *station)
{
    for (int i = 0; i < station->platformCount; i++) {
        Platform *p = &station->platforms[i];
        const Train *best = NULL;

        for (int j = 0; j < station->trainCount; j++) {
            const Train *t = &station->trains[j];
            if (t->platformNumber != p->platformNumber) {
                continue;
            }
            /* "HH:MM" strings compare correctly with strcmp. */
            if (best == NULL || strcmp(t->arrivalTime, best->arrivalTime) < 0) {
                best = t;
            }
        }

        if (best != NULL) {
            snprintf(p->trainNumber, sizeof p->trainNumber, "%s", best->trainNumber);
            snprintf(p->destination, sizeof p->destination, "%s", best->destination);
            snprintf(p->arrivalTime, sizeof p->arrivalTime, "%s", best->arrivalTime);
            p->delayMinutes = best->delayMinutes;
        } else {
            snprintf(p->trainNumber, sizeof p->trainNumber, "%s", "-");
            snprintf(p->destination, sizeof p->destination, "%s", "No train");
            snprintf(p->arrivalTime, sizeof p->arrivalTime, "%s", "--:--");
            p->delayMinutes = 0;
        }
    }
}

/* ------------------------ Recommendation helpers -------------------- */

/* Returns the index of the least crowded platform, ignoring excludeIndex
 * (pass -1 to consider every platform). Returns -1 if none qualifies. */
int getLeastCrowdedIndex(const Station *station, int excludeIndex)
{
    int best = -1;
    double bestPercent = 0.0;

    for (int i = 0; i < station->platformCount; i++) {
        if (i == excludeIndex) {
            continue;
        }
        double percent = getOccupancyPercent(&station->platforms[i]);
        if (best < 0 || percent < bestPercent) {
            best = i;
            bestPercent = percent;
        }
    }
    return best;
}

/* Fills order[] with platform indices sorted from least to most crowded
 * (insertion sort). */
void buildRanking(const Station *station, int order[])
{
    for (int i = 0; i < station->platformCount; i++) {
        double currentPercent = getOccupancyPercent(&station->platforms[i]);
        int j = i - 1;

        while (j >= 0 &&
               getOccupancyPercent(&station->platforms[order[j]]) > currentPercent) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = i;
    }
}

/* ------------------------------ Sorting ----------------------------- */

/* Fills order[] with platform indices in the requested order
 * (insertion sort on an index list; the real data is never moved). */
void sortPlatformIndices(const Station *station, int order[], PlatformSort mode)
{
    for (int i = 0; i < station->platformCount; i++) {
        const Platform *current = &station->platforms[i];
        int j = i - 1;

        while (j >= 0) {
            const Platform *other = &station->platforms[order[j]];
            int shouldMove;

            if (mode == SORT_MOST_CROWDED_FIRST) {
                shouldMove = getOccupancyPercent(other) < getOccupancyPercent(current);
            } else {
                shouldMove = other->platformNumber > current->platformNumber;
            }
            if (!shouldMove) {
                break;
            }
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = i;
    }
}

/* Returns a positive number if train a should come after train b. */
static int compareTrains(const Train *a, const Train *b, TrainSort mode)
{
    int result = 0;

    if (mode == SORT_TRAIN_NUMBER) {
        result = strcmp(a->trainNumber, b->trainNumber);
    } else if (mode == SORT_TRAIN_DESTINATION) {
        result = strcmp(a->destination, b->destination);
    }
    if (result == 0) {                    /* default and tie-breaker */
        result = strcmp(a->arrivalTime, b->arrivalTime);
    }
    return result;
}

/* Fills order[] with train indices in the requested order. */
void sortTrainIndices(const Station *station, int order[], TrainSort mode)
{
    for (int i = 0; i < station->trainCount; i++) {
        const Train *current = &station->trains[i];
        int j = i - 1;

        while (j >= 0 && compareTrains(&station->trains[order[j]], current, mode) > 0) {
            order[j + 1] = order[j];
            j--;
        }
        order[j + 1] = i;
    }
}

/* ------------------------------ Forecast ---------------------------- */

/* A very simple "AI-style" forecast: take the average change between the
 * saved history records (a straight-line trend) and add it to the current
 * passenger count. At least two records are needed. */
void computeForecast(const Platform *platform, const HistoryRecord records[],
                     int count, Forecast *forecast)
{
    memset(forecast, 0, sizeof *forecast);
    if (count < 2) {
        return;
    }

    forecast->valid = 1;
    forecast->averageChange =
        (double)(records[count - 1].passengerCount - records[0].passengerCount) /
        (count - 1);

    double predicted = platform->passengerCount + forecast->averageChange;
    int rounded = (int)(predicted + (predicted >= 0.0 ? 0.5 : -0.5));
    if (rounded < 0) {
        rounded = 0;
    }
    if (rounded > platform->maxCapacity) {
        rounded = platform->maxCapacity;
    }

    Platform future = *platform;
    future.passengerCount = rounded;

    forecast->predictedCount   = rounded;
    forecast->predictedPercent = getOccupancyPercent(&future);
    forecast->predictedLevel   = getCrowdLevel(&future);
}
