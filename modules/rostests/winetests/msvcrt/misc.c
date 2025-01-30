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
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <math.h>
#include <process.h>

static inline BOOL almost_equal(double d1, double d2) {
    if(d1-d2>-1e-30 && d1-d2<1e-30)
        return TRUE;
    return FALSE;
}

/* MS' "long double" is an 80 bit FP that takes 12 bytes*/
struct uld { ULONG lo, hi, exp; };

static int (__cdecl *prand_s)(unsigned int *);
static int (__cdecl *pI10_OUTPUT)(struct uld, int, int, void*);
static int (__cdecl *pstrerror_s)(char *, size_t, int);
static int (__cdecl *p_get_doserrno)(int *);
static int (__cdecl *p_get_errno)(int *);
static int (__cdecl *p_set_doserrno)(int);
static int (__cdecl *p_set_errno)(int);
static void (__cdecl *p__invalid_parameter)(const wchar_t*,
        const wchar_t*, const wchar_t*, unsigned int, uintptr_t);
static void (__cdecl *p_qsort_s)(void*, size_t, size_t,
        int (__cdecl*)(void*, const void*, const void*), void*);
static double (__cdecl *p_atan)(double);
static double (__cdecl *p_exp)(double);
static double (__cdecl *p_tanh)(double);
static void *(__cdecl *p_lfind_s)(const void*, const void*, unsigned int*,
        size_t, int (__cdecl *)(void*, const void*, const void*), void*);

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
    p__invalid_parameter = (void *)GetProcAddress(hmod, "_invalid_parameter");
    p_qsort_s = (void *)GetProcAddress(hmod, "qsort_s");
    p_atan = (void *)GetProcAddress(hmod, "atan");
    p_exp = (void *)GetProcAddress(hmod, "exp");
    p_tanh = (void *)GetProcAddress(hmod, "tanh");
    p_lfind_s = (void *)GetProcAddress(hmod, "_lfind_s");
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
    struct uld d;
    int size;
    int flags;

    I10_OUTPUT_data out;
    int ret;
    const char *remain;
} I10_OUTPUT_test;

