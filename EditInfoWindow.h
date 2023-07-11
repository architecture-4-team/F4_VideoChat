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
#include "CallService.h"
#include "Util.h"

using namespace Microsoft::WRL;
// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewControllerEditInfo;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webviewEditInfo;


class EditInfoWindow
{
public:
	EditInfoWindow(HWND window, SocketClient* socketClient, HWND mainWindow, std::string address);
	~EditInfoWindow();
	void startWebview(HWND gWindow, std::string uuid);

private:
	HWND hWindow;
	std::string g_address;
};

