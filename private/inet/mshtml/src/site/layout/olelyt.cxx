//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       olelyt.cxx
//
//  Contents:   Implementation of COleayout and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OLELYT_HXX_
#define X_OLELYT_HXX_
#include "olelyt.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_EOBJECT_HXX
#define X_EOBJECT_HXX
#include "eobject.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

DeclareTag(tagOleRgn, "OleLayout", "File window region");

MtDefine(COleLayout, Layout, "COleLayout");

ExternTag(tagOleSiteRect);
ExternTag(tagViewHwndChange);
extern BOOL IntersectRectE (RECT * prRes, const RECT * pr1, const RECT * pr2);


const CLayout::LAYOUTDESC COleLayout::s_layoutdesc =
{
    0, // _dwFlags
};


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::Reset
//
//  Synopsis:   Remove the object's HWND from the view's list of MFC-like hwnds.
//
//----------------------------------------------------------------------------

void
COleLayout::Reset(BOOL fForce)
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    HWND hwnd = pSiteOle->GetHwnd();
    
    if (hwnd != NULL)
    {
        GetView()->RemoveClippingOuterWindow(hwnd);
    }
    
    super::Reset(fForce);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::Draw
//
//  Synopsis:   Draw the object.
//
//----------------------------------------------------------------------------

