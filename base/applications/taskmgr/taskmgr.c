/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Application Entry-point
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

#include "perfpage.h"
#include "about.h"
#include "affinity.h"
#include "debug.h"
#include "priority.h"

#define STATUS_WINDOW   2001

/* Global Variables: */
HINSTANCE hInst;                 /* current instance */

HWND hMainWnd;                   /* Main Window */
HWND hStatusWnd;                 /* Status Bar Window */
HWND hTabWnd;                    /* Tab Control Window */

HMENU hWindowMenu = NULL;

int  nMinimumWidth;              /* Minimum width of the dialog (OnSize()'s cx) */
int  nMinimumHeight;             /* Minimum height of the dialog (OnSize()'s cy) */

int  nOldWidth;                  /* Holds the previous client area width */
int  nOldHeight;                 /* Holds the previous client area height */

BOOL bTrackMenu = FALSE;         /* Signals when we display menu hints */
BOOL bWasKeyboardInput = FALSE;  /* TabChange by Keyboard or Mouse ? */

TASKMANAGER_SETTINGS TaskManagerSettings;

////////////////////////////////////////////////////////////////////////////////
// Taken from WinSpy++ 1.7
// https://www.catch22.net/projects/winspy/
// Copyright (c) 2002 by J Brown
//

//
//	Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

//
typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

//
//	Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc fnEnableThemeDialogTexture;

    hUXTheme = LoadLibraryA("uxtheme.dll");

    if(hUXTheme)
    {
        fnEnableThemeDialogTexture =
            (ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

        if(fnEnableThemeDialogTexture)
        {
            fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);

            FreeLibrary(hUXTheme);
            return TRUE;
        }
        else
        {
            // Failed to locate API!
            FreeLibrary(hUXTheme);
            return FALSE;
        }
    }
    else
    {
        // Not running under XP? Just fail gracefully
        return FALSE;
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow)
{
    HANDLE hProcess;
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    HANDLE hMutex;

    /* check wether we're already running or not */
    hMutex = CreateMutexW(NULL, TRUE, L"taskmgrros");
    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        /* Restore existing taskmanager and bring window to front */
        /* Relies on the fact that the application title string and window title are the same */
        HWND hTaskMgr;
        TCHAR szTaskmgr[128];

        LoadString(hInst, IDS_APP_TITLE, szTaskmgr, _countof(szTaskmgr));
        hTaskMgr = FindWindow(NULL, szTaskmgr);

        if (hTaskMgr != NULL)
        {
            SendMessage(hTaskMgr, WM_SYSCOMMAND, SC_RESTORE, 0);
            SetForegroundWindow(hTaskMgr);
        }

        CloseHandle(hMutex);
        return 0;
    }
    else if (!hMutex)
    {
        return 1;
    }

    /* Initialize global variables */
    hInst = hInstance;

    /* Change our priority class to HIGH */
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
    CloseHandle(hProcess);

    /* Now lets get the SE_DEBUG_NAME privilege
     * so that we can debug processes
     */

    /* Get a token for this process. */
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        /* Get the LUID for the debug privilege. */
        if (LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid))
        {
            tkp.PrivilegeCount = 1;  /* one privilege to set */
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            /* Get the debug privilege for this process. */
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
        }
        CloseHandle(hToken);
    }

    /* Load our settings from the registry */
    LoadSettings();

    /* Initialize perf data */
    if (!PerfDataInitialize())
        return -1;

    /*
     * Set our shutdown parameters: we want to shutdown the very last,
     * without displaying any end task dialog if needed.
     */
    SetProcessShutdownParameters(1, SHUTDOWN_NORETRY);

    DialogBoxW(hInst, (LPCWSTR)IDD_TASKMGR_DIALOG, NULL, TaskManagerWndProc);

    /* Save our settings to the registry */
    SaveSettings();
    PerfDataUninitialize();
    CloseHandle(hMutex);
    if (hWindowMenu)
        DestroyMenu(hWindowMenu);
    return 0;
}

/* Message handler for dialog box. */
INT_PTR CALLBACK
TaskManagerWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#if 0
    HDC              hdc;
    PAINTSTRUCT      ps;
    RECT             rc;
