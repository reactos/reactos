/*
 * Unit test suite for ntdll env functions
 *
 * Copyright 2003 Eric Pouech
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

#include <stdio.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlQueryEnvironmentVariable_U)(PWSTR, PUNICODE_STRING, PUNICODE_STRING);
static NTSTATUS (WINAPI* pRtlQueryEnvironmentVariable)(WCHAR*, WCHAR*, SIZE_T, WCHAR*, SIZE_T, SIZE_T*);
static NTSTATUS (WINAPI *pRtlExpandEnvironmentStrings)(WCHAR*, WCHAR*, SIZE_T, WCHAR*, SIZE_T, SIZE_T*);
static NTSTATUS (WINAPI *pRtlExpandEnvironmentStrings_U)(LPWSTR, PUNICODE_STRING, PUNICODE_STRING, PULONG);
static NTSTATUS (WINAPI *pRtlCreateProcessParameters)(RTL_USER_PROCESS_PARAMETERS**,
                                                      const UNICODE_STRING*, const UNICODE_STRING*,
                                                      const UNICODE_STRING*, const UNICODE_STRING*,
                                                      PWSTR, const UNICODE_STRING*, const UNICODE_STRING*,
                                                      const UNICODE_STRING*, const UNICODE_STRING*);
static void (WINAPI *pRtlDestroyProcessParameters)(RTL_USER_PROCESS_PARAMETERS *);

static void *initial_env;

static WCHAR  small_env[] = {'f','o','o','=','t','o','t','o',0,
                             'f','o','=','t','i','t','i',0,
                             'f','o','o','o','=','t','u','t','u',0,
                             's','r','=','a','n','=','o','u','o',0,
                             'g','=','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',
                                     'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',0,
			     '=','o','O','H','=','I','I','I',0,
                             'n','u','l','=',0,
                             0};

static void testQuery(void)
{
    struct test
    {
        const char *var;
        int len;
        NTSTATUS status;
        const char *val;
        NTSTATUS alt;
    };

    static const struct test tests[] =
    {
        {"foo", 256, STATUS_SUCCESS, "toto"},
        {"FoO", 256, STATUS_SUCCESS, "toto"},
        {"foo=", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"foo ", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"foo", 1, STATUS_BUFFER_TOO_SMALL, "toto"},
        {"foo", 3, STATUS_BUFFER_TOO_SMALL, "toto"},
        {"foo", 4, STATUS_SUCCESS, "toto", STATUS_BUFFER_TOO_SMALL},
        {"foo", 5, STATUS_SUCCESS, "toto"},
        {"fooo", 256, STATUS_SUCCESS, "tutu"},
        {"f", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"g", 256, STATUS_SUCCESS, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},
        {"sr=an", 256, STATUS_SUCCESS, "ouo", STATUS_VARIABLE_NOT_FOUND},
        {"sr", 256, STATUS_SUCCESS, "an=ouo"},
	{"=oOH", 256, STATUS_SUCCESS, "III"},
        {"", 256, STATUS_VARIABLE_NOT_FOUND, NULL},
        {"nul", 256, STATUS_SUCCESS, ""},
        {NULL, 0, 0, NULL}
    };

    WCHAR               bn[257];
    WCHAR               bv[257];
    UNICODE_STRING      name;
    UNICODE_STRING      value;
    NTSTATUS            nts;
    SIZE_T              name_length;
    SIZE_T              value_length;
    SIZE_T              return_length;
    unsigned int i;

    for (i = 0; tests[i].var; i++)
    {
        const struct test *test = &tests[i];
        name.Length = strlen(test->var) * 2;
        name.MaximumLength = name.Length + 2;
        name.Buffer = bn;
        value.Length = 0;
        value.MaximumLength = test->len * 2;
        value.Buffer = bv;
        bv[test->len] = '@';

        pRtlMultiByteToUnicodeN( bn, sizeof(bn), NULL, test->var, strlen(test->var)+1 );
        nts = pRtlQueryEnvironmentVariable_U(small_env, &name, &value);
        ok( nts == test->status || (test->alt && nts == test->alt),
            "[%d]: Wrong status for '%s', expecting %lx got %lx\n",
            i, test->var, test->status, nts );
        if (nts == test->status) switch (nts)
        {
        case STATUS_SUCCESS:
            pRtlMultiByteToUnicodeN( bn, sizeof(bn), NULL, test->val, strlen(test->val)+1 );
            ok( value.Length == strlen(test->val) * sizeof(WCHAR), "Wrong length %d for %s\n",
                value.Length, test->var );
            ok((value.Length == strlen(test->val) * sizeof(WCHAR) && memcmp(bv, bn, value.Length) == 0) ||
	       lstrcmpW(bv, bn) == 0, 
	       "Wrong result for %s/%d\n", test->var, test->len);
            ok(bv[test->len] == '@', "Writing too far away in the buffer for %s/%d\n", test->var, test->len);
            break;
        case STATUS_BUFFER_TOO_SMALL:
            ok( value.Length == strlen(test->val) * sizeof(WCHAR), 
                "Wrong returned length %d (too small buffer) for %s\n", value.Length, test->var );
            break;
        }
    }

    if (pRtlQueryEnvironmentVariable)
    {
        for (i = 0; tests[i].var; i++)
        {
            const struct test* test = &tests[i];
            name_length = strlen(test->var);
            value_length = test->len;
            value.Buffer = bv;
            bv[test->len] = '@';

            pRtlMultiByteToUnicodeN(bn, sizeof(bn), NULL, test->var, strlen(test->var) + 1);
            nts = pRtlQueryEnvironmentVariable(small_env, bn, name_length, bv, value_length, &return_length);
            ok(nts == test->status || (test->alt && nts == test->alt),
                "[%d]: Wrong status for '%s', expecting %lx got %lx\n",
                i, test->var, test->status, nts);
            if (nts == test->status) switch (nts)
            {
            case STATUS_SUCCESS:
                pRtlMultiByteToUnicodeN(bn, sizeof(bn), NULL, test->val, strlen(test->val) + 1);
                ok(return_length == strlen(test->val), "Wrong length %Id for %s\n",
                    return_length, test->var);
                ok(!memcmp(bv, bn, return_length), "Wrong result for %s/%d\n", test->var, test->len);
                ok(bv[test->len] == '@', "Writing too far away in the buffer for %s/%d\n", test->var, test->len);
                break;
            case STATUS_BUFFER_TOO_SMALL:
                ok(return_length == (strlen(test->val) + 1),
                    "Wrong returned length %Id (too small buffer) for %s\n", return_length, test->var);
                break;
            }
        }
    }
    else win_skip("RtlQueryEnvironmentVariable not available, skipping tests\n");
}

static void testExpand(void)
{
    static const struct test
    {
        const char *src;
        const char *dst;
    } tests[] =
    {
        {"hello%foo%world",             "hellototoworld"},
        {"hello%=oOH%world",            "helloIIIworld"},
        {"hello%foo",                   "hello%foo"},
        {"hello%bar%world",             "hello%bar%world"},
        /*
         * {"hello%foo%world%=oOH%eeck",   "hellototoworldIIIeeck"},
         * Interestingly enough, with a 8 WCHAR buffers, we get on 2k:
         *      helloIII
         * so it seems like strings overflowing the buffer are written 
         * (truncated) but the write cursor is not advanced :-/
         */
        {NULL, NULL}
    };

    const struct test*  test;
    NTSTATUS            nts;
    UNICODE_STRING      us_src, us_dst;
    WCHAR               src[256], dst[256], rst[256];
    ULONG               ul;

    for (test = tests; test->src; test++)
    {
        pRtlMultiByteToUnicodeN(src, sizeof(src), NULL, test->src, strlen(test->src)+1);
        pRtlMultiByteToUnicodeN(rst, sizeof(rst), NULL, test->dst, strlen(test->dst)+1);

        us_src.Length = strlen(test->src) * sizeof(WCHAR);
        us_src.MaximumLength = us_src.Length + 2;
        us_src.Buffer = src;

        us_dst.Length = 0;
        us_dst.MaximumLength = 0;
        us_dst.Buffer = NULL;

        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_BUFFER_TOO_SMALL, "Call failed (%lu)\n", nts);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s: %lu\n", test->src, ul );

        us_dst.Length = 0;
        us_dst.MaximumLength = sizeof(dst);
        us_dst.Buffer = dst;

        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_SUCCESS, "Call failed (%lu)\n", nts);
        ok(ul == us_dst.Length + sizeof(WCHAR), 
           "Wrong returned length for %s: %lu\n", test->src, ul);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s: %lu\n", test->src, ul);
        ok(lstrcmpW(dst, rst) == 0, "Wrong result for %s: expecting %s\n",
           test->src, test->dst);

        us_dst.Length = 0;
        us_dst.MaximumLength = 8 * sizeof(WCHAR);
        us_dst.Buffer = dst;
        dst[8] = '-';
        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_BUFFER_TOO_SMALL, "Call failed (%lu)\n", nts);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s (with buffer too small): %lu\n", test->src, ul);
        ok(dst[8] == '-', "Writing too far in buffer (got %c/%d)\n", dst[8], dst[8]);
    }

}

