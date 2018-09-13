//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       imgctx.cxx
//
//  Contents:   CImgCtx
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

// Debugging ------------------------------------------------------------------

DeclareTag(tagNoPreTile, "DocBack", "No Pre-Tile");
MtDefine(CImgCtx, Dwn, "CImgCtx")

// Definitions ----------------------------------------------------------------

#define PRETILE_AREA      (16 * 1024)

// CImgCtx --------------------------------------------------------------------

CImgCtx::CImgCtx()
{
    _yTop = -1;
}

// CImgCtx (IUnknown) ---------------------------------------------------------

STDMETHODIMP
CImgCtx::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IImgCtx || riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
        ((LPUNKNOWN)*ppv)->AddRef();
        return(S_OK);
    }
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG)
CImgCtx::AddRef()
{
    return(super::AddRef());
}

STDMETHODIMP_(ULONG)
CImgCtx::Release()
{
    return(super::Release());
}

// CImgCtx (IImgCtx) ----------------------------------------------------------

STDMETHODIMP
CImgCtx::Load(LPCWSTR pszUrl, DWORD dwFlags)
{
    CDwnDoc *   pDwnDoc  = NULL;
    CDwnInfo *  pDwnInfo = NULL;
    DWORD       dwBindf  = 0;
    DWNLOADINFO dli      = { 0 };
    HRESULT     hr;

    pDwnDoc = new CDwnDoc;

    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if ((dwFlags & DWNF_COLORMODE) == 0)
    {
        dwFlags |= GetDefaultColorMode();
    }

    dli.pDwnDoc   = pDwnDoc;
    dli.pInetSess = TlsGetInternetSession();
    dli.pchUrl    = pszUrl;

    pDwnDoc->SetRefresh(IncrementLcl());

    // This is a bit of a hack to allow our performance measurements to
    // be able to disable WinInet write caching when using IImgCtx.  We
    // don't expect anyone to really do this in the real world, but it
    // doesn't do any harm if they do.

    if (dwFlags & 0x80000000)
    {
        dwBindf |= BINDF_NOWRITECACHE;
    }

    if (dwFlags & 0x40000000)
    {
        dwBindf |= BINDF_GETNEWESTVERSION;
    }

    pDwnDoc->SetBindf(dwBindf);
    pDwnDoc->SetDocBindf(dwBindf); // (should be irrelevant)
    pDwnDoc->SetDownf(dwFlags & ~DWNF_STATE | DWNF_NOOPTIMIZE);

    hr = THR(CDwnInfo::Create(DWNCTX_IMG, &dli, &pDwnInfo));
    if (hr)
        goto Cleanup;

    pDwnInfo->AddDwnCtx(this);

    SetLoad(TRUE, &dli, FALSE);

Cleanup:
    if (pDwnInfo)
        pDwnInfo->Release();
    if (pDwnDoc)
        pDwnDoc->Release();

    RRETURN(hr);
}

STDMETHODIMP
CImgCtx::SelectChanges(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal)
{
    WORD wNewChg = 0;

    EnterCriticalSection();

    _wChgReq &= (WORD)~ulChgOff;

    if (fSignal)
    {
        DWORD dwState = GetImgInfo()->GetFlags(DWNF_STATE);

        if (    (GetImgInfo()->_xWid || GetImgInfo()->_yHei)
            &&  !(_wChgReq & IMGCHG_SIZE)
            &&  (ulChgOn & IMGCHG_SIZE))
        {
            wNewChg |= IMGCHG_SIZE;
        }

        if (    (dwState & (IMGLOAD_COMPLETE | IMGLOAD_ERROR | IMGLOAD_STOPPED))
            &&  !(_wChgReq & IMGCHG_COMPLETE)
            &&  (ulChgOn & IMGCHG_COMPLETE))
        {
            wNewChg |= IMGCHG_COMPLETE;
        }

        if (    (dwState & IMGLOAD_COMPLETE)
            &&  !(_wChgReq & IMGCHG_VIEW)
            &&  (ulChgOn & IMGCHG_VIEW))
        {
            wNewChg |= IMGCHG_VIEW;
        }

        if (    (dwState & IMGANIM_ANIMATED)
            &&  !(_wChgReq & IMGCHG_ANIMATE)
            &&  (ulChgOn & IMGCHG_ANIMATE))
        {
            wNewChg |= IMGCHG_ANIMATE;
        }
    }

    _wChgReq |= (WORD)ulChgOn;
    
    if (wNewChg)
    {
        super::Signal(wNewChg);
    }

    LeaveCriticalSection();

    return(S_OK);
}

