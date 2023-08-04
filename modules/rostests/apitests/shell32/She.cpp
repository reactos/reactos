/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for She* functions
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "shelltest.h"
#include <undocshell.h>

typedef LPSTR (WINAPI *FN_SheRemoveQuotesA)(LPSTR psz);
typedef LPWSTR (WINAPI *FN_SheRemoveQuotesW)(LPWSTR psz);

static FN_SheRemoveQuotesA pSheRemoveQuotesA = NULL;
static FN_SheRemoveQuotesW pSheRemoveQuotesW = NULL;

static void test_SheRemoveQuotesA(void)
{
    CHAR sz0[] = "A\"Test\"";
    CHAR sz1[] = "\"Test\"";
    CHAR sz2[] = "\"Test\"123";

    BOOL bGotException = FALSE;
    _SEH2_TRY
    {
        pSheRemoveQuotesA(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bGotException = TRUE;
    }
    _SEH2_END;
    ok_int(bGotException, TRUE);

    ok_ptr(pSheRemoveQuotesA(sz0), sz0);
    ok_str(sz0, "A\"Test\"");

    ok_ptr(pSheRemoveQuotesA(sz1), sz1);
    ok_str(sz1, "Test");

    ok_ptr(pSheRemoveQuotesA(sz2), sz2);
    ok_str(sz2, "Test");
}

static void test_SheRemoveQuotesW(void)
{
    WCHAR sz0[] = L"A\"Test\"";
    WCHAR sz1[] = L"\"Test\"";
    WCHAR sz2[] = L"\"Test\"123";

    BOOL bGotException = FALSE;
    _SEH2_TRY
    {
        pSheRemoveQuotesW(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bGotException = TRUE;
    }
    _SEH2_END;
    ok_int(bGotException, TRUE);

    ok_ptr(pSheRemoveQuotesW(sz0), sz0);
    ok_wstr(sz0, L"A\"Test\"");

    ok_ptr(pSheRemoveQuotesW(sz1), sz1);
    ok_wstr(sz1, L"Test");

    ok_ptr(pSheRemoveQuotesW(sz2), sz2);
    ok_wstr(sz2, L"Test");
}

START_TEST(She)
{
    HINSTANCE hShell32 = GetModuleHandleW(L"shell32");
    pSheRemoveQuotesA = (FN_SheRemoveQuotesA)GetProcAddress(hShell32, "SheRemoveQuotesA");
    pSheRemoveQuotesW = (FN_SheRemoveQuotesW)GetProcAddress(hShell32, "SheRemoveQuotesW");

    if (!pSheRemoveQuotesA || !pSheRemoveQuotesW)
    {
        skip("SheRemoveQuotes not found");
        return;
    }

    test_SheRemoveQuotesA();
    test_SheRemoveQuotesW();
}
