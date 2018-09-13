//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       dwncache.cxx
//
//  Contents:   
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_BITS_HXX_
#define X_BITS_HXX_
#include "bits.hxx"
#endif

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnCache,     "Dwn", "! Trace DwnInfo cache")
PerfDbgTag(tagDwnCacheOff,  "Dwn", "! Disable DwnInfo cache")

MtDefine(GAryDwnInfo, PerProcess, "g_aryDwnInfo::_pv")
MtDefine(GAryDwnInfoCache, PerProcess, "g_aryDwnInfoCache::_pv")

// Types ----------------------------------------------------------------------

struct DWNINFOENTRY
{
    DWORD           dwKey;
    CDwnInfo *      pDwnInfo;
};

struct FINDINFO
{
    LPCTSTR         pchUrl;
    UINT            dt;
    DWORD           dwKey;
    UINT            cbUrl;
    UINT            iEnt;
    UINT            cEnt;
    CDwnInfo *      pDwnInfo;
    DWNINFOENTRY *  pde;
};

typedef CDataAry<DWNINFOENTRY> CDwnInfoAry;

// Globals --------------------------------------------------------------------

CDwnInfoAry         g_aryDwnInfo(Mt(GAryDwnInfo));            // Active references
CDwnInfoAry         g_aryDwnInfoCache(Mt(GAryDwnInfoCache));  // Cached references
ULONG               g_ulDwnInfoSize         = 0;
ULONG               g_ulDwnInfoItemThresh   =  128 * 1024;
ULONG               g_ulDwnInfoThreshBytes  = 1024 * 1024;
LONG                g_ulDwnInfoThreshCount  = 128;
DWORD               g_dwDwnInfoLru          = 0;

#define HashData BUGBUGHashData // avoid conflict with shlwapi
void                HashData(LPBYTE pbData, DWORD cbData,
                        LPBYTE pbHash, DWORD cbHash);


// Debugging ------------------------------------------------------------------

#if DBG==1

