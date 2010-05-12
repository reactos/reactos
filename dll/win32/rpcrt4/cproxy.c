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
 * 
 * TODO: Handle non-i386 architectures
 */

#include "config.h"
#include "wine/port.h"

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

/* I don't know what MS's std proxy structure looks like,
   so this probably doesn't match, but that shouldn't matter */
typedef struct {
  const IRpcProxyBufferVtbl *lpVtbl;
  LPVOID *PVtbl;
  LONG RefCount;
  const IID* piid;
  LPUNKNOWN pUnkOuter;
  IUnknown *base_object;  /* must be at offset 0x10 from PVtbl */
  IRpcProxyBuffer *base_proxy;
  PCInterfaceName name;
  LPPSFACTORYBUFFER pPSFactory;
  LPRPCCHANNELBUFFER pChannel;
} StdProxyImpl;

static const IRpcProxyBufferVtbl StdProxy_Vtbl;

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

#if defined(__i386__)

#include "pshpack1.h"

struct thunk {
  BYTE push;
  DWORD index;
  BYTE jmp;
  LONG handler;
};

#include "poppack.h"

extern void call_stubless_func(void);
__ASM_GLOBAL_FUNC(call_stubless_func,
                  "pushl %esp\n\t"  /* pointer to index */
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "call " __ASM_NAME("ObjectStubless") __ASM_STDCALL(4) "\n\t"
                  "popl %edx\n\t"  /* args size */
                  __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                  "movl (%esp),%ecx\n\t"  /* return address */
                  "addl %edx,%esp\n\t"
                  "jmp *%ecx" );

HRESULT WINAPI ObjectStubless(DWORD *args)
{
    DWORD index = args[0];
    void **iface = (void **)args[2];
    const void **vtbl = (const void **)*iface;
    const MIDL_STUBLESS_PROXY_INFO *stubless = *(const MIDL_STUBLESS_PROXY_INFO **)(vtbl - 2);
    const PFORMAT_STRING fs = stubless->ProcFormatString + stubless->FormatStringOffset[index];

    /* store bytes to remove from stack */
    args[0] = *(const WORD*)(fs + 8);
    TRACE("(%p)->(%d)([%d bytes]) ret=%08x\n", iface, index, args[0], args[1]);

    return NdrClientCall2(stubless->pStubDesc, fs, args + 2);
}

#define BLOCK_SIZE 1024
#define MAX_BLOCKS 64  /* 64k methods should be enough for anybody */

static const struct thunk *method_blocks[MAX_BLOCKS];

static const struct thunk *allocate_block( unsigned int num )
{
    unsigned int i;
    struct thunk *prev, *block;

    block = VirtualAlloc( NULL, BLOCK_SIZE * sizeof(*block),
                          MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if (!block) return NULL;

    for (i = 0; i < BLOCK_SIZE; i++)
    {
        block[i].push    = 0x68; /* pushl */
        block[i].index   = BLOCK_SIZE * num + i + 3;
        block[i].jmp     = 0xe9; /* jmp */
        block[i].handler = (char *)call_stubless_func - (char *)(&block[i].handler + 1);
    }
    VirtualProtect( block, BLOCK_SIZE * sizeof(*block), PAGE_EXECUTE_READ, NULL );
    prev = InterlockedCompareExchangePointer( (void **)&method_blocks[num], block, NULL );
    if (prev) /* someone beat us to it */
    {
        VirtualFree( block, 0, MEM_RELEASE );
        block = prev;
    }
    return block;
}

static BOOL fill_stubless_table( IUnknownVtbl *vtbl, DWORD num )
{
    const void **entry = (const void **)(vtbl + 1);
    DWORD i, j;

    if (num - 3 > BLOCK_SIZE * MAX_BLOCKS)
    {
        FIXME( "%u methods not supported\n", num );
        return FALSE;
    }
    for (i = 0; i < (num - 3 + BLOCK_SIZE - 1) / BLOCK_SIZE; i++)
    {
        const struct thunk *block = method_blocks[i];
        if (!block && !(block = allocate_block( i ))) return FALSE;
        for (j = 0; j < BLOCK_SIZE && j < num - 3 - i * BLOCK_SIZE; j++, entry++)
            if (*entry == (LPVOID)-1) *entry = &block[j];
    }
    return TRUE;
}

#else  /* __i386__ */

static BOOL fill_stubless_table( IUnknownVtbl *vtbl, DWORD num )
{
    ERR("stubless proxies are not supported on this architecture\n");
    return FALSE;
}

#endif  /* __i386__ */

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
    TRACE("stubless vtbl %p: count=%d\n", vtbl->Vtbl, count );
    fill_stubless_table( (IUnknownVtbl *)vtbl->Vtbl, count );
  }

  if (!IsEqualGUID(vtbl->header.piid, riid)) {
    ERR("IID mismatch during proxy creation\n");
    return RPC_E_UNEXPECTED;
  }

  This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(StdProxyImpl));
  if (!This) return E_OUTOFMEMORY;

  if (!pUnkOuter) pUnkOuter = (IUnknown *)This;
  This->lpVtbl = &StdProxy_Vtbl;
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
          HeapFree( GetProcessHeap(), 0, This );
          return r;
      }
  }

  *ppProxy = (LPRPCPROXYBUFFER)&This->lpVtbl;
  *ppvObj = &This->PVtbl;
  IUnknown_AddRef((IUnknown *)*ppvObj);
  IPSFactoryBuffer_AddRef(pPSFactory);

  TRACE( "iid=%s this %p proxy %p obj %p vtbl %p base proxy %p base obj %p\n",
         debugstr_guid(riid), This, *ppProxy, *ppvObj, This->PVtbl, This->base_proxy, This->base_object );
  return S_OK;
}

static void StdProxy_Destruct(LPRPCPROXYBUFFER iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);

  if (This->pChannel)
    IRpcProxyBuffer_Disconnect(iface);

  if (This->base_object) IUnknown_Release( This->base_object );
  if (This->base_proxy) IRpcProxyBuffer_Release( This->base_proxy );

  IPSFactoryBuffer_Release(This->pPSFactory);
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
  if (This->base_proxy) IRpcProxyBuffer_Connect( This->base_proxy, pChannel );
  return S_OK;
}

static VOID WINAPI StdProxy_Disconnect(LPRPCPROXYBUFFER iface)
{
  ICOM_THIS_MULTI(StdProxyImpl,lpVtbl,iface);
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
