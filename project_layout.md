# Daily Activities Tracker (Qt Desktop App) — Product Draft v1.2 (Windows MVP)

## 1) Purpose
A **Windows-first** desktop application (built with **Qt / Qt Creator Community Edition**) to help track day-to-day activities and longer-cycle routines (weekly/monthly/yearly). The app motivates consistency using:

- a **calendar + task list** workflow
- **binary completion tracking** (done / not done / skipped)
- a **scoring system** (supports **positive and negative points**)
- **streaks** (negative/bad outcomes can break streaks)
- dashboards and graphs showing **improvement or decline** over time

This is not intended to be a generic tracker; it’s designed to reflect your personal logic and scoring rules.

---

## 2) Core Concepts (Non-ambiguous Definitions)

### 2.1 Goal
A **Goal** is a defined activity you want to track repeatedly.

Each Goal MUST have:
- **Title** (e.g., “Brush teeth”, “Vacuum room”, “Cook breakfast”)
- **Scope**: `Daily` | `Weekly` | `Monthly` | `Yearly`
- **Points**: integer (can be **positive or negative**)
- **Penalty rule** for missing (configurable per goal; see 2.4)
- **Optional metadata**: category, notes, icon, reminder (future), sort order

> Note: “Bad things” can be modeled as goals with **negative points**.

---

### 2.2 Occurrence
An **Occurrence** is the instance of a Goal inside its time window.

Time windows:
- Daily goal → a specific **date**
- Weekly goal → a specific **week**
- Monthly goal → a specific **month**
- Yearly goal → a specific **year**

Each Occurrence has exactly one **Status** (see 2.3).

---

### 2.3 Status (Three-state tracking)
Each Occurrence can be:

1) **Completed**
- The goal was done within its time window.
- Score impact: adds the goal’s `Points` (which may be positive or negative).

2) **Skipped**
- Meaning: *“I could have done it, but I didn’t.”*
- Default score impact: **0**
- Important: Skip is not the same as Not Completed.

3) **Not Completed**
- Meaning: *“I should have done it, but I didn’t.”*
- Score impact depends on the goal’s penalty settings (see 2.4).

4) **Pending**
- Meaning: *“There is time to do it”*
- Scoring on hold

---

### 2.4 Penalty Settings (Per-goal flexibility)
Every Goal has a **Missing Behavior** configuration that decides what happens when it is **Not Completed**.

Supported missing behaviors:

- **Zero (default)**  
  Not Completed → score impact is **0**

- **Penalty**  
  Not Completed → score impact is **-PenaltyPoints**  
  (PenaltyPoints should be stored explicitly; it is NOT assumed to equal Points.)

Skipped behavior:
- Skipped → score impact is **0 by default**
- (Optional future extension: allow skip penalty too, but NOT required for MVP unless you want it now.)

---

### 2.5 Streaks
A **streak** measures consistent performance (exact formula can be simple in MVP):

- A streak is associated with a scope (Daily/Weekly/Monthly/Yearly) and can be tracked per-goal and/or as an overall streak per scope.
- A streak is **broken** when either:
  - a **negative-point outcome** is recorded (e.g., a “bad habit” completed with negative points), OR
  - a goal configured as “Penalty on Not Completed” becomes Not Completed within its time window (if you want missed penalties to break streaks too)

> MVP Recommendation:  
> Start with **Scope-level streaks** (Daily streak, Weekly streak, etc.) based on the **scope completion %** meeting a minimum threshold (e.g., ≥ 80%).  
> Then add per-goal streaks later if you want.

---

## 3) Scoring Model

### 3.1 Score Calculation (No overlap rule)
Scores never overlap across scopes.

- **Daily Score**: only daily goals for that date
- **Weekly Score**: only weekly goals for that week
- **Monthly Score**: only monthly goals for that month
- **Yearly Score**: only yearly goals for that year

This is strict: completing a weekly goal does **not** affect a day cell’s score color or daily graph.

---

### 3.2 Target Score and Completion %
To support fair comparisons and heatmaps, each window should calculate:

- **Earned Score** = sum of score impacts from all occurrences in that window
- **Target Score** = total *possible* positive points for goals scheduled in that window  
  (MVP: include only goals with positive points in Target Score to avoid weird “target negative” math.)

