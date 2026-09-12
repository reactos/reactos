/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <wininet.h>

#include "wine/test.h"

static BOOL old_jsproxy;

static BOOL (WINAPI *pInternetInitializeAutoProxyDll)
    (DWORD, LPSTR, LPSTR, AutoProxyHelperFunctions *, AUTO_PROXY_SCRIPT_BUFFER *);
static BOOL (WINAPI *pInternetDeInitializeAutoProxyDll)(LPSTR, DWORD);
static BOOL (WINAPI *pInternetGetProxyInfo)(LPCSTR, DWORD, LPSTR, DWORD, LPSTR *, LPDWORD);

static void test_InternetInitializeAutoProxyDll(void)
{
    const char url[] = "http://localhost";
    char script[] = "function FindProxyForURL(url, host) {return \"DIRECT\";}\0test";
    char script2[] = "function FindProxyForURL(url, host) {return \"PROXY 10.0.0.1:8080\";}\0test";
    char script3[] = "function FindProxyForURL(url, host) {return \"DIRECT\";}";
    char *proxy, host[] = "localhost";
    AUTO_PROXY_SCRIPT_BUFFER buf;
    DWORD err, len;
    BOOL ret;

    buf.dwStructSize = sizeof(buf);
    buf.lpszScriptBuffer = script;
    buf.dwScriptBufferSize = 0;
    SetLastError( 0xdeadbeef );
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    err = GetLastError();
    ok( !ret, "unexpected success\n" );
    if (!ret && err == 0xdeadbeef)
    {
        win_skip("InternetInitializeAutoProxyDll() is not supported on Windows 11\n");
        return;
    }
    ok( err == ERROR_INVALID_PARAMETER, "got %lu\n", err );

    buf.dwScriptBufferSize = strlen(script) + 1;
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    ok( ret, "got %lu\n", GetLastError() );

    ret = pInternetGetProxyInfo( url, strlen(url), host, strlen(host), &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "DIRECT" ), "got \"%s\"\n", proxy );
    ok( len == strlen(proxy) + 1, "got len=%ld for \"%s\"\n", len, proxy );
    GlobalFree( proxy );

    buf.dwScriptBufferSize = strlen(script2) + 1;
    buf.lpszScriptBuffer = script2;
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    ok( ret, "got %lu\n", GetLastError() );

    ret = pInternetGetProxyInfo( url, strlen(url), host, strlen(host), &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "PROXY 10.0.0.1:8080" ), "got \"%s\"\n", proxy );
    ok( len == strlen(proxy) + 1, "got len=%ld for \"%s\"\n", len, proxy );
    GlobalFree( proxy );

    buf.dwScriptBufferSize = strlen(script2) + 2;
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    ok( ret, "got %lu\n", GetLastError() );

    ret = pInternetGetProxyInfo( url, strlen(url), host, strlen(host), &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "PROXY 10.0.0.1:8080" ), "got \"%s\"\n", proxy );
    ok( len == strlen(proxy) + 1, "got len=%ld for \"%s\"\n", len, proxy );
    GlobalFree( proxy );

    buf.lpszScriptBuffer = script3;
    buf.dwScriptBufferSize = strlen(script3);
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    ok( ret || broken(old_jsproxy && !ret), "got %lu\n", GetLastError() );

    buf.dwScriptBufferSize = 1;
    script3[0] = 0;
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    ok( ret, "got %lu\n", GetLastError() );

    ret = pInternetDeInitializeAutoProxyDll( NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );
}

static void test_InternetGetProxyInfo(void)
{
    const char url[] = "http://localhost";
    char script[] = "function FindProxyForURL(url, host) { "
        "if (url.substring(0, 4) === 'test') return url + ' ' + host; "
        "return \"DIRECT\"; }";
    char *proxy, host[] = "localhost";
    AUTO_PROXY_SCRIPT_BUFFER buf;
    DWORD len, err;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = pInternetGetProxyInfo( url, strlen(url), host, strlen(host), &proxy, &len );
    err = GetLastError();
    if (!ret && err == 0xdeadbeef)
    {
        win_skip("InternetGetProxyInfo() is not supported on Windows 11\n");
        return;
    }
    ok( !ret, "unexpected success\n" );
    ok( err == ERROR_CAN_NOT_COMPLETE, "got %lu\n", err );

    buf.dwStructSize = sizeof(buf);
    buf.lpszScriptBuffer = script;
    buf.dwScriptBufferSize = strlen(script) + 1;
    ret = pInternetInitializeAutoProxyDll( 0, NULL, NULL, NULL, &buf );
    ok( ret, "got %lu\n", GetLastError() );

    len = 0;
    proxy = NULL;
    ret = pInternetGetProxyInfo( url, strlen(url), host, strlen(host), &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "DIRECT" ), "got \"%s\"\n", proxy );
    ok( len == strlen("DIRECT") + 1, "got %lu\n", len );
    GlobalFree( proxy );

    len = 0;
    proxy = NULL;
    ret = pInternetGetProxyInfo( url, strlen(url) + 1, host, strlen(host), &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "DIRECT" ), "got \"%s\"\n", proxy );
    ok( len == strlen("DIRECT") + 1, "got %lu\n", len );
    GlobalFree( proxy );

    len = 0;
    proxy = NULL;
    ret = pInternetGetProxyInfo( url, strlen(url) - 1, host, strlen(host), &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "DIRECT" ), "got \"%s\"\n", proxy );
    ok( len == strlen("DIRECT") + 1, "got %lu\n", len );
    GlobalFree( proxy );

    len = 0;
    proxy = NULL;
    ret = pInternetGetProxyInfo( url, strlen(url), host, strlen(host) + 1, &proxy, &len );
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "DIRECT" ), "got \"%s\"\n", proxy );
    ok( len == strlen("DIRECT") + 1, "got %lu\n", len );
    GlobalFree( proxy );

    len = 0;
    proxy = NULL;
    ret = pInternetGetProxyInfo( "testa", 4, host, 4, &proxy, &len);
    ok( ret, "got %lu\n", GetLastError() );
    ok( !strcmp( proxy, "test loca" ), "got \"%s\"\n", proxy );
    ok( len == 10, "got %lu\n", len );
    GlobalFree( proxy );

    ret = pInternetDeInitializeAutoProxyDll( NULL, 0 );
    ok( ret, "got %lu\n", GetLastError() );
}

START_TEST(jsproxy)
{
    HMODULE module = LoadLibraryA( "jsproxy.dll" );
    pInternetInitializeAutoProxyDll = (void *)GetProcAddress( module, "InternetInitializeAutoProxyDll" );
    pInternetDeInitializeAutoProxyDll = (void *)GetProcAddress( module, "InternetDeInitializeAutoProxyDll" );
    pInternetGetProxyInfo = (void *)GetProcAddress( module, "InternetGetProxyInfo" );

    if (!pInternetInitializeAutoProxyDll)
    {
        win_skip( "InternetInitializeAutoProxyDll not available\n" );
        return;
    }

    old_jsproxy = !GetProcAddress( module, "InternetGetProxyInfoEx" );
    if (old_jsproxy)
        trace( "InternetGetProxyInfoEx not available\n" );

    test_InternetInitializeAutoProxyDll();
    test_InternetGetProxyInfo();
}
