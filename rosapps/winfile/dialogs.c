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
TotalFileSize           [in] Specifies the total size of the file, in bytes. 
TotalBytesTransferred   [in] Specifies the total number of bytes transferred from the source file to the destination file since the copy operation began. 
StreamSize              [in] Specifies the total size of the current file stream, in bytes. 
StreamBytesTransferred  [in] Specifies the total number of bytes in the current stream that have been transferred from the source file to the destination file since the copy operation began. 
dwStreamNumber          [in] Handle to the current stream. The stream number is 1 the first time CopyProgressRoutine is called. 
dwCallbackReason        [in] Specifies the reason that CopyProgressRoutine was called. This parameter can be one of the following values. Value Meaning 
                             CALLBACK_CHUNK_FINISHED Another part of the data file was copied. 
                             CALLBACK_STREAM_SWITCH  Another stream was created and is about to be copied. This is the callback reason given when the callback routine is first invoked. 
hSourceFile             [in] Handle to the source file. 
hDestinationFile        [in] Handle to the destination file 
lpData                  [in] The argument passed to CopyProgressRoutine by the CopyFileEx or MoveFileWithProgress function. 

Return Values           The CopyProgressRoutine function should return one of the following values.
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

            GetDlgItemText(hDlg, IDC_FILE_MOVE_FROM, buffer_from, sizeof(buffer_from)/sizeof(TCHAR));
            GetDlgItemText(hDlg, IDC_FILE_MOVE_TO, buffer_to, sizeof(buffer_to)/sizeof(TCHAR));
