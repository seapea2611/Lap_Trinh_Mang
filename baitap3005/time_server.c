#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <regex.h>
#include <time.h>
#define BUF_SIZE 1024

void get_time(char *buf[]) {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    if (strcmp(buf, "dd/mm/yyyy") == 0)
    {
        strftime(buf, BUF_SIZE, "%d/%m/%Y\n", timeinfo);
    }
    else if (strcmp(buf, "dd/mm/yy") == 0)
    {
        strftime(buf, BUF_SIZE, "%d/%m/%y\n", timeinfo);
    }
    else if (strcmp(buf, "mm/dd/yyyy") == 0)
    {
        strftime(buf, BUF_SIZE, "%m/%d/%Y\n", timeinfo);
    }
    else if (strcmp(buf, "mm/dd/yy") == 0)
    {
        strftime(buf, BUF_SIZE, "%m/%d/%y\n", timeinfo);
    }
    else
    {
        strcpy(buf, "Invalid format\n");
    }
} 

void signalHandler(int signo)
{
    int pid = wait(NULL);
    printf("Child %d terminated.\n", pid);
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

    signal(SIGCHLD, signalHandler);
    
    while (1)
    {
        printf("Waiting for new client.\n");
        int client = accept(listener, NULL, NULL);
        printf("New client accepted: %d.\n", client);
        if (fork() == 0)
        {
            close(listener);
            char buf[256];
            while (1)
            {
                char *msg = "Enter command: ";
                int ret = recv(client, buf, sizeof(buf), 0);
                if (ret <= 0)
                    break;
                buf[ret] = 0;
                char buf1[9], buf2[11];
                int a = sscanf(buf, "%[^[][%[^]]", buf1, buf2);
                if(strcmp(buf1,"GET_TIME") == 0){
                    printf("Received from %d: %s\n", client, buf);
                    get_time(buf2);
                    send(client,buf2,strlen(buf2),0);
                }
                else {
                send(client, msg, strlen(msg), 0);
                }

            }
            close(client);
            exit(EXIT_SUCCESS);
        }

        close(client);
    }

    close(listener);    

    return 0;
}