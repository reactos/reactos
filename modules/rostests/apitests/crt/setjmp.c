/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for setjmp/longjmp
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2025 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>
#include <setjmp.h>

static jmp_buf g_jmp_buf;

static void TEST_setjmp_1(void)
{
    volatile int x = 2, y = 3, z = 4;

    memset(&g_jmp_buf, 0, sizeof(g_jmp_buf));

    if (setjmp(g_jmp_buf) == 0)
    {
        ok_int(TRUE, TRUE);
        ok_int(x, 2);
        ok_int(x, 3);
        ok_int(x, 4);
    }
    else
    {
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }
}

static void TEST_setjmp_2(void)
{
    volatile int x = 1001, y = 1002, z = 1003;
    volatile int value;

    memset(&g_jmp_buf, 0, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        ok_int(TRUE, TRUE);
        longjmp(g_jmp_buf, 999);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }
    else if (value == 999)
    {
        ok_int(x, 1001);
        ok_int(y, 1002);
        ok_int(z, 1003);
    }
    else
    {
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }
}

static void TEST_longjmp(int value)
{
    ok_int(TRUE, TRUE);
    longjmp(g_jmp_buf, 0xBEEFCAFE);
    ok_int(TRUE, FALSE);
}

static void TEST_setjmp_3(void)
{
    volatile int x = 1001, y = 1002, z = 1003;
    volatile int value;

    memset(&g_jmp_buf, 0, sizeof(g_jmp_buf));
    value = setjmp(g_jmp_buf);

    if (value == 0)
    {
        ok_int(TRUE, TRUE);

        z = 9999;
        TEST_longjmp();

        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }
    else if (value == 0xBEEFCAFE)
    {
        ok_int(x, 1001);
        ok_int(y, 1002);
        ok_int(z, 9999);
    }
    else
    {
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }
}

START_TEST(setjmp)
{
    TEST_setjmp_1();
    TEST_setjmp_2();
    TEST_setjmp_3();
}
