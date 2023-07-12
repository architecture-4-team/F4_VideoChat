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
#include "EditInfoWindow.h"
#include "Util.h"

#define MEDIADEBUG 0
#define USE_QUAD_WINDOW_CALL 0

// Global variables
HINSTANCE g_hInstance;

// Forward declarations
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT OnSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT OnCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#if MEDIADEBUG
void CreateConsoleWindow();

json11::Json testJson = json11::Json::object{
	{"command", "ACCEPT"},
	{"contents", json11::Json::object {
			{"uuid", "g_in_uuid"},
			{"callid", "g_in_callId"}
		}
	}
};
std::string testJsonStr = testJson.dump();
const char* testStr = testJsonStr.c_str();
#endif

std::string uiServerAddress = "";

MultimediaManager& mManager = MultimediaManager::GetInstance();

VideoWindows windows;

HWND g_loginWindow;
HWND g_mainWindow;
HWND g_contactWindow;
HWND g_outCallWindow;
HWND g_inCallWindow;
HWND g_editInfoWindow;

OutgoingCallWindow* outGoingCallWindow;
IncommingCallWindow* incommingCallWindow;

CallService* callService = &CallService::GetInstance();
Util* util = &Util::GetInstance();

SocketClient* socketClient;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hMainWindow;
	HWND hLoginWindow;

	//CreateConsoleWindow();

	//gst_debug_set_default_threshold(GST_LEVEL_FIXME);
	//gst_debug_set_active(true);

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
	uiServerAddress = jsonConfig["ui_server_address"].string_value();
	std::string uiLocal = jsonConfig["ui_local"].string_value();

	socketClient = new SocketClient(serverAddress.c_str(), serverPort);

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

	// Create a end call button
	HWND hCallEndButton = CreateWindowEx(0, _T("BUTTON"), _T("End Call"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		120, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(2), hInstance, nullptr);
	if (!hCallEndButton)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create a change user info button
	HWND hEditInfoButton = CreateWindowEx(0, _T("BUTTON"), _T("My Info"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		250, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(3), hInstance, nullptr);
	if (!hCallEndButton)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create the login window
	g_loginWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Login Window"), WS_OVERLAPPEDWINDOW,
		400, 50, 600, 800, hMainWindow, nullptr, hInstance, nullptr);

	if (!g_loginWindow)
	{
		MessageBox(hMainWindow, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	g_mainWindow = hMainWindow;

	if (!callService->Initialize(g_mainWindow, socketClient, g_loginWindow, g_contactWindow, g_outCallWindow, g_inCallWindow, g_hInstance)) {
		MessageBox(hMainWindow, _T("Failed to create call service."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}
	callService->SetVideoHandles(&windows);
	callService->ProcessMessages();

	// Show and update the main window
	ShowWindow(hMainWindow, SW_MAXIMIZE);
	UpdateWindow(hMainWindow);

	// Show and update the main window
	ShowWindow(g_loginWindow, SW_SHOW);
	UpdateWindow(g_loginWindow);

	socketClient->Connect(hMainWindow);

	LoginWindow* loginWindow = new LoginWindow(g_loginWindow, socketClient, g_mainWindow, uiServerAddress);
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
				CW_USEDEFAULT, CW_USEDEFAULT, 600, 800, nullptr, nullptr, g_hInstance, nullptr);
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

			// Call end
			callService->SendMessageToHandler(WM_BYE_MESSAGE, wParam, lParam);
		}
		else if (LOWORD(wParam) == 3)
		{
			HWND hEditInfoWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Contact List"), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 600, 800, nullptr, nullptr, g_hInstance, nullptr);
			if (!hEditInfoWindow)
			{
				MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
				return 1;
			}

			g_editInfoWindow = hEditInfoWindow;

			// Show the child window
			ShowWindow(hEditInfoWindow, SW_SHOW);
			UpdateWindow(hEditInfoWindow);

			EditInfoWindow* editInfoListWindow = new EditInfoWindow(hEditInfoWindow, socketClient, g_mainWindow, uiServerAddress);
			editInfoListWindow->startWebview(g_editInfoWindow, callService->GetMyUUID());
		}
#if MEDIADEBUG
		
		switch (LOWORD(wParam))
		{
		case IDC_START_SENDER:
			testJson = json11::Json::object{
				{"command", "ACCEPT"},
				{"contents", json11::Json::object {
						{"uuid", "g_in_uuid"},
						{"callid", "g_in_callId"}
					}
				}
			};
			testJsonStr = testJson.dump();
			testStr = testJsonStr.c_str();
			callService->SendMessageToHandler(WM_ACCEPT_INCOMMING_CALL_MESSAGE, 0, (LPARAM)testStr);
			break;
		case IDC_STOP_SENDER:

			break;
		case IDC_START_RECEIVER:

			break;
		case IDC_STOP_RECEIVER:

			break;
		case IDC_START_RECEIVER2:

			break;
		case IDC_STOP_RECEIVER2:

			break;
		case IDC_START_RECEIVER3:

			break;
		case IDC_STOP_RECEIVER3:

			break;
		case IDC_ACCEPT_ALL:
			//callService->SendMessageToHandler(WM_ACCEPT_INCOMMING_CALL_MESSAGE, 0, NULL);
			break;
		}
#endif
		break;
	case WM_CREATE:
		OnCreate(hWnd, msg, wParam, lParam);
		break;
	case WM_SIZE:
		OnSize(hWnd, msg, wParam, lParam);
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			// TODO: 여기에 hdc를 사용하는 그리기 코드를 추가합니다...

			RECT rect;
			GetClientRect(hWnd, &rect);
			HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255)); // 배경색을 흰색으로 설정
			FillRect(hdc, &rect, hBrush);
			DeleteObject(hBrush);

			EndPaint(hWnd, &ps);
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
		return 0;
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
	_BUTNUM_RECV4_START,
	_BUTNUM_RECV4_STOP,
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
	_VIDEO_4,
	_VIDEO_VIEW_MAX
}E_VIDEO_NUM;


