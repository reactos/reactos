/*************************************************************************
** 
**    OLE 2 Utility Code
**    
**    enumfetc.c
**    
**    This file contains a standard implementation of IEnumFormatEtc
**    interface.  
**    This file is part of the OLE 2.0 User Interface support library.
**    
**    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
**
*************************************************************************/

#define STRICT  1
#include <win32.h>
#include <shellapi.h>
#include <ole2.h>

#include "enumfetc.h"

STDAPI_(void) OleStdFree(LPVOID pmem);
STDAPI_(BOOL) OleStdCopyFormatEtc(LPFORMATETC petcDest, LPFORMATETC petcSrc);

typedef struct tagOleStdEnumFmtEtc {
  IEnumFORMATETCVtbl FAR* lpVtbl;
  ULONG m_dwRefs;      /* referance count */
  WORD m_wIndex;       /* current index in list */
  WORD m_wCount;       /* how many items in list */
  LPFORMATETC m_lpEtc;  /* list of formatetc */
} OLESTDENUMFMTETC, FAR* LPOLESTDENUMFMTETC;

VOID  OleStdEnumFmtEtc_Destroy(LPOLESTDENUMFMTETC pEtc);

STDMETHODIMP OleStdEnumFmtEtc_QueryInterface(
        LPENUMFORMATETC lpThis, REFIID riid, LPVOID FAR* ppobj);
STDMETHODIMP_(ULONG)  OleStdEnumFmtEtc_AddRef(LPENUMFORMATETC lpThis);
STDMETHODIMP_(ULONG)  OleStdEnumFmtEtc_Release(LPENUMFORMATETC lpThis);
STDMETHODIMP  OleStdEnumFmtEtc_Next(LPENUMFORMATETC lpThis, ULONG celt,
                                  LPFORMATETC rgelt, ULONG FAR* pceltFetched);
STDMETHODIMP  OleStdEnumFmtEtc_Skip(LPENUMFORMATETC lpThis, ULONG celt);
STDMETHODIMP  OleStdEnumFmtEtc_Reset(LPENUMFORMATETC lpThis);
STDMETHODIMP  OleStdEnumFmtEtc_Clone(LPENUMFORMATETC lpThis,
                                     LPENUMFORMATETC FAR* ppenum);
                       
static IEnumFORMATETCVtbl g_EnumFORMATETCVtbl = {
        OleStdEnumFmtEtc_QueryInterface,
        OleStdEnumFmtEtc_AddRef,
        OleStdEnumFmtEtc_Release,
        OleStdEnumFmtEtc_Next,
        OleStdEnumFmtEtc_Skip,
        OleStdEnumFmtEtc_Reset,
        OleStdEnumFmtEtc_Clone,
};

/////////////////////////////////////////////////////////////////////////////

        
STDAPI_(LPENUMFORMATETC)        
  OleStdEnumFmtEtc_Create(WORD wCount, LPFORMATETC lpEtc)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPMALLOC lpMalloc=NULL;
  LPOLESTDENUMFMTETC lpEF=NULL;
  DWORD dwSize;
  WORD i;
  HRESULT hRes;

  hRes = CoGetMalloc(MEMCTX_TASK, &lpMalloc);
  if (hRes != NOERROR) {
    return NULL;
  }

  lpEF = (LPOLESTDENUMFMTETC)lpMalloc->lpVtbl->Alloc(lpMalloc,
                                                 sizeof(OLESTDENUMFMTETC));
  if (lpEF == NULL) {
    goto errReturn;
  }

  lpEF->lpVtbl = &g_EnumFORMATETCVtbl;
  lpEF->m_dwRefs = 1;
  lpEF->m_wCount = wCount;
  lpEF->m_wIndex = 0;
  
  dwSize = sizeof(FORMATETC) * lpEF->m_wCount;

  lpEF->m_lpEtc = (LPFORMATETC)lpMalloc->lpVtbl->Alloc(lpMalloc, dwSize);
  if (lpEF->m_lpEtc == NULL) {
    goto errReturn;
  }

  for (i=0; i<wCount; i++) {
    OleStdCopyFormatEtc(
            (LPFORMATETC)&(lpEF->m_lpEtc[i]), (LPFORMATETC)&(lpEtc[i]));
  }

  return (LPENUMFORMATETC)lpEF;
  
