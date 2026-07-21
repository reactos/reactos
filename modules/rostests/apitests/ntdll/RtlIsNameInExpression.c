/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for RtlIsNameInExpression
 * COPYRIGHT:   Copyright 2026 Carl Bialorucki <carl.bialorucki@reactos.org>
 */

#include "precomp.h"

/* From SDK */
static BOOLEAN (NTAPI *pRtlIsNameInExpression)(
    _In_     PUNICODE_STRING Expression,
    _In_     PUNICODE_STRING Name,
    _In_     BOOLEAN IgnoreCase,
    _In_opt_ PWCH UpcaseTable
);

/* For your convenience, a normal ASCII table is provided here.
 *
 *  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
 *  0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
 *  ' ' ,'!' ,'"' ,'#' ,'$' ,'%' ,'&' ,'\'','(' ,')' ,'*' ,'+' ,',' ,'-' ,'.' ,'/' ,
 *  '0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,':' ,';' ,'<' ,'=' ,'>' ,'?' ,
 *  '@' ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
 *  'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,'[' ,'\\',']' ,'^' ,'_' ,
 *  '`' ,'a' ,'b' ,'c' ,'d' ,'e' ,'f' ,'g' ,'h' ,'i' ,'j' ,'k' ,'l' ,'m' ,'n' ,'o' ,
 *  'p' ,'q' ,'r' ,'s' ,'t' ,'u' ,'v' ,'w' ,'x' ,'y' ,'z' ,'{' ,'|' ,'}' ,'~' ,0x7F,
 */

static const WCHAR UpcaseTable[256] =
{
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    ' ' ,'!' ,'"' ,'#' ,'$' ,'%' ,'&' ,'\'','(' ,')' ,'*' ,'+' ,',' ,'-' ,'.' ,'/' ,
    '0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,':' ,';' ,'<' ,'=' ,'>' ,'?' ,
    '@' ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
    'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,'[' ,'\\',']' ,'^' ,'_' ,
    '`' ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
    'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,'{' ,'|' ,'}' ,'~' ,0x7F,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
    0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
    0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
    0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
    0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
    0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xFF,
};

/* Bad upcase tables */

static const WCHAR CustomUpcaseTable1[128] =
{
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
    ' ' ,'!' ,'"' ,'#' ,'$' ,'%' ,'&' ,'\'','(' ,')' ,'*' ,'+' ,',' ,'-' ,'.' ,'/' ,
    '0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,':' ,';' ,'<' ,'=' ,'>' ,'?' ,
    '@' ,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
    'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,'[' ,'\\',']' ,'^' ,'_' ,
    '`' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,'P' ,
    'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,'A' ,'{' ,'|' ,'}' ,'~' ,0x7F,
};

typedef struct _RtlExpressionTest
{
    PWCH    Expression;
    PWCH    Name;
    BOOLEAN IgnoreCase;
    PCWCH   UpcaseTable;
    BOOLEAN Expected;
} RtlExpressionTest;

