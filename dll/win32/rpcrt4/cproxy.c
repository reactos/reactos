/*
 * COM proxy implementation
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
 * TODO: Handle non-i386 architectures
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct StublessThunk;

/* I don't know what MS's std proxy structure looks like,
   so this probably doesn't match, but that shouldn't matter */
typedef struct {
  const IRpcProxyBufferVtbl *lpVtbl;
  LPVOID *PVtbl;
  LONG RefCount;
  const MIDL_STUBLESS_PROXY_INFO *stubless;
  const IID* piid;
  LPUNKNOWN pUnkOuter;
  PCInterfaceName name;
  LPPSFACTORYBUFFER pPSFactory;
  LPRPCCHANNELBUFFER pChannel;
  struct StublessThunk *thunks;
} StdProxyImpl;

static const IRpcProxyBufferVtbl StdProxy_Vtbl;

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

#if defined(__i386__)

#include "pshpack1.h"

struct StublessThunk {
  BYTE push;
  DWORD index;
  BYTE call;
  LONG handler;
  BYTE ret;
  WORD bytes;
  BYTE pad[3];
};

#include "poppack.h"

/* adjust the stack size since we don't use Windows's method */
#define STACK_ADJUST sizeof(DWORD)

#define FILL_STUBLESS(x,idx,stk) \
 x->push = 0x68; /* pushl [immediate] */ \
 x->index = (idx); \
 x->call = 0xe8; /* call [near] */ \
 x->handler = (char*)ObjectStubless - (char*)&x->ret; \
 x->ret = 0xc2; /* ret [immediate] */ \
 x->bytes = stk; \
 x->pad[0] = 0x8d; /* leal (%esi),%esi */ \
 x->pad[1] = 0x76; \
 x->pad[2] = 0x00;

static HRESULT WINAPI ObjectStubless(DWORD index)
{
  char *args = (char*)(&index + 2);
  LPVOID iface = *(LPVOID*)args;

  ICOM_THIS_MULTI(StdProxyImpl,PVtbl,iface);

  PFORMAT_STRING fs = This->stubless->ProcFormatString + This->stubless->FormatStringOffset[index];
  unsigned bytes = *(const WORD*)(fs+8) - STACK_ADJUST;
  TRACE("(%p)->(%d)([%d bytes]) ret=%08x\n", iface, index, bytes, *(DWORD*)(args+bytes));

  return NdrClientCall2(This->stubless->pStubDesc, fs, args);
}

#else  /* __i386__ */

/* can't do that on this arch */
struct StublessThunk { int dummy; };
#define FILL_STUBLESS(x,idx,stk) \
 ERR("stubless proxies are not supported on this architecture\n");
#define STACK_ADJUST 0

#endif  /* __i386__ */

