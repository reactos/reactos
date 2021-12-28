/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/environment.c
 * PURPOSE:     Environment variable settings
 * COPYRIGHT:   Copyright Eric Kohl
 *              Copyright 2021 Arnav Bhatt <arnavbhatt288@gmail.com>
 */

#include "precomp.h"
#include <commctrl.h>
#include <commdlg.h>
#include <string.h>
#include <strsafe.h>

#define MAX_STR_LENGTH  128

typedef struct _VARIABLE_DATA
{
    DWORD dwType;
    LPTSTR lpName;
    LPTSTR lpRawValue;
    LPTSTR lpCookedValue;
} VARIABLE_DATA, *PVARIABLE_DATA;

typedef struct _ENVIRONMENT_DIALOG_DATA
{
    DWORD cxMin;
    DWORD cyMin;
    DWORD cxOld;
    DWORD cyOld;
} ENVIRONMENT_DIALOG_DATA, *PENVIRONMENT_DIALOG_DATA;

typedef struct _ENVIRONMENT_EDIT_DIALOG_DATA
{
    BOOL bIsItemSelected;
    DWORD dwSelectedValueIndex;
    DWORD cxMin;
    DWORD cyMin;
    DWORD cxOld;
    DWORD cyOld;
    DWORD dwDlgID;
    HWND hEditBox;
    PVARIABLE_DATA VarData;
} EDIT_DIALOG_DATA, *PEDIT_DIALOG_DATA;

static BOOL
DetermineDialogBoxType(LPTSTR lpRawValue)
{
    DWORD dwValueLength;
    LPTSTR lpTemp;
    LPTSTR lpToken;

    dwValueLength = _tcslen(lpRawValue) + 1;
    lpTemp = GlobalAlloc(GPTR, dwValueLength * sizeof(TCHAR));
    if (!lpTemp)
        return FALSE;

    StringCchCopy(lpTemp, dwValueLength, lpRawValue);

    for (lpToken = _tcstok(lpTemp, _T(";"));
         lpToken != NULL;
         lpToken = _tcstok(NULL, _T(";")))
    {
        /* If the string has environment variable then expand it */
        ExpandEnvironmentStrings(lpToken, lpToken, dwValueLength);

        if (!PathIsDirectoryW(lpToken))
            return FALSE;
    }
    GlobalFree(lpTemp);

    return TRUE;
}

static DWORD
GatherDataFromEditBox(HWND hwndDlg,
                      PVARIABLE_DATA VarData)
{
    DWORD dwNameLength;
    DWORD dwValueLength;

    dwNameLength = SendDlgItemMessage(hwndDlg, IDC_VARIABLE_NAME, WM_GETTEXTLENGTH, 0, 0);
    dwValueLength = SendDlgItemMessage(hwndDlg, IDC_VARIABLE_VALUE, WM_GETTEXTLENGTH, 0, 0);

    if (dwNameLength == 0 || dwValueLength == 0)
    {
        return 0;
    }

    /* Reallocate the name buffer, regrowing it if necessary */
    if (!VarData->lpName || (_tcslen(VarData->lpName) < dwNameLength))
    {
        if (VarData->lpName)
            GlobalFree(VarData->lpName);

        VarData->lpName = GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
        if (!VarData->lpName)
            return 0;
    }
    SendDlgItemMessage(hwndDlg, IDC_VARIABLE_NAME, WM_GETTEXT, dwNameLength + 1, (LPARAM)VarData->lpName);

    /* Reallocate the value buffer, regrowing it if necessary */
    if (!VarData->lpRawValue || (_tcslen(VarData->lpRawValue) < dwValueLength))
    {
        if (VarData->lpRawValue)
            GlobalFree(VarData->lpRawValue);

        VarData->lpRawValue = GlobalAlloc(GPTR, (dwValueLength + 1) * sizeof(TCHAR));
        if (!VarData->lpRawValue)
            return 0;
    }
    SendDlgItemMessage(hwndDlg, IDC_VARIABLE_VALUE, WM_GETTEXT, dwValueLength + 1, (LPARAM)VarData->lpRawValue);

    return dwValueLength;
}

static DWORD
GatherDataFromListView(HWND hwndListView,
                       PVARIABLE_DATA VarData)
{
    DWORD dwValueLength;
    DWORD NumberOfItems;
    DWORD i;
    TCHAR szData[MAX_PATH];

    /* Gather the number of items for the semi-colon */
    NumberOfItems = ListView_GetItemCount(hwndListView);
    if (NumberOfItems == 0)
    {
        return 0;
    }

    /* Since the last item doesn't need the semi-colon subtract 1 */
    dwValueLength = NumberOfItems - 1;

    for (i = 0; i < NumberOfItems; i++)
    {
        ListView_GetItemText(hwndListView,
                             i,
                             0,
                             szData,
                             _countof(szData));
        dwValueLength += _tcslen(szData);
    }

    /* Reallocate the value buffer, regrowing it if necessary */
    if (!VarData->lpRawValue || (_tcslen(VarData->lpRawValue) < dwValueLength))
    {
        if (VarData->lpRawValue)
            GlobalFree(VarData->lpRawValue);

        VarData->lpRawValue = GlobalAlloc(GPTR, (dwValueLength + 1) * sizeof(TCHAR));
        if (!VarData->lpRawValue)
            return 0;
    }

    /* Copy the variable values while seperating them with a semi-colon except for the last value */
    for (i = 0; i < NumberOfItems; i++)
    {
        if (i > 0)
        {
            StringCchCat(VarData->lpRawValue, dwValueLength + 1, _T(";"));
        }
        ListView_GetItemText(hwndListView,
                             i,
                             0,
                             szData,
                             _countof(szData));
        StringCchCat(VarData->lpRawValue, dwValueLength + 1, szData);
    }
    return dwValueLength;
}

