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
 
 /* This testapp demonstrates WS_SYSMENU + WS_EX_DLGMODALFRAME 
  * behavior and shows that DrawCaption does care 
  * about WS_EX_DLGMODALFRAME and WS_EX_TOOLWINDOW 
  */

#include "windows.h"
#include "stdio.h"
#include "resource.h"

WCHAR WndClass[] = L"sysicon_class";

LRESULT CALLBACK WndProc(HWND hWnd, 
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
	    RECT Rect;
	    GetClientRect(hWnd, &Rect);
	    
	    Rect.left = 10;
	    Rect.top = 10;
	    Rect.right-=10;
	    Rect.bottom = 25;
	    
			hDc = BeginPaint(hWnd, &Ps);
			SetBkMode( hDc, TRANSPARENT );
			
	    DrawCaption(hWnd, hDc, &Rect, DC_GRADIENT | DC_ACTIVE | DC_TEXT | DC_ICON);
	
			EndPaint(hWnd, &Ps);
		
			return 0;
		}
			
   case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInst,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
	HWND hWnd1, hWnd2, hWnd3;
	MSG msg;
	WNDCLASSEX wcx;
	UINT result;
	
	memset(&wcx, 0, sizeof(wcx));
	wcx.cbSize = sizeof(wcx);
	wcx.lpfnWndProc = (WNDPROC) WndProc;
	wcx.hInstance = hInst;
	wcx.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wcx.lpszClassName = WndClass;
	
	if(!(result = RegisterClassEx(&wcx)))
	{
		return 1;
	}
	
	/* WS_EX_DLGMODALFRAME */
	hWnd1 = CreateWindowEx(WS_EX_DLGMODALFRAME, 
				WndClass, 
				L"WS_SYSMENU | WS_EX_DLGMODALFRAME",
				WS_CAPTION | WS_SYSMENU ,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				400,
				100,
				NULL,
				0,
				hInst,
				NULL);
	
	if(!hWnd1)
	{
		return 1;
	}
	
	ShowWindow(hWnd1, SW_SHOW); 
	UpdateWindow(hWnd1);  

	hWnd2 = CreateWindowEx(WS_EX_TOOLWINDOW,
				WndClass, 
				L"WS_SYSMENU | WS_EX_TOOLWINDOW",
				WS_CAPTION | WS_SYSMENU ,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				400,
				100,
				NULL,
				0,
				hInst,
				NULL);
	
	if(!hWnd2)
	{
		return 1;
	}
	
	ShowWindow(hWnd2, SW_SHOW); 
	UpdateWindow(hWnd2);  

	hWnd3 = CreateWindowEx(0,
				WndClass, 
				L"WS_SYSMENU ",
				WS_CAPTION | WS_SYSMENU ,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				400,
				100,
				NULL,
				0,
				hInst,
				NULL);
	
	if(!hWnd3)
	{
		return 1;
	}
	
	ShowWindow(hWnd3, SW_SHOW); 
	UpdateWindow(hWnd3);  
	
	while(GetMessage(&msg, NULL, 0, 0 ))
	{
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	} 

	UnregisterClass(WndClass, hInst);
	return 0;
}