#endif
    LPRECT           pRC;
    LPNMHDR          pnmh;
    WINDOWPLACEMENT  wp;

    switch (message) {
    case WM_INITDIALOG:
        // For now, the Help dialog menu item is disabled because of lacking of HTML Help support
        EnableMenuItem(GetMenu(hDlg), ID_HELP_TOPICS, MF_BYCOMMAND | MF_GRAYED);
        hMainWnd = hDlg;
        return OnCreate(hDlg);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        /* Process menu commands */
        switch (LOWORD(wParam))
        {
        case ID_FILE_NEW:
            TaskManager_OnFileNew();
            break;
        case ID_OPTIONS_ALWAYSONTOP:
            TaskManager_OnOptionsAlwaysOnTop();
            break;
        case ID_OPTIONS_MINIMIZEONUSE:
            TaskManager_OnOptionsMinimizeOnUse();
            break;
        case ID_OPTIONS_HIDEWHENMINIMIZED:
            TaskManager_OnOptionsHideWhenMinimized();
            break;
        case ID_OPTIONS_SHOW16BITTASKS:
            TaskManager_OnOptionsShow16BitTasks();
            break;
        case ID_RESTORE:
            TaskManager_OnRestoreMainWindow();
            break;
        case ID_VIEW_LARGE:
        case ID_VIEW_SMALL:
        case ID_VIEW_DETAILS:
            ApplicationPage_OnView(LOWORD(wParam));
            break;
        case ID_VIEW_SHOWKERNELTIMES:
            PerformancePage_OnViewShowKernelTimes();
            break;
        case ID_VIEW_CPUHISTORY_ONEGRAPHALL:
            PerformancePage_OnViewCPUHistoryOneGraphAll();
            break;
        case ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU:
            PerformancePage_OnViewCPUHistoryOneGraphPerCPU();
            break;
        case ID_VIEW_UPDATESPEED_HIGH:
        case ID_VIEW_UPDATESPEED_NORMAL:
        case ID_VIEW_UPDATESPEED_LOW:
        case ID_VIEW_UPDATESPEED_PAUSED:
            TaskManager_OnViewUpdateSpeed(LOWORD(wParam));
            break;
        case ID_VIEW_SELECTCOLUMNS:
            ProcessPage_OnViewSelectColumns();
            break;
        case ID_VIEW_REFRESH:
            PostMessageW(hDlg, WM_TIMER, 0, 0);
            break;
        case ID_WINDOWS_TILEHORIZONTALLY:
            ApplicationPage_OnWindowsTile(MDITILE_HORIZONTAL);
            break;
        case ID_WINDOWS_TILEVERTICALLY:
            ApplicationPage_OnWindowsTile(MDITILE_VERTICAL);
            break;
        case ID_WINDOWS_MINIMIZE:
            ApplicationPage_OnWindowsMinimize();
            break;
        case ID_WINDOWS_MAXIMIZE:
            ApplicationPage_OnWindowsMaximize();
            break;
        case ID_WINDOWS_CASCADE:
            ApplicationPage_OnWindowsCascade();
            break;
        case ID_WINDOWS_BRINGTOFRONT:
            ApplicationPage_OnWindowsBringToFront();
            break;
        case ID_APPLICATION_PAGE_SWITCHTO:
            ApplicationPage_OnSwitchTo();
            break;
        case ID_APPLICATION_PAGE_ENDTASK:
            ApplicationPage_OnEndTask();
            break;
        case ID_APPLICATION_PAGE_GOTOPROCESS:
            ApplicationPage_OnGotoProcess();
            break;
        case ID_PROCESS_PAGE_ENDPROCESS:
            ProcessPage_OnEndProcess();
            break;
        case ID_PROCESS_PAGE_ENDPROCESSTREE:
            ProcessPage_OnEndProcessTree();
            break;
        case ID_PROCESS_PAGE_DEBUG:
            ProcessPage_OnDebug();
            break;
        case ID_PROCESS_PAGE_SETAFFINITY:
            ProcessPage_OnSetAffinity();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_REALTIME:
            DoSetPriority(REALTIME_PRIORITY_CLASS);
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_HIGH:
            DoSetPriority(HIGH_PRIORITY_CLASS);
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_ABOVENORMAL:
            DoSetPriority(ABOVE_NORMAL_PRIORITY_CLASS);
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_NORMAL:
            DoSetPriority(NORMAL_PRIORITY_CLASS);
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_BELOWNORMAL:
            DoSetPriority(BELOW_NORMAL_PRIORITY_CLASS);
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_LOW:
            DoSetPriority(IDLE_PRIORITY_CLASS);
            break;
        case ID_PROCESS_PAGE_PROPERTIES:
            ProcessPage_OnProperties();
            break;
        case ID_PROCESS_PAGE_OPENFILELOCATION:
            ProcessPage_OnOpenFileLocation();
            break;

/* ShutDown items */
        case ID_SHUTDOWN_STANDBY:
            ShutDown_StandBy();
            break;
        case ID_SHUTDOWN_HIBERNATE:
            ShutDown_Hibernate();
            break;
        case ID_SHUTDOWN_POWEROFF:
            ShutDown_PowerOff();
            break;
        case ID_SHUTDOWN_REBOOT:
            ShutDown_Reboot();
            break;
        case ID_SHUTDOWN_LOGOFF:
            ShutDown_LogOffUser();
            break;
        case ID_SHUTDOWN_SWITCHUSER:
            ShutDown_SwitchUser();
            break;
        case ID_SHUTDOWN_LOCKCOMPUTER:
            ShutDown_LockComputer();
            break;
        case ID_SHUTDOWN_DISCONNECT:
            ShutDown_Disconnect();
            break;
        case ID_SHUTDOWN_EJECT_COMPUTER:
            ShutDown_EjectComputer();
            break;

        case ID_HELP_ABOUT:
            OnAbout();
            break;
        case ID_FILE_EXIT:
            EndDialog(hDlg, IDOK);
            break;
        }
        break;

    case WM_ONTRAYICON:
        switch(lParam)
        {
        case WM_RBUTTONDOWN:
            {
            POINT pt;
            BOOL OnTop;
            HMENU hMenu, hPopupMenu;

            GetCursorPos(&pt);

            OnTop = ((GetWindowLongPtrW(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);

            hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_TRAY_POPUP));
            hPopupMenu = GetSubMenu(hMenu, 0);

            if(IsWindowVisible(hMainWnd))
                DeleteMenu(hPopupMenu, ID_RESTORE, MF_BYCOMMAND);
            else
                SetMenuDefaultItem(hPopupMenu, ID_RESTORE, FALSE);

            if(OnTop)
                CheckMenuItem(hPopupMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MF_CHECKED);
            else
                CheckMenuItem(hPopupMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MF_UNCHECKED);

            SetForegroundWindow(hMainWnd);
            TrackPopupMenuEx(hPopupMenu, 0, pt.x, pt.y, hMainWnd, NULL);

            DestroyMenu(hMenu);
            break;
            }
        case WM_LBUTTONDBLCLK:
            TaskManager_OnRestoreMainWindow();
            break;
        }
        break;

    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        if ((pnmh->hwndFrom == hTabWnd) &&
            (pnmh->idFrom == IDC_TAB))
        {
            switch (pnmh->code)
            {
                case TCN_SELCHANGE:
                    TaskManager_OnTabWndSelChange();
                    break;
                case TCN_KEYDOWN:
                    bWasKeyboardInput = TRUE;
                    break;
                case NM_CLICK:
                    bWasKeyboardInput = FALSE;
                break;
            }
        }
        break;

    case WM_SIZING:
        /* Make sure the user is sizing the dialog */
        /* in an acceptable range */
        pRC = (LPRECT)lParam;
        if ((wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT)) {
            /* If the width is too small enlarge it to the minimum */
            if (nMinimumWidth > (pRC->right - pRC->left))
                pRC->left = pRC->right - nMinimumWidth;
        } else {
            /* If the width is too small enlarge it to the minimum */
            if (nMinimumWidth > (pRC->right - pRC->left))
                pRC->right = pRC->left + nMinimumWidth;
        }
        if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT)) {
            /* If the height is too small enlarge it to the minimum */
            if (nMinimumHeight > (pRC->bottom - pRC->top))
                pRC->top = pRC->bottom - nMinimumHeight;
        } else {
            /* If the height is too small enlarge it to the minimum */
            if (nMinimumHeight > (pRC->bottom - pRC->top))
                pRC->bottom = pRC->top + nMinimumHeight;
        }
        return TRUE;
        break;

    case WM_SIZE:
        /* Handle the window sizing in it's own function */
        OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_MOVE:
        /* Handle the window moving in it's own function */
        OnMove(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_DESTROY:
        ShowWindow(hDlg, SW_HIDE);
        TrayIcon_RemoveIcon();
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hDlg, &wp);
        TaskManagerSettings.Left = wp.rcNormalPosition.left;
        TaskManagerSettings.Top = wp.rcNormalPosition.top;
        TaskManagerSettings.Right = wp.rcNormalPosition.right;
        TaskManagerSettings.Bottom = wp.rcNormalPosition.bottom;
        if (IsZoomed(hDlg) || (wp.flags & WPF_RESTORETOMAXIMIZED))
            TaskManagerSettings.Maximized = TRUE;
        else
            TaskManagerSettings.Maximized = FALSE;
        /* Get rid of the allocated command line cache, if any */
        PerfDataDeallocCommandLineCache();
        if (hWindowMenu)
            DestroyMenu(hWindowMenu);
        return DefWindowProcW(hDlg, message, wParam, lParam);

    case WM_TIMER:
        /* Refresh the performance data */
        PerfDataRefresh();
        RefreshApplicationPage();
        RefreshProcessPage();
        RefreshPerformancePage();
        TrayIcon_UpdateIcon();
        break;

    case WM_MENUSELECT:
        TaskManager_OnMenuSelect(hDlg, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;

    case WM_SYSCOLORCHANGE:
        /* Forward WM_SYSCOLORCHANGE to common controls */
        SendMessage(hApplicationPageListCtrl, WM_SYSCOLORCHANGE, 0, 0);
        SendMessage(hProcessPageListCtrl, WM_SYSCOLORCHANGE, 0, 0);
        SendMessage(hProcessPageHeaderCtrl, WM_SYSCOLORCHANGE, 0, 0);
        break;
    }

    return 0;
}

