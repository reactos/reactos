/*
 * Unit test suite for version functions
 *
 * Copyright 2006 Robert Shearman
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Needed for PRODUCT_* defines and GetProductInfo() */
#ifndef __REACTOS__
#define _WIN32_WINNT 0x0600
#endif

#include "wine/test.h"
#include "winbase.h"
#include "winternl.h"

static BOOL (WINAPI * pGetProductInfo)(DWORD, DWORD, DWORD, DWORD, DWORD *);
static NTSTATUS (WINAPI * pRtlGetVersion)(RTL_OSVERSIONINFOEXW *);

#define GET_PROC(func)                                     \
    p##func = (void *)GetProcAddress(hmod, #func);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("kernel32.dll");

    GET_PROC(GetProductInfo);

    hmod = GetModuleHandleA("ntdll.dll");

    GET_PROC(RtlGetVersion);
}

static void test_GetProductInfo(void)
{
    DWORD product;
    DWORD res;
    DWORD table[] = {9,8,7,6,
                     7,0,0,0,
                     6,2,0,0,
                     6,1,2,0,
                     6,1,1,0,
                     6,1,0,2,
                     6,1,0,0,
                     6,0,3,0,
                     6,0,2,0,
                     6,0,1,5,
                     6,0,1,0,
                     6,0,0,0,
                     5,3,0,0,
                     5,2,0,0,
                     5,1,0,0,
                     5,0,0,0,
                     0};

    DWORD *entry = table;

    if (!pGetProductInfo)
    {
        /* Not present before Vista */
        win_skip("GetProductInfo() not available\n");
        return;
    }

    while (*entry)
    {
        /* SetLastError() / GetLastError(): value is untouched */
        product = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        res = pGetProductInfo(entry[0], entry[1], entry[2], entry[3], &product);

        if (entry[0] >= 6)
            ok(res && (product > PRODUCT_UNDEFINED) && (product <= PRODUCT_ENTERPRISE_S_N_EVALUATION),
               "got %d and 0x%x (expected TRUE and a valid PRODUCT_* value)\n", res, product);
        else
            ok(!res && !product && (GetLastError() == 0xdeadbeef),
               "got %d and 0x%x with 0x%x (expected FALSE and PRODUCT_UNDEFINED with LastError untouched)\n",
               res, product, GetLastError());

        entry+= 4;
    }

    /* NULL pointer is not a problem */
    SetLastError(0xdeadbeef);
    res = pGetProductInfo(6, 1, 0, 0, NULL);
    ok( (!res) && (GetLastError() == 0xdeadbeef),
        "got %d with 0x%x (expected FALSE with LastError untouched\n", res, GetLastError());
}

