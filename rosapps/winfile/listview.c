/*
 *  ReactOS winfile
 *
 *  listview.c
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
#include "listview.h"
#include "dialogs.h"
#include "utils.h"
#include "run.h"
#include "trace.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

extern HINSTANCE hInst;

static WNDPROC g_orgListWndProc;


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void init_output(HWND hWnd)
{
	TCHAR b[16];
	HFONT old_font;
	HDC hdc = GetDC(hWnd);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, _T("1000"), 0, b, 16) > 4)
		Globals.num_sep = b[1];
	else
		Globals.num_sep = _T('.');

	old_font = SelectFont(hdc, Globals.hFont);
	GetTextExtentPoint32(hdc, _T(" "), 1, &Globals.spaceSize);
	SelectFont(hdc, old_font);
	ReleaseDC(hWnd, hdc);
}

static void AddEntryToList(HWND hwndLV, int idx, Entry* entry)
{ 
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_PARAM; 
    item.iItem = 0;//idx; 
    item.iSubItem = 0; 
    item.state = 0; 
    item.stateMask = 0; 
//    item.pszText = entry->data.cFileName; 
    item.pszText = LPSTR_TEXTCALLBACK; 
//    item.cchTextMax = strlen(entry->data.cFileName); 
    item.cchTextMax = 0; 
    item.iImage = 0; 
//    item.iImage = I_IMAGECALLBACK; 
    item.lParam = (LPARAM)entry;
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif
    ListView_InsertItem(hwndLV, &item);
}

// insert listctrl entries after index idx
static void InsertListEntries(HWND hWnd, Entry* entry, int idx)
{
  	ShowWindow(hWnd, SW_HIDE);

    if (idx == -1) {
    }
    idx = 0;

	for (; entry; entry = entry->next) {
#ifndef _LEFT_FILES
    	if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    	continue;
#endif
        //ListBox_InsertItemData(hWnd, idx, entry);
        AddEntryToList(hWnd, idx, entry); 
        ++idx;
    }
    ShowWindow(hWnd, SW_SHOW);
}

#define MAX_LIST_COLUMNS 5
static int default_column_widths[MAX_LIST_COLUMNS] = { 175, 100, 100, 100, 70 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_RIGHT, LVCFMT_RIGHT, LVCFMT_RIGHT, LVCFMT_LEFT };

static void CreateListColumns(HWND hWndListView)
{
    TCHAR szText[50];
    int index;
    LV_COLUMN lvC;
 
    // Create columns.
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    // Load the column labels from the resource file.
    for (index = 0; index < MAX_LIST_COLUMNS; index++) {
        lvC.iSubItem = index;
        lvC.cx = default_column_widths[index];
        lvC.fmt = column_alignment[index];
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, sizeof(szText));
        if (ListView_InsertColumn(hWndListView, index, &lvC) == -1) {
            // TODO: handle failure condition...
            break;
        }
    }
}

static HWND CreateListView(HWND hwndParent, int id) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndLV;    // handle to list view control 

    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(0, WC_LISTVIEW, _T("List View"), 
//        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER, 
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 

    // Initialize the image list, and add items to the control. 
/*
    if (!InitListViewImageLists(hwndLV) || 
            !InitListViewItems(hwndLV, lpszPathName)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
 */
    ListView_SetExtendedListViewStyle(hwndLV,  LVS_EX_FULLROWSELECT);
    CreateListColumns(hwndLV);

    return hwndLV;
} 

/*
int GetNumberFormat(
  LCID Locale,                // locale
  DWORD dwFlags,              // options
  LPCTSTR lpValue,            // input number string
  CONST NUMBERFMT *lpFormat,  // formatting information
  LPTSTR lpNumberStr,         // output buffer
  int cchNumber               // size of output buffer
);
 */
/*
typedef struct _numberfmt { 
  UINT      NumDigits; 
  UINT      LeadingZero; 
  UINT      Grouping; 
  LPTSTR    lpDecimalSep; 
  LPTSTR    lpThousandSep; 
  UINT      NegativeOrder; 
} NUMBERFMT, *LPNUMBERFMT; 
 */
/*
typedef struct _BY_HANDLE_FILE_INFORMATION {
  DWORD    dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    dwVolumeSerialNumber; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    nNumberOfLinks; 
  DWORD    nFileIndexHigh; 
  DWORD    nFileIndexLow; 
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION;  

GetDriveTypeW
GetFileType
GetLocaleInfoW
GetNumberFormatW

BOOL FileTimeToLocalFileTime(
  CONST FILETIME *lpFileTime,  // UTC file time to convert
  LPFILETIME lpLocalFileTime   // converted file time
);

BOOL FileTimeToSystemTime(
  CONST FILETIME *lpFileTime,  // file time to convert
  LPSYSTEMTIME lpSystemTime    // receives system time
);
 */ 

