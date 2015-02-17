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

#define COBJMACROS

#include <stdarg.h>
#include "windows.h"
#include "initguid.h"
#include "wmiutils.h"
#include "wbemcli.h"
#include "wine/test.h"

static const WCHAR path1[] = {0};
static const WCHAR path2[] = {'\\',0};
static const WCHAR path3[] = {'\\','\\','s','e','r','v','e','r',0};
static const WCHAR path4[] = {'\\','\\','s','e','r','v','e','r','\\',0};
static const WCHAR path5[] = {'\\','\\','.','\\',0};
static const WCHAR path6[] = {'/','/','.','/','r','o','o','t','/','c','i','m','v','2',0};
static const WCHAR path7[] =
    {'/','/','.','/','r','o','o','t','/','c','i','m','v','2',':','W','i','n','3','2','_',
     'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path8[] =
    {'/','r','o','o','t','/','c','i','m','v','2',':','W','i','n','3','2','_',
     'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path9[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path10[] =
    {'/','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path11[] =
    {'/','/','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path12[] =
    {'r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path13[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',0};
static const WCHAR path14[] =
    {'W','i','n','3','2','_','O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR path15[] =
    {'r','o','o','t','\\','c','i','m','v','2',0};
static const WCHAR path16[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',0};
static const WCHAR path17[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','d','=',
     '"','C',':','"',0};
static const WCHAR path18[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','d','=',
     '"','C',':','"',',','D','r','i','v','e','T','y','p','e','=','3',0};
static const WCHAR path19[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','d','=',0};
static const WCHAR path20[] =
    {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
     'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','d',' ','=',' ',
     '"','C',':','"',0};

static IWbemPath *create_path(void)
{
    HRESULT hr;
    IWbemPath *path;

    hr = CoCreateInstance( &CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemPath, (void **)&path );
    if (hr != S_OK)
    {
        win_skip( "can't create WbemDefPath instance, skipping tests\n" );
        return NULL;
    }
    return path;
}

static void test_IWbemPath_SetText(void)
{
    static const struct
    {
        const WCHAR *path;
        ULONG        mode;
        HRESULT      ret;
        BOOL         todo;
    } test[] =
    {
        { path1, 0, WBEM_E_INVALID_PARAMETER },
        { path1, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path2, 0, WBEM_E_INVALID_PARAMETER },
        { path2, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path3, 0, WBEM_E_INVALID_PARAMETER },
        { path3, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path4, 0, WBEM_E_INVALID_PARAMETER },
        { path4, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path5, 0, WBEM_E_INVALID_PARAMETER },
        { path5, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path6, 0, WBEM_E_INVALID_PARAMETER },
        { path6, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path7, 0, WBEM_E_INVALID_PARAMETER },
        { path7, WBEMPATH_CREATE_ACCEPT_RELATIVE, S_OK },
        { path7, WBEMPATH_CREATE_ACCEPT_ABSOLUTE, S_OK },
        { path7, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path7, WBEMPATH_TREAT_SINGLE_IDENT_AS_NS, WBEM_E_INVALID_PARAMETER, TRUE },
        { path7, WBEMPATH_TREAT_SINGLE_IDENT_AS_NS + 1, S_OK },
        { path8, WBEMPATH_CREATE_ACCEPT_RELATIVE, S_OK },
        { path8, WBEMPATH_CREATE_ACCEPT_ABSOLUTE, WBEM_E_INVALID_PARAMETER, TRUE },
        { path8, WBEMPATH_CREATE_ACCEPT_ALL, S_OK },
        { path8, WBEMPATH_TREAT_SINGLE_IDENT_AS_NS, WBEM_E_INVALID_PARAMETER, TRUE },
        { path8, WBEMPATH_TREAT_SINGLE_IDENT_AS_NS + 1, S_OK },
        { path9, WBEMPATH_CREATE_ACCEPT_ABSOLUTE, S_OK },
        { path10, WBEMPATH_CREATE_ACCEPT_ABSOLUTE, WBEM_E_INVALID_PARAMETER, TRUE },
        { path11, WBEMPATH_CREATE_ACCEPT_ABSOLUTE, S_OK },
        { path15, WBEMPATH_CREATE_ACCEPT_ALL, S_OK }
    };
    IWbemPath *path;
    HRESULT hr;
    UINT i;

    if (!(path = create_path())) return;

    hr = IWbemPath_SetText( path, 0, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    for (i = 0; i < sizeof(test)/sizeof(test[0]); i++)
    {
        hr = IWbemPath_SetText( path, test[i].mode, test[i].path );
        if (test[i].todo) todo_wine ok( hr == test[i].ret, "%u got %08x\n", i, hr );
        else ok( hr == test[i].ret, "%u got %08x\n", i, hr );

        if (test[i].ret == S_OK)
        {
            WCHAR buf[128];
            ULONG len;

            memset( buf, 0x55, sizeof(buf) );
            len = sizeof(buf)/sizeof(buf[0]);
            hr = IWbemPath_GetText( path, WBEMPATH_GET_ORIGINAL, &len, buf );
            ok( hr == S_OK, "%u got %08x\n", i, hr );
            ok( !lstrcmpW( buf, test[i].path ), "%u unexpected path %s\n", i, wine_dbgstr_w(buf) );
            ok( len == lstrlenW( test[i].path ) + 1, "%u unexpected length %u\n", i, len );
        }
    }
    IWbemPath_Release( path );
}

static void test_IWbemPath_GetText(void)
{
    static const WCHAR serviceW[] =
        {'W','i','n','3','2','_','S','e','r','v','i','c','e','.','N','a','m','e','=',
         '\"','S','e','r','v','i','c','e','\"',0};
    static const WCHAR classW[] =
        {'W','i','n','3','2','_','C','l','a','s','s',0};
    static const WCHAR expected1W[] =
        {'r','o','o','t','\\','c','i','m','v','2',':','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','d','=',
         '"','C',':','"',0};
    static const WCHAR expected2W[] =
        {'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k','.',
         'D','e','v','i','c','e','I','d','=','"','C',':','"',0};
    static const WCHAR expected3W[] =
        {'\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',0};
    WCHAR buf[128];
    ULONG len, count;
    IWbemPath *path;
    HRESULT hr;

    if (!(path = create_path())) return;

    hr = IWbemPath_GetText( path, 0, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = sizeof(buf)/sizeof(buf[0]);
    hr = IWbemPath_GetText( path, 0, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf)/sizeof(buf[0]), "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !buf[0], "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == 1, "unexpected length %u\n", len );

    hr = IWbemPath_GetText( path, WBEMPATH_GET_ORIGINAL, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = sizeof(buf)/sizeof(buf[0]);
    hr = IWbemPath_GetText( path, WBEMPATH_GET_ORIGINAL, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf)/sizeof(buf[0]), "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_ORIGINAL, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !buf[0], "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_TOO, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    todo_wine ok( !buf[0], "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    todo_wine ok( len == 1, "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path8 );
    ok( hr == S_OK, "got %08x\n", hr );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 2, "got %u\n", count );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_TOO, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path9 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path9 ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path13 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path13 ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_RELATIVE_ONLY, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path14 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path14 ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_NAMESPACE_ONLY, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path15 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path15 ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path12 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path12 ) + 1, "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path1 );
    ok( hr == S_OK, "got %08x\n", hr );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !count, "got %u\n", count );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path6 );
    ok( hr == S_OK, "got %08x\n", hr );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 2, "got %u\n", count );

    len = 0;
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_TOO, &len, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( len == lstrlenW( path16 ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_TOO, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path16 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path16 ) + 1, "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_TOO, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, path17 ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( path17 ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, expected1W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( expected1W ) + 1, "unexpected length %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_RELATIVE_ONLY, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, expected2W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( expected2W ) + 1, "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path15 );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_SERVER_TOO, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, expected3W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( expected3W ) + 1, "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path18 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path19 );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path20 );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    IWbemPath_Release( path );
    if (!(path = create_path())) return;

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, serviceW );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_RELATIVE_ONLY, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, serviceW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( serviceW ) + 1, "unexpected length %u\n", len );

    IWbemPath_Release( path );
    if (!(path = create_path())) return;

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, classW );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf)/sizeof(buf[0]);
    memset( buf, 0x55, sizeof(buf) );
    hr = IWbemPath_GetText( path, WBEMPATH_GET_RELATIVE_ONLY, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, classW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( classW ) + 1, "unexpected length %u\n", len );

    IWbemPath_Release( path );
}

