#include "home.h"
#include "ui_home.h"

#include <iostream>

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

Home::Home(QWidget *parent, const QString &userMode, int userId, const QString &username)
    : QDialog(parent), currentUser(username), userMode(userMode), userId(userId) // Store username
    , ui(new Ui::Home)
    , m_userMode(userMode)
    , selectedTable(nullptr)
    , m_sortAscending(true)
    , totalTables(14)  // Initialize with 14 tables since that's what we use
    , rightPanel(nullptr)
{
    // Initialize table status
    for (int i = 1; i <= totalTables; i++) {
        tableStatus[i] = false;
    }

    ui->setupUi(this);
    setupUI();
    setupTables();
    setupConnections();
    loadReservations();

    // Set up the pages
    setupReservationsPage();
    setupBookingPage();
    setupContactPage();
    setupWalkinPage();

    // Initialize with Dashboard/My Reservations selected
    ui->Dashboard->setChecked(true);
    ui->Orders->setChecked(false);
    ui->Menu->setChecked(false);
    ui->Locations->setChecked(false);

    // Show the reservations page by default
    showReservationsPage();

    // Start timer for periodic updates
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Home::updateTableStatus);
    timer->start(60000); // Update every minute
}


void Home::setupUI()
{
    // Set window title and background
    setWindowTitle("Restaurant Table Reservation System");
    this->setStyleSheet(
        "QDialog {"
        "    background-color: #F8F9FA;"
        "    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;"
        "}");

    ui->sidebar->setStyleSheet(
        "QWidget {"
        "    background-color: #1E293B;"
        "    border-right: 1px solid rgba(255, 255, 255, 0.08);"
        "    box-shadow: 2px 0 4px rgba(0, 0, 0, 0.1);"
        "}");
    ui->sidebar->setFixedWidth(280);
    ui->sidebar->setGeometry(0, 90, 280, height() - 90);

    QWidget* profileSection = new QWidget(ui->sidebar);
    profileSection->setGeometry(0, 0, 280, 100);
    profileSection->setStyleSheet(
        "QWidget {"
        "    background-color: rgba(255, 255, 255, 0.05);"
        "    border-bottom: 1px solid rgba(255, 255, 255, 0.08);"
        "}");

    QHBoxLayout* profileLayout = new QHBoxLayout(profileSection);
    profileLayout->setContentsMargins(24, 24, 24, 24);

    // profile image
    QLabel* profileImage = new QLabel(profileSection);
    profileImage->setFixedSize(48, 48);
    profileImage->setStyleSheet(
        "QLabel {"
        "    background-color: #3B82F6;"
        "    border-radius: 24px;"
        "    border: 2px solid rgba(255, 255, 255, 0.2);"
        "    margin-right: 16px;"
        "    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);"
        "}");

    QVBoxLayout* profileTextLayout = new QVBoxLayout;
    QLabel* nameLabel = new QLabel("Welcome back,");
    QLabel* userLabel = new QLabel(currentUser);
    nameLabel->setStyleSheet(
        "color: rgba(255, 255, 255, 0.7);"
        "font-size: 14px;"
        "letter-spacing: 0.3px;");
    userLabel->setStyleSheet(
        "color: white;"
        "font-size: 16px;"
        "font-weight: 600;"
        "letter-spacing: 0.3px;");
    profileTextLayout->addWidget(nameLabel);
    profileTextLayout->addWidget(userLabel);
    profileTextLayout->setSpacing(4);

    profileLayout->addWidget(profileImage);
    profileLayout->addLayout(profileTextLayout);
    profileLayout->addStretch();

    QString sidebarButtonStyle =
        "QPushButton {"
        "    text-align: left;"
        "    padding: 16px 24px 16px 56px;"
        "    border: none;"
        "    border-radius: 8px;"  // Rounded corners
        "    background-color: transparent;"
        "    color: rgba(255, 255, 255, 0.8);"
        "    font-size: 15px;"
        "    font-weight: 500;"
        "    margin: 4px 12px;"  // Margins for hover effect
        "    width: 256px;"
        "    background-position: 24px center;"
        "    background-repeat: no-repeat;"
        "    background-size: 20px 20px;"  // Slightly larger icons
        "    transition: all 0.2s ease;"  // Smooth hover transition
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "    color: white;"
        "    transform: translateX(2px);"
        "}"
        "QPushButton:checked {"
        "    background-color: rgba(59, 130, 246, 0.5);"  // blue when selected
        "    color: white;"
        "    border-left: 4px solid #3B82F6;"
        "}";

    // Enhanced navigation button setup
    QWidget* sidebarContent = new QWidget(ui->sidebar);
    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarContent);
    sidebarLayout->setSpacing(2);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);

    // Configure navigation buttons
    auto setupNavButton = [&](QPushButton* button, const QString& text, const QString& iconPath) {
        button->setText(text);
        button->setCheckable(true);
        button->setFixedHeight(56);
        button->setStyleSheet(sidebarButtonStyle + QString("QPushButton { background-image: url(%1); }").arg(iconPath));
    };

    if (m_userMode == "manager") {
        setupNavButton(ui->Dashboard, "All Reservations", ":/icons/calendar.svg");
        setupNavButton(ui->Orders, "Assign a Table", ":/icons/book-table.svg");
    }
    else {
        setupNavButton(ui->Dashboard, "My Reservations", ":/icons/calendar.svg");
        setupNavButton(ui->Orders, "Book a Table", ":/icons/book-table.svg");
    }
    setupNavButton(ui->Menu, "View Walk-In Wait Times", ":/icons/menu.svg");
    setupNavButton(ui->Locations, "Contact Us", ":/icons/contact.svg");

    // Enhanced info section
    QWidget* infoWidget = new QWidget;
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(24, 24, 24, 24);
    infoLayout->setSpacing(16);

    // Styled info header
    QLabel* infoHeader = new QLabel("My Information");
    infoHeader->setStyleSheet(
        "color: rgba(255, 255, 255, 0.9);"
        "font-size: 15px;"
        "font-weight: 600;"
        "letter-spacing: 0.5px;"
        "margin-bottom: 12px;");
    infoLayout->addWidget(infoHeader);

    // Info items with improved styling
    auto addInfo = [&](const QString& label, const QString& value) {
        QHBoxLayout* infoLayout = new QHBoxLayout;
        QLabel* labelWidget = new QLabel(label);
        QLabel* valueWidget = new QLabel(value);
        labelWidget->setStyleSheet(
            "color: rgba(255, 255, 255, 0.6);"
            "font-size: 14px;"
            "letter-spacing: 0.3px;");
        valueWidget->setStyleSheet(
            "color: white;"
            "font-size: 14px;"
            "font-weight: 500;");
        infoLayout->addWidget(labelWidget);
        infoLayout->addStretch();
        infoLayout->addWidget(valueWidget);
        return infoLayout;
    };

    infoLayout->addLayout(addInfo("Phone:", "(555) 123-4567"));
    infoLayout->addLayout(addInfo("Upcoming:", "2 Reservations"));
    infoLayout->addLayout(addInfo("Member Since:", "Jan 2024"));

    // Enhanced special offers section
    QLabel* offersLabel = new QLabel("Special Offers");
    offersLabel->setStyleSheet(
        "color: #10B981;"  // Modern green
        "font-size: 15px;"
        "font-weight: 600;"
        "letter-spacing: 0.5px;"
        "margin-top: 24px;"
        "margin-bottom: 8px;");
    infoLayout->addWidget(offersLabel);

    QLabel* offerDetails = new QLabel("• 10% off VIP tables\n• Free dessert on birthdays");
    offerDetails->setStyleSheet(
        "color: rgba(255, 255, 255, 0.7);"
        "font-size: 14px;"
        "line-height: 1.6;"
        "padding-left: 8px;");
    infoLayout->addWidget(offerDetails);

    // Modern logout button style
    QString logoutStyle =
        "QPushButton {"
        "    text-align: left;"
        "    padding: 16px 24px 16px 56px;"
        "    border: none;"
        "    border-radius: 8px;"
        "    background-color: rgba(239, 68, 68, 0.1);"  // Modern red
        "    color: #EF4444;"
        "    font-size: 15px;"
        "    font-weight: 600;"
        "    margin: 12px;"
        "    width: 256px;"
        "    background-image: url(:/icons/logout.svg);"
        "    background-position: 24px center;"
        "    background-repeat: no-repeat;"
        "    background-size: 20px 20px;"
        "    transition: all 0.2s ease;"
        "}"
        "QPushButton:hover {"
        "    background-color: #EF4444;"
        "    color: white;"
        "    transform: translateX(2px);"
        "}";
    ui->Logout->setStyleSheet(logoutStyle);

    // Layout assembly
    sidebarLayout->addSpacing(100);  // Space for profile section
    sidebarLayout->addWidget(ui->Dashboard);
    sidebarLayout->addWidget(ui->Orders);
    sidebarLayout->addWidget(ui->Menu);
    sidebarLayout->addWidget(ui->Locations);
    sidebarLayout->addWidget(infoWidget);
    sidebarLayout->addStretch(1);
    sidebarLayout->addWidget(ui->Logout);

    sidebarContent->setGeometry(0, 100, 280, ui->sidebar->height() - 100);

    connect(ui->Dashboard, &QPushButton::clicked, this, [this]() {
        ui->Dashboard->setChecked(true);
        ui->Orders->setChecked(false);
        ui->Menu->setChecked(false);
        ui->Locations->setChecked(false);
    });

    connect(ui->Orders, &QPushButton::clicked, this, [this]() {
        ui->Dashboard->setChecked(false);
        ui->Orders->setChecked(true);
        ui->Menu->setChecked(false);
        ui->Locations->setChecked(false);
    });

    //Hide elements
    ui->Table1_list->setVisible(false);
    ui->Reserve->setVisible(false);

    // Initialize additional UI components
    setupTableCapacityIndicators();
    setupLegend();
    setupFilters();
}

