/*
 * Copyright (C) 2008 Robert Shearman (for CodeWeavers)
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "rpc.h"
#include "rpcdce.h"
#include "ntdsapi.h"

#include "wine/test.h"

static void test_DsMakeSpn(void)
{
    DWORD ret;
    WCHAR spn[256];
    DWORD spn_length;
    static const WCHAR wszServiceClass[] = {'c','l','a','s','s',0};
    static const WCHAR wszServiceHost[] = {'h','o','s','t',0};
    static const WCHAR wszInstanceName[] = {'i','n','s','t','a','n','c','e',0};
    static const WCHAR wszReferrer[] = {'r','e','f','e','r','r','e','r',0};
    static const WCHAR wszSpn1[] = {'c','l','a','s','s','/','h','o','s','t',0};
    static const WCHAR wszSpn2[] = {'c','l','a','s','s','/','i','n','s','t','a','n','c','e','/','h','o','s','t',0};
    static const WCHAR wszSpn3[] = {'c','l','a','s','s','/','i','n','s','t','a','n','c','e',':','5','5','5','/','h','o','s','t',0};
    static const WCHAR wszSpn4[] = {'c','l','a','s','s','/','i','n','s','t','a','n','c','e',':','5','5','5','/','h','o','s','t',0};
    static const WCHAR wszSpn5[] = {'c','l','a','s','s','/','h','o','s','t',':','5','5','5',0};

    spn[0] = '\0';

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(NULL, NULL, NULL, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_INVALID_PARAMETER, "DsMakeSpnW should have failed with ERROR_INVALID_PARAMETER instead of %d\n", ret);

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(NULL, wszServiceHost, NULL, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_INVALID_PARAMETER, "DsMakeSpnW should have failed with ERROR_INVALID_PARAMETER instead of %d\n", ret);

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, NULL, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %d\n", ret);
    ok(!lstrcmpW(spn, wszSpn1), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn1) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %d\n", lstrlenW(wszSpn1) + 1, spn_length);

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, wszInstanceName, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %d\n", ret);
    ok(!lstrcmpW(spn, wszSpn2), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn2) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %d\n", lstrlenW(wszSpn2) + 1, spn_length);

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, wszInstanceName, 555, NULL, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %d\n", ret);
    ok(!lstrcmpW(spn, wszSpn3), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn3) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %d\n", lstrlenW(wszSpn3) + 1, spn_length);

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, wszInstanceName, 555, wszReferrer, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %d\n", ret);
    ok(!lstrcmpW(spn, wszSpn4), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn4) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %d\n", lstrlenW(wszSpn4) + 1, spn_length);

    spn_length = sizeof(spn)/sizeof(spn[0]);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, NULL, 555, wszReferrer, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %d\n", ret);
    ok(!lstrcmpW(spn, wszSpn5), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn5) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %d\n", lstrlenW(wszSpn5) + 1, spn_length);
}

static void test_DsClientMakeSpnForTargetServer(void)
{
    static const WCHAR classW[] = {'c','l','a','s','s',0};
    static const WCHAR hostW[] = {'h','o','s','t','.','d','o','m','a','i','n',0};
    static const WCHAR resultW[] = {'c','l','a','s','s','/','h','o','s','t','.','d','o','m','a','i','n',0};
    DWORD ret, len;
    WCHAR buf[256];

    ret = DsClientMakeSpnForTargetServerW( NULL, NULL, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = DsClientMakeSpnForTargetServerW( classW, NULL, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = DsClientMakeSpnForTargetServerW( classW, hostW, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    len = 0;
    ret = DsClientMakeSpnForTargetServerW( classW, hostW, &len, NULL );
    ok( ret == ERROR_BUFFER_OVERFLOW, "got %u\n", ret );
    ok( len == lstrlenW(resultW) + 1, "got %u\n", len );

    len = sizeof(buf)/sizeof(buf[0]);
    buf[0] = 0;
    ret = DsClientMakeSpnForTargetServerW( classW, hostW, &len, buf );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    ok( len == lstrlenW(resultW) + 1, "got %u\n", len );
    ok( !lstrcmpW( buf, resultW ), "wrong data\n" );
}

START_TEST( ntdsapi )
{
    test_DsMakeSpn();
    test_DsClientMakeSpnForTargetServer();
}
