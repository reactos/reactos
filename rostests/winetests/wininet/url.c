/*
 * Wininet - URL tests
 *
 * Copyright 2002 Aric Stewart
 * Copyright 2004 Mike McCormack
 * Copyright 2005 Hans Leidekker
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
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/test.h"

#define TEST_URL "http://www.winehq.org/site/about"
#define TEST_URL_HOST "www.winehq.org"
#define TEST_URL_PATH "/site/about"
#define TEST_URL2 "http://www.myserver.com/myscript.php?arg=1"
#define TEST_URL2_SERVER "www.myserver.com"
#define TEST_URL2_PATH "/myscript.php"
#define TEST_URL2_PATHEXTRA "/myscript.php?arg=1"
#define TEST_URL2_EXTRA "?arg=1"
#define TEST_URL3 "file:///C:/Program%20Files/Atmel/AVR%20Tools/STK500/STK500.xml"

#define CREATE_URL1 "http://username:password@www.winehq.org/site/about"
#define CREATE_URL2 "http://username@www.winehq.org/site/about"
#define CREATE_URL3 "http://username:"
#define CREATE_URL4 "http://www.winehq.org/site/about"
#define CREATE_URL5 "http://"
#define CREATE_URL6 "nhttp://username:password@www.winehq.org:80/site/about"
#define CREATE_URL7 "http://username:password@www.winehq.org:42/site/about"
#define CREATE_URL8 "https://username:password@www.winehq.org/site/about"
#define CREATE_URL9 "about:blank"
#define CREATE_URL10 "about://host/blank"
#define CREATE_URL11 "about:"
#define CREATE_URL12 "http://www.winehq.org:65535"
#define CREATE_URL13 "http://localhost/?test=123"

static void copy_compsA(
    URL_COMPONENTSA *src, 
    URL_COMPONENTSA *dst, 
    DWORD scheLen,
    DWORD hostLen,
    DWORD userLen,
    DWORD passLen,
    DWORD pathLen,
    DWORD extrLen )
{
    *dst = *src;
    dst->dwSchemeLength    = scheLen;
    dst->dwHostNameLength  = hostLen;
    dst->dwUserNameLength  = userLen;
    dst->dwPasswordLength  = passLen;
    dst->dwUrlPathLength   = pathLen;
    dst->dwExtraInfoLength = extrLen;
    SetLastError(0xfaceabad);
}

static void zero_compsA(
    URL_COMPONENTSA *dst, 
    DWORD scheLen,
    DWORD hostLen,
    DWORD userLen,
    DWORD passLen,
    DWORD pathLen,
    DWORD extrLen )
{
    ZeroMemory(dst, sizeof(URL_COMPONENTSA));
    dst->dwStructSize = sizeof(URL_COMPONENTSA);
    dst->dwSchemeLength    = scheLen;
    dst->dwHostNameLength  = hostLen;
    dst->dwUserNameLength  = userLen;
    dst->dwPasswordLength  = passLen;
    dst->dwUrlPathLength   = pathLen;
    dst->dwExtraInfoLength = extrLen;
    SetLastError(0xfaceabad);
}

static void InternetCrackUrl_test(void)
{
  URL_COMPONENTSA urlSrc, urlComponents;
  char protocol[32], hostName[1024], userName[1024];
  char password[1024], extra[1024], path[1024];
  BOOL ret, firstret;
  DWORD GLE, firstGLE;

  ZeroMemory(&urlSrc, sizeof(urlSrc));
  urlSrc.dwStructSize = sizeof(urlSrc);
  urlSrc.lpszScheme = protocol;
  urlSrc.lpszHostName = hostName;
  urlSrc.lpszUserName = userName;
  urlSrc.lpszPassword = password;
  urlSrc.lpszUrlPath = path;
  urlSrc.lpszExtraInfo = extra;

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrl(TEST_URL, 0,0,&urlComponents);
  ok( ret, "InternetCrackUrl failed, error %d\n",GetLastError());
  ok((strcmp(TEST_URL_PATH,path) == 0),"path cracked wrong\n");

  /* Bug 1805: Confirm the returned lengths are correct:                     */
  /* 1. When extra info split out explicitly */
  zero_compsA(&urlComponents, 0, 1, 0, 0, 1, 1);
  ok(InternetCrackUrlA(TEST_URL2, 0, 0, &urlComponents),"InternetCrackUrl failed, error %d\n", GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL2_PATH),".dwUrlPathLength should be %d, but is %d\n", (DWORD)strlen(TEST_URL2_PATH), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL2_PATH,strlen(TEST_URL2_PATH)),"lpszUrlPath should be %s but is %s\n", TEST_URL2_PATH, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL2_SERVER),".dwHostNameLength should be %d, but is %d\n", (DWORD)strlen(TEST_URL2_SERVER), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL2_SERVER,strlen(TEST_URL2_SERVER)),"lpszHostName should be %s but is %s\n", TEST_URL2_SERVER, urlComponents.lpszHostName);
  ok(urlComponents.dwExtraInfoLength == strlen(TEST_URL2_EXTRA),".dwExtraInfoLength should be %d, but is %d\n", (DWORD)strlen(TEST_URL2_EXTRA), urlComponents.dwExtraInfoLength);
  ok(!strncmp(urlComponents.lpszExtraInfo,TEST_URL2_EXTRA,strlen(TEST_URL2_EXTRA)),"lpszExtraInfo should be %s but is %s\n", TEST_URL2_EXTRA, urlComponents.lpszHostName);

  /* 2. When extra info is not split out explicitly and is in url path */
  zero_compsA(&urlComponents, 0, 1, 0, 0, 1, 0);
  ok(InternetCrackUrlA(TEST_URL2, 0, 0, &urlComponents),"InternetCrackUrl failed with GLE %d\n",GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL2_PATHEXTRA),".dwUrlPathLength should be %d, but is %d\n", (DWORD)strlen(TEST_URL2_PATHEXTRA), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL2_PATHEXTRA,strlen(TEST_URL2_PATHEXTRA)),"lpszUrlPath should be %s but is %s\n", TEST_URL2_PATHEXTRA, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL2_SERVER),".dwHostNameLength should be %d, but is %d\n", (DWORD)strlen(TEST_URL2_SERVER), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL2_SERVER,strlen(TEST_URL2_SERVER)),"lpszHostName should be %s but is %s\n", TEST_URL2_SERVER, urlComponents.lpszHostName);
  ok(urlComponents.nPort == INTERNET_DEFAULT_HTTP_PORT,"urlComponents->nPort should have been 80 instead of %d\n", urlComponents.nPort);
  ok(urlComponents.nScheme == INTERNET_SCHEME_HTTP,"urlComponents->nScheme should have been INTERNET_SCHEME_HTTP instead of %d\n", urlComponents.nScheme);

  zero_compsA(&urlComponents, 1, 1, 1, 1, 1, 1);
  ok(InternetCrackUrlA(TEST_URL, strlen(TEST_URL), 0, &urlComponents),"InternetCrackUrl failed with GLE %d\n",GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL_PATH),".dwUrlPathLength should be %d, but is %d\n", lstrlenA(TEST_URL_PATH), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL_PATH,strlen(TEST_URL_PATH)),"lpszUrlPath should be %s but is %s\n", TEST_URL_PATH, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL_HOST),".dwHostNameLength should be %d, but is %d\n", lstrlenA(TEST_URL_HOST), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL_HOST,strlen(TEST_URL_HOST)),"lpszHostName should be %s but is %s\n", TEST_URL_HOST, urlComponents.lpszHostName);
  ok(urlComponents.nPort == INTERNET_DEFAULT_HTTP_PORT,"urlComponents->nPort should have been 80 instead of %d\n", urlComponents.nPort);
  ok(urlComponents.nScheme == INTERNET_SCHEME_HTTP,"urlComponents->nScheme should have been INTERNET_SCHEME_HTTP instead of %d\n", urlComponents.nScheme);
  ok(!urlComponents.lpszUserName, ".lpszUserName should have been set to NULL\n");
  ok(!urlComponents.lpszPassword, ".lpszPassword should have been set to NULL\n");
  ok(!urlComponents.lpszExtraInfo, ".lpszExtraInfo should have been set to NULL\n");
  ok(!urlComponents.dwUserNameLength,".dwUserNameLength should be 0, but is %d\n", urlComponents.dwUserNameLength);
  ok(!urlComponents.dwPasswordLength,".dwPasswordLength should be 0, but is %d\n", urlComponents.dwPasswordLength);
  ok(!urlComponents.dwExtraInfoLength,".dwExtraInfoLength should be 0, but is %d\n", urlComponents.dwExtraInfoLength);

  /*3. Check for %20 */
  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ok(InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents),"InternetCrackUrl failed with GLE %d\n",GetLastError());

  /* Tests for lpsz* members pointing to real strings while 
   * some corresponding length members are set to zero.
   * As of IE7 (wininet 7.0*?) all members are checked. So we
   * run the first test and expect the outcome to be the same
   * for the first four (scheme, hostname, username and password).
   * The last two (path and extrainfo) are the same for all versions
   * of the wininet.dll.
   */
  copy_compsA(&urlSrc, &urlComponents, 0, 1024, 1024, 1024, 2048, 1024);
  SetLastError(0xdeadbeef);
  firstret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  firstGLE = GetLastError();

  copy_compsA(&urlSrc, &urlComponents, 32, 0, 1024, 1024, 2048, 1024);
  SetLastError(0xdeadbeef);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  ok(ret==firstret && (GLE==firstGLE), "InternetCrackUrl returned %d with GLE=%d (expected to return %d)\n",
    ret, GetLastError(), firstret);

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 0, 1024, 2048, 1024);
  SetLastError(0xdeadbeef);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  ok(ret==firstret && (GLE==firstGLE), "InternetCrackUrl returned %d with GLE=%d (expected to return %d)\n",
    ret, GetLastError(), firstret);

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 0, 2048, 1024);
  SetLastError(0xdeadbeef);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  ok(ret==firstret && (GLE==firstGLE), "InternetCrackUrl returned %d with GLE=%d (expected to return %d)\n",
    ret, GetLastError(), firstret);

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 0, 1024);
  SetLastError(0xdeadbeef);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  todo_wine
  ok(ret==0 && (GLE==ERROR_INVALID_HANDLE || GLE==ERROR_INSUFFICIENT_BUFFER),
     "InternetCrackUrl returned %d with GLE=%d (expected to return 0 and ERROR_INVALID_HANDLE or ERROR_INSUFFICIENT_BUFFER)\n",
    ret, GLE);

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 0);
  SetLastError(0xdeadbeef);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  todo_wine
  ok(ret==0 && (GLE==ERROR_INVALID_HANDLE || GLE==ERROR_INSUFFICIENT_BUFFER),
     "InternetCrackUrl returned %d with GLE=%d (expected to return 0 and ERROR_INVALID_HANDLE or ERROR_INSUFFICIENT_BUFFER)\n",
    ret, GLE);

  copy_compsA(&urlSrc, &urlComponents, 0, 0, 0, 0, 0, 0);
  ret = InternetCrackUrlA(TEST_URL3, 0, ICU_DECODE, &urlComponents);
  GLE = GetLastError();
  todo_wine
  ok(ret==0 && GLE==ERROR_INVALID_PARAMETER,
     "InternetCrackUrl returned %d with GLE=%d (expected to return 0 and ERROR_INVALID_PARAMETER)\n",
    ret, GLE);

  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrl("about://host/blank", 0,0,&urlComponents);
  ok(ret, "InternetCrackUrl failed with %d\n", GetLastError());
  ok(!strcmp(urlComponents.lpszScheme, "about"), "lpszScheme was \"%s\" instead of \"about\"\n", urlComponents.lpszScheme);
  ok(!strcmp(urlComponents.lpszHostName, "host"), "lpszHostName was \"%s\" instead of \"host\"\n", urlComponents.lpszHostName);
  ok(!strcmp(urlComponents.lpszUrlPath, "/blank"), "lpszUrlPath was \"%s\" instead of \"/blank\"\n", urlComponents.lpszUrlPath);

  /* try a NULL lpszUrl */
  SetLastError(0xdeadbeef);
  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrl(NULL, 0, 0, &urlComponents);
  GLE = GetLastError();
  ok(ret == FALSE, "Expected InternetCrackUrl to fail\n");
  ok(GLE == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GLE);

  /* try an empty lpszUrl, GetLastError returns 12006, whatever that means
   * we just need to fail and not return success
   */
  SetLastError(0xdeadbeef);
  copy_compsA(&urlSrc, &urlComponents, 32, 1024, 1024, 1024, 2048, 1024);
  ret = InternetCrackUrl("", 0, 0, &urlComponents);
  GLE = GetLastError();
  ok(ret == FALSE, "Expected InternetCrackUrl to fail\n");
  ok(GLE != 0xdeadbeef && GLE != ERROR_SUCCESS, "Expected GLE to represent a failure\n");

  /* Invalid Call: must set size of components structure (Windows only
   * enforces this on the InternetCrackUrlA version of the call) */
  copy_compsA(&urlSrc, &urlComponents, 0, 1024, 1024, 1024, 2048, 1024);
  SetLastError(0xdeadbeef);
  urlComponents.dwStructSize = 0;
  ret = InternetCrackUrlA(TEST_URL, 0, 0, &urlComponents);
  ok(ret == FALSE, "Expected InternetCrackUrl to fail\n");
  ok(GLE != 0xdeadbeef && GLE != ERROR_SUCCESS, "Expected GLE to represent a failure\n");

  /* Invalid Call: size of dwStructSize must be one of the "standard" sizes
   * of the URL_COMPONENTS structure (Windows only enforces this on the
   * InternetCrackUrlA version of the call) */
  copy_compsA(&urlSrc, &urlComponents, 0, 1024, 1024, 1024, 2048, 1024);
  SetLastError(0xdeadbeef);
  urlComponents.dwStructSize = sizeof(urlComponents) + 1;
  ret = InternetCrackUrlA(TEST_URL, 0, 0, &urlComponents);
  ok(ret == FALSE, "Expected InternetCrackUrl to fail\n");
  ok(GLE != 0xdeadbeef && GLE != ERROR_SUCCESS, "Expected GLE to represent a failure\n");
}

