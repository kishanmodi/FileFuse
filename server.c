#define _XOPEN_SOURCE 500 // For FTW_PHYS
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ftw.h>
#include <arpa/inet.h>
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

#define INVALID_IP_ERROR "Error: Invalid IP Address\n"
#define INVALID_PORT_ERROR "Error: Invalid Port Number\n"
#define ALLOCATION_ERROR "Error allocating memory\n"
#define INVALID_ARGUMENTS_ERROR "Error: Invalid number of arguments for %s\n"
#define INVALID_COMMAND_ERROR "Error: Invalid command\n"

struct DirectoryInfo
{
    char name[1024];
    time_t creation_time;
};

struct FileInfo
{
    char name[1024];
    time_t creation_time;
    size_t size;
};

void traverse_dir_info(struct DirectoryInfo *directories, int *num_directories, const char *dir_name)
{
    DIR *dir;
    struct dirent *entry;
    struct stat file_info;

    dir = opendir(dir_name);
    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_name[0] == '.')
            continue;

        char path[MAX_PATH_LEN];
        sprintf(path, "%s/%s", dir_name, entry->d_name);
        if (lstat(path, &file_info) == -1)
        {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(file_info.st_mode))
        {
            if (*num_directories < 100)
            { // Ensure array bounds are not exceeded
                strncpy(directories[*num_directories].name, entry->d_name, sizeof(directories[*num_directories].name));
                directories[*num_directories].creation_time = file_info.st_ctime;
                (*num_directories)++;
                traverse_dir_info(directories, num_directories, path); // Recursive call
            }
            else
            {
                fprintf(stderr, "Maximum number of directories exceeded.\n");
                break;
            }
        }
    }

    closedir(dir);
}

void free_dir_info(struct DirectoryInfo *directories)
{
    free(directories);
}

void free_file_info(struct FileInfo *files)
{
    free(files);
}

int compare_directories_by_name(const void *a, const void *b)
{
    const struct DirectoryInfo *dir1 = (const struct DirectoryInfo *)a;
    const struct DirectoryInfo *dir2 = (const struct DirectoryInfo *)b;
    return strcmp(dir1->name, dir2->name);
}

int compare_directories_by_creation_time(const void *a, const void *b)
{
    const struct DirectoryInfo *dir1 = (const struct DirectoryInfo *)a;
    const struct DirectoryInfo *dir2 = (const struct DirectoryInfo *)b;
    return difftime(dir1->creation_time, dir2->creation_time);
}

void sort_directories(struct DirectoryInfo *directories, int num_directories, const char *sort_key)
{
    if (strcmp(sort_key, "name") == 0)
    {
        qsort(directories, num_directories, sizeof(struct DirectoryInfo), compare_directories_by_name);
    }
    else if (strcmp(sort_key, "creation_time") == 0)
    {
        qsort(directories, num_directories, sizeof(struct DirectoryInfo), compare_directories_by_creation_time);
    }
}

char **tokenizer(char *command, int *token_count)
{
    char **command_tokens = (char **)malloc((MAX_TOKEN_COUNT + 1) * sizeof(char *));
    if (command_tokens == NULL)
    {
        // Handle allocation failure
        fprintf(stderr, ALLOCATION_ERROR);
        return NULL;
    }

    char *token = strtok(command, " ");
    *token_count = 0;
    while (token != NULL && *token_count < MAX_TOKEN_COUNT)
    {
        command_tokens[*token_count] = strdup(token);
        if (command_tokens[*token_count] == NULL)
        {
            // Handle allocation failure
            fprintf(stderr, ALLOCATION_ERROR);
            for (int i = 0; i < *token_count; i++)
            {
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

void connection_handler(int new_socket)
{
    char client_code[1024] = {0};

    while (1)
    {
        // Read the command from the client
        read(new_socket, client_code, MAX_COMMAND_LENGTH);

        int token_count = 0;
        char **command_tokens = tokenizer(client_code, &token_count);

        char message[MAX_RESPONSE_LENGTH];

        if (strcmp(command_tokens[0], "dirlist") == 0)
        {
            if (strcmp(command_tokens[1], "-a") == 0 || strcmp(command_tokens[1], "-t") == 0)
            {
                struct DirectoryInfo *directories = malloc(MAX_PATH_LEN * sizeof(struct DirectoryInfo));

                int num_directories = 0;

                traverse_dir_info(directories, &num_directories, "/home/patel319");

                if (strcmp(command_tokens[1], "-a") == 0)
                {
                    sort_directories(directories, num_directories, "name");

                    for (int i = 0; i < num_directories; i++)
                    {
                        strcat(message, directories[i].name);
                        strcat(message, "\n");
                    }

                    write(new_socket, message, strlen(message));
                }
                if (strcmp(command_tokens[1], "-t") == 0)
                {
                    sort_directories(directories, num_directories, "creation_time");

                    for (int i = 0; i < num_directories; i++)
                    {
                        strcat(message, directories[i].name);
                        strcat(message, "\n");
                    }

                    write(new_socket, message, strlen(message));
                }

                free_dir_info(directories);
            }
        }
        else if (strcmp(command_tokens[0], "w24fn") == 0)
        {
        }
        else if (strcmp(command_tokens[0], "w24fz") == 0)
        {
        }
        else if (strcmp(command_tokens[0], "w24ft") == 0)
        {
        }
        else if (strcmp(command_tokens[0], "w24db") == 0)
        {
        }
        else if (strcmp(command_tokens[0], "w24da") == 0)
        {
        }
        else if (strcmp(command_tokens[0], "quitc") == 0)
        {
            break;
        }

        strcpy(message, "");
    }

    close(new_socket);
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // Binding socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for connections
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on: \n ");

    system("hostname -i");

    printf("%d\n", SERVER_PORT);

    // Changing directory to ~
    if (chdir("/home/patel319") != 0)
    {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Accepting incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        int client_process = fork();

        if (client_process == 0)
        {
            connection_handler(new_socket);
        }
    }

    return 0;
}
