/*
 * COM proxy implementation
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
 * Copyright 2009 Alexandre Julliard
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "objbase.h"
#include "rpcproxy.h"

#include "cpsf.h"
#include "ndr_misc.h"
#include "ndr_stubless.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static const IRpcProxyBufferVtbl StdProxy_Vtbl;

static inline StdProxyImpl *impl_from_IRpcProxyBuffer(IRpcProxyBuffer *iface)
{
    return CONTAINING_RECORD(iface, StdProxyImpl, IRpcProxyBuffer_iface);
}

static inline StdProxyImpl *impl_from_proxy_obj( void *iface )
{
    return CONTAINING_RECORD(iface, StdProxyImpl, PVtbl);
}

extern void ObjectStublessClient3(void);
extern void ObjectStublessClient4(void);

BOOL fill_stubless_table( IUnknownVtbl *vtbl, DWORD num )
{
    size_t entry_size = (char *)ObjectStublessClient4 - (char *)ObjectStublessClient3;
    const void **entry = (const void **)(vtbl + 1);
    DWORD i;

    if (num >= NB_THUNK_ENTRIES)
    {
        FIXME( "%lu methods not supported\n", num );
        return FALSE;
    }
    for (i = 0; i < num - 3; i++, entry++)
        if (*entry == (void *)-1) *entry = (char *)ObjectStublessClient3 + i * entry_size;

    return TRUE;
}

HRESULT StdProxy_Construct(REFIID riid,
                           LPUNKNOWN pUnkOuter,
                           const ProxyFileInfo *ProxyInfo,
                           int Index,
                           LPPSFACTORYBUFFER pPSFactory,
                           LPRPCPROXYBUFFER *ppProxy,
                           LPVOID *ppvObj)
{
  StdProxyImpl *This;
  PCInterfaceName name = ProxyInfo->pNamesArray[Index];
  CInterfaceProxyVtbl *vtbl = ProxyInfo->pProxyVtblList[Index];

  TRACE("(%p,%p,%p,%p,%p) %s\n", pUnkOuter, vtbl, pPSFactory, ppProxy, ppvObj, name);

  /* TableVersion = 2 means it is the stubless version of CInterfaceProxyVtbl */
  if (ProxyInfo->TableVersion > 1) {
    ULONG count = ProxyInfo->pStubVtblList[Index]->header.DispatchTableCount;
    vtbl = (CInterfaceProxyVtbl *)((const void **)vtbl + 1);
    TRACE("stubless vtbl %p: count=%ld\n", vtbl->Vtbl, count );
    fill_stubless_table( (IUnknownVtbl *)vtbl->Vtbl, count );
  }

  if (!IsEqualGUID(vtbl->header.piid, riid)) {
    ERR("IID mismatch during proxy creation\n");
    return RPC_E_UNEXPECTED;
  }

  This = calloc(1, sizeof(StdProxyImpl));
  if (!This) return E_OUTOFMEMORY;

  if (!pUnkOuter) pUnkOuter = (IUnknown *)&This->IRpcProxyBuffer_iface;
  This->IRpcProxyBuffer_iface.lpVtbl = &StdProxy_Vtbl;
  This->PVtbl = vtbl->Vtbl;
  /* one reference for the proxy */
  This->RefCount = 1;
  This->piid = vtbl->header.piid;
  This->base_object = NULL;
  This->base_proxy = NULL;
  This->pUnkOuter = pUnkOuter;
  This->name = name;
  This->pPSFactory = pPSFactory;
  This->pChannel = NULL;

  if(ProxyInfo->pDelegatedIIDs && ProxyInfo->pDelegatedIIDs[Index])
  {
      HRESULT r = create_proxy( ProxyInfo->pDelegatedIIDs[Index], NULL,
                                &This->base_proxy, (void **)&This->base_object );
      if (FAILED(r))
      {
          free( This );
          return r;
      }
  }

  *ppProxy = &This->IRpcProxyBuffer_iface;
  *ppvObj = &This->PVtbl;
  IUnknown_AddRef((IUnknown *)*ppvObj);
  IPSFactoryBuffer_AddRef(pPSFactory);

  TRACE( "iid=%s this %p proxy %p obj %p vtbl %p base proxy %p base obj %p\n",
         debugstr_guid(riid), This, *ppProxy, *ppvObj, This->PVtbl, This->base_proxy, This->base_object );
  return S_OK;
}

