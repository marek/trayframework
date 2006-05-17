/////////////////////////////////////////////////////////////////////////////
//
// ccctray.cpp -- Compact Check Count Tray Appliaction
// v0.01
// Written by Marek Kudlacz
// Copyright (c)2005
//
/////////////////////////////////////////////////////////////////////////////

// Headers
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

// Libs
#pragma comment(lib, "comctl32.lib")

// Various consts & Defs
#define	WM_USER_SHELLICON WM_USER + 1
#define WM_TASKBAR_CREATE RegisterWindowMessage(_T("TaskbarCreated"))

// Globals
HWND hWnd;
HINSTANCE hInst;
NOTIFYICONDATA structNID;
BOOL Enabled;

/* ================================================================================================================== */

/*
Name: ... AboutDlgProc(...)
Desc: proccess the about dialog
*/
BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
			return TRUE;
			break;
        case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hwnd, IDOK);
					break;
			}
			break;
        default:
            return FALSE;
    }
    return TRUE;
}

/* ================================================================================================================== */

/*
Name: ... WndProc(...)
Desc: Main hidden "Window" that handles the messaging for our system tray
*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	POINT lpClickPoint;

	if(Message == WM_TASKBAR_CREATE) {			// Taskbar has been recreated (Explorer crashed?)
		// Display tray icon
		if(!Shell_NotifyIcon(NIM_ADD, &structNID)) {
			MessageBox(NULL, "Systray Icon Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
			DestroyWindow(hWnd);
			return -1;
		}
	}

	switch(Message)
	{
		case WM_DESTROY:
			Shell_NotifyIcon(NIM_DELETE, &structNID);	// Remove Tray Item
			PostQuitMessage(0);							// Quit
			break;
		case WM_USER_SHELLICON:			// sys tray icon Messages
			switch(LOWORD(lParam))
			{
				case WM_RBUTTONDOWN:
					{
						HMENU hMenu, hSubMenu;
						// get mouse cursor position x and y as lParam has the Message itself
						GetCursorPos(&lpClickPoint);

						// Load menu resource
						hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_POPUP_MENU));
						if(!hMenu)
							return -1;	// !0, message not successful?

						// Select the first submenu
						hSubMenu = GetSubMenu(hMenu, 0);
						if(!hSubMenu) {
							DestroyMenu(hMenu);        // Be sure to Destroy Menu Before Returning
							return -1;
						}

						// Set Enabled State
						if(Enabled)
							CheckMenuItem(hMenu, ID_POPUP_ENABLE, MF_BYCOMMAND | MF_CHECKED);
						else
							CheckMenuItem(hMenu, ID_POPUP_ENABLE, MF_BYCOMMAND | MF_UNCHECKED);

						// Display menu
						SetForegroundWindow(hWnd);
						TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
						SendMessage(hWnd, WM_NULL, 0, 0);

						// Kill off objects we're done with
						DestroyMenu(hMenu);
					}
					break;
			}
			break;
		case WM_CLOSE:
				DestroyWindow(hWnd);	// Destroy Window
				break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_POPUP_EXIT:
					DestroyWindow(hWnd);		// Destroy Window
					break;
				case ID_POPUP_ABOUT:			// Open about box
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC)AboutDlgProc);
					break;
				case ID_POPUP_ENABLE:			// Toggle Enable
					Enabled = !Enabled;
					break;
			}
			break;
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;		// Return 0 = Message successfully proccessed
}

/* ================================================================================================================== */

/*
Name: ... WinMain(...)
Desc: Main Entry point
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG Msg;
	WNDCLASSEX wc;
	HANDLE hMutexInstance;
	INITCOMMONCONTROLSEX iccex;

	// Check for single instance
	// ------------------------------
	// Note: I recommend to use the GUID Creation Tool for the most unique id
	// Tools->Create GUID for Visual Studio .Net 2003
	// Or search somewhere in the Platform SDK for other environments
	hMutexInstance = CreateMutex(NULL, FALSE,_T("TrayApp-{1EB489D6-6702-43cd-A859-C2BA7DB58B06}"));
	if(GetLastError() == ERROR_ALREADY_EXISTS || GetLastError() == ERROR_ACCESS_DENIED)
		return 0;

	// Copy instance so it can be used globally in other methods
	hInst = hInstance;

	// Init common controls (if you're using them)
	// ------------------------------
	// See: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/commctls/common/structures/initcommoncontrolsex.asp
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_UPDOWN_CLASS | ICC_LISTVIEW_CLASSES;
	if(!InitCommonControlsEx(&iccex)) {
		MessageBox(NULL, "Cannot Initialize Common Controls!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Window "class"
	wc.cbSize =			sizeof(WNDCLASSEX);
	wc.style =			CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc =	WndProc;
	wc.cbClsExtra =		0;
	wc.cbWndExtra =		0;
	wc.hInstance =		hInstance;
	wc.hIcon =			LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_TRAYICON));
	wc.hCursor =		LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground =	(HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName =	NULL;
	wc.lpszClassName =	"Tray Application";
	wc.hIconSm		 =	LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_TRAYICON));
	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Create the hidden window
	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"Tray Application",
		"Tray Application Framework",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);
	if(hWnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// tray icon settings
	structNID.cbSize = sizeof(NOTIFYICONDATA);
	structNID.hWnd = (HWND)hWnd;
	structNID.uID = IDI_TRAYICON;
	structNID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	strcpy(structNID.szTip, "Tray Application Tip");
	structNID.hIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_TRAYICON));
	structNID.uCallbackMessage = WM_USER_SHELLICON;

	// Display tray icon
	if(!Shell_NotifyIcon(NIM_ADD, &structNID)) {
		MessageBox(NULL, "Systray Icon Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Set mode to enabled
	Enabled = TRUE;

	// Message Loop
	while(GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}
