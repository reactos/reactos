//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmload.cxx
//
//  Contents:   CHtmCtx
//              CHtmInfo
//              CHtmLoad
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifdef WIN16
#define MB_PRECOMPOSED   0
#endif

// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, CStr &cstrBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagHtmInfo,   "Dwn", "Trace CHtmInfo")
PerfDbgTag(tagHtmLoad,   "Dwn", "Trace CHtmLoad");

MtDefine(CHtmCtx, Dwn, "CHtmCtx")
MtDefine(CHtmInfo, Dwn, "CHtmInfo")
MtDefine(CHtmInfoGetFile, CHtmInfo, "CHtmInfo::GetFile")
MtDefine(CHtmInfoGetPretransformedFile, CHtmInfo, "CHtmInfo::GetPretransformedFile")
#ifdef VSTUDIO7
MtDefine(CHtmInfo_aryTagsource_pv, CHtmInfo, "CHtmInfo::_aryTagsource::_pv")
#endif //VSTUDIO7
MtDefine(CHtmLoad, Dwn, "CHtmLoad")
MtDefine(CHtmLoad_aryDwnCtx_pv, CHtmLoad, "CHtmLoad::_aryDwnCtx::_pv")

// CHtmCtx --------------------------------------------------------------------

void
CHtmCtx::SetLoad(BOOL fLoad, DWNLOADINFO * pdli, BOOL fReload)
{
    HTMLOADINFO * phli = (HTMLOADINFO *)pdli;

    super::SetLoad(fLoad, pdli, fReload);

    // Handle synchronous IStream load case (bug 35102)

    if (    fLoad
        &&  !phli->phpi
        &&  phli->fParseSync)
    {
        CHtmLoad *pHtmLoad = GetHtmInfo()->GetHtmLoad();

        if (pHtmLoad)
        {
            pHtmLoad->FinishSyncLoad();
        }
    }
}

BOOL
CHtmCtx::IsLoading()
{
    return(GetHtmInfo()->IsLoading());
}

BOOL
CHtmCtx::IsOpened()
{
    return(GetHtmInfo()->IsOpened());
}

BOOL
CHtmCtx::WasOpened()
{
    return(GetHtmInfo()->WasOpened());
}

BOOL
CHtmCtx::IsHttp449()
{
    return(GetHtmInfo()->IsHttp449());
}

BOOL
CHtmCtx::FromMimeFilter()
{
    return(GetHtmInfo()->FromMimeFilter());
}

HRESULT
CHtmCtx::GetBindResult()
{
    return(GetHtmInfo()->GetBindResult());
}

HRESULT
CHtmCtx::Write(LPCTSTR pch, BOOL fParseNow)
{
    return(GetHtmInfo()->Write(pch, fParseNow));
}

HRESULT
CHtmCtx::WriteUnicodeSignature()
{
    // This should take care both 2/4 bytes wchar
    WCHAR abUnicodeSignature = NATIVE_UNICODE_SIGNATURE;
    return(GetHtmInfo()->OnSource((BYTE*)&abUnicodeSignature, sizeof(WCHAR)));
}

void
CHtmCtx::Close()
{
    GetHtmInfo()->Close();
}

void
CHtmCtx::Sleep(BOOL fSleep, BOOL fExecute)
{
    GetHtmInfo()->Sleep(fSleep, fExecute);
}

void
CHtmCtx::SetCodePage(CODEPAGE cp)
{
    GetHtmInfo()->SetDocCodePage(cp);
}

CDwnCtx *
CHtmCtx::GetDwnCtx(UINT dt, LPCTSTR pch)
{
    return(GetHtmInfo()->GetDwnCtx(dt, pch));
}

BOOL
CHtmCtx::IsKeepRefresh()
{
    return(GetHtmInfo()->IsKeepRefresh());
}

IStream *
CHtmCtx::GetRefreshStream()
{
    return(GetHtmInfo()->GetRefreshStream());
}

TCHAR *
CHtmCtx::GetFailureUrl()
{
    return(GetHtmInfo()->GetFailureUrl());
}

void
CHtmCtx::DoStop()
{
    GetHtmInfo()->DoStop();
}

TCHAR *
CHtmCtx::GetErrorString()
{
    return(GetHtmInfo()->GetErrorString());
}

void
CHtmCtx::GetRawEcho(BYTE **ppb, ULONG *pcb)
{
    GetHtmInfo()->GetRawEcho(ppb, pcb);
}

void
CHtmCtx::GetSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    GetHtmInfo()->GetSecConInfo(ppsci);
}

#ifdef VSTUDIO7
ELEMENT_TAG
CHtmCtx::GetBaseTagFromFactory(LPTSTR pchTagName)
{
    CTagsource * ptagsrc;

    ptagsrc = GetHtmInfo()->GetTagsource(pchTagName);
    return (ptagsrc) ? ptagsrc->_etagBase : ETAG_NULL;
}

