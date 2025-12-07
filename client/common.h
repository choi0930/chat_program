#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

int make_aes128_key(const char *password, const unsigned char *salt, size_t salt_len, unsigned char *key_out, size_t key_len);
int sha256_hash(const unsigned char *input, size_t input_len, unsigned char *output);

