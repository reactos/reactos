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

#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wininet.h"

static BOOL (WINAPI *pPathIsValidCharA)(char,DWORD);
static BOOL (WINAPI *pPathIsValidCharW)(WCHAR,DWORD);
static LPWSTR  (WINAPI *pPathCombineW)(LPWSTR, LPCWSTR, LPCWSTR);
static HRESULT (WINAPI *pPathCreateFromUrlA)(LPCSTR, LPSTR, LPDWORD, DWORD);
static HRESULT (WINAPI *pPathCreateFromUrlW)(LPCWSTR, LPWSTR, LPDWORD, DWORD);
static HRESULT (WINAPI *pPathCreateFromUrlAlloc)(LPCWSTR, LPWSTR*, DWORD);
static BOOL    (WINAPI *pPathAppendA)(LPSTR, LPCSTR);
static BOOL    (WINAPI *pPathUnExpandEnvStringsA)(LPCSTR, LPSTR, UINT);
static BOOL    (WINAPI *pPathUnExpandEnvStringsW)(LPCWSTR, LPWSTR, UINT);
static BOOL    (WINAPI *pPathIsRelativeA)(LPCSTR);
static BOOL    (WINAPI *pPathIsRelativeW)(LPCWSTR);

/* ################ */

static const struct {
    const char *url;
    const char *path;
    DWORD ret, todo;
} TEST_PATHFROMURL[] = {
    /* 0 leading slash */
    {"file:c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:c|/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:cx|/foo/bar", "cx|\\foo\\bar", S_OK, 0},
    {"file:c:foo/bar", "c:foo\\bar", S_OK, 0},
    {"file:c|foo/bar", "c:foo\\bar", S_OK, 0},
    {"file:c:/foo%20ba%2fr", "c:\\foo ba/r", S_OK, 0},
    {"file:foo%20ba%2fr", "foo ba/r", S_OK, 0},
    {"file:foo/bar/", "foo\\bar\\", S_OK, 0},

    /* 1 leading (back)slash */
    {"file:/c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:\\c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:/c|/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:/cx|/foo/bar", "\\cx|\\foo\\bar", S_OK, 0},
    {"file:/c:foo/bar", "c:foo\\bar", S_OK, 0},
    {"file:/c|foo/bar", "c:foo\\bar", S_OK, 0},
    {"file:/c:/foo%20ba%2fr", "c:\\foo ba/r", S_OK, 0},
    {"file:/foo%20ba%2fr", "\\foo ba/r", S_OK, 0},
    {"file:/foo/bar/", "\\foo\\bar\\", S_OK, 0},

    /* 2 leading (back)slashes */
    {"file://c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file://c:/d:/foo/bar", "c:\\d:\\foo\\bar", S_OK, 0},
    {"file://c|/d|/foo/bar", "c:\\d|\\foo\\bar", S_OK, 0},
    {"file://cx|/foo/bar", "\\\\cx|\\foo\\bar", S_OK, 0},
    {"file://c:foo/bar", "c:foo\\bar", S_OK, 0},
    {"file://c|foo/bar", "c:foo\\bar", S_OK, 0},
    {"file://c:/foo%20ba%2fr", "c:\\foo%20ba%2fr", S_OK, 0},
    {"file://c%3a/foo/../bar", "\\\\c:\\foo\\..\\bar", S_OK, 0},
    {"file://c%7c/foo/../bar", "\\\\c|\\foo\\..\\bar", S_OK, 0},
    {"file://foo%20ba%2fr", "\\\\foo ba/r", S_OK, 0},
    {"file://localhost/c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file://localhost/c:/foo%20ba%5Cr", "c:\\foo ba\\r", S_OK, 0},
    {"file://LocalHost/c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:\\\\localhost\\c:\\foo\\bar", "c:\\foo\\bar", S_OK, 0},
    {"file://incomplete", "\\\\incomplete", S_OK, 0},

    /* 3 leading (back)slashes (omitting hostname) */
    {"file:///c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"File:///c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:///c:/foo%20ba%2fr", "c:\\foo ba/r", S_OK, 0},
    {"file:///foo%20ba%2fr", "\\foo ba/r", S_OK, 0},
    {"file:///foo/bar/", "\\foo\\bar\\", S_OK, 0},
    {"file:///localhost/c:/foo/bar", "\\localhost\\c:\\foo\\bar", S_OK, 0},

    /* 4 leading (back)slashes */
    {"file:////c:/foo/bar", "c:\\foo\\bar", S_OK, 0},
    {"file:////c:/foo%20ba%2fr", "c:\\foo%20ba%2fr", S_OK, 0},
    {"file:////foo%20ba%2fr", "\\\\foo%20ba%2fr", S_OK, 0},

    /* 5 and more leading (back)slashes */
    {"file://///c:/foo/bar", "\\\\c:\\foo\\bar", S_OK, 0},
    {"file://///c:/foo%20ba%2fr", "\\\\c:\\foo ba/r", S_OK, 0},
    {"file://///foo%20ba%2fr", "\\\\foo ba/r", S_OK, 0},
    {"file://////c:/foo/bar", "\\\\c:\\foo\\bar", S_OK, 0},

    /* Leading (back)slashes cannot be escaped */
    {"file:%2f%2flocalhost%2fc:/foo/bar", "//localhost/c:\\foo\\bar", S_OK, 0},
    {"file:%5C%5Clocalhost%5Cc:/foo/bar", "\\\\localhost\\c:\\foo\\bar", S_OK, 0},

    /* Hostname handling */
    {"file://l%6fcalhost/c:/foo/bar", "\\\\localhostc:\\foo\\bar", S_OK, 0},
    {"file://localhost:80/c:/foo/bar", "\\\\localhost:80c:\\foo\\bar", S_OK, 0},
    {"file://host/c:/foo/bar", "\\\\hostc:\\foo\\bar", S_OK, 0},
    {"file://host//c:/foo/bar", "\\\\host\\\\c:\\foo\\bar", S_OK, 0},
    {"file://host/\\c:/foo/bar", "\\\\host\\\\c:\\foo\\bar", S_OK, 0},
    {"file://host/c:foo/bar", "\\\\hostc:foo\\bar", S_OK, 0},
    {"file://host/foo/bar", "\\\\host\\foo\\bar", S_OK, 0},
    {"file:\\\\host\\c:\\foo\\bar", "\\\\hostc:\\foo\\bar", S_OK, 0},
    {"file:\\\\host\\ca\\foo\\bar", "\\\\host\\ca\\foo\\bar", S_OK, 0},
    {"file:\\\\host\\c|\\foo\\bar", "\\\\hostc|\\foo\\bar", S_OK, 0},
    {"file:\\%5Chost\\c:\\foo\\bar", "\\\\host\\c:\\foo\\bar", S_OK, 0},
    {"file:\\\\host\\cx:\\foo\\bar", "\\\\host\\cx:\\foo\\bar", S_OK, 0},
    {"file:///host/c:/foo/bar", "\\host\\c:\\foo\\bar", S_OK, 0},

    /* Not file URLs */
    {"c:\\foo\\bar", NULL, E_INVALIDARG, 0},
    {"foo/bar", NULL, E_INVALIDARG, 0},
    {"http://foo/bar", NULL, E_INVALIDARG, 0},

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

static const struct {
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

static LPWSTR GetWideString(const char *src)
{
  WCHAR *ret;

  if (!src)
    return NULL;

  ret = malloc(2 * INTERNET_MAX_URL_LENGTH * sizeof(WCHAR));

  MultiByteToWideChar(CP_ACP, 0, src, -1, ret, INTERNET_MAX_URL_LENGTH);

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
    DWORD len, len2, ret;
    WCHAR ret_pathW[INTERNET_MAX_URL_LENGTH];
    WCHAR *pathW, *urlW;

    if (!pPathCreateFromUrlA) {
        win_skip("PathCreateFromUrlA not found\n");
        return;
    }

    /* Won't say how much is needed without a buffer */
    len = 0xdeca;
    ret = pPathCreateFromUrlA("file://foo", NULL, &len, 0);
    ok(ret == E_INVALIDARG, "got 0x%08lx expected E_INVALIDARG\n", ret);
    ok(len == 0xdeca, "got %lx expected 0xdeca\n", len);

    /* Test the decoding itself */
    for (i = 0; i < ARRAY_SIZE(TEST_PATHFROMURL); i++) {
        len = INTERNET_MAX_URL_LENGTH;
        ret = pPathCreateFromUrlA(TEST_PATHFROMURL[i].url, ret_path, &len, 0);
        todo_wine_if (TEST_PATHFROMURL[i].todo & 0x1)
            ok(ret == TEST_PATHFROMURL[i].ret, "ret %08lx from url %s\n", ret, TEST_PATHFROMURL[i].url);
        if(SUCCEEDED(ret) && TEST_PATHFROMURL[i].path) {
            if(!(TEST_PATHFROMURL[i].todo & 0x2)) {
                ok(!lstrcmpiA(ret_path, TEST_PATHFROMURL[i].path), "got %s expected %s from url %s\n", ret_path, TEST_PATHFROMURL[i].path,  TEST_PATHFROMURL[i].url);
                ok(len == strlen(ret_path), "ret len %ld from url %s\n", len, TEST_PATHFROMURL[i].url);
            } else todo_wine
                /* Wrong string, don't bother checking the length */
                ok(!lstrcmpiA(ret_path, TEST_PATHFROMURL[i].path), "got %s expected %s from url %s\n", ret_path, TEST_PATHFROMURL[i].path,  TEST_PATHFROMURL[i].url);
        }

        if (pPathCreateFromUrlW) {
            len = INTERNET_MAX_URL_LENGTH;
            pathW = GetWideString(TEST_PATHFROMURL[i].path);
            urlW = GetWideString(TEST_PATHFROMURL[i].url);
            ret = pPathCreateFromUrlW(urlW, ret_pathW, &len, 0);
            WideCharToMultiByte(CP_ACP, 0, ret_pathW, -1, ret_path, sizeof(ret_path),NULL,NULL);
            todo_wine_if (TEST_PATHFROMURL[i].todo & 0x1)
                ok(ret == TEST_PATHFROMURL[i].ret, "ret %08lx from url L\"%s\"\n", ret, TEST_PATHFROMURL[i].url);
            if(SUCCEEDED(ret) && TEST_PATHFROMURL[i].path) {
                if(!(TEST_PATHFROMURL[i].todo & 0x2)) {
                    ok(!lstrcmpiW(ret_pathW, pathW), "got %s expected %s from url L\"%s\"\n",
                       ret_path, TEST_PATHFROMURL[i].path, TEST_PATHFROMURL[i].url);
                    ok(len == lstrlenW(ret_pathW), "ret len %ld from url L\"%s\"\n", len, TEST_PATHFROMURL[i].url);
                } else todo_wine
                    /* Wrong string, don't bother checking the length */
                    ok(!lstrcmpiW(ret_pathW, pathW), "got %s expected %s from url L\"%s\"\n",
                       ret_path, TEST_PATHFROMURL[i].path, TEST_PATHFROMURL[i].url);
            }

            if (SUCCEEDED(ret))
            {
                /* Check what happens if the buffer is too small */
                len2 = 2;
                ret = pPathCreateFromUrlW(urlW, ret_pathW, &len2, 0);
                ok(ret == E_POINTER, "ret %08lx, expected E_POINTER from url %s\n", ret, TEST_PATHFROMURL[i].url);
                todo_wine_if (TEST_PATHFROMURL[i].todo & 0x4)
                    ok(len2 == len + 1, "got len = %ld expected %ld from url %s\n", len2, len + 1, TEST_PATHFROMURL[i].url);
            }

            free(urlW);
            free(pathW);
        }
    }

    if (pPathCreateFromUrlAlloc)
    {
        static const WCHAR fileW[] = {'f','i','l','e',':','/','/','f','o','o',0};
        static const WCHAR fooW[] = {'\\','\\','f','o','o',0};

        pathW = NULL;
        ret = pPathCreateFromUrlAlloc(fileW, &pathW, 0);
        ok(ret == S_OK, "got 0x%08lx expected S_OK\n", ret);
        ok(lstrcmpiW(pathW, fooW) == 0, "got %s expected %s\n", wine_dbgstr_w(pathW), wine_dbgstr_w(fooW));
        LocalFree(pathW);
    }
}


static void test_PathIsUrl(void)
{
    size_t i;
    BOOL ret;

    for (i = 0; i < ARRAY_SIZE(TEST_PATH_IS_URL); i++) {
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

    /* For whatever reason, PathIsValidCharA and PathAppendA share the same
     * ordinal number in some native versions. Check this to prevent a crash.
     */
    if (!pPathIsValidCharA || pPathIsValidCharA == (void*)pPathAppendA)
    {
        win_skip("PathIsValidCharA isn't available\n");
        return;
    }

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

    if (!pPathIsValidCharW)
    {
        win_skip("PathIsValidCharW isn't available\n");
        return;
    }

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

    if (!pPathCombineW)
    {
        win_skip("PathCombineW isn't available\n");
        return;
    }

    wszString2 = malloc(MAX_PATH * sizeof(WCHAR));

    /* NULL test */
    wszString = pPathCombineW(NULL, NULL, NULL);
    ok (wszString == NULL, "Expected a NULL return\n");

    /* Some NULL */
    wszString2[0] = 'a';
    wszString = pPathCombineW(wszString2, NULL, NULL);
    ok (wszString == NULL ||
        broken(wszString[0] == 'a'), /* Win95 and some W2K */
        "Expected a NULL return\n");
    ok (wszString2[0] == 0 ||
        broken(wszString2[0] == 'a'), /* Win95 and some W2K */
        "Destination string not empty\n");

    free(wszString2);

    /* overflow test */
    wstr2[0] = wstr2[1] = wstr2[2] = 'A';
    for (i=3; i<MAX_PATH/2; i++)
        wstr1[i] = wstr2[i] = 'A';
    wstr1[(MAX_PATH/2) - 1] = wstr2[MAX_PATH/2] = 0;
    memset(wbuf, 0xbf, sizeof(wbuf));

    wszString = pPathCombineW(wbuf, wstr1, wstr2);
    ok(wszString == NULL, "Expected a NULL return\n");
    ok(wbuf[0] == 0 ||
       broken(wbuf[0] == 0xbfbf), /* Win95 and some W2K */
       "Buffer contains data\n");

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
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try NULL dest and NULL directory */
    SetLastError(0xdeadbeef);
    str = PathCombineA(NULL, NULL, "one\\two\\three");
    ok(str == NULL, "Expected NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try all NULL*/
    SetLastError(0xdeadbeef);
    str = PathCombineA(NULL, NULL, NULL);
    ok(str == NULL, "Expected NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try NULL file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", NULL);
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\"), "Expected C:\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try empty file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\"), "Expected C:\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try empty directory and file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "", "");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "\\") ||
       broken(!lstrcmpA(str, "control")), /* Win95 and some W2K */
       "Expected \\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try NULL directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, NULL, "one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "one\\two\\three"), "Expected one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try NULL directory and empty file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, NULL, "");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "\\") ||
       broken(!lstrcmpA(str, "one\\two\\three")), /* Win95 and some W2K */
       "Expected \\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef ||
       broken(GetLastError() == ERROR_INVALID_PARAMETER), /* Win95 */
       "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try NULL directory and file part */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, NULL, NULL);
    ok(str == NULL ||
       broken(str != NULL), /* Win95 and some W2K */
       "Expected str == NULL, got %p\n", str);
    ok(!dest[0] || broken(!lstrcmpA(dest, "control")), /* Win95 and some W2K */
       "Expected 0 length, got %i\n", lstrlenA(dest));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try directory without backslash */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:", "one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try directory with backslash */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try directory with backslash and file with prepended backslash */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "\\one\\two\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try previous test, with backslash appended as well */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "\\one\\two\\three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a relative directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "relative\\dir", "\\one\\two\\three\\");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    /* Vista fails which probably makes sense as PathCombineA expects an absolute dir */
    if (str)
    {
        ok(str == dest, "Expected str == dest, got %p\n", str);
        ok(!lstrcmpA(str, "one\\two\\three\\"), "Expected one\\two\\three\\, got %s\n", str);
    }

    /* try forward slashes */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "one/two/three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\one/two/three\\"), "Expected one/two/three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a really weird directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\/\\/", "\\one\\two\\three\\");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try periods */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "one\\..\\two\\.\\three");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\two\\three"), "Expected C:\\two\\three, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try .. as file */
    /* try forward slashes */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", "..");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "C:\\"), "Expected C:\\, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try relative paths */
    /* try forward slashes */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "../../../one/two/", "*");
    ok(str == dest, "Expected str == dest, got %p\n", str);
    ok(!lstrcmpA(str, "../../../one/two/\\*"), "Expected ../../../one/two/\\*, got %s\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';

    /* try a file longer than MAX_PATH */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, "C:\\", too_long);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(!dest[0] || broken(!lstrcmpA(dest, "control")), /* Win95 and some W2K */
       "Expected 0 length, got %i\n", lstrlenA(dest));
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a directory longer than MAX_PATH */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, too_long, "one\\two\\three");
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(!dest[0] || broken(!lstrcmpA(dest, "control")), /* Win95 and some W2K */
       "Expected 0 length, got %i\n", lstrlenA(dest));
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    memset(one, 'b', HALF_LEN);
    memset(two, 'c', HALF_LEN);
    one[HALF_LEN - 1] = '\0';
    two[HALF_LEN - 1] = '\0';

    /* destination string is longer than MAX_PATH, but not the constituent parts */
    SetLastError(0xdeadbeef);
    lstrcpyA(dest, "control");
    str = PathCombineA(dest, one, two);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(!dest[0] || broken(!lstrcmpA(dest, "control")), /* Win95 and some W2K */
       "Expected 0 length, got %i\n", lstrlenA(dest));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
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
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try an empty path */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!path[0], "Expected empty string, got %i\n", lstrlenA(path));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a relative path */
    lstrcpyA(path, "one\\two");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmpA(path, "one\\two\\"), "Expected one\\two\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try periods */
    lstrcpyA(path, "one\\..\\two");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmpA(path, "one\\..\\two\\"), "Expected one\\..\\two\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try just a space */
    lstrcpyA(path, " ");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmpA(path, " \\"), "Expected  \\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* path already has backslash */
    lstrcpyA(path, "C:\\one\\");
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(path);
    ok(str == (path + lstrlenA(path)), "Expected str to point to end of path, got %p\n", str);
    ok(!lstrcmpA(path, "C:\\one\\"), "Expected C:\\one\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';

    /* path is longer than MAX_PATH */
    SetLastError(0xdeadbeef);
    str = PathAddBackslashA(too_long);
    ok(str == NULL, "Expected str == NULL, got %p\n", str);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
}

