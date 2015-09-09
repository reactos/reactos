/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/msconfig.c
 * PURPOSE:     msconfig main dialog
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 *
 */

#include "precomp.h"
#include "utils.h"

#include "toolspage.h"
#include "srvpage.h"
#include "startuppage.h"
#include "freeldrpage.h"
#include "systempage.h"
#include "generalpage.h"

/* Allow only for a single instance of MSConfig */
#ifdef _MSC_VER
    #pragma data_seg("MSConfigInstance")
    HWND hSingleWnd = NULL;
    #pragma data_seg()
    #pragma comment(linker, "/SECTION:MSConfigInstance,RWS")
#else
    HWND hSingleWnd __attribute__((section ("MSConfigInstance"), shared)) = NULL;
#endif

/* Defaults for ReactOS */
BOOL bIsWindows = FALSE;
BOOL bIsOSVersionLessThanVista = TRUE;

HINSTANCE hInst = NULL;
LPWSTR szAppName = NULL;
HWND hMainWnd;                   /* Main Window */

HWND hTabWnd;                    /* Tab Control Window */
// HICON hDialogIcon = NULL;
HICON   hIcon          = NULL;
WNDPROC wpOrigEditProc = NULL;


////////////////////////////////////////////////////////////////////////////////
// Taken from WinSpy++ 1.7
// http://www.catch22.net/software/winspy
// Copyright (c) 2002 by J Brown
//
 
//
//  Copied from uxtheme.h
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
//  Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
    HMODULE hUXTheme;
    ETDTProc fnEnableThemeDialogTexture;

    hUXTheme = LoadLibrary(_T("uxtheme.dll"));

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

/* About Box dialog */
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}


/* Message handler for dialog box. */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        case WM_SYSCOMMAND:
        {
            switch (LOWORD(wParam) /*GET_WM_COMMAND_ID(wParam, lParam)*/)
            {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    // break;
                    return TRUE;
            }

            // break;
            return FALSE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam) /*GET_WM_COMMAND_ID(wParam, lParam)*/)
            {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    // break;
                    return TRUE;
            }

            break;
            // return FALSE;
        }

#if 0
        case WM_SYSCOLORCHANGE:
            /* Forward WM_SYSCOLORCHANGE to common controls */
            SendMessage(hServicesListCtrl, WM_SYSCOLORCHANGE, 0, 0);
            SendMessage(hStartupListCtrl, WM_SYSCOLORCHANGE, 0, 0);
            SendMessage(hToolsListCtrl, WM_SYSCOLORCHANGE, 0, 0);
            break;
#endif

        case WM_DESTROY:
        {
            if (hIcon)
                DestroyIcon(hIcon);

            if (wpOrigEditProc)
                SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)wpOrigEditProc);
        }

        default:
            break;
    }

    /* Return */
    if (wpOrigEditProc)
        return CallWindowProc(wpOrigEditProc, hWnd, uMessage, wParam, lParam);
    else
        return FALSE;
}


