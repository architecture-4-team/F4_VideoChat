#pragma once
#include "socketClient.h"
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include "common.h"
#include "json11.hpp"
#include "MultimediaManager.h"

typedef struct {
    HWND videoWindow0; // for Sender
    HWND videoWindow1; // for Receiver1
    HWND videoWindow2; // for Receiver2
    HWND videoWindow3; // for Receiver3
    HWND videoWindow4; // for Receiver4
}VideoWindows;

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
    void SetVideoHandles(VideoWindows* windows);

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

    MultimediaManager& mManager = MultimediaManager::GetInstance();
    VideoWindows* windows;
    void SetupCall(int numCalls);
    void EndCall();

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};