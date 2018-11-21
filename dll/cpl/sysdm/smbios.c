/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/smbios.c
 * PURPOSE:     Retrieve device or motherboard name identifier from DMI/SMBIOS
 * COPYRIGHT:   Copyright 2018 Stanislav Motylkov <x86corez@gmail.com>
 *
 */

#include "precomp.h"

#include <strsafe.h>
#include <udmihelp.h>

typedef struct GENERIC_NAME
{
    PCWSTR pwName;
    BOOL bCaseSensitive;
} GENERIC_NAME;

typedef struct VENDOR_LONG_NAME
{
    PCWSTR pwLongName;
    PCWSTR pwShortName;
} VENDOR_LONG_NAME;

typedef struct REDUNDANT_WORD
{
    PCWSTR pwStr;
    BOOL bReplaceFirstWord;
} REDUNDANT_WORD;

static
BOOL
IsPunctuation(
    _In_ WCHAR chr)
{
    return (chr <= L' ' || chr == L'.' || chr == L',');
}

/*
 * Trim redundant characters
 */
static
VOID
TrimPunctuation(
    _Inout_ PWSTR pStr)
{
    SIZE_T Length;
    UINT i = 0;

    if (!pStr)
        return;

    Length = wcslen(pStr);
    if (Length == 0)
        return;

    /* Trim leading characters */
    while (i < Length && IsPunctuation(pStr[i]))
    {
        i++;
    }

    if (i > 0)
    {
        Length -= i;
        memmove(pStr, pStr + i, (Length + 1) * sizeof(WCHAR));
    }

    /* Trim trailing characters */
    while (Length && IsPunctuation(pStr[Length-1]))
    {
        pStr[Length-1] = L'\0';
        --Length;
    }
}

/*
 * Case insensitive variant of wcsstr
 */
static
wchar_t * wcsistr(const wchar_t *s, const wchar_t *b)
{
    wchar_t *x;
    wchar_t *y;
    wchar_t *c;
    x = (wchar_t *)s;
    while (*x)
    {
        if (towlower(*x) == towlower(*b))
        {
            y = x;
            c = (wchar_t *)b;
            while (*y && *c && towlower(*y) == towlower(*c))
            {
                c++;
                y++;
            }
            if (!*c)
                return x;
        }
        x++;
    }
    return NULL;
}

static
wchar_t * wcsistr_plus(const wchar_t *s, wchar_t *b)
{
    wchar_t * result = wcsistr(s, b);
    UINT len = wcslen(b);
    // workarounds
    if (!result && b[len - 1] == L' ' && wcschr(s, L',') != NULL)
    {
        b[len - 1] = L',';
        result = wcsistr(s, b);
        b[len - 1] = L' ';
        if (!result)
        {
            b[0] = L',';
            result = wcsistr(s, b);
            b[0] = L' ';
        }
    }
    if (!result && b[len - 1] == L' ' && wcschr(s, L'(') != NULL)
    {
        b[len - 1] = L'(';
        result = wcsistr(s, b);
        b[len - 1] = L' ';
    }
    if (!result && b[len - 1] == L' ' && wcschr(s, L'_') != NULL)
    {
        b[0] = L'_';
        result = wcsistr(s, b);
        b[0] = L' ';
    }
    if (!result && b[0] == L' ' && b[len - 1] == L' ' && wcschr(s, L')') != NULL)
    {
        b[0] = L')';
        result = wcsistr(s, b);
        b[0] = L' ';
    }
    return result;
}

/*
 * Replaces full word with another shorter word
 */
