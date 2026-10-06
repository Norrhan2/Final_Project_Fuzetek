#pragma once
#include <cctype>
#include <cmath>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
using namespace std;


struct Money {
    static long long fromDouble(double amount) { return llround(amount * 100.0); }

    static string toString(long long cents) {
        long long absolute = cents < 0 ? -cents : cents;
        long long fraction = absolute % 100;
        string text = to_string(absolute / 100) + "." + (fraction < 10 ? "0" : "") + to_string(fraction);
        return cents < 0 ? "-" + text : text;
    }
};

enum class PaymentMethod { Cash, Card, Wallet };
enum class PaymentStatus { Pending, Completed, Failed, Refunded };

inline string methodToString(PaymentMethod method) {
    switch (method) {
    case PaymentMethod::Cash:   return "Cash";
    case PaymentMethod::Card:   return "Card";
    case PaymentMethod::Wallet: return "Wallet";
    }
    return "Unknown";
}

inline PaymentMethod stringToMethod(const string& text) {
    if (text == "Cash")   return PaymentMethod::Cash;
    if (text == "Card")   return PaymentMethod::Card;
    if (text == "Wallet") return PaymentMethod::Wallet;
    throw invalid_argument("Unknown payment method: " + text);
}

inline string statusToString(PaymentStatus status) {
    switch (status) {
    case PaymentStatus::Pending:   return "Pending";
    case PaymentStatus::Completed: return "Completed";
    case PaymentStatus::Failed:    return "Failed";
    case PaymentStatus::Refunded:  return "Refunded";
    }
    return "Unknown";
}

inline PaymentStatus stringToStatus(const string& text) {
    if (text == "Pending")   return PaymentStatus::Pending;
    if (text == "Completed") return PaymentStatus::Completed;
    if (text == "Failed")    return PaymentStatus::Failed;
    if (text == "Refunded")  return PaymentStatus::Refunded;
    throw invalid_argument("Unknown payment status: " + text);
}

struct PaymentRequest {
    int bookingId = 0;
    int customerId = 0;
    long long amountCents = 0;
};

struct PaymentResult {
    bool success = false;
    string message;

    static PaymentResult ok(string message) { return { true, move(message) }; }
    static PaymentResult fail(string message) { return { false, move(message) }; }
};



struct CardDetails {
    string number;   
    string expiry;   
    string cvv;      
};


struct PaymentRecord {
    int id = 0;
    int bookingId = 0;
    PaymentMethod method = PaymentMethod::Cash;
    long long amountCents = 0;
    PaymentStatus status = PaymentStatus::Pending;
};


class IWalletLedger {
public:
    virtual ~IWalletLedger() = default;
    
    virtual bool debit(int customerId, long long amountCents) = 0;
    virtual void credit(int customerId, long long amountCents) = 0;
};


class PaymentStrategy {
public:
    virtual ~PaymentStrategy() = default;

    virtual PaymentMethod method() const = 0;

    PaymentResult pay(const PaymentRequest& request) {
        if (request.amountCents <= 0) return PaymentResult::fail("Amount must be greater than zero.");
        return doPay(request);
    }

    PaymentResult refund(const PaymentRequest& request) {
        if (request.amountCents <= 0) return PaymentResult::fail("Amount must be greater than zero.");
        return doRefund(request);
    }

protected:
    virtual PaymentResult doPay(const PaymentRequest& request) = 0;
    virtual PaymentResult doRefund(const PaymentRequest& request) = 0;
};


class CashPayment : public PaymentStrategy {
public:
    PaymentMethod method() const override { return PaymentMethod::Cash; }

protected:
    
    PaymentResult doPay(const PaymentRequest& request) override {
        return PaymentResult::ok("Cash payment of " + Money::toString(request.amountCents) + " recorded.");
    }
    PaymentResult doRefund(const PaymentRequest& request) override {
        return PaymentResult::ok("Refund of " + Money::toString(request.amountCents) + " to be handed back in cash.");
    }
};


class CardPayment : public PaymentStrategy {
    CardDetails card;

public:
    explicit CardPayment(CardDetails card = CardDetails{}) : card(move(card)) {}

    PaymentMethod method() const override { return PaymentMethod::Card; }

    
    static string digitsOnly(const string& text) {
        string digits;
        for (char c : text) {
            if (c == ' ' || c == '-') continue;
            if (!isdigit(static_cast<unsigned char>(c))) return "";
            digits += c;
        }
        return digits;
    }

