#include "ContactsListWindow.h"
#include "common.h"

HWND g_contact_handle;
HWND g_contact_main_handle;
SocketClient* g_contact_socketClient;
char buffer[1024];
std::string g_uuid;
std::string g_address_contact;

ContactListWindow::ContactListWindow(HWND window, SocketClient* socket, HWND mainWindow, std::string address)
{
	hWindow = window;
	g_contact_socketClient = socket;
	g_contact_main_handle = mainWindow;
	g_address_contact = address;
};

void ContactListWindow::startWebview(HWND gWindow, std::string uuid)
{
	g_uuid = uuid;
	g_contact_handle = gWindow;
	HWND lWindow = hWindow;
	// show login webview
	CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[lWindow](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

				// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
				env->CreateCoreWebView2Controller(lWindow, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[lWindow](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (controller != nullptr) {
							webviewControllerContact = controller;
							webviewControllerContact->get_CoreWebView2(&webviewContact);
						}

						// Add a few settings for the webview
						// The demo step is redundant since the values are the default settings
						wil::com_ptr<ICoreWebView2Settings> settings;
						webviewContact->get_Settings(&settings);
						settings->put_IsScriptEnabled(TRUE);
						settings->put_AreDefaultScriptDialogsEnabled(TRUE);
						settings->put_IsWebMessageEnabled(TRUE);

						// Resize WebView to fit the bounds of the parent window
						RECT bounds;
						GetClientRect(lWindow, &bounds);
						webviewControllerContact->put_Bounds(bounds);

						TCHAR _buffer[MAX_PATH];
						
						if (g_address_contact == "") // local file
						{
							DWORD res = GetCurrentDirectory(MAX_PATH, _buffer);
							_tcscat_s(_buffer, _T("/contacts.html"));
						}
						else
						{
							g_address_contact = g_address_contact + "/contactList?uuid=" + g_uuid;
							Util::GetInstance().SetStringToTCharBuffer(g_address_contact, _buffer, MAX_PATH);
						}
						webviewContact->Navigate(_buffer);

						EventRegistrationToken token;
						webviewContact->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
						// Schedule an async task to get the document URL
						webviewContact->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
							[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
								LPCWSTR URL = resultObjectAsJson;
								//doSomethingWithURL(URL);
								return S_OK;
							}).Get());
						// </Scripting>
						// <CommunicationHostWeb>
						// Step 6 - Communication between host and web content
						// Set an event handler for the host to return received message back to the web content
						webviewContact->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
							[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
								wil::unique_cotaskmem_string message;
								args->TryGetWebMessageAsString(&message);

								int length = WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, nullptr, 0, nullptr, nullptr);

//								char buffer[1024];
//								char* buffer = new char[length];
								WideCharToMultiByte(CP_UTF8, 0, message.get(), -1, buffer, length, nullptr, nullptr);

								std::string messgeJsonStr(buffer);

								std::string errorMessage;
								json11::Json messageJson = json11::Json::parse(messgeJsonStr, errorMessage);

								//MessageBox(g_contact_handle, message.get(), _T("info"), MB_ICONERROR | MB_OK);

								if (messageJson["action"] == "call") {
									//send event to main window to activate a call window
									//PostMessage(g_contact_main_handle, WM_CONTACT_MESSAGE, 0, (LPARAM)buffer);
									CallService::GetInstance().SendMessageToHandler(WM_CONTACT_MESSAGE, 0, (LPARAM)buffer);
								}
								else if (messageJson["action"] == "join") 
								{
									CallService::GetInstance().SendMessageToHandler(WM_JOIN_MULTICALL_MESSAGE, 0, (LPARAM)buffer);
								}
								SendMessage(g_contact_handle, WM_CLOSE, 0, 0);

//								free(buffer);
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}

