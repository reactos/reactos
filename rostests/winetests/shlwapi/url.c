/* Unit test suite for Path functions
 *
 * Copyright 2002 Matthew Mastracci
 * Copyright 2007,2008 Detlef Riekenberg
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

/* ################ */
static HMODULE hShlwapi;
static HRESULT (WINAPI *pUrlCanonicalizeW)(LPCWSTR, LPWSTR, LPDWORD, DWORD);

static const char* TEST_URL_1 = "http://www.winehq.org/tests?date=10/10/1923";
static const char* TEST_URL_2 = "http://localhost:8080/tests%2e.html?date=Mon%2010/10/1923";
static const char* TEST_URL_3 = "http://foo:bar@localhost:21/internal.php?query=x&return=y";
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
    {"", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, 7, "http://"},
    {"", URL_APPLY_GUESSSCHEME, S_FALSE, TEST_APPLY_MAX_LENGTH, untouchedA},
    {"", URL_APPLY_DEFAULT, S_OK, 7, "http://"}
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
    {"http://www.winehq.org/tests/..#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests/../#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests\\../#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests/..\\#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org\\tests/../#example", 0, S_OK, "http://www.winehq.org/#example", FALSE},
    {"http://www.winehq.org/tests/../#example", URL_DONT_SIMPLIFY, S_OK, "http://www.winehq.org/tests/../#example", FALSE},
    {"http://www.winehq.org/tests/foo bar", URL_ESCAPE_SPACES_ONLY| URL_DONT_ESCAPE_EXTRA_INFO , S_OK, "http://www.winehq.org/tests/foo%20bar", FALSE},
    {"http://www.winehq.org/tests/foo%20bar", URL_UNESCAPE , S_OK, "http://www.winehq.org/tests/foo bar", FALSE},
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
    {"http://www.winehq.org/site/about", URL_FILE_USE_PATHURL, S_OK, "http://www.winehq.org/site/about", FALSE},
    {"file_://www.winehq.org/site/about", URL_FILE_USE_PATHURL, S_OK, "file_://www.winehq.org/site/about", FALSE},
    {"c:\\dir\\file", 0, S_OK, "file:///c:/dir/file", FALSE},
    {"file:///c:\\dir\\file", 0, S_OK, "file:///c:/dir/file", FALSE},
    {"c:dir\\file", 0, S_OK, "file:///c:dir/file", FALSE},
    {"c:\\tests\\foo bar", URL_FILE_USE_PATHURL, S_OK, "file://c:\\tests\\foo bar", FALSE},
    {"c:\\tests\\foo bar", 0, S_OK, "file:///c:/tests/foo%20bar", FALSE},
    {"res:///c:/tests/foo%20bar", URL_UNESCAPE , S_OK, "res:///c:/tests/foo bar", FALSE},
    {"res:///c:/tests\\foo%20bar", URL_UNESCAPE , S_OK, "res:///c:/tests\\foo bar", TRUE},
    {"res:///c:/tests/foo%20bar", 0, S_OK, "res:///c:/tests/foo%20bar", FALSE},
    {"res:///c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res:///c:/tests/foo%20bar", TRUE},
    {"res://c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res://c:/tests/foo%20bar", TRUE},
    {"res://c:/tests\\../tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res://c:/tests/foo%20bar", TRUE},
    {"res://c:/tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res://c:/tests/foo%20bar", TRUE},
    {"res:///c://tests/foo%20bar", URL_FILE_USE_PATHURL, S_OK, "res:///c://tests/foo%20bar", TRUE},
    {"res:///c:\\tests\\foo bar", 0, S_OK, "res:///c:\\tests\\foo bar", TRUE},
    {"res:///c:\\tests\\foo bar", URL_DONT_SIMPLIFY, S_OK, "res:///c:\\tests\\foo bar", TRUE},
    {"A", 0, S_OK, "A", FALSE},
    {"/uri-res/N2R?urn:sha1:B3K", URL_DONT_ESCAPE_EXTRA_INFO | URL_WININET_COMPATIBILITY /*0x82000000*/, S_OK, "/uri-res/N2R?urn:sha1:B3K", TRUE} /*LimeWire online installer calls this*/,
    {"http:www.winehq.org/dir/../index.html", 0, S_OK, "http:www.winehq.org/index.html"},
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
    {"ftp:////fo/o@bar.baz/foo/bar", 0, 0, S_OK, "ftp:////fo/o@bar.baz/foo/bar"}
};

/* ################ */

typedef struct _TEST_URL_COMBINE {
    const char *url1;
    const char *url2;
    DWORD flags;
    HRESULT expectret;
    const char *expecturl;
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
    {"file:///C:\\dir\\file.txt", "test.txt", 0, S_OK, "file:///C:/dir/test.txt"},
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
    {"foo:/today/", "foo:calendar", 0, S_OK, "foo:/today/calendar"},
    {"mk:@MSITStore:dir/test.chm::dir/index.html", "image.jpg", 0, S_OK, "mk:@MSITStore:dir/test.chm::dir/image.jpg"},
    {"mk:@MSITStore:dir/test.chm::dir/dir2/index.html", "../image.jpg", 0, S_OK, "mk:@MSITStore:dir/test.chm::dir/image.jpg"}
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
    {	"file:partial",					FALSE,	TRUE	}
};

