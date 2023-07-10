#include "CallProcessService.h"

CallProcessService& CallProcessService::GetInstance() {
	static CallProcessService instance;
	return instance;
}

bool CallProcessService::Initialize(HWND hMainWnd, SocketClient* socket,
							HWND loginHandle, HWND contactHandle, HWND outCallHandle, 
							HWND inCallHandle, HINSTANCE hInstance) {
    gMainWnd = hMainWnd;
	m_socketClient = socket;
	m_loginWindow = loginHandle;
	m_contactWindow = contactHandle;
	m_outCallWindow = outCallHandle;
	m_inCallWindow = inCallHandle;
	m_hInstance = hInstance;
    // 윈도우 클래스 등록
    WNDCLASS wc = {};
    wc.lpfnWndProc = CallProcessService::StaticWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"CallMessageHandlerClass";
    if (!RegisterClass(&wc))
        return false;

    // 메시지 처리용 윈도우 생성
    hWnd_ = CreateWindow(wc.lpszClassName, L"Call Message Handler", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!hWnd_)
        return false;

    return true;
}

void CallProcessService::Shutdown() {
    if (hWnd_) {
        DestroyWindow(hWnd_);
        hWnd_ = NULL;
    }

    UnregisterClass(L"CallMessageHandlerClass", GetModuleHandle(NULL));
}

void CallProcessService::ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void CallProcessService::SendMessageToHandler(UINT message, WPARAM wParam, LPARAM lParam) {
    PostMessage(hWnd_, message, wParam, lParam);
}


LRESULT CALLBACK CallProcessService::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	CallProcessService* instance = new CallProcessService();
    return instance->WndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CallProcessService::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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

    // 메시지 처리
    switch (msg) {
    case WM_USER:
        std::cout << "Received WM_USER message" << std::endl;
        break;

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
			m_inCallWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Incomming Call Window"), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, m_hInstance, nullptr);
			if (!m_inCallWindow)
			{
				MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
				return 1;
			}

			ShowWindow(m_inCallWindow, SW_SHOW);
			UpdateWindow(m_inCallWindow);

			incommingCallWindow = new IncommingCallWindow(m_inCallWindow, m_socketClient, gMainWnd, callerEmail, uuidString, emailString, callIdString);
			incommingCallWindow->startWebview(m_inCallWindow);

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

				if (m_outCallWindow) {
					SendMessage(m_outCallWindow, WM_CLOSE, 0, 0);
				}
			}
			else if (response == "BUSY")
			{
				MessageBox(hWnd, _T("Callee is busy."), _T("info"), MB_ICONERROR | MB_OK);

				if (m_outCallWindow) {
					SendMessage(m_outCallWindow, WM_CLOSE, 0, 0);
				}

			}
			else if (response == "REJECT")
			{
				MessageBox(hWnd, _T("Callee is rejected."), _T("info"), MB_ICONERROR | MB_OK);

				if (m_outCallWindow) {
					SendMessage(m_outCallWindow, WM_CLOSE, 0, 0);
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

			if (m_outCallWindow) {
				SendMessage(m_outCallWindow, WM_CLOSE, 0, 0);
			}
		}
		break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

std::string CallProcessService::GetCallId() {
	return callIdString;
}

std::string CallProcessService::GetMyEmail() {
	return emailString;
}

std::string CallProcessService::GetMyUUID() {
	return uuidString;
}

void CallProcessService::SetCallId(std::string id) {
	callIdString = id;
}

void CallProcessService::SetMyEmail(std::string email) {
	emailString = email;
}

void CallProcessService::SetMyUUID(std::string uuid) {
	uuidString = uuid;
}