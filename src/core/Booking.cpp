#pragma once

#include <string>
using namespace std;

enum class BookingStatus {
    Pending,
    Confirmed,
    Cancelled
};

class Booking {
private:
    int id;
    int customerId;
    int eventId;
    int ticketId;
    BookingStatus status;
    string createdAt;

public:
    Booking(int id, int customerId, int eventId, int ticketId, BookingStatus status, string createdAt = "")
        : id(id), customerId(customerId), eventId(eventId), ticketId(ticketId), status(status), createdAt(createdAt) {
    }

    int getId() const { return id; }
    int getCustomerId() const { return customerId; }
    int getEventId() const { return eventId; }
    int getTicketId() const { return ticketId; }
    BookingStatus getStatus() const { return status; }
    string getCreatedAt() const { return createdAt; }

    string getStatusString() const {
        return statusToString(status);
    }

    bool canCancel() const {
        return status != BookingStatus::Cancelled;
    }

    void markAsConfirmed() {
        status = BookingStatus::Confirmed;
    }

    void markAsCancelled() {
        status = BookingStatus::Cancelled;
    }

    static string statusToString(BookingStatus status) {
        switch (status) {
        case BookingStatus::Pending:   return "Pending";
        case BookingStatus::Confirmed: return "Confirmed";
        case BookingStatus::Cancelled: return "Cancelled";
        default:                       return "Unknown";
        }
    }

    static BookingStatus stringToStatus(const string& statusStr) {
        if (statusStr == "Confirmed") return BookingStatus::Confirmed;
        if (statusStr == "Cancelled") return BookingStatus::Cancelled;
        return BookingStatus::Pending;
    }
};