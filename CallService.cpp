#include "CallService.h"



CallService& CallService::GetInstance() {
    static CallService callInstance;
    return callInstance;
}

bool CallService::Initialize(HWND mainHandle, SocketClient* socket, 
	HWND loginHandle, HWND contactHandle, HWND outCallHandle,
	HWND inCallHandle, HINSTANCE hInstance) {
    mMainWindow = mainHandle;
	m_socketClient = socket;
	m_loginWindow = loginHandle;
	m_contactWindow = contactHandle;
	m_outCallWindow = outCallHandle;
	m_inCallWindow = inCallHandle;
	m_hInstance = hInstance;
    // 윈도우 클래스 등록
    WNDCLASS wc = {};
    wc.lpfnWndProc = CallService::StaticWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"MyCallServiceClass";
    if (!RegisterClass(&wc))
        return false;

    // 메시지 처리용 윈도우 생성
    hWnd_ = CreateWindow(wc.lpszClassName, L"My Message Handler", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!hWnd_)
        return false;

    return true;
}

void CallService::Shutdown() {
    if (hWnd_) {
        DestroyWindow(hWnd_);
        hWnd_ = NULL;
    }

    UnregisterClass(L"MyCallServiceClass", GetModuleHandle(NULL));
}

void CallService::ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void CallService::SendMessageToHandler(UINT message, WPARAM wParam, LPARAM lParam) {
    PostMessage(hWnd_, message, wParam, lParam);
}

LRESULT CALLBACK CallService::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    CallService* instance = &GetInstance();
    return instance->WndProc(hWnd, message, wParam, lParam);

}

LRESULT CALLBACK CallService::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
	json11::Json loginJson;
	json11::Json joinJson;

    // 메시지 처리
    switch (msg) {
    case WM_USER:
        std::cout << "Received WM_USER message" << std::endl;
        break;

    case WM_RECEIVED_MESSAGE:
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
			ShowWindow(mMainWindow, SW_RESTORE);
			SetForegroundWindow(mMainWindow);

			Util::GetInstance().PlayRing();
			std::string callerEmail;
			callerEmail = contentsJson["email"].string_value();
			callIdString = contentsJson["callid"].string_value();

			SetDestEmail(callerEmail);

			PostMessage(mMainWindow, WM_CREATE_INCOMMING_WINDOW_MESSAGE, 0, 0);
		}
		else if (command == "SESSION")
		{
			if (response == "OK")
			{
				uuidString = contentsJson["uuid"].string_value();

				std::string welcome = "welcome " + uuidString;
				std::wstring welcomeMessage(welcome.begin(), welcome.end());

				SendMessage(m_loginWindow, WM_CLOSE, 0, 0);
				PostMessage(mMainWindow, WM_FOREGROUND_MAIN_MESSAGE, 0, 0);
//				MessageBox(hWnd, welcomeMessage.c_str(), _T("info"), MB_ICONERROR | MB_OK);
			}
			else
			{
				MessageBox(hWnd, _T("fail to login"), _T("info"), MB_ICONERROR | MB_OK);
			}
		}
		else if (command == "CANCEL")
		{
			PostMessage(mMainWindow, WM_HIDE_END_CALL_BUTTON_MESSAGE, 0, 0);

			Util::GetInstance().StopPlaying();

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
			PostMessage(mMainWindow, WM_SHOW_END_CALL_BUTTON_MESSAGE, 0, 0);

			Util::GetInstance().StopPlaying();
			// When I get accept message from callee
			std::string uuid;
			std::string callId;
			uuid = contentsJson["uuid"].string_value();
			callId = contentsJson["callid"].string_value();
			callIdString = callId;

			if (m_outCallWindow) {
				SendMessage(m_outCallWindow, WM_CLOSE, 0, 0);
			}
			SetupCall(1);

			myName = converter.from_bytes(GetDestEmail());
			SetWindowTextW(windows->hTextWnd1, myName.c_str());
		}
		else if (command == "LEAVE") // the other user leave the multi call room.
		{
			std::string myUUID = contentsJson["uuid"].string_value();
			std::string emailReave = contentsJson["leave_email"].string_value();
			std::string rooId = contentsJson["roomid"].string_value();
			
			// do something when other user leave the room.
		}
		else if (command == "BYE")
		{
			PostMessage(mMainWindow, WM_HIDE_END_CALL_BUTTON_MESSAGE, 0, 0);
		}
        break;

	case WM_LOGON_COMPLETED_MESSAGE:
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);

		SetMyUUID(receiveJson["uuid"].string_value());
		SetMyEmail(receiveJson["email"].string_value());

		loginJson = json11::Json::object{
			{"command", "SESSION"},
				{"contents",json11::Json::object{
					{"email", GetMyEmail()},
					{"uuid", GetMyUUID()}
				}
			}
		};
		m_socketClient->SendMessageW(loginJson.dump());
		
		myName = converter.from_bytes(GetMyEmail());
		SetWindowTextW(windows->hTextWnd0, myName.c_str());
		break;

	case WM_CONTACT_MESSAGE:
		Util::GetInstance().PlayOutCall();
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);
		targetEmail = receiveJson["email"].string_value();
		targetUuid = receiveJson["uuid"].string_value();

		SetDestEmail(targetEmail);

		wsEmail = std::wstring(targetEmail.begin(), targetEmail.end());

		//MessageBox(hWnd, wsEmail.c_str(), _T("message from contact"), MB_ICONERROR | MB_OK);

		PostMessage(mMainWindow, WM_CREATE_OUTGOING_WINDOW_MESSAGE, 0, 0);
		// send invite to server
		inviteJson = json11::Json::object{
			{"command", "INVITE"},
			{"contents", json11::Json::object {
				{"uuid", GetMyUUID()},
				{"target", targetEmail}}
			}
		};

		m_socketClient->SendMessageW(inviteJson.dump());

		break;

	case WM_JOIN_MULTICALL_MESSAGE:
		message = reinterpret_cast<const char*>(lParam);
		length = MultiByteToWideChar(CP_UTF8, 0, message, -1, nullptr, 0);
		wideMessage.resize(length);
		MultiByteToWideChar(CP_UTF8, 0, message, -1, &wideMessage[0], length);

		stdMessage = std::string(message);
		receiveJson = json11::Json::parse(stdMessage, errorMessage);

		SetCallId(receiveJson["roomid"].string_value());

		joinJson = json11::Json::object{
			{"command", "JOIN"},
			{"response", "PARTICIPATE"},
			{"contents", json11::Json::object{
				{"uuid", GetMyUUID()},
				{"roomid", GetCallId()}}
			}
		};

		m_socketClient->SendMessageW(joinJson.dump());

		PostMessage(mMainWindow, WM_SHOW_LEAVE_ROOM_BUTTON_MESSAGE, 0, 0);
		// Join mulicall 
		SetupCall(3);
		break;

	case WM_ACCEPT_INCOMMING_CALL_MESSAGE:
		// Accept Response to the caller
		PostMessage(mMainWindow, WM_SHOW_END_CALL_BUTTON_MESSAGE, 0, 0);
		Util::GetInstance().StopPlaying();
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
		SetCallId(contentsJson["callid"].string_value());
		wStringCommon = std::wstring(stdMessage.begin(), stdMessage.end());

		//MessageBox(hWnd, wStringCommon.c_str(), _T("accept call"), MB_ICONERROR | MB_OK);

		SetupCall(1);
		myName = converter.from_bytes(GetDestEmail());
		SetWindowTextW(windows->hTextWnd1, myName.c_str());
		break;
	case WM_BYE_MESSAGE:
		PostMessage(mMainWindow, WM_HIDE_END_CALL_BUTTON_MESSAGE, 0, 0);
		EndCall();
		break;
	case WM_LEAVE_MESSAGE: // When I leave the multi call room
		PostMessage(mMainWindow, WM_HIDE_LEAVE_ROOM_BUTTON_MESSAGE, 0, 0);
		// add here
		EndCall();
		break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

