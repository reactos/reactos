/*
 *  @doc    INTERNAL
 *
 *  @module FONT.C -- font cache |
 *
 *      includes font cache, char width cache;
 *      create logical font if not in cache, look up
 *      character widths on an as needed basis (this
 *      has been abstracted away into a separate class
 *      so that different char width caching algos can
 *      be tried.) <nl>
 *
 *  Owner: <nl>
 *      David R. Fulmer <nl>
 *      Christian Fortini <nl>
 *      Jon Matousek <nl>
 *
 *  History: <nl>
 *      7/26/95     jonmat  cleanup and reorganization, factored out
 *                  char width caching code into a separate class.
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

DeclareTag( tagTextOutA, "TextOut", "Render using TextOutA always." );
DeclareTag( tagTextOutFE, "TextOut", "Reneder using FE ExtTextOutW workaround.");
DeclareTag( tagCCcsMakeFont, "Font", "Trace CCcs::MakeFont");
DeclareTag( tagCCcsCompare, "Font", "Trace CCcs::Compare");
DeclareTag( tagFontLeaks, "Font", "Trace HFONT leaks");
DeclareTag( tagNoFontLinkAdjust, "Font", "Don't adjust font height when fontlinking");
DeclareTag( tagFont, "Font", "Trace fonts");

MtDefine(CCcs, PerProcess, "CCcs")
MtDefine(CBaseCcs, PerProcess, "CBaseCcs")
MtDefine(CWidthCache, PerProcess, "CWidthCache")
MtDefine(CWidthCacheEntry, CWidthCache, "CWidthCache::GetEntry")
MtDefine(CFontCache, PerProcess, "CFontCache")

#if DBG==1

const int cMapSize = 1024;
class CDCToHFONT
{
    HDC   _dc[cMapSize];
    HFONT _font[cMapSize];
public:
    CDCToHFONT() {
        memset(_dc, 0, cMapSize*sizeof(HDC));
        memset(_font, 0, cMapSize*sizeof(HFONT));
    };
    void Assign(HDC hdc, HFONT hfont) {
        int freePos = -1;
        for (int i = 0; i < cMapSize; i++) {
            if (_dc[i] == hdc) {
                _font[i] = hfont;
                return;
            } else if (freePos == -1 && _dc[i] == 0)
                freePos = i;
        }
        Assert(freePos != -1);
        _dc[freePos] = hdc;
        _font[freePos] = hfont;
    };
    int FindNextFont(int posStart, HFONT hfont) {
        for (int i = posStart; i < cMapSize; i++) {
            if (_font[i] == hfont)
                return i;
        }
        return -1;
    };
    void Erase(int pos) {
        _dc[pos] = 0;
        _font[pos] = 0;
    };
    HFONT Font(int pos) {
        return _font[pos];
    };
    HDC DC(int pos) {
        return _dc[pos];
    };
};

CDCToHFONT mDc2Font;

HFONT SelectFontEx(HDC hdc, HFONT hfont)
{
#ifndef _WIN64
//$ Win64: GetObjectType is returning zero on Axp64
    Assert(OBJ_FONT == GetObjectType((HGDIOBJ)hfont));
#endif
    if (IsTagEnabled(tagFontLeaks))
    {
        mDc2Font.Assign(hdc, hfont);
    }
    return SelectFont(hdc, hfont);
}

BOOL DeleteFontEx(HFONT hfont)
{
    int pos = -1;
    if (IsTagEnabled(tagFontLeaks))
    {
        while (-1 != (pos = mDc2Font.FindNextFont(pos+1, hfont))) 
        {
            TraceTag((tagFontLeaks, "##### Attempt to delete selected font"));
            SelectObject(mDc2Font.DC(pos), GetStockObject(SYSTEM_FONT));
            mDc2Font.Erase(pos);
        }
    }
    return DeleteObject((HGDIOBJ)hfont);
}
#endif // DBG==1

#ifdef WIN16
#define GetCharWidthW   GetCharWidth
#endif //WIN16

#define CLIP_DFA_OVERRIDE   0x40    //  Used to disable Korea & Taiwan font association

// corresponds to yHeightCharPtsMost in textedit.h
#define yHeightCharMost 32760

ExternTag(tagMetrics);

static TCHAR lfScriptFaceName[LF_FACESIZE] = _T("Script");

inline const TCHAR * Arial() { return _T("Arial"); }
inline const TCHAR * TimesNewRoman() { return _T("Times New Roman"); }

CFontCache g_FontCache;

static SCRIPT_IDS sidsSystem = sidsNotSet;

// U+0000 -> U+4DFF     All Latin and other phonetics.
// U+4E00 -> U+ABFF     CJK Ideographics
// U+AC00 -> U+FFFF     Korean+, as Korean ends at U+D7A3

// For 2 caches at CJK Ideo split, max cache sizes {256,512} that give us a
// respective collision rate of <4% and <22%, and overall rate of <8%.
// Stats based on a 300K char Japanese text file.
INT maxCacheSize[TOTALCACHES] = {255, 511, 511};

DeclareTag(tagFontCache, "FontCache", "FontCache");

#define IsZeroWidth(ch) ((ch) >= 0x0300 && IsZeroWidthChar((ch)))

//
// Work around Win9x GDI bugs
//

static inline BOOL
FEFontOnNonFE95(BYTE bCharSet)
{
#ifndef WIN16   //BUGBUG: revisit DBCS issue
    // Use ExtTextOutA to hack around all sort of Win95FE EMF or font problems
    return VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID &&
            !g_fFarEastWin9X &&
            IsFECharset(bCharSet);
#else
    return FALSE;
#endif
}

#if DBG==1
LONG CBaseCcs::s_cMaxCccs = 0;
LONG CBaseCcs::s_cTotalCccs = 0;
#endif

//+-----------------------------------------------------------------------
//
//  Function:   DeinitUniscribe
//
//  Synopsis:   Clears script caches in USP.DLL private heap. This will
//              permit a clean shut down of USP.DLL (Uniscribe).
//
//------------------------------------------------------------------------
void
DeinitUniscribe()
{
    g_FontCache.FreeScriptCaches();
}


/*
 *  CFontCache & fc()
 *
 *  @func
 *      return the global font cache.
 *  @comm
 *      current #defined to store 16 logical fonts and
 *      respective character widths.
 *
 */
CFontCache & fc()
{
    return(g_FontCache);
}

// ===================================  CFontCache  ====================================


void InitFontCache()
{
    g_FontCache.Init();
}

void DeInitFontCache()
{
    g_FontCache.DeInit();
}

// Return our small face name cache to a zero state.
void ClearFaceCache()
{
    EnterCriticalSection(&g_FontCache._csFaceCache);
    g_FontCache._iCacheLen = 0;
    g_FontCache._iCacheNext = 0;
    LeaveCriticalSection(&g_FontCache._csFaceCache);
}

/*
 *  CFontCache::DeInit()
 *
 *  @mfunc
 *      This is allocated as an extern. Don't let the
 *      CRT determine when we get dealloc'ed.
 */
