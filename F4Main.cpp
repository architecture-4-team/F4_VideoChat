//#include "SocketCommunication.h"
#include "socketClient.h"
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <stdio.h>
#include <iostream>
#include "resource.h"
#include "json11.hpp"
#include "common.h"
#include "LoginWindow.h"
#include "ContactsListWindow.h"
#include <mmsystem.h>
#include "CallService.h"
#include "IncommingCallWindow.h"
#include "OutgoingCallWindow.h"
#include "Util.h"

// Global variables
HINSTANCE g_hInstance;

// Forward declarations
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


std::string uiServerAddress = "";

HWND g_hEdit;

HWND g_loginWindow;
HWND g_mainWindow;
HWND g_contactWindow;
HWND g_outCallWindow;
HWND g_inCallWindow;

OutgoingCallWindow* outGoingCallWindow;
IncommingCallWindow* incommingCallWindow;

CallService* callService = &CallService::GetInstance();
Util* util = &Util::GetInstance();

SocketClient* socketClient;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hMainWindow;
	HWND hLoginWindow;

	// Store the instance handle
	g_hInstance = hInstance;

	// load config file
	TCHAR _buffer[MAX_PATH];
	DWORD res = GetCurrentDirectory(MAX_PATH, _buffer);
	_tcscat_s(_buffer, _T("/config.json"));

	std::string jsonStr = util->LoadFile(_buffer);
	std::string err;
	json11::Json jsonConfig = json11::Json::parse(jsonStr, err);

	std::string serverAddress = jsonConfig["server_address"].string_value();
	int serverPort = jsonConfig["server_port"].int_value();
	std::string uiServerAddress = jsonConfig["ui_server_address"].string_value();
	std::string uiLocal = jsonConfig["ui_local"].string_value();

	socketClient = new SocketClient("127.0.0.1", serverPort);


	// Register the main window class
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, MainWndProc, 0, 0, hInstance, nullptr,
						LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, _T("MainWindowClass"), nullptr };
	if (!RegisterClassEx(&wcex))
	{
		MessageBox(nullptr, _T("Failed to register window class."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Register the child window class
	WNDCLASSEX wcexChild = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, ChildWndProc, 0, 0, hInstance, nullptr,
							 LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, _T("ChildWindowClass"), nullptr };
	if (!RegisterClassEx(&wcexChild))
	{
		MessageBox(nullptr, _T("Failed to register child window class."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create the main window
	hMainWindow = CreateWindowEx(0, _T("MainWindowClass"), _T("Main Window"), WS_OVERLAPPEDWINDOW,
		0, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);
	
	if (!hMainWindow)
	{
		MessageBox(nullptr, _T("Failed to create main window."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create a contact list button
	HWND hButton = CreateWindowEx(0, _T("BUTTON"), _T("Contact List"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		10, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(1), hInstance, nullptr);
	if (!hButton)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create a call end button
	HWND hCallEndButton = CreateWindowEx(0, _T("BUTTON"), _T("End Call"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		120, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(2), hInstance, nullptr);
	if (!hCallEndButton)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create the login window
	hLoginWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Login Window"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 600, hMainWindow, nullptr, hInstance, nullptr);

	if (!hLoginWindow)
	{
		MessageBox(hMainWindow, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	g_mainWindow = hMainWindow;
	g_loginWindow = hLoginWindow;

	if (!callService->Initialize(g_mainWindow, socketClient, g_loginWindow, g_contactWindow, g_outCallWindow, g_inCallWindow, g_hInstance)) {
		MessageBox(hMainWindow, _T("Failed to create call service."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}
	callService->ProcessMessages();
	
	// Show and update the main window
	ShowWindow(hMainWindow, SW_MAXIMIZE);
	UpdateWindow(hMainWindow);

	// Show and update the main window
	ShowWindow(hLoginWindow, nCmdShow);
	UpdateWindow(hLoginWindow);

	socketClient->Connect(hMainWindow);

	LoginWindow* loginWindow = new LoginWindow(hLoginWindow, socketClient, g_mainWindow, uiServerAddress);
	loginWindow->startWebview(g_loginWindow);

	// Message loop
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const char* message;
	std::wstring wideMessage;
	std::string stdMessage;
	std::string errorMessage;
	json11::Json receiveJson;
	std::string command;
	std::string response;
	std::string contentsJsonString;
	json11::Json contentsJson;
	int length;
	std::string targetEmail;
	std::string targetUuid;
	std::wstring wsEmail;
	json11::Json inviteJson;
	std::wstring wStringCommon;

	switch (msg)
	{
	case WM_SOCKET_MESSAGE:
		callService->SendMessageToHandler(WM_RECEIVED_MESSAGE, wParam, lParam);
		break;

	case WM_CREATE_INCOMMING_WINDOW_MESSAGE:
		// create incomming call window
		g_inCallWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Incomming Call Window"), WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, g_hInstance, nullptr);
		if (!g_inCallWindow)
		{
			MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
			return 1;
		}

		ShowWindow(g_inCallWindow, SW_SHOW);
		UpdateWindow(g_inCallWindow);

		incommingCallWindow = new IncommingCallWindow(g_inCallWindow, socketClient, g_mainWindow, 
			callService->GetDestEmail(), callService->GetMyUUID(), callService->GetMyEmail(), callService->GetCallId());
		incommingCallWindow->startWebview(g_inCallWindow);

		break;

	case WM_CREATE_OUTGOING_WINDOW_MESSAGE:
		g_outCallWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Calling Window"), WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, g_hInstance, nullptr);
		if (!g_outCallWindow)
		{
			MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
			return 1;
		}

		ShowWindow(g_outCallWindow, SW_SHOW);
		UpdateWindow(g_outCallWindow);

		outGoingCallWindow = new OutgoingCallWindow(g_outCallWindow, socketClient, g_mainWindow, 
			callService->GetDestEmail(), callService->GetMyUUID(), callService->GetMyEmail(), callService->GetCallId());
		outGoingCallWindow->startWebview(g_outCallWindow);

		callService->UpdateOutCallHandle(g_outCallWindow);
		break;

	case WM_DESTROY:
		socketClient->Disconnect();
		PostQuitMessage(0);
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == 1) 
		{
			HWND hContactWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Contact List"), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, g_hInstance, nullptr);
			if (!hContactWindow)
			{
				MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
				return 1;
			}

			g_contactWindow = hContactWindow;

			// Show the child window
			ShowWindow(hContactWindow, SW_SHOW);
			UpdateWindow(hContactWindow);

			ContactListWindow* contactsListWindow = new ContactListWindow(hContactWindow, socketClient, g_mainWindow, uiServerAddress);
			contactsListWindow->startWebview(g_contactWindow, callService->GetMyUUID());

		}
		else if (LOWORD(wParam) == 2)
		{
			json11::Json byeJson = json11::Json::object{
				{"command", "BYE"},
				{"contents", json11::Json::object {
						{"uuid", callService->GetMyUUID()},
						{"callid", callService->GetCallId()}
					}
				}
			}; 
			socketClient->SendMessageW(byeJson.dump());
		}
		break;

	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

