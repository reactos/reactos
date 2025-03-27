/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for SheGetPathOffsetW
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <shelltest.h>
#include <undocshell.h>
#include <versionhelpers.h>

typedef INT (WINAPI *FN_SheGetPathOffsetW)(PCWSTR);

static FN_SheGetPathOffsetW s_pSheGetPathOffsetW = NULL;

static void
Test_SheGetPathOffsetW(void)
{
    INT ret;

    ret = s_pSheGetPathOffsetW(NULL);
    ok_int(ret, -1);

    ret = s_pSheGetPathOffsetW(L"");
    ok_int(ret, -1);

    ret = s_pSheGetPathOffsetW(L"C");
    ok_int(ret, -1);

    ret = s_pSheGetPathOffsetW(L"C:");
    ok_int(ret, 2);
    ret = s_pSheGetPathOffsetW(L"C:\\");
    ok_int(ret, 2);
    ret = s_pSheGetPathOffsetW(L"C:/");
    ok_int(ret, 2);

    ret = s_pSheGetPathOffsetW(L"C:Windows");
    ok_int(ret, -1);
    ret = s_pSheGetPathOffsetW(L"C:\\Windows");
    ok_int(ret, 2);
    ret = s_pSheGetPathOffsetW(L"C:/Windows");
    ok_int(ret, 2);

    ret = s_pSheGetPathOffsetW(L"\\\\SHARED");
    ok_int(ret, -1);

    ret = s_pSheGetPathOffsetW(L"\\\\SHARED\\TEST");
    ok_int(ret, 13);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED/TEST");
    ok_int(ret, 13);

    ret = s_pSheGetPathOffsetW(L"//SHARED\\TEST");
    ok_int(ret, -1);
    ret = s_pSheGetPathOffsetW(L"//SHARED/TEST/");
    ok_int(ret, -1);
    ret = s_pSheGetPathOffsetW(L"//SHARED/TEST/TEST");
    ok_int(ret, -1);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED\\TEST\\");
    ok_int(ret, 13);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED\\TEST/");
    ok_int(ret, 13);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED\\TEST\\TEST");
    ok_int(ret, 13);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED\\TEST/TEST");
    ok_int(ret, 13);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED/TEST/TEST");
    ok_int(ret, 13);
    ret = s_pSheGetPathOffsetW(L"\\\\SHARED/TEST\\TEST");
    ok_int(ret, 13); 
}

START_TEST(SheGetPathOffset)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+\n");
        return;
    }
    if (!IsWindowsVersionOrGreater(5, 2, 0))
    {
        skip("XP\n");
        return;
    }

    HINSTANCE hShell32 = GetModuleHandleW(L"shell32.dll");

    s_pSheGetPathOffsetW = (FN_SheGetPathOffsetW)GetProcAddress(hShell32, MAKEINTRESOURCEA(349));
    if (!s_pSheGetPathOffsetW)
    {
        s_pSheGetPathOffsetW = (FN_SheGetPathOffsetW)GetProcAddress(hShell32, "SheGetPathOffsetW");
        if (!s_pSheGetPathOffsetW)
        {
            skip("SheGetPathOffsetW not found\n");
            return;
        }
    }

    Test_SheGetPathOffsetW();
}
