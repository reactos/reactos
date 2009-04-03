/******************************************************************************
 *
 * Global memory implementation of ILockBytes.
 *
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

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 * HGLOBALLockBytesImpl definition.
 *
 * This class implements the ILockBytes interface and represents a byte array
 * object supported by an HGLOBAL pointer.
 */
struct HGLOBALLockBytesImpl
{
  /*
   * Needs to be the first item in the struct
   * since we want to cast this in an ILockBytes pointer
   */
  const ILockBytesVtbl *lpVtbl;

  /*
   * Reference count
   */
  LONG        ref;

  /*
   * Support for the LockBytes object
   */
  HGLOBAL supportHandle;

  /*
   * This flag is TRUE if the HGLOBAL is destroyed when the object
   * is finally released.
   */
  BOOL    deleteOnRelease;

  /*
   * Helper variable that contains the size of the byte array
   */
  ULARGE_INTEGER     byteArraySize;
};

typedef struct HGLOBALLockBytesImpl HGLOBALLockBytesImpl;

/*
 * Method definition for the HGLOBALLockBytesImpl class.
 */
static HGLOBALLockBytesImpl* HGLOBALLockBytesImpl_Construct(
    HGLOBAL  hGlobal,
    BOOL     fDeleteOnRelease);

static void HGLOBALLockBytesImpl_Destroy(HGLOBALLockBytesImpl* This);

static HRESULT WINAPI HGLOBALLockBytesImpl_SetSize( ILockBytes* iface, ULARGE_INTEGER libNewSize );

static const ILockBytesVtbl HGLOBALLockBytesImpl_Vtbl;

/******************************************************************************
 *           CreateILockBytesOnHGlobal     [OLE32.@]
 *
 * Create a byte array object which is intended to be the compound file foundation.
 * This object supports a COM implementation of the ILockBytes interface.
 *
 * PARAMS
 *  hGlobal           [ I] Global memory handle
 *  fDeleteOnRelease  [ I] Whether the handle should be freed when the object is released. 
 *  ppLkbyt           [ O] Address of ILockBytes pointer that receives
 *                         the interface pointer to the new byte array object.
 *
 * RETURNS
 *  Success: S_OK
 *
 * NOTES
 *  The supplied ILockBytes pointer can be used by the StgCreateDocfileOnILockBytes
 *  function to build a compound file on top of this byte array object.
 *  The ILockBytes interface instance calls the GlobalReAlloc function to grow
 *  the memory block as required.
 */
HRESULT WINAPI CreateILockBytesOnHGlobal(HGLOBAL      hGlobal,
                                         BOOL         fDeleteOnRelease,
                                         LPLOCKBYTES* ppLkbyt)
{
  HGLOBALLockBytesImpl* newLockBytes;

  newLockBytes = HGLOBALLockBytesImpl_Construct(hGlobal, fDeleteOnRelease);

  if (newLockBytes != NULL)
  {
    return IUnknown_QueryInterface((IUnknown*)newLockBytes,
                                   &IID_ILockBytes,
                                   (void**)ppLkbyt);
  }

  return E_OUTOFMEMORY;
}

/******************************************************************************
 *           GetHGlobalFromILockBytes     [OLE32.@]
 *
 * Retrieve a global memory handle to a byte array object created
 * using the CreateILockBytesOnHGlobal function.
 *
 * PARAMS
 *  plkbyt   [ I]  Pointer to the ILockBytes interface on byte array object
 *  phglobal [ O]  Address to store a global memory handle
 * RETURNS
 *  S_OK          if *phglobal has a correct value
 *  E_INVALIDARG  if any parameters are invalid
 *  
 */
HRESULT WINAPI GetHGlobalFromILockBytes(ILockBytes* plkbyt, HGLOBAL* phglobal)
{
  HGLOBALLockBytesImpl* const pMemLockBytes = (HGLOBALLockBytesImpl*)plkbyt;
  STATSTG stbuf;
  HRESULT hres;
  ULARGE_INTEGER start;
  ULONG xread;

  *phglobal = 0;
  if (pMemLockBytes->lpVtbl == &HGLOBALLockBytesImpl_Vtbl) {
    *phglobal = pMemLockBytes->supportHandle;
    if (*phglobal == 0)
      return E_INVALIDARG;
    return S_OK;
  }
  /* It is not our lockbytes implementation, so use a more generic way */
  hres = ILockBytes_Stat(plkbyt,&stbuf,0);
  if (hres != S_OK) {
     ERR("Cannot ILockBytes_Stat, %x\n",hres);
     return hres;
  }
  FIXME("cbSize is %d\n",stbuf.cbSize.u.LowPart);
  *phglobal = GlobalAlloc( GMEM_MOVEABLE|GMEM_SHARE, stbuf.cbSize.u.LowPart);
  if (!*phglobal)
    return E_INVALIDARG;
  memset(&start,0,sizeof(start));
  hres = ILockBytes_ReadAt(plkbyt, start, GlobalLock(*phglobal), stbuf.cbSize.u.LowPart, &xread);
  GlobalUnlock(*phglobal);
  if (hres != S_OK) {
    FIXME("%p->ReadAt failed with %x\n",plkbyt,hres);
    return hres;
  }
  if (stbuf.cbSize.u.LowPart != xread) {
    FIXME("Read size is not requested size %d vs %d?\n",stbuf.cbSize.u.LowPart, xread);
  }
  return S_OK;
}

