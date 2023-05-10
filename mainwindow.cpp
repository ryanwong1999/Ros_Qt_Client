#include "mainwindow.h"
#include "ui_mainwindow.h"

bool isconnected = false; // 是否连接的标志位
int headLevelCnt = 90;
int headPitchCnt = 90;
int page_now = 0;

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
    this->initVLC();
    this->initPSC();

    //连接rosbridge
//    QString path = QString("ws://%1:%2").arg(ui->lineEdit_RobotIP->text()).arg(9090);
    QString path = QString("ws://%1:%2").arg("192.168.0.117").arg("9090");
    QUrl url = QUrl(path);
    m_websocket.open(url);

    this->allconnect();

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
    //定时器
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

    ui->mapViz->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 消除滚动条
    ui->mapViz->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 视图场景加载
    m_qgraphicsScene = new QGraphicsScene; // 要用QGraphicsView就必须要有QGraphicsScene搭配着用
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

void MainWindow::initPSC()
{
    //云台复位按钮
    connect(ui->btn_headReset,&QPushButton::clicked,[&]{
        headLevelCnt = 90;
        headPitchCnt = 90;
        psc_angle_control(QString::number(90), QString::number(90));
    });
    //云台上看长按
    ui->btn_headUp->setAutoRepeat(true); //启用长按
    ui->btn_headUp->setAutoRepeatDelay(400);//触发长按的时间
    ui->btn_headUp->setAutoRepeatInterval(50);//长按时click信号间隔
    connect(ui->btn_headUp,&QPushButton::clicked,[&]{
        headPitchCnt+=5;
        psc_angle_control(QString::number(headLevelCnt), QString::number(headPitchCnt));
        if(headPitchCnt>=135) headPitchCnt=135;//将最大值控制在135
    });
    //云台下看长按
    ui->btn_headDown->setAutoRepeat(true); //启用长按
    ui->btn_headDown->setAutoRepeatDelay(400);//触发长按的时间
    ui->btn_headDown->setAutoRepeatInterval(50);//长按时click信号间隔
    connect(ui->btn_headDown,&QPushButton::clicked,[&]{
        headPitchCnt-=5;
        psc_angle_control(QString::number(headLevelCnt), QString::number(headPitchCnt));
        if(headPitchCnt<=45) headPitchCnt=45;//将最小值控制在45
    });
    //云台右看长按
    ui->btn_headRight->setAutoRepeat(true); //启用长按
    ui->btn_headRight->setAutoRepeatDelay(400);//触发长按的时间
    ui->btn_headRight->setAutoRepeatInterval(50);//长按时click信号间隔
    connect(ui->btn_headRight,&QPushButton::clicked,[&]{
        headLevelCnt+=5;
        psc_angle_control(QString::number(headLevelCnt), QString::number(headPitchCnt));
        if(headPitchCnt<=180) headPitchCnt=180;//将最大值控制在180
    });
    //云台左看长按
    ui->btn_headLeft->setAutoRepeat(true); //启用长按
    ui->btn_headLeft->setAutoRepeatDelay(400);//触发长按的时间
    ui->btn_headLeft->setAutoRepeatInterval(50);//长按时click信号间隔
    connect(ui->btn_headLeft,&QPushButton::clicked,[&]{
        headLevelCnt-=5;
        psc_angle_control(QString::number(headLevelCnt), QString::number(headPitchCnt));
        if(headPitchCnt>=0) headPitchCnt=0;//将最小值控制在0
    });
}

