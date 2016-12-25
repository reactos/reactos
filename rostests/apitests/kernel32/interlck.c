/*
 * Unit test suite for interlocked functions.
 *
 * Copyright 2006 Hervé Poussineau
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

static void test_InterlockedCompareExchange(void)
{
    LONG dest, res;

    dest = 0;
    res = InterlockedCompareExchange( &dest, 1, 0 );
    ok( res == 0 && dest == 1,
        "Expected 0 and 1, got %ld and %ld", res, dest );

    dest = 1;
    res = InterlockedCompareExchange( &dest, 2, 0 );
    ok( res == 1 && dest == 1,
        "Expected 1 and 1, got %ld and %ld", res, dest );
}

static void test_InterlockedDecrement(void)
{
    LONG dest, res;

    dest = 1;
    res = InterlockedDecrement( &dest );
    ok( res == 0 && dest == 0,
        "Expected 0 and 0, got %ld and %ld", res, dest );

    dest = 0;
    res = InterlockedDecrement( &dest );
    ok( res == -1 && dest == -1,
        "Expected -1 and -1, got %ld and %ld", res, dest );

    dest = -1;
    res = InterlockedDecrement( &dest );
    ok( res == -2 && dest == -2,
        "Expected -2 and -2, got %ld and %ld", res, dest );
}

static void test_InterlockedExchange(void)
{
    LONG dest, res;

    dest = 0;
    res = InterlockedExchange( &dest, 1 );
    ok( res == 0 && dest == 1,
        "Expected 0 and 1, got %ld and %ld", res, dest );

    dest = 1;
    res = InterlockedExchange( &dest, 2 );
    ok( res == 1 && dest == 2,
        "Expected 1 and 2, got %ld and %ld", res, dest );

    dest = 1;
    res = InterlockedExchange( &dest, -1 );
    ok( res == 1 && dest == -1,
        "Expected 1 and -1, got %ld and %ld", res, dest );
}

static void test_InterlockedExchangeAdd(void)
{
    LONG dest, res;

    dest = 0;
    res = InterlockedExchangeAdd( &dest, 1 );
    ok( res == 0 && dest == 1,
        "Expected 0 and 1, got %ld and %ld", res, dest );

    dest = 1;
    res = InterlockedExchangeAdd( &dest, 2 );
    ok( res == 1 && dest == 3,
        "Expected 1 and 3, got %ld and %ld", res, dest );

    dest = 1;
    res = InterlockedExchangeAdd( &dest, -1 );
    ok( res == 1 && dest == 0,
        "Expected 1 and 0, got %ld and %ld", res, dest );
}

static void test_InterlockedIncrement(void)
{
    LONG dest, res;

    dest = -2;
    res = InterlockedIncrement( &dest );
    ok( res == -1 && dest == -1,
        "Expected -1 and -1, got %ld and %ld", res, dest );

    dest = -1;
    res = InterlockedIncrement( &dest );
    ok( res == 0 && dest == 0,
        "Expected 0 and 0, got %ld and %ld", res, dest );

    dest = 0;
    res = InterlockedIncrement( &dest );
    ok( res == 1 && dest == 1,
        "Expected 1 and 1, got %ld and %ld", res, dest );
}

START_TEST(interlck)
{
    test_InterlockedCompareExchange();
    test_InterlockedDecrement();
    test_InterlockedExchange();
    test_InterlockedExchangeAdd();
    test_InterlockedIncrement();
}
