#pragma once
#include "socketClient.h"
#include <iostream>
#include <windows.h>
#include <tchar.h>
#include "common.h"
#include "json11.hpp"
#include "IncommingCallWindow.h"

class CallProcessService
{
public:
	static CallProcessService& GetInstance();
	bool Initialize(HWND hMainWnd, SocketClient* socket, HWND loginHandle, HWND contactHandle, HWND outCallHandle, HWND inCallHandle, HINSTANCE m_hInstance);
	void Shutdown();
	void ProcessMessages();
	void SendMessageToHandler(UINT message, WPARAM wParam, LPARAM lParam);
	std::string GetCallId();
	std::string GetMyEmail();
	std::string GetMyUUID();
	void SetCallId(std::string id);
	void SetMyEmail(std::string email);
	void SetMyUUID(std::string uuid);

private:
	HWND hWnd_;
	HWND gMainWnd;
	std::string uuidString = "";
	std::string emailString = "";
	std::string callIdString = "";
	HWND m_loginWindow;
	HWND m_contactWindow;
	HWND m_outCallWindow;
	HWND m_inCallWindow;
	HINSTANCE m_hInstance;
	SocketClient* m_socketClient;
	IncommingCallWindow* incommingCallWindow;
	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

