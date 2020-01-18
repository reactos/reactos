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

#ifdef __i386__

extern void call_stubless_func(void);
__ASM_GLOBAL_FUNC(call_stubless_func,
                  "movl 4(%esp),%ecx\n\t"         /* This pointer */
                  "movl (%ecx),%ecx\n\t"          /* This->lpVtbl */
                  "movl -8(%ecx),%ecx\n\t"        /* MIDL_STUBLESS_PROXY_INFO */
                  "movl 8(%ecx),%edx\n\t"         /* info->FormatStringOffset */
                  "movzwl (%edx,%eax,2),%edx\n\t" /* FormatStringOffset[index] */
                  "addl 4(%ecx),%edx\n\t"         /* info->ProcFormatString + offset */
                  "movzbl 1(%edx),%eax\n\t"       /* Oi_flags */
                  "andl $0x08,%eax\n\t"           /* Oi_HAS_RPCFLAGS */
                  "shrl $1,%eax\n\t"
                  "movzwl 4(%edx,%eax),%eax\n\t"  /* arguments size */
                  "pushl %eax\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "leal 8(%esp),%eax\n\t"         /* &This */
                  "pushl %eax\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "pushl %edx\n\t"                /* format string */
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "pushl (%ecx)\n\t"              /* info->pStubDesc */
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  "call " __ASM_NAME("ndr_client_call") "\n\t"
                  "leal 12(%esp),%esp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset -12\n\t")
                  "popl %edx\n\t"                 /* arguments size */
                  __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                  "movl (%esp),%ecx\n\t"  /* return address */
                  "addl %edx,%esp\n\t"
                  "jmp *%ecx" );

#include "pshpack1.h"
struct thunk
{
  BYTE mov_eax;
  DWORD index;
  BYTE jmp;
  LONG handler;
};
#include "poppack.h"

static inline void init_thunk( struct thunk *thunk, unsigned int index )
{
    thunk->mov_eax = 0xb8; /* movl $n,%eax */
    thunk->index   = index;
    thunk->jmp     = 0xe9; /* jmp */
    thunk->handler = (char *)call_stubless_func - (char *)(&thunk->handler + 1);
}

#elif defined(__x86_64__)

extern void call_stubless_func(void);
__ASM_GLOBAL_FUNC(call_stubless_func,
                  "subq $0x38,%rsp\n\t"
                  __ASM_SEH(".seh_stackalloc 0x38\n\t")
                  __ASM_SEH(".seh_endprologue\n\t")
                  __ASM_CFI(".cfi_adjust_cfa_offset 0x38\n\t")
                  "movq %rcx,0x40(%rsp)\n\t"
                  "movq %rdx,0x48(%rsp)\n\t"
                  "movq %r8,0x50(%rsp)\n\t"
                  "movq %r9,0x58(%rsp)\n\t"
                  "leaq 0x40(%rsp),%r8\n\t"       /* &This */
                  "movq (%rcx),%rcx\n\t"          /* This->lpVtbl */
                  "movq -0x10(%rcx),%rcx\n\t"     /* MIDL_STUBLESS_PROXY_INFO */
                  "movq 0x10(%rcx),%rdx\n\t"      /* info->FormatStringOffset */
                  "movzwq (%rdx,%r10,2),%rdx\n\t" /* FormatStringOffset[index] */
                  "addq 8(%rcx),%rdx\n\t"         /* info->ProcFormatString + offset */
                  "movq (%rcx),%rcx\n\t"          /* info->pStubDesc */
                  "movq %xmm1,0x20(%rsp)\n\t"
                  "movq %xmm2,0x28(%rsp)\n\t"
                  "movq %xmm3,0x30(%rsp)\n\t"
                  "leaq 0x18(%rsp),%r9\n\t"       /* fpu_args */
                  "call " __ASM_NAME("ndr_client_call") "\n\t"
                  "addq $0x38,%rsp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset -0x38\n\t")
                  "ret" );

#include "pshpack1.h"
struct thunk
{
    BYTE mov_r10[3];
    DWORD index;
    BYTE mov_rax[2];
    void *call_stubless;
    BYTE jmp_rax[2];
};
#include "poppack.h"

static const struct thunk thunk_template =
{
    { 0x49, 0xc7, 0xc2 }, 0,  /* movq $index,%r10 */
    { 0x48, 0xb8 }, 0,        /* movq $call_stubless_func,%rax */
    { 0xff, 0xe0 }            /* jmp *%rax */
};

static inline void init_thunk( struct thunk *thunk, unsigned int index )
{
    *thunk = thunk_template;
    thunk->index = index;
    thunk->call_stubless = call_stubless_func;
}

#elif defined(__arm__)

