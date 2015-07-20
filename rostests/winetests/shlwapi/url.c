/* Unit test suite for Path functions
 *
 * Copyright 2002 Matthew Mastracci
 * Copyright 2007-2010 Detlef Riekenberg
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

//#include <stdarg.h>
//#include <stdio.h>

#include <wine/test.h>
//#include "windef.h"
//#include "winbase.h"
#include <winreg.h>
#include <winnls.h>
#include <shlwapi.h>
#include <wininet.h>
#include <intshcut.h>

/* ################ */
static HMODULE hShlwapi;
static HRESULT (WINAPI *pUrlUnescapeA)(LPSTR,LPSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlUnescapeW)(LPWSTR,LPWSTR,LPDWORD,DWORD);
static BOOL    (WINAPI *pUrlIsA)(LPCSTR,URLIS);
static BOOL    (WINAPI *pUrlIsW)(LPCWSTR,URLIS);
static HRESULT (WINAPI *pUrlHashA)(LPCSTR,LPBYTE,DWORD);
static HRESULT (WINAPI *pUrlHashW)(LPCWSTR,LPBYTE,DWORD);
static HRESULT (WINAPI *pUrlGetPartA)(LPCSTR,LPSTR,LPDWORD,DWORD,DWORD);
static HRESULT (WINAPI *pUrlGetPartW)(LPCWSTR,LPWSTR,LPDWORD,DWORD,DWORD);
static HRESULT (WINAPI *pUrlEscapeA)(LPCSTR,LPSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlEscapeW)(LPCWSTR,LPWSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlCreateFromPathA)(LPCSTR,LPSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlCreateFromPathW)(LPCWSTR,LPWSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlCombineA)(LPCSTR,LPCSTR,LPSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlCombineW)(LPCWSTR,LPCWSTR,LPWSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlCanonicalizeA)(LPCSTR, LPSTR, LPDWORD, DWORD);
static HRESULT (WINAPI *pUrlCanonicalizeW)(LPCWSTR, LPWSTR, LPDWORD, DWORD);
static HRESULT (WINAPI *pUrlApplySchemeA)(LPCSTR,LPSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pUrlApplySchemeW)(LPCWSTR,LPWSTR,LPDWORD,DWORD);
static HRESULT (WINAPI *pParseURLA)(LPCSTR,PARSEDURLA*);
static HRESULT (WINAPI *pParseURLW)(LPCWSTR,PARSEDURLW*);
static HRESULT (WINAPI *pHashData)(LPBYTE, DWORD, LPBYTE, DWORD);

static const char* TEST_URL_1 = "http://www.winehq.org/tests?date=10/10/1923";
static const char* TEST_URL_2 = "http://localhost:8080/tests%2e.html?date=Mon%2010/10/1923";
static const char* TEST_URL_3 = "http://foo:bar@localhost:21/internal.php?query=x&return=y";
static const char* TEST_URL_4 = "http://foo:bar@google.*.com:21/internal.php?query=x&return=y";

