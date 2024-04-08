#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error in socket creation");
        exit(EXIT_FAILURE);
    }

    // Set server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error in connection");
        exit(EXIT_FAILURE);
    }

    // Loop until user enters "quitc"
    while (1) {
        // Wait for user input
        printf("Enter a message (or 'quitc' to exit): ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove newline character from the input
        buffer[strcspn(buffer, "\n")] = '\0';

        // Check if user wants to quit
        if (strcmp(buffer, "quitc") == 0) {
            break;
        }

        // Send message to the server
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
            perror("Error in sending message");
            exit(EXIT_FAILURE);
        }

        // Receive and print the output from the server
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(clientSocket, buffer, BUFFER_SIZE, 0) < 0) {
            perror("Error in receiving message");
            exit(EXIT_FAILURE);
        }
        printf("Server response: %s\n", buffer);
    }

    // Close the socket
    close(clientSocket);

    return 0;
}