void
DwnCacheInvariant()
{
    DWNINFOENTRY * pde, * pdeT;
    UINT cEnt, cEntT;
    DWORD dwKey, dwKeyPrev, dwSizeT;

    // For each entry in the active array, verify that:
    //      - all the keys are in sorted order
    //      - each key is correct (is a hash of the URL)
    //      - no two entries with the same URL+type have the same _dwRefresh
    //      - each active item is not in the cache array
    //      - no _pDwnInfoLock item is in the cache array
    //      - no _pDwnInfoLock item has a _pDwnInfoLock != NULL
    //      - no _pDwnInfoLock item is incomplete

    pde       = g_aryDwnInfo;
    cEnt      = g_aryDwnInfo.Size();
    dwKeyPrev = 0;

    for (; cEnt > 0; --cEnt, dwKeyPrev = pde->dwKey, ++pde)
    {
        Assert(pde->dwKey >= dwKeyPrev);
        
        HashData((BYTE *)pde->pDwnInfo->GetUrl(),
            _tcslen(pde->pDwnInfo->GetUrl()) * sizeof(TCHAR),
            (BYTE *)&dwKey, sizeof(DWORD));

        Assert(pde->dwKey == dwKey);

        pdeT  = pde + 1;
        cEntT = cEnt - 1;

        for (; cEntT > 0; --cEntT, ++pdeT)
        {
            if (pdeT->dwKey != dwKey)
                break;

            if (    StrCmpC(pde->pDwnInfo->GetUrl(), pdeT->pDwnInfo->GetUrl()) == 0
                &&  pde->pDwnInfo->GetType() == pdeT->pDwnInfo->GetType())
            {
                Assert(!pde->pDwnInfo->AttachEarly(pde->pDwnInfo->GetType(),
                    pdeT->pDwnInfo->GetRefresh(),
                    pdeT->pDwnInfo->GetFlags(0xFFFFFFFF),
                    pdeT->pDwnInfo->GetBindf()));
            }
        }

        pdeT  = g_aryDwnInfoCache;
        cEntT = g_aryDwnInfoCache.Size();

        for (; cEntT > 0; --cEntT, ++pdeT)
        {
            Assert(pdeT->pDwnInfo != pde->pDwnInfo);
        }

        if (pde->pDwnInfo->GetDwnInfoLock())
        {
            pdeT  = g_aryDwnInfoCache;
            cEntT = g_aryDwnInfoCache.Size();
            
            for (; cEntT > 0; --cEntT, ++pdeT)
            {
                Assert(pdeT->pDwnInfo != pde->pDwnInfo->GetDwnInfoLock());
            }

            Assert(pde->pDwnInfo->GetDwnInfoLock()->GetDwnInfoLock() == NULL);
            Assert(pde->pDwnInfo->GetDwnInfoLock()->TstFlags(IMGLOAD_COMPLETE));
        }
    }

    // For each entry in the cache array, verify that:
    //      - all the keys are in sorted order
    //      - each key is correct (is a hash of the URL)
    //      - no two entries with the same URL+type exist in the cache
    //      - each cached item is not in the active array
    //      - each cached item is not locked by an active item
    //      - each cached item is complete
    //      - each cached item has the correct cache size
    //      - each cached item has _ulRefs == 0
    //      - each cached item has _pDwnInfoLock == NULL

    pde       = g_aryDwnInfoCache;
    cEnt      = g_aryDwnInfoCache.Size();
    dwKeyPrev = 0;
    dwSizeT   = 0;

    for (; cEnt > 0; --cEnt, ++pde)
    {
        dwSizeT += pde->pDwnInfo->GetCacheSize();

        Assert(pde->dwKey >= dwKeyPrev);
        
        HashData((BYTE *)pde->pDwnInfo->GetUrl(),
            _tcslen(pde->pDwnInfo->GetUrl()) * sizeof(TCHAR),
            (BYTE *)&dwKey, sizeof(DWORD));

        Assert(pde->dwKey == dwKey);

        pdeT  = pde + 1;
        cEntT = cEnt - 1;

        for (; cEntT > 0; --cEntT, ++pdeT)
        {
            if (pdeT->dwKey != dwKey)
                break;

            if (pde->pDwnInfo->GetType() == pdeT->pDwnInfo->GetType())
            {
                Assert(StrCmpC(pde->pDwnInfo->GetUrl(),
                    pdeT->pDwnInfo->GetUrl()) != 0);
            }
        }

        pdeT  = g_aryDwnInfo;
        cEntT = g_aryDwnInfo.Size();

        for (; cEntT > 0; --cEntT, ++pdeT)
        {
            Assert(pdeT->pDwnInfo != pde->pDwnInfo);
            Assert(pdeT->pDwnInfo->GetDwnInfoLock() != pde->pDwnInfo);
        }

        // The following assert was disabled (by dinartem) because it doesn't
        // always hold.  It is possible for CImgInfo::ComputeCacheSize to
        // return a higher value in the case where a transparency mask is
        // computed *after* the image is cached.  This can happen if an
        // animated GIF doesn't display all of its transparent frames before
        // it is cached.
        // 
        // Assert(pde->pDwnInfo->GetCacheSize() == pde->pDwnInfo->ComputeCacheSize());

        Assert(pde->pDwnInfo->TstFlags(DWNLOAD_COMPLETE));
        Assert(pde->pDwnInfo->GetRefs() == 0);
        Assert(pde->pDwnInfo->GetDwnInfoLock() == NULL);
    }

    Assert(dwSizeT == g_ulDwnInfoSize);
}

#endif

// Internal -------------------------------------------------------------------

