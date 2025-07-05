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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include "winbase.h"
#include "winternl.h"
#include "appmodel.h"

static BOOL (WINAPI * pGetProductInfo)(DWORD, DWORD, DWORD, DWORD, DWORD *);
static UINT (WINAPI * pEnumSystemFirmwareTables)(DWORD, void *, DWORD);
static UINT (WINAPI * pGetSystemFirmwareTable)(DWORD, DWORD, void *, DWORD);
static LONG (WINAPI * pPackageIdFromFullName)(const WCHAR *, UINT32, UINT32 *, BYTE *);
static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, void *, ULONG, ULONG *);
static NTSTATUS (WINAPI * pRtlGetVersion)(RTL_OSVERSIONINFOEXW *);

#define GET_PROC(func)                                     \
    p##func = (void *)GetProcAddress(hmod, #func);

/* Firmware table providers */
#define ACPI 0x41435049
#define FIRM 0x4649524D
#define RSMB 0x52534D42

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("kernel32.dll");

    GET_PROC(GetProductInfo);
    GET_PROC(EnumSystemFirmwareTables);
    GET_PROC(GetSystemFirmwareTable);
    GET_PROC(PackageIdFromFullName);

    hmod = GetModuleHandleA("ntdll.dll");

    GET_PROC(NtQuerySystemInformation);
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
               "got %ld and 0x%lx (expected TRUE and a valid PRODUCT_* value)\n", res, product);
        else
            ok(!res && !product && (GetLastError() == 0xdeadbeef),
               "got %ld and 0x%lx with 0x%lx (expected FALSE and PRODUCT_UNDEFINED with LastError untouched)\n",
               res, product, GetLastError());

        entry+= 4;
    }

    /* NULL pointer is not a problem */
    SetLastError(0xdeadbeef);
    res = pGetProductInfo(6, 1, 0, 0, NULL);
    ok( (!res) && (GetLastError() == 0xdeadbeef),
        "got %ld with 0x%lx (expected FALSE with LastError untouched\n", res, GetLastError());
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
        "Expected ERROR_INSUFFICIENT_BUFFER or 0xdeadbeef (Win9x), got %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    infoA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) / 2;
    ret = GetVersionExA(&infoA);
    ok(!ret, "Expected GetVersionExA to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == 0xdeadbeef /* Win9x */,
        "Expected ERROR_INSUFFICIENT_BUFFER or 0xdeadbeef (Win9x), got %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    infoA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA) * 2;
    ret = GetVersionExA(&infoA);
    ok(!ret, "Expected GetVersionExA to fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
        GetLastError() == 0xdeadbeef /* Win9x */,
        "Expected ERROR_INSUFFICIENT_BUFFER or 0xdeadbeef (Win9x), got %ld\n",
        GetLastError());

    SetLastError(0xdeadbeef);
    infoA.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    ret = GetVersionExA(&infoA);
    ok(ret, "Expected GetVersionExA to succeed\n");
    ok(GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got %ld\n", GetLastError());

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
            skip("GetVersionEx and VerifyVersionInfo are faking values\n");
            return;
        }
    }

    for (i = 0; i < ARRAY_SIZE(verify_version_tests); i++)
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
        ok(test->err ? !ret : ret, "%u: unexpected return value %d.\n", i, ret);
        if (!ret)
            ok(GetLastError() == test->err, "%u: unexpected error code %ld, expected %ld.\n", i, GetLastError(), test->err);
    }

    /* test handling of version numbers */
    /* v3.10 is always less than v4.x even
     * if the minor version is tested */
    info.dwMajorVersion = 3;
    info.dwMinorVersion = 10;
    ret = VerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %ld\n", GetLastError());

    info.dwMinorVersion = 0;
    info.wServicePackMajor = 10;
    ret = VerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %ld\n", GetLastError());

    info.wServicePackMajor = 0;
    info.wServicePackMinor = 10;
    ret = VerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(VerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %ld\n", GetLastError());

    /* test bad dwOSVersionInfoSize */
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwOSVersionInfoSize = 0;
    ret = VerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %ld\n", GetLastError());
}

