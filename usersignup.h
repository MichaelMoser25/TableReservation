#ifndef USERSIGNUP_H
#define USERSIGNUP_H

#include <QWidget>
// #include "loginscreen.h"

namespace Ui {
class usersignup;
}

class usersignup : public QWidget
{
    Q_OBJECT

public:
    explicit usersignup(QWidget *parent = nullptr);
    ~usersignup();

signals:
    void backToLogin();
    void accountCreated(const QString &username, const QString &password, const QString &permissions, const QString &number);


private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    Ui::usersignup *ui;
    // LoginScreen *login;
};

#endif // USERSIGNUP_H
