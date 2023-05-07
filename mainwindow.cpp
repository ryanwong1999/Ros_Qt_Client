#include "mainwindow.h"
#include "ui_mainwindow.h"

bool isconnected = false; // 是否连接的标志位
// 多线程处理数据
MyThread laser_thread;
MyThread map_thread;
MyThread robot_pose_thread;
MyThread pms_thread;
MyThread envirment_thread;

MyThread robot_state_thread;
MyThread plan_thread;
MyThread movebase_result_thread;
MyThread environment_data_thread;

struct Quaternion // 四元数结构体
{
    double w, x, y, z;
};

typedef struct { // 欧拉角结构体
    double yaw, pitch, roll;
} EulerAngle;
EulerAngle e;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_login.show();
    auto f = [&](){
        this->show();
        m_login.hide();
    };
    connect(&m_login, &login::sendLoginSuccess, this, f);
    this->initUi();
    initVLC();

    //连接rosbridge
//    QString path = QString("ws://%1:%2").arg(ui->lineEdit_RobotIP->text()).arg(9090);
    QString path = QString("ws://%1:%2").arg("192.168.0.117").arg("9090");
    QUrl url = QUrl(path);
    m_websocket.open(url);

    allconnect();
}

MainWindow::~MainWindow()
{
    //delete vlc
    delete ipc_player;
    delete ipc_media;
    delete ipc_instance;
    delete red_player;
    delete red_media;
    delete red_instance;
    //关闭websocket
    m_websocket.close();
    delete ui;
}

//初始化界面显示
void MainWindow::initUi()
{
    this->setProperty("canMove",false);
    this->setWindowTitle("巡检机器人管理系统");

    QTimer *timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerUpdata()));
    timer->start(1000);

    // 设置comboBox_RobotIP只能输入IP
    //  正在表达式限制输入
    QRegExp exp("\\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\b");
    QRegExpValidator *Validator = new QRegExpValidator(exp);

    ui->lineEdit_RobotIP->setValidator(Validator);
    ui->lineEdit_IPCIP->setValidator(Validator);
    ui->lineEdit_REDIP->setValidator(Validator);
    // 用于占位
//    ui->lineEdit_RobotIP->setInputMask("000.000.000.000;");
//    ui->lineEdit_IPCIP->setInputMask("000.000.000.000;");
//    ui->lineEdit_REDIP->setInputMask("000.000.000.000;");

    ui->mapViz->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 消除滚动条
    ui->mapViz->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 视图场景加载
    m_qgraphicsScene =
        new QGraphicsScene; // 要用QGraphicsView就必须要有QGraphicsScene搭配着用
    m_qgraphicsScene->clear();
    // 创建item
    m_roboItem = new Ui::roboItem();
    // 视图添加item
    m_qgraphicsScene->addItem(m_roboItem);
    // 设置item的坐标原点与视图的原点重合（默认为视图中心）
    //  widget添加视图
    ui->mapViz->setScene(m_qgraphicsScene);
}



void MainWindow::allconnect()
{
    // 连接Websocket的连接状态
    connect(&m_websocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onTextMessageReceived(QString))); // 连接接收数据成功信号
    connect(&m_websocket, &QWebSocket::connected, this, [=]()                                                // 连接成功信号
            {
                isconnected = true;
                ui->label_connect->setText("已连接");
                qDebug()<<"已连接m_websocket"<<endl;
//                // 保存IP和端口
//                QSettings ip("./setting.ini", QSettings::IniFormat);
//                ip.setValue("WS/IP", ui->iplineedit->text());
//                ip.setValue("WS/Port", ui->portlineedit->text());
                init();
            });
    connect(&m_websocket, &QWebSocket::disconnected, this, [=]() // 连接断开信号
            {
                isconnected = false;
                ui->label_connect->setText("未连接");
                qDebug()<<"断开m_websocket"<<endl;
            });
}

// 连接后加载
void MainWindow::init()
{
    QSettings checkbox_setting("./subsetting.ini", QSettings::IniFormat);
    QStringList keys = checkbox_setting.allKeys();
    if (keys.length() == 0) // 无配置文件将订阅所有话题，并生成配置文件
    {
        QStringList topics = {"/map", "/robot_pose", "/scan", "/Pms_get_status", "/Envirment_data"};
        foreach (QString topic, topics) {
            checkbox_setting.setValue("checkbox_setting" + topic, "true");
        }
        read_map();        // 订阅地图话题map
        getrobot_pose();   // 订阅机器人位置，属自定义话题，话题robot_pose
        read_laser();      // 订阅雷达消息，话题scan
        read_pms();        // 订阅电量消息
        read_envirment();  // 订阅环境数据
    }
    foreach (QString key, keys) {
        if (checkbox_setting.value(key).toString() == "true") {
            key = key.remove("checkbox_setting");
            QString sub = QString("{\"op\":\"subscribe\",\"topic\":\"%1\"}").arg(key);
            m_websocket.sendTextMessage(sub);
        }
    }
}

