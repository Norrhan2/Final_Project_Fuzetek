#pragma once

#include "../core/Venue.cpp"
#include "DatabaseManager.cpp"
#include <iostream>
#include <optional>
#include <pqxx/pqxx>
#include <string>
#include <vector>

class VenueRepository {
private:
    static Venue fromRow(const pqxx::row& row) {
        return Venue(row["id"].as<int>(), row["name"].as<std::string>(),
                     row["address"].as<std::string>(), row["capacity"].as<int>());
    }

    static void setError(std::string* error, const std::string& message) {
        if (error) *error = message;
    }

public:
    static bool createVenue(Venue& venue, std::string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result res = txn.exec_params(
                "INSERT INTO Venues (name, address, capacity) VALUES ($1, $2, $3) RETURNING id;",
                venue.getName(), venue.getAddress(), venue.getCapacity());

            venue.setId(res[0][0].as<int>());
            txn.commit();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Venue Creation Error: " << e.what() << std::endl;
            setError(error, "Could not save the venue. Please check the database connection.");
        }
        return false;
    }

    // A venue cannot shrink below the biggest event already planned in it.
    static bool updateVenue(const Venue& venue, std::string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result res = txn.exec_params(
                "UPDATE Venues SET name = $1, address = $2, capacity = $3 "
                "WHERE id = $4 "
                "  AND $3::int >= COALESCE((SELECT MAX(capacity) FROM Events WHERE venue_id = $4), 0) "
                "RETURNING id;",
                venue.getName(), venue.getAddress(), venue.getCapacity(), venue.getId());

            if (res.empty()) {
                setError(error, "Could not update the venue: its capacity is lower than an event planned in it.");
                return false;
            }
            txn.commit();
            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Venue Update Error: " << e.what() << std::endl;
            setError(error, "Could not save the venue. Please check the database connection.");
        }
        return false;
    }

    static bool deleteVenue(int venueId, std::string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);
            txn.exec_params("DELETE FROM Venues WHERE id = $1;", venueId);
            txn.commit();
            return true;
        }
        catch (const pqxx::foreign_key_violation&) {
            setError(error, "This venue is used by existing events and cannot be deleted.");
        }
        catch (const std::exception& e) {
            std::cerr << "Venue Deletion Error: " << e.what() << std::endl;
            setError(error, "Could not delete the venue.");
        }
        return false;
    }

    static std::optional<Venue> getVenueById(int venueId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result res = txn.exec_params("SELECT id, name, address, capacity FROM Venues WHERE id = $1;", venueId);
            if (!res.empty()) return fromRow(res[0]);
        }
        catch (const std::exception& e) {
            std::cerr << "Get Venue Error: " << e.what() << std::endl;
        }
        return std::nullopt;
    }

    static std::vector<Venue> getAllVenues() {
        std::vector<Venue> list;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result res = txn.exec("SELECT id, name, address, capacity FROM Venues ORDER BY name ASC;");
            for (const auto& row : res) list.push_back(fromRow(row));
        }
        catch (const std::exception& e) {
            std::cerr << "Get All Venues Error: " << e.what() << std::endl;
        }
        return list;
    }
};