extern void call_stubless_func(void);
__ASM_GLOBAL_FUNC(call_stubless_func,
                  "push {r0-r3}\n\t"
                  "mov r2, sp\n\t"              /* stack_top */
                  "push {fp,lr}\n\t"
                  "mov fp, sp\n\t"
                  "ldr r0, [r0]\n\t"            /* This->lpVtbl */
                  "ldr r0, [r0,#-8]\n\t"        /* MIDL_STUBLESS_PROXY_INFO */
                  "ldr r1, [r0,#8]\n\t"         /* info->FormatStringOffset */
                  "ldrh r1, [r1,ip]\n\t"        /* info->FormatStringOffset[index] */
                  "ldr ip, [r0,#4]\n\t"         /* info->ProcFormatString */
                  "add r1, ip\n\t"              /* info->ProcFormatString + offset */
                  "ldr r0, [r0]\n\t"            /* info->pStubDesc */
#ifdef __SOFTFP__
                  "mov r3, #0\n\t"
#else
                  "vpush {s0-s15}\n\t"          /* store the s0-s15/d0-d7 arguments */
                  "mov r3, sp\n\t"              /* fpu_stack */
#endif
                  "bl " __ASM_NAME("ndr_client_call") "\n\t"
                  "mov sp, fp\n\t"
                  "pop {fp,lr}\n\t"
                  "add sp, #16\n\t"
                  "bx lr" );

struct thunk
{
    DWORD ldr_ip;         /* ldr ip,[pc] */
    DWORD ldr_pc;         /* ldr pc,[pc] */
    DWORD index;
    void *func;
};

static inline void init_thunk( struct thunk *thunk, unsigned int index )
{
    thunk->ldr_ip = 0xe59fc000; /* ldr ip,[pc] */
    thunk->ldr_pc = 0xe59ff000; /* ldr pc,[pc] */
    thunk->index  = index * sizeof(unsigned short);
    thunk->func   = call_stubless_func;
}

#elif defined(__aarch64__)

extern void call_stubless_func(void);
__ASM_GLOBAL_FUNC( call_stubless_func,
                   "stp x29, x30, [sp, #-0x90]!\n\t"
                   "mov x29, sp\n\t"
                   "stp d0, d1, [sp, #0x10]\n\t"
                   "stp d2, d3, [sp, #0x20]\n\t"
                   "stp d4, d5, [sp, #0x30]\n\t"
                   "stp d6, d7, [sp, #0x40]\n\t"
                   "stp x0, x1, [sp, #0x50]\n\t"
                   "stp x2, x3, [sp, #0x60]\n\t"
                   "stp x4, x5, [sp, #0x70]\n\t"
                   "stp x6, x7, [sp, #0x80]\n\t"
                   "ldr x0, [x0]\n\t"                /* This->lpVtbl */
                   "ldr x0, [x0, #-16]\n\t"          /* MIDL_STUBLESS_PROXY_INFO */
                   "ldp x1, x4, [x0, #8]\n\t"        /* info->ProcFormatString, FormatStringOffset */
                   "ldrh w4, [x4, x16, lsl #1]\n\t"  /* info->FormatStringOffset[index] */
                   "add x1, x1, x4\n\t"              /* info->ProcFormatString + offset */
                   "ldr x0, [x0]\n\t"                /* info->pStubDesc */
                   "add x2, sp, #0x50\n\t"           /* stack */
                   "add x3, sp, #0x10\n\t"           /* fpu_stack */
                   "bl " __ASM_NAME("ndr_client_call") "\n\t"
                   "ldp x29, x30, [sp], #0x90\n\t"
                   "ret" )

struct thunk
{
    DWORD ldr_index;     /* ldr w16, index */
    DWORD ldr_func;      /* ldr x17, func */
    DWORD br;            /* br x17 */
    DWORD index;
    void *func;
};

static inline void init_thunk( struct thunk *thunk, unsigned int index )
{
    thunk->ldr_index = 0x18000070; /* ldr w16,index */
    thunk->ldr_func  = 0x58000071; /* ldr x17,func */
    thunk->br        = 0xd61f0220; /* br x17 */
    thunk->index     = index;
    thunk->func      = call_stubless_func;
}

#else  /* __i386__ */

#warning You must implement stubless proxies for your CPU

struct thunk
{
  DWORD index;
};

static inline void init_thunk( struct thunk *thunk, unsigned int index )
{
    thunk->index = index;
}

#endif  /* __i386__ */

#define BLOCK_SIZE 1024
#define MAX_BLOCKS 64  /* 64k methods should be enough for anybody */

static const struct thunk *method_blocks[MAX_BLOCKS];

static const struct thunk *allocate_block( unsigned int num )
{
    unsigned int i;
    struct thunk *prev, *block;
    DWORD oldprot;

    block = VirtualAlloc( NULL, BLOCK_SIZE * sizeof(*block),
                          MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if (!block) return NULL;

    for (i = 0; i < BLOCK_SIZE; i++) init_thunk( &block[i], BLOCK_SIZE * num + i + 3 );
    VirtualProtect( block, BLOCK_SIZE * sizeof(*block), PAGE_EXECUTE_READ, &oldprot );
    prev = InterlockedCompareExchangePointer( (void **)&method_blocks[num], block, NULL );
    if (prev) /* someone beat us to it */
    {
        VirtualFree( block, 0, MEM_RELEASE );
        block = prev;
    }
    return block;
}

BOOL fill_stubless_table( IUnknownVtbl *vtbl, DWORD num )
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
          HeapFree( GetProcessHeap(), 0, This );
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
    HeapFree(GetProcessHeap(),0,This);
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
