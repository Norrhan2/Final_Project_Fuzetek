# Ahmed's Task: Admin Dashboard & Stats + QA

> README line: **Ahmed | Admin Dashboard & Stats + QA | Sales statistics logic, admin dashboard screen, tests, Git/Jira upkeep**

This doc explains the task in simple words, based on what is really inside the project today (branch `Ahmed_yasser`).
No code here. Only explanation and plan.

---

## 1. Short version (cave-man)

You have 4 jobs:

1. **Stats brain** - one class that asks database "how many tickets sold? how much money?" and gives numbers back.
2. **Dashboard screen** - one window that shows those numbers to Admin (and Organizer for own events).
3. **Tests** - check that everybody's code works. Find bugs. Tell team.
4. **Git/Jira** - keep branches, pull requests, and tickets tidy so team does not make mess.

Your work sits **on top of** everyone else's work. You read their data. You do not change their tables or classes. So you need to understand what they built. That is why this doc maps it.

---

## 2. Your 4 deliverables

| # | Deliverable | Where file goes (from README structure) | State today |
|---|---|---|---|
| 1 | Sales statistics logic | new stats class (suggest `src/core/SalesStatistics.cpp` + repo in `src/database/StatsRepository.cpp`) | does not exist |
| 2 | Admin dashboard screen | `src/gui/AdminDashboard.cpp` | file exists, **empty (0 bytes)** |
| 3 | Tests | `tests/` folder (README lists it) | folder does not exist yet |
| 4 | Git/Jira upkeep | GitHub repo + Jira board | ongoing; no files |

Project also lists `docs/` with structure.md and diagrams - not made yet. Likely QA/doc owner (you) is a good person to nudge on this.

---

## 3. What the project has right now (map of teammates' work)

I read every file. Many are done, some are empty.

| Area | Owner | File | Status |
|---|---|---|---|
| User / Admin / Organizer / Customer classes | Norhan | `core/User.cpp` | **empty** |
| Database connection | Norhan | `database/DatabaseManager.cpp` | **empty** (but every repo calls it) |
| User repo | Norhan | `database/UserRepository.cpp` | **empty** |
| Login screen | Norhan | `gui/LoginWindow.cpp` | **empty** |
| Main shell / navigation | Norhan | `gui/MainWindow.cpp`, `main.cpp` | **empty** |
| Event + Venue | Donia | `core/Event.cpp`, `Venue.cpp`, repos, 3 GUI forms | done |
| Tickets, factory, states | Mahmoud | `core/Ticket.cpp`, `TicketState.cpp`, `TicketRepository.cpp`, `TicketTypeWindow.cpp` | done |
| Booking + cancel | Zeinab | `core/Booking.cpp`, `BookingRepository.cpp`, `BookingWindow.cpp` | done |
| Payment (Strategy, wallet, refund) | Mostafa | `core/Payment.cpp`, `PaymentRepository.cpp`, `PaymentWindow.cpp` | done (biggest module) |
| Attendee / check-in | Youssef | `core/Attendee.cpp`, `AttendeeRepository.cpp`, `CheckInWindow.cpp` | class done; repo + window **empty** |
| Database SQL | shared | `database/schema.sql` | **only Attendees table** is there |
| Seed data | shared | `database/seed_data.sql` | **empty** |

### Key facts that matter for stats

**Tables your numbers come from** (from README + code):

