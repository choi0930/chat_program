#ifndef CRYPTO_UTIL_H
#define CRYPTO_UTIL_H

#include <stddef.h>

//openssl
int make_aes128_key(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *key_out, size_t key_len);
int sha256_hash(const unsigned char *input, size_t input_len, unsigned char *output);

#endif