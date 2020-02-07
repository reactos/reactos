/*
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            dll/cpl/appwiz/createlink.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 *                  Johannes Anderwald
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 * UPDATE HISTORY:
 *      06-17-2004  Created
 */

#include "appwiz.h"
#include <commctrl.h>
#include <shellapi.h>
#include <strsafe.h>

BOOL
IsShortcut(HKEY hKey)
{
    WCHAR Value[10];
    DWORD Size;
    DWORD Type;

    Size = sizeof(Value);
    if (RegQueryValueExW(hKey, L"IsShortcut", NULL, &Type, (LPBYTE)Value, &Size) != ERROR_SUCCESS)
        return FALSE;

    if (Type != REG_SZ)
        return FALSE;

    return (wcsicmp(Value, L"yes") == 0);
}

BOOL
IsExtensionAShortcut(LPWSTR lpExtension)
{
    HKEY hKey;
    WCHAR Buffer[100];
    DWORD Size;
    DWORD Type;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, lpExtension, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (IsShortcut(hKey))
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    Size = sizeof(Buffer);
    if (RegQueryValueEx(hKey, NULL, NULL, &Type, (LPBYTE)Buffer, &Size) != ERROR_SUCCESS || Type != REG_SZ)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, Buffer, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (IsShortcut(hKey))
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    RegCloseKey(hKey);
    return FALSE;
}

BOOL
CreateShortcut(PCREATE_LINK_CONTEXT pContext)
{
    IShellLinkW *pShellLink, *pSourceShellLink;
    IPersistFile *pPersistFile;
    HRESULT hr;
    WCHAR Path[MAX_PATH];
    LPWSTR lpExtension;

    /* get the extension */
    lpExtension = PathFindExtensionW(pContext->szTarget);

    if (IsExtensionAShortcut(lpExtension))
    {
        hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_ALL, &IID_IShellLinkW, (void**)&pSourceShellLink);

        if (FAILED(hr))
            return FALSE;

        hr = IUnknown_QueryInterface(pSourceShellLink, &IID_IPersistFile, (void**)&pPersistFile);
        if (FAILED(hr))
        {
            IUnknown_Release(pSourceShellLink);
            return FALSE;
        }

        hr = pPersistFile->lpVtbl->Load(pPersistFile, (LPCOLESTR)pContext->szTarget, STGM_READ);
        IUnknown_Release(pPersistFile);

        if (FAILED(hr))
        {
            IUnknown_Release(pSourceShellLink);
            return FALSE;
        }

        hr = IShellLinkW_GetPath(pSourceShellLink, Path, _countof(Path), NULL, 0);
        IUnknown_Release(pSourceShellLink);

        if (FAILED(hr))
        {
            return FALSE;
        }
    }
    else
    {
        StringCchCopyW(Path, _countof(Path), pContext->szTarget);
    }

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_ALL,
                          &IID_IShellLinkW, (void**)&pShellLink);

    if (hr != S_OK)
        return FALSE;

    pShellLink->lpVtbl->SetPath(pShellLink, Path);
    pShellLink->lpVtbl->SetDescription(pShellLink, pContext->szDescription);
    pShellLink->lpVtbl->SetWorkingDirectory(pShellLink, pContext->szWorkingDirectory);

    hr = IUnknown_QueryInterface(pShellLink, &IID_IPersistFile, (void**)&pPersistFile);
    if (hr != S_OK)
    {
        IUnknown_Release(pShellLink);
        return FALSE;
    }

    hr = pPersistFile->lpVtbl->Save(pPersistFile, pContext->szLinkName, TRUE);
    IUnknown_Release(pPersistFile);
    IUnknown_Release(pShellLink);
    return (hr == S_OK);
}

BOOL
CreateInternetShortcut(PCREATE_LINK_CONTEXT pContext)
{
    IUniformResourceLocatorW *pURL = NULL;
    IPersistFile *pPersistFile = NULL;
    HRESULT hr;
    WCHAR szPath[MAX_PATH];
    GetFullPathNameW(pContext->szLinkName, _countof(szPath), szPath, NULL);

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL,
                          &IID_IUniformResourceLocatorW, (void **)&pURL);
    if (FAILED(hr))
        return FALSE;

    hr = IUnknown_QueryInterface(pURL, &IID_IPersistFile, (void **)&pPersistFile);
    if (FAILED(hr))
    {
        IUnknown_Release(pURL);
        return FALSE;
    }

    pURL->lpVtbl->SetURL(pURL, pContext->szTarget, 0);

    hr = pPersistFile->lpVtbl->Save(pPersistFile, szPath, TRUE);

    IUnknown_Release(pPersistFile);
    IUnknown_Release(pURL);

    return SUCCEEDED(hr);
}

