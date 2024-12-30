#ifndef LOGINSCREEN_H
#define LOGINSCREEN_H

#include <QDialog>
#include <QtSql>
#include <QDebug>
#include <QFileInfo>

#include "restaurant.h"
#include "usersignup.h"

namespace Ui {
class LoginScreen;
}

class LoginScreen : public QDialog
{
    Q_OBJECT

public:
    explicit LoginScreen(QWidget *parent = nullptr);
    QString validateCredentials(const QString& enteredUsername, const QString& enteredPassword);
    void createAccount(QString Username, QString Password, QString Permission, QString number);

    //database function definition
    void connectToDatabase();
    bool addUser(const QString &username, const QString &password, const QString &permission, const QString &number);
    bool checkUserCredentials(const QString &username, const QString &password);
    QString hashPassword(const QString &password);

    ~LoginScreen();

private slots:
    void on_pushButton_login_clicked();
    void on_pushButton_signup_clicked();
    void showLoginScreen();

private:
    Ui::LoginScreen *ui;
    Restaurant *res;
    usersignup *signup;
    QStringList localCredentials;
};

#endif // LOGINSCREEN_H
