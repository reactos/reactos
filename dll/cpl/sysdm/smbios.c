/*
 * PROJECT:     ReactOS System Control Panel Applet
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/cpl/sysdm/smbios.c
 * PURPOSE:     Retrieve device or motherboard name identifier from DMI/SMBIOS
 * COPYRIGHT:   Copyright 2018-2020 Stanislav Motylkov <x86corez@gmail.com>
 *
 */

#include "precomp.h"

#include <udmihelp.h>

typedef struct GENERIC_NAME
{
    PCWSTR pwName;
    BOOL bCaseSensitive;
    BOOL bRemove;
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

static
BOOL IsDigitStrA(PCHAR DmiString)
{
    PCHAR c = DmiString;
    if (!c)
    {
        return FALSE;
    }
    while (*c)
    {
        if (*c >= '0' && *c <= '9')
        {
            c++;
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
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
        if (!result && wcschr(s, L'.'))
        {
            b[len - 1] = L'.';
            result = wcsistr(s, b);
            b[len - 1] = L' ';
        }
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
BOOL IsGenericSystemName(PCWSTR ven, PCWSTR dev, BOOL * bRemove)
{
    static const GENERIC_NAME Vendors[] =
    {
        // some ASUS boards
        { L"To Be Filled By O.E.M.", FALSE, TRUE },
        { L"To Be Filled By O.E.M", FALSE, TRUE },
        { L"System manufacturer", FALSE, TRUE },
        // some Gigabyte boards
        { L"Default string", TRUE, TRUE },
        { L"LTD Delovoy Office", TRUE, FALSE },
        { L"Motherboard by ZOTAC", TRUE, FALSE },
        // various boards
        { L"Type2 - Board Manufacturer", TRUE, TRUE },
        { L"Type2 - Board Vendor Name1", TRUE, TRUE },
        { L"BASE_BOARD_MANUFACTURER", TRUE, TRUE },
        { L"$(DEFAULT_STRING)", TRUE, TRUE },
        { L"DEPO Computers", TRUE, FALSE },
        { L"-", TRUE, TRUE },
        { L"N/A", TRUE, TRUE },
        { L"OEM", TRUE, TRUE },
        { L"O.E.M", TRUE, TRUE },
        { L"empty", TRUE, TRUE },
        { L"insyde", TRUE, FALSE },
        { L"Unknow", TRUE, TRUE },
        { L"Not Applicable", TRUE, TRUE },
        // distinguish between Oracle and older VirtualBox releases (Sun, etc.)
        { L"innotek GmbH", TRUE, FALSE },
    };
    static const GENERIC_NAME Devices[] =
    {
        // some ASUS boards
        { L"To Be Filled By O.E.M.", FALSE, TRUE },
        { L"To Be Filled By O.E.M", FALSE, TRUE },
        { L"All Series", TRUE, TRUE },
        { L"System Product Name", TRUE, TRUE },
        { L"System Name", TRUE, TRUE },
        // some Gigabyte boards
        { L"Default string", TRUE, TRUE },
        // some MSI boards
        { L"Please change product name", TRUE, TRUE },
        // some Intel boards
        { L"Computer", TRUE, TRUE },
        { L"ChiefRiver Platform", TRUE, FALSE },
        { L"OakTrail Platform", TRUE, FALSE },
        { L"SharkBay Platform", TRUE, FALSE },
        { L"HuronRiver Platform", TRUE, FALSE },
        { L"SandyBridge Platform", TRUE, FALSE },
        { L"Broadwell Platform", TRUE, FALSE },
        { L"Kabylake Platform", TRUE, FALSE },
        { L"Sabine Platform", TRUE, FALSE },
        // various boards
        { L"Base Board Product Name", TRUE, TRUE },
        { L"Base Board Version", TRUE, TRUE },
        { L"Type2 - Board Product Name1", TRUE, TRUE },
        { L"Type2 - Board Product Name", TRUE, TRUE },
        { L"Type2 - Board Version", TRUE, TRUE },
        { L"MODEL_NAME", TRUE, TRUE },
        { L"$(DEFAULT_STRING)", TRUE, TRUE },
        { L"*", TRUE, TRUE },
        { L"T", TRUE, TRUE },
        { L"GEG", TRUE, TRUE },
        { L"N/A", TRUE, TRUE },
        { L"---", TRUE, TRUE },
        { L"OEM", TRUE, TRUE },
        { L"INVA", TRUE, TRUE },
        { L"O.E.M", TRUE, TRUE },
        { L"empty", TRUE, TRUE },
        { L"DNSNB", TRUE, FALSE },
        { L"12345", TRUE, FALSE },
        { L"``````", TRUE, TRUE },
        { L"Uknown", TRUE, TRUE },
        { L"Desktop", FALSE, TRUE },
        { L"Invalid", FALSE, TRUE },
        { L"Reserved", TRUE, TRUE },
        { L"Not Applicable", TRUE, TRUE },
        { L"HaierComputer", TRUE, FALSE },
        { L"DEPO Computers", TRUE, FALSE },
        { L"InsydeH2O EFI BIOS", TRUE, TRUE },
        { L"HP All-in-One", TRUE, FALSE },
        { L"MP Server", TRUE, FALSE },
        { L"0000000000", TRUE, TRUE },
        // some Foxconn boards
        { L"Aquarius Pro, Std, Elt Series", TRUE, FALSE },
        // some ASUS server boards
        { L"Aquarius Server", TRUE, FALSE },
        { L"Aquarius Server G2", TRUE, FALSE },
        // some Supermicro server boards
        { L"Super Server", TRUE, FALSE },
        // some Positivo devices
        { L"POSITIVO MOBILE", FALSE, FALSE },
    };
    BOOL bMatch;
    UINT i;

    if (bRemove)
    {
        *bRemove = FALSE;
    }
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
            bMatch = !_wcsicmp(ven, Vendors[i].pwName);
        }
        if (bMatch)
        {
            if (bRemove)
            {
                *bRemove = Vendors[i].bRemove;
            }
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
            bMatch = !_wcsicmp(dev, Devices[i].pwName);
        }
        if (bMatch)
        {
            if (bRemove)
            {
                *bRemove = Devices[i].bRemove;
            }
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
        "ThinkPad",    // Lenovo
        "IdeaPad",     // Lenovo
        "IdeaCentre",  // Lenovo
    };
    static const PCWSTR Aliases[] =
    {
        NULL,
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
            DmiStrings[SYS_FAMILY] != NULL &&
            !_stricmp(DmiStrings[SYS_FAMILY], KnownFamilies[i]))
        {
            if (wcslen(pBuf) > 0 && wcslen(dev) > 0)
            {
                StringCchCatW(pBuf, cchBuf, L" ");
            }
            StringCchCatW(pBuf, cchBuf, wideStr);
        }
    }
}

static
BOOL TrimNonPrintable(PCHAR DmiString)
{
    PCHAR c = DmiString;
    if (!c)
    {
        return FALSE;
    }
    while (*c)
    {
        if (*c >= 0x20 && *c <= 0x7e)
        {
            c++;
        }
        else
        {
            *c = 0;
            return TRUE;
        }
    }
    return FALSE;
}

/* TrimNonPrintable function wrapper. It does special preprocessing
 * so the function returns FALSE in some corner cases, making the parser
 * use system strings anyway (instead of board strings). */
static
BOOL TrimNonPrintableProd(PCHAR DmiString)
{
    PCHAR c;

    if (!DmiString)
    {
        return FALSE;
    }

    /* Special handling for HP with broken revision */
    c = strstr(DmiString, "(\xFF\xFF");
    if (c > DmiString)
    {
        *c = 0;
    }

    return TrimNonPrintable(DmiString);
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
        { L"Intel(R) Client Systems", L"Intel" },
        { L"InformationComputerSystems", L"ICS" },
        { L"Bernecker + Rainer  Industrie-Elektronik", L"Bernecker & Rainer" },
        { L"CHUWI INNOVATION AND TECHNOLOGY", L"CHUWI" },
        { L"CHUWI INNOVATION LIMITED", L"CHUWI" },
        { L"CHUWI  INNOVATION  LIMITED", L"CHUWI" },
        { L"http://www.abit.com.tw/", L"ABIT" },
        { L"http:\\\\www.abit.com.tw", L"ABIT" },
        { L"www.abit.com.tw", L"ABIT" },
        { L"CASPER BILGISAYAR SISTEMLERI A.S", L"Casper" },
        { L"Colorful Technology And Development", L"Colorful" },
        { L"Colorful Yu Gong Technology And Development", L"Colorful Yu Gong" },
        { L"HaierComputer", L"Haier" },
        { L"Haier Information Technology (Shen Zhen)", L"Haier" },
        { L"HASEECOMPUTERS", L"Hasee" },
        { L"HELIOS BUSINESS COMPUTER", L"HELIOS" },
        { L"Shanghai Zongzhi InfoTech", L"Zongzhi" },
        { L"TSING HUA TONGFANG CO.,LTD", L"TSINGHUA TONGFANG" },
        { L"Yeston Digital Technology Co.,LTD", L"Yeston" },
    };
    static const REDUNDANT_WORD RedundantWords[] =
    {
        { L"Corporation", FALSE },
        { L"Communication", FALSE },
        { L"Computer", FALSE },
        { L"Computers", FALSE },
        { L"Group", FALSE },
        { L"Cloud", FALSE },
        { L"Center", FALSE },
        { L"Systems", FALSE },
        { L"Microsystems", FALSE },
        { L"Infosystems", FALSE },
        { L"Digital", FALSE },
        { L"Electronics", FALSE },
        { L"Electric", FALSE },
        { L"Elektronik", FALSE },
        { L"Software", FALSE },
        { L"Foundation", FALSE },
        { L"International", FALSE },
        { L"Interantonal", FALSE }, // on purpose (some MSI boards)
        { L"INTERANTIONAL", FALSE }, // on purpose (some MSI boards)
        { L"Industrial", FALSE },
        { L"Industrie", FALSE },
        { L"Information", FALSE },
        { L"Informatica", FALSE },
        { L"Produkte", FALSE },
        { L"Technology", FALSE },
        { L"Tecohnology", FALSE }, // on purpose (some Gigabyte boards)
        { L"Technologies", FALSE },
        { L"Tecnologia", FALSE },
        { L"Limited", FALSE },
        { L"Int", FALSE },
        { L"Inc", FALSE },
        { L"Co", FALSE },
        { L"Corp", FALSE },
        { L"Crop", FALSE },
        { L"LLC", FALSE },
        { L"Ltd", FALSE },
        { L"LTDA", FALSE },
        { L"GmbH", FALSE },
        { L"S.p.A", FALSE },
        { L"A.S.", FALSE },
        { L"S.A", FALSE },
        { L"S.A.S", FALSE },
        { L"S/A", FALSE },
        { L"SA", FALSE },
        { L"SAS", FALSE },
        { L"BV", FALSE },
        { L"AG", FALSE },
        { L"OOO", TRUE },
        { L"CJSC", FALSE },
        { L"INT'L", FALSE },
        { L"INTL", FALSE },
        { L"plc", FALSE },
    };
    PVOID SMBiosBuf;
    PCHAR DmiStrings[ID_STRINGS_MAX] = { 0 };
    WCHAR ven[512], dev[512];
    CHAR tmpstr[512];
    BOOL bTrimProduct, bTrimFamily, bGenericName, bRemove;
    UINT i;
    PWCHAR j;

    SMBiosBuf = LoadSMBiosData(DmiStrings);
    if (!SMBiosBuf)
    {
        return FALSE;
    }

    TrimNonPrintable(DmiStrings[SYS_VENDOR]);
    bTrimProduct = TrimNonPrintableProd(DmiStrings[SYS_PRODUCT]);
    TrimNonPrintable(DmiStrings[SYS_VERSION]);
    bTrimFamily = TrimNonPrintable(DmiStrings[SYS_FAMILY]);
    TrimNonPrintable(DmiStrings[BOARD_VENDOR]);
    TrimNonPrintable(DmiStrings[BOARD_NAME]);
    TrimNonPrintable(DmiStrings[BOARD_VERSION]);

    if (bTrimProduct)
    {
        if (DmiStrings[SYS_FAMILY] && !bTrimFamily)
        {
            DmiStrings[SYS_PRODUCT] = DmiStrings[SYS_FAMILY];
            bTrimProduct = FALSE;
        }
    }

    GetSMBiosStringW(DmiStrings[SYS_VENDOR], ven, _countof(ven), TRUE);
    GetSMBiosStringW(DmiStrings[SYS_PRODUCT], dev, _countof(dev), TRUE);
    bGenericName = IsGenericSystemName(ven, dev, NULL) || bTrimProduct;

    if (wcslen(dev) == 0 ||
        !wcscmp(dev, ven) ||
        bGenericName)
    {
        BOOL bGenericVen = FALSE, bRemoveVen = FALSE, bGenericDev = (wcslen(dev) == 0 || !wcscmp(dev, ven) || bTrimProduct);

        if (bGenericName && IsGenericSystemName(ven, NULL, &bRemove))
        {
            if (bRemove)
            {
                *ven = 0;
            }
            bGenericVen = TRUE;
            bRemoveVen = bRemove;
        }
        if (bGenericName && IsGenericSystemName(NULL, dev, &bRemove))
        {
            if (bRemove)
            {
                *dev = 0;
            }
            bGenericDev = TRUE;
        }
        // system strings are unusable, use board strings
        if (DmiStrings[BOARD_VENDOR] != NULL || !bGenericName)
        {
            if ((DmiStrings[BOARD_VENDOR] &&
                strlen(DmiStrings[BOARD_VENDOR]) >= 2 &&
                strstr(DmiStrings[BOARD_VENDOR], "  ") != DmiStrings[BOARD_VENDOR]) ||
                bGenericVen)
            {
                GetSMBiosStringW(DmiStrings[BOARD_VENDOR], ven, _countof(ven), TRUE);
            }
            GetSMBiosStringW(DmiStrings[BOARD_NAME], dev, _countof(dev), TRUE);

            if (IsGenericSystemName(ven, NULL, &bRemove) && bRemove)
            {
                *ven = 0;

                if (bGenericVen && !bRemoveVen)
                {
                    GetSMBiosStringW(DmiStrings[SYS_VENDOR], ven, _countof(ven), TRUE);
                }
            }
            if (IsGenericSystemName(NULL, dev, &bRemove) && bRemove)
            {
                *dev = 0;

                if (!bGenericDev)
                {
                    GetSMBiosStringW(DmiStrings[SYS_PRODUCT], dev, _countof(dev), TRUE);
                }
            }
            if (wcslen(dev) == 0 &&
                DmiStrings[SYS_VERSION] != NULL)
            {
                GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);

                if (IsGenericSystemName(NULL, dev, &bRemove) && bRemove)
                {
                    *dev = 0;
                }
            }
            if (wcslen(dev) == 0 &&
                DmiStrings[BOARD_VERSION] != NULL)
            {
                GetSMBiosStringW(DmiStrings[BOARD_VERSION], dev, _countof(dev), TRUE);

                if (IsGenericSystemName(NULL, dev, &bRemove) && bRemove)
                {
                    *dev = 0;
                }
            }
        }
        else if (DmiStrings[BOARD_NAME] != NULL)
        {
            GetSMBiosStringW(DmiStrings[BOARD_NAME], dev, _countof(dev), TRUE);

            if (IsGenericSystemName(NULL, dev, &bRemove) && bRemove)
            {
                *dev = 0;
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

            if (IsGenericSystemName(ven, NULL, &bRemove) && bRemove)
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
        if (wcsstr(dev, LongNames[i].pwLongName) == dev)
        {
            // swap ven and dev
            StringCchCopyW(pBuf, cchBuf, ven);
            StringCchCopyW(ven, _countof(ven), dev);
            StringCchCopyW(dev, _countof(dev), pBuf);
        }
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
    if (!_wcsicmp(ven, L"LENOVO"))
    {
        StringCchCopyW(ven, _countof(ven), L"Lenovo");

        if (DmiStrings[SYS_VERSION] != NULL)
        {
            if (!strncmp(DmiStrings[SYS_VERSION], "ThinkPad   ", 11))
            {
                DmiStrings[SYS_VERSION][8] = L'\0';
            }
            if (wcslen(dev) > 0 &&
                (!strcmp(DmiStrings[SYS_VERSION], "IdeaCentre") ||
                !strcmp(DmiStrings[SYS_VERSION], "ThinkPad")))
            {
                DmiStrings[SYS_FAMILY] = DmiStrings[SYS_VERSION];
                DmiStrings[SYS_VERSION] = NULL;
            }
            else
            {
                StringCchCopyA(tmpstr, _countof(tmpstr), DmiStrings[SYS_VERSION]);
                _strupr(tmpstr);
            }
        }

        if (DmiStrings[SYS_VERSION] != NULL &&
            strcmp(tmpstr, " ") &&
            strcmp(tmpstr, "LENOVO") &&
            strstr(tmpstr, "LENOVO   ") == NULL &&
            strstr(tmpstr, "LENOVO PRODUCT") == NULL &&
            strstr(tmpstr, "LENOVOPRODUCT") == NULL &&
            strstr(tmpstr, "INVALID") == NULL &&
            strncmp(tmpstr, "   ", 3) &&
            (strlen(tmpstr) >= 3 || !IsDigitStrA(tmpstr)) &&
            strstr(DmiStrings[SYS_VERSION], "Rev ") == NULL &&
            strstr(DmiStrings[SYS_VERSION], "1.") == NULL &&
            wcsistr(dev, L"System ") == NULL && // includes System x and ThinkSystem
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
        (strstr(DmiStrings[SYS_VERSION], "ThinkPad ") != NULL ||
         strstr(DmiStrings[SYS_VERSION], "ThinkCentre ") != NULL))
    {
        GetSMBiosStringW(DmiStrings[SYS_VERSION], dev, _countof(dev), TRUE);
    }

    // workaround for DEXP
    if (!wcscmp(ven, L"DEXP"))
    {
        if (DmiStrings[SYS_PRODUCT] != NULL &&
            DmiStrings[SYS_VERSION] != NULL &&
            (!_stricmp(DmiStrings[SYS_PRODUCT], "Tablet PC") ||
             !_stricmp(DmiStrings[SYS_PRODUCT], "Notebook") ||
             !_stricmp(DmiStrings[SYS_PRODUCT], "Decktop")))
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
        (!wcscmp(ven, L"HP") && wcsstr(dev, L" by HP") != NULL) ||
        (!wcscmp(ven, L"INTEL") && wcsstr(dev, L" INTEL") != NULL))
    {
        // device string contains vendor string, use second only
        StringCchCopyW(pBuf, cchBuf, dev);
    }
    else
    {
        if (wcslen(ven) > 0 && wcslen(dev) > 0 && (j = wcschr(dev, L' ')))
        {
            // check if vendor string ends with first word of device string
            i = j - dev;
            if (wcslen(ven) > i && !_wcsnicmp(ven + wcslen(ven) - i, dev, i))
            {
                ven[wcslen(ven) - i] = L'\0';
                TrimPunctuation(ven);
            }
        }
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
