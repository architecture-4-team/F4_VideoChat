#include "OutGoingCallWindow.h"
#include "common.h"

HWND g_outcall_handle;
HWND g_outcall_main_handle;
SocketClient* g_outcall_socketClient;
std::string g_callee;

OutgoingCallWindow::OutgoingCallWindow(HWND window, SocketClient* socket, HWND mainWindow, std::string callee, std::string myUUID, std::string myEmail)
{
	hWindow = window;
	g_outcall_socketClient = socket;
	g_outcall_main_handle = mainWindow;
	g_callee = callee;
};

void OutgoingCallWindow::startWebview(HWND gWindow)
{
	g_outcall_handle = gWindow;
	HWND lWindow = hWindow;
	// show login webview
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[lWindow](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(lWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[lWindow](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewControllerOutCall = controller;
							webviewControllerOutCall->get_CoreWebView2(&webviewOutCall);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webviewOutCall->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(lWindow, &bounds);
						webviewControllerOutCall->put_Bounds(bounds);

						TCHAR _buffer[MAX_PATH];
						DWORD res = GetCurrentDirectory(MAX_PATH, _buffer);
						_tcscat_s(_buffer, _T("/outgoingCall.html"));
						webviewOutCall->Navigate(_buffer);

						EventRegistrationToken token;
						webviewOutCall->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						// Schedule an async task to get the document URL
						webviewOutCall->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								LPCWSTR URL = resultObjectAsJson;
								//doSomethingWithURL(URL);

								return S_OK;
							}).Get());

						// </Scripting>
						// <CommunicationHostWeb>
						// Step 6 - Communication between host and web content
						// Set an event handler for the host to return received message back to the web content
						webviewOutCall->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);

								if (wcscmp(message.get(), L"complete_loading") == 0) {

									//MessageBox(g_outcall_handle, message.get(), _T("info"), MB_ICONERROR | MB_OK);

									json11::Json callJson;

									callJson = json11::Json::object{
										{"sound", "start"},
										{"email", g_callee},
									};

									std::string callJsonStr = callJson.dump();
									std::wstring wsCallJsonStr(callJsonStr.begin(), callJsonStr.end());
									
									webviewOutCall->PostWebMessageAsString(wsCallJsonStr.c_str());

								}
								else if (wcscmp(message.get(), L"stop_call") == 0) //on stop calling..
								{
									// todo, we don't make this process. call stop while calling..
									/*
									json11::Json byeJson = json11::Json::object{
										{"command", "BYE"},
										{"contents", json11::Json::object {
												{"uuid", g_out_uuid},
												{"callid", g_out_callId}
											}
										}
									};
									g_outcall_socketClient->SendMessageW(byeJson.dump());
									*/
									SendMessage(g_outcall_handle, WM_CLOSE, 0, 0);
								}
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}

