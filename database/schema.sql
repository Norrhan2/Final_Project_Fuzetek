CREATE TABLE Users (
    id       SERIAL PRIMARY KEY,
    name     VARCHAR(100) NOT NULL,
    email    VARCHAR(150) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    role     VARCHAR(20)  NOT NULL CHECK (role IN ('Admin','Organizer','Customer'))
);

CREATE TABLE Venues (
    id       SERIAL PRIMARY KEY,
    name     VARCHAR(150) NOT NULL,
    address  VARCHAR(255) NOT NULL,
    capacity INTEGER NOT NULL CHECK (capacity > 0)
);

CREATE TABLE Events (
    id           SERIAL PRIMARY KEY,
    organizer_id INTEGER NOT NULL REFERENCES Users(id),
    venue_id     INTEGER NOT NULL REFERENCES Venues(id),
    title        VARCHAR(200) NOT NULL,
    date         TIMESTAMP NOT NULL,
    capacity     INTEGER NOT NULL CHECK (capacity > 0)
);

CREATE TABLE Tickets (
    id       SERIAL PRIMARY KEY,
    event_id INTEGER NOT NULL REFERENCES Events(id) ON DELETE CASCADE,
    type     VARCHAR(20) NOT NULL CHECK (type IN ('Regular','VIP','Student')),
    price    NUMERIC(10,2) NOT NULL CHECK (price >= 0),
    state    VARCHAR(20) NOT NULL DEFAULT 'Available'
             CHECK (state IN ('Available','Reserved','Sold','Cancelled'))
);

CREATE TABLE Bookings (
    id          SERIAL PRIMARY KEY,
    customer_id INTEGER NOT NULL REFERENCES Users(id),
    event_id    INTEGER NOT NULL REFERENCES Events(id),
    ticket_id   INTEGER NOT NULL REFERENCES Tickets(id),
    status      VARCHAR(20) NOT NULL DEFAULT 'Pending'
                CHECK (status IN ('Pending','Confirmed','Cancelled')),
    created_at  TIMESTAMP NOT NULL DEFAULT now()
);
-- a ticket can have only one non-cancelled booking
CREATE UNIQUE INDEX one_active_booking_per_ticket
    ON Bookings(ticket_id) WHERE status <> 'Cancelled';

CREATE TABLE Payments (
    id         SERIAL PRIMARY KEY,
    booking_id INTEGER NOT NULL UNIQUE REFERENCES Bookings(id) ON DELETE CASCADE,
    method     VARCHAR(20) NOT NULL CHECK (method IN ('Cash','Card','Wallet')),
    amount     NUMERIC(10,2) NOT NULL CHECK (amount > 0),
    status     VARCHAR(20) NOT NULL DEFAULT 'Pending'
               CHECK (status IN ('Pending','Completed','Failed','Refunded')),
    paid_at    TIMESTAMP
);

CREATE TABLE Wallets (
    user_id INTEGER PRIMARY KEY REFERENCES Users(id) ON DELETE CASCADE,
    balance NUMERIC(12,2) NOT NULL DEFAULT 0 CHECK (balance >= 0)
);

CREATE TABLE Attendees (
    id            SERIAL PRIMARY KEY,
    booking_id    INTEGER NOT NULL UNIQUE REFERENCES Bookings(id) ON DELETE CASCADE,
    customer_id   INTEGER NOT NULL REFERENCES Users(id) ON DELETE CASCADE,
    checked_in_at TIMESTAMP NOT NULL DEFAULT now()
);