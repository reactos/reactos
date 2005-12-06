/* Unit test suite for Path functions
 *
 * Copyright 2002 Matthew Mastracci
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

static HMODULE hShlwapi;
static HRESULT (WINAPI *pPathIsValidCharA)(char,DWORD);
static HRESULT (WINAPI *pPathIsValidCharW)(WCHAR,DWORD);

const char* TEST_URL_1 = "http://www.winehq.org/tests?date=10/10/1923";
const char* TEST_URL_2 = "http://localhost:8080/tests%2e.html?date=Mon%2010/10/1923";
const char* TEST_URL_3 = "http://foo:bar@localhost:21/internal.php?query=x&return=y";

typedef struct _TEST_URL_CANONICALIZE {
    const char *url;
    DWORD flags;
    HRESULT expectret;
    const char *expecturl;
} TEST_URL_CANONICALIZE;

const TEST_URL_CANONICALIZE TEST_CANONICALIZE[] = {
    /*FIXME {"http://www.winehq.org/tests/../tests/../..", 0, S_OK, "http://www.winehq.org/"},*/
    {"http://www.winehq.org/tests/../tests", 0, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\n", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\r", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\r", 0, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests\r", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests"},
    {"http://www.winehq.org/tests/../tests/", 0, S_OK, "http://www.winehq.org/tests/"},
    {"http://www.winehq.org/tests/../tests/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/../", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/..?query=x&return=y", 0, S_OK, "http://www.winehq.org/?query=x&return=y"},
    {"http://www.winehq.org/tests/../?query=x&return=y", 0, S_OK, "http://www.winehq.org/?query=x&return=y"},
    {"http://www.winehq.org/tests/..#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org/tests/../#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org/tests/../#example", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../#example"},
    {"http://www.winehq.org/tests/foo bar", URL_ESCAPE_SPACES_ONLY| URL_DONT_ESCAPE_EXTRA_INFO , S_OK, "http://www.winehq.org/tests/foo%20bar"},
    {"http://www.winehq.org/tests/foo%20bar", URL_UNESCAPE , S_OK, "http://www.winehq.org/tests/foo bar"},
    {"file:///c:/tests/foo%20bar", URL_UNESCAPE , S_OK, "file:///c:/tests/foo bar"},
};

typedef struct _TEST_URL_ESCAPE {
    const char *url;
    DWORD flags;
    DWORD expectescaped;
    HRESULT expectret;
    const char *expecturl;
} TEST_URL_ESCAPE;

const TEST_URL_ESCAPE TEST_ESCAPE[] = {
    {"http://www.winehq.org/tests0", 0, 0, S_OK, "http://www.winehq.org/tests0"},
    {"http://www.winehq.org/tests1\n", 0, 0, S_OK, "http://www.winehq.org/tests1%0A"},
    {"http://www.winehq.org/tests2\r", 0, 0, S_OK, "http://www.winehq.org/tests2%0D"},
    {"http://www.winehq.org/tests3\r", URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, 0, S_OK, "http://www.winehq.org/tests3\r"},
    {"http://www.winehq.org/tests4\r", URL_ESCAPE_SPACES_ONLY, 0, S_OK, "http://www.winehq.org/tests4\r"},
    {"http://www.winehq.org/tests5\r", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY, 0, S_OK, "http://www.winehq.org/tests5\r"},
    {"/direct/swhelp/series6/6.2i_latestservicepack.dat\r", URL_ESCAPE_SPACES_ONLY, 0, S_OK, "/direct/swhelp/series6/6.2i_latestservicepack.dat\r"},

    {"file://////foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file://///foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:////foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:///localhost/foo/bar\\baz", 0, 0, S_OK, "file:///localhost/foo/bar/baz"},
    {"file:///foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file://loCalHost/foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file://foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:/localhost/foo/bar\\baz", 0, 0, S_OK, "file:///localhost/foo/bar/baz"},
    {"file:/foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:foo/bar\\baz", 0, 0, S_OK, "file:foo/bar/baz"},
    {"file:\\foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:\\\\foo/bar\\baz", 0, 0, S_OK, "file://foo/bar/baz"},
    {"file:\\\\\\foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:\\\\localhost\\foo/bar\\baz", 0, 0, S_OK, "file:///foo/bar/baz"},
    {"file:///f oo/b?a r\\baz", 0, 0, S_OK, "file:///f%20oo/b?a r\\baz"},
    {"file:///foo/b#a r\\baz", 0, 0, S_OK, "file:///foo/b%23a%20r/baz"},
    {"file:///f o^&`{}|][\"<>\\%o/b#a r\\baz", 0, 0, S_OK, "file:///f%20o%5E%26%60%7B%7D%7C%5D%5B%22%3C%3E/%o/b%23a%20r/baz"},
    {"file:///f o%o/b?a r\\b%az", URL_ESCAPE_PERCENT, 0, S_OK, "file:///f%20o%25o/b?a r\\b%az"},
    {"file:/foo/bar\\baz", URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "file:%2Ffoo%2Fbar%5Cbaz"},

    {"foo/b%ar\\ba?z\\", URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "foo%2Fb%ar%5Cba%3Fz%5C"},
    {"foo/b%ar\\ba?z\\", URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "foo%2Fb%25ar%5Cba%3Fz%5C"},
    {"foo/bar\\ba?z\\", 0, 0, S_OK, "foo/bar%5Cba?z\\"},
    {"/foo/bar\\ba?z\\", 0, 0, S_OK, "/foo/bar%5Cba?z\\"},
    {"/foo/bar\\ba#z\\", 0, 0, S_OK, "/foo/bar%5Cba#z\\"},
    {"/foo/%5C", 0, 0, S_OK, "/foo/%5C"},
    {"/foo/%5C", URL_ESCAPE_PERCENT, 0, S_OK, "/foo/%255C"},

    {"http://////foo/bar\\baz", 0, 0, S_OK, "http://////foo/bar/baz"},
    {"http://///foo/bar\\baz", 0, 0, S_OK, "http://///foo/bar/baz"},
    {"http:////foo/bar\\baz", 0, 0, S_OK, "http:////foo/bar/baz"},
    {"http:///foo/bar\\baz", 0, 0, S_OK, "http:///foo/bar/baz"},
    {"http://localhost/foo/bar\\baz", 0, 0, S_OK, "http://localhost/foo/bar/baz"},
    {"http://foo/bar\\baz", 0, 0, S_OK, "http://foo/bar/baz"},
    {"http:/foo/bar\\baz", 0, 0, S_OK, "http:/foo/bar/baz"},
    {"http:foo/bar\\ba?z\\", 0, 0, S_OK, "http:foo%2Fbar%2Fba?z\\"},
    {"http:foo/bar\\ba#z\\", 0, 0, S_OK, "http:foo%2Fbar%2Fba#z\\"},
    {"http:\\foo/bar\\baz", 0, 0, S_OK, "http:/foo/bar/baz"},
    {"http:\\\\foo/bar\\baz", 0, 0, S_OK, "http://foo/bar/baz"},
    {"http:\\\\\\foo/bar\\baz", 0, 0, S_OK, "http:///foo/bar/baz"},
    {"http:\\\\\\\\foo/bar\\baz", 0, 0, S_OK, "http:////foo/bar/baz"},
    {"http:/fo ?o/b ar\\baz", 0, 0, S_OK, "http:/fo%20?o/b ar\\baz"},
    {"http:fo ?o/b ar\\baz", 0, 0, S_OK, "http:fo%20?o/b ar\\baz"},
    {"http:/foo/bar\\baz", URL_ESCAPE_SEGMENT_ONLY, 0, S_OK, "http:%2Ffoo%2Fbar%5Cbaz"},

    {"https://foo/bar\\baz", 0, 0, S_OK, "https://foo/bar/baz"},
    {"https:/foo/bar\\baz", 0, 0, S_OK, "https:/foo/bar/baz"},
    {"https:\\foo/bar\\baz", 0, 0, S_OK, "https:/foo/bar/baz"},

    {"foo:////foo/bar\\baz", 0, 0, S_OK, "foo:////foo/bar%5Cbaz"},
    {"foo:///foo/bar\\baz", 0, 0, S_OK, "foo:///foo/bar%5Cbaz"},
    {"foo://localhost/foo/bar\\baz", 0, 0, S_OK, "foo://localhost/foo/bar%5Cbaz"},
    {"foo://foo/bar\\baz", 0, 0, S_OK, "foo://foo/bar%5Cbaz"},
    {"foo:/foo/bar\\baz", 0, 0, S_OK, "foo:/foo/bar%5Cbaz"},
    {"foo:foo/bar\\baz", 0, 0, S_OK, "foo:foo%2Fbar%5Cbaz"},
    {"foo:\\foo/bar\\baz", 0, 0, S_OK, "foo:%5Cfoo%2Fbar%5Cbaz"},
    {"foo:/foo/bar\\ba?\\z", 0, 0, S_OK, "foo:/foo/bar%5Cba?\\z"},
    {"foo:/foo/bar\\ba#\\z", 0, 0, S_OK, "foo:/foo/bar%5Cba#\\z"},

    {"mailto:/fo/o@b\\%a?\\r.b#\\az", 0, 0, S_OK, "mailto:%2Ffo%2Fo@b%5C%a%3F%5Cr.b%23%5Caz"},
    {"mailto:fo/o@b\\%a?\\r.b#\\az", 0, 0, S_OK, "mailto:fo%2Fo@b%5C%a%3F%5Cr.b%23%5Caz"},
    {"mailto:fo/o@b\\%a?\\r.b#\\az", URL_ESCAPE_PERCENT, 0, S_OK, "mailto:fo%2Fo@b%5C%25a%3F%5Cr.b%23%5Caz"},

    {"ftp:fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:fo%2Fo@bar.baz%2Ffoo%2Fbar"},
    {"ftp:/fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:/fo/o@bar.baz/foo/bar"},
    {"ftp://fo/o@bar.baz/fo?o\\bar", 0, 0, S_OK, "ftp://fo/o@bar.baz/fo?o\\bar"},
    {"ftp://fo/o@bar.baz/fo#o\\bar", 0, 0, S_OK, "ftp://fo/o@bar.baz/fo#o\\bar"},
    {"ftp://localhost/o@bar.baz/fo#o\\bar", 0, 0, S_OK, "ftp://localhost/o@bar.baz/fo#o\\bar"},
    {"ftp:///fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:///fo/o@bar.baz/foo/bar"},
    {"ftp:////fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:////fo/o@bar.baz/foo/bar"}
};

typedef struct _TEST_URL_COMBINE {
    const char *url1;
    const char *url2;
    DWORD flags;
    HRESULT expectret;
    const char *expecturl;
} TEST_URL_COMBINE;

const TEST_URL_COMBINE TEST_COMBINE[] = {
    {"http://www.winehq.org/tests", "tests1", 0, S_OK, "http://www.winehq.org/tests1"},
    /*FIXME {"http://www.winehq.org/tests", "../tests2", 0, S_OK, "http://www.winehq.org/tests2"},*/
    {"http://www.winehq.org/tests/", "../tests3", 0, S_OK, "http://www.winehq.org/tests3"},
    {"http://www.winehq.org/tests/../tests", "tests4", 0, S_OK, "http://www.winehq.org/tests4"},
    {"http://www.winehq.org/tests/../tests/", "tests5", 0, S_OK, "http://www.winehq.org/tests/tests5"},
    {"http://www.winehq.org/tests/../tests/", "/tests6/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/..", "tests7/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/?query=x&return=y", "tests8", 0, S_OK, "http://www.winehq.org/tests/tests8"},
    {"http://www.winehq.org/tests/#example", "tests9", 0, S_OK, "http://www.winehq.org/tests/tests9"},
    {"http://www.winehq.org/tests/../tests/", "/tests10/..", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests10/.."},
    {"http://www.winehq.org/tests/../", "tests11", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../tests11"},
};

struct {
    const char *path;
    const char *url;
    DWORD ret;
} TEST_URLFROMPATH [] = {
    {"foo", "file:foo", S_OK},
    {"foo\\bar", "file:foo/bar", S_OK},
    {"\\foo\\bar", "file:///foo/bar", S_OK},
    {"c:\\foo\\bar", "file:///c:/foo/bar", S_OK},
    {"c:foo\\bar", "file:///c:foo/bar", S_OK},
    {"c:\\foo/b a%r", "file:///c:/foo/b%20a%25r", S_OK},
    {"c:\\foo\\foo bar", "file:///c:/foo/foo%20bar", S_OK},
#if 0
    /* The following test fails on native shlwapi as distributed with Win95/98.
     * Wine matches the behaviour of later versions.
     */
    {"xx:c:\\foo\\bar", "xx:c:\\foo\\bar", S_FALSE}
#endif
};

struct {
    const char *url;
    const char *path;
    DWORD ret;
} TEST_PATHFROMURL[] = {
    {"file:///c:/foo/ba%5Cr", "c:\\foo\\ba\\r", S_OK},
    {"file:///c:/foo/../ba%5Cr", "c:\\foo\\..\\ba\\r", S_OK},
    {"file:///host/c:/foo/bar", "\\host\\c:\\foo\\bar", S_OK},
    {"file://host/c:/foo/bar", "\\\\hostc:\\foo\\bar", S_OK},
    {"file://host/c:/foo/bar", "\\\\hostc:\\foo\\bar", S_OK},
    {"file:\\\\host\\c:\\foo\\bar", "\\\\hostc:\\foo\\bar", S_OK},
    {"file:\\\\host\\ca\\foo\\bar", "\\\\host\\ca\\foo\\bar", S_OK},
    {"file:\\\\host\\c|\\foo\\bar", "\\\\hostc|\\foo\\bar", S_OK},
    {"file:\\%5Chost\\c:\\foo\\bar", "\\\\host\\c:\\foo\\bar", S_OK},
    {"file:\\\\host\\cx:\\foo\\bar", "\\\\host\\cx:\\foo\\bar", S_OK},
    {"file://c:/foo/bar", "c:\\foo\\bar", S_OK},
    {"file://c:/d:/foo/bar", "c:\\d:\\foo\\bar", S_OK},
    {"file://c|/d|/foo/bar", "c:\\d|\\foo\\bar", S_OK},
    {"file://host/foo/bar", "\\\\host\\foo\\bar", S_OK},
    {"file:/foo/bar", "\\foo\\bar", S_OK},
    {"file:/foo/bar/", "\\foo\\bar\\", S_OK},
    {"file:foo/bar", "foo\\bar", S_OK},
    {"file:c:/foo/bar", "c:\\foo\\bar", S_OK},
    {"file:c|/foo/bar", "c:\\foo\\bar", S_OK},
    {"file:cx|/foo/bar", "cx|\\foo\\bar", S_OK},
    {"file:////c:/foo/bar", "c:\\foo\\bar", S_OK},
/*    {"file:////c:/foo/foo%20bar", "c:\\foo\\foo%20bar", S_OK},*/

    {"c:\\foo\\bar", NULL, E_INVALIDARG},
    {"foo/bar", NULL, E_INVALIDARG},
    {"http://foo/bar", NULL, E_INVALIDARG},

};

struct {
    char url[30];
    const char *expect;
} TEST_URL_UNESCAPE[] = {
    {"file://foo/bar", "file://foo/bar"},
    {"file://fo%20o%5Ca/bar", "file://fo o\\a/bar"}
};


struct {
    const char *path;
    BOOL expect;
} TEST_PATH_IS_URL[] = {
    {"http://foo/bar", TRUE},
    {"c:\\foo\\bar", FALSE},
    {"foo://foo/bar", TRUE},
    {"foo\\bar", FALSE},
    {"foo.bar", FALSE},
    {"bogusscheme:", TRUE},
    {"http:partial", TRUE}
};

struct {
    const char *url;
    BOOL expectOpaque;
    BOOL expectFile;
} TEST_URLIS_ATTRIBS[] = {
    {	"ftp:",						FALSE,	FALSE	},
    {	"http:",					FALSE,	FALSE	},
    {	"gopher:",					FALSE,	FALSE	},
    {	"mailto:",					TRUE,	FALSE	},
    {	"news:",					FALSE,	FALSE	},
    {	"nntp:",					FALSE,	FALSE	},
    {	"telnet:",					FALSE,	FALSE	},
    {	"wais:",					FALSE,	FALSE	},
    {	"file:",					FALSE,	TRUE	},
    {	"mk:",						FALSE,	FALSE	},
    {	"https:",					FALSE,	FALSE	},
    {	"shell:",					TRUE,	FALSE	},
    {	"https:",					FALSE,	FALSE	},
    {   "snews:",					FALSE,	FALSE	},
    {   "local:",					FALSE,	FALSE	},
    {	"javascript:",					TRUE,	FALSE	},
    {	"vbscript:",					TRUE,	FALSE	},
    {	"about:",					TRUE,	FALSE	},
    {   "res:",						FALSE,	FALSE	},
    {	"bogusscheme:",					FALSE,	FALSE	},
    {	"file:\\\\e:\\b\\c",				FALSE,	TRUE	},
    {	"file://e:/b/c",				FALSE,	TRUE	},
    {	"http:partial",					FALSE,	FALSE	},
    {	"mailto://www.winehq.org/test.html",		TRUE,	FALSE	},
    {	"file:partial",					FALSE,	TRUE	}
};


static LPWSTR GetWideString(const char* szString)
{
  LPWSTR wszString = HeapAlloc(GetProcessHeap(), 0, (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));
  
  MultiByteToWideChar(0, 0, szString, -1, wszString, INTERNET_MAX_URL_LENGTH);

  return wszString;
}

static void FreeWideString(LPWSTR wszString)
{
   HeapFree(GetProcessHeap(), 0, wszString);
}

static void hash_url(const char* szUrl)
{
  LPCSTR szTestUrl = szUrl;
  LPWSTR wszTestUrl = GetWideString(szTestUrl);
  
  DWORD cbSize = sizeof(DWORD);
  DWORD dwHash1, dwHash2;
  ok(UrlHashA(szTestUrl, (LPBYTE)&dwHash1, cbSize) == S_OK, "UrlHashA didn't return S_OK\n");
  ok(UrlHashW(wszTestUrl, (LPBYTE)&dwHash2, cbSize) == S_OK, "UrlHashW didn't return S_OK\n");

  FreeWideString(wszTestUrl);

  ok(dwHash1 == dwHash2, "Hashes didn't compare\n");
}

static void test_UrlHash(void)
{
  hash_url(TEST_URL_1);
  hash_url(TEST_URL_2);
  hash_url(TEST_URL_3);
}

static void test_url_part(const char* szUrl, DWORD dwPart, DWORD dwFlags, const char* szExpected)
{
  CHAR szPart[INTERNET_MAX_URL_LENGTH];
  WCHAR wszPart[INTERNET_MAX_URL_LENGTH];
  LPWSTR wszUrl = GetWideString(szUrl);
  LPWSTR wszConvertedPart;

  DWORD dwSize;

  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartA(szUrl, szPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartA for \"%s\" part 0x%08lx didn't return S_OK but \"%s\"\n", szUrl, dwPart, szPart);
  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartW(wszUrl, wszPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartW didn't return S_OK\n" );

  wszConvertedPart = GetWideString(szPart);

  ok(strcmpW(wszPart,wszConvertedPart)==0, "Strings didn't match between ascii and unicode UrlGetPart!\n");

  FreeWideString(wszUrl);
  FreeWideString(wszConvertedPart);

  /* Note that v6.0 and later don't return '?' with the query */
  ok(strcmp(szPart,szExpected)==0 ||
     (*szExpected=='?' && !strcmp(szPart,szExpected+1)),
	 "Expected %s, but got %s\n", szExpected, szPart);
}

static void test_UrlGetPart(void)
{
  test_url_part(TEST_URL_3, URL_PART_HOSTNAME, 0, "localhost");
  test_url_part(TEST_URL_3, URL_PART_PORT, 0, "21");
  test_url_part(TEST_URL_3, URL_PART_USERNAME, 0, "foo");
  test_url_part(TEST_URL_3, URL_PART_PASSWORD, 0, "bar");
  test_url_part(TEST_URL_3, URL_PART_SCHEME, 0, "http");
  test_url_part(TEST_URL_3, URL_PART_QUERY, 0, "?query=x&return=y");
}

static void test_url_escape(const char *szUrl, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwEscaped;
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR *urlW, *expected_urlW;
    dwEscaped=INTERNET_MAX_URL_LENGTH;

    ok(UrlEscapeA(szUrl, szReturnUrl, &dwEscaped, dwFlags) == dwExpectReturn, "UrlEscapeA didn't return 0x%08lx from \"%s\"\n", dwExpectReturn, szUrl);
    ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", szExpectUrl, szReturnUrl, szUrl);
    
    dwEscaped = INTERNET_MAX_URL_LENGTH;
    urlW = GetWideString(szUrl);
    expected_urlW = GetWideString(szExpectUrl);
    ok(UrlEscapeW(urlW, ret_urlW, &dwEscaped, dwFlags) == dwExpectReturn, "UrlEscapeW didn't return 0x%08lx from \"%s\"\n", dwExpectReturn, szUrl);
    WideCharToMultiByte(CP_ACP,0,ret_urlW,-1,szReturnUrl,INTERNET_MAX_URL_LENGTH,0,0);
    ok(strcmpW(ret_urlW, expected_urlW)==0, "Expected \"%s\", but got \"%s\" from \"%s\" flags %08lx\n", szExpectUrl, szReturnUrl, szUrl, dwFlags);
    FreeWideString(urlW);
    FreeWideString(expected_urlW);

}

static void test_url_canonicalize(const char *szUrl, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl = GetWideString(szUrl);
    LPWSTR wszExpectUrl = GetWideString(szExpectUrl);
    LPWSTR wszConvertedUrl;
    
    DWORD dwSize;
    
    dwSize = INTERNET_MAX_URL_LENGTH;
    ok(UrlCanonicalizeA(szUrl, NULL, &dwSize, dwFlags) != dwExpectReturn, "Unexpected return for NULL buffer\n");
    ok(UrlCanonicalizeA(szUrl, szReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeA didn't return 0x%08lx\n", dwExpectReturn);
    ok(strcmp(szReturnUrl,szExpectUrl)==0, "UrlCanonicalizeA dwFlags 0x%08lx Expected %s, but got %s\n", dwFlags, szExpectUrl, szReturnUrl);

    dwSize = INTERNET_MAX_URL_LENGTH;
    ok(UrlCanonicalizeW(wszUrl, NULL, &dwSize, dwFlags) != dwExpectReturn, "Unexpected return for NULL buffer\n");
    ok(UrlCanonicalizeW(wszUrl, wszReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeW didn't return 0x%08lx\n", dwExpectReturn);
    wszConvertedUrl = GetWideString(szReturnUrl);
    ok(strcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCanonicalize!\n");
    FreeWideString(wszConvertedUrl);
    
            
    FreeWideString(wszUrl);
    FreeWideString(wszExpectUrl);
}


static void test_UrlEscape(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_ESCAPE)/sizeof(TEST_ESCAPE[0]); i++) {
        test_url_escape(TEST_ESCAPE[i].url, TEST_ESCAPE[i].flags,
                              TEST_ESCAPE[i].expectret, TEST_ESCAPE[i].expecturl);
    }
}

static void test_UrlCanonicalize(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_CANONICALIZE)/sizeof(TEST_CANONICALIZE[0]); i++) {
        test_url_canonicalize(TEST_CANONICALIZE[i].url, TEST_CANONICALIZE[i].flags,
                              TEST_CANONICALIZE[i].expectret, TEST_CANONICALIZE[i].expecturl);
    }
}

static void test_url_combine(const char *szUrl1, const char *szUrl2, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl)
{
    HRESULT hr;
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl1 = GetWideString(szUrl1);
    LPWSTR wszUrl2 = GetWideString(szUrl2);
    LPWSTR wszExpectUrl = GetWideString(szExpectUrl);
    LPWSTR wszConvertedUrl;

    DWORD dwSize;
    DWORD dwExpectLen = lstrlen(szExpectUrl);

    hr = UrlCombineA(szUrl1, szUrl2, NULL, NULL, dwFlags);
    ok(hr == E_INVALIDARG, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, E_INVALIDARG);
    
    dwSize = 0;
    hr = UrlCombineA(szUrl1, szUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);
    
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, dwExpectReturn);
    ok(dwSize == dwExpectLen, "Got length %ld, expected %ld\n", dwSize, dwExpectLen);
    if(SUCCEEDED(hr)) {
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected %s, but got %s\n", szExpectUrl, szReturnUrl);
    }

    dwSize = 0;
    hr = UrlCombineW(wszUrl1, wszUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08lx, expected 0x%08lx\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %ld, expected %ld\n", dwSize, dwExpectLen+1);
    
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineW returned 0x%08lx, expected 0x%08lx\n", hr, dwExpectReturn);
    ok(dwSize == dwExpectLen, "Got length %ld, expected %ld\n", dwSize, dwExpectLen);
    if(SUCCEEDED(hr)) {
        wszConvertedUrl = GetWideString(szReturnUrl);
        ok(strcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCombine!\n");
        FreeWideString(wszConvertedUrl);
    }

    FreeWideString(wszUrl1);
    FreeWideString(wszUrl2);
    FreeWideString(wszExpectUrl);
}

static void test_UrlCombine(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_COMBINE)/sizeof(TEST_COMBINE[0]); i++) {
        test_url_combine(TEST_COMBINE[i].url1, TEST_COMBINE[i].url2, TEST_COMBINE[i].flags,
                         TEST_COMBINE[i].expectret, TEST_COMBINE[i].expecturl);
    }
}

static void test_UrlCreateFromPath(void)
{
    size_t i;
    char ret_url[INTERNET_MAX_URL_LENGTH];
    DWORD len, ret;
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR *pathW, *urlW;

    for(i = 0; i < sizeof(TEST_URLFROMPATH) / sizeof(TEST_URLFROMPATH[0]); i++) {
        len = INTERNET_MAX_URL_LENGTH;
        ret = UrlCreateFromPathA(TEST_URLFROMPATH[i].path, ret_url, &len, 0);
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08lx from path %s\n", ret, TEST_URLFROMPATH[i].path);
        ok(!lstrcmpi(ret_url, TEST_URLFROMPATH[i].url), "url %s from path %s\n", ret_url, TEST_URLFROMPATH[i].path);
        ok(len == strlen(ret_url), "ret len %ld from path %s\n", len, TEST_URLFROMPATH[i].path);

        len = INTERNET_MAX_URL_LENGTH;
        pathW = GetWideString(TEST_URLFROMPATH[i].path);
        urlW = GetWideString(TEST_URLFROMPATH[i].url);
        ret = UrlCreateFromPathW(pathW, ret_urlW, &len, 0);
        WideCharToMultiByte(CP_ACP, 0, ret_urlW, -1, ret_url, sizeof(ret_url),0,0);
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08lx from path L\"%s\", expected %08lx\n",
           ret, TEST_URLFROMPATH[i].path, TEST_URLFROMPATH[i].ret);
        ok(!lstrcmpiW(ret_urlW, urlW), "got %s expected %s from path L\"%s\"\n", ret_url, TEST_URLFROMPATH[i].url, TEST_URLFROMPATH[i].path);
        ok(len == strlenW(ret_urlW), "ret len %ld from path L\"%s\"\n", len, TEST_URLFROMPATH[i].path);
        FreeWideString(urlW);
        FreeWideString(pathW);
    }
}

static void test_UrlIs(void)
{
    BOOL ret;
    size_t i;
    WCHAR wurl[80];

    for(i = 0; i < sizeof(TEST_PATH_IS_URL) / sizeof(TEST_PATH_IS_URL[0]); i++) {
	MultiByteToWideChar(CP_ACP, 0, TEST_PATH_IS_URL[i].path, -1, wurl, 80);

        ret = UrlIsA( TEST_PATH_IS_URL[i].path, URLIS_URL );
        ok( ret == TEST_PATH_IS_URL[i].expect,
            "returned %d from path %s, expected %d\n", ret, TEST_PATH_IS_URL[i].path,
            TEST_PATH_IS_URL[i].expect );

        ret = UrlIsW( wurl, URLIS_URL );
        ok( ret == TEST_PATH_IS_URL[i].expect,
            "returned %d from path (UrlIsW) %s, expected %d\n", ret, TEST_PATH_IS_URL[i].path,
            TEST_PATH_IS_URL[i].expect );
    }
    for(i = 0; i < sizeof(TEST_URLIS_ATTRIBS) / sizeof(TEST_URLIS_ATTRIBS[0]); i++) {
	MultiByteToWideChar(CP_ACP, 0, TEST_URLIS_ATTRIBS[i].url, -1, wurl, 80);

        ret = UrlIsA( TEST_URLIS_ATTRIBS[i].url, URLIS_OPAQUE);
	ok( ret == TEST_URLIS_ATTRIBS[i].expectOpaque,
	    "returned %d for URLIS_OPAQUE, url \"%s\", expected %d\n", ret, TEST_URLIS_ATTRIBS[i].url,
	    TEST_URLIS_ATTRIBS[i].expectOpaque );
        ret = UrlIsA( TEST_URLIS_ATTRIBS[i].url, URLIS_FILEURL);
	ok( ret == TEST_URLIS_ATTRIBS[i].expectFile,
	    "returned %d for URLIS_FILEURL, url \"%s\", expected %d\n", ret, TEST_URLIS_ATTRIBS[i].url,
	    TEST_URLIS_ATTRIBS[i].expectFile );

        ret = UrlIsW( wurl, URLIS_OPAQUE);
	ok( ret == TEST_URLIS_ATTRIBS[i].expectOpaque,
	    "returned %d for URLIS_OPAQUE (UrlIsW), url \"%s\", expected %d\n", ret, TEST_URLIS_ATTRIBS[i].url,
	    TEST_URLIS_ATTRIBS[i].expectOpaque );
        ret = UrlIsW( wurl, URLIS_FILEURL);
	ok( ret == TEST_URLIS_ATTRIBS[i].expectFile,
	    "returned %d for URLIS_FILEURL (UrlIsW), url \"%s\", expected %d\n", ret, TEST_URLIS_ATTRIBS[i].url,
	    TEST_URLIS_ATTRIBS[i].expectFile );
    }
}

static void test_UrlUnescape(void)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR *urlW, *expected_urlW; 
    DWORD dwEscaped;
    size_t i;

    for(i=0; i<sizeof(TEST_URL_UNESCAPE)/sizeof(TEST_URL_UNESCAPE[0]); i++) { 
        dwEscaped=INTERNET_MAX_URL_LENGTH;
        ok(UrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, &dwEscaped, 0) == S_OK, "UrlEscapeA didn't return 0x%08lx from \"%s\"\n", S_OK, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,TEST_URL_UNESCAPE[i].expect)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url);

        dwEscaped = INTERNET_MAX_URL_LENGTH;
        urlW = GetWideString(TEST_URL_UNESCAPE[i].url);
        expected_urlW = GetWideString(TEST_URL_UNESCAPE[i].expect);
        ok(UrlUnescapeW(urlW, ret_urlW, &dwEscaped, 0) == S_OK, "UrlEscapeW didn't return 0x%08lx from \"%s\"\n", S_OK, TEST_URL_UNESCAPE[i].url);
        WideCharToMultiByte(CP_ACP,0,ret_urlW,-1,szReturnUrl,INTERNET_MAX_URL_LENGTH,0,0);
        ok(strcmpW(ret_urlW, expected_urlW)==0, "Expected \"%s\", but got \"%s\" from \"%s\" flags %08lx\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url, 0L);
        FreeWideString(urlW);
        FreeWideString(expected_urlW);
    }

}

static void test_PathSearchAndQualify(void)
{
    WCHAR path1[] = {'c',':','\\','f','o','o',0};
    WCHAR expect1[] = {'c',':','\\','f','o','o',0};
    WCHAR path2[] = {'c',':','f','o','o',0};
    WCHAR c_drive[] = {'c',':',0}; 
    WCHAR foo[] = {'f','o','o',0}; 
    WCHAR path3[] = {'\\','f','o','o',0};
    WCHAR winini[] = {'w','i','n','.','i','n','i',0};
    WCHAR out[MAX_PATH];
    WCHAR cur_dir[MAX_PATH];
    WCHAR dot[] = {'.',0};

    /* c:\foo */
    ok(PathSearchAndQualifyW(path1, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    ok(!lstrcmpiW(out, expect1), "strings don't match\n");

    /* c:foo */
    ok(PathSearchAndQualifyW(path2, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(c_drive, MAX_PATH, cur_dir, NULL);
    PathAddBackslashW(cur_dir);
    strcatW(cur_dir, foo);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");    

    /* foo */
    ok(PathSearchAndQualifyW(foo, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(dot, MAX_PATH, cur_dir, NULL);
    PathAddBackslashW(cur_dir);
    strcatW(cur_dir, foo);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");    

    /* \foo */
    ok(PathSearchAndQualifyW(path3, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(dot, MAX_PATH, cur_dir, NULL);
    strcpyW(cur_dir + 2, path3);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");

    /* win.ini */
    ok(PathSearchAndQualifyW(winini, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    if(!SearchPathW(NULL, winini, NULL, MAX_PATH, cur_dir, NULL))
        GetFullPathNameW(winini, MAX_PATH, cur_dir, NULL);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");

}

static void test_PathCreateFromUrl(void)
{
    size_t i;
    char ret_path[INTERNET_MAX_URL_LENGTH];
    DWORD len, ret;
    WCHAR ret_pathW[INTERNET_MAX_URL_LENGTH];
    WCHAR *pathW, *urlW;

    for(i = 0; i < sizeof(TEST_PATHFROMURL) / sizeof(TEST_PATHFROMURL[0]); i++) {
        len = INTERNET_MAX_URL_LENGTH;
        ret = PathCreateFromUrlA(TEST_PATHFROMURL[i].url, ret_path, &len, 0);
        ok(ret == TEST_PATHFROMURL[i].ret, "ret %08lx from url %s\n", ret, TEST_PATHFROMURL[i].url);
        if(TEST_PATHFROMURL[i].path) {
           ok(!lstrcmpi(ret_path, TEST_PATHFROMURL[i].path), "got %s expected %s from url %s\n", ret_path, TEST_PATHFROMURL[i].path,  TEST_PATHFROMURL[i].url);
           ok(len == strlen(ret_path), "ret len %ld from url %s\n", len, TEST_PATHFROMURL[i].url);
        }
        len = INTERNET_MAX_URL_LENGTH;
        pathW = GetWideString(TEST_PATHFROMURL[i].path);
        urlW = GetWideString(TEST_PATHFROMURL[i].url);
        ret = PathCreateFromUrlW(urlW, ret_pathW, &len, 0);
        WideCharToMultiByte(CP_ACP, 0, ret_pathW, -1, ret_path, sizeof(ret_path),0,0);
        ok(ret == TEST_PATHFROMURL[i].ret, "ret %08lx from url L\"%s\"\n", ret, TEST_PATHFROMURL[i].url);
        if(TEST_PATHFROMURL[i].path) {
            ok(!lstrcmpiW(ret_pathW, pathW), "got %s expected %s from url L\"%s\"\n", ret_path, TEST_PATHFROMURL[i].path, TEST_PATHFROMURL[i].url);
            ok(len == strlenW(ret_pathW), "ret len %ld from url L\"%s\"\n", len, TEST_PATHFROMURL[i].url);
        }
        FreeWideString(urlW);
        FreeWideString(pathW);
    }
}


static void test_PathIsUrl(void)
{
    size_t i;
    BOOL ret;

    for(i = 0; i < sizeof(TEST_PATH_IS_URL)/sizeof(TEST_PATH_IS_URL[0]); i++) {
        ret = PathIsURLA(TEST_PATH_IS_URL[i].path);
        ok(ret == TEST_PATH_IS_URL[i].expect,
           "returned %d from path %s, expected %d\n", ret, TEST_PATH_IS_URL[i].path,
           TEST_PATH_IS_URL[i].expect);
    }
}

static const DWORD SHELL_charclass[] =
{
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000080, 0x00000100, 0x00000200, 0x00000100,
    0x00000100, 0x00000100, 0x00000100, 0x00000100,
    0x00000100, 0x00000100, 0x00000002, 0x00000100,
    0x00000040, 0x00000100, 0x00000004, 0x00000000,
    0x00000100, 0x00000100, 0x00000100, 0x00000100,
    0x00000100, 0x00000100, 0x00000100, 0x00000100,
    0x00000100, 0x00000100, 0x00000010, 0x00000020,
    0x00000000, 0x00000100, 0x00000000, 0x00000001,
    0x00000100, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0x00000100,
    0x00000008, 0x00000100, 0x00000100, 0x00000100,
    0x00000100, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0x00000100,
    0x00000000, 0x00000100, 0x00000100
};

static void test_PathIsValidCharA(void)
{
    BOOL ret;
    unsigned int c;

    ret = pPathIsValidCharA( 0x7f, 0 );
    ok ( !ret, "PathIsValidCharA succeeded: 0x%08lx\n", (DWORD)ret );

    ret = pPathIsValidCharA( 0x7f, 1 );
    ok ( !ret, "PathIsValidCharA succeeded: 0x%08lx\n", (DWORD)ret );

    for (c = 0; c < 0x7f; c++)
    {
        ret = pPathIsValidCharA( c, ~0U );
        ok ( ret == SHELL_charclass[c] || (ret == 1 && SHELL_charclass[c] == 0xffffffff),
             "PathIsValidCharA failed: 0x%02x got 0x%08lx expected 0x%08lx\n",
             c, (DWORD)ret, SHELL_charclass[c] );
    }

    for (c = 0x7f; c <= 0xff; c++)
    {
        ret = pPathIsValidCharA( c, ~0U );
        ok ( ret == 0x00000100,
             "PathIsValidCharA failed: 0x%02x got 0x%08lx expected 0x00000100\n",
             c, (DWORD)ret );
    }
}

static void test_PathIsValidCharW(void)
{
    BOOL ret;
    unsigned int c;

    ret = pPathIsValidCharW( 0x7f, 0 );
    ok ( !ret, "PathIsValidCharW succeeded: 0x%08lx\n", (DWORD)ret );

    ret = pPathIsValidCharW( 0x7f, 1 );
    ok ( !ret, "PathIsValidCharW succeeded: 0x%08lx\n", (DWORD)ret );

    for (c = 0; c < 0x7f; c++)
    {
        ret = pPathIsValidCharW( c, ~0U );
        ok ( ret == SHELL_charclass[c] || (ret == 1 && SHELL_charclass[c] == 0xffffffff),
             "PathIsValidCharW failed: 0x%02x got 0x%08lx expected 0x%08lx\n",
             c, (DWORD)ret, SHELL_charclass[c] );
    }

    for (c = 0x007f; c <= 0xffff; c++)
    {
        ret = pPathIsValidCharW( c, ~0U );
        ok ( ret == 0x00000100,
             "PathIsValidCharW failed: 0x%02x got 0x%08lx expected 0x00000100\n",
             c, (DWORD)ret );
    }
}

static void test_PathMakePretty(void)
{
   char buff[MAX_PATH];

   ok (PathMakePrettyA(NULL) == FALSE, "PathMakePretty: NULL path succeeded\n");
   buff[0] = '\0';
   ok (PathMakePrettyA(buff) == TRUE, "PathMakePretty: Empty path failed\n");

   strcpy(buff, "C:\\A LONG FILE NAME WITH \\SPACES.TXT");
   ok (PathMakePrettyA(buff) == TRUE, "PathMakePretty: Long UC name failed\n");
   ok (strcmp(buff, "C:\\a long file name with \\spaces.txt") == 0,
       "PathMakePretty: Long UC name not changed\n");

   strcpy(buff, "C:\\A LONG FILE NAME WITH \\MixedCase.TXT");
   ok (PathMakePrettyA(buff) == FALSE, "PathMakePretty: Long MC name succeeded\n");
   ok (strcmp(buff, "C:\\A LONG FILE NAME WITH \\MixedCase.TXT") == 0,
       "PathMakePretty: Failed but modified path\n");

   strcpy(buff, "TEST");
   ok (PathMakePrettyA(buff) == TRUE,  "PathMakePretty: Short name failed\n");
   ok (strcmp(buff, "Test") == 0,  "PathMakePretty: 1st char lowercased %s\n", buff);
}

START_TEST(path)
{
  hShlwapi = LoadLibraryA("shlwapi.dll");
  if (!hShlwapi) return;

  test_UrlHash();
  test_UrlGetPart();
  test_UrlCanonicalize();
  test_UrlEscape();
  test_UrlCombine();
  test_UrlCreateFromPath();
  test_UrlIs();
  test_UrlUnescape();

  test_PathSearchAndQualify();
  test_PathCreateFromUrl();
  test_PathIsUrl();
  
  test_PathMakePretty();

  /* For whatever reason, PathIsValidCharA and PathAppendA share the same
   * ordinal number in some native versions. Check this to prevent a crash.
   */
  pPathIsValidCharA = (void*)GetProcAddress(hShlwapi, (LPSTR)455);
  if (pPathIsValidCharA && pPathIsValidCharA != (void*)GetProcAddress(hShlwapi, "PathAppendA"))
  {
    test_PathIsValidCharA();

     pPathIsValidCharW = (void*)GetProcAddress(hShlwapi, (LPSTR)456);
     if (pPathIsValidCharW) test_PathIsValidCharW();
  }
}
