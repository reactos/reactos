/*
 * Unit test suite for signal function.
 *
 * Copyright 2009 Peter Rosin
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
#include <winbase.h>
#include <signal.h>

static int test_value = 0;

static void __cdecl sighandler(int signum)
{
    void **ret = __pxcptinfoptrs();

    ok(ret != NULL, "ret = NULL\n");
    if(signum == SIGABRT)
        ok(*ret == (void*)0xdeadbeef, "*ret = %p\n", *ret);
    else if(signum == SIGSEGV)
        ok(*ret == NULL, "*ret = %p\n", *ret);
    ++test_value;
}

static void test_signal(void)
{
    void (__cdecl *old)(int);
    int res;

    old = signal(SIGBREAK, sighandler);
    ok(old != SIG_ERR, "Failed to install signal handler for SIGBREAK\n");
    test_value = 0;
    res = raise(SIGBREAK);
    ok(res == 0, "Failed to raise SIGBREAK\n");
    ok(test_value == 1, "SIGBREAK handler not invoked\n");
}

static void test___pxcptinfoptrs(void)
{
    void **ret = __pxcptinfoptrs();
    int res;

    ok(ret != NULL, "ret == NULL\n");
    ok(*ret == NULL, "*ret != NULL\n");

    test_value = 0;

    *ret = (void*)0xdeadbeef;
    signal(SIGSEGV, sighandler);
    res = raise(SIGSEGV);
    ok(res == 0, "failed to raise SIGSEGV\n");
    ok(*ret == (void*)0xdeadbeef, "*ret = %p\n", *ret);

    signal(SIGABRT, sighandler);
    res = raise(SIGABRT);
    ok(res == 0, "failed to raise SIGBREAK\n");
    ok(*ret == (void*)0xdeadbeef, "*ret = %p\n", *ret);

    ok(test_value == 2, "test_value = %d\n", test_value);
}

START_TEST(signal)
{
    test_signal();
    test___pxcptinfoptrs();
}
