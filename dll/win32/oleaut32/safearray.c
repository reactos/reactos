/*************************************************************************
 * OLE Automation - SafeArray
 *
 * This file contains the implementation of the SafeArray functions.
 *
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002-2003 Marcus Meissner
 * Copyright 2003 Jon Griffiths
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
/* Memory Layout of a SafeArray:
 *
 * -0x10: start of memory.
 * -0x10: GUID for VT_DISPATCH and VT_UNKNOWN safearrays (if FADF_HAVEIID)
 * -0x04: DWORD varianttype; (for all others, except VT_RECORD) (if FADF_HAVEVARTYPE)
 *  -0x4: IRecordInfo* iface;  (if FADF_RECORD, for VT_RECORD (can be NULL))
 *  0x00: SAFEARRAY,
 *  0x10: SAFEARRAYBOUNDS[0...]
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "variant.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(variant);

/************************************************************************
 * SafeArray {OLEAUT32}
 *
 * NOTES
 * The SafeArray data type provides the underlying interface for Ole
 * Automations arrays, used for example to represent array types in
 * Visual Basic(tm) and to gather user defined parameters for invocation through
 * an IDispatch interface.
 *
 * Safe arrays provide bounds checking and automatically manage the data
 * types they contain, for example handing reference counting and copying
 * of interface pointers. User defined types can be stored in arrays
 * using the IRecordInfo interface.
 *
 * There are two types of SafeArray, normal and vectors. Normal arrays can have
 * multiple dimensions and the data for the array is allocated separately from
 * the array header. This is the most flexible type of array. Vectors, on the
 * other hand, are fixed in size and consist of a single allocated block, and a
 * single dimension.
 *
 * DATATYPES
 * The following types of data can be stored within a SafeArray.
 * Numeric:
 *|  VT_I1, VT_UI1, VT_I2, VT_UI2, VT_I4, VT_UI4, VT_I8, VT_UI8, VT_INT, VT_UINT,
 *|  VT_R4, VT_R8, VT_CY, VT_DECIMAL
 * Interfaces:
 *|  VT_DISPATCH, VT_UNKNOWN, VT_RECORD
 * Other:
 *|  VT_VARIANT, VT_INT_PTR, VT_UINT_PTR, VT_BOOL, VT_ERROR, VT_DATE, VT_BSTR
 *
 * FUNCTIONS
 *  BstrFromVector()
 *  VectorFromBstr()
 */

/* Undocumented hidden space before the start of a SafeArray descriptor */
#define SAFEARRAY_HIDDEN_SIZE sizeof(GUID)

/* features listed here are not propagated to newly created array or data copy
   created with SafeArrayCopy()/SafeArrayCopyData() */
static const USHORT ignored_copy_features =
        FADF_AUTO |
        FADF_STATIC |
        FADF_EMBEDDED |
        FADF_FIXEDSIZE |
        FADF_CREATEVECTOR;

/* Allocate memory */
static inline void* SAFEARRAY_Malloc(ULONG size)
{
  void *ret = CoTaskMemAlloc(size);
  if (ret)
    memset(ret, 0, size);
  return ret;
}

/* Free memory */
static inline void SAFEARRAY_Free(void *ptr)
{
  CoTaskMemFree(ptr);
}

/* Get the size of a supported VT type (0 means unsupported) */
static DWORD SAFEARRAY_GetVTSize(VARTYPE vt)
{
  switch (vt)
  {
    case VT_I1:
    case VT_UI1:      return sizeof(BYTE);
    case VT_BOOL:
    case VT_I2:
    case VT_UI2:      return sizeof(SHORT);
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:    return sizeof(LONG);
    case VT_R8:
    case VT_I8:
    case VT_UI8:      return sizeof(LONG64);
    case VT_INT:
    case VT_UINT:     return sizeof(INT);
    case VT_INT_PTR:
    case VT_UINT_PTR: return sizeof(UINT_PTR);
    case VT_CY:       return sizeof(CY);
    case VT_DATE:     return sizeof(DATE);
    case VT_BSTR:     return sizeof(BSTR);
    case VT_DISPATCH: return sizeof(LPDISPATCH);
    case VT_VARIANT:  return sizeof(VARIANT);
    case VT_UNKNOWN:  return sizeof(LPUNKNOWN);
    case VT_DECIMAL:  return sizeof(DECIMAL);
    /* Note: Return a non-zero size to indicate vt is valid. The actual size
     * of a UDT is taken from the result of IRecordInfo_GetSize().
     */
    case VT_RECORD:   return 32;
  }
  return 0;
}

/* Set the hidden data for an array */
static inline void SAFEARRAY_SetHiddenDWORD(SAFEARRAY* psa, DWORD dw)
{
  /* Implementation data is stored in the 4 bytes before the header */
  LPDWORD lpDw = (LPDWORD)psa;
  lpDw[-1] = dw;
}

/* Get the hidden data from an array */
static inline DWORD SAFEARRAY_GetHiddenDWORD(SAFEARRAY* psa)
{
  LPDWORD lpDw = (LPDWORD)psa;
  return lpDw[-1];
}

/* Get the number of cells in a SafeArray */
static ULONG SAFEARRAY_GetCellCount(const SAFEARRAY *psa)
{
  const SAFEARRAYBOUND* psab = psa->rgsabound;
  USHORT cCount = psa->cDims;
  ULONG ulNumCells = 1;

  while (cCount--)
  {
    /* This is a valid bordercase. See testcases. -Marcus */
    if (!psab->cElements)
      return 0;
    ulNumCells *= psab->cElements;
    psab++;
  }
  return ulNumCells;
}

/* Allocate a descriptor for an array */
static HRESULT SAFEARRAY_AllocDescriptor(ULONG ulSize, SAFEARRAY **ppsaOut)
{
  char *ptr = SAFEARRAY_Malloc(ulSize + SAFEARRAY_HIDDEN_SIZE);

  if (!ptr)
  {
      *ppsaOut = NULL;
      return E_OUTOFMEMORY;
  }

  *ppsaOut = (SAFEARRAY*)(ptr + SAFEARRAY_HIDDEN_SIZE);
  return S_OK;
}

/* Set the features of an array */
static void SAFEARRAY_SetFeatures(VARTYPE vt, SAFEARRAY *psa)
{
  /* Set the IID if we have one, otherwise set the type */
  if (vt == VT_DISPATCH)
  {
    psa->fFeatures = FADF_HAVEIID;
    SafeArraySetIID(psa, &IID_IDispatch);
  }
  else if (vt == VT_UNKNOWN)
  {
    psa->fFeatures = FADF_HAVEIID;
    SafeArraySetIID(psa, &IID_IUnknown);
  }
  else if (vt == VT_RECORD)
    psa->fFeatures = FADF_RECORD;
  else
  {
    psa->fFeatures = FADF_HAVEVARTYPE;
    SAFEARRAY_SetHiddenDWORD(psa, vt);
  }
}

