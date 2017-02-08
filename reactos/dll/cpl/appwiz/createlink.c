/*
 * PROJECT:         ReactOS Software Control Panel
 * FILE:            dll/cpl/appwiz/createlink.c
 * PURPOSE:         ReactOS Software Control Panel
 * PROGRAMMER:      Gero Kuehn (reactos.filter@gkware.com)
 *                  Dmitry Chapyshev (lentind@yandex.ru)
 *                  Johannes Anderwald
 * UPDATE HISTORY:
 *      06-17-2004  Created
 */

#include "appwiz.h"

#include <tchar.h>

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
    lpExtension = wcsrchr(pContext->szTarget, '.');

    if (IsExtensionAShortcut(lpExtension))
    {
        hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_ALL, &IID_IShellLinkW, (void**)&pSourceShellLink);

        if (hr != S_OK)
            return FALSE;

        hr = pSourceShellLink->lpVtbl->QueryInterface(pSourceShellLink, &IID_IPersistFile, (void**)&pPersistFile);
        if (hr != S_OK)
        {
            pSourceShellLink->lpVtbl->Release(pSourceShellLink);
            return FALSE;
        }

        hr = pPersistFile->lpVtbl->Load(pPersistFile, (LPCOLESTR)pContext->szTarget, STGM_READ);
        pPersistFile->lpVtbl->Release(pPersistFile);

        if (hr != S_OK)
        {
            pSourceShellLink->lpVtbl->Release(pSourceShellLink);
            return FALSE;
        }

        hr = pSourceShellLink->lpVtbl->GetPath(pSourceShellLink, Path, sizeof(Path) / sizeof(WCHAR), NULL, 0);
        pSourceShellLink->lpVtbl->Release(pSourceShellLink);

        if (hr != S_OK)
        {
            return FALSE;
        }
    }
    else
    {
        wcscpy(Path, pContext->szTarget);
    }

    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_ALL,
                   &IID_IShellLinkW, (void**)&pShellLink);

    if (hr != S_OK)
        return FALSE;


    pShellLink->lpVtbl->SetPath(pShellLink, Path);
    pShellLink->lpVtbl->SetDescription(pShellLink, pContext->szDescription);
    pShellLink->lpVtbl->SetWorkingDirectory(pShellLink, pContext->szWorkingDirectory);

    hr = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile, (void**)&pPersistFile);
    if (hr != S_OK)
    {
        pShellLink->lpVtbl->Release(pShellLink);
        return FALSE;
    }

    hr = pPersistFile->lpVtbl->Save(pPersistFile, pContext->szLinkName, TRUE);
    pPersistFile->lpVtbl->Release(pPersistFile);
    pShellLink->lpVtbl->Release(pShellLink);
    return (hr == S_OK);
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
    WCHAR szPath[MAX_PATH];
    WCHAR szDesc[100];
    BROWSEINFOW brws;
    LPITEMIDLIST pidllist;
    IMalloc* malloc;

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
                        SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_LOCATION, WM_SETTEXT, 0, (LPARAM)szPath);

                    /* Free memory, if possible */
                    if (SUCCEEDED(SHGetMalloc(&malloc)))
                    {
                        IMalloc_Free(malloc, pidllist);
                        IMalloc_Release(malloc);
                    }

                    break;
            }
            break;
        case WM_NOTIFY:
            lppsn  = (LPPSHNOTIFY) lParam;
            if (lppsn->hdr.code == PSN_WIZNEXT)
            {
                pContext = (PCREATE_LINK_CONTEXT) GetWindowLongPtr(hwndDlg, DWLP_USER);
                SendDlgItemMessageW(hwndDlg, IDC_SHORTCUT_LOCATION, WM_GETTEXT, MAX_PATH, (LPARAM)pContext->szTarget);
                ///
                /// FIXME
                /// it should also be possible to create a link to folders, shellobjects etc....
                ///
                if (GetFileAttributesW(pContext->szTarget) == INVALID_FILE_ATTRIBUTES)
                {
                    szDesc[0] = L'\0';
                    szPath[0] = L'\0';
                    if (LoadStringW(hApplet, IDS_CREATE_SHORTCUT, szDesc, 100) < 100 &&
                        LoadStringW(hApplet, IDS_ERROR_NOT_FOUND, szPath, MAX_PATH) < MAX_PATH)
                    {
                        WCHAR szError[MAX_PATH + 100];
                        swprintf(szError, szPath, pContext->szTarget);
                        MessageBoxW(hwndDlg, szError, szDesc, MB_ICONERROR);
                    }
                    SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_LOCATION, EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hwndDlg, IDC_SHORTCUT_LOCATION));
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return -1;
                }
                else
                {
                    WCHAR * first, *last;
                    wcscpy(pContext->szWorkingDirectory, pContext->szTarget);
                    first = wcschr(pContext->szWorkingDirectory, L'\\');
                    last = wcsrchr(pContext->szWorkingDirectory, L'\\');
                    wcscpy(pContext->szDescription, &last[1]);

                    if (first != last)
                        last[0] = L'\0';
                    else
                        first[1] = L'\0';

                    first = wcsrchr(pContext->szDescription, L'.');

                    if(first)
                        first[0] = L'\0';
                }

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

    switch(uMsg)
    {
        case WM_INITDIALOG:
            ppsp = (LPPROPSHEETPAGEW)lParam;
            pContext = (PCREATE_LINK_CONTEXT) ppsp->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pContext);
            SendDlgItemMessageW(hwndDlg, IDC_SHORTCUT_NAME, WM_SETTEXT, 0, (LPARAM)pContext->szDescription);
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
            break;
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case EN_CHANGE:
                    if (SendDlgItemMessage(hwndDlg, IDC_SHORTCUT_NAME, WM_GETTEXTLENGTH, 0, 0))
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
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
            if (lppsn->hdr.code == PSN_WIZFINISH)
            {
                SendDlgItemMessageW(hwndDlg, IDC_SHORTCUT_NAME, WM_GETTEXT, MAX_PATH, (LPARAM)pContext->szDescription);
                wcscat(pContext->szLinkName, pContext->szDescription);
                wcscat(pContext->szLinkName, L".lnk");
                if (!CreateShortcut(pContext))
                {
                    MessageBox(hwndDlg, _T("Failed to create shortcut"), _T("Error"), MB_ICONERROR);
                }
            }


    }
    return FALSE;
}