    static bool luhnValid(const string& digits) {
        if (digits.size() < 13 || digits.size() > 19) return false;
        int sum = 0;
        bool doubleIt = false;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
            int d = *it - '0';
            if (doubleIt) {
                d *= 2;
                if (d > 9) d -= 9;
            }
            sum += d;
            doubleIt = !doubleIt;
        }
        return sum % 10 == 0;
    }

    
    static bool expiryValid(const string& expiry, int nowYear, int nowMonth) {
        if (expiry.size() != 5 || expiry[2] != '/') return false;
        for (int i : { 0, 1, 3, 4 })
            if (!isdigit(static_cast<unsigned char>(expiry[i]))) return false;
        int month = (expiry[0] - '0') * 10 + (expiry[1] - '0');
        int year = 2000 + (expiry[3] - '0') * 10 + (expiry[4] - '0');
        if (month < 1 || month > 12) return false;
        return year > nowYear || (year == nowYear && month >= nowMonth);
    }

    static bool cvvValid(const string& cvv) {
        if (cvv.size() < 3 || cvv.size() > 4) return false;
        for (char c : cvv)
            if (!isdigit(static_cast<unsigned char>(c))) return false;
        return true;
    }

protected:
    PaymentResult doPay(const PaymentRequest& request) override {
        string digits = digitsOnly(card.number);
        if (!luhnValid(digits)) return PaymentResult::fail("Invalid card number.");

        time_t now = time(nullptr);
        tm local{};
#ifdef _WIN32
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        if (!expiryValid(card.expiry, local.tm_year + 1900, local.tm_mon + 1))
            return PaymentResult::fail("Card is expired or the expiry date is invalid (use MM/YY).");
        if (!cvvValid(card.cvv)) return PaymentResult::fail("Invalid security code (CVV).");

        
        
        if (digits == "4000000000000002") return PaymentResult::fail("Card declined by the issuer.");

        string last4 = digits.substr(digits.size() - 4);
        return PaymentResult::ok("Card ending " + last4 + " approved for " + Money::toString(request.amountCents) + ".");
    }

    
    PaymentResult doRefund(const PaymentRequest& request) override {
        return PaymentResult::ok("Refund of " + Money::toString(request.amountCents) + " sent back to the original card.");
    }
};


class WalletPayment : public PaymentStrategy {
    IWalletLedger& ledger;

public:
    explicit WalletPayment(IWalletLedger& ledger) : ledger(ledger) {}

    PaymentMethod method() const override { return PaymentMethod::Wallet; }

protected:
    PaymentResult doPay(const PaymentRequest& request) override {
        if (!ledger.debit(request.customerId, request.amountCents))
            return PaymentResult::fail("Insufficient wallet balance. " + Money::toString(request.amountCents) + " is required.");
        return PaymentResult::ok("Paid " + Money::toString(request.amountCents) + " from your wallet.");
    }
    PaymentResult doRefund(const PaymentRequest& request) override {
        ledger.credit(request.customerId, request.amountCents);
        return PaymentResult::ok("Refunded " + Money::toString(request.amountCents) + " to your wallet.");
    }
};

struct PaymentContext {
    IWalletLedger* wallet = nullptr;   
    CardDetails card;                  
};

class PaymentStrategyFactory {
public:
    static unique_ptr<PaymentStrategy> create(PaymentMethod method, const PaymentContext& context) {
        switch (method) {
        case PaymentMethod::Cash:
            return make_unique<CashPayment>();
        case PaymentMethod::Card:
            return make_unique<CardPayment>(context.card);
        case PaymentMethod::Wallet:
            if (!context.wallet) throw logic_error("Wallet payment requires a wallet ledger.");
            return make_unique<WalletPayment>(*context.wallet);
        }
        throw invalid_argument("Unsupported payment method.");
    }
};

class Payment {
    int id;
    int bookingId;
    long long amountCents;
    PaymentStatus status;
    unique_ptr<PaymentStrategy> strategy;

public:
    Payment(int bookingId, long long amountCents, unique_ptr<PaymentStrategy> strategy,
            PaymentStatus status = PaymentStatus::Pending, int id = 0)
        : id(id), bookingId(bookingId), amountCents(amountCents), status(status), strategy(move(strategy)) {
        if (!this->strategy) throw invalid_argument("Payment requires a strategy.");
    }

    int getId() const { return id; }
    int getBookingId() const { return bookingId; }
    long long getAmountCents() const { return amountCents; }
    PaymentStatus getStatus() const { return status; }
    PaymentMethod getMethod() const { return strategy->method(); }
    string getStatusString() const { return statusToString(status); }

    bool canProcess() const { return status == PaymentStatus::Pending || status == PaymentStatus::Failed; }
    bool canRefund() const { return status == PaymentStatus::Completed; }

    PaymentResult process(const PaymentRequest& request) {
        if (!canProcess()) return PaymentResult::fail("This payment is already " + getStatusString() + ".");
        PaymentResult result = strategy->pay(request);
        status = result.success ? PaymentStatus::Completed : PaymentStatus::Failed;
        return result;
    }