STDMETHODIMP
CImgCtx::SetCallback(PFNIMGCTXCALLBACK pfn, void * pvPrivateData)
{
    super::SetCallback(pfn, pvPrivateData);
    return(S_OK);
}

STDMETHODIMP
CImgCtx::Disconnect()
{
    super::Disconnect();
    return(S_OK);
}

STDMETHODIMP
CImgCtx::GetUpdateRects(RECT *prc, RECT *prectImg, LONG *pcrc)
{
    int logicalRow0;
    int logicalRowN;
    RECT updateRect;
    int nDestLogicalRow0, nDestLogicalRowN;
    int nDestHeight = prectImg->bottom - prectImg->top;
    int height = GetImgInfo()->_yHei;
    int nrc = 0;

    if (_yTop == -1)
    {
        prc[nrc++] = *prectImg;
        goto exit;
    }
    else
    {
        logicalRow0 = _yTop;
        logicalRowN = _yBot;
    }

    if ((_yTop == _yBot) || (height == 0)) {
        *pcrc = 0;
        return S_OK;
    }

    nDestLogicalRow0 = (int) (((long) logicalRow0 * nDestHeight) / height);
    nDestLogicalRowN = (int) (((long) logicalRowN * nDestHeight) / height);
    if (nDestLogicalRow0 != 0 && (((long) logicalRow0 * nDestHeight) % height) )
        nDestLogicalRow0--;
    if ((((long) logicalRowN * nDestHeight) % height))
        nDestLogicalRowN++;
    updateRect.left = prectImg->left;
    updateRect.right = prectImg->right;

    if (_yBot > _yTop)
    {
        updateRect.top  = prectImg->top + nDestLogicalRow0;
        updateRect.bottom = prectImg->top + nDestLogicalRowN + 1; 
        if (updateRect.bottom > prectImg->bottom)
            updateRect.bottom = prectImg->bottom;
        prc[nrc++] = updateRect;
    }
    else
    {
        updateRect.top = prectImg->top + nDestLogicalRow0;
        updateRect.bottom = prectImg->bottom;
        prc[nrc++] = updateRect;

        updateRect.top = prectImg->top;
        updateRect.bottom = prectImg->top + nDestLogicalRowN + 1;
        prc[nrc++] = updateRect;
    }

exit:
    _yTop = _yBot;
    *pcrc = nrc;
    return S_OK;
}


STDMETHODIMP
CImgCtx::GetStateInfo(ULONG *pulState, SIZE *psize, BOOL fClear)
{
    *pulState = GetState(fClear, psize);
    return(S_OK);
}

STDMETHODIMP
CImgCtx::GetPalette(HPALETTE *phpal)
{
    if (phpal == NULL)
    {
        return(E_INVALIDARG);
    }

    *phpal = g_hpalHalftone;
    return(S_OK);
}

STDMETHODIMP
CImgCtx::Draw(HDC hdc, RECT * prcDst)
{
    GetImgInfo()->DrawImage(hdc, prcDst, NULL, SRCCOPY, DRAWIMAGE_NHPALETTE);
    return(S_OK);
}

STDMETHODIMP
CImgCtx::Tile(HDC hdc, POINT * pptOrg, RECT * prc, SIZE * psizePrint)
{
    Tile(hdc, pptOrg, prc, psizePrint, COLORREF_NONE, NULL, DRAWIMAGE_NHPALETTE);
    return(S_OK);
}

