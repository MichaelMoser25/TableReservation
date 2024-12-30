#include "restaurant.h"
#include <Qtime>
#include "loginscreen.h"
#include "ui_restaurant.h"
#include <algorithm>

Restaurant::Restaurant(QWidget *parent, const QString &userType)
    : QWidget(parent)
    , ui(new Ui::Restaurant)
{
    ui->setupUi(this);
    setupGraphics();

    ui->availabilityList->hide();
    availableTimes << "11:00 AM" << "12:00 PM" << "1:00 PM" << "2:00 PM";
    ui->Table1_list->addItems(availableTimes);
    ui->Table1_list->setVisible(false);
    ui->Confirm1->setVisible(false);
    ui->Cancel1->setVisible(false);
    ui->back_select->setVisible(false);
    ui->reserve_label->setVisible(false);

    if (userType == "customer") {
        ui->customer_label->setVisible(true);
        ui->manager_label->setVisible(false);
    }

    if (userType == "manager") {
        ui->manager_label->setVisible(true);
        ui->customer_label->setVisible(false);
    }

    connect(ui->Table1, &QPushButton::clicked, this, &Restaurant::showOptions);
    connect(ui->Confirm1, &QPushButton::clicked, this, &Restaurant::confirmReservation);
    connect(ui->Cancel1, &QPushButton::clicked, this, &Restaurant::cancelReservation);
}

Restaurant::~Restaurant()
{
    delete ui;
}

void Restaurant::showOptions()
{
    ui->Table1_list->setVisible(true);
    ui->Confirm1->setVisible(true);
    ui->back_select->setVisible(true);
    ui->reserve_label->setVisible(true);
}

void Restaurant::confirmReservation()
{
    int index = ui->Table1_list->currentIndex();
    if (index >= 0) {
        lastReservedTime = ui->Table1_list->itemText(index);
        QMessageBox::information(this, "Reserved", "Table reserved for " + lastReservedTime);
        availableTimes.removeAt(index);
        ui->Table1_list->clear();
        ui->Table1_list->addItems(availableTimes);
        ui->Table1->setText("Table Reserved for " + lastReservedTime);
        ui->Table1->setEnabled(false);
        ui->Table1_list->setVisible(false);
        ui->Confirm1->setVisible(false);
        ui->back_select->setVisible(false);
        ui->reserve_label->setVisible(false);
        ui->Cancel1->setVisible(true);
        updateAvailabilityDisplay();
    }
}

void Restaurant::cancelReservation()
{
    if (!lastReservedTime.isEmpty()) {
        availableTimes.append(lastReservedTime);
        std::sort(availableTimes.begin(), availableTimes.end());

        ui->Table1_list->clear();
        ui->Table1_list->addItems(availableTimes);

        ui->Table1->setText("Table1");
        ui->Table1->setEnabled(true);

        QMessageBox::information(this,
                                 "Cancelled",
                                 "Reservation for " + lastReservedTime + " cancelled.");
        lastReservedTime.clear();
        ui->Cancel1->setVisible(false);

        updateAvailabilityDisplay();
    }
}

void Restaurant::updateAvailabilityDisplay()
{
    ui->availabilityList->clear();

    QStringList allTimes;
    allTimes << "11:00 AM" << "12:00 PM" << "1:00 PM" << "2:00 PM";

    foreach (const QString &time, allTimes) {
        QString status;
        if (availableTimes.contains(time)) {
            status = "Available";
        } else {
            status = "Reserved";
        }

        QString waitTime = getWaitTime(time);
        QString displayText = "Table1 - " + time + ": " + status;
        if (!waitTime.isEmpty()) {
            displayText += " (" + waitTime + ")";
        }
        ui->availabilityList->addItem(displayText);
    }
}

QString Restaurant::getWaitTime(const QString &time)
{
    if (!availableTimes.contains(time)) {
        QTime currentTime = QTime::currentTime();
        QTime reservationTime = QTime::fromString(time, "h:mm AP");

        if (reservationTime.isValid() && reservationTime > currentTime) {
            int minutes = currentTime.secsTo(reservationTime) / 60;
            return QString::number(minutes) + " mins wait";
        }
    }
    return "";
}

void Restaurant::setupGraphics()
{
    QGraphicsScene *table_scene = new QGraphicsScene(this);
    // QGraphicsRectItem* table1_graph = new QGraphicsRectItem(0, 0, 250, 140);

    QGraphicsScene *seat_scene = new QGraphicsScene(this);
    QGraphicsEllipseItem *seat = new QGraphicsEllipseItem(0, 0, 50, 50);
    // QGraphicsEllipseItem* seat2 = new QGraphicsEllipseItem(0, 0, 50, 50);

    // table_scene->addItem(table1_graph);
    seat_scene->addItem(seat);
    seat->setBrush(QBrush(QColor(193, 193, 193)));
    // seat_scene->addItem(seat2);

    ui->Table1_graphics->setScene(table_scene);
    ui->seat->setScene(seat_scene);
    ui->seat2->setScene(seat_scene);
    ui->seat3->setScene(seat_scene);
    ui->seat4->setScene(seat_scene);
}

void Restaurant::on_pushButton_logout_clicked()
{
    // login = new LoginScreen();
    // login->show();
    this->close();
}
