/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for (Rtl)IsTextUnicode.
 * PROGRAMMERS:     Hermes Belusca-Maito
 *                  Dmitry Chapyshev
 */

#include "precomp.h"

#include <stdio.h>

PVOID LoadCodePageData(ULONG Code)
{
    char filename[MAX_PATH], sysdir[MAX_PATH];
    HANDLE hFile;
    PVOID Data = NULL;
    GetSystemDirectoryA(sysdir, MAX_PATH);

    if (Code != -1)
        StringCbPrintfA(filename, sizeof(filename),  "%s\\c_%lu.nls", sysdir, Code);
    else
        StringCbPrintfA(filename, sizeof(filename),  "%s\\l_intl.nls", sysdir);

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwRead;
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        Data = malloc(dwFileSize);
        ReadFile(hFile, Data, dwFileSize, &dwRead, NULL);
        CloseHandle(hFile);
    }
    return Data;
}

/* https://www.microsoft.com/resources/msdn/goglobal/default.mspx */
void SetupLocale(ULONG AnsiCode, ULONG OemCode, ULONG Unicode)
{
    NLSTABLEINFO NlsTable;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;

    AnsiCodePageData = LoadCodePageData(AnsiCode);
    OemCodePageData = LoadCodePageData(OemCode);
    UnicodeCaseTableData = LoadCodePageData(Unicode);

    RtlInitNlsTables(AnsiCodePageData, OemCodePageData, UnicodeCaseTableData, &NlsTable);
    RtlResetRtlTranslations(&NlsTable);
    /* Do NOT free the buffers here, they are directly used!
        Yes, we leak the old buffers, but this is a test anyway... */

}

