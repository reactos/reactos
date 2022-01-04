/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     0BSD (https://spdx.org/licenses/0BSD.html)
 * PURPOSE:     Test for RtlUnicodeToOemN
 * COPYRIGHT:   Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

static const struct
{
    ULONG AnsiCp;
    ULONG OemCp;
    struct
    {
        LPCWSTR StrW;
        NTSTATUS Status;
        ULONG ReturnedSize;
        LPCSTR StrOem;
    } Test[10];
} TestData[] =
{
    {
        1252, 1252, /* Western SBCS */
        {
            {
                L"ABCDEF",
                STATUS_SUCCESS,
                6,
                "ABCDEF"
            },
            {
                L"ABCDEF",
                STATUS_SUCCESS,
                6,
                "ABCDEF\xAA\xAA\xAA"
            },
            {
                L"ABCDEF",
                STATUS_BUFFER_OVERFLOW,
                3,
                "ABC"
            },
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_SUCCESS,
                6,
                "??????"
            },
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_BUFFER_OVERFLOW,
                3,
                "???"
            },
        }
    },
    {
        1252, 932, /* Western SBCS - Modified SJIS */
        {
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_SUCCESS,
                12,
                "\x83\x66\x83\x58\x83\x4e\x83\x67\x83\x62\x83\x76"
            },
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_SUCCESS,
                12,
                "\x83\x66\x83\x58\x83\x4e\x83\x67\x83\x62\x83\x76\xAA\xAA\xAA"
            },
        }
    },
    {
        932, 1252, /* Modified SJIS - Western SBCS */
        {
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_SUCCESS,
                6,
                "??????"
            },
            {
                L"\u30c7\u30b9\u30afABC",
                STATUS_SUCCESS,
                6,
                "???ABC"
            },
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_BUFFER_OVERFLOW,
                3,
                "???"
            },
        }
    },
    {
        932, 932, /* Modified SJIS - Modified SJIS */
        {
            {
                L"\u30c7\u30b9\u30af\u30c8\u30c3\u30d7",
                STATUS_SUCCESS,
                12,
                "\x83\x66\x83\x58\x83\x4e\x83\x67\x83\x62\x83\x76"
            }
        }
    },
};

START_TEST(RtlUnicodeToOemN)
{
    ULONG Length;
    LPSTR StrOem;
    ULONG ResultSize;
    NTSTATUS Status;
    CHAR OemBuffer[4];

    /* Basic things */
    Status = RtlUnicodeToOemN(NULL, 0, NULL, NULL, 0);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = RtlUnicodeToOemN(NULL, 0, NULL, L"ABCDEF", 0);
    ok_ntstatus(Status, STATUS_SUCCESS);

    Status = RtlUnicodeToOemN(NULL, 0, NULL, NULL, 2);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);

    Status = RtlUnicodeToOemN(NULL, 0, NULL, L"A", 2);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);

    Status = RtlUnicodeToOemN(NULL, 0, NULL, L"A", 2);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);

    StartSeh()
    Status = RtlUnicodeToOemN(NULL, 1, NULL, L"A", 2);
    EndSeh(STATUS_ACCESS_VIOLATION)

    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 1, NULL, L"A", 2);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], (CHAR)0xAA);

    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 1, NULL, L"AB", 4);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], (CHAR)0xAA);

    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 2, NULL, L"A", 4);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], '\0');

    /* RtlUnicodeToOemN doesn't care about string termination */
    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 1, NULL, L"A", 4);
    ok_ntstatus(Status, STATUS_BUFFER_OVERFLOW);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], (CHAR)0xAA);

    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    OemBuffer[2] = 0xAA;
    OemBuffer[3] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 4, NULL, L"A\0B", 8);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], '\0');
    ok_char(OemBuffer[2], 'B');
    ok_char(OemBuffer[3], '\0');

    /* Odd Unicode buffer size */
    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    OemBuffer[2] = 0xAA;
    OemBuffer[3] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 2, NULL, L"AB", 5);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], 'B');
    ok_char(OemBuffer[2], (CHAR)0xAA);
    ok_char(OemBuffer[3], (CHAR)0xAA);

    /* Odd Unicode buffer size */
    OemBuffer[0] = 0xAA;
    OemBuffer[1] = 0xAA;
    OemBuffer[2] = 0xAA;
    OemBuffer[3] = 0xAA;
    Status = RtlUnicodeToOemN(OemBuffer, 3, NULL, L"AB", 5);
    ok_ntstatus(Status, STATUS_SUCCESS);
    ok_char(OemBuffer[0], 'A');
    ok_char(OemBuffer[1], 'B');
    ok_char(OemBuffer[2], (CHAR)0xAA);
    ok_char(OemBuffer[3], (CHAR)0xAA);

    for (int i = 0; i < _countof(TestData); i++)
    {
        SetupLocale(TestData[i].AnsiCp, TestData[i].OemCp, -1);

        for (int j = 0; TestData[i].Test[j].StrW != NULL; j++)
        {
            Length = strlen(TestData[i].Test[j].StrOem);
            StrOem = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length + 1);

            memset(StrOem, 0xAA, Length + 1);
            ResultSize = 0x0BADF00D;

            Status = RtlUnicodeToOemN(StrOem,
                                      Length,
                                      &ResultSize,
                                      TestData[i].Test[j].StrW,
                                      wcslen(TestData[i].Test[j].StrW) * sizeof(WCHAR));

            ok_ntstatus(Status, TestData[i].Test[j].Status);
            ok_long(ResultSize, TestData[i].Test[j].ReturnedSize);
            for (int k = 0; k < ResultSize; k++)
            {
                ok(StrOem[k] == TestData[i].Test[j].StrOem[k],
                   "Wrong char \\x%02x, expected TestData[%u].Test[%u].StrOem[%u] (\\x%02x)\n",
                   StrOem[k], i, j, k, TestData[i].Test[j].StrOem[k]);
            }
            for (int k = ResultSize; k < (Length + 1); k++)
            {
                ok_char(StrOem[k], (CHAR)0xAA);
            }

            RtlFreeHeap(RtlGetProcessHeap(), 0, StrOem);
        }
    }
}
