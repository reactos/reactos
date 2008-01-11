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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>

#include "rpc.h"
#include "rpcdce.h"

typedef unsigned int unsigned32;
typedef struct twr_t
    {
    unsigned32 tower_length;
    /* [size_is] */ byte tower_octet_string[ 1 ];
    } 	twr_t;

RPC_STATUS WINAPI TowerExplode(const twr_t *tower, RPC_SYNTAX_IDENTIFIER *object, RPC_SYNTAX_IDENTIFIER *syntax, char **protseq, char **endpoint, char **address);
RPC_STATUS WINAPI TowerConstruct(const RPC_SYNTAX_IDENTIFIER *object, const RPC_SYNTAX_IDENTIFIER *syntax, const char *protseq, const char *endpoint, const char *address, twr_t **tower);

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

static void UuidConversionAndComparison(void) {
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
	    ok( (UuidEqual(PUuid1, PUuid2, &rslt) == Uuid_Comparison_Grid[i1][i2]), "UUID Equality\n" );
        }

    /* Uuid to String to Uuid (char) */
    for (i1 = 0; i1 < 10; i1++) {
        Uuid1 = Uuid_Table[i1];
	ok( (UuidToStringA(&Uuid1, (unsigned char**)&str) == RPC_S_OK), "Simple UUID->String copy\n" );
	ok( (UuidFromStringA((unsigned char*)str, &Uuid2) == RPC_S_OK), "Simple String->UUID copy from generated UUID String\n" );
	ok( UuidEqual(&Uuid1, &Uuid2, &rslt), "Uuid -> String -> Uuid transform\n" );
	/* invalid uuid tests  -- size of valid UUID string=36 */
	for (i2 = 0; i2 < 36; i2++) {
	    x = str[i2];
	    str[i2] = 'g'; /* whatever, but "g" is a good boundary condition */
	    ok( (UuidFromStringA((unsigned char*)str, &Uuid1) == RPC_S_INVALID_STRING_UUID), "Invalid UUID String\n" );
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
	ok( (rslt == RPC_S_OK), "Simple UUID->WString copy\n" );
	ok( (UuidFromStringW(wstr, &Uuid2) == RPC_S_OK), "Simple WString->UUID copy from generated UUID String\n" );
	ok( UuidEqual(&Uuid1, &Uuid2, &rslt), "Uuid -> WString -> Uuid transform\n" );
	/* invalid uuid tests  -- size of valid UUID string=36 */
	for (i2 = 0; i2 < 36; i2++) {
	    wx = wstr[i2];
	    wstr[i2] = 'g'; /* whatever, but "g" is a good boundary condition */
	    ok( (UuidFromStringW(wstr, &Uuid1) == RPC_S_INVALID_STRING_UUID), "Invalid UUID WString\n" );
	    wstr[i2] = wx; /* change it back so remaining tests are interesting. */
	}
    }
}

static void TestDceErrorInqText (void)
{
    char bufferInvalid [1024];
    char buffer [1024]; /* The required size is not documented but would
                         * appear to be 256.
                         */
    DWORD dwCount;

    dwCount = FormatMessageA (FORMAT_MESSAGE_FROM_SYSTEM | 
              FORMAT_MESSAGE_IGNORE_INSERTS,
              NULL, RPC_S_NOT_RPC_ERROR, 0, bufferInvalid,
              sizeof(bufferInvalid)/sizeof(bufferInvalid[0]), NULL);

    /* A random sample of DceErrorInqText */
    /* 0 is success */
    ok ((DceErrorInqTextA (0, (unsigned char*)buffer) == RPC_S_OK),
            "DceErrorInqTextA(0...)\n");
    /* A real RPC_S error */
    ok ((DceErrorInqTextA (RPC_S_INVALID_STRING_UUID, (unsigned char*)buffer) == RPC_S_OK),
            "DceErrorInqTextA(valid...)\n");

    if (dwCount)
    {
        /* A message for which FormatMessage should fail
         * which should return RPC_S_OK and the 
         * fixed "not valid" message
         */
        ok ((DceErrorInqTextA (35, (unsigned char*)buffer) == RPC_S_OK &&
                    strcmp (buffer, bufferInvalid) == 0),
                "DceErrorInqTextA(unformattable...)\n");
        /* One for which FormatMessage should succeed but 
         * DceErrorInqText should "fail"
         * 3814 is generally quite a long message
         */
        ok ((DceErrorInqTextA (3814, (unsigned char*)buffer) == RPC_S_OK &&
                    strcmp (buffer, bufferInvalid) == 0),
                "DceErrorInqTextA(deviation...)\n");
    }
    else
        ok (0, "Cannot set up for DceErrorInqText\n");
}

static RPC_DISPATCH_FUNCTION IFoo_table[] =
{
    0
};

