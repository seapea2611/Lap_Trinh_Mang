#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

void *client_thread(void *);

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
        int client = accept(listener, NULL, NULL);
        if (client == -1)
        {
            perror("accept() failed");
            continue;
        }
        printf("New client connected: %d \n", client);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_thread, &client);
        pthread_detach(thread_id);
    }

    close(listener);

    return 0;
}

void *client_thread(void *param)
{
    char buf[256];
    int client = *(int *)param;
    int ret = recv(client, buf, sizeof(buf), 0);
    buf[ret] = 0;
    puts(buf);

    char method[16], uri[256];
    sscanf(buf, "%s%s", method, uri);

    if (strcmp(method, "GET") == 0)
    {
        if (strcmp(uri, "/") == 0)
        {
             DIR *dir;
            struct dirent *entry;
            char rel_dir[256] = "../..";
            strcat(rel_dir, uri);
            dir = opendir(rel_dir);
            if (dir != NULL)
            {
                char response[4096] = "<html><body>";
                char *subresponse = malloc(512);
                while ((entry = readdir(dir)) != NULL)
                {
                    if (entry->d_type == DT_DIR)
                    {
                        sprintf(subresponse, "<a href=\"%s/\"><b>%s/</b></a><br>", entry->d_name, entry->d_name);
                    }
                    if (entry->d_type == DT_REG)
                    {
                        sprintf(subresponse, "<a href=\"%s\"><i>%s</i></a><br>", entry->d_name, entry->d_name);
                    }
                    strcat(response, subresponse);
                }
                strcat(response, "</html></body>");
                char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                send(client, header, strlen(header), 0);
                send(client, response, strlen(response), 0);
            }
            else
            {
                char *response = "HTTP/1.1 404 File not found\r\nContent-Type: text/html\r\n\r\n<html><body><h1>opendir() failed</h1></body></html>";
                send(client, response, strlen(response), 0);
            }
        }
        else
        {
            DIR *dir;
            struct dirent *entry;
            char rel_dir[256] = "../..";
            strcat(rel_dir, uri);
            dir = opendir(rel_dir);
            if (dir != NULL)
            {
                char response[4096] = "<html><body>";
                char *subresponse = malloc(512);
                while ((entry = readdir(dir)) != NULL)
                {
                    if (entry->d_type == DT_DIR)
                    {
                        sprintf(subresponse, "<a href=\"%s/\"><b>%s/</b></a><br>", entry->d_name, entry->d_name);
                    }
                    if (entry->d_type == DT_REG)
                    {
                        sprintf(subresponse, "<a href=\"%s\"><i>%s</i></a><br>", entry->d_name, entry->d_name);
                    }
                    strcat(response, subresponse);
                }
                strcat(response, "</html></body>");
                char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
                send(client, header, strlen(header), 0);
                send(client, response, strlen(response), 0);
            }
            else if (strstr(rel_dir, ".txt") != NULL || strstr(rel_dir, ".c") != NULL || strstr(rel_dir, ".cpp") != NULL)
             {
                char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
                send(client, header, strlen(header), 0);

                FILE *f = fopen(rel_dir, "rb");
                while (1)
                {
                    int len = fread(buf, 1, sizeof(buf), f);
                    if (len <= 0)
                        break;
                    send(client, buf, len, 0);
                }
                fclose(f);
            }
            else if (strstr(rel_dir, ".jpg") != NULL || strstr(rel_dir, ".png") != NULL)
            {
                FILE *f = fopen(rel_dir, "rb");

                char header[512];
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: %ld\r\n\r\n", fsize);
                send(client, header, strlen(header), 0);
                while (1)
                {
                    int len = fread(buf, 1, sizeof(buf), f);
                    if (len <= 0)
                        break;
                    send(client, buf, len, 0);
                }
                fclose(f);
            }
            else if (strstr(rel_dir, ".mp3") != NULL)
            {
                FILE *f = fopen(rel_dir, "rb");

                char header[512];
                fseek(f, 0, SEEK_END);
                long fsize = ftell(f);
                fseek(f, 0, SEEK_SET);
                sprintf(header, "HTTP/1.1 200 OK\r\nContent-Type: audio/mpeg\r\nContent-Length: %ld\r\n\r\n", fsize);
                send(client, header, strlen(header), 0);
                 while (1)
                {
                    int len = fread(buf, 1, sizeof(buf), f);
                    if (len <= 0)
                        break;
                    send(client, buf, len, 0);
                }
                fclose(f);
            }
        }
    }
    close(client);
}