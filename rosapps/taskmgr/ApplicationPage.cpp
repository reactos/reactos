/*
 *  ReactOS Task Manager
 *
 *  applicationpage.cpp
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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
    
#include "TaskMgr.h"
#include "ApplicationPage.h"
#include "ProcessPage.h"

typedef struct
{
    HWND    hWnd;
    char    szTitle[260];
    HICON   hIcon;
    BOOL    bHung;
} APPLICATION_PAGE_LIST_ITEM, *LPAPPLICATION_PAGE_LIST_ITEM;

HWND            hApplicationPage;               // Application List Property Page

HWND            hApplicationPageListCtrl;       // Application ListCtrl Window
HWND            hApplicationPageEndTaskButton;  // Application End Task button
HWND            hApplicationPageSwitchToButton; // Application Switch To button
HWND            hApplicationPageNewTaskButton;  // Application New Task button

static int      nApplicationPageWidth;
static int      nApplicationPageHeight;

static HANDLE   hApplicationPageEvent = NULL;   // When this event becomes signaled then we refresh the app list

static BOOL     bSortAscending = TRUE;

void            ApplicationPageRefreshThread(void *lpParameter);
BOOL CALLBACK   EnumWindowsProc(HWND hWnd, LPARAM lParam);
void            AddOrUpdateHwnd(HWND hWnd, char *szTitle, HICON hIcon, BOOL bHung);
void            ApplicationPageUpdate(void);
void            ApplicationPageOnNotify(WPARAM wParam, LPARAM lParam);
void            ApplicationPageShowContextMenu1(void);
void            ApplicationPageShowContextMenu2(void);
int CALLBACK    ApplicationPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

void SwitchToThisWindow (
HWND hWnd,   // Handle to the window that should be activated
BOOL bRestore // Restore the window if it is minimized
);

LRESULT CALLBACK ApplicationPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT        rc;
    int         nXDifference;
    int         nYDifference;
    LV_COLUMN   column;
    char        szTemp[256];

    switch (message)
    {
    case WM_INITDIALOG:

        // Save the width and height
        GetClientRect(hDlg, &rc);
        nApplicationPageWidth = rc.right;
        nApplicationPageHeight = rc.bottom;

        // Update window position
        SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

        // Get handles to the controls
        hApplicationPageListCtrl = GetDlgItem(hDlg, IDC_APPLIST);
        hApplicationPageEndTaskButton = GetDlgItem(hDlg, IDC_ENDTASK);
        hApplicationPageSwitchToButton = GetDlgItem(hDlg, IDC_SWITCHTO);
        hApplicationPageNewTaskButton = GetDlgItem(hDlg, IDC_NEWTASK);

        SetWindowText(hApplicationPageListCtrl, "Tasks");

        // Initialize the application page's controls
        column.mask = LVCF_TEXT|LVCF_WIDTH;
        strcpy(szTemp, "Task");
        column.pszText = szTemp;
        column.cx = 250;
        ListView_InsertColumn(hApplicationPageListCtrl, 0, &column);    // Add the "Task" column
        column.mask = LVCF_TEXT|LVCF_WIDTH;
        strcpy(szTemp, "Status");
        column.pszText = szTemp;
        column.cx = 95;
        ListView_InsertColumn(hApplicationPageListCtrl, 1, &column);    // Add the "Status" column

        ListView_SetImageList(hApplicationPageListCtrl, ImageList_Create(16, 16, ILC_COLOR8|ILC_MASK, 0, 1), LVSIL_SMALL);
        ListView_SetImageList(hApplicationPageListCtrl, ImageList_Create(32, 32, ILC_COLOR8|ILC_MASK, 0, 1), LVSIL_NORMAL);

        UpdateApplicationListControlViewSetting();

        // Start our refresh thread
        _beginthread(ApplicationPageRefreshThread, 0, NULL);

        return TRUE;

    case WM_DESTROY:
        // Close the event handle, this will make the
        // refresh thread exit when the wait fails
        CloseHandle(hApplicationPageEvent);
        break;

    case WM_COMMAND:

        // Handle the button clicks
        switch (LOWORD(wParam))
        {
        case IDC_ENDTASK:
            ApplicationPage_OnEndTask();
            break;
        case IDC_SWITCHTO:
            ApplicationPage_OnSwitchTo();
            break;
        case IDC_NEWTASK:
            SendMessage(hMainWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_NEW, 0), 0);
            break;
        }

        break;

    case WM_SIZE:
        int     cx, cy;

        if (wParam == SIZE_MINIMIZED)
            return 0;

        cx = LOWORD(lParam);
        cy = HIWORD(lParam);
        nXDifference = cx - nApplicationPageWidth;
        nYDifference = cy - nApplicationPageHeight;
        nApplicationPageWidth = cx;
        nApplicationPageHeight = cy;

        // Reposition the application page's controls
        GetWindowRect(hApplicationPageListCtrl, &rc);
        cx = (rc.right - rc.left) + nXDifference;
        cy = (rc.bottom - rc.top) + nYDifference;
        SetWindowPos(hApplicationPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageListCtrl, NULL, TRUE);
        
        GetClientRect(hApplicationPageEndTaskButton, &rc);
        MapWindowPoints(hApplicationPageEndTaskButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageEndTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageEndTaskButton, NULL, TRUE);
        
        GetClientRect(hApplicationPageSwitchToButton, &rc);
        MapWindowPoints(hApplicationPageSwitchToButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageSwitchToButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageSwitchToButton, NULL, TRUE);
        
        GetClientRect(hApplicationPageNewTaskButton, &rc);
        MapWindowPoints(hApplicationPageNewTaskButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageNewTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageNewTaskButton, NULL, TRUE);

        break;

    case WM_NOTIFY:
        ApplicationPageOnNotify(wParam, lParam);
        break;
        
    }

  return 0;
}

void RefreshApplicationPage(void)
{
    // Signal the event so that our refresh thread
    // will wake up and refresh the application page
    SetEvent(hApplicationPageEvent);
}

void UpdateApplicationListControlViewSetting(void)
{
    DWORD   dwStyle = GetWindowLong(hApplicationPageListCtrl, GWL_STYLE);

    dwStyle &= ~LVS_REPORT;
    dwStyle &= ~LVS_ICON;
    dwStyle &= ~LVS_LIST;
    dwStyle &= ~LVS_SMALLICON;

    if (TaskManagerSettings.View_LargeIcons)
        dwStyle |= LVS_ICON;
    else if (TaskManagerSettings.View_SmallIcons)
        dwStyle |= LVS_SMALLICON;
    else
        dwStyle |= LVS_REPORT;

    SetWindowLong(hApplicationPageListCtrl, GWL_STYLE, dwStyle);

    RefreshApplicationPage();
}

void ApplicationPageRefreshThread(void *lpParameter)
{
    // Create the event
    hApplicationPageEvent = CreateEvent(NULL, TRUE, TRUE, "Application Page Event");

    // If we couldn't create the event then exit the thread
    if (!hApplicationPageEvent)
        return;

    while (1)
    {
        DWORD   dwWaitVal;

        // Wait on the event
        dwWaitVal = WaitForSingleObject(hApplicationPageEvent, INFINITE);

        // If the wait failed then the event object must have been
        // closed and the task manager is exiting so exit this thread
        if (dwWaitVal == WAIT_FAILED)
            return;

        if (dwWaitVal == WAIT_OBJECT_0)
        {
            // Reset our event
            ResetEvent(hApplicationPageEvent);

            /*
             * FIXME:
             *
             * Should this be EnumDesktopWindows() instead?
             */
            EnumWindows(EnumWindowsProc, 0);
        }
    }
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    HICON       hIcon;
    char        szText[260];
    BOOL        bLargeIcon;

    // Skip our window
    if (hWnd == hMainWnd)
        return TRUE;

    bLargeIcon = TaskManagerSettings.View_LargeIcons ? TRUE : FALSE;

    GetWindowText(hWnd, szText, 260); // Get the window text

    // Get the icon for this window
    hIcon = NULL;
    SendMessageTimeout(hWnd, WM_GETICON, bLargeIcon ? ICON_BIG /*1*/ : ICON_SMALL /*0*/, 0, 0, 1000, (unsigned long*)&hIcon);

    if (!hIcon)
    {
        hIcon = (HICON)GetClassLong(hWnd, bLargeIcon ? GCL_HICON : GCL_HICONSM);
        if (!hIcon) hIcon = (HICON)GetClassLong(hWnd, bLargeIcon ? GCL_HICONSM : GCL_HICON);
        if (!hIcon) SendMessageTimeout(hWnd, WM_QUERYDRAGICON, 0, 0, 0, 1000, (unsigned long*)&hIcon);
        if (!hIcon) SendMessageTimeout(hWnd, WM_GETICON, bLargeIcon ? ICON_SMALL /*0*/ : ICON_BIG /*1*/, 0, 0, 1000, (unsigned long*)&hIcon);
    }

    if (!hIcon)
        hIcon = LoadIcon(hInst, bLargeIcon ? MAKEINTRESOURCE(IDI_WINDOW) : MAKEINTRESOURCE(IDI_WINDOWSM));

    // Check and see if this is a top-level app window
    if ((strlen(szText) <= 0) ||
        !IsWindowVisible(hWnd) ||
        (GetParent(hWnd) != NULL) ||
        (GetWindow(hWnd, GW_OWNER) != NULL) ||
        (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
    {
        return TRUE; // Skip this window
    }

    BOOL    bHung = FALSE;

    typedef int (FAR __stdcall *IsHungAppWindowProc)(HWND);
    IsHungAppWindowProc IsHungAppWindow;

    IsHungAppWindow = (IsHungAppWindowProc)(FARPROC)GetProcAddress(GetModuleHandle("USER32.DLL"), "IsHungAppWindow");

    if (IsHungAppWindow)
        bHung = IsHungAppWindow(hWnd);

    AddOrUpdateHwnd(hWnd, szText, hIcon, bHung);

    return TRUE;
}

void AddOrUpdateHwnd(HWND hWnd, char *szTitle, HICON hIcon, BOOL bHung)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    HIMAGELIST                      hImageListLarge;
    HIMAGELIST                      hImageListSmall;
    LV_ITEM                         item;
    int                             i;
    BOOL                            bAlreadyInList = FALSE;
    BOOL                            bItemRemoved = FALSE;

    memset(&item, 0, sizeof(LV_ITEM));

    // Get the image lists
    hImageListLarge = ListView_GetImageList(hApplicationPageListCtrl, LVSIL_NORMAL);
    hImageListSmall = ListView_GetImageList(hApplicationPageListCtrl, LVSIL_SMALL);

    // Check to see if it's already in our list
    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_IMAGE|LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
        if (pAPLI->hWnd == hWnd)
        {
            bAlreadyInList = TRUE;
            break;
        }
    }

    // If it is already in the list then update it if necessary
    if (bAlreadyInList)
    {
        // Check to see if anything needs updating
        if ((pAPLI->hIcon != hIcon) ||
            (stricmp(pAPLI->szTitle, szTitle) != 0) ||
            (pAPLI->bHung != bHung))
        {
            // Update the structure
            pAPLI->hIcon = hIcon;
            pAPLI->bHung = bHung;
            strcpy(pAPLI->szTitle, szTitle);

            // Update the image list
            ImageList_ReplaceIcon(hImageListLarge, item.iItem, hIcon);
            ImageList_ReplaceIcon(hImageListSmall, item.iItem, hIcon);

            // Update the list view
            ListView_RedrawItems(hApplicationPageListCtrl, 0, ListView_GetItemCount(hApplicationPageListCtrl));
            //UpdateWindow(hApplicationPageListCtrl);
            InvalidateRect(hApplicationPageListCtrl, NULL, 0);
        }
    }
    // It is not already in the list so add it
    else
    {
        pAPLI = new APPLICATION_PAGE_LIST_ITEM;

        pAPLI->hWnd = hWnd;
        pAPLI->hIcon = hIcon;
        pAPLI->bHung = bHung;
        strcpy(pAPLI->szTitle, szTitle);

        // Add the item to the list
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
        ImageList_AddIcon(hImageListLarge, hIcon);
        item.iImage = ImageList_AddIcon(hImageListSmall, hIcon);
        item.pszText = LPSTR_TEXTCALLBACK;
        item.iItem = ListView_GetItemCount(hApplicationPageListCtrl);
        item.lParam = (LPARAM)pAPLI;
        ListView_InsertItem(hApplicationPageListCtrl, &item);
    }


    // Check to see if we need to remove any items from the list
    for (i=ListView_GetItemCount(hApplicationPageListCtrl)-1; i>=0; i--)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_IMAGE|LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
        if (!IsWindow(pAPLI->hWnd)||
            (strlen(pAPLI->szTitle) <= 0) ||
            !IsWindowVisible(pAPLI->hWnd) ||
            (GetParent(pAPLI->hWnd) != NULL) ||
            (GetWindow(pAPLI->hWnd, GW_OWNER) != NULL) ||
            (GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
        {
            ImageList_Remove(hImageListLarge, item.iItem);
            ImageList_Remove(hImageListSmall, item.iItem);

            ListView_DeleteItem(hApplicationPageListCtrl, item.iItem);
            delete pAPLI;
            bItemRemoved = TRUE;
        }
    }

    //
    // If an item was removed from the list then
    // we need to resync all the items with the
    // image list
    //
    if (bItemRemoved)
    {
        for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
        {
            memset(&item, 0, sizeof(LV_ITEM));
            item.mask = LVIF_IMAGE;
            item.iItem = i;
            item.iImage = i;
            ListView_SetItem(hApplicationPageListCtrl, &item);
        }
    }

    ApplicationPageUpdate();
}

