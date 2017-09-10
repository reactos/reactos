/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for FindFirstFile's wildcard substitution
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#include "kernel32_test.h"

START_TEST(FindFile)
{
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;
    struct
    {
        PCWSTR Expression;
        PCWSTR ExpectedExpression;
    } Tests[] =
    {
        { L"Hello",     L"Hello" },

        { L"*",         L"*" },
        { L"a*",        L"a*" },
        { L"*a",        L"*a" },
        { L"a*a",       L"a*a" },
        { L"**",        L"**" },
        { L"*a*",       L"*a*" },
        { L"a*a*a",     L"a*a*a" },

        { L"*.*",       L"*" },
        { L"a*.*",      L"a<\"*" },
        { L"*.*a",      L"<\"*a" },
        { L"a*.*a",     L"a<\"*a" },
        { L"*.**.*",    L"<\"*<\"*" },
        { L"*.*a*.*",   L"<\"*a<\"*" },
        { L"a*.*a*.*a", L"a<\"*a<\"*a" },

        { L".*",        L"\"*" },
        { L"a.*",       L"a\"*" },
        { L".*a",       L"\"*a" },
        { L"a.*a",      L"a\"*a" },
        { L".*.*",      L"\"<\"*" },
        { L".*a.*",     L"\"*a\"*" },
        { L"a.*a.*a",   L"a\"*a\"*a" },

        { L"*.",        L"<" },
        { L"a*.",       L"a<" },
        { L"*.a",       L"<.a" },
        { L"a*.a",      L"a<.a" },
        { L"*.*.",      L"*" },
        { L"*.a*.",     L"<.a<" },
        { L"a*.a*.a",   L"a<.a<.a" },

        { L"?",         L">" },
        { L"a?",        L"a>" },
        { L"?a",        L">a" },
        { L"a?a",       L"a>a" },
        { L"??",        L">>" },
        { L"?a?",       L">a>" },
        { L"a?a?a",     L"a>a>a" },

        { L"?.?",        L">\">" },
        { L"a?.?",       L"a>\">" },
        { L"?.?a",       L">\">a" },
        { L"a?.?a",      L"a>\">a" },
        { L"?.??.?",     L">\">>\">" },
        { L"?.?a?.?",    L">\">a>\">" },
        { L"a?.?a?.?a",  L"a>\">a>\">a" },

        { L".?",         L"\">" },
        { L"a.?",        L"a\">" },
        { L".?a",        L"\">a" },
        { L"a.?a",       L"a\">a" },
        { L".?.?",       L"\">\">" },
        { L".?a.?",      L"\">a\">" },
        { L"a.?a.?a",    L"a\">a\">a" },

        { L"?.",         L">" },
        { L"a?.",        L"a>" },
        { L"?.a",        L">.a" },
        { L"a?.a",       L"a>.a" },
        { L"?.?.",       L">\">" },
        { L"?.a?.",      L">.a>" },
        { L"a?.a?.a",    L"a>.a>.a" },

        { L"f*.",       L"f<" },
        { L"f.*",       L"f\"*" },
        { L"f*.*",      L"f<\"*" },
        { L"f*.f*",     L"f<.f*" },
        { L"f*f.*",     L"f*f\"*" },
        { L"f*.*f",     L"f<\"*f" },

        /* TODO: add more. Have fun */
    };
    const INT TestCount = sizeof(Tests) / sizeof(Tests[0]);
    INT i;
    WCHAR ExpressionBuffer[MAX_PATH];

    KmtLoadDriver(L"kernel32", FALSE);
    KmtOpenDriver();

    for (i = 0; i < TestCount; i++)
    {
        trace("[%d] '%ls', '%ls'\n", i, Tests[i].Expression, Tests[i].ExpectedExpression);
        KmtSendWStringToDriver(IOCTL_EXPECT_EXPRESSION, Tests[i].ExpectedExpression);
        wcscpy(ExpressionBuffer, L"\\\\.\\Global\\GLOBALROOT\\Device\\Kmtest-kernel32\\");
        wcscat(ExpressionBuffer, Tests[i].Expression);
        FindHandle = FindFirstFileW(ExpressionBuffer, &FindData);
        ok(FindHandle != NULL && FindHandle != INVALID_HANDLE_VALUE, "Handle: %p, error=%lu\n", (PVOID)FindHandle, GetLastError());
        if (FindHandle != INVALID_HANDLE_VALUE)
            FindClose(FindHandle);
    }

    KmtCloseDriver();
    KmtUnloadDriver();
}
