/*
 *  fontview
 *
 *  fontview.c
 *
 *  Copyright (C) 2007  Timo Kreuzer <timo <dot> kreuzer <at> reactos <dot> org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "precomp.h"

#include <winnls.h>
#include <shellapi.h>

#include "fontview.h"
#include "resource.h"

HINSTANCE g_hInstance;
EXTLOGFONTW g_ExtLogFontW;
LPCWSTR g_fileName;

static const WCHAR g_szFontViewClassName[] = L"FontViewWClass";

/* GetFontResourceInfoW is undocumented */
BOOL WINAPI GetFontResourceInfoW(LPCWSTR lpFileName, DWORD *pdwBufSize, void* lpBuffer, DWORD dwType);

DWORD
FormatString(
	DWORD dwFlags,
	HINSTANCE hInstance,
	DWORD dwStringId,
	DWORD dwLanguageId,
	LPWSTR lpBuffer,
	DWORD nSize,
	va_list* Arguments
)
{
	DWORD dwRet;
	int len;
	WCHAR Buffer[1000];

	len = LoadStringW(hInstance, dwStringId, (LPWSTR)Buffer, 1000);

	if (len)
	{
		dwFlags |= FORMAT_MESSAGE_FROM_STRING;
		dwFlags &= ~(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM);
		dwRet = FormatMessageW(dwFlags, Buffer, 0, dwLanguageId, lpBuffer, nSize, Arguments);
		return dwRet;
	}
	return 0;
}

static void
ErrorMsgBox(HWND hParent, DWORD dwCaptionID, DWORD dwMessageId, ...)
{
	HLOCAL hMemCaption = NULL;
	HLOCAL hMemText = NULL;
	va_list args;

	va_start(args, dwMessageId);
	FormatString(FORMAT_MESSAGE_ALLOCATE_BUFFER,
	              NULL, dwMessageId, 0, (LPWSTR)&hMemText, 0, &args);
	va_end(args);

	FormatString(FORMAT_MESSAGE_ALLOCATE_BUFFER,
	              NULL, dwCaptionID, 0, (LPWSTR)&hMemCaption, 0, NULL);

	MessageBoxW(hParent, hMemText, hMemCaption, MB_ICONERROR);

	LocalFree(hMemCaption);
	LocalFree(hMemText);
}

