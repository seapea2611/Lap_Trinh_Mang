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
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>

// void signalHandler(int signo)
// {
//     int pid = wait(NULL);
//     printf("Child %d terminated.\n", pid);
// }

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
    addr.sin_port = htons(9000);

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

    while (1)
    {
        printf("Waiting for new client.\n");
        int client = accept(listener, NULL, NULL);
        printf("New client accepted: %d.\n", client);
        if (fork() == 0)
        {
            close(listener);
            char buf[256];
            char filenames[1000] = "";
            int count = 0;
            while (1)
            {
                DIR *dir;
                struct dirent *entry;

                // Mở thư mục
                dir = opendir(".");
                if (dir == NULL)
                {
                    char *msg = "ERROR No files to download\r\n";
                    send(client, msg,strlen(msg), 0);
                    return 1;
                }

                // Duyệt các tệp và thư mục trong thư mục
                while ((entry = readdir(dir)) != NULL)
                {
                    strcat(filenames, entry->d_name);  // Ghép tên vào chuỗi kết quả
                    strcat(filenames, "\n");  // Thêm khoảng trắng giữa các tên
                    if (entry->d_type == DT_REG) {  // Kiểm tra nếu là file
                         count++;
                    }
                }
                if(count == 0) {
                    char *msg = "ERROR No files to download\r\n";
                    send(client, msg,strlen(msg), 0);
                    return 1;
                }
                // char *buff = "OK %d\r\n%s, count, filenames";
                char buff[2000];
                sprintf(buff, "OK %d\r\n%s", count, filenames);
                send(client, buff, strlen(buff), 0);

                int ret = recv(client, buf, sizeof(buf), 0);
                if(ret < 0) {
                    break;
                }
                buf[ret] = 0;
                printf("Received from %d: %s\n", client, buf);
                buf[ret] = '\0';
                FILE *fp = fopen(buf, "rb");
                
                fseek(fp, 0, SEEK_END); // Di chuyển con trỏ tệp đến cuối file
                long size = ftell(fp);  // Lấy vị trí con trỏ tệp, tức là kích thước file
                fseek(fp,0,SEEK_SET);
                sprintf(buf, "OK %ld\r\n", size);
                send(client, buf, strlen(buf), 0);
                printf("helo");
            
                // Đóng thư mục
                closedir(dir);
                fclose(fp);
                close(client);
            }
            close(client);
            exit(0);
        }

        close(client);
    }

    getchar();
    killpg(0, SIGKILL);

    close(listener);

    return 0;
}