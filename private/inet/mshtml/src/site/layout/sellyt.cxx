//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       sellyt.cxx
//
//  Contents:   Implementation of CSelectLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_SELLYT_HXX_
#define X_SELLYT_HXX_
#include "sellyt.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_SBBASE_HXX_
#define X_SBBASE_HXX_
#include "sbbase.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_BUTTUTIL_HXX_
#define X_BUTTUTIL_HXX_
#include "buttutil.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

MtDefine(CSelectLayout, Layout, "CSelectLayout")

IMPLEMENT_LAYOUT_FNS(CSelectElement, CSelectLayout)

const CLayout::LAYOUTDESC CSelectLayout::s_layoutdesc =
{
    LAYOUTDESC_NEVEROPAQUE, // _dwFlags
};

DeclareTag(tagSelectSetPos, "SelectPos", "Trace SELECT SetPositions");
DeclareTag(tagSelectNotify, "SelectNotify", "Trace the flat SELECT notifications");


//+------------------------------------------------------------------------
//
//  Member:     CSelectLayout::Notify
//
//  Synopsis:   Hook into the layout notification. listen for any changed in the tree
//              under the SELECT
//
//
//-------------------------------------------------------------------------

void
CSelectLayout::InternalNotify(void)
{
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());

    if ( !TestLock(CElement::ELEMENTLOCK_SIZING) &&
         pSelect->_fEnableLayoutRequests )
    {

        //
        //  Otherwise, accumulate the information
        //
        BOOL    fWasDirty = IsDirty();

        //
        //  If transitioning to dirty, post a layout request
        //

        _fDirty = TRUE;
        pSelect->_fOptionsDirty = TRUE;

        TraceTag((tagSelectNotify, "SELECT 0x%lx was dirtied", this));

        if ( !fWasDirty && IsDirty() )
        {
            TraceTag((tagSelectNotify, "SELECT 0x%lx enqueueing layout request", this));
            PostLayoutRequest(LAYOUT_MEASURE);
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CSelectLayout::Notify
//
//  Synopsis:   Hook into the layout notification. listen for any changed in the tree
//              under the SELECT
//
//
//-------------------------------------------------------------------------
void
CSelectLayout::Notify(CNotification * pnf)
{
    Assert(!pnf->IsReceived(_snLast));

    BOOL    fWasDirty = IsDirty() || _fSizeThis;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());

    super::Notify(pnf);

    if (    TestLock(CElement::ELEMENTLOCK_SIZING)
        ||  pnf->IsType(NTYPE_ELEMENT_RESIZE)
        ||  pnf->IsType(NTYPE_ELEMENT_ENSURERECALC)
        ||  pnf->IsType(NTYPE_ELEMENT_RESIZEANDREMEASURE))
        goto Cleanup;

    pSelect = DYNCAST(CSelectElement, ElementOwner());

    if ( ! pSelect->_fEnableLayoutRequests )
        goto Cleanup;

    if (IsInvalidationNotification(pnf))
    {
        if (pSelect->_hwnd)
        {
            ::InvalidateRect(pSelect->_hwnd, NULL, FALSE);
        }
    }

    if (pnf->IsType(NTYPE_ELEMENT_ADD_ADORNER))
        goto Cleanup;
        
    if ( pnf->IsHandled() )
        goto Cleanup;

    if ( !pnf->IsTextChange())
        goto Handled;

    _fDirty = TRUE;
    pSelect->_fOptionsDirty = TRUE;
    if (pSelect->_hwnd)
        pSelect->PushStateToControl();

    TraceTag((tagSelectNotify, "SELECT 0x%lx was dirtied", this));

    if ( !fWasDirty && IsDirty() )
    {
        TraceTag((tagSelectNotify, "SELECT 0x%lx enqueueing layout request", this));
        PostLayoutRequest(pnf->LayoutFlags() | LAYOUT_MEASURE);
    }


Handled:
    pnf->SetHandler(pSelect);

Cleanup:
    return;
}





//+-------------------------------------------------------------------------
//
//  Method:     DoLayout
//
//  Synopsis:   Layout contents
//
//  Arguments:  grfLayout   - One or more LAYOUT_xxxx flags
//
//--------------------------------------------------------------------------

void
CSelectLayout::DoLayout(DWORD grfLayout)
{
    Assert(grfLayout & (LAYOUT_MEASURE | LAYOUT_POSITION | LAYOUT_ADORNERS));

    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());
    BOOL fTreeChanged = IsDirty();

    TraceTag((tagSelectNotify, "SELECT 0x%lx DoLayout called, flags: 0x%lx", this, grfLayout));

    Assert(grfLayout & LAYOUT_MEASURE);
    Assert(!(grfLayout & LAYOUT_ADORNERS));
    Assert(!HasRequestQueue());

    if ( _fDirty )
    {
        if (pSelect->_hwnd)
            pSelect->PushStateToControl();

        _fDirty = FALSE;
        pSelect->ResizeElement();

    }

        //  Remove the request from the layout queue
    TraceTag((tagSelectNotify, "SELECT 0x%lx DEqueueing layout request", this));
    RemoveLayoutRequest();

    if ( _fSizeThis )
    {
        CCalcInfo   CI(this);

        TraceTag((tagSelectNotify, "SELECT 0x%lx DoLayout caused EnsureDefaultSize", this));
        CI._grfLayout |= grfLayout;

        if (_fForceLayout)
        {
            CI._grfLayout |= LAYOUT_FORCE;
            _fForceLayout = FALSE;

            EnsureDispNode(&CI, TRUE);
            SetPositionAware();
        }

        if ( fTreeChanged || (grfLayout & LAYOUT_FORCE) )
        {
            pSelect->_sizeDefault.cx = pSelect->_sizeDefault.cy = 0;
        }
        EnsureDefaultSize(&CI);
    }

}

