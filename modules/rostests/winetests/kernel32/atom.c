/*
 * Unit tests for atom functions
 *
 * Copyright (c) 2002 Alexandre Julliard
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
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"

#define DOUBLE(x)       (WCHAR)((x<<8)|(x))

static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};
static const WCHAR FOOBARW[] = {'F','O','O','B','A','R',0};
static const WCHAR _foobarW[] = {'_','f','o','o','b','a','r',0};
static const WCHAR integfmt[] = {'#','%','d',0};

static void do_initA(char* tmp, const char* pattern, int len)
{
    const char* p = pattern;

    while (len--)
    {
        *tmp++ = *p++;
        if (!*p) p = pattern;
    }
    *tmp = '\0';
}

static void do_initW(WCHAR* tmp, const char* pattern, int len)
{
    const char* p = pattern;

    while (len--)
    {
        *tmp++ = *p++;
        if (!*p) p = pattern;
    }
    *tmp = '\0';
}

static BOOL unicode_OS;

static void test_add_atom(void)
{
    ATOM atom, w_atom;
    INT_PTR i;

    SetLastError( 0xdeadbeef );
    atom = GlobalAddAtomA( "foobar" );
    ok( atom >= 0xc000, "bad atom id %x\n", atom );
    ok( GetLastError() == 0xdeadbeef, "GlobalAddAtomA set last error\n" );

    /* Verify that it can be found (or not) appropriately */
    ok( GlobalFindAtomA( "foobar" ) == atom, "could not find atom foobar\n" );
    ok( GlobalFindAtomA( "FOOBAR" ) == atom, "could not find atom FOOBAR\n" );
    ok( !GlobalFindAtomA( "_foobar" ), "found _foobar\n" );

    /* Add the same atom, specifying string as unicode; should
     * find the first one, not add a new one */
    SetLastError( 0xdeadbeef );
    w_atom = GlobalAddAtomW( foobarW );
    if (w_atom && GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        unicode_OS = TRUE;
    else
        trace("WARNING: Unicode atom APIs are not supported on this platform\n");

    if (unicode_OS)
    {
        ok( w_atom == atom, "Unicode atom does not match ANSI one\n" );
        ok( GetLastError() == 0xdeadbeef, "GlobalAddAtomW set last error\n" );
    }

    /* Verify that it can be found (or not) appropriately via unicode name */
    if (unicode_OS)
    {
        ok( GlobalFindAtomW( foobarW ) == atom, "could not find atom foobar\n" );
        ok( GlobalFindAtomW( FOOBARW ) == atom, "could not find atom FOOBAR\n" );
        ok( !GlobalFindAtomW( _foobarW ), "found _foobar\n" );
    }

    /* Test integer atoms
     * (0x0001 .. 0xbfff) should be valid;
     * (0xc000 .. 0xffff) should be invalid */

    SetLastError( 0xdeadbeef );
    ok( GlobalAddAtomA(0) == 0 && GetLastError() == 0xdeadbeef, "succeeded to add atom 0\n" );
    if (unicode_OS)
    {
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomW(0) == 0 && GetLastError() == 0xdeadbeef, "succeeded to add atom 0\n" );
    }

    SetLastError( 0xdeadbeef );
    for (i = 1; i <= 0xbfff; i++)
    {
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomA((LPCSTR)i) == i && GetLastError() == 0xdeadbeef,
            "failed to add atom %Ix\n", i );
        if (unicode_OS)
        {
            SetLastError( 0xdeadbeef );
            ok( GlobalAddAtomW((LPCWSTR)i) == i && GetLastError() == 0xdeadbeef,
                "failed to add atom %Ix\n", i );
        }
    }

    for (i = 0xc000; i <= 0xffff; i++)
    {
        ok( !GlobalAddAtomA((LPCSTR)i), "succeeded adding %Ix\n", i );
        if (unicode_OS)
            ok( !GlobalAddAtomW((LPCWSTR)i), "succeeded adding %Ix\n", i );
    }
}

