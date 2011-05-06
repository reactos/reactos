/*
 *  Copyright 2006 Saveliy Tretiakov
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "windows.h"
#include "stdio.h"
#include "resource.h"

WCHAR WndClass[] = L"capicon_class";

HINSTANCE hInst;
INT testnum = 0;


LRESULT CALLBACK WndProc(HWND hWnd,
							 UINT msg,
							 WPARAM wParam,
							 LPARAM lParam)
{
   HICON hIcon;

	switch (msg)
	{
	   case WM_GETICON:
	      if(testnum>2)
	      {
	         if(wParam == ICON_SMALL)
	            hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON2SM));
	         else if(wParam == ICON_BIG)
	            hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON2BIG));
	         else hIcon = (HICON)1;

	         if(!hIcon)
	         {
	            printf("LoadIcon() failed: %d\n", (INT)GetLastError());
	            break;
	         }

	         return (LRESULT)hIcon;
	      }
	      break;

      	case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


int wmain(int argc, wchar_t**argv)
{
	HWND hWnd;
	MSG msg;
	WNDCLASSEX wcx;
	UINT result;

	if(argc<2)
	{
		printf("DrawCaption icon test.\n");
		printf("USAGE: drawcap.exe <testnumber>\n\n");
		printf("Available tests:\n"
			"1. Class small icon\n"
			"2. Class big icon\n"
			"3. Class small icon + WM_GETICON\n"
			"4. Class big icon + WM_GETICON\n"
			"5. WM_GETICON only\n\n");
		return 0;
	}

	testnum = _wtoi(argv[1]);
	if(testnum < 1 || testnum > 5)
	{
		printf("Unknown test %d\n", testnum);
		return 1;
	}

	hInst = GetModuleHandle(NULL);

	memset(&wcx, 0, sizeof(wcx));
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = (WNDPROC) WndProc;
	wcx.hInstance = hInst;
	wcx.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wcx.lpszClassName = WndClass;
	if(testnum<5)wcx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON1BIG));
	if(testnum == 1 || testnum == 3)
	   wcx.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON1SM));

	if(!(result = RegisterClassEx(&wcx)))
	{
		printf("Shit! RegisterClassEx failed: %d\n",
			(int)GetLastError());
		return 1;
	}

	hWnd = CreateWindowEx(0,
				WndClass,
				L"DrawCaption icon test",
				WS_OVERLAPPED|WS_THICKFRAME|WS_SYSMENU,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				250,
				100,
				NULL,
				0,
				hInst,
				NULL);

	if(!hWnd)
	{
		printf("Shit! Can't create wnd!\n");
		UnregisterClass(WndClass, hInst);
		return 1;
	}


	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0 ))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnregisterClass(WndClass, hInst);
	return 0;
}