//+------------------------------------------------------------------------
//
//  Member:     CSelectLayout::EnsureDefaultSize, protected
//
//  Synopsis:   Compute the default size of the control
//              if it hasn't been done yet.
//
//  Note:       Jiggles the window to handle integralheight listboxes correctly.
//
//-------------------------------------------------------------------------
#define CX_SELECT_DEFAULT_HIMETRIC  634L  // 1/4 logical inch

HRESULT
CSelectLayout::EnsureDefaultSize(CCalcInfo * pci)
{
    Assert(ElementOwner());

    HRESULT             hr = S_OK;
    long                lSIZE;
    CRect               rcSave;
    int                 i;
    long                lWidth, lMaxWidth;
    COptionElement *    pOption;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());
    SIZE &              sizeDefault = pSelect->_sizeDefault;

    if ( sizeDefault.cx && sizeDefault.cy )
        goto Cleanup;

#if DBG == 1
    _cEnsureDefaultSize++;
#endif

    lMaxWidth = 0;

    lSIZE = pSelect->GetAAsize();

    if ( lSIZE == 0 )
    {
        lSIZE = pSelect->_fMultiple ? 4 : 1;
    }

    sizeDefault.cy = pSelect->_lFontHeight * lSIZE +
                      pci->DocPixelsFromWindowX(6);

    pSelect->BuildOptionsCache();

    //  Measure the listbox lines
    for ( i = pSelect->_aryOptions.Size() - 1; i >= 0; i-- )
    {
        pOption = pSelect->_aryOptions[i];
        lWidth = pOption->MeasureLine(pci);

        if ( lWidth > lMaxWidth )
        {
            lMaxWidth = lWidth;
            pSelect->_poptLongestText = pOption;
        }
    }

    sizeDefault.cx = pci->DocPixelsFromWindowX(g_sizeScrollbar.cx) +
                      lMaxWidth +
                      pci->DocPixelsFromWindowX(6 + 4);   //  6 is magic number for borders

    if ( sizeDefault.cx < pci->DeviceFromHimetricCX(CX_SELECT_DEFAULT_HIMETRIC) )
    {
        sizeDefault.cx = pci->DeviceFromHimetricCX(CX_SELECT_DEFAULT_HIMETRIC);
    }

    pSelect->_lComboHeight = pSelect->_lFontHeight * DEFAULT_COMBO_ITEMS;

    pSelect->_lMaxWidth = lMaxWidth;
    _fDirty = FALSE;