/* Create an array */
static SAFEARRAY* SAFEARRAY_Create(VARTYPE vt, UINT cDims, const SAFEARRAYBOUND *rgsabound, ULONG ulSize)
{
  SAFEARRAY *psa = NULL;
  unsigned int i;

  if (!rgsabound)
    return NULL;

  if (SUCCEEDED(SafeArrayAllocDescriptorEx(vt, cDims, &psa)))
  {
    switch (vt)
    {
      case VT_BSTR:     psa->fFeatures |= FADF_BSTR; break;
      case VT_UNKNOWN:  psa->fFeatures |= FADF_UNKNOWN; break;
      case VT_DISPATCH: psa->fFeatures |= FADF_DISPATCH; break;
      case VT_VARIANT:  psa->fFeatures |= FADF_VARIANT; break;
    }

    for (i = 0; i < cDims; i++)
      memcpy(psa->rgsabound + i, rgsabound + cDims - 1 - i, sizeof(SAFEARRAYBOUND));

    if (ulSize)
      psa->cbElements = ulSize;

    if (!psa->cbElements || FAILED(SafeArrayAllocData(psa)))
    {
      SafeArrayDestroyDescriptor(psa);
      psa = NULL;
    }
  }
  return psa;
}

/* Create an array as a vector */
static SAFEARRAY* SAFEARRAY_CreateVector(VARTYPE vt, LONG lLbound, ULONG cElements, ULONG ulSize)
{
  SAFEARRAY *psa = NULL;

  if (ulSize || (vt == VT_RECORD))
  {
    /* Allocate the header and data together */
    if (SUCCEEDED(SAFEARRAY_AllocDescriptor(sizeof(SAFEARRAY) + ulSize * cElements, &psa)))
    {
      SAFEARRAY_SetFeatures(vt, psa);

      psa->cDims = 1;
      psa->fFeatures |= FADF_CREATEVECTOR;
      psa->pvData = &psa[1]; /* Data follows the header */
      psa->cbElements = ulSize;
      psa->rgsabound[0].cElements = cElements;
      psa->rgsabound[0].lLbound = lLbound;

      switch (vt)
      {
        case VT_BSTR:     psa->fFeatures |= FADF_BSTR; break;
        case VT_UNKNOWN:  psa->fFeatures |= FADF_UNKNOWN; break;
        case VT_DISPATCH: psa->fFeatures |= FADF_DISPATCH; break;
        case VT_VARIANT:  psa->fFeatures |= FADF_VARIANT; break;
      }
    }
  }
  return psa;
}

/* Free data items in an array */
static HRESULT SAFEARRAY_DestroyData(SAFEARRAY *psa, ULONG ulStartCell)
{
  if (psa->pvData && !(psa->fFeatures & FADF_DATADELETED))
  {
    ULONG ulCellCount = SAFEARRAY_GetCellCount(psa);

    if (ulStartCell > ulCellCount) {
      FIXME("unexpected ulCellCount %ld, start %ld\n", ulCellCount, ulStartCell);
      return E_UNEXPECTED;
    }

    ulCellCount -= ulStartCell;

    if (psa->fFeatures & (FADF_UNKNOWN|FADF_DISPATCH))
    {
      LPUNKNOWN *lpUnknown = (LPUNKNOWN *)psa->pvData + ulStartCell;

      while(ulCellCount--)
      {
        if (*lpUnknown)
          IUnknown_Release(*lpUnknown);
        lpUnknown++;
      }
    }
    else if (psa->fFeatures & FADF_RECORD)
    {
      IRecordInfo *lpRecInfo;

      if (SUCCEEDED(SafeArrayGetRecordInfo(psa, &lpRecInfo)))
      {
        PBYTE pRecordData = psa->pvData;
        while(ulCellCount--)
        {
          IRecordInfo_RecordClear(lpRecInfo, pRecordData);
          pRecordData += psa->cbElements;
        }
        IRecordInfo_Release(lpRecInfo);
      }
    }
    else if (psa->fFeatures & FADF_BSTR)
    {
      BSTR* lpBstr = (BSTR*)psa->pvData + ulStartCell;

      while(ulCellCount--)
      {
        SysFreeString(*lpBstr);
        lpBstr++;
      }
    }
    else if (psa->fFeatures & FADF_VARIANT)
    {
      VARIANT* lpVariant = (VARIANT*)psa->pvData + ulStartCell;

      while(ulCellCount--)
      {
        HRESULT hRet = VariantClear(lpVariant);

        if (FAILED(hRet)) FIXME("VariantClear of element failed!\n");
        lpVariant++;
      }
    }
  }
  return S_OK;
}

