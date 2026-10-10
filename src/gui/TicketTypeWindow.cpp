#pragma once

#include "../database/EventRepository.cpp"
#include "../database/TicketRepository.cpp"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>
using namespace std;

// Organizer screen: pick one of YOUR events, create tickets (in bulk), see/delete them.
class TicketTypeWindow : public QWidget {
private:
    int organizerId;
    TicketRepository repository;
    vector<Event> myEvents;

    QComboBox* eventCombo = nullptr;
    QComboBox* typeCombo = nullptr;
    QDoubleSpinBox* priceInput = nullptr;
    QSpinBox* quantityInput = nullptr;
    QLabel* capacityLabel = nullptr;
    QTableWidget* ticketTable = nullptr;

    int selectedEventId() const {
        int index = eventCombo->currentIndex();
        return index >= 0 ? eventCombo->itemData(index).toInt() : -1;
    }

    void setupUI() {
        setWindowTitle("Ticket Type Management");
        resize(760, 540);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel("<h2>Ticket Type Management</h2>", this));

        myEvents = EventRepository::getEventsByOrganizer(organizerId);
        eventCombo = new QComboBox(this);
        for (const auto& event : myEvents) {
            eventCombo->addItem(QString::fromStdString(event.getTitle() + "  (" + event.getDate() + ")"), event.getId());
        }
        auto* eventRow = new QHBoxLayout();
        eventRow->addWidget(new QLabel("Event:", this));
        eventRow->addWidget(eventCombo, 1);
        mainLayout->addLayout(eventRow);

        typeCombo = new QComboBox(this);
        typeCombo->addItems({ "Regular", "VIP", "Student" });
        priceInput = new QDoubleSpinBox(this);
        priceInput->setRange(0.0, 1000000.0);
        priceInput->setDecimals(2);
        priceInput->setValue(100.0);
        quantityInput = new QSpinBox(this);
        quantityInput->setRange(1, 1000);
        quantityInput->setValue(10);

        auto* createRow = new QHBoxLayout();
        createRow->addWidget(new QLabel("Type:", this));
        createRow->addWidget(typeCombo);
        createRow->addWidget(new QLabel("Price:", this));
        createRow->addWidget(priceInput);
        createRow->addWidget(new QLabel("Quantity:", this));
        createRow->addWidget(quantityInput);
        auto* btnCreate = new QPushButton("Create Tickets", this);
        createRow->addWidget(btnCreate);
        mainLayout->addLayout(createRow);

        capacityLabel = new QLabel(this);
        mainLayout->addWidget(capacityLabel);

        ticketTable = new QTableWidget(this);
        ticketTable->setColumnCount(4);
        ticketTable->setHorizontalHeaderLabels({ "ID", "Type", "Price", "State" });
        ticketTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        ticketTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        ticketTable->setSelectionMode(QAbstractItemView::SingleSelection);
        ticketTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(ticketTable);

        auto* bottomRow = new QHBoxLayout();
        auto* btnRefresh = new QPushButton("Refresh", this);
        auto* btnDelete = new QPushButton("Delete Selected Ticket", this);
        bottomRow->addWidget(btnRefresh);
        bottomRow->addStretch();
        bottomRow->addWidget(btnDelete);
        mainLayout->addLayout(bottomRow);

        connect(eventCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refreshTickets(); });
        connect(btnCreate, &QPushButton::clicked, this, [this]() { createTickets(); });
        connect(btnRefresh, &QPushButton::clicked, this, [this]() { refreshTickets(); });
        connect(btnDelete, &QPushButton::clicked, this, [this]() { deleteTicket(); });
    }

    void createTickets() {
        int eventId = selectedEventId();
        if (eventId < 0) {
            QMessageBox::warning(this, "No event", "Create an event first (My Events).");
            return;
        }

        string error;
        int created = repository.createTickets(eventId, typeCombo->currentText().toStdString(),
                                               priceInput->value(), quantityInput->value(), organizerId, &error);
        if (created < 0) {
            QMessageBox::critical(this, "Error", QString::fromStdString(error));
            return;
        }
        QMessageBox::information(this, "Success", QString::number(created) + " ticket(s) created.");
        refreshTickets();
    }

    void deleteTicket() {
        int row = ticketTable->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "No Selection", "Please select a ticket first.");
            return;
        }
        int ticketId = ticketTable->item(row, 0)->text().toInt();

        auto answer = QMessageBox::question(this, "Confirm Delete", "Delete ticket #" + QString::number(ticketId) + "?",
                                            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes) return;

        string error;
        if (repository.deleteTicket(ticketId, organizerId, &error)) refreshTickets();
        else QMessageBox::critical(this, "Error", QString::fromStdString(error));
    }

public:
    explicit TicketTypeWindow(int organizerId, QWidget* parent = nullptr)
        : QWidget(parent), organizerId(organizerId) {
        setupUI();
        refreshTickets();
    }

    void refreshTickets() {
        ticketTable->setRowCount(0);

        int eventId = selectedEventId();
        if (eventId < 0) {
            capacityLabel->setText("You have no events yet. Create one in \"My Events\" first.");
            return;
        }

        auto tickets = repository.getTicketsByEvent(eventId);
        int active = 0;
        for (const auto& ticket : tickets) {
            int row = ticketTable->rowCount();
            ticketTable->insertRow(row);
            ticketTable->setItem(row, 0, new QTableWidgetItem(QString::number(ticket->getId())));
            ticketTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(ticket->getTicketType())));
            ticketTable->setItem(row, 2, new QTableWidgetItem(QString::number(ticket->getPrice(), 'f', 2)));
            ticketTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(ticket->getState())));
            if (ticket->getState() != "Cancelled") ++active;
        }

        int capacity = myEvents[eventCombo->currentIndex()].getCapacity();
        capacityLabel->setText("Tickets created: " + QString::number(active) + " / " + QString::number(capacity) +
                               " (event capacity)");
    }
};