static void test_RtlExpandEnvironmentStrings(void)
{
    int i;
    WCHAR buf[256];
    HRESULT status;
    UNICODE_STRING us_src, us_dst, us_name, us_value;
    static const struct test_info
    {
        const WCHAR *input;
        const WCHAR *expected_str;
        int count_in;
        int expected_count_out;
    } tests[] =
    {
        /*  0 */ { L"Long long value",       L"abcdefghijklmnopqrstuv",  0, 16 },
        /*  1 */ { L"Long long value",       L"abcdefghijklmnopqrstuv",  1, 16 },
        /*  2 */ { L"Long long value",       L"Lbcdefghijklmnopqrstuv",  2, 16 },
        /*  3 */ { L"Long long value",       L"Locdefghijklmnopqrstuv",  3, 16 },
        /*  4 */ { L"Long long value",       L"Long long valuopqrstuv", 15, 16 },
        /*  5 */ { L"Long long value",       L"Long long value",        16, 16 },
        /*  6 */ { L"Long long value",       L"Long long value",        17, 16 },
        /*  7 */ { L"%TVAR% long long",      L"abcdefghijklmnopqrstuv",  0, 15 },
        /*  8 */ { L"%TVAR% long long",      L"",                        1, 15 },
        /*  9 */ { L"%TVAR% long long",      L"",                        2, 15 },
        /* 10 */ { L"%TVAR% long long",      L"",                        4, 15 },
        /* 11 */ { L"%TVAR% long long",      L"WINE",                    5, 15 },
        /* 12 */ { L"%TVAR% long long",      L"WINE fghijklmnopqrstuv",  6, 15 },
        /* 13 */ { L"%TVAR% long long",      L"WINE lghijklmnopqrstuv",  7, 15 },
        /* 14 */ { L"%TVAR% long long",      L"WINE long long",         15, 15 },
        /* 15 */ { L"%TVAR% long long",      L"WINE long long",         16, 15 },
        /* 16 */ { L"%TVAR%%TVAR% long",     L"",                        4, 14 },
        /* 17 */ { L"%TVAR%%TVAR% long",     L"WINE",                    5, 14 },
        /* 18 */ { L"%TVAR%%TVAR% long",     L"WINE",                    6, 14 },
        /* 19 */ { L"%TVAR%%TVAR% long",     L"WINE",                    8, 14 },
        /* 20 */ { L"%TVAR%%TVAR% long",     L"WINEWINE",                9, 14 },
        /* 21 */ { L"%TVAR%%TVAR% long",     L"WINEWINE jklmnopqrstuv", 10, 14 },
        /* 22 */ { L"%TVAR%%TVAR% long",     L"WINEWINE long",          14, 14 },
        /* 23 */ { L"%TVAR%%TVAR% long",     L"WINEWINE long",          15, 14 },
        /* 24 */ { L"%TVAR% %TVAR% long",    L"WINE",                    5, 15 },
        /* 25 */ { L"%TVAR% %TVAR% long",    L"WINE ",                   6, 15 },
        /* 26 */ { L"%TVAR% %TVAR% long",    L"WINE ",                   8, 15 },
        /* 27 */ { L"%TVAR% %TVAR% long",    L"WINE ",                   9, 15 },
        /* 28 */ { L"%TVAR% %TVAR% long",    L"WINE WINE",              10, 15 },
        /* 29 */ { L"%TVAR% %TVAR% long",    L"WINE WINE klmnopqrstuv", 11, 15 },
        /* 30 */ { L"%TVAR% %TVAR% long",    L"WINE WINE llmnopqrstuv", 12, 15 },
        /* 31 */ { L"%TVAR% %TVAR% long",    L"WINE WINE lonnopqrstuv", 14, 15 },
        /* 32 */ { L"%TVAR% %TVAR% long",    L"WINE WINE long",         15, 15 },
        /* 33 */ { L"%TVAR% %TVAR% long",    L"WINE WINE long",         16, 15 },
        /* 34 */ { L"%TVAR2% long long",     L"abcdefghijklmnopqrstuv",  1, 18 },
        /* 35 */ { L"%TVAR2% long long",     L"%bcdefghijklmnopqrstuv",  2, 18 },
        /* 36 */ { L"%TVAR2% long long",     L"%TVdefghijklmnopqrstuv",  4, 18 },
        /* 37 */ { L"%TVAR2% long long",     L"%TVAR2ghijklmnopqrstuv",  7, 18 },
        /* 38 */ { L"%TVAR2% long long",     L"%TVAR2%hijklmnopqrstuv",  8, 18 },
        /* 39 */ { L"%TVAR2% long long",     L"%TVAR2% ijklmnopqrstuv",  9, 18 },
        /* 40 */ { L"%TVAR2% long long",     L"%TVAR2% ljklmnopqrstuv", 10, 18 },
        /* 41 */ { L"%TVAR2% long long",     L"%TVAR2% long long",      18, 18 },
        /* 42 */ { L"%TVAR2% long long",     L"%TVAR2% long long",      19, 18 },
        /* 43 */ { L"%TVAR long long",       L"abcdefghijklmnopqrstuv",  1, 16 },
        /* 44 */ { L"%TVAR long long",       L"%bcdefghijklmnopqrstuv",  2, 16 },
        /* 45 */ { L"%TVAR long long",       L"%Tcdefghijklmnopqrstuv",  3, 16 },
        /* 46 */ { L"%TVAR long long",       L"%TVAR long lonopqrstuv", 15, 16 },
        /* 47 */ { L"%TVAR long long",       L"%TVAR long long",        16, 16 },
        /* 48 */ { L"%TVAR long long",       L"%TVAR long long",        17, 16 },
    };

    RtlInitUnicodeString(&us_name, L"TVAR");
    RtlInitUnicodeString(&us_value, L"WINE");
    status = RtlSetEnvironmentVariable(NULL, &us_name, &us_value);
    ok(status == STATUS_SUCCESS, "RtlSetEnvironmentVariable failed with %lx\n", status);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const struct test_info *test = &tests[i];
        SIZE_T out_len;
        HRESULT expected_status = test->count_in >= test->expected_count_out ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;

        wcscpy(buf, L"abcdefghijklmnopqrstuv");
        status = pRtlExpandEnvironmentStrings(NULL, (WCHAR*)test->input, wcslen(test->input), buf, test->count_in, &out_len);
        ok(out_len == test->expected_count_out, "Test %d: got %Iu\n", i, out_len);
        ok(status == expected_status, "Test %d: Expected status %lx, got %lx\n", i, expected_status, status);
        ok(!wcscmp(buf, test->expected_str), "Test %d: got %s\n", i, debugstr_w(buf));
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        const struct test_info *test = &tests[i];
        DWORD out_len;
        HRESULT expected_status = test->count_in >= test->expected_count_out ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL;

        us_src.Length = wcslen(test->input) * sizeof(WCHAR);
        us_src.MaximumLength = us_src.Length;
        us_src.Buffer = (WCHAR*)test->input;

        us_dst.Length = test->count_in * sizeof(WCHAR);
        us_dst.MaximumLength = us_dst.Length;
        us_dst.Buffer = buf;

        wcscpy(buf, L"abcdefghijklmnopqrstuv");
        status = pRtlExpandEnvironmentStrings_U(NULL, &us_src, &us_dst, &out_len);
        ok(out_len / sizeof(WCHAR) == test->expected_count_out, "Test %d: got %lu\n", i, out_len);
        ok(status == expected_status, "Test %d: Expected status %lx, got %lx\n", i, expected_status, status);
        ok(!wcscmp(buf, test->expected_str), "Test %d: got %s\n", i, debugstr_w(buf));
    }
    status = RtlSetEnvironmentVariable(NULL, &us_name, NULL);
    ok(status == STATUS_SUCCESS, "RtlSetEnvironmentVariable failed with %lx\n", status);
}

