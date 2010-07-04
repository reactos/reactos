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
static int (__cdecl *pmemcpy_s)(void *, MSVCRT_size_t, void*, MSVCRT_size_t);
static int (__cdecl *pI10_OUTPUT)(long double, int, int, void*);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    prand_s = (void *)GetProcAddress(hmod, "rand_s");
    pmemcpy_s = (void*)GetProcAddress(hmod, "memcpy_s");
    pI10_OUTPUT = (void*)GetProcAddress(hmod, "$I10_OUTPUT");
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

static void test_memcpy_s(void)
{
    static char data[] = "data\0to\0be\0copied";
    static char dest[32];
    int ret;

    if(!pmemcpy_s)
    {
        win_skip("memcpy_s is not available\n");
        return;
    }

    errno = 0xdeadbeef;
    ret = pmemcpy_s(NULL, 0, NULL, 0);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    dest[0] = 'x';
    ret = pmemcpy_s(dest, 10, NULL, 0);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
    ok(dest[0] == 'x', "dest[0] != \'x\'\n");

    errno = 0xdeadbeef;
    ret = pmemcpy_s(NULL, 10, data, 10);
    ok(ret == EINVAL, "ret = %x\n", ret);
    ok(errno == EINVAL, "errno = %x\n", errno);

    errno = 0xdeadbeef;
    dest[7] = 'x';
    ret = pmemcpy_s(dest, 10, data, 5);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
    ok(memcmp(dest, data, 10), "All data copied\n");
    ok(!memcmp(dest, data, 5), "First five bytes are different\n");

    errno = 0xdeadbeef;
    ret = pmemcpy_s(data, 10, data, 10);
    ok(ret == 0, "ret = %x\n", ret);
    ok(errno == 0xdeadbeef, "errno = %x\n", errno);
    ok(!memcmp(dest, data, 5), "data was destroyed during overwriting\n");

    errno = 0xdeadbeef;
    dest[0] = 'x';
    ret = pmemcpy_s(dest, 5, data, 10);
    ok(ret == ERANGE, "ret = %x\n", ret);
    ok(errno == ERANGE, "errno = %x\n", errno);
    ok(dest[0] == '\0', "dest[0] != \'\\0\'\n");
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
    int i, j, ret;

    if(!pI10_OUTPUT) {
        win_skip("I10_OUTPUT not available\n");
        return;
    }

    for(i=0; i<sizeof(I10_OUTPUT_tests)/sizeof(I10_OUTPUT_test); i++) {
        memset(out.str, '#', sizeof(out.str));

        ret = pI10_OUTPUT(I10_OUTPUT_tests[i].d, I10_OUTPUT_tests[i].size, I10_OUTPUT_tests[i].flags, &out);
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

START_TEST(misc)
{
    init();

    test_rand_s();
    test_memcpy_s();
    test_I10_OUTPUT();
}
