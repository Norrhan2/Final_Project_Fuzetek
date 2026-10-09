#pragma once

#include "../core/SalesStatistics.cpp"
#include "DatabaseManager.cpp"
#include <pqxx/pqxx>
#include <iostream>
#include <optional>
#include <string>
using namespace std;

class StatsRepository {
    // organizerId = 0 means "all events" (Admin). Otherwise only that organizer's events.
    static string organizerFilter() {
        return "($1::int = 0 OR e.organizer_id = $1::int)";
    }

public:
    StatsRepository() = default;

    optional<SalesReport> getReport(int organizerId = 0) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);   // read-only use, never committed

            SalesReport report;
            const string filter = organizerFilter();

            // Revenue and refunds
            pqxx::result money = txn.exec_params(
                "SELECT "
                "  round(COALESCE(SUM(CASE WHEN p.status = 'Completed' THEN p.amount END), 0) * 100)::bigint AS revenue_cents, "
                "  round(COALESCE(SUM(CASE WHEN p.status = 'Refunded'  THEN p.amount END), 0) * 100)::bigint AS refunded_cents "
                "FROM Payments p "
                "JOIN Bookings b ON b.id = p.booking_id "
                "JOIN Events e ON e.id = b.event_id "
                "WHERE " + filter + ";",
                organizerId
            );
            report.summary.revenueCents = money[0]["revenue_cents"].as<long long>();
            report.summary.refundedCents = money[0]["refunded_cents"].as<long long>();

            // Tickets by state
            pqxx::result tickets = txn.exec_params(
                "SELECT t.state, COUNT(*) AS n "
                "FROM Tickets t JOIN Events e ON e.id = t.event_id "
                "WHERE " + filter + " GROUP BY t.state;",
                organizerId
            );
            for (const auto& row : tickets) {
                string state = row["state"].as<string>();
                int n = row["n"].as<int>();
                if (state == "Available") report.summary.ticketsAvailable = n;
                else if (state == "Reserved") report.summary.ticketsReserved = n;
                else if (state == "Sold") report.summary.ticketsSold = n;
                else if (state == "Cancelled") report.summary.ticketsCancelled = n;
            }

            // Bookings by status
            pqxx::result bookings = txn.exec_params(
                "SELECT b.status, COUNT(*) AS n "
                "FROM Bookings b JOIN Events e ON e.id = b.event_id "
                "WHERE " + filter + " GROUP BY b.status;",
                organizerId
            );
            for (const auto& row : bookings) {
                string status = row["status"].as<string>();
                int n = row["n"].as<int>();
                if (status == "Pending") report.summary.bookingsPending = n;
                else if (status == "Confirmed") report.summary.bookingsConfirmed = n;
                else if (status == "Cancelled") report.summary.bookingsCancelled = n;
            }

            // Check-ins
            pqxx::result checkIns = txn.exec_params(
                "SELECT COUNT(*) AS n "
                "FROM Attendees a "
                "JOIN Bookings b ON b.id = a.booking_id "
                "JOIN Events e ON e.id = b.event_id "
                "WHERE " + filter + ";",
                organizerId
            );
            report.summary.checkedIn = checkIns[0]["n"].as<int>();

            // Sales by ticket type (completed payments only)
            pqxx::result byType = txn.exec_params(
                "SELECT t.type AS label, COUNT(*) AS n, "
                "       round(COALESCE(SUM(p.amount), 0) * 100)::bigint AS amount_cents "
                "FROM Payments p "
                "JOIN Bookings b ON b.id = p.booking_id "
                "JOIN Tickets t ON t.id = b.ticket_id "
                "JOIN Events e ON e.id = b.event_id "
                "WHERE p.status = 'Completed' AND " + filter + " "
                "GROUP BY t.type ORDER BY t.type;",
                organizerId
            );
            for (const auto& row : byType) {
                report.byTicketType.push_back({
                    row["label"].as<string>(),
                    row["n"].as<int>(),
                    row["amount_cents"].as<long long>()
                });
            }

            // Sales by payment method (completed payments only)
            pqxx::result byMethod = txn.exec_params(
                "SELECT p.method AS label, COUNT(*) AS n, "
                "       round(COALESCE(SUM(p.amount), 0) * 100)::bigint AS amount_cents "
                "FROM Payments p "
                "JOIN Bookings b ON b.id = p.booking_id "
                "JOIN Events e ON e.id = b.event_id "
                "WHERE p.status = 'Completed' AND " + filter + " "
                "GROUP BY p.method ORDER BY p.method;",
                organizerId
            );
            for (const auto& row : byMethod) {
                report.byPaymentMethod.push_back({
                    row["label"].as<string>(),
                    row["n"].as<int>(),
                    row["amount_cents"].as<long long>()
                });
            }

            // Per event (subqueries avoid join row-multiplication)
            pqxx::result perEvent = txn.exec_params(
                "SELECT e.id, e.title, e.date::text AS date, e.capacity, "
                "  (SELECT COUNT(*) FROM Tickets t WHERE t.event_id = e.id AND t.state = 'Sold') AS sold, "
                "  (SELECT COUNT(*) FROM Tickets t WHERE t.event_id = e.id) AS total, "
                "  (SELECT round(COALESCE(SUM(p.amount), 0) * 100)::bigint "
                "     FROM Payments p JOIN Bookings b ON b.id = p.booking_id "
                "     WHERE b.event_id = e.id AND p.status = 'Completed') AS revenue_cents, "
                "  (SELECT COUNT(*) FROM Attendees a JOIN Bookings b ON b.id = a.booking_id "
                "     WHERE b.event_id = e.id) AS checked_in "
                "FROM Events e "
                "WHERE " + filter + " "
                "ORDER BY revenue_cents DESC, e.date ASC;",
                organizerId
            );
            for (const auto& row : perEvent) {
                EventSales ev;
                ev.eventId = row["id"].as<int>();
                ev.title = row["title"].as<string>();
                ev.date = row["date"].as<string>();
                ev.capacity = row["capacity"].as<int>();
                ev.ticketsSold = row["sold"].as<int>();
                ev.ticketsTotal = row["total"].as<int>();
                ev.revenueCents = row["revenue_cents"].as<long long>();
                ev.checkedIn = row["checked_in"].as<int>();
                report.events.push_back(ev);
            }

            return report;
        }
        catch (const exception& e) {
            cerr << "Error in StatsRepository::getReport: " << e.what() << endl;
            return nullopt;
        }
    }
};
