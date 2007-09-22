/*
 *  Copyright 2003 J Brown
 *  Copyright 2006 Eric Kohl
 *  Copyright 2007 Marc Piulachs (marc.piulachs@codexchange.net)
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <windows.h>
#include <tchar.h>
#include "resource.h"

#define RANDOM( min, max ) ((rand() % (int)(((max)+1) - (min))) + (min))

#define APPNAME _T("Logon")
#define APP_TIMER			1
#define APP_TIMER_INTERVAL	2000

#define BITMAP_HEIGHT	 240;
#define BITMAP_WIDTH	 340

HINSTANCE hInstance;

BOOL fullscreen = FALSE;

void DrawScreen (HDC hdc, HDC hMemDC , RECT rect)
{
	int x;
	int y;
	int width = BITMAP_WIDTH;
	int height = BITMAP_HEIGHT;

	if (!fullscreen)
	{
		width =  width / 20;
		height = height / 20;
	}

	x = RANDOM (0, rect.right - width);
	y = RANDOM (0, rect.bottom - height);

	BitBlt(
		hdc, 
		x, 
		y, 
		width, 
		height, 
		hMemDC, 
		0, 
		0, 
		SRCCOPY);
}

HBITMAP GetScreenSaverBitmap (void)
{
	OSVERSIONINFOEX osvi;
	
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx ((OSVERSIONINFO *) &osvi);

	switch(osvi.wProductType)
	{
		case VER_NT_WORKSTATION:
			return LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_WORKSTATION));
			break;
		default:
			return LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_SERVER));
			break;
	}
}

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static POINT ptLast;
	static POINT ptCursor;
	static BOOL  fFirstTime = TRUE;

	static PAINTSTRUCT ps;
	static RECT rect;
	static HDC hDC;
	static HDC hMemDC;
	static HBRUSH hBlkBrush;
	static HBITMAP bitmap;

	switch (msg)
	{
		case WM_CREATE: 
		{
			hDC = GetDC(hwnd);
			hBlkBrush = (HBRUSH) GetStockObject(BLACK_BRUSH);
			hMemDC = CreateCompatibleDC(hDC);
			GetClientRect(hwnd, &rect);

			bitmap = GetScreenSaverBitmap ();

			if(bitmap == NULL)
			{
				MessageBox(
				   hwnd, 
				   _T("Fatal Error: Could not load bitmap"), 
				   _T("Error"),
				   MB_OK | MB_ICONEXCLAMATION); 
			}

			SetTimer (
				hwnd, 
				APP_TIMER, 
				APP_TIMER_INTERVAL, 
				NULL);

			 break; 
		 }
		case WM_PAINT:
			{	
				hDC = BeginPaint(hwnd, &ps);
				SelectObject(hMemDC, bitmap);
				DrawScreen (hDC , hMemDC , rect);
				EndPaint(hwnd, &ps);
				break;
			}
		case WM_TIMER :
		{
			if (wParam == APP_TIMER)
			{
				InvalidateRect(hwnd, NULL, 1);
			}
		}
		  case WM_ERASEBKGND:
			{
				SelectObject(hDC, hBlkBrush);
		        
				PatBlt(
					hDC,
					0,
					0, 
					rect.right, 
					rect.bottom, 
					PATCOPY);
				break;
			}
			case WM_DESTROY:
			{
				KillTimer (hwnd, APP_TIMER);
				DeleteObject(bitmap);
				ShowCursor(TRUE);
				PostQuitMessage(0);
				break;	
			}
			
			// break out of screen-saver if any keyboard activity
			case WM_NOTIFY:
			case WM_SYSKEYDOWN:
				PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;

			// break out of screen-saver if any mouse activity
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MOUSEMOVE:
				// If we've got a parent then we must be a preview
				if(GetParent(hwnd) != 0)
					return 0;

				if(fFirstTime)
				{
					GetCursorPos(&ptLast);
					fFirstTime = FALSE;
				}

			GetCursorPos(&ptCursor);

			// if the mouse has moved more than 3 pixels then exit
			if(abs(ptCursor.x - ptLast.x) >= 3 || abs(ptCursor.y - ptLast.y) >= 3)
				PostMessage(hwnd, WM_CLOSE, 0, 0);

			ptLast = ptCursor;

			return 0;
		}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitSaver(HWND hwndParent)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style            = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc      = WndProc;
	wc.lpszClassName    = APPNAME;
	wc.hbrBackground    = (HBRUSH)GetStockObject(BLACK_BRUSH);
	RegisterClass(&wc);

	if (hwndParent != 0)
	{
		RECT rect;
		GetClientRect(hwndParent, &rect);
		CreateWindow(APPNAME, APPNAME,
		             WS_VISIBLE | WS_CHILD,
		             0, 0,
		             rect.right,
		             rect.bottom,
		             hwndParent, 0,
		             hInstance, NULL);
		fullscreen = FALSE;
	}
	else
	{
		HWND hwnd;
		hwnd = CreateWindowEx(WS_EX_TOPMOST,
                          APPNAME, 
                          APPNAME,
                          WS_VISIBLE | WS_POPUP,
                          0, 0,
                          GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                          HWND_DESKTOP, 0,
                          hInstance, NULL);
        
    SetWindowPos(hwnd, 
                 0, 0, 0, 0, 0, 
                 SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_SHOWWINDOW);

    ShowCursor(FALSE);
		fullscreen = TRUE;
	}
}

void ParseCommandLine(PSTR szCmdLine, int *chOption, HWND *hwndParent)
{
	int ch = *szCmdLine++;

	if(ch == '-' || ch == '/')
		ch = *szCmdLine++;

	if(ch >= 'A' && ch <= 'Z')
		ch += 'a' - 'A';

	*chOption = ch;
	ch = *szCmdLine++;

	if(ch == ':')
		ch = *szCmdLine++;

	while(ch == ' ' || ch == '\t')
		ch = *szCmdLine++;

	if(isdigit(ch))
	{
		unsigned int i = atoi(szCmdLine - 1);
		*hwndParent = (HWND)i;
	}
	else
		*hwndParent = 0;
}

void Configure(void)
{
	TCHAR szTitle[256];
	TCHAR szText[256];

	LoadString(hInstance,
		   IDS_TITLE,
		   szTitle,
		   256);

	LoadString(hInstance,
		   IDS_TEXT,
		   szText,
		   256);

	MessageBox(0,
	           szText,
	           szTitle,
	           MB_OK | MB_ICONWARNING);
}

int WINAPI WinMain (HINSTANCE hInst,
                    HINSTANCE hPrev,
                    LPSTR lpCmdLine,
                    int iCmdShow)
{
	HWND	hwndParent;
	UINT	nPreviousState;
	int	chOption;
	MSG	Message;

	hInstance = hInst;

	ParseCommandLine(lpCmdLine, &chOption, &hwndParent);

	SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, TRUE, &nPreviousState, 0);

	switch (chOption)
	{
		case 's':
			InitSaver(0);
			break;

		case 'p':
			InitSaver(hwndParent);
			break;

		case 'c':
		default:
			Configure();
			return 0;
	}

	while (GetMessage(&Message, 0, 0, 0))
		DispatchMessage(&Message);

	SystemParametersInfo(SPI_SETSCREENSAVERRUNNING, FALSE, &nPreviousState, 0);

	return Message.wParam;
}