void CFontCache::DeInit()
{
    DWORD i;


    for (i = 0; i < cccsMost; i++)
    {
        if (_rpBaseCcs[i])
        {
            _rpBaseCcs[i]->NullOutScriptCache();
            _rpBaseCcs[i]->PrivateRelease();
        }
    }

    DeleteCriticalSection(&_cs);
    DeleteCriticalSection(&_csOther);
    DeleteCriticalSection(&_csFaceCache);
    DeleteCriticalSection(&_csFaceNames);

    _atFontInfo.Free();  // Maybe we should do this in a critical section, but naw...

#if DBG == 1
    TraceTag((tagMetrics, "Font Metrics:"));

    TraceTag((tagMetrics, "\tSize of FontCache:%ld", sizeof(CFontCache) ));
    TraceTag((tagMetrics, "\tSize of Cccs:%ld + a min of 1024 bytes", sizeof(CCcs) ));
    TraceTag((tagMetrics, "\tMax no. of fonts allocated:%ld", CBaseCcs::s_cMaxCccs));
    TraceTag((tagMetrics, "\tNo. of fonts replaced: %ld", _cCccsReplaced));
#endif
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::EnsureScriptIDsForFont
//
//  Synopsis:   When we add a new facename to our _atFontInfo cache, we
//              defer the calculation of the script IDs (sids) for this
//              face.  An undetermined sids has the value of sidsNotSet.
//              Inside of CBaseCcs::MakeFont, we need to make sure that the
//              script IDs are computed, as we will need this information
//              to fontlink properly.
//
//  Returns:    SCRIPT_IDS.  If an error occurs, we return sidsAll, which
//              effectively disables fontlinking for this font.
//
//-----------------------------------------------------------------------------

SCRIPT_IDS
CFontCache::EnsureScriptIDsForFont(
    HDC hdc,
    CBaseCcs * pBaseCcs,
    BOOL fDownloadedFont )
{
    const LONG latmFontInfo = pBaseCcs->_latmLFFaceName;

    if (latmFontInfo)
    {
        CFontInfo * pfi;
        HRESULT hr = THR(_atFontInfo.GetInfoFromAtom(latmFontInfo-1, &pfi));

        if (!hr)
        {
            if (pfi->_sids == sidsNotSet)
            {
                if (!fDownloadedFont)
                {
                    pfi->_sids = ScriptIDsFromFont( hdc,
                                                    pBaseCcs->_hfont,
                                                    pBaseCcs->_sPitchAndFamily & TMPF_TRUETYPE );

                    if (pBaseCcs->_fLatin1CoverageSuspicious)
                    {
                        pfi->_sids &= ~(ScriptBit(sidLatin));
                    }
                }
                else
                {
                    pfi->_sids = sidsAll; // don't fontlink for embedded fonts
                }
            }

            return pfi->_sids;
        }
    }
    else
    {
        if (sidsSystem == sidsNotSet)
        {
            sidsSystem = ScriptIDsFromFont( hdc,
                                            pBaseCcs->_hfont,
                                            pBaseCcs->_sPitchAndFamily & TMPF_TRUETYPE );
        }

        return sidsSystem;
    }

    return sidsAll;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::ScriptIDsForAtom
//
//  Returns:    Return the SCRIPT_IDS associated with the facename-base atom.
//
//-----------------------------------------------------------------------------

SCRIPT_IDS
CFontCache::ScriptIDsForAtom( LONG latmFontInfo )
{
    Assert(latmFontInfo);

    CFontInfo * pfi;
    HRESULT hr = THR(_atFontInfo.GetInfoFromAtom(latmFontInfo-1, &pfi));

    if (!hr)
    {
        Assert(pfi->_sids == sidsNotSet);

        return pfi->_sids;
    }

    return sidsAll;
}

LONG
CFontCache::GetAtomWingdings()
{
    if( !_latmWingdings )
    {
        _latmWingdings = GetAtomFromFaceName( _T("Wingdings") );
        _atFontInfo.SetScriptIDsOnAtom( _latmWingdings - 1, sidsAll );
    }
    return _latmWingdings;
}

HRESULT
CFontCache::AddScriptIdToAtom(
    LONG latmFontInfo,
    SCRIPT_ID sid )
{
    RRETURN(_atFontInfo.AddScriptIDOnAtom(latmFontInfo-1, sid));
}

/*
 *  CFontCache::FreeScriptCaches ()
 *
 *  @mfunc
 *      frees SCRIPT_CACHE memory allocated in USP.DLL
 */
void
CFontCache::FreeScriptCaches()
{
    DWORD i;

    
    for (i = 0; i < cccsMost; i++)
    {
        if (_rpBaseCcs[i])
            _rpBaseCcs[i]->ReleaseScriptCache();
    }

}


/*
 *  CCcs* CFontCache::GetCcs(hdc, pcf, lZoomNumerator, lZoomDenominator, yPixelsPerInch)
 *
 *  @mfunc
 *      Search the font cache for a matching logical font and return it.
 *      If a match is not found in the cache,  create one.
 *
 *  @rdesc
 *      A logical font matching the given CHARFORMAT info.
 */
CCcs*
CFontCache::GetCcs(
    HDC hdc,
    CDocInfo * pdci,
    const CCharFormat * const pcf )
{
    CCcs *pccs = new CCcs();

    if (pccs)
    {
        pccs->_hdc = hdc;
        pccs->_pBaseCcs = GetBaseCcs(hdc, pdci, pcf, NULL);
        if (!pccs->_pBaseCcs)
        {
            delete pccs;
            pccs = NULL;
        }
    }

    return pccs;
}

CCcs*
CFontCache::GetFontLinkCcs(
    HDC hdc,
    CDocInfo * pdci,
    CCcs * pccsBase,
    const CCharFormat * const pcf )
{
    CCcs *pccs;

#if DBG==1
    //
    // Allow disabling the height-adjusting feature of fontlinking through tags
    //
    
    if (IsTagEnabled(tagNoFontLinkAdjust))
        return GetCcs(hdc, pdci, pcf);
#endif

    pccs = new CCcs();

    if (pccs)
    {
        CBaseCcs * pBaseBaseCcs = pccsBase->_pBaseCcs;

        pBaseBaseCcs->AddRef();

        pccs->_hdc = hdc;
        pccs->_pBaseCcs = GetBaseCcs(hdc, pdci, pcf, pBaseBaseCcs);
        if (!pccs->_pBaseCcs)
        {
            delete pccs;
            pccs = NULL;
            goto Cleanup;
        }

        TraceTag(( tagFont,
                   "GetFontLinkCcs %S based on %S",
                   fc().GetFaceNameFromAtom(pcf->_latmFaceName),
                   fc().GetFaceNameFromAtom(pBaseBaseCcs->_latmLFFaceName)) );
                   
        pBaseBaseCcs->PrivateRelease();
    }
    else
    {
        pccs = pccsBase;
    }

Cleanup:

    return pccs;
}

CBaseCcs*
CFontCache::GetBaseCcs(
    HDC hdc,
    CDocInfo * pdci,
    const CCharFormat *const pcf,     //@parm description of desired logical font
    CBaseCcs * pBaseBaseCcs)          //@parm facename from which we're fontlinking
{
    CBaseCcs *pBaseCcs = NULL;
    LONG      lfHeight;
    int       i;
    BYTE      bCrc;
    SHORT     hashKey;
    CBaseCcs::CompareArgs cargs;
    BOOL (CBaseCcs::*CompareFunc)(CBaseCcs::CompareArgs*);

    // Duplicate the format structure because we might need to change some of the
    // values by the zoom factor
    // and in the case of sub superscript
    CCharFormat cf = *pcf;

    //FUTURE igorzv
    //Take subscript size, subscript offset, superscript offset, superscript size
    // from the OUTLINETEXMETRIC

    lfHeight = -cf.GetHeightInPixels( pdci );

    // change cf._yHeight in the case of sub superscript
    if (cf._fSuperscript || cf._fSubscript)
    {
        LONG yHeight = cf.GetHeightInTwips(pdci->_pDoc);

        if (cf._fSubscript)
        {
            cf._yOffset -= yHeight / 2;
        }
        else
        {
            cf._yOffset += yHeight / 2;
        }

        cf._bCrcFont = cf.ComputeFontCrc();

        // NOTE: lfHeight has already been scaled by 2/3 if *SCRIPT set
    }

    // BUGBUG (cthrash) this looks suspicious.  Evidently the only time cy will not
    // equal pdci->_sizeInch.cy is when we're minimized.  Even that's a little bogus.
    // We shouldn't be measuring when minimized.  Let's remove the code.
    //
    //    int cy = pdci->DeviceFromHimetricCY(2540);
    //    if (cy != pdci->_sizeInch.cy)
    //    {
    //        cf._yHeight = MulDivQuick(cf._yHeight, cy, pdci->_sizeInch.cy);
    //        cf._yOffset = MulDivQuick(cf._yOffset, cy, pdci->_sizeInch.cy);
    //    }

    bCrc = cf._bCrcFont;

    Assert (bCrc == cf.ComputeFontCrc());

    if (!lfHeight)
        lfHeight--; // round error, make this a minimum legal height of -1.

    cargs.pcf = &cf;
    cargs.lfHeight = lfHeight;

    if (pBaseBaseCcs)
    {
        cargs.latmBaseFaceName = pBaseBaseCcs->_latmLFFaceName;
        CompareFunc = CBaseCcs::CompareForFontLink;
    }
    else
    {
        cargs.latmBaseFaceName = pcf->_latmFaceName;
        CompareFunc = CBaseCcs::Compare;
    }

    EnterCriticalSection(&_cs);

    // check our hash before going sequential.
    hashKey = bCrc & QUICKCRCSEARCHSIZE;
    if ( bCrc == quickCrcSearch[hashKey].bCrc )
    {
        pBaseCcs = quickCrcSearch[hashKey].pBaseCcs;
        if (pBaseCcs && pBaseCcs->_bCrc == bCrc)
        {
            if ((pBaseCcs->*CompareFunc)( &cargs ))
            {
                goto matched;
            }
        }
        pBaseCcs = NULL;
    }
    quickCrcSearch[hashKey].bCrc = bCrc;

    // squentially search ccs for same character format
    for(i = 0; i < cccsMost; i++)
    {
        CBaseCcs *  pBaseCcsTemp = _rpBaseCcs[i];

        if ( pBaseCcsTemp && pBaseCcsTemp->_bCrc == bCrc)
        {
            if ((pBaseCcsTemp->*CompareFunc)( &cargs ))
            {
                pBaseCcs = pBaseCcsTemp;
                break;
            }
        }
    }

matched:
    if (!pBaseCcs)
    {
        // we did not find a match, init a new font cache.
        pBaseCcs = GrabInitNewBaseCcs ( hdc, &cf, pdci, cargs.latmBaseFaceName );

        if (   pBaseCcs
            && pBaseBaseCcs
            && !pBaseCcs->_fHeightAdjustedForFontlinking)
        {
            pBaseCcs->FixupForFontLink( hdc, pBaseBaseCcs );
        }
    }
    else
    {
        if (pBaseCcs->_dwAge != _dwAgeNext - 1)
            pBaseCcs->_dwAge = _dwAgeNext++;

        // the same font can be used at different offsets.
#ifdef  IE5_ZOOM

        pBaseCcs->_yOffset =  cf._yOffset
                          ? pdci->DyzFromDyt(pdci->DytFromTwp(cf._yOffset))
                          : 0;

#if DBG==1
        long dyzOld = MulDivQuick(cf._yOffset, pdci->_sizeInch.cy, LY_PER_INCH);
        Assert(pBaseCcs->_yOffset == dyzOld || pdci->IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

        pBaseCcs->_yOffset =  cf._yOffset
                          ? (cf._yOffset * pdci->_sizeInch.cy / LY_PER_INCH)
                          : 0;

#endif  // IE5_ZOOM
    }

    if (pBaseCcs)
    {
        quickCrcSearch[hashKey].pBaseCcs = pBaseCcs;

        // AddRef the entry being returned
        pBaseCcs->AddRef();

        pBaseCcs->EnsureFastCacheExists(hdc);
    }

    LeaveCriticalSection(&_cs);

    return pBaseCcs;
}

// Checks to see if this face name is in the atom table.
// if not, puts it in.  We report externally 1 higher than the
// actualy atom table values, so that we can reserve latom==0
// to the error case, or a blank string.
LONG
CFontCache::GetAtomFromFaceName( const TCHAR* szFaceName )
{
    HRESULT hr;
    LONG lAtom=0;

    // If they pass in the NULL string, when they ask for it out again,
    // they're gonna get a blank string, which is different.
    Assert(szFaceName);

    if( szFaceName && *szFaceName ) {

        EnterCriticalSection(&_csFaceNames);
        hr= _atFontInfo.GetAtomFromName(szFaceName, &lAtom);
        if( hr )
        {
            // String not in there.  Put it in.
            // Note we defer the calculation of the SCRIPT_IDS.
            hr= THR(_atFontInfo.AddInfoToAtomTable(szFaceName, &lAtom));
            AssertSz(hr==S_OK,"Failed to add Font Face Name to Atom Table");
        }
        if( hr == S_OK )
        {
            lAtom++;
        }
        LeaveCriticalSection(&_csFaceNames);
    }
    // else input was NULL or empty string.  Return latom=0.

    return lAtom;
}

// Checks to see if this face name is in the atom table.
LONG
CFontCache::FindAtomFromFaceName( const TCHAR* szFaceName )
{
    HRESULT hr;
    LONG lAtom=0;

    // If they pass in the NULL string, when they ask for it out again,
    // they're gonna get a blank string, which is different.
    Assert(szFaceName);

    if( szFaceName && *szFaceName ) {

        EnterCriticalSection(&_csFaceNames);
        hr= _atFontInfo.GetAtomFromName(szFaceName, &lAtom);
        if( hr == S_OK )
        {
            lAtom++;
        }
        LeaveCriticalSection(&_csFaceNames);
    }

    // lAtom of zero means that it wasn't found or that the input was bad.
    return lAtom;
}

const TCHAR *
CFontCache::GetFaceNameFromAtom( LONG latmFaceName )
{
    const TCHAR* szReturn=g_Zero.ach;

    if( latmFaceName )
    {
        HRESULT hr;
        CFontInfo * pfi;

        EnterCriticalSection(&_csFaceNames);
        hr = THR(_atFontInfo.GetInfoFromAtom(latmFaceName-1, &pfi));
        LeaveCriticalSection(&_csFaceNames);
        Assert( !hr );
        Assert( pfi->_cstrFaceName.Length() < LF_FACESIZE );
        szReturn = pfi->_cstrFaceName;
    }
    return szReturn;
}

void
CBaseCcs::NullOutScriptCache()
{
    Assert(_sc == NULL);

    // A safety valve so we don't crash.
    if(_sc)
        _sc = NULL;
}


void
CBaseCcs::ReleaseScriptCache()
{
    // Free the script cache
    if (_sc)
    {
        ::ScriptFreeCache(&_sc);
        // NB (mikejoch) If ScriptFreeCache() fails then there is no way to
        // free the cache, so we'll end up leaking it. This shouldn't ever
        // happen since the only way for _sc to be non- NULL is via some other
        // USP function succeeding.
    }
    Assert(_sc == NULL);
}

void
CBaseCcs::PrivateRelease()
{
    if (InterlockedDecrement((LONG *)&_dwRefCount) == 0)
        delete this;
}

/*
 *  CBaseCcs* CFontCache::GrabInitNewBaseCcs(hdc, pcf)
 *
 *  @mfunc
 *      create a logical font and store it in our cache.
 *
 */
CBaseCcs*
CFontCache::GrabInitNewBaseCcs(
    HDC hdc,                        //@parm HDC into which font will be selected
    const CCharFormat * const pcf,  //@parm description of desired logical font
    CDocInfo * pdci,                //@parm Y Pixels per Inch
    LONG latmBaseFaceName )
{
    int     i;
    int     iReplace = -1;
    int     iOldest = -1;
    DWORD   dwAgeReplace = 0xffffffff;
    DWORD   dwAgeOldest = 0xffffffff;
    CBaseCcs *pBaseCcsNew = new CBaseCcs();

    // Initialize new CBaseCcs
    if (!pBaseCcsNew || !pBaseCcsNew->Init(hdc, pcf, pdci, latmBaseFaceName))
    {
        if (pBaseCcsNew)
            pBaseCcsNew->PrivateRelease();

        AssertSz(FALSE, "CFontCache::GrabInitNewBaseCcs init of entry FAILED");
        return NULL;
    }

    MemSetName((pBaseCcsNew, "CBaseCcs F:%ls, H:%d, W:%d", pBaseCcsNew->_lf.lfFaceName, -pBaseCcsNew->_lf.lfHeight, pBaseCcsNew->_lf.lfWeight));

    // look for unused entry and oldest in use entry
    for(i = 0; i < cccsMost && _rpBaseCcs[i]; i++)
    {
        CBaseCcs * pBaseCcs = _rpBaseCcs[i];
        if (pBaseCcs->_dwAge < dwAgeOldest)
        {
            iOldest = i;
            dwAgeOldest = pBaseCcs->_dwAge;
        }
        if (pBaseCcs->_dwRefCount == 1)
        {
            if (pBaseCcs->_dwAge < dwAgeReplace)
            {
                iReplace  = i;
                dwAgeReplace = pBaseCcs->_dwAge;
            }
        }
    }

    if (i == cccsMost)     // Didn't find an unused entry, use oldest entry
    {
        int hashKey;
        // if we didn't find a replacement, replace the oldest
        if (iReplace == -1)
        {
            Assert(iOldest != -1);
            iReplace = iOldest;
        }

#if DBG == 1
        _cCccsReplaced++;
#endif

        hashKey = _rpBaseCcs[iReplace]->_bCrc & QUICKCRCSEARCHSIZE;
        if (quickCrcSearch[hashKey].pBaseCcs == _rpBaseCcs[iReplace])
        {
            quickCrcSearch[hashKey].pBaseCcs = NULL;
        }

        TraceTag((tagFontCache, "Releasing font(F:%ls, H:%d, W:%d) from slot %d",
                  _rpBaseCcs[iReplace]->_lf.lfFaceName,
                  -_rpBaseCcs[iReplace]->_lf.lfHeight,
                  _rpBaseCcs[iReplace]->_lf.lfWeight,
                  iReplace));

        _rpBaseCcs[iReplace]->PrivateRelease();
        i = iReplace;
    }

    TraceTag((tagFontCache, "Storing font(F:%ls, H:%d, W:%d) in slot %d",
              pBaseCcsNew->_lf.lfFaceName,
              -pBaseCcsNew->_lf.lfHeight,
              pBaseCcsNew->_lf.lfWeight,
              i));

    _rpBaseCcs[i]  = pBaseCcsNew;

    return pBaseCcsNew;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::SetHanCodePageInfo()
//
//  Synopsis:   Determine the FE font converage of the system.  This
//              information will used to choose between FE fonts when we have
//              a text run of ambiguous Han characters.
//
//  Returns:    A dword of FS_* bits
//
//-----------------------------------------------------------------------------

const BYTE s_abCharSet[] =
{ 
    JOHAB_CHARSET,       // FS_JOHAB                0x00200000L
    CHINESEBIG5_CHARSET, // FS_CHINESETRAD          0x00100000L
    HANGEUL_CHARSET,     // FS_WANSUNG              0x00080000L
    GB2312_CHARSET,      // FS_CHINESESIMP          0x00040000L
    SHIFTJIS_CHARSET,    // FS_JISJAPAN             0x00020000L
    VIETNAMESE_CHARSET,  // FS_VIETNAMESE           0x00000100L
    BALTIC_CHARSET,      // FS_BALTIC               0x00000080L
    ARABIC_CHARSET,      // FS_ARABIC               0x00000040L
    HEBREW_CHARSET,      // FS_HEBREW               0x00000020L
    TURKISH_CHARSET,     // FS_TURKISH              0x00000010L
    GREEK_CHARSET,       // FS_GREEK                0x00000008L
    RUSSIAN_CHARSET,     // FS_CYRILLIC             0x00000004L
    EASTEUROPE_CHARSET,  // FS_LATIN2               0x00000002L
};

const DWORD s_adwFontScripts[] =
{
    FS_JOHAB,            // JOHAB_CHARSET
    FS_CHINESETRAD,      // CHINESEBIG5_CHARSET
    FS_WANSUNG,          // HANGEUL_CHARSET
    FS_CHINESESIMP,      // GB2312_CHARSET
    FS_JISJAPAN,         // SHIFTJIS_CHARSET
    FS_VIETNAMESE,       // VIETNAMESE_CHARSET
    FS_BALTIC,           // BALTIC_CHARSET
    FS_ARABIC,           // ARABIC_CHARSET
    FS_HEBREW,           // HEBREW_CHARSET
    FS_TURKISH,          // TURKISH_CHARSET
    FS_GREEK,            // GREEK_CHARSET
    FS_CYRILLIC,         // RUSSIAN_CHARSET
    FS_LATIN2,           // EASTEUROPE_CHARSET
};

int FAR PASCAL CALLBACK
SetSupportedCodePageInfoEnumFontCallback(
    const LOGFONT FAR * lplf,
    const TEXTMETRIC FAR * lptm,
    DWORD FontType,
    LPARAM lParam )
{
    *((BOOL *)lParam) = TRUE; // fFound;

    return 0;
}

DWORD
CFontCache::SetSupportedCodePageInfo(HDC hdc)
{
    DWORD dwSupportedCodePageInfo = 0;
    
    LOGFONT lf;
    BOOL fFound;
    int i;

    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    
    for (i=ARRAY_SIZE(s_abCharSet);i;)
    {
        lf.lfCharSet = s_abCharSet[--i];

        fFound = FALSE;

        EnumFontFamiliesEx(hdc, &lf, SetSupportedCodePageInfoEnumFontCallback, LPARAM(&fFound), 0);

        if (fFound)
        {
            dwSupportedCodePageInfo |= s_adwFontScripts[i];
        }
    }
    
    _dwSupportedCodePageInfo = dwSupportedCodePageInfo;
    
    return dwSupportedCodePageInfo;
}

// =============================  CBaseCcs  class  ===================================================


/*
 *  BOOL CBaseCcs::Init()
 *
 *  @mfunc
 *      Init one font cache object. The global font cache stores
 *      individual CBaseCcs objects.
 */
BOOL
CBaseCcs::Init(
    HDC hdc,                        //@parm HDC into which font will be selected
    const CCharFormat * const pcf,  //@parm description of desired logical font
    CDocInfo * pdci,                //@parm Y pixels per inch
    LONG latmBaseFaceName )
{
    _sc = NULL; // Initialize script cache to NULL - COMPLEXSCRIPT

    _fPrinting = pdci && pdci->_pDoc && pdci->_pDoc->IsPrintDoc();

    _bConvertMode = CM_NONE;

    if ( MakeFont(hdc, pcf, pdci) )
    {
        _bCrc = pcf->_bCrcFont;
        _yCfHeight = pcf->_yHeight;
        _latmBaseFaceName = latmBaseFaceName;
        _fHeightAdjustedForFontlinking = FALSE;

        // BUGBUG (cthrash) This needs to be removed.  We used to calculate
        // this all the time, now we only calculate it on an as-needed basis,
        // which means at intrinsics fontlinking time.
      
        _dwLangBits = 0;

        // the same font can be used at different offsets.
#ifdef  IE5_ZOOM

        _yOffset =  pcf->_yOffset ?
                    pdci->DyzFromDyt(pdci->DytFromTwp(pcf->_yOffset)) :
                    0;

#if DBG==1
        long dyzOld = MulDivQuick(pcf->_yOffset, pdci->_sizeInch.cy, LY_PER_INCH);
        Assert(_yOffset == dyzOld || pdci->IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

        _yOffset =  pcf->_yOffset ?
                    (pcf->_yOffset * pdci->_sizeInch.cy / LY_PER_INCH) :
                    0;

#endif  // IE5_ZOOM

        _dwAge = fc()._dwAgeNext++;

        return TRUE;         // successfully created a new font cache.
    }

    return FALSE;
}

/*
 *  BOOL CBaseCcs::MakeFont(hdc, pcf)
 *
 *  @mfunc
 *      Wrapper, setup for CreateFontIndirect() to create the font to be
 *      selected into the HDC.
 *
 *  @rdesc
 *      TRUE if OK, FALSE if allocation failure
 */

CODEPAGE DefaultCodePageFromCharSet(BYTE,UINT);

BOOL
CBaseCcs::MakeFont(
    HDC hdc,                        //@parm HDC into which  font will be selected
    const CCharFormat * const pcf,  //@parm description of desired logical font
    CDocInfo * pdci)
{
    BOOL fTweakedCharSet = FALSE;
    HFONT hfontOriginalCharset = NULL;
    TCHAR pszNewFaceName[LF_FACESIZE];
    const CODEPAGE cpDoc = pdci->_pDoc->GetFamilyCodePage();
    const LCID lcid = pcf->_lcid;
    BOOL fRes;
    LONG lfHeight;

    // We need the _sCodePage in case we need to call ExtTextOutA rather than ExtTextOutW.  
    _sCodePage = (USHORT)DefaultCodePageFromCharSet(pcf->_bCharSet, cpDoc);

    // Computes font height
    AssertSz(pcf->_yHeight <= INT_MAX, "It's too big");

    //  Roundoff can result in a height of zero, which is bad.
    //  If this happens, use the minimum legal height.
    lfHeight = -(const_cast<CCharFormat *const>(pcf)->GetHeightInPixels( pdci ));
    if (lfHeight > 0)
    {
        lfHeight = -lfHeight;       // TODO: do something more intelligent...
    }
    else if (!lfHeight)
    {
        lfHeight--;                 // round error, make this a minimum legal height of -1.
    }
    _lf.lfHeight = _yOriginalHeight = lfHeight;
    _lf.lfWidth  = 0;

    if ( pcf->_wWeight != 0 )
    {
        _lf.lfWeight = pcf->_wWeight;
    }
    else
    {
        _lf.lfWeight    = pcf->_fBold ? FW_BOLD : FW_NORMAL;
    }
    _lf.lfItalic        = pcf->_fItalic;
    _lf.lfUnderline     = 0; // pcf->_fUnderline;
    _lf.lfStrikeOut     = 0; // pcf->_fStrikeOut;
    _lf.lfCharSet       = pcf->_bCharSet;
    _lf.lfEscapement    = 0;
    _lf.lfOrientation   = 0;
    _lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    _lf.lfQuality       = DEFAULT_QUALITY;
    _lf.lfPitchAndFamily = _bPitchAndFamily = pcf->_bPitchAndFamily;
    _lf.lfClipPrecision = CLIP_DEFAULT_PRECIS | CLIP_DFA_OVERRIDE;

    // HACK (cthrash) Don't pick a non-TT font for generic font types 'serif'
    // or 'sans-serif.'  More precisely, prevent MS Sans Serif and MS Serif
    // from getting selected.

    if (   pcf->_latmFaceName == 0
        && (pcf->_bPitchAndFamily & (FF_ROMAN | FF_SWISS)))
    {
        _lf.lfOutPrecision |= OUT_TT_ONLY_PRECIS;
    }
    
    // Only use CLIP_EMBEDDED when necessary, or Win95 will make you pay.
    if (pcf->_fDownloadedFont)
    {
        _lf.lfClipPrecision |= CLIP_EMBEDDED;
    }

    // Note (paulpark): We must be careful with _lf.  It is important that
    // _latmLFFaceName be kept in sync with _lf.lfFaceName.  The way to do that
    // is to use the SetLFFaceName function.
    SetLFFaceNameAtm(pcf->_latmFaceName);

    fRes = GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

    if (   (_sPitchAndFamily & 0x00F0) != pcf->_bPitchAndFamily
        && (   pcf->_bPitchAndFamily == FF_SCRIPT
            || pcf->_bPitchAndFamily == FF_DECORATIVE))
   
    {
        // If we ask for FF_SCRIPT or FF_DECORATIVE, we sometimes get
        // a font which has neither of those characteristics (since
        // the font matcher does not map anything in OEM_CHARSET to
        // DEFAULT_CHARSET).  In this case use our default face names.

        _lf.lfCharSet = DEFAULT_CHARSET; // accept all charsets

        if (pcf->_bPitchAndFamily == FF_SCRIPT)
        {
            SetLFFaceName(lfScriptFaceName);
        }
        else
        {
            SetLFFaceName(Arial()); // BUGBUG (what should this be)
        }

        //
        // Check the glyph coverage of this font.  See if the sid in
        // question is covered.  If the font doesn't cover the script
        // we want, don't bother switching - restore.
        //

#if 0
        const SCRIPT_IDS sidsFont = fc().EnsureScriptIDsForAtom(hdc, _latmLFFaceName, pcf);
        const SCRIPT_IDS sidsText = ScriptBit(sid);

        if (sidsFont & sidsText)
        {
            DestroyFont();
            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);
        }
        else
        {
            _lf.lfCharSet = pcf->_bCharSet;
            SetLFFaceNameAtm(pcf->_latmFaceName);
        }
#endif
    }

    if (!_tcsiequal(pszNewFaceName, _lf.lfFaceName))
    {
        BOOL fCorrectFont = FALSE;

        if (_bCharSet == SYMBOL_CHARSET)
        {
            // #1. if the face changed, and the specified charset was SYMBOL,
            //     but the face name exists and suports ANSI, we give preference
            //     to the face name

            _lf.lfCharSet = ANSI_CHARSET;
            fTweakedCharSet = TRUE;

            hfontOriginalCharset = _hfont;
            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

            if (_tcsiequal(pszNewFaceName, _lf.lfFaceName))
                // that's right, ANSI is the asnwer
                fCorrectFont = TRUE;
            else
                // no, fall back by default
                // the charset we got was right
                _lf.lfCharSet = pcf->_bCharSet;
        }
        else if (_lf.lfCharSet == DEFAULT_CHARSET &&
                 _bCharSet == DEFAULT_CHARSET)
        {
            // #2. If we got the "default" font back, we don't know what it
            // means (could be anything) so we verify that this guy's not SYMBOL
            // (symbol is never default, but the OS could be lying to us!!!)
            // we would like to verify more like whether it actually gave us
            // Japanese instead of ANSI and labeled it "default"...
            // but SYMBOL is the least we can do

            _lf.lfCharSet = SYMBOL_CHARSET;
            SetLFFaceName(pszNewFaceName);
            fTweakedCharSet = TRUE;

            hfontOriginalCharset = _hfont;
            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

            if (_tcsiequal(pszNewFaceName, _lf.lfFaceName))
                // that's right, it IS symbol!
                // 'correct' the font to the 'true' one,
                //  and we'll get fMappedToSymbol
                fCorrectFont = TRUE;

            // always restore the charset name, we didn't want to
            // question he original choice of charset here
            _lf.lfCharSet = pcf->_bCharSet;

        }
#ifndef NO_MULTILANG
        else if ( _bConvertMode != CM_SYMBOL &&
                  IsFECharset(_lf.lfCharSet) &&
                  (!g_fFarEastWinNT || !g_fFarEastWin9X))
        {
            // NOTE (cthrash) _lf.lfCharSet is what we asked for, _bCharset is what we got.

            if (_bCharSet != _lf.lfCharSet &&
                (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID))
            {
                SCRIPT_ID sid;
                LONG latmFontFace;

                // on Win95, when rendering to PS driver,
                // it will give us something other than what we asked.
                // We have to try some known font we got from GDI
                switch (_lf.lfCharSet)
                {
                    case CHINESEBIG5_CHARSET:
                        sid = sidBopomofo;
                        break;

                    case SHIFTJIS_CHARSET:
                        sid = sidKana;
                        break;

                    case HANGEUL_CHARSET:
                        sid = sidHangul;
                        break;

                    case GB2312_CHARSET:
                        sid = sidHan;
                        break;

                    default:
                        sid = sidDefault;
                        break;
                }

                ScriptAppropriateFaceNameAtom(sid, pdci->_pDoc, FALSE, &latmFontFace);
                SetLFFaceNameAtm(latmFontFace);
            }
            else if (   _lf.lfCharSet == GB2312_CHARSET
                     && _lf.lfPitchAndFamily | FIXED_PITCH)
            {
                // HACK (cthrash) On vanilla PRC systems, you will not be able to ask
                // for a fixed-pitch font which covers the non-GB2312 GBK codepoints.
                // We come here if we asked for a fixed-pitch PRC font but failed to 
                // get a facename match.  So we try again, only without FIXED_PITCH
                // set.  The side-effect is that CBaseCcs::Compare needs to compare
                // against the original _bPitchAndFamily else it will fail every time.
                
                _lf.lfPitchAndFamily = _lf.lfPitchAndFamily ^ FIXED_PITCH;
            }
            else
            {
                // this is a FE Font (from Lang pack) on a nonFEsystem
                SetLFFaceName(pszNewFaceName);
            }

            hfontOriginalCharset = _hfont;

            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

            if (_tcsiequal(pszNewFaceName, _lf.lfFaceName))
            {
                // that's right, it IS the FE font we want!
                // 'correct' the font to the 'true' one.
                fCorrectFont = TRUE;
            }

            fTweakedCharSet = TRUE;
        }
#endif // !NO_IME

        if (hfontOriginalCharset)
        {
            // either keep the old font or the new one

            if (fCorrectFont)
            {
                DeleteFontEx(hfontOriginalCharset);
                hfontOriginalCharset = NULL;
            }
            else
            {
                // fall back to the original font
                DeleteFontEx(_hfont);

                _hfont = hfontOriginalCharset;
                hfontOriginalCharset = NULL;

                GetTextMetrics( hdc, cpDoc, lcid );
            }
        }
    }

RetryCreateFont:
    if (!pcf->_fDownloadedFont)
    {
        // could be that we just plain symply get mapped to symbol.
        // avoid it
        BOOL fMappedToSymbol =  (_bCharSet == SYMBOL_CHARSET &&
                                 _lf.lfCharSet != SYMBOL_CHARSET);

        BOOL fChangedCharset = (_bCharSet != _lf.lfCharSet &&
                                _lf.lfCharSet != DEFAULT_CHARSET);

        if (fChangedCharset || fMappedToSymbol)
        {
            const TCHAR * pchFallbackFaceName = (pcf->_bPitchAndFamily & FF_ROMAN)
                                                ? TimesNewRoman()
                                                : Arial();
            
            // Here, the system did not preserve the font language or mapped
            // our non-symbol font onto a symbol font,
            // which will look awful when displayed.
            // Giving us a symbol font when we asked for a non-symbol one
            // (default can never be symbol) is very bizzare and means
            // that either the font name is not known or the system
            // has gone complete nuts here.
            // The charset language takes priority over the font name.
            // Hence, I would argue that nothing can be done to save the
            // situation at this point, and we have to
            // delete the font name and retry

            // let's tweak it a bit
            fTweakedCharSet = TRUE;

            if (_tcsiequal(_lf.lfFaceName, pchFallbackFaceName))
            {
                // we've been here already
                // no font with an appropriate charset is on the system
                // try getting the ANSI one for the original font name
                // next time around, we'll null out the name as well!!
                if (_lf.lfCharSet == ANSI_CHARSET)
                {
                    TraceTag((tagWarning, "Asking for ANSI ARIAL and not getting it?!"));

                    // those Win95 guys have definitely outbugged me
                    goto Cleanup;
                }

                DeleteFontEx(_hfont);
                _hfont = NULL;
                SetLFFaceNameAtm(pcf->_latmFaceName);
                _lf.lfCharSet = ANSI_CHARSET;

            }
            else
            {
                DeleteFontEx(_hfont);
                _hfont = NULL;
                SetLFFaceName(pchFallbackFaceName);
            }

            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);
            goto RetryCreateFont;
        }

    }

Cleanup:
    if (fTweakedCharSet || _bConvertMode == CM_SYMBOL)
    {
        // we must preserve the original charset value, since it is used in Compare()
        _lf.lfCharSet = pcf->_bCharSet;
        SetLFFaceNameAtm( pcf->_latmFaceName );
    }

    if (hfontOriginalCharset)
    {
        DeleteFontEx(hfontOriginalCharset);
        hfontOriginalCharset = NULL;
    }

    // if we're really really stuck, just get the system font and hope for the best.
    if ( _hfont == 0 )
    {
        _hfont = (HFONT)GetStockObject(SYSTEM_FONT);
    }

    _fFEFontOnNonFEWin95 = FEFontOnNonFE95( _bCharSet );

    // Make sure we know what have script IDs computed for this font.  Cache this value
    // to avoid a lookup later.

    _sids = fc().EnsureScriptIDsForFont( hdc, this, pcf->_fDownloadedFont );
    _sids &= ScriptIDsFromCharSet(_bCharSet);

    TraceTag((tagCCcsMakeFont,
              "CCcs::MakeFont(facename=%S,charset=%d) returned %S(charset=%d)",
             fc().GetFaceNameFromAtom(pcf->_latmFaceName),
             pcf->_bCharSet,
             fc().GetFaceNameFromAtom(_latmLFFaceName),
             _bCharSet));

    return _hfont != 0;
}


/*
 *  BOOL CBaseCcs::GetFontWithMetrics (szNewFaceName)
 *
 *  @mfunc
 *      Get metrics used by the measurer and renderer and the new face name.
 */

BOOL
CBaseCcs::GetFontWithMetrics (
    HDC hdc,                              
    TCHAR* szNewFaceName,
    CODEPAGE cpDoc,
    LCID lcid )
{
#ifdef UNIX
    BOOL bWasDefault = FALSE;

    if (( _lf.lfCharSet == DEFAULT_CHARSET ) ||
        ( _lf.lfCharSet == SYMBOL_CHARSET )) {
        // On Unix we actually do sometimes map to a symbol charset
        // and somewhere in this code we don't handle that case well
        // and end up showing symbols where text should be.  I don't
        // have time to fix that right now so always ask for ansi.
        _lf.lfCharSet = ANSI_CHARSET;
        bWasDefault = TRUE;
    }
#endif

    // we want to keep _lf untouched as it is used in Compare().
    _hfont = CreateFontIndirect(&_lf);


#ifdef UNIX
    if ( bWasDefault ) {
        _lf.lfCharSet = DEFAULT_CHARSET;
    }
#endif

    if (_hfont)
    {
        // FUTURE (alexgo) if a font was not created then we may want to select
        //      a default one.
        //      If we do this, then BE SURE not to overwrite the values of _lf as
        //      it is used to match with a pcf in our Compare().
        //
        // get text metrics, in logical units, that are constant for this font,
        // regardless of the hdc in use.

        if (GetTextMetrics( hdc, cpDoc, lcid ))
        {
            HFONT hfontOld = SelectFontEx(hdc, _hfont);
            GetTextFace(hdc, LF_FACESIZE, szNewFaceName);
            SelectFontEx(hdc, hfontOld);
        }
        else
        {
            szNewFaceName[0] = 0;
        }
    }

    return (_hfont != NULL);
}

/*
 *  BOOL CBaseCcs::GetTextMetrics ( )
 *
 *  @mfunc
 *      Get metrics used by the measureer and renderer.
 *
 *  @comm
 *      These are in logical coordinates which are dependent
 *      on the mapping mode and font selected into the hdc.
 */
BOOL
CBaseCcs::GetTextMetrics(
    HDC hdc,
    CODEPAGE cpDoc,
    LCID lcid )
{
    BOOL        fRes = TRUE;
    HFONT       hfontOld;
    TEXTMETRIC  tm;

    AssertSz(_hfont, "CBaseCcs::Fill - CBaseCcs has no font");
    if ( GetCurrentObject( hdc, OBJ_FONT ) != _hfont )
    {
        hfontOld = SelectFontEx(hdc, _hfont);

        if (!hfontOld)
        {
            fRes = FALSE;
            DestroyFont();
            goto Cleanup;
        }
    }
    else
        hfontOld = 0;

    if (!::GetTextMetrics(hdc, &tm))
    {
        WHEN_DBG(GetLastError());
        fRes = FALSE;
        DeleteFontEx(_hfont);
        _hfont = NULL;
        DestroyFont();
        goto Cleanup;
    }

    // if we didn't know the true codepage, determine this now.
    if (_lf.lfCharSet == DEFAULT_CHARSET)
    {
        // BUGBUG (cthrash) Remove this.  The _sCodePage computed by MakeFont should
        // be accurate enough.
        
        _sCodePage = (USHORT)DefaultCodePageFromCharSet( tm.tmCharSet, cpDoc, lcid );
    }

    // the metrics, in logical units, dependent on the map mode and font.
    _yHeight         = (SHORT) tm.tmHeight;
    _yDescent        = _yTextMetricDescent = (SHORT) tm.tmDescent;
    _xAveCharWidth   = (SHORT) tm.tmAveCharWidth;
    _xMaxCharWidth   = (SHORT) tm.tmMaxCharWidth;
    _xOverhangAdjust = (SHORT) tm.tmOverhang;
    _sPitchAndFamily = (SHORT) tm.tmPitchAndFamily;
    _bCharSet        = tm.tmCharSet;

    if (   _bCharSet == SHIFTJIS_CHARSET
        || _bCharSet == CHINESEBIG5_CHARSET
        || _bCharSet == HANGEUL_CHARSET
        || _bCharSet == GB2312_CHARSET
       )
    {
        if (tm.tmExternalLeading == 0)
        {
            // Increase descent by 1/8 font height for FE fonts
            LONG delta = _yHeight / 8;
            _yDescent += (SHORT)delta;
            _yHeight  += (SHORT)delta;
        }
        else
        {
            //
            // use the external leading
            //

            _yDescent += (SHORT)tm.tmExternalLeading;
            _yHeight += (SHORT)tm.tmExternalLeading;
        }
    }

#ifndef UNIX // We don't support TRUE_TYPE font yet.
    // bug in windows95, synthesized italic?
    if ( _lf.lfItalic && 0 == tm.tmOverhang &&
         !(TMPF_TRUETYPE & tm.tmPitchAndFamily) &&
         !( (TMPF_DEVICE & tm.tmPitchAndFamily) &&
            (TMPF_VECTOR & tm.tmPitchAndFamily ) ) )
    {                                               // This is a *best* guess.
        // When printing to a PCL printer, we prefer zero as our overhang adjust. (41546)
        DWORD   dwDCObjType = GetObjectType(hdc);
        if (!_fPrinting
            || (dwDCObjType != OBJ_ENHMETADC && dwDCObjType != OBJ_METADC && dwDCObjType != OBJ_DC))
        {
            _xOverhangAdjust = (SHORT) (tm.tmMaxCharWidth >> 1);
        }
    }
#endif

    _xOverhang = 0;
    _xUnderhang = 0;
    if ( _lf.lfItalic )
    {
        _xOverhang =  SHORT ( (tm.tmAscent + 1) >> 2 );
        _xUnderhang =  SHORT ( (tm.tmDescent + 1) >> 2 );
    }

    // HACK (cthrash) Many Win3.1 vintage fonts (such as MS Sans Serif, Courier)
    // will claim to support all of Latin-1 when in fact it does not.  The hack
    // is to check the last character, and if the font claims that it U+2122
    // TRADEMARK, then we suspect the coverage is busted.

    _fLatin1CoverageSuspicious =    !(_sPitchAndFamily & TMPF_TRUETYPE)
                                 && (PRIMARYLANGID(LANGIDFROMLCID(g_lcidUserDefault)) == LANG_ENGLISH);

    // if fix pitch, the tm bit is clear
    _fFixPitchFont = !(TMPF_FIXED_PITCH & tm.tmPitchAndFamily);
    _xDefDBCWidth = 0;

    if (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID)
    {
        // Hack imported from Word via Riched.  This is how they compute the
        // discrepancy between the return values of GetCharWidthA and
        // GetCharWidthW.   Compute it once per CBaseCcs, so we don't have to
        // recompute on every Include call.

        const TCHAR chX = _T('X');
        SIZE size;
        INT dxX;

        GetTextExtentPoint(hdc, &chX, 1, &size);
        GetCharWidthA(hdc, chX, chX, &dxX);

        _sAdjustFor95Hack = size.cx - dxX;

        _fLatin1CoverageSuspicious &= (tm.tmLastChar == 0xFF);
    }
    else
    {
        _sAdjustFor95Hack = 0;

        _fLatin1CoverageSuspicious &= (tm.tmLastChar == 0x2122);
    }

    if (_bCharSet == SYMBOL_CHARSET)
    {
        // Must use doc codepage, unless of course we have a Unicode document
        // In this event, we pick cp1252, just to maximize coverage.

        _sCodePage = IsStraightToUnicodeCodePage(cpDoc) ? CP_1252 : cpDoc;

        _bConvertMode = CM_SYMBOL;        
    }
    else if (IsExtTextOutWBuggy( _sCodePage ))
    {
        _bConvertMode = CM_MULTIBYTE;
    }
#if !defined(WIN16) && !defined(WINCE)
    else if (_bConvertMode == CM_NONE &&
         VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID)
    {
        // Some fonts have problems under Win95 with the GetCharWidthW call;
        // this is a simple heuristic to determine if this problem exists.

        INT     widthA, widthW;
        BOOL    fResA, fResW;

        // Future(BradO):  We should add the expression
        //  "&& IsFELCID(GetSystemDefaultLCID())" to the 'if' below to use
        //  Unicode GetCharWidth and ExtTextOut for FE fonts on non-FE
        //  systems (see postponed bug #3337).

        // Yet another hack - FE font on Non-FE Win95 cannot use
        // GetCharWidthW and ExtTextOutW
        if (FEFontOnNonFE95(tm.tmCharSet))
        {
            // always use ANSI call for DBC fonts.
            _bConvertMode = CM_FEONNONFE;

            // setup _xDefDBWidth to by-pass some Trad. Chinese character
            // width problem.
            if (CHINESEBIG5_CHARSET == tm.tmCharSet)
            {
                BYTE    ansiChar[2] = {0xD8, 0xB5 };

                fResA = GetCharWidthA( hdc, *((USHORT *) ansiChar),
                                       *((USHORT *) ansiChar), &widthA );
                if (fResA && widthA)
                {
                    _xDefDBCWidth = (SHORT)widthA;
                }
            }
        }
        else
        {
            fResA = GetCharWidthA( hdc, ' ', ' ', &widthA );
            fResW = GetCharWidthW( hdc, L' ', L' ', &widthW );
            if ( fResA && fResW && widthA != widthW )
            {
                _bConvertMode = CM_MULTIBYTE;
            }
            else
            {
                fResA = GetCharWidthA( hdc, 'a', 'a', &widthA );
                fResW = GetCharWidthW( hdc, L'a', L'a', &widthW );
                if ( fResA && fResW && widthA != widthW )
                {
                    _bConvertMode = CM_MULTIBYTE;
                }
            }
        }
    }
#endif // !WIN16 && !WINCE

Cleanup:

    if (hfontOld)
        SelectFontEx(hdc, hfontOld);

    return fRes;
}


/*
 *  CBaseCcs::NeedConvertNBSPs
 *
 *  @mfunc
 *      Determine NBSPs need conversion or not during render
 *      Some fonts wont render NBSPs correctly.
 *      Flag fonts which have this problem.
 *
 */
BOOL
CBaseCcs::NeedConvertNBSPs(HDC hdc, CDoc *pDoc)
{
    HFONT   hfontOld;

    Assert(!_fConvertNBSPsSet);

    if ( GetCurrentObject( hdc, OBJ_FONT ) != _hfont )
    {
        hfontOld = SelectFontEx(hdc, _hfont);

        if (!hfontOld)
            goto Cleanup;
    }
    else
    {
        hfontOld = 0;
    }

    // BUGBUG (cthrash) Once ExtTextOutW is supported in CRenderer, we need
    //                  to set _fConvertNBSPsIfA, so that we can better tune when we need to
    //                  convert NBSPs depending on which ExtTextOut variant we call.

#if !defined(WIN16) && !defined(WINCE)
    if (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID)
    {
        extern BOOL IsATMInstalled();

        Assert(pDoc);
        if (   pDoc
            && pDoc->IsPrintDoc()
            && IsATMInstalled()
           )
        {
            _fConvertNBSPs = FALSE;
        }
        else
        {
            TCHAR   ch = WCH_NBSP;
            char    b;
            BOOL    fUsedDefChar;

            WideCharToMultiByte(_sCodePage, 0, &ch, 1, &b, 1, NULL, &fUsedDefChar);

            if (fUsedDefChar)
            {
                _fConvertNBSPs = TRUE;
            }
            else
            {
                // Some fonts (like Wide Latin) claim the width of spaces and
                // NBSP chars are the same, but when you actually call ExtTextOut,
                // you'll get fatter spaces;

                ABC abcSpace, abcNbsp;

                _fConvertNBSPs = !GetCharABCWidthsA( hdc, ' ', ' ', &abcSpace ) ||
                                 !GetCharABCWidthsA( hdc, b, b, &abcNbsp ) ||
                                 abcSpace.abcA != abcNbsp.abcA ||
                                 abcSpace.abcB != abcNbsp.abcB ||
                                 abcSpace.abcC != abcNbsp.abcC;
            }
        }
    }
    else
    {
#ifndef UNIX // UNIX doesn't have true-type fonts
        ABC abcSpace, abcNbsp;

        _fConvertNBSPs = !GetCharABCWidthsW( hdc, L' ', L' ', &abcSpace ) ||
                         !GetCharABCWidthsW( hdc, WCH_NBSP, WCH_NBSP, &abcNbsp ) ||
                         abcSpace.abcA != abcNbsp.abcA ||
                         abcSpace.abcB != abcNbsp.abcB ||
                         abcSpace.abcC != abcNbsp.abcC;
#else // UNIX
        int lSpace, lNbsp;

        _fConvertNBSPs = !GetCharWidthW( hdc, L' ', L' ', &lSpace ) ||
                         !GetCharWidthW( hdc, WCH_NBSP, WCH_NBSP, &lNbsp ) ||
                         lSpace != lNbsp;
#endif
    }
#endif // !WIN16 && !WINCE

Cleanup:

    if (hfontOld)
    {
        SelectFontEx(hdc, hfontOld);
    }

    _fConvertNBSPsSet = TRUE;
    return TRUE;
}


/*
 *  CBaseCcs::DestroyFont
 *
 *  @mfunc
 *      Destroy font handle for this CBaseCcs
 *
 */

void
CBaseCcs::DestroyFont()
{
    // clear out any old font
    if (_hfont)
    {
        DeleteFontEx(_hfont);
        _hfont = 0;
    }

    // make sure the script cache is freed
    if (_sc)
    {
        ::ScriptFreeCache(&_sc);
        // NB (mikejoch) If ScriptFreeCache() fails then there is no way to
        // free the cache, so we'll end up leaking it. This shouldn't ever
        // happen since the only way for _sc to be non- NULL is via some other
        // USP function succeeding.
    }

    Assert(_sc == NULL);
}

/*
 *  CBaseCcs::Compare (pcf, lfHeight, CBaseCcs * pBaseBaseCcs )
 *
 *  @mfunc
 *      Compares this font cache with the font properties of a
 *      given CHARFORMAT
 *
 *  @rdesc
 *      FALSE iff did not match exactly.
 */

BOOL
CBaseCcs::Compare( CompareArgs * pCompareArgs )
{
    VerifyLFAtom();
    // NB: We no longer create our logical font with an underline and strike through.
    // We draw strike through & underline separately.

    // If are mode is CM_MULTIBYTE, we need the sid to match exactly, otherwise we
    // will not render correctly.  For example, <FONT FACE=Arial>A&#936; will have two
    // text runs, first sidAsciiLatin, second sidCyrillic.  If are conversion mode is
    // multibyte, we need to make two fonts, one with ANSI_CHARSET, the other with
    // RUSSIAN_CHARSET.
    const CCharFormat * pcf = pCompareArgs->pcf;

    BOOL fMatched =    (_yCfHeight == pcf->_yHeight) // because different mapping modes
                    && (_lf.lfWeight == (pcf->_fBold ? FW_BOLD : FW_NORMAL))
                    && (_latmLFFaceName == pcf->_latmFaceName)
                    && (_lf.lfItalic == pcf->_fItalic)
                    && (_lf.lfHeight == pCompareArgs->lfHeight)  // have diff logical coords
                    && (   pcf->_bCharSet == DEFAULT_CHARSET
                        || _bCharSet == DEFAULT_CHARSET
                        || pcf->_bCharSet == _bCharSet)
                    && (_bPitchAndFamily == pcf->_bPitchAndFamily);

    WHEN_DBG( if (!fMatched) )
    {
        TraceTag((tagCCcsCompare,
                  "%s%s%s%s%s%s%s",
                  (_yCfHeight == pcf->_yHeight) ? "" : "height ",
                  (_lf.lfWeight == (pcf->_fBold ? FW_BOLD : FW_NORMAL)) ? "" : "weight ",
                  (_latmLFFaceName == pcf->_latmFaceName) ? "" : "facename ",
                  (_lf.lfItalic == pcf->_fItalic) ? "" : "italicness ",
                  (_lf.lfHeight == pCompareArgs->lfHeight) ? "" : "logical-height ",
                  (   pcf->_bCharSet == DEFAULT_CHARSET
                   || _bCharSet == DEFAULT_CHARSET
                   || pcf->_bCharSet == _bCharSet) ? "" : "charset ",
                  (_lf.lfPitchAndFamily == pcf->_bPitchAndFamily) ? "" : "pitch&family" ));
    }

    return fMatched;
}

BOOL
CBaseCcs::CompareForFontLink( CompareArgs * pCompareArgs )
{
    // The difference between CBaseCcs::Compare and CBaseCcs::CompareForFontLink is in it's
    // treatment of adjusted/pre-adjusted heights.  If we ask to fontlink with MS Mincho
    // in the middle of 10px Arial text, we may choose to instantiate an 11px MS Mincho to
    // compenstate for the ascent/descent discrepancies.  10px is the _yOriginalHeight, and
    // 11px is the _lf.lfHeight in this scenario.  If we again ask for 10px MS Mincho while
    // fontlinking Arial, we want to match based on the original height, not the adjust height.
    // CBaseCcs::Compare, on the other hand, is only concerned with the adjusted height.

    VerifyLFAtom();
    // NB: We no longer create our logical font with an underline and strike through.
    // We draw strike through & underline separately.

    // If are mode is CM_MULTIBYTE, we need the sid to match exactly, otherwise we
    // will not render correctly.  For example, <FONT FACE=Arial>A&#936; will have two
    // text runs, first sidAsciiLatin, second sidCyrillic.  If are conversion mode is
    // multibyte, we need to make two fonts, one with ANSI_CHARSET, the other with
    // RUSSIAN_CHARSET.
    const CCharFormat * pcf = pCompareArgs->pcf;

    BOOL fMatched =    (_yCfHeight == pcf->_yHeight) // because different mapping modes
                    && (_lf.lfWeight == (pcf->_fBold ? FW_BOLD : FW_NORMAL))
                    && (_latmLFFaceName == pcf->_latmFaceName)
                    && (_latmBaseFaceName == pCompareArgs->latmBaseFaceName)
                    && (_lf.lfItalic == pcf->_fItalic)
                    && (_yOriginalHeight == pCompareArgs->lfHeight)  // have diff logical coords
                    && (   pcf->_bCharSet == DEFAULT_CHARSET
                        || _bCharSet == DEFAULT_CHARSET
                        || pcf->_bCharSet == _bCharSet)
                    && (_bPitchAndFamily == pcf->_bPitchAndFamily);

    WHEN_DBG( if (!fMatched) )
    {
        TraceTag((tagCCcsCompare,
                  "%s%s%s%s%s%s%s",
                  (_yCfHeight == pcf->_yHeight) ? "" : "height ",
                  (_lf.lfWeight == (pcf->_fBold ? FW_BOLD : FW_NORMAL)) ? "" : "weight ",
                  (_latmLFFaceName == pcf->_latmFaceName) ? "" : "facename ",
                  (_lf.lfItalic == pcf->_fItalic) ? "" : "italicness ",
                  (_yOriginalHeight == pCompareArgs->lfHeight) ? "" : "logical-height ",
                  (   pcf->_bCharSet == DEFAULT_CHARSET
                   || _bCharSet == DEFAULT_CHARSET
                   || pcf->_bCharSet == _bCharSet) ? "" : "charset ",
                  (_lf.lfPitchAndFamily == pcf->_bPitchAndFamily) ? "" : "pitch&family" ));
    }

    return fMatched;
}

void CBaseCcs::SetLFFaceNameAtm(LONG latmFaceName)
{
    VerifyLFAtom();
    _latmLFFaceName= latmFaceName;
    // Sets the string inside _lf to what _latmLFFaceName represents.
    _tcsncpy(_lf.lfFaceName, fc().GetFaceNameFromAtom(_latmLFFaceName), LF_FACESIZE );
}

void CBaseCcs::SetLFFaceName(const TCHAR * szFaceName)
{
    VerifyLFAtom();
    _latmLFFaceName= fc().GetAtomFromFaceName(szFaceName);
    _tcsncpy(_lf.lfFaceName, szFaceName, LF_FACESIZE );
}


CONVERTMODE
CBaseCcs::GetConvertMode(
    BOOL fEnhancedMetafile,
    BOOL fMetafile )
{
#ifdef WIN16
    //BUGWIN16: What are the conditions under win16?

    return CM_NONE;
#else
#if DBG==1
    if (IsTagEnabled(tagTextOutA))
    {
        if (IsTagEnabled(tagTextOutFE))
        {
            TraceTag((tagTextOutFE,
                      "tagTextOutFE is being ignored "
                      "(tagTextOutA is set.)"));
        }

        return CM_MULTIBYTE;
    }
    else if (IsTagEnabled(tagTextOutFE))
    {
        return CM_FEONNONFE;
    }
#endif

    CONVERTMODE cm = (CONVERTMODE)_bConvertMode;

    // For hack around ExtTextOutW Win95 FE problems.
    // NB (cthrash) The following is an enumeration of conditions under which
    // ExtTextOutW is broken.  This code is originally from RichEdit.

    if (cm == CM_MULTIBYTE)
    {
        // If we want CM_MULTIBYTE, that's what we get.
    }
    else if (g_fFarEastWin9X || _fFEFontOnNonFEWin95)
    {
        // Ultimately call ReExtTextOutW, unless symbol.
        // If symbol, call ExtTextOutA.

        if (cm != CM_SYMBOL)
        {
            if (   IsExtTextOutWBuggy( _sCodePage )
                && IsFECharset( _lf.lfCharSet ) )
            {
                // CHT ExtTextOutW does not work on Win95 Golden on
                // many characters (cthrash).

                cm = CM_MULTIBYTE;
            }
            else
            {
                cm = CM_FEONNONFE;
            }
        }
    }
    else
    {
        if (CM_SYMBOL != cm)
        {
#if NOTYET
            if (fEnhancedMetafile &&
                ((VER_PLATFORM_WIN32_WINDOWS   == g_dwPlatformID) ||
                 (VER_PLATFORM_WIN32_MACINTOSH == g_dwPlatformID)))
#else
                if (fEnhancedMetafile &&
                    (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID))
#endif
                {
                    cm = CM_MULTIBYTE;
                }
                else if (fMetafile && g_fFarEastWinNT)
                {
                    // FE NT metafile ExtTextOutW hack.
                    cm = CM_MULTIBYTE;
                }
        }
        if ((VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID) &&
            fMetafile && !fEnhancedMetafile )
        {
            //Win95 can't handle TextOutW to regular metafiles
            cm = CM_MULTIBYTE;
        }
    }

    return cm;
#endif // WIN16
}

#if !defined(WIN16) && !defined(WINCE)
// This function uses GetCharacterPlacement to get character widths. This
// This works better than GetCharWidths() under Win95.
static BOOL
GetWin95CharWidth(
    HDC hdc,
    UINT iFirstChar,
    UINT iLastChar,
    int *pWidths)
{
    UINT i, start;
    TCHAR chars[257];
    GCP_RESULTS gcp={sizeof(GCP_RESULTS), NULL, NULL, pWidths,
    NULL, NULL, NULL, 128, 0};

    // Avoid overflows.
    Assert (iLastChar - iFirstChar < 128);

    // We want to get all the widths from 0 to 255 to stay in sync with our
    // width cache. Unfortunately, the zeroeth character is a zero, which
    // is the string terminator, which prevents any of the other characters
    // from getting the correct with. Since we can't use it anyway, we get the
    // width of a space character for zero.
    start=iFirstChar;
    if (iFirstChar == 0)
    {
        chars[0] = (TCHAR)' ';
        start++;
    }

    // Fill up our "string" with
    for (i=start; i<=iLastChar; i++)
    {
        chars[i-iFirstChar] = (TCHAR)i;
    }
    // Null terminate because I'm suspicious of GetCharacterPlacement's handling
    // of NULL characters.
    // chars[i-iFirstChar] = 0;

    gcp.nGlyphs = iLastChar - iFirstChar + 1;

    return !!GetCharacterPlacement(hdc, chars, iLastChar-iFirstChar + 1, 0, &gcp, 0);
}
#endif // !WIN16 && !WINCE

/*
 *  CBaseCcs::FillWidths (ch, rlWidth)
 *
 *  @mfunc
 *      Fill in this CBaseCcs with metrics info for given device
 *
 *  @rdesc
 *      TRUE if OK, FALSE if failed
 */
BOOL
CBaseCcs::FillWidths(
    HDC hdc,
    TCHAR ch,       //@parm The TCHAR character we need a width for.
    LONG &rlWidth)  //@parm the width of the character
{
    BOOL  fRes = FALSE;
    HFONT hfontOld;

    hfontOld = PushFont(hdc);

    // fill up the width info.
    fRes = _widths.FillWidth( hdc, this, ch, rlWidth );

    PopFont(hdc, hfontOld);

    return fRes;
}


// Selects the instance var _hfont so we can measure, but remembers the old
// font so we don't actually change anything when we're done measuring.
// Returns the previously selected font.
HFONT
CBaseCcs::PushFont(HDC hdc)
{
    HFONT hfontOld;

    AssertSz( _hfont, "CBaseCcs has no font");

    //  The mapping mode for the HDC is set before we get here.
    hfontOld = (HFONT)GetCurrentObject( hdc, OBJ_FONT );
    Assert( hfontOld != NULL );  // Otherwise using NULL is invalid.
    if ( hfontOld != _hfont )
    {
        WHEN_DBG( HFONT hfontReturn = )
            SelectFontEx( hdc, _hfont);
        AssertSz( hfontReturn == hfontOld, "GDI failure changing fonts" );
    }
    return hfontOld;
}


// Restores the selected font from before PushFont.
// This is really just a lot like SelectFont, but is optimized, and
// will only work if PushFont was called before it.
void
CBaseCcs::PopFont(HDC hdc, HFONT hfontOld)
{
    // This assert will fail if Pushfont was not called before popfont,
    // Or if somebody else changes fonts in between.  (They shouldn't.)
    Assert( _hfont == (HFONT) GetCurrentObject(hdc, OBJ_FONT) );
#ifndef _WIN64
//$ Win64: GetObjectType is returning zero on Axp64
    Assert( OBJ_FONT == GetObjectType( hfontOld ) );
#endif

    if( hfontOld != _hfont)
    {
        WHEN_DBG( HFONT hfontReturn = )
            SelectFontEx(hdc, hfontOld);
        AssertSz( hfontReturn == _hfont, "GDI failure changing fonts" );
    }
}


// Goes into a critical section, and allocates memory for a width cache.
void
CWidthCache::ThreadSafeCacheAlloc(void** ppCache, size_t iSize)
{
    EnterCriticalSection(&(fc()._csOther));

    if (!*ppCache)
    {
        *ppCache = MemAllocClear( Mt(CWidthCacheEntry), iSize );
    }

    LeaveCriticalSection(&(fc()._csOther));

}


// Fills in the widths of the low 128 characters.
// Allocates memory for the block if needed.
BOOL
CWidthCache::PopulateFastWidthCache(HDC hdc, CBaseCcs* pBaseCcs)
{
    BOOL  fRes;
    HFONT hfontOld;
    
    // First switch to the appropriate font.
    hfontOld = pBaseCcs->PushFont(hdc);

    // characters in 0 - 127 range (cache 0), so initialize the character widths for
    // for all of them.
    int widths[ FAST_WIDTH_CACHE_SIZE ];
    int i;

#if !defined(WINCE) && !defined(UNIX)
    // If this is Win95 and this is not a true type font
    // GetCharWidth*() returns unreliable results, so we do something
    // slow, painful, but accurate.
#if defined(WIN16) || defined(_MAC)
    fRes = GetCharWidth(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
    // ## v-gsrir
    // Overhang adjustments for non-truetype fonts in Win16
    if (pBaseCcs->_xOverhangAdjust)
        if (fRes)
        {
            for (i=0; i< FAST_WIDTH_CACHE_SIZE; i++)
                widths[i] -= pBaseCcs->_xOverhangAdjust;
        }

#else
    if (!g_fUnicodePlatform)
    {
        LONG lfAbsHeight = abs(pBaseCcs->_lf.lfHeight);

        // COMPLEXSCRIPT - With so many nonsupported charsets, why not avoid using GCP?
        //                 GCP does not work with any of the languages that need it.
        // HACK (cthrash) If the absolute height is too small, the return values
        // from GDI are unreliable.  We might as well use the GetCharWidthA values,
        // which aren't stellar either, but empirically better.
        if (lfAbsHeight > 3 &&
            !(pBaseCcs->_sPitchAndFamily & TMPF_VECTOR) &&
            pBaseCcs->_bCharSet != SYMBOL_CHARSET &&
            pBaseCcs->_bCharSet != ARABIC_CHARSET &&
            pBaseCcs->_bCharSet != HEBREW_CHARSET &&
            pBaseCcs->_bCharSet != VIETNAMESE_CHARSET &&
            pBaseCcs->_bCharSet != THAI_CHARSET)
        {
            fRes = GetWin95CharWidth(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
        }
        else
        {
            fRes = GetCharWidthA(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
            if (fRes)
            {
                for (i=0; i< FAST_WIDTH_CACHE_SIZE; i++)
                    widths[i] -= pBaseCcs->_xOverhangAdjust;
            }
        }
    }
    else
#endif // !WIN16
#endif // !WINCE && !UNIX

    {
#ifdef WIN16
        fRes = GetCharWidth(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
#else
        fRes = GetCharWidth32(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
#endif
    }

    // Copy the results back into the real cache, if it worked.
    if (fRes)
    {
        Assert( !_pFastWidthCache );  // Since we should only populate this once.

        ThreadSafeCacheAlloc( (void **)&_pFastWidthCache, sizeof(CharWidth) * FAST_WIDTH_CACHE_SIZE );

        if( !_pFastWidthCache )
        {
            // We're kinda screwed if we can't get memory for the cache.
            AssertSz(0,"Failed to allocate fast width cache.");
            fRes = FALSE;
            goto Cleanup;
        }

        for(i = 0; i < FAST_WIDTH_CACHE_SIZE; i++)
        {
            Assert( widths[i] <= MAXSHORT );  // since there isn't a MAXUSHORT
            _pFastWidthCache[i]= (widths[i]) ? widths[i] : pBaseCcs->_xAveCharWidth;
        }

        // NB (cthrash) Measure NBSPs with space widths.
        SetCacheEntry(WCH_NBSP, _pFastWidthCache[_T(' ')] );
    }

Cleanup:
    pBaseCcs->PopFont(hdc, hfontOld);

    return fRes;
}  // PopulateFastWidthCache


// =========================  WidthCache by jonmat  =========================
/*
 *  CWidthCache::FillWidth(hdc, ch, xOverhang, rlWidth)
 *
 *  @mfunc
 *      Call GetCharWidth() to obtain the width of the given char.
 *
 *  @comm
 *      The HDC must be setup with the mapping mode and proper font
 *      selected *before* calling this routine.
 *
 *  @rdesc
 *      Returns TRUE if we were able to obtain the widths.
 *
 */
BOOL
CWidthCache::FillWidth (
    HDC         hdc,
    CBaseCcs   *pBaseCcs,      //@parm CCcs object
    const TCHAR ch,            //@parm Char to obtain width for
    LONG       &rlWidth )      //@parm Width of character
{
    BOOL    fRes;
    INT     numOfDBCS = 0;
    CacheEntry  widthData;
    
    // BUGBUG GetCharWidthW is really broken for bullet on Win95J. Sometimes it will return
    // a width or 0 or 1198 or ...So, hack around it. Yuk!
    // Also, WideCharToMultiByte() on Win95J will NOT convert bullet either.

    Assert( !IsCharFast(ch) );  // This code shouldn't be called for that.

    if ( ch == WCH_NBSP )
    {
        // Use the width of a space for an NBSP
        fRes = pBaseCcs->Include(hdc, L' ', rlWidth);

        if( !fRes )
        {
            // Error condition, just use the default width.
            rlWidth = pBaseCcs->_xAveCharWidth;
        }

        widthData.ch = ch;
        widthData.width = rlWidth;
        *GetEntry(ch) = widthData;
    }
    else
    {
        INT xWidth = 0;

        // Diacritics, tone marks, and the like have 0 width so return 0 if
        // GetCharWidthW() succeeds.
        BOOL fZeroWidth = IsZeroWidth(ch);


#ifdef WIN16
        // when IME is present for win16, g_fUnicodePlatform might be available
        BOOL fEUDCFixup = IsEUDCChar(ch);
#else
        BOOL fEUDCFixup = g_dwPlatformVersion < 0x40000 && IsEUDCChar(ch);
#endif
        if ( pBaseCcs->_bConvertMode == CM_SYMBOL )
        {
            TCHAR chT = ch;
            
#ifndef UNIX
            if (chT > 255)
            {
                const BYTE b = InWindows1252ButNotInLatin1(ch);
            
                if (b)
                {
                    chT = b;
                }
            }
#endif

            fRes = chT > 255 ? FALSE : GetCharWidthA( hdc, chT, chT, &xWidth );
        }
        else if ( !fEUDCFixup && pBaseCcs->_bConvertMode != CM_MULTIBYTE )
        {
            // GetCharWidthW will crash on a 0xffff.
            Assert(ch != 0xffff);
            fRes = GetCharWidthW( hdc, ch, ch, &xWidth );


            // See comment in CBaseCcs::GetTextMetrics

            xWidth += pBaseCcs->_sAdjustFor95Hack;
        }
        else
        {
            fRes = FALSE;
        }

        // either fAnsi case or GetCharWidthW fail, let's try GetCharWidthA
#ifndef UNIX
        if (!fRes || (0 == xWidth && !fZeroWidth))
#else // It's possible on UNIX with charWidth=0.
        if (!fRes)
#endif
        {
            WORD wDBCS;
            char ansiChar[2] = {0};
            UINT uCP = fEUDCFixup ? CP_ACP : pBaseCcs->_sCodePage;

            // Convert string
            numOfDBCS = WideCharToMultiByte( uCP, 0, &ch, 1,
                                             ansiChar, 2, NULL, NULL);

            if (2 == numOfDBCS)
                wDBCS = (BYTE)ansiChar[0] << 8 | (BYTE)ansiChar[1];
            else
                wDBCS = (BYTE)ansiChar[0];

            fRes = GetCharWidthA( hdc, wDBCS, wDBCS, &xWidth );
        }

        widthData.width = (USHORT)xWidth;

        if ( fRes )
        {
#ifndef UNIX // On Unix , charWidth == 0 is not a bug.
            if (0 == widthData.width && !fZeroWidth)
            {
                // Sometimes GetCharWidth will return a zero length for small
                // characters. When this happens we will use the default width
                // for the font if that is non-zero otherwise we just us 1
                // because this is the smallest valid value.

                // BUGBUG - under Win95 Trad. Chinese, there is a bug in the
                // font. It is returning a width of 0 for a few characters
                // (Eg 0x09F8D, 0x81E8) In such case, we need to use 2 *
                // pBaseCcs->_xAveCharWidth since these are DBCS

                if (0 == pBaseCcs->_xAveCharWidth)
                {
                    widthData.width = 1;
                }
                else
                {
                    widthData.width = (numOfDBCS == 2)
                                      ? (pBaseCcs->_xDefDBCWidth
                                         ? pBaseCcs->_xDefDBCWidth
                                         : 2 * pBaseCcs->_xAveCharWidth)
                                      : pBaseCcs->_xAveCharWidth;
                } 
            }
#endif
            widthData.ch      = ch;
            if (widthData.width <= pBaseCcs->_xOverhangAdjust)
                widthData.width = 1;
            else
                widthData.width   -= pBaseCcs->_xOverhangAdjust;
            rlWidth = widthData.width;
            *GetEntry(ch) = widthData;
        }
        //       else
        //            rlWidth = pBaseCcs->_xAveCharWidth;
    }

    AssertSz(fRes, "no width?");

    Assert( widthData.width == rlWidth );  // Did we forget to set it?

    return fRes;
}

/*
 *  CWidthCache::~CWidthCache()
 *
 *  @mfunc
 *      Free any allocated caches.
 *
 */

CWidthCache::~CWidthCache()
{
    INT i;

    for (i = 0; i < TOTALCACHES; i++ )
    {
        if (_pWidthCache[i])
            MemFree(_pWidthCache[i]);
    }
    MemFree(_pFastWidthCache);
}

// BUGBUG (cthrash) This needs to be removed as soon as the FontLinkTextOut
// is cleaned up for complex scripts.

void
CBaseCcs::EnsureLangBits(HDC hdc)
{
    if (!_dwLangBits)
    {
        // Get the charsets supported by this font.
        if (_bCharSet != SYMBOL_CHARSET)
        {
            _dwLangBits = GetFontScriptBits( hdc,
                                             fc().GetFaceNameFromAtom( _latmLFFaceName ),
                                             &_lf );
        }
        else
        {
            // See comment in GetFontScriptBits.
            // SBITS_ALLLANGS means _never_ fontlink.

            _dwLangBits = SBITS_ALLLANGS;
        }
    }
}

BYTE
InWindows1252ButNotInLatin1Helper(WCHAR ch)
{
    for (int i=32;i--;)
    {
        if (ch == g_achLatin1MappingInUnicodeControlArea[i])
        {
            return 0x80 + i;
        }
    }

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::FixupForFontLink
//
//  Purpose:    Optionally scale a font height when fontlinking.
//
//              This code was borrowed from UniScribe (usp10\str_ana.cxx)
//
//              Let's say you're base font is a 10pt Tahoma, and you need
//              to substitute a Chinese font (e.g. MingLiU) for some ideo-
//              graphic characters.  When you simply ask for a 10pt MingLiU,
//              you'll get a visibly smaller font, due to the difference in
//              in distrubution of the ascenders/descenders.  The purpose of
//              this function is to examine the discrepancy and pick a
//              slightly larger or smaller font for improved legibility. We
//              may also adjust the baseline by 1 pixel.
//              
//-----------------------------------------------------------------------------

void
CBaseCcs::FixupForFontLink(
    HDC hdc,
    CBaseCcs * pBaseBaseCcs )
{
    int iOriginalDescender = pBaseBaseCcs->_yDescent;
    int iFallbackDescender = _yDescent;
    int iOriginalAscender  = pBaseBaseCcs->_yHeight - iOriginalDescender;
    int iFallbackAscender  = _yHeight - iFallbackDescender;

    if (   iFallbackAscender  > 0
        && iFallbackDescender > 0)
    {
        int iAscenderRatio  = 1024 * iOriginalAscender  / iFallbackAscender;
        int iDescenderRatio = 1024 * iOriginalDescender / iFallbackDescender;
        int iNewRatio;
        SHORT yDescentAdjust = 0;

        if (iAscenderRatio != iDescenderRatio)
        {

            // We'll allow moving the baseline by one pixel to reduce the amount of
            // scaling required.

            if (iAscenderRatio < iDescenderRatio)
            {
                // Clipping, if any, would happen in the ascender.

                yDescentAdjust = +1;
                iOriginalAscender++;    // Move the baseline down one pixel.
                iOriginalDescender--;
                TraceTag((tagMetrics, "Moving baseline down one pixel to leave more room for ascender"));
            }
            else
            {
                // Clipping, if any, would happen in the descender.

                yDescentAdjust = -1;
                iOriginalAscender--;    // Move the baseline up one pixel.
                iOriginalDescender++;
                TraceTag((tagMetrics, "Moving baseline up one pixel to leave more room for descender"));
            }


            // Recalculate ascender and descender ratios based on shifted baseline

            iAscenderRatio  = 1024 * iOriginalAscender  / iFallbackAscender;
            iDescenderRatio = 1024 * iOriginalDescender / iFallbackDescender;
        }

        // Establish extent of worst mismatch, either too big or too small

		iNewRatio = iAscenderRatio;
		iNewRatio = max(iNewRatio, 768); // Never reduce size by over 25%

        if (iNewRatio < 1000 || iNewRatio > 1048)
        {
            LONG  lfHeightCurrent = _lf.lfHeight;
            LONG  lAdjust = (iNewRatio < 1024) ? 1023 : 0; // round towards 100% (1024)
            LONG  lfHeightNew = (lfHeightCurrent * iNewRatio - lAdjust) / 1024;

            Assert( lfHeightCurrent < 0 );  // lfHeight should be negative; otherwise rounding will be incorrect

            if (lfHeightNew != lfHeightCurrent)
            {
                SHORT yHeightCurrent = _yHeight;
                SHORT sCodePageCurrent = _sCodePage;
                TCHAR achNewFaceName[LF_FACESIZE];
                HFONT hfontCurrent = _hfont;

                // Reselect with new ratio

                TraceTag((tagMetrics, "Reselecting fallback font to improve legibility"));
                TraceTag((tagMetrics, " Original font ascender %4d, descender %4d, lfHeight %4d, \'%S\'",
                          iOriginalAscender, iOriginalDescender, pBaseBaseCcs->_yHeight, fc().GetFaceNameFromAtom(pBaseBaseCcs->_latmLFFaceName)));
                TraceTag((tagMetrics, " Fallback font ascender %4d, descender %4d, -> lfHeight %4d, \'%s\'",
                          iFallbackAscender, iFallbackDescender, _yHeight * iNewRatio / 1024, fc().GetFaceNameFromAtom(_latmLFFaceName)));
				
                _lf.lfHeight = lfHeightNew;

                if (GetFontWithMetrics(hdc, achNewFaceName, CP_UCS_2, 0))
                {
                    DeleteFontEx(hfontCurrent);

					_yDescent += yDescentAdjust;
                }
                else
                {
                    Assert(_hfont == NULL);

                    _lf.lfHeight = lfHeightCurrent;
                    _yHeight = yHeightCurrent;
                    _hfont = hfontCurrent;
                }

                _sCodePage = sCodePageCurrent;
            }
        }
    }

    _fHeightAdjustedForFontlinking = TRUE;
}
