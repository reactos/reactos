/*
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"

#include "ifs.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/******************************************************************************
 * HGLOBALLockBytesImpl16 definition.
 *
 * This class implements the ILockBytes interface and represents a byte array
 * object supported by an HGLOBAL pointer.
 */
struct HGLOBALLockBytesImpl16
{
  /*
   * Needs to be the first item in the struct
   * since we want to cast this in an ILockBytes pointer
   */
  const ILockBytes16Vtbl *lpVtbl;
  LONG        ref;

  /*
   * Support for the LockBytes object
   */
  HGLOBAL16 supportHandle;

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

typedef struct HGLOBALLockBytesImpl16 HGLOBALLockBytesImpl16;

/******************************************************************************
 *
 * HGLOBALLockBytesImpl16 implementation
 *
 */

/******************************************************************************
 * This is the constructor for the HGLOBALLockBytesImpl16 class.
 *
 * Params:
 *    hGlobal          - Handle that will support the stream. can be NULL.
 *    fDeleteOnRelease - Flag set to TRUE if the HGLOBAL16 will be released
 *                       when the IStream object is destroyed.
 */
static HGLOBALLockBytesImpl16*
HGLOBALLockBytesImpl16_Construct(HGLOBAL16 hGlobal,
				 BOOL16 fDeleteOnRelease)
{
  HGLOBALLockBytesImpl16* newLockBytes;

  static ILockBytes16Vtbl vt16;
  static SEGPTR msegvt16;
  HMODULE16 hcomp = GetModuleHandle16("OLE2");


  TRACE("(%x,%d)\n",hGlobal,fDeleteOnRelease);
  newLockBytes = HeapAlloc(GetProcessHeap(), 0, sizeof(HGLOBALLockBytesImpl16));
  if (newLockBytes == NULL)
    return NULL;

  /*
   * Set up the virtual function table and reference count.
   */
  if (!msegvt16)
  {
#define VTENT(x) vt16.x = (void*)GetProcAddress16(hcomp,"HGLOBALLockBytesImpl16_"#x);assert(vt16.x)
      VTENT(QueryInterface);
      VTENT(AddRef);
      VTENT(Release);
      VTENT(ReadAt);
      VTENT(WriteAt);
      VTENT(Flush);
      VTENT(SetSize);
      VTENT(LockRegion);
      VTENT(UnlockRegion);
#undef VTENT
      msegvt16 = MapLS( &vt16 );
  }
  newLockBytes->lpVtbl	= (const ILockBytes16Vtbl*)msegvt16;
  newLockBytes->ref	= 0;
  /*
   * Initialize the support.
   */
  newLockBytes->supportHandle = hGlobal;
  newLockBytes->deleteOnRelease = fDeleteOnRelease;

  /*
   * This method will allocate a handle if one is not supplied.
   */
  if (newLockBytes->supportHandle == 0)
    newLockBytes->supportHandle = GlobalAlloc16(GMEM_MOVEABLE | GMEM_NODISCARD, 0);

  /*
   * Initialize the size of the array to the size of the handle.
   */
  newLockBytes->byteArraySize.u.HighPart = 0;
  newLockBytes->byteArraySize.u.LowPart  = GlobalSize16(
					    newLockBytes->supportHandle);

  return (HGLOBALLockBytesImpl16*)MapLS(newLockBytes);
}

/******************************************************************************
 * This is the destructor of the HGLOBALStreamImpl class.
 *
 * This method will clean-up all the resources used-up by the given
 * HGLOBALLockBytesImpl16 class. The pointer passed-in to this function will be
 * freed and will not be valid anymore.
 */
static void HGLOBALLockBytesImpl16_Destroy(HGLOBALLockBytesImpl16* This)
{
  TRACE("()\n");
  /*
   * Release the HGlobal if the constructor asked for that.
   */
  if (This->deleteOnRelease)
  {
    GlobalFree16(This->supportHandle);
    This->supportHandle = 0;
  }

  /*
   * Finally, free the memory used-up by the class.
   */
  HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 * This implements the IUnknown method AddRef for this
 * class
 */
ULONG CDECL HGLOBALLockBytesImpl16_AddRef(ILockBytes16* iface)
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)iface;

  TRACE("(%p)\n",This);

  return InterlockedIncrement(&This->ref);
}


/******************************************************************************
 * This implements the IUnknown method QueryInterface for this
 * class
 */
HRESULT CDECL HGLOBALLockBytesImpl16_QueryInterface(
      ILockBytes16*  iface,	/* [in] SEGPTR */
      REFIID       riid,        /* [in] */
      void**       ppvObject)   /* [out][iid_is] (ptr to SEGPTR!) */
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)MapSL((SEGPTR)iface);

  TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppvObject);
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
  if (	!memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) ||
        !memcmp(&IID_ILockBytes, riid, sizeof(IID_ILockBytes))
  )
    *ppvObject = (void*)iface;

  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0) {
    FIXME("Unknown IID %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  HGLOBALLockBytesImpl16_AddRef((ILockBytes16*)This);

  return S_OK;
}

/******************************************************************************
 * This implements the IUnknown method Release for this
 * class
 */
ULONG CDECL HGLOBALLockBytesImpl16_Release(ILockBytes16* iface)
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)iface;
  ULONG ref;

  TRACE("(%p)\n",This);

  ref = InterlockedDecrement(&This->ref);

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (ref==0)
    HGLOBALLockBytesImpl16_Destroy(This);
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
HRESULT CDECL HGLOBALLockBytesImpl16_ReadAt(
      ILockBytes16*  iface,
      ULARGE_INTEGER ulOffset,  /* [in] */
      void*          pv,        /* [out][length_is][size_is] */
      ULONG          cb,        /* [in] */
      ULONG*         pcbRead)   /* [out] */
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)iface;

  void* supportBuffer;
  ULONG bytesReadBuffer = 0;
  ULONG bytesToReadFromBuffer;

  TRACE("(%p,%d,%p,%d,%p)\n",This,ulOffset.u.LowPart,pv,cb,pcbRead);
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
  supportBuffer = GlobalLock16(This->supportHandle);

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
  GlobalUnlock16(This->supportHandle);

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
 * It will change the size of the byte array.
 *
 * See the documentation of ILockBytes for more info.
 */