static const I10_OUTPUT_test I10_OUTPUT_tests[] = {
    /* arg3 = 0 */
    { { 0x00000000, 0x00000000, 0x0000 /* 0.0 */ }, 10, 0, {0, ' ', 1, "0"}, 1, "" },
    { { 0x00000000, 0x80000000, 0x3fff /* 1.0 */ }, 10, 0, {1, ' ', 1, "1"}, 1, "000000009" },
    { { 0x00000000, 0x80000000, 0xbfff /* -1.0 */ }, 10, 0, {1, '-', 1, "1"}, 1, "000000009" },
    { { 0x0a3d7000, 0x9d70a3d7, 0x3fff /* 1.23 */ }, 10, 0, {1, ' ', 3, "123"}, 1, "0000009" },
    { { 0x00000000, 0x9184e72a, 0x402a /* 1e13 */ }, 10, 0, {14, ' ', 1, "1"}, 1, "000000009" },
    { { 0x04675000, 0xc9f2c9cd, 0x4062 /* 1e30 */ }, 30, 0, {31, ' ', 21, "100000000000000001988"}, 1, "" },
    { { 0x4bb41000, 0xe12e1342, 0x3fd3 /* 1e-13 */ }, 10, 0, {-12, ' ', 1, "1"}, 1, "000000000" },
    { { 0x00000000, 0x80000000, 0x3ffd /* 0.25 */ }, 10, 0, {0, ' ', 2, "25"}, 1, "00000000" },
    { { 0xbf94d800, 0x800000d6, 0x3fff /* 1.0000001 */ }, 10, 0, {1, ' ', 8, "10000001"}, 1, "00" },
    { { 0x00000000, 0x80000000, 0x7fff /* +inf */ }, 10, 0, {1, ' ', 5, "1#INF"}, 0, "" },
    { { 0x00000000, 0x80000000, 0xffff /* -inf */ }, 10, 0, {1, '-', 5, "1#INF"}, 0, "" },
    { { 0x00000001, 0x80000000, 0x7fff /* snan */ }, 10, 0, {1, ' ', 6, "1#SNAN"}, 0, "" },
    { { 0x00000001, 0x80000000, 0xffff /* snan */ }, 10, 0, {1, '-', 6, "1#SNAN"}, 0, "" },
    { { 0x00000000, 0xc0000000, 0x7fff /* qnan */ }, 10, 0, {1, ' ', 6, "1#QNAN"}, 0, "" },
    { { 0x00000000, 0x40000000, 0xffff /* qnan */ }, 10, 0, {1, '-', 6, "1#QNAN"}, 0, "" },
    /* arg3 = 1 */
    { { 0x00000000, 0x00000000, 0x0000 /* 0 */ }, 10, 1, {0, ' ', 1, "0"}, 1, "" },
    { { 0x00000000, 0x80000000, 0x3fff /* 1 */ }, 10, 1, {1, ' ', 1, "1"}, 1, "0000000009" },
    { { 0x00000000, 0x80000000, 0xbfff /* -1 */ }, 10, 1, {1, '-', 1, "1"}, 1, "0000000009" },
    { { 0x0a3d7000, 0x9d70a3d7, 0x3fff /* 1.23 */ }, 10, 1, {1, ' ', 3, "123"}, 1, "00000009" },
    { { 0x00000000, 0x9184e72a, 0x402a /* 1e13 */ }, 10, 1, {14, ' ', 1, "1"}, 1, "00000000000000000009" },
    { { 0x04675000, 0xc9f2c9cd, 0x4062 /* 1e30 */ }, 30, 1, {31, ' ', 21, "100000000000000001988"}, 1, "" },
    { { 0x4bb41000, 0xe12e1342, 0x3fd3 /* 1e-13 */ }, 10, 1, {0, ' ', 1, "0"}, 1, "" },
    { { 0xe57a4000, 0xd6bf94d5, 0x3fe7 /* 1e-7 */ }, 10, 1, {-6, ' ', 1, "1"}, 1, "09" },
    { { 0x00000000, 0x80000000, 0x3ffd /* 0.25 */ }, 10, 1, {0, ' ', 2, "25"}, 1, "00000000" },
    { { 0xbf94d800, 0x800000d6, 0x3fff /* 1.0000001 */ }, 10, 1, {1, ' ', 8, "10000001"}, 1, "000" },
    { { 0x00000000, 0x80000000, 0x7fff /* +inf */ }, 10, 1, {1, ' ', 5, "1#INF"}, 0, "" },
    { { 0x00000000, 0x80000000, 0xffff /* -inf */ }, 10, 1, {1, '-', 5, "1#INF"}, 0, "" },
    { { 0x00000001, 0x80000000, 0x7fff /* snan */ }, 10, 1, {1, ' ', 6, "1#SNAN"}, 0, "" },
    { { 0x00000000, 0xc0000000, 0x7fff /* qnan */ }, 10, 1, {1, ' ', 6, "1#QNAN"}, 0, "" },
    { { 0x00000000, 0x40000000, 0x7fff /* qnan */ }, 10, 1, {1, ' ', 6, "1#QNAN"}, 0, "" },
    /* too small buffer */
    { { 0x00000000, 0x00000000, 0x0000 /* 0 */ }, 0, 0, {0, ' ', 1, "0"}, 1, "" },
    { { 0x00000000, 0x00000000, 0x0000 /* 0 */ }, 0, 1, {0, ' ', 1, "0"}, 1, "" },
    { { 0x00000000, 0xf6000000, 0x4005 /* 123 */ }, 2, 0, {3, ' ', 2, "12"}, 1, "" },
    { { 0x00000000, 0xf6000000, 0x4005 /* 123 */ }, 0, 0, {0, ' ', 1, "0"}, 1, "" },
    { { 0x00000000, 0xf6000000, 0x4005 /* 123 */ }, 2, 1, {3, ' ', 3, "123"}, 1, "09" },
    { { 0x0a3d7000, 0xfd70a3d7, 0x3ffe /* 0.99 */ }, 1, 0, {1, ' ', 1, "1"}, 1, "" },
    { { 0x00000000, 0x9a5db800, 0x4013 /* 1264567.0 */ }, 2, 0, {7, ' ', 2, "13"}, 1, "" },
    { { 0x00000000, 0x9a5db800, 0x4013 /* 1264567.0 */ }, 2, 1, {7, ' ', 7, "1264567"}, 1, "00" },
    { { 0x00000000, 0x932c05a6, 0x401d /* 1234567891.0 */ }, 2, 1, {10, ' ', 10, "1234567891"}, 1, "09" }
};

