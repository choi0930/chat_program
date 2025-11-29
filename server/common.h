#define NAME_SIZE 20

typedef struct {
    int user_id;              // 고유 ID
    char user_name[NAME_SIZE]; // 사용자 이름, null-terminated
    int sock_fd;              // 연결된 소켓 번호
} ClientInfo;