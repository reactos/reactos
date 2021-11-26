/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetFontResourceInfoW
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

/* GetFontResourceInfoW is undocumented */
BOOL WINAPI GetFontResourceInfoW(LPCWSTR lpFileName, DWORD *pdwBufSize, void* lpBuffer, DWORD dwType);

/* structure of test entry */
typedef struct GFRI_ENTRY
{
    LPCWSTR     File;
    BOOL        Preinstalled;
    WCHAR       FontInfo[64];
    INT         FontCount;
    WCHAR       FaceNames[10][64];
} GFRI_ENTRY;

/* test entries */
static const GFRI_ENTRY TestEntries[] =
{
    { L"symbol.ttf", TRUE, L"Symbol|", 1, { L"Symbol" } },
    { L"tahoma.ttf", TRUE, L"Tahoma|", 1, { L"Tahoma" } },
    { L"tahomabd.ttf", TRUE, L"Tahoma Bold|", 1, { L"Tahoma" } }
};

/* Japanese */
static const GFRI_ENTRY AdditionalTestEntriesJapanese[] =
{
    {
        /* MS Gothic & MS UI Gothic & MS PGothic */
        L"msgothic.ttc", TRUE,
        {
            0xFF2D, 0xFF33, 0x0020, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0x0020,
            0x0026, 0x0020, 0x004D, 0x0053, 0x0020, 0x0055, 0x0049, 0x0020,
            0x0047, 0x006F, 0x0074, 0x0068, 0x0069, 0x0063, 0x0020, 0x0026,
            0x0020, 0xFF2D, 0xFF33, 0x0020, 0xFF30, 0x30B4, 0x30B7, 0x30C3,
            0x30AF, L'|', 0
        },
        6,
        {
            { 0xFF2D, 0xFF33, 0x0020, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 },
            { L'@', 0xFF2D, 0xFF33, 0x0020, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 },
            L"MS UI Gothic",
            L"@MS UI Gothic",
            { 0xFF2D, 0xFF33, 0x0020, 0xFF30, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 },
            { L'@', 0xFF2D, 0xFF33, 0x0020, 0xFF30, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 }
        }
    },
    {
        L"ExampleFont.ttf", FALSE,
        L"JapaneseDisplayName|",
        1,
        {
            L"JapaneseFamilyName"
        }
    }
};

/* English */
static const GFRI_ENTRY AdditionalTestEntriesEnglish[] =
{
    {
        /* MS Gothic & MS UI Gothic & MS PGothic */
        L"msgothic.ttc", TRUE,
        L"MS Gothic & MS UI Gothic & MS PGothic|",
        6,
        {
            L"MS Gothic",
            L"@MS Gothic",
            L"MS UI Gothic",
            L"@MS UI Gothic",
            L"MS PGothic",
            L"@MS PGothic"
        }
    },
    {
        L"ExampleFont.ttf", FALSE,
        L"EnglishDisplayName|",
        1,
        {
            L"EnglishFamilyName"
        }
    }
};

static void
GetSystemFontDirectory(LPWSTR pszDir)
{
    GetWindowsDirectoryW(pszDir, MAX_PATH);
    lstrcatW(pszDir, L"\\Fonts");
}

static void
GetSystemFontPath(LPWSTR pszPath, LPCWSTR pszFile)
{
    GetSystemFontDirectory(pszPath);
    lstrcatW(pszPath, L"\\");
    lstrcatW(pszPath, pszFile);
}

static INT
GetMultiSzLength(const WCHAR *pszz)
{
    INT Len, TotalLen = 0;
    for (;;)
    {
        Len = lstrlenW(pszz);
        TotalLen += Len + 1;
        pszz += Len + 1;
        if (*pszz == 0)
            break;
    }
    ++TotalLen;
    return TotalLen;
}

static void
ReplaceChars(WCHAR *pch, INT Len, WCHAR From, WCHAR To)
{
    while (Len > 0)
    {
        if (*pch == From)
        {
            *pch = To;
        }
        //printf("0x%04X, ", *pch);
        ++pch;
        --Len;
    }
}

static void
SzFromMultiSz(WCHAR *pszz)
{
    INT Length = GetMultiSzLength(pszz);
    //printf("Length: %d\n", Length);
    if (Length > 0)
    {
        ReplaceChars(pszz, Length - 1, L'\0', L'|');
        pszz[Length - 1] = 0;
    }
    else
    {
        pszz[0] = 0;
    }
    //printf("pszz: %S\n", pszz);
}

