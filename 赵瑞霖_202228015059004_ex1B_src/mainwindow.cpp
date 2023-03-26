#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTextBrowser>
#include <winsock.h>
#include <string>
#include <windows.h>
#include <iostream>
#include <QDebug>
#include <QCloseEvent>

#ifdef __cplusplus
extern "C"{
#endif
#include "openssl/applink.c"
#ifdef __cplusplus
}
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->Transmit,&QPushButton::clicked,this,&MainWindow::FileTrasfer);
    connect(ui->OpenPort,&QPushButton::clicked,this,&MainWindow::OpenPort);
    connect(ui->Connection,&QPushButton::clicked,this,&MainWindow::Connect);
    connect(this,&MainWindow::m,ui->Console,&QTextBrowser::append);
}

MainWindow::~MainWindow()
{
    delete ui;
}

int MainWindow::MakeClient(int& sock, RSA* &pubKey, RSA* &priKey, WSADATA& ws, string IP, unsigned int& pointer, unsigned int& ack, AES_KEY& encrypto){
    SOCKET csock;
    sockaddr_in caddr;
    char identification[] = "client";
    char server_pub_key_file_name[] = "server-pub.key";
    char client_pri_key_file_name[] = "client-pri.key";
    char AESKey[] = "ABCDEFGHIJKLMNOP";
    char message[LENGTH], buf[LENGTH];
    AES_set_encrypt_key((const unsigned char*)AESKey, 128, &encrypto);
    QString text = "Server:Connection succeeded.";
    FILE* pubFile = fopen(server_pub_key_file_name, "rb");
    FILE* priFile = fopen(client_pri_key_file_name, "rb");
    if(ReadPublicKey(pubFile, pubKey)<0){
        text = "Read server's public key failed.";
        qDebug()<<text<<endl;
        emit m(text);
        return Error(READ_KEY_FILE_ERROR);
    }
    if(ReadPrivateKey(priFile, priKey)<0){
        text = "Read client's private key failed.";
        qDebug()<<text<<endl;
        emit m(text);
        return Error(READ_KEY_FILE_ERROR);
    }
    pointer = getRand();
    WSAStartup(MAKEWORD(2,2), &ws);

    sock = CreateConnection(IP.c_str(), PORT);
    if(sock<0) return SOCKET_OPEN_ERROR;
    qDebug()<<"Connection succeeded!"<<endl;
    if(TransferStart(sock, message, buf, pointer, ack, pubKey, priKey, identification)<0){
        text = "Remote host terminated the connection.";
        qDebug()<<text<<endl;
        emit m(text);
        return Error(ACCIDENTAL_TERMINATION_ERROR);
    }
    text = "Server:Connection succeeded.";
    emit MainWindow::m(text);
    return 1;
}

int MainWindow::ClientFileTransfer(int& sock, RSA* &pubKey, RSA* &priKey, char* fileName, AES_KEY& encrypto, \
                                   unsigned int& pointer, unsigned int& ack, string IP, FILE* &uploadFile, bool& isFirstTime){
    int n;
    unsigned int type, length, seq;
    bool fileFound = false; //代表文件是否成功被找到
    bool ConnectionClosed = false; //代表服务器端是否主动放弃了连接
    char AESKey[] = "ABCDEFGHIJKLMNOP";
    char targetFileName[1024];
    char message[LENGTH], buf[LENGTH];
    unsigned char data[1024], encryptedData[1024];
    QString text;
    //发送用自己的私钥加密，再用公钥加密的会话密钥（现在发现只能一次加密，两次加密会出错，详见该函数的注释）
    if(isFirstTime)KeySessionset(AESKey,pubKey,message,sock,pointer,ack);
    isFirstTime = false;

    uploadFile = fopen(fileName, "rb");
    if(uploadFile!=NULL)fileFound = true;
    else return -1;
    qDebug()<<"The file has been found."<<endl;

    if(IP=="127.0.0.1")strcpy(targetFileName,"transferedData");
    else strcpy(targetFileName, fileName);

    CreateMessage(FILENAME_TRANSFER,targetFileName,sizeof(targetFileName),pointer,ack,message);
    send(sock, message, LENGTH, 0);
    text = "The name of file has been transferred.";
    emit m(text);
    //发送文件大小
    unsigned long long file_size = FileSize(fileName);
    CreateMessage(FILESIZE,(char*)&file_size,8,pointer,ack,message);
    send(sock, message, LENGTH, 0);
    text = "The size of file has been transferred.";
    emit m(text);
    //开始传输文件
    while(!(feof(uploadFile))){ //传输文件
        QApplication::processEvents();
        n = fread(data, sizeof(char), 1024, uploadFile);
        AES_encrypt(data, data, &encrypto);
        CreateMessage(DATA_TRANSFER, (char*)data, n, pointer, ack, message);
        //CreateMessage(DATA_TRANSFER, (char*)data, n, pointer, message); 非加密传输
        send(sock, message, LENGTH, 0);
        if(recv(sock,buf,LENGTH,0)<0){
            text = "An error occured from the server because it didn't response us.";
            qDebug()<<text<<endl;
            emit m(text);
            return NO_REPLY_ERROR;
        }
        else{
            type = buf[0];
            length = charToInt(buf+1, 2);
            seq = charToInt(buf+3, 4);
            if(type==DATA_TRANSFER_ACK&&(ack==seq)){
                //ack = charToInt(buf+7, 4);
                ack = seq+1;
                continue;
            }
        }
    }
    text = "File Transmission succeeded!!! Congratulations!!!";
    qDebug()<<text<<endl;
    ConnectionClose(sock,uploadFile,message,buf,pointer,ack,false);
    emit m(text);
    return 1;
}