static void test_SystemFirmwareTable(void)
{
    static const ULONG min_sfti_len = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);
    ULONG expected_len;
    UINT len;
    NTSTATUS status;
    SYSTEM_FIRMWARE_TABLE_INFORMATION *sfti;
    UCHAR *smbios_table;

    if (!pGetSystemFirmwareTable || !pEnumSystemFirmwareTables)
    {
        win_skip("SystemFirmwareTable functions not available\n");
        return;
    }

    sfti = HeapAlloc(GetProcessHeap(), 0, sizeof(*sfti));
    ok(!!sfti, "Failed to allocate memory\n");
    sfti->ProviderSignature = RSMB;
    sfti->Action = SystemFirmwareTable_Get;
    sfti->TableID = 0;
    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, min_sfti_len, &expected_len);
    if (expected_len == 0) /* xp, 2003 */
    {
        win_skip("SystemFirmwareTableInformation is not available\n");
        HeapFree(GetProcessHeap(), 0, sfti);
        return;
    }
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySystemInformation failed %lx\n", status );
    sfti = HeapReAlloc(GetProcessHeap(), 0, sfti, expected_len);
    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, expected_len, &expected_len);
    ok( !status, "NtQuerySystemInformation failed %lx\n", status );

    expected_len -= min_sfti_len;
    ok( sfti->TableBufferLength == expected_len, "wrong len %lu/%lx\n",
        sfti->TableBufferLength, expected_len );
    len = pGetSystemFirmwareTable(RSMB, 0, NULL, 0);
    ok(len == expected_len, "Expected length %lu, got %u\n", expected_len, len);

    smbios_table = HeapAlloc(GetProcessHeap(), 0, expected_len);
    len = pGetSystemFirmwareTable(RSMB, 0, smbios_table, expected_len);
    ok(len == expected_len, "Expected length %lu, got %u\n", expected_len, len);
    ok(len == 0 || !memcmp(smbios_table, sfti->TableBuffer, 6),
       "Expected prologue %02x %02x %02x %02x %02x %02x, got %02x %02x %02x %02x %02x %02x\n",
       sfti->TableBuffer[0], sfti->TableBuffer[1], sfti->TableBuffer[2],
       sfti->TableBuffer[3], sfti->TableBuffer[4], sfti->TableBuffer[5],
       smbios_table[0], smbios_table[1], smbios_table[2],
       smbios_table[3], smbios_table[4], smbios_table[5]);
    HeapFree(GetProcessHeap(), 0, smbios_table);

    sfti->Action = SystemFirmwareTable_Enumerate;
    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, min_sfti_len, &expected_len);
    ok( status == STATUS_BUFFER_TOO_SMALL, "NtQuerySystemInformation failed %lx\n", status );
    sfti = HeapReAlloc(GetProcessHeap(), 0, sfti, expected_len);
    status = pNtQuerySystemInformation(SystemFirmwareTableInformation, sfti, expected_len, &expected_len);
    ok( !status, "NtQuerySystemInformation failed %lx\n", status );
    ok( expected_len == min_sfti_len + sizeof(UINT), "wrong len %lu\n", expected_len );
    ok( sfti->TableBufferLength == sizeof(UINT), "wrong len %lu\n", sfti->TableBufferLength );
    ok( *(UINT *)sfti->TableBuffer == 0, "wrong table id %x\n", *(UINT *)sfti->TableBuffer );

    len = pEnumSystemFirmwareTables( RSMB, NULL, 0 );
    ok( len == sizeof(UINT), "wrong len %u\n", len );
    smbios_table = malloc( len );
    len = pEnumSystemFirmwareTables( RSMB, smbios_table, len );
    ok( len == sizeof(UINT), "wrong len %u\n", len );
    ok( *(UINT *)smbios_table == 0, "wrong table id %x\n", *(UINT *)smbios_table );
    free( smbios_table );

    HeapFree(GetProcessHeap(), 0, sfti);
}

static const struct
{
    UINT32 code;
    const WCHAR *name;
    BOOL broken;
}
arch_data[] =
{
    {PROCESSOR_ARCHITECTURE_INTEL,   L"X86"},
    {PROCESSOR_ARCHITECTURE_ARM,     L"Arm"},
    {PROCESSOR_ARCHITECTURE_AMD64,   L"X64"},
    {PROCESSOR_ARCHITECTURE_NEUTRAL, L"Neutral"},
    {PROCESSOR_ARCHITECTURE_ARM64,   L"Arm64",   TRUE /* Before Win10. */},
    {PROCESSOR_ARCHITECTURE_UNKNOWN, L"Unknown", TRUE /* Before Win10 1709. */},
};

static const WCHAR *arch_string_from_code(UINT32 arch)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(arch_data); ++i)
        if (arch_data[i].code == arch)
            return arch_data[i].name;

    return NULL;
}

