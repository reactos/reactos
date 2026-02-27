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
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "objbase.h"

#include "ndr_misc.h"
#include "rpcndr.h"
#include "ndrtypes.h"
#include "rpcproxy.h"
#include "cpsf.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* CoMarshalInterface/CoUnmarshalInterface works on streams,
 * so implement a simple stream on top of the RPC buffer
 * (which also implements the MInterfacePointer structure) */
typedef struct RpcStreamImpl
{
  IStream IStream_iface;
  LONG RefCount;
  PMIDL_STUB_MESSAGE pMsg;
  LPDWORD size;
  unsigned char *data;
  DWORD pos;
} RpcStreamImpl;

static inline RpcStreamImpl *impl_from_IStream(IStream *iface)
{
  return CONTAINING_RECORD(iface, RpcStreamImpl, IStream_iface);
}

static HRESULT WINAPI RpcStream_QueryInterface(LPSTREAM iface,
                                              REFIID riid,
                                              LPVOID *obj)
{
  if (IsEqualGUID(&IID_IUnknown, riid) ||
      IsEqualGUID(&IID_ISequentialStream, riid) ||
      IsEqualGUID(&IID_IStream, riid)) {
    *obj = iface;
    IStream_AddRef(iface);
    return S_OK;
  }

  *obj = NULL;
  return E_NOINTERFACE;
}

static ULONG WINAPI RpcStream_AddRef(LPSTREAM iface)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  return InterlockedIncrement( &This->RefCount );
}

static ULONG WINAPI RpcStream_Release(LPSTREAM iface)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  ULONG ref = InterlockedDecrement( &This->RefCount );
  if (!ref) {
    TRACE("size=%ld\n", *This->size);
    This->pMsg->Buffer = This->data + *This->size;
    free(This);
  }
  return ref;
}

static HRESULT WINAPI RpcStream_Read(LPSTREAM iface,
                                    void *pv,
                                    ULONG cb,
                                    ULONG *pcbRead)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
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
  RpcStreamImpl *This = impl_from_IStream(iface);
  if (This->data + cb > (unsigned char *)This->pMsg->RpcMsg->Buffer + This->pMsg->BufferLength)
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
  RpcStreamImpl *This = impl_from_IStream(iface);
  switch (origin) {
  case STREAM_SEEK_SET:
    This->pos = move.LowPart;
    break;
  case STREAM_SEEK_CUR:
    This->pos = This->pos + move.LowPart;
    break;
  case STREAM_SEEK_END:
    This->pos = *This->size + move.LowPart;
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
  RpcStreamImpl *This = impl_from_IStream(iface);
  *This->size = newSize.LowPart;
  return S_OK;
}

static HRESULT WINAPI RpcStream_CopyTo(IStream *iface, IStream *dest,
  ULARGE_INTEGER len, ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI RpcStream_Commit(IStream *iface, DWORD flags)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p)->(0x%08lx): stub\n", This, flags);
  return E_NOTIMPL;
}

static HRESULT WINAPI RpcStream_Revert(IStream *iface)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI RpcStream_LockRegion(IStream *iface,
  ULARGE_INTEGER offset, ULARGE_INTEGER len, DWORD locktype)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI RpcStream_UnlockRegion(IStream *iface,
  ULARGE_INTEGER offset, ULARGE_INTEGER len, DWORD locktype)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI RpcStream_Stat(IStream *iface, STATSTG *stat, DWORD flag)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
}

static HRESULT WINAPI RpcStream_Clone(IStream *iface, IStream **cloned)
{
  RpcStreamImpl *This = impl_from_IStream(iface);
  FIXME("(%p): stub\n", This);
  return E_NOTIMPL;
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
  RpcStream_CopyTo,
  RpcStream_Commit,
  RpcStream_Revert,
  RpcStream_LockRegion,
  RpcStream_UnlockRegion,
  RpcStream_Stat,
  RpcStream_Clone
};

