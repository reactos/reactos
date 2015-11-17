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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winerror.h>

#include "rpc.h"
#include "rpcdce.h"
#include "secext.h"

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
	RpcStringFreeA((unsigned char **)&str);
    }

    /* Uuid to String to Uuid (wchar) */
    for (i1 = 0; i1 < 10; i1++) {
        Uuid1 = Uuid_Table[i1];
        rslt=UuidToStringW(&Uuid1, &wstr);
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
	RpcStringFreeW(&wstr);
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
    unsigned char *binding, *principal;
    handle_t IFoo_IfHandle;
    ULONG level, authnsvc, authzsvc;
    RPC_AUTH_IDENTITY_HANDLE identity;
    static unsigned char foo[] = "foo";
    static unsigned char ncacn_ip_tcp[] = "ncacn_ip_tcp";
    static unsigned char address[] = "127.0.0.1";
    static unsigned char endpoint[] = "4114";
    static unsigned char spn[] = "principal";

    status = RpcNetworkIsProtseqValidA(foo);
    ok(status == RPC_S_INVALID_RPC_PROTSEQ, "return wrong\n");

    status = RpcNetworkIsProtseqValidA(ncacn_ip_tcp);
    ok(status == RPC_S_OK, "return wrong\n");

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_NOT_LISTENING,
       "wrong RpcMgmtStopServerListening error (%u)\n", status);

    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_NOT_LISTENING,
       "wrong RpcMgmtWaitServerListen error status (%u)\n", status);

    status = RpcServerListen(1, 20, FALSE);
    ok(status == RPC_S_NO_PROTSEQS_REGISTERED,
       "wrong RpcServerListen error (%u)\n", status);

    status = RpcServerUseProtseqEpA(ncacn_ip_tcp, 20, endpoint, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp failed (%u)\n", status);

    status = RpcServerRegisterIf(IFoo_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf failed (%u)\n", status);

    status = RpcServerListen(1, 20, TRUE);
    ok(status == RPC_S_OK, "RpcServerListen failed (%u)\n", status);

    status = RpcServerListen(1, 20, TRUE);
todo_wine {
    ok(status == RPC_S_ALREADY_LISTENING,
       "wrong RpcServerListen error (%u)\n", status);
}

    status = RpcStringBindingComposeA(NULL, ncacn_ip_tcp, address,
                                     endpoint, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose failed (%u)\n", status);

    status = RpcBindingFromStringBindingA(binding, &IFoo_IfHandle);
    ok(status == RPC_S_OK, "RpcBindingFromStringBinding failed (%u)\n",
       status);

    status = RpcBindingSetAuthInfoA(IFoo_IfHandle, NULL, RPC_C_AUTHN_LEVEL_NONE,
                                   RPC_C_AUTHN_WINNT, NULL, RPC_C_AUTHZ_NAME);
    ok(status == RPC_S_OK, "RpcBindingSetAuthInfo failed (%u)\n", status);

    status = RpcBindingInqAuthInfoA(IFoo_IfHandle, NULL, NULL, NULL, NULL, NULL);
    ok(status == RPC_S_BINDING_HAS_NO_AUTH, "RpcBindingInqAuthInfo failed (%u)\n",
       status);

    status = RpcBindingSetAuthInfoA(IFoo_IfHandle, spn, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                   RPC_C_AUTHN_WINNT, NULL, RPC_C_AUTHZ_NAME);
    ok(status == RPC_S_OK, "RpcBindingSetAuthInfo failed (%u)\n", status);

    level = authnsvc = authzsvc = 0;
    principal = (unsigned char *)0xdeadbeef;
    identity = (RPC_AUTH_IDENTITY_HANDLE *)0xdeadbeef;
    status = RpcBindingInqAuthInfoA(IFoo_IfHandle, &principal, &level, &authnsvc,
                                   &identity, &authzsvc);

    ok(status == RPC_S_OK, "RpcBindingInqAuthInfo failed (%u)\n", status);
    ok(identity == NULL, "expected NULL identity, got %p\n", identity);
    ok(principal != (unsigned char *)0xdeadbeef, "expected valid principal, got %p\n", principal);
    ok(level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY, "expected RPC_C_AUTHN_LEVEL_PKT_PRIVACY, got %d\n", level);
    ok(authnsvc == RPC_C_AUTHN_WINNT, "expected RPC_C_AUTHN_WINNT, got %d\n", authnsvc);
    todo_wine ok(authzsvc == RPC_C_AUTHZ_NAME, "expected RPC_C_AUTHZ_NAME, got %d\n", authzsvc);
    if (status == RPC_S_OK) RpcStringFreeA(&principal);

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_OK, "RpcMgmtStopServerListening failed (%u)\n",
       status);

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_OK, "RpcMgmtStopServerListening failed (%u)\n",
       status);

    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
    ok(status == RPC_S_OK, "RpcServerUnregisterIf failed (%u)\n", status);

    status = RpcMgmtWaitServerListen();
todo_wine {
    ok(status == RPC_S_OK, "RpcMgmtWaitServerListen failed (%u)\n", status);
}

    status = RpcStringFreeA(&binding);
    ok(status == RPC_S_OK, "RpcStringFree failed (%u)\n", status);

    status = RpcBindingFree(&IFoo_IfHandle);
    ok(status == RPC_S_OK, "RpcBindingFree failed (%u)\n", status);
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
    ok(ret == RPC_S_OK ||
       broken(ret == RPC_S_INVALID_RPC_PROTSEQ), /* Vista */
       "TowerConstruct failed with error %d\n", ret);
    if (ret == RPC_S_INVALID_RPC_PROTSEQ)
    {
        /* Windows Vista fails with this error and crashes if we continue */
        win_skip("TowerConstruct failed, we are most likely on Windows Vista\n");
        return;
    }

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
    ok(ret == RPC_S_OK, "TowerExplode failed with error %d\n", ret);
    ok(!memcmp(&object, &mapi_if_id, sizeof(mapi_if_id)), "object id didn't match\n");
    ok(!memcmp(&syntax, &ndr_syntax, sizeof(syntax)), "syntax id didn't match\n");
    ok(!strcmp(protseq, "ncacn_ip_tcp"), "protseq was \"%s\" instead of \"ncacn_ip_tcp\"\n", protseq);
    ok(!strcmp(endpoint, "135"), "endpoint was \"%s\" instead of \"135\"\n", endpoint);
    ok(!strcmp(address, "10.0.0.1"), "address was \"%s\" instead of \"10.0.0.1\"\n", address);

    I_RpcFree(protseq);
    I_RpcFree(endpoint);
    I_RpcFree(address);

    ret = TowerExplode(tower, NULL, NULL, NULL, NULL, NULL);
    ok(ret == RPC_S_OK, "TowerExplode failed with error %d\n", ret);

    I_RpcFree(tower);

    /* test the behaviour for ip_tcp with name instead of dotted IP notation */
    ret = TowerConstruct(&mapi_if_id, &ndr_syntax, "ncacn_ip_tcp", "135", "localhost", &tower);
    ok(ret == RPC_S_OK, "TowerConstruct failed with error %d\n", ret);
    ret = TowerExplode(tower, NULL, NULL, NULL, NULL, &address);
    ok(ret == RPC_S_OK, "TowerExplode failed with error %d\n", ret);
    ok(!strcmp(address, "0.0.0.0") ||
       broken(!strcmp(address, "255.255.255.255")),
       "address was \"%s\" instead of \"0.0.0.0\"\n", address);

    I_RpcFree(address);
    I_RpcFree(tower);

    /* test the behaviour for np with no address */
    ret = TowerConstruct(&mapi_if_id, &ndr_syntax, "ncacn_np", "\\pipe\\test", NULL, &tower);
    ok(ret == RPC_S_OK, "TowerConstruct failed with error %d\n", ret);
    ret = TowerExplode(tower, NULL, NULL, NULL, NULL, &address);
    ok(ret == RPC_S_OK ||
       broken(ret != RPC_S_OK), /* win2k, indeterminate */
       "TowerExplode failed with error %d\n", ret);
    /* Windows XP SP3 sets address to NULL */
    ok(!address || !strcmp(address, ""), "address was \"%s\" instead of \"\" or NULL (XP SP3)\n", address);

    I_RpcFree(address);
    I_RpcFree(tower);
}

