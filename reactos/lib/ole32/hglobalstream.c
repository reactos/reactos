/*
 * HGLOBAL Stream implementation
 *
 * This file contains the implementation of the stream interface
 * for streams contained supported by an HGLOBAL pointer.
 *
 * Copyright 1999 Francis Beaudet
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"
#include "winreg.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/****************************************************************************
 * HGLOBALStreamImpl definition.
 *
 * This class imlements the IStream inteface and represents a stream
 * supported by an HGLOBAL pointer.
 */
struct HGLOBALStreamImpl
{
  ICOM_VFIELD(IStream);  /* Needs to be the first item in the stuct
			  * since we want to cast this in a IStream pointer */

  /*
   * Reference count
   */
  ULONG		     ref;

  /*
   * Support for the stream
   */
  HGLOBAL supportHandle;

  /*
   * This flag is TRUE if the HGLOBAL is destroyed when the stream
   * is finally released.
   */
  BOOL    deleteOnRelease;

  /*
   * Helper variable that contains the size of the stream
   */
  ULARGE_INTEGER     streamSize;

  /*
   * This is the current position of the cursor in the stream
   */
  ULARGE_INTEGER     currentPosition;
};

typedef struct HGLOBALStreamImpl HGLOBALStreamImpl;

/*
 * Method definition for the StgStreamImpl class.
 */
HGLOBALStreamImpl* HGLOBALStreamImpl_Construct(
		HGLOBAL  hGlobal,
		BOOL     fDeleteOnRelease);

void HGLOBALStreamImpl_Destroy(
                HGLOBALStreamImpl* This);

void HGLOBALStreamImpl_OpenBlockChain(
                HGLOBALStreamImpl* This);

HRESULT WINAPI HGLOBALStreamImpl_QueryInterface(
		IStream*      iface,
		REFIID         riid,		/* [in] */
		void**         ppvObject);  /* [iid_is][out] */

ULONG WINAPI HGLOBALStreamImpl_AddRef(
		IStream*      iface);

ULONG WINAPI HGLOBALStreamImpl_Release(
		IStream*      iface);

HRESULT WINAPI HGLOBALStreamImpl_Read(
	        IStream*      iface,
		void*          pv,        /* [length_is][size_is][out] */
		ULONG          cb,        /* [in] */
		ULONG*         pcbRead);  /* [out] */

HRESULT WINAPI HGLOBALStreamImpl_Write(
		IStream*      iface,
		const void*    pv,          /* [size_is][in] */
		ULONG          cb,          /* [in] */
		ULONG*         pcbWritten); /* [out] */

HRESULT WINAPI HGLOBALStreamImpl_Seek(
		IStream*      iface,
		LARGE_INTEGER   dlibMove,         /* [in] */
		DWORD           dwOrigin,         /* [in] */
		ULARGE_INTEGER* plibNewPosition); /* [out] */

HRESULT WINAPI HGLOBALStreamImpl_SetSize(
	        IStream*      iface,
		ULARGE_INTEGER  libNewSize);  /* [in] */

HRESULT WINAPI HGLOBALStreamImpl_CopyTo(
		IStream*      iface,
		IStream*      pstm,         /* [unique][in] */
		ULARGE_INTEGER  cb,           /* [in] */
		ULARGE_INTEGER* pcbRead,      /* [out] */
		ULARGE_INTEGER* pcbWritten);  /* [out] */

HRESULT WINAPI HGLOBALStreamImpl_Commit(
	    	IStream*      iface,
		DWORD           grfCommitFlags); /* [in] */

HRESULT WINAPI HGLOBALStreamImpl_Revert(
		IStream*  iface);

HRESULT WINAPI HGLOBALStreamImpl_LockRegion(
		IStream*     iface,
		ULARGE_INTEGER libOffset,   /* [in] */
		ULARGE_INTEGER cb,          /* [in] */
		DWORD          dwLockType); /* [in] */

HRESULT WINAPI HGLOBALStreamImpl_UnlockRegion(
		IStream*     iface,
		ULARGE_INTEGER libOffset,   /* [in] */
	        ULARGE_INTEGER cb,          /* [in] */
		DWORD          dwLockType); /* [in] */

HRESULT WINAPI HGLOBALStreamImpl_Stat(
		IStream*     iface,
	        STATSTG*       pstatstg,     /* [out] */
	        DWORD          grfStatFlag); /* [in] */

