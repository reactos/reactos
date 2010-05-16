/*
 * Regedit find dialog
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


#include <regedit.h>

static TCHAR s_szFindWhat[256];
static const TCHAR s_szFindFlags[] = _T("FindFlags");
static const TCHAR s_szFindFlagsR[] = _T("FindFlagsReactOS");
static HWND s_hwndAbortDialog;
static BOOL s_bAbort;

static DWORD s_dwFlags;
static TCHAR s_szName[MAX_PATH];
static DWORD s_cbName;
static const TCHAR s_empty[] = {0};
static const TCHAR s_backslash[] = {'\\', 0};

extern VOID SetValueName(HWND hwndLV, LPCTSTR pszValueName);

BOOL DoEvents(VOID)
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            s_bAbort = TRUE;
        if (!IsDialogMessage(s_hwndAbortDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return s_bAbort;
}

static LPTSTR lstrstri(LPCTSTR psz1, LPCTSTR psz2)
{
    INT i, cch1, cch2;

    cch1 = lstrlen(psz1);
    cch2 = lstrlen(psz2);
    for(i = 0; i <= cch1 - cch2; i++)
    {
        if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                          psz1 + i, cch2, psz2, cch2) == 2)
            return (LPTSTR) (psz1 + i);
    }
    return NULL;
}

static BOOL CompareName(LPCTSTR pszName1, LPCTSTR pszName2)
{
    if (s_dwFlags & RSF_WHOLESTRING)
    {
        if (s_dwFlags & RSF_MATCHCASE)
            return lstrcmp(pszName1, pszName2) == 0;
        else
            return lstrcmpi(pszName1, pszName2) == 0;
    }
    else
    {
        if (s_dwFlags & RSF_MATCHCASE)
            return _tcsstr(pszName1, pszName2) != NULL;
        else
            return lstrstri(pszName1, pszName2) != NULL;
    }
}

static BOOL
CompareData(
    DWORD   dwType,
    LPCTSTR psz1,
    LPCTSTR psz2)
{
    INT i, cch1 = lstrlen(psz1), cch2 = lstrlen(psz2);
    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
    {
        if (s_dwFlags & RSF_WHOLESTRING)
        {
            if (s_dwFlags & RSF_MATCHCASE)
                return 2 == CompareString(LOCALE_SYSTEM_DEFAULT, 0,
                    psz1, cch1, psz2, cch2);
            else
                return 2 == CompareString(LOCALE_SYSTEM_DEFAULT, 
                    NORM_IGNORECASE, psz1, cch1, psz2, cch2);
        }

        for(i = 0; i <= cch1 - cch2; i++)
        {
            if (s_dwFlags & RSF_MATCHCASE)
            {
                if (2 == CompareString(LOCALE_SYSTEM_DEFAULT, 0,
                    psz1 + i, cch2, psz2, cch2))
                    return TRUE;
            }
            else
            {
                if (2 == CompareString(LOCALE_SYSTEM_DEFAULT,
                    NORM_IGNORECASE, psz1 + i, cch2, psz2, cch2))
                    return TRUE;
            }
        }
    }
    return FALSE;
}

int compare(const void *x, const void *y)
{
    const LPCTSTR *a = (const LPCTSTR *)x;
    const LPCTSTR *b = (const LPCTSTR *)y;
    return lstrcmpi(*a, *b);
}

BOOL RegFindRecurse(
    HKEY    hKey, 
    LPCTSTR pszSubKey,
    LPCTSTR pszValueName,
    LPTSTR *ppszFoundSubKey,
    LPTSTR *ppszFoundValueName)
{
    HKEY hSubKey;
    LONG lResult;
    TCHAR szSubKey[MAX_PATH];
    DWORD i, c, cb, type;
    BOOL fPast = FALSE;
    LPTSTR *ppszNames = NULL;
    LPBYTE pb = NULL;

    if (DoEvents())
        return FALSE;

    lstrcpy(szSubKey, pszSubKey);
    hSubKey = NULL;

    lResult = RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
    if (lResult != ERROR_SUCCESS)
        return FALSE;

    if (pszValueName == NULL)
        pszValueName = s_empty;

    lResult = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, NULL, NULL, NULL,
        &c, NULL, NULL, NULL, NULL);
    if (lResult != ERROR_SUCCESS)
        goto err;
    ppszNames = (LPTSTR *) malloc(c * sizeof(LPTSTR));
    if (ppszNames == NULL)
        goto err;
    ZeroMemory(ppszNames, c * sizeof(LPTSTR));

    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        s_cbName = MAX_PATH * sizeof(TCHAR);
        lResult = RegEnumValue(hSubKey, i, s_szName, &s_cbName, NULL, NULL, 
                               NULL, &cb);
        if (lResult == ERROR_NO_MORE_ITEMS)
        {
            c = i;
            break;
        }
        if (lResult != ERROR_SUCCESS)
            goto err;
        if (s_cbName >= MAX_PATH * sizeof(TCHAR))
            continue;

        ppszNames[i] = _tcsdup(s_szName);
    }

    qsort(ppszNames, c, sizeof(LPTSTR), compare);

    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        if (!fPast && lstrcmpi(ppszNames[i], pszValueName) == 0)
        {
            fPast = TRUE;
            continue;
        }
        if (!fPast)
            continue;

        if ((s_dwFlags & RSF_LOOKATVALUES) && 
            CompareName(ppszNames[i], s_szFindWhat))
        {
            *ppszFoundSubKey = _tcsdup(szSubKey);
            if (ppszNames[i][0] == 0)
                *ppszFoundValueName = NULL;
            else
                *ppszFoundValueName = _tcsdup(ppszNames[i]);
            goto success;
        }

        lResult = RegQueryValueEx(hSubKey, ppszNames[i], NULL, &type,
            NULL, &cb);
        if (lResult != ERROR_SUCCESS)
            goto err;
        pb = malloc(cb);
        if (pb == NULL)
            goto err;
        lResult = RegQueryValueEx(hSubKey, ppszNames[i], NULL, &type,
            pb, &cb);
        if (lResult != ERROR_SUCCESS)
            goto err;

        if ((s_dwFlags & RSF_LOOKATDATA) && 
            CompareData(type, (LPTSTR) pb, s_szFindWhat))
        {
            *ppszFoundSubKey = _tcsdup(szSubKey);
            if (ppszNames[i][0] == 0)
                *ppszFoundValueName = NULL;
            else
                *ppszFoundValueName = _tcsdup(ppszNames[i]);
            goto success;
        }
        free(pb);
        pb = NULL;
    }

    if (ppszNames != NULL)
    {
        for(i = 0; i < c; i++)
            free(ppszNames[i]);
        free(ppszNames);
    }
    ppszNames = NULL;

    lResult = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, &c, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL);
    if (lResult != ERROR_SUCCESS)
        goto err;
    ppszNames = (LPTSTR *) malloc(c * sizeof(LPTSTR));
    if (ppszNames == NULL)
        goto err;
    ZeroMemory(ppszNames, c * sizeof(LPTSTR));

    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        s_cbName = MAX_PATH * sizeof(TCHAR);
        lResult = RegEnumKeyEx(hSubKey, i, s_szName, &s_cbName, NULL, NULL, 
                               NULL, NULL);
        if (lResult == ERROR_NO_MORE_ITEMS)
        {
            c = i;
            break;
        }
        if (lResult != ERROR_SUCCESS)
            goto err;
        if (s_cbName >= MAX_PATH * sizeof(TCHAR))
            continue;

        ppszNames[i] = _tcsdup(s_szName);
    }

    qsort(ppszNames, c, sizeof(LPTSTR), compare);

    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        if ((s_dwFlags & RSF_LOOKATKEYS) &&
            CompareName(ppszNames[i], s_szFindWhat))
        {
            *ppszFoundSubKey = malloc(
                (lstrlen(szSubKey) + lstrlen(ppszNames[i]) + 2) * 
                    sizeof(TCHAR));
            if (*ppszFoundSubKey == NULL)
                goto err;
            if (szSubKey[0])
            {
                lstrcpy(*ppszFoundSubKey, szSubKey);
                lstrcatW(*ppszFoundSubKey, s_backslash);
            }
            else
                **ppszFoundSubKey = 0;
            lstrcatW(*ppszFoundSubKey, ppszNames[i]);
            *ppszFoundValueName = NULL;
            goto success;
        }

        if (RegFindRecurse(hSubKey, ppszNames[i], NULL, ppszFoundSubKey,
                           ppszFoundValueName))
        {
            LPTSTR psz = *ppszFoundSubKey;
            *ppszFoundSubKey = malloc(
                (lstrlen(szSubKey) + lstrlen(psz) + 2) * sizeof(TCHAR));
            if (*ppszFoundSubKey == NULL)
                goto err;
            if (szSubKey[0])
            {
                lstrcpy(*ppszFoundSubKey, szSubKey);
                lstrcatW(*ppszFoundSubKey, s_backslash);
            }
            else
                **ppszFoundSubKey = 0;
            lstrcatW(*ppszFoundSubKey, psz);
            free(psz);
            goto success;
        }
    }

err:
    if (ppszNames != NULL)
    {
        for(i = 0; i < c; i++)
            free(ppszNames[i]);
        free(ppszNames);
    }
    free(pb);
    RegCloseKey(hSubKey);
    return FALSE;

success:
    if (ppszNames != NULL)
    {
        for(i = 0; i < c; i++)
            free(ppszNames[i]);
        free(ppszNames);
    }
    RegCloseKey(hSubKey);
    return TRUE;
}

BOOL RegFindWalk(
    HKEY *  phKey, 
    LPCTSTR pszSubKey,
    LPCTSTR pszValueName,
    LPTSTR *ppszFoundSubKey,
    LPTSTR *ppszFoundValueName)
{
    LONG lResult;
    DWORD i, c;
    HKEY hBaseKey, hSubKey;
    TCHAR szKeyName[MAX_PATH];
    TCHAR szSubKey[MAX_PATH];
    LPTSTR pch;
    BOOL fPast;
    LPTSTR *ppszNames = NULL;

    hBaseKey = *phKey;
    if (RegFindRecurse(hBaseKey, pszSubKey, pszValueName, ppszFoundSubKey,
                       ppszFoundValueName))
        return TRUE;

    if (lstrlen(pszSubKey) >= MAX_PATH)
        return FALSE;

    lstrcpy(szSubKey, pszSubKey);
    while(szSubKey[0] != 0)
    {
        if (DoEvents())
            return FALSE;

        pch = _tcsrchr(szSubKey, _T('\\'));
        if (pch == NULL)
        {
            lstrcpy(szKeyName, szSubKey);
            szSubKey[0] = 0;
            hSubKey = hBaseKey;
        }
        else
        {
            lstrcpyn(szKeyName, pch + 1, MAX_PATH);
            *pch = 0;
            lResult = RegOpenKeyEx(hBaseKey, szSubKey, 0, KEY_ALL_ACCESS,
                &hSubKey);
            if (lResult != ERROR_SUCCESS)
                return FALSE;
        }

        lResult = RegQueryInfoKey(hSubKey, NULL, NULL, NULL, &c, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL);
        if (lResult != ERROR_SUCCESS)
            goto err;

        ppszNames = (LPTSTR *) malloc(c * sizeof(LPTSTR));
        if (ppszNames == NULL)
            goto err;
        ZeroMemory(ppszNames, c * sizeof(LPTSTR));

        for(i = 0; i < c; i++)
        {
            if (DoEvents())
                goto err;

            s_cbName = MAX_PATH * sizeof(TCHAR);
            lResult = RegEnumKeyExW(hSubKey, i, s_szName, &s_cbName,
                NULL, NULL, NULL, NULL);
            if (lResult == ERROR_NO_MORE_ITEMS)
            {
                c = i;
                break;
            }
            if (lResult != ERROR_SUCCESS)
                break;
            ppszNames[i] = _tcsdup(s_szName);
        }

        qsort(ppszNames, c, sizeof(LPTSTR), compare);

        fPast = FALSE;
        for(i = 0; i < c; i++)
        {
            if (DoEvents())
                goto err;

            if (!fPast && lstrcmpi(ppszNames[i], szKeyName) == 0)
            {
                fPast = TRUE;
                continue;
            }
            if (!fPast)
                continue;

            if ((s_dwFlags & RSF_LOOKATKEYS) &&
                CompareName(ppszNames[i], s_szFindWhat))
            {
                *ppszFoundSubKey = malloc(
                    (lstrlen(szSubKey) + lstrlen(ppszNames[i]) + 2) *
                        sizeof(TCHAR));
                if (*ppszFoundSubKey == NULL)
                    goto err;
                if (szSubKey[0])
                {
                    lstrcpy(*ppszFoundSubKey, szSubKey);
                    lstrcatW(*ppszFoundSubKey, s_backslash);
                }
                else
                    **ppszFoundSubKey = 0;
                lstrcatW(*ppszFoundSubKey, ppszNames[i]);
                *ppszFoundValueName = NULL;
                goto success;
            }

            if (RegFindRecurse(hSubKey, ppszNames[i], NULL, 
                               ppszFoundSubKey, ppszFoundValueName))
            {
                LPTSTR psz = *ppszFoundSubKey;
                *ppszFoundSubKey = malloc(
                    (lstrlen(szSubKey) + lstrlen(psz) + 2) * 
                        sizeof(TCHAR));
                if (*ppszFoundSubKey == NULL)
                    goto err;
                if (szSubKey[0])
                {
                    lstrcpy(*ppszFoundSubKey, szSubKey);
                    lstrcatW(*ppszFoundSubKey, s_backslash);
                }
                else
                    **ppszFoundSubKey = 0;
                lstrcatW(*ppszFoundSubKey, psz);
                free(psz);
                goto success;
            }
        }
        if (ppszNames != NULL)
        {
            for(i = 0; i < c; i++)
                free(ppszNames[i]);
            free(ppszNames);
        }
        ppszNames = NULL;

        if (hBaseKey != hSubKey)
            RegCloseKey(hSubKey);
    }

    if (*phKey == HKEY_CLASSES_ROOT)
    {
        *phKey = HKEY_CURRENT_USER;
        if (RegFindRecurse(*phKey, s_empty, NULL, ppszFoundSubKey,
                           ppszFoundValueName))
            return TRUE;
    }

    if (*phKey == HKEY_CURRENT_USER)
    {
        *phKey = HKEY_LOCAL_MACHINE;
        if (RegFindRecurse(*phKey, s_empty, NULL, ppszFoundSubKey,
                           ppszFoundValueName))
            goto success;
    }

    if (*phKey == HKEY_LOCAL_MACHINE)
    {
        *phKey = HKEY_USERS;
        if (RegFindRecurse(*phKey, s_empty, NULL, ppszFoundSubKey,
                           ppszFoundValueName))
            goto success;
    }

    if (*phKey == HKEY_USERS)
    {
        *phKey = HKEY_CURRENT_CONFIG;
        if (RegFindRecurse(*phKey, s_empty, NULL, ppszFoundSubKey,
                           ppszFoundValueName))
            goto success;
    }

err:
    if (ppszNames != NULL)
    {
        for(i = 0; i < c; i++)
            free(ppszNames[i]);
        free(ppszNames);
    }
    if (hBaseKey != hSubKey)
        RegCloseKey(hSubKey);
    return FALSE;

success:
    if (ppszNames != NULL)
    {
        for(i = 0; i < c; i++)
            free(ppszNames[i]);
        free(ppszNames);
    }
    if (hBaseKey != hSubKey)
        RegCloseKey(hSubKey);
    return TRUE;
}


static DWORD GetFindFlags(void)
{
    HKEY hKey;
    DWORD dwType, dwValue, cbData;
    DWORD dwFlags = RSF_LOOKATKEYS | RSF_LOOKATVALUES | RSF_LOOKATDATA;

    if (RegOpenKey(HKEY_CURRENT_USER, g_szGeneralRegKey, &hKey) == ERROR_SUCCESS)
    {
        /* Retrieve flags from registry key */
        cbData = sizeof(dwValue);
        if (RegQueryValueEx(hKey, s_szFindFlags, NULL, &dwType, (LPBYTE) &dwValue, &cbData) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
                dwFlags = (dwFlags & ~0x0000FFFF) | ((dwValue & 0x0000FFFF) << 0);
        }

        /* Retrieve ReactOS Regedit specific flags from registry key */
        cbData = sizeof(dwValue);
        if (RegQueryValueEx(hKey, s_szFindFlagsR, NULL, &dwType, (LPBYTE) &dwValue, &cbData) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
                dwFlags = (dwFlags & ~0xFFFF0000) | ((dwValue & 0x0000FFFF) << 16);
        }

        RegCloseKey(hKey);
    }
    return dwFlags;
}