static void test_I_RpcMapWin32Status(void)
{
    LONG win32status;
    RPC_STATUS rpc_status;
    BOOL w2k3_up = FALSE;

    /* Windows 2003 and Vista return STATUS_UNSUCCESSFUL if given an unknown status */
    win32status = I_RpcMapWin32Status(9999);
    if (win32status == STATUS_UNSUCCESSFUL)
    {
        trace("We are on Windows 2003 or Vista\n");
        w2k3_up = TRUE;
    }

    /* On Windows XP-SP1 and below some statuses are not mapped and return
     * the given status
     */
    for (rpc_status = 0; rpc_status < 10000; rpc_status++)
    {
        LONG expected_win32status;
        BOOL missing = FALSE;

        win32status = I_RpcMapWin32Status(rpc_status);
        switch (rpc_status)
        {
        case ERROR_SUCCESS: expected_win32status = ERROR_SUCCESS; break;
        case ERROR_ACCESS_DENIED: expected_win32status = STATUS_ACCESS_DENIED; break;
        case ERROR_INVALID_HANDLE: expected_win32status = RPC_NT_SS_CONTEXT_MISMATCH; break;
        case ERROR_OUTOFMEMORY: expected_win32status = STATUS_NO_MEMORY; break;
        case ERROR_INVALID_PARAMETER: expected_win32status = STATUS_INVALID_PARAMETER; break;
        case ERROR_INSUFFICIENT_BUFFER: expected_win32status = STATUS_BUFFER_TOO_SMALL; break;
        case ERROR_MAX_THRDS_REACHED: expected_win32status = STATUS_NO_MEMORY; break;
        case ERROR_NOACCESS: expected_win32status = STATUS_ACCESS_VIOLATION; break;
        case ERROR_NOT_ENOUGH_SERVER_MEMORY: expected_win32status = STATUS_INSUFF_SERVER_RESOURCES; break;
        case ERROR_WRONG_PASSWORD:  expected_win32status = STATUS_WRONG_PASSWORD; missing = TRUE; break;
        case ERROR_INVALID_LOGON_HOURS: expected_win32status = STATUS_INVALID_LOGON_HOURS; missing = TRUE; break;
        case ERROR_PASSWORD_EXPIRED: expected_win32status = STATUS_PASSWORD_EXPIRED; missing = TRUE; break;
        case ERROR_ACCOUNT_DISABLED: expected_win32status = STATUS_ACCOUNT_DISABLED; missing = TRUE; break;
        case ERROR_INVALID_SECURITY_DESCR: expected_win32status = STATUS_INVALID_SECURITY_DESCR; break;
        case RPC_S_INVALID_STRING_BINDING: expected_win32status = RPC_NT_INVALID_STRING_BINDING; break;
        case RPC_S_WRONG_KIND_OF_BINDING: expected_win32status = RPC_NT_WRONG_KIND_OF_BINDING; break;
        case RPC_S_INVALID_BINDING: expected_win32status = RPC_NT_INVALID_BINDING; break;
        case RPC_S_PROTSEQ_NOT_SUPPORTED: expected_win32status = RPC_NT_PROTSEQ_NOT_SUPPORTED; break;
        case RPC_S_INVALID_RPC_PROTSEQ: expected_win32status = RPC_NT_INVALID_RPC_PROTSEQ; break;
        case RPC_S_INVALID_STRING_UUID: expected_win32status = RPC_NT_INVALID_STRING_UUID; break;
        case RPC_S_INVALID_ENDPOINT_FORMAT: expected_win32status = RPC_NT_INVALID_ENDPOINT_FORMAT; break;
        case RPC_S_INVALID_NET_ADDR: expected_win32status = RPC_NT_INVALID_NET_ADDR; break;
        case RPC_S_NO_ENDPOINT_FOUND: expected_win32status = RPC_NT_NO_ENDPOINT_FOUND; break;
        case RPC_S_INVALID_TIMEOUT: expected_win32status = RPC_NT_INVALID_TIMEOUT; break;
        case RPC_S_OBJECT_NOT_FOUND: expected_win32status = RPC_NT_OBJECT_NOT_FOUND; break;
        case RPC_S_ALREADY_REGISTERED: expected_win32status = RPC_NT_ALREADY_REGISTERED; break;
        case RPC_S_TYPE_ALREADY_REGISTERED: expected_win32status = RPC_NT_TYPE_ALREADY_REGISTERED; break;
        case RPC_S_ALREADY_LISTENING: expected_win32status = RPC_NT_ALREADY_LISTENING; break;
        case RPC_S_NO_PROTSEQS_REGISTERED: expected_win32status = RPC_NT_NO_PROTSEQS_REGISTERED; break;
        case RPC_S_NOT_LISTENING: expected_win32status = RPC_NT_NOT_LISTENING; break;
        case RPC_S_UNKNOWN_MGR_TYPE: expected_win32status = RPC_NT_UNKNOWN_MGR_TYPE; break;
        case RPC_S_UNKNOWN_IF: expected_win32status = RPC_NT_UNKNOWN_IF; break;
        case RPC_S_NO_BINDINGS: expected_win32status = RPC_NT_NO_BINDINGS; break;
        case RPC_S_NO_PROTSEQS: expected_win32status = RPC_NT_NO_PROTSEQS; break;
        case RPC_S_CANT_CREATE_ENDPOINT: expected_win32status = RPC_NT_CANT_CREATE_ENDPOINT; break;
        case RPC_S_OUT_OF_RESOURCES: expected_win32status = RPC_NT_OUT_OF_RESOURCES; break;
        case RPC_S_SERVER_UNAVAILABLE: expected_win32status = RPC_NT_SERVER_UNAVAILABLE; break;
        case RPC_S_SERVER_TOO_BUSY: expected_win32status = RPC_NT_SERVER_TOO_BUSY; break;
        case RPC_S_INVALID_NETWORK_OPTIONS: expected_win32status = RPC_NT_INVALID_NETWORK_OPTIONS; break;
        case RPC_S_NO_CALL_ACTIVE: expected_win32status = RPC_NT_NO_CALL_ACTIVE; break;
        case RPC_S_CALL_FAILED: expected_win32status = RPC_NT_CALL_FAILED; break;
        case RPC_S_CALL_FAILED_DNE: expected_win32status = RPC_NT_CALL_FAILED_DNE; break;
        case RPC_S_PROTOCOL_ERROR: expected_win32status = RPC_NT_PROTOCOL_ERROR; break;
        case RPC_S_UNSUPPORTED_TRANS_SYN: expected_win32status = RPC_NT_UNSUPPORTED_TRANS_SYN; break;
        case RPC_S_UNSUPPORTED_TYPE: expected_win32status = RPC_NT_UNSUPPORTED_TYPE; break;
        case RPC_S_INVALID_TAG: expected_win32status = RPC_NT_INVALID_TAG; break;
        case RPC_S_INVALID_BOUND: expected_win32status = RPC_NT_INVALID_BOUND; break;
        case RPC_S_NO_ENTRY_NAME: expected_win32status = RPC_NT_NO_ENTRY_NAME; break;
        case RPC_S_INVALID_NAME_SYNTAX: expected_win32status = RPC_NT_INVALID_NAME_SYNTAX; break;
        case RPC_S_UNSUPPORTED_NAME_SYNTAX: expected_win32status = RPC_NT_UNSUPPORTED_NAME_SYNTAX; break;
        case RPC_S_UUID_NO_ADDRESS: expected_win32status = RPC_NT_UUID_NO_ADDRESS; break;
        case RPC_S_DUPLICATE_ENDPOINT: expected_win32status = RPC_NT_DUPLICATE_ENDPOINT; break;
        case RPC_S_UNKNOWN_AUTHN_TYPE: expected_win32status = RPC_NT_UNKNOWN_AUTHN_TYPE; break;
        case RPC_S_MAX_CALLS_TOO_SMALL: expected_win32status = RPC_NT_MAX_CALLS_TOO_SMALL; break;
        case RPC_S_STRING_TOO_LONG: expected_win32status = RPC_NT_STRING_TOO_LONG; break;
        case RPC_S_PROTSEQ_NOT_FOUND: expected_win32status = RPC_NT_PROTSEQ_NOT_FOUND; break;
        case RPC_S_PROCNUM_OUT_OF_RANGE: expected_win32status = RPC_NT_PROCNUM_OUT_OF_RANGE; break;
        case RPC_S_BINDING_HAS_NO_AUTH: expected_win32status = RPC_NT_BINDING_HAS_NO_AUTH; break;
        case RPC_S_UNKNOWN_AUTHN_SERVICE: expected_win32status = RPC_NT_UNKNOWN_AUTHN_SERVICE; break;
        case RPC_S_UNKNOWN_AUTHN_LEVEL: expected_win32status = RPC_NT_UNKNOWN_AUTHN_LEVEL; break;
        case RPC_S_INVALID_AUTH_IDENTITY: expected_win32status = RPC_NT_INVALID_AUTH_IDENTITY; break;
        case RPC_S_UNKNOWN_AUTHZ_SERVICE: expected_win32status = RPC_NT_UNKNOWN_AUTHZ_SERVICE; break;
        case EPT_S_INVALID_ENTRY: expected_win32status = EPT_NT_INVALID_ENTRY; break;
        case EPT_S_CANT_PERFORM_OP: expected_win32status = EPT_NT_CANT_PERFORM_OP; break;
        case EPT_S_NOT_REGISTERED: expected_win32status = EPT_NT_NOT_REGISTERED; break;
        case EPT_S_CANT_CREATE: expected_win32status = EPT_NT_CANT_CREATE; break;
        case RPC_S_NOTHING_TO_EXPORT: expected_win32status = RPC_NT_NOTHING_TO_EXPORT; break;
        case RPC_S_INCOMPLETE_NAME: expected_win32status = RPC_NT_INCOMPLETE_NAME; break;
        case RPC_S_INVALID_VERS_OPTION: expected_win32status = RPC_NT_INVALID_VERS_OPTION; break;
        case RPC_S_NO_MORE_MEMBERS: expected_win32status = RPC_NT_NO_MORE_MEMBERS; break;
        case RPC_S_NOT_ALL_OBJS_UNEXPORTED: expected_win32status = RPC_NT_NOT_ALL_OBJS_UNEXPORTED; break;
        case RPC_S_INTERFACE_NOT_FOUND: expected_win32status = RPC_NT_INTERFACE_NOT_FOUND; break;
        case RPC_S_ENTRY_ALREADY_EXISTS: expected_win32status = RPC_NT_ENTRY_ALREADY_EXISTS; break;
        case RPC_S_ENTRY_NOT_FOUND: expected_win32status = RPC_NT_ENTRY_NOT_FOUND; break;
        case RPC_S_NAME_SERVICE_UNAVAILABLE: expected_win32status = RPC_NT_NAME_SERVICE_UNAVAILABLE; break;
        case RPC_S_INVALID_NAF_ID: expected_win32status = RPC_NT_INVALID_NAF_ID; break;
        case RPC_S_CANNOT_SUPPORT: expected_win32status = RPC_NT_CANNOT_SUPPORT; break;
        case RPC_S_NO_CONTEXT_AVAILABLE: expected_win32status = RPC_NT_NO_CONTEXT_AVAILABLE; break;
        case RPC_S_INTERNAL_ERROR: expected_win32status = RPC_NT_INTERNAL_ERROR; break;
        case RPC_S_ZERO_DIVIDE: expected_win32status = RPC_NT_ZERO_DIVIDE; break;
        case RPC_S_ADDRESS_ERROR: expected_win32status = RPC_NT_ADDRESS_ERROR; break;
        case RPC_S_FP_DIV_ZERO: expected_win32status = RPC_NT_FP_DIV_ZERO; break;
        case RPC_S_FP_UNDERFLOW: expected_win32status = RPC_NT_FP_UNDERFLOW; break;
        case RPC_S_FP_OVERFLOW: expected_win32status = RPC_NT_FP_OVERFLOW; break;
        case RPC_S_CALL_IN_PROGRESS: expected_win32status = RPC_NT_CALL_IN_PROGRESS; break;
        case RPC_S_NO_MORE_BINDINGS: expected_win32status = RPC_NT_NO_MORE_BINDINGS; break;
        case RPC_S_CALL_CANCELLED: expected_win32status = RPC_NT_CALL_CANCELLED; missing = TRUE; break;
        case RPC_S_INVALID_OBJECT: expected_win32status = RPC_NT_INVALID_OBJECT; break;
        case RPC_S_INVALID_ASYNC_HANDLE: expected_win32status = RPC_NT_INVALID_ASYNC_HANDLE; missing = TRUE; break;
        case RPC_S_INVALID_ASYNC_CALL: expected_win32status = RPC_NT_INVALID_ASYNC_CALL; missing = TRUE; break;
        case RPC_S_GROUP_MEMBER_NOT_FOUND: expected_win32status = RPC_NT_GROUP_MEMBER_NOT_FOUND; break;
        case RPC_X_NO_MORE_ENTRIES: expected_win32status = RPC_NT_NO_MORE_ENTRIES; break;
        case RPC_X_SS_CHAR_TRANS_OPEN_FAIL: expected_win32status = RPC_NT_SS_CHAR_TRANS_OPEN_FAIL; break;
        case RPC_X_SS_CHAR_TRANS_SHORT_FILE: expected_win32status = RPC_NT_SS_CHAR_TRANS_SHORT_FILE; break;
        case RPC_X_SS_IN_NULL_CONTEXT: expected_win32status = RPC_NT_SS_IN_NULL_CONTEXT; break;
        case RPC_X_SS_CONTEXT_DAMAGED: expected_win32status = RPC_NT_SS_CONTEXT_DAMAGED; break;
        case RPC_X_SS_HANDLES_MISMATCH: expected_win32status = RPC_NT_SS_HANDLES_MISMATCH; break;
        case RPC_X_SS_CANNOT_GET_CALL_HANDLE: expected_win32status = RPC_NT_SS_CANNOT_GET_CALL_HANDLE; break;
        case RPC_X_NULL_REF_POINTER: expected_win32status = RPC_NT_NULL_REF_POINTER; break;
        case RPC_X_ENUM_VALUE_OUT_OF_RANGE: expected_win32status = RPC_NT_ENUM_VALUE_OUT_OF_RANGE; break;
        case RPC_X_BYTE_COUNT_TOO_SMALL: expected_win32status = RPC_NT_BYTE_COUNT_TOO_SMALL; break;
        case RPC_X_BAD_STUB_DATA: expected_win32status = RPC_NT_BAD_STUB_DATA; break;
        case RPC_X_PIPE_CLOSED: expected_win32status = RPC_NT_PIPE_CLOSED; missing = TRUE; break;
        case RPC_X_PIPE_DISCIPLINE_ERROR: expected_win32status = RPC_NT_PIPE_DISCIPLINE_ERROR; missing = TRUE; break;
        case RPC_X_PIPE_EMPTY: expected_win32status = RPC_NT_PIPE_EMPTY; missing = TRUE; break;
        case ERROR_PASSWORD_MUST_CHANGE: expected_win32status = STATUS_PASSWORD_MUST_CHANGE; missing = TRUE; break;
        case ERROR_ACCOUNT_LOCKED_OUT: expected_win32status = STATUS_ACCOUNT_LOCKED_OUT; missing = TRUE; break;
        default:
            if (w2k3_up)
                expected_win32status = STATUS_UNSUCCESSFUL;
            else
                expected_win32status = rpc_status;
        }

        ok(win32status == expected_win32status ||
            broken(missing && win32status == rpc_status),
            "I_RpcMapWin32Status(%d) should have returned 0x%x instead of 0x%x%s\n",
            rpc_status, expected_win32status, win32status,
            broken(missing) ? " (or have returned with the given status)" : "");
    }
}

