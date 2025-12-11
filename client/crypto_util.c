#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

#include "common.h"
#include "crypto_util.h"

unsigned char aes_key[16];

// AES-128 -> 16 bytes
int make_aes128_key(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *key_out, size_t key_len){
    
    const int iter = 100000; //반복연산 횟수 
    
    int check = PKCS5_PBKDF2_HMAC(password, strlen(password), salt, salt_len, iter, EVP_sha256(), key_len, key_out);
    
    return check == 1 ? 0 : -1;
}

int sha256_hash(const unsigned char *input, size_t input_len, unsigned char *output){
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    if (EVP_DigestUpdate(ctx, input, input_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    unsigned int output_len = 0;
    if (EVP_DigestFinal_ex(ctx, output, &output_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_MD_CTX_free(ctx);

    return (output_len == 32) ? 0 : -1;  
}
//암호화
int aes_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext){
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    // 16바이트 랜덤 IV 생성
    if (!RAND_bytes(iv, 16)) {
        return -1;
    }

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -2;

    // AES-128-CBC 초기화
    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        return -3;

    // 암호화 수행
    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        return -4;

    ciphertext_len = len;

    // 패딩 처리
    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        return -5;

    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;  // 암호문 길이 반환
}
//복호화
int aes_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext){
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    ctx = EVP_CIPHER_CTX_new();
    if(!ctx) 
        return -1;

    if(EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv) != 1)
        return -2;
    
    if(EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
        return -3;
    
    plaintext_len = len;

    if(EVP_DecryptFinal_ex(ctx, plaintext+len, &len) != 1)
        return -4;
    
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}