#pragma once

#include <string>
#include <memory>

using namespace std;

class Ticket;


class TicketState {
public:
    virtual ~TicketState() = default;

    virtual string getName() const = 0;

    virtual bool reserve(Ticket& ticket) = 0;

    virtual bool sell(Ticket& ticket) = 0;

    virtual bool cancel(Ticket& ticket) = 0;
};


class AvailableState : public TicketState {
public:
    string getName() const override {
        return "Available";
    }

    bool reserve(Ticket& ticket) override;

    bool sell(Ticket& ticket) override;

    bool cancel(Ticket& ticket) override;
};


class ReservedState : public TicketState {
public:
    string getName() const override {
        return "Reserved";
    }

    bool reserve(Ticket& ticket) override;

    bool sell(Ticket& ticket) override;

    bool cancel(Ticket& ticket) override;
};


class SoldState : public TicketState {
public:
    string getName() const override {
        return "Sold";
    }

    bool reserve(Ticket& ticket) override;

    bool sell(Ticket& ticket) override;

    bool cancel(Ticket& ticket) override;
};


class CancelledState : public TicketState {
public:
    string getName() const override {
        return "Cancelled";
    }

    bool reserve(Ticket& ticket) override;

    bool sell(Ticket& ticket) override;

    bool cancel(Ticket& ticket) override;
};


bool AvailableState::reserve(Ticket& ticket) {
    ticket.setState(make_shared<ReservedState>());
    return true;
}


bool AvailableState::sell(Ticket& ticket) {
    return false;
}


bool AvailableState::cancel(Ticket& ticket) {
    return false;
}


bool ReservedState::reserve(Ticket& ticket) {
    return false;
}


bool ReservedState::sell(Ticket& ticket) {
    ticket.setState(make_shared<SoldState>());
    return true;
}


bool ReservedState::cancel(Ticket& ticket) {
    ticket.setState(make_shared<CancelledState>());
    return true;
}


bool SoldState::reserve(Ticket& ticket) {
    return false;
}


bool SoldState::sell(Ticket& ticket) {
    return false;
}


bool SoldState::cancel(Ticket& ticket) {
    ticket.setState(make_shared<CancelledState>());
    return true;
}


bool CancelledState::reserve(Ticket& ticket) {
    return false;
}


bool CancelledState::sell(Ticket& ticket) {
    return false;
}


bool CancelledState::cancel(Ticket& ticket) {
    return false;
}