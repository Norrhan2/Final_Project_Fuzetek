#pragma once

#include <memory>
#include <stdexcept>
#include <string>
using namespace std;

struct Roles {
    static constexpr const char* ADMIN     = "Admin";
    static constexpr const char* ORGANIZER = "Organizer";
    static constexpr const char* CUSTOMER  = "Customer";

    static bool isValid(const string& role) {
        return role == ADMIN || role == ORGANIZER || role == CUSTOMER;
    }
};

// Abstract base: identity data + what the role is ALLOWED to do.
// The GUI asks "can this user do X?" and never checks role names itself,
// so adding a new role means adding a class, not editing every screen (Open/Closed).
class User {
private:
    int id;
    string name;
    string email;

public:
    User(int id, string name, string email)
        : id(id), name(std::move(name)), email(std::move(email)) {}

    virtual ~User() = default;

    int getId() const { return id; }
    const string& getName() const { return name; }
    const string& getEmail() const { return email; }

    virtual string getRole() const = 0;

    // Permissions: default = no. Subclasses switch on only what they own.
    virtual bool canManageUsers() const { return false; }
    virtual bool canManageEvents() const { return false; }
    virtual bool canManageVenues() const { return false; }
    virtual bool canManageTickets() const { return false; }
    virtual bool canViewStatistics() const { return false; }
    virtual bool canViewAllStatistics() const { return false; }   // all events vs own events
    virtual bool canBook() const { return false; }
    virtual bool canCheckIn() const { return false; }
};

class Admin : public User {
public:
    using User::User;
    string getRole() const override { return Roles::ADMIN; }

    bool canManageUsers() const override { return true; }
    bool canViewStatistics() const override { return true; }
    bool canViewAllStatistics() const override { return true; }
};

class Organizer : public User {
public:
    using User::User;
    string getRole() const override { return Roles::ORGANIZER; }

    bool canManageEvents() const override { return true; }
    bool canManageVenues() const override { return true; }
    bool canManageTickets() const override { return true; }
    bool canViewStatistics() const override { return true; }   // own events only
};

class Customer : public User {
public:
    using User::User;
    string getRole() const override { return Roles::CUSTOMER; }

    bool canBook() const override { return true; }
    bool canCheckIn() const override { return true; }
};

// Factory: turns the role string stored in the Users table into the right subclass.
class UserFactory {
public:
    static unique_ptr<User> create(int id, const string& name, const string& email, const string& role) {
        if (role == Roles::ADMIN)     return make_unique<Admin>(id, name, email);
        if (role == Roles::ORGANIZER) return make_unique<Organizer>(id, name, email);
        if (role == Roles::CUSTOMER)  return make_unique<Customer>(id, name, email);
        throw invalid_argument("Unknown role: " + role);
    }
};