/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/settings.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

static HWND MainDlgWnd;

typedef struct
{
    LANGID LangId;
    TCHAR LangName[MAX_PATH];
    TCHAR LayoutName[MAX_PATH];
    TCHAR ValName[CCH_ULONG_DEC + 1];
    TCHAR IndName[MAX_PATH];
} LAYOUT_ITEM, *LPLAYOUT_ITEM;

BOOL
GetLayoutName(LPCTSTR lcid, LPTSTR name)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH];

    wsprintf(szBuf, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), lcid);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szBuf);

        if (RegQueryValueEx(hKey, _T("Layout Text"), NULL, NULL, (LPBYTE)name, &dwBufLen) == ERROR_SUCCESS)
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

static VOID
InitLangList(HWND hWnd)
{
    HKEY hKey, hSubKey;
    TCHAR szBuf[MAX_PATH], szPreload[CCH_LAYOUT_ID + 1], szSub[CCH_LAYOUT_ID + 1];
    LAYOUT_ITEM lItem;
    DWORD dwIndex = 0, dwType, dwSize;
    LV_ITEM item = {0};
    HWND hList = GetDlgItem(hWnd, IDC_KEYLAYOUT_LIST);
    INT i;

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"),
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(lItem.ValName);

        while (RegEnumValue(hKey, dwIndex, lItem.ValName, &dwSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szPreload);
            RegQueryValueEx(hKey, lItem.ValName, NULL, NULL, (LPBYTE)szPreload, &dwSize);

            lItem.LangId = (LANGID)_tcstoul(szPreload, NULL, 16);

            GetLocaleInfo(lItem.LangId, LOCALE_SISO639LANGNAME, (LPTSTR)szBuf, sizeof(szBuf) / sizeof(TCHAR));
            lstrcpy(lItem.IndName, _tcsupr(szBuf));

            GetLocaleInfo(lItem.LangId, LOCALE_SLANGUAGE, (LPTSTR)szBuf, sizeof(szBuf) / sizeof(TCHAR));
            lstrcpy(lItem.LangName, szBuf);

            // Does this keyboard layout have a substitute?
            // Then add the substitute instead of the Layout ID
            if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"),
                             0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
            {
                dwSize = sizeof(szSub);

                if (RegQueryValueEx(hSubKey, szPreload, NULL, NULL, (LPBYTE)szSub, &dwSize) == ERROR_SUCCESS)
                {
                    lstrcpy(szPreload, szSub);
                }

                RegCloseKey(hSubKey);
            }

            GetLayoutName(szPreload, lItem.LayoutName);

            item.pszText = lItem.IndName;
            item.iItem   = (INT) dwIndex;
            item.lParam  = (LPARAM)_ttoi(lItem.ValName);
            i = ListView_InsertItem(hList, &item);

            ListView_SetItemText(hList, i, 1, lItem.LangName);
            ListView_SetItemText(hList, i, 2, lItem.LayoutName);

            dwIndex++;

            if (lstrcmp(lItem.ValName, _T("1")) == 0)
            {
                (VOID) ListView_SetHotItem(hList, i);
            }
        }

        RegCloseKey(hKey);
    }
}

VOID
UpdateLayoutsList(VOID)
{
    (VOID) ListView_DeleteAllItems(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST));
    InitLangList(MainDlgWnd);
}

static VOID
DeleteLayout(VOID)
{
    INT iIndex, LayoutNum;
    LVITEM item;
    HKEY hKey;
    HWND hLayoutList = GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST);
    TCHAR szLayoutNum[10 + 1], szTitle[MAX_PATH], szConf[MAX_PATH];

    iIndex = (INT) SendMessage(hLayoutList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

    if (iIndex != -1)
    {
        LoadString(hApplet, IDS_REM_QUESTION, szConf, sizeof(szConf) / sizeof(TCHAR));
        LoadString(hApplet, IDS_CONFIRMATION, szTitle, sizeof(szTitle) / sizeof(TCHAR));

        if (MessageBox(MainDlgWnd, szConf, szTitle, MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            ZeroMemory(&item, sizeof(LVITEM));

            item.mask = LVIF_PARAM;
            item.iItem = iIndex;

            (VOID) ListView_GetItem(hLayoutList, &item);
            LayoutNum = (INT) item.lParam;

            if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
            {
                _ultot(LayoutNum, szLayoutNum, 10);
                if (RegDeleteValue(hKey, szLayoutNum) == ERROR_SUCCESS)
                {
                    UpdateLayoutsList();
                }
            }
            RegCloseKey(hKey);
        }
    }
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
            MainDlgWnd = hwndDlg;
            AddListColumn(hwndDlg);
            (VOID) ListView_SetExtendedListViewStyle(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST),
                                                     LVS_EX_FULLROWSELECT);
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
                case IDC_REMOVE_BUTTON:
                    DeleteLayout();
                    break;

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