static void test_I10_OUTPUT(void)
{
    I10_OUTPUT_data out;
    int i, j, ret;

    if(!pI10_OUTPUT) {
        win_skip("I10_OUTPUT not available\n");
        return;
    }

    for(i=0; i<ARRAY_SIZE(I10_OUTPUT_tests); i++) {
        memset(out.str, '#', sizeof(out.str));
        ret = pI10_OUTPUT(I10_OUTPUT_tests[i].d, I10_OUTPUT_tests[i].size, I10_OUTPUT_tests[i].flags, &out);
        ok(ret == I10_OUTPUT_tests[i].ret, "%d: ret = %d\n", i, ret);
        ok(out.pos == I10_OUTPUT_tests[i].out.pos, "%d: out.pos = %hd\n", i, out.pos);
        ok(out.sign == I10_OUTPUT_tests[i].out.sign, "%d: out.size = %c\n", i, out.sign);
        ok(out.len == I10_OUTPUT_tests[i].out.len, "%d: out.len = %d\n", i, (int)out.len);
        ok(!strcmp(out.str, I10_OUTPUT_tests[i].out.str), "%d: out.str = %s\n", i, out.str);

        j = strlen(I10_OUTPUT_tests[i].remain);
        todo_wine_if(j && I10_OUTPUT_tests[i].remain[j-1]=='9')
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
    ok(buf[0] == 0, "Expected output buffer to be null terminated\n");

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
    ok(_doserrno == ERROR_INVALID_CMM, "Expected _doserrno to be ERROR_INVALID_CMM, got %ld\n", _doserrno);
    ok(errno == EBADF, "Expected errno to be EBADF, got %d\n", errno);

    _doserrno = ERROR_INVALID_CMM;
    errno = EBADF;
    out = 0xdeadbeef;
    ret = p_get_doserrno(&out);
    ok(ret == 0, "Expected _get_doserrno to return 0, got %d\n", ret);
    ok(out == ERROR_INVALID_CMM, "Expected output variable to be ERROR_INVALID_CMM, got %d\n", out);
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
       "Expected _doserrno to be ERROR_FILE_NOT_FOUND, got %ld\n", _doserrno);

    _doserrno = ERROR_INVALID_CMM;
    ret = p_set_doserrno(-1);
    ok(ret == 0, "Expected _set_doserrno to return 0, got %d\n", ret);
    ok(_doserrno == -1,
       "Expected _doserrno to be -1, got %ld\n", _doserrno);

    _doserrno = ERROR_INVALID_CMM;
    ret = p_set_doserrno(0xdeadbeef);
    ok(ret == 0, "Expected _set_doserrno to return 0, got %d\n", ret);
    ok(_doserrno == 0xdeadbeef,
       "Expected _doserrno to be 0xdeadbeef, got %ld\n", _doserrno);
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

static void test__popen_child(int fd)
{
    /* don't execute any tests here */
    /* ExitProcess is used to set return code of _pclose */
    printf("child output\n");
    if ((HANDLE)_get_osfhandle(fd) != INVALID_HANDLE_VALUE)
        ExitProcess(1);
    ExitProcess(0x37);
}

static void test__popen_read_child(void)
{
    char buf[1024], *rets;

    rets = fgets(buf, sizeof(buf), stdin);
    if (strcmp(buf, "child-to-parent\n") != 0)
        ExitProcess(1);

    rets = fgets(buf, sizeof(buf), stdin);
    if (rets)
        ExitProcess(2);
    ExitProcess(3);
}

static void test__popen(const char *name)
{
    FILE *pipe;
    char *tempf, buf[1024];
    int ret, fd;

    tempf = _tempnam(".", "wne");
    ok(tempf != NULL, "_tempnam failed\n");
    fd = _open(tempf, _O_CREAT | _O_WRONLY);
    ok(fd != -1, "open failed\n");

    sprintf(buf, "\"%s\" misc popen %d", name, fd);
    pipe = _popen(buf, "r");
    ok(pipe != NULL, "_popen failed with error: %d\n", errno);

    fgets(buf, sizeof(buf), pipe);
    ok(!strcmp(buf, "child output\n"), "buf = %s\n", buf);

    ret = _pclose(pipe);
    ok(ret == 0x37, "_pclose returned %x, expected 0x37\n", ret);
    _close(fd);
    _unlink(tempf);
    free(tempf);

    errno = 0xdeadbeef;
    ret = _pclose((FILE*)0xdeadbeef);
    ok(ret == -1, "_pclose returned %x, expected -1\n", ret);
    if(p_set_errno)
        ok(errno == EBADF, "errno = %d\n", errno);

    sprintf(buf, "\"%s\" misc popen_read", name);
    pipe = _popen(buf, "w");
    ok(pipe != NULL, "_popen failed with error: %d\n", errno);

    ret = fputs("child-to-parent\n", pipe);
    ok(ret != EOF, "fputs returned %x\n", ret);

    ret = _pclose(pipe);
    ok(ret == 0x3, "_pclose returned %x, expected 0x3\n", ret);
}

static void test__invalid_parameter(void)
{
    if(!p__invalid_parameter) {
        win_skip("_invalid_parameter not available\n");
        return;
    }

    p__invalid_parameter(NULL, NULL, NULL, 0, 0);
}

struct qsort_test
{
    int pos;
    int *base;

    struct {
        int l;
        int r;
    } cmp[64];
};

static int __cdecl qsort_comp(void *ctx, const void *l, const void *r)
{
    struct qsort_test *qt = ctx;

    if(qt) {
        ok(qt->pos < 64, "qt->pos = %d\n", qt->pos);
        ok(qt->cmp[qt->pos].l == (int*)l-qt->base,
           "%d) l on %ld position\n", qt->pos, (long)((int*)l - qt->base));
        ok(qt->cmp[qt->pos].r == (int*)r-qt->base,
           "%d) r on %ld position\n", qt->pos, (long)((int*)r - qt->base));
        qt->pos++;
    }

    return *(int*)l%1000 - *(int*)r%1000;
}

static void test_qsort_s(void)
{
    static const int nonstable_test[] = {9000, 8001, 7002, 6003, 1003, 5004, 4005, 3006, 2007};
    int tab[100], i;

    struct qsort_test small_sort = {
        0, tab, {
            {1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 5}, {7, 6},
            {1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 5},
            {1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4},
            {1, 0}, {2, 1}, {3, 2}, {4, 3},
            {1, 0}, {2, 1}, {3, 2},
            {1, 0}, {2, 1},
            {1, 0}
        }
    }, small_sort2 = {
        0, tab, {
            {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0},
            {1, 0}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1},
            {1, 0}, {2, 1}, {3, 2}, {4, 2}, {5, 2},
            {1, 0}, {2, 1}, {3, 2}, {4, 3},
            {1, 0}, {2, 1}, {3, 2},
            {1, 0}, {2, 1},
            {1, 0}
        }
    }, quick_sort = {
        0, tab, {
            {0, 4}, {0, 8}, {4, 8},
            {1, 4}, {2, 4}, {3, 4}, {5, 4}, {6, 4}, {7, 4}, {7, 4}, {6, 4},
            {6, 4},
            {8, 7},
            {1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 4},
            {1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 3},
            {1, 0}, {2, 1}, {3, 2}, {4, 2},
            {1, 0}, {2, 1}, {3, 2},
            {1, 0}, {2, 1},
            {1, 0}
        }
    };

    if(!p_qsort_s) {
        win_skip("qsort_s not available\n");
        return;
    }

    for(i=0; i<8; i++) tab[i] = i;
    p_qsort_s(tab, 8, sizeof(int), qsort_comp, &small_sort);
    ok(small_sort.pos == 28, "small_sort.pos = %d\n", small_sort.pos);
    for(i=0; i<8; i++)
        ok(tab[i] == i, "tab[%d] = %d\n", i, tab[i]);

    for(i=0; i<8; i++) tab[i] = 7-i;
    p_qsort_s(tab, 8, sizeof(int), qsort_comp, &small_sort2);
    ok(small_sort2.pos == 28, "small_sort2.pos = %d\n", small_sort2.pos);
    for(i=0; i<8; i++)
        ok(tab[i] == i, "tab[%d] = %d\n", i, tab[i]);

    for(i=0; i<9; i++) tab[i] = i;
    tab[5] = 1;
    tab[6] = 2;
    p_qsort_s(tab, 9, sizeof(int), qsort_comp, &quick_sort);
    ok(quick_sort.pos == 34, "quick_sort.pos = %d\n", quick_sort.pos);

    /* show that qsort is not stable */
    for(i=0; i<9; i++) tab[i] = 8-i + 1000*(i+1);
    tab[0] = 1003;
    p_qsort_s(tab, 9, sizeof(int), qsort_comp, NULL);
    for(i=0; i<9; i++)
        ok(tab[i] == nonstable_test[i], "tab[%d] = %d, expected %d\n", i, tab[i], nonstable_test[i]);

    /* check if random data is sorted */
    srand(0);
    for(i=0; i<100; i++) tab[i] = rand()%1000;
    p_qsort_s(tab, 100, sizeof(int), qsort_comp, NULL);
    for(i=1; i<100; i++)
        ok(tab[i-1] <= tab[i], "data sorted incorrectly on position %d: %d <= %d\n", i, tab[i-1], tab[i]);

    /* test if random permutation is sorted correctly */
    for(i=0; i<100; i++) tab[i] = i;
    for(i=0; i<100; i++) {
        int b = rand()%100;
        int e = rand()%100;

        if(b == e) continue;
        tab[b] ^= tab[e];
        tab[e] ^= tab[b];
        tab[b] ^= tab[e];
    }
    p_qsort_s(tab, 100, sizeof(int), qsort_comp, NULL);
    for(i=0; i<100; i++)
        ok(tab[i] == i, "data sorted incorrectly on position %d: %d\n", i, tab[i]);
}

