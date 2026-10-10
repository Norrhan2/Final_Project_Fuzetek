#pragma once

#include <string>

// One row per checked-in booking.
class Attendee {
private:
    int id;
    int bookingId;
    int customerId;
    std::string checkedInAt;   // "YYYY-MM-DD HH:MM"

public:
    Attendee(int id, int bookingId, int customerId, std::string checkedInAt)
        : id(id), bookingId(bookingId), customerId(customerId), checkedInAt(std::move(checkedInAt)) {}

    int getId() const { return id; }
    int getBookingId() const { return bookingId; }
    int getCustomerId() const { return customerId; }
    const std::string& getCheckedInAt() const { return checkedInAt; }
};