HRESULT WINAPI StdProxy_Construct(REFIID riid,
                                 LPUNKNOWN pUnkOuter,
                                 const ProxyFileInfo *ProxyInfo,
                                 int Index,
                                 LPPSFACTORYBUFFER pPSFactory,
                                 LPRPCPROXYBUFFER *ppProxy,
                                 LPVOID *ppvObj)
{
  StdProxyImpl *This;
  const MIDL_STUBLESS_PROXY_INFO *stubless = NULL;
  PCInterfaceName name = ProxyInfo->pNamesArray[Index];
  CInterfaceProxyVtbl *vtbl = ProxyInfo->pProxyVtblList[Index];

  TRACE("(%p,%p,%p,%p,%p) %s\n", pUnkOuter, vtbl, pPSFactory, ppProxy, ppvObj, name);

  /* TableVersion = 2 means it is the stubless version of CInterfaceProxyVtbl */
  if (ProxyInfo->TableVersion > 1) {
    stubless = *(const void **)vtbl;
    vtbl = (CInterfaceProxyVtbl *)((const void **)vtbl + 1);
    TRACE("stubless=%p\n", stubless);
  }

  TRACE("iid=%s\n", debugstr_guid(vtbl->header.piid));
  TRACE("vtbl=%p\n", vtbl->Vtbl);

  if (!IsEqualGUID(vtbl->header.piid, riid)) {
    ERR("IID mismatch during proxy creation\n");
    return RPC_E_UNEXPECTED;
  }

  This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(StdProxyImpl));
  if (!This) return E_OUTOFMEMORY;

  if (stubless) {
    CInterfaceStubVtbl *svtbl = ProxyInfo->pStubVtblList[Index];
    unsigned long i, count = svtbl->header.DispatchTableCount;
    /* Maybe the original vtbl is just modified directly to point at
     * ObjectStublessClientXXX thunks in real Windows, but I don't like it
     */
    TRACE("stubless thunks: count=%ld\n", count);
    This->thunks = HeapAlloc(GetProcessHeap(),0,sizeof(struct StublessThunk)*count);
    This->PVtbl = HeapAlloc(GetProcessHeap(),0,sizeof(LPVOID)*count);
    for (i=0; i<count; i++) {
      struct StublessThunk *thunk = &This->thunks[i];
      if (vtbl->Vtbl[i] == (LPVOID)-1) {
        PFORMAT_STRING fs = stubless->ProcFormatString + stubless->FormatStringOffset[i];
        unsigned bytes = *(const WORD*)(fs+8) - STACK_ADJUST;
        TRACE("method %ld: stacksize=%d\n", i, bytes);
        FILL_STUBLESS(thunk, i, bytes)
        This->PVtbl[i] = thunk;
      }
      else {
        memset(thunk, 0, sizeof(struct StublessThunk));
        This->PVtbl[i] = vtbl->Vtbl[i];
      }
    }
  }
  else 
    This->PVtbl = vtbl->Vtbl;

  This->lpVtbl = &StdProxy_Vtbl;
  /* one reference for the proxy */
  This->RefCount = 1;
  This->stubless = stubless;
  This->piid = vtbl->header.piid;
  This->pUnkOuter = pUnkOuter;
  This->name = name;
  This->pPSFactory = pPSFactory;
  This->pChannel = NULL;
  *ppProxy = (LPRPCPROXYBUFFER)&This->lpVtbl;
  *ppvObj = &This->PVtbl;
  /* if there is no outer unknown then the caller will control the lifetime
   * of the proxy object through the proxy buffer, so no need to increment the
   * ref count of the proxy object */
  if (pUnkOuter)
    IUnknown_AddRef((IUnknown *)*ppvObj);
  IPSFactoryBuffer_AddRef(pPSFactory);

  return S_OK;
}

static void StdProxy_Destruct(LPRPCPROXYBUFFER iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);

  if (This->pChannel)
    IRpcProxyBuffer_Disconnect(iface);

  IPSFactoryBuffer_Release(This->pPSFactory);
  if (This->thunks) {
    HeapFree(GetProcessHeap(),0,This->PVtbl);
    HeapFree(GetProcessHeap(),0,This->thunks);
  }
  HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI StdProxy_QueryInterface(LPRPCPROXYBUFFER iface,
                                             REFIID riid,
                                             LPVOID *obj)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);
  TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(riid),obj);

  if (IsEqualGUID(&IID_IUnknown,riid) ||
      IsEqualGUID(This->piid,riid)) {
    *obj = &This->PVtbl;
    InterlockedIncrement(&This->RefCount);
    return S_OK;
  }

  if (IsEqualGUID(&IID_IRpcProxyBuffer,riid)) {
    *obj = &This->lpVtbl;
    InterlockedIncrement(&This->RefCount);
    return S_OK;
  }

  return E_NOINTERFACE;
}

static ULONG WINAPI StdProxy_AddRef(LPRPCPROXYBUFFER iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);
  TRACE("(%p)->AddRef()\n",This);

  return InterlockedIncrement(&This->RefCount);
}