STDMETHODIMP
CImgCtx::StretchBlt(HDC hdc, int dstX, int dstY, int dstXE, int dstYE, int srcX, int srcY, int srcXE, int srcYE, DWORD dwROP)
{
    RECT rcSrc, rcDest;

    if (dstXE < 0 || dstYE < 0)
        return E_FAIL;
        
    rcSrc.left = srcX;
    rcSrc.top = srcY;
    rcSrc.right = srcX + srcXE;
    rcSrc.bottom = srcY + srcYE;

    rcDest.left = dstX;
    rcDest.top = dstY;
    rcDest.right = dstX + dstXE;
    rcDest.bottom = dstY + dstYE;

    return GetImgInfo()->DrawImage(hdc, &rcDest, &rcSrc, dwROP, DRAWIMAGE_NHPALETTE);
}

HRESULT
CImgCtx::DrawEx(HDC hdc, RECT * prcDst, DWORD dwFlags)
{
    GetImgInfo()->DrawImage(hdc, prcDst, NULL, SRCCOPY, dwFlags);
    return(S_OK);
}


// CImgCtx (Animation) --------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CImgCtx::InitImgAnimState
//
//-------------------------------------------------------------------------
void CImgCtx::InitImgAnimState(IMGANIMSTATE * pImgAnimState)
{
    GetImgInfo()->InitImgAnimState(pImgAnimState);
}

BOOL CImgCtx::NextFrame(IMGANIMSTATE *pImgAnimState, DWORD dwCurTimeMS, DWORD *pdwFrameTimeMS)
{
    return GetImgInfo()->NextFrame(pImgAnimState, dwCurTimeMS, pdwFrameTimeMS);
}

//+------------------------------------------------------------------------
//
//  Member:     CImgCtx::DrawFrame
//
//  Synopsis:   
//
//-------------------------------------------------------------------------

void CImgCtx::DrawFrame(HDC hdc, IMGANIMSTATE * pImgAnimState, RECT * prcDst,
    RECT * prcSrc, RECT *prcDstFull, DWORD dwFlags)
{
    GetImgInfo()->DrawFrame(hdc, pImgAnimState, prcDst, prcSrc, prcDstFull, dwFlags);
}

// CImgCtx (Public) -----------------------------------------------------------

void
CImgCtx::Tile(HDC hdc, POINT * pptOrg, RECT * prc, SIZE * psizePrint,
    COLORREF crBack, IMGANIMSTATE * pImgAnimState, DWORD dwFlags)
{
    BOOL fOpaque = !!(GetState() & IMGTRANS_OPAQUE);

    // If the source image is 1x1 just let the normal StretchBlt mechanism
    // fill up the destination.

    if (GetImgInfo()->_xWid == 1 && GetImgInfo()->_yHei == 1)
    {
        if (crBack != COLORREF_NONE)
        {
            PatBltBrush(hdc, prc, PATCOPY, crBack);
        }

        if (pImgAnimState)
        {
            GetImgInfo()->DrawFrame(hdc, pImgAnimState, prc, NULL, NULL, dwFlags);
        }
        else
        {
            GetImgInfo()->DrawImage(hdc, prc, NULL, SRCCOPY, dwFlags);
        }

        goto Cleanup;
    }

    // If the image is opaque and we are writing into an offscreen bitmap
    // which is not clipped, just tile directly into it.

    if (    psizePrint == NULL
        &&  (fOpaque || crBack != COLORREF_NONE)
        &&  GetObjectType(hdc) == OBJ_MEMDC)
    {
        GDIRECT rcBox;
        int iRgn = GetClipBox(hdc, &rcBox);

        if (    iRgn == NULLREGION
            ||  (   iRgn == SIMPLEREGION
                &&  prc->left >= rcBox.left
                &&  prc->top >= rcBox.top
                &&  prc->right <= rcBox.right
                &&  prc->bottom <= rcBox.bottom))
        {
            TileFast(hdc, prc, pptOrg->x, pptOrg->y, fOpaque, crBack, pImgAnimState, dwFlags);
            goto Cleanup;
        }
    }

    // Otherwise just tile the slow way (it may still decide to pretile)

    TileSlow(hdc, prc, pptOrg->x, pptOrg->y, psizePrint, fOpaque, crBack, pImgAnimState, dwFlags);

Cleanup:
    ;
}