/* Copy data items from one array to another. Destination data is freed before copy. */
static HRESULT SAFEARRAY_CopyData(SAFEARRAY *psa, SAFEARRAY *dest)
{
  HRESULT hr = S_OK;

  if (!psa->pvData)
    return S_OK;

  if (!dest->pvData || psa->fFeatures & FADF_DATADELETED)
    return E_INVALIDARG;
  else
  {
    ULONG ulCellCount = SAFEARRAY_GetCellCount(psa);

    dest->fFeatures = (dest->fFeatures & FADF_CREATEVECTOR) | (psa->fFeatures & ~ignored_copy_features);

    if (psa->fFeatures & FADF_VARIANT)
    {
      VARIANT *src_var = psa->pvData;
      VARIANT *dest_var = dest->pvData;

      while(ulCellCount--)
      {
        HRESULT hRet;

        /* destination is cleared automatically */
        hRet = VariantCopy(dest_var, src_var);
        if (FAILED(hRet)) FIXME("VariantCopy failed with %#lx, element %lu.\n", hRet, ulCellCount);
        src_var++;
        dest_var++;
      }
    }
    else if (psa->fFeatures & FADF_BSTR)
    {
      BSTR *src_bstr = psa->pvData;
      BSTR *dest_bstr = dest->pvData;

      while(ulCellCount--)
      {
        SysFreeString(*dest_bstr);
        if (*src_bstr)
        {
          *dest_bstr = SysAllocStringByteLen((char*)*src_bstr, SysStringByteLen(*src_bstr));
          if (!*dest_bstr)
            return E_OUTOFMEMORY;
        }
        else
          *dest_bstr = NULL;
        src_bstr++;
        dest_bstr++;
      }
    }
    else if (psa->fFeatures & FADF_RECORD)
    {
      BYTE *dest_data = dest->pvData;
      BYTE *src_data = psa->pvData;
      IRecordInfo *record;

      SafeArrayGetRecordInfo(psa, &record);
      while (ulCellCount--)
      {
          /* RecordCopy() clears destination record */
          hr = IRecordInfo_RecordCopy(record, src_data, dest_data);
          if (FAILED(hr)) break;
          src_data += psa->cbElements;
          dest_data += psa->cbElements;
      }

      SafeArraySetRecordInfo(dest, record);
      /* This value is set to 32 bytes by default on descriptor creation,
         update with actual structure size. */
      dest->cbElements = psa->cbElements;
      IRecordInfo_Release(record);
    }
    else if (psa->fFeatures & (FADF_UNKNOWN|FADF_DISPATCH))
    {
      IUnknown **dest_unk = dest->pvData;
      IUnknown **src_unk = psa->pvData;

      /* release old iface, addref new one */
      while (ulCellCount--)
      {
          if (*dest_unk)
              IUnknown_Release(*dest_unk);
          *dest_unk = *src_unk;
          if (*dest_unk)
              IUnknown_AddRef(*dest_unk);
          src_unk++;
          dest_unk++;
      }
    }
    else
    {
      /* Copy the data over */
      memcpy(dest->pvData, psa->pvData, ulCellCount * psa->cbElements);
    }

    if (psa->fFeatures & FADF_HAVEIID)
    {
      GUID guid;
      SafeArrayGetIID(psa, &guid);
      SafeArraySetIID(dest, &guid);
    }
    else if (psa->fFeatures & FADF_HAVEVARTYPE)
    {
      SAFEARRAY_SetHiddenDWORD(dest, SAFEARRAY_GetHiddenDWORD(psa));
    }
  }

  return hr;
}

/*************************************************************************
 *		SafeArrayAllocDescriptor (OLEAUT32.36)
 *
 * Allocate and initialise a descriptor for a SafeArray.
 *
 * PARAMS
 *  cDims   [I] Number of dimensions of the array
 *  ppsaOut [O] Destination for new descriptor
 *
 * RETURNS
 * Success: S_OK. ppsaOut is filled with a newly allocated descriptor.
 * Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayAllocDescriptor(UINT cDims, SAFEARRAY **ppsaOut)
{
  LONG allocSize;
  HRESULT hr;

  TRACE("(%d,%p)\n", cDims, ppsaOut);
  
  if (!cDims || cDims >= 0x10000) /* Maximum 65535 dimensions */
    return E_INVALIDARG;

  if (!ppsaOut)
    return E_POINTER;

  /* We need enough space for the header and its bounds */
  allocSize = sizeof(SAFEARRAY) + sizeof(SAFEARRAYBOUND) * (cDims - 1);

  hr = SAFEARRAY_AllocDescriptor(allocSize, ppsaOut);
  if (FAILED(hr))
    return hr;

  (*ppsaOut)->cDims = cDims;

  TRACE("%d: %lu bytes allocated for descriptor.\n", cDims, allocSize);
  return S_OK;
}

/*************************************************************************
 *		SafeArrayAllocDescriptorEx (OLEAUT32.41)
 *
 * Allocate and initialise a descriptor for a SafeArray of a given type.
 *
 * PARAMS
 *  vt      [I] The type of items to store in the array
 *  cDims   [I] Number of dimensions of the array
 *  ppsaOut [O] Destination for new descriptor
 *
 * RETURNS
 *  Success: S_OK. ppsaOut is filled with a newly allocated descriptor.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - This function does not check that vt is an allowed VARTYPE.
 *  - Unlike SafeArrayAllocDescriptor(), vt is associated with the array.
 *  See SafeArray.
 */
HRESULT WINAPI SafeArrayAllocDescriptorEx(VARTYPE vt, UINT cDims, SAFEARRAY **ppsaOut)
{
  ULONG cbElements;
  HRESULT hRet;

  TRACE("(%s,%u,%p)\n", debugstr_vt(vt), cDims, ppsaOut);

  cbElements = SAFEARRAY_GetVTSize(vt);
  if (!cbElements)
    WARN("Creating a descriptor with an invalid VARTYPE!\n");

  hRet = SafeArrayAllocDescriptor(cDims, ppsaOut);

  if (SUCCEEDED(hRet))
  {
    SAFEARRAY_SetFeatures(vt, *ppsaOut);
    (*ppsaOut)->cbElements = cbElements;
  }
  return hRet;
}

/*************************************************************************
 *		SafeArrayAllocData (OLEAUT32.37)
 *
 * Allocate the data area of a SafeArray.
 *
 * PARAMS
 *  psa [I] SafeArray to allocate the data area of.
 *
 * RETURNS
 *  Success: S_OK. The data area is allocated and initialised.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  See SafeArray.
 */
HRESULT WINAPI SafeArrayAllocData(SAFEARRAY *psa)
{
  HRESULT hRet = E_INVALIDARG;
  
  TRACE("(%p)\n", psa);
  
  if (psa)
  {
    ULONG ulSize = SAFEARRAY_GetCellCount(psa);

    psa->pvData = SAFEARRAY_Malloc(ulSize * psa->cbElements);

    if (psa->pvData)
    {
      hRet = S_OK;
      TRACE("%lu bytes allocated for data at %p (%lu objects).\n",
            ulSize * psa->cbElements, psa->pvData, ulSize);
    }
    else
      hRet = E_OUTOFMEMORY;
  }
  return hRet;
}

/*************************************************************************
 *		SafeArrayCreate (OLEAUT32.15)
 *
 * Create a new SafeArray.
 *
 * PARAMS
 *  vt        [I] Type to store in the safe array
 *  cDims     [I] Number of array dimensions
 *  rgsabound [I] Bounds of the array dimensions
 *
 * RETURNS
 *  Success: A pointer to a new array object.
 *  Failure: NULL, if any parameter is invalid or memory allocation fails.
 *
 * NOTES
 *  Win32 allows arrays with 0 sized dimensions. This bug is not reproduced
 *  in the Wine implementation.
 *  See SafeArray.
 */
SAFEARRAY* WINAPI SafeArrayCreate(VARTYPE vt, UINT cDims, SAFEARRAYBOUND *rgsabound)
{
  TRACE("(%s,%u,%p)\n", debugstr_vt(vt), cDims, rgsabound);

  if (vt == VT_RECORD)
    return NULL;

  return SAFEARRAY_Create(vt, cDims, rgsabound, 0);
}

