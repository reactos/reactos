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

#include "main.h"
#include "about.h"
#include "dialogs.h"
#include "settings.h"
#include "utils.h"
#include "debug.h"


BOOL CALLBACK ExecuteDialogWndProg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static struct ExecuteDialog* dlg;

	switch(message) {
		case WM_INITDIALOG:
			dlg = (struct ExecuteDialog*) lParam;
			return 1;

		case WM_COMMAND: {
			int id = (int)wParam;

			if (id == IDOK) {
				GetWindowText(GetDlgItem(hDlg, 201), dlg->cmd, MAX_PATH);
				dlg->cmdshow = Button_GetState(GetDlgItem(hDlg,214))&BST_CHECKED?
												SW_SHOWMINIMIZED: SW_SHOWNORMAL;
				EndDialog(hDlg, id);
			} else if (id == IDCANCEL)
				EndDialog(hDlg, id);

			return 1;}
	}

	return 0;
}


BOOL CALLBACK ExecuteOptionsConfirmationWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static struct ExecuteDialog* dlg;
	int id;

	switch (message) {
	case WM_INITDIALOG:
		dlg = (struct ExecuteDialog*) lParam;
        Button_SetCheck(GetDlgItem(hDlg,IDC_CONFIRMATION_FILE_DELETE),   Confirmation & CONFIRM_FILE_DELETE ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CONFIRMATION_DIR_DELETE),    Confirmation & CONFIRM_DIR_DELETE ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CONFIRMATION_FILE_REPLACE),  Confirmation & CONFIRM_FILE_REPLACE ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CONFIRMATION_MOUSE_ACTIONS), Confirmation & CONFIRM_MOUSE_ACTIONS ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CONFIRMATION_DISK_COMMANDS), Confirmation & CONFIRM_DISK_COMMANDS ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CONFIRMATION_MODIFY_SYSTEM), Confirmation & CONFIRM_MODIFY_SYSTEM ? BST_CHECKED : BST_UNCHECKED);
		return 1;
	case WM_COMMAND:
		id = (int)wParam;
		if (id == IDOK) {
			GetWindowText(GetDlgItem(hDlg, 201), dlg->cmd, MAX_PATH);
			dlg->cmdshow = Button_GetState(GetDlgItem(hDlg,214))&BST_CHECKED?SW_SHOWMINIMIZED: SW_SHOWNORMAL;

            if (Button_GetState(GetDlgItem(hDlg,IDC_CONFIRMATION_FILE_DELETE)) & BST_CHECKED)
                 Confirmation |= CONFIRM_FILE_DELETE;
            else Confirmation &= ~CONFIRM_FILE_DELETE;
            if (Button_GetState(GetDlgItem(hDlg,IDC_CONFIRMATION_DIR_DELETE)) & BST_CHECKED)
                 Confirmation |= CONFIRM_DIR_DELETE;
            else Confirmation &= ~CONFIRM_DIR_DELETE;
            if (Button_GetState(GetDlgItem(hDlg,IDC_CONFIRMATION_FILE_REPLACE)) & BST_CHECKED)
                 Confirmation |= CONFIRM_FILE_REPLACE;
            else Confirmation &= ~CONFIRM_FILE_REPLACE;
            if (Button_GetState(GetDlgItem(hDlg,IDC_CONFIRMATION_MOUSE_ACTIONS)) & BST_CHECKED)
                 Confirmation |= CONFIRM_MOUSE_ACTIONS;
            else Confirmation &= ~CONFIRM_MOUSE_ACTIONS;
            if (Button_GetState(GetDlgItem(hDlg,IDC_CONFIRMATION_DISK_COMMANDS)) & BST_CHECKED)
                 Confirmation |= CONFIRM_DISK_COMMANDS;
            else Confirmation &= ~CONFIRM_DISK_COMMANDS;
            if (Button_GetState(GetDlgItem(hDlg,IDC_CONFIRMATION_MODIFY_SYSTEM)) & BST_CHECKED)
                 Confirmation |= CONFIRM_MODIFY_SYSTEM;
            else Confirmation &= ~CONFIRM_MODIFY_SYSTEM;

			EndDialog(hDlg, id);
		} else if (id == IDCANCEL)
			EndDialog(hDlg, id);
		return 1;
	}
	return 0;
}


BOOL CALLBACK ExecuteViewFileTypeWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static struct ExecuteDialog* dlg;
	int id;

	switch (message) {
	case WM_INITDIALOG:
		dlg = (struct ExecuteDialog*)lParam;
        Button_SetCheck(GetDlgItem(hDlg,IDC_VIEW_TYPE_DIRECTORIES), ViewType & VIEW_DIRECTORIES ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_VIEW_TYPE_PROGRAMS),    ViewType & VIEW_PROGRAMS    ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_VIEW_TYPE_DOCUMENTS),   ViewType & VIEW_DOCUMENTS   ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_VIEW_TYPE_OTHERS),      ViewType & VIEW_OTHER       ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_VIEW_TYPE_SYSFILES),    ViewType & VIEW_SYSTEM      ? BST_CHECKED : BST_UNCHECKED);
		return 1;
	case WM_COMMAND:
		id = (int)wParam;
		if (id == IDOK) {
			GetWindowText(GetDlgItem(hDlg, 201), dlg->cmd, MAX_PATH);
			dlg->cmdshow = Button_GetState(GetDlgItem(hDlg,214))&BST_CHECKED?SW_SHOWMINIMIZED: SW_SHOWNORMAL;

            if (Button_GetState(GetDlgItem(hDlg,IDC_VIEW_TYPE_DIRECTORIES)) & BST_CHECKED)
                 ViewType |= VIEW_DIRECTORIES;
            else ViewType &= ~VIEW_DIRECTORIES;
            if (Button_GetState(GetDlgItem(hDlg,IDC_VIEW_TYPE_PROGRAMS)) & BST_CHECKED)
                 ViewType |= VIEW_PROGRAMS;
            else ViewType &= ~VIEW_PROGRAMS;
            if (Button_GetState(GetDlgItem(hDlg,IDC_VIEW_TYPE_DOCUMENTS)) & BST_CHECKED)
                 ViewType |= VIEW_DOCUMENTS;
            else ViewType &= ~VIEW_DOCUMENTS;
            if (Button_GetState(GetDlgItem(hDlg,IDC_VIEW_TYPE_OTHERS)) & BST_CHECKED)
                 ViewType |= VIEW_OTHER;
            else ViewType &= ~VIEW_OTHER;
            if (Button_GetState(GetDlgItem(hDlg,IDC_VIEW_TYPE_SYSFILES)) & BST_CHECKED)
                 ViewType |= VIEW_SYSTEM;
            else ViewType &= ~VIEW_SYSTEM;
			EndDialog(hDlg, id);
        } else if (id == IDCANCEL)
			EndDialog(hDlg, id);
    	return 1;
	}
	return 0;
}


/*
extern TCHAR ViewTypeMaskStr[MAX_TYPE_MASK_LEN];
 */

