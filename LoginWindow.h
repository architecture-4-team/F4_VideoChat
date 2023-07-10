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

using namespace Microsoft::WRL;
// Pointer to WebViewController
static wil::com_ptr<ICoreWebView2Controller> webviewController2;

// Pointer to WebView window
static wil::com_ptr<ICoreWebView2> webview2;


class LoginWindow
{
public:
	LoginWindow(HWND window, SocketClient* socketClient, HWND mainWindow);
	~LoginWindow();
	void startWebview(HWND gWindow);

private:
	HWND hWindow;
};