std::string CallService::GetCallId() {
	return callIdString;
}

std::string CallService::GetMyEmail() {
	return emailString;
}

std::string CallService::GetMyUUID() {
	return uuidString;
}

void CallService::SetCallId(std::string id) {
	callIdString = id;
}

void CallService::SetMyEmail(std::string email) {
	emailString = email;
}

void CallService::SetMyUUID(std::string uuid) {
	uuidString = uuid;
}

std::string CallService::GetDestEmail() {
	return destEmailString;
}

void CallService::SetDestEmail(std::string email) {
	destEmailString = email;
}

void CallService::UpdateOutCallHandle(HWND handle) {
	m_outCallWindow = handle;
}

void CallService::SetupCall(int numCalls) {
	// Intentionally not having the break
	switch(numCalls)
	{
	case 4:
		mManager.setupReceiver(windows->videoWindow4, 5007, 5008, 4); // third video setup
		mManager.playReceiver(4);
	case 3:
		mManager.setupReceiver(windows->videoWindow3, 5005, 5006, 3); // third video setup
		mManager.playReceiver(3);
	case 2:
		mManager.setupReceiver(windows->videoWindow2, 5003, 5004, 2); // second video setup
		mManager.playReceiver(2);
	case 1:
		mManager.setupReceiver(windows->videoWindow1, 5001, 5002, 1); // first video setup
		mManager.playReceiver(1);
	default:
		break;
	}
	mManager.setupSender(windows->videoWindow0, "192.168.2.8", 10001, 10002); // init manager with my video view
	mManager.makeCall(); // Receiver is decided by server application, just send video and audio
}

void CallService::EndCall() {
	mManager.endCall();

	InvalidateRect(windows->videoWindow4, NULL, TRUE);
	UpdateWindow(windows->videoWindow4);

	InvalidateRect(windows->videoWindow3, NULL, TRUE);
	UpdateWindow(windows->videoWindow3);

	InvalidateRect(windows->videoWindow2, NULL, TRUE);
	UpdateWindow(windows->videoWindow2);

	InvalidateRect(windows->videoWindow1, NULL, TRUE);
	UpdateWindow(windows->videoWindow1);

	InvalidateRect(windows->videoWindow0, NULL, TRUE);
	UpdateWindow(windows->videoWindow0);

}

void CallService::SetVideoHandles(VideoWindows* windows) {
	this->windows = windows;
}