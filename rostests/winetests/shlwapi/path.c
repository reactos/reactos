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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

static HMODULE hShlwapi;
static HRESULT (WINAPI *pPathIsValidCharA)(char,DWORD);
static HRESULT (WINAPI *pPathIsValidCharW)(WCHAR,DWORD);
static LPWSTR  (WINAPI *pPathCombineW)(LPWSTR, LPCWSTR, LPCWSTR);

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
    {"http://www.winehq.org/tests\\../#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org/tests/..\\#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org\\tests/../#example", 0, S_OK, "http://www.winehq.org/#example"},
    {"http://www.winehq.org/tests/../#example", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../#example"},
    {"http://www.winehq.org/tests/foo bar", URL_ESCAPE_SPACES_ONLY| URL_DONT_ESCAPE_EXTRA_INFO , S_OK, "http://www.winehq.org/tests/foo%20bar"},
    {"http://www.winehq.org/tests/foo%20bar", URL_UNESCAPE , S_OK, "http://www.winehq.org/tests/foo bar"},
    {"file:///c:/tests/foo%20bar", URL_UNESCAPE , S_OK, "file:///c:/tests/foo bar"},
    {"file:///c:/tests\\foo%20bar", URL_UNESCAPE , S_OK, "file:///c:/tests/foo bar"},
    {"file:///c:/tests/foo%20bar", 0, S_OK, "file:///c:/tests/foo%20bar"},
    {"file:///c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar"},
    {"file://c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar"},
    {"file://c:/tests\\../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar"},
    {"file://c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar"},
    {"file:///c://tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\\\tests\\foo bar"},
    {"file:///c:\\tests\\foo bar", 0, S_OK, "file:///c:/tests/foo bar"},
    {"file:///c:\\tests\\foo bar", URL_DONT_SIMPLIFY, S_OK, "file:///c:/tests/foo bar"},
    {"http://www.winehq.org/site/about", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org/site/about"},
    {"file_://www.winehq.org/site/about", URL_FILE_USE_PATHURL, S_OK, "file_://www.winehq.org/site/about"},
    {"c:\\dir\\file", 0, S_OK, "file:///c:/dir/file"},
    {"file:///c:\\dir\\file", 0, S_OK, "file:///c:/dir/file"},
    {"c:dir\\file", 0, S_OK, "file:///c:dir/file"},
    {"c:\\tests\\foo bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar"},
    {"c:\\tests\\foo bar", 0, S_OK, "file:///c:/tests/foo%20bar"},
    {"A", 0, S_OK, "A"},
    {"", 0, S_OK, ""}
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
    {"http://www.%77inehq.org/tests", "tests1", 0, S_OK, "http://www.%77inehq.org/tests1"},
    /*FIXME {"http://www.winehq.org/tests", "../tests2", 0, S_OK, "http://www.winehq.org/tests2"},*/
    {"http://www.winehq.org/tests/", "../tests3", 0, S_OK, "http://www.winehq.org/tests3"},
    {"http://www.winehq.org/tests/test1", "test2", 0, S_OK, "http://www.winehq.org/tests/test2"},
    {"http://www.winehq.org/tests/../tests", "tests4", 0, S_OK, "http://www.winehq.org/tests4"},
    {"http://www.winehq.org/tests/../tests/", "tests5", 0, S_OK, "http://www.winehq.org/tests/tests5"},
    {"http://www.winehq.org/tests/../tests/", "/tests6/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/..", "tests7/..", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/?query=x&return=y", "tests8", 0, S_OK, "http://www.winehq.org/tests/tests8"},
    {"http://www.winehq.org/tests/#example", "tests9", 0, S_OK, "http://www.winehq.org/tests/tests9"},
    {"http://www.winehq.org/tests/../tests/", "/tests10/..", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests10/.."},
    {"http://www.winehq.org/tests/../", "tests11", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../tests11"},
    {"file:///C:\\dir\\file.txt", "test.txt", 0, S_OK, "file:///C:/dir/test.txt"},
    {"http://www.winehq.org/test/", "test%20file.txt", 0, S_OK, "http://www.winehq.org/test/test%20file.txt"},
    {"http://www.winehq.org/test/", "test%20file.txt", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org/test/test%20file.txt"},
    {"http://www.winehq.org%2ftest/", "test%20file.txt", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org%2ftest/test%20file.txt"},
    {"xxx:@MSITStore:file.chm/file.html", "dir/file", 0, S_OK, "xxx:dir/file"},
    {"mk:@MSITStore:file.chm::/file.html", "/dir/file", 0, S_OK, "mk:@MSITStore:file.chm::/dir/file"},
    {"mk:@MSITStore:file.chm::/file.html", "mk:@MSITStore:file.chm::/dir/file", 0, S_OK, "mk:@MSITStore:file.chm::/dir/file"},
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

struct {
    const char *path;
    const char *result;
} TEST_PATH_UNQUOTE_SPACES[] = {
    { "abcdef",                    "abcdef"         },
    { "\"abcdef\"",                "abcdef"         },
    { "\"abcdef",                  "\"abcdef"       },
    { "abcdef\"",                  "abcdef\""       },
    { "\"\"abcdef\"\"",            "\"abcdef\""     },
    { "abc\"def",                  "abc\"def"       },
    { "\"abc\"def",                "\"abc\"def"     },
    { "\"abc\"def\"",              "abc\"def"       },
    { "\'abcdef\'",                "\'abcdef\'"     },
    { "\"\"",                      ""               },
    { "\"",                        ""               }
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

static LPSTR strdupA(LPCSTR p)
{
    LPSTR ret;
    DWORD len = (strlen(p) + 1);
    ret = HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(ret, p, len);
    return ret;
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
  ok( UrlGetPartA(szUrl, szPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartA for \"%s\" part 0x%08x didn't return S_OK but \"%s\"\n", szUrl, dwPart, szPart);
  dwSize = INTERNET_MAX_URL_LENGTH;
  ok( UrlGetPartW(wszUrl, wszPart, &dwSize, dwPart, dwFlags) == S_OK, "UrlGetPartW didn't return S_OK\n" );

  wszConvertedPart = GetWideString(szPart);

  ok(lstrcmpW(wszPart,wszConvertedPart)==0, "Strings didn't match between ascii and unicode UrlGetPart!\n");

  FreeWideString(wszUrl);
  FreeWideString(wszConvertedPart);

  /* Note that v6.0 and later don't return '?' with the query */
  ok(strcmp(szPart,szExpected)==0 ||
     (*szExpected=='?' && !strcmp(szPart,szExpected+1)),
	 "Expected %s, but got %s\n", szExpected, szPart);
}

static void test_UrlGetPart(void)
{
  CHAR szPart[INTERNET_MAX_URL_LENGTH];
  DWORD dwSize;
  HRESULT res;

  dwSize = sizeof szPart;
  szPart[0]='x'; szPart[1]=0;
  res = UrlGetPartA("hi", szPart, &dwSize, URL_PART_SCHEME, 0);
  todo_wine {
  ok (res==S_FALSE, "UrlGetPartA(\"hi\") returned %08X\n", res);
  ok(szPart[0]==0, "UrlGetPartA(\"hi\") return \"%s\" instead of \"\"\n", szPart);
  }
  dwSize = sizeof szPart;
  szPart[0]='x'; szPart[1]=0;
  res = UrlGetPartA("hi", szPart, &dwSize, URL_PART_QUERY, 0);
  todo_wine {
  ok (res==S_FALSE, "UrlGetPartA(\"hi\") returned %08X\n", res);
  ok(szPart[0]==0, "UrlGetPartA(\"hi\") return \"%s\" instead of \"\"\n", szPart);
  }
  
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

    ok(UrlEscapeA(szUrl, szReturnUrl, &dwEscaped, dwFlags) == dwExpectReturn, "UrlEscapeA didn't return 0x%08x from \"%s\"\n", dwExpectReturn, szUrl);
    ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", szExpectUrl, szReturnUrl, szUrl);
    
    dwEscaped = INTERNET_MAX_URL_LENGTH;
    urlW = GetWideString(szUrl);
    expected_urlW = GetWideString(szExpectUrl);
    ok(UrlEscapeW(urlW, ret_urlW, &dwEscaped, dwFlags) == dwExpectReturn, "UrlEscapeW didn't return 0x%08x from \"%s\"\n", dwExpectReturn, szUrl);
    WideCharToMultiByte(CP_ACP,0,ret_urlW,-1,szReturnUrl,INTERNET_MAX_URL_LENGTH,0,0);
    ok(lstrcmpW(ret_urlW, expected_urlW)==0, "Expected \"%s\", but got \"%s\" from \"%s\" flags %08x\n", szExpectUrl, szReturnUrl, szUrl, dwFlags);
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
    ok(UrlCanonicalizeA(szUrl, szReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeA didn't return 0x%08x\n", dwExpectReturn);
    ok(strcmp(szReturnUrl,szExpectUrl)==0, "UrlCanonicalizeA dwFlags 0x%08x Expected \"%s\", but got \"%s\"\n", dwFlags, szExpectUrl, szReturnUrl);

    dwSize = INTERNET_MAX_URL_LENGTH;
    ok(UrlCanonicalizeW(wszUrl, NULL, &dwSize, dwFlags) != dwExpectReturn, "Unexpected return for NULL buffer\n");
    ok(UrlCanonicalizeW(wszUrl, wszReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeW didn't return 0x%08x\n", dwExpectReturn);
    wszConvertedUrl = GetWideString(szReturnUrl);
    ok(lstrcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCanonicalize!\n");
    FreeWideString(wszConvertedUrl);
    
            
    FreeWideString(wszUrl);
    FreeWideString(wszExpectUrl);
}


static void test_UrlEscape(void)
{
    DWORD size;
    HRESULT ret;
    unsigned int i;

    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);

    for(i=0; i<sizeof(TEST_ESCAPE)/sizeof(TEST_ESCAPE[0]); i++) {
        test_url_escape(TEST_ESCAPE[i].url, TEST_ESCAPE[i].flags,
                              TEST_ESCAPE[i].expectret, TEST_ESCAPE[i].expecturl);
    }
}

static void test_UrlCanonicalize(void)
{
    unsigned int i;
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwSize;
    HRESULT hr;

    for(i=0; i<sizeof(TEST_CANONICALIZE)/sizeof(TEST_CANONICALIZE[0]); i++) {
        test_url_canonicalize(TEST_CANONICALIZE[i].url, TEST_CANONICALIZE[i].flags,
                              TEST_CANONICALIZE[i].expectret, TEST_CANONICALIZE[i].expecturl);
    }

    /* move to TEST_CANONICALIZE when fixed */
    dwSize = sizeof szReturnUrl;
    /*LimeWire online installer calls this*/
    hr = UrlCanonicalizeA("/uri-res/N2R?urn:sha1:B3K", szReturnUrl, &dwSize,URL_DONT_ESCAPE_EXTRA_INFO | URL_WININET_COMPATIBILITY /*0x82000000*/);
    ok(hr==S_OK,"UrlCanonicalizeA returned 0x%08x instead of S_OK\n", hr);
    todo_wine {
        ok(strcmp(szReturnUrl,"/uri-res/N2R?urn:sha1:B3K")==0, "UrlCanonicalizeA got \"%s\"  instead of \"/uri-res/N2R?urn:sha1:B3K\"\n", szReturnUrl);
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
    ok(hr == E_INVALIDARG, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, E_INVALIDARG);
    
    dwSize = 0;
    hr = UrlCombineA(szUrl1, szUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08x, expected 0x%08x\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);
    
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, dwExpectReturn);
    ok(dwSize == dwExpectLen, "Got length %d, expected %d\n", dwSize, dwExpectLen);
    if(SUCCEEDED(hr)) {
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "Expected %s, but got %s\n", szExpectUrl, szReturnUrl);
    }

    dwSize = 0;
    hr = UrlCombineW(wszUrl1, wszUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08x, expected 0x%08x\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, E_POINTER);
    ok(dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);
    
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineW returned 0x%08x, expected 0x%08x\n", hr, dwExpectReturn);
    ok(dwSize == dwExpectLen, "Got length %d, expected %d\n", dwSize, dwExpectLen);
    if(SUCCEEDED(hr)) {
        wszConvertedUrl = GetWideString(szReturnUrl);
        ok(lstrcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCombine!\n");
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
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08x from path %s\n", ret, TEST_URLFROMPATH[i].path);
        ok(!lstrcmpi(ret_url, TEST_URLFROMPATH[i].url), "url %s from path %s\n", ret_url, TEST_URLFROMPATH[i].path);
        ok(len == strlen(ret_url), "ret len %d from path %s\n", len, TEST_URLFROMPATH[i].path);

        len = INTERNET_MAX_URL_LENGTH;
        pathW = GetWideString(TEST_URLFROMPATH[i].path);
        urlW = GetWideString(TEST_URLFROMPATH[i].url);
        ret = UrlCreateFromPathW(pathW, ret_urlW, &len, 0);
        WideCharToMultiByte(CP_ACP, 0, ret_urlW, -1, ret_url, sizeof(ret_url),0,0);
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08x from path L\"%s\", expected %08x\n",
           ret, TEST_URLFROMPATH[i].path, TEST_URLFROMPATH[i].ret);
        ok(!lstrcmpiW(ret_urlW, urlW), "got %s expected %s from path L\"%s\"\n", ret_url, TEST_URLFROMPATH[i].url, TEST_URLFROMPATH[i].path);
        ok(len == lstrlenW(ret_urlW), "ret len %d from path L\"%s\"\n", len, TEST_URLFROMPATH[i].path);
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
    static char inplace[] = "file:///C:/Program%20Files";
    static WCHAR inplaceW[] = {'f','i','l','e',':','/','/','/','C',':','/',
                               'P','r','o','g','r','a','m','%','2','0','F','i','l','e','s',0};

    for(i=0; i<sizeof(TEST_URL_UNESCAPE)/sizeof(TEST_URL_UNESCAPE[0]); i++) { 
        dwEscaped=INTERNET_MAX_URL_LENGTH;
        ok(UrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, &dwEscaped, 0) == S_OK, "UrlUnescapeA didn't return 0x%08x from \"%s\"\n", S_OK, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,TEST_URL_UNESCAPE[i].expect)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url);

        dwEscaped = INTERNET_MAX_URL_LENGTH;
        urlW = GetWideString(TEST_URL_UNESCAPE[i].url);
        expected_urlW = GetWideString(TEST_URL_UNESCAPE[i].expect);
        ok(UrlUnescapeW(urlW, ret_urlW, &dwEscaped, 0) == S_OK, "UrlUnescapeW didn't return 0x%08x from \"%s\"\n", S_OK, TEST_URL_UNESCAPE[i].url);
        WideCharToMultiByte(CP_ACP,0,ret_urlW,-1,szReturnUrl,INTERNET_MAX_URL_LENGTH,0,0);
        ok(lstrcmpW(ret_urlW, expected_urlW)==0, "Expected \"%s\", but got \"%s\" from \"%s\" flags %08lx\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url, 0L);
        FreeWideString(urlW);
        FreeWideString(expected_urlW);
    }

    dwEscaped = sizeof(inplace);
    ok(UrlUnescapeA(inplace, NULL, &dwEscaped, URL_UNESCAPE_INPLACE) == S_OK, "UrlUnescapeA failed unexpectedly\n");

    dwEscaped = sizeof(inplaceW);
    ok(UrlUnescapeW(inplaceW, NULL, &dwEscaped, URL_UNESCAPE_INPLACE) == S_OK, "UrlUnescapeW failed unexpectedly\n");
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
    lstrcatW(cur_dir, foo);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");    

    /* foo */
    ok(PathSearchAndQualifyW(foo, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(dot, MAX_PATH, cur_dir, NULL);
    PathAddBackslashW(cur_dir);
    lstrcatW(cur_dir, foo);
    ok(!lstrcmpiW(out, cur_dir), "strings don't match\n");    

    /* \foo */
    ok(PathSearchAndQualifyW(path3, out, MAX_PATH) != 0,
       "PathSearchAndQualify rets 0\n");
    GetFullPathNameW(dot, MAX_PATH, cur_dir, NULL);
    lstrcpyW(cur_dir + 2, path3);
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
    static const char url[] = "http://www.winehq.org";

    /* Check ret_path = NULL */
    len = sizeof(url);
    ret = PathCreateFromUrlA(url, NULL, &len, 0); 
    ok ( ret == E_INVALIDARG, "got 0x%08x expected E_INVALIDARG\n", ret);

    for(i = 0; i < sizeof(TEST_PATHFROMURL) / sizeof(TEST_PATHFROMURL[0]); i++) {
        len = INTERNET_MAX_URL_LENGTH;
        ret = PathCreateFromUrlA(TEST_PATHFROMURL[i].url, ret_path, &len, 0);
        ok(ret == TEST_PATHFROMURL[i].ret, "ret %08x from url %s\n", ret, TEST_PATHFROMURL[i].url);
        if(TEST_PATHFROMURL[i].path) {
           ok(!lstrcmpi(ret_path, TEST_PATHFROMURL[i].path), "got %s expected %s from url %s\n", ret_path, TEST_PATHFROMURL[i].path,  TEST_PATHFROMURL[i].url);
           ok(len == strlen(ret_path), "ret len %d from url %s\n", len, TEST_PATHFROMURL[i].url);
        }
        len = INTERNET_MAX_URL_LENGTH;
        pathW = GetWideString(TEST_PATHFROMURL[i].path);
        urlW = GetWideString(TEST_PATHFROMURL[i].url);
        ret = PathCreateFromUrlW(urlW, ret_pathW, &len, 0);
        WideCharToMultiByte(CP_ACP, 0, ret_pathW, -1, ret_path, sizeof(ret_path),0,0);
        ok(ret == TEST_PATHFROMURL[i].ret, "ret %08x from url L\"%s\"\n", ret, TEST_PATHFROMURL[i].url);
        if(TEST_PATHFROMURL[i].path) {
            ok(!lstrcmpiW(ret_pathW, pathW), "got %s expected %s from url L\"%s\"\n", ret_path, TEST_PATHFROMURL[i].path, TEST_PATHFROMURL[i].url);
            ok(len == lstrlenW(ret_pathW), "ret len %d from url L\"%s\"\n", len, TEST_PATHFROMURL[i].url);
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
    ok ( !ret, "PathIsValidCharA succeeded: 0x%08x\n", (DWORD)ret );

    ret = pPathIsValidCharA( 0x7f, 1 );
    ok ( !ret, "PathIsValidCharA succeeded: 0x%08x\n", (DWORD)ret );

    for (c = 0; c < 0x7f; c++)
    {
        ret = pPathIsValidCharA( c, ~0U );
        ok ( ret == SHELL_charclass[c] || (ret == 1 && SHELL_charclass[c] == 0xffffffff),
             "PathIsValidCharA failed: 0x%02x got 0x%08x expected 0x%08x\n",
             c, (DWORD)ret, SHELL_charclass[c] );
    }

    for (c = 0x7f; c <= 0xff; c++)
    {
        ret = pPathIsValidCharA( c, ~0U );
        ok ( ret == 0x00000100,
             "PathIsValidCharA failed: 0x%02x got 0x%08x expected 0x00000100\n",
             c, (DWORD)ret );
    }
}

static void test_PathIsValidCharW(void)
{
    BOOL ret;
    unsigned int c, err_count = 0;

    ret = pPathIsValidCharW( 0x7f, 0 );
    ok ( !ret, "PathIsValidCharW succeeded: 0x%08x\n", (DWORD)ret );

    ret = pPathIsValidCharW( 0x7f, 1 );
    ok ( !ret, "PathIsValidCharW succeeded: 0x%08x\n", (DWORD)ret );

    for (c = 0; c < 0x7f; c++)
    {
        ret = pPathIsValidCharW( c, ~0U );
        ok ( ret == SHELL_charclass[c] || (ret == 1 && SHELL_charclass[c] == 0xffffffff),
             "PathIsValidCharW failed: 0x%02x got 0x%08x expected 0x%08x\n",
             c, (DWORD)ret, SHELL_charclass[c] );
    }

    for (c = 0x007f; c <= 0xffff; c++)
    {
        ret = pPathIsValidCharW( c, ~0U );
        ok ( ret == 0x00000100,
             "PathIsValidCharW failed: 0x%02x got 0x%08x expected 0x00000100\n",
             c, (DWORD)ret );
        if (ret != 0x00000100)
        {
            if(++err_count > 100 ) {
                trace("skipping rest of PathIsValidCharW tests "
                      "because of the current number of errors\n");
                break;
            }
        }
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

static void test_PathMatchSpec(void)
{
    static const char file[] = "c:\\foo\\bar\\filename.ext";
    static const char spec1[] = ".ext";
    static const char spec2[] = "*.ext";
    static const char spec3[] = "*.ext ";
    static const char spec4[] = "  *.ext";
    static const char spec5[] = "* .ext";
    static const char spec6[] = "*. ext";
    static const char spec7[] = "* . ext";
    static const char spec8[] = "*.e?t";
    static const char spec9[] = "filename.ext";
    static const char spec10[] = "*bar\\filename.ext";
    static const char spec11[] = " foo; *.ext";
    static const char spec12[] = "*.ext;*.bar";
    static const char spec13[] = "*bar*";

    ok (PathMatchSpecA(file, spec1) == FALSE, "PathMatchSpec: Spec1 failed\n");
    ok (PathMatchSpecA(file, spec2) == TRUE, "PathMatchSpec: Spec2 failed\n");
    ok (PathMatchSpecA(file, spec3) == FALSE, "PathMatchSpec: Spec3 failed\n");
    ok (PathMatchSpecA(file, spec4) == TRUE, "PathMatchSpec: Spec4 failed\n");
    todo_wine ok (PathMatchSpecA(file, spec5) == TRUE, "PathMatchSpec: Spec5 failed\n");
    todo_wine ok (PathMatchSpecA(file, spec6) == TRUE, "PathMatchSpec: Spec6 failed\n");
    ok (PathMatchSpecA(file, spec7) == FALSE, "PathMatchSpec: Spec7 failed\n");
    ok (PathMatchSpecA(file, spec8) == TRUE, "PathMatchSpec: Spec8 failed\n");
    ok (PathMatchSpecA(file, spec9) == FALSE, "PathMatchSpec: Spec9 failed\n");
    ok (PathMatchSpecA(file, spec10) == TRUE, "PathMatchSpec: Spec10 failed\n");
    ok (PathMatchSpecA(file, spec11) == TRUE, "PathMatchSpec: Spec11 failed\n");
    ok (PathMatchSpecA(file, spec12) == TRUE, "PathMatchSpec: Spec12 failed\n");
    ok (PathMatchSpecA(file, spec13) == TRUE, "PathMatchSpec: Spec13 failed\n");
}

static void test_PathCombineW(void)
{
    LPWSTR wszString, wszString2;
    WCHAR wbuf[MAX_PATH+1], wstr1[MAX_PATH] = {'C',':','\\',0}, wstr2[MAX_PATH];
    static const WCHAR expout[] = {'C',':','\\','A','A',0};
    int i;
   
    wszString2 = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));

    /* NULL test */
    wszString = pPathCombineW(NULL, NULL, NULL);
    ok (wszString == NULL, "Expected a NULL return\n");

    /* Some NULL */
    wszString2[0] = 'a';
    wszString = pPathCombineW(wszString2, NULL, NULL);
    ok (wszString == NULL, "Expected a NULL return\n");
    ok (wszString2[0] == 0, "Destination string not empty\n");

    HeapFree(GetProcessHeap(), 0, wszString2);

    /* overflow test */
    wstr2[0] = wstr2[1] = wstr2[2] = 'A';
    for (i=3; i<MAX_PATH/2; i++)
        wstr1[i] = wstr2[i] = 'A';
    wstr1[(MAX_PATH/2) - 1] = wstr2[MAX_PATH/2] = 0;
    memset(wbuf, 0xbf, sizeof(wbuf));

    wszString = pPathCombineW(wbuf, wstr1, wstr2);
    ok(wszString == NULL, "Expected a NULL return\n");
    ok(wbuf[0] == 0, "Buffer contains data\n");

    /* PathCombineW can be used in place */
    wstr1[3] = 0;
    wstr2[2] = 0;
    ok(PathCombineW(wstr1, wstr1, wstr2) == wstr1, "Expected a wstr1 return\n");
    ok(StrCmpW(wstr1, expout) == 0, "Unexpected PathCombine output\n");
}


#define LONG_LEN (MAX_PATH * 2)
#define HALF_LEN (MAX_PATH / 2 + 1)

static void test_PathCombineA(void)
{
    LPSTR str;
    char dest[MAX_PATH];
    char too_long[LONG_LEN];
    char one[HALF_LEN], two[HALF_LEN];

    /* try NULL dest */
    SetLastError(0xdeadbeef);
    str = PathCombineA(NULL, "C:\\", "one\\two\\three");
    ok(str == NULL, "Expected NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try NULL dest and NULL directory */
    SetLastError(0xdeadbeef);
    str = PathCombineA(NULL, NULL, "one\\two\\three");
    ok(str == NULL, "Expected NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try all NULL*/
    SetLastError(0xdeadbeef);
    str = PathCombineA(NULL, NULL, NULL);
    ok(str == NULL, "Expected NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try NULL file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", NULL);
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\"), "Expected C:\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try empty file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\"), "Expected C:\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try empty directory and file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "", "");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "\\"), "Expected \\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try NULL directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, NULL, "one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "one\\two\\three"), "Expected one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try NULL directory and empty file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, NULL, "");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "\\"), "Expected \\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try NULL directory and file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, NULL, NULL);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(lstrlenA(dest) == 0, "Expected 0 length, got %i\n", lstrlenA(dest));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try directory without backslash */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:", "one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try directory with backslash */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try directory with backslash and file with prepended backslash */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "\\one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try previous test, with backslash appended as well */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "\\one\\two\\three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a relative directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "relative\\dir", "\\one\\two\\three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "one\\two\\three\\"), "Expected one\\two\\three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try forward slashes */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "one/two/three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\one/two/three\\"), "Expected one/two/three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a really weird directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\/\\/", "\\one\\two\\three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try periods */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "one\\..\\two\\.\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\two\\three"), "Expected C:\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try .. as file */
    /* try forward slashes */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "..");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmp(str, "C:\\"), "Expected C:\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';

    /* try a file longer than MAX_PATH */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", too_long);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(lstrlenA(dest) == 0, "Expected 0 length, got %i\n", lstrlenA(dest));
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a directory longer than MAX_PATH */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, too_long, "one\\two\\three");
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(lstrlenA(dest) == 0, "Expected 0 length, got %i\n", lstrlenA(dest));
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    memset(one, 'b', HALF_LEN);
    memset(two, 'c', HALF_LEN);
    one[HALF_LEN - 1] = '\0';
    two[HALF_LEN - 1] = '\0';

    /* destination string is longer than MAX_PATH, but not the constituent parts */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, one, two);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(lstrlenA(dest) == 0, "Expected 0 length, got %i\n", lstrlenA(dest));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_PathAddBackslash(void)
{
    LPSTR str;
    char path[MAX_PATH];
    char too_long[LONG_LEN];

    /* try a NULL path */
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(NULL);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try an empty path */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(lstrlenA(path) == 0, "Expected empty string, got %i\n", lstrlenA(path));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a relative path */
    lstrcpyA(path, "one\\two");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmp(path, "one\\two\\"), "Expected one\\two\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try periods */
    lstrcpyA(path, "one\\..\\two");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmp(path, "one\\..\\two\\"), "Expected one\\..\\two\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try just a space */
    lstrcpyA(path, " ");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmp(path, " \\"), "Expected  \\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* path already has backslash */
    lstrcpyA(path, "C:\\one\\");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmp(path, "C:\\one\\"), "Expected C:\\one\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';

    /* path is longer than MAX_PATH */
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(too_long);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_PathAppendA(void)
{
    char path[MAX_PATH];
    char too_long[LONG_LEN];
    char one[HALF_LEN], two[HALF_LEN];
    BOOL res;

    lstrcpy(path, "C:\\one");

    /* try NULL pszMore */
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, NULL);
    ok(!res, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one"), "Expected C:\\one, got %s\n", path);

    /* try empty pszMore */
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one"), "Expected C:\\one, got %s\n", path);

    /* try NULL pszPath */
    SetLastError(0xdeadbeef);
    res = PathAppendA(NULL, "two\\three");
    ok(!res, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try empty pszPath */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "two\\three"), "Expected \\two\\three, got %s\n", path);

    /* try empty pszPath and empty pszMore */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "\\"), "Expected \\, got %s\n", path);

    /* try legit params */
    lstrcpy(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", path);

    /* try pszPath with backslash after it */
    lstrcpy(path, "C:\\one\\");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", path);

    /* try pszMore with backslash before it */
    lstrcpy(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "\\two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", path);

    /* try pszMore with backslash after it */
    lstrcpy(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three\\");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", path);

    /* try spaces in pszPath */
    lstrcpy(path, "C: \\ one ");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C: \\ one \\two\\three"), "Expected C: \\ one \\two\\three, got %s\n", path);

    /* try spaces in pszMore */
    lstrcpy(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, " two \\ three ");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(path, "C:\\one\\ two \\ three "), "Expected 'C:\\one\\ two \\ three ', got %s\n", path);

    /* pszPath is too long */
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(too_long, "two\\three");
    ok(!res, "Expected failure\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(lstrlen(too_long) == 0, "Expected length of too_long to be zero, got %i\n", lstrlen(too_long));

    /* pszMore is too long */
    lstrcpy(path, "C:\\one");
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, too_long);
    ok(!res, "Expected failure\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(lstrlen(path) == 0, "Expected length of path to be zero, got %i\n", lstrlen(path));

    /* both params combined are too long */
    memset(one, 'a', HALF_LEN);
    one[HALF_LEN - 1] = '\0';
    memset(two, 'b', HALF_LEN);
    two[HALF_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(one, two);
    ok(!res, "Expected failure\n");
    ok(lstrlen(one) == 0, "Expected length of one to be zero, got %i\n", lstrlen(one));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_PathCanonicalizeA(void)
{
    char dest[MAX_PATH];
    char too_long[LONG_LEN];
    BOOL res;

    /* try a NULL source */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, NULL);
    ok(!res, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, 
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    todo_wine
    {
        ok(!lstrcmp(dest, "test"), "Expected test, got %s\n", dest);
    }

    /* try an empty source */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "\\"), "Expected \\, got %s\n", dest);

    /* try a NULL dest */
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(NULL, "C:\\");
    ok(!res, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, 
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* try empty dest */
    dest[0] = '\0';
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\"), "Expected C:\\, got %s\n", dest);

    /* try non-empty dest */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\"), "Expected C:\\, got %s\n", dest);

    /* try a space for source */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, " ");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, " "), "Expected ' ', got %s\n", dest);

    /* try a relative path */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "one\\two");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "one\\two"), "Expected one\\two, got %s\n", dest);

    /* try current dir and previous dir */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one\\.\\..\\two\\three\\..");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\two"), "Expected C:\\two, got %s\n", dest);

    /* try simple forward slashes */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/two/three\\four/five\\six");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\one/two/three\\four/five\\six"),
       "Expected C:\\one/two/three\\four/five\\six, got %s\n", dest);

    /* try simple forward slashes with same dir */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/.\\two");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\one/.\\two"), "Expected C:\\one/.\\two, got %s\n", dest);

    /* try simple forward slashes with change dir */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/.\\two\\..");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\one/."), "Expected C:\\one/., got %s\n", dest);

    /* try forward slashes with change dirs
     * NOTE: if there is a forward slash in between two backslashes,
     * everything in between the two backslashes is considered on dir
     */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/.\\..\\two/three\\..\\four/.five");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    ok(!lstrcmp(dest, "C:\\four/.five"), "Expected C:\\four/.five, got %s\n", dest);

    /* try src is too long */
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, too_long);
    todo_wine
    {
        ok(!res, "Expected failure\n");
        ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    }
    ok(lstrlen(too_long) == LONG_LEN - 1, "Expected length LONG_LEN - 1, got %i\n", lstrlen(too_long));
}

