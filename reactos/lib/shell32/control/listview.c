/*
 *  ReactOS shell32 - Control Panel ListCtrl implementation
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include <windowsx.h>
#include "control.h"
#include "listview.h"

#include "assert.h"
#include "trace.h"


static int _GetSystemDirectory(LPTSTR buffer, int buflen)
{
#if 0
    return GetSystemDirectory(buffer, buflen);
#else
    return GetCurrentDirectory(buflen, buffer);
//    if (lstrcpyn(buffer, szTestDirName, buflen - 1)) {
//        return lstrlen(buffer);
//    }
//    return 0;
#endif
}


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 250, 500 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT };

CPlApplet* pListHead; // holds pointer to linked list of cpl modules CPlApplet*
//static CPlApplet* pListHead; // holds pointer to linked list of cpl modules CPlApplet*


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void AddAppletsToListView(HWND hwndLV, CPlApplet* pApplet)
{
    LVITEM item;
    UINT count;

    for (count = 0; count < pApplet->count; count++) {
        int index = 0;
        NEWCPLINFO* pCPLInfo = &pApplet->info[count];

        CPlEntry*  pCPlEntry;
        if (!(pCPlEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pCPlEntry))))
            return;
    
        pCPlEntry->hWnd = hwndLV;
        pCPlEntry->nSubProg = count;
        pCPlEntry->pCPlApplet = pApplet;

        if (pCPLInfo->hIcon) { // add the icon to an image list
            HIMAGELIST hImageList;
            hImageList = ListView_GetImageList(hwndLV, LVSIL_NORMAL);
            index = ImageList_AddIcon(hImageList, pCPLInfo->hIcon); 
            hImageList = ListView_GetImageList(hwndLV, LVSIL_SMALL);
            ImageList_AddIcon(hImageList, pCPLInfo->hIcon);
            DestroyIcon(pCPLInfo->hIcon); 
        }
        item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_PARAM; 
        item.iItem = 0;//idx; 
        item.iSubItem = 0; 
        item.state = 0; 
        item.stateMask = 0; 
//        item.pszText = LPSTR_TEXTCALLBACK; 
//        item.cchTextMax = 50;
        item.pszText = pCPLInfo->szName;
        item.cchTextMax = _tcslen(item.pszText);
        item.iImage = index; 
        item.lParam = (LPARAM)pCPlEntry;
#if (_WIN32_IE >= 0x0300)
        item.iIndent = 0;
#endif
        index = ListView_InsertItem(hwndLV, &item);
        if (index != -1 && pCPLInfo->szInfo != NULL) {
            ListView_SetItemText(hwndLV, index, 1, pCPLInfo->szInfo);
        }
    }
}

static void CreateListColumns(HWND hwndLV)
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
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, 50);
        if (ListView_InsertColumn(hwndLV, index, &lvC) == -1) {
            // TODO: handle failure condition...
            break;
        }
    }
}

// InitListViewImageLists - creates image lists for a list view control.
// This function only creates image lists. It does not insert the
// items into the control, which is necessary for the control to be 
// visible.   
// Returns TRUE if successful, or FALSE otherwise. 
// hwndLV - handle to the list view control. 
static BOOL InitListViewImageLists(HWND hwndLV) 
{ 
//    HICON hiconItem;     // icon for list view items 
    HIMAGELIST hLarge;   // image list for icon view 
    HIMAGELIST hSmall;   // image list for other views 
 
    // Create the full-sized icon image lists. 
    hLarge = ImageList_Create(GetSystemMetrics(SM_CXICON), 
        GetSystemMetrics(SM_CYICON), ILC_MASK, 1, 20); 
    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), ILC_MASK, 1, 20); 
 
    // Add an icon to each image list.  
//    hiconItem = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ITEM)); 
//    hiconItem = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CONTROL)); 
//    ImageList_AddIcon(hLarge, hiconItem); 
//    ImageList_AddIcon(hSmall, hiconItem); 
//    DestroyIcon(hiconItem); 
	
    /*********************************************************
    Usually you have multiple icons; therefore, the previous
    four lines of code can be inside a loop. The following code 
    shows such a loop. The icons are defined in the application's
    header file as resources, which are numbered consecutively
    starting with IDS_FIRSTICON. The number of icons is
    defined in the header file as C_ICONS.
	
    for(index = 0; index < C_ICONS; index++) {
        hIconItem = LoadIcon (hInst, MAKEINTRESOURCE (IDS_FIRSTICON + index));
        ImageList_AddIcon(hSmall, hIconItem);
        ImageList_AddIcon(hLarge, hIconItem);
        Destroy(hIconItem);
    }
    *********************************************************/
 
    // Assign the image lists to the list view control. 
    ListView_SetImageList(hwndLV, hLarge, LVSIL_NORMAL); 
    ListView_SetImageList(hwndLV, hSmall, LVSIL_SMALL); 
    return TRUE; 
} 

