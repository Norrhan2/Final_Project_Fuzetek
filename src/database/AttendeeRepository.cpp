#pragma once

#include "../core/Attendee.cpp"
#include "DatabaseManager.cpp"
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>
using namespace std;

struct CheckInResult {
    bool success = false;
    string message;
};

// One row of the customer's check-in screen.
struct CheckInEntry {
    int bookingId = 0;
    string eventTitle;
    string eventDate;
    string ticketType;
    string checkedInAt;   // empty = not checked in yet

    bool isCheckedIn() const { return !checkedInAt.empty(); }
};

class AttendeeRepository {
private:
    static constexpr int WINDOW_HOURS = 24;   // check-in opens/closes this long around the event start

public:
    AttendeeRepository() = default;

    // Rules: the booking is yours, it is Confirmed (paid), the event is within the check-in window,
    // and it can only be checked in once (UNIQUE booking_id).
    CheckInResult checkIn(int bookingId, int customerId) {
        auto fail = [](const string& message) { return CheckInResult{ false, message }; };

        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            const string hours = to_string(WINDOW_HOURS);
            pqxx::result r = txn.exec_params(
                "SELECT b.customer_id, b.status, "
                "       (now() >= e.date - interval '" + hours + " hours' "
                "        AND now() <= e.date + interval '" + hours + " hours') AS in_window "
                "FROM Bookings b JOIN Events e ON e.id = b.event_id "
                "WHERE b.id = $1 FOR UPDATE OF b;",
                bookingId);

            if (r.empty()) return fail("Booking not found.");
            if (r[0]["customer_id"].as<int>() != customerId) return fail("This booking belongs to another customer.");
            if (r[0]["status"].as<string>() != "Confirmed") return fail("Only paid (confirmed) bookings can be checked in.");
            if (!r[0]["in_window"].as<bool>())
                return fail("Check-in is only possible from " + hours + " hours before until " + hours +
                            " hours after the event starts.");

            pqxx::result ins = txn.exec_params(
                "INSERT INTO Attendees (booking_id, customer_id) VALUES ($1, $2) "
                "ON CONFLICT (booking_id) DO NOTHING RETURNING id;",
                bookingId, customerId);
            if (ins.empty()) return fail("You are already checked in for this booking.");

            txn.commit();
            return { true, "Checked in successfully. Enjoy the event!" };
        }
        catch (const exception& e) {
            cerr << "Error in AttendeeRepository::checkIn: " << e.what() << endl;
            return fail("Could not check in. Please try again.");
        }
    }

    vector<CheckInEntry> getEntriesByCustomer(int customerId) {
        vector<CheckInEntry> entries;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT b.id AS booking_id, e.title, to_char(e.date, 'YYYY-MM-DD HH24:MI') AS event_date, t.type, "
                "       COALESCE(to_char(a.checked_in_at, 'YYYY-MM-DD HH24:MI'), '') AS checked_in_at "
                "FROM Bookings b "
                "JOIN Events e ON e.id = b.event_id "
                "JOIN Tickets t ON t.id = b.ticket_id "
                "LEFT JOIN Attendees a ON a.booking_id = b.id "
                "WHERE b.customer_id = $1 AND b.status = 'Confirmed' "
                "ORDER BY e.date;",
                customerId);

            for (const auto& row : r) {
                CheckInEntry entry;
                entry.bookingId = row["booking_id"].as<int>();
                entry.eventTitle = row["title"].as<string>();
                entry.eventDate = row["event_date"].as<string>();
                entry.ticketType = row["type"].as<string>();
                entry.checkedInAt = row["checked_in_at"].as<string>();
                entries.push_back(entry);
            }
        }
        catch (const exception& e) {
            cerr << "Error in AttendeeRepository::getEntriesByCustomer: " << e.what() << endl;
        }
        return entries;
    }

    optional<Attendee> findByBooking(int bookingId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, booking_id, customer_id, to_char(checked_in_at, 'YYYY-MM-DD HH24:MI') AS checked_in_at "
                "FROM Attendees WHERE booking_id = $1;", bookingId);
            if (r.empty()) return nullopt;
            return Attendee(r[0]["id"].as<int>(), r[0]["booking_id"].as<int>(),
                            r[0]["customer_id"].as<int>(), r[0]["checked_in_at"].as<string>());
        }
        catch (const exception& e) {
            cerr << "Error in AttendeeRepository::findByBooking: " << e.what() << endl;
            return nullopt;
        }
    }
};