static void SetFindFlags(DWORD dwFlags)
{
    HKEY hKey;
    DWORD dwDisposition;
    DWORD dwData;

    if (RegCreateKeyEx(HKEY_CURRENT_USER, g_szGeneralRegKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        dwData = (dwFlags >> 0) & 0x0000FFFF;
        RegSetValueEx(hKey, s_szFindFlags, 0, REG_DWORD, (const BYTE *) &dwData, sizeof(dwData));

        dwData = (dwFlags >> 16) & 0x0000FFFF;
        RegSetValueEx(hKey, s_szFindFlagsR, 0, REG_DWORD, (const BYTE *) &dwData, sizeof(dwData));

        RegCloseKey(hKey);
    }
}

static INT_PTR CALLBACK AbortFindDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(hDlg);

    switch(uMsg)
    {
        case WM_CLOSE:
            s_bAbort = TRUE;
            break;

        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        case IDCANCEL:
                            s_bAbort = TRUE;
                            break;
                    }
                    break;
            }
            break;
    }
    return 0;
}

BOOL FindNext(HWND hWnd)
{
    HKEY hKeyRoot;
    LPCTSTR pszKeyPath;
    BOOL fSuccess;
    TCHAR szFullKey[512];
    LPCTSTR pszValueName;
    LPTSTR pszFoundSubKey, pszFoundValueName;

    s_dwFlags = GetFindFlags();

    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    if (pszKeyPath == NULL)
    {
        hKeyRoot = HKEY_CLASSES_ROOT;
        pszKeyPath = s_empty;
    }

    /* Create abort find dialog */
    s_hwndAbortDialog = CreateDialog(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_FINDING), hWnd, AbortFindDialogProc);
    if (s_hwndAbortDialog)
    {
        ShowWindow(s_hwndAbortDialog, SW_SHOW);
        UpdateWindow(s_hwndAbortDialog);
    }
    s_bAbort = FALSE;

    pszValueName = GetValueName(g_pChildWnd->hListWnd, -1);

    EnableWindow(hFrameWnd, FALSE);
    EnableWindow(g_pChildWnd->hTreeWnd, FALSE);
    EnableWindow(g_pChildWnd->hListWnd, FALSE);
    EnableWindow(g_pChildWnd->hAddressBarWnd, FALSE);

    fSuccess = RegFindWalk(&hKeyRoot, pszKeyPath, pszValueName, 
                           &pszFoundSubKey, &pszFoundValueName);

    EnableWindow(hFrameWnd, TRUE);
    EnableWindow(g_pChildWnd->hTreeWnd, TRUE);
    EnableWindow(g_pChildWnd->hListWnd, TRUE);
    EnableWindow(g_pChildWnd->hAddressBarWnd, TRUE);

    if (s_hwndAbortDialog)
    {
        DestroyWindow(s_hwndAbortDialog);
        s_hwndAbortDialog = NULL;
    }

    if (fSuccess)
    {
        RegKeyGetName(szFullKey, COUNT_OF(szFullKey), hKeyRoot, pszFoundSubKey);
        SelectNode(g_pChildWnd->hTreeWnd, szFullKey);
        SetValueName(g_pChildWnd->hListWnd, pszFoundValueName);
        free(pszFoundSubKey);
        free(pszFoundValueName);
        SetFocus(g_pChildWnd->hListWnd);
    }
    return TRUE;
}