LPTSTR
CHtmCtx::GetCodeBaseFromFactory(LPTSTR pchTagName)
{
    CTagsource *pTagSrc;
    pTagSrc = GetHtmInfo()->GetTagsource(pchTagName);
    if (pTagSrc != NULL)
        return (LPTSTR)pTagSrc->_cstrCodebase;
    else
        return NULL;
}

HRESULT
CHtmCtx::AddTagsources(LPTSTR *rgstrTag, ELEMENT_TAG *rgetagBase, LPTSTR *rgstrCodebase, INT cTags)
{
    return GetHtmInfo()->AddTagsources(rgstrTag, rgetagBase, rgstrCodebase, cTags);
}

HRESULT
CHtmCtx::AddTagsource(LPTSTR pchTag, ELEMENT_TAG etagBase, LPTSTR pchCodebase)
{
    return GetHtmInfo()->AddTagsource(pchTag, etagBase, pchCodebase);
}
#endif //VSTUDIO7

// CHtmInfo -------------------------------------------------------------------

ULONG
CHtmInfo::Release()
{
    // Skip over the caching logic of CDwnInfo::Release.  CHtmInfo does not
    // get cached.

    return(CBaseFT::Release());
}

CHtmInfo::~CHtmInfo()
{
    // Frees any resources that can be used on both HtmPre and HtmPost (Subaddref and addref) threads

    if (_pstmSrc)
    {
        _pstmSrc->Release();
        _pstmSrc = NULL;
    }

    MemFree(_pbSrc);
}

void
CHtmInfo::Passivate()
{
    // Frees only those resources used only on the HtmPost (addref) thread

    if (_hLock)
    {
        InternetUnlockRequestFile(_hLock);
    }

    ReleaseInterface(_pUnkLock);

    _cstrFailureUrl.Free();

    ClearInterface(&_pstmFile);
    ClearInterface(&_pstmRefresh);

    MemFree(_pchDecoded);
    _pchDecoded = NULL;

    MemFree(_pchError);
    _pchError = NULL;

    MemFree(_pbRawEcho);
    _pbRawEcho = NULL;
    _cbRawEcho = 0;

    MemFree(_pSecConInfo);
    _pSecConInfo = NULL;

    super::Passivate();
}

HRESULT
CHtmInfo::Init(DWNLOADINFO * pdli)
{
    HTMLOADINFO * phli = (HTMLOADINFO *)pdli;
    CDwnDoc * pDwnDoc = pdli->pDwnDoc;
    HRESULT hr;

    _fOpened = _fWasOpened = pdli->fClientData;
    _cpDoc   = pDwnDoc->GetDocCodePage();
    _pmi     = phli->pmi;
    _fKeepRefresh = phli->fKeepRefresh;
    _dwClass = pdli->fClientData ? PROGSINK_CLASS_HTML | PROGSINK_CLASS_NOSPIN : PROGSINK_CLASS_HTML;

    hr = THR(_cstrFailureUrl.Set(phli->pchFailureUrl));
    if (hr)
        goto Cleanup;

    ReplaceInterface(&_pstmRefresh, phli->pstmRefresh);

    hr = THR(super::Init(pdli));
    if (hr)
        goto Cleanup;

    if (!_cstrUrl && phli->pchBase)
    {
        hr = THR(_cstrUrl.Set(phli->pchBase));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmInfo::NewDwnCtx(CDwnCtx ** ppDwnCtx)
{
    *ppDwnCtx = new CHtmCtx;
    RRETURN(*ppDwnCtx ? S_OK : E_OUTOFMEMORY);
}

HRESULT
CHtmInfo::NewDwnLoad(CDwnLoad ** ppDwnLoad)
{
    *ppDwnLoad = new CHtmLoad;
    RRETURN(*ppDwnLoad ? S_OK : E_OUTOFMEMORY);
}

void
CHtmInfo::SetDocCodePage(CODEPAGE cp)
{
    if (_cpDoc != cp)
    {
        _cpDoc      = cp;
        _cbDecoded  = 0;
        _cchDecoded = 0;

        MemFree(_pchDecoded);
        _pchDecoded = NULL;
    }
}

CDwnCtx *
CHtmInfo::GetDwnCtx(UINT dt, LPCTSTR pch)
{
    CHtmLoad * pHtmLoad = GetHtmLoad();
    return(pHtmLoad ? pHtmLoad->GetDwnCtx(dt, pch) : NULL);
}

void
CHtmInfo::DoStop()
{
    CHtmLoad * pHtmLoad = GetHtmLoad();

    if (TstFlags(DWNLOAD_LOADING))
    {
        UpdFlags(DWNLOAD_MASK, DWNLOAD_STOPPED);
    }

    if (pHtmLoad)
        pHtmLoad->DoStop();
}

void
CHtmInfo::TakeErrorString(TCHAR **ppchError)
{
    Assert(!_pchError);

    delete _pchError; // defensive

    _pchError = *ppchError;

    *ppchError = NULL;
}

void
CHtmInfo::TakeRawEcho(BYTE **ppb, ULONG *pcb)
{
    Assert(!_pbRawEcho);

    delete _pbRawEcho; // defensive

    _pbRawEcho = *ppb;
    _cbRawEcho = *pcb;
    *ppb = NULL;
    *pcb = 0;
}

void
CHtmInfo::GetRawEcho(BYTE **ppb, ULONG *pcb)
{
    *ppb = _pbRawEcho;
    *pcb = _cbRawEcho;
}

void
CHtmInfo::TakeSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    Assert(!_pSecConInfo);

    delete _pSecConInfo; // defensive

    _pSecConInfo = *ppsci;
    *ppsci = NULL;
}

void
CHtmInfo::GetSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsi)
{
    *ppsi = _pSecConInfo;
}


