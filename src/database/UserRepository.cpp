#pragma once

#include "../core/User.cpp"
#include "DatabaseManager.cpp"
#include <QByteArray>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QString>
#include <pqxx/pqxx>
#include <iostream>
#include <vector>
using namespace std;

struct AuthResult {
    unique_ptr<User> user;
    string error;
};

class UserRepository {
private:
    // ---- password hashing: salted SHA-256, 10,000 rounds. Stored as "salt$hexdigest". ----
    static string newSalt() {
        QByteArray bytes(16, 0);
        for (int i = 0; i < bytes.size(); ++i)
            bytes[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
        return bytes.toHex().toStdString();
    }

    static string hashPassword(const string& password, const string& saltHex) {
        QByteArray data = QByteArray::fromStdString(saltHex + password);
        QByteArray digest = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
        for (int i = 1; i < 10000; ++i)
            digest = QCryptographicHash::hash(digest + data, QCryptographicHash::Sha256);
        return saltHex + "$" + digest.toHex().toStdString();
    }

    static bool verifyPassword(const string& password, const string& stored) {
        size_t pos = stored.find('$');
        if (pos == string::npos) return false;
        string recomputed = hashPassword(password, stored.substr(0, pos));
        if (recomputed.size() != stored.size()) return false;
        unsigned char diff = 0;                       // constant-time comparison
        for (size_t i = 0; i < stored.size(); ++i)
            diff |= static_cast<unsigned char>(recomputed[i] ^ stored[i]);
        return diff == 0;
    }

    static string normalizeEmail(const string& email) {
        return QString::fromStdString(email).trimmed().toLower().toStdString();
    }

public:
    UserRepository() = default;

    // Returns the new user id, or -1 (and fills *error) on failure.
    // Customers automatically get an empty wallet row.
    int registerUser(const string& name, const string& email, const string& password,
                     const string& role, string* error = nullptr) {
        auto fail = [&](const string& message) {
            if (error) *error = message;
            return -1;
        };
        if (!Roles::isValid(role)) return fail("Unknown role.");

        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "INSERT INTO Users (name, email, password, role) VALUES ($1, $2, $3, $4) RETURNING id;",
                name, normalizeEmail(email), hashPassword(password, newSalt()), role
            );
            int id = r[0][0].as<int>();

            if (role == Roles::CUSTOMER) {
                txn.exec_params(
                    "INSERT INTO Wallets (user_id, balance) VALUES ($1, 0) ON CONFLICT (user_id) DO NOTHING;",
                    id
                );
            }

            txn.commit();
            return id;
        }
        catch (const pqxx::unique_violation&) {
            return fail("This email is already registered.");
        }
        catch (const exception& e) {
            cerr << "Error in UserRepository::registerUser: " << e.what() << endl;
            return fail("Could not create the account. Please check the database connection.");
        }
    }

    AuthResult authenticate(const string& email, const string& password) {
        AuthResult result;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, name, email, password, role FROM Users WHERE email = $1;",
                normalizeEmail(email)
            );

            // Same message for "no such email" and "wrong password" (no account enumeration).
            if (r.empty() || !verifyPassword(password, r[0]["password"].as<string>())) {
                result.error = "Invalid email or password.";
                return result;
            }

            result.user = UserFactory::create(
                r[0]["id"].as<int>(),
                r[0]["name"].as<string>(),
                r[0]["email"].as<string>(),
                r[0]["role"].as<string>()
            );
        }
        catch (const exception& e) {
            cerr << "Error in UserRepository::authenticate: " << e.what() << endl;
            result.error = "Could not reach the database. Please try again.";
        }
        return result;
    }

    vector<unique_ptr<User>> getAllUsers() {
        vector<unique_ptr<User>> users;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::nontransaction txn(*conn);

            pqxx::result r = txn.exec("SELECT id, name, email, role FROM Users ORDER BY id;");
            for (const auto& row : r) {
                users.push_back(UserFactory::create(
                    row["id"].as<int>(),
                    row["name"].as<string>(),
                    row["email"].as<string>(),
                    row["role"].as<string>()
                ));
            }
        }
        catch (const exception& e) {
            cerr << "Error in UserRepository::getAllUsers: " << e.what() << endl;
        }
        return users;
    }

    bool deleteUser(int userId, string* error = nullptr) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);
            txn.exec_params("DELETE FROM Users WHERE id = $1;", userId);
            txn.commit();
            return true;
        }
        catch (const pqxx::foreign_key_violation&) {
            if (error) *error = "This user still has events or bookings and cannot be deleted.";
        }
        catch (const exception& e) {
            cerr << "Error in UserRepository::deleteUser: " << e.what() << endl;
            if (error) *error = "Could not delete the user.";
        }
        return false;
    }
};