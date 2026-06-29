/*
 * COM stub (CStdStubBuffer) implementation
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
#include "excpt.h"

#include "objbase.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wine/asm.h"
#include "wine/exception.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static LONG WINAPI stub_filter(EXCEPTION_POINTERS *eptr)
{
    if (eptr->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
        return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_EXECUTE_HANDLER;
}

static CStdStubBuffer *impl_from_IRpcStubBuffer(IRpcStubBuffer *iface)
{
    return CONTAINING_RECORD(&iface->lpVtbl, CStdStubBuffer, lpVtbl);
}

static inline cstdstubbuffer_delegating_t *impl_from_delegating( IRpcStubBuffer *iface )
{
    return CONTAINING_RECORD(impl_from_IRpcStubBuffer(iface), cstdstubbuffer_delegating_t, stub_buffer);
}

static const CInterfaceStubHeader *get_stub_header(const CStdStubBuffer *stub)
{
    const CInterfaceStubVtbl *vtbl = CONTAINING_RECORD(stub->lpVtbl, CInterfaceStubVtbl, Vtbl);

    return &vtbl->header;
}

HRESULT CStdStubBuffer_Construct(REFIID riid,
                                 LPUNKNOWN pUnkServer,
                                 PCInterfaceName name,
                                 CInterfaceStubVtbl *vtbl,
                                 LPPSFACTORYBUFFER pPSFactory,
                                 LPRPCSTUBBUFFER *ppStub)
{
  CStdStubBuffer *This;
  IUnknown *pvServer;
  HRESULT r;
  TRACE("(%p,%p,%p,%p) %s\n", pUnkServer, vtbl, pPSFactory, ppStub, name);
  TRACE("iid=%s\n", debugstr_guid(vtbl->header.piid));
  TRACE("vtbl=%p\n", &vtbl->Vtbl);

  if (!IsEqualGUID(vtbl->header.piid, riid)) {
    ERR("IID mismatch during stub creation\n");
    return RPC_E_UNEXPECTED;
  }

  r = IUnknown_QueryInterface(pUnkServer, riid, (void**)&pvServer);
  if(FAILED(r))
    return r;

  This = calloc(1, sizeof(CStdStubBuffer));
  if (!This) {
    IUnknown_Release(pvServer);
    return E_OUTOFMEMORY;
  }

  This->lpVtbl = &vtbl->Vtbl;
  This->RefCount = 1;
  This->pvServerObject = pvServer;
  This->pPSFactory = pPSFactory;
  *ppStub = (LPRPCSTUBBUFFER)This;

  IPSFactoryBuffer_AddRef(pPSFactory);
  return S_OK;
}


BOOL fill_delegated_proxy_table(IUnknownVtbl *vtbl, DWORD num)
{
    const void **entry = (const void **)(vtbl + 1);
    DWORD i;

    if (num > NB_THUNK_ENTRIES)
    {
        FIXME( "%lu methods not supported\n", num );
        return FALSE;
    }
    vtbl->QueryInterface = IUnknown_QueryInterface_Proxy;
    vtbl->AddRef = IUnknown_AddRef_Proxy;
    vtbl->Release = IUnknown_Release_Proxy;
    for (i = 0; i < num - 3; i++)
        if (!entry[i]) entry[i] = delegating_vtbl.methods[i];
    return TRUE;
}

const IUnknownVtbl *get_delegating_vtbl(DWORD num)
{
    if (num > NB_THUNK_ENTRIES)
    {
        FIXME( "%lu methods not supported\n", num );
        return NULL;
    }
    return &delegating_vtbl.vtbl;
}

HRESULT CStdStubBuffer_Delegating_Construct(REFIID riid,
                                            LPUNKNOWN pUnkServer,
                                            PCInterfaceName name,
                                            CInterfaceStubVtbl *vtbl,
                                            REFIID delegating_iid,
                                            LPPSFACTORYBUFFER pPSFactory,
                                            LPRPCSTUBBUFFER *ppStub)
{
    cstdstubbuffer_delegating_t *This;
    IUnknown *pvServer;
    HRESULT r;

    TRACE("(%p,%p,%p,%p) %s\n", pUnkServer, vtbl, pPSFactory, ppStub, name);
    TRACE("iid=%s delegating to %s\n", debugstr_guid(vtbl->header.piid), debugstr_guid(delegating_iid));
    TRACE("vtbl=%p\n", &vtbl->Vtbl);

    if (!IsEqualGUID(vtbl->header.piid, riid))
    {
        ERR("IID mismatch during stub creation\n");
        return RPC_E_UNEXPECTED;
    }

    r = IUnknown_QueryInterface(pUnkServer, riid, (void**)&pvServer);
    if(FAILED(r)) return r;

    This = calloc(1, sizeof(*This));
    if (!This)
    {
        IUnknown_Release(pvServer);
        return E_OUTOFMEMORY;
    }

    This->base_obj.lpVtbl = get_delegating_vtbl( vtbl->header.DispatchTableCount );
    r = create_stub(delegating_iid, &This->base_obj, &This->base_stub);
    if(FAILED(r))
    {
        free(This);
        IUnknown_Release(pvServer);
        return r;
    }

    This->stub_buffer.lpVtbl = &vtbl->Vtbl;
    This->stub_buffer.RefCount = 1;
    This->stub_buffer.pvServerObject = pvServer;
    This->stub_buffer.pPSFactory = pPSFactory;
    *ppStub = (LPRPCSTUBBUFFER)&This->stub_buffer;

    IPSFactoryBuffer_AddRef(pPSFactory);
    return S_OK;
}

HRESULT WINAPI CStdStubBuffer_QueryInterface(LPRPCSTUBBUFFER iface,
                                            REFIID riid,
                                            LPVOID *obj)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(riid),obj);

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_IRpcStubBuffer, riid))
  {
    IRpcStubBuffer_AddRef(iface);
    *obj = iface;
    return S_OK;
  }
  *obj = NULL;
  return E_NOINTERFACE;
}

ULONG WINAPI CStdStubBuffer_AddRef(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  TRACE("(%p)->AddRef()\n",This);
  return InterlockedIncrement(&This->RefCount);
}

ULONG WINAPI NdrCStdStubBuffer_Release(LPRPCSTUBBUFFER iface,
                                      LPPSFACTORYBUFFER pPSF)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  ULONG refs;

  TRACE("(%p)->Release()\n",This);

  refs = InterlockedDecrement(&This->RefCount);
  if (!refs)
  {
    /* test_Release shows that native doesn't call Disconnect here.
       We'll leave it in for the time being. */
    IRpcStubBuffer_Disconnect(iface);

    IPSFactoryBuffer_Release(pPSF);
    free(This);
  }
  return refs;
}