BOOL IsInternetLocation(LPCWSTR pszLocation)
{
    return (PathIsURLW(pszLocation) || wcsstr(pszLocation, L"www.") == pszLocation);
}

/* Remove all invalid characters from the name */
void
DoConvertNameForFileSystem(LPWSTR szName)
{
    LPWSTR pch1, pch2;
    for (pch1 = pch2 = szName; *pch1; ++pch1)
    {
        if (wcschr(L"\\/:*?\"<>|", *pch1) != NULL)
        {
            /* *pch1 is an invalid character */
            continue;
        }
        *pch2 = *pch1;
        ++pch2;
    }
    *pch2 = 0;
}

BOOL
DoValidateShortcutName(PCREATE_LINK_CONTEXT pContext)
{
    SIZE_T cch;
    LPCWSTR pch, pszName = pContext->szDescription;

    if (!pszName || !pszName[0])
        return FALSE;

    cch = wcslen(pContext->szOrigin) + wcslen(pszName) + 1;
    if (cch >= MAX_PATH)
        return FALSE;

    pch = pszName;
    for (pch = pszName; *pch; ++pch)
    {
        if (wcschr(L"\\/:*?\"<>|", *pch) != NULL)
        {
            /* *pch is an invalid character */
            return FALSE;
        }
    }

    return TRUE;
}

INT_PTR
CALLBACK
WelcomeDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    LPPROPSHEETPAGEW ppsp;
    PCREATE_LINK_CONTEXT pContext;
    LPPSHNOTIFY lppsn;
    WCHAR szPath[MAX_PATH * 2];
    WCHAR szDesc[100];
    BROWSEINFOW brws;
    LPITEMIDLIST pidllist;
    LPWSTR pch;
    SHFILEINFOW FileInfo;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            ppsp = (LPPROPSHEETPAGEW)lParam;
            pContext = (PCREATE_LINK_CONTEXT) ppsp->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
            break;
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case EN_CHANGE:
                    if (SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_LOCATION, WM_GETTEXTLENGTH, 0, 0))
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
                    }
                    break;
            }
            switch(LOWORD(wParam))
            {
                case IDC_SHORTCUT_BROWSE:
                    ZeroMemory(&brws, sizeof(brws));
                    brws.hwndOwner = hwndDlg;
                    brws.pidlRoot = NULL;
                    brws.pszDisplayName = szPath;
                    brws.ulFlags = BIF_BROWSEINCLUDEFILES;
                    brws.lpfn = NULL;
                    pidllist = SHBrowseForFolderW(&brws);
                    if (!pidllist)
                        break;

                    if (SHGetPathFromIDListW(pidllist, szPath))
                        SetDlgItemTextW(hwndDlg, IDC_SHORTCUT_LOCATION, szPath);

                    /* Free memory, if possible */
                    CoTaskMemFree(pidllist);
                    break;
            }
            break;
        case WM_NOTIFY:
            lppsn  = (LPPSHNOTIFY) lParam;
            pContext = (PCREATE_LINK_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (lppsn->hdr.code == PSN_SETACTIVE)
            {
                SetDlgItemTextW(hwndDlg, IDC_SHORTCUT_LOCATION, pContext->szTarget);
            }
            else if (lppsn->hdr.code == PSN_WIZNEXT)
            {
                GetDlgItemTextW(hwndDlg, IDC_SHORTCUT_LOCATION, pContext->szTarget, _countof(pContext->szTarget));
                StrTrimW(pContext->szTarget, L" \t");

                ExpandEnvironmentStringsW(pContext->szTarget, szPath, _countof(szPath));
                StringCchCopyW(pContext->szTarget, _countof(pContext->szTarget), szPath);

                if (IsInternetLocation(pContext->szTarget))
                {
                    /* internet */
                    WCHAR szName[128];
                    LoadStringW(hApplet, IDS_NEW_INTERNET_SHORTCUT, szName, _countof(szName));
                    StringCchCopyW(pContext->szDescription, _countof(pContext->szDescription), szName);

                    pContext->szWorkingDirectory[0] = 0;
                }
                else if (GetFileAttributesW(pContext->szTarget) != INVALID_FILE_ATTRIBUTES)
                {
                    /* file */
                    SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_LOCATION, EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_SHORTCUT_LOCATION));
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);

                    /* get display name */
                    FileInfo.szDisplayName[0] = 0;
                    if (SHGetFileInfoW(pContext->szTarget, 0, &FileInfo, sizeof(FileInfo), SHGFI_DISPLAYNAME))
                        StringCchCopyW(pContext->szDescription, _countof(pContext->szDescription), FileInfo.szDisplayName);

                    /* set working directory */
                    StringCchCopyW(pContext->szWorkingDirectory, _countof(pContext->szWorkingDirectory),
                                   pContext->szTarget);
                    PathRemoveBackslashW(pContext->szWorkingDirectory);
                    pch = PathFindFileNameW(pContext->szWorkingDirectory);
                    if (pch && *pch)
                        *pch = 0;
                    PathRemoveBackslashW(pContext->szWorkingDirectory);
                }
                else
                {
                    /* not found */
                    WCHAR szError[MAX_PATH + 100];

                    SendDlgItemMessageW(hwndDlg, IDC_SHORTCUT_LOCATION, EM_SETSEL, 0, -1);

                    LoadStringW(hApplet, IDS_CREATE_SHORTCUT, szDesc, _countof(szDesc));
                    LoadStringW(hApplet, IDS_ERROR_NOT_FOUND, szPath, _countof(szPath));
                    StringCchPrintfW(szError, _countof(szError), szPath, pContext->szTarget);
                    MessageBoxW(hwndDlg, szError, szDesc, MB_ICONERROR);

                    /* prevent the wizard to go next */
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }
            }
            else if (lppsn->hdr.code == PSN_RESET && !lppsn->lParam)
            {
                /* The user has clicked [Cancel] */
                DeleteFileW(pContext->szOldFile);
                SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, pContext->szOldFile, NULL);
            }
            break;
    }
    return FALSE;
}

