//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       transmit.cxx
//
//  Contents:   Transmit_as routines for oleprx32.dll.
//
//  Functions:  operator new
//              operator delete
//              EXCEPINFO_to_xmit
//              EXCEPINFO_from_xmit
//              EXCEPINFO_free_inst
//              EXCEPINFO_free_xmit
//              HGLOBAL_to_xmit
//              HGLOBAL_from_xmit
//              HGLOBAL_free_inst
//              HGLOBAL_free_xmit
//              HMETAFILEPICT_to_xmit
//              HMETAFILEPICT_from_xmit
//              HMETAFILEPICT_free_inst
//              HMETAFILEPICT_free_xmit
//              HENHMETAFILE_to_xmit
//              HENHMETAFILE_from_xmit
//              HENHMETAFILE_free_inst
//              HENHMETAFILE_free_xmit
//              HBITMAP_to_xmit
//              HBITMAP_from_xmit
//              HBITMAP_free_inst
//              HBITMAP_free_xmit
//              HBRUSH_to_xmit
//              HBRUSH_from_xmit
//              HBRUSH_free_inst
//              HBRUSH_free_xmit
//              STGMEDIUM_to_xmit
//              STGMEDIUM_from_xmit
//              STGMEDIUM_free_inst
//              STGMEDIUM_free_xmit
//              HACCEL_to_xmit
//              HACCEL_from_xmit
//              HACCEL_free_inst
//              HACCEL_free_xmit
//              UINT_to_xmit
//              UINT_from_xmit
//              UINT_free_inst
//              UINT_free_xmit
//              WPARAM_to_xmit
//              WPARAM_from_xmit
//              WPARAM_free_inst
//              WPARAM_free_xmit
//
//  History:    24-Aug-93   ShannonC    Created
//              24-Nov-93   ShannonC    Added HGLOBAL
//              14-May-94   DavePl      Added HENHMETAFILE
//              18-May-94   ShannonC    Added HACCEL, UINT, WPARAM
//              19-May-94   DavePl      Added HENHMETAFILE to STGMEDIUM code
//              25-May-96   JohannP     Moved to urlmon; minor modifications
//
//--------------------------------------------------------------------------
#include "stdrpc.hxx"
#pragma hdrstop

#include "objbase.h"
#include "transmit.h"
#include "crtsubst.h"

#ifndef _CHICAGO_
    HBRUSH     OleGdiConvertBrush(HBRUSH hbrush);
    HBRUSH     OleGdiCreateLocalBrush(HBRUSH hbrushRemote);
#endif  // _CHICAGO_


void __RPC_USER HENHMETAFILE_to_xmit (HENHMETAFILE __RPC_FAR *pHEnhMetafile,
    RemHENHMETAFILE __RPC_FAR * __RPC_FAR *ppxmit);
void __RPC_USER HENHMETAFILE_from_xmit( RemHENHMETAFILE __RPC_FAR *pxmit,
    HENHMETAFILE __RPC_FAR *pHEnhMetafile );
void __RPC_USER HENHMETAFILE_free_xmit( RemHENHMETAFILE __RPC_FAR *pxmit);
void __RPC_USER HPALETTE_to_xmit (HPALETTE __RPC_FAR *pHPALETTE,
    RemHPALETTE __RPC_FAR * __RPC_FAR *ppxmit);
void __RPC_USER HPALETTE_from_xmit( RemHPALETTE __RPC_FAR *pxmit,
    HPALETTE __RPC_FAR *pHPALETTE );
void __RPC_USER HPALETTE_free_xmit( RemHPALETTE __RPC_FAR *pxmit);
void __RPC_USER HPALETTE_free_inst( HPALETTE __RPC_FAR *pHPALETTE);




WINOLEAPI_(void) ReleaseStgMedium(LPSTGMEDIUM pStgMed);

// BUGBUG: setting NTDEBUG=retail does not build this retail, so i cant
//     use DBG to conditionally generate this code, hence i must
//     disable it for now.
//
// #if DBG==1
// #define Assert(a) ((a) ? NOERROR : FnAssert(#a, NULL, __FILE__, __LINE__))
// #else
#define Assert(a) ((void)0)
// #endif

#pragma code_seg(".orpc")

// we dont need these when we are in with ole32.dll
#if 0
//+-------------------------------------------------------------------------
//
//  Function:  operator new
7//
//  Synopsis:  Override operator new so we don't need C runtime library.
//
//--------------------------------------------------------------------------
void *
_CRTAPI1
operator new (size_t size)
{
    return CoTaskMemAlloc(size);
}


//+-------------------------------------------------------------------------
//
//  Function:  operator delete
//
//  Synopsis:  Override operator delete so we don't need C runtime library.
//
//--------------------------------------------------------------------------
void
_CRTAPI1
operator delete (void * pObj)
{
    CoTaskMemFree(pObj);
}
#endif

//+-------------------------------------------------------------------------
//
//  class:  CPunkForRelease
//
//  purpose:    special IUnknown for remoted STGMEDIUMs
//
//  history:    02-Mar-94   Rickhi      Created
//
//  notes:  This class is used to do the cleanup correctly when certain
//      types of storages are passed between processes or machines
//      in Nt.
//
//      GLOBAL, GDI, and BITMAP handles cannot be passed between
//      processes, so we actually copy the whole data and create a
//      new handle in the receiving process. However, STGMEDIUMs have
//      this weird behaviour where if PunkForRelease is non-NULL then
//      the sender is responsible for cleanup, not the receiver. Since
//      we create a new handle in the receiver, we would leak handles
//      if we didnt do some special processing.  So, we do the
//      following...
//
//          During STGMEDIUM_from_xmit, if there is a pUnkForRelease
//          replace it with a CPunkForRelease.  When Release is called
//          on the CPunkForRelease, do the necessary cleanup work,
//          then call the real PunkForRelease.
//
//+-------------------------------------------------------------------------

class   CPunkForRelease : public IUnknown
{
public:
    CPunkForRelease(STGMEDIUM *pStgMed);

    //  IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

private:
    ~CPunkForRelease(void);

    ULONG       _cRefs;                 //  reference count
    STGMEDIUM   _stgmed;                //  storage medium
    IUnknown  * _pUnkForRelease;        //  real pUnkForRelease
};


inline CPunkForRelease::CPunkForRelease(STGMEDIUM *pStgMed) :
    _cRefs(1),
    _stgmed(*pStgMed)
{
    //  NOTE: we assume the caller has verified pStgMed is not NULL,
    //  and the pUnkForRelease is non-null, otherwise there is no
    //  point in constructing this object.  The tymed must also be
    //  one of the special ones.

    Assert(pStgMed);
    Assert(pStgMed->tymed == TYMED_HGLOBAL ||
       pStgMed->tymed == TYMED_GDI  ||
       pStgMed->tymed == TYMED_MFPICT  ||
       pStgMed->tymed == TYMED_ENHMF);

    _pUnkForRelease = pStgMed->pUnkForRelease;
}


inline CPunkForRelease::~CPunkForRelease()
{
    //  since we really have our own copies of these handles, just
    //  pretend like the callee is responsible for the release, and
    //  recurse into ReleaseStgMedium to do the cleanup.

    _stgmed.pUnkForRelease = NULL;
    ReleaseStgMedium(&_stgmed);

    //  release the callers punk
    _pUnkForRelease->Release();
}

STDMETHODIMP_(ULONG) CPunkForRelease::AddRef(void)
{
    InterlockedIncrement((LONG *)&_cRefs);
    return _cRefs;
}

STDMETHODIMP_(ULONG) CPunkForRelease::Release(void)
{
    if (InterlockedDecrement((LONG *)&_cRefs) == 0)
    {
    delete this;
    return 0;
    }
    else
    return _cRefs;
}

STDMETHODIMP CPunkForRelease::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
    *ppv = (void *)(IUnknown *) this;
    AddRef();
    return S_OK;
    }
    else
    {
    *ppv = NULL;
    return E_NOINTERFACE;
    }
}