static void InternetCrackUrlW_test(void)
{
    WCHAR url[] = {
        'h','t','t','p',':','/','/','1','9','2','.','1','6','8','.','0','.','2','2','/',
        'C','F','I','D','E','/','m','a','i','n','.','c','f','m','?','C','F','S','V','R',
        '=','I','D','E','&','A','C','T','I','O','N','=','I','D','E','_','D','E','F','A',
        'U','L','T', 0 };
    static const WCHAR url2[] = { '.','.','/','R','i','t','z','.','x','m','l',0 };
    static const WCHAR url3[] = { 'h','t','t','p',':','/','/','x','.','o','r','g',0 };
    URL_COMPONENTSW comp;
    WCHAR scheme[20], host[20], user[20], pwd[20], urlpart[50], extra[50];
    DWORD error;
    BOOL r;

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszScheme = scheme;
    comp.dwSchemeLength = sizeof scheme;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUserName = user;
    comp.dwUserNameLength = sizeof user;
    comp.lpszPassword = pwd;
    comp.dwPasswordLength = sizeof pwd;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = extra;
    comp.dwExtraInfoLength = sizeof extra;

    SetLastError(0xdeadbeef);
    r = InternetCrackUrlW(NULL, 0, 0, &comp );
    error = GetLastError();
    ok( !r, "InternetCrackUrlW succeeded unexpectedly\n");
    ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error);

    SetLastError(0xdeadbeef);
    r = InternetCrackUrlW(url, 0, 0, NULL );
    error = GetLastError();
    ok( !r, "InternetCrackUrlW succeeded unexpectedly\n");
    ok( error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error);

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 4, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 15, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 29, "extra length wrong\n");
 
    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 0, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 44, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 0, "extra length wrong\n");

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = NULL;
    comp.dwExtraInfoLength = sizeof extra;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 0, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 15, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 29, "extra length wrong\n");

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof(comp));
    comp.dwStructSize = sizeof(comp);
    comp.lpszScheme = scheme;
    comp.dwSchemeLength = sizeof(scheme)/sizeof(scheme[0]);
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof(host)/sizeof(host[0]);
    comp.lpszUserName = user;
    comp.dwUserNameLength = sizeof(user)/sizeof(user[0]);
    comp.lpszPassword = pwd;
    comp.dwPasswordLength = sizeof(pwd)/sizeof(pwd[0]);
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof(urlpart)/sizeof(urlpart[0]);
    comp.lpszExtraInfo = extra;
    comp.dwExtraInfoLength = sizeof(extra)/sizeof(extra[0]);

    r = InternetCrackUrlW(url2, 0, 0, &comp);
    todo_wine {
    ok(!r, "InternetCrackUrl should have failed\n");
    ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME,
        "InternetCrackUrl should have failed with error ERROR_INTERNET_UNRECOGNIZED_SCHEME instead of error %d\n",
        GetLastError());
    }

    /* Test to see whether cracking a URL without a filename initializes urlpart */
    urlpart[0]=0xba;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszScheme = scheme;
    comp.dwSchemeLength = sizeof scheme;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUserName = user;
    comp.dwUserNameLength = sizeof user;
    comp.lpszPassword = pwd;
    comp.dwPasswordLength = sizeof pwd;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = extra;
    comp.dwExtraInfoLength = sizeof extra;
    r = InternetCrackUrlW(url3, 0, 0, &comp );
    ok( r, "InternetCrackUrlW failed unexpectedly\n");
    ok( host[0] == 'x', "host should be x.org\n");
    ok( urlpart[0] == 0, "urlpart should be empty\n");
}

