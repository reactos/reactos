/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include <windows.h>
#include <winsxs.h>

#include "wine/test.h"

static void test_QueryAssemblyInfo( void )
{
    static const WCHAR wineW[] =
        L"wine,version=\"1.2.3.4\",type=\"win32\",processorArchitecture=\"x86\",publicKeyToken=\"1234567890abcdef\"";
    HRESULT hr;
    ASSEMBLY_INFO info;
    IAssemblyCache *cache = NULL;
    WCHAR path[MAX_PATH];
    char comctl_path1[MAX_PATH], comctl_path2[MAX_PATH];
    const WCHAR *comctlW;

    hr = CreateAssemblyCache( &cache, 0 );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( cache != NULL, "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, L"", NULL );
    ok( hr == E_INVALIDARG, "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, L"wine", NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE ), "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, L"wine,version=\"1.2.3.4\"", NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE ), "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, L"wine,version=\"1.2.3.4\",type=\"win32\"", NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE ), "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, L"wine,version=\"1.2.3.4\",type=\"win32\",processorArchitecture=\"x86\"",
                                           NULL );
    todo_wine ok( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), "got %08lx\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wineW, NULL );
    todo_wine ok( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), "got %08lx\n", hr );

    GetWindowsDirectoryA( comctl_path1, MAX_PATH );
    lstrcatA( comctl_path1, "\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef" );
    GetWindowsDirectoryA( comctl_path2, MAX_PATH );
    lstrcatA( comctl_path2, "\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.19041.3636_none_a863d714867441db" );
    if (GetFileAttributesA( comctl_path1 ) != INVALID_FILE_ATTRIBUTES)
        comctlW = L"microsoft.windows.common-controls,version=\"6.0.2600.2982\",type=\"win32\",processorArchitecture=\"x86\","
                   "publicKeyToken=\"6595b64144ccf1df\"";
    else if (GetFileAttributesA( comctl_path2 ) != INVALID_FILE_ATTRIBUTES)
        comctlW = L"microsoft.windows.common-controls,version=\"6.0.19041.3636\",type=\"win32\",processorArchitecture=\"x86\","
                   "publicKeyToken=\"6595b64144ccf1df\"";
    else
    {
        skip( "no assembly to test with\n" );
        IAssemblyCache_Release( cache );
        return;
    }

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, NULL );
    ok( hr == S_OK, "got %08lx\n", hr );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wineW, &info );
    todo_wine ok( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), "got %08lx\n", hr );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, &info );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( info.dwAssemblyFlags == 1, "got %08lx\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %lu\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.pszCurrentAssemblyPathBuf == NULL, "got %p\n", info.pszCurrentAssemblyPathBuf );
    ok( !info.cchBuf, "got %lu\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    info.pszCurrentAssemblyPathBuf = path;
    info.cchBuf = ARRAY_SIZE( path );
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, &info );
    ok( hr == S_OK, "got %08lx\n", hr );
    ok( info.dwAssemblyFlags == 1, "got %08lx\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %lu\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.cchBuf == ARRAY_SIZE( path ), "got %lu\n", info.cchBuf );
    ok( path[0], "empty path\n" );
    lstrcatW( path, L"comctl32.dll" );
    ok( GetFileAttributesW( path ) != INVALID_FILE_ATTRIBUTES, "%s should exist\n", wine_dbgstr_w( path ));

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    info.pszCurrentAssemblyPathBuf = path;
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, &info );
    ok( hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ), "got %08lx\n", hr );
    ok( info.dwAssemblyFlags == 1, "got %08lx\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %lu\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.cchBuf, "got %lu\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 1, comctlW, &info );
    ok( hr == E_INVALIDARG, "got %08lx\n", hr );
    ok( !info.dwAssemblyFlags, "got %08lx\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %lu\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.pszCurrentAssemblyPathBuf == NULL, "got %p\n", info.pszCurrentAssemblyPathBuf );
    ok( !info.cchBuf, "got %lu\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 2, comctlW, &info );
    ok( hr == E_INVALIDARG, "got %08lx\n", hr );
    ok( !info.dwAssemblyFlags, "got %08lx\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %lu\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.pszCurrentAssemblyPathBuf == NULL, "got %p\n", info.pszCurrentAssemblyPathBuf );
    ok( !info.cchBuf, "got %lu\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    info.pszCurrentAssemblyPathBuf = path;
    info.cchBuf = ARRAY_SIZE( path );
    path[0] = 0;
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 2, comctlW, &info );
    ok( hr == E_INVALIDARG, "got %08lx\n", hr );
    ok( !info.dwAssemblyFlags, "got %08lx\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %lu\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.cchBuf == ARRAY_SIZE( path ), "got %lu\n", info.cchBuf );
    ok( !path[0], "got %s\n", wine_dbgstr_w(path) );

    IAssemblyCache_Release( cache );
}

START_TEST(cache)
{
    test_QueryAssemblyInfo();
}
