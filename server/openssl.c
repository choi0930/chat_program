#include "common.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

void make_salt(unsigned char *salt, size_t len){
    RAND_bytes(salt, len);
}