/* ########################### */

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

/* ########################### */

static void test_UrlApplyScheme(void)
{
    CHAR newurl[TEST_APPLY_MAX_LENGTH];
    WCHAR urlW[TEST_APPLY_MAX_LENGTH];
    WCHAR newurlW[TEST_APPLY_MAX_LENGTH];
    HRESULT res;
    DWORD len;
    DWORD i;

    for(i = 0; i < sizeof(TEST_APPLY)/sizeof(TEST_APPLY[0]); i++) {
        len = TEST_APPLY_MAX_LENGTH;
        lstrcpyA(newurl, untouchedA);
        res = UrlApplySchemeA(TEST_APPLY[i].url, newurl, &len, TEST_APPLY[i].flags);
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

        res = UrlApplySchemeW(urlW, newurlW, &len, TEST_APPLY[i].flags);
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
    res = UrlApplySchemeA(TEST_APPLY[0].url, newurl, &len, TEST_APPLY[0].flags);
    ok(res == E_POINTER, "got HRESULT 0x%x (expected E_POINTER)\n", res);
    /* The returned length include the space for the terminating 0 */
    i = lstrlenA(TEST_APPLY[0].newurl)+1;
    ok(len == i, "got len %d (expected %d)\n", len, i);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

    /* NULL as parameter. The length and the buffer are not modified */
    lstrcpyA(newurl, untouchedA);
    len = TEST_APPLY_MAX_LENGTH;
    res = UrlApplySchemeA(NULL, newurl, &len, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%x (expected E_INVALIDARG)\n", res);
    ok(len == TEST_APPLY_MAX_LENGTH, "got len %d\n", len);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

    len = TEST_APPLY_MAX_LENGTH;
    res = UrlApplySchemeA(TEST_APPLY[0].url, NULL, &len, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%x (expected E_INVALIDARG)\n", res);
    ok(len == TEST_APPLY_MAX_LENGTH, "got len %d\n", len);

    lstrcpyA(newurl, untouchedA);
    res = UrlApplySchemeA(TEST_APPLY[0].url, newurl, NULL, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%x (expected E_INVALIDARG)\n", res);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

}

/* ########################### */

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

/* ########################### */

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

/* ########################### */

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

/* ########################### */

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
    ok(UrlCanonicalizeA(szUrl, NULL, &dwSize, dwFlags) != dwExpectReturn, "Unexpected return for NULL buffer, index %d\n", index);
    ret = UrlCanonicalizeA(szUrl, szReturnUrl, &dwSize, dwFlags);
    ok(ret == dwExpectReturn || ret == dwExpectReturnAlt,
       "UrlCanonicalizeA failed: expected=0x%08x or 0x%08x, got=0x%08x, index %d\n",
       dwExpectReturn, dwExpectReturnAlt, ret, index);
    if (todo)
        todo_wine
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "UrlCanonicalizeA dwFlags 0x%08x url '%s' Expected \"%s\", but got \"%s\", index %d\n", dwFlags, szUrl, szExpectUrl, szReturnUrl, index);
    else
        ok(strcmp(szReturnUrl,szExpectUrl)==0, "UrlCanonicalizeA dwFlags 0x%08x url '%s' Expected \"%s\", but got \"%s\", index %d\n", dwFlags, szUrl, szExpectUrl, szReturnUrl, index);

    dwSize = INTERNET_MAX_URL_LENGTH;
    ok(UrlCanonicalizeW(wszUrl, NULL, &dwSize, dwFlags) != dwExpectReturn, "Unexpected return for NULL buffer, index %d\n", index);
    ok(UrlCanonicalizeW(wszUrl, wszReturnUrl, &dwSize, dwFlags) == dwExpectReturn, "UrlCanonicalizeW didn't return 0x%08x, index %d\n", dwExpectReturn, index);
    wszConvertedUrl = GetWideString(szReturnUrl);
    ok(lstrcmpW(wszReturnUrl, wszConvertedUrl)==0, "Strings didn't match between ascii and unicode UrlCanonicalize, index %d!\n", index);
    FreeWideString(wszConvertedUrl);


    FreeWideString(wszUrl);
    FreeWideString(wszExpectUrl);
}


static void test_UrlEscape(void)
{
    DWORD size = 0;
    HRESULT ret;
    unsigned int i;
    char empty_string[] = "";

    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 0, "got %d, expected %d\n", size, 0);

    size = 0;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 0, "got %d, expected %d\n", size, 0);

    size = 1;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 1, "got %d, expected %d\n", size, 1);

    size = 1;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %x, expected %x\n", ret, E_INVALIDARG);
    ok(size == 1, "got %d, expected %d\n", size, 1);

    size = 1;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_POINTER, "got %x, expected %x\n", ret, E_POINTER);
    ok(size == 34, "got %d, expected %d\n", size, 34);

    for(i=0; i<sizeof(TEST_ESCAPE)/sizeof(TEST_ESCAPE[0]); i++) {
        test_url_escape(TEST_ESCAPE[i].url, TEST_ESCAPE[i].flags,
                              TEST_ESCAPE[i].expectret, TEST_ESCAPE[i].expecturl);
    }
}