#ifdef UNUSED
//+-------------------------------------------------------------------------
//
//  Function:   EXCEPINFO_to_xmit
//
//  Synopsis:   Convert an EXCEPINFO to a RemEXCEPINFO structure so it can be sent
//                              over the network.
//
//--------------------------------------------------------------------------
void __RPC_USER EXCEPINFO_to_xmit (EXCEPINFO *pinst, RemEXCEPINFO **ppxmit)
{
    unsigned int cSource = 0;
    unsigned int cDescription = 0;
    unsigned int cHelpFile = 0;
    unsigned int *pCount;
    wchar_t *pTemp;

    if(pinst->pfnDeferredFillIn)
    {
    //Fill in the EXCEPINFO structure.
    (pinst->pfnDeferredFillIn) (pinst);
    }

    //Calculate the total size of the strings.
    if(pinst->bstrSource)
    {
    pCount = (unsigned int *) pinst->bstrSource;
    pCount--;
    cSource = *pCount;
    }

    if(pinst->bstrDescription)
    {
    pCount = (unsigned int *) pinst->bstrDescription;
    pCount--;
    cDescription = *pCount;
    }

    if(pinst->bstrHelpFile)
    {
    pCount = (unsigned int *) pinst->bstrHelpFile;
    pCount--;
    cHelpFile = *pCount;
    }

    *ppxmit = (RemEXCEPINFO *) NdrOleAllocate(sizeof(RemEXCEPINFO) +
                          ((cSource + cDescription + cHelpFile) * sizeof(wchar_t)));
    (*ppxmit)->wCode = pinst->wCode;
    (*ppxmit)->wReserved = pinst->wReserved;
    (*ppxmit)->dwHelpContext = pinst->dwHelpContext;
    (*ppxmit)->scode = pinst->scode;
    (*ppxmit)->cSource = cSource;
    (*ppxmit)->cDescription = cDescription;
    (*ppxmit)->cHelpFile = cHelpFile;

    pTemp = (*ppxmit)->strings;
    if(pinst->bstrSource)
    {
    memcpy(pTemp, pinst->bstrSource, (*ppxmit)->cSource * sizeof(wchar_t));
    pTemp += cSource;
    }

    if(pinst->bstrDescription)
    {
    memcpy(pTemp, pinst->bstrDescription, (*ppxmit)->cDescription * sizeof(wchar_t));
    pTemp += cDescription;
    }

    if(pinst->bstrHelpFile)
    {
    memcpy(pTemp, pinst->bstrHelpFile, (*ppxmit)->cHelpFile * sizeof(wchar_t));
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   EXCEPINFO_from_xmit
//
//  Synopsis:   Convert a RemEXCEPINFO structure to an EXCEPINFO.
//
//--------------------------------------------------------------------------
void __RPC_USER EXCEPINFO_from_xmit (RemEXCEPINFO *pxmit, EXCEPINFO *pinst)
{
    wchar_t *pTemp;
    unsigned int *pCount;

    pinst->wCode = pxmit->wCode;
    pinst->wReserved = pxmit->wReserved;
    pinst->bstrSource = 0;
    pinst->bstrDescription = 0;
    pinst->bstrHelpFile = 0;
    pinst->dwHelpContext = pxmit->dwHelpContext;
    pinst->pvReserved = 0;
    pinst->pfnDeferredFillIn = 0;
    pinst->scode = pxmit->scode;

    //unmarshal BSTRs
    pTemp = pxmit->strings;

    if(pxmit->cSource)
    {
    pCount = (unsigned int *) NdrOleAllocate(sizeof(int) + pxmit->cSource * sizeof(wchar_t) + sizeof(wchar_t));

    //set the BSTR count.
    *pCount = pxmit->cSource;
    pCount++;
    pinst->bstrSource = (BSTR) pCount;

    //copy the BSTR characters
    memcpy(pinst->bstrSource, pTemp, pxmit->cSource * sizeof(wchar_t));

    //zero-terminate the BSTR.
    pinst->bstrSource[pxmit->cSource] = 0;

    //advance the data pointer.
    pTemp += pxmit->cSource;
    }

    if(pxmit->cDescription)
    {
    pCount = (unsigned int *) NdrOleAllocate(sizeof(int) + pxmit->cDescription * sizeof(wchar_t) + sizeof(wchar_t));

    //set the character count.
    *pCount = pxmit->cDescription;
    pCount++;
    pinst->bstrDescription = (BSTR) pCount;

    //copy the characters
    memcpy(pinst->bstrDescription, pTemp, pxmit->cDescription *sizeof(wchar_t));

    //zero-terminate the BSTR.
    pinst->bstrDescription[pxmit->cDescription] = 0;

    //advance the data pointer.
    pTemp += pxmit->cDescription;
    }

    if(pxmit->cHelpFile)
    {
    pCount = (unsigned int *) NdrOleAllocate(sizeof(int) + pxmit->cHelpFile * sizeof(wchar_t) + sizeof(wchar_t));

    //set the BSTR count.
    *pCount = pxmit->cHelpFile;
    pCount++;
    pinst->bstrHelpFile = (BSTR) pCount;

    //copy the BSTR characters
    memcpy(pinst->bstrHelpFile, pTemp, pxmit->cHelpFile * sizeof(wchar_t));

    //zero-terminate the BSTR.
    pinst->bstrHelpFile[pxmit->cHelpFile] = 0;

    //advance the data pointer.
    pTemp += pxmit->cHelpFile;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   EXCEPINFO_free_inst
//
//  Synopsis:   Free the contents of an EXCEPINFO structure.
//
//--------------------------------------------------------------------------
void __RPC_USER EXCEPINFO_free_inst (EXCEPINFO *pinst)
{
    unsigned int *pInt;

    if(pinst)
    {
    if(pinst->bstrSource)
    {
        pInt = (unsigned int *) pinst->bstrSource;
        pInt--;
        NdrOleFree(pInt);
    }

    if(pinst->bstrDescription)
    {
        pInt = (unsigned int *) pinst->bstrDescription;
        pInt--;
        NdrOleFree(pInt);
    }

    if(pinst->bstrHelpFile)
    {
        pInt = (unsigned int *) pinst->bstrHelpFile;
        pInt--;
        NdrOleFree(pInt);
    }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   EXCEPINFO_free_xmit
//
//  Synopsis:   Free a RemEXCEPINFO previously obtained via EXCEPINFO_to_xmit.
//
//--------------------------------------------------------------------------
void __RPC_USER EXCEPINFO_free_xmit (RemEXCEPINFO *pxmit)
{
    if(pxmit)
    {
    NdrOleFree(pxmit);
    }
}

#endif //UNUSED
//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_to_xmit
//
//  Synopsis:   Convert an HGLOBAL to a RemHGLOBAL structure so it can be sent
//              over the network.
//
//  Derivation: We get the size of the global memory block,
//              allocate a RemHGLOBAL structure, then copy the contents
//              of the global memory block into the RemHGLOBAL structure.
//
//--------------------------------------------------------------------------

void __RPC_USER HGLOBAL_to_xmit (HGLOBAL *pinst, RemHGLOBAL **ppxmit)
{
    HGLOBAL hGlobal = *pinst;

    //calculate size - we give a null hGlobal a size of zero
    DWORD cbData = (DWORD) ((hGlobal)?GlobalSize(hGlobal):0);

    //allocate memory
    *ppxmit = (RemHGLOBAL *) NdrOleAllocate(sizeof(RemHGLOBAL) + cbData);

    // save size of variable length data
    (*ppxmit)->cbData = cbData;

    if (hGlobal != NULL)
    {
    // There is an hglobal to transmit
    (*ppxmit)->fNullHGlobal = FALSE;

    // Remember that an HGLOBAL can be alloc'd to zero size. So we
    // check whether there is anything to copy.
    if (cbData != 0)
    {
        // Copy the data
        void *pData = GlobalLock(hGlobal);
        memcpy((*ppxmit)->data, pData, cbData);
        GlobalUnlock(hGlobal);
    }
    }
    else
    {
    (*ppxmit)->fNullHGlobal = TRUE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_from_xmit
//
//  Synopsis:   Convert a RemHGLOBAL structure to an HGLOBAL.
//
//  Derivation: We get the data size, allocate a global memory block,
//                              then copy the data from the RemHGLOBAL structure to
//                              the global memory block.
//
//--------------------------------------------------------------------------
void __RPC_USER HGLOBAL_from_xmit (RemHGLOBAL __RPC_FAR *pxmit, HGLOBAL __RPC_FAR *pinst)
{
    // Default to NULL hglobal
    HGLOBAL hGlobal = NULL;
    void *pData;

    //allocate memory
    if (!pxmit->fNullHGlobal)
    {
    hGlobal = GlobalAlloc(GMEM_MOVEABLE, pxmit->cbData);

    if(hGlobal)
    {
        //copy the data
        pData = GlobalLock(hGlobal);
        if(pData)
        {
        memcpy(pData, pxmit->data, pxmit->cbData);
        GlobalUnlock(hGlobal);
        }
    }
    else
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }
    }

    *pinst = hGlobal;
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_free_inst
//
//  Synopsis:   Free an HGLOBAL.
//
//--------------------------------------------------------------------------
void __RPC_USER HGLOBAL_free_inst(HGLOBAL *pinst)
{
    if(pinst)
    {
    if(*pinst)
    {
        GlobalFree(*pinst);
    }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_free_xmit
//
//  Synopsis:   Free a RemHGLOBAL previously obtained via HGLOBAL_to_xmit.
//
//--------------------------------------------------------------------------
void __RPC_USER HGLOBAL_free_xmit(RemHGLOBAL *pxmit)
{
    if(pxmit != 0)
        NdrOleFree(pxmit);
}


//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_to_xmit
//
//  Synopsis:   Converts a metafilepict handle into a global handle
//              that can be remoted
//
//  Arguments:  [pHMetafilePict]-- pointer to the original metafile handle
//              [ppxmit]        -- set to point to the transmitted value
//
//  Returns:    void
//
//  Algorithm:  calls a private gdi api to convert the handle
//
//  History:    16-Nov-93 alexgo    created
//              07-Jan-94 rickhi    copy the metafile
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HMETAFILEPICT_to_xmit (HMETAFILEPICT __RPC_FAR *pHMetafilePict,
    RemHMETAFILEPICT __RPC_FAR * __RPC_FAR *ppxmit)
{
#ifdef NEW_GDI_MARSHALLING
    //  CODEWORK: we can use this in the Daytona and Local Cairo case,
    //            but not in Chicago or remote Cairo

    //calculate size
    DWORD cbData = sizeof(long);

    //allocate memory
    *ppxmit = (RemHMETAFILEPICT *) NdrOleAllocate(sizeof(RemHMETAFILEPICT) + cbData);

    if (*ppxmit)
    {
    //copy data
    (*ppxmit)->cbData = cbData;
    // BUGBUG: enable this code!
    //long lData = (long)OleGdiConvertMetaFilePict(*(HANDLE *)pHMetafilePict);

    memcpy((*ppxmit)->data, &lData, cbData);
    }
    else
    {
    RpcRaiseException(E_OUTOFMEMORY);
    }

#else

    //  lock the data
    METAFILEPICT *pmfp = (METAFILEPICT *)GlobalLock(*(HANDLE *)pHMetafilePict);

    if (pmfp)
    {
    // calculate the size needed to hold the windows metafile
    DWORD cbData = GetMetaFileBitsEx(pmfp->hMF, 0, NULL);

    // allocate memory
    *ppxmit = (RemHMETAFILEPICT *) NdrOleAllocate(sizeof(RemHMETAFILEPICT) + cbData);

    // copy data
    (*ppxmit)->cbData = cbData;
    (*ppxmit)->mm     = pmfp->mm;
    (*ppxmit)->xExt   = pmfp->xExt;
    (*ppxmit)->yExt   = pmfp->yExt;

    GetMetaFileBitsEx(pmfp->hMF, cbData, &((*ppxmit)->data[0]));

    GlobalUnlock(*(HANDLE *)pHMetafilePict);
    }
    else
    {
    RpcRaiseException(E_OUTOFMEMORY);
    }

#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_from_xmit
//
//  Synopsis:   Converts a global metafilepict handle into a metafile
//              handle that a process can use
//
//  Arguments:  [pxmit]         -- the transmitted global handle
//              [pHMetafilePict]-- where to put the local metafilepict handle
//
//  Returns:    void
//
//  Algorithm:  calls a private gdi api to convert the global handle
//
//  History:    16-Nov-93 alexgo    created
//              07-Jan-94 rickhi    copy the metafile
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HMETAFILEPICT_from_xmit( RemHMETAFILEPICT __RPC_FAR *pxmit,
    HMETAFILEPICT __RPC_FAR *pHMetafilePict )
{
#ifdef  NEW_GDI_MARSHALLLING
    //  CODEWORK: we can use this in the Daytona and Local Cairo case,
    //            but not in Chicago or remote Cairo

    long lh;
    memcpy(&lh, pxmit->data, pxmit->cbData);
    *pHMetafilePict = (HMETAFILE)GdiCreateLocalMetaFilePict(
                (HANDLE)lh);
#else

    // allocate memory
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(METAFILEPICT));
    *pHMetafilePict = hGlobal;

    if(hGlobal)
    {
    //copy the data
    METAFILEPICT *pmfp = (METAFILEPICT *)GlobalLock(hGlobal);
    if(pmfp)
    {
        pmfp->mm     = pxmit->mm;
        pmfp->xExt   = pxmit->xExt;
        pmfp->yExt   = pxmit->yExt;

        //  create a windows metatfile from the data
        pmfp->hMF = SetMetaFileBitsEx(pxmit->cbData, pxmit->data);
        GlobalUnlock(hGlobal);
    }
    }
    else
    {
    RpcRaiseException(E_OUTOFMEMORY);
    }

#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_free_xmit
//
//  Synopsis:   Free's the global metafilepict handle that gets remoted
//
//  Arguments:  [pxmit]         -- the transmitted metafilepict handle
//
//  Returns:    void
//
//  History:    16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HMETAFILEPICT_free_xmit( RemHMETAFILEPICT __RPC_FAR *pxmit)
{
    if(pxmit != 0)
        NdrOleFree(pxmit);
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_free_inst
//
//  Synopsis:   does nothing, as no memory is allocated
//
//  Arguments:  [pHMetafilePict]        -- pointer to the metafilepict
//
//  Returns:    void
//
//  Algorithm:
//
//  History:    16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HMETAFILEPICT_free_inst( HMETAFILEPICT __RPC_FAR *pHMetafilePict)
{
    METAFILEPICT *pmf;

    if(*pHMetafilePict)
    {
    pmf = (METAFILEPICT *) GlobalLock(*pHMetafilePict);
    if(pmf)
    {
        DeleteMetaFile(pmf->hMF);
        GlobalUnlock(*pHMetafilePict);
    }
    GlobalFree(*pHMetafilePict);
    }
}


#ifndef NEW_GDI_MARSHALLLING
//+-------------------------------------------------------------------------
//
//  Function:   GetColorTableSize
//
//  Synopsis:   computes the size of the color table using the info in
//      the supplied BITMAPINFO
//
//  Arguments:  [pbmi]   -- pointer to the BITMAPINFO for the bitmap
//
//  Returns:    size of the color table
//
//+-------------------------------------------------------------------------

ULONG  GetColorTableSize(BITMAPINFO *pbmi)
{
    // compute size of memory needed. it must account for the header
    // info, color table, and bitmap, as well as the RemHBITMAP.

    ULONG ulColorTableSize;
    if (pbmi->bmiHeader.biClrUsed)
    {
    // biClrUsed contains number of RGBQUADs used
    ulColorTableSize = pbmi->bmiHeader.biClrUsed;
    }
    else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
    {
    // size is 3 DWORD color masks. sizeof(DWORD) == sizeof(RGBQUAD)
    ulColorTableSize = 3;
    }
    else
    {
    // compute number of RGBQUADs from biBitCount
    ulColorTableSize = (pbmi->bmiHeader.biBitCount == 24) ? 0 :
               (1<<pbmi->bmiHeader.biBitCount);
    }

    return (ulColorTableSize * sizeof(RGBQUAD));
}
#endif


//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_to_xmit
//
//  Synopsis:   Converts a bitmap handle into a global handle
//              that can be remoted
//
//  Arguments:  [pBitmap]       -- pointer to the original bitmap handle
//              [ppxmit]        -- set to point to the transmitted value
//
//  Returns:    void
//
//  Algorithm:  calls a private gdi api to convert the handle
//
//  History:    16-Nov-93 alexgo    created
//              07-Jan-94 rickhi    copy the bitmap
//              12-Aug-94 davepl    Rewrote OLD_GDI_MARSHALLING section
//
//  Notes:      CODEWORK: this code specifically does not account for OS2
//              style DIBs.  Verify this is OK.  Unless the Windows APIs
//              deal with OS2 bitmaps on their own...
//
//--------------------------------------------------------------------------

void __RPC_USER HBITMAP_to_xmit (HBITMAP __RPC_FAR *pBitmap,
    RemHBITMAP __RPC_FAR * __RPC_FAR *ppxmit)
{
#ifdef  NEW_GDI_MARSHALLLING
    //  CODEWORK: we can use this in the Daytona and Local Cairo case,
    //            but not in Chicago or remote Cairo

    //calculate size
    DWORD cbData = sizeof(long);

    //allocate memory
    *ppxmit = (RemHBITMAP *) NdrOleAllocate(sizeof(RemHBITMAP) + cbData);

    if (*ppxmit)
    {
    //copy data
    (*ppxmit)->cbData = cbData;
    long lData = (long)GdiConvertBitmap(*pBitmap);
    memcpy((*ppxmit)->data, &lData, cbData);
    }
    else
    {
    RpcRaiseException(E_OUTOFMEMORY);
    }

#else

    BITMAP bm;
    HBITMAP hBitmap = (HBITMAP) * pBitmap;

    // Get information about the bitmap

#if defined(_CHICAGO_)
    if (FALSE == GetObjectA(hBitmap, sizeof(BITMAP), &bm))
#else
    if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
#endif
    {
        RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Allocate space for the raw bitmap bits and the bm structure, plus
    // the RemHBITMAP structure all at once.

    DWORD dwCount = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;
    *ppxmit = (RemHBITMAP *) (BYTE *) NdrOleAllocate(sizeof(RemHBITMAP)
                                                     + dwCount + sizeof(bm));

    if (NULL == *ppxmit)
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }

    // lpBits points to the portion of the RemHBITMAP structure where
    // we will store the BITMAP structure and raw bits

    BYTE * lpBits = (BYTE *) &(*ppxmit)->data[0];

    // Get the raw bits.  Offset sizeof(BITMAP) into the buffer so
    // that we can stick the BITMAP struct at the front before
    // transmission

    if (0 == GetBitmapBits(hBitmap, dwCount, lpBits + sizeof(bm)))
    {
        NdrOleFree(*ppxmit);
        *ppxmit = NULL;
        RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Stuff the bm structure in before the bits

    memcpy(lpBits, (void *) &bm, sizeof(bm));

    (*ppxmit)->cbData = dwCount + sizeof(bm);

#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_from_xmit
//
//  Synopsis:   Converts a global bitmap handle into a bitmap
//              handle that a process can use
//
//  Arguments:  [pxmit]         -- the transmitted global handle
//              [pBitmap]       -- where to put the local bitmap handle
//
//  Returns:    void
//
//  Algorithm:  Creates a local bitmap and then associates the remote
//              bitmap with the local one.
//
//  History:    16-Nov-93 alexgo    created
//              07-Jan-94 rickhi    copy the bitmap
//              12-Aug-94 davepl    rewrote the OLD_GDI_MARSHALLING section
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBITMAP_from_xmit( RemHBITMAP __RPC_FAR *pxmit,
    HBITMAP __RPC_FAR *pBitmap )
{
#ifdef  NEW_GDI_MARSHALLLING
    //  CODEWORK: we can use this in the Daytona and Local Cairo case,
    //            but not in Chicago or remote Cairo

    ULONG hLocal = (ULONG)GdiCreateLocalBitmap();

    ULONG lh;
    memcpy(&lh, pxmit->data, pxmit->cbData);

    GdiAssociateObject(hLocal, lh);
    *pBitmap = (HBITMAP)hLocal;

#else

    BITMAP * pbm    = (BITMAP *) &(pxmit->data[0]);
    BYTE   * lpBits = ((BYTE *) pbm) + sizeof(BITMAP);

    // Create a bitmap based on the BITMAP structure and the raw bits in
    // the transmission buffer

    *pBitmap = CreateBitmap(pbm->bmWidth,
                            pbm->bmHeight,
                            pbm->bmPlanes,
                            pbm->bmBitsPixel,
                            (void *) lpBits);

    // If no bitmap came back, raise an exception rather than just returning

    if (NULL == *pBitmap)
    {
        RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
    }


#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_free_xmit
//
//  Synopsis:   Free's the buffer used to serialize the bitmap
//
//  Effects:
//
//  Arguments:  [pxmit]         -- the transmitted bitmap serialized buffer
//
//  Returns:    void
//
//  History:    dd-mmm-yy Author    Comment
//              16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBITMAP_free_xmit( RemHBITMAP __RPC_FAR *pxmit)
{
    if(pxmit != 0)
        NdrOleFree(pxmit);
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_free_inst
//
//  Synopsis:   Destroys the bitmap object
//
//  Arguments:  [pBitmap]       -- pointer to the bitmap handle
//
//  Returns:    void
//
//  History:    dd-mmm-yy Author    Comment
//              16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBITMAP_free_inst( HBITMAP __RPC_FAR *pBitmap)
{
    DeleteObject(*pBitmap);
}


//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_to_xmit
//
//  Synopsis:   Converts a brush handle into a global handle
//              that can be remoted
//
//  Arguments:  [pBrush]        -- pointer to the original brush handle
//              [ppxmit]        -- set to point to the transmitted value
//
//  Returns:    void
//
//  Algorithm:  calls a private gdi api to convert the handle
//
//  History:    dd-mmm-yy Author    Comment
//              16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBRUSH_to_xmit (HBRUSH __RPC_FAR *pBrush,
    RemHBRUSH __RPC_FAR * __RPC_FAR *ppxmit)
{
#ifndef _CHICAGO_
    //  CODEWORK: we can use this in the Daytona and Local Cairo case,
    //            but not in Chicago or remote Cairo

    //calculate size
    DWORD cbData = sizeof(long);

    //allocate memory
    *ppxmit = (RemHBRUSH *) NdrOleAllocate(sizeof(RemHBRUSH) + cbData);

    if (*ppxmit)
    {
    //copy data
    (*ppxmit)->cbData = cbData;

    // BUGBUG: enable this code!
    //long lData = (long)OleGdiConvertBrush(*pBrush);
    //memcpy((*ppxmit)->data, &lData, cbData);
    }
    else
    {
    RpcRaiseException(E_OUTOFMEMORY);
    }
#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_from_xmit
//
//  Synopsis:   Converts a global brush handle into a brush
//              handle that a process can use
//
//  Arguments:  [pxmit]         -- the transmitted global handle
//              [pBrush]        -- where to put the local brush handle
//
//  Returns:    void
//
//  Algorithm:  calls a private gdi api to convert the global handle
//
//  History:    dd-mmm-yy Author    Comment
//              16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBRUSH_from_xmit( RemHBRUSH __RPC_FAR *pxmit,
    HBRUSH __RPC_FAR *phBrush )
{
#ifndef _CHICAGO_
    //  CODEWORK: we can use this in the Daytona and Local Cairo case,
    //            but not in Chicago or remote Cairo

    // BUGBUG: enable this code!
    //*phBrush = OleGdiCreateLocalBrush((HBRUSH)(pxmit->data));
    *phBrush = NULL;

#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_free_xmit
//
//  Synopsis:   Free's the global brush handle that gets remoted
//
//  Arguments:  [pxmit]         -- the transmitted brush handle
//
//  Returns:    void
//
//  History:    dd-mmm-yy Author    Comment
//              16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBRUSH_free_xmit( RemHBRUSH __RPC_FAR *pxmit)
{
#ifndef _CHICAGO_
    if(pxmit != 0)
        NdrOleFree(pxmit);
#endif
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_free_inst
//
//  Synopsis:   Delete an HBRUSH.
//
//  Arguments:  [pBrush]        -- pointer to the metafile
//
//  Returns:    void
//
//  History:    dd-mmm-yy Author    Comment
//              16-Nov-93 alexgo    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER HBRUSH_free_inst( HBRUSH __RPC_FAR *pBrush)
{
#ifndef _CHICAGO_
    DeleteObject(*pBrush);
#endif
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CStreamOnMessage::AddRef( THIS )
{
  return ref_count += 1;
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Clone(THIS_ IStream * *ppstm)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Commit(THIS_ DWORD grfCommitFlags)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::CopyTo(THIS_ IStream *pstm,
                  ULARGE_INTEGER cb,
                  ULARGE_INTEGER *pcbRead,
                  ULARGE_INTEGER *pcbWritten)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
CStreamOnMessage::CStreamOnMessage(unsigned char **ppMessageBuffer)
    : ref_count(1), ppBuffer(ppMessageBuffer), cbMaxStreamLength(0xFFFFFFFF)
{
    pStartOfStream = *ppMessageBuffer;
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::LockRegion(THIS_ ULARGE_INTEGER libOffset,
                  ULARGE_INTEGER cb,
                  DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown))
  {
    *ppvObj = (IUnknown *) this;
    ref_count += 1;
    return ResultFromScode(S_OK);
  }
  else if (IsEqualIID(riid, IID_IStream))
  {
    *ppvObj = (IStream *) this;
    ref_count += 1;
    return ResultFromScode(S_OK);
  }
  else
    return ResultFromScode(E_NOINTERFACE);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Read(THIS_ VOID HUGEP *pv,
                  ULONG cb, ULONG *pcbRead)
{
  memcpy( pv, *ppBuffer, cb );
  *ppBuffer += cb;
  if (pcbRead != NULL)
    *pcbRead = cb;
  return ResultFromScode(S_OK);
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CStreamOnMessage::Release( THIS )
{
  ref_count -= 1;
  if (ref_count == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;

}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Revert(THIS)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Seek(THIS_ LARGE_INTEGER dlibMove,
                  DWORD dwOrigin,
                  ULARGE_INTEGER *plibNewPosition)
{
  ULONG   pos;

  // Verify that the offset isn't out of range.
  if (dlibMove.HighPart != 0)
    return ResultFromScode( E_FAIL );

  // Determine the new seek pointer.
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      pos = dlibMove.LowPart;
      break;

    case STREAM_SEEK_CUR:
      /* Must use signed math here. */
      pos = (ULONG) (*ppBuffer - pStartOfStream);
      if ((long) dlibMove.LowPart < 0 &&
      pos < (unsigned long) - (long) dlibMove.LowPart)
    return ResultFromScode( E_FAIL );
      pos += (long) dlibMove.LowPart;
      break;

    case STREAM_SEEK_END:
        return ResultFromScode(E_NOTIMPL);
    break;

    default:
      return ResultFromScode( E_FAIL );
  }

  // Set the seek pointer.
  *ppBuffer = pStartOfStream + pos;
  if (plibNewPosition != NULL)
  {
    plibNewPosition->LowPart = pos;
    plibNewPosition->HighPart = 0;
  }
  return ResultFromScode(S_OK);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::SetSize(THIS_ ULARGE_INTEGER libNewSize)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Stat(THIS_ STATSTG *pstatstg, DWORD grfStatFlag)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
                  ULARGE_INTEGER cb,
                  DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Write(THIS_ VOID const HUGEP *pv,
                  ULONG cb,
                  ULONG *pcbWritten)
{
  // Write the data.
  memcpy( *ppBuffer, pv, cb );
  if (pcbWritten != NULL)
    *pcbWritten = cb;
  *ppBuffer += cb;
  return ResultFromScode(S_OK);
}


//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_to_xmit
//
//  Synopsis:   Convert an STGMEDIUM to a RemSTGMEDIUM structure so it can be sent
//                              over the network.
//
//                              The marshalled STGMEDIUM looks like this:
//                              RemSTGMEDIUM | data from union | data from pUnkForRelease
//--------------------------------------------------------------------------

void __RPC_USER STGMEDIUM_to_xmit (STGMEDIUM *pinst, RemSTGMEDIUM **ppxmit)
{
    unsigned char *pData;
    RemHGLOBAL *pRemHGLOBAL;
    RemHBITMAP *pRemHBITMAP;
    RemHPALETTE *pRemHPALETTE;
    RemHMETAFILEPICT *pRemHMETAFILEPICT;
    RemHENHMETAFILE *pRemHENHMETAFILE;

    long size;
    unsigned long count;
    unsigned long cbInterface;
    HRESULT hr = S_OK;
    DWORD *pcbData;
    DWORD *pcbSize;
    DWORD cbData;
    unsigned char *pStart;

    // If the TYMED for the STGMEDIUM is TYMED_GDI, we need a bit more information
    // (ie: what _kind_ of GDI object it is).  The field is unused for anything
    // except TYMED_GDI

    DWORD dwHandleType = 0;

    //calculate size of marshalled STGMEDIUM.
    size = sizeof(RemSTGMEDIUM);

    //add the size of data[].
    switch(pinst->tymed)
    {
    case TYMED_NULL:
        break;
    case TYMED_MFPICT:
        HMETAFILEPICT_to_xmit(&pinst->hMetaFilePict, &pRemHMETAFILEPICT);
        size += sizeof(RemHMETAFILEPICT) + pRemHMETAFILEPICT->cbData;
        break;
    case TYMED_ENHMF:
        HENHMETAFILE_to_xmit(&pinst->hEnhMetaFile, &pRemHENHMETAFILE);
        size += sizeof(RemHENHMETAFILE) + pRemHENHMETAFILE->cbData;
        break;
    case TYMED_GDI:

        // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
        // those types we know about based on the object type, and reject
        // those which we do not support.

        switch(GetObjectType( (HGDIOBJ) pinst->hGlobal ))
        {
            case OBJ_BITMAP:

                HBITMAP_to_xmit(&pinst->hBitmap, &pRemHBITMAP);
                size += sizeof(RemHBITMAP) + pRemHBITMAP->cbData;
                dwHandleType = OBJ_BITMAP;
                break;

            case OBJ_PAL:

                HPALETTE_to_xmit((HPALETTE *) &pinst->hBitmap, &pRemHPALETTE);
                size += sizeof(RemHPALETTE) + pRemHPALETTE->cbData;
                dwHandleType = OBJ_PAL;
                break;

            default:
                RpcRaiseException(DV_E_TYMED);
        }
        break;

    case TYMED_HGLOBAL:
        HGLOBAL_to_xmit(&pinst->hGlobal, &pRemHGLOBAL);
        size += sizeof(RemHGLOBAL) + pRemHGLOBAL->cbData;
        break;
    case TYMED_FILE:
        count = wcslen(pinst->lpszFileName) + 1;
        size += 4; //allocate room for character count.
        size += count * sizeof(wchar_t);
        break;
    case TYMED_ISTREAM:
        size += 4;
        if(pinst->pstm)
        {
            //Align the buffer on an 4 byte boundary.
            size += 3;
            size &= (unsigned int)0xfffffffc;

            //Allocate space for the length and array bounds.
            size += 8;

            hr = CoGetMarshalSizeMax(&cbInterface, IID_IStream, pinst->pstm, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr == S_OK)
            size += cbInterface;
        }
        break;
    case TYMED_ISTORAGE:
        size += 4;
        if(pinst->pstg)
        {
            //Align the buffer on an 4 byte boundary.
            size += 3;
            size &= (unsigned int)0xfffffffc;

            //Allocate space for the length and array bounds.
            size += 8;

            hr = CoGetMarshalSizeMax(&cbInterface, IID_IStorage, pinst->pstg, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr == S_OK)
            size += cbInterface;
        }
        break;
    default:
        break;
    }


    //Allocate space for pUnkForRelease.
    if(pinst->pUnkForRelease)
    {
        //Align the buffer on an 4 byte boundary.
        size += 3;
        size &= (unsigned int)0xfffffffc;

        //Allocate space for the length and array bounds.
        size += 8;

        hr = CoGetMarshalSizeMax(&cbInterface, IID_IUnknown, pinst->pUnkForRelease, MSHCTX_NOSHAREDMEM, 0, MSHLFLAGS_NORMAL);
    if(hr == S_OK)
    {
        size += cbInterface;
    }
    }

    //allocate memory
    *ppxmit = (RemSTGMEDIUM *) NdrOleAllocate(size);

    //Marshal STGMEDIUM
    (*ppxmit)->tymed = pinst->tymed;
    //SUNDOWN: typecast problem
    (*ppxmit)->pData = PtrToUlong(pinst->hGlobal);
    (*ppxmit)->pUnkForRelease = PtrToUlong(pinst->pUnkForRelease);
    (*ppxmit)->cbData = size - sizeof(RemSTGMEDIUM);
    (*ppxmit)->dwHandleType = dwHandleType;

    pData = (*ppxmit)->data;

    switch(pinst->tymed)
    {
    case TYMED_NULL:
        break;
    case TYMED_MFPICT:
        //Note that we called HMETAFILEPICT_to_xmit earlier so we could
        //get the size.
        memcpy(pData, pRemHMETAFILEPICT, sizeof(RemHMETAFILEPICT) + pRemHMETAFILEPICT->cbData);
        pData += sizeof(RemHMETAFILEPICT) + pRemHMETAFILEPICT->cbData;
        HMETAFILEPICT_free_xmit(pRemHMETAFILEPICT);
        break;
    case TYMED_ENHMF:
        memcpy(pData, pRemHENHMETAFILE, sizeof(RemHENHMETAFILE) + pRemHENHMETAFILE->cbData);
        pData += sizeof(RemHENHMETAFILE) + pRemHENHMETAFILE->cbData;
        HENHMETAFILE_free_xmit(pRemHENHMETAFILE);
        break;
    case TYMED_HGLOBAL:
        //Note that we called HGLOBAL_to_xmit earlier so we could
        //get the size.
        memcpy(pData, pRemHGLOBAL, sizeof(RemHGLOBAL) + pRemHGLOBAL->cbData);
    pData += sizeof(RemHGLOBAL) + pRemHGLOBAL->cbData;
        HGLOBAL_free_xmit(pRemHGLOBAL);
        break;

    case TYMED_GDI:

        switch(dwHandleType)
        {
        case OBJ_BITMAP:

            memcpy(pData, pRemHBITMAP, sizeof(RemHBITMAP) + pRemHBITMAP->cbData);
            pData += sizeof(RemHBITMAP) + pRemHBITMAP->cbData;
            HBITMAP_free_xmit(pRemHBITMAP);
            break;

        case OBJ_PAL:

            memcpy(pData, pRemHPALETTE, sizeof(RemHPALETTE) + pRemHPALETTE->cbData);
            pData += sizeof(RemHPALETTE) + pRemHPALETTE->cbData;
            HPALETTE_free_xmit(pRemHPALETTE);
        }

        break;

    case TYMED_FILE:
        //copy the length.
        memcpy(pData, &count, sizeof(count));
        pData += sizeof(count);

        //copy the string.
    memcpy(pData, pinst->lpszFileName, count * sizeof(wchar_t));
        pData += count * sizeof(wchar_t);
        break;
    case TYMED_ISTREAM:
        if(pinst->pstm)
        {
            CStreamOnMessage stream((unsigned char **) &pData);

            //Align the buffer on an 4 byte boundary
            *(unsigned long FAR *)&pData += 3;
            *(unsigned long FAR *)&pData &= 0xfffffffc;

            //Leave space for cbData.
            pcbData = (DWORD *) pData;
            *(unsigned long FAR *)&pData += 4;

            //Leave space for size.
            pcbSize = (DWORD *) pData;
        *(unsigned long FAR *)&pData += 4;

            pStart = (unsigned char *) pData;

            hr = CoMarshalInterface(&stream, IID_IStream, pinst->pstm, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr != S_OK)
            {
                RpcRaiseException(hr);
            }

            cbData = (DWORD) (pData - pStart);
            *pcbData = cbData;
            *pcbSize = cbData;
        }
        break;
    case TYMED_ISTORAGE:
        if(pinst->pstg)
        {
            CStreamOnMessage stream((unsigned char **) &pData);

            //Align the buffer on an 4 byte boundary
            *(unsigned long FAR *)&pData += 3;
            *(unsigned long FAR *)&pData &= 0xfffffffc;

            //Leave space for cbData.
            pcbData = (DWORD *) pData;
            *(unsigned long FAR *)&pData += 4;

            //Leave space for size.
            pcbSize = (DWORD *) pData;
            *(unsigned long FAR *)&pData += 4;

            pStart = (unsigned char *) pData;

            hr = CoMarshalInterface(&stream, IID_IStorage, pinst->pstg, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr != S_OK)
            {
                RpcRaiseException(hr);
            }

            cbData = (DWORD) (pData - pStart);
            *pcbData = cbData;
            *pcbSize = cbData;
        }
        break;
    default:
        break;
    }


    if(pinst->pUnkForRelease)
    {
        CStreamOnMessage stream((unsigned char **) &pData);

        //Align the buffer on an 4 byte boundary
        *(unsigned long FAR *)&pData += 3;
        *(unsigned long FAR *)&pData &= 0xfffffffc;

        //Leave space for cbData.
        pcbData = (DWORD *) pData;
        *(unsigned long FAR *)&pData += 4;

        //Leave space for size.
        pcbSize = (DWORD *) pData;
        *(unsigned long FAR *)&pData += 4;

        pStart = (unsigned char *) pData;

        hr = CoMarshalInterface(&stream, IID_IUnknown, pinst->pUnkForRelease, MSHCTX_NOSHAREDMEM, 0, MSHLFLAGS_NORMAL);
        if(hr != S_OK)
        {
            RpcRaiseException(hr);
        }

        cbData = (DWORD) (pData - pStart);
        *pcbData = cbData;
        *pcbSize = cbData;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_from_xmit
//
//  Synopsis:   Convert a RemSTGMEDIUM structure to an STGMEDIUM.
//
//--------------------------------------------------------------------------
void __RPC_USER STGMEDIUM_from_xmit (RemSTGMEDIUM __RPC_FAR *pxmit, STGMEDIUM __RPC_FAR *pinst)
{
    HRESULT hr = S_OK;
    unsigned char *pData;

    pinst->tymed = pxmit->tymed;
    pData = pxmit->data;

    switch(pinst->tymed)
    {
    case TYMED_NULL:
    break;

    case TYMED_MFPICT:
    HMETAFILEPICT_from_xmit((RemHMETAFILEPICT *)pData, &pinst->hMetaFilePict);
    pData += sizeof(RemHMETAFILEPICT) + ((RemHMETAFILEPICT *)pData)->cbData;
    break;

    case TYMED_ENHMF:
    HENHMETAFILE_from_xmit((RemHENHMETAFILE *)pData, &pinst->hEnhMetaFile);
    pData += sizeof(RemHENHMETAFILE) + ((RemHENHMETAFILE *)pData)->cbData;
    break;

    case TYMED_HGLOBAL:
    HGLOBAL_from_xmit((RemHGLOBAL *)pData, &pinst->hGlobal);
    pData += sizeof(RemHGLOBAL) + ((RemHGLOBAL *)pData)->cbData;
    break;

    // When unmarshalling a STGMEDIUM with TYMED_GDI, we need to know
    // what kind of GDI object is packaged, so we inspect the dwHandleType
    // field which was set during the marshalling of the STGMEDIUM

    case TYMED_GDI:

    switch(pxmit->dwHandleType)
    {
        case OBJ_BITMAP:

            HBITMAP_from_xmit((RemHBITMAP *)pData, &pinst->hBitmap);
            pData += sizeof(RemHBITMAP) + ((RemHBITMAP *)pData)->cbData;
            break;

        case OBJ_PAL:

            HPALETTE_from_xmit((RemHPALETTE *)pData, (HPALETTE *) &pinst->hBitmap);
            pData += sizeof(RemHPALETTE) + ((RemHPALETTE *)pData)->cbData;
            break;

        default:

            RpcRaiseException(DV_E_TYMED);
    }
    break;


    case TYMED_FILE:
    {
        unsigned long count;

        //unmarshal the count.
        memcpy(&count, pData, sizeof(count));
        pData += sizeof(count);

        //allocate memory.
        pinst->lpszFileName = (wchar_t *)NdrOleAllocate(count * sizeof(wchar_t));

        //copy the string.
        memcpy(pinst->lpszFileName, pData, count * sizeof(wchar_t));
        pData += count * sizeof(wchar_t);
    }
    break;

    case TYMED_ISTREAM:

    if (pxmit->pData)
    {
    CStreamOnMessage stream(&pData);

    //Align the buffer on an 4 byte boundary
    *(unsigned long FAR *)&pData += 3;
    *(unsigned long FAR *)&pData &= 0xfffffffc;

    //Skip over cbData.
    *(unsigned long FAR *)&pData += 4;

    //Skip over cbSize.
    *(unsigned long FAR *)&pData += 4;

    hr = CoUnmarshalInterface(&stream, IID_IStream,
                  (void **) &pinst->pstm);
    if(hr != S_OK)
    {
        RpcRaiseException(hr);
    }
    }
    else
    {
    pinst->pstm = NULL;
    }
    break;

    case TYMED_ISTORAGE:

    if (pxmit->pData)
    {
    CStreamOnMessage stream(&pData);

    //Align the buffer on an 4 byte boundary
    *(unsigned long FAR *)&pData += 3;
    *(unsigned long FAR *)&pData &= 0xfffffffc;

    //Skip over cbData.
    *(unsigned long FAR *)&pData += 4;

    //Skip over cbSize.
    *(unsigned long FAR *)&pData += 4;

    hr = CoUnmarshalInterface(&stream, IID_IStorage,
                  (void **) &pinst->pstg);
    if(hr != S_OK)
    {
        RpcRaiseException(hr);
    }
    }
    else
    {
    pinst->pstg = NULL;
    }

    break;
    default:
    break;
    }


    pinst->pUnkForRelease = NULL;

    if(pxmit->pUnkForRelease)
    {
    CStreamOnMessage stream(&pData);

    //Align the buffer on an 4 byte boundary
    *(unsigned long FAR *)&pData += 3;
    *(unsigned long FAR *)&pData &= 0xfffffffc;

    //Skip over cbData.
    *(unsigned long FAR *)&pData += 4;

    //Skip over cbSize.
    *(unsigned long FAR *)&pData += 4;

    hr = CoUnmarshalInterface(&stream, IID_IUnknown, (void **) &pinst->pUnkForRelease);
    if(hr != S_OK)
    {
        RpcRaiseException(hr);
    }

    //  replace the punkForRelease with our custom release
    //  handler for special situations.

    if (pinst->tymed == TYMED_HGLOBAL ||
        pinst->tymed == TYMED_MFPICT  ||
        pinst->tymed == TYMED_ENHMF   ||
        pinst->tymed == TYMED_GDI)
    {
        IUnknown *punkTmp = (IUnknown *) new CPunkForRelease(pinst);
        if (!punkTmp)
        {
        RpcRaiseException(E_OUTOFMEMORY);
        }
        pinst->pUnkForRelease = punkTmp;
    }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_free_inst
//
//  Synopsis:   Free the contents of an STGMEDIUM structure.
//
//--------------------------------------------------------------------------

void __RPC_USER STGMEDIUM_free_inst(STGMEDIUM *pinst)
{
    if(pinst)
    {
    if (pinst->tymed == TYMED_FILE)
    {
        NdrOleFree(pinst->lpszFileName);
        pinst->lpszFileName = NULL;

        if (pinst->pUnkForRelease)
        {
        pinst->pUnkForRelease->Release();
        pinst->pUnkForRelease = 0;
        }
    }
    else
    {
        ReleaseStgMedium(pinst);
    }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_free_xmit
//
//  Synopsis:   Free a RemSTGMEDIUM previously obtained from STGMEDIUM_to_xmit.
//
//--------------------------------------------------------------------------
void __RPC_USER STGMEDIUM_free_xmit(RemSTGMEDIUM *pxmit)
{
    if(pxmit)
    {
    NdrOleFree(pxmit);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_to_xmit
//
//  Synopsis:   Converts an SNB structure to a remotable structure
//
//  Arguments:  [pSNB]   -- pointer to the original SNB
//              [ppxmit] -- set to point to the transmitted value
//
//  Returns:    void
//
//  Algorithm:
//
//  History:    10-01-94    rickhi  Created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER SNB_to_xmit (SNB __RPC_FAR *pSNB,
    RemSNB __RPC_FAR * __RPC_FAR *ppxmit)
{
    // calculate the size of the structure needed. add 1 for the NULL
    // terminator

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSNB && *pSNB)
    {
    // compute the number of strings and the total number of
    // characters in all the strings.
    SNB snb = *pSNB;

    WCHAR *psz = *snb;
    while (psz)
    {
        ulCntChar += wcslen(psz) + 1;
        ulCntStr++;
        snb++;
        psz = *snb;
    }
    }

    // allocate memory
    RemSNB *pRemSNB = (RemSNB *) NdrOleAllocate(sizeof(RemSNB) +
                        ulCntChar * sizeof(WCHAR));

    if (pRemSNB)
    {
    // copy the data
    pRemSNB->ulCntStr  = ulCntStr;
    pRemSNB->ulCntChar = ulCntChar;

    if (pSNB && *pSNB)
    {
        // copy the string ptrs into the new structure
        SNB snb = *pSNB;

        WCHAR *pszSrc;
        WCHAR *pszTgt = pRemSNB->rgString;
        while (pszSrc = *snb++)
        {
        ULONG ulLen = wcslen(pszSrc) + 1;
        memcpy(pszTgt, pszSrc, ulLen * sizeof(WCHAR));
        pszTgt += ulLen;
        }
    }

    *ppxmit = pRemSNB;
    }
    else
    {
    RpcRaiseException(E_OUTOFMEMORY);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_from_xmit
//
//  Synopsis:   converts a RemSNB to an SNB
//
//  Arguments:  [pxmit] -- the transmitted RemSNB
//              [pSNB]  -- where to put the local SNB
//
//  Returns:    void
//
//  History:    10-Jan-94 rickhi    created
//
//  Notes:
//
//--------------------------------------------------------------------------

void __RPC_USER SNB_from_xmit( RemSNB __RPC_FAR *pxmit,
    SNB __RPC_FAR *pSNB )
{
    if (pxmit)
    {
    if (pxmit->ulCntStr == 0)
    {
        *pSNB = NULL;
        return;
    }

    SNB snb = (SNB) NdrOleAllocate((pxmit->ulCntStr+1) * sizeof(WCHAR *) +
            pxmit->ulCntChar * sizeof(WCHAR));

    //  set the out parameter
    *pSNB = snb;

    if (snb)
    {
        // create the pointer array
        WCHAR *pszSrc = pxmit->rgString;
        WCHAR *pszTgt = (WCHAR *)(snb + pxmit->ulCntStr + 1);

        for (ULONG i = pxmit->ulCntStr; i>0; i--)
        {
        *snb++ = pszTgt;

        ULONG ulLen = wcslen(pszSrc) + 1;
        pszSrc += ulLen;
        pszTgt += ulLen;
        }

        *snb++ = NULL;

        // copy the actual strings
        memcpy(snb, pxmit->rgString, pxmit->ulCntChar * sizeof(WCHAR));
    }
    else
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_free_xmit
//
//  Synopsis:   Free's the memory for the RemSNB structure
//
//  Arguments:  [pxmit]  -- the transmitted SNB to free
//
//  Returns:    void
//
//  History:    10-Jan-94 rickhi    created
//
//--------------------------------------------------------------------------

void __RPC_USER SNB_free_xmit( RemSNB __RPC_FAR *pxmit)
{
    if(pxmit != 0)
        NdrOleFree(pxmit);
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_free_inst
//
//  Synopsis:   Deletes an SNB.
//
//  Arguments:  [pSNB]  -- pointer to the SNB to free
//
//  Returns:    void
//
//  History:    10-Jan-94 created       created
//
//--------------------------------------------------------------------------

void __RPC_USER SNB_free_inst( SNB __RPC_FAR *pSNB)
{
    if (pSNB)
    {
    NdrOleFree(*pSNB);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   HMENU_to_xmit
//
//  Synopsis:   Convert an HMENU to a long.
//
//  Notes:      Both the source process and the destination process must
//              reside on the same machine.  This code assumes that the
//              destination process can use the HMENU received from
//              the source process.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HMENU_to_xmit(HMENU *pHandle, LONG_PTR **ppLong)
{
    *(ppLong) = (LONG_PTR*) NdrOleAllocate(sizeof(LONG_PTR));
    **ppLong = (LONG_PTR) *(pHandle);
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_from_xmit
//
//  Synopsis:   Convert a long to an HMENU.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HMENU_from_xmit(long *pLong, HMENU *pHandle)
{
    *(pHandle) = (HMENU) *(pLong);
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_free_inst
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HMENU_free_inst(HMENU *pHandle)
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_free_xmit
//
//  Synopsis:   Free a pointer to a long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HMENU_free_xmit(long *pLong)
{
    if(pLong != 0)
        NdrOleFree(pLong);
}

EXTERN_C void __RPC_USER HOLEMENU_to_xmit (HOLEMENU *pinst, RemHGLOBAL **ppxmit)
{
    HGLOBAL_to_xmit((HGLOBAL *)pinst, ppxmit);
}

EXTERN_C void __RPC_USER HOLEMENU_from_xmit (RemHGLOBAL __RPC_FAR *pxmit, HOLEMENU __RPC_FAR *pinst)
{
    HGLOBAL_from_xmit (pxmit, (HGLOBAL __RPC_FAR *)pinst);
}

EXTERN_C void __RPC_USER HOLEMENU_free_inst(HOLEMENU *pinst)
{
    HGLOBAL_free_inst((HGLOBAL *)pinst);
}

EXTERN_C void __RPC_USER HOLEMENU_free_xmit(RemHGLOBAL *pxmit)
{
    HGLOBAL_free_xmit(pxmit);
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_to_xmit
//
//  Synopsis:   Convert an HWND to a long.
//
//  Notes:      Both the source process and the destination process must
//              reside on the same machine.  This code assumes that the
//              destination process can use the HWND received from
//              the source process.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HWND_to_xmit(HWND *pHandle, LONG_PTR **ppLong)
{
    *(ppLong) = (LONG_PTR*) NdrOleAllocate(sizeof(LONG_PTR));
    **ppLong = (LONG_PTR) *(pHandle);
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_from_xmit
//
//  Synopsis:   Convert a long to an HWND.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HWND_from_xmit(long *pLong, HWND *pHandle)
{
    *(pHandle) = (HWND) *(pLong);
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_free_inst
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HWND_free_inst(HWND *pHandle)
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_free_xmit
//
//  Synopsis:   Free a pointer to a long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HWND_free_xmit(long *pLong)
{
    if(pLong != 0)
        NdrOleFree(pLong);
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_to_xmit
//
//  Synopsis:   Converts an enhanced metafile handle into a serial buffer
//              that can be remoted
//
//  Arguments:  [pHEnhMetafile] -- pointer to the original emf handle
//              [ppxmit]        -- set to point to the transmitted value
//
//  Returns:    void
//
//  History:    14-Nov-94   DavePl  Created
//
//
//--------------------------------------------------------------------------

void __RPC_USER HENHMETAFILE_to_xmit (HENHMETAFILE __RPC_FAR *pHEnhMetafile,
    RemHENHMETAFILE __RPC_FAR * __RPC_FAR *ppxmit)
{
    // A few inefficient temp vars here to avoid ugly casts later

    HENHMETAFILE hemf = *pHEnhMetafile;

    // Calculate the number of bytes we need in order to serialize the
    // metafile to memory

    DWORD cbData = GetEnhMetaFileBits(hemf, 0, NULL);
    if (cbData == 0)
    {
        RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Allocate the appropriate number of bytes

    *ppxmit = (RemHENHMETAFILE *)
        NdrOleAllocate(sizeof(RemHENHMETAFILE) + cbData);


    // If the allocation was successful, get the bits into our buffer.
    // Otherwise, throw an exception

    if (*ppxmit)
    {
    if (0==GetEnhMetaFileBits(hemf, cbData, &((*ppxmit)->data[0])))
        {
            NdrOleFree(*ppxmit);
            *ppxmit = NULL;

            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
        }
        (*ppxmit)->cbData = cbData;
    }
    else
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_from_xmit
//
//  Synopsis:   Converts a serialized enhanced metafile into an emf handle
//              that is usable by applications
//
//  Arguments:  [pxmit]         -- the transmitted global handle
//              [pHEnhMetafile] -- where to put the local metafilepict handle
//
//  Returns:    void
//
//  History:    14-May-94 DavePl    Created
//
//--------------------------------------------------------------------------

void __RPC_USER HENHMETAFILE_from_xmit( RemHENHMETAFILE __RPC_FAR *pxmit,
    HENHMETAFILE __RPC_FAR *pHEnhMetafile )
{

    // Generate a handle to the enhanced metafile by doing a
    // Setbits on the raw data

    *pHEnhMetafile = SetEnhMetaFileBits(pxmit->cbData, pxmit->data);

    if (NULL == *pHEnhMetafile)
    {
        RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_free_xmit
//
//  Synopsis:   Free's the remote data
//
//  Arguments:  [pxmit]         -- the transmitted data
//
//  Returns:    void
//
//  History:    14-May-94     DavePl    Created
//
//--------------------------------------------------------------------------

void __RPC_USER HENHMETAFILE_free_xmit( RemHENHMETAFILE __RPC_FAR *pxmit)
{
    if(pxmit != 0)
        NdrOleFree(pxmit);
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_free_inst
//
//  Synopsis:   destroys the metafile
//
//  Arguments:  [pHEnhMetafile]        -- handle to the enhanced metafile
//
//  Returns:    void
//
//  Algorithm:
//
//  History:    14-May-94     DavePl    Created
//
//--------------------------------------------------------------------------

void __RPC_USER HENHMETAFILE_free_inst( HENHMETAFILE __RPC_FAR *pHEnhMetafile)
{
    DeleteEnhMetaFile (*pHEnhMetafile);
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_to_xmit
//
//  Synopsis:   Converts a palette into a serialized buffer
//              that can be remoted
//
//  Arguments:  [pHPALETTE] -- pointer to the original palette handle
//              [ppxmit]    -- set to point to the transmitted value
//
//  Returns:    void
//
//  History:    11-Aug-94   DavePl  Created
//
//
//--------------------------------------------------------------------------

void __RPC_USER HPALETTE_to_xmit (HPALETTE __RPC_FAR *pHPALETTE,
    RemHPALETTE __RPC_FAR * __RPC_FAR *ppxmit)
{
    // Determine the number of color entries in the palette
    DWORD cEntries = GetPaletteEntries(*pHPALETTE, 0, 0, NULL);

    // Calculate the resultant data size
    DWORD cbData = cEntries * sizeof(PALETTEENTRY);

    // Allocate space for the struct and the entries
    *ppxmit = (RemHPALETTE *) NdrOleAllocate(sizeof(RemHPALETTE) + cbData);

    if (NULL == *ppxmit)
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }

    if (cbData)
    {
        if (0 == GetPaletteEntries(*pHPALETTE,
                                   0,
                                   cEntries,
                                   (PALETTEENTRY *) &((*ppxmit)->data[0])))
        {
            NdrOleFree(*ppxmit);
            *ppxmit = NULL;
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    (*ppxmit)->cbData = cbData;

}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_from_xmit
//
//  Synopsis:   Converts a serialized palette into an palette handle
//              that is usable by applications
//
//  Arguments:  [pxmit]         -- the transmitted buffer
//              [pHPALETTE]     -- where to put the local palette handle
//
//  Returns:    void
//
//  History:    11-Aug-94 DavePl    Created
//
//--------------------------------------------------------------------------

void __RPC_USER HPALETTE_from_xmit( RemHPALETTE __RPC_FAR *pxmit,
    HPALETTE __RPC_FAR *pHPALETTE )
{
    DWORD cEntries = pxmit->cbData / sizeof(PALETTEENTRY);
    LOGPALETTE * pLogPal;

    // If there are 0 color entries, we need to allocate the LOGPALETTE
    // structure with the one dummy entry (it's a variably sized struct).
    // Otherwise, we need to allocate enough space for the extra n-1
    // entries at the tail of the structure

    if (0 == cEntries)
    {
        pLogPal = (LOGPALETTE *) NdrOleAllocate(sizeof(LOGPALETTE));
    }
    else
    {
        pLogPal = (LOGPALETTE *) NdrOleAllocate(sizeof(LOGPALETTE) +
                                          (cEntries - 1) * sizeof(PALETTEENTRY));

        // If there are entries, and if we have a buffer, move the
        // entries into out LOGPALETTE structure

        if (pLogPal)
        {
            memcpy(&(pLogPal->palPalEntry[0]), pxmit->data, pxmit->cbData);
        }
    }

    // If we didn't get a buffer at all...

    if (NULL == pLogPal)
    {
        RpcRaiseException(E_OUTOFMEMORY);
    }

    // Fill in the rest of the structure

    pLogPal->palVersion = 0x300;
    pLogPal->palNumEntries = (unsigned short) cEntries;

    // Attempt to create the palette

    *pHPALETTE = CreatePalette(pLogPal);

    // Success or failure, we're done with the LOGPALETTE structure

    NdrOleFree(pLogPal);

    // If the creation failed, raise an exception

    if (NULL == *pHPALETTE)
    {
        RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_free_xmit
//
//  Synopsis:   Frees the remote data
//
//  Arguments:  [pxmit]         -- the transmitted data
//
//  Returns:    void
//
//  History:    11-Aug-94     DavePl    Created
//
//--------------------------------------------------------------------------

void __RPC_USER HPALETTE_free_xmit( RemHPALETTE __RPC_FAR *pxmit)
{
    if(NULL != pxmit)
    {
        NdrOleFree(pxmit);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_free_inst
//
//  Synopsis:   destroys the palette
//
//  Arguments:  [pHPALETTE]        -- handle to the palette
//
//  Returns:    void
//
//  Algorithm:
//
//  History:    11-Aug-94     DavePl    Created
//
//--------------------------------------------------------------------------

void __RPC_USER HPALETTE_free_inst( HPALETTE __RPC_FAR *pHPALETTE)
{
    DeleteObject( (HGDIOBJ) *pHPALETTE);
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_to_xmit
//
//  Synopsis:   Convert an HACCEL to a long.
//
//  Notes:      Both the source process and the destination process must
//              reside on the same machine.  This code assumes that the
//              destination process can use the HACCEL received from
//              the source process.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HACCEL_to_xmit(HACCEL *phAccel, LONG_PTR **ppLong)
{
    *ppLong = (LONG_PTR*) NdrOleAllocate(sizeof(LONG_PTR));
    **ppLong = (LONG_PTR) *phAccel;
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_from_xmit
//
//  Synopsis:   Convert a long to an HACCEL.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HACCEL_from_xmit(long *pLong, HACCEL *pHandle)
{
    *pHandle = (HACCEL) *pLong;
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_free_inst
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HACCEL_free_inst(HACCEL *pHandle)
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_free_xmit
//
//  Synopsis:   Free a pointer to a long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER HACCEL_free_xmit(long *pLong)
{
    if(pLong != 0)
        NdrOleFree(pLong);
}

//+-------------------------------------------------------------------------
//
//  Function:   UINT_to_xmit
//
//  Synopsis:   Convert a UINT to a 32 bit long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER UINT_to_xmit(UINT *pUint, unsigned long **ppLong)
{
    *ppLong = (unsigned long *) NdrOleAllocate(sizeof(long));
    **ppLong = (unsigned long) *pUint;
}

//+-------------------------------------------------------------------------
//
//  Function:   UINT_from_xmit
//
//  Synopsis:   Convert a 32 bit long to a UINT.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER UINT_from_xmit(
    unsigned long __RPC_FAR *pLong,
    UINT __RPC_FAR *pUint
    )
{
    *pUint = (UINT) *pLong;
}

//+-------------------------------------------------------------------------
//
//  Function:   UINT_free_inst
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER UINT_free_inst(UINT *pHandle)
{
}

//+-------------------------------------------------------------------------
//
//  Function:   UINT_free_xmit
//
//  Synopsis:   Free a pointer to a long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER UINT_free_xmit(unsigned long *pLong)
{
    if(pLong != 0)
        NdrOleFree(pLong);
}

//+-------------------------------------------------------------------------
//
//  Function:   WPARAM_to_xmit
//
//  Synopsis:   Convert a WPARAM to a 32 bit long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER WPARAM_to_xmit(WPARAM *pHandle, unsigned long **ppLong)
{
    *ppLong = (unsigned long *) NdrOleAllocate(sizeof(unsigned long));
    **ppLong = (unsigned long) *pHandle;
}

//+-------------------------------------------------------------------------
//
//  Function:   WPARAM_from_xmit
//
//  Synopsis:   Convert a 32 bit long to a WPARAM.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER WPARAM_from_xmit(unsigned long *pLong, WPARAM *pHandle)
{
    *pHandle = (WPARAM) *pLong;
}

//+-------------------------------------------------------------------------
//
//  Function:   WPARAM_free_inst
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER WPARAM_free_inst(WPARAM *pHandle)
{
}

//+-------------------------------------------------------------------------
//
//  Function:   WPARAM_free_xmit
//
//  Synopsis:   Free a pointer to a long.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER WPARAM_free_xmit(unsigned long *pLong)
{
    if(pLong != 0)
        NdrOleFree(pLong);
}


#ifdef UNUSED
//+-------------------------------------------------------------------------
// new code
//+-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_to_xmit
//
//  Synopsis:   Convert an STGMEDIUM to a RemBINDINFO structure so it can be sent
//                              over the network.
//
//                              The marshalled BINDINFO looks like this:
//                              RemBINDINFO | data from union | data from pUnkForRelease
//--------------------------------------------------------------------------

void __RPC_USER BINDINFO_to_xmit (BINDINFO *pinst, RemBINDINFO **ppxmit)
{
    unsigned char *pData;
    RemHGLOBAL *pRemHGLOBAL;
    RemHBITMAP *pRemHBITMAP;
    RemHPALETTE *pRemHPALETTE;
    RemHMETAFILEPICT *pRemHMETAFILEPICT;
    RemHENHMETAFILE *pRemHENHMETAFILE;

    long size;
    unsigned long count;
    unsigned long cbInterface;
    HRESULT hr = S_OK;
    DWORD *pcbData;
    DWORD *pcbSize;
    DWORD cbData;
    unsigned char *pStart;

    // If the TYMED for the BINDINFO is TYMED_GDI, we need a bit more information
    // (ie: what _kind_ of GDI object it is).  The field is unused for anything
    // except TYMED_GDI

    DWORD dwHandleType = 0;

    //calculate size of marshalled BINDINFO.
    size = sizeof(RemBINDINFO);

    //add the size of data[].
    switch(pinst->tymed)
    {
    case TYMED_NULL:
        break;
    case TYMED_MFPICT:
        HMETAFILEPICT_to_xmit(&pinst->hMetaFilePict, &pRemHMETAFILEPICT);
        size += sizeof(RemHMETAFILEPICT) + pRemHMETAFILEPICT->cbData;
        break;
    case TYMED_ENHMF:
        HENHMETAFILE_to_xmit(&pinst->hEnhMetaFile, &pRemHENHMETAFILE);
        size += sizeof(RemHENHMETAFILE) + pRemHENHMETAFILE->cbData;
        break;
    case TYMED_GDI:

        // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
        // those types we know about based on the object type, and reject
        // those which we do not support.

        switch(GetObjectType( (HGDIOBJ) pinst->hGlobal ))
        {
            case OBJ_BITMAP:

                HBITMAP_to_xmit(&pinst->hBitmap, &pRemHBITMAP);
                size += sizeof(RemHBITMAP) + pRemHBITMAP->cbData;
                dwHandleType = OBJ_BITMAP;
                break;

            case OBJ_PAL:

                HPALETTE_to_xmit((HPALETTE *) &pinst->hBitmap, &pRemHPALETTE);
                size += sizeof(RemHPALETTE) + pRemHPALETTE->cbData;
                dwHandleType = OBJ_PAL;
                break;

            default:
                RpcRaiseException(DV_E_TYMED);
        }
        break;

    case TYMED_HGLOBAL:
        HGLOBAL_to_xmit(&pinst->hGlobal, &pRemHGLOBAL);
        size += sizeof(RemHGLOBAL) + pRemHGLOBAL->cbData;
        break;
    case TYMED_FILE:
        count = wcslen(pinst->lpszFileName) + 1;
        size += 4; //allocate room for character count.
        size += count * sizeof(wchar_t);
        break;
    case TYMED_ISTREAM:
        size += 4;
        if(pinst->pstm)
        {
            //Align the buffer on an 4 byte boundary.
            size += 3;
            size &= (unsigned int)0xfffffffc;

            //Allocate space for the length and array bounds.
            size += 8;

            hr = CoGetMarshalSizeMax(&cbInterface, IID_IStream, pinst->pstm, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr == S_OK)
            size += cbInterface;
        }
        break;
    case TYMED_ISTORAGE:
        size += 4;
        if(pinst->pstg)
        {
            //Align the buffer on an 4 byte boundary.
            size += 3;
            size &= (unsigned int)0xfffffffc;

            //Allocate space for the length and array bounds.
            size += 8;

            hr = CoGetMarshalSizeMax(&cbInterface, IID_IStorage, pinst->pstg, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr == S_OK)
            size += cbInterface;
        }
        break;
    default:
        break;
    }


    //Allocate space for pUnkForRelease.
    if(pinst->pUnkForRelease)
    {
        //Align the buffer on an 4 byte boundary.
        size += 3;
        size &= (unsigned int)0xfffffffc;

        //Allocate space for the length and array bounds.
        size += 8;

        hr = CoGetMarshalSizeMax(&cbInterface, IID_IUnknown, pinst->pUnkForRelease, MSHCTX_NOSHAREDMEM, 0, MSHLFLAGS_NORMAL);
    if(hr == S_OK)
    {
        size += cbInterface;
    }
    }

    //allocate memory
    *ppxmit = (RemBINDINFO *) NdrOleAllocate(size);

    //Marshal BINDINFO
    (*ppxmit)->tymed = pinst->tymed;
    (*ppxmit)->pData = (unsigned long) pinst->hGlobal;
    (*ppxmit)->pUnkForRelease = (unsigned long) pinst->pUnkForRelease;
    (*ppxmit)->cbData = size - sizeof(RemBINDINFO);
    (*ppxmit)->dwHandleType = dwHandleType;

    pData = (*ppxmit)->data;

    switch(pinst->tymed)
    {
    case TYMED_NULL:
        break;
    case TYMED_MFPICT:
        //Note that we called HMETAFILEPICT_to_xmit earlier so we could
        //get the size.
        memcpy(pData, pRemHMETAFILEPICT, sizeof(RemHMETAFILEPICT) + pRemHMETAFILEPICT->cbData);
        pData += sizeof(RemHMETAFILEPICT) + pRemHMETAFILEPICT->cbData;
        HMETAFILEPICT_free_xmit(pRemHMETAFILEPICT);
        break;
    case TYMED_ENHMF:
        memcpy(pData, pRemHENHMETAFILE, sizeof(RemHENHMETAFILE) + pRemHENHMETAFILE->cbData);
        pData += sizeof(RemHENHMETAFILE) + pRemHENHMETAFILE->cbData;
        HENHMETAFILE_free_xmit(pRemHENHMETAFILE);
        break;
    case TYMED_HGLOBAL:
        //Note that we called HGLOBAL_to_xmit earlier so we could
        //get the size.
        memcpy(pData, pRemHGLOBAL, sizeof(RemHGLOBAL) + pRemHGLOBAL->cbData);
    pData += sizeof(RemHGLOBAL) + pRemHGLOBAL->cbData;
        HGLOBAL_free_xmit(pRemHGLOBAL);
        break;

    case TYMED_GDI:

        switch(dwHandleType)
        {
        case OBJ_BITMAP:

            memcpy(pData, pRemHBITMAP, sizeof(RemHBITMAP) + pRemHBITMAP->cbData);
            pData += sizeof(RemHBITMAP) + pRemHBITMAP->cbData;
            HBITMAP_free_xmit(pRemHBITMAP);
            break;

        case OBJ_PAL:

            memcpy(pData, pRemHPALETTE, sizeof(RemHPALETTE) + pRemHPALETTE->cbData);
            pData += sizeof(RemHPALETTE) + pRemHPALETTE->cbData;
            HPALETTE_free_xmit(pRemHPALETTE);
        }

        break;

    case TYMED_FILE:
        //copy the length.
        memcpy(pData, &count, sizeof(count));
        pData += sizeof(count);

        //copy the string.
    memcpy(pData, pinst->lpszFileName, count * sizeof(wchar_t));
        pData += count * sizeof(wchar_t);
        break;
    case TYMED_ISTREAM:
        if(pinst->pstm)
        {
            CStreamOnMessage stream((unsigned char **) &pData);

            //Align the buffer on an 4 byte boundary
            *(unsigned long FAR *)&pData += 3;
            *(unsigned long FAR *)&pData &= 0xfffffffc;

            //Leave space for cbData.
            pcbData = (DWORD *) pData;
            *(unsigned long FAR *)&pData += 4;

            //Leave space for size.
            pcbSize = (DWORD *) pData;
        *(unsigned long FAR *)&pData += 4;

            pStart = (unsigned char *) pData;

            hr = CoMarshalInterface(&stream, IID_IStream, pinst->pstm, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr != S_OK)
            {
                RpcRaiseException(hr);
            }

            cbData = (DWORD) (pData - pStart);
            *pcbData = cbData;
            *pcbSize = cbData;
        }
        break;
    case TYMED_ISTORAGE:
        if(pinst->pstg)
        {
            CStreamOnMessage stream((unsigned char **) &pData);

            //Align the buffer on an 4 byte boundary
            *(unsigned long FAR *)&pData += 3;
            *(unsigned long FAR *)&pData &= 0xfffffffc;

            //Leave space for cbData.
            pcbData = (DWORD *) pData;
            *(unsigned long FAR *)&pData += 4;

            //Leave space for size.
            pcbSize = (DWORD *) pData;
            *(unsigned long FAR *)&pData += 4;

            pStart = (unsigned char *) pData;

            hr = CoMarshalInterface(&stream, IID_IStorage, pinst->pstg, MSHCTX_LOCAL, 0, MSHLFLAGS_NORMAL);
            if(hr != S_OK)
            {
                RpcRaiseException(hr);
            }

            cbData = (DWORD) (pData - pStart);
            *pcbData = cbData;
            *pcbSize = cbData;
        }
        break;
    default:
        break;
    }


    if(pinst->pUnkForRelease)
    {
        CStreamOnMessage stream((unsigned char **) &pData);

        //Align the buffer on an 4 byte boundary
        *(unsigned long FAR *)&pData += 3;
        *(unsigned long FAR *)&pData &= 0xfffffffc;

        //Leave space for cbData.
        pcbData = (DWORD *) pData;
        *(unsigned long FAR *)&pData += 4;

        //Leave space for size.
        pcbSize = (DWORD *) pData;
        *(unsigned long FAR *)&pData += 4;

        pStart = (unsigned char *) pData;

        hr = CoMarshalInterface(&stream, IID_IUnknown, pinst->pUnkForRelease, MSHCTX_NOSHAREDMEM, 0, MSHLFLAGS_NORMAL);
        if(hr != S_OK)
        {
            RpcRaiseException(hr);
        }

        cbData = (DWORD) (pData - pStart);
        *pcbData = cbData;
        *pcbSize = cbData;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   BINDINFO_from_xmit
//
//  Synopsis:   Convert a RemBINDINFO structure to an BINDINFO.
//
//--------------------------------------------------------------------------
void __RPC_USER BINDINFO_from_xmit (RemBINDINFO __RPC_FAR *pxmit, BINDINFO __RPC_FAR *pinst)
{
    HRESULT hr = S_OK;
    unsigned char *pData;

    pinst->tymed = pxmit->tymed;
    pData = pxmit->data;

    switch(pinst->tymed)
    {
    case TYMED_NULL:
    break;

    case TYMED_MFPICT:
    HMETAFILEPICT_from_xmit((RemHMETAFILEPICT *)pData, &pinst->hMetaFilePict);
    pData += sizeof(RemHMETAFILEPICT) + ((RemHMETAFILEPICT *)pData)->cbData;
    break;

    case TYMED_ENHMF:
    HENHMETAFILE_from_xmit((RemHENHMETAFILE *)pData, &pinst->hEnhMetaFile);
    pData += sizeof(RemHENHMETAFILE) + ((RemHENHMETAFILE *)pData)->cbData;
    break;

    case TYMED_HGLOBAL:
    HGLOBAL_from_xmit((RemHGLOBAL *)pData, &pinst->hGlobal);
    pData += sizeof(RemHGLOBAL) + ((RemHGLOBAL *)pData)->cbData;
    break;

    // When unmarshalling a BINDINFO with TYMED_GDI, we need to know
    // what kind of GDI object is packaged, so we inspect the dwHandleType
    // field which was set during the marshalling of the BINDINFO

    case TYMED_GDI:

    switch(pxmit->dwHandleType)
    {
        case OBJ_BITMAP:

            HBITMAP_from_xmit((RemHBITMAP *)pData, &pinst->hBitmap);
            pData += sizeof(RemHBITMAP) + ((RemHBITMAP *)pData)->cbData;
            break;

        case OBJ_PAL:

            HPALETTE_from_xmit((RemHPALETTE *)pData, (HPALETTE *) &pinst->hBitmap);
            pData += sizeof(RemHPALETTE) + ((RemHPALETTE *)pData)->cbData;
            break;

        default:

            RpcRaiseException(DV_E_TYMED);
    }
    break;


    case TYMED_FILE:
    {
        unsigned long count;

        //unmarshal the count.
        memcpy(&count, pData, sizeof(count));
        pData += sizeof(count);

        //allocate memory.
        pinst->lpszFileName = (wchar_t *)NdrOleAllocate(count * sizeof(wchar_t));

        //copy the string.
        memcpy(pinst->lpszFileName, pData, count * sizeof(wchar_t));
        pData += count * sizeof(wchar_t);
    }
    break;

    case TYMED_ISTREAM:

    if (pxmit->pData)
    {
    CStreamOnMessage stream(&pData);

    //Align the buffer on an 4 byte boundary
    *(unsigned long FAR *)&pData += 3;
    *(unsigned long FAR *)&pData &= 0xfffffffc;

    //Skip over cbData.
    *(unsigned long FAR *)&pData += 4;

    //Skip over cbSize.
    *(unsigned long FAR *)&pData += 4;

    hr = CoUnmarshalInterface(&stream, IID_IStream,
                  (void **) &pinst->pstm);
    if(hr != S_OK)
    {
        RpcRaiseException(hr);
    }
    }
    else
    {
    pinst->pstm = NULL;
    }
    break;

    case TYMED_ISTORAGE:

    if (pxmit->pData)
    {
    CStreamOnMessage stream(&pData);

    //Align the buffer on an 4 byte boundary
    *(unsigned long FAR *)&pData += 3;
    *(unsigned long FAR *)&pData &= 0xfffffffc;

    //Skip over cbData.
    *(unsigned long FAR *)&pData += 4;

    //Skip over cbSize.
    *(unsigned long FAR *)&pData += 4;

    hr = CoUnmarshalInterface(&stream, IID_IStorage,
                  (void **) &pinst->pstg);
    if(hr != S_OK)
    {
        RpcRaiseException(hr);
    }
    }
    else
    {
    pinst->pstg = NULL;
    }

    break;
    default:
    break;
    }


    pinst->pUnkForRelease = NULL;

    if(pxmit->pUnkForRelease)
    {
    CStreamOnMessage stream(&pData);

    //Align the buffer on an 4 byte boundary
    *(unsigned long FAR *)&pData += 3;
    *(unsigned long FAR *)&pData &= 0xfffffffc;

    //Skip over cbData.
    *(unsigned long FAR *)&pData += 4;

    //Skip over cbSize.
    *(unsigned long FAR *)&pData += 4;

    hr = CoUnmarshalInterface(&stream, IID_IUnknown, (void **) &pinst->pUnkForRelease);
    if(hr != S_OK)
    {
        RpcRaiseException(hr);
    }

    //  replace the punkForRelease with our custom release
    //  handler for special situations.

    if (pinst->tymed == TYMED_HGLOBAL ||
        pinst->tymed == TYMED_MFPICT  ||
        pinst->tymed == TYMED_ENHMF   ||
        pinst->tymed == TYMED_GDI)
    {
        IUnknown *punkTmp = (IUnknown *) new CPunkForRelease(pinst);
        if (!punkTmp)
        {
        RpcRaiseException(E_OUTOFMEMORY);
        }
        pinst->pUnkForRelease = punkTmp;
    }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   BINDINFO_free_inst
//
//  Synopsis:   Free the contents of an BINDINFO structure.
//
//--------------------------------------------------------------------------

void __RPC_USER BINDINFO_free_inst(BINDINFO *pinst)
{
    if(pinst)
    {
        NdrOleFree(pinst);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   BINDINFO_free_xmit
//
//  Synopsis:   Free a RemBINDINFO previously obtained from BINDINFO_to_xmit.
//
//--------------------------------------------------------------------------
void __RPC_USER BINDINFO_free_xmit(RemBINDINFO *pxmit)
{
    if(pxmit)
    {
        NdrOleFree(pxmit);
    }
}
#endif //UNUSED





