#include "common.h"

#define BUF_SIZE 100
#define MAX_CLNT 256
#define NAME_SIZE 20

typedef struct {
    int user_id;              // 고유 ID
    char user_name[NAME_SIZE]; // 사용자 이름, null-terminated
    int sock_fd;              // 연결된 소켓 번호
} ClientInfo;

void *handle_clnt(void * arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);
int read_all(int sock, void *buf, int len);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
ClientInfo clients[MAX_CLNT];

pthread_mutex_t mutx;
pthread_mutex_t file_mutx;


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
    ClientInfo new_client;

    int clnt_sock = *((int*)arg);
    int str_len=0, i, fd, user_id_num;
    char msg[BUF_SIZE];
    char clnt_name[BUF_SIZE], file_buf[BUF_SIZE], buf[BUF_SIZE], status[2];
    
        int len = 0;
        
        //clnt_socks[clnt_cnt++]=clnt_sock;
        memset(clnt_name, 0x00, BUF_SIZE);
        memset(file_buf, 0x00, BUF_SIZE);
        //접속한 클라이언트 이름

        int32_t net_len;
        int32_t nlen;

        //read(clnt_sock, &net_len, sizeof(net_len));
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read(clnt_sock, clnt_name, nlen);
        clnt_name[nlen] = '\0';

        printf("clnt_name:  %s len : %ld\n", clnt_name, strlen(clnt_name));
        strncpy(new_client.user_name, clnt_name, strlen(clnt_name));
      

        //기존 유저인지 확인
        read(clnt_sock, status, 1);
        status[1] = '\0';
        printf("status : %s\n", status);

        if(strcmp(status, "1") == 0){
            
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

            
        }else if(strcmp(status, "0") == 0){
            //status = 0 -> 기존 유저
            read_all(clnt_sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read(clnt_sock, buf, nlen);
            buf[nlen] = '\0';
           
            user_id_num = atoi(buf);
            printf("user_id: %d\n", user_id_num);
        }

        printf("\n now user_id: %d\n", user_id_num);
        
        
        
        new_client.sock_fd = clnt_sock;
        new_client.user_id = user_id_num;
        
        printf("\nclient sock : %d\n", new_client.sock_fd);
        printf("client_user_id : %d\n", new_client.user_id);
        printf("client_name: %s\n\n", new_client.user_name);
        
        pthread_mutex_lock(&mutx);
        //접속한 클라이언트 정보 저장
        clients[clnt_cnt++] = new_client;
        printf("현재 접속한 클라이언트 수 : [%d]\n", clnt_cnt);
        pthread_mutex_unlock(&mutx);
    while(1){
        memset(buf, 0x00, BUF_SIZE);
        read_all(clnt_sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        read(clnt_sock, buf, nlen);
        buf[nlen] = '\0';

        printf("클라이언트가 요청한 명령 : %s\n", buf);

        if(strcmp(buf, "exit") == 0){
            printf("[%s] 클라이언트 접속 종료\n", new_client.user_name);
            
            pthread_mutex_lock(&mutx);
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
            printf("현재 접속한 클라이언트 수 : [%d]\n", clnt_cnt);
            pthread_mutex_unlock(&mutx);
            close(clnt_sock);
            return NULL;

        }else if(strcmp(buf, "mkroom") == 0){
            char room_name[NAME_SIZE];
            unsigned char salt[16];
            memset(room_name, 0x00, NAME_SIZE);
            memset(salt, 0x00, 16);
            
            read_all(clnt_sock, &net_len, sizeof(net_len));
            nlen = ntohl(net_len);
            read(clnt_sock, room_name, nlen);
            room_name[nlen] = '\0';
            printf("request user_id : %d\n", new_client.user_id);
            printf("room_name : %s\n", room_name);
            
            //랜덤 salt값 생성
            make_salt(salt, sizeof(salt));
            for(int i = 0; i < 16; i++)
                printf("%02x", salt[i]);
            printf("\n");

            //int32_t nlen = (int32_t)strlen(salt);
            int32_t nlen = 16;
            int32_t net_len = htonl(nlen);
            write(clnt_sock, &net_len, sizeof(net_len));
            write(clnt_sock, salt, nlen);

        }
    }    
    /*
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
        */
}

void send_msg(char * msg, int len){
    int i;
    pthread_mutex_lock(&mutx);
    for(i=0; i<clnt_cnt; i++)
        write(clnt_socks[i], msg, len);
    pthread_mutex_unlock(&mutx);
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