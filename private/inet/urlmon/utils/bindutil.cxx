
// Microsoft Corporation 1997
//
// This file contains methods for copying and releasing STGMEDIUMs and BINDINFOs
// (which contains a STGMEDIUM).
//
// Created: 8-15-97 t-gpease 
//

#include <urlmon.hxx>

#undef  URLMONOFFSETOF
#define URLMONOFFSETOF(t,f)   ((DWORD_PTR)(&((t*)0)->f))

//
// This function clears the BINDINFO structure. 
//
//- The "cbSize" will not be clear and will be the same as it 
// was pasted in. This is by design so that you can reuse the 
// BINDINFO (eg copy another BINDINFO into it). 
//
//- It does NOT release the memory that the BINDINFO oocupies 
// (this could be on the stack). 
//
//- It does properly release items that the BINDINFO might be 
// pointing to.
//
STDAPI_(void)
ReleaseBindInfo(BINDINFO * pbindinfo)
{
    if ( !pbindinfo)
        return;

    DWORD cb = pbindinfo->cbSize;

    UrlMkAssert( cb && 
        "CopyBindInfo(): cbSize of the BINDINFO is ZERO. We need the cbSize to be set before calling us.");
    if(!cb )
        return; // we don't know which structure it is, so don't touch it.

    if (cb >= URLMONOFFSETOF(BINDINFO, dwReserved) && pbindinfo->pUnk)
    {
        pbindinfo->pUnk->Release();
    }

    // Orginal BINDINFO no need to check size
    if (pbindinfo->szExtraInfo)
    {
        delete [] pbindinfo->szExtraInfo;
    }

    // Orginal BINDINFO no need to check size
    if (pbindinfo->szCustomVerb)
    {
        delete [] pbindinfo->szCustomVerb;
    }

    // Orginal BINDINFO no need to check size
    ReleaseStgMedium(&pbindinfo->stgmedData);

    // set this to zero so other function will not try to use the BINDINFO
    ZeroMemory(pbindinfo, cb);
    pbindinfo->cbSize = cb;  // but keep this intact

    return;
}

//
// This function copies STGMEDIUM stuctures.
//
//- Users need to allocate the memory that the STGMEDIUM Dest will be copied
// into.
//
//- If anything goes wrong, we return the proper HRESULT and STGMEDIUM.will
// have been zero-ed.
//
STDAPI
CopyStgMedium(const STGMEDIUM * pcstgmedSrc,
                 STGMEDIUM * pstgmedDest)
{
    HRESULT         hr = S_OK;

    if ( !pcstgmedSrc || !pstgmedDest )
        return E_POINTER;

    ZeroMemory(pstgmedDest, sizeof(*pstgmedDest));

    switch (pcstgmedSrc->tymed)
    {

    case TYMED_HGLOBAL:
        {
            void *          pvDest;
            const void *    pcvSrc;
            DWORD_PTR       dwcbLen;
            HGLOBAL         hGlobalDest;

            if (!pcstgmedSrc->hGlobal)
                break;  // nothing to do

            hr = E_OUTOFMEMORY;
            pcvSrc = GlobalLock(pcstgmedSrc->hGlobal);
            if (!pcvSrc)
                goto Cleanup;

            dwcbLen = GlobalSize(pcstgmedSrc->hGlobal);
            // We can't do the following line:
            // hGlobalDest = GlobalAlloc((GMEM_MOVEABLE | GMEM_SHARE), dwcbLen);
            // because we hand out the bindinfo.stgmedData.hglobal to callers
            // that do NOT lock it and use the handle instead of the pointer.
            hGlobalDest = GlobalAlloc(GMEM_FIXED | GMEM_SHARE, dwcbLen);
            if (!hGlobalDest)
            {
                GlobalUnlock(pcstgmedSrc->hGlobal);
                goto Cleanup;
            }


            pvDest = GlobalLock(hGlobalDest);
            if (!pvDest)
            {
                GlobalFree(hGlobalDest);
                GlobalUnlock(pcstgmedSrc->hGlobal);
                goto Cleanup;
            }

            UrlMkAssert(dwcbLen>>32 == 0);
            memcpy(pvDest, pcvSrc, (unsigned long)dwcbLen);

            pstgmedDest->hGlobal = hGlobalDest;

            GlobalUnlock(hGlobalDest);
            GlobalUnlock(pcstgmedSrc->hGlobal);
            hr = S_OK;
        }
        break;

    case TYMED_FILE:
        {
            if (!pcstgmedSrc->lpszFileName)
                break; // nothing to do

            hr = E_OUTOFMEMORY;

            LPWSTR lpwstr = OLESTRDuplicate(pcstgmedSrc->lpszFileName);
            if (!lpwstr)
                goto Cleanup;

            pstgmedDest->lpszFileName = lpwstr;

            hr = S_OK;
        }
        break;

    case TYMED_ISTREAM:
        {
            pstgmedDest->pstm = pcstgmedSrc->pstm;
            if ( pstgmedDest->pstm )
                pstgmedDest->pstm->AddRef();
        }
        break;

    case TYMED_ISTORAGE:
        {
            pstgmedDest->pstg = pcstgmedSrc->pstg;
            if ( pstgmedDest->pstg )
                pstgmedDest->pstg->AddRef();
        }
        break;

    default:
        UrlMkAssert( !"CloneStgMedium has encountered a TYMED it doesn't know how to copy." );
        // fall thru and copy it.

    case TYMED_NULL: // blindly copy
    case TYMED_GDI:  // Just copy...
        memcpy(pstgmedDest, pcstgmedSrc, sizeof(*pstgmedDest));
        break;
    }

    // Common things that can be copied if we get to this point.
    pstgmedDest->tymed = pcstgmedSrc->tymed;
    pstgmedDest->pUnkForRelease = pcstgmedSrc->pUnkForRelease;
    if (pstgmedDest->pUnkForRelease)
        (pstgmedDest->pUnkForRelease)->AddRef();

Cleanup:
    return hr;
}