void Home::setupTables()
{
    // Initialize table information
    tables["Table1"] = TableInfo(4);
    tables["Table2"] = TableInfo(4);
    tables["Table3"] = TableInfo(4);
    tables["Table4"] = TableInfo(4);
    tables["Table5"] = TableInfo(4);
    tables["Table6"] = TableInfo(4);
    tables["Table7"] = TableInfo(4);
    tables["Table8"] = TableInfo(4);
    tables["Table9"] = TableInfo(4);
    tables["Table10"] = TableInfo(4);
    tables["Table11"] = TableInfo(4);
    tables["Table12"] = TableInfo(4);
    tables["Table13"] = TableInfo(8);
    tables["Table14"] = TableInfo(8);

    QString tableStyle = "QPushButton {"
                         "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                         "                               stop:0 #FFFFFF, stop:1 #F5F5F5);"
                         "    color: #2C3E50;"
                         "    border-radius: 12px;"
                         "    border: 1.5px solid #E0E0E0;"  // Lighter, subtle border
                         "    font-size: 15px;"
                         "    font-weight: 500;"  // Slightly bolder text
                         "    padding: 12px 20px;"  // padding for better proportions
                         "    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);"  // Subtle shadow for depth
                         "    transition: all 0.3s ease;"  // Smooth transition for all changes
                         "}"
                         "QPushButton:hover {"
                         "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                         "                               stop:0 #4CAF50, stop:1 #45A049);"
                         "    color: white;"
                         "    border-color: #388E3C;"
                         "    box-shadow: 0 4px 8px rgba(76, 175, 80, 0.2);"  // Green-tinted shadow on hover
                         "    transform: translateY(-1px);"  // Slight lift effect
                         "}"
                         "QPushButton:pressed {"
                         "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                         "                               stop:0 #388E3C, stop:1 #2E7D32);"
                         "    color: white;"
                         "    border-color: #2E7D32;"
                         "    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);"  // Reduced shadow when pressed
                         "    transform: translateY(1px);"  // Slight press effect
                         "}";

    for (const auto &tableName : tables.keys()) {
        QPushButton *tableButton = findChild<QPushButton *>(tableName);
        if (tableButton) {
            tableButton->setStyleSheet(tableStyle);
            connect(tableButton, &QPushButton::clicked, this, &Home::handleTableClick);
        }
    }

    // Load existing reservations and update appearances
    loadReservations();
    for (const auto &tableName : tables.keys()) {
        QPushButton *tableButton = findChild<QPushButton *>(tableName);
        if (tableButton) {
            updateTableAppearance(tableButton);
        }
    }
}

void Home::handleTableClick()
{
    QPushButton *clickedTable = qobject_cast<QPushButton *>(sender());
    if (!clickedTable)
        return;

    // Store the previous selected table
    QPushButton *prevTable = selectedTable;

    // Update the selected table
    selectedTable = clickedTable;

    // Update appearances for both previous and newly selected tables
    if (prevTable) {
        selectedTable = nullptr;  // Temporarily clear selection to get default appearance
        updateTableAppearance(prevTable);
        selectedTable = clickedTable;  // Restore selection
    }

    // Update appearance of newly clicked table
    updateTableAppearance(clickedTable);

    // Show reservation prompt
    showReservationPrompt(clickedTable);
}

void Home::updateTableAppearance(QPushButton *table)
{
    if (!table)
        return;

    const TableInfo &info = tables[table->objectName()];
    QString styleSheet;

    bool isReservedAtSelectedTime = false;
    if (!ui->Table1_list->currentText().isEmpty()) {
        QTime selectedTime = QTime::fromString(ui->Table1_list->currentText(), "hh:mm AP");
        QDateTime selectedDateTime = QDateTime(QDateTime::currentDateTime().date(), selectedTime);

        for (const QDateTime &reservedTime : info.reservedTimes) {
            if (reservedTime.time() == selectedTime) {
                isReservedAtSelectedTime = true;
                break;
            }
        }
    }

    if (isReservedAtSelectedTime) {
        // Reserved table style - Professional red theme
        styleSheet = "QPushButton {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #FF5252, stop:1 #F44336);"
                     "    color: white;"
                     "    border-radius: 12px;"
                     "    border: 1.5px solid #D32F2F;"
                     "    font-size: 15px;"
                     "    font-weight: 500;"
                     "    padding: 12px 20px;"
                     "    box-shadow: 0 2px 4px rgba(244, 67, 54, 0.2);"
                     "    transition: all 0.3s ease;"
                     "}"
                     "QPushButton:hover {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #FF6B6B, stop:1 #FF5252);"
                     "    box-shadow: 0 4px 8px rgba(244, 67, 54, 0.3);"
                     "}"
                     "QPushButton:pressed {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #D32F2F, stop:1 #C62828);"
                     "    box-shadow: 0 2px 4px rgba(244, 67, 54, 0.1);"
                     "}";
    } else if (table == selectedTable) {
        // Selected table style - Professional green theme
        styleSheet = "QPushButton {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #4CAF50, stop:1 #45A049);"
                     "    color: white;"
                     "    border-radius: 12px;"
                     "    border: 1.5px solid #388E3C;"
                     "    font-size: 15px;"
                     "    font-weight: 500;"
                     "    padding: 12px 20px;"
                     "    box-shadow: 0 2px 4px rgba(76, 175, 80, 0.2);"
                     "    transition: all 0.3s ease;"
                     "}"
                     "QPushButton:hover {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #66BB6A, stop:1 #4CAF50);"
                     "    box-shadow: 0 4px 8px rgba(76, 175, 80, 0.3);"
                     "}"
                     "QPushButton:pressed {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #388E3C, stop:1 #2E7D32);"
                     "    box-shadow: 0 2px 4px rgba(76, 175, 80, 0.1);"
                     "}";
    } else {
        // Available table style - Professional white theme
        styleSheet = "QPushButton {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #FFFFFF, stop:1 #F5F5F5);"
                     "    color: #2C3E50;"
                     "    border-radius: 12px;"
                     "    border: 1.5px solid #E0E0E0;"
                     "    font-size: 15px;"
                     "    font-weight: 500;"
                     "    padding: 12px 20px;"
                     "    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);"
                     "    transition: all 0.3s ease;"
                     "}"
                     "QPushButton:hover {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #4CAF50, stop:1 #45A049);"
                     "    color: white;"
                     "    border-color: #388E3C;"
                     "    box-shadow: 0 4px 8px rgba(76, 175, 80, 0.2);"
                     "}"
                     "QPushButton:pressed {"
                     "    background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                     "                               stop:0 #388E3C, stop:1 #2E7D32);"
                     "    color: white;"
                     "    border-color: #2E7D32;"
                     "    box-shadow: 0 2px 4px rgba(76, 175, 80, 0.1);"
                     "}";
    }

    table->setStyleSheet(styleSheet);
}

