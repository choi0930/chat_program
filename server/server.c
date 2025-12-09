#include "common.h"
#include "server_cmd.h"
#include "crypto_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

void *handle_clnt(void * arg);
int get_client_index(int clnt_sock);
void send_room_msg(int room_id, int sender_uid, char *msg, int len);
//void send_msg(char * msg, int len);

int main(int argc, char *argv[]){
    int serv_sock, clnt_sock, fd, user_id_num;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    
    pthread_t t_id;
    if(argc!=2){
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    pthread_mutex_init(&file_mutx, NULL);
    pthread_mutex_init(&mKchat_room_mutx, NULL);
    pthread_mutex_init(&roomId_file_mutx, NULL);

    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port=htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock, 5)==-1)
        error_handling("listen() error");

    while(1){
        
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

        int *pclnt = malloc(sizeof(int));
        *pclnt = clnt_sock;
    
        pthread_create(&t_id, NULL, handle_clnt, (void*)pclnt);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sock);
    return 0;
}

void * handle_clnt(void * arg){
    ClientInfo new_client;
    int32_t net_len;
    int32_t nlen;
    int len = 0;
    int str_len=0, i, fd, user_id_num;
    char clnt_name[NAME_SIZE], file_buf[BUF_SIZE], buf[BUF_SIZE], msg[MSG_SIZE], status;

    int clnt_sock = *((int*)arg);
    free(arg);
        
    memset(clnt_name, 0x00, NAME_SIZE);
    memset(file_buf, 0x00, BUF_SIZE);
        //접속한 클라이언트 이름
        
        //read(clnt_sock, &net_len, sizeof(net_len));
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(clnt_sock, clnt_name, nlen);
        clnt_name[nlen] = '\0';

        printf("clnt_name:  %s len : %ld\n", clnt_name, strlen(clnt_name));
        strncpy(new_client.user_name, clnt_name, NAME_SIZE-1);
        new_client.user_name[NAME_SIZE-1] = '\0';
      

        //기존 유저인지 확인
        read(clnt_sock, &status, 1);
        
        printf("status : %c\n", status);

        if(status == '1'){
            
            //status = 1 -> 신규유저 user_id 발급
            pthread_mutex_lock(&file_mutx);
            fd = open("user_id_num.txt", O_RDWR);
            if(fd < 0)
                error_handling("user_id file open error");
        
            len = read(fd, file_buf, BUF_SIZE-1);
            if(len < 0)
                error_handling("user_id file read error");
        
            file_buf[len] = '\0';

            user_id_num = atoi(file_buf);
            user_id_num++;
            snprintf(file_buf, BUF_SIZE, "%d", user_id_num);
            nlen = (int32_t)strlen(file_buf);

            lseek(fd, 0, SEEK_SET);
            write(fd, file_buf, nlen);
            close(fd);

            pthread_mutex_unlock(&file_mutx);
            
            printf("new user_id_num : %d\n", user_id_num);
            printf("file buf nlen: %d\n", nlen);
            int32_t net_len = htonl(nlen);

            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, file_buf, nlen);

            
        }else if(status == '0'){
            //status = 0 -> 기존 유저
            read_all(clnt_sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read_all(clnt_sock, buf, nlen);
            buf[nlen] = '\0';
           
            user_id_num = atoi(buf);
            printf("user_id: %d\n", user_id_num);
        }
        
        new_client.sock_fd = clnt_sock;
        new_client.user_id = user_id_num;
        new_client.cur_room_id = -1;
        /*----디버깅용----------------------------------------*/
        //printf("\n now user_id: %d\n", user_id_num);
        printf("\nclient sock : %d\n", new_client.sock_fd);
        printf("client_user_id : %d\n", new_client.user_id);
        printf("client_name: %s\n", new_client.user_name);
        printf("cur_room_id: %d\n\n", new_client.cur_room_id);
        /*---------------------------------------------------*/

        //접속한 클라이언트 정보 저장
        pthread_mutex_lock(&mutx);
        clients[clnt_cnt++] = new_client;
        printf("현재 접속한 클라이언트 수 : [%d]\n", clnt_cnt);
        pthread_mutex_unlock(&mutx);
    
    int user_index = get_client_index(clnt_sock);

    while(1){

        memset(msg, 0x00, MSG_SIZE);
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read_all(clnt_sock, msg, nlen);
        msg[nlen] = '\0';
         
        if(clients[user_index].cur_room_id != -1){
            if(strcmp(msg, "/leave") == 0){ //체팅방 나가기
                pthread_mutex_lock(&mKchat_room_mutx); 
                //채팅방에서 유저 out
                for (int i = 0; i < room_cnt; i++) {
                    if(rooms[i].room_id == clients[user_index].cur_room_id){
                        for(int k = 0; k<rooms[i].in_clnt_cnt; k++){
                            if(rooms[i].clnt_users[k] == clients[user_index].user_id){
                                rooms[i].clnt_users[k] = rooms[i].clnt_users[rooms[i].in_clnt_cnt - 1];
                                rooms[i].in_clnt_cnt--;
                                break;
                            }
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&mKchat_room_mutx);
                pthread_mutex_lock(&mutx);
                //유저 정보에서 참여하고있었던 방id out
                clients[user_index].cur_room_id = -1;
                pthread_mutex_unlock(&mutx);
                
                //체팅 종료 시그널
                char leave_signal[] = "__LEAVE__";
                int32_t slen = strlen(leave_signal);
                int32_t net_slen = htonl(slen);
                write(clnt_sock, &net_slen, sizeof(net_len));
                write(clnt_sock, leave_signal, slen);
                
                continue; //명령어 모드로 전환
            }
            send_room_msg(clients[user_index].cur_room_id, clients[user_index].user_id, msg, nlen);
            continue; 
        }

        printf("클라이언트가 요청한 명령 : %s\n", msg);

        if(strcmp(msg, "exit") == 0){
            printf("[%s] 클라이언트 접속 종료\n", new_client.user_name);
            
            pthread_mutex_lock(&mutx);
            for (i = 0; i < clnt_cnt; i++) {
                if (clnt_sock == clients[i].sock_fd) {
                    clients[i] = clients[clnt_cnt - 1];  // 마지막 것을 그냥 덮어씀
                    clnt_cnt--;
                    break;
                }
            }
            /*
            for(i=0; i<clnt_cnt; i++){
                if(clnt_sock == clients[i].sock_fd){
                    while(i < clnt_cnt-1){
                        clients[i] = clients[i+1];
                        i++;
                    }
                    break;
                }
            
            }
            clnt_cnt--;
            */
            printf("현재 접속한 클라이언트 수 : [%d]\n", clnt_cnt);
            pthread_mutex_unlock(&mutx);
            close(clnt_sock);
            return NULL;

        }else if(strcmp(msg, "mkroom") == 0){
            cmd_mkroom(clnt_sock, clients[user_index].user_id);
        }else if(strcmp(msg, "user_list") == 0){
            print_user_list(clnt_sock);
        }else if(strcmp(msg, "room_list") == 0){
            print_room_list(clnt_sock);
        }else if(strcmp(msg, "rm_room") == 0){
            rm_room(clnt_sock, clients[user_index].user_id);
        }else if(strcmp(msg, "join_room") == 0){
            join_room(clnt_sock, clients[user_index].user_id);
        }
    }    
    
}

int get_client_index(int clnt_sock){
    for(int i = 0; i<clnt_cnt; i++){
        if(clients[i].sock_fd == clnt_sock){
            return i;
        }
    }
    return -1;
}

void send_room_msg(int room_id, int sender_uid, char *msg, int len) {
    int32_t net_len = htonl(len);

    pthread_mutex_lock(&mKchat_room_mutx);

    for (int r = 0; r < room_cnt; r++) {
        if (rooms[r].room_id == room_id) {

            for (int i = 0; i < rooms[r].in_clnt_cnt; i++) {
                int uid = rooms[r].clnt_users[i];

                if(uid == sender_uid) continue; // 자기 자신 제외
                
                // uid → sock_fd 찾기
                for (int c = 0; c < clnt_cnt; c++) {
                    if(clients[c].user_id == uid) {
                        //길이 전송
                        write(clients[c].sock_fd, &net_len, sizeof(net_len));
                        //메시지 전송
                        write(clients[c].sock_fd, msg, len);
                        break;
                    }
                }
            }
            break;
        }
    }

    pthread_mutex_unlock(&mKchat_room_mutx);
}

/*
void send_msg(char * msg, int len){
    int i;
    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}
    */
