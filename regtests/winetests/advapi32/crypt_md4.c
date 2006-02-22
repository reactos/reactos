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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

typedef struct
{
    unsigned int buf[4];
    unsigned int i[2];
    unsigned char in[64];
    unsigned char digest[16];
} MD4_CTX;

typedef VOID (WINAPI *fnMD4Init)( MD4_CTX *ctx );
typedef VOID (WINAPI *fnMD4Update)( MD4_CTX *ctx, const unsigned char *src, const int len );
typedef VOID (WINAPI *fnMD4Final)( MD4_CTX *ctx );

fnMD4Init pMD4Init;
fnMD4Update pMD4Update;
fnMD4Final pMD4Final;

#define ctxcmp( a, b ) memcmp( (char*)a, (char*)b, FIELD_OFFSET( MD4_CTX, in ) )

void test_md4_ctx()
{
    static unsigned char message[] =
        "In our Life there's If"
        "In our beliefs there's Lie"
        "In our business there is Sin"
        "In our bodies, there is Die";

    int size = strlen( message );
    HMODULE module;

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

    if (!(module = LoadLibrary( "advapi32.dll" ))) return;

    pMD4Init = (fnMD4Init)GetProcAddress( module, "MD4Init" );
    pMD4Update = (fnMD4Update)GetProcAddress( module, "MD4Update" );
    pMD4Final = (fnMD4Final)GetProcAddress( module, "MD4Final" );

    if (!pMD4Init || !pMD4Update || !pMD4Final) goto out;

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

out:
    FreeLibrary( module );
}

START_TEST(crypt_md4)
{
    test_md4_ctx();
}
