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

int print_roomId_roomName(int clnt_sock);

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

    //printf("request user_id : %d\n", user_id);
    //printf("room_name : %s\n", room_name);
    strncpy(new_room.room_name, room_name, NAME_SIZE);

    //랜덤 salt값 생성
    make_salt(salt, sizeof(salt));
    for(int i = 0; i < 16; i++)
        //printf("%02x", salt[i]);
        //printf("\n");

            nlen = 16;
            net_len = htonl(nlen);
            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, salt, nlen);

            read_all(clnt_sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read_all(clnt_sock, hash_value, nlen);

            //printf("hash_value: ");
                //for (int i = 0; i < 32; i++)
                    //printf("%02x", hash_value[i]);
                //printf("\n");

            new_room.user_id = user_id;

            memcpy(new_room.salt, salt, KEY_SIZE);
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
            /*
            printf("\nroom id : %d\n", new_room.room_id);
            printf("room_ user_id : %d\n", new_room.user_id);
             printf("room name : %s\n", new_room.room_name);
            printf("user cnt: %d\n\n", new_room.in_clnt_cnt); 
            memset(new_room.clnt_users, 0x00, BUF_SIZE);
          */  
            pthread_mutex_lock(&mKchat_room_mutx);
            rooms[room_cnt++] = new_room;
            //printf("방수 : [%d]\n", room_cnt);
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

void print_room_list(int clnt_sock){
    int32_t net_len, nlen;
    char flag = 0;
    char send_buf[BUF_SIZE];

    pthread_mutex_lock(&mKchat_room_mutx);
    if(room_cnt != 0){
        flag = 1;
        write(clnt_sock, &flag, 1);
    }else{//채팅방이 없는경우
        write(clnt_sock, &flag, 1);
    }
    
    if(flag == 1){
        // 1) 채팅방 개수 전송
        nlen = snprintf(send_buf, BUF_SIZE, "%d", room_cnt);
        net_len = htonl(nlen);
        write(clnt_sock, &net_len, sizeof(net_len));
        write(clnt_sock, send_buf, nlen);

        // 2) 각 채팅방 이름 전송
        for(int i=0; i<room_cnt; i++){
            nlen = strnlen(rooms[i].room_name, NAME_SIZE);
            net_len = htonl(nlen);

            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, rooms[i].room_name, nlen);
        }
    }

    pthread_mutex_unlock(&mKchat_room_mutx);
}

int print_roomId_roomName(int clnt_sock){
        int32_t nlen, net_len;
        char flag = 0;
        char send_buf[BUF_SIZE];

        memset(send_buf, 0x00, BUF_SIZE);

        pthread_mutex_lock(&mKchat_room_mutx);
        if(room_cnt != 0){
            flag = 1;
        }
        
        write(clnt_sock, &flag, 1);

        if(flag == 1){
            
            nlen = snprintf(send_buf, BUF_SIZE, "%d", room_cnt);
            net_len = htonl(nlen);
            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, send_buf, nlen);
            //각 방 정보 전송 room_id + room_name
            for(int k = 0; k < room_cnt; k++){
                memset(send_buf, 0x00, BUF_SIZE);

                // room_id전송
                snprintf(send_buf, BUF_SIZE, "%d", rooms[k].room_id);
                nlen = strlen(send_buf);
                net_len = htonl(nlen);
                write(clnt_sock, &net_len, sizeof(net_len));
                write(clnt_sock, send_buf, nlen);

                //room_name 전송
                nlen = strnlen((char*)rooms[k].room_name, NAME_SIZE);
                net_len = htonl(nlen);
                write(clnt_sock, &net_len, sizeof(net_len));
                write(clnt_sock, rooms[k].room_name, nlen);
            }
            
        }
        pthread_mutex_unlock(&mKchat_room_mutx);
        return flag == 1 ? 1 : 0;
}

void rm_room(int clnt_sock, int user_id){
    //int user_id = 0;
    char flag = 0;
    char room_name[1024];
    memset(room_name, 0x00, 1024);
/*
    for (int i = 0; i < clnt_cnt; i++) {
        if (clnt_sock == clients[i].sock_fd) {
            user_id = clients[i].user_id;
            printf("del request user_id : %d\n", clients[i].user_id);
            break;
        }
    }
*/
    pthread_mutex_lock(&mKchat_room_mutx);
    // 요청한 클라이언트가 만든 방 개수 개산
    int cnt = 0;
    for (int k = 0; k < room_cnt; k++) {
        if(user_id == rooms[k].user_id){
            cnt++;
        }
    }
    pthread_mutex_unlock(&mKchat_room_mutx);

    if(cnt != 0){
        flag = 1;
        write(clnt_sock, &flag, 1);
    }else{//채팅방이 없는경우
        write(clnt_sock, &flag, 1);
    }

    
    if(flag == 1){
        pthread_mutex_lock(&mKchat_room_mutx);
        int32_t nlen, net_len;

        char buf[32], recv_buf[BUF_SIZE];
        snprintf(buf, sizeof(buf), "%d", cnt);
        //방개수
        nlen = strlen(buf);
        net_len = htonl(nlen);
        write(clnt_sock, &net_len, sizeof(net_len));
        write(clnt_sock, buf, nlen);

        //각 방 정보 전송 room_id + room_name
        for(int k = 0; k < room_cnt; k++){
            if(rooms[k].user_id == user_id){

                // room_id전송
                snprintf(buf, sizeof(buf), "%d", rooms[k].room_id);
                nlen = strlen(buf);
                net_len = htonl(nlen);
                write(clnt_sock, &net_len, sizeof(net_len));
                write(clnt_sock, buf, nlen);

                //room_name 전송
                nlen = strnlen((char*)rooms[k].room_name, NAME_SIZE);
                net_len = htonl(nlen);
                write(clnt_sock, &net_len, sizeof(net_len));
                write(clnt_sock, rooms[k].room_name, nlen);
            }
        }
        pthread_mutex_unlock(&mKchat_room_mutx);

        memset(recv_buf, 0x00, BUF_SIZE);
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(clnt_sock, recv_buf, nlen);
        recv_buf[nlen] = '\0';
        int room_id = atoi(recv_buf);

        //printf("rm request room_id : %d", room_id);
        //채팅방 삭제
        pthread_mutex_lock(&mKchat_room_mutx); 
        for (int i = 0; i < room_cnt; i++) {
            if (room_id == rooms[i].room_id) {

                nlen = strnlen((char*)rooms[i].room_name, NAME_SIZE);
                net_len = htonl(nlen);
                write(clnt_sock, &net_len, sizeof(net_len));
                write(clnt_sock, rooms[i].room_name, nlen);

                rooms[i] = rooms[room_cnt - 1]; 
                room_cnt--;
                break;
            }
        }
        pthread_mutex_unlock(&mKchat_room_mutx);
    }
}

