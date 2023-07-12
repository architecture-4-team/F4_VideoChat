#pragma once
#include <ws2tcpip.h>
#include <thread>
#include <iostream>
#include <winsock2.h>
//#include "CallService.h"

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
    bool m_running_conn;
    bool m_is_connected;
//    CallService* m_callService;
    bool Initialize();
    void ReceiveThread();
    std::unique_ptr<std::thread> m_thread_conn;
    void ConnectionThread();
};