typedef LONG (WINAPI *CPlApplet_Ptr)(HWND, UINT, LONG, LONG);

static void AddEntryToList(HWND hwndLV, LPTSTR szName, LPTSTR szInfo, CPlEntry* pCPlEntry)
{ 
    LVITEM item;
    int index;

    assert(pCPlEntry);
    memset(&item, 0, sizeof(LVITEM));
    item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_PARAM; 
    if (szName != NULL) {
        item.pszText = szName;
        item.cchTextMax = _tcslen(item.pszText);
        item.iImage = pCPlEntry->nIconIndex; 
    } else {
        item.pszText = LPSTR_TEXTCALLBACK;
        item.cchTextMax = MAX_CPL_NAME;
        item.iImage = I_IMAGECALLBACK;
    }
    item.lParam = (LPARAM)pCPlEntry;
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif
    index = ListView_InsertItem(hwndLV, &item);
    if (index != -1) {
        if (szInfo != NULL) {
            ListView_SetItemText(hwndLV, index, 1, szInfo);
        } else {
            ListView_SetItemText(hwndLV, index, 1, LPSTR_TEXTCALLBACK);
        }
    }
}

#if 0
/*
static CPlApplet* Control_LoadApplet(HWND hwndLV, LPTSTR buffer, CPlApplet** pListHead)
{
    HMODULE hCpl;
    hCpl = LoadLibrary(buffer);
    if (hCpl) {
        CPlApplet_Ptr pCPlApplet;
        pCPlApplet = (CPlApplet_Ptr)(FARPROC)GetProcAddress(hCpl, "CPlApplet");
        if (pCPlApplet)	{
            if (pCPlApplet(hwndLV, CPL_INIT, 0, 0)) {
                int nSubProgs = pCPlApplet(hwndLV, CPL_GETCOUNT, 0, 0);
                if (nSubProgs == 0) {
                    TRACE(_T("No subprogram in applet\n"));
                }

                {
                CPlApplet* applet;
                if (!(applet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet)))) {
                    return applet;
                }
                applet->next = *pListHead;
                *pListHead = applet;
                }
                strncpy(applet->filename, buffer, MAX_PATH);
                while (nSubProgs && nSubProgs--) {
                    CPLINFO cplInfo;
                    memset(&cplInfo, 0, sizeof(CPLINFO));
                    pCPlApplet(hwndLV, CPL_INQUIRE, nSubProgs, (LPARAM)&cplInfo);
                    if (cplInfo.idName == CPL_DYNAMIC_RES) {
#if UNICODE
                        NEWCPLINFO cplNewInfo;
                        memset(&cplNewInfo, 0, sizeof(NEWCPLINFO));
                        cplNewInfo.dwSize = sizeof(NEWCPLINFO);
                        pCPlApplet(hwndLV, CPL_NEWINQUIRE, nSubProgs, (LPARAM)&cplNewInfo);
                        cplNewInfo.lData = hCpl;
                        AddEntryToList(hwndLV, &cplNewInfo);
#endif
                    } else {
                        int index = 0;
                        NEWCPLINFO cplNewInfo;
                        memset(&cplNewInfo, 0, sizeof(NEWCPLINFO));
                        cplNewInfo.dwSize = sizeof(NEWCPLINFO);
                        if (LoadString(hCpl, cplInfo.idName, cplNewInfo.szName, sizeof(cplNewInfo.szName)/sizeof(TCHAR))) {
                        }
                        if (LoadString(hCpl, cplInfo.idInfo, cplNewInfo.szInfo, sizeof(cplNewInfo.szInfo)/sizeof(TCHAR))) {
                        }
                        cplNewInfo.hIcon = LoadImage(hCpl, (LPCTSTR)cplInfo.idIcon, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
                        cplNewInfo.lData = (LONG)hCpl;
                        AddEntryToList(hwndLV, &cplNewInfo);
                    }
                }
                return TRUE;
            } else {
                TRACE(_T("Init of applet has failed\n"));
            }
        } else {
            TRACE(_T("Not a valid control panel applet %s\n"), buffer);
        }
        FreeLibrary(hCpl);
    } else {
        TRACE(_T("Cannot load control panel applet %s\n"), buffer);
    }
    return FALSE;
}
 */
