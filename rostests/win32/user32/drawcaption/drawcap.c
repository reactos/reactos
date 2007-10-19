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
#include "resource.h"
#include "stdio.h"

WCHAR CaptWndClass[] = L"captwnd_class";

HINSTANCE hInst;
INT testnum = 0;

//BOOL STDCALL (*DrawCaptionTemp) (
//		 HWND        hwnd,
//		 HDC         hdc,
//		 const RECT *rect,
//		 HFONT       hFont,
//		 HICON       hIcon,
//		 LPCWSTR     str,
//		 UINT        uFlags);

VOID CapTest(HWND hWnd,
	HDC hDc,
	LPRECT pR,
	WCHAR *Text,
	DWORD Flags,
	WCHAR *AddonStr,
	DWORD Addon)
{
	WCHAR Buf[512];

	lstrcpy(Buf, AddonStr);
	if(lstrlen(Buf))lstrcat(Buf, L" | ");
	lstrcat(Buf, Text);

	DrawText( hDc, Buf, lstrlen(Buf), pR, DT_LEFT );

	pR->top+=20;
	pR->bottom+=20;

	if(!DrawCaption(hWnd, hDc, pR, Flags | Addon))
	{
		printf("PAINT: DrawCaption failed: %d\n", (int)GetLastError());
	}

	pR->top+=30;
	pR->bottom+=30;
}

VOID DrawCaptionTest(HWND hWnd, HDC hDc, WCHAR *AddonStr, DWORD Addon)
{
	RECT Rect;
	GetClientRect(hWnd, &Rect);
	Rect.bottom = 30;
	Rect.left = 10;
	Rect.right-=10;
	Rect.top = 10;

	CapTest(hWnd, hDc, &Rect, L"DC_TEXT:", DC_TEXT, AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_ACTIVE:",
		DC_TEXT | DC_ACTIVE,
		AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_ICON:" ,
		DC_TEXT | DC_ICON,
		AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_ACTIVE | DC_ICON:" ,
		DC_TEXT | DC_ACTIVE | DC_ICON,
		AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_INBUTTON:" ,
		DC_TEXT | DC_INBUTTON,
		AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_ACTIVE | DC_INBUTTON:" ,
		DC_TEXT | DC_ACTIVE | DC_INBUTTON,
		AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_ICON | DC_INBUTTON:" ,
		DC_TEXT | DC_ICON | DC_INBUTTON,
		AddonStr, Addon);

	CapTest(hWnd, hDc, &Rect,
		L"DC_TEXT | DC_ACTIVE | DC_ICON | DC_INBUTTON:" ,
		DC_TEXT | DC_ACTIVE | DC_ICON | DC_INBUTTON,
		AddonStr, Addon);

}

LRESULT CALLBACK CaptWndProc(HWND hWnd,
							 UINT msg,
							 WPARAM wParam,
							 LPARAM lParam)
{


	switch (msg)
	{

		case WM_PAINT:
		{
			HDC hDc;
			PAINTSTRUCT Ps;

			hDc = BeginPaint(hWnd, &Ps);
			SetBkMode( hDc, TRANSPARENT );

			switch(testnum)
			{
			case 1:
				DrawCaptionTest(hWnd, hDc, L"", 0);
				break;
			case 2:
				DrawCaptionTest(hWnd, hDc, L"DC_GRADIENT", DC_GRADIENT);
				break;
			case 3:
				DrawCaptionTest(hWnd, hDc, L"DC_SMALLCAP", DC_SMALLCAP);
				break;
			case 4:
				DrawCaptionTest(hWnd, hDc, L"DC_BUTTONS", DC_BUTTONS);
				break;
			case 5:
				DrawCaptionTest(hWnd, hDc,
					L"DC_GRADIENT | DC_SMALLCAP",
					DC_GRADIENT | DC_SMALLCAP);
				break;
			case 6:
				DrawCaptionTest(hWnd, hDc,
					L"DC_GRADIENT | DC_BUTTONS",
					DC_GRADIENT | DC_BUTTONS);
				break;
			case 7:
				DrawCaptionTest(hWnd, hDc,
					L"DC_BUTTONS | DC_SMALLCAP",
					DC_BUTTONS | DC_SMALLCAP);
				break;
			case 8:
				DrawCaptionTest(hWnd, hDc,
					L"DC_BUTTONS | DC_SMALLCAP | DC_GRADIENT",
					DC_BUTTONS | DC_SMALLCAP | DC_GRADIENT);
				break;
			}

			EndPaint(hWnd, &Ps);

			return 0;
		}

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


INT main(INT argc, CHAR **argv)
{
	HWND hWnd;
	MSG msg;
	WNDCLASSEX wcx;
	UINT result;
	HBRUSH hBr;
	//HMODULE hLib;

	if(argc<2)
	{
		printf("DrawCaption testcode.\n");
		printf("USAGE: drawcap.exe <testnumber> [useicon]\n\n");
		printf("Available tests:\n"
			"1. DrawCaption test\n"
			"2. DrawCaption test + DC_GRADIENT\n"
			"3. DrawCaption test + DC_SMALLCAP\n"
			"4. DrawCaption test + DC_BUTTONS\n"
			"5. DrawCaption test + DC_GRADIENT | DC_SMALLCAP\n"
			"6. DrawCaption test + DC_GRADIENT | DC_BUTTONS\n"
			"7. DrawCaption test + DC_BUTTONS | DC_SMALLCAP\n"
			"8. DrawCaption test + DC_BUTTONS | DC_SMALLCAP | DC_GRADIENT\n\n");
		return 0;
	}

	testnum = atoi(argv[1]);
	if(testnum < 1 || testnum > 8)
	{
		printf("Unknown test %d\n", testnum);
		return 1;
	}

	hInst = GetModuleHandle(NULL);

	//hLib = LoadLibrary(L"user32");
	//if(!hLib)
	//{
	//	printf("Shit! Can't load user32.dll\n");
	//	return 1;
	//}

	//DrawCaptionTemp = GetProcAddress(hLib, "DrawCaptionTempW");
	//if(!DrawCaptionTemp)
	//{
	//	printf("Shit! Can't get DrawCaptionTemp address\n");
	//	return 1;
	//}

	hBr = CreateSolidBrush(RGB(255, 255, 255));
	if(!hBr)
	{
		printf("Shit! Can't create brush.");
		return 1;
	}

	memset(&wcx, 0, sizeof(wcx));
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = (WNDPROC) CaptWndProc;
	wcx.hInstance = hInst;
	wcx.hbrBackground = hBr;
	wcx.lpszClassName = CaptWndClass;
	if(argc > 2) wcx.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON1SM));

	if(!(result = RegisterClassEx(&wcx)))
	{
		printf("Shit! RegisterClassEx failed: %d\n",
			(int)GetLastError());
		DeleteObject(hBr);
		return 1;
	}

	hWnd = CreateWindowEx(0,
				CaptWndClass,
				L"DrawCaption test",
				WS_OVERLAPPED|WS_THICKFRAME|WS_SYSMENU,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				600,
				470,
				NULL,
				0,
				hInst,
				NULL);

	if(!hWnd)
	{
		printf("Shit! Can't create wnd!\n");
		UnregisterClass(CaptWndClass, hInst);
		DeleteObject(hBr);
		return 1;
	}


	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0 ))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(hBr);
	UnregisterClass(CaptWndClass, hInst);
	return 0;
}
