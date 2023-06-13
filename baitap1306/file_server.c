#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 9000
#define BUFFER_SIZE 1024

void send_file_list(int client_socket);
void send_file_content(int client_socket, char* filename);

int main() {
    int server_fd, client_socket, pid;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("Socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        // Accept new client connection
        client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("Socket accept failed");
            exit(EXIT_FAILURE);
        }

        // Fork a new process to handle the client request
        pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process: handle client request
            close(server_fd);

            // Send file list to the client
            send_file_list(client_socket);

            // Receive the requested filename from client
            char filename[BUFFER_SIZE];
            memset(filename, 0, sizeof(filename));
            read(client_socket, filename, sizeof(filename));

            // Remove trailing newline character
            filename[strcspn(filename, "\n")] = '\0';

            // Send file content to the client
            send_file_content(client_socket, filename);

            // Close the client socket
            close(client_socket);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            close(client_socket);
        }
    }

    // Close the server socket
    close(server_fd);

    return 0;
}

void send_file_list(int client_socket) {
    DIR *dir;
    struct dirent *ent;

    dir = opendir(".");
    if (dir == NULL) {
        perror("Unable to open directory");
        char *error_msg = "ERRORNofilestodownload\r\n";
        write(client_socket, error_msg, strlen(error_msg));
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    int file_count = 0;
    char file_list[BUFFER_SIZE];
    memset(file_list, 0, sizeof(file_list));

    // Iterate over the directory entries and count files
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == DT_REG) {
            file_count++;
            strcat(file_list, ent->d_name);
            strcat(file_list, "\r\n");
        }
    }

    closedir(dir);

    // Send the file list to the client
    if (file_count > 0) {
        char response[BUFFER_SIZE];
        sprintf(response, "OK%d\r\n%s\r\n\r\n", file_count, file_list);
        write(client_socket, response, strlen(response));
    } else {
        char *error_msg = "ERRORNofilestodownload\r\n";
        write(client_socket, error_msg, strlen(error_msg));
        close(client_socket);
        exit(EXIT_FAILURE);
    }
}

void send_file_content(int client_socket, char* filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        char *error_msg = "ERRORFileNotFound\r\n";
        write(client_socket, error_msg, strlen(error_msg));

        char response[BUFFER_SIZE];
        memset(response, 0, sizeof(response));
        read(client_socket, response, sizeof(response));

        // Remove trailing newline character
        response[strcspn(response, "\n")] = '\0';

        // Retry with the new filename
        send_file_content(client_socket, response);
    } else {
        // Calculate the file size
        fseek(file, 0L, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        // Send the file size to the client
        char response[BUFFER_SIZE];
        sprintf(response, "OK %ld\r\n", file_size);
        write(client_socket, response, strlen(response));

        // Send the file content to the client
        char buffer[BUFFER_SIZE];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            write(client_socket, buffer, bytes_read);
        }

        fclose(file);
    }
}