/*
 * Unit tests for MD4 functions
 *
 * Copyright 2004 Hans Leidekker
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

#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

static VOID (WINAPI *pMD4Init)( MD4_CTX *ctx );
static VOID (WINAPI *pMD4Update)( MD4_CTX *ctx, const unsigned char *src, const int len );
static VOID (WINAPI *pMD4Final)( MD4_CTX *ctx );
static int (WINAPI *pSystemFunction007)(const UNICODE_STRING *, LPBYTE);
typedef int (WINAPI *md4hashfunc)(LPVOID, const LPBYTE, LPBYTE);

static md4hashfunc pSystemFunction010;
static md4hashfunc pSystemFunction011;

#define ctxcmp( a, b ) memcmp( a, b, FIELD_OFFSET( MD4_CTX, in ) )

static void test_md4_ctx(void)
{
    static unsigned char message[] =
        "In our Life there's If"
        "In our beliefs there's Lie"
        "In our business there is Sin"
        "In our bodies, there is Die";

    int size = sizeof(message) - 1;

    MD4_CTX ctx;
    MD4_CTX ctx_initialized = 
    {
        { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 },
        { 0, 0 }
    };

    MD4_CTX ctx_update1 =
    {
        { 0x5e592ef7, 0xbdcb1567, 0x2b626d17, 0x7d1198bd },
        { 0x00000338, 0 }
    };

    MD4_CTX ctx_update2 =
    {
        { 0x05dcfd65, 0xb3711c0d, 0x9e3369c2, 0x903ead11 },
        { 0x00000670, 0 }
    };

    unsigned char expect[16] =
        { 0x5f, 0xd3, 0x9b, 0x29, 0x47, 0x53, 0x47, 0xaf,
          0xa5, 0xba, 0x0c, 0x05, 0xff, 0xc0, 0xc7, 0xda };


    memset( &ctx, 0, sizeof(ctx) );
    pMD4Init( &ctx );
    ok( !ctxcmp( &ctx, &ctx_initialized ), "invalid initialization\n" );

    pMD4Update( &ctx, message, size );
    ok( !ctxcmp( &ctx, &ctx_update1 ), "update doesn't work correctly\n" );

    pMD4Update( &ctx, message, size );
    ok( !ctxcmp( &ctx, &ctx_update2 ), "update doesn't work correctly\n" );

    pMD4Final( &ctx );
    ok( ctxcmp( &ctx, &ctx_initialized ), "context has changed\n" );
    ok( !memcmp( ctx.digest, expect, sizeof(expect) ), "incorrect result\n" );

}

static void test_SystemFunction007(void)
{
    int r;
    UNICODE_STRING str;
    BYTE output[0x10];
    BYTE expected[0x10] = { 0x24, 0x0a, 0xf0, 0x9d, 0x84, 0x1c, 0xda, 0xcf, 
                            0x56, 0xeb, 0x6b, 0x96, 0x55, 0xec, 0xcf, 0x0a };
    WCHAR szFoo[] = {'f','o','o',0 };

    if (0)
    {
    /* crashes on Windows */
    r = pSystemFunction007(NULL, NULL);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");
    }

    str.Buffer = szFoo;
    str.Length = 4*sizeof(WCHAR);
    str.MaximumLength = str.Length;

    memset(output, 0, sizeof output);
    r = pSystemFunction007(&str, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");

    ok(!memcmp(output, expected, sizeof expected), "response wrong\n");
}

static void test_md4hashfunc(md4hashfunc func)
{
    unsigned char expected[0x10] = {
        0x48, 0x7c, 0x3f, 0x5e, 0x2b, 0x0d, 0x6a, 0x79,
        0x32, 0x4e, 0xcd, 0xbe, 0x9c, 0x15, 0x16, 0x6f };
    unsigned char in[0x10], output[0x10];
    int r;

    memset(in, 0, sizeof in);
    memset(output, 0, sizeof output);
    r = func(0, in, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");
    ok( !memcmp(expected, output, sizeof output), "output wrong\n");
}

START_TEST(crypt_md4)
{
    HMODULE module;

    module = GetModuleHandleA( "advapi32.dll" );

    pMD4Init = (void *)GetProcAddress( module, "MD4Init" );
    pMD4Update = (void *)GetProcAddress( module, "MD4Update" );
    pMD4Final = (void *)GetProcAddress( module, "MD4Final" );

    if (pMD4Init && pMD4Update && pMD4Final)
        test_md4_ctx();
    else
        win_skip("MD4Init and/or MD4Update and/or MD4Final are not available\n");

    pSystemFunction007 = (void *)GetProcAddress( module, "SystemFunction007" );
    if (pSystemFunction007)
        test_SystemFunction007();
    else
        win_skip("SystemFunction007 is not available\n");

    pSystemFunction010 = (md4hashfunc)GetProcAddress( module, "SystemFunction010" );
    if (pSystemFunction010)
        test_md4hashfunc(pSystemFunction010);
    else
        win_skip("SystemFunction010 is not available\n");

    pSystemFunction011 = (md4hashfunc)GetProcAddress( module, "SystemFunction011" );
    if (pSystemFunction011)
        test_md4hashfunc(pSystemFunction011);
    else
        win_skip("SystemFunction011 is not available\n");
}
