#pragma once

#include "../core/User.cpp"
#include "../database/PaymentRepository.cpp"   // WalletRepository
#include "LoginWindow.cpp"                      // RegisterDialog

// Screens opened from the shell
#include "EventSearchWindow.cpp"
#include "EventFormWindow.cpp"
#include "VenueFormWindow.cpp"
#include "TicketTypeWindow.cpp"
#include "BookingWindow.cpp"
#include "PaymentWindow.cpp"
#include "AdminDashboard.cpp"
#include "CheckInWindow.cpp"

#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <memory>
using namespace std;

// Admin screen: list users, add a user with any role, delete a user.
class UserManagementWindow : public QWidget {
private:
    int currentUserId;
    UserRepository userRepo;
    QTableWidget* table = nullptr;

    void setupUI() {
        setWindowTitle("Manage Users");
        resize(640, 400);

        auto* mainLayout = new QVBoxLayout(this);
        table = new QTableWidget(this);
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({ "ID", "Name", "Email", "Role" });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mainLayout->addWidget(table);

        auto* buttons = new QHBoxLayout();
        auto* btnRefresh = new QPushButton("Refresh", this);
        auto* btnAdd = new QPushButton("Add User", this);
        auto* btnDelete = new QPushButton("Delete Selected", this);
        buttons->addWidget(btnRefresh);
        buttons->addStretch();
        buttons->addWidget(btnAdd);
        buttons->addWidget(btnDelete);
        mainLayout->addLayout(buttons);

        connect(btnRefresh, &QPushButton::clicked, this, [this]() { refresh(); });
        connect(btnAdd, &QPushButton::clicked, this, [this]() {
            RegisterDialog dialog(true, this);
            if (dialog.exec() == QDialog::Accepted) refresh();
        });
        connect(btnDelete, &QPushButton::clicked, this, [this]() { handleDelete(); });
    }

    void refresh() {
        table->setRowCount(0);
        auto users = userRepo.getAllUsers();
        for (size_t i = 0; i < users.size(); ++i) {
            int r = static_cast<int>(i);
            table->insertRow(r);
            table->setItem(r, 0, new QTableWidgetItem(QString::number(users[i]->getId())));
            table->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(users[i]->getName())));
            table->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(users[i]->getEmail())));
            table->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(users[i]->getRole())));
        }
    }

    void handleDelete() {
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::warning(this, "Selection required", "Please select a user first.");
            return;
        }
        int userId = table->item(row, 0)->text().toInt();
        if (userId == currentUserId) {
            QMessageBox::warning(this, "Not allowed", "You cannot delete your own account.");
            return;
        }
        auto reply = QMessageBox::question(this, "Confirm delete",
                                           "Delete user #" + QString::number(userId) + "?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        string error;
        if (userRepo.deleteUser(userId, &error)) refresh();
        else QMessageBox::critical(this, "Error", QString::fromStdString(error));
    }

public:
    explicit UserManagementWindow(int currentUserId, QWidget* parent = nullptr)
            : QWidget(parent), currentUserId(currentUserId) {
        setupUI();
        refresh();
    }
};

// The shell: shows only the buttons the logged-in user's role is allowed to use.
class MainWindow : public QWidget {
private:
    unique_ptr<User> user;
    function<void()> onLogout;
    bool loggingOut = false;
    WalletRepository walletRepo;

    // Top-level screens get no parent (so they are real windows) and free themselves on close.
    template <class W>
    static void showWindow(W* window) {
        window->setAttribute(Qt::WA_DeleteOnClose);
        window->show();
    }

    void addNavButton(QVBoxLayout* layout, const QString& text, function<void()> action) {
        auto* button = new QPushButton(text, this);
        button->setMinimumHeight(40);
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [action]() { action(); });
    }

    void showWallet() {
        auto balance = walletRepo.getBalanceCents(user->getId());
        bool ok = false;
        double amount = QInputDialog::getDouble(
                this, "My Wallet",
                "Current balance: " + QString::fromStdString(Money::toString(balance.value_or(0))) +
                "\n\nTop-up amount (0 to cancel):",
                0.0, 0.0, 100000.0, 2, &ok);
        if (!ok || amount <= 0.0) return;

        if (walletRepo.topUp(user->getId(), Money::fromDouble(amount))) {
            auto updated = walletRepo.getBalanceCents(user->getId());
            QMessageBox::information(this, "Wallet",
                                     "Top-up successful.\nNew balance: " + QString::fromStdString(Money::toString(updated.value_or(0))));
        }
        else {
            QMessageBox::critical(this, "Error", "Could not top up the wallet.");
        }
    }

    void setupUI() {
        setWindowTitle("Event & Ticket Manager");
        resize(380, 560);

        auto* layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel(
                "<h2>Welcome, " + QString::fromStdString(user->getName()).toHtmlEscaped() + "</h2>"
                                                                                            "<i>Role: " + QString::fromStdString(user->getRole()) + "</i>", this));

        // Everyone
        addNavButton(layout, "Search Events", [this]() {
            showWindow(new EventSearchWindow(user->getId(), user->canBook()));
        });

        // Customer
        if (user->canBook()) {
            addNavButton(layout, "My Bookings", [this]() { showWindow(new BookingWindow(user->getId())); });
            addNavButton(layout, "My Wallet", [this]() { showWallet(); });
        }
        if (user->canCheckIn()) {
            addNavButton(layout, "Check-In", [this]() { showWindow(new CheckInWindow(user->getId())); });
        }

        // Organizer
        if (user->canManageEvents()) {
            addNavButton(layout, "My Events", [this]() { showWindow(new MyEventsWindow(user->getId())); });
        }
        if (user->canManageVenues()) {
            addNavButton(layout, "Manage Venues", [this]() { showWindow(new VenuesWindow()); });
        }
        if (user->canManageTickets()) {
            addNavButton(layout, "Ticket Types", [this]() { showWindow(new TicketTypeWindow(user->getId())); });
        }

        // Organizer (own events) / Admin (all events)
        if (user->canViewStatistics()) {
            addNavButton(layout, "Sales Dashboard", [this]() {
                showWindow(new AdminDashboard(user->getId(), user->canViewAllStatistics()));
            });
        }

        // Admin
        if (user->canManageUsers()) {
            addNavButton(layout, "Manage Users", [this]() { showWindow(new UserManagementWindow(user->getId())); });
        }

        layout->addStretch();

        auto* btnLogout = new QPushButton("Log out", this);
        btnLogout->setMinimumHeight(36);
        layout->addWidget(btnLogout);
        connect(btnLogout, &QPushButton::clicked, this, [this]() { logout(); });
    }

    void logout() {
        loggingOut = true;
        QApplication::closeAllWindows();     // closes this shell and every screen it opened
        if (onLogout) onLogout();
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (!loggingOut) QApplication::quit();   // closing the shell with X exits the app
        QWidget::closeEvent(event);
    }

public:
    explicit MainWindow(unique_ptr<User> loggedInUser, QWidget* parent = nullptr)
            : QWidget(parent), user(std::move(loggedInUser)) {
        setupUI();
    }

    void setOnLogout(function<void()> callback) { onLogout = std::move(callback); }
};