void ApplicationPageUpdate(void)
{
    // Enable or disable the "End Task" & "Switch To" buttons
    if (ListView_GetSelectedCount(hApplicationPageListCtrl))
    {
        EnableWindow(hApplicationPageEndTaskButton, TRUE);
        EnableWindow(hApplicationPageSwitchToButton, TRUE);
    }
    else
    {
        EnableWindow(hApplicationPageEndTaskButton, FALSE);
        EnableWindow(hApplicationPageSwitchToButton, FALSE);
    }

    // If we are on the applications tab the the windows menu will
    // be present on the menu bar so enable & disable the menu items
    if (TabCtrl_GetCurSel(hTabWnd) == 0)
    {
        HMENU   hMenu;
        HMENU   hWindowsMenu;

        hMenu = GetMenu(hMainWnd);
        hWindowsMenu = GetSubMenu(hMenu, 3);

        // Only one item selected
        if (ListView_GetSelectedCount(hApplicationPageListCtrl) == 1)
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_ENABLED);
        }
        // More than one item selected
        else if (ListView_GetSelectedCount(hApplicationPageListCtrl) > 1)
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
        // No items selected
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
    int             idctrl;
    LPNMHDR         pnmh;
    LPNM_LISTVIEW   pnmv;
    LV_DISPINFO*    pnmdi;

    idctrl = (int) wParam;
    pnmh = (LPNMHDR) lParam;
    pnmv = (LPNM_LISTVIEW) lParam;
    pnmdi = (LV_DISPINFO*) lParam;

    if (pnmh->hwndFrom == hApplicationPageListCtrl)
    {
        switch (pnmh->code)
        {
        case LVN_ITEMCHANGED:
            ApplicationPageUpdate();
            break;
            
        case LVN_GETDISPINFO:
            LPAPPLICATION_PAGE_LIST_ITEM    pAPLI;

            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)pnmdi->item.lParam;

            // Update the item text
            if (pnmdi->item.iSubItem == 0)
            {
                strncpy(pnmdi->item.pszText, pAPLI->szTitle, pnmdi->item.cchTextMax);
            }

            // Update the item status
            else if (pnmdi->item.iSubItem == 1)
            {
                if (pAPLI->bHung)
                    strncpy(pnmdi->item.pszText, "Not Responding", pnmdi->item.cchTextMax);
                else
                    strncpy(pnmdi->item.pszText, "Running", pnmdi->item.cchTextMax);
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

            ListView_SortItems(hApplicationPageListCtrl, ApplicationPageCompareFunc, 0);
            bSortAscending = !bSortAscending;

            break;
        }
    }

}