static void test_PathFindExtensionA(void)
{
    LPSTR ext;
    char path[MAX_PATH];
    char too_long[LONG_LEN];

    /* try a NULL path */
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(NULL);
    ok(ext == NULL, "Expected NULL, got %p\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try an empty path */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path, "Expected ext == path, got %p\n", ext);
    ok(lstrlen(ext) == 0, "Expected length 0, got %i\n", lstrlen(ext));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a path without an extension */
    lstrcpy(path, "file");
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path + lstrlen(path), "Expected ext == path, got %p\n", ext);
    ok(lstrlen(ext) == 0, "Expected length 0, got %i\n", lstrlen(ext));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a path with an extension */
    lstrcpy(path, "file.txt");
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path + lstrlen("file"),
       "Expected ext == path + lstrlen(\"file\"), got %p\n", ext);
    ok(!lstrcmp(ext, ".txt"), "Expected .txt, got %s\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a path with two extensions */
    lstrcpy(path, "file.txt.doc");
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path + lstrlen("file.txt"),
       "Expected ext == path + lstrlen(\"file.txt\"), got %p\n", ext);
    ok(!lstrcmp(ext, ".doc"), "Expected .txt, got %s\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a path longer than MAX_PATH without an extension*/
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(too_long);
    ok(ext == too_long + LONG_LEN - 1, "Expected ext == too_long + LONG_LEN - 1, got %p\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try a path longer than MAX_PATH with an extension*/
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    lstrcpy(too_long + 300, ".abcde");
    too_long[lstrlen(too_long)] = 'a';
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(too_long);
    ok(ext == too_long + 300, "Expected ext == too_long + 300, got %p\n", ext);
    ok(lstrlen(ext) == LONG_LEN - 301, "Expected LONG_LEN - 301, got %i\n", lstrlen(ext));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_PathBuildRootA(void)
{
    LPSTR root;
    char path[10];
    char root_expected[26][4];
    char drive;
    int j;

    /* set up the expected paths */
    for (drive = 'A'; drive <= 'Z'; drive++)
        sprintf(root_expected[drive - 'A'], "%c:\\", drive);

    /* test the expected values */
    for (j = 0; j < 26; j++)
    {
        SetLastError(0xdeadbeef);
        lstrcpy(path, "aaaaaaaaa");
        root = PathBuildRootA(path, j);
        ok(root == path, "Expected root == path, got %p\n", root);
        ok(!lstrcmp(root, root_expected[j]), "Expected %s, got %s\n", root_expected[j], root);
        ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    }

    /* test a negative drive number */
    SetLastError(0xdeadbeef);
    lstrcpy(path, "aaaaaaaaa");
    root = PathBuildRootA(path, -1);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmp(path, "aaaaaaaaa"), "Expected aaaaaaaaa, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* test a drive number greater than 25 */
    SetLastError(0xdeadbeef);
    lstrcpy(path, "aaaaaaaaa");
    root = PathBuildRootA(path, 26);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmp(path, "aaaaaaaaa"), "Expected aaaaaaaaa, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* length of path is less than 4 */
    SetLastError(0xdeadbeef);
    lstrcpy(path, "aa");
    root = PathBuildRootA(path, 0);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmp(path, "A:\\"), "Expected A:\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* path is NULL */
    SetLastError(0xdeadbeef);
    root = PathBuildRootA(NULL, 0);
    ok(root == NULL, "Expected root == NULL, got %p\n", root);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_PathCommonPrefixA(void)
{
    char path1[MAX_PATH], path2[MAX_PATH];
    char out[MAX_PATH];
    int count;

    /* test NULL path1 */
    SetLastError(0xdeadbeef);
    lstrcpy(path2, "C:\\");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(NULL, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    todo_wine
    {
        ok(!lstrcmp(out, "aaa"), "Expected aaa, got %s\n", out);
    }
    ok(!lstrcmp(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* test NULL path2 */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "C:\\");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, NULL, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    todo_wine
    {
        ok(!lstrcmp(out, "aaa"), "Expected aaa, got %s\n", out);
    }
    ok(!lstrcmp(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* test empty path1 */
    SetLastError(0xdeadbeef);
    path1[0] = '\0';
    lstrcpy(path2, "C:\\");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    ok(lstrlen(out) == 0, "Expected 0 length out, got %i\n", lstrlen(out));
    ok(lstrlen(path1) == 0, "Expected 0 length path1, got %i\n", lstrlen(path1));
    ok(!lstrcmp(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* test empty path1 */
    SetLastError(0xdeadbeef);
    path2[0] = '\0';
    lstrcpy(path1, "C:\\");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    ok(lstrlen(out) == 0, "Expected 0 length out, got %i\n", lstrlen(out));
    ok(lstrlen(path2) == 0, "Expected 0 length path2, got %i\n", lstrlen(path2));
    ok(!lstrcmp(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* paths are legit, out is NULL */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "C:\\");
    lstrcpy(path2, "C:\\");
    count = PathCommonPrefixA(path1, path2, NULL);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmp(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(!lstrcmp(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* all parameters legit */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "C:\\");
    lstrcpy(path2, "C:\\");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmp(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(!lstrcmp(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(!lstrcmp(out, "C:\\"), "Expected C:\\, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* path1 and path2 not the same, but common prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "C:\\one\\two");
    lstrcpy(path2, "C:\\one\\three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 6, "Expected 6, got %i\n", count);
    ok(!lstrcmp(path1, "C:\\one\\two"), "Expected C:\\one\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "C:\\one\\three"), "Expected C:\\one\\three, got %s\n", path2);
    ok(!lstrcmp(out, "C:\\one"), "Expected C:\\one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try . prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\.two");
    lstrcpy(path2, "one\\.three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmp(path1, "one\\.two"), "Expected one\\.two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\.three"), "Expected one\\.three, got %s\n", path2);
    ok(!lstrcmp(out, "one"), "Expected one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try .. prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\..two");
    lstrcpy(path2, "one\\..three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmp(path1, "one\\..two"), "Expected one\\..two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\..three"), "Expected one\\..three, got %s\n", path2);
    ok(!lstrcmp(out, "one"), "Expected one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try ... prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\...two");
    lstrcpy(path2, "one\\...three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmp(path1, "one\\...two"), "Expected one\\...two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\...three"), "Expected one\\...three, got %s\n", path2);
    ok(!lstrcmp(out, "one"), "Expected one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try .\ prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\.\\two");
    lstrcpy(path2, "one\\.\\three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 5, "Expected 5, got %i\n", count);
    ok(!lstrcmp(path1, "one\\.\\two"), "Expected one\\.\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\.\\three"), "Expected one\\.\\three, got %s\n", path2);
    ok(!lstrcmp(out, "one\\."), "Expected one\\., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try ..\ prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\..\\two");
    lstrcpy(path2, "one\\..\\three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 6, "Expected 6, got %i\n", count);
    ok(!lstrcmp(path1, "one\\..\\two"), "Expected one\\..\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\..\\three"), "Expected one\\..\\three, got %s\n", path2);
    ok(!lstrcmp(out, "one\\.."), "Expected one\\.., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try ...\\ prefix */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\...\\two");
    lstrcpy(path2, "one\\...\\three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 7, "Expected 7, got %i\n", count);
    ok(!lstrcmp(path1, "one\\...\\two"), "Expected one\\...\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\...\\three"), "Expected one\\...\\three, got %s\n", path2);
    ok(!lstrcmp(out, "one\\..."), "Expected one\\..., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try prefix that is not an msdn labeled prefix type */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "same");
    lstrcpy(path2, "same");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 4, "Expected 4, got %i\n", count);
    ok(!lstrcmp(path1, "same"), "Expected same, got %s\n", path1);
    ok(!lstrcmp(path2, "same"), "Expected same, got %s\n", path2);
    ok(!lstrcmp(out, "same"), "Expected same, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try . after directory */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\mid.\\two");
    lstrcpy(path2, "one\\mid.\\three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 8, "Expected 8, got %i\n", count);
    ok(!lstrcmp(path1, "one\\mid.\\two"), "Expected one\\mid.\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\mid.\\three"), "Expected one\\mid.\\three, got %s\n", path2);
    ok(!lstrcmp(out, "one\\mid."), "Expected one\\mid., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try . in the middle of a directory */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\mid.end\\two");
    lstrcpy(path2, "one\\mid.end\\three");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 11, "Expected 11, got %i\n", count);
    ok(!lstrcmp(path1, "one\\mid.end\\two"), "Expected one\\mid.end\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "one\\mid.end\\three"), "Expected one\\mid.end\\three, got %s\n", path2);
    ok(!lstrcmp(out, "one\\mid.end"), "Expected one\\mid.end, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* try comparing a .. with the expanded path */
    SetLastError(0xdeadbeef);
    lstrcpy(path1, "one\\..\\two");
    lstrcpy(path2, "two");
    lstrcpy(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    ok(!lstrcmp(path1, "one\\..\\two"), "Expected one\\..\\two, got %s\n", path1);
    ok(!lstrcmp(path2, "two"), "Expected two, got %s\n", path2);
    ok(lstrlen(out) == 0, "Expected 0 length out, got %i\n", lstrlen(out));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
}

static void test_PathUnquoteSpaces(void)
{
    int i;
    for(i = 0; i < sizeof(TEST_PATH_UNQUOTE_SPACES) / sizeof(TEST_PATH_UNQUOTE_SPACES[0]); i++)
    {
        char *path = strdupA(TEST_PATH_UNQUOTE_SPACES[i].path);
        WCHAR *pathW = GetWideString(TEST_PATH_UNQUOTE_SPACES[i].path);
        WCHAR *resultW = GetWideString(TEST_PATH_UNQUOTE_SPACES[i].result);

        PathUnquoteSpacesA(path);
        ok(!strcmp(path, TEST_PATH_UNQUOTE_SPACES[i].result), "%s (A): got %s expected %s\n",
           TEST_PATH_UNQUOTE_SPACES[i].path, path,
           TEST_PATH_UNQUOTE_SPACES[i].result);

        PathUnquoteSpacesW(pathW);
        ok(!lstrcmpW(pathW, resultW), "%s (W): strings differ\n",
           TEST_PATH_UNQUOTE_SPACES[i].path);
        FreeWideString(pathW);
        FreeWideString(resultW);
        HeapFree(GetProcessHeap(), 0, path);
    }
}

START_TEST(path)
{
  hShlwapi = GetModuleHandleA("shlwapi.dll");

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

  test_PathAddBackslash();
  test_PathMakePretty();
  test_PathMatchSpec();

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

  pPathCombineW = (void*)GetProcAddress(hShlwapi, "PathCombineW");
  if (pPathCombineW)
    test_PathCombineW();

  test_PathCombineA();
  test_PathAppendA();
  test_PathCanonicalizeA();
  test_PathFindExtensionA();
  test_PathBuildRootA();
  test_PathCommonPrefixA();
  test_PathUnquoteSpaces();
}
