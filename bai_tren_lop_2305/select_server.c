#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

void normalizeString(char *str)
{
    // Xóa bỏ dấu cách thừa
    int i, j;
    int len = strlen(str);
    for (i = 0, j = 0; i < len; i++)
    {
        if (isspace((unsigned char)str[i]))
        {
            if (i > 0 && !isspace((unsigned char)str[i - 1]))
            {
                str[j++] = ' ';
            }
        }
        else
        {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';

    // Viết hoa chữ cái đầu
    len = strlen(str);
    for (i = 0; i < len; i++)
    {
        if (i == 0 || isspace((unsigned char)str[i - 1]))
        {
            str[i] = toupper((unsigned char)str[i]);
        }
        else
        {
            str[i] = tolower((unsigned char)str[i]);
        }
    }
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9090);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        return 1;
    }

    fd_set fdread;
    int clients[64];
    int num_clients = 0;

    char buf[256];
    char bye[10] = "Goodbye!";

    // struct timeval tv;

    while (1)
    {
        FD_ZERO(&fdread);

        FD_SET(listener, &fdread);
        for (int i = 0; i < num_clients; i++)
            FD_SET(clients[i], &fdread);

        // tv.tv_usec = 0;
        // tv.tv_sec = 5;

        int ret = select(FD_SETSIZE, &fdread, NULL, NULL, NULL);

        if (ret < 0)
        {
            perror("select() failed");
            break;
        }

        if (ret == 0)
        {
            printf("Timed out.\n");
            continue;
        }

        if (FD_ISSET(listener, &fdread))
        {
            int client = accept(listener, NULL, NULL);
            // TODO: Kiem tra gioi han
            clients[num_clients++] = client;
            printf("New client connected %d\n", client);
        }

        for (int i = 0; i < num_clients; i++)
            if (FD_ISSET(clients[i], &fdread))
            {
                int ret = recv(clients[i], buf, sizeof(buf), 0);
                if (ret <= 0)
                {
                    // TODO: Xoa client ra khoi mang
                    continue;
                }
                if (strcmp(buf, "exit\n") == 0)
                {
                    if (send(clients[i], bye, strlen(bye), 0))
                    {
                        close(clients[i]);
                        FD_CLR(clients[i], &fdread);
                    }
                }
                
                else
                {
                    buf[ret] = 0;
                    printf("Received from %d: %s\n", clients[i], buf);
                    normalizeString(buf);
                    send(clients[i], buf, strlen(buf), 0);
                }
            }
    }

    close(listener);

    return 0;
}