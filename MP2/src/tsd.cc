#include "helpers.h"

struct post {
    std::string user, content, post_time_string;
    time_t post_time;
};

class User {
   private:
    std::string name;
    std::vector<User> following;
    std::vector<struct post> timeline;

   public:
    User(std::string _name) {
        name = _name;
    }

    std::string get_name() {
        return name;
    }
};

class ServerInstance {
    std::vector<User> users;
    int numUsers;

   public:
    ServerInstance() {
        numUsers = 0;
    }

    int addUser(std::string name) {
        listUsers();

        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == name) {
                return 0;
            }
        }

        users.push_back(User(name));

        numUsers++;

        return 1;
    }

    void listUsers() {
        printf("\nUsers Listed: \n");

        for (int i = 0; i < numUsers; i++) {
            printf("%s \n", users[i].get_name().c_str());
        }

        printf("\n");
    }
};

// Global instance of our server. Stores the data for this server, essentially a local DB
ServerInstance serverInstance;

class SNSServiceImpl final : public SNSService::Service {
    Status List(ServerContext* context, const Request* request, Reply* reply) override {
        // ------------------------------------------------------------
        // In this function, you are to write code that handles
        // LIST request from the user. Ensure that both the fields
        // all_users & following_users are populated
        // ------------------------------------------------------------
        return Status::OK;
    }

    Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
        // ------------------------------------------------------------
        // In this function, you are to write code that handles
        // request from a user to follow one of the existing
        // users
        // ------------------------------------------------------------
        return Status::OK;
    }

    Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
        // ------------------------------------------------------------
        // In this function, you are to write code that handles
        // request from a user to unfollow one of his/her existing
        // followers
        // ------------------------------------------------------------
        return Status::OK;
    }

    Status Login(ServerContext* context, const Request* request, Reply* reply) override {
        // get username of request
        std::string user = request->username();

        // check if user already exists. if not, add them
        if (!serverInstance.addUser(user)) {
            return grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "There is already a user with username: " + user);
        }

        return Status::OK;
    }

    Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
        // ------------------------------------------------------------
        // In this function, you are to write code that handles
        // receiving a message/post from a user, recording it in a file
        // and then making it available on his/her follower's streams
        // ------------------------------------------------------------
        return Status::OK;
    }
};

void RunServer(std::string port_no) {
    // address of the server
    std::string server_address = "127.0.0.1:" + port_no;

    // implementation of the service
    SNSServiceImpl service;

    // an object that's gonna be passed to our server to get it built
    ServerBuilder builder;

    // Listen on the given address without any authentication mechanism
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    // This says that "service" is the code that the server will use to talk to the client
    builder.RegisterService(&service);

    // Assembling the server
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server running on localhost, listening on port: " << port_no << std::endl;

    server->Wait();
}

int main(int argc, char** argv) {
    std::string port = "3010";
    int opt = 0;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = optarg;
                break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    RunServer(port);

    return 0;
}
