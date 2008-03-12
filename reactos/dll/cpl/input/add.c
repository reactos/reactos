/*
 *
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/add.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
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
    TCHAR Layout[MAX_PATH], Lang[MAX_PATH], LangID[MAX_PATH];
    INT iIndex;
    LCID Lcid;

    iIndex = SendMessage(hLangList, CB_GETCURSEL, 0, 0);
    Lcid = SendMessage(hLangList, CB_GETITEMDATA, iIndex, 0);

    GetLocaleInfo(MAKELCID(Lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, (WORD*)Lang, sizeof(Lang));

    _stprintf(LangID, _T("0000%s"), Lang);

    if (GetLayoutName(LangID, Layout))
    {
        SendMessage(hLayoutList, CB_SELECTSTRING,
                    (WPARAM) -1, (LPARAM)Layout);
    }
}

static VOID
AddNewLayout(HWND hwndDlg)
{
    TCHAR Lang[MAX_PATH], LangID[MAX_PATH], LayoutID[MAX_PATH];
    INT iLang, iLayout;
    LCID Lcid;

    iLang = SendMessage(hLangList, CB_GETCURSEL, 0, 0);
    iLayout = SendMessage(hLayoutList, CB_GETCURSEL, 0, 0);

    if ((iLang == CB_ERR) || (iLayout == CB_ERR)) return;

    Lcid = (LCID) SendMessage(hLangList, CB_GETITEMDATA, iLang, 0);
    GetLocaleInfo(MAKELCID(Lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, (WORD*)Lang, sizeof(Lang));
    _stprintf(LangID, _T("0000%s"), Lang);

    _tcscpy(LayoutID, (LPTSTR)SendMessage(hLayoutList, CB_GETITEMDATA, iLayout, 0));

    if (_tcscmp(LangID, LayoutID) == 0)
    {
        MessageBox(0, L"", L"", MB_OK);
        HKEY hKey;

        if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_WRITE, &hKey))
        {
            
        }
    }
}

VOID
CreateKeyboardLayoutList(VOID)
{
    HKEY hKey, hSubKey;
    TCHAR szBuf[MAX_PATH], KeyName[MAX_PATH];
    LONG Ret;
    DWORD dwIndex = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, _T("System\\CurrentControlSet\\Control\\Keyboard Layouts"), &hKey) == ERROR_SUCCESS)
    {
        Ret = RegEnumKey(hKey, dwIndex, szBuf, sizeof(szBuf) / sizeof(TCHAR));
        if (Ret == ERROR_SUCCESS)
        {
            while (Ret == ERROR_SUCCESS)
            {
                _stprintf(KeyName, _T("System\\CurrentControlSet\\Control\\Keyboard Layouts\\%s"), szBuf);
                if (RegOpenKey(HKEY_LOCAL_MACHINE, KeyName, &hSubKey) == ERROR_SUCCESS)
                {
                    DWORD Length = MAX_PATH;

                    if (RegQueryValueEx(hSubKey, _T("Layout Text"), NULL, NULL, (LPBYTE)KeyName, &Length) == ERROR_SUCCESS)
                    {
                        UINT iIndex;
                        iIndex = (UINT) SendMessage(hLayoutList, CB_ADDSTRING, 0, (LPARAM)KeyName);

                        SendMessage(hLayoutList, CB_SETITEMDATA, iIndex, (LPARAM)szBuf);

                        // FIXME!
                        if (_tcscmp(szBuf, _T("00000409")) == 0)
                        {
                            SendMessage(hLayoutList, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)KeyName);
                        }

                        dwIndex++;
                        Ret = RegEnumKey(hKey, dwIndex, szBuf, sizeof(szBuf) / sizeof(TCHAR));
                    }
                }

                RegCloseKey(hSubKey);
            }
        }
    }

    RegCloseKey(hKey);
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
            CreateKeyboardLayoutList();
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
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}

/* EOF */
