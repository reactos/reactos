/*
 * HGLOBAL Stream implementation
 *
 * This file contains the implementation of the stream interface
 * for streams contained supported by an HGLOBAL pointer.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 2016 Dmitry Timoshkov
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

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hglobalstream);

struct handle_wrapper
{
    LONG ref;
    HGLOBAL hglobal;
    ULONG size;
    BOOL delete_on_release;
    CRITICAL_SECTION lock;
};

static void handle_addref(struct handle_wrapper *handle)
{
    InterlockedIncrement(&handle->ref);
}

static void handle_release(struct handle_wrapper *handle)
{
    ULONG ref = InterlockedDecrement(&handle->ref);

    if (!ref)
    {
        if (handle->delete_on_release)
        {
            GlobalFree(handle->hglobal);
            handle->hglobal = NULL;
        }

        handle->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&handle->lock);
        HeapFree(GetProcessHeap(), 0, handle);
    }
}

static ULONG handle_read(struct handle_wrapper *handle, ULONG *pos, void *dest, ULONG len)
{
    void *source;

    EnterCriticalSection(&handle->lock);

    if (*pos < handle->size)
        len = min(handle->size - *pos, len);
    else
        len = 0;

    source = GlobalLock(handle->hglobal);
    if (source)
    {
        memcpy(dest, (char *)source + *pos, len);
        *pos += len;
        GlobalUnlock(handle->hglobal);
    }
    else
    {
        WARN("read from invalid hglobal %p\n", handle->hglobal);
        len = 0;
    }

    LeaveCriticalSection(&handle->lock);
    return len;
}

static ULONG handle_write(struct handle_wrapper *handle, ULONG *pos, const void *source, ULONG len)
{
    void *dest;

    if (!len)
        return 0;

    EnterCriticalSection(&handle->lock);

    if (*pos + len > handle->size)
    {
        HGLOBAL hglobal = GlobalReAlloc(handle->hglobal, *pos + len, GMEM_MOVEABLE);
        if (hglobal)
        {
            handle->hglobal = hglobal;
            handle->size = *pos + len;
        }
        else
        {
            len = 0;
            goto done;
        }
    }

    dest = GlobalLock(handle->hglobal);
    if (dest)
    {
        memcpy((char *)dest + *pos, source, len);
        *pos += len;
        GlobalUnlock(handle->hglobal);
    }
    else
    {
        WARN("write to invalid hglobal %p\n", handle->hglobal);
        /* len = 0; */
    }

done:
    LeaveCriticalSection(&handle->lock);
    return len;
}

static HGLOBAL handle_gethglobal(struct handle_wrapper *handle)
{
    return handle->hglobal;
}

static HRESULT handle_setsize(struct handle_wrapper *handle, ULONG size)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&handle->lock);

    if (handle->size != size)
    {
        HGLOBAL hglobal = GlobalReAlloc(handle->hglobal, size, GMEM_MOVEABLE);
        if (hglobal)
        {
            handle->hglobal = hglobal;
            handle->size = size;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    LeaveCriticalSection(&handle->lock);
    return hr;
}

static ULONG handle_getsize(struct handle_wrapper *handle)
{
    return handle->size;
}

static struct handle_wrapper *handle_create(HGLOBAL hglobal, BOOL delete_on_release)
{
    struct handle_wrapper *handle;

    handle = HeapAlloc(GetProcessHeap(), 0, sizeof(*handle));
    if (handle)
    {
        handle->ref = 1;
        handle->hglobal = hglobal;
        handle->size = GlobalSize(hglobal);
        handle->delete_on_release = delete_on_release;
        InitializeCriticalSection(&handle->lock);
        handle->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": handle_wrapper.lock");
    }
    return handle;
}

/****************************************************************************
 * HGLOBALStreamImpl definition.
 *
 * This class implements the IStream interface and represents a stream
 * supported by an HGLOBAL pointer.
 */
typedef struct
{
  IStream IStream_iface;
  LONG ref;

  struct handle_wrapper *handle;

  /* current position of the cursor */
  ULARGE_INTEGER currentPosition;
} HGLOBALStreamImpl;

static inline HGLOBALStreamImpl *impl_from_IStream(IStream *iface)
{
  return CONTAINING_RECORD(iface, HGLOBALStreamImpl, IStream_iface);
}

