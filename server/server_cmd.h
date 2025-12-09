#ifndef SERVER_CMD_H
#define SERVER_CMD_H

#include "common.h"

//server_cmd
void error_handling(char * msg);
int read_all(int sock, void *buf, int len);
void cmd_mkroom(int clnt_sock, int user_id);
void print_user_list(int clnt_sock);
void print_room_list(int clnt_sock);
void rm_room(int clnt_sock, int user_id);
void join_room(int clnt_sock, int user_id);
void print_inRoom_user(int clnt_sock, int room_idx);

#endif
