#pragma once

#include "Payment.cpp"   // Money helper (cents -> text)
#include <cstdio>
#include <string>
#include <vector>
using namespace std;

// Whole-system (or one organizer's) totals.
struct SalesSummary {
    long long revenueCents = 0;     // payments with status Completed
    long long refundedCents = 0;    // payments with status Refunded

    int ticketsAvailable = 0;
    int ticketsReserved = 0;
    int ticketsSold = 0;
    int ticketsCancelled = 0;

    int bookingsPending = 0;
    int bookingsConfirmed = 0;
    int bookingsCancelled = 0;

    int checkedIn = 0;              // rows in Attendees

    int ticketsTotal() const {
        return ticketsAvailable + ticketsReserved + ticketsSold + ticketsCancelled;
    }
    int bookingsTotal() const {
        return bookingsPending + bookingsConfirmed + bookingsCancelled;
    }
};

// One row per event.
struct EventSales {
    int eventId = 0;
    string title;
    string date;
    int capacity = 0;
    int ticketsSold = 0;
    int ticketsTotal = 0;
    long long revenueCents = 0;
    int checkedIn = 0;
};

// Used for "by ticket type" and "by payment method".
struct BreakdownRow {
    string label;
    int count = 0;
    long long amountCents = 0;
};

struct SalesReport {
    SalesSummary summary;
    vector<EventSales> events;
    vector<BreakdownRow> byTicketType;
    vector<BreakdownRow> byPaymentMethod;
};

// Pure calculations (no database) so they are easy to unit test.
struct SalesStatistics {
    static double percent(long long part, long long whole) {
        if (whole <= 0) return 0.0;
        return 100.0 * static_cast<double>(part) / static_cast<double>(whole);
    }

    static string formatPercent(double value) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f%%", value);
        return buffer;
    }

    static double cancellationRate(const SalesSummary& summary) {
        return percent(summary.bookingsCancelled, summary.bookingsTotal());
    }

    static double attendanceRate(const SalesSummary& summary) {
        return percent(summary.checkedIn, summary.ticketsSold);
    }

    static double soldVsCapacity(const EventSales& event) {
        return percent(event.ticketsSold, event.capacity);
    }
};
