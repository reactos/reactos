/*
 * ReactOS Explorer
 *
 * Copyright 2015 Jared Smudde <computerwhiz02@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

#define BITMAP_WIDTH 16
#define BITMAP_HEIGHT 16

#define IDC_NOTIFY_OPTIONS 5000

typedef struct
{
    DWORD hwnd;
    DWORD uId;
    DWORD uCallbackMsg;
    DWORD unkow[2];
    HICON hIcon;
    WCHAR text[64];
}TRAYDATA;

// Returns TRUE if successful, and FALSE otherwise. 
BOOL AddListViewColumn(HWND hWndListView, LPWSTR label, INT iCol, int w)
{
    LVCOLUMNW lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.iSubItem = iCol;
    lvc.pszText = label;
    lvc.cchTextMax = lstrlenW(label);
    lvc.cx = w;
    lvc.fmt = LVCFMT_CENTER;

    return (ListView_InsertColumn(hWndListView, iCol, &lvc) != -1);
}

// Returns TRUE if successful, and FALSE otherwise. 
BOOL InsertItem(HWND hListView, TBBUTTON *tb, TRAYDATA *data, int id)
{
    LVITEMW iItem;

    iItem.mask = LVIF_TEXT | LVIF_IMAGE;
    iItem.pszText = data->text;
    iItem.cchTextMax = lstrlenW(data->text);
    iItem.iSubItem = 0;
    iItem.iItem = id;
    iItem.iImage = id;

    return (ListView_InsertItem(hListView, &iItem) != -1);
}

// Return Toolbar Window hander on successful, Null otherwise
HWND FindTrayToolbarWindowHandle()
{
    HWND hwnd;
    if ((hwnd = FindWindowW(L"Shell_TrayWnd", NULL)))
        if ((hwnd = FindWindowExW(hwnd, NULL, L"TrayNotifyWnd", NULL)))
            if ((hwnd = FindWindowExW(hwnd, NULL, L"SysPager", NULL)))
                if ((hwnd = FindWindowExW(hwnd, NULL, L"ToolbarWindow32", NULL)))
                    return hwnd;

    return NULL;
}

BOOL ExtractData(HWND hListView, HIMAGELIST hImageList)
{
    const int BUFFER_SIZE = 0x1000;
    VOID* pLocalBuffer = (VOID*)new CHAR[BUFFER_SIZE];

    HWND hToolbar = FindTrayToolbarWindowHandle();
    int count = SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0);

    DWORD processId = 0;
    GetWindowThreadProcessId(hToolbar, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess)
        return FALSE;

    VOID *pRemoteBuffer = VirtualAllocEx(hProcess, NULL, BUFFER_SIZE, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteBuffer)
        return FALSE;

    ZeroMemory(pLocalBuffer, BUFFER_SIZE);

    hImageList = ImageList_Create(BITMAP_WIDTH, BITMAP_HEIGHT, FALSE, count, 0);

    for (int i = 0; i < count; i++)
    {
        DWORD pBytesRead;

        // TBBUTTON
        if (SendMessageW(hToolbar, TB_GETBUTTON, i, (LPARAM)pRemoteBuffer))
        {
            pBytesRead = 0;
            if (ReadProcessMemory(hProcess, pRemoteBuffer, pLocalBuffer, sizeof(TBBUTTON), &pBytesRead))
            {
         //  dwData
                pBytesRead = 0;
                TBBUTTON *tbb = reinterpret_cast<TBBUTTON*>(pLocalBuffer);
                TRAYDATA *pData = reinterpret_cast<TRAYDATA*>(((CHAR*)pLocalBuffer) + sizeof(TBBUTTON));
                if (ReadProcessMemory(hProcess, (VOID*)tbb->dwData, pData, sizeof(TRAYDATA), &pBytesRead))
                {
                    HICON hIcon = CopyIcon(pData->hIcon);
                    if (hIcon)
                        ImageList_AddIcon(hImageList, hIcon);

                    InsertItem(hListView, tbb, pData, i);
                }
            }
        }
    }
    ListView_SetImageList(hListView, hImageList, LVSIL_SMALL);

    VirtualFree(pRemoteBuffer, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    delete (CHAR*)pLocalBuffer;
    return TRUE;
}

INT_PTR CALLBACK CustomizeNotifyIconsProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HWND hListView = NULL;
    static HWND hCombo = NULL;
    static HIMAGELIST hImageList;

    switch (Message)
    {
        case WM_INITDIALOG:
        {
            WCHAR szText[64];

            hListView = GetDlgItem(hwnd, IDC_NOTIFICATION_LIST);
            hCombo = CreateWindowExW(0, L"COMBOBOX", L"", CBS_DROPDOWN | WS_CHILD | WS_BORDER,
                                     0, 0, 180, 50, hListView, (HMENU)IDC_NOTIFY_OPTIONS, NULL, 0);

            HFONT hf = (HFONT)SendMessage(hListView, WM_GETFONT, 0, 0);
            if (hf)
                SendMessage(hCombo, WM_SETFONT, (WPARAM)hf, (LPARAM)TRUE);

            LoadStringW(NULL, IDS_NOTIFY_SHOWALL, szText, sizeof(szText)/sizeof(szText[0]));
            SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)szText);
            LoadStringW(NULL, IDS_NOTIFY_HIDEALL, szText, sizeof(szText)/sizeof(szText[0]));
            SendMessageW(hCombo, CB_ADDSTRING, 1, (LPARAM)szText);
            LoadStringW(NULL, IDS_NOTIFY_SHOWNOTIFY, szText, sizeof(szText)/sizeof(szText[0]));
            SendMessageW(hCombo, CB_ADDSTRING, 2, (LPARAM)szText);
            SendMessageW(hCombo, CB_SETCURSEL, 0, 0);

            ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

            LoadStringW(NULL, IDS_NOTIFY_COLUMN1, szText, sizeof(szText)/sizeof(szText[0]));
            AddListViewColumn(hListView, szText, 0, 146);
            LoadStringW(NULL, IDS_NOTIFY_COLUMN2, szText, sizeof(szText)/sizeof(szText[0]));
            AddListViewColumn(hListView, szText, 1, 200);

            ExtractData(hListView, hImageList);

            return TRUE;
        }
        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->idFrom == IDC_NOTIFICATION_LIST)
            {
                switch (((LPNMHDR)lParam)->code)
                {
                    case LVN_ITEMCHANGED:
                    {
                        RECT rc;
                        NM_LISTVIEW *nmlv = (NM_LISTVIEW*)lParam;
                        ListView_GetSubItemRect(nmlv->hdr.hwndFrom, nmlv->iItem, 1, LVIR_LABEL, &rc);
                        SetWindowPos(hCombo, NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW);
                        return FALSE;
                    }
                }
            }
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    ListView_DeleteAllItems(hListView);
                    ImageList_Destroy(hImageList);
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

VOID ShowCustomizeNotifyIcons(HINSTANCE hInst, HWND hExplorer)
{
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_NOTIFICATIONS_CUSTOMIZE), hExplorer, CustomizeNotifyIconsProc);
}
