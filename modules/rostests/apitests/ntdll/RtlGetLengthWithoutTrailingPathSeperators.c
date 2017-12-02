/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlGetLengthWithoutTrailingPathSeperators
 * PROGRAMMER:      David Quintana <gigaherz@gmail.com>
 */

#include "precomp.h"

#define MakeTestEntry_Success(str, expect) \
    { str, expect, STATUS_SUCCESS, __LINE__ }

struct test_data {
    PWSTR input;
    ULONG expected_output;
    NTSTATUS expected_result;
    int line;
} test_entries[] = {

    /* NULL in UNICODE_STRING */
    MakeTestEntry_Success( NULL, 0 ),

    /* Length tests */
    MakeTestEntry_Success( L"", 0 ),
    MakeTestEntry_Success( L"T", 1 ),
    MakeTestEntry_Success( L"Te", 2 ),
    MakeTestEntry_Success( L"Tes", 3 ),
    MakeTestEntry_Success( L"Test", 4 ),

    /* Separators tests */
    MakeTestEntry_Success( L"\\.", 2 ),
    MakeTestEntry_Success( L"\\.", 2 ),
    MakeTestEntry_Success( L"\\.\\", 2 ),
    MakeTestEntry_Success( L"\\.\\T", 4 ),
    MakeTestEntry_Success( L"\\.\\Te", 5 ),
    MakeTestEntry_Success( L"\\.\\Tes", 6 ),
    MakeTestEntry_Success( L"\\.\\Test", 7 ),
    MakeTestEntry_Success( L"\\.\\Test\\", 7 ),
    MakeTestEntry_Success( L"\\.\\Test\\s", 9 ),
    MakeTestEntry_Success( L"\\.\\T\\est", 8 ),
    MakeTestEntry_Success( L"\\.\\T\\e\\st", 9 ),
    MakeTestEntry_Success( L"\\.\\T\\e\\s\\t", 10 ),
    MakeTestEntry_Success( L"\\.\\T\\e\\s\\t\\", 10 ),
    MakeTestEntry_Success( L"\\Tests\\String\\", 13 ),
    MakeTestEntry_Success( L"\\.\\Test\\String\\", 14 ),
    MakeTestEntry_Success( L"\\.\\Tests\\String\\", 15 ),
    MakeTestEntry_Success( L"\\.\\Tests\\String\\s", 17 ),
    MakeTestEntry_Success( L"\\.\\Tests\\String\\as", 18 ),
    MakeTestEntry_Success( L"\\.\\Tests\\String\\asd", 19 ),
    MakeTestEntry_Success( L"\\.\\Tests\\String\\asdf", 20 ),
    MakeTestEntry_Success( L"\\Tests\\String\\sdfsdf\\", 20 ),
    MakeTestEntry_Success( L"C:\\Tests\\String\\sdfsdf\\", 22 ),

    /* Separator-only tests */
    MakeTestEntry_Success( L"\\", 0 ),
    MakeTestEntry_Success( L"/", 0 ),

    /* Mixed separators tests */
    MakeTestEntry_Success( L"/Test/String", 12 ),
    MakeTestEntry_Success( L"\\Test/String", 12 ),
    MakeTestEntry_Success( L"/Test\\String", 12 ),
    MakeTestEntry_Success( L"\\Test/String", 12 ),
    MakeTestEntry_Success( L"/Test/String\\", 12 ),
    MakeTestEntry_Success( L"\\Test/String\\", 12 ),
    MakeTestEntry_Success( L"/Test\\String\\", 12 ),
    MakeTestEntry_Success( L"\\Test/String\\", 12 ),
    MakeTestEntry_Success( L"/Test/String/", 12 ),
    MakeTestEntry_Success( L"\\Test/String/", 12 ),
    MakeTestEntry_Success( L"/Test\\String/", 12 ),
    MakeTestEntry_Success( L"\\Test/String/", 12 ),
    MakeTestEntry_Success( L"\\Test/String/", 12 ),
    MakeTestEntry_Success( L"\\Test\\\\String/", 13 ),

    /* Common path formats tests */
    MakeTestEntry_Success( L"Test\\String", 11 ),
    MakeTestEntry_Success( L"\\Test\\String", 12 ),
    MakeTestEntry_Success( L".\\Test\\String", 13 ),
    MakeTestEntry_Success( L"\\.\\Test\\String", 14 ),
    MakeTestEntry_Success( L"\\??\\Test\\String", 15 ),

    /* Redundant trailing tests */
    MakeTestEntry_Success( L"\\??\\Test\\String\\", 15 ),
    MakeTestEntry_Success( L"\\??\\Test\\String\\\\", 15 ),
    MakeTestEntry_Success( L"\\??\\Test\\String\\\\\\\\\\", 15 ),

};

