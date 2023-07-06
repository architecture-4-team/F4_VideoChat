#include "SocketCommunication.h"

#pragma comment(lib, "ws2_32.lib")


SocketCommunication::SocketCommunication(const std::string& serverIP, int serverPort) :
    serverIP_(serverIP),
    serverPort_(serverPort),
    stopThread_(false),
    responseReceived_(false)
{
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0) {
        throw std::runtime_error("Failed to initialize Winsock.");
    }

    // Create a socket
    clientSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket_ == INVALID_SOCKET) {
        WSACleanup();
        throw std::runtime_error("Failed to create socket.");
    }

    // Set up server details
    serverAddress_.sin_family = AF_INET;
    if (inet_pton(AF_INET, serverIP_.c_str(), &(serverAddress_.sin_addr)) <= 0) {
        closesocket(clientSocket_);
        WSACleanup();
        throw std::runtime_error("Invalid server IP address.");
    }
    serverAddress_.sin_port = htons(serverPort_);
}

SocketCommunication::~SocketCommunication() {
    // Clean up
    stopThread_ = true;
    cv_.notify_one();
    if (thread_.joinable()) {
        thread_.join();
    }
    closesocket(clientSocket_);
    WSACleanup();
}

void SocketCommunication::Start() {
    if (!thread_.joinable()) {
        thread_ = std::thread(&SocketCommunication::ThreadRoutine, this);
    }
}

void SocketCommunication::SendMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    outgoingMessage_ = message;
    responseReceived_ = false;
    cv_.notify_one();
}

std::string SocketCommunication::ReceiveResponse() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return responseReceived_; });
    return incomingMessage_;
}

void SocketCommunication::ThreadRoutine() {
    // Connect to the server
    if (connect(clientSocket_, reinterpret_cast<sockaddr*>(&serverAddress_), sizeof(serverAddress_)) == SOCKET_ERROR) {
        throw std::runtime_error("Failed to connect to the server.");
    }

    const int bufferSize = 4096;
    char buffer[bufferSize]{};

    while (!stopThread_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return !outgoingMessage_.empty() || stopThread_; });

        if (!stopThread_) {
            // Send message to the server
            if (send(clientSocket_, outgoingMessage_.c_str(), outgoingMessage_.size(), 0) == SOCKET_ERROR) {
                throw std::runtime_error("Failed to send data to the server.");
            }

            // Clear the outgoing message
            outgoingMessage_.clear();

            // Receive response from the server
            int bytesReceived = recv(clientSocket_, buffer, bufferSize, 0);
            if (bytesReceived > 0) {
                incomingMessage_ = std::string(buffer, bytesReceived);
                responseReceived_ = true;
                lock.unlock();
                cv_.notify_one();
            }
            else if (bytesReceived == 0) {
                throw std::runtime_error("Connectionclosed by the server.");
            }
            else {
                throw std::runtime_error("Error in receiving data from the server.");
            }

            // Clear the buffer
            memset(buffer, 0, bufferSize);
        }
    }
}


