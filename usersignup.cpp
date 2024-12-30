#include "usersignup.h"
#include "ui_usersignup.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QMessageBox>

usersignup::usersignup(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::usersignup)
{
    ui->setupUi(this);
}

usersignup::~usersignup()
{
    delete ui;
}

QString hashPassword(const QString &password) {
    QByteArray hashed = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString(hashed.toHex());
}

void usersignup::on_pushButton_2_clicked()
{
    emit backToLogin();
    this->close();
}

void usersignup::on_pushButton_clicked()
{
    QString username = ui->usernameEnter->text();
    QString password = ui->passwordEnter->text();
    QString pNumber = ui->phoneEnter->text();
    QString managerCode = ui->codeEnter->text();

    QString hashedPassword = hashPassword(password);

    // Check if username already exists
    QSqlQuery query;
    query.prepare("SELECT username FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        QMessageBox::warning(this, "Signup Error", "Username already exists. Please choose a different username.");
        return;
    }

    // Emit signal to create account in database
    if (managerCode == "8008") {
        emit accountCreated(username, hashedPassword, "manager", pNumber);
    }
    else {
        emit accountCreated(username, hashedPassword, "customer", pNumber);
    }
    emit backToLogin();
    this->close();
}
