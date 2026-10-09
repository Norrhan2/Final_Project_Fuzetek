// EventSearchWindow.h
#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>

class EventSearchWindow : public QWidget
{
    Q_OBJECT
private:
    QLineEdit *searchBar;
    QTableWidget *eventsTable;
    QPushButton *searchBtn;

public:
    explicit EventSearchWindow(QWidget *parent = nullptr);

private slots:
    void performSearch();
};