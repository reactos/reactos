/*
 * Unit tests for SystemFunction006 (LMHash?)
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

typedef VOID (WINAPI *fnSystemFunction006)( PCSTR passwd, PSTR lmhash );
fnSystemFunction006 pSystemFunction006;

static void test_SystemFunction006()
{
    static unsigned char lmhash[16 + 1];

    unsigned char passwd[] = { 's','e','c','r','e','t', 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned char expect[] =
        { 0x85, 0xf5, 0x28, 0x9f, 0x09, 0xdc, 0xa7, 0xeb,
          0xaa, 0xd3, 0xb4, 0x35, 0xb5, 0x14, 0x04, 0xee };

    pSystemFunction006( passwd, lmhash );

    ok( !memcmp( lmhash, expect, sizeof(expect) ),
        "lmhash: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
        lmhash[0], lmhash[1], lmhash[2], lmhash[3], lmhash[4], lmhash[5],
        lmhash[6], lmhash[7], lmhash[8], lmhash[9], lmhash[10], lmhash[11],
        lmhash[12], lmhash[13], lmhash[14], lmhash[15] );
}

START_TEST(crypt_lmhash)
{
    HMODULE module;

    if (!(module = LoadLibrary("advapi32.dll"))) return;

    pSystemFunction006 = (fnSystemFunction006)GetProcAddress( module, "SystemFunction006" );

    if (!pSystemFunction006) goto out;

    if (pSystemFunction006)
        test_SystemFunction006();

out:
    FreeLibrary( module );
}
