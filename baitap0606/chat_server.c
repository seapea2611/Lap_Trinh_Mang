#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>

#define MAX_CLIENTS 64
#define MAX_MSG_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char id[20];
    char name[50];
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;

void *client_thread(void *);

int main() {
    int server = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(9000); 

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
        
    while (1)
    {
         struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server, (struct sockaddr *)&client_addr, &client_len);
        printf("New client connected\n");

        // Thêm client vào danh sách
        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client_sock;
        clients[client_count].addr = client_addr;
        strcpy(clients[client_count].name, "");
        strcpy(clients[client_count].id, "");
        client_count++;
        pthread_mutex_unlock(&clients_mutex);

        // Tạo thread để xử lý client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_thread, (void *)&clients[client_count - 1]) != 0)
        {
            perror("pthread_create() failed");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
    }
    
    close(server);    

    return 0;
}

void *client_thread(void *param)
{
    client_t *client = (client_t *)param;
    char buffer[MAX_CLIENTS];

    // Nhận id và name từ client
    while (1)
    {
        // Yêu cầu client nhập "client_id: client_name"
        char *msg = "Enter your \"client_id: client_name\": ";
        if (send(client->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
        }

        // Nhận "client_id: client_name" từ client
        memset(buffer, 0, MAX_CLIENTS);
        int len = recv(client->sockfd, buffer, MAX_CLIENTS, 0);
        if (len < 0)
        {
            perror("recv() failed");
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd == clients[i].sockfd)
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
                    break;
                }
            }
            return NULL;
        }
        else
        {
            // Xóa ký tự xuống dòng
            buffer[strcspn(buffer, "\n")] = 0;

            // Tách id và name
            char id[MAX_CLIENTS];
            char name[MAX_CLIENTS];
            char temp[MAX_CLIENTS];

            int ret = sscanf(buffer, "%s %s %s", id, name, temp);
            if (ret == 2)
            {
                int len = strlen(id);
                if (id[len - 1] != ':')
                {
                    char *msg = "Invalid format! Please try again!\n";
                    if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                    }
                    continue;
                }
                else
                {
                    id[len - 1] = 0;
                    strcpy(client->id, id);
                    strcpy(client->name, name);

                    break;
                }
            }
            else
            {
                char *msg = "Invalid format! Please try again!\n";
                if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                }
                continue;
            }
        }
    }

    // Nhận tin nhắn từ client và gửi lại cho tất cả client khác
    while (1)
    {
        memset(buffer, 0, MAX_CLIENTS);
        int len = recv(client->sockfd, buffer, MAX_CLIENTS, 0);
        if (len < 0)
        {
            perror("recv() failed");
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd == clients[i].sockfd)
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
                    break;
                }
            }
            break;
        }
        else
        {
            // Xóa ký tự xuống dòng
            buffer[strcspn(buffer, "\n")] = 0;

            // Định dạng tin nhắn cần gửi
            char msg_to_send[MAX_CLIENTS + 10];
            sprintf(msg_to_send, "%s: %s\n\n", client->id, buffer);

            // Gửi tin nhắn cho tất cả client khác
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd != clients[i].sockfd)
                {
                    if (strcmp(clients[i].id, "") == 0)
                    {
                        continue;
                    }
                    if (send(clients[i].sockfd, msg_to_send, strlen(msg_to_send), 0) < 0)
                    {
                        perror("send() failed");
                    }
                }
            }
        }
    }

    return NULL;
}