void ApplicationPageShowContextMenu1(void)
{
    HMENU   hMenu;
    HMENU   hSubMenu;
    POINT   pt;

    GetCursorPos(&pt);

    hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_APPLICATION_PAGE_CONTEXT1));
    hSubMenu = GetSubMenu(hMenu, 0);

    if (TaskManagerSettings.View_LargeIcons)
        CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);
    else if (TaskManagerSettings.View_SmallIcons)
        CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);

    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);

    DestroyMenu(hMenu);
}

void ApplicationPageShowContextMenu2(void)
{
    HMENU   hMenu;
    HMENU   hSubMenu;
    POINT   pt;

    GetCursorPos(&pt);

    hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_APPLICATION_PAGE_CONTEXT2));
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

void ApplicationPage_OnViewLargeIcons(void)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.View_LargeIcons = TRUE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = FALSE;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnViewSmallIcons(void)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = TRUE;
    TaskManagerSettings.View_Details = FALSE;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnViewDetails(void)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = TRUE;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnWindowsTileHorizontally(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;
    HWND*                           hWndArray;
    int                             nWndCount;

    hWndArray = new HWND[ListView_GetItemCount(hApplicationPageListCtrl)];
    nWndCount = 0;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI)
            {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }

    TileWindows(NULL, MDITILE_HORIZONTAL, NULL, nWndCount, hWndArray);

    delete[] hWndArray;
}

