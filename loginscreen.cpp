#include "loginscreen.h"
#include <QMessageBox>
#include "restaurant.h"
#include "ui_loginscreen.h"
#include "usersignup.h"
#include "home.h"
#include <QPixmap>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDir>
#include <QCoreApplication>

LoginScreen::LoginScreen(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginScreen)
{
    ui->setupUi(this);

    QSqlDatabase logindb = QSqlDatabase::addDatabase("QSQLITE");
    logindb.setDatabaseName("testdb.db");
    logindb.open();

    if (!logindb.open()) {
        qDebug() << "Error: Could not connect to database." << logindb.lastError().text();
    } else {
        qDebug() << "Database connected successfully!";
    }
}

LoginScreen::~LoginScreen()
{
    delete ui;
}

QString LoginScreen::hashPassword(const QString &password) {
    QByteArray hashed = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hashed.toHex());
}

void LoginScreen::on_pushButton_login_clicked() {
    QString username = ui->lineEdit_username->text();
    QString password = hashPassword(ui->lineEdit_password->text());

    // Query the database for user credentials
    QSqlQuery query;
    query.prepare("SELECT permission, id FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (query.exec() && query.next()) {
        QString permission = query.value(0).toString();
        int userId = query.value(1).toInt();

        qDebug() << "Login successful!";
        qDebug() << "Permission:" << permission << ", User ID:" << userId << ", Username:" << username;

        // Pass the username correctly to Home
        auto res = new Home(nullptr, permission, userId, username);
        res->show();
        this->close();
    } else {
        QMessageBox::warning(this, "Login Error", "Invalid username or password");
    }
}



void LoginScreen::on_pushButton_signup_clicked()
{
    signup = new usersignup(nullptr);
    connect(signup, &usersignup::backToLogin, this, &LoginScreen::showLoginScreen);
    connect(signup, &usersignup::accountCreated, this, &LoginScreen::createAccount);

    signup->show();
    this->hide();
}

void LoginScreen::showLoginScreen()
{
    this->show();
}

void LoginScreen::createAccount(QString Username, QString Password, QString Permission, QString number) {
    if (addUser(Username, Password, Permission, number)) {
        qDebug() << "Account created successfully!";
        QMessageBox::information(this, "Signup Success", "Account created successfully!");
    } else {
        QMessageBox::warning(this, "Signup Error", "Could not create account.");
    }
    this->show();
}

bool LoginScreen::addUser(const QString &username, const QString &password, const QString &permission, const QString &number) {
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, permission, number) VALUES (:username, :password, :permission, :number)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":permission", permission);
    query.bindValue(":number", number);

    qDebug() << "Bound values:"
             << "username:" << username
             << "password:" << password
             << "permission:" << permission;

    if (query.exec()) {
        return true;
    } else {
        qDebug() << "Error: Could not add user." << query.lastError().text();
        return false;
    }
}

bool LoginScreen::checkUserCredentials(const QString &username, const QString &password) {
    QSqlQuery query;
    query.prepare("SELECT permission FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (query.exec() && query.next()) {
        return true;
    } else {
        return false;
    }
}
