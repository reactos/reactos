/*
 *  ReactOS Applications
 *  Copyright (C) 1998-2004 ReactOS Team
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
 *
 *  $Id: notepad.c,v 1.1 2003/12/30 11:52:04 rcampbell Exp $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Applications
 *  PURPOSE:          Notepad Replacement
 *  FILE:             apps/dist/notepad/notepad.c
 *  PROGRAMER:        Richard Campbell (eek2121@comcast.net)
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include "notepad.h"

HWND g_hWnd, g_EditWnd;
char *szFilename = "Untitled";

LRESULT WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_SIZE:
		case WM_MOVE:
			{
				RECT r;

				GetClientRect(g_hWnd,&r);
				MoveWindow(g_EditWnd,r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE);
				break;
			}
		case WM_CLOSE:
			{
				DestroyWindow(g_EditWnd);
				DestroyWindow(g_hWnd);
				PostQuitMessage(0);
				break;
			}
		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
					case ID_FILE_NEW:
						{
							MessageBox(g_hWnd,"The text in the Untitled file has changed.\n\n Do you want to save the changes?","Notepad",MB_YESNOCANCEL | MB_ICONEXCLAMATION);
						}
					case ID_FILE_EXIT:
						PostQuitMessage(0);
				}
				break;
			}
	}
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WINAPI EditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateMainWnd()
{
    WNDCLASS wc;
	HWND hWnd;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = TEXT("notepad");
	wc.lpfnWndProc = MainWndProc;
	
	wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	if(!RegisterClass(&wc))
		return NULL;

	hWnd = CreateWindow("notepad",
						"Notepad",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						0,
						LoadMenu(NULL,MAKEINTRESOURCE(IDC_NOTEPAD)),
						NULL,
						0);

    return hWnd;
}

HWND CreateEditWnd(HWND hParent, DWORD dwWidth, DWORD dwHeight)
{
		HWND hWnd;
		hWnd = CreateWindowEx(WS_EX_CLIENTEDGE,
						"EDIT",
						NULL,
						WS_HSCROLL | WS_VSCROLL | WS_VISIBLE |
						WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL |
						ES_MULTILINE | ES_WANTRETURN,
						0,
						0,
						dwWidth,
						dwHeight,
						hParent,
						0,
						NULL,
						0);
		return hWnd;
}

int WINAPI 
WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
	MSG msg;
	RECT r;
	HACCEL hAccel;
	g_hWnd = CreateMainWnd();

	if(!g_hWnd)
		PostQuitMessage(0);

	GetClientRect(g_hWnd,&r);
	g_EditWnd = CreateEditWnd(g_hWnd, r.right - r.left, r.bottom - r.top);

	if (!g_EditWnd)
	{
		return GetLastError();
	}

	ShowWindow(g_hWnd,nCmdShow);
	UpdateWindow(g_hWnd);

    hAccel = LoadAccelerators( hInstance, MAKEINTRESOURCE(IDC_NOTEPAD) );

    if( hAccel != NULL )
    {
        while( GetMessage(&msg, 0, 0, 0)) {
            if( !TranslateAccelerator(g_hWnd, hAccel, &msg ) )
			{
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
    }
	else
    {
        while (GetMessage(&msg, 0, 0, 0))
		{
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