static LRESULT OnCreate(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const unsigned int videoWidth = 320;
	const unsigned int videoHeight = 240;
	const unsigned int videoWidthMargin = 20;
	const unsigned int videoHeightMargin = 80;

	unsigned int videoPosX[_VIDEO_VIEW_MAX] = { 20 };
	unsigned int videoPosY[_VIDEO_VIEW_MAX] = { 100 };

	std::fill(videoPosX, videoPosX + _VIDEO_VIEW_MAX, 20);
	std::fill(videoPosY, videoPosY + _VIDEO_VIEW_MAX, 100);

#if USE_QUAD_WINDOW_CALL
	videoPosX[_VIDEO_0] = 750;
	videoPosY[_VIDEO_0] = 200;

	videoPosX[_VIDEO_2] = videoPosX[_VIDEO_1] + videoWidth + videoWidthMargin;
	videoPosY[_VIDEO_2] = videoPosY[_VIDEO_1];

	videoPosX[_VIDEO_3] = videoPosX[_VIDEO_1];
	videoPosY[_VIDEO_3] = videoPosY[_VIDEO_1] + videoHeight + videoHeightMargin;

	videoPosX[_VIDEO_4] = videoPosX[_VIDEO_2];
	videoPosY[_VIDEO_4] = videoPosY[_VIDEO_3];
#else
	videoPosX[_VIDEO_1] = videoPosX[_VIDEO_0] + videoWidth + videoWidthMargin;
	videoPosY[_VIDEO_1] = videoPosY[_VIDEO_0];

	videoPosX[_VIDEO_2] = videoPosX[_VIDEO_0];
	videoPosY[_VIDEO_2] = videoPosY[_VIDEO_0] + videoHeight + videoHeightMargin;

	videoPosX[_VIDEO_3] = videoPosX[_VIDEO_1];
	videoPosY[_VIDEO_3] = videoPosY[_VIDEO_2];
#endif 

	windows.videoWindow0 = CreateWindowW(
		L"STATIC",
		L"Sender",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_0], videoPosY[_VIDEO_0], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);

	SetWindowLongPtr(windows.videoWindow0, GWL_STYLE, GetWindowLongPtr(windows.videoWindow0, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(windows.videoWindow0, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

	windows.videoWindow1 = CreateWindowW(
		L"STATIC",
		L"Receiver1",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_1], videoPosY[_VIDEO_1], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	
	SetWindowLongPtr(windows.videoWindow1, GWL_STYLE, GetWindowLongPtr(windows.videoWindow1, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(windows.videoWindow1, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

	
	windows.videoWindow2 = CreateWindowW(
		L"STATIC",
		L"Receiver2",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_2], videoPosY[_VIDEO_2], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	
	SetWindowLongPtr(windows.videoWindow2, GWL_STYLE, GetWindowLongPtr(windows.videoWindow2, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(windows.videoWindow2, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

	
	windows.videoWindow3 = CreateWindowW(
		L"STATIC",
		L"Receiver3",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_3], videoPosY[_VIDEO_3], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	
	SetWindowLongPtr(windows.videoWindow3, GWL_STYLE, GetWindowLongPtr(windows.videoWindow3, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(windows.videoWindow3, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);

#if USE_QUAD_WINDOW_CALL
	windows.videoWindow4 = CreateWindowW(
		L"STATIC",
		L"Receiver4",
		WS_CHILD | WS_VISIBLE | WS_BORDER,
		videoPosX[_VIDEO_4], videoPosY[_VIDEO_4], videoWidth, videoHeight,
		hWnd, nullptr, g_hInstance, nullptr
	);
	
	SetWindowLongPtr(windows.videoWindow4, GWL_STYLE, GetWindowLongPtr(windows.videoWindow4, GWL_STYLE) | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	SetWindowPos(windows.videoWindow4, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_ASYNCWINDOWPOS);
#endif 
	
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

	posX[_BUTNUM_RECV4_START] = videoPosX[_VIDEO_4] + (videoWidth / 4) - (buttonWidth / 3);
	posY[_BUTNUM_RECV4_START] = videoPosY[_VIDEO_4] + videoHeight + 10;
	posX[_BUTNUM_RECV4_STOP] = posX[_BUTNUM_RECV4_START] + buttonWidth + widthMargin;
	posY[_BUTNUM_RECV4_STOP] = posY[_BUTNUM_RECV4_START];

	posX[_BUTNUM_RECV_ACCEPT_ALL] = 700;
	posY[_BUTNUM_RECV_ACCEPT_ALL] = 10;


	// ��ư ����
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

	// ��ư ����
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
		_T("Start R4"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV4_START], posY[_BUTNUM_RECV4_START], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_START_RECEIVER4,
		((LPCREATESTRUCT)lParam)->hInstance,
		NULL
	);

	CreateWindow(
		_T("button"),
		_T("Stop R4"),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		posX[_BUTNUM_RECV4_STOP], posY[_BUTNUM_RECV4_STOP], buttonWidth, buttonHeight,
		hWnd,
		(HMENU)IDC_STOP_RECEIVER4,
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

LRESULT OnSize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int cxClient, cyClient;

    cxClient = LOWORD(lParam);
    cyClient = HIWORD(lParam);

    return DefWindowProc(hWnd, message, wParam, lParam);
}


#if MEDIADEBUG
void SetStdOutToNewConsole()
{
	// Attach the console to the calling process
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		// If the console is not available, create a new console
		AllocConsole();
	}

	// Redirect standard input/output to the console
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
	freopen_s(reinterpret_cast<FILE**>(stdin), "CONIN$", "r", stdin);
	freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);

	// Update the synchronization of C and C++ standard streams
	std::ios::sync_with_stdio();
}
#endif