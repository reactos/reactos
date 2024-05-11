/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/desk.c
 * PURPOSE:         ReactOS Display Control Panel
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "desk.h"

#include <shellapi.h>
#include <cplext.h>
#include <winnls.h>

#define NDEBUG
#include <debug.h>


/* Enable this for InstallScreenSaverW() to determine a possible full path
 * to the specified screensaver file, verify its existence and use it.
 * (NOTE: This is not Windows desk.cpl-compatible.) */
// #define CHECK_SCR_FULL_PATH


VOID WINAPI Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);

static LONG APIENTRY DisplayApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ThemesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK BackgroundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ScreenSaverPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
UINT CALLBACK SettingsPageCallbackProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

HINSTANCE hApplet = NULL;
HWND hCPLWindow = NULL;

/* Applets */
APPLET Applets[] =
{
    {
        IDC_DESK_ICON,
        IDS_CPLNAME,
        IDS_CPLDESCRIPTION,
        DisplayApplet
    }
};

HMENU
LoadPopupMenu(IN HINSTANCE hInstance,
              IN LPCTSTR lpMenuName)
{
    HMENU hMenu, hSubMenu = NULL;

    hMenu = LoadMenu(hInstance,
                     lpMenuName);

    if (hMenu != NULL)
    {
        hSubMenu = GetSubMenu(hMenu,
                              0);
        if (hSubMenu != NULL &&
            !RemoveMenu(hMenu,
                        0,
                        MF_BYPOSITION))
        {
            hSubMenu = NULL;
        }

        DestroyMenu(hMenu);
    }

    return hSubMenu;
}

static BOOL CALLBACK
DisplayAppletPropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_DESK_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}

static BOOL
InitPropSheetPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC DlgProc, LPFNPSPCALLBACK pfnCallback)
{
    HPROPSHEETPAGE hPage;
    PROPSHEETPAGE psp;

    if (ppsh->nPages < MAX_DESK_PAGES)
    {
        ZeroMemory(&psp, sizeof(psp));
        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        if (pfnCallback != NULL)
            psp.dwFlags |= PSP_USECALLBACK;
        psp.hInstance = hApplet;
        psp.pszTemplate = MAKEINTRESOURCE(idDlg);
        psp.pfnDlgProc = DlgProc;
        psp.pfnCallback = pfnCallback;

        hPage = CreatePropertySheetPage(&psp);
        if (hPage != NULL)
        {
            return DisplayAppletPropSheetAddPage(hPage, (LPARAM)ppsh);
        }
    }

    return FALSE;
}

static const struct
{
    WORD idDlg;
    DLGPROC DlgProc;
    LPFNPSPCALLBACK Callback;
    LPWSTR Name;
} PropPages[] =
{
    /* { IDD_THEMES, ThemesPageProc, NULL, L"Themes" }, */ /* TODO: */
    { IDD_BACKGROUND, BackgroundPageProc, NULL, L"Desktop" },
    { IDD_SCREENSAVER, ScreenSaverPageProc, NULL, L"Screen Saver" },
    { IDD_APPEARANCE, AppearancePageProc, NULL, L"Appearance" },
    { IDD_SETTINGS, SettingsPageProc, SettingsPageCallbackProc, L"Settings" },
};

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDC_DESK_ICON));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