static INT
GetSelectedListViewItem(HWND hwndListView)
{
    INT iCount;
    INT iItem;

    iCount = SendMessage(hwndListView,
                         LVM_GETITEMCOUNT,
                         0,
                         0);
    if (iCount != LB_ERR)
    {
        for (iItem = 0; iItem < iCount; iItem++)
        {
            if (SendMessage(hwndListView,
                            LVM_GETITEMSTATE,
                            iItem,
                            (LPARAM) LVIS_SELECTED) == LVIS_SELECTED)
            {
                return iItem;
            }
        }
    }

    return -1;
}

static LRESULT CALLBACK
ListViewSubclassProc(HWND hListBox,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam,
                    UINT_PTR uIdSubclass,
                    DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
        case WM_DESTROY:
        {
            RemoveWindowSubclass(hListBox, ListViewSubclassProc, uIdSubclass);
            break;
        }

        /* Whenever the control is resized make sure it doesn't spawn the horizontal scrollbar */
        case WM_SIZE:
        {
            ShowScrollBar(hListBox, SB_HORZ, FALSE);
            break;
        }
    }

    return DefSubclassProc(hListBox, uMsg, wParam, lParam);
}

static VOID
AddEmptyItem(HWND hwndListView,
             DWORD dwSelectedValueIndex)
{
    LV_ITEM lvi;

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.cchTextMax = MAX_PATH;
    lvi.pszText = _T("");
    lvi.iItem = dwSelectedValueIndex;
    lvi.iSubItem = 0;
    ListView_InsertItem(hwndListView, &lvi);
}

static VOID
AddValuesToList(HWND hwndDlg,
                PEDIT_DIALOG_DATA DlgData)
{
    LV_COLUMN column;
    LV_ITEM lvi;
    RECT rItem;

    DWORD dwValueLength;
    DWORD i;
    HWND hwndListView;
    LPTSTR lpTemp;
    LPTSTR lpToken;

    ZeroMemory(&column, sizeof(column));
    ZeroMemory(&lvi, sizeof(lvi));

    hwndListView = GetDlgItem(hwndDlg, IDC_LIST_VARIABLE_VALUE);

    GetClientRect(hwndListView, &rItem);

    column.mask = LVCF_WIDTH;
    column.cx = rItem.right;
    ListView_InsertColumn(hwndListView, 0, &column);
    ShowScrollBar(hwndListView, SB_HORZ, FALSE);

    lvi.mask = LVIF_TEXT | LVIF_STATE;
    lvi.cchTextMax = MAX_PATH;
    lvi.iSubItem = 0;

    dwValueLength = _tcslen(DlgData->VarData->lpRawValue) + 1;
    lpTemp = GlobalAlloc(GPTR, dwValueLength * sizeof(TCHAR));
    if (!lpTemp)
        return;

    StringCchCopy(lpTemp, dwValueLength, DlgData->VarData->lpRawValue);

    for (lpToken = _tcstok(lpTemp, _T(";")), i = 0;
         lpToken != NULL;
         lpToken = _tcstok(NULL, _T(";")), i++)
    {
        lvi.iItem = i;
        lvi.pszText = lpToken;
        lvi.state = (i == 0) ? LVIS_SELECTED : 0;
        ListView_InsertItem(hwndListView, &lvi);
    }

    DlgData->dwSelectedValueIndex = 0;
    ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
    ListView_SetItemState(hwndListView, DlgData->dwSelectedValueIndex,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    ListView_Update(hwndListView, DlgData->dwSelectedValueIndex);
    GlobalFree(lpTemp);
}

static VOID
BrowseRequiredFile(HWND hwndDlg)
{
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH] = _T("");

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = _T("All Files (*.*)\0*.*\0");
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileName(&ofn))
    {
        SetDlgItemText(hwndDlg, IDC_VARIABLE_VALUE, szFile);
    }
}

static VOID
BrowseRequiredFolder(HWND hwndDlg,
                     PEDIT_DIALOG_DATA DlgData)
{
    HWND hwndListView;
    TCHAR szDir[MAX_PATH];

    BROWSEINFO bi;
    LPITEMIDLIST pidllist;

    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hwndDlg;
    bi.ulFlags = BIF_NEWDIALOGSTYLE;

    hwndListView = GetDlgItem(hwndDlg, IDC_LIST_VARIABLE_VALUE);

    pidllist = SHBrowseForFolder(&bi);
    if (!pidllist)
    {
        return;
    }

    if (SHGetPathFromIDList(pidllist, szDir))
    {
        if (DlgData->dwDlgID == IDD_EDIT_VARIABLE_FANCY)
        {
            /* If no item is selected then create a new empty item and add the required location to it */
            if (!DlgData->bIsItemSelected)
            {
                DlgData->dwSelectedValueIndex = ListView_GetItemCount(hwndListView);
                AddEmptyItem(hwndListView, DlgData->dwSelectedValueIndex);
            }
            ListView_SetItemText(hwndListView,
                                 DlgData->dwSelectedValueIndex,
                                 0,
                                 szDir);
            ListView_SetItemState(hwndListView, DlgData->dwSelectedValueIndex,
                                  LVIS_FOCUSED | LVIS_SELECTED,
                                  LVIS_FOCUSED | LVIS_SELECTED);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_VARIABLE_VALUE, szDir);
        }
    }

    CoTaskMemFree(pidllist);
}

