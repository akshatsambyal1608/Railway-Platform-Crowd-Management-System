/*
 * main.c
 * ---------------------------------------------------------------------
 * Railway Platform Crowd Management System
 *
 * A menu-driven terminal program that SIMULATES passenger crowd levels
 * on railway platforms. It uses sample data only - there is no
 * connection to real railway systems.
 *
 * Files:
 *   main.c     start-up, main menu, settings
 *   utils.c    input, validation, colours and drawing helpers
 *   crowd.c    crowd levels, statistics, sorting, forecast (no printing)
 *   storage.c  sample data, loading, saving, history log, report export
 *   screens.c  all screens that display information
 *   manage.c   all features that change data
 * ---------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "railway.h"

static void printMenu(void)
{
    printScreenHeader("MAIN MENU");
    printf("\n    1. Dashboard\n");
    printf("    2. Platform details\n");
    printf("    3. Update passenger count\n");
    printf("    4. Train information\n");
    printf("    5. Search trains\n");
    printf("    6. Smart platform recommendation\n");
    printf("    7. Crowd analytics report\n");
    printf("    8. Crowd history and forecast\n");
    printf("    9. Manage platforms and trains\n");
    printf("   10. Simulate crowd change\n");
    printf("   11. Export report to file\n");
    printf("   12. Settings\n");
    printf("    0. Save and exit\n\n");
}

/* Settings: colours, reset to sample data, clear history. */
static void settingsMenu(Station *station)
{
    printScreenHeader("SETTINGS");
    printf("\n  1. Turn colours %s\n", station->colorEnabled ? "OFF" : "ON");
    printf("  2. Reset platforms, trains and history to sample data\n");
    printf("  3. Clear crowd history\n");
    printf("  0. Back to main menu\n\n");

    int choice = readIntInRange("Enter your choice (0-3): ", 0, 3);

    if (choice == 1) {
        station->colorEnabled = !station->colorEnabled;
        printf("[OK] Colours are now %s.\n", station->colorEnabled ? "ON" : "OFF");
    } else if (choice == 2) {
        if (!confirmAction("This replaces ALL current data with sample data. Continue?")) {
            printf("[INFO] Reset cancelled.\n");
            return;
        }
        loadSampleData(station);
        if (saveStationData(station) && writeSampleHistory()) {
            printf("[OK] Sample data restored and saved.\n");
        } else {
            printf("[ERROR] Sample data restored, but could not be saved to file.\n");
        }
    } else if (choice == 3) {
        if (!confirmAction("Delete all saved crowd history?")) {
            printf("[INFO] Cancelled.\n");
            return;
        }
        if (clearHistory()) {
            printf("[OK] History cleared.\n");
        } else {
            printf("[ERROR] Could not clear the history file.\n");
        }
    }
}

int main(void)
{
    Station station;
    int running = 1;

    srand((unsigned int)time(NULL)); /* seed the random simulation */

    station.colorEnabled = detectColorSupport();

    /* Load saved data automatically; fall back to sample data if needed. */
    clearScreen();
    printScreenHeader("STARTING UP");
    printf("\n");

    LoadStatus status = loadStationData(&station);
    if (status == LOAD_OK) {
        printf("[OK] Saved data loaded: %d platforms, %d trains.\n",
               station.platformCount, station.trainCount);
    } else if (status == LOAD_PARTIAL) {
        printf("[WARNING] Data loaded, but some invalid lines were skipped.\n");
        printf("          Loaded %d platforms and %d trains.\n",
               station.platformCount, station.trainCount);
    } else {
        printf("[WARNING] Data files are missing or corrupted.\n");
        printf("          Loading built-in sample data instead.\n");
        loadSampleData(&station);
        if (saveStationData(&station) && writeSampleHistory()) {
            printf("[OK] Sample data files created in the data/ folder.\n");
        } else {
            printf("[ERROR] Could not write data files. Run the program from the\n");
            printf("        project folder (the one containing data/). Working in memory only.\n");
        }
    }
    pauseForEnter();

    while (running) {
        clearScreen();
        printMenu();
        int choice = readIntInRange("Enter your choice (0-12): ", 0, 12);
        clearScreen();

        switch (choice) {
            case 1:  displayDashboard(&station);        break;
            case 2:  displayPlatformDetails(&station);  break;
            case 3:  updatePassengerCount(&station);    break;
            case 4:  displayTrains(&station);           break;
            case 5:  searchTrains(&station);            break;
            case 6:  recommendPlatforms(&station);      break;
            case 7:  displayAnalyticsReport(&station);  break;
            case 8:  historyMenu(&station);             break;
            case 9:  manageMenu(&station);              break;
            case 10: simulateCrowdChange(&station);     break;
            case 11:
                printScreenHeader("EXPORT REPORT");
                if (exportReport(&station)) {
                    printf("\n[OK] Report saved to %s\n", REPORT_FILE);
                } else {
                    printf("\n[ERROR] Could not write %s. Run from the project folder.\n",
                           REPORT_FILE);
                }
                break;
            case 12: settingsMenu(&station);            break;
            case 0:
                if (saveStationData(&station)) {
                    printf("[OK] Data saved. Thank you for using the system. Goodbye!\n");
                } else {
                    printf("[ERROR] Could not save data. Goodbye!\n");
                }
                running = 0;
                break;
            default:
                break;
        }

        /* manageMenu() pauses by itself after each of its actions. */
        if (running && choice != 9) {
            pauseForEnter();
        }
    }

    return EXIT_SUCCESS;
}
