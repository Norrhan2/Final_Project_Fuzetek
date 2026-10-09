#include "EventRepository.h"
#include "DatabaseManager.h"
#include <pqxx/pqxx>
#include <iostream>

bool EventRepository::createEvent(Event &event)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        pqxx::result res = txn.exec_params(
            "INSERT INTO Events (organizer_id, venue_id, title, date, capacity) "
            "VALUES ($1, $2, $3, $4, $5) RETURNING id;",
            event.getOrganizerId(), event.getVenueId(), event.getTitle(), event.getDate(), event.getCapacity());

        if (!res.empty())
        {
            event.setId(res[0][0].as<int>());
            txn.commit();
            return true;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Event Creation Error: " << e.what() << std::endl;
    }
    return false;
}

bool EventRepository::updateEvent(const Event &event)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        txn.exec_params(
            "UPDATE Events SET venue_id = $1, title = $2, date = $3, capacity = $4 WHERE id = $5;",
            event.getVenueId(), event.getTitle(), event.getDate(), event.getCapacity(), event.getId());
        txn.commit();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Event Update Error: " << e.what() << std::endl;
    }
    return false;
}

bool EventRepository::deleteEvent(int eventId)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::work txn(*conn);

        txn.exec_params("DELETE FROM Events WHERE id = $1;", eventId);
        txn.commit();
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Event Deletion Error: " << e.what() << std::endl;
    }
    return false;
}

std::optional<Event> EventRepository::getEventById(int eventId)
{
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::nontransaction txn(*conn);

        pqxx::result res = txn.exec_params("SELECT id, organizer_id, venue_id, title, date, capacity FROM Events WHERE id = $1;", eventId);
        if (!res.empty())
        {
            const auto &row = res[0];
            return Event(row[0].as<int>(), row[1].as<int>(), row[2].as<int>(), row[3].as<std::string>(), row[4].as<std::string>(), row[5].as<int>());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Get Event Error: " << e.what() << std::endl;
    }
    return std::nullopt;
}

std::vector<Event> EventRepository::searchEvents(const std::string &queryTitle)
{
    std::vector<Event> list;
    try
    {
        auto conn = DatabaseManager::getInstance().getConnection();
        pqxx::nontransaction txn(*conn);

        std::string pattern = "%" + queryTitle + "%";
        pqxx::result res = txn.exec_params(
            "SELECT id, organizer_id, venue_id, title, date, capacity FROM Events WHERE LOWER(title) LIKE LOWER($1) ORDER BY date ASC;",
            pattern);

        for (const auto &row : res)
        {
            list.emplace_back(row[0].as<int>(), row[1].as<int>(), row[2].as<int>(), row[3].as<std::string>(), row[4].as<std::string>(), row[5].as<int>());
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Search Events Error: " << e.what() << std::endl;
    }
    return list;
}