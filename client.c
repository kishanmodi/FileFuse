#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_COMMAND_LENGTH 1024

// DEFAULT SYNTAX FOR DEFINED COMMANDS
// 1 dirlist -a -- Return All File Under Home Directory of Server --- Alphabetic Order
// 2 dirlist -t -- Return All File Under Home Directory of Server --- Time Order
// 3 w24fn <filename> -- Return File Name size (bytes), date of creation, file permissions
// 4 w24fz <size1> <size2> -- Return All Files with size between size1 and size2 in tar.gz format
// 5 w24ft <file_extension1> <file_extension2> <file_extension3> -- upto 3 file extensions, min 1 --> return all files with the given extensions in tar.gz format
// 6 w24db <date> -- Return all files created before the given date in tar.gz format
// 7 w24da <date> -- Return all files created after the given date in tar.gz format
// 8 quitc -- Close the connection with the server

// Function to tokenize the command and return the tokens in an array
char *tokenize(char *command) {
    char *command_tokens[5];
    char *copy_of_command = strdup(command);
    char *token = strtok(copy_of_command, " ");
    int tokenCount = 0;
    while (token != NULL) {
        // Format the token
        // Remove trailing space, tab , newline characters
        while (token[strlen(token) - 1] == ' ' || token[strlen(token) - 1] == '\t' || token[strlen(token) - 1] == '\n') {
            token[strlen(token) - 1] = '\0';
        }
        // Remove leading space, tab , newline characters
        while (token[0] == ' ' || token[0] == '\t' || token[0] == '\n') {
            token++;
        }

        // Store the token in the array
        command_tokens[tokenCount] = token;
        token = strtok(NULL, " ");
        tokenCount++;
    }
    command_tokens[tokenCount] = NULL;
    return command_tokens;
}

// Function to verify the syntax of the command
int verify_command_syntax(char *command) {
    // Check if the command is empty
    if (strcmp(command, "") == 0) {
        fprintf(stderr, "Error:  Empty command\n");
        return 0;
    }

    // Tokenize the command and store the tokens in an array
    char *command_tokens = tokenize(command);

    // Count the number of tokens in the command
    int tokenCount = 0;
    while (command_tokens[tokenCount] != NULL) {
        tokenCount++;
    }

    // Check if the command is valid
    if (strcmp(command_tokens[0], "dirlist") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, "Error: Invalid number of arguments for dirlist\n");
            return 0;
        }
        if (strcmp(command_tokens[1], "-a") != 0 && strcmp(command_tokens[1], "-t") != 0) {
            fprintf(stderr, "Error: Invalid argument for dirlist\n");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24fn") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, "Error:  Invalid number of arguments for w24fn\n");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24fz") == 0) {
        if (tokenCount != 3) {
            fprintf(stderr, "Error:  Invalid number of arguments for w24fz\n");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24ft") == 0) {
        if (tokenCount < 3 || tokenCount > 5) {
            fprintf(stderr, "Error:  Invalid number of arguments for w24ft\n");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24db") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, "Error:  Invalid number of arguments for w24db\n");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "w24da") == 0) {
        if (tokenCount != 2) {
            fprintf(stderr, "Error:  Invalid number of arguments for w24da\n");
            return 0;
        }
    } else if (strcmp(command_tokens[0], "quitc") == 0) {
        if (tokenCount != 1) {
            fprintf(stderr, "Error:  Invalid number of arguments for quitc\n");
            return 0;
        }
    } else {
        fprintf(stderr, "Error:  Invalid command\n");
        return 0;  // Command is invalid
    }

    return 1;  // Command is valid
}

int main() {
    char input[MAX_COMMAND_LENGTH];

    while (1) {
        printf("shell24$ ");
        fgets(input, MAX_COMMAND_LENGTH, stdin);  // Read input from the user
        input[strlen(input) - 1] = '\0';          // Remove the newline character from the input

        if (strcmp(input, "") == 0) {  // Handle empty input line, Enter key
            continue;
        }

        if (verify_command_syntax(input) == 0) {  // Verify the syntax of the command
            continue;
        }

        // Tokenize the command and store the tokens in an array
        char *command_tokens = tokenize(input);
    }
    exit(1);
}