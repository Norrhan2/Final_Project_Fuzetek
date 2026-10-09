#include "VenueFormWindow.h"
#include "../database/VenueRepository.h"
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

VenueFormWindow::VenueFormWindow(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Add / Edit Venue");
    resize(400, 200);

    auto *layout = new QFormLayout(this);

    nameInput = new QLineEdit(this);
    addressInput = new QLineEdit(this);
    capacityInput = new QSpinBox(this);
    capacityInput->setRange(1, 500000);

    layout->addRow("Venue Name:", nameInput);
    layout->addRow("Address:", addressInput);
    layout->addRow("Capacity:", capacityInput);

    auto *saveBtn = new QPushButton("Save", this);
    layout->addRow(saveBtn);

    connect(saveBtn, &QPushButton::clicked, this, &VenueFormWindow::onSaveClicked);
}

void VenueFormWindow::setVenueForEdit(const Venue &venue)
{
    currentVenue = venue;
    isEditMode = true;
    nameInput->setText(QString::fromStdString(venue.getName()));
    addressInput->setText(QString::fromStdString(venue.getAddress()));
    capacityInput->setValue(venue.getCapacity());
}

void VenueFormWindow::onSaveClicked()
{
    if (nameInput->text().trimmed().isEmpty() || addressInput->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, "Error", "Please fill in all required fields.");
        return;
    }

    currentVenue.setName(nameInput->text().toStdString());
    currentVenue.setAddress(addressInput->text().toStdString());
    currentVenue.setCapacity(capacityInput->value());

    bool success = isEditMode ? VenueRepository::updateVenue(currentVenue)
                              : VenueRepository::createVenue(currentVenue);

    if (success)
    {
        QMessageBox::information(this, "Success", "Data saved successfully.");
        accept();
    }
    else
    {
        QMessageBox::critical(this, "Error", "An error occurred while connecting to the database.");
    }
}