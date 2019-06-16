/*
 * Regedit listviews
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
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

#include "regedit.h"

#define CX_ICON            16
#define CY_ICON            16
#define LISTVIEW_NUM_ICONS 2

int Image_String = 0;
int Image_Bin = 0;
INT iListViewSelect = -1;

typedef struct tagLINE_INFO
{
    DWORD dwValType;
    LPWSTR name;
    void* val;
    size_t val_len;
} LINE_INFO, *PLINE_INFO;

typedef struct tagSORT_INFO
{
    INT  iSortingColumn;
    BOOL bSortAscending;
} SORT_INFO, *PSORT_INFO;

/*******************************************************************************
 * Global and Local Variables:
 */

static INT g_iSortedColumn = 0;

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static const int default_column_widths[MAX_LIST_COLUMNS] = { 35, 25, 40 };  /* in percents */
static const int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };

LPCWSTR GetValueName(HWND hwndLV, int iStartAt)
{
    int item;
    LVITEMW LVItem;
    PLINE_INFO lineinfo;

    /*
       if a new item is inserted, then no allocation,
       otherwise the heap block will be lost!
    */
    item = ListView_GetNextItem(hwndLV, iStartAt, LVNI_SELECTED);
    if (item == -1) return NULL;

    /*
        Should be always TRUE anyways
    */
    LVItem.iItem = item;
    LVItem.iSubItem = 0;
    LVItem.mask = LVIF_PARAM;
    if (ListView_GetItem(hwndLV, &LVItem) == FALSE)
        return NULL;

    lineinfo = (PLINE_INFO)LVItem.lParam;
    if (lineinfo == NULL)
        return NULL;

    return lineinfo->name;
}

