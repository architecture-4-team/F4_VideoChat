#pragma once
#include "socketClient.h"
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <stdio.h>
#include <iostream>
#include "WebView2.h"
#include "json11.hpp"

using namespace Microsoft::WRL;
// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewControllerOutCall;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webviewOutCall;


class OutgoingCallWindow
{
public:
	OutgoingCallWindow(HWND window, SocketClient* socketClient, HWND mainWindow, std::string callee, std::string myUUID, std::string myEmail);
	~OutgoingCallWindow();
	void startWebview(HWND gWindow);

private:
	HWND hWindow;
};