/* Display Applet */
static LONG APIENTRY
DisplayApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam)
{
    HPROPSHEETPAGE hpsp[MAX_DESK_PAGES];
    PROPSHEETHEADER psh;
    HPSXA hpsxa = NULL;
    UINT i;
    LPWSTR *argv = NULL;
    LPCWSTR pwszSelectedTab = NULL;
    LPCWSTR pwszFile = NULL;
    LPCWSTR pwszAction = NULL;
    INT nPage = 0;
    BITMAP bitmap;

    UNREFERENCED_PARAMETER(wParam);

    hCPLWindow = hwnd;

    if (uMsg == CPL_STARTWPARMSW && lParam)
    {
        int argc;
        int i;

        nPage = _wtoi((PWSTR)lParam);

#if 0
        argv = CommandLineToArgvW((LPCWSTR)lParam, &argc);
#else
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);
#endif

        if (argv && argc)
        {
            for (i = 0; i<argc; i++)
            {
#if 0
                if (argv[i][0] == L'@')
                    pwszSelectedTab = &argv[i][1];
#else
                if (wcsncmp(argv[i], L"desk,@", 6) == 0)
                    pwszSelectedTab = &argv[i][6];
#endif
                else if (wcsncmp(argv[i], L"/Action:", 8) == 0)
                    pwszAction = &argv[i][8];
                else if (wcsncmp(argv[i], L"/file:", 6) == 0)
                    pwszFile = &argv[i][6];
            }
        }
    }

    if(pwszAction && wcsncmp(pwszAction, L"ActivateMSTheme", 15) == 0)
    {
        ActivateThemeFile(pwszFile);
        goto cleanup;
    }

    g_GlobalData.pwszFile = pwszFile;
    g_GlobalData.pwszAction = pwszAction;
    g_GlobalData.desktop_color = GetSysColor(COLOR_DESKTOP);

    /* Initialize the monitor preview bitmap, used on multiple pages */
    g_GlobalData.hMonitorBitmap = LoadBitmapW(hApplet, MAKEINTRESOURCEW(IDC_MONITOR));
    if (g_GlobalData.hMonitorBitmap != NULL)
    {
        GetObjectW(g_GlobalData.hMonitorBitmap, sizeof(bitmap), &bitmap);

        g_GlobalData.bmMonWidth = bitmap.bmWidth;
        g_GlobalData.bmMonHeight = bitmap.bmHeight;
    }

    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_USECALLBACK | PSH_PROPTITLE | PSH_USEICONID;
    psh.hwndParent = hCPLWindow;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDC_DESK_ICON);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPLNAME);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    /* Allow shell extensions to replace the background page */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Desk"), MAX_DESK_PAGES - psh.nPages);

    for (i = 0; i < _countof(PropPages); i++)
    {
        if (pwszSelectedTab && _wcsicmp(pwszSelectedTab, PropPages[i].Name) == 0)
            psh.nStartPage = i;

        /* Override the background page if requested by a shell extension */
        if (PropPages[i].idDlg == IDD_BACKGROUND && hpsxa != NULL &&
            SHReplaceFromPropSheetExtArray(hpsxa, CPLPAGE_DISPLAY_BACKGROUND, DisplayAppletPropSheetAddPage, (LPARAM)&psh) != 0)
        {
            /* The shell extension added one or more pages to replace the background page.
               Don't create the built-in page anymore! */
            continue;
        }

        InitPropSheetPage(&psh, PropPages[i].idDlg, PropPages[i].DlgProc, PropPages[i].Callback);
    }

    /* NOTE: Don't call SHAddFromPropSheetExtArray here because this applet only allows
             replacing the background page but not extending the applet by more pages */

    if (nPage != 0 && psh.nStartPage == 0)
        psh.nStartPage = nPage;

    PropertySheet(&psh);

cleanup:
    DeleteObject(g_GlobalData.hMonitorBitmap);

    if (hpsxa != NULL)
        SHDestroyPropSheetExtArray(hpsxa);

    if (argv)
        LocalFree(argv);

    return TRUE;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    UINT i = (UINT)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return _countof(Applets);

        case CPL_INQUIRE:
            if (i < _countof(Applets))
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            else
            {
                return TRUE;
            }
            break;

        case CPL_DBLCLK:
            if (i < _countof(Applets))
                Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;

        case CPL_STARTWPARMSW:
            if (i < _countof(Applets))
                return Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;
    }

    return FALSE;
}