void ApplicationPage_OnWindowsTileVertically(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;
    HWND*                           hWndArray;
    int                             nWndCount;

    hWndArray = new HWND[ListView_GetItemCount(hApplicationPageListCtrl)];
    nWndCount = 0;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI)
            {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }

    TileWindows(NULL, MDITILE_VERTICAL, NULL, nWndCount, hWndArray);

    delete[] hWndArray;
}

void ApplicationPage_OnWindowsMinimize(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI)
            {
                ShowWindow(pAPLI->hWnd, SW_MINIMIZE);
            }
        }
    }
}

void ApplicationPage_OnWindowsMaximize(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI)
            {
                ShowWindow(pAPLI->hWnd, SW_MAXIMIZE);
            }
        }
    }
}

void ApplicationPage_OnWindowsCascade(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;
    HWND*                           hWndArray;
    int                             nWndCount;

    hWndArray = new HWND[ListView_GetItemCount(hApplicationPageListCtrl)];
    nWndCount = 0;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI)
            {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }

    CascadeWindows(NULL, 0, NULL, nWndCount, hWndArray);

    delete[] hWndArray;
}

void ApplicationPage_OnWindowsBringToFront(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }

    if (pAPLI)
    {
        if (IsIconic(pAPLI->hWnd))
            ShowWindow(pAPLI->hWnd, SW_RESTORE);

        BringWindowToTop(pAPLI->hWnd);
    }
}

