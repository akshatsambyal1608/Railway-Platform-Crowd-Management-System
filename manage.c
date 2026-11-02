/*
 * manage.c
 * ---------------------------------------------------------------------
 * Features that CHANGE data: passenger updates, crowd simulation, and the
 * Manage menu (add/remove platforms and trains, capacity, delays).
 * Every change is saved to the text files straight away.
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "railway.h"

/* Saves the data files and reports the result. */
static void saveWithMessage(const Station *station)
{
    if (saveStationData(station)) {
        printf("[OK] Changes saved to file.\n");
    } else {
        printf("[ERROR] Could not save to file. Changes exist only in memory.\n");
    }
}

/* Adds a history record and warns if that fails. */
static void logHistory(const Platform *platform)
{
    if (!appendHistory(platform)) {
        printf("[WARNING] Could not write to the history log.\n");
    }
}

/* Asks for a platform number and returns its index, or -1 if the user
 * cancelled or the platform does not exist (a message is printed). */
static int selectPlatformIndex(const Station *station, const char *prompt)
{
    int number = readIntInRange(prompt, 0, MAX_PLATFORM_NO);
    if (number == 0) {
        printf("[INFO] Cancelled.\n");
        return -1;
    }

    int index = findPlatformIndex(station, number);
    if (index < 0) {
        printf("[ERROR] Platform %d does not exist.\n", number);
    }
    return index;
}

/* Asks for a train number and returns its index, or -1 if not found. */
static int selectTrainIndex(const Station *station)
{
    char number[INPUT_LEN];

    readFieldString("Enter train number: ", number, sizeof number);
    int index = findTrainIndex(station, number);
    if (index < 0) {
        printf("[ERROR] Train %s was not found.\n", number);
    }
    return index;
}

/* ===================================================================== */
/* Passenger updates and simulation                                      */
/* ===================================================================== */

/* Lets the user change the passenger count of one platform. */
void updatePassengerCount(Station *station)
{
    printScreenHeader("UPDATE PASSENGER COUNT");
    printf("\n");
    printPlatformTable(station, SORT_PLATFORM_NUMBER);

    int index = selectPlatformIndex(station, "\nEnter platform number to update (0 to cancel): ");
    if (index < 0) {
        return;
    }

    Platform *p = &station->platforms[index];
    char prompt[80];
    snprintf(prompt, sizeof prompt, "Enter new passenger count (0-%d): ", p->maxCapacity);
    p->passengerCount = readIntInRange(prompt, 0, p->maxCapacity);

    CrowdLevel level = getCrowdLevel(p);
    printf("\n[OK] Platform %d updated: %d/%d passengers (%.1f%%) - %s\n",
           p->platformNumber, p->passengerCount, p->maxCapacity,
           getOccupancyPercent(p), getLevelName(level));

    if (level == LEVEL_OVERCROWDED) {
        printf("\n  *** WARNING: Platform %d has become OVERCROWDED! ***\n", p->platformNumber);
        displayAlerts(station);
    }

    logHistory(p);
    saveWithMessage(station);
}

/* Randomly changes every platform's crowd to imitate passengers arriving
 * and leaving. Each platform changes by up to +/-25% of its capacity. */
void simulateCrowdChange(Station *station)
{
    printScreenHeader("SIMULATE CROWD CHANGE");
    printf("\n  Passengers arrive and leave... (random simulation)\n\n");

    for (int i = 0; i < station->platformCount; i++) {
        Platform *p = &station->platforms[i];
        int range = p->maxCapacity / 4;
        int change = (rand() % (2 * range + 1)) - range;
        int oldCount = p->passengerCount;

        p->passengerCount += change;
        if (p->passengerCount < 0) {
            p->passengerCount = 0;
        }
        if (p->passengerCount > p->maxCapacity) {
            p->passengerCount = p->maxCapacity;
        }

        printf("  Platform %-2d : %4d -> %4d passengers (%5.1f%%, %s)\n",
               p->platformNumber, oldCount, p->passengerCount,
               getOccupancyPercent(p), getLevelName(getCrowdLevel(p)));
        logHistory(p);
    }

    printf("\n  ALERTS\n");
    displayAlerts(station);
    printf("\n");
    saveWithMessage(station);
}

/* ===================================================================== */
/* Platform management                                                   */
/* ===================================================================== */

