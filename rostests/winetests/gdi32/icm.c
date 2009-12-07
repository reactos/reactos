/*
 * Tests for ICM functions
 *
 * Copyright (C) 2005, 2008 Hans Leidekker
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "wine/test.h"

static const WCHAR displayW[] = {'D','I','S','P','L','A','Y',0};

static void test_GetICMProfileA( HDC dc )
{
    BOOL ret;
    DWORD size, error;
    char filename[MAX_PATH];

    SetLastError( 0xdeadbeef );
    ret = GetICMProfileA( NULL, NULL, NULL );
    if ( !ret && ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) )
    {
        win_skip( "GetICMProfileA is not implemented\n" );
        return;
    }
    ok( !ret, "GetICMProfileA succeeded\n" );

    ret = GetICMProfileA( dc, NULL, NULL );
    ok( !ret, "GetICMProfileA succeeded\n" );

    size = MAX_PATH;
    ret = GetICMProfileA( dc, &size, NULL );
    ok( !ret, "GetICMProfileA succeeded\n" );

    size = MAX_PATH;
    ret = GetICMProfileA( NULL, &size, filename );
    ok( !ret, "GetICMProfileA succeeded\n" );

    size = 0;
    filename[0] = 0;
    SetLastError(0xdeadbeef);
    ret = GetICMProfileA( dc, &size, filename );
    error = GetLastError();
    ok( !ret, "GetICMProfileA succeeded\n" );
    ok( size, "expected size > 0\n" );
    ok( filename[0] == 0, "Expected filename to be empty\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER ||
        error == ERROR_SUCCESS, /* Win95 */
        "got %d, expected ERROR_INSUFFICIENT_BUFFER or ERROR_SUCCESS(Win95)\n", error );

    /* Next test will crash on Win95 */
    if ( error == ERROR_INSUFFICIENT_BUFFER )
    {
        ret = GetICMProfileA( dc, NULL, filename );
        ok( !ret, "GetICMProfileA succeeded\n" );
    }

    size = MAX_PATH;
    ret = GetICMProfileA( dc, &size, filename );
    ok( ret, "GetICMProfileA failed %d\n", GetLastError() );

    trace( "%s\n", filename );
}

static void test_GetICMProfileW( HDC dc )
{
    BOOL ret;
    DWORD size, error;
    WCHAR filename[MAX_PATH];

    SetLastError( 0xdeadbeef );
    ret = GetICMProfileW( NULL, NULL, NULL );
    if ( !ret && ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) )
    {
        win_skip( "GetICMProfileW is not implemented\n" );
        return;
    }
    ok( !ret, "GetICMProfileW succeeded\n" );

    ret = GetICMProfileW( dc, NULL, NULL );
    ok( !ret, "GetICMProfileW succeeded\n" );

    if (0)
    {
        /* Vista crashes */
        size = MAX_PATH;
        ret = GetICMProfileW( dc, &size, NULL );
        ok( ret, "GetICMProfileW failed %d\n", GetLastError() );
    }

    ret = GetICMProfileW( dc, NULL, filename );
    ok( !ret, "GetICMProfileW succeeded\n" );

    size = MAX_PATH;
    ret = GetICMProfileW( NULL, &size, filename );
    ok( !ret, "GetICMProfileW succeeded\n" );

    size = 0;
    SetLastError(0xdeadbeef);
    ret = GetICMProfileW( dc, &size, filename );
    error = GetLastError();
    ok( !ret, "GetICMProfileW succeeded\n" );
    ok( size, "expected size > 0\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER, "got %d, expected ERROR_INSUFFICIENT_BUFFER\n", error );

    size = MAX_PATH;
    ret = GetICMProfileW( dc, &size, filename );
    ok( ret, "GetICMProfileW failed %d\n", GetLastError() );
}

static void test_SetICMMode( HDC dc )
{
    INT ret, knob, save;
    BOOL impl;

    SetLastError( 0xdeadbeef );
    impl = GetICMProfileA( NULL, NULL, NULL );
    if ( !impl && ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) )
    {
        win_skip( "On NT4 where SetICMMode is not implemented but this is not advertised\n" );
        return;
    }

    SetLastError( 0xdeadbeef );
    ret = SetICMMode( NULL, 0 );
    ok( !ret, "SetICMMode succeeded (%d)\n", GetLastError() );

    ret = SetICMMode( dc, -1 );
    ok( !ret, "SetICMMode succeeded (%d)\n", GetLastError() );

    save = SetICMMode( dc, ICM_QUERY );
    ok( save == ICM_ON || save == ICM_OFF, "SetICMMode failed (%d)\n", GetLastError() );

    if (save == ICM_ON) knob = ICM_OFF; else knob = ICM_ON;

    ret = SetICMMode( dc, knob );
    todo_wine ok( ret, "SetICMMode failed (%d)\n", GetLastError() );

    ret = SetICMMode( dc, ICM_QUERY );
    todo_wine ok( ret == knob, "SetICMMode failed (%d)\n", GetLastError() );

    ret = SetICMMode( dc, save );
    ok( ret, "SetICMMode failed (%d)\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    dc = CreateDCW( displayW, NULL, NULL, NULL );
    if ( !dc && ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) )
    {
        win_skip( "CreateDCW is not implemented\n" );
        return;
    }
    ok( dc != NULL, "CreateDCW failed (%d)\n", GetLastError() );

    ret = SetICMMode( dc, ICM_QUERY );
    ok( ret == ICM_OFF, "SetICMMode failed (%d)\n", GetLastError() );

    DeleteDC( dc );
}

START_TEST(icm)
{
    HDC dc = GetDC( NULL );

    test_GetICMProfileA( dc );
    test_GetICMProfileW( dc );
    test_SetICMMode( dc );

    ReleaseDC( NULL, dc );
}
