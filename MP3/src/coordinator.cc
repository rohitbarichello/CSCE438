#include <google/protobuf/duration.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <stdlib.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <functional>
#include <thread>

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

std::unordered_map<int, std::vector<int>> masterTable;
std::unordered_map<int, std::vector<int>> slaveTable;
std::unordered_map<int, std::vector<int>> syncTable;

// vector of threads that shut down their server if they don't hear from it every 10 seconds
std::vector<std::thread> serverTimers;


std::string assignServer(int client_id) {
    int serverId = -1;

    serverId = (client_id % 3) + 1;

    // check if assigned server is active
    if (masterTable.at(serverId)[1]) {
        return std::to_string(masterTable.at(serverId)[0]); // return port for master
    }
    else {
        return std::to_string(slaveTable.at(serverId)[0]); // return port for slave
    }
}

// void serverDead(int server) {
//     // masterTable.at(server)[1] = 0;

//     std::cout<<"Server " << server  <<" is dead"<<std::endl;
// }

// std::thread timer_start(std::function<void(int)> func, unsigned int interval, int server) {
//     return std::thread([&, func, interval, server]() {
//         while (true) {
//             auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
//             func(server);
//             std::this_thread::sleep_until(x);
//         }
//     });
// }

class SNSServiceImpl final : public SNSService::Service {
    Status FindServerAddr(ServerContext* context, const Request* request, Reply* reply) override {
        std::string client_id_str = request->username();
        int client_id = stoi(client_id_str);
        std::string hostname = "127.0.0.1";
        std::string port = assignServer(client_id);

        reply->set_msg(hostname + ":" + port);

        return Status::OK;
    }

    // Status Ping(ServerContext* context, const Request* request, Reply* reply) override {
    //     std::string server_id_str = request->username();
    //     int server_id = stoi(server_id_str);

    //     masterTable.at(server_id)[1] = 1;
    //     std::cout<<"Server "<< server_id <<" back online"<<std::endl;

    //     serverTimers[server_id - 1].join();
    //     serverTimers[server_id - 1] = timer_start(serverDead, 10000, server_id);

    //     return Status::OK;
    // }
};


void RunCoordinator(std::string port_no) {
    std::string server_address = "127.0.0.1:" + port_no;
    SNSServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Coordinator listening on " << server_address << std::endl;

    // std::thread timerThread = timer_start(serverDead, 10000, 1);
    // serverTimers.push_back(timerThread);
    // timerThread = timer_start(serverDead, 10000, 2);
    // serverTimers.push_back(timerThread);
    // timerThread = timer_start(serverDead, 10000, 3);
    // serverTimers.push_back(timerThread);


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

    // initialize routing tables
    masterTable[1] = {9190, 1};
    masterTable[2] = {9290, 1};
    masterTable[3] = {9390, 1};
    slaveTable[1] = {9490, 1};
    slaveTable[2] = {9590, 1};
    slaveTable[3] = {9690, 1};
    syncTable[1] = {9790, 1};
    syncTable[2] = {9890, 1};
    syncTable[3] = {9990, 1};

    // run coordinator 
    RunCoordinator(port);

    return 0;
}