static void test_RpcStringBindingParseA(void)
{
    static unsigned char valid_binding[] = "00000000-0000-0000-c000-000000000046@ncacn_np:.[endpoint=\\pipe\\test]";
    static unsigned char valid_binding2[] = "00000000-0000-0000-c000-000000000046@ncacn_np:.[\\pipe\\test]";
    static unsigned char invalid_uuid_binding[] = "{00000000-0000-0000-c000-000000000046}@ncacn_np:.[endpoint=\\pipe\\test]";
    static unsigned char invalid_ep_binding[] = "00000000-0000-0000-c000-000000000046@ncacn_np:.[endpoint=test]";
    static unsigned char invalid_binding[] = "00000000-0000-0000-c000-000000000046@ncacn_np";
    RPC_STATUS status;
    unsigned char *uuid;
    unsigned char *protseq;
    unsigned char *network_addr;
    unsigned char *endpoint;
    unsigned char *options;

    /* test all parameters */
    status = RpcStringBindingParseA(valid_binding, &uuid, &protseq, &network_addr, &endpoint, &options);
    ok(status == RPC_S_OK, "RpcStringBindingParseA failed with error %d\n", status);
    ok(!strcmp((char *)uuid, "00000000-0000-0000-c000-000000000046"), "uuid should have been 00000000-0000-0000-C000-000000000046 instead of %s\n", uuid);
    ok(!strcmp((char *)protseq, "ncacn_np"), "protseq should have been ncacn_np instead of %s\n", protseq);
    ok(!strcmp((char *)network_addr, "."), "network_addr should have been . instead of %s\n", network_addr);
    ok(!strcmp((char *)endpoint, "pipetest"), "endpoint should have been pipetest instead of %s\n", endpoint);
    if (options)
        ok(!strcmp((char *)options, ""), "options should have been \"\" of \"%s\"\n", options);
    else
        todo_wine ok(FALSE, "options is NULL\n");
    RpcStringFreeA(&uuid);
    RpcStringFreeA(&protseq);
    RpcStringFreeA(&network_addr);
    RpcStringFreeA(&endpoint);
    RpcStringFreeA(&options);

    /* test all parameters with different type of string binding */
    status = RpcStringBindingParseA(valid_binding2, &uuid, &protseq, &network_addr, &endpoint, &options);
    ok(status == RPC_S_OK, "RpcStringBindingParseA failed with error %d\n", status);
    ok(!strcmp((char *)uuid, "00000000-0000-0000-c000-000000000046"), "uuid should have been 00000000-0000-0000-C000-000000000046 instead of %s\n", uuid);
    ok(!strcmp((char *)protseq, "ncacn_np"), "protseq should have been ncacn_np instead of %s\n", protseq);
    ok(!strcmp((char *)network_addr, "."), "network_addr should have been . instead of %s\n", network_addr);
    ok(!strcmp((char *)endpoint, "pipetest"), "endpoint should have been pipetest instead of %s\n", endpoint);
    if (options)
        ok(!strcmp((char *)options, ""), "options should have been \"\" of \"%s\"\n", options);
    else
        todo_wine ok(FALSE, "options is NULL\n");
    RpcStringFreeA(&uuid);
    RpcStringFreeA(&protseq);
    RpcStringFreeA(&network_addr);
    RpcStringFreeA(&endpoint);
    RpcStringFreeA(&options);

    /* test with as many parameters NULL as possible */
    status = RpcStringBindingParseA(valid_binding, NULL, &protseq, NULL, NULL, NULL);
    ok(status == RPC_S_OK, "RpcStringBindingParseA failed with error %d\n", status);
    ok(!strcmp((char *)protseq, "ncacn_np"), "protseq should have been ncacn_np instead of %s\n", protseq);
    RpcStringFreeA(&protseq);

    /* test with invalid uuid */
    status = RpcStringBindingParseA(invalid_uuid_binding, NULL, &protseq, NULL, NULL, NULL);
    ok(status == RPC_S_INVALID_STRING_UUID, "RpcStringBindingParseA should have returned RPC_S_INVALID_STRING_UUID instead of %d\n", status);
    ok(protseq == NULL, "protseq was %p instead of NULL\n", protseq);

    /* test with invalid endpoint */
    status = RpcStringBindingParseA(invalid_ep_binding, NULL, &protseq, NULL, NULL, NULL);
    ok(status == RPC_S_OK, "RpcStringBindingParseA failed with error %d\n", status);
    RpcStringFreeA(&protseq);

    /* test with invalid binding */
    status = RpcStringBindingParseA(invalid_binding, &uuid, &protseq, &network_addr, &endpoint, &options);
    todo_wine
    ok(status == RPC_S_INVALID_STRING_BINDING, "RpcStringBindingParseA should have returned RPC_S_INVALID_STRING_BINDING instead of %d\n", status);
    todo_wine
    ok(uuid == NULL, "uuid was %p instead of NULL\n", uuid);
    if (uuid)
        RpcStringFreeA(&uuid);
    ok(protseq == NULL, "protseq was %p instead of NULL\n", protseq);
    todo_wine
    ok(network_addr == NULL, "network_addr was %p instead of NULL\n", network_addr);
    if (network_addr)
        RpcStringFreeA(&network_addr);
    ok(endpoint == NULL, "endpoint was %p instead of NULL\n", endpoint);
    ok(options == NULL, "options was %p instead of NULL\n", options);
}

