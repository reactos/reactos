/*
 * Unit tests for MD5 functions
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

static VOID (WINAPI *pMD5Init)( MD5_CTX *ctx );
static VOID (WINAPI *pMD5Update)( MD5_CTX *ctx, const unsigned char *src, const int len );
static VOID (WINAPI *pMD5Final)( MD5_CTX *ctx );

#define ctxcmp( a, b ) memcmp( a, b, FIELD_OFFSET( MD5_CTX, in ) )

static void test_md5_ctx(void)
{
    static unsigned char message[] =
        "In our Life there's If"
        "In our beliefs there's Lie"
        "In our business there is Sin"
        "In our bodies, there is Die";

    int size = sizeof(message) - 1;
    HMODULE module;

    MD5_CTX ctx;
    MD5_CTX ctx_initialized = 
    {
        { 0, 0 },
        { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 }
    };

    MD5_CTX ctx_update1 =
    {
        { 0x00000338, 0 },
        { 0x068cb64d, 0xb7a05790, 0x426979ee, 0xed67e221 }
    };

    MD5_CTX ctx_update2 =
    {
        { 0x00000670, 0 },
        { 0x2f7afe58, 0xcc3e9315, 0x709c465c, 0xbf6414c8 }
    };

    unsigned char expect[16] =
        { 0x43, 0x03, 0xdd, 0x8c, 0x60, 0xd9, 0x3a, 0x22,
          0x0b, 0x28, 0xd0, 0xb2, 0x65, 0x93, 0xd0, 0x36 };

    module = GetModuleHandleA("advapi32.dll");

    pMD5Init = (void *)GetProcAddress( module, "MD5Init" );
    pMD5Update = (void *)GetProcAddress( module, "MD5Update" );
    pMD5Final = (void *)GetProcAddress( module, "MD5Final" );

    if (!pMD5Init || !pMD5Update || !pMD5Final)
    {
        win_skip("Needed functions are not available\n");
        return;
    }

    memset( &ctx, 0, sizeof(ctx) );
    pMD5Init( &ctx );
    ok( !ctxcmp( &ctx, &ctx_initialized ), "invalid initialization\n" );

    pMD5Update( &ctx, message, size );
    ok( !ctxcmp( &ctx, &ctx_update1 ), "update doesn't work correctly\n" );

    pMD5Update( &ctx, message, size );
    ok( !ctxcmp( &ctx, &ctx_update2 ), "update doesn't work correctly\n" );

    pMD5Final( &ctx );
    ok( ctxcmp( &ctx, &ctx_initialized ), "context has changed\n" );
    ok( !memcmp( ctx.digest, expect, sizeof(expect) ), "incorrect result\n" );
}

START_TEST(crypt_md5)
{
    test_md5_ctx();
}