#endif

static void LoadApplet(HWND hwndLV, LPTSTR buffer, CPlApplet** pListHead)
{
    HMODULE hModule;
    hModule = LoadLibrary(buffer);
    if (hModule) {
        CPlApplet_Ptr pCPlApplet;
        pCPlApplet = (CPlApplet_Ptr)(FARPROC)GetProcAddress(hModule, "CPlApplet");
        if (pCPlApplet)	{
            if (pCPlApplet(hwndLV, CPL_INIT, 0, 0)) {
                CPlApplet* applet;
                int nSubProgs = pCPlApplet(hwndLV, CPL_GETCOUNT, 0, 0);
                if (nSubProgs == 0) {
                    TRACE(_T("No subprogram in applet\n"));
                }
                if (!(applet = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*applet)))) {
                    goto loadapplet_error;
                }
                applet->next = *pListHead;
                *pListHead = applet;
                _tcsncpy(applet->filename, buffer, MAX_PATH);
                while (nSubProgs && nSubProgs--) {
                    NEWCPLINFO cplNewInfo;
                    memset(&cplNewInfo, 0, sizeof(NEWCPLINFO));
                    cplNewInfo.dwSize = sizeof(NEWCPLINFO);
                    pCPlApplet(hwndLV, CPL_NEWINQUIRE, nSubProgs, (LPARAM)&cplNewInfo);
                    if (cplNewInfo.hIcon == 0) {
                        CPLINFO cplInfo;
                        memset(&cplInfo, 0, sizeof(CPLINFO));
                        pCPlApplet(hwndLV, CPL_INQUIRE, nSubProgs, (LPARAM)&cplInfo);
                        if (cplInfo.idIcon == 0 || cplInfo.idName == 0) {
                            TRACE(_T("Couldn't get info from sp %u\n"), nSubProgs);
                        } else {
                            TCHAR szName[MAX_CPL_NAME];
                            TCHAR szInfo[MAX_CPL_INFO];
                            HICON hIcon = NULL;
                            CPlEntry* pCPlEntry;
                            if (!(pCPlEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CPlEntry)))) {
                                goto loadapplet_error;
                            }
                            pCPlEntry->nSubProg = nSubProgs;
                            pCPlEntry->lData = cplInfo.lData;
                            pCPlEntry->pCPlApplet = applet;
                            hIcon = LoadImage(hModule, (LPCTSTR)cplInfo.idIcon, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
                            LoadString(hModule, cplInfo.idName, szName, MAX_CPL_NAME);
                            LoadString(hModule, cplInfo.idInfo, szInfo, MAX_CPL_INFO);
                            if (hIcon) { // add the icon to an image list
                                HIMAGELIST hImageList;
                                hImageList = ListView_GetImageList(hwndLV, LVSIL_NORMAL);
                                pCPlEntry->nIconIndex = ImageList_AddIcon(hImageList, hIcon); 
                                hImageList = ListView_GetImageList(hwndLV, LVSIL_SMALL);
                                ImageList_AddIcon(hImageList, hIcon);
                                DestroyIcon(hIcon); 
                            }
                            AddEntryToList(hwndLV, szName, szInfo, pCPlEntry);
                        }
                    } else {
                        HIMAGELIST hImageList;
                        CPlEntry* pCPlEntry;
                        TRACE(_T("Using CPL_NEWINQUIRE data\n"));
                        if (!(pCPlEntry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CPlEntry)))) {
                            goto loadapplet_error;
                        }
                        applet->hModule = LoadLibrary(buffer);
                        pCPlEntry->nSubProg = nSubProgs;
                        pCPlEntry->lData = cplNewInfo.lData;
                        pCPlEntry->pCPlApplet = applet;
                        hImageList = ListView_GetImageList(hwndLV, LVSIL_NORMAL);
                        pCPlEntry->nIconIndex = ImageList_AddIcon(hImageList, cplNewInfo.hIcon); 
                        hImageList = ListView_GetImageList(hwndLV, LVSIL_SMALL);
                        ImageList_AddIcon(hImageList, cplNewInfo.hIcon);
                        DestroyIcon(cplNewInfo.hIcon); 
                        AddEntryToList(hwndLV, NULL, NULL, pCPlEntry);
                    }
                }
            } else {
                TRACE(_T("Init of applet has failed\n"));
            }
        } else {
            TRACE(_T("Not a valid control panel applet %s\n"), buffer);
        }
