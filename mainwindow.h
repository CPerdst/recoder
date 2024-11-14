#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "recoder.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void SIG_send_camera_image(QImage image);
    void SIG_send_window_image(QImage image);
    void SIG_record_stopped();

public slots:
    void SLOT_receive_camera_image(QImage image);
    void SLOT_receive_window_image(QImage image);
    void SLOT_record_stopped();

private slots:
    void on_pb_begin_clicked();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    std::shared_ptr<recoder> recoder_;
};
#endif // MAINWINDOW_H