static WCHAR *get_params_string( RTL_USER_PROCESS_PARAMETERS *params, UNICODE_STRING *str )
{
    if (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED) return str->Buffer;
    return (WCHAR *)((char *)params + (UINT_PTR)str->Buffer);
}

static SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += wcslen(end) + 1;
    return end + 1 - env;
}

static UINT_PTR align(UINT_PTR size, unsigned int alignment)
{
    return (size + (alignment - 1)) & ~(alignment - 1);
}

static UINT_PTR check_string_( int line, RTL_USER_PROCESS_PARAMETERS *params, UNICODE_STRING *str,
                               const UNICODE_STRING *expect, UINT_PTR pos )
{
    if (expect)
    {
        ok_(__FILE__,line)( str->Length == expect->Length, "wrong length %u/%u\n",
                            str->Length, expect->Length );
        ok_(__FILE__,line)( str->MaximumLength == expect->MaximumLength,
                            "wrong maxlength %u/%u\n", str->MaximumLength, expect->MaximumLength );
    }
    if (!str->MaximumLength)
    {
        ok_(__FILE__,line)( str->Buffer == NULL, "buffer not null %p\n", str->Buffer );
        return pos;
    }
    if (expect)
        ok_(__FILE__,line)( (UINT_PTR)str->Buffer == align(pos, sizeof(void *)) ||
                            broken( (UINT_PTR)str->Buffer == align(pos, 4) ), /* win7 */
                            "wrong buffer %Ix/%Ix\n", (UINT_PTR)str->Buffer, pos );
    else  /* initial params are not aligned */
        ok_(__FILE__,line)( (UINT_PTR)str->Buffer == pos,
                            "wrong buffer %Ix/%Ix\n", (UINT_PTR)str->Buffer, pos );
    if (str->Length < str->MaximumLength)
    {
        WCHAR *ptr = get_params_string( params, str );
        ok_(__FILE__,line)( !ptr[str->Length / sizeof(WCHAR)], "string not null-terminated %s\n",
                            wine_dbgstr_wn( ptr, str->MaximumLength / sizeof(WCHAR) ));
    }
    return (UINT_PTR)str->Buffer + str->MaximumLength;
}
#define check_string(params,str,expect,pos) check_string_(__LINE__,params,str,expect,pos)

