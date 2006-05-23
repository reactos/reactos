/*
 * Unit tests for C library environment routines
 *
 * Copyright 2004 Mike Hearn <mh@codeweavers.com>
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

#include "wine/test.h"
#include <stdlib.h>

START_TEST(environ)
{
    ok( _putenv("cat=") == 0, "_putenv failed on deletion of nonexistent environment variable\n" );
    ok( _putenv("cat=dog") == 0, "failed setting cat=dog\n" );
    ok( strcmp(getenv("cat"), "dog") == 0, "getenv did not return 'dog'\n" );
    ok( _putenv("cat=") == 0, "failed deleting cat\n" );

    ok( _putenv("=") == -1, "should not accept '=' as input\n" );
    ok( _putenv("=dog") == -1, "should not accept '=dog' as input\n" );

    ok( getenv("nonexistent") == NULL, "getenv should fail with nonexistent var name\n" );
}
