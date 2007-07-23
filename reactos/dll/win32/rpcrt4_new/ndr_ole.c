/*
 * OLE32 callouts, COM interface marshalling
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
 * TODO:
 *  - fix the wire-protocol to match MS/RPC
 *  - finish RpcStream_Vtbl
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "objbase.h"

#include "ndr_misc.h"
#include "rpcndr.h"
#include "rpcproxy.h"
#include "wine/rpcfc.h"
#include "cpsf.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static HMODULE hOLE;

static HRESULT (WINAPI *COM_GetMarshalSizeMax)(ULONG *,REFIID,LPUNKNOWN,DWORD,LPVOID,DWORD);
static HRESULT (WINAPI *COM_MarshalInterface)(LPSTREAM,REFIID,LPUNKNOWN,DWORD,LPVOID,DWORD);
static HRESULT (WINAPI *COM_UnmarshalInterface)(LPSTREAM,REFIID,LPVOID*);
static HRESULT (WINAPI *COM_ReleaseMarshalData)(LPSTREAM);
static HRESULT (WINAPI *COM_GetClassObject)(REFCLSID,DWORD,COSERVERINFO *,REFIID,LPVOID *);
static HRESULT (WINAPI *COM_GetPSClsid)(REFIID,CLSID *);
static LPVOID (WINAPI *COM_MemAlloc)(ULONG);
static void (WINAPI *COM_MemFree)(LPVOID);

static HMODULE LoadCOM(void)
{
  if (hOLE) return hOLE;
  hOLE = LoadLibraryA("OLE32.DLL");
  if (!hOLE) return 0;
  COM_GetMarshalSizeMax  = (LPVOID)GetProcAddress(hOLE, "CoGetMarshalSizeMax");
  COM_MarshalInterface   = (LPVOID)GetProcAddress(hOLE, "CoMarshalInterface");
  COM_UnmarshalInterface = (LPVOID)GetProcAddress(hOLE, "CoUnmarshalInterface");
  COM_ReleaseMarshalData = (LPVOID)GetProcAddress(hOLE, "CoReleaseMarshalData");
  COM_GetClassObject     = (LPVOID)GetProcAddress(hOLE, "CoGetClassObject");
  COM_GetPSClsid         = (LPVOID)GetProcAddress(hOLE, "CoGetPSClsid");
  COM_MemAlloc = (LPVOID)GetProcAddress(hOLE, "CoTaskMemAlloc");
  COM_MemFree  = (LPVOID)GetProcAddress(hOLE, "CoTaskMemFree");
  return hOLE;
}

/* CoMarshalInterface/CoUnmarshalInterface works on streams,
 * so implement a simple stream on top of the RPC buffer
 * (which also implements the MInterfacePointer structure) */
typedef struct RpcStreamImpl
{
  const IStreamVtbl *lpVtbl;
  DWORD RefCount;
  PMIDL_STUB_MESSAGE pMsg;
  LPDWORD size;
  char *data;
  DWORD pos;
} RpcStreamImpl;

static HRESULT WINAPI RpcStream_QueryInterface(LPSTREAM iface,
                                              REFIID riid,
                                              LPVOID *obj)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_ISequentialStream, riid) ||
      IsEqualGUID(&IID_IStream, riid)) {
    *obj = This;
    This->RefCount++;
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG WINAPI RpcStream_AddRef(LPSTREAM iface)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  return ++(This->RefCount);
}

static ULONG WINAPI RpcStream_Release(LPSTREAM iface)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  if (!--(This->RefCount)) {
    TRACE("size=%d\n", *This->size);
    This->pMsg->Buffer = (unsigned char*)This->data + *This->size;
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  return This->RefCount;
}

static HRESULT WINAPI RpcStream_Read(LPSTREAM iface,
                                    void *pv,
                                    ULONG cb,
                                    ULONG *pcbRead)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  HRESULT hr = S_OK;
  if (This->pos + cb > *This->size)
  {
    cb = *This->size - This->pos;
    hr = S_FALSE;
  }
  if (cb) {
    memcpy(pv, This->data + This->pos, cb);
    This->pos += cb;
  }
  if (pcbRead) *pcbRead = cb;
  return hr;
}

