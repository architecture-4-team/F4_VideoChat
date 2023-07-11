#include "IncommingCallWindow.h"

HWND g_incomming_handle;
HWND g_incomming_main_handle;
SocketClient* g_incomming_socketClient;
std::string g_caller;
std::string g_in_uuid;
std::string g_in_callId;
char strParam[1024];

IncommingCallWindow::IncommingCallWindow(HWND window, SocketClient* socket, HWND mainWindow, std::string caller, std::string myUUID, std::string myEmail, std::string callId)
{
	hWindow = window;
	g_incomming_socketClient = socket;
	g_incomming_main_handle = mainWindow;
	g_caller = caller;
	g_in_uuid = myUUID;
	g_in_callId = callId;
};

void IncommingCallWindow::startWebview(HWND gWindow)
{
	g_incomming_handle = gWindow;
	HWND lWindow = hWindow;
	// show login webview
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[lWindow](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(lWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[lWindow](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewControllerInCall = controller;
							webviewControllerInCall->get_CoreWebView2(&webviewInCall);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webviewInCall->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(lWindow, &bounds);
						webviewControllerInCall->put_Bounds(bounds);

						TCHAR _buffer[MAX_PATH];
						DWORD res = GetCurrentDirectory(MAX_PATH, _buffer);
						_tcscat_s(_buffer, _T("/incommingCall.html"));
						webviewInCall->Navigate(_buffer);

						EventRegistrationToken token;
						webviewInCall->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						// Schedule an async task to get the document URL
						webviewInCall->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								LPCWSTR URL = resultObjectAsJson;
								//doSomethingWithURL(URL);

								return S_OK;
							}).Get());

						// </Scripting>
						// <CommunicationHostWeb>
						// Step 6 - Communication between host and web content
						// Set an event handler for the host to return received message back to the web content
						webviewInCall->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);

								if (wcscmp(message.get(), L"complete_loading") == 0) {

									//MessageBox(g_incomming_handle, message.get(), _T("info"), MB_ICONERROR | MB_OK);

									json11::Json callJson;

									callJson = json11::Json::object{
										{"sound", "start"},
										{"email", g_caller},
									};

									std::string callJsonStr = callJson.dump();
									std::wstring wsCallJsonStr(callJsonStr.begin(), callJsonStr.end());

									webviewInCall->PostWebMessageAsString(wsCallJsonStr.c_str());

								}
								else if (wcscmp(message.get(), L"accept_call") == 0) //accept call
								{
									json11::Json acceptJson = json11::Json::object{
										{"command", "ACCEPT"},
										{"contents", json11::Json::object {
												{"uuid", g_in_uuid},
												{"callid", g_in_callId}
											}
										}
									};

									g_incomming_socketClient->SendMessageW(acceptJson.dump());

									std::string acceptJsonStr = acceptJson.dump();
									acceptJsonStr.copy(strParam, sizeof(strParam) - 1);
									//PostMessage(g_incomming_main_handle, WM_ACCEPT_INCOMMING_CALL_MESSAGE, 0, (LPARAM)strParam);
									CallService::GetInstance().SendMessageToHandler(WM_ACCEPT_INCOMMING_CALL_MESSAGE, 0, (LPARAM)strParam);
									SendMessage(g_incomming_handle, WM_CLOSE, 0, 0);
								}
								else if (wcscmp(message.get(), L"reject_call") == 0) //reject call
								{
									json11::Json rejectJson = json11::Json::object{
										{"command", "CANCEL"},
										{"response", "REJECT"},
										{"contents", json11::Json::object {
												{"uuid", g_in_uuid},
												{"callid", g_in_callId}
											}
										}
									};
									g_incomming_socketClient->SendMessageW(rejectJson.dump());

									SendMessage(g_incomming_handle, WM_CLOSE, 0, 0);
								}
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}