HRESULT WINAPI StdProxy_QueryInterface(IRpcProxyBuffer *iface, REFIID riid, void **obj)
{
  StdProxyImpl *This = impl_from_IRpcProxyBuffer(iface);
  TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(riid),obj);

  if (IsEqualGUID(&IID_IUnknown,riid) ||
      IsEqualGUID(This->piid,riid)) {
    *obj = &This->PVtbl;
    InterlockedIncrement(&This->RefCount);
    return S_OK;
  }

  if (IsEqualGUID(&IID_IRpcProxyBuffer,riid)) {
    *obj = &This->IRpcProxyBuffer_iface;
    InterlockedIncrement(&This->RefCount);
    return S_OK;
  }

  return E_NOINTERFACE;
}

ULONG WINAPI StdProxy_AddRef(IRpcProxyBuffer *iface)
{
  StdProxyImpl *This = impl_from_IRpcProxyBuffer(iface);
  TRACE("(%p)->AddRef()\n",This);

  return InterlockedIncrement(&This->RefCount);
}

static ULONG WINAPI StdProxy_Release(LPRPCPROXYBUFFER iface)
{
  ULONG refs;
  StdProxyImpl *This = impl_from_IRpcProxyBuffer(iface);
  TRACE("(%p)->Release()\n",This);

  refs = InterlockedDecrement(&This->RefCount);
  if (!refs)
  {
    if (This->pChannel)
      IRpcProxyBuffer_Disconnect(&This->IRpcProxyBuffer_iface);

    if (This->base_object) IUnknown_Release( This->base_object );
    if (This->base_proxy) IRpcProxyBuffer_Release( This->base_proxy );

    IPSFactoryBuffer_Release(This->pPSFactory);
    free(This);
  }

  return refs;
}

HRESULT WINAPI StdProxy_Connect(IRpcProxyBuffer *iface, IRpcChannelBuffer *pChannel)
{
  StdProxyImpl *This = impl_from_IRpcProxyBuffer(iface);
  TRACE("(%p)->Connect(%p)\n",This,pChannel);

  This->pChannel = pChannel;
  IRpcChannelBuffer_AddRef(pChannel);
  if (This->base_proxy) IRpcProxyBuffer_Connect( This->base_proxy, pChannel );
  return S_OK;
}

void WINAPI StdProxy_Disconnect(IRpcProxyBuffer *iface)
{
  StdProxyImpl *This = impl_from_IRpcProxyBuffer(iface);
  TRACE("(%p)->Disconnect()\n",This);

  if (This->base_proxy) IRpcProxyBuffer_Disconnect( This->base_proxy );

  IRpcChannelBuffer_Release(This->pChannel);
  This->pChannel = NULL;
}

static const IRpcProxyBufferVtbl StdProxy_Vtbl =
{
  StdProxy_QueryInterface,
  StdProxy_AddRef,
  StdProxy_Release,
  StdProxy_Connect,
  StdProxy_Disconnect
};

static void StdProxy_GetChannel(LPVOID iface,
                                   LPRPCCHANNELBUFFER *ppChannel)
{
  StdProxyImpl *This = impl_from_proxy_obj( iface );
  TRACE("(%p)->GetChannel(%p) %s\n",This,ppChannel,This->name);

  if(This->pChannel)
    IRpcChannelBuffer_AddRef(This->pChannel);

  *ppChannel = This->pChannel;
}

static void StdProxy_GetIID(LPVOID iface,
                               const IID **ppiid)
{
  StdProxyImpl *This = impl_from_proxy_obj( iface );
  TRACE("(%p)->GetIID(%p) %s\n",This,ppiid,This->name);

  *ppiid = This->piid;
}

HRESULT WINAPI IUnknown_QueryInterface_Proxy(LPUNKNOWN iface,
                                            REFIID riid,
                                            LPVOID *ppvObj)
{
  StdProxyImpl *This = impl_from_proxy_obj( iface );
  TRACE("(%p)->QueryInterface(%s,%p) %s\n",This,debugstr_guid(riid),ppvObj,This->name);
  return IUnknown_QueryInterface(This->pUnkOuter,riid,ppvObj);
}

ULONG WINAPI IUnknown_AddRef_Proxy(LPUNKNOWN iface)
{
  StdProxyImpl *This = impl_from_proxy_obj( iface );
  TRACE("(%p)->AddRef() %s\n",This,This->name);
  return IUnknown_AddRef(This->pUnkOuter);
}

ULONG WINAPI IUnknown_Release_Proxy(LPUNKNOWN iface)
{
  StdProxyImpl *This = impl_from_proxy_obj( iface );
  TRACE("(%p)->Release() %s\n",This,This->name);
  return IUnknown_Release(This->pUnkOuter);
}

/***********************************************************************
 *           NdrProxyInitialize [RPCRT4.@]
 */
