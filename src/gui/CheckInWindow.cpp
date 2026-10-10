#pragma once

#include "../database/AttendeeRepository.cpp"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
using namespace std;

// Customer screen: shows paid bookings and lets the customer check in at the event.
class CheckInWindow : public QWidget {
private:
    int customerId;
    AttendeeRepository attendeeRepo;
    QTableWidget* table = nullptr;

    void setupUI() {
        setWindowTitle("Event Check-In");
        resize(760, 400);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel(
            "<h2>Check-In</h2>You can check in from 24 hours before until 24 hours after the event starts.", this));

        table = new QTableWidget(this);
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({ "Booking ID", "Event", "Date", "Ticket", "Checked in at" });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(table);

        auto* buttons = new QHBoxLayout();
        auto* btnRefresh = new QPushButton("Refresh", this);
        auto* btnCheckIn = new QPushButton("Check In Selected", this);
        buttons->addWidget(btnRefresh);
        buttons->addStretch();
        buttons->addWidget(btnCheckIn);
        mainLayout->addLayout(buttons);

        connect(btnRefresh, &QPushButton::clicked, this, [this]() { refresh(); });
        connect(btnCheckIn, &QPushButton::clicked, this, [this]() { handleCheckIn(); });
    }

    void handleCheckIn() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection required", "Please select a booking first.");
            return;
        }
        int bookingId = table->item(row, 0)->text().toInt();

        CheckInResult result = attendeeRepo.checkIn(bookingId, customerId);
        if (result.success) QMessageBox::information(this, "Check-in", QString::fromStdString(result.message));
        else QMessageBox::warning(this, "Check-in failed", QString::fromStdString(result.message));
        refresh();
    }

public:
    explicit CheckInWindow(int customerId, QWidget* parent = nullptr)
        : QWidget(parent), customerId(customerId) {
        setupUI();
        refresh();
    }

    void refresh() {
        table->setRowCount(0);
        auto entries = attendeeRepo.getEntriesByCustomer(customerId);
        for (size_t i = 0; i < entries.size(); ++i) {
            int row = static_cast<int>(i);
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::number(entries[i].bookingId)));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(entries[i].eventTitle)));
            table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(entries[i].eventDate)));
            table->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(entries[i].ticketType)));
            table->setItem(row, 4, new QTableWidgetItem(entries[i].isCheckedIn()
                ? QString::fromStdString(entries[i].checkedInAt) : QString("Not yet")));
        }
    }
};