VOID SetValueName(HWND hwndLV, LPCWSTR pszValueName)
{
    INT i, c;
    LVFINDINFOW fi;

    c = ListView_GetItemCount(hwndLV);
    for(i = 0; i < c; i++)
    {
        ListView_SetItemState(hwndLV, i, 0, LVIS_FOCUSED | LVIS_SELECTED);
    }
    if (pszValueName == NULL)
        i = 0;
    else
    {
        fi.flags    = LVFI_STRING;
        fi.psz      = pszValueName;
        i = ListView_FindItem(hwndLV, -1, &fi);
    }
    ListView_SetItemState(hwndLV, i, LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
    iListViewSelect = i;
}

BOOL IsDefaultValue(HWND hwndLV, int i)
{
    PLINE_INFO lineinfo;
    LVITEMW Item;

    Item.mask = LVIF_PARAM;
    Item.iItem = i;
    if(ListView_GetItem(hwndLV, &Item))
    {
        lineinfo = (PLINE_INFO)Item.lParam;
        return lineinfo && (!lineinfo->name || !wcscmp(lineinfo->name, L""));
    }
    return FALSE;
}

/*******************************************************************************
 * Local module support methods
 */
static void AddEntryToList(HWND hwndLV, LPWSTR Name, DWORD dwValType, void* ValBuf, DWORD dwCount, int Position, BOOL ValExists)
{
    PLINE_INFO linfo;
    LVITEMW item;
    int index;

    linfo = (PLINE_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINE_INFO) + dwCount);
    linfo->dwValType = dwValType;
    linfo->val_len = dwCount;
    if (dwCount > 0)
    {
        memcpy(&linfo[1], ValBuf, dwCount);
        linfo->val = &linfo[1];
    }
    linfo->name = _wcsdup(Name);

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    item.iItem = (Position == -1 ? 0: Position);
    item.iSubItem = 0;
    item.state = 0;
    item.stateMask = 0;
    item.pszText = Name;
    item.cchTextMax = (int)wcslen(item.pszText);
    if (item.cchTextMax == 0)
        item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = 0;
    item.lParam = (LPARAM)linfo;
    switch(dwValType)
    {
        case REG_SZ:
        case REG_EXPAND_SZ:
        case REG_MULTI_SZ:
            item.iImage = Image_String;
            break;
        default:
            item.iImage = Image_Bin;
            break;
    }

    /*    item.lParam = (LPARAM)ValBuf; */
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif

    index = ListView_InsertItem(hwndLV, &item);
    if (index != -1)
    {
        switch (dwValType)
        {
        case REG_SZ:
        case REG_EXPAND_SZ:
            if(dwCount > 0)
            {
                ListView_SetItemText(hwndLV, index, 2, ValBuf);
            }
            else if(!ValExists)
            {
                WCHAR buffer[255];
                /* load (value not set) string */
                LoadStringW(hInst, IDS_VALUE_NOT_SET, buffer, COUNT_OF(buffer));
                ListView_SetItemText(hwndLV, index, 2, buffer);
            }
            break;
        case REG_MULTI_SZ:
        {
            LPWSTR src, str;
            if(dwCount >= 2)
            {
                src = (LPWSTR)ValBuf;
                str = HeapAlloc(GetProcessHeap(), 0, dwCount + sizeof(WCHAR));
                if(str != NULL)
                {
                    *str = L'\0';
                    /* concatenate all srings */
                    while(*src != L'\0')
                    {
                        wcscat(str, src);
                        wcscat(str, L" ");
                        src += wcslen(src) + 1;
                    }
                    ListView_SetItemText(hwndLV, index, 2, str);
                    HeapFree(GetProcessHeap(), 0, str);
                }
                else
                    ListView_SetItemText(hwndLV, index, 2, L"");
            }
            else
                ListView_SetItemText(hwndLV, index, 2, L"");
        }
        break;
        case REG_DWORD:
        {
            WCHAR buf[200];
            if(dwCount == sizeof(DWORD))
            {
                wsprintf(buf, L"0x%08x (%u)", *(DWORD*)ValBuf, *(DWORD*)ValBuf);
            }
            else
            {
                LoadStringW(hInst, IDS_INVALID_DWORD, buf, COUNT_OF(buf));
            }
            ListView_SetItemText(hwndLV, index, 2, buf);
        }
        /*            lpsRes = convertHexToDWORDStr(lpbData, dwLen); */
        break;
        default:
        {
            unsigned int i;
            LPBYTE pData = (LPBYTE)ValBuf;
            LPWSTR strBinary;
            if(dwCount > 0)
            {
                strBinary = HeapAlloc(GetProcessHeap(), 0, (dwCount * sizeof(WCHAR) * 3) + sizeof(WCHAR));
                for (i = 0; i < dwCount; i++)
                {
                    wsprintf( strBinary + i*3, L"%02X ", pData[i] );
                }
                strBinary[dwCount * 3] = 0;
                ListView_SetItemText(hwndLV, index, 2, strBinary);
                HeapFree(GetProcessHeap(), 0, strBinary);
            }
            else
            {
                WCHAR szText[128];
                LoadStringW(hInst, IDS_BINARY_EMPTY, szText, COUNT_OF(szText));
                ListView_SetItemText(hwndLV, index, 2, szText);
            }
        }
        break;
        }
    }
}

static BOOL CreateListColumns(HWND hWndListView, INT cxTotal)
{
    WCHAR szText[50];
    int index;
    LVCOLUMN lvC;

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    /* Load the column labels from the resource file. */
    for (index = 0; index < MAX_LIST_COLUMNS; index++)
    {
        lvC.iSubItem = index;
        lvC.cx = (cxTotal * default_column_widths[index]) / 100;
        lvC.fmt = column_alignment[index];
        LoadStringW(hInst, IDS_LIST_COLUMN_FIRST + index, szText, COUNT_OF(szText));
        if (ListView_InsertColumn(hWndListView, index, &lvC) == -1) return FALSE;
    }
    return TRUE;
}

