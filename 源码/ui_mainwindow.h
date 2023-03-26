/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QWidget *gridLayoutWidget;
    QGridLayout *gridLayout_2;
    QPushButton *Transmit;
    QLabel *label;
    QTextBrowser *Console;
    QPushButton *OpenPort;
    QLabel *label_2;
    QPushButton *Connection;
    QLineEdit *TargetIP;
    QLineEdit *FilePath;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        MainWindow->setDocumentMode(false);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayoutWidget = new QWidget(centralwidget);
        gridLayoutWidget->setObjectName(QString::fromUtf8("gridLayoutWidget"));
        gridLayoutWidget->setGeometry(QRect(20, 10, 751, 531));
        gridLayout_2 = new QGridLayout(gridLayoutWidget);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        Transmit = new QPushButton(gridLayoutWidget);
        Transmit->setObjectName(QString::fromUtf8("Transmit"));

        gridLayout_2->addWidget(Transmit, 0, 0, 1, 1);

        label = new QLabel(gridLayoutWidget);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_2->addWidget(label, 1, 0, 1, 1);

        Console = new QTextBrowser(gridLayoutWidget);
        Console->setObjectName(QString::fromUtf8("Console"));

        gridLayout_2->addWidget(Console, 3, 0, 1, 3);

        OpenPort = new QPushButton(gridLayoutWidget);
        OpenPort->setObjectName(QString::fromUtf8("OpenPort"));

        gridLayout_2->addWidget(OpenPort, 0, 2, 1, 1);

        label_2 = new QLabel(gridLayoutWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_2->addWidget(label_2, 2, 0, 1, 1);

        Connection = new QPushButton(gridLayoutWidget);
        Connection->setObjectName(QString::fromUtf8("Connection"));

        gridLayout_2->addWidget(Connection, 0, 1, 1, 1);

        TargetIP = new QLineEdit(gridLayoutWidget);
        TargetIP->setObjectName(QString::fromUtf8("TargetIP"));

        gridLayout_2->addWidget(TargetIP, 1, 1, 1, 2);

        FilePath = new QLineEdit(gridLayoutWidget);
        FilePath->setObjectName(QString::fromUtf8("FilePath"));

        gridLayout_2->addWidget(FilePath, 2, 1, 1, 2);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 29));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        Transmit->setText(QCoreApplication::translate("MainWindow", "\344\274\240\350\276\223\357\274\210\345\256\242\346\210\267\347\253\257\344\270\223\347\224\250\357\274\211", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\347\233\256\347\232\204IP\345\234\260\345\235\200", nullptr));
        OpenPort->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\347\253\257\345\217\243\357\274\210\346\234\215\345\212\241\347\253\257\344\270\223\347\224\250\357\274\211", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "\344\274\240\350\276\223\346\226\207\344\273\266\350\267\257\345\276\204", nullptr));
        Connection->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245\357\274\210\345\256\242\346\210\267\347\253\257\344\270\223\347\224\250\357\274\211", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
