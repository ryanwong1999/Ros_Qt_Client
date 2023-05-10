#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QMenu>
#include <QAction>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QDateTime>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QPointF>
#include <QSettings>
#include <QString>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
#include <QRegExpValidator>

#include <VLCQtCore/Common.h>
#include <VLCQtCore/Instance.h>
#include <VLCQtCore/Media.h>
#include <VLCQtCore/MediaPlayer.h>
#include "login.h"
#include "math.h"
#include "roboItem.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class VlcInstance;
class VlcMedia;
class VlcMediaPlayer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //初始化vlc
    void initVLC();
    //打开rtsp
    void openUrl();
    // 初始加载保存的IP和Port
    void ip_init();
    //Ui界面初始化函数
    void initUi();
    void initPSC();
    // 连接信号与槽
    void allconnect();
    // 连接Websocket后需初始化的内容
    void init();
    // 订阅地图信息
    void read_map();
    // 订阅雷达信息
    void read_laser();
    //订阅电量
    void read_pms();
    //订阅环境数据
    void read_envirment();
    // 控制速度
    void cmd_vel(QString x, QString z);
    //控制云台
    void psc_angle_control(QString level, QString pitch);
    // 订阅位置信息
    void getrobot_pose();
    // 地图信息处理函数
    void map_data_handle(QString data);
    // 位置信息处理
    void get_RobotPose(QString posedata);
    // String转json函数
    QJsonObject QstringToJson(QString jsonString);

    // 传入处理后的地图信息
    void mapCallback(QJsonValue width, QJsonValue height, QJsonValue map, QJsonValue resolution, QJsonValue positionx, QJsonValue positiony);

private:
    Ui::MainWindow *ui;

    VlcInstance *ipc_instance;
    VlcMedia *ipc_media;
    VlcMediaPlayer *ipc_player;

    VlcInstance *red_instance;
    VlcMedia *red_media;
    VlcMediaPlayer *red_player;

    login m_login;
    // Websocket
    QWebSocket m_websocket;
    // 场景
    QGraphicsScene *m_qgraphicsScene = NULL;
    // 绘画
    Ui::roboItem *m_roboItem = NULL;


private Q_SLOTS:
  // 收到message槽
  void onTextMessageReceived(const QString &message);

public slots:
    void timerUpdata(void);

private slots:
    void on_btn_watch_clicked();
    void on_btn_log_clicked();
    void on_btn_task_clicked();
    void on_btn_set_clicked();
};

// 线程类
class MyThread : public QThread
{

  public:
    MainWindow *w = NULL;
    // 接收的信息
    QString message = "";
    int select = 0;
    void run()
    {
        switch (select) {
//        case 1:
//            w->map_data_handle(message);
//            break;
//        case 2:
//            w->get_RobotPose(message);
//            break;
//        case 3:
//            w->get_car_state(message);
//            break;
//        case 4:
//            w->get_plan_data(message);
//            break;
//        case 5:
//            w->laserScan_data_handle(message);
//            break;
//        case 6:
//            w->movebase_result_data_handle(message);
//            break;
        }
    }
};
#endif // MAINWINDOW_H