ULONG WINAPI NdrCStdStubBuffer2_Release(LPRPCSTUBBUFFER iface,
                                        LPPSFACTORYBUFFER pPSF)
{
    cstdstubbuffer_delegating_t *This = impl_from_delegating( iface );
    ULONG refs;

    TRACE("(%p)->Release()\n", This);

    refs = InterlockedDecrement(&This->stub_buffer.RefCount);
    if (!refs)
    {
        /* Just like NdrCStdStubBuffer_Release, we shouldn't call
           Disconnect here */
        IRpcStubBuffer_Disconnect((IRpcStubBuffer *)&This->stub_buffer);

        IRpcStubBuffer_Release(This->base_stub);
        IPSFactoryBuffer_Release(pPSF);
        free(This);
    }

    return refs;
}

HRESULT WINAPI CStdStubBuffer_Connect(LPRPCSTUBBUFFER iface,
                                     LPUNKNOWN lpUnkServer)
{
    CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
    HRESULT r;
    IUnknown *new = NULL;

    TRACE("(%p)->Connect(%p)\n",This,lpUnkServer);

    r = IUnknown_QueryInterface(lpUnkServer, get_stub_header(This)->piid, (void**)&new);
    new = InterlockedExchangePointer((void**)&This->pvServerObject, new);
    if(new)
        IUnknown_Release(new);
    return r;
}