int num_tests = sizeof(test_entries) / sizeof(struct test_data);

START_TEST(RtlGetLengthWithoutTrailingPathSeperators)
{
    ULONG len;
    UNICODE_STRING str;
    NTSTATUS res;
    int i;

    struct test_data * pentry = test_entries;

    for(i = 0; i < num_tests; i++, pentry++)
    {
        RtlInitUnicodeString(
            &str, pentry->input);

        len = 0xDEADBEEF;

        StartSeh()
            res = RtlGetLengthWithoutTrailingPathSeperators(0, &str, &len);
        EndSeh(STATUS_SUCCESS);

        ok(res == pentry->expected_result,
            "Unexpected result 0x%08lx (expected 0x%08lx) in [%d:%d]\n",
            res, pentry->expected_result,
            i, pentry->line);
        ok(len == pentry->expected_output,
            "Unexpected length %lu (expected %lu) in [%d:%d]\n",
            len, pentry->expected_output,
            i, pentry->line);
    }

    // Invalid parameters

    len = 0xDEADBEEF;

    StartSeh()
        res = RtlGetLengthWithoutTrailingPathSeperators(0, NULL, &len);
    EndSeh(STATUS_SUCCESS);

    ok(res == STATUS_INVALID_PARAMETER,
        "Unexpected result 0x%08lx (expected STATUS_INVALID_PARAMETER)\n",
        res);
    ok(len == 0,
        "Unexpected length %08lx (expected 0)\n",
        len);

    StartSeh()
        res = RtlGetLengthWithoutTrailingPathSeperators(0, &str, NULL);
    EndSeh(STATUS_SUCCESS);

    ok(res == STATUS_INVALID_PARAMETER,
        "Unexpected result 0x%08lx (expected STATUS_INVALID_PARAMETER)\n",
        res);

    StartSeh()
        res = RtlGetLengthWithoutTrailingPathSeperators(0, NULL, NULL);
    EndSeh(STATUS_SUCCESS);

    ok(res == STATUS_INVALID_PARAMETER,
        "Unexpected result 0x%08lx (expected STATUS_INVALID_PARAMETER)\n",
        res);

    for(i = 0; i < 32; i++)
    {
        len = 0xDEADBEEF;

        StartSeh()
            res = RtlGetLengthWithoutTrailingPathSeperators(1<<i, &str, &len);
        EndSeh(STATUS_SUCCESS);

        ok(res == STATUS_INVALID_PARAMETER,
            "Unexpected result 0x%08lx (expected STATUS_INVALID_PARAMETER)\n",
            res);

        ok(len == 0,
            "Unexpected length %08lx (expected 0)\n",
            len);
    }

    len = 0xDEADBEEF;

    StartSeh()
        res = RtlGetLengthWithoutTrailingPathSeperators(0xFFFFFFFF, &str, &len);
    EndSeh(STATUS_SUCCESS);

    ok(res == STATUS_INVALID_PARAMETER,
        "Unexpected result 0x%08lx (expected STATUS_INVALID_PARAMETER)\n",
        res);

    ok(len == 0,
        "Unexpected length %08lx (expected 0)\n",
        len);
}