/*************************************************************************
 *		SafeArrayCreateEx (OLEAUT32.15)
 *
 * Create a new SafeArray.
 *
 * PARAMS
 *  vt        [I] Type to store in the safe array
 *  cDims     [I] Number of array dimensions
 *  rgsabound [I] Bounds of the array dimensions
 *  pvExtra   [I] Extra data
 *
 * RETURNS
 *  Success: A pointer to a new array object.
 *  Failure: NULL, if any parameter is invalid or memory allocation fails.
 *
 * NOTES
 * See SafeArray.
 */
SAFEARRAY* WINAPI SafeArrayCreateEx(VARTYPE vt, UINT cDims, SAFEARRAYBOUND *rgsabound, LPVOID pvExtra)
{
  ULONG ulSize = 0;
  IRecordInfo* iRecInfo = pvExtra;
  SAFEARRAY* psa;
 
  TRACE("(%s,%u,%p,%p)\n", debugstr_vt(vt), cDims, rgsabound, pvExtra);
 
  if (vt == VT_RECORD)
  {
    if  (!iRecInfo)
      return NULL;
    IRecordInfo_GetSize(iRecInfo, &ulSize);
  }
  psa = SAFEARRAY_Create(vt, cDims, rgsabound, ulSize);

  if (pvExtra)
  {
    switch(vt)
    {
      case VT_RECORD:
        SafeArraySetRecordInfo(psa, pvExtra);
        break;
      case VT_UNKNOWN:
      case VT_DISPATCH:
        SafeArraySetIID(psa, pvExtra);
        break;
    }
  }
  return psa;
}

/************************************************************************
 *		SafeArrayCreateVector (OLEAUT32.411)
 *
 * Create a one dimensional, contiguous SafeArray.
 *
 * PARAMS
 *  vt        [I] Type to store in the safe array
 *  lLbound   [I] Lower bound of the array
 *  cElements [I] Number of elements in the array
 *
 * RETURNS
 *  Success: A pointer to a new array object.
 *  Failure: NULL, if any parameter is invalid or memory allocation fails.
 *
 * NOTES
 * See SafeArray.
 */
SAFEARRAY* WINAPI SafeArrayCreateVector(VARTYPE vt, LONG lLbound, ULONG cElements)
{
  TRACE("%s, %ld, %lu.\n", debugstr_vt(vt), lLbound, cElements);

  if (vt == VT_RECORD)
    return NULL;

  return SAFEARRAY_CreateVector(vt, lLbound, cElements, SAFEARRAY_GetVTSize(vt));
}

/************************************************************************
 *		SafeArrayCreateVectorEx (OLEAUT32.411)
 *
 * Create a one dimensional, contiguous SafeArray.
 *
 * PARAMS
 *  vt        [I] Type to store in the safe array
 *  lLbound   [I] Lower bound of the array
 *  cElements [I] Number of elements in the array
 *  pvExtra   [I] Extra data
 *
 * RETURNS
 *  Success: A pointer to a new array object.
 *  Failure: NULL, if any parameter is invalid or memory allocation fails.
 *
 * NOTES
 * See SafeArray.
 */
SAFEARRAY* WINAPI SafeArrayCreateVectorEx(VARTYPE vt, LONG lLbound, ULONG cElements, LPVOID pvExtra)
{
  ULONG ulSize;
  IRecordInfo* iRecInfo = pvExtra;
  SAFEARRAY* psa;

  TRACE("%s, %ld, %lu, %p.\n", debugstr_vt(vt), lLbound, cElements, pvExtra);

  if (vt == VT_RECORD)
  {
    if  (!iRecInfo)
      return NULL;
    IRecordInfo_GetSize(iRecInfo, &ulSize);
  }
  else
    ulSize = SAFEARRAY_GetVTSize(vt);

  psa = SAFEARRAY_CreateVector(vt, lLbound, cElements, ulSize);

  if (pvExtra)
  {
    switch(vt)
    {
      case VT_RECORD:
        SafeArraySetRecordInfo(psa, iRecInfo);
        break;
      case VT_UNKNOWN:
      case VT_DISPATCH:
        SafeArraySetIID(psa, pvExtra);
        break;
    }
  }
  return psa;
}

/*************************************************************************
 *		SafeArrayDestroyDescriptor (OLEAUT32.38)
 *
 * Destroy a SafeArray.
 *
 * PARAMS
 *  psa [I] SafeArray to destroy.
 *
 * RETURNS
 *  Success: S_OK. The resources used by the array are freed.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayDestroyDescriptor(SAFEARRAY *psa)
{
  TRACE("(%p)\n", psa);
    
  if (psa)
  {
    LPVOID lpv = (char*)psa - SAFEARRAY_HIDDEN_SIZE;

    if (psa->cLocks)
      return DISP_E_ARRAYISLOCKED; /* Can't destroy a locked array */

    if (psa->fFeatures & FADF_RECORD)
      SafeArraySetRecordInfo(psa, NULL);

    if (psa->fFeatures & FADF_CREATEVECTOR &&
        !(psa->fFeatures & FADF_DATADELETED))
        SAFEARRAY_DestroyData(psa, 0); /* Data not previously deleted */

    SAFEARRAY_Free(lpv);
  }
  return S_OK;
}

/*************************************************************************
 *		SafeArrayLock (OLEAUT32.21)
 *
 * Increment the lock counter of a SafeArray.
 *
 * PARAMS
 *  psa [O] SafeArray to lock
 *
 * RETURNS
 *  Success: S_OK. The array lock is incremented.
 *  Failure: E_INVALIDARG if psa is NULL, or E_UNEXPECTED if too many locks
 *           are held on the array at once.
 *
 * NOTES
 *  In Win32 these locks are not thread safe.
 *  See SafeArray.
 */
HRESULT WINAPI SafeArrayLock(SAFEARRAY *psa)
{
  ULONG ulLocks;

  TRACE("(%p)\n", psa);
    
  if (!psa)
    return E_INVALIDARG;

  ulLocks = InterlockedIncrement( (LONG*) &psa->cLocks);

  if (ulLocks > 0xffff) /* Maximum of 16384 locks at a time */
  {
    WARN("Out of locks!\n");
    InterlockedDecrement( (LONG*) &psa->cLocks);
    return E_UNEXPECTED;
  }
  return S_OK;
}

