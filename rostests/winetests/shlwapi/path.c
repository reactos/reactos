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

/* ################ */

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


static struct {
    const char *path;
    BOOL expect;
} TEST_PATH_IS_URL[] = {
    {"http://foo/bar", TRUE},
    {"c:\\foo\\bar", FALSE},
    {"c:/foo/bar", FALSE},
    {"foo://foo/bar", TRUE},
    {"foo\\bar", FALSE},
    {"foo.bar", FALSE},
    {"bogusscheme:", TRUE},
    {"http:partial", TRUE},
    {"www.winehq.org", FALSE},
    /* More examples that the user might enter as the browser start page */
    {"winehq.org", FALSE},
    {"ftp.winehq.org", FALSE},
    {"http://winehq.org", TRUE},
    {"http://www.winehq.org", TRUE},
    {"https://winehq.org", TRUE},
    {"https://www.winehq.org", TRUE},
    {"ftp://winehq.org", TRUE},
    {"ftp://ftp.winehq.org", TRUE},
    {"file://does_not_exist.txt", TRUE},
    {"about:blank", TRUE},
    {"about:home", TRUE},
    {"about:mozilla", TRUE},
    /* scheme is case independent */
    {"HTTP://www.winehq.org", TRUE},
    /* a space at the start is not allowed */
    {" http://www.winehq.org", FALSE},
    {"", FALSE},
    {NULL, FALSE}
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

/* ################ */

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

/* ################ */

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

    for (c = 0; c < 0x7f; c++)
    {
        ret = pPathIsValidCharA( c, ~0U );
        ok ( ret || !SHELL_charclass[c], "PathIsValidCharA failed: 0x%02x got 0x%08x\n", c, ret );
    }

    for (c = 0x7f; c <= 0xff; c++)
    {
        ret = pPathIsValidCharA( c, ~0U );
        ok ( ret, "PathIsValidCharA failed: 0x%02x got 0x%08x\n", c, ret );
    }
}

static void test_PathIsValidCharW(void)
{
    BOOL ret;
    unsigned int c;

    for (c = 0; c < 0x7f; c++)
    {
        ret = pPathIsValidCharW( c, ~0U );
        ok ( ret || !SHELL_charclass[c], "PathIsValidCharW failed: 0x%02x got 0x%08x\n", c, ret );
    }

    for (c = 0x007f; c <= 0xffff; c++)
    {
        ret = pPathIsValidCharW( c, ~0U );
        ok ( ret, "PathIsValidCharW failed: 0x%02x got 0x%08x\n", c, ret );
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
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());
    /* Vista fails which probably makes sense as PathCombineA expects an absolute dir */
    if (str)
    {
        ok(str == dest, "Expected str == dest, got %p\n", str);
        ok(!lstrcmp(str, "one\\two\\three\\"), "Expected one\\two\\three\\, got %s\n", str);
    }

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
    char dest[LONG_LEN + MAX_PATH];
    char too_long[LONG_LEN];
    BOOL res;

    /* try a NULL source */
    lstrcpy(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, NULL);
    ok(!res, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, 
       "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    ok(dest[0] == 0 || !lstrcmp(dest, "test"),
       "Expected either an empty string (Vista) or test, got %s\n", dest);

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
    ok(!lstrcmp(dest, "C:\\one/.") ||
       !lstrcmp(dest, "C:\\one/"), /* Vista */
       "Expected \"C:\\one/.\" or \"C:\\one/\", got \"%s\"\n", dest);

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
    ok(!res, "Expected failure\n");
    todo_wine
    {
        ok(GetLastError() == 0xdeadbeef || GetLastError() == ERROR_FILENAME_EXCED_RANGE /* Vista */,
        "Expected 0xdeadbeef or ERROR_FILENAME_EXCED_RANGE, got %d\n", GetLastError());
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
    ok(!lstrcmp(path, "aaaaaaaaa") ||
       lstrlenA(path) == 0, /* Vista */
       "Expected aaaaaaaaa or empty string, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %d\n", GetLastError());

    /* test a drive number greater than 25 */
    SetLastError(0xdeadbeef);
    lstrcpy(path, "aaaaaaaaa");
    root = PathBuildRootA(path, 26);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmp(path, "aaaaaaaaa") ||
       lstrlenA(path) == 0, /* Vista */
       "Expected aaaaaaaaa or empty string, got %s\n", path);
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

/* ################ */

START_TEST(path)
{
  hShlwapi = GetModuleHandleA("shlwapi.dll");

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