static VOID
MoveListItem(HWND hwndDlg,
             PEDIT_DIALOG_DATA DlgData,
             BOOL bMoveUp)
{
    TCHAR szDest[MAX_PATH];
    TCHAR szSource[MAX_PATH];
    HWND hwndListView;
    DWORD dwSrcIndex, dwDestIndex, dwLastIndex;

    hwndListView = GetDlgItem(hwndDlg, IDC_LIST_VARIABLE_VALUE);

    dwLastIndex = ListView_GetItemCount(hwndListView) - 1;
    dwSrcIndex = DlgData->dwSelectedValueIndex;
    dwDestIndex = bMoveUp ? (dwSrcIndex - 1) : (dwSrcIndex + 1);

    if ((bMoveUp && dwSrcIndex > 0) || (!bMoveUp && dwSrcIndex < dwLastIndex))
    {
        ListView_GetItemText(hwndListView,
                             dwSrcIndex,
                             0,
                             szDest,
                             _countof(szDest));
        ListView_GetItemText(hwndListView,
                             dwDestIndex,
                             0,
                             szSource,
                             _countof(szSource));

        ListView_SetItemText(hwndListView,
                             dwDestIndex,
                             0,
                             szDest);
        ListView_SetItemText(hwndListView,
                             dwSrcIndex,
                             0,
                             szSource);

        DlgData->dwSelectedValueIndex = dwDestIndex;
        ListView_SetItemState(hwndListView, DlgData->dwSelectedValueIndex,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
    }
}

static VOID
OnEnvironmentEditDlgResize(HWND hwndDlg,
                           PEDIT_DIALOG_DATA DlgData,
                           DWORD cx,
                           DWORD cy)
{
    RECT rect;
    HDWP hdwp = NULL;
    HWND hItemWnd;

    if ((cx == DlgData->cxOld) && (cy == DlgData->cyOld))
        return;

    if (DlgData->dwDlgID == IDD_EDIT_VARIABLE)
    {
        hdwp = BeginDeferWindowPos(5);

        /* For the edit control */
        hItemWnd = GetDlgItem(hwndDlg, IDC_VARIABLE_NAME);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  0, 0,
                                  (rect.right - rect.left) + (cx - DlgData->cxOld),
                                  rect.bottom - rect.top,
                                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_VARIABLE_VALUE);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  0, 0,
                                  (rect.right - rect.left) + (cx - DlgData->cxOld),
                                  rect.bottom - rect.top,
                                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    else if (DlgData->dwDlgID == IDD_EDIT_VARIABLE_FANCY)
    {
        hdwp = BeginDeferWindowPos(11);

        /* For the list view control */
        hItemWnd = GetDlgItem(hwndDlg, IDC_LIST_VARIABLE_VALUE);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  0, 0,
                                  (rect.right - rect.left) + (cx - DlgData->cxOld),
                                  (rect.bottom - rect.top) + (cy - DlgData->cyOld),
                                  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            ListView_SetColumnWidth(hItemWnd, 0, (rect.right - rect.left) + (cx - DlgData->cxOld));
        }

        /* For the buttons */
        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_BROWSE_FOLDER);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_NEW);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_EDIT);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_DELETE);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_MOVE_UP);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_MOVE_DOWN);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

        hItemWnd = GetDlgItem(hwndDlg, IDC_BUTTON_EDIT_TEXT);
        GetWindowRect(hItemWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

        if (hdwp)
        {
            hdwp = DeferWindowPos(hdwp,
                                  hItemWnd,
                                  NULL,
                                  rect.left + (cx - DlgData->cxOld),
                                  rect.top,
                                  0, 0,
                                  SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    hItemWnd = GetDlgItem(hwndDlg, IDOK);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + (cy - DlgData->cyOld),
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDCANCEL);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + (cy - DlgData->cyOld),
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /* For the size grip */
    hItemWnd = GetDlgItem(hwndDlg, IDC_DIALOG_GRIP);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + (cy - DlgData->cyOld),
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (hdwp)
    {
        EndDeferWindowPos(hdwp);
    }

    DlgData->cxOld = cx;
    DlgData->cyOld = cy;
}

static BOOL
OnBeginLabelEdit(NMLVDISPINFO* pnmv)
{
    HWND hwndEdit;

    hwndEdit = ListView_GetEditControl(pnmv->hdr.hwndFrom);
    if (hwndEdit == NULL)
    {
        return TRUE;
    }

    SendMessage(hwndEdit, EM_SETLIMITTEXT, MAX_PATH - 1, 0);

    return FALSE;
}

static BOOL
OnEndLabelEdit(NMLVDISPINFO* pnmv)
{
    HWND hwndEdit;
    TCHAR szOldDir[MAX_PATH];
    TCHAR szNewDir[MAX_PATH];

    hwndEdit = ListView_GetEditControl(pnmv->hdr.hwndFrom);
    if (hwndEdit == NULL)
    {
        return TRUE;
    }

    /* Leave, if there is no valid listview item */
    if (pnmv->item.iItem == -1)
    {
        return FALSE;
    }

    ListView_GetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szOldDir,
                         _countof(szOldDir));

    SendMessage(hwndEdit, WM_GETTEXT, _countof(szNewDir), (LPARAM)szNewDir);

    /* If there is nothing in the text box then remove the item */
    if (_tcslen(szNewDir) == 0)
    {
        ListView_DeleteItem(pnmv->hdr.hwndFrom, pnmv->item.iItem);
        ListView_SetItemState(pnmv->hdr.hwndFrom, pnmv->item.iItem - 1,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
        return FALSE;
    }

    /* If nothing has been changed then just bail out */
    if (_tcscmp(szOldDir, szNewDir) == 0)
    {
        return FALSE;
    }

    ListView_SetItemText(pnmv->hdr.hwndFrom,
                         pnmv->item.iItem, 0,
                         szNewDir);

    return TRUE;
}

static BOOL
OnNotifyEditVariableDlg(HWND hwndDlg, PEDIT_DIALOG_DATA DlgData, NMHDR *phdr)
{
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)phdr;

    switch (phdr->idFrom)
    {
        case IDC_LIST_VARIABLE_VALUE:
            switch (phdr->code)
            {
                case NM_CLICK:
                {
                    /* Detect if an item is selected */
                    DlgData->bIsItemSelected = (lpnmlv->iItem != -1);
                    if (lpnmlv->iItem != -1)
                    {
                        DlgData->dwSelectedValueIndex = lpnmlv->iItem;
                    }
                    break;
                }

                case NM_DBLCLK:
                {
                    /* Either simulate IDC_BUTTON_NEW or edit an item depending upon the condition */
                    if (lpnmlv->iItem == -1)
                    {
                        SendMessage(GetDlgItem(hwndDlg, IDC_BUTTON_NEW), BM_CLICK, 0, 0);
                    }
                    else
                    {
                        ListView_EditLabel(GetDlgItem(hwndDlg, IDC_LIST_VARIABLE_VALUE), DlgData->dwSelectedValueIndex);
                    }
                    break;
                }

                case LVN_BEGINLABELEDIT:
                {
                    return OnBeginLabelEdit((NMLVDISPINFO*)phdr);
                }

                case LVN_ENDLABELEDIT:
                {
                    return OnEndLabelEdit((NMLVDISPINFO*)phdr);
                }
            }
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
EditVariableDlgProc(HWND hwndDlg,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam)
{
    PEDIT_DIALOG_DATA DlgData;
    HWND hwndListView;

    DlgData = (PEDIT_DIALOG_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);
    hwndListView = GetDlgItem(hwndDlg, IDC_LIST_VARIABLE_VALUE);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            RECT rect;

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            DlgData = (PEDIT_DIALOG_DATA)lParam;

            GetClientRect(hwndDlg, &rect);
            DlgData->cxOld = rect.right - rect.left;
            DlgData->cyOld = rect.bottom - rect.top;

            GetWindowRect(hwndDlg, &rect);
            DlgData->cxMin = rect.right - rect.left;
            DlgData->cyMin = rect.bottom - rect.top;

            /* Either get the values from list box or from edit box */
            if (DlgData->dwDlgID == IDD_EDIT_VARIABLE_FANCY)
            {
                /* Subclass the listview control first */
                SetWindowSubclass(hwndListView, ListViewSubclassProc, 1, 0);

                if (DlgData->VarData->lpRawValue != NULL)
                {
                    AddValuesToList(hwndDlg, DlgData);
                }
            }
            else
            {
                if (DlgData->VarData->lpName != NULL)
                {
                    SendDlgItemMessage(hwndDlg, IDC_VARIABLE_NAME, WM_SETTEXT, 0, (LPARAM)DlgData->VarData->lpName);
                }

                if (DlgData->VarData->lpRawValue != NULL)
                {
                    SendDlgItemMessage(hwndDlg, IDC_VARIABLE_VALUE, WM_SETTEXT, 0, (LPARAM)DlgData->VarData->lpRawValue);
                }
            }
            break;
        }

        case WM_SIZE:
        {
            OnEnvironmentEditDlgResize(hwndDlg, DlgData, LOWORD(lParam), HIWORD(lParam));
            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, 0);
            return TRUE;
        }

        case WM_SIZING:
        {
            /* Forbid resizing the dialog smaller than its minimal size */
            PRECT pRect = (PRECT)lParam;

            if ((wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT))
            {
                if (pRect->right - pRect->left < DlgData->cxMin)
                    pRect->left = pRect->right - DlgData->cxMin;
            }
            else
            if ((wParam == WMSZ_RIGHT) || (wParam == WMSZ_TOPRIGHT) || (wParam == WMSZ_BOTTOMRIGHT))
            {
                if (pRect->right - pRect->left < DlgData->cxMin)
                    pRect->right = pRect->left + DlgData->cxMin;
            }

            if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT))
            {
                if (pRect->bottom - pRect->top < DlgData->cyMin)
                    pRect->top = pRect->bottom - DlgData->cyMin;
            }
            else
            if ((wParam == WMSZ_BOTTOM) || (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_BOTTOMRIGHT))
            {
                if (pRect->bottom - pRect->top < DlgData->cyMin)
                    pRect->bottom = pRect->top + DlgData->cyMin;
            }

            /* Make sure the normal variable edit dialog doesn't change its height */
            if (DlgData->dwDlgID == IDD_EDIT_VARIABLE)
            {
                if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT))
                {
                    if (pRect->bottom - pRect->top > DlgData->cyMin)
                        pRect->top = pRect->bottom - DlgData->cyMin;
                }
                else
                if ((wParam == WMSZ_BOTTOM) || (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_BOTTOMRIGHT))
                {
                    if (pRect->bottom - pRect->top > DlgData->cyMin)
                        pRect->bottom = pRect->top + DlgData->cyMin;
                }
            }

            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            return OnNotifyEditVariableDlg(hwndDlg, DlgData, (NMHDR*)lParam);
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    LPTSTR p;
                    DWORD dwValueLength;

                    /* Either set the values to the list box or to the edit box */
                    if (DlgData->dwDlgID == IDD_EDIT_VARIABLE_FANCY)
                    {
                        dwValueLength = GatherDataFromListView(hwndListView, DlgData->VarData);
                    }
                    else
                    {
                        dwValueLength = GatherDataFromEditBox(hwndDlg, DlgData->VarData);
                    }

                    if (dwValueLength == 0)
                    {
                        break;
                    }

                    if (DlgData->VarData->lpCookedValue != NULL)
                    {
                        GlobalFree(DlgData->VarData->lpCookedValue);
                        DlgData->VarData->lpCookedValue = NULL;
                    }

                    p = _tcschr(DlgData->VarData->lpRawValue, _T('%'));
                    if (p && _tcschr(++p, _T('%')))
                    {
                        DlgData->VarData->dwType = REG_EXPAND_SZ;

                        DlgData->VarData->lpCookedValue = GlobalAlloc(GPTR, 2 * MAX_PATH * sizeof(TCHAR));
                        if (!DlgData->VarData->lpCookedValue)
                            return FALSE;

                        ExpandEnvironmentStrings(DlgData->VarData->lpRawValue,
                                                 DlgData->VarData->lpCookedValue,
                                                 2 * MAX_PATH);
                    }
                    else
                    {
                        DlgData->VarData->dwType = REG_SZ;

                        DlgData->VarData->lpCookedValue = GlobalAlloc(GPTR, (dwValueLength + 1) * sizeof(TCHAR));
                        if (!DlgData->VarData->lpCookedValue)
                            return FALSE;

                        _tcscpy(DlgData->VarData->lpCookedValue, DlgData->VarData->lpRawValue);
                    }

                    EndDialog(hwndDlg, 1);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDC_BUTTON_BROWSE_FILE:
                {
                    BrowseRequiredFile(hwndDlg);
                    break;
                }

                case IDC_BUTTON_BROWSE_FOLDER:
                {
                    BrowseRequiredFolder(hwndDlg, DlgData);
                    break;
                }

                case IDC_BUTTON_DELETE:
                {
                    DWORD dwLastIndex;

                    dwLastIndex = ListView_GetItemCount(hwndListView) - 1;
                    ListView_DeleteItem(hwndListView, DlgData->dwSelectedValueIndex);

                    if (dwLastIndex == DlgData->dwSelectedValueIndex)
                    {
                        DlgData->dwSelectedValueIndex--;
                    }

                    ListView_SetItemState(hwndListView, DlgData->dwSelectedValueIndex,
                                          LVIS_FOCUSED | LVIS_SELECTED,
                                          LVIS_FOCUSED | LVIS_SELECTED);
                    break;
                }

                case IDC_BUTTON_MOVE_UP:
                {
                    MoveListItem(hwndDlg, DlgData, TRUE);
                    break;
                }

                case IDC_BUTTON_MOVE_DOWN:
                {
                    MoveListItem(hwndDlg, DlgData, FALSE);
                    break;
                }

                case IDC_BUTTON_EDIT_TEXT:
                {
                    TCHAR szStr[MAX_STR_LENGTH] = _T("");
                    TCHAR szStr2[MAX_STR_LENGTH] = _T("");

                    LoadString(hApplet, IDS_ENVIRONMENT_WARNING, szStr, _countof(szStr));
                    LoadString(hApplet, IDS_ENVIRONMENT_WARNING_TITLE, szStr2, _countof(szStr2));

                    if (MessageBox(hwndDlg,
                                   szStr,
                                   szStr2,
                                   MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON1) == IDOK)
                    {
                        EndDialog(hwndDlg, -1);
                    }
                    break;
                }

                case IDC_BUTTON_NEW:
                {
                    DlgData->dwSelectedValueIndex = ListView_GetItemCount(hwndListView);
                    AddEmptyItem(hwndListView, DlgData->dwSelectedValueIndex);
                    ListView_EditLabel(hwndListView, DlgData->dwSelectedValueIndex);
                    break;
                }

                case IDC_BUTTON_EDIT:
                {
                    ListView_EditLabel(hwndListView, DlgData->dwSelectedValueIndex);
                    break;
                }
            }
            break;
    }

    return FALSE;
}


