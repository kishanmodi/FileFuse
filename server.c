#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080
#define MIRROR_1_PORT 9001
#define MIRROR_2_PORT 10001
#define MAX_CONNECTIONS 5
#define MAX_COMMAND_LENGTH 1000

void connection_handler(int new_socket)
{
    char client_code[1024] = {0};

    while (1) {
        // Read the command from the client
        read(new_socket, client_code, 1024);

        printf("Received command from client: %s\n", client_code);

        // Check if the client wants to quit
        if (strcmp(client_code, "quitc") == 0) {
            break;
        }

        char message[] = "This can be any Resposnse from server\n";
        write(new_socket, message, strlen(message));
    }

    // Close the socket for this client
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