/*************************************************************************
 *		SafeArrayUnlock (OLEAUT32.22)
 *
 * Decrement the lock counter of a SafeArray.
 *
 * PARAMS
 *  psa [O] SafeArray to unlock
 *
 * RETURNS
 *  Success: S_OK. The array lock is decremented.
 *  Failure: E_INVALIDARG if psa is NULL, or E_UNEXPECTED if no locks are
 *           held on the array.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayUnlock(SAFEARRAY *psa)
{
  TRACE("(%p)\n", psa);
  
  if (!psa)
    return E_INVALIDARG;

  if (InterlockedDecrement( (LONG*) &psa->cLocks) < 0)
  {
    WARN("Unlocked but no lock held!\n");
    InterlockedIncrement( (LONG*) &psa->cLocks);
    return E_UNEXPECTED;
  }
  return S_OK;
}

/*************************************************************************
 *		SafeArrayPutElement (OLEAUT32.26)
 *
 * Put an item into a SafeArray.
 *
 * PARAMS
 *  psa       [I] SafeArray to insert into
 *  rgIndices [I] Indices to insert at
 *  pvData    [I] Data to insert
 *
 * RETURNS
 *  Success: S_OK. The item is inserted
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayPutElement(SAFEARRAY *psa, LONG *rgIndices, void *pvData)
{
  HRESULT hRet;

  TRACE("(%p,%p,%p)\n", psa, rgIndices, pvData);

  if (!psa || !rgIndices)
    return E_INVALIDARG;

  hRet = SafeArrayLock(psa);

  if (SUCCEEDED(hRet))
  {
    PVOID lpvDest;

    hRet = SafeArrayPtrOfIndex(psa, rgIndices, &lpvDest);

    if (SUCCEEDED(hRet))
    {
      if (psa->fFeatures & FADF_VARIANT)
      {
        VARIANT* lpVariant = pvData;
        VARIANT* lpDest = lpvDest;

        hRet = VariantCopy(lpDest, lpVariant);
        if (FAILED(hRet)) FIXME("VariantCopy failed with %#lx.\n", hRet);
      }
      else if (psa->fFeatures & FADF_BSTR)
      {
        BSTR  lpBstr = (BSTR)pvData;
        BSTR* lpDest = lpvDest;

        SysFreeString(*lpDest);

        *lpDest = SysAllocStringByteLen((char*)lpBstr, SysStringByteLen(lpBstr));
        if (!*lpDest)
          hRet = E_OUTOFMEMORY;
      }
      else if (psa->fFeatures & (FADF_UNKNOWN|FADF_DISPATCH))
      {
        IUnknown  *lpUnknown = pvData;
        IUnknown **lpDest = lpvDest;

        if (lpUnknown)
          IUnknown_AddRef(lpUnknown);
        if (*lpDest)
          IUnknown_Release(*lpDest);
        *lpDest = lpUnknown;
      }
      else if (psa->fFeatures & FADF_RECORD)
      {
        IRecordInfo *record;

        SafeArrayGetRecordInfo(psa, &record);
        hRet = IRecordInfo_RecordCopy(record, pvData, lpvDest);
        IRecordInfo_Release(record);
      } else
        /* Copy the data over */
        memcpy(lpvDest, pvData, psa->cbElements);
    }
    SafeArrayUnlock(psa);
  }
  return hRet;
}


/*************************************************************************
 *		SafeArrayGetElement (OLEAUT32.25)
 *
 * Get an item from a SafeArray.
 *
 * PARAMS
 *  psa       [I] SafeArray to get from
 *  rgIndices [I] Indices to get from
 *  pvData    [O] Destination for data
 *
 * RETURNS
 *  Success: S_OK. The item data is returned in pvData.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayGetElement(SAFEARRAY *psa, LONG *rgIndices, void *pvData)
{
  HRESULT hRet;

  TRACE("(%p,%p,%p)\n", psa, rgIndices, pvData);
    
  if (!psa || !rgIndices || !pvData)
    return E_INVALIDARG;

  hRet = SafeArrayLock(psa);

  if (SUCCEEDED(hRet))
  {
    PVOID lpvSrc;

    hRet = SafeArrayPtrOfIndex(psa, rgIndices, &lpvSrc);

    if (SUCCEEDED(hRet))
    {
      if (psa->fFeatures & FADF_VARIANT)
      {
        VARIANT* lpVariant = lpvSrc;
        VARIANT* lpDest = pvData;

        /* The original content of pvData is ignored. */
        V_VT(lpDest) = VT_EMPTY;
        hRet = VariantCopy(lpDest, lpVariant);
        if (FAILED(hRet)) FIXME("VariantCopy failed with %#lx.\n", hRet);
      }
      else if (psa->fFeatures & FADF_BSTR)
      {
        BSTR* lpBstr = lpvSrc;
        BSTR* lpDest = pvData;

        if (*lpBstr)
        {
          *lpDest = SysAllocStringByteLen((char*)*lpBstr, SysStringByteLen(*lpBstr));
          if (!*lpBstr)
            hRet = E_OUTOFMEMORY;
        }
        else
          *lpDest = NULL;
      }
      else if (psa->fFeatures & (FADF_UNKNOWN|FADF_DISPATCH))
      {
        IUnknown **src_unk = lpvSrc;
        IUnknown **dest_unk = pvData;

        if (*src_unk)
          IUnknown_AddRef(*src_unk);
        *dest_unk = *src_unk;
      }
      else if (psa->fFeatures & FADF_RECORD)
      {
        IRecordInfo *record;

        SafeArrayGetRecordInfo(psa, &record);
        hRet = IRecordInfo_RecordCopy(record, lpvSrc, pvData);
        IRecordInfo_Release(record);
      }
      else
        /* Copy the data over */
        memcpy(pvData, lpvSrc, psa->cbElements);
    }
    SafeArrayUnlock(psa);
  }
  return hRet;
}

