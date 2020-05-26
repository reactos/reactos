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
#include "wine/exception.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define STUB_HEADER(This) (((const CInterfaceStubHeader*)((This)->lpVtbl))[-1])

static LONG WINAPI stub_filter(EXCEPTION_POINTERS *eptr)
{
    if (eptr->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
        return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_EXECUTE_HANDLER;
}

static inline cstdstubbuffer_delegating_t *impl_from_delegating( IRpcStubBuffer *iface )
{
    return CONTAINING_RECORD((void *)iface, cstdstubbuffer_delegating_t, stub_buffer);
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

  This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CStdStubBuffer));
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

static CRITICAL_SECTION delegating_vtbl_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &delegating_vtbl_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": delegating_vtbl_section") }
};
static CRITICAL_SECTION delegating_vtbl_section = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef struct
{
    DWORD ref;
    DWORD size;
    IUnknownVtbl vtbl;
    /* remaining entries in vtbl */
} ref_counted_vtbl;

static ref_counted_vtbl *current_vtbl;


static HRESULT WINAPI delegating_QueryInterface(IUnknown *pUnk, REFIID iid, void **ppv)
{
    *ppv = pUnk;
    return S_OK;
}

static ULONG WINAPI delegating_AddRef(IUnknown *pUnk)
{
    return 1;
}

static ULONG WINAPI delegating_Release(IUnknown *pUnk)
{
    return 1;
}

/* The idea here is to replace the first param on the stack
   ie. This (which will point to cstdstubbuffer_delegating_t)
   with This->stub_buffer.pvServerObject and then jump to the
   relevant offset in This->stub_buffer.pvServerObject's vtbl.
*/
#ifdef __i386__

#include "pshpack1.h"
typedef struct {
    BYTE mov1[4];    /* mov 0x4(%esp),%eax     8b 44 24 04 */
    BYTE mov2[3];    /* mov 0x10(%eax),%eax    8b 40 10 */
    BYTE mov3[4];    /* mov %eax,0x4(%esp)     89 44 24 04 */
    BYTE mov4[2];    /* mov (%eax),%eax        8b 00 */
    BYTE mov5[2];    /* jmp *offset(%eax)      ff a0 offset */
    DWORD offset;
    BYTE pad[1];     /* nop                    90 */
} vtbl_method_t;
#include "poppack.h"

static const BYTE opcodes[20] = { 0x8b, 0x44, 0x24, 0x04, 0x8b, 0x40, 0x10, 0x89, 0x44, 0x24, 0x04,
                                  0x8b, 0x00, 0xff, 0xa0, 0, 0, 0, 0, 0x90 };

#elif defined(__x86_64__)

#include "pshpack1.h"
typedef struct
{
    BYTE mov1[4];    /* movq 0x20(%rcx),%rcx   48 8b 49 20 */
    BYTE mov2[3];    /* movq (%rcx),%rax       48 8b 01 */
    BYTE jmp[2];     /* jmp *offset(%rax)      ff a0 offset */
    DWORD offset;
    BYTE pad[3];     /* lea 0x0(%rsi),%rsi     48 8d 36 */
} vtbl_method_t;
#include "poppack.h"

static const BYTE opcodes[16] = { 0x48, 0x8b, 0x49, 0x20, 0x48, 0x8b, 0x01,
                                  0xff, 0xa0, 0, 0, 0, 0, 0x48, 0x8d, 0x36 };
#elif defined(__arm__)

static const DWORD opcodes[] =
{
    0xe52d4004,    /* push {r4} */
    0xe5900010,    /* ldr r0, [r0, #16] */
    0xe5904000,    /* ldr r4, [r0] */
    0xe59fc008,    /* ldr ip, [pc, #8] */
    0xe08cc004,    /* add ip, ip, r4 */
    0xe49d4004,    /* pop {r4} */
    0xe59cf000     /* ldr pc, [ip] */
};

typedef struct
{
    DWORD opcodes[ARRAY_SIZE(opcodes)];
    DWORD offset;
} vtbl_method_t;

#elif defined(__aarch64__)

static const DWORD opcodes[] =
{
    0xf9401000,   /* ldr x0, [x0,#32] */
    0xf9400010,   /* ldr x16, [x0] */
    0x18000071,   /* ldr w17, offset */
    0xf8716a10,   /* ldr x16, [x16,x17] */
    0xd61f0200    /* br x16 */
};

typedef struct
{
    DWORD opcodes[ARRAY_SIZE(opcodes)];
    DWORD offset;
} vtbl_method_t;

