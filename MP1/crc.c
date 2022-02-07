#include "interface.h"

int connect_to(const char* host, const int port);
Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const int port);

void signal_callback_handler(int signum) {
    printf("\nClient shutting down \n");

    exit(signum);
}

int main(int argc, char** argv) {
    signal(SIGINT, signal_callback_handler);

    // make sure correct number of args passed
    if (argc != 3) {
        fprintf(stderr, "usage: enter host address and port number\n");
        exit(1);
    }

    display_title();

    while (true) {
        // get the file descriptor of the socket associated with this instance of the program
        int sockfd = connect_to(argv[1], atoi(argv[2]));

        // prompt user for command to enter
        char command[MAX_DATA];
        get_command(command, MAX_DATA);

        // convert user's command to uppercase for parsing ease
        touppercase(command, strlen(command) - 1);

        // process the command and get a reply then display the reply to the user
        Reply reply = process_command(sockfd, command);
        display_reply(command, reply);

        // handle join commands correctly
        if (strncmp(command, "JOIN", 4) == 0 && reply.status == SUCCESS) {
            send(sockfd, "1", MAX_DATA, 0);
            printf("Now you are in the chatmode\n");
            process_chatmode(argv[1], reply.port);
        } else {
            send(sockfd, "0", MAX_DATA, 0);
        }

        close(sockfd);
    }

    return 0;
}

/*
  Connect to the server using given host and port information
  1. connect to server
  2. after connection established, we're ready to send or receive message from/to server
  3. return socket filedescriptor for later use by rest of program
 
  @parameter host    host address given by command line argument
  @parameter port    port given by command line argument
  
  @return socket fildescriptor
 */
int connect_to(const char* host, const int port) {
    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // sockardd_in struct for connecting to server
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr(host);

    // connect to server at address "serveraddr"
    int connectionStatus = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    // connectionStatus of 0 indicates success in connecting to desired host on specified port
    if (connectionStatus != 0) {
        printf("connect() call failed with errno %i: %s \n", errno, strerror(errno));
        exit(1);
    }

    return sockfd;
}

std::vector<std::string> parse_response(char* buffer) {
    std::string response = buffer;
    std::vector<std::string> args;

    int semiColonIndex = 0;
    while (semiColonIndex != std::string::npos && response.length() > 1) {
        semiColonIndex = response.find(";");

        std::string arg = response.substr(0, semiColonIndex);
        args.push_back(arg);

        response = response.substr(semiColonIndex + 1, response.length() - semiColonIndex - 1);
    }

    return args;
}

/* 
  Send an input command to the server and return the result
 
  @parameter sockfd   socket file descriptor to commnunicate with the server
  @parameter command  command will be sent to the server
 
  @return    Reply    
 */
Reply process_command(const int sockfd, char* command) {
    // ------------------------------------------------------------
    // GUIDE 3:
    // Then, you should create a variable of Reply structure
    // provided by the interface and initialize it according to
    // the result.
    //
    // For example, if a given command is "JOIN room1"
    // and the server successfully created the chatroom,
    // the server will reply a message including information about
    // success/failure, the number of members and port number.
    // By using this information, you should set the Reply variable.
    // the variable will be set as following:
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // reply.num_member = number;
    // reply.port = port;
    //
    // "number" and "port" variables are just an integer variable
    // and can be initialized using the message fomr the server.
    //
    // For another example, if a given command is "CREATE room1"
    // and the server failed to create the chatroom becuase it
    // already exists, the Reply varible will be set as following:
    //
    // Reply reply;
    // reply.status = FAILURE_ALREADY_EXISTS;
    //
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    //
    // "list" is a string that contains a list of chat rooms such
    // as "r1,r2,r3,"
    // ------------------------------------------------------------
    Reply reply;

    send(sockfd, command, MAX_DATA, 0);
    char buffer[MAX_DATA];
    recv(sockfd, buffer, MAX_DATA, 0);

    std::vector<std::string> args = parse_response(buffer);

    // printf("args: ");
    // for(int i = 0; i < args.size(); i++) {
    //     printf("%s ", args[i].c_str());
    // }
    // printf("\n");

    reply.status = (Status)stoi(args[1]);

    if (args.size() >= 4) {
        reply.status = (Status)stoi(args[3]);
        reply.num_member = (Status)stoi(args[2]);
        reply.port = (Status)stoi(args[1]);
    } else {
        reply.status = (Status)stoi(args[1]);
    }

    return reply;
}

/* 
  Get into the chat mode
  
  @parameter host     host address
  @parameter port     port
 */
void process_chatmode(const char* host, const int port) {
    printf("PROCESSS CHATMODE: \n");
    int room_sockfd = connect_to(host, port);
    printf("PROCESSS CHATMODE: connected to room on port %i \n", port);

    // char message[MAX_DATA];
    // get_message(message, MAX_DATA);

    printf("PROCESSS CHATMODE: Ready to send \n");
    // send(room_sockfd, message, MAX_DATA, 0);

    // auto recv_message = [=]() {
    //     while (true) {
    //         char buffer[MAX_DATA];
    //         recv(room_sockfd, buffer, MAX_DATA, 0);

    //         display_message(buffer);

    //         // close(sockfd);
    //     }
    // };
    // std::thread recv_thread(recv_message);

    while (true) {
        // printf("PROCESSS CHATMODE: Send loop \n");
        char message[MAX_DATA];
        get_message(message, MAX_DATA);

        // printf("Message: %s \n", message);

        send(room_sockfd, message, MAX_DATA, 0);

        // close(room_sockfd);
    }

    // std::thread send_thread(send_message);

    // send_thread.join();
    // recv_thread.join();
}
