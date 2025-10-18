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

#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"
#include "intshcut.h"
#include "winternl.h"

static const char* TEST_URL_1 = "http://www.winehq.org/tests?date=10/10/1923";
static const char* TEST_URL_2 = "http://localhost:8080/tests%2e.html?date=Mon%2010/10/1923";
static const char* TEST_URL_3 = "http://foo:bar@localhost:21/internal.php?query=x&return=y";

static const WCHAR winehqW[] = L"http://www.winehq.org/";
static const char winehqA[] = "http://www.winehq.org/";
static const CHAR untouchedA[] = "untouched";

typedef struct _TEST_URL_APPLY {
    const char * url;
    DWORD flags;
    HRESULT res;
    const char * newurl;
} TEST_URL_APPLY;

static const TEST_URL_APPLY TEST_APPLY[] = {
    {"www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, "http://www.winehq.org"},
    {"www.winehq.org", URL_APPLY_GUESSSCHEME, S_OK, "http://www.winehq.org"},
    {"www.winehq.org", URL_APPLY_DEFAULT, S_OK, "http://www.winehq.org"},
    {"ftp.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, "ftp://ftp.winehq.org"},
    {"ftp.winehq.org", URL_APPLY_GUESSSCHEME, S_OK, "ftp://ftp.winehq.org"},
    {"ftp.winehq.org", URL_APPLY_DEFAULT, S_OK, "http://ftp.winehq.org"},
    {"winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, "http://winehq.org"},
    {"winehq.org", URL_APPLY_GUESSSCHEME, S_FALSE},
    {"winehq.org", URL_APPLY_DEFAULT, S_OK, "http://winehq.org"},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME, S_FALSE},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_FORCEAPPLY, S_FALSE},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_FORCEAPPLY | URL_APPLY_DEFAULT, S_OK, "http://http://www.winehq.org"},
    {"http://www.winehq.org", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_FALSE},
    {"", URL_APPLY_GUESSSCHEME | URL_APPLY_DEFAULT, S_OK, "http://"},
    {"", URL_APPLY_GUESSSCHEME, S_FALSE},
    {"", URL_APPLY_DEFAULT, S_OK, "http://"},
    {"u:\\windows", URL_APPLY_GUESSFILE | URL_APPLY_DEFAULT, S_OK, "file:///u:/windows"},
    {"u:\\windows", URL_APPLY_GUESSFILE, S_OK, "file:///u:/windows"},
    {"u:\\windows", URL_APPLY_DEFAULT, S_OK, "http://u:\\windows"},
    {"file:///c:/windows", URL_APPLY_GUESSFILE, S_FALSE},
    {"aa:\\windows", URL_APPLY_GUESSFILE, S_FALSE},
    {"\\\\server\\share", URL_APPLY_DEFAULT, S_OK, "http://\\\\server\\share"},
    {"\\\\server\\share", URL_APPLY_GUESSFILE, S_OK, "file://server/share"},
    {"\\\\server\\share", URL_APPLY_GUESSSCHEME, S_FALSE},
    {"file://server/share", URL_APPLY_GUESSFILE, S_FALSE},
    {"file://server/share", URL_APPLY_GUESSSCHEME, S_FALSE},
};

typedef struct _TEST_URL_ESCAPE {
    const char *url;
    DWORD flags;
    const char *expecturl;
} TEST_URL_ESCAPE;

static const TEST_URL_ESCAPE TEST_ESCAPE[] = {
    {"http://www.winehq.org/tests0", 0, "http://www.winehq.org/tests0"},
    {"http://www.winehq.org/tests1\n", 0, "http://www.winehq.org/tests1%0A"},
    {"http://www.winehq.org/tests2\r", 0, "http://www.winehq.org/tests2%0D"},
    {"http://www.winehq.org/tests3\r", URL_ESCAPE_SPACES_ONLY|URL_ESCAPE_UNSAFE, "http://www.winehq.org/tests3\r"},
    {"http://www.winehq.org/tests4\r", URL_ESCAPE_SPACES_ONLY, "http://www.winehq.org/tests4\r"},
    {"http://www.winehq.org/tests5\r", URL_WININET_COMPATIBILITY|URL_ESCAPE_SPACES_ONLY, "http://www.winehq.org/tests5\r"},
    {"/direct/swhelp/series6/6.2i_latestservicepack.dat\r", URL_ESCAPE_SPACES_ONLY, "/direct/swhelp/series6/6.2i_latestservicepack.dat\r"},

    {"file://////foo/bar\\baz", 0, "file://foo/bar/baz"},
    {"file://///foo/bar\\baz", 0, "file://foo/bar/baz"},
    {"file:////foo/bar\\baz", 0, "file://foo/bar/baz"},
    {"file:///localhost/foo/bar\\baz", 0, "file:///localhost/foo/bar/baz"},
    {"file:///foo/bar\\baz", 0, "file:///foo/bar/baz"},
    {"file://loCalHost/foo/bar\\baz", 0, "file:///foo/bar/baz"},
    {"file://foo/bar\\baz", 0, "file://foo/bar/baz"},
    {"file:/localhost/foo/bar\\baz", 0, "file:///localhost/foo/bar/baz"},
    {"file:/foo/bar\\baz", 0, "file:///foo/bar/baz"},
    {"file:foo/bar\\baz", 0, "file:foo/bar/baz"},
    {"file:\\foo/bar\\baz", 0, "file:///foo/bar/baz"},
    {"file:\\\\foo/bar\\baz", 0, "file://foo/bar/baz"},
    {"file:\\\\\\foo/bar\\baz", 0, "file:///foo/bar/baz"},
    {"file:\\\\localhost\\foo/bar\\baz", 0, "file:///foo/bar/baz"},
    {"file:///f oo/b?a r\\baz", 0, "file:///f%20oo/b?a r\\baz"},
    {"file:///foo/b#a r\\baz", 0, "file:///foo/b%23a%20r/baz"},
    {"file:///f o^&`{}|][\"<>\\%o/b#a r\\baz", 0, "file:///f%20o%5E%26%60%7B%7D%7C%5D%5B%22%3C%3E/%o/b%23a%20r/baz"},
    {"file:///f o%o/b?a r\\b%az", URL_ESCAPE_PERCENT, "file:///f%20o%25o/b?a r\\b%az"},
    {"file:/foo/bar\\baz", URL_ESCAPE_SEGMENT_ONLY, "file:%2Ffoo%2Fbar%5Cbaz"},

    {"foo/b%ar\\ba?z\\", URL_ESCAPE_SEGMENT_ONLY, "foo%2Fb%ar%5Cba%3Fz%5C"},
    {"foo/b%ar\\ba?z\\", URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY, "foo%2Fb%25ar%5Cba%3Fz%5C"},
    {"foo/bar\\ba?z\\", 0, "foo/bar%5Cba?z\\"},
    {"/foo/bar\\ba?z\\", 0, "/foo/bar%5Cba?z\\"},
    {"/foo/bar\\ba#z\\", 0, "/foo/bar%5Cba#z\\"},
    {"/foo/%5C", 0, "/foo/%5C"},
    {"/foo/%5C", URL_ESCAPE_PERCENT, "/foo/%255C"},

    {"http://////foo/bar\\baz", 0, "http://////foo/bar/baz"},
    {"http://///foo/bar\\baz", 0, "http://///foo/bar/baz"},
    {"http:////foo/bar\\baz", 0, "http:////foo/bar/baz"},
    {"http:///foo/bar\\baz", 0, "http:///foo/bar/baz"},
    {"http://localhost/foo/bar\\baz", 0, "http://localhost/foo/bar/baz"},
    {"http://foo/bar\\baz", 0, "http://foo/bar/baz"},
    {"http:/foo/bar\\baz", 0, "http:/foo/bar/baz"},
    {"http:foo/bar\\ba?z\\", 0, "http:foo%2Fbar%2Fba?z\\"},
    {"http:foo/bar\\ba#z\\", 0, "http:foo%2Fbar%2Fba#z\\"},
    {"http:\\foo/bar\\baz", 0, "http:/foo/bar/baz"},
    {"http:\\\\foo/bar\\baz", 0, "http://foo/bar/baz"},
    {"http:\\\\\\foo/bar\\baz", 0, "http:///foo/bar/baz"},
    {"http:\\\\\\\\foo/bar\\baz", 0, "http:////foo/bar/baz"},
    {"http:/fo ?o/b ar\\baz", 0, "http:/fo%20?o/b ar\\baz"},
    {"http:fo ?o/b ar\\baz", 0, "http:fo%20?o/b ar\\baz"},
    {"http:/foo/bar\\baz", URL_ESCAPE_SEGMENT_ONLY, "http:%2Ffoo%2Fbar%5Cbaz"},

    {"https://foo/bar\\baz", 0, "https://foo/bar/baz"},
    {"https:/foo/bar\\baz", 0, "https:/foo/bar/baz"},
    {"https:\\foo/bar\\baz", 0, "https:/foo/bar/baz"},

    {"foo:////foo/bar\\baz", 0, "foo:////foo/bar%5Cbaz"},
    {"foo:///foo/bar\\baz", 0, "foo:///foo/bar%5Cbaz"},
    {"foo://localhost/foo/bar\\baz", 0, "foo://localhost/foo/bar%5Cbaz"},
    {"foo://foo/bar\\baz", 0, "foo://foo/bar%5Cbaz"},
    {"foo:/foo/bar\\baz", 0, "foo:/foo/bar%5Cbaz"},
    {"foo:foo/bar\\baz", 0, "foo:foo%2Fbar%5Cbaz"},
    {"foo:\\foo/bar\\baz", 0, "foo:%5Cfoo%2Fbar%5Cbaz"},
    {"foo:/foo/bar\\ba?\\z", 0, "foo:/foo/bar%5Cba?\\z"},
    {"foo:/foo/bar\\ba#\\z", 0, "foo:/foo/bar%5Cba#\\z"},

    {"mailto:/fo/o@b\\%a?\\r.b#\\az", 0, "mailto:%2Ffo%2Fo@b%5C%a%3F%5Cr.b%23%5Caz"},
    {"mailto:fo/o@b\\%a?\\r.b#\\az", 0, "mailto:fo%2Fo@b%5C%a%3F%5Cr.b%23%5Caz"},
    {"mailto:fo/o@b\\%a?\\r.b#\\az", URL_ESCAPE_PERCENT, "mailto:fo%2Fo@b%5C%25a%3F%5Cr.b%23%5Caz"},

    {"ftp:fo/o@bar.baz/foo/bar", 0, "ftp:fo%2Fo@bar.baz%2Ffoo%2Fbar"},
    {"ftp:/fo/o@bar.baz/foo/bar", 0, "ftp:/fo/o@bar.baz/foo/bar"},
    {"ftp://fo/o@bar.baz/fo?o\\bar", 0, "ftp://fo/o@bar.baz/fo?o\\bar"},
    {"ftp://fo/o@bar.baz/fo#o\\bar", 0, "ftp://fo/o@bar.baz/fo#o\\bar"},
    {"ftp://localhost/o@bar.baz/fo#o\\bar", 0, "ftp://localhost/o@bar.baz/fo#o\\bar"},
    {"ftp:///fo/o@bar.baz/foo/bar", 0, "ftp:///fo/o@bar.baz/foo/bar"},
    {"ftp:////fo/o@bar.baz/foo/bar", 0, "ftp:////fo/o@bar.baz/foo/bar"},

    {"ftp\x1f\1end/", 0, "ftp%1F%01end/"}
};

typedef struct _TEST_URL_ESCAPEW {
    const WCHAR url[INTERNET_MAX_URL_LENGTH];
    DWORD flags;
    const WCHAR expecturl[INTERNET_MAX_URL_LENGTH];
    const WCHAR win7url[INTERNET_MAX_URL_LENGTH];  /* <= Win7 */
} TEST_URL_ESCAPEW;

static const TEST_URL_ESCAPEW TEST_ESCAPEW[] = {
    {L" <>\"", URL_ESCAPE_AS_UTF8, L"%20%3C%3E%22"},
    {L"{}|\\", URL_ESCAPE_AS_UTF8, L"%7B%7D%7C%5C"},
    {L"^][`", URL_ESCAPE_AS_UTF8, L"%5E%5D%5B%60"},
    {L"&/?#", URL_ESCAPE_AS_UTF8, L"%26/?#"},
    {L"Mass", URL_ESCAPE_AS_UTF8, L"Mass"},

    /* broken < Win8/10 */
    {L"Ma\xdf", URL_ESCAPE_AS_UTF8, L"Ma%C3%9F", L"Ma%DF"},
    {L"\xd841\xdf0e", URL_ESCAPE_AS_UTF8, L"%F0%A0%9C%8E", L"%EF%BF%BD%EF%BF%BD"}, /* 0x2070E */
    {L"\xd85e\xde3e", URL_ESCAPE_AS_UTF8, L"%F0%A7%A8%BE", L"%EF%BF%BD%EF%BF%BD"}, /* 0x27A3E */
    {L"\xd85e", URL_ESCAPE_AS_UTF8, L"%EF%BF%BD", L"\xd85e"},
    {L"\xd85eQ", URL_ESCAPE_AS_UTF8, L"%EF%BF%BDQ", L"\xd85eQ"},
    {L"\xdc00", URL_ESCAPE_AS_UTF8, L"%EF%BF%BD", L"\xdc00"},
    {L"\xffff", URL_ESCAPE_AS_UTF8, L"%EF%BF%BF", L"\xffff"},
};

/* ################ */

typedef struct _TEST_URL_COMBINE {
    const char *url1;
    const char *url2;
    DWORD flags;
    const char *expecturl;
} TEST_URL_COMBINE;