/* ########################### */

static void test_UrlCanonicalizeA(void)
{
    unsigned int i;
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwSize;
    DWORD urllen;
    HRESULT hr;

    urllen = lstrlenA(winehqA);

    /* buffer has no space for the result */
    dwSize=urllen-1;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY  | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'E_POINTER' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen+1);

    /* buffer has no space for the terminating '\0' */
    dwSize=urllen;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'E_POINTER' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen+1);

    /* buffer has the required size */
    dwSize=urllen+1;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'S_OK' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen);

    /* buffer is larger as the required size */
    dwSize=urllen+2;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%x with %u and size %u for '%s' and %u (expected 'S_OK' and size %u)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen);

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
        skip("UrlCanonicalizeW\n");
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

        MultiByteToWideChar(CP_ACP, 0, "http://www.winehq.org/X", -1, szUrl, 128);
        pos = lstrlenW(szUrl) - 1;
        szUrl[pos] = i;
        urllen = INTERNET_MAX_URL_LENGTH;
        pUrlCanonicalizeW(szUrl, szReturnUrl, &urllen, 0);
        choped = lstrlenW(szReturnUrl) < lstrlenW(szUrl);
        ok(choped == (i <= 32), "Incorrect char chopping for char %d\n", i);
    }
}

/* ########################### */

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

/* ########################### */

static void test_UrlCombine(void)
{
    unsigned int i;
    for(i=0; i<sizeof(TEST_COMBINE)/sizeof(TEST_COMBINE[0]); i++) {
        test_url_combine(TEST_COMBINE[i].url1, TEST_COMBINE[i].url2, TEST_COMBINE[i].flags,
                         TEST_COMBINE[i].expectret, TEST_COMBINE[i].expecturl);
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

/* ########################### */

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
    static WCHAR another_inplaceW[] = {'f','i','l','e',':','/','/','/','C',':','/','P','r','o','g','r','a','m','%','2','0','F','i','l','e','s',0};

    for(i=0; i<sizeof(TEST_URL_UNESCAPE)/sizeof(TEST_URL_UNESCAPE[0]); i++) {
        dwEscaped=INTERNET_MAX_URL_LENGTH;
        ok(UrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, &dwEscaped, 0) == S_OK, "UrlUnescapeA didn't return 0x%08x from \"%s\"\n", S_OK, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,TEST_URL_UNESCAPE[i].expect)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url);

        ZeroMemory(szReturnUrl, sizeof(szReturnUrl));
        /* if we set the bufferpointer to NULL here UrlUnescape  fails and string gets not converted */
        ok(UrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, NULL, 0) == E_INVALIDARG, "UrlUnescapeA didn't return 0x%08x from \"%s\"\n", E_INVALIDARG ,TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,"")==0, "Expected empty string\n");

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
    ok(!strcmp(inplace, expected), "got %s expected %s\n", inplace, expected);
    ok(dwEscaped == 27, "got %d expected 27\n", dwEscaped);

    /* if we set the bufferpointer to NULL, the string apparently still gets converted (Google Lively does this)) */
    ok(UrlUnescapeA(another_inplace, NULL, NULL, URL_UNESCAPE_INPLACE) == S_OK, "UrlUnescapeA failed unexpectedly\n");
    ok(!strcmp(another_inplace, expected), "got %s expected %s\n", another_inplace, expected);

    dwEscaped = sizeof(inplaceW);
    ok(UrlUnescapeW(inplaceW, NULL, &dwEscaped, URL_UNESCAPE_INPLACE) == S_OK, "UrlUnescapeW failed unexpectedly\n");
    ok(dwEscaped == 50, "got %d expected 50\n", dwEscaped);

    /* if we set the bufferpointer to NULL, the string apparently still gets converted (Google Lively does this)) */
    ok(UrlUnescapeW(another_inplaceW, NULL, NULL, URL_UNESCAPE_INPLACE) == S_OK, "UrlUnescapeW failed unexpectedly\n");
    ok(lstrlenW(another_inplaceW) == 24, "got %d expected 24\n", lstrlenW(another_inplaceW));

}

/* ########################### */

START_TEST(url)
{

  hShlwapi = GetModuleHandleA("shlwapi.dll");
  pUrlCanonicalizeW = (void *) GetProcAddress(hShlwapi, "UrlCanonicalizeW");

  test_UrlApplyScheme();
  test_UrlHash();
  test_UrlGetPart();
  test_UrlCanonicalizeA();
  test_UrlCanonicalizeW();
  test_UrlEscape();
  test_UrlCombine();
  test_UrlCreateFromPath();
  test_UrlIs();
  test_UrlUnescape();

}