void
WINAPI
InstallScreenSaverW(
    IN HWND hWindow,
    IN HANDLE hInstance,
    IN LPCWSTR pszFile,
    IN UINT nCmdShow)
{
    LRESULT rc;
    HKEY regKey;
    INT Timeout;
#ifdef CHECK_SCR_FULL_PATH
    HANDLE hFile;
    WIN32_FIND_DATAW fdFile;
#endif
    DWORD dwLen;
    WCHAR szFullPath[MAX_PATH];

    if (!pszFile)
    {
        DPRINT1("InstallScreenSaver() null file\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }
    DPRINT("InstallScreenSaver() Installing screensaver %ls\n", pszFile);

#ifdef CHECK_SCR_FULL_PATH
    /* Retrieve the actual path to the file and verify whether it exists */
    dwLen = GetFullPathNameW(pszFile, _countof(szFullPath), szFullPath, NULL);
    if (dwLen == 0 || dwLen > _countof(szFullPath))
    {
        DPRINT1("InstallScreenSaver() File %ls not accessible\n", pszFile);
        return;
    }
    hFile = FindFirstFile(szFullPath, &fdFile);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DPRINT1("InstallScreenSaver() File %ls not found\n", pszFile);
        return;
    }
    FindClose(hFile);
    /* Use the full file path from now on */
    pszFile = szFullPath;
#endif

    rc = RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"Control Panel\\Desktop",
                       0,
                       KEY_SET_VALUE,
                       &regKey);
    if (rc == ERROR_SUCCESS)
    {
        /* Set the screensaver */
        SIZE_T Length = (wcslen(pszFile) + 1) * sizeof(WCHAR);
        rc = RegSetValueExW(regKey,
                            L"SCRNSAVE.EXE",
                            0,
                            REG_SZ,
                            (PBYTE)pszFile,
                            (DWORD)Length);
        RegCloseKey(regKey);
    }
    if (rc != ERROR_SUCCESS)
    {
        DPRINT1("InstallScreenSaver() Could not change the current screensaver\n");
        return;
    }

    SystemParametersInfoW(SPI_SETSCREENSAVEACTIVE, TRUE, 0, SPIF_UPDATEINIFILE);

    /* If no screensaver timeout is present, default to 10 minutes (600 seconds) */
    Timeout = 0;
    if (!SystemParametersInfoW(SPI_GETSCREENSAVETIMEOUT, 0, &Timeout, 0) || (Timeout <= 0))
        SystemParametersInfoW(SPI_SETSCREENSAVETIMEOUT, 600, 0, SPIF_UPDATEINIFILE);

    /* Retrieve the name of this current instance of desk.cpl */
    dwLen = GetModuleFileNameW(hApplet, szFullPath, _countof(szFullPath));
    if ((dwLen == 0) || (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        /* We failed, copy the default value */
        StringCchCopyW(szFullPath, _countof(szFullPath), L"desk.cpl");
    }

    /* Build the desk.cpl command-line to start the ScreenSaver page.
     * Equivalent to: "desk.cpl,ScreenSaver,@ScreenSaver" */
    rc = StringCchCatW(szFullPath, _countof(szFullPath), L",,1");
    if (FAILED(rc))
        return;

    /* Open the ScreenSaver page in this desk.cpl instance */
    DPRINT("InstallScreenSaver() Starting '%ls'\n", szFullPath);
    Control_RunDLLW(hWindow, hInstance, szFullPath, nCmdShow);
}

void
WINAPI
InstallScreenSaverA(
    IN HWND hWindow,
    IN HANDLE hInstance,
    IN LPCSTR pszFile,
    IN UINT nCmdShow)
{
    LPWSTR lpwString;
    int nLength;

    if (!pszFile)
    {
        DPRINT1("InstallScreenSaver() null file\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    /* Convert the string to unicode */
    lpwString = NULL;
    nLength = MultiByteToWideChar(CP_ACP, 0, pszFile, -1, NULL, 0);
    if (nLength != 0)
    {
        lpwString = LocalAlloc(LMEM_FIXED, nLength * sizeof(WCHAR));
        if (lpwString)
        {
            if (!MultiByteToWideChar(CP_ACP, 0, pszFile, -1, lpwString, nLength))
            {
                LocalFree(lpwString);
                lpwString = NULL;
            }
        }
    }
    if (!lpwString)
    {
        DPRINT1("InstallScreenSaver() not enough memory to convert string to unicode\n");
        return;
    }

    /* Call the unicode function */
    InstallScreenSaverW(hWindow, hInstance, lpwString, nCmdShow);

    LocalFree(lpwString);
}

BOOL WINAPI
DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            RegisterPreviewControl(hInstDLL);
//        case DLL_THREAD_ATTACH:
            hApplet = hInstDLL;
            break;

        case DLL_PROCESS_DETACH:
            UnregisterPreviewControl(hInstDLL);
            CoUninitialize();
            break;
    }

    return TRUE;
}
