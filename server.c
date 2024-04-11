#define _XOPEN_SOURCE 500 // For FTW_PHYS
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ftw.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>

#define SERVER_PORT 8081
#define MIRROR_1_PORT 9001
#define MIRROR_2_PORT 10001
#define MAX_CONNECTIONS 5
#define MAX_COMMAND_LENGTH 1024
#define MAX_TOKEN_COUNT 10
#define MAX_PATH_LEN 1024
#define MAX_DIRS 1000
#define MAX_RESPONSE_LENGTH 102400

// * Temporary Directory Path for TarKdm
char* tempDirPath = "/var/tmp/a4TarTempDir";

#define INVALID_IP_ERROR "Error: Invalid IP Address\n"
#define INVALID_PORT_ERROR "Error: Invalid Port Number\n"
#define ALLOCATION_ERROR "Error allocating memory\n"
#define INVALID_ARGUMENTS_ERROR "Error: Invalid number of arguments for %s\n"
#define INVALID_COMMAND_ERROR "Error: Invalid command\n"
#define COMPLETE_MESSAGE "@#COMPLETE#@"
#define FILE_NOT_FOUND_ERROR "File not found"

struct DirectoryInfo {
    char name[1024];
    time_t creation_time;
};

struct FileInfo {
    char name[1024];
    time_t creation_time;
    size_t size;
    char file_path[1024];
    char file_permissions[10];
    char file_extension[10];
};

// Common Var For File Commands
struct FileInfo *file_list[1024];
int file_count = 0;

// Common Var For File Search
int single_file_search = 0;
char *fileNameToSearch = NULL;

// Common Var For File Extension Search
char *file_extensions[3] = {0};
int search_by_extension = 0;

// Common Var For File Size Search
int search_by_size = 0;
int upper_bound_size = 0;
int lower_bound_size = 0;

// Common Var For File Date Search
int search_by_date_before = 0;
int search_by_date_after = 0;
time_t provided_date = 0;




// Function to reset all the global variables
void reset_global_variables() {
    single_file_search = 0;
    fileNameToSearch = NULL;
    search_by_extension = 0;
    search_by_size = 0;
    search_by_date_before = 0;
    search_by_date_after = 0;
    provided_date = 0;
    upper_bound_size = 0;
    lower_bound_size = 0;
    file_count = 0;
    // File List
    for (int i = 0; i < 1024; i++) {
        file_list[i] = NULL;
    }
    // File Extensions
    for (int i = 0; i < 3; i++) {
        file_extensions[i] = NULL;
    }
}

void traverse_dir_info(struct DirectoryInfo *directories, int *num_directories, const char *dir_name) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;

    dir = opendir(dir_name);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_name[0] == '.')
            continue;

        char path[MAX_PATH_LEN];
        sprintf(path, "%s/%s", dir_name, entry->d_name);
        if (lstat(path, &file_info) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(file_info.st_mode)) {
            if (*num_directories < 100) { // Ensure array bounds are not exceeded
                strncpy(directories[*num_directories].name, entry->d_name, sizeof(directories[*num_directories].name));
                directories[*num_directories].creation_time = file_info.st_ctime;
                (*num_directories)++;
                traverse_dir_info(directories, num_directories, path); // Recursive call
            } else {
                fprintf(stderr, "Maximum number of directories exceeded.\n");
                break;
            }
        }
    }

    closedir(dir);
}

void free_dir_info(struct DirectoryInfo *directories) {
    free(directories);
}

void free_file_info(struct FileInfo *files) {
    free(files);
}

int compare_directories_by_name(const void *a, const void *b) {
    const struct DirectoryInfo *dir1 = (const struct DirectoryInfo *)a;
    const struct DirectoryInfo *dir2 = (const struct DirectoryInfo *)b;
    return strcasecmp(dir1->name, dir2->name);
}

int compare_directories_by_creation_time(const void *a, const void *b) {
    const struct DirectoryInfo *dir1 = (const struct DirectoryInfo *)a;
    const struct DirectoryInfo *dir2 = (const struct DirectoryInfo *)b;
    return difftime(dir1->creation_time, dir2->creation_time);
}