static unsigned int get_package_str_size(const WCHAR *str)
{
    return str ? (lstrlenW(str) + 1) * sizeof(*str) : 0;
}

static unsigned int get_package_id_size(const PACKAGE_ID *id)
{
    return sizeof(*id) + get_package_str_size(id->name)
            + get_package_str_size(id->resourceId) + 14 * sizeof(WCHAR);
}

static void packagefullname_from_packageid(WCHAR *buffer, size_t count, const PACKAGE_ID *id)
{
#if defined(__REACTOS__) && defined(_MSC_VER)
    _snwprintf(buffer, count, L"%s_%u.%u.%u.%u_%s_%s_%s", id->name, id->version.Major,
            id->version.Minor, id->version.Build, id->version.Revision,
            arch_string_from_code(id->processorArchitecture), id->resourceId,
            id->publisherId);
#else
    swprintf(buffer, count, L"%s_%u.%u.%u.%u_%s_%s_%s", id->name, id->version.Major,
            id->version.Minor, id->version.Build, id->version.Revision,
            arch_string_from_code(id->processorArchitecture), id->resourceId,
            id->publisherId);
#endif
}

static void test_PackageIdFromFullName(void)
{
    static const PACKAGE_ID test_package_id =
    {
        0, PROCESSOR_ARCHITECTURE_INTEL,
                {{.Major = 1, .Minor = 2, .Build = 3, .Revision = 4}},
                (WCHAR *)L"TestPackage", NULL,
                (WCHAR *)L"TestResourceId", (WCHAR *)L"0abcdefghjkme"
    };
    UINT32 size, expected_size;
    PACKAGE_ID test_id;
    WCHAR fullname[512];
    BYTE id_buffer[512];
    unsigned int i;
    PACKAGE_ID *id;
    LONG ret;

    if (!pPackageIdFromFullName)
    {
        win_skip("PackageIdFromFullName not available.\n");
        return;
    }

    packagefullname_from_packageid(fullname, ARRAY_SIZE(fullname), &test_package_id);

    id = (PACKAGE_ID *)id_buffer;

    memset(id_buffer, 0xcc, sizeof(id_buffer));
    expected_size = get_package_id_size(&test_package_id);
    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(fullname, 0, &size, id_buffer);
    ok(ret == ERROR_SUCCESS, "Got unexpected ret %lu.\n", ret);
    ok(size == expected_size, "Got unexpected length %u, expected %u.\n", size, expected_size);
    ok(!lstrcmpW(id->name, test_package_id.name), "Got unexpected name %s.\n", debugstr_w(id->name));
    ok(!lstrcmpW(id->resourceId, test_package_id.resourceId), "Got unexpected resourceId %s.\n",
            debugstr_w(id->resourceId));
    ok(!lstrcmpW(id->publisherId, test_package_id.publisherId), "Got unexpected publisherId %s.\n",
            debugstr_w(id->publisherId));
    ok(!id->publisher, "Got unexpected publisher %s.\n", debugstr_w(id->publisher));
    ok(id->processorArchitecture == PROCESSOR_ARCHITECTURE_INTEL, "Got unexpected processorArchitecture %u.\n",
            id->processorArchitecture);
    ok(id->version.Version == 0x0001000200030004, "Got unexpected Version %s.\n",
            wine_dbgstr_longlong(id->version.Version));
    ok((BYTE *)id->name == id_buffer + sizeof(*id), "Got unexpected name %p, buffer %p.\n", id->name, id_buffer);
    ok((BYTE *)id->resourceId == (BYTE *)id->name + (lstrlenW(id->name) + 1) * 2,
            "Got unexpected resourceId %p, buffer %p.\n", id->resourceId, id_buffer);
    ok((BYTE *)id->publisherId == (BYTE *)id->resourceId + (lstrlenW(id->resourceId) + 1) * 2,
            "Got unexpected publisherId %p, buffer %p.\n", id->resourceId, id_buffer);

    ret = pPackageIdFromFullName(fullname, 0, NULL, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %ld.\n", ret);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(NULL, 0, &size, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %ld.\n", ret);
    ok(size == sizeof(id_buffer), "Got unexpected size %u.\n", size);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(fullname, 0, &size, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %ld.\n", ret);
    ok(size == sizeof(id_buffer), "Got unexpected size %u.\n", size);

    size = expected_size - 1;
    ret = pPackageIdFromFullName(fullname, 0, &size, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %ld.\n", ret);
    ok(size == expected_size - 1, "Got unexpected size %u.\n", size);

    size = expected_size - 1;
    ret = pPackageIdFromFullName(fullname, 0, &size, id_buffer);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "Got unexpected ret %ld.\n", ret);
    ok(size == expected_size, "Got unexpected size %u.\n", size);

    size = 0;
    ret = pPackageIdFromFullName(fullname, 0, &size, NULL);
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "Got unexpected ret %ld.\n", ret);
    ok(size == expected_size, "Got unexpected size %u.\n", size);

    for (i = 0; i < ARRAY_SIZE(arch_data); ++i)
    {
        test_id = test_package_id;
        test_id.processorArchitecture = arch_data[i].code;
        packagefullname_from_packageid(fullname, ARRAY_SIZE(fullname), &test_id);
        size = expected_size;
        ret = pPackageIdFromFullName(fullname, 0, &size, id_buffer);
        ok(ret == ERROR_SUCCESS || broken(arch_data[i].broken && ret == ERROR_INVALID_PARAMETER),
                "Got unexpected ret %lu.\n", ret);
        if (ret != ERROR_SUCCESS)
            continue;
        ok(size == expected_size, "Got unexpected length %u, expected %u.\n", size, expected_size);
        ok(id->processorArchitecture == arch_data[i].code, "Got unexpected processorArchitecture %u, arch %S.\n",
                id->processorArchitecture, arch_data[i].name);
    }

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3.4_X86_TestResourceId_0abcdefghjkme", 0, &size, id_buffer);
    ok(ret == ERROR_SUCCESS, "Got unexpected ret %lu.\n", ret);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3.4_X86_TestResourceId_abcdefghjkme", 0, &size, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3.4_X86_TestResourceId_0abcdefghjkmee", 0, &size, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3_X86_TestResourceId_0abcdefghjkme", 0, &size, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3.4_X86_TestResourceId_0abcdefghjkme_", 0, &size, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3.4_X86__0abcdefghjkme", 0, &size, id_buffer);
    ok(ret == ERROR_SUCCESS, "Got unexpected ret %lu.\n", ret);
    ok(!lstrcmpW(id->resourceId, L""), "Got unexpected resourceId %s.\n", debugstr_w(id->resourceId));

    size = sizeof(id_buffer);
    ret = pPackageIdFromFullName(L"TestPackage_1.2.3.4_X86_0abcdefghjkme", 0, &size, id_buffer);
    ok(ret == ERROR_INVALID_PARAMETER, "Got unexpected ret %lu.\n", ret);
}

