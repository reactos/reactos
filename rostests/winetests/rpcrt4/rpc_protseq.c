/*
 * RPC Protocol Sequence Server Tests
 *
 * Copyright 2008-2009 Robert Shearman
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

static void test_RpcServerUseProtseq(void)
{
    RPC_STATUS status;
    RPC_BINDING_VECTOR *bindings;
    ULONG i;
    ULONG binding_count_before;
    ULONG binding_count_after1;
    ULONG binding_count_after2;
    ULONG endpoints_registered = 0;
    static unsigned char iptcp[] = "ncacn_ip_tcp";
    static unsigned char np[] = "ncacn_np";
    static unsigned char ncalrpc[] = "ncalrpc";

    status = RpcServerInqBindings(&bindings);
    if (status == RPC_S_NO_BINDINGS)
        binding_count_before = 0;
    else
    {
        binding_count_before = bindings->Count;
        ok(status == RPC_S_OK, "RpcServerInqBindings failed with status %d\n", status);
        RpcBindingVectorFree(&bindings);
    }

    /* show that RpcServerUseProtseqEp(..., NULL, ...) is the same as
     * RpcServerUseProtseq(...) */
    status = RpcServerUseProtseqEpA(ncalrpc, 0, NULL, NULL);
    ok(status == RPC_S_OK || broken(status == RPC_S_INVALID_ENDPOINT_FORMAT),
       "RpcServerUseProtseqEp with NULL endpoint failed with status %d\n",
       status);

    /* register protocol sequences without explicit endpoints */
    status = RpcServerUseProtseqA(np, 0, NULL);
    if (status == RPC_S_PROTSEQ_NOT_SUPPORTED)
        win_skip("ncacn_np not supported\n");
    else
        ok(status == RPC_S_OK, "RpcServerUseProtseq(ncacn_np) failed with status %d\n", status);
    if (status == RPC_S_OK) endpoints_registered++;

    status = RpcServerUseProtseqA(iptcp, 0, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseq(ncacn_ip_tcp) failed with status %d\n", status);
    if (status == RPC_S_OK) endpoints_registered++;

    status = RpcServerUseProtseqA(ncalrpc, 0, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp(ncalrpc) failed with status %d\n", status);
    if (status == RPC_S_OK) endpoints_registered++;

    status = RpcServerInqBindings(&bindings);
    ok(status == RPC_S_OK, "RpcServerInqBindings failed with status %d\n", status);
    binding_count_after1 = bindings->Count;
    ok(binding_count_after1 == binding_count_before + endpoints_registered,
       "wrong binding count - before: %u, after %u, endpoints registered %u\n",
       binding_count_before, binding_count_after1, endpoints_registered);
    for (i = 0; i < bindings->Count; i++)
    {
        RPC_CSTR str_bind;
        status = RpcBindingToStringBindingA(bindings->BindingH[i], &str_bind);
        ok(status == RPC_S_OK, "RpcBindingToStringBinding failed with status %d\n", status);
        trace("string binding: %s\n", str_bind);
        RpcStringFreeA(&str_bind);
    }
    RpcBindingVectorFree(&bindings);

    /* re-register - endpoints should be reused */
    status = RpcServerUseProtseqA(np, 0, NULL);
    if (status == RPC_S_PROTSEQ_NOT_SUPPORTED)
        win_skip("ncacn_np not supported\n");
    else
        ok(status == RPC_S_OK, "RpcServerUseProtseq(ncacn_np) failed with status %d\n", status);

    status = RpcServerUseProtseqA(iptcp, 0, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseq(ncacn_ip_tcp) failed with status %d\n", status);

    status = RpcServerUseProtseqA(ncalrpc, 0, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp(ncalrpc) failed with status %d\n", status);

    status = RpcServerInqBindings(&bindings);
    ok(status == RPC_S_OK, "RpcServerInqBindings failed with status %d\n", status);
    binding_count_after2 = bindings->Count;
    ok(binding_count_after2 == binding_count_after1,
       "bindings should have been re-used - after1: %u after2: %u\n",
       binding_count_after1, binding_count_after2);
    RpcBindingVectorFree(&bindings);
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

static void test_endpoint_mapper(RPC_CSTR protseq, RPC_CSTR address)
{
    static unsigned char annotation[] = "Test annotation string.";
    RPC_STATUS status;
    RPC_BINDING_VECTOR *binding_vector;
    handle_t handle;
    unsigned char *binding;

    status = RpcServerRegisterIf(IFoo_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "%s: RpcServerRegisterIf failed (%u)\n", protseq, status);

    status = RpcServerInqBindings(&binding_vector);
    ok(status == RPC_S_OK, "%s: RpcServerInqBindings failed with error %u\n", protseq, status);

    /* register endpoints created in test_RpcServerUseProtseq */
    status = RpcEpRegisterA(IFoo_v0_0_s_ifspec, binding_vector, NULL, annotation);
    ok(status == RPC_S_OK, "%s: RpcEpRegisterA failed with error %u\n", protseq, status);
    /* reregister the same endpoint with no annotation */
    status = RpcEpRegisterA(IFoo_v0_0_s_ifspec, binding_vector, NULL, NULL);
    ok(status == RPC_S_OK, "%s: RpcEpRegisterA failed with error %u\n", protseq, status);

    status = RpcStringBindingComposeA(NULL, protseq, address,
                                     NULL, NULL, &binding);
    ok(status == RPC_S_OK, "%s: RpcStringBindingCompose failed (%u)\n", protseq, status);

    status = RpcBindingFromStringBindingA(binding, &handle);
    ok(status == RPC_S_OK, "%s: RpcBindingFromStringBinding failed (%u)\n", protseq, status);

    RpcStringFreeA(&binding);

    status = RpcBindingReset(handle);
    ok(status == RPC_S_OK, "%s: RpcBindingReset failed with error %u\n", protseq, status);

    status = RpcEpResolveBinding(handle, IFoo_v0_0_s_ifspec);
    ok(status == RPC_S_OK || broken(status == RPC_S_SERVER_UNAVAILABLE), /* win9x */
       "%s: RpcEpResolveBinding failed with error %u\n", protseq, status);

    status = RpcBindingReset(handle);
    ok(status == RPC_S_OK, "%s: RpcBindingReset failed with error %u\n", protseq, status);

    status = RpcBindingFree(&handle);
    ok(status == RPC_S_OK, "%s: RpcBindingFree failed with error %u\n", protseq, status);

    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
    ok(status == RPC_S_OK, "%s: RpcServerUnregisterIf failed (%u)\n", protseq, status);

    status = RpcEpUnregister(IFoo_v0_0_s_ifspec, binding_vector, NULL);
    ok(status == RPC_S_OK, "%s: RpcEpUnregisterA failed with error %u\n", protseq, status);

    status = RpcBindingVectorFree(&binding_vector);
    ok(status == RPC_S_OK, "%s: RpcBindingVectorFree failed with error %u\n", protseq, status);
}

START_TEST( rpc_protseq )
{
    static unsigned char ncacn_np[] = "ncacn_np";
    static unsigned char ncalrpc[] = "ncalrpc";
    static unsigned char np_address[] = ".";

    test_RpcServerUseProtseq();
    test_endpoint_mapper(ncacn_np, np_address);
    test_endpoint_mapper(ncalrpc, NULL);
}