SOCKET MainWindow::MakeServer(int& sock, RSA* &pubKey, RSA* &priKey, WSADATA& ws){
    char client_pub_key_file_name[] = "client-pub.key";
    char server_pri_key_file_name[] = "server-pri.key";
    short port = 9000;
    SOCKET csock;
    WSAStartup(MAKEWORD(2,2), &ws);
    FILE* pubFile = fopen(client_pub_key_file_name, "rb");
    FILE* priFile = fopen(server_pri_key_file_name, "rb");
    QString text;
    char so[7];
    fwrite(so,sizeof(char),5,pubFile);
    if(ReadPublicKey(pubFile, pubKey)<0){
        text = "Read client's public key failed.";
        qDebug()<<text<<endl;
        emit m(text);
        return Error(READ_KEY_FILE_ERROR);
    }
    if(ReadPrivateKey(priFile, priKey)<0){
        text = "Read server's private key failed.";
        qDebug()<<text<<endl;
        emit m(text);
        return Error(READ_KEY_FILE_ERROR);
    }

    if(CreateServer(sock, csock, port)<0){
        text = "Server creation failed.";
        qDebug()<<text<<endl;
        return Error(SOCKET_OPEN_ERROR);
        emit m(text);
    }
    text = "Server creation succeeded.";
    emit m(text);
    return csock;
}

int MainWindow::ServerReceiveFile(SOCKET csock, int sock, RSA* &pubKey, RSA* &priKey, FILE* &downloadData){
    char identification[] = "server";
    char fileName[1024] = {0};
    char AESKey[17] = {0};
    char buf[LENGTH] = {0}, message[LENGTH] = {0}, data[1024] = {0};
    //unsigned int pointer = 0, ack = 0, seq = 0;
    int n, length, type, stage = 0;
    unsigned int pointer = getRand(), ack = 0, seq = 0; //pointer代表当前server端发送报文的序号，ack代表收到的来自client端的报文序号
    unsigned long long fileSize = 0, receivedFileSize = 0; //预计接收的文件大小
    //在收到报文并确认其有效（pointer==ack）后，发送报文（也可以不发），pointer+=length
    AES_KEY decrypto;
    QString text;
    while(1){
        QCoreApplication::processEvents();
        n = recv(csock, buf, LENGTH, 0);
        if(n>0){
            type = buf[0];
            length = charToInt(buf+1, 2);
            seq = charToInt(buf+3, 4);
            ack = charToInt(buf+7, 4);
            //printf("我收到了type为%d的报文。\n", type);
            if(type==CONNECTION_REQUEST&&stage==0){
                ack = seq+1;
                CreateMessage(CONNECTION_REQUEST_AGREE,buf,0,pointer,ack,message);
                send(csock,message,LENGTH,0);
                text = "Connection has established from server, now prepare to transfer the file";
                qDebug()<<text<<endl;
                //证明是文件名
                //downloadData = fopen(buf+1,"ab");
                //pointerIncrease(pointer, 0);
                emit m(text);
                stage = 1;
            }
            else if(type==FILENAME_TRANSFER){
                if((stage==3&&ack==pointer&&ack!=0)||(stage == 6)){
                    pointer = ack;
                    text = "Has received the name of file from client. creating file...";
                    //printf("Has received the name of file from client: %s, creating file...\n", buf+11);
                    qDebug()<<text<<endl;
                    qDebug()<<pointer<<' '<<ack<<' '<<' '<<seq<<endl;
                    //pointerIncrease(pointer, length); //这里也许要改
                    downloadData = fopen(buf+11, "ab");
                    emit m(text);
                    stage = 4;
                }
            }
            else if(type==FILESIZE&&stage==4){
                receivedFileSize = 0;
                memcpy((char*)&fileSize, buf+11, sizeof(unsigned long long));
                stage = 5;
            }
            else if(type==DATA_TRANSFER && ack==pointer && ack!=0 && stage == 5){
                ack = seq+1;
                AES_decrypt((unsigned char*)(buf+11), (unsigned char*)(buf+11), &decrypto);
                fwrite((buf+11), sizeof(char), length, downloadData);
                receivedFileSize += length;
                CreateMessage(DATA_TRANSFER_ACK,NULL,12,pointer,ack,message);
                send(csock,message,12,0);
                //pointerIncrease(pointer, length);
                /*非加密传输
                fwrite((buf+7), sizeof(char), length, downloadData);
                printf("%d\n",length);*/
            }
            else if(type==DATA_TRANSFER_END && ack==pointer && stage == 5){
                ack = seq+1;
                qDebug()<<"received:"<<receivedFileSize<<' '<<"target:"<<fileSize<<endl;
                if(receivedFileSize == fileSize){
                    CreateMessage(DATA_TRANSFER_END_AGREE,buf,0,pointer,ack,message);
                    send(csock,message,LENGTH,0);
                    text = "Transmission completed successfully from server.";
                    qDebug()<<text<<endl;
                    emit m(text);
                    stage = 6;
                }
            }
            else if(type==IDENTIFICATION && ack==pointer && ack!=0 && stage == 1){
                //对面发来了加密版的自己的身份信息。
                pointer = ack;
                ack = seq+1;
                length = decrypt(priKey, (unsigned char*)(buf+11), (unsigned char*)data, length);
                if(length<=0){
                    text = "Decryption failed.";
                    qDebug()<<text<<endl;
                    emit m(text);
                    return Error(DECRYPTION_ERROR);
                }
                strcat(data,identification);
                length = encrypt(pubKey,(unsigned char*)data,(unsigned char*)buf,strlen(data));
                if(length<=0){
                    text = "Encryption failed.";
                    qDebug()<<text<<endl;
                    emit m(text);
                    return Error(ENCRYPTION_ERROR);
                }
                CreateMessage(IDENTIFICATION,buf,length,pointer,ack,message);
                send(csock,message,LENGTH,0);
                stage = 2;
            }
            else if(type==KEY_SESSION && strlen(AESKey)==0 && stage == 2){ //会话密钥的传递
                //pointer++;
                length = decrypt(priKey, (unsigned char*)(buf+11), (unsigned char*)data, length);
                if(length<=0){
                    text = "Decryption failed.";
                    qDebug()<<text<<endl;
                    emit m(text);
                    return Error(DECRYPTION_ERROR);
                }
                memcpy(AESKey,data,length);
                AES_set_decrypt_key((const unsigned char*)AESKey, 128, &decrypto);
                //pointerIncrease(pointer, length);
                stage = 3;
            }
            else if(type==DATA_TRANSFER_END_AGREE && stage == 6){
                text = "Remote host agreed your termination request.";
                qDebug()<<text<<endl;
                stage = 0;
                emit m(text);
                break;
            }
        }
    }
    fclose(downloadData);
    closesocket(csock);
    closesocket(sock);
    WSACleanup();
    return 1;
}


