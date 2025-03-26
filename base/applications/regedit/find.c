/*
 * Regedit find dialog
 *
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

#define RSF_WHOLESTRING    0x00000001
#define RSF_LOOKATKEYS     0x00000002
#define RSF_LOOKATVALUES   0x00000004
#define RSF_LOOKATDATA     0x00000008
#define RSF_MATCHCASE      0x00010000

static WCHAR s_szFindWhat[256];
static const WCHAR s_szFindFlags[] = L"FindFlags";
static const WCHAR s_szFindFlagsR[] = L"FindFlagsReactOS";
static HWND s_hwndAbortDialog;
static BOOL s_bAbort;

static DWORD s_dwFlags;
static WCHAR s_szName[MAX_PATH];
static DWORD s_cchName;
static const WCHAR s_empty[] = L"";
static const WCHAR s_backslash[] = L"\\";

extern VOID SetValueName(HWND hwndLV, LPCWSTR pszValueName);

BOOL DoEvents(VOID)
{
    MSG msg;
    if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            s_bAbort = TRUE;
        if (!IsDialogMessageW(s_hwndAbortDialog, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return s_bAbort;
}

static LPWSTR lstrstri(LPCWSTR psz1, LPCWSTR psz2)
{
    INT i, cch1, cch2;

    cch1 = wcslen(psz1);
    cch2 = wcslen(psz2);
    for(i = 0; i <= cch1 - cch2; i++)
    {
        if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                           psz1 + i, cch2, psz2, cch2) == 2)
            return (LPWSTR) (psz1 + i);
    }
    return NULL;
}

static BOOL CompareName(LPCWSTR pszName1, LPCWSTR pszName2)
{
    if (s_dwFlags & RSF_WHOLESTRING)
    {
        if (s_dwFlags & RSF_MATCHCASE)
            return wcscmp(pszName1, pszName2) == 0;
        else
            return _wcsicmp(pszName1, pszName2) == 0;
    }
    else
    {
        if (s_dwFlags & RSF_MATCHCASE)
            return wcsstr(pszName1, pszName2) != NULL;
        else
            return lstrstri(pszName1, pszName2) != NULL;
    }
}

/* Do not assume that pch1 is terminated with UNICODE_NULL */
static BOOL MatchString(LPCWCH pch1, INT cch1, LPCWCH pch2, INT cch2)
{
    INT i;
    DWORD dwNorm = ((s_dwFlags & RSF_MATCHCASE) ? 0 : NORM_IGNORECASE);

    if (s_dwFlags & RSF_WHOLESTRING)
        return 2 == CompareStringW(LOCALE_SYSTEM_DEFAULT, dwNorm, pch1, cch1, pch2, cch2);

    if (cch1 < cch2)
        return FALSE;

    for (i = 0; i <= cch1 - cch2; i++)
    {
        if (2 == CompareStringW(LOCALE_SYSTEM_DEFAULT, dwNorm, pch1 + i, cch2, pch2, cch2))
            return TRUE;
    }

    return FALSE;
}

static BOOL MatchData(DWORD dwType, LPCVOID pv1, DWORD cb1)
{
    if (dwType == REG_SZ || dwType == REG_EXPAND_SZ || dwType == REG_MULTI_SZ)
        return MatchString(pv1, (INT)(cb1 / sizeof(WCHAR)), s_szFindWhat, lstrlenW(s_szFindWhat));

    return FALSE;
}

int compare(const void *x, const void *y)
{
    const LPCWSTR *a = (const LPCWSTR *)x;
    const LPCWSTR *b = (const LPCWSTR *)y;
    return _wcsicmp(*a, *b);
}