/******************************************************************************
 *
 * HGLOBALLockBytesImpl implementation
 *
 */

/******************************************************************************
 * This is the constructor for the HGLOBALLockBytesImpl class.
 *
 * PARAMS
 *    hGlobal          [ I] Handle that will support the stream. can be NULL.
 *    fDeleteOnRelease [ I] Flag set to TRUE if the HGLOBAL will be released
 *                          when the IStream object is destroyed.
 */
static HGLOBALLockBytesImpl* HGLOBALLockBytesImpl_Construct(HGLOBAL hGlobal,
                                                            BOOL    fDeleteOnRelease)
{
  HGLOBALLockBytesImpl* newLockBytes;
  newLockBytes = HeapAlloc(GetProcessHeap(), 0, sizeof(HGLOBALLockBytesImpl));

  if (newLockBytes!=0)
  {
    /*
     * Set up the virtual function table and reference count.
     */
    newLockBytes->lpVtbl = &HGLOBALLockBytesImpl_Vtbl;
    newLockBytes->ref    = 0;

    /*
     * Initialize the support.
     */
    newLockBytes->supportHandle = hGlobal;
    newLockBytes->deleteOnRelease = fDeleteOnRelease;

    /*
     * This method will allocate a handle if one is not supplied.
     */
    if (newLockBytes->supportHandle == 0)
    {
      newLockBytes->supportHandle = GlobalAlloc(GMEM_MOVEABLE |
                                                GMEM_NODISCARD,
                                                0);
    }

    /*
     * Initialize the size of the array to the size of the handle.
     */
    newLockBytes->byteArraySize.u.HighPart = 0;
    newLockBytes->byteArraySize.u.LowPart  = GlobalSize(
                                              newLockBytes->supportHandle);
  }

  return newLockBytes;
}

/******************************************************************************
 * This is the destructor of the HGLOBALStreamImpl class.
 *
 * This method will clean-up all the resources used-up by the given
 * HGLOBALLockBytesImpl class. The pointer passed-in to this function will be
 * freed and will not be valid anymore.
 */
