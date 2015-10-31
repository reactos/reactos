/*
 * PROJECT:     ReactOS Applications
 * LICENSE:     LGPL - See COPYING in the top level directory
 * FILE:        base/applications/msconfig_new/msconfig.c
 * PURPOSE:     MSConfig main dialog
 * COPYRIGHT:   Copyright 2005-2006 Christoph von Wittich <Christoph@ApiViewer.de>
 */

#include "precomp.h"
#include "fileutils.h"
#include "utils.h"

#include "generalpage.h"
#include "systempage.h"
#include "freeldrpage.h"
#include "srvpage.h"
// #include "startuppage.h"
#include "toolspage.h"

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

/* Language-independent Vendor strings */
const LPCWSTR IDS_REACTOS   = L"ReactOS";
const LPCWSTR IDS_MICROSOFT = L"Microsoft";
const LPCWSTR IDS_WINDOWS   = L"Windows";

HINSTANCE hInst = NULL;
LPWSTR szAppName = NULL;
HWND hMainWnd;                   /* Main Window */

HWND  hTabWnd;                   /* Tab Control Window */
HICON hIcon = NULL, hIconSm = NULL;
WNDPROC wpOrigEditProc = NULL;


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


/* Message handler for dialog box */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        case WM_SYSCOMMAND:
        {
            switch (LOWORD(wParam) /*GET_WM_COMMAND_ID(wParam, lParam)*/)
            {
                case IDM_ABOUT:
                    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, About);
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
                    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, About);
                    // break;
                    return TRUE;
            }

            break;
            // return FALSE;
        }

        case WM_DESTROY:
        {
            if (wpOrigEditProc)
                SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)wpOrigEditProc);

            if (hIcon)   DestroyIcon(hIcon);
            if (hIconSm) DestroyIcon(hIconSm);
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

