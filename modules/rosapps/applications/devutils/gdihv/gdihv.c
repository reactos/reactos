/*
 *	Gdi handle viewer
 *
 *	gdihv.c
 *
 *	Copyright (C) 2007	Timo Kreuzer <timo <dot> kreuzer <at> reactos <dot> org>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gdihv.h"

HINSTANCE g_hInstance;
PGDI_TABLE_ENTRY GdiHandleTable = 0;

static
PGDI_TABLE_ENTRY
MyGdiQueryTable()
{
	PTEB pTeb = NtCurrentTeb();
	PPEB pPeb = pTeb->ProcessEnvironmentBlock;
	return pPeb->GdiSharedHandleTable;
}

int WINAPI _tWinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPTSTR lpszArgument,
                    int nStyle)

{
	g_hInstance = hThisInstance;

	InitCommonControls();

	GdiHandleTable = MyGdiQueryTable();

	DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_MAINWND), HWND_DESKTOP, MainWindow_WndProc, 0);

	/* The program return value is 0 */
	return 0;
}
