/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for FsRtlIsNameInExpression/FsRtlIsDbcsInExpression
 * PROGRAMMER:      Pierre Schweitzer <pierre.schweitzer@reactos.org>
 *                  Víctor Martínez Calvo <vmartinez@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

struct
{   /* API parameters */
    PCWSTR Expression;
    PCWSTR Name;
    BOOLEAN IgnoreCase;
    /* Flag for tests which shouldn't be tested in checked builds */
    BOOLEAN AssertsInChecked;
    /* Expected test result */
    BOOLEAN Expected;
} Tests[] =
{
    /* TODO: reorganize AssertsInChecked. This needs to be separate for *Name* and *Dbcs* or something.
     * We currently fail a lot of them in Windows despite AssertsInChecked.
     * E.g. *Dbcs* asserts !islower(Expression[i]) and FsRtlDoesDbcsContainWildCards(Expression) for some reason.
     */
    { L"",                      L"",                            FALSE,  TRUE,   TRUE },
    { L"",                      L"a",                           FALSE,  TRUE,   FALSE },
    { L"*",                     L"a",                           FALSE,  TRUE,   TRUE },
    { L"*",                     L"",                            FALSE,  TRUE,   FALSE },
    { L"**",                    L"",                            FALSE,  TRUE,   FALSE },
    { L"**",                    L"a",                           FALSE,  FALSE,  TRUE },
    { L"ntdll.dll",             L".",                           FALSE,  TRUE,   FALSE },
    { L"ntdll.dll",             L"~1",                          FALSE,  TRUE,   FALSE },
    { L"ntdll.dll",             L"..",                          FALSE,  TRUE,   FALSE },
    { L"ntdll.dll",             L"ntdll.dll",                   FALSE,  TRUE,   TRUE },
    { L"smss.exe",              L".",                           FALSE,  TRUE,   FALSE },
    { L"smss.exe",              L"~1",                          FALSE,  TRUE,   FALSE },
    { L"smss.exe",              L"..",                          FALSE,  TRUE,   FALSE },
    { L"smss.exe",              L"ntdll.dll",                   FALSE,  TRUE,   FALSE },
    { L"smss.exe",              L"NTDLL.DLL",                   FALSE,  TRUE,   FALSE },
    { L"nt??krnl.???",          L"ntoskrnl.exe",                FALSE,  FALSE,  TRUE },
    { L"he*o",                  L"hello",                       FALSE,  FALSE,  TRUE },
    { L"he*o",                  L"helo",                        FALSE,  FALSE,  TRUE },
    { L"he*o",                  L"hella",                       FALSE,  FALSE,  FALSE },
    { L"he*",                   L"hello",                       FALSE,  FALSE,  TRUE },
    { L"he*",                   L"helo",                        FALSE,  FALSE,  TRUE },
    { L"he*",                   L"hella",                       FALSE,  FALSE,  TRUE },
    { L"*.cpl",                 L"kdcom.dll",                   FALSE,  FALSE,  FALSE },
    { L"*.cpl",                 L"bootvid.dll",                 FALSE,  FALSE,  FALSE },
    { L"*.cpl",                 L"ntoskrnl.exe",                FALSE,  FALSE,  FALSE },
    { L".",                     L"NTDLL.DLL",                   FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L".",                           FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"..",                          FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"SETUP.EXE",                   FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"f0_",                         FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"f0_",                         TRUE,   FALSE,  FALSE },
    { L"F0_*.*",                L"F0_",                         FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"f0_.",                        FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"f0_.",                        TRUE,   FALSE,  TRUE },
    { L"F0_*.*",                L"F0_.",                        FALSE,  FALSE,  TRUE },
    { L"F0_*.*",                L"F0_001",                      FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"F0_001",                      TRUE,   FALSE,  FALSE },
    { L"F0_*.*",                L"f0_001",                      FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"f0_001",                      TRUE,   FALSE,  FALSE },
    { L"F0_*.*",                L"F0_001.",                     FALSE,  FALSE,  TRUE },
    { L"F0_*.*",                L"f0_001.txt",                  FALSE,  FALSE,  FALSE },
    { L"F0_*.*",                L"f0_001.txt",                  TRUE,   FALSE,  TRUE },
    { L"F0_*.*",                L"F0_001.txt",                  FALSE,  FALSE,  TRUE },
    { L"F0_*.*",                L"F0_001.txt",                  TRUE,   FALSE,  TRUE },
    { L"F0_*.",                 L".",                           FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"..",                          FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"SETUP.EXE",                   FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"f0_",                         FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"f0_",                         TRUE,   FALSE,  FALSE },
    { L"F0_*.",                 L"F0_",                         FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"f0_.",                        FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"f0_.",                        TRUE,   FALSE,  TRUE },
    { L"F0_*.",                 L"F0_.",                        FALSE,  FALSE,  TRUE },
    { L"F0_*.",                 L"F0_001",                      FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"F0_001",                      TRUE,   FALSE,  FALSE },
    { L"F0_*.",                 L"f0_001",                      FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"f0_001",                      TRUE,   FALSE,  FALSE },
    { L"F0_*.",                 L"F0_001.",                     FALSE,  FALSE,  TRUE },
    { L"F0_*.",                 L"f0_001.txt",                  FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"f0_001.txt",                  TRUE,   FALSE,  FALSE },
    { L"F0_*.",                 L"F0_001.txt",                  FALSE,  FALSE,  FALSE },
    { L"F0_*.",                 L"F0_001.txt",                  TRUE,   FALSE,  FALSE },
    { L"F0_<\"*",               L".",                           FALSE,  FALSE,  FALSE },
    { L"F0_<\"*",               L"..",                          FALSE,  FALSE,  FALSE },
    { L"F0_<\"*",               L"SETUP.EXE",                   FALSE,  FALSE,  FALSE },
    { L"F0_<\"*",               L"f0_",                         TRUE,   FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_",                         FALSE,  FALSE,  TRUE },
    { L"F0_<\"*",               L"f0_.",                        FALSE,  FALSE,  FALSE },
    { L"F0_<\"*",               L"f0_.",                        TRUE,   FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_.",                        FALSE,  FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_001",                      FALSE,  FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_001",                      TRUE,   FALSE,  TRUE },
    { L"F0_<\"*",               L"f0_001",                      FALSE,  FALSE,  FALSE },
    { L"F0_<\"*",               L"f0_001",                      TRUE,   FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_001.",                     FALSE,  FALSE,  TRUE },
    { L"F0_<\"*",               L"f0_001.txt",                  FALSE,  FALSE,  FALSE },
    { L"F0_<\"*",               L"f0_001.txt",                  TRUE,   FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_001.txt",                  FALSE,  FALSE,  TRUE },
    { L"F0_<\"*",               L"F0_001.txt",                  TRUE,   FALSE,  TRUE },
    { L"*.TTF",                 L".",                           FALSE,  FALSE,  FALSE },
    { L"*.TTF",                 L"..",                          FALSE,  FALSE,  FALSE },
    { L"*.TTF",                 L"SETUP.INI",                   FALSE,  FALSE,  FALSE },
    { L"*",                     L".",                           FALSE,  FALSE,  TRUE },
    { L"*",                     L"..",                          FALSE,  FALSE,  TRUE },
    { L"*",                     L"SETUP.INI",                   FALSE,  FALSE,  TRUE },
    { L".*",                    L"1",                           FALSE,  FALSE,  FALSE },
    { L".*",                    L"01",                          FALSE,  FALSE,  FALSE },
    { L".*",                    L" ",                           FALSE,  FALSE,  FALSE },
    { L".*",                    L"",                            FALSE,  TRUE,   FALSE },
    { L".*",                    L".",                           FALSE,  FALSE,  TRUE },
    { L".*",                    L"1.txt",                       FALSE,  FALSE,  FALSE },
    { L".*",                    L" .txt",                       FALSE,  FALSE,  FALSE },
    { L".*",                    L".txt",                        FALSE,  FALSE,  TRUE },
    { L"\"ntoskrnl.exe",        L"ntoskrnl.exe",                FALSE,  FALSE,  FALSE },
    { L"ntoskrnl\"exe",         L"ntoskrnl.exe",                FALSE,  FALSE,  TRUE },
    { L"ntoskrn\".exe",         L"ntoskrnl.exe",                FALSE,  FALSE,  FALSE },
    { L"ntoskrn\"\"exe",        L"ntoskrnl.exe",                FALSE,  FALSE,  FALSE },
    { L"ntoskrnl.\"exe",        L"ntoskrnl.exe",                FALSE,  FALSE,  FALSE },
    { L"ntoskrnl.exe\"",        L"ntoskrnl.exe",                FALSE,  FALSE,  TRUE },
    { L"ntoskrnl.exe",          L"ntoskrnl.exe",                FALSE,  FALSE,  TRUE },
    { L"*.c.d",                 L"a.b.c.d",                     FALSE,  FALSE,  TRUE },
    { L"*.?.c.d",               L"a.b.c.d",                     FALSE,  FALSE,  TRUE },
    { L"**.?.c.d",              L"a.b.c.d",                     FALSE,  FALSE,  TRUE },
    { L"a.**.c.d",              L"a.b.c.d",                     FALSE,  FALSE,  TRUE },
    { L"a.b.??.d",              L"a.b.c1.d",                    FALSE,  FALSE,  TRUE },
    { L"a.b.??.d",              L"a.b.c.d",                     FALSE,  FALSE,  FALSE },
    { L"a.b.*?.d",              L"a.b.c.d",                     FALSE,  FALSE,  TRUE },
    { L"a.b.*??.d",             L"a.b.ccc.d",                   FALSE,  FALSE,  TRUE },
    { L"a.b.*??.d",             L"a.b.cc.d",                    FALSE,  FALSE,  TRUE },
    { L"a.b.*??.d",             L"a.b.c.d",                     FALSE,  FALSE,  FALSE },
    { L"a.b.*?*.d",             L"a.b.c.d",                     FALSE,  FALSE,  TRUE },
    { L"*?",                    L"",                            FALSE,  TRUE,   FALSE },
    { L"*?",                    L"a",                           FALSE,  FALSE,  TRUE },
    { L"*?",                    L"aa",                          FALSE,  FALSE,  TRUE },
    { L"*?",                    L"aaa",                         FALSE,  FALSE,  TRUE },
    { L"?*?",                   L"",                            FALSE,  TRUE,   FALSE },
    { L"?*?",                   L"a",                           FALSE,  FALSE,  FALSE },
    { L"?*?",                   L"aa",                          FALSE,  FALSE,  TRUE },
    { L"?*?",                   L"aaa",                         FALSE,  FALSE,  TRUE },
    { L"?*?",                   L"aaaa",                        FALSE,  FALSE,  TRUE },
    { L"C:\\ReactOS\\**",       L"C:\\ReactOS\\dings.bmp",      FALSE,  FALSE,  TRUE },
    { L"C:\\ReactOS\\***",      L"C:\\ReactOS\\dings.bmp",      FALSE,  FALSE,  TRUE },
    { L"C:\\Windows\\*a*",      L"C:\\ReactOS\\dings.bmp",      FALSE,  FALSE,  FALSE },
    { L"C:\\ReactOS\\*.bmp",    L"C:\\Windows\\explorer.exe",   FALSE,  FALSE,  FALSE },
    { L"*.bmp;*.dib",           L"winhlp32.exe",                FALSE,  FALSE,  FALSE },
    { L"*.*.*.*",               L"127.0.0.1",                   FALSE,  FALSE,  TRUE },
    { L"*?*?*?*",               L"1.0.0.1",                     FALSE,  FALSE,  TRUE },
    { L"?*?*?*?",               L"1.0.0.1",                     FALSE,  FALSE,  TRUE },
    { L"?.?.?.?",               L"1.0.0.1",                     FALSE,  FALSE,  TRUE },
    { L"*a*ab*abc",             L"aabaabcdadabdabc",            FALSE,  FALSE,  TRUE },
    { L"ab<exe",                L"abcd.exe",                    FALSE,  FALSE,  TRUE },
    { L"ab<exe",                L"ab.exe",                      FALSE,  FALSE,  TRUE },
    { L"ab<exe",                L"abcdexe",                     FALSE,  FALSE,  TRUE },
    { L"ab<exe",                L"acd.exe",                     FALSE,  FALSE,  FALSE },
    { L"a.b<exe",               L"a.bcd.exe",                   FALSE,  FALSE,  TRUE },
    { L"a<b.exe",               L"a.bcd.exe",                   FALSE,  FALSE,  FALSE },
    { L"a.b.exe",               L"a.bcd.exe",                   FALSE,  FALSE,  FALSE },
    { L"abc.exe",               L"abc.exe",                     FALSE,  FALSE,  TRUE },
    { L"abc.exe",               L"abc.exe.",                    FALSE,  FALSE,  FALSE },
    { L"abc.exe",               L"abc.exe.back",                FALSE,  FALSE,  FALSE },
    { L"abc.exe",               L"abc.exes",                    FALSE,  FALSE,  FALSE },
    { L"a>c.exe",               L"abc.exe",                     FALSE,  FALSE,  TRUE },
    { L"a>c.exe",               L"ac.exe",                      FALSE,  FALSE,  FALSE },
    { L"a>>>exe",               L"abc.exe",                     FALSE,  FALSE,  FALSE },
    { L"a>>>exe",               L"ac.exe",                      FALSE,  FALSE,  FALSE },
    { L"<.exe",                 L"test.exe",                    FALSE,  FALSE,  TRUE },
    { L"<.EXE",                 L"test.exe",                    TRUE,   FALSE,  TRUE },
};

static VOID FsRtlIsNameInExpressionTest()
{
    ULONG i;
    for (i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++)
    {
        BOOLEAN TestResult;
        UNICODE_STRING Expression;
        UNICODE_STRING Name;

        RtlInitUnicodeString(&Expression, Tests[i].Expression);
        RtlInitUnicodeString(&Name, Tests[i].Name);

        TestResult = FsRtlIsNameInExpression(&Expression, &Name, Tests[i].IgnoreCase, NULL);

        ok(TestResult == Tests[i].Expected, "FsRtlIsNameInExpression(%wZ,%wZ,%s,NULL): Expected %s, got %s\n",
           &Expression, &Name, Tests[i].IgnoreCase ? "TRUE" : "FALSE", Tests[i].Expected ? "TRUE" : "FALSE", TestResult ? "TRUE" : "FALSE");
    }

    /* TODO: test UpcaseTable */
}

static VOID FsRtlIsDbcsInExpressionTest()
{
    ULONG i;
    for (i = 0; i < sizeof(Tests) / sizeof(Tests[0]); i++)
    {
        BOOLEAN TestResult;
        UNICODE_STRING UExpression;
        UNICODE_STRING UName;
        ANSI_STRING Expression;
        ANSI_STRING Name;

        /* Don't run Tests which are known to assert in checked builds */
        if (KmtIsCheckedBuild && Tests[i].AssertsInChecked)
            continue;

        /* Ignore Tests flagged IgnoreCase==TRUE to avoid duplicated testing */
        if (Tests[i].IgnoreCase)
            continue;

        RtlInitUnicodeString(&UExpression, Tests[i].Expression);
        RtlInitUnicodeString(&UName, Tests[i].Name);

        RtlUnicodeStringToAnsiString(&Expression, &UExpression, TRUE);
        RtlUnicodeStringToAnsiString(&Name, &UName, TRUE);

        TestResult = FsRtlIsDbcsInExpression(&Expression, &Name);

        ok(TestResult == Tests[i].Expected, "FsRtlIsDbcsInExpression(%Z,%Z): Expected %s, got %s\n",
           &Expression, &Name, Tests[i].Expected ? "TRUE" : "FALSE", TestResult ? "TRUE" : "FALSE");

        RtlFreeAnsiString(&Expression);
        RtlFreeAnsiString(&Name);
    }
}

START_TEST(FsRtlExpression)
{
    FsRtlIsNameInExpressionTest();
    FsRtlIsDbcsInExpressionTest();
}
