/*
 * tests for Microsoft Installer functionality
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#include <stdio.h>
#include <windows.h>
#include <msi.h>
#include <msiquery.h>

#include "wine/test.h"

typedef struct test_MSIFILEHASHINFO {
    ULONG dwFileHashInfoSize;
    ULONG dwData[4];
} test_MSIFILEHASHINFO, *test_PMSIFILEHASHINFO;

typedef INSTALLSTATE (WINAPI *fnMsiUseFeatureExA)(LPCSTR, LPCSTR ,DWORD, DWORD );
fnMsiUseFeatureExA pMsiUseFeatureExA;
typedef UINT (WINAPI *fnMsiOpenPackageExA)(LPCSTR, DWORD, MSIHANDLE*);
fnMsiOpenPackageExA pMsiOpenPackageExA;
typedef UINT (WINAPI *fnMsiOpenPackageExW)(LPCWSTR, DWORD, MSIHANDLE*);
fnMsiOpenPackageExW pMsiOpenPackageExW;
typedef INSTALLSTATE (WINAPI *fnMsiGetComponentPathA)(LPCSTR, LPCSTR, LPSTR, DWORD*);
fnMsiGetComponentPathA pMsiGetComponentPathA;
typedef UINT (WINAPI *fnMsiGetFileHashA)(LPCSTR, DWORD, test_PMSIFILEHASHINFO);
fnMsiGetFileHashA pMsiGetFileHashA;

static void test_usefeature(void)
{
    UINT r;

    if (!pMsiUseFeatureExA)
        return;

    r = MsiQueryFeatureState(NULL,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = MsiQueryFeatureState("{9085040-6000-11d3-8cfe-0150048383c9}" ,NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL,NULL,0,0);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA(NULL, "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}",
                         NULL, -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{9085040-6000-11d3-8cfe-0150048383c9}",
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{0085040-6000-11d3-8cfe-0150048383c9}",
                         "WORDVIEWFiles", -2, 0 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");

    r = pMsiUseFeatureExA("{90850409-6000-11d3-8cfe-0150048383c9}",
                         "WORDVIEWFiles", -2, 1 );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return val\n");
}

static void test_null(void)
{
    MSIHANDLE hpkg;
    UINT r;

    r = pMsiOpenPackageExW(NULL, 0, &hpkg);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiQueryProductStateW(NULL);
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return\n");

    r = MsiEnumFeaturesW(NULL,0,NULL,NULL);
    ok( r == ERROR_INVALID_PARAMETER,"wrong error\n");

    r = MsiConfigureFeatureW(NULL, NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", NULL, 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error\n");

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", "foo", 0);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = MsiConfigureFeatureA("{00000000-0000-0000-0000-000000000000}", "foo", INSTALLSTATE_DEFAULT);
    ok( r == ERROR_UNKNOWN_PRODUCT, "wrong error %d\n", r);
}

static void test_getcomponentpath(void)
{
    INSTALLSTATE r;
    char buffer[0x100];
    DWORD sz;

    if(!pMsiGetComponentPathA)
        return;

    r = pMsiGetComponentPathA( NULL, NULL, NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "bogus", "bogus", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", NULL, NULL );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    sz = sizeof buffer;
    buffer[0]=0;
    r = pMsiGetComponentPathA( "bogus", "{00000000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-000000000000}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{00000000-0000-0000-0000-00000000}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000409-78E1-11D2-B60F-006097C998E7}",
        "{029E403D-A86A-1D11-5B5B0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_INVALIDARG, "wrong return value\n");

    r = pMsiGetComponentPathA( "{00000000-78E1-11D2-B60F-006097C9987e}",
                            "{00000000-A68A-11d1-5B5B-0006799C897E}", buffer, &sz );
    ok( r == INSTALLSTATE_UNKNOWN, "wrong return value\n");
}

static void test_filehash(void)
{
    const char name[] = "msitest.bin";
    const char data[] = {'a','b','c'};
    HANDLE handle;
    UINT r;
    test_MSIFILEHASHINFO hash;
    DWORD count = 0;

    if (!pMsiGetFileHashA)
        return;

    DeleteFile(name);

    memset(&hash, 0, sizeof hash);
    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = pMsiGetFileHashA(name, 0, NULL);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    memset(&hash, 0, sizeof hash);
    hash.dwFileHashInfoSize = sizeof hash;
    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_FILE_NOT_FOUND, "wrong error %d\n", r);

    handle = CreateFile(name, GENERIC_READ|GENERIC_WRITE, 0, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
    ok(handle != INVALID_HANDLE_VALUE, "failed to create file\n");

    WriteFile(handle, data, sizeof data, &count, NULL);
    CloseHandle(handle);

    memset(&hash, 0, sizeof hash);
    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    memset(&hash, 0, sizeof hash);
    hash.dwFileHashInfoSize = sizeof hash;
    r = pMsiGetFileHashA(name, 1, &hash);
    ok( r == ERROR_INVALID_PARAMETER, "wrong error %d\n", r);

    r = pMsiGetFileHashA(name, 0, &hash);
    ok( r == ERROR_SUCCESS, "wrong error %d\n", r);

    ok(hash.dwFileHashInfoSize == sizeof hash, "hash size changed\n");
    ok(hash.dwData[0] == 0x98500190 &&
       hash.dwData[1] == 0xb04fd23c &&
       hash.dwData[2] == 0x7d3f96d6 &&
       hash.dwData[3] == 0x727fe128, "hash of abc incorrect\n");

    DeleteFile(name);
}

START_TEST(msi)
{
    HMODULE hmod = GetModuleHandle("msi.dll");
    pMsiUseFeatureExA = (fnMsiUseFeatureExA)
        GetProcAddress(hmod, "MsiUseFeatureExA");
    pMsiOpenPackageExA = (fnMsiOpenPackageExA)
        GetProcAddress(hmod, "MsiOpenPackageExA");
    pMsiOpenPackageExW = (fnMsiOpenPackageExW)
        GetProcAddress(hmod, "MsiOpenPackageExW");
    pMsiGetComponentPathA = (fnMsiGetComponentPathA)
        GetProcAddress(hmod, "MsiGetComponentPathA" );
    pMsiGetFileHashA = (fnMsiGetFileHashA)
        GetProcAddress(hmod, "MsiGetFileHashA" );

    test_usefeature();
    test_null();
    test_getcomponentpath();
    test_filehash();
}
