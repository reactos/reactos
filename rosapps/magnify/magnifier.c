/*
 * PROJECT:     ReactOS Magnify
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/magnify/magnifier.c
 * PURPOSE:     
 * COPYRIGHT:   Copyright 2007 Marc Piulachs <marc.piulachs@codexchange.net>
 *
 */

#include <windows.h>
#include <shellapi.h>
#include "magnifier.h"
#include "resource.h"

const TCHAR szWindowClass[] = TEXT("MAGNIFIER");

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
HWND hMainWnd;

TCHAR szTitle[MAX_LOADSTRING];					// The title bar text

#define REPAINT_SPEED	100

HWND hDesktopWindow = NULL;

//Current magnified area
POINT cp;

//Last positions
POINT pMouse;
POINT pCaret;
POINT pFocus;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	AboutProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	OptionsProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WarningProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAGNIFIER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style            = CS_HREDRAW | CS_VREDRAW; 
	wc.lpfnWndProc      = WndProc;
	wc.cbClsExtra       = 0;
	wc.cbWndExtra       = 0;
	wc.hInstance        = hInstance;
	wc.hIcon            = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName     = MAKEINTRESOURCE(IDC_MAGNIFIER);
	wc.lpszClassName    = szWindowClass;

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

	// Create the Window
   hMainWnd = CreateWindowEx(
	    WS_EX_TOPMOST,
		szWindowClass, 
		szTitle, 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		CW_USEDEFAULT, 
		NULL, 
		NULL, 
		hInstance, 
		NULL);

   if (!hMainWnd)
   {
      return FALSE;
   }

   ShowWindow(hMainWnd, nCmdShow);
   UpdateWindow(hMainWnd);

   return TRUE;
}

void Refresh ()
{
	if (!IsIconic(hMainWnd))
	{
		// Invalidate the client area forcing a WM_PAINT message 
		InvalidateRgn(hMainWnd, NULL, TRUE); 
	}
}