int WINAPI
WinMain (HINSTANCE hThisInstance,
         HINSTANCE hPrevInstance,
         LPSTR lpCmdLine,
         int nCmdShow)
{
	int argc;
	WCHAR** argv;
	WCHAR szFileName[MAX_PATH] = L"";
	DWORD dwSize;
	HWND hMainWnd;
	MSG msg;
	WNDCLASSEXW wincl;
	LPCWSTR fileName;

    switch (GetUserDefaultUILanguage())
    {
    case MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT):
      SetProcessDefaultLayout(LAYOUT_RTL);
      break;

    default:
      break;
    }

	g_hInstance = hThisInstance;

	/* Get unicode command line */
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argc < 2)
	{
		OPENFILENAMEW fontOpen;
		HLOCAL dialogTitle = NULL;

		/* Gets the title for the dialog box ready */
		FormatString(FORMAT_MESSAGE_ALLOCATE_BUFFER,
		          NULL, IDS_OPEN, 0, (LPWSTR)&dialogTitle, 0, NULL);

		/* Clears out any values of fontOpen before we use it */
		ZeroMemory(&fontOpen, sizeof(fontOpen));

		/* Sets up the open dialog box */
		fontOpen.lStructSize = sizeof(fontOpen);
		fontOpen.hwndOwner = NULL;
		fontOpen.lpstrFilter = L"TrueType Font (*.ttf)\0*.ttf\0"
			L"All Files (*.*)\0*.*\0";
		fontOpen.lpstrFile = szFileName;
		fontOpen.lpstrTitle = dialogTitle;
		fontOpen.nMaxFile = MAX_PATH;
		fontOpen.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		fontOpen.lpstrDefExt = L"ttf";

		/* Opens up the Open File dialog box in order to chose a font file. */
		if(GetOpenFileNameW(&fontOpen))
		{
			fileName = fontOpen.lpstrFile;
			g_fileName = fileName;
		} else {
			/* If the user decides to close out of the open dialog effectively
			exiting the program altogether */
			return 0;
		}

		LocalFree(dialogTitle);
	}
	else
	{
		/* Try to add the font resource from command line */
		fileName = argv[1];
		g_fileName = fileName;
	}

	if (!AddFontResourceW(fileName))
	{
		ErrorMsgBox(0, IDS_ERROR, IDS_ERROR_NOFONT, fileName);
		return -1;
	}

	/* Get the font name */
	dwSize = sizeof(g_ExtLogFontW.elfFullName);
	if (!GetFontResourceInfoW(fileName, &dwSize, g_ExtLogFontW.elfFullName, 1))
	{
		ErrorMsgBox(0, IDS_ERROR, IDS_ERROR_NOFONT, fileName);
		return -1;
	}

	dwSize = sizeof(LOGFONTW);
	if (!GetFontResourceInfoW(fileName, &dwSize, &g_ExtLogFontW.elfLogFont, 2))
	{
		ErrorMsgBox(0, IDS_ERROR, IDS_ERROR_NOFONT, fileName);
		return -1;
	}

	if (!Display_InitClass(hThisInstance))
	{
		ErrorMsgBox(0, IDS_ERROR, IDS_ERROR_NOCLASS);
		return -1;
	}

	/* The main window class */
	wincl.cbSize = sizeof (WNDCLASSEXW);
	wincl.style = CS_DBLCLKS;
	wincl.lpfnWndProc = MainWndProc;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hInstance = hThisInstance;
	wincl.hIcon = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TT));
	wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
	wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wincl.lpszMenuName = NULL;
	wincl.lpszClassName = g_szFontViewClassName;
	wincl.hIconSm = LoadIcon (GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TT));

	/* Register the window class, and if it fails quit the program */
	if (!RegisterClassExW (&wincl))
	{
		ErrorMsgBox(0, IDS_ERROR, IDS_ERROR_NOCLASS);
		return 0;
	}

	/* The class is registered, let's create the main window */
	hMainWnd = CreateWindowExW(
				0,						/* Extended possibilites for variation */
				g_szFontViewClassName,	/* Classname */
				g_ExtLogFontW.elfFullName,/* Title Text */
				WS_OVERLAPPEDWINDOW,	/* default window */
				CW_USEDEFAULT,			/* Windows decides the position */
				CW_USEDEFAULT,			/* where the window ends up on the screen */
				544,					/* The programs width */
				375,					/* and height in pixels */
				HWND_DESKTOP,			/* The window is a child-window to desktop */
				NULL,					/* No menu */
				hThisInstance,			/* Program Instance handler */
				NULL					/* No Window Creation data */
			);
	ShowWindow(hMainWnd, nCmdShow);

	/* Main message loop */
	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	RemoveFontResourceW(argv[1]);

	return (int)msg.wParam;
}

