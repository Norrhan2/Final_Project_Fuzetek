#pragma once
#include "../core/Event.h"
#include <vector>
#include <optional>

class EventRepository
{
public:
    static bool createEvent(Event &event);
    static bool updateEvent(const Event &event);
    static bool deleteEvent(int eventId);
    static std::optional<Event> getEventById(int eventId);
    static std::vector<Event> getEventsByOrganizer(int organizerId);
    static std::vector<Event> searchEvents(const std::string &queryTitle);
};