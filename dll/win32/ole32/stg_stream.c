/*
 * Compound Storage (32 bit version)
 * Stream implementation
 *
 * This file contains the implementation of the stream interface
 * for streams contained in a compound storage.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Thuy Nguyen
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/debug.h"

#include "storage32.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/***
 * This implements the IUnknown method QueryInterface for this
 * class
 */
static HRESULT WINAPI StgStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */
		  void**         ppvObject)   /* [iid_is][out] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  if (ppvObject==0)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_ISequentialStream, riid) ||
      IsEqualIID(&IID_IStream, riid))
  {
    *ppvObject = &This->IStream_iface;
  }
  else
    return E_NOINTERFACE;

  IStream_AddRef(iface);

  return S_OK;
}

/***
 * This implements the IUnknown method AddRef for this
 * class
 */
static ULONG WINAPI StgStreamImpl_AddRef(
		IStream* iface)
{
  StgStreamImpl* This = impl_from_IStream(iface);
  return InterlockedIncrement(&This->ref);
}

/***
 * This implements the IUnknown method Release for this
 * class
 */
static ULONG WINAPI StgStreamImpl_Release(
		IStream* iface)
{
  StgStreamImpl* This = impl_from_IStream(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  if (!ref)
  {
    TRACE("(%p)\n", This);

    /*
     * Release the reference we are holding on the parent storage.
     * IStorage_Release(&This->parentStorage->IStorage_iface);
     *
     * No, don't do this. Some apps call IStorage_Release without
     * calling IStream_Release first. If we grab a reference the
     * file is not closed, and the app fails when it tries to
     * reopen the file (Easy-PC, for example). Just inform the
     * storage that we have closed the stream
     */

    if (This->parentStorage)
      StorageBaseImpl_RemoveStream(This->parentStorage, This);
    This->parentStorage = 0;
    HeapFree(GetProcessHeap(), 0, This);
  }

  return ref;
}

/***
 * This method is part of the ISequentialStream interface.
 *
 * It reads a block of information from the stream at the current
 * position. It then moves the current position at the end of the
 * read block
 *
 * See the documentation of ISequentialStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_Read(
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */
		  ULONG*         pcbRead)   /* [out] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  ULONG bytesReadBuffer;
  HRESULT res;

  TRACE("%p, %p, %lu, %p.\n", iface, pv, cb, pcbRead);

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  /*
   * If the caller is not interested in the number of bytes read,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbRead==0)
    pcbRead = &bytesReadBuffer;

  res = StorageBaseImpl_StreamReadAt(This->parentStorage,
                                     This->dirEntry,
                                     This->currentPosition,
                                     cb,
                                     pv,
                                     pcbRead);

  if (SUCCEEDED(res))
  {
    /*
     * Advance the pointer for the number of positions read.
     */
    This->currentPosition.QuadPart += *pcbRead;
  }

  TRACE("<-- %#lx\n", res);
  return res;
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
static HRESULT WINAPI StgStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */
		  ULONG          cb,          /* [in] */
		  ULONG*         pcbWritten)  /* [out] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  ULONG bytesWritten = 0;
  HRESULT res;

  TRACE("%p, %p, %lu, %p.\n", iface, pv, cb, pcbWritten);

  /*
   * Do we have permission to write to this stream?
   */
  switch(STGM_ACCESS_MODE(This->grfMode))
  {
  case STGM_WRITE:
  case STGM_READWRITE:
      break;
  default:
      WARN("access denied by flags: %#lx\n", STGM_ACCESS_MODE(This->grfMode));
      return STG_E_ACCESSDENIED;
  }

  if (!pv)
    return STG_E_INVALIDPOINTER;

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }
 
  /*
   * If the caller is not interested in the number of bytes written,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;

  /*
   * Initialize the out parameter
   */
  *pcbWritten = 0;

  if (cb == 0)
  {
    TRACE("<-- S_OK, written 0\n");
    return S_OK;
  }

  res = StorageBaseImpl_StreamWriteAt(This->parentStorage,
                                      This->dirEntry,
                                      This->currentPosition,
                                      cb,
                                      pv,
                                      pcbWritten);

  /*
   * Advance the position pointer for the number of positions written.
   */
  This->currentPosition.QuadPart += *pcbWritten;

  if (SUCCEEDED(res))
    res = StorageBaseImpl_Flush(This->parentStorage);

  TRACE("<-- %#lx, written %lu\n", res, *pcbWritten);
  return res;
}

