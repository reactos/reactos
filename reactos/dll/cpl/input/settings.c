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
static HIMAGELIST hImgList;
// for SaveInputLang()
static INT OldLayoutNum;

typedef struct
{
    LANGID LangId;
    TCHAR LangName[MAX_PATH];
    TCHAR LayoutName[MAX_PATH];
    TCHAR ValName[CCH_ULONG_DEC + 1];
    TCHAR IndName[MAX_PATH];
} LAYOUT_ITEM, *LPLAYOUT_ITEM;


static INT
IsLayoutSelected()
{
    INT iIndex = (INT) SendMessage(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST),
                                   LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

    return iIndex;
}

BOOL
IsLayoutExists(LPTSTR szLayoutID, LPTSTR szLangID)
{
    HKEY hKey, hSubKey;
    TCHAR szPreload[CCH_LAYOUT_ID + 1], szLayoutNum[3 + 1],
          szTmp[CCH_LAYOUT_ID + 1], szOldLangID[CCH_LAYOUT_ID + 1];
    DWORD dwIndex = 0, dwType, dwSize;
    BOOL IsLangExists = FALSE;
    LANGID langid;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"),
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szLayoutNum);

        while (RegEnumValue(hKey, dwIndex, szLayoutNum, &dwSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szPreload);
            if (RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szPreload, &dwSize) != ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }

            langid = (LANGID)_tcstoul(szPreload, NULL, 16);
            GetLocaleInfo(langid, LOCALE_ILANGUAGE, szTmp, sizeof(szTmp) / sizeof(TCHAR));
            wsprintf(szOldLangID, _T("0000%s"), szTmp);

            if (_tcscmp(szOldLangID, szLangID) == 0) IsLangExists = TRUE;

            if (szPreload[0] == 'd')
            {
                if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"),
                                 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szTmp);
                    RegQueryValueEx(hSubKey, szPreload, NULL, NULL, (LPBYTE)szTmp, &dwSize);

                    if ((_tcscmp(szTmp, szLayoutID) == 0)&&(IsLangExists))
                    {
                        RegCloseKey(hSubKey);
                        RegCloseKey(hKey);
                        return TRUE;
                    }
                }
            }
            else
            {
                if (_tcscmp(szPreload, szLayoutID) == 0)
                {
                    RegCloseKey(hKey);
                    return TRUE;
                }
            }

            IsLangExists = FALSE;
            dwSize = sizeof(szLayoutNum);
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    return FALSE;
}

static HICON
CreateLayoutIcon(LPTSTR szInd)
{
    HDC hdc, hdcsrc;
    HBITMAP hBitmap, hBmpNew, hBmpOld;
    RECT rect;
    DWORD bkColor, bkText;
    HFONT hFont = NULL;
    ICONINFO IconInfo;
    HICON hIcon = NULL;

    hdcsrc = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcsrc);
    hBitmap = CreateCompatibleBitmap(hdcsrc, 16, 16);
    ReleaseDC(NULL, hdcsrc);

    if (hdc && hBitmap)
    {
        hBmpNew = CreateBitmap(16, 16, 1, 1, NULL);
        if (hBmpNew)
        {
            hBmpOld = SelectObject(hdc, hBitmap);
            rect.right = 16;
            rect.left = 0;
            rect.bottom = 16;
            rect.top = 0;

            bkColor = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            bkText  = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

            ExtTextOut(hdc, rect.left, rect.top, ETO_OPAQUE, &rect, _T(""), 0, NULL);

            hFont = CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               DEFAULT_QUALITY, FF_DONTCARE, _T("Tahoma"));

            SelectObject(hdc, hFont);
            DrawText(hdc, _tcsupr(szInd), 2, &rect, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
            SelectObject(hdc, hBmpNew);
            PatBlt(hdc, 0, 0, 16, 16, BLACKNESS);
            SelectObject(hdc, hBmpOld);

            IconInfo.hbmColor = hBitmap;
            IconInfo.hbmMask = hBmpNew;
            IconInfo.fIcon = TRUE;

            hIcon = CreateIconIndirect(&IconInfo);

            DeleteObject(hBmpNew);
            DeleteObject(hBmpOld);
            DeleteObject(hFont);
        }
    }

    DeleteDC(hdc);
    DeleteObject(hBitmap);

    return hIcon;
}

