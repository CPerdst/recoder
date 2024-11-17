#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include "recoder.h"
#include "QDebug"
#include "eventCapturer.h"

extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/log.h"
    #include "libswscale/swscale.h"
}

// 自定义日志回调函数
void ffmpeg_log_callback(void* ptr, int level, const char* fmt, va_list vargs) {
    // 过滤日志级别，可以根据需要调整
    if (level > av_log_get_level()) {
        return;
    }

    // 将 FFmpeg 的日志级别转换为 Qt 的日志级别
    QString msg = "";
    switch (level) {
        case AV_LOG_QUIET:
            return; // 不输出
        case AV_LOG_PANIC:
        case AV_LOG_FATAL:
            msg = QString("[FATAL] ");
            break;
        case AV_LOG_ERROR:
            msg = QString("[ERROR] ");
            break;
        case AV_LOG_WARNING:
            msg = QString("[WARNING] ");
            break;
        case AV_LOG_INFO:
            msg = QString("[INFO] ");
            break;
        case AV_LOG_VERBOSE:
            msg = QString("[VERBOSE] ");
            break;
        case AV_LOG_DEBUG:
            msg = QString("[DEBUG] ");
            break;
        default:
            msg = QString("[LOG] ");
            break;
    }

    // 格式化日志消息
    char message[1024];
    vsnprintf(message, sizeof(message), fmt, vargs);
    msg += QString(message);

    // 将日志信息输出到 qDebug()
    if (level <= AV_LOG_INFO) {
        qDebug().noquote() << msg;
    } else if (level <= AV_LOG_WARNING) {
        qWarning().noquote() << msg;
    } else if (level <= AV_LOG_ERROR) {
        qCritical().noquote() << msg;
    } else {
        // 其他级别可以选择忽略或用 qDebug() 输出
        qDebug().noquote() << msg;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , recoder_(nullptr)
{
    ui->setupUi(this);
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(ffmpeg_log_callback);
//    graber_.reset(new graber());
//    grab_option ro;
//    ro.init(false, true, false, {25, 1});

    recoder_.reset(new recoder());


    auto video_callback = [this](AVFrame* f){
        QImage image = QImage(f->data[0], f->width, f->height, QImage::Format::Format_RGB888);
        emit SIG_send_window_image(image);
    };

    auto camera_callback = [this](AVFrame* f){
        QImage image = QImage(f->data[0], f->width, f->height, QImage::Format::Format_RGB888);
        emit SIG_send_camera_image(image);
    };

    auto stop_callback = [this](){
        emit SIG_record_stopped();
    };

    recoder_->init("C:\\Users\\l1Akr\\Documents\\QtApp\\build-lkmedialib-Desktop_Qt_5_12_10_MinGW_32_bit-Debug\\debug\\output.mp4",
                   true,
                   true,
                   false,
                   false,
                   {25, 1},
                   video_callback,
                   camera_callback,
                   stop_callback);

//    graber_->init(ro, video_callback, camera_callback, stop_callback);

    connect(this, SIGNAL(SIG_send_camera_image(QImage)),
            this, SLOT(SLOT_receive_camera_image(QImage)));
    connect(this, SIGNAL(SIG_send_window_image(QImage)),
            this, SLOT(SLOT_receive_window_image(QImage)));
    connect(this, SIGNAL(SIG_record_stopped()),
            this, SLOT(SLOT_record_stopped()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SLOT_receive_camera_image(QImage image)
{
    const QPixmap pix = QPixmap::fromImage(image.scaled(ui->lb_show->width(),
                                                      ui->lb_show->height(),
                                                      Qt::KeepAspectRatio));
    ui->lb_show->setPixmap(pix);
}

void MainWindow::SLOT_receive_window_image(QImage image)
{
    const QPixmap pix = QPixmap::fromImage(image.scaled(ui->lb_show2->width(),
                                                      ui->lb_show2->height(),
                                                      Qt::KeepAspectRatio));
    ui->lb_show2->setPixmap(pix);
}

void MainWindow::SLOT_record_stopped()
{
    QMessageBox::information(this, QString("录制完毕"), QString("感谢您的使用"));
}

void MainWindow::on_pb_begin_clicked()
{
    recoder_->record();
}

void MainWindow::on_pushButton_clicked()
{

}
