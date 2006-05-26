/*
 * Unit test suite for ntdll path functions
 *
 * Copyright 2002 Alexandre Julliard
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

#include "ntdll_test.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlUnicodeToMultiByteN)(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
static UINT (WINAPI *pRtlDetermineDosPathNameType_U)( PCWSTR path );
static ULONG (WINAPI *pRtlIsDosDeviceName_U)( PCWSTR dos_name );
static NTSTATUS (WINAPI *pRtlOemStringToUnicodeString)(UNICODE_STRING *, const STRING *, BOOLEAN );
static BOOLEAN (WINAPI *pRtlIsNameLegalDOS8Dot3)(const UNICODE_STRING*,POEM_STRING,PBOOLEAN);
static DWORD (WINAPI *pRtlGetFullPathName_U)(const WCHAR*,ULONG,WCHAR*,WCHAR**);


static void test_RtlDetermineDosPathNameType(void)
{
    struct test
    {
        const char *path;
        UINT ret;
    };

    static const struct test tests[] =
    {
        { "\\\\foo", 1 },
        { "//foo", 1 },
        { "\\/foo", 1 },
        { "/\\foo", 1 },
        { "\\\\", 1 },
        { "//", 1 },
        { "c:\\foo", 2 },
        { "c:/foo", 2 },
        { "c://foo", 2 },
        { "c:\\", 2 },
        { "c:/", 2 },
        { "c:foo", 3 },
        { "c:f\\oo", 3 },
        { "c:foo/bar", 3 },
        { "\\foo", 4 },
        { "/foo", 4 },
        { "\\", 4 },
        { "/", 4 },
        { "foo", 5 },
        { "", 5 },
        { "\0:foo", 5 },
        { "\\\\.\\foo", 6 },
        { "//./foo", 6 },
        { "/\\./foo", 6 },
        { "\\\\.foo", 1 },
        { "//.foo", 1 },
        { "\\\\.", 7 },
        { "//.", 7 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[MAX_PATH];
    UINT ret;

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlDetermineDosPathNameType_U( buffer );
        ok( ret == test->ret, "Wrong result %d/%d for %s\n", ret, test->ret, test->path );
    }
}


static void test_RtlIsDosDeviceName(void)
{
    struct test
    {
        const char *path;
        WORD pos;
        WORD len;
    };

    static const struct test tests[] =
    {
        { "\\\\.\\CON",    8, 6 },
        { "\\\\.\\con",    8, 6 },
        { "\\\\.\\CON2",   0, 0 },
        { "",              0, 0 },
        { "\\\\foo\\nul",  0, 0 },
        { "c:\\nul:",      6, 6 },
        { "c:\\nul::",     0, 0 },
        { "c:prn     ",    4, 6 },
        { "c:prn.......",  4, 6 },
        { "c:prn... ...",  4, 6 },
        { "c:NUL  ....  ", 0, 0 },
        { "c: . . .",      0, 0 },
        { "c:",            0, 0 },
        { " . . . :",      0, 0 },
        { ":",             0, 0 },
        { "c:nul. . . :",  4, 6 },
        { "c:nul . . :",   0, 0 },
        { "c:nul0",        0, 0 },
        { "c:prn:aaa",     0, 0 },
        { "c:PRN:.txt",    4, 6 },
        { "c:aux:.txt...", 4, 6 },
        { "c:prn:.txt:",   4, 6 },
        { "c:nul:aaa",     0, 0 },
        { "con:",          0, 6 },
        { "lpt1:",         0, 8 },
        { "c:com5:",       4, 8 },
        { "CoM4:",         0, 8 },
        { "lpt9:",         0, 8 },
        { "c:\\lpt0.txt",  0, 0 },
        { "c:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
          "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\\nul.txt", 1000, 6 },
        { NULL, 0 }
    };

    const struct test *test;
    WCHAR buffer[2000];
    ULONG ret;

    for (test = tests; test->path; test++)
    {
        pRtlMultiByteToUnicodeN( buffer, sizeof(buffer), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlIsDosDeviceName_U( buffer );
        ok( ret == MAKELONG( test->len, test->pos ),
            "Wrong result (%d,%d)/(%d,%d) for %s\n",
            HIWORD(ret), LOWORD(ret), test->pos, test->len, test->path );
    }
}

static void test_RtlIsNameLegalDOS8Dot3(void)
{
    struct test
    {
        const char *path;
        BOOLEAN result;
        BOOLEAN spaces;
    };

    static const struct test tests[] =
    {
        { "12345678",     TRUE,  FALSE },
        { "123 5678",     TRUE,  TRUE  },
        { "12345678.",    FALSE, 2 /*not set*/ },
        { "1234 678.",    FALSE, 2 /*not set*/ },
        { "12345678.a",   TRUE,  FALSE },
        { "12345678.a ",  FALSE, 2 /*not set*/ },
        { "12345678.a c", TRUE,  TRUE  },
        { " 2345678.a ",  FALSE, 2 /*not set*/ },
        { "1 345678.abc", TRUE,  TRUE },
        { "1      8.a c", TRUE,  TRUE },
        { "1 3 5 7 .abc", FALSE, 2 /*not set*/ },
        { "12345678.  c", TRUE,  TRUE },
        { "123456789.a",  FALSE, 2 /*not set*/ },
        { "12345.abcd",   FALSE, 2 /*not set*/ },
        { "12345.ab d",   FALSE, 2 /*not set*/ },
        { ".abc",         FALSE, 2 /*not set*/ },
        { "12.abc.d",     FALSE, 2 /*not set*/ },
        { ".",            TRUE,  FALSE },
        { "..",           TRUE,  FALSE },
        { "...",          FALSE, 2 /*not set*/ },
        { "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", FALSE, 2 /*not set*/ },
        { NULL, 0 }
    };

    const struct test *test;
    UNICODE_STRING ustr;
    OEM_STRING oem, oem_ret;
    WCHAR buffer[200];
    char buff2[12];
    BOOLEAN ret, spaces;

    ustr.MaximumLength = sizeof(buffer);
    ustr.Buffer = buffer;
    for (test = tests; test->path; test++)
    {
        char path[100];
        strcpy(path, test->path);
        oem.Buffer = path;
        oem.Length = strlen(test->path);
        oem.MaximumLength = oem.Length + 1;
        pRtlOemStringToUnicodeString( &ustr, &oem, FALSE );
        spaces = 2;
        oem_ret.Length = oem_ret.MaximumLength = sizeof(buff2);
        oem_ret.Buffer = buff2;
        ret = pRtlIsNameLegalDOS8Dot3( &ustr, &oem_ret, &spaces );
        ok( ret == test->result, "Wrong result %d/%d for '%s'\n", ret, test->result, test->path );
        ok( spaces == test->spaces, "Wrong spaces value %d/%d for '%s'\n", spaces, test->spaces, test->path );
        if (strlen(test->path) <= 12)
        {
            char str[13];
            int i;
            strcpy( str, test->path );
            for (i = 0; str[i]; i++) str[i] = toupper(str[i]);
            ok( oem_ret.Length == strlen(test->path), "Wrong length %d/%d for '%s'\n",
                oem_ret.Length, strlen(test->path), test->path );
            ok( !memcmp( oem_ret.Buffer, str, oem_ret.Length ),
                "Wrong string '%.*s'/'%s'\n", oem_ret.Length, oem_ret.Buffer, str );
        }
    }
}
static void test_RtlGetFullPathName_U(void)
{
    struct test
    {
        const char *path;
        const char *rname;
        const char *rfile;
    };

    static const struct test tests[] =
        {
            { "c:/test",                     "c:\\test",         "test"},
            { "c:/test     ",                "c:\\test",         "test"},
            { "c:/test.",                    "c:\\test",         "test"},
            { "c:/test  ....   ..   ",       "c:\\test",         "test"},
            { "c:/test/  ....   ..   ",      "c:\\test\\",       NULL},
            { "c:/test/..",                  "c:\\",             NULL},
            { "c:/test/.. ",                 "c:\\test\\",       NULL},
            { "c:/TEST",                     "c:\\test",         "test"},
            { "c:/test/file",                "c:\\test\\file",   "file"},
            { "c:/test./file",               "c:\\test\\file",   "file"},
            { "c:/test../file",              "c:\\test.\\file",  "file"},
            { "c:/test.. /file",             "c:\\test.. \\file","file"},
            { "c:/test/././file",            "c:\\test\\file",   "file"},
            { "c:/test\\.\\.\\file",         "c:\\test\\file",   "file"},
            { "c:/test/\\.\\.\\file",        "c:\\test\\file",   "file"},
            { "c:/test\\\\.\\.\\file",       "c:\\test\\file",   "file"},
            { "c:/test\\test1\\..\\.\\file", "c:\\test\\file",   "file"},
            { "c:///test\\.\\.\\file//",     "c:\\test\\file\\", NULL},
            { "c:///test\\..\\file\\..\\//", "c:\\",             NULL},
            { NULL, NULL, NULL}
        };

    const struct test *test;
    WCHAR pathbufW[2*MAX_PATH], rbufferW[MAX_PATH];
    CHAR  rbufferA[MAX_PATH], rfileA[MAX_PATH];
    ULONG ret;
    WCHAR *file_part;
    DWORD reslen;
    UINT len;

    for (test = tests; test->path; test++)
    {
        len= strlen(test->rname) * sizeof(WCHAR);
        pRtlMultiByteToUnicodeN(pathbufW , sizeof(pathbufW), NULL, test->path, strlen(test->path)+1 );
        ret = pRtlGetFullPathName_U( pathbufW,MAX_PATH, rbufferW, &file_part);
        ok( ret == len, "Wrong result %ld/%d for \"%s\"\n", ret, len, test->path );
        ok(pRtlUnicodeToMultiByteN(rbufferA,MAX_PATH,&reslen,rbufferW,(lstrlenW(rbufferW) + 1) * sizeof(WCHAR)) == STATUS_SUCCESS,
           "RtlUnicodeToMultiByteN failed\n");
        ok(lstrcmpiA(rbufferA,test->rname) == 0, "Got \"%s\" expected \"%s\"\n",rbufferA,test->rname);
        if (file_part)
        {
            ok(pRtlUnicodeToMultiByteN(rfileA,MAX_PATH,&reslen,file_part,(lstrlenW(file_part) + 1) * sizeof(WCHAR)) == STATUS_SUCCESS,
               "RtlUnicodeToMultiByteN failed\n");
            ok(test->rfile && !lstrcmpiA(rfileA,test->rfile), "Got \"%s\" expected \"%s\"\n",rfileA,test->rfile);
        }
        else
        {
            ok( !test->rfile, "Got NULL expected \"%s\"\n", test->rfile );
        }
    }

}

START_TEST(path)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlUnicodeToMultiByteN = (void *)GetProcAddress(mod,"RtlUnicodeToMultiByteN");
    pRtlDetermineDosPathNameType_U = (void *)GetProcAddress(mod,"RtlDetermineDosPathNameType_U");
    pRtlIsDosDeviceName_U = (void *)GetProcAddress(mod,"RtlIsDosDeviceName_U");
    pRtlOemStringToUnicodeString = (void *)GetProcAddress(mod,"RtlOemStringToUnicodeString");
    pRtlIsNameLegalDOS8Dot3 = (void *)GetProcAddress(mod,"RtlIsNameLegalDOS8Dot3");
    pRtlGetFullPathName_U = (void *)GetProcAddress(mod,"RtlGetFullPathName_U");
    if (pRtlDetermineDosPathNameType_U)
        test_RtlDetermineDosPathNameType();
    if (pRtlIsDosDeviceName_U)
        test_RtlIsDosDeviceName();
    if (pRtlIsNameLegalDOS8Dot3)
        test_RtlIsNameLegalDOS8Dot3();
    if (pRtlGetFullPathName_U && pRtlMultiByteToUnicodeN)
        test_RtlGetFullPathName_U();
}
