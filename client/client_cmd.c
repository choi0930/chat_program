#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "common.h"
#include "client_cmd.h"
#include "crypto_util.h"

int read_all(int sock, void *buf, int len) {//길이 4바이트를 받는 함수
    int received = 0;
    while(received < len) {
        int n = read(sock, buf + received, len - received);
        if(n <= 0) return n;  // error or disconnected
        received += n;
    }
    return received;
}

void cmd_mkroom(int sock, int user_id){
    char room_name[NAME_SIZE];
            char password[NAME_SIZE];
            unsigned char salt[KEY_SIZE];
            unsigned char aes_key[KEY_SIZE];
            unsigned char hash_value[HASH_SIZE];

            memset(room_name, 0x00, NAME_SIZE);
            memset(salt, 0x00, 16);

            printf("채팅방 이름 입력 : ");
            fgets(room_name, NAME_SIZE, stdin);
            room_name[strcspn(room_name, "\n")] = 0;

            int32_t nlen = (int32_t)strlen(room_name);
            int32_t net_len = htonl(nlen);

            write(sock, &net_len, sizeof(net_len));
            write(sock, room_name, nlen);
            printf("request user_id : %d\n", user_id);

           //salt값 받기
            read_all(sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read(sock, salt, nlen);
        
            for(int i = 0; i < 16; i++)
                printf("%02x", salt[i]);
            printf("\n");
            
            printf("채팅방 비밀번호 입력 : ");
            fgets(password, NAME_SIZE, stdin);
            password[strcspn(password, "\n")] = 0;

            if(make_aes128_key(password, salt, nlen, aes_key, KEY_SIZE) == 0){
                printf("KEY: ");
                for (int i = 0; i < 16; i++)
                    printf("%02x", aes_key[i]);
                printf("\n");
            }else{
                printf("키 생성 실패\n");
            }

            if(sha256_hash(aes_key, KEY_SIZE, hash_value) == 0){
              printf("hash_value: ");
                for (int i = 0; i < 32; i++)
                    printf("%02x", hash_value[i]);
                printf("\n");
            }else{
                printf("hash fail\n");
            }

            nlen = (int32_t)strlen(hash_value);
            net_len = htonl(nlen);
            write(sock, &net_len, sizeof(net_len));
            write(sock, hash_value, nlen);
}