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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include "main.h"

#define CX_ICON    16
#define CY_ICON    16
#define NUM_ICONS    2

int Image_String = 0;
int Image_Bin = 0;

typedef struct tagLINE_INFO
{
    DWORD dwValType;
    LPTSTR name;
    void* val;
    size_t val_len;
} LINE_INFO, *PLINE_INFO;

/*******************************************************************************
 * Global and Local Variables:
 */

static DWORD g_columnToSort = ~0UL;
static BOOL  g_invertSort = FALSE;
static LPTSTR g_valueName;

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 200, 175, 400 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };

LPCTSTR GetValueName(HWND hwndLV, int iStartAt)
{
    int item, len, maxLen;
    LPTSTR newStr;
    LVITEM LVItem;
    PLINE_INFO lineinfo;

    if (!g_valueName) g_valueName = HeapAlloc(GetProcessHeap(), 0, 1024);
    if (!g_valueName) return NULL;
    *g_valueName = 0;
    maxLen = HeapSize(GetProcessHeap(), 0, g_valueName);
    if (maxLen == (SIZE_T) - 1) return NULL;

    item = ListView_GetNextItem(hwndLV, iStartAt, LVNI_SELECTED);
    if (item == -1) return NULL;
    LVItem.mask = LVIF_PARAM;
    LVItem.iItem = item;
    for(;;)
    {
      if(ListView_GetItem(hwndLV, &LVItem))
      {
        lineinfo = (PLINE_INFO)LVItem.lParam;
        if(!lineinfo->name)
        {
          *g_valueName = 0;
          return g_valueName;
        }
        len = _tcslen(lineinfo->name);
        if (len < maxLen - 1) break;
        newStr = HeapReAlloc(GetProcessHeap(), 0, g_valueName, maxLen * 2);
        if (!newStr) return NULL;
        g_valueName = newStr;
        maxLen *= 2;
      }
      else
        return NULL;
    }
    memcpy(g_valueName, lineinfo->name, sizeof(TCHAR) * (len + 1));
    return g_valueName;
}

BOOL IsDefaultValue(HWND hwndLV, int i)
{
  PLINE_INFO lineinfo;
  LVITEM Item;
  
  Item.mask = LVIF_PARAM;
  Item.iItem = i;
  if(ListView_GetItem(hwndLV, &Item))
  {
    lineinfo = (PLINE_INFO)Item.lParam;
    return lineinfo && (!lineinfo->name || !_tcscmp(lineinfo->name, _T("")));
  }
  return FALSE;
}

/*******************************************************************************
 * Local module support methods
 */
static void AddEntryToList(HWND hwndLV, LPTSTR Name, DWORD dwValType, void* ValBuf, DWORD dwCount, int Position, BOOL ValExists)
{
    LINE_INFO* linfo;
    LVITEM item;
    int index;

    linfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINE_INFO) + dwCount);
    linfo->dwValType = dwValType;
    linfo->val_len = dwCount;
    if(dwCount > 0)
    {
      memcpy(&linfo[1], ValBuf, dwCount);
    }
    linfo->name = _tcsdup(Name);

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    item.iItem = (Position == -1 ? 0: Position);
    item.iSubItem = 0;
    item.state = 0;
    item.stateMask = 0;
    item.pszText = Name;
    item.cchTextMax = _tcslen(item.pszText);
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
    if (index != -1) {
        switch (dwValType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            if(dwCount > 0)
            {
              ListView_SetItemText(hwndLV, index, 2, ValBuf);
            }
            else if(!ValExists)
            {
              TCHAR buffer[255];
              /* load (value not set) string */
              LoadString(hInst, IDS_VALUE_NOT_SET, buffer, sizeof(buffer)/sizeof(TCHAR));
              ListView_SetItemText(hwndLV, index, 2, buffer);
            }
            break;
        case REG_MULTI_SZ:
            {
              LPTSTR src, str, cursrc;
              if(dwCount >= 2)
              {
	          src = (LPTSTR)ValBuf;
		  str = HeapAlloc(GetProcessHeap(), 0, dwCount);
                  if(str != NULL)
                  {
                      *str = _T('\0');
		      /* concatenate all srings */
                      while(*src != _T('\0'))
                      {
			  _tcscat(str, src);
			  _tcscat(str, _T(" "));
			  src += _tcslen(src) + 1;
                      }
                      ListView_SetItemText(hwndLV, index, 2, str);
		      HeapFree(GetProcessHeap(), 0, str);
                  }
                  else
                    ListView_SetItemText(hwndLV, index, 2, _T(""));
              }
              else
                ListView_SetItemText(hwndLV, index, 2, _T(""));
            }
            break;
        case REG_DWORD: {
                TCHAR buf[200];
                if(dwCount == sizeof(DWORD))
                {
                  wsprintf(buf, _T("0x%08X (%d)"), *(DWORD*)ValBuf, *(DWORD*)ValBuf);
                }
                else
                {
                  LoadString(hInst, IDS_INVALID_DWORD, buf, sizeof(buf)/sizeof(TCHAR));
                }
                ListView_SetItemText(hwndLV, index, 2, buf);
            }
            /*            lpsRes = convertHexToDWORDStr(lpbData, dwLen); */
            break;
        default:
	    {
                unsigned int i;
                LPBYTE pData = (LPBYTE)ValBuf;
                LPTSTR strBinary;
                if(dwCount > 0)
                {
		    strBinary = HeapAlloc(GetProcessHeap(), 0, (dwCount * sizeof(TCHAR) * 3) + 1);
                    for (i = 0; i < dwCount; i++)
                    {
                        wsprintf( strBinary + i*3, _T("%02X "), pData[i] );
                    }
                    strBinary[dwCount * 3] = 0;
                    ListView_SetItemText(hwndLV, index, 2, strBinary);
                    HeapFree(GetProcessHeap(), 0, strBinary);
                }
                else
                {
                    TCHAR szText[128];
		    LoadString(hInst, IDS_BINARY_EMPTY, szText, sizeof(szText)/sizeof(TCHAR));
		    ListView_SetItemText(hwndLV, index, 2, szText);
                }
            }
            break;
        }
    }
}

