/*
 * Unit test suite for ntdll path functions
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

#include "ntdll_test.h"

static NTSTATUS (WINAPI *pRtlMultiByteToUnicodeN)( LPWSTR dst, DWORD dstlen, LPDWORD reslen,
                                                   LPCSTR src, DWORD srclen );
static NTSTATUS (WINAPI *pRtlCreateEnvironment)(BOOLEAN, PWSTR*);
static NTSTATUS (WINAPI *pRtlDestroyEnvironment)(PWSTR);
static NTSTATUS (WINAPI *pRtlQueryEnvironmentVariable_U)(PWSTR, PUNICODE_STRING, PUNICODE_STRING);
static void     (WINAPI *pRtlSetCurrentEnvironment)(PWSTR, PWSTR*);
static NTSTATUS (WINAPI *pRtlSetEnvironmentVariable)(PWSTR*, PUNICODE_STRING, PUNICODE_STRING);
static NTSTATUS (WINAPI *pRtlExpandEnvironmentStrings_U)(LPWSTR, PUNICODE_STRING, PUNICODE_STRING, PULONG);

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
            "[%d]: Wrong status for '%s', expecting %x got %x\n",
            i, test->var, test->status, nts );
        if (nts == test->status) switch (nts)
        {
        case STATUS_SUCCESS:
            pRtlMultiByteToUnicodeN( bn, sizeof(bn), NULL, test->val, strlen(test->val)+1 );
            ok( value.Length == strlen(test->val) * sizeof(WCHAR), "Wrong length %d for %s\n",
                value.Length, test->var );
            ok((value.Length == strlen(test->val) * sizeof(WCHAR) && memcmp(bv, bn, test->len*sizeof(WCHAR)) == 0) ||
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
}

static void testSetHelper(LPWSTR* env, const char* var, const char* val, NTSTATUS ret, NTSTATUS alt)
{
    WCHAR               bvar[256], bval1[256], bval2[256];
    UNICODE_STRING      uvar;
    UNICODE_STRING      uval;
    NTSTATUS            nts;

    uvar.Length = strlen(var) * sizeof(WCHAR);
    uvar.MaximumLength = uvar.Length + sizeof(WCHAR);
    uvar.Buffer = bvar;
    pRtlMultiByteToUnicodeN( bvar, sizeof(bvar), NULL, var, strlen(var)+1 );
    if (val)
    {
        uval.Length = strlen(val) * sizeof(WCHAR);
        uval.MaximumLength = uval.Length + sizeof(WCHAR);
        uval.Buffer = bval1;
        pRtlMultiByteToUnicodeN( bval1, sizeof(bval1), NULL, val, strlen(val)+1 );
    }
    nts = pRtlSetEnvironmentVariable(env, &uvar, val ? &uval : NULL);
    ok(nts == ret || (alt && nts == alt), "Setting var %s=%s (%x/%x)\n", var, val, nts, ret);
    if (nts == STATUS_SUCCESS)
    {
        uval.Length = 0;
        uval.MaximumLength = sizeof(bval2);
        uval.Buffer = bval2;
        nts = pRtlQueryEnvironmentVariable_U(*env, &uvar, &uval);
        switch (nts)
        {
        case STATUS_SUCCESS:
            ok(lstrcmpW(bval1, bval2) == 0, "Cannot get value written to environment\n");
            break;
        case STATUS_VARIABLE_NOT_FOUND:
            ok(val == NULL ||
               broken(strchr(var,'=') != NULL), /* variable containing '=' may be set but not found again on NT4 */
               "Couldn't find variable, but didn't delete it. val = %s\n", val);
            break;
        default:
            ok(0, "Wrong ret %u for %s\n", nts, var);
            break;
        }
    }
}

