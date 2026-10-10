#pragma once

#include "../core/Ticket.cpp"
#include "DatabaseManager.cpp"
#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <vector>
using namespace std;

// Every row in Tickets is ONE seat. Capacity of the event = maximum number of non-cancelled tickets.
class TicketRepository {
private:
    static unique_ptr<Ticket> fromRow(const pqxx::row& row) {
        return TicketFactory::createTicket(row["type"].as<string>(), row["id"].as<int>(),
                                           row["event_id"].as<int>(), row["price"].as<double>(),
                                           row["state"].as<string>());
    }

    static void setError(string* error, const string& message) {
        if (error) *error = message;
    }

    vector<unique_ptr<Ticket>> queryByEvent(int eventId, const string& extraCondition) {
        vector<unique_ptr<Ticket>> results;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, event_id, type, price, state FROM Tickets WHERE event_id = $1 " + extraCondition +
                " ORDER BY id;", eventId);
            for (const auto& row : r) {
                auto ticket = fromRow(row);
                if (ticket) results.push_back(std::move(ticket));
            }
        }
        catch (const exception& e) {
            cerr << "Error in TicketRepository::queryByEvent: " << e.what() << endl;
        }
        return results;
    }

public:
    TicketRepository() = default;

    // Creates `quantity` tickets in one transaction. Returns how many were created, or -1 (and fills *error).
    int createTickets(int eventId, const string& type, double price, int quantity, int organizerId,
                      string* error = nullptr) {
        auto fail = [&](const string& message) { setError(error, message); return -1; };

        if (type != "Regular" && type != "VIP" && type != "Student") return fail("Unknown ticket type.");
        if (price < 0) return fail("The price cannot be negative.");
        if (quantity < 1 || quantity > 1000) return fail("Quantity must be between 1 and 1000.");

        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            // Lock the event row so two requests cannot both squeeze under the capacity.
            pqxx::result ev = txn.exec_params(
                "SELECT organizer_id, capacity FROM Events WHERE id = $1 FOR UPDATE;", eventId);
            if (ev.empty()) return fail("Event not found.");
            if (ev[0]["organizer_id"].as<int>() != organizerId)
                return fail("You can only add tickets to your own events.");

            int capacity = ev[0]["capacity"].as<int>();
            int existing = txn.exec_params(
                "SELECT COUNT(*) FROM Tickets WHERE event_id = $1 AND state <> 'Cancelled';", eventId)[0][0].as<int>();
            if (existing + quantity > capacity)
                return fail("Capacity exceeded: the event holds " + to_string(capacity) + " tickets and already has " +
                            to_string(existing) + ".");

            for (int i = 0; i < quantity; ++i) {
                txn.exec_params(
                    "INSERT INTO Tickets (event_id, type, price, state) VALUES ($1, $2, $3, 'Available');",
                    eventId, type, price);
            }
            txn.commit();
            return quantity;
        }
        catch (const exception& e) {
            cerr << "Error in TicketRepository::createTickets: " << e.what() << endl;
            return fail("Could not create the tickets. Please check the database connection.");
        }
    }

    unique_ptr<Ticket> getTicketById(int ticketId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, event_id, type, price, state FROM Tickets WHERE id = $1;", ticketId);
            if (r.empty()) return nullptr;
            return fromRow(r[0]);
        }
        catch (const exception& e) {
            cerr << "Error in TicketRepository::getTicketById: " << e.what() << endl;
            return nullptr;
        }
    }

    vector<unique_ptr<Ticket>> getTicketsByEvent(int eventId) { return queryByEvent(eventId, ""); }

    vector<unique_ptr<Ticket>> getAvailableTicketsByEvent(int eventId) {
        return queryByEvent(eventId, "AND state = 'Available'");
    }

    // Only Available tickets of the organizer's own events can be removed.
    bool deleteTicket(int ticketId, int organizerId, string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "DELETE FROM Tickets WHERE id = $1 AND state = 'Available' "
                "AND event_id IN (SELECT id FROM Events WHERE organizer_id = $2) RETURNING id;",
                ticketId, organizerId);
            if (r.empty()) {
                setError(error, "Only Available tickets of your own events can be deleted.");
                return false;
            }
            txn.commit();
            return true;
        }
        catch (const pqxx::foreign_key_violation&) {
            setError(error, "This ticket has booking history and cannot be deleted.");
        }
        catch (const exception& e) {
            cerr << "Error in TicketRepository::deleteTicket: " << e.what() << endl;
            setError(error, "Could not delete the ticket.");
        }
        return false;
    }
};