void WINAPI CStdStubBuffer_Disconnect(LPRPCSTUBBUFFER iface)
{
    CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
    IUnknown *old;
    TRACE("(%p)->Disconnect()\n",This);

    old = InterlockedExchangePointer((void**)&This->pvServerObject, NULL);

    if(old)
        IUnknown_Release(old);
}

HRESULT WINAPI CStdStubBuffer_Invoke(LPRPCSTUBBUFFER iface,
                                    PRPCOLEMESSAGE pMsg,
                                    LPRPCCHANNELBUFFER pChannel)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  const CInterfaceStubHeader *header = get_stub_header(This);
  DWORD dwPhase = STUB_UNMARSHAL;
  HRESULT hr = S_OK;

  TRACE("(%p)->Invoke(%p,%p)\n",This,pMsg,pChannel);

  __TRY
  {
    if (header->pDispatchTable)
      header->pDispatchTable[pMsg->iMethod](iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
    else /* pure interpreted */
      NdrStubCall2(iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
  }
  __EXCEPT(stub_filter)
  {
    DWORD dwExceptionCode = GetExceptionCode();
    WARN("a stub call failed with exception 0x%08lx (%ld)\n", dwExceptionCode, dwExceptionCode);
    if (FAILED(dwExceptionCode))
      hr = dwExceptionCode;
    else
      hr = HRESULT_FROM_WIN32(dwExceptionCode);
  }
  __ENDTRY

  return hr;
}

LPRPCSTUBBUFFER WINAPI CStdStubBuffer_IsIIDSupported(LPRPCSTUBBUFFER iface,
                                                    REFIID riid)
{
    CStdStubBuffer *stub = impl_from_IRpcStubBuffer(iface);

    TRACE("(%p)->IsIIDSupported(%s)\n", stub, debugstr_guid(riid));

    if (IsEqualGUID(get_stub_header(stub)->piid, riid))
        return iface;
    return NULL;
}

ULONG WINAPI CStdStubBuffer_CountRefs(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  TRACE("(%p)->CountRefs()\n",This);
  return This->RefCount;
}

HRESULT WINAPI CStdStubBuffer_DebugServerQueryInterface(LPRPCSTUBBUFFER iface,
                                                       LPVOID *ppv)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  TRACE("(%p)->DebugServerQueryInterface(%p)\n",This,ppv);
  return S_OK;
}

void WINAPI CStdStubBuffer_DebugServerRelease(LPRPCSTUBBUFFER iface,
                                             LPVOID pv)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  TRACE("(%p)->DebugServerRelease(%p)\n",This,pv);
}

const IRpcStubBufferVtbl CStdStubBuffer_Vtbl =
{
    CStdStubBuffer_QueryInterface,
    CStdStubBuffer_AddRef,
    NULL,
    CStdStubBuffer_Connect,
    CStdStubBuffer_Disconnect,
    CStdStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease
};

static HRESULT WINAPI CStdStubBuffer_Delegating_Connect(LPRPCSTUBBUFFER iface,
                                                        LPUNKNOWN lpUnkServer)
{
    cstdstubbuffer_delegating_t *This = impl_from_delegating(iface);
    HRESULT r;
    TRACE("(%p)->Connect(%p)\n", This, lpUnkServer);

    r = CStdStubBuffer_Connect(iface, lpUnkServer);
    if(SUCCEEDED(r))
        r = IRpcStubBuffer_Connect(This->base_stub, (IUnknown*)&This->base_obj);

    return r;
}