HRESULT WINAPI HGLOBALStreamImpl_Clone(
		IStream*     iface,
		IStream**    ppstm);       /* [out] */


/*
 * Virtual function table for the HGLOBALStreamImpl class.
 */
static ICOM_VTABLE(IStream) HGLOBALStreamImpl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
  HGLOBALStreamImpl* newStream;

  newStream = HGLOBALStreamImpl_Construct(hGlobal,
					  fDeleteOnRelease);

  if (newStream!=NULL)
  {
    return IUnknown_QueryInterface((IUnknown*)newStream,
				   &IID_IStream,
				   (void**)ppstm);
  }

  return E_OUTOFMEMORY;
}

/***********************************************************************
 *           GetHGlobalFromStream     [OLE32.@]
 */
HRESULT WINAPI GetHGlobalFromStream(IStream* pstm, HGLOBAL* phglobal)
{
  HGLOBALStreamImpl* pStream;

  if (pstm == NULL)
    return E_INVALIDARG;

  pStream = (HGLOBALStreamImpl*) pstm;

  /*
   * Verify that the stream object was created with CreateStreamOnHGlobal.
   */
  if (pStream->lpVtbl == &HGLOBALStreamImpl_Vtbl)
    *phglobal = pStream->supportHandle;
  else
  {
    *phglobal = 0;
    return E_INVALIDARG;
  }

  return S_OK;
}

/******************************************************************************
** HGLOBALStreamImpl implementation
*/

/***
 * This is the constructor for the HGLOBALStreamImpl class.
 *
 * Params:
 *    hGlobal          - Handle that will support the stream. can be NULL.
 *    fDeleteOnRelease - Flag set to TRUE if the HGLOBAL will be released
 *                       when the IStream object is destroyed.
 */
HGLOBALStreamImpl* HGLOBALStreamImpl_Construct(
		HGLOBAL  hGlobal,
		BOOL     fDeleteOnRelease)
{
  HGLOBALStreamImpl* newStream;

  newStream = HeapAlloc(GetProcessHeap(), 0, sizeof(HGLOBALStreamImpl));

  if (newStream!=0)
  {
    /*
     * Set-up the virtual function table and reference count.
     */
    newStream->lpVtbl = &HGLOBALStreamImpl_Vtbl;
    newStream->ref    = 0;

    /*
     * Initialize the support.
     */
    newStream->supportHandle = hGlobal;
    newStream->deleteOnRelease = fDeleteOnRelease;

    /*
     * This method will allocate a handle if one is not supplied.
     */
    if (!newStream->supportHandle)
    {
      newStream->supportHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD |
					     GMEM_SHARE, 0);
    }

    /*
     * Start the stream at the beginning.
     */
    newStream->currentPosition.u.HighPart = 0;
    newStream->currentPosition.u.LowPart = 0;

    /*
     * Initialize the size of the stream to the size of the handle.
     */
    newStream->streamSize.u.HighPart = 0;
    newStream->streamSize.u.LowPart  = GlobalSize(newStream->supportHandle);
  }

  return newStream;
}

/***
 * This is the destructor of the HGLOBALStreamImpl class.
 *
 * This method will clean-up all the resources used-up by the given HGLOBALStreamImpl
 * class. The pointer passed-in to this function will be freed and will not
 * be valid anymore.
 */
void HGLOBALStreamImpl_Destroy(HGLOBALStreamImpl* This)
{
  TRACE("(%p)\n", This);

  /*
   * Release the HGlobal if the constructor asked for that.
   */
  if (This->deleteOnRelease)
  {
    GlobalFree(This->supportHandle);
    This->supportHandle=0;
  }

  /*
   * Finally, free the memory used-up by the class.
   */
  HeapFree(GetProcessHeap(), 0, This);
}

/***
 * This implements the IUnknown method QueryInterface for this
 * class
 */
HRESULT WINAPI HGLOBALStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */
		  void**         ppvObject)   /* [iid_is][out] */
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  /*
   * Perform a sanity check on the parameters.
   */
  if (ppvObject==0)
    return E_INVALIDARG;

  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;

  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = (IStream*)This;
  }
  else if (memcmp(&IID_IStream, riid, sizeof(IID_IStream)) == 0)
  {
    *ppvObject = (IStream*)This;
  }

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
    return E_NOINTERFACE;

  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  HGLOBALStreamImpl_AddRef(iface);

  return S_OK;
}

