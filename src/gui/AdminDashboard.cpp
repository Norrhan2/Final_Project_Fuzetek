#pragma once

#include "../database/StatsRepository.cpp"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

// Admin sees every event. An Organizer (isAdmin = false) sees only their own events.
class AdminDashboard : public QWidget {
private:
    int viewerId;
    bool isAdmin;
    StatsRepository statsRepo;

    QLabel* lblRevenue = nullptr;
    QLabel* lblRefunds = nullptr;
    QLabel* lblTicketsSold = nullptr;
    QLabel* lblBookings = nullptr;
    QLabel* lblCancelRate = nullptr;
    QLabel* lblCheckedIn = nullptr;

    QTableWidget* eventsTable = nullptr;
    QTableWidget* typeTable = nullptr;
    QTableWidget* methodTable = nullptr;
    QPushButton* btnRefresh = nullptr;

    static QString text(const std::string& s) { return QString::fromStdString(s); }

    static QTableWidgetItem* cell(const QString& value, bool rightAlign = false) {
        auto* item = new QTableWidgetItem(value);
        if (rightAlign) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return item;
    }

    static QGroupBox* makeCard(const QString& title, QLabel*& valueLabel, QWidget* parent) {
        auto* box = new QGroupBox(title, parent);
        auto* layout = new QVBoxLayout(box);
        valueLabel = new QLabel("-", box);
        valueLabel->setAlignment(Qt::AlignCenter);
        valueLabel->setStyleSheet("font-size: 20px; font-weight: bold;");
        layout->addWidget(valueLabel);
        return box;
    }

    static void setupTable(QTableWidget* table, const QStringList& headers) {
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    void setupUI() {
        setWindowTitle(isAdmin ? "Admin Dashboard" : "Organizer Sales Dashboard");
        resize(900, 640);

        auto* mainLayout = new QVBoxLayout(this);

        // Title + refresh
        auto* topLayout = new QHBoxLayout();
        auto* title = new QLabel(isAdmin ? "<h2>Sales Statistics - All Events</h2>"
                                         : "<h2>Sales Statistics - My Events</h2>", this);
        btnRefresh = new QPushButton("Refresh", this);
        topLayout->addWidget(title);
        topLayout->addStretch();
        topLayout->addWidget(btnRefresh);
        mainLayout->addLayout(topLayout);

        // Summary cards
        auto* cards = new QHBoxLayout();
        cards->addWidget(makeCard("Total Revenue", lblRevenue, this));
        cards->addWidget(makeCard("Refunded", lblRefunds, this));
        cards->addWidget(makeCard("Tickets Sold", lblTicketsSold, this));
        cards->addWidget(makeCard("Bookings", lblBookings, this));
        cards->addWidget(makeCard("Cancellation Rate", lblCancelRate, this));
        cards->addWidget(makeCard("Checked In", lblCheckedIn, this));
        mainLayout->addLayout(cards);

        // Per-event table
        mainLayout->addWidget(new QLabel("<b>Sales per event</b>", this));
        eventsTable = new QTableWidget(this);
        setupTable(eventsTable, { "Event ID", "Title", "Date", "Sold / Capacity", "Filled", "Revenue", "Checked In" });
        mainLayout->addWidget(eventsTable, 3);

        // Breakdown tables side by side
        auto* breakdownLayout = new QHBoxLayout();

        auto* typeLayout = new QVBoxLayout();
        typeLayout->addWidget(new QLabel("<b>Sales by ticket type</b>", this));
        typeTable = new QTableWidget(this);
        setupTable(typeTable, { "Ticket Type", "Tickets Sold", "Revenue" });
        typeLayout->addWidget(typeTable);

        auto* methodLayout = new QVBoxLayout();
        methodLayout->addWidget(new QLabel("<b>Sales by payment method</b>", this));
        methodTable = new QTableWidget(this);
        setupTable(methodTable, { "Payment Method", "Payments", "Amount" });
        methodLayout->addWidget(methodTable);

        breakdownLayout->addLayout(typeLayout);
        breakdownLayout->addLayout(methodLayout);
        mainLayout->addLayout(breakdownLayout, 2);

        connect(btnRefresh, &QPushButton::clicked, this, &AdminDashboard::refreshStats);
    }

    void fillBreakdown(QTableWidget* table, const std::vector<BreakdownRow>& rows) {
        table->setRowCount(0);
        for (size_t i = 0; i < rows.size(); ++i) {
            int r = static_cast<int>(i);
            table->insertRow(r);
            table->setItem(r, 0, cell(text(rows[i].label)));
            table->setItem(r, 1, cell(QString::number(rows[i].count), true));
            table->setItem(r, 2, cell(text(Money::toString(rows[i].amountCents)), true));
        }
    }

    void fillEvents(const std::vector<EventSales>& events) {
        eventsTable->setRowCount(0);
        for (size_t i = 0; i < events.size(); ++i) {
            const EventSales& ev = events[i];
            int r = static_cast<int>(i);
            eventsTable->insertRow(r);
            eventsTable->setItem(r, 0, cell(QString::number(ev.eventId)));
            eventsTable->setItem(r, 1, cell(text(ev.title)));
            eventsTable->setItem(r, 2, cell(text(ev.date)));
            eventsTable->setItem(r, 3, cell(QString::number(ev.ticketsSold) + " / " + QString::number(ev.capacity), true));
            eventsTable->setItem(r, 4, cell(text(SalesStatistics::formatPercent(SalesStatistics::soldVsCapacity(ev))), true));
            eventsTable->setItem(r, 5, cell(text(Money::toString(ev.revenueCents)), true));
            eventsTable->setItem(r, 6, cell(QString::number(ev.checkedIn), true));
        }
    }

public:
    explicit AdminDashboard(int viewerId, bool isAdmin = true, QWidget* parent = nullptr)
        : QWidget(parent), viewerId(viewerId), isAdmin(isAdmin) {
        setupUI();
        refreshStats();
    }

    void refreshStats() {
        int organizerFilter = isAdmin ? 0 : viewerId;
        auto report = statsRepo.getReport(organizerFilter);

        if (!report) {
            QMessageBox::critical(this, "Error", "Could not load sales statistics. Please try again.");
            return;
        }

        const SalesSummary& s = report->summary;

        lblRevenue->setText(text(Money::toString(s.revenueCents)));
        lblRefunds->setText(text(Money::toString(s.refundedCents)));
        lblTicketsSold->setText(QString::number(s.ticketsSold) + " / " + QString::number(s.ticketsTotal()));
        lblBookings->setText(QString::number(s.bookingsTotal()));
        lblCancelRate->setText(text(SalesStatistics::formatPercent(SalesStatistics::cancellationRate(s))));
        lblCheckedIn->setText(QString::number(s.checkedIn) + " (" +
                              text(SalesStatistics::formatPercent(SalesStatistics::attendanceRate(s))) + ")");

        fillEvents(report->events);
        fillBreakdown(typeTable, report->byTicketType);
        fillBreakdown(methodTable, report->byPaymentMethod);
    }
};
