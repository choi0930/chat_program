#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "common.h"
#include "client_cmd.h"
#include "crypto_util.h"

void * send_msg(void * arg);
void * recv_msg(void * msg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[]){
    int sock, user_id, fd;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void * thread_return;
    char buf[BUF_SIZE], file_buf[BUF_SIZE], status[1];
    if(argc!=4){
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);
    sock=socket(PF_INET, SOCK_STREAM, 0);

    memset(aes_key, 0x00, KEY_SIZE);
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

    int len = read(fd, file_buf, BUF_SIZE-1);
    
    if(len>0){ // user_id가 이미 존재 기존유저인경우
        printf("if 상황 1 기존 유저\n");

        //status[0] = ;
        write(sock, "0", 1);
        
        file_buf[len] = '\0';
        nlen = (int32_t)strlen(file_buf);
        net_len = htonl(nlen);
        write(sock, &net_len, sizeof(net_len));
        write(sock, file_buf, nlen);

        user_id = atoi(file_buf);
        printf("기존유저 id : %d\n", user_id);
        
    }else if(len == 0){ // 신규 유저
        printf("if 상황 2 신규유저\n");

        //status[0] = "1";
        write(sock, "1", 1);
        
        //read(sock, &net_len, sizeof(net_len));
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);
        printf("file buf nlen: %d\n", nlen);
        read_all(sock, buf, nlen);
        buf[nlen] = '\0';
        
        user_id = atoi(buf);
        printf("new user_id: %d\n", user_id);

        write(fd, buf, strlen(buf));
    }else{ //오류
        error_handling("user_id file read error");  
    }
    close(fd);
    
    
    while(1){
        int flag=0;
        memset(buf, 0x00, BUF_SIZE);
        
        //print_room_list(sock);

        printf("명령어 입력 (mkroom, join_room, user_list, room_list, rm_room, exit): ");
        fgets(buf, BUF_SIZE, stdin);
        buf[strcspn(buf, "\n")] = 0;
        //명령어 길이, 명령어 전송
        nlen = (int32_t)strlen(buf);
        net_len = htonl(nlen);

        write(sock, &net_len, sizeof(net_len));
        write(sock, buf, nlen);

        if(strcmp(buf, "exit") == 0){
            printf("종료\n");
            break;
        }else if(strcmp(buf, "mkroom") == 0){
            cmd_mkroom(sock);
        }else if(strcmp(buf, "user_list") == 0){
            print_user_list(sock);
        }else if(strcmp(buf, "room_list") == 0){
            print_room_list(sock);
        }else if(strcmp(buf, "rm_room") == 0){
            rm_room(sock);
        }else if(strcmp(buf, "join_room") == 0){
            flag = join_room(sock, name);
        }

        if(flag == 1){
        pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
        pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
        pthread_join(snd_thread, &thread_return);
        pthread_join(rcv_thread, &thread_return);
        }
    }
    close(sock);
    return 0;
}

void * send_msg(void *arg){
    int32_t nlen, net_len;
    int sock= *((int*)arg);
    char name_msg[NAME_SIZE+MSG_SIZE+2]; //크기 1024
    char msg[MSG_SIZE];

    unsigned char ciphertext[MSG_SIZE + 32];
    unsigned char iv[16];
    int cipher_len;

    while(1){

        fgets(msg, BUF_SIZE, stdin);
        msg[strcspn(msg, "\n")] = 0;

        if(strcmp(msg,"/leave") == 0){ //채팅방 나가기
            //채팅방 나가기 동작
            nlen = (int32_t)strlen(msg);
            net_len = htonl(nlen);
            write(sock, &net_len, sizeof(net_len));
            write(sock, msg, nlen);

            return NULL;
        }else if(strcmp(msg, "/list") == 0){ //채팅방 접속인원 보기
            nlen = (int32_t)strlen(msg);
            net_len = htonl(nlen);
            write(sock, &net_len, sizeof(net_len));
            write(sock, msg, nlen);
            continue;
        }
        sprintf(name_msg,"%s %s", name, msg);
        cipher_len = aes_encrypt((unsigned char*)name_msg, strlen(name_msg), aes_key, iv, ciphertext);

        if(cipher_len < 0){
            printf("암호화 실패\n");
            continue;
        }
        int total_len = 16+cipher_len;
        net_len = htonl(total_len);
        write(sock, &net_len, sizeof(net_len)); //전체 길이
        write(sock, iv, 16);                    // IV
        write(sock, ciphertext, cipher_len);    // 암호문
        /*
        
        nlen = (int32_t)strlen(name_msg);
        net_len = htonl(nlen);
        write(sock, &net_len, sizeof(net_len));
        write(sock, name_msg, nlen);
        */
    }
    return NULL;
}

void * recv_msg(void * arg){
    int sock = *((int*)arg);
    int32_t net_len, nlen;
    char recv_msg[NAME_SIZE+MSG_SIZE];
    
    unsigned char ciphertext[MSG_SIZE + 32];
    unsigned char plaintext[MSG_SIZE + NAME_SIZE];
    unsigned char iv[16];
    int cipher_len;

    while (1) {
        // 1) 길이 먼저 받기
        if (read_all(sock, &net_len, sizeof(net_len)) <= 0) {
            printf("서버와 연결이 종료되었습니다.\n");
            break;
        }

        nlen = ntohl(net_len);
        if(nlen <16){
            char msg[32];
            read_all(sock, msg, nlen);
            msg[nlen] = '\0';

            if (strcmp(msg, "__LEAVE__") == 0) { // 채팅방 종료
            printf("채팅방에서 나갔습니다.\n");
            break;  // recv 스레드 종료 → main으로 복귀
            }else if(strcmp(msg, "__LIST__") == 0){ //채팅방에 접속한 인원 목록 출력
                int32_t nlen, net_len;
    int user_cnt;
    char user_name[NAME_SIZE], recv_buf[256];

    //유저 수 받기
    read_all(sock, &net_len, sizeof(net_len));
    nlen = ntohl(net_len);
    read_all(sock, recv_buf, nlen);
    recv_buf[nlen] = '\0';

    user_cnt = atoi(recv_buf);
    printf("\n--- 접속자: %d명 ---\n", user_cnt);

    //각 사용자 이름 출력
    for(int i=0; i<user_cnt; i++){
        read_all(sock, &net_len, sizeof(net_len));
        nlen = ntohl(net_len);

        read_all(sock, user_name, nlen);
        user_name[nlen] = '\0';

        printf(" - %s\n", user_name);
    }

    printf("-----------------------\n");
            }
            continue;
        }

        // 2) IV 받기
        if (read_all(sock, iv, 16) <= 0){
            break;
        }
        
        // 3) 나머지 암호문 받기
        cipher_len = nlen - 16;
        read_all(sock, ciphertext, cipher_len);
        //복호화
        int plain_len = aes_decrypt(ciphertext, cipher_len, aes_key, iv, plaintext);
        plaintext[plain_len] = '\0';

        printf("%s\n", plaintext);

        // 4) 일반 메시지 출력
        //printf("%s\n", recv_msg);
    }
    return NULL;
}

void error_handling(char * msg){
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}