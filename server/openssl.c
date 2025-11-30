#include "common.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

char make_salt(unsigned char *salt, size_t len){
    RAND_bytes(salt, len);
}