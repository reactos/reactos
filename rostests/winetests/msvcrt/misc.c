/*
 * Unit tests for miscellaneous msvcrt functions
 *
 * Copyright 2010 Andrew Nguyen
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

#include "wine/test.h"
#include <errno.h>
#include "msvcrt.h"

static int (__cdecl *prand_s)(unsigned int *);
static int (__cdecl *pI10_OUTPUT)(long double, int, int, void*);
static int (__cdecl *pstrerror_s)(char *, MSVCRT_size_t, int);
static int (__cdecl *p_get_doserrno)(int *);
static int (__cdecl *p_get_errno)(int *);
static int (__cdecl *p_set_doserrno)(int);
static int (__cdecl *p_set_errno)(int);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    prand_s = (void *)GetProcAddress(hmod, "rand_s");
    pI10_OUTPUT = (void*)GetProcAddress(hmod, "$I10_OUTPUT");
    pstrerror_s = (void *)GetProcAddress(hmod, "strerror_s");
    p_get_doserrno = (void *)GetProcAddress(hmod, "_get_doserrno");
    p_get_errno = (void *)GetProcAddress(hmod, "_get_errno");
    p_set_doserrno = (void *)GetProcAddress(hmod, "_set_doserrno");
    p_set_errno = (void *)GetProcAddress(hmod, "_set_errno");
}

static void test_rand_s(void)
{
    int ret;
    unsigned int rand;

    if (!prand_s)
    {
        win_skip("rand_s is not available\n");
        return;
    }

    errno = EBADF;
    ret = prand_s(NULL);
    ok(ret == EINVAL, "Expected rand_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to return EINVAL, got %d\n", errno);

    ret = prand_s(&rand);
    ok(ret == 0, "Expected rand_s to return 0, got %d\n", ret);
}

typedef struct _I10_OUTPUT_data {
    short pos;
    char sign;
    BYTE len;
    char str[100];
} I10_OUTPUT_data;

typedef struct _I10_OUTPUT_test {
    long double d;
    int size;
    int flags;

    I10_OUTPUT_data out;
    int ret;
    const char *remain;
} I10_OUTPUT_test;

static const I10_OUTPUT_test I10_OUTPUT_tests[] = {
    /* arg3 = 0 */
    { 0.0, 10, 0, {0, ' ', 1, "0"}, 1, "" },
    { 1.0, 10, 0, {1, ' ', 1, "1"}, 1, "000000009" },
    { -1.0, 10, 0, {1, '-', 1, "1"}, 1, "000000009" },
    { 1.23, 10, 0, {1, ' ', 3, "123"}, 1, "0000009" },
    { 1e13, 10, 0, {14, ' ', 1, "1"}, 1, "000000009" },
    { 1e30, 30, 0, {31, ' ', 21, "100000000000000001988"}, 1, "" },
    { 1e-13, 10, 0, {-12, ' ', 1, "1"}, 1, "000000000" },
    { 0.25, 10, 0, {0, ' ', 2, "25"}, 1, "00000000" },
    { 1.0000001, 10, 0, {1, ' ', 8, "10000001"}, 1, "00" },
    /* arg3 = 1 */
    { 0.0, 10, 1, {0, ' ', 1, "0"}, 1, "" },
    { 1.0, 10, 1, {1, ' ', 1, "1"}, 1, "0000000009" },
    { -1.0, 10, 1, {1, '-', 1, "1"}, 1, "0000000009" },
    { 1.23, 10, 1, {1, ' ', 3, "123"}, 1, "00000009" },
    { 1e13, 10, 1, {14, ' ', 1, "1"}, 1, "00000000000000000009" },
    { 1e30, 30, 1, {31, ' ', 21, "100000000000000001988"}, 1, "" },
    { 1e-13, 10, 1, {0, ' ', 1, "0"}, 1, "" },
    { 1e-7, 10, 1, {-6, ' ', 1, "1"}, 1, "09" },
    { 0.25, 10, 1, {0, ' ', 2, "25"}, 1, "00000000" },
    { 1.0000001, 10, 1, {1, ' ', 8, "10000001"}, 1, "000" },
    /* too small buffer */
    { 0.0, 0, 0, {0, ' ', 1, "0"}, 1, "" },
    { 0.0, 0, 1, {0, ' ', 1, "0"}, 1, "" },
    { 123.0, 2, 0, {3, ' ', 2, "12"}, 1, "" },
    { 123.0, 0, 0, {0, ' ', 1, "0"}, 1, "" },
    { 123.0, 2, 1, {3, ' ', 3, "123"}, 1, "09" },
    { 0.99, 1, 0, {1, ' ', 1, "1"}, 1, "" },
    { 1264567.0, 2, 0, {7, ' ', 2, "13"}, 1, "" },
    { 1264567.0, 2, 1, {7, ' ', 7, "1264567"}, 1, "00" },
    { 1234567891.0, 2, 1, {10, ' ', 10, "1234567891"}, 1, "09" }
};

