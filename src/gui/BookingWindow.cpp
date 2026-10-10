#pragma once

#include "../database/BookingRepository.cpp"
#include "../database/EventRepository.cpp"
#include "../database/TicketRepository.cpp"
#include "PaymentWindow.cpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <vector>
using namespace std;

// Shared by the event-search screen and this screen:
// reserve the ticket, then immediately offer payment. If the customer closes the payment window
// ("Pay later"), the booking stays Pending and can be paid or cancelled from "My Bookings".
inline void startBookingAndPayment(QWidget* parent, int customerId, int eventId, int ticketId,
                                   double price, function<void()> onDone) {
    BookingRepository bookingRepo;
    string error;
    int bookingId = bookingRepo.createBooking(customerId, eventId, ticketId, &error);

    if (bookingId == -1) {
        QMessageBox::warning(parent, "Booking failed", QString::fromStdString(error));
        if (onDone) onDone();
        return;
    }

    auto* payWindow = new PaymentWindow(bookingId, price, parent);
    QPointer<QWidget> guard(parent);
    payWindow->setOnFinished([guard, onDone](bool paid) {
        if (!paid && guard) {
            QMessageBox::information(guard, "Payment pending",
                "Your ticket is reserved but not paid yet. You can pay or cancel it from \"My Bookings\".");
        }
        if (onDone) onDone();
    });
    payWindow->show();
}

class BookingWindow : public QWidget {
private:
    int currentCustomerId;
    BookingRepository bookingRepo;
    TicketRepository ticketRepo;
    PaymentModule paymentModule;

    QTableWidget* bookingsTable = nullptr;

    void setupUI() {
        setWindowTitle("My Bookings");
        resize(760, 420);

        auto* mainLayout = new QVBoxLayout(this);

        bookingsTable = new QTableWidget(this);
        bookingsTable->setColumnCount(5);
        bookingsTable->setHorizontalHeaderLabels({ "Booking ID", "Event", "Ticket ID", "Status", "Payment" });
        bookingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        bookingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        bookingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        bookingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(bookingsTable);

        auto* btnLayout = new QHBoxLayout();
        auto* btnRefresh = new QPushButton("Refresh", this);
        auto* btnPay = new QPushButton("Pay Now", this);
        auto* btnCancel = new QPushButton("Cancel Selected Booking", this);
        btnLayout->addWidget(btnRefresh);
        btnLayout->addStretch();
        btnLayout->addWidget(btnPay);
        btnLayout->addWidget(btnCancel);
        mainLayout->addLayout(btnLayout);

        connect(btnRefresh, &QPushButton::clicked, this, [this]() { refreshBookingsList(); });
        connect(btnPay, &QPushButton::clicked, this, [this]() { handlePaySelectedBooking(); });
        connect(btnCancel, &QPushButton::clicked, this, [this]() { handleCancelSelectedBooking(); });
    }

public:
    explicit BookingWindow(int customerId, QWidget* parent = nullptr)
        : QWidget(parent), currentCustomerId(customerId) {
        setupUI();
        refreshBookingsList();
    }

    void initiateBooking(int eventId, int ticketId, double amount) {
        startBookingAndPayment(this, currentCustomerId, eventId, ticketId, amount, [this]() { refreshBookingsList(); });
    }

    void refreshBookingsList() {
        bookingsTable->setRowCount(0);
        vector<Booking> bookings = bookingRepo.getBookingsByCustomer(currentCustomerId);

        for (size_t i = 0; i < bookings.size(); ++i) {
            int row = static_cast<int>(i);
            bookingsTable->insertRow(row);

            auto event = EventRepository::getEventById(bookings[i].getEventId());
            QString eventText = event ? QString::fromStdString(event->getTitle())
                                      : QString::number(bookings[i].getEventId());

            auto payment = paymentModule.payments().findByBooking(bookings[i].getId());
            QString paymentText = payment ? QString::fromStdString(statusToString(payment->status)) : "Unpaid";

            auto* idItem = new QTableWidgetItem(QString::number(bookings[i].getId()));
            idItem->setData(Qt::UserRole, bookings[i].getId());

            bookingsTable->setItem(row, 0, idItem);
            bookingsTable->setItem(row, 1, new QTableWidgetItem(eventText));
            bookingsTable->setItem(row, 2, new QTableWidgetItem(QString::number(bookings[i].getTicketId())));
            bookingsTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(bookings[i].getStatusString())));
            bookingsTable->setItem(row, 4, new QTableWidgetItem(paymentText));
        }
    }

    void handlePaySelectedBooking() {
        int row = bookingsTable->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection Required", "Please select a booking to pay.");
            return;
        }
        if (bookingsTable->item(row, 3)->text() != "Pending") {
            QMessageBox::information(this, "Notice", "Only Pending bookings can be paid.");
            return;
        }

        int bookingId = bookingsTable->item(row, 0)->data(Qt::UserRole).toInt();
        int ticketId = bookingsTable->item(row, 2)->text().toInt();

        auto ticket = ticketRepo.getTicketById(ticketId);
        if (!ticket) {
            QMessageBox::critical(this, "Error", "The ticket of this booking could not be found.");
            return;
        }

        auto* payWindow = new PaymentWindow(bookingId, ticket->getPrice(), this);
        payWindow->setOnFinished([this](bool) { refreshBookingsList(); });
        payWindow->show();
    }

    void handleCancelSelectedBooking() {
        int row = bookingsTable->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection Required", "Please select a booking to cancel.");
            return;
        }

        int bookingId = bookingsTable->item(row, 0)->data(Qt::UserRole).toInt();
        if (bookingsTable->item(row, 3)->text() == "Cancelled") {
            QMessageBox::information(this, "Notice", "This booking is already cancelled.");
            return;
        }

        auto reply = QMessageBox::question(
            this, "Confirm Cancellation",
            "Are you sure you want to cancel booking #" + QString::number(bookingId) + "?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        string error;
        if (!bookingRepo.cancelBooking(bookingId, currentCustomerId, &error)) {
            QMessageBox::critical(this, "Error", QString::fromStdString(error));
            return;
        }

        // The booking is now Cancelled, so PaymentService may refund whatever was paid.
        PaymentOutcome refund = paymentModule.service().refundBooking(bookingId);
        if (refund.success) {
            QMessageBox::information(this, "Booking cancelled",
                                     "Booking cancelled.\n" + QString::fromStdString(refund.message));
        }
        else {
            QMessageBox::warning(this, "Refund problem",
                                 "The booking was cancelled, but the refund failed:\n" +
                                 QString::fromStdString(refund.message));
        }
        refreshBookingsList();
    }
};