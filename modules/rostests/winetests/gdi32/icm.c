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
    ok( size > 0, "got %lu\n", size );

    size = 0;
    ret = GetICMProfileA( dc, &size, NULL );
    ok( !ret, "GetICMProfileA succeeded\n" );
    ok( size > 0, "got %lu\n", size );

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
    ok( error == ERROR_INSUFFICIENT_BUFFER, "got %ld, expected ERROR_INSUFFICIENT_BUFFER\n", error );

    ret = GetICMProfileA( dc, NULL, filename );
    ok( !ret, "GetICMProfileA succeeded\n" );

    size = MAX_PATH;
    ret = GetICMProfileA( dc, &size, filename );
    ok( ret, "GetICMProfileA failed %ld\n", GetLastError() );

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
        ok( ret, "GetICMProfileW failed %ld\n", GetLastError() );
    }

    ret = GetICMProfileW( dc, NULL, filename );
    ok( !ret, "GetICMProfileW succeeded\n" );

    size = MAX_PATH;
    ret = GetICMProfileW( NULL, &size, filename );
    ok( !ret, "GetICMProfileW succeeded\n" );

    size = 0;
    ret = GetICMProfileW( dc, &size, NULL );
    ok( !ret, "GetICMProfileW succeeded\n" );
    ok( size > 0, "got %lu\n", size );

    size = 0;
    SetLastError(0xdeadbeef);
    ret = GetICMProfileW( dc, &size, filename );
    error = GetLastError();
    ok( !ret, "GetICMProfileW succeeded\n" );
    ok( size, "expected size > 0\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER, "got %ld, expected ERROR_INSUFFICIENT_BUFFER\n", error );

    size = MAX_PATH;
    ret = GetICMProfileW( dc, &size, filename );
    ok( ret, "GetICMProfileW failed %ld\n", GetLastError() );
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
    ok( !ret, "SetICMMode succeeded (%ld)\n", GetLastError() );

    ret = SetICMMode( dc, -1 );
    ok( !ret, "SetICMMode succeeded (%ld)\n", GetLastError() );

    save = SetICMMode( dc, ICM_QUERY );
    ok( save == ICM_ON || save == ICM_OFF, "SetICMMode failed (%ld)\n", GetLastError() );

    if (save == ICM_ON) knob = ICM_OFF; else knob = ICM_ON;

    ret = SetICMMode( dc, knob );
    todo_wine ok( ret, "SetICMMode failed (%ld)\n", GetLastError() );

    ret = SetICMMode( dc, ICM_QUERY );
    todo_wine ok( ret == knob, "SetICMMode failed (%ld)\n", GetLastError() );

    ret = SetICMMode( dc, save );
    ok( ret, "SetICMMode failed (%ld)\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    dc = CreateDCW( L"DISPLAY", NULL, NULL, NULL );
    if ( !dc && ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) )
    {
        win_skip( "CreateDCW is not implemented\n" );
        return;
    }
    ok( dc != NULL, "CreateDCW failed (%ld)\n", GetLastError() );

    ret = SetICMMode( dc, ICM_QUERY );
    ok( ret == ICM_OFF, "SetICMMode failed (%ld)\n", GetLastError() );

    DeleteDC( dc );
}

static INT CALLBACK enum_profiles_callbackA( LPSTR filename, LPARAM lparam )
{
    trace("%s\n", filename);
    return 1;
}

static void test_EnumICMProfilesA( HDC dc )
{
    INT ret;

    ret = EnumICMProfilesA( NULL, NULL, 0 );
    ok(ret == -1 || broken(ret == 0) /* nt4 */, "expected -1, got %d\n", ret);

    ret = EnumICMProfilesA( dc, enum_profiles_callbackA, 0 );
    ok(ret == -1 || ret == 1 || broken(ret == 0) /* nt4 */,
       "expected -1 or 1, got %d\n", ret);

    ret = EnumICMProfilesA( dc, NULL, 0 );
    ok(ret == -1 || broken(ret == 0) /* nt4 */, "expected -1, got %d\n", ret);
}

static INT CALLBACK enum_profiles_callbackW( LPWSTR filename, LPARAM lparam )
{
    return 1;
}

static void test_EnumICMProfilesW( HDC dc )
{
    INT ret;

    ret = EnumICMProfilesW( NULL, NULL, 0 );
    ok(ret == -1 || broken(ret == 0) /* NT4 */, "expected -1, got %d\n", ret);

    ret = EnumICMProfilesW( dc, NULL, 0 );
    ok(ret == -1 || broken(ret == 0) /* NT4 */, "expected -1, got %d\n", ret);

    ret = EnumICMProfilesW( dc, enum_profiles_callbackW, 0 );
    ok(ret == -1 || ret == 1 || broken(ret == 0) /* NT4 */, "expected -1 or 1, got %d\n", ret);
}

static void test_SetICMProfileA( HDC dc )
{
    BOOL ret;
    char profile[MAX_PATH];
    DWORD len, error;

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileA( NULL, NULL );
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetICMProfileA is not implemented\n");
        return;
    }

    len = sizeof(profile);
    ret = GetICMProfileA( dc, &len, profile );
    ok(ret, "GetICMProfileA failed %lu\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileA( NULL, NULL );
    error = GetLastError();
    ok(!ret, "SetICMProfileA succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileA( NULL, profile );
    error = GetLastError();
    ok(!ret, "SetICMProfileA succeeded\n");
    ok(error == ERROR_INVALID_HANDLE,
       "expected ERROR_INVALID_HANDLE, got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileA( dc, NULL );
    error = GetLastError();
    ok(!ret, "SetICMProfileA succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %lu\n", error);

    ret = SetICMProfileA( dc, profile );
    ok(ret, "SetICMProfileA failed %lu\n", GetLastError());
}

static void test_SetICMProfileW( HDC dc )
{
    BOOL ret;
    WCHAR profile[MAX_PATH];
    DWORD len, error;

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileW( NULL, NULL );
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetICMProfileW is not implemented\n");
        return;
    }

    len = ARRAY_SIZE(profile);
    ret = GetICMProfileW( dc, &len, profile );
    ok(ret, "GetICMProfileW failed %lu\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileW( NULL, NULL );
    error = GetLastError();
    ok(!ret, "SetICMProfileW succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileW( NULL, profile );
    error = GetLastError();
    ok(!ret, "SetICMProfileW succeeded\n");
    ok(error == ERROR_INVALID_HANDLE, "expected ERROR_INVALID_HANDLE, got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = SetICMProfileW( dc, NULL );
    error = GetLastError();
    ok(!ret, "SetICMProfileW succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", error);

    ret = SetICMProfileW( dc, profile );
    ok(ret, "SetICMProfileW failed %lu\n", GetLastError());
}

START_TEST(icm)
{
    HDC dc = GetDC( NULL );

    test_GetICMProfileA( dc );
    test_GetICMProfileW( dc );
    test_SetICMMode( dc );
    test_EnumICMProfilesA( dc );
    test_EnumICMProfilesW( dc );
    test_SetICMProfileA( dc );
    test_SetICMProfileW( dc );

    ReleaseDC( NULL, dc );
}
