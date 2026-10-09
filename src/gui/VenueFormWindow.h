#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include "../core/Venue.h"

class VenueFormWindow : public QDialog
{
    Q_OBJECT
private:
    QLineEdit *nameInput;
    QLineEdit *addressInput;
    QSpinBox *capacityInput;
    Venue currentVenue;
    bool isEditMode{false};

public:
    explicit VenueFormWindow(QWidget *parent = nullptr);
    void setVenueForEdit(const Venue &venue);

private slots:
    void onSaveClicked();
};