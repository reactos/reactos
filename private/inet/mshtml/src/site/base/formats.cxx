//+ ---------------------------------------------------------------------------
//
//  File:       formats.cxx
//
//  Contents:   ComputeFormats and associated utilities
//
//  Classes:
//
// ----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FCACHE_HXX_
#define X_FCACHE_HXX_
#include "fcache.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_CURSTYLE_HXX_
#define X_CURSTYLE_HXX_
#include "curstyle.hxx"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_RECALC_H_
#define X_RECALC_H_
#include "recalc.h"
#endif

#ifndef X_RECALC_HXX_
#define X_RECALC_HXX_
#include "recalc.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif


// Debugging ------------------------------------------------------------------

DeclareTag(tagMetrics, "Metrics", "Metrics");
DeclareTag(tagFormatCaches, "FormatCaches", "Trace format caching");
ExternTag(tagRecalcStyle);

PerfDbgExtern(tagDataCacheDisable);
MtDefine(THREADSTATE_pCharFormatCache, THREADSTATE, "THREADSTATE::_pCharFormatCache")
MtDefine(THREADSTATE_pParaFormatCache, THREADSTATE, "THREADSTATE::_pParaFormatCache")
MtDefine(THREADSTATE_pFancyFormatCache, THREADSTATE, "THREADSTATE::_pFancyFormatCache")
MtDefine(THREADSTATE_pStyleExpandoCache, THREADSTATE, "THREADSTATE::_pStyleExpandoCache")
MtDefine(CStyleSheetHref, CStyleSheet, "CStyleSheet::_achAbsoluteHref")
MtDefine(ComputeFormats, Metrics, "ComputeFormats")
MtDefine(CharFormatAddRef, ComputeFormats, "CharFormat simple AddRef")
MtDefine(CharFormatTouched, ComputeFormats, "CharFormat touched needlessly")
MtDefine(CharFormatCached, ComputeFormats, "CharFormat cached")
MtDefine(ParaFormatAddRef, ComputeFormats, "ParaFormat simple AddRef")
MtDefine(ParaFormatTouched, ComputeFormats, "ParaFormat touched needlessly")
MtDefine(ParaFormatCached, ComputeFormats, "ParaFormat cached")
MtDefine(FancyFormatAddRef, ComputeFormats, "FancyFormat simple AddRef")
MtDefine(FancyFormatTouched, ComputeFormats, "FancyFormat touched needlessly")
MtDefine(FancyFormatCached, ComputeFormats, "FancyFormat cached")
MtDefine(StyleExpandoCached, ComputeFormats, "StyleExpando cached")
MtDefine(StyleExpandoAddRef, ComputeFormats, "StyleExpando simple AddRef")

// Globals --------------------------------------------------------------------

CCharFormat     g_cfStock;
CParaFormat     g_pfStock;
CFancyFormat    g_ffStock;
BOOL            g_fStockFormatsInitialized = FALSE;

#define MAX_FORMAT_INDEX 0x7FFF

// Thread Init + Deinit -------------------------------------------------------

void
DeinitFormatCache (THREADSTATE * pts)
{
#if DBG == 1
    if(pts->_pCharFormatCache)
    {
        TraceTag((tagMetrics, "Format Metrics:"));

        TraceTag((tagMetrics, "\tSize of formats char:%ld para:%ld fancy: %ld",
                                sizeof (CCharFormat),
                                sizeof (CParaFormat),
                                sizeof (CFancyFormat)));

        TraceTag((tagMetrics, "\tMax char format cache entries: %ld",
                                pts->_pCharFormatCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax para format cache entries: %ld",
                                pts->_pParaFormatCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax fancy format cache entries: %ld",
                                pts->_pFancyFormatCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax currentStyle expando cache entries: %ld",
                                pts->_pStyleExpandoCache->_cMaxEls));
    }
#endif

    if (pts->_pParaFormatCache)
    {
        if (pts->_ipfDefault >= 0)
            pts->_pParaFormatCache->ReleaseData(pts->_ipfDefault);
        delete pts->_pParaFormatCache;
    }

    if (pts->_pFancyFormatCache)
    {
        if (pts->_iffDefault >= 0)
            pts->_pFancyFormatCache->ReleaseData(pts->_iffDefault);
        delete pts->_pFancyFormatCache;
    }

    delete pts->_pCharFormatCache;
    delete pts->_pStyleExpandoCache;
}

