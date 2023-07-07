#include "socketClient.h"
#include "common.h"

HWND m_mainWindow;

SocketClient::SocketClient(const char* ipAddress, int port) : 
    m_socket(INVALID_SOCKET), 
    m_thread(nullptr), m_running(false) 
{
    m_ipAddress = ipAddress;
    m_port = port;
}

bool SocketClient::Connect(HWND mainWindow) {
    m_mainWindow = mainWindow;
    // �ʱ�ȭ
    if (!Initialize())
        return false;

    // ���� ����
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET) {
        //Cleanup();
        return false;
    }

    // ���� ���� ����
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(m_port);
    inet_pton(AF_INET, m_ipAddress, &(serverAddress.sin_addr));

    // ������ ����
    if (connect(m_socket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        //Cleanup();
        return false;
    }

    // ���� ������ ����
    m_running = true;
    m_thread = std::make_unique<std::thread>(&SocketClient::ReceiveThread, this);

    return true;
}

void SocketClient::Disconnect() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    m_running = false;

    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }

    WSACleanup();
}

void SocketClient::SendMessage(const std::string& message) {
    if (m_socket != INVALID_SOCKET) {
        send(m_socket, message.c_str(), strlen(message.c_str()), 0);
    }
}

bool SocketClient::Initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;

    return true;
}

void SocketClient::ReceiveThread() {
    const int bufferSize = 1024;
    char buffer[bufferSize];

    while (m_running) {
        int bytesReceived = recv(m_socket, buffer, bufferSize - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Received: " << buffer << std::endl;
            PostMessage(m_mainWindow, WM_SOCKET_MESSAGE, 0, (LPARAM)buffer);
        }
        else if (bytesReceived == 0) {
            // ������ ���� ���
            break;
        }
        else {
            // ������ �߻��� ���
            break;
        }
    }
}