//
// This function copies BINDINFO structures.
//
// NOTE: IE4 added properties to the BINDINFO structure. This function
//       works with both versions. If the structure is extended in the
//       future, it will blindly copy the additional properties in the 
//       structure.
//
//- Users need to allocate the memory that the BINDINFO Dest will be copied
//  into.
//
//- Users also need to set the "cbSize" of the BINDINFO Dest to 
//  the sizeof(BINDINFO) before calling (see NOTE above).
//
//- If there is a "cbSize" conflict between the Src and the Dest, we will:
//  1) Src->cbSize = Dest->cbSize, normal copy.
//  2) Src->cbSize > Dest->cbSize, copy as much info about the Src into
//     the Dest as will fit.
//  3) Src->cbSize < Dest->cbSize, copy the Src into the Dest, Zero the
//     remaining unfilled portion of the Dest.
//
//- We don't not copy the securityattribute. We clear this value of the
//  BINDINFO Dest.
//
//- If anything goes wrong, we return the proper HRESULT and clear the
//  the BINDINFO structure using ReleaseBindInfo() (below).
//  NOTE: If "cbSize" of the Dest is ZERO, we don't do anything.
//
STDAPI
CopyBindInfo( const BINDINFO * pcbiSrc,
                BINDINFO *pbiDest )
{
/****** 8-15-97 t-gpease
Current structure at the time of writing... from URLMON.H
typedef struct tagBINDINFO {
    ULONG cbSize;
    LPWSTR szExtraInfo;
    STGMEDIUM stgmedData;
    DWORD grfBindInfoF;
    DWORD dwBindVerb;
    LPWSTR szCustomVerb;
    DWORD cbstgmedData;
// new part below this line //
    DWORD dwOptions;
    DWORD dwOptionsFlags;
    DWORD dwCodePage;
    SECURITY_ATTRIBUTES securityAttributes;
    IID iid;
    IUnknown __RPC_FAR *pUnk;
    DWORD dwReserved;
} BINDINFO;

Anything bigger than this we will blindly copy.the size of cbSize
*******/

    // NOTE: hr will an error until just before the Cleanup label.
    HRESULT hr    = E_INVALIDARG;  
    DWORD   cbSrc;
    DWORD   cbDst;

    if (!pcbiSrc || !pbiDest)
        return E_POINTER;

    cbSrc = pcbiSrc->cbSize;
    cbDst = pbiDest->cbSize;
    
    UrlMkAssert( cbSrc &&
        "CopyBindInfo(): cbSize of the source is ZERO. You must set the cbSize.");
    UrlMkAssert( cbDst &&
        "CopyBindInfo(): cbSize of the destination is ZERO. It needs to be set to the size of the BINDINFO before calling us.");
    if (!cbSrc || !cbDst)
        goto Abort;   // nothing to do or can do.

    // Copy those bytes in common, zero the rest if any
    memcpy(pbiDest, pcbiSrc, min(cbSrc, cbDst));
    pbiDest->cbSize = cbDst; // always keep this intact

    if (cbDst > cbSrc)
    {
        ZeroMemory((BYTE *)pbiDest + cbSrc, cbDst - cbSrc);
    }

    if (cbDst >= URLMONOFFSETOF(BINDINFO, dwReserved))
    {
        ZeroMemory(&pbiDest->securityAttributes, sizeof(SECURITY_ATTRIBUTES));
    }

    if (pcbiSrc->pUnk && cbDst >= URLMONOFFSETOF(BINDINFO, dwReserved))
    {
        pbiDest->pUnk->AddRef();
    }

    // NULL these anything fails we don't want to free the Sources resources.
    pbiDest->szExtraInfo  = NULL;
    pbiDest->szCustomVerb = NULL;    
    
    // Original BINDINFO no need to check size
    hr = CopyStgMedium( &pcbiSrc->stgmedData, &pbiDest->stgmedData );
    if (hr)
        goto Cleanup;

    // Original BINDINFO no need to check size
    if (pcbiSrc->szExtraInfo)
    {
        LPWSTR lpwstr = OLESTRDuplicate(pcbiSrc->szExtraInfo);
        if (!lpwstr)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pbiDest->szExtraInfo= lpwstr;
    }

    // Original BINDINFO no need to check size
    if (pcbiSrc->szCustomVerb)
    {
        LPWSTR lpwstr = OLESTRDuplicate(pcbiSrc->szCustomVerb);
        if (!lpwstr)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        pbiDest->szCustomVerb= lpwstr;
    }

Cleanup:
    if (hr)
    {
        // This will set pbiDest members to zero so other function will not 
        // try to use the new BINDINFO.
        ReleaseBindInfo( pbiDest );
    }

Abort:
    return(hr);
}

