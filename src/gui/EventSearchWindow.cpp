#include "EventSearchWindow.h"
#include "../database/EventRepository.h"
#include "../database/VenueRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

EventSearchWindow::EventSearchWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Search Events");
    resize(700, 400);

    auto *mainLayout = new QVBoxLayout(this);
    auto *topLayout = new QHBoxLayout();

    searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("Enter event title to search...");

    searchBtn = new QPushButton("Search", this);
    topLayout->addWidget(searchBar);
    topLayout->addWidget(searchBtn);

    eventsTable = new QTableWidget(this);
    eventsTable->setColumnCount(5);
    eventsTable->setHorizontalHeaderLabels({"ID", "Event Title", "Venue", "Date", "Capacity"});
    eventsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(eventsTable);

    connect(searchBtn, &QPushButton::clicked, this, &EventSearchWindow::performSearch);
    connect(searchBar, &QLineEdit::returnPressed, this, &EventSearchWindow::performSearch);

    performSearch(); // Load initial data
}

void EventSearchWindow::performSearch()
{
    std::string query = searchBar->text().toStdString();
    auto results = EventRepository::searchEvents(query);

    eventsTable->setRowCount(0);

    for (size_t i = 0; i < results.size(); ++i)
    {
        eventsTable->insertRow(static_cast<int>(i));

        auto venue = VenueRepository::getVenueById(results[i].getVenueId());
        std::string venueName = venue.has_value() ? venue->getName() : "Unknown";

        eventsTable->setItem(i, 0, new QTableWidgetItem(QString::number(results[i].getId())));
        eventsTable->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(results[i].getTitle())));
        eventsTable->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(venueName)));
        eventsTable->setItem(i, 3, new QTableWidgetItem(QString::fromStdString(results[i].getDate())));
        eventsTable->setItem(i, 4, new QTableWidgetItem(QString::number(results[i].getCapacity())));
    }
}