/*************************************************************************
 *		SafeArrayGetUBound (OLEAUT32.19)
 *
 * Get the upper bound for a given SafeArray dimension
 *
 * PARAMS
 *  psa      [I] Array to get dimension upper bound from
 *  nDim     [I] The dimension number to get the upper bound of
 *  plUbound [O] Destination for the upper bound
 *
 * RETURNS
 *  Success: S_OK. plUbound contains the dimensions upper bound.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayGetUBound(SAFEARRAY *psa, UINT nDim, LONG *plUbound)
{
  TRACE("(%p,%d,%p)\n", psa, nDim, plUbound);
    
  if (!psa || !plUbound)
    return E_INVALIDARG;

  if(!nDim || nDim > psa->cDims)
    return DISP_E_BADINDEX;

  *plUbound = psa->rgsabound[psa->cDims - nDim].lLbound +
              psa->rgsabound[psa->cDims - nDim].cElements - 1;

  return S_OK;
}

/*************************************************************************
 *		SafeArrayGetLBound (OLEAUT32.20)
 *
 * Get the lower bound for a given SafeArray dimension
 *
 * PARAMS
 *  psa      [I] Array to get dimension lower bound from
 *  nDim     [I] The dimension number to get the lower bound of
 *  plLbound [O] Destination for the lower bound
 *
 * RETURNS
 *  Success: S_OK. plUbound contains the dimensions lower bound.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayGetLBound(SAFEARRAY *psa, UINT nDim, LONG *plLbound)
{
  TRACE("(%p,%d,%p)\n", psa, nDim, plLbound);

  if (!psa || !plLbound)
    return E_INVALIDARG;

  if(!nDim || nDim > psa->cDims)
    return DISP_E_BADINDEX;

  *plLbound = psa->rgsabound[psa->cDims - nDim].lLbound;
  return S_OK;
}

/*************************************************************************
 *		SafeArrayGetDim (OLEAUT32.17)
 *
 * Get the number of dimensions in a SafeArray.
 *
 * PARAMS
 *  psa [I] Array to get the dimensions of
 *
 * RETURNS
 *  The number of array dimensions in psa, or 0 if psa is NULL.
 *
 * NOTES
 * See SafeArray.
 */
UINT WINAPI SafeArrayGetDim(SAFEARRAY *psa)
{
  TRACE("(%p) returning %d\n", psa, psa ? psa->cDims : 0u);  
  return psa ? psa->cDims : 0;
}

/*************************************************************************
 *		SafeArrayGetElemsize (OLEAUT32.18)
 *
 * Get the size of an element in a SafeArray.
 *
 * PARAMS
 *  psa [I] Array to get the element size from
 *
 * RETURNS
 *  The size of a single element in psa, or 0 if psa is NULL.
 *
 * NOTES
 * See SafeArray.
 */
UINT WINAPI SafeArrayGetElemsize(SAFEARRAY *psa)
{
    TRACE("%p, returning %ld.\n", psa, psa ? psa->cbElements : 0u);
    return psa ? psa->cbElements : 0;
}

/*************************************************************************
 *		SafeArrayAccessData (OLEAUT32.23)
 *
 * Lock a SafeArray and return a pointer to its data.
 *
 * PARAMS
 *  psa     [I] Array to get the data pointer from
 *  ppvData [O] Destination for the arrays data pointer
 *
 * RETURNS
 *  Success: S_OK. ppvData contains the arrays data pointer, and the array
 *           is locked.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayAccessData(SAFEARRAY *psa, void **ppvData)
{
  HRESULT hr;

  TRACE("(%p,%p)\n", psa, ppvData);

  if(!psa || !ppvData)
    return E_INVALIDARG;

  hr = SafeArrayLock(psa);
  *ppvData = SUCCEEDED(hr) ? psa->pvData : NULL;

  return hr;
}


/*************************************************************************
 *		SafeArrayUnaccessData (OLEAUT32.24)
 *
 * Unlock a SafeArray after accessing its data.
 *
 * PARAMS
 *  psa     [I] Array to unlock
 *
 * RETURNS
 *  Success: S_OK. The array is unlocked.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayUnaccessData(SAFEARRAY *psa)
{
  TRACE("(%p)\n", psa);
  return SafeArrayUnlock(psa);
}

/************************************************************************
 *		SafeArrayPtrOfIndex (OLEAUT32.148)
 *
 * Get the address of an item in a SafeArray.
 *
 * PARAMS
 *  psa       [I] Array to get the items address from
 *  rgIndices [I] Index of the item in the array
 *  ppvData   [O] Destination for item address
 *
 * RETURNS
 *  Success: S_OK. ppvData contains a pointer to the item.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  This function does not lock the array.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayPtrOfIndex(SAFEARRAY *psa, LONG *rgIndices, void **ppvData)
{
  USHORT dim;
  ULONG cell = 0, dimensionSize = 1;
  SAFEARRAYBOUND* psab;
  LONG c1;

  TRACE("(%p,%p,%p)\n", psa, rgIndices, ppvData);
  
  /* The general formula for locating the cell number of an entry in an n
   * dimensional array (where cn = coordinate in dimension dn) is:
   *
   * c1 + c2 * sizeof(d1) + c3 * sizeof(d2) ... + cn * sizeof(c(n-1))
   *
   * We calculate the size of the last dimension at each step through the
   * dimensions to avoid recursing to calculate the last dimensions size.
   */
  if (!psa || !rgIndices || !ppvData)
    return E_INVALIDARG;

  psab = psa->rgsabound + psa->cDims - 1;
  c1 = *rgIndices++;

  if (c1 < psab->lLbound || c1 >= psab->lLbound + (LONG)psab->cElements)
    return DISP_E_BADINDEX; /* Initial index out of bounds */

  for (dim = 1; dim < psa->cDims; dim++)
  {
    dimensionSize *= psab->cElements;

    psab--;

    if (!psab->cElements ||
        *rgIndices < psab->lLbound ||
        *rgIndices >= psab->lLbound + (LONG)psab->cElements)
    return DISP_E_BADINDEX; /* Index out of bounds */

    cell += (*rgIndices - psab->lLbound) * dimensionSize;
    rgIndices++;
  }

  cell += (c1 - psa->rgsabound[psa->cDims - 1].lLbound);

  *ppvData = (char*)psa->pvData + cell * psa->cbElements;
  return S_OK;
}

/************************************************************************
 *		SafeArrayDestroyData (OLEAUT32.39)
 *
 * Destroy the data associated with a SafeArray.
 *
 * PARAMS
 *  psa [I] Array to delete the data from
 *
 * RETURNS
 *  Success: S_OK. All items and the item data are freed.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayDestroyData(SAFEARRAY *psa)
{
  HRESULT hr;

  TRACE("(%p)\n", psa);
  
  if (!psa)
    return E_INVALIDARG;

  if (psa->cLocks)
    return DISP_E_ARRAYISLOCKED; /* Can't delete a locked array */

  /* Delete the actual item data */
  hr = SAFEARRAY_DestroyData(psa, 0);
  if (FAILED(hr))
    return hr;

  if (psa->pvData)
  {
    if (psa->fFeatures & FADF_STATIC)
    {
      ZeroMemory(psa->pvData, SAFEARRAY_GetCellCount(psa) * psa->cbElements);
      return S_OK;
    }
    /* If this is not a vector, free the data memory block */
    if (!(psa->fFeatures & FADF_CREATEVECTOR))
    {
      SAFEARRAY_Free(psa->pvData);
      psa->pvData = NULL;
    }
    else
      psa->fFeatures |= FADF_DATADELETED; /* Mark the data deleted */

  }
  return S_OK;
}

