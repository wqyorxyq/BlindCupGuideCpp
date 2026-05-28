#include "blind_cup_guide/LocalHttpServer.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace bcg {
namespace {

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle invalidSocket = INVALID_SOCKET;

void closeSocket(SocketHandle socket) {
    closesocket(socket);
}
#else
using SocketHandle = int;
constexpr SocketHandle invalidSocket = -1;

void closeSocket(SocketHandle socket) {
    close(socket);
}
#endif

SocketHandle toSocket(std::uintptr_t value) {
    return static_cast<SocketHandle>(value);
}

std::uintptr_t fromSocket(SocketHandle socket) {
    return static_cast<std::uintptr_t>(socket);
}

std::string statusText(int status) {
    switch (status) {
    case 200:
        return "OK";
    case 204:
        return "No Content";
    case 400:
        return "Bad Request";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 500:
        return "Internal Server Error";
    default:
        return "OK";
    }
}

std::string lowerCopy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string trim(std::string text) {
    const auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
                         return std::isspace(ch) != 0;
                     }).base();

    if (begin >= end) {
        return {};
    }
    return {begin, end};
}

bool sendAll(SocketHandle socket, const char* data, int length) {
    int sent = 0;
    while (sent < length) {
        const int result = send(socket, data + sent, length - sent, 0);
        if (result <= 0) {
            return false;
        }
        sent += result;
    }
    return true;
}

bool sendAll(SocketHandle socket, const std::string& data) {
    return sendAll(socket, data.data(), static_cast<int>(data.size()));
}

std::string readRequestBytes(SocketHandle socket) {
    std::string data;
    char buffer[4096]{};

    while (data.find("\r\n\r\n") == std::string::npos) {
        const int received = recv(socket, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (received <= 0) {
            return data;
        }
        data.append(buffer, received);
        if (data.size() > 1024 * 1024) {
            throw std::runtime_error("HTTP request header is too large.");
        }
    }

    const auto headerEnd = data.find("\r\n\r\n");
    const auto header = data.substr(0, headerEnd);
    std::istringstream headerStream(header);
    std::string line;
    std::getline(headerStream, line);

    std::size_t contentLength = 0;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        auto name = lowerCopy(trim(line.substr(0, colon)));
        auto value = trim(line.substr(colon + 1));
        if (name == "content-length") {
            contentLength = static_cast<std::size_t>(std::stoul(value));
        }
    }

    const auto bodyStart = headerEnd + 4;
    while (data.size() - bodyStart < contentLength) {
        const int received = recv(socket, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (received <= 0) {
            break;
        }
        data.append(buffer, received);
    }

    return data;
}

HttpRequest parseRequest(const std::string& raw) {
    HttpRequest request;
    const auto headerEnd = raw.find("\r\n\r\n");
    const auto header = headerEnd == std::string::npos ? raw : raw.substr(0, headerEnd);

    std::istringstream stream(header);
    std::string requestLine;
    std::getline(stream, requestLine);
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }

    std::istringstream requestLineStream(requestLine);
    std::string target;
    requestLineStream >> request.method >> target;
    const auto queryMarker = target.find('?');
    if (queryMarker == std::string::npos) {
        request.path = target;
    } else {
        request.path = target.substr(0, queryMarker);
        request.query = target.substr(queryMarker + 1);
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        request.headers[lowerCopy(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
    }

    if (headerEnd != std::string::npos) {
        request.body = raw.substr(headerEnd + 4);
        const auto found = request.headers.find("content-length");
        if (found != request.headers.end()) {
            const auto length = static_cast<std::size_t>(std::stoul(found->second));
            if (request.body.size() > length) {
                request.body.resize(length);
            }
        }
    }

    return request;
}

void sendResponse(SocketHandle socket, const HttpResponse& response) {
    std::ostringstream header;
    header << "HTTP/1.1 " << response.status << ' ' << statusText(response.status) << "\r\n";
    header << "Content-Type: " << response.contentType << "\r\n";
    header << "Content-Length: " << response.body.size() << "\r\n";
    header << "Connection: close\r\n";
    header << "Cache-Control: no-store\r\n";
    for (const auto& [name, value] : response.headers) {
        header << name << ": " << value << "\r\n";
    }
    header << "\r\n";

    sendAll(socket, header.str());
    if (!response.body.empty()) {
        sendAll(socket, response.body);
    }
}

int hexValue(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

std::string urlDecode(std::string_view value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index) {
        const char ch = value[index];
        if (ch == '+') {
            decoded.push_back(' ');
        } else if (ch == '%' && index + 2 < value.size()) {
            const int high = hexValue(value[index + 1]);
            const int low = hexValue(value[index + 2]);
            if (high >= 0 && low >= 0) {
                decoded.push_back(static_cast<char>(high * 16 + low));
                index += 2;
            } else {
                decoded.push_back(ch);
            }
        } else {
            decoded.push_back(ch);
        }
    }
    return decoded;
}

} // namespace

LocalHttpServer::LocalHttpServer(int port, Handler handler)
    : port_(port),
      handler_(std::move(handler)) {}

LocalHttpServer::~LocalHttpServer() {
    stop();
}

void LocalHttpServer::start() {
    if (running_) {
        return;
    }

#ifdef _WIN32
    WSADATA wsaData{};
    const int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        throw std::runtime_error("WSAStartup failed.");
    }
#endif

    const SocketHandle serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == invalidSocket) {
        throw std::runtime_error("Failed to create HTTP socket.");
    }

    int reuse = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse),
               sizeof(reuse));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(static_cast<unsigned short>(port_));

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closeSocket(serverSocket);
        throw std::runtime_error("Failed to bind HTTP server to 127.0.0.1:" +
                                 std::to_string(port_) + ".");
    }

    if (listen(serverSocket, SOMAXCONN) != 0) {
        closeSocket(serverSocket);
        throw std::runtime_error("Failed to listen on HTTP socket.");
    }

    listenSocket_ = fromSocket(serverSocket);
    running_ = true;
    acceptThread_ = std::thread(&LocalHttpServer::acceptLoop, this);
}

