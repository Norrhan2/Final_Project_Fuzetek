#pragma once

#include "../database/TicketRepository.cpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>

#include <vector>
#include <memory>

using namespace std;


class TicketTypeWindow : public QWidget {
private:

    QLineEdit* eventIdInput;

    QComboBox* ticketTypeCombo;

    QDoubleSpinBox* priceInput;

    QPushButton* createButton;

    QPushButton* refreshButton;

    QPushButton* deleteButton;

    QTableWidget* ticketTable;

    TicketRepository repository;


public:

    TicketTypeWindow(QWidget* parent = nullptr)
        : QWidget(parent) {

        setWindowTitle("Ticket Type Management");

        resize(800, 550);


        QVBoxLayout* mainLayout =
            new QVBoxLayout(this);


        QLabel* titleLabel =
            new QLabel("Ticket Type Management");

        titleLabel->setStyleSheet(
            "font-size: 22px; font-weight: bold;"
        );

        mainLayout->addWidget(titleLabel);


        QHBoxLayout* eventLayout =
            new QHBoxLayout();


        QLabel* eventLabel =
            new QLabel("Event ID:");

        eventIdInput =
            new QLineEdit();

        eventIdInput->setPlaceholderText(
            "Enter event ID"
        );


        eventLayout->addWidget(eventLabel);

        eventLayout->addWidget(
            eventIdInput
        );


        mainLayout->addLayout(
            eventLayout
        );


        QHBoxLayout* typeLayout =
            new QHBoxLayout();


        QLabel* typeLabel =
            new QLabel("Ticket Type:");

        ticketTypeCombo =
            new QComboBox();


        ticketTypeCombo->addItem(
            "Regular"
        );

        ticketTypeCombo->addItem(
            "VIP"
        );

        ticketTypeCombo->addItem(
            "Student"
        );


        typeLayout->addWidget(
            typeLabel
        );

        typeLayout->addWidget(
            ticketTypeCombo
        );


        mainLayout->addLayout(
            typeLayout
        );


        QHBoxLayout* priceLayout =
            new QHBoxLayout();


        QLabel* priceLabel =
            new QLabel("Price:");

        priceInput =
            new QDoubleSpinBox();


        priceInput->setMinimum(
            0.0
        );

        priceInput->setMaximum(
            1000000.0
        );

        priceInput->setDecimals(
            2
        );

        priceInput->setValue(
            100.0
        );


        priceLayout->addWidget(
            priceLabel
        );

        priceLayout->addWidget(
            priceInput
        );


        mainLayout->addLayout(
            priceLayout
        );


        QHBoxLayout* buttonLayout =
            new QHBoxLayout();


        createButton =
            new QPushButton(
                "Create Ticket"
            );


        refreshButton =
            new QPushButton(
                "Refresh Tickets"
            );


        deleteButton =
            new QPushButton(
                "Delete Ticket"
            );


        buttonLayout->addWidget(
            createButton
        );

        buttonLayout->addWidget(
            refreshButton
        );

        buttonLayout->addWidget(
            deleteButton
        );


        mainLayout->addLayout(
            buttonLayout
        );


        ticketTable =
            new QTableWidget();


        ticketTable->setColumnCount(
            5
        );


        ticketTable->setHorizontalHeaderLabels({
            "ID",
            "Event ID",
            "Type",
            "Price",
            "State"
        });


        ticketTable->setSelectionBehavior(
            QAbstractItemView::SelectRows
        );


        ticketTable->setEditTriggers(
            QAbstractItemView::NoEditTriggers
        );


        mainLayout->addWidget(
            ticketTable
        );


        connect(
            createButton,
            &QPushButton::clicked,
            this,
            &TicketTypeWindow::createTicket
        );


        connect(
            refreshButton,
            &QPushButton::clicked,
            this,
            &TicketTypeWindow::refreshTickets
        );


        connect(
            deleteButton,
            &QPushButton::clicked,
            this,
            &TicketTypeWindow::deleteTicket
        );
    }


private slots:

    void createTicket() {

        bool ok = false;

        int eventId =
            eventIdInput->text().toInt(&ok);


        if (!ok || eventId <= 0) {

            QMessageBox::warning(
                this,
                "Invalid Event ID",
                "Please enter a valid Event ID."
            );

            return;
        }


        string type =
            ticketTypeCombo->currentText()
                .toStdString();


        double price =
            priceInput->value();


        int ticketId =
            repository.createTicket(
                eventId,
                type,
                price
            );


        if (ticketId == -1) {

            QMessageBox::critical(
                this,
                "Error",
                "Failed to create ticket."
            );

            return;
        }


        QMessageBox::information(
            this,
            "Success",
            QString(
                "Ticket created successfully.\nTicket ID: %1"
            ).arg(ticketId)
        );


        refreshTickets();
    }


    void refreshTickets() {

        bool ok = false;

        int eventId =
            eventIdInput->text().toInt(&ok);


        if (!ok || eventId <= 0) {

            QMessageBox::warning(
                this,
                "Invalid Event ID",
                "Please enter a valid Event ID."
            );

            return;
        }


        vector<unique_ptr<Ticket>> tickets =
            repository.getTicketsByEvent(
                eventId
            );


        ticketTable->setRowCount(
            0
        );


        for (
            const auto& ticket : tickets
        ) {

            int row =
                ticketTable->rowCount();


            ticketTable->insertRow(
                row
            );


            ticketTable->setItem(
                row,
                0,
                new QTableWidgetItem(
                    QString::number(
                        ticket->getId()
                    )
                )
            );


            ticketTable->setItem(
                row,
                1,
                new QTableWidgetItem(
                    QString::number(
                        ticket->getEventId()
                    )
                )
            );


            ticketTable->setItem(
                row,
                2,
                new QTableWidgetItem(
                    QString::fromStdString(
                        ticket->getTicketType()
                    )
                )
            );


            ticketTable->setItem(
                row,
                3,
                new QTableWidgetItem(
                    QString::number(
                        ticket->getPrice(),
                        'f',
                        2
                    )
                )
            );


            ticketTable->setItem(
                row,
                4,
                new QTableWidgetItem(
                    QString::fromStdString(
                        ticket->getState()
                    )
                )
            );
        }


        ticketTable->resizeColumnsToContents();
    }


    void deleteTicket() {

        QList<QTableWidgetItem*> selected =
            ticketTable->selectedItems();


        if (selected.isEmpty()) {

            QMessageBox::warning(
                this,
                "No Selection",
                "Please select a ticket first."
            );

            return;
        }


        int row =
            selected.first()->row();


        int ticketId =
            ticketTable
                ->item(row, 0)
                ->text()
                .toInt();


        QMessageBox::StandardButton answer =
            QMessageBox::question(
                this,
                "Confirm Delete",
                "Are you sure you want to delete this ticket?"
            );


        if (
            answer != QMessageBox::Yes
        ) {
            return;
        }


        if (
            repository.deleteTicket(
                ticketId
            )
        ) {

            QMessageBox::information(
                this,
                "Success",
                "Ticket deleted successfully."
            );

            refreshTickets();
        }
        else {

            QMessageBox::critical(
                this,
                "Error",
                "Failed to delete ticket."
            );
        }
    }
};