#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1" // Change this to the IP address of your server
#define SERVER_PORT 9090
#define BUFFER_SIZE 4096

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read, bytes_written;

    while (1) {
        // Client sends message
        printf("Client: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send message to server
        bytes_written = write(sockfd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("Write to server failed");
            break;
        }

        // Check if the client wants to quit
        if (strncmp(buffer, "quitc", 5) == 0) {
            printf("Client quitting...\n");
            break;
        }

        // Receive message from server
        bytes_read = read(sockfd, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("Read from server failed");
            break;
        }

        // Check if the server wants to quit
        if (strncmp(buffer, "quitc", 5) == 0) {
            printf("Server quitting...\n");
            break;
        }

        // Print received message
        printf("Server: %s", buffer);

        // Check if the server sent "quitc"
        if (strncmp(buffer, "quitc", 5) == 0) {
            printf("Server quitting...\n");
            break;
        }

        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    // Close socket
    close(sockfd);

    return 0;
}