static void test_PathAppendA(void)
{
    char path[MAX_PATH];
    char too_long[LONG_LEN];
    char half[HALF_LEN];
    BOOL res;

    lstrcpyA(path, "C:\\one");

    /* try NULL pszMore */
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, NULL);
    ok(!res, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one"), "Expected C:\\one, got %s\n", path);

    /* try empty pszMore */
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one"), "Expected C:\\one, got %s\n", path);

    /* try NULL pszPath */
    SetLastError(0xdeadbeef);
    res = PathAppendA(NULL, "two\\three");
    ok(!res, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try empty pszPath */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "two\\three"), "Expected \\two\\three, got %s\n", path);

    /* try empty pszPath and empty pszMore */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "\\"), "Expected \\, got %s\n", path);

    /* try legit params */
    lstrcpyA(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", path);

    /* try pszPath with backslash after it */
    lstrcpyA(path, "C:\\one\\");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", path);

    /* try pszMore with backslash before it */
    lstrcpyA(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "\\two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one\\two\\three"), "Expected C:\\one\\two\\three, got %s\n", path);

    /* try pszMore with backslash after it */
    lstrcpyA(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three\\");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one\\two\\three\\"), "Expected C:\\one\\two\\three\\, got %s\n", path);

    /* try spaces in pszPath */
    lstrcpyA(path, "C: \\ one ");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, "two\\three");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C: \\ one \\two\\three"), "Expected C: \\ one \\two\\three, got %s\n", path);

    /* try spaces in pszMore */
    lstrcpyA(path, "C:\\one");
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, " two \\ three ");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(path, "C:\\one\\ two \\ three "), "Expected 'C:\\one\\ two \\ three ', got %s\n", path);

    /* pszPath is too long */
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(too_long, "two\\three");
    ok(!res, "Expected failure\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!too_long[0] || broken(lstrlenA(too_long) == (LONG_LEN - 1)), /* Win95 and some W2K */
       "Expected length of too_long to be zero, got %i\n", lstrlenA(too_long));

    /* pszMore is too long */
    lstrcpyA(path, "C:\\one");
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, too_long);
    ok(!res, "Expected failure\n");
    todo_wine ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!path[0] || broken(!lstrcmpA(path, "C:\\one")), /* Win95 and some W2K */
       "Expected length of path to be zero, got %i\n", lstrlenA(path));

    /* both params combined are too long */
    memset(path, 'a', HALF_LEN);
    path[HALF_LEN - 1] = '\0';
    memset(half, 'b', HALF_LEN);
    half[HALF_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    res = PathAppendA(path, half);
    ok(!res, "Expected failure\n");
    ok(!path[0] || broken(lstrlenA(path) == (HALF_LEN - 1)), /* Win95 and some W2K */
       "Expected length of path to be zero, got %i\n", lstrlenA(path));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
}

