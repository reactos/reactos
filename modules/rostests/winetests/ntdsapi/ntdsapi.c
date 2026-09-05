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
    static const WCHAR wszServiceClass[] = L"class";
    static const WCHAR wszServiceHost[] = L"host";
    static const WCHAR wszInstanceName[] = L"instance";
    static const WCHAR wszReferrer[] = L"referrer";
    static const WCHAR wszSpn1[] = L"class/host";
    static const WCHAR wszSpn2[] = L"class/instance/host";
    static const WCHAR wszSpn3[] = L"class/instance:555/host";
    static const WCHAR wszSpn4[] = L"class/instance:555/host";
    static const WCHAR wszSpn5[] = L"class/host:555";

    spn[0] = '\0';

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(NULL, NULL, NULL, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_INVALID_PARAMETER, "DsMakeSpnW should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", ret);

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(NULL, wszServiceHost, NULL, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_INVALID_PARAMETER, "DsMakeSpnW should have failed with ERROR_INVALID_PARAMETER instead of %ld\n", ret);

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, NULL, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %ld\n", ret);
    ok(!lstrcmpW(spn, wszSpn1), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn1) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %ld\n", lstrlenW(wszSpn1) + 1, spn_length);

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, wszInstanceName, 0, NULL, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %ld\n", ret);
    ok(!lstrcmpW(spn, wszSpn2), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn2) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %ld\n", lstrlenW(wszSpn2) + 1, spn_length);

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, wszInstanceName, 555, NULL, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %ld\n", ret);
    ok(!lstrcmpW(spn, wszSpn3), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn3) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %ld\n", lstrlenW(wszSpn3) + 1, spn_length);

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, wszInstanceName, 555, wszReferrer, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %ld\n", ret);
    ok(!lstrcmpW(spn, wszSpn4), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn4) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %ld\n", lstrlenW(wszSpn4) + 1, spn_length);

    spn_length = ARRAY_SIZE(spn);
    ret = DsMakeSpnW(wszServiceClass, wszServiceHost, NULL, 555, wszReferrer, &spn_length, spn);
    ok(ret == ERROR_SUCCESS, "DsMakeSpnW should have succeeded instead of failing with %ld\n", ret);
    ok(!lstrcmpW(spn, wszSpn5), "DsMakeSpnW returned unexpected SPN %s\n", wine_dbgstr_w(spn));
    ok(spn_length == lstrlenW(wszSpn5) + 1, "DsMakeSpnW should have returned spn_length of %d instead of %ld\n", lstrlenW(wszSpn5) + 1, spn_length);
}

static void test_DsClientMakeSpnForTargetServer(void)
{
    static const WCHAR classW[] = L"class";
    static const WCHAR hostW[] = L"host.domain";
    static const WCHAR resultW[] = L"class/host.domain";
    DWORD ret, len;
    WCHAR buf[256];

    ret = DsClientMakeSpnForTargetServerW( NULL, NULL, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = DsClientMakeSpnForTargetServerW( classW, NULL, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = DsClientMakeSpnForTargetServerW( classW, hostW, NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    len = 0;
    ret = DsClientMakeSpnForTargetServerW( classW, hostW, &len, NULL );
    ok( ret == ERROR_BUFFER_OVERFLOW, "got %lu\n", ret );
    ok( len == lstrlenW(resultW) + 1, "got %lu\n", len );

    len = ARRAY_SIZE(buf);
    buf[0] = 0;
    ret = DsClientMakeSpnForTargetServerW( classW, hostW, &len, buf );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    ok( len == lstrlenW(resultW) + 1, "got %lu\n", len );
    ok( !lstrcmpW( buf, resultW ), "wrong data\n" );
}

START_TEST( ntdsapi )
{
    test_DsMakeSpn();
    test_DsClientMakeSpnForTargetServer();
}