static ULONG WINAPI StdProxy_Release(LPRPCPROXYBUFFER iface)
{
  ULONG refs;
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);
  TRACE("(%p)->Release()\n",This);

  refs = InterlockedDecrement(&This->RefCount);
  if (!refs)
    StdProxy_Destruct((LPRPCPROXYBUFFER)&This->lpVtbl);
  return refs;
}

static HRESULT WINAPI StdProxy_Connect(LPRPCPROXYBUFFER iface,
                                      LPRPCCHANNELBUFFER pChannel)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);
  TRACE("(%p)->Connect(%p)\n",This,pChannel);

  This->pChannel = pChannel;
  IRpcChannelBuffer_AddRef(pChannel);
  return S_OK;
}

static VOID WINAPI StdProxy_Disconnect(LPRPCPROXYBUFFER iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);
  TRACE("(%p)->Disconnect()\n",This);

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
  ICOM_THIS_MULTI(StdProxyImpl,PVtbl,iface);
  TRACE("(%p)->GetChannel(%p) %s\n",This,ppChannel,This->name);

  *ppChannel = This->pChannel;
}

static void StdProxy_GetIID(LPVOID iface,
                               const IID **ppiid)
{
  ICOM_THIS_MULTI(StdProxyImpl,PVtbl,iface);
  TRACE("(%p)->GetIID(%p) %s\n",This,ppiid,This->name);

  *ppiid = This->piid;
}

HRESULT WINAPI IUnknown_QueryInterface_Proxy(LPUNKNOWN iface,
                                            REFIID riid,
                                            LPVOID *ppvObj)
{
  ICOM_THIS_MULTI(StdProxyImpl,PVtbl,iface);
  TRACE("(%p)->QueryInterface(%s,%p) %s\n",This,debugstr_guid(riid),ppvObj,This->name);
  return IUnknown_QueryInterface(This->pUnkOuter,riid,ppvObj);
}

ULONG WINAPI IUnknown_AddRef_Proxy(LPUNKNOWN iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,PVtbl,iface);
  TRACE("(%p)->AddRef() %s\n",This,This->name);
  return IUnknown_AddRef(This->pUnkOuter);
}

ULONG WINAPI IUnknown_Release_Proxy(LPUNKNOWN iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,PVtbl,iface);
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
    pStubMsg->fBufferValid = TRUE;
  }
}

/***********************************************************************
 *           NdrProxyErrorHandler [RPCRT4.@]
 */
HRESULT WINAPI NdrProxyErrorHandler(DWORD dwExceptionCode)
{
  WARN("(0x%08x): a proxy call failed\n", dwExceptionCode);

  if (FAILED(dwExceptionCode))
    return dwExceptionCode;
  else
    return HRESULT_FROM_WIN32(dwExceptionCode);
}

HRESULT WINAPI
CreateProxyFromTypeInfo( LPTYPEINFO pTypeInfo, LPUNKNOWN pUnkOuter, REFIID riid,
                         LPRPCPROXYBUFFER *ppProxy, LPVOID *ppv )
{
    typedef INT (WINAPI *MessageBoxA)(HWND,LPCSTR,LPCSTR,UINT);
    HMODULE hUser32 = LoadLibraryA("user32");
    MessageBoxA pMessageBoxA = (void *)GetProcAddress(hUser32, "MessageBoxA");

    FIXME("%p %p %s %p %p\n", pTypeInfo, pUnkOuter, debugstr_guid(riid), ppProxy, ppv);
    if (pMessageBoxA)
    {
        pMessageBoxA(NULL,
            "The native implementation of OLEAUT32.DLL cannot be used "
            "with Wine's RPCRT4.DLL. Remove OLEAUT32.DLL and try again.\n",
            "Wine: Unimplemented CreateProxyFromTypeInfo",
            0x10);
        ExitProcess(1);
    }
    return E_NOTIMPL;
}