void MainWindow::closeEvent(QCloseEvent *event){
    char buf[LENGTH] = {0}, message[LENGTH] = {0};
    QString text = "The client closed the connection proactively.";
    qDebug()<<text<<endl;
    if(isConnected){
        ConnectionClose(sock, uploadData, message, buf, pointer, ack, true);
        WSACleanup();
    }
    event->accept();
}

//槽函数定义
void MainWindow::FileTrasfer(){
    QString filepath = ui->FilePath->text();
    QString ip = ui->TargetIP->text();
    //qDebug()<<filepath<<' '<<ip<<endl;
    if(filepath.isEmpty()&&ip.isEmpty())QMessageBox::information(NULL,"通知","您还没有输入文件路径和IP地址!");
    else if(filepath.isEmpty())QMessageBox::information(NULL,"通知","您还未输入要传输的文件！");
    else if(ip.isEmpty())QMessageBox::information(NULL,"通知","您还没有输入目的IP地址！");
    if(isClient&&!(isConnected))QMessageBox::information(NULL,"通知","您还没有连接！");
    else{
        string FileName = filepath.toStdString(), IP = ip.toStdString();
        ClientFileTransfer(sock, pubKey, priKey, (char*)FileName.data(), encrypto, pointer, ack, IP, uploadData, isFirstTime);
    }
}

void MainWindow::OpenPort(){
    WSADATA ws;
    if(isClient&&isConnected)QMessageBox::information(NULL,"通知","您已作为客户端开放端口，不能再转换模式！");
    else isClient = false;
    if((csock=MakeServer(sock, pubKey, priKey, ws))<0){
        QMessageBox::information(NULL,"通知","Connection Error！");
    }
    else{
        QMessageBox::information(NULL,"通知","连接成功！");
        ServerReceiveFile(csock, sock, pubKey, priKey, downloadData);
    }
}

void MainWindow::Connect(){
    WSADATA ws;
    QString ip = ui->TargetIP->text();
    if(ip.isEmpty())QMessageBox::information(NULL,"通知","您还没有输入IP地址!");
    else{
        if(MakeClient(sock, pubKey, priKey, ws, ip.toStdString(), pointer, ack, encrypto)<0)QMessageBox::information(NULL,"通知","Connection Error！");
        else{
            QMessageBox::information(NULL,"通知","连接成功！");
            isConnected = true;
        }
    }
}
