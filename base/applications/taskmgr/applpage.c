/*
 *  ReactOS Task Manager
 *
 *  applpage.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *                2005         Klemens Friedl <frik85@reactos.at>
 *                2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

typedef struct
{
    HWND    hWnd;
    WCHAR   szTitle[260];
    HICON   hIcon;
    BOOL    bHung;
} APPLICATION_PAGE_LIST_ITEM, *LPAPPLICATION_PAGE_LIST_ITEM;

HWND            hApplicationPage;               /* Application List Property Page */
HWND            hApplicationPageListCtrl;       /* Application ListCtrl Window */
HWND            hApplicationPageEndTaskButton;  /* Application End Task button */
HWND            hApplicationPageSwitchToButton; /* Application Switch To button */
HWND            hApplicationPageNewTaskButton;  /* Application New Task button */
static int      nApplicationPageWidth;
static int      nApplicationPageHeight;
static BOOL     bSortAscending = TRUE;
DWORD WINAPI    ApplicationPageRefreshThread(void *lpParameter);
BOOL            noApps;
BOOL            bApplicationPageSelectionMade = FALSE;

BOOL CALLBACK   EnumWindowsProc(HWND hWnd, LPARAM lParam);
void            AddOrUpdateHwnd(HWND hWnd, WCHAR *szTitle, HICON hIcon, BOOL bHung);
void            ApplicationPageUpdate(void);
void            ApplicationPageOnNotify(WPARAM wParam, LPARAM lParam);
void            ApplicationPageShowContextMenu1(void);
void            ApplicationPageShowContextMenu2(void);
int CALLBACK    ApplicationPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int             ProcGetIndexByProcessId(DWORD dwProcessId);

#ifdef RUN_APPS_PAGE
static HANDLE   hApplicationThread = NULL;
static DWORD    dwApplicationThread;
#endif

static INT
GetSystemColorDepth(VOID)
{
    DEVMODE pDevMode;
    INT ColorDepth;

    pDevMode.dmSize = sizeof(DEVMODE);
    pDevMode.dmDriverExtra = 0;

    if (!EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &pDevMode))
        return ILC_COLOR;

    switch (pDevMode.dmBitsPerPel)
    {
        case 32: ColorDepth = ILC_COLOR32; break;
        case 24: ColorDepth = ILC_COLOR24; break;
        case 16: ColorDepth = ILC_COLOR16; break;
        case  8: ColorDepth = ILC_COLOR8;  break;
        case  4: ColorDepth = ILC_COLOR4;  break;
        default: ColorDepth = ILC_COLOR;   break;
    }

    return ColorDepth;
}

void AppPageCleanup(void)
{
    int i;
    LV_ITEM item;
    LPAPPLICATION_PAGE_LIST_ITEM pData;
    for (i = 0; i < ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        pData = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
        HeapFree(GetProcessHeap(), 0, pData);
    }
}


