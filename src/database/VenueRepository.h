#pragma once
#include "../core/Venue.h"
#include <vector>
#include <optional>

class VenueRepository
{
public:
    static bool createVenue(Venue &venue);
    static bool updateVenue(const Venue &venue);
    static bool deleteVenue(int venueId);
    static std::optional<Venue> getVenueById(int venueId);
    static std::vector<Venue> getAllVenues();
};