- **Completion %** = EarnedPositive / TargetScore  
  (MVP: define clearly as: sum of earned points from positive-point goals divided by target.)

> Important: negative-point goals will reduce Earned Score, and can also break streaks, but they should NOT distort the Target Score baseline.

---

## 4) Scheduling Rules (MVP)

### 4.1 Daily Goals
- Occurs every day by default.
- (Optional but easy later: choose weekdays.)

### 4.2 Weekly Goals
- Can be completed **any day during the week**.
- Week definition must be global (pick one for MVP):
  - Recommended: ISO week (Monday start)

### 4.3 Monthly Goals
- Can be completed **any day during the month**.

### 4.4 Yearly Goals
- Can be completed **any day during the year**.

> Day-specific scheduling is explicitly out of scope for MVP; can be added later.

---

## 5) Main UI Layout (Two-portion screen)

### 5.1 Main Window
Split into two primary areas:

#### Left: Calendar Panel
- View toggle: **Weekly / Monthly**
- Each cell shows:
  - date number
  - score indicator (score and/or %)
  - background heat color (red ↔ green gradient) based on completion %

Calendar meaning by view:
- Monthly/weekly calendar cells show **Daily Scores only** (because calendar days represent days).
- Weekly/Monthly/Yearly scores are shown in their own scope-specific screens/panels (see 5.2 and 6).

#### Right: Tasks Panel
- Scope tabs (explicit):
  - **Daily**
  - **Weekly**
  - **Monthly**
  - **Yearly**

Behavior:
- User selects a date (calendar click).
- Task panel updates depending on selected scope:
  - Daily tab → daily occurrences for that date
  - Weekly tab → weekly occurrences for the week containing that date
  - Monthly tab → monthly occurrences for the month containing that date
  - Yearly tab → yearly occurrences for the year containing that date

Each tab displays:
- Summary: **Earned Score / Target Score / Completion %**
- List of occurrences with status controls

---

### 5.2 Task Row Design (per occurrence)
Each row includes:
- status control (Completed / Skipped / Not Completed)
- title
- points (and penalty indicator if configured)
- optional: category icon
- optional: notes indicator (future)

Status interaction rules:
- Selecting **Completed** applies Points (positive or negative)
- Selecting **Skipped** applies 0
- Selecting **Not Completed** applies either 0 or a penalty depending on the goal config

---

## 6) Additional Screens

### 6.1 Dashboard Screen
Purpose: quick understanding of performance.

Include:
- KPI cards:
  - Today (Daily)
  - This Week
  - This Month
  - This Year
- Trend charts:
  - daily completion % trend (last 30 days)
  - weekly score trend (last 12 weeks)
  - monthly score trend (last 12 months)

Charts show improvement or decrement over time.

---

### 6.2 Comparison Screen
Purpose: compare performance between periods.

Compare options (MVP):
- this week vs last week
- this month vs last month
- this year vs last year

Show:
- earned score
- completion %
- biggest improvements / declines (by category and/or goal)

---

## 7) Heatmap / Color Cues

### 7.1 Daily Calendar Cells
Calendar day cells represent **Daily scope only**.

Color is based on daily completion %:
- 0% → red
- 100% → green
- gradient in between

Displayed metric in the cell (choose one for MVP):
- `Completion %` (recommended, easiest to interpret), OR
- `Earned Score`, OR
- compact: `Score • %`

---

### 7.2 Weekly/Monthly/Yearly “Cells”
Since you want weekly/monthly/yearly calendars too:

- Weekly view: a “week bar” or week grid that displays **Weekly scope performance**
- Monthly view: a month grid that displays **Monthly scope performance**
- Yearly view: a year grid (12 months) that displays **Yearly scope performance**

These are separate from the day calendar heatmap and follow the **no overlap** rule.

> MVP suggestion:
> Keep the main calendar as day-based.
> Add scope summary widgets (week/month/year) rather than building 4 different calendar engines immediately.

---

## 8) Data Storage & Sync Strategy (Updated)

### 8.1 Goals
- The app MUST work fully offline.
- **Local database is mandatory** and is the source of truth.
- Cloud storage is optional and will be added later using **Supabase** (opt-in).

---

