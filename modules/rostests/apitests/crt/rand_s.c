/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for rand_s
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <apitest.h>
#include <stdio.h>

#ifdef TEST_STATIC_CRT
errno_t __cdecl rand_s(_Out_ unsigned int* _RandomValue);
#endif

typedef int __cdecl rand_s_t(unsigned int*);
rand_s_t *p_rand_s;

void test_rand_s_performance(void)
{
    unsigned long long start, end;
    unsigned int val;
    int i;

    start = __rdtsc();
    for (i = 0; i < 10000; i++)
    {
        p_rand_s(&val);
    }
    end = __rdtsc();
    printf("rand_s took %llu cycles\n", end - start);
}

START_TEST(rand_s)
{
    unsigned int val;
    int ret;

#ifndef TEST_STATIC_CRT
    /* Dynamically load rand_s from mvcrt */
    HMODULE msvcrt = GetModuleHandleA("msvcrt");
    p_rand_s = (rand_s_t*)GetProcAddress(msvcrt, "rand_s");
    if (!p_rand_s)
    {
        win_skip("rand_s is not available\n");
        return;
    }
#else
    p_rand_s = rand_s;
#endif

    /* Test performance */
    test_rand_s_performance();

    /* Test with NULL pointer */
    ret = p_rand_s(NULL);
    ok(ret == EINVAL, "Expected EINVAL, got %d\n", ret);

    /* Test with valid pointer */
    ret = p_rand_s(&val);
    ok(ret == 0, "Expected 0, got %d\n", ret);
}
