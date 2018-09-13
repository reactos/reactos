//+----------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       filter.cxx
//
//  Contents:   CFilter and related classes
//
//-----------------------------------------------------------------------------


#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_DOBJ_HXX_
#define X_DOBJ_HXX_
#include "dobj.hxx"
#endif

#ifndef X_RTFTOHTM_HXX_
#define X_RTFTOHTM_HXX_
#include "rtftohtm.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_OCIDL_H_
#define X_OCIDL_H_
#include "ocidl.h"
#endif

#ifndef X_OCMM_H_
#define X_OCMM_H_
#include "ocmm.h"
#endif

#ifndef X_HTMLFILTER_H_
#define X_HTMLFILTER_H
#include "htmlfilter.h"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

#ifndef X_DDRAWEX_H_
#define X_DDRAWEX_H_
#include <ddrawex.h>
#endif
#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

DeclareTag(tagFilter, "Filter", "Trace filter behaviour")
DeclareTag(tagFilterFakeSource, "Filter", "Don't render the source disp tree")
DeclareTag(tagFilterPaintScreen, "Filter", "Paint source to the screen in Draw")
DeclareTag(tagFilterDontDrawSource, "Filter", "Don't render the source filter")
ExternTag(tagHackGDICoords);

MtDefine(CFilter, Utilities, "CFilter")


//+----------------------------------------------------------------------------
//
//  Member:     CFilter/~CFilter
//
//  Synopsis:   Constructor/destructor
//
//  Arguments:  pElement - Associated CElement
//
//-----------------------------------------------------------------------------
CFilter::CFilter(CElement *pElement)
{
    Assert(pElement);
    Assert(pElement->GetFirstBranch()->IsInMarkup());
    Assert(pElement->GetMarkup()->IsPrimaryMarkup());

    _pElement    = pElement;



    _ulRefs = 1;


    _dwStatus = 0;
    _grfFlags = 0;


    // By the time we are instantiated, the element
    // visibility is well known
    if (pElement->GetFirstBranch()->GetCharFormat()->_fVisibilityHidden)
        _dwStatus |= FILTER_STATUS_INVISIBLE;

    CLayout * pLayout = pElement->GetUpdatedLayout();
    Assert(pLayout);
    if(pLayout)
    {
        if (pLayout->_fSurface)
            _dwStatus |= FILTER_STATUS_SURFACE;
        if (pLayout->_f3DSurface)
            _dwStatus |= FILTER_STATUS_3DSURFACE;

        CDispNode* pDispNode = pLayout->GetElementDispNode();


        if (pDispNode)
        {
            pLayout->OpenView();
            pDispNode->SetFiltered(TRUE);

            if (pDispNode->IsOpaque())
                _dwStatus |= FILTER_STATUS_OPAQUE;
        }
    }
}

CFilter::~CFilter()
{
    _pElement = NULL;

    Assert(_ulRefs == 0);
    Assert(_pSource == 0);
}


void CFilter::Detach(BOOL fCleanup)
{
    if (Element())
    {
        CLayout * pLayout = Element()->GetCurLayout();

        Assert(pLayout);
        Assert(Element()->GetFilterPtr() == this);

        // Tell the display node that we are no longer filtering

        CDispNode *pDispNode = pLayout->GetElementDispNode();
        if (pDispNode)
        {
            pLayout->OpenView();
            pDispNode->SetFiltered(FALSE);
        }

        //
        //  Disconnect from the element and associated layout
        //

        Element()->DelFilterPtr();

        _pElement = NULL;

        //
        // Now that we're out the Element's hair, it's time to clean up ourselves
        // Even under normal conditions, we may have a bunch of filters.
        //
        // We only try to do this if we are detaching under normal conditions
        // If something bad happened, it's not worth the effort.  It's safer to
        // leak memory than to wander around our list trying vainly to release
        // everyone
        //

        if (fCleanup)
        {
            while (_pSource  && (_pSource != (IHTMLViewFilter *)this))
            {
                IHTMLViewFilter *pNext = NULL;
                IGNORE_HR(_pSource->GetSource(&pNext));

                _pSource->SetSite(NULL);
                _pSource->SetSource(NULL);

                _pSource->Release();
                _pSource = pNext;

            }

            if (_pSource)
            {
                Assert(_pSource == (IHTMLViewFilter *)this);
                _pSource->Release();
                _pSource = 0;
            }
        }

        Assert(!_pSource);
        Assert(!_pFilterSite);
        Assert(!Element());

        Release();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     QueryInterface/AddRef/Release
//
//  Synopsis:   Stock COM IUnknown interface
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CFilter::QueryInterface(
    REFIID      iid,
    LPVOID *    ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IHTMLViewFilter*)this, IUnknown)
    QI_INHERITS(this, IHTMLViewFilter)
    QI_INHERITS(this, IHTMLViewFilterSite)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}


