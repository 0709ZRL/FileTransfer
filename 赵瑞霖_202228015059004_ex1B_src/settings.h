/*
传输协议说明
第0字节：
为一无符号整数，代表报文行为。
0，代表请求建立连接。
1，代表同意建立连接，仅在收到0报文时回复。
2，代表进行文件名的传输。
3，代表进行文件内容的传输。
4，代表文件传输结束。
5，代表同意文件传输结束，仅在收到4报文时回复。
6，代表进行身份验证信息的传输，传输的可能是密钥或身份信息等。
7，代表中断连接，这代表着主动中断连接，或者因错误等各种情况而结束。
8，代表传输会话密钥，这代表着传输会话密钥。
9，代表文件大小
10，代表对传输文件的报文的确认，仅在收到3报文时回复。
第1，2字节：
为一无符号整数，代表报文的长度（字节），最高为1024字节。
第3，4，5，6字节：
为一无符号整数，代表文件传输位置的指针。PTR
第7，8，9，10字节：
为一无符号整数，代表确认号。
第11——11+1023字节：
为文件主体内容。
*/
#include <iostream>
#include <sys\stat.h>
#include <ctime>
#include "cryptolib.h"
#include <QApplication>
#pragma once

using namespace std;

#define LENGTH 1024+12
#define CONNECTION_REQUEST 0
#define CONNECTION_REQUEST_AGREE 1
#define FILENAME_TRANSFER 2
#define DATA_TRANSFER 3
#define DATA_TRANSFER_END 4
#define DATA_TRANSFER_END_AGREE 5
#define IDENTIFICATION 6
#define CONNECTION_BREAK 7
#define KEY_SESSION 8
#define FILESIZE 9
#define DATA_TRANSFER_ACK 10

#define READ_KEY_FILE_ERROR -1
#define SOCKET_OPEN_ERROR -2
#define SOCKET_BIND_ERROR -3
#define SOCKET_LISTEN_ERROR -4
#define SOCKET_ACCEPT_ERROR -5
#define ENCRYPTION_ERROR -6
#define DECRYPTION_ERROR -7
#define DATA_READ_ERROR -8
#define DATA_CLOSE_ERROR -9
#define NO_REPLY_ERROR -10
#define ACCIDENTAL_TERMINATION_ERROR -11
#define IDENTIFICATION_ERROR -12
#define CONNECTION_ERROR -13
#define OTHERS -14

inline unsigned int getRand(){
    srand(time(NULL));
    return rand();
};

inline int CreateServer(int& sock, SOCKET& csock, u_short port){
    //这个函数与下面的函数不同之处在于，这个函数是服务器端（接收者）使用的，下面的函数是客户端（发送者）使用的。
    sock=socket(AF_INET, SOCK_STREAM, 0);
    if(sock==-1){
        cout<<"Create failed."<<endl;
        return SOCKET_OPEN_ERROR;
    }
    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = htonl(0);
    if(bind(sock,(sockaddr*)&saddr,sizeof(saddr))<0){
        cout<<"Bind failed."<<endl;
        return SOCKET_BIND_ERROR;
    }
    if(listen(sock, 10)==SOCKET_ERROR){
        cout<<"Listen failed."<<endl;
        return SOCKET_LISTEN_ERROR;
    }
    sockaddr_in caddr;
    int caddr_len=sizeof(caddr);
    csock = accept(sock, (sockaddr*)&caddr, &caddr_len);
    if(csock==INVALID_SOCKET){
        cout<<"Accept failed."<<endl;
        return SOCKET_ACCEPT_ERROR;
    }
    return 1;
};