void ApplicationPage_OnSwitchTo(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }

    if (pAPLI)
    {
        typedef void (WINAPI *PROCSWITCHTOTHISWINDOW) (HWND, BOOL);
        PROCSWITCHTOTHISWINDOW SwitchToThisWindow;

        HMODULE hUser32 = GetModuleHandle("user32");

        SwitchToThisWindow = (PROCSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");


        if (SwitchToThisWindow)
            SwitchToThisWindow(pAPLI->hWnd, TRUE);
        else
        {
            if (IsIconic(pAPLI->hWnd))
                ShowWindow(pAPLI->hWnd, SW_RESTORE);

            BringWindowToTop(pAPLI->hWnd);
            SetForegroundWindow(pAPLI->hWnd);
        }

        if (TaskManagerSettings.MinimizeOnUse)
            ShowWindow(hMainWnd, SW_MINIMIZE);
    }
}

void ApplicationPage_OnEndTask(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEM                         item;
    int                             i;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI)
            {
                PostMessage(pAPLI->hWnd, WM_CLOSE, 0, 0);
            }
        }
    }
}

void ApplicationPage_OnGotoProcess(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM pAPLI = NULL;
    LV_ITEM item;
    int i;
    NMHDR nmhdr;

    for (i=0; i<ListView_GetItemCount(hApplicationPageListCtrl); i++)
    {
        memset(&item, 0, sizeof(LV_ITEM));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        ListView_GetItem(hApplicationPageListCtrl, &item);

        if (item.state & LVIS_SELECTED)
        {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }

    if (pAPLI)
    {
        DWORD   dwProcessId;

        GetWindowThreadProcessId(pAPLI->hWnd, &dwProcessId);

        //
        // Switch to the process tab
        //
        TabCtrl_SetCurFocus(hTabWnd, 1);

        //
        // FIXME: Select the process item in the list
        //
        for (i=0; i<ListView_GetItemCount(hProcessPage); i++)
        {

        }
    }
}

int CALLBACK ApplicationPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LPAPPLICATION_PAGE_LIST_ITEM Param1;
    LPAPPLICATION_PAGE_LIST_ITEM Param2;

	if (bSortAscending)
	{
        Param1 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam1;
        Param2 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam2;
	}
	else
	{
        Param1 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam2;
        Param2 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam1;
    }
    return strcmp(Param1->szTitle, Param2->szTitle);
}
