#ifndef CLIENT_CMD_H
#define CLIENT_CMD_H

//client_cmd
int read_all(int sock, void *buf, int len);
void cmd_mkroom(int sock, int user_id);
void print_user_list(int sock);
void print_room_list(int sock);
void rm_room(int sock, int user_id);
void join_room(int sock, char *name);

#endif
