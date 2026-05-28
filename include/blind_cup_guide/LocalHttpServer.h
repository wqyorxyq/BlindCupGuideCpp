#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <thread>

namespace bcg {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::map<std::string, std::string> headers;
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/plain; charset=utf-8";
    std::string body;
    std::map<std::string, std::string> headers;
};

class LocalHttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    LocalHttpServer(int port, Handler handler);
    ~LocalHttpServer();

    LocalHttpServer(const LocalHttpServer&) = delete;
    LocalHttpServer& operator=(const LocalHttpServer&) = delete;

    void start();
    void stop();
    bool isRunning() const;
    int port() const;

private:
    void acceptLoop();
    void handleClient(std::uintptr_t clientSocket);

    int port_ = 8080;
    Handler handler_;
    std::atomic<bool> running_{false};
    std::uintptr_t listenSocket_ = 0;
    std::thread acceptThread_;
};

std::map<std::string, std::string> parseUrlEncoded(const std::string& body);

} // namespace bcg
