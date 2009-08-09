/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/add.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 *                  Colin Finck
 * UPDATE HISTORY:
 *      06-09-2007  Created
 */

#include "resource.h"
#include "input.h"

static HWND hLangList;
static HWND hLayoutList;

static VOID
SelectLayoutByLang(VOID)
{
    TCHAR Layout[MAX_PATH], Lang[MAX_PATH], LangID[CCH_LAYOUT_ID + 1];
    INT iIndex;
    LCID Lcid;

    iIndex = SendMessage(hLangList, CB_GETCURSEL, 0, 0);
    Lcid = SendMessage(hLangList, CB_GETITEMDATA, iIndex, 0);

    GetLocaleInfo(MAKELCID(Lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, Lang, sizeof(Lang) / sizeof(TCHAR));

    wsprintf(LangID, _T("0000%s"), Lang);

    if (GetLayoutName(LangID, Layout))
    {
        SendMessage(hLayoutList, CB_SELECTSTRING,
                    (WPARAM) -1, (LPARAM)Layout);
    }
}

INT
GetLayoutCount(LPTSTR szLang)
{
    HKEY hKey;
    TCHAR szLayoutID[3 + 1], szPreload[CCH_LAYOUT_ID + 1], szLOLang[MAX_PATH];
    DWORD dwIndex = 0, dwType, dwSize;
    UINT Count = 0, i, j;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"),
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szLayoutID);

        while (RegEnumValue(hKey, dwIndex, szLayoutID, &dwSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szPreload);
            RegQueryValueEx(hKey, szLayoutID, NULL, NULL, (LPBYTE)szPreload, &dwSize);

            for (i = 4, j = 0; i < _tcslen(szPreload)+1; i++, j++)
                szLOLang[j] = szPreload[i];

            if (_tcscmp(szLOLang, szLang) == 0) Count += 1;

            dwSize = sizeof(szLayoutID);
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    return Count;
}

static VOID
AddNewLayout(HWND hwndDlg)
{
    TCHAR NewLayout[CCH_ULONG_DEC + 1], Lang[MAX_PATH],
          LangID[CCH_LAYOUT_ID + 1], Layout[MAX_PATH],
          SubPath[CCH_LAYOUT_ID + 1], szMessage[MAX_PATH];
    INT iLayout, iLang;
    HKEY hKey, hSubKey;
    DWORD cValues;
    PTSTR pts;
    LCID lcid;

    iLayout = SendMessage(hLayoutList, CB_GETCURSEL, 0, 0);
    if (iLayout == CB_ERR) return;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            _ultot(cValues + 1, NewLayout, 10);

            iLang = SendMessage(hLangList, CB_GETCURSEL, 0, 0);
            lcid = SendMessage(hLangList, CB_GETITEMDATA, iLang, 0);
            pts = (PTSTR) SendMessage(hLayoutList, CB_GETITEMDATA, iLayout, 0);

            GetLocaleInfo(MAKELCID(lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, Lang, sizeof(Lang) / sizeof(TCHAR));
            wsprintf(LangID, _T("0000%s"), Lang);

            if (IsLayoutExists(pts, LangID))
            {
                LoadString(hApplet, IDS_LAYOUT_EXISTS2, szMessage, sizeof(szMessage) / sizeof(TCHAR));
                MessageBox(hwndDlg, szMessage, NULL, MB_OK | MB_ICONINFORMATION);

                RegCloseKey(hKey);
                return;
            }

            if (_tcscmp(LangID, pts) != 0)
            {
                if (!GetLayoutName(pts, Layout))
                {
                    RegCloseKey(hKey);
                    return;
                }
            }
            else
            {
                if (!GetLayoutName(LangID, Layout))
                {
                    RegCloseKey(hKey);
                    return;
                }
            }

            if (SendMessage(hLayoutList, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)Layout) != CB_ERR)
            {
                if (GetLayoutCount(Lang) >= 1)
                {
                    wsprintf(SubPath, _T("d%03d%s"), GetLayoutCount(Lang), Lang);
                }
                else if ((_tcscmp(LangID, pts) != 0) && (GetLayoutCount(Lang) == 0))
                {
                    wsprintf(SubPath, _T("d%03d%s"), 0, Lang);
                }
                else SubPath[0] = '\0';
            }
            else
            {
                RegCloseKey(hKey);
                return;
            }

            if (_tcslen(SubPath) != 0)
            {
                if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0, NULL,
                                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                   NULL, &hSubKey, NULL) == ERROR_SUCCESS)
                {
                    if (RegSetValueEx(hSubKey, SubPath, 0, REG_SZ, (LPBYTE)pts,
                                      (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR))) != ERROR_SUCCESS)
                    {
                        RegCloseKey(hSubKey);
                        RegCloseKey(hKey);
                        return;
                    }
                    RegCloseKey(hSubKey);
                }
                lstrcpy(pts, SubPath);
            }

            if (RegSetValueEx(hKey,
                              NewLayout,
                              0,
                              REG_SZ,
                              (LPBYTE)pts,
                              (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR))) == ERROR_SUCCESS)
            {
                UpdateLayoutsList();
            }
        }
        RegCloseKey(hKey);
    }
}

