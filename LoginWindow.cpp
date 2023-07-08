#include "LoginWindow.h"

HWND g_handle;

LoginWindow::LoginWindow(HWND window) 
{
	hWindow = window;
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

						// Schedule an async task to navigate to Bing
						//webview->Navigate(L"http://www.google.com");
						webview2->Navigate(L"file:///C:/Users/yongs/Projects/F4_VideoChat/login.html");

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

								// ToDo
								// receive uuid from login window and save it to uuid variable

								MessageBox(g_handle, message.get(), _T("Error"), MB_ICONERROR | MB_OK);
								SendMessage(g_handle, WM_CLOSE, 0, 0);
								webview->PostWebMessageAsString(message.get());
								return S_OK;
							}).Get(), &token);

						return S_OK;
					}).Get());
				return S_OK;
			}).Get());
}
	