static VOID
GetEnvironmentVariables(HWND hwndListView,
                        HKEY hRootKey,
                        LPTSTR lpSubKeyName)
{
    HKEY hKey;
    DWORD dwValues;
    DWORD dwMaxValueNameLength;
    DWORD dwMaxValueDataLength;
    DWORD i;
    LPTSTR lpName;
    LPTSTR lpData;
    LPTSTR lpExpandData;
    DWORD dwNameLength;
    DWORD dwDataLength;
    DWORD dwType;
    PVARIABLE_DATA VarData;

    LV_ITEM lvi;
    int iItem;

    if (RegOpenKeyEx(hRootKey,
                     lpSubKeyName,
                     0,
                     KEY_READ,
                     &hKey))
        return;

    if (RegQueryInfoKey(hKey,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwValues,
                        &dwMaxValueNameLength,
                        &dwMaxValueDataLength,
                        NULL,
                        NULL))
    {
        RegCloseKey(hKey);
        return;
    }

    lpName = GlobalAlloc(GPTR, (dwMaxValueNameLength + 1) * sizeof(TCHAR));
    if (lpName == NULL)
    {
        RegCloseKey(hKey);
        return;
    }

    lpData = GlobalAlloc(GPTR, (dwMaxValueDataLength + 1) * sizeof(TCHAR));
    if (lpData == NULL)
    {
        GlobalFree(lpName);
        RegCloseKey(hKey);
        return;
    }

    lpExpandData = GlobalAlloc(GPTR, 2048 * sizeof(TCHAR));
    if (lpExpandData == NULL)
    {
        GlobalFree(lpName);
        GlobalFree(lpData);
        RegCloseKey(hKey);
        return;
    }

    for (i = 0; i < dwValues; i++)
    {
        dwNameLength = dwMaxValueNameLength + 1;
        dwDataLength = dwMaxValueDataLength + 1;

        if (RegEnumValue(hKey,
                         i,
                         lpName,
                         &dwNameLength,
                         NULL,
                         &dwType,
                         (LPBYTE)lpData,
                         &dwDataLength))
        {
            GlobalFree(lpExpandData);
            GlobalFree(lpName);
            GlobalFree(lpData);
            RegCloseKey(hKey);
            return;
        }

        if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
            continue;

        VarData = GlobalAlloc(GPTR, sizeof(VARIABLE_DATA));

        VarData->dwType = dwType;

        VarData->lpName = GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpName, lpName);

        VarData->lpRawValue = GlobalAlloc(GPTR, (dwDataLength + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpRawValue, lpData);

        ExpandEnvironmentStrings(lpData, lpExpandData, 2048);

        VarData->lpCookedValue = GlobalAlloc(GPTR, (_tcslen(lpExpandData) + 1) * sizeof(TCHAR));
        _tcscpy(VarData->lpCookedValue, lpExpandData);

        memset(&lvi, 0x00, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        lvi.lParam = (LPARAM)VarData;
        lvi.pszText = VarData->lpName;
        lvi.state = (i == 0) ? LVIS_SELECTED : 0;
        iItem = ListView_InsertItem(hwndListView, &lvi);

        ListView_SetItemText(hwndListView, iItem, 1, VarData->lpCookedValue);
    }

    GlobalFree(lpExpandData);
    GlobalFree(lpName);
    GlobalFree(lpData);
    RegCloseKey(hKey);
}


static VOID
SetEnvironmentDialogListViewColumns(HWND hwndListView)
{
    RECT rect;
    LV_COLUMN column;
    TCHAR szStr[32];

    GetClientRect(hwndListView, &rect);

    memset(&column, 0x00, sizeof(column));
    column.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    column.fmt=LVCFMT_LEFT;
    column.cx = (INT)((rect.right - rect.left) * 0.32);
    column.iSubItem = 0;
    LoadString(hApplet, IDS_VARIABLE, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 0, &column);

    column.cx = (INT)((rect.right - rect.left) * 0.63);
    column.iSubItem = 1;
    LoadString(hApplet, IDS_VALUE, szStr, sizeof(szStr) / sizeof(szStr[0]));
    column.pszText = szStr;
    (void)ListView_InsertColumn(hwndListView, 1, &column);
}


static VOID
OnInitEnvironmentDialog(HWND hwndDlg)
{
    HWND hwndListView;

    /* Set user environment variables */
    hwndListView = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_LIST);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetEnvironmentDialogListViewColumns(hwndListView);

    GetEnvironmentVariables(hwndListView,
                            HKEY_CURRENT_USER,
                            _T("Environment"));

    (void)ListView_SetColumnWidth(hwndListView, 2, LVSCW_AUTOSIZE_USEHEADER);

    ListView_SetItemState(hwndListView, 0,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    (void)ListView_Update(hwndListView,0);

    /* Set system environment variables */
    hwndListView = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);

    (void)ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);

    SetEnvironmentDialogListViewColumns(hwndListView);

    GetEnvironmentVariables(hwndListView,
                            HKEY_LOCAL_MACHINE,
                            _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"));

    (void)ListView_SetColumnWidth(hwndListView, 2, LVSCW_AUTOSIZE_USEHEADER);

    ListView_SetItemState(hwndListView, 0,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);

    (void)ListView_Update(hwndListView, 0);
}