VOID
CreateKeyboardLayoutList(HWND hItemsList)
{
    HKEY hKey;
    PTSTR pstrLayoutID;
    TCHAR szLayoutID[CCH_LAYOUT_ID + 1], KeyName[MAX_PATH];
    DWORD dwIndex = 0;
    DWORD dwSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Keyboard Layouts"), 0, KEY_ENUMERATE_SUB_KEYS, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szLayoutID) / sizeof(TCHAR);

        while (RegEnumKeyEx(hKey, dwIndex, szLayoutID, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            GetLayoutName(szLayoutID, KeyName);

            INT iIndex = (INT) SendMessage(hItemsList, CB_ADDSTRING, 0, (LPARAM)KeyName);

            pstrLayoutID = (PTSTR)HeapAlloc(hProcessHeap, 0, sizeof(szLayoutID));
            lstrcpy(pstrLayoutID, szLayoutID);
            SendMessage(hItemsList, CB_SETITEMDATA, iIndex, (LPARAM)pstrLayoutID);

            // FIXME!
            if (_tcscmp(szLayoutID, _T("00000409")) == 0)
            {
                SendMessage(hItemsList, CB_SETCURSEL, (WPARAM)iIndex, (LPARAM)0);
            }

            dwIndex++;

            dwSize = sizeof(szLayoutID) / sizeof(TCHAR);
        }

        RegCloseKey(hKey);
    }
}

/* Language enumerate procedure */
static BOOL CALLBACK
LanguagesEnumProc(LPTSTR lpLanguage)
{
    LCID Lcid;
    TCHAR Lang[1024];
    INT Index;

    Lcid = _tcstoul(lpLanguage, NULL, 16);

    GetLocaleInfo(Lcid, LOCALE_SLANGUAGE, Lang, sizeof(Lang));
    Index = (INT)SendMessage(hLangList, CB_ADDSTRING,
                             0, (LPARAM)Lang);

    SendMessage(hLangList, CB_SETITEMDATA,
                Index, (LPARAM)Lcid);

    // FIXME!
    if (Lcid == 0x0409)
    {
        SendMessage(hLangList, CB_SELECTSTRING,
                    (WPARAM) -1, (LPARAM)Lang);
    }

    return TRUE;
}

INT_PTR CALLBACK
AddDlgProc(HWND hDlg,
           UINT message,
           WPARAM wParam,
           LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            hLangList = GetDlgItem(hDlg, IDC_INPUT_LANG_COMBO);
            hLayoutList = GetDlgItem(hDlg, IDC_KEYBOARD_LO_COMBO);
            EnumSystemLocales(LanguagesEnumProc, LCID_INSTALLED);
            CreateKeyboardLayoutList(hLayoutList);
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_INPUT_LANG_COMBO:
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        SelectLayoutByLang();
                    }
                }
                break;

                case IDOK:
                {
                    AddNewLayout(hDlg);
                    EndDialog(hDlg, LOWORD(wParam));
                }
                break;

                case IDCANCEL:
                {
                    EndDialog(hDlg, LOWORD(wParam));
                }
            }
        }
        break;

        case WM_DESTROY:
        {
            INT iCount;

            for(iCount = SendMessage(hLayoutList, CB_GETCOUNT, 0, 0); --iCount >= 0;)
                HeapFree(hProcessHeap, 0, (LPVOID)SendMessage(hLayoutList, CB_GETITEMDATA, iCount, 0));
        }
        break;
    }

    return FALSE;
}

/* EOF */
