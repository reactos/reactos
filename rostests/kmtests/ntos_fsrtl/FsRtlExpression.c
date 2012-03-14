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
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
        RtlInitUnicodeString(&Expression, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    }

    RtlInitUnicodeString(&Expression, L"**");
    if (!KmtIsCheckedBuild)
    {
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    }
    RtlInitUnicodeString(&Name, L"a");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"ntdll.dll");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"~1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"ntdll.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"smss.exe");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"~1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"ntdll.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"NTDLL.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"nt??krnl.???");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"he*o");
    RtlInitUnicodeString(&Name, L"hello");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"helo");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"hella");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"he*");
    RtlInitUnicodeString(&Name, L"hello");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"helo");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"hella");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"*.cpl");
    RtlInitUnicodeString(&Name, L"kdcom.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"bootvid.dll");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L".");
    RtlInitUnicodeString(&Name, L"NTDLL.DLL");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"F0_*.*");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"SETUP.EXE");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"F0_001");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"*.TTF");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"SETUP.INI");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"*");
    RtlInitUnicodeString(&Name, L".");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"..");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"SETUP.INI");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"\"ntoskrnl.exe");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"ntoskrnl\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"ntoskrn\".exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"ntoskrn\"\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"ntoskrnl.\"exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"ntoskrnl.exe\"");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"ntoskrnl.exe.");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"*.c.d");
    RtlInitUnicodeString(&Name, L"a.b.c.d");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"*.?.c.d");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    }
    RtlInitUnicodeString(&Name, L"a");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"aa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"aaa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"?*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitUnicodeString(&Name, L"");
        ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    }
    RtlInitUnicodeString(&Name, L"a");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"aa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"aaa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"aaaa");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    /* Tests from #5923 */
    RtlInitUnicodeString(&Expression, L"C:\\ReactOS\\**");
    RtlInitUnicodeString(&Name, L"C:\\ReactOS\\dings.bmp");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"C:\\ReactOS\\***");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"C:\\Windows\\*a*");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"C:\\ReactOS\\*.bmp");
    RtlInitUnicodeString(&Name, L"C:\\Windows\\explorer.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"*.bmp;*.dib");
    RtlInitUnicodeString(&Name, L"winhlp32.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    /* Backtracking tests */
    RtlInitUnicodeString(&Expression, L"*.*.*.*");
    RtlInitUnicodeString(&Name, L"127.0.0.1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"*?*?*?*");
    RtlInitUnicodeString(&Name, L"1.0.0.1");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"?*?*?*?");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"?.?.?.?");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"*a*ab*abc");
    RtlInitUnicodeString(&Name, L"aabaabcdadabdabc");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    /* Tests for extra wildcards */
    RtlInitUnicodeString(&Expression, L"ab<exe");
    RtlInitUnicodeString(&Name, L"abcd.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"ab.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"abcdexe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"acd.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"a.b<exe");
    RtlInitUnicodeString(&Name, L"a.bcd.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Expression, L"a<b.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"a.b.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitUnicodeString(&Expression, L"abc.exe\"");
    RtlInitUnicodeString(&Name, L"abc.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"abc.exe.");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"abc.exe.back");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"abc.exes");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitUnicodeString(&Expression, L"a>c.exe");
    RtlInitUnicodeString(&Name, L"abc.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitUnicodeString(&Name, L"ac.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Expression, L"a>>>exe");
    RtlInitUnicodeString(&Name, L"abc.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitUnicodeString(&Name, L"ac.exe");
    ok(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL) == FALSE, "expected FALSE, got TRUE\n");
}

static VOID FsRtlIsDbcsInExpressionTest()
{
    ANSI_STRING Expression, Name;

    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Expression, "*");
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
        RtlInitAnsiString(&Expression, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    }

    RtlInitAnsiString(&Expression, "**");
    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    }
    RtlInitAnsiString(&Name, "a");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "ntdll.dll");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "~1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "ntdll.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "smss.exe");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "~1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "ntdll.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "NTDLL.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "nt??krnl.???");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "he*o");
    RtlInitAnsiString(&Name, "hello");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "helo");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "hella");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "he*");
    RtlInitAnsiString(&Name, "hello");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "helo");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "hella");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "*.cpl");
    RtlInitAnsiString(&Name, "kdcom.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "bootvid.dll");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, ".");
    RtlInitAnsiString(&Name, "NTDLL.DLL");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "F0_*.*");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "SETUP.EXE");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "F0_001");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "*.TTF");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "SETUP.INI");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "*");
    RtlInitAnsiString(&Name, ".");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "..");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "SETUP.INI");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "\"ntoskrnl.exe");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "ntoskrnl\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "ntoskrn\".exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "ntoskrn\"\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "ntoskrnl.\"exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "ntoskrnl.exe\"");
    RtlInitAnsiString(&Name, "ntoskrnl.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "ntoskrnl.exe.");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "*.c.d");
    RtlInitAnsiString(&Name, "a.b.c.d");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "*.?.c.d");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    }
    RtlInitAnsiString(&Name, "a");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "aa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "aaa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "?*?");
    if (!KmtIsCheckedBuild)
    {
        RtlInitAnsiString(&Name, "");
        ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    }
    RtlInitAnsiString(&Name, "a");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "aa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "aaa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "aaaa");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    /* Tests from #5923 */
    RtlInitAnsiString(&Expression, "C:\\ReactOS\\**");
    RtlInitAnsiString(&Name, "C:\\ReactOS\\dings.bmp");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "C:\\ReactOS\\***");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "C:\\Windows\\*a*");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "C:\\ReactOS\\*.bmp");
    RtlInitAnsiString(&Name, "C:\\Windows\\explorer.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "*.bmp;*.dib");
    RtlInitAnsiString(&Name, "winhlp32.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    /* Backtracking tests */
    RtlInitAnsiString(&Expression, "*.*.*.*");
    RtlInitAnsiString(&Name, "127.0.0.1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "*?*?*?*");
    RtlInitAnsiString(&Name, "1.0.0.1");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "?*?*?*?");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "?.?.?.?");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "*a*ab*abc");
    RtlInitAnsiString(&Name, "aabaabcdadabdabc");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    /* Tests for extra wildcards */
    RtlInitAnsiString(&Expression, "ab<exe");
    RtlInitAnsiString(&Name, "abcd.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "ab.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "abcdexe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "acd.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "a.b<exe");
    RtlInitAnsiString(&Name, "a.bcd.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Expression, "a<b.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "a.b.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");

    RtlInitAnsiString(&Expression, "abc.exe\"");
    RtlInitAnsiString(&Name, "abc.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "abc.exe.");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "abc.exe.back");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "abc.exes");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");

    RtlInitAnsiString(&Expression, "a>c.exe");
    RtlInitAnsiString(&Name, "abc.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == TRUE, "expected TRUE, got FALSE\n");
    RtlInitAnsiString(&Name, "ac.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Expression, "a>>>exe");
    RtlInitAnsiString(&Name, "abc.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
    RtlInitAnsiString(&Name, "ac.exe");
    ok(FsRtlIsDbcsInExpression(&Expression, &Name) == FALSE, "expected FALSE, got TRUE\n");
}

START_TEST(FsRtlExpression)
{
    FsRtlIsNameInExpressionTest();
    FsRtlIsDbcsInExpressionTest();
}