static void test_I10_OUTPUT(void)
{
    I10_OUTPUT_data out;
    int i, j = sizeof(long double), ret;

    if(!pI10_OUTPUT) {
        win_skip("I10_OUTPUT not available\n");
        return;
    }
    if (j != 12)
        trace("sizeof(long double) = %d on this machine\n", j);

    for(i=0; i<sizeof(I10_OUTPUT_tests)/sizeof(I10_OUTPUT_test); i++) {
        memset(out.str, '#', sizeof(out.str));

        if (sizeof(long double) == 12)
            ret = pI10_OUTPUT(I10_OUTPUT_tests[i].d, I10_OUTPUT_tests[i].size, I10_OUTPUT_tests[i].flags, &out);
        else {
            /* MS' "long double" is an 80 bit FP that takes 12 bytes*/
            typedef struct { ULONG x80[3]; } uld; /* same calling convention */
            union { long double ld; uld ld12; } fp80;
            int (__cdecl *pI10_OUTPUT12)(uld, int, int, void*) = (void*)pI10_OUTPUT;
            fp80.ld = I10_OUTPUT_tests[i].d;
            ret = pI10_OUTPUT12(fp80.ld12, I10_OUTPUT_tests[i].size, I10_OUTPUT_tests[i].flags, &out);
        }
        ok(ret == I10_OUTPUT_tests[i].ret, "%d: ret = %d\n", i, ret);
        ok(out.pos == I10_OUTPUT_tests[i].out.pos, "%d: out.pos = %hd\n", i, out.pos);
        ok(out.sign == I10_OUTPUT_tests[i].out.sign, "%d: out.size = %c\n", i, out.sign);
        ok(out.len == I10_OUTPUT_tests[i].out.len, "%d: out.len = %d\n", i, (int)out.len);
        ok(!strcmp(out.str, I10_OUTPUT_tests[i].out.str), "%d: out.str = %s\n", i, out.str);

        j = strlen(I10_OUTPUT_tests[i].remain);
        if(j && I10_OUTPUT_tests[i].remain[j-1]=='9')
            todo_wine ok(!strncmp(out.str+out.len+1, I10_OUTPUT_tests[i].remain, j),
                    "%d: &out.str[%d] = %.25s...\n", i, out.len+1, out.str+out.len+1);
        else
            ok(!strncmp(out.str+out.len+1, I10_OUTPUT_tests[i].remain, j),
                    "%d: &out.str[%d] = %.25s...\n", i, out.len+1, out.str+out.len+1);


        for(j=out.len+strlen(I10_OUTPUT_tests[i].remain)+1; j<sizeof(out.str); j++)
            if(out.str[j] != '#')
                ok(0, "%d: out.str[%d] = %c (expected \'#\')\n", i, j, out.str[j]);
    }
}