// OnGetDispInfo - processes the LVN_GETDISPINFO 
// notification message. 
 
static void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    SYSTEMTIME SystemTime;
    FILETIME LocalFileTime;
    static TCHAR buffer[200];

//    LVITEM* pItem = &(plvdi->item);
//    Entry* entry = (Entry*)pItem->lParam;
    Entry* entry = (Entry*)plvdi->item.lParam;
    ASSERT(entry);

    plvdi->item.pszText = NULL;

    switch (plvdi->item.iSubItem) {
    case 0:
        plvdi->item.pszText = entry->data.cFileName; 
//    item.cchTextMax = strlen(entry->data.cFileName); 
//        plvdi->item.pszText = rgPetInfo[plvdi->item.iItem].szKind;
        break;
    case 1:
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
                    buffer + sizeof(buffer)/2, sizeof(buffer)/2)) {
                plvdi->item.pszText = buffer + sizeof(buffer)/2;
            } else {
                plvdi->item.pszText = buffer;
            }
        } else {
            plvdi->item.pszText = _T("unknown");
        }
        break;
    case 2:
        plvdi->item.pszText = _T("error");
        if (FileTimeToLocalFileTime(&entry->bhfi.ftLastWriteTime, &LocalFileTime)) {
            if (FileTimeToSystemTime(&LocalFileTime, &SystemTime)) {
                if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &SystemTime, NULL, buffer, sizeof(buffer))) {
                    plvdi->item.pszText = buffer;
                }
            }
        }
        break;
    case 3:
        plvdi->item.pszText = _T("error");
        if (FileTimeToLocalFileTime(&entry->bhfi.ftLastWriteTime, &LocalFileTime)) {
            if (FileTimeToSystemTime(&LocalFileTime, &SystemTime)) {
//                if (GetTimeFormat(UserDefaultLCID, 0, &SystemTime, NULL, buffer, sizeof(buffer))) {
                if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, buffer, sizeof(buffer))) {
                    plvdi->item.pszText = buffer;
                }
            }
        }
        break;
    case 4:
        plvdi->item.pszText = _T("");
        _tcscpy(buffer, _T(" "));

        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) _tcscat(buffer, _T("a")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) _tcscat(buffer, _T("c")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) _tcscat(buffer, _T("d")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) _tcscat(buffer, _T("e")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) _tcscat(buffer, _T("h")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) _tcscat(buffer, _T("n")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) _tcscat(buffer, _T("o")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY) _tcscat(buffer, _T("r")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) _tcscat(buffer, _T("p")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) _tcscat(buffer, _T("f")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) _tcscat(buffer, _T("s")); else _tcscat(buffer, _T(" "));
        if (entry->bhfi.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) _tcscat(buffer, _T("t")); else _tcscat(buffer, _T(" "));
        plvdi->item.pszText = buffer;
        break;
    default:
        _tcscpy(buffer, _T(" "));
        plvdi->item.pszText = buffer;
        break;
    }
} 
/*
FILE_ATTRIBUTE_ARCHIVE The file or directory is an archive file. Applications use this attribute to mark files for backup or removal. 
FILE_ATTRIBUTE_COMPRESSED The file or directory is compressed. For a file, this means that all of the data in the file is compressed. For a directory, this means that compression is the default for newly created files and subdirectories. 
FILE_ATTRIBUTE_DIRECTORY The handle identifies a directory. 
FILE_ATTRIBUTE_ENCRYPTED The file or directory is encrypted. For a file, this means that all data in the file is encrypted. For a directory, this means that encryption is the default for newly created files and subdirectories. 
FILE_ATTRIBUTE_HIDDEN The file or directory is hidden. It is not included in an ordinary directory listing. 
FILE_ATTRIBUTE_NORMAL The file has no other attributes. This attribute is valid only if used alone. 
FILE_ATTRIBUTE_OFFLINE The file data is not immediately available. This attribute indicates that the file data has been physically moved to offline storage. This attribute is used by Remote Storage, the hierarchical storage management software in Windows 2000. Applications should not arbitrarily change this attribute. 
FILE_ATTRIBUTE_READONLY The file or directory is read-only. Applications can read the file but cannot write to it or delete it. In the case of a directory, applications cannot delete it. 
FILE_ATTRIBUTE_REPARSE_POINT The file has an associated reparse point. 
FILE_ATTRIBUTE_SPARSE_FILE The file is a sparse file. 
FILE_ATTRIBUTE_SYSTEM The file or directory is part of the operating system or is used exclusively by the operating system. 
FILE_ATTRIBUTE_TEMPORARY The file is being used for temporary storage. File systems attempt to keep all the data in memory for quicker access, rather than flushing the data back to mass storage. A temporary file should be deleted by the application as soon as it is no longer needed.
 */


 // OnEndLabelEdit - processes the LVN_ENDLABELEDIT 
 // notification message. 
 // Returns TRUE if the label is changed, or FALSE otherwise. 

