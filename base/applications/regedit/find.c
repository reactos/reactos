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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <regedit.h>

static TCHAR s_szFindWhat[256];
static const TCHAR s_szFindFlags[] = _T("FindFlags");
static const TCHAR s_szFindFlagsR[] = _T("FindFlagsReactOS");
static HWND s_hwndAbortDialog;
static BOOL s_bAbort;



static DWORD GetFindFlags(void)
{
    HKEY hKey;
    DWORD dwFlags = RSF_LOOKATKEYS;
    DWORD dwType, dwValue, cbData;

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

static BOOL RegSearchProc(LPVOID lpParam)
{
    MSG msg;
    UNREFERENCED_PARAMETER(lpParam);

    if (s_hwndAbortDialog && PeekMessage(&msg, s_hwndAbortDialog, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return s_bAbort;
}

BOOL FindNext(HWND hWnd)
{
    HKEY hKeyRoot;
    LPCTSTR pszFindWhat;
    LPCTSTR pszKeyPath;
    DWORD dwFlags;
    LONG lResult;
    TCHAR szSubKey[512];
    TCHAR szError[512];
    TCHAR szTitle[64];
    TCHAR szFullKey[512];

    pszFindWhat = s_szFindWhat;
    dwFlags = GetFindFlags() & ~(RSF_LOOKATVALUES | RSF_LOOKATDATA);

    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    lstrcpyn(szSubKey, pszKeyPath, sizeof(szSubKey) / sizeof(szSubKey[0]));

    /* Create abort find dialog */
    s_hwndAbortDialog = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FINDING), hWnd, AbortFindDialogProc);
    if (s_hwndAbortDialog)
        ShowWindow(s_hwndAbortDialog, SW_SHOW);
    s_bAbort = FALSE;

    lResult = RegSearch(hKeyRoot, szSubKey, sizeof(szSubKey) / sizeof(szSubKey[0]),
        pszFindWhat, 0, dwFlags, RegSearchProc, NULL);

    if (s_hwndAbortDialog)
    {
        DestroyWindow(s_hwndAbortDialog);
        s_hwndAbortDialog = NULL;
    }

    /* Did the user click "Cancel"?  If so, exit without displaying an error message */
    if (lResult == ERROR_OPERATION_ABORTED)
        return FALSE;

    if (lResult != ERROR_SUCCESS)
    {
        LoadString(NULL, IDS_APP_TITLE, szTitle, sizeof(szTitle) / sizeof(szTitle[0]));

        if ((lResult != ERROR_NO_MORE_ITEMS) || !LoadString(NULL, IDS_FINISHEDFIND, szError, sizeof(szError) / sizeof(szError[0])))
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lResult, 0,
                szError, sizeof(szError) / sizeof(szError[0]), NULL);
        }
        MessageBox(hWnd, szError, szTitle, MB_OK);
        return FALSE;
    }

    RegKeyGetName(szFullKey, sizeof(szFullKey) / sizeof(szFullKey[0]), hKeyRoot, szSubKey);
    SelectNode(g_pChildWnd->hTreeWnd, szFullKey);
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

            /* Looking at values is not yet implemented */
            hControl = GetDlgItem(hDlg, IDC_LOOKAT_KEYS);
            if (hControl)
                SendMessage(hControl, BM_SETCHECK, (dwFlags & RSF_LOOKATKEYS) ? TRUE : FALSE, 0);

            /* Looking at values is not yet implemented */
            hControl = GetDlgItem(hDlg, IDC_LOOKAT_VALUES);
            if (hControl)
            {
                lStyle = GetWindowLong(hControl, GWL_STYLE);
                SetWindowLong(hControl, GWL_STYLE, lStyle | WS_DISABLED);
            }

            /* Looking at data is not yet implemented */
            hControl = GetDlgItem(hDlg, IDC_LOOKAT_DATA);
            if (hControl)
            {
                lStyle = GetWindowLong(hControl, GWL_STYLE);
                SetWindowLong(hControl, GWL_STYLE, lStyle | WS_DISABLED);
            }

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
                                lStyle = GetWindowLong(hControl, GWL_STYLE);
                                if (s_szSavedFindValue[0])
                                    lStyle &= ~WS_DISABLED;
                                else
                                    lStyle |= WS_DISABLED;
                                SetWindowLong(hControl, GWL_STYLE, lStyle);
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

