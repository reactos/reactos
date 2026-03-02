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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
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
  ILockBytes ILockBytes_iface;
  LONG ref;

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

static inline HGLOBALLockBytesImpl *impl_from_ILockBytes( ILockBytes *iface )
{
    return CONTAINING_RECORD(iface, HGLOBALLockBytesImpl, ILockBytes_iface);
}

static const ILockBytesVtbl HGLOBALLockBytesImpl_Vtbl;

/******************************************************************************
 *           CreateILockBytesOnHGlobal     [OLE32.@]
 *
 * Create a byte array object which is intended to be the compound file foundation.
 * This object supports a COM implementation of the ILockBytes interface.
 *
 * PARAMS
 *  global            [ I] Global memory handle
 *  delete_on_release [ I] Whether the handle should be freed when the object is released.
 *  ret               [ O] Address of ILockBytes pointer that receives
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
HRESULT WINAPI CreateILockBytesOnHGlobal(HGLOBAL global, BOOL delete_on_release, ILockBytes **ret)
{
  HGLOBALLockBytesImpl* lockbytes;

  lockbytes = HeapAlloc(GetProcessHeap(), 0, sizeof(HGLOBALLockBytesImpl));
  if (!lockbytes) return E_OUTOFMEMORY;

  lockbytes->ILockBytes_iface.lpVtbl = &HGLOBALLockBytesImpl_Vtbl;
  lockbytes->ref = 1;

  /*
   * Initialize the support.
   */
  lockbytes->supportHandle = global;
  lockbytes->deleteOnRelease = delete_on_release;

  /*
   * This method will allocate a handle if one is not supplied.
   */
  if (lockbytes->supportHandle == 0)
    lockbytes->supportHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, 0);

  /*
   * Initialize the size of the array to the size of the handle.
   */
  lockbytes->byteArraySize.HighPart = 0;
  lockbytes->byteArraySize.LowPart  = GlobalSize(lockbytes->supportHandle);

  *ret = &lockbytes->ILockBytes_iface;

  return S_OK;
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
HRESULT WINAPI GetHGlobalFromILockBytes(ILockBytes* iface, HGLOBAL* phglobal)
{
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);
  STATSTG stbuf;
  HRESULT hres;
  ULARGE_INTEGER start;
  ULONG xread;

  *phglobal = 0;
  if (This->ILockBytes_iface.lpVtbl == &HGLOBALLockBytesImpl_Vtbl) {
    *phglobal = This->supportHandle;
    if (*phglobal == 0)
      return E_INVALIDARG;
    return S_OK;
  }
  /* It is not our lockbytes implementation, so use a more generic way */
  hres = ILockBytes_Stat(iface,&stbuf,STATFLAG_NONAME);
  if (hres != S_OK) {
     ERR("Cannot ILockBytes_Stat, %lx\n",hres);
     return hres;
  }
  TRACE("cbSize is %s\n", wine_dbgstr_longlong(stbuf.cbSize.QuadPart));
  *phglobal = GlobalAlloc( GMEM_MOVEABLE|GMEM_SHARE, stbuf.cbSize.LowPart);
  if (!*phglobal)
    return E_INVALIDARG;
  memset(&start,0,sizeof(start));
  hres = ILockBytes_ReadAt(iface, start, GlobalLock(*phglobal), stbuf.cbSize.LowPart, &xread);
  GlobalUnlock(*phglobal);
  if (hres != S_OK) {
    FIXME("%p->ReadAt failed with %lx\n",iface,hres);
    return hres;
  }
  if (stbuf.cbSize.LowPart != xread) {
    FIXME("Read size is not requested size %ld vs %ld?\n",stbuf.cbSize.LowPart, xread);
  }
  return S_OK;
}

/******************************************************************************
 *
 * HGLOBALLockBytesImpl implementation
 *
 */

/******************************************************************************
 * This implements the IUnknown method QueryInterface for this
 * class
 */
static HRESULT WINAPI HGLOBALLockBytesImpl_QueryInterface(
      ILockBytes*  iface,
      REFIID       riid,        /* [in] */
      void**       ppvObject)   /* [iid_is][out] */
{
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);

  if (ppvObject==0)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(riid, &IID_IUnknown) ||
      IsEqualIID(riid, &IID_ILockBytes))
  {
    *ppvObject = &This->ILockBytes_iface;
  }
  else
    return E_NOINTERFACE;

  ILockBytes_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 * This implements the IUnknown method AddRef for this
 * class
 */
static ULONG WINAPI HGLOBALLockBytesImpl_AddRef(ILockBytes* iface)
{
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);
  return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 * This implements the IUnknown method Release for this
 * class
 */
static ULONG WINAPI HGLOBALLockBytesImpl_Release(ILockBytes* iface)
{
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);
  ULONG ref;

  ref = InterlockedDecrement(&This->ref);
  if (!ref)
  {
    if (This->deleteOnRelease)
    {
      GlobalFree(This->supportHandle);
      This->supportHandle = 0;
    }
    HeapFree(GetProcessHeap(), 0, This);
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
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);

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
  if (ulOffset.LowPart > This->byteArraySize.LowPart)
    return E_FAIL;

  /*
   * Using the known size of the array, calculate the number of bytes
   * to read.
   */
  bytesToReadFromBuffer = min(This->byteArraySize.LowPart - ulOffset.LowPart, cb);

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->supportHandle);

  memcpy(pv,
         (char *) supportBuffer + ulOffset.LowPart,
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
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);

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
    newSize.HighPart = 0;
    newSize.LowPart = ulOffset.LowPart + cb;
  }

  /*
   * Verify if we need to grow the stream
   */
  if (newSize.LowPart > This->byteArraySize.LowPart)
  {
    /* grow stream */
    if (ILockBytes_SetSize(iface, newSize) == STG_E_MEDIUMFULL)
      return STG_E_MEDIUMFULL;
  }

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->supportHandle);

  memcpy((char *) supportBuffer + ulOffset.LowPart, pv, cb);

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
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);
  HGLOBAL supportHandle;

  /*
   * As documented.
   */
  if (libNewSize.HighPart != 0)
    return STG_E_INVALIDFUNCTION;

  if (This->byteArraySize.LowPart == libNewSize.LowPart)
    return S_OK;

  /*
   * Re allocate the HGlobal to fit the new size of the stream.
   */
  supportHandle = GlobalReAlloc(This->supportHandle, libNewSize.LowPart, GMEM_MOVEABLE);

  if (supportHandle == 0)
    return STG_E_MEDIUMFULL;

  This->supportHandle = supportHandle;
  This->byteArraySize.LowPart = libNewSize.LowPart;

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
  HGLOBALLockBytesImpl* This = impl_from_ILockBytes(iface);

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