INT_PTR CALLBACK
ApplicationPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT       rc;
    int        nXDifference;
    int        nYDifference;
    LV_COLUMN  column;
    WCHAR      szTemp[256];
    int        cx, cy;

    switch (message) {
    case WM_INITDIALOG:

        /* Save the width and height */
        GetClientRect(hDlg, &rc);
        nApplicationPageWidth = rc.right;
        nApplicationPageHeight = rc.bottom;

        /* Update window position */
        SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

        /* Get handles to the controls */
        hApplicationPageListCtrl = GetDlgItem(hDlg, IDC_APPLIST);
        hApplicationPageEndTaskButton = GetDlgItem(hDlg, IDC_ENDTASK);
        hApplicationPageSwitchToButton = GetDlgItem(hDlg, IDC_SWITCHTO);
        hApplicationPageNewTaskButton = GetDlgItem(hDlg, IDC_NEWTASK);

        SetWindowTextW(hApplicationPageListCtrl, L"Tasks");

        /* Initialize the application page's controls */
        column.mask = LVCF_TEXT|LVCF_WIDTH;

        LoadStringW(hInst, IDS_TAB_TASK, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 250;
        (void)ListView_InsertColumn(hApplicationPageListCtrl, 0, &column);    /* Add the "Task" column */
        column.mask = LVCF_TEXT|LVCF_WIDTH;
        LoadStringW(hInst, IDS_TAB_STATUS, szTemp, 256);
        column.pszText = szTemp;
        column.cx = 95;
        (void)ListView_InsertColumn(hApplicationPageListCtrl, 1, &column);    /* Add the "Status" column */

        (void)ListView_SetImageList(hApplicationPageListCtrl, ImageList_Create(16, 16, GetSystemColorDepth()|ILC_MASK, 0, 1), LVSIL_SMALL);
        (void)ListView_SetImageList(hApplicationPageListCtrl, ImageList_Create(32, 32, GetSystemColorDepth()|ILC_MASK, 0, 1), LVSIL_NORMAL);

        UpdateApplicationListControlViewSetting();

        /* Start our refresh thread */
#ifdef RUN_APPS_PAGE
        hApplicationThread = CreateThread(NULL, 0, ApplicationPageRefreshThread, NULL, 0, &dwApplicationThread);
#endif

        /* Refresh page */
        ApplicationPageUpdate();

        return TRUE;

    case WM_DESTROY:
        /* Close refresh thread */
#ifdef RUN_APPS_PAGE
        EndLocalThread(&hApplicationThread, dwApplicationThread);
#endif
        AppPageCleanup();
        break;

    case WM_COMMAND:

        /* Handle the button clicks */
        switch (LOWORD(wParam))
        {
        case IDC_ENDTASK:
            ApplicationPage_OnEndTask();
            break;
        case IDC_SWITCHTO:
            ApplicationPage_OnSwitchTo();
            break;
        case IDC_NEWTASK:
            SendMessageW(hMainWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_NEW, 0), 0);
            break;
        }

        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;

        cx = LOWORD(lParam);
        cy = HIWORD(lParam);
        nXDifference = cx - nApplicationPageWidth;
        nYDifference = cy - nApplicationPageHeight;
        nApplicationPageWidth = cx;
        nApplicationPageHeight = cy;

        /* Reposition the application page's controls */
        GetWindowRect(hApplicationPageListCtrl, &rc);
        cx = (rc.right - rc.left) + nXDifference;
        cy = (rc.bottom - rc.top) + nYDifference;
        SetWindowPos(hApplicationPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageListCtrl, NULL, TRUE);

        GetClientRect(hApplicationPageEndTaskButton, &rc);
        MapWindowPoints(hApplicationPageEndTaskButton, hDlg, (LPPOINT)(PRECT)(&rc), sizeof(RECT)/sizeof(POINT));
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageEndTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageEndTaskButton, NULL, TRUE);

        GetClientRect(hApplicationPageSwitchToButton, &rc);
        MapWindowPoints(hApplicationPageSwitchToButton, hDlg, (LPPOINT)(PRECT)(&rc), sizeof(RECT)/sizeof(POINT));
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageSwitchToButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageSwitchToButton, NULL, TRUE);

        GetClientRect(hApplicationPageNewTaskButton, &rc);
        MapWindowPoints(hApplicationPageNewTaskButton, hDlg, (LPPOINT)(PRECT)(&rc), sizeof(RECT)/sizeof(POINT));
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageNewTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageNewTaskButton, NULL, TRUE);

        break;

    case WM_NOTIFY:
        ApplicationPageOnNotify(wParam, lParam);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_DELETE)
            ProcessPage_OnEndProcess();
        break;

    }

  return 0;
}

void RefreshApplicationPage(void)
{
#ifdef RUN_APPS_PAGE
    /* Signal the event so that our refresh thread */
    /* will wake up and refresh the application page */
    PostThreadMessage(dwApplicationThread, WM_TIMER, 0, 0);
#endif
}

void UpdateApplicationListControlViewSetting(void)
{
    DWORD  dwStyle = GetWindowLongPtrW(hApplicationPageListCtrl, GWL_STYLE);

    dwStyle &= ~(LVS_REPORT | LVS_ICON | LVS_LIST | LVS_SMALLICON);

    switch (TaskManagerSettings.ViewMode) {
    case ID_VIEW_LARGE:   dwStyle |= LVS_ICON; break;
    case ID_VIEW_SMALL:   dwStyle |= LVS_SMALLICON; break;
    case ID_VIEW_DETAILS: dwStyle |= LVS_REPORT; break;
    }
    SetWindowLongPtrW(hApplicationPageListCtrl, GWL_STYLE, dwStyle);

    RefreshApplicationPage();
}