static void test_strerror_s(void)
{
    int ret;
    char buf[256];

    if (!pstrerror_s)
    {
        win_skip("strerror_s is not available\n");
        return;
    }

    errno = EBADF;
    ret = pstrerror_s(NULL, 0, 0);
    ok(ret == EINVAL, "Expected strerror_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = pstrerror_s(NULL, sizeof(buf), 0);
    ok(ret == EINVAL, "Expected strerror_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    memset(buf, 'X', sizeof(buf));
    errno = EBADF;
    ret = pstrerror_s(buf, 0, 0);
    ok(ret == EINVAL, "Expected strerror_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);
    ok(buf[0] == 'X', "Expected output buffer to be untouched\n");

    memset(buf, 'X', sizeof(buf));
    ret = pstrerror_s(buf, 1, 0);
    ok(ret == 0, "Expected strerror_s to return 0, got %d\n", ret);
    ok(strlen(buf) == 0, "Expected output buffer to be null terminated\n");

    memset(buf, 'X', sizeof(buf));
    ret = pstrerror_s(buf, 2, 0);
    ok(ret == 0, "Expected strerror_s to return 0, got %d\n", ret);
    ok(strlen(buf) == 1, "Expected output buffer to be truncated\n");

    memset(buf, 'X', sizeof(buf));
    ret = pstrerror_s(buf, sizeof(buf), 0);
    ok(ret == 0, "Expected strerror_s to return 0, got %d\n", ret);

    memset(buf, 'X', sizeof(buf));
    ret = pstrerror_s(buf, sizeof(buf), -1);
    ok(ret == 0, "Expected strerror_s to return 0, got %d\n", ret);
}

static void test__get_doserrno(void)
{
    int ret, out;

    if (!p_get_doserrno)
    {
        win_skip("_get_doserrno is not available\n");
        return;
    }

    _doserrno = ERROR_INVALID_CMM;
    errno = EBADF;
    ret = p_get_doserrno(NULL);
    ok(ret == EINVAL, "Expected _get_doserrno to return EINVAL, got %d\n", ret);
    ok(_doserrno == ERROR_INVALID_CMM, "Expected _doserrno to be ERROR_INVALID_CMM, got %d\n", _doserrno);
    ok(errno == EBADF, "Expected errno to be EBADF, got %d\n", errno);

    _doserrno = ERROR_INVALID_CMM;
    errno = EBADF;
    out = 0xdeadbeef;
    ret = p_get_doserrno(&out);
    ok(ret == 0, "Expected _get_doserrno to return 0, got %d\n", ret);
    ok(out == ERROR_INVALID_CMM, "Expected output variable to be ERROR_INVAID_CMM, got %d\n", out);
}

static void test__get_errno(void)
{
    int ret, out;

    if (!p_get_errno)
    {
        win_skip("_get_errno is not available\n");
        return;
    }

    errno = EBADF;
    ret = p_get_errno(NULL);
    ok(ret == EINVAL, "Expected _get_errno to return EINVAL, got %d\n", ret);
    ok(errno == EBADF, "Expected errno to be EBADF, got %d\n", errno);

    errno = EBADF;
    out = 0xdeadbeef;
    ret = p_get_errno(&out);
    ok(ret == 0, "Expected _get_errno to return 0, got %d\n", ret);
    ok(out == EBADF, "Expected output variable to be EBADF, got %d\n", out);
}

static void test__set_doserrno(void)
{
    int ret;

    if (!p_set_doserrno)
    {
        win_skip("_set_doserrno is not available\n");
        return;
    }

    _doserrno = ERROR_INVALID_CMM;
    ret = p_set_doserrno(ERROR_FILE_NOT_FOUND);
    ok(ret == 0, "Expected _set_doserrno to return 0, got %d\n", ret);
    ok(_doserrno == ERROR_FILE_NOT_FOUND,
       "Expected _doserrno to be ERROR_FILE_NOT_FOUND, got %d\n", _doserrno);

    _doserrno = ERROR_INVALID_CMM;
    ret = p_set_doserrno(-1);
    ok(ret == 0, "Expected _set_doserrno to return 0, got %d\n", ret);
    ok(_doserrno == -1,
       "Expected _doserrno to be -1, got %d\n", _doserrno);

    _doserrno = ERROR_INVALID_CMM;
    ret = p_set_doserrno(0xdeadbeef);
    ok(ret == 0, "Expected _set_doserrno to return 0, got %d\n", ret);
    ok(_doserrno == 0xdeadbeef,
       "Expected _doserrno to be 0xdeadbeef, got %d\n", _doserrno);
}

static void test__set_errno(void)
{
    int ret;

    if (!p_set_errno)
    {
        win_skip("_set_errno is not available\n");
        return;
    }

    errno = EBADF;
    ret = p_set_errno(EINVAL);
    ok(ret == 0, "Expected _set_errno to return 0, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_set_errno(-1);
    ok(ret == 0, "Expected _set_errno to return 0, got %d\n", ret);
    ok(errno == -1, "Expected errno to be -1, got %d\n", errno);

    errno = EBADF;
    ret = p_set_errno(0xdeadbeef);
    ok(ret == 0, "Expected _set_errno to return 0, got %d\n", ret);
    ok(errno == 0xdeadbeef, "Expected errno to be 0xdeadbeef, got %d\n", errno);
}

START_TEST(misc)
{
    init();

    test_rand_s();
    test_I10_OUTPUT();
    test_strerror_s();
    test__get_doserrno();
    test__get_errno();
    test__set_doserrno();
    test__set_errno();
}