static HRESULT RpcStream_Create(PMIDL_STUB_MESSAGE pStubMsg, BOOL init, ULONG *size, IStream **stream)
{
  RpcStreamImpl *This;

  *stream = NULL;
  This = malloc(sizeof(RpcStreamImpl));
  if (!This) return E_OUTOFMEMORY;
  This->IStream_iface.lpVtbl = &RpcStream_Vtbl;
  This->RefCount = 1;
  This->pMsg = pStubMsg;
  This->size = (LPDWORD)pStubMsg->Buffer;
  This->data = pStubMsg->Buffer + sizeof(DWORD);
  This->pos = 0;
  if (init) *This->size = 0;
  TRACE("init size=%ld\n", *This->size);

  if (size) *size = *This->size;
  *stream = &This->IStream_iface;
  return S_OK;
}

static const IID* get_ip_iid(PMIDL_STUB_MESSAGE pStubMsg, unsigned char *pMemory, PFORMAT_STRING pFormat)
{
  const IID *riid;
  if (!pFormat) return &IID_IUnknown;
  TRACE("format=%02x %02x\n", pFormat[0], pFormat[1]);
  if (pFormat[0] != FC_IP) FIXME("format=%d\n", pFormat[0]);
  if (pFormat[1] == FC_CONSTANT_IID) {
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
  if (pStubMsg->Buffer + sizeof(DWORD) <= (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength) {
    hr = RpcStream_Create(pStubMsg, TRUE, NULL, &stream);
    if (hr == S_OK) {
      if (pMemory)
        hr = CoMarshalInterface(stream, riid, (IUnknown *)pMemory,
                                pStubMsg->dwDestContext, pStubMsg->pvDestContext,
                                MSHLFLAGS_NORMAL);
      IStream_Release(stream);
    }

    if (FAILED(hr))
      RpcRaiseException(hr);
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
  IUnknown **unk = (IUnknown **)ppMemory;
  LPSTREAM stream;
  HRESULT hr;

  TRACE("(%p,%p,%p,%d)\n", pStubMsg, ppMemory, pFormat, fMustAlloc);

  /* Avoid reference leaks for [in, out] pointers. */
  if (pStubMsg->IsClient && *unk)
    IUnknown_Release(*unk);

  *unk = NULL;
  if (pStubMsg->Buffer + sizeof(DWORD) < (unsigned char *)pStubMsg->RpcMsg->Buffer + pStubMsg->BufferLength) {
    ULONG size;

    hr = RpcStream_Create(pStubMsg, FALSE, &size, &stream);
    if (hr == S_OK) {
      if (size != 0)
        hr = CoUnmarshalInterface(stream, &IID_NULL, (void **)unk);

      IStream_Release(stream);
    }

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

  TRACE("(%p,%p,%p)\n", pStubMsg, pMemory, pFormat);
  CoGetMarshalSizeMax(&size, riid, (IUnknown *)pMemory,
                      pStubMsg->dwDestContext, pStubMsg->pvDestContext,
                      MSHLFLAGS_NORMAL);
  TRACE("size=%ld\n", size);
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
void * WINAPI NdrOleAllocate(SIZE_T Size)
{
  return CoTaskMemAlloc(Size);
}

/***********************************************************************
 *           NdrOleFree [RPCRT4.@]
 */
void WINAPI NdrOleFree(void *NodeToFree)
{
  CoTaskMemFree(NodeToFree);
}

/***********************************************************************
 * Helper function to create a proxy.
 * Probably similar to NdrpCreateProxy.
 */
HRESULT create_proxy(REFIID iid, IUnknown *pUnkOuter, IRpcProxyBuffer **pproxy, void **ppv)
{
    CLSID clsid;
    IPSFactoryBuffer *psfac;
    HRESULT r;

    r = CoGetPSClsid(iid, &clsid);
    if(FAILED(r)) return r;

    r = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (void **)&psfac);
    if(FAILED(r)) return r;

    r = IPSFactoryBuffer_CreateProxy(psfac, pUnkOuter, iid, pproxy, ppv);

    IPSFactoryBuffer_Release(psfac);
    return r;
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

    r = CoGetPSClsid(iid, &clsid);
    if(FAILED(r)) return r;

    r = CoGetClassObject(&clsid, CLSCTX_INPROC_SERVER, NULL, &IID_IPSFactoryBuffer, (void **)&psfac);
    if(FAILED(r)) return r;

    r = IPSFactoryBuffer_CreateStub(psfac, iid, pUnk, ppstub);

    IPSFactoryBuffer_Release(psfac);
    return r;
}
