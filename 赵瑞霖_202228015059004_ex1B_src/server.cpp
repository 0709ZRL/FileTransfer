#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <iostream>
#include <pthread.h>
#include "settings.h"
#include <QDebug>

using namespace std;

//extern WSADATA ws;
//extern RSA *pubKey = NULL, *priKey = NULL;

inline SOCKET MakeServer(int& sock, RSA* &pubKey, RSA* &priKey, WSADATA& ws){
    char client_pub_key_file_name[] = "client-pub.key";
    char server_pri_key_file_name[] = "server-pri.key";
    short port = 9000;
    SOCKET csock;
    WSAStartup(MAKEWORD(2,2), &ws);
    FILE* pubFile = fopen(client_pub_key_file_name, "rb");
    FILE* priFile = fopen(server_pri_key_file_name, "rb");

    char so[7];
    fwrite(so,sizeof(char),5,pubFile);
    if(ReadPublicKey(pubFile, pubKey)<0){
        cout<<"Read client's public key failed."<<endl;
        return Error(READ_KEY_FILE_ERROR);
    }
    if(ReadPrivateKey(priFile, priKey)<0){
        cout<<"Read server's private key failed."<<endl;
        return Error(READ_KEY_FILE_ERROR);
    }

    if(CreateServer(sock, csock, port)<0){
        cout<<"Server creation failed."<<endl;
        return Error(SOCKET_OPEN_ERROR);
    }
    return csock;
}

inline int ServerReveiveFile(SOCKET csock, int sock, RSA* &pubKey, RSA* &priKey){
    char identification[] = "server";
    char fileName[1024] = {0};
    char AESKey[17] = {0};
    char buf[LENGTH] = {0}, message[LENGTH] = {0}, data[1024] = {0};
    //unsigned int pointer = 0, ack = 0, seq = 0;
    int n, length, type, stage = 0;
    unsigned int pointer = getRand(), ack = 0, seq = 0; //pointer代表当前server端发送报文的序号，ack代表收到的来自client端的报文序号
    unsigned long long fileSize = 0, receivedFileSize = 0; //预计接收的文件大小
    //在收到报文并确认其有效（pointer==ack）后，发送报文（也可以不发），pointer+=length
    FILE* downloadData = NULL;
    AES_KEY decrypto;
    while(1){
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
                cout<<"Connection has established from server, now prepare to transfer the file."<<endl;
                //证明是文件名
                //downloadData = fopen(buf+1,"ab");
                //pointerIncrease(pointer, 0);
                stage = 1;
            }
            else if(type==FILENAME_TRANSFER && ack==pointer && ack!=0 && stage == 3){
                printf("Has received the name of file from client: %s, creating file...\n", buf+11);
                //pointerIncrease(pointer, length); //这里也许要改
                downloadData = fopen(buf+11, "ab");
                stage = 4;
            }
            else if(type==FILESIZE&&stage==4){
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
            else if(type==DATA_TRANSFER_END && stage == 5){
                printf("received:%lld target:%lld\n", receivedFileSize, fileSize);
                if(receivedFileSize == fileSize){
                    CreateMessage(DATA_TRANSFER_END_AGREE,buf,0,pointer,ack,message);
                    send(csock,message,LENGTH,0);
                    cout<<"Transmission completed successfully from server."<<endl;
                    stage = 6;
                }
            }
            else if(type==IDENTIFICATION && ack==pointer && ack!=0 && stage == 1){
                //对面发来了加密版的自己的身份信息。
                pointer = ack;
                ack = seq+1;
                length = decrypt(priKey, (unsigned char*)(buf+11), (unsigned char*)data, length);
                if(length<=0){
                    cout<<"Decryption failed."<<endl;
                    return Error(DECRYPTION_ERROR);
                }
                strcat(data,identification);
                length = encrypt(pubKey,(unsigned char*)data,(unsigned char*)buf,strlen(data));
                if(length<=0){
                    cout<<"Encryption failed."<<endl;
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
                    cout<<"Decryption failed."<<endl;
                    return Error(DECRYPTION_ERROR);
                }
                memcpy(AESKey,data,length);
                AES_set_decrypt_key((const unsigned char*)AESKey, 128, &decrypto);
                //pointerIncrease(pointer, length);
                stage = 3;
            }
            else if(type==DATA_TRANSFER_END_AGREE && stage == 6){
                cout<<"Remote host agreed your end request."<<endl;
                stage = 0;
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