static BOOL
GetLayoutID(LPTSTR szLayoutNum, LPTSTR szLCID)
{
    DWORD dwBufLen;
    DWORD dwRes;
    HKEY hKey;
    TCHAR szTempLCID[CCH_LAYOUT_ID + 1];

    // Get the Layout ID
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);
        dwRes = RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szTempLCID, &dwBufLen);

        if (dwRes != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey(hKey);
    }

    // Look for a substitude of this layout
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szTempLCID);

        if (RegQueryValueEx(hKey, szTempLCID, NULL, NULL, (LPBYTE)szLCID, &dwBufLen) != ERROR_SUCCESS)
        {
            // No substitute found, then use the old LCID
            lstrcpy(szLCID, szTempLCID);
        }

        RegCloseKey(hKey);
    }
    else
    {
        // Substitutes key couldn't be opened, so use the old LCID
        lstrcpy(szLCID, szTempLCID);
    }

    return TRUE;
}

BOOL
GetLayoutName(LPCTSTR szLCID, LPTSTR szName)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[MAX_PATH], szDispName[MAX_PATH], szIndex[MAX_PATH], szPath[MAX_PATH];
    HANDLE hLib;
    int i, j, k;

    wsprintf(szBuf, _T("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szLCID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = sizeof(szBuf);

        if (RegQueryValueEx(hKey, _T("Layout Display Name"), NULL, NULL, (LPBYTE)szDispName, &dwBufLen) == ERROR_SUCCESS)
        {
            if (szDispName[0] == '@')
            {
                for (i = 0; i < _tcslen(szDispName); i++)
                {
                    if ((szDispName[i] == ',') && (szDispName[i + 1] == '-'))
                    {
                        for (j = i + 2, k = 0; j < _tcslen(szDispName)+1; j++, k++)
                        {
                            szIndex[k] = szDispName[j];
                        }
                        szDispName[i - 1] = '\0';
                        break;
                    }
                    else szDispName[i] = szDispName[i + 1];
                }

                if (ExpandEnvironmentStrings(szDispName, szPath, MAX_PATH))
                {
                    hLib = LoadLibrary(szPath);
                    if (hLib)
                    {
                        if (LoadString(hLib, _ttoi(szIndex), szPath, sizeof(szPath) / sizeof(TCHAR)) != 0)
                        {
                            _tcscpy(szName, szPath);
                            RegCloseKey(hKey);
                            return TRUE;
                        }
                        FreeLibrary(hLib);
                    }
                }
            }
        }

        dwBufLen = sizeof(szBuf);

        if (RegQueryValueEx(hKey, _T("Layout Text"), NULL, NULL, (LPBYTE)szName, &dwBufLen) == ERROR_SUCCESS)
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
    LoadString(hApplet, IDS_LANGUAGE, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    column.pszText      = szBuf;
    column.cx           = 175;
    (VOID) ListView_InsertColumn(hList, 0, &column);

    column.fmt          = LVCFMT_RIGHT;
    column.cx           = 155;
    column.iSubItem     = 1;
    LoadString(hApplet, IDS_LAYOUT, szBuf, sizeof(szBuf) / sizeof(TCHAR));
    column.pszText      = szBuf;
    (VOID) ListView_InsertColumn(hList, 1, &column);
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
    INT i, imgIndex;

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;

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
            imgIndex = ImageList_AddIcon(hImgList, CreateLayoutIcon(lItem.IndName));

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

            item.pszText = lItem.LangName;
            item.iItem   = (INT) dwIndex;
            item.lParam  = (LPARAM)_ttoi(lItem.ValName);
            item.iImage  = imgIndex;
            i = ListView_InsertItem(hList, &item);

            ListView_SetItemText(hList, i, 1, lItem.LayoutName);

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
    (VOID) ImageList_Destroy(hImgList);
    (VOID) ListView_DeleteAllItems(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST));
    hImgList = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 0, 1);
    InitLangList(MainDlgWnd);
    (VOID) ListView_SetImageList(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST), hImgList, LVSIL_SMALL);
}

