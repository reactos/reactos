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


BOOL CALLBACK ExecuteDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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


BOOL CALLBACK OptionsConfirmationWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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


BOOL CALLBACK ViewFileTypeWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

////////////////////////////////////////////////////////////////////////////////
/*
TotalFileSize 
[in] Specifies the total size of the file, in bytes. 
TotalBytesTransferred 
[in] Specifies the total number of bytes transferred from the source file to the destination file since the copy operation began. 
StreamSize 
[in] Specifies the total size of the current file stream, in bytes. 
StreamBytesTransferred 
[in] Specifies the total number of bytes in the current stream that have been transferred from the source file to the destination file since the copy operation began. 
dwStreamNumber 
[in] Handle to the current stream. The stream number is 1 the first time CopyProgressRoutine is called. 
dwCallbackReason 
[in] Specifies the reason that CopyProgressRoutine was called. This parameter can be one of the following values. Value Meaning 
CALLBACK_CHUNK_FINISHED Another part of the data file was copied. 
CALLBACK_STREAM_SWITCH Another stream was created and is about to be copied. This is the callback reason given when the callback routine is first invoked. 


hSourceFile 
[in] Handle to the source file. 
hDestinationFile 
[in] Handle to the destination file 
lpData 
[in] The argument passed to CopyProgressRoutine by the CopyFileEx or MoveFileWithProgress function. 
Return Values
The CopyProgressRoutine function should return one of the following values.

Value Meaning 
PROGRESS_CONTINUE Continue the copy operation. 
PROGRESS_CANCEL Cancel the copy operation and delete the destination file. 
PROGRESS_STOP Stop the copy operation. It can be restarted at a later time. 
PROGRESS_QUIET Continue the copy operation, but stop invoking CopyProgressRoutine to report progress. 
 */
DWORD CALLBACK CopyProgressRoutine(
  LARGE_INTEGER TotalFileSize,          // file size
  LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
  LARGE_INTEGER StreamSize,             // bytes in stream
  LARGE_INTEGER StreamBytesTransferred, // bytes transferred for stream
  DWORD dwStreamNumber,                 // current stream
  DWORD dwCallbackReason,               // callback reason
  HANDLE hSourceFile,                   // handle to source file
  HANDLE hDestinationFile,              // handle to destination file
  LPVOID lpData                         // from CopyFileEx
)
{
    return 0L;
}

BOOL CALLBACK MoveFileWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static struct ExecuteDialog* dlg;
	int id;
    TCHAR buffer_from[1000];
    TCHAR buffer_to[1000];

	switch (message) {
	case WM_INITDIALOG:
		dlg = (struct ExecuteDialog*)lParam;

        _tcscpy(buffer_from, _T("C:\\TEMP\\API_SPY\\TEMP\\foobar.txt"));
        SetDlgItemText(hDlg, IDC_FILE_MOVE_FROM, buffer_from);
        _tcscpy(buffer_to, _T("C:\\TEMP\\API_SPY\\TEMP\\foobar2.txt"));
        SetDlgItemText(hDlg, IDC_FILE_MOVE_TO, buffer_to);
/*
        Button_SetCheck(GetDlgItem(hDlg,IDC_VIEW_TYPE_DIRECTORIES), ViewType & VIEW_DIRECTORIES ? BST_CHECKED : BST_UNCHECKED);
 */
		return 1;
	case WM_COMMAND:
		id = (int)wParam;
		if (id == IDOK) {
            LPVOID lpData = NULL;                     // parameter for callback
            DWORD dwFlags = MOVEFILE_COPY_ALLOWED;    // move options

            GetDlgItemText(hDlg, IDC_FILE_MOVE_FROM, buffer_from, sizeof(buffer_from));
            GetDlgItemText(hDlg, IDC_FILE_MOVE_TO, buffer_to, sizeof(buffer_to));
/*
BOOL MoveFileWithProgress(
  LPCTSTR lpExistingFileName,            // file name
  LPCTSTR lpNewFileName,                 // new file name
  LPPROGRESS_ROUTINE lpProgressRoutine,  // callback function
  LPVOID lpData,                         // parameter for callback
  DWORD dwFlags                          // move options
);
DWORD FormatMessage(
  DWORD dwFlags,      // source and processing options
  LPCVOID lpSource,   // message source
  DWORD dwMessageId,  // message identifier
  DWORD dwLanguageId, // language identifier
  LPTSTR lpBuffer,    // message buffer
  DWORD nSize,        // maximum size of message buffer
  va_list *Arguments  // array of message inserts
);
 */
//            if (!MoveFileWithProgress(buffer_from, buffer_to, &CopyProgressRoutine, lpData, dwFlags)) {
            if (!MoveFileEx(buffer_from, buffer_to, dwFlags)) {
                DWORD err = GetLastError();
                HLOCAL hMem;
                if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (LPTSTR)&hMem, 10, NULL)) {
                    MessageBox(hDlg, hMem, szTitle, MB_OK);
                    LocalFree(hMem);
                } else {
                    MessageBox(hDlg, _T("Unknown Error"), szTitle, MB_OK);
                }
            }

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

