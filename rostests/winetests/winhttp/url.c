/*
 * Copyright 2008 Hans Leidekker
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
#include "winnls.h"
#include "winhttp.h"

#include "wine/test.h"

#define ICU_ESCAPE      0x80000000

static WCHAR empty[]    = {0};
static WCHAR ftp[]      = {'f','t','p',0};
static WCHAR http[]     = {'h','t','t','p',0};
static WCHAR winehq[]   = {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static WCHAR username[] = {'u','s','e','r','n','a','m','e',0};
static WCHAR password[] = {'p','a','s','s','w','o','r','d',0};
static WCHAR about[]    = {'/','s','i','t','e','/','a','b','o','u','t',0};
static WCHAR query[]    = {'?','q','u','e','r','y',0};
static WCHAR escape[]   = {' ','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/',':',';','<','=','>','?','@','[','\\',']','^','_','`','{','|','}','~',0};

static const WCHAR url1[]  =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url2[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url3[] = {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':',0};
static const WCHAR url4[] =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/',0};
static const WCHAR url6[] =
    {'f','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','8','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url7[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','4','2','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url8[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t',
     '%','2','0','!','%','2','2','%','2','3','$','%','2','5','&','\'','(',')','*','+',',','-','.','/',':',';','%','3','C','=','%','3','E','?','@','%',
     '5','B','%','5','C','%','5','D','%','5','E','_','%','6','0','%','7','B','%','7','C','%','7','D','%','7','E',0};
static const WCHAR url9[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url10[] =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','8','0','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};
static const WCHAR url11[] =
    {'h','t','t','p','s',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g',':','4','4','3','/','s','i','t','e','/','a','b','o','u','t','?','q','u','e','r','y',0};



static const WCHAR url_k1[]  =
    {'h','t','t','p',':','/','/','u','s','e','r','n','a','m','e',':','p','a','s','s','w','o','r','d',
     '@','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR url_k2[]  =
    {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k3[]  =
    {'h','t','t','p','s',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','p','o','s','t','?',0};
static const WCHAR url_k4[]  =
    {'H','T','T','P',':','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k5[]  =
    {'h','t','t','p',':','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k6[]  =
    {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR url_k7[]  =
    {'w','w','w',0};
static const WCHAR url_k8[]  =
    {'h','t','t','p',0};
static const WCHAR url_k9[]  =
    {'h','t','t','p',':','/','/','w','i','n','e','h','q','?',0};
static const WCHAR url_k10[]  =
    {'h','t','t','p',':','/','/','w','i','n','e','h','q','/','p','o','s','t',';','a',0};

static const char *debugstr_w(LPCWSTR str)
{
    static char buf[1024];
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static void fill_url_components( URL_COMPONENTS *uc )
{
    uc->dwStructSize = sizeof(URL_COMPONENTS);
    uc->lpszScheme = http;
    uc->dwSchemeLength = lstrlenW( uc->lpszScheme );
    uc->nScheme = INTERNET_SCHEME_HTTP;
    uc->lpszHostName = winehq;
    uc->dwHostNameLength = lstrlenW( uc->lpszHostName );
    uc->nPort = 80;
    uc->lpszUserName = username;
    uc->dwUserNameLength = lstrlenW( uc->lpszUserName );
    uc->lpszPassword = password;
    uc->dwPasswordLength = lstrlenW( uc->lpszPassword );
    uc->lpszUrlPath = about;
    uc->dwUrlPathLength = lstrlenW( uc->lpszUrlPath );
    uc->lpszExtraInfo = query;
    uc->dwExtraInfoLength = lstrlenW( uc->lpszExtraInfo );
}

static void WinHttpCreateUrl_test( void )
{
    URL_COMPONENTS uc;
    WCHAR *url;
    DWORD len;
    BOOL ret;

    /* NULL components */
    len = ~0u;
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( NULL, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0u, "expected len ~0u got %u\n", len );

    /* zero'ed components */
    memset( &uc, 0, sizeof(URL_COMPONENTS) );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0u, "expected len ~0u got %u\n", len );

    /* valid components, NULL url, NULL length */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, NULL );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );
    ok( len == ~0u, "expected len ~0u got %u\n", len );

    /* valid components, NULL url */
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* correct size, NULL url */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCreateUrl( &uc, 0, NULL, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* valid components, allocated url, short length */
    SetLastError( 0xdeadbeef );
    url = HeapAlloc( GetProcessHeap(), 0, 256 * sizeof(WCHAR) );
    url[0] = 0;
    len = 2;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER, "expected ERROR_INSUFFICIENT_BUFFER got %u\n", GetLastError() );
    ok( len == 57, "expected len 57 got %u\n", len );

    /* allocated url, NULL scheme */
    uc.lpszScheme = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url1 ), "url doesn't match\n" );

    /* allocated url, 0 scheme */
    fill_url_components( &uc );
    uc.nScheme = 0;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );

    /* valid components, allocated url */
    fill_url_components( &uc );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %d\n", len );
    ok( !lstrcmpW( url, url1 ), "url doesn't match\n" );

    /* valid username, NULL password */
    fill_url_components( &uc );
    uc.lpszPassword = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );

    /* valid username, empty password */
    fill_url_components( &uc );
    uc.lpszPassword = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url3 ), "url doesn't match\n" );

    /* valid password, NULL username */
    fill_url_components( &uc );
    SetLastError( 0xdeadbeef );
    uc.lpszUserName = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( !ret, "expected failure\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", GetLastError() );

    /* valid password, empty username */
    fill_url_components( &uc );
    uc.lpszUserName = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n");

    /* NULL username, NULL password */
    fill_url_components( &uc );
    uc.lpszUserName = NULL;
    uc.lpszPassword = NULL;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 38, "expected len 38 got %u\n", len );
    ok( !lstrcmpW( url, url4 ), "url doesn't match\n" );

    /* empty username, empty password */
    fill_url_components( &uc );
    uc.lpszUserName = empty;
    uc.lpszPassword = empty;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 56, "expected len 56 got %u\n", len );
    ok( !lstrcmpW( url, url5 ), "url doesn't match\n" );

    /* nScheme has lower precedence than lpszScheme */
    fill_url_components( &uc );
    uc.lpszScheme = ftp;
    uc.dwSchemeLength = lstrlenW( uc.lpszScheme );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == lstrlenW( url6 ), "expected len %d got %u\n", lstrlenW( url6 ) + 1, len );
    ok( !lstrcmpW( url, url6 ), "url doesn't match\n" );

    /* non-standard port */
    uc.lpszScheme = http;
    uc.dwSchemeLength = lstrlenW( uc.lpszScheme );
    uc.nPort = 42;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 59, "expected len 59 got %u\n", len );
    ok( !lstrcmpW( url, url7 ), "url doesn't match\n" );

    /* escape extra info */
    fill_url_components( &uc );
    uc.lpszExtraInfo = escape;
    uc.dwExtraInfoLength = lstrlenW( uc.lpszExtraInfo );
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, ICU_ESCAPE, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 113, "expected len 113 got %u\n", len );
    ok( !lstrcmpW( url, url8 ), "url doesn't match\n" );

    /* NULL lpszScheme, 0 nScheme and nPort */
    fill_url_components( &uc );
    uc.lpszScheme = NULL;
    uc.dwSchemeLength = 0;
    uc.nScheme = 0;
    uc.nPort = 0;
    url[0] = 0;
    len = 256;
    ret = WinHttpCreateUrl( &uc, 0, url, &len );
    ok( ret, "expected success\n" );
    ok( len == 58, "expected len 58 got %u\n", len );
    ok( !lstrcmpW( url, url9 ), "url doesn't match\n" );

    HeapFree( GetProcessHeap(), 0, url );
}

