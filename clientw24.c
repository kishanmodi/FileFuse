// DEFAULT SYNTAX FOR DEFINED COMMANDS
// 1 dirlist -a -- Return All File Under Home Directory of Server --- Alphabetic Order
// 2 dirlist -t -- Return All File Under Home Directory of Server --- Time Order
// 3 w24fn <filename> -- Return File Name size (bytes), date of creation, file permissions
// 4 w24fz <size1> <size2> -- Return All Files with size between size1 and size2 in tar.gz format
// 5 w24ft <file_extension1> <file_extension2> <file_extension3> -- upto 3 file extensions, min 1 --> return all files
// with the given extensions in tar.gz format
// 6 w24fdb <date> -- Return all files created before the given date in
// tar.gz format
// 7 w24fda <date> -- Return all files created after the given date in tar.gz format
// 8 quitc -- Close the connection with the server

#include <arpa/inet.h>   // For inet_addr
#include <netinet/in.h>  // For sockaddr_in
#include <regex.h>       // For regex
#include <stdio.h>       // For standard I/O
#include <stdlib.h>      // For standard library
#include <string.h>      // For string functions
#include <sys/socket.h>  // For socket functions
#include <sys/types.h>   // For system types
#include <unistd.h>      // For close function

// Constants
#define MAX_COMMAND_LENGTH 1024             // Maximum length of the command
#define MAX_TOKEN_COUNT    10               // Maximum number of tokens in the command
#define RESPONSE_CHUNK     1024             // Maximum length of the response
#define IP_LENGTH          INET_ADDRSTRLEN  // Length of IP Address

// Error Messages
#define INVALID_IP_ERROR        "Error: Invalid IP Address\n"
#define INVALID_PORT_ERROR      "Error: Invalid Port Number\n"
#define ALLOCATION_ERROR        "Error allocating memory\n"
#define INVALID_ARGUMENTS_ERROR "Error: Invalid number of arguments for %s\n"
#define INVALID_DATE_ERROR      "Error: Invalid Date Format\n"
#define INVALID_COMMAND_ERROR   "Error: Invalid command\n"
#define COMPLETE_MESSAGE        "@#COMPLETE#@"
#define FILE_NOT_FOUND          "File not found"
#define CONNECTION_CLOSED       "Connection closed"

// Global Variables
int socket_fd, port;  // Socket File Descriptor and Port Number
char ip[IP_LENGTH];   // IP Address

int establish_connection();
void close_connection();
int validate_ip_port();
char **tokenizer(char *command, int *token_count);
int verify_command_syntax(char **command_tokens, int tokenCount);
int verifyDateFormat(char *date_r);

// FUNCTION DEFINITIONS

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
void close_connection() { close(socket_fd); }

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