static void test_get_atom_name(void)
{
    char buf[10];
    WCHAR bufW[10];
    int i;
    UINT len;
    static const WCHAR resultW[] = {'f','o','o','b','a','r',0,'.','.','.'};
    char in[257], out[257];
    WCHAR inW[257], outW[257];

    ATOM atom = GlobalAddAtomA( "foobar" );

    /* Get the name of the atom we added above */
    memset( buf, '.', sizeof(buf) );
    len = GlobalGetAtomNameA( atom, buf, 10 );
    ok( len == strlen("foobar"), "bad length %d\n", len );
    ok( !memcmp( buf, "foobar\0...", 10 ), "bad buffer contents\n" );

    /* Repeat, unicode-style */
    if (unicode_OS)
    {
        for (i = 0; i < 10; i++) bufW[i] = '.';
        SetLastError( 0xdeadbeef );
        len = GlobalGetAtomNameW( atom, bufW, 10 );
        ok( len && GetLastError() == 0xdeadbeef, "GlobalGetAtomNameW failed\n" );
        ok( len == lstrlenW(foobarW), "bad length %d\n", len );
        ok( !memcmp( bufW, resultW, 10*sizeof(WCHAR) ), "bad buffer contents\n" );
    }

    /* Check error code returns */
    memset(buf, '.', 10);
    ok( !GlobalGetAtomNameA( atom, buf,  0 ), "succeeded\n" );
    ok( !memcmp( buf, "..........", 10 ), "should not touch buffer\n" );

    if (unicode_OS)
    {
        static const WCHAR sampleW[] = {'.','.','.','.','.','.','.','.','.','.'};

        for (i = 0; i < 10; i++) bufW[i] = '.';
        ok( !GlobalGetAtomNameW( atom, bufW, 0 ), "succeeded\n" );
        ok( !memcmp( bufW, sampleW, sizeof(sampleW) ), "should not touch buffer\n" );
    }

    /* Test integer atoms */
    for (i = 0; i <= 0xbfff; i++)
    {
        memset( buf, 'a', 10 );
        len = GlobalGetAtomNameA( (ATOM)i, buf, 10 );
        if (i)
        {
            char res[20];
            ok( (len > 1) && (len < 7), "bad length %d\n", len );
            sprintf( res, "#%d", i );
            memset( res + strlen(res) + 1, 'a', 10 );
            ok( !memcmp( res, buf, 10 ), "bad buffer contents %s\n", buf );
            if (len <= 1 || len >= 7) break;  /* don't bother testing all of them */
        }
        else
            ok( !len, "bad length %d\n", len );

        SetLastError(0xdeadbeef);
        len = GlobalGetAtomNameA( (ATOM)i, buf, 2);
        ok(!len, "bad length %d\n", len);
	ok(GetLastError() == ERROR_MORE_DATA || GetLastError() == ERROR_INVALID_PARAMETER,
            "wrong error conditions %lu for %u\n", GetLastError(), i);
    }

    memset( buf, '.', sizeof(buf) );
    len = GlobalGetAtomNameA( atom, buf, 6 );
    ok( len == 0, "bad length %d\n", len );
    ok( !memcmp( buf, "fooba\0....", 10 ), "bad buffer contents\n");
    if (unicode_OS)
    {
        static const WCHAR resW[] = {'f','o','o','b','a','r','.','.','.','.'};
        for (len = 0; len < 10; len++) bufW[len] = '.';
	SetLastError(0xdeadbeef);
        len = GlobalGetAtomNameW( atom, bufW, 6 );
        ok( len && GetLastError() == 0xdeadbeef, "GlobalGetAtomNameW failed\n" );
        ok( len == lstrlenW(foobarW), "bad length %d\n", len );
        ok( !memcmp( bufW, resW, 10*sizeof(WCHAR) ), "bad buffer contents\n" );
    }

    /* test string limits & overflow */
    do_initA(in, "abcdefghij", 255);
    atom = GlobalAddAtomA(in);
    ok(atom, "couldn't add atom for %s\n", in);
    len = GlobalGetAtomNameA(atom, out, sizeof(out));
    ok(len == 255, "length mismatch (%u instead of 255)\n", len);
    for (i = 0; i < 255; i++)
    {
        ok(out[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, out[i], "abcdefghij"[i % 10]);
    }
    ok(out[255] == '\0', "wrong end of string\n");
    memset(out, '.', sizeof(out));
    SetLastError(0xdeadbeef);
    len = GlobalGetAtomNameA(atom, out, 10);
    ok(!len, "bad length %d\n", len);
    ok(GetLastError() == ERROR_MORE_DATA, "wrong error code (%lu instead of %u)\n", GetLastError(), ERROR_MORE_DATA);
    for (i = 0; i < 9; i++)
    {
        ok(out[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, out[i], "abcdefghij"[i % 10]);
    }
    ok(out[9] == '\0', "wrong end of string\n");
    ok(out[10] == '.', "wrote after end of buf\n");
    do_initA(in, "abcdefghij", 256);
    atom = GlobalAddAtomA(in);
    ok(!atom, "succeeded\n");
    if (unicode_OS)
    {
        /* test integral atoms */
        for (i = 0; i <= 0xbfff; i++)
        {
            memset(outW, 'a', sizeof(outW));
            len = GlobalGetAtomNameW( (ATOM)i, outW, 10 );
            if (i)
            {
                WCHAR res[20];
                
                ok( (len > 1) && (len < 7), "bad length %d\n", len );
                wsprintfW( res, integfmt, i );
                memset( res + lstrlenW(res) + 1, 'a', 10 * sizeof(WCHAR));
                ok( !memcmp( res, outW, 10 * sizeof(WCHAR) ), "bad buffer contents for %d\n", i );
                if (len <= 1 || len >= 7) break;  /* don't bother testing all of them */
            }
            else
                ok( !len, "bad length %d\n", len );

            memset(outW, '.', sizeof(outW));
            SetLastError(0xdeadbeef);
            len = GlobalGetAtomNameW( (ATOM)i, outW, 1);
            if (i)
            {
                /* len == 0 with ERROR_MORE_DATA is on NT3.51 */
                ok(len == 1 || (len == 0 && GetLastError() == ERROR_MORE_DATA),
                         "0x%04x: got %u with %ld (expected '1' or '0' with "
                         "ERROR_MORE_DATA)\n", i, len, GetLastError());
                ok(outW[1] == DOUBLE('.'), "buffer overwrite\n");
            }
            else ok(len == 0 && GetLastError() == ERROR_INVALID_PARAMETER, "0 badly handled\n");
        }

        do_initW(inW, "abcdefghij", 255);
        atom = GlobalAddAtomW(inW);
        ok(atom, "couldn't add atom for %s\n", in);
        len = GlobalGetAtomNameW(atom, outW, ARRAY_SIZE(outW));
        ok(len == 255, "length mismatch (%u instead of 255)\n", len);
        for (i = 0; i < 255; i++)
        {
            ok(outW[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, outW[i], "abcdefghij"[i % 10]);
        }
        ok(outW[255] == '\0', "wrong end of string\n");
        memset(outW, '.', sizeof(outW));
        len = GlobalGetAtomNameW(atom, outW, 10);
        ok(len == 10, "succeeded\n");
        for (i = 0; i < 10; i++)
        {
            ok(outW[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, outW[i], "abcdefghij"[i % 10]);
        }
        ok(outW[10] == DOUBLE('.'), "wrote after end of buf\n");
        do_initW(inW, "abcdefghij", 256);
        atom = GlobalAddAtomW(inW);
        ok(!atom, "succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error code\n");
    }
}

static void test_error_handling(void)
{
    char buffer[260];
    WCHAR bufferW[260];
    int i;

    memset( buffer, 'a', 256 );
    buffer[256] = 0;
    ok( !GlobalAddAtomA(buffer), "add succeeded\n" );
    ok( !GlobalFindAtomA(buffer), "find succeeded\n" );

    if (unicode_OS)
    {
        for (i = 0; i < 256; i++) bufferW[i] = 'b';
        bufferW[256] = 0;
        ok( !GlobalAddAtomW(bufferW), "add succeeded\n" );
        ok( !GlobalFindAtomW(bufferW), "find succeeded\n" );
    }
}

static void test_local_add_atom(void)
{
    ATOM atom, w_atom;
    INT_PTR i;

    SetLastError( 0xdeadbeef );
    atom = AddAtomA( "foobar" );
    ok( atom >= 0xc000, "bad atom id %x\n", atom );
    ok( GetLastError() == 0xdeadbeef, "AddAtomA set last error\n" );

    /* Verify that it can be found (or not) appropriately */
    ok( FindAtomA( "foobar" ) == atom, "could not find atom foobar\n" );
    ok( FindAtomA( "FOOBAR" ) == atom, "could not find atom FOOBAR\n" );
    ok( !FindAtomA( "_foobar" ), "found _foobar\n" );

    /* Add the same atom, specifying string as unicode; should
     * find the first one, not add a new one */
    SetLastError( 0xdeadbeef );
    w_atom = AddAtomW( foobarW );
    if (w_atom && GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        unicode_OS = TRUE;
    else
        trace("WARNING: Unicode atom APIs are not supported on this platform\n");

    if (unicode_OS)
    {
        ok( w_atom == atom, "Unicode atom does not match ANSI one\n" );
        ok( GetLastError() == 0xdeadbeef, "AddAtomW set last error\n" );
    }

    /* Verify that it can be found (or not) appropriately via unicode name */
    if (unicode_OS)
    {
        ok( FindAtomW( foobarW ) == atom, "could not find atom foobar\n" );
        ok( FindAtomW( FOOBARW ) == atom, "could not find atom FOOBAR\n" );
        ok( !FindAtomW( _foobarW ), "found _foobar\n" );
    }

    /* Test integer atoms
     * (0x0001 .. 0xbfff) should be valid;
     * (0xc000 .. 0xffff) should be invalid */

    SetLastError( 0xdeadbeef );
    ok( AddAtomA(0) == 0 && GetLastError() == 0xdeadbeef, "succeeded to add atom 0\n" );
    if (unicode_OS)
    {
        SetLastError( 0xdeadbeef );
        ok( AddAtomW(0) == 0 && GetLastError() == 0xdeadbeef, "succeeded to add atom 0\n" );
    }

    SetLastError( 0xdeadbeef );
    for (i = 1; i <= 0xbfff; i++)
    {
        SetLastError( 0xdeadbeef );
        ok( AddAtomA((LPCSTR)i) == i && GetLastError() == 0xdeadbeef,
            "failed to add atom %Ix\n", i );
        if (unicode_OS)
        {
            SetLastError( 0xdeadbeef );
            ok( AddAtomW((LPCWSTR)i) == i && GetLastError() == 0xdeadbeef,
                "failed to add atom %Ix\n", i );
        }
    }

    for (i = 0xc000; i <= 0xffff; i++)
    {
        ok( !AddAtomA((LPCSTR)i), "succeeded adding %Ix\n", i );
        if (unicode_OS)
            ok( !AddAtomW((LPCWSTR)i), "succeeded adding %Ix\n", i );
    }
}

static void test_local_get_atom_name(void)
{
    char buf[10], in[257], out[257];
    WCHAR bufW[10], inW[257], outW[257];
    int i;
    UINT len;
    static const WCHAR resultW[] = {'f','o','o','b','a','r',0,'.','.','.'};

    ATOM atom = AddAtomA( "foobar" );

    /* Get the name of the atom we added above */
    memset( buf, '.', sizeof(buf) );
    len = GetAtomNameA( atom, buf, 10 );
    ok( len == strlen("foobar"), "bad length %d\n", len );
    ok( !memcmp( buf, "foobar\0...", 10 ), "bad buffer contents\n" );

    /* Repeat, unicode-style */
    if (unicode_OS)
    {
        for (i = 0; i < 10; i++) bufW[i] = '.';
        SetLastError( 0xdeadbeef );
        len = GetAtomNameW( atom, bufW, 10 );
        ok( len && GetLastError() == 0xdeadbeef, "GetAtomNameW failed\n" );
        ok( len == lstrlenW(foobarW), "bad length %d\n", len );
        ok( !memcmp( bufW, resultW, 10*sizeof(WCHAR) ), "bad buffer contents\n" );
    }

    /* Get the name of the atom we added above */
    memset( buf, '.', sizeof(buf) );
    len = GetAtomNameA( atom, buf, 6 );
    ok( len == 5, "bad length %d\n", len );
    ok( !memcmp( buf, "fooba\0....", 10 ), "bad buffer contents\n" );
 
    /* Repeat, unicode-style */
    if (unicode_OS)
    {
        WCHAR resW[] = {'f','o','o','b','a','\0','.','.','.','.'};
        for (i = 0; i < 10; i++) bufW[i] = '.';
        SetLastError( 0xdeadbeef );
        len = GetAtomNameW( atom, bufW, 6 );
        ok( len && GetLastError() == 0xdeadbeef, "GlobalGetAtomNameW failed\n" );
        ok( len == 5, "bad length %d\n", len );
        ok( !memcmp( bufW, resW, 10*sizeof(WCHAR) ), "bad buffer contents\n" );
    }
 
    /* Check error code returns */
    memset(buf, '.', 10);
    ok( !GetAtomNameA( atom, buf,  0 ), "succeeded\n" );
    ok( !memcmp( buf, "..........", 10 ), "should not touch buffer\n" );

    if (unicode_OS)
    {
        static const WCHAR sampleW[] = {'.','.','.','.','.','.','.','.','.','.'};

        for (i = 0; i < 10; i++) bufW[i] = '.';
        ok( !GetAtomNameW( atom, bufW, 0 ), "succeeded\n" );
        ok( !memcmp( bufW, sampleW, sizeof(sampleW) ), "should not touch buffer\n" );
    }

    /* Test integer atoms */
    for (i = 0; i <= 0xbfff; i++)
    {
        memset( buf, 'a', 10 );
        len = GetAtomNameA( (ATOM)i, buf, 10 );
        if (i)
        {
            char res[20];
            ok( (len > 1) && (len < 7), "bad length %d for %s\n", len, buf );
            sprintf( res, "#%d", i );
            memset( res + strlen(res) + 1, 'a', 10 );
            ok( !memcmp( res, buf, 10 ), "bad buffer contents %s\n", buf );
        }
        else
            ok( !len, "bad length %d\n", len );

        len = GetAtomNameA( (ATOM)i, buf, 1);
        ok(!len, "succeed with %u for %u\n", len, i);

        /* ERROR_MORE_DATA is on nt3.51 sp5 */
        if (i)
            ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER ||
               GetLastError() == ERROR_MORE_DATA,
               "wrong error conditions %lu for %u\n", GetLastError(), i);
        else
            ok(GetLastError() == ERROR_INVALID_PARAMETER ||
               GetLastError() == ERROR_MORE_DATA,
               "wrong error conditions %lu for %u\n", GetLastError(), i);
    }
    /* test string limits & overflow */
    do_initA(in, "abcdefghij", 255);
    atom = AddAtomA(in);
    ok(atom, "couldn't add atom for %s\n", in);
    len = GetAtomNameA(atom, out, sizeof(out));
    ok(len == 255, "length mismatch (%u instead of 255)\n", len);
    for (i = 0; i < 255; i++)
    {
        ok(out[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, out[i], "abcdefghij"[i % 10]);
    }
    ok(out[255] == '\0', "wrong end of string\n");
    memset(out, '.', sizeof(out));
    len = GetAtomNameA(atom, out, 10);
    ok(len == 9, "succeeded %d\n", len);
    for (i = 0; i < 9; i++)
    {
        ok(out[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, out[i], "abcdefghij"[i % 10]);
    }
    ok(out[9] == '\0', "wrong end of string\n");
    ok(out[10] == '.', "buffer overwrite\n");
    do_initA(in, "abcdefghij", 256);
    atom = AddAtomA(in);
    ok(!atom, "succeeded\n");

    /* ERROR_MORE_DATA is on nt3.51 sp5 */
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_MORE_DATA,
       "wrong error code (%lu)\n", GetLastError());

    if (unicode_OS)
    {
        /* test integral atoms */
        for (i = 0; i <= 0xbfff; i++)
        {
            memset(outW, 'a', sizeof(outW));
            len = GetAtomNameW( (ATOM)i, outW, 10 );
            if (i)
            {
                WCHAR res[20];
                
                ok( (len > 1) && (len < 7), "bad length %d\n", len );
                wsprintfW( res, integfmt, i );
                memset( res + lstrlenW(res) + 1, 'a', 10 * sizeof(WCHAR));
                ok( !memcmp( res, outW, 10 * sizeof(WCHAR) ), "bad buffer contents for %d\n", i );
            }
            else
                ok( !len, "bad length %d\n", len );

            len = GetAtomNameW( (ATOM)i, outW, 1);
            ok(!len, "succeed with %u for %u\n", len, i);

            /* ERROR_MORE_DATA is on nt3.51 sp5 */
            ok(GetLastError() == ERROR_MORE_DATA ||
               GetLastError() == (i ? ERROR_INSUFFICIENT_BUFFER : ERROR_INVALID_PARAMETER),
               "wrong error conditions %lu for %u\n", GetLastError(), i);
        }
        do_initW(inW, "abcdefghij", 255);
        atom = AddAtomW(inW);
        ok(atom, "couldn't add atom for %s\n", in);
        len = GetAtomNameW(atom, outW, ARRAY_SIZE(outW));
        ok(len == 255, "length mismatch (%u instead of 255)\n", len);
        for (i = 0; i < 255; i++)
        {
            ok(outW[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, outW[i], "abcdefghij"[i % 10]);
        }
        ok(outW[255] == '\0', "wrong end of string\n");
        memset(outW, '.', sizeof(outW));
        len = GetAtomNameW(atom, outW, 10);
        ok(len == 9, "succeeded %d\n", len);
        for (i = 0; i < 9; i++)
        {
            ok(outW[i] == "abcdefghij"[i % 10], "wrong string at %i (%c instead of %c)\n", i, outW[i], "abcdefghij"[i % 10]);
        }
        ok(outW[9] == '\0', "wrong end of string\n");
        ok(outW[10] == DOUBLE('.'), "buffer overwrite\n");
        do_initW(inW, "abcdefghij", 256);
        atom = AddAtomW(inW);
        ok(!atom, "succeeded\n");

        /* ERROR_MORE_DATA is on nt3.51 sp5 */
        ok(GetLastError() == ERROR_INVALID_PARAMETER ||
           GetLastError() == ERROR_MORE_DATA,
           "wrong error code (%lu)\n", GetLastError());
    }
}

static void test_local_error_handling(void)
{
    char buffer[260];
    WCHAR bufferW[260];
    int i;

    memset( buffer, 'a', 256 );
    buffer[256] = 0;
    ok( !AddAtomA(buffer), "add succeeded\n" );
    ok( !FindAtomA(buffer), "find succeeded\n" );

    if (unicode_OS)
    {
        for (i = 0; i < 256; i++) bufferW[i] = 'b';
        bufferW[256] = 0;
        ok( !AddAtomW(bufferW), "add succeeded\n" );
        ok( !FindAtomW(bufferW), "find succeeded\n" );
    }
}

START_TEST(atom)
{
    /* Global atom table seems to be available to GUI apps only in
       Win7, so let's turn this app into a GUI app */
    GetDesktopWindow();

    test_add_atom();
    test_get_atom_name();
    test_error_handling();
    test_local_add_atom();
    test_local_get_atom_name();
    test_local_error_handling();
}