static void test_IWbemPath_GetClassName(void)
{
    static const WCHAR classW[] = {'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k',0};
    IWbemPath *path;
    HRESULT hr;
    WCHAR buf[32];
    ULONG len;

    if (!(path = create_path())) return;

    hr = IWbemPath_GetClassName( path, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = 0;
    hr = IWbemPath_GetClassName( path, &len, NULL );
    ok( hr == WBEM_E_INVALID_OBJECT_PATH, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetClassName( path, &len, buf );
    ok( hr == WBEM_E_INVALID_OBJECT_PATH, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetClassName( path, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf) / sizeof(buf[0]), "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    len = 0;
    hr = IWbemPath_GetClassName( path, &len, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetClassName( path, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf) / sizeof(buf[0]), "unexpected length %u\n", len );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetClassName( path, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, classW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( classW ) + 1, "unexpected length %u\n", len );

    IWbemPath_Release( path );
}

static void test_IWbemPath_SetClassName(void)
{
    static const WCHAR classW[] = {'c','l','a','s','s',0};
    static const WCHAR emptyW[] = {0};
    IWbemPath *path;
    WCHAR buf[16];
    ULONG len;
    ULONGLONG flags;
    HRESULT hr;

    if (!(path = create_path())) return;

    hr = IWbemPath_SetClassName( path, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_SetClassName( path, emptyW );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemPath_SetClassName( path, classW );
    ok( hr == S_OK, "got %08x\n", hr );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetClassName( path, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, classW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == (WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_IS_CLASS_REF |
                  WBEMPATH_INFO_HAS_SUBSCOPES | WBEMPATH_INFO_V2_COMPLIANT |
                  WBEMPATH_INFO_CIM_COMPLIANT),
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    IWbemPath_Release( path );
}

static void test_IWbemPath_GetServer(void)
{
    static const WCHAR dotW[] = {'.',0};
    IWbemPath *path;
    HRESULT hr;
    WCHAR buf[32];
    ULONG len;

    if (!(path = create_path())) return;

    hr = IWbemPath_GetServer( path, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = 0;
    hr = IWbemPath_GetServer( path, &len, NULL );
    ok( hr == WBEM_E_NOT_AVAILABLE, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, buf );
    ok( hr == WBEM_E_NOT_AVAILABLE, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf) / sizeof(buf[0]), "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    len = 0;
    hr = IWbemPath_GetServer( path, &len, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf) / sizeof(buf[0]), "unexpected length %u\n", len );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, dotW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( dotW ) + 1, "unexpected length %u\n", len );

    IWbemPath_Release( path );
}

static void test_IWbemPath_GetInfo(void)
{
    IWbemPath *path;
    HRESULT hr;
    ULONGLONG resp;

    if (!(path = create_path())) return;

    hr = IWbemPath_GetInfo( path, 0, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_GetInfo( path, 1, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    resp = 0xdeadbeef;
    hr = IWbemPath_GetInfo( path, 0, &resp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( resp == (WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_SERVER_NAMESPACE_ONLY),
        "got %lx%08lx\n", (unsigned long)(resp >> 32), (unsigned long)resp );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemPath_GetInfo( path, 0, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_GetInfo( path, 1, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    resp = 0xdeadbeef;
    hr = IWbemPath_GetInfo( path, 0, &resp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( resp == (WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_IS_INST_REF |
                 WBEMPATH_INFO_HAS_SUBSCOPES | WBEMPATH_INFO_V2_COMPLIANT |
                 WBEMPATH_INFO_CIM_COMPLIANT | WBEMPATH_INFO_PATH_HAD_SERVER),
        "got %lx%08lx\n", (unsigned long)(resp >> 32), (unsigned long)resp );

    IWbemPath_Release( path );
    if (!(path = create_path())) return;

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path12 );
    ok( hr == S_OK, "got %08x\n", hr );

    resp = 0xdeadbeef;
    hr = IWbemPath_GetInfo( path, 0, &resp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( resp == (WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_IS_CLASS_REF |
                 WBEMPATH_INFO_HAS_SUBSCOPES | WBEMPATH_INFO_V2_COMPLIANT |
                 WBEMPATH_INFO_CIM_COMPLIANT),
        "got %lx%08lx\n", (unsigned long)(resp >> 32), (unsigned long)resp );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path1 );
    ok( hr == S_OK, "got %08x\n", hr );

    resp = 0xdeadbeef;
    hr = IWbemPath_GetInfo( path, 0, &resp );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( resp == (WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_SERVER_NAMESPACE_ONLY),
        "got %lx%08lx\n", (unsigned long)(resp >> 32), (unsigned long)resp );

    IWbemPath_Release( path );
}

static void test_IWbemPath_SetServer(void)
{
    static const WCHAR serverW[] = {'s','e','r','v','e','r',0};
    static const WCHAR emptyW[] = {0};
    IWbemPath *path;
    WCHAR buf[16];
    ULONG len;
    ULONGLONG flags;
    HRESULT hr;

    if (!(path = create_path())) return;

    hr = IWbemPath_SetServer( path, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, buf );
    ok( hr == WBEM_E_NOT_AVAILABLE, "got %08x\n", hr );

    hr = IWbemPath_SetServer( path, emptyW );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemPath_SetServer( path, serverW );
    ok( hr == S_OK, "got %08x\n", hr );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, serverW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == (WBEMPATH_INFO_HAS_MACHINE_NAME | WBEMPATH_INFO_V1_COMPLIANT |
                  WBEMPATH_INFO_V2_COMPLIANT | WBEMPATH_INFO_CIM_COMPLIANT |
                  WBEMPATH_INFO_SERVER_NAMESPACE_ONLY | WBEMPATH_INFO_PATH_HAD_SERVER),
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    hr = IWbemPath_SetServer( path, NULL );
    ok( hr == S_OK, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetServer( path, &len, buf );
    ok( hr == WBEM_E_NOT_AVAILABLE, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == (WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_SERVER_NAMESPACE_ONLY),
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    IWbemPath_Release( path );
}

static void test_IWbemPath_GetNamespaceAt(void)
{
    static const WCHAR rootW[] = {'r','o','o','t',0};
    static const WCHAR cimv2W[] = {'c','i','m','v','2',0};
    IWbemPath *path;
    HRESULT hr;
    WCHAR buf[32];
    ULONG len;

    if (!(path = create_path())) return;

    hr = IWbemPath_GetNamespaceAt( path, 0, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = 0;
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf) / sizeof(buf[0]), "unexpected length %u\n", len );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    len = 0;
    hr = IWbemPath_GetNamespaceAt( path, 2, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );
    ok( len == sizeof(buf) / sizeof(buf[0]), "unexpected length %u\n", len );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, rootW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( rootW ) + 1, "unexpected length %u\n", len );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 1, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, cimv2W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( cimv2W ) + 1, "unexpected length %u\n", len );

    IWbemPath_Release( path );
}

static void test_IWbemPath_RemoveAllNamespaces(void)
{
    static const ULONGLONG expected_flags =
        WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_IS_INST_REF |
        WBEMPATH_INFO_HAS_SUBSCOPES | WBEMPATH_INFO_V2_COMPLIANT |
        WBEMPATH_INFO_CIM_COMPLIANT | WBEMPATH_INFO_PATH_HAD_SERVER;
    IWbemPath *path;
    WCHAR buf[16];
    ULONG len;
    ULONGLONG flags;
    HRESULT hr;

    if (!(path = create_path())) return;

    hr = IWbemPath_RemoveAllNamespaces( path );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    hr = IWbemPath_RemoveAllNamespaces( path );
    ok( hr == S_OK, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    IWbemPath_Release( path );
}

static void test_IWbemPath_RemoveNamespaceAt(void)
{
    static const ULONGLONG expected_flags =
        WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_IS_INST_REF |
        WBEMPATH_INFO_HAS_SUBSCOPES | WBEMPATH_INFO_V2_COMPLIANT |
        WBEMPATH_INFO_CIM_COMPLIANT | WBEMPATH_INFO_PATH_HAD_SERVER;
    static const WCHAR cimv2W[] = {'c','i','m','v','2',0};
    IWbemPath *path;
    WCHAR buf[16];
    ULONG len, count;
    ULONGLONG flags;
    HRESULT hr;

    if (!(path = create_path())) return;

    hr = IWbemPath_RemoveNamespaceAt( path, 0 );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_SetText( path, WBEMPATH_CREATE_ACCEPT_ALL, path17 );
    ok( hr == S_OK, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 2, "got %u\n", count );

    hr = IWbemPath_RemoveNamespaceAt( path, 0 );
    ok( hr == S_OK, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 1, "got %u\n", count );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, cimv2W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( cimv2W ) + 1, "unexpected length %u\n", len );

    hr = IWbemPath_RemoveNamespaceAt( path, 0 );
    ok( hr == S_OK, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !count, "got %u\n", count );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    IWbemPath_Release( path );
}

static void test_IWbemPath_SetNamespaceAt(void)
{
    static const ULONGLONG expected_flags =
        WBEMPATH_INFO_ANON_LOCAL_MACHINE | WBEMPATH_INFO_V1_COMPLIANT |
        WBEMPATH_INFO_V2_COMPLIANT | WBEMPATH_INFO_CIM_COMPLIANT |
        WBEMPATH_INFO_SERVER_NAMESPACE_ONLY;
    static const WCHAR rootW[] = {'r','o','o','t',0};
    static const WCHAR cimv2W[] = {'c','i','m','v','2',0};
    IWbemPath *path;
    WCHAR buf[16];
    ULONG len, count;
    ULONGLONG flags;
    HRESULT hr;

    if (!(path = create_path())) return;

    hr = IWbemPath_SetNamespaceAt( path, 0, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_SetNamespaceAt( path, 1, cimv2W );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemPath_SetNamespaceAt( path, 0, cimv2W );
    ok( hr == S_OK, "got %08x\n", hr );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 1, "got %u\n", count );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, cimv2W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( cimv2W ) + 1, "unexpected length %u\n", len );

    hr = IWbemPath_SetNamespaceAt( path, 0, rootW );
    ok( hr == S_OK, "got %08x\n", hr );

    flags = 0;
    hr = IWbemPath_GetInfo( path, 0, &flags );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flags == expected_flags,
        "got %lx%08lx\n", (unsigned long)(flags >> 32), (unsigned long)flags );

    count = 0xdeadbeef;
    hr = IWbemPath_GetNamespaceCount( path, &count );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( count == 2, "got %u\n", count );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 0, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, rootW ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( rootW ) + 1, "unexpected length %u\n", len );

    buf[0] = 0;
    len = sizeof(buf) / sizeof(buf[0]);
    hr = IWbemPath_GetNamespaceAt( path, 1, &len, buf );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( !lstrcmpW( buf, cimv2W ), "unexpected buffer contents %s\n", wine_dbgstr_w(buf) );
    ok( len == lstrlenW( cimv2W ) + 1, "unexpected length %u\n", len );

    IWbemPath_Release( path );
}

START_TEST (path)
{
    CoInitialize( NULL );

    test_IWbemPath_SetText();
    test_IWbemPath_GetText();
    test_IWbemPath_GetClassName();
    test_IWbemPath_SetClassName();
    test_IWbemPath_GetServer();
    test_IWbemPath_GetInfo();
    test_IWbemPath_SetServer();
    test_IWbemPath_GetNamespaceAt();
    test_IWbemPath_RemoveAllNamespaces();
    test_IWbemPath_RemoveNamespaceAt();
    test_IWbemPath_SetNamespaceAt();

    CoUninitialize();
}