Cleanup:
    RRETURN(hr);
}




//+-------------------------------------------------------------------------
//
//  Method:     CSelectLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CSelectLayout::CalcSize ( CCalcInfo * pci,
                          SIZE *      psize,
                          SIZE *      psizeDefault)
{
    Assert(ElementOwner());
    CScopeFlag          csfCalcing(this);
    CSaveCalcInfo       sci(pci);
    HRESULT             hr;
    DWORD               grfLayout = 0;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());
    SIZE &              sizeDefault = pSelect->_sizeDefault;

#if DBG == 1
    _cCalcSize++;
#endif

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    _fSizeThis = _fSizeThis || (pci->_grfLayout & LAYOUT_FORCE);

    if(  pSelect->_fNeedMorph && pSelect->_hwnd ||
        (_pDispNode && ( (unsigned)_pDispNode->IsRightToLeft() != _fRightToLeft)))
    {
        // We may have changed our flow direction. Recreate the control.
        pSelect->Morph();
        if(_pDispNode)
            GetElementDispNode()->RequestViewChange();
    }

    if ( ! pSelect->_hwnd )
    {
        hr = THR(AcquireFont(pci));
        if ( FAILED(hr) )
            goto Cleanup;

        if ( Doc() && Doc()->_pInPlace && Doc()->_pInPlace->_hwnd )
        {
            hr = THR(pSelect->EnsureControlWindow());

            if ( hr )
                goto Cleanup;
        }
    }
    else if ( pSelect->_fRefreshFont ) 
    {
        hr = THR(AcquireFont(pci));
        if ( FAILED(hr)) 
            goto Cleanup;

        Assert(pSelect->_hFont);

        // AcquireFont returns S_FALSE if the new font is the same as the old one.
        if (hr != S_FALSE)
        {
            ::SendMessage(pSelect->_hwnd, WM_SETFONT, (WPARAM)pSelect->_hFont, MAKELPARAM(FALSE,0));
            if (! pSelect->_fListbox )
            {
                pSelect->SendSelectMessage(CSelectElement::Select_SetItemHeight, (WPARAM)-1, pSelect->_lFontHeight);
            }
            pSelect->SendSelectMessage(CSelectElement::Select_SetItemHeight, 0, pSelect->_lFontHeight);
        }
        hr = S_OK;
    }

    if ( pci->_grfLayout & LAYOUT_FORCE || _fDirty)
    {
        sizeDefault.cx = sizeDefault.cy = 0;
    }

    IGNORE_HR(EnsureDefaultSize(pci));

Cleanup:

    TraceTag((tagSelectSetPos, "SELECT 0x%lx CalcSize, cx=%d, cy=%d", this, psize->cx, psize->cy));

    // if we're a combo box, ignore height set by CSS and use
    // the default height. BUT, we do use the CSS width if it 
    // has been set.
    if (! pSelect->_fListbox)
    {
        CTreeNode * pTreeNode     = GetFirstBranch();
        CUnitValue  uvx;

        if (pTreeNode)
        {
            uvx = pTreeNode->GetCascadedwidth();
            sizeDefault.cx = (uvx.IsNullOrEnum() || (pci->_grfLayout & LAYOUT_USEDEFAULT)
                                    ? sizeDefault.cx
                                    : max(0L, uvx.XGetPixelValue(pci, pci->_sizeParent.cx,
                                                        pTreeNode->GetFontHeightInTwips(&uvx))));

            pci->_grfLayout |= LAYOUT_USEDEFAULT;
        }
    }

    grfLayout |= super::CalcSize(pci, psize, &sizeDefault);

    //
    // If anything changed, ensure the display node is position aware
    //
    if (    _pDispNode
        &&  (grfLayout & (LAYOUT_THIS | LAYOUT_HRESIZE | LAYOUT_VRESIZE)))
    {
        SetPositionAware();
    }

    return grfLayout;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::AcquireFont, protected
