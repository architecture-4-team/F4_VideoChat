#include "LoginWindow.h"
#include "common.h"

HWND g_handle;
HWND g_mainHandle;
SocketClient* g_socketClient;
char loginBuffer[1024];

LoginWindow::LoginWindow(HWND window, SocketClient* socket, HWND mainWindow)
{
	hWindow = window;
	g_socketClient = socket;
	g_mainHandle = mainWindow;
};

void LoginWindow::startWebview(HWND gWindow)
{
	g_handle = gWindow;
	HWND lWindow = hWindow;
	// show login webview
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[lWindow](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(lWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[lWindow](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewController2 = controller;
							webviewController2->get_CoreWebView2(&webview2);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webview2->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(lWindow, &bounds);
						webviewController2->put_Bounds(bounds);

						TCHAR buffer[MAX_PATH];
						DWORD res = GetCurrentDirectory(MAX_PATH, buffer);
						_tcscat_s(buffer, _T("/login.html"));

						webview2->Navigate(buffer);

						EventRegistrationToken token;
						webview2->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						// Schedule an async task to get the document URL
						webview2->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								LPCWSTR URL = resultObjectAsJson;
								//doSomethingWithURL(URL);
								return S_OK;
							}).Get());
						// </Scripting>
						// <CommunicationHostWeb>
						// Step 6 - Communication between host and web content
						// Set an event handler for the host to return received message back to the web content
						webview2->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);


								//processMessage(&message);
								json11::Json loginJson;

								if (wcscmp(message.get(), L"loginA_cliked") == 0) {
									loginJson = json11::Json::object{
										{"command", "LOGIN"},
										{"contents",json11::Json::object{
												{"email", "aaa@gmail.com"},
												{"password", "1234"}
											}
										}
									};

									std::string loginJsonStr = loginJson.dump();
									g_socketClient->SendMessageW(loginJsonStr);
								}
								else if (wcscmp(message.get(), L"loginB_cliked") == 0)
								{
									loginJson = json11::Json::object{
										{"command", "LOGIN"},
										{"contents",json11::Json::object{
												{"email", "bbb@gmail.com"},
												{"password", "1234"}
											}
										}
									};
									std::string loginJsonStr = loginJson.dump();
									g_socketClient->SendMessageW(loginJsonStr);
								}
								else
								{
									int length = WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, nullptr, 0, nullptr, nullptr);
									WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, loginBuffer, length, nullptr, nullptr);
									std::string messgeJsonStr(loginBuffer);
									std::string errorMessage;
									json11::Json messageJson = json11::Json::parse(messgeJsonStr, errorMessage);

									MessageBox(g_handle, message.get(), _T("info"), MB_ICONERROR | MB_OK);

									if (messageJson["action"] == "login_complete") {
										//send event to main window to activate a call window
										//PostMessage(g_mainHandle, WM_LOGON_COMPLETED_MESSAGE, 0, (LPARAM)loginBuffer);
										CallService::GetInstance().SendMessageToHandler(WM_LOGON_COMPLETED_MESSAGE, 0, (LPARAM)loginBuffer);
									}
									else
									{
										//toDo, the other action ??
									}
									SendMessage(g_handle, WM_CLOSE, 0, 0);
								}

								// ToDo
								// receive uuid from login window and save it to uuid variable

								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}
	