errReturn:
  if (lpEF != NULL) {
    lpMalloc->lpVtbl->Free(lpMalloc, lpEF);
  }
  
  if (lpMalloc != NULL) {
    lpMalloc->lpVtbl->Release(lpMalloc);
  }
  
  return NULL;

} /* OleStdEnumFmtEtc_Create()
   */


VOID
  OleStdEnumFmtEtc_Destroy(LPOLESTDENUMFMTETC lpEF)
//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
{
    LPMALLOC lpMalloc=NULL;
    WORD i;

    if (lpEF != NULL) {

        if (CoGetMalloc(MEMCTX_TASK, &lpMalloc) == NOERROR) {

            /* OLE2NOTE: we MUST free any memory that was allocated for
            **    TARGETDEVICES contained within the FORMATETC elements.
            */
            for (i=0; i<lpEF->m_wCount; i++) {
                OleStdFree(lpEF->m_lpEtc[i].ptd);
            }

            if (lpEF->m_lpEtc != NULL) {
                lpMalloc->lpVtbl->Free(lpMalloc, lpEF->m_lpEtc);
            }

            lpMalloc->lpVtbl->Free(lpMalloc, lpEF);
            lpMalloc->lpVtbl->Release(lpMalloc);
        }
    }
} /* OleStdEnumFmtEtc_Destroy()
   */


STDMETHODIMP
  OleStdEnumFmtEtc_QueryInterface(
                LPENUMFORMATETC lpThis, REFIID riid, LPVOID FAR* ppobj)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;
  *ppobj = NULL;
  
  if (IsEqualIID(riid,&IID_IUnknown) || IsEqualIID(riid,&IID_IEnumFORMATETC)){
    *ppobj = (LPVOID)lpEF;
  }

  if (*ppobj == NULL) return ResultFromScode(S_FALSE);
  else{
    OleStdEnumFmtEtc_AddRef(lpThis);
    return NOERROR;
  }
  
} /* OleStdEnumFmtEtc_QueryInterface()
   */


STDMETHODIMP_(ULONG)
  OleStdEnumFmtEtc_AddRef(LPENUMFORMATETC lpThis)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;
  return lpEF->m_dwRefs++;

} /* OleStdEnumFmtEtc_AddRef()
   */


STDMETHODIMP_(ULONG)
  OleStdEnumFmtEtc_Release(LPENUMFORMATETC lpThis)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;
  DWORD dwRefs = --lpEF->m_dwRefs;

  if (dwRefs == 0) 
    OleStdEnumFmtEtc_Destroy(lpEF);

  return dwRefs;

} /* OleStdEnumFmtEtc_Release()
   */


STDMETHODIMP 
  OleStdEnumFmtEtc_Next(LPENUMFORMATETC lpThis, ULONG celt, LPFORMATETC rgelt,
                      ULONG FAR* pceltFetched)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;
  ULONG i=0;
  WORD wOffset;

  if (rgelt == NULL) {
    return ResultFromScode(E_INVALIDARG);
  }
  
  while (i < celt) {
    wOffset = lpEF->m_wIndex + (WORD)i;

    if (wOffset < lpEF->m_wCount) {
      OleStdCopyFormatEtc(
            (LPFORMATETC)&(rgelt[i]), (LPFORMATETC)&(lpEF->m_lpEtc[wOffset]));
      lpEF->m_wIndex ++;
      i++;
    }else{
      break;
    }
  }

  if (pceltFetched != NULL) {
    *pceltFetched = i;
  }

  if (i != celt) {
    return ResultFromScode(S_FALSE);
  }

  return NOERROR;
} /* OleStdEnumFmtEtc_Next()
   */


STDMETHODIMP 
  OleStdEnumFmtEtc_Skip(LPENUMFORMATETC lpThis, ULONG celt)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;
  ULONG i=0;
  WORD wOffset;

  while (i < celt) {
    wOffset = lpEF->m_wIndex + (WORD)i;

    if (wOffset < lpEF->m_wCount) {
      lpEF->m_wIndex ++;
      i++;
    }else{
      break;
    }
  }

  if (i != celt) {
    return ResultFromScode(S_FALSE);
  }

  return NOERROR;
} /* OleStdEnumFmtEtc_Skip()
   */


STDMETHODIMP 
  OleStdEnumFmtEtc_Reset(LPENUMFORMATETC lpThis)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;
  lpEF->m_wIndex = 0;

  return NOERROR;
} /* OleStdEnumFmtEtc_Reset()
   */