static void HGLOBALLockBytesImpl_Destroy(HGLOBALLockBytesImpl* This)
{
  /*
   * Release the HGlobal if the constructor asked for that.
   */
  if (This->deleteOnRelease)
  {
    GlobalFree(This->supportHandle);
    This->supportHandle = 0;
  }

  /*
   * Finally, free the memory used-up by the class.
   */
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 * This implements the IUnknown method QueryInterface for this
 * class
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_QueryInterface(
      ILockBytes*  iface,
      REFIID       riid,        /* [in] */
      void**       ppvObject)   /* [iid_is][out] */
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

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
  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_ILockBytes))
  {
    *ppvObject = This;
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
  IUnknown_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 * This implements the IUnknown method AddRef for this
 * class
 */
static ULONG WINAPI HGLOBALLockBytesImpl_AddRef(ILockBytes* iface)
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;
  return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 * This implements the IUnknown method Release for this
 * class
 */
static ULONG WINAPI HGLOBALLockBytesImpl_Release(ILockBytes* iface)
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;
  ULONG ref;

  ref = InterlockedDecrement(&This->ref);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (ref==0)
  {
    HGLOBALLockBytesImpl_Destroy(This);
  }

  return ref;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It reads a block of information from the byte array at the specified
 * offset.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_ReadAt(
      ILockBytes*    iface,
      ULARGE_INTEGER ulOffset,  /* [in] */
      void*          pv,        /* [length_is][size_is][out] */
      ULONG          cb,        /* [in] */
      ULONG*         pcbRead)   /* [out] */
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

  void* supportBuffer;
  ULONG bytesReadBuffer = 0;
  ULONG bytesToReadFromBuffer;

  /*
   * If the caller is not interested in the number of bytes read,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbRead == 0)
    pcbRead = &bytesReadBuffer;

  /*
   * Make sure the offset is valid.
   */
  if (ulOffset.u.LowPart > This->byteArraySize.u.LowPart)
    return E_FAIL;

  /*
   * Using the known size of the array, calculate the number of bytes
   * to read.
   */
  bytesToReadFromBuffer = min(This->byteArraySize.u.LowPart -
                              ulOffset.u.LowPart, cb);

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->supportHandle);

  memcpy(pv,
         (char *) supportBuffer + ulOffset.u.LowPart,
         bytesToReadFromBuffer);

  /*
   * Return the number of bytes read.
   */
  *pcbRead = bytesToReadFromBuffer;

  /*
   * Cleanup
   */
  GlobalUnlock(This->supportHandle);

  /*
   * The function returns S_OK if the specified number of bytes were read
   * or the end of the array was reached.
   * It returns STG_E_READFAULT if the number of bytes to read does not equal
   * the number of bytes actually read.
   */
  if(*pcbRead == cb)
    return S_OK;

  return STG_E_READFAULT;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It writes the specified bytes at the specified offset.
 * position. If the array is too small, it will be resized.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_WriteAt(
      ILockBytes*    iface,
      ULARGE_INTEGER ulOffset,    /* [in] */
      const void*    pv,          /* [size_is][in] */
      ULONG          cb,          /* [in] */
      ULONG*         pcbWritten)  /* [out] */
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

  void*          supportBuffer;
  ULARGE_INTEGER newSize;
  ULONG          bytesWritten = 0;

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
    newSize.u.LowPart = ulOffset.u.LowPart + cb;
  }

  /*
   * Verify if we need to grow the stream
   */
  if (newSize.u.LowPart > This->byteArraySize.u.LowPart)
  {
    /* grow stream */
    if (HGLOBALLockBytesImpl_SetSize(iface, newSize) == STG_E_MEDIUMFULL)
      return STG_E_MEDIUMFULL;
  }

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->supportHandle);

  memcpy((char *) supportBuffer + ulOffset.u.LowPart, pv, cb);

  /*
   * Return the number of bytes written.
   */
  *pcbWritten = cb;

  /*
   * Cleanup
   */
  GlobalUnlock(This->supportHandle);

  return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_Flush(ILockBytes* iface)
{
  return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It will change the size of the byte array.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_SetSize(
      ILockBytes*     iface,
      ULARGE_INTEGER  libNewSize)   /* [in] */
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;
  HGLOBAL supportHandle;

  /*
   * As documented.
   */
  if (libNewSize.u.HighPart != 0)
    return STG_E_INVALIDFUNCTION;

  if (This->byteArraySize.u.LowPart == libNewSize.u.LowPart)
    return S_OK;

  /*
   * Re allocate the HGlobal to fit the new size of the stream.
   */
  supportHandle = GlobalReAlloc(This->supportHandle, libNewSize.u.LowPart, 0);

  if (supportHandle == 0)
    return STG_E_MEDIUMFULL;

  This->supportHandle = supportHandle;
  This->byteArraySize.u.LowPart = libNewSize.u.LowPart;

  return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * The global memory implementation of ILockBytes does not support locking.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_LockRegion(
      ILockBytes*    iface,
      ULARGE_INTEGER libOffset,   /* [in] */
      ULARGE_INTEGER cb,          /* [in] */
      DWORD          dwLockType)  /* [in] */
{
  return STG_E_INVALIDFUNCTION;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * The global memory implementation of ILockBytes does not support locking.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_UnlockRegion(
      ILockBytes*    iface,
      ULARGE_INTEGER libOffset,   /* [in] */
      ULARGE_INTEGER cb,          /* [in] */
      DWORD          dwLockType)  /* [in] */
{
  return STG_E_INVALIDFUNCTION;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * This method returns information about the current
 * byte array object.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_Stat(
      ILockBytes*  iface,
      STATSTG*     pstatstg,     /* [out] */
      DWORD        grfStatFlag)  /* [in] */
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

  memset(pstatstg, 0, sizeof(STATSTG));

  pstatstg->pwcsName = NULL;
  pstatstg->type     = STGTY_LOCKBYTES;
  pstatstg->cbSize   = This->byteArraySize;

  return S_OK;
}

/*
 * Virtual function table for the HGLOBALLockBytesImpl class.
 */
static const ILockBytesVtbl HGLOBALLockBytesImpl_Vtbl =
{
    HGLOBALLockBytesImpl_QueryInterface,
    HGLOBALLockBytesImpl_AddRef,
    HGLOBALLockBytesImpl_Release,
    HGLOBALLockBytesImpl_ReadAt,
    HGLOBALLockBytesImpl_WriteAt,
    HGLOBALLockBytesImpl_Flush,
    HGLOBALLockBytesImpl_SetSize,
    HGLOBALLockBytesImpl_LockRegion,
    HGLOBALLockBytesImpl_UnlockRegion,
    HGLOBALLockBytesImpl_Stat,
};
