#include "helpers.h"

class Client : public IClient {
   public:
    Client(const std::string& hname, const std::string& uname, const std::string& p) : hostname(hname), username(uname), port(p) {}

   protected:
    virtual int connectTo();
    virtual IReply processCommand(std::string& input);
    virtual void processTimeline();

   private:
    std::string hostname, username, port;
    std::unique_ptr<SNSService::Stub> stub_;
};

int main(int argc, char** argv) {
    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1) {
        switch (opt) {
            case 'h':
                hostname = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);

    myc.run_client();

    return 0;
}

int Client::connectTo() {
    // create our stub
    stub_ = SNSService::NewStub(
        grpc::CreateChannel(
            hostname + ":" + port,
            grpc::InsecureChannelCredentials()));

    // container for server request
    Request request;
    request.set_username(username);

    // Container for server response
    Reply reply;

    // our context container
    ClientContext context;

    // Actual Remote Procedure Call
    Status status = stub_->Login(&context, request, &reply);

    // Returns results based on RPC status
    if (status.ok()) {
        return 1;
    } else {
        return -1;
    }
}

IReply Client::processCommand(std::string& input) {
    // our reply struct
    IReply ire;

    // parse input and store in command and arg strings
    std::string command;
    std::string arg = "";
    int spaceIndex = input.find(" ");

    if (spaceIndex == std::string::npos) {
        command = input;
    } else {
        command = input.substr(0, spaceIndex);
        arg = input.substr(spaceIndex + 1, input.size() - spaceIndex - 1);
    }

    // container for server request
    Request request;
    request.set_username(username);
    command = touppercase(command);
    request.set_arguments(arg);

    // Container for server response
    Reply reply;

    // our context container
    ClientContext context;

    // fire off RPC corresponding to command
    if (command == "FOLLOW") {
        grpc::Status status = stub_->Follow(&context, request, &reply);

        ire.grpc_status = status;

        if (status.ok()) {
            ire.comm_status = SUCCESS;
        } else {
            ire.comm_status = FAILURE_NOT_EXISTS;
        }
    } else if (command == "UNFOLLOW") {
        grpc::Status status = stub_->UnFollow(&context, request, &reply);

        ire.grpc_status = status;

        if (status.ok()) {
            ire.comm_status = SUCCESS;
        } else {
            ire.comm_status = FAILURE_NOT_EXISTS;
        }
    } else if (command == "LIST") {
        grpc::Status status = stub_->List(&context, request, &reply);

        ire.grpc_status = status;

        if (status.ok()) {
            ire.comm_status = SUCCESS;
        } else {
            ire.comm_status = FAILURE_UNKNOWN;
        }

        std::vector<std::string> following_users;
        std::vector<std::string> all_users;

        const google::protobuf::RepeatedPtrField<std::string>& all_users_protobuf = reply.all_users();
        for (int i = 0; i < all_users_protobuf.size(); i++) {
            all_users.push_back(all_users_protobuf[i]);
        }

        const google::protobuf::RepeatedPtrField<std::string>& following_users_protobuf = reply.following_users();
        for (int i = 0; i < following_users_protobuf.size(); i++) {
            following_users.push_back(following_users_protobuf[i]);
        }

        ire.all_users = all_users;
        ire.following_users = following_users;
    } else if (command == "TIMELINE") {
        ire.grpc_status = grpc::Status(grpc::StatusCode::OK, "Entering Timeline Mode");
        ire.comm_status = SUCCESS;
    }

    return ire;
}

void Client::processTimeline() {
    ClientContext context;

    // RPC Call
    std::shared_ptr<grpc::ClientReaderWriter<Message, Message> > stream(stub_->Timeline(&context));

    // initial write to tell server the username over this channel
    Message sendingUsername;
    sendingUsername.set_username(username);
    stream->Write(sendingUsername);

    // read thread implementation
    auto write = [this, stream]() {
        Message outgoing;

        outgoing.set_username(username);

        while (true) {
            outgoing.set_msg(getPostMessage());
            // outgoing.set_timestamp(time(0));

            stream->Write(outgoing);
        }
    };

    // read thread implementation
    auto read = [this, stream]() {
        Message incoming;

        while (true) {
            if (stream->Read(&incoming)) {
                time_t timeT = google::protobuf::util::TimeUtil::TimestampToTimeT(incoming.timestamp());
                displayPostMessage(incoming.username(), incoming.msg(), timeT);
            }
        }
    };

    std::thread writeThread(write);
    std::thread readThread(read);

    writeThread.join();
    readThread.join();
}