DWORD WINAPI ApplicationPageRefreshThread(void *lpParameter)
{
    MSG msg;
    INT i;
    BOOL                            bItemRemoved = FALSE;
    LV_ITEM                         item;
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    HIMAGELIST                      hImageListLarge;
    HIMAGELIST                      hImageListSmall;

    /* If we couldn't create the event then exit the thread */
    while (1)
    {
        /*  Wait for an the event or application close */
        if (GetMessage(&msg, NULL, 0, 0) <= 0)
            return 0;

        if (msg.message == WM_TIMER)
        {
            /*
             * FIXME:
             *
             * Should this be EnumDesktopWindows() instead?
             */
            noApps = TRUE;
            EnumWindows(EnumWindowsProc, 0);
            if (noApps)
            {
                (void)ListView_DeleteAllItems(hApplicationPageListCtrl);
                bApplicationPageSelectionMade = FALSE;
            }

            /* Get the image lists */
            hImageListLarge = ListView_GetImageList(hApplicationPageListCtrl, LVSIL_NORMAL);
            hImageListSmall = ListView_GetImageList(hApplicationPageListCtrl, LVSIL_SMALL);

            /* Check to see if we need to remove any items from the list */
            for (i=ListView_GetItemCount(hApplicationPageListCtrl)-1; i>=0; i--)
            {
                memset(&item, 0, sizeof(LV_ITEM));
                item.mask = LVIF_IMAGE|LVIF_PARAM;
                item.iItem = i;
                (void)ListView_GetItem(hApplicationPageListCtrl, &item);

                pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
                if (!IsWindow(pAPLI->hWnd)||
                    (wcslen(pAPLI->szTitle) <= 0) ||
                    !IsWindowVisible(pAPLI->hWnd) ||
                    (GetParent(pAPLI->hWnd) != NULL) ||
                    (GetWindow(pAPLI->hWnd, GW_OWNER) != NULL) ||
                    (GetWindowLongPtr(pAPLI->hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
                {
                    ImageList_Remove(hImageListLarge, item.iItem);
                    ImageList_Remove(hImageListSmall, item.iItem);

                    (void)ListView_DeleteItem(hApplicationPageListCtrl, item.iItem);
                    HeapFree(GetProcessHeap(), 0, pAPLI);
                    bItemRemoved = TRUE;
                }
            }

            /*
             * If an item was removed from the list then
             * we need to resync all the items with the
             * image list
             */
            if (bItemRemoved)
            {
                for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
                {
                    memset(&item, 0, sizeof(LV_ITEM));
                    item.mask = LVIF_IMAGE;
                    item.iItem = i;
                    item.iImage = i;
                    (void)ListView_SetItem(hApplicationPageListCtrl, &item);
                }
                bItemRemoved = FALSE;
            }

            ApplicationPageUpdate();
        }
    }
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    HICON   hIcon;
    WCHAR   szText[260];
    BOOL    bLargeIcon;
    BOOL    bHung = FALSE;
    LRESULT bAlive;

    typedef int (FAR __stdcall *IsHungAppWindowProc)(HWND);
    IsHungAppWindowProc IsHungAppWindow;

    /* Skip our window */
    if (hWnd == hMainWnd)
        return TRUE;

    bLargeIcon = (TaskManagerSettings.ViewMode == ID_VIEW_LARGE);

    GetWindowTextW(hWnd, szText, 260); /* Get the window text */

    /* Check and see if this is a top-level app window */
    if ((wcslen(szText) <= 0) ||
        !IsWindowVisible(hWnd) ||
        (GetParent(hWnd) != NULL) ||
        (GetWindow(hWnd, GW_OWNER) != NULL) ||
        (GetWindowLongPtrW(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
    {
        return TRUE; /* Skip this window */
    }

    noApps = FALSE;

#define GET_ICON(type) \
    SendMessageTimeoutW(hWnd, WM_GETICON, (type), 0, SMTO_ABORTIFHUNG, 100, (PDWORD_PTR)&hIcon)

    /* Get the icon for this window */
    hIcon = NULL;
    bAlive = GET_ICON(bLargeIcon ? ICON_BIG : ICON_SMALL);
    if (!hIcon)
    {
        /* We failed, try to retrieve other icons... */
        if (!hIcon && bAlive)
            GET_ICON(bLargeIcon ? ICON_SMALL : ICON_BIG);
        if (!hIcon)
            hIcon = (HICON)(LONG_PTR)GetClassLongPtrW(hWnd, bLargeIcon ? GCL_HICON : GCL_HICONSM);
        if (!hIcon)
            hIcon = (HICON)(LONG_PTR)GetClassLongPtrW(hWnd, bLargeIcon ? GCL_HICONSM : GCL_HICON);

        /* If we still do not have any icon, load the default one */
        if (!hIcon) hIcon = LoadIconW(hInst, bLargeIcon ? MAKEINTRESOURCEW(IDI_WINDOW) : MAKEINTRESOURCEW(IDI_WINDOWSM));
    }
#undef GET_ICON

    bHung = FALSE;

    IsHungAppWindow = (IsHungAppWindowProc)(FARPROC)GetProcAddress(GetModuleHandleW(L"USER32.DLL"), "IsHungAppWindow");

    if (IsHungAppWindow)
        bHung = IsHungAppWindow(hWnd);

    AddOrUpdateHwnd(hWnd, szText, hIcon, bHung);

    return TRUE;
}

void AddOrUpdateHwnd(HWND hWnd, WCHAR *szTitle, HICON hIcon, BOOL bHung)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    HIMAGELIST                      hImageListLarge;
    HIMAGELIST                      hImageListSmall;
    LV_ITEM                         item;
    int                             i;
    BOOL                            bAlreadyInList = FALSE;

    memset(&item, 0, sizeof(LV_ITEM));

    /* Get the image lists */
    hImageListLarge = ListView_GetImageList(hApplicationPageListCtrl, LVSIL_NORMAL);
    hImageListSmall = ListView_GetImageList(hApplicationPageListCtrl, LVSIL_SMALL);

    /* Check to see if it's already in our list */
    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_IMAGE|LVIF_PARAM;
        item.iItem = i;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);

        pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
        if (pAPLI->hWnd == hWnd)
        {
            bAlreadyInList = TRUE;
            break;
        }
    }

    /* If it is already in the list then update it if necessary */
    if (bAlreadyInList)
    {
        /* Check to see if anything needs updating */
        if ((pAPLI->hIcon != hIcon) ||
            (_wcsicmp(pAPLI->szTitle, szTitle) != 0) ||
            (pAPLI->bHung != bHung))
        {
            /* Update the structure */
            pAPLI->hIcon = hIcon;
            pAPLI->bHung = bHung;
            wcscpy(pAPLI->szTitle, szTitle);

            /* Update the image list */
            ImageList_ReplaceIcon(hImageListLarge, item.iItem, hIcon);
            ImageList_ReplaceIcon(hImageListSmall, item.iItem, hIcon);

            /* Update the list view */
            (void)ListView_RedrawItems(hApplicationPageListCtrl, 0, ListView_GetItemCount(hApplicationPageListCtrl));
            /* UpdateWindow(hApplicationPageListCtrl); */
            InvalidateRect(hApplicationPageListCtrl, NULL, 0);
        }
    }
    /* It is not already in the list so add it */
    else
    {
        pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)HeapAlloc(GetProcessHeap(), 0, sizeof(APPLICATION_PAGE_LIST_ITEM));

        pAPLI->hWnd = hWnd;
        pAPLI->hIcon = hIcon;
        pAPLI->bHung = bHung;
        wcscpy(pAPLI->szTitle, szTitle);

        /* Add the item to the list */
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
        ImageList_AddIcon(hImageListLarge, hIcon);
        item.iImage = ImageList_AddIcon(hImageListSmall, hIcon);
        item.pszText = LPSTR_TEXTCALLBACK;
        item.iItem = ListView_GetItemCount(hApplicationPageListCtrl);
        item.lParam = (LPARAM)pAPLI;
        (void)ListView_InsertItem(hApplicationPageListCtrl, &item);
    }

    /* Select first item if any */
    if ((ListView_GetNextItem(hApplicationPageListCtrl, -1, LVNI_FOCUSED | LVNI_SELECTED) == -1) &&
        (ListView_GetItemCount(hApplicationPageListCtrl) > 0) && !bApplicationPageSelectionMade)
    {
        ListView_SetItemState(hApplicationPageListCtrl, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        bApplicationPageSelectionMade = TRUE;
    }
    /*
    else
    {
        bApplicationPageSelectionMade = FALSE;
    }
    */
}

void ApplicationPageUpdate(void)
{
    /* Enable or disable the "End Task" & "Switch To" buttons */
    if (ListView_GetSelectedCount(hApplicationPageListCtrl))
    {
        EnableWindow(hApplicationPageEndTaskButton, TRUE);
    }
    else
    {
        EnableWindow(hApplicationPageEndTaskButton, FALSE);
    }
    /* Enable "Switch To" button only if one app is selected */
    if (ListView_GetSelectedCount(hApplicationPageListCtrl) == 1 )
    {
        EnableWindow(hApplicationPageSwitchToButton, TRUE);
    }
    else
    {
    EnableWindow(hApplicationPageSwitchToButton, FALSE);
    }

    /* If we are on the applications tab the windows menu will be */
    /* present on the menu bar so enable & disable the menu items */
    if (TabCtrl_GetCurSel(hTabWnd) == 0)
    {
        HMENU  hMenu;
        HMENU  hWindowsMenu;

        hMenu = GetMenu(hMainWnd);
        hWindowsMenu = GetSubMenu(hMenu, 3);

        /* Only one item selected */
        if (ListView_GetSelectedCount(hApplicationPageListCtrl) == 1)
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_ENABLED);
        }
        /* More than one item selected */
        else if (ListView_GetSelectedCount(hApplicationPageListCtrl) > 1)
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
        /* No items selected */
        else
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
    }
}

void ApplicationPageOnNotify(WPARAM wParam, LPARAM lParam)
{
    LPNMHDR                       pnmh;
    LV_DISPINFO*                  pnmdi;
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI;
    WCHAR                         szMsg[256];

    pnmh = (LPNMHDR) lParam;
    pnmdi = (LV_DISPINFO*) lParam;

    if (pnmh->hwndFrom == hApplicationPageListCtrl) {
        switch (pnmh->code) {
        case LVN_ITEMCHANGED:
            ApplicationPageUpdate();
            break;

        case LVN_GETDISPINFO:
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)pnmdi->item.lParam;

            /* Update the item text */
            if (pnmdi->item.iSubItem == 0)
            {
                wcsncpy(pnmdi->item.pszText, pAPLI->szTitle, pnmdi->item.cchTextMax);
            }

            /* Update the item status */
            else if (pnmdi->item.iSubItem == 1)
            {
                if (pAPLI->bHung)
                {
                    LoadStringW( GetModuleHandleW(NULL), IDS_NOT_RESPONDING , szMsg, sizeof(szMsg) / sizeof(szMsg[0]));
                }
                else
                {
                    LoadStringW( GetModuleHandleW(NULL), IDS_RUNNING, (LPWSTR) szMsg, sizeof(szMsg) / sizeof(szMsg[0]));
                }
                wcsncpy(pnmdi->item.pszText, szMsg, pnmdi->item.cchTextMax);
            }

            break;

        case NM_RCLICK:

            if (ListView_GetSelectedCount(hApplicationPageListCtrl) < 1)
            {
                ApplicationPageShowContextMenu1();
            }
            else
            {
                ApplicationPageShowContextMenu2();
            }

            break;

        case NM_DBLCLK:

            ApplicationPage_OnSwitchTo();

            break;

        case LVN_KEYDOWN:

            if (((LPNMLVKEYDOWN)lParam)->wVKey == VK_DELETE)
                ApplicationPage_OnEndTask();

            break;

        }
    }
    else if (pnmh->hwndFrom == ListView_GetHeader(hApplicationPageListCtrl))
    {
        switch (pnmh->code)
        {
        case NM_RCLICK:

            if (ListView_GetSelectedCount(hApplicationPageListCtrl) < 1)
            {
                ApplicationPageShowContextMenu1();
            }
            else
            {
                ApplicationPageShowContextMenu2();
            }

            break;

        case HDN_ITEMCLICK:

            (void)ListView_SortItems(hApplicationPageListCtrl, ApplicationPageCompareFunc, 0);
            bSortAscending = !bSortAscending;

            break;
        }
    }

}

