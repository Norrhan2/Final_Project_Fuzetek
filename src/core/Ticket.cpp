 #pragma once

#include <string>
#include <memory>
#include <iostream>

using namespace std;

class TicketState;

class Ticket {
private:
    int id;
    int eventId;
    string type;
    double price;
    shared_ptr<TicketState> state;

public:
    Ticket(int id, int eventId, const string& type, double price);

    Ticket(int id,
           int eventId,
           const string& type,
           double price,
           const string& stateName);

    virtual ~Ticket() = default;

    int getId() const {
        return id;
    }

    int getEventId() const {
        return eventId;
    }

    string getType() const {
        return type;
    }

    double getPrice() const {
        return price;
    }

    string getState() const;

    bool reserve();
    bool sell();
    bool cancel();

    void setState(shared_ptr<TicketState> newState);

    void setStateByName(const string& stateName);

    virtual string getTicketType() const {
        return type;
    }

    virtual void display() const {
        cout << "Ticket ID: " << id << endl;
        cout << "Event ID: " << eventId << endl;
        cout << "Type: " << type << endl;
        cout << "Price: " << price << endl;
        cout << "State: " << getState() << endl;
    }
};


class RegularTicket : public Ticket {
public:
    RegularTicket(int id, int eventId, double price)
        : Ticket(id, eventId, "Regular", price) {
    }

    RegularTicket(int id, int eventId, double price, const string& state)
        : Ticket(id, eventId, "Regular", price, state) {
    }

    string getTicketType() const override {
        return "Regular";
    }
};


class VIPTicket : public Ticket {
public:
    VIPTicket(int id, int eventId, double price)
        : Ticket(id, eventId, "VIP", price) {
    }

    VIPTicket(int id, int eventId, double price, const string& state)
        : Ticket(id, eventId, "VIP", price, state) {
    }

    string getTicketType() const override {
        return "VIP";
    }
};


class StudentTicket : public Ticket {
public:
    StudentTicket(int id, int eventId, double price)
        : Ticket(id, eventId, "Student", price) {
    }

    StudentTicket(int id, int eventId, double price, const string& state)
        : Ticket(id, eventId, "Student", price, state) {
    }

    string getTicketType() const override {
        return "Student";
    }
};


class TicketFactory {
public:
    static unique_ptr<Ticket> createTicket(
        const string& type,
        int id,
        int eventId,
        double price
    ) {
        if (type == "Regular") {
            return make_unique<RegularTicket>(id, eventId, price);
        }

        if (type == "VIP") {
            return make_unique<VIPTicket>(id, eventId, price);
        }

        if (type == "Student") {
            return make_unique<StudentTicket>(id, eventId, price);
        }

        return nullptr;
    }

    static unique_ptr<Ticket> createTicket(
        const string& type,
        int id,
        int eventId,
        double price,
        const string& state
    ) {
        if (type == "Regular") {
            return make_unique<RegularTicket>(
                id, eventId, price, state
            );
        }

        if (type == "VIP") {
            return make_unique<VIPTicket>(
                id, eventId, price, state
            );
        }

        if (type == "Student") {
            return make_unique<StudentTicket>(
                id, eventId, price, state
            );
        }

        return nullptr;
    }
};


// TicketState is included here because main.cpp is intended
// to compile the project as one translation unit.
#include "TicketState.cpp"


Ticket::Ticket(int id,
               int eventId,
               const string& type,
               double price)
    : id(id),
      eventId(eventId),
      type(type),
      price(price),
      state(make_shared<AvailableState>()) {
}


Ticket::Ticket(int id,
               int eventId,
               const string& type,
               double price,
               const string& stateName)
    : id(id),
      eventId(eventId),
      type(type),
      price(price) {
    setStateByName(stateName);
}


string Ticket::getState() const {
    return state ? state->getName() : "Unknown";
}


void Ticket::setState(shared_ptr<TicketState> newState) {
    if (newState) {
        state = newState;
    }
}


void Ticket::setStateByName(const string& stateName) {
    if (stateName == "Available") {
        state = make_shared<AvailableState>();
    }
    else if (stateName == "Reserved") {
        state = make_shared<ReservedState>();
    }
    else if (stateName == "Sold") {
        state = make_shared<SoldState>();
    }
    else if (stateName == "Cancelled") {
        state = make_shared<CancelledState>();
    }
    else {
        state = make_shared<AvailableState>();
    }
}


bool Ticket::reserve() {
    return state && state->reserve(*this);
}


bool Ticket::sell() {
    return state && state->sell(*this);
}


bool Ticket::cancel() {
    return state && state->cancel(*this);
}