inline int CreateConnection(const char* ip, int port){
    //这个函数与上面的函数不同之处在于，上面的函数是服务器端（接收者）使用的，这个函数是客户端（发送者）使用的。
    int sock=socket(AF_INET, SOCK_STREAM, 0);
    if(sock==-1){
        cout<<"Create failed."<<endl;
        closesocket(sock);
        return SOCKET_OPEN_ERROR;
    }
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.S_un.S_addr = inet_addr(ip);
    if(connect(sock,(sockaddr*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR){
        cout<<"Connection failed."<<endl;
        closesocket(sock);
        return CONNECTION_ERROR;
    }
    return sock;
};

inline void intToChar(int num, char* target, int size){
    if(size==2){
        memcpy(target, (unsigned short*)&num, size);
    }
    else if(size==4){
        memcpy(target, (int*)&num, size);
    }
};

inline int charToInt(char *target, int size){
    int num = 0;
    if(size==2){
        memcpy((int*)&num, target, 2);
        /*num = (0|target[0])<<8;
        num = num|(0|target[1]);*/
    }
    else if(size==4){
        memcpy((int*)&num, target, 4);
        /*num = (0|target[0])<<24;
        num = num|((0|target[1])<<16);
        num = num|((0|target[2])<<8);
        num = num|(0|target[3]);*/
    }
    return num;
};

inline unsigned int pointerIncrease(unsigned int& pointer, unsigned int length){
    /*if(length==0)pointer++;
    else pointer+=length;*/
    pointer++;
};

inline void CreateMessage(int type, char* data, unsigned int length, unsigned int& pointer, unsigned int ack, char* message){
    /*
    type: 报文的类型。
    data: 需要传输的数据。仅在type=2,3时有效。
    length: 报文内容的长度。
    pointer: 报文指针。
    message: 报文存储的位置。
    */
    memset(message, 0, sizeof(message));
    message[0] = (char)type;
    intToChar(length, message+1, 2);
    //printf("%x   %x", message[1],message[2]);
    intToChar(pointer, message+3, 4);
    intToChar(ack, message+7, 4);
    if(type==FILENAME_TRANSFER||type==KEY_SESSION||type==IDENTIFICATION||type==FILESIZE){
        memcpy(message+11, data, length);
    }
    else if(type==DATA_TRANSFER){ //因为使用AES加密时，它的函数并没有返回密文的长度，因此为了防止文件损坏，只能把整个数组全都复制过去
        memcpy(message+11, data, length);
    }
    pointerIncrease(pointer, length); //我们在这里做出强制规定，指针的增长必须在这里进行。
};

inline int TransferStart(int& socket, char* message, char* buffer, unsigned int &pointer, \
    unsigned int &ack, RSA* pubKey, RSA* priKey, char* identification){
    /*
    说明：该函数为客户端使用。
    客户端首先给服务器发送0报文请求连接。
    若服务器返回1报文，代表其同意链接。
    此时，客户端用服务器的公钥加密自己的身份信息
    发给服务器。
    服务器获得了后用自己的私钥解密信息，将得到的
    身份信息拼接上自己的身份信息并用服务器的公钥
    加密此信息发给客户端。
    客户端拿到后，用自己的私钥解密，得到两个身份
    信息，则判断这个身份信息和自己开始发出去的是
    否一致，如果一致，则证明对方的身份。
    */
    //这里ptr=seq
    int n = 0, type, seq, length;
    unsigned char data[1024] = {0};
    //请求建立连接，类似TCP的第一次握手
    CreateMessage(CONNECTION_REQUEST,NULL,0,pointer,ack,message);
    send(socket, message, 11, 0);
    //收到建立连接的回复，类似TCP的第二次握手
    n = recv(socket, buffer, LENGTH, 0);
    if(n<0){
        cout<<"No message received, transmission aborted."<<endl;
        return NO_REPLY_ERROR;
    }
    type = buffer[0];
    seq = charToInt(buffer+3, 4);
    ack = charToInt(buffer+7, 4);
    if(type!=CONNECTION_REQUEST_AGREE||ack!=pointer)return OTHERS;
    //加密自己的身份信息并发送给服务器，类似TCP的第三次握手
    ack = seq+1;
    length = encrypt(pubKey, (unsigned char*)identification, data, sizeof(identification));
    CreateMessage(IDENTIFICATION, (char*)data, length, pointer, ack, message);
    send(socket, message, LENGTH, 0);
    //收到来自服务器的报文，第四次握手
    n = recv(socket, buffer, LENGTH, 0);
    if(n<0){
        cout<<"No encrypted message received, transmission aborted."<<endl;
        return NO_REPLY_ERROR;
    }
    type = buffer[0];
    seq = charToInt(buffer+3, 4);
    ack = charToInt(buffer+7, 4);
    if(type!=IDENTIFICATION||ack!=pointer)return OTHERS;
    //对服务器的报文进行解密
    unsigned char text[1024] = {0};
    length = charToInt(buffer+1, 2);
    n = decrypt(priKey, (unsigned char*)(buffer+11), text, length);
    if(n<=0){
        cout<<"Decryption failed."<<endl;
        return DECRYPTION_ERROR;
    }
    //判断对方发来的身份信息和自己的身份信息是否一致
    for(int i=0;i<strlen(identification);i++){
        if(identification[i]!=text[i]){
            cout<<"Identification differs from the former one."<<endl;
            return IDENTIFICATION_ERROR;
        }
    }
    ack = seq+1;
    return 1;
};

inline int KeySessionset(char* AESKey, RSA* pubKey, char* message, int sock, unsigned int& pointer, unsigned int& ack){ //进行会话密钥的传递
    //发送用自己的私钥加密，再用公钥加密的会话密钥
    //贴士：问题就出现在这里，不知何故，在私钥加密后无法再用公钥加密，很奇怪。只好退而求其次，只用公钥加密，不进行第二次加密。
    //length = encrypt_private(priKey,(unsigned char*)AESKey,(unsigned char*)message,strlen(AESKey));
    //if(length<=0)return -1;
    int length;
    unsigned char data[1024] = {0};
    memset(message, 0, sizeof(message));
    length = encrypt(pubKey,(unsigned char*)AESKey,data,strlen(AESKey));
    if(length<=0)return ENCRYPTION_ERROR;
    CreateMessage(KEY_SESSION,(char*)data,length,pointer,ack,message);
    send(sock, message, LENGTH, 0);
};

inline void ConnectionClose(int sock, FILE* file, char* message, char* buffer, unsigned int& pointer, unsigned int& ack, bool isLastTime){
    /*
     * 关闭连接。注意在本协议中，只有客户端（上传文件的一方）会主动请求关闭连接。
     * 首先，客户端发送一个DATA_TRANSFER_END类型的报文，等候服务器回复。
     * 若服务器回复了一个DATA_TRANSFER_END_AGREE类型的报文，则代表服务器同意本次文件传输的结束。
     * 只有在客户端同样回复了一个DATA_TRANSFER_END_AGREE类型的报文后，服务器端才会关闭本次连接。
    */
    if(isLastTime==false){
        int n = 0, type, ptr;
        CreateMessage(DATA_TRANSFER_END,NULL,0,pointer,ack,message);
        send(sock,message,11,0);
        while(true){
            QApplication::processEvents();
            n = recv(sock, buffer, LENGTH, 0);
            if(n<0){
                cout<<"Remote host didn't answer the request, aborted."<<endl;
            }
            else{
                type = charToInt(buffer, 2);
                ptr = charToInt(buffer+3, 4);
                ack = charToInt(buffer+7, 4);
                if(fclose(file)<0)cout<<"File close failed."<<endl;
                return;
            }
        }
    }
    else{
        cout<<"Remote host accepeted your close request."<<endl;
        CreateMessage(DATA_TRANSFER_END_AGREE, NULL, 0, pointer, ack, message);
        send(sock,message,11,0);
        closesocket(sock);
    }
};

inline int Error(int ErrorNum){
    char command = 0;
    printf("An error occured, error number: %d\nPlease input q to terminate this connection.", ErrorNum);
    while(command!='q')scanf("%c", &command);
    return ErrorNum;
};

inline unsigned long long FileSize(char* fileName){
    unsigned long long size;
    struct __stat64 st;
    __stat64(fileName, &st);
    size = st.st_size;
    return size;
};