void sort_directories(struct DirectoryInfo *directories, int num_directories, const char *sort_key) {
    if (strcmp(sort_key, "name") == 0) {
        qsort(directories, num_directories, sizeof(struct DirectoryInfo), compare_directories_by_name);
    } else if (strcmp(sort_key, "creation_time") == 0) {
        qsort(directories, num_directories, sizeof(struct DirectoryInfo), compare_directories_by_creation_time);
    }
}

int Traverse_Files_Recursively(const char *filePath, const struct stat *fileStats, int typeFlag, struct FTW *fileDirInfo) {
    if (typeFlag != FTW_F) {
        return 0;
    }

    const char *current_file_name = filePath + fileDirInfo->base;

    if (single_file_search) {
        if (strcasecmp(fileNameToSearch, current_file_name) == 0) {
            struct FileInfo *file_data = (struct FileInfo *)malloc(sizeof(struct FileInfo));
            if (file_data == NULL) {
                // Handle allocation failure
                fprintf(stderr, ALLOCATION_ERROR);
                return 1;
            }

            strncpy(file_data->name, current_file_name, sizeof(file_data->name));
            file_data->creation_time = fileStats->st_ctime;
            file_data->size = fileStats->st_size;
            strncpy(file_data->file_path, filePath, sizeof(file_data->file_path));
            snprintf(file_data->file_permissions, sizeof(file_data->file_permissions), "%s%s%s%s%s%s%s%s%s",
                     S_ISDIR(fileStats->st_mode) ? "d" : "-",
                     (fileStats->st_mode & S_IRUSR) ? "r" : "-",
                     (fileStats->st_mode & S_IWUSR) ? "w" : "-",
                     (fileStats->st_mode & S_IXUSR) ? "x" : "-",
                     (fileStats->st_mode & S_IRGRP) ? "r" : "-",
                     (fileStats->st_mode & S_IWGRP) ? "w" : "-",
                     (fileStats->st_mode & S_IXGRP) ? "x" : "-",
                     (fileStats->st_mode & S_IROTH) ? "r" : "-",
                     (fileStats->st_mode & S_IWOTH) ? "w" : "-",
                     (fileStats->st_mode & S_IXOTH) ? "x" : "-");
            file_list[file_count] = file_data;
            file_count++;
            return 1; // Stop NFTW Process as file is found
        }
    }

    if (search_by_size) {
        if (fileStats->st_size >= lower_bound_size && fileStats->st_size <= upper_bound_size) {
            struct FileInfo *file_data = (struct FileInfo *)malloc(sizeof(struct FileInfo));
            if (file_data == NULL) {
                // Handle allocation failure
                fprintf(stderr, ALLOCATION_ERROR);
                return 1;
            }

            strncpy(file_data->name, current_file_name, sizeof(file_data->name));
            file_data->size = fileStats->st_size;
            strncpy(file_data->file_path, filePath, sizeof(file_data->file_path));

            file_list[file_count] = file_data;
            file_count++;
            return 0;
        }
    }

    if (search_by_extension) {
        for (int i = 0; i < 3; i++) {
            if (file_extensions[i] != NULL) {
                if (strcasecmp(file_extensions[i], current_file_name + strlen(current_file_name) - strlen(file_extensions[i])) == 0) {
                    struct FileInfo *file_data = (struct FileInfo *)malloc(sizeof(struct FileInfo));
                    if (file_data == NULL) {
                        // Handle allocation failure
                        fprintf(stderr, ALLOCATION_ERROR);
                        return 1;
                    }

                    strncpy(file_data->name, current_file_name, sizeof(file_data->name));
                    strncpy(file_data->file_path, filePath, sizeof(file_data->file_path));
                    strncpy(file_data->file_extension, file_extensions[i], sizeof(file_data->file_extension));

                    file_list[file_count] = file_data;
                    file_count++;
                    return 0;
                }
            }
        }
    }

    if (search_by_date_before) {
        if (fileStats->st_ctime < provided_date) {
            struct FileInfo *file_data = (struct FileInfo *)malloc(sizeof(struct FileInfo));
            if (file_data == NULL) {
                // Handle allocation failure
                fprintf(stderr, ALLOCATION_ERROR);
                return 1;
            }

            strncpy(file_data->name, current_file_name, sizeof(file_data->name));
            file_data->creation_time = fileStats->st_ctime;
            strncpy(file_data->file_path, filePath, sizeof(file_data->file_path));

            file_list[file_count] = file_data;
            file_count++;
            return 0;
        }
    }

    if (search_by_date_after) {
        if (fileStats->st_ctime > provided_date) {
            struct FileInfo *file_data = (struct FileInfo *)malloc(sizeof(struct FileInfo));
            if (file_data == NULL) {
                // Handle allocation failure
                fprintf(stderr, ALLOCATION_ERROR);
                return 1;
            }

            strncpy(file_data->name, current_file_name, sizeof(file_data->name));
            file_data->creation_time = fileStats->st_ctime;
            strncpy(file_data->file_path, filePath, sizeof(file_data->file_path));

            file_list[file_count] = file_data;
            file_count++;
            return 0;
        }
    }

    return 0; // No matching criteria found
}