void
CImgCtx::TileFast(HDC hdc, RECT * prc, LONG xDstOrg, LONG yDstOrg,
    BOOL fOpaque, COLORREF crBack, IMGANIMSTATE * pImgAnimState, DWORD dwFlags)
{
    LONG xDst    = prc->left;
    LONG yDst    = prc->top;
    LONG xDstWid = prc->right - xDst;
    LONG yDstHei = prc->bottom - yDst;
    LONG xSrcWid = GetImgInfo()->_xWid;
    LONG ySrcHei = GetImgInfo()->_yHei;
    LONG xWid, yHei, xSrcOrg, ySrcOrg;
    LONG xBltSrc, xBltSrcWid, xBltDst, xBltDstWid, xBltWid;
    LONG yBltSrc, yBltSrcHei, yBltDst, yBltDstHei, yBltHei;
    RECT rcSrc, rcDst, rcDstFull;
    HRESULT hr = S_OK;

    if (xSrcWid == 0 || ySrcHei == 0 || xDstWid == 0 || yDstHei == 0)
        return;

    if (!fOpaque && crBack != COLORREF_NONE)
    {
        PatBltBrush(hdc, prc, PATCOPY, crBack);
    }

    // Currently (xDstOrg,yDstOrg) define a point on the infinite plane of 
    // the hdc where the upper-left corner of the image should be aligned.
    // Here we convert this point into offsets from the upper-left corner
    // of the image where the first pixel will be drawn as defined by prc.
    // That is, what is the coordinate of the pixel in the image which
    // will be drawn at the location (xDst,yDst).

    xSrcOrg = abs(xDst - xDstOrg) % xSrcWid;
    if (xDst < xDstOrg && xSrcOrg > 0)
        xSrcOrg = xSrcWid - xSrcOrg;

    ySrcOrg = abs(yDst - yDstOrg) % ySrcHei;
    if (yDst < yDstOrg && ySrcOrg > 0)
        ySrcOrg = ySrcHei - ySrcOrg;

    // Now that we know how the tiling is going to start, we need to draw
    // the image onto the hdc up to four times in order to get a prototypical
    // image of size (xSrcWid,ySrcHei) which is rotated in both x and y
    // dimensions to the desired tiling alignment.  If either xOrg or yOrg is
    // zero, no rotation is necessary in that dimension.

    // The first block to draw starts at (xSrcOrg,ySrcOrg) inside the image.
    // Draw it at (xDst,yDst).

    xWid = min(xSrcWid - xSrcOrg, xDstWid);
    yHei = min(ySrcHei - ySrcOrg, yDstHei);

    if (xWid > 0 && yHei > 0)
    {
        rcSrc.left       = xSrcOrg;
        rcSrc.top        = ySrcOrg;
        rcDst.left       = xDst;
        rcDst.top        = yDst;
        rcDstFull.left   = xDst - xSrcOrg;
        rcDstFull.top    = yDst - ySrcOrg;
        rcSrc.right      = rcSrc.left + xWid;
        rcSrc.bottom     = rcSrc.top  + yHei;
        rcDst.right      = rcDst.left + xWid;
        rcDst.bottom     = rcDst.top  + yHei;
        rcDstFull.right  = rcDstFull.left + xSrcWid;
        rcDstFull.bottom = rcDstFull.top  + ySrcHei;

        if (pImgAnimState)
        {
            GetImgInfo()->DrawFrame(hdc, pImgAnimState, &rcDst, &rcSrc, &rcDstFull, dwFlags);
        }
        else
        {
            hr = THR(GetImgInfo()->DrawImage(hdc, &rcDst, &rcSrc, SRCCOPY, dwFlags));
            if (hr)
                goto Cleanup;
        }
        
    }

    // The second block has the same width, but is drawn directly below
    // the first and starts at (xSrcOrg,0) in the image.

    xWid = min(xSrcWid - xSrcOrg, xDstWid);
    yHei = min(ySrcOrg, yDstHei - (ySrcHei - ySrcOrg));

    if (xWid > 0 && yHei > 0)
    {
        rcSrc.left   = xSrcOrg;
        rcSrc.top    = 0;
        rcDst.left   = xDst;
        rcDst.top    = yDst + (ySrcHei - ySrcOrg);
        rcDstFull.left   = xDst - xSrcOrg;
        rcDstFull.top    = rcDst.top;
        rcSrc.right  = rcSrc.left + xWid;
        rcSrc.bottom = rcSrc.top + yHei;
        rcDst.right  = rcDst.left + xWid;
        rcDst.bottom = rcDst.top + yHei;
        rcDstFull.right  = rcDstFull.left + xSrcWid;
        rcDstFull.bottom = rcDstFull.top  + ySrcHei;

        if (pImgAnimState)
        {
            GetImgInfo()->DrawFrame(hdc, pImgAnimState, &rcDst, &rcSrc, &rcDstFull, dwFlags);
        }
        else
        {
            hr = THR(GetImgInfo()->DrawImage(hdc, &rcDst, &rcSrc, SRCCOPY, dwFlags));
            if (hr)
                goto Cleanup;
        }
    }
    
    // The third block has the same height as the first, but is draw directly
    // to the right of the first and starts at (0,ySrcOrg) inside the image.

    xWid = min(xSrcOrg, xDstWid - (xSrcWid - xSrcOrg));
    yHei = min(ySrcHei - ySrcOrg, yDstHei);

    if (xWid > 0 && yHei > 0)
    {
        rcSrc.left   = 0;
        rcSrc.top    = ySrcOrg;
        rcDst.left   = xDst + (xSrcWid - xSrcOrg);
        rcDst.top    = yDst;
        rcDstFull.left   = rcDst.left;
        rcDstFull.top    = yDst - ySrcOrg;
        rcSrc.right  = rcSrc.left + xWid;
        rcSrc.bottom = rcSrc.top + yHei;
        rcDst.right  = rcDst.left + xWid;
        rcDst.bottom = rcDst.top + yHei;
        rcDstFull.right  = rcDstFull.left + xSrcWid;
        rcDstFull.bottom = rcDstFull.top  + ySrcHei;

        if (pImgAnimState)
        {
            GetImgInfo()->DrawFrame(hdc, pImgAnimState, &rcDst, &rcSrc, &rcDstFull, dwFlags);
        }
        else
        {
            hr = THR(GetImgInfo()->DrawImage(hdc, &rcDst, &rcSrc, SRCCOPY, dwFlags));
            if (hr)
                goto Cleanup;
        }
    }

    // The fourth block has the same width as the third and the same height
    // as the second and starts at (0,0) inside the image.

    xWid = min(xSrcOrg, xDstWid - (xSrcWid - xSrcOrg));
    yHei = min(ySrcOrg, yDstHei - (ySrcHei - ySrcOrg));

    if (xWid > 0 && yHei > 0)
    {
        rcSrc.left   = 0;
        rcSrc.top    = 0;
        rcDst.left   = xDst + (xSrcWid - xSrcOrg);
        rcDst.top    = yDst + (ySrcHei - ySrcOrg);
        rcDstFull.left   = rcDst.left;
        rcDstFull.top    = rcDst.top;
        rcSrc.right  = rcSrc.left + xWid;
        rcSrc.bottom = rcSrc.top + yHei;
        rcDst.right  = rcDst.left + xWid;
        rcDst.bottom = rcDst.top + yHei;
        rcDstFull.right  = rcDstFull.left + xSrcWid;
        rcDstFull.bottom = rcDstFull.top  + ySrcHei;

        if (pImgAnimState)
        {
            GetImgInfo()->DrawFrame(hdc, pImgAnimState, &rcDst, &rcSrc, &rcDstFull, dwFlags);
        }
        else
        {
            hr = THR(GetImgInfo()->DrawImage(hdc, &rcDst, &rcSrc, SRCCOPY, dwFlags));
            if (hr)
                goto Cleanup;
        }
    }

    // At this point we've draw the entire source image into the destination
    // at the correct tiling alignment.  Now we want to duplicate this
    // copy horizontally to fill the entire xDstWid.

    yHei = min(ySrcHei, yDstHei);

    if (xDstWid > xSrcWid)
    {
        xBltSrc    = xDst;
        xBltSrcWid = xSrcWid;
        xBltDst    = xDst + xSrcWid;
        xBltDstWid = xDstWid - xSrcWid;

        while (xBltDstWid)
        {
            xBltWid     = min(xBltSrcWid, xBltDstWid);

            BitBlt(hdc, xBltDst, yDst, xBltWid, yHei,
                   hdc, xBltSrc, yDst, SRCCOPY);

            xBltDst    += xBltWid;
            xBltDstWid -= xBltWid;
            xBltSrcWid *= 2;
        }
    }

    // The last step is to copy the fully tiled horizontal scanlines
    // vertically to fill the desitination.
    
    if (yDstHei > ySrcHei)
    {
        yBltSrc    = yDst;
        yBltSrcHei = ySrcHei;
        yBltDst    = yDst + ySrcHei;
        yBltDstHei = yDstHei - ySrcHei;

        while (yBltDstHei)
        {
            yBltHei    = min(yBltSrcHei, yBltDstHei);

            BitBlt(hdc, xDst, yBltDst, xDstWid, yBltHei,
                   hdc, xDst, yBltSrc, SRCCOPY);

            yBltDst    += yBltHei;
            yBltDstHei -= yBltHei;
            yBltSrcHei *= 2;
        }
    }

Cleanup: ;
} 