void Home::showReservationPrompt(QPushButton *table)
{
    if (!table)
        return;

    const TableInfo &info = tables[table->objectName()];

    // Store the current selection and time if they exist
    QString currentTime;
    if (ui->Table1_list->isVisible()) {
        currentTime = ui->Table1_list->currentText();
    }

    // Create new right panel
    if (rightPanel) {
        // Before deleting, reparent the widgets we want to keep
        ui->Table1_list->setParent(this);
        ui->Reserve->setParent(this);
        delete rightPanel;
    }

    // Create container for right panel with adjusted dimensions
    rightPanel = new QWidget(this);
    rightPanel->setGeometry(1000, 140, 340, 640);
    rightPanel->setStyleSheet(
        "QWidget {"
        "    background-color: white;"
        "    border-radius: 8px;"
        "    border: 1px solid #E0E0E0;"
        "}");

    QVBoxLayout* panelLayout = new QVBoxLayout(rightPanel);
    panelLayout->setSpacing(12);
    panelLayout->setContentsMargins(20, 20, 20, 20);

    // Table Header Section
    QLabel* tableHeader = new QLabel(QString("Table %1").arg(table->objectName().right(table->objectName().length() - 5)));
    tableHeader->setStyleSheet(
        "QLabel {"
        "    color: #1B4965;"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    border: none;"
        "    padding-bottom: 8px;"
        "}");
    panelLayout->addWidget(tableHeader);

    // Table Details Section
    QWidget* detailsWidget = new QWidget;
    QGridLayout* detailsLayout = new QGridLayout(detailsWidget);
    detailsLayout->setSpacing(8);
    detailsLayout->setContentsMargins(0, 0, 0, 8);

    auto addDetail = [&](const QString& label, const QString& value, int row) {
        QLabel* labelWidget = new QLabel(label);
        QLabel* valueWidget = new QLabel(value);
        labelWidget->setStyleSheet("color: #666; font-size: 14px;");
        valueWidget->setStyleSheet("color: #2C3E50; font-size: 14px;");
        labelWidget->setFixedWidth(80);
        detailsLayout->addWidget(labelWidget, row, 0);
        detailsLayout->addWidget(valueWidget, row, 1);
    };

    addDetail("Capacity:", QString("%1 seats").arg(info.seats), 0);
    addDetail("Type:", info.isVIP ? "VIP" : "Standard", 1);
    if (info.isVIP) {
        addDetail("Min. Spend:", QString("$%1").arg(info.minSpend), 2);
    }

    panelLayout->addWidget(detailsWidget);

    // Time Selection Section
    QLabel* timeLabel = new QLabel("Select Time");
    timeLabel->setStyleSheet(
        "color: #1B4965;"
        "font-size: 16px;"
        "font-weight: bold;"
        "margin-top: 8px;");
    panelLayout->addWidget(timeLabel);

    // Setup time selector
    if (ui->Table1_list->count() == 0) {
        populateTimeSlots();
    }
    ui->Table1_list->setParent(rightPanel);
    ui->Table1_list->setFixedHeight(40);
    ui->Table1_list->setStyleSheet(
        "QComboBox {"
        "    background-color: white;"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    font-size: 14px;"
        "    color: #2C3E50;"
        "}"
        "QComboBox:hover {"
        "    border-color: #1B4965;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "}");
    panelLayout->addWidget(ui->Table1_list);

    // Restore previous time selection if it exists
    if (!currentTime.isEmpty()) {
        int index = ui->Table1_list->findText(currentTime);
        if (index >= 0) {
            ui->Table1_list->setCurrentIndex(index);
        }
    }

    // Special Requests Section
    QLabel* specialLabel = new QLabel("Special Requests");
    specialLabel->setStyleSheet(
        "color: #1B4965;"
        "font-size: 16px;"
        "font-weight: bold;"
        "margin-top: 16px;");
    panelLayout->addWidget(specialLabel);

    specialRequests = new QTextEdit(rightPanel);
    specialRequests->setStyleSheet(
        "QTextEdit {"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    font-size: 14px;"
        "    color: #2C3E50;"
        "    background-color: white;"
        "}"
        "QTextEdit:focus {"
        "    border-color: #1B4965;"
        "}");
    specialRequests->setPlaceholderText("Enter any special requests or notes...");
    specialRequests->setFixedHeight(80);
    panelLayout->addWidget(specialRequests);

    // Contact Information Section
    QLabel* contactLabel = new QLabel("Contact Information");
    contactLabel->setStyleSheet(
        "color: #1B4965;"
        "font-size: 16px;"
        "font-weight: bold;"
        "margin-top: 16px;");
    panelLayout->addWidget(contactLabel);

    nameInput = new QLineEdit(rightPanel);
    nameInput->setPlaceholderText("Name");
    nameInput->setFixedHeight(40);
    nameInput->setStyleSheet(
        "QLineEdit {"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    font-size: 14px;"
        "    color: #2C3E50;"
        "    margin-bottom: 8px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #1B4965;"
        "}");
    panelLayout->addWidget(nameInput);

    phoneInput = new QLineEdit(rightPanel);
    phoneInput->setPlaceholderText("Phone Number");
    phoneInput->setFixedHeight(40);
    phoneInput->setStyleSheet(nameInput->styleSheet());
    panelLayout->addWidget(phoneInput);

    // Reserve Button
    ui->Reserve->setParent(rightPanel);
    ui->Reserve->setFixedHeight(45);
    ui->Reserve->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border-radius: 6px;"
        "    width: 150px;"
        "    height: 90px;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    border: none;"
        "    margin-top: 20px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45A049;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3D8B40;"
        "}"
        );
    ui->Reserve->setCursor(Qt::PointingHandCursor);
    panelLayout->addWidget(ui->Reserve);

    // Add stretch to push everything up
    panelLayout->addStretch();

    // Make everything visible
    rightPanel->show();
    ui->Table1_list->setVisible(true);
    ui->Reserve->setVisible(true);
}





void Home::populateTimeSlots()
{
    ui->Table1_list->clear();

    QDateTime currentTime = QDateTime::currentDateTime();
    for (int hour = 11; hour <= 22; hour++) { // Restaurant hours: 11 AM to 10 PM
        for (int minute = 0; minute < 60; minute += 30) {
            QDateTime slotTime = QDateTime(currentTime.date(), QTime(hour, minute));
            if (slotTime > currentTime) {
                QString timeStr = slotTime.toString("hh:mm AP");
                ui->Table1_list->addItem(timeStr);
            }
        }
    }

    // Update all table appearances after populating time slots
    for (const auto &tableName : tables.keys()) {
        QPushButton *tableButton = findChild<QPushButton *>(tableName);
        if (tableButton) {
            updateTableAppearance(tableButton);
        }
    }
}









void Home::on_Reserve_clicked()
{
    if (!selectedTable || ui->Table1_list->currentText().isEmpty()) {
        QMessageBox::warning(this, "Reservation Error", "Please select a table and time slot.");
        return;
    }

    QString tableId = selectedTable->objectName();
    QDateTime currentTime = QDateTime::currentDateTime();
    QTime selectedTime = QTime::fromString(ui->Table1_list->currentText(), "hh:mm AP");
    QDateTime reservationTime = QDateTime(currentTime.date(), selectedTime);

    // Check if the time is already reserved
    //change this to check db times
    bool timeAlreadyReserved = false;
    for (const QDateTime &existingTime : tables[tableId].reservedTimes) {
        if (existingTime.date() == reservationTime.date() &&
            existingTime.time() == reservationTime.time()) {
            timeAlreadyReserved = true;
            break;
        }
    }
    //change to check db times
    if (timeAlreadyReserved) {
        QMessageBox::warning(this, "Reservation Error", "This time slot is already reserved.");
        return;
    }

    // Add the new reservation
    //make query to add to db
    tables[tableId].isReserved = true;
    tables[tableId].reservationTime = reservationTime;
    tables[tableId].reservedTimes.append(reservationTime);

    // Save the reservation to the database
    if (!saveReservationToDatabase(tableId, reservationTime, currentUser)) {
        QMessageBox::warning(this, "Database Error", "Failed to save reservation to the database.");
        return;
    }

    saveReservations();
    updateTableAppearance(selectedTable);
    populateTimeSlots(); // Refresh the available time slots

    QMessageBox::information(this, "Reservation Confirmed", "Reservation successfully made.");
}


bool Home::saveReservationToDatabase(const QString &tableId, const QDateTime &reservationTime, const QString &username)
{
    // Ensure database connection
    QSqlDatabase logindb = QSqlDatabase::database();

    // Prepare and execute the SQL query
    QSqlQuery query(logindb);
    query.prepare("INSERT INTO reservations (table_id, reservation_time, username) "
                  "VALUES (:table_id, :reservation_time, :username)");
    query.bindValue(":table_id", tableId);
    query.bindValue(":reservation_time", reservationTime.toString(Qt::ISODate));
    query.bindValue(":username", username);

    if (!query.exec()) {
        qDebug() << "Failed to insert reservation:" << query.lastError().text();
        return false;
    }

    return true;
}












void Home::on_Menu_clicked()
{
    showWalkinPage();
}
void Home::on_Locations_clicked()
{
    showContactPage();
}


void Home::on_Logout_clicked()
{
    QApplication::quit(); // Exit the application
}

void Home::resetTableStyles()
{
    for (auto it = tables.begin(); it != tables.end(); ++it) {
        QPushButton *tableButton = findChild<QPushButton *>(it.key());
        if (tableButton) {
            tableButton->setStyleSheet("background-color: white; border: 2px solid grey;");
        }
    }
}


void Home::saveReservations()
{
    QFile file("reservations.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject rootObj;
        for (auto it = tables.begin(); it != tables.end(); ++it) {
            QJsonObject tableObj;
            tableObj["seats"] = it.value().seats;
            tableObj["isReserved"] = it.value().isReserved;
            tableObj["reservationTime"] = it.value().reservationTime.toString(Qt::ISODate);
            tableObj["customerName"] = it.value().customerName;

            // Save the list of reserved times
            QJsonArray reservedTimesArray;
            for (const QDateTime &time : it.value().reservedTimes) {
                reservedTimesArray.append(time.toString(Qt::ISODate));
            }
            tableObj["reservedTimes"] = reservedTimesArray;

            rootObj[it.key()] = tableObj;
        }

        QJsonDocument doc(rootObj);
        file.write(doc.toJson());
    }
}
//--------

void Home::setupConnections()
{
    // Connect UI actions (button clicks) to the corresponding slots
    connect(ui->Logout, &QPushButton::clicked, this, &Home::on_Logout_clicked);
    connect(ui->Reserve, &QPushButton::clicked, this, &Home::on_Reserve_clicked);
    connect(ui->Dashboard, &QPushButton::clicked, this, &Home::on_Dashboard_clicked);
    connect(ui->Orders, &QPushButton::clicked, this, &Home::on_Orders_clicked);
    connect(ui->Menu, &QPushButton::clicked, this, &Home::on_Menu_clicked);
    connect(ui->Locations, &QPushButton::clicked, this, &Home::on_Locations_clicked);
    connect(ui->Table1_list, &QComboBox::currentTextChanged, this, &Home::on_Table1_list_currentTextChanged);

}