static void addPlatform(Station *station)
{
    printScreenHeader("ADD PLATFORM");

    if (station->platformCount >= MAX_PLATFORMS) {
        printf("\n[ERROR] The limit of %d platforms has been reached.\n", MAX_PLATFORMS);
        return;
    }

    int number = readIntInRange("\nNew platform number (1-99): ", 1, MAX_PLATFORM_NO);
    if (findPlatformIndex(station, number) >= 0) {
        printf("[ERROR] Platform %d already exists.\n", number);
        return;
    }

    int capacity = readIntInRange("Maximum capacity (1-9999): ", 1, MAX_CAPACITY);

    char prompt[80];
    snprintf(prompt, sizeof prompt, "Current passenger count (0-%d): ", capacity);
    int passengers = readIntInRange(prompt, 0, capacity);

    /* Add at the end, then move it left until the numbers are in order. */
    int i = station->platformCount++;
    memset(&station->platforms[i], 0, sizeof station->platforms[i]);
    station->platforms[i].platformNumber = number;
    station->platforms[i].passengerCount = passengers;
    station->platforms[i].maxCapacity    = capacity;

    while (i > 0 && station->platforms[i - 1].platformNumber > station->platforms[i].platformNumber) {
        Platform temp = station->platforms[i - 1];
        station->platforms[i - 1] = station->platforms[i];
        station->platforms[i] = temp;
        i--;
    }

    syncPlatformTrainInfo(station);
    printf("\n[OK] Platform %d added (%d/%d passengers).\n", number, passengers, capacity);
    logHistory(&station->platforms[findPlatformIndex(station, number)]);
    saveWithMessage(station);
}

static void removePlatform(Station *station)
{
    printScreenHeader("REMOVE PLATFORM");
    printf("\n");
    printPlatformTable(station, SORT_PLATFORM_NUMBER);

    if (station->platformCount <= 1) {
        printf("\n[ERROR] At least one platform must remain.\n");
        return;
    }

    int index = selectPlatformIndex(station, "\nPlatform number to remove (0 to cancel): ");
    if (index < 0) {
        return;
    }

    int number = station->platforms[index].platformNumber;
    int assigned = 0;
    for (int i = 0; i < station->trainCount; i++) {
        if (station->trains[i].platformNumber == number) {
            assigned++;
        }
    }
    if (assigned > 0) {
        printf("[ERROR] Platform %d still has %d train(s). Reassign or remove them first.\n",
               number, assigned);
        return;
    }

    char question[INPUT_LEN];
    snprintf(question, sizeof question, "Really remove platform %d?", number);
    if (!confirmAction(question)) {
        printf("[INFO] Removal cancelled.\n");
        return;
    }

    for (int i = index; i < station->platformCount - 1; i++) {
        station->platforms[i] = station->platforms[i + 1];
    }
    station->platformCount--;

    printf("[OK] Platform %d removed.\n", number);
    saveWithMessage(station);
}

static void changeCapacity(Station *station)
{
    printScreenHeader("CHANGE PLATFORM CAPACITY");
    printf("\n");
    printPlatformTable(station, SORT_PLATFORM_NUMBER);

    int index = selectPlatformIndex(station, "\nPlatform number (0 to cancel): ");
    if (index < 0) {
        return;
    }

    Platform *p = &station->platforms[index];
    int minimum = (p->passengerCount > 0) ? p->passengerCount : 1;

    char prompt[100];
    snprintf(prompt, sizeof prompt, "New capacity (%d-%d, cannot be below current passengers): ",
             minimum, MAX_CAPACITY);
    p->maxCapacity = readIntInRange(prompt, minimum, MAX_CAPACITY);

    printf("\n[OK] Platform %d capacity is now %d (%.1f%% - %s).\n",
           p->platformNumber, p->maxCapacity, getOccupancyPercent(p),
           getLevelName(getCrowdLevel(p)));
    logHistory(p);
    saveWithMessage(station);
}

/* ===================================================================== */
/* Train management                                                      */
/* ===================================================================== */

static void addTrain(Station *station)
{
    Train t;
    memset(&t, 0, sizeof t);

    printScreenHeader("ADD TRAIN");

    if (station->trainCount >= MAX_TRAINS) {
        printf("\n[ERROR] The limit of %d trains has been reached.\n", MAX_TRAINS);
        return;
    }

    printf("\n");
    for (;;) {
        readFieldString("Train number (letters/digits, up to 9): ", t.trainNumber, sizeof t.trainNumber);
        if (!isValidTrainNumber(t.trainNumber)) {
            printf("[ERROR] Use only letters and digits (no spaces or symbols).\n");
        } else if (findTrainIndex(station, t.trainNumber) >= 0) {
            printf("[ERROR] Train %s already exists.\n", t.trainNumber);
        } else {
            break;
        }
    }

    readFieldString("Train name: ", t.trainName, sizeof t.trainName);
    readFieldString("Destination: ", t.destination, sizeof t.destination);
    readTimeString("Scheduled arrival time (HH:MM): ", t.arrivalTime, sizeof t.arrivalTime);

    for (;;) {
        t.platformNumber = readIntInRange("Assigned platform number: ", 1, MAX_PLATFORM_NO);
        if (findPlatformIndex(station, t.platformNumber) < 0) {
            printf("[ERROR] Platform %d does not exist. Add it first or choose another.\n",
                   t.platformNumber);
        } else {
            break;
        }
    }

    if (hasPlatformClash(station, t.platformNumber, t.arrivalTime, -1)) {
        printf("[ERROR] Another train already arrives on platform %d at %s.\n",
               t.platformNumber, t.arrivalTime);
        return;
    }

    t.delayMinutes = readIntInRange("Delay in minutes (0 = on time): ", 0, MAX_DELAY);

    station->trains[station->trainCount++] = t;
    syncPlatformTrainInfo(station);

    printf("\n[OK] Train %s added to platform %d.\n", t.trainNumber, t.platformNumber);
    saveWithMessage(station);
}

