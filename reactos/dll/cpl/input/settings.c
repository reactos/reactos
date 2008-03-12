/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/settings.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

#define BUFSIZE 256

typedef struct
{
    LANGID LangId;
    TCHAR LangName[MAX_PATH];
    TCHAR LayoutName[MAX_PATH];
    TCHAR ValName[MAX_PATH];
    TCHAR IndName[MAX_PATH];
    TCHAR SubName[MAX_PATH];
} LAYOUT_ITEM, *LPLAYOUT_ITEM;

BOOL
GetLayoutName(LPCTSTR lcid, LPTSTR name)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[BUFSIZE];

    _stprintf(szBuf, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"),lcid);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = BUFSIZE;
        if (RegQueryValueEx(hKey,_T("Layout Text"),NULL,NULL,(LPBYTE)name,&dwBufLen) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }
    }

    return FALSE;
}

static VOID
AddListColumn(HWND hWnd)
{
    LV_COLUMN column;
    TCHAR szBuf[MAX_PATH];
    HWND hList = GetDlgItem(hWnd, IDC_KEYLAYOUT_LIST);

    ZeroMemory(&column, sizeof(LV_COLUMN));
    column.mask         = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.fmt          = LVCFMT_LEFT;
    column.iSubItem     = 0;
    column.pszText      = NULL;
    column.cx           = 25;
    (VOID) ListView_InsertColumn(hList, 0, &column);

    column.fmt          = LVCFMT_LEFT;
    column.iSubItem     = 1;
    LoadString(hApplet, IDS_LANGUAGE, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    column.pszText      = szBuf;
    column.cx           = 160;
    (VOID) ListView_InsertColumn(hList, 1, &column);

    column.fmt          = LVCFMT_RIGHT;
    column.cx           = 145;
    column.iSubItem     = 2;
    LoadString(hApplet, IDS_LAYOUT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    column.pszText      = szBuf;
    (VOID) ListView_InsertColumn(hList, 2, &column);
}

static BOOL
InitLangList(HWND hWnd)
{
    HKEY hKey, hSubKey;
    TCHAR szBuf[MAX_PATH], szPreload[MAX_PATH], szSub[MAX_PATH];
    LAYOUT_ITEM lItem;
    AddListColumn(hWnd);
    LONG Ret;
    DWORD dwIndex = 0, dwType, dwSize;
    LV_ITEM item;
    HWND hList = GetDlgItem(hWnd, IDC_KEYLAYOUT_LIST);

    (VOID) ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"),
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = MAX_PATH;
        Ret = RegEnumValue(hKey, dwIndex, szBuf, &dwSize, NULL, &dwType, NULL, NULL);
        if (Ret == ERROR_SUCCESS)
        {
            while (Ret == ERROR_SUCCESS)
            {
                _tcscpy(lItem.ValName, szBuf);

                dwSize = MAX_PATH;
                RegQueryValueEx(hKey, szBuf, NULL, NULL, (LPBYTE)szPreload, &dwSize);

                lItem.LangId = _tcstoul(szPreload, NULL, 16);

                GetLocaleInfo(lItem.LangId, LOCALE_SISO639LANGNAME, (LPTSTR) szBuf, sizeof(szBuf));
                _tcscpy(lItem.IndName, _tcsupr(szBuf));

                GetLocaleInfo(lItem.LangId, LOCALE_SLANGUAGE, (LPTSTR)szBuf, sizeof(szBuf));
                _tcscpy(lItem.LangName, szBuf);
                
                if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"),
                                 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
                {
                    dwSize = MAX_PATH;
                    if (RegQueryValueEx(hSubKey, szPreload, NULL, NULL, (LPBYTE)szSub, &dwSize) == ERROR_SUCCESS)
                    {
                        _tcscpy(lItem.SubName, szPreload);
                        if (GetLayoutName(szSub, szBuf))
                        {
                            _tcscpy(lItem.LayoutName, szBuf);
                        }
                    }
                    else
                    {
                        _tcscpy(lItem.SubName, _T(""));
                    }
                }

                if (_tcslen(lItem.SubName) < 2)
                {
                    if (GetLayoutName(szPreload, szBuf))
                    {
                        _tcscpy(lItem.LayoutName, szBuf);
                    }
                }
                
                ZeroMemory(&item, sizeof(LV_ITEM));
                item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                item.pszText = lItem.IndName;
                item.lParam  = (LPARAM)&lItem;
                item.iItem   = (INT) dwIndex;
                INT i = ListView_InsertItem(hList, &item);

                ListView_SetItemText(hList, i, 1, lItem.LangName);
                ListView_SetItemText(hList, i, 2, lItem.LayoutName);

                dwIndex++;
                Ret = RegEnumValue(hKey, dwIndex, szBuf, &dwSize, NULL, &dwType, NULL, NULL);
                RegCloseKey(hSubKey);

                if (_tcscmp(lItem.ValName, _T("1")) == 0)
                {
                    (VOID) ListView_SetHotItem(hList, i);
                }
            }
        }
    }

    RegCloseKey(hKey);
    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
SettingPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            InitLangList(hwndDlg);
            EnableWindow(GetDlgItem(hwndDlg, IDC_PROP_BUTTON),FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SET_DEFAULT),FALSE);
        }
            break;
        case WM_NOTIFY:
        {
            switch (LOWORD(wParam))
            {

            }
        }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_KEY_SET_BTN:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_KEYSETTINGS),
                              hwndDlg,
                              KeySettingsDlgProc);
                    break;

                case IDC_ADD_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_ADD),
                              hwndDlg,
                              AddDlgProc);
                    break;

                case IDC_PROP_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_INPUT_LANG_PROP),
                              hwndDlg,
                              InputLangPropDlgProc);
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
