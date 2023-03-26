#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#pragma once
 
using namespace std;

/*#ifdef __cplusplus
extern "C"{
#endif
#include "openssl/applink.c"
#ifdef __cplusplus
}
#endif*/

inline void GetError(){ //捕获并输出产生的各种错误。
    unsigned long err= ERR_get_error(); //获取错误号
    char err_msg[1024] = { 0 };
    ERR_error_string(err, err_msg); // 格式：error:errId:库:函数:原因
    printf("Error:%ld, Message:%s\n", err, err_msg);
};

inline int GenerateKey(RSA* &pRSA, RSA* &pRSApub, RSA* &pRSApri){ //生成一对儿私钥和公钥
    pRSA = RSA_new();
    BIGNUM* pBNE = BN_new();
    BN_set_word(pBNE, RSA_3);
    if(RSA_generate_key_ex(pRSA, 512, pBNE, NULL)<=0){
        printf("RSA key generation failed.\n");
        return -1;
    }
    pRSApub = RSAPublicKey_dup(pRSA);
    pRSApri = RSAPrivateKey_dup(pRSA);
    //RSA_print_fp(stdout, pRSApri, 0);
    //printf("%p %p %p\n", pRSA, pRSApub, pRSApri);
    BN_free(pBNE);
    return 1;
};

inline int encrypt(RSA* key, unsigned char* text, unsigned char *dest, int length){ //加密，只能用公钥加密。
    int size;
    int bytes = RSA_size(key);
    unsigned char* pCipher = (unsigned char*)malloc(1024);
    memset(pCipher, 0, sizeof(pCipher));
    if((size = RSA_public_encrypt(length, (const unsigned char*)text, pCipher, key, RSA_PKCS1_PADDING))<0){
        printf("Encryption failed.\n");
        return -1;
    }
    memcpy(dest,pCipher,bytes);
    free(pCipher);
    return size;
};

inline int encrypt_private(RSA* key, unsigned char* text, unsigned char *dest, int length){ //用私钥加密的特殊版本，仅在生成会话密钥的时候起作用，但是剩下的时候都不能用。
    int size;
    int bytes = RSA_size(key);
    unsigned char* pCipher = (unsigned char*)malloc(bytes);
    memset(pCipher, 0, sizeof(pCipher));
    if((size = RSA_private_encrypt(length, (const unsigned char*)text, pCipher, key, RSA_PKCS1_PADDING))<0){
        printf("Encryption failed.\n");
        return -1;
    }
    memcpy(dest,pCipher,bytes);
    free(pCipher);
    return size;
};

inline int decrypt(RSA* key, unsigned char* text, unsigned char* dest, int length){ //解密，只能用私钥解密。
    int size = 0;
    int bytes = RSA_size(key);
    unsigned char* pCipher = (unsigned char*)malloc(bytes);
    memset(pCipher, 0, sizeof(pCipher));
    if((size = RSA_private_decrypt(length, (unsigned char*)text, pCipher, key, RSA_PKCS1_PADDING))<=0){
        GetError();
        printf("Decryption failed.\n");
        return -1;
    }
    memcpy(dest,pCipher,size);
    free(pCipher);
    return size;
};

inline int SavePublicKey(FILE* file, const RSA* key){
    if(PEM_write_RSAPublicKey(file, key)<=0){
        printf("Save public key to file failed.\n");
        return -1;
    }
    else return 1;
};

inline int SavePrivateKey(FILE* file, RSA* key){
    if(PEM_write_RSAPrivateKey(file,key,NULL,NULL,0,NULL,NULL)<=0){
        printf("Save private key to file failed.\n");
        return -1;
    }
    else return 1;
};

inline int ReadPublicKey(FILE* file, RSA* &pubKey){
    if((pubKey = PEM_read_RSAPublicKey(file, &pubKey, NULL, NULL))==NULL){
        printf("Read public key from file failed.\n");
        return -1;
    }
    else return 1;
};

inline int ReadPrivateKey(FILE* file, RSA* &priKey){
    if((priKey = PEM_read_RSAPrivateKey(file, &priKey, NULL, NULL))==NULL){
        printf("Read private key from file failed.\n");
        return -1;
    }
    else return 1;
};

inline unsigned char* SHA256Hash(FILE* file){
    char buffer[1024] = {0};
    int n = 0;
    SHA256_CTX sha256;
    if(SHA256_Init(&sha256)<=0)printf("Failure.\n");
    rewind(file);
    while(!(feof(file))){
        n = fread(buffer, sizeof(char), 1024, file);
        SHA256_Update(&sha256, buffer, n);
    }
    unsigned char hash[1024] = {0};
    SHA256_Final(hash, &sha256);
    return hash;
};
