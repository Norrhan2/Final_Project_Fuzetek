#include "VenueRepository.h"
#include "DatabaseManager.h"
#include <pqxx/pqxx>
#include <iostream>

bool VenueRepository::createVenue(Venue &venue)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(
            "INSERT INTO Venues (name, address, capacity) VALUES ($1, $2, $3) RETURNING id;",
            venue.getName(), venue.getAddress(), venue.getCapacity());

        if (!res.empty())
        {
            venue.setId(res[0][0].as<int>());
            txn.commit();
            return true;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Venue Creation Error: " << e.what() << std::endl;
    }
    return false;
}

bool VenueRepository::updateVenue(const Venue &venue)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        txn.exec_params(
            "UPDATE Venues SET name = $1, address = $2, capacity = $3 WHERE id = $4;",
            venue.getName(), venue.getAddress(), venue.getCapacity(), venue.getId());
        txn.commit();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Venue Update Error: " << e.what() << std::endl;
    }
    return false;
}

bool VenueRepository::deleteVenue(int venueId)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        txn.exec_params("DELETE FROM Venues WHERE id = $1;", venueId);
        txn.commit();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Venue Deletion Error: " << e.what() << std::endl;
    }
    return false;
}

std::optional<Venue> VenueRepository::getVenueById(int venueId)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::nontransaction txn(*conn);

        pqxx::result res = txn.exec_params("SELECT id, name, address, capacity FROM Venues WHERE id = $1;", venueId);
        if (!res.empty())
        {
            const auto &row = res[0];
            return Venue(row[0].as<int>(), row[1].as<std::string>(), row[2].as<std::string>(), row[3].as<int>());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Get Venue Error: " << e.what() << std::endl;
    }
    return std::nullopt;
}

std::vector<Venue> VenueRepository::getAllVenues()
{
    std::vector<Venue> list;
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::nontransaction txn(*conn);

        pqxx::result res = txn.exec("SELECT id, name, address, capacity FROM Venues ORDER BY name ASC;");
        for (const auto &row : res)
        {
            list.emplace_back(row[0].as<int>(), row[1].as<std::string>(), row[2].as<std::string>(), row[3].as<int>());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Get All Venues Error: " << e.what() << std::endl;
    }
    return list;
}