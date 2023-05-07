#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QDebug>

namespace Ui {
class login;
}

class login : public QDialog
{
    Q_OBJECT

public:
    explicit login(QWidget *parent = nullptr);
    ~login();

private slots:
    void on_btn_login_clicked();
    void on_btn_exit_clicked();

signals:
    void sendLoginSuccess();

private:
    Ui::login *ui;
    void initUi();          //Ui界面初始化函数
};

#endif // LOGIN_H
