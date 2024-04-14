// DEFAULT SYNTAX FOR DEFINED COMMANDS
// 1 dirlist -a -- Return All File Under Home Directory of Server --- Alphabetic Order
// 2 dirlist -t -- Return All File Under Home Directory of Server --- Time Order
// 3 w24fn <filename> -- Return File Name size (bytes), date of creation, file permissions
// 4 w24fz <size1> <size2> -- Return All Files with size between size1 and size2 in tar.gz format
// 5 w24ft <file_extension1> <file_extension2> <file_extension3> -- upto 3 file extensions, min 1 --> return all files with the given extensions in tar.gz format
// 6 w24db <date> -- Return all files created before the given date in tar.gz format
// 7 w24da <date> -- Return all files created after the given date in tar.gz format
// 8 quitc -- Close the connection with the server

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Constants
#define MAX_COMMAND_LENGTH 1024
#define MAX_TOKEN_COUNT    10
#define RESPONSE_CHUNK     1024
#define IP_LENGTH          INET_ADDRSTRLEN

// Error Messages
#define INVALID_IP_ERROR        "Error: Invalid IP Address\n"
#define INVALID_PORT_ERROR      "Error: Invalid Port Number\n"
#define ALLOCATION_ERROR        "Error allocating memory\n"
#define INVALID_ARGUMENTS_ERROR "Error: Invalid number of arguments for %s\n"
#define INVALID_COMMAND_ERROR   "Error: Invalid command\n"
#define COMPLETE_MESSAGE        "@#COMPLETE#@"
#define FILE_NOT_FOUND          "File not found"

// Global Variables
int socket_fd, port;
char ip[IP_LENGTH];

int establish_connection();
void close_connection();
int validate_ip_port();
char **tokenizer(char *command, int *token_count);
int verify_command_syntax(char **command_tokens, int tokenCount);

// Function to establish connection with server using socket
int establish_connection() {
    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        fprintf(stderr, "Error creating socket\n");
        return 0;
    }

    // Define server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // Connect to server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error connecting to server\n");
        return 0;
    }
    return 1;
}

// Function to close connection with server
void close_connection() {
    close(socket_fd);
}

// Function to validate IP address and port number
int validate_ip_port() {
    // Validate IP Address
    if (inet_addr(ip) == -1) {
        fprintf(stderr, INVALID_IP_ERROR);
        return 0;
    }
    // Validate Port Number
    if (port < 1024 || port > 65535) {
        fprintf(stderr, INVALID_PORT_ERROR);
        return 0;
    }
    return 1;
}

// Function to tokenize the command and return the tokens in an array
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

// Function to verify the syntax of the command
int verify_command_syntax(char **command_tokens, int tokenCount) {
    // Check if the command is valid
    if (strcmp(command_tokens[0], "dirlist") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "dirlist");
            return 0;
        }
        if (strcmp(command_tokens[1], "-a") != 0 && strcmp(command_tokens[1], "-t") != 0) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "dirlist");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24fn") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24fn");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24fz") == 0) {
        if (tokenCount != 3) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24fz");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24ft") == 0) {
        if (tokenCount > 4 || tokenCount < 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24ft");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24db") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24db");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24da") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24da");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "quitc") == 0) {
        if (tokenCount != 1) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "quitc");
            return 0;
        }
    } else {
        fprintf(stderr, INVALID_COMMAND_ERROR);
        return 0;  // Command is invalid
    }

    return 1;
}

