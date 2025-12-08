#ifndef SERVER_CMD_H
#define SERVER_CMD_H

#include "common.h"

//server_cmd
void error_handling(char * msg);
int read_all(int sock, void *buf, int len);
void cmd_mkroom(int clnt_sock, int user_id);

#endif