/***
 * This implements the IUnknown method AddRef for this
 * class
 */
ULONG WINAPI HGLOBALStreamImpl_AddRef(
		IStream* iface)
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  This->ref++;

  return This->ref;
}

/***
 * This implements the IUnknown method Release for this
 * class
 */
ULONG WINAPI HGLOBALStreamImpl_Release(
		IStream* iface)
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  ULONG newRef;

  This->ref--;

  newRef = This->ref;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (newRef==0)
  {
    HGLOBALStreamImpl_Destroy(This);
  }

  return newRef;
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
HRESULT WINAPI HGLOBALStreamImpl_Read(
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */
		  ULONG*         pcbRead)   /* [out] */
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  void* supportBuffer;
  ULONG bytesReadBuffer;
  ULONG bytesToReadFromBuffer;

  TRACE("(%p, %p, %ld, %p)\n", iface,
	pv, cb, pcbRead);

  /*
   * If the caller is not interested in the nubmer of bytes read,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbRead==0)
    pcbRead = &bytesReadBuffer;

  /*
   * Using the known size of the stream, calculate the number of bytes
   * to read from the block chain
   */
  bytesToReadFromBuffer = min( This->streamSize.u.LowPart - This->currentPosition.u.LowPart, cb);

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->supportHandle);

  memcpy(pv, (char *) supportBuffer+This->currentPosition.u.LowPart, bytesToReadFromBuffer);

  /*
   * Move the current position to the new position
   */
  This->currentPosition.u.LowPart+=bytesToReadFromBuffer;

  /*
   * Return the number of bytes read.
   */
  *pcbRead = bytesToReadFromBuffer;

  /*
   * Cleanup
   */
  GlobalUnlock(This->supportHandle);

  /*
   * The function returns S_OK if the buffer was filled completely
   * it returns S_FALSE if the end of the stream is reached before the
   * buffer is filled
   */
  if(*pcbRead == cb)
    return S_OK;

  return S_FALSE;
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
HRESULT WINAPI HGLOBALStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */
		  ULONG          cb,          /* [in] */
		  ULONG*         pcbWritten)  /* [out] */
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  void*          supportBuffer;
  ULARGE_INTEGER newSize;
  ULONG          bytesWritten = 0;

  TRACE("(%p, %p, %ld, %p)\n", iface,
	pv, cb, pcbWritten);

  /*
   * If the caller is not interested in the number of bytes written,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;

  if (cb == 0)
  {
    return S_OK;
  }
  else
  {
    newSize.u.HighPart = 0;
    newSize.u.LowPart = This->currentPosition.u.LowPart + cb;
  }

  /*
   * Verify if we need to grow the stream
   */
  if (newSize.u.LowPart > This->streamSize.u.LowPart)
  {
    /* grow stream */
   IStream_SetSize(iface, newSize);
  }

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->supportHandle);

  memcpy((char *) supportBuffer+This->currentPosition.u.LowPart, pv, cb);

  /*
   * Move the current position to the new position
   */
  This->currentPosition.u.LowPart+=cb;

  /*
   * Return the number of bytes read.
   */
  *pcbWritten = cb;

  /*
   * Cleanup
   */
  GlobalUnlock(This->supportHandle);

  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will move the current stream pointer according to the parameters
 * given.
 *
 * See the documentation of IStream for more info.
 */