// Main function
int main(int argc, char *argv[]) {
    strcpy(ip, "127.0.0.1");
    port = 8081;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Validate IP Address and Port Number
    if (validate_ip_port() == 0) {
        return 0;
    }

    // Establish Connection with Server
    if (establish_connection() == 0) {
        return 0;
    }

    char input[MAX_COMMAND_LENGTH];

    while (1) {
        printf("Enter Command: ");
        fgets(input, MAX_COMMAND_LENGTH, stdin);  // Read input from the user
        input[strlen(input) - 1] = '\0';          // Remove the newline character from the input

        if (strcmp(input, "") == 0) {
            continue;
        }

        // Tokenize the Command
        int token_count = 0;
        char **command_tokens = tokenizer(input, &token_count);

        // Verify the Syntax of the Command
        if (verify_command_syntax(command_tokens, token_count) == 0) {
            continue;
        }

        if (strcmp(command_tokens[0], "quitc") == 0) {
            break;
        }

        char *command = (char *)malloc(strlen(command_tokens[0]) * sizeof(char) + 1);
        strcpy(command, command_tokens[0]);
        for (int i = 1; i < token_count; i++) {
            command = (char *)realloc(command, strlen(command) + strlen(command_tokens[i]) + 2);
            strcat(command, " ");
            strcat(command, command_tokens[i]);
        }

        // Send Command to Server
        send(socket_fd, command, strlen(command) + 1, 0);

        printf("Waiting for response from server...\n");

        char response[RESPONSE_CHUNK] = {0};

        int bytes_received;

        if (strcmp(command_tokens[0], "dirlist") == 0) {
            if (strcmp(command_tokens[1], "-a") == 0) {
                while (1) {
                    bytes_received = recv(socket_fd, response, RESPONSE_CHUNK, 0);
                    if (strcmp(response, COMPLETE_MESSAGE) == 0)
                        break;
                    printf("%s", response);
                }
            } else if (strcmp(command_tokens[1], "-t") == 0) {
                while (1) {
                    bytes_received = recv(socket_fd, response, RESPONSE_CHUNK, 0);
                    if (strcmp(response, COMPLETE_MESSAGE) == 0)
                        break;
                    printf("%s", response);
                }
            }
        } else {
            char *fileDataBuffer = (char *)malloc(RESPONSE_CHUNK * sizeof(char));
            int flag = 0;
            size_t bytes_received;
            while ((bytes_received = recv(socket_fd, response, RESPONSE_CHUNK, 0)) > 0) {
                printf("Received %ld bytes\n", bytes_received);

                // Read last 12 bytes and compare with COMPLETE_MESSAGE
                if (bytes_received >= 12) {
                    printf("response1: %s\n", response);
                    char *temp = (char *)malloc(13);
                    memcpy(temp, response + bytes_received - 12, 12);
                    temp[12] = '\0';
                    if (strcmp(temp, COMPLETE_MESSAGE) == 0) {
                        flag = 1;
                        bytes_received -= 12;
                    }
                    free(temp);
                }

                if (strcmp(response, FILE_NOT_FOUND) == 0) {
                    printf("response: %s\n", response);
                    flag = 2;
                    break;
                }

                // Append the received data to the fileDataBuffer for tar file
                fileDataBuffer = (char *)realloc(fileDataBuffer, strlen(fileDataBuffer) + bytes_received);
                strncat(fileDataBuffer, response, bytes_received);

                if (flag == 1) {
                    break;
                }
            }

            if (flag == 1) {
                remove("temp.tar.gz");
                FILE *fp = fopen("temp.tar.gz", "wb");

                if (fp == NULL) {
                    perror("fopen");
                    exit(EXIT_FAILURE);
                }

                fwrite(fileDataBuffer, 1, strlen(fileDataBuffer), fp);
                fclose(fp);
                printf("File received successfully.\n");
            } else if (flag == 2) {
                printf("File not found.\n");
                remove("temp.tar.gz");
            } else {
                remove("temp.tar.gz");
            }
            free(fileDataBuffer);
        }

        int i = 0;

        while (command_tokens[i] != NULL) {
            free(command_tokens[i]);
            i++;
        }

        free(command);
        free(command_tokens);

        // Empty the socket buffer to receive the next response
        memset(response, 0, RESPONSE_CHUNK);
    }

    close_connection();
    return 0;
}