void Home::on_FloorPlan_clicked()
{
    // Implement the action when the "Floor Plan" button is clicked
    qDebug() << "Floor Plan button clicked";

}

void Home::on_ReservationList_clicked()
{
    // Implement the action when the "Reservation List" button is clicked
    qDebug() << "Reservation List button clicked";
}

//-----------------------------

void Home::loadReservations()
{
    QFile file("reservations.json");
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();

        for (const QString &tableId : root.keys()) {
            QJsonObject tableObj = root[tableId].toObject();
            TableInfo table(tableObj["seats"].toInt());
            table.isReserved = tableObj["isReserved"].toBool();
            table.reservationTime = QDateTime::fromString(
                tableObj["reservationTime"].toString(), Qt::ISODate);
            table.customerName = tableObj["customerName"].toString();

            // Load the reserved times
            QJsonArray reservedTimesArray = tableObj["reservedTimes"].toArray();
            for (const QJsonValue &timeValue : reservedTimesArray) {
                table.reservedTimes.append(
                    QDateTime::fromString(timeValue.toString(), Qt::ISODate));
            }

            tables[tableId] = table;
        }
    }
}


bool Home::isTableAvailable(const QString &tableId, const QDateTime &requestedTime)
{
    if (tables.contains(tableId)) {
        const TableInfo &table = tables[tableId];
        return !table.isReserved || table.reservationTime != requestedTime;
    }
    return false;
}

void Home::updateTableStatus()
{
    // Check for expired reservations and mark them as available
    for (auto it = tables.begin(); it != tables.end(); ++it) {
        if (it.value().isReserved && it.value().reservationTime <= QDateTime::currentDateTime()) {
            it.value().isReserved = false; // Mark as available if reservation time has passed
        }
    }
}



bool Home::reserveTable(int tableNumber,
                        const std::string &name,
                        const std::string &date,
                        int guests)
{
    if (tableStatus[tableNumber]) {
        std::cout << "Error: Table " << tableNumber << " is already reserved." << std::endl;
        return false;
    }
    tableStatus[tableNumber] = true;
    reservationDetails[tableNumber] = "Reserved by " + name + " for " + date + " with "
                                      + std::to_string(guests) + " guests.";
    return true;
}

bool Home::cancelReservation(int tableNumber)
{
    if (!tableStatus[tableNumber]) {
        std::cout << "Error: Table " << tableNumber << " is already available." << std::endl;
        return false;
    }
    tableStatus[tableNumber] = false;
    reservationDetails.erase(tableNumber);
    return true;
}



bool Home::checkAvailability(int tableNumber)
{
    return !tableStatus[tableNumber];
}

std::string Home::displayTableStatus()
{
    std::string status = "Table Status:\n";
    for (int i = 1; i <= totalTables; i++) {
        status += "Table " + std::to_string(i) + ": ";
        if (tableStatus[i]) {
            status += "Reserved - " + reservationDetails[i] + "\n";
        } else {
            status += "Available\n";
        }
    }
    return status;
}



void Home::on_Table1_list_currentTextChanged(const QString &text)
{
    // Update the appearance of all tables when a new time is selected
    for (const auto &tableName : tables.keys()) {
        QPushButton *tableButton = findChild<QPushButton *>(tableName);
        if (tableButton) {
            updateTableAppearance(tableButton);
        }
    }
}


void Home::setupLegend()
{
    QWidget* legend = new QWidget(this);
    legend->setObjectName("legend");  // Add this line
    QHBoxLayout* legendLayout = new QHBoxLayout(legend);

    // Add legend items
    auto addLegendItem = [&](const QString& text, const QString& color) {
        QWidget* item = new QWidget;
        QHBoxLayout* itemLayout = new QHBoxLayout(item);

        QLabel* dot = new QLabel;
        dot->setFixedSize(12, 12);
        dot->setStyleSheet(QString("background-color: %1; border-radius: 6px;").arg(color));

        QLabel* label = new QLabel(text);
        label->setStyleSheet("color: #495057; font-size: 13px;");

        itemLayout->addWidget(dot);
        itemLayout->addWidget(label);
        legendLayout->addWidget(item);
    };

    addLegendItem("Available", "#FFFFFF");
    addLegendItem("Selected", "#4CAF50");
    addLegendItem("Reserved", "#FF4444");
    addLegendItem("VIP", "#FFD700");
}

void Home::setupFilters()
{
    QWidget* filterBar = new QWidget(this);
    filterBar->setGeometry(700, 70, 500, 40);
    QHBoxLayout* filterLayout = new QHBoxLayout(filterBar);
    filterLayout->setSpacing(10);
    filterLayout->setContentsMargins(0, 0, 0, 0);

    QString filterStyle =
        "QComboBox {"
        "    background-color: white;"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 5px 10px;"
        "    min-width: 120px;"
        "    color: #2C3E50;"
        "    font-size: 13px;"
        "}"
        "QComboBox:hover {"
        "    border-color: #1B4965;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    padding-right: 10px;"
        "}"
        "QLabel {"
        "    color: #2C3E50;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "}";

    // Initialize filters
    timeFilter = new QComboBox(filterBar);
    timeFilter->addItems({"All Times", "Lunch (11:00-15:00)", "Dinner (17:00-22:00)"});
    timeFilter->setStyleSheet(filterStyle);

    capacityFilter = new QComboBox(filterBar);
    capacityFilter->addItems({"Any Capacity", "2-4 Guests", "5-8 Guests", "8+ Guests"});
    capacityFilter->setStyleSheet(filterStyle);

    // Create and style labels
    QLabel* timeLabel = new QLabel("Time:");
    QLabel* capacityLabel = new QLabel("Capacity:");
    timeLabel->setStyleSheet(filterStyle);
    capacityLabel->setStyleSheet(filterStyle);

    // Add widgets to layout
    filterLayout->addWidget(timeLabel);
    filterLayout->addWidget(timeFilter);
    filterLayout->addSpacing(20);
    filterLayout->addWidget(capacityLabel);
    filterLayout->addWidget(capacityFilter);
    filterLayout->addStretch();

    // Connect signals
    connect(timeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Home::onFilterChanged);
    connect(capacityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Home::onFilterChanged);
}

void Home::setupQuickStats()
{
    QWidget* statsBar = new QWidget(this);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsBar);

    auto createStatWidget = [](const QString& label, const QString& value) {
        QWidget* widget = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(widget);

        QLabel* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #1B4965;");

        QLabel* textLabel = new QLabel(label);
        textLabel->setStyleSheet("font-size: 14px; color: #6C757D;");

        layout->addWidget(valueLabel);
        layout->addWidget(textLabel);
        return widget;
    };

    statsLayout->addWidget(createStatWidget("Available Tables", "8"));
    statsLayout->addWidget(createStatWidget("Reserved Today", "6"));
    statsLayout->addWidget(createStatWidget("Waiting List", "3"));
    statsLayout->addWidget(createStatWidget("Total Capacity", "56"));
}

void Home::showTableDetails(QPushButton* table)
{
    const TableInfo& info = tables[table->objectName()];

    QDialog* details = new QDialog(this);
    details->setWindowTitle("Table Details");
    details->setStyleSheet(
        "QDialog {"
        "    background-color: white;"
        "    border-radius: 8px;"
        "}"
        "QLabel {"
        "    color: #2C3E50;"
        "    font-size: 14px;"
        "}"
        "QPushButton {"
        "    background-color: #1B4965;"
        "    color: white;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-size: 14px;"
        "    border: none;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2C5F7C;"
        "}");

    QVBoxLayout* layout = new QVBoxLayout(details);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);

    // Add table information
    auto addDetail = [&](const QString& label, const QString& value) {
        QHBoxLayout* row = new QHBoxLayout;
        QLabel* labelWidget = new QLabel(label);
        QLabel* valueWidget = new QLabel(value);
        labelWidget->setStyleSheet("font-weight: bold; min-width: 120px;");
        row->addWidget(labelWidget);
        row->addWidget(valueWidget);
        layout->addLayout(row);
    };

    addDetail("Table Number:", table->objectName());
    addDetail("Seats:", QString::number(info.seats));
    addDetail("Status:", info.isReserved ? "Reserved" : "Available");
    addDetail("Section:", info.isVIP ? "VIP" : "Regular");

    if (info.isReserved) {
        addDetail("Reserved For:", info.customerName);
        addDetail("Time:", info.reservationTime.toString("hh:mm AP"));
    }

    if (!info.specialNotes.isEmpty()) {
        addDetail("Special Notes:", info.specialNotes);
    }

    if (info.isVIP) {
        addDetail("Minimum Spend:", QString("$%1").arg(info.minSpend, 0, 'f', 2));
    }

    // Add some spacing
    layout->addSpacing(10);

    // Add close button
    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setCursor(Qt::PointingHandCursor);
    connect(closeButton, &QPushButton::clicked, details, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);

    details->setFixedSize(400, layout->sizeHint().height() + 40);
    details->exec();
}

