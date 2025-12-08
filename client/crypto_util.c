#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

#include "common.h"
#include "crypto_util.h"


int make_aes128_key(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *key_out, size_t key_len){
    
    const int iter = 100000;

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