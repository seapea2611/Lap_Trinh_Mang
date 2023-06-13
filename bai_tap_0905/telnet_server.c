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

#define MAX_CLIENT 10
#define MAX_MSG_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char taikhoan[20];
    char matkhau[50];
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
    client_t clients[MAX_CLIENT];
    int client_count = 0;

    while (1)
    {
        // Khởi tạo mảng file descriptor
        fd_set readfds;
        FD_ZERO(&readfds);

        // Thêm server vào mảng file descriptor
        FD_SET(server, &readfds);

        // Thêm các client vào mảng file descriptor
        if (client_count == 0)
        {
            printf("\nWaiting for client on %s:9000\n",
                   inet_ntoa(server_addr.sin_addr));
        }
        else
        {
            for (int i = 0; i < client_count; i++)
            {
                FD_SET(clients[i].sockfd, &readfds);
            }
        }

        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select() failed");
            continue;
        }

        // Kiểm tra server có sẵn sàng nhận kết nối mới không
        if (FD_ISSET(server, &readfds))
        {
            // Chấp nhận kết nối mới
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
            if (client < 0)
            {
                perror("accept() failed");
                continue;
            }

            // Thêm client vào mảng client
            if (client_count < MAX_CLIENT)
            {
                clients[client_count].sockfd = client;
                clients[client_count].addr = client_addr;
                strcpy(clients[client_count].taikhoan, "");
                strcpy(clients[client_count].matkhau, "");
                client_count++;
                printf("Client from %s:%d connected\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                char *question = "Enter your \"account password\": ";
                if (send(client, question, strlen(question), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
            }
            else
            {
                printf("Maximum number of clients reached\n");
                printf("Client %d disconnected\n", client);
            }
        }

        // Kiểm tra các client có sẵn sàng nhận dữ liệu không
        for (int i = 0; i < client_count; i++)
        {
            if (FD_ISSET(clients[i].sockfd, &readfds))
            {
                // Nhận dữ liệu từ client
                char msg[MAX_MSG_LEN];
                memset(msg, 0, MAX_MSG_LEN);
                int msg_len = recv(clients[i].sockfd, msg, MAX_MSG_LEN, 0);
                if (msg_len < 0)
                {
                    perror("recv() failed");
                    continue;
                }
                else if (msg_len == 0)
                {
                    // Xóa client khỏi mảng client
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port));
                    for (int j = i; j < client_count - 1; j++)
                    {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;

                    // Xóa client khỏi mảng file descriptor
                    FD_CLR(clients[i].sockfd, &readfds);

                    continue;
                }
                else
                {
                    // Xử lý dữ liệu
                    msg[msg_len] = '\0';
                    if (strcmp(clients[i].taikhoan, "") == 0 && strcmp(clients[i].matkhau, "") == 0)
                    {
                        // Lấy taikhoan và matkhau của client
                        char taikhoan[20], matkhau[50];
                        FILE *file = fopen("data.txt", "r");
                        if (file == NULL)
                            {
                                  perror("Failed to open database.txt");
                            }
                        char line[256];
                        int found = 1;
                        while(fgets(line, sizeof(line), file) != NULL) {
                            line[strcspn(line, "\n")] = '\0';

                            if(strcmp(line, msg) == 0) {
                                found = 0;
                                break;
                            }
                        }
                        fclose(file);
                        if(!found) {
                            if (send(clients[i].sockfd, "Invalid taikhoan. Please try again!\n", 37, 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                        else {
                        int ret = sscanf(msg, "%s %s", taikhoan, matkhau);
                        if (ret == 2)
                        {
                            strcpy(clients[i].taikhoan, taikhoan);
                            strcpy(clients[i].matkhau, matkhau);
                            printf("Client from %s:%d registered as %s:%s\n", inet_ntoa(clients[i].addr.sin_addr), ntohs(clients[i].addr.sin_port), clients[i].taikhoan, clients[i].matkhau);
                            if (send(clients[i].sockfd, "Sucess!\n", 9, 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                        else
                        {
                            if (send(clients[i].sockfd, "Invalit taikhoan format. Please try again!\n", 44, 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            char *question = "Enter your \"client_taikhoan: client_matkhau\": ";
                            if (send(clients[i].sockfd, question, strlen(question), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            continue;
                        }
                        }
                    }
                    else
                    {
                        
                        char message[MAX_MSG_LEN + 1];
                        sprintf(message, "%s: %s", clients[i].taikhoan, msg);
                        for (int j = 0; j < client_count; j++)
                        {
                            if (j != i)
                            {
                                if (send(clients[j].sockfd, message, strlen(message), 0) < 0)
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

    // Đóng socket
    close(server);

    return 0;
}