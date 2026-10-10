#pragma once

#include "../core/Booking.cpp"
#include "../core/Ticket.cpp"
#include "DatabaseManager.cpp"
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>
using namespace std;

class BookingRepository {
private:
    // Locks the ticket row, rebuilds the Ticket object in its CURRENT state, lets the State pattern decide
    // whether the transition is legal, then writes the new state back.
    static bool transitionTicket(pqxx::work& txn, int ticketId, const function<bool(Ticket&)>& action) {
        pqxx::result r = txn.exec_params(
            "SELECT id, event_id, type, price, state FROM Tickets WHERE id = $1 FOR UPDATE;", ticketId);
        if (r.empty()) return false;

        unique_ptr<Ticket> ticket = TicketFactory::createTicket(
            r[0]["type"].as<string>(), r[0]["id"].as<int>(), r[0]["event_id"].as<int>(),
            r[0]["price"].as<double>(), r[0]["state"].as<string>());
        if (!ticket || !action(*ticket)) return false;

        txn.exec_params("UPDATE Tickets SET state = $1 WHERE id = $2;", ticket->getState(), ticketId);
        return true;
    }

    static Booking fromRow(const pqxx::row& row) {
        return Booking(row["id"].as<int>(), row["customer_id"].as<int>(), row["event_id"].as<int>(),
                       row["ticket_id"].as<int>(), Booking::stringToStatus(row["status"].as<string>()));
    }

public:
    BookingRepository() = default;

    // Reserves the ticket and creates a Pending booking. Returns the booking id, or -1 (and fills *error).
    int createBooking(int customerId, int eventId, int ticketId, string* error = nullptr) {
        auto fail = [&](const string& message) { if (error) *error = message; return -1; };

        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result ev = txn.exec_params("SELECT (date > now()) AS upcoming FROM Events WHERE id = $1;", eventId);
            if (ev.empty()) return fail("Event not found.");
            if (!ev[0]["upcoming"].as<bool>()) return fail("This event has already started or finished.");

            pqxx::result own = txn.exec_params("SELECT 1 FROM Tickets WHERE id = $1 AND event_id = $2;", ticketId, eventId);
            if (own.empty()) return fail("This ticket does not belong to the selected event.");

            if (!transitionTicket(txn, ticketId, [](Ticket& t) { return t.reserve(); }))
                return fail("Sorry, this ticket is no longer available.");

            pqxx::result r = txn.exec_params(
                "INSERT INTO Bookings (customer_id, event_id, ticket_id, status) "
                "VALUES ($1, $2, $3, 'Pending') RETURNING id;",
                customerId, eventId, ticketId);

            txn.commit();
            return r[0][0].as<int>();
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::createBooking: " << e.what() << endl;
            return fail("Could not create the booking. Please try again.");
        }
    }

    bool updateStatus(int bookingId, BookingStatus status) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);
            txn.exec_params("UPDATE Bookings SET status = $1 WHERE id = $2;", Booking::statusToString(status), bookingId);
            txn.commit();
            return true;
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::updateStatus: " << e.what() << endl;
            return false;
        }
    }

    // Cancels the booking and puts the ticket back on sale (State pattern: release()).
    // The caller must then call PaymentService::refundBooking() so a paid booking gets its money back.
    bool cancelBooking(int bookingId, int customerId, string* error = nullptr) {
        auto fail = [&](const string& message) { if (error) *error = message; return false; };

        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT b.ticket_id, b.status, b.customer_id, (e.date > now()) AS upcoming "
                "FROM Bookings b JOIN Events e ON e.id = b.event_id WHERE b.id = $1 FOR UPDATE OF b;",
                bookingId);

            if (r.empty()) return fail("Booking not found.");
            if (r[0]["customer_id"].as<int>() != customerId) return fail("This booking belongs to another customer.");
            if (r[0]["status"].as<string>() == "Cancelled") return fail("This booking is already cancelled.");
            if (!r[0]["upcoming"].as<bool>()) return fail("Bookings cannot be cancelled after the event has started.");

            pqxx::result checkedIn = txn.exec_params("SELECT 1 FROM Attendees WHERE booking_id = $1;", bookingId);
            if (!checkedIn.empty()) return fail("This booking is already checked in and cannot be cancelled.");

            int ticketId = r[0]["ticket_id"].as<int>();

            txn.exec_params("UPDATE Bookings SET status = 'Cancelled' WHERE id = $1;", bookingId);
            if (!transitionTicket(txn, ticketId, [](Ticket& t) { return t.release(); }))
                return fail("The ticket could not be released.");

            txn.commit();
            return true;
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::cancelBooking: " << e.what() << endl;
            return fail("Could not cancel the booking. Please try again.");
        }
    }

    optional<Booking> getBookingById(int bookingId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, customer_id, event_id, ticket_id, status FROM Bookings WHERE id = $1;", bookingId);
            if (r.empty()) return nullopt;
            return fromRow(r[0]);
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::getBookingById: " << e.what() << endl;
            return nullopt;
        }
    }

    vector<Booking> getBookingsByCustomer(int customerId) {
        vector<Booking> results;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, customer_id, event_id, ticket_id, status FROM Bookings "
                "WHERE customer_id = $1 ORDER BY id DESC;", customerId);
            for (const auto& row : r) results.push_back(fromRow(row));
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::getBookingsByCustomer: " << e.what() << endl;
        }
        return results;
    }
};