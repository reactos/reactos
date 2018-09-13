//+---------------------------------------------------------------------
//
//   File:      ebody.cxx
//
//  Contents:   Body element class
//
//  Classes:    CBodyLayout
//
//------------------------------------------------------------------------

#include "headers.hxx"
#include "frame.hxx"

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_BODYLYT_HXX_
#define X_BODYLYT_HXX_
#include "bodylyt.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

const CLayout::LAYOUTDESC CBodyLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

//+---------------------------------------------------------------------------
//
//  Member:     CBodyLayout::HandleMessage
//
//  Synopsis:   Check if we have a setcursor or mouse down in the
//              border (outside of the client rect) so that we can
//              pass the message to the containing frameset if we're
//              hosted in one.
//
//----------------------------------------------------------------------------
HRESULT
BUGCALL
CBodyLayout::HandleMessage(CMessage *pMessage)
{
    HRESULT hr = S_FALSE;

    // have any mouse messages
    //
    if(     pMessage->message >= WM_MOUSEFIRST
        &&  pMessage->message <= WM_MOUSELAST
        &&  pMessage->message != WM_MOUSEMOVE
        &&  ElementOwner() == Doc()->_pElemCurrent)
    {
        RequestFocusRect(FALSE);
    }

    if (   (   pMessage->message >= WM_MOUSEFIRST
#ifndef WIN16
            && pMessage->message != WM_MOUSEWHEEL
#endif // ndef WIN16
            && pMessage->message <= WM_MOUSELAST)
        || pMessage->message == WM_SETCURSOR)
    {
        RECT rc;

        GetRect(&rc, COORDSYS_GLOBAL);

        if (    pMessage->htc != HTC_HSCROLLBAR
            &&  pMessage->htc != HTC_VSCROLLBAR
            &&  !PtInRect(&rc, pMessage->pt)
            &&  Doc()->GetCaptureObject() != ElementOwner() ) // marka - don't send message to frame if we have capture.
        {
            CFrameSite * pFrameSite = Doc()->ParentFrameSite();
            if (pFrameSite)
            {
                // we've got a FrameSite so now go to it's parent,
                // which is the FrameSetSite
                //
                CTreeNode * pNode = pFrameSite->GetFirstBranch()->Parent();
                CDoc *      pDoc = pFrameSite->Doc();
                Assert(pNode);

                // cons together a new msg to pass up to the frameset
                // so we don't have to hack directly on the CMessage
                // passed in.
                //
                CMessage msg(pMessage);
                MapWindowPoints(msg.hwnd,
                                pDoc->_pInPlace->_hwnd,
                                &msg.pt,
                                1);
                msg.hwnd = pDoc->_pInPlace->_hwnd;
                msg.SetNodeHit(pFrameSite->GetFirstBranch());

                // It turns out that CDoc::OnWindowMessage will toss
                // mouse up events unless it's seen a mouse down
                // event.  This hack tricks it into thinking it's
                // seen the down event.
                //
                if (pMessage->message == WM_LBUTTONDOWN)
                    pDoc->_fGotLButtonDown = TRUE;

                return THR_NOTRACE(pDoc->PumpMessage(&msg, pNode));
            }
            else
            {
                // Its on the inset, and we are not hosted inside
                // of a frame, so just consume the message here.
                //
                hr = S_OK;
                goto Cleanup;
            }
        }
    }

    hr = THR(super::HandleMessage(pMessage));

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CBodyLayout::UpdateScrollbarInfo, protected
//
//  Synopsis:   Update CDispNodeInfo to reflect the correct scroll-related settings
//
//-------------------------------------------------------------------------

void
CBodyLayout::UpdateScrollInfo(
    CDispNodeInfo * pdni) const
{
#ifdef NO_SCROLLBAR
    DWORD   dwFrameOptions = FRAMEOPTIONS_SCROLL_NO;
#else
    CDoc *  pDoc = Doc();

    //
    //  Treat the top-level print document as having clipping
    //

    if (    pDoc->IsPrintDoc()
        &&  pDoc->GetRootDoc() == pDoc)
    {
        pdni->_overflowX =
        pdni->_overflowY = styleOverflowHidden;
    }

    //
    //  If overflow was not set, use the HTML attributes set
    //

    if (    pdni->_overflowX == styleOverflowNotSet
        ||  pdni->_overflowY == styleOverflowNotSet)
    {
        DWORD   dwFrameOptions = pDoc->_dwFrameOptions & (  FRAMEOPTIONS_SCROLL_NO
                                                        |   FRAMEOPTIONS_SCROLL_YES
                                                        |   FRAMEOPTIONS_SCROLL_AUTO);

        if (pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_SCROLL_NO)
        {
            dwFrameOptions = FRAMEOPTIONS_SCROLL_NO;
        }
        else
        {
            switch (((CBodyLayout *)this)->Body()->GetAAscroll())
            {
            case bodyScrollno:
                dwFrameOptions = FRAMEOPTIONS_SCROLL_NO;
                break;

            case bodyScrollyes:
                dwFrameOptions = FRAMEOPTIONS_SCROLL_YES;
                break;

            case bodyScrollauto:
                dwFrameOptions = FRAMEOPTIONS_SCROLL_AUTO;
                break;

            case bodyScrolldefault:
                if (!dwFrameOptions)
                {
                    dwFrameOptions = FRAMEOPTIONS_SCROLL_YES;
                }
                break;
            }
        }

#endif // NO_SCROLLBAR

        switch (dwFrameOptions)
        {
        case FRAMEOPTIONS_SCROLL_NO:
            if (pdni->_overflowX == styleOverflowNotSet)
                pdni->_overflowX = styleOverflowHidden;
            if (pdni->_overflowY == styleOverflowNotSet)
                pdni->_overflowY = styleOverflowHidden;
            break;

        case FRAMEOPTIONS_SCROLL_AUTO:
            if (pdni->_overflowX == styleOverflowNotSet)
                pdni->_overflowX = styleOverflowAuto;
            if (pdni->_overflowY == styleOverflowNotSet)
                pdni->_overflowY = styleOverflowAuto;
            break;

        case FRAMEOPTIONS_SCROLL_YES:
        default:
            pdni->_sp._fHSBAllowed = TRUE;
            pdni->_sp._fHSBForced  = FALSE;
            pdni->_sp._fVSBAllowed =
            pdni->_sp._fVSBForced  = TRUE;
            break;
        }
    }

    //
    //  If an overflow value was set or generated, set the scrollbar properties using it
    //

    if (    pdni->_overflowX != styleOverflowNotSet
        ||  pdni->_overflowY != styleOverflowNotSet)
    {
        GetDispNodeScrollbarProperties(pdni);
    }
}


// JS OnSelection event
HRESULT
CBodyLayout::OnSelectionChange(void)
{
    DYNCAST(CBodyElement, ElementOwner())->Fire_onselect();  //JS event
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CalcSize
//
//  Synopsis:   see CSite::CalcSize documentation
//
//----------------------------------------------------------------------------
DWORD
CBodyLayout::CalcSize( CCalcInfo * pci,
                       SIZE *      psize,
                       SIZE *      psizeDefault)
{
    CScopeFlag  csfCalcing(this);
    DWORD dwRet = super::CalcSize(pci, psize, psizeDefault);

    // Update the focus rect
    if (_fFocusRect && ElementOwner() == Doc()->_pElemCurrent)
    {
        RedrawFocusRect();
    }

    return dwRet;

}

//+---------------------------------------------------------------------------
//
//  Member:     CBodyLayout::NotifyScrollEvent
//
//  Synopsis:   Respond to a change in the scroll position of the display node
//
//----------------------------------------------------------------------------

void
CBodyLayout::NotifyScrollEvent(
    RECT *  prcScroll,
    SIZE *  psizeScrollDelta)
{
    // Update the focus rect
    if (_fFocusRect && ElementOwner() == Doc()->_pElemCurrent)
    {
        RedrawFocusRect();
    }

    super::NotifyScrollEvent(prcScroll, psizeScrollDelta);
}

//+---------------------------------------------------------------------------
//
//  Member:     RequestFocusRect
//
//  Synopsis:   Turns on/off the focus rect of the body.
//
//  Arguments:  fOn     flag for requested state
//
//----------------------------------------------------------------------------
void
CBodyLayout::RequestFocusRect(BOOL fOn)
{
    if (!_fFocusRect != !fOn)
    {
        _fFocusRect = fOn;
        RedrawFocusRect();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     RedrawFocusRect
//
//  Synopsis:   Redraw the focus rect of the body.
//
//----------------------------------------------------------------------------
void
CBodyLayout::RedrawFocusRect()
{
    Assert(ElementOwner() == Doc()->_pElemCurrent);
    CView * pView = GetView();

    // Force update of focus shape
    pView->SetFocus(NULL, 0);
    pView->SetFocus(ElementOwner(), 0);
    pView->InvalidateFocus();
}

//+---------------------------------------------------------------------------
//
//  Member:     CBodyLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CBodyLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;
    CRectShape *    pShape;
    HRESULT         hr = S_FALSE;

    Assert(ppShape);
    *ppShape = NULL;

    if (!_fFocusRect || Doc()->IsPrintDoc())
        goto Cleanup;

    GetClientRect(&rc, CLIENTRECT_BACKGROUND);
    if (rc.IsEmpty())
        goto Cleanup;


    pShape = new CRectShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pShape->_rect = rc;
    pShape->_cThick = 2; // always draw extra thick for BODY
    *ppShape = pShape;

    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     GetBackgroundInfo
//
//  Synopsis:   Fills out a background info for which has details on how
//              to display a background color &| background image.
//
//-------------------------------------------------------------------------

BOOL
CBodyLayout::GetBackgroundInfo(
    CFormDrawInfo *     pDI,
    BACKGROUNDINFO *    pbginfo,
    BOOL                fAll,
    BOOL                fRightToLeft)
{
    Assert(pDI || !fAll);

    super::GetBackgroundInfo(pDI, pbginfo, fAll, fRightToLeft);

    if (pbginfo->crBack == COLORREF_NONE)
    {
        pbginfo->crBack = Doc()->_pOptionSettings->crBack();
        Assert(pbginfo->crBack != COLORREF_NONE);
    }

    return TRUE;
}