void Home::setupTableCapacityIndicators()
{
    QString tableStyle =
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "                               stop:0 #FFFFFF, stop:1 #F5F5F5);"
        "    color: #2C3E50;"
        "    border: 2px solid #E0E0E0;"
        "    border-radius: 12px;"
        "    font-size: 14px;"
        "    padding: 10px;"
        "    text-align: center;"
        "    min-height: 80px;"
        "}"
        "QPushButton:hover {"
        "    border-color: #1B4965;"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "                               stop:0 #F8F9FA, stop:1 #E9ECEF);"
        "}"
        "QPushButton[isVIP=\"true\"] {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "                               stop:0 #FFF8E1, stop:1 #FFF8DC);"
        "    border: 2px solid #FFD700;"
        "}"
        "QPushButton QLabel {"
        "    font-size: 12px;"
        "    color: #666;"
        "    margin-top: 4px;"
        "}";

    for (int i = 1; i <= 14; i++) {
        QString tableName = QString("Table%1").arg(i);
        QPushButton *tableBtn = findChild<QPushButton *>(tableName);
        if (tableBtn) {
            int capacity = (i >= 13) ? 8 : 4;
            tables[tableName] = TableInfo(capacity);

            // Create a more detailed table display
            QString displayText = QString("Table %1\n%2 seats\n%3")
                                      .arg(i)
                                      .arg(capacity)
                                      .arg(capacity >= 8 ? "VIP" : "Standard");

            tableBtn->setText(displayText);
            tableBtn->setStyleSheet(tableStyle);

            if (capacity >= 8) {
                tableBtn->setProperty("isVIP", true);
                tableBtn->style()->unpolish(tableBtn);
                tableBtn->style()->polish(tableBtn);
            }
        }
    }
}


void Home::onFilterChanged()
{
    if (!timeFilter || !capacityFilter)
        return;

    QString selectedTimeRange = timeFilter->currentText();
    QString selectedCapacity = capacityFilter->currentText();

    for (const auto &tableName : tables.keys()) {
        QPushButton *tableButton = findChild<QPushButton *>(tableName);
        if (!tableButton)
            continue;

        const TableInfo &info = tables[tableName];
        bool showTable = true;

        // Apply capacity filter
        if (selectedCapacity != "Any Capacity") {
            if (selectedCapacity == "2-4 Guests" && info.seats > 4) {
                showTable = false;
            } else if (selectedCapacity == "5-8 Guests" && (info.seats < 5 || info.seats > 8)) {
                showTable = false;
            } else if (selectedCapacity == "8+ Guests" && info.seats < 8) {
                showTable = false;
            }
        }

        // Apply time filter for reserved tables
        if (showTable && selectedTimeRange != "All Times" && info.isReserved) {
            QTime reservationTime = info.reservationTime.time();

            if (selectedTimeRange == "Lunch (11:00-15:00)") {
                if (reservationTime < QTime(11, 0) || reservationTime > QTime(15, 0)) {
                    showTable = false;
                }
            } else if (selectedTimeRange == "Dinner (17:00-22:00)") {
                if (reservationTime < QTime(17, 0) || reservationTime > QTime(22, 0)) {
                    showTable = false;
                }
            }
        }

        // Update table visibility
        tableButton->setVisible(showTable);
        if (showTable) {
            updateTableAppearance(tableButton);
        }
    }
}

void Home::setupReservationsPage()
{
    // Create main reservations page widget
    reservationsPage = new QWidget(this);
    reservationsPage->setGeometry(300, 90, 980, 800);
    reservationsPage->setStyleSheet("background-color: #F8F9FA;");

    // Main Layout
    QVBoxLayout* mainLayout = new QVBoxLayout(reservationsPage);
    mainLayout->setSpacing(24);
    mainLayout->setContentsMargins(40, 32, 40, 32);

    // Header Section
    QWidget* headerSection = new QWidget;
    QHBoxLayout* headerLayout = new QHBoxLayout(headerSection);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    // Title and Greeting
    QVBoxLayout* titleLayout = new QVBoxLayout;
    QLabel* titleLabel = new QLabel("My Reservations", headerSection);
    titleLabel->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #1B4965;");

    QString greeting = QTime::currentTime().hour() < 12 ? "Good morning" :
                           QTime::currentTime().hour() < 17 ? "Good afternoon" : "Good evening";
    QLabel* greetingLabel = new QLabel(QString("%1, customer").arg(greeting), headerSection);
    greetingLabel->setStyleSheet(
        "font-size: 16px;"
        "color: #6C757D;");

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(greetingLabel);
    titleLayout->setSpacing(4);

    // Date
    QLabel* dateLabel = new QLabel(QDate::currentDate().toString("dddd, MMMM d, yyyy"), headerSection);
    dateLabel->setStyleSheet(
        "font-size: 16px;"
        "color: #6C757D;"
        "background-color: white;"
        "border: 1px solid #E0E0E0;"
        "border-radius: 8px;"
        "padding: 8px 16px;");

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(dateLabel);

    mainLayout->addWidget(headerSection);

    // Stats Section
    QWidget* statsSection = new QWidget(reservationsPage);
    QHBoxLayout* statsLayout = new QHBoxLayout(statsSection);
    statsLayout->setSpacing(20);
    statsLayout->setContentsMargins(0, 0, 0, 0);

    // Function to create stat cards
    auto createStatCard = [](const QString& icon, const QString& value, const QString& label) -> QWidget* {
        QWidget* card = new QWidget;
        card->setStyleSheet(
            "QWidget {"
            "    background-color: white;"
            "    border: 1px solid #E0E0E0;"
            "    border-radius: 12px;"
            "}");

        QHBoxLayout* cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);
        cardLayout->setSpacing(16);

        // Icon
        QLabel* iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(40, 40);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(
            "background-color: #E3F2FD;"
            "border-radius: 20px;"
            "font-size: 18px;");

        QVBoxLayout* textLayout = new QVBoxLayout;

        QLabel* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(
            "font-size: 24px;"
            "font-weight: bold;"
            "color: #1B4965;");

        QLabel* labelLabel = new QLabel(label);
        labelLabel->setStyleSheet(
            "font-size: 14px;"
            "color: #6C757D;");

        textLayout->addWidget(valueLabel);
        textLayout->addWidget(labelLabel);
        textLayout->setSpacing(4);

        cardLayout->addWidget(iconLabel);
        cardLayout->addLayout(textLayout);
        cardLayout->addStretch();

        return card;
    };

    // Create stats cards
    if (m_userMode == "manager") {
        statsLayout->addWidget(createStatCard("🗓", QString::number(calculateActiveReservations()), "Active Reservations"));
        statsLayout->addWidget(createStatCard("⭐", QString::number(countVIPReservations()), "VIP Bookings"));
        statsLayout->addWidget(createStatCard("💰", QString("$%1").arg(calculateDailyRevenue()), "Total Revenue"));
    }

    mainLayout->addWidget(statsSection);

    // Divider
    QFrame* divider = new QFrame(reservationsPage);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("background-color: #E0E0E0; border: none;");
    mainLayout->addWidget(divider);

    // Upcoming Reservations Label
    QLabel* upcomingLabel = new QLabel("Upcoming Reservations", reservationsPage);
    upcomingLabel->setStyleSheet(
        "font-size: 20px;"
        "font-weight: bold;"
        "color: #1B4965;");
    mainLayout->addWidget(upcomingLabel);

    // Scrollable Reservations Area
    QScrollArea* scrollArea = new QScrollArea(reservationsPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    QWidget* scrollContent = new QWidget;
    QVBoxLayout* reservationsLayout = new QVBoxLayout(scrollContent);
    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(scrollArea);

    // Load initial reservations
    loadUserReservations();

    // Setup the reservation controls (search, filters, etc.)
    setupReservationControls();

    reservationsPage->show();
}

void Home::setupBookingPage()
{

    bookingPage = new QWidget(this);
    bookingPage->setGeometry(180 ,60, 1200, 720);
    //bookingPage->setStyleSheet("background-color: transparent;");


    QVBoxLayout* layout = new QVBoxLayout(bookingPage);


    // Move existing table buttons to this page
    QList<QPushButton*> tableButtons = findChildren<QPushButton*>(QRegularExpression("Table\\d+"));
    foreach(QPushButton* button, tableButtons) {
        button->setParent(bookingPage);
    }

    // Initially hide the booking page
    bookingPage->hide();
}

