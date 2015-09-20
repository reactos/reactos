/*
 * PROJECT:         ReactOS International Control Panel
 * FILE:            dll/cpl/intl/kblayouts.c
 * PURPOSE:         Functions for manipulation with keyboard layouts
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "intl.h"

/* Character Count of a layout ID like "00000409" */
#define CCH_LAYOUT_ID    8

/* Maximum Character Count of a ULONG in decimal */
#define CCH_ULONG_DEC    10


/* szLayoutID like 00000409, szLangID like 00000409 */
static BOOL
IsLayoutExists(PWSTR szLayoutID, PWSTR szLangID)
{
    HKEY hKey, hSubKey;
    WCHAR szPreload[CCH_LAYOUT_ID + 1], szLayoutNum[3 + 1],
          szTmp[CCH_LAYOUT_ID + 1], szOldLangID[CCH_LAYOUT_ID + 1];
    DWORD dwIndex = 0, dwType, dwSize;
    BOOL IsLangExists = FALSE;
    LANGID langid;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload",
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szLayoutNum);

        while (RegEnumValueW(hKey, dwIndex, szLayoutNum, &dwSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szPreload);
            if (RegQueryValueExW(hKey, szLayoutNum, NULL, NULL, (LPBYTE)szPreload, &dwSize) != ERROR_SUCCESS)
            {
                RegCloseKey(hKey);
                return FALSE;
            }

            langid = (LANGID)wcstoul(szPreload, NULL, 16);
            GetLocaleInfoW(langid, LOCALE_ILANGUAGE, szTmp, sizeof(szTmp) / sizeof(WCHAR));
            wsprintf(szOldLangID, L"0000%s", szTmp);

            if (wcscmp(szOldLangID, szLangID) == 0)
                IsLangExists = TRUE;
            else
                IsLangExists = FALSE;

            if (szPreload[0] == 'd')
            {
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes",
                                 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(szTmp);
                    RegQueryValueExW(hSubKey, szPreload, NULL, NULL, (LPBYTE)szTmp, &dwSize);

                    if ((wcscmp(szTmp, szLayoutID) == 0)&&(IsLangExists))
                    {
                        RegCloseKey(hSubKey);
                        RegCloseKey(hKey);
                        return TRUE;
                    }
                }
            }
            else
            {
                if ((wcscmp(szPreload, szLayoutID) == 0) && (IsLangExists))
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
GetLayoutCount(PWSTR szLang)
{
    HKEY hKey;
    WCHAR szLayoutID[3 + 1], szPreload[CCH_LAYOUT_ID + 1], szLOLang[MAX_PATH];
    DWORD dwIndex = 0, dwType, dwSize;
    UINT Count = 0, i, j;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload",
        0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szLayoutID);

        while (RegEnumValue(hKey, dwIndex, szLayoutID, &dwSize, NULL, &dwType, NULL, NULL) == ERROR_SUCCESS)
        {
            dwSize = sizeof(szPreload);
            RegQueryValueExW(hKey, szLayoutID, NULL, NULL, (LPBYTE)szPreload, &dwSize);

            for (i = 4, j = 0; i < wcslen(szPreload)+1; i++, j++)
                szLOLang[j] = szPreload[i];

            if (wcscmp(szLOLang, szLang) == 0) Count += 1;

            dwSize = sizeof(szLayoutID);
            dwIndex++;
        }

        RegCloseKey(hKey);
    }

    return Count;
}

/* szLayoutID like 00000409, szLangID like 00000409 */
static BOOL
AddNewLayout(PWSTR szLayoutID, PWSTR szLangID)
{
    WCHAR NewLayout[CCH_ULONG_DEC + 1], Lang[MAX_PATH],
          LangID[CCH_LAYOUT_ID + 1], SubPath[CCH_LAYOUT_ID + 1];
    HKEY hKey, hSubKey;
    DWORD cValues;
    LCID lcid;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            _ultow(cValues + 1, NewLayout, 10);

            lcid = wcstoul(szLangID, NULL, 16);

            GetLocaleInfoW(MAKELCID(lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, Lang, sizeof(Lang) / sizeof(WCHAR));
            wsprintf(LangID, L"0000%s", Lang);

            if (IsLayoutExists(szLayoutID, LangID))
            {
                RegCloseKey(hKey);
                return FALSE;
            }

            if (GetLayoutCount(Lang) >= 1)
            {
                wsprintf(SubPath, L"d%03d%s", GetLayoutCount(Lang), Lang);
            }
            else if ((wcscmp(LangID, szLayoutID) != 0) && (GetLayoutCount(Lang) == 0))
            {
                wsprintf(SubPath, L"d%03d%s", 0, Lang);
            }
            else SubPath[0] = L'\0';

            if (wcslen(SubPath) != 0)
            {
                if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0, NULL,
                                    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                    NULL, &hSubKey, NULL) == ERROR_SUCCESS)
                {
                    if (RegSetValueExW(hSubKey, SubPath, 0, REG_SZ, (LPBYTE)szLayoutID,
                                       (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(WCHAR))) != ERROR_SUCCESS)
                    {
                        RegCloseKey(hSubKey);
                        RegCloseKey(hKey);
                        return FALSE;
                    }
                    RegCloseKey(hSubKey);
                }
                lstrcpy(szLayoutID, SubPath);
            }

            RegSetValueExW(hKey,
                           NewLayout,
                           0,
                           REG_SZ,
                           (LPBYTE)szLayoutID,
                           (DWORD)((CCH_LAYOUT_ID + 1) * sizeof(WCHAR)));
        }
        RegCloseKey(hKey);
    }

    return TRUE;
}

VOID
AddNewKbLayoutsByLcid(LCID Lcid)
{
    HINF hIntlInf;
    WCHAR szLang[CCH_LAYOUT_ID + 1], szLangID[CCH_LAYOUT_ID + 1];
    WCHAR szLangStr[MAX_STR_SIZE], szLayoutStr[MAX_STR_SIZE], szStr[MAX_STR_SIZE];
    INFCONTEXT InfContext;
    LONG Count;
    DWORD FieldCount, Index;

    GetLocaleInfoW(MAKELCID(Lcid, SORT_DEFAULT), LOCALE_ILANGUAGE, szLang, sizeof(szLang) / sizeof(WCHAR));
    wsprintf(szLangID, L"0000%s", szLang);

    hIntlInf = SetupOpenInfFileW(L"intl.inf", NULL, INF_STYLE_WIN4, NULL);

    if (hIntlInf == INVALID_HANDLE_VALUE)
        return;

    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        hIntlInf = NULL;
        return;
    }

    Count = SetupGetLineCount(hIntlInf, L"Locales");
    if (Count <= 0) return;

    if (SetupFindFirstLine(hIntlInf, L"Locales", szLangID, &InfContext))
    {
        FieldCount = SetupGetFieldCount(&InfContext);

        if (FieldCount != 0)
        {
            for (Index = 5; Index <= FieldCount; Index++)
            {
                if (SetupGetStringField(&InfContext, Index, szStr, MAX_STR_SIZE, NULL))
                {
                    INT i, j;

                    if (wcslen(szStr) != 13) continue;

                    wsprintf(szLangStr, L"0000%s", szStr);
                    szLangStr[8] = L'\0';

                    for (i = 5, j = 0; i <= wcslen(szStr); i++, j++)
                        szLayoutStr[j] = szStr[i];

                    AddNewLayout(szLayoutStr, szLangStr);
                }
            }
        }
    }
    SetupCloseInfFile(hIntlInf);
}
