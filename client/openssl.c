#include "common.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

int make_aes128_key(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *key_out, size_t key_len){
    
    const int iter = 100000;

    int check = PKCS5_PBKDF2_HMAC(password, strlen(password), salt, salt_len, iter, EVP_sha256(), key_len, key_out);
    
    return check == 1 ? 0 : -1;
}