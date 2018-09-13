/*
 *  @doc    INTERNAL
 *
 *  @module FONTINFO.H -- Font information
 *  
 *  Purpose:
 *      Font info, used with caching and fontlinking
 *  
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      6/25/98     cthrash     Created
 *
 *  Copyright (c) 1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X_FONTINFO_HXX_
#define X_FONTINFO_HXX_
#include "fontinfo.hxx"
#endif

MtDefine(CFontInfoCache, Utilities, "CFontInfoCache")
MtDefine(CFontInfoCache_pv, CFontInfoCache, "CFontInfoCache::_pv")

// NB (cthrash) Face names are case-sensitive under Unix.

#ifdef UNIX
#define STRCMPFUNC StrCmpC
#else
#define STRCMPFUNC StrCmpIC
#endif

HRESULT
CFontInfoCache::AddInfoToAtomTable( LPCTSTR pchFaceName, LONG *plIndex )
{
    HRESULT hr = S_OK;
    LONG lIndex;
    CFontInfo * pfi;
    
    for (lIndex = 0; lIndex < Size(); lIndex++)
    {
        pfi = (CFontInfo *)Deref(sizeof(CFontInfo), lIndex);
        if (!STRCMPFUNC(pchFaceName, pfi->_cstrFaceName))
            break;
    }
    if (lIndex == Size())
    {
        CFontInfo fi;
        
        //
        // Not found, so add element to array.
        //

        hr = THR(AppendIndirect(&fi));
        if (hr)
            goto Cleanup;

        lIndex = Size() - 1;
        pfi = (CFontInfo *)Deref(sizeof(CFontInfo), lIndex);
        pfi->_sids = sidsNotSet;
        hr = THR(pfi->_cstrFaceName.Set(pchFaceName));
        if (hr)
            goto Cleanup;
    }

    if (plIndex)
    {
        *plIndex = lIndex;
    }

Cleanup:
    RRETURN(hr);

}


HRESULT
CFontInfoCache::GetAtomFromName( LPCTSTR pch, LONG *plIndex )
{
    LONG        lIndex;
    HRESULT     hr = S_OK;
    CFontInfo * pfi;

    for (lIndex = 0; lIndex < Size(); lIndex++)
    {
        pfi = (CFontInfo *)Deref(sizeof(CFontInfo), lIndex);

        if(!STRCMPFUNC(pfi->_cstrFaceName, pch))
            break;
    }
    
    if (lIndex == Size())
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    if (plIndex)
    {
        *plIndex = lIndex;
    }

Cleanup:    
    RRETURN(hr);
}

HRESULT 
CFontInfoCache::GetInfoFromAtom(LONG lIndex, CFontInfo **ppfi)
{
    HRESULT hr = S_OK;
    CFontInfo * pfi;
    
    if (Size() <= lIndex)
    {
        hr = DISP_E_MEMBERNOTFOUND;
        goto Cleanup;
    }

    pfi = (CFontInfo *)Deref(sizeof(CFontInfo), lIndex);
    *ppfi = pfi;
    
Cleanup:    
    RRETURN1(hr, DISP_E_MEMBERNOTFOUND);
}


void
CFontInfoCache::Free()
{
    CFontInfo *  pCFontInfo;
    LONG i;
    
    for (i=0; i<Size(); i++)
    {
        pCFontInfo = (CFontInfo *)Deref(sizeof(CFontInfo), i);
        pCFontInfo->_cstrFaceName.Free();
    }
    DeleteAll();
}


#if DBG==1
void
CFontInfoCache::Dump()
{
    CFontInfo *  pFontInfo;
    LONG i;

    OutputDebugStringA("*** FontInfoCache dump ***\r\n");

    for (i=0; i<Size(); i++)
    {
        TCHAR ach[LF_FACESIZE+20];
        BOOL fNotFirst = FALSE;
        SCRIPT_ID sid;

        pFontInfo = (CFontInfo *)Deref(sizeof(CFontInfo), i);

        wsprintf(ach, TEXT("%2d: "), i+1);

        OutputDebugString(ach);
        OutputDebugString(pFontInfo->_cstrFaceName);

        for (sid=0; sid<sidTridentLim; sid++)
        {
            if (pFontInfo->_sids & ScriptBit(sid))
            {
                OutputDebugStringA(fNotFirst ? ";" : " (");

                fNotFirst = TRUE;

                OutputDebugString(SidName(sid));
            }
        }
        
        OutputDebugStringA(fNotFirst ? ")\r\n" : "\r\n");
    }
}
#endif
