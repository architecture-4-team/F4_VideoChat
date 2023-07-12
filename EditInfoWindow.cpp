#include "EditInfoWindow.h"
#include "common.h"

HWND g_editInfo_handle;
HWND g_editInfo_main_handle;
SocketClient* g_editInfo_socketClient;
char bufferEditInfo[1024];
std::string g_editInfo_uuid;
std::string g_editInfo_address;

EditInfoWindow::EditInfoWindow(HWND window, SocketClient* socket, HWND mainWindow, std::string address)
{
	hWindow = window;
	g_editInfo_socketClient = socket;
	g_editInfo_main_handle = mainWindow;
	g_editInfo_address = address;
};

void EditInfoWindow::startWebview(HWND gWindow, std::string uuid)
{
	g_editInfo_uuid = uuid;
	g_editInfo_handle = gWindow;
	HWND lWindow = hWindow;
	// show login webview
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[lWindow](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window g_editInfo_handle
				env->CreateCoreWebView2Controller(lWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[lWindow](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewControllerEditInfo = controller;
							webviewControllerEditInfo->get_CoreWebView2(&webviewEditInfo);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webviewEditInfo->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(lWindow, &bounds);
						webviewControllerEditInfo->put_Bounds(bounds);

						TCHAR _buffer[MAX_PATH];

						if (g_editInfo_address == "") // local file
						{
							DWORD res = GetCurrentDirectory(MAX_PATH, _buffer);
							_tcscat_s(_buffer, _T("/editInfo.html"));
						}
						else
						{
							g_editInfo_address = g_editInfo_address + "/changeMyInfo?uuid=" + g_editInfo_uuid;
							Util::GetInstance().SetStringToTCharBuffer(g_editInfo_address, _buffer, MAX_PATH);
						}
						webviewEditInfo->Navigate(_buffer);

						EventRegistrationToken token;
						webviewEditInfo->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						// Schedule an async task to get the document URL
						webviewEditInfo->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								LPCWSTR URL = resultObjectAsJson;
								//doSomethingWithURL(URL);
								return S_OK;
							}).Get());
						// </Scripting>
						// <CommunicationHostWeb>
						// Step 6 - Communication between host and web content
						// Set an event handler for the host to return received message back to the web content
						webviewEditInfo->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);

								int length = WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, nullptr, 0, nullptr, nullptr);

								//								char buffer[1024];
								//								char* buffer = new char[length];
								WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, bufferEditInfo, length, nullptr, nullptr);

								std::string messgeJsonStr(bufferEditInfo);

								std::string errorMessage;
								json11::Json messageJson = json11::Json::parse(messgeJsonStr, errorMessage);

								//MessageBox(g_editInfo_handle, message.get(), _T("info"), MB_ICONERROR | MB_OK);

								if (messageJson["action"] == "close") {
									SendMessage(g_editInfo_handle, WM_CLOSE, 0, 0);
								}
								else
								{
									//toDo, the other action ??
								}

								//								free(buffer);
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}