static void fill_url_components(LPURL_COMPONENTS lpUrlComponents)
{
    static CHAR http[]       = "http",
                winehq[]     = "www.winehq.org",
                username[]   = "username",
                password[]   = "password",
                site_about[] = "/site/about",
                empty[]      = "";

    lpUrlComponents->dwStructSize = sizeof(URL_COMPONENTS);
    lpUrlComponents->lpszScheme = http;
    lpUrlComponents->dwSchemeLength = strlen(lpUrlComponents->lpszScheme);
    lpUrlComponents->nScheme = INTERNET_SCHEME_HTTP;
    lpUrlComponents->lpszHostName = winehq;
    lpUrlComponents->dwHostNameLength = strlen(lpUrlComponents->lpszHostName);
    lpUrlComponents->nPort = 80;
    lpUrlComponents->lpszUserName = username;
    lpUrlComponents->dwUserNameLength = strlen(lpUrlComponents->lpszUserName);
    lpUrlComponents->lpszPassword = password;
    lpUrlComponents->dwPasswordLength = strlen(lpUrlComponents->lpszPassword);
    lpUrlComponents->lpszUrlPath = site_about;
    lpUrlComponents->dwUrlPathLength = strlen(lpUrlComponents->lpszUrlPath);
    lpUrlComponents->lpszExtraInfo = empty;
    lpUrlComponents->dwExtraInfoLength = strlen(lpUrlComponents->lpszExtraInfo);
}