void LocalHttpServer::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    const auto socket = toSocket(listenSocket_);
    if (socket != invalidSocket) {
#ifdef _WIN32
        shutdown(socket, SD_BOTH);
#else
        shutdown(socket, SHUT_RDWR);
#endif
        closeSocket(socket);
    }

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

bool LocalHttpServer::isRunning() const {
    return running_;
}

int LocalHttpServer::port() const {
    return port_;
}

void LocalHttpServer::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddress{};
#ifdef _WIN32
        int addressLength = sizeof(clientAddress);
#else
        socklen_t addressLength = sizeof(clientAddress);
#endif
        const SocketHandle clientSocket =
            accept(toSocket(listenSocket_), reinterpret_cast<sockaddr*>(&clientAddress),
                   &addressLength);

        if (clientSocket == invalidSocket) {
            if (running_) {
                std::cerr << "HTTP accept failed.\n";
            }
            continue;
        }

        std::thread(&LocalHttpServer::handleClient, this, fromSocket(clientSocket)).detach();
    }
}

void LocalHttpServer::handleClient(std::uintptr_t clientSocketValue) {
    const auto clientSocket = toSocket(clientSocketValue);
    try {
        const auto raw = readRequestBytes(clientSocket);
        if (!raw.empty()) {
            const auto request = parseRequest(raw);
            auto response = handler_(request);
            sendResponse(clientSocket, response);
        }
    } catch (const std::exception& error) {
        HttpResponse response;
        response.status = 500;
        response.body = std::string{"Internal server error: "} + error.what();
        sendResponse(clientSocket, response);
    }

    closeSocket(clientSocket);
}

std::map<std::string, std::string> parseUrlEncoded(const std::string& body) {
    std::map<std::string, std::string> values;
    std::size_t start = 0;
    while (start <= body.size()) {
        const auto ampersand = body.find('&', start);
        const auto end = ampersand == std::string::npos ? body.size() : ampersand;
        const auto part = std::string_view{body}.substr(start, end - start);
        if (!part.empty()) {
            const auto equals = part.find('=');
            const auto key = equals == std::string_view::npos ? part : part.substr(0, equals);
            const auto value = equals == std::string_view::npos ? std::string_view{} :
                                                               part.substr(equals + 1);
            values[urlDecode(key)] = urlDecode(value);
        }
        if (ampersand == std::string::npos) {
            break;
        }
        start = ampersand + 1;
    }
    return values;
}

} // namespace bcg
