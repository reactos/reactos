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
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

  assert( pRpcMessage && pStubMsg && pStubDesc );

  pRpcMessage->Handle = NULL;
  pRpcMessage->ProcNum = ProcNum;
  pRpcMessage->RpcInterfaceInformation = pStubDesc->RpcInterfaceInformation;
  pRpcMessage->RpcFlags = 0;
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
  pStubMsg->fBufferValid = 0;
  pStubMsg->uFlags = 0;
  pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
  pStubMsg->pfnFree = pStubDesc->pfnFree;
  pStubMsg->StackTop = NULL;
  pStubMsg->StubDesc = pStubDesc;
  pStubMsg->FullPtrRefId = 0;
  pStubMsg->PointerLength = 0;
  pStubMsg->fInDontFree = 0;
  pStubMsg->fDontCallFreeInst = 0;
  pStubMsg->fInOnlyParam = 0;
  pStubMsg->fHasReturn = 0;
  pStubMsg->fHasExtensions = 0;
  pStubMsg->fHasNewCorrDesc = 0;
  pStubMsg->fUnused = 0;
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

  assert( pRpcMsg && pStubMsg && pStubDesc );

  /* not everyone allocates stack space for w2kReserved */
  memset(pStubMsg, 0, FIELD_OFFSET(MIDL_STUB_MESSAGE,pCSInfo));

  pStubMsg->ReuseBuffer = TRUE;
  pStubMsg->IsClient = FALSE;
  pStubMsg->StubDesc = pStubDesc;
  pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
  pStubMsg->pfnFree = pStubDesc->pfnFree;
  pStubMsg->RpcMsg = pRpcMsg;
  pStubMsg->Buffer = pStubMsg->BufferStart = pRpcMsg->Buffer;
  pStubMsg->BufferLength = pRpcMsg->BufferLength;
  pStubMsg->BufferEnd = pStubMsg->Buffer + pStubMsg->BufferLength;

  /* FIXME: determine the proper return value */
  return NULL;
}

/***********************************************************************
 *           NdrGetBuffer [RPCRT4.@]
 */
unsigned char *WINAPI NdrGetBuffer(PMIDL_STUB_MESSAGE stubmsg, ULONG buflen, RPC_BINDING_HANDLE handle)
{
  TRACE("(stubmsg == ^%p, buflen == %u, handle == %p): wild guess.\n", stubmsg, buflen, handle);
  
  assert( stubmsg && stubmsg->RpcMsg );

  /* I guess this is our chance to put the binding handle into the RPC_MESSAGE */
  stubmsg->RpcMsg->Handle = handle;
  
  stubmsg->RpcMsg->BufferLength = buflen;
  if (I_RpcGetBuffer(stubmsg->RpcMsg) != S_OK)
    return NULL;

  stubmsg->Buffer = stubmsg->BufferStart = stubmsg->RpcMsg->Buffer;
  stubmsg->BufferLength = stubmsg->RpcMsg->BufferLength;
  stubmsg->BufferEnd = stubmsg->Buffer + stubmsg->BufferLength;
  return (stubmsg->Buffer = (unsigned char *)stubmsg->RpcMsg->Buffer);
}
/***********************************************************************
 *           NdrFreeBuffer [RPCRT4.@]
 */
void WINAPI NdrFreeBuffer(PMIDL_STUB_MESSAGE pStubMsg)
{
  TRACE("(pStubMsg == ^%p): wild guess.\n", pStubMsg);
  I_RpcFreeBuffer(pStubMsg->RpcMsg);
  pStubMsg->BufferLength = 0;
  pStubMsg->Buffer = pStubMsg->BufferEnd = (unsigned char *)(pStubMsg->RpcMsg->Buffer = NULL);
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
    FIXME("(%p, %p, %p, %ld): stub\n", pStubMsg, pCommStatus, pFaultStatus, Status);

    *pCommStatus = 0;
    *pFaultStatus = 0;

    return RPC_S_OK;
}