void
CImgCtx::TileSlow(HDC hdc, RECT * prc, LONG xDstOrg, LONG yDstOrg,
    SIZE * psizePrint, BOOL fOpaque, COLORREF crBack,
    IMGANIMSTATE * pImgAnimState, DWORD dwFlags)
{
    LONG    xDst    = prc->left;
    LONG    yDst    = prc->top;
    LONG    xDstWid = prc->right - xDst;
    LONG    yDstHei = prc->bottom - yDst;
    LONG    xFullWid= GetImgInfo()->_xWid;
    LONG    yFullHei= GetImgInfo()->_yHei;
    LONG    xSrcWid = psizePrint ? psizePrint->cx : xFullWid;
    LONG    ySrcHei = psizePrint ? psizePrint->cy : yFullHei;
    LONG    xPreWid, yPreHei, xSrcOrg, ySrcOrg;
    LONG    xBltSrc, xBltDst, xBltDstWid, xBltWid, xBltSrcOrg;
    LONG    yBltSrc, yBltDst, yBltDstHei, yBltHei, yBltSrcOrg;
    RECT    rcSrc, rcDst, rcDstFull;
    HDC     hdcMem = NULL;
    HBITMAP hbmMem = NULL;
    HBITMAP hbmSav = NULL;
    HRESULT hr = S_OK;

    if (xSrcWid == 0 || ySrcHei == 0 || xDstWid == 0 || yDstHei == 0)
        return;

    // Currently (xSrcOrg,ySrcOrg) define a point on the infinite plane of 
    // the hdc where the upper-left corner of the image should be aligned.
    // Here we convert this point into offsets from the upper-left corner
    // of the image where the first pixel will be drawn as defined by prc.
    // That is, what is the coordinate of the pixel in the image which
    // will be drawn at the location (xDst,yDst).

    xSrcOrg = abs(xDst - xDstOrg) % xSrcWid;
    if (xDst < xDstOrg && xSrcOrg > 0)
        xSrcOrg = xSrcWid - xSrcOrg;

    ySrcOrg = abs(yDst - yDstOrg) % ySrcHei;
    if (yDst < yDstOrg && ySrcOrg > 0)
        ySrcOrg = ySrcHei - ySrcOrg;

    // If the source image is very small, it makes sense to pre-tile it
    // into an offscreen bitmap.  The area of the destination needs to
    // be at least four times the area of the source.

    #if DBG==1
    xPreWid = 0;
    yPreHei = 0;
    if (IsTagEnabled(tagNoPreTile))
        goto nopretile;
    #endif

    xPreWid = min(xSrcWid, xDstWid);
    yPreHei = min(ySrcHei, yDstHei);

    if (    psizePrint
        ||  xPreWid * yPreHei >= PRETILE_AREA
        ||  xPreWid * yPreHei * 4 > xDstWid * yDstHei
        ||  (!fOpaque && crBack == COLORREF_NONE))
    {
        if (!psizePrint && !fOpaque && crBack != COLORREF_NONE)
        {
            PatBltBrush(hdc, prc, PATCOPY, crBack);
        }
        goto nopretile;
    }

    // Increase the dimensions of the pretile area as far as possible

    xPreWid = max(xPreWid, min(xDstWid, (PRETILE_AREA / (yPreHei * xSrcWid)) * xSrcWid));
    yPreHei = max(yPreHei, min(yDstHei, (PRETILE_AREA / (xPreWid * ySrcHei)) * ySrcHei));

    Assert(xPreWid * yPreHei <= PRETILE_AREA);
    Assert(xPreWid > 0 && xPreWid <= xDstWid);
    Assert(yPreHei > 0 && yPreHei <= yDstHei);

    hdcMem = GetMemoryDC();

    if (hdcMem == NULL)
        goto Cleanup;

    hbmMem = CreateCompatibleBitmap(hdc, xPreWid, yPreHei);

    if (hbmMem == NULL)
        goto Cleanup;

    hbmSav = (HBITMAP)SelectObject(hdcMem, hbmMem);

    rcDst.left   = 0;
    rcDst.top    = 0;
    rcDst.right  = xPreWid;
    rcDst.bottom = yPreHei;

    TileFast(hdcMem, &rcDst, xSrcWid - xSrcOrg, ySrcHei - ySrcOrg,
        fOpaque, crBack, pImgAnimState, dwFlags);

    xSrcOrg = 0;
    ySrcOrg = 0;
    xSrcWid = xPreWid;
    ySrcHei = yPreHei;

nopretile:

    if (psizePrint)
    {
        // Remember the original image source sizes in xPreWid/yPreHei in
        // order to compute the rcSrc in pixels below.

        xPreWid = GetImgInfo()->_xWid;
        yPreHei = GetImgInfo()->_yHei;

        if (xPreWid == 0 || yPreHei == 0)
            return;
    }

    yBltDst    = yDst;
    yBltDstHei = yDstHei;
    yBltSrcOrg = ySrcOrg;

    while (yBltDstHei)
    {
        yBltSrc     = yBltSrcOrg;
        yBltHei     = min(ySrcHei - yBltSrcOrg, yBltDstHei);
        xBltDst     = xDst;
        xBltDstWid  = xDstWid;
        xBltSrcOrg  = xSrcOrg;

        while (xBltDstWid)
        {
            xBltSrc = xBltSrcOrg;
            xBltWid = min(xSrcWid - xBltSrcOrg, xBltDstWid);

            if (hdcMem)
            {
                BitBlt(hdc, xBltDst, yBltDst, xBltWid, yBltHei,
                    hdcMem, xBltSrc, yBltSrc, SRCCOPY);
            }
            else
            {
                if (psizePrint)
                {
                    rcSrc.left   = MulDivQuick(xBltSrc, xPreWid, xSrcWid);
                    rcSrc.top    = MulDivQuick(yBltSrc, yPreHei, ySrcHei);
                    rcSrc.right  = MulDivQuick(xBltSrc + xBltWid, xPreWid, xSrcWid);
                    rcSrc.bottom = MulDivQuick(yBltSrc + yBltHei, yPreHei, ySrcHei);
                }
                else
                {
                    rcSrc.left   = xBltSrc;
                    rcSrc.top    = yBltSrc;
                    rcSrc.right  = xBltSrc + xBltWid;
                    rcSrc.bottom = yBltSrc + yBltHei;
                }

                rcDst.left   = xBltDst;
                rcDst.top    = yBltDst;
                rcDst.right  = xBltDst + xBltWid;
                rcDst.bottom = yBltDst + yBltHei;

                rcDstFull.left   = xBltDst - xBltSrc;
                rcDstFull.top    = yBltDst - yBltSrc;
                rcDstFull.right  = xBltDst - xBltSrc + xFullWid;
                rcDstFull.bottom = yBltDst - yBltSrc + yFullHei;
                
                if (pImgAnimState)
                {
                    GetImgInfo()->DrawFrame(hdc, pImgAnimState, &rcDst, &rcSrc, &rcDstFull, dwFlags);
                }
                else
                {
                    hr = THR(GetImgInfo()->DrawImage(hdc, &rcDst, &rcSrc, SRCCOPY, dwFlags));
                    if (hr)
                        goto Cleanup;
                }
            }

            xBltDst    += xBltWid;
            xBltDstWid -= xBltWid;
            xBltSrcOrg  = 0;
        }

        yBltDst    += yBltHei;
        yBltDstHei -= yBltHei;
        yBltSrcOrg  = 0;
    }

Cleanup:

    if (hbmSav)
        SelectObject(hdcMem, hbmSav);
    if (hbmMem)
        DeleteObject(hbmMem);
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);
}

