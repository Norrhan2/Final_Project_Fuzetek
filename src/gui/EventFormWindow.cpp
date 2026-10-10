#pragma once

#include "../database/EventRepository.cpp"
#include "../database/VenueRepository.cpp"
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

// Create / edit one event (dialog).
class EventFormWindow : public QDialog {
private:
    QLineEdit* titleInput = nullptr;
    QDateTimeEdit* dateInput = nullptr;
    QComboBox* venueComboBox = nullptr;
    QSpinBox* capacityInput = nullptr;

    int organizerId;
    Event currentEvent;
    bool isEditMode = false;
    std::vector<Venue> venuesList;

    void onVenueSelected(int index) {
        if (index < 0 || index >= static_cast<int>(venuesList.size())) return;
        int maxCap = venuesList[index].getCapacity();
        capacityInput->setMaximum(maxCap);
        if (capacityInput->value() > maxCap) capacityInput->setValue(maxCap);
    }

    void onSaveClicked() {
        if (titleInput->text().trimmed().isEmpty() || venueComboBox->currentIndex() < 0) {
            QMessageBox::warning(this, "Error", "Please enter an event title and select a venue.");
            return;
        }
        if (!isEditMode && dateInput->dateTime() <= QDateTime::currentDateTime()) {
            QMessageBox::warning(this, "Error", "The event date must be in the future.");
            return;
        }

        currentEvent.setOrganizerId(organizerId);
        currentEvent.setVenueId(venueComboBox->currentData().toInt());
        currentEvent.setTitle(titleInput->text().trimmed().toStdString());
        currentEvent.setDate(dateInput->dateTime().toString("yyyy-MM-dd HH:mm").toStdString());
        currentEvent.setCapacity(capacityInput->value());

        std::string error;
        bool success = isEditMode ? EventRepository::updateEvent(currentEvent, &error)
                                  : EventRepository::createEvent(currentEvent, &error);
        if (success) {
            QMessageBox::information(this, "Success", "Event saved successfully!");
            accept();
        }
        else {
            QMessageBox::critical(this, "Error", QString::fromStdString(error));
        }
    }

public:
    explicit EventFormWindow(int currentOrganizerId, QWidget* parent = nullptr)
        : QDialog(parent), organizerId(currentOrganizerId) {
        setWindowTitle("Create Event");
        resize(450, 250);

        auto* layout = new QFormLayout(this);

        titleInput = new QLineEdit(this);
        titleInput->setMaxLength(200);
        dateInput = new QDateTimeEdit(QDateTime::currentDateTime().addDays(1), this);
        dateInput->setDisplayFormat("yyyy-MM-dd HH:mm");
        dateInput->setCalendarPopup(true);

        venueComboBox = new QComboBox(this);
        venuesList = VenueRepository::getAllVenues();
        for (const auto& v : venuesList)
            venueComboBox->addItem(QString::fromStdString(v.getName()), v.getId());

        capacityInput = new QSpinBox(this);
        capacityInput->setRange(1, 100000);
        if (!venuesList.empty()) capacityInput->setMaximum(venuesList[0].getCapacity());

        layout->addRow("Event Title:", titleInput);
        layout->addRow("Event Date:", dateInput);
        layout->addRow("Venue:", venueComboBox);
        layout->addRow("Capacity:", capacityInput);

        auto* saveBtn = new QPushButton("Save Event", this);
        layout->addRow(saveBtn);

        connect(venueComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int index) { onVenueSelected(index); });
        connect(saveBtn, &QPushButton::clicked, this, [this]() { onSaveClicked(); });
    }

    void setEventForEdit(const Event& event) {
        currentEvent = event;
        isEditMode = true;
        setWindowTitle("Edit Event");

        titleInput->setText(QString::fromStdString(event.getTitle()));
        dateInput->setDateTime(QDateTime::fromString(QString::fromStdString(event.getDate()).left(16), "yyyy-MM-dd HH:mm"));
        for (int i = 0; i < venueComboBox->count(); ++i) {
            if (venueComboBox->itemData(i).toInt() == event.getVenueId()) venueComboBox->setCurrentIndex(i);
        }
        capacityInput->setValue(event.getCapacity());
    }
};

// The organizer's own events: list, create, edit, delete.
class MyEventsWindow : public QWidget {
private:
    int organizerId;
    QTableWidget* table = nullptr;

    int selectedEventId() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection required", "Please select an event first.");
            return -1;
        }
        return table->item(row, 0)->text().toInt();
    }

    void setupUI() {
        setWindowTitle("My Events");
        resize(760, 420);

        auto* mainLayout = new QVBoxLayout(this);
        table = new QTableWidget(this);
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({ "ID", "Title", "Venue", "Date", "Capacity" });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(table);

        auto* buttons = new QHBoxLayout();
        auto* btnRefresh = new QPushButton("Refresh", this);
        auto* btnNew = new QPushButton("New Event", this);
        auto* btnEdit = new QPushButton("Edit Selected", this);
        auto* btnDelete = new QPushButton("Delete Selected", this);
        buttons->addWidget(btnRefresh);
        buttons->addStretch();
        buttons->addWidget(btnNew);
        buttons->addWidget(btnEdit);
        buttons->addWidget(btnDelete);
        mainLayout->addLayout(buttons);

        connect(btnRefresh, &QPushButton::clicked, this, [this]() { refresh(); });
        connect(btnNew, &QPushButton::clicked, this, [this]() {
            EventFormWindow dialog(organizerId, this);
            if (dialog.exec() == QDialog::Accepted) refresh();
        });
        connect(btnEdit, &QPushButton::clicked, this, [this]() {
            int id = selectedEventId();
            if (id < 0) return;
            auto event = EventRepository::getEventById(id);
            if (!event) { QMessageBox::critical(this, "Error", "Event not found."); return; }
            EventFormWindow dialog(organizerId, this);
            dialog.setEventForEdit(*event);
            if (dialog.exec() == QDialog::Accepted) refresh();
        });
        connect(btnDelete, &QPushButton::clicked, this, [this]() { handleDelete(); });
    }

    void handleDelete() {
        int id = selectedEventId();
        if (id < 0) return;
        auto reply = QMessageBox::question(this, "Confirm delete",
                                           "Delete event #" + QString::number(id) + " and all of its tickets?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        std::string error;
        if (EventRepository::deleteEvent(id, organizerId, &error)) refresh();
        else QMessageBox::critical(this, "Error", QString::fromStdString(error));
    }

public:
    explicit MyEventsWindow(int organizerId, QWidget* parent = nullptr)
        : QWidget(parent), organizerId(organizerId) {
        setupUI();
        refresh();
    }

    void refresh() {
        table->setRowCount(0);
        auto events = EventRepository::getEventsByOrganizer(organizerId);
        for (size_t i = 0; i < events.size(); ++i) {
            int row = static_cast<int>(i);
            table->insertRow(row);
            auto venue = VenueRepository::getVenueById(events[i].getVenueId());
            std::string venueName = venue ? venue->getName() : "Unknown";

            table->setItem(row, 0, new QTableWidgetItem(QString::number(events[i].getId())));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(events[i].getTitle())));
            table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(venueName)));
            table->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(events[i].getDate())));
            table->setItem(row, 4, new QTableWidgetItem(QString::number(events[i].getCapacity())));
        }
    }
};