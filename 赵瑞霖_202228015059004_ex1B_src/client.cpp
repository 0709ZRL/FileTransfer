#include <cstdio>
#include <cstdlib>
#include <string>
#include <windows.h>
#include <iostream>
#include "settings.h"
#include "mainwindow.h"
#include <QDebug>

using namespace std;

#define PORT 9000

inline int MakeClient(int& sock, RSA* &pubKey, RSA* &priKey, WSADATA& ws, string IP, unsigned int& pointer, unsigned int& ack, AES_KEY& encrypto){
    SOCKET csock;
    sockaddr_in caddr;
    char identification[] = "client";
    char server_pub_key_file_name[] = "server-pub.key";
    char client_pri_key_file_name[] = "client-pri.key";
    char AESKey[] = "ABCDEFGHIJKLMNOP";
    char message[LENGTH], buf[LENGTH];
    AES_set_encrypt_key((const unsigned char*)AESKey, 128, &encrypto);
    FILE* pubFile = fopen(server_pub_key_file_name, "rb");
    FILE* priFile = fopen(client_pri_key_file_name, "rb");
    if(ReadPublicKey(pubFile, pubKey)<0){
        cout<<"Read server's public key failed."<<endl;
        return Error(READ_KEY_FILE_ERROR);
    }
    if(ReadPrivateKey(priFile, priKey)<0){
        cout<<"Read client's private key failed."<<endl;
        return Error(READ_KEY_FILE_ERROR);
    }
    pointer = getRand();
    WSAStartup(MAKEWORD(2,2), &ws);

    sock = CreateConnection(IP.c_str(), PORT);
    if(sock<0) return SOCKET_OPEN_ERROR;
    cout<<"Connection succeeded!"<<endl;
    if(TransferStart(sock, message, buf, pointer, ack, pubKey, priKey, identification)<0){
        cout<<"Remote host terminated the connection."<<endl;
        return Error(ACCIDENTAL_TERMINATION_ERROR);
    }
    return 1;
}

inline int ClientFileTransfer(int& sock, RSA* &pubKey, RSA* &priKey, char* fileName, AES_KEY& encrypto, unsigned int& pointer, unsigned int& ack, string IP){
    int n;
    unsigned int type, length, seq;
    FILE* uploadFile = NULL; //文件指针
    bool fileFound = false; //代表文件是否成功被找到
    bool ConnectionClosed = false; //代表服务器端是否主动放弃了连接
    char AESKey[] = "ABCDEFGHIJKLMNOP";
    char targetFileName[1024];
    char message[LENGTH], buf[LENGTH];
    unsigned char data[1024], encryptedData[1024];
    //发送用自己的私钥加密，再用公钥加密的会话密钥（现在发现只能一次加密，两次加密会出错，详见该函数的注释）
    KeySessionset(AESKey,pubKey,message,sock,pointer,ack);

    uploadFile = fopen(fileName, "rb");
    if(uploadFile!=NULL)fileFound = true;
    else return -1;

    if(IP=="127.0.0.1")strcpy(targetFileName,"transferedData");
    else strcpy(targetFileName, fileName);

    CreateMessage(FILENAME_TRANSFER,targetFileName,sizeof(targetFileName),pointer,ack,message);
    send(sock, message, LENGTH, 0);

    //发送文件大小
    unsigned long long file_size = FileSize(fileName);
    CreateMessage(FILESIZE,(char*)&file_size,8,pointer,ack,message);
    send(sock, message, LENGTH, 0);

    //开始传输文件
    while(!(feof(uploadFile))){ //传输文件
        n = fread(data, sizeof(char), 1024, uploadFile);
        AES_encrypt(data, data, &encrypto);
        CreateMessage(DATA_TRANSFER, (char*)data, n, pointer, ack, message);
        //CreateMessage(DATA_TRANSFER, (char*)data, n, pointer, message); 非加密传输
        send(sock, message, LENGTH, 0);
        if(recv(sock,buf,LENGTH,0)<0){
            cout<<"An error occured from the server because it didn't response us."<<endl;
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
    cout<<"File Transmission succeeded."<<endl;
    ConnectionClose(sock, uploadFile, message, buf, pointer, ack);
    WSACleanup();
    return 1;
}