static void
Test_GetFontResourceInfoW_case0(LPCWSTR pszFilePath, const GFRI_ENTRY *Entry)
{
    BOOL Ret;
    DWORD Size, Case = 0;
    DWORD Data;

    /* data NULL, size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);

    /* data NULL, size non-zero */
    Size = 1024;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 0);
    ok_int(Size, 1024);

    /* size zero */
    Data = 0xDEADFACE;
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, &Data, Case);
    ok_int(Ret, 1);
    ok_int(Data, 0xDEADFACE);
    ok_int(Size, 4);

    /* size non-zero */
    Data = 0xDEADFACE;
    Size = sizeof(Data);
    Ret = GetFontResourceInfoW(pszFilePath, &Size, &Data, Case);
    ok_int(Ret, 1);
    ok_int(Data, Entry->FontCount);
    ok_int(Size, 4);
}

static void
Test_GetFontResourceInfoW_case1(LPCWSTR pszFilePath, const GFRI_ENTRY *Entry)
{
    BOOL Ret;
    DWORD Size, Case = 1;
    static WCHAR Data[1024 / sizeof(WCHAR)];

    /* data NULL, size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 1);
    /* FIXME: What's the result of Size? */
    ok(Size != 0, "Size expected non-zero but zero\n");

    /* data NULL, size non-zero */
    Size = 1024;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 0);
    ok_int(Size, 1024);

    /* size zero */
    Size = 0;
    CopyMemory(Data, L"ABC\0", sizeof(L"ABC\0"));
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    /* FIXME: What's the result of Size? */
    ok(Size != 0, "Size expected non-zero but zero\n");
    ok(lstrcmpiW(Data, L"ABC") == 0, "data mismatched: \"%S\"\n", Data);

    /* size non-zero */
    Size = 1024;
    CopyMemory(Data, L"ABC\0", sizeof(L"ABC\0"));
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    /* FIXME: What's the result of Size? */
    ok(Size != 0, "Size expected non-zero but zero\n");

    SzFromMultiSz(Data);
    ok(lstrcmpiW(Data, Entry->FontInfo) == 0, "data mismatched: \"%S\" and \"%S\"\n",
       Data, Entry->FontInfo);
#if 0
    if (lstrcmpiW(Data, Entry->FontInfo) != 0)
    {
        int i, len = lstrlenW(Data) + 1;
        for (i = 0; i < len; ++i)
        {
            printf("0x%04X <=> 0x%04X\n", Data[i], Entry->FontInfo[i]);
        }
    }
#endif
}

static void
Test_GetFontResourceInfoW_case2(LPCWSTR pszFilePath, const GFRI_ENTRY *Entry)
{
    BOOL Ret;
    DWORD i, Size, Case = 2;
    static LOGFONTW Data[100];

    /* data NULL, size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 1);
    ok_int(Size, Entry->FontCount * sizeof(LOGFONTW));

    /* data NULL, size non-zero */
    Size = 1024;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 0);
    ok_int(Size, 1024);

    /* size zero */
    Size = 0;
    ZeroMemory(Data, sizeof(Data));
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, Entry->FontCount * sizeof(LOGFONTW));

    /* size non-zero */
    Size = sizeof(Data);
    ZeroMemory(Data, sizeof(Data));
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, Entry->FontCount * sizeof(LOGFONTW));
    for (i = 0; i < Entry->FontCount; ++i)
    {
        ok(lstrcmpiW(Data[i].lfFaceName, Entry->FaceNames[i]) == 0,
            "face name #%d mismatched: \"%S\" and \"%S\"\n", (int)i, Data[i].lfFaceName, Entry->FaceNames[i]);
#if 0
        if (lstrcmpiW(Data[i].lfFaceName, Entry->FaceNames[i]) != 0)
        {
            int k, len = lstrlenW(Data[i].lfFaceName);
            for (k = 0; k < len; ++k)
            {
                printf("0x%04X <=> 0x%04X\n", Entry->FaceNames[i][k], Data[i].lfFaceName[k]);
            }
        }
#endif
    }
}

static void
Test_GetFontResourceInfoW_case3(LPCWSTR pszFilePath, const GFRI_ENTRY *Entry)
{
    BOOL Ret;
    DWORD Size, Case = 3;
    DWORD Data[2];

    /* data NULL, size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);

    /* data NULL, size non-zero */
    Size = sizeof(Data);
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 0);
    ok_int(Size, 8);

    /* size zero */
    Size = 0;
    Data[0] = 0xDEADFACE;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);
    ok_int(Data[0], 0xDEADFACE);

    /* size non-zero */
    Size = sizeof(Data);
    Data[0] = 0xDEADFACE;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);
    ok_int(Data[0], 1);
}

