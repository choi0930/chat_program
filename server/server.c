#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256


void *handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
ClientInfo clients[MAX_CLNT];

pthread_mutex_t mutx;

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
    serv_sock=socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));

    pthread_mutex_init(&mutx, NULL);
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

    
        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }

    close(serv_sock);
    return 0;
}

void * handle_clnt(void * arg){
    int clnt_sock = *((int*)arg);
    int str_len=0, i, fd, user_id_num;
    char msg[BUF_SIZE];
    char clnt_name[BUF_SIZE], file_buf[BUF_SIZE], buf[BUF_SIZE], status[1];
    pthread_mutex_lock(&mutx);
        int len = 0;
        ClientInfo new_client;
        //clnt_socks[clnt_cnt++]=clnt_sock;
        memset(clnt_name, 0x00, BUF_SIZE);
        memset(file_buf, 0x00, BUF_SIZE);
        //접속한 클라이언트 이름
        len = read(clnt_sock, clnt_name, NAME_SIZE-1);
        clnt_name[len] = '\0';
        printf("clnt_name:  %s len : %ld\n", clnt_name, strlen(clnt_name));
        strncpy(new_client.user_name, clnt_name, strlen(clnt_name));
        sleep(1);

        //기존 유저인지 확인
        read(clnt_sock, status, 1);
        status[1] = '\0';
        printf("status : %s\n", status);

        if(strcmp(status, "1") == 0){
            
            //status = 1 -> 신규유저 user_id 발급
        
            fd = open("user_id_num.txt", O_RDWR);
            if(fd < 0)
                error_handling("user_id file open error");
        
            len = read(fd, file_buf, BUF_SIZE-1);
            if(len < 0)
                error_handling("user_id file read error");
        
            file_buf[len] = '\0';

            user_id_num = atoi(file_buf);
            user_id_num++;

            printf("new user_id_num : %d\n", user_id_num);
            snprintf(file_buf, BUF_SIZE, "%d", user_id_num);
            write(clnt_sock, file_buf, BUF_SIZE);

            lseek(fd, 0, SEEK_SET);
            write(fd, file_buf, strlen(file_buf));
            close(fd);
        }else if(strcmp(status, "0") == 0){
            //status = 0 -> 기존 유저

            len = read(clnt_sock, buf, BUF_SIZE);
            buf[len] = '\0';
           
            user_id_num = atoi(buf);
            printf("user_id: %d\n", user_id_num);
        }

        printf("\n now user_id: %d\n", user_id_num);
        
        
        //접속한 클라이언트 정보 저장
        
        new_client.sock_fd = clnt_sock;
        new_client.user_id = user_id_num;
        
        clients[clnt_cnt++] = new_client;
        printf("\nclient sock : %d\n", new_client.sock_fd);
        printf("client_user_id : %d\n", new_client.user_id);
        printf("client_name: %s\n\n", new_client.user_name);
        printf("현재 접속한 클라이언트 수 : [%d]", clnt_cnt);
        pthread_mutex_unlock(&mutx);
        
    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0)
        send_msg(msg, str_len);
    
        pthread_mutex_lock(&mutx);
        for(i=0; i<clnt_cnt; i++){
            if(clnt_sock==clnt_socks[i]){
                while(i++<clnt_cnt-1)
                    clnt_socks[i]=clnt_socks[i+1];
                break;
            }
            
        }
        clnt_cnt--;
        pthread_mutex_unlock(&mutx);
        close(clnt_sock);
        return NULL;
}

void send_msg(char * msg, int len){
    int i;
    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}