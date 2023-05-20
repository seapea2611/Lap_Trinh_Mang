#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>

#define MAX_CLIENT 64
#define MAX_MSG_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char id[20];
    char name[50];
} client_t;

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
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(9000);

    // Gán địa chỉ cho socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server, MAX_CLIENT) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    // Khởi tạo mảng client
    
    //char buf[256];

    client_t clients[MAX_CLIENT];
    int client_count = 0;
    int users[64];      // Mang socket client da dang nhap
    char *user_ids[64];

    while (1)
    {
        struct pollfd fds[MAX_CLIENT];
         int nfds = 1;
        fds[0].fd = server;
        fds[0].events = POLLIN;

        if (client_count == 0)
        {
            printf("Waiting for clients on %s:9000\n",
                   inet_ntoa(server_addr.sin_addr));
        }
        else
        {
            for (int i = 0; i < client_count; i++)
            {
                fds[nfds].fd = clients[i].sockfd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int ret = poll(fds, nfds, -1);
        if(ret < 0) {
            perror("poll() failed");
            break;
        }

         //     printf("ret = %d\n", ret);

        // Thêm các client vào mảng file descriptor
        if (fds[0].revents & POLLIN)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
            if (nfds < MAX_CLIENT)
            {
                // printf("New client connected: %d\n", client);
                // fds[nfds].fd = client;
                // fds[nfds].events = POLLIN;
                // nfds++;
                clients[client_count].sockfd = client;
                clients[client_count].addr = client_addr;
                strcpy(clients[client_count].id, "");
                strcpy(clients[client_count].name, "");
                client_count++;
                
                printf("Client connected from %s:%d\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                // Gửi thông báo yêu cầu client nhập "client_id: client_name"
                char *msg = "Enter your \"client_id: client_name\": ";
                if (send(client, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
            else
            {
                printf("Too many connections\n");
                close(client);
            }
        }

        for(int i = 0; i < client_count; i ++) {
            if (fds[i + 1].revents & (POLLIN | POLLERR)) {// Sự kiện client
                char buf[MAX_MSG_LEN];
                 memset(buf, 0, MAX_MSG_LEN);
                int rec = recv(clients[i].sockfd, buf, sizeof(buf), 0);
                if (rec <= 0)
                {
                     printf("Client from %s:%d disconnected\n", inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));

                    // Gửi thông báo cho các client khác biết client này đã ngắt kết nối
                    // Đóng socket client
                    close(clients[i].sockfd);

                    // Xóa client khỏi mảng clin_clienent
                    clients[i] = clients[client_count - 1];
                    client_count--;

                    // Xóa socket client khỏi mảng file descriptor
                    fds[i + 1] = fds[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                }
                else
                {
                    buf[strcspn(buf, "\n")] = 0;
                    if (strcmp(clients[i].id, "") == 0 && strcmp(clients[i].name, "") == 0)
                    {
                        // Lấy id và name của client
                        char id[20];
                        char name[50];
                        if (sscanf(buf, "%[^:]: %s", id, name) == 2)
                        {
                            // Kiểm tra xem id có trùng với các client khác không
                            int is_valid = 1;
                            for (int j = 0; j < client_count; j++)
                            {
                                if (strcmp(clients[j].id, id) == 0)
                                {
                                    is_valid = 0;
                                    break;
                                }
                            }
                            if (is_valid)
                            {
                                strcpy(clients[i].id, id);
                                strcpy(clients[i].name, name);
                                printf("Client %s:%d registered as %s:%s\n",
                                       inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port),
                                       clients[i].id, clients[i].name);
                                char *msg = "Registered successfully!\n";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else
                            {
                                char *msg = "Client ID already exists!\n";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                                msg = "Enter again your \"client_id: client_name\": ";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            char *msg = "Invalid format!\n";
                            if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            msg = "Enter again your \"client_id: client_name\": ";
                            if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                    }
                    else {
                        char message[MAX_MSG_LEN];
                        char receiver[20];
                        int re = sscanf(buf, "%[^@]@%s", receiver, message);
                        char msg_to_send[MAX_MSG_LEN];
                        sprintf(msg_to_send, "%s: %s\n",clients[i].id, buf);

                         if (re == 2)
                        {
                            // Gửi tin nhắn đến một người khác
                            char mess[1030];
                            int ret = sscanf(msg_to_send, "%*[^@]@%s", mess);
                            char message_send[MAX_MSG_LEN + 30];
                            sprintf(message_send, "Message from %s: %s\n",clients[i].id, mess);
                            int is_valid = 0;
                            for (int j = 0; j < client_count; j++)
                            {
                                if (strcmp(clients[j].id, receiver) == 0)
                                {
                                    is_valid = 1;
                                    if (send(clients[j].sockfd, message_send, strlen(message_send), 0) < 0)
                                    {
                                        perror("send() failed");
                                        continue;
                                    }
                                    break;
                                }
                            }
                            if (!is_valid)
                            {
                                char *msg = "Invalid receiver!\n";
                                if (send(clients[i].sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            // Gửi tin nhắn đến mọi người
                            for (int j = 0; j < client_count; j++)
                            {
                                if (j != i && strcmp(clients[j].id, "") != 0)
                                {
                                    if (send(clients[j].sockfd, msg_to_send, strlen(msg_to_send), 0) < 0)
                                    {
                                        perror("send() failed");
                                        continue;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }
    close(server);

    return 0;
}