void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr)
{
    SetBkColor(hDC, clr);
    ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
}

static void SetUpdateSpeed(HWND hWnd)
{
    /* Setup update speed (pause=fall down) */
    switch (TaskManagerSettings.UpdateSpeed) {
    case ID_VIEW_UPDATESPEED_HIGH:
        SetTimer(hWnd, 1, 500, NULL);
        break;
    case ID_VIEW_UPDATESPEED_NORMAL:
        SetTimer(hWnd, 1, 2000, NULL);
        break;
    case ID_VIEW_UPDATESPEED_LOW:
        SetTimer(hWnd, 1, 4000, NULL);
        break;
    }
}

BOOL OnCreate(HWND hWnd)
{
    HMENU   hMenu;
    HMENU   hEditMenu;
    HMENU   hViewMenu;
    HMENU   hShutMenu;
    HMENU   hUpdateSpeedMenu;
    HMENU   hCPUHistoryMenu;
    int     nActivePage;
    int     nParts[3];
    RECT    rc;
    WCHAR   szTemp[256];
    WCHAR   szLogOffItem[MAX_PATH];
    LPWSTR  lpUserName;
    TCITEM  item;
    DWORD   len = 0;

    SendMessageW(hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(hInst, MAKEINTRESOURCEW(IDI_TASKMANAGER)));

    /* Initialize the Windows Common Controls DLL */
    InitCommonControls();

    /* Get the minimum window sizes */
    GetWindowRect(hWnd, &rc);
    nMinimumWidth = (rc.right - rc.left);
    nMinimumHeight = (rc.bottom - rc.top);

    /* Create the status bar */
    hStatusWnd = CreateStatusWindow(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, L"", hWnd, STATUS_WINDOW);
    if(!hStatusWnd)
        return FALSE;

    /* Create the status bar panes */
    nParts[0] = STATUS_SIZE1;
    nParts[1] = STATUS_SIZE2;
    nParts[2] = STATUS_SIZE3;
    SendMessageW(hStatusWnd, SB_SETPARTS, _countof(nParts), (LPARAM)(LPINT)nParts);

    /* Create tab pages */
    hTabWnd = GetDlgItem(hWnd, IDC_TAB);
#if 1
    hApplicationPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_APPLICATION_PAGE), hWnd, ApplicationPageWndProc); EnableDialogTheme(hApplicationPage);
    hProcessPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PROCESS_PAGE), hWnd, ProcessPageWndProc); EnableDialogTheme(hProcessPage);
    hPerformancePage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PERFORMANCE_PAGE), hWnd, PerformancePageWndProc); EnableDialogTheme(hPerformancePage);