void MainWindow::initVLC()
{
    //可见光初始化
    ipc_instance = new VlcInstance(VlcCommon::args(), this);
    ipc_player = new VlcMediaPlayer(ipc_instance);
    ipc_player->setVideoWidget(ui->widget_ipc);
    //红外初始化
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

    //QString red_url = "rtsp://192.168.0.112:554/user=admin&password=&channel=1&stream=1.sdp?";
    QString red_url = "rtsp://192.168.0.117:9554/live?channel=0&subtype=0";
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
    } else if (message.contains("/robot_pose")) {
        robot_pose_thread.select = 2;
        robot_pose_thread.w = this;
        robot_pose_thread.message = message;
        robot_pose_thread.start();
        // get_RobotPose(message)
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

// 对收到的地图数据进行处理
void MainWindow::map_data_handle(QString data)
{
    qDebug() << "map_data_handle";
    QJsonObject jsondata, msgdata, infodata, origin, position;
    jsondata = QstringToJson(data);
    msgdata = jsondata["msg"].toObject();
    infodata = msgdata["info"].toObject();
    QJsonValue width = infodata["width"];
    QJsonValue height = infodata["height"];
    QJsonValue resolution = infodata["resolution"];
    origin = infodata["origin"].toObject();
    position = origin["position"].toObject();
    QJsonValue positionx = position["x"];
    QJsonValue positiony = position["y"];
    QJsonValue map = msgdata["data"]; // 地图的所有数据
    // 使用map[num];访问其中的第num+1个元素元素
    mapCallback(width, height, map, resolution, positionx, positiony);
}

// 地图信息订阅回调函数
// 在此函数传入map_data_paint处理后的数据，使用roboItem定义好的paintMaps(map_image)，就可以在GraphiceView中画出地图，参考了古月居的程序
void MainWindow::mapCallback(QJsonValue width, QJsonValue height, QJsonValue map, QJsonValue resolution, QJsonValue positionx, QJsonValue positiony)
{
    int size;
    size = width.toInt() * height.toInt();
    m_mapResolution = resolution.toDouble(); // msg->info.resolution;
    double origin_x = positionx.toDouble();  // msg->info.origin.position.x;
    double origin_y = positiony.toDouble();  // msg->info.origin.position.y;
    QImage map_image(width.toInt(), height.toInt(), QImage::Format_RGB32);
    for (int i = 0; i < size; i++) { // 想办法获取map的长度
        int x = i % width.toInt();
        int y = (int)i / width.toInt();
        // 计算像素值
        QColor color;
        if (map[i] == 100) {
            color = Qt::black; // black
        } else if (map[i] == 0) {
            color = Qt::white; // white
        } else if (map[i] == -1) {
            color = Qt::gray; // gray
        }
        map_image.setPixel(x, y, qRgb(color.red(), color.green(), color.blue()));
    }
    // 延y翻转地图 因为解析到的栅格地图的坐标系原点为左下角
    // 但是图元坐标系为左上角度
    map_image = rotateMapWithY(map_image);
    emit update_map(map_image);
    // updateMap(map_image);
    // m_roboItem->paintMaps(map_image);
    // 计算翻转后的图元坐标系原点的世界坐标
    double origin_x_ = origin_x;
    double origin_y_ = origin_y + height.toInt() * m_mapResolution;
    // 世界坐标系原点在图元坐标系下的坐标
    m_wordOrigin.setX(fabs(origin_x_) / m_mapResolution);
    m_wordOrigin.setY(fabs(origin_y_) / m_mapResolution);
}

QImage MainWindow::rotateMapWithY(QImage map)
{ // 沿Y轴翻转
    QImage res = map;
    for (int x = 0; x < map.width(); x++) {
        for (int y = 0; y < map.height(); y++) {
            res.setPixelColor(x, map.height() - y - 1, map.pixel(x, y));
        }
    }
    return res;
}

// 四元数转欧拉角，传入四元数，返回欧拉角
EulerAngle quaternionToEuler(Quaternion q)
{
    // EulerAngle euler;
    //  计算yaw、pitch、roll
    e.yaw = atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z));
    e.pitch = asin(2 * (q.w * q.y - q.z * q.x));
    e.roll = atan2(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y));
    return e;
}
// 欧拉角转四元数，传入欧拉角，返回四元数
Quaternion ToQuaternion(double yaw, double pitch, double roll)
{
    // Abbreviations for the various angular functions
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);

    Quaternion q;
    q.w = cy * cp * cr + sy * sp * sr;
    q.x = cy * cp * sr - sy * sp * cr;
    q.y = sy * cp * sr + cy * sp * cr;
    q.z = sy * cp * cr - cy * sp * sr;

    return q;
}

// 订阅地图信息
void MainWindow::read_map()
{
    qDebug() << "read_map";
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

// 发送头部消息
void MainWindow::psc_angle_control(QString level, QString pitch)
{
    QString data = QString("{\"msg\":{\"set_level\":%1,\"set_pitch\":%2},\"op\":\"publish\",\"topic\":\"/PSC_angle_control\"}").arg(level).arg(pitch);
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