HRESULT
CHtmInfo::OnLoadFile(LPCTSTR pszFile, HANDLE * phLock, BOOL fPretransform)
{
    PerfDbgLog2(tagHtmInfo, this, "+CHtmInfo::OnLoadFile (psz=%ls,hLock=%lX)",
        pszFile, *phLock);

    HRESULT hr = fPretransform 
                     ? THR(_cstrPretransformedFile.Set(pszFile))
                     : THR(_cstrFile.Set(pszFile));

    if (hr == S_OK)
    {
        _hLock = *phLock;
        *phLock = NULL;
    }

    PerfDbgLog1(tagHtmInfo, this, "-CHtmInfo::OnLoadFile (hr=%lX)", hr);
    RRETURN(hr);
}

void
CHtmInfo::OnLoadDone(HRESULT hrErr)
{
    if (!_hrBind && hrErr)
        _hrBind = hrErr;

    // on success, we can release failure information
    if (!_hrBind)
    {
        _cstrFailureUrl.Free();
        if (!_fKeepRefresh)
            ClearInterface(&_pstmRefresh);
    }

    if (TstFlags(DWNLOAD_LOADING))
    {
        UpdFlags(DWNLOAD_MASK, _hrBind ? DWNLOAD_ERROR : DWNLOAD_COMPLETE);
    }

    // BUGBUG:

    // Other DwnCtxs now require an explcit SetProgSink(NULL) to detach the progsink.
    // However, this is not true for HtmCtx's: an HtmCtx disconnects itself from its
    // progsink automatically when it's done loading. We could fix this, but it would
    // add an extra layer of signalling; no reason to (dbau)

    DelProgSinks();
}

void
CHtmInfo::OnBindResult(HRESULT hr)
{
    _hrBind = hr;
}

HRESULT
CHtmInfo::GetFile(LPTSTR * ppch)
{
    RRETURN(_cstrFile ? MemAllocString(Mt(CHtmInfoGetFile), _cstrFile, ppch) : E_FAIL);
}


HRESULT
CHtmInfo::GetPretransformedFile(LPTSTR * ppch)
{
    RRETURN(_cstrPretransformedFile ? MemAllocString(Mt(CHtmInfoGetPretransformedFile), _cstrPretransformedFile, ppch) : E_FAIL);
}

#ifdef VSTUDIO7

HRESULT
CHtmInfo::AddTagsources(LPTSTR *rgstrTag, ELEMENT_TAG *rgetagBase, LPTSTR *rgstrCodebase, INT cTags)
{
    HRESULT      hr = E_FAIL;
    CTagsource * ptagsrc = NULL;

    EnterCriticalSection();

    Assert(rgstrTag);
    Assert(rgetagBase);

    for (int iTag = 0; iTag < cTags; ++iTag)
    {
        ptagsrc = (CTagsource *)_aryTagsource.Append();
        if (!ptagsrc)
        {
            goto Cleanup;
        }
            
        ptagsrc->_cstrTagName.Set(rgstrTag[iTag]);
        ptagsrc->_etagBase = rgetagBase[iTag];
        if (rgstrCodebase)
            ptagsrc->_cstrCodebase.Set(rgstrCodebase[iTag]);
    }

    hr = S_OK;

Cleanup:
    
    LeaveCriticalSection();
    
    RRETURN(hr);
}

HRESULT
CHtmInfo::AddTagsource(LPTSTR pchTag, ELEMENT_TAG etagBase, LPTSTR pchCodebase)
{
    Assert(pchTag);
    Assert(etagBase);

    LPTSTR *rgstrCodebase = (pchCodebase) ? &pchCodebase : NULL;

    RRETURN(AddTagsources(&pchTag, &etagBase, rgstrCodebase, 1));    
}

CTagsource *
CHtmInfo::GetTagsource(LPTSTR pchTagName)
{
    int          cTags;
    int          iTag = 0;
    CTagsource   *ptagsrc = NULL;

    EnterCriticalSection();

    cTags = _aryTagsource.Size();
    if (cTags == 0)
        goto Cleanup;

    for (iTag = 0; iTag < cTags; ++iTag)
    {
        ptagsrc = (CTagsource *)&_aryTagsource[iTag];
        if (StrCmpIC(ptagsrc->_cstrTagName, pchTagName) == 0)
        {
            goto Cleanup;
        }
    }

    ptagsrc = NULL;

Cleanup:

    LeaveCriticalSection();
    return ptagsrc;
}
#endif //VSTUDIO7

