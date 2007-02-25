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

static SContextHandle *CtxList = NULL;

static CRITICAL_SECTION CtxList_cs;
static CRITICAL_SECTION_DEBUG CtxList_cs_debug =
{
    0, 0, &CtxList_cs,
    { &CtxList_cs_debug.ProcessLocksList, &CtxList_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": CtxList_cs") }
};
static CRITICAL_SECTION CtxList_cs = { &CtxList_cs_debug, -1, 0, 0, 0, 0 };

SContextHandle* RPCRT4_SrvAllocCtxHdl()
{
  SContextHandle *Hdl, *Tmp;
  
  if((Hdl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
        sizeof(SContextHandle)))==NULL) return NULL;
  
  EnterCriticalSection(&CtxList_cs);
  
  if(!CtxList) CtxList = Hdl;
  else 
  {
    for(Tmp = CtxList; Tmp->Next; Tmp = Tmp->Next);
    Tmp->Next = Hdl;
    Hdl->Prev = Tmp;
  }
  
  LeaveCriticalSection(&CtxList_cs);
  
  return Hdl;
}

void RPCRT4_SrvFreeCtxHdl(SContextHandle *Hdl)
{
  EnterCriticalSection(&CtxList_cs);
  
  if(Hdl->Prev && Hdl->Next) 
  {
    ((SContextHandle*)Hdl->Prev)->Next = Hdl->Next;
    ((SContextHandle*)Hdl->Next)->Prev = Hdl->Prev;
  }
  else if(Hdl->Next)
  {
    ((SContextHandle*)Hdl->Next)->Prev = NULL;
    CtxList = (SContextHandle*)Hdl->Next;
  }
  else if(Hdl->Prev)
    ((SContextHandle*)Hdl->Prev)->Next = NULL;
  else CtxList = NULL;
  
  HeapFree(GetProcessHeap(), 0, Hdl);
  LeaveCriticalSection(&CtxList_cs);
}

SContextHandle* RPCRT4_SrvFindCtxHdl(ContextHandleNdr *CtxNdr)
{
  SContextHandle *Hdl, *Ret=NULL;
  RPC_STATUS status;
  EnterCriticalSection(&CtxList_cs);
  
  for(Hdl = CtxList; Hdl; Hdl = Hdl->Next)
    if(UuidCompare(&Hdl->Ndr.uuid, &CtxNdr->uuid, &status)==0)
    {
      Ret = Hdl;
      break;
    }
    
  LeaveCriticalSection(&CtxList_cs);
  return Ret;
}

SContextHandle* RPCRT4_SrvUnmarshallCtxHdl(ContextHandleNdr *Ndr)
{
  SContextHandle *Hdl = NULL;
  RPC_STATUS status;
  
  if(!Ndr)
  {
    if((Hdl = RPCRT4_SrvAllocCtxHdl())==NULL)
      RpcRaiseException(ERROR_OUTOFMEMORY);
    
    UuidCreate(&Hdl->Ndr.uuid);
  }
  else if(!UuidIsNil(&Ndr->uuid, &status))  
    Hdl = RPCRT4_SrvFindCtxHdl(Ndr);
  
  return Hdl;
}

void RPCRT4_SrvMarshallCtxHdl(SContextHandle *Hdl, ContextHandleNdr *Ndr)
{
  if(!Hdl->Value)
  {
    RPCRT4_SrvFreeCtxHdl(Hdl);
    ZeroMemory(Ndr, sizeof(ContextHandleNdr));
  }
  else memcpy(Ndr, &Hdl->Ndr, sizeof(ContextHandleNdr));
}

void RPCRT4_DoContextRundownIfNeeded(RpcConnection *Conn)
{

}

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

  if(!pContextHandle || !BindHandle) 
    RpcRaiseException(ERROR_INVALID_HANDLE);

  NDRCContextUnmarshall(pContextHandle,
  (CContextHandle*)BindHandle,
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
 *           NdrServerContextMarshall
 */
void WINAPI NdrServerContextMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                     NDR_SCONTEXT ContextHandle,
                                     NDR_RUNDOWN RundownRoutine)
{
  SContextHandle *Hdl;
  RpcBinding *Binding;
  
  if(!ContextHandle) 
    RpcRaiseException(ERROR_INVALID_HANDLE);

  Hdl = (SContextHandle*)ContextHandle;
  Binding = (RpcBinding*)pStubMsg->RpcMsg->Handle;
  Hdl->Rundown = RundownRoutine;
  Hdl->Conn = Binding->FromConn;
  RPCRT4_SrvMarshallCtxHdl(Hdl, (ContextHandleNdr*)pStubMsg->Buffer);
  pStubMsg->Buffer += sizeof(ContextHandleNdr);
}

/***********************************************************************
 *           NdrServerContextUnmarshall
 */
NDR_SCONTEXT WINAPI NdrServerContextUnmarshall(PMIDL_STUB_MESSAGE pStubMsg)
{
  SContextHandle *Hdl;

  Hdl = RPCRT4_SrvUnmarshallCtxHdl((ContextHandleNdr*)pStubMsg->Buffer);
  return (NDR_SCONTEXT)Hdl;
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

/***********************************************************************
 *           NDRSContextMarshall
 */
void WINAPI NDRSContextMarshall(IN NDR_SCONTEXT CContext,
                                OUT void *pBuff,
                                IN NDR_RUNDOWN userRunDownIn)
{
  SContextHandle *Hdl;
  
  if(!CContext) 
    RpcRaiseException(ERROR_INVALID_HANDLE);
    
  Hdl = (SContextHandle*)CContext;
  Hdl->Rundown = userRunDownIn;
  RPCRT4_SrvMarshallCtxHdl(Hdl, (ContextHandleNdr*)pBuff);
}

/***********************************************************************
 *           NDRSContextUnmarshall
 */
NDR_SCONTEXT WINAPI NDRSContextUnmarshall(IN void *pBuff,
                                          IN unsigned long DataRepresentation)
{
  return (NDR_SCONTEXT) RPCRT4_SrvUnmarshallCtxHdl((ContextHandleNdr*)pBuff);
}

/***********************************************************************
 *           NDRSContextMarshallEx
 */
void WINAPI NDRSContextMarshallEx(IN RPC_BINDING_HANDLE BindingHandle,
                                  IN NDR_SCONTEXT CContext,
                                  OUT void *pBuff,
                                  IN NDR_RUNDOWN userRunDownIn)
{
  FIXME("stub\n");
}

/***********************************************************************
 *           NDRSContextUnmarshallEx
 */
NDR_SCONTEXT WINAPI NDRSContextUnmarshallEx(IN RPC_BINDING_HANDLE BindingHandle,
                                            IN void *pBuff,
                                            IN unsigned long DataRepresentation)
{
  FIXME("stub\n");
  return NULL;
}