void join_room(int clnt_sock, int user_id){
    int check = print_roomId_roomName(clnt_sock);
    int room_id = 0;
    int32_t net_len, nlen;
    char recv_buf[BUF_SIZE], room_name[NAME_SIZE];
    char flag = 0;
    unsigned char salt[KEY_SIZE];
    unsigned char hash_value[HASH_SIZE];

    memset(recv_buf, 0x00, BUF_SIZE);
    if(check == 1){
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(clnt_sock, recv_buf, nlen);
        recv_buf[nlen] = '\0';
        room_id = atoi(recv_buf);
        //printf("입장요청 room_id : %d\n", room_id);

        //요청받은 채팅방의 salt값
        pthread_mutex_lock(&mKchat_room_mutx); 
        for (int i = 0; i < room_cnt; i++) {
            if (room_id == rooms[i].room_id) {
                memcpy(salt, rooms[i].salt, KEY_SIZE);
                break;
            }
        }
        pthread_mutex_unlock(&mKchat_room_mutx);
        //salt값 send client
        nlen = 16;
        net_len = htonl(nlen);
        write(clnt_sock, &net_len, sizeof(net_len));
        write(clnt_sock, salt, nlen);

        //hash값 recv
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(clnt_sock, hash_value, nlen);

        pthread_mutex_lock(&mKchat_room_mutx); 
        for (int i = 0; i < room_cnt; i++) {
            if (room_id == rooms[i].room_id) {
                if(memcmp(hash_value, rooms[i].hash_value, HASH_SIZE) == 0){
                    //printf("hash match!\n");
                    rooms[i].clnt_users[rooms[i].in_clnt_cnt] = user_id;
                    rooms[i].in_clnt_cnt++;
                    strncpy(room_name, rooms[i].room_name, NAME_SIZE);
                    flag = 1;
                }else{
                    //printf("hash mismatch!\n");
                }
                break;
            }
        }
        pthread_mutex_unlock(&mKchat_room_mutx);
        //flag 전송
        write(clnt_sock, &flag, 1);
        //접속한 방 id 정보 유저정보의 cur_room_id에 기록
        if(flag == 1){
            //방이름 전송
            nlen = strlen(room_name);
            net_len = htonl(nlen);
            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, room_name, nlen);
            
            pthread_mutex_lock(&mutx); 

            for(int k = 0; k<clnt_cnt; k++){
                if(user_id == clients[k].user_id){
                    clients[k].cur_room_id = room_id;
                    break;
                }
            }

            pthread_mutex_unlock(&mutx); 
        }
    }
        
}

void print_inRoom_user(int clnt_sock, int room_idx) {
    int32_t net_len, nlen;

    pthread_mutex_lock(&mKchat_room_mutx);
    int cnt = rooms[room_idx].in_clnt_cnt;
    int user_id_arr[cnt];
    for(int i = 0; i < cnt; i++)
        user_id_arr[i] = rooms[room_idx].clnt_users[i];
    pthread_mutex_unlock(&mKchat_room_mutx);

    // 1) 접속자 수 전송
    char send_buf[32];
    nlen = snprintf(send_buf, sizeof(send_buf), "%d", cnt);
    net_len = htonl(nlen);
    write(clnt_sock, &net_len, 4);
    write(clnt_sock, send_buf, nlen);

    // 2) 사용자 이름 전송
    pthread_mutex_lock(&mutx);

    for(int i = 0; i < cnt; i++){
        int uid = user_id_arr[i];

        // user_id → user_name 찾기
        for(int k = 0; k < clnt_cnt; k++){
            if(clients[k].user_id == uid){

                int name_len = strlen(clients[k].user_name);
                int32_t net_name_len = htonl(name_len);

                write(clnt_sock, &net_name_len, 4);
                write(clnt_sock, clients[k].user_name, name_len);

                break;
            }
        }
    }

    pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
