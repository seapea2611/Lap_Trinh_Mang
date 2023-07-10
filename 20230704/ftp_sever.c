#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_COMMAND_SIZE 256
#define MAX_RESPONSE_SIZE 2048

int main()
{
    int ctrl_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in ctrl_addr;
    ctrl_addr.sin_family = AF_INET;
    ctrl_addr.sin_addr.s_addr = inet_addr("172.23.208.1");
    ctrl_addr.sin_port = htons(21);

    if (connect(ctrl_socket, (struct sockaddr *)&ctrl_addr, sizeof(ctrl_addr)))
    {
        perror("connect() failed");
        return 1;
    }

    char buf[MAX_RESPONSE_SIZE];
    int len = recv(ctrl_socket, buf, sizeof(buf), 0);
    buf[len] = 0;
    printf("%s", buf);

    char username[64], password[64];

    while (1)
    {
        printf("Nhap username: ");
        scanf("%s", username);
        printf("Nhap password: ");
        scanf("%s", password);

        sprintf(buf, "USER %s\r\n", username);
        send(ctrl_socket, buf, strlen(buf), 0);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        sprintf(buf, "PASS %s\r\n", password);
        send(ctrl_socket, buf, strlen(buf), 0);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        if (strncmp(buf, "230", 3) == 0)
        {
            printf("Dang nhap thanh cong.\n");
            break;
        }
        else
        {
            printf("Dang nhap that bai.\n");
        }
    }

    char command[MAX_COMMAND_SIZE];
    int useEPSV = 0;
    while (1)
    {
        printf("Nhap lenh PASV hoac EPSV: ");
        scanf("%s", command);

        if (strcasecmp(command, "PASV") == 0)
        {
            useEPSV = 0;
            break;
        }
        else if (strcasecmp(command, "EPSV") == 0)
        {
            useEPSV = 1;
            break;
        }
        else
        {
            printf("Lenh khong hop le. Vui long nhap lai.\n");
        }
    }

    if (useEPSV == 0)
    {
        send(ctrl_socket, "PASV\r\n", 6, 0);
        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        char *pos1 = strchr(buf, '(') + 1;
        char *pos2 = strchr(pos1, ')');
        char temp[256];
        int n = pos2 - pos1;
        memcpy(temp, pos1, n);
        temp[n] = 0;

        char *p = strtok(temp, ",");
        int i1 = atoi(p);
        p = strtok(NULL, ",");
        int i2 = atoi(p);
        p = strtok(NULL, ",");
        int i3 = atoi(p);
        p = strtok(NULL, ",");
        int i4 = atoi(p);
        p = strtok(NULL, ",");
        int p1 = atoi(p);
        p = strtok(NULL, ",");
        int p2 = atoi(p);

        struct sockaddr_in data_addr;
        data_addr.sin_family = AF_INET;
        sprintf(temp, "%d.%d.%d.%d", i1, i2, i3, i4);
        data_addr.sin_addr.s_addr = inet_addr(temp);
        data_addr.sin_port = htons(p1 * 256 + p2);

        int data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr)))
        {
            perror("connect() failed");
            return 1;
        }

        char *filename = "hi.txt";
        sprintf(buf, "STOR %s\r\n", filename);
        send(ctrl_socket, buf, strlen(buf), 0);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        FILE *file = fopen(filename, "rb");
        if (file == NULL)
        {
            printf("Khong the mo file.\n");
            return 1;
        }

        while (1)
        {
            int bytesRead = fread(buf, 1, sizeof(buf), file);
            if (bytesRead <= 0)
                break;

            send(data_socket, buf, bytesRead, 0);
        }

        fclose(file);
        close(data_socket);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        send(ctrl_socket, "QUIT\r\n", 6, 0);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        close(ctrl_socket);
    }
    else
    {
        send(ctrl_socket, "EPSV\r\n", 6, 0);
        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        char *pos1 = strchr(buf, '(') + 1;
        char *pos2 = strchr(pos1, ')');
        char temp[256];
        int n = pos2 - pos1;
        memcpy(temp, pos1, n);
        temp[n] = 0;

        char *p = strtok(temp, "|");
        int p1 = atoi(p);

        struct sockaddr_in data_addr;
        data_addr.sin_family = AF_INET;
        data_addr.sin_addr.s_addr = inet_addr("172.23.208.1");
        data_addr.sin_port = htons(p1);

        int data_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr)))
        {
            perror("connect() failed");
            return 1;
        }

        char *filename = "hi.txt";
        sprintf(buf, "STOR %s\r\n", filename);
        send(ctrl_socket, buf, strlen(buf), 0);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        FILE *file = fopen(filename, "rb");
        if (file == NULL)
        {
            printf("Khong the mo file.\n");
            return 1;
        }

        while (1)
        {
            int bytesRead = fread(buf, 1, sizeof(buf), file);
            if (bytesRead <= 0)
                break;

            send(data_socket, buf, bytesRead, 0);
        }

        fclose(file);
        close(data_socket);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        send(ctrl_socket, "QUIT\r\n", 6, 0);

        len = recv(ctrl_socket, buf, sizeof(buf), 0);
        buf[len] = 0;
        printf("%s", buf);

        close(ctrl_socket);
    }

    return 0;
}