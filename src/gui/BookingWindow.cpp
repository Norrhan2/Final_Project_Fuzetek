#pragma once

#include "../database/BookingRepository.cpp"
#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVariant>
#include <vector>

class BookingWindow : public QWidget {
private:
    int currentCustomerId;
    BookingRepository bookingRepo;

    QTableWidget* bookingsTable;
    QPushButton* btnCancelBooking;
    QPushButton* btnRefresh;

    void setupUI() {
        setWindowTitle("Manage Bookings");
        resize(680, 400);

        auto* mainLayout = new QVBoxLayout(this);

        bookingsTable = new QTableWidget(this);
        bookingsTable->setColumnCount(4);
        bookingsTable->setHorizontalHeaderLabels({ "Booking ID", "Event ID", "Ticket ID", "Status" });
        bookingsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        bookingsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        bookingsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        bookingsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        mainLayout->addWidget(bookingsTable);

        auto* btnLayout = new QHBoxLayout();
        btnRefresh = new QPushButton("Refresh", this);
        btnCancelBooking = new QPushButton("Cancel Selected Booking", this);

        btnLayout->addWidget(btnRefresh);
        btnLayout->addStretch();
        btnLayout->addWidget(btnCancelBooking);

        mainLayout->addLayout(btnLayout);

        connect(btnRefresh, &QPushButton::clicked, this, &BookingWindow::refreshBookingsList);
        connect(btnCancelBooking, &QPushButton::clicked, this, &BookingWindow::handleCancelSelectedBooking);
    }

public:
    explicit BookingWindow(int customerId, QWidget* parent = nullptr)
        : QWidget(parent), currentCustomerId(customerId) {
        setupUI();
        refreshBookingsList();
    }

    void initiateBooking(int eventId, int ticketId, double amount) {
        int newBookingId = bookingRepo.createBooking(currentCustomerId, eventId, ticketId);
        if (newBookingId != -1) {
            QMessageBox::information(this, "Success", "Ticket reserved! Booking ID: " + QString::number(newBookingId));
            refreshBookingsList();

			// connect to payment window 
            // PaymentWindow *payWin = new PaymentWindow(newBookingId, amount, this);
            // payWin->show();
        }
        else {
            QMessageBox::critical(this, "Error", "Failed to book ticket. Please try again.");
        }
    }

    void refreshBookingsList() {
        bookingsTable->setRowCount(0);
        std::vector<Booking> bookings = bookingRepo.getBookingsByCustomer(currentCustomerId);

        for (size_t i = 0; i < bookings.size(); ++i) {
            bookingsTable->insertRow(static_cast<int>(i));

            auto* idItem = new QTableWidgetItem(QString::number(bookings[i].getId()));
            idItem->setData(Qt::UserRole, bookings[i].getId());

            bookingsTable->setItem(static_cast<int>(i), 0, idItem);
            bookingsTable->setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::number(bookings[i].getEventId())));
            bookingsTable->setItem(static_cast<int>(i), 2, new QTableWidgetItem(QString::number(bookings[i].getTicketId())));
            bookingsTable->setItem(static_cast<int>(i), 3, new QTableWidgetItem(QString::fromStdString(bookings[i].getStatusString())));
        }
    }

    void handleCancelSelectedBooking() {
        int selectedRow = bookingsTable->currentRow();
        if (selectedRow < 0) {
            QMessageBox::warning(this, "Selection Required", "Please select a booking to cancel.");
            return;
        }

        int bookingId = bookingsTable->item(selectedRow, 0)->data(Qt::UserRole).toInt();
        QString status = bookingsTable->item(selectedRow, 3)->text();

        if (status == "Cancelled") {
            QMessageBox::information(this, "Notice", "This booking is already cancelled.");
            return;
        }

        auto reply = QMessageBox::question(
            this, "Confirm Cancellation",
            "Are you sure you want to cancel booking #" + QString::number(bookingId) + "?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            if (bookingRepo.cancelBooking(bookingId)) {
                QMessageBox::information(this, "Success", "Booking successfully cancelled.");
                refreshBookingsList();
            }
            else {
                QMessageBox::critical(this, "Error", "Failed to cancel booking.");
            }
        }
    }
};