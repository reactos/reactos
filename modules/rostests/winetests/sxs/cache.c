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
    static const WCHAR emptyW[] = {0};
    static const WCHAR wine1W[] = {'w','i','n','e',0};
    static const WCHAR wine2W[] =
        {'w','i','n','e',',',
         'v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',0};
    static const WCHAR wine3W[] =
        {'w','i','n','e',',',
         'v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',0};
    static const WCHAR wine4W[] =
        {'w','i','n','e',',',
         'v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',',',
         'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',
         '\"','x','8','6','\"',0};
    static const WCHAR wine5W[] =
        {'w','i','n','e',',',
         'v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',',',
         'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',
         '\"','x','8','6','\"',',','p','u','b','l','i','c','K','e','y','T','o','k','e','n','=',
         '\"','1','2','3','4','5','6','7','8','9','0','a','b','c','d','e','f','\"',0};
    static const WCHAR comctl1W[] =
        {'m','i','c','r','o','s','o','f','t','.','w','i','n','d','o','w','s','.',
         'c','o','m','m','o','n','-','c','o','n','t','r','o','l','s',',',
         'v','e','r','s','i','o','n','=','\"','6','.','0','.','2','6','0','0','.','2','9','8','2','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',',',
         'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',
         '\"','x','8','6','\"',',','p','u','b','l','i','c','K','e','y','T','o','k','e','n','=',
         '\"','6','5','9','5','b','6','4','1','4','4','c','c','f','1','d','f','\"',0};
    static const WCHAR comctl2W[] =
        {'m','i','c','r','o','s','o','f','t','.','w','i','n','d','o','w','s','.',
         'c','o','m','m','o','n','-','c','o','n','t','r','o','l','s',',',
         'v','e','r','s','i','o','n','=','\"','6','.','0','.','3','7','9','0','.','4','7','7','0','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',',',
         'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',
         '\"','x','8','6','\"',',','p','u','b','l','i','c','K','e','y','T','o','k','e','n','=',
         '\"','6','5','9','5','b','6','4','1','4','4','c','c','f','1','d','f','\"',0};
    static const WCHAR comctl3W[] =
        {'m','i','c','r','o','s','o','f','t','.','w','i','n','d','o','w','s','.',
         'c','o','m','m','o','n','-','c','o','n','t','r','o','l','s',',',
         'v','e','r','s','i','o','n','=','\"','6','.','0','.','8','2','5','0','.','0','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',',',
         'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',
         '\"','x','8','6','\"',',','p','u','b','l','i','c','K','e','y','T','o','k','e','n','=',
         '\"','6','5','9','5','b','6','4','1','4','4','c','c','f','1','d','f','\"',0};
    static const WCHAR comctl4W[] =
        {'m','i','c','r','o','s','o','f','t','.','w','i','n','d','o','w','s','.',
         'c','o','m','m','o','n','-','c','o','n','t','r','o','l','s',',',
         'v','e','r','s','i','o','n','=','\"','6','.','0','.','7','6','0','1','.','1','7','5','1','4','\"',',',
         't','y','p','e','=','\"','w','i','n','3','2','\"',',',
         'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e','=',
         '\"','x','8','6','\"',',','p','u','b','l','i','c','K','e','y','T','o','k','e','n','=',
         '\"','6','5','9','5','b','6','4','1','4','4','c','c','f','1','d','f','\"',0};
    HRESULT hr;
    ASSEMBLY_INFO info;
    IAssemblyCache *cache = NULL;
    WCHAR path[MAX_PATH];
    char comctl_path1[MAX_PATH], comctl_path2[MAX_PATH], comctl_path3[MAX_PATH], comctl_path4[MAX_PATH];
    const WCHAR *comctlW;

    hr = CreateAssemblyCache( &cache, 0 );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( cache != NULL, "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, emptyW, NULL );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wine1W, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE ) ||
        broken(hr == E_INVALIDARG) /* winxp */, "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wine2W, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE ) ||
        broken(hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND )) /* winxp */, "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wine3W, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE ) ||
        broken(hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND )) /* winxp */, "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wine4W, NULL );
    todo_wine ok( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), "got %08x\n", hr );

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wine5W, NULL );
    todo_wine ok( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), "got %08x\n", hr );

    GetWindowsDirectoryA( comctl_path1, MAX_PATH );
    lstrcatA( comctl_path1, "\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.2600.2982_none_deadbeef" );
    GetWindowsDirectoryA( comctl_path2, MAX_PATH );
    lstrcatA( comctl_path2, "\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.3790.4770_x-ww_05fdf087" );
    GetWindowsDirectoryA( comctl_path3, MAX_PATH );
    lstrcatA( comctl_path3, "\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.8250.0_none_c119e7cca62b92bd" );
    GetWindowsDirectoryA( comctl_path4, MAX_PATH );
    lstrcatA( comctl_path4, "\\winsxs\\x86_microsoft.windows.common-controls_6595b64144ccf1df_6.0.7601.17514_none_41e6975e2bd6f2b2" );
    if (GetFileAttributesA( comctl_path1 ) != INVALID_FILE_ATTRIBUTES) comctlW = comctl1W;
    else if (GetFileAttributesA( comctl_path2 ) != INVALID_FILE_ATTRIBUTES) comctlW = comctl2W;
    else if (GetFileAttributesA( comctl_path3 ) != INVALID_FILE_ATTRIBUTES) comctlW = comctl3W;
    else if (GetFileAttributesA( comctl_path4 ) != INVALID_FILE_ATTRIBUTES) comctlW = comctl4W;
    else
    {
        skip( "no assembly to test with\n" );
        IAssemblyCache_Release( cache );
        return;
    }

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, wine5W, &info );
    todo_wine ok( hr == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ), "got %08x\n", hr );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, &info );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( info.dwAssemblyFlags == 1, "got %08x\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %u\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.pszCurrentAssemblyPathBuf == NULL, "got %p\n", info.pszCurrentAssemblyPathBuf );
    ok( !info.cchBuf, "got %u\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    info.pszCurrentAssemblyPathBuf = path;
    info.cchBuf = ARRAY_SIZE( path );
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, &info );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( info.dwAssemblyFlags == 1, "got %08x\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %u\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.cchBuf == ARRAY_SIZE( path ), "got %u\n", info.cchBuf );
    ok( path[0], "empty path\n" );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    info.pszCurrentAssemblyPathBuf = path;
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, comctlW, &info );
    ok( hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ), "got %08x\n", hr );
    ok( info.dwAssemblyFlags == 1, "got %08x\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %u\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.cchBuf, "got %u\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 1, comctlW, &info );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( !info.dwAssemblyFlags, "got %08x\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %u\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.pszCurrentAssemblyPathBuf == NULL, "got %p\n", info.pszCurrentAssemblyPathBuf );
    ok( !info.cchBuf, "got %u\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 2, comctlW, &info );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( !info.dwAssemblyFlags, "got %08x\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %u\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.pszCurrentAssemblyPathBuf == NULL, "got %p\n", info.pszCurrentAssemblyPathBuf );
    ok( !info.cchBuf, "got %u\n", info.cchBuf );

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    info.pszCurrentAssemblyPathBuf = path;
    info.cchBuf = ARRAY_SIZE( path );
    path[0] = 0;
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 2, comctlW, &info );
    ok( hr == E_INVALIDARG, "got %08x\n", hr );
    ok( !info.dwAssemblyFlags, "got %08x\n", info.dwAssemblyFlags );
    ok( !info.uliAssemblySizeInKB.u.LowPart, "got %u\n", info.uliAssemblySizeInKB.u.LowPart );
    ok( info.cchBuf == ARRAY_SIZE( path ), "got %u\n", info.cchBuf );
    ok( !path[0], "got %s\n", wine_dbgstr_w(path) );

    IAssemblyCache_Release( cache );
}

START_TEST(cache)
{
    test_QueryAssemblyInfo();
}
