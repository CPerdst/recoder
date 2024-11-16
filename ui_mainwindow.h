/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.10
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QLabel *lb_show;
    QPushButton *pb_begin;
    QPushButton *pushButton;
    QLabel *lb_show2;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(2209, 1476);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        lb_show = new QLabel(centralwidget);
        lb_show->setObjectName(QString::fromUtf8("lb_show"));
        lb_show->setGeometry(QRect(0, 260, 481, 301));
        lb_show->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        pb_begin = new QPushButton(centralwidget);
        pb_begin->setObjectName(QString::fromUtf8("pb_begin"));
        pb_begin->setGeometry(QRect(80, 870, 80, 20));
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(280, 860, 99, 28));
        lb_show2 = new QLabel(centralwidget);
        lb_show2->setObjectName(QString::fromUtf8("lb_show2"));
        lb_show2->setGeometry(QRect(520, 260, 1681, 1151));
        lb_show2->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);"));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 2209, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", nullptr));
        lb_show->setText(QString());
        pb_begin->setText(QApplication::translate("MainWindow", "begin", nullptr));
        pushButton->setText(QApplication::translate("MainWindow", "stop", nullptr));
        lb_show2->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