ULONG
CFilter::AddRef()
{
    return ++_ulRefs;
}


ULONG
CFilter::Release()
{
    ULONG ulRefs = --_ulRefs;

    if (ulRefs == 0)
        delete this;
    return ulRefs;
}


//+----------------------------------------------------------------------------
//
//  IHTMLViewFilter methods
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CFilter::SetSource(IHTMLViewFilter *pFilter)
{
    AssertSz(FALSE, "Don't call this!");
    RRETURN(E_FAIL);
}


STDMETHODIMP
CFilter::GetSource(IHTMLViewFilter **ppFilter)
{
    AssertSz(FALSE, "Don't call this!");
    RRETURN(E_FAIL);
}


STDMETHODIMP
CFilter::SetSite(IHTMLViewFilterSite *pFilterSite)
{
    ReplaceInterface(&_pFilterSite, pFilterSite);
    return S_OK;
}


STDMETHODIMP
CFilter::GetSite(IHTMLViewFilterSite **ppFilterSite)
{
    if (ppFilterSite == NULL)
        return E_POINTER;

    if (_pFilterSite)
        _pFilterSite->AddRef();

    *ppFilterSite = _pFilterSite;
    return S_OK;
}

STDMETHODIMP
CFilter::SetPosition(LPCRECT prc)
{
    Assert(_size.cx == (prc->right - prc->left));
    Assert(_size.cy == (prc->bottom - prc->top));

    return S_OK;
}

