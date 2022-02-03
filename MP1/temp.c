#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

// socket file descriptor stored globally for graceful closing of socket
void signal_callback_handler(int signum) {
    printf("\nServer shutting down \n");

    exit(signum);
}

int main(int argc, char *argv[]) {
    int PORT = atoi(argv[2]);
    int bytesReceived = 0;
    char *server = argv[1];

    signal(SIGINT, signal_callback_handler);

    while (true) {
        // create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);

        // sockardd_in struct for connecting to server
        sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(PORT);
        serveraddr.sin_addr.s_addr = inet_addr(server);

        // connect to server at address "serveraddr"
        int connectionStatus = connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));

        // send message to server
        std::string message = "Hello from the client";
        send(sockfd, message.c_str(), message.size(), 0);

        // receive message from server
        char buffer[100];
        recv(sockfd, buffer, 100, 0);
        printf("Message Recieved: %s \n", buffer);

        close(sockfd);
    }

    return 0;
}