void WINAPI NdrProxyInitialize(void *This,
                              PRPC_MESSAGE pRpcMsg,
                              PMIDL_STUB_MESSAGE pStubMsg,
                              PMIDL_STUB_DESC pStubDescriptor,
                              unsigned int ProcNum)
{
  TRACE("(%p,%p,%p,%p,%d)\n", This, pRpcMsg, pStubMsg, pStubDescriptor, ProcNum);
  NdrClientInitializeNew(pRpcMsg, pStubMsg, pStubDescriptor, ProcNum);
  StdProxy_GetChannel(This, &pStubMsg->pRpcChannelBuffer);
  if (!pStubMsg->pRpcChannelBuffer)
    RpcRaiseException(CO_E_OBJNOTCONNECTED);
  IRpcChannelBuffer_GetDestCtx(pStubMsg->pRpcChannelBuffer,
                               &pStubMsg->dwDestContext,
                               &pStubMsg->pvDestContext);
  TRACE("channel=%p\n", pStubMsg->pRpcChannelBuffer);
}

/***********************************************************************
 *           NdrProxyGetBuffer [RPCRT4.@]
 */
void WINAPI NdrProxyGetBuffer(void *This,
                             PMIDL_STUB_MESSAGE pStubMsg)
{
  HRESULT hr;
  const IID *riid = NULL;

  TRACE("(%p,%p)\n", This, pStubMsg);
  pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
  pStubMsg->dwStubPhase = PROXY_GETBUFFER;
  StdProxy_GetIID(This, &riid);
  hr = IRpcChannelBuffer_GetBuffer(pStubMsg->pRpcChannelBuffer,
                                  (RPCOLEMESSAGE*)pStubMsg->RpcMsg,
                                  riid);
  if (FAILED(hr))
  {
    RpcRaiseException(hr);
    return;
  }
  pStubMsg->fBufferValid = TRUE;
  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;
  pStubMsg->dwStubPhase = PROXY_MARSHAL;
}

/***********************************************************************
 *           NdrProxySendReceive [RPCRT4.@]
 */
void WINAPI NdrProxySendReceive(void *This,
                               PMIDL_STUB_MESSAGE pStubMsg)
{
  ULONG Status = 0;
  HRESULT hr;

  TRACE("(%p,%p)\n", This, pStubMsg);

  if (!pStubMsg->pRpcChannelBuffer)
  {
    WARN("Trying to use disconnected proxy %p\n", This);
    RpcRaiseException(RPC_E_DISCONNECTED);
  }

  pStubMsg->dwStubPhase = PROXY_SENDRECEIVE;
  /* avoid sending uninitialised parts of the buffer on the wire */
  pStubMsg->RpcMsg->BufferLength = pStubMsg->Buffer - (unsigned char *)pStubMsg->RpcMsg->Buffer;
  hr = IRpcChannelBuffer_SendReceive(pStubMsg->pRpcChannelBuffer,
                                    (RPCOLEMESSAGE*)pStubMsg->RpcMsg,
                                    &Status);
  pStubMsg->dwStubPhase = PROXY_UNMARSHAL;
  pStubMsg->BufferLength = pStubMsg->RpcMsg->BufferLength;
  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;

  /* raise exception if call failed */
  if (hr == RPC_S_CALL_FAILED) RpcRaiseException(*(DWORD*)pStubMsg->Buffer);
  else if (FAILED(hr)) RpcRaiseException(hr);
}

/***********************************************************************
 *           NdrProxyFreeBuffer [RPCRT4.@]
 */
void WINAPI NdrProxyFreeBuffer(void *This,
                              PMIDL_STUB_MESSAGE pStubMsg)
{
  TRACE("(%p,%p)\n", This, pStubMsg);

  if (pStubMsg->fBufferValid)
  {
    IRpcChannelBuffer_FreeBuffer(pStubMsg->pRpcChannelBuffer,
                                 (RPCOLEMESSAGE*)pStubMsg->RpcMsg);
    pStubMsg->fBufferValid = FALSE;
  }
  IRpcChannelBuffer_Release(pStubMsg->pRpcChannelBuffer);
  pStubMsg->pRpcChannelBuffer = NULL;
}

/***********************************************************************
 *           NdrProxyErrorHandler [RPCRT4.@]
 */
HRESULT WINAPI NdrProxyErrorHandler(DWORD dwExceptionCode)
{
  WARN("(0x%08lx): a proxy call failed\n", dwExceptionCode);

  if (FAILED(dwExceptionCode))
    return dwExceptionCode;
  else
    return HRESULT_FROM_WIN32(dwExceptionCode);
}
