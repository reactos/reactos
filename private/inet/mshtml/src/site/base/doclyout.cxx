//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       layout.cxx
//
//  Contents:   Layout management
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

DeclareTag(tagDocSize, "DocSize tracing", "Trace changes to document size");

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetExtent, IOleObject
//
//  Synopsis:   Called when the container wants to tell us a new size.  We
//              override this method so we can maintain the logical size of
//              the form properly.
//
//  Arguments:  dwAspect    Aspect which is changing.
//              psizel      New size
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::SetExtent(DWORD dwAspect, SIZEL *psizel)
{
    HRESULT hr;

    TraceTagEx((tagDocSize, TAG_NONAME,
           "Doc : SetExtent - sizel(%d, %d)",
           (psizel ? psizel->cx : 0),
           (psizel ? psizel->cy : 0)));

#ifdef WIN16
    // in case we were are on some other thread's stack (e.g. Java)...
    // impersonate our own
    CThreadImpersonate cimp(_dwThreadId);
#endif
    hr = THR(CServer::SetExtent(dwAspect, psizel));
    if (hr)
        goto Cleanup;

    //
    // Update the transform with the new extents
    //

    if (dwAspect == DVASPECT_CONTENT && psizel && (psizel->cx || psizel->cy))
    {
        RECT    rc;

#ifdef  IE5_ZOOM

        psizel->cx = max(psizel->cx, (LONG)_dci.HimFromDxt(_dci.DxtFromDxz(1)));
        psizel->cy = max(psizel->cy, (LONG)_dci.HimFromDyt(_dci.DytFromDyz(1)));

#if DBG==1
        long cxNew = _dci.HimFromDxt(_dci.DxtFromDxz(1));
        long cyNew = _dci.HimFromDyt(_dci.DytFromDyz(1));
        long cxOld = MulDivQuick(1L, 2540L, _dci._sizeInch.cx);
        long cyOld = MulDivQuick(1L, 2540L, _dci._sizeInch.cy);
        Assert(cxNew == cxOld || _dci.IsZoomed());
        Assert(cyNew == cyOld || _dci.IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

        psizel->cx = max(psizel->cx, (LONG)MulDivQuick(1L, 2540L, _dci._sizeInch.cx));
        psizel->cy = max(psizel->cy, (LONG)MulDivQuick(1L, 2540L, _dci._sizeInch.cy));

#endif  // IE5_ZOOM

        rc.left    =
        rc.top     = 0;

#ifdef  IE5_ZOOM

        rc.right   = max((long)_dci.DxzFromDxt(_dci.DxtFromHim(psizel->cx)), 1L);
        rc.bottom  = max((long)_dci.DyzFromDyt(_dci.DytFromHim(psizel->cy)), 1L);

#if DBG==1
        cxNew = _dci.DxzFromDxt(_dci.DxtFromHim(psizel->cx));
        cyNew = _dci.DyzFromDyt(_dci.DytFromHim(psizel->cy));
        cxOld = MulDivQuick(psizel->cx, _dci._sizeInch.cx, 2540L);
        cyOld = MulDivQuick(psizel->cy, _dci._sizeInch.cy, 2540L);
        Assert(cxNew == cxOld || _dci.IsZoomed());
        Assert(cyNew == cyOld || _dci.IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

        rc.right   = max((long)MulDivQuick(psizel->cx, _dci._sizeInch.cx, 2540L), 1L);
        rc.bottom  = max((long)MulDivQuick(psizel->cy, _dci._sizeInch.cy, 2540L), 1L);

#endif  // IE5_ZOOM

        _dci.CTransform::Init(&rc, *psizel);

        _view.SetViewSize(_dci._sizeDst);

        TraceTagEx((tagDocSize, TAG_NONAME,
               "Doc : SetExtent - _dci._sizeDst(%d, %d)",
               _dci._sizeDst.cx,
               _dci._sizeDst.cy));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::SetObjectRects, IOleInplaceObject
//
//  Synopsis:   Set position and clip rectangles.
//
//-------------------------------------------------------------------------

HRESULT
CDoc::SetObjectRects(LPCOLERECT prcPos, LPCOLERECT prcClip)
{
    RECT    rcPosPrev, rcClipPrev;
    RECT    rcPos, rcClip;
    long    cx, cy;
    HRESULT hr;

    TraceTagEx((tagDocSize, TAG_NONAME,
           "Doc : SetObjectRects - rcPos(%d, %d, %d, %d) rcClip(%d, %d, %d, %d)",
           (!prcPos ? 0 : prcPos->left),
           (!prcPos ? 0 : prcPos->top),
           (!prcPos ? 0 : prcPos->right),
           (!prcPos ? 0 : prcPos->bottom),
           (!prcClip ? 0 : prcClip->left),
           (!prcClip ? 0 : prcClip->top),
           (!prcClip ? 0 : prcClip->right),
           (!prcClip ? 0 : prcClip->bottom)));

#ifdef IE5_ZOOM
    RECT    rcPosNew;       // non-scaled size of destination rectangle
    RECT    rcPosNewScaled; // scaled size of destination rectangle
#endif  // IE5_ZOOM

    Assert(State() >= OS_INPLACE);
    Assert(_pInPlace);

    rcPosPrev  = _pInPlace->_rcPos;
    rcClipPrev = _pInPlace->_rcClip;

#ifdef WIN16
    // in case we were are on some other thread's stack (e.g. Java)...
    // impersonate our own
    CThreadImpersonate cimp(_dwThreadId);
#endif
    {
        CLock   Lock(this, SERVERLOCK_IGNOREERASEBKGND);

        hr = THR(CServer::SetObjectRects(prcPos, prcClip));
        if (hr)
            goto Cleanup;
    }

#ifdef IE5_ZOOM

    rcPosNew = _pInPlace->_rcPos;
    _dci.RectzFromRectr(&rcPosNewScaled, &rcPosNew);

    rcPos  = rcPosNewScaled;

#else   // !IE5_ZOOM

    rcPos  = _pInPlace->_rcPos;

#endif  // IE5_ZOOM

    rcClip = _pInPlace->_rcClip;

    //
    // Update transform to contain the correct destination RECT
    //

    cx = rcPos.right  - rcPos.left;
    cy = rcPos.bottom - rcPos.top;
    if (cx || cy)
    {
        RECT    rc = rcPos;
        SIZEL   sizel;

        if (!cx)
        {
            cx = 1;
            rc.right = rc.left + 1;
        }

        if (!cy)
        {
            cy = 1;
            rc.bottom = rc.top + 1;
        }

#ifdef  IE5_ZOOM

        sizel.cx = max(_sizel.cx, (LONG)_dci.HimFromDxt(_dci.DxtFromDxz(1)));
        sizel.cy = max(_sizel.cy, (LONG)_dci.HimFromDyt(_dci.DytFromDyz(1)));

#if DBG==1
        long cxNew = _dci.HimFromDxt(_dci.DxtFromDxz(1));
        long cyNew = _dci.HimFromDyt(_dci.DytFromDyz(1));
        long cxOld = MulDivQuick(1L, 2540L, _dci._sizeInch.cx);
        long cyOld = MulDivQuick(1L, 2540L, _dci._sizeInch.cy);
        Assert(cxNew == cxOld || _dci.IsZoomed());
        Assert(cyNew == cyOld || _dci.IsZoomed());
#endif  // DBG==1

#else   // !IE5_ZOOM

        sizel.cx = max(_sizel.cx, (LONG)MulDivQuick(1L, 2540L, _dci._sizeInch.cx));
        sizel.cy = max(_sizel.cy, (LONG)MulDivQuick(1L, 2540L, _dci._sizeInch.cy));

#endif  // IE5_ZOOM

        _dci.CTransform::Init(&rc, sizel);
    }

    //
    // If necessary, initiate a re-layout
    //

#ifdef IE5_ZOOM

    if (rcPosNew.right - rcPosNew.left != rcPosPrev.right - rcPosPrev.left ||
        rcPosNew.bottom - rcPosNew.top != rcPosPrev.bottom - rcPosPrev.top  )

#else   // !IE5_ZOOM

    if (rcPos.right - rcPos.left != rcPosPrev.right - rcPosPrev.left ||
        rcPos.bottom - rcPos.top != rcPosPrev.bottom - rcPosPrev.top  )

#endif  // IE5_ZOOM
    {
#ifdef IE5_ZOOM

        if (_dci.IsZoomed())
        {
            SIZE sizeNew;

            sizeNew.cx = _pInPlace->_rcPos.right - _pInPlace->_rcPos.left;
            sizeNew.cy = _pInPlace->_rcPos.bottom - _pInPlace->_rcPos.top;

            _view.SetViewSize(sizeNew);
        }
        else
        {
            _view.SetViewSize(_dci._sizeDst);
        }

#else   // !IE5_ZOOM

        _view.SetViewSize(_dci._sizeDst);

#endif  // IE5_ZOOM
    }

    //
    //  Otherwise, just invalidate the view
    //

    else if (rcClip.right - rcClip.left != rcClipPrev.right - rcClipPrev.left ||
             rcClip.bottom - rcClip.top != rcClipPrev.bottom - rcClipPrev.top  )
    {
// BUGBUG: We need an invalidation...will a partial work? (brendand)
        Invalidate();
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureFormatCacheChange
//
//  Synopsis:   Clears the format caches for the client site and then
//              invalidates it.
//
//----------------------------------------------------------------------------

void
CDoc::EnsureFormatCacheChange(DWORD dwFlags)
{
    if ((State() < OS_RUNNING) || !PrimaryRoot())
        return;

    PrimaryRoot()->EnsureFormatCacheChange(dwFlags);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ForceRelayout
//
//  Synopsis:   Whack the entire display
//
//----------------------------------------------------------------------------

HRESULT
CDoc::ForceRelayout ( )
{
    HRESULT hr = S_OK;
    CNotification   nf;

    if (_pPrimaryMarkup)
    {
        PrimaryRoot()->EnsureFormatCacheChange( ELEMCHNG_CLEARCACHES );
        nf.DisplayChange(PrimaryRoot());
        PrimaryRoot()->SendNotification(&nf);     
    }

    _view.ForceRelayout();

    RRETURN( hr );
}
