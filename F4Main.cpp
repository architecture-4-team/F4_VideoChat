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
#include "MultimediaManager.h"
#include <gst/gst.h>

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

#define MEDIADEBUG 0

static LRESULT OnCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#if MEDIADEBUG
static void SetStdOutToNewConsole(void);
static FILE* pCout = NULL;
#endif

//SocketCommunication* socketComm = new SocketCommunication(std::string("127.0.0.1"), 10000);
SocketClient* socketClient = new SocketClient("127.0.0.1", 10000);

MultimediaManager& mManager = MultimediaManager::GetInstance();

HWND videoWindow0; // Video 출력용 윈도우 핸들
HWND videoWindow1; // Video 출력용 윈도우 핸들
HWND videoWindow2; // Video 출력용 윈도우 핸들
HWND videoWindow3; // Video 출력용 윈도우 핸들

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

	//SetStdOutToNewConsole();

	//gst_debug_set_default_threshold(GST_LEVEL_FIXME);
	//gst_debug_set_active(true);

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
#if MEDIADEBUG
		// 메뉴 선택을 구문 분석합니다:
		switch (LOWORD(wParam))
		{
		case IDC_START_SENDER:
			mManager.setupSender(videoWindow0, "127.0.0.1", 10001, 10002); // init manager with my video view
			// Server Ip has been set up at initialize
			mManager.makeCall(); // Receiver is decided by server application, just send video and audio
			break;
		case IDC_STOP_SENDER:
			mManager.pauseCall();
			break;
		case IDC_START_RECEIVER:
			mManager.setupReceiver(videoWindow1, 10001, 10002, 1); // first video setup
			mManager.playReceiver(1);
			break;
		case IDC_STOP_RECEIVER:
			mManager.pauseReceiver(1);
			break;
		case IDC_START_RECEIVER2:
			mManager.setupReceiver(videoWindow2, 10001, 10002, 2); // first video setup
			mManager.playReceiver(2);
			break;
		case IDC_STOP_RECEIVER2:
			mManager.pauseReceiver(2);
			break;
		case IDC_START_RECEIVER3:
			mManager.setupReceiver(videoWindow3, 10001, 10002, 3); // first video setup
			mManager.playReceiver(3);
			break;
		case IDC_STOP_RECEIVER3:
			mManager.pauseReceiver(3);
			break;
		case IDC_ACCEPT_ALL:
			mManager.setupReceiver(videoWindow1, 10001, 10002, 1); // first video setup
			mManager.setupReceiver(videoWindow2, 10001, 10002, 2); // second video setup
			mManager.setupReceiver(videoWindow3, 10001, 10002, 3); // third video setup
			mManager.makeReceiverStateChange(1, GST_STATE_PAUSED);
			mManager.makeReceiverStateChange(2, GST_STATE_PAUSED);
			mManager.makeReceiverStateChange(3, GST_STATE_PAUSED);
			mManager.playReceiver(1);
			mManager.playReceiver(2);
			mManager.playReceiver(3);
			break;
		}
#endif
		break;
	case WM_CREATE:
		OnCreate(hWnd, msg, wParam, lParam);
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

#if MEDIADEBUG
typedef enum
{
	_BUTNUM_SEND_START,
	_BUTNUM_SEND_STOP,
	_BUTNUM_RECV1_START,
	_BUTNUM_RECV1_STOP,
	_BUTNUM_RECV2_START,
	_BUTNUM_RECV2_STOP,
	_BUTNUM_RECV3_START,
	_BUTNUM_RECV3_STOP,
	_BUTNUM_RECV_ACCEPT_ALL,
	_BUTNUM_MAX
}E_BUT_NUM;
#endif
typedef enum
{
	_VIDEO_0,
	_VIDEO_1,
	_VIDEO_2,
	_VIDEO_3,
	_VIDEO_VIEW_MAX
}E_VIDEO_NUM;

