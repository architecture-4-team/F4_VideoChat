#pragma once
#include "socketClient.h"
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include "common.h"
#include "json11.hpp"

class CallService {
public:
    static CallService& GetInstance();
    bool Initialize(HWND mainHandle, SocketClient* socket,
        HWND loginHandle, HWND contactHandle, HWND outCallHandle,
        HWND inCallHandle, HINSTANCE hInstance);
    void Shutdown();
    void ProcessMessages();
    void SendMessageToHandler(UINT message, WPARAM wParam, LPARAM lParam);

    std::string GetCallId();
    std::string GetMyEmail();
    std::string GetMyUUID();
    std::string GetDestEmail();

    void SetCallId(std::string id);
    void SetMyEmail(std::string email);
    void SetMyUUID(std::string uuid);
    void SetDestEmail(std::string email);

    void UpdateOutCallHandle(HWND handle);

private:
    HWND hWnd_;
    HWND mMainWindow;
    std::string callIdString;
    std::string emailString;
    std::string uuidString;
    std::string destEmailString;
    SocketClient* m_socketClient;
    HWND m_loginWindow;
    HWND m_contactWindow;
    HWND m_outCallWindow;
    HWND m_inCallWindow;
    HINSTANCE m_hInstance;
    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};