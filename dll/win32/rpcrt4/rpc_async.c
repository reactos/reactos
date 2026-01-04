/*
 * Asynchronous Call Support Functions
 *
 * Copyright 2007 Robert Shearman (for CodeWeavers)
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
 *
 */

#include <stdarg.h>

#include "rpc.h"
#include "rpcndr.h"
#include "rpcasync.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_message.h"
#include "ndr_stubless.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

#define RPC_ASYNC_SIGNATURE 0x43595341

static inline BOOL valid_async_handle(PRPC_ASYNC_STATE pAsync)
{
    return pAsync->Signature == RPC_ASYNC_SIGNATURE;
}

/***********************************************************************
 *           RpcAsyncInitializeHandle [RPCRT4.@]
 *
 * Initialises an asynchronous state so it can be used in other asynchronous
 * functions and for use in asynchronous calls.
 *
 * PARAMS
 *  pAsync [I] Asynchronous state to initialise.
 *  Size   [I] Size of the memory pointed to by pAsync.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI RpcAsyncInitializeHandle(PRPC_ASYNC_STATE pAsync, unsigned int Size)
{
    TRACE("(%p, %d)\n", pAsync, Size);

    if (Size != sizeof(*pAsync))
    {
        ERR("invalid Size %d\n", Size);
        return ERROR_INVALID_PARAMETER;
    }

    pAsync->Size = sizeof(*pAsync);
    pAsync->Signature = RPC_ASYNC_SIGNATURE;
    pAsync->Lock = 0;
    pAsync->Flags = 0;
    pAsync->StubInfo = NULL;
    pAsync->RuntimeInfo = NULL;
    memset(pAsync->Reserved, 0, sizeof(*pAsync) - FIELD_OFFSET(RPC_ASYNC_STATE, Reserved));

    return RPC_S_OK;
}

/***********************************************************************
 *           RpcAsyncGetCallStatus [RPCRT4.@]
 *
 * Retrieves the current status of the asynchronous call taking place.
 *
 * PARAMS
 *  pAsync [I] Asynchronous state to initialise.
 *
 * RETURNS
 *  RPC_S_OK - The call was successfully completed.
 *  RPC_S_INVALID_ASYNC_HANDLE - The asynchronous structure is not valid.
 *  RPC_S_ASYNC_CALL_PENDING - The call is still in progress and has not been completed.
 *  Any other error code - The call failed.
 */
RPC_STATUS WINAPI RpcAsyncGetCallStatus(PRPC_ASYNC_STATE pAsync)
{
    FIXME("(%p): stub\n", pAsync);
    return RPC_S_INVALID_ASYNC_HANDLE;
}

/***********************************************************************
 *           RpcAsyncCompleteCall [RPCRT4.@]
 *
 * Completes a client or server asynchronous call.
 *
 * PARAMS
 *  pAsync [I] Asynchronous state to initialise.
 *  Reply  [I] The return value of the asynchronous function.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI RpcAsyncCompleteCall(PRPC_ASYNC_STATE pAsync, void *Reply)
{
    struct async_call_data *data;

    TRACE("(%p, %p)\n", pAsync, Reply);

    if (!valid_async_handle(pAsync))
        return RPC_S_INVALID_ASYNC_HANDLE;

    /* FIXME: check completed */

    TRACE("pAsync %p, pAsync->StubInfo %p\n", pAsync, pAsync->StubInfo);

    data = pAsync->StubInfo;
    if (data->pStubMsg->IsClient)
        return NdrpCompleteAsyncClientCall(pAsync, Reply);

    return NdrpCompleteAsyncServerCall(pAsync, Reply);
}

/***********************************************************************
 *           RpcAsyncAbortCall [RPCRT4.@]
 *
 * Aborts the asynchronous server call taking place.
 *
 * PARAMS
 *  pAsync        [I] Asynchronous server state to abort.
 *  ExceptionCode [I] Exception code to return to the client in a fault packet.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI RpcAsyncAbortCall(PRPC_ASYNC_STATE pAsync, ULONG ExceptionCode)
{
    FIXME("(%p, %ld/0x%lx): stub\n", pAsync, ExceptionCode, ExceptionCode);
    return RPC_S_INVALID_ASYNC_HANDLE;
}

/***********************************************************************
 *           RpcAsyncCancelCall [RPCRT4.@]
 *
 * Cancels the asynchronous client call taking place.
 *
 * PARAMS
 *  pAsync        [I] Asynchronous client state to abort.
 *  fAbortCall    [I] If TRUE, then send a cancel to the server, otherwise
 *                    just wait for the call to complete.
 *
 * RETURNS
 *  Success: RPC_S_OK.
 *  Failure: Any error code.
 */
RPC_STATUS WINAPI RpcAsyncCancelCall(PRPC_ASYNC_STATE pAsync, BOOL fAbortCall)
{
    FIXME("(%p, %s): stub\n", pAsync, fAbortCall ? "TRUE" : "FALSE");
    return RPC_S_INVALID_ASYNC_HANDLE;
}
