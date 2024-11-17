#include "mainwindow.h"

#include <QApplication>
#include "QDebug"
#include "eventCapturer.h"

void lk_log_callback(std::string& str){
    qDebug() << str.c_str();
}

void lk_log_init(){
    logger::logger::Root()->setConsoleCallback(lk_log_callback);
    logger::logger::Root()->setLevel(packer::Trace);
    logger::logger::Root()->setLogFormater("[%level] [%s {%Y-%m-%d %H:%M:%S}] [%filepath:%line]: %message");
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    lk_log_init();
    MainWindow w;
    w.show();
    return a.exec();
}