void
COleLayout::Draw(CFormDrawInfo * pDI, CDispNode *)
{
    CDoc     *  pDoc     = Doc();
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    INSTANTCLASSINFO * pici;

    Assert(pSiteOle);

    if (pDI->_fInplacePaint)
    {
        Assert(pDI->_dwDrawAspect == DVASPECT_CONTENT);

        if (pSiteOle->_state == OS_OPEN)
        {
#ifndef WINCE
            HDC     hdc = pDI->GetGlobalDC(TRUE);
            HBRUSH  hbr;

            hbr = CreateHatchBrush(
                            HS_DIAGCROSS,
                            GetSysColorQuick(COLOR_WINDOWFRAME));
            FillRect(hdc, &pDI->_rc, hbr);
            DeleteObject(hbr);
#endif // WINCE
            goto Cleanup;
        }
        else if (pSiteOle->_state >= OS_INPLACE &&
                 !pSiteOle->_fWindowlessInplace)
        {
            TraceTagEx((tagOLEWatch,
                TAG_NONAME,
                "TRIDENT Draw with _state >= OS_INPLACE && !_fWindowlessInplace, site %x",
                pSiteOle));

#if DBG==1
            if (IsTagEnabled(tagOleRgn))
            {
                HWND    hwnd = pSiteOle->GetHwnd();
                HDC     hdc  = ::GetDC(hwnd);
                HBRUSH  hbr  = ::CreateSolidBrush((COLORREF)0x000000FF);
                HRGN    hrgn = ::CreateRectRgnIndirect(&g_Zero.rc);

                ::GetWindowRgn(hwnd, hrgn);
                ::FillRgn(hdc, hrgn, hbr);

                ::DeleteObject(hrgn);
                ::DeleteObject(hbr);
                ::ReleaseDC(hwnd, hdc);
            }
#endif

            goto Cleanup;
        }
    }

    Assert((pDI->_rc.right - pDI->_rc.left)
        && (pDI->_rc.bottom - pDI->_rc.top));
    
    // Don't draw unless:
    //
    //  1)  Object is windowless inplace...  or...
    //  2)  Baseline state is less than OS_INPLACE... or...
    //  3)  We're not drawing into the inplace dc (someone else called
    //      IVO::Draw on trident itself)... or...
    //  4)  The control is oc96 or greater.  This is because atl has
    //      a bug where it totally ignores the pfNoRedraw argument of
    //      IOleInPlaceSiteEx::OnInPlaceActivateEx.  If we didn't do
    //      the IVO::Draw here, the control would never paint.
    //      ie4 bug 54603.  (anandra)
    //
    // Otherwise calling IVO::Draw would likely leave a "ghost" image
    // that would soon be replaced when activation occurs - leading to
    // flicker...  (philco)
    //
    pici = pSiteOle->GetInstantClassInfo();
    
    if (   pSiteOle->_pVO
        && (   pSiteOle->_fWindowlessInplace
            || (pSiteOle->BaselineState(pDoc->State()) < OS_INPLACE)
            || !pDI->_fInplacePaint
            || (pici && pici->IsOC96())))
    {
        HDC     hdc = pDI->GetGlobalDC(!pDoc->IsPrintDoc()); // 61839: Don't clip when printing.
        POINT   ptBrushOriginSave, ptViewportOrigin, ptNewBrushOrigin;
        BOOL    fBrushOrgChanged;
        IViewObject * pVODelegate = NULL;
        
        ptBrushOriginSave = g_Zero.pt;  // this is just to appease the LINT (alexa)

        Assert( !_fSurface ||  pDI->IsMemory() ||  pDoc->IsPrintDoc());

        // BUGBUG: The marshalling code for IViewObject::Draw was
        //   not updated to the oc96 spec and thus does not know
        //   about the pvAspect usage.  It returns E_INVALIDARG if
        //   a non-NULL pvAspect is passed in.  So we check to see
        //   if we're talking to an OLE proxy and pass in NULL then.
        //   Otherwise we pass in the default value.
        //   (anandra) Apr-16-97.
        //
        // The MultiMedia Structured Graphics control mmsgrfxe.ocx
        // expects us to manage their brush origin.  Although the
        // contract does not state we have to, and MSDN in fact states
        // they should not count on the brush origin, it is fairly
        // easy for us to do it here so we extend this nice gesture.
        //
        // The issue is that whenever they move about on the screen
        // the brush origin must move with them so that any patterns
        // which are brushed onto their images remain aligned.  Also,
        // our off-screen rendering into a memory DC complicates things
        // by moving the viewport origin.
        //
        fBrushOrgChanged = GetViewportOrgEx(hdc, &ptViewportOrigin);
        if (fBrushOrgChanged)
        {
            ptNewBrushOrigin.x = (ptViewportOrigin.x + pDI->_rc.left) % 8;
            if( ptNewBrushOrigin.x < 0 )
                ptNewBrushOrigin.x += 8;
            ptNewBrushOrigin.y = (ptViewportOrigin.y + pDI->_rc.top) % 8;
            if( ptNewBrushOrigin.x < 0 )
                ptNewBrushOrigin.y += 8;

            fBrushOrgChanged = SetBrushOrgEx(
                            hdc,
                            ptNewBrushOrigin.x,
                            ptNewBrushOrigin.y,
                            &ptBrushOriginSave );
        }
        
        // a windowless in-place control expects to render at the coordinates
        // it was given in SetObjectRects.  We oblige by manipulating the
        // viewport origin so it will render in the correct place.  This is
        // especially important for filtered controls, since the offscreen
        // buffer kept by the filter has a different coordinate system than
        // the one that was used when SetObjectRects was called.
        RECTL* prc;
        if (pDI->_fInplacePaint && pSiteOle->_fWindowlessInplace)
        {
            prc = NULL;
            CPoint ptOrg = 
                pDI->_rc.TopLeft() + (CSize&)ptViewportOrigin
                - _rcWnd.TopLeft().AsSize();
            ::SetViewportOrgEx(hdc, ptOrg.x, ptOrg.y, NULL);
        }
        else
        {
            prc = (RECTL*) &pDI->_rc;
        }
        
        if (pSiteOle->OlesiteTag() == COleSite::OSTAG_APPLET   // if javaApplet
            && Doc()->IsPrintDoc()                             // and printing
            && pSiteOle->_pUnkPrintDelegate )                  // and we have a print delegate
        {
            THR_OLE(pSiteOle->_pUnkPrintDelegate->QueryInterface(IID_IViewObject, 
                                                                 (void**)&pVODelegate) );
        }
        
        THR_OLE( (pVODelegate ? pVODelegate : pSiteOle->_pVO ) ->Draw(
                pDI->_dwDrawAspect,
                pDI->_lindex,
                (pSiteOle->IsOleProxy() || pVODelegate )
                    ? NULL
                    : pDI->_pvAspect,
                pDI->_ptd,
                pDI->_hic,
                hdc,
                prc,
                pDI->_prcWBounds,
                pDI->_pfnContinue,
                pDI->_dwContinue));

        if (pVODelegate)
        {
            // we are done with this object, release all references
            pVODelegate->Release();
            ClearInterface(&(pSiteOle->_pUnkPrintDelegate) );
        }

        if (prc == NULL)
        {
            ::SetViewportOrgEx(hdc, ptViewportOrigin.x, ptViewportOrigin.y, NULL);
        }
        
        if (fBrushOrgChanged)
        {
            SetBrushOrgEx(
                    hdc,
                    ptBrushOriginSave.x,
                    ptBrushOriginSave.y,
                    NULL);
        }
    }
    else
    {
        // no _pVO
        //
        if (!(pSiteOle->_pUnkCtrl))
        {
            // object hasn't been created, so we show placeholder for it
            //
            HDC hdc = pDI->GetGlobalDC(TRUE);
            SIZE sizePrint;
            SIZE sizeGrab = {GRABSIZE, GRABSIZE};
            BOOL fPrint = pDoc->IsPrintDoc();
            if (fPrint)
            {
                GetPlaceHolderBitmapSize(
                        pSiteOle->_fFailedToCreate,
                        &sizePrint);
                sizePrint.cx = pDI->DocPixelsFromWindowX(sizePrint.cx);
                sizePrint.cy = pDI->DocPixelsFromWindowY(sizePrint.cy);
                sizeGrab.cx = pDI->DocPixelsFromWindowX(GRABSIZE);
                sizeGrab.cy = pDI->DocPixelsFromWindowY(GRABSIZE);
            }
            
            DrawPlaceHolder(
                    hdc,
                    pDI->_rc,
                    NULL, CP_UNDEFINED, 0, 0,  // no alt text
                    &sizeGrab,
                    pSiteOle->_fFailedToCreate,
                    0,
                    pSiteOle->GetBackgroundColor(),
                    fPrint ? &sizePrint : NULL,
                    FALSE,
                    pDI->DrawImageFlags());
        }
        else
        {
            // this is an object which is created but it does not
            // support _pVO
            //
            if (pDoc->_fDesignMode)
            {
                // Draw a rectangle so that we don't see garbage in the site
                //
                HDC hdc = pDI->GetGlobalDC(TRUE);
                HBRUSH hbrOld;

                hbrOld = (HBRUSH) SelectObject(
                                hdc,
                                GetStockObject(LTGRAY_BRUSH));
                Rectangle(
                        hdc,
                        pDI->_rc.left,
                        pDI->_rc.top,
                        pDI->_rc.right,
                        pDI->_rc.bottom);
                SelectObject(hdc, hbrOld);
            }
            TraceTagEx((tagOLEWatch,
                TAG_NONAME,
                "TRIDENT Attempt to draw when ViewObject is NULL, site %x",
                pSiteOle));
        }
    }

Cleanup:

    return;
}


