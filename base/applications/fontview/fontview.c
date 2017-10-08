/*
 *  fontview
 *
 *  fontview.c
 *
 *  Copyright (C) 2007  Timo Kreuzer <timo <dot> kreuzer <at> reactos <dot> org>
 *  Copyright (C) 2016-2017  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
#include <windowsx.h>
#include <winreg.h>

#include "fontview.h"
#include "resource.h"

HINSTANCE g_hInstance;
INT g_FontIndex = 0;
INT g_NumFonts = 0;
LOGFONTW g_LogFonts[64];
LPCWSTR g_fileName;
WCHAR g_FontTitle[1024] = L"";
BOOL g_FontPrint = FALSE;

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
ErrorMsgBox(HWND hParent, DWORD dwMessageId, ...)
{
	HLOCAL hMemCaption = NULL;
	HLOCAL hMemText = NULL;
	va_list args;

	va_start(args, dwMessageId);
	FormatString(FORMAT_MESSAGE_ALLOCATE_BUFFER,
	              NULL, dwMessageId, 0, (LPWSTR)&hMemText, 0, &args);
	va_end(args);

	FormatString(FORMAT_MESSAGE_ALLOCATE_BUFFER,
	              NULL, IDS_ERROR, 0, (LPWSTR)&hMemCaption, 0, NULL);

	MessageBoxW(hParent, hMemText, hMemCaption, MB_ICONERROR);

	LocalFree(hMemCaption);
	LocalFree(hMemText);
}

int WINAPI
wWinMain(HINSTANCE hThisInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nCmdShow)
{
	int argc;
	INT i;
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
        WCHAR filter[MAX_PATH*2], dialogTitle[MAX_PATH];

		LoadStringW(NULL, IDS_OPEN, dialogTitle, ARRAYSIZE(dialogTitle));
		LoadStringW(NULL, IDS_FILTER_LIST, filter, ARRAYSIZE(filter));

		/* Clears out any values of fontOpen before we use it */
		ZeroMemory(&fontOpen, sizeof(fontOpen));

		/* Sets up the open dialog box */
		fontOpen.lStructSize = sizeof(fontOpen);
		fontOpen.hwndOwner = NULL;
		fontOpen.lpstrFilter = filter;
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
	}
	else
	{
		/* Try to add the font resource from command line */
//		fileName = argv[1];
		if (argc == 2)
		{
			fileName = argv[1];
		}
		else
		{
			/* Windows fontview supports the /p option, which displays print dialog */
			fileName = argv[2];
			if (wcscmp(argv[1], L"/p") == 0)
			{
				g_FontPrint = TRUE;
			}
			else
			{
				fileName = argv[1];
			}
		}
		g_fileName = fileName;
	}

	if (!AddFontResourceW(fileName))
	{
		ErrorMsgBox(0, IDS_ERROR_NOFONT, fileName);
		return -1;
	}

	/* Get the font name */
	dwSize = sizeof(g_LogFonts);
	ZeroMemory(g_LogFonts, sizeof(g_LogFonts));
	if (!GetFontResourceInfoW(fileName, &dwSize, g_LogFonts, 2))
	{
		ErrorMsgBox(0, IDS_ERROR_NOFONT, fileName);
		return -1;
	}
	g_NumFonts = 0;
	for (i = 0; i < ARRAYSIZE(g_LogFonts); ++i)
	{
		if (g_LogFonts[i].lfFaceName[0] == 0)
			break;

		++g_NumFonts;
	}
	if (g_NumFonts == 0)
	{
		ErrorMsgBox(0, IDS_ERROR_NOFONT, fileName);
		return -1;
	}

	/* get font title */
	dwSize = sizeof(g_FontTitle);
	ZeroMemory(g_FontTitle, sizeof(g_FontTitle));
	GetFontResourceInfoW(fileName, &dwSize, g_FontTitle, 1);

	if (!Display_InitClass(hThisInstance))
	{
		ErrorMsgBox(0, IDS_ERROR_NOCLASS);
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
		ErrorMsgBox(0, IDS_ERROR_NOCLASS);
		return 0;
	}

	/* The class is registered, let's create the main window */
	hMainWnd = CreateWindowExW(
				0,						/* Extended possibilities for variation */
				g_szFontViewClassName,	/* Classname */
				g_FontTitle,			/* Title Text */
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
		if (IsDialogMessage(hMainWnd, &msg))
			continue;
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
	WCHAR szPrevious[MAX_STRING];
	WCHAR szNext[MAX_STRING];
	HWND hDisplay, hButtonInstall, hButtonPrint, hButtonPrev, hButtonNext;

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

	/* Create the install button */
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

	/* Create the previous button */
	LoadStringW(g_hInstance, IDS_PREVIOUS, szPrevious, MAX_BUTTONNAME);
	hButtonPrev = CreateWindowExW(
				0,						/* Extended style */
				L"button",				/* Classname */
				szPrevious,				/* Title text */
				WS_CHILD | WS_VISIBLE,	/* Window style */
				450,					/* X-pos */
				BUTTON_POS_Y,			/* Y-Pos */
				BUTTON_WIDTH,			/* Width */
				BUTTON_HEIGHT,			/* Height */
				hwnd,					/* Parent */
				(HMENU)IDC_PREV,		/* Identifier */
				g_hInstance,			/* Program Instance handler */
				NULL					/* Window Creation data */
			);
	SendMessage(hButtonPrev, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	/* Create the next button */
	LoadStringW(g_hInstance, IDS_NEXT, szNext, MAX_BUTTONNAME);
	hButtonNext = CreateWindowExW(
				0,						/* Extended style */
				L"button",				/* Classname */
				szNext,					/* Title text */
				WS_CHILD | WS_VISIBLE,	/* Window style */
				450,					/* X-pos */
				BUTTON_POS_Y,			/* Y-Pos */
				BUTTON_WIDTH,			/* Width */
				BUTTON_HEIGHT,			/* Height */
				hwnd,					/* Parent */
				(HMENU)IDC_NEXT,		/* Identifier */
				g_hInstance,			/* Program Instance handler */
				NULL					/* Window Creation data */
			);
	SendMessage(hButtonNext, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);

	EnableWindow(hButtonPrev, FALSE);
	if (g_NumFonts <= 1)
		EnableWindow(hButtonNext, FALSE);

	/* Init the display window with the font name */
	g_FontIndex = 0;
	SendMessage(hDisplay, FVM_SETTYPEFACE, 0, (LPARAM)&g_LogFonts[g_FontIndex]);
	ShowWindow(hDisplay, SW_SHOWNORMAL);

	if (g_FontPrint)
		PostMessage(hwnd, WM_COMMAND, IDC_PRINT, 0);

	return 0;
}

static LRESULT
MainWnd_OnSize(HWND hwnd)
{
	RECT rc;
	HWND hInstall, hPrint, hPrev, hNext, hDisplay;
	HDWP hDWP;

	GetClientRect(hwnd, &rc);

	hDWP = BeginDeferWindowPos(5);

	hInstall = GetDlgItem(hwnd, IDC_INSTALL);
	if (hDWP)
		hDWP = DeferWindowPos(hDWP, hInstall, NULL, BUTTON_POS_X, BUTTON_POS_Y, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	hPrint = GetDlgItem(hwnd, IDC_PRINT);
	if (hDWP)
		hDWP = DeferWindowPos(hDWP, hPrint, NULL, BUTTON_POS_X + BUTTON_WIDTH + BUTTON_PADDING, BUTTON_POS_Y, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	hPrev = GetDlgItem(hwnd, IDC_PREV);
	if (hDWP)
		hDWP = DeferWindowPos(hDWP, hPrev, NULL, rc.right - (BUTTON_WIDTH * 2 + BUTTON_PADDING + BUTTON_POS_X), BUTTON_POS_Y, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	hNext = GetDlgItem(hwnd, IDC_NEXT);
	if (hDWP)
		hDWP = DeferWindowPos(hDWP, hNext, NULL, rc.right - (BUTTON_WIDTH + BUTTON_POS_X), BUTTON_POS_Y, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);

	hDisplay = GetDlgItem(hwnd, IDC_DISPLAY);
	if (hDWP)
		hDWP = DeferWindowPos(hDWP, hDisplay, NULL, 0, HEADER_SIZE, rc.right, rc.bottom - HEADER_SIZE, SWP_NOZORDER);

	EndDeferWindowPos(hDWP);

	InvalidateRect(hwnd, NULL, TRUE);

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
    WCHAR szFullName[64];

    WCHAR szSrcPath[MAX_PATH];
    WCHAR szDestPath[MAX_PATH];
    PWSTR pszFileName;
    LONG res;
    HKEY hKey;

    SendDlgItemMessage(hwnd, IDC_DISPLAY, FVM_GETFULLNAME, 64, (LPARAM)szFullName);
//    MessageBoxW(hwnd, szFullName, L"Debug", MB_OK);

    /* First, we have to find out if the font still exists */
    if (GetFileAttributes(g_fileName) == INVALID_FILE_ATTRIBUTES)
    {
        /* Fail, if the source file does not exist */
        ErrorMsgBox(0, IDS_ERROR_NOFONT, g_fileName);
        return -1;
    }

    /* Build the full destination file name */
    GetFullPathNameW(g_fileName, MAX_PATH, szSrcPath, &pszFileName);

    GetWindowsDirectoryW(szDestPath, MAX_PATH);
    wcscat(szDestPath, L"\\Fonts\\");
    wcscat(szDestPath, pszFileName);

    /* Debug Message */
//    MessageBoxW(hwnd, szDestPath, L"szDestPath", MB_OK);
//    MessageBoxW(hwnd, pszFileName, L"pszFileExt", MB_OK);

    /* Check if the file already exists */
    if (GetFileAttributesW(szDestPath) != INVALID_FILE_ATTRIBUTES)
    {
        MessageBoxW(hwnd, L"This font is already installed!", L"Already Installed", MB_OK);
        return 0;
    }

    /* Copy the font file */
    if (!CopyFileW(g_fileName, szDestPath, TRUE))
    {
        MessageBoxW(hwnd,L"Failed to copy the font file!", L"File Error", MB_OK);
        return -1;
    }

    /* Open the fonts key */
    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                        0,
                        KEY_ALL_ACCESS,
                        &hKey);
    if (res != ERROR_SUCCESS)
    {
        MessageBoxW(hwnd, L"Failed top open the fonts key!", L"Debug1", MB_OK);
        return -1;
    }

    /* Register the font */
    res = RegSetValueExW(hKey,
                         szFullName,
                         0,
                         REG_SZ,
                         (LPBYTE)pszFileName,
                         (wcslen(pszFileName) + 1) * sizeof(WCHAR));
    if (res != ERROR_SUCCESS)
    {
        MessageBoxW(hwnd, L"Failed to register the new font!", L"Debug2", MB_OK);
        RegCloseKey(hKey);
        return -1;
    }

    /* Close the fonts key */
    RegCloseKey(hKey);

    /* if all of this goes correctly, message the user about success */
    MessageBoxW(hwnd, L"Font Installation Completed.", L"Success", MB_OK);

    return 0;
}

static LRESULT
MainWnd_OnPrev(HWND hwnd)
{
	HWND hDisplay;
	if (g_FontIndex > 0)
	{
		--g_FontIndex;
		EnableWindow(GetDlgItem(hwnd, IDC_NEXT), TRUE);
		if (g_FontIndex == 0)
			EnableWindow(GetDlgItem(hwnd, IDC_PREV), FALSE);

		hDisplay = GetDlgItem(hwnd, IDC_DISPLAY);
		SendMessage(hDisplay, FVM_SETTYPEFACE, 0, (LPARAM)&g_LogFonts[g_FontIndex]);
		InvalidateRect(hDisplay, NULL, TRUE);
	}
	return 0;
}

static LRESULT
MainWnd_OnNext(HWND hwnd)
{
	HWND hDisplay;
	if (g_FontIndex + 1 < g_NumFonts)
	{
		++g_FontIndex;
		EnableWindow(GetDlgItem(hwnd, IDC_PREV), TRUE);
		if (g_FontIndex == g_NumFonts - 1)
			EnableWindow(GetDlgItem(hwnd, IDC_NEXT), FALSE);

		hDisplay = GetDlgItem(hwnd, IDC_DISPLAY);
		SendMessage(hDisplay, FVM_SETTYPEFACE, 0, (LPARAM)&g_LogFonts[g_FontIndex]);
		InvalidateRect(hDisplay, NULL, TRUE);
	}
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

				case IDC_PRINT:
					return Display_OnPrint(hwnd);

				case IDC_PREV:
					return MainWnd_OnPrev(hwnd);

				case IDC_NEXT:
					return MainWnd_OnNext(hwnd);
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