static BOOL InitListViewImageLists(HWND hwndLV)
{
    HIMAGELIST himl;  /* handle to image list  */
    HICON hico;       /* handle to icon  */

    /* Create the image list.  */
    if ((himl = ImageList_Create(CX_ICON, CY_ICON,
                                 ILC_MASK, 0, LISTVIEW_NUM_ICONS)) == NULL)
    {
        return FALSE;
    }

    hico = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_BIN));
    Image_Bin = ImageList_AddIcon(himl, hico);

    hico = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_STRING));
    Image_String = ImageList_AddIcon(himl, hico);

    /* Fail if not all of the images were added.  */
    if (ImageList_GetImageCount(himl) < LISTVIEW_NUM_ICONS)
    {
        return FALSE;
    }

    /* Associate the image list with the tree view control.  */
    (void)ListView_SetImageList(hwndLV, himl, LVSIL_SMALL);

    return TRUE;
}

/* OnGetDispInfo - processes the LVN_GETDISPINFO notification message.  */

static void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    static WCHAR buffer[200];

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0;

    switch (plvdi->item.iSubItem)
    {
    case 0:
        LoadStringW(hInst, IDS_DEFAULT_VALUE_NAME, buffer, COUNT_OF(buffer));
        plvdi->item.pszText = buffer;
        break;
    case 1:
        switch (((LINE_INFO*)plvdi->item.lParam)->dwValType)
        {
            case REG_NONE:
                plvdi->item.pszText = L"REG_NONE";
                break;
            case REG_SZ:
                plvdi->item.pszText = L"REG_SZ";
                break;
            case REG_EXPAND_SZ:
                plvdi->item.pszText = L"REG_EXPAND_SZ";
                break;
            case REG_BINARY:
                plvdi->item.pszText = L"REG_BINARY";
                break;
            case REG_DWORD: /* REG_DWORD_LITTLE_ENDIAN */
                plvdi->item.pszText = L"REG_DWORD";
                break;
            case REG_DWORD_BIG_ENDIAN:
                plvdi->item.pszText = L"REG_DWORD_BIG_ENDIAN";
                break;
            case REG_LINK:
                plvdi->item.pszText = L"REG_LINK";
                break;
            case REG_MULTI_SZ:
                plvdi->item.pszText = L"REG_MULTI_SZ";
                break;
            case REG_RESOURCE_LIST:
                plvdi->item.pszText = L"REG_RESOURCE_LIST";
                break;
            case REG_FULL_RESOURCE_DESCRIPTOR:
                plvdi->item.pszText = L"REG_FULL_RESOURCE_DESCRIPTOR";
                break;
            case REG_RESOURCE_REQUIREMENTS_LIST:
                plvdi->item.pszText = L"REG_RESOURCE_REQUIREMENTS_LIST";
                break;
            case REG_QWORD: /* REG_QWORD_LITTLE_ENDIAN */
                plvdi->item.pszText = L"REG_QWORD";
                break;
            default:
            {
                WCHAR buf2[200];
                LoadStringW(hInst, IDS_UNKNOWN_TYPE, buf2, COUNT_OF(buf2));
                wsprintf(buffer, buf2, ((LINE_INFO*)plvdi->item.lParam)->dwValType);
                plvdi->item.pszText = buffer;
                break;
            }
        }
        break;
    case 3:
        plvdi->item.pszText = L"";
        break;
    }
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PSORT_INFO pSortInfo = (PSORT_INFO)lParamSort;
    LINE_INFO *l, *r;
    DWORD dw1, dw2;
    DWORDLONG qw1, qw2;

    l = (LINE_INFO*)lParam1;
    r = (LINE_INFO*)lParam2;

    if (pSortInfo->iSortingColumn == 1 && l->dwValType != r->dwValType)
    {
        /* Sort by type */
        if (pSortInfo->bSortAscending)
            return ((int)l->dwValType - (int)r->dwValType);
        else
            return ((int)r->dwValType - (int)l->dwValType);
    }
    if (pSortInfo->iSortingColumn == 2)
    {
        /* Sort by value */
        if (l->dwValType != r->dwValType)
        {
            if (pSortInfo->bSortAscending)
                return ((int)l->dwValType - (int)r->dwValType);
            else
                return ((int)r->dwValType - (int)l->dwValType);
        }

        if (l->val == NULL && r->val == NULL)
            return 0;

        if (pSortInfo->bSortAscending)
        {
            if (l->val == NULL)
                return -1;
            if (r->val == NULL)
                return 1;
        }
        else
        {
            if (l->val == NULL)
                return 1;
            if (r->val == NULL)
                return -1;
        }

        switch(l->dwValType)
        {
            case REG_DWORD:
            {
                dw1 = *(DWORD*)l->val;
                dw2 = *(DWORD*)r->val;
                if (pSortInfo->bSortAscending)
                    // return (dw1 > dw2 ? 1 : -1);
                    return ((int)dw1 - (int)dw2);
                else
                    // return (dw1 > dw2 ? -1 : 1);
                    return ((int)dw2 - (int)dw1);
            }

            case REG_QWORD: /* REG_QWORD_LITTLE_ENDIAN */
            {
                qw1 = *(DWORDLONG*)l->val;
                qw2 = *(DWORDLONG*)r->val;
                if (pSortInfo->bSortAscending)
                    // return (qw1 > qw2 ? 1 : -1);
                    return ((int)qw1 - (int)qw2);
                else
                    // return (qw1 > qw2 ? -1 : 1);
                    return ((int)qw2 - (int)qw1);
            }

            default:
            {
                INT nCompare = 0;

                if (pSortInfo->bSortAscending)
                {
                    nCompare = memcmp(l->val, r->val, min(l->val_len, r->val_len));
                    if (nCompare == 0)
                        nCompare = l->val_len - r->val_len;
                }
                else
                {
                    nCompare = memcmp(r->val, l->val, min(r->val_len, l->val_len));
                    if (nCompare == 0)
                        nCompare = r->val_len - l->val_len;
                }

                return nCompare;
            }
        }
    }

    /* Sort by name */
    return (pSortInfo->bSortAscending ? StrCmpLogicalW(l->name, r->name) : StrCmpLogicalW(r->name, l->name));
}