static HRESULT WINAPI HGLOBALStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */
		  void**         ppvObject)   /* [iid_is][out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  if (ppvObject==0)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_ISequentialStream, riid) ||
      IsEqualIID(&IID_IStream, riid))
  {
    *ppvObject = &This->IStream_iface;
  }

  if ((*ppvObject)==0)
    return E_NOINTERFACE;

  IStream_AddRef(iface);

  return S_OK;
}

static ULONG WINAPI HGLOBALStreamImpl_AddRef(IStream* iface)
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI HGLOBALStreamImpl_Release(
		IStream* iface)
{
  HGLOBALStreamImpl* This= impl_from_IStream(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  if (!ref)
  {
    handle_release(This->handle);
    HeapFree(GetProcessHeap(), 0, This);
  }

  return ref;
}

/***
 * This method is part of the ISequentialStream interface.
 *
 * If reads a block of information from the stream at the current
 * position. It then moves the current position at the end of the
 * read block
 *
 * See the documentation of ISequentialStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Read(
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */
		  ULONG*         pcbRead)   /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);
  ULONG num_bytes;

  TRACE("(%p, %p, %d, %p)\n", iface, pv, cb, pcbRead);

  num_bytes = handle_read(This->handle, &This->currentPosition.u.LowPart, pv, cb);
  if (pcbRead) *pcbRead = num_bytes;

  return S_OK;
}

/***
 * This method is part of the ISequentialStream interface.
 *
 * It writes a block of information to the stream at the current
 * position. It then moves the current position at the end of the
 * written block. If the stream is too small to fit the block,
 * the stream is grown to fit.
 *
 * See the documentation of ISequentialStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */
		  ULONG          cb,          /* [in] */
		  ULONG*         pcbWritten)  /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);
  ULONG num_bytes;

  TRACE("(%p, %p, %d, %p)\n", iface, pv, cb, pcbWritten);

  num_bytes = handle_write(This->handle, &This->currentPosition.u.LowPart, pv, cb);
  if (pcbWritten) *pcbWritten = num_bytes;

  return (num_bytes < cb) ? E_OUTOFMEMORY : S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will move the current stream pointer according to the parameters
 * given.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Seek(
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */
		  DWORD           dwOrigin,         /* [in] */
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  ULARGE_INTEGER newPosition = This->currentPosition;
  HRESULT hr = S_OK;

  TRACE("(%p, %x%08x, %d, %p)\n", iface, dlibMove.u.HighPart,
	dlibMove.u.LowPart, dwOrigin, plibNewPosition);

  /*
   * The file pointer is moved depending on the given "function"
   * parameter.
   */
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      newPosition.u.HighPart = 0;
      newPosition.u.LowPart = 0;
      break;
    case STREAM_SEEK_CUR:
      break;
    case STREAM_SEEK_END:
      newPosition.QuadPart = handle_getsize(This->handle);
      break;
    default:
      hr = STG_E_SEEKERROR;
      goto end;
  }

  /*
   * Move the actual file pointer
   * If the file pointer ends-up after the end of the stream, the next Write operation will
   * make the file larger. This is how it is documented.
   */
  newPosition.u.HighPart = 0;
  newPosition.u.LowPart += dlibMove.QuadPart;

  if (dlibMove.u.LowPart >= 0x80000000 &&
      newPosition.u.LowPart >= dlibMove.u.LowPart)
  {
    /* We tried to seek backwards and went past the start. */
    hr = STG_E_SEEKERROR;
    goto end;
  }

  This->currentPosition = newPosition;

end:
  if (plibNewPosition) *plibNewPosition = This->currentPosition;

  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * It will change the size of a stream.
 *
 * TODO: Switch from small blocks to big blocks and vice versa.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_SetSize(
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  TRACE("(%p, %d)\n", iface, libNewSize.u.LowPart);

  /*
   * HighPart is ignored as shown in tests
   */
  return handle_setsize(This->handle, libNewSize.u.LowPart);
}

/***
 * This method is part of the IStream interface.
 *
 * It will copy the 'cb' Bytes to 'pstm' IStream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_CopyTo(
				    IStream*      iface,
				    IStream*      pstm,         /* [unique][in] */
				    ULARGE_INTEGER  cb,           /* [in] */
				    ULARGE_INTEGER* pcbRead,      /* [out] */
				    ULARGE_INTEGER* pcbWritten)   /* [out] */
{
  HRESULT        hr = S_OK;
  BYTE           tmpBuffer[128];
  ULONG          bytesRead, bytesWritten, copySize;
  ULARGE_INTEGER totalBytesRead;
  ULARGE_INTEGER totalBytesWritten;

  TRACE("(%p, %p, %d, %p, %p)\n", iface, pstm,
	cb.u.LowPart, pcbRead, pcbWritten);

  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;

  totalBytesRead.QuadPart = 0;
  totalBytesWritten.QuadPart = 0;

  while ( cb.QuadPart > 0 )
  {
    if ( cb.QuadPart >= sizeof(tmpBuffer) )
      copySize = sizeof(tmpBuffer);
    else
      copySize = cb.u.LowPart;

    hr = IStream_Read(iface, tmpBuffer, copySize, &bytesRead);
    if (FAILED(hr))
        break;

    totalBytesRead.QuadPart += bytesRead;

    if (bytesRead)
    {
        hr = IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);
        if (FAILED(hr))
            break;

        totalBytesWritten.QuadPart += bytesWritten;
    }

    if (bytesRead!=copySize)
      cb.QuadPart = 0;
    else
      cb.QuadPart -= bytesRead;
  }

  if (pcbRead) pcbRead->QuadPart = totalBytesRead.QuadPart;
  if (pcbWritten) pcbWritten->QuadPart = totalBytesWritten.QuadPart;

  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Commit(
		  IStream*      iface,
		  DWORD         grfCommitFlags)  /* [in] */
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Revert(
		  IStream* iface)
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_LockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return STG_E_INVALIDFUNCTION;
}