static void test_PathCanonicalizeA(void)
{
    char dest[LONG_LEN + MAX_PATH];
    char too_long[LONG_LEN];
    BOOL res;

    /* try a NULL source */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, NULL);
    ok(!res, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, 
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    ok(dest[0] == 0 || !lstrcmpA(dest, "test"),
       "Expected either an empty string (Vista) or test, got %s\n", dest);

    /* try an empty source */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "\\") ||
       broken(!lstrcmpA(dest, "test")), /* Win95 and some W2K */
       "Expected \\, got %s\n", dest);

    /* try a NULL dest */
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(NULL, "C:\\");
    ok(!res, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, 
       "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* try empty dest */
    dest[0] = '\0';
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\"), "Expected C:\\, got %s\n", dest);

    /* try non-empty dest */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\"), "Expected C:\\, got %s\n", dest);

    /* try a space for source */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, " ");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, " "), "Expected ' ', got %s\n", dest);

    /* try a relative path */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "one\\two");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "one\\two"), "Expected one\\two, got %s\n", dest);

    /* try current dir and previous dir */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one\\.\\..\\two\\three\\..");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\two"), "Expected C:\\two, got %s\n", dest);

    /* try simple forward slashes */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/two/three\\four/five\\six");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\one/two/three\\four/five\\six"),
       "Expected C:\\one/two/three\\four/five\\six, got %s\n", dest);

    /* try simple forward slashes with same dir */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/.\\two");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\one/.\\two"), "Expected C:\\one/.\\two, got %s\n", dest);

    /* try simple forward slashes with change dir */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/.\\two\\..");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\one/.") ||
       !lstrcmpA(dest, "C:\\one/"), /* Vista */
       "Expected \"C:\\one/.\" or \"C:\\one/\", got \"%s\"\n", dest);

    /* try relative forward slashes */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "../../one/two/");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "../../one/two/"), "Expected ../../one/two/, got %s\n", dest);

    /* try relative forward slashes */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "../../one/two/\\*");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "../../one/two/\\*"), "Expected ../../one/two/\\*, got %s\n", dest);

    /* try forward slashes with change dirs
     * NOTE: if there is a forward slash in between two backslashes,
     * everything in between the two backslashes is considered on dir
     */
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, "C:\\one/.\\..\\two/three\\..\\four/.five");
    ok(res, "Expected success\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    ok(!lstrcmpA(dest, "C:\\four/.five"), "Expected C:\\four/.five, got %s\n", dest);

    /* try src is too long */
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    lstrcpyA(dest, "test");
    SetLastError(0xdeadbeef);
    res = PathCanonicalizeA(dest, too_long);
    ok(!res ||
       broken(res), /* Win95, some W2K and XP-SP1 */
       "Expected failure\n");
    todo_wine
    {
        ok(GetLastError() == 0xdeadbeef || GetLastError() == ERROR_FILENAME_EXCED_RANGE /* Vista */,
        "Expected 0xdeadbeef or ERROR_FILENAME_EXCED_RANGE, got %ld\n", GetLastError());
    }
    ok(lstrlenA(too_long) == LONG_LEN - 1, "Expected length LONG_LEN - 1, got %i\n", lstrlenA(too_long));
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
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try an empty path */
    path[0] = '\0';
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path, "Expected ext == path, got %p\n", ext);
    ok(!ext[0], "Expected length 0, got %i\n", lstrlenA(ext));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a path without an extension */
    lstrcpyA(path, "file");
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path + lstrlenA(path), "Expected ext == path, got %p\n", ext);
    ok(!ext[0], "Expected length 0, got %i\n", lstrlenA(ext));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a path with an extension */
    lstrcpyA(path, "file.txt");
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path + lstrlenA("file"),
       "Expected ext == path + lstrlenA(\"file\"), got %p\n", ext);
    ok(!lstrcmpA(ext, ".txt"), "Expected .txt, got %s\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a path with two extensions */
    lstrcpyA(path, "file.txt.doc");
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(path);
    ok(ext == path + lstrlenA("file.txt"),
       "Expected ext == path + lstrlenA(\"file.txt\"), got %p\n", ext);
    ok(!lstrcmpA(ext, ".doc"), "Expected .txt, got %s\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a path longer than MAX_PATH without an extension*/
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(too_long);
    ok(ext == too_long + LONG_LEN - 1, "Expected ext == too_long + LONG_LEN - 1, got %p\n", ext);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try a path longer than MAX_PATH with an extension*/
    memset(too_long, 'a', LONG_LEN);
    too_long[LONG_LEN - 1] = '\0';
    lstrcpyA(too_long + 300, ".abcde");
    too_long[lstrlenA(too_long)] = 'a';
    SetLastError(0xdeadbeef);
    ext = PathFindExtensionA(too_long);
    ok(ext == too_long + 300, "Expected ext == too_long + 300, got %p\n", ext);
    ok(lstrlenA(ext) == LONG_LEN - 301, "Expected LONG_LEN - 301, got %i\n", lstrlenA(ext));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
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
        lstrcpyA(path, "aaaaaaaaa");
        root = PathBuildRootA(path, j);
        ok(root == path, "Expected root == path, got %p\n", root);
        ok(!lstrcmpA(root, root_expected[j]), "Expected %s, got %s\n", root_expected[j], root);
        ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
    }

    /* test a negative drive number */
    SetLastError(0xdeadbeef);
    lstrcpyA(path, "aaaaaaaaa");
    root = PathBuildRootA(path, -1);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmpA(path, "aaaaaaaaa") || !path[0], /* Vista */
       "Expected aaaaaaaaa or empty string, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* test a drive number greater than 25 */
    SetLastError(0xdeadbeef);
    lstrcpyA(path, "aaaaaaaaa");
    root = PathBuildRootA(path, 26);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmpA(path, "aaaaaaaaa") || !path[0], /* Vista */
       "Expected aaaaaaaaa or empty string, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* length of path is less than 4 */
    SetLastError(0xdeadbeef);
    lstrcpyA(path, "aa");
    root = PathBuildRootA(path, 0);
    ok(root == path, "Expected root == path, got %p\n", root);
    ok(!lstrcmpA(path, "A:\\"), "Expected A:\\, got %s\n", path);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* path is NULL */
    SetLastError(0xdeadbeef);
    root = PathBuildRootA(NULL, 0);
    ok(root == NULL, "Expected root == NULL, got %p\n", root);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
}

