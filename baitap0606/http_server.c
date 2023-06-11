#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define NUMBER_THREADS 8
#define NUMBER_CLIENTS 10

void *client_proc(void *);

int main()
{
    // Tạo socket
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

    // Gán địa chỉ server vào socket
    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server, NUMBER_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for new client on %s:%d\n",
           inet_ntoa(server_addr.sin_addr),
           ntohs(server_addr.sin_port));

    pthread_t thread_id;
    for (int i = 0; i < NUMBER_THREADS; i++)
    {
        if (pthread_create(&thread_id, NULL, client_proc, &server) != 0)
        {
            perror("pthread_create() failed");
            sched_yield();
        }
    }

    pthread_join(thread_id, NULL);

    getchar();
    // Đóng socket
    close(server);

    return 0;
}

void *client_proc(void *param)
{
    int server = *(int *)param;
    char buf[BUF_SIZE];

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
        printf("New client from %s:%d accepted in thread %ld with pid %d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), pthread_self(), getpid());

        // Nhận dữ liệu từ client
        int len = recv(client, buf, BUF_SIZE, 0);
        if (len < 0)
        {
            perror("recv() failed");
            continue;
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
            close(client);
            continue;
        }
        else
        {
            buf[strcspn(buf, "\n")] = 0;
            printf("Received %d bytes from client %d: %s\n", len, client, buf);

            // Gửi dữ liệu cho client
            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html> \n";
            if (send(client, msg, strlen(msg), 0) < 0)
            {
                perror("send() failed");
                exit(EXIT_FAILURE);
            }
        }

        // Đóng kết nối
        printf("Client from %s:%d disconnected\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        close(client);
    }

    return NULL;
}