void Home::setupContactPage()
{
    contactPage = new QWidget(this);
    // Adjust geometry to match the screenshot exactly
    contactPage->setGeometry(300, 90, 900, 740);
    contactPage->setStyleSheet("background-color: #F8F9FA;");

    QVBoxLayout* layout = new QVBoxLayout(contactPage);
    layout->setSpacing(16);
    layout->setContentsMargins(40, 40, 40, 40);

    // Title
    QLabel* titleLabel = new QLabel("Contact Us", contactPage);
    titleLabel->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #1B4965;"
        );

    // Subtitle
    QLabel* subtitleLabel = new QLabel("We'd love to hear from you", contactPage);
    subtitleLabel->setStyleSheet(
        "font-size: 16px;"
        "color: #6C757D;"
        "margin-bottom: 24px;"
        );

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);

    // Card Container
    QWidget* card = new QWidget(contactPage);
    card->setStyleSheet(
        "QWidget {"
        "    background-color: white;"
        "    border: 1px solid #E0E0E0;"
        "    border-radius: 12px;"
        "}"
        );
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(20);
    cardLayout->setContentsMargins(24, 24, 24, 24);

    // Contact Items
    auto createContactItem = [](const QString& icon, const QString& value) -> QWidget* {
        QWidget* item = new QWidget;
        QHBoxLayout* itemLayout = new QHBoxLayout(item);
        itemLayout->setSpacing(12);
        itemLayout->setContentsMargins(8, 8, 8, 8);

        // Icon in light blue circle
        QLabel* iconLabel = new QLabel(icon);
        iconLabel->setFixedSize(32, 32);
        iconLabel->setStyleSheet(
            "background-color: #E3F2FD;"
            "border-radius: 16px;"
            "color: #1B4965;"
            "qproperty-alignment: AlignCenter;"
            );

        // Value
        QLabel* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet(
            "color: #2C3E50;"
            "font-size: 15px;"
            );

        // Copy button
        QPushButton* copyBtn = new QPushButton("Copy");
        copyBtn->setFixedWidth(60);
        copyBtn->setCursor(Qt::PointingHandCursor);
        copyBtn->setStyleSheet(
            "QPushButton {"
            "    color: #1B4965;"
            "    border: none;"
            "    padding: 6px;"
            "    font-size: 13px;"
            "    background: none;"
            "}"
            "QPushButton:hover {"
            "    text-decoration: underline;"
            "}"
            );

        itemLayout->addWidget(iconLabel);
        itemLayout->addWidget(valueLabel, 1);
        itemLayout->addWidget(copyBtn);

        QObject::connect(copyBtn, &QPushButton::clicked, [value]() {
            QClipboard* clipboard = QApplication::clipboard();
            clipboard->setText(value);
        });

        return item;
    };

    cardLayout->addWidget(createContactItem("📧", "21cap15@queensu.ca"));
    cardLayout->addWidget(createContactItem("📱", "(555) 123-4567"));
    cardLayout->addWidget(createContactItem("🕒", "Monday - Sunday: 11:00 AM - 10:00 PM"));

    // Message Section
    /*
    QLabel* messageLabel = new QLabel("Send us a Message");
    messageLabel->setStyleSheet(
        "font-size: 18px;"
        "font-weight: 600;"
        "color: #1B4965;"
        "margin-top: 20px;"
        );
    cardLayout->addWidget(messageLabel);
     */

    // Form inputs
    QString inputStyle =
        "QLineEdit, QTextEdit {"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 8px;"
        "    padding: 12px;"
        "    font-size: 14px;"
        "    color: #2C3E50;"
        "    background: white;"
        "}";

    // Name Input
    /*
    QLineEdit* nameInput = new QLineEdit;
    nameInput->setPlaceholderText("Your Name");
    nameInput->setStyleSheet(inputStyle);
    nameInput->setFixedHeight(45);
    cardLayout->addWidget(nameInput);
    */

    /*
    // Email Input
    QLineEdit* emailInput = new QLineEdit;
    emailInput->setPlaceholderText("Your Email");
    emailInput->setStyleSheet(inputStyle);
    emailInput->setFixedHeight(45);
    cardLayout->addWidget(emailInput);


    // Message Input
    QTextEdit* messageInput = new QTextEdit;
    messageInput->setPlaceholderText("Your Message");
    messageInput->setStyleSheet(inputStyle);
    messageInput->setFixedHeight(150);
    cardLayout->addWidget(messageInput);

    // Button Container for right alignment
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    QPushButton* sendButton = new QPushButton("Send Message");
    sendButton->setCursor(Qt::PointingHandCursor);
    sendButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #1B4965;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 10px 20px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2C5F7C;"
        "}"
        );
    buttonLayout->addWidget(sendButton);
    cardLayout->addLayout(buttonLayout);

    */

    // Add card to main layout
    layout->addWidget(card);
    layout->addStretch();

    /*
    // Connect send button
    connect(sendButton, &QPushButton::clicked, this, [=]() {
        if (nameInput->text().isEmpty() || emailInput->text().isEmpty() || messageInput->toPlainText().isEmpty()) {
            QMessageBox::warning(this, "Missing Information", "Please fill in all fields.");
            return;
        }

        QMessageBox::information(this, "Message Sent", "Thank you for your message. We'll get back to you soon!");
        nameInput->clear();
        emailInput->clear();
        messageInput->clear();


    });

    */

    // Initially hide the page
    contactPage->hide();
}


void Home::setupWalkinPage() {
    walkinPage = new QWidget(this);
    walkinPage->setGeometry(300, 90, 900, 740);
    walkinPage->setStyleSheet("background-color: #F8F9FA;");

    QVBoxLayout* layout = new QVBoxLayout(walkinPage);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignTop);

    QLabel* titleLabel = new QLabel("Walk-In Wait Times", walkinPage);
    titleLabel->setStyleSheet(
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #1B4965;"
        "margin-bottom: 5px;"
        );
    layout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("We hope to see you shortly.", walkinPage);
    subtitleLabel->setStyleSheet(
        "font-size: 16px;"
        "color: #6C757D;"
        "margin-bottom: 15px;"
        );
    layout->addWidget(subtitleLabel);

    totalTablesLabel = new QLabel("0");
    reservedTablesLabel = new QLabel("0");
    big_waitTimesLabel = new QLabel("0");
    small_waitTimesLabel = new QLabel("0");

    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(12);
    addInfoRow(infoLayout, "Total number of tables:", &totalTablesLabel);
    addInfoRow(infoLayout, "Total number of currently reserved tables:", &reservedTablesLabel);
    addInfoRow(infoLayout, "Estimated wait time for a party of 1-4:", &small_waitTimesLabel);
    addInfoRow(infoLayout, "Estimated wait time for a party greater than 4:", &big_waitTimesLabel);
    layout->addLayout(infoLayout);

    walkinPage->hide();
}

void Home::addInfoRow(QVBoxLayout* layout, const QString& labelText, QLabel** valueLabel) {
    QHBoxLayout* rowLayout = new QHBoxLayout();

    QLabel* label = new QLabel(labelText);
    label->setStyleSheet(
        "font-size: 20px;"
        "color: #495057;"
        );

    *valueLabel = new QLabel("Loading...");
    (*valueLabel)->setStyleSheet(
        "font-size: 20px;"
        "color: #1B4965;"
        "font-weight: bold;"
        );

    rowLayout->addWidget(label);
    rowLayout->addWidget(*valueLabel);

    layout->addLayout(rowLayout);
}




void Home::showReservationsPage()
{
    if (reservationsPage && bookingPage) {
        // Hide the reservation prompt if it exists
        if (rightPanel) {
            rightPanel->hide();
        }

        // Reset selected table
        if (selectedTable) {
            selectedTable = nullptr;
            for (const auto &tableName : tables.keys()) {
                QPushButton *tableButton = findChild<QPushButton *>(tableName);
                if (tableButton) {
                    updateTableAppearance(tableButton);
                }
            }
        }

        // Hide legend items
        QWidget* legendWidget = findChild<QWidget*>("legend");
        if (legendWidget) legendWidget->hide();

        // Hide filters
        if (timeFilter) timeFilter->parentWidget()->hide();
        if (capacityFilter) capacityFilter->parentWidget()->hide();

        reservationsPage->show();
        bookingPage->hide();
        contactPage->hide();
        walkinPage->hide();
        ui->Dashboard->setChecked(true);
        ui->Orders->setChecked(false);
        ui->Menu->setChecked(false);
        ui->Locations->setChecked(false);
    }
}
void Home::showBookingPage()
{
    if (reservationsPage && bookingPage) {
        // Hide the reservation prompt if it exists
        if (rightPanel) {
            rightPanel->hide();
        }

        // Reset selected table
        if (selectedTable) {
            selectedTable = nullptr;
            for (const auto &tableName : tables.keys()) {
                QPushButton *tableButton = findChild<QPushButton *>(tableName);
                if (tableButton) {
                    updateTableAppearance(tableButton);
                }
            }
        }

        // Show legend items
        QWidget* legendWidget = findChild<QWidget*>("legend");
        if (legendWidget) legendWidget->show();

        // Show filters
        if (timeFilter) timeFilter->parentWidget()->show();
        if (capacityFilter) capacityFilter->parentWidget()->show();

        bookingPage->show();
        reservationsPage->hide();
        contactPage->hide();
        walkinPage->hide();
        ui->Dashboard->setChecked(false);
        ui->Orders->setChecked(true);
        ui->Menu->setChecked(false);
        ui->Locations->setChecked(false);
    }
}