LONG CALLBACK
ShowCreateShortcutWizard(HWND hwndCPl, LPWSTR szPath)
{
    PROPSHEETHEADERW psh;
    HPROPSHEETPAGE ahpsp[2];
    PROPSHEETPAGE psp;
    UINT nPages = 0;
    UINT nLength;
    DWORD attrs;

    PCREATE_LINK_CONTEXT pContext = (PCREATE_LINK_CONTEXT) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CREATE_LINK_CONTEXT));
    if (!pContext)
    {
       /* no memory */
       return FALSE;
    }
    nLength = wcslen(szPath);
    if (!nLength)
    {
        HeapFree(GetProcessHeap(), 0, pContext);

        /* no directory given */
        return FALSE;
    }

    attrs = GetFileAttributesW(szPath);
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        HeapFree(GetProcessHeap(), 0, pContext);

        /* invalid path */
        return FALSE;
    }

    wcscpy(pContext->szLinkName, szPath);
    if (pContext->szLinkName[nLength-1] != L'\\')
    {
        pContext->szLinkName[nLength] = L'\\';
        pContext->szLinkName[nLength+1] = L'\0';
    }


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
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK;
    psh.hInstance = hApplet;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCEW(IDB_SHORTCUT);

    /* Display the wizard */
    PropertySheet(&psh);
    HeapFree(GetProcessHeap(), 0, pContext);
    return TRUE;
}

LONG
CALLBACK
NewLinkHereW(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    return ShowCreateShortcutWizard(hwndCPl, (LPWSTR) lParam1);
}

LONG
CALLBACK
NewLinkHereA(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    WCHAR szFile[MAX_PATH];

    if (MultiByteToWideChar(CP_ACP, 0, (LPSTR) lParam1, -1, szFile, MAX_PATH))
    {
        return ShowCreateShortcutWizard(hwndCPl, szFile);
    }
    return -1;
}