BOOL
DwnCacheFind(CDwnInfoAry * pary, FINDINFO * pfi)
{
    DWNINFOENTRY *  pde;
    LPCTSTR         pchUrl;
    CDwnInfo *      pDwnInfo;
    DWORD           dwKey;
    UINT            cb1, cb2;

    if (pfi->pde == NULL)
    {
        DWNINFOENTRY *  pdeBase = *pary;
        LONG            iEntLo = 0, iEntHi = (LONG)pary->Size() - 1, iEnt;

        cb1 = pfi->cbUrl = _tcslen(pfi->pchUrl) * sizeof(TCHAR);

        HashData((BYTE *)pfi->pchUrl, pfi->cbUrl, (BYTE *)&dwKey, sizeof(DWORD));

        pfi->dwKey = dwKey;

        while (iEntLo <= iEntHi)
        {
            iEnt = (iEntLo + iEntHi) / 2;
            pde  = &pdeBase[iEnt];

            if (pde->dwKey == dwKey)
            {
                iEntLo = iEnt;

                while (iEnt > 0 && pde[-1].dwKey == dwKey)
                {
                    --iEnt, --pde;
                }
                
                pfi->iEnt = iEnt;
                pfi->cEnt = pary->Size() - iEnt - 1;
                pfi->pde  = pde;

                goto validate;
            }
            else if (pde->dwKey < dwKey)
                iEntLo = iEnt + 1;
            else
                iEntHi = iEnt - 1;
        }

        pfi->iEnt = iEntLo;

        return(FALSE);
    }
                
advance:

    if (pfi->cEnt == 0)
    {
        return(FALSE);
    }

    pfi->iEnt += 1;
    pfi->cEnt -= 1;
    pfi->pde  += 1;
    pde        = pfi->pde;
    cb1        = pfi->cbUrl;
    dwKey      = pfi->dwKey;

    if (pde->dwKey != dwKey)
    {
        return(FALSE);
    }

validate:

    pDwnInfo = pde->pDwnInfo;

    if (pDwnInfo->GetType() != pfi->dt)
        goto advance;

    pchUrl = pDwnInfo->GetUrl();
    cb2    = _tcslen(pchUrl) * sizeof(TCHAR);

    if (    cb1 != cb2
        ||  memcmp(pchUrl, pfi->pchUrl, cb1) != 0)
        goto advance;

    pfi->pDwnInfo = pDwnInfo;

    return(TRUE);
}

void
DwnCachePurge()
{
    LONG            cb, cbItem;
    UINT            iEnt, cEnt, iLru = 0;
    DWORD           dwLru, dwLruItem;
    DWNINFOENTRY *  pde;
    CDwnInfo *      pDwnInfo;

    cb   = (LONG)(g_ulDwnInfoSize - g_ulDwnInfoThreshBytes);
    cEnt = g_aryDwnInfoCache.Size();

    if (cb <= 0 && cEnt > (UINT)g_ulDwnInfoThreshCount)
    {
        Assert(cEnt - g_ulDwnInfoThreshCount == 1);
        cb = 1;
    }

    while (cb > 0 && cEnt > 0)
    {
        dwLru = 0xFFFFFFFF;
        pde   = g_aryDwnInfoCache;

        for (iEnt = 0; iEnt < cEnt; ++iEnt, ++pde)
        {
            pDwnInfo  = pde->pDwnInfo;
            dwLruItem = pDwnInfo->GetLru();

            if (dwLru > dwLruItem)
            {
                dwLru = dwLruItem;
                iLru  = iEnt;
            }
        }

        if (dwLru != 0xFFFFFFFF)
        {
            pDwnInfo = g_aryDwnInfoCache[iLru].pDwnInfo;
            cbItem   = pDwnInfo->GetCacheSize();
            
            cb -= cbItem;
            g_ulDwnInfoSize -= cbItem;

            g_aryDwnInfoCache.Delete(iLru);
            cEnt -= 1;

            PerfDbgLog4(tagDwnCache, pDwnInfo,
                "DwnCachePurge DelCache cb=%ld [n=%ld,t=%ld] %ls",
                cbItem, cEnt, g_ulDwnInfoSize, pDwnInfo->GetUrl());

            IncrementSecondaryObjectCount(10);

            pDwnInfo->SubRelease();

            #if DBG==1
            DwnCacheInvariant();
            #endif
        }
    }

    Assert(g_ulDwnInfoSize <= g_ulDwnInfoThreshBytes);
    Assert(g_aryDwnInfoCache.Size() <= g_ulDwnInfoThreshCount);
}

void
DwnCacheDeinit()
{
    DWNINFOENTRY * pde = g_aryDwnInfoCache;
    UINT cEnt = g_aryDwnInfoCache.Size();

    #if DBG==1
    DwnCacheInvariant();
    #endif

    for (; cEnt > 0; --cEnt, ++pde)
    {
        #if DBG==1
        g_ulDwnInfoSize -= pde->pDwnInfo->GetCacheSize();
        #endif

        PerfDbgLog4(tagDwnCache, pde->pDwnInfo,
            "DwnCacheDeinit DelCache cb=%ld [n=%ld,t=%ld] %ls",
            pde->pDwnInfo->GetCacheSize(), cEnt - 1, g_ulDwnInfoSize,
            pde->pDwnInfo->GetUrl());

        IncrementSecondaryObjectCount(10);

        pde->pDwnInfo->SubRelease();
    }

    Assert(g_ulDwnInfoSize == 0);
    g_aryDwnInfoCache.DeleteAll();

    AssertSz(g_aryDwnInfo.Size() == 0,
        "One or more CDwnInfo objects were leaked.  Most likely caused by "
        "leaking a CDoc or CImgElement.");

    g_aryDwnInfo.DeleteAll();
}