- `Events` - id, organizer_id, venue_id, title, date, capacity
- `Venues` - id, name, address, capacity
- `Tickets` - id, event_id, type (Regular/VIP/Student), price, state (Available / Reserved / Sold / Cancelled)
- `Bookings` - id, customer_id, event_id, ticket_id, status (Pending / Confirmed / Cancelled)
- `Payments` - id, booking_id, method (Cash/Card/Wallet), amount, status (Pending / Completed / Failed / Refunded), paid_at
- `Attendees` - id, booking_id, customer_id, checked_in_at
- `Wallets` - user_id, balance (used by Mostafa's code, but **not in README or schema.sql** yet)

**How one ticket sale flows** (so you know which row means "real sale"):

```
Ticket row Available
   -> customer books  -> Booking = Pending,  Ticket = Reserved
   -> customer pays   -> Payment = Completed, Booking = Confirmed, Ticket = Sold
   -> customer checks in -> Attendee row created
Cancel path: Booking = Cancelled, Ticket goes back to Available, Payment may become Refunded
```

Important: **one Ticket row = one seat**. A booking points to exactly one `ticket_id`. So "tickets sold" = count of Ticket rows in state Sold.

**Money rule used by Mostafa:** money is counted in **cents** (whole numbers) via a `Money` helper, to avoid decimal bugs. Database stores `amount` as decimal. Your stats should follow same idea: sum in database, show with 2 decimals, never add up floating numbers in C++ loops.

**Code style the team uses** (copy it so your code fits in):

- Repos: get connection from `DatabaseManager::getInstance().getConnection()`, open `pqxx::work` (or `nontransaction` for read-only), run query with parameters, catch errors, print to `cerr`, return empty/false on failure.
- GUI: a Qt `QWidget` with a `setupUI()` function, a repo as member, tables with `QTableWidget`.
- Build: README says **only `main.cpp` is compiled**, and `.cpp` files include each other (e.g. `Ticket.cpp` includes `TicketState.cpp`). Header-style `#pragma once` is at top of most files.

---

## 4. Job 1 - Sales statistics logic

### What to measure

Start simple. These are the numbers an Admin would want:

| Stat | Plain meaning | Data from | Rule |
|---|---|---|---|
| Total revenue | All money earned | Payments | sum `amount` where status = Completed. **Do not** count Refunded, Failed, Pending |
| Total refunds | Money given back | Payments | sum where status = Refunded |
| Net revenue | Revenue minus refunds, if you decide Refunded rows are separate | Payments | decide with team (see Section 9) |
| Tickets sold | Number of seats sold | Tickets | count state = Sold |
| Tickets reserved (unpaid) | Held but not paid | Tickets | count state = Reserved |
| Tickets available | Still for sale | Tickets | count state = Available |
| Bookings by status | Pending / Confirmed / Cancelled counts | Bookings | group by status |
| Cancellation rate | cancelled bookings / all bookings | Bookings | watch for divide-by-zero |
| Revenue per event | money for each event | Payments -> Bookings -> Events | join through booking_id then event_id |
| Sold vs capacity per event | "80 of 100 sold" | Tickets + Events.capacity | capacity enforcement is a README feature |
| Sales by ticket type | Regular vs VIP vs Student | Tickets.type | count + sum price of Sold |
| Sales by payment method | Cash vs Card vs Wallet | Payments.method | count + sum for Completed |
| Check-in count / attendance rate | how many came | Attendees vs sold | checked-in / sold |
| Top events | best 5 events by revenue | same as revenue per event | order desc, limit 5 |

### Who sees what (from README roles)

- **Admin**: all events, all organizers.
- **Organizer**: only their own events ("view their event's sales"). So every stat query should be able to be filtered by `organizer_id`.
- Customer: no stats.

### How to build it (design idea, no code)

Follow the same 3 layers every teammate uses ("full vertical", README):

1. **Core layer** - small plain structs/classes holding results (e.g. a "summary" holder and a "per-event row" holder). No database inside. Easy to test.
2. **Repository layer** - one class that runs the SQL (sum/count/group by) and fills those holders.
3. **GUI layer** - the dashboard (Job 2) only calls the repo and shows results.

Tip for tests: Mostafa split his payment code so database is behind an interface (`IPaymentSession`), which lets him test without a real database. You can do the same: put the "calculate percentages / cancellation rate / format money" logic in core code that takes plain numbers, so it can be tested with no database.

---

## 5. Job 2 - Admin dashboard screen (`AdminDashboard.cpp`)

Current state: empty file. Needs a Qt window in the same style as `BookingWindow` (QWidget + `setupUI()`).

Suggested layout (keep it simple, it is a student project):

```
+----------------------------------------------------------+
|  Admin Dashboard                              [Refresh]  |
+----------------------------------------------------------+
|  [Total Revenue] [Tickets Sold] [Bookings] [Cancel Rate] |   <- summary cards (labels)
+----------------------------------------------------------+
|  Table: per event                                        |
|  Event | Date | Sold / Capacity | Revenue | Checked in   |
+----------------------------------------------------------+
|  Small tables or lists:                                  |
|  - Sales by ticket type    - Sales by payment method     |
+----------------------------------------------------------+
```

Things to decide / handle:

- **Who opens it?** `MainWindow` (Norhan, empty) must show it only when logged-in user is Admin. Until login exists, build the window to take a "who is viewing" input (admin id or organizer id), the same way `BookingWindow` takes `customerId` and `EventFormWindow` takes `organizerId`.
- **Admin vs Organizer mode**: same window, but organizer sees only own events.
- **Empty data**: show 0 and "no events yet", not a crash or blank.
- **Errors**: if database fails, show message box (same as `BookingWindow` does).
- **Refresh button** so numbers can be reloaded after new sales.
- README also says Admin can "manage all users". That feature is not assigned to a clear person; the dashboard is the likely home. **Ask the team** if user management belongs to you (see Section 9).

---

## 6. Job 3 - Tests / QA

The `tests/` folder does not exist yet. You create it. Two kinds of QA work:

### 6a. Automated tests (in `tests/`)

Best targets (pure logic, no database needed):

| Module | What to test | Why easy |
|---|---|---|
| Ticket states (`TicketState.cpp`) | Available->Reserved ok; Available->Sold must fail; Sold->Cancel must fail; Reserved->Sold ok; Reserved->Cancel ok | pure C++ |
| `TicketFactory` | "Regular"/"VIP"/"Student" give right type; unknown name gives null | pure C++ |
| `Booking` | `canCancel`, status <-> string conversion, unknown string falls back to Pending | pure C++ |
| Card payment checks | Luhn number check, expiry check, CVV check, declined test card | static functions, pure |
| `Money` | cents <-> text, negative amounts, rounding | pure |
| `PaymentService` | pay success, pay already-paid booking, cancelled booking, price changed, wallet not enough money, refund rules | Mostafa made fake-able interfaces, so you can write a fake session |
| Your stats logic | percentages, divide-by-zero, empty data | you write it testable |

Database tests (need real PostgreSQL with test data): booking -> pay -> cancel -> refund full flow; stats numbers match known seed data; capacity limit.

Needs a test framework decision (e.g. Catch2 / GoogleTest / plain Qt Test) and a `CMakeLists.txt` (README lists it, but it is **not in the repo yet**). Ask the team who owns CMake.

### 6b. Manual / integration QA

Walk through each README feature as each role, write results down: create event -> define venue -> add ticket types -> search -> book -> pay (3 methods) -> cancel -> check in -> see stats. Report each failure as a Jira bug.

### 6c. Problems I already spotted while reading (QA starter list)

These are real findings from reading the code. Check them and report to owners.

| # | Where | Problem | Owner |
|---|---|---|---|
| 1 | `database/schema.sql` | Only has `Attendees` table. Other 6 tables + `Wallets` are missing. Also it points to table `User(id)` - README says `Users`, and `User` is a reserved word in PostgreSQL | whole team / Norhan |
| 2 | `database/seed_data.sql` | Empty. No demo data, so stats can not be verified | shared |
| 3 | `database/DatabaseManager.cpp`, `User*`, `LoginWindow`, `MainWindow`, `main.cpp` | Empty. Nothing can connect or run yet | Norhan |
| 4 | `database/EventRepository.cpp` | Includes `DatabaseManager.h`, but only `DatabaseManager.cpp` exists | Donia / Norhan |
| 5 | `Event`/`Venue` includes | Code includes `../core/Event.h` and `Venue.h`, but files are named `Event.cpp`, `Venue.cpp`. Also Donia's files use `.h` + separate `.cpp` style, but README says only `main.cpp` is compiled. Build style mismatch | Donia |
| 6 | `core/Attendee.cpp` | `getCustomer()` returns `serial` instead of `customer_id` (copy/paste bug). No `#pragma once`. Getters not `const` | Youssef |
| 7 | `database/BookingRepository.cpp` `createBooking` | Does not check that ticket is Available before reserving. Two customers could grab the same ticket | Zeinab |
| 8 | `BookingRepository.cpp` `cancelBooking` | Sets ticket straight to Available with SQL, skipping the State pattern (Reserved->Cancelled). Also does **not** call refund, so a paid booking cancelled stays paid | Zeinab + Mostafa |
| 9 | `BookingRepository.cpp` reads | `createdAt` in Booking model is never loaded from database | Zeinab |
| 10 | `gui/BookingWindow.cpp` | Link to `PaymentWindow` is commented out - booking and payment screens are not connected | Zeinab + Mostafa |
| 11 | `TicketRepository.cpp` `getTicketById` | Returns a raw `Ticket*` made with `release()` - memory leak risk if caller forgets delete | Mahmoud |
| 12 | Capacity | README says "capacity enforcement", but nothing checks tickets created vs `Events.capacity` or venue capacity | Mahmoud / Donia |
| 13 | `README.md` vs code | README Attendees table has no `customer_id`; code/schema has it | Youssef |
| 14 | `Payment.cpp` | `Wallets` table is used but is not in README schema | Mostafa |

Do not fix others' code yourself. Report to owner through Jira, or discuss first.

---

## 7. Job 4 - Git / Jira upkeep

What I see in git now:

- Branch: you are on `Ahmed_yasser`. Remote has branches per feature: `feat/booking-core-model`, `feat/booking-database-repo`, `feat/booking-gui-window`, `mahmoud-ticket-management`, `payment/class`, `payment/repo`, `payment/gui`, `AttendeeANDCheckIn`, `Donia-Event&Venue`.
- Everything merges into `main` by pull request (PR #1 to #9 so far), merged by Norrhan2 (repo owner).
- Branch names are **not consistent** (`feat/...`, `payment/...`, person-name style). Good upkeep job: agree on one naming rule.
- Commit messages are mixed ("Update Payment.cpp", "paymentwindow gui done", "feat(event-venue): ..."). Suggest one style, e.g. `feat(module): short message`.

Your upkeep checklist:

- [ ] Agree branch naming + commit message style with team, write it in `docs/structure.md`.
- [ ] Add `.gitignore` (README lists it; not in repo yet): build folders, IDE files, compiled output.
- [ ] Ask for `CMakeLists.txt` to exist (listed in README, missing).
- [ ] Create Jira epics per module (7 owners) and one ticket per feature in README list.
- [ ] Make tickets for each bug in Section 6c with owner, steps, expected vs actual.
- [ ] Review PRs before merge: does it build? does it match README structure? any empty files being merged?
- [ ] Keep `main` always buildable. Remind people to pull/merge `main` into their branch before PR.
- [ ] Update `docs/` (structure, class diagram, ER diagram) when schema/classes change. Currently the README says these exist; folder was missing.
- [ ] Tag a release / demo version before final presentation.
- [ ] Keep Jira board status honest (To Do / In Progress / In Review / Done).

---

## 8. Suggested order of work

1. **Now** - Create Jira tickets for your 4 jobs + the 14 findings. Set up `.gitignore` and branch/commit rules.
2. **Get shared base ready** - Push team to finish `schema.sql` (all tables), `DatabaseManager`, and `seed_data.sql`. You can not test stats without tables and data. Offer to write seed data (a few events, tickets in each state, bookings, payments, one refund, a few check-ins) because you know best what numbers you need.
3. **Stats logic** - core holders -> repository queries -> unit tests for the math.
4. **Dashboard** - build the window, first with Admin view, then Organizer filter.
5. **Test suite** - state machine, factory, booking, payment rules first (cheap, high value). Then database flows.
6. **Integration** - once `MainWindow`/`LoginWindow` exist, hook dashboard in, run full manual walkthrough.
7. **Final QA pass** - regression run before demo, update docs.

---

## 9. Questions to ask the team (before you build)

1. Is a refunded payment kept as status `Refunded` (so revenue = Completed only), or do we also want "net revenue"? Pick one definition so numbers agree everywhere.
2. Does the dashboard count **Sold tickets** or **Confirmed bookings** as "sales"? They should match; if not, that is a bug to report.
3. Does "Admin manage all users" (README Roles) belong to your dashboard, or to Norhan's User module?
4. Who writes `CMakeLists.txt` and what test framework do we use?
5. Who finishes `schema.sql` and when? Can I write `seed_data.sql`?
6. Should organizer see dashboard too (README says "view their event's sales")? If yes, I will filter by organizer id.
7. How does `MainWindow` open the dashboard, and what does it pass in (user id and role)?

---

## 10. What "done" looks like for you

- Dashboard opens, shows correct numbers that match the database for known seed data.
- Organizer sees only own events; Admin sees all.
- Test folder exists, tests run with one command, key logic covered, results recorded.
- Bug list is in Jira, each with owner, and the serious ones are closed before demo.
- Repo is clean: consistent branches, `main` builds, docs up to date.