static void
Test_GetFontResourceInfoW_case4(LPCWSTR pszFilePath, const GFRI_ENTRY *Entry)
{
    BOOL Ret;
    DWORD Size, Case = 4;
    WCHAR Data[MAX_PATH];

    /* data NULL, size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 1);
    ok_int(Size, (lstrlenW(pszFilePath) + 1) * sizeof(WCHAR));

    /* data NULL, size non-zero */
    Size = MAX_PATH;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 0);
    ok_int(Size, MAX_PATH);

    /* size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, (lstrlenW(pszFilePath) + 1) * sizeof(WCHAR));

    /* size non-zero */
    Size = MAX_PATH;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, (lstrlenW(pszFilePath) + 1) * sizeof(WCHAR));
    ok(lstrcmpiW(pszFilePath, Data) == 0, "data mismatched: \"%S\"\n", Data);
}

static void
Test_GetFontResourceInfoW_case5(LPCWSTR pszFilePath, const GFRI_ENTRY *Entry)
{
    BOOL Ret;
    DWORD Size, Case = 5;
    DWORD Data;

    /* data NULL, size zero */
    Size = 0;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);

    /* data NULL, size non-zero */
    Size = sizeof(Data);
    Ret = GetFontResourceInfoW(pszFilePath, &Size, NULL, Case);
    ok_int(Ret, 0);
    ok_int(Size, 4);

    /* size zero */
    Size = 0;
    Data = 0xDEADFACE;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, &Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);
    ok_int(Data, 0xDEADFACE);

    /* size non-zero */
    Size = sizeof(Data);
    Data = 0xDEADFACE;
    Ret = GetFontResourceInfoW(pszFilePath, &Size, &Data, Case);
    ok_int(Ret, 1);
    ok_int(Size, 4);
    ok_int(Data, 0);
}

static void
DoEntry(const GFRI_ENTRY *Entry)
{
    WCHAR szPath[MAX_PATH], szTempPath[MAX_PATH];
    BOOL Installed = FALSE;

    if (Entry->Preinstalled)
    {
        GetSystemFontPath(szPath, Entry->File);
        printf("GetSystemFontPath: %S\n", szPath);
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            skip("Font file \"%S\" was not found\n", szPath);
            return;
        }
    }
    else
    {
        /* load font data from resource */
        HANDLE hFile;
        HMODULE hMod = GetModuleHandleW(NULL);
        HRSRC hRsrc = FindResourceW(hMod, Entry->File, (LPCWSTR)RT_RCDATA);
        HGLOBAL hGlobal = LoadResource(hMod, hRsrc);
        DWORD Size = SizeofResource(hMod, hRsrc);
        LPVOID pFont = LockResource(hGlobal);

        /* get temporary file name */
        GetTempPathW(_countof(szTempPath), szTempPath);
        GetTempFileNameW(szTempPath, L"FNT", 0, szPath);
        printf("GetTempFileNameW: %S\n", szPath);

        /* write to file */
        hFile = CreateFileW(szPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        WriteFile(hFile, pFont, Size, &Size, NULL);
        CloseHandle(hFile);

        /* check existence */
        if (GetFileAttributesW(szPath) == INVALID_FILE_ATTRIBUTES)
        {
            skip("Font file \"%S\" was not stored\n", szPath);
            return;
        }

        /* install */
        Installed = !!AddFontResourceW(szPath);
        if (!Installed)
        {
            skip("Font file \"%S\" was not installed\n", szPath);
            RemoveFontResourceW(szPath);
            DeleteFileW(szPath);
            return;
        }
    }

    Test_GetFontResourceInfoW_case0(szPath, Entry);
    Test_GetFontResourceInfoW_case1(szPath, Entry);
    Test_GetFontResourceInfoW_case2(szPath, Entry);
    Test_GetFontResourceInfoW_case3(szPath, Entry);
    Test_GetFontResourceInfoW_case4(szPath, Entry);
    Test_GetFontResourceInfoW_case5(szPath, Entry);

    if (!Entry->Preinstalled)
    {
        if (Installed)
        {
            RemoveFontResourceW(szPath);
            DeleteFileW(szPath);
        }
    }
}

START_TEST(GetFontResourceInfoW)
{
    INT i;
    const GFRI_ENTRY *Entry;

    printf("sizeof(LOGFONTW) == %u\n", (int)sizeof(LOGFONTW));

    for (i = 0; i < _countof(TestEntries); ++i)
    {
        Entry = &TestEntries[i];
        DoEntry(Entry);
    }

    if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_JAPANESE)
    {
        /* Japanese */
        for (i = 0; i < _countof(AdditionalTestEntriesJapanese); ++i)
        {
            Entry = &AdditionalTestEntriesJapanese[i];
            DoEntry(Entry);
        }
    }
    else
    {
        /* non-Japanese */
        for (i = 0; i < _countof(AdditionalTestEntriesEnglish); ++i)
        {
            Entry = &AdditionalTestEntriesEnglish[i];
            DoEntry(Entry);
        }
    }
}
