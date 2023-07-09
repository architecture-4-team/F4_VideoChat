#include "IncommingCallWindow.h"
#include "common.h"

HWND g_incomming_handle;
HWND g_incomming_main_handle;
SocketClient* g_incomming_socketClient;
std::string g_caller;

IncommingCallWindow::IncommingCallWindow(HWND window, SocketClient* socket, HWND mainWindow, std::string caller, std::string myUUID, std::string myEmail)
{
	hWindow = window;
	g_incomming_socketClient = socket;
	g_incomming_main_handle = mainWindow;
	g_caller = caller;
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

						webviewInCall->Navigate(L"file:///C:/Users/yongs/Projects/F4_VideoChat/incommingCall.html");

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
								else if (wcscmp(message.get(), L"stop_call") == 0) //on stop calling..
								{
									//todo send server to cancel call
									/*
									json11::Json inviteJson = json11::Json::object{
										{"command", "CANCEL"},
										{"contents", json11::Json::object {
												{"uuid", uuidString},
												{"email", destUserString}
											}
										}
									};
									*/

									SendMessage(g_incomming_handle, WM_CLOSE, 0, 0);
								}
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}

