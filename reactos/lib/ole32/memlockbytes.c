/******************************************************************************
 *
 * Global memory implementation of ILockBytes.
 *
 * Copyright 1999 Thuy Nguyen
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <ole32/ole32.h>
#include <compobj.h>
#include <storage32.h>

#include <debug.h>

/******************************************************************************
 * HGLOBALLockBytesImpl definition.
 *
 * This class imlements the ILockBytes inteface and represents a byte array
 * object supported by an HGLOBAL pointer.
 */
struct HGLOBALLockBytesImpl
{
  /*
   * Needs to be the first item in the stuct 
   * since we want to cast this in an ILockBytes pointer
   */
  ICOM_VFIELD(ILockBytes);

  /*
   * Reference count
   */
  ULONG        ref;

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
HGLOBALLockBytesImpl* HGLOBALLockBytesImpl_Construct(
    HGLOBAL  hGlobal,
    BOOL     fDeleteOnRelease);

void HGLOBALLockBytesImpl_Destroy(HGLOBALLockBytesImpl* This);

HRESULT WINAPI HGLOBALLockBytesImpl_QueryInterface(
    ILockBytes*   iface,
    REFIID        riid,        /* [in] */
    void**        ppvObject);  /* [iid_is][out] */

ULONG WINAPI HGLOBALLockBytesImpl_AddRef(
    ILockBytes*   iface);

ULONG WINAPI HGLOBALLockBytesImpl_Release(
    ILockBytes*   iface);

HRESULT WINAPI HGLOBALLockBytesImpl_ReadAt(
    ILockBytes*    iface,
    ULARGE_INTEGER ulOffset,  /* [in] */
    void*          pv,        /* [length_is][size_is][out] */
    ULONG          cb,        /* [in] */
    ULONG*         pcbRead);  /* [out] */

HRESULT WINAPI HGLOBALLockBytesImpl_WriteAt(
    ILockBytes*    iface,
    ULARGE_INTEGER ulOffset,    /* [in] */
    const void*    pv,          /* [size_is][in] */
    ULONG          cb,          /* [in] */
    ULONG*         pcbWritten); /* [out] */

HRESULT WINAPI HGLOBALLockBytesImpl_Flush(
    ILockBytes*     iface);

HRESULT WINAPI HGLOBALLockBytesImpl_SetSize(
    ILockBytes*     iface,
    ULARGE_INTEGER  libNewSize);  /* [in] */

HRESULT WINAPI HGLOBALLockBytesImpl_LockRegion(
    ILockBytes*    iface,
    ULARGE_INTEGER libOffset,   /* [in] */
    ULARGE_INTEGER cb,          /* [in] */
    DWORD          dwLockType); /* [in] */

HRESULT WINAPI HGLOBALLockBytesImpl_UnlockRegion(
    ILockBytes*    iface,
    ULARGE_INTEGER libOffset,   /* [in] */
    ULARGE_INTEGER cb,          /* [in] */
    DWORD          dwLockType); /* [in] */

HRESULT WINAPI HGLOBALLockBytesImpl_Stat(
    ILockBytes*    iface,
    STATSTG*       pstatstg,     /* [out] */
    DWORD          grfStatFlag); /* [in]  */

/*
 * Virtual function table for the HGLOBALLockBytesImpl class.
 */
static ICOM_VTABLE(ILockBytes) HGLOBALLockBytesImpl_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

/******************************************************************************
 *           CreateILockBytesOnHGlobal     [OLE32.57]
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
 *           GetHGlobalFromILockBytes     [OLE32.70]
 */
HRESULT WINAPI GetHGlobalFromILockBytes(ILockBytes* plkbyt, HGLOBAL* phglobal)
{
  HGLOBALLockBytesImpl* const pMemLockBytes = (HGLOBALLockBytesImpl*)plkbyt;

  if (ICOM_VTBL(pMemLockBytes) == &HGLOBALLockBytesImpl_Vtbl)
    *phglobal = pMemLockBytes->supportHandle;
  else
    *phglobal = 0;

  if (*phglobal == 0)
    return E_INVALIDARG;

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
 * Params:
 *    hGlobal          - Handle that will support the stream. can be NULL.
 *    fDeleteOnRelease - Flag set to TRUE if the HGLOBAL will be released
 *                       when the IStream object is destroyed.
 */
HGLOBALLockBytesImpl* HGLOBALLockBytesImpl_Construct(HGLOBAL hGlobal,
                                                     BOOL    fDeleteOnRelease)
{
  HGLOBALLockBytesImpl* newLockBytes;
  newLockBytes = HeapAlloc(GetProcessHeap(), 0, sizeof(HGLOBALLockBytesImpl));
 
  if (newLockBytes!=0)
  {
    /*
     * Set up the virtual function table and reference count.
     */
    ICOM_VTBL(newLockBytes) = &HGLOBALLockBytesImpl_Vtbl;
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
void HGLOBALLockBytesImpl_Destroy(HGLOBALLockBytesImpl* This)
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
HRESULT WINAPI HGLOBALLockBytesImpl_QueryInterface(
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
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
  {
    *ppvObject = (ILockBytes*)This;
  }
  else if (memcmp(&IID_ILockBytes, riid, sizeof(IID_ILockBytes)) == 0)
  {
    *ppvObject = (ILockBytes*)This;
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
  HGLOBALLockBytesImpl_AddRef(iface);

  return S_OK;;
}

/******************************************************************************
 * This implements the IUnknown method AddRef for this
 * class
 */
ULONG WINAPI HGLOBALLockBytesImpl_AddRef(ILockBytes* iface)
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

  This->ref++;

  return This->ref;
}

/******************************************************************************
 * This implements the IUnknown method Release for this
 * class
 */
ULONG WINAPI HGLOBALLockBytesImpl_Release(ILockBytes* iface)
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

  ULONG newRef;

  This->ref--;

  newRef = This->ref;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (newRef==0)
  {
    HGLOBALLockBytesImpl_Destroy(This);
  }

  return newRef;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It reads a block of information from the byte array at the specified
 * offset.
 *
 * See the documentation of ILockBytes for more info.
 */
HRESULT WINAPI HGLOBALLockBytesImpl_ReadAt(
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
HRESULT WINAPI HGLOBALLockBytesImpl_WriteAt(
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
HRESULT WINAPI HGLOBALLockBytesImpl_Flush(ILockBytes* iface)
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
HRESULT WINAPI HGLOBALLockBytesImpl_SetSize(
      ILockBytes*     iface,
      ULARGE_INTEGER  libNewSize)   /* [in] */
{
  HGLOBALLockBytesImpl* const This=(HGLOBALLockBytesImpl*)iface;

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
  This->supportHandle = GlobalReAlloc(This->supportHandle,
                                      libNewSize.u.LowPart,
                                      0);

  if (This->supportHandle == 0)
    return STG_E_MEDIUMFULL;

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
HRESULT WINAPI HGLOBALLockBytesImpl_LockRegion(
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
HRESULT WINAPI HGLOBALLockBytesImpl_UnlockRegion(
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
HRESULT WINAPI HGLOBALLockBytesImpl_Stat(
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

