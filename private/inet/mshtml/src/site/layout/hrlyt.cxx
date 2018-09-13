//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       hrlyt.cxx
//
//  Contents:   Implementation of CHRLayout
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

#ifndef X_HRLYT_HXX_
#define X_HRLYT_HXX_
#include "hrlyt.hxx"
#endif

#ifndef X_EHR_HXX_
#define X_EHR_HXX_
#include "ehr.hxx"
#endif

MtDefine(CHRLayout, Layout, "CHRLayout")

const CLayout::LAYOUTDESC CHRLayout::s_layoutdesc =
{
    0, // _dwFlags
};

//+-------------------------------------------------------------------------
//
//  Method:     CHRLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------
#define MAX_HR_SIZE         100L // 100 pixels
#define MIN_HR_SIZE         1L   // 1 pixel
#define DEFAULT_HR_SIZE     2L
#define MIN_LEGAL_WIDTH     1L
#define MAX_HR_WIDTH        32000L

DWORD
CHRLayout::CalcSize( CCalcInfo * pci,
                     SIZE      * psize,
                     SIZE      * psizeDefault)
{
    Assert(ElementOwner());

    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    CSize           sizeOriginal;
    DWORD           grfReturn;

    Assert(pci);
    Assert(psize);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    _fSizeThis = (_fSizeThis || (pci->_grfLayout & LAYOUT_FORCE));

    // Handle sizing and min/max requests here
    if (   (    pci->_smMode != SIZEMODE_SET
            &&  _fSizeThis)
        || pci->_smMode == SIZEMODE_MMWIDTH
        || pci->_smMode == SIZEMODE_MINWIDTH
       )
    {
        CUnitValue  uv;

        // First, calculate the correct width
        // (Treat missing widths as if this object is percentage-sized. While not precisely
        //  correct, since such HRs simply take on the size of their parent, it does ensure
        //  the parent container will recalc the HR when their own size changes.)
        uv = GetFirstBranch()->GetCascadedwidth();

        switch (pci->_smMode)
        {
        case SIZEMODE_MMWIDTH:
            psize->cx =
            psize->cy = (uv.IsNullOrEnum() || PercentWidth()
                                ? MIN_LEGAL_WIDTH
                                : min(uv.XGetPixelValue(pci, 0, GetFirstBranch()->GetFontHeightInTwips(&uv)),
                                      (LONG)USHRT_MAX));
            break;

        case SIZEMODE_NATURAL:
            {
                LONG    cxParent;

                // Always use the available space.  We used to use the parent's
                // size in case of a percent width.  The old code looked as follows:
                // cxParent = max(1L, (_fWidthPercent ? pci->_sizeParent.cx : psize->cx));
                // This was changed because of compatibility issues explained in bug 22948.
                // In either case, "pin" the value to greater than or equal to 1
                cxParent = max(1L, psize->cx);

                // If the user did not supply a value or it is wider than the parent, use parent's width
                if (uv.IsNull())
                {
                    psize->cx = cxParent;
                }

                // Otherwise, take value from the user settings
                else
                {
                    LONG    cx = uv.XGetPixelValue(pci, cxParent, GetFirstBranch()->GetFontHeightInTwips(&uv));

                    // If less than zero, then "pin" to zero
                    // If greater than zero, use what the user specified
                    // If equal to zero, use zero if width is a percentage
                    //                   otherwise, use parent's width

                    if(cx < 0)
                        psize->cx = 0;
                    else if(cx > 0)
                        psize->cx = min(cx, MAX_HR_WIDTH);
                    else
                        psize->cx = 1;

                    // Finally, ensure the size does not exceed the maximum
                    psize->cx = max((LONG)MIN_LEGAL_WIDTH, psize->cx);
                }
            }
            break;

        case SIZEMODE_MINWIDTH:
            psize->cx = (uv.IsNullOrEnum() || PercentWidth()
                            ? MIN_LEGAL_WIDTH
                            : uv.XGetPixelValue(pci, 0, GetFirstBranch()->GetFontHeightInTwips(&uv)));
            break;

#if DBG==1
        default:
            Assert(FALSE);
            break;
#endif
        }

        // Then, for all but min/max modes, determine the correct height
        if (pci->_smMode != SIZEMODE_MMWIDTH)
        {
            uv = GetFirstBranch()->GetCascadedheight();

            // Determine the user specified height (if any) and the default height
            // (HR height can only be in pixels, so no need to pass the parent size)
            LONG    cyDefault = pci->DocPixelsFromWindowY(DEFAULT_HR_SIZE);
            LONG    cy        = uv.YGetPixelValue(pci, 0, GetFirstBranch()->GetFontHeightInTwips(&uv));

            // If less than the default, use the default size
            // Otherwise, "pin" to the maximum
            // NOTE: The calculated default can be less than DEFAULT_HR_SIZE
            //       when zooming is in effect. We must still "pin" to
            //       DEFAULT_HR_SIZE as the minimum height.
            // NOTE: Height is "pin'd" to a maximum here, rather than in the PDL,
            //       so we can accept sizes greater than DEFAULT_HR_SIZE

            if(uv.IsNull())
                psize->cy = max(cyDefault, DEFAULT_HR_SIZE);
            else if(cy < MIN_HR_SIZE)
                psize->cy = MIN_HR_SIZE;
            else if(cy > MAX_HR_SIZE)
                psize->cy = min(max(cy, DEFAULT_HR_SIZE), MAX_HR_SIZE);
            else
                psize->cy = cy;

            // Finally, set _sizeProposed (if appropriate)
            if (    pci->_smMode == SIZEMODE_NATURAL
                ||  pci->_smMode == SIZEMODE_SET
                ||  pci->_smMode == SIZEMODE_FULLSIZE)
            {
                //
                // If dirty, ensure display tree nodes exist
                //

                if (    _fSizeThis
                    &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
                {
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
            }
        }
    }

    // Otherwise, defer to default handling
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

    return grfReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object.
//
//----------------------------------------------------------------------------

void
CHRLayout::Draw (CFormDrawInfo * pDI, CDispNode *)
{
    DrawRule(pDI,
             pDI->_rc,
             DYNCAST(CHRElement, ElementOwner())->GetAAnoShade(),
             GetFirstBranch()->GetCascadedcolor(),
             ElementOwner()->GetBackgroundColor());
}

//+---------------------------------------------------------------------------
//
//  Function:     DrawRule
//
//  Synopsis:   Draw the rule with specified parameters
//
//----------------------------------------------------------------------------

static HRESULT
DrawRule(CFormDrawInfo * pDI, const RECT &rc, BOOL fNoShade, const CColorValue &cvCOLOR, COLORREF colorBack)
{
    HGDIOBJ     hBrush = NULL;
    HDC         hdc = pDI->GetDC();
    HPEN        hOrigPen;

    Assert(!IsRectEmpty(&rc));

    // When a color value is specified the 3d effect must be off
    if(!fNoShade && cvCOLOR.IsDefined())
    {
        fNoShade = TRUE;
    }

    if(fNoShade)
    {
        HBRUSH      hOrigBrush;

        if(cvCOLOR.IsDefined())
        {
            Verify(hBrush = CreateSolidBrush(cvCOLOR.GetColorRef()));
        }
        else
        {
            Verify(hBrush = CreateSolidBrush(GetSysColorQuick(COLOR_3DSHADOW)));
        }

        hOrigBrush = (HBRUSH)SelectObject(hdc, hBrush);
        hOrigPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
        SelectObject(hdc, hOrigBrush);
        SelectObject(hdc, hOrigPen);
        DeleteObject(hBrush);
    }
    else
    {
        // Draw the ruler with 3d effect
        COLORREF    lightColor;
        COLORREF    darkColor;
        HPEN        hDarkPen;
        HPEN        hLightPen;

        if (!pDI->_pDoc->IsPrintDoc())
        {
            COLORREF colorBtnFace = GetSysColorQuick(COLOR_BTNFACE);

            darkColor = GetSysColorQuick(COLOR_3DSHADOW);
            // for IE3/Nav3 compatibility
            // If the background color is the same as the border
            // color, Nav3 choose a different color than ButtonFace
            // This is the case in WC3 test page.
            // we do the same thing here
            if ((colorBack & CColorValue::MASK_COLOR) == colorBtnFace)
            {
                lightColor = RGB(230, 230, 230);
            }
            else
            {
                lightColor = colorBtnFace;
            }
        }
        else
        {
            // When printing
            darkColor  = RGB(128, 128, 128);
            lightColor = RGB(0, 0, 0);
        }

        Verify(hDarkPen = CreatePen(PS_SOLID, 0, darkColor));
        hOrigPen = (HPEN)SelectObject(hdc, hDarkPen);
        MoveToEx(hdc, rc.right - 1, rc.top, (POINT *)NULL);
        LineTo(hdc, rc.left, rc.top);
        if(rc.bottom > rc.top )
        {
            LineTo(hdc, rc.left, rc.bottom -1);
        }

        Verify(hLightPen = CreatePen(PS_SOLID, 0, lightColor));

        SelectObject(hdc, hLightPen);
        DeleteObject(hDarkPen);
        if(rc.bottom - 1 > rc.top)
        {
            MoveToEx(hdc, rc.left, rc.bottom -1, (POINT *)NULL);
            LineTo(hdc, rc.right - 1, rc.bottom -1);
            if(rc.bottom > rc.top)
            {
                LineTo(hdc, rc.right - 1, rc.top);
            }
        }

        // Restore the original pen
        SelectObject(hdc, hOrigPen);

        DeleteObject(hLightPen);
    }


    return S_OK;
}
