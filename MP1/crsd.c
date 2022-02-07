#include "interface.h"

// socket file descriptor stored globally for graceful closing of socket
int sockfd;

class Chatroom {
   private:
    std::string name;
    int port, num_members, room_sockfd;
    sockaddr_in room_addr;
    std::vector<int> members;

   public:
    Chatroom(std::string _name, int _port) {
        name = _name;
        num_members = 0;
        port = _port;

        room_sockfd = socket(AF_INET, SOCK_STREAM, 0);

        room_addr.sin_family = AF_INET;
        room_addr.sin_addr.s_addr = INADDR_ANY;  // listen from any address
        room_addr.sin_port = htons(port);

        bind(room_sockfd, (struct sockaddr*)&room_addr, sizeof(room_addr));

        listen(room_sockfd, 10);

        printf("Room %s created on port %i \n", name.c_str(), port);
    }

    std::string get_name() {
        return name;
    }

    int get_port() {
        return port;
    }

    int get_num_members() {
        return num_members;
    }

    std::vector<int> get_members() {
        return members;
    }

    int addUser() {
        // Grab a connection from the queue
        int addrlen = sizeof(room_addr);

        // "connection" is a file descriptor of the new socket created to recieve message from this user
        int connection = accept(room_sockfd, (struct sockaddr*)&room_addr, (socklen_t*)&addrlen);

        members.push_back(connection);
        num_members++;

        printf("User %i added to room %s,%i ; numMembers: %i, %i \n", connection, name.c_str(), port, num_members, int(members.size()));
        return connection;
    }
};

void receive_messages(std::vector<int> members, int userSession_fd, fd_set read_fds) {
    printf("RECIEVE MESSAGES: \n");

    // check which members have sent data
    for (int i = 0; i < members.size(); i++) {
        if (FD_ISSET(members[i], &read_fds)) {
            // recieve message from this member
            char message[MAX_DATA];
            int recvStatus = recv(userSession_fd, message, MAX_DATA, 0);
            if (recvStatus == -1) {
                printf("recv() call failed with errno %i: %s \n", errno, strerror(errno));
                exit(1);
            }
            printf("RECV:%s \n", message);

            // now send this message back to all members that aren't the sender
            for (int i = 0; i < members.size(); i++) {
                if (members[i] != userSession_fd) {
                    send(members[i], message, MAX_DATA, 0);
                }
            }
        }
    }
}

fd_set build_read_list(std::vector<int> members) {
    printf("READ LIST: num members is %i\n", int(members.size()));
    printf("READ LIST: ");
    fd_set read_fds;
    FD_ZERO(&read_fds);

    for (int i = 0; i < members.size(); i++) {
        FD_SET(members[i], &read_fds);
        printf("%i ", members[i]);
    }

    printf("\n");

    return read_fds;
}

// void receive_messages(int userSession_fd, std::vector<int> members) {
//     printf("RECIEVE MESSAGES: \n");

//     // while (true) {
//     char message[MAX_DATA];
//     int recvStatus = recv(userSession_fd, message, MAX_DATA, 0);
//     if (recvStatus == -1) {
//         printf("recv() call failed with errno %i: %s \n", errno, strerror(errno));
//         exit(1);
//     }
//     printf("RECV:%s \n", message);

//     for (int i = 0; i < members.size(); i++) {
//         if ((members)[i] != userSession_fd) {
//             send((members)[i], message, MAX_DATA, 0);
//         }
//     }
//     // }
// }

class Server {
    std::vector<Chatroom> rooms;
    int numRooms, portAddend;

   public:
    Server() {
        numRooms = 0;
        portAddend = 0;
    }

    void addRoom(std::string name) {
        int newPort = rand() % 49150 + 1025;

        for (int i = 0; i < rooms.size(); i++) {
            while (newPort == rooms[i].get_port()) {
                newPort = rand() % 49150 + 1025;
            }
        }

        rooms.push_back(Chatroom(name, newPort));
        numRooms++;

        printf("Room %s added\n", name.c_str());
    }

    void removeRoom(std::string name) {
        for (int i = 0; i < numRooms; i++) {
            if (rooms[i].get_name() == name) {
                rooms.erase(rooms.begin() + i);
                break;
            }
        }

        numRooms--;
    }

    bool roomExists(std::string name) {
        for (int i = 0; i < numRooms; i++) {
            if (rooms[i].get_name() == name) {
                return true;
            }
        }

        return false;
    }

    int get_numRooms() {
        return numRooms;
    }

    int get_port(std::string name) {
        for (int i = 0; i < numRooms; i++) {
            if (rooms[i].get_name() == name) {
                return rooms[i].get_port();
            }
        }

        return -1;
    }

    int num_members(std::string name) {
        for (int i = 0; i < numRooms; i++) {
            if (rooms[i].get_name() == name) {
                return rooms[i].get_num_members();
            }
        }

        return -1;
    }

    void addUser(std::string name) {
        for (int i = 0; i < numRooms; i++) {
            if (rooms[i].get_name() == name) {
                rooms[i].addUser();
            }
        }
    }

