# 🚉 Railway Platform Crowd Management System

A menu-driven, terminal-based **C project** that **simulates** passenger crowd levels on railway platforms, flags overcrowded platforms, shows train information and delays, forecasts crowd trends, and recommends less crowded platforms.

> ⚠️ **Simulation notice:** This project uses **sample (fictional) data only**. It is **not** connected to any real-time railway system, and its recommendations and forecasts must **never** replace official railway announcements or staff instructions.

![Language](https://img.shields.io/badge/language-C11-blue)
![Compiler](https://img.shields.io/badge/compiler-GCC-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-yellow)

---

## 📖 Description

Railway stations get crowded, and overcrowded platforms are a safety concern. This project models a small station with several platforms. For each platform it tracks how many passengers are present, compares that with the platform's capacity, classifies the crowd level, keeps a history of past levels, estimates where the crowd is heading, and suggests where passengers could go instead.

It was built as a first-year B.Tech CSE (Artificial Intelligence) college project and is written to be readable, well-commented and modular.

## 🎯 Project Objectives

1. Apply core C concepts (structures, arrays, pointers, file handling, functions, multiple source files) in a realistic project.
2. Simulate and classify platform crowd levels using clear, testable rules.
3. Implement searching, sorting, statistics and a simple trend forecast in C.
4. Persist data in text files and handle missing or corrupted files gracefully.
5. Build a clean, user-friendly terminal interface with full input validation.
6. Practise good project structure, documentation and GitHub workflow.

## ✨ Features

| Area | Feature |
|---|---|
| **Dashboard** | Date/time, totals, average occupancy, overcrowded count, platform-wise table with crowd bars, and alerts |
| **Crowd detection** | `LOW` < 30% · `MODERATE` 30–<60% · `HIGH` 60–<90% · `OVERCROWDED` ≥ 90% (exact boundaries, integer maths) |
| **Platforms** | View sorted by number or by crowd; update passenger counts; instant overcrowding warning with an alternative |
| **Trains** | List sorted by time, number or destination; search by train number, destination or platform; **delay tracking** with expected arrival time |
| **Smart recommendation** | Least crowded platform · full ranking · alternatives for a crowded platform · **advice for a specific train** |
| **Analytics** | Platform-wise report, most/least crowded, averages, totals, overcrowding statistics |
| **History log** | Every update and simulation is saved with a timestamp; view the last records per platform |
| **Crowd forecast** | Simple trend-based forecast of each platform's next crowd level, with **early warnings** |
| **Management** | Add/remove platforms, change capacity, add/remove trains, reassign trains, set delays - all validated (no duplicate numbers, no two trains on one platform at the same time) |
| **Report export** | Save a plain-text crowd report to `data/crowd_report.txt` |
| **Simulation** | Randomly simulates passengers arriving and leaving |
| **Colours** | Optional ANSI colours for crowd levels (auto-detected, `NO_COLOR` respected, can be toggled in Settings) |
| **Persistence** | Auto-load on start, auto-save on every change; falls back to sample data if files are missing or corrupted |
| **Settings** | Toggle colours, reset to sample data, clear history |

## 🛠️ Technologies Used

- **Language:** C (C11 standard)
- **Compiler:** GCC
- **Build tool:** Make
- **Storage:** Plain text files (pipe-delimited)
- **Version control:** Git & GitHub

## 💻 System Requirements

- Windows, Linux or macOS
- GCC (MinGW-w64 on Windows, `build-essential` on Linux, Xcode Command Line Tools on macOS)
- `make` (optional - you can compile with a single `gcc` command)
- A terminal at least **74 characters wide**

## 📥 Installation

```bash
# 1. Clone the repository (replace the URL with your own)
git clone https://github.com/<your-username>/railway-platform-crowd-management.git

# 2. Enter the project folder
cd railway-platform-crowd-management
```

**Installing GCC**

| OS | Command |
|---|---|
| Ubuntu / Debian | `sudo apt install build-essential` |
| macOS | `xcode-select --install` |
| Windows | Install [MSYS2](https://www.msys2.org/) or MinGW-w64, then add its `bin` folder to `PATH` |

## ▶️ Compilation and Execution

**Using Make (recommended):**

```bash
make          # compile
make run      # compile and run
make clean    # remove the compiled program
```

**Without Make:**

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude src/main.c src/utils.c src/crowd.c src/storage.c src/screens.c src/manage.c -o railway_crowd
./railway_crowd          # Linux / macOS
railway_crowd.exe        # Windows (Command Prompt)
```

> **Important:** Run the program from the **project root folder** (the one that contains `data/`). Otherwise it cannot find the data files and will work in memory only.

The project compiles with **no warnings** under `-Wall -Wextra -Wpedantic` (and also `-Wshadow -Wconversion -Wformat=2`).

## 📘 Usage Guide

When the program starts it loads saved data and shows the main menu:

| Option | What it does |
|---|---|
| `1` Dashboard | Overview of the whole station with alerts |
| `2` Platform details | Next train, delay, load and status; sort by number or crowd |
| `3` Update passenger count | Choose a platform and enter a new count (0 to capacity) |
| `4` Train information | All trains sorted by time, number or destination |
| `5` Search trains | By train number, destination (partial) or platform |
| `6` Smart platform recommendation | Least crowded, ranking, alternatives, or advice for a train |
| `7` Crowd analytics report | Detailed statistics and overcrowding report |
| `8` Crowd history and forecast | View saved history; forecast next crowd levels |
| `9` Manage platforms and trains | Add/remove platforms and trains, capacity, reassign, delays |
| `10` Simulate crowd change | Random arrivals/departures (±25% of capacity) |
| `11` Export report to file | Writes `data/crowd_report.txt` |
| `12` Settings | Colours on/off, reset to sample data, clear history |
| `0` Save and exit | Saves everything and quits |

**Try this demo:**

1. Option `3`: set platform `4` to `900` (90% of 1000) and watch the overcrowding warning and alternative appear.
2. Option `8` → `2`: see the forecast and early warnings.
3. Option `9` → `4`: add a train, then `9` → `7` to give it a delay.
4. Option `6` → `4`: ask for advice for a delayed train.
5. Option `12` → `2`: reset everything to the sample data.

## 🖥️ Sample Terminal Output

**Dashboard (option 1):**

```text
========================================================================
                RAILWAY PLATFORM CROWD MANAGEMENT SYSTEM
                    (Simulation - sample data only)
------------------------------------------------------------------------
                               DASHBOARD
========================================================================

  Date and time         : 2026-11-02 06:22:53
  Total platforms       : 5
  Total trains          : 5
  Total passengers      : 2450 (capacity 5000)
  Average occupancy     : 49.0%
  Overcrowded platforms : 1

+------+-----------+--------+----------------------------+-------------+
| Plat | Pass/Cap  | Occ %  | Crowd Bar                  | Status      |
+------+-----------+--------+----------------------------+-------------+
| 1    |  100/1000 |  10.0% | ###....................... | LOW         |
| 2    |  250/1000 |  25.0% | #######................... | LOW         |
| 3    |  450/1000 |  45.0% | ############.............. | MODERATE    |
| 4    |  700/1000 |  70.0% | ##################........ | HIGH        |
| 5    |  950/1000 |  95.0% | #########################. | OVERCROWDED |
+------+-----------+--------+----------------------------+-------------+

  ALERTS
  !!! ALERT: Platform 5 is OVERCROWDED (95.0% - 950 of 1000 passengers)
      -> Suggested alternative: Platform 1 (10.0%, LOW)
```

**Platform ranking (option 6 → 2):**

```text
+------+------+--------+-------------+---------------------------------+
| Rank | Plat | Occ %  | Status      | Next Train                      |
+------+------+--------+-------------+---------------------------------+
| 1    | 1    |  10.0% | LOW         | 12001 - Delhi                   |
| 2    | 2    |  25.0% | LOW         | 12002 - Chandigarh              |
| 3    | 3    |  45.0% | MODERATE    | 12003 - Jammu                   |
| 4    | 4    |  70.0% | HIGH        | 12004 - Amritsar                |
| 5    | 5    |  95.0% | OVERCROWDED | 12005 - Srinagar                |
+------+------+--------+-------------+---------------------------------+
  (Rank 1 = least crowded)

  NOTE: These recommendations are SIMULATED from sample data.
  They must NOT replace official railway announcements or
  instructions from railway staff.
```

**Advice for a train (option 6 → 4, train `12005`):**

```text
  Train 12005 (Valley Express) to Srinagar
  Scheduled 11:40, expected 12:05 (delayed by 25 min)
  Arrives on Platform 5, currently OVERCROWDED (95.0%).
  The platform is crowded - wait where there is more room, and
  move to Platform 1 (10.0%) only if staff announce it.

  NOTE: These recommendations are SIMULATED from sample data.
  They must NOT replace official railway announcements or
  instructions from railway staff.
```

**Crowd history (option 8 → 1, platform `5`):**

```text
  Last 4 record(s), oldest first:

+---------------------+------+-----------+--------+--------------------+
| Time                | Plat | Pass/Cap  | Occ %  | Status             |
+---------------------+------+-----------+--------+--------------------+
| 2026-11-01 09:00:00 | 5    |  700/1000 |  70.0% | HIGH               |
| 2026-11-01 09:15:00 | 5    |  790/1000 |  79.0% | HIGH               |
| 2026-11-01 09:30:00 | 5    |  880/1000 |  88.0% | HIGH               |
| 2026-11-01 09:45:00 | 5    |  950/1000 |  95.0% | OVERCROWDED        |
+---------------------+------+-----------+--------+--------------------+
```

**Crowd forecast (option 8 → 2):**

```text
+------+---------+------------+-----------+---------+------------------+
| Plat | Now %   | Avg change | Predicted | Pred %  | Forecast level   |
+------+---------+------------+-----------+---------+------------------+
| 1    |   10.0% | +20.0      | 120       |   12.0% | LOW              |
| 2    |   25.0% | +43.3      | 293       |   29.3% | LOW              |
| 3    |   45.0% | +50.0      | 500       |   50.0% | MODERATE         |
| 4    |   70.0% | +60.0      | 760       |   76.0% | HIGH             |
| 5    |   95.0% | +83.3      | 1000      |  100.0% | OVERCROWDED      |
+------+---------+------------+-----------+---------+------------------+

  NOTE: A forecast is a simple straight-line estimate from SIMULATED
  data. It is not a real prediction and must NOT replace official
  railway announcements or instructions from railway staff.
```

**Manage menu (option 9):**

```text
                      MANAGE PLATFORMS AND TRAINS
========================================================================

   1. Add a platform
   2. Remove a platform
   3. Change a platform's capacity
   4. Add a train
   5. Remove a train
   6. Reassign a train to another platform
   7. Set a train's delay
   0. Back to main menu
```

## 🏗️ Project Architecture

```text
railway-platform-crowd-management/
├── src/
│   ├── main.c        # start-up, main menu, settings
│   ├── utils.c       # input, validation, colours, drawing helpers
│   ├── crowd.c       # crowd levels, statistics, sorting, forecast (no printing)
│   ├── storage.c     # sample data, load/save, history log, report export
│   ├── screens.c     # every screen that displays information
│   └── manage.c      # every feature that changes data
├── include/
│   └── railway.h     # constants, structures, all function prototypes
├── data/
│   ├── platforms.txt # platformNumber|passengerCount|maxCapacity
│   ├── trains.txt    # trainNumber|name|destination|HH:MM|platform|delayMinutes
│   └── history.txt   # timestamp|platformNumber|passengerCount|capacity
├── docs/
│   ├── project_report.md
│   ├── github_setup.md
│   └── screenshots/
├── tests/
│   └── test_cases.md
├── .gitignore
├── LICENSE
├── Makefile
└── README.md
```

**Separation of concerns:** `crowd.c` contains the logic and never prints; `screens.c` only displays; `manage.c` changes data and saves it; `storage.c` is the only file that reads or writes the data files.

**Data flow**

```text
 data/*.txt ──load──▶ Station (in memory) ──▶ dashboard / search / analytics / forecast
      ▲                    │  ▲
      └─────save───────────┘  └── user input (validated)
 data/history.txt ◀── appended on every update / simulation
```

**Core data structures** (defined in `railway.h`)

- `Platform` - number, passenger count, capacity, plus the next train's number, destination, time and delay
- `Train` - number, name, destination, scheduled time, assigned platform, delay
- `Station` - arrays of platforms and trains with their counts; **passed by pointer** so no global variables are needed
- `CrowdStats`, `HistoryRecord`, `Forecast` - results used by the dashboard, history and forecast screens

> A platform's train details are **derived** from `trains.txt` (the earliest-arriving train assigned to that platform), so the same information is never stored twice and can never get out of sync.

**Forecast method:** the average change between the last (up to 5) history records of a platform is added to its current passenger count (a straight-line trend), clamped to `0..capacity`. At least two records are needed. The sample `history.txt` has four snapshots so the forecast works immediately.

## 📚 C Concepts Demonstrated

- **Structures**, `typedef`, **enumerations** (`CrowdLevel`, `LoadStatus`, sort modes) and designated initialisers
- **Arrays** of structures and **pointers** (`const` pointers, pointers to structures, pass-by-pointer)
- **Multiple source files**, a shared header, `static` helper functions, header guards
- **File handling** - `fopen` (read/write/append), `fgets`, `sscanf`, `fprintf`, `fclose` with error checks; one routine writes to the screen **or** a file (`FILE *`)
- **String handling** - `snprintf`, `strcmp`, `strtol`, `memmove`; safe input without `scanf`/`gets`
- **Algorithms** - linear search, insertion sort (on index arrays), min/max, averages, linear trend forecast
- **Input validation** - range checks, over-long input, forbidden characters, duplicates, time format, clash detection
- **Preprocessor** - `#define` constants, `#ifdef _WIN32` for cross-platform code
- **Date and time** - `time`, `localtime`, `strftime`
- **Safe coding** - bounded copies, no memory leaks (fixed arrays only), no unsafe functions
- **Build automation** with a Makefile

## 🚧 Limitations and Future Enhancements

**Current limitations**

- Uses simulated sample data; no real-time railway integration
- Fixed limits: 10 platforms and 20 trains
- The forecast is a simple straight-line trend, not a trained machine-learning model
- The history file grows with every update (use Settings → Clear history)
- Terminal interface only

**Possible enhancements**

- Dynamic memory (`malloc`) to remove the fixed limits
- A real prediction model (for example linear regression or a small neural network) trained on the history, which links well to the AI specialisation
- Time-of-day aware forecasts (peak hours)
- Sound alerts, and a GUI or web front-end
- Unit tests with a C test framework

## 📸 Screenshots

Add your own screenshots to `docs/screenshots/` after running the program, then reference them here:

```markdown
![Dashboard](docs/screenshots/dashboard.png)
![Forecast](docs/screenshots/forecast.png)
```

## 👤 Author

**Akshat Singh Sambyal**
UID: 26BAI1009
B.Tech CSE (Artificial Intelligence), [Your College Name]
GitHub: [@your-username](https://github.com/your-username)

## 📄 License

This project is released under the [MIT License](LICENSE).