static void test_I_RpcExceptionFilter(void)
{
    ULONG exception;
    int retval;
    int (WINAPI *pI_RpcExceptionFilter)(ULONG) = (void *)GetProcAddress(GetModuleHandleA("rpcrt4.dll"), "I_RpcExceptionFilter");

    if (!pI_RpcExceptionFilter)
    {
        win_skip("I_RpcExceptionFilter not exported\n");
        return;
    }

    for (exception = 0; exception < STATUS_REG_NAT_CONSUMPTION; exception++)
    {
        /* skip over uninteresting bits of the number space */
        if (exception == 2000) exception = 0x40000000;
        if (exception == 0x40000005) exception = 0x80000000;
        if (exception == 0x80000005) exception = 0xc0000000;

        retval = pI_RpcExceptionFilter(exception);
        switch (exception)
        {
        case STATUS_DATATYPE_MISALIGNMENT:
        case STATUS_BREAKPOINT:
        case STATUS_ACCESS_VIOLATION:
        case STATUS_ILLEGAL_INSTRUCTION:
        case STATUS_PRIVILEGED_INSTRUCTION:
        case STATUS_INSTRUCTION_MISALIGNMENT:
        case STATUS_STACK_OVERFLOW:
        case STATUS_POSSIBLE_DEADLOCK:
            ok(retval == EXCEPTION_CONTINUE_SEARCH, "I_RpcExceptionFilter(0x%x) should have returned %d instead of %d\n",
               exception, EXCEPTION_CONTINUE_SEARCH, retval);
            break;
        case STATUS_GUARD_PAGE_VIOLATION:
        case STATUS_IN_PAGE_ERROR:
        case STATUS_HANDLE_NOT_CLOSABLE:
            trace("I_RpcExceptionFilter(0x%x) returned %d\n", exception, retval);
            break;
        default:
            ok(retval == EXCEPTION_EXECUTE_HANDLER, "I_RpcExceptionFilter(0x%x) should have returned %d instead of %d\n",
               exception, EXCEPTION_EXECUTE_HANDLER, retval);
        }
    }
}