START_TEST(IsTextUnicode)
{
#define INVALID_FLAG    0xFFFFFFFF

#define NEW_TEST(Buffer, Flags, ResultFlags, Success)   \
    { (PVOID)(Buffer), sizeof((Buffer)), (Flags), (ResultFlags), (Success) }

    static struct
    {
        /* Input */
        PVOID Buffer;
        INT   Size;
        INT   Flags;

        /* Output */
        INT   ResultFlags;
        BOOL  Success;
    } Tests[] =
    {
        /* ANSI string */

        // 0
        NEW_TEST("ANSI string", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST("ANSI string", IS_TEXT_UNICODE_STATISTICS, 0, FALSE),
        NEW_TEST("ANSI string", INVALID_FLAG, 0, FALSE),

        /* UNICODE strings */

        // 3
        NEW_TEST(L"a", IS_TEXT_UNICODE_ASCII16, IS_TEXT_UNICODE_ASCII16, TRUE),
        NEW_TEST(L"a", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_STATISTICS | IS_TEXT_UNICODE_ASCII16, TRUE),
        NEW_TEST(L"a", IS_TEXT_UNICODE_STATISTICS, IS_TEXT_UNICODE_STATISTICS, TRUE),
        NEW_TEST(L"a", INVALID_FLAG, 0, TRUE),

        // 7
        NEW_TEST(L"UNICODE String 0", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST(L"UNICODE String 0", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_CONTROLS | IS_TEXT_UNICODE_STATISTICS, TRUE),
        NEW_TEST(L"UNICODE String 0", IS_TEXT_UNICODE_STATISTICS, IS_TEXT_UNICODE_STATISTICS, TRUE),
        NEW_TEST(L"UNICODE String 0", INVALID_FLAG, 0, TRUE),

        // 11
        NEW_TEST(L"\xFEFF" L"UNICODE String 1", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST(L"\xFEFF" L"UNICODE String 1", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_CONTROLS, TRUE),
        NEW_TEST(L"\xFEFF" L"UNICODE String 1", IS_TEXT_UNICODE_STATISTICS, 0, FALSE),
        NEW_TEST(L"\xFEFF" L"UNICODE String 1", INVALID_FLAG, 0, TRUE),

        // 15
        NEW_TEST(L"\xFFFE" L"UNICODE String 2", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST(L"\xFFFE" L"UNICODE String 2", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_CONTROLS, TRUE),
        NEW_TEST(L"\xFFFE" L"UNICODE String 2", IS_TEXT_UNICODE_STATISTICS, 0, FALSE),
        NEW_TEST(L"\xFFFE" L"UNICODE String 2", INVALID_FLAG, 0, FALSE),

        // 19
        NEW_TEST(L"UNICODE String 3 Привет!", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST(L"UNICODE String 3 Привет!", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_CONTROLS | IS_TEXT_UNICODE_STATISTICS, TRUE),
        NEW_TEST(L"UNICODE String 3 Привет!", IS_TEXT_UNICODE_STATISTICS, IS_TEXT_UNICODE_STATISTICS, TRUE),
        NEW_TEST(L"UNICODE String 3 Привет!", INVALID_FLAG, 0, TRUE),

        // 23
        NEW_TEST(L"\xFEFF" L"UNICODE String 4 Привет!", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST(L"\xFEFF" L"UNICODE String 4 Привет!", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_CONTROLS, TRUE),
        NEW_TEST(L"\xFEFF" L"UNICODE String 4 Привет!", IS_TEXT_UNICODE_STATISTICS, 0, FALSE),
        NEW_TEST(L"\xFEFF" L"UNICODE String 4 Привет!", INVALID_FLAG, 0, TRUE),

        // 27
        NEW_TEST(L"\xFFFE" L"UNICODE String 5 Привет!", IS_TEXT_UNICODE_ASCII16, 0, FALSE),
        NEW_TEST(L"\xFFFE" L"UNICODE String 5 Привет!", IS_TEXT_UNICODE_UNICODE_MASK, IS_TEXT_UNICODE_CONTROLS, TRUE),
        NEW_TEST(L"\xFFFE" L"UNICODE String 5 Привет!", IS_TEXT_UNICODE_STATISTICS, 0, FALSE),
        NEW_TEST(L"\xFFFE" L"UNICODE String 5 Привет!", INVALID_FLAG, 0, FALSE),

        // 31
        /* Reverse BOM */
        NEW_TEST(L"UNICODE S" L"\xFFFE" L"tring 5 Привет!", IS_TEXT_UNICODE_ILLEGAL_CHARS, IS_TEXT_UNICODE_ILLEGAL_CHARS, FALSE),
        /* UNICODE_NUL */
        NEW_TEST(L"UNICODE S" L"\x0000" L"tring 5 Привет!", IS_TEXT_UNICODE_ILLEGAL_CHARS, IS_TEXT_UNICODE_ILLEGAL_CHARS, FALSE),
        /* ASCII CRLF (packed into one word) */
        NEW_TEST(L"UNICODE S" L"\x0A0D" L"tring 5 Привет!", IS_TEXT_UNICODE_ILLEGAL_CHARS, IS_TEXT_UNICODE_ILLEGAL_CHARS, FALSE),
        /* Unicode 0xFFFF */
        NEW_TEST(L"UNICODE S" L"\xFFFF" L"tring 5 Привет!", IS_TEXT_UNICODE_ILLEGAL_CHARS, IS_TEXT_UNICODE_ILLEGAL_CHARS, FALSE),

        // 35
        NEW_TEST(L"UNICODE String 0", IS_TEXT_UNICODE_DBCS_LEADBYTE, 0, FALSE)
    };

    const char japanese_with_lead[] = "ABC" "\x83\x40" "D";
    const char simplfied_chinese_with_lead[] = "ABC" "\xC5\xC5" "D";
    const char korean_with_lead[] = "ABC" "\xBF\xAD" "D";
    const char traditional_chinese_with_lead[] = "ABC" "\xB1\xC1" "D";

    UINT i;
    BOOL Success;
    INT Result;

    for (i = 0; i < ARRAYSIZE(Tests); ++i)
    {
        Result = Tests[i].Flags;
        Success = IsTextUnicode(Tests[i].Buffer, Tests[i].Size, ((Result != INVALID_FLAG) ? &Result : NULL));
        ok(Success == Tests[i].Success, "IsTextUnicode(%u) returned 0x%x, expected %s\n", i, Success, (Tests[i].Success ? "TRUE" : "FALSE"));
        if (Result != INVALID_FLAG)
            ok(Result == Tests[i].ResultFlags, "IsTextUnicode(%u) Result returned 0x%x, expected 0x%x\n", i, Result, Tests[i].ResultFlags);
    }

    /* Japanese */
    SetupLocale(932, 932, -1);

    Result = IS_TEXT_UNICODE_DBCS_LEADBYTE;
    ok(!IsTextUnicode(japanese_with_lead, sizeof(japanese_with_lead), &Result), "IsTextUnicode() returned TRUE, expected FALSE\n");
    ok(Result == IS_TEXT_UNICODE_DBCS_LEADBYTE, "Result returned 0x%x, expected 0x%x\n", Result, IS_TEXT_UNICODE_DBCS_LEADBYTE);

    /* Simplified Chinese */
    SetupLocale(936, 936, -1);

    Result = IS_TEXT_UNICODE_DBCS_LEADBYTE;
    ok(!IsTextUnicode(simplfied_chinese_with_lead, sizeof(simplfied_chinese_with_lead), &Result), "IsTextUnicode() returned TRUE, expected FALSE\n");
    ok(Result == IS_TEXT_UNICODE_DBCS_LEADBYTE, "Result returned 0x%x, expected 0x%x\n", Result, IS_TEXT_UNICODE_DBCS_LEADBYTE);

    /* Korean */
    SetupLocale(949, 949, -1);

    Result = IS_TEXT_UNICODE_DBCS_LEADBYTE;
    ok(!IsTextUnicode(korean_with_lead, sizeof(korean_with_lead), &Result), "IsTextUnicode() returned TRUE, expected FALSE\n");
    ok(Result == IS_TEXT_UNICODE_DBCS_LEADBYTE, "Result returned 0x%x, expected 0x%x\n", Result, IS_TEXT_UNICODE_DBCS_LEADBYTE);

    /* Traditional Chinese */
    SetupLocale(950, 950, -1);

    Result = IS_TEXT_UNICODE_DBCS_LEADBYTE;
    ok(!IsTextUnicode(traditional_chinese_with_lead, sizeof(traditional_chinese_with_lead), &Result), "IsTextUnicode() returned TRUE, expected FALSE\n");
    ok(Result == IS_TEXT_UNICODE_DBCS_LEADBYTE, "Result returned 0x%x, expected 0x%x\n", Result, IS_TEXT_UNICODE_DBCS_LEADBYTE);
}
