#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "sns.grpc.pb.h"

using csce438::ListReply;
using csce438::Message;
using csce438::Reply;
using csce438::Request;
using csce438::SNSService;
using google::protobuf::Duration;
using google::protobuf::Timestamp;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;

std::string id;
int oldFileSize_timeline = 0;
int oldFileSize_followers = 0;
std::unique_ptr<SNSService::Stub> sync_stub;

void checkTimeline() {
    // check size of file
    std::ifstream in_file(id + ".txt", std::ios::binary);
    in_file.seekg(0, std::ios::end);
    int file_size = in_file.tellg();

    if file_size
        > oldFileSize {
            oldFileSize_timeline = file_size;
            sendNewTimeline();
        }
}

void checkFollowers() {
    // check size of file
    std::ifstream in_file(id + "followers.txt", std::ios::binary);
    in_file.seekg(0, std::ios::end);
    int file_size = in_file.tellg();

    if file_size
        > oldFileSize {
            oldFileSize_followers = file_size;
            sendNewFollowers(line);
        }
}

void sendNewFollowers(std::string line) {
    Request request;
    Reply reply;
    ClientContext context;

    request.set_username(id);
    request.set_filetype("followers");
    request.set_arguments(line)

    Status status = coordinator_stub->Ping(&context, request, &reply);
}

void sendNewTimeline(std::string line) {
    Request request;
    Reply reply;
    ClientContext context;

    request.set_username(id);
    request.set_filetype("timeline");
    request.set_arguments(line)

    Status status = coordinator_stub->Ping(&context, request, &reply);
}

std::thread timer_start(std::function<void(void)> func, unsigned int interval) {
    return std::thread([&, func, interval]() {
        while (true) {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
            func();
            std::this_thread::sleep_until(x);
        }
    });
}

void connectToSynchronizer() {
    std::string synchronizer_connection_info = synchronizer_ip + ":" + synchronizer_port;

    sync_stub = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
        grpc::CreateChannel(
            coordinator_connection_info, grpc::InsecureChannelCredentials())));
}

class SNSServiceImpl final : public SNSService::Service {
    Status SendFileData(ServerContext* context, const Request* request, Reply* reply) override {
        std::string line = request->arguments();
        std::string fileID = request->username();
        std::string fileType = request->fileType();

        std::string filename = "";

        if(fileType == "timeline") {
            filename = fileID + ".txt";
        }
        else if(fileType == "followers") {
            filename = fileID + "followers.txt";
        }

        fstream newfile;
        newfile.open(filename, std::ios::out);  
        if (newfile.is_open())                 
            newfile << line;  
        newfile.close();                       
    }

    return Status::OK;
}


void RunSynchronizer(std::string port_no) {
    std::string server_address = "127.0.0.1:" + port_no;
    SNSServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Synchronizer listening on " << server_address << std::endl;

    timer_start(checkTimeline, 30000);
    timer_start(checkFollowers, 30000);

    server->Wait();
}

int main(int argc, char** argv) {
    std::string port = "3010";
    int opt = 0;
    id = "4";
    while ((opt = getopt(argc, argv, "p:i:")) != -1) {
        switch (opt) {
            case 'p':
                port = optarg;
                break;
            case 'i':
                port = optarg;
                break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    // run synchronizer
    RunSynchronizer(port);

    return 0;
}