void ApplicationPageShowContextMenu1(void)
{
    HMENU  hMenu;
    HMENU  hSubMenu;
    POINT  pt;

    GetCursorPos(&pt);

    hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATION_PAGE_CONTEXT1));
    hSubMenu = GetSubMenu(hMenu, 0);

    CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, TaskManagerSettings.ViewMode, MF_BYCOMMAND);

    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);

    DestroyMenu(hMenu);
}

void ApplicationPageShowContextMenu2(void)
{
    HMENU  hMenu;
    HMENU  hSubMenu;
    POINT  pt;

    GetCursorPos(&pt);

    hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATION_PAGE_CONTEXT2));
    hSubMenu = GetSubMenu(hMenu, 0);

    if (ListView_GetSelectedCount(hApplicationPageListCtrl) == 1)
    {
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_ENABLED);
    }
    else if (ListView_GetSelectedCount(hApplicationPageListCtrl) > 1)
    {
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    }

    SetMenuDefaultItem(hSubMenu, ID_APPLICATION_PAGE_SWITCHTO, MF_BYCOMMAND);

    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);

    DestroyMenu(hMenu);
}

void ApplicationPage_OnView(DWORD dwMode)
{
    HMENU  hMenu;
    HMENU  hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.ViewMode = dwMode;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, dwMode, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnWindowsTile(DWORD dwMode)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;
    HWND*                         hWndArray;
    int                           nWndCount;

    hWndArray = (HWND*)HeapAlloc(GetProcessHeap(), 0, sizeof(HWND) * ListView_GetItemCount(hApplicationPageListCtrl));
    nWndCount = 0;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }

    TileWindows(NULL, dwMode, NULL, nWndCount, hWndArray);
    HeapFree(GetProcessHeap(), 0, hWndArray);
}

