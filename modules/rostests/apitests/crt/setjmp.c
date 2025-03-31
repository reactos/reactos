/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for setjmp and longjmp
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2025 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>
#include <setjmp.h>

static jmp_buf g_jmp_buf;

static void TEST_setjmp_simple(void)
{
    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));

    if (setjmp(g_jmp_buf) == 0)
    {
        ok(TRUE, "Yes, come here\n");
    }
    else
    {
        ok(FALSE, "No, never come here\n");
    }
}

static void TEST_setjmp_return_check(void)
{
    volatile int x = 1001, y = 1002, z = 1003;
    volatile int value;

    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        ok(TRUE, "Yes, come here\n");
        longjmp(g_jmp_buf, 999);
        ok(FALSE, "No, never come here\n");
    }
    else if (value == 999)
    {
        ok(TRUE, "Yes, come here\n");
        ok_int(x, 1001);
        ok_int(y, 1002);
        ok_int(z, 1003);
    }
    else
    {
        ok(FALSE, "No, never come here\n");
        skip("\n");
    }
}

static void TEST_longjmp(int value)
{
    ok(TRUE, "Yes, come here\n");
    longjmp(g_jmp_buf, value);
    ok(FALSE, "No, never come here\n");
}

static void TEST_setjmp_longjmp_integration(void)
{
    volatile int value;

    memset(&g_jmp_buf, 0xCC, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        ok(TRUE, "Yes, come here\n");

        TEST_longjmp(0xBEEFCAFE);

        ok(FALSE, "No, never come here\n");
    }
    else if (value == 0xBEEFCAFE)
    {
        ok(TRUE, "Yes, come here\n");
    }
    else
    {
        ok(FALSE, "No, never come here\n");
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
            ok(FALSE, "No, never come here\n");
            return;
        }
        went_zero = TRUE;

        ok(TRUE, "Yes, come here\n");

        TEST_longjmp(0); /* giving zero should go to one */

        ok(FALSE, "No, never come here\n");
    }
    else if (value == 1)
    {
        ok(TRUE, "Yes, come here\n");
    }
    else
    {
        ok(FALSE, "No, never come here\n");
    }
}

START_TEST(setjmp)
{
    TEST_setjmp_simple();
    TEST_setjmp_return_check();
    TEST_setjmp_longjmp_integration();
    TEST_setjmp_zero_longjmp_check();
    /* FIXME: These tests are insufficiant */
}
