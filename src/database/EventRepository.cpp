#pragma once

#include "../core/Event.cpp"
#include "DatabaseManager.cpp"
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

// Stateless repository: every method is static and uses its own short-lived connection.
class EventRepository {
private:
    static std::string columns() {
        return "id, organizer_id, venue_id, title, to_char(date, 'YYYY-MM-DD HH24:MI') AS date, capacity";
    }

    static Event fromRow(const pqxx::row& row) {
        return Event(row["id"].as<int>(), row["organizer_id"].as<int>(), row["venue_id"].as<int>(),
                     row["title"].as<std::string>(), row["date"].as<std::string>(), row["capacity"].as<int>());
    }

    static void setError(std::string* error, const std::string& message) {
        if (error) *error = message;
    }

public:
    static bool createEvent(Event& event, std::string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result venue = txn.exec_params("SELECT capacity FROM Venues WHERE id = $1;", event.getVenueId());
            if (venue.empty()) { setError(error, "The selected venue no longer exists."); return false; }
            if (event.getCapacity() > venue[0][0].as<int>()) {
                setError(error, "The event capacity cannot exceed the venue capacity.");
                return false;
            }

            pqxx::result res = txn.exec_params(
                "INSERT INTO Events (organizer_id, venue_id, title, date, capacity) "
                "VALUES ($1, $2, $3, $4, $5) RETURNING id;",
                event.getOrganizerId(), event.getVenueId(), event.getTitle(), event.getDate(), event.getCapacity());

            event.setId(res[0][0].as<int>());
            txn.commit();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Event Creation Error: " << e.what() << std::endl;
            setError(error, "Could not save the event. Please check the database connection.");
        }
        return false;
    }

    // Only the owning organizer can update; capacity must fit the venue and the tickets already created.
    static bool updateEvent(const Event& event, std::string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result res = txn.exec_params(
                "UPDATE Events SET venue_id = $1, title = $2, date = $3, capacity = $4 "
                "WHERE id = $5 AND organizer_id = $6 "
                "  AND $4::int <= (SELECT capacity FROM Venues WHERE id = $1) "
                "  AND $4::int >= (SELECT COUNT(*) FROM Tickets WHERE event_id = $5 AND state <> 'Cancelled') "
                "RETURNING id;",
                event.getVenueId(), event.getTitle(), event.getDate(), event.getCapacity(),
                event.getId(), event.getOrganizerId());

            if (res.empty()) {
                setError(error, "Could not update the event. It may not be yours, the capacity may exceed the "
                                "venue, or it may be lower than the number of tickets already created.");
                return false;
            }
            txn.commit();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Event Update Error: " << e.what() << std::endl;
            setError(error, "Could not save the event. Please check the database connection.");
        }
        return false;
    }

    static bool deleteEvent(int eventId, int organizerId, std::string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result res = txn.exec_params(
                "DELETE FROM Events WHERE id = $1 AND organizer_id = $2 RETURNING id;", eventId, organizerId);
            if (res.empty()) { setError(error, "Event not found, or it is not yours."); return false; }
            txn.commit();
            return true;
        }
        catch (const pqxx::foreign_key_violation&) {
            setError(error, "This event already has bookings and cannot be deleted.");
        }
        catch (const std::exception& e) {
            std::cerr << "Event Deletion Error: " << e.what() << std::endl;
            setError(error, "Could not delete the event.");
        }
        return false;
    }

    static std::optional<Event> getEventById(int eventId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result res = txn.exec_params("SELECT " + columns() + " FROM Events WHERE id = $1;", eventId);
            if (!res.empty()) return fromRow(res[0]);
        }
        catch (const std::exception& e) {
            std::cerr << "Get Event Error: " << e.what() << std::endl;
        }
        return std::nullopt;
    }

    static std::vector<Event> getEventsByOrganizer(int organizerId) {
        std::vector<Event> list;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result res = txn.exec_params(
                "SELECT " + columns() + " FROM Events WHERE organizer_id = $1 ORDER BY Events.date ASC;", organizerId);
            for (const auto& row : res) list.push_back(fromRow(row));
        }
        catch (const std::exception& e) {
            std::cerr << "Get Organizer Events Error: " << e.what() << std::endl;
        }
        return list;
    }

    static std::vector<Event> searchEvents(const std::string& queryTitle) {
        std::vector<Event> list;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result res = txn.exec_params(
                "SELECT " + columns() + " FROM Events WHERE LOWER(title) LIKE LOWER($1) ORDER BY Events.date ASC;",
                "%" + queryTitle + "%");
            for (const auto& row : res) list.push_back(fromRow(row));
        }
        catch (const std::exception& e) {
            std::cerr << "Search Events Error: " << e.what() << std::endl;
        }
        return list;
    }

    static int countAvailableTickets(int eventId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result res = txn.exec_params(
                "SELECT COUNT(*) FROM Tickets WHERE event_id = $1 AND state = 'Available';", eventId);
            return res[0][0].as<int>();
        }
        catch (const std::exception& e) {
            std::cerr << "Count Tickets Error: " << e.what() << std::endl;
        }
        return 0;
    }
};