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

#include <assert.h>

#include "wine/test.h"
#include "winbase.h"

static BOOL (WINAPI * pVerifyVersionInfoA)(LPOSVERSIONINFOEXA, DWORD, DWORDLONG);
static ULONGLONG (WINAPI * pVerSetConditionMask)(ULONGLONG, DWORD, BYTE);

#define KERNEL32_GET_PROC(func)                                     \
    p##func = (void *)GetProcAddress(hKernel32, #func);             \
    if(!p##func) trace("GetProcAddress(hKernel32, '%s') failed\n", #func);

static void init_function_pointers(void)
{
    HMODULE hKernel32;

    pVerifyVersionInfoA = NULL;
    pVerSetConditionMask = NULL;

    hKernel32 = GetModuleHandleA("kernel32.dll");
    assert(hKernel32);
    KERNEL32_GET_PROC(VerifyVersionInfoA);
    KERNEL32_GET_PROC(VerSetConditionMask);
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
    ok(GetLastError() == 0xdeadbeef,
        "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_VerifyVersionInfo(void)
{
    OSVERSIONINFOEX info;
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
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionExA((OSVERSIONINFO *)&info);
    servicepack = info.wServicePackMajor;
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

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
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
        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx((OSVERSIONINFO *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER));
        ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx((OSVERSIONINFO *)&info);
        info.wServicePackMajor--;
        ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
            pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
        ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());
    }

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_LESS));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
    info.wServicePackMajor++;
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_LESS_EQUAL));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
    info.wServicePackMajor--;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    /* test the failure hierarchy for the four version fields */

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
    info.wServicePackMajor++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
    info.dwMinorVersion++;
    SetLastError(0xdeadbeef);
    ret = pVerifyVersionInfoA(&info, VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MINORVERSION, VER_GREATER_EQUAL));
    error = GetLastError();
    ok(!ret, "VerifyVersionInfoA succeeded\n");
    ok(error == ERROR_OLD_WIN_VERSION || broken(error == ERROR_BAD_ARGUMENTS) /* some win2k */,
       "VerifyVersionInfoA should have failed with ERROR_OLD_WIN_VERSION instead of %d\n", error);

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
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

    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
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

    /* test bad dwOSVersionInfoSize */
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&info);
    info.dwOSVersionInfoSize = 0;
    ret = pVerifyVersionInfoA(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
        pVerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL));
    ok(ret || broken(!ret) /* some win2k */, "VerifyVersionInfoA failed with error %d\n", GetLastError());
}

START_TEST(version)
{
    init_function_pointers();

    test_GetVersionEx();
    test_VerifyVersionInfo();
}