static void test_GetVersionEx(void)
{
    OSVERSIONINFOA infoA;
    OSVERSIONINFOEXA infoExA;
    BOOL ret;

    if (0)
    {
        /* Silently crashes on XP */
        GetVersionExA(NULL);
    }

    SetLastError(0xdeadbeef);
    memset(&infoA,0,sizeof infoA);
    ret = GetVersionExA(&infoA);
    ok(!ret, "Expected GetVersionExA to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == 0xdeadbeef /* Win9x */,
        "Expected ERROR_INSUFFICIENT_BUFFER or 0xdeadbeef (Win9x), got %d\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    infoA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) / 2;
    ret = GetVersionExA(&infoA);
    ok(!ret, "Expected GetVersionExA to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == 0xdeadbeef /* Win9x */,
        "Expected ERROR_INSUFFICIENT_BUFFER or 0xdeadbeef (Win9x), got %d\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    infoA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) * 2;
    ret = GetVersionExA(&infoA);
    ok(!ret, "Expected GetVersionExA to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == 0xdeadbeef /* Win9x */,
        "Expected ERROR_INSUFFICIENT_BUFFER or 0xdeadbeef (Win9x), got %d\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    infoA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    ret = GetVersionExA(&infoA);
    ok(ret, "Expected GetVersionExA to succeed\n");
    ok(GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    infoExA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    ret = GetVersionExA((OSVERSIONINFOA *)&infoExA);
    ok(ret, "GetVersionExA failed.\n");

    if (!infoExA.wServicePackMajor && !infoExA.wServicePackMinor)
        ok(!infoExA.szCSDVersion[0], "got '%s'\n", infoExA.szCSDVersion);
}

static void test_VerifyVersionInfo(void)
{
    enum srcversion_mode
    {
        SRCVERSION_ZERO         = 0,
        SRCVERSION_CURRENT      = 1,
        SRCVERSION_INC_MINOR    = 2,
        SRCVERSION_INC_SP_MINOR = 3,
        SRCVERSION_INC_SP_MAJOR = 4,
        SRCVERSION_DEC_SP_MAJOR = 5,
        SRCVERSION_DEC_MAJOR    = 6,
        SRCVERSION_INC_BUILD    = 7,
        SRCVERSION_REQUIRES_SP  = 0x1000,
    };

    struct verify_version_test
    {
        DWORD verifymask; /* Type mask for VerifyVersionInfo() */
        DWORD srcinfo;    /* The way current version info is modified. */
        DWORD err;        /* Error code on failure, 0 on success. */

        DWORD typemask1;
        DWORD condition1;
        DWORD typemask2;
        DWORD condition2;
        DWORD typemask3;
        DWORD condition3;
        DWORD typemask4;
        DWORD condition4;

        BOOL todo;
    } verify_version_tests[] =
    {
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            0,

            VER_MAJORVERSION, VER_EQUAL,
            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_CURRENT,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_CURRENT,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_AND,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            0,

            VER_MAJORVERSION, VER_LESS_EQUAL,
            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_AND,
            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_OR,
            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_EQUAL,
        },
        {
            VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            0,

            VER_SERVICEPACKMAJOR, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_SERVICEPACKMAJOR, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            0,

            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            0,

            VER_MAJORVERSION, VER_EQUAL,
            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_EQUAL,
            VER_MINORVERSION, VER_GREATER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_EQUAL,
            VER_SERVICEPACKMINOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            0,

            VER_MAJORVERSION, VER_LESS_EQUAL,
            VER_SERVICEPACKMAJOR, VER_GREATER,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            0,

            VER_MAJORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            0,

            VER_MAJORVERSION, VER_EQUAL,
            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_MAJOR,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_CURRENT,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_GREATER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_LESS_EQUAL,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_AND,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION,
            SRCVERSION_ZERO,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
            SRCVERSION_ZERO,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_SUITENAME,
            SRCVERSION_ZERO,
            0,

            VER_SUITENAME, VER_AND,
        },
        {
            VER_SUITENAME,
            SRCVERSION_ZERO,
            0,

            VER_SUITENAME, VER_OR,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            0,

            VER_MINORVERSION, VER_LESS,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            0,

            VER_MINORVERSION, VER_LESS_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_SP_MAJOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_MINOR,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_CURRENT,
            0,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_BUILD,
            ERROR_OLD_WIN_VERSION,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_INC_BUILD,
            0,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            0,

            VER_MINORVERSION, VER_GREATER,
        },
        {
            VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            0,

            VER_MINORVERSION, VER_GREATER_EQUAL,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            ERROR_OLD_WIN_VERSION,

            VER_MAJORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_GREATER,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_EQUAL,
            VER_SERVICEPACKMAJOR, VER_GREATER,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_LESS_EQUAL,
            VER_SERVICEPACKMAJOR, VER_GREATER,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            0,

            VER_MAJORVERSION, VER_GREATER_EQUAL,
            VER_MINORVERSION, VER_AND,
            VER_SERVICEPACKMAJOR, VER_GREATER,
        },
        {
            VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            SRCVERSION_DEC_SP_MAJOR | SRCVERSION_REQUIRES_SP,
            ERROR_OLD_WIN_VERSION,

            VER_SERVICEPACKMAJOR, VER_GREATER,
            VER_SERVICEPACKMINOR, VER_EQUAL,
        },
    };

    OSVERSIONINFOEXA info;
    DWORD servicepack;
    unsigned int i;
    BOOL ret;

    /* Before we start doing some tests we should check what the version of
     * the ServicePack is. Tests on a box with no ServicePack will fail otherwise.
     */
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    servicepack = info.wServicePackMajor;
    if (servicepack == 0)
        skip("There is no ServicePack on this system. Some tests will be skipped.\n");

    /* Win8.1+ returns Win8 version in GetVersionEx when there's no app manifest targeting 8.1 */
    if (info.dwMajorVersion == 6 && info.dwMinorVersion == 2)
    {
        RTL_OSVERSIONINFOEXW rtlinfo;
        rtlinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
        ok(!pRtlGetVersion(&rtlinfo), "RtlGetVersion failed\n");

        if (rtlinfo.dwMajorVersion != 6 || rtlinfo.dwMinorVersion != 2)
        {
            win_skip("GetVersionEx and VerifyVersionInfo are faking values\n");
            return;
        }
    }

    for (i = 0; i < sizeof(verify_version_tests)/sizeof(verify_version_tests[0]); i++)
    {
        struct verify_version_test *test = &verify_version_tests[i];
        DWORD srcinfo = test->srcinfo;
        ULONGLONG mask;

        if (servicepack == 0 && srcinfo & SRCVERSION_REQUIRES_SP)
            continue;
        srcinfo &= ~SRCVERSION_REQUIRES_SP;

        info.dwOSVersionInfoSize = sizeof(info);
        GetVersionExA((OSVERSIONINFOA *)&info);

        switch (srcinfo)
        {
        case SRCVERSION_ZERO:
            memset(&info, 0, sizeof(info));
            break;
        case SRCVERSION_INC_MINOR:
            info.dwMinorVersion++;
            break;
        case SRCVERSION_INC_SP_MINOR:
            info.wServicePackMinor++;
            break;
        case SRCVERSION_INC_SP_MAJOR:
            info.wServicePackMajor++;
            break;
        case SRCVERSION_DEC_SP_MAJOR:
            info.wServicePackMajor--;
            break;
        case SRCVERSION_DEC_MAJOR:
            info.dwMajorVersion--;
            break;
        case SRCVERSION_INC_BUILD:
            info.dwBuildNumber++;
            break;
        default:
            ;
        }

        mask = VerSetConditionMask(0, test->typemask1, test->condition1);
        if (test->typemask2)
            mask = VerSetConditionMask(mask, test->typemask2, test->condition2);
        if (test->typemask3)
            mask = VerSetConditionMask(mask, test->typemask3, test->condition3);
        if (test->typemask4)
            mask = VerSetConditionMask(mask, test->typemask4, test->condition4);

        SetLastError(0xdeadbeef);
        ret = VerifyVersionInfoA(&info, test->verifymask, mask);
    todo_wine_if(test->todo)
    {
        ok(test->err ? !ret : ret, "%u: unexpected return value %d.\n", i, ret);
        if (!ret)
            ok(GetLastError() == test->err, "%u: unexpected error code %d, expected %d.\n", i, GetLastError(), test->err);
    }
    }

    /* test handling of version numbers */
    /* v3.10 is always less than v4.x even
     * if the minor version is tested */
    info.dwMajorVersion = 3;
    info.dwMinorVersion = 10;
    ret = VerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwMinorVersion = 0;
    info.wServicePackMajor = 10;
    ret = VerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.wServicePackMajor = 0;
    info.wServicePackMinor = 10;
    ret = VerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    /* test bad dwOSVersionInfoSize */
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwOSVersionInfoSize = 0;
    ret = VerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());
}

START_TEST(version)
{
    init_function_pointers();

    test_GetProductInfo();
    test_GetVersionEx();
    test_VerifyVersionInfo();
}
