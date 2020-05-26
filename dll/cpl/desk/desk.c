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
#include <debug.h>

#define NUM_APPLETS    (1)

static LONG APIENTRY DisplayApplet(HWND hwnd, UINT uMsg, LPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK ThemesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK BackgroundPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ScreenSaverPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AppearancePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
UINT CALLBACK SettingsPageCallbackProc(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

HINSTANCE hApplet = 0;
HWND hCPLWindow;

/* Applets */
APPLET Applets[NUM_APPLETS] =
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
    TCHAR Caption[1024];
    UINT i;
    LPWSTR *argv = NULL;
    LPCWSTR pwszSelectedTab = NULL;
    LPCWSTR pwszFile = NULL;
    LPCWSTR pwszAction = NULL;
    INT nPage = 0;

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

    LoadString(hApplet, IDS_CPLNAME, Caption, sizeof(Caption) / sizeof(TCHAR));

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USECALLBACK | PSH_PROPTITLE | PSH_USEICONID;
    psh.hwndParent = hCPLWindow;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDC_DESK_ICON);
    psh.pszCaption = Caption;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    /* Allow shell extensions to replace the background page */
    hpsxa = SHCreatePropSheetExtArray(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Desk"), MAX_DESK_PAGES - psh.nPages);

    for (i = 0; i != sizeof(PropPages) / sizeof(PropPages[0]); i++)
    {
        if (pwszSelectedTab && wcsicmp(pwszSelectedTab, PropPages[i].Name) == 0)
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
    int i = (int)lParam1;

    switch (uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            break;

        case CPL_DBLCLK:
            Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;

        case CPL_STARTWPARMSW:
            return Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
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
    WCHAR pszSystemDir[MAX_PATH];
    WCHAR pszDrive[3];
    WCHAR pszPath[MAX_PATH];
    WCHAR pszFilename[MAX_PATH];
    WCHAR pszExt[MAX_PATH];
    LPWSTR pszOutName;
    UINT uCompressionType=FILE_COMPRESSION_NONE;
    DWORD dwSourceSize;
    DWORD dwTargetSize;
    DWORD rc;

    if (!pszFile)
    {
        DPRINT("InstallScreenSaver() null file\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }
    DPRINT("InstallScreenSaver() Installing screensaver %ls\n", pszFile);

    rc = SetupGetFileCompressionInfoW(pszFile, &pszOutName, &dwSourceSize, &dwTargetSize, &uCompressionType);
    if (ERROR_SUCCESS != rc)
    {
        DPRINT("InstallScreenSaver() SetupGetFileCompressionInfo failed with error 0x%lx\n", rc);
        SetLastError(rc);
        return;
    }
    if (!GetSystemDirectoryW((LPWSTR)pszSystemDir, sizeof(pszSystemDir)/sizeof(WCHAR)))
    {
        MyFree(pszOutName);
        DPRINT("InstallScreenSaver() GetSystemDirectory failed with error 0x%lx\n", GetLastError());
        return;
    }
    _wsplitpath(pszOutName, pszDrive, pszPath, pszFilename, pszExt);
    MyFree(pszOutName);
    StringCbCatW(pszSystemDir, sizeof(pszSystemDir), L"\\");
    StringCbCatW(pszSystemDir, sizeof(pszSystemDir), pszFilename);
    StringCbCatW(pszSystemDir, sizeof(pszSystemDir), pszExt);
    rc = SetupDecompressOrCopyFileW(pszFile, pszSystemDir, &uCompressionType);
    DPRINT("InstallScreenSaver() Copying to %ls, compression type %d return 0x%lx\n", pszFile, uCompressionType, rc);
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

    if (!pszFile)
    {
        DPRINT("InstallScreenSaver() null file\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }
    DPRINT("InstallScreenSaver() Install from file %s\n", pszFile);
    lpwString = pSetupMultiByteToUnicode(pszFile, 0);
    if (!lpwString)
    {
        DPRINT("InstallScreenSaver() not enough memory to convert string to unicode\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }
    InstallScreenSaverW(hWindow, hInstance, lpwString, nCmdShow);
    MyFree(lpwString);
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
