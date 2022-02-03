#include "interface.h"

// socket file descriptor stored globally for graceful closing of socket
int sockfd;

enum command_type { CREATE,
                    JOIN,
                    DELETE };

class Chatroom {
   private:
    std::string name;
    int port, num_members, room_sockfd;
    sockaddr_in room_addr;

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
};

class Server {
    std::vector<Chatroom> rooms;
    int numRooms, portAddend;

   public:
    Server() {
        numRooms = 0;
        portAddend = 0;
    }

    void addRoom(std::string name) {
        rooms.push_back(Chatroom(name, 1024 + portAddend++));
        numRooms++;
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
};

void signal_callback_handler(int signum) {
    close(sockfd);
    printf("\nServer shutting down \n");

    exit(signum);
}

struct clientMessage {
    // struct joinResponse {
    //     int port, numMembers;
    // };

    std::string arg;
    command_type type;
    // joinResponse join;
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

    // bind our socket to port
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

        clientMessage command = parse_message(buffer);

        std::string response = "";
        if (command.valid) {
            std::string room = command.arg;
            switch (command.type) {
                case CREATE:
                    response += "CREATE;";
                    if (server.roomExists(room)) {
                        response += std::to_string(FAILURE_ALREADY_EXISTS) + ";";
                    } else {
                        server.addRoom(room);
                        response += "0;";
                    }
                    break;
                case JOIN:
                    response += "JOIN;";
                    if (server.roomExists(room)) {
                        response += std::to_string(server.get_port(room)) + ";";
                        response += std::to_string(server.num_members(room)) + ";";
                        response += "0;";
                    } else {
                        response += "2;";
                    }
                    break;
                case DELETE:
                    response += "DELETE;";
                    if (server.roomExists(room)) {
                        // 1. send a warning to all connected members of the chatroom
                        // 2. terminate connections
                        // 3. close socket for that room
                        server.removeRoom(room);
                        response += "0;";
                    } else {
                        response += "2;";
                    }
                    break;
                default:
                    response += "4;";
            }
        } else {
            response += "3;";
        }

        // send response
        send(connection, response.c_str(), response.size(), 0);
        std::cout << "TEST: " << response << std::endl;

        // Close the connection
        close(connection);
    }

    return 0;
}