static VOID
DeleteLayout(VOID)
{
    INT iIndex, LayoutNum;
    LVITEM item;
    HKEY hKey, hSubKey;
    HWND hLayoutList = GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST);
    TCHAR szLayoutNum[3 + 1], szTitle[MAX_PATH],
          szConf[MAX_PATH], szPreload[CCH_LAYOUT_ID + 1];
    DWORD dwSize;

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

            if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0,
                             KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
            {
                _ultot(LayoutNum, szLayoutNum, 10);

                dwSize = sizeof(szPreload);
                RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szPreload, &dwSize);

                if (szPreload[0] == 'd')
                {
                    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0,
                                     KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
                    {
                        if (RegDeleteValue(hSubKey, szPreload) != ERROR_SUCCESS)
                        {
                            RegCloseKey(hSubKey);
                            RegCloseKey(hKey);
                            return;
                        }
                        RegCloseKey(hSubKey);
                    }
                }

                if (RegDeleteValue(hKey, szLayoutNum) == ERROR_SUCCESS)
                {
                    UpdateLayoutsList();
                }
            }
            RegCloseKey(hKey);
        }
    }
}

static VOID
SetDefaultLayout()
{
    HKL hKl;
    TCHAR szLCID[CCH_LAYOUT_ID + 1], szLayoutNum[CCH_ULONG_DEC + 1];
    LVITEM item;
    INT LayoutNum;

    if (IsLayoutSelected() != -1)
    {
        ZeroMemory(&item, sizeof(LVITEM));

        item.mask = LVIF_PARAM;
        item.iItem = IsLayoutSelected();

        (VOID) ListView_GetItem(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST), &item);

        LayoutNum = (INT) item.lParam;
        _ultot(LayoutNum, szLayoutNum, 10);

        if (GetLayoutID(szLayoutNum, szLCID))
        {
            hKl = LoadKeyboardLayout(szLCID, KLF_ACTIVATE);
            SystemParametersInfo(SPI_SETDEFAULTINPUTLANG, 0, &hKl, SPIF_SENDWININICHANGE);
        }
    }
}

static VOID
SaveInputLang(HWND hDlg)
{
    HKEY hKey, hSubKey;
    TCHAR szLayoutID[CCH_LAYOUT_ID + 1], szLayoutNum[CCH_ULONG_DEC + 1],
          szPreload[CCH_LAYOUT_ID + 1], LangID[CCH_LAYOUT_ID + 1], szMessage[MAX_PATH],
          Lang[MAX_PATH], SubPath[MAX_PATH];
    PTSTR pts;
    INT iLayout;
    DWORD dwSize;
    LANGID langid;

    iLayout = SendMessage(GetDlgItem(hDlg, IDC_KB_LAYOUT_IME_COMBO), CB_GETCURSEL, 0, 0);
    if (iLayout == CB_ERR) return;

    pts = (PTSTR) SendMessage(GetDlgItem(hDlg, IDC_KB_LAYOUT_IME_COMBO), CB_GETITEMDATA, iLayout, 0);

    _ultot(OldLayoutNum, szLayoutNum, 10);
    if (!GetLayoutID(szLayoutNum, szLayoutID)) return;

    // if old layout = selected layout
    if (_tcscmp(szLayoutID, pts) == 0) return;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0,
                     KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szPreload);
        if (RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szPreload, &dwSize) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return;
        }

        langid = (LANGID)_tcstoul(szPreload, NULL, 16);
        GetLocaleInfo(langid, LOCALE_ILANGUAGE, Lang, sizeof(Lang) / sizeof(TCHAR));
        wsprintf(LangID, _T("0000%s"), Lang);

        if (IsLayoutExists(pts, LangID))
        {
            LoadString(hApplet, IDS_LAYOUT_EXISTS, szMessage, sizeof(szMessage) / sizeof(TCHAR));
            MessageBox(hDlg, szMessage, NULL, MB_OK | MB_ICONINFORMATION);

            RegCloseKey(hKey);
            return;
        }

        if (szPreload[0] == 'd')
        {
            if (_tcscmp(LangID, pts) == 0)
            {
                if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0,
                                 KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
                {
                    if (RegDeleteValue(hSubKey, szPreload) != ERROR_SUCCESS)
                    {
                        RegCloseKey(hSubKey);
                        RegCloseKey(hKey);
                        return;
                    }
                    RegCloseKey(hSubKey);

                    RegSetValueEx(hKey, szLayoutNum, 0, REG_SZ, (LPBYTE)pts,
                                  (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR)));
                }
            }
            else
            {
                if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0,
                                 KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
                {
                    RegSetValueEx(hSubKey, szPreload, 0, REG_SZ, (LPBYTE)pts,
                                  (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR)));

                    RegCloseKey(hSubKey);
                }
            }
        }
        else
        {
            if (_tcscmp(LangID, pts) == 0)
            {
                RegSetValueEx(hKey, szLayoutNum, 0, REG_SZ, (LPBYTE)pts,
                              (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR)));
            }
            else
            {
                if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0, NULL,
                                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                   NULL, &hSubKey, NULL) == ERROR_SUCCESS)
                {
                    wsprintf(SubPath, _T("d%03d%s"), GetLayoutCount(Lang)-1, Lang);

                    RegSetValueEx(hSubKey, SubPath, 0, REG_SZ, (LPBYTE)pts,
                                  (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR)));

                    RegSetValueEx(hKey, szLayoutNum, 0, REG_SZ, (LPBYTE)SubPath,
                                  (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR)));

                    RegCloseKey(hSubKey);
                }
            }
        }

        RegCloseKey(hKey);
    }
}