RtlExpressionTest RtlExpressionTests[] =
{
    { L"",                   L"",                           FALSE,  NULL,               TRUE  },
    { L"",                   L"a",                          FALSE,  NULL,               FALSE },
    { L"*",                  L"a",                          FALSE,  NULL,               TRUE  },
    { L"*",                  L"",                           FALSE,  NULL,               FALSE },
    { L"**",                 L"",                           FALSE,  NULL,               FALSE },
    { L"**",                 L"a",                          FALSE,  NULL,               TRUE  },
    { L"ntdll.dll",          L".",                          FALSE,  NULL,               FALSE },
    { L"ntdll.dll",          L"~1",                         FALSE,  NULL,               FALSE },
    { L"ntdll.dll",          L"..",                         FALSE,  NULL,               FALSE },
    { L"ntdll.dll",          L"ntdll.dll",                  FALSE,  NULL,               TRUE  },
    { L"smss.exe",           L".",                          FALSE,  NULL,               FALSE },
    { L"smss.exe",           L"~1",                         FALSE,  NULL,               FALSE },
    { L"smss.exe",           L"..",                         FALSE,  NULL,               FALSE },
    { L"smss.exe",           L"ntdll.dll",                  FALSE,  NULL,               FALSE },
    { L"smss.exe",           L"NTDLL.DLL",                  FALSE,  NULL,               FALSE },
    { L"nt??krnl.???",       L"ntoskrnl.exe",               FALSE,  NULL,               TRUE  },
    { L"he*o",               L"hello",                      FALSE,  NULL,               TRUE  },
    { L"he*o",               L"helo",                       FALSE,  NULL,               TRUE  },
    { L"he*o",               L"hella",                      FALSE,  NULL,               FALSE },
    { L"he*",                L"hello",                      FALSE,  NULL,               TRUE  },
    { L"he*",                L"helo",                       FALSE,  NULL,               TRUE  },
    { L"he*",                L"hella",                      FALSE,  NULL,               TRUE  },
    { L"*.cpl",              L"kdcom.dll",                  FALSE,  NULL,               FALSE },
    { L"*.cpl",              L"bootvid.dll",                FALSE,  NULL,               FALSE },
    { L"*.cpl",              L"ntoskrnl.exe",               FALSE,  NULL,               FALSE },
    { L".",                  L"NTDLL.DLL",                  FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L".",                          FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"..",                         FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"SETUP.EXE",                  FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"f0_",                        FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"f0_",                        TRUE,   NULL,               FALSE },
    { L"F0_*.*",             L"F0_",                        FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"f0_.",                       FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"f0_.",                       TRUE,   NULL,               TRUE  },
    { L"F0_*.*",             L"F0_.",                       FALSE,  NULL,               TRUE  },
    { L"F0_*.*",             L"F0_001",                     FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"F0_001",                     TRUE,   NULL,               FALSE },
    { L"F0_*.*",             L"f0_001",                     FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"f0_001",                     TRUE,   NULL,               FALSE },
    { L"F0_*.*",             L"F0_001.",                    FALSE,  NULL,               TRUE  },
    { L"F0_*.*",             L"f0_001.txt",                 FALSE,  NULL,               FALSE },
    { L"F0_*.*",             L"f0_001.txt",                 TRUE,   NULL,               TRUE  },
    { L"F0_*.*",             L"F0_001.txt",                 FALSE,  NULL,               TRUE  },
    { L"F0_*.*",             L"F0_001.txt",                 TRUE,   NULL,               TRUE  },
    { L"F0_*.",              L".",                          FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"..",                         FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"SETUP.EXE",                  FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"f0_",                        FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"f0_",                        TRUE,   NULL,               FALSE },
    { L"F0_*.",              L"F0_",                        FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"f0_.",                       FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"f0_.",                       TRUE,   NULL,               TRUE  },
    { L"F0_*.",              L"F0_.",                       FALSE,  NULL,               TRUE  },
    { L"F0_*.",              L"F0_001",                     FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"F0_001",                     TRUE,   NULL,               FALSE },
    { L"F0_*.",              L"f0_001",                     FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"f0_001",                     TRUE,   NULL,               FALSE },
    { L"F0_*.",              L"F0_001.",                    FALSE,  NULL,               TRUE  },
    { L"F0_*.",              L"f0_001.txt",                 FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"f0_001.txt",                 TRUE,   NULL,               FALSE },
    { L"F0_*.",              L"F0_001.txt",                 FALSE,  NULL,               FALSE },
    { L"F0_*.",              L"F0_001.txt",                 TRUE,   NULL,               FALSE },
    { L"F0_<\"*",            L".",                          FALSE,  NULL,               FALSE },
    { L"F0_<\"*",            L"..",                         FALSE,  NULL,               FALSE },
    { L"F0_<\"*",            L"SETUP.EXE",                  FALSE,  NULL,               FALSE },
    { L"F0_<\"*",            L"f0_",                        TRUE,   NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_",                        FALSE,  NULL,               TRUE  },
    { L"F0_<\"*",            L"f0_.",                       FALSE,  NULL,               FALSE },
    { L"F0_<\"*",            L"f0_.",                       TRUE,   NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_.",                       FALSE,  NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_001",                     FALSE,  NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_001",                     TRUE,   NULL,               TRUE  },
    { L"F0_<\"*",            L"f0_001",                     FALSE,  NULL,               FALSE },
    { L"F0_<\"*",            L"f0_001",                     TRUE,   NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_001.",                    FALSE,  NULL,               TRUE  },
    { L"F0_<\"*",            L"f0_001.txt",                 FALSE,  NULL,               FALSE },
    { L"F0_<\"*",            L"f0_001.txt",                 TRUE,   NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_001.txt",                 FALSE,  NULL,               TRUE  },
    { L"F0_<\"*",            L"F0_001.txt",                 TRUE,   NULL,               TRUE  },
    { L"*.TTF",              L".",                          FALSE,  NULL,               FALSE },
    { L"*.TTF",              L"..",                         FALSE,  NULL,               FALSE },
    { L"*.TTF",              L"SETUP.INI",                  FALSE,  NULL,               FALSE },
    { L"*",                  L".",                          FALSE,  NULL,               TRUE  },
    { L"*",                  L"..",                         FALSE,  NULL,               TRUE  },
    { L"*",                  L"SETUP.INI",                  FALSE,  NULL,               TRUE  },
    { L".*",                 L"1",                          FALSE,  NULL,               FALSE },
    { L".*",                 L"01",                         FALSE,  NULL,               FALSE },
    { L".*",                 L" ",                          FALSE,  NULL,               FALSE },
    { L".*",                 L"",                           FALSE,  NULL,               FALSE },
    { L".*",                 L".",                          FALSE,  NULL,               TRUE  },
    { L".*",                 L"1.txt",                      FALSE,  NULL,               FALSE },
    { L".*",                 L" .txt",                      FALSE,  NULL,               FALSE },
    { L".*",                 L".txt",                       FALSE,  NULL,               TRUE  },
    { L"\"ntoskrnl.exe",     L"ntoskrnl.exe",               FALSE,  NULL,               FALSE },
    { L"ntoskrnl\"exe",      L"ntoskrnl.exe",               FALSE,  NULL,               TRUE  },
    { L"ntoskrn\".exe",      L"ntoskrnl.exe",               FALSE,  NULL,               FALSE },
    { L"ntoskrn\"\"exe",     L"ntoskrnl.exe",               FALSE,  NULL,               FALSE },
    { L"ntoskrnl.\"exe",     L"ntoskrnl.exe",               FALSE,  NULL,               FALSE },
    { L"ntoskrnl.exe\"",     L"ntoskrnl.exe",               FALSE,  NULL,               TRUE  },
    { L"ntoskrnl.exe",       L"ntoskrnl.exe",               FALSE,  NULL,               TRUE  },
    { L"*.c.d",              L"a.b.c.d",                    FALSE,  NULL,               TRUE  },
    { L"*.?.c.d",            L"a.b.c.d",                    FALSE,  NULL,               TRUE  },
    { L"**.?.c.d",           L"a.b.c.d",                    FALSE,  NULL,               TRUE  },
    { L"a.**.c.d",           L"a.b.c.d",                    FALSE,  NULL,               TRUE  },
    { L"a.b.??.d",           L"a.b.c1.d",                   FALSE,  NULL,               TRUE  },
    { L"a.b.??.d",           L"a.b.c.d",                    FALSE,  NULL,               FALSE },
    { L"a.b.*?.d",           L"a.b.c.d",                    FALSE,  NULL,               TRUE  },
    { L"a.b.*??.d",          L"a.b.ccc.d",                  FALSE,  NULL,               TRUE  },
    { L"a.b.*??.d",          L"a.b.cc.d",                   FALSE,  NULL,               TRUE  },
    { L"a.b.*??.d",          L"a.b.c.d",                    FALSE,  NULL,               FALSE },
    { L"a.b.*?*.d",          L"a.b.c.d",                    FALSE,  NULL,               TRUE  },
    { L"*?",                 L"",                           FALSE,  NULL,               FALSE },
    { L"*?",                 L"a",                          FALSE,  NULL,               TRUE  },
    { L"*?",                 L"aa",                         FALSE,  NULL,               TRUE  },
    { L"*?",                 L"aaa",                        FALSE,  NULL,               TRUE  },
    { L"?*?",                L"",                           FALSE,  NULL,               FALSE },
    { L"?*?",                L"a",                          FALSE,  NULL,               FALSE },
    { L"?*?",                L"aa",                         FALSE,  NULL,               TRUE  },
    { L"?*?",                L"aaa",                        FALSE,  NULL,               TRUE  },
    { L"?*?",                L"aaaa",                       FALSE,  NULL,               TRUE  },
    { L"C:\\ReactOS\\**",    L"C:\\ReactOS\\dings.bmp",     FALSE,  NULL,               TRUE  },
    { L"C:\\ReactOS\\***",   L"C:\\ReactOS\\dings.bmp",     FALSE,  NULL,               TRUE  },
    { L"C:\\Windows\\*a*",   L"C:\\ReactOS\\dings.bmp",     FALSE,  NULL,               FALSE },
    { L"C:\\ReactOS\\*.bmp", L"C:\\Windows\\explorer.exe",  FALSE,  NULL,               FALSE },
    { L"*.bmp;*.dib",        L"winhlp32.exe",               FALSE,  NULL,               FALSE },
    { L"*.*.*.*",            L"127.0.0.1",                  FALSE,  NULL,               TRUE  },
    { L"*?*?*?*",            L"1.0.0.1",                    FALSE,  NULL,               TRUE  },
    { L"?*?*?*?",            L"1.0.0.1",                    FALSE,  NULL,               TRUE  },
    { L"?.?.?.?",            L"1.0.0.1",                    FALSE,  NULL,               TRUE  },
    { L"*a*ab*abc",          L"aabaabcdadabdabc",           FALSE,  NULL,               TRUE  },
    { L"ab<exe",             L"abcd.exe",                   FALSE,  NULL,               TRUE  },
    { L"ab<exe",             L"ab.exe",                     FALSE,  NULL,               TRUE  },
    { L"ab<exe",             L"abcdexe",                    FALSE,  NULL,               TRUE  },
    { L"ab<exe",             L"acd.exe",                    FALSE,  NULL,               FALSE },
    { L"a.b<exe",            L"a.bcd.exe",                  FALSE,  NULL,               TRUE  },
    { L"a<b.exe",            L"a.bcd.exe",                  FALSE,  NULL,               FALSE },
    { L"a.b.exe",            L"a.bcd.exe",                  FALSE,  NULL,               FALSE },
    { L"abc.exe",            L"abc.exe",                    FALSE,  NULL,               TRUE  },
    { L"abc.exe",            L"abc.exe.",                   FALSE,  NULL,               FALSE },
    { L"abc.exe",            L"abc.exe.back",               FALSE,  NULL,               FALSE },
    { L"abc.exe",            L"abc.exes",                   FALSE,  NULL,               FALSE },
    { L"a>c.exe",            L"abc.exe",                    FALSE,  NULL,               TRUE  },
    { L"a>c.exe",            L"ac.exe",                     FALSE,  NULL,               FALSE },
    { L"a>>>exe",            L"abc.exe",                    FALSE,  NULL,               FALSE },
    { L"a>>>exe",            L"ac.exe",                     FALSE,  NULL,               FALSE },
    { L"<.exe",              L"test.exe",                   FALSE,  NULL,               TRUE  },
    { L"<.EXE",              L"test.exe",                   TRUE,   NULL,               TRUE  },
    { L"*_MICROSOFT.WINDOWS.COMMON-CONTROLS_6595B64144CCF1DF_6.0.*.*_*_*.MANIFEST",
      L"X86_MICROSOFT.VC90.ATL_1FC8B3B9A1E18E3B_9.0.30729.6161_X-WW_92453BB7.CAT",
                                                            FALSE,  NULL,               FALSE },
    { L"FI<<<<<<<<",         L"FILE",                       FALSE,  NULL,               TRUE  },
    { L"<<<<<<<<<.<",        L".",                          FALSE,  NULL,               TRUE  },
    { L"F<",                 L"FILE.TXT",                   FALSE,  NULL,               FALSE },
    { L"B",                  L"a",                          TRUE,   CustomUpcaseTable1, TRUE  },
    { L"B.TXT",              L"a.sws",                      TRUE,   CustomUpcaseTable1, TRUE  },
    { L"*",                  L"a",                          TRUE,   CustomUpcaseTable1, TRUE  },
    { L"B?",                 L"ax",                         TRUE,   CustomUpcaseTable1, TRUE  },
    { L"BB",                 L"aa",                         TRUE,   CustomUpcaseTable1, TRUE  },
    { L"A",                  L"a",                          TRUE,   CustomUpcaseTable1, FALSE },
    { L"Z",                  L"z",                          TRUE,   CustomUpcaseTable1, FALSE },
    { L"B",                  L"A",                          TRUE,   CustomUpcaseTable1, FALSE },
    { L"AB",                 L"ab",                         TRUE,   CustomUpcaseTable1, FALSE },
    { L"AB",                 L"ba",                         TRUE,   CustomUpcaseTable1, FALSE },
    { L"\xC9",               L"\xE9",                       TRUE,   NULL,               TRUE  },
    { L"\xD1*",              L"\xF1XYZ",                    TRUE,   NULL,               TRUE  },
    { L"A\u00D1B",           L"a\u00F1b",                   TRUE,   NULL,               TRUE  },
    { L"\xD6",               L"\xF6",                       TRUE,   NULL,               TRUE  },
    { L"\xDC",               L"\xFC",                       TRUE,   NULL,               TRUE  },
    { L"\x9E",               L"\xDF",                       TRUE,   NULL,               FALSE },
    { L"\xA9",               L"\xAE",                       TRUE,   NULL,               FALSE },
    { L"\xC9",               L"\xE8",                       TRUE,   NULL,               FALSE },
    { L"\xC9",               L"\xE9",                       FALSE,  NULL,               FALSE },
    { L"A\xD1B",             L"a\xF1C",                     TRUE,   NULL,               FALSE },
    { L"*",                  L"*",                          FALSE,  NULL,               TRUE  },
    { L"A*Z",                L"A*Z",                        FALSE,  NULL,               TRUE  },
    { L"A<Z",                L"A<Z",                        FALSE,  NULL,               TRUE  },
    { L"*Z",                 L"**Z",                        FALSE,  NULL,               TRUE  },
    { L"A<TXT",              L"A<.TXT",                     FALSE,  NULL,               TRUE  },
    { L"A*Z",                L"A*Y",                        FALSE,  NULL,               FALSE },
    { L"A<Z",                L"A<Y",                        FALSE,  NULL,               FALSE },
    { L"A?Z",                L"A**Z",                       FALSE,  NULL,               FALSE },
    { L"*Z",                 L"*Y",                         FALSE,  NULL,               FALSE },
    { L"A<TXT",              L"A<.DOC",                     FALSE,  NULL,               FALSE },
    { L"A?C",                L"A?C",                        FALSE,  NULL,               TRUE  },
    { L"A*C",                L"A<>\"C",                     FALSE,  NULL,               TRUE  },
    { L"A\\B",               L"A\\B",                       FALSE,  NULL,               TRUE  },
    { L"A/B",                L"A/B",                        FALSE,  NULL,               TRUE  },
    { L"A*B",                L"A>B",                        FALSE,  NULL,               TRUE  },
    { L"A?C",                L"A??C",                       FALSE,  NULL,               FALSE },
    { L"A\\B",               L"A/B",                        FALSE,  NULL,               FALSE },
    { L"A/B",                L"AB",                         FALSE,  NULL,               FALSE },
    { L"A*C",                L"A<>D",                       FALSE,  NULL,               FALSE },
    { L"A?B",                L"AB",                         FALSE,  NULL,               FALSE },
};

#define RtlIsNameInExpressionOk(Test, TestResult) \
do { ok(TestResult == Test.Expected,                                    \
        "RtlIsNameInExpression(%S, %S, %s, %s): Expected %s, got %s\n", \
        Test.Expression,                                                \
        Test.Name,                                                      \
        Test.IgnoreCase ? "TRUE" : "FALSE",                             \
        Test.UpcaseTable ? "NOT NULL" : "NULL",                         \
        Test.Expected ? "TRUE" : "FALSE",                               \
        TestResult ? "TRUE" : "FALSE");                                 \
} while(0)

static void RtlIsNameInExpressionTest()
{
    if (!pRtlIsNameInExpression)
    {
        skip("RtlIsNameInExpression unavailable\n");
        return;
    }

    for (ULONG i = 0; i < ARRAYSIZE(RtlExpressionTests); i++)
    {
        BOOLEAN Result;
        UNICODE_STRING Expression, Name;

        RtlInitUnicodeString(&Expression, RtlExpressionTests[i].Expression);
        RtlInitUnicodeString(&Name, RtlExpressionTests[i].Name);

        Result = pRtlIsNameInExpression(&Expression, &Name, RtlExpressionTests[i].IgnoreCase,
                                        (PWCH)RtlExpressionTests[i].UpcaseTable);
        RtlIsNameInExpressionOk(RtlExpressionTests[i], Result);

        if (RtlExpressionTests[i].UpcaseTable == NULL)
        {
            /* System upcase table tests should also pass with a normal upcase table. */
            Result = pRtlIsNameInExpression(&Expression, &Name, RtlExpressionTests[i].IgnoreCase, (PWCH)UpcaseTable);
            RtlIsNameInExpressionOk(RtlExpressionTests[i], Result);
        }
    }
}

START_TEST(RtlIsNameInExpression)
{
    HMODULE hdll = GetModuleHandleW(L"ntdll.dll");
    pRtlIsNameInExpression = (PVOID)GetProcAddress(hdll, "RtlIsNameInExpression");
    if (!pRtlIsNameInExpression)
    {
        hdll = GetModuleHandleW(L"ntdll_vista.dll");
        pRtlIsNameInExpression = (PVOID)GetProcAddress(hdll, "RtlIsNameInExpression");
    }
    RtlIsNameInExpressionTest();
}