int
FtCompare(FILETIME * pft1, FILETIME * pft2)
{
    return(pft1->dwHighDateTime > pft2->dwHighDateTime ? 1 :
        (pft1->dwHighDateTime < pft2->dwHighDateTime ? -1 :
        (pft1->dwLowDateTime > pft2->dwLowDateTime ? 1 :
        (pft1->dwLowDateTime < pft2->dwLowDateTime ? -1 : 0))));
}

// External -------------------------------------------------------------------

HRESULT
CDwnInfo::Create(UINT dt, DWNLOADINFO * pdli, CDwnInfo ** ppDwnInfo)
{
    CDwnInfo *  pDwnInfo    = NULL;
    FINDINFO    fi;
    BOOL        fScanActive = FALSE;
    HRESULT     hr          = S_OK;

    g_csDwnCache.Enter();
    memset(&fi, 0, sizeof(FINDINFO));

    if (dt != DWNCTX_HTM && pdli->pchUrl && *pdli->pchUrl)
    {
        CDwnDoc * pDwnDoc = pdli->pDwnDoc;

        fi.pchUrl = pdli->pchUrl;
        fi.dt     = dt;

        if (fi.dt == DWNCTX_FILE)
            fi.dt = DWNCTX_BITS;

        while (DwnCacheFind(&g_aryDwnInfo, &fi))
        {
            fScanActive = TRUE;

            // If we have a bind-in-progress, don't attach early to an image in the cache.  We end
            // up abandoning the bind-in-progress and it never terminates properly.  We probably
            // could just hit it with an ABORT, but this case is not common enough to add yet
            // another state transition at this time (this case is when you hyperlink to a page
            // which turns out to actually be an image).

			if (pdli->fResynchronize || pdli->pDwnBindData)
				break;

            if (fi.pDwnInfo->AttachEarly(dt, pDwnDoc->GetRefresh(), pDwnDoc->GetDownf(), pDwnDoc->GetBindf()))
            {
                pDwnInfo = fi.pDwnInfo;
                pDwnInfo->AddRef();

                PerfDbgLog2(tagDwnCache, fi.pDwnInfo,
                    "DwnInfoCache AttachEarly %ld@%ls",
                    fi.pDwnInfo->GetRefresh(), fi.pDwnInfo->GetUrl());

                break;
            }
        }
    }

    if (pDwnInfo == NULL)
    {
        switch (dt)
        {
            case DWNCTX_HTM:
                pDwnInfo = new CHtmInfo();
                break;

            case DWNCTX_IMG:
                pDwnInfo = new CImgInfo();
                break;

            case DWNCTX_BITS:
            case DWNCTX_FILE:
                pDwnInfo = new CBitsInfo(dt);
                break;

            default:
                AssertSz(FALSE, "Unknown DWNCTX type");
                break;
        }

        if (pDwnInfo == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(pDwnInfo->Init(pdli));

        if (hr == S_OK && dt != DWNCTX_HTM && *pDwnInfo->GetUrl())
        {
            DWNINFOENTRY de = { fi.dwKey, pDwnInfo };
            hr = THR(g_aryDwnInfo.InsertIndirect(fi.iEnt, &de));

            if (hr == S_OK)
            {
                PerfDbgLog2(tagDwnCache, pDwnInfo,
                    "DwnInfoCache InsActive %ld@%ls",
                    pDwnInfo->GetRefresh(), pDwnInfo->GetUrl());

                if (    dt == DWNCTX_IMG
                    &&  (pDwnInfo->GetBindf() & (BINDF_GETNEWESTVERSION|BINDF_NOWRITECACHE|BINDF_RESYNCHRONIZE|BINDF_PRAGMA_NO_CACHE)) == 0
                    &&  !pdli->pDwnBindData)
                {
                    UINT uScheme = GetUrlScheme(pDwnInfo->GetUrl());

                    if (uScheme == URL_SCHEME_FILE || uScheme == URL_SCHEME_HTTP || uScheme == URL_SCHEME_HTTPS)
                    {
                        pDwnInfo->AttachByLastModEx(fScanActive, uScheme);
                    }
                }
            }
        }

        if (hr)
        {
            pDwnInfo->Release();
            goto Cleanup;
        }
    }

    #if DBG==1
    DwnCacheInvariant();
    #endif

    *ppDwnInfo = pDwnInfo;

Cleanup:

    g_csDwnCache.Leave();

    RRETURN(hr);
}

BOOL
CDwnInfo::AttachByLastMod(CDwnLoad * pDwnLoad, FILETIME * pft, BOOL fDoAttach)
{
    if (!_cstrUrl || !*_cstrUrl)
    {
        return(FALSE);
    }

    EnterCriticalSection();

    BOOL fRet = FALSE;

    if (pDwnLoad == _pDwnLoad)
    {
        _ftLastMod = *pft;

        if (fDoAttach)
        {
            g_csDwnCache.Enter();

            fRet = AttachByLastModEx(TRUE, URL_SCHEME_UNKNOWN);

            g_csDwnCache.Leave();
        }
    }

    LeaveCriticalSection();

    return(fRet);
}

BOOL
CDwnInfo::AttachByLastModEx(BOOL fScanActive, UINT uScheme)
{
    UINT        dt          = GetType();
    FINDINFO    fi          = { _cstrUrl, dt };
    CDwnInfo *  pDwnInfo    = NULL;
    BOOL        fGotLastMod = _ftLastMod.dwLowDateTime || _ftLastMod.dwHighDateTime;

    if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_IMGCACHE))
    {
        goto Cleanup;
    }

    if (fScanActive)
    {
        while (DwnCacheFind(&g_aryDwnInfo, &fi))
        {
            if (fi.pDwnInfo != this && fi.pDwnInfo->TstFlags(DWNLOAD_COMPLETE))
            {
                if (!fGotLastMod)
                {
                    if (!GetUrlLastModTime(_cstrUrl, uScheme, _dwBindf, &_ftLastMod))
                        goto Cleanup;

                    fGotLastMod = TRUE;
                }

                if (memcmp(&_ftLastMod, &fi.pDwnInfo->_ftLastMod, sizeof(FILETIME)) == 0)
                {
                    // If the active object is locking another object, then attach
                    // to the locked object in order to avoid chains of locked objects.

                    pDwnInfo = fi.pDwnInfo->_pDwnInfoLock;

                    if (pDwnInfo == NULL)
                        pDwnInfo = fi.pDwnInfo;

                    if (CanAttachLate(pDwnInfo))
                        pDwnInfo->SubAddRef();
                    else
                        pDwnInfo = NULL;

                    break;
                }
            }
        }
    }

    if (pDwnInfo == NULL)
    {
        memset(&fi, 0, sizeof(FINDINFO));
        fi.pchUrl = _cstrUrl;
        fi.dt     = dt;

        while (DwnCacheFind(&g_aryDwnInfoCache, &fi))
        {
            if (fi.pDwnInfo != this && CanAttachLate(fi.pDwnInfo))
            {
                if (!fGotLastMod)
                {
                    if (!GetUrlLastModTime(_cstrUrl, uScheme, _dwBindf, &_ftLastMod))
                        goto Cleanup;

                    fGotLastMod = TRUE;
                }
                
                if (memcmp(&_ftLastMod, &fi.pDwnInfo->_ftLastMod, sizeof(FILETIME)) == 0)
                {
                    Assert(!fi.pDwnInfo->_pDwnInfoLock);

                    pDwnInfo = fi.pDwnInfo;

                    g_ulDwnInfoSize -= pDwnInfo->GetCacheSize();
                    g_aryDwnInfoCache.Delete(fi.iEnt);

                    IncrementSecondaryObjectCount(10);

                    PerfDbgLog4(tagDwnCache, pDwnInfo,
                        "DwnInfoCache DelCache cb=%d [n=%ld,t=%ld] %ls",
                        pDwnInfo->GetCacheSize(), g_aryDwnInfoCache.Size(),
                        g_ulDwnInfoSize, pDwnInfo->GetUrl());

                    break;
                }
            }
        }
    }

    #if DBG==1
    DwnCacheInvariant();
    #endif

    if (pDwnInfo)
    {
        PerfDbgLog5(tagDwnCache, this,
            "DwnInfoCache %s %lX (%s) %ld@%ls",
            uScheme == URL_SCHEME_UNKNOWN ? "AttachLate" : "AttachEarlyByLastMod",
            pDwnInfo, pDwnInfo->GetRefs() ? "active" : "cached",
            GetRefresh(), GetUrl());

        AttachLate(pDwnInfo);
        pDwnInfo->SubRelease();
    }

