/*
 * Unit test suite for rpc functions
 *
 * Copyright 2002 Greg Turner
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
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>

#include "wine/unicode.h"
#include "rpc.h"

static UUID Uuid_Table[10] = {
  { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} }, /* 0 (null) */
  { 0xdeadbeef, 0xdead, 0xbeef, {0x10, 0x21, 0x35, 0x56, 0x89, 0xa0, 0xf4, 0x8a} }, /* 1 */
  { 0xabadfeed, 0x49ff, 0xbead, {0x8a, 0xf4, 0xa0, 0x89, 0x56, 0x35, 0x21, 0x10} }, /* 2 */
  { 0x93da375c, 0x1324, 0x1355, {0x87, 0xff, 0x49, 0x44, 0x34, 0x44, 0x22, 0x19} }, /* 3 */
  { 0xdeadbeef, 0xdead, 0xbeef, {0x10, 0x21, 0x35, 0x56, 0x89, 0xa0, 0xf4, 0x8b} }, /* 4 (~1) */
  { 0x9badfeed, 0x49ff, 0xbead, {0x8a, 0xf4, 0xa0, 0x89, 0x56, 0x35, 0x21, 0x10} }, /* 5 (~2) */
  { 0x00000000, 0x0001, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} }, /* 6 (~0) */
  { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01} }, /* 7 (~0) */
  { 0x12312312, 0x1231, 0x1231, {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff} }, /* 8 */
  { 0x11111111, 0x1111, 0x1111, {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11} }  /* 9 */
};

/* index of "10" means "NULL" */
static BOOL Uuid_Comparison_Grid[11][11] = {
  { TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE  },
  { FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
  { FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
  { FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
  { FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
  { FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE },
  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE },
  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE },
  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE },
  { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE },
  { TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE  }
};

void UuidConversionAndComparison(void) {
    CHAR strx[100], x;
    LPSTR str = strx;
    WCHAR wstrx[100], wx;
    LPWSTR wstr = wstrx;

    UUID Uuid1, Uuid2, *PUuid1, *PUuid2;
    RPC_STATUS rslt;

    int i1,i2;

    /* Uuid Equality */
    for (i1 = 0; i1 < 11; i1++)
        for (i2 = 0; i2 < 11; i2++) {
	    if (i1 < 10) {
	        Uuid1 = Uuid_Table[i1]; 
		PUuid1 = &Uuid1;
            } else {
	        PUuid1 = NULL;
	    }        
	    if (i2 < 10) {
	        Uuid2 = Uuid_Table[i2];
		PUuid2 = &Uuid2;
            } else {
	        PUuid2 = NULL;
	    }
	    ok( (UuidEqual(PUuid1, PUuid2, &rslt) == Uuid_Comparison_Grid[i1][i2]), "UUID Equality" );
        }

    /* Uuid to String to Uuid (char) */
    for (i1 = 0; i1 < 10; i1++) {
        Uuid1 = Uuid_Table[i1];
	ok( (UuidToStringA(&Uuid1, (unsigned char**)&str) == RPC_S_OK), "Simple UUID->String copy" );
	ok( (UuidFromStringA(str, &Uuid2) == RPC_S_OK), "Simple String->UUID copy from generated UUID String" );
	ok( UuidEqual(&Uuid1, &Uuid2, &rslt), "Uuid -> String -> Uuid transform" );
	/* invalid uuid tests  -- size of valid UUID string=36 */
	for (i2 = 0; i2 < 36; i2++) {
	    x = str[i2];
	    str[i2] = 'g'; /* whatever, but "g" is a good boundary condition */
	    ok( (UuidFromStringA(str, &Uuid1) == RPC_S_INVALID_STRING_UUID), "Invalid UUID String" );
	    str[i2] = x; /* change it back so remaining tests are interesting. */
	}
    }

    /* Uuid to String to Uuid (wchar) */
    for (i1 = 0; i1 < 10; i1++) {
        Uuid1 = Uuid_Table[i1];
        rslt=UuidToStringW(&Uuid1, &wstr);
        if (rslt==RPC_S_CANNOT_SUPPORT) {
            /* Must be Win9x (no Unicode support), skip the tests */
            break;
        }
	ok( (rslt == RPC_S_OK), "Simple UUID->WString copy" );
	ok( (UuidFromStringW(wstr, &Uuid2) == RPC_S_OK), "Simple WString->UUID copy from generated UUID String" );
	ok( UuidEqual(&Uuid1, &Uuid2, &rslt), "Uuid -> WString -> Uuid transform" );
	/* invalid uuid tests  -- size of valid UUID string=36 */
	for (i2 = 0; i2 < 36; i2++) {
	    wx = wstr[i2];
	    wstr[i2] = 'g'; /* whatever, but "g" is a good boundary condition */
	    ok( (UuidFromStringW(wstr, &Uuid1) == RPC_S_INVALID_STRING_UUID), "Invalid UUID WString" );
	    wstr[i2] = wx; /* change it back so remaining tests are interesting. */
	}
    }
}

START_TEST( rpc )
{
    trace ( " ** Uuid Conversion and Comparison Tests **\n" );
    UuidConversionAndComparison();
}
