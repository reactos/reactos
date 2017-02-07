#include "wined3dcfg.h"

static LONG ReadSetting(HKEY hKey, PWCHAR szKey, PWCHAR szBuffer, DWORD dwSize)
{
    return RegQueryValueExW(hKey, szKey, NULL, NULL, (LPBYTE)szBuffer, &dwSize);
}

static VOID SaveSetting(HKEY hKey, PWCHAR szKey, PWCHAR szState)
{
    RegSetValueExW(hKey, szKey, 0, REG_SZ, (LPBYTE)szState, (wcslen(szState) + 1) * sizeof(WCHAR));
}

static VOID InitSettings(HWND hWndDlg)
{
    HKEY hKey;
    WCHAR szBuffer[MAX_KEY_LENGTH];
    DWORD dwSize = MAX_KEY_LENGTH;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      KEY_WINE,
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    if(ReadSetting(hKey, KEY_GLSL, szBuffer, dwSize) == ERROR_SUCCESS)
        CheckDlgButton(hWndDlg, IDC_GLSL, (wcscmp(VALUE_DISABLED, szBuffer) != 0) ? BST_CHECKED : BST_UNCHECKED);

    if(ReadSetting(hKey, KEY_MULTISAMPLING, szBuffer, dwSize) == ERROR_SUCCESS)
        CheckDlgButton(hWndDlg, IDC_MULTISAMPLING, (wcscmp(VALUE_ENABLED, szBuffer) == 0) ? BST_CHECKED : BST_UNCHECKED);

    if(ReadSetting(hKey, KEY_PIXELSHADERS, szBuffer, dwSize) == ERROR_SUCCESS)
        CheckDlgButton(hWndDlg, IDC_PIXELSHADERS, (wcscmp(VALUE_ENABLED, szBuffer) == 0) ? BST_CHECKED : BST_UNCHECKED);

    if(ReadSetting(hKey, KEY_STRICTDRAWORDERING, szBuffer, dwSize) == ERROR_SUCCESS)
        CheckDlgButton(hWndDlg, IDC_STRICTDRAWORDERING, (wcscmp(VALUE_ENABLED, szBuffer) == 0) ? BST_CHECKED : BST_UNCHECKED);

    if(ReadSetting(hKey, KEY_VERTEXSHADERS, szBuffer, dwSize) == ERROR_SUCCESS)
        CheckDlgButton(hWndDlg, IDC_VERTEXSHADERS, (wcscmp(VALUE_NONE, szBuffer) != 0) ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_ADDSTRING, 0, (LPARAM)VALUE_FBO);
    SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_ADDSTRING, 0, (LPARAM)VALUE_BACKBUFFER);

    SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_SETITEMDATA, ITEM_FBO, (LPARAM)ITEM_FBO);
    SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_SETITEMDATA, ITEM_BACKBUFFER, (LPARAM)ITEM_BACKBUFFER);

    if(ReadSetting(hKey, KEY_OFFSCREEN, szBuffer, dwSize) == ERROR_SUCCESS && !wcscmp(VALUE_BACKBUFFER, szBuffer))
        SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_SETCURSEL, 1, 0);
    else
        SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_SETCURSEL, 0, 0);

    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_ADDSTRING, 0, (LPARAM)VALUE_READTEX);
    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_ADDSTRING, 0, (LPARAM)VALUE_READDRAW);
    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_ADDSTRING, 0, (LPARAM)VALUE_DISABLED);

    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_SETITEMDATA, (WPARAM)ITEM_READTEX, (LPARAM)ITEM_READTEX);
    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_SETITEMDATA, (WPARAM)ITEM_READDRAW, (LPARAM)ITEM_READDRAW);
    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_SETITEMDATA, (WPARAM)ITEM_DISABLED, (LPARAM)ITEM_DISABLED);

    SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_SETCURSEL, 0, 0);

    if(ReadSetting(hKey, KEY_LOCKING, szBuffer, dwSize) == ERROR_SUCCESS)
    {
        if(!wcscmp(VALUE_READDRAW, szBuffer))
            SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_SETCURSEL, 1, 0);
        else if(!wcscmp(VALUE_DISABLED, szBuffer))
            SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_SETCURSEL, 2, 0);
    }

    RegCloseKey(hKey);
}

static VOID WriteSettings(HWND hWndDlg)
{
    HKEY hKey;
    INT iCurSel;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      KEY_WINE,
                      0,
                      KEY_WRITE,
                      &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    SaveSetting(hKey, KEY_GLSL, (IsDlgButtonChecked(hWndDlg, IDC_GLSL) == BST_CHECKED) ? VALUE_ENABLED : VALUE_DISABLED);
    SaveSetting(hKey, KEY_MULTISAMPLING, (IsDlgButtonChecked(hWndDlg, IDC_MULTISAMPLING) == BST_CHECKED) ? VALUE_ENABLED : VALUE_DISABLED);
    SaveSetting(hKey, KEY_PIXELSHADERS, (IsDlgButtonChecked(hWndDlg, IDC_PIXELSHADERS) == BST_CHECKED) ? VALUE_ENABLED : VALUE_DISABLED);
    SaveSetting(hKey, KEY_STRICTDRAWORDERING, (IsDlgButtonChecked(hWndDlg, IDC_STRICTDRAWORDERING) == BST_CHECKED) ? VALUE_ENABLED : VALUE_DISABLED);
    SaveSetting(hKey, KEY_VERTEXSHADERS, (IsDlgButtonChecked(hWndDlg, IDC_VERTEXSHADERS) == BST_CHECKED) ? VALUE_ENABLED : VALUE_NONE);

    iCurSel = (INT)SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_GETCURSEL, 0, 0);

    if(iCurSel != CB_ERR)
    {
        iCurSel = (INT)SendDlgItemMessageW(hWndDlg, IDC_LOCKING, CB_GETITEMDATA, (WPARAM)iCurSel, 0);

        if(iCurSel == ITEM_READDRAW)
            SaveSetting(hKey, KEY_LOCKING, VALUE_READDRAW);
        else if(iCurSel == ITEM_DISABLED)
            SaveSetting(hKey, KEY_LOCKING, VALUE_DISABLED);
        else
            SaveSetting(hKey, KEY_LOCKING, VALUE_READTEX);
    }

    iCurSel = (INT)SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_GETCURSEL, 0, 0);

    if(iCurSel != CB_ERR)
    {
        iCurSel = (INT)SendDlgItemMessageW(hWndDlg, IDC_OFFSCREEN, CB_GETITEMDATA, (WPARAM)iCurSel, 0);

        if(iCurSel == ITEM_BACKBUFFER)
            SaveSetting(hKey, KEY_OFFSCREEN, VALUE_BACKBUFFER);
        else
            SaveSetting(hKey, KEY_OFFSCREEN, VALUE_FBO);
    }

    RegCloseKey(hKey);
}


INT_PTR CALLBACK GeneralPageProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPPSHNOTIFY lppsn;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            InitSettings(hWndDlg);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_GLSL:
                case IDC_LOCKING:
                case IDC_MULTISAMPLING:
                case IDC_OFFSCREEN:
                case IDC_PIXELSHADERS:
                case IDC_STRICTDRAWORDERING:
                case IDC_VERTEXSHADERS:
                    PropSheet_Changed(GetParent(hWndDlg), hWndDlg);
                    break;
                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            lppsn = (LPPSHNOTIFY)lParam;
            if (lppsn->hdr.code == PSN_APPLY)
            {
                WriteSettings(hWndDlg);
                return TRUE;
            }
            break;
    }

    return FALSE;
}