static
VOID wcsrep(
    _Inout_ PWSTR pwStr,
    _In_ PCWSTR pwFind,
    _In_ PCWSTR pwReplace,
    _In_ BOOL bReplaceFirstWord)
{
    PWSTR pwsStr, pwsFind, pwsReplace, pwsBuf = NULL;
    SIZE_T lenStr;
    SIZE_T lenFind;
    SIZE_T lenReplace;

    if (!pwStr || !pwFind || !pwReplace ||
        wcslen(pwStr) == 0 ||
        wcslen(pwFind) == 0 ||
        wcslen(pwFind) < wcslen(pwReplace))
    {
        return;
    }
    lenStr = wcslen(pwStr) + 2 + 1;
    lenFind = wcslen(pwFind) + 2 + 1;
    lenReplace = wcslen(pwReplace) + 2 + 1;

    pwsStr = HeapAlloc(GetProcessHeap(), 0, lenStr * sizeof(WCHAR));
    if (!pwsStr)
    {
        return;
    }
    StringCchCopyW(pwsStr, lenStr, L" ");
    StringCchCatW(pwsStr, lenStr, pwStr);
    StringCchCatW(pwsStr, lenStr, L" ");

    pwsFind = HeapAlloc(GetProcessHeap(), 0, lenFind * sizeof(WCHAR));
    if (!pwsFind)
    {
        goto freeStr;
    }
    StringCchCopyW(pwsFind, lenFind, L" ");
    StringCchCatW(pwsFind, lenFind, pwFind);
    StringCchCatW(pwsFind, lenFind, L" ");

    if (!(pwsBuf = wcsistr_plus(pwsStr, pwsFind)))
    {
        goto freeFind;
    }
    if (!bReplaceFirstWord && pwsBuf - pwsStr < 2)
    {
        goto freeFind;
    }

    pwsReplace = HeapAlloc(GetProcessHeap(), 0, lenReplace * sizeof(WCHAR));
    if (!pwsReplace)
    {
        goto freeFind;
    }
    StringCchCopyW(pwsReplace, lenReplace, L" ");
    StringCchCatW(pwsReplace, lenReplace, pwReplace);
    StringCchCatW(pwsReplace, lenReplace, L" ");

    do
    {
        // replace substring
        memmove(pwsBuf, pwsReplace, (lenReplace - 1) * sizeof(WCHAR));
        // shift characters
        memmove(pwsBuf + lenReplace - (wcslen(pwReplace) > 0 ? 1 : 2), pwsBuf + lenFind - 1, (lenStr - lenFind - (pwsBuf - pwsStr) + 1) * sizeof(WCHAR));
    }
    while ((pwsBuf = wcsistr_plus(pwsStr, pwsFind)) != NULL);

    TrimDmiStringW(pwsStr);
    StringCchCopyW(pwStr, wcslen(pwStr), pwsStr);

    HeapFree(GetProcessHeap(), 0, pwsReplace);
freeFind:
    HeapFree(GetProcessHeap(), 0, pwsFind);
freeStr:
    HeapFree(GetProcessHeap(), 0, pwsStr);
}

