#include <stdio.h>
#include <stdlib.h>
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
            read_all(sock, salt, nlen);

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

            nlen = HASH_SIZE;
            net_len = htonl(nlen);
            write(sock, &net_len, sizeof(net_len));
            write(sock, hash_value, nlen);
}

void print_user_list(int sock){
    int32_t nlen, net_len;
    int user_cnt;
    char user_name[NAME_SIZE], recv_buf[256];

    // 1) 유저 수 받기
    read_all(sock, &net_len, sizeof(net_len));
    nlen = ntohl(net_len);
    read_all(sock, recv_buf, nlen);
    recv_buf[nlen] = '\0';

    user_cnt = atoi(recv_buf);
    printf("\n--- 접속자: %d명 ---\n", user_cnt);

    // 2) 각 사용자 이름 출력
    for(int i=0; i<user_cnt; i++){
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);

        read_all(sock, user_name, nlen);
        user_name[nlen] = '\0';

        printf(" - %s\n", user_name);
    }

    printf("-----------------------\n");
}

void print_room_list(int sock){
    int32_t nlen, net_len;
    int room_cnt;
    char flag = 0;
    char room_name[NAME_SIZE], recv_buf[256];

    read(sock, &flag, 1);

    if(flag == 1){
        // 1) 채팅방 개수 받기
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(sock, recv_buf, nlen);
        recv_buf[nlen] = '\0';

        room_cnt = atoi(recv_buf);
        printf("\n--- 채팅방: %d개 ---\n", room_cnt);

        // 2) 각 사용자 이름 출력
        for(int i=0; i<room_cnt; i++){
            read_all(sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);

            read_all(sock, room_name, nlen);
            room_name[nlen] = '\0';

            printf(" - %s\n", room_name);
        }

        printf("-----------------------\n");
    }else{
        printf("\n--- 채팅방 없음 ---\n");
    }
    
}

void rm_room(int sock, int user_id){
    //printf("del request user_id : %d\n", user_id);
    int32_t net_len, nlen;
    char buf[BUF_SIZE], room_name[NAME_SIZE];
    char flag = 0;
    int room_cnt;

    read(sock, &flag, 1);
    if(flag == 1){
        // 1. 방 개수 받기
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(sock, buf, nlen);
        buf[nlen] = '\0';

        room_cnt = atoi(buf);
        printf("\n--- 내가 만든 방 %d개 ---\n", room_cnt);

        // 2. 각 방 정보 받기
        for (int i = 0; i < room_cnt; i++) {

            // room_id
            read_all(sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read_all(sock, buf, nlen);
            buf[nlen] = '\0';
            int room_id = atoi(buf);

            // room_name
            read_all(sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read_all(sock, room_name, nlen);
            room_name[nlen] = '\0';

            printf("[%d] %s\n", room_id, room_name);
        }

        printf("------------------------\n");

        memset(buf, 0x00, BUF_SIZE);
        // 3. 삭제할 방 id 입력 및 전송
        printf("삭제할 room_id 입력: ");
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = 0;

        nlen = strlen(buf);
        net_len = htonl(nlen);
        write(sock, &net_len, sizeof(net_len));
        write(sock, buf, nlen);
        
        //삭제한 방이름
        memset(room_name, 0x00, NAME_SIZE);
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(sock, room_name, nlen);
        room_name[nlen] = '\0';
        
        printf("\n--- [%s] 삭제 완료 ---\n", room_name);

    }else{
        printf("\n--- 생성한 채팅방 없음 ---\n");
    }
}