#else
    hApplicationPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_APPLICATION_PAGE), hTabWnd, ApplicationPageWndProc); EnableDialogTheme(hApplicationPage);
    hProcessPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PROCESS_PAGE), hTabWnd, ProcessPageWndProc); EnableDialogTheme(hProcessPage);
    hPerformancePage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PERFORMANCE_PAGE), hTabWnd, PerformancePageWndProc); EnableDialogTheme(hPerformancePage);
#endif

    /* Insert tabs */
    LoadStringW(hInst, IDS_TAB_APPS, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 0, &item);
    LoadStringW(hInst, IDS_TAB_PROCESSES, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 1, &item);
    LoadStringW(hInst, IDS_TAB_PERFORMANCE, szTemp, 256);
    memset(&item, 0, sizeof(TCITEM));
    item.mask = TCIF_TEXT;
    item.pszText = szTemp;
    (void)TabCtrl_InsertItem(hTabWnd, 2, &item);

    /* Size everything correctly */
    GetClientRect(hWnd, &rc);
    nOldWidth = rc.right;
    nOldHeight = rc.bottom;
    /* nOldStartX = rc.left; */
    /*nOldStartY = rc.top;  */

#define PAGE_OFFSET_LEFT    17
#define PAGE_OFFSET_TOP     72
#define PAGE_OFFSET_WIDTH   (PAGE_OFFSET_LEFT*2)
#define PAGE_OFFSET_HEIGHT  (PAGE_OFFSET_TOP+32)

    if ((TaskManagerSettings.Left != 0) ||
        (TaskManagerSettings.Top != 0) ||
        (TaskManagerSettings.Right != 0) ||
        (TaskManagerSettings.Bottom != 0))
    {
        MoveWindow(hWnd, TaskManagerSettings.Left, TaskManagerSettings.Top, TaskManagerSettings.Right - TaskManagerSettings.Left, TaskManagerSettings.Bottom - TaskManagerSettings.Top, TRUE);
#ifdef __GNUC__TEST__
        MoveWindow(hApplicationPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
        MoveWindow(hProcessPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
        MoveWindow(hPerformancePage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
#endif
    }
    if (TaskManagerSettings.Maximized)
        ShowWindow(hWnd, SW_MAXIMIZE);

    /* Set the always on top style */
    hMenu = GetMenu(hWnd);
    hEditMenu = GetSubMenu(hMenu, 1);
    hViewMenu = GetSubMenu(hMenu, 2);
    hShutMenu = GetSubMenu(hMenu, 4);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);
    hCPUHistoryMenu  = GetSubMenu(hViewMenu, 7);

    /* Check or uncheck the always on top menu item */
    if (TaskManagerSettings.AlwaysOnTop) {
        CheckMenuItem(hEditMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    } else {
        CheckMenuItem(hEditMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }

    /* Check or uncheck the minimize on use menu item */
    if (TaskManagerSettings.MinimizeOnUse)
        CheckMenuItem(hEditMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_UNCHECKED);

    /* Check or uncheck the hide when minimized menu item */
    if (TaskManagerSettings.HideWhenMinimized)
        CheckMenuItem(hEditMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_UNCHECKED);

    /* Check or uncheck the show 16-bit tasks menu item */
    if (TaskManagerSettings.Show16BitTasks)
        CheckMenuItem(hEditMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_UNCHECKED);

    /* Set the view mode */
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, TaskManagerSettings.ViewMode, MF_BYCOMMAND);

    if (TaskManagerSettings.ShowKernelTimes)
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);

    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, TaskManagerSettings.UpdateSpeed, MF_BYCOMMAND);

    if (TaskManagerSettings.CPUHistory_OneGraphPerCPU)
        CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);

    nActivePage = TaskManagerSettings.ActiveTabPage;
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 0);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 1);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, 2);
    TabCtrl_SetCurFocus/*Sel*/(hTabWnd, nActivePage);

    /* Set the username in the "Log Off %s" item of the Shutdown menu */

    /* 1- Get the menu item text and store it temporarily */
    GetMenuStringW(hShutMenu, ID_SHUTDOWN_LOGOFF, szTemp, 256, MF_BYCOMMAND);

    /* 2- Retrieve the username length first, then allocate a buffer for it and call it again */
    if (!GetUserNameW(NULL, &len) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        lpUserName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
        if (lpUserName && GetUserNameW(lpUserName, &len))
        {
            _snwprintf(szLogOffItem, _countof(szLogOffItem), szTemp, lpUserName);
            szLogOffItem[_countof(szLogOffItem) - 1] = UNICODE_NULL;
        }
        else
        {
            _snwprintf(szLogOffItem, _countof(szLogOffItem), szTemp, L"n/a");
        }

        if (lpUserName) HeapFree(GetProcessHeap(), 0, lpUserName);
    }
    else
    {
        _snwprintf(szLogOffItem, _countof(szLogOffItem), szTemp, L"n/a");
    }

    /* 3- Set the menu item text to its formatted counterpart */
    ModifyMenuW(hShutMenu, ID_SHUTDOWN_LOGOFF, MF_BYCOMMAND | MF_STRING, ID_SHUTDOWN_LOGOFF, szLogOffItem);

    /* Setup update speed */
    SetUpdateSpeed(hWnd);

    /*
     * Refresh the performance data
     * Sample it twice so we can establish
     * the delta values & cpu usage
     */
    PerfDataRefresh();
    PerfDataRefresh();

    RefreshApplicationPage();
    RefreshProcessPage();
    RefreshPerformancePage();

    TrayIcon_AddIcon();

    return TRUE;
}