void Draw(HDC aDc)
{
	HDC desktopHdc = NULL;
	HDC HdcStrech;
	HANDLE hOld;
	HBITMAP HbmpStrech;

	RECT R;
	RECT appRect;
	DWORD rop = SRCCOPY;
	HCURSOR hCursor;
	CURSORINFO info;

	desktopHdc = GetWindowDC (hDesktopWindow);

	GetClientRect(hMainWnd, &appRect);
	GetWindowRect(hDesktopWindow, &R);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    GetCursorInfo(&info);
    hCursor = info.hCursor;

	 /* Create a memory DC compatible with client area DC.*/ 
	HdcStrech = CreateCompatibleDC(desktopHdc); 
	   
	/* Create a bitmap compatible with the client area DC.*/ 
	HbmpStrech = CreateCompatibleBitmap(
		desktopHdc,
		R.right,
		R.bottom); 

	/* Select our bitmap in memory DC and save the old one.*/ 
	hOld = SelectObject (HdcStrech , HbmpStrech);

	/* Paint the screen bitmap to our in memory DC */
	BitBlt(
		HdcStrech,
		0,
		0,
		R.right,
		R.bottom,
		desktopHdc,
		0,
		0,
		SRCCOPY);

	/* Draw the mouse pointer in the right position */
	DrawIcon(
		HdcStrech , 
		pMouse.x - 10, 
		pMouse.y - 10, 
		hCursor);

	int Width = (R.right - R.left);
	int Height = (R.bottom - R.top);

	int AppWidth = (appRect.right - appRect.left);
	int AppHeight = (appRect.bottom - appRect.top);

	LONG blitAreaWidth = AppWidth / iZoom;
	LONG blitAreaHeight = AppHeight / iZoom;

	LONG blitAreaX = (cp.x) - (blitAreaWidth /2);
	LONG blitAreaY = (cp.y) - (blitAreaHeight /2);

	if (blitAreaX < 0)
	{
	   blitAreaX = 0;
	}

	if (blitAreaY < 0)
	{
	   blitAreaY = 0;
	}

	if (blitAreaX > (Width - blitAreaWidth))
	{
		blitAreaX = (Width - blitAreaWidth);
	}

	if (blitAreaY > (Height - blitAreaHeight))
	{
		blitAreaY = (Height - blitAreaHeight);
	}

	if (bInvertColors)
	{
		rop = NOTSRCCOPY;
	}

	StretchBlt(
		HdcStrech,
		0,
		0,
		AppWidth,
		AppHeight,
		HdcStrech,
		blitAreaX,
		blitAreaY,
		blitAreaWidth,
		blitAreaHeight,
		rop);

	/* Blast the image from memory DC to client DC.*/ 
	BitBlt (
		aDc,
		0 , 
		0 , 
		AppWidth ,
		AppHeight ,
		HdcStrech ,
		0 ,
		0 , 
		SRCCOPY);

	/* Cleanup.*/ 
	SelectObject (HdcStrech, hOld);
	DeleteObject (HbmpStrech);
	DeleteDC (HdcStrech);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
		case WM_TIMER:
		{
			POINT pNewMouse;
			POINT pNewCaret;
			POINT pNewFocus;

			//Get current mouse position 
			GetCursorPos (&pNewMouse);

			//Get caret position
			HWND hwnd1 = GetForegroundWindow (); 
			DWORD a = GetWindowThreadProcessId(hwnd1, NULL); 
			DWORD b = GetCurrentThreadId(); 
			AttachThreadInput (a, b, TRUE); 
			HWND hwnd2 = GetFocus(); 

			GetCaretPos( &pNewCaret); 
			ClientToScreen (hwnd2, (LPPOINT) &pNewCaret); 
			AttachThreadInput (a, b, FALSE); 

			//Get current control focus
			HWND hwnd3 = GetFocus ();
			RECT controlRect;
			GetWindowRect (hwnd3 , &controlRect);
			pNewFocus.x = controlRect.left;
			pNewFocus.y = controlRect.top;

			//If mouse has moved ....
			if (((pMouse.x != pNewMouse.x) || (pMouse.y != pNewMouse.y)) && bFollowMouse)
			{
				//Update to new position
				pMouse = pNewMouse;
				cp = pNewMouse;
				Refresh();
			}
			else if (((pCaret.x != pNewCaret.x) || (pCaret.y != pNewCaret.y)) && bFollowCaret)
			{
				//Update to new position
				pCaret = pNewCaret;
				cp = pNewCaret;
				Refresh();
			}
			else if (((pFocus.x != pNewFocus.x) || (pFocus.y != pNewFocus.y)) && bFollowFocus)
			{
				//Update to new position
				pFocus = pNewFocus;
				cp = pNewFocus;
				Refresh();
			}
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
			case IDM_OPTIONS:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOGOPTIONS), hWnd, (DLGPROC)OptionsProc);
				break;
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)AboutProc);
				break;
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
   case WM_PAINT:
		{
			PAINTSTRUCT PaintStruct;
			HDC dc;
			dc = BeginPaint(hWnd, &PaintStruct);
			Draw(dc);
			EndPaint(hWnd, &PaintStruct);
		}
      break;
   case WM_ERASEBKGND:
		//handle WM_ERASEBKGND by simply returning non-zero because we did all the drawing in WM_PAINT. 
	   break;
	case WM_DESTROY:
		//Save settings to registry
		SaveSettings ();
		KillTimer (hWnd , 1);
		PostQuitMessage(0);
		break;
	case WM_CREATE:
			//Load settings from registry
			LoadSettings ();

			//Get the desktop window
			hDesktopWindow = GetDesktopWindow();
			
			if (bShowWarning)
			{
				DialogBox (hInst, MAKEINTRESOURCE(IDD_WARNINGDIALOG), hWnd, (DLGPROC)WarningProc);
			}

			if (bStartMinimized)
			{
				ShowWindow (hMainWnd, SW_MINIMIZE );
			}
			
			//Set the timer 
			SetTimer (hWnd , 1, REPAINT_SPEED , NULL);
			break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for options box.