static VOID
InitInputLangPropDlg(HWND hDlg)
{
    HKEY hKey, hSubKey;
    LVITEM item;
    INT LayoutNum;
    TCHAR szLayoutNum[10 + 1], szPreload[CCH_LAYOUT_ID + 1],
          szTmp[CCH_LAYOUT_ID + 1], szName[MAX_PATH];
    DWORD dwSize;
    LANGID langid;

    ZeroMemory(&item, sizeof(LVITEM));

    item.mask = LVIF_PARAM;
    item.iItem = IsLayoutSelected();

    (VOID) ListView_GetItem(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST), &item);
    LayoutNum = (INT) item.lParam;
    OldLayoutNum = LayoutNum;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0,
                     KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        _ultot(LayoutNum, szLayoutNum, 10);

        dwSize = sizeof(szPreload);
        RegQueryValueEx(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szPreload, &dwSize);

        langid = (LANGID)_tcstoul(szPreload, NULL, 16);
        GetLocaleInfo(langid, LOCALE_SLANGUAGE, (LPTSTR)szName, sizeof(szName) / sizeof(TCHAR));
        SetWindowText(GetDlgItem(hDlg, IDC_INPUT_LANG_STR), szName);

        if (szPreload[0] == 'd')
        {
            if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0,
                                     KEY_ALL_ACCESS, &hSubKey) == ERROR_SUCCESS)
            {
                if (RegQueryValueEx(hSubKey, szPreload, NULL, NULL, (LPBYTE)szTmp, &dwSize) != ERROR_SUCCESS)
                {
                    RegCloseKey(hSubKey);
                    RegCloseKey(hKey);
                    return;
                }
                lstrcpy(szPreload, szTmp);
                RegCloseKey(hSubKey);
            }
        }

        if (GetLayoutName(szPreload, szName))
        {
            SendMessage(GetDlgItem(hDlg, IDC_KB_LAYOUT_IME_COMBO),
                        CB_SELECTSTRING, (WPARAM)-1, (LPARAM)szName);
        }
    }
    RegCloseKey(hKey);
}

INT_PTR CALLBACK
InputLangPropDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
            CreateKeyboardLayoutList(GetDlgItem(hDlg, IDC_KB_LAYOUT_IME_COMBO));
            InitInputLangPropDlg(hDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    SaveInputLang(hDlg);
                    UpdateLayoutsList();
                    EndDialog(hDlg,LOWORD(wParam));
                    break;

                case IDCANCEL:
                    EndDialog(hDlg,LOWORD(wParam));
                    break;
            }
            break;
    }

    return FALSE;
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
            hImgList = ImageList_Create(16, 16, ILC_COLOR8 | ILC_MASK, 0, 1);
            InitLangList(hwndDlg);
            (VOID) ListView_SetImageList(GetDlgItem(MainDlgWnd, IDC_KEYLAYOUT_LIST), hImgList, LVSIL_SMALL);
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
                    if (IsLayoutSelected() != -1)
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_INPUT_LANG_PROP),
                              hwndDlg,
                              InputLangPropDlgProc);
                    break;

                case IDC_SET_DEFAULT:
                    SetDefaultLayout();
                    UpdateLayoutsList();
                    break;
            }
            break;
        case WM_DESTROY:
            (VOID) ImageList_Destroy(hImgList);
            break;
    }

    return FALSE;
}

/* EOF */