void ApplicationPage_OnWindowsMinimize(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                ShowWindowAsync(pAPLI->hWnd, SW_MINIMIZE);
            }
        }
    }
}

void ApplicationPage_OnWindowsMaximize(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                ShowWindowAsync(pAPLI->hWnd, SW_MAXIMIZE);
            }
        }
    }
}

void ApplicationPage_OnWindowsCascade(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;
    HWND*                         hWndArray;
    int                           nWndCount;

    hWndArray = (HWND*)HeapAlloc(GetProcessHeap(), 0, sizeof(HWND) * ListView_GetItemCount(hApplicationPageListCtrl));
    nWndCount = 0;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }
    CascadeWindows(NULL, 0, NULL, nWndCount, hWndArray);
    HeapFree(GetProcessHeap(), 0, hWndArray);
}

void ApplicationPage_OnWindowsBringToFront(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }
    if (pAPLI) {
        SwitchToThisWindow(pAPLI->hWnd, TRUE);
    }
}

void ApplicationPage_OnSwitchTo(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }
    if (pAPLI) {
        SwitchToThisWindow(pAPLI->hWnd, TRUE);
        if (TaskManagerSettings.MinimizeOnUse)
            ShowWindowAsync(hMainWnd, SW_MINIMIZE);
    }
}