INT_PTR
CALLBACK
FinishDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    LPPROPSHEETPAGEW ppsp;
    PCREATE_LINK_CONTEXT pContext;
    LPPSHNOTIFY lppsn;
    LPWSTR pch;
    WCHAR szText[MAX_PATH];
    WCHAR szMessage[128];

    switch(uMsg)
    {
        case WM_INITDIALOG:
            ppsp = (LPPROPSHEETPAGEW)lParam;
            pContext = (PCREATE_LINK_CONTEXT) ppsp->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
            break;
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case EN_CHANGE:
                    if (SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_NAME, WM_GETTEXTLENGTH, 0, 0))
                    {
                        GetDlgItemTextW(hwndDlg, IDC_SHORTCUT_NAME, szText, _countof(szText));
                        StrTrimW(szText, L" \t");
                        if (szText[0])
                            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
                        else
                            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                    }
                    break;
            }
            break;
        case WM_NOTIFY:
            lppsn  = (LPPSHNOTIFY) lParam;
            pContext = (PCREATE_LINK_CONTEXT) GetWindowLongPtr(hwndDlg, DWLP_USER);
            if (lppsn->hdr.code == PSN_SETACTIVE)
            {
                /* TODO: Use shell32!PathCleanupSpec instead of DoConvertNameForFileSystem */
                DoConvertNameForFileSystem(pContext->szDescription);
                SetDlgItemTextW(hwndDlg, IDC_SHORTCUT_NAME, pContext->szDescription);
            }
            else if (lppsn->hdr.code == PSN_WIZFINISH)
            {
                GetDlgItemTextW(hwndDlg, IDC_SHORTCUT_NAME, pContext->szDescription, _countof(pContext->szDescription));
                StrTrimW(pContext->szDescription, L" \t");

                if (!DoValidateShortcutName(pContext))
                {
                    SendDlgItemMessageW(hwndDlg, IDC_SHORTCUT_NAME, EM_SETSEL, 0, -1);

                    LoadStringW(hApplet, IDS_INVALID_NAME, szMessage, _countof(szMessage));
                    MessageBoxW(hwndDlg, szMessage, NULL, MB_ICONERROR);

                    /* prevent the wizard to go next */
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }

                /* if old shortcut file exists, then delete it now */
                DeleteFileW(pContext->szOldFile);
                SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, pContext->szOldFile, NULL);

                if (IsInternetLocation(pContext->szTarget))
                {
                    /* internet */
                    StringCchCopyW(pContext->szLinkName, _countof(pContext->szLinkName),
                                   pContext->szOrigin);
                    PathAppendW(pContext->szLinkName, pContext->szDescription);

                    /* change extension if any */
                    pch = PathFindExtensionW(pContext->szLinkName);
                    if (pch && *pch)
                        *pch = 0;
                    StringCchCatW(pContext->szLinkName, _countof(pContext->szLinkName), L".url");

                    if (!CreateInternetShortcut(pContext))
                    {
                        LoadStringW(hApplet, IDS_CANTMAKEINETSHORTCUT, szMessage, _countof(szMessage));
                        MessageBoxW(hwndDlg, szMessage, NULL, MB_ICONERROR);
                    }
                }
                else
                {
                    /* file */
                    StringCchCopyW(pContext->szLinkName, _countof(pContext->szLinkName),
                                   pContext->szOrigin);
                    PathAppendW(pContext->szLinkName, pContext->szDescription);

                    /* change extension if any */
                    pch = PathFindExtensionW(pContext->szLinkName);
                    if (pch && *pch)
                        *pch = 0;
                    StringCchCatW(pContext->szLinkName, _countof(pContext->szLinkName), L".lnk");

                    if (!CreateShortcut(pContext))
                    {
                        WCHAR szMessage[128];
                        LoadStringW(hApplet, IDS_CANTMAKESHORTCUT, szMessage, _countof(szMessage));
                        MessageBoxW(hwndDlg, szMessage, NULL, MB_ICONERROR);
                    }
                }
            }
            else if (lppsn->hdr.code == PSN_RESET && !lppsn->lParam)
            {
                /* The user has clicked [Cancel] */
                DeleteFileW(pContext->szOldFile);
                SHChangeNotify(SHCNE_DELETE, SHCNF_PATHW, pContext->szOldFile, NULL);
            }
            break;
    }
    return FALSE;
}