loadapplet_error:
        FreeLibrary(hModule);
    } else {
        TRACE(_T("Cannot load control panel applet %s\n"), buffer);
    }
}


static BOOL InitListViewItems(HWND hwndLV, LPTSTR szPath)
{
    WIN32_FIND_DATA data;
    HANDLE hFind;
    TCHAR buffer[MAX_PATH+10], *p;
    UINT length;

    length = _GetSystemDirectory(buffer, sizeof(buffer)/sizeof(TCHAR));
    p = &buffer[length];
	lstrcpy(p, _T("\\*.cpl"));
    memset(&data, 0, sizeof(WIN32_FIND_DATA));
	hFind = FindFirstFile(buffer, &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
#if 0
                CPlApplet* pApplet;
			    lstrcpy(p+1, data.cFileName);
                pApplet = Control_LoadApplet(hwndLV, buffer, &pListHead);
                if (pApplet != NULL) {
                    AddAppletsToListView(hwndLV, pApplet);
                }
#else
			    lstrcpy(p+1, data.cFileName);
                LoadApplet(hwndLV, buffer, &pListHead);
#endif
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
    }
    return TRUE;
}

// OnGetDispInfo - processes the LVN_GETDISPINFO notification message. 
static void OnGetDispInfo(HWND hWnd, NMLVDISPINFO* plvdi)
{
    CPlEntry* pCPlEntry = (CPlEntry*)plvdi->item.lParam;

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0; 
    if (pCPlEntry != NULL) {
        CPlApplet* pApplet = pCPlEntry->pCPlApplet;
        assert(pApplet);
        if (pApplet->hModule) {
            CPlApplet_Ptr pCPlApplet;
            pCPlApplet = (CPlApplet_Ptr)(FARPROC)GetProcAddress(pApplet->hModule, "CPlApplet");
            if (pCPlApplet)	{
                static NEWCPLINFO cplNewInfo;
                memset(&cplNewInfo, 0, sizeof(NEWCPLINFO));
                cplNewInfo.dwSize = sizeof(NEWCPLINFO);
                pCPlApplet(hWnd, CPL_NEWINQUIRE, pCPlEntry->nSubProg, (LPARAM)&cplNewInfo);

                if (plvdi->item.mask && LVIF_IMAGE) {
                    if (cplNewInfo.hIcon) { // add the icon to an image list
                        HIMAGELIST hImageList;
                        hImageList = ListView_GetImageList(hWnd, LVSIL_NORMAL);
                        pCPlEntry->nIconIndex = ImageList_ReplaceIcon(hImageList, pCPlEntry->nIconIndex, cplNewInfo.hIcon); 
                        hImageList = ListView_GetImageList(hWnd, LVSIL_SMALL);
                        ImageList_ReplaceIcon(hImageList, pCPlEntry->nIconIndex, cplNewInfo.hIcon);
                        DestroyIcon(cplNewInfo.hIcon); 
                    }
                    plvdi->item.iImage = pCPlEntry->nIconIndex;
                }
                if (plvdi->item.mask && LVIF_STATE) {
                }
                if (plvdi->item.mask && LVIF_TEXT) {
                    switch (plvdi->item.iSubItem) {
                    case 0:
                        plvdi->item.pszText = cplNewInfo.szName;
                        plvdi->item.cchTextMax = _tcslen(plvdi->item.pszText); 
                        break;
                    case 1:
                        plvdi->item.pszText = cplNewInfo.szInfo;
                        plvdi->item.cchTextMax = _tcslen(plvdi->item.pszText); 
                        break;
                    case 2:
                        plvdi->item.pszText = _T("");
                        plvdi->item.cchTextMax = _tcslen(plvdi->item.pszText); 
                        break;
                    }
                }
            }
        }
    }
} 

