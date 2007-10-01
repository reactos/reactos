/* 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/advmon.c
 * PURPOSE:         Advanced monitor/display settings
 */

#include "desk.h"

#define MAX_ADVANCED_PAGES  32

static BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_ADVANCED_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}

static BOOL
InitPropSheetPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC DlgProc, LPARAM lParam)
{
    HPROPSHEETPAGE hPage;
    PROPSHEETPAGE psp;

    if (ppsh->nPages < MAX_ADVANCED_PAGES)
    {
        ZeroMemory(&psp, sizeof(psp));
        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hApplet;
        psp.pszTemplate = MAKEINTRESOURCE(idDlg);
        psp.pfnDlgProc = DlgProc;
        psp.lParam = lParam;

        hPage = CreatePropertySheetPage(&psp);
        if (hPage != NULL)
        {
            return PropSheetAddPage(hPage, (LPARAM)ppsh);
        }
    }

    return FALSE;
}

static INT_PTR CALLBACK
AdvGeneralPageProc(HWND hwndDlg,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    PDISPLAY_DEVICE_ENTRY DispDevice = NULL;
    INT_PTR Ret = 0;

    if (uMsg != WM_INITDIALOG)
        DispDevice = (PDISPLAY_DEVICE_ENTRY)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            DispDevice = (PDISPLAY_DEVICE_ENTRY)(((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)DispDevice);

            Ret = TRUE;
            break;
    }

    return Ret;
}

static VOID
BuildAdvPropTitle(IDataObject *pdo, LPTSTR lpBuffer, DWORD dwBufferLen)
{
    UINT uiMonitorName, uiDisplayName;
    LPTSTR lpMonitorName, lpDisplayName;
    TCHAR szFormatBuff[32];

    if (!LoadString(hApplet, IDS_ADVANCEDTITLEFMT, szFormatBuff, sizeof(szFormatBuff) / sizeof(szFormatBuff[0])))
    {
        szFormatBuff[0] = _T('\0');
    }

    uiMonitorName = RegisterClipboardFormat(DESK_EXT_MONITORNAME);
    uiDisplayName = RegisterClipboardFormat(DESK_EXT_DISPLAYNAME);

    lpMonitorName = QueryDeskCplString(pdo, uiMonitorName);
    lpDisplayName = QueryDeskCplString(pdo, uiDisplayName);

    _sntprintf(lpBuffer, dwBufferLen, szFormatBuff, lpMonitorName, lpDisplayName);

    if (lpMonitorName != NULL)
        LocalFree((HLOCAL)lpMonitorName);
    if (lpDisplayName != NULL)
        LocalFree((HLOCAL)lpDisplayName);
}


typedef HPSXA (WINAPI * CPSEAE)(HKEY,LPCWSTR,UINT,IDataObject*);

BOOL
DisplayAdvancedSettings(HWND hWndParent, PDISPLAY_DEVICE_ENTRY DisplayDevice)
{
    TCHAR szCaption[128];
    HPROPSHEETPAGE hpsp[MAX_ADVANCED_PAGES];
    PROPSHEETHEADER psh;
    HPSXA hpsxaDev, hpsxaDisp;
    BOOL Ret;
    IDataObject *pdo;
    HMODULE hShell32 = NULL;
    CPSEAE msvc_SHCreatePropSheetExtArrayEx;

    /* silence gcc warning */
    msvc_SHCreatePropSheetExtArrayEx = NULL;

    /* FIXME: Build the "%s and %s" caption string for the monitor and adapter name */
    szCaption[0] = _T('\0');

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPTITLE;
    psh.hwndParent = hWndParent;
    psh.hInstance = hApplet;
    psh.pszCaption = szCaption;
    psh.phpage = hpsp;

    InitPropSheetPage(&psh, IDD_ADVANCED_GENERAL, AdvGeneralPageProc, (LPARAM)DisplayDevice);

    pdo = CreateDevSettings(DisplayDevice);

    if (pdo != NULL)
        BuildAdvPropTitle(pdo, szCaption, sizeof(szCaption) / sizeof(szCaption[0]));

#ifdef _MSC_VER
    hShell32 = LoadLibrary(_T("shell32.dll"));
    if(hShell32 == NULL)
        return FALSE;
    msvc_SHCreatePropSheetExtArrayEx = (CPSEAE)GetProcAddress(hShell32, (LPCSTR)194);
    hpsxaDev = msvc_SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device"), MAX_ADVANCED_PAGES - psh.nPages, pdo);
#else
    hpsxaDev = SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device"), MAX_ADVANCED_PAGES - psh.nPages, pdo);
#endif
    if (hpsxaDev != NULL)
      SHAddFromPropSheetExtArray(hpsxaDev, PropSheetAddPage, (LPARAM)&psh);

#ifdef _MSC_VER
    hpsxaDisp = msvc_SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device"), MAX_ADVANCED_PAGES - psh.nPages, pdo);
#else
    hpsxaDisp = SHCreatePropSheetExtArrayEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display"), MAX_ADVANCED_PAGES - psh.nPages, pdo);
#endif
    if (hpsxaDisp != NULL)
      SHAddFromPropSheetExtArray(hpsxaDisp, PropSheetAddPage, (LPARAM)&psh);

    Ret = (LONG)(PropertySheet(&psh) != -1);

    if (hpsxaDisp != NULL)
        SHDestroyPropSheetExtArray(hpsxaDisp);

    if (hpsxaDev != NULL)
        SHDestroyPropSheetExtArray(hpsxaDev);

    IDataObject_Release(pdo);

    if (hShell32)
        FreeLibrary(hShell32);

    return Ret;
}