int create_tar_and_send(int new_socket){
    // Create dir with timestamp in /var/tmp/a4TarTempDir
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char tempTarDirName[1024];
    sprintf(tempTarDirName, "Temp%d%d%d%d%d%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    char tempDirFullPath[1024];
    sprintf(tempDirFullPath, "%s/%s", tempDirPath, tempTarDirName);
    mkdir(tempDirFullPath, 0777);

    for (int i = 0; i < file_count; i++) {
        char copyCommand[1024];
        sprintf(copyCommand, "cp %s %s", file_list[i]->file_path, tempDirFullPath);
        printf("Copy Command: %s\n", copyCommand);
        system(copyCommand);
    }

    // Tar the Files
    char tarCommand[1024];
    sprintf(tarCommand, "tar -czf /var/tmp/a4TarTempDir/%s.tar.gz -C %s .", tempTarDirName, tempDirFullPath);
    system(tarCommand);

    char tarFilePath[1024];
    sprintf(tarFilePath, "/var/tmp/a4TarTempDir/%s.tar.gz", tempTarDirName);

    // Send Tar file in chunks
    FILE *tarFile = fopen(tarFilePath, "rb");
    if (tarFile == NULL) {
        fprintf(stderr, "Error Opening Tar File\n");
        return 0;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), tarFile)) > 0) {
        send(new_socket, buffer, bytesRead, 0);
        printf("Bytes Sent: %ld\n", bytesRead);
    }
    fclose(tarFile);

    // Delete Temp Tar File Created and Temp Directory for that tar
    char deleteCommand[1024];
    sprintf(deleteCommand, "rm -rf %s %s", tempDirFullPath, tarFilePath);
    system(deleteCommand);

    return 1;
}

char **tokenizer(char *command, int *token_count) {
    char **command_tokens = (char **)malloc((MAX_TOKEN_COUNT + 1) * sizeof(char *));
    if (command_tokens == NULL) {
        // Handle allocation failure
        fprintf(stderr, ALLOCATION_ERROR);
        return NULL;
    }

    char *token = strtok(command, " ");
    *token_count = 0;
    while (token != NULL && *token_count < MAX_TOKEN_COUNT) {
        command_tokens[*token_count] = strdup(token);
        if (command_tokens[*token_count] == NULL) {
            // Handle allocation failure
            fprintf(stderr, ALLOCATION_ERROR);
            for (int i = 0; i < *token_count; i++) {
                free(command_tokens[i]);
            }
            free(command_tokens);
            return NULL;
        }
        (*token_count)++;
        token = strtok(NULL, " ");
    }
    command_tokens[*token_count] = NULL;
    return command_tokens;
}

