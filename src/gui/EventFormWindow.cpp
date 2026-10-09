#include "EventFormWindow.h"
#include "../database/EventRepository.h"
#include "../database/VenueRepository.h"
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

EventFormWindow::EventFormWindow(int currentOrganizerId, QWidget *parent)
    : QDialog(parent), organizerId(currentOrganizerId)
{
    setWindowTitle("Manage Event");
    resize(450, 250);

    auto *layout = new QFormLayout(this);

    titleInput = new QLineEdit(this);
    dateInput = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    dateInput->setDisplayFormat("yyyy-MM-dd HH:mm");

    venueComboBox = new QComboBox(this);
    venuesList = VenueRepository::getAllVenues();
    for (const auto &v : venuesList)
    {
        venueComboBox->addItem(QString::fromStdString(v.getName()), v.getId());
    }

    capacityInput = new QSpinBox(this);
    capacityInput->setRange(1, 100000);

    if (!venuesList.empty())
    {
        capacityInput->setMaximum(venuesList[0].getCapacity());
    }

    layout->addRow("Event Title:", titleInput);
    layout->addRow("Event Date:", dateInput);
    layout->addRow("Venue:", venueComboBox);
    layout->addRow("Capacity:", capacityInput);

    auto *saveBtn = new QPushButton("Save Event", this);
    layout->addRow(saveBtn);

    connect(venueComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EventFormWindow::onVenueSelected);
    connect(saveBtn, &QPushButton::clicked, this, &EventFormWindow::onSaveClicked);
}

void EventFormWindow::onVenueSelected(int index)
{
    if (index >= 0 && index < static_cast<int>(venuesList.size()))
    {
        int maxCap = venuesList[index].getCapacity();
        capacityInput->setMaximum(maxCap);
        if (capacityInput->value() > maxCap)
        {
            capacityInput->setValue(maxCap);
        }
    }
}

void EventFormWindow::onSaveClicked()
{
    if (titleInput->text().trimmed().isEmpty() || venueComboBox->currentIndex() < 0)
    {
        QMessageBox::warning(this, "Error", "Please enter event title and select a venue.");
        return;
    }

    int selectedVenueId = venueComboBox->currentData().toInt();
    currentEvent.setOrganizerId(organizerId);
    currentEvent.setVenueId(selectedVenueId);
    currentEvent.setTitle(titleInput->text().toStdString());
    currentEvent.setDate(dateInput->dateTime().toString("yyyy-MM-dd HH:mm").toStdString());
    currentEvent.setCapacity(capacityInput->value());

    bool success = isEditMode ? EventRepository::updateEvent(currentEvent)
                              : EventRepository::createEvent(currentEvent);

    if (success)
    {
        QMessageBox::information(this, "Success", "Event saved successfully!");
        accept();
    }
    else
    {
        QMessageBox::critical(this, "Error", "Failed to save event data.");
    }
}