static void testSet(void)
{
    LPWSTR              env;
    char                tmp[16];
    int                 i;

    ok(pRtlCreateEnvironment(FALSE, &env) == STATUS_SUCCESS, "Creating environment\n");

    testSetHelper(&env, "cat", "dog", STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", "horse", STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", "zz", STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", NULL, STATUS_SUCCESS, 0);
    testSetHelper(&env, "cat", NULL, STATUS_SUCCESS, STATUS_VARIABLE_NOT_FOUND);
    testSetHelper(&env, "foo", "meouw", STATUS_SUCCESS, 0);
    testSetHelper(&env, "me=too", "also", STATUS_SUCCESS, STATUS_INVALID_PARAMETER);
    testSetHelper(&env, "me", "too=also", STATUS_SUCCESS, 0);
    testSetHelper(&env, "=too", "also", STATUS_SUCCESS, 0);
    testSetHelper(&env, "=", "also", STATUS_SUCCESS, 0);

    for (i = 0; i < 128; i++)
    {
        sprintf(tmp, "zork%03d", i);
        testSetHelper(&env, tmp, "is alive", STATUS_SUCCESS, 0);
    }

    for (i = 0; i < 128; i++)
    {
        sprintf(tmp, "zork%03d", i);
        testSetHelper(&env, tmp, NULL, STATUS_SUCCESS, 0);
    }
    testSetHelper(&env, "fOo", NULL, STATUS_SUCCESS, 0);

    ok(pRtlDestroyEnvironment(env) == STATUS_SUCCESS, "Destroying environment\n");
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
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s: %u\n", test->src, ul );

        us_dst.Length = 0;
        us_dst.MaximumLength = sizeof(dst);
        us_dst.Buffer = dst;

        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_SUCCESS, "Call failed (%u)\n", nts);
        ok(ul == us_dst.Length + sizeof(WCHAR), 
           "Wrong returned length for %s: %u\n", test->src, ul);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s: %u\n", test->src, ul);
        ok(lstrcmpW(dst, rst) == 0, "Wrong result for %s: expecting %s\n",
           test->src, test->dst);

        us_dst.Length = 0;
        us_dst.MaximumLength = 8 * sizeof(WCHAR);
        us_dst.Buffer = dst;
        dst[8] = '-';
        nts = pRtlExpandEnvironmentStrings_U(small_env, &us_src, &us_dst, &ul);
        ok(nts == STATUS_BUFFER_TOO_SMALL, "Call failed (%u)\n", nts);
        ok(ul == strlen(test->dst) * sizeof(WCHAR) + sizeof(WCHAR), 
           "Wrong  returned length for %s (with buffer too small): %u\n", test->src, ul);
        ok(dst[8] == '-', "Writing too far in buffer (got %c/%d)\n", dst[8], dst[8]);
    }

}

START_TEST(env)
{
    HMODULE mod = GetModuleHandleA("ntdll.dll");
    if (!mod)
    {
        win_skip("Not running on NT, skipping tests\n");
        return;
    }

    pRtlMultiByteToUnicodeN = (void *)GetProcAddress(mod,"RtlMultiByteToUnicodeN");
    pRtlCreateEnvironment = (void*)GetProcAddress(mod, "RtlCreateEnvironment");
    pRtlDestroyEnvironment = (void*)GetProcAddress(mod, "RtlDestroyEnvironment");
    pRtlQueryEnvironmentVariable_U = (void*)GetProcAddress(mod, "RtlQueryEnvironmentVariable_U");
    pRtlSetCurrentEnvironment = (void*)GetProcAddress(mod, "RtlSetCurrentEnvironment");
    pRtlSetEnvironmentVariable = (void*)GetProcAddress(mod, "RtlSetEnvironmentVariable");
    pRtlExpandEnvironmentStrings_U = (void*)GetProcAddress(mod, "RtlExpandEnvironmentStrings_U");

    if (pRtlQueryEnvironmentVariable_U)
        testQuery();
    if (pRtlSetEnvironmentVariable)
        testSet();
    if (pRtlExpandEnvironmentStrings_U)
        testExpand();
}
