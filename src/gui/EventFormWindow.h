#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QSpinBox>
#include "../core/Event.h"
#include "../core/Venue.h"
#include <vector>

class EventFormWindow : public QDialog
{
    Q_OBJECT
private:
    QLineEdit *titleInput;
    QDateTimeEdit *dateInput;
    QComboBox *venueComboBox;
    QSpinBox *capacityInput;

    int organizerId;
    Event currentEvent;
    bool isEditMode{false};
    std::vector<Venue> venuesList;

public:
    explicit EventFormWindow(int currentOrganizerId, QWidget *parent = nullptr);

private slots:
    void onVenueSelected(int index);
    void onSaveClicked();
};