static int CALLBACK
PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    // NOTE: This callback is needed to set large icon correctly.
    HICON hIcon;
    switch (uMsg)
    {
        case PSCB_INITIALIZED:
        {
            hIcon = LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_APPINETICO));
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            break;
        }
    }
    return 0;
}

LONG CALLBACK
ShowCreateShortcutWizard(HWND hwndCPl, LPCWSTR szPath)
{
    PROPSHEETHEADERW psh;
    HPROPSHEETPAGE ahpsp[2];
    PROPSHEETPAGE psp;
    UINT nPages = 0;
    UINT nLength;
    PCREATE_LINK_CONTEXT pContext;
    WCHAR szMessage[128];
    LPWSTR pch;

    pContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*pContext));
    if (!pContext)
    {
        /* no memory */
        LoadStringW(hApplet, IDS_NO_MEMORY, szMessage, _countof(szMessage));
        MessageBoxW(hwndCPl, szMessage, NULL, MB_ICONERROR);
        return FALSE;
    }

    nLength = wcslen(szPath);
    if (!nLength)
    {
        HeapFree(GetProcessHeap(), 0, pContext);

        /* no directory given */
        LoadStringW(hApplet, IDS_NO_DIRECTORY, szMessage, _countof(szMessage));
        MessageBoxW(hwndCPl, szMessage, NULL, MB_ICONERROR);
        return FALSE;
    }

    if (!PathFileExistsW(szPath))
    {
        HeapFree(GetProcessHeap(), 0, pContext);

        /* invalid path */
        LoadStringW(hApplet, IDS_INVALID_PATH, szMessage, _countof(szMessage));
        MessageBoxW(hwndCPl, szMessage, NULL, MB_ICONERROR);
        return FALSE;
    }

    /* build the pContext->szOrigin and pContext->szOldFile */
    if (PathIsDirectoryW(szPath))
    {
        StringCchCopyW(pContext->szOrigin, _countof(pContext->szOrigin), szPath);
        pContext->szOldFile[0] = 0;
    }
    else
    {
        StringCchCopyW(pContext->szOrigin, _countof(pContext->szOrigin), szPath);
        pch = PathFindFileNameW(pContext->szOrigin);
        if (pch && *pch)
            *pch = 0;

        StringCchCopyW(pContext->szOldFile, _countof(pContext->szOldFile), szPath);

        pch = PathFindFileNameW(szPath);
        if (pch && *pch)
        {
            /* build szDescription */
            StringCchCopyW(pContext->szDescription, _countof(pContext->szDescription), pch);
            *pch = 0;

            pch = PathFindExtensionW(pContext->szDescription);
            *pch = 0;
        }
    }
    PathAddBackslashW(pContext->szOrigin);

    /* Create the Welcome page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.hInstance = hApplet;
    psp.pfnDlgProc = WelcomeDlgProc;
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_SHORTCUT_LOCATION);
    psp.lParam = (LPARAM)pContext;
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the Finish page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
    psp.pfnDlgProc = FinishDlgProc;
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_SHORTCUT_FINISH);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_USEICONID | PSH_USECALLBACK;
    psh.hInstance = hApplet;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_APPINETICO);
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCEW(IDB_SHORTCUT);
    psh.pfnCallback = PropSheetProc;

    /* Display the wizard */
    PropertySheet(&psh);
    HeapFree(GetProcessHeap(), 0, pContext);
    return TRUE;
}

LONG
CALLBACK
NewLinkHereW(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    InitCommonControls();
    return ShowCreateShortcutWizard(hwndCPl, (LPWSTR)lParam1);
}

LONG
CALLBACK
NewLinkHereA(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, (LPSTR)lParam1, -1, szFile, _countof(szFile)))
    {
        InitCommonControls();
        return ShowCreateShortcutWizard(hwndCPl, szFile);
    }
    return -1;
}
