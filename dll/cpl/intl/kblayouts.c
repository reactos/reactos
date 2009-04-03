/*
 * PROJECT:         ReactOS International Control Panel
 * FILE:            dll/cpl/intl/kblayouts.c
 * PURPOSE:         Functions for manipulation with keyboard layouts
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <setupapi.h>
#include <tchar.h>
#include <stdio.h>

#include "intl.h"
#include "resource.h"

/* Character Count of a layout ID like "00000409" */
#define CCH_LAYOUT_ID    8

/* Maximum Character Count of a ULONG in decimal */
#define CCH_ULONG_DEC    10


/* szLayoutID like 00000409, szLangID like 00000409 */
static BOOL
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

            if (_tcscmp(szOldLangID, szLangID) == 0)
                IsLangExists = TRUE;
            else
                IsLangExists = FALSE;

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
                if ((_tcscmp(szPreload, szLayoutID) == 0) && (IsLangExists))
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

static INT
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

/* szLayoutID like 00000409, szLangID like 00000409 */
static BOOL
AddNewLayout(LPTSTR szLayoutID, LPTSTR szLangID)
{
    TCHAR NewLayout[CCH_ULONG_DEC + 1], Lang[MAX_PATH],
          LangID[CCH_LAYOUT_ID + 1], SubPath[CCH_LAYOUT_ID + 1];
    HKEY hKey, hSubKey;
    DWORD cValues;
    LCID lcid;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Preload"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            _ultot(cValues + 1, NewLayout, 10);

            lcid = _tcstoul(szLangID, NULL, 16);

            GetLocaleInfo(MAKELCID(lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, Lang, sizeof(Lang) / sizeof(TCHAR));
            wsprintf(LangID, _T("0000%s"), Lang);

            if (IsLayoutExists(szLayoutID, LangID))
            {
                RegCloseKey(hKey);
                return FALSE;
            }

            if (GetLayoutCount(Lang) >= 1)
            {
                wsprintf(SubPath, _T("d%03d%s"), GetLayoutCount(Lang), Lang);
            }
            else if ((_tcscmp(LangID, szLayoutID) != 0) && (GetLayoutCount(Lang) == 0))
            {
                wsprintf(SubPath, _T("d%03d%s"), 0, Lang);
            }
            else SubPath[0] = '\0';

            if (_tcslen(SubPath) != 0)
            {
                if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Keyboard Layout\\Substitutes"), 0, NULL,
                                   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                   NULL, &hSubKey, NULL) == ERROR_SUCCESS)
                {
                    if (RegSetValueEx(hSubKey, SubPath, 0, REG_SZ, (LPBYTE)szLayoutID,
                                      (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR))) != ERROR_SUCCESS)
                    {
                        RegCloseKey(hSubKey);
                        RegCloseKey(hKey);
                        return FALSE;
                    }
                    RegCloseKey(hSubKey);
                }
                lstrcpy(szLayoutID, SubPath);
            }

            RegSetValueEx(hKey,
                          NewLayout,
                          0,
                          REG_SZ,
                          (LPBYTE)szLayoutID,
                          (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(TCHAR)));
        }
        RegCloseKey(hKey);
    }

    return TRUE;
}

VOID
AddNewKbLayoutsByLcid(LCID Lcid)
{
    HINF hIntlInf;
    TCHAR szLang[CCH_LAYOUT_ID + 1], szLangID[CCH_LAYOUT_ID + 1];
    TCHAR szLangStr[MAX_STR_SIZE], szLayoutStr[MAX_STR_SIZE], szStr[MAX_STR_SIZE];
    INFCONTEXT InfContext;
    LONG Count;
    DWORD FieldCount, Index;

    GetLocaleInfo(MAKELCID(Lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, szLang, sizeof(szLang) / sizeof(TCHAR));
    wsprintf(szLangID, _T("0000%s"), szLang);

    hIntlInf = SetupOpenInfFile(_T("intl.inf"), NULL, INF_STYLE_WIN4, NULL);

    if (hIntlInf == INVALID_HANDLE_VALUE)
        return;

    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        hIntlInf = NULL;
        return;
    }

    Count = SetupGetLineCount(hIntlInf, _T("Locales"));
    if (Count <= 0) return;

    if (SetupFindFirstLine(hIntlInf, _T("Locales"), szLangID, &InfContext))
    {
        FieldCount = SetupGetFieldCount(&InfContext);

        if (FieldCount != 0)
        {
            for (Index = 5; Index <= FieldCount; Index++)
            {
                if (SetupGetStringField(&InfContext, Index, szStr, MAX_STR_SIZE, NULL))
                {
                    INT i, j;

                    if (_tcslen(szStr) != 13) continue;

                    wsprintf(szLangStr, _T("0000%s"), szStr);
                    szLangStr[8] = '\0';

                    for (i = 5, j = 0; i <= _tcslen(szStr); i++, j++)
                        szLayoutStr[j] = szStr[i];

                    AddNewLayout(szLayoutStr, szLangStr);
                }
            }
        }
    }

    SetupCloseInfFile(hIntlInf);
}