static void reset_url_components( URL_COMPONENTS *uc )
{
    memset( uc, 0, sizeof(URL_COMPONENTS) );
    uc->dwStructSize = sizeof(URL_COMPONENTS);
    uc->dwSchemeLength    = ~0u;
    uc->dwHostNameLength  = ~0u;
    uc->nPort             =  0;
    uc->dwUserNameLength  = ~0u;
    uc->dwPasswordLength  = ~0u;
    uc->dwUrlPathLength   = ~0u;
    uc->dwExtraInfoLength = ~0u;
}

static void WinHttpCrackUrl_test( void )
{
    URL_COMPONENTSW uc;
    WCHAR scheme[20], user[20], pass[20], host[20], path[40], extra[20];
    DWORD error;
    BOOL ret;

    /* buffers of sufficient length */
    scheme[0] = 0;
    user[0] = 0;
    pass[0] = 0;
    host[0] = 0;
    path[0] = 0;
    extra[0] = 0;

    uc.dwStructSize = sizeof(URL_COMPONENTS);
    uc.nScheme = 0;
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszUserName = user;
    uc.dwUserNameLength = 20;
    uc.lpszPassword = pass;
    uc.dwPasswordLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.nPort = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 20;

    ret = WinHttpCrackUrl( url1, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme\n" );
    ok( !memcmp( uc.lpszScheme, http, sizeof(http) ), "unexpected scheme\n" );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length\n" );
    ok( !memcmp( uc.lpszUserName, username, sizeof(username) ), "unexpected username\n" );
    ok( uc.dwUserNameLength == 8, "unexpected username length\n" );
    ok( !memcmp( uc.lpszPassword, password, sizeof(password) ), "unexpected password\n" );
    ok( uc.dwPasswordLength == 8, "unexpected password length\n" );
    ok( !memcmp( uc.lpszHostName, winehq, sizeof(winehq) ), "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( !memcmp( uc.lpszUrlPath, about, sizeof(about) ), "unexpected path\n" );
    ok( uc.dwUrlPathLength == 11, "unexpected path length\n" );
    ok( !memcmp( uc.lpszExtraInfo, query, sizeof(query) ), "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 6, "unexpected extra info length\n" );

    /* buffer of insufficient length */
    scheme[0] = 0;
    uc.dwSchemeLength = 1;

    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( url1, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl failed\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER, "WinHttpCrackUrl failed\n" );
    ok( uc.dwSchemeLength == 5, "unexpected scheme length: %u\n", uc.dwSchemeLength );

    /* no buffers */
    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k1, 0, 0,&uc);

    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme\n" );
    ok( uc.lpszScheme == url_k1,"unexpected scheme\n" );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length\n" );
    ok( uc.lpszUserName == url_k1 + 7, "unexpected username\n" );
    ok( uc.dwUserNameLength == 8, "unexpected username length\n" );
    ok( uc.lpszPassword == url_k1 + 16, "unexpected password\n" );
    ok( uc.dwPasswordLength == 8, "unexpected password length\n" );
    ok( uc.lpszHostName == url_k1 + 25, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url_k1 + 39, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 11, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k1 + 50, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k2, 0, 0,&uc);

    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nScheme == INTERNET_SCHEME_HTTP, "unexpected scheme\n" );
    ok( uc.lpszScheme == url_k2, "unexpected scheme\n" );
    ok( uc.dwSchemeLength == 4, "unexpected scheme length\n" );
    ok( uc.lpszUserName == NULL ,"unexpected username\n" );
    ok( uc.dwUserNameLength == 0, "unexpected username length\n" );
    ok( uc.lpszPassword == NULL, "unexpected password\n" );
    ok( uc.dwPasswordLength == 0, "unexpected password length\n" );
    ok( uc.lpszHostName == url_k2 + 7, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url_k2 + 21, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 0, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k2 + 21, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k3, 0, 0, &uc );

    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nScheme == INTERNET_SCHEME_HTTPS, "unexpected scheme\n" );
    ok( uc.lpszScheme == url_k3, "unexpected scheme\n" );
    ok( uc.dwSchemeLength == 5, "unexpected scheme length\n" );
    ok( uc.lpszUserName == NULL, "unexpected username\n" );
    ok( uc.dwUserNameLength == 0, "unexpected username length\n" );
    ok( uc.lpszPassword == NULL, "unexpected password\n" );
    ok( uc.dwPasswordLength == 0, "unexpected password length\n" );
    ok( uc.lpszHostName == url_k3 + 8, "unexpected hostname\n" );
    ok( uc.dwHostNameLength == 14, "unexpected hostname length\n" );
    ok( uc.nPort == 443, "unexpected port: %u\n", uc.nPort );
    ok( uc.lpszUrlPath == url_k3 + 22, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 5, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k3 + 27, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 1, "unexpected extra info length\n" );

    /* bad parameters */
    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k4, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k5, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k6, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k7, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k8, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k9, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.lpszUrlPath == url_k9 + 14, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 0, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k9 + 14, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url_k10, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.lpszUrlPath == url_k10 + 13, "unexpected path\n" );
    ok( uc.dwUrlPathLength == 7, "unexpected path length\n" );
    ok( uc.lpszExtraInfo == url_k10 + 20, "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 0, "unexpected extra info length\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url5, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( empty, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    ret = WinHttpCrackUrl( url1, 0, 0, NULL );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    ret = WinHttpCrackUrl( NULL, 0, 0, &uc );
    ok( !ret, "WinHttpCrackUrl failed\n" );

    /* decoding without buffers */
    reset_url_components( &uc );
    SetLastError(0xdeadbeef);
    ret = WinHttpCrackUrl( url8, 0, ICU_DECODE, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl failed\n" );
    ok( error == ERROR_INVALID_PARAMETER, "WinHttpCrackUrl failed\n" );

    /* decoding with buffers */
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszUserName = user;
    uc.dwUserNameLength = 20;
    uc.lpszPassword = pass;
    uc.dwPasswordLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.nPort = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 20;
    path[0] = 0;

    ret = WinHttpCrackUrl( url8, 0, ICU_DECODE, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( !memcmp( uc.lpszUrlPath + 11, escape, 21 * sizeof(WCHAR) ), "unexpected path\n" );
    ok( uc.dwUrlPathLength == 32, "unexpected path length\n" );
    ok( !memcmp( uc.lpszExtraInfo, escape + 21, 12 * sizeof(WCHAR) ), "unexpected extra info\n" );
    ok( uc.dwExtraInfoLength == 12, "unexpected extra info length\n" );

    /* Urls with specified port numbers */
    /* decoding with buffers */
    uc.lpszScheme = scheme;
    uc.dwSchemeLength = 20;
    uc.lpszUserName = user;
    uc.dwUserNameLength = 20;
    uc.lpszPassword = pass;
    uc.dwPasswordLength = 20;
    uc.lpszHostName = host;
    uc.dwHostNameLength = 20;
    uc.nPort = 0;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 40;
    uc.lpszExtraInfo = extra;
    uc.dwExtraInfoLength = 20;
    path[0] = 0;

    ret = WinHttpCrackUrl( url7, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( !memcmp( uc.lpszHostName, winehq, sizeof(winehq) ), "unexpected host name: %s\n", debugstr_w(uc.lpszHostName) );
    ok( uc.dwHostNameLength == 14, "unexpected host name length: %d\n", uc.dwHostNameLength );
    ok( uc.nPort == 42, "unexpected port: %u\n", uc.nPort );

    /* decoding without buffers */
    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url9, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nPort == 0, "unexpected port: %u\n", uc.nPort );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url10, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nPort == 80, "unexpected port: %u\n", uc.nPort );

    reset_url_components( &uc );
    ret = WinHttpCrackUrl( url11, 0, 0, &uc );
    ok( ret, "WinHttpCrackUrl failed\n" );
    ok( uc.nPort == 443, "unexpected port: %u\n", uc.nPort );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( empty, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u, expected ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n", error );

    reset_url_components( &uc );
    SetLastError( 0xdeadbeef );
    ret = WinHttpCrackUrl( http, 0, 0, &uc );
    error = GetLastError();
    ok( !ret, "WinHttpCrackUrl succeeded\n" );
    ok( error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME, "got %u, expected ERROR_WINHTTP_UNRECOGNIZED_SCHEME\n", error );
}

START_TEST (url)
{
    WinHttpCreateUrl_test();
    WinHttpCrackUrl_test();
}