/***
 * This method is part of the IStream interface.
 *
 * It will move the current stream pointer according to the parameters
 * given.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_Seek(
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */
		  DWORD           dwOrigin,         /* [in] */
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  ULARGE_INTEGER newPosition;
  DirEntry currentEntry;
  HRESULT hr;

  TRACE("%p, %ld, %ld, %p.\n", iface, dlibMove.LowPart, dwOrigin, plibNewPosition);

  /*
   * fail if the stream has no parent (as does windows)
   */

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  /*
   * The caller is allowed to pass in NULL as the new position return value.
   * If it happens, we assign it to a dynamic variable to avoid special cases
   * in the code below.
   */
  if (plibNewPosition == 0)
  {
    plibNewPosition = &newPosition;
  }

  /*
   * The file pointer is moved depending on the given "function"
   * parameter.
   */
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      plibNewPosition->u.HighPart = 0;
      plibNewPosition->u.LowPart  = 0;
      break;
    case STREAM_SEEK_CUR:
      *plibNewPosition = This->currentPosition;
      break;
    case STREAM_SEEK_END:
      hr = StorageBaseImpl_ReadDirEntry(This->parentStorage, This->dirEntry, &currentEntry);
      if (FAILED(hr)) return hr;
      *plibNewPosition = currentEntry.size;
      break;
    default:
      WARN("invalid dwOrigin %ld\n", dwOrigin);
      return STG_E_INVALIDFUNCTION;
  }

  plibNewPosition->QuadPart += dlibMove.QuadPart;

  /*
   * tell the caller what we calculated
   */
  This->currentPosition = *plibNewPosition;

  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will change the size of a stream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_SetSize(
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  HRESULT      hr;

  TRACE("%p, %ld.\n", iface, libNewSize.LowPart);

  if(!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  /*
   * As documented.
   */
  if (libNewSize.HighPart != 0)
  {
    WARN("invalid value for libNewSize.HighPart %ld\n", libNewSize.HighPart);
    return STG_E_INVALIDFUNCTION;
  }

  /*
   * Do we have permission?
   */
  if (!(This->grfMode & (STGM_WRITE | STGM_READWRITE)))
  {
    WARN("access denied\n");
    return STG_E_ACCESSDENIED;
  }

  hr = StorageBaseImpl_StreamSetSize(This->parentStorage, This->dirEntry, libNewSize);

  if (SUCCEEDED(hr))
    hr = StorageBaseImpl_Flush(This->parentStorage);

  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * It will copy the 'cb' Bytes to 'pstm' IStream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_CopyTo(
				    IStream*      iface,
				    IStream*      pstm,         /* [unique][in] */
				    ULARGE_INTEGER  cb,           /* [in] */
				    ULARGE_INTEGER* pcbRead,      /* [out] */
				    ULARGE_INTEGER* pcbWritten)   /* [out] */
{
  StgStreamImpl* This = impl_from_IStream(iface);
  HRESULT        hr = S_OK;
  BYTE           tmpBuffer[128];
  ULONG          bytesRead, bytesWritten, copySize;
  ULARGE_INTEGER totalBytesRead;
  ULARGE_INTEGER totalBytesWritten;

  TRACE("%p, %p, %ld, %p, %p.\n", iface, pstm, cb.LowPart, pcbRead, pcbWritten);

  /*
   * Sanity check
   */

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;

  totalBytesRead.QuadPart = 0;
  totalBytesWritten.QuadPart = 0;

  while ( cb.QuadPart > 0 )
  {
    if ( cb.QuadPart >= sizeof(tmpBuffer) )
      copySize = sizeof(tmpBuffer);
    else
      copySize = cb.LowPart;

    IStream_Read(iface, tmpBuffer, copySize, &bytesRead);

    totalBytesRead.QuadPart += bytesRead;

    IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);

    totalBytesWritten.QuadPart += bytesWritten;

    /*
     * Check that read & write operations were successful
     */
    if (bytesRead != bytesWritten)
    {
      hr = STG_E_MEDIUMFULL;
      WARN("medium full\n");
      break;
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
 * For streams contained in structured storages, this method
 * does nothing. This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_Commit(
		  IStream*      iface,
		  DWORD           grfCommitFlags)  /* [in] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  return StorageBaseImpl_Flush(This->parentStorage);
}

/***
 * This method is part of the IStream interface.
 *
 * For streams contained in structured storages, this method
 * does nothing. This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_Revert(
		  IStream* iface)
{
  return S_OK;
}

static HRESULT WINAPI StgStreamImpl_LockRegion(
					IStream*     iface,
					ULARGE_INTEGER libOffset,   /* [in] */
					ULARGE_INTEGER cb,          /* [in] */
					DWORD          dwLockType)  /* [in] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  FIXME("not implemented!\n");
  return E_NOTIMPL;
}

static HRESULT WINAPI StgStreamImpl_UnlockRegion(
					  IStream*     iface,
					  ULARGE_INTEGER libOffset,   /* [in] */
					  ULARGE_INTEGER cb,          /* [in] */
					  DWORD          dwLockType)  /* [in] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  FIXME("not implemented!\n");
  return E_NOTIMPL;
}

/***
 * This method is part of the IStream interface.
 *
 * This method returns information about the current
 * stream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI StgStreamImpl_Stat(
		  IStream*     iface,
		  STATSTG*       pstatstg,     /* [out] */
		  DWORD          grfStatFlag)  /* [in] */
{
  StgStreamImpl* This = impl_from_IStream(iface);

  DirEntry     currentEntry;
  HRESULT      hr;

  TRACE("%p, %p, %#lx.\n", This, pstatstg, grfStatFlag);

  /*
   * if stream has no parent, return STG_E_REVERTED
   */

  if (!This->parentStorage)
  {
    WARN("storage reverted\n");
    return STG_E_REVERTED;
  }

  /*
   * Read the information from the directory entry.
   */
  hr = StorageBaseImpl_ReadDirEntry(This->parentStorage,
					     This->dirEntry,
					     &currentEntry);

  if (SUCCEEDED(hr))
  {
    StorageUtl_CopyDirEntryToSTATSTG(This->parentStorage,
                     pstatstg,
				     &currentEntry,
				     grfStatFlag);

    pstatstg->grfMode = This->grfMode;

    /* In simple create mode cbSize is the current pos */
    if((This->parentStorage->openFlags & STGM_SIMPLE) && This->parentStorage->create)
      pstatstg->cbSize = This->currentPosition;

    return S_OK;
  }

  WARN("failed to read entry\n");
  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * This method returns a clone of the interface that allows for
 * another seek pointer
 *
 * See the documentation of IStream for more info.
 *
 * I am not totally sure what I am doing here but I presume that this
 * should be basically as simple as creating a new stream with the same
 * parent etc and positioning its seek cursor.
 */
static HRESULT WINAPI StgStreamImpl_Clone(
				   IStream*     iface,
				   IStream**    ppstm) /* [out] */
{
  StgStreamImpl* This = impl_from_IStream(iface);
  StgStreamImpl* new_stream;
  LARGE_INTEGER seek_pos;

  TRACE("%p %p\n", This, ppstm);

  /*
   * Sanity check
   */

  if (!This->parentStorage)
    return STG_E_REVERTED;

  if ( ppstm == 0 )
    return STG_E_INVALIDPOINTER;

  new_stream = StgStreamImpl_Construct (This->parentStorage, This->grfMode, This->dirEntry);

  if (!new_stream)
    return STG_E_INSUFFICIENTMEMORY; /* Currently the only reason for new_stream=0 */

  *ppstm = &new_stream->IStream_iface;
  IStream_AddRef(*ppstm);

  seek_pos.QuadPart = This->currentPosition.QuadPart;

  return IStream_Seek(*ppstm, seek_pos, STREAM_SEEK_SET, NULL);
}

/*
 * Virtual function table for the StgStreamImpl class.
 */
static const IStreamVtbl StgStreamVtbl =
{
    StgStreamImpl_QueryInterface,
    StgStreamImpl_AddRef,
    StgStreamImpl_Release,
    StgStreamImpl_Read,
    StgStreamImpl_Write,
    StgStreamImpl_Seek,
    StgStreamImpl_SetSize,
    StgStreamImpl_CopyTo,
    StgStreamImpl_Commit,
    StgStreamImpl_Revert,
    StgStreamImpl_LockRegion,
    StgStreamImpl_UnlockRegion,
    StgStreamImpl_Stat,
    StgStreamImpl_Clone
};

/******************************************************************************
** StgStreamImpl implementation
*/

/***
 * This is the constructor for the StgStreamImpl class.
 *
 * Params:
 *    parentStorage - Pointer to the storage that contains the stream to open
 *    dirEntry      - Index of the directory entry that points to this stream.
 */
StgStreamImpl* StgStreamImpl_Construct(
		StorageBaseImpl* parentStorage,
    DWORD            grfMode,
    DirRef           dirEntry)
{
  StgStreamImpl* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(StgStreamImpl));

  if (newStream)
  {
    /*
     * Set-up the virtual function table and reference count.
     */
    newStream->IStream_iface.lpVtbl = &StgStreamVtbl;
    newStream->ref       = 0;

    newStream->parentStorage = parentStorage;

    /*
     * We want to nail-down the reference to the storage in case the
     * stream out-lives the storage in the client application.
     *
     * -- IStorage_AddRef(&newStream->parentStorage->IStorage_iface);
     *
     * No, don't do this. Some apps call IStorage_Release without
     * calling IStream_Release first. If we grab a reference the
     * file is not closed, and the app fails when it tries to
     * reopen the file (Easy-PC, for example)
     */

    newStream->grfMode = grfMode;
    newStream->dirEntry = dirEntry;

    /*
     * Start the stream at the beginning.
     */
    newStream->currentPosition.HighPart = 0;
    newStream->currentPosition.LowPart = 0;

    /* add us to the storage's list of active streams */
    StorageBaseImpl_AddStream(parentStorage, newStream);
  }

  return newStream;
}