    Chatroom getRoom(std::string name) {
        for (int i = 0; i < numRooms; i++) {
            if (rooms[i].get_name() == name) {
                printf("Chatroom with room name %s returned \n", rooms[i].get_name().c_str());
                return rooms[i];
            }
        }

        printf("Chatroom with room name %s not found", name.c_str());
    }
};

void signal_callback_handler(int signum) {
    close(sockfd);
    printf("\nServer shutting down \n");

    exit(signum);
}

struct clientMessage {
    std::string arg;
    command_type type;
    bool valid = true;
};

clientMessage parse_message(char* buffer) {
    clientMessage client_message;

    std::string command_string = buffer;
    std::string command_string_type;
    int spaceIndex = command_string.find(" ");

    if (spaceIndex == std::string::npos) {
        client_message.valid = false;
    } else {
        command_string_type = command_string.substr(0, spaceIndex);

        if (command_string_type == "CREATE") {
            client_message.type = CREATE;
        } else if (command_string_type == "JOIN") {
            client_message.type = JOIN;
        } else if (command_string_type == "DELETE") {
            client_message.type = DELETE;
        } else {
            client_message.valid = false;
        }

        client_message.arg = command_string.substr(spaceIndex + 1, command_string.size() - spaceIndex - 1);
    }

    return client_message;
}

int main(int argc, char** argv) {
    srand(time(0));

    Server server;

    // register signal handler for ctrl+c server abortion
    signal(SIGINT, signal_callback_handler);

    // grab port number arg
    int PORT_NUMBER = atoi(argv[1]);

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // create a sockaddr_in struct to act as the second arg for bind(). we use sockaddr_in because we
    // are using IP
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // listen from any address
    addr.sin_port = htons(PORT_NUMBER);

    // bind our socket to port
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    // listen for incoming connections, allowing a max of 10 in the queue before refusing additional requests
    listen(sockfd, 10);

    printf("Server running on port: %i \n", PORT_NUMBER);
    printf("Exit with ctrl+c \n");

    // for selecting room-specific sockets to read from
    fd_set read_fds;
    timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 100;

    while (true) {
        // Grab a connection from the queue
        int addrlen = sizeof(addr);  // this will be the length of addr
        int connection = accept(sockfd, (struct sockaddr*)&addr, (socklen_t*)&addrlen);

        // Read from the connection
        char buffer[MAX_DATA];
        recv(connection, buffer, MAX_DATA, 0);

        clientMessage command = parse_message(buffer);

        std::string response = "";
        std::string room = command.arg;
        if (command.valid) {
            switch (command.type) {
                case CREATE:
                    response += "CREATE;";
                    if (server.roomExists(room)) {
                        response += std::to_string(FAILURE_ALREADY_EXISTS) + ";";
                    } else {
                        server.addRoom(room);
                        response += std::to_string(SUCCESS) + ";";
                    }
                    break;
                case JOIN:
                    response += "JOIN;";
                    if (server.roomExists(room)) {
                        response += std::to_string(server.get_port(room)) + ";";
                        response += std::to_string(server.num_members(room) + 1) + ";";
                        response += std::to_string(SUCCESS) + ";";
                    } else {
                        response += std::to_string(FAILURE_NOT_EXISTS) + ";";
                    }
                    break;
                case DELETE:
                    response += "DELETE;";
                    if (server.roomExists(room)) {
                        // 1. send a warning to all connected members of the chatroom
                        // 2. terminate connections
                        // 3. close socket for that room
                        server.removeRoom(room);
                        response += std::to_string(SUCCESS) + ";";
                    } else {
                        response += std::to_string(FAILURE_NOT_EXISTS) + ";";
                    }
                    break;
                default:
                    response += std::to_string(FAILURE_UNKNOWN) + ";";
            }
        } else {
            response += std::to_string(FAILURE_INVALID) + ";";
        }

        // send response
        send(connection, response.c_str(), response.size(), 0);

        // receive a second message to see if intiating chatmode
        char inChatMode[MAX_DATA];
        recv(connection, inChatMode, MAX_DATA, 0);
        printf("CHAT MODE: %i\n", atoi(inChatMode));

        if (atoi(inChatMode)) {
            Chatroom chatroom = server.getRoom(room);
            int userSession_fd = chatroom.addUser();  // we add a user and therefore its fd to "members"

            // fd_set containing all elements in "members"
            printf("NUM ROOMS: %i \n", server.get_numRooms());
            read_fds = build_read_list(chatroom.get_members());

            // int ready_fds = select(FD_SETSIZE, &read_fds, NULL, NULL, &timeout);
            // printf("SELECTION OCCURED:  \n");
            // if (ready_fds == -1) {
            //     printf("select() call failed with errno %i: %s \n", errno, strerror(errno));
            //     exit(1);
            // } else if (ready_fds == -1) {
            //     printf("select() call returned 0. Nothing to read");
            // } else {
            //     // recieve incoming chats and send them back to chat members
            //     printf("%i fds ready for I/O \n", ready_fds);
            //     receive_messages(chatroom.get_members(), userSession_fd, read_fds);
            // }

            // printf("USER SESSION: %i\n", userSession_fd);

            // std::thread userSession(receive_messages, userSession_fd, server.getRoom(room).get_members());
        }

        // Close the connection
        close(connection);
    }

    return 0;
}