static void InternetCreateUrlA_test(void)
{
	URL_COMPONENTS urlComp;
	LPSTR szUrl;
	DWORD len = -1;
	BOOL ret;
        static CHAR empty[]      = "",
                    nhttp[]      = "nhttp",
                    http[]       = "http",
                    https[]      = "https",
                    winehq[]     = "www.winehq.org",
                    localhost[]  = "localhost",
                    username[]   = "username",
                    password[]   = "password",
                    root[]       = "/",
                    site_about[] = "/site/about",
                    extra_info[] = "?test=123",
                    about[]      = "about",
                    blank[]      = "blank",
                    host[]       = "host";

	/* test NULL lpUrlComponents */
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(NULL, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
	ok(len == -1, "Expected len -1, got %d\n", len);

	/* test zero'ed lpUrlComponents */
	ZeroMemory(&urlComp, sizeof(URL_COMPONENTS));
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
	ok(len == -1, "Expected len -1, got %d\n", len);

	/* test valid lpUrlComponents, NULL lpdwUrlLength */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, NULL);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
	ok(len == -1, "Expected len -1, got %d\n", len);

	/* test valid lpUrlComponents, empty szUrl
	 * lpdwUrlLength is size of buffer required on exit, including
	 * the terminating null when GLE == ERROR_INSUFFICIENT_BUFFER
	 */
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		"Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
	ok(len == 51, "Expected len 51, got %d\n", len);

	/* test correct size, NULL szUrl */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	ret = InternetCreateUrlA(&urlComp, 0, NULL, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		"Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
	ok(len == 51, "Expected len 51, got %d\n", len);

	/* test valid lpUrlComponents, alloc-ed szUrl, small size */
	SetLastError(0xdeadbeef);
	szUrl = HeapAlloc(GetProcessHeap(), 0, len);
	len -= 2;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
		"Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
	ok(len == 51, "Expected len 51, got %d\n", len);

	/* alloc-ed szUrl, NULL lpszScheme
	 * shows that it uses nScheme instead
	 */
	SetLastError(0xdeadbeef);
	urlComp.lpszScheme = NULL;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 50, "Expected len 50, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL1), "Expected %s, got %s\n", CREATE_URL1, szUrl);

	/* alloc-ed szUrl, invalid nScheme
	 * any nScheme out of range seems ignored
	 */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.nScheme = -3;
	len++;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 50, "Expected len 50, got %d\n", len);

	/* test valid lpUrlComponents, alloc-ed szUrl */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 50, "Expected len 50, got %d\n", len);
	ok(strstr(szUrl, "80") == NULL, "Didn't expect to find 80 in szUrl\n");
	ok(!strcmp(szUrl, CREATE_URL1), "Expected %s, got %s\n", CREATE_URL1, szUrl);

	/* valid username, NULL password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszPassword = NULL;
	len = 42;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 41, "Expected len 41, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL2), "Expected %s, got %s\n", CREATE_URL2, szUrl);

	/* valid username, empty password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszPassword = empty;
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 50, "Expected len 50, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL3), "Expected %s, got %s\n", CREATE_URL3, szUrl);

	/* valid password, NULL username
	 * if password is provided, username has to exist
	 */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = NULL;
	len = 42;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(!ret, "Expected failure\n");
	ok(GetLastError() == ERROR_INVALID_PARAMETER,
		"Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
	ok(len == 42, "Expected len 42, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL3), "Expected %s, got %s\n", CREATE_URL3, szUrl);

	/* valid password, empty username
	 * if password is provided, username has to exist
	 */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = empty;
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 50, "Expected len 50, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL5), "Expected %s, got %s\n", CREATE_URL5, szUrl);

	/* NULL username, NULL password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = NULL;
	urlComp.lpszPassword = NULL;
	len = 42;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 32, "Expected len 32, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL4), "Expected %s, got %s\n", CREATE_URL4, szUrl);

	/* empty username, empty password */
	fill_url_components(&urlComp);
	SetLastError(0xdeadbeef);
	urlComp.lpszUserName = empty;
	urlComp.lpszPassword = empty;
	len = 51;
	ret = InternetCreateUrlA(&urlComp, 0, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(GetLastError() == 0xdeadbeef,
		"Expected 0xdeadbeef, got %d\n", GetLastError());
	ok(len == 50, "Expected len 50, got %d\n", len);
	ok(!strcmp(szUrl, CREATE_URL5), "Expected %s, got %s\n", CREATE_URL5, szUrl);

	/* shows that nScheme is ignored, as the appearance of the port number
	 * depends on lpszScheme and the string copied depends on lpszScheme.
	 */
	fill_url_components(&urlComp);
	HeapFree(GetProcessHeap(), 0, szUrl);
	urlComp.lpszScheme = nhttp;
	urlComp.dwSchemeLength = strlen(urlComp.lpszScheme);
	len = strlen(CREATE_URL6) + 1;
	szUrl = HeapAlloc(GetProcessHeap(), 0, len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL6), "Expected len %d, got %d\n", lstrlenA(CREATE_URL6) + 1, len);
	ok(!strcmp(szUrl, CREATE_URL6), "Expected %s, got %s\n", CREATE_URL6, szUrl);

	/* if lpszScheme != "http" or nPort != 80, display nPort */
	HeapFree(GetProcessHeap(), 0, szUrl);
        urlComp.lpszScheme = http;
	urlComp.dwSchemeLength = strlen(urlComp.lpszScheme);
	urlComp.nPort = 42;
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == 53, "Expected len 53, got %d\n", len);
	ok(strstr(szUrl, "42") != NULL, "Expected to find 42 in szUrl\n");
	ok(!strcmp(szUrl, CREATE_URL7), "Expected %s, got %s\n", CREATE_URL7, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = http;
	urlComp.dwSchemeLength = 0;
	urlComp.nScheme = INTERNET_SCHEME_HTTP;
	urlComp.lpszHostName = winehq;
	urlComp.dwHostNameLength = 0;
	urlComp.nPort = 80;
	urlComp.lpszUserName = username;
	urlComp.dwUserNameLength = 0;
	urlComp.lpszPassword = password;
	urlComp.dwPasswordLength = 0;
	urlComp.lpszUrlPath = site_about;
	urlComp.dwUrlPathLength = 0;
	urlComp.lpszExtraInfo = empty;
	urlComp.dwExtraInfoLength = 0;
	len = strlen(CREATE_URL1);
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL1), "Expected len %d, got %d\n", lstrlenA(CREATE_URL1), len);
	ok(!strcmp(szUrl, CREATE_URL1), "Expected %s, got %s\n", CREATE_URL1, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = https;
	urlComp.dwSchemeLength = 0;
	urlComp.nScheme = INTERNET_SCHEME_HTTP;
	urlComp.lpszHostName = winehq;
	urlComp.dwHostNameLength = 0;
	urlComp.nPort = 443;
	urlComp.lpszUserName = username;
	urlComp.dwUserNameLength = 0;
	urlComp.lpszPassword = password;
	urlComp.dwPasswordLength = 0;
	urlComp.lpszUrlPath = site_about;
	urlComp.dwUrlPathLength = 0;
	urlComp.lpszExtraInfo = empty;
	urlComp.dwExtraInfoLength = 0;
	len = strlen(CREATE_URL8);
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL8), "Expected len %d, got %d\n", lstrlenA(CREATE_URL8), len);
	ok(!strcmp(szUrl, CREATE_URL8), "Expected %s, got %s\n", CREATE_URL8, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = about;
	urlComp.dwSchemeLength = 5;
	urlComp.lpszUrlPath = blank;
	urlComp.dwUrlPathLength = 5;
	len = strlen(CREATE_URL9);
	len++; /* work around bug in native wininet */
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL9), "Expected len %d, got %d\n", lstrlenA(CREATE_URL9), len);
	ok(!strcmp(szUrl, CREATE_URL9), "Expected %s, got %s\n", CREATE_URL9, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = about;
	urlComp.lpszHostName = host;
	urlComp.lpszUrlPath = blank;
	len = strlen(CREATE_URL10);
	len++; /* work around bug in native wininet */
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL10), "Expected len %d, got %d\n", lstrlenA(CREATE_URL10), len);
	ok(!strcmp(szUrl, CREATE_URL10), "Expected %s, got %s\n", CREATE_URL10, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.nPort = 8080;
	urlComp.lpszScheme = about;
	len = strlen(CREATE_URL11);
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL11), "Expected len %d, got %d\n", lstrlenA(CREATE_URL11), len);
	ok(!strcmp(szUrl, CREATE_URL11), "Expected %s, got %s\n", CREATE_URL11, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

	memset(&urlComp, 0, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(URL_COMPONENTS);
	urlComp.lpszScheme = http;
	urlComp.dwSchemeLength = 0;
	urlComp.nScheme = INTERNET_SCHEME_HTTP;
	urlComp.lpszHostName = winehq;
	urlComp.dwHostNameLength = 0;
	urlComp.nPort = 65535;
	len = strlen(CREATE_URL12);
	szUrl = HeapAlloc(GetProcessHeap(), 0, ++len);
	ret = InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
	ok(ret, "Expected success\n");
	ok(len == strlen(CREATE_URL12), "Expected len %d, got %d\n", lstrlenA(CREATE_URL12), len);
	ok(!strcmp(szUrl, CREATE_URL12), "Expected %s, got %s\n", CREATE_URL12, szUrl);

	HeapFree(GetProcessHeap(), 0, szUrl);

    memset(&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.lpszScheme = http;
    urlComp.dwSchemeLength = strlen(urlComp.lpszScheme);
    urlComp.lpszHostName = localhost;
    urlComp.dwHostNameLength = strlen(urlComp.lpszHostName);
    urlComp.nPort = 80;
    urlComp.lpszUrlPath = root;
    urlComp.dwUrlPathLength = strlen(urlComp.lpszUrlPath);
    urlComp.lpszExtraInfo = extra_info;
    urlComp.dwExtraInfoLength = strlen(urlComp.lpszExtraInfo);
    len = 256;
    szUrl = HeapAlloc(GetProcessHeap(), 0, len);
    InternetCreateUrlA(&urlComp, ICU_ESCAPE, szUrl, &len);
    ok(ret, "Expected success\n");
    ok(len == strlen(CREATE_URL13), "Got len %u\n", len);
    ok(!strcmp(szUrl, CREATE_URL13), "Expected \"%s\", got \"%s\"\n", CREATE_URL13, szUrl);

    HeapFree(GetProcessHeap(), 0, szUrl);
}

START_TEST(url)
{
    InternetCrackUrl_test();
    InternetCrackUrlW_test();
    InternetCreateUrlA_test();
}
