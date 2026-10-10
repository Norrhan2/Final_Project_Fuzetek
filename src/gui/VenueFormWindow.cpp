#pragma once

#include "../database/VenueRepository.cpp"
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

// Add / edit one venue (dialog).
class VenueFormWindow : public QDialog {
private:
    QLineEdit* nameInput = nullptr;
    QLineEdit* addressInput = nullptr;
    QSpinBox* capacityInput = nullptr;
    Venue currentVenue;
    bool isEditMode = false;

    void onSaveClicked() {
        if (nameInput->text().trimmed().isEmpty() || addressInput->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Error", "Please fill in all required fields.");
            return;
        }

        currentVenue.setName(nameInput->text().trimmed().toStdString());
        currentVenue.setAddress(addressInput->text().trimmed().toStdString());
        currentVenue.setCapacity(capacityInput->value());

        std::string error;
        bool success = isEditMode ? VenueRepository::updateVenue(currentVenue, &error)
                                  : VenueRepository::createVenue(currentVenue, &error);
        if (success) {
            QMessageBox::information(this, "Success", "Data saved successfully.");
            accept();
        }
        else {
            QMessageBox::critical(this, "Error", QString::fromStdString(error));
        }
    }

public:
    explicit VenueFormWindow(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Add / Edit Venue");
        resize(400, 200);

        auto* layout = new QFormLayout(this);
        nameInput = new QLineEdit(this);
        addressInput = new QLineEdit(this);
        capacityInput = new QSpinBox(this);
        capacityInput->setRange(1, 500000);

        layout->addRow("Venue Name:", nameInput);
        layout->addRow("Address:", addressInput);
        layout->addRow("Capacity:", capacityInput);

        auto* saveBtn = new QPushButton("Save", this);
        layout->addRow(saveBtn);
        connect(saveBtn, &QPushButton::clicked, this, [this]() { onSaveClicked(); });
    }

    void setVenueForEdit(const Venue& venue) {
        currentVenue = venue;
        isEditMode = true;
        nameInput->setText(QString::fromStdString(venue.getName()));
        addressInput->setText(QString::fromStdString(venue.getAddress()));
        capacityInput->setValue(venue.getCapacity());
    }
};

// All venues: list, add, edit, delete.
class VenuesWindow : public QWidget {
private:
    QTableWidget* table = nullptr;

    int selectedVenueId() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection required", "Please select a venue first.");
            return -1;
        }
        return table->item(row, 0)->text().toInt();
    }

    void setupUI() {
        setWindowTitle("Manage Venues");
        resize(640, 400);

        auto* mainLayout = new QVBoxLayout(this);
        table = new QTableWidget(this);
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({ "ID", "Name", "Address", "Capacity" });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(table);

        auto* buttons = new QHBoxLayout();
        auto* btnRefresh = new QPushButton("Refresh", this);
        auto* btnAdd = new QPushButton("Add Venue", this);
        auto* btnEdit = new QPushButton("Edit Selected", this);
        auto* btnDelete = new QPushButton("Delete Selected", this);
        buttons->addWidget(btnRefresh);
        buttons->addStretch();
        buttons->addWidget(btnAdd);
        buttons->addWidget(btnEdit);
        buttons->addWidget(btnDelete);
        mainLayout->addLayout(buttons);

        connect(btnRefresh, &QPushButton::clicked, this, [this]() { refresh(); });
        connect(btnAdd, &QPushButton::clicked, this, [this]() {
            VenueFormWindow dialog(this);
            if (dialog.exec() == QDialog::Accepted) refresh();
        });
        connect(btnEdit, &QPushButton::clicked, this, [this]() {
            int id = selectedVenueId();
            if (id < 0) return;
            auto venue = VenueRepository::getVenueById(id);
            if (!venue) { QMessageBox::critical(this, "Error", "Venue not found."); return; }
            VenueFormWindow dialog(this);
            dialog.setVenueForEdit(*venue);
            if (dialog.exec() == QDialog::Accepted) refresh();
        });
        connect(btnDelete, &QPushButton::clicked, this, [this]() {
            int id = selectedVenueId();
            if (id < 0) return;
            auto reply = QMessageBox::question(this, "Confirm delete", "Delete venue #" + QString::number(id) + "?",
                                               QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) return;
            std::string error;
            if (VenueRepository::deleteVenue(id, &error)) refresh();
            else QMessageBox::critical(this, "Error", QString::fromStdString(error));
        });
    }

public:
    explicit VenuesWindow(QWidget* parent = nullptr) : QWidget(parent) {
        setupUI();
        refresh();
    }

    void refresh() {
        table->setRowCount(0);
        auto venues = VenueRepository::getAllVenues();
        for (size_t i = 0; i < venues.size(); ++i) {
            int row = static_cast<int>(i);
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::number(venues[i].getId())));
            table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(venues[i].getName())));
            table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(venues[i].getAddress())));
            table->setItem(row, 3, new QTableWidgetItem(QString::number(venues[i].getCapacity())));
        }
    }
};