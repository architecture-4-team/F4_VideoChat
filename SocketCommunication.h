#pragma once
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <WS2tcpip.h>
#include <Winsock2.h>
class SocketCommunication
{
public:
    SocketCommunication(const std::string& serverIP, int serverPort);
    ~SocketCommunication();
    void Start();
    void SendMessage(const std::string& message);
    std::string ReceiveResponse();
    void ThreadRoutine();

private:
    std::string serverIP_;
    int serverPort_;
    WSADATA wsaData_;
    SOCKET clientSocket_;
    sockaddr_in serverAddress_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::string outgoingMessage_;
    std::string incomingMessage_;
    bool responseReceived_;
    bool stopThread_;
};