//
//  Synopsis:   Beg, borrow or steal a font and set it to the control window
//
//  returns:    S_FALSE if the current font and old font are the same
//
//----------------------------------------------------------------------------

HRESULT
CSelectLayout::AcquireFont(CCalcInfo * pci)
{
    Assert(ElementOwner());

    HRESULT             hr = S_OK;
    LOGFONT             lfOld;
    LOGFONT             lf;
    CCcs *              pccs = NULL;
    CSelectElement *    pSelect = DYNCAST(CSelectElement, ElementOwner());

    const CCharFormat * pcf = GetFirstBranch()->GetCharFormat();

    memset(&lfOld, 0, sizeof(lfOld));
    memset(&lf, 0, sizeof(lf));

    if ( ! pcf )
        goto Error;

    pccs = fc().GetCcs(pci->_hdc, pci, pcf);
    if ( ! pccs )
        goto Error;

    lf = pccs->GetBaseCcs()->_lf;    //  Copy it out
    lf.lfUnderline = pcf->_fUnderline;
    lf.lfStrikeOut = pcf->_fStrikeOut;

    if (pSelect->_hFont)
    {
        ::GetObject(pSelect->_hFont, sizeof(lfOld), &lfOld);
        if (!memcmp(&lf, &lfOld, sizeof(lfOld)))
        {
            hr = S_FALSE;
            goto Cleanup;
        }
        else
        {
            Verify(DeleteObject(pSelect->_hFont));
            pSelect->_hFont = NULL;
        }
    }
    
    pSelect->_hFont = CreateFontIndirect(&lf);
    pSelect->_lFontHeight = pccs->GetBaseCcs()->_yHeight;

    pSelect->_fRefreshFont = FALSE;

Cleanup:
    if ( pccs )
    {
        pccs->Release();
    }
    RRETURN1(hr, S_FALSE);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::DrawControl, protected
//
//  Synopsis:   Renders the SELECT into a DC which is different
//              from the inplace-active DC.
//
//  Note:       Used mainly to print.
//
//----------------------------------------------------------------------------

void
CSelectLayout::DrawControl(CFormDrawInfo * pDI, BOOL fPrinting)
{
    HDC                 hdc             = pDI->GetDC(TRUE);
    CRect               rc              = pDI->_rc;
    CRect               rcLine;
    CRect               rcScrollbar;
    CSize               sizeInner;
    long                xOffset;
    HFONT               hFontOld        = 0;
    const CCharFormat * pcf;
    const CParaFormat * pPF;
    int                 cColors;
    COLORREF            crText, crBack;
    COLORREF            crSelectedText  = 0x00FFFFFF, crSelectedBack=0;
    CCcs *              pccs            = 0;
    COptionElement *    pOption;
    long                lSelectedIndex  = 0;
    long                cVisibleLines;
    long                lTopIndex;
    long                lItemHeight;
    long                i;
    BOOL                fPrintBW;
    CColorValue         ccv;
    CStr                cstrTransformed;
    CStr *              pcstrDisplayText;
    long                cOptions;
    BOOL                fRTL;
    CSelectElement *    pSelect         = DYNCAST(CSelectElement, ElementOwner());
    UINT                taOld = 0;
    CBorderInfo         bi;

    if (ElementOwner()->GetBorderInfo(pDI, &bi, TRUE))
    {
        bi.aiWidths[BORDER_TOP]    = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP], FALSE);
        bi.aiWidths[BORDER_RIGHT]  = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT], FALSE);
        bi.aiWidths[BORDER_BOTTOM] = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM], FALSE);
        bi.aiWidths[BORDER_LEFT]   = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT], FALSE);

        ::DrawBorder(pDI, &rc, &bi);
    }

    //  Deflate the rect
    sizeInner = rc.Size();
    AdjustSizeForBorder(&sizeInner, pDI, FALSE);    //  FALSE means deflate

    //  BUGBUG(laszlog): Use BORDERINFO here ?

    rc.top += (rc.Height() - sizeInner.cy) / 2;
    rc.bottom = rc.top + sizeInner.cy;
    rc.left += (rc.Width() - sizeInner.cx) / 2;
    rc.right = rc.left + sizeInner.cx;

    xOffset = pDI->DocPixelsFromWindowX(2);

    //  set up font, text and background colors

    cColors = GetDeviceCaps(hdc, NUMCOLORS);

    if ( !Doc()->PaintBackground() && cColors <= 2)
    {
        fPrintBW = TRUE;
        crBack = 0x00FFFFFF;
        crText = pSelect->GetAAdisabled() ? GetSysColorQuick(COLOR_GRAYTEXT) : 0x0;
        crSelectedBack = crText;
        crSelectedText = crBack;
    }
    else
    {
        fPrintBW = FALSE;
        crText = 0x0;
        if ( GetFirstBranch()->GetCascadedbackgroundColor().IsDefined() )
        {
            crBack = GetFirstBranch()->GetCascadedbackgroundColor().GetColorRef();
        }
        else
        {
            crBack = GetSysColorQuick(COLOR_WINDOW);
        }
    }

    //  Fill the background with crBack

    PatBltBrush(hdc, &rc, PATCOPY, crBack);

    pcf = GetFirstBranch()->GetCharFormat();
    pPF = GetFirstBranch()->GetParaFormat();

    // find out what our reading order is.
    fRTL = pPF->HasRTL(TRUE);

    // ComplexText
    if(fRTL)
    {
        taOld = GetTextAlign(hdc);
        SetTextAlign(hdc, TA_RTLREADING | TA_RIGHT);
    }

    if ( pcf )
    {
        pccs = fc().GetCcs(hdc, pDI, pcf);

        if (!pccs)
            return;

        hFontOld = SelectFontEx(hdc, pccs->GetBaseCcs()->_hfont);

        //  BUGBUG(laszlog): Underline!
    }

    if ( ! pSelect->_fMultiple )
    {
        lSelectedIndex = pSelect->GetCurSel();
    }

    if ( pSelect->_fListbox )
    {
#ifndef WIN16
                // BUGWIN16: Win16 doesn't support Select_GetTopIndex,
                // so am turning this off. is this right ?? vreddy - 7/16/97
        if ( pSelect->_hwnd )
        {
            lTopIndex = pSelect->SendSelectMessage(CSelectElement::Select_GetTopIndex, 0, 0);
        }
        else
#endif
        {
            lTopIndex = pSelect->_lTopIndex;
        }

        cVisibleLines = pSelect->GetAAsize();
        cOptions = pSelect->_aryOptions.Size();

        //  Handle the default height fudge
        if ( cVisibleLines == 0 )
        {
            cVisibleLines = 4;
        }

        if ( pSelect->GetFirstBranch()->GetCascadedheight().IsNull() )
        {
            lItemHeight = rc.Height() / cVisibleLines;
        }
        else
        {
            lItemHeight = pSelect->_lFontHeight;
            cVisibleLines = (rc.Height() / lItemHeight) + 1;
        }


        if ( ! pSelect->_fMultiple )
        {
            lSelectedIndex = pSelect->GetCurSel();
        }


        if ( cVisibleLines < cOptions )
        {
            rcScrollbar = rc;
            // put the scrollbar rect on the appropriate side of the control based on
            // the direction
            if(!fRTL)
            {
                rcScrollbar.left = rcScrollbar.right - pDI->DocPixelsFromWindowX(g_sizeScrollbar.cx);
                rc.right = rcScrollbar.left;
            }
            else
            {
                rcScrollbar.right = rcScrollbar.left + pDI->DocPixelsFromWindowX(g_sizeScrollbar.cx);
                rc.left = rcScrollbar.right;
            }
        }


        cVisibleLines += lTopIndex;
        cVisibleLines = min(cOptions, cVisibleLines);
    }
    else
    {
        CUtilityButton cub;
        SIZEL sizel;

        rcScrollbar = rc;
        // put the scrollbar rect on the appropriate side of the control based on
        // the direction
        if(!fRTL)
        {
            rcScrollbar.left = rcScrollbar.right - pDI->DocPixelsFromWindowX(g_sizeScrollbar.cx);
            rc.right = rcScrollbar.left;
        }
        else
        {
            rcScrollbar.right = rcScrollbar.left + pDI->DocPixelsFromWindowX(g_sizeScrollbar.cx);
            rc.left = rcScrollbar.right;
        }

        pDI->DocumentFromWindow(&sizel, rcScrollbar.Size());

        //  Draw the drop button here
        cub.DrawButton(pDI, NULL,
                       BG_COMBO,
                       FALSE,
                       !pSelect->GetAAdisabled(),
                       FALSE,
                       rcScrollbar,
                       sizel,//extent
                       0);

        if ( lSelectedIndex == -1 )
            goto Cleanup;   //  Combobox is empty

        lTopIndex = lSelectedIndex;
        cVisibleLines = lTopIndex + 1;
        lItemHeight = rc.Height();
    }

    rcLine = rc;

    for ( i = lTopIndex, rcLine.bottom = rc.top + lItemHeight;
          i < cVisibleLines;
          i++, rcLine.OffsetRect(0, lItemHeight), rcLine.bottom = min(rc.bottom, rcLine.bottom) )
    {
        int iRet = -1;

        Assert(i < pSelect->_aryOptions.Size());

        pOption = pSelect->_aryOptions[i];

        pcstrDisplayText = pOption->GetDisplayText(&cstrTransformed);


        if ( fPrintBW )
        {
            SetBkColor  (hdc, pSelect->_fListbox && pOption->_fSELECTED ? crSelectedBack : crBack);
            SetTextColor(hdc, pSelect->_fListbox && pOption->_fSELECTED ? crSelectedText : crText);
            PatBltBrush(hdc, &rcLine, PATCOPY, pSelect->_fListbox && pOption->_fSELECTED ? crSelectedBack : crBack);
        }
        else
        {
            pOption->GetDisplayColors(&crText, &crBack, pSelect->_fListbox);

            SetBkColor(hdc, crBack);
            SetTextColor(hdc, crText);
            PatBltBrush(hdc, &rcLine, PATCOPY, crBack);
        }

        //  Set up clipping
#ifdef WIN16
        GDIRECT gr, *prc;
        CopyRect(&gr, &rc);
        prc = &gr;
#else
        RECT *prc = &rc;
#endif

        if ( pcf )
        {
            if ( pOption->CheckFontLinking(hdc, pccs) )
            {
                // this option requires font linking

                iRet = FontLinkTextOut(hdc,
                                       !fRTL ? rcLine.left + xOffset : rcLine.right - xOffset,
                                       rcLine.top,
                                       ETO_CLIPPED,
                                       prc,
                                       *pcstrDisplayText,
                                       pcstrDisplayText->Length(),
                                       pDI,
                                       pcf,
                                       FLTO_TEXTOUTONLY);
            }
        }
        if (iRet < 0)
        {
            // no font linking

            VanillaTextOut(pccs,
                           hdc,
                           !fRTL ? rcLine.left + xOffset : rcLine.right - xOffset,
                           rcLine.top,
                           ETO_CLIPPED,
                           prc,
                           *pcstrDisplayText,
                           pcstrDisplayText->Length(),
                           Doc()->GetCodePage(),
                           NULL);

            if (pSelect->_hFont && g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
            {
                // Workaround for win95 gdi ExtTextOutW underline bugs.
                DrawUnderlineStrikeOut(rcLine.left + xOffset,
                                       rcLine.top,
                                       pOption->MeasureLine(NULL),
                                       hdc,
                                       pSelect->_hFont,
                                       prc);
            }
        }
    }



Cleanup:
    if (hFontOld)
        SelectFont(hdc, hFontOld);
    if(fRTL)
        SetTextAlign(hdc, taOld);

    if ( pccs )
    {
        pccs->Release();
    }

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectElement::Draw
//
//  Synopsis:   Draw the site and its children to the screen.
//
//----------------------------------------------------------------------------

void
CSelectLayout::Draw(CFormDrawInfo *pDI, CDispNode *)
{
    if (pDI->_fInplacePaint)
    {
        Assert(pDI->_dwDrawAspect == DVASPECT_CONTENT);
        //DrawBorder(pDI);

        goto Cleanup;
    }

    //  Use ExtTextOut to draw the listbox line by line

    if ( pDI->_dwDrawAspect == DVASPECT_CONTENT ||
         pDI->_dwDrawAspect == DVASPECT_DOCPRINT )
    {
        DrawControl(pDI, pDI->_dwDrawAspect == DVASPECT_DOCPRINT);
    }

Cleanup:
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectLayout::OnFormatsChange
//
//  Synopsis:   Handle formats change notification
//
//----------------------------------------------------------------------------
HRESULT
CSelectLayout::OnFormatsChange(DWORD dwFlags)
{
    CSelectElement * pElementSelect = DYNCAST(CSelectElement, ElementOwner());

    if ( dwFlags & ELEMCHNG_CLEARCACHES )
    {
        pElementSelect->_fRefreshFont = TRUE;
    }

    HRESULT          hr = THR(super::OnFormatsChange(dwFlags));

    if(hr)
        goto Cleanup;

    pElementSelect->ChangeBackgroundBrush();
    pElementSelect->_hbrushHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);

Cleanup:
    RRETURN(hr);
}


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
CSelectLayout::HandleViewChange(
    DWORD           flags,
    const RECT*     prcClient,
    const RECT*     prcClip,
    CDispNode*      pDispNode)
{
    CSelectElement * pSelect = DYNCAST(CSelectElement, ElementOwner());
    
    BOOL fInView = !!(flags & VCF_INVIEW);
    BOOL fInViewChanged = !!(flags & VCF_INVIEWCHANGED);
    BOOL fMoved = !!(flags & VCF_POSITIONCHANGED);
    BOOL fNoRedraw = !!(flags & VCF_NOREDRAW);

    pSelect->_fWindowVisible = fInView;
    
    // BUGBUG (donmarsh) - For now, CSelectElement creates its HWND
    // during CalcSize.  However, it would be a significant perf win
    // to create it here when it first becomes visible.  KrisMa will fix.
    Assert((pSelect->Doc() && pSelect->Doc()->State() < OS_INPLACE) ||
        pSelect->_hwnd || (pSelect->Doc() && pSelect->Doc()->IsPrintDoc()));
    
    if (fInView)
    {
        if (pSelect->_fSetComboVert)
        {
            pSelect->_fSetComboVert = FALSE;

            UINT uFlags = SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOREDRAW;
      
            // this call just sets the window size
            SetWindowPos(pSelect->_hwnd,
                NULL,
                prcClient->left,
                prcClient->top,
                prcClient->right - prcClient->left,
                pSelect->_fListbox ? prcClient->bottom - prcClient->top :
                    pSelect->_lComboHeight,
                uFlags);

            // make sure we show the window
            fInViewChanged = TRUE;
        }
    }
    
    DWORD positionChangeFlags = SWP_NOACTIVATE | SWP_NOZORDER;
    if (fInViewChanged)
        positionChangeFlags |= (fInView) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
    if (fNoRedraw)
        positionChangeFlags |= SWP_NOREDRAW;
    
    BOOL fDoWindowPos = fMoved || fInViewChanged;

    // take care of changing clip
    if (fInView)
    {
        if ((CRect&)(*prcClip) == *prcClient)
        {
            // we want to remove the clip region only if this window had one
            // in order to reduce flash caused by extra WM_ERASEBKGND messages
            HRGN hrgnClip = ::CreateRectRgn(0,0,0,0);
            if (hrgnClip == NULL || ::GetWindowRgn(pSelect->_hwnd, hrgnClip) != ERROR)
            {
                // If fNoRedraw is false, we're not scrolling so we
                // need to set the window size. If we are scrolling,
                // setting the size causes a WM_NCPAINT if a height is
                // set. (See bug 67670.) We don't want this. (That's why
                // it's called a bug.)
                if (fNoRedraw)
                    positionChangeFlags |= SWP_NOSIZE;

                // do any pending requests before calling SetWindowPos (bug 71091)
                GetView()->FlushDeferSetWindowPos();
                ::SetWindowRgn(pSelect->_hwnd, NULL, !fNoRedraw);
                ::SetWindowPos(pSelect->_hwnd,
                    NULL,
                    prcClient->left, prcClient->top, prcClient->right - prcClient->left, prcClient->bottom - prcClient->top,
                    positionChangeFlags);
                fDoWindowPos = FALSE;
            }
            ::DeleteObject(hrgnClip);
        }
        else
        {
            // If fNoRedraw is false, we're not scrolling so we
            // need to set the window size. If we are scrolling,
            // setting the size causes a WM_NCPAINT if a height is
            // set. (See bug 67670.) We don't want this. (That's why
            // it's called a bug.)
            if (fNoRedraw)
                positionChangeFlags |= SWP_NOSIZE;
            CRect rcClip(*prcClip);
            rcClip.OffsetRect(-((const CRect*)prcClient)->TopLeft().AsSize());

            // do any pending requests before calling SetWindowPos (bug 71091)
            GetView()->FlushDeferSetWindowPos();
            ::SetWindowRgn(pSelect->_hwnd, ::CreateRectRgnIndirect(&rcClip), !fNoRedraw);
            ::SetWindowPos(pSelect->_hwnd,
                NULL,
                prcClient->left, prcClient->top, prcClient->right - prcClient->left, prcClient->bottom - prcClient->top,
                positionChangeFlags);
            fDoWindowPos = FALSE;
        }
    }

    if (fDoWindowPos)
    {
        // we have to redraw a child window that was clipped by a parent's
        // border, but is now unclipped
        positionChangeFlags |= SWP_NOCOPYBITS;
        DeferSetWindowPos(pSelect->_hwnd, prcClient, positionChangeFlags, NULL);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSelectLayout::ProcessDisplayTreeTraversal
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
CSelectLayout::ProcessDisplayTreeTraversal(void *pClientData)
{
    CSelectElement * pSelect = DYNCAST(CSelectElement, ElementOwner());
    if (pSelect->_hwnd)
    {
        CView::CWindowOrderInfo* pWindowOrderInfo =
            (CView::CWindowOrderInfo*) pClientData;
        pWindowOrderInfo->AddWindow(pSelect->_hwnd);
    }
    
    return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Member : GetChildElementTopLeft
//
//  Synopsis : CLayout virtual override, the job of this function is to
//      translate Queries for the top left of an option (the only element
//      that would be a child of the select) into the top left, of the select
//      itself.
//
//----------------------------------------------------------------------------

HRESULT
CSelectLayout::GetChildElementTopLeft(POINT & pt, CElement * pChild)
{
    // Only OPTIONs can be children of SELECT
    Assert( pChild && pChild->Tag() == ETAG_OPTION );
    
    // an option's top left is reported as 0,0 (Bug #41111).
    pt.x = pt.y = 0;
    return S_OK;
}