static LRESULT
MainWnd_OnCreate(HWND hwnd)
{
	WCHAR szQuit[MAX_BUTTONNAME];
	WCHAR szPrint[MAX_BUTTONNAME];
	WCHAR szString[MAX_STRING];
	HWND hDisplay, hButtonInstall, hButtonPrint;

	/* create the display window */
	hDisplay = CreateWindowExW(
				0,						/* Extended style */
				g_szFontDisplayClassName,	/* Classname */
				L"",				/* Title text */
				WS_CHILD | WS_VSCROLL,	/* Window style */
				0,						/* X-pos */
				HEADER_SIZE,			/* Y-Pos */
				550,					/* Width */
				370-HEADER_SIZE,		/* Height */
				hwnd,					/* Parent */
				(HMENU)IDC_DISPLAY,		/* Identifier */
				g_hInstance,			/* Program Instance handler */
				NULL					/* Window Creation data */
			);

	LoadStringW(g_hInstance, IDS_STRING, szString, MAX_STRING);
	SendMessage(hDisplay, FVM_SETSTRING, 0, (LPARAM)szString);

	/* Init the display window with the font name */
	SendMessage(hDisplay, FVM_SETTYPEFACE, 0, (LPARAM)&g_ExtLogFontW);
	ShowWindow(hDisplay, SW_SHOWNORMAL);

	/* Create the quit button */
	LoadStringW(g_hInstance, IDS_INSTALL, szQuit, MAX_BUTTONNAME);
	hButtonInstall = CreateWindowExW(
				0,						/* Extended style */
				L"button",				/* Classname */
				szQuit,					/* Title text */
				WS_CHILD | WS_VISIBLE,	/* Window style */
				BUTTON_POS_X,			/* X-pos */
				BUTTON_POS_Y,			/* Y-Pos */
				BUTTON_WIDTH,			/* Width */
				BUTTON_HEIGHT,			/* Height */
				hwnd,					/* Parent */
				(HMENU)IDC_INSTALL,		/* Identifier */
				g_hInstance,			/* Program Instance handler */
				NULL					/* Window Creation data */
			);
	SendMessage(hButtonInstall, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	/* Create the print button */
	LoadStringW(g_hInstance, IDS_PRINT, szPrint, MAX_BUTTONNAME);
	hButtonPrint = CreateWindowExW(
				0,						/* Extended style */
				L"button",				/* Classname */
				szPrint,				/* Title text */
				WS_CHILD | WS_VISIBLE,	/* Window style */
				450,					/* X-pos */
				BUTTON_POS_Y,			/* Y-Pos */
				BUTTON_WIDTH,			/* Width */
				BUTTON_HEIGHT,			/* Height */
				hwnd,					/* Parent */
				(HMENU)IDC_PRINT,		/* Identifier */
				g_hInstance,			/* Program Instance handler */
				NULL					/* Window Creation data */
			);
	SendMessage(hButtonPrint, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	return 0;
}

static LRESULT
MainWnd_OnSize(HWND hwnd)
{
	RECT rc;

	GetClientRect(hwnd, &rc);
	MoveWindow(GetDlgItem(hwnd, IDC_PRINT), rc.right - BUTTON_WIDTH - BUTTON_POS_X, BUTTON_POS_Y, BUTTON_WIDTH, BUTTON_HEIGHT, TRUE);
	MoveWindow(GetDlgItem(hwnd, IDC_DISPLAY), 0, HEADER_SIZE, rc.right, rc.bottom - HEADER_SIZE, TRUE);

	return 0;
}

static LRESULT
MainWnd_OnPaint(HWND hwnd)
{
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rc;

	hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rc);
	rc.top = HEADER_SIZE - 2;
	rc.bottom = HEADER_SIZE;
	FillRect(hDC, &rc, GetStockObject(GRAY_BRUSH));
	EndPaint(hwnd, &ps);
	return 0;
}

static LRESULT
MainWnd_OnInstall(HWND hwnd)
{
	DWORD fontExists;

	/* First, we have to find out if the font still exists. */
	fontExists = GetFileAttributes((LPCSTR)g_fileName);
	if (fontExists != 0xFFFFFFFF) /* If the file does not exist */
	{
		ErrorMsgBox(0, IDS_ERROR, IDS_ERROR_NOFONT, g_fileName);
		return -1;
	}

	//CopyFile(g_fileName, NULL, TRUE);

	MessageBox(hwnd, TEXT("This function is unimplemented"), TEXT("Unimplemented"), MB_OK);

	return 0;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_CREATE:
			return MainWnd_OnCreate(hwnd);

		case WM_PAINT:
			return MainWnd_OnPaint(hwnd);

		case WM_SIZE:
			return MainWnd_OnSize(hwnd);

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_INSTALL:
					return MainWnd_OnInstall(hwnd);
					break;

				case IDC_PRINT:
					return Display_OnPrint(hwnd);
					break;
			}
			break;

		case WM_DESTROY:
			PostQuitMessage (0);	/* send a WM_QUIT to the message queue */
			break;

		default:					/* for messages that we don't deal with */
			return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

/* EOF */