void Home::showContactPage()
{
    std::cout << "Called show contact page" << std::endl;
    if (reservationsPage && bookingPage && contactPage) {
        // Hide the reservation prompt if it exists
        if (rightPanel) {
            rightPanel->hide();
        }

        // Hide legend items
        QWidget* legendWidget = findChild<QWidget*>("legend");
        if (legendWidget) legendWidget->hide();

        // Hide filters
        // Hide filters
        if (timeFilter) {
            timeFilter->parentWidget()->hide();
            std::cout << "Hiding time filter" << std::endl;
        }
        if (capacityFilter) {
            capacityFilter->parentWidget()->hide();
            std::cout << "Hiding capacity filter" << std::endl;
        }
        contactPage->update();

        // Hide unrelated pages
        reservationsPage->hide();
        bookingPage->hide();
        walkinPage->hide();

        // Show contact page
        contactPage->show();

        // Update button states
        ui->Dashboard->setChecked(false);
        ui->Orders->setChecked(false);
        ui->Menu->setChecked(false);
        ui->Locations->setChecked(true);
    }
}

void Home::showWalkinPage()
{
    std::cout << "Called show walkin page" << std::endl;
    if (reservationsPage && bookingPage && contactPage) {
        // Hide the reservation prompt if it exists
        if (rightPanel) {
            rightPanel->hide();
        }

        // Hide legend items
        QWidget* legendWidget = findChild<QWidget*>("legend");
        if (legendWidget) legendWidget->hide();

        // Hide filters
        if (timeFilter) {
            timeFilter->parentWidget()->hide();
            std::cout << "Hiding time filter" << std::endl;
        }
        if (capacityFilter) {
            capacityFilter->parentWidget()->hide();
            std::cout << "Hiding capacity filter" << std::endl;
        }
        walkinPage->update();

        updateWalkinInfo();

        // Hide unrelated pages
        reservationsPage->hide();
        bookingPage->hide();
        contactPage->hide();

        // Show contact page
        walkinPage->show();

        // Update button states
        ui->Dashboard->setChecked(false);
        ui->Orders->setChecked(false);
        ui->Menu->setChecked(true);
        ui->Locations->setChecked(false);
    }
}


void Home::updateWalkinInfo()
{
    std::cout<<"update walk in info funciton called"<<std::endl;
    //Get Current Time
    QDateTime currentDateTime = QDateTime::currentDateTime();

    int small_table = 0;
    int big_table = 0;

    // store table IDs of active tables in list
    QList<QString> activeTables;

    // Check each table for active reservations
    for (const auto& tableId : tables.keys()) {
        std::cout << "in get loop" << small_table << std::endl;
        const TableInfo& tableInfo = tables[tableId];
        for (const QDateTime& reservationTime : tableInfo.reservedTimes) {
            // Check if the current time is within 1 hour of the start of the reservation
            if (reservationTime <= currentDateTime && reservationTime.addSecs(3600) > currentDateTime) {
                activeTables.append(tableId);
                break; //
            }
        }
    }

    std::cout << "Active Tables Count: " << activeTables.size() << std::endl;
    for (const QString& tableId : activeTables) {
        std::cout << "Active Table ID:" << tableId.toStdString() << std::endl;
    }

    // count number of big vs small active tables
    for (const QString& tableId : activeTables) {
        //std::cout << "in count loop " << small_table << std::endl;
        const TableInfo& tableInfo = tables[tableId];
        if (tableInfo.seats == 4) {
            std::cout << "small counted: " << small_table << std::endl;
            small_table++;
        } else if (tableInfo.seats == 8) {
            std::cout << "big counted: " << big_table << std::endl;
            big_table++;
        }
    }

    numTables = 14;



    //estimation is simplified, should rely on historical patterns in practice
    //overall pattern shows that as the num reservations for a time increases, so does the estimated wait time
    //handle zero division error
    if (small_table==12){
        bigWaitTime=90;
    }else if(small_table<12){
        smallWaitTime=std::max(5, 90/(12-small_table));
        smallWaitTime=5*(smallWaitTime+4)/5; //round to closest 5 mniutes
    }else{
        smallWaitTime = 0;
    }

    if(big_table==2) {
        bigWaitTime=90;
    }else if (big_table==1) {
        bigWaitTime=30;
    } else {
        bigWaitTime = 0; // No wait time if all big tables are free
    }



    total_res=small_table+big_table;
    // Update the labels with the new data
    totalTablesLabel->setText(QString::number(numTables));
    reservedTablesLabel->setText(QString::number(total_res));
    small_waitTimesLabel->setText(QString("%1 mins").arg(smallWaitTime));
    big_waitTimesLabel->setText(QString("%1 mins").arg(bigWaitTime));
}




int Home::calculateActiveReservations()
{
    int count = 0;
    QDateTime currentDateTime = QDateTime::currentDateTime();

    for (const auto& tableInfo : tables) {
        for (const QDateTime& reservationTime : tableInfo.reservedTimes) {
            if (reservationTime > currentDateTime) {
                count++;
            }
        }
    }
    return count;
}






void Home::loadUserReservations(const ReservationFilter& filter)
{
    QScrollArea* scrollArea = reservationsPage->findChild<QScrollArea*>();
    if (!scrollArea || !scrollArea->widget()) return;

    QVBoxLayout* listLayout = qobject_cast<QVBoxLayout*>(scrollArea->widget()->layout());
    if (!listLayout) return;

    // Clear existing items
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Fetch reservations from the database specific to the current user
    QSqlDatabase logindb = QSqlDatabase::database();
    QSqlQuery query(logindb);
    if(m_userMode == "customer")
    {
        query.prepare("SELECT table_id, reservation_time FROM reservations WHERE username = :username");
        query.bindValue(":username", currentUser);
    }
    else if(m_userMode == "manager")
    {
        query.prepare("SELECT table_id, reservation_time FROM reservations");
    }

    if (!query.exec()) {
        qDebug() << "Failed to load reservations for user:" << query.lastError().text();
        return;
    }

    // Create reservation cards for the current user
    while (query.next()) {
        QString tableId = query.value("table_id").toString();
        QDateTime reservationTime = QDateTime::fromString(query.value("reservation_time").toString(), Qt::ISODate);

        // Ensure that table info exists for the tableId
        if (!tables.contains(tableId)) continue;  // If the table doesn't exist, skip

        const TableInfo& info = tables[tableId];

        // Apply the filter function here, passing the TableInfo and reservationTime
        if (!filter(info, reservationTime)) continue;  // Apply filter if any

        QWidget* card = new QWidget;
        card->setStyleSheet(
            "QWidget {"
            "    background-color: white;"
            "    border: 1px solid #E0E0E0;"
            "    border-radius: 8px;"
            "    padding: 16px;"
            "}");

        QHBoxLayout* cardLayout = new QHBoxLayout(card);

        // Table Number
        QLabel* tableNum = new QLabel(QString("Table %1").arg(tableId.mid(5)));
        tableNum->setStyleSheet(
            "font-weight: bold;"
            "min-width: 80px;"
            "color: #1B4965;");

        // Time and Date
        QVBoxLayout* timeLayout = new QVBoxLayout;
        QLabel* dateLabel = new QLabel(reservationTime.toString("MMM d, yyyy"));
        QLabel* timeLabel = new QLabel(reservationTime.toString("h:mm AP"));
        dateLabel->setStyleSheet("color: #2C3E50;");
        timeLabel->setStyleSheet("color: #6C757D;");
        timeLayout->addWidget(dateLabel);
        timeLayout->addWidget(timeLabel);

        // Status
        QLabel* statusLabel = new QLabel(info.isVIP ? "VIP" : "Standard");
        statusLabel->setStyleSheet(info.isVIP ?
                                       "color: #FFB400; font-weight: bold;" :
                                       "color: #6C757D;");

        // Cancel Button
        QPushButton* cancelBtn = new QPushButton("Cancel");
        cancelBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #DC3545;"
            "    color: white;"
            "    border-radius: 4px;"
            "    padding: 8px 16px;"
            "    border: none;"
            "} "
            "QPushButton:hover {"
            "    background-color: #C82333;"
            "}");

        cancelBtn->setFixedWidth(100);

        // Layout assembly
        cardLayout->addWidget(tableNum);
        cardLayout->addLayout(timeLayout);
        cardLayout->addWidget(statusLabel);
        cardLayout->addStretch();
        cardLayout->addWidget(cancelBtn);

        listLayout->addWidget(card);

        // Connect cancel button
        connect(cancelBtn, &QPushButton::clicked, this, [this, tableId, reservationTime]() {
            // Remove reservation from the database
            if (!removeReservationFromDatabase(tableId, reservationTime, currentUser)) {
                QMessageBox::warning(this, "Database Error", "Failed to cancel reservation.");
                return;
            }

            // Remove from local reservation list
            TableInfo& tableInfo = tables[tableId];
            tableInfo.reservedTimes.removeOne(reservationTime);
            saveReservations();
            loadUserReservations();  // Refresh the list
        });
    }

    // Add stretch at the end
    listLayout->addStretch();
}




bool Home::removeReservationFromDatabase(const QString &tableId, const QDateTime &reservationTime, const QString &username)
{
    // Ensure database connection
    QSqlDatabase logindb = QSqlDatabase::database();

    // Prepare and execute the SQL query
    QSqlQuery query(logindb);
    query.prepare("DELETE FROM reservations WHERE table_id = :table_id AND reservation_time = :reservation_time");
    query.bindValue(":table_id", tableId);
    query.bindValue(":reservation_time", reservationTime.toString(Qt::ISODate));
    query.bindValue(":username", username);

    if (!query.exec()) {
        qDebug() << "Failed to remove reservation:" << query.lastError().text();
        return false;
    }

    return true;
}