#define TEST_VERSION_WIN7   1
#define TEST_VERSION_WIN8   2
#define TEST_VERSION_WIN8_1 4
#define TEST_VERSION_WIN10  8

static const struct
{
    unsigned int pe_version_major, pe_version_minor;
    unsigned int manifest_versions;
    unsigned int expected_major, expected_minor;
}
test_pe_os_version_tests[] =
{
    { 4, 0,                         0,  6, 2},
    { 4, 0,        TEST_VERSION_WIN10, 10, 0},
    { 6, 3,         TEST_VERSION_WIN8,  6, 2},
    {10, 0,                         0, 10, 0},
    { 6, 3,                         0,  6, 3},
    { 6, 4,                         0,  6, 3},
    { 9, 0,                         0,  6, 3},
    {11, 0,                         0, 10, 0},
    {10, 0,
            TEST_VERSION_WIN7 | TEST_VERSION_WIN8 | TEST_VERSION_WIN8_1,
                                        6, 3},
};

static void test_pe_os_version_child(unsigned int test)
{
    OSVERSIONINFOEXA info;
    BOOL ret;

    info.dwOSVersionInfoSize = sizeof(info);
    ret = GetVersionExA((OSVERSIONINFOA *)&info);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ok(info.dwMajorVersion == test_pe_os_version_tests[test].expected_major,
            "Test %u, expected major version %u, got %lu.\n", test, test_pe_os_version_tests[test].expected_major,
            info.dwMajorVersion);
    ok(info.dwMinorVersion == test_pe_os_version_tests[test].expected_minor,
            "Test %u, expected minor version %u, got %lu.\n", test, test_pe_os_version_tests[test].expected_minor,
            info.dwMinorVersion);
}