Cleanup:
    return(!!pDwnInfo);
}

ULONG
CDwnInfo::Release()
{
    g_csDwnCache.Enter();

    ULONG ulRefs = (ULONG)InterlockedRelease();

    if (ulRefs == 0 && _cstrUrl && *_cstrUrl)
    {
        FINDINFO    fi       = { _cstrUrl, GetType() };
        CDwnInfo *  pDwnInfo = NULL;
        DWORD       cbSize;
        UINT        iEntDel  = 0;
        BOOL        fDeleted = FALSE;

        // We want to cache this object if it is completely loaded and
        // we either own the bits (don't have a lock on another object)
        // or we are locking another object, but it has no active references.
        // In the latter case, we actually want to cache the locked object,
        // not this object, because we don't allow entries in the cache which
        // have locks on other objects.

        if (!TstFlags(DWNLOAD_COMPLETE))
        {
            PerfDbgLog1(tagDwnCache, this,
                "DwnInfoCache Release (no cache; not complete) %ls",
                GetUrl());
        }
        else if (TstFlags(DWNF_DOWNLOADONLY))
        {
            PerfDbgLog1(tagDwnCache, this,
                "DwnInfoCache Release (no cache; download only) %ls",
                GetUrl());
        }
        else if (_ftLastMod.dwLowDateTime == 0 && _ftLastMod.dwHighDateTime == 0)
        {
            PerfDbgLog1(tagDwnCache, this,
                "DwnInfoCache Release (no cache; no _ftLastMod) %ls",
                GetUrl());
        }
        else if (_pDwnInfoLock)
        {
            if (_pDwnInfoLock->GetRefs() > 0)
            {
                PerfDbgLog1(tagDwnCache, this,
                    "DwnInfoCache Release (no cache; locked and target is active) %ls",
                    GetUrl());
            }
            else
            {
                pDwnInfo = _pDwnInfoLock;
                pDwnInfo->SubAddRef();
            }
        }
#ifdef SWITCHES_ENABLED
        else if (IsSwitchNoImageCache())
        {
        }
#endif
#if DBG==1 || defined(PERFTAGS)
        else if (IsPerfDbgEnabled(tagDwnCacheOff))
        {
        }
#endif
        else if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_IMGCACHE))
        {
        }
        else
        {
            pDwnInfo = this;
            SubAddRef();
        }

        while (DwnCacheFind(&g_aryDwnInfo, &fi))
        {
            if (fi.pDwnInfo == this)
            {
                PerfDbgLog2(tagDwnCache, fi.pDwnInfo,
                    "DwnInfoCache DelActive %ld@%ls",
                    fi.pDwnInfo->GetRefresh(), fi.pDwnInfo->GetUrl());

                iEntDel = fi.iEnt;
                fDeleted = TRUE;

                if (pDwnInfo == NULL)
                    break;
            }
            else if (pDwnInfo && fi.pDwnInfo->_pDwnInfoLock == pDwnInfo)
            {
                // Some other active object has the target locked.  We don't
                // put it into the cache because it is still accessible
                // through that active object.

                PerfDbgLog2(tagDwnCache, this,
                    "DwnInfoCache Release (no cache; active item %lX has us locked) %ls",
                    fi.pDwnInfo, GetUrl());

                pDwnInfo->SubRelease();
                pDwnInfo = NULL;

                if (fDeleted)
                    break;
            }
        }

        if (fDeleted)
        {
            g_aryDwnInfo.Delete(iEntDel);
        }

        AssertSz(fDeleted, "Can't find reference to CDwnInfo in active array");

        if (pDwnInfo)
        {
            cbSize = pDwnInfo->ComputeCacheSize();

            if (cbSize == 0)
            {
                PerfDbgLog1(tagDwnCache, this,
                    "DwnInfoCache Release (no cache; ComputeCacheSize declined) %ls",
                    GetUrl());
            }
            else if (cbSize > g_ulDwnInfoItemThresh)
            {
                PerfDbgLog3(tagDwnCache, this,
                    "DwnInfoCache Release (no cache; cb=%lX is too big) (max %lX) %ls",
                    cbSize, g_ulDwnInfoItemThresh, GetUrl());
            }
            else
            {
                pDwnInfo->SetCacheSize(cbSize);
                pDwnInfo->SetLru(++g_dwDwnInfoLru);

                memset(&fi, 0, sizeof(FINDINFO));
                fi.pchUrl = pDwnInfo->GetUrl();
                fi.dt     = pDwnInfo->GetType();
                
                while (DwnCacheFind(&g_aryDwnInfoCache, &fi))
                {
                    if (FtCompare(&fi.pDwnInfo->_ftLastMod, &pDwnInfo->_ftLastMod) > 0)
                    {
                        // The current entry in the cache has a newer mod date
                        // then the one we're trying to add.  Forget about
                        // ours.

                        PerfDbgLog1(tagDwnCache, this,
                            "DwnInfoCache Release (no cache; existing item is newer) %ls",
                            GetUrl());

                        goto endcache;
                    }

                    // Replace the older entry with this newer one.

                    g_ulDwnInfoSize -= fi.pDwnInfo->GetCacheSize();

                    PerfDbgLog4(tagDwnCache, fi.pDwnInfo,
                        "DwnInfoCache DelCache cb=%d [n=%ld,t=%ld] %ls",
                        fi.pDwnInfo->GetCacheSize(), g_aryDwnInfoCache.Size(),
                        g_ulDwnInfoSize, fi.pDwnInfo->GetUrl());

                    fi.pDwnInfo->SubRelease();
                    fi.pDwnInfo = pDwnInfo;
                    g_aryDwnInfoCache[fi.iEnt].pDwnInfo = pDwnInfo;
                    pDwnInfo = NULL;

                    g_ulDwnInfoSize += cbSize;

                    PerfDbgLog4(tagDwnCache, fi.pDwnInfo,
                        "DwnInfoCache InsCache cb=%d [n=%ld,t=%ld] %ls",
                        cbSize, g_aryDwnInfoCache.Size(), g_ulDwnInfoSize,
                        fi.pDwnInfo->GetUrl());

                    if (g_ulDwnInfoSize > g_ulDwnInfoThreshBytes)
                    {
                        DwnCachePurge();
                    }

                    #if DBG==1
                    DwnCacheInvariant();
                    #endif

                    goto endcache;
                }

                // No matching entry found, but now we know where to insert.

                DWNINFOENTRY de = { fi.dwKey, pDwnInfo };

                if (g_aryDwnInfoCache.InsertIndirect(fi.iEnt, &de) == S_OK)
                {
                    // Each CDwnInfo stored in the cache maintains a secondary
                    // reference count (inheriting from CBaseFT).  Since this
                    // object is now globally cached without active references,
                    // it should no longer maintain its secondary reference
                    // because it will be destroyed when the DLL is unloaded.

                    DecrementSecondaryObjectCount(10);
                    pDwnInfo = NULL;

                    g_ulDwnInfoSize += cbSize;

                    PerfDbgLog4(tagDwnCache, de.pDwnInfo,
                        "DwnInfoCache InsCache cb=%d [n=%ld,t=%ld] %ls",
                        cbSize, g_aryDwnInfoCache.Size(), g_ulDwnInfoSize,
                        de.pDwnInfo->GetUrl());

                    #if DBG==1
                    DwnCacheInvariant();
                    #endif

                    if (    g_ulDwnInfoSize > g_ulDwnInfoThreshBytes
                        ||  g_aryDwnInfoCache.Size() > g_ulDwnInfoThreshCount)
                    {
                        DwnCachePurge();
                    }
                }
            }

        endcache:

            if (pDwnInfo)
            {
                pDwnInfo->SubRelease();
                pDwnInfo = NULL;
            }

            #if DBG==1
            DwnCacheInvariant();
            #endif
        }
    }

    g_csDwnCache.Leave();

    if (ulRefs == 0)
    {
        Passivate();
        SubRelease();
    }

    return(ulRefs);
}