void Home::on_Dashboard_clicked()
{
    showReservationsPage();

    // Update the statistics and reservations list
    QScrollArea* scrollArea = reservationsPage->findChild<QScrollArea*>();
    if (scrollArea && scrollArea->widget()) {
        QVBoxLayout* listLayout = qobject_cast<QVBoxLayout*>(scrollArea->widget()->layout());
        if (listLayout) {
            // Clear and reload reservations
            while (QLayoutItem* item = listLayout->takeAt(0)) {
                delete item->widget();
                delete item;
            }
            loadUserReservations();
        }
    }

    // Update any statistics display
    QList<QLabel*> statLabels = reservationsPage->findChildren<QLabel*>();
    for (QLabel* label : statLabels) {
        if (label->text().contains("$")) {
            // Update revenue stat
            label->setText(QString("$%1").arg(calculateDailyRevenue()));
        } else if (label->text().toInt() > 0) {
            // Update numeric stats
            if (label->text().contains("VIP")) {
                label->setText(QString::number(countVIPReservations()));
            } else {
                label->setText(QString::number(calculateActiveReservations()));
            }
        }
    }

    // Update the date/time based greeting
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString greeting;
    int hour = currentDateTime.time().hour();
    if (hour < 12) greeting = "Good morning";
    else if (hour < 17) greeting = "Good afternoon";
    else greeting = "Good evening";

    QList<QLabel*> labels = reservationsPage->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label->text().startsWith("Good")) {
            label->setText(QString("%1, %2").arg(greeting).arg(m_userMode));
        } else if (label->text().contains("day,")) {
            label->setText(currentDateTime.toString("dddd, MMMM d, yyyy"));
        }
    }
}
void Home::on_Orders_clicked()
{
    showBookingPage();
}


void Home::cleanupNavigation()
{
    // Hide the reservation prompt if it exists
    if (rightPanel) {
        rightPanel->hide();
    }

    // Reset selected table
    if (selectedTable) {
        selectedTable = nullptr;
        for (const auto &tableName : tables.keys()) {
            QPushButton *tableButton = findChild<QPushButton *>(tableName);
            if (tableButton) {
                updateTableAppearance(tableButton);
            }
        }
    }
}

void Home::setupReservationControls()
{
    // Create container for reservation controls
    QWidget* controlsContainer = new QWidget(reservationsPage);  // Make it a child of reservationsPage
    controlsContainer->setObjectName("reservationControls");

    QHBoxLayout* controlsLayout = new QHBoxLayout(controlsContainer);
    controlsLayout->setSpacing(16);
    controlsLayout->setContentsMargins(0, 16, 0, 16);

    // Search Box with text icon (using emoji instead of image)
    QWidget* searchContainer = new QWidget;
    QHBoxLayout* searchLayout = new QHBoxLayout(searchContainer);
    searchLayout->setContentsMargins(12, 0, 12, 0);
    searchLayout->setSpacing(8);

    QLabel* searchIcon = new QLabel("🔍");
    searchIcon->setFixedSize(16, 16);
    searchIcon->setStyleSheet("color: #6C757D;");

    reservationSearchBox = new QLineEdit;
    reservationSearchBox->setPlaceholderText("Search reservations...");
    reservationSearchBox->setStyleSheet(
        "QLineEdit {"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    font-size: 14px;"
        "    min-width: 200px;"
        "}");

    searchLayout->addWidget(searchIcon);
    searchLayout->addWidget(reservationSearchBox);

    // Filters Style
    QString filterStyle =
        "QComboBox {"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 8px 12px;"
        "    min-width: 140px;"
        "    font-size: 14px;"
        "    background: white;"
        "}"
        "QComboBox:hover {"
        "    border-color: #1B4965;"
        "}";

    // Status Filter
    reservationStatusFilter = new QComboBox;
    reservationStatusFilter->addItems({"All Status", "Upcoming", "Completed", "Cancelled"});
    reservationStatusFilter->setStyleSheet(filterStyle);

    // Date Filter
    reservationDateFilter = new QComboBox;
    reservationDateFilter->addItems({"All Time", "Today", "This Week", "This Month"});
    reservationDateFilter->setStyleSheet(filterStyle);

    // Type Filter
    reservationTypeFilter = new QComboBox;
    reservationTypeFilter->addItems({"All Types", "Standard", "VIP"});
    reservationTypeFilter->setStyleSheet(filterStyle);

    // Sort Button
    QPushButton* sortButton = new QPushButton("Sort by Date ↓");
    sortButton->setObjectName("sortButton");
    sortButton->setStyleSheet(
        "QPushButton {"
        "    border: 1px solid #DEE2E6;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-size: 14px;"
        "    background: white;"
        "}"
        "QPushButton:hover {"
        "    background: #F8F9FA;"
        "}");

    // Add widgets to layout
    controlsLayout->addWidget(searchContainer);
    controlsLayout->addWidget(reservationStatusFilter);
    controlsLayout->addWidget(reservationDateFilter);
    controlsLayout->addWidget(reservationTypeFilter);
    controlsLayout->addStretch();
    controlsLayout->addWidget(sortButton);

    // Add to main layout
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(reservationsPage->layout());
    if (mainLayout) {
        // Add after the stats cards but before the reservations list
        mainLayout->insertWidget(3, controlsContainer);
    }

    // Connect signals
    connect(reservationSearchBox, &QLineEdit::textChanged, this, &Home::filterReservations);
    connect(reservationStatusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Home::filterReservations);
    connect(reservationDateFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Home::filterReservations);
    connect(reservationTypeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &Home::filterReservations);
    connect(sortButton, &QPushButton::clicked, this, &Home::toggleReservationSort);
}
void Home::filterReservations()
{
    QString searchText = reservationSearchBox->text().toLower();
    QString status = reservationStatusFilter->currentText();
    QString dateRange = reservationDateFilter->currentText();
    QString type = reservationTypeFilter->currentText();

    QDateTime currentDateTime = QDateTime::currentDateTime();

    loadUserReservations([=](const TableInfo& info, const QDateTime& reservationTime) -> bool {
        // Search text filter
        if (!searchText.isEmpty()) {
            QString tableStr = QString("Table %1").arg(info.seats);
            if (!tableStr.toLower().contains(searchText) &&
                !info.customerName.toLower().contains(searchText)) {
                return false;
            }
        }

        // Status filter
        if (status == "Upcoming" && reservationTime < currentDateTime) return false;
        if (status == "Completed" && reservationTime > currentDateTime) return false;

        // Date range filter
        if (dateRange == "Today" && reservationTime.date() != currentDateTime.date()) return false;
        if (dateRange == "This Week") {
            int daysTo = currentDateTime.date().daysTo(reservationTime.date());
            if (daysTo < 0 || daysTo > 7) return false;
        }
        if (dateRange == "This Month" &&
            (reservationTime.date().month() != currentDateTime.date().month() ||
             reservationTime.date().year() != currentDateTime.date().year())) {
            return false;
        }

        // Type filter
        if (type == "Standard Tables" && info.isVIP) return false;
        if (type == "VIP Tables" && !info.isVIP) return false;

        return true;
    });
}










void Home::toggleReservationSort()
{
    m_sortAscending = !m_sortAscending;
    loadUserReservations();  // Reload with new sort order
}

void Home::exportReservations()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Export Reservations", "",
                                                    "CSV Files (*.csv);;All Files (*)");

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "Table,Date,Time,Type,Status,Customer Name\n";

    QDateTime currentDateTime = QDateTime::currentDateTime();

    for (auto it = tables.begin(); it != tables.end(); ++it) {
        const TableInfo& info = it.value();
        for (const QDateTime& reservationTime : info.reservedTimes) {
            QString status = reservationTime > currentDateTime ? "Upcoming" : "Completed";
            out << QString("%1,%2,%3,%4,%5,%6\n")
                       .arg(it.key())
                       .arg(reservationTime.date().toString("yyyy-MM-dd"))
                       .arg(reservationTime.time().toString("HH:mm"))
                       .arg(info.isVIP ? "VIP" : "Standard")
                       .arg(status)
                       .arg(info.customerName);
        }
    }

    file.close();
    QMessageBox::information(this, "Export Complete", "Reservations have been exported successfully.");
}

int Home::calculateDailyRevenue()
{
    int revenue = 0;
    QDate today = QDate::currentDate();

    for (const auto& tableInfo : tables) {
        for (const QDateTime& reservationTime : tableInfo.reservedTimes) {
            if (reservationTime.date() == today) {
                revenue += tableInfo.isVIP ? 200 : 100;  // VIP tables cost more
            }
        }
    }
    return revenue;
}

int Home::countVIPReservations()
{
    int count = 0;
    QDate today = QDate::currentDate();

    for (const auto& tableInfo : tables) {
        if (tableInfo.isVIP) {
            for (const QDateTime& reservationTime : tableInfo.reservedTimes) {
                if (reservationTime.date() == today) {
                    count++;
                }
            }
        }
    }
    return count;
}

Home::~Home()
{
    delete ui;
}
