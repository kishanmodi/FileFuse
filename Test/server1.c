#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DST_PORT 10000
#define SERVER_PORT 9090
#define BUFFER_SIZE 4096

void forward_port(int clientfd, struct sockaddr_in client_addr, char* dst_ip, int dst_port) {
    printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Set up the destination address
    struct sockaddr_in dst_addr;
    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = inet_addr(dst_ip);
    dst_addr.sin_port = htons(dst_port);

    // Create a socket for the destination server
    int dst_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (dst_sockfd == -1) {
        perror("Destination socket creation failed");
        close(clientfd);
        exit(EXIT_FAILURE);
    }

    // Connect to the destination
    if (connect(dst_sockfd, (struct sockaddr*)&dst_addr, sizeof(dst_addr)) == -1) {
        perror("Connect to destination failed");
        close(clientfd);
        close(dst_sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to destination %s:%d\n", dst_ip, dst_port);

    printf("Port forwarding from port %d to port %d\n", ntohs(client_addr.sin_port), dst_port);

    if(fork() == 0) 
    {
        // Forward data client to destination infinitely
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read, bytes_written;
        while(1)
        {
            while ((bytes_read = read(clientfd, buffer, sizeof(buffer))) > 0) {
                bytes_written = write(dst_sockfd, buffer, bytes_read);
                if (bytes_written == -1) {
                    perror("Write to destination failed");
                    break;
                }
            }
            //breaks on "quitc"
            if(strncmp(buffer, "quitc", 5) == 0)
            {
                break;
            }
        }
        if (bytes_read == -1) {
            perror("Read from client failed");
        }

        // Close the connections
        close(dst_sockfd);
        close(clientfd);
        printf("Connection closed\n");
    }
    else
    {
        // Forward data destination to client infinitely
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read, bytes_written;
        while(1)
        {
            while ((bytes_read = read(dst_sockfd, buffer, sizeof(buffer))) > 0) {
                bytes_written = write(clientfd, buffer, bytes_read);
                if (bytes_written == -1) {
                    perror("Write to destination failed");
                    break;
                }
            }
            //breaks on "quitc"
            if(strncmp(buffer, "quitc", 5) == 0)
            {
                break;
            }
        }
        if (bytes_read == -1) {
            perror("Read from client failed");
        }

        // Close the connections
        close(dst_sockfd);
        close(clientfd);
        printf("Connection closed\n");
    }
}


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

        strcat(buffer, " (Server1)\n");
        // Server sends message
        printf("Server: ");
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
    address.sin_port = htons(SERVER_PORT);

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

    printf("%d\n", SERVER_PORT);

    // Changing directory to ~
    if (chdir("/home/meet") != 0)
    {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    int i = 1;

    while (1)
    {
        // Accepting incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        if(i/2 == 1)
        {
            i++;
            if(fork() == 0)
            {
                crequest(new_socket);
            }
        }
        else
        {
            i++;
            if (fork() == 0)
            {
                forward_port(new_socket, address, "0.0.0.0", DST_PORT);
            }
        }
    }

    return 0;
}