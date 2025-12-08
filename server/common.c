#include "common.h"

int clnt_cnt = 0;
int room_cnt = 0;
int clnt_socks[MAX_CLNT];
ClientInfo clients[MAX_CLNT];
ChatRoomInfo rooms[BUF_SIZE];

pthread_mutex_t mutx;
pthread_mutex_t file_mutx;
pthread_mutex_t mKchat_room_mutx;
pthread_mutex_t roomId_file_mutx;