// Function to verify the date format
int verifyDateFormat(char *date_r) {
    regex_t regex;  // Structure to store compiled regex
    int regexCheck;
    char *date = strdup(date_r);                                                                                   // Copy the date to a new string
    regexCheck = regcomp(&regex, "^[0-9]{4}-((0[1-9])|(1[0-2]))-((0[1-9])|([1-2][0-9])|(3[01]))$", REG_EXTENDED);  //    Compile the regex
    if (regexCheck != 0) {
        printf("Error compiling regex\n");
        return 0;
    }
    regexCheck = regexec(&regex, date, 0, NULL, 0);
    if (regexCheck == 0) {
        regfree(&regex);
        return 1;  // Date is valid
    } else if (regexCheck == REG_NOMATCH) {
        regfree(&regex);
        return 0;  // Date is invalid
    } else {
        regfree(&regex);
        return 0;  // Error in regex
    }
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
    } else if (strcmp(command_tokens[0], "w24fdb") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24fdb");
            return 0;
        }

        if (verifyDateFormat(command_tokens[1]) == 0) {
            fprintf(stderr, INVALID_DATE_ERROR);
            return 0;
        }

    } else if (strcmp(command_tokens[0], "w24fda") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "w24fda");
            return 0;
        }

        if (verifyDateFormat(command_tokens[1]) == 0) {
            fprintf(stderr, INVALID_DATE_ERROR);
            return 0;
        }
    } else if (strcmp(command_tokens[0], "who") == 0) {
        if (tokenCount != 1) {
            fprintf(stderr, INVALID_ARGUMENTS_ERROR, "who");
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
    // Validate Command Line Arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP Address> <Port Number>\n", argv[0]);
        return 0;
    }

    // Store IP Address and Port Number
    strcpy(ip, argv[1]);
    port = atoi(argv[2]);

    // Validate IP Address and Port Number
    if (validate_ip_port() == 0) {
        return 0;
    }

    // Establish Connection with Server
    if (establish_connection() == 0) {
        return 0;
    }

    char input[MAX_COMMAND_LENGTH];  // Buffer to store the input command

    while (1) {
        printf("$");                              // Prompt the user to enter a command
        fgets(input, MAX_COMMAND_LENGTH, stdin);  // Read input from the user
        input[strlen(input) - 1] = '\0';          // Remove the newline character from the input

        if (strcmp(input, "") == 0 || strlen(input) == 0) {
            continue;  // Skip if the input is empty
        }

        // Tokenize the Command
        int token_count = 0;
        char **command_tokens = tokenizer(input, &token_count);

        // Verify the Syntax of the Command
        if (verify_command_syntax(command_tokens, token_count) == 0) {
            continue;
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

        if (strcmp(command_tokens[0], "dirlist") == 0 || strcmp(command_tokens[0], "who") == 0) {
            if (strcmp(command_tokens[0], "who") == 0) {
                while (1) {
                    bytes_received = recv(socket_fd, response, RESPONSE_CHUNK, 0);
                    response[bytes_received] = '\0';
                    if (strstr(response, COMPLETE_MESSAGE) != NULL) {
                        response[bytes_received - 12] = '\0';
                        printf("%s", response);
                        break;
                    }
                }
            } else if (strcmp(command_tokens[1], "-a") == 0) {
                while (1) {
                    bytes_received = recv(socket_fd, response, RESPONSE_CHUNK, 0);
                    if (strstr(response, COMPLETE_MESSAGE) != NULL) break;
                    printf("%s", response);
                }
            } else if (strcmp(command_tokens[1], "-t") == 0) {
                while (1) {
                    bytes_received = recv(socket_fd, response, RESPONSE_CHUNK, 0);
                    if (strstr(response, COMPLETE_MESSAGE) != NULL) break;
                    printf("%s", response);
                }
            }
        } else if (strcmp(command_tokens[0], "w24fn") == 0) {
            size_t bytes_received;
            int flag = 0;
            while ((bytes_received = recv(socket_fd, response, RESPONSE_CHUNK - 1, 0)) > 0) {
                response[bytes_received] = '\0';
                // Read last 12 bytes and compare with COMPLETE_MESSAGE
                if (bytes_received >= 12) {
                    char *temp = (char *)malloc(13);
                    memcpy(temp, response + bytes_received - 12, 12);
                    temp[12] = '\0';
                    if (strcmp(temp, COMPLETE_MESSAGE) == 0) {
                        flag = 1;
                        bytes_received -= 12;
                        response[bytes_received] = '\0';
                    }
                    free(temp);
                }
                if (strcmp(response, FILE_NOT_FOUND) == 0) {
                    memset(response, 0, RESPONSE_CHUNK);
                    printf("File not found.\n");
                    flag = 2;
                    break;
                }
                printf("%s", response);
                if (flag == 1) {
                    memset(response, 0, RESPONSE_CHUNK);
                    break;
                }
            }
        } else if (strstr(command_tokens[0], "quitc")) {
            printf("Connection closed.\n");
            break;
        } else {
            int flag = 0;
            FILE *fp = fopen("local_temp.tar.gz", "wb");

            size_t bytes_received;
            while ((bytes_received = recv(socket_fd, response, RESPONSE_CHUNK - 1, 0)) > 0) {
                response[bytes_received] = '\0';
                // Read last 12 bytes and compare with COMPLETE_MESSAGE
                if (bytes_received >= 12) {
                    char *temp = (char *)malloc(13);
                    memcpy(temp, response + bytes_received - 12, 12);
                    temp[12] = '\0';
                    if (strcmp(temp, COMPLETE_MESSAGE) == 0) {
                        flag = 1;
                        bytes_received -= 12;
                        response[bytes_received] = '\0';
                    }
                    free(temp);
                }

                if (strcmp(response, FILE_NOT_FOUND) == 0) {
                    memset(response, 0, RESPONSE_CHUNK);
                    printf("File not found.\n");
                    flag = 2;
                    break;
                }
                fwrite(response, 1, bytes_received, fp);
                if (flag == 1) {
                    memset(response, 0, RESPONSE_CHUNK);
                    break;
                }
            }

            fclose(fp);

            if (flag == 1) {
                printf("File received successfully.\n");
                rename("local_temp.tar.gz", "temp.tar.gz");
                remove("local_temp.tar.gz");
            }
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