static void WINAPI CStdStubBuffer_Delegating_Disconnect(LPRPCSTUBBUFFER iface)
{
    cstdstubbuffer_delegating_t *This = impl_from_delegating(iface);
    TRACE("(%p)->Disconnect()\n", This);

    IRpcStubBuffer_Disconnect(This->base_stub);
    CStdStubBuffer_Disconnect(iface);
}

static ULONG WINAPI CStdStubBuffer_Delegating_CountRefs(LPRPCSTUBBUFFER iface)
{
    cstdstubbuffer_delegating_t *This = impl_from_delegating(iface);
    ULONG ret;
    TRACE("(%p)->CountRefs()\n", This);

    ret = CStdStubBuffer_CountRefs(iface);
    ret += IRpcStubBuffer_CountRefs(This->base_stub);

    return ret;
}

const IRpcStubBufferVtbl CStdStubBuffer_Delegating_Vtbl =
{
    CStdStubBuffer_QueryInterface,
    CStdStubBuffer_AddRef,
    NULL,
    CStdStubBuffer_Delegating_Connect,
    CStdStubBuffer_Delegating_Disconnect,
    CStdStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer_Delegating_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease
};

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface)
{
    CStdStubBuffer *stub = impl_from_IRpcStubBuffer(iface);

    return get_stub_header(stub)->pServerInfo;
}

/************************************************************************
 *           NdrStubForwardingFunction [RPCRT4.@]
 */
void __RPC_STUB NdrStubForwardingFunction( IRpcStubBuffer *iface, IRpcChannelBuffer *pChannel,
                                           PRPC_MESSAGE pMsg, DWORD *pdwStubPhase )
{
    /* Note pMsg is passed intact since RPCOLEMESSAGE is basically a RPC_MESSAGE. */

    cstdstubbuffer_delegating_t *This = impl_from_delegating(iface);
    HRESULT r = IRpcStubBuffer_Invoke(This->base_stub, (RPCOLEMESSAGE*)pMsg, pChannel);
    if(FAILED(r)) RpcRaiseException(r);
    return;
}

/***********************************************************************
 *           NdrStubInitialize [RPCRT4.@]
 */
void WINAPI NdrStubInitialize(PRPC_MESSAGE pRpcMsg,
                             PMIDL_STUB_MESSAGE pStubMsg,
                             PMIDL_STUB_DESC pStubDescriptor,
                             LPRPCCHANNELBUFFER pRpcChannelBuffer)
{
  TRACE("(%p,%p,%p,%p)\n", pRpcMsg, pStubMsg, pStubDescriptor, pRpcChannelBuffer);
  NdrServerInitializeNew(pRpcMsg, pStubMsg, pStubDescriptor);
  pStubMsg->pRpcChannelBuffer = pRpcChannelBuffer;
  IRpcChannelBuffer_GetDestCtx(pStubMsg->pRpcChannelBuffer,
                               &pStubMsg->dwDestContext,
                               &pStubMsg->pvDestContext);
}

/***********************************************************************
 *           NdrStubGetBuffer [RPCRT4.@]
 */
void WINAPI NdrStubGetBuffer(LPRPCSTUBBUFFER iface,
                            LPRPCCHANNELBUFFER pRpcChannelBuffer,
                            PMIDL_STUB_MESSAGE pStubMsg)
{
  CStdStubBuffer *This = impl_from_IRpcStubBuffer(iface);
  HRESULT hr;

  TRACE("(%p, %p, %p)\n", This, pRpcChannelBuffer, pStubMsg);

  pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
  hr = IRpcChannelBuffer_GetBuffer(pRpcChannelBuffer,
    (RPCOLEMESSAGE *)pStubMsg->RpcMsg, get_stub_header(This)->piid);
  if (FAILED(hr))
  {
    RpcRaiseException(hr);
    return;
  }

  pStubMsg->Buffer = pStubMsg->RpcMsg->Buffer;
}