/*
BOOL MoveFileWithProgress(
  LPCTSTR lpExistingFileName,            // file name
  LPCTSTR lpNewFileName,                 // new file name
  LPPROGRESS_ROUTINE lpProgressRoutine,  // callback function
  LPVOID lpData,                         // parameter for callback
  DWORD dwFlags                          // move options
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

void ShowFixedFileInfo(HWND hDlg, VS_FIXEDFILEINFO* pFixedFileInfo)
{
    TCHAR* str = NULL;

    switch (pFixedFileInfo->dwFileType) {
    case VFT_UNKNOWN:    str = _T("The file type is unknown to the system."); break;
    case VFT_APP:        str = _T("The file contains an application."); break;
    case VFT_DLL:        str = _T("The file contains a dynamic-link library (DLL)."); break;
    case VFT_DRV:
        str = _T("The file contains a device driver. If dwFileType is VFT_DRV, dwFileSubtype contains a more specific description of the driver.");
        switch (pFixedFileInfo->dwFileSubtype) {
        case VFT2_UNKNOWN: str = _T("The driver type is unknown by the system."); break;
        case VFT2_DRV_COMM: str = _T("The file contains a communications driver."); break;
        case VFT2_DRV_PRINTER: str = _T("The file contains a printer driver."); break;
        case VFT2_DRV_KEYBOARD: str = _T("The file contains a keyboard driver"); break;
        case VFT2_DRV_LANGUAGE: str = _T("The file contains a language driver"); break;
        case VFT2_DRV_DISPLAY: str = _T("The file contains a display driver"); break;
        case VFT2_DRV_MOUSE: str = _T("The file contains a mouse driver"); break;
        case VFT2_DRV_NETWORK: str = _T("The file contains a network driver"); break;
        case VFT2_DRV_SYSTEM: str = _T("The file contains a system driver"); break;
        case VFT2_DRV_INSTALLABLE: str = _T("The file contains an installable driver"); break;
        case VFT2_DRV_SOUND: str = _T("The file contains a sound driver"); break;
        }
        break;
    case VFT_FONT:
        str = _T("The file contains a font. If dwFileType is VFT_FONT, dwFileSubtype contains a more specific description of the font file.");
        switch (pFixedFileInfo->dwFileSubtype) {
        case VFT2_UNKNOWN: str = _T("The font type is unknown the system."); break; 
        case VFT2_FONT_RASTER: str = _T("The file contains a raster font."); break; 
        case VFT2_FONT_VECTOR: str = _T("The file contains a vector font."); break; 
        case VFT2_FONT_TRUETYPE: str = _T("The file contains a TrueType font."); break; 

        }
        break;
    case VFT_VXD:        str = _T("The file contains a virtual device."); break;
    case VFT_STATIC_LIB: str = _T("The file contains a static-link library."); break;
    }
    if (str != NULL) {
        SetDlgItemText(hDlg, IDC_STATIC_PROP_VERSION, str);
//        SetDlgItemText(hDlg, IDC_STATIC_PROP_COPYRIGHT, pVersionData);
    }
}

// Structure used to store enumerated languages and code pages.
struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
} *lpTranslate;

void AddFileInfoValue(HWND hDlg, void* pVersionData, struct LANGANDCODEPAGE lpTranslate, UINT i, LPCTSTR info_str)
{
    TCHAR SubBlock[200];
    TCHAR* pVal;
    UINT nValLen;

    wsprintf(SubBlock, TEXT("\\StringFileInfo\\%04x%04x\\%s"), 
             lpTranslate.wLanguage, lpTranslate.wCodePage, info_str);
    // Retrieve file description for language and code page "i". 
    if (VerQueryValue(pVersionData, SubBlock, &pVal, &nValLen)) {
        ListBox_InsertItemData(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_TYPES), i, info_str);
//		ListBox_InsertItemData(pane->hwnd, idx, entry);
        SendMessage(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_VALUES), WM_SETTEXT, 0, (LPARAM)pVal);
    }
}

static TCHAR* InfoStrings[] = { 
                TEXT("Comments"), 
                TEXT("InternalName"), 
                TEXT("ProductName"), 
                TEXT("CompanyName"), 
                TEXT("LegalCopyright"), 
                TEXT("ProductVersion"), 
                TEXT("FileDescription"), 
                TEXT("LegalTrademarks"), 
                TEXT("PrivateBuild"), 
                TEXT("FileVersion"), 
                TEXT("OriginalFilename"), 
                TEXT("SpecialBuild"),
                TEXT(""),
                NULL
};

void SelectVersionString(HWND hDlg, LPARAM lParam)
{
    int idx;

//	idx = ListBox_FindItemData(hwnd, , child->left.cur);
	idx = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_TYPES));

	//Entry* entry = (Entry*) ListBox_GetItemData(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_TYPES), idx);

//        SendMessage(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_VALUES), WM_SETTEXT, 0, (LPARAM)pVal);

//	ListBox_SetCurSel(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_TYPES), idx);

//    ListBox_InsertItemData(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_TYPES), i, _T("FileDescription"));
//    SendMessage(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_VALUES), WM_SETTEXT, 0, pVal);

}

void CheckForFileInfo(HWND hDlg, TCHAR* strFilename)
{
    TCHAR SubBlock[200];
    UINT i;
    DWORD dwHandle;
    DWORD dwVersionDataLen = GetFileVersionInfoSize(strFilename, &dwHandle);
    if (dwVersionDataLen != 0L) {
        void* pVersionData = malloc(dwVersionDataLen);
        if (GetFileVersionInfo(strFilename, 0, dwVersionDataLen, pVersionData)) {
            TCHAR* pVal;
            UINT nValLen;
//            LPTSTR SubBlock = _T("\\");
            _tcscpy(SubBlock, TEXT("\\"));
            if (VerQueryValue(pVersionData, SubBlock, &pVal, &nValLen)) {
                if (nValLen == sizeof(VS_FIXEDFILEINFO)) {
                    ShowFixedFileInfo(hDlg, (VS_FIXEDFILEINFO*)pVal);
                }
            }
#if 1
            {

    // Read the list of languages and code pages.
    _tcscpy(SubBlock, TEXT("\\VarFileInfo\\Translation"));
    if (VerQueryValue(pVersionData, SubBlock, (LPVOID*)&pVal, &nValLen)) {
        // Read the file description for each language and code page.
        for (i = 0; i < (nValLen/sizeof(struct LANGANDCODEPAGE)); i++) {
            int j = 0;
            TCHAR* pInfoString;
            while (pInfoString = InfoStrings[j]) {
                if (pInfoString != NULL && _tcslen(pInfoString)) {
                    struct LANGANDCODEPAGE* lpTranslate = (struct LANGANDCODEPAGE*)pVal;
                    AddFileInfoValue(hDlg, pVersionData, lpTranslate[i],
                                     j, pInfoString);
                }
                ++j;
            }
//            lpTranslate = (struct LANGANDCODEPAGE*)pVal;
/*
            wsprintf(SubBlock, TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
                     lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            // Retrieve file description for language and code page "i". 
            if (VerQueryValue(pVersionData, SubBlock, &pVal, &nValLen)) {
                ListBox_InsertItemData(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_TYPES), i, _T("FileDescription"));
                SendMessage(GetDlgItem(hDlg, IDC_LIST_PROP_VERSION_VALUES), WM_SETTEXT, 0, pVal);
            }
 */
        }
    }
            }
#endif
        }
        free(pVersionData);
    }
}

