#ifndef CLIENT_CMD_H
#define CLIENT_CMD_H

//client_cmd
int read_all(int sock, void *buf, int len);
void cmd_mkroom(int sock); //채팅방 생성
void print_user_list(int sock); //서버에 접속한 인원 목록 출력
void print_room_list(int sock); //서버에 존재하는 채팅방 출력
void rm_room(int sock); //채팅방 삭제
int join_room(int sock, char *name); //채팅방 입장

#endif