static BOOL OnEndLabelEdit(NMLVDISPINFO* plvdi)
{ 
    if (plvdi->item.iItem == -1) 
        return FALSE; 
 
    // Copy the new label text to the application-defined structure. 
//    lstrcpyn(rgPetInfo[plvdi->item.iItem].szKind, plvdi->item.pszText, 10);
    
    return TRUE;
    // To make a more robust application you should send an EM_LIMITTEXT
    // message to the edit control to prevent the user from entering too
    // many characters in the field. 
} 

/*
typedef struct _BY_HANDLE_FILE_INFORMATION {
  DWORD    dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    dwVolumeSerialNumber; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    nNumberOfLinks; 
  DWORD    nFileIndexHigh; 
  DWORD    nFileIndexLow; 
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION;  
 */
/*
typedef struct _WIN32_FIND_DATA {
  DWORD    dwFileAttributes; 
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
  DWORD    nFileSizeHigh; 
  DWORD    nFileSizeLow; 
  DWORD    dwReserved0; 
  DWORD    dwReserved1; 
  TCHAR    cFileName[ MAX_PATH ]; 
  TCHAR    cAlternateFileName[ 14 ]; 
} WIN32_FIND_DATA, *PWIN32_FIND_DATA; 
 */

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    Entry* pItem1 = (Entry*)lParam1;
    Entry* pItem2 = (Entry*)lParam2;

    if (pItem1 != NULL && pItem2 != NULL) {
		switch (lParamSort) {
        case ID_VIEW_SORT_BY_NAME:
            return _tcscmp(pItem1->data.cFileName, pItem2->data.cFileName);
            break;
        case ID_VIEW_SORT_BY_TYPE:
//            if (pItem1->bhfi.nFileSizeLow != pItem2->bhfi.nFileSizeLow) {
//                return (pItem1->bhfi.nFileSizeLow < pItem2->bhfi.nFileSizeLow) ? -1 : 1;
//            }
            break;
        case ID_VIEW_SORT_BY_SIZE:
            if (pItem1->bhfi.nFileSizeLow != pItem2->bhfi.nFileSizeLow) {
                return (pItem1->bhfi.nFileSizeLow < pItem2->bhfi.nFileSizeLow) ? -1 : 1;
            }
            break;
        case ID_VIEW_SORT_BY_DATE:
            return CompareFileTime(&pItem1->bhfi.ftLastWriteTime, &pItem2->bhfi.ftLastWriteTime);
            break;
		}
    }
    return 0;
}

static void CmdSortItems(HWND hWnd, UINT cmd)
{
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_BY_NAME, MF_BYCOMMAND);
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_BY_TYPE, MF_BYCOMMAND);
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_BY_SIZE, MF_BYCOMMAND);
	CheckMenuItem(Globals.hMenuView, ID_VIEW_SORT_BY_DATE, MF_BYCOMMAND);
    ListView_SortItems(hWnd, &CompareFunc, cmd);
    CheckMenuItem(Globals.hMenuView, cmd, MF_BYCOMMAND | MF_CHECKED);
}

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT cmd = LOWORD(wParam);
	//HWND hChildWnd;

    if (1) {
		switch (cmd) {
        case ID_FILE_OPEN:
            {
            LVITEM item;
            item.mask = LVIF_PARAM;
//            UINT selected_count = ListView_GetSelectedCount(hWnd);

            item.iItem = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
            if (item.iItem != -1) {
                if (ListView_GetItem(hWnd, &item)) {
                    Entry* entry = (Entry*)item.lParam;
                    OpenTarget(hWnd, entry->data.cFileName);
                }
            }
            }

            break;
        case ID_FILE_MOVE:
            //OnFileMove(hWnd);
            break;
        case ID_FILE_COPY:
        case ID_FILE_COPY_CLIPBOARD:
        case ID_FILE_DELETE:
        case ID_FILE_RENAME:
        case ID_FILE_PROPERTIES:
        case ID_FILE_COMPRESS:
        case ID_FILE_UNCOMPRESS:
            break;
        case ID_FILE_RUN:
            OnFileRun();
            break;
        case ID_FILE_PRINT:
        case ID_FILE_ASSOCIATE:
        case ID_FILE_CREATE_DIRECTORY:
            break;
        case ID_VIEW_SORT_BY_NAME:
        case ID_VIEW_SORT_BY_TYPE:
        case ID_VIEW_SORT_BY_SIZE:
        case ID_VIEW_SORT_BY_DATE:
            CmdSortItems(hWnd, cmd);
            break;
		default:
            return FALSE;
		}
	}
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