### 8.2 Local Database (Mandatory): PostgreSQL on Windows
All data is stored locally in **PostgreSQL**.

#### Local PostgreSQL Provisioning (Two options)
**Option A — User-installed PostgreSQL**
- User installs PostgreSQL separately.
- App connects to `127.0.0.1` with configured credentials.
- App checks version + schema on startup and runs migrations if needed.

**Option B — Bundled “Portable PostgreSQL” (Recommended)**
- The installer ships a known PostgreSQL version with the app.
- On first run, the app:
  1) creates a Postgres data directory under the app data path  
  2) initializes the database cluster  
  3) starts PostgreSQL bound to localhost only  
  4) creates the application database + dedicated DB user  
  5) runs schema migrations
- On each app start: ensure local PostgreSQL is running (start if needed).
- On app exit: stop PostgreSQL gracefully (or keep running by a clear policy).

> MVP Implementation note:  
> If you want a smooth “non-technical user” experience, Option B is strongly preferred.

---

### 8.3 Local Security Rules
- PostgreSQL MUST bind ONLY to `127.0.0.1` (no LAN/public binds).
- Use a dedicated DB user for the app (least privilege).
- Store DB credentials locally with restricted file permissions (and/or OS encryption).
- No external network access to the local DB.

---

### 8.4 Cloud Database (Future Optional): Supabase
Supabase will be added later for:
- cloud backup/restore
- multi-device sync
- possible web dashboard expansion (future)

Cloud is OFF by default and requires user opt-in + authentication (Supabase Auth).

---

### 8.5 Schema Compatibility Requirement
To ensure easy migration/sync to Supabase Postgres later:
- Use PostgreSQL-native types that Supabase supports.
- Prefer **UUID primary keys** (cloud-friendly).
- Include `created_at` and `updated_at` timestamps for sync support.
- Include `deleted_at` for soft delete / tombstones (recommended for sync).

---

### 8.6 Sync Model (Future)
When cloud sync is enabled:
- Local PostgreSQL remains the source of truth.
- A sync engine pushes/pulls changes between local and Supabase.

Minimum sync metadata per record:
- `id` (UUID)
- `updated_at` (timestamp)
- `deleted_at` (nullable timestamp; tombstone for deletes)

Conflict strategy (initial):
- **Last-write-wins** based on `updated_at`.

---

### 8.7 Local Backup & Restore
The app MUST provide:
- export backup (SQL dump or app-managed backup format)
- restore from backup file

---

## 9) MVP Feature Checklist (First Shippable Version)

### Must-have
- Goal CRUD (create/edit/delete)
- Occurrence tracking with 3 states (Completed/Skipped/Not Completed)
- Per-goal missing behavior: zero or penalty
- Support negative-point goals
- Scoring for daily/weekly/monthly/yearly (no overlap)
- Main split UI: calendar + tasks
- Daily heatmap coloring
- Dashboard with trends
- Local PostgreSQL storage (mandatory)
- Export/Restore backups

### Later
- reminders
- day-specific weekly tasks
- measurable goals (minutes/hours)
- advanced streak configurations
- Supabase cloud sync and multi-device support
- cross-platform packaging

---

## 10) Market Apps Closest to This Idea (References)
Closest matches by concept (habit/task tracking + scoring + charts):

- **Way of Life** (strong color-coded tracking / visual cues)
- **Strides** (goals + analytics)
- **HabitBull** (habit tracking + calendar views)
- **Loop Habit Tracker** (simple tracking + stats)
- **Habitica** (gamified scoring approach)

Your app becomes distinct because it combines:
- multi-scope calendars (daily/weekly/monthly/yearly)
- explicit “Skipped vs Not Completed” semantics
- configurable penalties + negative-point “bad tasks”
- no score overlap by scope
- local-first desktop design with optional cloud sync

---

## 11) Final Decisions Locked In
- Points can be **negative**
- Negative/bad outcomes can **break streaks**
- Tracking is **binary** (no measurable metrics in MVP)
- Weekly goals: complete **any day** during the week (no day-specific rules yet)
- Heatmap and scoring are **scope-isolated** (no overlap)
- Target platform for MVP: **Windows**
- Local database is mandatory: **PostgreSQL**
- Future optional cloud: **Supabase**

---
