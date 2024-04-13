#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DST_PORT 10000
#define SERVER_PORT 9090
#define BUFFER_SIZE 4096


void crequest(int new_socket) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read, bytes_written;

    while (1) {
        // Receive message from client
        bytes_read = read(new_socket, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            perror("Read from client failed");
            break;
        }

        // Check if the client wants to quit
        if (strncmp(buffer, "quitc", 5) == 0) {
            printf("Client quitting...\n");
            break;
        }

        // Print received message
        printf("Client: %s", buffer);

        // Clear the buffer

        // Server sends message

        strcat(buffer, " (Server2)\n");
        // Send message to client
        bytes_written = write(new_socket, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("Write to client failed");
            break;
        }

        // Check if the server wants to quit
        if (strncmp(buffer, "quitc", 5) == 0) {
            printf("Server quitting...\n");
            break;
        }

        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    // Close the socket
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
    address.sin_port = htons(DST_PORT);

    // Binding socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for connections
    if (listen(server_fd, 5) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on: \n ");

    system("hostname -i");

    printf("%d\n", DST_PORT);

    // Changing directory to ~
    if (chdir("/home/meet") != 0)
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
            crequest(new_socket);
        }
    }

    return 0;
}