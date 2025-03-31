/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for setjmp and longjmp
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2025 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>
#include <setjmp.h>
#include <assert.h>

static jmp_buf g_jmp_buf;

static INT s_check_points[18] = { 0 };

#define CHECK_POINT(number) do { \
    assert(number < _countof(s_check_points)); \
    s_check_points[number] = __LINE__; \
} while (0)

static void TEST_setjmp_simple(void)
{
    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));

    if (setjmp(g_jmp_buf) == 0)
    {
        CHECK_POINT(0);
    }
    else
    {
        CHECK_POINT(1);
    }
}

static void TEST_setjmp_return_check(void)
{
    volatile int x = 1001, value;
    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        CHECK_POINT(2);
        longjmp(g_jmp_buf, 999);
        CHECK_POINT(3);
    }
    else if (value == 999)
    {
        CHECK_POINT(4);
        ok_int(x, 1001);
    }
    else
    {
        CHECK_POINT(5);
    }
}

static void TEST_longjmp(int value)
{
    CHECK_POINT(6);
    longjmp(g_jmp_buf, value);
    CHECK_POINT(7);
}

static void TEST_setjmp_longjmp_integration(void)
{
    volatile int value;

    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        CHECK_POINT(8);
        TEST_longjmp(0xBEEFCAFE);
        CHECK_POINT(9);
    }
    else if (value == 0xBEEFCAFE)
    {
        CHECK_POINT(10);
    }
    else
    {
        CHECK_POINT(11);
    }
}

static void TEST_setjmp_zero_longjmp_check(void)
{
    volatile int value;
    volatile BOOL went_zero = FALSE;

    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        if (went_zero)
        {
            CHECK_POINT(12);
            return;
        }
        went_zero = TRUE;

        CHECK_POINT(13);

        TEST_longjmp(0); /* giving zero should go to one */

        CHECK_POINT(14);
    }
    else if (value == 1)
    {
        if (went_zero)
        {
            CHECK_POINT(15);
        }
        else
        {
            CHECK_POINT(16);
        }
    }
    else
    {
        CHECK_POINT(17);
    }
}

START_TEST(setjmp)
{
    ZeroMemory(&s_check_points, sizeof(s_check_points));

    /* FIXME: These tests are insufficiant */
    TEST_setjmp_simple();
    TEST_setjmp_return_check();
    TEST_setjmp_longjmp_integration();
    TEST_setjmp_zero_longjmp_check();

#define DO_COME(number)    ok(s_check_points[number],  "Line %d: Didn't reach\n",    s_check_points[number])
#define NEVER_COME(number) ok(!s_check_points[number], "Line %d: Wrongly reached\n", s_check_points[number])

    DO_COME(0);
    NEVER_COME(1);
    DO_COME(2);
    NEVER_COME(3);
    DO_COME(4);
    NEVER_COME(5);
    DO_COME(6);
    NEVER_COME(7);
    DO_COME(8);
    NEVER_COME(9);
    DO_COME(10);
    NEVER_COME(11);
    NEVER_COME(12);
    DO_COME(13);
    NEVER_COME(14);
    DO_COME(15);
    NEVER_COME(16);
    NEVER_COME(17);
}