static void test_process_params(void)
{
    static WCHAR empty[] = {0};
    static const UNICODE_STRING empty_str = { 0, sizeof(empty), empty };
    static const UNICODE_STRING null_str = { 0, 0, NULL };
    static WCHAR exeW[] = {'c',':','\\','f','o','o','.','e','x','e',0};
    static WCHAR dummyW[] = {'d','u','m','m','y','1',0};
    static WCHAR dummy_dirW[MAX_PATH] = {'d','u','m','m','y','2',0};
    static WCHAR dummy_env[] = {'a','=','b',0,'c','=','d',0,0};
    UNICODE_STRING image = { sizeof(exeW) - sizeof(WCHAR), sizeof(exeW), exeW };
    UNICODE_STRING dummy = { sizeof(dummyW) - sizeof(WCHAR), sizeof(dummyW), dummyW };
    UNICODE_STRING dummy_dir = { 6*sizeof(WCHAR), sizeof(dummy_dirW), dummy_dirW };
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    RTL_USER_PROCESS_PARAMETERS *cur_params = NtCurrentTeb()->Peb->ProcessParameters;
    SIZE_T size;
    WCHAR *str;
    UINT_PTR pos;
    NTSTATUS status = pRtlCreateProcessParameters( &params, &image, NULL, NULL, NULL, NULL,
                                                   NULL, NULL, NULL, NULL );
    ok( !status, "failed %lx\n", status );
    size = HeapSize( GetProcessHeap(), 0, params );
    ok( size != ~(SIZE_T)0, "not a heap block %p\n", params );
    ok( params->AllocationSize == params->Size,
        "wrong AllocationSize %lx/%lx\n", params->AllocationSize, params->Size );
    ok( params->Size < size, "wrong Size %lx/%Ix\n", params->Size, size );
    ok( params->Flags == 0, "wrong Flags %lu\n", params->Flags );
    ok( params->DebugFlags == 0, "wrong Flags %lu\n", params->DebugFlags );
    ok( params->ConsoleHandle == 0, "wrong ConsoleHandle %p\n", params->ConsoleHandle );
    ok( params->ConsoleFlags == 0, "wrong ConsoleFlags %lu\n", params->ConsoleFlags );
    ok( params->hStdInput == 0, "wrong hStdInput %p\n", params->hStdInput );
    ok( params->hStdOutput == 0, "wrong hStdOutput %p\n", params->hStdOutput );
    ok( params->hStdError == 0, "wrong hStdError %p\n", params->hStdError );
    ok( params->dwX == 0, "wrong dwX %lu\n", params->dwX );
    ok( params->dwY == 0, "wrong dwY %lu\n", params->dwY );
    ok( params->dwXSize == 0, "wrong dwXSize %lu\n", params->dwXSize );
    ok( params->dwYSize == 0, "wrong dwYSize %lu\n", params->dwYSize );
    ok( params->dwXCountChars == 0, "wrong dwXCountChars %lu\n", params->dwXCountChars );
    ok( params->dwYCountChars == 0, "wrong dwYCountChars %lu\n", params->dwYCountChars );
    ok( params->dwFillAttribute == 0, "wrong dwFillAttribute %lu\n", params->dwFillAttribute );
    ok( params->dwFlags == 0, "wrong dwFlags %lu\n", params->dwFlags );
    ok( params->wShowWindow == 0, "wrong wShowWindow %lu\n", params->wShowWindow );
    pos = (UINT_PTR)params->CurrentDirectory.DosPath.Buffer;

    ok( params->CurrentDirectory.DosPath.MaximumLength == MAX_PATH * sizeof(WCHAR),
        "wrong length %x\n", params->CurrentDirectory.DosPath.MaximumLength );
    pos = check_string( params, &params->CurrentDirectory.DosPath,
                        &cur_params->CurrentDirectory.DosPath, pos );
    if (params->DllPath.MaximumLength)
        pos = check_string( params, &params->DllPath, &cur_params->DllPath, pos );
    else
        pos = check_string( params, &params->DllPath, &null_str, pos );
    pos = check_string( params, &params->ImagePathName, &image, pos );
    pos = check_string( params, &params->CommandLine, &image, pos );
    pos = check_string( params, &params->WindowTitle, &empty_str, pos );
    pos = check_string( params, &params->Desktop, &empty_str, pos );
    pos = check_string( params, &params->ShellInfo, &empty_str, pos );
    pos = check_string( params, &params->RuntimeInfo, &null_str, pos );
    pos = align(pos, 4);
    ok( pos == params->Size || pos + 4 == params->Size,
        "wrong pos %Ix/%lx\n", pos, params->Size );
    pos = params->Size;
    ok( (char *)params->Environment - (char *)params == (UINT_PTR)pos,
        "wrong env %Ix/%Ix\n", (UINT_PTR)((char *)params->Environment - (char *)params), pos);
    pos += get_env_length(params->Environment) * sizeof(WCHAR);
    ok( align(pos, sizeof(void *)) == size ||
        broken( align(pos, 4) == size ), "wrong size %Ix/%Ix\n", pos, size );
    ok( params->EnvironmentSize == size - ((char *)params->Environment - (char *)params),
        "wrong len %Ix/%Ix\n", params->EnvironmentSize,
        size - ((char *)params->Environment - (char *)params) );
    pRtlDestroyProcessParameters( params );

    status = pRtlCreateProcessParameters( &params, &image, &dummy, &dummy, &dummy, dummy_env,
                                          &dummy, &dummy, &dummy, &dummy );
    ok( !status, "failed %lx\n", status );
    size = HeapSize( GetProcessHeap(), 0, params );
    ok( size != ~(SIZE_T)0, "not a heap block %p\n", params );
    ok( params->AllocationSize == params->Size,
        "wrong AllocationSize %lx/%lx\n", params->AllocationSize, params->Size );
    ok( params->Size < size, "wrong Size %lx/%Ix\n", params->Size, size );
    pos = (UINT_PTR)params->CurrentDirectory.DosPath.Buffer;

    if (params->CurrentDirectory.DosPath.Length == dummy_dir.Length + sizeof(WCHAR))
    {
        /* win10 appends a backslash */
        dummy_dirW[dummy_dir.Length / sizeof(WCHAR)] = '\\';
        dummy_dir.Length += sizeof(WCHAR);
    }
    pos = check_string( params, &params->CurrentDirectory.DosPath, &dummy_dir, pos );
    pos = check_string( params, &params->DllPath, &dummy, pos );
    pos = check_string( params, &params->ImagePathName, &image, pos );
    pos = check_string( params, &params->CommandLine, &dummy, pos );
    pos = check_string( params, &params->WindowTitle, &dummy, pos );
    pos = check_string( params, &params->Desktop, &dummy, pos );
    pos = check_string( params, &params->ShellInfo, &dummy, pos );
    pos = check_string( params, &params->RuntimeInfo, &dummy, pos );
    pos = align(pos, 4);
    ok( pos == params->Size || pos + 4 == params->Size,
        "wrong pos %Ix/%lx\n", pos, params->Size );
    pos = params->Size;
    ok( (char *)params->Environment - (char *)params == pos,
        "wrong env %Ix/%Ix\n", (UINT_PTR)((char *)params->Environment - (char *)params), pos);
    pos += get_env_length(params->Environment) * sizeof(WCHAR);
    ok( align(pos, sizeof(void *)) == size ||
        broken( align(pos, 4) == size ), "wrong size %Ix/%Ix\n", pos, size );
    ok( params->EnvironmentSize == size - ((char *)params->Environment - (char *)params),
        "wrong len %Ix/%Ix\n", params->EnvironmentSize,
        size - ((char *)params->Environment - (char *)params) );
    pRtlDestroyProcessParameters( params );

    /* also test the actual parameters of the current process */

    ok( cur_params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED, "current params not normalized\n" );
    size = HeapSize( GetProcessHeap(), 0, cur_params );
    ok( size != ~(SIZE_T)0, "not a heap block %p\n", cur_params );
    ok( cur_params->AllocationSize == cur_params->Size,
        "wrong AllocationSize %lx/%lx\n", cur_params->AllocationSize, cur_params->Size );
    ok( cur_params->Size == size, "wrong Size %lx/%Ix\n", cur_params->Size, size );

    /* CurrentDirectory points outside the params, and DllPath may be null */
    pos = (UINT_PTR)cur_params->DllPath.Buffer;
    if (!pos) pos = (UINT_PTR)cur_params->ImagePathName.Buffer;
    pos = check_string( cur_params, &cur_params->DllPath, NULL, pos );
    pos = check_string( cur_params, &cur_params->ImagePathName, NULL, pos );
    pos = check_string( cur_params, &cur_params->CommandLine, NULL, pos );
    pos = check_string( cur_params, &cur_params->WindowTitle, NULL, pos );
    pos = check_string( cur_params, &cur_params->Desktop, NULL, pos );
    pos = check_string( cur_params, &cur_params->ShellInfo, NULL, pos );
    pos = check_string( cur_params, &cur_params->RuntimeInfo, NULL, pos );
    /* environment may follow */
    str = (WCHAR *)pos;
    if (pos - (UINT_PTR)cur_params < cur_params->Size) str += get_env_length(str);
    ok( (char *)str == (char *)cur_params + cur_params->Size,
        "wrong end ptr %p/%p\n", str, (char *)cur_params + cur_params->Size );

    /* initial environment is a separate block */

    ok( (char *)initial_env < (char *)cur_params || (char *)initial_env >= (char *)cur_params + size,
        "initial environment inside block %p / %p\n", cur_params, initial_env );
    size = HeapSize( GetProcessHeap(), 0, initial_env );
    ok( size != ~(SIZE_T)0, "env is not a heap block %p / %p\n", cur_params, initial_env );
    ok( cur_params->EnvironmentSize == size,
        "wrong len %Ix/%Ix\n", cur_params->EnvironmentSize, size );
}

