#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <iostream>
// #include <sys/socket.h>

#include "interface.h"

// socket file descriptor stored globally for graceful closing of socket
int sockfd;

class Chatroom {
    // members
    std::string name;
    int port, num_members;

    Chatroom(std::string _name, int _port) {
        name = _name;
        num_members = 0;
        port = _port;
    }

    std::string name() {
        return name;
    }
}

class Server {
    Vector<Chatroom> rooms;
    int numRooms;

    Server() {
        numRooms = 0;
    }

    void addRoom(std::string name, int port) {
        rooms.push_back(Chatroom(name, port));
        numRooms++;
    }

    void removeRoom(std::string name) {
        for(int i = 0; i < numRooms; i++) {
            if(rooms[i].name() == name) {
                rooms.erase(i);
                break;
            }
        }

        numRooms--;
    }

    bool roomAlreadyExists(std::string name) {
        for(int i = 0; i < numRooms; i++) {
            if(rooms[i].name() == name) {
                return true;
            }
        }
    }
}

void
signal_callback_handler(int signum) {
    close(sockfd);
    printf("\nServer shutting down \n");

    exit(signum);
}

int main(int argc, char** argv) {
    Server server;

    // register signal handler for ctrl+c server abortion
    signal(SIGINT, signal_callback_handler);

    // grab port number arg
    int PORT_NUMBER = atoi(argv[1]);

    // consider adding an error handler for each step along the way to make code more robust
    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // printf("Socket with file descriptor %i assigned to server \n", sockfd);

    // create a sockaddr_in struct to act as the second arg for bind(). we use sockaddr_in because we
    // are using IP
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // listen from any address
    addr.sin_port = htons(PORT_NUMBER);

    // bind our socket to port 9999
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    // printf("Socket with file descriptor %i bound to port %i \n", sockfd, PORT_NUMBER);

    // listen for incoming connections, allowing a max of 10 in the queue before refusing additional requests
    listen(sockfd, 10);

    printf("Server running on port: %i \n", PORT_NUMBER);
    printf("Exit with ctrl+c \n");

    while (true) {
        // Grab a connection from the queue
        int addrlen = sizeof(addr);  // this will be the length of addr
        int connection = accept(sockfd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);

        // Read from the connection
        char buffer[100];
        recv(connection, buffer, 100, 0);
        printf("Message Recieved: %s \n", buffer);

        if (CREATE) {
            if (server.roomAlreadyExists(PASSED ROOM NAME)) {
                // SEND MESSAGE THAT THE ROOM ALREADEY EXISTS
            } else {
                server.addRoom(PASSED Room Name);
            }
        } else if (DELETE) {
        } else if (JOIN) {
        } else {
            handle error
        }

        // // Send a message to the connection
        // std::string response = "Good talking to you\n";
        // send(connection, response.c_str(), response.size(), 0);

        // Close the connection
        close(connection);
    }

    return 0;
}
