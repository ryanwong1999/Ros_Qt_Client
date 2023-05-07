#include "login.h"
#include "ui_login.h"
#include <QGraphicsDropShadowEffect>

login::login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::login)
{
    ui->setupUi(this);
    this->setProperty("canMove",true);
    this->initUi();
}

login::~login()
{
    delete ui;
}

void login::initUi()
{
    //初始化窗口边框
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor("#444444"));
    shadow->setBlurRadius(16);
    ui->w_bg_2->setGraphicsEffect(shadow);
    ui->w_bg->setMargin(12);
}


void login::on_btn_login_clicked()
{
    emit sendLoginSuccess();
    qDebug()<<"进入主界面"<<endl;
}

void login::on_btn_exit_clicked()
{
    exit(0);
}
