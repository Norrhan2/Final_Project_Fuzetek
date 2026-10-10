#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
using namespace std;

// Singleton: knows HOW to reach PostgreSQL, hands out connections.
// Every call to getConnection() returns an independent connection, so a
// long-running transaction (e.g. PgPaymentSession) never blocks other screens.
class DatabaseManager {
private:
    string connectionInfo;
    mutex mtx;

    DatabaseManager() {
        // Override without recompiling:  set EVENT_DB_CONNINFO="host=... dbname=... user=... password=..."
        const char* env = std::getenv("EVENT_DB_CONNINFO");
        connectionInfo = (env && *env)
            ? string(env)
            : "host=localhost port=5432 dbname=event_ticket_db user=postgres password=postgres";
    }

public:
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    void setConnectionInfo(const string& info) {
        lock_guard<mutex> lock(mtx);
        connectionInfo = info;
    }

    // Throws pqxx::broken_connection if PostgreSQL cannot be reached
    // (every repository already catches std::exception).
    shared_ptr<pqxx::connection> getConnection() {
        string info;
        {
            lock_guard<mutex> lock(mtx);
            info = connectionInfo;
        }
        return make_shared<pqxx::connection>(info);
    }

    bool testConnection(string* error = nullptr) {
        try {
            auto conn = getConnection();
            pqxx::nontransaction txn(*conn);
            txn.exec("SELECT 1;");
            return true;
        }
        catch (const exception& e) {
            if (error) *error = e.what();
            cerr << "Database connection failed: " << e.what() << endl;
            return false;
        }
    }
};