void crequest(int new_socket) {
    // Buffer to store the command received from the client
    char client_code[1024] = {0};

    while (1) {
        // Read the command from the client

        // CReate Temp Directory
        int mkdirReturn = system("mkdir -p /var/tmp/a4TarTempDir");
        if (mkdirReturn != 0) {
            perror("Error Creating Temp Directory");
        }
        recv(new_socket, client_code, MAX_COMMAND_LENGTH,0);

        printf("Received command: %s from socket: %d\n", client_code, new_socket);

        int token_count = 0;
        char **command_tokens = tokenizer(client_code, &token_count);

        char message[MAX_RESPONSE_LENGTH];

        if (strcmp(command_tokens[0], "dirlist") == 0) {
            if (strcmp(command_tokens[1], "-a") == 0 || strcmp(command_tokens[1], "-t") == 0) {
                struct DirectoryInfo *directories = malloc(MAX_PATH_LEN * sizeof(struct DirectoryInfo));

                int num_directories = 0;

                traverse_dir_info(directories, &num_directories, "/home/modi97/Desktop/Files");

                if (strcmp(command_tokens[1], "-a") == 0) {
                    sort_directories(directories, num_directories, "name");
                    for (int i = 0; i < num_directories; i++) {
                        strcpy(message, directories[i].name);
                        strcat(message, "\n");
                        send(new_socket, message, MAX_RESPONSE_LENGTH,0);
                    }
                }
                if (strcmp(command_tokens[1], "-t") == 0) {
                    sort_directories(directories, num_directories, "creation_time");
                    for (int i = 0; i < num_directories; i++) {
                        strcpy(message, directories[i].name);
                        strcat(message, "\n");
                        send(new_socket, message, MAX_RESPONSE_LENGTH,0);
                    }
                }
                free_dir_info(directories);
            }
        } else if (strcmp(command_tokens[0], "w24fn") == 0) {
            if(token_count != 2) {
                sprintf(message, INVALID_ARGUMENTS_ERROR, command_tokens[0]);
                send(new_socket, message, strlen(message),0);
                continue;
            }
            fileNameToSearch = command_tokens[1];
            single_file_search = 1;

            nftw("/home/modi97/Desktop/Files", Traverse_Files_Recursively, 20, FTW_PHYS);

            // Single File Search
            file_count = 0;
            if (file_count == 0) {
                sprintf(message, FILE_NOT_FOUND_ERROR);
                send(new_socket, message, strlen(message),0);
            } else {
                // Send File Details
                for (int i = 0; i < file_count; i++) {
                    snprintf(message, sizeof(message), "File Name: %s\nFile Path: %s\nFile Size: %ld\nFile Permissions: %s\n",
                             file_list[i]->name, file_list[i]->file_path, file_list[i]->size, file_list[i]->file_permissions);
                    send(new_socket, message, strlen(message),0);
                }
            }
        } else if (strcmp(command_tokens[0], "w24fz") == 0) {
            if(token_count != 3) {
                sprintf(message, INVALID_ARGUMENTS_ERROR, command_tokens[0]);
                send(new_socket, message, strlen(message),0);
                continue;
            }
            lower_bound_size = atoi(command_tokens[1]);
            upper_bound_size = atoi(command_tokens[2]);
            search_by_size = 1;

            nftw("/home/modi97/Desktop/Files", Traverse_Files_Recursively, 20, FTW_PHYS);

            file_count = 0;
            if (file_count == 0) {
                sprintf(message, FILE_NOT_FOUND_ERROR);
                int bytesSent = send(new_socket, message, strlen(message),0);
                printf("Bytes Sent: %d\n", bytesSent);
                printf("Message: %s\n", message);
            } else {
                // Create Tar File and Send
                int ret = create_tar_and_send(new_socket);

                if(ret){
                    printf("Tar File Created and Sent\n");
                }
                else{
                    printf("Error Creating Tar File\n");
                }
                // // Create dir with timestamp in /var/tmp/a4TarTempDir
                // time_t t = time(NULL);
                // struct tm tm = *localtime(&t);
                // char tempTarDirName[1024];
                // sprintf(tempTarDirName, "Temp%d%d%d%d%d%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

                // char tempDirFullPath[1024];
                // sprintf(tempDirFullPath, "%s/%s", tempDirPath, tempTarDirName);
                // mkdir(tempDirFullPath, 0777);

                // if (mkdirReturn != 0) {
                //     perror("Error Creating Temp Directory--");
                // }

                // for (int i = 0; i < file_count; i++) {
                //     char copyCommand[1024];
                //     sprintf(copyCommand, "cp %s %s", file_list[i]->file_path, tempTarDirName);
                //     system(copyCommand);
                // }

                // // Tar the Files
                // char tarCommand[1024];
                // sprintf(tarCommand, "tar -czf /var/tmp/a4TarTempDir/%s.tar.gz -C %s .", tempTarDirName, tempDirFullPath);
                // system(tarCommand);

                // char tarFilePath[1024];
                // sprintf(tarFilePath, "/var/tmp/a4TarTempDir/%s.tar.gz", tempTarDirName);

                // // Send Tar file in chunks
                // FILE *tarFile = fopen(tarFilePath, "rb");
                // if (tarFile == NULL) {
                //     perror("Error Opening Tar File");
                // }

                // char buffer[1024];
                // size_t bytesRead;
                // while ((bytesRead = fread(buffer, 1, sizeof(buffer), tarFile)) > 0) {
                //     send(new_socket, buffer, bytesRead, 0);
                // }
                // fclose(tarFile);

                // // Delete Temp Tar File Created and Temp Directory for that tar
                // char deleteCommand[1024];
                // sprintf(deleteCommand, "rm -rf %s %s", tempDirFullPath, tarFilePath);
                // system(deleteCommand);
            }
        } else if (strcmp(command_tokens[0], "w24ft") == 0) {
            if(token_count < 3 || token_count > 5) {
                sprintf(message, INVALID_ARGUMENTS_ERROR, command_tokens[0]);
                send(new_socket, message, strlen(message),0);
                continue;
            }
            for (int i = 1; i < token_count; i++) {
                file_extensions[i - 1] = command_tokens[i];
            }
            search_by_extension = 1;
            nftw("/home/modi97/Desktop", Traverse_Files_Recursively, 20, FTW_PHYS);

            // Send Tar File
            if (file_count == 0) {
                sprintf(message, FILE_NOT_FOUND_ERROR);
                send(new_socket, message, strlen(message),0);
            } else {
                // Create Tar File and Send
                int ret = create_tar_and_send(new_socket);

                if(ret){
                    printf("Tar File Created and Sent\n");
                }
                else{
                    printf("Error Creating Tar File\n");
                }
            }

        } else if (strcmp(command_tokens[0], "w24db") == 0) {
            if(token_count != 2) {
                sprintf(message, INVALID_ARGUMENTS_ERROR, command_tokens[0]);
                send(new_socket, message, strlen(message),0);
                continue;
            }
            provided_date = command_tokens[1];
            search_by_date_before = 1;

            nftw("/home/modi97/Desktop", Traverse_Files_Recursively, 20, FTW_PHYS);

            // Send Tar File
            if (file_count == 0) {
                sprintf(message, FILE_NOT_FOUND_ERROR);
                send(new_socket, message, strlen(message),0);
            } else {
                // Create Tar File and Send
                int ret = create_tar_and_send(new_socket);

                if(ret){
                    printf("Tar File Created and Sent\n");
                }
                else{
                    printf("Error Creating Tar File\n");
                }
            }
        } else if (strcmp(command_tokens[0], "w24da") == 0) {
            if(token_count != 2) {
                sprintf(message, INVALID_ARGUMENTS_ERROR, command_tokens[0]);
                send(new_socket, message, strlen(message),0);
                continue;
            }
            provided_date = command_tokens[1];
            search_by_date_after = 1;

            nftw("/home/modi97/Desktop", Traverse_Files_Recursively, 20, FTW_PHYS);

            // Send Tar File
            if (file_count == 0) {
                sprintf(message, FILE_NOT_FOUND_ERROR);
                send(new_socket, message, strlen(message),0);
            } else {
                // Create Tar File and Send
                int ret = create_tar_and_send(new_socket);

                if(ret){
                    printf("Tar File Created and Sent\n");
                }
                else{
                    printf("Error Creating Tar File\n");
                }
            }
        } else if (strcmp(command_tokens[0], "quitc") == 0) {
            break;
        } else {
            sprintf(message, INVALID_COMMAND_ERROR);
            send(new_socket, message, strlen(message),0);
        }
        int finalBytes = send(new_socket, COMPLETE_MESSAGE, strlen(COMPLETE_MESSAGE),0);
        printf("Final Bytes Sent: %d\n", finalBytes);
        reset_global_variables();
    }

    // Closing the socket
    close(new_socket);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // Binding socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for connections
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on: \n ");

    system("hostname -i");

    printf("%d\n", SERVER_PORT);

    // Changing directory to ~
    if (chdir("/home/modi97") != 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accepting incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        int client_process = fork();
        if (client_process == 0) {
            reset_global_variables();
            crequest(new_socket);
        }
    }

    return 0;
}
