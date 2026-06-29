/*
 * MIDL proxy/stub stuff
 *
 * Copyright 2002 Ove Kåven, TransGaming Technologies
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
 * TODO:
 *  - figure out whether we *really* got this right
 *  - check for errors and throw exceptions
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "objbase.h"

#include "rpcproxy.h"

#include "wine/debug.h"

#include "ndr_misc.h"
#include "rpcndr.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

/************************************************************************
 *             NdrClientInitializeNew [RPCRT4.@]
 */
void WINAPI NdrClientInitializeNew( PRPC_MESSAGE pRpcMessage, PMIDL_STUB_MESSAGE pStubMsg, 
                                    PMIDL_STUB_DESC pStubDesc, unsigned int ProcNum )
{
  TRACE("(pRpcMessage == ^%p, pStubMsg == ^%p, pStubDesc == ^%p, ProcNum == %d)\n",
    pRpcMessage, pStubMsg, pStubDesc, ProcNum);

  pRpcMessage->Handle = NULL;
  pRpcMessage->ProcNum = ProcNum | RPC_FLAGS_VALID_BIT;
  pRpcMessage->RpcInterfaceInformation = pStubDesc->RpcInterfaceInformation;
  pRpcMessage->RpcFlags = 0;
  pRpcMessage->ReservedForRuntime = NULL;
  pRpcMessage->DataRepresentation = NDR_LOCAL_DATA_REPRESENTATION;

  pStubMsg->RpcMsg = pRpcMessage;
  pStubMsg->BufferStart = NULL;
  pStubMsg->BufferEnd = NULL;
  pStubMsg->BufferLength = 0;
  pStubMsg->IsClient = TRUE;
  pStubMsg->ReuseBuffer = FALSE;
  pStubMsg->pAllocAllNodesContext = NULL;
  pStubMsg->pPointerQueueState = NULL;
  pStubMsg->IgnoreEmbeddedPointers = 0;
  pStubMsg->PointerBufferMark = NULL;
  pStubMsg->CorrDespIncrement = 0;
  pStubMsg->uFlags = 0;
  pStubMsg->UniquePtrCount = 0;
  pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
  pStubMsg->pfnFree = pStubDesc->pfnFree;
  pStubMsg->StackTop = NULL;
  pStubMsg->StubDesc = pStubDesc;
  pStubMsg->FullPtrRefId = 0;
  pStubMsg->PointerLength = 0;
  pStubMsg->fInDontFree = 0;
  pStubMsg->fDontCallFreeInst = 0;
  pStubMsg->fHasReturn = 0;
  pStubMsg->fHasExtensions = 0;
  pStubMsg->fHasNewCorrDesc = 0;
  pStubMsg->fIsIn = 0;
  pStubMsg->fIsOut = 0;
  pStubMsg->fIsOicf = 0;
  pStubMsg->fBufferValid = 0;
  pStubMsg->fHasMemoryValidateCallback = 0;
  pStubMsg->fInFree = 0;
  pStubMsg->fNeedMCCP = 0;
  pStubMsg->fUnused2 = 0;
  pStubMsg->dwDestContext = MSHCTX_DIFFERENTMACHINE;
  pStubMsg->pvDestContext = NULL;
  pStubMsg->pRpcChannelBuffer = NULL;
  pStubMsg->pArrayInfo = NULL;
  pStubMsg->dwStubPhase = 0;
  /* FIXME: LowStackMark */
  pStubMsg->pAsyncMsg = NULL;
  pStubMsg->pCorrInfo = NULL;
  pStubMsg->pCorrMemory = NULL;
  pStubMsg->pMemoryList = NULL;
}

/***********************************************************************
 *             NdrServerInitializeNew [RPCRT4.@]
 */
unsigned char* WINAPI NdrServerInitializeNew( PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg,
                                              PMIDL_STUB_DESC pStubDesc )
{
  TRACE("(pRpcMsg == ^%p, pStubMsg == ^%p, pStubDesc == ^%p)\n", pRpcMsg, pStubMsg, pStubDesc);

  pStubMsg->RpcMsg = pRpcMsg;
  pStubMsg->Buffer = pStubMsg->BufferStart = pRpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->Buffer + pRpcMsg->BufferLength;
  pStubMsg->BufferLength = pRpcMsg->BufferLength;
  pStubMsg->IsClient = FALSE;
  pStubMsg->ReuseBuffer = FALSE;
  pStubMsg->pAllocAllNodesContext = NULL;
  pStubMsg->pPointerQueueState = NULL;
  pStubMsg->IgnoreEmbeddedPointers = 0;
  pStubMsg->PointerBufferMark = NULL;
  pStubMsg->CorrDespIncrement = 0;
  pStubMsg->uFlags = 0;
  pStubMsg->UniquePtrCount = 0;
  pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
  pStubMsg->pfnFree = pStubDesc->pfnFree;
  pStubMsg->StackTop = NULL;
  pStubMsg->StubDesc = pStubDesc;
  pStubMsg->FullPtrXlatTables = NULL;
  pStubMsg->FullPtrRefId = 0;
  pStubMsg->PointerLength = 0;
  pStubMsg->fInDontFree = 0;
  pStubMsg->fDontCallFreeInst = 0;
  pStubMsg->fHasReturn = 0;
  pStubMsg->fHasExtensions = 0;
  pStubMsg->fHasNewCorrDesc = 0;
  pStubMsg->fIsIn = 0;
  pStubMsg->fIsOut = 0;
  pStubMsg->fIsOicf = 0;
  pStubMsg->fHasMemoryValidateCallback = 0;
  pStubMsg->fInFree = 0;
  pStubMsg->fNeedMCCP = 0;
  pStubMsg->fUnused2 = 0;
  pStubMsg->dwDestContext = MSHCTX_DIFFERENTMACHINE;
  pStubMsg->pvDestContext = NULL;
  pStubMsg->pRpcChannelBuffer = NULL;
  pStubMsg->pArrayInfo = NULL;
  pStubMsg->dwStubPhase = 0;
  /* FIXME: LowStackMark */
  pStubMsg->pAsyncMsg = NULL;
  pStubMsg->pCorrInfo = NULL;
  pStubMsg->pCorrMemory = NULL;
  pStubMsg->pMemoryList = NULL;

  return NULL;
}

