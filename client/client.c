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
#define NAME_SIZE 20

void * send_msg(void * arg);
void * recv_msg(void * msg);
void error_handling(char * msg);
int read_all(int sock, void *buf, int len);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[]){
    int sock, user_id, fd;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void * thread_return;
    char buf[BUF_SIZE], file_buf[BUF_SIZE], *status;
    if(argc!=4){
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_addr.sin_port=htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");
    
    //클라이언트 이름
    int32_t nlen = (int32_t)strlen(argv[3]);
    int32_t net_len = htonl(nlen);
    write(sock, &net_len, sizeof(net_len));
    write(sock, argv[3], nlen);

    memset(buf, 0x00, BUF_SIZE);
    memset(file_buf, 0x00, BUF_SIZE);
    
    fd = open("user_id_num.txt", O_RDWR | O_CREAT, 0666);
    if(fd<0)
        error_handling("user_id file open error");

    int len = read(fd, file_buf, BUF_SIZE);
    
    if(len>0){ // user_id가 이미 존재 기존유저인경우
        printf("if 상황 1 기존 유저\n");

        status = "0";
        write(sock, status, 1);
        
        file_buf[len] = '\0';
        int32_t nlen = (int32_t)strlen(file_buf);
        int32_t net_len = htonl(nlen);
        write(sock, &net_len, sizeof(net_len));
        write(sock, file_buf, nlen);

        user_id = atoi(file_buf);
        printf("기존유저 id : %d\n", user_id);
        
    }else if(len == 0){ // 신규 유저
        printf("if 상황 2 신규유저\n");

        status = "1";
        write(sock, status, 1);
        
        //read(sock, &net_len, sizeof(net_len));
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        printf("file buf nlen: %d\n", nlen);
        read(sock, buf, nlen);
        buf[nlen] = '\0';
        
        user_id = atoi(buf);
        printf("new user_id: %d\n", user_id);

        write(fd, buf, strlen(buf));
    }else{ //오류
        printf("if 상황 3 오류\n");
        status = "-1";
        write(sock, status, 1);
        
        error_handling("user_id file read error");  
    }
    close(fd);
    

    while(1){
        memset(buf, 0x00, BUF_SIZE);
        
        printf("명령어 입력 (mkroom, user_list ,exit): ");
        fgets(buf, BUF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;
        //명령어 길이, 명령어 전송
        int32_t nlen = (int32_t)strlen(buf);
        int32_t net_len = htonl(nlen);

        write(sock, &net_len, sizeof(net_len));
        write(sock, buf, nlen);
        if(strcmp(buf, "exit") == 0){
            printf("종료\n");
            break;
        }else if(strcmp(buf, "mkroom") == 0){
            char room_name[NAME_SIZE];
            unsigned char salt[16];
            memset(room_name, 0x00, NAME_SIZE);
            memset(salt, 0x00, 16);

            printf("방이름 입력 : ");
            fgets(room_name, NAME_SIZE, stdin);
            room_name[strcspn(room_name, "\n")] = 0;

            int32_t nlen = (int32_t)strlen(room_name);
            int32_t net_len = htonl(nlen);

            write(sock, &net_len, sizeof(net_len));
            write(sock, room_name, nlen);
            printf("request user_id : %d\n", user_id);

           //salt값 받기
            read_all(sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read(sock, salt, nlen);
            salt[nlen] = '\0';
            for(int i = 0; i < 16; i++)
                printf("%02x", salt[i]);
            printf("\n");
        }
    }
   
    /*
    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    */
    close(sock);
    return 0;
}

void * send_msg(void *arg){
    int sock= *((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    while(1){
        fgets(msg, BUF_SIZE, stdin);
        if(!strcmp(msg,"q\n") || !strcmp(msg,"Q\n")){
            close(sock);
            exit(0);
        }
        sprintf(name_msg,"%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

void * recv_msg(void * arg){
    int sock=*((int *)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    int str_len;
    while(1){
        str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
        if(str_len==-1){
            return (void*)-1;
        }
        name_msg[str_len]=0;
        fputs(name_msg, stdout);
    }
    return NULL;
}
int read_all(int sock, void *buf, int len) {//길이 4바이트를 받는 함수
    int received = 0;
    while(received < len) {
        int n = read(sock, buf + received, len - received);
        if(n <= 0) return n;  // error or disconnected
        received += n;
    }
    return received;
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}