//#include "SocketCommunication.h"
#include "socketClient.h"
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <stdio.h>
#include <iostream>
#include "WebView2.h"
#include "resource.h"
#include "json11.hpp"
#include "common.h"
#include "LoginWindow.h"
#include "ContactsListWindow.h"
#include "OutgoingCallWindow.h"
#include <mmsystem.h>
#include "IncommingCallWindow.h"

using namespace Microsoft::WRL;

// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webview;

// Global variables
HINSTANCE g_hInstance;

// Forward declarations
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//SocketCommunication* socketComm = new SocketCommunication(std::string("127.0.0.1"), 10000);
SocketClient* socketClient = new SocketClient("127.0.0.1", 10000);

HWND g_loginWindow;
HWND g_mainWindow;
HWND g_contactWindow;
HWND g_callWindow;
HWND g_inCallWindow;

std::string uuidString = "";
std::string emailString = "";
std::string callIdString = "";

OutgoingCallWindow* outGoingCallWindow;
IncommingCallWindow* incommingCallWindow;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hMainWindow;
	HWND hLoginWindow;

	// Store the instance handle
	g_hInstance = hInstance;

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

	socketClient->Connect(hMainWindow);

	// Show and update the main window
	ShowWindow(hMainWindow, SW_MAXIMIZE);
	UpdateWindow(hMainWindow);

	// Show and update the main window
	ShowWindow(hLoginWindow, nCmdShow);
	UpdateWindow(hLoginWindow);


	LoginWindow* loginWindow = new LoginWindow(hLoginWindow, socketClient, g_mainWindow);
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
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);
		command = receiveJson["command"].string_value();
		response = receiveJson["response"].string_value();
		contentsJsonString = receiveJson["contents"].dump();
		contentsJson = json11::Json::parse(contentsJsonString, errorMessage);

		if (command == "INVITE")
		{
			std::string callerEmail;
			callerEmail = contentsJson["email"].string_value();
			callIdString = contentsJson["callid"].string_value();

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

			incommingCallWindow = new IncommingCallWindow(g_inCallWindow, socketClient, g_mainWindow, callerEmail, uuidString, emailString, callIdString);
			incommingCallWindow->startWebview(g_inCallWindow);

		}
		else if (command == "LOGIN") 
		{
			if (response == "OK") 
			{
				uuidString = contentsJson["uuid"].string_value();
				emailString = contentsJson["email"].string_value();

				std::string welcome = "welcome " + uuidString + " " + emailString;
				std::wstring welcomeMessage(welcome.begin(), welcome.end());

				MessageBox(hWnd, welcomeMessage.c_str(), _T("info"), MB_ICONERROR | MB_OK);
			}
			else
			{
				MessageBox(hWnd, _T("fail to login"), _T("info"), MB_ICONERROR | MB_OK);
			}
		}
		else if (command == "CANCEL")
		{
			if (response == "NOT AVAILABLE")
			{
				MessageBox(hWnd, _T("Callee is not available."), _T("info"), MB_ICONERROR | MB_OK);

				if (g_callWindow) {
					SendMessage(g_callWindow, WM_CLOSE, 0, 0);
				}
			}
			else if (response == "BUSY")
			{
				MessageBox(hWnd, _T("Callee is busy."), _T("info"), MB_ICONERROR | MB_OK);

				if (g_callWindow) {
					SendMessage(g_callWindow, WM_CLOSE, 0, 0);
				}

			}
			else if (response == "REJECT")
			{
				MessageBox(hWnd, _T("Callee is rejected."), _T("info"), MB_ICONERROR | MB_OK);

				if (g_callWindow) {
					SendMessage(g_callWindow, WM_CLOSE, 0, 0);
				}

			}
		}
		else if (command == "ACCEPT")
		{
			std::string uuid;
			std::string callId;
			uuid = contentsJson["uuid"].string_value();
			callId = contentsJson["callid"].string_value();
			callIdString = callId;

			if (g_callWindow) {
				SendMessage(g_callWindow, WM_CLOSE, 0, 0);
			}

		}
		break;

	case WM_LOGON_COMPLETED_MESSAGE:
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);
		targetEmail = receiveJson["email"].string_value();
		targetUuid = receiveJson["uuid"].string_value();

		//uuidString = targetUuid; //todo: we have to change of return value of login request.
		emailString = targetEmail;

		wsEmail = std::wstring(targetEmail.begin(), targetEmail.end());

		MessageBox(hWnd, wsEmail.c_str(), _T("login completed!"), MB_ICONERROR | MB_OK);

		break;

	case WM_CONTACT_MESSAGE:
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);
		targetEmail = receiveJson["email"].string_value();
		targetUuid = receiveJson["uuid"].string_value();

		wsEmail = std::wstring(targetEmail.begin(), targetEmail.end());

		MessageBox(hWnd, wsEmail.c_str(), _T("message from contact"), MB_ICONERROR | MB_OK);

		// create outgoing call window
		g_callWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Calling Window"), WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, g_hInstance, nullptr);
		if (!g_callWindow)
		{
			MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
			return 1;
		}

		ShowWindow(g_callWindow, SW_SHOW);
		UpdateWindow(g_callWindow);

		outGoingCallWindow = new OutgoingCallWindow(g_callWindow, socketClient, g_mainWindow, targetEmail, uuidString, emailString, callIdString);
		outGoingCallWindow->startWebview(g_callWindow);

		// send invite to server
		inviteJson = json11::Json::object{
			{"command", "INVITE"},
			{"contents", json11::Json::object {
				{"uuid", uuidString},
				{"target", targetEmail}}
			}
		};

		socketClient->SendMessageW(inviteJson.dump());

		break;

	case WM_ACCEPT_INCOMMING_CALL_MESSAGE:
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);
		command = receiveJson["command"].string_value();
		contentsJsonString = receiveJson["contents"].dump();
		contentsJson = json11::Json::parse(contentsJsonString, errorMessage);

		contentsJson["uuid"].string_value();
		callIdString = contentsJson["callid"].string_value();

		wStringCommon = std::wstring(stdMessage.begin(), stdMessage.end());

		MessageBox(hWnd, wStringCommon.c_str(), _T("accept call"), MB_ICONERROR | MB_OK);

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

			ContactListWindow* contactsListWindow = new ContactListWindow(hContactWindow, socketClient, g_mainWindow);
			contactsListWindow->startWebview(g_contactWindow);

		}
		else if (LOWORD(wParam) == 2)
		{
			json11::Json byeJson = json11::Json::object{
				{"command", "BYE"},
				{"contents", json11::Json::object {
						{"uuid", uuidString},
						{"callid", callIdString}
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