static NTSTATUS set_env_var(WCHAR **env, const WCHAR *var, const WCHAR *value)
{
    UNICODE_STRING var_string, value_string;
    RtlInitUnicodeString(&var_string, var);
    if (value) RtlInitUnicodeString(&value_string, value);
    return RtlSetEnvironmentVariable(env, &var_string, value ? &value_string : NULL);
}

static void check_env_var_(int line, const char *var, const char *value)
{
    char buffer[20];
    DWORD size = GetEnvironmentVariableA(var, buffer, sizeof(buffer));
    if (value)
    {
        ok_(__FILE__, line)(size == strlen(value), "wrong size %lu\n", size);
        ok_(__FILE__, line)(!strcmp(buffer, value), "wrong value %s\n", debugstr_a(buffer));
    }
    else
    {
        ok_(__FILE__, line)(!size, "wrong size %lu\n", size);
        ok_(__FILE__, line)(GetLastError() == ERROR_ENVVAR_NOT_FOUND, "got error %lu\n", GetLastError());
    }
}
#define check_env_var(a, b) check_env_var_(__LINE__, a, b)

static void test_RtlSetCurrentEnvironment(void)
{
    NTSTATUS status;
    WCHAR *old_env, *env, *prev;
    BOOL ret;
    SIZE_T size;

    status = RtlCreateEnvironment(FALSE, &env);
    ok(!status, "got %#lx\n", status);

    ret = SetEnvironmentVariableA("testenv1", "heis");
    ok(ret, "got error %lu\n", GetLastError());
    ret = SetEnvironmentVariableA("testenv2", "dyo");
    ok(ret, "got error %lu\n", GetLastError());

    status = set_env_var(&env, L"testenv1", L"unus");
    ok(!status, "got %#lx\n", status);
    status = set_env_var(&env, L"testenv3", L"tres");
    ok(!status, "got %#lx\n", status);

    old_env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == get_env_length(old_env) * sizeof(WCHAR),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == HeapSize( GetProcessHeap(), 0, old_env ),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);

    RtlSetCurrentEnvironment(env, &prev);
    ok(prev == old_env, "got wrong previous env %p\n", prev);
    ok(NtCurrentTeb()->Peb->ProcessParameters->Environment == env, "got wrong current env\n");
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == get_env_length(env) * sizeof(WCHAR),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == HeapSize( GetProcessHeap(), 0, env ),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);

    check_env_var("testenv1", "unus");
    check_env_var("testenv2", NULL);
    check_env_var("testenv3", "tres");
    check_env_var("PATH", NULL);

    env = HeapReAlloc( GetProcessHeap(), 0, env, HeapSize( GetProcessHeap(), 0, env) + 120 );
    RtlSetCurrentEnvironment(env, &prev);
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == HeapSize( GetProcessHeap(), 0, env ),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);

    RtlSetCurrentEnvironment(old_env, NULL);
    ok(NtCurrentTeb()->Peb->ProcessParameters->Environment == old_env, "got wrong current env\n");
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == get_env_length(old_env) * sizeof(WCHAR),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);
    ok(NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == HeapSize( GetProcessHeap(), 0, old_env ),
       "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize);

    check_env_var("testenv1", "heis");
    check_env_var("testenv2", "dyo");
    check_env_var("testenv3", NULL);

    env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    size = get_env_length(env) * sizeof(WCHAR);
    ok( NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == size,
        "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize );
    ok( size == HeapSize( GetProcessHeap(), 0, env ),
        "got wrong size %Iu / %Iu\n", size, HeapSize( GetProcessHeap(), 0, env ));

    SetEnvironmentVariableA("testenv1", NULL);
    SetEnvironmentVariableA("testenv2", NULL);

    env = NtCurrentTeb()->Peb->ProcessParameters->Environment;
    ok( NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize == size,
        "got wrong size %Iu\n", NtCurrentTeb()->Peb->ProcessParameters->EnvironmentSize );
    ok( size == HeapSize( GetProcessHeap(), 0, env ),
        "got wrong size %Iu / %Iu\n", size, HeapSize( GetProcessHeap(), 0, env ));
    ok( size > get_env_length(env) * sizeof(WCHAR), "got wrong size %Iu\n", size );
}

static void query_env_var_(int line, WCHAR *env, const WCHAR *var, const WCHAR *value)
{
    UNICODE_STRING var_string, value_string;
    WCHAR value_buffer[9];
    NTSTATUS status;

    RtlInitUnicodeString(&var_string, var);
    value_string.Buffer = value_buffer;
    value_string.MaximumLength = sizeof(value_buffer);

    status = RtlQueryEnvironmentVariable_U(env, &var_string, &value_string);
    if (value)
    {
        ok_(__FILE__, line)(!status, "got %#lx\n", status);
        ok_(__FILE__, line)(value_string.Length/sizeof(WCHAR) == wcslen(value),
            "wrong size %Iu\n", value_string.Length/sizeof(WCHAR));
        ok_(__FILE__, line)(!wcscmp(value_string.Buffer, value), "wrong value %s\n", debugstr_w(value_string.Buffer));
    }
    else
        ok_(__FILE__, line)(status == STATUS_VARIABLE_NOT_FOUND, "got %#lx\n", status);
}
#define query_env_var(a, b, c) query_env_var_(__LINE__, a, b, c)

static void test_RtlSetEnvironmentVariable(void)
{
    NTSTATUS status;
    WCHAR *env;

    status = RtlCreateEnvironment(FALSE, &env);
    ok(!status, "got %#lx\n", status);

    status = set_env_var(&env, L"cat", L"dog");
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"cat", L"dog");

    status = set_env_var(&env, L"cat", L"horse");
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"cat", L"horse");

    status = set_env_var(&env, L"cat", NULL);
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"cat", NULL);

    status = set_env_var(&env, L"cat", NULL);
    ok(!status, "got %#lx\n", status);

    status = set_env_var(&env, L"foo", L"meouw");
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"foo", L"meouw");

    status = set_env_var(&env, L"fOo", NULL);
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"foo", NULL);

    status = set_env_var(&env, L"horse", NULL);
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"horse", NULL);

    status = set_env_var(&env, L"me=too", L"also");
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);

    status = set_env_var(&env, L"me", L"too=also");
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"me", L"too=also");

    status = set_env_var(&env, L"=too", L"also");
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"=too", L"also");

    status = set_env_var(&env, L"=", L"also");
    ok(!status, "got %#lx\n", status);
    query_env_var(env, L"=", L"also");

    status = RtlDestroyEnvironment(env);
    ok(!status, "got %#lx\n", status);
}

START_TEST(env)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");

    initial_env = NtCurrentTeb()->Peb->ProcessParameters->Environment;

    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlQueryEnvironmentVariable_U = (void*)GetProcAddress(mod, "RtlQueryEnvironmentVariable_U");
    pRtlQueryEnvironmentVariable = (void*)GetProcAddress(mod, "RtlQueryEnvironmentVariable");
    pRtlExpandEnvironmentStrings = (void*)GetProcAddress(mod, "RtlExpandEnvironmentStrings");
    pRtlExpandEnvironmentStrings_U = (void*)GetProcAddress(mod, "RtlExpandEnvironmentStrings_U");
    pRtlCreateProcessParameters = (void*)GetProcAddress(mod, "RtlCreateProcessParameters");
    pRtlDestroyProcessParameters = (void*)GetProcAddress(mod, "RtlDestroyProcessParameters");

    testQuery();
    testExpand();
    test_process_params();
    test_RtlSetCurrentEnvironment();
    test_RtlSetEnvironmentVariable();
    test_RtlExpandEnvironmentStrings();
}
