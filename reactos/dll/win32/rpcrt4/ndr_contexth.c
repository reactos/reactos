/*
 * Context Handle Functions
 *
 * Copyright 2006 Saveliy Tretiakov
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

#include "rpc.h"
#include "rpcndr.h"
#include "wine/debug.h"

#include "rpc_binding.h"
#include "ndr_contexth.h"


/***********************************************************************
 *           NDRCContextBinding
 */
RPC_BINDING_HANDLE WINAPI NDRCContextBinding(NDR_CCONTEXT CContext)
{
    if(!CContext)
		RpcRaiseException(ERROR_INVALID_HANDLE);

    return (RPC_BINDING_HANDLE)((CContextHandle*)CContext)->Binding;
}

/***********************************************************************
 *           NDRCContextMarshall
 */
void WINAPI NDRCContextMarshall(NDR_CCONTEXT CContext, void *pBuff )
{
	CContextHandle *ctx = (CContextHandle*)CContext;
	memcpy(pBuff, &ctx->Ndr, sizeof(ContextHandleNdr));
}

/***********************************************************************
 *           NdrClientContextMarshall
 */
void WINAPI NdrClientContextMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                     NDR_CCONTEXT ContextHandle,
                                     int fCheck)
{
   
	if(!ContextHandle) 
		RpcRaiseException(ERROR_INVALID_HANDLE);

	NDRCContextMarshall(ContextHandle, pStubMsg->Buffer);
	
	pStubMsg->Buffer += sizeof(ContextHandleNdr);

}

/***********************************************************************
 *           NDRCContextUnmarshall
 */
void WINAPI NDRCContextUnmarshall(NDR_CCONTEXT *pCContext, 
                                  RPC_BINDING_HANDLE hBinding,
                                  void *pBuff, 
                                  unsigned long DataRepresentation )
{
	CContextHandle *ctx = (CContextHandle*)*pCContext;
	ContextHandleNdr *ndr = (ContextHandleNdr*)pBuff;
	RPC_STATUS status;

	if(UuidIsNil(&ndr->uuid, &status)) 
	{
		if(ctx)
		{
			RPCRT4_DestroyBinding(ctx->Binding);
			HeapFree(GetProcessHeap(), 0, ctx);
		}
		*pCContext = NULL;
	}
	else 
	{
		ctx = HeapAlloc(GetProcessHeap(), 0, sizeof(CContextHandle));
		if(!ctx) RpcRaiseException(ERROR_OUTOFMEMORY);

		status = RpcBindingCopy(hBinding, (RPC_BINDING_HANDLE*) &ctx->Binding);
		if(status != RPC_S_OK) RpcRaiseException(status);
		
		memcpy(&ctx->Ndr, ndr, sizeof(ContextHandleNdr));
		*pCContext = (NDR_CCONTEXT)ctx;
	}
}

/***********************************************************************
 *           NdrClientContextUnmarshall
 */
void WINAPI NdrClientContextUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                       NDR_CCONTEXT * pContextHandle,
                                       RPC_BINDING_HANDLE BindHandle)
{
	if(!pContextHandle) 
		RpcRaiseException(ERROR_INVALID_HANDLE);

	NDRCContextUnmarshall(pContextHandle,
		((CContextHandle*)pContextHandle)->Binding,
		pStubMsg->Buffer, 
		pStubMsg->RpcMsg->DataRepresentation);
	
	pStubMsg->Buffer += sizeof(ContextHandleNdr);
}

/***********************************************************************
 *           RpcSmDestroyClientContext
 */
RPC_STATUS WINAPI RpcSmDestroyClientContext(void** ContextHandle)
{
	CContextHandle *ctx = (CContextHandle*)ContextHandle;

	if(!ctx)
		return RPC_X_SS_CONTEXT_MISMATCH;

	RPCRT4_DestroyBinding(ctx->Binding);
	HeapFree(GetProcessHeap(), 0, ctx);
	*ContextHandle = NULL;

	return RPC_S_OK;
}

/***********************************************************************
 *           RpcSsDestroyClientContext
 */
void WINAPI RpcSsDestroyClientContext(void** ContextHandle)
{
	RPC_STATUS status;

	status = RpcSmDestroyClientContext(ContextHandle);

	if(status != RPC_S_OK) 
		RpcRaiseException(status);
}

/***********************************************************************
 *           NdrServerContextMarshall
 */
void WINAPI NdrServerContextMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                     NDR_SCONTEXT ContextHandle,
                                     NDR_RUNDOWN RundownRoutine )
{
    FIXME("(%p, %p, %p): stub\n", pStubMsg, ContextHandle, RundownRoutine);
}

/***********************************************************************
 *           NdrServerContextUnmarshall
 */
NDR_SCONTEXT WINAPI NdrServerContextUnmarshall(PMIDL_STUB_MESSAGE pStubMsg)
{
    FIXME("(%p): stub\n", pStubMsg);
    return NULL;
}

/***********************************************************************
 *           NdrContextHandleSize
 */
void WINAPI NdrContextHandleSize(PMIDL_STUB_MESSAGE pStubMsg,
                                 unsigned char* pMemory,
                                 PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p, %p): stub\n", pStubMsg, pMemory, pFormat);
}

/***********************************************************************
 *           NdrContextHandleInitialize
 */
NDR_SCONTEXT WINAPI NdrContextHandleInitialize(PMIDL_STUB_MESSAGE pStubMsg,
                                               PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p): stub\n", pStubMsg, pFormat);
    return NULL;
}

/***********************************************************************
 *           NdrServerContextNewMarshall
 */
void WINAPI NdrServerContextNewMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                        NDR_SCONTEXT ContextHandle,
                                        NDR_RUNDOWN RundownRoutine,
                                        PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p, %p, %p): stub\n", pStubMsg, ContextHandle, RundownRoutine, pFormat);
}

/***********************************************************************
 *           NdrServerContextNewUnmarshall
 */
NDR_SCONTEXT WINAPI NdrServerContextNewUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  PFORMAT_STRING pFormat)
{
    FIXME("(%p, %p): stub\n", pStubMsg, pFormat);
    return NULL;
}