/************************************************************************
 *		SafeArrayCopyData (OLEAUT32.412)
 *
 * Copy all data from one SafeArray to another.
 *
 * PARAMS
 *  psaSource [I] Source for copy
 *  psaTarget [O] Destination for copy
 *
 * RETURNS
 *  Success: S_OK. psaTarget contains a copy of psaSource.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  The two arrays must have the same number of dimensions and elements.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayCopyData(SAFEARRAY *psaSource, SAFEARRAY *psaTarget)
{
  int dim;

  TRACE("(%p,%p)\n", psaSource, psaTarget);
  
  if (!psaSource || !psaTarget ||
      psaSource->cDims != psaTarget->cDims ||
      psaSource->cbElements != psaTarget->cbElements)
    return E_INVALIDARG;

  /* Each dimension must be the same size */
  for (dim = psaSource->cDims - 1; dim >= 0 ; dim--)
    if (psaSource->rgsabound[dim].cElements !=
       psaTarget->rgsabound[dim].cElements)
      return E_INVALIDARG;

  return SAFEARRAY_CopyData(psaSource, psaTarget);
}

/************************************************************************
 *		SafeArrayDestroy (OLEAUT32.16)
 *
 * Destroy a SafeArray.
 *
 * PARAMS
 *  psa [I] Array to destroy
 *
 * RETURNS
 *  Success: S_OK. All resources used by the array are freed.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayDestroy(SAFEARRAY *psa)
{
  TRACE("(%p)\n", psa);

  if(!psa)
    return S_OK;

  if(psa->cLocks > 0)
    return DISP_E_ARRAYISLOCKED;

  /* Native doesn't check to see if the free succeeds */
  SafeArrayDestroyData(psa);
  SafeArrayDestroyDescriptor(psa);
  return S_OK;
}

/************************************************************************
 *		SafeArrayCopy (OLEAUT32.27)
 *
 * Make a duplicate of a SafeArray.
 *
 * PARAMS
 *  psa     [I] Source for copy
 *  ppsaOut [O] Destination for new copy
 *
 * RETURNS
 *  Success: S_OK. ppsaOut contains a copy of the array.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayCopy(SAFEARRAY *psa, SAFEARRAY **ppsaOut)
{
  HRESULT hRet;

  TRACE("(%p,%p)\n", psa, ppsaOut);

  if (!ppsaOut)
    return E_INVALIDARG;

  *ppsaOut = NULL;

  if (!psa)
    return S_OK; /* Handles copying of NULL arrays */

  if (!psa->cbElements)
    return E_INVALIDARG;

  if (psa->fFeatures & (FADF_RECORD|FADF_HAVEIID|FADF_HAVEVARTYPE))
  {
    VARTYPE vt;

    hRet = SafeArrayGetVartype(psa, &vt);
    if (SUCCEEDED(hRet))
      hRet = SafeArrayAllocDescriptorEx(vt, psa->cDims, ppsaOut);
  }
  else
  {
    hRet = SafeArrayAllocDescriptor(psa->cDims, ppsaOut);
    if (SUCCEEDED(hRet))
    {
      (*ppsaOut)->fFeatures = psa->fFeatures & ~ignored_copy_features;
      (*ppsaOut)->cbElements = psa->cbElements;
    }
  }

  if (SUCCEEDED(hRet))
  {
    /* Copy dimension bounds */
    memcpy((*ppsaOut)->rgsabound, psa->rgsabound, psa->cDims * sizeof(SAFEARRAYBOUND));

    (*ppsaOut)->pvData = SAFEARRAY_Malloc(SAFEARRAY_GetCellCount(psa) * psa->cbElements);
    if (!(*ppsaOut)->pvData)
    {
      SafeArrayDestroyDescriptor(*ppsaOut);
      *ppsaOut = NULL;
      return E_OUTOFMEMORY;
    }

    hRet = SAFEARRAY_CopyData(psa, *ppsaOut);
    if (FAILED(hRet))
    {
      SAFEARRAY_Free((*ppsaOut)->pvData);
      SafeArrayDestroyDescriptor(*ppsaOut);
      *ppsaOut = NULL;
      return hRet;
    }
  }

  return hRet;
}

/************************************************************************
 *		SafeArrayRedim (OLEAUT32.40)
 *
 * Changes the characteristics of the last dimension of a SafeArray
 *
 * PARAMS
 *  psa      [I] Array to change
 *  psabound [I] New bound details for the last dimension
 *
 * RETURNS
 *  Success: S_OK. psa is updated to reflect the new bounds.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayRedim(SAFEARRAY *psa, SAFEARRAYBOUND *psabound)
{
  SAFEARRAYBOUND *oldBounds;
  HRESULT hr;

  TRACE("(%p,%p)\n", psa, psabound);
  
  if (!psa || !psabound)
    return E_INVALIDARG;

  if (psa->fFeatures & FADF_FIXEDSIZE || psa->cLocks)
    return DISP_E_ARRAYISLOCKED;

  hr = SafeArrayLock(psa);
  if (FAILED(hr))
    return hr;

  oldBounds = psa->rgsabound;
  oldBounds->lLbound = psabound->lLbound;

  if (psabound->cElements != oldBounds->cElements)
  {
    if (psabound->cElements < oldBounds->cElements)
    {
      /* Shorten the final dimension. */
      ULONG ulStartCell = psabound->cElements *
                          (SAFEARRAY_GetCellCount(psa) / oldBounds->cElements);
      SAFEARRAY_DestroyData(psa, ulStartCell);
    }
    else
    {
      /* Lengthen the final dimension */
      ULONG ulOldSize, ulNewSize;
      PVOID pvNewData;

      ulOldSize = SAFEARRAY_GetCellCount(psa) * psa->cbElements;
      if (ulOldSize)
        ulNewSize = (ulOldSize / oldBounds->cElements) * psabound->cElements;
      else {
	int oldelems = oldBounds->cElements;
	oldBounds->cElements = psabound->cElements;
        ulNewSize = SAFEARRAY_GetCellCount(psa) * psa->cbElements;
	oldBounds->cElements = oldelems;
      }

      if (!(pvNewData = SAFEARRAY_Malloc(ulNewSize)))
      {
        SafeArrayUnlock(psa);
        return E_OUTOFMEMORY;
      }

      memcpy(pvNewData, psa->pvData, ulOldSize);
      SAFEARRAY_Free(psa->pvData);
      psa->pvData = pvNewData;
    }
    oldBounds->cElements = psabound->cElements;
  }

  SafeArrayUnlock(psa);
  return S_OK;
}