/* OnMove()
 * This function handles all the moving events for the application
 * It moves every child window that needs moving
 */
void OnMove( WPARAM nType, int cx, int cy )
{
#ifdef __GNUC__TEST__
    MoveWindow(hApplicationPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
    MoveWindow(hProcessPage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
    MoveWindow(hPerformancePage, TaskManagerSettings.Left + PAGE_OFFSET_LEFT, TaskManagerSettings.Top + PAGE_OFFSET_TOP, TaskManagerSettings.Right - TaskManagerSettings.Left - PAGE_OFFSET_WIDTH, TaskManagerSettings.Bottom - TaskManagerSettings.Top - PAGE_OFFSET_HEIGHT, FALSE);
#endif
}

/* OnSize()
 * This function handles all the sizing events for the application
 * It re-sizes every window, and child window that needs re-sizing
 */
void OnSize( WPARAM nType, int cx, int cy )
{
    int   nParts[3];
    int   nXDifference;
    int   nYDifference;
    RECT  rc;

    if (nType == SIZE_MINIMIZED)
    {
        if (TaskManagerSettings.HideWhenMinimized)
            ShowWindow(hMainWnd, SW_HIDE);
        return;
    }

    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    /* Update the status bar size */
    GetWindowRect(hStatusWnd, &rc);
    SendMessageW(hStatusWnd, WM_SIZE, nType, MAKELPARAM(cx,rc.bottom - rc.top));

    /* Update the status bar pane sizes */
    nParts[0] = STATUS_SIZE1;
    nParts[1] = STATUS_SIZE2;
    nParts[2] = cx;
    SendMessageW(hStatusWnd, SB_SETPARTS, _countof(nParts), (LPARAM)(LPINT)nParts);

    /* Resize the tab control */
    GetWindowRect(hTabWnd, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hTabWnd, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);

    /* Resize the application page */
    GetWindowRect(hApplicationPage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hApplicationPage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);

    /* Resize the process page */
    GetWindowRect(hProcessPage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hProcessPage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);

    /* Resize the performance page */
    GetWindowRect(hPerformancePage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hPerformancePage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
}

void LoadSettings(void)
{
    HKEY   hKey;
    WCHAR  szSubKey[] = L"Software\\ReactOS\\TaskManager";
    int    i;
    DWORD  dwSize;

    /* Window size & position settings */
    TaskManagerSettings.Maximized = FALSE;
    TaskManagerSettings.Left = 0;
    TaskManagerSettings.Top = 0;
    TaskManagerSettings.Right = 0;
    TaskManagerSettings.Bottom = 0;

    /* Tab settings */
    TaskManagerSettings.ActiveTabPage = 0;

    /* Options menu settings */
    TaskManagerSettings.AlwaysOnTop = TRUE;
    TaskManagerSettings.MinimizeOnUse = TRUE;
    TaskManagerSettings.HideWhenMinimized = FALSE;
    TaskManagerSettings.Show16BitTasks = TRUE;

    /* Update speed settings */
    TaskManagerSettings.UpdateSpeed = ID_VIEW_UPDATESPEED_NORMAL;

    /* Applications page settings */
    TaskManagerSettings.ViewMode = ID_VIEW_DETAILS;

    /* Processes page settings */
    TaskManagerSettings.ShowProcessesFromAllUsers = FALSE; /* It's the default */

    for (i = 0; i < COLUMN_NMAX; i++) {
        TaskManagerSettings.Columns[i] = ColumnPresets[i].bDefaults;
        TaskManagerSettings.ColumnOrderArray[i] = i;
        TaskManagerSettings.ColumnSizeArray[i] = ColumnPresets[i].size;
    }

    TaskManagerSettings.SortColumn = COLUMN_IMAGENAME;
    TaskManagerSettings.SortAscending = TRUE;

    /* Performance page settings */
    TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
    TaskManagerSettings.ShowKernelTimes = FALSE;

    /* Open the key */
    if (RegOpenKeyExW(HKEY_CURRENT_USER, szSubKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;
    /* Read the settings */
    dwSize = sizeof(TASKMANAGER_SETTINGS);
    RegQueryValueExW(hKey, L"Preferences", NULL, NULL, (LPBYTE)&TaskManagerSettings, &dwSize);

    /*
     * ATM, the 'ImageName' column is always visible
     * (and grayed in configuration dialog)
     * This will avoid troubles if the registry gets corrupted.
     */
    TaskManagerSettings.Column_ImageName = TRUE;

    /* Close the key */
    RegCloseKey(hKey);
}

void SaveSettings(void)
{
    HKEY hKey;
    WCHAR szSubKey[] = L"Software\\ReactOS\\TaskManager";

    /* Open (or create) the key */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, szSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;
    /* Save the settings */
    RegSetValueExW(hKey, L"Preferences", 0, REG_BINARY, (LPBYTE)&TaskManagerSettings, sizeof(TASKMANAGER_SETTINGS));
    /* Close the key */
    RegCloseKey(hKey);
}

void TaskManager_OnRestoreMainWindow(void)
{
    BOOL OnTop;

    OnTop = ((GetWindowLongPtrW(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);

    OpenIcon(hMainWnd);
    SetForegroundWindow(hMainWnd);
    SetWindowPos(hMainWnd, (OnTop ? HWND_TOPMOST : HWND_TOP), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

void TaskManager_OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    WCHAR str[100] = L"";

    /*
     * Reset the status bar if we close the current menu, or
     * we open the system menu or hover above a menu separator.
     * Adapted from comctl32!MenuHelp().
     */
    if ((LOWORD(nFlags) == 0xFFFF && hSysMenu == NULL) ||
        (nFlags & (MF_SEPARATOR | MF_SYSMENU)))
    {
        /* Set the status bar for multiple-parts output */
        SendMessageW(hStatusWnd, SB_SIMPLE, (WPARAM)FALSE, (LPARAM)0);
        bTrackMenu = FALSE;

        /* Trigger update of status bar columns and performance page asynchronously */
        RefreshPerformancePage();
        return;
    }

    /* Otherwise, retrieve the appropriate menu hint string */
    if (LoadStringW(hInst, nItemID, str, _countof(str)))
    {
        /* First newline terminates actual string */
        LPWSTR lpsz = wcschr(str, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }

    /* Set the status bar for single-part output, if needed... */
    if (!bTrackMenu)
        SendMessageW(hStatusWnd, SB_SIMPLE, (WPARAM)TRUE, (LPARAM)0);
    bTrackMenu = TRUE;

    /* ... and display the menu hint */
    SendMessageW(hStatusWnd, SB_SETTEXT, SB_SIMPLEID | SBT_NOBORDERS, (LPARAM)str);
}

void TaskManager_OnViewUpdateSpeed(DWORD dwSpeed)
{
    HMENU  hMenu;
    HMENU  hViewMenu;
    HMENU  hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = dwSpeed;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, dwSpeed, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);

    SetUpdateSpeed(hMainWnd);
}

void TaskManager_OnTabWndSelChange(void)
{
    int    i;
    HMENU  hMenu;
    HMENU  hOptionsMenu;
    HMENU  hViewMenu;
    HMENU  hSubMenu;
    WCHAR  szTemp[256];
    SYSTEM_INFO sysInfo;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hOptionsMenu = GetSubMenu(hMenu, 1);
    TaskManagerSettings.ActiveTabPage = TabCtrl_GetCurSel(hTabWnd);
    for (i = GetMenuItemCount(hViewMenu) - 1; i > 2; i--) {
        hSubMenu = GetSubMenu(hViewMenu, i);
        if (hSubMenu)
            DestroyMenu(hSubMenu);
        RemoveMenu(hViewMenu, i, MF_BYPOSITION);
    }
    RemoveMenu(hOptionsMenu, 3, MF_BYPOSITION);
    if (hWindowMenu)
        DestroyMenu(hWindowMenu);
    switch (TaskManagerSettings.ActiveTabPage) {
    case 0:
        ShowWindow(hApplicationPage, SW_SHOW);
        ShowWindow(hProcessPage, SW_HIDE);
        ShowWindow(hPerformancePage, SW_HIDE);
        BringWindowToTop(hApplicationPage);

        LoadStringW(hInst, IDS_MENU_LARGEICONS, szTemp, 256);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_LARGE, szTemp);

        LoadStringW(hInst, IDS_MENU_SMALLICONS, szTemp, 256);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SMALL, szTemp);

        LoadStringW(hInst, IDS_MENU_DETAILS, szTemp, 256);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_DETAILS, szTemp);

        if (GetMenuItemCount(hMenu) <= 5) {
            hWindowMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_WINDOWSMENU));

            LoadStringW(hInst, IDS_MENU_WINDOWS, szTemp, 256);
            InsertMenuW(hMenu, 3, MF_BYPOSITION|MF_POPUP, (UINT_PTR) hWindowMenu, szTemp);

            DrawMenuBar(hMainWnd);
        }
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, TaskManagerSettings.ViewMode, MF_BYCOMMAND);

        /*
         * Give the application list control focus
         */
        if (!bWasKeyboardInput)
            SetFocus(hApplicationPageListCtrl);
        break;

    case 1:
        ShowWindow(hApplicationPage, SW_HIDE);
        ShowWindow(hProcessPage, SW_SHOW);
        ShowWindow(hPerformancePage, SW_HIDE);
        BringWindowToTop(hProcessPage);

        LoadStringW(hInst, IDS_MENU_SELECTCOLUMNS, szTemp, 256);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SELECTCOLUMNS, szTemp);

        LoadStringW(hInst, IDS_MENU_16BITTASK, szTemp, 256);
        AppendMenuW(hOptionsMenu, MF_STRING, ID_OPTIONS_SHOW16BITTASKS, szTemp);

        if (TaskManagerSettings.Show16BitTasks)
            CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
        if (GetMenuItemCount(hMenu) > 5)
        {
            DeleteMenu(hMenu, 3, MF_BYPOSITION);
            DrawMenuBar(hMainWnd);
        }
        /*
         * Give the process list control focus
         */
        if (!bWasKeyboardInput)
            SetFocus(hProcessPageListCtrl);
        break;

    case 2:
        ShowWindow(hApplicationPage, SW_HIDE);
        ShowWindow(hProcessPage, SW_HIDE);
        ShowWindow(hPerformancePage, SW_SHOW);
        BringWindowToTop(hPerformancePage);
        if (GetMenuItemCount(hMenu) > 5) {
            DeleteMenu(hMenu, 3, MF_BYPOSITION);
            DrawMenuBar(hMainWnd);
        }

        GetSystemInfo(&sysInfo);

        /* Hide CPU graph options on single CPU systems */
        if (sysInfo.dwNumberOfProcessors > 1)
        {
            hSubMenu = CreatePopupMenu();

            LoadStringW(hInst, IDS_MENU_ONEGRAPHALLCPUS, szTemp, 256);
            AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_CPUHISTORY_ONEGRAPHALL, szTemp);

            LoadStringW(hInst, IDS_MENU_ONEGRAPHPERCPU, szTemp, 256);
            AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, szTemp);

            LoadStringW(hInst, IDS_MENU_CPUHISTORY, szTemp, 256);
            AppendMenuW(hViewMenu, MF_STRING|MF_POPUP, (UINT_PTR) hSubMenu, szTemp);

            if (TaskManagerSettings.CPUHistory_OneGraphPerCPU)
                CheckMenuRadioItem(hSubMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
            else
                CheckMenuRadioItem(hSubMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);
        }

        LoadStringW(hInst, IDS_MENU_SHOWKERNELTIMES, szTemp, 256);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOWKERNELTIMES, szTemp);

        if (TaskManagerSettings.ShowKernelTimes)
            CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
        else
            CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);

        /*
         * Give the tab control focus
         */
        if (!bWasKeyboardInput)
            SetFocus(hTabWnd);
        break;
    }
}