static RPC_DISPATCH_TABLE IFoo_v0_0_DispatchTable =
{
    0,
    IFoo_table
};

static const RPC_SERVER_INTERFACE IFoo___RpcServerInterface =
{
    sizeof(RPC_SERVER_INTERFACE),
    {{0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x12,0x34}},{0,0}},
    {{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},
    &IFoo_v0_0_DispatchTable,
    0,
    0,
    0,
    0,
    0,
};

static RPC_IF_HANDLE IFoo_v0_0_s_ifspec = (RPC_IF_HANDLE)& IFoo___RpcServerInterface;

static void test_rpc_ncacn_ip_tcp(void)
{
    RPC_STATUS status;
    unsigned char *binding;
    handle_t IFoo_IfHandle;
    static unsigned char foo[] = "foo";
    static unsigned char ncacn_ip_tcp[] = "ncacn_ip_tcp";
    static unsigned char address[] = "127.0.0.1";
    static unsigned char endpoint[] = "4114";

    status = RpcNetworkIsProtseqValid(foo);
    ok(status == RPC_S_INVALID_RPC_PROTSEQ, "return wrong\n");

    status = RpcNetworkIsProtseqValid(ncacn_ip_tcp);
    ok(status == RPC_S_OK, "return wrong\n");

    status = RpcMgmtStopServerListening(NULL);
todo_wine {
    ok(status == RPC_S_NOT_LISTENING,
       "wrong RpcMgmtStopServerListening error (%lu)\n", status);
}

    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_NOT_LISTENING,
       "wrong RpcMgmtWaitServerListen error status (%lu)\n", status);

    status = RpcServerListen(1, 20, FALSE);
    ok(status == RPC_S_NO_PROTSEQS_REGISTERED,
       "wrong RpcServerListen error (%lu)\n", status);

    status = RpcServerUseProtseqEp(ncacn_ip_tcp, 20, endpoint, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp failed (%lu)\n", status);

    status = RpcServerRegisterIf(IFoo_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf failed (%lu)\n", status);

    status = RpcServerListen(1, 20, TRUE);
todo_wine {
    ok(status == RPC_S_OK, "RpcServerListen failed (%lu)\n", status);
}

    status = RpcServerListen(1, 20, TRUE);
todo_wine {
    ok(status == RPC_S_ALREADY_LISTENING,
       "wrong RpcServerListen error (%lu)\n", status);
}

    status = RpcStringBindingCompose(NULL, ncacn_ip_tcp, address,
                                     endpoint, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose failed (%lu)\n", status);

    status = RpcBindingFromStringBinding(binding, &IFoo_IfHandle);
    ok(status == RPC_S_OK, "RpcBindingFromStringBinding failed (%lu)\n",
       status);

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_OK, "RpcMgmtStopServerListening failed (%lu)\n",
       status);

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_OK, "RpcMgmtStopServerListening failed (%lu)\n",
       status);

    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
    ok(status == RPC_S_OK, "RpcServerUnregisterIf failed (%lu)\n", status);

    status = RpcMgmtWaitServerListen();
todo_wine {
    ok(status == RPC_S_OK, "RpcMgmtWaitServerListen failed (%lu)\n", status);
}

    status = RpcStringFree(&binding);
    ok(status == RPC_S_OK, "RpcStringFree failed (%lu)\n", status);

    status = RpcBindingFree(&IFoo_IfHandle);
    ok(status == RPC_S_OK, "RpcBindingFree failed (%lu)\n", status);
}

/* this is what's generated with MS/RPC - it includes an extra 2
 * bytes in the protocol floor */
static const unsigned char tower_data_tcp_ip1[] =
{
    0x05,0x00,0x13,0x00,0x0d,0x00,0xdb,0xf1,
    0xa4,0x47,0xca,0x67,0x10,0xb3,0x1f,0x00,
    0xdd,0x01,0x06,0x62,0xda,0x00,0x00,0x02,
    0x00,0x00,0x00,0x13,0x00,0x0d,0x04,0x5d,
    0x88,0x8a,0xeb,0x1c,0xc9,0x11,0x9f,0xe8,
    0x08,0x00,0x2b,0x10,0x48,0x60,0x02,0x00,
    0x02,0x00,0x00,0x00,0x01,0x00,0x0b,0x02,
    0x00,0x00,0x00,0x01,0x00,0x07,0x02,0x00,
    0x00,0x87,0x01,0x00,0x09,0x04,0x00,0x0a,
    0x00,0x00,0x01,
};
/* this is the optimal data that i think should be generated */
static const unsigned char tower_data_tcp_ip2[] =
{
    0x05,0x00,0x13,0x00,0x0d,0x00,0xdb,0xf1,
    0xa4,0x47,0xca,0x67,0x10,0xb3,0x1f,0x00,
    0xdd,0x01,0x06,0x62,0xda,0x00,0x00,0x02,
    0x00,0x00,0x00,0x13,0x00,0x0d,0x04,0x5d,
    0x88,0x8a,0xeb,0x1c,0xc9,0x11,0x9f,0xe8,
    0x08,0x00,0x2b,0x10,0x48,0x60,0x02,0x00,
    0x02,0x00,0x00,0x00,0x01,0x00,0x0b,0x00,
    0x00,0x01,0x00,0x07,0x02,0x00,0x00,0x87,
    0x01,0x00,0x09,0x04,0x00,0x0a,0x00,0x00,
    0x01,
};