// CHtmLoad ----------------------------------------------------------------

HRESULT
CHtmLoad::Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::Init %ls",
        pdli->pchUrl ? pdli->pchUrl : g_Zero.ach);

    HTMLOADINFO *   phli    = (HTMLOADINFO *)pdli;
    CDoc *          pDoc    = phli->pDoc;
    CMarkup *       pMarkup = phli->pMarkup;
    CDwnDoc *       pDwnDoc = pdli->pDwnDoc;
    CHtmTagStm *    pHtmTagStm = NULL;
    BOOL            fSync;
    HRESULT         hr;

    // Protect against 'this' or pDoc being destroyed while inside this
    // function.

    SubAddRef();
    CDoc::CLock Lock(pDoc);

    // Memorize binding information for sub-downloads

    _pDoc = pDoc;
    _pDoc->SubAddRef();
    _pMarkup = pMarkup;
    _pMarkup->SubAddRef();

    _fPasting = phli->phpi != NULL;

    fSync = _fPasting || phli->fParseSync;

    hr = THR(_cstrUrlDef.Set(phli->pchBase));
    if (hr)
        goto Cleanup;

    _ftHistory = phli->ftHistory;

    // Prepare document root for pasting

    if (_fPasting)
    {
        hr = THR( pDoc->_cstrPasteUrl.Set( phli->phpi->cstrSourceUrl ) );
        if (hr)
            goto Cleanup;
    }

    // Create the postparser and the input stream to the postparser

    _pHtmPost = new CHtmPost;

    if (_pHtmPost == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pHtmTagStm = new CHtmTagStm;

    if (pHtmTagStm == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pHtmPost->Init(this, pHtmTagStm, _pDoc, _pMarkup, phli->phpi, fSync || pdli->fClientData));
    if (hr)
        goto Cleanup;

    // Call our superclass to initialize and start binding.

    hr = THR(super::Init(pdli, pDwnInfo,
            pdli->fClientData ? IDS_BINDSTATUS_GENERATINGDATA_TEXT : IDS_BINDSTATUS_DOWNLOADINGDATA_TEXT,
            DWNF_ISDOCBIND | DWNF_GETCONTENTTYPE | DWNF_GETREFRESH | DWNF_GETMODTIME |
            DWNF_GETFILELOCK | DWNF_GETFLAGS | DWNF_GETSTATUSCODE | DWNF_HANDLEECHO | DWNF_GETSECCONINFO ));
    if (hr)
        goto Cleanup;

    // Create and initialize the preparser

    _pHtmPre = new CHtmPre(phli->phpi ? phli->phpi->cp : pDwnDoc->GetDocCodePage());

    if (_pHtmPre == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

#ifdef XMV_PARSE
    hr = THR(_pHtmPre->Init(this, pDwnDoc, pdli->pInetSess, phli->pstmLeader,
                _pDwnBindData, pHtmTagStm, phli->phpi, _cstrUrlDef, phli->pVersions, _pMarkup->IsXML()));
#else
    hr = THR(_pHtmPre->Init(this, pDwnDoc, pdli->pInetSess, phli->pstmLeader,
                _pDwnBindData, pHtmTagStm, phli->phpi, _cstrUrlDef, phli->pVersions));
#endif
    if (hr)
        goto Cleanup;

    if (phli->pmi == g_pmiTextPlain)
    {
        hr = THR(_pHtmPre->GoIntoPlaintextMode());
        if (hr)
            goto Cleanup;
    }

    // If we are loading synchronously, the caller expects the entire tree
    // to be built before this function returns, so we enter a loop calling
    // the preparser and postparser until they are done.

    if (fSync)
    {
        HRESULT hr2;

        Assert(phli->pstm);

        // don't wait for the message loop; execute syncrhonously as if
        // we're inside an inline script
        hr = THR(pMarkup->EnterInline());
        if (hr)
            goto Cleanup;

        while (!_pHtmPost->IsDone())
        {
            hr = THR(_pHtmPre->Exec()); // runs up to first </SCRIPT>
            if (hr)
                goto CleanupSync;

            hr = THR(_pHtmPost->Exec(INFINITE));
            if (hr)
                goto CleanupSync;

            // Exec can push a message loop, during which CHtmLoad::Close can be called,
            // so protect against _pHtmPost going NULL

            if (!_pHtmPost)
                goto CleanupSync;
        }

    CleanupSync:
        hr2 = THR(pMarkup->LeaveInline());
        if (!hr)
            hr = hr2;

        goto Cleanup;
    }

    // If we are keeping the document open, then just return without launching
    // the preparser.  All interactions with it will come via script writes.
    // In this case, the post is driven manually via PostManRunNested.

    if (pdli->fClientData)
    {
        _pHtmPre->Suspend();
        goto Cleanup;
    }

    // We are going to be running the postparser asynchronously as a task
    // on this thread.  Start the task now.  It will not actually run until
    // it is unblocked.

    PostManEnqueue(_pHtmPost);

    // We are going to be running the preparser asynchronously as a task
    // on the download thread.  Start the task now.  It will not actually
    // run until it is unblocked.

    hr = THR(StartDwnTask(_pHtmPre));
    if (hr)
        goto Cleanup;

Cleanup:

    if (pHtmTagStm)
        pHtmTagStm->Release();

    SubRelease();

    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::Init (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::OnBindRedirect(LPCTSTR pchRedirect, LPCTSTR pchMethod)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::OnBindRedirect %ls", pchRedirect);

    CHtmPre *pHtmPre = NULL;
    HRESULT hr;

    hr = THR(_cstrRedirect.Set(pchRedirect));
    if (hr)
        goto Cleanup;

    hr = THR(_cstrMethod.Set(pchMethod));
    if (hr)
        goto Cleanup;

    pHtmPre = GetHtmPreAsync();

    if (pHtmPre)
    {
        hr = THR(pHtmPre->OnRedirect(pchRedirect));

        pHtmPre->SubRelease();
    }

Cleanup:
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnBindRecirect (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::OnBindHeaders()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnBindHeaders");

    LPCTSTR     pch;
    HANDLE      hLock       = NULL;
    CHtmPre *   pHtmPre     = GetHtmPreAsync();
    HRESULT     hr          = S_OK;
    FILETIME    ftLastMod;
    BOOL        fPretransform;

    pch = _pDwnBindData->GetFileLock(&hLock, &fPretransform);

    if (pch)
    {
        hr = THR(GetHtmInfo()->OnLoadFile(pch, &hLock, fPretransform));
        if (hr)
            goto Cleanup;
    }

    pch = _pDwnBindData->GetContentType();

    if (pch && pHtmPre)
    {
        hr = THR(pHtmPre->SetContentTypeFromHeader(pch));
        if (hr)
            goto Cleanup;
    }

    pch = _pDwnBindData->GetRefresh();

    if (pch)
    {
        hr = THR(_cstrRefresh.Set(pch));
        if (hr)
            goto Cleanup;
    }

    ftLastMod = _pDwnBindData->GetLastMod();

    // Enable history if:
    //  (1) loading from cache or file, and
    //  (2) mod-dates match

    if (    (   (_pDwnBindData->GetScheme() == URL_SCHEME_FILE)
            ||  (_pDwnBindData->GetReqFlags() & INTERNET_REQFLAG_FROM_CACHE))
        &&  (   ftLastMod.dwLowDateTime == _ftHistory.dwLowDateTime
            &&  ftLastMod.dwHighDateTime == _ftHistory.dwHighDateTime))
    {
        _fLoadHistory = TRUE;
    }

    // Enable 449 echo if
    // (1) We got a 449 response
    // (2) We have echo-headers to use

    if (_pDwnBindData->GetStatusCode() == 449)
    {
        Assert(!_pbRawEcho);

        _pDwnBindData->GiveRawEcho(&_pbRawEcho, &_cbRawEcho);
    }

    _pDwnBindData->GiveSecConInfo(&_pSecConInfo);

    _pDwnInfo->SetLastMod(ftLastMod);
    _pDwnInfo->SetSecFlags(_pDwnBindData->GetSecFlags());

    GetHtmInfo()->SetMimeFilter(_pDwnBindData->FromMimeFilter());

Cleanup:
    if (hLock)
        InternetUnlockRequestFile(hLock);
    if (pHtmPre)
        pHtmPre->SubRelease();
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnBindHeaders (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::OnBindMime(MIMEINFO * pmi)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::OnBindMime %ls",
        pmi ? pmi->pch : g_Zero.ach);

    CHtmPre *   pHtmPre = GetHtmPreAsync();
    HRESULT     hr      = S_OK;

    if (pmi && pHtmPre && !_pDwnInfo->GetMimeInfo())
    {
        if (pmi->pfnImg)
        {
            _pDwnInfo->SetMimeInfo(pmi);
            _pmi = pmi;

            // The caller didn't specify a mime type and it looks like
            // we've got an image.

            _fDwnBindTerm = TRUE;
            _pDwnBindData->Disconnect();

            _fImageFile = TRUE;

            hr = THR(pHtmPre->InsertImage(GetUrl(), _pDwnBindData));
            if (hr)
            {
                _pDwnBindData->Terminate(E_ABORT);
                goto Cleanup;
            }

            // Returing S_FALSE tells the callback dispatcher to forget
            // about calling us anymore.  We don't care what happens to
            // the binding in progress ... it belongs to the image loader
            // now.

            hr = S_FALSE;
        }
        else if (pmi == g_pmiTextPlain || pmi == g_pmiTextHtml || pmi == g_pmiTextComponent)
        {
            _pDwnInfo->SetMimeInfo(pmi);
            _pmi = pmi;

            if (pmi == g_pmiTextPlain)
            {
                hr = THR(pHtmPre->GoIntoPlaintextMode());
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
    if (pHtmPre)
        pHtmPre->SubRelease();
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnBindMime (hr=%lX)", hr);
    RRETURN1(hr, S_FALSE);
}

HRESULT
CHtmLoad::OnBindData()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnBindData");

    HRESULT hr = S_OK;
    CHtmPre * pHtmPre;

    if (!_pmi)
    {
        BYTE  ab[200];
        ULONG cb;

        hr = THR(_pDwnBindData->Peek(ab, ARRAY_SIZE(ab), &cb));
        if (hr)
            goto Cleanup;

        if (cb < ARRAY_SIZE(ab) && _pDwnBindData->IsPending())
            goto Cleanup;

#if !defined(WINCE) && !defined(WIN16)
        _pmi = GetMimeInfoFromData(ab, cb, _pDwnBindData->GetContentType());

        if (_pmi == NULL || _pmi != g_pmiTextPlain && !_pmi->pfnImg) // non-image/plaintext -> assume HTML
#endif
        {
            _pmi = g_pmiTextHtml;
        }

        _pDwnBindData->SetMimeInfo(_pmi);

        hr = THR(OnBindMime(_pmi));
        if (hr)
            goto Cleanup;
    }

    pHtmPre = GetHtmPreAsync();

    if (pHtmPre)
    {
        pHtmPre->SetBlocked(FALSE);
        pHtmPre->SubRelease();
    }

Cleanup:
    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnBindData (hr=0)");
    RRETURN1(hr, S_FALSE);
}

void
CHtmLoad::OnBindDone(HRESULT hrErr)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::OnBindDone (hrErr=%lX)", hrErr);

    CHtmPre * pHtmPre = GetHtmPreAsync();

    if (pHtmPre)
    {
        pHtmPre->SetBlocked(FALSE);
        pHtmPre->SubRelease();
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnBindDone");
}

void
CHtmLoad::GiveRawEcho(BYTE **ppb, ULONG *pcb)
{
    Assert(!*ppb);

    *ppb = _pbRawEcho;
    *pcb = _cbRawEcho;
    _pbRawEcho = NULL;
    _cbRawEcho = 0;
}

void
CHtmLoad::GiveSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    Assert(!*ppsci);

    *ppsci = _pSecConInfo;
    _pSecConInfo = NULL;
}

void
CHtmLoad::FinishSyncLoad()
{
    if (_pHtmPost)
    {
        OnPostDone(S_OK);
    }
}

void
CHtmLoad::Passivate()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::Passivate");

    super::Passivate();

    if (_pHtmPre)
    {
        _pHtmPre->Release();
        _pHtmPre = NULL;
    }

    if (_pHtmPost)
    {
        _pHtmPost->Die();
        _pHtmPost->Release();
        _pHtmPost = NULL;
    }

    if (_pDoc)
    {
        _pDoc->_cstrPasteUrl.Free();

        _pDoc->SubRelease();
        _pDoc = NULL;
    }

    if (_pMarkup)
    {
        _pMarkup->SubRelease();
        _pMarkup = NULL;
    }

    MemFree(_pbRawEcho);
    _pbRawEcho = NULL;

    MemFree(_pSecConInfo);
    _pSecConInfo = NULL;

    for (int i = 0; i < DWNCTX_MAX; i++)
    {
        UINT        cEnt     = _aryDwnCtx[i].Size();
        CDwnCtx **  ppDwnCtx = _aryDwnCtx[i];

        for (; cEnt > 0; --cEnt, ++ppDwnCtx)
        {
            if (*ppDwnCtx)
            {
#ifndef WIN16
                PerfDbgLog3(tagHtmLoad, this,
                    "CHtmLoad::Passivate Release unclaimed CDwnCtx %lX #%d %ls",
                    *ppDwnCtx, ppDwnCtx - (CDwnCtx **)_aryDwnCtx[i], (*ppDwnCtx)->GetUrl());
#endif //ndef WIN16

                (*ppDwnCtx)->Release();
            }
        }
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::Passivate");
}

#if 0
void
CHtmLoad::UpdateLGT()
{
    if (_pHtmPre)
    {
        _pHtmPre->UpdateLGT(_pDoc);
    }
}
#endif

void
CHtmLoad::OnPostFinishScript()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnPostFinishScript");

    //
    // It's possible to get here without a _pHtmPre if we were
    // given an ExecStop in the middle of script execution.
    //

    if (_pHtmPre && !_pHtmPre->Resume())
    {
        _pHtmPre->SetBlocked(FALSE);
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnPostFinishScript");
}

void
CHtmLoad::DoStop()
{
    // Abort the binding
    _pDwnBindData->Terminate(E_ABORT);

    // Tell htmpost to stop processing tokens
    if (_pHtmPost)
        _pHtmPost->DoStop();

}


HRESULT
CHtmLoad::OnPostRestart(CODEPAGE codepage)
{
    IStream *pstm = NULL;
    CDwnBindData *pDwnBindData = NULL;
    HRESULT hr;

    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnPostFinishScript");

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pstm));
    if (hr)
        goto Cleanup;

    hr = THR(GetHtmInfo()->CopyOriginalSource(pstm, 0));
    if (hr)
        goto Cleanup;

    hr = THR(pstm->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    pDwnBindData = _pDwnBindData;
    pDwnBindData->AddRef();

    _fDwnBindTerm = TRUE;
    _pDwnBindData->Disconnect();

    hr = THR(_pDoc->RestartLoad(pstm, pDwnBindData, codepage));
    if (hr)
    {
        pDwnBindData->Terminate(E_ABORT);
        if (_pHtmPre && !_pHtmPre->Resume())
        {
            _pHtmPre->SetBlocked(FALSE);
        }
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pstm);
    if (pDwnBindData)
        pDwnBindData->Release();

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnPostFinishScript");

    RRETURN(hr);
}

HRESULT
CHtmLoad::OnPostStart()
{
    HRESULT hr = S_OK;

    Assert(_pDoc->_dwTID == GetCurrentThreadId());

    // Set the _fImageFile flag on the document based on the media
    // type of the load context.

    _pDoc->_fImageFile = _fImageFile;

    // If we were redirected, adjust the base URL of the document
    // and discard original post data if any, and clear 449 echo

    if (_cstrRedirect)
    {
        BOOL fKeepPost;
        CStr cstrSpecialURL;

        if (WrapSpecialUrl(_cstrRedirect, &cstrSpecialURL, _pDoc->_cstrUrl, FALSE, FALSE) != S_OK)
        {
            hr = E_ACCESSDENIED;     // Something bad happened, security spoof
            goto Cleanup;
        }

        fKeepPost = (_cstrMethod && _tcsequal(_cstrMethod, _T("POST")));

        hr = THR(_pDoc->SetUrl(_cstrRedirect, fKeepPost));
        if (hr)
            goto Cleanup;

        // BUGFIX 20348: don't discard 449 echo information on redirect
        // MemFree(_pbRawEcho);
        // _pbRawEcho = NULL;
        // _cbRawEcho = 0;
    }

    if (_cstrRefresh)
    {
        _pDoc->ProcessHttpEquiv(_T("Refresh"), _cstrRefresh);
    }

    // block history if needed

    if (!_fLoadHistory)
    {
        _pDoc->ClearLoadHistoryStreams();
    }

    if (!_fPasting)
    {
        // notify doc about security
        _pDoc->OnHtmDownloadSecFlags(GetSecFlags());
    }

    // grab the raw header data
    GetHtmInfo()->TakeRawEcho(&_pbRawEcho, &_cbRawEcho);

    // grab the security info structure
    GetHtmInfo()->TakeSecConInfo(&_pSecConInfo);

Cleanup:
    RRETURN(hr);
}

void
CHtmLoad::OnPostDone(HRESULT hrErr)
{
    //
    // If CHtmLoad::Close has been called before CHtmPost has finished,
    // we may have already released and NULL'ed this pointer
    //

    if (_pHtmPost)
    {
        if (_pHtmPost->_pchError)
        {
            GetHtmInfo()->TakeErrorString(&(_pHtmPost->_pchError));
        }

        PostManDequeue(_pHtmPost);
        _pHtmPost->Release();
        _pHtmPost = NULL;
    }

    OnDone(hrErr);
}


HRESULT
CHtmLoad::Write(LPCTSTR pch, BOOL fParseNow)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::Write");

    BOOL    fExhausted;
    ULONG   cch = _tcslen(pch);
    HRESULT hr;

    //$ Note that if this is an image we might be in pass-thru mode ...
    //$ only write to the CDwnBindData but don't tokenize or post-parse.

    AssertSz(_pHtmPre, "document.write called and preparser is gone");

    // InsertText pushes the specified text into the preparser's buffer (stacking when needed)
    // It also counts and adds the source to the HtmInfo if appropriate
    hr = THR(_pHtmPre->InsertText(pch, cch));
    if (hr)
        goto Cleanup;

    if (fParseNow)
    {
        // An extra EnterInline / LeaveInline is needed around Write so we treat the parser
        // as synchronous in the case of a cross-window or C-code write (not initiated by script)
        
        hr = THR(_pMarkup->EnterInline());
        if (hr) 
            goto Cleanup;
        
        for (;;)
        {
            // TokenizeText runs the preparser as far as possible (stopping at scripts when needed)
            hr = THR(_pHtmPre->TokenizeText(&fExhausted));
            if (hr)
                goto Cleanup;

            if (fExhausted)
                break;

            hr = THR(_pHtmPost->RunNested());
            if (hr)
                goto Cleanup;
        }
        
        hr = THR(_pMarkup->LeaveInline());
        if (hr) 
            goto Cleanup;
    }

Cleanup:
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnScriptWrite (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::Close()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnClose");

    HRESULT hr;
    HRESULT hr2;

    //$ Don't run tokenizer if we are in pass-thru mode to an image.

    Verify(!_pHtmPre->Resume());

    // don't wait for the message loop; execute syncrhonously as if
    // we're inside an inline script
    hr = THR(_pMarkup->EnterInline());
    if (hr)
        goto Cleanup;

    while (!_pHtmPost->IsDone())
    {
        hr = THR(_pHtmPre->Exec()); // runs up to first </SCRIPT>
        if (hr)
            goto CleanupSync;

        hr = THR(_pHtmPost->Exec(INFINITE));
        if (hr)
            goto CleanupSync;
    }

CleanupSync:
    hr2 = THR(_pMarkup->LeaveInline());
    if (!hr)
        hr = hr2;

    if (hr)
        goto Cleanup;

    if (_pHtmPost)
    {
        OnPostDone(S_OK);
    }

Cleanup:
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnClose (hr=%lX)", hr);
    RRETURN(hr);
}

void
CHtmLoad::Sleep(BOOL fSleep, BOOL fExecute)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::Sleep");

    if (_pHtmPost)
    {
        if (fSleep)
        {
            _pHtmPost->_dwFlags |= POSTF_SLEEP;
            PostManSuspend(_pHtmPost);
        }
        else
        {
            _pHtmPost->_dwFlags &= ~POSTF_SLEEP;
            PostManResume(_pHtmPost, fExecute);
        }
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::Sleep");
}

HRESULT
CHtmLoad::AddDwnCtx(UINT dt, CDwnCtx * pDwnCtx)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::AddDwnCtx");

    HRESULT hr;

    EnterCriticalSection();

    if (_fPassive)
        hr = E_ABORT;
    else
    {
        hr = THR(_aryDwnCtx[dt].Append(pDwnCtx));

        if (hr == S_OK)
        {
            PerfDbgLog3(tagHtmLoad, this, "CHtmLoad::AddDwnCtx %lX #%d %ls",
                pDwnCtx, _aryDwnCtx[dt].Size() - 1, pDwnCtx->GetUrl());

            pDwnCtx->AddRef();
        }
    }

    LeaveCriticalSection();

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::AddDwnCtx");
    RRETURN(hr);
}

CDwnCtx *
CHtmLoad::GetDwnCtx(UINT dt, LPCTSTR pch)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::GetDwnCtx");

    CDwnCtx * pDwnCtx = NULL;
    int i, iLast;

    EnterCriticalSection();

    // loop order: check _iDwnCtxFirst first, then the rest

    if (_aryDwnCtx[dt].Size())
    {
        iLast = _iDwnCtxFirst[dt];
        Assert(iLast <= _aryDwnCtx[dt].Size());

        if (iLast == _aryDwnCtx[dt].Size())
            iLast = 0;

        i = iLast;

        do
        {
            Assert(i < _aryDwnCtx[dt].Size());

            pDwnCtx = _aryDwnCtx[dt][i];

            if (pDwnCtx && !StrCmpC(pDwnCtx->GetUrl(), pch))
            {
                PerfDbgLog3(tagHtmLoad, this, "CHtmLoad::GetDwnCtx %lX #%d %ls",
                    pDwnCtx, i, pch);

                _aryDwnCtx[dt][i] = NULL;

                #if DBG==1
                if (i != _iDwnCtxFirst[dt])
                {
                    TraceTag((tagError, "CHtmLoad DwnCtx vector %d out of order", dt));
                }
                #endif

                _iDwnCtxFirst[dt] = i + 1; // this may == _aryDwnCtx[dt].Size()
                break;
            }

            pDwnCtx = NULL;

            if (++i == _aryDwnCtx[dt].Size())
                i = 0;

        }
        while (i != iLast);
    }

    #if DBG==1 || defined(PERFTAGS)
    if (pDwnCtx == NULL)
        PerfDbgLog1(tagHtmLoad, this, "CHtmLoad::GetDwnCtx failed %ls", pch);
    #endif

    LeaveCriticalSection();

    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::GetDwnCtx (pDwnCtx=%lX)", pDwnCtx);
    return(pDwnCtx);
}

CHtmPre *
CHtmLoad::GetHtmPreAsync()
{
    EnterCriticalSection();

    CHtmPre * pHtmPre = _fPassive ? NULL : _pHtmPre;

    if (pHtmPre)
        pHtmPre->SubAddRef();

    LeaveCriticalSection();

    return(pHtmPre);
}

#ifdef XMV_PARSE
void
CHtmLoad::SetGenericParse(BOOL fDoGeneric)
{
    if (_pHtmPre)
        _pHtmPre->SetGenericParse(fDoGeneric);
}
#endif


