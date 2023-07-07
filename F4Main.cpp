#include "SocketCommunication.h"
#include <Windows.h>
#include <tchar.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <stdio.h>
#include <iostream>
#include "WebView2.h"
#include "resource.h"

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

SocketCommunication* socketComm = new SocketCommunication(std::string("127.0.0.1"), 10000);

HWND g_childWindow;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hMainWindow;
	HWND hChildWindow;

	socketComm->Start();

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
		0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	
	if (!hMainWindow)
	{
		MessageBox(nullptr, _T("Failed to create main window."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create a button
	HWND hButton = CreateWindowEx(0, _T("BUTTON"), _T("Contact List"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		10, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(1), hInstance, nullptr);
	if (!hButton)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create a button
	HWND hButtonCall = CreateWindowEx(0, _T("BUTTON"), _T("Call Menu"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		120, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(2), hInstance, nullptr);
	if (!hButtonCall)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	// Create a button
	HWND hButtonLogin = CreateWindowEx(0, _T("BUTTON"), _T("Login"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		230, 10, 100, 30, hMainWindow, reinterpret_cast<HMENU>(3), hInstance, nullptr);
	if (!hButtonCall)
	{
		MessageBox(nullptr, _T("Failed to create button."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}


	// Create the child window
	hChildWindow = CreateWindowEx(0, _T("ChildWindowClass"), _T("Login Window"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 600, hMainWindow, nullptr, hInstance, nullptr);

	if (!hChildWindow)
	{
		MessageBox(hMainWindow, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}
	g_childWindow = hChildWindow;

	// Show and update the main window
	ShowWindow(hMainWindow, SW_MAXIMIZE);
	UpdateWindow(hMainWindow);

	// Show and update the main window
	ShowWindow(hChildWindow, nCmdShow);
	UpdateWindow(hChildWindow);

	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[hChildWindow](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(hChildWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[hChildWindow](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewController = controller;
							webviewController->get_CoreWebView2(&webview);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webview->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(hChildWindow, &bounds);
						webviewController->put_Bounds(bounds);

						// Schedule an async task to navigate to Bing
						//webview->Navigate(L"http://www.google.com");
						webview->Navigate(L"file:///C:/Users/yongs/Projects/F4_VideoChat/login.html");
						
						EventRegistrationToken token;
						// <NavigationEvents>
						// Step 4 - Navigation events
						// register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
/*
						webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string uri;
								args->get_Uri(&uri);
								std::wstring source(uri.get());
								if (source.substr(0, 5) != L"https") {
									args->put_Cancel(true);
								}
								return S_OK;
							}).Get(), &token);
*/
						// </NavigationEvents>

						// <Scripting>
						// Step 5 - Scripting
						// Schedule an async task to add initialization script that freezes the Object object
						webview->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						// Schedule an async task to get the document URL
						webview->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								LPCWSTR URL = resultObjectAsJson;
								//doSomethingWithURL(URL);
								return S_OK;
							}).Get());
						// </Scripting>

						// <CommunicationHostWeb>
						// Step 6 - Communication between host and web content
						// Set an event handler for the host to return received message back to the web content
						webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);
								//processMessage(&message);
								MessageBox(g_childWindow, message.get(), _T("Error"), MB_ICONERROR | MB_OK);
								SendMessage(g_childWindow, WM_CLOSE, 0, 0);
								webview->PostWebMessageAsString(message.get());
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());

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
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1) 
		{
			HWND hChildWnd2 = CreateWindowEx(0, _T("ChildWindowClass"), _T("Contact List"), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, g_hInstance, nullptr);
			if (!hChildWnd2)
			{
				MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
				return 1;
			}

			// Show the child window
			ShowWindow(hChildWnd2, SW_SHOW);
			UpdateWindow(hChildWnd2);
		}
		else if (LOWORD(wParam) == 2)
		{
			HWND hChildWnd2 = CreateWindowEx(0, _T("ChildWindowClass"), _T("Call Menu"), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, 800, 200, nullptr, nullptr, g_hInstance, nullptr);
			if (!hChildWnd2)
			{
				MessageBox(hWnd, _T("Failed to create child window."), _T("Error"), MB_ICONERROR | MB_OK);
				return 1;
			}

			// Show the child window
			ShowWindow(hChildWnd2, SW_SHOW);
			UpdateWindow(hChildWnd2);

			CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
				Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
					[hChildWnd2](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

						// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
						env->CreateCoreWebView2Controller(hChildWnd2, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
							[hChildWnd2](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
								if (controller != nullptr) {
									webviewController = controller;
									webviewController->get_CoreWebView2(&webview);
								}

								// Add a few settings for the webview
								// The demo step is redundant since the values are the default settings
								wil::com_ptr<ICoreWebView2Settings> settings;
								webview->get_Settings(&settings);
								settings->put_IsScriptEnabled(TRUE);
								settings->put_AreDefaultScriptDialogsEnabled(TRUE);
								settings->put_IsWebMessageEnabled(TRUE);

								// Resize WebView to fit the bounds of the parent window
								RECT bounds;
								GetClientRect(hChildWnd2, &bounds);
								webviewController->put_Bounds(bounds);

								// Schedule an async task to navigate to Bing
								webview->Navigate(L"file:///C:/Users/yongs/Projects/callMenu/call.html");

								EventRegistrationToken token;
								webview->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
								webview->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
									[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
										LPCWSTR URL = resultObjectAsJson;
										//doSomethingWithURL(URL);
										return S_OK;
									}).Get());
								// </Scripting>

								// <CommunicationHostWeb>
								// Step 6 - Communication between host and web content
								// Set an event handler for the host to return received message back to the web content
								webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
									[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
										wil::unique_cotaskmem_string message;
										args->TryGetWebMessageAsString(&message);
										//processMessage(&message);
										//webview->PostWebMessageAsJson(L"{\"CallState\": \"success\"}");
										std::wstring wstr(message.get());
										// Convert std::wstring to std::string
										std::string sstr(wstr.begin(), wstr.end());

										if (sstr == "call_start") {
											socketComm->SendMessageW("{\"command\": \"INVITE\",\"contents\" : {\"uuid\": \"src_uuid\",\"target\" : \"dst_contact_id\"}}");
										}

										return S_OK;
									}).Get(), &token);

								return S_OK;
							}).Get());
						return S_OK;
					}).Get());
		}
		else if (LOWORD(wParam) == 3) {
			socketComm->SendMessageW(std::string("{\"command\": \"LOGIN\",\"contents\" : {\"email\" : \"aaa@gmail.com\",\"password\" : \"1234\"}}"));
			std::string res = socketComm->ReceiveResponse();
			
			std::wstring wstr(res.begin(), res.end());
			MessageBox(hWnd, wstr.c_str(), _T("Error"), MB_ICONERROR | MB_OK);
			
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