STDMETHODIMP 
  OleStdEnumFmtEtc_Clone(LPENUMFORMATETC lpThis, LPENUMFORMATETC FAR* ppenum)
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
{
  LPOLESTDENUMFMTETC lpEF = (LPOLESTDENUMFMTETC)lpThis;

  if (ppenum == NULL) {
    return ResultFromScode(E_INVALIDARG);
  }
  
  *ppenum = OleStdEnumFmtEtc_Create(lpEF->m_wCount, lpEF->m_lpEtc);

  return (*ppenum != NULL ? NOERROR : ResultFromScode(E_OUTOFMEMORY));

} /* OleStdEnumFmtEtc_Clone()
   */



/* OleStdMalloc
** ------------
**    allocate memory using the currently active IMalloc* allocator
*/
STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize)
{
    LPVOID pout;
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != S_OK) {
//        OleDbgAssertSz(0, szAssertMemAlloc);
        return NULL;
    }

    pout = (LPVOID)pmalloc->lpVtbl->Alloc(pmalloc, ulSize);

    if (pmalloc != NULL) {
        ULONG refs = pmalloc->lpVtbl->Release(pmalloc);
    }

    return pout;
}

/*
 * OleStdCopyTargetDevice()
 *
 * Purpose:
 *  duplicate a TARGETDEVICE struct. this function allocates memory for 
 *  the copy. the caller MUST free the allocated copy when done with it 
 *  using the standard allocator returned from CoGetMalloc.
 *  (OleStdFree can be used to free the copy).
 *
 * Parameters:
 *  ptdSrc      pointer to source TARGETDEVICE
 *
 * Return Value:
 *    pointer to allocated copy of ptdSrc
 *    if ptdSrc==NULL then retuns NULL is returned.
 *    if ptdSrc!=NULL and memory allocation fails, then NULL is returned
 */
STDAPI_(DVTARGETDEVICE FAR*) OleStdCopyTargetDevice(DVTARGETDEVICE FAR* ptdSrc)
{
  DVTARGETDEVICE FAR* ptdDest = NULL;

  if (ptdSrc == NULL) {
    return NULL;
  }

  if ((ptdDest = (DVTARGETDEVICE FAR*)OleStdMalloc(ptdSrc->tdSize)) != NULL) {
    hmemcpy(ptdDest, ptdSrc, (size_t)ptdSrc->tdSize);
  }

  return ptdDest;
} 



/*
 * OleStdCopyFormatEtc()
 *
 * Purpose:
 *  Copies the contents of a FORMATETC structure. this function takes 
 *  special care to copy correctly copying the pointer to the TARGETDEVICE 
 *  contained within the source FORMATETC structure.
 *  if the source FORMATETC has a non-NULL TARGETDEVICE, then a copy
 *  of the TARGETDEVICE will be allocated for the destination of the 
 *  FORMATETC (petcDest).
 *
 *  OLE2NOTE: the caller MUST free the allocated copy of the TARGETDEVICE 
 *  within the destination FORMATETC when done with it 
 *  using the standard allocator returned from CoGetMalloc.
 *  (OleStdFree can be used to free the copy).
 *
 * Parameters:
 *  petcDest      pointer to destination FORMATETC
 *  petcSrc       pointer to source FORMATETC
 *
 * Return Value:
 *    pointer to allocated copy of ptdSrc; retuns NULL if not successful
 */
STDAPI_(BOOL) OleStdCopyFormatEtc(LPFORMATETC petcDest, LPFORMATETC petcSrc)
{
  if ((petcDest == NULL) || (petcSrc == NULL)) {
    return FALSE;
  }

  petcDest->cfFormat = petcSrc->cfFormat;
  petcDest->ptd      = OleStdCopyTargetDevice(petcSrc->ptd);
  petcDest->dwAspect = petcSrc->dwAspect;
  petcDest->lindex   = petcSrc->lindex;
  petcDest->tymed    = petcSrc->tymed;

  return TRUE;
      
}

/* OleStdFree
** ----------
**    free memory using the currently active IMalloc* allocator
*/
STDAPI_(void) OleStdFree(LPVOID pmem)
{
    LPMALLOC pmalloc;

    if (pmem == NULL) 
        return;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != S_OK) {
//        OleDbgAssertSz(0, szAssertMemAlloc);
        return;
    }

    pmalloc->lpVtbl->Free(pmalloc, pmem);

    if (pmalloc != NULL) {
        ULONG refs = pmalloc->lpVtbl->Release(pmalloc);
    }

}

