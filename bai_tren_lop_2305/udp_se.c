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
#include <sys/types.h>
#include <netinet/in.h>

#define MAX_LENGTH 20

int main(int argc, char *argv[])
{
    // Kiểm tra đầu vào
    if (argc != 4)
    {
        printf("Dau vao k du");
    }

    // Tạo socket
    int sender = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sender < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server receiver
    struct sockaddr_in sen_addr;
    sen_addr.sin_family = AF_INET;
    sen_addr.sin_addr.s_addr = inet_addr(argv[1]);
    sen_addr.sin_port = htons(atoi(argv[2]));

    struct sockaddr_in rec_addr;
    rec_addr.sin_family = AF_INET;
    rec_addr.sin_addr.s_addr = inet_addr(argv[1]);
    rec_addr.sin_port = htons(atoi(argv[3]));

    int recver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (bind(recver, (struct sockaddr *)&rec_addr, sizeof(rec_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

// Khởi tạo mảng file descriptor
        fd_set fdread, fdtest;
        FD_ZERO(&fdread);
 FD_SET(STDIN_FILENO, &fdread);
        FD_SET(recver, &fdread);
        char buf[256];
    while(1) {
        fdtest = fdread;
        int max = sender + 1;
        select(recver + 1, &fdtest, NULL, NULL ,NULL);

        if (FD_ISSET(STDIN_FILENO, &fdtest)) // Kiểm tra bàn phím 
         {
        fgets(buf, sizeof(buf), stdin);
        sendto(sender, buf, strlen(buf), 0, (struct sockaddr *)&sen_addr, sizeof(sen_addr));
        }

        if (FD_ISSET(recver, &fdtest)) // Kiểm tra socket client
        {
        int ret = recvfrom(recver, buf, sizeof(buf),0, NULL, NULL);
        buf[ret] = 0;
            printf("Received: %s\n"
                , buf);
        }
     
    }
    // Đóng socket
    close(sender);
    close(recver);

    return 0;
}