int CALLBACK PropSheetCallback(HWND hDlg, UINT message, LPARAM lParam)
{
    switch (message)
    {
        case PSCB_PRECREATE:
        {
            LPDLGTEMPLATE   dlgTemplate   =   (LPDLGTEMPLATE)lParam;
            LPDLGTEMPLATEEX dlgTemplateEx = (LPDLGTEMPLATEEX)lParam;

            // MFC : DS_MODALFRAME | DS_3DLOOK | DS_CONTEXTHELP | DS_SETFONT | WS_POPUP | WS_VISIBLE | WS_CAPTION;
            DWORD style   = DS_SHELLFONT | DS_MODALFRAME | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU;
                         // DS_SHELLFONT | DS_CENTER | WS_MINIMIZEBOX | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
            DWORD exStyle = WS_EX_CONTROLPARENT | WS_EX_APPWINDOW;

            /* Hide the dialog by default; we will center it on screen later, and then show it */
            style &= ~WS_VISIBLE;

            /* Set the styles of the property sheet dialog */
            if (dlgTemplateEx->signature == 0xFFFF)
            {
                dlgTemplateEx->style   = style;
                dlgTemplateEx->exStyle = exStyle;
            }
            else
            {
                dlgTemplate->style           = style;
                dlgTemplate->dwExtendedStyle = exStyle;
            }

            break;
        }

        case PSCB_INITIALIZED:
        {
            /* Customize the window's system menu, add items before the "Close" item */
            LPWSTR szMenuString;
            HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
            assert(hSysMenu);

            szMenuString = LoadResourceString(hInst, IDS_ABOUT);
            if (szMenuString)
            {
                /* "About" menu */
                InsertMenuW(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_ABOUT, szMenuString);
                /* Separator */
                InsertMenuW(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR          , 0 , NULL);

                MemFree(szMenuString);
            }
            DrawMenuBar(hDlg);

            /* Set the dialog icons */
            hIcon   = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
            hIconSm = (HICON)CopyImage(hIcon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_COPYFROMRESOURCE);
            SendMessage(hDlg, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

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
    PROPSHEETHEADERW psh;
    PROPSHEETPAGEW   psp[7];
    unsigned int nPages = 0;

    /* Header */
    psh.dwSize      = sizeof(psh);
    psh.dwFlags     = PSH_PROPSHEETPAGE | PSH_MODELESS | /*PSH_USEICONID |*/ PSH_HASHELP | /*PSH_NOCONTEXTHELP |*/ PSH_USECALLBACK;
    psh.hInstance   = hInstance;
    psh.hwndParent  = hwndOwner;
    // psh.pszIcon     = MAKEINTRESOURCEW(IDI_APPICON); // Disabled because it only sets the small icon; the big icon is a stretched version of the small one.
    psh.pszCaption  = lpszTitle;
    psh.nStartPage  = 0;
    psh.ppsp        = psp;
    psh.pfnCallback = PropSheetCallback;

    /* General page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL_PAGE);
    psp[nPages].pfnDlgProc  = GeneralPageWndProc;
    ++nPages;

    // if (bIsOSVersionLessThanVista)
    {
        /* SYSTEM.INI page */
        if (MyFileExists(lpszSystemIni, NULL))
        {
            psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
            psp[nPages].dwFlags     = PSP_HASHELP | PSP_USETITLE;
            psp[nPages].hInstance   = hInstance;
            psp[nPages].pszTitle    = MAKEINTRESOURCEW(IDS_TAB_SYSTEM);
            psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_SYSTEM_PAGE);
            psp[nPages].pfnDlgProc  = SystemPageWndProc;
            psp[nPages].lParam      = (LPARAM)lpszSystemIni;
            ++nPages;

            BackupIniFile(lpszSystemIni);
        }

        /* WIN.INI page */
        if (MyFileExists(lpszWinIni, NULL))
        {
            psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
            psp[nPages].dwFlags     = PSP_HASHELP | PSP_USETITLE;
            psp[nPages].hInstance   = hInstance;
            psp[nPages].pszTitle    = MAKEINTRESOURCEW(IDS_TAB_WIN);
            psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_SYSTEM_PAGE);
            psp[nPages].pfnDlgProc  = WinPageWndProc;
            psp[nPages].lParam      = (LPARAM)lpszWinIni;
            ++nPages;

            BackupIniFile(lpszWinIni);
        }
    }

    /* FreeLdr page */
    // TODO: Program the interface for Vista: "light" BCD editor...
    if (!bIsWindows || (bIsWindows && bIsOSVersionLessThanVista))
    {
        LPCWSTR lpszLoaderIniFile = NULL;
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
            psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
            psp[nPages].dwFlags     = PSP_HASHELP | PSP_USETITLE;
            psp[nPages].hInstance   = hInstance;
            psp[nPages].pszTitle    = MAKEINTRESOURCEW(dwTabNameId);
            psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_FREELDR_PAGE);
            psp[nPages].pfnDlgProc  = FreeLdrPageWndProc;
            psp[nPages].lParam      = (LPARAM)lpszLoaderIniFile;
            ++nPages;

            BackupIniFile(lpszLoaderIniFile);
        }
    }

    /* Services page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_SERVICES_PAGE);
    psp[nPages].pfnDlgProc  = ServicesPageWndProc;
    ++nPages;

#if 0
    /* Startup page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_STARTUP_PAGE);
    psp[nPages].pfnDlgProc  = StartupPageWndProc;
    ++nPages;
#endif

    /* Tools page */
    psp[nPages].dwSize      = sizeof(PROPSHEETPAGEW);
    psp[nPages].dwFlags     = PSP_HASHELP;
    psp[nPages].hInstance   = hInstance;
    psp[nPages].pszTemplate = MAKEINTRESOURCEW(IDD_TOOLS_PAGE);
    psp[nPages].pfnDlgProc  = ToolsPageWndProc;
    ++nPages;

    /* Set the total number of created pages */
    psh.nPages = nPages;

    /* Create the property sheet */
    hPropSheet = (HWND)PropertySheetW(&psh);
    if (hPropSheet)
    {
        /* Center the property sheet on the desktop and show it */
        ClipOrCenterWindowToMonitor(hPropSheet, MONITOR_WORKAREA | MONITOR_CENTER);
        ShowWindow(hPropSheet, SW_SHOWNORMAL);
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

    hAccelTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_MSCONFIG));

    /* Message loop */
    while (IsWindow(hMainWnd) && GetMessageW(&msg, NULL, 0, 0))
    {
        /*
         * PropSheet_GetCurrentPageHwnd returns NULL when the user clicks the OK or Cancel button
         * and after all of the pages have been notified. Apply button doesn't cause this to happen.
         * We can then use the DestroyWindow function to destroy the property sheet.
         */
        if (PropSheet_GetCurrentPageHwnd(hMainWnd) == NULL)
            break;

        /* Process the accelerator table */
        if (!TranslateAcceleratorW(hMainWnd, hAccelTable, &msg))
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
                DispatchMessageW(&msg);
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