BOOL RegFindRecurse(
    HKEY    hKey,
    LPCWSTR pszSubKey,
    LPCWSTR pszValueName,
    LPWSTR *ppszFoundSubKey,
    LPWSTR *ppszFoundValueName)
{
    HKEY hSubKey;
    LONG lResult;
    WCHAR szSubKey[MAX_PATH];
    DWORD i, c, cb, type;
    BOOL fPast;
    LPWSTR *ppszNames = NULL;
    LPBYTE pb = NULL;

    if (DoEvents())
        return FALSE;

    if(wcslen(pszSubKey) >= _countof(szSubKey))
        return FALSE;

    StringCbCopyW(szSubKey, sizeof(szSubKey), pszSubKey);
    hSubKey = NULL;

    lResult = RegOpenKeyExW(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
    if (lResult != ERROR_SUCCESS)
        return FALSE;

    lResult = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL, NULL, NULL,
                              &c, NULL, NULL, NULL, NULL);
    if (lResult != ERROR_SUCCESS)
        goto err;
    ppszNames = (LPWSTR *) malloc(c * sizeof(LPWSTR));
    if (ppszNames == NULL)
        goto err;
    ZeroMemory(ppszNames, c * sizeof(LPWSTR));

    /* Retrieve the value names associated with the current key */
    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        s_cchName = _countof(s_szName);
        lResult = RegEnumValueW(hSubKey, i, s_szName, &s_cchName, NULL, NULL,
                               NULL, &cb);
        if (lResult == ERROR_NO_MORE_ITEMS)
        {
            c = i;
            break;
        }
        if (lResult != ERROR_SUCCESS)
            goto err;
        if (s_cchName >= _countof(s_szName))
            continue;

        ppszNames[i] = _wcsdup(s_szName);
    }

    qsort(ppszNames, c, sizeof(LPWSTR), compare);

    /* If pszValueName is NULL, the function will search for all values within the key */
    fPast = (pszValueName == NULL);

    /* Search within the values */
    for (i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        if (!fPast && _wcsicmp(ppszNames[i], pszValueName) == 0)
        {
            fPast = TRUE;
            continue;
        }
        if (!fPast)
            continue;

        if ((s_dwFlags & RSF_LOOKATVALUES) &&
                CompareName(ppszNames[i], s_szFindWhat))
        {
            *ppszFoundSubKey = _wcsdup(szSubKey);
            *ppszFoundValueName = _wcsdup(ppszNames[i]);
            goto success;
        }

        lResult = RegQueryValueExW(hSubKey, ppszNames[i], NULL, &type,
                                  NULL, &cb);
        if (lResult != ERROR_SUCCESS)
            goto err;
        pb = malloc(cb);
        if (pb == NULL)
            goto err;
        lResult = RegQueryValueExW(hSubKey, ppszNames[i], NULL, &type,
                                  pb, &cb);
        if (lResult != ERROR_SUCCESS)
            goto err;

        if ((s_dwFlags & RSF_LOOKATDATA) && MatchData(type, pb, cb))
        {
            *ppszFoundSubKey = _wcsdup(szSubKey);
            *ppszFoundValueName = _wcsdup(ppszNames[i]);
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

    /* Retrieve the number of sub-keys */
    lResult = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &c, NULL, NULL,
                              NULL, NULL, NULL, NULL, NULL);
    if (lResult != ERROR_SUCCESS)
        goto err;
    ppszNames = (LPWSTR *) malloc(c * sizeof(LPWSTR));
    if (ppszNames == NULL)
        goto err;
    ZeroMemory(ppszNames, c * sizeof(LPWSTR));

    /* Retrieve the names of the sub-keys */
    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        s_cchName = _countof(s_szName);
        lResult = RegEnumKeyExW(hSubKey, i, s_szName, &s_cchName, NULL, NULL,
                               NULL, NULL);
        if (lResult == ERROR_NO_MORE_ITEMS)
        {
            c = i;
            break;
        }
        if (lResult != ERROR_SUCCESS)
            goto err;
        if (s_cchName >= _countof(s_szName))
            continue;

        ppszNames[i] = _wcsdup(s_szName);
    }

    qsort(ppszNames, c, sizeof(LPWSTR), compare);

    /* Search within the sub-keys */
    for(i = 0; i < c; i++)
    {
        if (DoEvents())
            goto err;

        if ((s_dwFlags & RSF_LOOKATKEYS) &&
                CompareName(ppszNames[i], s_szFindWhat))
        {
            *ppszFoundSubKey = malloc(
                                   (wcslen(szSubKey) + wcslen(ppszNames[i]) + 2) *
                                   sizeof(WCHAR));
            if (*ppszFoundSubKey == NULL)
                goto err;
            if (szSubKey[0])
            {
                wcscpy(*ppszFoundSubKey, szSubKey);
                wcscat(*ppszFoundSubKey, s_backslash);
            }
            else
                **ppszFoundSubKey = 0;
            wcscat(*ppszFoundSubKey, ppszNames[i]);
            *ppszFoundValueName = NULL;
            goto success;
        }

        /* Search within the value entries of the sub-key */
        if (RegFindRecurse(hSubKey, ppszNames[i], NULL, ppszFoundSubKey,
                           ppszFoundValueName))
        {
            LPWSTR psz = *ppszFoundSubKey;
            SIZE_T cbFoundSubKey = (wcslen(szSubKey) + wcslen(psz) + 2) * sizeof(WCHAR);
            *ppszFoundSubKey = malloc(cbFoundSubKey);
            if (*ppszFoundSubKey == NULL)
                goto err;
            if (szSubKey[0])
            {
                StringCbCopyW(*ppszFoundSubKey, cbFoundSubKey, szSubKey);
                StringCbCatW(*ppszFoundSubKey, cbFoundSubKey, s_backslash);
            }
            else
                **ppszFoundSubKey = 0;
            wcscat(*ppszFoundSubKey, psz);
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
    LPCWSTR pszSubKey,
    LPCWSTR pszValueName,
    LPWSTR *ppszFoundSubKey,
    LPWSTR *ppszFoundValueName)
{
    LONG lResult;
    DWORD i, c;
    HKEY hBaseKey, hSubKey;
    WCHAR szKeyName[MAX_PATH];
    WCHAR szSubKey[MAX_PATH];
    LPWSTR pch;
    BOOL fPast, fKeyMatched;
    LPWSTR *ppszNames = NULL;

    hBaseKey = *phKey;

    if (wcslen(pszSubKey) >= _countof(szSubKey))
        return FALSE;

    if (RegFindRecurse(hBaseKey, pszSubKey, pszValueName, ppszFoundSubKey,
                       ppszFoundValueName))
        return TRUE;

    StringCbCopyW(szSubKey, sizeof(szSubKey), pszSubKey);
    while(szSubKey[0] != 0)
    {
        if (DoEvents())
            return FALSE;

        pch = wcsrchr(szSubKey, L'\\');
        if (pch == NULL)
        {
            wcscpy(szKeyName, szSubKey);
            szSubKey[0] = 0;
            hSubKey = hBaseKey;
        }
        else
        {
            lstrcpynW(szKeyName, pch + 1, MAX_PATH);
            *pch = 0;
            lResult = RegOpenKeyExW(hBaseKey, szSubKey, 0, KEY_ALL_ACCESS,
                                   &hSubKey);
            if (lResult != ERROR_SUCCESS)
                return FALSE;
        }

        lResult = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, &c, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL);
        if (lResult != ERROR_SUCCESS)
            goto err;

        ppszNames = (LPWSTR *) malloc(c * sizeof(LPWSTR));
        if (ppszNames == NULL)
            goto err;
        ZeroMemory(ppszNames, c * sizeof(LPWSTR));

        for(i = 0; i < c; i++)
        {
            if (DoEvents())
                goto err;

            s_cchName = _countof(s_szName);
            lResult = RegEnumKeyExW(hSubKey, i, s_szName, &s_cchName,
                                    NULL, NULL, NULL, NULL);
            if (lResult == ERROR_NO_MORE_ITEMS)
            {
                c = i;
                break;
            }
            if (lResult != ERROR_SUCCESS)
                break;
            ppszNames[i] = _wcsdup(s_szName);
        }

        qsort(ppszNames, c, sizeof(LPWSTR), compare);

        fPast = FALSE;
        for(i = 0; i < c; i++)
        {
            if (DoEvents())
                goto err;

            if (!fPast && _wcsicmp(ppszNames[i], szKeyName) == 0)
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
                                       (wcslen(szSubKey) + wcslen(ppszNames[i]) + 2) *
                                       sizeof(WCHAR));
                if (*ppszFoundSubKey == NULL)
                    goto err;
                if (szSubKey[0])
                {
                    wcscpy(*ppszFoundSubKey, szSubKey);
                    wcscat(*ppszFoundSubKey, s_backslash);
                }
                else
                    **ppszFoundSubKey = 0;
                wcscat(*ppszFoundSubKey, ppszNames[i]);
                *ppszFoundValueName = NULL;
                goto success;
            }

            fKeyMatched = (_wcsicmp(ppszNames[i], szKeyName) == 0);
            if (RegFindRecurse(hSubKey, ppszNames[i], (fKeyMatched ? pszValueName : NULL),
                               ppszFoundSubKey, ppszFoundValueName))
            {
                LPWSTR psz = *ppszFoundSubKey;
                SIZE_T cbFoundSubKey = (wcslen(szSubKey) + wcslen(psz) + 2) * sizeof(WCHAR);
                *ppszFoundSubKey = malloc(cbFoundSubKey);
                if (*ppszFoundSubKey == NULL)
                    goto err;
                if (szSubKey[0])
                {
                    StringCbCopyW(*ppszFoundSubKey, cbFoundSubKey, szSubKey);
                    StringCbCatW(*ppszFoundSubKey, cbFoundSubKey, s_backslash);
                }
                else
                    **ppszFoundSubKey = 0;
                wcscat(*ppszFoundSubKey, psz);
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

    if (RegOpenKeyW(HKEY_CURRENT_USER, g_szGeneralRegKey, &hKey) == ERROR_SUCCESS)
    {
        /* Retrieve flags from registry key */
        cbData = sizeof(dwValue);
        if (RegQueryValueExW(hKey, s_szFindFlags, NULL, &dwType, (LPBYTE) &dwValue, &cbData) == ERROR_SUCCESS)
        {
            if (dwType == REG_DWORD)
                dwFlags = (dwFlags & ~0x0000FFFF) | ((dwValue & 0x0000FFFF) << 0);
        }

        /* Retrieve ReactOS Regedit specific flags from registry key */
        cbData = sizeof(dwValue);
        if (RegQueryValueExW(hKey, s_szFindFlagsR, NULL, &dwType, (LPBYTE) &dwValue, &cbData) == ERROR_SUCCESS)
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

    if (RegCreateKeyExW(HKEY_CURRENT_USER, g_szGeneralRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        dwData = (dwFlags >> 0) & 0x0000FFFF;
        RegSetValueExW(hKey, s_szFindFlags, 0, REG_DWORD, (const BYTE *) &dwData, sizeof(dwData));

        dwData = (dwFlags >> 16) & 0x0000FFFF;
        RegSetValueExW(hKey, s_szFindFlagsR, 0, REG_DWORD, (const BYTE *) &dwData, sizeof(dwData));

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
    LPCWSTR pszKeyPath;
    BOOL fSuccess;
    WCHAR szFullKey[512];
    LPCWSTR pszValueName;
    LPWSTR pszFoundSubKey, pszFoundValueName;

    if (wcslen(s_szFindWhat) == 0)
    {
        FindDialog(hWnd);
        return TRUE;
    }

    s_dwFlags = GetFindFlags();

    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    if (pszKeyPath == NULL)
    {
        hKeyRoot = HKEY_CLASSES_ROOT;
        pszKeyPath = s_empty;
    }

    /* Create abort find dialog */
    s_hwndAbortDialog = CreateDialogW(GetModuleHandle(NULL),
                                     MAKEINTRESOURCEW(IDD_FINDING), hWnd, AbortFindDialogProc);
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
        GetKeyName(szFullKey, ARRAY_SIZE(szFullKey), hKeyRoot, pszFoundSubKey);
        SelectNode(g_pChildWnd->hTreeWnd, szFullKey);
        free(pszFoundSubKey);

        if (pszFoundValueName != NULL)
        {
            SetValueName(g_pChildWnd->hListWnd, pszFoundValueName);
            free(pszFoundValueName);
            SetFocus(g_pChildWnd->hListWnd);
        }
        else
        {
            SetFocus(g_pChildWnd->hTreeWnd);
        }
    }
    return fSuccess || s_bAbort;
}

static INT_PTR CALLBACK FindDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR iResult = 0;
    HWND hControl;
    LONG lStyle;
    DWORD dwFlags;
    static WCHAR s_szSavedFindValue[256];

    switch(uMsg)
    {
    case WM_INITDIALOG:
        dwFlags = GetFindFlags();

        hControl = GetDlgItem(hDlg, IDC_LOOKAT_KEYS);
        if (hControl)
            SendMessageW(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATKEYS) ? TRUE : FALSE, 0);

        hControl = GetDlgItem(hDlg, IDC_LOOKAT_VALUES);
        if (hControl)
            SendMessageW(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATVALUES) ? TRUE : FALSE, 0);

        hControl = GetDlgItem(hDlg, IDC_LOOKAT_DATA);
        if (hControl)
            SendMessageW(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATDATA) ? TRUE : FALSE, 0);

        /* Match whole string */
        hControl = GetDlgItem(hDlg, IDC_MATCHSTRING);
        if (hControl)
            SendMessageW(hControl, BM_SETCHECK, (dwFlags & RSF_WHOLESTRING) ? TRUE : FALSE, 0);

        /* Case sensitivity */
        hControl = GetDlgItem(hDlg, IDC_MATCHCASE);
        if (hControl)
            SendMessageW(hControl, BM_SETCHECK, (dwFlags & RSF_MATCHCASE) ? TRUE : FALSE, 0);

        hControl = GetDlgItem(hDlg, IDC_FINDWHAT);
        if (hControl)
        {
            SetWindowTextW(hControl, s_szSavedFindValue);
            SetFocus(hControl);
            SendMessageW(hControl, EM_SETSEL, 0, -1);
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
                if (hControl && (SendMessageW(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    dwFlags |= RSF_LOOKATKEYS;

                hControl = GetDlgItem(hDlg, IDC_LOOKAT_VALUES);
                if (hControl && (SendMessageW(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    dwFlags |= RSF_LOOKATVALUES;

                hControl = GetDlgItem(hDlg, IDC_LOOKAT_DATA);
                if (hControl && (SendMessageW(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    dwFlags |= RSF_LOOKATDATA;

                hControl = GetDlgItem(hDlg, IDC_MATCHSTRING);
                if (hControl && (SendMessageW(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    dwFlags |= RSF_WHOLESTRING;

                hControl = GetDlgItem(hDlg, IDC_MATCHCASE);
                if (hControl && (SendMessageW(hControl, BM_GETCHECK, 0, 0) == BST_CHECKED))
                    dwFlags |= RSF_MATCHCASE;

                SetFindFlags(dwFlags);

                hControl = GetDlgItem(hDlg, IDC_FINDWHAT);
                if (hControl)
                    GetWindowTextW(hControl, s_szFindWhat, ARRAY_SIZE(s_szFindWhat));
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
                GetWindowTextW((HWND) lParam, s_szSavedFindValue, ARRAY_SIZE(s_szSavedFindValue));
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

void FindNextMessageBox(HWND hWnd)
{
    if (!FindNext(hWnd))
    {
        WCHAR msg[128], caption[128];

        LoadStringW(hInst, IDS_FINISHEDFIND, msg, ARRAY_SIZE(msg));
        LoadStringW(hInst, IDS_APP_TITLE, caption, ARRAY_SIZE(caption));
        MessageBoxW(hWnd, msg, caption, MB_ICONINFORMATION);
    }
}

void FindDialog(HWND hWnd)
{
    if (DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_FIND),
                       hWnd, FindDialogProc, 0) != 0)
    {
        FindNextMessageBox(hWnd);
    }
}
