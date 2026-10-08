#include <chrono>

namespace FinalProject {
    
class Attendee {
private:
    int serial;
    int customer_id;
    int booking_id;
    std::chrono::system_clock::time_point timestamp;

    
public:
    
    Attendee(int serial, int customerId, int bookingId, std::chrono::system_clock::time_point at)
        : serial(serial), customer_id(customerId), booking_id(bookingId), timestamp(at) {
    }
    int getSerial() {
        return serial;
    }
    
    int getCustomer() {
        return serial;
    }
    
    int getBookingId() {
        return booking_id;
    }
    
    std::chrono::system_clock::time_point getTimeStamp() {
        return timestamp;
    }
};
    
    
    
    
}