#include <pshpack1.h>
typedef struct DLGTEMPLATEEX
{
    WORD dlgVer;
    WORD signature;
    DWORD helpID;
    DWORD exStyle;
    DWORD style;
    WORD cDlgItems;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX;
#include <poppack.h>


VOID ModifySystemMenu(HWND hWnd)
{
    WCHAR szMenuString[255];

    /* Customize the window's system menu, add items before the "Close" item */
    HMENU hSysMenu = GetSystemMenu(hWnd, FALSE);
    assert(hSysMenu);

    /* New entries... */
    if (LoadStringW(hInst,
                    IDS_ABOUT,
                    szMenuString,
                    ARRAYSIZE(szMenuString)) > 0)
    {
        /* "About" menu */
        InsertMenuW(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_ABOUT, szMenuString);
        /* Separator */
        InsertMenuW(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR          , 0 , NULL);
    }

    DrawMenuBar(hWnd);
    return;
}

int CALLBACK PropSheetCallback(HWND hDlg, UINT message, LPARAM lParam)
{
    switch (message)
    {
        case PSCB_PRECREATE:
        {
            LPDLGTEMPLATE   dlgTemplate   =   (LPDLGTEMPLATE)lParam;
            LPDLGTEMPLATEEX dlgTemplateEx = (LPDLGTEMPLATEEX)lParam;

            /* Set the styles of the property sheet dialog */
            if (dlgTemplateEx->signature == 0xFFFF)
            {
                //// MFC : DS_MODALFRAME | DS_3DLOOK | DS_CONTEXTHELP | DS_SETFONT | WS_POPUP | WS_VISIBLE | WS_CAPTION;

                dlgTemplateEx->style   = DS_SHELLFONT | DS_MODALFRAME | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
                // dlgTemplateEx->style   = DS_SHELLFONT | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
                dlgTemplateEx->exStyle = WS_EX_CONTROLPARENT | WS_EX_APPWINDOW;
            }
            else
            {
                dlgTemplate->style           = DS_SHELLFONT | DS_MODALFRAME | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
                // dlgTemplate->style           = DS_SHELLFONT | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
                dlgTemplate->dwExtendedStyle = WS_EX_CONTROLPARENT | WS_EX_APPWINDOW;
            }

            break;
        }

        case PSCB_INITIALIZED:
        {
            /* Modify the system menu of the property sheet dialog */
            ModifySystemMenu(hDlg);

            /* Set the dialog icons */
            hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
            SendMessage(hDlg, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

            /* Sub-class the property sheet window procedure */
            wpOrigEditProc = (WNDPROC)SetWindowLongPtr(hDlg, DWLP_DLGPROC, (LONG_PTR)MainWndProc);

            break;
        }

        default:
            break;
    }

    return FALSE;
}

HWND CreatePropSheet(HINSTANCE hInstance, HWND hwndOwner, LPCTSTR lpszTitle)
{
    HWND hPropSheet;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp[7];
    unsigned int nPages = 0;
    
    /* Header */
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE | PSH_MODELESS | /*PSH_USEICONID |*/ PSH_HASHELP | /*PSH_NOCONTEXTHELP |*/ PSH_USECALLBACK;
    psh.hInstance   = hInstance;
    psh.hwndParent  = hwndOwner;
    //psh.pszIcon     = MAKEINTRESOURCE(IDI_APPICON); // It's crap... Only works for the small icon and not the big...
    psh.pszCaption  = lpszTitle;
    psh.nStartPage  = 0;
    psh.ppsp        = psp;
    psh.pfnCallback = (PFNPROPSHEETCALLBACK)PropSheetCallback;

    /* General page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_GENERAL_PAGE);
    psp[nPages].pfnDlgProc  = (DLGPROC)GeneralPageWndProc;
    ++nPages;

#if 0
    if (bIsWindows && bIsOSVersionLessThanVista)
    {
        /* SYSTEM.INI page */
        if (MyFileExists(lpszSystemIni, NULL))
        {
            psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
            psp[nPages].dwFlags     = PSP_HASHELP | PSP_USETITLE;
            psp[nPages].hInstance   = hInstance;
            psp[nPages].pszTitle    = MAKEINTRESOURCE(IDS_TAB_SYSTEM);
            psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_SYSTEM_PAGE);
            psp[nPages].pfnDlgProc  = (DLGPROC)SystemPageWndProc;
            psp[nPages].lParam      = (LPARAM)lpszSystemIni;
            ++nPages;

            BackupIniFile(lpszSystemIni);
        }

        /* WIN.INI page */
        if (MyFileExists(lpszWinIni, NULL))
        {
            psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
            psp[nPages].dwFlags     = PSP_HASHELP | PSP_USETITLE;
            psp[nPages].hInstance   = hInstance;
            psp[nPages].pszTitle    = MAKEINTRESOURCE(IDS_TAB_WIN);
            psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_SYSTEM_PAGE);
            psp[nPages].pfnDlgProc  = (DLGPROC)WinPageWndProc;
            psp[nPages].lParam      = (LPARAM)lpszWinIni;
            ++nPages;

            BackupIniFile(lpszWinIni);
        }
    }

    /* FreeLdr page */
    // TODO: Program the interface for Vista : "light" BCD editor...
    if (!bIsWindows || (bIsWindows && bIsOSVersionLessThanVista))
    {
        LPCTSTR lpszLoaderIniFile = NULL;
        DWORD   dwTabNameId       = 0;
        if (bIsWindows)
        {
            lpszLoaderIniFile = lpszBootIni;
            dwTabNameId       = IDS_TAB_BOOT;
        }
        else
        {
            lpszLoaderIniFile = lpszFreeLdrIni;
            dwTabNameId       = IDS_TAB_FREELDR;
        }

        if (MyFileExists(lpszLoaderIniFile, NULL))
        {
            psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
            psp[nPages].dwFlags     = PSP_HASHELP | PSP_USETITLE;
            psp[nPages].hInstance   = hInstance;
            psp[nPages].pszTitle    = MAKEINTRESOURCE(dwTabNameId);
            psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_FREELDR_PAGE);
            psp[nPages].pfnDlgProc  = (DLGPROC)FreeLdrPageWndProc;
            psp[nPages].lParam      = (LPARAM)lpszLoaderIniFile;
            ++nPages;

            BackupIniFile(lpszLoaderIniFile);
        }
    }

    /* Services page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_SERVICES_PAGE);
    psp[nPages].pfnDlgProc  = (DLGPROC)ServicesPageWndProc;
    ++nPages;

    /* Startup page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_STARTUP_PAGE);
    psp[nPages].pfnDlgProc  = (DLGPROC)StartupPageWndProc;
    ++nPages;

    /* Tools page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGE);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCE(IDD_TOOLS_PAGE);
    psp[nPages].pfnDlgProc  = (DLGPROC)ToolsPageWndProc;
    ++nPages;
#endif

    /* Set the total number of created pages */
    psh.nPages = nPages;

    /* Create the property sheet */
    hPropSheet = (HWND)PropertySheet(&psh);
    if (hPropSheet)
    {
        /* Center the property sheet on the desktop */
        //ShowWindow(hPropSheet, SW_HIDE);
        ClipOrCenterWindowToMonitor(hPropSheet, MONITOR_WORKAREA | MONITOR_CENTER);
        //ShowWindow(hPropSheet, SW_SHOWNORMAL);
    }

    return hPropSheet;
}

