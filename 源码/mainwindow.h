#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <winsock.h>
#include "cryptolib.h"
#include "settings.h"
#include <windows.h>
#pragma once

#define PORT 9000

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    bool isClient = true;
    bool isConnected = false;
    bool isFirstTime = true;
    QString FileName, IP;
    int sock;
    SOCKET csock;
    RSA *pubKey = NULL, *priKey = NULL;
    unsigned int pointer = 0, ack = 0;
    AES_KEY decrypto, encrypto;
    FILE *uploadData = NULL, *downloadData = NULL;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int MakeClient(int& sock, RSA* &pubKey, RSA* &priKey, WSADATA& ws, string IP, unsigned int& pointer, unsigned int& ack, AES_KEY& encrypto);
    int ClientFileTransfer(int& sock, RSA* &pubKey, RSA* &priKey, char* fileName, AES_KEY& encrypto, \
                           unsigned int& pointer, unsigned int& ack, string IP, FILE* &uploadData, bool& isFirstTime);
    SOCKET MakeServer(int& sock, RSA* &pubKey, RSA* &priKey, WSADATA& ws);
    int ServerReceiveFile(SOCKET csock, int sock, RSA* &pubKey, RSA* &priKey, FILE* &downloadData);

signals:
    void m(QString text);
private:
    Ui::MainWindow *ui;

private slots:
    void FileTrasfer();
    void Connect();
    void OpenPort();
protected:
    void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