static void test_PathCommonPrefixA(void)
{
    char path1[MAX_PATH], path2[MAX_PATH];
    char out[MAX_PATH];
    int count;

    /* test NULL path1 */
    SetLastError(0xdeadbeef);
    lstrcpyA(path2, "C:\\");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(NULL, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    todo_wine
    {
        ok(!lstrcmpA(out, "aaa"), "Expected aaa, got %s\n", out);
    }
    ok(!lstrcmpA(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* test NULL path2 */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "C:\\");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, NULL, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    todo_wine
    {
        ok(!lstrcmpA(out, "aaa"), "Expected aaa, got %s\n", out);
    }
    ok(!lstrcmpA(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* test empty path1 */
    SetLastError(0xdeadbeef);
    path1[0] = '\0';
    lstrcpyA(path2, "C:\\");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    ok(!out[0], "Expected 0 length out, got %i\n", lstrlenA(out));
    ok(!path1[0], "Expected 0 length path1, got %i\n", lstrlenA(path1));
    ok(!lstrcmpA(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* test empty path1 */
    SetLastError(0xdeadbeef);
    path2[0] = '\0';
    lstrcpyA(path1, "C:\\");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    ok(!out[0], "Expected 0 length out, got %i\n", lstrlenA(out));
    ok(!path2[0], "Expected 0 length path2, got %i\n", lstrlenA(path2));
    ok(!lstrcmpA(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* paths are legit, out is NULL */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "C:\\");
    lstrcpyA(path2, "C:\\");
    count = PathCommonPrefixA(path1, path2, NULL);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmpA(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(!lstrcmpA(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* all parameters legit */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "C:\\");
    lstrcpyA(path2, "C:\\");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmpA(path1, "C:\\"), "Expected C:\\, got %s\n", path1);
    ok(!lstrcmpA(path2, "C:\\"), "Expected C:\\, got %s\n", path2);
    ok(!lstrcmpA(out, "C:\\"), "Expected C:\\, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* path1 and path2 not the same, but common prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "C:\\one\\two");
    lstrcpyA(path2, "C:\\one\\three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 6, "Expected 6, got %i\n", count);
    ok(!lstrcmpA(path1, "C:\\one\\two"), "Expected C:\\one\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "C:\\one\\three"), "Expected C:\\one\\three, got %s\n", path2);
    ok(!lstrcmpA(out, "C:\\one"), "Expected C:\\one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try . prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\.two");
    lstrcpyA(path2, "one\\.three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\.two"), "Expected one\\.two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\.three"), "Expected one\\.three, got %s\n", path2);
    ok(!lstrcmpA(out, "one"), "Expected one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try .. prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\..two");
    lstrcpyA(path2, "one\\..three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\..two"), "Expected one\\..two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\..three"), "Expected one\\..three, got %s\n", path2);
    ok(!lstrcmpA(out, "one"), "Expected one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try ... prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\...two");
    lstrcpyA(path2, "one\\...three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 3, "Expected 3, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\...two"), "Expected one\\...two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\...three"), "Expected one\\...three, got %s\n", path2);
    ok(!lstrcmpA(out, "one"), "Expected one, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try .\ prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\.\\two");
    lstrcpyA(path2, "one\\.\\three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 5, "Expected 5, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\.\\two"), "Expected one\\.\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\.\\three"), "Expected one\\.\\three, got %s\n", path2);
    ok(!lstrcmpA(out, "one\\."), "Expected one\\., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try ..\ prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\..\\two");
    lstrcpyA(path2, "one\\..\\three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 6, "Expected 6, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\..\\two"), "Expected one\\..\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\..\\three"), "Expected one\\..\\three, got %s\n", path2);
    ok(!lstrcmpA(out, "one\\.."), "Expected one\\.., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try ...\\ prefix */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\...\\two");
    lstrcpyA(path2, "one\\...\\three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 7, "Expected 7, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\...\\two"), "Expected one\\...\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\...\\three"), "Expected one\\...\\three, got %s\n", path2);
    ok(!lstrcmpA(out, "one\\..."), "Expected one\\..., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try prefix that is not an msdn labeled prefix type */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "same");
    lstrcpyA(path2, "same");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 4, "Expected 4, got %i\n", count);
    ok(!lstrcmpA(path1, "same"), "Expected same, got %s\n", path1);
    ok(!lstrcmpA(path2, "same"), "Expected same, got %s\n", path2);
    ok(!lstrcmpA(out, "same"), "Expected same, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try . after directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\mid.\\two");
    lstrcpyA(path2, "one\\mid.\\three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 8, "Expected 8, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\mid.\\two"), "Expected one\\mid.\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\mid.\\three"), "Expected one\\mid.\\three, got %s\n", path2);
    ok(!lstrcmpA(out, "one\\mid."), "Expected one\\mid., got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try . in the middle of a directory */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\mid.end\\two");
    lstrcpyA(path2, "one\\mid.end\\three");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 11, "Expected 11, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\mid.end\\two"), "Expected one\\mid.end\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "one\\mid.end\\three"), "Expected one\\mid.end\\three, got %s\n", path2);
    ok(!lstrcmpA(out, "one\\mid.end"), "Expected one\\mid.end, got %s\n", out);
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());

    /* try comparing a .. with the expanded path */
    SetLastError(0xdeadbeef);
    lstrcpyA(path1, "one\\..\\two");
    lstrcpyA(path2, "two");
    lstrcpyA(out, "aaa");
    count = PathCommonPrefixA(path1, path2, out);
    ok(count == 0, "Expected 0, got %i\n", count);
    ok(!lstrcmpA(path1, "one\\..\\two"), "Expected one\\..\\two, got %s\n", path1);
    ok(!lstrcmpA(path2, "two"), "Expected two, got %s\n", path2);
    ok(!out[0], "Expected 0 length out, got %i\n", lstrlenA(out));
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got %ld\n", GetLastError());
}

static void test_PathUnquoteSpaces(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(TEST_PATH_UNQUOTE_SPACES); i++)
    {
        char *path = strdup(TEST_PATH_UNQUOTE_SPACES[i].path);
        WCHAR *pathW = GetWideString(TEST_PATH_UNQUOTE_SPACES[i].path);
        WCHAR *resultW = GetWideString(TEST_PATH_UNQUOTE_SPACES[i].result);

        PathUnquoteSpacesA(path);
        ok(!strcmp(path, TEST_PATH_UNQUOTE_SPACES[i].result), "%s (A): got %s expected %s\n",
           TEST_PATH_UNQUOTE_SPACES[i].path, path,
           TEST_PATH_UNQUOTE_SPACES[i].result);

        PathUnquoteSpacesW(pathW);
        ok(!lstrcmpW(pathW, resultW), "%s (W): strings differ\n",
           TEST_PATH_UNQUOTE_SPACES[i].path);
        free(pathW);
        free(resultW);
        free(path);
    }
}

static void test_PathGetDriveNumber(void)
{
    static const CHAR test1A[] = "a:\\test.file";
    static const CHAR test2A[] = "file:////b:\\test.file";
    static const CHAR test3A[] = "file:///c:\\test.file";
    static const CHAR test4A[] = "file:\\\\c:\\test.file";
    static const CHAR test5A[] = "\\\\?\\C:\\dir\\file.txt";
    static const WCHAR test1W[] =
        {'a',':','\\',0};
    static const WCHAR test5W[] =
        {'\\','\\','?','\\','C',':','\\','d','i','r','\\','f','i','l','e',0};
    int ret;

    SetLastError(0xdeadbeef);
    ret = PathGetDriveNumberA(NULL);
    ok(ret == -1, "got %d\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %ld\n", GetLastError());

    ret = PathGetDriveNumberA(test1A);
    ok(ret == 0, "got %d\n", ret);
    ret = PathGetDriveNumberW(test1W);
    ok(ret == 0, "got %d\n", ret);
    ret = PathGetDriveNumberA(test2A);
    ok(ret == -1, "got %d\n", ret);
    ret = PathGetDriveNumberA(test3A);
    ok(ret == -1, "got %d\n", ret);
    ret = PathGetDriveNumberA(test4A);
    ok(ret == -1, "got %d\n", ret);

    ret = PathGetDriveNumberA(test5A);
    ok(ret == -1, "got %d\n", ret);
    ret = PathGetDriveNumberW(test5W);
    ok(ret == 2 || broken(ret == -1) /* winxp */, "got = %d\n", ret);
}

static void test_PathUnExpandEnvStrings(void)
{
    static const WCHAR sysrootW[] = {'%','S','y','s','t','e','m','R','o','o','t','%',0};
    static const WCHAR sysdriveW[] = {'%','S','y','s','t','e','m','D','r','i','v','e','%',0};
    static const WCHAR nonpathW[] = {'p','a','t','h',0};
    static const WCHAR computernameW[] = {'C','O','M','P','U','T','E','R','N','A','M','E',0};
    static const char sysrootA[] = "%SystemRoot%";
    static const char sysdriveA[] = "%SystemDrive%";
    WCHAR pathW[MAX_PATH], buffW[MAX_PATH], sysdrvW[3], envvarW[30];
    char path[MAX_PATH], buff[MAX_PATH], sysdrvA[3], envvarA[30];
    BOOL ret;
    UINT len;

    if (!pPathUnExpandEnvStringsA || !pPathUnExpandEnvStringsW)
    {
        win_skip("PathUnExpandEnvStrings not available\n");
        return;
    }

    /* The value of ComputerName is not a path */
    ret = GetEnvironmentVariableA("COMPUTERNAME", envvarA, sizeof(envvarA));
    ok(ret, "got %d\n", ret);
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsA(envvarA, buff, sizeof(buff));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());

    ret = GetEnvironmentVariableW(computernameW, envvarW, ARRAY_SIZE(envvarW));
    ok(ret, "got %d\n", ret);
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsW(envvarW, buffW, ARRAY_SIZE(buffW));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());

    /* something that can't be represented with env var */
    strcpy(path, "somepath_name");
    strcpy(buff, "xx");
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsA(path, buff, sizeof(buff));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());
    ok(buff[0] == 'x', "wrong return string %s\n", buff);

    len = GetSystemDirectoryA(path, MAX_PATH);
    ok(len > 0, "failed to get sysdir\n");

    sysdrvA[0] = path[0];
    strcpy(&sysdrvA[1], ":");

    /* buffer size is not enough */
    strcpy(buff, "xx");
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsA(path, buff, 5);
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d\n", ret);
    ok(buff[0] == 'x', "wrong return string %s\n", buff);

    /* buffer size is enough to hold variable name only */
    strcpy(buff, "xx");
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsA(path, buff, sizeof(sysrootA));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());
    ok(buff[0] == 'x', "wrong return string %s\n", buff);

    /* enough size */
    buff[0] = 0;
    ret = pPathUnExpandEnvStringsA(path, buff, sizeof(buff));
    ok(ret, "got %d\n", ret);
    ok(!strncmp(buff, sysrootA, sizeof(sysrootA)-1), "wrong return string %s\n", buff);

    /* expanded value occurs multiple times */
    /* for drive C: it unexpands it like 'C:C:' -> '%SystemDrive%C:' */
    buff[0] = 0;
    strcpy(path, sysdrvA);
    strcat(path, sysdrvA);
    ret = pPathUnExpandEnvStringsA(path, buff, sizeof(buff));
    ok(ret, "got %d\n", ret);
    /* expected string */
    strcpy(path, sysdriveA);
    strcat(path, sysdrvA);
    ok(!strcmp(buff, path), "wrong unexpanded string %s, expected %s\n", buff, path);

    /* now with altered variable */
    ret = GetEnvironmentVariableA("SystemDrive", envvarA, sizeof(envvarA));
    ok(ret, "got %d\n", ret);

    ret = SetEnvironmentVariableA("SystemDrive", "WW");
    ok(ret, "got %d\n", ret);

    /* variables are not cached */
    strcpy(path, sysdrvA);
    strcat(path, sysdrvA);
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsA(path, buff, sizeof(buff));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());

    ret = SetEnvironmentVariableA("SystemDrive", envvarA);
    ok(ret, "got %d\n", ret);

    /* PathUnExpandEnvStringsW */

    /* something that can't be represented with env var */
    lstrcpyW(pathW, nonpathW);
    buffW[0] = 'x'; buffW[1] = 0;
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsW(pathW, buffW, ARRAY_SIZE(buffW));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());
    ok(buffW[0] == 'x', "wrong return string %s\n", wine_dbgstr_w(buffW));

    len = GetSystemDirectoryW(pathW, MAX_PATH);
    ok(len > 0, "failed to get sysdir\n");

    sysdrvW[0] = pathW[0];
    sysdrvW[1] = ':';
    sysdrvW[2] = 0;

    /* buffer size is not enough */
    buffW[0] = 'x'; buffW[1] = 0;
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsW(pathW, buffW, 5);
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());
    ok(buffW[0] == 'x', "wrong return string %s\n", wine_dbgstr_w(buffW));

    /* buffer size is enough to hold variable name only */
    buffW[0] = 'x'; buffW[1] = 0;
    SetLastError(0xdeadbeef);
    ret = pPathUnExpandEnvStringsW(pathW, buffW, ARRAY_SIZE(sysrootW));
    ok(!ret && GetLastError() == 0xdeadbeef, "got %d, error %ld\n", ret, GetLastError());
    ok(buffW[0] == 'x', "wrong return string %s\n", wine_dbgstr_w(buffW));

    /* enough size */
    buffW[0] = 0;
    ret = pPathUnExpandEnvStringsW(pathW, buffW, ARRAY_SIZE(buffW));
    ok(ret, "got %d\n", ret);
    ok(!memcmp(buffW, sysrootW, sizeof(sysrootW) - sizeof(WCHAR)), "wrong return string %s\n", wine_dbgstr_w(buffW));

    /* expanded value occurs multiple times */
    /* for drive C: it unexpands it like 'C:C:' -> '%SystemDrive%C:' */
    buffW[0] = 0;
    lstrcpyW(pathW, sysdrvW);
    lstrcatW(pathW, sysdrvW);
    ret = pPathUnExpandEnvStringsW(pathW, buffW, ARRAY_SIZE(buffW));
    ok(ret, "got %d\n", ret);
    /* expected string */
    lstrcpyW(pathW, sysdriveW);
    lstrcatW(pathW, sysdrvW);
    ok(!lstrcmpW(buffW, pathW), "wrong unexpanded string %s, expected %s\n", wine_dbgstr_w(buffW), wine_dbgstr_w(pathW));
}

static const struct {
    const char *path;
    BOOL expect;
} test_path_is_relative[] = {
    {NULL, TRUE},
    {"\0", TRUE},
    {"test.txt", TRUE},
    {"\\\\folder\\test.txt", FALSE},
    {"file://folder/test.txt", TRUE},
    {"C:\\test.txt", FALSE},
    {"file:///C:/test.txt", TRUE}
};

static void test_PathIsRelativeA(void)
{
    BOOL ret;
    int i, num;

    if (!pPathIsRelativeA) {
        win_skip("PathIsRelativeA not available\n");
        return;
    }

    num = ARRAY_SIZE(test_path_is_relative);
    for (i = 0; i < num; i++) {
        ret = pPathIsRelativeA(test_path_is_relative[i].path);
        ok(ret == test_path_is_relative[i].expect,
          "PathIsRelativeA(\"%s\") expects %d, got %d.\n",
          test_path_is_relative[i].path, test_path_is_relative[i].expect, ret);
    }
}

static void test_PathIsRelativeW(void)
{
    BOOL ret;
    int i, num;
    LPWSTR path;

    if (!pPathIsRelativeW) {
        win_skip("PathIsRelativeW not available\n");
        return;
    }

    num = ARRAY_SIZE(test_path_is_relative);
    for (i = 0; i < num; i++) {
        path = GetWideString(test_path_is_relative[i].path);

        ret = pPathIsRelativeW(path);
        ok(ret == test_path_is_relative[i].expect,
          "PathIsRelativeW(\"%s\") expects %d, got %d.\n",
          test_path_is_relative[i].path, test_path_is_relative[i].expect, ret);

        free(path);
    }
}

static void test_PathStripPathA(void)
{
    const char const_path[] = "test";
    char path[] = "short//path\\file.txt";

    PathStripPathA(path);
    ok(!strcmp(path, "file.txt"), "path = %s\n", path);

    /* following test should not crash */
    /* LavView 2013 depends on that behaviour */
    PathStripPathA((char*)const_path);
}

static void test_PathUndecorate(void)
{
    static const struct {
        const WCHAR *path;
        const WCHAR *expect;
    } tests[] = {
        { L"c:\\test\\a[123]",          L"c:\\test\\a" },
        { L"c:\\test\\a[123].txt",      L"c:\\test\\a.txt" },
        { L"c:\\test\\a.txt[123]",      L"c:\\test\\a.txt[123]" },
        { L"c:\\test\\a[123a].txt",     L"c:\\test\\a[123a].txt" },
        { L"c:\\test\\a[a123].txt",     L"c:\\test\\a[a123].txt" },
        { L"c:\\test\\a[12\x0660].txt", L"c:\\test\\a[12\x0660].txt" },
        { L"c:\\test\\a[12]file",       L"c:\\test\\a[12]file" },
        { L"c:\\test[123]\\a",          L"c:\\test[123]\\a" },
        { L"c:\\test\\[123]",           L"c:\\test\\[123]" },
        { L"a[123]",                    L"a" },
        { L"a[]",                       L"a" },
        { L"[123]",                     L"[123]" }
    };
    char bufa[MAX_PATH], expect[MAX_PATH];
    WCHAR buf[MAX_PATH];
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        wcscpy(buf, tests[i].path);
        PathUndecorateW(buf);
        ok(!wcscmp(buf, tests[i].expect), "PathUndecorateW returned %s, expected %s\n",
           wine_dbgstr_w(buf), wine_dbgstr_w(tests[i].expect));

        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, tests[i].path, -1, bufa, ARRAY_SIZE(bufa), "?", NULL);
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, tests[i].expect, -1, expect, ARRAY_SIZE(expect), "?", NULL);
        PathUndecorateA(bufa);
        ok(!strcmp(bufa, expect), "PathUndecorateA returned %s, expected %s\n", bufa, expect);
    }

    PathUndecorateA(NULL);
    PathUndecorateW(NULL);
}

