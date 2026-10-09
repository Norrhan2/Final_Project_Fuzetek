#pragma once

#include "../database/PaymentRepository.cpp"
#include <QCloseEvent>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <memory>

class PaymentWindow : public QWidget {
private:
    int bookingId;
    long long amountCents;

    unique_ptr<PaymentModule> ownedModule;
    PaymentService* service;
    function<void(bool)> onFinished;
    bool paid = false;
    bool notified = false;

    QComboBox* methodBox = nullptr;
    QLabel* hintLabel = nullptr;
    QGroupBox* cardGroup = nullptr;
    QLineEdit* cardNumberEdit = nullptr;
    QLineEdit* expiryEdit = nullptr;
    QLineEdit* cvvEdit = nullptr;
    QPushButton* btnPay = nullptr;
    QPushButton* btnPayLater = nullptr;

    void setupUI() {
        setWindowTitle("Payment");
        setWindowFlag(Qt::Window);
        setWindowModality(Qt::ApplicationModal);
        setAttribute(Qt::WA_DeleteOnClose);
        resize(380, 340);

        auto* mainLayout = new QVBoxLayout(this);

        auto* summary = new QLabel(
            "<b>Booking #" + QString::number(bookingId) + "</b><br>Amount due: <b>" +
            QString::fromStdString(Money::toString(amountCents)) + "</b>", this);
        mainLayout->addWidget(summary);

        auto* form = new QFormLayout();
        methodBox = new QComboBox(this);
        methodBox->addItem("Cash",   static_cast<int>(PaymentMethod::Cash));
        methodBox->addItem("Card",   static_cast<int>(PaymentMethod::Card));
        methodBox->addItem("Wallet", static_cast<int>(PaymentMethod::Wallet));
        form->addRow("Payment method:", methodBox);
        mainLayout->addLayout(form);

        cardGroup = new QGroupBox("Card details", this);
        auto* cardForm = new QFormLayout(cardGroup);
        cardNumberEdit = new QLineEdit(cardGroup);
        cardNumberEdit->setPlaceholderText("1234 5678 9012 3456");
        cardNumberEdit->setMaxLength(23);
        expiryEdit = new QLineEdit(cardGroup);
        expiryEdit->setPlaceholderText("MM/YY");
        expiryEdit->setMaxLength(5);
        cvvEdit = new QLineEdit(cardGroup);
        cvvEdit->setPlaceholderText("CVV");
        cvvEdit->setMaxLength(4);
        cvvEdit->setEchoMode(QLineEdit::Password);
        cardForm->addRow("Card number:", cardNumberEdit);
        cardForm->addRow("Expiry:", expiryEdit);
        cardForm->addRow("CVV:", cvvEdit);
        mainLayout->addWidget(cardGroup);

        hintLabel = new QLabel(this);
        hintLabel->setWordWrap(true);
        mainLayout->addWidget(hintLabel);
        mainLayout->addStretch();

        auto* btnLayout = new QHBoxLayout();
        btnPayLater = new QPushButton("Pay later", this);
        btnPay = new QPushButton("Pay " + QString::fromStdString(Money::toString(amountCents)), this);
        btnPay->setObjectName("btnPay");
        btnPayLater->setObjectName("btnPayLater");
        btnPay->setDefault(true);
        btnLayout->addWidget(btnPayLater);
        btnLayout->addStretch();
        btnLayout->addWidget(btnPay);
        mainLayout->addLayout(btnLayout);

        connect(methodBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { updateMethodUI(); });
        connect(btnPay, &QPushButton::clicked, this, [this]() { handlePay(); });
        connect(btnPayLater, &QPushButton::clicked, this, [this]() { close(); });

        updateMethodUI();
    }

    PaymentMethod selectedMethod() const {
        return static_cast<PaymentMethod>(methodBox->currentData().toInt());
    }

    void updateMethodUI() {
        PaymentMethod method = selectedMethod();
        cardGroup->setVisible(method == PaymentMethod::Card);
        switch (method) {
        case PaymentMethod::Cash:
            hintLabel->setText("Cash is recorded as received at the counter. The booking is confirmed immediately.");
            break;
        case PaymentMethod::Card:
            hintLabel->setText("Your card details are used for this payment only and are never stored.");
            break;
        case PaymentMethod::Wallet:
            hintLabel->setText("The amount is deducted from your wallet balance.");
            break;
        }
    }

    void clearCardFields() {
        cardNumberEdit->clear();
        expiryEdit->clear();
        cvvEdit->clear();
    }

    void notifyOnce(bool result) {
        if (notified) return;
        notified = true;
        if (onFinished) onFinished(result);
    }

    void handlePay() {
        PaymentMethod method = selectedMethod();

        CardDetails card;
        if (method == PaymentMethod::Card) {
            card.number = cardNumberEdit->text().toStdString();
            card.expiry = expiryEdit->text().toStdString();
            card.cvv = cvvEdit->text().toStdString();
        }

        btnPay->setEnabled(false);
        PaymentOutcome outcome = service->payForBooking(bookingId, method, amountCents, card);
        clearCardFields();   // do not keep the card number in the widgets

        if (outcome.success) {
            paid = true;
            QMessageBox::information(this, "Payment successful", QString::fromStdString(outcome.message));
            notifyOnce(true);
            close();
        }
        else {
            QMessageBox::warning(this, "Payment failed", QString::fromStdString(outcome.message));
            btnPay->setEnabled(true);
        }
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        notifyOnce(paid);    
        QWidget::closeEvent(event);
    }

public:
    PaymentWindow(int bookingId, double amount, QWidget* parent = nullptr)
        : QWidget(parent), bookingId(bookingId), amountCents(Money::fromDouble(amount)),
          ownedModule(make_unique<PaymentModule>()), service(&ownedModule->service()) {
        setupUI();
    }

    PaymentWindow(int bookingId, double amount, PaymentService& paymentService, QWidget* parent = nullptr)
        : QWidget(parent), bookingId(bookingId), amountCents(Money::fromDouble(amount)),
          service(&paymentService) {
        setupUI();
    }

    void setOnFinished(function<void(bool)> callback) {
        onFinished = std::move(callback);
    }
};