static void OnDeleteItem(NMLISTVIEW* pnmlv)
{
    CPlEntry* pCPlEntry = (CPlEntry*)pnmlv->lParam;
    if (pCPlEntry != NULL) {
        HeapFree(GetProcessHeap(), 0, pCPlEntry);
    }
}

static void OnItemChanged(NMLISTVIEW* pnmlv)
{
}

void Control_LaunchApplet(HWND hwndLV, CPlEntry* pCPlEntry)
{
    CPlApplet_Ptr pCPlApplet;
    CPlApplet* pApplet = pCPlEntry->pCPlApplet;

    assert(pApplet);
    if (pApplet->hModule == NULL) {
        pApplet->hModule = LoadLibrary(pApplet->filename);
        pCPlApplet = (CPlApplet_Ptr)(FARPROC)GetProcAddress(pApplet->hModule, "CPlApplet");
        if (pCPlApplet)	{
            if (pCPlApplet(hwndLV, CPL_INIT, 0, 0)) {
                unsigned int nSubProgs = pCPlApplet(hwndLV, CPL_GETCOUNT, 0, 0);
                if (nSubProgs < pCPlEntry->nSubProg) {
                    TRACE(_T("Only %u subprograms in applet, requested %u\n"), nSubProgs, pCPlEntry->nSubProg);
                    return;
                }
            } else {
                TRACE(_T("Init of applet has failed\n"));
                return;
            }
        } else {
            TRACE(_T("Not a valid control panel applet %s\n"), pApplet->filename);
            return;
        }
    }
    if (pApplet->hModule) {
        pCPlApplet = (CPlApplet_Ptr)(FARPROC)GetProcAddress(pApplet->hModule, "CPlApplet");
        if (pCPlApplet)	{
            TCHAR* extraPmts = NULL;
            if (!pCPlApplet(hwndLV, CPL_STARTWPARMS, pCPlEntry->nSubProg, (LPARAM)extraPmts))
                pCPlApplet(hwndLV, CPL_DBLCLK, pCPlEntry->nSubProg, pCPlEntry->lData);
        }

    }
//        NEWCPLINFO* pCPLInfo = &(pCPlEntry->pCPlApplet->info[pCPlEntry->nSubProg]);
//        TCHAR* extraPmts = NULL;
//        if (pCPLInfo->dwSize && pApplet->proc) {
//            if (!pApplet->proc(pApplet->hWnd, CPL_STARTWPARMS, pCPlEntry->nSubProg, (LPARAM)extraPmts))
//                pApplet->proc(pApplet->hWnd, CPL_DBLCLK, pCPlEntry->nSubProg, pCPLInfo->lData);
//        }
}

static void OnDblClick(HWND hWnd, NMITEMACTIVATE* nmitem)
{
    LVHITTESTINFO info;
/*
#ifdef _MSC_VER
    switch (nmitem->uKeyFlags) {
    case LVKF_ALT:     //  The ALT key is pressed.  
        break;
    case LVKF_CONTROL: //  The CTRL key is pressed.
        break;
    case LVKF_SHIFT:   //  The SHIFT key is pressed.   
        break;
    }
#endif
 */
    info.pt.x = nmitem->ptAction.x;
    info.pt.y = nmitem->ptAction.y;
    if (ListView_HitTest(hWnd, &info) != -1) {
        LVITEM item;
        item.mask = LVIF_PARAM;
        item.iItem = info.iItem;
        if (ListView_GetItem(hWnd, &item)) {
            Control_LaunchApplet(hWnd, (CPlEntry*)item.lParam);
        }
    }
}

