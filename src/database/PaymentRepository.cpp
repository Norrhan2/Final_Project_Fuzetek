#pragma once

#include "../core/Payment.cpp"
#include "DatabaseManager.cpp"
#include <pqxx/pqxx>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
using namespace std;

class PgWalletLedger : public IWalletLedger {
    pqxx::work& txn;

public:
    explicit PgWalletLedger(pqxx::work& txn) : txn(txn) {}

    
    
    bool debit(int customerId, long long amountCents) override {
        pqxx::result r = txn.exec_params(
            "UPDATE Wallets SET balance = balance - ($2::numeric / 100) "
            "WHERE user_id = $1 AND balance >= ($2::numeric / 100);",
            customerId, amountCents
        );
        return r.affected_rows() == 1;
    }

    void credit(int customerId, long long amountCents) override {
        txn.exec_params(
            "INSERT INTO Wallets (user_id, balance) VALUES ($1, $2::numeric / 100) "
            "ON CONFLICT (user_id) DO UPDATE SET balance = Wallets.balance + EXCLUDED.balance;",
            customerId, amountCents
        );
    }
};


class PgPaymentSession : public IPaymentSession {
    
    decltype(DatabaseManager::getInstance().getConnection()) conn;
    pqxx::work txn;
    PgWalletLedger ledger;

    static PaymentRecord toRecord(const pqxx::row& row) {
        PaymentRecord record;
        record.id = row["id"].as<int>();
        record.bookingId = row["booking_id"].as<int>();
        record.method = stringToMethod(row["method"].as<string>());
        record.amountCents = row["amount_cents"].as<long long>();
        record.status = stringToStatus(row["status"].as<string>());
        return record;
    }

public:
    PgPaymentSession()
        : conn(DatabaseManager::getInstance().getConnection()), txn(*conn), ledger(txn) {}

    optional<PayableBooking> loadBooking(int bookingId) override {
        
        pqxx::result r = txn.exec_params(
            "SELECT b.id, b.customer_id, b.status, t.id AS ticket_id, "
            "       round(t.price::numeric * 100)::bigint AS price_cents "
            "FROM Bookings b JOIN Tickets t ON t.id = b.ticket_id "
            "WHERE b.id = $1 FOR UPDATE OF b;",
            bookingId
        );
        if (r.empty()) return nullopt;

        PayableBooking booking;
        booking.bookingId = r[0]["id"].as<int>();
        booking.customerId = r[0]["customer_id"].as<int>();
        booking.ticketId = r[0]["ticket_id"].as<int>();
        booking.amountCents = r[0]["price_cents"].as<long long>();
        booking.status = r[0]["status"].as<string>();
        return booking;
    }

    optional<PaymentRecord> loadPayment(int bookingId) override {
        pqxx::result r = txn.exec_params(
            "SELECT id, booking_id, method, (amount * 100)::bigint AS amount_cents, status "
            "FROM Payments WHERE booking_id = $1 FOR UPDATE;",
            bookingId
        );
        if (r.empty()) return nullopt;
        return toRecord(r[0]);
    }

    int savePayment(const PaymentRecord& record) override {
        
        
        pqxx::result r = txn.exec_params(
            "INSERT INTO Payments (booking_id, method, amount, status, paid_at) "
            "VALUES ($1, $2, $3::numeric / 100, $4::text, "
            "        CASE WHEN $4::text = 'Completed' THEN now() END) "
            "ON CONFLICT (booking_id) DO UPDATE SET "
            "    method  = EXCLUDED.method, "
            "    amount  = EXCLUDED.amount, "
            "    status  = EXCLUDED.status, "
            "    paid_at = CASE WHEN EXCLUDED.status = 'Completed' THEN now() ELSE Payments.paid_at END "
            "WHERE Payments.status IN ('Pending', 'Failed') "
            "   OR (Payments.status = 'Completed' AND EXCLUDED.status = 'Refunded') "
            "RETURNING id;",
            record.bookingId, methodToString(record.method), record.amountCents, statusToString(record.status)
        );
        if (r.empty()) throw runtime_error("Payment row is final and cannot be overwritten.");
        return r[0][0].as<int>();
    }

    void confirmBooking(int bookingId) override {
        pqxx::result booking = txn.exec_params(
            "UPDATE Bookings SET status = 'Confirmed' WHERE id = $1 AND status = 'Pending';",
            bookingId
        );
        if (booking.affected_rows() != 1) throw runtime_error("Booking could not be confirmed (its state changed).");

        
        pqxx::result ticket = txn.exec_params(
            "UPDATE Tickets SET state = 'Sold' "
            "WHERE id = (SELECT ticket_id FROM Bookings WHERE id = $1) AND state = 'Reserved';",
            bookingId
        );
        if (ticket.affected_rows() != 1) throw runtime_error("Ticket is not Reserved, so it cannot be sold.");
    }

    IWalletLedger& wallet() override { return ledger; }

    void commit() override { txn.commit(); }
};


class PaymentRepository : public IPaymentSessionFactory {
public:
    PaymentRepository() = default;

    unique_ptr<IPaymentSession> begin() override {
        return make_unique<PgPaymentSession>();
    }

    optional<PaymentRecord> findByBooking(int bookingId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT id, booking_id, method, (amount * 100)::bigint AS amount_cents, status "
                "FROM Payments WHERE booking_id = $1;",
                bookingId
            );
            if (r.empty()) return nullopt;

            PaymentRecord record;
            record.id = r[0]["id"].as<int>();
            record.bookingId = r[0]["booking_id"].as<int>();
            record.method = stringToMethod(r[0]["method"].as<string>());
            record.amountCents = r[0]["amount_cents"].as<long long>();
            record.status = stringToStatus(r[0]["status"].as<string>());
            return record;
        }
        catch (const exception& e) {
            cerr << "Error in PaymentRepository::findByBooking: " << e.what() << endl;
            return nullopt;
        }
    }
};


class WalletRepository {
public:
    WalletRepository() = default;

    optional<long long> getBalanceCents(int customerId) {
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            pqxx::result r = txn.exec_params(
                "SELECT (balance * 100)::bigint AS balance_cents FROM Wallets WHERE user_id = $1;",
                customerId
            );
            if (r.empty()) return nullopt;
            return r[0][0].as<long long>();
        }
        catch (const exception& e) {
            cerr << "Error in WalletRepository::getBalanceCents: " << e.what() << endl;
            return nullopt;
        }
    }

    bool topUp(int customerId, long long amountCents) {
        if (amountCents <= 0) return false;
        try {
            auto conn = DatabaseManager::getInstance().getConnection();
            pqxx::work txn(*conn);

            PgWalletLedger(txn).credit(customerId, amountCents);

            txn.commit();
            return true;
        }
        catch (const exception& e) {
            cerr << "Error in WalletRepository::topUp: " << e.what() << endl;
            return false;
        }
    }
};

class PaymentModule {
    PaymentRepository repository;
    PaymentService paymentService;

public:
    PaymentModule() : repository(), paymentService(repository) {}

    PaymentModule(const PaymentModule&) = delete;
    PaymentModule& operator=(const PaymentModule&) = delete;

    PaymentService& service() { return paymentService; }
    PaymentRepository& payments() { return repository; }
};