static BOOL ListView_Sort(HWND hListView, int iSortingColumn, int iSortedColumn)
{
    if (!(GetWindowLongPtr(hListView, GWL_STYLE) & LVS_NOSORTHEADER) &&
         (iSortingColumn >= 0) )
    {
        BOOL bSortAscending;
        SORT_INFO SortInfo;

        HWND hHeader = ListView_GetHeader(hListView);
        HDITEM hColumn = {0};

        /* If we are sorting according to another column, uninitialize the old one */
        if ( (iSortedColumn >= 0) && (iSortingColumn != iSortedColumn) )
        {
            hColumn.mask = HDI_FORMAT;
            Header_GetItem(hHeader, iSortedColumn, &hColumn);
            hColumn.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            Header_SetItem(hHeader, iSortedColumn, &hColumn);
        }

        /* Get the sorting state of the new column */
        hColumn.mask = HDI_FORMAT;
        Header_GetItem(hHeader, iSortingColumn, &hColumn);

        /*
         * Check whether we are sorting the list because the user clicked
         * on a column, or because we are refreshing the list:
         *
         * iSortedColumn >= 0 - User clicked on a column; holds the
         *                      old sorting column index.
         * iSortedColumn  < 0 - List being refreshed.
         */
        if (iSortedColumn >= 0)
        {
            /* Invert the sorting direction */
            bSortAscending = ((hColumn.fmt & HDF_SORTUP) == 0);
        }
        else
        {
            /*
             * If the sorting state of the column is uninitialized,
             * initialize it by default to ascending sorting.
             */
            if ((hColumn.fmt & (HDF_SORTUP | HDF_SORTDOWN)) == 0)
                hColumn.fmt |= HDF_SORTUP;

            /* Keep the same sorting direction */
            bSortAscending = ((hColumn.fmt & HDF_SORTUP) != 0);
        }

        /* Set the new column sorting state */
        hColumn.fmt &= ~(bSortAscending ? HDF_SORTDOWN : HDF_SORTUP  );
        hColumn.fmt |=  (bSortAscending ? HDF_SORTUP   : HDF_SORTDOWN);
        Header_SetItem(hHeader, iSortingColumn, &hColumn);

        /* Sort the list */
        SortInfo.iSortingColumn = iSortingColumn;
        SortInfo.bSortAscending = bSortAscending;
        return ListView_SortItems(hListView, CompareFunc, (LPARAM)&SortInfo);
    }
    else
        return TRUE;
}

BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result)
{
    NMLVDISPINFO* Info;
    int iSortingColumn;
    UNREFERENCED_PARAMETER(wParam);
    *Result = TRUE;
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_GETDISPINFO:
        OnGetDispInfo((NMLVDISPINFO*)lParam);
        return TRUE;
    case LVN_COLUMNCLICK:
        iSortingColumn = ((LPNMLISTVIEW)lParam)->iSubItem;
        (void)ListView_Sort(hWnd, iSortingColumn, g_iSortedColumn);
        g_iSortedColumn = iSortingColumn;
        return TRUE;
    case NM_DBLCLK:
    case NM_RETURN:
    {
        SendMessageW(hFrameWnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_MODIFY, 0), 0);
    }
    return TRUE;
    case NM_SETFOCUS:
        g_pChildWnd->nFocusPanel = 0;
        break;
    case LVN_BEGINLABELEDIT:
        Info = (NMLVDISPINFO*)lParam;
        if(Info)
        {
            PLINE_INFO lineinfo = (PLINE_INFO)Info->item.lParam;
            if(!lineinfo->name || !wcscmp(lineinfo->name, L""))
            {
                *Result = TRUE;
            }
            else
            {
                *Result = FALSE;
            }
        }
        else
            *Result = TRUE;
        return TRUE;
    case LVN_ENDLABELEDIT:
        Info = (NMLVDISPINFO*)lParam;
        if(Info && Info->item.pszText)
        {
            PLINE_INFO lineinfo = (PLINE_INFO)Info->item.lParam;
            if(!lineinfo->name || !wcscmp(lineinfo->name, L""))
            {
                *Result = FALSE;
            }
            else
            {
                if(wcslen(Info->item.pszText) == 0)
                {
                    WCHAR msg[128], caption[128];

                    LoadStringW(hInst, IDS_ERR_RENVAL_TOEMPTY, msg, COUNT_OF(msg));
                    LoadStringW(hInst, IDS_ERR_RENVAL_CAPTION, caption, COUNT_OF(caption));
                    MessageBoxW(0, msg, caption, 0);
                    *Result = TRUE;
                }
                else
                {
                    HKEY hKeyRoot;
                    LPCWSTR keyPath;
                    LONG lResult;

                    keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
                    lResult = RenameValue(hKeyRoot, keyPath, Info->item.pszText, lineinfo->name);
                    lineinfo->name = realloc(lineinfo->name, (wcslen(Info->item.pszText)+1)*sizeof(WCHAR));
                    if (lineinfo->name != NULL)
                        wcscpy(lineinfo->name, Info->item.pszText);

                    *Result = TRUE;
                    return (lResult == ERROR_SUCCESS);
                }
            }
        }
        else
            *Result = TRUE;

        return TRUE;
    }
    return FALSE;
}

HWND CreateListView(HWND hwndParent, HMENU id, INT cx)
{
    RECT rcClient;
    HWND hwndLV;

    /* Get the dimensions of the parent window's client area, and create the list view control. */
    GetClientRect(hwndParent, &rcClient);
    hwndLV = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"List View",
                             WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_EDITLABELS | LVS_SHOWSELALWAYS,
                             0, 0, rcClient.right, rcClient.bottom,
                             hwndParent, id, hInst, NULL);
    if (!hwndLV) return NULL;

    /* Initialize the image list, and add items to the control. */
    if (!CreateListColumns(hwndLV, cx)) goto fail;
    if (!InitListViewImageLists(hwndLV)) goto fail;

    return hwndLV;
