#pragma once

#include <memory>
#include <string>
using namespace std;

class Ticket;

// State pattern: every state decides which transitions are legal.
//   Available --reserve--> Reserved --sell--> Sold
//   Reserved/Sold --cancel-->  Cancelled   (ticket is voided)
//   Reserved/Sold --release--> Available   (booking cancelled, seat goes back on sale)
class TicketState {
public:
    virtual ~TicketState() = default;
    virtual string getName() const = 0;
    virtual bool reserve(Ticket& ticket) = 0;
    virtual bool sell(Ticket& ticket) = 0;
    virtual bool cancel(Ticket& ticket) = 0;
    virtual bool release(Ticket& ticket) = 0;
};

class AvailableState : public TicketState {
public:
    string getName() const override { return "Available"; }
    bool reserve(Ticket& ticket) override;
    bool sell(Ticket&) override { return false; }
    bool cancel(Ticket&) override { return false; }
    bool release(Ticket&) override { return false; }
};

class ReservedState : public TicketState {
public:
    string getName() const override { return "Reserved"; }
    bool reserve(Ticket&) override { return false; }
    bool sell(Ticket& ticket) override;
    bool cancel(Ticket& ticket) override;
    bool release(Ticket& ticket) override;
};

class SoldState : public TicketState {
public:
    string getName() const override { return "Sold"; }
    bool reserve(Ticket&) override { return false; }
    bool sell(Ticket&) override { return false; }
    bool cancel(Ticket& ticket) override;
    bool release(Ticket& ticket) override;
};

class CancelledState : public TicketState {
public:
    string getName() const override { return "Cancelled"; }
    bool reserve(Ticket&) override { return false; }
    bool sell(Ticket&) override { return false; }
    bool cancel(Ticket&) override { return false; }
    bool release(Ticket&) override { return false; }
};

// Bodies need the complete Ticket type, so they live here (this file is included at the bottom of Ticket.cpp).
inline bool AvailableState::reserve(Ticket& ticket) { ticket.setState(make_shared<ReservedState>()); return true; }

inline bool ReservedState::sell(Ticket& ticket)    { ticket.setState(make_shared<SoldState>()); return true; }
inline bool ReservedState::cancel(Ticket& ticket)  { ticket.setState(make_shared<CancelledState>()); return true; }
inline bool ReservedState::release(Ticket& ticket) { ticket.setState(make_shared<AvailableState>()); return true; }

inline bool SoldState::cancel(Ticket& ticket)      { ticket.setState(make_shared<CancelledState>()); return true; }
inline bool SoldState::release(Ticket& ticket)     { ticket.setState(make_shared<AvailableState>()); return true; }