//+-------------------------------------------------------------------------
//
//  Method:     COleLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------
DWORD
COleLayout::CalcSize( CCalcInfo * pci,
                      SIZE *      psize,
                      SIZE *      psizeDefault)
{
    Assert(pci);
    Assert(psize);
    Assert(ElementOwner());

    CScopeFlag      csfCalcing(this);
    CDoc          * pDoc         = Doc();
    CSize           sizeOriginal;
    CUnitValue      uvWidth      = GetFirstBranch()->GetCascadedwidth();
    CUnitValue      uvHeight     = GetFirstBranch()->GetCascadedheight();
    COleSite *      pSiteOle     = DYNCAST(COleSite, ElementOwner());
    RECT            tmpRect      = { 0, 0, 2048, 2048 };
    int             cxBorder;
    int             cyBorder;
    DWORD           grfReturn;
    INSTANTCLASSINFO * pici;


    CSaveCalcInfo   sci(pci, this);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    GetClientRect(&tmpRect, CLIENTRECT_USERECT, pci);
    cxBorder = (2048 - tmpRect.right)  + (tmpRect.left - 0);
    cyBorder = (2048 - tmpRect.bottom) + (tmpRect.top  - 0);

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    _fSizeThis = (_fSizeThis || (pci->_grfLayout & LAYOUT_FORCE));

    // If sizing or retrieving min/max, handle it here
    //
    if (   _fSizeThis
        || pci->_smMode == SIZEMODE_MMWIDTH
        || pci->_smMode == SIZEMODE_MINWIDTH
        || pci->_smMode == SIZEMODE_SET)
    {
        IOleObject *    pObject       = NULL;
        BOOL            fHaveWidth    = FALSE;
        BOOL            fHaveHeight   = FALSE;
        SIZEL           sizel;
        HRESULT         hr;
        
        sizel = g_Zero.size;

        _fContentsAffectSize = (uvWidth.IsNullOrEnum()
                                     || uvHeight.IsNullOrEnum());

        switch (pci->_smMode)
        {
        case SIZEMODE_MMWIDTH:
        case SIZEMODE_MINWIDTH:
            // If the user didn't specify a size in the html,
            // ask the control for it's preferred size.
            //
#if 0
            // Do not call IViewObjectEx::GetNaturalExtent because there's
            // a spec bug which does not detail the fact that the
            // DVEXTENTINFO struct can be NULL. ATL always dereferences it,
            // so we cannot safely call this function anymore.  Oh well.
            // See IE4 bug 44365. (anandra)
            //
            if (   (uvWidth.IsNullOrEnum() || uvHeight.IsNullOrEnum())
                && _fUseViewObjectEx)
            {
                HRESULT hr;
                IGNORE_HR(hr = ((IViewObjectEx *)(pSiteOle->_pVO)->GetNaturalExtent(
                                DVASPECT_CONTENT,
                                -1,
                                NULL,
                                NULL,
                                NULL,
                                &sizel));
                if(OK(hr))
                {
                    pci->DeviceFromHimetric(psize, sizel);
                    psize->cx += cxBorder;
                    psize->cy += cyBorder;
                    fHaveWidth  =
                    fHaveHeight = TRUE;
                }
                // else -- note that we will then try IOleObject::GetExtent()
                // below.
            }
#endif
            if(PercentWidth())
            {
                psize->cx = 0;
                fHaveWidth = TRUE;
            }

            if(PercentHeight())
            {
                psize->cy = 0;
                fHaveHeight = TRUE;
            }

            // Fall min/max requests through
            // (This is done to allow user specified settings to override
            //  the object's preferred size. Additionally, if the object
            //  does not have a preferred or user set size, the min/max are
            //  the object's current size.)

        case SIZEMODE_NATURAL:
            // If this control is marked invisible at run time, and we're in
            // browse mode, give it zero size.
            //
            if ((_fInvisibleAtRuntime && !pDoc->_fDesignMode) || pSiteOle->_fHidden)
            {
                fHaveWidth = fHaveHeight = TRUE;
                psize->cx = psize->cy = 0;
            }

            // Now see if we can get something directly from the html
            //
            if (!uvWidth.IsNullOrEnum() && !fHaveWidth)
            {
                psize->cx  = max(0L,
                                 uvWidth.XGetPixelValue(
                                         pci,
                                         psize->cx,  // Use available space.
                                         GetFirstBranch()->GetFontHeightInTwips(& uvWidth)));
                psize->cx += cxBorder;
                fHaveWidth = TRUE;
            }
            if (!uvHeight.IsNullOrEnum() && !fHaveHeight)
            {
                psize->cy   = max(0L,
                                  uvHeight.YGetPixelValue(
                                         pci,
                                         pci->_sizeParent.cy,
                                         GetFirstBranch()->GetFontHeightInTwips(& uvHeight)));
                psize->cy += cyBorder;
                fHaveHeight = TRUE;
            }

            // If either the width or height value is missing,
            // fill it in with the object's current size
            //
            if (!fHaveWidth || !fHaveHeight)
            {
                SIZE    size;

                hr = THR_OLE(pSiteOle->QueryControlInterface(
                        IID_IOleObject,
                         (void **)&pObject));
                if (!hr)
                {
                    hr = THR_OLE(pObject->GetExtent(DVASPECT_CONTENT, &sizel));
                    if (hr && pSiteOle->_state < OS_RUNNING)
                    {
                        hr = THR(pSiteOle->TransitionTo(OS_RUNNING));
                        if (OK(hr))
                        {
                            hr = THR_OLE(pObject->GetExtent(
                                            DVASPECT_CONTENT,
                                            &sizel));
                        }
                    }
                }

                // Use a default size if the request failed
                //
                if (hr)
                {
                    sizel.cx = HimetricFromHPix(16);
                    sizel.cy = HimetricFromVPix(16);
                    hr = S_OK; // so the error does not get propagated
                }
                Assert (!hr);

                pci->DeviceFromHimetric(&size, sizel);
                size.cx += cxBorder;
                size.cy += cyBorder;

                if (!fHaveWidth)
                {
                    psize->cx = size.cx;
                }
                if (!fHaveHeight)
                {
                    psize->cy = size.cy;
                }
            }

            // Fall SIZEMODE_NATURAL requests through to set the object's size
            // SIZEMODE_MM/MINWIDTH requests, however, stop here
            //
            if (   pci->_smMode == SIZEMODE_MMWIDTH
                || pci->_smMode == SIZEMODE_MINWIDTH)
            {
                if (pci->_smMode == SIZEMODE_MMWIDTH)
                {
                    psize->cy = psize->cx;
                }
                break;
            }

        case SIZEMODE_SET:
            // If we are asking for something different than last time,
            // then we negotiate with the control.
            //
            pci->HimetricFromDevice(&sizel,
                                    max(0L,(LONG)(psize->cx - cxBorder)),
                                    max(0L,(LONG)(psize->cy - cyBorder)));
            if (sizel.cx != _sizelLast.cx ||
                sizel.cy != _sizelLast.cy)
            {
                Assert(sizel.cx >= 0);
                Assert(sizel.cy >= 0);

                // Need to be running to set the size.
                //
                if (!pObject)
                {
                    pSiteOle->QueryControlInterface(
                            IID_IOleObject,
                            (void **)&pObject);
                }

                if (pObject)
                {
                    // Control must be at least running to set the extent.
                    if (pSiteOle->State() < OS_RUNNING)
                    {
                        IGNORE_HR(pSiteOle->TransitionTo(OS_RUNNING));
                    }

                    // The CDK implementation of IOleObject::SetExtent calls
                    // IOleInplaceSite::OnPosRectChange with the old position
                    // of the control.  We note that we are setting the extent
                    // in order to give OnPosRectChange a reasonable answer.
                    //
                    _sizelLast = sizel;

                    pici = pSiteOle->GetInstantClassInfo();

                    // If pici is NULL, we don't know if COMPAT_NO_SETEXTENT
                    // is set. If this is the case, do not set the extent.
                    //
                    if (pici && !(pici->dwCompatFlags & COMPAT_NO_SETEXTENT))
                    {
                        SIZEL   sizelTemp;

                        {
                            CElement::CLock lock1(pSiteOle, CElement::ELEMENTLOCK_RECALC);
                            COleSite::CLock lock2(pSiteOle, COleSite::OLESITELOCK_SETEXTENT);

                            hr = THR_OLE(pObject->SetExtent(
                                    DVASPECT_CONTENT,
                                    &sizel));
                        }

                        // Some controls don't listen to what we tell them,
                        // so ask the control again about how large it wants
                        // to be.  Basically the control has veto power
                        // over what the html says
                        //
                        if (OK(THR_OLE(pObject->GetExtent(
                                    DVASPECT_CONTENT,
                                    &sizelTemp))))
                        {
                            _sizelLast = sizel = sizelTemp;
                        }

                        // Reset sizel to zero if we're invisibleatruntime
                        // The ms music control responds with non-zero extent
                        // even after SetExtent(0).  IE4 bug 36598. (anandra)
                        //
                        if ((_fInvisibleAtRuntime && !pDoc->_fDesignMode) || pSiteOle->_fHidden)
                        {
                            sizel.cx = sizel.cy = 0;
                        }
                    }
                }
            }

            // Take whatever we have at this point as the size.
            //
            pci->DeviceFromHimetric(psize, sizel);

            // Add in any hspace/vspace and borders.
            //
            psize->cx += cxBorder;
            psize->cy += cyBorder;

            //
            // If dirty, ensure display tree nodes exist
            //

            if (    _fSizeThis
                &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
            {
                SetPositionAware();
                SetInsertionAware();

                grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
            }

            _fSizeThis    = FALSE;
            grfReturn    |= LAYOUT_THIS  |
                            (psize->cx != sizeOriginal.cx
                                    ? LAYOUT_HRESIZE
                                    : 0) |
                            (psize->cy != sizeOriginal.cy
                                    ? LAYOUT_VRESIZE
                                    : 0);

            //
            // Size display nodes if size changes occurred
            //

            if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                SizeDispNode(pci, *psize);
            }

            //
            //  Ensure baseline state
            //
            
            if (pSiteOle->_pUnkCtrl)
            {
                OLE_SERVER_STATE os = pSiteOle->BaselineState(pDoc->State());

                if (os > pSiteOle->State())
                    pDoc->GetView()->DeferTransition(pSiteOle);
            }

            break;

        default:
            Assert(0);
            break;
        }
        ReleaseInterface(pObject);
    }

    // Otherwise, defer to default handling
    else
    {
        grfReturn = super::CalcSize(pci, psize, psizeDefault);
    }

    return grfReturn;
}


//+------------------------------------------------------------------------
//
//  Member:     GetMarginInfo
//
//  Synopsis:   add hspace/vspace into the CSS margin
//
//-------------------------------------------------------------------------

void
COleLayout::GetMarginInfo(CParentInfo * ppri,
                          LONG        * plLMargin,
                          LONG        * plTMargin,
                          LONG        * plRMargin,
                          LONG        * plBMargin)
{
    super::GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);

    if (Tag() == ETAG_OBJECT || Tag() == ETAG_APPLET)
    {
        CObjectElement * pObject = DYNCAST(CObjectElement, ElementOwner());
        LONG lhMargin = pObject->GetAAhspace();
        LONG lvMargin = pObject->GetAAvspace();

        if (lhMargin < 0)
            lhMargin = 0;
        if (lvMargin < 0)
            lvMargin = 0;

        if (plLMargin)
        {
            *plLMargin += lhMargin;
        }
        if (plRMargin)
        {
            *plRMargin += lhMargin;
        }

        if (plTMargin)
        {
            *plTMargin += lvMargin;
        }
        if (plBMargin)
        {
            *plBMargin += lvMargin;
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     OnFormatsChange
//
//  Synopsis:   Handle formats change notification
//
//-------------------------------------------------------------------------

HRESULT
COleLayout::OnFormatsChange(DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    IOleControl *   pControl;

    hr = THR(super::OnFormatsChange(dwFlags));
    if(hr)
        goto Cleanup;

    if (OK(DYNCAST(COleSite, ElementOwner())->QueryControlInterface(IID_IOleControl, (void **)&pControl)))
    {
        IGNORE_HR(pControl->OnAmbientPropertyChange(DISPID_UNKNOWN));
        pControl->Release();
    }

Cleanup:
    RRETURN(hr);
}


#if 1
// BUGBUG (donmarsh) - IE 4 used the following routine to handle extra
// invalidation, which is necessary if we have an asynchronously-drawn OLE
// control.  This actually causes performance degradation for scrolling of
// all OLE controls (not just asynchronous ones), and visible flashing.
// To fix, we need to grab a lock that prevents anything from drawing to the
// screen before our deferred set object rects call is complete.  There isn't
// enough time to make such a drastic change before our beta 2 release, we
// should review this for RTM version.

//+-----------------------------------------------------------------------------
//
// Synopsis:   in assumption that rcOld rectangle was shifted along one axis to
//             rectangle rcNew, this function finds the rectangle of difference
//             between rcOld and rcNew. [if the shift was along 2 axes then the
//             difference will be non-rectangular area]
//
//------------------------------------------------------------------------------
static BOOL
FindRectAreaDifference (CRect * prcRes, const CRect& rcOld, const CRect& rcNew)
{
    Assert (prcRes);

    // if size of prOld is different from size of prNew
    //
    if (rcOld.Width() != rcNew.Width() || rcOld.Height() != rcNew.Height())
    {
        return FALSE;
    }

    if (rcNew.top < rcOld.top)
    {
        // rect was shifted up
        // if also shifted along the other axis
        //
        if (rcNew.left != rcOld.left)
            return FALSE;

        prcRes->SetRect(rcNew.left, rcNew.bottom, rcNew.right, rcOld.bottom);
    }
    else if (rcOld.top < rcNew.top)
    {
        // rect was shifted down
        // if also shifted along the other axis
        //
        if (rcNew.left != rcOld.left)
            return FALSE;

        prcRes->SetRect(rcNew.left, rcOld.top, rcNew.right, rcNew.top);
    }
    else if (rcNew.left < rcOld.left)
    {
        // rect was shifted to the left
        // if also shifted along the other axis
        //
        if (rcNew.top != rcOld.top)
            return FALSE;

        prcRes->SetRect(rcNew.right, rcNew.top, rcOld.right, rcNew.bottom);
    }
    else if (rcOld.left < rcNew.left)
    {
        // rect was shifted to the right
        // if also shifted along the other axis
        //
        if (rcNew.top != rcOld.top)
            return FALSE;

        prcRes->SetRect(rcOld.left, rcNew.top, rcNew.left, rcNew.bottom);
    }
    else
    {
        // position was not changed - set rect to empty
        //
        prcRes->SetRectEmpty();
    }

    return TRUE;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     HandleViewChange
//
//  Synopsis:   Respond to change of in view status
//
//  Arguments:  flags           flags containing state transition info
//              prcClient       client rect in global coordinates
//              prcClip         clip rect in global coordinates
//              pDispNode       node which moved
//
//----------------------------------------------------------------------------
void
COleLayout::HandleViewChange(
    DWORD           flags,
    const RECT*     prcClient,
    const RECT*     prcClip,
    CDispNode*      pDispNode)
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    HWND        hwnd;
    INSTANTCLASSINFO * pici;
    BOOL fInvalidate = FALSE;

    if (pSiteOle->_state < OS_INPLACE)
    {
        _rcWnd = *prcClient;
        return;
    }
    
    hwnd = pSiteOle->GetHwnd();
    if (hwnd == NULL)
    {
        _rcWnd = *prcClient;
    }
    else
    {
        CTreeNode * pTreeNode = pSiteOle->GetFirstBranch();
        CRect* prcInvalid = NULL;
        CRect rcInvalid;

        if (    pTreeNode->IsVisibilityHidden()
            ||  pTreeNode->IsDisplayNone())
        {
            flags |= VCF_INVIEWCHANGED;
            flags &= ~VCF_INVIEW;
        }
        
        // When positioning objects, we move the object
        // hwnd from under it.  This is for two reasons: it
        // reduces flicker when scrolling because we don't invalidate,
        // and we can then draw the bits on the screen directly as
        // well.  That way when the object calls MoveWindow in response
        // to SetObjectRects, nothing happens.
        
        DWORD positionChangeFlags = SWP_NOACTIVATE | SWP_NOZORDER;
        CRect rcTarget = *prcClient;
        CRect rcWndBefore;
        BOOL fIsClippingOuterWindow = GetView()->IsClippingOuterWindow(hwnd);

        // if it's an MFC-like control, move directly to the clipping rect
        if (fIsClippingOuterWindow)
        {
            rcTarget = *prcClip;
        }
        
        GetView()->GetHWNDRect(hwnd, &rcWndBefore);
        if (!fIsClippingOuterWindow || rcWndBefore.Size() == rcTarget.Size())
        {
            positionChangeFlags |= SWP_NOSIZE;
        }
        
        if (flags & VCF_INVIEWCHANGED)
        {
            positionChangeFlags |=
                (flags & VCF_INVIEW) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
            DeferSetWindowPos(hwnd, &rcTarget, positionChangeFlags, NULL);
        }
        else
        {
            BOOL fClipMoved = ! (rcWndBefore.Size() == CRect(*prcClip).Size() &&
                     (rcWndBefore.TopLeft() - _rcWnd.TopLeft()) ==
                     (CRect(*prcClip).TopLeft() - CRect(*prcClient).TopLeft()));
            if (flags & VCF_NOREDRAW)
            {
                if (!fIsClippingOuterWindow || !fClipMoved)
                {
                    positionChangeFlags |= SWP_NOREDRAW;
                }
                else
                {
                    fInvalidate = TRUE;
                }
#if 1
// BUGBUG (donmarsh) - IE 4 did the following extra invalidation, which
// is necessary to clean up something that gets drawn asynchronously in the
// wrong position before our set object rects call completes.  See comment
// above FindRectAreaDifference.
                if (FindRectAreaDifference(&rcInvalid, _rcWnd, (const CRect&) *prcClient))
                    prcInvalid = &rcInvalid;
#endif
            }
      
            DeferSetWindowPos(hwnd, &rcTarget, positionChangeFlags, prcInvalid);
        }
        
        // remember our new "theoretical" rect to allow subsequent
        // DeferSetWindowPos call above
        _rcWnd = *prcClient;
        
        // (alexz) (anandra)
        // set window region to clip rectangle. This eliminates
        // flickering for several controls - e.g. several of ncompass
        // controls or proto view treeview. These controls flicker
        // if rcClip is different from rc in SetObjectRects,
        // which, however, gets fixed by proper call to SetWindowRgn.
        // rc can be different from rcClip very often - during
        // scrolling when object is only partially visible.
        // NOTE: This used to be done only for a known set of controls (via a compatibility flag).
        //       However, it turns out that more controls need it so now it is done all the
        //       time. (brendand)
        // Bugs: # 37662, 43877, 43885.
        {
            CRect rcClip(*prcClip);
            rcClip.OffsetRect(-rcTarget.TopLeft().AsSize());
            //
            //  NOTE: The following call to has been changed from asynch to synch since the asynch form
            //        results in the 3rd party application, FirstAid, faulting! (brendand/kensy)
            //
            pici = pSiteOle->GetInstantClassInfo();
            
            if (pici && (pici->dwCompatFlags & COMPAT_ALWAYSDEFERSETWINDOWRGN))
            {
                DeferSetWindowRgn(hwnd, &rcClip, !prcInvalid);
            }
            else
            {
                ::SetWindowRgn(hwnd, ::CreateRectRgnIndirect(&rcClip), !prcInvalid);

                TraceTag((tagViewHwndChange, "SWRgn %x Rect: %ld %ld %ld %ld  Redraw: %u",
                    hwnd,
                    rcClip.top, rcClip.bottom, rcClip.left, rcClip.right,
                    !prcInvalid));
            }
        }
    }
    
    RECT rcNeg = { -1, -1, -1, -1 };

    DeferSetObjectRects(
        pSiteOle->_pInPlaceObject, 
        (pSiteOle->IsDisplayNone() || pSiteOle->IsVisibilityHidden())
            ? &rcNeg
            : (RECT*) prcClient,
        (RECT*) prcClip,
        hwnd,
        fInvalidate);

}


//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::ProcessDisplayTreeTraversal
//              
//  Synopsis:   Add our window to the z order list.
//              
//  Arguments:  pClientData     window order information
//              
//  Returns:    TRUE to continue display tree traversal
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
COleLayout::ProcessDisplayTreeTraversal(void *pClientData)
{
    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    
    if (pSiteOle && pSiteOle->_state >= OS_INPLACE)
    {
        HWND hwnd = pSiteOle->GetHwnd();
        if (hwnd)
        {
            CView::CWindowOrderInfo* pWindowOrderInfo =
                (CView::CWindowOrderInfo*) pClientData;
            pWindowOrderInfo->AddWindow(hwnd);
        }
    }
    
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Member:     COleLayout::HitTestContent
//
//  Synopsis:   Determine if the given display leaf node contains the hit point.
//
//  Arguments:  pptHit          hit test point
//              pDispNode       pointer to display node
//              pClientData     client-specified data value for hit testing pass
//
//  Returns:    TRUE if the display leaf node contains the point
//
//----------------------------------------------------------------------------

BOOL
COleLayout::HitTestContent(
    const POINT *   pptHit,
    CDispNode *     pDispNode,
    void *          pClientData)
{
    Assert(pptHit);
    Assert(pDispNode);
    Assert(pClientData);

    COleSite *  pSiteOle = DYNCAST(COleSite, ElementOwner());
    CHitTestInfo *  phti = (CHitTestInfo *) pClientData;
    HTC htc = HTC_YES;
    BOOL fRet = TRUE;

    Assert(pSiteOle);

    if (pSiteOle->_fUseViewObjectEx)
    {
        DWORD dwHitResult = HITRESULT_OUTSIDE;

        if (pSiteOle->_pVO)
        {
            RECT rcClient;

            GetClientRect(&rcClient);

            if (FAILED(((IViewObjectEx *) (pSiteOle->_pVO))->QueryHitPoint(
                                                    DVASPECT_CONTENT,
                                                    &rcClient,
                                                    *pptHit,
                                                    0,
                                                    &dwHitResult)))
                dwHitResult = HITRESULT_OUTSIDE;
        }

        if (dwHitResult == HITRESULT_OUTSIDE)
        {
            htc = HTC_NO;
            fRet = FALSE;
        }
        else if (dwHitResult != HITRESULT_HIT)   // HITRESULT_TRANSPARENT or HITRESULT_CLOSE
        {
            fRet = FALSE;
        }
    }

    if ((phti->_htc == HTC_NO) || fRet)
    {
        phti->_htc = htc;

        if (htc == HTC_YES)
        {
            phti->_pNodeElement = ElementContent()->GetFirstBranch();
            phti->_ptContent    = *pptHit;
            phti->_pDispNode    = pDispNode;
            phti->_phtr->_fWantArrow = TRUE;
        }
    }

    return(fRet);
}

//+---------------------------------------------------------------------------
//  Member:     COleLayout::DragEnter
//
//  Synopsis:   This is called when an object is started to be dragged over this 
//              layout area. If the inplace object is a windowless control, then 
//              the call is delegated to the control. Otherwise, the call is 
//              delegated to the super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::DragEnter(
                    IDataObject *pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    DWORD *pdwEffect)
{
    HRESULT             hr;
    IDropTarget *       pDT = NULL;
    COleSite    *       pSiteOle = DYNCAST(COleSite, ElementOwner());
    IPointerInactive *  pPI = NULL;
    DWORD               dwPolicy = 0;

    // check if the control needs to be activated because of the drag drop operation
    if (pSiteOle->_state < OS_INPLACE)
    {
        // the control is not inplace active, inplace activation is needed.

        if (!THR_NOTRACE(pSiteOle->QueryControlInterface(IID_IPointerInactive, (void **) &pPI)))
        {
            hr = THR(pPI->GetActivationPolicy(&dwPolicy));
            if (hr)
                goto Cleanup;

            // should we activate the control if not active?
            if (dwPolicy & POINTERINACTIVE_ACTIVATEONDRAG)
            {
                hr = THR(pSiteOle->TransitionTo(OS_INPLACE, NULL));
                if (hr)
                    goto Cleanup;
            }
        }
    }
    
    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));
        if (!hr)
        {

// CONSIDERATION: (ferhane)
// In the call below, if the DragEnter is returning S_FALSE then it means that 
// the control is trying to delegate the call to us. We could immediately drop step and 
// call the super for the hr==S_FALSE case, instead of returning S_FALSE to the caller.
// However, that would require us to keep track of which data types was refused by this
// control, so that we could not call it but process the call ourselves for the following 
// DragOver and Drop calls that we would receive.
// According to spec, there is no guarantee that any of the IDropTarget methods other than
// the DragEnter would return S_FALSE. So, we are the ones ending up tracking this ... 

            //delegate the call
            hr = THR_OLE(pDT->DragEnter(pDataObj, grfKeyState, pt, pdwEffect));
                goto Cleanup;            
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::DragEnter(pDataObj, grfKeyState, pt, pdwEffect));

Cleanup:
    ReleaseInterface(pDT);
    ReleaseInterface(pPI);

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//  Member:     COleLayout::DragOver
//
//  Synopsis:   This is called when an object is being dragged over this layout 
//              area. If the inplace object is a windowless control, then the 
//              call is delegated to the control. Otherwise, the call is delegated 
//              to the super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
    HRESULT         hr;
    IDropTarget *   pDT = NULL;
    COleSite    *   pSiteOle = DYNCAST(COleSite, ElementOwner());

    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));
        if (!hr)
        {
            //delegate the call
            hr = THR_OLE(pDT->DragOver( grfKeyState, pt, pdwEffect));
            goto Cleanup;
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::DragOver( grfKeyState, pt, pdwEffect));

