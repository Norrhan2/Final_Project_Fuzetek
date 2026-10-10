#pragma once
#include <string>
#include <utility>

class Event
{
private:
    int id{0};
    int organizerId{0};
    int venueId{0};
    std::string title;
    std::string date; // ISO Format: YYYY-MM-DD HH:MM
    int capacity{0};

public:
    Event() = default;
    Event(int id, int organizerId, int venueId, std::string title, std::string date, int capacity)
        : id(id), organizerId(organizerId), venueId(venueId),
          title(std::move(title)), date(std::move(date)), capacity(capacity) {}

    // Getters
    int getId() const { return id; }
    int getOrganizerId() const { return organizerId; }
    int getVenueId() const { return venueId; }
    const std::string &getTitle() const { return title; }
    const std::string &getDate() const { return date; }
    int getCapacity() const { return capacity; }

    // Setters
    void setId(int newId) { id = newId; }
    void setOrganizerId(int id) { organizerId = id; }
    void setVenueId(int id) { venueId = id; }
    void setTitle(const std::string &newTitle) { title = newTitle; }
    void setDate(const std::string &newDate) { date = newDate; }
    void setCapacity(int newCapacity) { capacity = newCapacity; }
};