static void test_PathRemoveBlanks(void)
{
    struct remove_blanks_test {
        const char* input;
        const char* expected;
    };
    struct remove_blanks_test tests[] = {
        {"", ""},
        {" ", ""},
        {"test", "test"},
        {" test", "test"},
        {"  test", "test"},
        {"test ", "test"},
        {"test  ", "test"},
        {" test  ", "test"},
        {"  test ", "test"}};
    char pathA[MAX_PATH];
    WCHAR pathW[MAX_PATH];
    int i, ret;
    const UINT CP_ASCII = 20127;

    PathRemoveBlanksW(NULL);
    PathRemoveBlanksA(NULL);

    for (i=0; i < ARRAY_SIZE(tests); i++)
    {
        strcpy(pathA, tests[i].input);
        PathRemoveBlanksA(pathA);
        ok(strcmp(pathA, tests[i].expected) == 0, "input string '%s', expected '%s', got '%s'\n",
            tests[i].input, tests[i].expected, pathA);

        ret = MultiByteToWideChar(CP_ASCII, MB_ERR_INVALID_CHARS, tests[i].input, -1, pathW, MAX_PATH);
        ok(ret != 0, "MultiByteToWideChar failed for '%s'\n", tests[i].input);

        PathRemoveBlanksW(pathW);

        ret = WideCharToMultiByte(CP_ASCII, 0, pathW, -1, pathA, MAX_PATH, NULL, NULL);
        ok(ret != 0, "WideCharToMultiByte failed for %s from test string '%s'\n", wine_dbgstr_w(pathW), tests[i].input);

        ok(strcmp(pathA, tests[i].expected) == 0, "input string '%s', expected '%s', got '%s'\n",
            tests[i].input, tests[i].expected, pathA);
    }
}