/***********************************************************************
 *           NdrGetBuffer [RPCRT4.@]
 */
unsigned char *WINAPI NdrGetBuffer(PMIDL_STUB_MESSAGE stubmsg, ULONG buflen, RPC_BINDING_HANDLE handle)
{
  RPC_STATUS status;

  TRACE("(stubmsg == ^%p, buflen == %lu, handle == %p)\n", stubmsg, buflen, handle);
  
  stubmsg->RpcMsg->Handle = handle;
  stubmsg->RpcMsg->BufferLength = buflen;

  status = I_RpcGetBuffer(stubmsg->RpcMsg);
  if (status != RPC_S_OK)
    RpcRaiseException(status);

  stubmsg->Buffer = stubmsg->RpcMsg->Buffer;
  stubmsg->fBufferValid = TRUE;
  stubmsg->BufferLength = stubmsg->RpcMsg->BufferLength;
  return stubmsg->Buffer;
}
/***********************************************************************
 *           NdrFreeBuffer [RPCRT4.@]
 */
void WINAPI NdrFreeBuffer(PMIDL_STUB_MESSAGE pStubMsg)
{
  TRACE("(pStubMsg == ^%p)\n", pStubMsg);
  if (pStubMsg->fBufferValid)
  {
    I_RpcFreeBuffer(pStubMsg->RpcMsg);
    pStubMsg->fBufferValid = FALSE;
  }
}

/************************************************************************
 *           NdrSendReceive [RPCRT4.@]
 */
unsigned char *WINAPI NdrSendReceive( PMIDL_STUB_MESSAGE stubmsg, unsigned char *buffer  )
{
  RPC_STATUS status;

  TRACE("(stubmsg == ^%p, buffer == ^%p)\n", stubmsg, buffer);

  /* FIXME: how to handle errors? (raise exception?) */
  if (!stubmsg) {
    ERR("NULL stub message.  No action taken.\n");
    return NULL;
  }
  if (!stubmsg->RpcMsg) {
    ERR("RPC Message not present in stub message.  No action taken.\n");
    return NULL;
  }

  /* avoid sending uninitialised parts of the buffer on the wire */
  stubmsg->RpcMsg->BufferLength = buffer - (unsigned char *)stubmsg->RpcMsg->Buffer;
  status = I_RpcSendReceive(stubmsg->RpcMsg);
  if (status != RPC_S_OK)
    RpcRaiseException(status);

  stubmsg->BufferLength = stubmsg->RpcMsg->BufferLength;
  stubmsg->BufferStart = stubmsg->RpcMsg->Buffer;
  stubmsg->BufferEnd = stubmsg->BufferStart + stubmsg->BufferLength;
  stubmsg->Buffer = stubmsg->BufferStart;

  /* FIXME: is this the right return value? */
  return NULL;
}

/************************************************************************
 *           NdrMapCommAndFaultStatus [RPCRT4.@]
 */
RPC_STATUS RPC_ENTRY NdrMapCommAndFaultStatus( PMIDL_STUB_MESSAGE pStubMsg,
                                               ULONG *pCommStatus,
                                               ULONG *pFaultStatus,
                                               RPC_STATUS Status )
{
    TRACE("(%p, %p, %p, %ld)\n", pStubMsg, pCommStatus, pFaultStatus, Status);

    switch (Status)
    {
    case ERROR_INVALID_HANDLE:
    case RPC_S_INVALID_BINDING:
    case RPC_S_UNKNOWN_IF:
    case RPC_S_SERVER_UNAVAILABLE:
    case RPC_S_SERVER_TOO_BUSY:
    case RPC_S_CALL_FAILED_DNE:
    case RPC_S_PROTOCOL_ERROR:
    case RPC_S_UNSUPPORTED_TRANS_SYN:
    case RPC_S_UNSUPPORTED_TYPE:
    case RPC_S_PROCNUM_OUT_OF_RANGE:
    case EPT_S_NOT_REGISTERED:
    case RPC_S_COMM_FAILURE:
        *pCommStatus = Status;
        *pFaultStatus = 0;
        break;
    default:
        *pCommStatus = 0;
        *pFaultStatus = Status;
    }

    return RPC_S_OK;
}