static const TEST_URL_COMBINE TEST_COMBINE[] = {
    {"http://www.winehq.org/tests", "tests1", 0, "http://www.winehq.org/tests1"},
    {"http://www.%77inehq.org/tests", "tests1", 0, "http://www.%77inehq.org/tests1"},
    /*FIXME {"http://www.winehq.org/tests", "../tests2", 0, "http://www.winehq.org/tests2"},*/
    {"http://www.winehq.org/tests/", "../tests3", 0, "http://www.winehq.org/tests3"},
    {"http://www.winehq.org/tests/test1", "test2", 0, "http://www.winehq.org/tests/test2"},
    {"http://www.winehq.org/tests/../tests", "tests4", 0, "http://www.winehq.org/tests4"},
    {"http://www.winehq.org/tests/../tests/", "tests5", 0, "http://www.winehq.org/tests/tests5"},
    {"http://www.winehq.org/tests/../tests/", "/tests6/..", 0, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/", "\\tests6\\..", 0, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/../tests/..", "tests7/..", 0, "http://www.winehq.org/"},
    {"http://www.winehq.org/tests/?query=x&return=y", "tests8", 0, "http://www.winehq.org/tests/tests8"},
    {"http://www.winehq.org/tests/#example", "tests9", 0, "http://www.winehq.org/tests/tests9"},
    {"http://www.winehq.org/tests/../tests/", "/tests10/..", URL_DONT_SIMPLIFY, "http://www.winehq.org/tests10/.."},
    {"http://www.winehq.org/tests/../", "tests11", URL_DONT_SIMPLIFY, "http://www.winehq.org/tests/../tests11"},
    {"http://www.winehq.org/test12", "#", 0, "http://www.winehq.org/test12#"},
    {"http://www.winehq.org/test13#aaa", "#bbb", 0, "http://www.winehq.org/test13#bbb"},
    {"http://www.winehq.org/test14#aaa/bbb#ccc", "#", 0, "http://www.winehq.org/test14#"},
    {"http://www.winehq.org/tests/?query=x/y/z", "tests15", 0, "http://www.winehq.org/tests/tests15"},
    {"http://www.winehq.org/tests/?query=x/y/z#example", "tests16", 0, "http://www.winehq.org/tests/tests16"},
    {"file:///C:\\dir\\file.txt", "test.txt", 0, "file:///C:/dir/test.txt"},
    {"file:///C:\\dir\\file.txt#hash\\hash", "test.txt", 0, "file:///C:/dir/file.txt#hash/test.txt"},
    {"file:///C:\\dir\\file.html#hash\\hash", "test.html", 0, "file:///C:/dir/test.html"},
    {"file:///C:\\dir\\file.htm#hash\\hash", "test.htm", 0, "file:///C:/dir/test.htm"},
    {"file:///C:\\dir\\file.hTmL#hash\\hash", "test.hTmL", 0, "file:///C:/dir/test.hTmL"},
    {"file:///C:\\dir.html\\file.txt#hash\\hash", "test.txt", 0, "file:///C:/dir.html/file.txt#hash/test.txt"},
    {"C:\\winehq\\winehq.txt", "C:\\Test\\test.txt", 0, "file:///C:/Test/test.txt"},
    {"http://www.winehq.org/test/", "test%20file.txt", 0, "http://www.winehq.org/test/test%20file.txt"},
    {"http://www.winehq.org/test/", "test%20file.txt", URL_FILE_USE_PATHURL, "http://www.winehq.org/test/test%20file.txt"},
    {"http://www.winehq.org%2ftest/", "test%20file.txt", URL_FILE_USE_PATHURL, "http://www.winehq.org%2ftest/test%20file.txt"},
    {"xxx:@MSITStore:file.chm/file.html", "dir/file", 0, "xxx:dir/file"},
    {"mk:@MSITStore:file.chm::/file.html", "/dir/file", 0, "mk:@MSITStore:file.chm::/dir/file"},
    {"mk:@MSITStore:file.chm::/file.html", "mk:@MSITStore:file.chm::/dir/file", 0, "mk:@MSITStore:file.chm::/dir/file"},
    {"foo:today", "foo:calendar", 0, "foo:calendar"},
    {"foo:today", "bar:calendar", 0, "bar:calendar"},
    {"foo:/today", "foo:calendar", 0, "foo:/calendar"},
    {"Foo:/today/", "fOo:calendar", 0, "foo:/today/calendar"},
    {"mk:@MSITStore:dir/test.chm::dir/index.html", "image.jpg", 0, "mk:@MSITStore:dir/test.chm::dir/image.jpg"},
    {"mk:@MSITStore:dir/test.chm::dir/dir2/index.html", "../image.jpg", 0, "mk:@MSITStore:dir/test.chm::dir/image.jpg"},
    {"c:\\test\\", "//share/file.txt", 0, "file://share/file.txt"},
    {"c:\\test\\", "\\\\share\\file.txt", 0, "file://share/file.txt"},
    /* UrlCombine case 2 tests.  Schemes do not match */
    {"outbind://xxxxxxxxx", "http://wine1/dir", 0, "http://wine1/dir"},
    {"xxxx://xxxxxxxxx", "http://wine2/dir", 0, "http://wine2/dir"},
    {"ftp://xxxxxxxxx/", "http://wine3/dir", 0, "http://wine3/dir"},
    {"outbind://xxxxxxxxx", "http://wine4/dir", URL_PLUGGABLE_PROTOCOL, "http://wine4/dir"},
    {"xxx://xxxxxxxxx", "http://wine5/dir", URL_PLUGGABLE_PROTOCOL, "http://wine5/dir"},
    {"ftp://xxxxxxxxx/", "http://wine6/dir", URL_PLUGGABLE_PROTOCOL, "http://wine6/dir"},
    {"http://xxxxxxxxx", "outbind://wine7/dir", 0, "outbind://wine7/dir"},
    {"xxx://xxxxxxxxx", "ftp://wine8/dir", 0, "ftp://wine8/dir"},
    {"ftp://xxxxxxxxx/", "xxx://wine9/dir", 0, "xxx://wine9/dir"},
    {"http://xxxxxxxxx", "outbind://wine10/dir", URL_PLUGGABLE_PROTOCOL, "outbind://wine10/dir"},
    {"xxx://xxxxxxxxx", "ftp://wine11/dir", URL_PLUGGABLE_PROTOCOL, "ftp://wine11/dir"},
    {"ftp://xxxxxxxxx/", "xxx://wine12/dir", URL_PLUGGABLE_PROTOCOL, "xxx://wine12/dir"},
    {"http://xxxxxxxxx", "outbind:wine13/dir", 0, "outbind:wine13/dir"},
    {"xxx://xxxxxxxxx", "ftp:wine14/dir", 0, "ftp:wine14/dir"},
    {"ftp://xxxxxxxxx/", "xxx:wine15/dir", 0, "xxx:wine15/dir"},
    {"outbind://xxxxxxxxx/", "http:wine16/dir", 0, "http:wine16/dir"},
    {"http://xxxxxxxxx", "outbind:wine17/dir", URL_PLUGGABLE_PROTOCOL, "outbind:wine17/dir"},
    {"xxx://xxxxxxxxx", "ftp:wine18/dir", URL_PLUGGABLE_PROTOCOL, "ftp:wine18/dir"},
    {"ftp://xxxxxxxxx/", "xXx:wine19/dir", URL_PLUGGABLE_PROTOCOL, "xxx:wine19/dir"},
    {"outbind://xxxxxxxxx/", "http:wine20/dir", URL_PLUGGABLE_PROTOCOL, "http:wine20/dir"},
    {"file:///c:/dir/file.txt", "index.html?test=c:/abc", URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO, "file:///c:/dir/index.html?test=c:/abc"}
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
    {"xx:c:\\foo\\bar", "xx:c:\\foo\\bar", S_FALSE}
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

static struct
{
    const WCHAR *url;
    const WCHAR *expect;
    DWORD flags;
} TEST_URL_UNESCAPEW[] =
{
    { L"file://foo/bar", L"file://foo/bar" },
    { L"file://fo%20o%5Ca/bar", L"file://fo o\\a/bar" },
    { L"file://%24%25foobar", L"file://$%foobar" },
    { L"file:///C:/Program Files", L"file:///C:/Program Files" },
    { L"file:///C:/Program Files", L"file:///C:/Program Files", URL_UNESCAPE_AS_UTF8 },
    { L"file:///C:/Program%20Files", L"file:///C:/Program Files" },
    { L"file:///C:/Program%20Files", L"file:///C:/Program Files", URL_UNESCAPE_AS_UTF8 },
    { L"file://foo/%E4%B8%AD%E6%96%87/bar", L"file://foo/\xe4\xb8\xad\xe6\x96\x87/bar" }, /* with 3 btyes utf-8 */
    { L"file://foo/%E4%B8%AD%E6%96%87/bar", L"file://foo/\x4e2d\x6587/bar", URL_UNESCAPE_AS_UTF8 },
    /* mix corrupt and good utf-8 */
    { L"file://foo/%E4%AD%E6%96%87/bar", L"file://foo/\xfffd\x6587/bar", URL_UNESCAPE_AS_UTF8 },
    { L"file://foo/%F0%9F%8D%B7/bar", L"file://foo/\xf0\x9f\x8d\xb7/bar" }, /* with 4 btyes utf-8 */
    { L"file://foo/%F0%9F%8D%B7/bar", L"file://foo/\xd83c\xdf77/bar", URL_UNESCAPE_AS_UTF8 },
    /* non-escaped chars between multi-byte escaped chars */
#if defined(__REACTOS__) && defined(_MSC_VER)
    { L"file://foo/%E4%B8%ADabc%E6%96%87/bar", L"file://foo/\x4e2d""abc""\u6587/bar", URL_UNESCAPE_AS_UTF8 },
    { L"file://foo/%E4B8%AD/bar", L"file://foo/\ufffd""B8\ufffd/bar", URL_UNESCAPE_AS_UTF8 },
    { L"file://foo/%E4%G8%AD/bar", L"file://foo/\ufffd""%G8\ufffd/bar", URL_UNESCAPE_AS_UTF8 },
#else
    { L"file://foo/%E4%B8%ADabc%E6%96%87/bar", L"file://foo/\x4e2d""abc""\x6587/bar", URL_UNESCAPE_AS_UTF8 },
    { L"file://foo/%E4B8%AD/bar", L"file://foo/\xfffd""B8\xfffd/bar", URL_UNESCAPE_AS_UTF8 },
    { L"file://foo/%E4%G8%AD/bar", L"file://foo/\xfffd""%G8\xfffd/bar", URL_UNESCAPE_AS_UTF8 },
#endif
    { L"file://foo/%G4%B8%AD/bar", L"file://foo/%G4\xfffd\xfffd/bar", URL_UNESCAPE_AS_UTF8 },
};

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
    WCHAR urlW[INTERNET_MAX_URL_LENGTH], newurlW[INTERNET_MAX_URL_LENGTH], expectW[INTERNET_MAX_URL_LENGTH];
    char newurl[INTERNET_MAX_URL_LENGTH];
    HRESULT res;
    DWORD len;
    DWORD i;

    for (i = 0; i < ARRAY_SIZE(TEST_APPLY); i++) {
        len = ARRAY_SIZE(newurl);
        strcpy(newurl, "untouched");
        res = UrlApplySchemeA(TEST_APPLY[i].url, newurl, &len, TEST_APPLY[i].flags);
        ok( res == TEST_APPLY[i].res,
            "#%ldA: got HRESULT 0x%lx (expected 0x%lx)\n", i, res, TEST_APPLY[i].res);
        if (res == S_OK)
        {
            ok(len == strlen(newurl), "Test %lu: Expected length %Iu, got %lu.\n", i, strlen(newurl), len);
            ok(!strcmp(newurl, TEST_APPLY[i].newurl), "Test %lu: Expected %s, got %s.\n",
                    i, debugstr_a(TEST_APPLY[i].newurl), debugstr_a(newurl));
        }
        else
        {
            ok(len == ARRAY_SIZE(newurl), "Test %lu: Got length %lu.\n", i, len);
            ok(!strcmp(newurl, "untouched"), "Test %lu: Got %s.\n", i, debugstr_a(newurl));
        }

        /* returned length is in character */
        MultiByteToWideChar(CP_ACP, 0, TEST_APPLY[i].url, -1, urlW, ARRAY_SIZE(urlW));
        MultiByteToWideChar(CP_ACP, 0, TEST_APPLY[i].newurl, -1, expectW, ARRAY_SIZE(expectW));

        len = ARRAY_SIZE(newurlW);
        wcscpy(newurlW, L"untouched");
        res = UrlApplySchemeW(urlW, newurlW, &len, TEST_APPLY[i].flags);
        ok( res == TEST_APPLY[i].res,
            "#%ldW: got HRESULT 0x%lx (expected 0x%lx)\n", i, res, TEST_APPLY[i].res);
        if (res == S_OK)
        {
            ok(len == wcslen(newurlW), "Test %lu: Expected length %Iu, got %lu.\n", i, wcslen(newurlW), len);
            ok(!wcscmp(newurlW, expectW), "Test %lu: Expected %s, got %s.\n",
                    i, debugstr_w(expectW), debugstr_w(newurlW));
        }
        else
        {
            ok(len == ARRAY_SIZE(newurlW), "Test %lu: Got length %lu.\n", i, len);
            ok(!wcscmp(newurlW, L"untouched"), "Test %lu: Got %s.\n", i, debugstr_w(newurlW));
        }
    }

    /* buffer too small */
    lstrcpyA(newurl, untouchedA);
    len = lstrlenA(TEST_APPLY[0].newurl);
    res = UrlApplySchemeA(TEST_APPLY[0].url, newurl, &len, TEST_APPLY[0].flags);
    ok(res == E_POINTER, "got HRESULT 0x%lx (expected E_POINTER)\n", res);
    /* The returned length include the space for the terminating 0 */
    i = lstrlenA(TEST_APPLY[0].newurl)+1;
    ok(len == i, "got len %ld (expected %ld)\n", len, i);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

    /* NULL as parameter. The length and the buffer are not modified */
    lstrcpyA(newurl, untouchedA);
    len = ARRAY_SIZE(newurl);
    res = UrlApplySchemeA(NULL, newurl, &len, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%lx (expected E_INVALIDARG)\n", res);
    ok(len == ARRAY_SIZE(newurl), "got len %ld\n", len);
    ok(!lstrcmpA(newurl, untouchedA), "got '%s' (expected '%s')\n", newurl, untouchedA);

    len = ARRAY_SIZE(newurl);
    res = UrlApplySchemeA(TEST_APPLY[0].url, NULL, &len, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%lx (expected E_INVALIDARG)\n", res);
    ok(len == ARRAY_SIZE(newurl), "got len %ld\n", len);

    lstrcpyA(newurl, untouchedA);
    res = UrlApplySchemeA(TEST_APPLY[0].url, newurl, NULL, TEST_APPLY[0].flags);
    ok(res == E_INVALIDARG, "got HRESULT 0x%lx (expected E_INVALIDARG)\n", res);
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
  res = UrlHashA(szTestUrl, (LPBYTE)&dwHash1, cbSize);
  ok(res == S_OK, "UrlHashA returned 0x%lx (expected S_OK) for %s\n", res, szUrl);

  res = UrlHashW(wszTestUrl, (LPBYTE)&dwHash2, cbSize);
  ok(res == S_OK, "UrlHashW returned 0x%lx (expected S_OK) for %s\n", res, szUrl);
  ok(dwHash1 == dwHash2,
      "Hashes didn't match (A: 0x%lx, W: 0x%lx) for %s\n", dwHash1, dwHash2, szUrl);
  FreeWideString(wszTestUrl);
}

static void test_UrlHash(void)
{
  hash_url(TEST_URL_1);
  hash_url(TEST_URL_2);
  hash_url(TEST_URL_3);
}

static void test_UrlGetPart(void)
{
    WCHAR bufferW[200];
    char buffer[200];
    unsigned int i;
    HRESULT hr;
    DWORD size;

    static const struct
    {
        const char *url;
        DWORD part;
        DWORD flags;
        HRESULT hr;
        const char *expect;
    }
    tests[] =
    {
        {"hi", URL_PART_SCHEME, 0, S_FALSE, ""},
        {"hi", URL_PART_USERNAME, 0, E_FAIL},
        {"hi", URL_PART_PASSWORD, 0, E_FAIL},
        {"hi", URL_PART_HOSTNAME, 0, E_FAIL},
        {"hi", URL_PART_PORT, 0, E_FAIL},
        {"hi", URL_PART_QUERY, 0, S_FALSE, ""},

        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_SCHEME, 0, S_OK, "http"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_USERNAME, 0, S_OK, "foo"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_PASSWORD, 0, S_OK, "bar"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_PORT, 0, S_OK, "21"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_QUERY, 0, S_OK, "query=x&return=y"},
        {"http://foo:bar@localhost:21/internal.php#anchor", URL_PART_QUERY, 0, S_FALSE, ""},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_SCHEME, URL_PARTFLAG_KEEPSCHEME, S_OK, "http"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_USERNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:foo"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_PASSWORD, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:bar"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:localhost"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_PORT, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:21"},
        {"http://foo:bar@localhost:21/internal.php?query=x&return=y", URL_PART_QUERY, URL_PARTFLAG_KEEPSCHEME, S_OK, "query=x&return=y"},

        {"http://localhost/", URL_PART_USERNAME, 0, E_INVALIDARG},
        {"http://localhost/", URL_PART_PASSWORD, 0, E_INVALIDARG},
        {"http://localhost/", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http://localhost/", URL_PART_PORT, 0, E_INVALIDARG},
        {"http://localhost/", URL_PART_QUERY, 0, S_FALSE, ""},

        {"http://localhost:port/", URL_PART_USERNAME, 0, E_INVALIDARG},
        {"http://localhost:port/", URL_PART_PASSWORD, 0, E_INVALIDARG},
        {"http://localhost:port/", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http://localhost:port/", URL_PART_PORT, 0, S_OK, "port"},
        {"http://:", URL_PART_HOSTNAME, 0, S_FALSE, ""},
        {"http://:", URL_PART_PORT, 0, S_FALSE, ""},

        {"http://user@localhost", URL_PART_USERNAME, 0, S_OK, "user"},
        {"http://user@localhost", URL_PART_PASSWORD, 0, E_INVALIDARG},
        {"http://user@localhost", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http://user@localhost", URL_PART_PORT, 0, E_INVALIDARG},
        {"http://@", URL_PART_USERNAME, 0, S_FALSE, ""},
        {"http://@", URL_PART_PASSWORD, 0, E_INVALIDARG},
        {"http://@", URL_PART_HOSTNAME, 0, S_FALSE, ""},

        {"http://user:pass@localhost", URL_PART_USERNAME, 0, S_OK, "user"},
        {"http://user:pass@localhost", URL_PART_PASSWORD, 0, S_OK, "pass"},
        {"http://user:pass@localhost", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http://user:pass@localhost", URL_PART_PORT, 0, E_INVALIDARG},
        {"http://:@", URL_PART_USERNAME, 0, S_FALSE, ""},
        {"http://:@", URL_PART_PASSWORD, 0, S_FALSE, ""},
        {"http://:@", URL_PART_HOSTNAME, 0, S_FALSE, ""},

        {"http://host:port:q", URL_PART_HOSTNAME, 0, S_OK, "host"},
        {"http://host:port:q", URL_PART_PORT, 0, S_OK, "port:q"},
        {"http://user:pass:q@host", URL_PART_USERNAME, 0, S_OK, "user"},
        {"http://user:pass:q@host", URL_PART_PASSWORD, 0, S_OK, "pass:q"},
        {"http://user@host@q", URL_PART_USERNAME, 0, S_OK, "user"},
        {"http://user@host@q", URL_PART_HOSTNAME, 0, S_OK, "host@q"},

        {"http:localhost/index.html", URL_PART_HOSTNAME, 0, E_FAIL},
        {"http:/localhost/index.html", URL_PART_HOSTNAME, 0, E_FAIL},

        {"http://localhost\\index.html", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http:/\\localhost/index.html", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"http:\\/localhost/index.html", URL_PART_HOSTNAME, 0, S_OK, "localhost"},

        {"ftp://localhost\\index.html", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"ftp:/\\localhost/index.html", URL_PART_HOSTNAME, 0, S_OK, "localhost"},
        {"ftp:\\/localhost/index.html", URL_PART_HOSTNAME, 0, S_OK, "localhost"},

        {"http://host?a:b@c:d", URL_PART_HOSTNAME, 0, S_OK, "host"},
        {"http://host?a:b@c:d", URL_PART_QUERY, 0, S_OK, "a:b@c:d"},
        {"http://host#a:b@c:d", URL_PART_HOSTNAME, 0, S_OK, "host"},
        {"http://host#a:b@c:d", URL_PART_QUERY, 0, S_FALSE, ""},

        /* All characters, other than those with special meaning, are allowed. */
        {"http://foo:bar@google.*.com:21/internal.php?query=x&return=y", URL_PART_HOSTNAME, 0, S_OK, "google.*.com"},
        {"http:// !\"$%&'()*+,-.;<=>[]^_`{|~}:pass@host", URL_PART_USERNAME, 0, S_OK, " !\"$%&'()*+,-.;<=>[]^_`{|~}"},
        {"http://user: !\"$%&'()*+,-.;<=>[]^_`{|~}@host", URL_PART_PASSWORD, 0, S_OK, " !\"$%&'()*+,-.;<=>[]^_`{|~}"},
        {"http:// !\"$%&'()*+,-.;<=>[]^_`{|~}", URL_PART_HOSTNAME, 0, S_OK, " !\"$%&'()*+,-.;<=>[]^_`{|~}"},
        {"http://host: !\"$%&'()*+,-.;<=>[]^_`{|~}", URL_PART_PORT, 0, S_OK, " !\"$%&'()*+,-.;<=>[]^_`{|~}"},

        {"http:///index.html", URL_PART_HOSTNAME, 0, S_FALSE, ""},
        {"http:///index.html", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:"},
        {"file://h o s t/c:/windows/file", URL_PART_HOSTNAME, 0, S_OK, "h o s t"},
        {"file://h o s t/c:/windows/file", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "h o s t"},
        {"file://foo:bar@localhost:21/file?query=x", URL_PART_USERNAME, 0, E_FAIL},
        {"file://foo:bar@localhost:21/file?query=x", URL_PART_PASSWORD, 0, E_FAIL},
        {"file://foo:bar@localhost:21/file?query=x", URL_PART_HOSTNAME, 0, S_OK, "foo:bar@localhost:21"},
        {"file://foo:bar@localhost:21/file?query=x", URL_PART_PORT, 0, E_FAIL},
        {"file://foo:bar@localhost:21/file?query=x", URL_PART_QUERY, 0, S_OK, "query=x"},
        {"http://user:pass 123@www.wine hq.org", URL_PART_HOSTNAME, 0, S_OK, "www.wine hq.org"},
        {"http://user:pass 123@www.wine hq.org", URL_PART_PASSWORD, 0, S_OK, "pass 123"},
        {"about:blank", URL_PART_SCHEME, 0, S_OK, "about"},
        {"about:blank", URL_PART_HOSTNAME, 0, E_FAIL},
        {"x-excid://36C00000/guid:{048B4E89-2E92-496F-A837-33BA02FF6D32}/Message.htm", URL_PART_SCHEME, 0, S_OK, "x-excid"},
        {"x-excid://36C00000/guid:{048B4E89-2E92-496F-A837-33BA02FF6D32}/Message.htm", URL_PART_HOSTNAME, 0, E_FAIL},
        {"x-excid://36C00000/guid:{048B4E89-2E92-496F-A837-33BA02FF6D32}/Message.htm", URL_PART_QUERY, 0, S_FALSE, ""},
        {"foo://bar-url/test", URL_PART_SCHEME, 0, S_OK, "foo"},
        {"foo://bar-url/test", URL_PART_HOSTNAME, 0, E_FAIL},
        {"foo://bar-url/test", URL_PART_QUERY, 0, S_FALSE, ""},
        {"ascheme:", URL_PART_SCHEME, 0, S_OK, "ascheme"},
        {"res://some.dll/find.dlg", URL_PART_SCHEME, 0, S_OK, "res"},
        {"res://some.dll/find.dlg", URL_PART_QUERY, 0, S_FALSE, ""},
        {"http://www.winehq.org", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:www.winehq.org"},
        {"file:///index.html", URL_PART_HOSTNAME, 0, S_FALSE, ""},
        {"file:///index.html", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_FALSE, ""},
        {"file://c:\\index.htm", URL_PART_HOSTNAME, 0, S_FALSE, ""},
        {"file://c:\\index.htm", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_FALSE, ""},
        {"file:some text", URL_PART_HOSTNAME, 0, S_FALSE, ""},
        {"index.htm", URL_PART_HOSTNAME, 0, E_FAIL},
        {"sChEmE-.+:", URL_PART_SCHEME, 0, S_OK, "scheme-.+"},
        {"scheme_:", URL_PART_SCHEME, 0, S_FALSE, ""},
        {"scheme :", URL_PART_SCHEME, 0, S_FALSE, ""},
        {"sch eme:", URL_PART_SCHEME, 0, S_FALSE, ""},
        {":", URL_PART_SCHEME, 0, S_FALSE, ""},
        {"a:", URL_PART_SCHEME, 0, S_FALSE, ""},
        {"0:", URL_PART_SCHEME, 0, S_FALSE, ""},
        {"ab:", URL_PART_SCHEME, 0, S_OK, "ab"},

        {"about://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"file://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"ftp://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"gopher://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"http://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"https://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"javascript://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"local://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"mailto://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"mk://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"news://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"nntp://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"res://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"shell://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"snews://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"telnet://hostname/", URL_PART_HOSTNAME, 0, S_OK, "hostname"},
        {"vbscript://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},
        {"wais://hostname/", URL_PART_HOSTNAME, 0, E_FAIL},

        {"file://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "hostname"},
        {"ftp://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "ftp:hostname"},
        {"gopher://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "gopher:hostname"},
        {"http://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "http:hostname"},
        {"https://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "https:hostname"},
        {"news://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "news:hostname"},
        {"nntp://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "nntp:hostname"},
        {"snews://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "snews:hostname"},
        {"telnet://hostname/", URL_PART_HOSTNAME, URL_PARTFLAG_KEEPSCHEME, S_OK, "telnet:hostname"},
    };

    hr = UrlGetPartA(NULL, NULL, NULL, URL_PART_SCHEME, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = UrlGetPartA(NULL, buffer, &size, URL_PART_SCHEME, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = UrlGetPartA("res://some.dll/find.dlg", NULL, &size, URL_PART_SCHEME, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    hr = UrlGetPartA("res://some.dll/find.dlg", buffer, NULL, URL_PART_SCHEME, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    size = 0;
    strcpy(buffer, "x");
    hr = UrlGetPartA("hi", buffer, &size, URL_PART_SCHEME, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);
    ok(!strcmp(buffer, "x"), "Got result %s.\n", debugstr_a(buffer));
    ok(!size, "Got size %lu.\n", size);
#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) < 6) {
        skip("UrlGetPart test list broken on WS03.\n");
        goto skip_UrlGetPartTestList;
    }
#endif

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        WCHAR urlW[200], expectW[200];
        const char *expect = tests[i].expect;
        const char *url = tests[i].url;
        DWORD flags = tests[i].flags;
        DWORD part = tests[i].part;

        winetest_push_context("URL %s, part %#lx, flags %#lx", debugstr_a(url), part, flags);

        size = 1;
        strcpy(buffer, "x");
        hr = UrlGetPartA(url, buffer, &size, part, flags);
        if (tests[i].hr == S_OK)
            ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
        else
            ok(hr == tests[i].hr, "Got hr %#lx.\n", hr);

        if (hr == S_FALSE)
        {
            ok(!size, "Got size %lu.\n", size);
            ok(!buffer[0], "Got result %s.\n", debugstr_a(buffer));
        }
        else
        {
            if (hr == E_POINTER)
                ok(size == strlen(expect) + 1, "Got size %lu.\n", size);
            else
                ok(size == 1, "Got size %lu.\n", size);
            ok(!strcmp(buffer, "x"), "Got result %s.\n", debugstr_a(buffer));
        }

        size = sizeof(buffer);
        strcpy(buffer, "x");
        hr = UrlGetPartA(url, buffer, &size, part, flags);
        ok(hr == tests[i].hr, "Got hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            ok(size == strlen(buffer), "Got size %lu.\n", size);
            ok(!strcmp(buffer, expect), "Got result %s.\n", debugstr_a(buffer));
        }
        else
        {
            ok(size == sizeof(buffer), "Got size %lu.\n", size);
            ok(!strcmp(buffer, "x"), "Got result %s.\n", debugstr_a(buffer));
        }

        MultiByteToWideChar(CP_ACP, 0, url, -1, urlW, ARRAY_SIZE(urlW));

        size = 1;
        wcscpy(bufferW, L"x");
        hr = UrlGetPartW(urlW, bufferW, &size, part, flags);
        if (tests[i].hr == S_OK)
            ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
        else
            ok(hr == (tests[i].hr == S_FALSE ? S_OK : tests[i].hr), "Got hr %#lx.\n", hr);

        if (SUCCEEDED(hr))
        {
            ok(!size, "Got size %lu.\n", size);
            ok(!buffer[0], "Got result %s.\n", debugstr_a(buffer));
        }
        else
        {
            if (hr == E_POINTER)
                ok(size == strlen(expect) + 1, "Got size %lu.\n", size);
            else
                ok(size == 1, "Got size %lu.\n", size);
            ok(!wcscmp(bufferW, L"x"), "Got result %s.\n", debugstr_w(bufferW));
        }

        size = ARRAY_SIZE(bufferW);
        wcscpy(bufferW, L"x");
        hr = UrlGetPartW(urlW, bufferW, &size, part, flags);
        ok(hr == (tests[i].hr == S_FALSE ? S_OK : tests[i].hr), "Got hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            ok(size == wcslen(bufferW), "Got size %lu.\n", size);
            MultiByteToWideChar(CP_ACP, 0, buffer, -1, expectW, ARRAY_SIZE(expectW));
            ok(!wcscmp(bufferW, expectW), "Got result %s.\n", debugstr_w(bufferW));
        }
        else
        {
            ok(size == ARRAY_SIZE(bufferW), "Got size %lu.\n", size);
            ok(!wcscmp(bufferW, L"x"), "Got result %s.\n", debugstr_w(bufferW));
        }

        winetest_pop_context();
    }

    /* Test non-ASCII characters. */
#ifdef __REACTOS__
skip_UrlGetPartTestList:
#endif

    size = ARRAY_SIZE(bufferW);
    wcscpy(bufferW, L"x");
    hr = UrlGetPartW(L"http://\x01\x7f\x80\xff:pass@host", bufferW, &size, URL_PART_USERNAME, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == wcslen(bufferW), "Got size %lu.\n", size);
    ok(!wcscmp(bufferW, L"\x01\x7f\x80\xff"), "Got result %s.\n", debugstr_w(bufferW));
}

/* ########################### */
static void check_url_canonicalize(const char *url, DWORD flags, const char *expect)
{
    char output[INTERNET_MAX_URL_LENGTH];
    WCHAR outputW[INTERNET_MAX_URL_LENGTH];
    WCHAR *urlW = GetWideString(url);
    WCHAR *expectW;
    HRESULT hr;
    DWORD size;

    winetest_push_context("URL %s, flags %#lx", debugstr_a(url), flags);

    size = INTERNET_MAX_URL_LENGTH;
    hr = UrlCanonicalizeA(url, NULL, &size, flags);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = UrlCanonicalizeA(url, output, &size, flags);
    ok(hr == S_OK || (!url[0] && hr == S_FALSE) /* Vista+ */, "Got unexpected hr %#lx.\n", hr);
    ok(!strcmp(output, expect), "Expected %s, got %s.\n", debugstr_a(expect), debugstr_a(output));

    size = INTERNET_MAX_URL_LENGTH;
    hr = UrlCanonicalizeW(urlW, NULL, &size, flags);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    hr = UrlCanonicalizeW(urlW, outputW, &size, flags);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    expectW = GetWideString(output);
    ok(!wcscmp(outputW, expectW), "Expected %s, got %s.\n", debugstr_w(expectW), debugstr_w(outputW));
    FreeWideString(expectW);

    FreeWideString(urlW);

    winetest_pop_context();
}


static void test_UrlEscapeA(void)
{
    DWORD size = 0;
    HRESULT ret;
    unsigned int i;
    char empty_string[] = "";

    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 0, "got %ld, expected %d\n", size, 0);

    size = 0;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 0, "got %ld, expected %d\n", size, 0);

    size = 1;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 1, "got %ld, expected %d\n", size, 1);

    size = 1;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 1, "got %ld, expected %d\n", size, 1);

    size = 1;
    empty_string[0] = 127;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_POINTER, "got %lx, expected %lx\n", ret, E_POINTER);
    ok(size == 34, "got %ld, expected %d\n", size, 34);
    ok(empty_string[0] == 127, "String has changed, empty_string[0] = %d\n", empty_string[0]);

    size = 1;
    empty_string[0] = 127;
    ret = UrlEscapeA("/woningplan/woonkamer basis.swf", empty_string, &size, URL_ESCAPE_AS_UTF8);
#ifdef __REACTOS__
    ok(ret == E_NOTIMPL || broken(ret == E_POINTER) /* Vista */, "Got unexpected hr %#lx.\n", ret);
    ok(size == 1 || broken(size == 34) /* Vista */, "Got unexpected size %lu.\n", size);
#else
    ok(ret == E_NOTIMPL, "Got unexpected hr %#lx.\n", ret);
    ok(size == 1, "Got unexpected size %lu.\n", size);
#endif
    ok(empty_string[0] == 127, "String has changed, empty_string[0] = %d\n", empty_string[0]);

    for (i = 0; i < ARRAY_SIZE(TEST_ESCAPE); i++) {
        CHAR ret_url[INTERNET_MAX_URL_LENGTH];

        size = INTERNET_MAX_URL_LENGTH;
        ret = UrlEscapeA(TEST_ESCAPE[i].url, ret_url, &size, TEST_ESCAPE[i].flags);
        ok(ret == S_OK, "Got unexpected hr %#lx for %s.\n", ret, debugstr_a(TEST_ESCAPE[i].url));
        ok(!strcmp(ret_url, TEST_ESCAPE[i].expecturl), "Expected \"%s\", but got \"%s\" for \"%s\"\n",
            TEST_ESCAPE[i].expecturl, ret_url, TEST_ESCAPE[i].url);
    }
}

static void test_UrlEscapeW(void)
{
    WCHAR ret_urlW[INTERNET_MAX_URL_LENGTH];
    WCHAR overwrite[10] = L"foo bar";
    WCHAR empty_string[] = {0};
    DWORD size;
    HRESULT ret;
    WCHAR wc;
    int i;
#ifdef __REACTOS__
    DWORD _ntVersion = GetVersion();
    BYTE _ntMajor = LOBYTE(LOWORD(_ntVersion));
    BYTE _ntMinor = HIBYTE(LOWORD(_ntVersion));
#endif

    /* Check error paths */

    ret = UrlEscapeW(L"/test", NULL, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);

    size = 0;
    ret = UrlEscapeW(L"/test", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 0, "got %ld, expected %d\n", size, 0);

    ret = UrlEscapeW(L"/test", empty_string, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);

    size = 0;
    ret = UrlEscapeW(L"/test", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 0, "got %ld, expected %d\n", size, 0);

    ret = UrlEscapeW(L"/test", NULL, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);

    size = 1;
    ret = UrlEscapeW(L"/test", NULL, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);
    ok(size == 1, "got %ld, expected %d\n", size, 1);

    ret = UrlEscapeW(L"/test", empty_string, NULL, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_INVALIDARG, "got %lx, expected %lx\n", ret, E_INVALIDARG);

    size = 1;
    ret = UrlEscapeW(L"/test", empty_string, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_POINTER, "got %lx, expected %lx\n", ret, E_POINTER);
    ok(size == 6, "got %ld, expected %d\n", size, 6);

    /* Check actual escaping */

    size = ARRAY_SIZE(overwrite);
    ret = UrlEscapeW(overwrite, overwrite, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == S_OK, "got %lx, expected S_OK\n", ret);
    ok(size == 9, "got %ld, expected 9\n", size);
    ok(!wcscmp(overwrite, L"foo%20bar"), "Got unexpected string %s.\n", debugstr_w(overwrite));

    size = 1;
    wc = 127;
    ret = UrlEscapeW(overwrite, &wc, &size, URL_ESCAPE_SPACES_ONLY);
    ok(ret == E_POINTER, "got %lx, expected %lx\n", ret, E_POINTER);
    ok(size == 10, "got %ld, expected 10\n", size);
    ok(wc == 127, "String has changed, wc = %d\n", wc);

    /* non-ASCII range */
    size = ARRAY_SIZE(ret_urlW);
    ret = UrlEscapeW(L"ftp\x1f\xff\xfa\x2122q/", ret_urlW, &size, 0);
    ok(ret == S_OK, "got %lx, expected S_OK\n", ret);
    ok(!wcscmp(ret_urlW, L"ftp%1F%FF%FA\x2122q/"), "Got unexpected string %s.\n", debugstr_w(ret_urlW));

    for (i = 0; i < ARRAY_SIZE(TEST_ESCAPE); i++) {

        WCHAR *urlW, *expected_urlW;

        size = INTERNET_MAX_URL_LENGTH;
        urlW = GetWideString(TEST_ESCAPE[i].url);
        expected_urlW = GetWideString(TEST_ESCAPE[i].expecturl);
        ret = UrlEscapeW(urlW, ret_urlW, &size, TEST_ESCAPE[i].flags);
        ok(ret == S_OK, "Got unexpected hr %#lx for %s.\n", ret, debugstr_w(urlW));
        ok(!lstrcmpW(ret_urlW, expected_urlW), "Expected %s, but got %s for %s flags %08lx\n",
            wine_dbgstr_w(expected_urlW), wine_dbgstr_w(ret_urlW), wine_dbgstr_w(urlW), TEST_ESCAPE[i].flags);
        FreeWideString(urlW);
        FreeWideString(expected_urlW);
    }

    for (i = 0; i < ARRAY_SIZE(TEST_ESCAPEW); i++) {
        WCHAR ret_url[INTERNET_MAX_URL_LENGTH];

#ifdef __REACTOS__
        /* TEST_ESCAPEW[6, 7] is incorrect for WS03, Vista. */
        if ((i == 6 || i == 7) && (_ntMajor < 6 || (_ntMajor == 6 && _ntMinor == 0)))
            continue;
#endif
        size = INTERNET_MAX_URL_LENGTH;
        ret = UrlEscapeW(TEST_ESCAPEW[i].url, ret_url, &size, TEST_ESCAPEW[i].flags);
        ok(ret == S_OK, "Got unexpected hr %#lx for %s.\n", ret, debugstr_w(TEST_ESCAPEW[i].url));
        ok(!wcscmp(ret_url, TEST_ESCAPEW[i].expecturl)
                || broken(!wcscmp(ret_url, TEST_ESCAPEW[i].win7url)),
                "Expected %s, but got %s for %s.\n", debugstr_w(TEST_ESCAPEW[i].expecturl),
                debugstr_w(ret_url), debugstr_w(TEST_ESCAPEW[i].url));
    }
}

struct canonicalize_test
{
    const char *url;
    DWORD flags;
    const char *expect;
};

static void test_UrlCanonicalizeA(void)
{
    CHAR szReturnUrl[4*INTERNET_MAX_URL_LENGTH];
    CHAR longurl[4*INTERNET_MAX_URL_LENGTH];
    char url[200], expect[200];
    unsigned int f, i, j;
    DWORD dwSize;
    DWORD urllen;
    HRESULT hr;

    static const struct canonicalize_test unk_scheme_tests[] =
    {
        /* Single and double dots behave as one would expect, with the following
         * notable rules:
         *
         * (1) A single or double dot as the first element (the "hostname") is
         *     always emitted as-is.
         *
         * (2) If a double dot would undo the hostname, it is emitted as-is
         *     instead.
         *
         * (3) If a single or double dot is the last element (either because of
         *     the above rule or because of URL_DONT_SIMPLIFY), a trailing
         *     backslash is appended.
         *
         * A trailing backslash is always appended after the hostname.
         */

        {"//", 0, "///"},
        {"//a", 0, "//a/"},
        {"//a/", 0, "//a/"},
        {"//a/b", 0, "//a/b"},
        {"//a/b/", 0, "//a/b/"},
        {"//.", 0, "//./"},
        {"//./", 0, "//./"},
        {"//./a", 0, "//./a"},
        {"//././a", 0, "//./a"},
        {"//a/.", 0, "//a/"},
        {"//a/./", 0, "//a/"},
        {"//a/./b", 0, "//a/b"},
        {"///./a", 0, "///a"},
        {"//a/.b/", 0, "//a/.b/"},
        {"//a/b./", 0, "//a/b./"},

        {"//..", 0, "//../"},
        {"//../", 0, "//../"},
        {"//../a", 0, "//../a"},
        {"//../a/..", 0, "//../"},
        {"//.././..", 0, "//../../"},
        {"//../a/../..", 0, "//../../"},
        {"//./a/../..", 0, "//./../"},
        {"//a/..", 0, "//a/../"},
        {"//a/../../b/./c/..", 0, "//a/../../b/"},
        {"//a/b/..", 0, "//a/"},
        {"//a/b/...", 0, "//a/b/..."},
        {"//a/b/../", 0, "//a/"},
        {"//a/b/../c", 0, "//a/c"},
        {"//a/b/../c/..", 0, "//a/"},
        {"//a/b/../c/../..", 0, "//a/../"},
        {"//a/b/../../../c", 0, "//a/../../c"},
        {"///..", 0, "///../"},
        {"////..", 0, "///"},
        {"//a/..b/", 0, "//a/..b/"},
        {"//a/b../", 0, "//a/b../"},
        {"//A/B", 0, "//A/B"},

        {"//././a", URL_DONT_SIMPLIFY, "//././a"},
        {"//a/.", URL_DONT_SIMPLIFY, "//a/./"},
        {"//a/./", URL_DONT_SIMPLIFY, "//a/./"},
        {"//a/./b", URL_DONT_SIMPLIFY, "//a/./b"},
        {"///./a", URL_DONT_SIMPLIFY, "///./a"},

        {"//..", URL_DONT_SIMPLIFY, "//../"},
        {"//../", URL_DONT_SIMPLIFY, "//../"},
        {"//../a", URL_DONT_SIMPLIFY, "//../a"},
        {"//../a/..", URL_DONT_SIMPLIFY, "//../a/../"},
        {"//../a/...", URL_DONT_SIMPLIFY, "//../a/..."},
        {"//.././..", URL_DONT_SIMPLIFY, "//.././../"},
        {"//../a/../..", URL_DONT_SIMPLIFY, "//../a/../../"},
        {"//./a/../..", URL_DONT_SIMPLIFY, "//./a/../../"},
        {"//a/..", URL_DONT_SIMPLIFY, "//a/../"},
        {"//a/../../b/./c/..", URL_DONT_SIMPLIFY, "//a/../../b/./c/../"},
        {"//a/b/..", URL_DONT_SIMPLIFY, "//a/b/../"},
        {"//a/b/../", URL_DONT_SIMPLIFY, "//a/b/../"},
        {"//a/b/../c", URL_DONT_SIMPLIFY, "//a/b/../c"},
        {"//a/b/../c/..", URL_DONT_SIMPLIFY, "//a/b/../c/../"},
        {"//a/b/../c/../..", URL_DONT_SIMPLIFY, "//a/b/../c/../../"},
        {"///..", URL_DONT_SIMPLIFY, "///../"},
        {"////..", URL_DONT_SIMPLIFY, "////../"},

        /* After ? or #, dots are not simplified. */
        {"//a/b?c/./d", 0, "//a/b?c/./d"},
        {"//a/b#c/./d", 0, "//a/b#c/./d"},
        {"//a/b#c/.", 0, "//a/b#c/."},
        /* ? and # can also be considered a boundary for trailing dots. */
        {"//a/b/.?", 0, "//a/b/?"},
        {"//a/b/..?", 0, "//a/?"},
        {"//a/b/..?", URL_DONT_SIMPLIFY, "//a/b/../?"},
        {"//a/b/.#", 0, "//a/b/#"},
        {"//a/b/..#", 0, "//a/#"},
        {"//a/b/..#", URL_DONT_SIMPLIFY, "//a/b/../#"},
        {"//a/..?", 0, "//a/../?"},
        {"//a/..#", 0, "//a/../#"},
        {"//..?", 0, "//../?"},
        {"//..#", 0, "//../#"},
        {"//?/a/./", 0, "///?/a/./"},
        {"//#/a/./", 0, "///#/a/./"},
        /* The first ? is reordered before the first #. */
        {"//a/b#c?d", 0, "//a/b?d#c"},
        {"//a/b?c#d?e", 0, "//a/b?c#d?e"},
        {"//a/b#c?d#e", 0, "//a/b?d#e#c"},
        {"//a/b#c#d?e", 0, "//a/b?e#c#d"},
        {"//a/b#c?d?e", 0, "//a/b?d?e#c"},

        /* Backslashes are not treated as path separators. */
        {"//a/b\\c/../.\\", 0, "//a/.\\"},
        {"//a\\b/../", 0, "//a\\b/../"},
        {"//a/b\\../", 0, "//a/b\\../"},
        {"//a/b/..\\", 0, "//a/b/..\\"},

        /* Whitespace and unsafe characters are not (by default) escaped. */
        {"//a/b &c", 0, "//a/b &c"},

        /* If one slash is omitted, the rules are much the same, except that
         * there is no "hostname". Single dots are always collapsed; double dots
         * are collapsed unless they would undo the "scheme". */

        {"/a", 0, "/a"},
        {"/a/", 0, "/a/"},
        {"/.", 0, "/"},
        {"/./", 0, "/"},
        {"/././a", 0, "/a"},
        {"/a/.", 0, "/a/"},
        {"/a/./", 0, "/a/"},
        {"/a/./b", 0, "/a/b"},

        {"/..", 0, "/../"},
        {"/../", 0, "/../"},
        {"/../a", 0, "/../a"},
        {"/../a/..", 0, "/../"},
        {"/a/..", 0, "/"},
        {"/a/../..", 0, "/../"},
        {"/a/b/..", 0, "/a/"},
        {"/a/b/../", 0, "/a/"},
        {"/a/b/../c", 0, "/a/c"},
        {"/a/b/../c/..", 0, "/a/"},
        {"/a/b/../c/../..", 0, "/"},

        {"/a/b?c/./d", 0, "/a/b?c/./d"},
        {"/a/b#c/./d", 0, "/a/b#c/./d"},
        {"/a/b#c?d", 0, "/a/b?d#c"},

        /* Just as above, backslashes are not treated as path separators. */
        {"/a/b\\c/../.\\", 0, "/a/.\\"},
        {"/a/b\\/c", 0, "/a/b\\/c"},
        {"/a/b\\.c", 0, "/a/b\\.c"},
        /* If the first character after the slash is a backslash, it is skipped.
         * It is not interpreted as a forward slash.
         * The tests above show that this is not due to the backslash being
         * interpreted as an escape character. */
        {"/\\././a", 0, "/a"},
        /* The sequence /\/ does not result in use of the double-slash rules.
         * Rather, the resulting // is treated as an empty path element. */
        {"/\\/././a", 0, "//a"},
        {"/\\/././a/", 0, "//a/"},
        {"/\\/..", 0, "/"},
        {"//a/\\b", 0, "//a/\\b"},

        {"/a/b &c", 0, "/a/b &c"},

        {"//a/b%20%26c", URL_UNESCAPE, "//a/b &c"},
    };

    static const struct
    {
        const char *url;
        DWORD flags;
        const char *expect;
        const char *expect_ftp;
    }
    http_tests[] =
    {
        /* A set of schemes including http differs from the "default" behaviour
         * in the following ways:
         *
         * (1) If a double dot would undo the hostname, it is dropped instead.
         *
         * (2) If the first element after the hostname is a single or double
         *     dot, no further dots are simplified.
         *
         * (3) Trailing backslashes are not automatically appended after dots.
         */

        {"//", 0, "///"},
        {"//a", 0, "//a/"},
        {"//a/", 0, "//a/"},
        {"//a/b", 0, "//a/b"},
        {"//a/b/", 0, "//a/b/"},
        {"//.", 0, "//./"},
        {"//./", 0, "//./"},
        {"//././a/.", 0, "//././a/."},
        {"//a/.", 0, "//a/."},
        {"//a/./b/./../", 0, "//a/./b/./../"},
        {"//a/b/.", 0, "//a/b/"},
        {"//a/b/.", URL_DONT_SIMPLIFY, "//a/b/."},
        {"//a/b/./", 0, "//a/b/"},
        {"//a/b/./c", 0, "//a/b/c"},
        {"///./a", 0, "///./a"},
        {"////./a", 0, "////a"},

        {"//..", 0, "//../"},
        {"//../", 0, "//../"},
        {"//../a", 0, "//../a"},
        {"//../a/..", 0, "//../"},
        {"//../a/../..", 0, "//../"},
        {"//./a/../..", 0, "//./"},
        {"//a/../", 0, "//a/../"},
        {"//a/../../b/./../", 0, "//a/../../b/./../"},
        {"//a/.././", 0, "//a/.././"},
        {"//a/b/..", 0, "//a/"},
        {"//a/b/..", URL_DONT_SIMPLIFY, "//a/b/.."},
        {"//a/b/../", 0, "//a/"},
        {"//a/b/.././", 0, "//a/"},
        {"//a/b/../c", 0, "//a/c"},
        {"//a/b/../c/..", 0, "//a/"},
        {"//a/b/../c/../..", 0, "//a/"},
        {"//a/b/../../../c", 0, "//a/c"},
        {"///a/.", 0, "///a/"},
        {"///..", 0, "///.."},
        {"////..", 0, "///"},
        {"//a//../../..", 0, "//a/"},

        {"//a/b?c/./d", 0, "//a/b?c/./d"},
        {"//a/b#c/./d", 0, "//a/b#c/./d"},
        {"//a/b#c?d", 0, "//a/b#c?d"},
        {"//a/b?c#d", 0, "//a/b?c#d"},

        {"//localhost/b", 0, "//localhost/b"},

        /* Most of these schemes translates backslashes to forward slashes,
         * including the initial pair, and interpret them appropriately.
         *
         * A few schemes, including ftp, don't translate backslashes to forward
         * slashes, but still interpret them as path separators, with the
         * exception that the hostname must end in a forward slash. */

        {"//a/b\\", 0, "//a/b/", "//a/b\\"},
        {"//a/b\\./c", 0, "//a/b/c", "//a/b\\c"},
        {"//a/b/.\\c", 0, "//a/b/c"},
        {"//a/b\\c/../.\\", 0, "//a/b/", "//a/b\\"},
        {"//a\\b", 0, "//a/b", "//a\\b/"},
        {"//a\\b/..", 0, "//a/", "//a\\b/.."},
        {"//a/b\\c", 0, "//a/b/c", "//a/b\\c"},
        {"/\\a\\..", 0, "//a/..", "/\\a\\../"},
        {"\\/a\\..", 0, "//a/..", "\\/a\\../"},

        {"//a/b &c", 0, "//a/b &c"},

        /* If one or both slashes is missing, the portion after the colon is
         * treated like a normal path, without a hostname. Single and double
         * dots are always collapsed, and double dots which would rewind past
         * the scheme are dropped instead. */

        {"a", 0, "a"},
        {"a/", 0, "a/"},
        {"a/.", 0, "a/"},
        {"a/..", 0, ""},
        {"a/../..", 0, ""},
        {"a/../..", URL_DONT_SIMPLIFY, "a/../.."},
        {"", 0, ""},
        {"/", 0, "/"},
        {"/.", 0, "/"},
        {"/..", 0, ""},
        {"/../..", 0, ""},
        {".", 0, ""},
        {"..", 0, ""},
        {"./", 0, ""},
        {"../", 0, ""},

        {"a/b?c/.\\d", 0, "a/b?c/.\\d"},
        {"a/b#c/.\\d", 0, "a/b#c/.\\d"},

        {"a\\b\\", 0, "a/b/", "a\\b\\"},

        {"a/b &c", 0, "a/b &c"},

        {"/foo/bar/baz", URL_ESCAPE_SEGMENT_ONLY, "/foo/bar/baz"},
        {"/foo/bar/baz?a#b", URL_ESCAPE_SEGMENT_ONLY, "/foo/bar/baz?a#b"},

        {"//www.winehq.org/tests\n", URL_ESCAPE_SPACES_ONLY | URL_ESCAPE_UNSAFE, "//www.winehq.org/tests"},
        {"//www.winehq.org/tests\r", URL_ESCAPE_SPACES_ONLY | URL_ESCAPE_UNSAFE, "//www.winehq.org/tests"},
        {"//www.winehq.org/tests/foo bar", URL_ESCAPE_SPACES_ONLY | URL_DONT_ESCAPE_EXTRA_INFO, "//www.winehq.org/tests/foo%20bar"},
        {"//www.winehq.org/tests/foo%20bar", 0, "//www.winehq.org/tests/foo%20bar"},
        {"//www.winehq.org/tests/foo%20bar", URL_UNESCAPE, "//www.winehq.org/tests/foo bar"},
        {"//www.winehq.org/%E6%A1%9C.html", 0, "//www.winehq.org/%E6%A1%9C.html"},
    };

    static const struct canonicalize_test opaque_tests[] =
    {
        /* Opaque protocols, predictably, do not modify the portion after the
         * scheme. */
        {"//a/b/./c/../d\\e", 0, "//a/b/./c/../d\\e"},
        {"/a/b/./c/../d\\e", 0, "/a/b/./c/../d\\e"},
        {"a/b/./c/../d\\e", 0, "a/b/./c/../d\\e"},
        {"", 0, ""},
        {"//a/b &c", 0, "//a/b &c"},
        {"//a/b%20%26c", URL_UNESCAPE, "//a/b &c"},
    };

    static const struct canonicalize_test file_tests[] =
    {
        /* file:// is almost identical to http://, except that a URL beginning
         * with file://// (four or more slashes) is stripped down to two
         * slashes. The first non-empty element is interpreted as a hostname;
         * and the rest follows the usual rules.
         *
         * The intent here is probably to detect UNC paths, although it's
         * unclear why an arbitrary number of slashes are skipped in that case.
         */

        {"file://", 0, "file:///"},
        {"file://a", 0, "file://a/"},
        {"file://a/", 0, "file://a/"},
        {"file://a//", 0, "file://a//"},
        {"file://a/b", 0, "file://a/b"},
        {"file://a/b/", 0, "file://a/b/"},
        {"file://.", 0, "file://./"},
        {"file://./", 0, "file://./"},
        {"file://././a/.", 0, "file://././a/."},
        {"file://a/.", 0, "file://a/."},
        {"file://a/./b/./../", 0, "file://a/./b/./../"},
        {"file://a/b/.", 0, "file://a/b/"},
        {"file://a/b/.", URL_DONT_SIMPLIFY, "file://a/b/."},
        {"file://a/b/./", 0, "file://a/b/"},
        {"file://a/b/./c", 0, "file://a/b/c"},
        {"file:///./a", 0, "file:///./a"},
        {"file:////./a", 0, "file://./a"},

        {"file://..", 0, "file://../"},
        {"file://../", 0, "file://../"},
        {"file://../a", 0, "file://../a"},
        {"file://../a/..", 0, "file://../"},
        {"file://../a/../..", 0, "file://../"},
        {"file://./a/../..", 0, "file://./"},
        {"file://a/../", 0, "file://a/../"},
        {"file://a/../../b/./../", 0, "file://a/../../b/./../"},
        {"file://a/.././", 0, "file://a/.././"},
        {"file://a/b/..", 0, "file://a/"},
        {"file://a/b/../", 0, "file://a/"},
        {"file://a/b/.././", 0, "file://a/"},
        {"file://a/b/../c", 0, "file://a/c"},
        {"file://a/b/../c/..", 0, "file://a/"},
        {"file://a/b/../c/../..", 0, "file://a/"},
        {"file://a/b/../../../c", 0, "file://a/c"},
        {"file:///.", 0, "file:///."},
        {"file:///..", 0, "file:///.."},
        {"file:///a/.", 0, "file:///a/"},

        {"file:////", 0, "file:///"},
        {"file:////a/./b/../c", 0, "file://a/./b/../c"},
        {"file://///a/./b/../c", 0, "file://a/./b/../c"},
        {"file://////a/./b/../c", 0, "file://a/./b/../c"},
        {"file:////a/b/./../c", 0, "file://a/c"},
        {"file:////a/b/./../..", 0, "file://a/"},
        {"file://///a/b/./../c", 0, "file://a/c"},
        {"file://////a/b/./../c", 0, "file://a/c"},
        {"file:////.", 0, "file://./"},
        {"file:////..", 0, "file://../"},
        {"file:////./b/./../c", 0, "file://./c"},
        {"file:////./b/./../..", 0, "file://./"},
        {"file://///./b/./../c", 0, "file://./c"},
        {"file://////./b/./../c", 0, "file://./c"},

        /* Drive-like paths get an extra slash (i.e. an empty hostname, to
         * signal that the host is the local machine). The drive letter is
         * treated as the path root. */
        {"file://a:", 0, "file:///a:"},
        {"file://a:/b", 0, "file:///a:/b"},
        {"file://a:/b/../..", 0, "file:///a:/"},
        {"file://a:/./../..", 0, "file:///a:/./../.."},
        {"file://a|/b", 0, "file:///a|/b"},
        {"file://ab:/c", 0, "file://ab:/c"},
        {"file:///a:", 0, "file:///a:"},
        {"file:////a:", 0, "file:///a:"},
        {"file://///a:", 0, "file:///a:"},
        {"file://host/a:/b/../..", 0, "file://host/a:/"},

        /* URL_FILE_USE_PATHURL (and URL_WININET_COMPATIBILITY) have their own
         * set of rules:
         *
         * (1) Dot processing works exactly like the "unknown scheme" rules,
         *     instead of the file/http rules demonstrated above.
         *
         * (2) Some number of backslashes is appended after the two forward
         *     slashes. The number basically corresponds to the detected path
         *     type (two for a remote path, one for a local path, none for a
         *     local drive path). A local path is one where the hostname is
         *     empty or "localhost". If all path elements are empty then no
         *     backslashes are appended.
         */

        {"file://", URL_FILE_USE_PATHURL, "file://"},
        {"file://a", URL_FILE_USE_PATHURL, "file://\\\\a"},
        {"file://a/", URL_FILE_USE_PATHURL, "file://\\\\a"},
        {"file://a//", URL_FILE_USE_PATHURL, "file://\\\\a\\\\"},
        {"file://a/b", URL_FILE_USE_PATHURL, "file://\\\\a\\b"},
        {"file://a//b", URL_FILE_USE_PATHURL, "file://\\\\a\\\\b"},
        {"file://a/.", URL_FILE_USE_PATHURL, "file://\\\\a\\"},
        {"file://a/../../b/./c/..", URL_FILE_USE_PATHURL, "file://\\\\a\\..\\..\\b\\"},
        {"file://./../../b/./c/..", URL_FILE_USE_PATHURL, "file://\\\\.\\..\\..\\b\\"},
        {"file://../../../b/./c/..", URL_FILE_USE_PATHURL, "file://\\\\..\\..\\..\\b\\"},
        {"file://a/b/.", URL_FILE_USE_PATHURL, "file://\\\\a\\b\\"},
        {"file://a/b/.", URL_FILE_USE_PATHURL | URL_DONT_SIMPLIFY, "file://\\\\a\\b\\.\\"},
        {"file://a/b/../../../c", URL_FILE_USE_PATHURL, "file://\\\\a\\..\\..\\c"},

        {"file:///", URL_FILE_USE_PATHURL, "file://"},
        {"file:///.", URL_FILE_USE_PATHURL, "file://\\"},
        {"file:///..", URL_FILE_USE_PATHURL, "file://\\..\\"},
        {"file:///../../b/./c/..", URL_FILE_USE_PATHURL, "file://\\..\\..\\b\\"},
        {"file:///a/b/./c/..", URL_FILE_USE_PATHURL, "file://\\a\\b\\"},

        {"file:////", URL_FILE_USE_PATHURL, "file://"},
        {"file:////.", URL_FILE_USE_PATHURL, "file://\\\\."},
        {"file:////a/./b/../c", URL_FILE_USE_PATHURL, "file://\\\\a\\c"},
        {"file://///", URL_FILE_USE_PATHURL, "file://"},
        {"file://///a/./b/../c", URL_FILE_USE_PATHURL, "file://\\\\a\\c"},

        {"file://a:", URL_FILE_USE_PATHURL, "file://a:"},
        {"file://a:/", URL_FILE_USE_PATHURL, "file://a:\\"},
        {"file://a:/b", URL_FILE_USE_PATHURL, "file://a:\\b"},
        {"file://a:/b/../..", URL_FILE_USE_PATHURL, "file://a:\\..\\"},
        {"file://a|/b", URL_FILE_USE_PATHURL, "file://a|\\b"},
        {"file:///a:", URL_FILE_USE_PATHURL, "file://a:"},
        {"file:////a:", URL_FILE_USE_PATHURL, "file://a:"},

        /* URL_WININET_COMPATIBILITY is almost identical, but ensures a trailing
         * backslash in two cases:
         *
         * (1) if all path elements are empty,
         *
         * (2) if the path consists of just the hostname.
         */

        {"file://", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file://a", URL_WININET_COMPATIBILITY, "file://\\\\a\\"},
        {"file://a/", URL_WININET_COMPATIBILITY, "file://\\\\a\\"},
        {"file://a//", URL_WININET_COMPATIBILITY, "file://\\\\a\\\\"},
        {"file://a/b", URL_WININET_COMPATIBILITY, "file://\\\\a\\b"},
        {"file://a//b", URL_WININET_COMPATIBILITY, "file://\\\\a\\\\b"},
        {"file://a/.", URL_WININET_COMPATIBILITY, "file://\\\\a\\"},
        {"file://a/../../b/./c/..", URL_WININET_COMPATIBILITY, "file://\\\\a\\..\\..\\b\\"},
        {"file://./../../b/./c/..", URL_WININET_COMPATIBILITY, "file://\\\\.\\..\\..\\b\\"},
        {"file://../../../b/./c/..", URL_WININET_COMPATIBILITY, "file://\\\\..\\..\\..\\b\\"},
        {"file://a/b/../../../c", URL_WININET_COMPATIBILITY, "file://\\\\a\\..\\..\\c"},

        {"file:///", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file:///.", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file:///..", URL_WININET_COMPATIBILITY, "file://\\..\\"},
        {"file:///../../b/./c/..", URL_WININET_COMPATIBILITY, "file://\\..\\..\\b\\"},
        {"file:///a/b/./c/..", URL_WININET_COMPATIBILITY, "file://\\a\\b\\"},

        {"file:////", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file:////.", URL_WININET_COMPATIBILITY, "file://\\\\.\\"},
        {"file:////a/./b/../c", URL_WININET_COMPATIBILITY, "file://\\\\a\\c"},
        {"file://///", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file://///a/./b/../c", URL_WININET_COMPATIBILITY, "file://\\\\a\\c"},

        {"file://a:", URL_WININET_COMPATIBILITY, "file://a:"},

        {"file://", URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY, "file://"},

        {"file://localhost/a", 0, "file://localhost/a"},
        {"file://localhost//a", 0, "file://localhost//a"},
        {"file://localhost/a:", 0, "file://localhost/a:"},
        {"file://localhost/a:/b/../..", 0, "file://localhost/a:/"},
        {"file://localhost/a:/./../..", 0, "file://localhost/a:/./../.."},
        {"file://localhost", URL_FILE_USE_PATHURL, "file://"},
        {"file://localhost/", URL_FILE_USE_PATHURL, "file://"},
        {"file://localhost/b", URL_FILE_USE_PATHURL, "file://\\b"},
        {"file://127.0.0.1/b", URL_FILE_USE_PATHURL, "file://\\\\127.0.0.1\\b"},
        {"file://localhost//b", URL_FILE_USE_PATHURL, "file://\\b"},
        {"file://localhost///b", URL_FILE_USE_PATHURL, "file://\\\\b"},
        {"file:///localhost/b", URL_FILE_USE_PATHURL, "file://\\localhost\\b"},
        {"file:////localhost/b", URL_FILE_USE_PATHURL, "file://\\b"},
        {"file://///localhost/b", URL_FILE_USE_PATHURL, "file://\\b"},
        {"file://localhost/a:", URL_FILE_USE_PATHURL, "file://a:"},
        {"file://localhost/a:/b/../..", URL_FILE_USE_PATHURL, "file://a:\\..\\"},
        {"file://localhost?a/b", URL_FILE_USE_PATHURL, "file://"},
        {"file://localhost#a/b", URL_FILE_USE_PATHURL, "file://\\\\localhost#a/b"},
        {"file://localhostq", URL_FILE_USE_PATHURL, "file://\\\\localhostq"},

        {"file://localhost", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file://localhost/", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file://localhost/b", URL_WININET_COMPATIBILITY, "file://\\b"},
        {"file://localhost//b", URL_WININET_COMPATIBILITY, "file://\\b"},
        {"file://127.0.0.1/b", URL_WININET_COMPATIBILITY, "file://\\\\127.0.0.1\\b"},
        {"file://localhost/a:", URL_WININET_COMPATIBILITY, "file://a:"},
        {"file://localhost?a/b", URL_WININET_COMPATIBILITY, "file://\\?a/b"},
        {"file://localhost#a/b", URL_WININET_COMPATIBILITY, "file://\\\\localhost#a/b"},

        /* # has some weird behaviour:
         *
         * - Dot processing happens normally after it, including rewinding past
         *   the #. It's not treated as a path separator for the purposes of
         *   rewinding.
         *
         * - However, if neither file flag is used, and the first character
         *   after the hostname (plus an optional slash) is a hash, no dot
         *   processing takes place.
         *
         * - If the previous path segment ends in .htm or .html, the rest of
         *   the URL is emitted verbatim (no dot or slash canonicalization).
         *   This does not apply to the hostname. If URL_FILE_USE_PATHURL is
         *   used, though, the rest of the URL including the # is omitted.
         *
         * - It is treated as a path terminator for dots, but only if neither
         *   file flag is used. It does not begin a path element.
         *
         * - If there is a # anywhere in the output string (and the string
         *   doesn't fall under the .html exception), all subsequent slashes
         *   are converted to forward slashes instead of backslashes.
         *   This means that rewinding past the hash will revert to backslashes.
         *   (This of course only affects the case where file flags are used;
         *   if no file flags are used then slashes are converted to forward
         *   slashes anyway.)
         */
        {"file://a/b#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b#c/./d\\e", 0, "file://a/b#c/d/e"},
        {"file://a/b.htm#c/../d\\e", 0, "file://a/b.htm#c/../d\\e"},
        {"file://a/b.html#c/../d\\e", 0, "file://a/b.html#c/../d\\e"},
        {"file://a/b.hTmL#c/../d\\e", 0, "file://a/b.hTmL#c/../d\\e"},
        {"file://a/b.xhtml#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.php#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.asp#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.aspx#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.ht#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.txt#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.htmlq#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.html/q#c/../d\\e", 0, "file://a/b.html/d/e"},
        {"file://a/.html#c/../d\\e", 0, "file://a/.html#c/../d\\e"},
        {"file://a/html#c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b#c/./d.html#e/../f", 0, "file://a/b#c/d.html#e/../f"},
        {"file://a.html#/b/../c", 0, "file://a.html#/c"},
        {"file://a/b#c/../d/e", URL_FILE_USE_PATHURL, "file://\\\\a\\d\\e"},
        {"file://a/b#c/./d/e", URL_FILE_USE_PATHURL, "file://\\\\a\\b#c/d/e"},
        {"file://a/b.html#c/../d\\e", URL_FILE_USE_PATHURL, "file://\\\\a\\b.html"},
        {"file://a/b.html#c/../d\\e", URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY, "file://\\\\a\\b.html"},
        {"file://a/b#c/../d/e", URL_WININET_COMPATIBILITY, "file://\\\\a\\d\\e"},
        {"file://a/b#c/./d/e", URL_WININET_COMPATIBILITY, "file://\\\\a\\b#c/d/e"},
        {"file://a/b.html#c/../d\\e", URL_WININET_COMPATIBILITY, "file://\\\\a\\b.html#c/../d\\e"},
        {"file://a/c#/../d", 0, "file://a/d"},
        {"file://a/c#/../d", URL_FILE_USE_PATHURL, "file://\\\\a\\d"},
        {"file://a/c#/../d", URL_WININET_COMPATIBILITY, "file://\\\\a\\d"},
        {"file://a/#c/../d\\e", 0, "file://a/#c/../d\\e"},
        {"file://a/#c/../d/e", URL_FILE_USE_PATHURL, "file://\\\\a\\d\\e"},
        {"file://a/#c/../d/e", URL_WININET_COMPATIBILITY, "file://\\\\a\\d\\e"},
        {"file://a//#c/../d", 0, "file://a//#c/../d"},
        {"file://a//#c/../d", URL_FILE_USE_PATHURL, "file://\\\\a\\\\d"},
        {"file://a//#c/../d", URL_WININET_COMPATIBILITY, "file://\\\\a\\\\d"},
        {"file://a/\\#c/../d", 0, "file://a//#c/../d"},
        {"file://a///#c/../d", 0, "file://a///d"},
        {"file://a/b/#c/../d", 0, "file://a/b/d"},
        {"file://a/b/.#c", 0, "file://a/b/#c"},
        {"file://a/b/..#c", 0, "file://a/#c"},
        {"file://a/b/.#c", URL_FILE_USE_PATHURL, "file://\\\\a\\b\\.#c"},
        {"file://a/b/..#c", URL_FILE_USE_PATHURL, "file://\\\\a\\b\\..#c"},
        {"file://a/b/.#c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b\\.#c"},
        {"file://a/b/..#c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b\\..#c"},
        {"file://a/b#../c", 0, "file://a/b#../c"},
        {"file://a/b/#../c", 0, "file://a/b/#../c"},
        {"file://a/b#../c", URL_FILE_USE_PATHURL, "file://\\\\a\\b#../c"},
        {"file://a/b#../c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b#../c"},
        {"file://#/b\\./", 0, "file://#/b/"},
        {"file://#/./b\\./", 0, "file://#/./b/./"},
        {"file://#/b\\./", URL_FILE_USE_PATHURL, "file://\\\\#/b/"},
        {"file://#/b\\./", URL_WININET_COMPATIBILITY, "file://\\\\#/b/"},
        {"file://a#/b\\./", 0, "file://a#/b/"},
        {"file://a#/./b\\./", 0, "file://a#/./b/./"},
        {"file://a#/b\\./", URL_FILE_USE_PATHURL, "file://\\\\a#/b/"},
        {"file://a#/b\\./", URL_WININET_COMPATIBILITY, "file://\\\\a#/b/"},
        {"file://a#/b\\./", URL_FILE_USE_PATHURL | URL_DONT_SIMPLIFY, "file://\\\\a#/b/./"},
        {"file://a#/b\\.", URL_FILE_USE_PATHURL | URL_DONT_SIMPLIFY, "file://\\\\a#/b/./"},
        {"file://a#/b/../../", 0, "file://a#/"},

        /* ? is similar, with the following exceptions:
         *
         * - URLs ending in .htm(l) are not treated specially.
         *
         * - With URL_FILE_USE_PATHURL, the rest of the URL including the ? is
         *   just omitted (much like the .html case above).
         *
         * - With URL_WININET_COMPATIBILITY, the rest of the URL is always
         *   emitted verbatim (completely opaque, like other schemes).
         */

        {"file://a/b?c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b.html?c/../d\\e", 0, "file://a/d/e"},
        {"file://a/b?c/../d\\e", URL_FILE_USE_PATHURL, "file://\\\\a\\b"},
        {"file://a/b.html?c/../d\\e", URL_FILE_USE_PATHURL, "file://\\\\a\\b.html"},
        {"file://a/b?c/../d\\e", URL_WININET_COMPATIBILITY, "file://\\\\a\\b?c/../d\\e"},
        {"file://a/b.html?c/../d\\e", URL_WININET_COMPATIBILITY, "file://\\\\a\\b.html?c/../d\\e"},
        {"file://a/b?c/../d", URL_FILE_USE_PATHURL | URL_WININET_COMPATIBILITY, "file://\\\\a\\b"},
        {"file://a/?c/../d", 0, "file://a/?c/../d"},
        {"file://a/?c/../d", URL_FILE_USE_PATHURL, "file://\\\\a"},
        {"file://a/?c/../d", URL_WININET_COMPATIBILITY, "file://\\\\a\\?c/../d"},
        {"file://a//?c/../d", 0, "file://a//?c/../d"},
        {"file://a//?c/../d", URL_FILE_USE_PATHURL, "file://\\\\a\\\\"},
        {"file://a//?c/../d", URL_WININET_COMPATIBILITY, "file://\\\\a\\\\?c/../d"},
        {"file://a/\\?c/../d", 0, "file://a//?c/../d"},
        {"file://a///?c/../d", 0, "file://a///d"},
        {"file://a/b/?c/../d", 0, "file://a/b/d"},
        {"file://a/b/.?c", 0, "file://a/b/?c"},
        {"file://a/b/..?c", 0, "file://a/?c"},
        {"file://a/b/.?c", URL_FILE_USE_PATHURL, "file://\\\\a\\b\\"},
        {"file://a/b/..?c", URL_FILE_USE_PATHURL, "file://\\\\a\\"},
        {"file://a/b/.?c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b\\?c"},
        {"file://a/b/..?c", URL_WININET_COMPATIBILITY, "file://\\\\a\\?c"},
        {"file://?/a\\./", 0, "file://?/a/"},
        {"file://?/./a\\./", 0, "file://?/./a/./"},
        {"file://?/a\\./", URL_FILE_USE_PATHURL, "file://"},
        {"file://?/a\\./", URL_WININET_COMPATIBILITY, "file://\\?/a\\./"},
        {"file://a?/a\\./", 0, "file://a?/a/"},
        {"file://a?/./a\\./", 0, "file://a?/./a/./"},
        {"file://a?/a\\./", URL_FILE_USE_PATHURL, "file://\\\\a"},
        {"file://a?/a\\./", URL_WININET_COMPATIBILITY, "file://\\\\a\\?/a\\./"},

        {"file://a/b.html?c#d/..", 0, "file://a/"},
        {"file://a/b.html?c.html#d/..", 0, "file://a/b.html?c.html#d/.."},
        {"file://a/b?\\#c\\d", 0, "file://a/b?/#c/d"},
        {"file://a/b?\\#c\\d", URL_WININET_COMPATIBILITY, "file://\\\\a\\b?\\#c\\d"},
        {"file://a/b?\\#c\\d", URL_FILE_USE_PATHURL, "file://\\\\a\\b"},
        {"file://a/b#\\?c\\d", 0, "file://a/b#/?c/d"},
        {"file://a/b#\\?c\\d", URL_WININET_COMPATIBILITY, "file://\\\\a\\b#/?c\\d"},
        {"file://a/b#\\?c\\d", URL_FILE_USE_PATHURL, "file://\\\\a\\b#/"},
        {"file://a/b.html#c?d", URL_WININET_COMPATIBILITY, "file://\\\\a\\b.html?d#c"},

        /* file: treats backslashes like forward slashes, including the
         * initial pair. */
        {"file://a/b\\", 0, "file://a/b/"},
        {"file://a/b\\c/../.\\", 0, "file://a/b/"},
        {"file://a\\b", 0, "file://a/b"},
        {"file:/\\a\\..", 0, "file://a/.."},
        {"file:\\/a\\..", 0, "file://a/.."},
        {"file:\\\\a\\b", URL_FILE_USE_PATHURL, "file://\\\\a\\b"},
        {"file:\\\\a\\b", URL_WININET_COMPATIBILITY, "file://\\\\a\\b"},
        {"file:\\///a/./b/../c", 0, "file://a/./b/../c"},
        {"file:/\\//a/./b/../c", 0, "file://a/./b/../c"},
        {"file://\\/a/./b/../c", 0, "file://a/./b/../c"},
        {"file:///\\a/./b/../c", 0, "file://a/./b/../c"},

        {"file://a/b &c", 0, "file://a/b &c"},
        {"file://a/b &c", URL_FILE_USE_PATHURL, "file://\\\\a\\b &c"},
        {"file://a/b &c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b &c"},
        {"file://a/b !\"$%&'()*+,-:;<=>@[]^_`{|}~c", URL_ESCAPE_UNSAFE, "file://a/b%20!%22$%%26'()*+,-:;%3C=%3E@%5B%5D%5E_%60%7B%7C%7D~c"},
        {"file://a/b%20%26c", 0, "file://a/b%20%26c"},
        {"file://a/b%20%26c", URL_FILE_USE_PATHURL, "file://\\\\a\\b &c"},
        {"file://a/b%20%26c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b%20%26c"},

        /* Omitting one slash behaves as if the URL had been written with an
         * empty hostname, and the output adds two slashes as such. */

        {"file:/", 0, "file:///"},
        {"file:/a", 0, "file:///a"},
        {"file:/./a", 0, "file:///./a"},
        {"file:/../a/..", 0, "file:///../a/.."},
        {"file:/./..", 0, "file:///./.."},
        {"file:/a/.", 0, "file:///a/"},
        {"file:/a/../..", 0, "file:///"},
        {"file:/a:", 0, "file:///a:"},
        {"file:/a:/b/../..", 0, "file:///a:/"},

        /* The same applies to the flags. */

        {"file:/", URL_FILE_USE_PATHURL, "file://"},
        {"file:/a", URL_FILE_USE_PATHURL, "file://\\a"},
        {"file:/.", URL_FILE_USE_PATHURL, "file://\\"},
        {"file:/./a", URL_FILE_USE_PATHURL, "file://\\a"},
        {"file:/../a", URL_FILE_USE_PATHURL, "file://\\..\\a"},
        {"file:/a/../..", URL_FILE_USE_PATHURL, "file://\\..\\"},
        {"file:/a/.", URL_FILE_USE_PATHURL | URL_DONT_SIMPLIFY, "file://\\a\\.\\"},
        {"file:/a:", URL_FILE_USE_PATHURL, "file://a:"},
        {"file:/a:/b/../..", URL_FILE_USE_PATHURL, "file://a:\\..\\"},

        {"file:/", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file:/a", URL_WININET_COMPATIBILITY, "file://\\a"},
        {"file:/.", URL_WININET_COMPATIBILITY, "file://\\"},
        {"file:/a:", URL_WININET_COMPATIBILITY, "file://a:"},

        {"file:/a/b#c/../d", 0, "file:///a/d"},
        {"file:/a/b?c/../d", 0, "file:///a/d"},

        {"file:/a\\b\\", 0, "file:///a/b/"},
        {"file:\\a/b/", 0, "file:///a/b/"},
        {"file:\\a\\b", URL_FILE_USE_PATHURL, "file://\\a\\b"},
        {"file:\\a\\b", URL_WININET_COMPATIBILITY, "file://\\a\\b"},

        {"file:/a/b &c", 0, "file:///a/b &c"},

        /* Omitting both slashes causes all dots to be collapsed, in the same
         * way as bare http. */

        {"file:a", 0, "file:a"},
        {"file:a/", 0, "file:a/"},
        {"file:a/.", 0, "file:a/"},
        {"file:a/..", 0, "file:"},
        {"file:a/../..", 0, "file:"},
        {"file:", 0, "file:"},
        {"file:.", 0, "file:"},
        {"file:..", 0, "file:"},
        {"file:./", 0, "file:"},
        {"file:../", 0, "file:"},

        {"file:a:", 0, "file:///a:"},

        /* URL_FILE_USE_PATHURL treats everything here as a local (relative?)
         * path. In the case that the path resolves to the current directory
         * a single backslash is emitted. */
        {"file:", URL_FILE_USE_PATHURL, "file://"},
        {"file:a", URL_FILE_USE_PATHURL, "file://a"},
        {"file:a/.", URL_FILE_USE_PATHURL, "file://a\\"},
        {"file:a/../..", URL_FILE_USE_PATHURL, "file://..\\"},
        {"file:./a", URL_FILE_USE_PATHURL, "file://a"},
        {"file:../a", URL_FILE_USE_PATHURL, "file://..\\a"},
        {"file:a/.", URL_FILE_USE_PATHURL | URL_DONT_SIMPLIFY, "file://a\\.\\"},
        {"file:a:", URL_FILE_USE_PATHURL, "file://a:"},

        /* URL_WININET_COMPATIBILITY doesn't emit a double slash. */
        {"file:", URL_WININET_COMPATIBILITY, "file:"},
        {"file:a", URL_WININET_COMPATIBILITY, "file:a"},
        {"file:./a", URL_WININET_COMPATIBILITY, "file:a"},
        {"file:../a", URL_WININET_COMPATIBILITY, "file:..\\a"},
        {"file:../b/./c/../d", URL_WININET_COMPATIBILITY | URL_DONT_SIMPLIFY, "file:..\\b\\.\\c\\..\\d"},
        {"file:a:", URL_WININET_COMPATIBILITY, "file://a:"},

        {"file:a/b?c/../d", 0, "file:a/d"},
        {"file:a/b#c/../d", 0, "file:a/d"},

        {"file:a\\b\\", 0, "file:a/b/"},

        {"file:a/b &c", 0, "file:a/b &c"},

        {"fIlE://A/B", 0, "file://A/B"},
        {"fIlE://A/B", URL_FILE_USE_PATHURL, "file://\\\\A\\B"},
        {"fIlE://A/B", URL_WININET_COMPATIBILITY, "file://\\\\A\\B"},
        {"fIlE:A:/B", 0, "file:///A:/B"},
        {"fIlE:A:/B", URL_FILE_USE_PATHURL, "file://A:\\B"},
        {"fIlE:A:/B", URL_WININET_COMPATIBILITY, "file://A:\\B"},
        {"fIlE://lOcAlHoSt/B", 0, "file://lOcAlHoSt/B"},
        {"fIlE://lOcAlHoSt/B", URL_FILE_USE_PATHURL, "file://\\B"},

        /* Drive paths are automatically converted to file paths. Dots are
         * collapsed unless the first segment after q: or q:/ is a dot. */

        {"q:a", 0, "file:///q:a"},
        {"q:a/.", 0, "file:///q:a/"},
        {"q:a/..", 0, "file:///q:"},
        {"q:a/../..", 0, "file:///q:"},
        {"q:./a/..", 0, "file:///q:./a/.."},
        {"q:../a/..", 0, "file:///q:../a/.."},
        {"q:/", 0, "file:///q:/"},
        {"q:/a", 0, "file:///q:/a"},
        {"q:/a/.", 0, "file:///q:/a/"},
        {"q:/a/..", 0, "file:///q:/"},
        {"q:/./a/..", 0, "file:///q:/./a/.."},
        {"q:/../a/..", 0, "file:///q:/../a/.."},
        {"q://./a", 0, "file:///q://a"},
        {"q://../a", 0, "file:///q:/a"},

        /* File flags use the "unknown scheme" rules, and the root of the path
         * is the first slash. */

        {"q:/a", URL_FILE_USE_PATHURL, "file://q:\\a"},
        {"q:/a/../..", URL_FILE_USE_PATHURL, "file://q:\\..\\"},
        {"q:a/../../b/..", URL_FILE_USE_PATHURL, "file://q:a\\..\\..\\"},
        {"q:./../../b/..", URL_FILE_USE_PATHURL, "file://q:.\\..\\..\\"},
        {"q:/a", URL_WININET_COMPATIBILITY, "file://q:\\a"},
        {"q:/a/../..", URL_WININET_COMPATIBILITY, "file://q:\\..\\"},
        {"q:a/../../b/..", URL_WININET_COMPATIBILITY, "file://q:a\\..\\..\\"},
        {"q:./../../b/..", URL_WININET_COMPATIBILITY, "file://q:.\\..\\..\\"},

        {"q:/a/b?c/../d", 0, "file:///q:/a/d"},
        {"q:/a/b#c/../d", 0, "file:///q:/a/d"},
        {"q:a?b", URL_FILE_USE_PATHURL, "file://q:a"},

        {"q:a\\b\\", 0, "file:///q:a/b/"},
        {"q:\\a/b", 0, "file:///q:/a/b"},

        /* Drive paths are also unique in that unsafe characters (and spaces)
         * are automatically escaped—but not if the file flags are used. */

        {"q:/a/b !\"$%&'()*+,-:;<=>@[]^_`{|}~c", 0, "file:///q:/a/b%20!%22$%25%26'()*+,-:;%3C=%3E@%5B%5D%5E_%60%7B%7C%7D~c"},
        {"q:/a/b &c", URL_FILE_USE_PATHURL, "file://q:\\a\\b &c"},
        {"q:/a/b &c", URL_WININET_COMPATIBILITY, "file://q:\\a\\b &c"},

        {"q:/a/b%20%26c", 0, "file:///q:/a/b%2520%2526c"},
        {"q:/a/b%20%26c", URL_UNESCAPE, "file:///q:/a/b &c"},
        {"q:/a/b%20%26c", URL_UNESCAPE | URL_ESCAPE_UNSAFE, "file:///q:/a/b%20%26c"},
        {"q:/a/b%20%26c", URL_FILE_USE_PATHURL, "file://q:\\a\\b &c"},
        {"q:/a/b%20%26c", URL_FILE_USE_PATHURL | URL_UNESCAPE, "file://q:\\a\\b &c"},
        {"q:/a/b%20%26c", URL_WININET_COMPATIBILITY, "file://q:\\a\\b%20%26c"},
        {"q:/a/b%20%26c", URL_WININET_COMPATIBILITY | URL_UNESCAPE, "file://q:\\a\\b &c"},

        {"q|a", 0, "file:///q%7Ca"},
        {"-:a", 0, "-:a"},
        {"Q:A", 0, "file:///Q:A"},

        /* A double initial backslash is also converted to a file path. The same
         * rules for hostnames apply. */

        {"\\\\", 0, "file:///"},
        {"\\\\a", 0, "file://a/"},
        {"\\\\../a\\b/..\\c/.\\", 0, "file://../a/c/"},
        {"\\\\a/./b/../c", 0, "file://a/./b/../c"},
        /* And, of course, four or more slashes gets collapsed... */
        {"\\\\//./b/./../c", 0, "file://./c"},
        {"\\\\///./b/./../c", 0, "file://./c"},

        {"\\\\a/b?c/../d", 0, "file://a/d"},
        {"\\\\a/b#c/../d", 0, "file://a/d"},

        /* Drive paths are "recognized" too, though. The following isn't
         * actually a local path, but UrlCanonicalize() doesn't seem to realize
         * that. */
        {"\\\\a:/b", 0, "file:///a:/b"},

        {"\\\\", URL_FILE_USE_PATHURL, "file://"},
        {"\\\\a", URL_FILE_USE_PATHURL, "file://\\\\a"},
        {"\\\\a/./..", URL_FILE_USE_PATHURL, "file://\\\\a\\..\\"},
        {"\\\\a:/b", URL_FILE_USE_PATHURL, "file://a:\\b"},
        {"\\\\", URL_WININET_COMPATIBILITY, "file://\\"},
        {"\\\\a", URL_WININET_COMPATIBILITY, "file://\\\\a\\"},
        {"\\\\a/./..", URL_WININET_COMPATIBILITY, "file://\\\\a\\..\\"},
        {"\\\\a:/b", URL_WININET_COMPATIBILITY, "file://a:\\b"},

        /* And, as with drive paths, unsafe characters are escaped. */
        {"\\\\a/b !\"$%&'()*+,-:;<=>@[]^_`{|}~c", 0, "file://a/b%20!%22$%25%26'()*+,-:;%3C=%3E@%5B%5D%5E_%60%7B%7C%7D~c"},
        {"\\\\a/b &c", URL_FILE_USE_PATHURL, "file://\\\\a\\b &c"},
        {"\\\\a/b &c", URL_WININET_COMPATIBILITY, "file://\\\\a\\b &c"},

        {"\\\\/b", 0, "file:///b"},
        {"\\\\/b", URL_FILE_USE_PATHURL, "file://\\b"},
        {"\\\\localhost/b", URL_FILE_USE_PATHURL, "file://\\b"},
        {"\\\\127.0.0.1/b", URL_FILE_USE_PATHURL, "file://\\\\127.0.0.1\\b"},
        {"\\\\localhost/b", URL_WININET_COMPATIBILITY, "file://\\b"},
        {"\\\\127.0.0.1/b", URL_WININET_COMPATIBILITY, "file://\\\\127.0.0.1\\b"},

        {"\\\\A/B", 0, "file://A/B"},

        {"file:///c:/tests/foo%20bar", URL_UNESCAPE, "file:///c:/tests/foo bar"},
        {"file:///c:/tests\\foo%20bar", URL_UNESCAPE, "file:///c:/tests/foo bar"},
        {"file:///c:/tests/foo%20bar", 0, "file:///c:/tests/foo%20bar"},
        {"file:///c:/tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://localhost/c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://localhost\\c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://localhost\\\\c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://localhost\\c:\\tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://c:/tests/../tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://c:/tests\\../tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file://c:/tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\tests\\foo bar"},
        {"file:///c://tests/foo%20bar", URL_FILE_USE_PATHURL, "file://c:\\\\tests\\foo bar"},
        {"file:///c:\\tests\\foo bar", 0, "file:///c:/tests/foo bar"},
        {"file:///c:\\tests\\foo bar", URL_DONT_SIMPLIFY, "file:///c:/tests/foo bar"},
        {"file:///c:\\tests\\foobar", 0, "file:///c:/tests/foobar"},
        {"file:///c:\\tests\\foobar", URL_WININET_COMPATIBILITY, "file://c:\\tests\\foobar"},
        {"file://home/user/file", 0, "file://home/user/file"},
        {"file:///home/user/file", 0, "file:///home/user/file"},
        {"file:////home/user/file", 0, "file://home/user/file"},
        {"file://home/user/file", URL_WININET_COMPATIBILITY, "file://\\\\home\\user\\file"},
        {"file:///home/user/file", URL_WININET_COMPATIBILITY, "file://\\home\\user\\file"},
        {"file:////home/user/file", URL_WININET_COMPATIBILITY, "file://\\\\home\\user\\file"},
        {"file://///home/user/file", URL_WININET_COMPATIBILITY, "file://\\\\home\\user\\file"},
        {"file://C:/user/file", 0, "file:///C:/user/file"},
        {"file://C:/user/file/../asdf", 0, "file:///C:/user/asdf"},
        {"file:///C:/user/file", 0, "file:///C:/user/file"},
        {"file:////C:/user/file", 0, "file:///C:/user/file"},
        {"file://C:/user/file", URL_WININET_COMPATIBILITY, "file://C:\\user\\file"},
        {"file:///C:/user/file", URL_WININET_COMPATIBILITY, "file://C:\\user\\file"},
        {"file:////C:/user/file", URL_WININET_COMPATIBILITY, "file://C:\\user\\file"},
    };

    static const struct canonicalize_test misc_tests[] =
    {
        {"", 0, ""},

        /* If both slashes are omitted, everything afterwards is replicated
         * as-is, with the exception that the final period is dropped from
         * "scheme:." */

        {"wine:.", 0, "wine:"},
        {"wine:.", URL_DONT_SIMPLIFY, "wine:."},
        {"wine:./", 0, "wine:./"},
        {"wine:..", 0, "wine:.."},
        {"wine:../", 0, "wine:../"},
        {"wine:a", 0, "wine:a"},
        {"wine:a/", 0, "wine:a/"},
        {"wine:a/b/./../c", 0, "wine:a/b/./../c"},

        {"wine:a/b?c/./d", 0, "wine:a/b?c/./d"},
        {"wine:a/b#c/./d", 0, "wine:a/b#c/./d"},
        {"wine:a/b#c?d", 0, "wine:a/b?d#c"},
        {"wine:.#c?d", 0, "wine:?d#c"},

        /* A backslash directly after the colon is not treated specially. */
        {"wine:\\././a", 0, "wine:\\././a"},

        {"wine:a/b &c", 0, "wine:a/b &c"},

        /* If there's no scheme or hostname, things mostly follow the "unknown
         * scheme" rules, except that a would-be empty string results in a
         * single slash instead. */

        {"a", 0, "a"},
        {"a/", 0, "a/"},
        {".", 0, "/"},
        {".", URL_DONT_SIMPLIFY, "./"},
        {"./", 0, "/"},
        {"./.", 0, "/"},
        {"././a", 0, "a"},
        {"a/.", 0, "a/"},
        {"a/./", 0, "a/"},
        {"a/./b", 0, "a/b"},

        {"..", 0, "../"},
        {"../", 0, "../"},
        {"../a", 0, "../a"},
        {"../a/..", 0, "../"},
        {"a/..", 0, "/"},
        {"a/../..", 0, "../"},
        {"a/b/..", 0, "a/"},
        {"a/b/../", 0, "a/"},
        {"a/b/../c", 0, "a/c"},
        {"a/b/../c/..", 0, "a/"},
        {"a/b/../c/../..", 0, "/"},

        {"a/b?c/./d", 0, "a/b?c/./d"},
        {"a/b#c/./d", 0, "a/b#c/./d"},
        {"a/b#c?d", 0, "a/b?d#c"},
        {"?c", 0, "?c"},
        {".?c", 0, "/?c"},

        {"?c/./d", 0, "?c/./d"},
        {"#c/./d", 0, "#c/./d"},

        {"a\\b/..", 0, "/"},

        {"a/b &c", 0, "a/b &c"},

        /* A colon by itself is not interpreted as any sort of scheme. */
        {"://../../a", 0, "a"},

        /* mk: is another idiosyncratic scheme, although thankfully it behaves
         * rather simply. It has no concept of a hostname; if two slashes follow
         * the scheme it simply treats them as two empty path elements. */
        {"mk:", 0, "mk:"},
        {"mk:.", 0, "mk:"},
        {"mk:..", 0, "mk:"},
        {"mk:/", 0, "mk:/"},
        {"mk:/.", 0, "mk:/"},
        {"mk:/..", 0, "mk:"},
        {"mk:a", 0, "mk:a"},
        {"mk:a:", 0, "mk:a:"},
        {"mk://", 0, "mk://"},
        {"mk://.", 0, "mk://"},
        {"mk://..", 0, "mk:/"},
        {"mk://../..", 0, "mk:"},
        {"mk://../..", URL_DONT_SIMPLIFY, "mk://../.."},
        {"mk://../../..", 0, "mk:"},

        /* Backslashes are not translated into forward slashes. They are treated
         * as path separators, but in a somewhat buggy manner: only dots before
         * a forward slash are collapsed, and a double dot rewinds to the
         * previous forward slash. */
        {"mk:a/.\\", 0, "mk:a/.\\"},
        {"mk:a/.\\b", 0, "mk:a/.\\b"},
        {"mk:a\\.\\b", 0, "mk:a\\.\\b"},
        {"mk:a\\./b", 0, "mk:a\\b"},
        {"mk:a./b", 0, "mk:a./b"},
        {"mk:a\\b/..\\c", 0, "mk:a\\b/..\\c"},
        {"mk:a\\b\\..\\c", 0, "mk:a\\b\\..\\c"},
        {"mk:a/b\\../c", 0, "mk:a/c"},
        {"mk:a\\b../c", 0, "mk:a\\b../c"},

        /* Progids get a forward slash appended if there isn't one already, and
         * dots don't rewind past them. Despite the fact that progids are
         * supposed to end with a colon, UrlCanonicalize() considers them to
         * end with the slash.
         *
         * If the first path segment is a dot or double dot, it's treated as
         * a relative path, like http, but only before a forward slash. */

        {"mk:@", 0, "mk:@/"},
        {"mk:@progid", 0, "mk:@progid/"},
        {"mk:@progid:a", 0, "mk:@progid:a/"},
        {"mk:@progid:a/b", 0, "mk:@progid:a/b"},
        {"mk:@Progid:a/b/../..", 0, "mk:@Progid:a/"},
        {"mk:@progid/a", 0, "mk:@progid/a"},
        {"mk:@progid\\a", 0, "mk:@progid\\a/"},
        {"mk:@progid/a/../..", 0, "mk:@progid/"},
        {"mk:@progid/.", 0, "mk:@progid/."},
        {"mk:@progid/.?", 0, "mk:@progid/.?"},
        {"mk:@progid/./..", 0, "mk:@progid/./.."},
        {"mk:@progid/../..", 0, "mk:@progid/../.."},
        {"mk:@progid/a\\.\\b", 0, "mk:@progid/a\\.\\b"},
        {"mk:@progid/a\\..\\b", 0, "mk:@progid/a\\..\\b"},
        {"mk:@progid/.\\..", 0, "mk:@progid/"},

        {"mk:a/b?c/../d", 0, "mk:a/b?c/../d"},
        {"mk:a/b#c/../d", 0, "mk:a/b#c/../d"},
        {"mk:a/b#c?d", 0, "mk:a/b#c?d"},
        {"mk:@progid/a/b?c/../d", 0, "mk:@progid/a/b?c/../d"},
        {"mk:@progid?c/d/..", 0, "mk:@progid?c/"},

        {"mk:a/b &c", 0, "mk:a/b &c"},

        {"mk:@MSITStore:dir/test.chm::/file.html/..", 0, "mk:@MSITStore:dir/test.chm::/"},
        {"mk:@MSITStore:dir/test.chm::/file.html/../..", 0, "mk:@MSITStore:dir/"},

        /* Whitespace except for plain spaces are stripped before parsing. */
        {" \t\n\rwi\t\n\rne\t\n\r:\t\n\r/\t\n\r/\t\n\r./../a/.\t\n\r./ \t\n\r", 0, "wine://./../"},
        /* Initial and final spaces and C0 control characters are also stripped,
         * but not 007F or C1 control characters. */
        {" \a\t\x01 wine://./.. \x1f\n\v ", 0, "wine://./../"},
        {" wine ://./..", 0, "wine :/"},
        {" wine: //a/../b", 0, "wine: //a/../b"},
        {" wine://a/b c/.. ", 0, "wine://a/"},
        {"\x7f/\a/\v/\x01/\x1f/\x80", 0, "\x7f/\a/\v/\x01/\x1f/\x80"},

        /* Schemes are not case-sensitive, but are flattened to lowercase.
         * The hostname for http-like schemes is also flattened to lowercase
         * (but not for file; see above). */
        {"wInE://A/B", 0, "wine://A/B"},
        {"hTtP://A/b/../../C", 0, "http://a/C"},
        {"fTP://A/B\\./C", 0, "ftp://a/B\\C"},
        {"aBoUT://A/B/./", 0, "about://A/B/./"},
        {"mK://..", 0, "mk:/"},

        /* Characters allowed in a scheme are alphanumeric, hyphen, plus, period. */
        {"0Aa+-.://./..", 0, "0aa+-.://./../"},
        {"a_://./..", 0, "a_:/"},
        {"a,://./..", 0, "a,:/"},

        {"/uri-res/N2R?urn:sha1:B3K", URL_DONT_ESCAPE_EXTRA_INFO | URL_WININET_COMPATIBILITY, "/uri-res/N2R?urn:sha1:B3K"} /* LimeWire online installer calls this */,
        {"mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm", 0,
         "mk:@MSITStore:C:\\Program Files/AutoCAD 2008\\Help/acad_acg.chm::/WSfacf1429558a55de1a7524c1004e616f8b-322b.htm"},
    };

    static const DWORD file_flags[] = {0, URL_FILE_USE_PATHURL, URL_WININET_COMPATIBILITY};

    urllen = lstrlenA(winehqA);

    /* Parameter checks */
    dwSize = ARRAY_SIZE(szReturnUrl);
    hr = UrlCanonicalizeA(NULL, szReturnUrl, &dwSize, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(dwSize == ARRAY_SIZE(szReturnUrl), "got size %lu\n", dwSize);

    dwSize = ARRAY_SIZE(szReturnUrl);
    hr = UrlCanonicalizeA(winehqA, NULL, &dwSize, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(dwSize == ARRAY_SIZE(szReturnUrl), "got size %lu\n", dwSize);

    hr = UrlCanonicalizeA(winehqA, szReturnUrl, NULL, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    dwSize = 0;
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!dwSize, "got size %lu\n", dwSize);

    /* buffer has no space for the result */
    dwSize=urllen-1;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY  | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%lx with %lu and size %lu for '%s' and %u (expected 'E_POINTER' and size %lu)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen+1);

    /* buffer has no space for the terminating '\0' */
    dwSize=urllen;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%lx with %lu and size %lu for '%s' and %u (expected 'E_POINTER' and size %lu)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen+1);

    /* buffer has the required size */
    dwSize=urllen+1;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%lx with %lu and size %lu for '%s' and %u (expected 'S_OK' and size %lu)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen);

    /* buffer is larger as the required size */
    dwSize=urllen+2;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%lx with %lu and size %lu for '%s' and %u (expected 'S_OK' and size %lu)\n",
        hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), urllen);

    /* length is set to 0 */
    dwSize=0;
    memset(szReturnUrl, '#', urllen+4);
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeA(winehqA, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_INVALIDARG) && (dwSize == 0),
            "got 0x%lx with %lu and size %lu for '%s' and %u (expected 'E_INVALIDARG' and size %u)\n",
            hr, GetLastError(), dwSize, szReturnUrl, lstrlenA(szReturnUrl), 0);

    /* url length > INTERNET_MAX_URL_SIZE */
    dwSize=sizeof(szReturnUrl);
    memset(longurl, 'a', sizeof(longurl));
    memcpy(longurl, winehqA, sizeof(winehqA)-1);
    longurl[sizeof(longurl)-1] = '\0';
    hr = UrlCanonicalizeA(longurl, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(dwSize == strlen(szReturnUrl), "got size %lu\n", dwSize);

    for (f = 0; f < ARRAY_SIZE(file_flags); ++f)
    {
        for (i = 0; i < ARRAY_SIZE(unk_scheme_tests); ++i)
        {
            check_url_canonicalize(unk_scheme_tests[i].url,
                    unk_scheme_tests[i].flags | file_flags[f], unk_scheme_tests[i].expect);
            sprintf(url, "wine:%s", unk_scheme_tests[i].url);
            sprintf(expect, "wine:%s", unk_scheme_tests[i].expect);
            check_url_canonicalize(url, unk_scheme_tests[i].flags | file_flags[f], expect);
        }

        for (i = 0; i < ARRAY_SIZE(http_tests); ++i)
        {
            static const struct
            {
                const char *prefix;
                BOOL ftp_like;
            }
            prefixes[] =
            {
                {"ftp", TRUE},
                {"gopher"},
                {"http"},
                {"https"},
                {"local", TRUE},
                {"news"},
                {"nntp"},
                {"res", TRUE},
                {"snews"},
                {"telnet"},
                {"wais", TRUE},
            };

            for (j = 0; j < ARRAY_SIZE(prefixes); ++j)
            {
                sprintf(url, "%s:%s", prefixes[j].prefix, http_tests[i].url);
                if (prefixes[j].ftp_like && http_tests[i].expect_ftp)
                    sprintf(expect, "%s:%s", prefixes[j].prefix, http_tests[i].expect_ftp);
                else
                    sprintf(expect, "%s:%s", prefixes[j].prefix, http_tests[i].expect);

                check_url_canonicalize(url, http_tests[i].flags | file_flags[f], expect);
            }
        }

        for (i = 0; i < ARRAY_SIZE(opaque_tests); ++i)
        {
            static const char *const prefixes[] = {"about", "javascript", "mailto", "shell", "vbscript"};

            for (j = 0; j < ARRAY_SIZE(prefixes); ++j)
            {
                sprintf(url, "%s:%s", prefixes[j], opaque_tests[i].url);
                sprintf(expect, "%s:%s", prefixes[j], opaque_tests[i].expect);
                check_url_canonicalize(url, opaque_tests[i].flags | file_flags[f], expect);
            }
        }

        for (i = 0; i < ARRAY_SIZE(misc_tests); i++)
            check_url_canonicalize(misc_tests[i].url, misc_tests[i].flags | file_flags[f], misc_tests[i].expect);
    }

    for (i = 0; i < ARRAY_SIZE(file_tests); i++)
        check_url_canonicalize(file_tests[i].url, file_tests[i].flags, file_tests[i].expect);
}

/* ########################### */

static void test_UrlCanonicalizeW(void)
{
    WCHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwSize;
    DWORD urllen;
    HRESULT hr;
    int i;

    urllen = lstrlenW(winehqW);

    /* Parameter checks */
    dwSize = ARRAY_SIZE(szReturnUrl);
    hr = UrlCanonicalizeW(NULL, szReturnUrl, &dwSize, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(dwSize == ARRAY_SIZE(szReturnUrl), "got size %lu\n", dwSize);

    dwSize = ARRAY_SIZE(szReturnUrl);
    hr = UrlCanonicalizeW(winehqW, NULL, &dwSize, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(dwSize == ARRAY_SIZE(szReturnUrl), "got size %lu\n", dwSize);

    hr = UrlCanonicalizeW(winehqW, szReturnUrl, NULL, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    dwSize = 0;
    hr = UrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_UNESCAPE);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!dwSize, "got size %lu\n", dwSize);

    /* buffer has no space for the result */
    dwSize = (urllen-1);
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%lx with %lu and size %lu for %u (expected 'E_POINTER' and size %lu)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen+1);


    /* buffer has no space for the terminating '\0' */
    dwSize = urllen;
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == E_POINTER) && (dwSize == (urllen + 1)),
        "got 0x%lx with %lu and size %lu for %u (expected 'E_POINTER' and size %lu)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen+1);

    /* buffer has the required size */
    dwSize = urllen +1;
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%lx with %lu and size %lu for %u (expected 'S_OK' and size %lu)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen);

    /* buffer is larger as the required size */
    dwSize = (urllen+2);
    memset(szReturnUrl, '#', (urllen+4) * sizeof(WCHAR));
    szReturnUrl[urllen+4] = '\0';
    SetLastError(0xdeadbeef);
    hr = UrlCanonicalizeW(winehqW, szReturnUrl, &dwSize, URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE);
    ok( (hr == S_OK) && (dwSize == urllen),
        "got 0x%lx with %lu and size %lu for %u (expected 'S_OK' and size %lu)\n",
        hr, GetLastError(), dwSize, lstrlenW(szReturnUrl), urllen);

    /* Only ASCII alphanumeric characters are allowed in a scheme. */
    dwSize = ARRAY_SIZE(szReturnUrl);
    hr = UrlCanonicalizeW(L"f\xe8ve://./..", szReturnUrl, &dwSize, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(!wcscmp(szReturnUrl, L"f\xe8ve:/"), "Got URL %s.\n", debugstr_w(szReturnUrl));
    ok(dwSize == wcslen(szReturnUrl), "got size %lu\n", dwSize);

    /* check that the characters 1..32 are chopped from the end of the string */
    for (i = 1; i < 65536; i++)
    {
        WCHAR szUrl[128];
        BOOL choped;
        int pos;

        wcscpy(szUrl, L"http://www.winehq.org/X");
        pos = lstrlenW(szUrl) - 1;
        szUrl[pos] = i;
        urllen = INTERNET_MAX_URL_LENGTH;
        UrlCanonicalizeW(szUrl, szReturnUrl, &urllen, 0);
        choped = lstrlenW(szReturnUrl) < lstrlenW(szUrl);
        ok(choped == (i <= 32), "Incorrect char chopping for char %d\n", i);
    }
}

/* ########################### */

static void check_url_combine(const char *szUrl1, const char *szUrl2, DWORD dwFlags, const char *szExpectUrl)
{
    HRESULT hr;
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    WCHAR wszReturnUrl[INTERNET_MAX_URL_LENGTH];
    LPWSTR wszUrl1, wszUrl2, wszExpectUrl, wszConvertedUrl;

    DWORD dwSize;
    DWORD dwExpectLen = lstrlenA(szExpectUrl);

    wszUrl1 = GetWideString(szUrl1);
    wszUrl2 = GetWideString(szUrl2);
    wszExpectUrl = GetWideString(szExpectUrl);

    dwSize = ARRAY_SIZE(szReturnUrl);
    hr = UrlCombineA(szUrl1, szUrl2, szReturnUrl, &dwSize, dwFlags);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(dwSize == dwExpectLen, "Got length %ld, expected %ld\n", dwSize, dwExpectLen);
    ok(!strcmp(szReturnUrl, szExpectUrl), "Expected %s, got %s.\n", szExpectUrl, szReturnUrl);

    dwSize = ARRAY_SIZE(wszReturnUrl);
    hr = UrlCombineW(wszUrl1, wszUrl2, wszReturnUrl, &dwSize, dwFlags);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(dwSize == dwExpectLen, "Got length %ld, expected %ld\n", dwSize, dwExpectLen);
    wszConvertedUrl = GetWideString(szReturnUrl);
    ok(!wcscmp(wszReturnUrl, wszConvertedUrl), "Expected %s, got %s.\n",
            debugstr_w(wszConvertedUrl), debugstr_w(wszReturnUrl));
    FreeWideString(wszConvertedUrl);

    FreeWideString(wszUrl1);
    FreeWideString(wszUrl2);
    FreeWideString(wszExpectUrl);
}

/* ########################### */

static void test_UrlCombine(void)
{
    WCHAR bufferW[30];
    char buffer[30];
    unsigned int i;
    HRESULT hr;
    DWORD size;

    hr = UrlCombineA("http://base/", "relative", NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    size = 0;
    hr = UrlCombineA("http://base/", "relative", NULL, &size, 0);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    ok(size == strlen("http://base/relative") + 1, "Got size %lu.\n", size);

    --size;
    strcpy(buffer, "x");
    hr = UrlCombineA("http://base/", "relative", buffer, &size, 0);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    ok(size == strlen("http://base/relative") + 1, "Got size %lu.\n", size);
    ok(!strcmp(buffer, "x"), "Got buffer contents %s.\n", debugstr_a(buffer));

    strcpy(buffer, "x");
    hr = UrlCombineA("http://base/", "relative", buffer, &size, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == strlen("http://base/relative"), "Got size %lu.\n", size);
    ok(!strcmp(buffer, "http://base/relative"), "Got buffer contents %s.\n", debugstr_a(buffer));

    hr = UrlCombineW(L"http://base/", L"relative", NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Got hr %#lx.\n", hr);

    size = 0;
    hr = UrlCombineW(L"http://base/", L"relative", NULL, &size, 0);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    ok(size == strlen("http://base/relative") + 1, "Got size %lu.\n", size);

    --size;
    wcscpy(bufferW, L"x");
    hr = UrlCombineW(L"http://base/", L"relative", bufferW, &size, 0);
    ok(hr == E_POINTER, "Got hr %#lx.\n", hr);
    ok(size == strlen("http://base/relative") + 1, "Got size %lu.\n", size);
    ok(!wcscmp(bufferW, L"x"), "Got buffer contents %s.\n", debugstr_a(buffer));

    wcscpy(bufferW, L"x");
    hr = UrlCombineW(L"http://base/", L"relative", bufferW, &size, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(size == strlen("http://base/relative"), "Got size %lu.\n", size);
    ok(!wcscmp(bufferW, L"http://base/relative"), "Got buffer contents %s.\n", debugstr_w(bufferW));

    for (i = 0; i < ARRAY_SIZE(TEST_COMBINE); i++) {
        check_url_combine(TEST_COMBINE[i].url1, TEST_COMBINE[i].url2, TEST_COMBINE[i].flags, TEST_COMBINE[i].expecturl);
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

    for (i = 0; i < ARRAY_SIZE(TEST_URLFROMPATH); i++) {
        len = INTERNET_MAX_URL_LENGTH;
        ret = UrlCreateFromPathA(TEST_URLFROMPATH[i].path, ret_url, &len, 0);
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08lx from path %s\n", ret, TEST_URLFROMPATH[i].path);
        ok(!lstrcmpiA(ret_url, TEST_URLFROMPATH[i].url), "url %s from path %s\n", ret_url, TEST_URLFROMPATH[i].path);
        ok(len == strlen(ret_url), "ret len %ld from path %s\n", len, TEST_URLFROMPATH[i].path);

        len = INTERNET_MAX_URL_LENGTH;
        pathW = GetWideString(TEST_URLFROMPATH[i].path);
        urlW = GetWideString(TEST_URLFROMPATH[i].url);
        ret = UrlCreateFromPathW(pathW, ret_urlW, &len, 0);
        WideCharToMultiByte(CP_ACP, 0, ret_urlW, -1, ret_url, sizeof(ret_url),0,0);
        ok(ret == TEST_URLFROMPATH[i].ret, "ret %08lx from path L\"%s\", expected %08lx\n",
            ret, TEST_URLFROMPATH[i].path, TEST_URLFROMPATH[i].ret);
        ok(!lstrcmpiW(ret_urlW, urlW), "got %s expected %s from path L\"%s\"\n",
            ret_url, TEST_URLFROMPATH[i].url, TEST_URLFROMPATH[i].path);
        ok(len == lstrlenW(ret_urlW), "ret len %ld from path L\"%s\"\n", len, TEST_URLFROMPATH[i].path);
        FreeWideString(urlW);
        FreeWideString(pathW);
    }
}

/* ########################### */

static void test_UrlIs_null(DWORD flag)
{
    BOOL ret;
    ret = UrlIsA(NULL, flag);
    ok(ret == FALSE, "pUrlIsA(NULL, %ld) failed\n", flag);
    ret = UrlIsW(NULL, flag);
    ok(ret == FALSE, "pUrlIsW(NULL, %ld) failed\n", flag);
}

static void test_UrlIs(void)
{
    BOOL ret;
    size_t i;
    WCHAR wurl[80];

    test_UrlIs_null(URLIS_APPLIABLE);
    test_UrlIs_null(URLIS_DIRECTORY);
    test_UrlIs_null(URLIS_FILEURL);
    test_UrlIs_null(URLIS_HASQUERY);
    test_UrlIs_null(URLIS_NOHISTORY);
    test_UrlIs_null(URLIS_OPAQUE);
    test_UrlIs_null(URLIS_URL);

    for (i = 0; i < ARRAY_SIZE(TEST_PATH_IS_URL); i++) {
        MultiByteToWideChar(CP_ACP, 0, TEST_PATH_IS_URL[i].path, -1, wurl, ARRAY_SIZE(wurl));

        ret = UrlIsA( TEST_PATH_IS_URL[i].path, URLIS_URL );
        ok( ret == TEST_PATH_IS_URL[i].expect,
            "returned %d from path %s, expected %d\n", ret, TEST_PATH_IS_URL[i].path,
            TEST_PATH_IS_URL[i].expect );

        ret = UrlIsW( wurl, URLIS_URL );
        ok( ret == TEST_PATH_IS_URL[i].expect,
            "returned %d from path (UrlIsW) %s, expected %d\n", ret,
            TEST_PATH_IS_URL[i].path, TEST_PATH_IS_URL[i].expect );
    }
    for (i = 0; i < ARRAY_SIZE(TEST_URLIS_ATTRIBS); i++) {
        MultiByteToWideChar(CP_ACP, 0, TEST_URLIS_ATTRIBS[i].url, -1, wurl, ARRAY_SIZE(wurl));

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
            "returned %d for URLIS_OPAQUE (UrlIsW), url \"%s\", expected %d\n",
            ret, TEST_URLIS_ATTRIBS[i].url, TEST_URLIS_ATTRIBS[i].expectOpaque );
        ret = UrlIsW( wurl, URLIS_FILEURL);
        ok( ret == TEST_URLIS_ATTRIBS[i].expectFile,
            "returned %d for URLIS_FILEURL (UrlIsW), url \"%s\", expected %d\n",
            ret, TEST_URLIS_ATTRIBS[i].url, TEST_URLIS_ATTRIBS[i].expectFile );
    }
}

/* ########################### */

static void test_UrlUnescape(void)
{
    WCHAR urlW[INTERNET_MAX_URL_LENGTH], bufferW[INTERNET_MAX_URL_LENGTH];
    CHAR szReturnUrl[INTERNET_MAX_URL_LENGTH];
    DWORD dwEscaped, unescaped;
    BOOL utf8_support = TRUE;
    static char inplace[] = "file:///C:/Program%20Files";
    static char another_inplace[] = "file:///C:/Program%20Files";
    static const char expected[] = "file:///C:/Program Files";
    HRESULT res;
    int i;

    for (i = 0; i < ARRAY_SIZE(TEST_URL_UNESCAPE); i++) {
        dwEscaped=INTERNET_MAX_URL_LENGTH;
        res = UrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, &dwEscaped, 0);
        ok(res == S_OK,
            "UrlUnescapeA returned 0x%lx (expected S_OK) for \"%s\"\n",
            res, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,TEST_URL_UNESCAPE[i].expect)==0, "Expected \"%s\", but got \"%s\" from \"%s\"\n", TEST_URL_UNESCAPE[i].expect, szReturnUrl, TEST_URL_UNESCAPE[i].url);

        ZeroMemory(szReturnUrl, sizeof(szReturnUrl));
        /* if we set the buffer pointer to NULL here, UrlUnescape fails and the string is not converted */
        res = UrlUnescapeA(TEST_URL_UNESCAPE[i].url, szReturnUrl, NULL, 0);
        ok(res == E_INVALIDARG,
            "UrlUnescapeA returned 0x%lx (expected E_INVALIDARG) for \"%s\"\n",
            res, TEST_URL_UNESCAPE[i].url);
        ok(strcmp(szReturnUrl,"")==0, "Expected empty string\n");
    }

    unescaped = INTERNET_MAX_URL_LENGTH;
    lstrcpyW(urlW, L"%F0%9F%8D%B7");
    res = UrlUnescapeW(urlW, NULL, &unescaped, URL_UNESCAPE_AS_UTF8 | URL_UNESCAPE_INPLACE);
    ok(res == S_OK, "Got %#lx.\n", res);
    if (!wcscmp(urlW, L"\xf0\x9f\x8d\xb7"))
    {
        utf8_support = FALSE;
        win_skip("Skip URL_UNESCAPE_AS_UTF8 tests for pre-win7 systems.\n");
    }

    for (i = 0; i < ARRAYSIZE(TEST_URL_UNESCAPEW); i++)
    {
        if (TEST_URL_UNESCAPEW[i].flags & URL_UNESCAPE_AS_UTF8 && !utf8_support)
            continue;

        lstrcpyW(urlW, TEST_URL_UNESCAPEW[i].url);

        memset(bufferW, 0xff, sizeof(bufferW));
        unescaped = INTERNET_MAX_URL_LENGTH;
        res = UrlUnescapeW(urlW, bufferW, &unescaped, TEST_URL_UNESCAPEW[i].flags);
        ok(res == S_OK, "[%d]: returned %#lx.\n", i, res);
        ok(unescaped == wcslen(TEST_URL_UNESCAPEW[i].expect), "[%d]: got unescaped %ld.\n", i, unescaped);
        ok(!wcscmp(bufferW, TEST_URL_UNESCAPEW[i].expect), "[%d]: got result %s.\n", i, debugstr_w(bufferW));

        /* Test with URL_UNESCAPE_INPLACE */
        unescaped = INTERNET_MAX_URL_LENGTH;
        res = UrlUnescapeW(urlW, NULL, &unescaped, TEST_URL_UNESCAPEW[i].flags | URL_UNESCAPE_INPLACE);
        ok(res == S_OK, "[%d]: returned %#lx.\n", i, res);
        ok(unescaped == INTERNET_MAX_URL_LENGTH, "[%d]: got unescaped %ld.\n", i, unescaped);
        ok(!wcscmp(urlW, TEST_URL_UNESCAPEW[i].expect), "[%d]: got result %s.\n", i, debugstr_w(urlW));

        lstrcpyW(urlW, TEST_URL_UNESCAPEW[i].url);
        unescaped = wcslen(TEST_URL_UNESCAPEW[i].expect) - 1;
        res = UrlUnescapeW(urlW, bufferW, &unescaped, TEST_URL_UNESCAPEW[i].flags);
        ok(res == E_POINTER, "[%d]: returned %#lx.\n", i, res);
    }

    dwEscaped = sizeof(inplace);
    res = UrlUnescapeA(inplace, NULL, &dwEscaped, URL_UNESCAPE_INPLACE);
    ok(res == S_OK, "UrlUnescapeA returned 0x%lx (expected S_OK)\n", res);
    ok(!strcmp(inplace, expected), "got %s expected %s\n", inplace, expected);
    ok(dwEscaped == 27, "got %ld expected 27\n", dwEscaped);

    /* if we set the buffer pointer to NULL, the string apparently still gets converted (Google Lively does this) */
    res = UrlUnescapeA(another_inplace, NULL, NULL, URL_UNESCAPE_INPLACE);
    ok(res == S_OK, "UrlUnescapeA returned 0x%lx (expected S_OK)\n", res);
    ok(!strcmp(another_inplace, expected), "got %s expected %s\n", another_inplace, expected);
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
    {"HTTP://www.winehq.org/",S_OK,4,URL_SCHEME_HTTP},
    {"a+-.://www.winehq.org/",S_OK,4,URL_SCHEME_UNKNOWN},
};

static void test_ParseURL(void)
{
    const struct parse_url_test_t *test;
    WCHAR url[INTERNET_MAX_URL_LENGTH];
    PARSEDURLA parseda;
    PARSEDURLW parsedw;
    HRESULT hres;

    for (test = parse_url_tests; test < parse_url_tests + ARRAY_SIZE(parse_url_tests); test++) {
        memset(&parseda, 0xd0, sizeof(parseda));
        parseda.cbSize = sizeof(parseda);
        hres = ParseURLA(test->url, &parseda);
        ok(hres == test->hres, "ParseURL failed: %08lx, expected %08lx\n", hres, test->hres);
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

        MultiByteToWideChar(CP_ACP, 0, test->url, -1, url, ARRAY_SIZE(url));

        memset(&parsedw, 0xd0, sizeof(parsedw));
        parsedw.cbSize = sizeof(parsedw);
        hres = ParseURLW(url, &parsedw);
        ok(hres == test->hres, "ParseURL failed: %08lx, expected %08lx\n", hres, test->hres);
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
    res = HashData(input, 16, output, 16);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08lx\n", res);
    ok(!memcmp(output, expected, sizeof(expected)), "data didn't match\n");

    /* Test hashing with larger output buffer. */
    res = HashData(input, 16, output, 32);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08lx\n", res);
    ok(!memcmp(output, expected2, sizeof(expected2)), "data didn't match\n");

    /* Test hashing with smaller input buffer. */
    res = HashData(input, 8, output, 16);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08lx\n", res);
    ok(!memcmp(output, expected3, sizeof(expected3)), "data didn't match\n");

    /* Test passing NULL pointers for input/output parameters. */
    res = HashData(NULL, 0, NULL, 0);
    ok(res == E_INVALIDARG, "Got unexpected hr %#lx.\n", res);

    res = HashData(input, 0, NULL, 0);
    ok(res == E_INVALIDARG, "Got unexpected hr %#lx.\n", res);

    res = HashData(NULL, 0, output, 0);
    ok(res == E_INVALIDARG, "Got unexpected hr %#lx.\n", res);

    /* Test passing valid pointers with sizes of zero. */
    for (i = 0; i < ARRAY_SIZE(input); i++)
        input[i] = 0x00;

    for (i = 0; i < ARRAY_SIZE(output); i++)
        output[i] = 0xFF;

    res = HashData(input, 0, output, 0);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08lx\n", res);

    /* The buffers should be unchanged. */
    for (i = 0; i < ARRAY_SIZE(input); i++)
        ok(input[i] == 0x00, "Expected the input buffer to be unchanged\n");

    for (i = 0; i < ARRAY_SIZE(output); i++)
        ok(output[i] == 0xFF, "Expected the output buffer to be unchanged\n");

    /* Input/output parameters are not validated. */
    res = HashData((BYTE *)0xdeadbeef, 0, (BYTE *)0xdeadbeef, 0);
    ok(res == S_OK, "Expected HashData to return S_OK, got 0x%08lx\n", res);

    if (0)
    {
        res = HashData((BYTE *)0xdeadbeef, 1, (BYTE *)0xdeadbeef, 1);
        trace("HashData returned 0x%08lx\n", res);
    }
}

/* ########################### */

START_TEST(url)
{
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