static HRESULT WINAPI RpcStream_Write(LPSTREAM iface,
                                     const void *pv,
                                     ULONG cb,
                                     ULONG *pcbWritten)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  if (This->data + cb > (char *)This->pMsg->BufferEnd)
    return STG_E_MEDIUMFULL;
  memcpy(This->data + This->pos, pv, cb);
  This->pos += cb;
  if (This->pos > *This->size) *This->size = This->pos;
  if (pcbWritten) *pcbWritten = cb;
  return S_OK;
}

static HRESULT WINAPI RpcStream_Seek(LPSTREAM iface,
                                    LARGE_INTEGER move,
                                    DWORD origin,
                                    ULARGE_INTEGER *newPos)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  switch (origin) {
  case STREAM_SEEK_SET:
    This->pos = move.u.LowPart;
    break;
  case STREAM_SEEK_CUR:
    This->pos = This->pos + move.u.LowPart;
    break;
  case STREAM_SEEK_END:
    This->pos = *This->size + move.u.LowPart;
    break;
  default:
    return STG_E_INVALIDFUNCTION;
  }
  if (newPos) {
    newPos->u.LowPart = This->pos;
    newPos->u.HighPart = 0;
  }
  return S_OK;
}

static HRESULT WINAPI RpcStream_SetSize(LPSTREAM iface,
                                       ULARGE_INTEGER newSize)
{
  RpcStreamImpl *This = (RpcStreamImpl *)iface;
  *This->size = newSize.u.LowPart;
  return S_OK;
}

static const IStreamVtbl RpcStream_Vtbl =
{
  RpcStream_QueryInterface,
  RpcStream_AddRef,
  RpcStream_Release,
  RpcStream_Read,
  RpcStream_Write,
  RpcStream_Seek,
  RpcStream_SetSize,
  NULL, /* CopyTo */
  NULL, /* Commit */
  NULL, /* Revert */
  NULL, /* LockRegion */
  NULL, /* UnlockRegion */
  NULL, /* Stat */
  NULL  /* Clone */
};

static LPSTREAM RpcStream_Create(PMIDL_STUB_MESSAGE pStubMsg, BOOL init)
{
  RpcStreamImpl *This;
  This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(RpcStreamImpl));
  if (!This) return NULL;
  This->lpVtbl = &RpcStream_Vtbl;
  This->RefCount = 1;
  This->pMsg = pStubMsg;
  This->size = (LPDWORD)pStubMsg->Buffer;
  This->data = (char*)(This->size + 1);
  This->pos = 0;
  if (init) *This->size = 0;
  TRACE("init size=%d\n", *This->size);
  return (LPSTREAM)This;
}

static const IID* get_ip_iid(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat)
{
  const IID *riid;
  if (!pFormat) return &IID_IUnknown;
  TRACE("format=%02x %02x\n", pFormat[0], pFormat[1]);
  if (pFormat[0] != RPC_FC_IP) FIXME("format=%d\n", pFormat[0]);
  if (pFormat[1] == RPC_FC_CONSTANT_IID) {
    riid = (const IID *)&pFormat[2];
  } else {
    ComputeConformance(pStubMsg, pMemory, pFormat+2, 0);
    riid = (const IID *)pStubMsg->MaxCount;
  }
  if (!riid) riid = &IID_IUnknown;
  TRACE("got %s\n", debugstr_guid(riid));
  return riid;
}

/***********************************************************************
 *           NdrInterfacePointerMarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrInterfacePointerMarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                  unsigned char *pMemory,
                                                  PFORMAT_STRING pFormat)
{
  const IID *riid = get_ip_iid(pStubMsg, pMemory, pFormat);
  LPSTREAM stream;
  HRESULT hr;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  pStubMsg->MaxCount = 0;
  if (!LoadCOM()) return NULL;
  if (pStubMsg->Buffer + sizeof(DWORD) <= (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength) {
    stream = RpcStream_Create(pStubMsg, TRUE);
    if (stream) {
      if (pMemory)
        hr = COM_MarshalInterface(stream, riid, (LPUNKNOWN)pMemory,
                                  pStubMsg->dwDestContext, pStubMsg->pvDestContext,
                                  MSHLFLAGS_NORMAL);
      else
        hr = S_OK;

      IStream_Release(stream);
      if (FAILED(hr))
        RpcRaiseException(hr);
    }
  }
  return NULL;
}

/***********************************************************************
 *           NdrInterfacePointerUnmarshall [RPCRT4.@]
 */