static int eq_nan(UINT64 ai, double b)
{
    UINT64 bi = *(UINT64*)&b;
    UINT64 mask;

#if defined(__i386__)
    mask = 0xFFFFFFFF00000000ULL;
#else
    mask = ~0;
#endif

    ok((ai & mask) == (bi & mask), "comparing %s and %s\n",
            wine_dbgstr_longlong(ai), wine_dbgstr_longlong(bi));
    return (ai & mask) == (bi & mask);
}

static int eq_nanf(DWORD ai, float b)
{
    DWORD bi = *(DWORD*)&b;
    ok(ai == bi, "comparing %08lx and %08lx\n", ai, bi);
    return ai == bi;
}

static void test_math_functions(void)
{
    static const UINT64 test_nan_i = 0xFFF0000123456780ULL;
    static const DWORD test_nanf_i = 0xFF801234;
    double test_nan = *(double*)&test_nan_i;
    float test_nanf = *(float*)&test_nanf_i;
    double ret;

    errno = 0xdeadbeef;
    p_atan(NAN);
    ok(errno == EDOM, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ret = p_atan(INFINITY);
    ok(almost_equal(ret, 1.57079632679489661923), "ret = %lf\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ret = p_atan(-INFINITY);
    ok(almost_equal(ret, -1.57079632679489661923), "ret = %lf\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_tanh(NAN);
    ok(errno == EDOM, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    ret = p_tanh(INFINITY);
    ok(almost_equal(ret, 1.0), "ret = %lf\n", ret);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_exp(NAN);
    ok(errno == EDOM, "errno = %d\n", errno);

    errno = 0xdeadbeef;
    p_exp(INFINITY);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);

    ok(eq_nan(test_nan_i | (1ULL << 51), cosh(test_nan)), "cosh not preserving nan\n");
    ok(eq_nan(test_nan_i | (1ULL << 51), sinh(test_nan)), "sinh not preserving nan\n");
    ok(eq_nan(test_nan_i | (1ULL << 51), tanh(test_nan)), "tanh not preserving nan\n");
    ok(eq_nanf(test_nanf_i | (1 << 22), coshf(test_nanf)), "coshf not preserving nan\n");
    ok(eq_nanf(test_nanf_i | (1 << 22), sinhf(test_nanf)), "sinhf not preserving nan\n");
    ok(eq_nanf(test_nanf_i | (1 << 22), tanhf(test_nanf)), "tanhf not preserving nan\n");
}

static void __cdecl test_thread_func(void *end_thread_type)
{
    if (end_thread_type == (void*)1)
        _endthread();
    else if (end_thread_type == (void*)2)
        ExitThread(0);
    else if (end_thread_type == (void*)3)
        _endthreadex(0);
}

static unsigned __stdcall test_thread_func_ex(void *arg)
{
    _endthread();
    return 0;
}

static void test_thread_handle_close(void)
{
    HANDLE hThread;
    DWORD ret;

    /* _beginthread: handle is not closed on ExitThread and _endthreadex */
    hThread = (HANDLE)_beginthread(test_thread_func, 0, NULL);
    ok(hThread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    WaitForSingleObject(hThread, INFINITE);
    ret = CloseHandle(hThread);
    ok(!ret, "ret = %ld\n", ret);

    hThread = (HANDLE)_beginthread(test_thread_func, 0, (void*)1);
    ok(hThread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    WaitForSingleObject(hThread, INFINITE);
    ret = CloseHandle(hThread);
    ok(!ret, "ret = %ld\n", ret);

    hThread = (HANDLE)_beginthread(test_thread_func, 0, (void*)2);
    ok(hThread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    Sleep(150);
    ret = WaitForSingleObject(hThread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %ld\n", ret);
    ret = CloseHandle(hThread);
    ok(ret, "ret = %ld\n", ret);

    hThread = (HANDLE)_beginthread(test_thread_func, 0, (void*)3);
    ok(hThread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    Sleep(150);
    ret = WaitForSingleObject(hThread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %ld\n", ret);
    ret = CloseHandle(hThread);
    ok(ret, "ret = %ld\n", ret);

    /* _beginthreadex: handle is not closed on _endthread */
    hThread = (HANDLE)_beginthreadex(NULL,0, test_thread_func_ex, NULL, 0, NULL);
    ok(hThread != NULL, "_beginthreadex failed (%d)\n", errno);
    Sleep(150);
    ret = WaitForSingleObject(hThread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %ld\n", ret);
    ret = CloseHandle(hThread);
    ok(ret, "ret = %ld\n", ret);
}

static void test_thread_suspended(void)
{
    HANDLE hThread;
    DWORD ret;

    hThread = (HANDLE)_beginthreadex(NULL, 0, test_thread_func_ex, NULL, CREATE_SUSPENDED, NULL);
    ok(hThread != NULL, "_beginthreadex failed (%d)\n", errno);
    ret = ResumeThread(hThread);
    ok(ret == 1, "suspend count = %ld\n", ret);
    ret = WaitForSingleObject(hThread, 200);
    ok(ret == WAIT_OBJECT_0, "ret = %ld\n", ret);
}

static int __cdecl _lfind_s_comp(void *ctx, const void *l, const void *r)
{
    *(int *)ctx = 0xdeadc0de;
    return *(int *)l - *(int *)r;
}

static void test__lfind_s(void)
{
    static const int tests[] = {9000, 8001, 7002, 6003, 1003, 5004, 4005, 3006, 2007};
    unsigned int num;
    void *found;
    int ctx;
    int key;

    if (!p_lfind_s)
    {
        win_skip("_lfind_s is not available\n");
        return;
    }

    key = 1234;
    num = ARRAY_SIZE(tests);

    errno = 0xdeadbeef;
    found = p_lfind_s(NULL, tests, &num, sizeof(int), _lfind_s_comp, NULL);
    ok(errno == EINVAL, "errno = %d\n", errno);
    ok(!found, "Expected NULL, got %p\n", found);

    errno = 0xdeadbeef;
    found = p_lfind_s(&key, NULL, &num, sizeof(int), _lfind_s_comp, NULL);
    ok(errno == EINVAL, "errno = %d\n", errno);
    ok(!found, "Expected NULL, got %p\n", found);

    errno = 0xdeadbeef;
    found = p_lfind_s(&key, tests, &num, 0, _lfind_s_comp, NULL);
    ok(errno == EINVAL, "errno = %d\n", errno);
    ok(!found, "Expected NULL, got %p\n", found);

    errno = 0xdeadbeef;
    found = p_lfind_s(&key, tests, &num, sizeof(int), NULL, NULL);
    ok(errno == EINVAL, "errno = %d\n", errno);
    ok(!found, "Expected NULL, got %p\n", found);

    ctx = -1;
    key = 9000;
    errno = 0xdeadbeef;
    found = p_lfind_s(&key, tests, &num, sizeof(int), _lfind_s_comp, &ctx);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);
    ok(found == tests, "Expected %p, got %p\n", tests, found);
    ok(ctx == 0xdeadc0de, "Expected 0xdeadc0de, got %x\n", ctx);

    ctx = -1;
    key = 2007;
    errno = 0xdeadbeef;
    found = p_lfind_s(&key, tests, &num, sizeof(int), _lfind_s_comp, &ctx);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);
    ok(found == tests+8, "Expected %p, got %p\n", tests+8, found);
    ok(ctx == 0xdeadc0de, "Expected 0xdeadc0de, got %x\n", ctx);

    ctx = -1;
    key = 1234;
    errno = 0xdeadbeef;
    found = p_lfind_s(&key, tests, &num, sizeof(int), _lfind_s_comp, &ctx);
    ok(errno == 0xdeadbeef, "errno = %d\n", errno);
    ok(!found, "Expected NULL, got %p\n", found);
    ok(ctx == 0xdeadc0de, "Expected 0xdeadc0de, got %x\n", ctx);
}

START_TEST(misc)
{
    int arg_c;
    char** arg_v;

    init();

    arg_c = winetest_get_mainargs(&arg_v);
    if(arg_c >= 3) {
        if (!strcmp(arg_v[2], "popen_read"))
            test__popen_read_child();
        else if(arg_c == 4 && !strcmp(arg_v[2], "popen"))
            test__popen_child(atoi(arg_v[3]));
        else
            ok(0, "invalid argument '%s'\n", arg_v[2]);

        return;
    }

    test_rand_s();
    test_I10_OUTPUT();
    test_strerror_s();
    test__get_doserrno();
    test__get_errno();
    test__set_doserrno();
    test__set_errno();
    test__popen(arg_v[0]);
    test__invalid_parameter();
    test_qsort_s();
    test_math_functions();
    test_thread_handle_close();
    test_thread_suspended();
    test__lfind_s();
}