#else

#warning You must implement delegated proxies/stubs for your CPU
typedef struct
{
    DWORD offset;
} vtbl_method_t;
static const BYTE opcodes[1];

#endif

#define BLOCK_SIZE 1024
#define MAX_BLOCKS 64  /* 64k methods should be enough for anybody */

static const vtbl_method_t *method_blocks[MAX_BLOCKS];

static const vtbl_method_t *allocate_block( unsigned int num )
{
    unsigned int i;
    vtbl_method_t *prev, *block;
    DWORD oldprot;

    block = VirtualAlloc( NULL, BLOCK_SIZE * sizeof(*block),
                          MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if (!block) return NULL;

    for (i = 0; i < BLOCK_SIZE; i++)
    {
        memcpy( &block[i], opcodes, sizeof(opcodes) );
        block[i].offset = (BLOCK_SIZE * num + i + 3) * sizeof(void *);
    }
    VirtualProtect( block, BLOCK_SIZE * sizeof(*block), PAGE_EXECUTE_READ, &oldprot );
    prev = InterlockedCompareExchangePointer( (void **)&method_blocks[num], block, NULL );
    if (prev) /* someone beat us to it */
    {
        VirtualFree( block, 0, MEM_RELEASE );
        block = prev;
    }
    return block;
}

static BOOL fill_delegated_stub_table(IUnknownVtbl *vtbl, DWORD num)
{
    const void **entry = (const void **)(vtbl + 1);
    DWORD i, j;

    if (num - 3 > BLOCK_SIZE * MAX_BLOCKS)
    {
        FIXME( "%u methods not supported\n", num );
        return FALSE;
    }
    vtbl->QueryInterface = delegating_QueryInterface;
    vtbl->AddRef = delegating_AddRef;
    vtbl->Release = delegating_Release;
    for (i = 0; i < (num - 3 + BLOCK_SIZE - 1) / BLOCK_SIZE; i++)
    {
        const vtbl_method_t *block = method_blocks[i];
        if (!block && !(block = allocate_block( i ))) return FALSE;
        for (j = 0; j < BLOCK_SIZE && j < num - 3 - i * BLOCK_SIZE; j++) *entry++ = &block[j];
    }
    return TRUE;
}

BOOL fill_delegated_proxy_table(IUnknownVtbl *vtbl, DWORD num)
{
    const void **entry = (const void **)(vtbl + 1);
    DWORD i, j;

    if (num - 3 > BLOCK_SIZE * MAX_BLOCKS)
    {
        FIXME( "%u methods not supported\n", num );
        return FALSE;
    }
    vtbl->QueryInterface = IUnknown_QueryInterface_Proxy;
    vtbl->AddRef = IUnknown_AddRef_Proxy;
    vtbl->Release = IUnknown_Release_Proxy;
    for (i = 0; i < (num - 3 + BLOCK_SIZE - 1) / BLOCK_SIZE; i++)
    {
        const vtbl_method_t *block = method_blocks[i];
        if (!block && !(block = allocate_block( i ))) return FALSE;
        for (j = 0; j < BLOCK_SIZE && j < num - 3 - i * BLOCK_SIZE; j++, entry++)
            if (!*entry) *entry = &block[j];
    }
    return TRUE;
}

IUnknownVtbl *get_delegating_vtbl(DWORD num_methods)
{
    IUnknownVtbl *ret;

    if (num_methods < 256) num_methods = 256;  /* avoid frequent reallocations */

    EnterCriticalSection(&delegating_vtbl_section);

    if(!current_vtbl || num_methods > current_vtbl->size)
    {
        ref_counted_vtbl *table = HeapAlloc(GetProcessHeap(), 0,
                                            FIELD_OFFSET(ref_counted_vtbl, vtbl) + num_methods * sizeof(void*));
        if (!table)
        {
            LeaveCriticalSection(&delegating_vtbl_section);
            return NULL;
        }

        table->ref = 0;
        table->size = num_methods;
        fill_delegated_stub_table(&table->vtbl, num_methods);

        if (current_vtbl && current_vtbl->ref == 0)
        {
            TRACE("freeing old table\n");
            HeapFree(GetProcessHeap(), 0, current_vtbl);
        }
        current_vtbl = table;
    }

    current_vtbl->ref++;
    ret = &current_vtbl->vtbl;
    LeaveCriticalSection(&delegating_vtbl_section);
    return ret;
}

void release_delegating_vtbl(IUnknownVtbl *vtbl)
{
    ref_counted_vtbl *table = (ref_counted_vtbl*)((DWORD *)vtbl - 1);

    EnterCriticalSection(&delegating_vtbl_section);
    table->ref--;
    TRACE("ref now %d\n", table->ref);
    if(table->ref == 0 && table != current_vtbl)
    {
        TRACE("... and we're not current so free'ing\n");
        HeapFree(GetProcessHeap(), 0, table);
    }
    LeaveCriticalSection(&delegating_vtbl_section);
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

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (!This)
    {
        IUnknown_Release(pvServer);
        return E_OUTOFMEMORY;
    }

    This->base_obj = get_delegating_vtbl( vtbl->header.DispatchTableCount );
    r = create_stub(delegating_iid, (IUnknown*)&This->base_obj, &This->base_stub);
    if(FAILED(r))
    {
        release_delegating_vtbl(This->base_obj);
        HeapFree(GetProcessHeap(), 0, This);
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
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
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
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->AddRef()\n",This);
  return InterlockedIncrement(&This->RefCount);
}

ULONG WINAPI NdrCStdStubBuffer_Release(LPRPCSTUBBUFFER iface,
                                      LPPSFACTORYBUFFER pPSF)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  ULONG refs;

  TRACE("(%p)->Release()\n",This);

  refs = InterlockedDecrement(&This->RefCount);
  if (!refs)
  {
    /* test_Release shows that native doesn't call Disconnect here.
       We'll leave it in for the time being. */
    IRpcStubBuffer_Disconnect(iface);

    IPSFactoryBuffer_Release(pPSF);
    HeapFree(GetProcessHeap(),0,This);
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
        release_delegating_vtbl(This->base_obj);

        IPSFactoryBuffer_Release(pPSF);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refs;
}

HRESULT WINAPI CStdStubBuffer_Connect(LPRPCSTUBBUFFER iface,
                                     LPUNKNOWN lpUnkServer)
{
    CStdStubBuffer *This = (CStdStubBuffer *)iface;
    HRESULT r;
    IUnknown *new = NULL;

    TRACE("(%p)->Connect(%p)\n",This,lpUnkServer);

    r = IUnknown_QueryInterface(lpUnkServer, STUB_HEADER(This).piid, (void**)&new);
    new = InterlockedExchangePointer((void**)&This->pvServerObject, new);
    if(new)
        IUnknown_Release(new);
    return r;
}

void WINAPI CStdStubBuffer_Disconnect(LPRPCSTUBBUFFER iface)
{
    CStdStubBuffer *This = (CStdStubBuffer *)iface;
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
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  DWORD dwPhase = STUB_UNMARSHAL;
  HRESULT hr = S_OK;

  TRACE("(%p)->Invoke(%p,%p)\n",This,pMsg,pChannel);

  __TRY
  {
    if (STUB_HEADER(This).pDispatchTable)
      STUB_HEADER(This).pDispatchTable[pMsg->iMethod](iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
    else /* pure interpreted */
      NdrStubCall2(iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
  }
  __EXCEPT(stub_filter)
  {
    DWORD dwExceptionCode = GetExceptionCode();
    WARN("a stub call failed with exception 0x%08x (%d)\n", dwExceptionCode, dwExceptionCode);
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
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->IsIIDSupported(%s)\n",This,debugstr_guid(riid));
  return IsEqualGUID(STUB_HEADER(This).piid, riid) ? iface : NULL;
}

ULONG WINAPI CStdStubBuffer_CountRefs(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->CountRefs()\n",This);
  return This->RefCount;
}

HRESULT WINAPI CStdStubBuffer_DebugServerQueryInterface(LPRPCSTUBBUFFER iface,
                                                       LPVOID *ppv)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->DebugServerQueryInterface(%p)\n",This,ppv);
  return S_OK;
}

void WINAPI CStdStubBuffer_DebugServerRelease(LPRPCSTUBBUFFER iface,
                                             LPVOID pv)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
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
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  return STUB_HEADER(This).pServerInfo;
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
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  HRESULT hr;

  TRACE("(%p, %p, %p)\n", This, pRpcChannelBuffer, pStubMsg);

  pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
  hr = IRpcChannelBuffer_GetBuffer(pRpcChannelBuffer,
    (RPCOLEMESSAGE *)pStubMsg->RpcMsg, STUB_HEADER(This).piid);
  if (FAILED(hr))
  {
    RpcRaiseException(hr);
    return;
  }

  pStubMsg->Buffer = pStubMsg->RpcMsg->Buffer;
}