static void test_pe_os_version(void)
{
    static const char manifest_header[] =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<assembly manifestVersion=\"1.0\" xmlns=\"urn:schemas-microsoft-com:asm.v1\""
                    " xmlns:asmv3=\"urn:schemas-microsoft-com:asm.v3\">\n"
                "\t<compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">\n"
                    "\t\t<application>\n";
    static const char manifest_footer[] =
                    "\t\t</application>\n"
                "\t</compatibility>\n"
            "</assembly>\n";
    static const char *version_guids[] =
    {
        "{35138b9a-5d96-4fbd-8e2d-a2440225f93a}",
        "{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}",
        "{1f676c76-80e1-4239-95bb-83d0f6d0da78}",
        "{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}",
    };
    LONG hdr_offset, offset_major, offset_minor;
    char str[MAX_PATH], tmp_exe_name[9];
    RTL_OSVERSIONINFOEXW rtlinfo;
    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi;
    DWORD result, code;
    unsigned int i, j;
    HANDLE file;
    char **argv;
    DWORD size;
    BOOL ret;

    winetest_get_mainargs( &argv );

    if (!pRtlGetVersion)
    {
        win_skip("RtlGetVersion is not supported, skipping tests.\n");
        return;
    }

    rtlinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
    ok(!pRtlGetVersion(&rtlinfo), "RtlGetVersion failed.\n");
    if (rtlinfo.dwMajorVersion < 10)
    {
        skip("Too old Windows version %lu.%lu, skipping tests.\n", rtlinfo.dwMajorVersion, rtlinfo.dwMinorVersion);
        return;
    }

    file = CreateFileA(argv[0], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed, GetLastError() %lu.\n", GetLastError());
    SetFilePointer(file, 0x3c, NULL, FILE_BEGIN);
    ReadFile(file, &hdr_offset, sizeof(hdr_offset), &size, NULL);
    CloseHandle(file);

    offset_major = hdr_offset + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader)
            + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, MajorOperatingSystemVersion);
    offset_minor = hdr_offset + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader)
            + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, MinorOperatingSystemVersion);

    si.cb = sizeof(si);

    for (i = 0; i < ARRAY_SIZE(test_pe_os_version_tests); ++i)
    {
        sprintf(tmp_exe_name, "tmp%u.exe", i);
        ret = CopyFileA(argv[0], tmp_exe_name, FALSE);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

        file = CreateFileA(tmp_exe_name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFile failed, GetLastError() %lu.\n", GetLastError());

        SetFilePointer(file, offset_major, NULL, FILE_BEGIN);
        WriteFile(file, &test_pe_os_version_tests[i].pe_version_major,
                sizeof(test_pe_os_version_tests[i].pe_version_major), &size, NULL);
        SetFilePointer(file, offset_minor, NULL, FILE_BEGIN);
        WriteFile(file, &test_pe_os_version_tests[i].pe_version_minor,
                sizeof(test_pe_os_version_tests[i].pe_version_minor), &size, NULL);

        CloseHandle(file);

        sprintf(str, "%s.manifest", tmp_exe_name);
        file = CreateFileA(str, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "CreateFile failed, GetLastError() %lu.\n", GetLastError());

        WriteFile(file, manifest_header, strlen(manifest_header), &size, NULL);
        for (j = 0; j < ARRAY_SIZE(version_guids); ++j)
        {
            if (test_pe_os_version_tests[i].manifest_versions & (1 << j))
            {
                sprintf(str, "\t\t\t<supportedOS Id=\"%s\"/>\n", version_guids[j]);
                WriteFile(file, str, strlen(str), &size, NULL);
            }
        }
        WriteFile(file, manifest_footer, strlen(manifest_footer), &size, NULL);

        CloseHandle(file);

        sprintf(str, "%s version pe_os_version %u", tmp_exe_name, i);

        ret = CreateProcessA(NULL, str, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        CloseHandle(pi.hThread);
        result = WaitForSingleObject(pi.hProcess, 10000);
        ok(result == WAIT_OBJECT_0, "Got unexpected result %#lx.\n", result);

        ret = GetExitCodeProcess(pi.hProcess, &code);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        ok(!code, "Test %u failed.\n", i);

        CloseHandle(pi.hProcess);

        DeleteFileA(tmp_exe_name);
        sprintf(str, "%s.manifest", tmp_exe_name);
        DeleteFileA(str);
    }
}

START_TEST(version)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs( &argv );

    init_function_pointers();

    if (argc >= 4)
    {
        if (!strcmp(argv[2], "pe_os_version"))
        {
            unsigned int test;

            test = atoi(argv[3]);
            test_pe_os_version_child(test);
        }
        return;
    }

    test_GetProductInfo();
    test_GetVersionEx();
    test_VerifyVersionInfo();
    test_pe_os_version();
    test_SystemFirmwareTable();
    test_PackageIdFromFullName();
}