static LRESULT OnCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const unsigned int videoWidth = 320;
	const unsigned int videoHeight = 240;
	const unsigned int videoWidthMargin = 20;
	const unsigned int videoHeightMargin = 80;

	unsigned int videoPosX[_VIDEO_VIEW_MAX] = { 20, };
	unsigned int videoPosY[_VIDEO_VIEW_MAX] = { 100, };

	videoPosX[_VIDEO_1] = videoPosX[_VIDEO_0] + videoWidth + videoWidthMargin;
	videoPosY[_VIDEO_1] = videoPosY[_VIDEO_0];


	videoPosX[_VIDEO_2] = videoPosX[_VIDEO_0];
	videoPosY[_VIDEO_2] = videoPosY[_VIDEO_0] + videoHeight + videoHeightMargin;


	videoPosX[_VIDEO_3] = videoPosX[_VIDEO_1];
	videoPosY[_VIDEO_3] = videoPosY[_VIDEO_2];


	// 비디오 윈도우 생성
	videoWindow0 = CreateWindowW(
		L"STATIC",
		L"Video Window",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_0], videoPosY[_VIDEO_0], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	// 비디오 출력을 위해 윈도우 스타일을 설정합니다.
	SetWindowLongPtr(videoWindow0, GWL_STYLE, GetWindowLongPtr(videoWindow0, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(videoWindow0, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

	// 비디오 윈도우 생성
	videoWindow1 = CreateWindowW(
		L"STATIC",
		L"Video Window1",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_1], videoPosY[_VIDEO_1], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	// 비디오 출력을 위해 윈도우 스타일을 설정합니다.
	SetWindowLongPtr(videoWindow1, GWL_STYLE, GetWindowLongPtr(videoWindow1, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(videoWindow1, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

	// 비디오 윈도우 생성
	videoWindow2 = CreateWindowW(
		L"STATIC",
		L"Video Window2",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_2], videoPosY[_VIDEO_2], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	// 비디오 출력을 위해 윈도우 스타일을 설정합니다.
	SetWindowLongPtr(videoWindow2, GWL_STYLE, GetWindowLongPtr(videoWindow2, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(videoWindow2, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

	// 비디오 윈도우 생성
	videoWindow3 = CreateWindowW(
		L"STATIC",
		L"Video Window3",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_3], videoPosY[_VIDEO_3], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	// 비디오 출력을 위해 윈도우 스타일을 설정합니다.
	SetWindowLongPtr(videoWindow3, GWL_STYLE, GetWindowLongPtr(videoWindow3, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(videoWindow3, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
#if MEDIADEBUG
	const unsigned int buttonWidth = 100;
	const unsigned int buttonHeight = 30;
	const unsigned int widthMargin = 20;
	const unsigned int heightMargin = 20;

	unsigned int posX[_BUTNUM_MAX] = { 70, };
	unsigned int posY[_BUTNUM_MAX] = { 350, };

	posX[_BUTNUM_SEND_START] = videoPosX[_VIDEO_0] + (videoWidth / 4) - (buttonWidth / 3);
	posY[_BUTNUM_SEND_START] = videoPosY[_VIDEO_0] + videoHeight + 10;
	posX[_BUTNUM_SEND_STOP] = posX[_BUTNUM_SEND_START] + buttonWidth + widthMargin;
	posY[_BUTNUM_SEND_STOP] = posY[_BUTNUM_SEND_START];

	posX[_BUTNUM_RECV1_START] = videoPosX[_VIDEO_1] + (videoWidth / 4) - (buttonWidth / 3);
	posY[_BUTNUM_RECV1_START] = videoPosY[_VIDEO_1] + videoHeight + 10;
	posX[_BUTNUM_RECV1_STOP] = posX[_BUTNUM_RECV1_START] + buttonWidth + widthMargin;
	posY[_BUTNUM_RECV1_STOP] = posY[_BUTNUM_RECV1_START];

	posX[_BUTNUM_RECV2_START] = videoPosX[_VIDEO_2] + (videoWidth / 4) - (buttonWidth / 3);
	posY[_BUTNUM_RECV2_START] = videoPosY[_VIDEO_2] + videoHeight + 10;
	posX[_BUTNUM_RECV2_STOP] = posX[_BUTNUM_RECV2_START] + buttonWidth + widthMargin;
	posY[_BUTNUM_RECV2_STOP] = posY[_BUTNUM_RECV2_START];

	posX[_BUTNUM_RECV3_START] = videoPosX[_VIDEO_3] + (videoWidth / 4) - (buttonWidth / 3);
	posY[_BUTNUM_RECV3_START] = videoPosY[_VIDEO_3] + videoHeight + 10;
	posX[_BUTNUM_RECV3_STOP] = posX[_BUTNUM_RECV3_START] + buttonWidth + widthMargin;
	posY[_BUTNUM_RECV3_STOP] = posY[_BUTNUM_RECV3_START];

	posX[_BUTNUM_RECV_ACCEPT_ALL] = 700;
	posY[_BUTNUM_RECV_ACCEPT_ALL] = 10;


	// 버튼 생성
	CreateWindow(
		_T("button"),
		_T("Start"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_SEND_START], posY[_BUTNUM_SEND_START], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_START_SENDER,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	// 버튼 생성
	CreateWindow(
		_T("button"),
		_T("Stop"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_SEND_STOP], posY[_BUTNUM_SEND_STOP], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_STOP_SENDER,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Start R1"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV1_START], posY[_BUTNUM_RECV1_START], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_START_RECEIVER,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Stop R1"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV1_STOP], posY[_BUTNUM_RECV1_STOP], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_STOP_RECEIVER,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Start R2"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV2_START], posY[_BUTNUM_RECV2_START], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_START_RECEIVER2,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Stop R2"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV2_STOP], posY[_BUTNUM_RECV2_STOP], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_STOP_RECEIVER2,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Start R3"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV3_START], posY[_BUTNUM_RECV3_START], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_START_RECEIVER3,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Stop R3"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV3_STOP], posY[_BUTNUM_RECV3_STOP], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_STOP_RECEIVER3,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("AcceptAll"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV_ACCEPT_ALL], posY[_BUTNUM_RECV_ACCEPT_ALL], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_ACCEPT_ALL,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);
#endif
	return 1;
}

#if MEDIADEBUG
static void SetStdOutToNewConsole(void)
{
	// Allocate a console for this app
	AllocConsole();
	AttachConsole(ATTACH_PARENT_PROCESS);
	freopen_s(&pCout, "CONOUT$", "w", stdout);
}
#endif