BOOL CALLBACK PropertiesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static struct PropertiesDialog* dlg;
    SYSTEMTIME SystemTime;
    FILETIME LocalFileTime;
    TCHAR buffer[MAX_PATH];
    TCHAR text[100];
	int id;
    int offset;
    DWORD dwFileAttributes;
    Entry* entry;

	switch (message) {
	case WM_INITDIALOG:
		dlg = (struct PropertiesDialog*)lParam;
        ASSERT(dlg);
        entry = ((struct PropertiesDialog*)lParam)->pEntry;
        ASSERT(entry);

        GetWindowText(hDlg, text, sizeof(text)/sizeof(TCHAR));
        wsprintf(buffer, text, dlg->pEntry->data.cFileName);
        SetWindowText(hDlg, buffer);
        SetDlgItemText(hDlg, IDC_STATIC_PROP_FILENAME, dlg->pEntry->data.cFileName);
        SetDlgItemText(hDlg, IDC_STATIC_PROP_PATH, dlg->pEntry->data.cAlternateFileName);

        if (entry->bhfi_valid) {
            NUMBERFMT numFmt;
            memset(&numFmt, 0, sizeof(numFmt));
            numFmt.NumDigits = 0;
            numFmt.LeadingZero = 0;
            numFmt.Grouping = 3;
            numFmt.lpDecimalSep = _T(".");
            numFmt.lpThousandSep = _T(",");
            numFmt.NegativeOrder = 0;

            //entry->bhfi.nFileSizeLow;
            //entry->bhfi.nFileSizeHigh;
            //entry->bhfi.ftCreationTime
            wsprintf(buffer, _T("%u"), entry->bhfi.nFileSizeLow);
            if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, buffer, &numFmt, 
                    buffer + MAX_PATH/2, MAX_PATH/2)) {
                SetDlgItemText(hDlg, IDC_STATIC_PROP_SIZE, buffer + MAX_PATH/2);
            } else {
                SetDlgItemText(hDlg, IDC_STATIC_PROP_SIZE, buffer);
            }
        } else {
        }

        SetDlgItemText(hDlg, IDC_STATIC_PROP_LASTCHANGE, _T("Date?"));
        if (FileTimeToLocalFileTime(&entry->bhfi.ftLastWriteTime, &LocalFileTime)) {
            if (FileTimeToSystemTime(&LocalFileTime, &SystemTime)) {
                if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &SystemTime, NULL, buffer, sizeof(buffer)/sizeof(TCHAR))) {
//                    SetDlgItemText(hDlg, IDC_STATIC_PROP_LASTCHANGE, buffer);
                }
            }
        }
        _tcscat(buffer, _T("  "));
        offset = _tcslen(buffer);

        if (FileTimeToLocalFileTime(&entry->bhfi.ftLastWriteTime, &LocalFileTime)) {
            if (FileTimeToSystemTime(&LocalFileTime, &SystemTime)) {
                if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, buffer + offset, sizeof(buffer)/sizeof(TCHAR) - offset)) {
                    SetDlgItemText(hDlg, IDC_STATIC_PROP_LASTCHANGE, buffer);
                }
            }
        }

        dwFileAttributes = dlg->pEntry->bhfi.dwFileAttributes;
        Button_SetCheck(GetDlgItem(hDlg,IDC_CHECK_READONLY), dwFileAttributes & FILE_ATTRIBUTE_READONLY ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CHECK_ARCHIVE), dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CHECK_COMPRESSED), dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CHECK_HIDDEN), dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? BST_CHECKED : BST_UNCHECKED);
        Button_SetCheck(GetDlgItem(hDlg,IDC_CHECK_SYSTEM), dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ? BST_CHECKED : BST_UNCHECKED);

        CheckForFileInfo(hDlg, dlg->pEntry->data.cFileName);
		return 1;

	case WM_COMMAND:
		id = (int)wParam;
		if (id == IDOK) {
//            LPVOID lpData = NULL;                     // parameter for callback
//            DWORD dwFlags = MOVEFILE_COPY_ALLOWED;    // move options
//            GetDlgItemText(hDlg, , buffer, sizeof(buffer)/sizeof(TCHAR));
//            GetDlgItemText(hDlg, , buffer, sizeof(buffer)/sizeof(TCHAR));
			EndDialog(hDlg, id);
        } else if (id == IDCANCEL) {
			EndDialog(hDlg, id);
        } else {
			switch(HIWORD(wParam)) {
			case LBN_SELCHANGE:
                if (LOWORD(wParam) == IDC_LIST_PROP_VERSION_TYPES) {
                    SelectVersionString(hDlg, lParam);
                }
//                {
//				int idx = ListBox_GetCurSel(pane->hwnd);
//				Entry* entry = (Entry*) ListBox_GetItemData(pane->hwnd, idx);
//				if (pane == &child->left) set_curdir(child, entry);
//				else                      pane->cur = entry;
//                }
				break;
			case LBN_DBLCLK:
				//activate_entry(child, pane);
				break;
            }
        }
    	return 1;
	}
	return 0;
}