HRESULT InitFormatCache(THREADSTATE * pts)                     // Called by DllMain()
{
    CParaFormat pf;
    CFancyFormat ff;
    HRESULT hr = S_OK;

    if (!g_fStockFormatsInitialized)
    {
        g_cfStock._ccvTextColor = RGB(0,0,0);
        g_ffStock._ccvBackColor = RGB(0xff, 0xff, 0xff);
        g_pfStock._lFontHeightTwips = 240;
        g_fStockFormatsInitialized = TRUE;
    }

    pts->_pCharFormatCache = new(Mt(THREADSTATE_pCharFormatCache)) CCharFormatCache;
    if(!pts->_pCharFormatCache)
        goto MemoryError;

    pts->_pParaFormatCache = new(Mt(THREADSTATE_pParaFormatCache)) CParaFormatCache;
    if(!pts->_pParaFormatCache)
        goto MemoryError;

    pts->_ipfDefault = -1;
    pf.InitDefault();
    pf._fHasDirtyInnerFormats = pf.AreInnerFormatsDirty();
    hr = THR(pts->_pParaFormatCache->CacheData(&pf, &pts->_ipfDefault));
    if (hr)
        goto Cleanup;

    pts->_ppfDefault = &(*pts->_pParaFormatCache)[pts->_ipfDefault];

    pts->_pFancyFormatCache = new(Mt(THREADSTATE_pFancyFormatCache)) CFancyFormatCache;
    if(!pts->_pFancyFormatCache)
        goto MemoryError;

    pts->_iffDefault = -1;
    ff.InitDefault();
    hr = THR(pts->_pFancyFormatCache->CacheData(&ff, &pts->_iffDefault));
    if (hr)
        goto Cleanup;

    pts->_pffDefault = &(*pts->_pFancyFormatCache)[pts->_iffDefault];

    pts->_pStyleExpandoCache = new(Mt(THREADSTATE_pStyleExpandoCache)) CStyleExpandoCache;
    if(!pts->_pStyleExpandoCache)
        goto MemoryError;

Cleanup:
    RRETURN(hr);

MemoryError:
    DeinitFormatCache(pts);
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnsureUserStyleSheets
//
//  Synopsis:   Ensure the user stylesheets collection exists if specified by
//              user in option setting, creates it if not..
//
//----------------------------------------------------------------------------

HRESULT EnsureUserStyleSheets(LPTSTR pchUserStylesheet)
{
    CCSSParser       *pcssp;
    HRESULT          hr = S_OK;
    CStyleSheetArray *pUSSA = TLS(pUserStyleSheets);
    CStyleSheet      *pUserStyleSheet = NULL;  // The stylesheet built from user specified file in Option settings
    const TCHAR      achFileProtocol[8] = _T("file://");

    if (pUSSA)
    {
        if (!_tcsicmp(pchUserStylesheet, pUSSA->_cstrUserStylesheet))
            goto Cleanup;
        else
        {
            // Force the user stylesheets collection to release its refs on stylesheets/rules.
            // No need to rel as no owner,
            pUSSA->Free( );

            // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
            // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
            pUSSA->CBase::PrivateRelease();
            pUSSA = TLS(pUserStyleSheets) = NULL;
        }
    }

    // bail out if user SS file is not specified, but "Use My Stylesheet" is checked in options
    if (!*pchUserStylesheet)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pUSSA = new CStyleSheetArray(NULL, NULL, 0);
    if (!pUSSA || pUSSA->_fInvalid)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pUSSA->CreateNewStyleSheet(NULL, &pUserStyleSheet));
    if (hr)
        goto Cleanup;

    pcssp = new CCSSParser(pUserStyleSheet, NULL);
    if (!pcssp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    Assert(!pUserStyleSheet->_achAbsoluteHref && "absoluteHref already computed.");

    pUserStyleSheet->_achAbsoluteHref = (TCHAR *)MemAlloc(Mt(CStyleSheetHref), (_tcslen(pchUserStylesheet) + ARRAY_SIZE(achFileProtocol)) * sizeof(TCHAR));
    if (! pUserStyleSheet->_achAbsoluteHref)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _tcscpy(pUserStyleSheet->_achAbsoluteHref, achFileProtocol);
    _tcscat(pUserStyleSheet->_achAbsoluteHref, pchUserStylesheet);

    hr = THR(pcssp->LoadFromFile(pchUserStylesheet, g_cpDefault));
    delete pcssp;
    if (hr)
        goto Cleanup;

    TLS(pUserStyleSheets) = pUSSA;
    pUSSA->_cstrUserStylesheet.Set(pchUserStylesheet);

Cleanup:
    if (hr && pUSSA)
    {
        // Force the user stylesheets collection to release its refs on stylesheets/rules.
        // No need to rel as no owner,
        pUSSA->Free( );

        // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
        // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
        pUSSA->CBase::PrivateRelease();
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   DeinitUserStyleSheets
//
//  Synopsis:   Deallocates thread local memory for User supplied Style
//              sheets if allocated. Called from DllThreadDetach.
//
//-------------------------------------------------------------------------

void DeinitUserStyleSheets(THREADSTATE *pts)
{
    if (pts->pUserStyleSheets)
    {
        // Force the user stylesheets collection to release its refs on stylesheets/rules.
        // No need to rel as no owner,
        pts->pUserStyleSheets->Free( );

        // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
        // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
        pts->pUserStyleSheets->CBase::PrivateRelease();
        pts->pUserStyleSheets = NULL;
    }
}

// Default CharFormat ---------------------------------------------------------

HRESULT
CDoc::CacheDefaultCharFormat()
{
    Assert(_pOptionSettings);
    Assert(_pCodepageSettings);
    Assert(_icfDefault == -1);
    Assert(_pcfDefault == NULL);

    THREADSTATE * pts = GetThreadState();
    CCharFormat cf;
    HRESULT hr;

    cf.InitDefault(_pOptionSettings, _pCodepageSettings);
    cf._bCrcFont = cf.ComputeFontCrc();
    cf._fHasDirtyInnerFormats = !!cf.AreInnerFormatsDirty();

    hr = THR(pts->_pCharFormatCache->CacheData(&cf, &_icfDefault));

    if (hr == S_OK)
        _pcfDefault = &(*pts->_pCharFormatCache)[_icfDefault];
    else
        _icfDefault = -1;

    RRETURN(hr);
}

void
CDoc::ClearDefaultCharFormat()
{
    if (_icfDefault >= 0)
    {
        TLS(_pCharFormatCache)->ReleaseData(_icfDefault);
        _icfDefault = -1;
        _pcfDefault = NULL;
    }
}

// CFormatInfo ----------------------------------------------------------------

#if DBG==1

void
CFormatInfo::UnprepareForDebug()
{
    _fPreparedCharFormatDebug = FALSE;
    _fPreparedParaFormatDebug = FALSE;
    _fPreparedFancyFormatDebug = FALSE;
}

void
CFormatInfo::PrepareCharFormat()
{
    if (!_fPreparedCharFormat)
        PrepareCharFormatHelper();
    _fPreparedCharFormatDebug = TRUE;
}

void
CFormatInfo::PrepareParaFormat()
{
    if (!_fPreparedParaFormat)
        PrepareParaFormatHelper();
    _fPreparedParaFormatDebug = TRUE;
}

void
CFormatInfo::PrepareFancyFormat()
{
    if (!_fPreparedFancyFormat)
        PrepareFancyFormatHelper();
    _fPreparedFancyFormatDebug = TRUE;
}

CCharFormat &
CFormatInfo::_cf()
{
    AssertSz(_fPreparedCharFormatDebug, "Attempt to access _cf without calling PrepareCharFormat");
    return(_cfDst);
}

CParaFormat &
CFormatInfo::_pf()
{
    AssertSz(_fPreparedParaFormatDebug, "Attempt to access _pf without calling PrepareParaFormat");
    return(_pfDst);
}

CFancyFormat &
CFormatInfo::_ff()
{
    AssertSz(_fPreparedFancyFormatDebug, "Attempt to access _ff without calling PrepareFancyFormat");
    return(_ffDst);
}

#endif

void
CFormatInfo::Cleanup()
{
    if (_pAAExpando)
    {
        _pAAExpando->Free();
        _pAAExpando = NULL;
    }

    _cstrBgImgUrl.Free();
    _cstrLiImgUrl.Free();
    _cstrFilters.Free();

}

CAttrArray *
CFormatInfo::GetAAExpando()
{
    if (_pAAExpando == NULL)
    {
        memset(&_AAExpando, 0, sizeof(_AAExpando));
        _pAAExpando = &_AAExpando;

        if (_pff->_iExpandos >= 0)
        {
            IGNORE_HR(_pAAExpando->CopyExpandos(GetExpandosAttrArrayFromCacheEx(_pff->_iExpandos)));
        }

        _fHasExpandos = TRUE;
    }

    return(_pAAExpando);
}

void
CFormatInfo::PrepareCharFormatHelper()
{
    Assert(_pcfSrc != NULL && _pcf == _pcfSrc);
    _pcf = &_cfDst;
    memcpy(&_cfDst, _pcfSrc, sizeof(CCharFormat));
    _fPreparedCharFormat = TRUE;
}

void
CFormatInfo::PrepareParaFormatHelper()
{
    Assert(_ppfSrc != NULL && _ppf == _ppfSrc);
    _ppf = &_pfDst;
    memcpy(&_pfDst, _ppfSrc, sizeof(CParaFormat));
    _fPreparedParaFormat = TRUE;
}

void
CFormatInfo::PrepareFancyFormatHelper()
{
    Assert(_pffSrc != NULL && _pff == _pffSrc);
    _pff = &_ffDst;
    memcpy(&_ffDst, _pffSrc, sizeof(CFancyFormat));
    _fPreparedFancyFormat = TRUE;
}

HRESULT
CFormatInfo::ProcessImgUrl(CElement * pElem, LPCTSTR lpszUrl, DISPID dispID,
    LONG * plCookie, BOOL fHasLayout)
{
    HRESULT hr = S_OK;

    if (lpszUrl && *lpszUrl)
    {
        hr = THR(pElem->GetImageUrlCookie(lpszUrl, plCookie));
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        // Return a null cookie.
        //
        *plCookie = 0;
    }

    pElem->AddImgCtx(dispID, *plCookie);
    pElem->_fHasImage = (*plCookie != 0);

    if (dispID == DISPID_A_LIURLIMGCTXCACHEINDEX)
    {
        // url images require a request resize when modified
        pElem->_fResizeOnImageChange = *plCookie != 0;
    }
    else if (dispID == DISPID_A_BGURLIMGCTXCACHEINDEX)
    {
        // sites draw their own background, so we don't have to inherit
        // their background info
        if (!fHasLayout)
        {
            PrepareCharFormat();
            _cf()._fHasBgImage = (*plCookie != 0);
            UnprepareForDebug();
        }
    }

Cleanup:
    RRETURN(hr);
}

// ComputeFormats Helpers -----------------------------------------------------

const CCharFormat *
CTreeNode::GetCharFormatHelper()
{
    // Only CharFormat and ParaFormat are in sync.
    Assert((_iCF == -1 && _iPF == -1) || _iFF == -1);

    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    Element()->ComputeFormats((CFormatInfo *)ab, this);
    return(_iCF >= 0 ? ::GetCharFormatEx(_iCF) : &g_cfStock);
}

const CParaFormat *
CTreeNode::GetParaFormatHelper()
{
    // Only CharFormat and ParaFormat are in sync.
    Assert((_iCF == -1 && _iPF == -1) || _iFF == -1);

    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    Element()->ComputeFormats((CFormatInfo *)ab, this);
    return(_iPF >= 0 ? ::GetParaFormatEx(_iPF) : &g_pfStock);
}

const CFancyFormat *
CTreeNode::GetFancyFormatHelper()
{
    // Only CharFormat and ParaFormat are in sync.
    Assert((_iCF == -1 && _iPF == -1) || _iFF == -1);

    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    Element()->ComputeFormats((CFormatInfo *)ab, this);
    return(_iFF >= 0 ? ::GetFancyFormatEx(_iFF) : &g_ffStock);
}

long
CTreeNode::GetCharFormatIndexHelper()
{
    // Only CharFormat and ParaFormat are in sync.
    Assert((_iCF == -1 && _iPF == -1) || _iFF == -1);

    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    Element()->ComputeFormats((CFormatInfo *)ab, this);
    return(_iCF);
}

long
CTreeNode::GetParaFormatIndexHelper()
{
    // Only CharFormat and ParaFormat are in sync.
    Assert((_iCF == -1 && _iPF == -1) || _iFF == -1);

    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    Element()->ComputeFormats((CFormatInfo *)ab, this);
    return(_iPF);
}

long
CTreeNode::GetFancyFormatIndexHelper()
{
    // Only CharFormat and ParaFormat are in sync.
    Assert((_iCF == -1 && _iPF == -1) || _iFF == -1);

    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    Element()->ComputeFormats((CFormatInfo *)ab, this);
    return(_iFF);
}

//+----------------------------------------------------------------------------
//
//  Member:     CNode::CacheNewFormats
//
//  Synopsis:   This function is called on conclusion on ComputeFormats
//              It caches the XFormat's we have just computed.
//              This exists so we can share more code between
//              CElement::ComputeFormats and CTable::ComputeFormats
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
CTreeNode::CacheNewFormats(CFormatInfo * pCFI)
{
    THREADSTATE * pts = GetThreadState();
    LONG lIndex, iExpando = -1;
    HRESULT hr = S_OK;

    // Only CharFormat and ParaFormat are in sync.
    Assert( (_iCF == -1 && _iPF == -1) || (_iCF != -1 && _iPF != -1));
    Assert(_iFF == -1);

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDataCacheDisable))
    {
        pCFI->PrepareCharFormat();
        pCFI->PrepareParaFormat();
        pCFI->PrepareFancyFormat();
    }
#endif

    if ( _iCF == -1)
    {
        //
        // CCharFormat
        //

        if (!pCFI->_fPreparedCharFormat)
        {
            MtAdd(Mt(CharFormatAddRef), 1, 0);
            _iCF = pCFI->_icfSrc;
            pts->_pCharFormatCache->AddRefData(_iCF);
        }
        else
        {
            WHEN_DBG(pCFI->_fPreparedCharFormatDebug = TRUE;)

            pCFI->_cf()._bCrcFont = pCFI->_cf().ComputeFontCrc();
            pCFI->_cf()._fHasDirtyInnerFormats = !!pCFI->_cf().AreInnerFormatsDirty();

            MtAdd(pCFI->_pcfSrc->Compare(&pCFI->_cf()) ? Mt(CharFormatTouched) : Mt(CharFormatCached), 1, 0);

            hr = THR(pts->_pCharFormatCache->CacheData(&pCFI->_cf(), &lIndex));
            if (hr)
                goto Error;

            Assert(lIndex < MAX_FORMAT_INDEX && lIndex >= 0);

            _iCF = lIndex;
        }

        //
        // ParaFormat
        //

        if (!pCFI->_fPreparedParaFormat)
        {
            MtAdd(Mt(ParaFormatAddRef), 1, 0);
            _iPF = pCFI->_ipfSrc;
            pts->_pParaFormatCache->AddRefData(_iPF);
        }
        else
        {
            WHEN_DBG(pCFI->_fPreparedParaFormatDebug = TRUE;)

            pCFI->_pf()._fHasDirtyInnerFormats = pCFI->_pf().AreInnerFormatsDirty();

            MtAdd(pCFI->_ppfSrc->Compare(&pCFI->_pf()) ? Mt(ParaFormatTouched) : Mt(ParaFormatCached), 1, 0);

            hr = THR(pts->_pParaFormatCache->CacheData(&pCFI->_pf(), &lIndex));
            if (hr)
                goto Error;

            Assert( lIndex < MAX_FORMAT_INDEX && lIndex >= 0 );

            _iPF = lIndex;
        }

        TraceTag((
            tagFormatCaches,
            "Caching char & para format for "
            "element (tag: %ls, SN: E%d N%d) in %d, %d",
            Element()->TagName(), Element()->SN(), SN(), _iCF, _iPF ));
    }

    //
    // CFancyFormat
    //

    if (pCFI->_pAAExpando)
    {
        MtAdd(Mt(StyleExpandoCached), 1, 0);

        hr = THR(pts->_pStyleExpandoCache->CacheData(pCFI->_pAAExpando, &iExpando));
        if (hr)
            goto Error;

        if (pCFI->_pff->_iExpandos != iExpando)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._iExpandos = iExpando;
            pCFI->UnprepareForDebug();
        }

        pCFI->_pAAExpando->Free();
        pCFI->_pAAExpando = NULL;
    }
    else
    {
        #ifdef PERFMETER
        if (pCFI->_pff->_iExpandos >= 0)
            MtAdd(Mt(StyleExpandoAddRef), 1, 0);
        #endif
    }

    if (!pCFI->_fPreparedFancyFormat)
    {
        MtAdd(Mt(FancyFormatAddRef), 1, 0);
        _iFF = pCFI->_iffSrc;
        pts->_pFancyFormatCache->AddRefData(_iFF);
    }
    else
    {
        WHEN_DBG(pCFI->_fPreparedFancyFormatDebug = TRUE;)

        MtAdd(pCFI->_pffSrc->Compare(&pCFI->_ff()) ? Mt(FancyFormatTouched) : Mt(FancyFormatCached), 1, 0);

        hr = THR(pts->_pFancyFormatCache->CacheData(&pCFI->_ff(), &lIndex));
        if (hr)
            goto Error;

        Assert(lIndex < MAX_FORMAT_INDEX && lIndex >= 0);

        _iFF = lIndex;

        if (iExpando >= 0)
        {
            pts->_pStyleExpandoCache->ReleaseData(iExpando);
        }
    }

    TraceTag((
        tagFormatCaches,
        "Caching fancy format for "
        "node (tag: %ls, SN: E%d N%d) in %d",
        Element()->TagName(), Element()->SN(), SN(), _iFF ));

    Assert(_iCF >= 0 && _iPF >= 0 && _iFF >= 0);

    pCFI->UnprepareForDebug();

    Assert(!pCFI->_pAAExpando);
    Assert(!pCFI->_cstrBgImgUrl);
    Assert(!pCFI->_cstrLiImgUrl);

    return(S_OK);

Error:
    if (_iCF >= 0) pts->_pCharFormatCache->ReleaseData(_iCF);
    if (_iPF >= 0) pts->_pParaFormatCache->ReleaseData(_iPF);
    if (_iFF >= 0) pts->_pFancyFormatCache->ReleaseData(_iFF);
    if (iExpando >= 0) pts->_pStyleExpandoCache->ReleaseData(iExpando);

    pCFI->Cleanup();

    _iCF = _iPF = _iFF = -1;

    RRETURN(hr);
}

