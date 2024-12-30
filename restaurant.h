#ifndef RESTAURANT_H
#define RESTAURANT_H

#include <QComboBox>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

// #include "loginscreen.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Restaurant;
}
QT_END_NAMESPACE

class Restaurant : public QWidget
{
    Q_OBJECT

public:
    Restaurant(QWidget *parent = nullptr, const QString &userType = "user");
    ~Restaurant();

private slots:
    void showOptions();
    void confirmReservation();
    void cancelReservation();
    void updateAvailabilityDisplay();

    void on_pushButton_logout_clicked();

private:
    Ui::Restaurant *ui;
    QStringList availableTimes;
    QString lastReservedTime;
    void updateAvailableTimes();
    void setupGraphics();
    QString getWaitTime(const QString &time);
    // LoginScreen *login;
};
#endif // RESTAURANT_H