static const WCHAR winehqW[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/',0};
static const  CHAR winehqA[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/',0};

/* ################ */

static const CHAR untouchedA[] = "untouched";

#define TEST_APPLY_MAX_LENGTH INTERNET_MAX_URL_LENGTH

typedef struct _TEST_URL_APPLY {
    const char * url;
    DWORD flags;
    HRESULT res;
    DWORD newlen;
    const char * newurl;
} TEST_URL_APPLY;

static const TEST_URL_APPLY TEST_APPLY[] = {
    {"www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, 21, "http://www.winehq.org"},
    {"www.winehq.org", URL_APPLY_GUESSSCHEME, S_OK, 21, "http://www.winehq.org"},
    {"www.winehq.org", URL_APPLY_DEFAULT, S_OK, 21, "http://www.winehq.org"},
    {"ftp.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, 20, "ftp://ftp.winehq.org"},
    {"ftp.winehq.org", URL_APPLY_GUESSSCHEME, S_OK, 20, "ftp://ftp.winehq.org"},
    {"ftp.winehq.org", URL_APPLY_DEFAULT, S_OK, 21, "http://ftp.winehq.org"},
    {"winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, 17, "http://winehq.org"},
    {"winehq.org", URL_APPLY_GUESSSCHEME, S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"winehq.org", URL_APPLY_DEFAULT, S_OK, 17, "http://winehq.org"},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME , S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_FORCEAPPLY, S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_FORCEAPPLY | URL_APPLY_DEFAULT, S_OK, 28, "http://http://www.winehq.org"},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, 7, "http://"},
    {"", URL_APPLY_GUESSSCHEME, S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"", URL_APPLY_DEFAULT, S_OK, 7, "http://"},
    {"u:\\windows", URL_APPLY_GUESSFILE | URL_APPLY_DEFAULT, S_OK, 18, "file:///u:/windows"},
    {"u:\\windows", URL_APPLY_GUESSFILE, S_OK, 18, "file:///u:/windows"},
    {"u:\\windows", URL_APPLY_DEFAULT, S_OK, 17, "http://u:\\windows"},
    {"file:///c:/windows", URL_APPLY_GUESSFILE , S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"aa:\\windows", URL_APPLY_GUESSFILE , S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
};

/* ################ */

typedef struct _TEST_URL_CANONICALIZE {
    const char *url;
    DWORD flags;
    HRESULT expectret;
    const char *expecturl;
    BOOL todo;
} TEST_URL_CANONICALIZE;

static const TEST_URL_CANONICALIZE TEST_CANONICALIZE[] = {
    {"http://www.winehq.org/tests/../tests/../..", 0, S_OK, "http://www.winehq.org/", TRUE},
    {"http://www.winehq.org/..", 0, S_OK, "http://www.winehq.org/..", FALSE},
    {"http://www.winehq.org/tests/tests2/../../tests", 0, S_OK, "http://www.winehq.org/tests", FALSE},
    {"http://www.winehq.org/tests/../tests", 0, S_OK, "http://www.winehq.org/tests", FALSE},
    {"http://www.winehq.org/tests\n", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, S_OK, "http://www.winehq.org/tests", FALSE},
    {"http://www.winehq.org/tests\r", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, S_OK, "http://www.winehq.org/tests", FALSE},
    {"http://www.winehq.org/tests\r", 0, S_OK, "http://www.winehq.org/tests", FALSE},
    {"http://www.winehq.org/tests\r", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests", FALSE},
    {"http://www.winehq.org/tests/../tests/", 0, S_OK, "http://www.winehq.org/tests/", FALSE},
    {"http://www.winehq.org/tests/../tests/..", 0, S_OK, "http://www.winehq.org/", FALSE},
    {"http://www.winehq.org/tests/../tests/../", 0, S_OK, "http://www.winehq.org/", FALSE},
    {"http://www.winehq.org/tests/..", 0, S_OK, "http://www.winehq.org/", FALSE},
    {"http://www.winehq.org/tests/../", 0, S_OK, "http://www.winehq.org/", FALSE},
    {"http://www.winehq.org/tests/..?query=x&return=y", 0, S_OK, "http://www.winehq.org/?query=x&return=y", FALSE},
    {"http://www.winehq.org/tests/../?query=x&return=y", 0, S_OK, "http://www.winehq.org/?query=x&return=y", FALSE},
    {"\tht\ttp\t://www\t.w\tineh\t\tq.or\tg\t/\ttests/..\t?\tquer\ty=x\t\t&re\tturn=y\t\t", 0, S_OK, "http://www.winehq.org/?query=x&return=y", FALSE},
    {"http://www.winehq.org/tests/..#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests/../#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests\\../#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests/..\\#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org\\tests/../#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests/../#example", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../#example", FALSE},
    {"http://www.winehq.org/tests/foo bar", URL_ESCAPE_SPACES_ONLY| URL_DONT_ESCAPE_EXTRA_INFO , S_OK, "http://www.winehq.org/tests/foo%20bar", FALSE},
    {"http://www.winehq.org/tests/foo%20bar", URL_UNESCAPE , S_OK, "http://www.winehq.org/tests/foo bar", FALSE},
    {"http://www.winehq.org", 0, S_OK, "http://www.winehq.org/", FALSE},
    {"http:///www.winehq.org", 0, S_OK, "http:///www.winehq.org", FALSE},
    {"http:////www.winehq.org", 0, S_OK, "http:////www.winehq.org", FALSE},
    {"file:///c:/tests/foo%20bar", URL_UNESCAPE , S_OK, "file:///c:/tests/foo bar", FALSE},
    {"file:///c:/tests\\foo%20bar", URL_UNESCAPE , S_OK, "file:///c:/tests/foo bar", FALSE},
    {"file:///c:/tests/foo%20bar", 0, S_OK, "file:///c:/tests/foo%20bar", FALSE},
    {"file:///c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://localhost/c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://localhost\\c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://localhost\\\\c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://localhost\\c:\\tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://c:/tests\\../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file://c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"file:///c://tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\\\tests\\foo bar", FALSE},
    {"file:///c:\\tests\\foo bar", 0, S_OK, "file:///c:/tests/foo bar", FALSE},
    {"file:///c:\\tests\\foo bar", URL_DONT_SIMPLIFY, S_OK, "file:///c:/tests/foo bar", FALSE},
    {"file:///c:\\tests\\foobar", 0, S_OK, "file:///c:/tests/foobar", FALSE},
    {"file:///c:\\tests\\foobar", URL_WININET_COMPATIBILITY, S_OK, "file://c:\\tests\\foobar", FALSE},
    {"file://home/user/file", 0, S_OK, "file://home/user/file", FALSE},
    {"file:///home/user/file", 0, S_OK, "file:///home/user/file", FALSE},
    {"file:////home/user/file", 0, S_OK, "file://home/user/file", FALSE},
    {"file://home/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://\\\\home\\user\\file", FALSE},
    {"file:///home/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://\\home\\user\\file", FALSE},
    {"file:////home/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://\\\\home\\user\\file", FALSE},
    {"file://///home/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://\\\\home\\user\\file", FALSE},
    {"file://C:/user/file", 0, S_OK, "file:///C:/user/file", FALSE},
    {"file://C:/user/file/../asdf", 0, S_OK, "file:///C:/user/asdf", FALSE},
    {"file:///C:/user/file", 0, S_OK, "file:///C:/user/file", FALSE},
    {"file:////C:/user/file", 0, S_OK, "file:///C:/user/file", FALSE},
    {"file://C:/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://C:\\user\\file", FALSE},
    {"file:///C:/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://C:\\user\\file", FALSE},
    {"file:////C:/user/file", URL_WININET_COMPATIBILITY, S_OK, "file://C:\\user\\file", FALSE},
    {"http:///www.winehq.org", 0, S_OK, "http:///www.winehq.org", FALSE},
    {"http:///www.winehq.org", URL_WININET_COMPATIBILITY, S_OK, "http:///www.winehq.org", FALSE},
    {"http://www.winehq.org/site/about", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org/site/about", FALSE},
    {"file_://www.winehq.org/site/about", URL_FILE_USE_PATHURL, S_OK, "file_://www.winehq.org/site/about", FALSE},
    {"c:\\dir\\file", 0, S_OK, "file:///c:/dir/file", FALSE},
    {"file:///c:\\dir\\file", 0, S_OK, "file:///c:/dir/file", FALSE},
    {"c:dir\\file", 0, S_OK, "file:///c:dir/file", FALSE},
    {"c:\\tests\\foo bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"c:\\tests\\foo bar", 0, S_OK, "file:///c:/tests/foo%20bar", FALSE},
    {"c\t:\t\\te\tsts\\fo\to \tbar\t", 0, S_OK, "file:///c:/tests/foo%20bar", FALSE},
    {"res://file", 0, S_OK, "res://file/", FALSE},
    {"res://file", URL_FILE_USE_PATHURL, S_OK, "res://file/", FALSE},
    {"res:///c:/tests/foo%20bar", URL_UNESCAPE , S_OK, "res:///c:/tests/foo bar", FALSE},
    {"res:///c:/tests\\foo%20bar", URL_UNESCAPE , S_OK, "res:///c:/tests\\foo bar", FALSE},
    {"res:///c:/tests/foo%20bar", 0, S_OK, "res:///c:/tests/foo%20bar", FALSE},
    {"res:///c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res:///c:/tests/foo%20bar", FALSE},
    {"res://c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res://c:/tests/foo%20bar", FALSE},
    {"res://c:/tests\\../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res://c:/tests/foo%20bar", FALSE},
    {"res://c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res://c:/tests/foo%20bar", FALSE},
    {"res:///c://tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res:///c://tests/foo%20bar", FALSE},
    {"res:///c:\\tests\\foo bar", 0, S_OK, "res:///c:\\tests\\foo bar", FALSE},
    {"res:///c:\\tests\\foo bar", URL_DONT_SIMPLIFY, S_OK, "res:///c:\\tests\\foo bar", FALSE},
    {"res://c:\\tests\\foo bar/res", URL_FILE_USE_PATHURL, S_OK, "res://c:\\tests\\foo bar/res", FALSE},
    {"res://c:\\tests/res\\foo%20bar/strange\\sth", 0, S_OK, "res://c:\\tests/res\\foo%20bar/strange\\sth", FALSE},
    {"res://c:\\tests/res\\foo%20bar/strange\\sth", URL_FILE_USE_PATHURL, S_OK, "res://c:\\tests/res\\foo%20bar/strange\\sth", FALSE},
    {"res://c:\\tests/res\\foo%20bar/strange\\sth", URL_UNESCAPE, S_OK, "res://c:\\tests/res\\foo bar/strange\\sth", FALSE},
    {"A", 0, S_OK, "A", FALSE},
    {"../A", 0, S_OK, "../A", FALSE},
    {".\\A", 0, S_OK, ".\\A", FALSE},
    {"A\\.\\B", 0, S_OK, "A\\.\\B", FALSE},
    {"A/../B", 0, S_OK, "B", TRUE},
    {"A/../B/./../C", 0, S_OK, "C", TRUE},
    {"A/../B/./../C", URL_DONT_SIMPLIFY, S_OK, "A/../B/./../C", FALSE},
    {".", 0, S_OK, "/", TRUE},
    {"./A", 0, S_OK, "A", TRUE},
    {"A/./B", 0, S_OK, "A/B", TRUE},
    {"/:test\\", 0, S_OK, "/:test\\", TRUE},
    {"/uri-res/N2R?urn:sha1:B3K", URL_DONT_ESCAPE_EXTRA_INFO | URL_WININET_COMPATIBILITY /*0x82000000*/, S_OK, "/uri-res/N2R?urn:sha1:B3K", FALSE} /*LimeWire online installer calls this*/,
    {"http:www.winehq.org/dir/../index.html", 0, S_OK, "http:www.winehq.org/index.html"},
    {"http://localhost/test.html", URL_FILE_USE_PATHURL, S_OK, "http://localhost/test.html"},
    {"http://localhost/te%20st.html", URL_FILE_USE_PATHURL, S_OK, "http://localhost/te%20st.html"},
    {"http://www.winehq.org/%E6%A1%9C.html", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org/%E6%A1%9C.html"},
    {"mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", 0, S_OK, "mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
    {"ftp:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", 0, S_OK, "ftp:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
    {"file:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", 0, S_OK, "file:@MSITStore:C:/Program Files/AutoCAD 2008/Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
    {"http:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", 0, S_OK, "http:@MSITStore:C:/Program Files/AutoCAD 2008/Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
    {"http:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", URL_FILE_USE_PATHURL, S_OK, "http:@MSITStore:C:/Program Files/AutoCAD 2008/Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
    {"mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", URL_FILE_USE_PATHURL, S_OK, "mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
};

/* ################ */

typedef struct _TEST_URL_ESCAPE {
    const char *url;
    DWORD flags;
    DWORD expectescaped;
    HRESULT expectret;
    const char *expecturl;
} TEST_URL_ESCAPE;

static const TEST_URL_ESCAPE TEST_ESCAPE[] = {
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
    {"ftp:////fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:////fo/o@bar.baz/foo/bar"},

    {"ftp\x1f\1end/", 0, 0, S_OK, "ftp%1F%01end/"}
};

/* ################ */

typedef struct _TEST_URL_COMBINE {
    const char *url1;
    const char *url2;
    DWORD flags;
    HRESULT expectret;
    const char *expecturl;
    BOOL todo;
} TEST_URL_COMBINE;

static const TEST_URL_COMBINE TEST_COMBINE[] = {
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
    {"http://www.winehq.org/test12", "#", 0, S_OK, "http://www.winehq.org/test12#"},
    {"http://www.winehq.org/test13#aaa", "#bbb", 0, S_OK, "http://www.winehq.org/test13#bbb"},
    {"http://www.winehq.org/test14#aaa/bbb#ccc", "#", 0, S_OK, "http://www.winehq.org/test14#"},
    {"http://www.winehq.org/tests/?query=x/y/z", "tests15", 0, S_OK, "http://www.winehq.org/tests/tests15"},
    {"http://www.winehq.org/tests/?query=x/y/z#example", "tests16", 0, S_OK, "http://www.winehq.org/tests/tests16"},
    {"http://www.winehq.org/tests17", ".", 0, S_OK, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests18/test", ".", 0, S_OK, "http://www.winehq.org/tests18/"},
    {"http://www.winehq.org/tests19/test", "./", 0, S_OK, "http://www.winehq.org/tests19/", FALSE},
    {"http://www.winehq.org/tests20/test", "/", 0, S_OK, "http://www.winehq.org/", FALSE},
    {"http://www.winehq.org/tests/test", "./test21", 0, S_OK, "http://www.winehq.org/tests/test21", FALSE},
    {"http://www.winehq.org/tests/test", "./test22/../test", 0, S_OK, "http://www.winehq.org/tests/test", FALSE},
    {"http://www.winehq.org/tests/", "http://www.winehq.org:80/tests23", 0, S_OK, "http://www.winehq.org/tests23", TRUE},
    {"http://www.winehq.org/tests/", "tests24/./test/../test", 0, S_OK, "http://www.winehq.org/tests/tests24/test", FALSE},
    {"http://www.winehq.org/tests/./tests25", "./", 0, S_OK, "http://www.winehq.org/tests/", FALSE},
    {"file:///C:\\dir\\file.txt", "test.txt", 0, S_OK, "file:///C:/dir/test.txt"},
    {"file:///C:\\dir\\file.txt#hash\\hash", "test.txt", 0, S_OK, "file:///C:/dir/file.txt#hash/test.txt"},
    {"file:///C:\\dir\\file.html#hash\\hash", "test.html", 0, S_OK, "file:///C:/dir/test.html"},
    {"file:///C:\\dir\\file.htm#hash\\hash", "test.htm", 0, S_OK, "file:///C:/dir/test.htm"},
    {"file:///C:\\dir\\file.hTmL#hash\\hash", "test.hTmL", 0, S_OK, "file:///C:/dir/test.hTmL"},
    {"file:///C:\\dir.html\\file.txt#hash\\hash", "test.txt", 0, S_OK, "file:///C:/dir.html/file.txt#hash/test.txt"},
    {"C:\\winehq\\winehq.txt", "C:\\Test\\test.txt", 0, S_OK, "file:///C:/Test/test.txt"},
    {"http://www.winehq.org/test/", "test%20file.txt", 0, S_OK, "http://www.winehq.org/test/test%20file.txt"},
    {"http://www.winehq.org/test/", "test%20file.txt", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org/test/test%20file.txt"},
    {"http://www.winehq.org%2ftest/", "test%20file.txt", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org%2ftest/test%20file.txt"},
    {"xxx:@MSITStore:file.chm/file.html", "dir/file", 0, S_OK, "xxx:dir/file"},
    {"mk:@MSITStore:file.chm::/file.html", "/dir/file", 0, S_OK, "mk:@MSITStore:file.chm::/dir/file"},
    {"mk:@MSITStore:file.chm::/file.html", "mk:@MSITStore:file.chm::/dir/file", 0, S_OK, "mk:@MSITStore:file.chm::/dir/file"},
    {"foo:today", "foo:calendar", 0, S_OK, "foo:calendar"},
    {"foo:today", "bar:calendar", 0, S_OK, "bar:calendar"},
    {"foo:/today", "foo:calendar", 0, S_OK, "foo:/calendar"},
    {"Foo:/today/", "fOo:calendar", 0, S_OK, "foo:/today/calendar"},
    {"mk:@MSITStore:dir/test.chm::dir/index.html", "image.jpg", 0, S_OK, "mk:@MSITStore:dir/test.chm::dir/image.jpg"},
    {"mk:@MSITStore:dir/test.chm::dir/dir2/index.html", "../image.jpg", 0, S_OK, "mk:@MSITStore:dir/test.chm::dir/image.jpg"},
    /* UrlCombine case 2 tests.  Schemes do not match */
    {"outbind://xxxxxxxxx","http://wine1/dir",0, S_OK,"http://wine1/dir"},
    {"xxxx://xxxxxxxxx","http://wine2/dir",0, S_OK,"http://wine2/dir"},
    {"ftp://xxxxxxxxx/","http://wine3/dir",0, S_OK,"http://wine3/dir"},
    {"outbind://xxxxxxxxx","http://wine4/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"http://wine4/dir"},
    {"xxx://xxxxxxxxx","http://wine5/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"http://wine5/dir"},
    {"ftp://xxxxxxxxx/","http://wine6/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"http://wine6/dir"},
    {"http://xxxxxxxxx","outbind://wine7/dir",0, S_OK,"outbind://wine7/dir"},
    {"xxx://xxxxxxxxx","ftp://wine8/dir",0, S_OK,"ftp://wine8/dir"},
    {"ftp://xxxxxxxxx/","xxx://wine9/dir",0, S_OK,"xxx://wine9/dir"},
    {"http://xxxxxxxxx","outbind://wine10/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"outbind://wine10/dir"},
    {"xxx://xxxxxxxxx","ftp://wine11/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"ftp://wine11/dir"},
    {"ftp://xxxxxxxxx/","xxx://wine12/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"xxx://wine12/dir"},
    {"http://xxxxxxxxx","outbind:wine13/dir",0, S_OK,"outbind:wine13/dir"},
    {"xxx://xxxxxxxxx","ftp:wine14/dir",0, S_OK,"ftp:wine14/dir"},
    {"ftp://xxxxxxxxx/","xxx:wine15/dir",0, S_OK,"xxx:wine15/dir"},
    {"outbind://xxxxxxxxx/","http:wine16/dir",0, S_OK,"http:wine16/dir"},
    {"http://xxxxxxxxx","outbind:wine17/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"outbind:wine17/dir"},
    {"xxx://xxxxxxxxx","ftp:wine18/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"ftp:wine18/dir"},
    {"ftp://xxxxxxxxx/","xXx:wine19/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"xxx:wine19/dir"},
    {"outbind://xxxxxxxxx/","http:wine20/dir",URL_PLUGGABLE_PROTOCOL, S_OK,"http:wine20/dir"},
    {"file:///c:/dir/file.txt","index.html?test=c:/abc",URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,S_OK,"file:///c:/dir/index.html?test=c:/abc"}
};

/* ################ */

static const struct {
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
    {"file:///c:/foo/bar", "file:///c:/foo/bar", S_FALSE},
#if 0
    /* The following test fails on native shlwapi as distributed with Win95/98.
     * Wine matches the behaviour of later versions.
     */
    {"xx:c:\\foo\\bar", "xx:c:\\foo\\bar", S_FALSE}
#endif
};

/* ################ */

static struct {
    char url[30];
    const char *expect;
} TEST_URL_UNESCAPE[] = {
    {"file://foo/bar", "file://foo/bar"},
    {"file://fo%20o%5Ca/bar", "file://fo o\\a/bar"},
    {"file://%24%25foobar", "file://$%foobar"}
};

/* ################ */

static const  struct {
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

/* ################ */

static const struct {
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
    {	"file:partial",					FALSE,	TRUE	},
    {	"File:partial",					FALSE,	TRUE	},
};

/* ########################### */

static LPWSTR GetWideString(const char* szString)
{
  LPWSTR wszString = HeapAlloc(GetProcessHeap(), 0, (2*INTERNET_MAX_URL_LENGTH) * sizeof(WCHAR));

  MultiByteToWideChar(CP_ACP, 0, szString, -1, wszString, INTERNET_MAX_URL_LENGTH);

  return wszString;
}


static void FreeWideString(LPWSTR wszString)
{
   HeapFree(GetProcessHeap(), 0, wszString);
}

/* ########################### */

static void test_UrlApplyScheme(void)
{
    CHAR newurl[TEST_APPLY_MAX_LENGTH];
    WCHAR urlW[TEST_APPLY_MAX_LENGTH];
    WCHAR newurlW[TEST_APPLY_MAX_LENGTH];
    HRESULT res;
    DWORD len;
    DWORD i;

    if (!pUrlApplySchemeA) {
        win_skip("UrlApplySchemeA not found\n");
        return;
    }

    for(i = 0; i < sizeof(TEST_APPLY)/sizeof(TEST_APPLY[0]); i++) {
        len = TEST_APPLY_MAX_LENGTH;
        lstrcpyA(newurl, untouchedA);
        res = pUrlApplySchemeA(TEST_APPLY[i].url, newurl, &len, TEST_APPLY[i].flags);
        ok( res == TEST_APPLY[i].res,
            "#%dA: got HRESULT 0x%x (expected 0x%x)\n", i, res, TEST_APPLY[i].res);

        ok( len == TEST_APPLY[i].newlen,
            "#%dA: got len %d (expected %d)\n", i, len, TEST_APPLY[i].newlen);

        ok( !lstrcmpA(newurl, TEST_APPLY[i].newurl),
            "#%dA: got '%s' (expected '%s')\n", i, newurl, TEST_APPLY[i].newurl);

        /* returned length is in character */
        len = TEST_APPLY_MAX_LENGTH;
        lstrcpyA(newurl, untouchedA);
        MultiByteToWideChar(CP_ACP, 0, newurl, -1, newurlW, len);
        MultiByteToWideChar(CP_ACP, 0, TEST_APPLY[i].url, -1, urlW, len);

        res = pUrlApplySchemeW(urlW, newurlW, &len, TEST_APPLY[i].flags);
        WideCharToMultiByte(CP_ACP, 0, newurlW, -1, newurl, TEST_APPLY_MAX_LENGTH, NULL, NULL);
        ok( res == TEST_APPLY[i].res,
            "#%dW: got HRESULT 0x%x (expected 0x%x)\n", i, res, TEST_APPLY[i].res);

        ok( len == TEST_APPLY[i].newlen,
            "#%dW: got len %d (expected %d)\n", i, len, TEST_APPLY[i].newlen);

        ok( !lstrcmpA(newurl, TEST_APPLY[i].newurl),
            "#%dW: got '%s' (expected '%s')\n", i, newurl, TEST_APPLY[i].newurl);

    }

    /* buffer too small */
    lstrcpyA(newurl, untouchedA);
    len = lstrlenA(TEST_APPLY[0].newurl);
    res = pUrlApplySchemeA(TEST_APPLY[0].url, newurl, &len, TEST_APPLY[0].flags);
    ok(res == E_POINTER, "got HRESULT 0x%x (expected E_POINTER)\n", res);
    /* The returned length include the space for the terminating 0 */
    i = lstrlenA(TEST_APPLY[0].newurl)+1;
    ok(len == i, "got len %d (expected %d)\n", len, i);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

    /* NULL as parameter. The length and the buffer are not modified */
    lstrcpyA(newurl, untouchedA);
    len = TEST_APPLY_MAX_LENGTH;
    res = pUrlApplySchemeA(NULL, newurl, &len, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%x (expected E_INVALIDARG)\n", res);
    ok(len == TEST_APPLY_MAX_LENGTH, "got len %d\n", len);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

    len = TEST_APPLY_MAX_LENGTH;
    res = pUrlApplySchemeA(TEST_APPLY[0].url, NULL, &len, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%x (expected E_INVALIDARG)\n", res);
    ok(len == TEST_APPLY_MAX_LENGTH, "got len %d\n", len);

    lstrcpyA(newurl, untouchedA);
    res = pUrlApplySchemeA(TEST_APPLY[0].url, newurl, NULL, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%x (expected E_INVALIDARG)\n", res);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

}

/* ########################### */

static void hash_url(const char* szUrl)
{
  LPCSTR szTestUrl = szUrl;
  LPWSTR wszTestUrl = GetWideString(szTestUrl);
  HRESULT res;

  DWORD cbSize = sizeof(DWORD);
  DWORD dwHash1, dwHash2;
  res = pUrlHashA(szTestUrl, (LPBYTE)&dwHash1, cbSize);
  ok(res == S_OK, "UrlHashA returned 0x%x (expected S_OK) for %s\n", res, szUrl);
  if (pUrlHashW) {
    res = pUrlHashW(wszTestUrl, (LPBYTE)&dwHash2, cbSize);
    ok(res == S_OK, "UrlHashW returned 0x%x (expected S_OK) for %s\n", res, szUrl);
    ok(dwHash1 == dwHash2,
        "Hashes didn't match (A: 0x%x, W: 0x%x) for %s\n", dwHash1, dwHash2, szUrl);
  }
  FreeWideString(wszTestUrl);

}

static void test_UrlHash(void)
{
  if (!pUrlHashA) {
    win_skip("UrlHashA not found\n");
    return;
  }

  hash_url(TEST_URL_1);
  hash_url(TEST_URL_2);
  hash_url(TEST_URL_3);
}

/* ########################### */

static void test_url_part(const char* szUrl, DWORD dwPart, DWORD dwFlags, const char* szExpected)
{
  CHAR szPart[INTERNET_MAX_URL_LENGTH];
  WCHAR wszPart[INTERNET_MAX_URL_LENGTH];
  LPWSTR wszUrl = GetWideString(szUrl);
  LPWSTR wszConvertedPart;
  HRESULT res;
  DWORD dwSize;

  dwSize = 1;
  res = pUrlGetPartA(szUrl, szPart, &dwSize, dwPart, dwFlags);
  ok(res == E_POINTER, "UrlGetPart for \"%s\" gave: 0x%08x\n", szUrl, res);
  ok(dwSize == strlen(szExpected)+1 ||
          (*szExpected == '?' && dwSize == strlen(szExpected)),
          "UrlGetPart for \"%s\" gave size: %u\n", szUrl, dwSize);

  dwSize = INTERNET_MAX_URL_LENGTH;
  res = pUrlGetPartA(szUrl, szPart, &dwSize, dwPart, dwFlags);
  ok(res == S_OK,
    "UrlGetPartA for \"%s\" part 0x%08x returned 0x%x and \"%s\"\n",
    szUrl, dwPart, res, szPart);
  if (pUrlGetPartW) {
    dwSize = INTERNET_MAX_URL_LENGTH;
    res = pUrlGetPartW(wszUrl, wszPart, &dwSize, dwPart, dwFlags);
    ok(res == S_OK,
      "UrlGetPartW for \"%s\" part 0x%08x returned 0x%x\n",
      szUrl, dwPart, res);

    wszConvertedPart = GetWideString(szPart);

    ok(lstrcmpW(wszPart,wszConvertedPart)==0,
        "Strings didn't match between ascii and unicode UrlGetPart!\n");

    FreeWideString(wszConvertedPart);
  }
  FreeWideString(wszUrl);

  /* Note that v6.0 and later don't return '?' with the query */
  ok(strcmp(szPart,szExpected)==0 ||
     (*szExpected=='?' && !strcmp(szPart,szExpected+1)),
	 "Expected %s, but got %s\n", szExpected, szPart);
}

/* ########################### */

static void test_UrlGetPart(void)
{
  const char* file_url = "file://h o s t/c:/windows/file";
  const char* http_url = "http://user:pass 123@www.wine hq.org";
  const char* res_url = "res://some.dll/find.dlg";
  const char* about_url = "about:blank";
  const char* excid_url = "x-excid://36C00000/guid:{048B4E89-2E92-496F-A837-33BA02FF6D32}/Message.htm";
  const char* foo_url = "foo://bar-url/test";
  const char* short_url = "ascheme:";

  CHAR szPart[INTERNET_MAX_URL_LENGTH];
  DWORD dwSize;
  HRESULT res;

  if (!pUrlGetPartA) {
    win_skip("UrlGetPartA not found\n");
    return;
  }

  res = pUrlGetPartA(NULL, NULL, NULL, URL_PART_SCHEME, 0);
  ok(res == E_INVALIDARG, "null params gave: 0x%08x\n", res);

  res = pUrlGetPartA(NULL, szPart, &dwSize, URL_PART_SCHEME, 0);
  ok(res == E_INVALIDARG, "null URL gave: 0x%08x\n", res);

  res = pUrlGetPartA(res_url, NULL, &dwSize, URL_PART_SCHEME, 0);
  ok(res == E_INVALIDARG, "null szPart gave: 0x%08x\n", res);

  res = pUrlGetPartA(res_url, szPart, NULL, URL_PART_SCHEME, 0);
  ok(res == E_INVALIDARG, "null URL gave: 0x%08x\n", res);

  dwSize = 0;
  szPart[0]='x'; szPart[1]=0;
  res = pUrlGetPartA("hi", szPart, &dwSize, URL_PART_SCHEME, 0);
  ok(res == E_INVALIDARG, "UrlGetPartA(*pcchOut = 0) returned %08X\n", res);
  ok(szPart[0] == 'x' && szPart[1] == 0, "UrlGetPartA(*pcchOut = 0) modified szPart: \"%s\"\n", szPart);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  dwSize = sizeof szPart;
  szPart[0]='x'; szPart[1]=0;
  res = pUrlGetPartA("hi", szPart, &dwSize, URL_PART_SCHEME, 0);
  ok (res==S_FALSE, "UrlGetPartA(\"hi\") returned %08X\n", res);
  ok(szPart[0]==0, "UrlGetPartA(\"hi\") return \"%s\" instead of \"\"\n", szPart);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  if(pUrlGetPartW)
  {
      const WCHAR hiW[] = {'h','i',0};
      WCHAR bufW[5];

      /* UrlGetPartW returns S_OK instead of S_FALSE */
      dwSize = sizeof szPart;
      bufW[0]='x'; bufW[1]=0;
      res = pUrlGetPartW(hiW, bufW, &dwSize, URL_PART_SCHEME, 0);
      todo_wine ok(res==S_OK, "UrlGetPartW(\"hi\") returned %08X\n", res);
      ok(bufW[0] == 0, "UrlGetPartW(\"hi\") return \"%c\"\n", bufW[0]);
      ok(dwSize == 0, "dwSize = %d\n", dwSize);
  }

  dwSize = sizeof szPart;
  szPart[0]='x'; szPart[1]=0;
  res = pUrlGetPartA("hi", szPart, &dwSize, URL_PART_QUERY, 0);
  ok (res==S_FALSE, "UrlGetPartA(\"hi\") returned %08X\n", res);
  ok(szPart[0]==0, "UrlGetPartA(\"hi\") return \"%s\" instead of \"\"\n", szPart);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  test_url_part(TEST_URL_3, URL_PART_HOSTNAME, 0, "localhost");
  test_url_part(TEST_URL_3, URL_PART_PORT, 0, "21");
  test_url_part(TEST_URL_3, URL_PART_USERNAME, 0, "foo");
  test_url_part(TEST_URL_3, URL_PART_PASSWORD, 0, "bar");
  test_url_part(TEST_URL_3, URL_PART_SCHEME, 0, "http");
  test_url_part(TEST_URL_3, URL_PART_QUERY, 0, "?query=x&return=y");

  test_url_part(TEST_URL_4, URL_PART_HOSTNAME, 0, "google.*.com");

  test_url_part(file_url, URL_PART_HOSTNAME, 0, "h o s t");

  test_url_part(http_url, URL_PART_HOSTNAME, 0, "www.wine hq.org");
  test_url_part(http_url, URL_PART_PASSWORD, 0, "pass 123");

  test_url_part(about_url, URL_PART_SCHEME, 0, "about");

  test_url_part(excid_url, URL_PART_SCHEME, 0, "x-excid");
  test_url_part(foo_url, URL_PART_SCHEME, 0, "foo");
  test_url_part(short_url, URL_PART_SCHEME, 0, "ascheme");

  dwSize = sizeof(szPart);
  res = pUrlGetPartA(about_url, szPart, &dwSize, URL_PART_HOSTNAME, 0);
  ok(res==E_FAIL, "returned %08x\n", res);

  test_url_part(res_url, URL_PART_SCHEME, 0, "res");
  test_url_part("http://www.winehq.org", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, "http:www.winehq.org");

  dwSize = sizeof szPart;
  szPart[0]='x'; szPart[1]=0;
  res = pUrlGetPartA(res_url, szPart, &dwSize, URL_PART_QUERY, 0);
  ok(res==S_FALSE, "UrlGetPartA returned %08X\n", res);
  ok(szPart[0]==0, "UrlGetPartA gave \"%s\" instead of \"\"\n", szPart);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  dwSize = sizeof(szPart);
  res = pUrlGetPartA("file://c:\\index.htm", szPart, &dwSize, URL_PART_HOSTNAME, 0);
  ok(res==S_FALSE, "returned %08x\n", res);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  dwSize = sizeof(szPart);
  szPart[0] = 'x'; szPart[1] = '\0';
  res = pUrlGetPartA("file:some text", szPart, &dwSize, URL_PART_HOSTNAME, 0);
  ok(res==S_FALSE, "returned %08x\n", res);
  ok(szPart[0] == '\0', "szPart[0] = %c\n", szPart[0]);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  dwSize = sizeof(szPart);
  szPart[0] = 'x'; szPart[1] = '\0';
  res = pUrlGetPartA("index.htm", szPart, &dwSize, URL_PART_HOSTNAME, 0);
  ok(res==E_FAIL, "returned %08x\n", res);

  dwSize = sizeof(szPart);
  szPart[0] = 'x'; szPart[1] = '\0';
  res = pUrlGetPartA(excid_url, szPart, &dwSize, URL_PART_HOSTNAME, 0);
  ok(res==E_FAIL, "returned %08x\n", res);
  ok(szPart[0] == 'x', "szPart[0] = %c\n", szPart[0]);
  ok(dwSize == sizeof(szPart), "dwSize = %d\n", dwSize);

  dwSize = sizeof(szPart);
  szPart[0] = 'x'; szPart[1] = '\0';
  res = pUrlGetPartA(excid_url, szPart, &dwSize, URL_PART_QUERY, 0);
  ok(res==S_FALSE, "returned %08x\n", res);
  ok(szPart[0] == 0, "szPart[0] = %c\n", szPart[0]);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);

  dwSize = sizeof(szPart);
  szPart[0] = 'x'; szPart[1] = '\0';
  res = pUrlGetPartA(foo_url, szPart, &dwSize, URL_PART_HOSTNAME, 0);
  ok(res==E_FAIL, "returned %08x\n", res);
  ok(szPart[0] == 'x', "szPart[0] = %c\n", szPart[0]);
  ok(dwSize == sizeof(szPart), "dwSize = %d\n", dwSize);

  dwSize = sizeof(szPart);
  szPart[0] = 'x'; szPart[1] = '\0';
  res = pUrlGetPartA(foo_url, szPart, &dwSize, URL_PART_QUERY, 0);
  ok(res==S_FALSE, "returned %08x\n", res);
  ok(szPart[0] == 0, "szPart[0] = %c\n", szPart[0]);
  ok(dwSize == 0, "dwSize = %d\n", dwSize);
}

/* ########################### */
static void test_url_canonicalize(int index, const char *szUrl, DWORD dwFlags, HRESULT dwExpectReturn, HRESULT dwExpectReturnAlt, const char *szExpectUrl, BOOL todo)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl = GetWideString(szUrl);
    LPWSTR wszExpectUrl = GetWideString(szExpectUrl);
    LPWSTR wszConvertedUrl;
    HRESULT ret;

    DWORD dwSize;

    dwSize = INTERNET_MAX_URL_LENGTH;
    ret = pUrlCanonicalizeA(szUrl, NULL, &dwSize, dwFlags);
    ok(ret != dwExpectReturn, "got 0s%x: Unexpected return for NULL buffer, index %d\n", ret, index);
    ret = pUrlCanonicalizeA(szUrl, szReturnUrl, &dwSize, dwFlags);
    ok(ret == dwExpectReturn || ret == dwExpectReturnAlt,
       "UrlCanonicalizeA failed: expected=0x%08x or 0x%08x, got=0x%08x, index %d\n",
       dwExpectReturn, dwExpectReturnAlt, ret, index);
    if (todo)
        todo_wine
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "UrlCanonicalizeA dwFlags 0x%08x url '%s' Expected \"%s\", but got \"%s\", index %d\n", dwFlags, szUrl, szExpectUrl, szReturnUrl, index);
    else
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "UrlCanonicalizeA dwFlags 0x%08x url '%s' Expected \"%s\", but got \"%s\", index %d\n", dwFlags, szUrl, szExpectUrl, szReturnUrl, index);

    if (pUrlCanonicalizeW) {
        dwSize = INTERNET_MAX_URL_LENGTH;
        ret = pUrlCanonicalizeW(wszUrl, NULL, &dwSize, dwFlags);
        ok(ret != dwExpectReturn, "got 0x%x: Unexpected return for NULL buffer, index %d\n", ret, index);
        ret = pUrlCanonicalizeW(wszUrl, wszReturnUrl, &dwSize, dwFlags);
        ok(ret == dwExpectReturn, "UrlCanonicalizeW failed: expected 0x%08x, got 0x%x, index %d\n",
            dwExpectReturn, ret, index);

        wszConvertedUrl = GetWideString(szReturnUrl);
        ok(lstrcmpW(wszReturnUrl, wszConvertedUrl)==0,
            "Strings didn't match between ascii and unicode UrlCanonicalize, index %d!\n", index);
        FreeWideString(wszConvertedUrl);
    }

    FreeWideString(wszUrl);
    FreeWideString(wszExpectUrl);
}


static void test_UrlEscapeA(void)
{
    DWORD size = 0;
    HRESULT ret;
    unsigned int i;
    char empty_string[] = "";

    if (!pUrlEscapeA) {
        win_skip("UrlEscapeA not found\n");
        return;
    }

    ret = pUrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 0, "got %d, expected %d\n", size, 0);

    size = 0;
    ret = pUrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 0, "got %d, expected %d\n", size, 0);

    size = 1;
    ret = pUrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 1, "got %d, expected %d\n", size, 1);

    size = 1;
    ret = pUrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 1, "got %d, expected %d\n", size, 1);

    size = 1;
    empty_string[0] = 127;
    ret = pUrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_POINTER, "got %x, expected %x\n", ret, E_POINTER);
    ok(size == 34, "got %d, expected %d\n", size, 34);
    ok(empty_string[0] == 127, "String has changed, empty_string[0] = %d\n", empty_string[0]);

    for(i=0; i<sizeof(TEST_ESCAPE)/sizeof(TEST_ESCAPE[0]); i++) {
        CHAR ret_url[INTERNET_MAX_URL_LENGTH];

        size = INTERNET_MAX_URL_LENGTH;
        ret = pUrlEscapeA(TEST_ESCAPE[i].url, ret_url, &size, TEST_ESCAPE[i].flags);
        ok(ret == TEST_ESCAPE[i].expectret, "UrlEscapeA returned 0x%08x instead of 0x%08x for \"%s\"\n",
            ret, TEST_ESCAPE[i].expectret, TEST_ESCAPE[i].url);
        ok(!strcmp(ret_url, TEST_ESCAPE[i].expecturl), "Expected \"%s\", but got \"%s\" for \"%s\"\n",
            TEST_ESCAPE[i].expecturl, ret_url, TEST_ESCAPE[i].url);
    }
}

static void test_UrlEscapeW(void)
{
    static const WCHAR naW[] = {'f','t','p',31,255,250,0x2122,'e','n','d','/',0};
    static const WCHAR naescapedW[] = {'f','t','p','%','1','F','%','F','F','%','F','A',0x2122,'e','n','d','/',0};
    static const WCHAR out[] = {'f','o','o','%','2','0','b','a','r',0};
    WCHAR overwrite[] = {'f','o','o',' ','b','a','r',0,0,0};
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    DWORD size = 0;
    HRESULT ret;
    WCHAR wc;
    int i;

    if (!pUrlEscapeW) {
        win_skip("UrlEscapeW not found\n");
        return;
    }

    size = sizeof(overwrite)/sizeof(WCHAR);
    ret = pUrlEscapeW(overwrite, overwrite, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == S_OK, "got %x, expected S_OK\n", ret);
    ok(size == 9, "got %d, expected 9\n", size);
    ok(!lstrcmpW(overwrite, out), "got %s, expected %s\n", wine_dbgstr_w(overwrite), wine_dbgstr_w(out));

    size = 1;
    wc = 127;
    ret = pUrlEscapeW(overwrite, &wc, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_POINTER, "got %x, expected %x\n", ret, E_POINTER);
    ok(size == 10, "got %d, expected 10\n", size);
    ok(wc == 127, "String has changed, wc = %d\n", wc);

    /* non-ASCII range */
    size = sizeof(ret_urlW)/sizeof(WCHAR);
    ret = pUrlEscapeW(naW, ret_urlW, &size, 0);
    ok(ret == S_OK, "got %x, expected S_OK\n", ret);
    ok(!lstrcmpW(naescapedW, ret_urlW), "got %s, expected %s\n", wine_dbgstr_w(ret_urlW), wine_dbgstr_w(naescapedW));

    for (i = 0; i < sizeof(TEST_ESCAPE)/sizeof(TEST_ESCAPE[0]); i++) {

        WCHAR *urlW, *expected_urlW;

        size = INTERNET_MAX_URL_LENGTH;
        urlW = GetWideString(TEST_ESCAPE[i].url);
        expected_urlW = GetWideString(TEST_ESCAPE[i].expecturl);
        ret = pUrlEscapeW(urlW, ret_urlW, &size, TEST_ESCAPE[i].flags);
        ok(ret == TEST_ESCAPE[i].expectret, "UrlEscapeW returned 0x%08x instead of 0x%08x for %s\n",
           ret, TEST_ESCAPE[i].expectret, wine_dbgstr_w(urlW));
        ok(!lstrcmpW(ret_urlW, expected_urlW), "Expected %s, but got %s for %s flags %08x\n",
            wine_dbgstr_w(expected_urlW), wine_dbgstr_w(ret_urlW), wine_dbgstr_w(urlW), TEST_ESCAPE[i].flags);
        FreeWideString(urlW);
        FreeWideString(expected_urlW);
    }
}

/* ########################### */

static void test_UrlCanonicalizeA(void)
{
    unsigned int i;
    CHAR szReturnUrl[4*INTERNET_MAX_URL_LENGTH];
    CHAR longurl[4*INTERNET_MAX_URL_LENGTH];
    DWORD dwSize;
    DWORD urllen;
    HRESULT hr;

    if (!pUrlCanonicalizeA) {
        win_skip("UrlCanonicalizeA not found\n");
        return;
    }

    urllen = lstrlenA(winehqA);

    /* buffer has no space for the result */
    dwSize=urllen-1;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY  | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'E_POINTER' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen+1);

    /* buffer has no space for the terminating '\0' */
    dwSize=urllen;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'E_POINTER' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen+1);

    /* buffer has the required size */
    dwSize=urllen+1;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'S_OK' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen);

    /* buffer is larger as the required size */
    dwSize=urllen+2;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'S_OK' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen);

    /* length is set to 0 */
    dwSize=0;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_INVALIDARG) && (dwSize == 0),
            "got 0x%x with %u and size %u for '%s' and %u (expected 'E_INVALIDARG' and size %u)\n",
            hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), 0);

    /* url length > INTERNET_MAX_URL_SIZE */
    dwSize=sizeof(szReturnUrl);
    memset(longurl, 'a', sizeof(longurl));
    memcpy(longurl, winehqA, sizeof(winehqA)-1);
    longurl[sizeof(longurl)-1] = '\0';
    hr = pUrlCanonicalizeA(longurl, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok(hr == S_OK, "hr = %x\n", hr);

    test_url_canonicalize(-1, "", 0, S_OK, S_FALSE /* Vista/win2k8 */, "", FALSE);

    /* test url-modification */
    for(i=0; i<sizeof(TEST_CANONICALIZE)/sizeof(TEST_CANONICALIZE[0]); i++) {
        test_url_canonicalize(i, TEST_CANONICALIZE[i].url, TEST_CANONICALIZE[i].flags,
                              TEST_CANONICALIZE[i].expectret, TEST_CANONICALIZE[i].expectret, TEST_CANONICALIZE[i].expecturl,
                              TEST_CANONICALIZE[i].todo);
    }
}

/* ########################### */

static void test_UrlCanonicalizeW(void)
{
    WCHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwSize;
    DWORD urllen;
    HRESULT hr;
    int i;


    if (!pUrlCanonicalizeW) {
        win_skip("UrlCanonicalizeW not found\n");
        return;
    }
    urllen = lstrlenW(winehqW);

    /* buffer has no space for the result */
    dwSize = (urllen-1);
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%x with %u and size %u for %u (expected 'E_POINTER' and size %u)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen+1);


    /* buffer has no space for the terminating '\0' */
    dwSize = urllen;
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%x with %u and size %u for %u (expected 'E_POINTER' and size %u)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen+1);

    /* buffer has the required size */
    dwSize = urllen +1;
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%x with %u and size %u for %u (expected 'S_OK' and size %u)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen);

    /* buffer is larger as the required size */
    dwSize = (urllen+2);
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = pUrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%x with %u and size %u for %u (expected 'S_OK' and size %u)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen);

    /* check that the characters 1..32 are chopped from the end of the string */
    for (i = 1; i < 65536; i++)
    {
        WCHAR szUrl[128];
        BOOL choped;
        int pos;

        MultiByteToWideChar(CP_ACP, 0, "http://www.winehq.org/X", -1, szUrl, sizeof(szUrl)/sizeof(szUrl[0]));
        pos = lstrlenW(szUrl) - 1;
        szUrl[pos] = i;
        urllen = INTERNET_MAX_URL_LENGTH;
        pUrlCanonicalizeW(szUrl, szReturnUrl, &urllen, 0);
        choped = lstrlenW(szReturnUrl) < lstrlenW(szUrl);
        ok(choped == (i <= 32), "Incorrect char chopping for char %d\n", i);
    }
}

/* ########################### */

static void test_url_combine(const char *szUrl1, const char *szUrl2, DWORD dwFlags, HRESULT dwExpectReturn, const char *szExpectUrl, BOOL todo)
{
    HRESULT hr;
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl1, wszUrl2, wszExpectUrl, wszConvertedUrl;

    DWORD dwSize;
    DWORD dwExpectLen = lstrlenA(szExpectUrl);

    if (!pUrlCombineA) {
        win_skip("UrlCombineA not found\n");
        return;
    }

    wszUrl1 = GetWideString(szUrl1);
    wszUrl2 = GetWideString(szUrl2);
    wszExpectUrl = GetWideString(szExpectUrl);

    hr = pUrlCombineA(szUrl1, szUrl2, NULL, NULL, dwFlags);
    ok(hr == E_INVALIDARG, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, E_INVALIDARG);

    dwSize = 0;
    hr = pUrlCombineA(szUrl1, szUrl2, NULL, &dwSize, dwFlags);
    ok(hr == E_POINTER, "Checking length of string, return was 0x%08x, expected 0x%08x\n", hr, E_POINTER);
    ok(todo || dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);

    dwSize--;
    hr = pUrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == E_POINTER, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, E_POINTER);
    ok(todo || dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);

    hr = pUrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == dwExpectReturn, "UrlCombineA returned 0x%08x, expected 0x%08x\n", hr, dwExpectReturn);

    if (todo)
    {
        todo_wine ok(dwSize == dwExpectLen && (FAILED(hr) || strcmp(szReturnUrl, szExpectUrl)==0),
                "Expected %s (len=%d), but got %s (len=%d)\n", szExpectUrl, dwExpectLen, SUCCEEDED(hr) ? szReturnUrl : "(null)", dwSize);
    }
    else
    {
        ok(dwSize == dwExpectLen, "Got length %d, expected %d\n", dwSize, dwExpectLen);
        if (SUCCEEDED(hr))
            ok(strcmp(szReturnUrl, szExpectUrl)==0, "Expected %s, but got %s\n", szExpectUrl, szReturnUrl);
    }

    if (pUrlCombineW) {
        dwSize = 0;
        hr = pUrlCombineW(wszUrl1, wszUrl2, NULL, &dwSize, dwFlags);
        ok(hr == E_POINTER, "Checking length of string, return was 0x%08x, expected 0x%08x\n", hr, E_POINTER);
        ok(todo || dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);

        dwSize--;
        hr = pUrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
        ok(hr == E_POINTER, "UrlCombineW returned 0x%08x, expected 0x%08x\n", hr, E_POINTER);
        ok(todo || dwSize == dwExpectLen+1, "Got length %d, expected %d\n", dwSize, dwExpectLen+1);

        hr = pUrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
        ok(hr == dwExpectReturn, "UrlCombineW returned 0x%08x, expected 0x%08x\n", hr, dwExpectReturn);
        ok(todo || dwSize == dwExpectLen, "Got length %d, expected %d\n", dwSize, dwExpectLen);
        if(SUCCEEDED(hr)) {
            wszConvertedUrl = GetWideString(szReturnUrl);
            ok(lstrcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCombine!\n");
            FreeWideString(wszConvertedUrl);
        }
    }

    FreeWideString(wszUrl1);
    FreeWideString(wszUrl2);
    FreeWideString(wszExpectUrl);
}

/* ########################### */

static void test_UrlCombine(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_COMBINE)/sizeof(TEST_COMBINE[0]); i++) {
        test_url_combine(TEST_COMBINE[i].url1, TEST_COMBINE[i].url2, TEST_COMBINE[i].flags,
                         TEST_COMBINE[i].expectret, TEST_COMBINE[i].expecturl, TEST_COMBINE[i].todo);
    }
}

/* ########################### */

static void test_UrlCreateFromPath(void)
{
    size_t i;
    char ret_url[INTERNET_MAX_URL_LENGTH];
    DWORD len, ret;
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR *pathW, *urlW;

    if (!pUrlCreateFromPathA) {
        win_skip("UrlCreateFromPathA not found\n");
        return;
    }

    for(i = 0; i < sizeof(TEST_URLFROMPATH) / sizeof(TEST_URLFROMPATH[0]); i++) {
        len = INTERNET_MAX_URL_LENGTH;
        ret = pUrlCreateFromPathA(TEST_URLFROMPATH[i].path, ret_url, &len, 0);
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08x from path %s\n", ret, TEST_URLFROMPATH[i].path);
        ok(!lstrcmpiA(ret_url, TEST_URLFROMPATH[i].url), "url %s from path %s\n", ret_url, TEST_URLFROMPATH[i].path);
        ok(len == strlen(ret_url), "ret len %d from path %s\n", len, TEST_URLFROMPATH[i].path);

        if (pUrlCreateFromPathW) {
            len = INTERNET_MAX_URL_LENGTH;
            pathW = GetWideString(TEST_URLFROMPATH[i].path);
            urlW = GetWideString(TEST_URLFROMPATH[i].url);
            ret = pUrlCreateFromPathW(pathW, ret_urlW, &len, 0);
            WideCharToMultiByte(CP_ACP, 0, ret_urlW, -1, ret_url, sizeof(ret_url),0,0);
            ok(ret == TEST_URLFROMPATH[i].ret, "ret %08x from path L\"%s\", expected %08x\n",
                ret, TEST_URLFROMPATH[i].path, TEST_URLFROMPATH[i].ret);
            ok(!lstrcmpiW(ret_urlW, urlW), "got %s expected %s from path L\"%s\"\n",
                ret_url, TEST_URLFROMPATH[i].url, TEST_URLFROMPATH[i].path);
            ok(len == lstrlenW(ret_urlW), "ret len %d from path L\"%s\"\n", len, TEST_URLFROMPATH[i].path);
            FreeWideString(urlW);
            FreeWideString(pathW);
        }
    }
}

/* ########################### */

static void test_UrlIs_null(DWORD flag)
{
    BOOL ret;
    ret = pUrlIsA(NULL, flag);
    ok(ret == FALSE, "pUrlIsA(NULL, %d) failed\n", flag);
    ret = pUrlIsW(NULL, flag);
    ok(ret == FALSE, "pUrlIsW(NULL, %d) failed\n", flag);
}

static void test_UrlIs(void)
{
    BOOL ret;
    size_t i;
    WCHAR wurl[80];

    if (!pUrlIsA) {
        win_skip("UrlIsA not found\n");
        return;
    }

    test_UrlIs_null(URLIS_APPLIABLE);
    test_UrlIs_null(URLIS_DIRECTORY);
    test_UrlIs_null(URLIS_FILEURL);
    test_UrlIs_null(URLIS_HASQUERY);
    test_UrlIs_null(URLIS_NOHISTORY);
    test_UrlIs_null(URLIS_OPAQUE);
    test_UrlIs_null(URLIS_URL);

    for(i = 0; i < sizeof(TEST_PATH_IS_URL) / sizeof(TEST_PATH_IS_URL[0]); i++) {
	MultiByteToWideChar(CP_ACP, 0, TEST_PATH_IS_URL[i].path, -1, wurl, sizeof(wurl)/sizeof(*wurl));

        ret = pUrlIsA( TEST_PATH_IS_URL[i].path, URLIS_URL );
        ok( ret == TEST_PATH_IS_URL[i].expect,
            "returned %d from path %s, expected %d\n", ret, TEST_PATH_IS_URL[i].path,
            TEST_PATH_IS_URL[i].expect );

        if (pUrlIsW) {
            ret = pUrlIsW( wurl, URLIS_URL );
            ok( ret == TEST_PATH_IS_URL[i].expect,
                "returned %d from path (UrlIsW) %s, expected %d\n", ret,
                TEST_PATH_IS_URL[i].path, TEST_PATH_IS_URL[i].expect );
        }
    }
    for(i = 0; i < sizeof(TEST_URLIS_ATTRIBS) / sizeof(TEST_URLIS_ATTRIBS[0]); i++) {
	MultiByteToWideChar(CP_ACP, 0, TEST_URLIS_ATTRIBS[i].url, -1, wurl, sizeof(wurl)/sizeof(*wurl));

        ret = pUrlIsA( TEST_URLIS_ATTRIBS[i].url, URLIS_OPAQUE);
	ok( ret == TEST_URLIS_ATTRIBS[i].expectOpaque,
	    "returned %d for URLIS_OPAQUE, url \"%s\", expected %d\n", ret, TEST_URLIS_ATTRIBS[i].url,
	    TEST_URLIS_ATTRIBS[i].expectOpaque );
        ret = pUrlIsA( TEST_URLIS_ATTRIBS[i].url, URLIS_FILEURL);
	ok( ret == TEST_URLIS_ATTRIBS[i].expectFile,
	    "returned %d for URLIS_FILEURL, url \"%s\", expected %d\n", ret, TEST_URLIS_ATTRIBS[i].url,
	    TEST_URLIS_ATTRIBS[i].expectFile );

        if (pUrlIsW) {
            ret = pUrlIsW( wurl, URLIS_OPAQUE);
            ok( ret == TEST_URLIS_ATTRIBS[i].expectOpaque,
                "returned %d for URLIS_OPAQUE (UrlIsW), url \"%s\", expected %d\n",
                ret, TEST_URLIS_ATTRIBS[i].url, TEST_URLIS_ATTRIBS[i].expectOpaque );
            ret = pUrlIsW( wurl, URLIS_FILEURL);
            ok( ret == TEST_URLIS_ATTRIBS[i].expectFile,
                "returned %d for URLIS_FILEURL (UrlIsW), url \"%s\", expected %d\n",
                ret, TEST_URLIS_ATTRIBS[i].url, TEST_URLIS_ATTRIBS[i].expectFile );
        }
    }
}

/* ########################### */

static void test_UrlUnescape(void)
{
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR *urlW, *expected_urlW;
    DWORD dwEscaped;
    size_t i;
    static char inplace[] = "file:///C:/Program%20Files";
    static char another_inplace[] = "file:///C:/Program%20Files";
    static const char expected[] = "file:///C:/Program Files";
    static WCHAR inplaceW[] = {'f','i','l','e',':','/','/','/','C',':','/','P','r','o','g','r','a','m',' ','F','i','l','e','s',0};
    static WCHAR another_inplaceW[] ={'f','i','l','e',':','/','/','/',
                'C',':','/','P','r','o','g','r','a','m','%','2','0','F','i','l','e','s',0};
    HRESULT res;

    if (!pUrlUnescapeA) {
        win_skip("UrlUnescapeA not found\n");
        return;
    }
    for(i=0; i<sizeof(TEST_URL_UNESCAPE)/sizeof(TEST_URL_UNESCAPE[0]); i++) {
        dwEscaped=INTERNET_MAX_URL_LENGTH;
        res = pUrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, &dwEscaped, 0);
        ok(res == S_OK,
            "UrlUnescapeA returned 0x%x (expected S_OK) for \"%s\"\n",
            res, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,TEST_URL_UNESCAPE[i].expect)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url);

        ZeroMemory(szReturnUrl, sizeof(szReturnUrl));
        /* if we set the buffer pointer to NULL here, UrlUnescape fails and the string is not converted */
        res = pUrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, NULL, 0);
        ok(res == E_INVALIDARG,
            "UrlUnescapeA returned 0x%x (expected E_INVALIDARG) for \"%s\"\n",
            res, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,"")==0, "Expected empty string\n");

        if (pUrlUnescapeW) {
            dwEscaped = INTERNET_MAX_URL_LENGTH;
            urlW = GetWideString(TEST_URL_UNESCAPE[i].url);
            expected_urlW = GetWideString(TEST_URL_UNESCAPE[i].expect);
            res = pUrlUnescapeW(urlW, ret_urlW, &dwEscaped, 0);
            ok(res == S_OK,
                "UrlUnescapeW returned 0x%x (expected S_OK) for \"%s\"\n",
                res, TEST_URL_UNESCAPE[i].url);

            WideCharToMultiByte(CP_ACP,0,ret_urlW,-1,szReturnUrl,INTERNET_MAX_URL_LENGTH,0,0);
            ok(lstrcmpW(ret_urlW, expected_urlW)==0,
                "Expected \"%s\", but got \"%s\" from \"%s\" flags %08lx\n",
                TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url, 0L);
            FreeWideString(urlW);
            FreeWideString(expected_urlW);
        }
    }

    dwEscaped = sizeof(inplace);
    res = pUrlUnescapeA(inplace, NULL, &dwEscaped, URL_UNESCAPE_INPLACE);
    ok(res == S_OK, "UrlUnescapeA returned 0x%x (expected S_OK)\n", res);
    ok(!strcmp(inplace, expected), "got %s expected %s\n", inplace, expected);
    ok(dwEscaped == 27, "got %d expected 27\n", dwEscaped);

    /* if we set the buffer pointer to NULL, the string apparently still gets converted (Google Lively does this) */
    res = pUrlUnescapeA(another_inplace, NULL, NULL, URL_UNESCAPE_INPLACE);
    ok(res == S_OK, "UrlUnescapeA returned 0x%x (expected S_OK)\n", res);
    ok(!strcmp(another_inplace, expected), "got %s expected %s\n", another_inplace, expected);

    if (pUrlUnescapeW) {
        dwEscaped = sizeof(inplaceW);
        res = pUrlUnescapeW(inplaceW, NULL, &dwEscaped, URL_UNESCAPE_INPLACE);
        ok(res == S_OK, "UrlUnescapeW returned 0x%x (expected S_OK)\n", res);
        ok(dwEscaped == 50, "got %d expected 50\n", dwEscaped);

        /* if we set the buffer pointer to NULL, the string apparently still gets converted (Google Lively does this) */
        res = pUrlUnescapeW(another_inplaceW, NULL, NULL, URL_UNESCAPE_INPLACE);
        ok(res == S_OK, "UrlUnescapeW returned 0x%x (expected S_OK)\n", res);

        ok(lstrlenW(another_inplaceW) == 24, "got %d expected 24\n", lstrlenW(another_inplaceW));
    }
}

static const struct parse_url_test_t {
    const char *url;
    HRESULT hres;
    UINT protocol_len;
    UINT scheme;
} parse_url_tests[] = {
    {"http://www.winehq.org/",S_OK,4,URL_SCHEME_HTTP},
    {"https://www.winehq.org/",S_OK,5,URL_SCHEME_HTTPS},
    {"ftp://www.winehq.org/",S_OK,3,URL_SCHEME_FTP},
    {"test.txt?test=c:/dir",URL_E_INVALID_SYNTAX},
    {"test.txt",URL_E_INVALID_SYNTAX},
    {"xxx://www.winehq.org/",S_OK,3,URL_SCHEME_UNKNOWN},
    {"1xx://www.winehq.org/",S_OK,3,URL_SCHEME_UNKNOWN},
    {"-xx://www.winehq.org/",S_OK,3,URL_SCHEME_UNKNOWN},
    {"xx0://www.winehq.org/",S_OK,3,URL_SCHEME_UNKNOWN},
    {"x://www.winehq.org/",URL_E_INVALID_SYNTAX},
    {"xx$://www.winehq.org/",URL_E_INVALID_SYNTAX},
    {"htt?p://www.winehq.org/",URL_E_INVALID_SYNTAX},
    {"ab-://www.winehq.org/",S_OK,3,URL_SCHEME_UNKNOWN},
    {" http://www.winehq.org/",URL_E_INVALID_SYNTAX},
};

static void test_ParseURL(void)
{
    const struct parse_url_test_t *test;
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    PARSEDURLA parseda;
    PARSEDURLW parsedw;
    HRESULT hres;

    for(test = parse_url_tests; test < parse_url_tests + sizeof(parse_url_tests)/sizeof(*parse_url_tests); test++) {
        memset(&parseda, 0xd0, sizeof(parseda));
        parseda.cbSize = sizeof(parseda);
        hres = pParseURLA(test->url, &parseda);
        ok(hres == test->hres, "ParseURL failed: %08x, expected %08x\n", hres, test->hres);
        if(hres == S_OK) {
            ok(parseda.pszProtocol == test->url, "parseda.pszProtocol = %s, expected %s\n",
               parseda.pszProtocol, test->url);
            ok(parseda.cchProtocol == test->protocol_len, "parseda.cchProtocol = %d, expected %d\n",
               parseda.cchProtocol, test->protocol_len);
            ok(parseda.pszSuffix == test->url+test->protocol_len+1, "parseda.pszSuffix = %s, expected %s\n",
               parseda.pszSuffix, test->url+test->protocol_len+1);
            ok(parseda.cchSuffix == strlen(test->url+test->protocol_len+1),
               "parseda.pszSuffix = %d, expected %d\n",
               parseda.cchSuffix, lstrlenA(test->url+test->protocol_len+1));
            ok(parseda.nScheme == test->scheme, "parseda.nScheme = %d, expected %d\n",
               parseda.nScheme, test->scheme);
        }else {
            ok(!parseda.pszProtocol, "parseda.pszProtocol = %p\n", parseda.pszProtocol);
            ok(parseda.nScheme == 0xd0d0d0d0, "nScheme = %d\n", parseda.nScheme);
        }

        MultiByteToWideChar(CP_ACP, 0, test->url, -1, url, sizeof(url)/sizeof(WCHAR));

        memset(&parsedw, 0xd0, sizeof(parsedw));
        parsedw.cbSize = sizeof(parsedw);
        hres = pParseURLW(url, &parsedw);
        ok(hres == test->hres, "ParseURL failed: %08x, expected %08x\n", hres, test->hres);
        if(hres == S_OK) {
            ok(parsedw.pszProtocol == url, "parsedw.pszProtocol = %s, expected %s\n",
               wine_dbgstr_w(parsedw.pszProtocol), wine_dbgstr_w(url));
            ok(parsedw.cchProtocol == test->protocol_len, "parsedw.cchProtocol = %d, expected %d\n",
               parsedw.cchProtocol, test->protocol_len);
            ok(parsedw.pszSuffix == url+test->protocol_len+1, "parsedw.pszSuffix = %s, expected %s\n",
               wine_dbgstr_w(parsedw.pszSuffix), wine_dbgstr_w(url+test->protocol_len+1));
            ok(parsedw.cchSuffix == strlen(test->url+test->protocol_len+1),
               "parsedw.pszSuffix = %d, expected %d\n",
               parsedw.cchSuffix, lstrlenA(test->url+test->protocol_len+1));
            ok(parsedw.nScheme == test->scheme, "parsedw.nScheme = %d, expected %d\n",
               parsedw.nScheme, test->scheme);
        }else {
            ok(!parsedw.pszProtocol, "parsedw.pszProtocol = %p\n", parseda.pszProtocol);
            ok(parsedw.nScheme == 0xd0d0d0d0, "nScheme = %d\n", parsedw.nScheme);
        }
    }
}

static void test_HashData(void)
{
    HRESULT res;
    BYTE input[16] = {0x51, 0x33, 0x4F, 0xA7, 0x45, 0x15, 0xF0, 0x52, 0x90,
                      0x2B, 0xE7, 0xF5, 0xFD, 0xE1, 0xA6, 0xA7};
    BYTE output[32];
    static const BYTE expected[] = {0x54, 0x9C, 0x92, 0x55, 0xCD, 0x82, 0xFF,
                                    0xA1, 0x8E, 0x0F, 0xCF, 0x93, 0x14, 0xAA,
                                    0xE3, 0x2D};
    static const BYTE expected2[] = {0x54, 0x9C, 0x92, 0x55, 0xCD, 0x82, 0xFF,
                                     0xA1, 0x8E, 0x0F, 0xCF, 0x93, 0x14, 0xAA,
                                     0xE3, 0x2D, 0x47, 0xFC, 0x80, 0xB8, 0xD0,
                                     0x49, 0xE6, 0x13, 0x2A, 0x30, 0x51, 0x8D,
                                     0xF9, 0x4B, 0x07, 0xA6};
    static const BYTE expected3[] = {0x2B, 0xDC, 0x9A, 0x1B, 0xF0, 0x5A, 0xF9,
                                     0xC6, 0xBE, 0x94, 0x6D, 0xF3, 0x33, 0xC1,
                                     0x36, 0x07};
    int i;

    /* Test hashing with identically sized input/output buffers. */
    res = pHashData(input, 16, output, 16);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08x\n", res);
    if(res == S_OK)
       ok(!memcmp(output, expected, sizeof(expected)),
          "Output buffer did not match expected contents\n");

    /* Test hashing with larger output buffer. */
    res = pHashData(input, 16, output, 32);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08x\n", res);
    if(res == S_OK)
       ok(!memcmp(output, expected2, sizeof(expected2)),
          "Output buffer did not match expected contents\n");

    /* Test hashing with smaller input buffer. */
    res = pHashData(input, 8, output, 16);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08x\n", res);
    if(res == S_OK)
       ok(!memcmp(output, expected3, sizeof(expected3)),
          "Output buffer did not match expected contents\n");

    /* Test passing NULL pointers for input/output parameters. */
    res = pHashData(NULL, 0, NULL, 0);
    ok(res == E_INVALIDARG || broken(res == S_OK), /* Windows 2000 */
       "Expected HashData to return E_INVALIDARG, got 0x%08x\n", res);

    res = pHashData(input, 0, NULL, 0);
    ok(res == E_INVALIDARG || broken(res == S_OK), /* Windows 2000 */
       "Expected HashData to return E_INVALIDARG, got 0x%08x\n", res);

    res = pHashData(NULL, 0, output, 0);
    ok(res == E_INVALIDARG || broken(res == S_OK), /* Windows 2000 */
       "Expected HashData to return E_INVALIDARG, got 0x%08x\n", res);

    /* Test passing valid pointers with sizes of zero. */
    for (i = 0; i < sizeof(input)/sizeof(BYTE); i++)
        input[i] = 0x00;

    for (i = 0; i < sizeof(output)/sizeof(BYTE); i++)
        output[i] = 0xFF;

    res = pHashData(input, 0, output, 0);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08x\n", res);

    /* The buffers should be unchanged. */
    for (i = 0; i < sizeof(input)/sizeof(BYTE); i++)
    {
        ok(input[i] == 0x00, "Expected the input buffer to be unchanged\n");
        if(input[i] != 0x00) break;
    }

    for (i = 0; i < sizeof(output)/sizeof(BYTE); i++)
    {
        ok(output[i] == 0xFF, "Expected the output buffer to be unchanged\n");
        if(output[i] != 0xFF) break;
    }

    /* Input/output parameters are not validated. */
    res = pHashData((BYTE *)0xdeadbeef, 0, (BYTE *)0xdeadbeef, 0);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08x\n", res);

    if (0)
    {
        res = pHashData((BYTE *)0xdeadbeef, 1, (BYTE *)0xdeadbeef, 1);
        trace("HashData returned 0x%08x\n", res);
    }
}

/* ########################### */

START_TEST(url)
{
  char *pFunc;

  hShlwapi = GetModuleHandleA("shlwapi.dll");

  /* SHCreateStreamOnFileEx was introduced in shlwapi v6.0 */
  pFunc = (void*)GetProcAddress(hShlwapi, "SHCreateStreamOnFileEx");
  if(!pFunc){
      win_skip("Too old shlwapi version\n");
      return;
  }

  pUrlUnescapeA = (void *) GetProcAddress(hShlwapi, "UrlUnescapeA");
  pUrlUnescapeW = (void *) GetProcAddress(hShlwapi, "UrlUnescapeW");
  pUrlIsA = (void *) GetProcAddress(hShlwapi, "UrlIsA");
  pUrlIsW = (void *) GetProcAddress(hShlwapi, "UrlIsW");
  pUrlHashA = (void *) GetProcAddress(hShlwapi, "UrlHashA");
  pUrlHashW = (void *) GetProcAddress(hShlwapi, "UrlHashW");
  pUrlGetPartA = (void *) GetProcAddress(hShlwapi, "UrlGetPartA");
  pUrlGetPartW = (void *) GetProcAddress(hShlwapi, "UrlGetPartW");
  pUrlEscapeA = (void *) GetProcAddress(hShlwapi, "UrlEscapeA");
  pUrlEscapeW = (void *) GetProcAddress(hShlwapi, "UrlEscapeW");
  pUrlCreateFromPathA = (void *) GetProcAddress(hShlwapi, "UrlCreateFromPathA");
  pUrlCreateFromPathW = (void *) GetProcAddress(hShlwapi, "UrlCreateFromPathW");
  pUrlCombineA = (void *) GetProcAddress(hShlwapi, "UrlCombineA");
  pUrlCombineW = (void *) GetProcAddress(hShlwapi, "UrlCombineW");
  pUrlCanonicalizeA = (void *) GetProcAddress(hShlwapi, "UrlCanonicalizeA");
  pUrlCanonicalizeW = (void *) GetProcAddress(hShlwapi, "UrlCanonicalizeW");
  pUrlApplySchemeA = (void *) GetProcAddress(hShlwapi, "UrlApplySchemeA");
  pUrlApplySchemeW = (void *) GetProcAddress(hShlwapi, "UrlApplySchemeW");
  pParseURLA = (void*)GetProcAddress(hShlwapi, (LPCSTR)1);
  pParseURLW = (void*)GetProcAddress(hShlwapi, (LPCSTR)2);
  pHashData = (void*)GetProcAddress(hShlwapi, "HashData");

  test_UrlApplyScheme();
  test_UrlHash();
  test_UrlGetPart();
  test_UrlCanonicalizeA();
  test_UrlCanonicalizeW();
  test_UrlEscapeA();
  test_UrlEscapeW();
  test_UrlCombine();
  test_UrlCreateFromPath();
  test_UrlIs();
  test_UrlUnescape();
  test_ParseURL();
  test_HashData();
}