BOOL Initialize(HINSTANCE hInstance)
{
    BOOL Success = TRUE;
    LPWSTR lpszVistaAppName = NULL;
    HANDLE hSemaphore;
    INITCOMMONCONTROLSEX InitControls;

    /* Initialize our global version flags */
    bIsWindows = TRUE; /* IsWindowsOS(); */ // TODO: Commented for testing purposes...
    bIsOSVersionLessThanVista = TRUE; /* IsOSVersionLessThanVista(); */ // TODO: Commented for testing purposes...

    /* Initialize global strings */
    szAppName = LoadResourceString(hInstance, IDS_MSCONFIG);
    if (!bIsOSVersionLessThanVista)
        lpszVistaAppName = LoadResourceString(hInstance, IDS_MSCONFIG_2);

    /* We use a semaphore in order to have a single-instance application */
    hSemaphore = CreateSemaphoreW(NULL, 0, 1, L"MSConfigRunning");
    if (!hSemaphore || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hSemaphore);

        /*
         * A semaphore with the same name already exist. It should have been
         * created by another instance of MSConfig. Try to find its window
         * and bring it to front.
         */
        if ( (hSingleWnd && IsWindow(hSingleWnd))                         ||
             ( (hSingleWnd = FindWindowW(L"#32770", szAppName)) != NULL ) ||
             (!bIsOSVersionLessThanVista ? ( (hSingleWnd = FindWindowW(L"#32770", lpszVistaAppName)) != NULL ) : FALSE) )
        {
            /* Found it. Show the window. */
            ShowWindow(hSingleWnd, SW_SHOWNORMAL);
            SetForegroundWindow(hSingleWnd);
        }

        /* Quit this instance of MSConfig */
        Success = FALSE;
    }
    if (!bIsOSVersionLessThanVista) MemFree(lpszVistaAppName);

    /* Quit now if we failed */
    if (!Success)
    {
        MemFree(szAppName);
        return FALSE;
    }

    /* Initialize the common controls */
    InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
    InitControls.dwICC  = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS /* | ICC_PROGRESS_CLASS | ICC_HOTKEY_CLASS*/;
    InitCommonControlsEx(&InitControls);

    hInst = hInstance;

    return Success;
}

VOID Cleanup(VOID)
{
    MemFree(szAppName);

    // // Close the sentry semaphore.
    // CloseHandle(hSemaphore);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    MSG msg;
    HACCEL hAccelTable;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    /*
     * Initialize this instance of MSConfig. Quit if we have
     * another instance already running.
     */
    if (!Initialize(hInstance))
        return -1;

    // hInst = hInstance;

    hMainWnd = CreatePropSheet(hInstance, NULL, szAppName);
    if (!hMainWnd)
    {
        /* We failed, cleanup and bail out */
        Cleanup();
        return -1;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MSCONFIG));

    /* Message loop */
    while (IsWindow(hMainWnd) && GetMessage(&msg, NULL, 0, 0))
    {
        /*
         * PropSheet_GetCurrentPageHwnd returns NULL when the user clicks the OK or Cancel button
         * and after all of the pages have been notified. Apply button doesn't cause this to happen.
         * We can then use the DestroyWindow function to destroy the property sheet.
         */
        if (PropSheet_GetCurrentPageHwnd(hMainWnd) == NULL)
            break;

        /* Process the accelerator table */
        if (!TranslateAccelerator(hMainWnd, hAccelTable, &msg))
        {
            /*
             * If e.g. an item on the tree view is being edited,
             * we cannot pass the event to PropSheet_IsDialogMessage.
             * Doing so causes the property sheet to be closed at Enter press
             * (instead of completing edit operation).
             */
            if (/*g_bDisableDialogDispatch ||*/ !PropSheet_IsDialogMessage(hMainWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    // FIXME: Process the results of MSConfig !!

    /* Destroy the accelerator table and the window */
    if (hAccelTable != NULL)
        DestroyAcceleratorTable(hAccelTable);

    DestroyWindow(hMainWnd);

    /* Finish cleanup and return */
    Cleanup();
    return (int)msg.wParam;
}

/* EOF */