INT_PTR CALLBACK OptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
		{
			//Add the zoom items....
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("1"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("2"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("3"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("4"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("5"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("6"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("7"));
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_ADDSTRING, 0, (LPARAM)("8"));

			//
			SendDlgItemMessage(hDlg, IDC_ZOOM, CB_SETCURSEL, iZoom - 1, 0);

			if (bFollowMouse)
				SendDlgItemMessage(hDlg,IDC_FOLLOWMOUSECHECK,BM_SETCHECK , wParam ,0);

			if (bFollowFocus)
				SendDlgItemMessage(hDlg,IDC_FOLLOWKEYBOARDCHECK,BM_SETCHECK , wParam ,0);

			if (bFollowCaret)
				SendDlgItemMessage(hDlg,IDC_FOLLOWTEXTEDITINGCHECK,BM_SETCHECK , wParam ,0);

			if (bInvertColors)
				SendDlgItemMessage(hDlg,IDC_INVERTCOLORSCHECK,BM_SETCHECK , wParam ,0);

			if (bStartMinimized)
				SendDlgItemMessage(hDlg,IDC_STARTMINIMIZEDCHECK,BM_SETCHECK , wParam ,0);

			if (bShowMagnifier)
				SendDlgItemMessage(hDlg,IDC_SHOWMAGNIFIERCHECK,BM_SETCHECK , wParam ,0);

			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDOK)
		{
		}
		if (LOWORD(wParam) == IDHELP)
		{
			MessageBox(hDlg , TEXT("Magnifier help not available yet!") , TEXT("Help") , MB_OK);
		}
		switch(LOWORD(wParam))
		{
            case IDC_ZOOM:
				if(HIWORD(wParam) == CBN_SELCHANGE) 
				{ 
					HWND hCombo = GetDlgItem(hDlg,IDC_ZOOM);

					/* Get index of current selection and the text of that selection. */ 
					iZoom = SendMessage( hCombo, CB_GETCURSEL, (WPARAM) wParam, (LPARAM) lParam ) + 1;

					//Update the magnigier UI
					Refresh ();
				}
				break;
			case IDC_INVERTCOLORSCHECK:
				bInvertColors = IsDlgButtonChecked (hDlg, IDC_INVERTCOLORSCHECK);
				Refresh ();
				break;
			case IDC_FOLLOWMOUSECHECK:
				bFollowMouse = IsDlgButtonChecked (hDlg, IDC_FOLLOWMOUSECHECK);
				break;
			case IDC_FOLLOWKEYBOARDCHECK:
				bFollowFocus = IsDlgButtonChecked (hDlg, IDC_FOLLOWKEYBOARDCHECK);
				break;
			case IDC_FOLLOWTEXTEDITINGCHECK:
				bFollowCaret = IsDlgButtonChecked (hDlg, IDC_FOLLOWTEXTEDITINGCHECK);
				break;
			case IDC_STARTMINIMIZEDCHECK:
				bStartMinimized = IsDlgButtonChecked (hDlg, IDC_STARTMINIMIZEDCHECK);
				break;
			case IDC_SHOWMAGNIFIER:
				bShowMagnifier = IsDlgButtonChecked (hDlg, IDC_SHOWMAGNIFIERCHECK);
				if (bShowMagnifier){
					ShowWindow (hMainWnd , SW_SHOW);
				}else{
					ShowWindow (hMainWnd , SW_HIDE);
				}
				break;
		}
	}
	return (INT_PTR)FALSE;
}

// Message handler for warning box.
INT_PTR CALLBACK WarningProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_SHOWWARNINGCHECK:
					bShowWarning = !IsDlgButtonChecked (hDlg, IDC_SHOWWARNINGCHECK);
					break;
			}
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}
