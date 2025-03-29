/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for setjmp/longjmp
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <apitest.h>
#include <setjmp.h>

static jmp_buf g_jmp_buf;

static void Test_longjmp(void)
{
    longjmp(g_jmp_buf, 1);
}

static void Test_setjmp_0(void)
{
    if (setjmp(g_jmp_buf) == 0)
    {
        ok_int(TRUE, TRUE);
    }
    else
    {
        ok_int(TRUE, FALSE);
    }
}

static void Test_setjmp_1(void)
{
    if (setjmp(g_jmp_buf) == 0)
    {
        Test_longjmp();
        ok_int(TRUE, FALSE);
    }
    else
    {
        ok_int(TRUE, TRUE);
    }
}

static void Test_setjmp_2(void)
{
    volatile int x = 2;
    volatile int y = 3;
    volatile int z = 4;
    if (setjmp(g_jmp_buf) == 0)
    {
        ok_int(TRUE, TRUE);
        Test_longjmp();
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
        ok_int(TRUE, FALSE);
    }
    else
    {
        ok_int(x, 2);
        ok_int(y, 3);
        ok_int(z, 4);
    }
}

START_TEST(setjmp)
{
    Test_setjmp_0();
    Test_setjmp_1();
    Test_setjmp_2();
}