// ComputeFormats -------------------------------------------------------------

HRESULT
CElement::ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget)
{
    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    CDoc *                  pDoc = Doc();
    THREADSTATE *           pts  = GetThreadState();
    CTreeNode *             pNodeParent;
    CElement *              pElemParent;
    BOOL                    fResetPosition = FALSE;
    BOOL                    fComputeFFOnly = pNodeTarget->_iCF != -1;
    HRESULT                 hr = S_OK;
    COMPUTEFORMATSTYPE      eExtraValues = pCFI->_eExtraValues;

    Assert(pCFI);
    Assert(SameScope(this, pNodeTarget));
    Assert(eExtraValues != ComputeFormatsType_Normal || ((pNodeTarget->_iCF == -1 && pNodeTarget->_iPF == -1) || pNodeTarget->_iFF == -1));
    AssertSz(!TLS(fInInitAttrBag), "Trying to compute formats during InitAttrBag! This is bogus and must be corrected!");

    TraceTag((tagRecalcStyle, "ComputeFormats"));

    //
    // Get the format of our parent before applying our own format.
    //

    pNodeParent = pNodeTarget->Parent();
    switch (_etag)
    {
        case ETAG_TR:
            {
                CTableSection *pSection = DYNCAST(CTableRow, this)->Section();
                if (pSection)
                    pNodeParent = pSection->GetFirstBranch();
            }
            break;
        case ETAG_TBODY:
        case ETAG_THEAD:
        case ETAG_TFOOT:
            {
                CTable *pTable = DYNCAST(CTableSection, this)->Table();
                if (pTable)
                    pNodeParent = pTable->GetFirstBranch();
                fResetPosition = TRUE;
            }
            break;
    }

    if (pNodeParent == NULL)
    {
        AssertSz(0, "CElement::ComputeFormats should not be called on elements without a parent node");
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // If the parent node has not computed formats yet, recursively compute them
    //

    pElemParent = pNodeParent->Element();

    if (    pNodeParent->_iCF == -1
        ||  pNodeParent->_iFF == -1
        ||  eExtraValues == ComputeFormatsType_GetInheritedValue )
    {
        SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

        hr = THR(pElemParent->ComputeFormats(pCFI, pNodeParent));

        SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

        if (hr)
            goto Cleanup;
    }

    Assert(pNodeParent->_iCF >= 0);
    Assert(pNodeParent->_iPF >= 0);
    Assert(pNodeParent->_iFF >= 0);

    //
    // NOTE: From this point forward any errors must goto Error instead of Cleanup!
    //

    pCFI->Reset();
    pCFI->_pNodeContext = pNodeTarget;

    //
    // Setup Fancy Format
    //

    if (_fInheritFF)
    {
        pCFI->_iffSrc = pNodeParent->_iFF;
        pCFI->_pffSrc = pCFI->_pff = &(*pts->_pFancyFormatCache)[pCFI->_iffSrc];
        pCFI->_fHasExpandos = FALSE;

        if (    pCFI->_pff->_bPositionType != stylePositionNotSet
            ||  pCFI->_pff->_bDisplay != styleDisplayNotSet
            ||  pCFI->_pff->_bVisibility != styleVisibilityNotSet
            ||  pCFI->_pff->_bOverflowX != styleOverflowNotSet
            ||  pCFI->_pff->_bOverflowY != styleOverflowNotSet
            ||  pCFI->_pff->_bPageBreaks != 0
            ||  pCFI->_pff->_fPositioned
            ||  pCFI->_pff->_fAutoPositioned
            ||  pCFI->_pff->_fScrollingParent
            ||  pCFI->_pff->_fZParent
            ||  pCFI->_pff->_ccvBackColor.IsDefined()
            ||  pCFI->_pff->_lImgCtxCookie != 0
            ||  pCFI->_pff->_iExpandos != -1
            ||  pCFI->_pff->_fHasExpressions != 0
            ||  pCFI->_pff->_pszFilters
            ||  pCFI->_pff->_fHasNoWrap)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bPositionType = stylePositionNotSet;
            pCFI->_ff()._bDisplay   = styleDisplayNotSet;
            pCFI->_ff()._bVisibility = styleVisibilityNotSet;
            pCFI->_ff()._bOverflowX = styleOverflowNotSet;
            pCFI->_ff()._bOverflowY = styleOverflowNotSet;
            pCFI->_ff()._bPageBreaks = 0;
            pCFI->_ff()._pszFilters = NULL;
            pCFI->_ff()._fPositioned = FALSE;
            pCFI->_ff()._fAutoPositioned = FALSE;
            pCFI->_ff()._fScrollingParent = FALSE;
            pCFI->_ff()._fZParent = FALSE;
            pCFI->_ff()._fHasNoWrap = FALSE;
            
            // We never ever inherit expandos or expressions
            pCFI->_ff()._iExpandos = -1;
            pCFI->_ff()._fHasExpressions = FALSE;

            if(Tag() != ETAG_TR)
            {
                //
                // do not inherit background from the table.
                //
                pCFI->_ff()._ccvBackColor.Undefine();
                pCFI->_ff()._lImgCtxCookie = 0;
                pCFI->UnprepareForDebug();
            }
        }
    }
    else
    {
        pCFI->_iffSrc = pts->_iffDefault;
        pCFI->_pffSrc = pCFI->_pff = pts->_pffDefault;

        Assert(pCFI->_pffSrc->_pszFilters == NULL);

    }

    if (!fComputeFFOnly)
    {
        //
        // Setup Char and Para formats
        //

        if (TestClassFlag(ELEMENTDESC_DONTINHERITSTYLE))
        {
            // The CharFormat inherits a couple of attributes from the parent, the rest from defaults.

            const CCharFormat * pcfParent = &(*pts->_pCharFormatCache)[pNodeParent->_iCF];
            const CParaFormat * ppfParent = &(*pts->_pParaFormatCache)[pNodeParent->_iPF];

            pCFI->_fDisplayNone      = pcfParent->_fDisplayNone;
            pCFI->_fVisibilityHidden = pcfParent->_fVisibilityHidden;

            if (pDoc->_icfDefault < 0)
            {
                hr = THR(pDoc->CacheDefaultCharFormat());
                if (hr)
                    goto Error;
            }

            Assert(pDoc->_icfDefault >= 0);
            Assert(pDoc->_pcfDefault != NULL);

            pCFI->_icfSrc = pDoc->_icfDefault;
            pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;

            // Some properties are ALWAYS inherited, regardless of ELEMENTDESC_DONTINHERITSTYLE.
            // Do that here:
            if (    pCFI->_fDisplayNone
                ||  pCFI->_fVisibilityHidden
                ||  pcfParent->_fHasBgColor
                ||  pcfParent->_fHasBgImage
                ||  pcfParent->_fRelative
                ||  pcfParent->_fNoBreakInner
                ||  pcfParent->_fRTL
                ||  pcfParent->_fBranchFiltered
                ||  pCFI->_pcf->_bCursorIdx != pcfParent->_bCursorIdx
                ||  pcfParent->_fDisabled)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fDisplayNone       = pCFI->_fDisplayNone;
                pCFI->_cf()._fVisibilityHidden  = pCFI->_fVisibilityHidden;
                pCFI->_cf()._fHasBgColor        = pcfParent->_fHasBgColor;
                pCFI->_cf()._fHasBgImage        = pcfParent->_fHasBgImage;
                pCFI->_cf()._fRelative          = pNodeParent->Element()->NeedsLayout()
                                                    ? FALSE
                                                    : pcfParent->_fRelative;
                pCFI->_cf()._fNoBreak           = pcfParent->_fNoBreakInner;
                pCFI->_cf()._fRTL               = pcfParent->_fRTL;
                pCFI->_cf()._fBranchFiltered    = pcfParent->_fBranchFiltered;
                pCFI->_cf()._bCursorIdx         = pcfParent->_bCursorIdx;
                pCFI->_cf()._fDisabled          = pcfParent->_fDisabled;
                pCFI->UnprepareForDebug();
            }


            pCFI->_ipfSrc = pNodeParent->_iPF;
            pCFI->_ppfSrc = pCFI->_ppf = ppfParent;

            if (    pCFI->_ppf->_fPreInner
                ||  pCFI->_ppf->_fInclEOLWhiteInner
                ||  pCFI->_ppf->_bBlockAlignInner != htmlBlockAlignNotSet)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPreInner              = FALSE;
                // BUGBUG: (KTam) shouldn't this be _fInclEOLWhiteInner? fix in IE6
                pCFI->_pf()._fInclEOLWhite          = FALSE;
                pCFI->_pf()._bBlockAlignInner       = htmlBlockAlignNotSet;
                pCFI->UnprepareForDebug();
            }

            if (pcfParent->_fHasGridValues)
            {
                pCFI->PrepareCharFormat();
                pCFI->PrepareParaFormat();
                pCFI->_cf()._fHasGridValues = TRUE;
                pCFI->_cf()._uLayoutGridMode = pcfParent->_uLayoutGridModeInner;
                pCFI->_cf()._uLayoutGridType = pcfParent->_uLayoutGridTypeInner;
                pCFI->_pf()._cuvCharGridSize = ppfParent->_cuvCharGridSizeInner;
                pCFI->_pf()._cuvLineGridSize = ppfParent->_cuvLineGridSizeInner;

                // BUGBUG (srinib) - if inner values need to be inherited too for elements like
                // input, select,textarea then copy the inner values from parent here
                // pCFI->_cf()._uLayoutGridModeInner = pcfParent->_uLayoutGridModeInner;
                // pCFI->_cf()._uLayoutGridTypeInner = pcfParent->_uLayoutGridTypeInner;
                // pCFI->_pf()._cuvCharGridSizeInner = ppfParent->_cuvCharGridSizeInner;
                // pCFI->_pf()._cuvLineGridSizeInner = ppfParent->_cuvLineGridSizeInner;
                pCFI->UnprepareForDebug();
            }

            // outer block alignment should still be inherited from
            // parent, but reset the inner block alignment
            pCFI->_bCtrlBlockAlign  = ppfParent->_bBlockAlignInner;
            pCFI->_bBlockAlign      = htmlBlockAlignNotSet;


            // outer direction should still be inherited from parent
            if(pCFI->_ppf->_fRTL != pCFI->_ppf->_fRTLInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTL = pCFI->_ppf->_fRTLInner;
                pCFI->UnprepareForDebug();
            }

        }
        else
        {
            // Inherit the Char and Para formats from the parent node

            pCFI->_icfSrc = pNodeParent->_iCF;
            pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
            pCFI->_ipfSrc = pNodeParent->_iPF;
            pCFI->_ppfSrc = pCFI->_ppf = &(*pts->_pParaFormatCache)[pCFI->_ipfSrc];

            pCFI->_fDisplayNone      = pCFI->_pcf->_fDisplayNone;
            pCFI->_fVisibilityHidden = pCFI->_pcf->_fVisibilityHidden;

            // If the parent had layoutness, clear the inner formats

            if (pNodeParent->Element()->NeedsLayout())
            {
                if (pCFI->_pcf->_fHasDirtyInnerFormats)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf().ClearInnerFormats();
                    pCFI->UnprepareForDebug();
                }

                if (pCFI->_ppf->_fHasDirtyInnerFormats)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf().ClearInnerFormats();
                    pCFI->UnprepareForDebug();
                }

                // copy parent's inner formats to current elements outer
                if (    pCFI->_ppf->_fPre != pCFI->_ppf->_fPreInner
                    ||  pCFI->_ppf->_fInclEOLWhite != pCFI->_ppf->_fInclEOLWhiteInner
                    ||  pCFI->_ppf->_bBlockAlign != pCFI->_ppf->_bBlockAlignInner)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._fPre = pCFI->_pf()._fPreInner;
                    pCFI->_pf()._fInclEOLWhite = pCFI->_pf()._fInclEOLWhiteInner;
                    pCFI->_pf()._bBlockAlign = pCFI->_pf()._bBlockAlignInner;
                    pCFI->UnprepareForDebug();
                }

                if (pCFI->_pcf->_fNoBreak != pCFI->_pcf->_fNoBreakInner)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fNoBreak = pCFI->_pcf->_fNoBreakInner;
                    pCFI->UnprepareForDebug();
                }
                
                // copy parent's inner formats to current elements outer
                if (pCFI->_pcf->_fHasGridValues)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->PrepareParaFormat();
                    pCFI->_cf()._uLayoutGridMode = pCFI->_pcf->_uLayoutGridModeInner;
                    pCFI->_cf()._uLayoutGridType = pCFI->_pcf->_uLayoutGridTypeInner;
                    pCFI->_pf()._cuvCharGridSize = pCFI->_ppf->_cuvCharGridSizeInner;
                    pCFI->_pf()._cuvLineGridSize = pCFI->_ppf->_cuvLineGridSizeInner;
                    pCFI->UnprepareForDebug();
                }
            }

            if (pCFI->_pcf->_fBranchFiltered)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fBranchFiltered = TRUE;
                pCFI->UnprepareForDebug();
            }

            // outer block alignment should still be inherited from
            // parent
            pCFI->_bCtrlBlockAlign  = pCFI->_ppf->_bBlockAlign;
            pCFI->_bBlockAlign      = pCFI->_ppf->_bBlockAlign;

            // outer direction should still be inherited from parent
            if(pCFI->_ppf->_fRTL != pCFI->_ppf->_fRTLInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTL = pCFI->_ppf->_fRTLInner;
                pCFI->UnprepareForDebug();
            }

        }

        pCFI->_bControlAlign = htmlControlAlignNotSet;
    }
    else
    {
        pCFI->_icfSrc = pDoc->_icfDefault;
        pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;
        pCFI->_ipfSrc = pts->_ipfDefault;
        pCFI->_ppfSrc = pCFI->_ppf = pts->_ppfDefault;
    }

    hr = THR(ApplyDefaultFormat(pCFI));
    if (hr)
        goto Error;

    hr = THR(ApplyInnerOuterFormats(pCFI));
    if (hr)
        goto Error;

    if (eExtraValues == ComputeFormatsType_Normal || 
        eExtraValues == ComputeFormatsType_ForceDefaultValue)
    {
        if (fResetPosition)
        {
            if (pCFI->_pff->_bPositionType != stylePositionstatic)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bPositionType = stylePositionstatic;
                pCFI->UnprepareForDebug();
            }
        }

		if (!Doc()->IsPrintDoc() && GetMarkup()->IsPrimaryMarkup() && (_fHasFilterCollectionPtr || pCFI->_fHasFilters))
			ComputeFilterFormat(pCFI);

        hr = THR(pNodeTarget->CacheNewFormats(pCFI));
        pCFI->_cstrFilters.Free();  // Arrggh!!! BUGBUG (michaelw)  This should really happen 
									// somewhere else (when you know where, put it there)
									// Fix CTableCell::ComputeFormats also
        if (hr)
            goto Error;

        // Cache whether an element is a block element or not for fast retrieval.
        pNodeTarget->_fBlockNess = pCFI->_pff->_fBlockNess;

        //
        // BUGBUG (srinib) - we need to move this bit onto element.
        //
        if (HasLayoutPtr())
        {
            GetCurLayout()->_fEditableDirty = TRUE;

            //
            // If the current element has a layout attached to but does not
            // need one, post a layout request to lazily destroy it.
            //
            if (!pCFI->_pff->_fHasLayout)
            {
                GetCurLayout()->PostLayoutRequest(LAYOUT_MEASURE|LAYOUT_POSITION);
            }
        }

        pNodeTarget->_fHasLayout = pCFI->_pff->_fHasLayout;

        // Update expressions in the recalc engine
        //
        // If we had expressions or have expressions then we need to tell the recalc engine
        // 
        if (_fHasStyleExpressions || pCFI->_pff->_fHasExpressions)
            IGNORE_HR(Doc()->_view.AddRecalcTask(this));

    }

Cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    RRETURN(hr);

Error:
    pCFI->Cleanup();
    goto Cleanup;
}

//---------------------------------------------------------------------------
//
// Member:		CElement::ComputeFilterFormat
//
// Description:	A helper function to do whatever work is needed
//				for filters.  This can be scheduling a filter
//				task (when the filters will be actually instantiated)
//				or filtering the visibility
//
//---------------------------------------------------------------------------
void
CElement::ComputeFilterFormat(CFormatInfo *pCFI)
{
    //
    // Filters can be tricky.  We used to instantiate them
    // right here while in ComputeFormats.  As ComputeFormats
    // can now be deferred to paint time, this is no longer practical
    //
    // Filters can also mess with visibility.  Fortunately, they never
    // do so when they are instantiated.  Typically they defer changes
    // the visibility property and then make them happen later on.
    //
    // In the (rare) event that the filter chain's opinion of visibility
    // changes and the filter string itself is changed before ComputeFormats
    // is called, we really don't care about the old filter chain's opinion
    // and will start everything afresh.  For this reason we are either
    // filtering the visibility bit (when the filter string is unchanged)
    // or scheduling a filter task to create or destroy the filter chain.
    // We never do both.
    //

    // Filters don't print, even if we bypass them for painting
    // because due to their abilities to manipulate the object model
    // they are able to move objects to unprintable positions
    // so in that case bail out for printing
    // We are aware that this may cause scripterrors against the filter
    // which we would ignore anyway in printing (Frankman)
    //
    // Although Frank's concerns about OM changes during print are
    // no longer a real issue, now is not the time to change this (michaelw)
    //

	Assert(!Doc()->IsPrintDoc() && GetMarkup()->IsPrimaryMarkup() && (_fHasFilterCollectionPtr || pCFI->_fHasFilters));

	LPOLESTR pszFilterNew = pCFI->_pff->_pszFilters;
    CFilterArray *pFA = GetFilterCollectionPtr();

    LPCTSTR pszFilterOld = pFA ? pFA->GetFullText() : NULL;

    if ((!pszFilterOld && !pszFilterNew)
    ||  (pszFilterOld && pszFilterNew && !StrCmpC(pszFilterOld, pszFilterNew)))
    {
        // Nothing has changed, just update the visibility bit as appropriate
		// It is possible for us to get here having successfully called AddFilterTask
		// and still have no filters (the filter couldn't be found or failed to hookup)

        if (HasFilterPtr())
        {
            BOOL fHiddenOld = pCFI->_pcf->_fVisibilityHidden;
            BOOL fHiddenNew = GetFilterPtr()->FilteredIsHidden(fHiddenOld);

            if (fHiddenOld != fHiddenNew)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fVisibilityHidden = fHiddenNew;
                pCFI->UnprepareForDebug();

                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bVisibility = fHiddenNew ? styleVisibilityHidden : styleVisibilityVisible;
                pCFI->UnprepareForDebug();
            }
        }
    }
    else
    {
        // The filter string has changed, schedule a task to destroy/create filters

        Verify(Doc()->AddFilterTask(this));
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::NeedsLayout, public
//
//  Synopsis:   Determines, based on attributes, whether this element needs
//              layout and if so what type.
//
//  Arguments:  [pCFI]  -- Pointer to FormatInfo with applied properties
//              [pNode] -- Context Node for this element
//              [plt]   -- Returns the type of layout we need.
//
//  Returns:    TRUE if this element needs a layout, FALSE if not.
//
//----------------------------------------------------------------------------

BOOL
CElement::ElementNeedsLayout(CFormatInfo *pCFI)
{
    
    if (    _fSite
        ||  (   !TestClassFlag(ELEMENTDESC_NOLAYOUT)
            &&  pCFI->_pff->ElementNeedsFlowLayout()))
    {
        return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CElement::HasPageBreakBefore / After
//
//  Synopsis:   Checks whether layout has page-break-before or after.
//
//-----------------------------------------------------------------------------

BOOL
CElement::HasPageBreakBefore()
{
    CTreeNode          * pNode = GetFirstBranch();
    const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;
    BOOL                 fPageBreakBefore = pFF ? (!!GET_PGBRK_BEFORE(pFF->_bPageBreaks)) : FALSE;

    return fPageBreakBefore;
}

BOOL
CElement::HasPageBreakAfter()
{
    CTreeNode          * pNode = GetFirstBranch();
    const CFancyFormat * pFF = pNode ? pNode->GetFancyFormat() : NULL;
    BOOL                 fPageBreakAfter = pFF ? (!!GET_PGBRK_AFTER(pFF->_bPageBreaks)) : FALSE;

    return fPageBreakAfter;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DetermineBlockness, public
//
//  Synopsis:   Determines if this element is a block element based on
//              inherent characteristics of the tag or current properties.
//
//  Arguments:  [pCFI] -- Pointer to current FormatInfo with applied properties
//
//  Returns:    TRUE if this element has blockness.
//
//----------------------------------------------------------------------------
BOOL
CElement::DetermineBlockness(CFormatInfo *pCFI)
{
    BOOL fHasBlockFlag = HasFlag(TAGDESC_BLOCKELEMENT); 
    BOOL fIsBlock      = fHasBlockFlag;
    styleDisplay disp  = (styleDisplay)(pCFI->_pff->_bDisplay);

    //
    // BUGBUG -- Are there any elements for which we don't want to override
    // blockness? (lylec)
    //
    if (disp == styleDisplayBlock)
    {
        fIsBlock = TRUE;
    }
    else if (disp == styleDisplayInline)
    {
        fIsBlock = FALSE;
    }

    return fIsBlock;
}

BOOL
IsBlockListElement(CTreeNode * pNode)
{
    return pNode->Element()->IsFlagAndBlock(TAGDESC_LIST);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::ApplyInnerOuterFormats, public
//
//  Synopsis:   Takes the current FormatInfo and creates the correct
//              inner and (if appropriate) outer formats.
//
//  Arguments:  [pCFI]     -- FormatInfo with applied properties
//              [pCFOuter] -- Place to store Outer format properties
//              [pPFOuter] -- Place to store Outer format properties
//
//  Returns:    HRESULT
//
//  Notes:      Inner/Outer sensitive formats are put in the _fXXXXInner
//              for inner and outer are held in _fXXXXXX
//
//----------------------------------------------------------------------------

HRESULT
CElement::ApplyInnerOuterFormats(CFormatInfo * pCFI)
{
    HRESULT     hr = S_OK;
    BOOL fHasLayout      = ElementNeedsLayout(pCFI);
    BOOL fNeedsOuter     = fHasLayout && 
                            (!_fSite || (TestClassFlag(ELEMENTDESC_TEXTSITE) &&
                                         !TestClassFlag(ELEMENTDESC_TABLECELL)));
    BOOL fHasLeftIndent  = FALSE;
    BOOL fHasRightIndent = FALSE;
    BOOL fIsBlockElement = DetermineBlockness(pCFI);
    LONG lFontHeight     = 1;
    CDoc * pDoc          = Doc();
    BOOL fComputeFFOnly    = pCFI->_pNodeContext->_iCF != -1;

    Assert(pCFI->_pNodeContext->Element() == this);

    if (    !!pCFI->_pff->_fHasLayout != !!fHasLayout
        ||  !!pCFI->_pff->_fBlockNess != !!fIsBlockElement)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fHasLayout = fHasLayout;
        pCFI->_ff()._fBlockNess = fIsBlockElement;
        pCFI->UnprepareForDebug();
    }

    if (   !fIsBlockElement
        && HasFlag(TAGDESC_LIST)
       )
    {
        //
        // If the current list is not a block element, and it is not parented by
        // any block-list elements, then we want the LI's inside to be treated
        // like naked LI's. To do this we have to set cuvOffsetPoints to 0.
        //
        CTreeNode *pNodeList = GetMarkup()->SearchBranchForCriteria(
            pCFI->_pNodeContext->Parent(), IsBlockListElement );
        if (!pNodeList)
        {
            pCFI->PrepareParaFormat();
            pCFI->_pf()._cuvOffsetPoints.SetValue( 0, CUnitValue::UNIT_POINT );
            pCFI->_pf()._cListing.SetNotInList();
            pCFI->_pf()._cListing.SetStyle(styleListStyleTypeDisc);
            pCFI->UnprepareForDebug();
        }
        else
        {
            styleListStyleType listType = pNodeList->GetFancyFormat()->_ListType;
            WORD               wLevel   = (WORD)pNodeList->GetParaFormat()->_cListing.GetLevel();

            pCFI->PrepareParaFormat();
            pCFI->_pf()._cListing.SetStyle(DYNCAST(CListElement, pNodeList->Element())->
                                           FilterHtmlListType(listType, wLevel));
            pCFI->UnprepareForDebug();
        }
    }
    
    if (!fComputeFFOnly)
    {
        if (pCFI->_fDisplayNone && !pCFI->_pcf->_fDisplayNone)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fDisplayNone = TRUE;
            pCFI->UnprepareForDebug();
        }

        if (pCFI->_fVisibilityHidden != unsigned(pCFI->_pcf->_fVisibilityHidden))
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fVisibilityHidden = pCFI->_fVisibilityHidden;
            pCFI->UnprepareForDebug();
        }

        if (fNeedsOuter)
        {
            if (pCFI->_fPre != pCFI->_ppf->_fPreInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPreInner = pCFI->_fPre;
                pCFI->UnprepareForDebug();
            }

            if (!!pCFI->_ppf->_fInclEOLWhiteInner != (pCFI->_fInclEOLWhite || TestClassFlag(ELEMENTDESC_SHOWTWS)))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fInclEOLWhiteInner = !pCFI->_pf()._fInclEOLWhiteInner;
                pCFI->UnprepareForDebug();
            }

            // NO WRAP
            if (pCFI->_fNoBreak)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fNoBreakInner = TRUE; //pCFI->_fNoBreak;
                pCFI->UnprepareForDebug();
            }

        }
        else
        {
            if (pCFI->_fPre && (!pCFI->_ppf->_fPre || !pCFI->_ppf->_fPreInner))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPre = pCFI->_pf()._fPreInner = TRUE;
                pCFI->UnprepareForDebug();
            }

            if (pCFI->_fInclEOLWhite && (!pCFI->_ppf->_fInclEOLWhite || !pCFI->_ppf->_fInclEOLWhiteInner))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fInclEOLWhite = pCFI->_pf()._fInclEOLWhiteInner = TRUE;
                pCFI->UnprepareForDebug();
            }

            if (pCFI->_fNoBreak && (!pCFI->_pcf->_fNoBreak || !pCFI->_pcf->_fNoBreakInner))
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fNoBreak = pCFI->_cf()._fNoBreakInner = TRUE;
                pCFI->UnprepareForDebug();
            }
        }

        if (pCFI->_fRelative)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fRelative = TRUE;
            pCFI->UnprepareForDebug();
        }
    }

    //
    // Note: (srinib) Currently table cell's do not support margins,
    // based on the implementation, this may change.
    //
    if (fIsBlockElement && pCFI->_pff->_fHasMargins && Tag() != ETAG_BODY)
    {
        pCFI->PrepareFancyFormat();

        // MARGIN-TOP - on block elements margin top is treated as before space
        if (!pCFI->_pff->_cuvMarginTop.IsNullOrEnum())
        {
            pCFI->_ff()._cuvSpaceBefore = pCFI->_pff->_cuvMarginTop;
        }

        // MARGIN-BOTTOM - on block elements margin top is treated as after space
        if (!pCFI->_pff->_cuvMarginBottom.IsNullOrEnum())
        {
            pCFI->_ff()._cuvSpaceAfter = pCFI->_pff->_cuvMarginBottom;
        }

        // MARGIN-LEFT - on block elements margin left is treated as left indent
        if (!pCFI->_pff->_cuvMarginLeft.IsNullOrEnum())
        {
            // We handle the various data types below when we accumulate values.
            fHasLeftIndent = TRUE;
        }

        // MARGIN-RIGHT - on block elements margin right is treated as right indent
        if (!pCFI->_pff->_cuvMarginRight.IsNullOrEnum())
        {
            // We handle the various data types below when we accumulate values.
            fHasRightIndent = TRUE;
        }

        pCFI->UnprepareForDebug();
    }

    if (!fComputeFFOnly)
    {
        if (!fHasLayout)
        {
            // PADDING / BORDERS
            //
            // For padding, set _fPadBord flag if CFI _fPadBord is set. Values have
            // already been copied. It always goes on inner.
            //
            if (pCFI->_fPadBord && fIsBlockElement && !pCFI->_ppf->_fPadBord)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }

            // BACKGROUND
            //
            // Sites draw their own background, so we don't have to inherit their
            // background info. Always goes on inner.
            //
            if (pCFI->_fHasBgColor && !pCFI->_pcf->_fHasBgColor)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fHasBgColor = TRUE;
                pCFI->UnprepareForDebug();
            }

            if (pCFI->_fHasBgImage)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fHasBgImage = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
    }

    //
    // FLOAT
    //
    // BUGBUG -- This code needs to move back to ApplyFormatInfoProperty to ensure
    // that the alignment follows the correct ordering. (lylec)
    //

    if (fHasLayout && pCFI->_pff->_bStyleFloat != styleStyleFloatNotSet)
    {
        htmlControlAlign hca   = htmlControlAlignNotSet;
        BOOL             fDoIt = TRUE;

        switch (pCFI->_pff->_bStyleFloat)
        {
        case styleStyleFloatLeft:
            hca = htmlControlAlignLeft;
            if (fIsBlockElement)
            {
                pCFI->_bCtrlBlockAlign = htmlBlockAlignLeft;
            }
            break;

        case styleStyleFloatRight:
            hca = htmlControlAlignRight;
            if (fIsBlockElement)
            {
                pCFI->_bCtrlBlockAlign = htmlBlockAlignRight;
            }
            break;

        case styleStyleFloatNone:
            hca = htmlControlAlignNotSet;
            break;

        default:
            fDoIt = FALSE;
        }

        if (fDoIt)
        {
            ApplySiteAlignment(pCFI, hca, this);
            pCFI->_fCtrlAlignLast = TRUE;

            // Autoclear works for float from CSS.  Navigator doesn't
            // autoclear for HTML floating.  Another annoying Nav compat hack.

            if (!pCFI->_pff->_fCtrlAlignFromCSS)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fCtrlAlignFromCSS = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
    }

    //
    // ALIGNMENT
    //
    // Alignment is tricky because DISPID_CONTROLALIGN should only set the
    // control align if it has layout, but sets the block alignment if it's
    // not.  Also, if the element has TAGDESC_OWNLINE then DISPID_CONTROLALIGN
    // sets _both_ the control alignment and block alignment.  However, you
    // can still have inline sites (that are not block elements) that have the
    // OWNLINE flag set.  Also, if both CONTROLALIGN and BLOCKALIGN are set,
    // we must remember the order they were applied.  The last kink is that
    // HR's break the pattern because they're not block elements but
    // DISPID_BLOCKALIGN does set the block align for them.
    //

    BOOL fOwnLine = HasFlag(TAGDESC_OWNLINE) && fHasLayout;

    if (fHasLayout)
    {
        if (pCFI->_pff->_bControlAlign != pCFI->_bControlAlign)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bControlAlign = pCFI->_bControlAlign;
            pCFI->UnprepareForDebug();
        }

        // If a site is positioned explicitly (absolute) it
        // overrides control alignment. We do that by simply turning off
        // control alignment.
        //
        if (    pCFI->_pff->_bControlAlign != htmlControlAlignNotSet
            &&  (   pCFI->_pff->_bControlAlign == htmlControlAlignRight
                ||  pCFI->_pff->_bControlAlign == htmlControlAlignLeft))
        {
            pCFI->PrepareFancyFormat();

            if (    pCFI->_pff->_bPositionType == stylePositionabsolute
                &&  Tag() != ETAG_BODY)
            {
                pCFI->_ff()._bControlAlign = htmlControlAlignNotSet;
                pCFI->_ff()._fAlignedLayout = FALSE;
            }
            else
            {
                pCFI->_ff()._fAlignedLayout = Tag() != ETAG_HR && Tag() != ETAG_LEGEND;
            }

            pCFI->UnprepareForDebug();

        }
    }

    if (!fComputeFFOnly)
    {
        if (fHasLayout && (fNeedsOuter || IsRunOwner()))
        {
            if (    pCFI->_ppf->_bBlockAlign != pCFI->_bCtrlBlockAlign
                ||  pCFI->_ppf->_bBlockAlignInner != pCFI->_bBlockAlign)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._bBlockAlign = pCFI->_bCtrlBlockAlign;
                pCFI->_pf()._bBlockAlignInner = pCFI->_bBlockAlign;
                pCFI->UnprepareForDebug();
            }
        }
        else if (fIsBlockElement || fOwnLine)
        {
            BYTE bAlign = pCFI->_bBlockAlign;

            if ((  (   !fIsBlockElement
                    && Tag() != ETAG_HR)
                 || pCFI->_fCtrlAlignLast)
                && (fOwnLine || !fHasLayout))
            {
                bAlign = pCFI->_bCtrlBlockAlign;
            }

            if (    pCFI->_ppf->_bBlockAlign != bAlign
                ||  pCFI->_ppf->_bBlockAlignInner != bAlign)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._bBlockAlign = bAlign;
                pCFI->_pf()._bBlockAlignInner = bAlign;
                pCFI->UnprepareForDebug();
            }
        }

        //
        // DIRECTION
        //
        if (fHasLayout && (fNeedsOuter || IsRunOwner()))
        {
            if(     (fIsBlockElement && pCFI->_ppf->_fRTL != pCFI->_pcf->_fRTL)
                ||  (pCFI->_ppf->_fRTLInner != pCFI->_pcf->_fRTL))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTLInner = pCFI->_pcf->_fRTL;
                // paulnel - we only set the inner for these guys because the
                //           positioning of the layout is determined by the
                //           parent and not by the outter _fRTL
                pCFI->UnprepareForDebug();
            }
        }
        else if (fIsBlockElement || fOwnLine)
        {
            if (pCFI->_ppf->_fRTLInner != pCFI->_pcf->_fRTL ||
                pCFI->_ppf->_fRTL != pCFI->_pcf->_fRTL)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTLInner = pCFI->_pcf->_fRTL;
                pCFI->_pf()._fRTL = pCFI->_pcf->_fRTL;
                pCFI->UnprepareForDebug();
            }
        }
        if (pCFI->_fBidiEmbed != pCFI->_pcf->_fBidiEmbed ||
            pCFI->_fBidiOverride != pCFI->_pcf->_fBidiOverride)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fBidiEmbed = pCFI->_fBidiEmbed;
            pCFI->_cf()._fBidiOverride = pCFI->_fBidiOverride;
            pCFI->UnprepareForDebug();
        }

        //
        // TEXTINDENT
        //

        // We used to apply text-indent only to block elems; now we apply regardless because text-indent
        // is always inherited, meaning inline elems can end up having text-indent in their PF 
        // (via format inheritance and not Apply).  If we don't allow Apply() to set it on inlines,
        // there's no way to change what's inherited.  This provides a workaround for bug #67276.

        if (!pCFI->_cuvTextIndent.IsNull())
        {
            pCFI->PrepareParaFormat();
            pCFI->_pf()._cuvTextIndent = pCFI->_cuvTextIndent;
            pCFI->UnprepareForDebug();
        }

        if (fIsBlockElement)
        {

            //
            // TEXTJUSTIFY
            //

            if (pCFI->_uTextJustify)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._uTextJustify = pCFI->_uTextJustify;
                pCFI->UnprepareForDebug();
            }

            //
            // TEXTJUSTIFYTRIM
            //

            if (pCFI->_uTextJustifyTrim)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._uTextJustifyTrim = pCFI->_uTextJustifyTrim;
                pCFI->UnprepareForDebug();
            }

            //
            // TEXTKASHIDA
            //

            if (!pCFI->_cuvTextKashida.IsNull())
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._cuvTextKashida = pCFI->_cuvTextKashida;
                pCFI->UnprepareForDebug();
            }
        }
    }

        

    //
    // Process any image urls (for background images, li's, etc).
    //

    if (!pCFI->_cstrBgImgUrl.IsNull())
    {
        pCFI->PrepareFancyFormat();
        pCFI->ProcessImgUrl(this, pCFI->_cstrBgImgUrl,
            DISPID_A_BGURLIMGCTXCACHEINDEX, &pCFI->_ff()._lImgCtxCookie, fHasLayout);
        pCFI->UnprepareForDebug();
        pCFI->_cstrBgImgUrl.Free();
    }

    if (!pCFI->_cstrLiImgUrl.IsNull())
    {
        pCFI->PrepareParaFormat();
        pCFI->ProcessImgUrl(this, pCFI->_cstrLiImgUrl,
            DISPID_A_LIURLIMGCTXCACHEINDEX, &pCFI->_pf()._lImgCookie, fHasLayout);
        pCFI->UnprepareForDebug();
        pCFI->_cstrLiImgUrl.Free();
    }

    //
    // ******** ACCUMULATE VALUES **********
    //

    if (!fComputeFFOnly)
    {
        //
        // LEFT/RIGHT indents
        //

        if (fHasLeftIndent)
        {
            CUnitValue cuv = pCFI->_pff->_cuvMarginLeft;

            pCFI->PrepareParaFormat();

            Assert (!cuv.IsNullOrEnum());

            //
            // LEFT INDENT
            //
            switch(cuv.GetUnitType() )
            {
            case CUnitValue::UNIT_PERCENT:
                pCFI->_pf()._cuvLeftIndentPercent.SetValue(
                    pCFI->_pf()._cuvLeftIndentPercent.GetUnitValue() +
                    cuv.GetUnitValue(),
                    CUnitValue::UNIT_PERCENT);
                break;

            case CUnitValue::UNIT_EM:
            case CUnitValue::UNIT_EX:
                if ( lFontHeight == 1 )
                    lFontHeight = pCFI->_pcf->GetHeightInTwips(pDoc);
                // Intentional fall-through...
            default:
                hr = cuv.ConvertToUnitType(CUnitValue::UNIT_POINT, 0,
                                           CUnitValue::DIRECTION_CX, lFontHeight);
                if (hr)
                    goto Cleanup;

                pCFI->_pf()._cuvLeftIndentPoints.SetValue(
                    pCFI->_pf()._cuvLeftIndentPoints.GetUnitValue() +
                    cuv.GetUnitValue(),
                    CUnitValue::UNIT_POINT);
            }
            pCFI->UnprepareForDebug();
        }

            //
            // RIGHT INDENT
            //
        if (fHasRightIndent)
        {
            CUnitValue cuv = pCFI->_pff->_cuvMarginRight;

            pCFI->PrepareParaFormat();

            Assert (!cuv.IsNullOrEnum());

            switch(cuv.GetUnitType() )
            {
            case CUnitValue::UNIT_PERCENT:
                pCFI->_pf()._cuvRightIndentPercent.SetValue(
                    pCFI->_pf()._cuvRightIndentPercent.GetUnitValue() +
                    cuv.GetUnitValue(),
                    CUnitValue::UNIT_PERCENT);
                break;

            case CUnitValue::UNIT_EM:
            case CUnitValue::UNIT_EX:
                if ( lFontHeight == 1 )
                    lFontHeight = pCFI->_pcf->GetHeightInTwips(pDoc);
                // Intentional fall-through...
            default:
                hr = cuv.ConvertToUnitType(CUnitValue::UNIT_POINT, 0,
                                           CUnitValue::DIRECTION_CX, lFontHeight);
                if (hr)
                    goto Cleanup;

                pCFI->_pf()._cuvRightIndentPoints.SetValue(
                    pCFI->_pf()._cuvRightIndentPoints.GetUnitValue()  +
                    cuv.GetUnitValue(),
                    CUnitValue::UNIT_POINT);
            }

            pCFI->UnprepareForDebug();
        }

        if (Tag() == ETAG_LI)
        {
            pCFI->PrepareParaFormat();
            pCFI->_pf()._cuvNonBulletIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
            pCFI->_pf()._cuvNonBulletIndentPercent.SetValue(0, CUnitValue::UNIT_PERCENT);
            pCFI->UnprepareForDebug();
        }

        //
        // LINE HEIGHT
        //
        switch ( pCFI->_pcf->_cuvLineHeight.GetUnitType() )
        {
        case CUnitValue::UNIT_EM:
        case CUnitValue::UNIT_EX:
            pCFI->PrepareCharFormat();
            if ( lFontHeight == 1 )
                lFontHeight = pCFI->_cf().GetHeightInTwips(pDoc);
            hr = pCFI->_cf()._cuvLineHeight.ConvertToUnitType( CUnitValue::UNIT_POINT, 1,
                CUnitValue::DIRECTION_CX, lFontHeight );
            pCFI->UnprepareForDebug();
            break;

        case CUnitValue::UNIT_PERCENT:
        {
            pCFI->PrepareCharFormat();
            if ( lFontHeight == 1 )
                lFontHeight = pCFI->_cf().GetHeightInTwips(pDoc);

            //
            // The following line of code does multiple things:
            //
            // 1) Takes the height in twips and applies the percentage scaling to it
            // 2) However, the percentages are scaled so we divide by the unit_percent
            //    scale multiplier
            // 3) Remember that its percent, so we need to divide by 100. Doing this
            //    gives us the desired value in twips.
            // 4) Dividing that by 20 and we get points.
            // 5) This value is passed down to SetPoints which will then scale it by the
            //    multiplier for points.
            //
            // (whew!)
            //
            pCFI->_cf()._cuvLineHeight.SetPoints(MulDivQuick(lFontHeight,
                        pCFI->_cf()._cuvLineHeight.GetUnitValue(),
                        20 * 100 * LONG(CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult)));

            pCFI->UnprepareForDebug();
        }
        break;
        }
    }

    if (    pCFI->_pff->_bPositionType == stylePositionrelative
        ||  pCFI->_pff->_bPositionType == stylePositionabsolute
        ||  Tag() == ETAG_ROOT
        ||  Tag() == ETAG_BODY)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fPositioned = TRUE;
        pCFI->UnprepareForDebug();
    }

    // cache important values
    if (pCFI->_pff->_fHasLayout)
    {
        if (    !TestClassFlag(ELEMENTDESC_NEVERSCROLL)
            &&  (   pCFI->_pff->_bOverflowX == styleOverflowAuto
                ||  pCFI->_pff->_bOverflowX == styleOverflowScroll
                ||  pCFI->_pff->_bOverflowY == styleOverflowAuto
                ||  pCFI->_pff->_bOverflowY == styleOverflowScroll
                ||  (pCFI->_pff->_bOverflowX == styleOverflowHidden && !pDoc->_fInHomePublisherDoc && !g_fInHomePublisher98)
                ||  (pCFI->_pff->_bOverflowY == styleOverflowHidden && !pDoc->_fInHomePublisherDoc && !g_fInHomePublisher98)
                ||  (   TestClassFlag(ELEMENTDESC_CANSCROLL)
                    &&  !pCFI->_pff->_fNoScroll)))
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fScrollingParent = TRUE;
            pCFI->UnprepareForDebug();
        }
    }

    if (    pCFI->_pff->_fScrollingParent
        ||  pCFI->_pff->_fPositioned)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fZParent = TRUE;
        pCFI->UnprepareForDebug();
    }

    if (    pCFI->_pff->_fPositioned
        &&  (   pCFI->_pff->_bPositionType != stylePositionabsolute
            ||  (pCFI->_pff->_cuvTop.IsNullOrEnum() && pCFI->_pff->_cuvBottom.IsNullOrEnum())
            ||  (pCFI->_pff->_cuvLeft.IsNullOrEnum() && pCFI->_pff->_cuvRight.IsNullOrEnum()) ) )
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fAutoPositioned = TRUE;
        pCFI->UnprepareForDebug();
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CElement::ApplyDefaultFormat(CFormatInfo * pCFI)
{
    HRESULT             hr = S_OK;
    CDoc *              pDoc = Doc();
    OPTIONSETTINGS *    pos  = pDoc->_pOptionSettings;
    BOOL                fUseStyleSheets = !pos || pos->fUseStylesheets;

    if (    _pAA
        ||  ( fUseStyleSheets && (pDoc->HasHostStyleSheets() || GetMarkup()->HasStyleSheets()) )
        ||  ( pos && pos->fUseMyStylesheet) 
        ||  (HasPeerHolder()) )
    {
        CAttrArray *        pInLineStyleAA;
        BOOL                fUserImportant = FALSE;
        BOOL                fDocumentImportant = FALSE;
        BOOL                fInlineImportant = FALSE;
        BOOL                fRuntimeImportant = FALSE;
        EMediaType          eMediaType = pDoc->IsPrintDoc() ? MEDIA_Print : MEDIA_Screen;

        Assert(pCFI && pCFI->_pNodeContext && SameScope(this, pCFI->_pNodeContext));

        // Ignore user's stylesheet and accessability settings if we're in a trusted dialog

        if (!pDoc->_fInTrustedHTMLDlg)
        {
            if (pos && pos->fUseMyStylesheet)
            {
                hr = EnsureUserStyleSheets(pos->cstrUserStylesheet);

                // Apply normal format if we are unable to ensure a USS
                if (!hr)
                {
                    hr = THR(TLS(pUserStyleSheets)->Apply(pCFI, APPLY_NoImportant, eMediaType, &fUserImportant));
                    if (hr)
                        goto Cleanup;
                }
                else
                    hr = S_OK;
            }

            pCFI->_fAlwaysUseMyColors   = g_fHighContrastMode ? TRUE : pos->fAlwaysUseMyColors;
            pCFI->_fAlwaysUseMyFontFace = pos->fAlwaysUseMyFontFace;
            pCFI->_fAlwaysUseMyFontSize = pos->fAlwaysUseMyFontSize;
        }

        // Apply any HTML formatting properties

        if (_pAA)
        {
            hr = THR(ApplyAttrArrayValues(pCFI, &_pAA, NULL, APPLY_All, NULL, FALSE));
            if (hr)
                goto Cleanup;
        }

        // Skip author stylesheet properties if they're turned off.
        if (fUseStyleSheets)
        {
            TraceTag((tagRecalcStyle, "Applying author style sheets"));

            hr = THR(GetMarkup()->ApplyStyleSheets(pCFI, APPLY_NoImportant, eMediaType, &fDocumentImportant));
            if (hr)
                goto Cleanup;

            if ( _pAA)
            {
                // Apply any inline STYLE rules

                pInLineStyleAA = GetInLineStyleAttrArray();

                if (pInLineStyleAA)
                {
                    TraceTag((tagRecalcStyle, "Applying inline style attr array"));

                    //
                    // The last parameter to ApplyAttrArrayValues is used to prevent the expression of that dispid from being
                    // overwritten.
                    //
                    // BUGBUG (michaelw) this hackyness should go away when we store both the expression and the value in a single CAttrValue
                    //

                    hr = THR(ApplyAttrArrayValues(pCFI, &pInLineStyleAA, NULL, APPLY_NoImportant, &fInlineImportant, TRUE, Doc()->_recalcHost.GetSetValueDispid(this)));
                    if (hr)
                        goto Cleanup;
                }

                if (GetRuntimeStylePtr())
                {
                    CAttrArray *    pRuntimeStyleAA;
                    
                    pRuntimeStyleAA = *GetRuntimeStylePtr()->GetAttrArray();
                    if (pRuntimeStyleAA)
                    {
                        TraceTag((tagRecalcStyle, "Applying runtime style attr array"));

                        //
                        // The last parameter to ApplyAttrArrayValues is used to 
                        // prevent the expression of that dispid from being
                        // overwritten.
                        //
                        // BUGBUG (michaelw) this hackyness should go away when 
                        // we store both the expression and the value in a single CAttrValue
                        //

                        hr = THR(ApplyAttrArrayValues(pCFI, &pRuntimeStyleAA, NULL, 
                                APPLY_NoImportant, &fRuntimeImportant, TRUE, 
                                Doc()->_recalcHost.GetSetValueDispid(this)));
                        if (hr)
                            goto Cleanup;
                    }
                }
            }
        }

        // Now handle any "!important" properties.
                // Order: document !important, inline, runtime, user !important.

        // Apply any document !important rules
        if (fDocumentImportant)
        {
            TraceTag((tagRecalcStyle, "Applying important doc styles"));

            hr = THR(GetMarkup()->GetStyleSheetArray()->Apply(pCFI, APPLY_ImportantOnly, eMediaType));
            if (hr)
                goto Cleanup;
        }

        // Apply any inline STYLE rules
        if (fInlineImportant)
        {
            TraceTag((tagRecalcStyle, "Applying important inline styles"));

            hr = THR(ApplyAttrArrayValues(pCFI, &pInLineStyleAA, NULL, APPLY_ImportantOnly));
            if (hr)
                goto Cleanup;
        }

        // Apply any runtime important STYLE rules
        if (fRuntimeImportant)
        {
            TraceTag((tagRecalcStyle, "Applying important runtimestyles"));

            hr = THR(ApplyAttrArrayValues(
                pCFI, GetRuntimeStylePtr()->GetAttrArray(), NULL, APPLY_ImportantOnly));
            if (hr)
                goto Cleanup;
        }

        if (HasPeerHolder() && pCFI->_eExtraValues == ComputeFormatsType_Normal)
        {
            CPeerHolder *   pPeerHolder = GetPeerHolder();

            if (pPeerHolder->TestFlagMulti(CPeerHolder::NEEDAPPLYSTYLE) &&
                !pPeerHolder->TestFlagMulti(CPeerHolder::LOCKAPPLYSTYLE))
            {
                //
                // This needs to be deferred so that arbitrary script code
                // does not run inside of the Compute pass.
                //

                hr = THR(ProcessPeerTask(PEERTASK_APPLYSTYLE_UNSTABLE));
                if (hr)
                    goto Cleanup;
            }
        }

        // Apply user !important rules last for accessibility
        if (fUserImportant)
        {
            TraceTag((tagRecalcStyle, "Applying important user styles"));

            hr = THR(TLS(pUserStyleSheets)->Apply(pCFI, APPLY_ImportantOnly, eMediaType));
            if (hr)
                goto Cleanup;
        }

    }
    
    if (ComputeFormatsType_ForceDefaultValue == pCFI->_eExtraValues)
    {
        Assert(pCFI->_pStyleForce);
        hr = THR(ApplyAttrArrayValues(
            pCFI, pCFI->_pStyleForce->GetAttrArray(), NULL, APPLY_All));
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//--------------------------------------------------------------
//
// Method:      CFilter::ApplyFilterCollection
//
// Description: Called async from EnsureView, this method looks at
//              the filter string and destroys and/or creates the
//              filter collection (and each filter).  If this method
//              is called and the filter string is non null then
//              we assume that we are replacing the old filter collection
//              with a new one.
//
//--------------------------------------------------------------

HRESULT
CElement::ApplyFilterCollection()
{
    CFilterArray *pFA = GetFilterCollectionPtr();
    LPCTSTR pszFilters = GetFirstBranch()->GetFancyFormat()->_pszFilters;

    // No filters on print or non primary docs
    Assert(!Doc()->IsPrintDoc());
    Assert(GetMarkup()->IsPrimaryMarkup());

    HRESULT hr = S_OK;

    if (pFA)
    {
       pFA->Passivate();
    }
    else if (pszFilters)
    {
        pFA = new CFilterArray(this);
        if (!pFA)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        AddPointer ( DISPID_INTERNAL_FILTERPTRCACHE, (void *)pFA, CAttrValue::AA_Internal );
        _fHasFilterCollectionPtr = TRUE;
    }

    if (pszFilters)
    {
        hr = THR(pFA->SetFullText(pszFilters));
        if (hr)
            goto Cleanup;

        // Build the new filter collection
        pFA->ParseFilterProperty(pszFilters, GetFirstBranch());
    }

Cleanup:

    RRETURN(hr);
}

// VoidCachedInfo -------------------------------------------------------------

void
CTreeNode::VoidCachedNodeInfo ( )
{
    THREADSTATE * pts = GetThreadState();

    // Only CharFormat and ParaFormat are in sync.
    Assert( (_iCF == -1 && _iPF == -1) || (_iCF >= 0  && _iPF >= 0) );

    if(_iCF != -1)
    {
        TraceTag((tagFormatCaches, "Releasing format cache entries "
                                "(iFF: %d, iPF: %d, iCF:%d )  for "
                                "node (SN: N%d)",
                                _iFF, _iPF, _iCF,  SN()));

        (pts->_pCharFormatCache)->ReleaseData( _iCF );
        _iCF = -1;

        (pts->_pParaFormatCache)->ReleaseData( _iPF );
        _iPF = -1;
    }

    if(_iFF != -1)
    {
        (pts->_pFancyFormatCache)->ReleaseData( _iFF );
        _iFF = -1;
    }

    Assert(_iCF == -1 && _iPF == -1 && _iFF == -1);
}

void
CTreeNode::VoidCachedInfo ( )
{
    Assert( Element() );
    Element()->_fDefinitelyNoBorders = FALSE;

    VoidCachedNodeInfo();
}

void
CTreeNode::VoidFancyFormat()
{
    THREADSTATE * pts = GetThreadState();

    if(_iFF != -1)
    {
        TraceTag((tagFormatCaches, "Releasing fancy format cache entry "
                                "(iFF: %d)  for "
                                "node (SN: N%d)",
                                _iFF,  SN()));

        (pts->_pFancyFormatCache)->ReleaseData( _iFF );
        _iFF = -1;
    }
}
