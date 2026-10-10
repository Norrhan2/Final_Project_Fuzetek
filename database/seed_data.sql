-- Demo accounts:
--   admin@eventhub.com     / Admin@123
--   organizer@eventhub.com / Organizer@123
--   customer@eventhub.com  / Customer@123

INSERT INTO Users (name, email, password, role) VALUES
('System Admin',     'admin@eventhub.com',     'a1b2c3d4e5f60718293a4b5c6d7e8f90$e4e22835de8ed2ebed3a1a08e7cddd8c51c9b3157211f11d69e3bab2f4a1000d', 'Admin'),
('Olivia Organizer', 'organizer@eventhub.com', '0f1e2d3c4b5a69788796a5b4c3d2e1f0$6d7e54c714f8ad41a0bbdb08f4cebbd87098b5f81e16f3e7c45c029d29ae7a69', 'Organizer'),
('Carl Customer',    'customer@eventhub.com',  '1234567890abcdef1234567890abcdef$25fc8f9382225470940452c93ec6acd8003265607d513e5d7968cf6800c8dec7', 'Customer');

INSERT INTO Wallets (user_id, balance)
SELECT id, 500.00 FROM Users WHERE email = 'customer@eventhub.com';

INSERT INTO Venues (name, address, capacity) VALUES
('Grand Hall',               'Downtown, Cairo',        800),
('Smart Conference Center',  'Smart Village, Giza',    300);

INSERT INTO Events (organizer_id, venue_id, title, date, capacity)
SELECT u.id, v.id, e.title, e.date::timestamp, e.capacity
FROM (VALUES
    ('Tech Summit 2026',  'Smart Conference Center', '2026-11-20 10:00', 250),
    ('Jazz Night',        'Grand Hall',              '2026-12-05 20:00', 700),
    ('Startup Pitch Day', 'Smart Conference Center', '2026-12-18 09:00', 150)
) AS e(title, venue, date, capacity)
JOIN Venues v ON v.name = e.venue
JOIN Users  u ON u.email = 'organizer@eventhub.com';

INSERT INTO Tickets (event_id, type, price)
SELECT e.id, t.type, t.price
FROM (VALUES
    ('Tech Summit 2026',  'Regular', 150.00),
    ('Tech Summit 2026',  'Regular', 150.00),
    ('Tech Summit 2026',  'VIP',     400.00),
    ('Tech Summit 2026',  'Student',  75.00),
    ('Jazz Night',        'Regular', 200.00),
    ('Jazz Night',        'Regular', 200.00),
    ('Jazz Night',        'VIP',     500.00),
    ('Startup Pitch Day', 'Regular', 100.00),
    ('Startup Pitch Day', 'Student',  50.00)
) AS t(title, type, price)
JOIN Events e ON e.title = t.title;

-- One completed sale so the dashboard has something to show:
-- the customer has bought the first Regular ticket of Tech Summit (paid by card).
WITH pick AS (
    SELECT t.id AS ticket_id, t.event_id, t.price
    FROM Tickets t JOIN Events e ON e.id = t.event_id
    WHERE e.title = 'Tech Summit 2026' AND t.type = 'Regular'
    ORDER BY t.id LIMIT 1
),
sold AS (
    UPDATE Tickets SET state = 'Sold'
    WHERE id = (SELECT ticket_id FROM pick)
    RETURNING id
),
bk AS (
    INSERT INTO Bookings (customer_id, event_id, ticket_id, status)
    SELECT u.id, p.event_id, p.ticket_id, 'Confirmed'
    FROM pick p, Users u WHERE u.email = 'customer@eventhub.com'
    RETURNING id
)
INSERT INTO Payments (booking_id, method, amount, status, paid_at)
SELECT bk.id, 'Card', p.price, 'Completed', now() FROM bk, pick p;