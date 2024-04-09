#define _XOPEN_SOURCE          1 /* Used this so it can be compilied on debian */
#define _XOPEN_SOURCE_EXTENDED 1 /* Same */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> // Include sys/socket.h for socket related functions
#include <sys/wait.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <ftw.h>
#include <fcntl.h>
#include <time.h> // Include time.h for ctime function

#define SERVER_PORT 1612
#define MAX_CONNECTIONS 5
#define MAX_COMMAND_LENGTH 1000

struct dir_data {
    char *dir_name;
    char *time;
};

struct dir_data *dir_list;
int dir_count;

int Traverse_Directories_Recursively(const char *path, const struct stat *file_stats, int type_flag, struct FTW *file_dir_info);

void Sort_Directories(char *sort_by);

void Get_Directories();

void Free_Directories();

void connection_handler(int new_socket);

int main();

int Traverse_Directories_Recursively(const char *path, const struct stat *file_stats, int type_flag, struct FTW *file_dir_info) {
    if (type_flag == FTW_D) {
        // If dir starts with '.' then skip it
        if (path[strlen(path) - 1] == '.' || path[strlen(path) - 2] == '/') {
            return 0;
        }

        if (dir_count >= 1000) {
            dir_list = (struct dir_data *) realloc(dir_list, 2 * dir_count * sizeof(struct dir_data));
        }
        dir_list[dir_count].dir_name = (char *) malloc(strlen(path) + 1);
        strcpy(dir_list[dir_count].dir_name, path);
        char *time_str = ctime(&file_stats->st_mtime);
        dir_list[dir_count].time = (char *) malloc(strlen(time_str) + 1);
        strcpy(dir_list[dir_count].time, time_str);
        dir_count++;

        printf("Directory: %s\n", path);
        printf("Last modified time: %s\n", time_str);
    }
    return 0;
}

void Sort_Directories(char *sort_by) {
    if (strcmp(sort_by, "time") == 0) {
        for (int i = 0; i < dir_count; i++) {
            for (int j = i + 1; j < dir_count; j++) {
                if (strcmp(dir_list[i].time, dir_list[j].time) < 0) {
                    struct dir_data temp = dir_list[i];
                    dir_list[i] = dir_list[j];
                    dir_list[j] = temp;
                }
            }
        }
    } else if (strcmp(sort_by, "name") == 0) {
        for (int i = 0; i < dir_count; i++) {
            for (int j = i + 1; j < dir_count; j++) {
                if (strcmp(dir_list[i].dir_name, dir_list[j].dir_name) < 0) {
                    struct dir_data temp = dir_list[i];
                    dir_list[i] = dir_list[j];
                    dir_list[j] = temp;
                }
            }
        }
    }
}

void Get_Directories() {
    dir_list = (struct dir_data *) malloc(1000 * sizeof(struct dir_data));
    dir_count = 0;
    nftw("/home/modi97/Desktop", Traverse_Directories_Recursively, 20, 0);
}

void Free_Directories() {
    for (int i = 0; i < dir_count; i++) {
        free(dir_list[i].dir_name);
        free(dir_list[i].time);
    }
    free(dir_list);
}

void connection_handler(int new_socket) {
    char client_code[MAX_COMMAND_LENGTH];

    while (1) {
        ssize_t bytes_received = read(new_socket, client_code, MAX_COMMAND_LENGTH);
        if (bytes_received < 0) {
            perror("read failed");
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            printf("Client disconnected\n");
            break;
        }

        printf("Received command from client: %s\n", client_code);

        if (strcmp(client_code, "quitc") == 0) {
            char *message = "Connection closed by the client.";
            ssize_t bytes_sent = write(new_socket, message, strlen(message));
            if (bytes_sent < 0) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }
            break;
        }

        if (strcmp(client_code, "dirlist -t") == 0) {
            Get_Directories();
            Sort_Directories("name");
            char *message = "List of directories in the home directory:\n";
            ssize_t bytes_sent = write(new_socket, message, strlen(message));
            if (bytes_sent < 0) {
                perror("write failed");
                exit(EXIT_FAILURE);
            }
            for (int i = 0; i < dir_count; i++) {

                char *temp = (char *) malloc(strlen(dir_list[i].dir_name) *sizeof(char) + 2);
                strcpy(temp, dir_list[i].dir_name);
                strcat(temp, "\n");
                bytes_sent = write(new_socket, temp, strlen(temp));
                if (bytes_sent < 0) {
                    perror("write failed");
                    exit(EXIT_FAILURE);
                }
                // bytes_sent = write(new_socket, dir_list[i].time, strlen(dir_list[i].time));
                // if (bytes_sent < 0) {
                //     perror("write failed");
                //     exit(EXIT_FAILURE);
                // }
            }
            Free_Directories();
        }

        char *message = "This can be any Response from server.";
        ssize_t bytes_sent = write(new_socket, message, strlen(message));
        if (bytes_sent < 0) {
            perror("write failed");
            exit(EXIT_FAILURE);
        }
    }

    close(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on:\n");

    struct sockaddr_in local_address;
    socklen_t local_address_len = sizeof(local_address);
    if (getsockname(server_fd, (struct sockaddr *) &local_address, &local_address_len) == -1) {
        perror("getsockname failed");
    } else {
        char ip_address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(local_address.sin_addr), ip_address, INET_ADDRSTRLEN);
        printf("IP address: %s\n", ip_address);
        printf("Port: %d\n", ntohs(local_address.sin_port));
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        int client_process = fork();

        if (client_process == 0) {
            connection_handler(new_socket);
            exit(EXIT_SUCCESS);
        } else if (client_process < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
