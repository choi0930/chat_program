#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "common.h"
#include "crypto_util.h"
#include "server_cmd.h"

extern int room_cnt;
extern int clnt_cnt;
extern ClientInfo clients[MAX_CLNT]; 
extern ChatRoomInfo rooms[BUF_SIZE];

extern pthread_mutex_t mutx;
extern pthread_mutex_t mKchat_room_mutx;
extern pthread_mutex_t roomId_file_mutx;

int read_all(int sock, void *buf, int len) {//길이 4바이트를 받는 함수
    int received = 0;
    while(received < len) {
        int n = read(sock, buf + received, len - received);
        if(n <= 0) return n;  // error or disconnected
        received += n;
    }
    return received;
}

void cmd_mkroom(int clnt_sock, int user_id){
            ChatRoomInfo new_room;
            int fd = 0;
            char file_buf[BUF_SIZE];
            char room_name[NAME_SIZE];
            unsigned char salt[KEY_SIZE];
            unsigned char hash_value[HASH_SIZE];
            int32_t net_len, nlen;
            
            memset(file_buf, 0x00, BUF_SIZE);
            memset(room_name, 0x00, NAME_SIZE);

            read_all(clnt_sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read_all(clnt_sock, room_name, nlen);
            room_name[nlen] = '\0';

            printf("request user_id : %d\n", user_id);
            printf("room_name : %s\n", room_name);
            strncpy(new_room.room_name, room_name, NAME_SIZE);

            //랜덤 salt값 생성
            make_salt(salt, sizeof(salt));
            for(int i = 0; i < 16; i++)
                printf("%02x", salt[i]);
            printf("\n");

            nlen = 16;
            net_len = htonl(nlen);
            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, salt, nlen);

            read_all(clnt_sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read_all(clnt_sock, hash_value, nlen);

            printf("hash_value: ");
                for (int i = 0; i < 32; i++)
                    printf("%02x", hash_value[i]);
                printf("\n");

            new_room.user_id = user_id;
            
            memcpy(new_room.hash_value, hash_value, HASH_SIZE);

            pthread_mutex_lock(&roomId_file_mutx);
            memset(file_buf, 0x00, BUF_SIZE);
            fd = open("room_id_num.txt", O_RDWR);

            if(fd < 0)
                error_handling("room_id_num file open error");
            int len = read(fd, file_buf, BUF_SIZE-1);
            if(len < 0)
                error_handling("room_id_num file read error");
            file_buf[len] = '\0';
            int roomId_num = atoi(file_buf);
            roomId_num++;

            snprintf(file_buf, BUF_SIZE, "%d", roomId_num);
            nlen = (int32_t)strlen(file_buf);

            lseek(fd, 0, SEEK_SET);
            write(fd, file_buf, nlen);
            close(fd);
            new_room.room_id = roomId_num;
            pthread_mutex_unlock(&roomId_file_mutx);

            
            new_room.in_clnt_cnt = 0;

            printf("\nroom id : %d\n", new_room.room_id);
            printf("room_ user_id : %d\n", new_room.user_id);
             printf("room name : %s\n", new_room.room_name);
            printf("user cnt: %d\n\n", new_room.in_clnt_cnt); 
            memset(new_room.clnt_users, 0x00, BUF_SIZE);
            
            pthread_mutex_lock(&mKchat_room_mutx);
            rooms[room_cnt++] = new_room;
            printf("방수 : [%d]\n", room_cnt);
            pthread_mutex_unlock(&mKchat_room_mutx);
}

void print_user_list(int clnt_sock){
    int32_t net_len, nlen;
    char send_buf[BUF_SIZE];

    pthread_mutex_lock(&mutx);

    // 1) 접속자 수 전송
    nlen = snprintf(send_buf, BUF_SIZE, "%d", clnt_cnt);
    net_len = htonl(nlen);
    write(clnt_sock, &net_len, sizeof(net_len));
    write(clnt_sock, send_buf, nlen);

    // 2) 각 사용자 이름 전송
    for(int i=0; i<clnt_cnt; i++){
        nlen = strnlen(clients[i].user_name, NAME_SIZE);
        net_len = htonl(nlen);

        write(clnt_sock, &net_len, sizeof(net_len));
        write(clnt_sock, clients[i].user_name, nlen);
    }

    pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
