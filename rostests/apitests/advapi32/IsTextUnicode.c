/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for (Rtl)IsTextUnicode.
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <winbase.h>

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
    };

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
}
