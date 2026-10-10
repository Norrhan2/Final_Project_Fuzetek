#pragma once

#include "../database/UserRepository.cpp"
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <memory>
using namespace std;

// Used in two places:
//  - LoginWindow  -> "Create account" (role is always Customer)
//  - Admin screen -> "Add user"       (admin picks the role)
class RegisterDialog : public QDialog {
private:
    bool allowRoleSelection;
    UserRepository userRepo;

    QLineEdit* nameEdit = nullptr;
    QLineEdit* emailEdit = nullptr;
    QLineEdit* passwordEdit = nullptr;
    QLineEdit* confirmEdit = nullptr;
    QComboBox* roleBox = nullptr;

    static bool validEmail(const QString& email) {
        static const QRegularExpression pattern(R"(^[^@\s]+@[^@\s]+\.[^@\s]+$)");
        return pattern.match(email).hasMatch();
    }

    void handleSave() {
        QString name = nameEdit->text().trimmed();
        QString email = emailEdit->text().trimmed();
        QString password = passwordEdit->text();

        if (name.isEmpty()) { QMessageBox::warning(this, "Missing data", "Please enter a name."); return; }
        if (!validEmail(email)) { QMessageBox::warning(this, "Invalid email", "Please enter a valid email address."); return; }
        if (password.size() < 6) { QMessageBox::warning(this, "Weak password", "The password must be at least 6 characters."); return; }
        if (password != confirmEdit->text()) { QMessageBox::warning(this, "Mismatch", "The passwords do not match."); return; }

        string role = allowRoleSelection ? roleBox->currentText().toStdString() : string(Roles::CUSTOMER);
        string error;
        int id = userRepo.registerUser(name.toStdString(), email.toStdString(), password.toStdString(), role, &error);

        if (id == -1) {
            QMessageBox::warning(this, "Registration failed", QString::fromStdString(error));
            return;
        }
        QMessageBox::information(this, "Success", allowRoleSelection ? "User added." : "Account created. You can log in now.");
        accept();
    }

public:
    explicit RegisterDialog(bool allowRoleSelection, QWidget* parent = nullptr)
        : QDialog(parent), allowRoleSelection(allowRoleSelection) {
        setWindowTitle(allowRoleSelection ? "Add User" : "Create Account");
        setModal(true);
        resize(360, 260);

        auto* form = new QFormLayout(this);
        nameEdit = new QLineEdit(this);
        emailEdit = new QLineEdit(this);
        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        confirmEdit = new QLineEdit(this);
        confirmEdit->setEchoMode(QLineEdit::Password);

        form->addRow("Full name:", nameEdit);
        form->addRow("Email:", emailEdit);
        form->addRow("Password:", passwordEdit);
        form->addRow("Confirm password:", confirmEdit);

        if (allowRoleSelection) {
            roleBox = new QComboBox(this);
            roleBox->addItems({ Roles::CUSTOMER, Roles::ORGANIZER, Roles::ADMIN });
            form->addRow("Role:", roleBox);
        }

        auto* btnSave = new QPushButton(allowRoleSelection ? "Add user" : "Register", this);
        btnSave->setDefault(true);
        form->addRow(btnSave);
        connect(btnSave, &QPushButton::clicked, this, [this]() { handleSave(); });
    }
};

class LoginWindow : public QWidget {
private:
    UserRepository userRepo;
    function<void(unique_ptr<User>)> onLoggedIn;

    QLineEdit* emailEdit = nullptr;
    QLineEdit* passwordEdit = nullptr;
    QPushButton* btnLogin = nullptr;
    QPushButton* btnRegister = nullptr;

    void setupUI() {
        setWindowTitle("Event & Ticket Manager - Login");
        resize(360, 260);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel("<h2>Event & Ticket Manager</h2>Please log in to continue.", this));

        auto* form = new QFormLayout();
        emailEdit = new QLineEdit(this);
        emailEdit->setPlaceholderText("you@example.com");
        passwordEdit = new QLineEdit(this);
        passwordEdit->setEchoMode(QLineEdit::Password);
        form->addRow("Email:", emailEdit);
        form->addRow("Password:", passwordEdit);
        mainLayout->addLayout(form);

        btnLogin = new QPushButton("Log in", this);
        btnLogin->setDefault(true);
        btnRegister = new QPushButton("Create account", this);
        mainLayout->addWidget(btnLogin);
        mainLayout->addWidget(btnRegister);
        mainLayout->addStretch();

        connect(btnLogin, &QPushButton::clicked, this, [this]() { handleLogin(); });
        connect(emailEdit, &QLineEdit::returnPressed, this, [this]() { handleLogin(); });
        connect(passwordEdit, &QLineEdit::returnPressed, this, [this]() { handleLogin(); });
        connect(btnRegister, &QPushButton::clicked, this, [this]() {
            RegisterDialog dialog(false, this);
            dialog.exec();
        });
    }

    void handleLogin() {
        QString email = emailEdit->text().trimmed();
        QString password = passwordEdit->text();

        if (email.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "Missing data", "Please enter your email and password.");
            return;
        }

        btnLogin->setEnabled(false);
        AuthResult result = userRepo.authenticate(email.toStdString(), password.toStdString());
        btnLogin->setEnabled(true);
        passwordEdit->clear();

        if (!result.user) {
            QMessageBox::warning(this, "Login failed", QString::fromStdString(result.error));
            return;
        }
        if (onLoggedIn) onLoggedIn(std::move(result.user));
    }

public:
    explicit LoginWindow(QWidget* parent = nullptr) : QWidget(parent) { setupUI(); }

    void setOnLoggedIn(function<void(unique_ptr<User>)> callback) { onLoggedIn = std::move(callback); }

    void clearFields() {
        emailEdit->clear();
        passwordEdit->clear();
        emailEdit->setFocus();
    }
};