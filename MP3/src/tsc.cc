#include <grpc++/grpc++.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "client.h"
#include "sns.grpc.pb.h"
using csce438::ListReply;
using csce438::Message;
using csce438::Reply;
using csce438::Request;
using csce438::SNSService;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

//  creates a Message protobuf
Message MakeMessage(const std::string& username, const std::string& msg) {
    Message m;
    m.set_username(username);
    m.set_msg(msg);
    google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
    timestamp->set_seconds(time(NULL));
    timestamp->set_nanos(0);
    m.set_allocated_timestamp(timestamp);
    return m;
}

// client class to represent this client instance. implements IClient
class Client : public IClient {
   public:
    Client(const std::string& hname, const std::string& uname, const std::string& p) : hostname(hname), username(uname), port(p) {}

   protected:
    virtual int connectTo();
    virtual IReply processCommand(std::string& input);
    virtual void processTimeline();

   private:
    std::string hostname;
    std::string username;
    std::string port;

    // You can have an instance of the client stub
    // as a member variable.
    std::unique_ptr<SNSService::Stub> stub_;
    std::unique_ptr<SNSService::Stub> coordinator_stub;

    IReply Login();
    std::string GetServer();
    IReply List();
    IReply Follow(const std::string& username2);
    IReply UnFollow(const std::string& username2);
    void Timeline(const std::string& username);
};

int main(int argc, char** argv) {
    std::string coordinator_ip = "127.0.0.1";
    std::string coordinator_port = "9090";    
    std::string id = "4";
    int opt = 0;

    while ((opt = getopt(argc, argv, "i:")) != -1) {
        switch (opt) {
            case 'i':
                id = optarg;
                break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    // create a client object for this instance. it has a stub that will get created in run_client()
    Client myc(coordinator_ip, id, coordinator_port);

    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo() {
    // create stub to talk to coordinator
    std::string coordinator_connection_info = hostname + ":" + port;

    coordinator_stub = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
        grpc::CreateChannel(
            coordinator_connection_info, grpc::InsecureChannelCredentials()
        )
    ));

    // get address of server
    std::string login_info = GetServer();

    // connect to server
    stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
        grpc::CreateChannel(
            login_info, grpc::InsecureChannelCredentials()
        )
    ));

    IReply ire = Login();
    if (!ire.grpc_status.ok()) {
        return -1;
    }
    return 1;
}

IReply Client::processCommand(std::string& input) {
    // ------------------------------------------------------------
    // GUIDE 1:
    // In this function, you are supposed to parse the given input
    // command and create your own message so that you call an
    // appropriate service method. The input command will be one
    // of the followings:
    //
    // FOLLOW <username>
    // UNFOLLOW <username>
    // LIST
    // TIMELINE
    //
    // - JOIN/LEAVE and "<username>" are separated by one space.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // GUIDE 2:
    // Then, you should create a variable of IReply structure
    // provided by the client.h and initialize it according to
    // the result. Finally you can finish this function by returning
    // the IReply.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Join" service method for JOIN command,
    // IReply can be set as follow:
    //
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Join(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //
    //      return ire;
    //
    // IMPORTANT:
    // For the command "LIST", you should set both "all_users" and
    // "following_users" member variable of IReply.
    // ------------------------------------------------------------
    IReply ire;
    std::size_t index = input.find_first_of(" ");
    if (index != std::string::npos) {
        std::string cmd = input.substr(0, index);

        /*
        if (input.length() == index + 1) {
            std::cout << "Invalid Input -- No Arguments Given\n";
        }
        */

        std::string argument = input.substr(index + 1, (input.length() - index));

        if (cmd == "FOLLOW") {
            return Follow(argument);
        } else if (cmd == "UNFOLLOW") {
            return UnFollow(argument);
        }
    } else {
        if (input == "LIST") {
            return List();
        } else if (input == "TIMELINE") {
            ire.comm_status = SUCCESS;
            return ire;
        }
    }

    ire.comm_status = FAILURE_INVALID;
    return ire;
}

void Client::processTimeline() {
    Timeline(username);
    // ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
    // ------------------------------------------------------------
}

IReply Client::List() {
    //Data being sent to the server
    Request request;
    request.set_username(username);

    //Container for the data from the server
    ListReply list_reply;

    //Context for the client
    ClientContext context;

    Status status = stub_->List(&context, request, &list_reply);
    IReply ire;
    ire.grpc_status = status;
    //Loop through list_reply.all_users and list_reply.following_users
    //Print out the name of each room
    if (status.ok()) {
        ire.comm_status = SUCCESS;
        std::string all_users;
        std::string following_users;
        for (std::string s : list_reply.all_users()) {
            ire.all_users.push_back(s);
        }
        for (std::string s : list_reply.followers()) {
            ire.followers.push_back(s);
        }
    }
    return ire;
}

IReply Client::Follow(const std::string& username2) {
    Request request;
    request.set_username(username);
    request.add_arguments(username2);

    Reply reply;
    ClientContext context;

    Status status = stub_->Follow(&context, request, &reply);
    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "unkown user name") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "unknown follower username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "you have already joined") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else if (reply.msg() == "Follow Successful") {
        ire.comm_status = SUCCESS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
    }
    return ire;
}

IReply Client::UnFollow(const std::string& username2) {
    Request request;

    request.set_username(username);
    request.add_arguments(username2);

    Reply reply;

    ClientContext context;

    Status status = stub_->UnFollow(&context, request, &reply);
    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "unknown follower username") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "you are not follower") {
        ire.comm_status = FAILURE_INVALID_USERNAME;
    } else if (reply.msg() == "UnFollow Successful") {
        ire.comm_status = SUCCESS;
    } else {
        ire.comm_status = FAILURE_UNKNOWN;
    }

    return ire;
}

IReply Client::Login() {
    Request request;
    request.set_username(username);
    Reply reply;
    ClientContext context;

    Status status = stub_->Login(&context, request, &reply);

    IReply ire;
    ire.grpc_status = status;
    if (reply.msg() == "you have already joined") {
        ire.comm_status = FAILURE_ALREADY_EXISTS;
    } else {
        ire.comm_status = SUCCESS;
    }
    return ire;
}

std::string Client::GetServer() {
    Request request;
    request.set_username(username);
    Reply reply;
    ClientContext context;

    Status status = coordinator_stub->FindServerAddr(&context, request, &reply);

    return reply.msg();
}

void Client::Timeline(const std::string& username) {
    ClientContext context;

    std::shared_ptr<ClientReaderWriter<Message, Message>> stream(
        stub_->Timeline(&context));

    //Thread used to read chat messages and send them to the server
    std::thread writer([username, stream]() {
        std::string input = "Set Stream";
        Message m = MakeMessage(username, input);
        stream->Write(m);
        while (1) {
            input = getPostMessage();
            m = MakeMessage(username, input);
            stream->Write(m);
        }
        stream->WritesDone();
    });

    std::thread reader([username, stream]() {
        Message m;
        while (stream->Read(&m)) {
            google::protobuf::Timestamp temptime = m.timestamp();
            std::time_t time = temptime.seconds();
            displayPostMessage(m.username(), m.msg(), time);
        }
    });

    //Wait for the threads to finish
    writer.join();
    reader.join();
}
