#include "helpers.h"



// Global counter to act as id for posts
int postCounter = 1;

struct post {
    std::string user, content;
    int id;
};

// Global vector of every post, acting as a stack of sorts. Newest post is at index 0
std::vector<post> all_posts;

class User {
   private:
    std::string name;
    std::vector<std::string> following;
    std::vector<std::string> followers;
    std::vector<int> timeline;
    std::vector<int> posts_seen;

   public:
    User(std::string _name) {
        name = _name;
    }

    std::string get_name() {
        return name;
    }

    void follow(std::string name) {
        for (int i = 0; i < following.size(); i++) {
            if (following[i] == name) {
                printf("Already following %s\n", name.c_str());
                return;
            }
        }

        following.push_back(name);
    }

    void unfollow(std::string name) {
        for (int i = 0; i < following.size(); i++) {
            if (following[i] == name) {
                following.erase(following.begin() + i);
                return;
            }
        }

        printf("Already not following %s\n", name.c_str());
    }

    std::vector<std::string> get_followers() {
        return followers;
    }

    void add_follower(std::string name) {
        followers.push_back(name);
    }

    void remove_follower(std::string name) {
        for (int i = 0; i < followers.size(); i++) {
            if (followers[i] == name) {
                followers.erase(followers.begin() + i);
                return;
            }
        }
    }

    void add_post(int Post) {
        timeline.insert(timeline.begin(), Post);
    }

    bool havent_seen_it(int Post) {
        for (int i = 0; i < posts_seen.size(); i++) {
            if (posts_seen[i] == Post) {
                return false;
            }
        }

        return true;
    }

    bool following_them(std::string name) {
        for (int i = 0; i < following.size(); i++) {
            if (following[i] == name) {
                return true;
            }
        }

        return false;
    }

    struct post* updateTimeline() {
        for (int i = 0; i < all_posts.size(); i++) {
            struct post Post = all_posts[i];
            if (havent_seen_it(Post.id) && following_them(Post.user)) {
                posts_seen.push_back(Post.id);
                return &Post;
            }
        }

        return nullptr;
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
        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == name) {
                return 0;
            }
        }

        users.push_back(User(name));

        numUsers++;

        return 1;
    }

    int follow(std::string user, std::string user_to_follow) {
        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == user_to_follow) {
                for (int j = 0; j < numUsers; j++) {
                    if (users[j].get_name() == user) {
                        users[j].follow(user_to_follow);
                    }
                }

                for (int j = 0; j < numUsers; j++) {
                    if (users[j].get_name() == user_to_follow) {
                        users[j].add_follower(user);
                    }
                }

                return 1;
            }
        }

        printf("There is no user with the name %s \n", user_to_follow.c_str());
        return 0;
    }

    int unfollow(std::string user, std::string user_to_unfollow) {
        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == user_to_unfollow) {
                for (int j = 0; j < numUsers; j++) {
                    if (users[j].get_name() == user) {
                        users[j].unfollow(user_to_unfollow);
                    }
                }

                for (int j = 0; j < numUsers; j++) {
                    if (users[j].get_name() == user_to_unfollow) {
                        users[j].remove_follower(user);
                    }
                }

                return 1;
            }
        }

        printf("There is no user with the name %s \n", user_to_unfollow.c_str());
        return 0;
    }

    std::vector<std::string> followers(std::string name) {
        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == name) {
                return users[i].get_followers();
            }
        }
    }

    std::vector<User> all_users() {
        return users;
    }

    void userPosted(std::string name, int Post) {
        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == name) {
                users[i].add_post(Post);
            }
        }
    }

    struct post *updateTimeline(std::string name) {
        for (int i = 0; i < numUsers; i++) {
            if (users[i].get_name() == name) {
                return users[i].updateTimeline();
            }
        }
    }
};

// Global instance of our server. Stores the data for this server, essentially a local DB
ServerInstance serverInstance;

class SNSServiceImpl final : public SNSService::Service {
    Status List(ServerContext* context, const Request* request, Reply* reply) override {
        // get username of request
        std::string user = request->username();

        std::vector<User> all_users = serverInstance.all_users();
        for (int i = 0; i < all_users.size(); i++) {
            reply->add_all_users(all_users[i].get_name());
        }

        std::vector<std::string> following_users = serverInstance.followers(user);
        for (int i = 0; i < following_users.size(); i++) {
            reply->add_following_users(following_users[i]);
        }

        return Status::OK;
    }

    Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
        // get username of request
        std::string user = request->username();
        std::string user_to_follow = request->arguments();

        int followSuccess = serverInstance.follow(user, user_to_follow);

        if (followSuccess) {
            return Status::OK;
        } else {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "There is no user with username: " + user_to_follow);
        }
    }

    Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
        // get username of request
        std::string user = request->username();
        std::string user_to_follow = request->arguments();

        int unfollowSuccess = serverInstance.unfollow(user, user_to_follow);

        if (unfollowSuccess) {
            return Status::OK;
        } else {
            return grpc::Status(grpc::StatusCode::NOT_FOUND, "There is no user with username: " + user_to_follow);
        }
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
        Message incoming;
        Message outgoing;
        bool read = true;

        stream->Read(&incoming);
        std::string user = incoming.username();

        while (true) {
            if (read) {
                if (stream->Read(&incoming)) {
                    struct post Post;

                    Post.id = postCounter++;
                    Post.user = incoming.username();
                    Post.content = incoming.msg();

                    serverInstance.userPosted(user, Post.id);
                    all_posts.insert(all_posts.begin(), Post);
                }

                read = false;
            } else {
                struct post* PostPtr = serverInstance.updateTimeline(user);

                if (PostPtr) {
                    struct post Post = *PostPtr;

                    outgoing.set_username(Post.user);
                    outgoing.set_msg(Post.content);

                    stream->Write(outgoing);
                }

                read = true;
            }
        }

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