static BOOL CreateListColumns(HWND hWndListView)
{
    TCHAR szText[50];
    int index;
    LV_COLUMN lvC;

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    /* Load the column labels from the resource file. */
    for (index = 0; index < MAX_LIST_COLUMNS; index++) {
        lvC.iSubItem = index;
        lvC.cx = default_column_widths[index];
        lvC.fmt = column_alignment[index];
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, sizeof(szText)/sizeof(TCHAR));
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
                                 ILC_MASK, 0, NUM_ICONS)) == NULL)
        return FALSE;

    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BIN));
    Image_Bin = ImageList_AddIcon(himl, hico);
    
    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_STRING));
    Image_String = ImageList_AddIcon(himl, hico);
    

    /* Fail if not all of the images were added.  */
    if (ImageList_GetImageCount(himl) < NUM_ICONS)
    {
      return FALSE;
    }

    /* Associate the image list with the tree view control.  */
    ListView_SetImageList(hwndLV, himl, LVSIL_SMALL);

    return TRUE;
}

/* OnGetDispInfo - processes the LVN_GETDISPINFO notification message.  */

static void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    static TCHAR buffer[200];

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0;

    switch (plvdi->item.iSubItem) {
    case 0:
        LoadString(hInst, IDS_DEFAULT_VALUE_NAME, buffer, sizeof(buffer)/sizeof(TCHAR));
	plvdi->item.pszText = buffer;
        break;
    case 1:
        switch (((LINE_INFO*)plvdi->item.lParam)->dwValType) {
        case REG_SZ:
            plvdi->item.pszText = _T("REG_SZ");
            break;
        case REG_EXPAND_SZ:
            plvdi->item.pszText = _T("REG_EXPAND_SZ");
            break;
        case REG_BINARY:
            plvdi->item.pszText = _T("REG_BINARY");
            break;
        case REG_DWORD:
            plvdi->item.pszText = _T("REG_DWORD");
            break;
        case REG_DWORD_BIG_ENDIAN:
            plvdi->item.pszText = _T("REG_DWORD_BIG_ENDIAN");
            break;
        case REG_MULTI_SZ:
            plvdi->item.pszText = _T("REG_MULTI_SZ");
            break;
        case REG_LINK:
            plvdi->item.pszText = _T("REG_LINK");
            break;
        case REG_RESOURCE_LIST:
            plvdi->item.pszText = _T("REG_RESOURCE_LIST");
            break;
        case REG_FULL_RESOURCE_DESCRIPTOR:
            plvdi->item.pszText = _T("REG_FULL_RESOURCE_DESCRIPTOR");
            break;
        case REG_RESOURCE_REQUIREMENTS_LIST:
            plvdi->item.pszText = _T("REG_RESOURCE_REQUIREMENTS_LIST");
            break;
        case REG_NONE:
            plvdi->item.pszText = _T("REG_NONE");
            break;
        default: {
            TCHAR buf2[200];
	    LoadString(hInst, IDS_UNKNOWN_TYPE, buf2, sizeof(buf2)/sizeof(TCHAR));
	    wsprintf(buffer, buf2, ((LINE_INFO*)plvdi->item.lParam)->dwValType);
            plvdi->item.pszText = buffer;
            break;
          }
        }
        break;
    case 3:
        plvdi->item.pszText = _T("");
        break;
    }
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LINE_INFO*l, *r;
    l = (LINE_INFO*)lParam1;
    r = (LINE_INFO*)lParam2;
        
    if (g_columnToSort == ~0UL) 
        g_columnToSort = 0;
    
    if (g_columnToSort == 1 && l->dwValType != r->dwValType)
        return g_invertSort ? (int)r->dwValType - (int)l->dwValType : (int)l->dwValType - (int)r->dwValType;
    if (g_columnToSort == 2) {
        /* FIXME: Sort on value */
    }
    return g_invertSort ? _tcscmp(r->name, l->name) : _tcscmp(l->name, r->name);
}

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) {
        /*    case ID_FILE_OPEN: */
        /*        break; */
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result)
{
    NMLVDISPINFO* Info;
    *Result = TRUE;
    switch (((LPNMHDR)lParam)->code) {
        case LVN_GETDISPINFO:
            OnGetDispInfo((NMLVDISPINFO*)lParam);
            return TRUE;
        case LVN_COLUMNCLICK:
            if (g_columnToSort == ((LPNMLISTVIEW)lParam)->iSubItem)
                g_invertSort = !g_invertSort;
            else {
                g_columnToSort = ((LPNMLISTVIEW)lParam)->iSubItem;
                g_invertSort = FALSE;
            }
                    
            ListView_SortItems(hWnd, CompareFunc, (WPARAM)hWnd);
            return TRUE;
        case NM_DBLCLK: 
            {
                SendMessage(hFrameWnd, WM_COMMAND, MAKEWPARAM(ID_EDIT_MODIFY, 0), 0);
            }
            return TRUE;

        case LVN_BEGINLABELEDIT:
            {
              PLINE_INFO lineinfo;
              Info = (NMLVDISPINFO*)lParam;
              if(Info)
              {
                lineinfo = (PLINE_INFO)Info->item.lParam;
                if(!lineinfo->name || !_tcscmp(lineinfo->name, _T("")))
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
            }
    }
    return FALSE;
}


HWND CreateListView(HWND hwndParent, int id)
{
    RECT rcClient;
    HWND hwndLV;

    /* Get the dimensions of the parent window's client area, and create the list view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, _T("List View"),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_EDITLABELS,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, (HMENU)id, hInst, NULL);
    if (!hwndLV) return NULL;
    
    /* Initialize the image list, and add items to the control.  */
    if (!CreateListColumns(hwndLV)) goto fail;
    if (!InitListViewImageLists(hwndLV)) goto fail;

    return hwndLV;
fail:
    DestroyWindow(hwndLV);
    return NULL;
}

BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPCTSTR keyPath)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD val_count;
    HKEY hNewKey;
    LONG errCode;
    INT count, i;
    LVITEM item;
    BOOL AddedDefault = FALSE;

    if (!hwndLV) return FALSE;
    
    ListView_EditLabel(hwndLV, -1);
    
    SendMessage(hwndLV, WM_SETREDRAW, FALSE, 0);
    count = ListView_GetItemCount(hwndLV);
    for (i = 0; i < count; i++) {
        item.mask = LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(hwndLV, &item);
        free(((LINE_INFO*)item.lParam)->name);
        HeapFree(GetProcessHeap(), 0, (void*)item.lParam);
    }
    g_columnToSort = ~0UL;
    ListView_DeleteAllItems(hwndLV);
    
    if(!hKey) return FALSE;

    errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
    if (errCode != ERROR_SUCCESS) return FALSE;

    /* get size information and resize the buffers if necessary */
    errCode = RegQueryInfoKey(hNewKey, NULL, NULL, NULL, NULL, &max_sub_key_len, NULL, 
                              &val_count, &max_val_name_len, &max_val_size, NULL, NULL);

    #define BUF_HEAD_SPACE 2 /* FIXME: check why this is required with ROS ??? */

    if (errCode == ERROR_SUCCESS) {
        TCHAR* ValName = HeapAlloc(GetProcessHeap(), 0, ++max_val_name_len * sizeof(TCHAR) + BUF_HEAD_SPACE);
        DWORD dwValNameLen = max_val_name_len;
        BYTE* ValBuf = HeapAlloc(GetProcessHeap(), 0, ++max_val_size/* + BUF_HEAD_SPACE*/);
        DWORD dwValSize = max_val_size;
        DWORD dwIndex = 0L;
        DWORD dwValType;
        /*                if (RegQueryValueEx(hNewKey, NULL, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) { */
        /*                    AddEntryToList(hwndLV, _T("(Default)"), dwValType, ValBuf, dwValSize); */
        /*                } */
        /*                dwValSize = max_val_size; */
        while (RegEnumValue(hNewKey, dwIndex, ValName, &dwValNameLen, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) {
            ValBuf[dwValSize] = 0;
            AddEntryToList(hwndLV, ValName, dwValType, ValBuf, dwValSize, -1, TRUE);
            dwValNameLen = max_val_name_len;
            dwValSize = max_val_size;
            dwValType = 0L;
            ++dwIndex;
            if(!_tcscmp(ValName, _T("")))
            {
              AddedDefault = TRUE;
            }
        }
        HeapFree(GetProcessHeap(), 0, ValBuf);
        HeapFree(GetProcessHeap(), 0, ValName);
    }
    if(!AddedDefault)
    {
      AddEntryToList(hwndLV, _T(""), REG_SZ, NULL, 0, 0, FALSE);
    }
    ListView_SortItems(hwndLV, CompareFunc, (WPARAM)hwndLV);
    RegCloseKey(hNewKey);
    SendMessage(hwndLV, WM_SETREDRAW, TRUE, 0);

    return TRUE;
}
