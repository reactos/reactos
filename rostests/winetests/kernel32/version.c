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
#define _WIN32_WINNT 0x0600

#include "wine/test.h"
#include "winbase.h"
#include "winternl.h"

static BOOL (WINAPI * pGetProductInfo)(DWORD, DWORD, DWORD, DWORD, DWORD *);
static BOOL (WINAPI * pVerifyVersionInfoA)(LPOSVERSIONINFOEXA, DWORD, DWORDLONG);
static ULONGLONG (WINAPI * pVerSetConditionMask)(ULONGLONG, DWORD, BYTE);
static NTSTATUS (WINAPI * pRtlGetVersion)(RTL_OSVERSIONINFOEXW *);

#define GET_PROC(func)                                     \
    p##func = (void *)GetProcAddress(hmod, #func);

static void init_function_pointers(void)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("kernel32.dll");

    GET_PROC(GetProductInfo);
    GET_PROC(VerifyVersionInfoA);
    GET_PROC(VerSetConditionMask);

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
            ok(res && (product > PRODUCT_UNDEFINED) && (product <= PRODUCT_PROFESSIONAL_WMC),
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
    ok(ret ||
       broken(ret == 0), /* win95 */
       "Expected GetVersionExA to succeed\n");
}

static void test_VerifyVersionInfo(void)
{
    OSVERSIONINFOEXA info;
    BOOL ret;
    DWORD servicepack, error;

    if(!pVerifyVersionInfoA || !pVerSetConditionMask)
    {
        win_skip("Needed functions not available\n");
        return;
    }

    /* Before we start doing some tests we should check what the version of
     * the ServicePack is. Tests on a box with no ServicePack will fail otherwise.
     */
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    servicepack = info.wServicePackMajor;

    /* Win8.1+ returns Win8 version in GetVersionEx when there's no app manifest targeting 8.1 */
    if (info.dwMajorVersion == 6 && info.dwMinorVersion == 2)
    {
        RTL_OSVERSIONINFOEXW rtlinfo;
        rtlinfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
        ok(SUCCEEDED(pRtlGetVersion(&rtlinfo)), "RtlGetVersion failed\n");

        if (rtlinfo.dwMajorVersion != 6 || rtlinfo.dwMinorVersion != 2)
        {
            win_skip("GetVersionEx and VerifyVersionInfo are faking values\n");
            return;
        }
    }

    memset(&info, 0, sizeof(info));

    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION,
        pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_BUILDNUMBER | VER_MAJORVERSION |
        VER_MINORVERSION/* | VER_PLATFORMID | VER_SERVICEPACKMAJOR |
        VER_SERVICEPACKMINOR | VER_SUITENAME | VER_PRODUCT_TYPE */,
        pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    /* tests special handling of VER_SUITENAME */

    ret = pVerifyVersionInfoA(&info, VER_SUITENAME,
        pVerSetConditionMask(0, VER_SUITENAME, VER_AND));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    ret = pVerifyVersionInfoA(&info, VER_SUITENAME,
        pVerSetConditionMask(0, VER_SUITENAME, VER_OR));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    /* test handling of version numbers */
    
    /* v3.10 is always less than v4.x even
     * if the minor version is tested */
    info.dwMajorVersion = 3;
    info.dwMinorVersion = 10;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwMinorVersion = 0;
    info.wServicePackMajor = 10;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.wServicePackMajor = 0;
    info.wServicePackMinor = 10;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL),
            VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some wink2 */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    if (servicepack == 0)
    {
        skip("There is no ServicePack on this system\n");
    }
    else
    {
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER));
        ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
        ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());
    }

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_LESS));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_LESS_EQUAL));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor--;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    /* test the failure hierarchy for the four version fields */

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMinorVersion++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMajorVersion++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwBuildNumber++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    /* systematically test behaviour of condition mask (tests sorted by condition mask value) */

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMinorVersion++;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_MINORVERSION, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMinorVersion++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_AND));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMinorVersion++;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_LESS_EQUAL), VER_MINORVERSION, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMinorVersion++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_AND), VER_MINORVERSION, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMinorVersion++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_OR), VER_MINORVERSION, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    if (servicepack)
    {
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.dwMajorVersion++;
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_LESS), VER_SERVICEPACKMAJOR, VER_EQUAL));
        ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());
    }

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    ret = pVerifyVersionInfoA(&info, VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_SERVICEPACKMAJOR, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_SERVICEPACKMAJOR, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL),
        VER_SERVICEPACKMAJOR, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL),
        VER_SERVICEPACKMAJOR, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL),
        VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMinor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_EQUAL), VER_SERVICEPACKMINOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    if (servicepack)
    {
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        SetLastError(0xdeadbeef);
        ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_GREATER));
        error = GetLastError();
        ok(!ret, "VerifyVersionInfoA succeeded\n");
        ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
           "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
            VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_GREATER));
        ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
            VER_MINORVERSION, VER_LESS_EQUAL), VER_SERVICEPACKMAJOR, VER_GREATER));
        ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
            VER_MINORVERSION, VER_AND), VER_SERVICEPACKMAJOR, VER_GREATER));
        ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());
    }

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_LESS_EQUAL), VER_SERVICEPACKMAJOR, VER_GREATER));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    if (servicepack)
    {
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
        GetVersionExA((OSVERSIONINFOA *)&info);
        info.wServicePackMajor--;
        SetLastError(0xdeadbeef);
        ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(pVerSetConditionMask(0, VER_SERVICEPACKMAJOR, VER_GREATER), VER_SERVICEPACKMINOR, VER_EQUAL));
        error = GetLastError();
        ok(!ret, "VerifyVersionInfoA succeeded\n");
        ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
           "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);
    }

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL),
        VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwMajorVersion--;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL),
        VER_MINORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_LESS_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_SERVICEPACKMAJOR, VER_AND));
    ok(ret, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    /* test bad dwOSVersionInfoSize */
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    GetVersionExA((OSVERSIONINFOA *)&info);
    info.dwOSVersionInfoSize = 0;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());
}

START_TEST(version)
{
    init_function_pointers();

    test_GetProductInfo();
    test_GetVersionEx();
    test_VerifyVersionInfo();
}