STDMETHODIMP
CFilter::Draw(HDC hdc, LPCRECT prc)
{
    Assert(hdc != NULL);
    
    if (hdc == NULL)
        return E_FAIL;
    
    WHEN_DBG(POINT pt;)
    WHEN_DBG(GetViewportOrgEx(hdc, &pt);)

    Assert(((CRect*)prc)->Size() == _size);

    // BUGBUG michaelw - some filters may actually want to change to palette
    //                   and I don't actually know that everyone depends on
    //                   palette being properly set.  No doubt this will change
    //                   when the overall palette work goes in anway.
    Element()->Doc()->GetPalette(hdc);

#if DBG == 1

    if (IsTagEnabled(tagFilterFakeSource))
    {
        SaveDC(hdc);

        HBRUSH hbrush = CreateHatchBrush(HS_DIAGCROSS, RGB(0, 255, 0));
        HPEN hpen = CreatePen(PS_SOLID, 2, RGB(0, 255, 255));

        SelectBrush(hdc, hbrush);
        SelectPen(hdc, hpen);
        Rectangle(hdc, prc->left, prc->top, prc->right, prc->bottom);
        
        SetTextColor(hdc, RGB(255, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        TCHAR *sz = _T("This text was rendered instead of the filtered element, it is primarily to test filters");

        DrawText(hdc, sz, -1, const_cast<LPRECT>(prc), DT_CENTER | DT_WORDBREAK);

        RestoreDC(hdc, -1);

    }
    else
    {
#endif

    if (_pContext)
    {
        // This is a draw call in response to our Draw on the filter

        POINT ptOrg;
        ::SetViewportOrgEx(hdc, 0, 0, &ptOrg);
        CDispNode* pDispNode = GetDispNode();
        Assert(pDispNode != NULL);
        pDispNode->DrawNodeForFilter(
            _pContext,
            hdc,
            ((CRect*)prc)->TopLeft() + (CSize&)ptOrg);
        ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
    }
    else
    {
        // This is an out of band call, typically done by a transition when Apply is called.  We need to
        // enlist the help of the view.

        POINT ptOrg;
        ::SetViewportOrgEx(hdc, 0, 0, &ptOrg);
        Element()->Doc()->GetView()->RenderElement(Element(), hdc);
        ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
    }
#if DBG == 1
    }
    if (IsTagEnabled(tagFilterPaintScreen))
    {
        HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
        if (hdcScreen)
        {
            BitBlt(hdcScreen, 0, 0, _size.cx, _size.cy, hdc, prc->left, prc->top, BLACKNESS);
            BitBlt(hdcScreen, 0, 0, _size.cx, _size.cy, hdc, prc->left, prc->top, SRCCOPY);  

            DeleteDC(hdcScreen);
        }
    }
#endif

    return S_OK;
}

STDMETHODIMP
CFilter::GetStatusBits(DWORD *pdwFlags)
{
    if (!pdwFlags)
        return E_POINTER;

    *pdwFlags = _dwStatus;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  IHTMLViewFilterSite methods
//
//-----------------------------------------------------------------------------

HRESULT
CFilter::GetDC(LPCRECT prc, DWORD dwFlags, HDC *phdc)
{
    if (dwFlags & OLEDC_NODRAW)
        RRETURN(Element()->GetUpdatedLayout()->GetDC(const_cast<LPRECT>(prc), dwFlags, phdc));

    if (_pFilterSite)
        _pFilterSite->InvalidateRect(prc, FALSE);

    RRETURN(E_FAIL);
}

HRESULT
CFilter::ReleaseDC(HDC hdc)
{
    RRETURN(Element()->Doc()->ReleaseDC(hdc));
}


STDMETHODIMP
CFilter::InvalidateRect(LPCRECT prc, BOOL fErase)
{
    if (Element()->Doc()->_view.IsInState(CView::VS_INRENDER))
    {
        TraceTag((tagFilter, "forced to defer invalidation becase the view is rendering"));
        _fNeedInvalidate = TRUE;    // REVIEW how do I ensure we get called again?
        return S_OK;
    }
        
    Assert(Element()->GetCurLayout() == Element()->GetUpdatedLayout());

    CDispNode *pDispNode = GetDispNode();

    if (!pDispNode)
    {
        TraceTag((tagFilter, "InvalidateRect ignored because GetDispNode() == 0"));
        return S_OK;
    }

    if ((_size.cx * _size.cy) == 0)
    {
        TraceTag((tagFilter, "InvalidateRect ignored because surface area is 0: %d %d", _size.cx, _size.cy));
        return S_OK;
    }

    CRect rc;

    if (!prc || _fNeedInvalidate)
        rc.SetRect(_size);
    else
        rc = *prc;

    pDispNode->Invalidate(rc, COORDSYS_CONTAINER, FALSE, TRUE);
    _fNeedInvalidate = FALSE;

    return S_OK;
}


STDMETHODIMP
CFilter::InvalidateRgn(HRGN hrgn, BOOL fErase)
{
    if (Element()->Doc()->_view.IsInState(CView::VS_INRENDER))
    {
        TraceTag((tagFilter, "forced to defer invalidation becase the view is rendering"));
        _fNeedInvalidate = TRUE;    // REVIEW how do I ensure we get called again?
        return S_OK;
    }
        
    Assert(Element()->GetCurLayout() == Element()->GetUpdatedLayout());

    CDispNode *pDispNode = GetDispNode();

    if (!pDispNode)
    {
        TraceTag((tagFilter, "InvalidateRect ignored because GetDispNode() == 0"));
        return S_OK;
    }

    if ((_size.cx * _size.cy) == 0)
    {
        TraceTag((tagFilter, "InvalidateRect ignored because surface area is 0: %d %d", _size.cx, _size.cy));
        return S_OK;
    }

    if (_fNeedInvalidate)
        RRETURN(InvalidateRect(NULL, FALSE));

    if (Element()->GetCurLayout()->OpenView())
    {
        // on Win9x, we jump through hoops to avoid the 16-bit coordinate
        // limitations in GDI
        BOOL fHackForGDI = g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;
    
#if DBG==1
        if (IsTagEnabled(tagHackGDICoords))
            fHackForGDI = TRUE;
#endif

        if (fHackForGDI)
        {
            CRect   rc;

            ::GetRgnBox(hrgn, &rc);
            pDispNode->Invalidate(rc, COORDSYS_CONTAINER, FALSE, TRUE);
        }
        else
        {
            pDispNode->Invalidate(CRegion(hrgn), COORDSYS_CONTAINER, FALSE, TRUE);
        }
    }
    return S_OK;
}


STDMETHODIMP
CFilter::OnStatusBitsChange(DWORD dwFlags)
{
    BOOL fDisplayHidden = !!(dwFlags & FILTER_STATUS_INVISIBLE);
    if (fDisplayHidden != !!_fDisplayHidden)
    {
        _fDisplayHidden = fDisplayHidden;
        if (!_fBlockHiddenChange)
            Element()->OnPropertyChange(DISPID_A_VISIBILITY, ELEMCHNG_CLEARCACHES);
    }

    CLayout *   pLayout = Element()->GetCurLayout();
    if (pLayout)
    {
        if (((_dwStatus & FILTER_STATUS_SURFACE) != (dwFlags & FILTER_STATUS_SURFACE))
        ||  ((_dwStatus & FILTER_STATUS_3DSURFACE) != (dwFlags & FILTER_STATUS_3DSURFACE)))
        {
            pLayout->SetSurfaceFlags(dwFlags & FILTER_STATUS_SURFACE,
                                 dwFlags & FILTER_STATUS_3DSURFACE,
                                 TRUE);
        }
        
        CDispNode * pDispNode = GetDispNode();
        if (pDispNode)
        {
            BOOL fOpaque = !!(dwFlags & FILTER_STATUS_OPAQUE);
            if (fOpaque != pDispNode->IsOpaque())
            {
                pLayout->OpenView();
                pDispNode->UnfilteredSetOpaque(fOpaque);
            }
        }
    }

    return S_OK;
}



//+----------------------------------------------------------------------------
//
//  Internal methods
//
//-----------------------------------------------------------------------------

HRESULT
CFilter::AddFilter(IHTMLViewFilter *pFilter)
{
    HRESULT hr;

    Assert(!_fInHookup);
    Assert(pFilter);
    Assert(Element()->GetFirstBranch()->IsInMarkup());
    Assert(Element()->GetFirstBranch()->GetMarkup()->IsPrimaryMarkup());


    _fInHookup = TRUE;

    hr = pFilter->SetSite((IHTMLViewFilterSite *)this);
    if (FAILED(hr))
        goto Cleanup;

    hr = pFilter->SetSource(_pSource ? _pSource : (IHTMLViewFilter *)this);
    if (FAILED(hr))
        goto Cleanup;

    ReplaceInterface(&_pSource, pFilter);
    FinishUpdate();

    {
        // stupid compiler bitches about initalization skipped by goto
        CDispNode *pDispNode = GetDispNode();
        if (pDispNode)
            SetSize(pDispNode->GetBounds().Size());
    }

Cleanup:
    if (FAILED(hr))
    {
        pFilter->SetSite(NULL);
        pFilter->SetSource(NULL);
        if (_pSource)
        {
            _pSource->SetSite(NULL);
            _pSource->SetSource(NULL);
        }
        Detach(FALSE);
    }

    _fInHookup = FALSE;
    RRETURN(hr);
}

HRESULT
CFilter::RemoveFilter(IHTMLViewFilter *pFilter)
{
    IHTMLViewFilterSite *   pSiteBefore   = NULL;
    IHTMLViewFilter *       pFilterBefore = NULL;
    IHTMLViewFilter *       pFilterAfter  = NULL;
    HRESULT             hr;

    Assert(!_fInHookup);
    Assert(pFilter);

    _fInHookup = TRUE;

    hr = pFilter->GetSource(&pFilterAfter);
    if (FAILED(hr))
        goto Cleanup;

    hr = pFilter->GetSite(&pSiteBefore);
    if (FAILED(hr))
        goto Cleanup;

    hr = pSiteBefore->QueryInterface(IID_IHTMLViewFilter, (LPVOID *)&pFilterBefore);
    if (FAILED(hr))
        goto Cleanup;

    hr = pFilter->SetSite(NULL);
    if (FAILED(hr))
        goto Cleanup;

    hr = pFilter->SetSource(NULL);
    if (FAILED(hr))
        goto Cleanup;

    Assert(pFilterAfter);
    Assert(pSiteBefore);
    Assert(pFilterBefore);

    if (pSiteBefore == (IHTMLViewFilterSite *)this)
    {
        hr = pFilterAfter->SetSite((IHTMLViewFilterSite *)this);
        if (FAILED(hr))
            goto Cleanup;
        ReplaceInterface(&_pSource, pFilterAfter);
    }
    else
    {
        hr = pFilterBefore->SetSource(pFilterAfter);
        if (FAILED(hr))
            goto Cleanup;
    }

    //
    // When all is said and done, are we in fact out of filters?
    //
    if (_pSource == (IHTMLViewFilter *)this)
    {
        ClearInterface(&_pSource);
        ClearInterface(&_pFilterSite);
        Detach();
    }
    else
        FinishUpdate();

Cleanup:
    _fInHookup = FALSE;

    ClearInterface(&pFilterBefore);
    ClearInterface(&pSiteBefore);
    ClearInterface(&pFilterAfter);

    if (FAILED(hr))
        Detach(FALSE);

    RRETURN(hr);
}


void
CFilter::EnsureFilterState()
{
    Assert(GetDispNode());
    Assert(GetDispNode()->IsFiltered());

    FinishUpdate();

    GetDispNode()->SetFiltered(TRUE);
    if (_fNeedInvalidate)
    {
        InvalidateRect(NULL, FALSE);
    }
}

void
CFilter::FinishUpdate()
{
    DWORD       dwFlags = 0;

    Assert(_pSource);

    if (SUCCEEDED(_pSource->GetStatusBits(&dwFlags)))
    {
        IGNORE_HR(OnStatusBitsChange(dwFlags));
    }
    else
    {
        //
        // Technically we should set the _fSurface and _f3DSurface bits
        // here too.  We choose not to in the name of robustness
        //
    }
    // We may have missed a SetSize, synthesize it

    CDispNode *pDispNode = GetDispNode();

    if (pDispNode)
    {
        SetSize(pDispNode->GetBounds().Size());
    }

}

BOOL
CFilter::FilteredIsHidden(BOOL fDisplayHidden)
{
    _fBlockHiddenChange = TRUE;

    FilteredSetStatusBits(FILTER_STATUS_INVISIBLE, fDisplayHidden ? FILTER_STATUS_INVISIBLE : 0);

    _fBlockHiddenChange = FALSE;

#if DBG == 1
    if (_pSource)
    {
        DWORD   dwStatus       = 0;
        IGNORE_HR(_pSource->GetStatusBits(&dwStatus));
        BOOL fDisplayHiddenDebug;

        fDisplayHiddenDebug = !!(dwStatus & FILTER_STATUS_INVISIBLE);

        Assert(!!fDisplayHiddenDebug == !!_fDisplayHidden);
    }
#endif

    return _fDisplayHidden;
}


void
CFilter::FilteredSetStatusBits(DWORD dwMask, DWORD dwFlags)
{
    DWORD dwStatus = _dwStatus;

    _dwStatus &= ~dwMask;       // clear out the old bits
    _dwStatus |= dwFlags;       // and stuff in the new

    if (dwStatus != _dwStatus)  // has anything changed?
    {
        if (_pFilterSite)
        {
            _pFilterSite->OnStatusBitsChange(_dwStatus);
        }
    }
}

void
CFilter::SetSize(const SIZE& size)
{
    _size = size;

    CRect rc(_size);

    if (_pSource)
        _pSource->SetPosition(&rc);
}

void
CFilter::DrawFiltered(CDispDrawContext* pContext)
{
    Assert(pContext != NULL);
    Assert(!Element()->_fHasPendingFilterTask);
    Assert(!_fNeedInvalidate);

    WHEN_DBG(POINT pt;)

    if (_pSource == NULL)
        return;

    CDispNode* pDispNode = GetDispNode();
    Assert(pDispNode != NULL);
    const CRect& rcBounds = pDispNode->GetBounds();
    
    Assert(rcBounds.Size() == _size);

    // If the size is 0 then there's nothing to draw
    // Don't waste time trying
    if (_size.cx == 0 || _size.cy == 0)
        return;

    CDispSurface *pSurface = pContext->GetDispSurface();
    Assert(pSurface != NULL);
    
    HDC hdc = 0;
    pSurface->GetDC(&hdc, rcBounds, pContext->_rcClip, TRUE, TRUE);
    Assert(hdc);

    WHEN_DBG(GetViewportOrgEx(hdc, &pt);)

    //
    // The IHammer filter implementations get very confused if
    // the rect we draw isn't the same we passed in SetPosition
    // So, we make sure the viewport org is setup properly
    //

    POINT ptOrg;

    OffsetViewportOrgEx(hdc, rcBounds.left, rcBounds.top, &ptOrg);

    CRect rcDraw(_size);

    // First make sure that if _fInvalidate is set, that the paint region
    // includes our entire rect

    if (_fNeedInvalidate)
    {
        CRect rc;
        GetClipBox(hdc, &rc);

        if (!rc.Contains(rcDraw))
        {
            InvalidateRect(NULL, FALSE);
            _fNeedInvalidate = FALSE;
            TraceTag((tagWarning, "DrawFiltered was called and _fInvalidate is still true and the update region doesn't contain the entire rect"));
        }
    }

#if DBG == 1

    if (IsTagEnabled(tagFilterDontDrawSource))
    {
        HBRUSH hbr = CreateSolidBrush(RGB(0, 255, 0));

        FillRect(hdc, &rcDraw, hbr);

        DeleteObject(hbr);

        TCHAR sz[] = _T("Don't draw filter source");

        for (int y = 0; y < _size.cy; y += 16)
        {
            ExtTextOut(hdc, 0, y, ETO_CLIPPED, &rcDraw, sz, sizeof(sz) / sizeof(TCHAR) - sizeof(TCHAR), NULL);
        }
        
        goto Cleanup;
    }
#endif

    Assert(_pContext == 0);
    // Remember the context for when the source draws us
    _pContext = pContext;

    {
        int iDC = SaveDC(hdc);
        _pSource->Draw(hdc, &rcDraw);
        RestoreDC(hdc, iDC);
    }

    _pContext = 0;

#if DBG == 1
Cleanup:
    if (IsTagEnabled(tagFilterPaintScreen))
    {
        HDC hdcScreen = CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
        if (hdcScreen)
        {
            BitBlt(hdcScreen, _size.cx, 0, _size.cx, _size.cy, hdc, 0, 0, BLACKNESS);
            BitBlt(hdcScreen, _size.cx, 0, _size.cx, _size.cy, hdc, 0, 0, SRCCOPY);  

            DeleteDC(hdcScreen);

        }
    }
#endif

    // Put the DC origin back the way it should be
    SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
}

void
CFilter::Invalidate(const RECT& rc, BOOL fSynchronousRedraw)
{
    if (_pFilterSite)
        _pFilterSite->InvalidateRect(NULL, FALSE);
}

void
CFilter::Invalidate(HRGN hrgn, BOOL fSynchronousRedraw)
{
    if (_pFilterSite)
        _pFilterSite->InvalidateRgn(NULL, FALSE);
}

void
CFilter::SetOpaque(BOOL fOpaque)
{
    FilteredSetStatusBits(
        FILTER_STATUS_OPAQUE,
        fOpaque ? FILTER_STATUS_OPAQUE : 0);
}

#if DBG == 1

void ShowDeviceCoordinates(HDC hdc, LPRECT prc)
{
    TCHAR buf[80];
    RECT rc = *prc;

    LPtoDP(hdc, (LPPOINT)&rc, 2);

    Format(0, buf, sizeof(buf) / sizeof(TCHAR), _T("<0d> <1d> <2d> <3d> => <4d> <5d> <6d> <7d>\n"), prc->left, prc->top, prc->right, prc->bottom, rc.left, rc.top, rc.right, rc.bottom);
    OutputDebugString(buf);
}

void ShowDeviceCoordinates(HDC hdc, LPPOINT ppt)
{
    ShowDeviceCoordinates(hdc, ppt->x, ppt->y);
}

void ShowDeviceCoordinates(HDC hdc, int x, int y)
{
    POINT pt;

    pt.x = x;
    pt.y = y;

    LPtoDP(hdc, &pt, 1);

    TCHAR buf[80];

    Format(0, buf, sizeof(buf) / sizeof(TCHAR), _T("<0d> <1d> => <2d> <3d>\n"), x, y, pt.x, pt.y);
    OutputDebugString(buf);
}
#endif