//显示时间
void MainWindow::timerUpdata()
{
    QFont font("Microsoft YaHei", 10, 30);
    QDateTime time = QDateTime::currentDateTime();
    QString time_str = time.toString("   yyyy-MM-dd \n hh:mm:ss dddd");
    ui -> label_time ->setFont(font);
    this -> ui->label_time->setText(time_str);
}

void MainWindow::initVLC()
{
    ipc_instance = new VlcInstance(VlcCommon::args(), this);
    ipc_player = new VlcMediaPlayer(ipc_instance);
    ipc_player->setVideoWidget(ui->widget_ipc);

    red_instance = new VlcInstance(VlcCommon::args(), this);
    red_player = new VlcMediaPlayer(red_instance);
    red_player->setVideoWidget(ui->widget_red);

    openUrl();
}

//打开rtsp
void MainWindow::openUrl()
{
    QString ipc_url = "rtsp://192.168.0.118:554/user=admin&password=&channel=1&stream=1.sdp?";
    ipc_media = new VlcMedia(ipc_url, ipc_instance);
    ipc_player->open(ipc_media);

    QString red_url = "rtsp://192.168.0.118:554/user=admin&password=&channel=1&stream=1.sdp?";
    red_media = new VlcMedia(red_url, red_instance);
    red_player->open(red_media);
}


// 接收到信息后来到这里进行判断，并启动相应的线程处理数据
void MainWindow::onTextMessageReceived(const QString &message)
{
    if (message.contains("/map")) // 如果是地图信息
    {
        map_thread.select = 1;
        map_thread.w = this;
        map_thread.message = message;
        map_thread.start();
        // map_data_paint(message);
    } else if (message.contains("/robot_pose")) {
        robot_pose_thread.select = 2;
        robot_pose_thread.w = this;
        robot_pose_thread.message = message;
        robot_pose_thread.start();
        // get_RobotPose(message);
    } else if (message.contains("/PMS_get_status")) {
        pms_thread.select = 3;
        pms_thread.w = this;
        pms_thread.message = message;
        pms_thread.start();
    } else if (message.contains("/Envirment_data")) {
        envirment_thread.select = 4;
        envirment_thread.w = this;
        envirment_thread.message = message;
        envirment_thread.start();
    }
}

// QString转Json程序
QJsonObject MainWindow::QstringToJson(QString jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toLocal8Bit().data());
    if (jsonDocument.isNull()) {
        // qDebug()<< "String NULL"<< jsonString.toLocal8Bit().data();
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}

// 订阅地图信息
void MainWindow::read_map()
{
    QString data = "{\"op\":\"subscribe\",\"topic\":\"/map\"}";
    m_websocket.sendTextMessage(data);
}

// 订阅机器人位置
void MainWindow::getrobot_pose()
{
    QString data = "{\"op\":\"subscribe\",\"topic\":\"/robot_pose\"}";
    m_websocket.sendTextMessage(data);
}

// 雷达订阅
void MainWindow::read_laser()
{
    QString data = "{\"op\":\"subscribe\",\"topic\":\"/scan\"}";
    m_websocket.sendTextMessage(data);
}

// 电量订阅
void MainWindow::read_pms()
{
    QString data = "{\"op\":\"subscribe\",\"topic\":\"/PMS_get_status\"}";
    m_websocket.sendTextMessage(data);
}

// 环境数据订阅
void MainWindow::read_envirment()
{
    QString data = "{\"op\":\"subscribe\",\"topic\":\"/Envirment_data\"}";
    m_websocket.sendTextMessage(data);
}

// 发送速度消息
void MainWindow::cmd_vel(QString x, QString z)
{
    QString data = QString("{\"msg\":{\"angular\":{\"x\":0,\"y\":0,\"z\":%1},\"linear\":{\"x\":%2,\"y\":0,\"z\":0}},\"op\":\"publish\",\"topic\":\"/cmd_vel\"}").arg(z).arg(x);
    m_websocket.sendTextMessage(data);
}


void MainWindow::on_btn_watch_clicked()
{
    ui->sw_page->setCurrentIndex(0);
    qDebug()<<"切换page_watch"<<endl;
}

void MainWindow::on_btn_log_clicked()
{
    ui->sw_page->setCurrentIndex(1);
    qDebug()<<"切换page_log"<<endl;
}

void MainWindow::on_btn_task_clicked()
{
    ui->sw_page->setCurrentIndex(2);
    qDebug()<<"切换page_task"<<endl;
}

void MainWindow::on_btn_set_clicked()

{
    ui->sw_page->setCurrentIndex(3);
    qDebug()<<"切换page_set"<<endl;
}