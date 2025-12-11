#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20
#define KEY_SIZE 16
#define HASH_SIZE 32
#define MSG_SIZE 1200

typedef struct {
    int user_id;              // 고유 ID
    char user_name[NAME_SIZE]; // 사용자 이름
    int sock_fd;              // 연결된 소켓 번호
    int cur_room_id;          // -1 -> 방에 없는 상태
} ClientInfo;

typedef struct {
    int room_id;               // 방  ID
    int user_id;              // 생성자 ID
    unsigned char room_name[NAME_SIZE]; //방이름
    unsigned char salt[KEY_SIZE]; //salt
    unsigned char hash_value[HASH_SIZE]; //비밀번호 hash값
    int clnt_users[BUF_SIZE]; // 접속자 정보
    int in_clnt_cnt; //접속자 수
} ChatRoomInfo;

extern int clnt_cnt;
extern int room_cnt;
extern int clnt_socks[MAX_CLNT];
extern ClientInfo clients[MAX_CLNT];
extern ChatRoomInfo rooms[BUF_SIZE];

extern pthread_mutex_t mutx;
extern pthread_mutex_t file_mutx;
extern pthread_mutex_t mKchat_room_mutx;
extern pthread_mutex_t roomId_file_mutx;

#endif