HRESULT WINAPI HGLOBALStreamImpl_Seek(
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */
		  DWORD           dwOrigin,         /* [in] */
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  ULARGE_INTEGER newPosition;

  TRACE("(%p, %lx%08lx, %ld, %p)\n", iface, dlibMove.u.HighPart,
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
      newPosition = This->currentPosition;
      break;
    case STREAM_SEEK_END:
      newPosition = This->streamSize;
      break;
    default:
      return STG_E_INVALIDFUNCTION;
  }

  /*
   * Move the actual file pointer
   * If the file pointer ends-up after the end of the stream, the next Write operation will
   * make the file larger. This is how it is documented.
   */
  newPosition.QuadPart = RtlLargeIntegerAdd(newPosition.QuadPart, dlibMove.QuadPart);
  if (newPosition.QuadPart < 0) return STG_E_INVALIDFUNCTION;

  if (plibNewPosition) *plibNewPosition = newPosition;
  This->currentPosition = newPosition;

  return S_OK;
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
HRESULT WINAPI HGLOBALStreamImpl_SetSize(
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;
  HGLOBAL supportHandle;

  TRACE("(%p, %ld)\n", iface, libNewSize.u.LowPart);

  /*
   * As documented.
   */
  if (libNewSize.u.HighPart != 0)
    return STG_E_INVALIDFUNCTION;

  if (This->streamSize.u.LowPart == libNewSize.u.LowPart)
    return S_OK;

  /*
   * Re allocate the HGlobal to fit the new size of the stream.
   */
  supportHandle = GlobalReAlloc(This->supportHandle, libNewSize.u.LowPart, 0);

  if (supportHandle == 0)
    return STG_E_MEDIUMFULL;

  This->supportHandle = supportHandle;
  This->streamSize.u.LowPart = libNewSize.u.LowPart;

  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will copy the 'cb' Bytes to 'pstm' IStream.
 *
 * See the documentation of IStream for more info.
 */
HRESULT WINAPI HGLOBALStreamImpl_CopyTo(
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

  TRACE("(%p, %p, %ld, %p, %p)\n", iface, pstm,
	cb.u.LowPart, pcbRead, pcbWritten);

  /*
   * Sanity check
   */
  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;

  totalBytesRead.u.LowPart = totalBytesRead.u.HighPart = 0;
  totalBytesWritten.u.LowPart = totalBytesWritten.u.HighPart = 0;

  /*
   * use stack to store data temporarly
   * there is surely more performant way of doing it, for now this basic
   * implementation will do the job
   */
  while ( cb.u.LowPart > 0 )
  {
    if ( cb.u.LowPart >= 128 )
      copySize = 128;
    else
      copySize = cb.u.LowPart;

    IStream_Read(iface, tmpBuffer, copySize, &bytesRead);

    totalBytesRead.u.LowPart += bytesRead;

    IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);

    totalBytesWritten.u.LowPart += bytesWritten;

    /*
     * Check that read & write operations were succesfull
     */
    if (bytesRead != bytesWritten)
    {
      hr = STG_E_MEDIUMFULL;
      break;
    }

    if (bytesRead!=copySize)
      cb.u.LowPart = 0;
    else
      cb.u.LowPart -= bytesRead;
  }

  /*
   * Update number of bytes read and written
   */
  if (pcbRead)
  {
    pcbRead->u.LowPart = totalBytesRead.u.LowPart;
    pcbRead->u.HighPart = totalBytesRead.u.HighPart;
  }

  if (pcbWritten)
  {
    pcbWritten->u.LowPart = totalBytesWritten.u.LowPart;
    pcbWritten->u.HighPart = totalBytesWritten.u.HighPart;
  }
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
HRESULT WINAPI HGLOBALStreamImpl_Commit(
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
HRESULT WINAPI HGLOBALStreamImpl_Revert(
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
HRESULT WINAPI HGLOBALStreamImpl_LockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

/*
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
HRESULT WINAPI HGLOBALStreamImpl_UnlockRegion(
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
HRESULT WINAPI HGLOBALStreamImpl_Stat(
		  IStream*     iface,
		  STATSTG*     pstatstg,     /* [out] */
		  DWORD        grfStatFlag)  /* [in] */
{
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;

  memset(pstatstg, 0, sizeof(STATSTG));

  pstatstg->pwcsName = NULL;
  pstatstg->type     = STGTY_STREAM;
  pstatstg->cbSize   = This->streamSize;

  return S_OK;
}

HRESULT WINAPI HGLOBALStreamImpl_Clone(
		  IStream*     iface,
		  IStream**    ppstm) /* [out] */
{
  ULARGE_INTEGER dummy;
  LARGE_INTEGER offset;
  HRESULT hr;
  HGLOBALStreamImpl* const This=(HGLOBALStreamImpl*)iface;
  TRACE(" Cloning %p (deleteOnRelease=%d seek position=%ld)\n",iface,This->deleteOnRelease,(long)This->currentPosition.QuadPart);
  hr=CreateStreamOnHGlobal(This->supportHandle, FALSE, ppstm);
  if(FAILED(hr))
    return hr;
  offset.QuadPart=(LONGLONG)This->currentPosition.QuadPart;
  HGLOBALStreamImpl_Seek(*ppstm,offset,STREAM_SEEK_SET,&dummy);
  return S_OK;
}