static void removeTrain(Station *station)
{
    int order[MAX_TRAINS];

    printScreenHeader("REMOVE TRAIN");

    if (station->trainCount == 0) {
        printf("\n[INFO] There are no trains to remove.\n");
        return;
    }

    sortTrainIndices(station, order, SORT_TRAIN_TIME);
    printf("\n");
    printTrainTable(station, order, station->trainCount);
    printf("\n");

    int index = selectTrainIndex(station);
    if (index < 0) {
        return;
    }

    char question[INPUT_LEN];
    snprintf(question, sizeof question, "Really remove train %s?", station->trains[index].trainNumber);
    if (!confirmAction(question)) {
        printf("[INFO] Removal cancelled.\n");
        return;
    }

    printf("[OK] Train %s removed.\n", station->trains[index].trainNumber);
    for (int i = index; i < station->trainCount - 1; i++) {
        station->trains[i] = station->trains[i + 1];
    }
    station->trainCount--;

    syncPlatformTrainInfo(station);
    saveWithMessage(station);
}

static void reassignTrain(Station *station)
{
    int order[MAX_TRAINS];

    printScreenHeader("REASSIGN TRAIN TO ANOTHER PLATFORM");

    if (station->trainCount == 0) {
        printf("\n[INFO] There are no trains.\n");
        return;
    }

    sortTrainIndices(station, order, SORT_TRAIN_TIME);
    printf("\n");
    printTrainTable(station, order, station->trainCount);
    printf("\n");

    int index = selectTrainIndex(station);
    if (index < 0) {
        return;
    }

    Train *t = &station->trains[index];
    int number = readIntInRange("New platform number: ", 1, MAX_PLATFORM_NO);

    if (findPlatformIndex(station, number) < 0) {
        printf("[ERROR] Platform %d does not exist.\n", number);
        return;
    }
    if (number == t->platformNumber) {
        printf("[INFO] Train %s is already on platform %d.\n", t->trainNumber, number);
        return;
    }
    if (hasPlatformClash(station, number, t->arrivalTime, index)) {
        printf("[ERROR] Another train already arrives on platform %d at %s.\n",
               number, t->arrivalTime);
        return;
    }

    t->platformNumber = number;
    syncPlatformTrainInfo(station);
    printf("\n[OK] Train %s moved to platform %d.\n", t->trainNumber, number);
    saveWithMessage(station);
}

static void setTrainDelay(Station *station)
{
    int order[MAX_TRAINS];

    printScreenHeader("SET TRAIN DELAY");

    if (station->trainCount == 0) {
        printf("\n[INFO] There are no trains.\n");
        return;
    }

    sortTrainIndices(station, order, SORT_TRAIN_TIME);
    printf("\n");
    printTrainTable(station, order, station->trainCount);
    printf("\n");

    int index = selectTrainIndex(station);
    if (index < 0) {
        return;
    }

    Train *t = &station->trains[index];
    t->delayMinutes = readIntInRange("Delay in minutes (0 = on time): ", 0, MAX_DELAY);
    syncPlatformTrainInfo(station);

    char expected[TIME_LEN];
    formatExpectedTime(t->arrivalTime, t->delayMinutes, expected, sizeof expected);
    printf("\n[OK] Train %s: scheduled %s, expected %s.\n", t->trainNumber,
           t->arrivalTime, expected);
    saveWithMessage(station);
}

/* ===================================================================== */
/* Manage menu                                                           */
/* ===================================================================== */

void manageMenu(Station *station)
{
    int choice;

    do {
        clearScreen();
        printScreenHeader("MANAGE PLATFORMS AND TRAINS");
        printf("\n   1. Add a platform\n");
        printf("   2. Remove a platform\n");
        printf("   3. Change a platform's capacity\n");
        printf("   4. Add a train\n");
        printf("   5. Remove a train\n");
        printf("   6. Reassign a train to another platform\n");
        printf("   7. Set a train's delay\n");
        printf("   0. Back to main menu\n\n");

        choice = readIntInRange("Enter your choice (0-7): ", 0, 7);
        clearScreen();

        switch (choice) {
            case 1: addPlatform(station);    break;
            case 2: removePlatform(station); break;
            case 3: changeCapacity(station); break;
            case 4: addTrain(station);       break;
            case 5: removeTrain(station);    break;
            case 6: reassignTrain(station);  break;
            case 7: setTrainDelay(station);  break;
            default: break;
        }

        if (choice != 0) {
            pauseForEnter();
        }
    } while (choice != 0);
}
