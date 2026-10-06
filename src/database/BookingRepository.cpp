#pragma once

#include "../core/Booking.cpp"
#include "DatabaseManager.cpp"
#include <pqxx/pqxx>
#include <vector>
#include <optional>
#include <iostream>
using namespace std;

class BookingRepository {
public:
    BookingRepository() = default;

    int createBooking(int customerId, int eventId, int ticketId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "INSERT INTO Bookings (customer_id, event_id, ticket_id, status) "
                "VALUES ($1, $2, $3, 'Pending') RETURNING id;",
                customerId, eventId, ticketId
            );

            txn.exec_params(
                "UPDATE Tickets SET state = 'Reserved' WHERE id = $1;",
                ticketId
            );

            txn.commit();
            return r[0][0].as<int>();
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::createBooking: " << e.what() << endl;
            return -1;
        }
    }

    bool updateStatus(int bookingId, BookingStatus status) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            string statusStr = Booking::statusToString(status);
            txn.exec_params(
                "UPDATE Bookings SET status = $1 WHERE id = $2;",
                statusStr, bookingId
            );

            txn.commit();
            return true;
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::updateStatus: " << e.what() << endl;
            return false;
        }
    }

    bool cancelBooking(int bookingId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT ticket_id, status FROM Bookings WHERE id = $1;",
                bookingId
            );

            if (r.empty()) return false;

            string currentStatus = r[0]["status"].as<string>();
            if (currentStatus == "Cancelled") return false;

            int ticketId = r[0]["ticket_id"].as<int>();

            txn.exec_params(
                "UPDATE Bookings SET status = 'Cancelled' WHERE id = $1;",
                bookingId
            );

            txn.exec_params(
                "UPDATE Tickets SET state = 'Available' WHERE id = $1;",
                ticketId
            );

            txn.commit();
            return true;
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::cancelBooking: " << e.what() << endl;
            return false;
        }
    }

    optional<Booking> getBookingById(int bookingId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, customer_id, event_id, ticket_id, status FROM Bookings WHERE id = $1;",
                bookingId
            );

            if (r.empty()) return nullopt;

            return Booking(
                r[0]["id"].as<int>(),
                r[0]["customer_id"].as<int>(),
                r[0]["event_id"].as<int>(),
                r[0]["ticket_id"].as<int>(),
                Booking::stringToStatus(r[0]["status"].as<string>())
            );
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
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, customer_id, event_id, ticket_id, status FROM Bookings "
                "WHERE customer_id = $1 ORDER BY id DESC;",
                customerId
            );

            for (const auto& row : r) {
                results.emplace_back(
                    row["id"].as<int>(),
                    row["customer_id"].as<int>(),
                    row["event_id"].as<int>(),
                    row["ticket_id"].as<int>(),
                    Booking::stringToStatus(row["status"].as<string>())
                );
            }
        }
        catch (const exception& e) {
            cerr << "Error in BookingRepository::getBookingsByCustomer: " << e.what() << endl;
        }
        return results;
    }
};