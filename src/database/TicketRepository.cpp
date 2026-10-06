#pragma once

#include "../core/Ticket.cpp"
#include "DatabaseManager.cpp"

#include <pqxx/pqxx>
#include <vector>
#include <optional>
#include <iostream>

using namespace std;


class TicketRepository {
public:

    TicketRepository() = default;


    int createTicket(
        int eventId,
        const string& type,
        double price
    ) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();

            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "INSERT INTO Tickets "
                "(event_id, type, price, state) "
                "VALUES ($1, $2, $3, 'Available') "
                "RETURNING id;",
                eventId,
                type,
                price
            );

            txn.commit();

            return r[0][0].as<int>();
        }
        catch (const exception& e) {
            cerr << "Error in TicketRepository::createTicket: "
                 << e.what() << endl;

            return -1;
        }
    }


    optional<Ticket*> getTicketById(int ticketId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();

            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, event_id, type, price, state "
                "FROM Tickets "
                "WHERE id = $1;",
                ticketId
            );

            if (r.empty()) {
                return nullopt;
            }

            int id = r[0]["id"].as<int>();
            int eventId = r[0]["event_id"].as<int>();
            string type = r[0]["type"].as<string>();
            double price = r[0]["price"].as<double>();
            string state = r[0]["state"].as<string>();

            unique_ptr<Ticket> ticket =
                TicketFactory::createTicket(
                    type,
                    id,
                    eventId,
                    price,
                    state
                );

            if (!ticket) {
                return nullopt;
            }

            Ticket* resultTicket = ticket.release();

            return resultTicket;
        }
        catch (const exception& e) {
            cerr << "Error in TicketRepository::getTicketById: "
                 << e.what() << endl;

            return nullopt;
        }
    }


    vector<unique_ptr<Ticket>> getTicketsByEvent(int eventId) {

        vector<unique_ptr<Ticket>> results;

        try {
            auto conn = DatabaseManager::getInstance().getConnection();

            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, event_id, type, price, state "
                "FROM Tickets "
                "WHERE event_id = $1 "
                "ORDER BY id;",
                eventId
            );

            for (const auto& row : r) {

                int id =
                    row["id"].as<int>();

                int currentEventId =
                    row["event_id"].as<int>();

                string type =
                    row["type"].as<string>();

                double price =
                    row["price"].as<double>();

                string state =
                    row["state"].as<string>();

                unique_ptr<Ticket> ticket =
                    TicketFactory::createTicket(
                        type,
                        id,
                        currentEventId,
                        price,
                        state
                    );

                if (ticket) {
                    results.push_back(
                        move(ticket)
                    );
                }
            }
        }
        catch (const exception& e) {

            cerr << "Error in TicketRepository::getTicketsByEvent: "
                 << e.what() << endl;
        }

        return results;
    }


    bool updateTicketState(
        int ticketId,
        const string& newState
    ) {
        if (
            newState != "Available" &&
            newState != "Reserved" &&
            newState != "Sold" &&
            newState != "Cancelled"
        ) {
            return false;
        }

        try {
            auto conn =
                DatabaseManager::getInstance().getConnection();

            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "UPDATE Tickets "
                "SET state = $1 "
                "WHERE id = $2 "
                "RETURNING id;",
                newState,
                ticketId
            );

            if (r.empty()) {
                return false;
            }

            txn.commit();

            return true;
        }
        catch (const exception& e) {

            cerr << "Error in TicketRepository::updateTicketState: "
                 << e.what() << endl;

            return false;
        }
    }


    bool deleteTicket(int ticketId) {

        try {
            auto conn =
                DatabaseManager::getInstance().getConnection();

            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "DELETE FROM Tickets "
                "WHERE id = $1 "
                "RETURNING id;",
                ticketId
            );

            if (r.empty()) {
                return false;
            }

            txn.commit();

            return true;
        }
        catch (const exception& e) {

            cerr << "Error in TicketRepository::deleteTicket: "
                 << e.what() << endl;

            return false;
        }
    }


    bool ticketExists(int ticketId) {

        try {
            auto conn =
                DatabaseManager::getInstance().getConnection();

            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id "
                "FROM Tickets "
                "WHERE id = $1;",
                ticketId
            );

            return !r.empty();
        }
        catch (const exception& e) {

            cerr << "Error in TicketRepository::ticketExists: "
                 << e.what() << endl;

            return false;
        }
    }
};