START_TEST(path)
{
    HMODULE hShlwapi = GetModuleHandleA("shlwapi.dll");

    /* SHCreateStreamOnFileEx was introduced in shlwapi v6.0 */
    if(!GetProcAddress(hShlwapi, "SHCreateStreamOnFileEx")){
        win_skip("Too old shlwapi version\n");
        return;
    }

    pPathCreateFromUrlA = (void*)GetProcAddress(hShlwapi, "PathCreateFromUrlA");
    pPathCreateFromUrlW = (void*)GetProcAddress(hShlwapi, "PathCreateFromUrlW");
    pPathCreateFromUrlAlloc = (void*)GetProcAddress(hShlwapi, "PathCreateFromUrlAlloc");
    pPathCombineW = (void*)GetProcAddress(hShlwapi, "PathCombineW");
    pPathIsValidCharA = (void*)GetProcAddress(hShlwapi, (LPSTR)455);
    pPathIsValidCharW = (void*)GetProcAddress(hShlwapi, (LPSTR)456);
    pPathAppendA = (void*)GetProcAddress(hShlwapi, "PathAppendA");
    pPathUnExpandEnvStringsA = (void*)GetProcAddress(hShlwapi, "PathUnExpandEnvStringsA");
    pPathUnExpandEnvStringsW = (void*)GetProcAddress(hShlwapi, "PathUnExpandEnvStringsW");
    pPathIsRelativeA = (void*)GetProcAddress(hShlwapi, "PathIsRelativeA");
    pPathIsRelativeW = (void*)GetProcAddress(hShlwapi, "PathIsRelativeW");

    test_PathSearchAndQualify();
    test_PathCreateFromUrl();
    test_PathIsUrl();

    test_PathAddBackslash();
    test_PathMakePretty();
    test_PathMatchSpec();

    test_PathIsValidCharA();
    test_PathIsValidCharW();

    test_PathCombineW();
    test_PathCombineA();
    test_PathAppendA();
    test_PathCanonicalizeA();
    test_PathFindExtensionA();
    test_PathBuildRootA();
    test_PathCommonPrefixA();
    test_PathUnquoteSpaces();
    test_PathGetDriveNumber();
    test_PathUnExpandEnvStrings();
    test_PathIsRelativeA();
    test_PathIsRelativeW();
    test_PathStripPathA();
    test_PathUndecorate();
    test_PathRemoveBlanks();
}
