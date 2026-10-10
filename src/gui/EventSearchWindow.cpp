#pragma once

#include "../database/EventRepository.cpp"
#include "../database/TicketRepository.cpp"
#include "../database/VenueRepository.cpp"
#include "BookingWindow.cpp"        // startBookingAndPayment()
#include <QDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
using namespace std;

// Lists the Available tickets of one event; "Book & Pay" reserves the chosen one and opens the payment window.
class TicketPickerDialog : public QDialog {
private:
    int customerId;
    int eventId;
    TicketRepository ticketRepo;
    QTableWidget* table = nullptr;

    void refresh() {
        table->setRowCount(0);
        auto tickets = ticketRepo.getAvailableTicketsByEvent(eventId);
        for (const auto& ticket : tickets) {
            int row = table->rowCount();
            table->insertRow(row);

            auto* idItem = new QTableWidgetItem(QString::number(ticket->getId()));
            idItem->setData(Qt::UserRole, ticket->getPrice());
            table->setItem(row, 0, idItem);
            table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(ticket->getTicketType())));
            table->setItem(row, 2, new QTableWidgetItem(QString::number(ticket->getPrice(), 'f', 2)));
        }
    }

    void handleBook() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection required", "Please select a ticket first.");
            return;
        }
        int ticketId = table->item(row, 0)->text().toInt();
        double price = table->item(row, 0)->data(Qt::UserRole).toDouble();

        startBookingAndPayment(this, customerId, eventId, ticketId, price, [this]() { refresh(); });
    }

public:
    TicketPickerDialog(int eventId, const QString& eventTitle, int customerId, QWidget* parent = nullptr)
        : QDialog(parent), customerId(customerId), eventId(eventId) {
        setWindowTitle("Tickets - " + eventTitle);
        resize(480, 360);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel("<b>" + eventTitle.toHtmlEscaped() + "</b> - available tickets", this));

        table = new QTableWidget(this);
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({ "Ticket ID", "Type", "Price" });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(table);

        auto* btnBook = new QPushButton("Book && Pay", this);
        mainLayout->addWidget(btnBook);
        connect(btnBook, &QPushButton::clicked, this, [this]() { handleBook(); });

        refresh();
    }
};

class EventSearchWindow : public QWidget {
private:
    int customerId;
    bool canBook;
    QLineEdit* searchBar = nullptr;
    QTableWidget* eventsTable = nullptr;

    void openTickets() {
        int row = eventsTable->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection required", "Please select an event first.");
            return;
        }
        int eventId = eventsTable->item(row, 0)->text().toInt();
        QString title = eventsTable->item(row, 1)->text();

        TicketPickerDialog dialog(eventId, title, customerId, this);
        dialog.exec();
        performSearch();   // refresh "Tickets left"
    }

public:
    explicit EventSearchWindow(int customerId, bool canBook, QWidget* parent = nullptr)
        : QWidget(parent), customerId(customerId), canBook(canBook) {
        setWindowTitle("Search Events");
        resize(820, 440);

        auto* mainLayout = new QVBoxLayout(this);
        auto* topLayout = new QHBoxLayout();

        searchBar = new QLineEdit(this);
        searchBar->setPlaceholderText("Enter event title to search...");
        auto* searchBtn = new QPushButton("Search", this);
        topLayout->addWidget(searchBar);
        topLayout->addWidget(searchBtn);

        eventsTable = new QTableWidget(this);
        eventsTable->setColumnCount(6);
        eventsTable->setHorizontalHeaderLabels({ "ID", "Event Title", "Venue", "Date", "Capacity", "Tickets left" });
        eventsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        eventsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        eventsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        eventsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        mainLayout->addLayout(topLayout);
        mainLayout->addWidget(eventsTable);

        if (canBook) {
            auto* btnTickets = new QPushButton("View Tickets && Book", this);
            mainLayout->addWidget(btnTickets);
            connect(btnTickets, &QPushButton::clicked, this, [this]() { openTickets(); });
            connect(eventsTable, &QTableWidget::cellDoubleClicked, this, [this](int, int) { openTickets(); });
        }

        connect(searchBtn, &QPushButton::clicked, this, [this]() { performSearch(); });
        connect(searchBar, &QLineEdit::returnPressed, this, [this]() { performSearch(); });

        performSearch();
    }

    void performSearch() {
        auto results = EventRepository::searchEvents(searchBar->text().toStdString());
        eventsTable->setRowCount(0);

        for (size_t i = 0; i < results.size(); ++i) {
            int row = static_cast<int>(i);
            eventsTable->insertRow(row);

            auto venue = VenueRepository::getVenueById(results[i].getVenueId());
            string venueName = venue ? venue->getName() : "Unknown";

            eventsTable->setItem(row, 0, new QTableWidgetItem(QString::number(results[i].getId())));
            eventsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(results[i].getTitle())));
            eventsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(venueName)));
            eventsTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(results[i].getDate())));
            eventsTable->setItem(row, 4, new QTableWidgetItem(QString::number(results[i].getCapacity())));
            eventsTable->setItem(row, 5, new QTableWidgetItem(
                QString::number(EventRepository::countAvailableTickets(results[i].getId()))));
        }
    }
};