static VOID
OnNewVariable(HWND hwndDlg,
              INT iDlgItem)
{
    HWND hwndListView;
    PEDIT_DIALOG_DATA DlgData;
    LV_ITEM lvi;
    INT iItem;

    DlgData = GlobalAlloc(GPTR, sizeof(EDIT_DIALOG_DATA));
    if (!DlgData)
        return;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);
    DlgData->dwDlgID = IDD_EDIT_VARIABLE;
    DlgData->dwSelectedValueIndex = -1;

    DlgData->VarData = GlobalAlloc(GPTR, sizeof(VARIABLE_DATA));
    if (!DlgData->VarData)
        return;

    if (DialogBoxParam(hApplet,
                       MAKEINTRESOURCE(DlgData->dwDlgID),
                       hwndDlg,
                       EditVariableDlgProc,
                       (LPARAM)DlgData) <= 0)
    {
        if (DlgData->VarData->lpName != NULL)
            GlobalFree(DlgData->VarData->lpName);

        if (DlgData->VarData->lpRawValue != NULL)
            GlobalFree(DlgData->VarData->lpRawValue);

        if (DlgData->VarData->lpCookedValue != NULL)
            GlobalFree(DlgData->VarData->lpCookedValue);

        GlobalFree(DlgData);
    }
    else
    {
        if (DlgData->VarData->lpName != NULL && (DlgData->VarData->lpCookedValue || DlgData->VarData->lpRawValue))
        {
            ZeroMemory(&lvi, sizeof(lvi));
            lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
            lvi.lParam = (LPARAM)DlgData->VarData;
            lvi.pszText = DlgData->VarData->lpName;
            lvi.state = 0;
            iItem = ListView_InsertItem(hwndListView, &lvi);

            ListView_SetItemText(hwndListView, iItem, 1, DlgData->VarData->lpCookedValue);
        }
    }
}