static
BOOL IsGenericSystemName(PCWSTR ven, PCWSTR dev)
{
    static const GENERIC_NAME Vendors[] =
    {
        { L"To Be Filled By O.E.M.", FALSE }, // some ASUS boards
        { L"System manufacturer", TRUE },     // some ASUS boards
        { L"Default string", TRUE },          // some Gigabyte boards
        { L"LTD Delovoy Office", TRUE },      // some Gigabyte boards
        { L"O.E.M", TRUE },                   // some AMD boards
        { L"DEPO Computers", TRUE },          // various boards
    };
    static const GENERIC_NAME Devices[] =
    {
        { L"To Be Filled By O.E.M.", FALSE }, // some ASUS boards
        { L"All Series", TRUE },              // some ASUS boards
        { L"System Product Name", TRUE },     // some ASUS boards
        { L"Default string", TRUE },          // some Gigabyte boards
        { L"Please change product name", TRUE }, // some MSI boards
        { L"Computer", TRUE },                // some Intel boards
        { L"ChiefRiver Platform", TRUE },     // some Intel boards
        { L"SharkBay Platform", TRUE },       // some Intel boards
        { L"HuronRiver Platform", TRUE },     // some Intel boards
        { L"SandyBridge Platform", TRUE },    // some Intel boards
        { L"Broadwell Platform", TRUE },      // some LG boards
        { L"Sabine Platform", TRUE },         // some AMD boards
        { L"O.E.M", TRUE },                   // some AMD boards
        { L"*", TRUE },                       // various boards
        { L"GEG", TRUE },                     // various boards
        { L"OEM", TRUE },                     // various boards
        { L"DEPO Computers", TRUE },          // various boards
        { L"Aquarius Pro, Std, Elt Series", TRUE }, // some Foxconn boards
        { L"Aquarius Server", TRUE },         // some ASUS server boards
        { L"Aquarius Server G2", TRUE },      // some ASUS server boards
        { L"Super Server", TRUE },            // some Supermicro server boards
        { L"POSITIVO MOBILE", FALSE },        // some Positivo devices
    };
    BOOL bMatch;
    UINT i;

    for (i = 0; i < _countof(Vendors); i++)
    {
        if (!ven)
        {
            break;
        }
        if (Vendors[i].bCaseSensitive)
        {
            bMatch = !wcscmp(ven, Vendors[i].pwName);
        }
        else
        {
            bMatch = !wcsicmp(ven, Vendors[i].pwName);
        }
        if (bMatch)
        {
            return TRUE;
        }
    }

    for (i = 0; i < _countof(Devices); i++)
    {
        if (!dev)
        {
            break;
        }
        if (Devices[i].bCaseSensitive)
        {
            bMatch = !wcscmp(dev, Devices[i].pwName);
        }
        else
        {
            bMatch = !wcsicmp(dev, Devices[i].pwName);
        }
        if (bMatch)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static
void AppendSystemFamily(PWSTR pBuf, SIZE_T cchBuf, PCHAR * DmiStrings, PWSTR dev)
{
    static const PCSTR KnownFamilies[] =
    {
        "Eee PC",      // ASUS
        "IdeaPad",     // Lenovo
        "IdeaCentre",  // Lenovo
    };
    static const PCWSTR Aliases[] =
    {
        NULL,
        NULL,
        L"IdeaCenter",
    };
    UINT i;
    WCHAR wideStr[128];

    for (i = 0; i < _countof(KnownFamilies); i++)
    {
        StringCchPrintfW(wideStr, _countof(wideStr), L"%S", KnownFamilies[i]);

        if (wcsistr(dev, wideStr) == NULL &&
            (!Aliases[i] || wcsistr(dev, Aliases[i]) == NULL) &&
            !stricmp(DmiStrings[SYS_FAMILY], KnownFamilies[i]))
        {
            if (wcslen(pBuf) > 0 && wcslen(dev) > 0)
            {
                StringCchCatW(pBuf, cchBuf, L" ");
            }
            StringCchCatW(pBuf, cchBuf, wideStr);
        }
    }
}

BOOL GetSystemName(PWSTR pBuf, SIZE_T cchBuf)
{
    static const VENDOR_LONG_NAME LongNames[] =
    {
        { L"ASUSTeK", L"ASUS" },
        { L"First International Computer", L"FIC" },
        { L"Hewlett-Packard", L"HP" },
        { L"MICRO-STAR", L"MSI" },
        { L"SGI.COM", L"SGI" },
        { L"Silicon Graphics International", L"SGI" },
        { L"InformationComputerSystems", L"ICS" },
        { L"CHUWI INNOVATION AND TECHNOLOGY", L"CHUWI" },
        { L"http://www.abit.com.tw/", L"ABIT" },
        { L"www.abit.com.tw", L"ABIT" },
        { L"Colorful Technology And Development", L"Colorful" },
        { L"HaierComputer", L"Haier" },
    };
    static const REDUNDANT_WORD RedundantWords[] =
    {
        { L"Corporation", FALSE },
        { L"Computer", FALSE },
        { L"Computers", FALSE },
        { L"Group", FALSE },
        { L"Cloud", FALSE },
        { L"Center", FALSE },
        { L"Systems", FALSE },
        { L"Microsystems", FALSE },
        { L"Infosystems", FALSE },
        { L"Electronics", FALSE },
        { L"Electric", FALSE },
        { L"Software", FALSE },
        { L"International", FALSE },
        { L"Interantonal", FALSE }, // on purpose (some MSI boards)
        { L"Industrial", FALSE },
        { L"Information", FALSE },
        { L"Technology", FALSE },
        { L"Tecohnology", FALSE }, // on purpose (some Gigabyte boards)
        { L"Technologies", FALSE },
        { L"Limited", FALSE },
        { L"Int", FALSE },
        { L"Inc", FALSE },
        { L"Co", FALSE },
        { L"Corp", FALSE },
        { L"Crop", FALSE },
        { L"Ltd", FALSE },
        { L"GmbH", FALSE },
        { L"S.p.A", FALSE },
        { L"S.A", FALSE },
        { L"SA", FALSE },
        { L"SAS", FALSE },
        { L"BV", FALSE },
        { L"AG", FALSE },
        { L"OOO", TRUE },
        { L"CJSC", FALSE },
        { L"INT'L", FALSE },
        { L"plc", FALSE },
    };
    PVOID SMBiosBuf;
    PCHAR DmiStrings[ID_STRINGS_MAX] = { 0 };
    WCHAR ven[512], dev[512];
    BOOL bGenericName;
    UINT i;
    PWCHAR j;

    SMBiosBuf = LoadSMBiosData(DmiStrings);
    if (!SMBiosBuf)
    {
        return FALSE;
    }

    GetSMBiosStringW(DmiStrings[SYS_VENDOR], ven, _countof(ven), TRUE);
    GetSMBiosStringW(DmiStrings[SYS_PRODUCT], dev, _countof(dev), TRUE);
    bGenericName = IsGenericSystemName(ven, dev);

    if (wcslen(dev) == 0 ||
        !wcscmp(dev, ven) ||
        bGenericName)
    {
        // system strings are unusable, use board strings
        if (DmiStrings[BOARD_VENDOR] != NULL || !bGenericName)
        {
            if ((DmiStrings[BOARD_VENDOR] &&
                strlen(DmiStrings[BOARD_VENDOR]) >= 2 &&
                strstr(DmiStrings[BOARD_VENDOR], "  ") != DmiStrings[BOARD_VENDOR]) ||
                IsGenericSystemName(ven, NULL))
            {
                GetSMBiosStringW(DmiStrings[BOARD_VENDOR], ven, _countof(ven), TRUE);
            }
            GetSMBiosStringW(DmiStrings[BOARD_NAME], dev, _countof(dev), TRUE);

            if (IsGenericSystemName(ven, NULL))
            {
                *ven = 0;
            }
            if (IsGenericSystemName(NULL, dev))
            {
                *dev = 0;
            }
            if (wcslen(dev) == 0 &&
                DmiStrings[SYS_VERSION] != NULL)
            {
                GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);

                if (IsGenericSystemName(NULL, dev))
                {
                    *dev = 0;
                }
            }
            if (wcslen(dev) == 0 &&
                DmiStrings[BOARD_VERSION] != NULL)
            {
                GetSMBiosStringW(DmiStrings[BOARD_VERSION], dev, _countof(dev), TRUE);

                if (IsGenericSystemName(NULL, dev))
                {
                    *dev = 0;
                }
            }
        }

        if (wcslen(ven) == 0 && wcslen(dev) == 0)
        {
            // board strings are empty, use BIOS vendor string
            GetSMBiosStringW(DmiStrings[BIOS_VENDOR], ven, _countof(ven), TRUE);
        }
    }
    else
    {
        if (wcslen(ven) < 2)
        {
            GetSMBiosStringW(DmiStrings[BOARD_VENDOR], ven, _countof(ven), TRUE);

            if (IsGenericSystemName(ven, NULL))
            {
                *ven = 0;
            }
        }
    }

    // workaround for LORD ELECTRONICS
    if (((j = wcsstr(ven, L" ")) != NULL) && (j - ven > 2))
    {
        i = j - ven;
        if (!wcsncmp(ven + wcslen(ven) - i, ven, i))
        {
            ven[wcslen(ven) - i] = L'\0';
        }
    }

    // make vendor strings shorter
    for (i = 0; i < _countof(LongNames); i++)
    {
        wcsrep(ven, LongNames[i].pwLongName, LongNames[i].pwShortName, TRUE);
    }

    // remove redundant words
    for (i = 0; i < _countof(RedundantWords); i++)
    {
        wcsrep(ven, RedundantWords[i].pwStr, L"", RedundantWords[i].bReplaceFirstWord);
    }
    for (i = 0; i < _countof(RedundantWords); i++)
    {
        StringCchCopyW(pBuf, cchBuf, RedundantWords[i].pwStr);
        StringCchCatW(pBuf, cchBuf, L".");
        wcsrep(ven, pBuf, L"", RedundantWords[i].bReplaceFirstWord);
    }

    // workaround for LENOVO notebooks
    if (!wcscmp(ven, L"LENOVO"))
    {
        StringCchCopyW(ven, _countof(ven), L"Lenovo");

        if (stricmp(DmiStrings[SYS_VERSION], "Lenovo") &&
            stricmp(DmiStrings[SYS_VERSION], "Lenovo Product") &&
            stricmp(DmiStrings[SYS_VERSION], " ") &&
            _strnicmp(DmiStrings[SYS_VERSION], "   ", 3) &&
            wcsistr(dev, L"IdeaPad ") == NULL &&
            wcsistr(dev, L"ThinkServer ") == NULL)
        {
            GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);
        }

        if (wcsstr(dev, L"Lenovo-") == dev)
        {
            // replace "-" with space
            dev[6] = L' ';
        }

        if (!wcscmp(dev, L"Lenovo"))
        {
            GetSMBiosStringW(DmiStrings[BOARD_NAME], dev, _countof(dev), TRUE);
        }
    }
    if (!wcscmp(ven, L"IBM") &&
        DmiStrings[SYS_VERSION] != NULL &&
        strstr(DmiStrings[SYS_VERSION], "ThinkPad ") != NULL)
    {
        GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);
    }

    // workaround for DEXP
    if (!wcscmp(ven, L"DEXP"))
    {
        if (!stricmp(DmiStrings[SYS_PRODUCT], "Tablet PC")
            && DmiStrings[SYS_VERSION] != NULL)
        {
            GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);
        }
    }

    // workaround for Razer Blade
    if (!wcscmp(ven, L"Razer") && !wcscmp(dev, L"Blade"))
    {
        if (DmiStrings[SYS_VERSION] != NULL)
        {
            StringCchCopyW(ven, _countof(ven), L"Razer Blade");
            GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);
        }
    }

    // workaround for MSI motherboards
    if (!wcscmp(ven, L"MSI") &&
        wcsstr(dev, L"MS-") != NULL &&
        DmiStrings[BOARD_NAME] != NULL &&
        strstr(DmiStrings[BOARD_NAME], "(MS-") != NULL)
    {
        GetSMBiosStringW(DmiStrings[BOARD_NAME], dev, _countof(dev), TRUE);
    }
    if (wcslen(ven) == 0 &&
        wcsstr(dev, L"MS-") == dev)
    {
        StringCchCopyW(ven, _countof(ven), L"MSI");
    }

    // trim redundant characters
    TrimPunctuation(ven);
    TrimPunctuation(dev);

    if (wcsistr(dev, ven) == dev ||
        (!wcscmp(ven, L"ASUS") && wcsstr(dev, L"ASUS") != NULL) ||
        (!wcscmp(ven, L"HP") && wcsstr(dev, L" by HP") != NULL))
    {
        // device string contains vendor string, use second only
        StringCchCopyW(pBuf, cchBuf, dev);
    }
    else
    {
        StringCchCopyW(pBuf, cchBuf, ven);
        AppendSystemFamily(pBuf, cchBuf, DmiStrings, dev);
        if (wcslen(pBuf) > 0 && wcslen(dev) > 0)
        {
            StringCchCatW(pBuf, cchBuf, L" ");
        }
        StringCchCatW(pBuf, cchBuf, dev);
    }

    FreeSMBiosData(SMBiosBuf);

    return (wcslen(pBuf) > 0);
}