BOOL ConfirmMessageBox(HWND hWnd, LPCWSTR Text, LPCWSTR Title, UINT Type)
{
    UINT positive = ((Type & 0xF) <= MB_OKCANCEL ? IDOK : IDYES);
    if (GetKeyState(VK_SHIFT) < 0)
        return TRUE;
    return (MessageBoxW(hWnd, Text, Title, Type) == positive);
}

VOID ShowWin32Error(DWORD dwError)
{
    LPWSTR lpMessageBuffer;

    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       dwError,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR)&lpMessageBuffer,
                       0, NULL) != 0)
    {
        MessageBoxW(hMainWnd, lpMessageBuffer, NULL, MB_OK | MB_ICONERROR);
        if (lpMessageBuffer) LocalFree(lpMessageBuffer);
    }
}

LPWSTR GetLastErrorText(LPWSTR lpszBuf, DWORD dwSize)
{
    DWORD  dwRet;
    LPWSTR lpszTemp = NULL;

    dwRet = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPWSTR)&lpszTemp,
                           0,
                           NULL );

    /* supplied buffer is not long enough */
    if (!dwRet || ( (long)dwSize < (long)dwRet+14)) {
        lpszBuf[0] = L'\0';
    } else {
        lpszTemp[lstrlenW(lpszTemp)-2] = L'\0';  /*remove cr and newline character */
        wsprintfW(lpszBuf, L"%s (0x%x)", lpszTemp, (int)GetLastError());
    }
    if (lpszTemp) {
        LocalFree((HLOCAL)lpszTemp);
    }
    return lpszBuf;
}

DWORD EndLocalThread(HANDLE *hThread, DWORD dwThread)
{
    DWORD dwExitCodeThread = 0;

    if (*hThread != NULL) {
        PostThreadMessage(dwThread,WM_QUIT,0,0);
        for (;;) {
            MSG msg;

            if (WAIT_OBJECT_0 == WaitForSingleObject(*hThread, 500))
                break;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        GetExitCodeThread(*hThread, &dwExitCodeThread);
        CloseHandle(*hThread);
        *hThread = NULL;
    }
    return dwExitCodeThread;
}

