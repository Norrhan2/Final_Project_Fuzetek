#include <QApplication>
#include <QMessageBox>
#include <memory>
#include <string>

// Single translation unit: this is the only file CMake compiles.
#include "gui/MainWindow.cpp"
#include "gui/LoginWindow.cpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Event & Ticket Manager");
    app.setStyle("Fusion");
    app.setFont(QFont("Segoe UI", 10));
    app.setStyleSheet(R"(
        QPushButton { background:#2d6cdf; color:white; border:none; border-radius:6px; padding:8px 14px; }
        QPushButton:hover { background:#2459b8; }
        QPushButton:pressed { background:#1c478f; }
        QPushButton:disabled { background:#b0b7c3; }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QDateTimeEdit {
            padding:6px; border:1px solid #c5cad3; border-radius:6px; background:white; }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus, QDateTimeEdit:focus {
            border:1px solid #2d6cdf; }
        QTableWidget { border:1px solid #d5d9e0; gridline-color:#e3e6eb; }
        QTableWidget::item:selected { background:#dbe7ff; color:black; }
        QHeaderView::section { background:#f0f2f5; padding:6px; border:none;
            border-bottom:1px solid #d5d9e0; font-weight:bold; }
        QGroupBox { border:1px solid #d5d9e0; border-radius:8px; margin-top:12px; padding-top:10px; }
        QGroupBox::title { subcontrol-origin:margin; left:10px; padding:0 4px; }
    )");

    string dbError;
    if (!DatabaseManager::getInstance().testConnection(&dbError)) {
        QMessageBox::critical(nullptr, "Database error",
            "Could not connect to PostgreSQL.\n\n" + QString::fromStdString(dbError) +
            "\n\nCheck that PostgreSQL is running and that the connection settings are correct "
            "(environment variable EVENT_DB_CONNINFO).");
        return 1;
    }

    LoginWindow login;
    login.setOnLoggedIn([&login](unique_ptr<User> user) {
        auto* shell = new MainWindow(std::move(user));
        shell->setAttribute(Qt::WA_DeleteOnClose);
        shell->setOnLogout([&login]() {
            login.clearFields();
            login.show();
        });
        login.hide();
        shell->show();
    });

    login.show();
    return app.exec();
}