static void test_towers(void)
{
    RPC_STATUS ret;
    twr_t *tower;
    static const RPC_SYNTAX_IDENTIFIER mapi_if_id = { { 0xa4f1db00, 0xca47, 0x1067, { 0xb3, 0x1f, 0x00, 0xdd, 0x01, 0x06, 0x62, 0xda } }, { 0, 0 } };
    static const RPC_SYNTAX_IDENTIFIER ndr_syntax = { { 0x8a885d04, 0x1ceb, 0x11c9, { 0x9f, 0xe8, 0x08, 0x00, 0x2b, 0x10, 0x48, 0x60 } }, { 2, 0 } };
    RPC_SYNTAX_IDENTIFIER object, syntax;
    char *protseq, *endpoint, *address;
    BOOL same;

    ret = TowerConstruct(&mapi_if_id, &ndr_syntax, "ncacn_ip_tcp", "135", "10.0.0.1", &tower);
    ok(ret == RPC_S_OK, "TowerConstruct failed with error %ld\n", ret);

    /* first check we have the right amount of data */
    ok(tower->tower_length == sizeof(tower_data_tcp_ip1) ||
       tower->tower_length == sizeof(tower_data_tcp_ip2),
        "Wrong size of tower %d\n", tower->tower_length);

    /* then do a byte-by-byte comparison */
    same = ((tower->tower_length == sizeof(tower_data_tcp_ip1)) &&
            !memcmp(&tower->tower_octet_string, tower_data_tcp_ip1, sizeof(tower_data_tcp_ip1))) ||
           ((tower->tower_length == sizeof(tower_data_tcp_ip2)) &&
            !memcmp(&tower->tower_octet_string, tower_data_tcp_ip2, sizeof(tower_data_tcp_ip2)));

    ok(same, "Tower data differs\n");
    if (!same)
    {
        unsigned32 i;
        for (i = 0; i < tower->tower_length; i++)
        {
            if (i % 8 == 0) printf("    ");
            printf("0x%02x,", tower->tower_octet_string[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    ret = TowerExplode(tower, &object, &syntax, &protseq, &endpoint, &address);
    ok(ret == RPC_S_OK, "TowerExplode failed with error %ld\n", ret);
    ok(!memcmp(&object, &mapi_if_id, sizeof(mapi_if_id)), "object id didn't match\n");
    ok(!memcmp(&syntax, &ndr_syntax, sizeof(syntax)), "syntax id didn't match\n");
    ok(!strcmp(protseq, "ncacn_ip_tcp"), "protseq was \"%s\" instead of \"ncacn_ip_tcp\"\n", protseq);
    ok(!strcmp(endpoint, "135"), "endpoint was \"%s\" instead of \"135\"\n", endpoint);
    ok(!strcmp(address, "10.0.0.1"), "address was \"%s\" instead of \"10.0.0.1\"\n", address);

    I_RpcFree(protseq);
    I_RpcFree(endpoint);
    I_RpcFree(address);

    ret = TowerExplode(tower, NULL, NULL, NULL, NULL, NULL);
    ok(ret == RPC_S_OK, "TowerExplode failed with error %ld\n", ret);

    I_RpcFree(tower);

    /* test the behaviour for ip_tcp with name instead of dotted IP notation */
    ret = TowerConstruct(&mapi_if_id, &ndr_syntax, "ncacn_ip_tcp", "135", "localhost", &tower);
    ok(ret == RPC_S_OK, "TowerConstruct failed with error %ld\n", ret);
    ret = TowerExplode(tower, NULL, NULL, NULL, NULL, &address);
    ok(ret == RPC_S_OK, "TowerExplode failed with error %ld\n", ret);
    ok(!strcmp(address, "0.0.0.0"), "address was \"%s\" instead of \"0.0.0.0\"\n", address);

    I_RpcFree(address);
    I_RpcFree(tower);
}

START_TEST( rpc )
{
    trace ( " ** Uuid Conversion and Comparison Tests **\n" );
    UuidConversionAndComparison();
    trace ( " ** DceErrorInqText **\n");
    TestDceErrorInqText();
    test_rpc_ncacn_ip_tcp();
    test_towers();
}
