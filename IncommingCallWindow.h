#pragma once
#include "socketClient.h"
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <stdio.h>
#include <iostream>
#include "json11.hpp"
#include "WebView2.h"
#include "common.h"
#include "CallService.h"

using namespace Microsoft::WRL;
// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewControllerInCall;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webviewInCall;


class IncommingCallWindow
{
public:
	IncommingCallWindow(HWND window, SocketClient* socketClient, HWND mainWindow, std::string callee, std::string myUUID, std::string myEmail, std::string callId);
	void startWebview(HWND gWindow);

private:
	HWND hWindow;
};