void ApplicationPage_OnEndTask(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;

    /* Trick: on Windows, pressing the CTRL key forces the task to be ended */
    BOOL ForceEndTask = !!(GetKeyState(VK_CONTROL) & 0x8000);

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                EndTask(pAPLI->hWnd, 0, ForceEndTask);
            }
        }
    }
}

void ApplicationPage_OnGotoProcess(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM  pAPLI = NULL;
    LV_ITEM                       item;
    int                           i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++) {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        (void)ListView_GetItem(hApplicationPageListCtrl, &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }
    if (pAPLI) {
        DWORD   dwProcessId;

        GetWindowThreadProcessId(pAPLI->hWnd, &dwProcessId);
        /*
         * Switch to the process tab
         */
        TabCtrl_SetCurFocus(hTabWnd, 1);
        /*
         * Select the process item in the list
         */
        i = ProcGetIndexByProcessId(dwProcessId);
        if (i != -1)
        {
            ListView_SetItemState(hProcessPageListCtrl,
                                  i,
                                  LVIS_SELECTED | LVIS_FOCUSED,
                                  LVIS_SELECTED | LVIS_FOCUSED);
            (void)ListView_EnsureVisible(hProcessPageListCtrl,
                                         i,
                                         FALSE);
        }
    }
}

int CALLBACK ApplicationPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LPAPPLICATION_PAGE_LIST_ITEM  Param1;
    LPAPPLICATION_PAGE_LIST_ITEM  Param2;

    if (bSortAscending) {
        Param1 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam1;
        Param2 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam2;
    } else {
        Param1 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam2;
        Param2 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam1;
    }
    return wcscmp(Param1->szTitle, Param2->szTitle);
}
