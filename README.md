# Event & Ticket Management System

A desktop application (C++) for creating, managing, and booking event tickets.
Built for Summer Training 2026 — Final Project.

## Problem
Small-to-mid event organizers currently manage events, ticket sales, and
check-ins through spreadsheets or manual processes, leading to overbooking,
lost sales records, and slow entry lines. This system centralizes event
creation, ticket sales, payments, and attendance tracking in one application.

## User Roles
| Role            | Can do |
|------------------|--------|
| Admin            | Manage all users, view sales statistics across all events |
| Event Organizer  | Create/edit/delete their events, define venues, define ticket types, view their event's sales |
| Customer         | Search events, book tickets, pay, cancel bookings, check in |

## Features
- Create / edit / delete events
- Define venues
- Define ticket types (Regular, VIP, Student)
- Ticket booking
- Payment (Cash, Card, Wallet)
- Cancel booking
- Attendance / check-in
- Search events
- Event capacity enforcement
- Sales statistics

## Tech Stack
- C++17
- Qt (GUI)
- PostgreSQL (via libpqxx)
- Git / GitHub, Jira

## Architecture & Design Patterns
- **Factory Pattern** — `TicketFactory` creates `RegularTicket`, `VIPTicket`,
  `StudentTicket` without the rest of the app knowing the concrete class.
- **Strategy Pattern** — `PaymentStrategy` lets a `Payment` be processed as
  `CashPayment`, `CardPayment`, or `WalletPayment` interchangeably.
- **State Pattern** — a `Ticket` moves between `Available → Reserved → Sold /
  Cancelled` states, with each state controlling what actions are valid.
- **Inheritance** — `User` is the base class for `Admin`, `Organizer`,
  `Customer`, each overriding role-specific permissions.

## Database Schema (tables & relationships)
Users (id, name, email, password, role)
Events (id, organizer_id FK, venue_id FK, title, date, capacity)
Venues (id, name, address, capacity)
Tickets (id, event_id FK, type, price, state)
Bookings (id, customer_id FK, event_id FK, ticket_id FK, status)
Payments (id, booking_id FK, method, amount, status)
Attendees (id, booking_id FK, checked_in_at)

Relationships:
- Organizer 1───N Events
- Event N───1 Venue
- Event 1───N Tickets
- Customer 1───N Bookings, Event 1───N Bookings
- Booking 1───1 Payment
- Booking 1───1 Attendee (on check-in)

## Project Structure
See `docs/structure.md` for full folder layout and file-by-file ownership.

## Team & Module Ownership
| Member | Module | Owns |
|---|---|---|
| [Leader name] | Auth, Roles & Core Infra | User/Admin/Organizer/Customer classes, DatabaseManager, Login screen, Main shell |
| [Member 2] | Event & Venue | Event/Venue classes, their DB repos, create/edit/search event screens |
| [Member 3] | Ticket Management | Ticket hierarchy, TicketFactory, Ticket states, ticket DB repo, ticket-type screen |
| [Member 4] | Booking & Cancellation | Booking class, booking DB repo, booking/cancel screens |
| [Member 5] | Payment | Payment class, PaymentStrategy hierarchy, payment DB repo, payment screen |
| [Member 6] | Check-in / Attendance | Attendee class, attendee DB repo, check-in screen |
| [Member 7] | Admin Dashboard & Stats + QA | Sales statistics logic, admin dashboard screen, tests, Git/Jira upkeep |

Each member is responsible for their module's full vertical: class design →
database table & queries → GUI screen. No one should touch only one layer.