fail:
    DestroyWindow(hwndLV);
    return NULL;
}

void DestroyListView(HWND hwndLV)
{
    INT count, i;
    LVITEMW item;

    count = ListView_GetItemCount(hwndLV);
    for (i = 0; i < count; i++)
    {
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hwndLV, &item);
        free(((LINE_INFO*)item.lParam)->name);
        HeapFree(GetProcessHeap(), 0, (void*)item.lParam);
    }

}

BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPCWSTR keyPath)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD val_count;
    HKEY hNewKey;
    LONG errCode;
    INT i, c;
    BOOL AddedDefault = FALSE;

    if (!hwndLV) return FALSE;

    (void)ListView_EditLabel(hwndLV, -1);

    SendMessageW(hwndLV, WM_SETREDRAW, FALSE, 0);
    DestroyListView(hwndLV);

    (void)ListView_DeleteAllItems(hwndLV);

    if(!hKey) return FALSE;

    errCode = RegOpenKeyExW(hKey, keyPath, 0, KEY_READ, &hNewKey);
    if (errCode != ERROR_SUCCESS) return FALSE;

    /* get size information and resize the buffers if necessary */
    errCode = RegQueryInfoKeyW(hNewKey, NULL, NULL, NULL, NULL, &max_sub_key_len, NULL,
                               &val_count, &max_val_name_len, &max_val_size, NULL, NULL);

    if (errCode == ERROR_SUCCESS)
    {
        WCHAR* ValName = HeapAlloc(GetProcessHeap(), 0, ++max_val_name_len * sizeof(WCHAR));
        DWORD dwValNameLen = max_val_name_len;
        BYTE* ValBuf = HeapAlloc(GetProcessHeap(), 0, max_val_size + sizeof(WCHAR));
        DWORD dwValSize = max_val_size;
        DWORD dwIndex = 0L;
        DWORD dwValType;
        /*                if (RegQueryValueExW(hNewKey, NULL, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) { */
        /*                    AddEntryToList(hwndLV, L"(Default)", dwValType, ValBuf, dwValSize); */
        /*                } */
        /*                dwValSize = max_val_size; */
        while (RegEnumValueW(hNewKey, dwIndex, ValName, &dwValNameLen, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS)
        {
            /* Add a terminating 0 character. Usually this is only necessary for strings. */
            ValBuf[dwValSize] = ValBuf[dwValSize + 1] = 0;

            AddEntryToList(hwndLV, ValName, dwValType, ValBuf, dwValSize, -1, TRUE);
            dwValNameLen = max_val_name_len;
            dwValSize = max_val_size;
            dwValType = 0L;
            ++dwIndex;
            if(!wcscmp(ValName, L""))
            {
                AddedDefault = TRUE;
            }
        }
        HeapFree(GetProcessHeap(), 0, ValBuf);
        HeapFree(GetProcessHeap(), 0, ValName);
    }
    RegCloseKey(hNewKey);

    if(!AddedDefault)
    {
        AddEntryToList(hwndLV, L"", REG_SZ, NULL, 0, 0, FALSE);
    }
    c = ListView_GetItemCount(hwndLV);
    for(i = 0; i < c; i++)
    {
        ListView_SetItemState(hwndLV, i, 0, LVIS_FOCUSED | LVIS_SELECTED);
    }
    ListView_SetItemState(hwndLV, iListViewSelect,
                          LVIS_FOCUSED | LVIS_SELECTED,
                          LVIS_FOCUSED | LVIS_SELECTED);
    (void)ListView_Sort(hwndLV, g_iSortedColumn, -1);
    SendMessageW(hwndLV, WM_SETREDRAW, TRUE, 0);

    return TRUE;
}
