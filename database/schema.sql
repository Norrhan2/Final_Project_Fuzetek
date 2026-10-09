CREATE TABLE Attendees (
                           id            SERIAL PRIMARY KEY,
                           booking_id    INTEGER NOT NULL UNIQUE REFERENCES Bookings(id) ON DELETE CASCADE,
                           customer_id   INTEGER NOT NULL UNIQUE REFERENCES User(id) ON DELETE CASCADE,
                           checked_in_at TIMESTAMP NOT NULL DEFAULT now()
);