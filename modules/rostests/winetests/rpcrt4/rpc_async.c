/*
 * Unit test suite for rpc functions
 *
 * Copyright 2008 Robert Shearman (for CodeWeavers)
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

#include "wine/test.h"

#include <rpc.h>
#include <rpcasync.h>

static void test_RpcAsyncInitializeHandle(void)
{
    char buffer[256];
    RPC_ASYNC_STATE async;
    RPC_STATUS status;
    int i;
    void *unset_ptr;

    status = RpcAsyncInitializeHandle((PRPC_ASYNC_STATE)buffer, sizeof(buffer));
    ok(status == ERROR_INVALID_PARAMETER, "RpcAsyncInitializeHandle with large Size should have returned ERROR_INVALID_PARAMETER instead of %ld\n", status);

    status = RpcAsyncInitializeHandle(&async, sizeof(async) - 1);
    ok(status == ERROR_INVALID_PARAMETER, "RpcAsyncInitializeHandle with small Size should have returned ERROR_INVALID_PARAMETER instead of %ld\n", status);

    memset(&async, 0xcc, sizeof(async));
    memset(&unset_ptr, 0xcc, sizeof(unset_ptr));
    status = RpcAsyncInitializeHandle(&async, sizeof(async));
    ok(status == RPC_S_OK, "RpcAsyncInitializeHandle failed with error %ld\n", status);

    ok(async.Size == sizeof(async), "async.Size wrong: %d\n", async.Size);
    ok(async.Signature == 0x43595341, "async.Signature should be 0x43595341, but is 0x%lx instead\n", async.Signature);
    ok(async.Lock == 0, "async.Lock should be 0, but is %ld instead\n", async.Lock);
    ok(async.Flags == 0, "async.Flags should be 0, but is %ld instead\n", async.Flags);
    ok(async.StubInfo == NULL, "async.StubInfo should be NULL, not %p\n", async.StubInfo);
    ok(async.UserInfo == unset_ptr, "async.UserInfo should be unset, not %p\n", async.UserInfo);
    ok(async.RuntimeInfo == NULL, "async.RuntimeInfo should be NULL, not %p\n", async.RuntimeInfo);
    ok(async.Event == 0xcccccccc, "async.Event should be unset, not %d\n", async.Event);
    ok(async.NotificationType == 0xcccccccc, "async.NotificationType should be unset, not %d\n", async.NotificationType);
    for (i = 0; i < 4; i++)
        ok(async.Reserved[i] == 0x0, "async.Reserved[%d] should be 0x0, not 0x%Ix\n", i, async.Reserved[i]);
}

static void test_RpcAsyncGetCallStatus(void)
{
    RPC_ASYNC_STATE async;
    RPC_STATUS status;

    status = RpcAsyncInitializeHandle(&async, sizeof(async));
    ok(status == RPC_S_OK, "RpcAsyncInitializeHandle failed with error %ld\n", status);

    status = RpcAsyncGetCallStatus(&async);
    todo_wine
    ok(status == RPC_S_INVALID_BINDING, "RpcAsyncGetCallStatus should have returned RPC_S_INVALID_BINDING instead of %ld\n", status);

    memset(&async, 0, sizeof(async));
    status = RpcAsyncGetCallStatus(&async);
    todo_wine
    ok(status == RPC_S_INVALID_BINDING, "RpcAsyncGetCallStatus should have returned RPC_S_INVALID_BINDING instead of %ld\n", status);
}

START_TEST( rpc_async )
{
    test_RpcAsyncInitializeHandle();
    test_RpcAsyncGetCallStatus();
}