unsigned char * WINAPI NdrInterfacePointerUnmarshall(PMIDL_STUB_MESSAGE pStubMsg,
                                                    unsigned char **ppMemory,
                                                    PFORMAT_STRING pFormat,
                                                    unsigned char fMustAlloc)
{
  LPSTREAM stream;
  HRESULT hr;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);
  if (!LoadCOM()) return NULL;
  *(LPVOID*)ppMemory = NULL;
  if (pStubMsg->Buffer + sizeof(DWORD) < (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength) {
    stream = RpcStream_Create(pStubMsg, FALSE);
    if (!stream) RpcRaiseException(E_OUTOFMEMORY);
    if (*((RpcStreamImpl *)stream)->size != 0)
      hr = COM_UnmarshalInterface(stream, &IID_NULL, (LPVOID*)ppMemory);
    else
      hr = S_OK;
    IStream_Release(stream);
    if (FAILED(hr))
        RpcRaiseException(hr);
  }
  return NULL;
}

/***********************************************************************
 *           NdrInterfacePointerBufferSize [RPCRT4.@]
 */
void WINAPI NdrInterfacePointerBufferSize(PMIDL_STUB_MESSAGE pStubMsg,
                                         unsigned char *pMemory,
                                         PFORMAT_STRING pFormat)
{
  const IID *riid = get_ip_iid(pStubMsg, pMemory, pFormat);
  ULONG size = 0;
  HRESULT hr;

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (!LoadCOM()) return;
  hr = COM_GetMarshalSizeMax(&size, riid, (LPUNKNOWN)pMemory,
                            pStubMsg->dwDestContext, pStubMsg->pvDestContext,
                            MSHLFLAGS_NORMAL);
  TRACE("size=%d\n", size);
  pStubMsg->BufferLength += sizeof(DWORD) + size;
}

/***********************************************************************
 *           NdrInterfacePointerMemorySize [RPCRT4.@]
 */
ULONG WINAPI NdrInterfacePointerMemorySize(PMIDL_STUB_MESSAGE pStubMsg,
                                           PFORMAT_STRING pFormat)
{
  ULONG size;

  TRACE("(%p,%p)\n", pStubMsg, pFormat);

  size = *(ULONG *)pStubMsg->Buffer;
  pStubMsg->Buffer += 4;
  pStubMsg->MemorySize += 4;

  pStubMsg->Buffer += size;

  return pStubMsg->MemorySize;
}

/***********************************************************************
 *           NdrInterfacePointerFree [RPCRT4.@]
 */
void WINAPI NdrInterfacePointerFree(PMIDL_STUB_MESSAGE pStubMsg,
                                   unsigned char *pMemory,
                                   PFORMAT_STRING pFormat)
{
  LPUNKNOWN pUnk = (LPUNKNOWN)pMemory;
  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  if (pUnk) IUnknown_Release(pUnk);
}

/***********************************************************************
 *           NdrOleAllocate [RPCRT4.@]
 */
void * WINAPI NdrOleAllocate(size_t Size)
{
  if (!LoadCOM()) return NULL;
  return COM_MemAlloc(Size);
}

/***********************************************************************
 *           NdrOleFree [RPCRT4.@]
 */
void WINAPI NdrOleFree(void *NodeToFree)
{
  if (!LoadCOM()) return;
  COM_MemFree(NodeToFree);
}

/***********************************************************************
 * Helper function to create a stub.
 * This probably looks very much like NdrpCreateStub.
 */
HRESULT create_stub(REFIID iid, IUnknown *pUnk, IRpcStubBuffer **ppstub)
{
    CLSID clsid;
    IPSFactoryBuffer *psfac;
    HRESULT r;

    if(!LoadCOM()) return E_FAIL;

    r = COM_GetPSClsid( iid, &clsid );
    if(FAILED(r)) return r;

    r = COM_GetClassObject( &clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (void**)&psfac );
    if(FAILED(r)) return r;

    r = IPSFactoryBuffer_CreateStub(psfac, iid, pUnk, ppstub);

    IPSFactoryBuffer_Release(psfac);
    return r;
}