static LRESULT CALLBACK ListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
	ASSERT(child);

	switch (message) {
/*
    case WM_CREATE:
        //CreateListView(hWnd);
        return 0;
 */
	case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        }
		break;
	case WM_DISPATCH_COMMAND:
		return _CmdWndProc(hWnd, message, wParam, lParam);
		break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) { 
        case LVN_GETDISPINFO: 
            OnGetDispInfo((NMLVDISPINFO*)lParam); 
            break; 
        case NM_DBLCLK:
            {
            NMITEMACTIVATE* nmitem = (LPNMITEMACTIVATE)lParam;
            LVHITTESTINFO info;

            if (nmitem->hdr.hwndFrom != hWnd) break; 
//            if (nmitem->hdr.idFrom != IDW_LISTVIEW) break; 
//            if (nmitem->hdr.code != ???) break; 
            switch (nmitem->uKeyFlags) {
            case LVKF_ALT:     //  The ALT key is pressed.  
                // properties dialog box ?
                break;
            case LVKF_CONTROL: //  The CTRL key is pressed.
                // run dialog box for providing parameters...
                break;
            case LVKF_SHIFT:   //  The SHIFT key is pressed.   
                break;
            }
            info.pt.x = nmitem->ptAction.x;
            info.pt.y = nmitem->ptAction.y;
            if (ListView_HitTest(hWnd, &info) != -1) {
                LVITEM item;
                item.mask = LVIF_PARAM;
                item.iItem = info.iItem;
                if (ListView_GetItem(hWnd, &item)) {
                    Entry* entry = (Entry*)item.lParam;
                    OpenTarget(hWnd, entry->data.cFileName);
                }
            }
            }
            break;

        case LVN_ENDLABELEDIT: 
            return OnEndLabelEdit((NMLVDISPINFO*)lParam);
            break;
        default:
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        }
//        return 0;
		break;
/*
		case WM_SETFOCUS:
			child->nFocusPanel = pane==&child->right? 1: 0;
			ListBox_SetSel(hWnd, TRUE, 1);
			//TODO: check menu items
			break;
		case WM_KEYDOWN:
			if (wParam == VK_TAB) {
				//TODO: SetFocus(Globals.hDriveBar)
				SetFocus(child->nFocusPanel? child->left.hWnd: child->right.hWnd);
			}
            break;
 */
        default:
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
            break;
	}
	return 0;
}


void CreateListWnd(HWND parent, Pane* pane, int id, LPTSTR lpszPathName)
{
//	static int s_init = 0;
	Entry* entry = pane->root;

	pane->treePane = 0;
#if 1
    pane->hWnd = CreateListView(parent, id);
#else
	pane->hWnd = CreateWindow(_T("ListBox"), _T(""), WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
								LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
								0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);
#endif
	SetWindowLong(pane->hWnd, GWL_USERDATA, (LPARAM)pane);
	g_orgListWndProc = SubclassWindow(pane->hWnd, ListWndProc);
	SendMessage(pane->hWnd, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into listbox
    if (entry) {
		InsertListEntries(pane->hWnd, entry, -1);
    }

	 // calculate column widths
//	if (!s_init) {
//		s_init = 1;
//		init_output(pane->hWnd);
//	}
//	calc_widths(pane, TRUE);
}

void RefreshList(HWND hWnd, Entry* entry)
{
    if (hWnd != NULL) {
        ListView_DeleteAllItems(hWnd);
        if (entry != NULL) {
            //TRACE("RefreshList(...) entry name: %s\n", entry->data.cFileName);
    	    InsertListEntries(hWnd, entry, -1);
        }
    }
}

/*
	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
    if (pane != NULL) {
//        ListBox_RemoveAll(hWnd, TRUE, 1);
        ListView_DeleteAllItems(pane->hWnd);
        if (entry) {
	    	InsertListEntries(pane->hWnd, entry, -1);
        }
    }
 */

