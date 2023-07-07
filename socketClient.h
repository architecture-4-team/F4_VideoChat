#pragma once
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
class SocketClient {
public:
    SocketClient(const char* ipAddress, int port);
    bool Connect(HWND mainWindow);
    void Disconnect();
    void SendMessage(const std::string&);

private:
    const char* m_ipAddress;
    int m_port;
    SOCKET m_socket;
    std::unique_ptr<std::thread> m_thread;
    bool m_running;

    bool Initialize();
    void ReceiveThread();
};