/*
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_UnlockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * This method returns information about the current
 * stream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Stat(
		  IStream*     iface,
		  STATSTG*     pstatstg,     /* [out] */
		  DWORD        grfStatFlag)  /* [in] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  memset(pstatstg, 0, sizeof(STATSTG));

  pstatstg->pwcsName = NULL;
  pstatstg->type     = STGTY_STREAM;
  pstatstg->cbSize.QuadPart = handle_getsize(This->handle);

  return S_OK;
}

static const IStreamVtbl HGLOBALStreamImplVtbl;

static HGLOBALStreamImpl *HGLOBALStreamImpl_Create(void)
{
    HGLOBALStreamImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (This)
    {
        This->IStream_iface.lpVtbl = &HGLOBALStreamImplVtbl;
        This->ref = 1;
    }
    return This;
}

static HRESULT WINAPI HGLOBALStreamImpl_Clone(
		  IStream*     iface,
		  IStream**    ppstm) /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);
  HGLOBALStreamImpl* clone;
  ULARGE_INTEGER dummy;
  LARGE_INTEGER offset;

  if (!ppstm) return E_INVALIDARG;

  *ppstm = NULL;

  TRACE(" Cloning %p (seek position=%d)\n", iface, This->currentPosition.u.LowPart);

  clone = HGLOBALStreamImpl_Create();
  if (!clone) return E_OUTOFMEMORY;

  *ppstm = &clone->IStream_iface;

  handle_addref(This->handle);
  clone->handle = This->handle;

  offset.QuadPart = (LONGLONG)This->currentPosition.QuadPart;
  IStream_Seek(*ppstm, offset, STREAM_SEEK_SET, &dummy);
  return S_OK;
}

static const IStreamVtbl HGLOBALStreamImplVtbl =
{
    HGLOBALStreamImpl_QueryInterface,
    HGLOBALStreamImpl_AddRef,
    HGLOBALStreamImpl_Release,
    HGLOBALStreamImpl_Read,
    HGLOBALStreamImpl_Write,
    HGLOBALStreamImpl_Seek,
    HGLOBALStreamImpl_SetSize,
    HGLOBALStreamImpl_CopyTo,
    HGLOBALStreamImpl_Commit,
    HGLOBALStreamImpl_Revert,
    HGLOBALStreamImpl_LockRegion,
    HGLOBALStreamImpl_UnlockRegion,
    HGLOBALStreamImpl_Stat,
    HGLOBALStreamImpl_Clone
};

/***********************************************************************
 *           CreateStreamOnHGlobal     [OLE32.@]
 */
HRESULT WINAPI CreateStreamOnHGlobal(
		HGLOBAL   hGlobal,
		BOOL      fDeleteOnRelease,
		LPSTREAM* ppstm)
{
  HGLOBALStreamImpl* This;

  if (!ppstm)
    return E_INVALIDARG;

  This = HGLOBALStreamImpl_Create();
  if (!This) return E_OUTOFMEMORY;

  /* allocate a handle if one is not supplied */
  if (!hGlobal)
    hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_NODISCARD|GMEM_SHARE, 0);

  This->handle = handle_create(hGlobal, fDeleteOnRelease);

  /* start at the beginning */
  This->currentPosition.u.HighPart = 0;
  This->currentPosition.u.LowPart = 0;

  *ppstm = &This->IStream_iface;

  return S_OK;
}

/***********************************************************************
 *           GetHGlobalFromStream     [OLE32.@]
 */
HRESULT WINAPI GetHGlobalFromStream(IStream* pstm, HGLOBAL* phglobal)
{
  HGLOBALStreamImpl* pStream;

  if (!pstm || !phglobal)
    return E_INVALIDARG;

  pStream = impl_from_IStream(pstm);

  /*
   * Verify that the stream object was created with CreateStreamOnHGlobal.
   */
  if (pStream->IStream_iface.lpVtbl == &HGLOBALStreamImplVtbl)
    *phglobal = handle_gethglobal(pStream->handle);
  else
  {
    *phglobal = 0;
    return E_INVALIDARG;
  }

  return S_OK;
}