static VOID
OnEditVariable(HWND hwndDlg,
               INT iDlgItem)
{
    HWND hwndListView;
    PEDIT_DIALOG_DATA DlgData;
    LV_ITEM lvi;
    INT iItem;
    INT iRet;

    DlgData = GlobalAlloc(GPTR, sizeof(EDIT_DIALOG_DATA));
    if (!DlgData)
        return;

    DlgData->dwDlgID = IDD_EDIT_VARIABLE;
    DlgData->dwSelectedValueIndex = -1;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    iItem = GetSelectedListViewItem(hwndListView);
    if (iItem != -1)
    {
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;

        if (ListView_GetItem(hwndListView, &lvi))
        {
            DlgData->VarData = (PVARIABLE_DATA)lvi.lParam;

            /* If the value has multiple values and directories then edit value with fancy dialog box */
            if (DetermineDialogBoxType(DlgData->VarData->lpRawValue))
                DlgData->dwDlgID = IDD_EDIT_VARIABLE_FANCY;

            iRet = DialogBoxParam(hApplet,
                                  MAKEINTRESOURCE(DlgData->dwDlgID),
                                  hwndDlg,
                                  EditVariableDlgProc,
                                  (LPARAM)DlgData);

            /* If iRet is less than 0 edit the value and name normally */
            if (iRet < 0)
            {
                DlgData->dwDlgID = IDD_EDIT_VARIABLE;
                iRet = DialogBoxParam(hApplet,
                                      MAKEINTRESOURCE(DlgData->dwDlgID),
                                      hwndDlg,
                                      EditVariableDlgProc,
                                      (LPARAM)DlgData);
            }

            if (iRet > 0)
            {
                ListView_SetItemText(hwndListView, iItem, 0, DlgData->VarData->lpName);
                ListView_SetItemText(hwndListView, iItem, 1, DlgData->VarData->lpCookedValue);
            }
        }

        GlobalFree(DlgData);
    }
}


static VOID
OnDeleteVariable(HWND hwndDlg,
                 INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    iItem = GetSelectedListViewItem(hwndListView);
    if (iItem != -1)
    {
        memset(&lvi, 0x00, sizeof(lvi));
        lvi.mask = LVIF_PARAM;
        lvi.iItem = iItem;

        if (ListView_GetItem(hwndListView, &lvi))
        {
            VarData = (PVARIABLE_DATA)lvi.lParam;
            if (VarData != NULL)
            {
                if (VarData->lpName != NULL)
                    GlobalFree(VarData->lpName);

                if (VarData->lpRawValue != NULL)
                    GlobalFree(VarData->lpRawValue);

                if (VarData->lpCookedValue != NULL)
                    GlobalFree(VarData->lpCookedValue);

                GlobalFree(VarData);
                lvi.lParam = 0;
            }
        }

        (void)ListView_DeleteItem(hwndListView, iItem);

        /* Select the previous item */
        if (iItem > 0)
            iItem--;

        ListView_SetItemState(hwndListView, iItem,
                              LVIS_FOCUSED | LVIS_SELECTED,
                              LVIS_FOCUSED | LVIS_SELECTED);
    }
}

