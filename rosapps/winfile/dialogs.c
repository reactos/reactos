/*
 *  ReactOS winfile
 *
 *  dialogs.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include <shellapi.h>
//#include <winspool.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "winfile.h"
#include "about.h"
#include "dialogs.h"
#include "utils.h"
#include "debug.h"


struct ExecuteDialog {
	TCHAR	cmd[MAX_PATH];
	int		cmdshow;
};


BOOL CALLBACK ExecuteDialogWndProg(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct ExecuteDialog* dlg;

	switch(nmsg) {
		case WM_INITDIALOG:
			dlg = (struct ExecuteDialog*) lparam;
			return 1;

		case WM_COMMAND: {
			int id = (int)wparam;

			if (id == IDOK) {
				GetWindowText(GetDlgItem(hwnd, 201), dlg->cmd, MAX_PATH);
				dlg->cmdshow = Button_GetState(GetDlgItem(hwnd,214))&BST_CHECKED?
												SW_SHOWMINIMIZED: SW_SHOWNORMAL;
				EndDialog(hwnd, id);
			} else if (id == IDCANCEL)
				EndDialog(hwnd, id);

			return 1;}
	}

	return 0;
}