static INT_PTR CALLBACK FindDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR iResult = 0;
    HWND hControl;
    LONG lStyle;
    DWORD dwFlags;
    static TCHAR s_szSavedFindValue[256];

    switch(uMsg)
    {
        case WM_INITDIALOG:
            dwFlags = GetFindFlags();

            hControl = GetDlgItem(hDlg, IDC_LOOKAT_KEYS);
            if (hControl)
                SendMessage(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATKEYS) ? TRUE : FALSE, 0);

            hControl = GetDlgItem(hDlg, IDC_LOOKAT_VALUES);
            if (hControl)
                SendMessage(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATVALUES) ? TRUE : FALSE, 0);

            hControl = GetDlgItem(hDlg, IDC_LOOKAT_DATA);
            if (hControl)
                SendMessage(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATDATA) ? TRUE : FALSE, 0);

            /* Match whole string */
            hControl = GetDlgItem(hDlg, IDC_MATCHSTRING);
            if (hControl)
                SendMessage(hControl, BM_SETCHECK, (dwFlags & RSF_WHOLESTRING) ? TRUE : FALSE, 0);

            /* Case sensitivity */
            hControl = GetDlgItem(hDlg, IDC_MATCHCASE);
            if (hControl)
                SendMessage(hControl, BM_SETCHECK, (dwFlags & RSF_MATCHCASE) ? TRUE : FALSE, 0);

            hControl = GetDlgItem(hDlg, IDC_FINDWHAT);
            if (hControl)
            {
                SetWindowText(hControl, s_szSavedFindValue);
                SetFocus(hControl);
                SendMessage(hControl, EM_SETSEL, 0, -1);
            }
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;

        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        case IDOK:
                            dwFlags = 0;

                            hControl = GetDlgItem(hDlg, IDC_LOOKAT_KEYS);
                            if (hControl && (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                                dwFlags |= RSF_LOOKATKEYS;

                            hControl = GetDlgItem(hDlg, IDC_LOOKAT_VALUES);
                            if (hControl && (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                                dwFlags |= RSF_LOOKATVALUES;

                            hControl = GetDlgItem(hDlg, IDC_LOOKAT_DATA);
                            if (hControl && (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                                dwFlags |= RSF_LOOKATDATA;

                            hControl = GetDlgItem(hDlg, IDC_MATCHSTRING);
                            if (hControl && (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                                dwFlags |= RSF_WHOLESTRING;

                            hControl = GetDlgItem(hDlg, IDC_MATCHCASE);
                            if (hControl && (SendMessage(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                                dwFlags |= RSF_MATCHCASE;

                            SetFindFlags(dwFlags);

                            hControl = GetDlgItem(hDlg, IDC_FINDWHAT);
                            if (hControl)
                                GetWindowText(hControl, s_szFindWhat, sizeof(s_szFindWhat) / sizeof(s_szFindWhat[0]));
                            EndDialog(hDlg, 1);
                            break;

                        case IDCANCEL:
                            EndDialog(hDlg, 0);
                            break;
                    }
                    break;

                case EN_CHANGE:
                    switch(LOWORD(wParam))
                    {
                        case IDC_FINDWHAT:
                            GetWindowText((HWND) lParam, s_szSavedFindValue, sizeof(s_szSavedFindValue) / sizeof(s_szSavedFindValue[0]));
                            hControl = GetDlgItem(hDlg, IDOK);
                            if (hControl)
                            {
                                lStyle = GetWindowLongPtr(hControl, GWL_STYLE);
                                if (s_szSavedFindValue[0])
                                    lStyle &= ~WS_DISABLED;
                                else
                                    lStyle |= WS_DISABLED;
                                SetWindowLongPtr(hControl, GWL_STYLE, lStyle);
                                RedrawWindow(hControl, NULL, NULL, RDW_INVALIDATE);
                            }
                            break;
                    }
            }
            break;
    }
    return iResult;
}

void FindDialog(HWND hWnd)
{
    if (DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FIND),
        hWnd, FindDialogProc, 0) != 0)
    {
        FindNext(hWnd);
    }
}