/************************************************************************
 *		SafeArrayGetVartype (OLEAUT32.77)
 *
 * Get the type of the items in a SafeArray.
 *
 * PARAMS
 *  psa [I] Array to get the type from
 *  pvt [O] Destination for the type
 *
 * RETURNS
 *  Success: S_OK. pvt contains the type of the items.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayGetVartype(SAFEARRAY* psa, VARTYPE* pvt)
{
  TRACE("(%p,%p)\n", psa, pvt);

  if (!psa || !pvt)
    return E_INVALIDARG;

  if (psa->fFeatures & FADF_RECORD)
    *pvt = VT_RECORD;
  else if ((psa->fFeatures & (FADF_HAVEIID|FADF_DISPATCH)) == (FADF_HAVEIID|FADF_DISPATCH))
    *pvt = VT_DISPATCH;
  else if (psa->fFeatures & FADF_HAVEIID)
    *pvt = VT_UNKNOWN;
  else if (psa->fFeatures & FADF_HAVEVARTYPE)
  {
    VARTYPE vt = SAFEARRAY_GetHiddenDWORD(psa);
    *pvt = vt;
  }
  else
    return E_INVALIDARG;

  return S_OK;
}

/************************************************************************
 *		SafeArraySetRecordInfo (OLEAUT32.@)
 *
 * Set the record info for a SafeArray.
 *
 * PARAMS
 *  psa    [I] Array to set the record info for
 *  pRinfo [I] Record info
 *
 * RETURNS
 *  Success: S_OK. The record info is stored with the array.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArraySetRecordInfo(SAFEARRAY *psa, IRecordInfo *pRinfo)
{
  IRecordInfo** dest = (IRecordInfo**)psa;

  TRACE("(%p,%p)\n", psa, pRinfo);
  
  if (!psa || !(psa->fFeatures & FADF_RECORD))
    return E_INVALIDARG;

  if (pRinfo)
    IRecordInfo_AddRef(pRinfo);

  if (dest[-1])
    IRecordInfo_Release(dest[-1]);

  dest[-1] = pRinfo;
  return S_OK;
}

/************************************************************************
 *		SafeArrayGetRecordInfo (OLEAUT32.@)
 *
 * Get the record info from a SafeArray.
 *
 * PARAMS
 *  psa    [I] Array to get the record info from
 *  pRinfo [O] Destination for the record info
 *
 * RETURNS
 *  Success: S_OK. pRinfo contains the record info, or NULL if there was none.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayGetRecordInfo(SAFEARRAY *psa, IRecordInfo **pRinfo)
{
  IRecordInfo** src = (IRecordInfo**)psa;

  TRACE("(%p,%p)\n", psa, pRinfo);

  if (!psa || !pRinfo || !(psa->fFeatures & FADF_RECORD))
    return E_INVALIDARG;

  *pRinfo = src[-1];

  if (*pRinfo)
    IRecordInfo_AddRef(*pRinfo);
  return S_OK;
}

/************************************************************************
 *		SafeArraySetIID (OLEAUT32.@)
 *
 * Set the IID for a SafeArray.
 *
 * PARAMS
 *  psa  [I] Array to set the IID from
 *  guid [I] IID
 *
 * RETURNS
 *  Success: S_OK. The IID is stored with the array
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArraySetIID(SAFEARRAY *psa, REFGUID guid)
{
  GUID* dest = (GUID*)psa;

  TRACE("(%p,%s)\n", psa, debugstr_guid(guid));

  if (!psa || !guid || !(psa->fFeatures & FADF_HAVEIID))
    return E_INVALIDARG;

  dest[-1] = *guid;
  return S_OK;
}

/************************************************************************
 *		SafeArrayGetIID (OLEAUT32.@)
 *
 * Get the IID from a SafeArray.
 *
 * PARAMS
 *  psa   [I] Array to get the ID from
 *  pGuid [O] Destination for the IID
 *
 * RETURNS
 *  Success: S_OK. pRinfo contains the IID, or NULL if there was none.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI SafeArrayGetIID(SAFEARRAY *psa, GUID *pGuid)
{
  GUID* src = (GUID*)psa;

  TRACE("(%p,%p)\n", psa, pGuid);

  if (!psa || !pGuid || !(psa->fFeatures & FADF_HAVEIID))
    return E_INVALIDARG;

  *pGuid = src[-1];
  return S_OK;
}

/************************************************************************
 *		VectorFromBstr (OLEAUT32.@)
 *
 * Create a SafeArray Vector from the bytes of a BSTR.
 *
 * PARAMS
 *  bstr [I] String to get bytes from
 *  ppsa [O] Destination for the array
 *
 * RETURNS
 *  Success: S_OK. ppsa contains the strings bytes as a VT_UI1 array.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI VectorFromBstr(BSTR bstr, SAFEARRAY **ppsa)
{
  SAFEARRAYBOUND sab;

  TRACE("(%p,%p)\n", bstr, ppsa);
  
  if (!ppsa)
    return E_INVALIDARG;

  sab.lLbound = 0;
  sab.cElements = SysStringByteLen(bstr);

  *ppsa = SAFEARRAY_Create(VT_UI1, 1, &sab, 0);

  if (*ppsa)
  {
    memcpy((*ppsa)->pvData, bstr, sab.cElements);
    return S_OK;
  }
  return E_OUTOFMEMORY;
}

/************************************************************************
 *		BstrFromVector (OLEAUT32.@)
 *
 * Create a BSTR from a SafeArray.
 *
 * PARAMS
 *  psa   [I] Source array
 *  pbstr [O] Destination for output BSTR
 *
 * RETURNS
 *  Success: S_OK. pbstr contains the arrays data.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  psa must be a 1 dimensional array of a 1 byte type.
 *
 * NOTES
 * See SafeArray.
 */
HRESULT WINAPI BstrFromVector(SAFEARRAY *psa, BSTR *pbstr)
{
  TRACE("(%p,%p)\n", psa, pbstr);

  if (!pbstr)
    return E_INVALIDARG;

  *pbstr = NULL;

  if (!psa || psa->cbElements != 1 || psa->cDims != 1)
    return E_INVALIDARG;

  *pbstr = SysAllocStringByteLen(psa->pvData, psa->rgsabound[0].cElements);
  if (!*pbstr)
    return E_OUTOFMEMORY;
  return S_OK;
}