    PaymentResult refund(const PaymentRequest& request) {
        if (!canRefund()) return PaymentResult::fail("Only completed payments can be refunded.");
        PaymentResult result = strategy->refund(request);
        if (result.success) status = PaymentStatus::Refunded;
        return result;
    }

    PaymentRecord toRecord() const {
        PaymentRecord record;
        record.id = id;
        record.bookingId = bookingId;
        record.method = strategy->method();
        record.amountCents = amountCents;
        record.status = status;
        return record;
    }
};

struct PayableBooking {
    int bookingId = 0;
    int customerId = 0;
    int ticketId = 0;
    long long amountCents = 0;
    string status;            
};


class IPaymentSession {
public:
    virtual ~IPaymentSession() = default;

    virtual optional<PayableBooking> loadBooking(int bookingId) = 0;
    virtual optional<PaymentRecord> loadPayment(int bookingId) = 0;

    virtual int savePayment(const PaymentRecord& record) = 0;

    virtual void confirmBooking(int bookingId) = 0;

    virtual IWalletLedger& wallet() = 0;

    virtual void commit() = 0;
};

class IPaymentSessionFactory {
public:
    virtual ~IPaymentSessionFactory() = default;
    virtual unique_ptr<IPaymentSession> begin() = 0;
};

struct PaymentOutcome {
    bool success = false;
    string message;
    int paymentId = -1;
};

class PaymentService {
    IPaymentSessionFactory& sessions;

    static PaymentOutcome failure(const string& message) { return { false, message, -1 }; }

public:
    explicit PaymentService(IPaymentSessionFactory& sessions) : sessions(sessions) {}

    
    
    PaymentOutcome payForBooking(int bookingId, PaymentMethod method,optional<long long> expectedAmountCents = nullopt,
                                 const CardDetails& card = CardDetails{}) {
        try {
            auto session = sessions.begin();

            auto booking = session->loadBooking(bookingId);
            if (!booking) return failure("Booking not found.");
            if (booking->status == "Cancelled") return failure("This booking was cancelled and cannot be paid.");
            if (booking->status != "Pending") return failure("This booking is already paid.");
            if (expectedAmountCents && *expectedAmountCents != booking->amountCents)
                return failure("The ticket price changed to " + Money::toString(booking->amountCents) +
                               ". Please review and try again.");

            auto existing = session->loadPayment(bookingId);
            if (existing && !(existing->status == PaymentStatus::Failed || existing->status == PaymentStatus::Pending))
                return failure("This booking already has a " + statusToString(existing->status) + " payment.");

            PaymentContext context;
            context.wallet = &session->wallet();
            context.card = card;

            Payment payment(bookingId, booking->amountCents, PaymentStrategyFactory::create(method, context),
                            PaymentStatus::Pending, existing ? existing->id : 0);

            PaymentRequest request{ bookingId, booking->customerId, booking->amountCents };
            PaymentResult result = payment.process(request);

            
            int paymentId = session->savePayment(payment.toRecord());
            if (result.success) session->confirmBooking(bookingId);
            session->commit();

            return { result.success, result.message, paymentId };
        }
        catch (const exception& e) {
            cerr << "Error in PaymentService::payForBooking: " << e.what() << endl;
            return failure("A system error occurred. You were not charged. Please try again.");
        }
    }

    PaymentOutcome refundBooking(int bookingId) {
        try {
            auto session = sessions.begin();

            auto booking = session->loadBooking(bookingId);
            if (!booking) return failure("Booking not found.");
            
            if (booking->status != "Cancelled") return failure("Cancel the booking before refunding it.");

            auto record = session->loadPayment(bookingId);
            if (!record || record->status == PaymentStatus::Pending || record->status == PaymentStatus::Failed)
                return { true, "No payment was taken for this booking, nothing to refund.", record ? record->id : -1 };
            if (record->status == PaymentStatus::Refunded)
                return { true, "This payment was already refunded.", record->id };

            PaymentContext context;
            context.wallet = &session->wallet();   

            Payment payment(bookingId, record->amountCents, PaymentStrategyFactory::create(record->method, context),
                            record->status, record->id);

            PaymentRequest request{ bookingId, booking->customerId, record->amountCents };
            PaymentResult result = payment.refund(request);
            if (!result.success) return failure(result.message);   

            session->savePayment(payment.toRecord());
            session->commit();
            return { true, result.message, record->id };
        }
        catch (const exception& e) {
            cerr << "Error in PaymentService::refundBooking: " << e.what() << endl;
            return failure("A system error occurred while refunding. Nothing was changed. Please try again.");
        }
    }
};