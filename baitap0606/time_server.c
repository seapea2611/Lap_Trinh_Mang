#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_CLIENTS 10

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;

void format_time(char *, const char *);
void *client_proc(void *);

int main()
{
    // Khởi tạo socket TCP
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9000);

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for new client on %s:%d\n",
           inet_ntoa(server_addr.sin_addr),
           ntohs(server_addr.sin_port));

    while (1)
    {
        // Chấp nhận kết nối
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        printf("New client connected");

        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client;
        clients[client_count].addr = client_addr;
        client_count++;
        pthread_mutex_unlock(&clients_mutex);

        // Tạo thread để xử lý client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_proc, (void *)&clients[client_count - 1]) != 0)
        {
            perror("pthread_create() failed");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
    }

    // Đóng socket
    close(server);

    return 0;
}

void format_time(char buf[], const char *format)
{
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(buf, BUF_SIZE, "%d/%m/%Y\n", timeinfo);
    }
    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(buf, BUF_SIZE, "%d/%m/%y\n", timeinfo);
    }
    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(buf, BUF_SIZE, "%m/%d/%Y\n", timeinfo);
    }
    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(buf, BUF_SIZE, "%m/%d/%y\n", timeinfo);
    }
    else
    {
        strcpy(buf, "Invalid format\n");
    }
}

void *client_proc(void *param)
{
    client_t client = *(client_t *)param;

    // Xử lý client
    while (1)
    {
        // Gửi yêu cầu đến client
        char *msg = "Enter command: ";
        if (send(client.sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            break;
        }

        // Nhận yêu cầu từ client
        char buf[BUF_SIZE];
        memset(buf, 0, BUF_SIZE);
        int len = recv(client.sockfd, buf, BUF_SIZE, 0);
        if (len < 0)
        {
            perror("recv() failed");
            break;
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client.addr.sin_addr),
                   ntohs(client.addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client.sockfd == clients[i].sockfd)
                {
                    pthread_mutex_lock(&clients_mutex);
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    if (client_count == 0)
                    {
                        printf("Waiting for clients on %s:%d...\n",
                               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    close(client.sockfd);
                    break;
                }
            }
            break;
        }
        else
        {
            // Xoá ký tự xuống dòng
            buf[strcspn(buf, "\n")] = 0;

            // Thoát nếu client gửi exit hoặc quit
            if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0)
            {
                char *msg = "Goodbye\n";
                if (send(client.sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    break;
                }

                printf("Client from %s:%d disconnected\n",
                       inet_ntoa(client.addr.sin_addr),
                       ntohs(client.addr.sin_port));
                for (int i = 0; i < client_count; i++)
                {
                    if (client.sockfd == clients[i].sockfd)
                    {
                        pthread_mutex_lock(&clients_mutex);
                        clients[i] = clients[client_count - 1];
                        client_count--;
                        if (client_count == 0)
                        {
                            printf("Waiting for clients on %s:%d...\n",
                                   inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
                        }
                        pthread_mutex_unlock(&clients_mutex);
                        close(client.sockfd);
                        break;
                    }
                }
                break;
            }

            // Xử lý yêu cầu
            char cmd[BUF_SIZE];
            char format[BUF_SIZE];
            char temp[BUF_SIZE];
            int ret = sscanf(buf, "%s %s %s", cmd, format, temp);
            if (ret == 2)
            {
                if (strcmp(cmd, "GET_TIME") == 0)
                {
                    memset(buf, 0, BUF_SIZE);
                    format_time(buf, format);
                    if (send(client.sockfd, buf, strlen(buf), 0) < 0)
                    {
                        perror("send() failed");
                        break;
                    }
                }
                else
                {
                    char *msg = "Invalid command\n";
                    if (send(client.sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        break;
                    }
                    continue;
                }
            }
            else
            {
                char *msg = "Invalid command\n";
                if (send(client.sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    break;
                }
                continue;
            }
        }
    }

    return NULL;
}