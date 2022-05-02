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
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

std::string coordinator_ip = "127.0.0.1";
std::string coordinator_port = "9090";
std::string id;
std::unique_ptr<SNSService::Stub> coordinator_stub;
std::string port;
std::string type;

struct Client {
    std::string username;
    bool connected = true;
    int following_file_size = 0;

    std::vector<Client*> client_followers;
    std::vector<Client*> client_following;

    // api for a bidirectional streaming call
    // writes and reads Message protobuf
    ServerReaderWriter<Message, Message>* stream = 0;

    // overloaded == operator
    bool operator==(const Client& c1) const {
        return (username == c1.username);
    }
};

//Vector that stores every client that has been created
std::vector<Client> client_db;

//Helper function used to find a Client object given its username
int find_user(std::string username) {
    int index = 0;
    for (Client c : client_db) {
        if (c.username == username)
            return index;
        index++;
    }
    return -1;
}

// makes a function call periodially given an interval
void timer_start(std::function<void(void)> func, unsigned int interval) {
    std::thread([&, func, interval]() {
        while (true) {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
            func();
            std::this_thread::sleep_until(x);
        }
    }).detach();
}

class SNSServiceImpl final : public SNSService::Service {
    Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
        Client user = client_db[find_user(request->username())];
        int index = 0;
        for (Client c : client_db) {
            list_reply->add_all_users(c.username);
        }
        std::vector<Client*>::const_iterator it;
        for (it = user.client_followers.begin(); it != user.client_followers.end(); it++) {
            list_reply->add_followers((*it)->username);
        }
        return Status::OK;
    }

    Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
        std::string username1 = request->username();
        std::string username2 = request->arguments(0);
        int join_index = find_user(username2);
        if (join_index < 0 || username1 == username2)
            reply->set_msg("unkown user name");
        else {
            Client* user1 = &client_db[find_user(username1)];
            Client* user2 = &client_db[join_index];
            if (std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()) {
                reply->set_msg("you have already joined");
                return Status::OK;
            }
            user1->client_following.push_back(user2);
            user2->client_followers.push_back(user1);
            reply->set_msg("Follow Successful");
        }
        return Status::OK;
    }

    Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
        std::string username1 = request->username();
        std::string username2 = request->arguments(0);
        int leave_index = find_user(username2);
        if (leave_index < 0 || username1 == username2)
            reply->set_msg("unknown follower username");
        else {
            Client* user1 = &client_db[find_user(username1)];
            Client* user2 = &client_db[leave_index];
            if (std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()) {
                reply->set_msg("you are not follower");
                return Status::OK;
            }
            user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2));
            user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
            reply->set_msg("UnFollow Successful");
        }
        return Status::OK;
    }

    Status Login(ServerContext* context, const Request* request, Reply* reply) override {
        Client c;
        std::string username = request->username();
        int user_index = find_user(username);
        if (user_index < 0) {
            c.username = username;
            client_db.push_back(c);
            reply->set_msg("Login Successful!");
        } else {
            Client* user = &client_db[user_index];
            if (user->connected)
                reply->set_msg("Invalid Username");
            else {
                std::string msg = "Welcome Back " + user->username;
                reply->set_msg(msg);
                user->connected = true;
            }
        }
        return Status::OK;
    }

    Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
        Message message;
        Client* c;
        while (stream->Read(&message)) {
            std::string username = message.username();
            int user_index = find_user(username);
            c = &client_db[user_index];

            //Write the current message to "username.txt"
            std::string filename = username + ".txt";
            std::ofstream user_file(filename, std::ios::app | std::ios::out | std::ios::in);
            google::protobuf::Timestamp temptime = message.timestamp();
            std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
            std::string fileinput = time + " :: " + message.username() + ":" + message.msg() + "\n";
            //"Set Stream" is the default message from the client to initialize the stream
            if (message.msg() != "Set Stream")
                user_file << fileinput;
            //If message = "Set Stream", print the first 20 chats from the people you follow
            else {
                if (c->stream == 0)
                    c->stream = stream;
                std::string line;
                std::vector<std::string> newest_twenty;
                std::ifstream in(username + "following.txt");
                int count = 0;
                //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
                while (getline(in, line)) {
                    if (c->following_file_size > 20) {
                        if (count < c->following_file_size - 20) {
                            count++;
                            continue;
                        }
                    }
                    newest_twenty.push_back(line);
                }
                Message new_msg;
                //Send the newest messages to the client to be displayed
                for (int i = 0; i < newest_twenty.size(); i++) {
                    new_msg.set_msg(newest_twenty[i]);
                    stream->Write(new_msg);
                }
                continue;
            }
            //Send the message to each follower's stream
            std::vector<Client*>::const_iterator it;
            for (it = c->client_followers.begin(); it != c->client_followers.end(); it++) {
                Client* temp_client = *it;
                if (temp_client->stream != 0 && temp_client->connected)
                    temp_client->stream->Write(message);
                //For each of the current user's followers, put the message in their following.txt file
                std::string temp_username = temp_client->username;
                std::string temp_file = temp_username + "following.txt";
                std::ofstream following_file(temp_file, std::ios::app | std::ios::out | std::ios::in);
                following_file << fileinput;
                temp_client->following_file_size++;
                std::ofstream user_file(temp_username + ".txt", std::ios::app | std::ios::out | std::ios::in);
                user_file << fileinput;
            }
        }
        //If the client disconnected from Chat Mode, set connected to false
        c->connected = false;
        return Status::OK;
    }
};

void connectToCoordinator() {
    std::string coordinator_connection_info = coordinator_ip + ":" + coordinator_port;

    coordinator_stub = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
        grpc::CreateChannel(
            coordinator_connection_info, grpc::InsecureChannelCredentials())));
}

void pingCoordinator() {
    std::cout<<"Pinging Coordinator"<<std::endl;

    Request request;
    Reply reply;
    ClientContext context;

    request.set_username(id);

    Status status = coordinator_stub->Ping(&context, request, &reply);
}

void RunServer(std::string port_no) {
    std::string server_address = "127.0.0.1:" + port_no;
    SNSServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    connectToCoordinator();
    // timer_start(pingCoordinator, 10000);
    server->Wait();
}

int main(int argc, char** argv) {
    port = "3010";
    id = "4";
    type = "master";
    int opt = 0;

    while ((opt = getopt(argc, argv, "i:p:t:")) != -1) {
        switch (opt) {
            case 'i':
                id = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 't':
                type = optarg;
                break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    RunServer(port);

    return 0;
}