static void test_RpcStringBindingFromBinding(void)
{
    static unsigned char ncacn_np[] = "ncacn_np";
    static unsigned char address[] = ".";
    static unsigned char endpoint[] = "\\pipe\\wine_rpc_test";
    RPC_STATUS status;
    handle_t handle;
    RPC_CSTR binding;

    status = RpcStringBindingComposeA(NULL, ncacn_np, address,
                                     endpoint, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose failed (%u)\n", status);

    status = RpcBindingFromStringBindingA(binding, &handle);
    ok(status == RPC_S_OK, "RpcBindingFromStringBinding failed (%u)\n", status);
    RpcStringFreeA(&binding);

    status = RpcBindingToStringBindingA(handle, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingFromBinding failed with error %u\n", status);

    ok(!strcmp((const char *)binding, "ncacn_np:.[\\\\pipe\\\\wine_rpc_test]"),
       "binding string didn't match what was expected: \"%s\"\n", binding);
    RpcStringFreeA(&binding);

    status = RpcBindingFree(&handle);
    ok(status == RPC_S_OK, "RpcBindingFree failed with error %u\n", status);
}

static void test_UuidCreate(void)
{
    UUID guid;
    BYTE version;

    UuidCreate(&guid);
    version = (guid.Data3 & 0xf000) >> 12;
    ok(version == 4 || broken(version == 1), "unexpected version %d\n",
       version);
    if (version == 4)
    {
        static UUID v4and = { 0, 0, 0x4000, { 0x80,0,0,0,0,0,0,0 } };
        static UUID v4or = { 0xffffffff, 0xffff, 0x4fff,
           { 0xbf,0xff,0xff,0xff,0xff,0xff,0xff,0xff } };
        UUID and, or;
        RPC_STATUS rslt;
        int i;

        and = guid;
        or = guid;
        /* Generate a bunch of UUIDs and mask them.  By the end, we expect
         * every randomly generated bit to have been zero at least once,
         * resulting in no bits set in the and mask except those which are not
         * randomly generated:  the version number and the topmost bits of the
         * Data4 field (treated as big-endian.)  Similarly, we expect only
         * the bits which are not randomly set to be cleared in the or mask.
         */
        for (i = 0; i < 1000; i++)
        {
            LPBYTE src, dst;

            UuidCreate(&guid);
            for (src = (LPBYTE)&guid, dst = (LPBYTE)&and;
             src - (LPBYTE)&guid < sizeof(guid); src++, dst++)
                *dst &= *src;
            for (src = (LPBYTE)&guid, dst = (LPBYTE)&or;
             src - (LPBYTE)&guid < sizeof(guid); src++, dst++)
                *dst |= *src;
        }
        ok(UuidEqual(&and, &v4and, &rslt),
           "unexpected bits set in V4 UUID: %s\n", wine_dbgstr_guid(&and));
        ok(UuidEqual(&or, &v4or, &rslt),
           "unexpected bits set in V4 UUID: %s\n", wine_dbgstr_guid(&or));
    }
    else
    {
        /* Older versions of Windows generate V1 UUIDs.  For these, there are
         * many stable bits, including at least the MAC address if one is
         * present.  Just check that Data4[0]'s most significant bits are
         * set as expected.
         */
        ok((guid.Data4[0] & 0xc0) == 0x80,
           "unexpected value in Data4[0]: %02x\n", guid.Data4[0] & 0xc0);
    }
}

static void test_UuidCreateSequential(void)
{
    UUID guid1;
    BYTE version;
    RPC_STATUS (WINAPI *pUuidCreateSequential)(UUID *) = (void *)GetProcAddress(GetModuleHandleA("rpcrt4.dll"), "UuidCreateSequential");
    RPC_STATUS (WINAPI *pI_UuidCreate)(UUID *) = (void*)GetProcAddress(GetModuleHandleA("rpcrt4.dll"), "I_UuidCreate");
    RPC_STATUS ret;

    if (!pUuidCreateSequential)
    {
        win_skip("UuidCreateSequential not exported\n");
        return;
    }

    ok(pI_UuidCreate != pUuidCreateSequential, "got %p, %p\n", pI_UuidCreate, pUuidCreateSequential);

    ret = pUuidCreateSequential(&guid1);
    ok(!ret || ret == RPC_S_UUID_LOCAL_ONLY,
       "expected RPC_S_OK or RPC_S_UUID_LOCAL_ONLY, got %08x\n", ret);
    version = (guid1.Data3 & 0xf000) >> 12;
    ok(version == 1, "unexpected version %d\n", version);
    if (version == 1)
    {
        UUID guid2;

        if (!ret)
        {
            /* If the call succeeded, there's a valid (non-multicast) MAC
             * address in the uuid:
             */
            ok(!(guid1.Data4[2] & 0x01),
               "GUID does not appear to contain a MAC address: %s\n",
               wine_dbgstr_guid(&guid1));
        }
        else
        {
            /* Otherwise, there's a randomly generated multicast MAC address
             * address in the uuid:
             */
            ok((guid1.Data4[2] & 0x01),
               "GUID does not appear to contain a multicast MAC address: %s\n",
               wine_dbgstr_guid(&guid1));
        }
        /* Generate another GUID, and make sure its MAC address matches the
         * first.
         */
        ret = pUuidCreateSequential(&guid2);
        ok(!ret || ret == RPC_S_UUID_LOCAL_ONLY,
           "expected RPC_S_OK or RPC_S_UUID_LOCAL_ONLY, got %08x\n", ret);
        version = (guid2.Data3 & 0xf000) >> 12;
        ok(version == 1, "unexpected version %d\n", version);
        ok(!memcmp(guid1.Data4, guid2.Data4, sizeof(guid2.Data4)),
           "unexpected value in MAC address: %s\n",
           wine_dbgstr_guid(&guid2));

        /* I_UuidCreate does exactly the same */
        pI_UuidCreate(&guid2);
        version = (guid2.Data3 & 0xf000) >> 12;
        ok(version == 1, "unexpected version %d\n", version);
        ok(!memcmp(guid1.Data4, guid2.Data4, sizeof(guid2.Data4)),
           "unexpected value in MAC address: %s\n",
           wine_dbgstr_guid(&guid2));
    }
}

static void test_RpcBindingFree(void)
{
    RPC_BINDING_HANDLE binding = NULL;
    RPC_STATUS status;

    status = RpcBindingFree(&binding);
    ok(status == RPC_S_INVALID_BINDING,
       "RpcBindingFree should have returned RPC_S_INVALID_BINDING instead of %d\n",
       status);
}

static void test_RpcServerInqDefaultPrincName(void)
{
    RPC_STATUS ret;
    RPC_CSTR principal, saved_principal;
    BOOLEAN (WINAPI *pGetUserNameExA)(EXTENDED_NAME_FORMAT,LPSTR,PULONG);
    char *username;
    ULONG len = 0;

    pGetUserNameExA = (void *)GetProcAddress( LoadLibraryA("secur32.dll"), "GetUserNameExA" );
    if (!pGetUserNameExA)
    {
        win_skip( "GetUserNameExA not exported\n" );
        return;
    }
    pGetUserNameExA( NameSamCompatible, NULL, &len );
    username = HeapAlloc( GetProcessHeap(), 0, len );
    pGetUserNameExA( NameSamCompatible, username, &len );

    ret = RpcServerInqDefaultPrincNameA( 0, NULL );
    ok( ret == RPC_S_UNKNOWN_AUTHN_SERVICE, "got %u\n", ret );

    ret = RpcServerInqDefaultPrincNameA( RPC_C_AUTHN_DEFAULT, NULL );
    ok( ret == RPC_S_UNKNOWN_AUTHN_SERVICE, "got %u\n", ret );

    principal = (RPC_CSTR)0xdeadbeef;
    ret = RpcServerInqDefaultPrincNameA( RPC_C_AUTHN_DEFAULT, &principal );
    ok( ret == RPC_S_UNKNOWN_AUTHN_SERVICE, "got %u\n", ret );
    ok( principal == (RPC_CSTR)0xdeadbeef, "got unexpected principal\n" );

    saved_principal = (RPC_CSTR)0xdeadbeef;
    ret = RpcServerInqDefaultPrincNameA( RPC_C_AUTHN_WINNT, &saved_principal );
    ok( ret == RPC_S_OK, "got %u\n", ret );
    ok( saved_principal != (RPC_CSTR)0xdeadbeef, "expected valid principal\n" );
    ok( !strcmp( (const char *)saved_principal, username ), "got \'%s\'\n", saved_principal );
    trace("%s\n", saved_principal);

    ret = RpcServerRegisterAuthInfoA( (RPC_CSTR)"wine\\test", RPC_C_AUTHN_WINNT, NULL, NULL );
    ok( ret == RPC_S_OK, "got %u\n", ret );

    principal = (RPC_CSTR)0xdeadbeef;
    ret = RpcServerInqDefaultPrincNameA( RPC_C_AUTHN_WINNT, &principal );
    ok( ret == RPC_S_OK, "got %u\n", ret );
    ok( principal != (RPC_CSTR)0xdeadbeef, "expected valid principal\n" );
    ok( !strcmp( (const char *)principal, username ), "got \'%s\'\n", principal );
    RpcStringFreeA( &principal );

    ret = RpcServerRegisterAuthInfoA( saved_principal, RPC_C_AUTHN_WINNT, NULL, NULL );
    ok( ret == RPC_S_OK, "got %u\n", ret );

    RpcStringFreeA( &saved_principal );
    HeapFree( GetProcessHeap(), 0, username );
}

START_TEST( rpc )
{
    UuidConversionAndComparison();
    TestDceErrorInqText();
    test_rpc_ncacn_ip_tcp();
    test_towers();
    test_I_RpcMapWin32Status();
    test_RpcStringBindingParseA();
    test_I_RpcExceptionFilter();
    test_RpcStringBindingFromBinding();
    test_UuidCreate();
    test_UuidCreateSequential();
    test_RpcBindingFree();
    test_RpcServerInqDefaultPrincName();
}