Cleanup:
    ReleaseInterface(pDT);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Member:     COleLayout::Drop
//
//  Synopsis:   This is called when an object is dropped on this layout area.
//              If the inplace object is a windowless control, then the call is 
//              delegated to the control. Otherwise, the call is delegated to the 
//              super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::Drop(   IDataObject *pDataObj,
                    DWORD grfKeyState,
                    POINTL pt,
                    DWORD *pdwEffect)
{
    HRESULT         hr;
    IDropTarget *   pDT = NULL;
    COleSite    *   pSiteOle = DYNCAST(COleSite, ElementOwner());

    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));
        if (!hr)
        {
            //delegate the call
            hr = THR_OLE(pDT->Drop( pDataObj, grfKeyState, pt, pdwEffect));
            goto Cleanup;
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::Drop( pDataObj, grfKeyState, pt, pdwEffect));

Cleanup:
    ReleaseInterface(pDT);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     COleLayout::DragLeave
//
//  Synopsis:   This is called when an object that was being dragged over this area 
//              leaves the area. If the inplace object is a windowless control, then the 
//              call is delegated to the control. Otherwise, the call is delegated 
//              to the super.
//----------------------------------------------------------------------------
HRESULT 
COleLayout::DragLeave()
{
    HRESULT         hr;
    IDropTarget *   pDT = NULL;
    COleSite    *   pSiteOle = DYNCAST(COleSite, ElementOwner());

    // if the ole site is a windowless control, we must delegate the call to the control.
    if (pSiteOle->_fWindowlessInplace)
    {
        // check if the control has a drop target.
        hr = THR_OLE(((IOleInPlaceObjectWindowless *)pSiteOle->_pInPlaceObject)->GetDropTarget(&pDT));
        if (!hr)
        {
            //delegate the call
            hr = THR_OLE(pDT->DragLeave());
            goto Cleanup;
        }
    }

    // Handle the call as if there was not a windowless control..
    hr = THR_OLE(super::DragLeave());

Cleanup:
    ReleaseInterface(pDT);

    RRETURN(hr);
}
