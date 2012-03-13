/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite FsRtl Test
 * PROGRAMMER:      Pierre Schweitzer <pierre.schweitzer@reactos.org>
 */

/* TODO: most of these calls fail the Windows checked build's !islower assertion and others */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static VOID FsRtlIsNameInExpressionTest()
{
    UNICODE_STRING Expression, Name;

    /* !Name->Length || !Expression->Length asserts */
    if (!KmtIsCheckedBuild)
    {
        RtlInitUnicodeString(&Expression, L"*");
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
        RtlInitUnicodeString(&Expression, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

        RtlInitUnicodeString(&Expression, L"**");
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
        RtlInitUnicodeString(&Name, L"a");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    }

    RtlInitUnicodeString(&Expression, L"ntdll.dll");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"~1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"ntdll.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"smss.exe");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"~1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"ntdll.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"NTDLL.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"nt??krnl.???");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"he*o");
    RtlInitUnicodeString(&Name, L"hello");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"helo");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"hella");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"he*");
    RtlInitUnicodeString(&Name, L"hello");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"helo");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"hella");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"*.cpl");
    RtlInitUnicodeString(&Name, L"kdcom.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"bootvid.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L".");
    RtlInitUnicodeString(&Name, L"NTDLL.DLL");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"F0_*.*");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"SETUP.EXE");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"F0_001");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"*.TTF");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"SETUP.INI");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"*");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"SETUP.INI");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"\"ntoskrnl.exe");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrnl\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"ntoskrn\".exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrn\"\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrnl.\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"ntoskrnl.exe\"");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe.");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"*.c.d");
    RtlInitUnicodeString(&Name, L"a.b.c.d");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"*.?.c.d");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    }
    RtlInitUnicodeString(&Name, L"a");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"aa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"aaa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"?*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    }
    RtlInitUnicodeString(&Name, L"a");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Name, L"aa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"aaa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Name, L"aaaa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    /* Tests from #5923 */
    RtlInitUnicodeString(&Expression, L"C:\\ReactOS\\**");
    RtlInitUnicodeString(&Name, L"C:\\ReactOS\\dings.bmp");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"C:\\ReactOS\\***");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"C:\\Windows\\*a*");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    RtlInitUnicodeString(&Expression, L"C:\\ReactOS\\*.bmp");
    RtlInitUnicodeString(&Name, L"C:\\Windows\\explorer.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");
    RtlInitUnicodeString(&Expression, L"*.bmp;*.dib");
    RtlInitUnicodeString(&Name, L"winhlp32.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE");

    /* Backtracking tests */
    RtlInitUnicodeString(&Expression, L"*.*.*.*");
    RtlInitUnicodeString(&Name, L"127.0.0.1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"*?*?*?*");
    RtlInitUnicodeString(&Name, L"1.0.0.1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"?*?*?*?");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
    RtlInitUnicodeString(&Expression, L"?.?.?.?");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");

    RtlInitUnicodeString(&Expression, L"*a*ab*abc");
    RtlInitUnicodeString(&Name, L"aabaabcdadabdabc");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE");
}

static VOID FsRtlIsDbcsInExpressionTest()
{
    ANSI_STRING Expression, Name;

    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Expression, "*");
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
        RtlInitAnsiString(&Expression, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

        RtlInitAnsiString(&Expression, "**");
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
        RtlInitAnsiString(&Name, "a");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    }

    RtlInitAnsiString(&Expression, "ntdll.dll");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "~1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "ntdll.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "smss.exe");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "~1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "ntdll.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "NTDLL.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "nt??krnl.???");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "he*o");
    RtlInitAnsiString(&Name, "hello");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "helo");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "hella");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "he*");
    RtlInitAnsiString(&Name, "hello");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "helo");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "hella");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "*.cpl");
    RtlInitAnsiString(&Name, "kdcom.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "bootvid.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, ".");
    RtlInitAnsiString(&Name, "NTDLL.DLL");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "F0_*.*");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "SETUP.EXE");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "F0_001");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "*.TTF");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "SETUP.INI");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "*");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "SETUP.INI");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "\"ntoskrnl.exe");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrnl\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "ntoskrn\".exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrn\"\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrnl.\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "ntoskrnl.exe\"");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "ntoskrnl.exe.");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "*.c.d");
    RtlInitAnsiString(&Name, "a.b.c.d");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "*.?.c.d");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    }
    RtlInitAnsiString(&Name, "a");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "aa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "aaa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "?*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    }
    RtlInitAnsiString(&Name, "a");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Name, "aa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "aaa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Name, "aaaa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    /* Tests from #5923 */
    RtlInitAnsiString(&Expression, "C:\\ReactOS\\**");
    RtlInitAnsiString(&Name, "C:\\ReactOS\\dings.bmp");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "C:\\ReactOS\\***");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "C:\\Windows\\*a*");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    RtlInitAnsiString(&Expression, "C:\\ReactOS\\*.bmp");
    RtlInitAnsiString(&Name, "C:\\Windows\\explorer.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");
    RtlInitAnsiString(&Expression, "*.bmp;*.dib");
    RtlInitAnsiString(&Name, "winhlp32.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE");

    /* Backtracking tests */
    RtlInitAnsiString(&Expression, "*.*.*.*");
    RtlInitAnsiString(&Name, "127.0.0.1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "*?*?*?*");
    RtlInitAnsiString(&Name, "1.0.0.1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "?*?*?*?");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
    RtlInitAnsiString(&Expression, "?.?.?.?");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");

    RtlInitAnsiString(&Expression, "*a*ab*abc");
    RtlInitAnsiString(&Name, "aabaabcdadabdabc");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE");
}

START_TEST(FsRtlExpression)
{
    FsRtlIsNameInExpressionTest();
    FsRtlIsDbcsInExpressionTest();
}
