#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , recoder_(nullptr)
{
    ui->setupUi(this);

    recoder_.reset(new recoder());
    record_option ro;
    ro.init(true, false, false, {100, 1});

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

    recoder_->init(ro, video_callback, camera_callback, stop_callback);

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
    QMessageBox::information(this, QString("录制完毕"), QString("感谢使用我们的产品"));
}

void MainWindow::on_pb_begin_clicked()
{
    recoder_->record();
}

void MainWindow::on_pushButton_clicked()
{
    recoder_->stop();
}