static VOID
OnEnvironmentDlgResize(HWND hwndDlg,
                       PENVIRONMENT_DIALOG_DATA DlgData,
                       DWORD cx,
                       DWORD cy)
{
    RECT rect;
    INT Colx, y = 0;
    HDWP hdwp = NULL;
    HWND hItemWnd;

    if ((cx == DlgData->cxOld) && (cy == DlgData->cyOld))
        return;

    hdwp = BeginDeferWindowPos(13);

    if (cy >= DlgData->cyOld)
        y += (cy - DlgData->cyOld + 1) / 2;
    else
        y -= (DlgData->cyOld - cy + 1) / 2;

    /* For the group box controls */
    hItemWnd = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_GROUP);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              0, 0,
                              (rect.right - rect.left) + (cx - DlgData->cxOld),
                              (rect.bottom - rect.top) + y,
                              SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_GROUP);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left, rect.top + y,
                              (rect.right - rect.left) + (cx - DlgData->cxOld),
                              (rect.bottom - rect.top) + y,
                              SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /* For the list view controls */
    hItemWnd = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_LIST);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              0, 0,
                              (rect.right - rect.left) + (cx - DlgData->cxOld),
                              (rect.bottom - rect.top) + y,
                              SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        Colx = ListView_GetColumnWidth(hItemWnd, 1);
        ListView_SetColumnWidth(hItemWnd, 1, Colx + (cx - DlgData->cxOld));
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left, rect.top + y,
                              (rect.right - rect.left) + (cx - DlgData->cxOld),
                              (rect.bottom - rect.top) + y,
                              SWP_NOZORDER | SWP_NOACTIVATE);
        Colx = ListView_GetColumnWidth(hItemWnd, 1);
        ListView_SetColumnWidth(hItemWnd, 1, Colx + (cx - DlgData->cxOld));
    }

    /* For the buttons */
    hItemWnd = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_NEW);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + y,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_EDIT);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + y,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_USER_VARIABLE_DELETE);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + y,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_NEW);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + y * 2,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_EDIT);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + y * 2,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDC_SYSTEM_VARIABLE_DELETE);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + y * 2,
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDOK);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + (cy - DlgData->cyOld),
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    hItemWnd = GetDlgItem(hwndDlg, IDCANCEL);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + (cy - DlgData->cyOld),
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    /* For the size grip */
    hItemWnd = GetDlgItem(hwndDlg, IDC_DIALOG_GRIP);
    GetWindowRect(hItemWnd, &rect);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rect, sizeof(RECT)/sizeof(POINT));

    if (hdwp)
    {
        hdwp = DeferWindowPos(hdwp,
                              hItemWnd,
                              NULL,
                              rect.left + (cx - DlgData->cxOld),
                              rect.top + (cy - DlgData->cyOld),
                              0, 0,
                              SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    if (hdwp)
    {
        EndDeferWindowPos(hdwp);
    }

    DlgData->cxOld = cx;
    DlgData->cyOld = cy;
}

static VOID
ReleaseListViewItems(HWND hwndDlg,
                     INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT nItemCount;
    INT i;

    hwndListView = GetDlgItem(hwndDlg, iDlgItem);

    memset(&lvi, 0x00, sizeof(lvi));

    nItemCount = ListView_GetItemCount(hwndListView);
    for (i = 0; i < nItemCount; i++)
    {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;

        if (ListView_GetItem(hwndListView, &lvi))
        {
            VarData = (PVARIABLE_DATA)lvi.lParam;
            if (VarData != NULL)
            {
                if (VarData->lpName != NULL)
                    GlobalFree(VarData->lpName);

                if (VarData->lpRawValue != NULL)
                    GlobalFree(VarData->lpRawValue);

                if (VarData->lpCookedValue != NULL)
                    GlobalFree(VarData->lpCookedValue);

                GlobalFree(VarData);
                lvi.lParam = 0;
            }
        }
    }
}


static VOID
SetAllVars(HWND hwndDlg,
           INT iDlgItem)
{
    HWND hwndListView;
    PVARIABLE_DATA VarData;
    LV_ITEM lvi;
    INT iItem;
    HKEY hKey;
    DWORD dwValueCount;
    DWORD dwMaxValueNameLength;
    LPTSTR *aValueArray;
    DWORD dwNameLength;
    DWORD i;
    TCHAR szBuffer[256];
    LPTSTR lpBuffer;

    memset(&lvi, 0x00, sizeof(lvi));

    /* Get the handle to the list box with all system vars in it */
    hwndListView = GetDlgItem(hwndDlg, iDlgItem);
    /* First item is 0 */
    iItem = 0;
    /* Set up struct to retrieve item */
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;

    /* Open or create the key */
    if (RegCreateKeyEx((iDlgItem == IDC_SYSTEM_VARIABLE_LIST ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER),
                       (iDlgItem == IDC_SYSTEM_VARIABLE_LIST ? _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment") : _T("Environment")),
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE | KEY_READ,
                       NULL,
                       &hKey,
                       NULL))
    {
        return;
    }

    /* Get the number of values and the maximum value name length */
    if (RegQueryInfoKey(hKey,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &dwValueCount,
                        &dwMaxValueNameLength,
                        NULL,
                        NULL,
                        NULL))
    {
        RegCloseKey(hKey);
        return;
    }

    if (dwValueCount > 0)
    {
        /* Allocate the value array */
        aValueArray = GlobalAlloc(GPTR, dwValueCount * sizeof(LPTSTR));
        if (aValueArray != NULL)
        {
            /* Get all value names */
            for (i = 0; i < dwValueCount; i++)
            {
                dwNameLength = 256;
                if (!RegEnumValue(hKey,
                                  i,
                                  szBuffer,
                                  &dwNameLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL))
                {
                    /* Allocate a value name buffer, fill it and attach it to the array */
                    lpBuffer = (LPTSTR)GlobalAlloc(GPTR, (dwNameLength + 1) * sizeof(TCHAR));
                    if (lpBuffer != NULL)
                    {
                        _tcscpy(lpBuffer, szBuffer);
                        aValueArray[i] = lpBuffer;
                    }
                }
            }

            /* Delete all values */
            for (i = 0; i < dwValueCount; i++)
            {
                if (aValueArray[i] != NULL)
                {
                    /* Delete the value */
                    RegDeleteValue(hKey,
                                   aValueArray[i]);

                    /* Free the value name */
                    GlobalFree(aValueArray[i]);
                }
            }

            /* Free the value array */
            GlobalFree(aValueArray);
        }
    }

    /* Loop through all variables */
    while (ListView_GetItem(hwndListView, &lvi))
    {
        /* Get the data in each item */
        VarData = (PVARIABLE_DATA)lvi.lParam;
        if (VarData != NULL)
        {
            /* Set the new value */
            if (RegSetValueEx(hKey,
                              VarData->lpName,
                              0,
                              VarData->dwType,
                              (LPBYTE)VarData->lpRawValue,
                              (DWORD)(_tcslen(VarData->lpRawValue) + 1) * sizeof(TCHAR)))
            {
                RegCloseKey(hKey);
                return;
            }
        }

        /* Fill struct for next item */
        lvi.mask = LVIF_PARAM;
        lvi.iItem = ++iItem;
    }

    RegCloseKey(hKey);
}


static BOOL
OnNotify(HWND hwndDlg, NMHDR *phdr)
{
    switch (phdr->code)
    {
        case NM_DBLCLK:
            if (phdr->idFrom == IDC_USER_VARIABLE_LIST ||
                phdr->idFrom == IDC_SYSTEM_VARIABLE_LIST)
            {
                OnEditVariable(hwndDlg, (INT)phdr->idFrom);
                return TRUE;
            }
            break;

        case LVN_KEYDOWN:
            if (((LPNMLVKEYDOWN)phdr)->wVKey == VK_DELETE &&
                (phdr->idFrom == IDC_USER_VARIABLE_LIST ||
                 phdr->idFrom == IDC_SYSTEM_VARIABLE_LIST))
            {
                OnDeleteVariable(hwndDlg, (INT)phdr->idFrom);
                return TRUE;
            }
            break;
    }

    return FALSE;
}


/* Environment dialog procedure */
INT_PTR CALLBACK
EnvironmentDlgProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PENVIRONMENT_DIALOG_DATA DlgData;
    DlgData = (PENVIRONMENT_DIALOG_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            RECT rect;

            DlgData = GlobalAlloc(GPTR, sizeof(ENVIRONMENT_DIALOG_DATA));
            if (!DlgData)
            {
                EndDialog(hwndDlg, 0);
                return (INT_PTR)TRUE;
            }
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)DlgData);

            GetClientRect(hwndDlg, &rect);
            DlgData->cxOld = rect.right - rect.left;
            DlgData->cyOld = rect.bottom - rect.top;

            GetWindowRect(hwndDlg, &rect);
            DlgData->cxMin = rect.right - rect.left;
            DlgData->cyMin = rect.bottom - rect.top;

            OnInitEnvironmentDialog(hwndDlg);
            break;
        }

        case WM_SIZE:
        {
            OnEnvironmentDlgResize(hwndDlg, DlgData, LOWORD(lParam), HIWORD(lParam));
            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, 0);
            return TRUE;
        }

        case WM_SIZING:
        {
            /* Forbid resizing the dialog smaller than its minimal size */
            PRECT pRect = (PRECT)lParam;

            if ((wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT))
            {
                if (pRect->right - pRect->left < DlgData->cxMin)
                    pRect->left = pRect->right - DlgData->cxMin;
            }
            else
            if ((wParam == WMSZ_RIGHT) || (wParam == WMSZ_TOPRIGHT) || (wParam == WMSZ_BOTTOMRIGHT))
            {
                if (pRect->right - pRect->left < DlgData->cxMin)
                    pRect->right = pRect->left + DlgData->cxMin;
            }

            if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT))
            {
                if (pRect->bottom - pRect->top < DlgData->cyMin)
                    pRect->top = pRect->bottom - DlgData->cyMin;
            }
            else
            if ((wParam == WMSZ_BOTTOM) || (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_BOTTOMRIGHT))
            {
                if (pRect->bottom - pRect->top < DlgData->cyMin)
                    pRect->bottom = pRect->top + DlgData->cyMin;
            }

            SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_USER_VARIABLE_NEW:
                    OnNewVariable(hwndDlg, IDC_USER_VARIABLE_LIST);
                    return TRUE;

                case IDC_USER_VARIABLE_EDIT:
                    OnEditVariable(hwndDlg, IDC_USER_VARIABLE_LIST);
                    return TRUE;

                case IDC_USER_VARIABLE_DELETE:
                    OnDeleteVariable(hwndDlg, IDC_USER_VARIABLE_LIST);
                    return TRUE;

                case IDC_SYSTEM_VARIABLE_NEW:
                    OnNewVariable(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    return TRUE;

                case IDC_SYSTEM_VARIABLE_EDIT:
                    OnEditVariable(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    return TRUE;

                case IDC_SYSTEM_VARIABLE_DELETE:
                    OnDeleteVariable(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    return TRUE;

                case IDOK:
                    SetAllVars(hwndDlg, IDC_USER_VARIABLE_LIST);
                    SetAllVars(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
                    SendMessage(HWND_BROADCAST, WM_WININICHANGE,
                                0, (LPARAM)_T("Environment"));
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;
            }
            break;

        case WM_DESTROY:
            ReleaseListViewItems(hwndDlg, IDC_USER_VARIABLE_LIST);
            ReleaseListViewItems(hwndDlg, IDC_SYSTEM_VARIABLE_LIST);
            GlobalFree(DlgData);
            break;

        case WM_NOTIFY:
            return OnNotify(hwndDlg, (NMHDR*)lParam);
    }

    return FALSE;
}

/* EOF */