HRESULT CDECL HGLOBALLockBytesImpl16_SetSize(
      ILockBytes16*   iface,
      ULARGE_INTEGER  libNewSize)   /* [in] */
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)iface;
  HGLOBAL16 supportHandle;

  TRACE("(%p,%d)\n",This,libNewSize.u.LowPart);
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
  supportHandle = GlobalReAlloc16(This->supportHandle, libNewSize.u.LowPart, 0);

  if (supportHandle == 0)
    return STG_E_MEDIUMFULL;

  This->supportHandle = supportHandle;
  This->byteArraySize.u.LowPart = libNewSize.u.LowPart;

  return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It writes the specified bytes at the specified offset.
 * position. If the array is too small, it will be resized.
 *
 * See the documentation of ILockBytes for more info.
 */
HRESULT CDECL HGLOBALLockBytesImpl16_WriteAt(
      ILockBytes16*  iface,
      ULARGE_INTEGER ulOffset,    /* [in] */
      const void*    pv,          /* [in][size_is] */
      ULONG          cb,          /* [in] */
      ULONG*         pcbWritten)  /* [out] */
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)iface;

  void*          supportBuffer;
  ULARGE_INTEGER newSize;
  ULONG          bytesWritten = 0;

  TRACE("(%p,%d,%p,%d,%p)\n",This,ulOffset.u.LowPart,pv,cb,pcbWritten);
  /*
   * If the caller is not interested in the number of bytes written,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;

  if (cb == 0)
    return S_OK;

  newSize.u.HighPart = 0;
  newSize.u.LowPart = ulOffset.u.LowPart + cb;

  /*
   * Verify if we need to grow the stream
   */
  if (newSize.u.LowPart > This->byteArraySize.u.LowPart)
  {
    /* grow stream */
    if (HGLOBALLockBytesImpl16_SetSize(iface, newSize) == STG_E_MEDIUMFULL)
      return STG_E_MEDIUMFULL;
  }

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock16(This->supportHandle);

  memcpy((char *) supportBuffer + ulOffset.u.LowPart, pv, cb);

  /*
   * Return the number of bytes written.
   */
  *pcbWritten = cb;

  /*
   * Cleanup
   */
  GlobalUnlock16(This->supportHandle);

  return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * See the documentation of ILockBytes for more info.
 */
HRESULT CDECL HGLOBALLockBytesImpl16_Flush(ILockBytes16* iface)
{
  TRACE("(%p)\n",iface);
  return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * The global memory implementation of ILockBytes does not support locking.
 *
 * See the documentation of ILockBytes for more info.
 */
HRESULT CDECL HGLOBALLockBytesImpl16_LockRegion(
      ILockBytes16*  iface,
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
HRESULT CDECL HGLOBALLockBytesImpl16_UnlockRegion(
      ILockBytes16*  iface,
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
HRESULT CDECL HGLOBALLockBytesImpl16_Stat(
      ILockBytes16*iface,
      STATSTG16*   pstatstg,     /* [out] */
      DWORD        grfStatFlag)  /* [in] */
{
  HGLOBALLockBytesImpl16* const This=(HGLOBALLockBytesImpl16*)iface;

  memset(pstatstg, 0, sizeof(STATSTG16));

  pstatstg->pwcsName = NULL;
  pstatstg->type     = STGTY_LOCKBYTES;
  pstatstg->cbSize   = This->byteArraySize;

  return S_OK;
}

/******************************************************************************
 *           CreateILockBytesOnHGlobal     [OLE2.54]
 *
 * Creates an ILockBytes interface for a HGLOBAL handle.
 *
 * PARAMS
 * 	hGlobal			the global handle (16bit)
 *	fDeleteOnRelease	delete handle on release.
 *	ppLkbyt			pointer to ILockBytes interface.
 *
 * RETURNS
 *	Staddard OLE error return codes.
 *
 */
HRESULT WINAPI CreateILockBytesOnHGlobal16(
	HGLOBAL16      hGlobal,          /* [in] */
	BOOL16         fDeleteOnRelease, /* [in] */
	LPLOCKBYTES16 *ppLkbyt)          /* [out] (ptr to SEGPTR!) */
{
  HGLOBALLockBytesImpl16* newLockBytes; /* SEGPTR */

  newLockBytes = HGLOBALLockBytesImpl16_Construct(hGlobal, fDeleteOnRelease);

  if (newLockBytes != NULL)
    return HGLOBALLockBytesImpl16_QueryInterface((ILockBytes16*)newLockBytes,
                                   &IID_ILockBytes,
                                   (void**)ppLkbyt);
  return E_OUTOFMEMORY;
}