struct CompareData {
    HWND hListWnd;
    int  nSortColumn;
};

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    struct CompareData* pcd = (struct CompareData*)lParamSort;

    TCHAR buf1[MAX_CPL_INFO];
    TCHAR buf2[MAX_CPL_INFO];

    ListView_GetItemText(pcd->hListWnd, lParam1, pcd->nSortColumn, buf1, sizeof(buf1)/sizeof(TCHAR));
    ListView_GetItemText(pcd->hListWnd, lParam2, pcd->nSortColumn, buf2, sizeof(buf2)/sizeof(TCHAR));
    return _tcscmp(buf1, buf2);
}

/*
static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    TCHAR buf1[1000];
    TCHAR buf2[1000];

    ListView_GetItemText((HWND)lParamSort, lParam1, 0, buf1, sizeof(buf1)/sizeof(TCHAR));
    ListView_GetItemText((HWND)lParamSort, lParam2, 0, buf2, sizeof(buf2)/sizeof(TCHAR));
    return _tcscmp(buf1, buf2);
}

static void ListViewPopUpMenu(HWND hWnd, POINT pt)
{
}

BOOL ListView_SortItemsEx(
    HWND hwnd, 
    PFNLVCOMPARE pfnCompare, 
    LPARAM lParamSort
);


 */
#ifdef __GNUC__
//#define LVM_FIRST               0x1000      // ListView messages
#define LVM_SORTITEMSEX          (0x1000 + 81)
#define ListView_SortItemsEx(hwndLV, _pfnCompare, _lPrm) \
  (BOOL)SendMessage((hwndLV), LVM_SORTITEMSEX, (WPARAM)(LPARAM)(_lPrm), (LPARAM)(PFNLVCOMPARE)(_pfnCompare))
#endif

BOOL SortListView(HWND hWnd, int nSortColumn)
{
    struct CompareData cd = { hWnd, nSortColumn };
    return ListView_SortItemsEx(hWnd, CompareFunc, &cd);
}

BOOL ListViewNotifyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    NMITEMACTIVATE* nmitem = (LPNMITEMACTIVATE)lParam;

    if (nmitem->hdr.idFrom == LIST_WINDOW) {
        switch (((LPNMHDR)lParam)->code) { 
        case LVN_GETDISPINFO: 
            OnGetDispInfo(hWnd, (NMLVDISPINFO*)lParam); 
            break; 
        case LVN_DELETEITEM:
            OnDeleteItem((NMLISTVIEW*)lParam);
            //pnmv = (LPNMLISTVIEW) lParam
            break;
        case LVN_ITEMCHANGED:
            OnItemChanged((NMLISTVIEW*)lParam);
            break;
        case NM_DBLCLK:
            OnDblClick(hWnd, nmitem);
            break;
        //case NM_RCLICK:
            //OnRightClick(hWnd, nmitem);
            //break; 
        default:
            return FALSE;
        }
    }
    return TRUE;
}

VOID DestroyListView(HWND hwndLV)
{
    if (pListHead)
        while ((pListHead = Control_UnloadApplet(pListHead)));
}

BOOL RefreshListView(HWND hwndLV, LPTSTR szPath)
{ 
    if (hwndLV != NULL) {
        ListView_DeleteAllItems(hwndLV);
    }
    return InitListViewItems(hwndLV, szPath);
}

HWND CreateListView(HWND hwndParent, int id)
{ 
    RECT rcClient;
    HWND hwndLV;
 
    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, _T("List View"), 
        WS_VISIBLE | WS_CHILD, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
    ListView_SetExtendedListViewStyle(hwndLV,  LVS_EX_FULLROWSELECT);
    CreateListColumns(hwndLV);

    // Initialize the image list, and add items to the control. 
    if (!InitListViewImageLists(hwndLV) || !InitListViewItems(hwndLV, NULL/*szPath*/)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
    return hwndLV;
} 

