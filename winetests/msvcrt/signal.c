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

START_TEST(signal)
{
    test_signal();
}