ULONG
CImgCtx::GetState(BOOL fClear, SIZE *psize)
{
    if (psize == NULL)
    {
        return(super::GetState(fClear));
    }
    else
    {
        EnterCriticalSection();

        CImgInfo * pImgInfo = GetImgInfo();

        psize->cx = pImgInfo->_xWid;
        psize->cy = pImgInfo->_yHei;

        ULONG ulState = super::GetState(fClear);

        LeaveCriticalSection();

        return(ulState);
    }
}

void
CImgCtx::Signal(WORD wChg, BOOL fInvalAll, int yBot)
{
    _yTop = Union(_yTop, _yBot, fInvalAll, yBot);
    _yBot = yBot;
    super::Signal(wChg);
}

HRESULT
CImgCtx::SaveAsBmp(IStream * pStm, BOOL fFileHeader)
{
    return GetImgInfo()->SaveAsBmp(pStm, fFileHeader);
}

#ifndef NO_ART
CArtPlayer *
CImgCtx::GetArtPlayer()
{
    return GetImgInfo()->GetArtPlayer();
}
#endif // ndef NO_ART

// Internal Functions ---------------------------------------------------------

void CALLBACK
ImgCtxNullCallback(void *, void *)
{
}

// Public Functions -----------------------------------------------------------

HRESULT
CreateIImgCtx(IUnknown * pUnkOuter, IUnknown **ppUnk)
{
    if (pUnkOuter != NULL)
    {
        *ppUnk = NULL;
        return(CLASS_E_NOAGGREGATION);
    }

    CImgCtx * pImgCtx = new CImgCtx;

    if (pImgCtx)
    {
        // The purpose of setting a NULL callback is to AddRef the current
        // THREADSTATE, which is a side-effect of setting a synchronous
        // callback.  This prevents the thread from passivating between the
        // time the user calls CoCreateInstance and sets a different callback
        // function and/or releases this object.

        pImgCtx->SetCallback(ImgCtxNullCallback, 0);
    }

    *ppUnk = pImgCtx;

    RRETURN(pImgCtx ? S_OK : E_OUTOFMEMORY);
}
