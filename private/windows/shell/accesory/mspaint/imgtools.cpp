
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "t_text.h"
#include "toolbox.h"
#include "imgcolor.h"
#include "undo.h"
#include "props.h"
#include "colorsrc.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CImgTool, CObject)
IMPLEMENT_DYNAMIC(CRubberTool, CImgTool)
IMPLEMENT_DYNAMIC(CClosedFormTool, CRubberTool)
IMPLEMENT_DYNAMIC(CFreehandTool, CImgTool)
IMPLEMENT_DYNAMIC(CSketchTool, CFreehandTool)
IMPLEMENT_DYNAMIC(CBrushTool, CFreehandTool)
IMPLEMENT_DYNAMIC(CPencilTool, CFreehandTool)
IMPLEMENT_DYNAMIC(CEraserTool, CFreehandTool)
IMPLEMENT_DYNAMIC(CAirBrushTool, CFreehandTool)
IMPLEMENT_DYNAMIC(CLineTool, CRubberTool)
IMPLEMENT_DYNAMIC(CRectTool, CClosedFormTool)
IMPLEMENT_DYNAMIC(CRoundRectTool, CClosedFormTool)
IMPLEMENT_DYNAMIC(CEllipseTool, CClosedFormTool)
IMPLEMENT_DYNAMIC(CPickColorTool, CImgTool)
IMPLEMENT_DYNAMIC(CFloodTool, CImgTool)
IMPLEMENT_DYNAMIC(CSelectTool, CImgTool)
IMPLEMENT_DYNAMIC(CZoomTool, CImgTool)

#include "memtrace.h"

extern CRect  rcDragBrush;

extern HDC  hRubberDC;


BOOL  g_bBrushVisible;
BOOL  g_bPickingColor;
UINT  g_nStrokeWidth = 1;

/***************************************************************************/
//
// Drawing Tool Classes
//

CRectTool             g_rectTool;
CRoundRectTool        g_roundRectTool;
CEllipseTool          g_ellipseTool;
CLineTool             g_lineTool;
CSelectTool           g_selectTool;
CBrushTool            g_brushTool;
CSketchTool           g_sketchTool;
CPencilTool           g_pencilTool;
CEraserTool           g_eraserTool;
CAirBrushTool         g_airBrushTool;
CFloodTool            g_floodTool;
CPickColorTool        g_pickColorTool;
CZoomTool             g_zoomTool;

/***************************************************************************/

CImgTool*  CImgTool::c_pHeadImgTool     = NULL;
CImgTool*  CImgTool::c_pCurrentImgTool  = &g_pencilTool;
CImgTool*  CImgTool::c_pPreviousImgTool = &g_pencilTool;
BOOL       CImgTool::c_bDragging        = FALSE;
int        CImgTool::c_nHideCount       = 0;

/***************************************************************************/

CImgTool::CImgTool()
    {
    m_bUsesBrush          = FALSE;
    m_bIsUndoable         = TRUE;
    m_bCanBePrevTool      = TRUE;
    m_bToggleWithPrev     = FALSE;
    m_bFilled             = FALSE;
    m_bBorder             = TRUE;
    m_bMultPtOpInProgress = FALSE;
    m_eDrawDirection      = eFREEHAND;

    m_nStrokeWidth = 0;
    m_nStrokeShape = roundBrush;

    m_nCursorID = LOWORD(IDC_CROSSHAIR);
    m_nCmdID    = NULL;

    // Link into the list of tools...
    m_pNextImgTool = c_pHeadImgTool;
    c_pHeadImgTool = this;
    }

/******************************************************************************/

eDRAWCONSTRAINTDIRECTION CImgTool::DetermineDrawDirection(MTI *pmti)
    {
    eDRAWCONSTRAINTDIRECTION eDrawDirection;

    // 45 is dominant, test first
    if ( (pmti->pt.x > pmti->ptPrev.x) &&
         (pmti->pt.y > pmti->ptPrev.y) )
        {
            eDrawDirection = eSOUTH_EAST;
        }
    else
        {
        if ( (pmti->pt.x > pmti->ptPrev.x) &&
             (pmti->pt.y < pmti->ptPrev.y) )
            {
                eDrawDirection = eNORTH_EAST;
            }
        else
            {
            if ( (pmti->pt.x < pmti->ptPrev.x) &&
                 (pmti->pt.y > pmti->ptPrev.y) )
                {
                    eDrawDirection = eSOUTH_WEST;
                }
            else
                {
                if ( (pmti->pt.x < pmti->ptPrev.x) &&
                     (pmti->pt.y < pmti->ptPrev.y) )
                    {
                        eDrawDirection = eNORTH_WEST;
                    }
                else
                    {
                    // Horizontal is the next dominant, test before vertical
                    if (pmti->ptPrev.x != pmti->pt.x)
                        {
                        eDrawDirection = eEAST_WEST;
                        pmti->pt.y = pmti->ptPrev.y;
                        }
                    else
                        {
                        if (pmti->ptPrev.y != pmti->pt.y)
                            {
                            eDrawDirection = eNORTH_SOUTH;
                            pmti->pt.x = pmti->ptPrev.x;
                            }
                        }
                    }
                }
            }
        }
    return eDrawDirection;
    }

/******************************************************************************/

void CImgTool::AdjustPointsForConstraint(MTI *pmti)
    {
    }

/******************************************************************************/

void CImgTool::PreProcessPoints(MTI *pmti)
    {
    if (pmti != NULL)
        {
        if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) //still in constrain mode
            {
            switch (m_eDrawDirection)
                {
                case eEAST_WEST:
                case eNORTH_SOUTH:
                case eNORTH_WEST:
                case eSOUTH_EAST:
                case eNORTH_EAST:
                case eSOUTH_WEST:
                     AdjustPointsForConstraint(pmti);
                     break;
                default: // not in constraint mode yet If shift down, check for
                         // mode and save mode else nothing.  Default is freehand
                     m_eDrawDirection = DetermineDrawDirection(pmti);
                     AdjustPointsForConstraint(pmti);
                    break;
                }
            }
        else
            {
            // shift not down
            m_eDrawDirection = eFREEHAND;
            }
        }
    }

/***************************************************************************/

void CImgTool::HideDragger(CImgWnd* pImgWnd)
    {
    ASSERT(c_pCurrentImgTool != NULL);

    if (c_nHideCount == 0)
        c_pCurrentImgTool->OnShowDragger(pImgWnd, FALSE);
    c_nHideCount++;
    }

/***************************************************************************/

void CImgTool::ShowDragger(CImgWnd* pImgWnd)
    {
    ASSERT(c_pCurrentImgTool != NULL);

    if (--c_nHideCount == 0)
        c_pCurrentImgTool->OnShowDragger(pImgWnd, TRUE);
    }

/***************************************************************************/

void CImgTool::Select(UINT nCmdID)
    {
    CImgTool* p = FromID(nCmdID);
    if (p)
        {
        p->Select();
        }
    }

/***************************************************************************/

void CImgTool::Select()
    {
    ASSERT(this != NULL);

    if (this == c_pCurrentImgTool && m_bToggleWithPrev)
        {
        SelectPrevious();
        return;
        }

    if (g_bCustomBrush)
        {
        g_bCustomBrush = FALSE;
        SetCombineMode(combineColor);
        }

    HideBrush();

    if (c_pCurrentImgTool->m_bCanBePrevTool && c_pCurrentImgTool != this)
        c_pPreviousImgTool = c_pCurrentImgTool;

    // Make sure to Deactivate the old one BEFORE activating the new one, so
    // globals (like g_nStrokeWidth) get set correctly
    if (c_pCurrentImgTool != NULL)
        c_pCurrentImgTool->OnActivate(FALSE);

    c_pCurrentImgTool = this;

    OnActivate(TRUE);

    if (c_pCurrentImgTool != this)
        {
        // Some tools may give up activation...
        ASSERT(!m_bCanBePrevTool);
        return;
        }

    SetCombineMode(combineColor);

    if (g_pImgToolWnd)
    {
        g_pImgToolWnd->SelectTool( (WORD)m_nCmdID );

        if (g_pImgToolWnd->m_hWnd)
            g_pImgToolWnd->InvalidateOptions();
    }

    CImgWnd::SetToolCursor();
    }

/***************************************************************************/

CImgTool* CImgTool::FromID(UINT nCmdID)
    {
    CImgTool* pImgTool = c_pHeadImgTool;
    while (pImgTool != NULL && pImgTool->m_nCmdID != nCmdID)
        pImgTool = pImgTool->m_pNextImgTool;
    return pImgTool;
    }

/***************************************************************************/

void CImgTool::SetStrokeWidth(UINT nNewStrokeWidth)
    {
    if (nNewStrokeWidth == m_nStrokeWidth)
        return;

    HideBrush();
    g_bCustomBrush = FALSE;
    m_nStrokeWidth = nNewStrokeWidth;
    g_pImgToolWnd->InvalidateOptions();

    extern MTI  mti;

    if (mti.fLeft || mti.fRight)
        OnDrag(CImgWnd::GetCurrent(), &mti);
    }

/***************************************************************************/

void CImgTool::SetStrokeShape(UINT nNewStrokeShape)
    {
    if (m_nStrokeShape == nNewStrokeShape)
        return;

    HideBrush();
    g_bCustomBrush = FALSE;
    m_nStrokeShape = nNewStrokeShape;
    g_pImgToolWnd->InvalidateOptions(FALSE);
    }

/***************************************************************************/

void CImgTool::OnActivate(BOOL bActivate)
    {
    if (bActivate)
        OnShowDragger(CImgWnd::GetCurrent(), TRUE);
    }

/***************************************************************************/

void CImgTool::OnEnter(CImgWnd* pImgWnd, MTI* pmti)
    {
    // No default action
    }

/***************************************************************************/

void CImgTool::OnLeave(CImgWnd* pImgWnd, MTI* pmti)
    {
    // No default action
    }

/***************************************************************************/

void CImgTool::OnShowDragger(CImgWnd* pImgWnd, BOOL bShowDragger)
    {
    // No default action
    }

/***************************************************************************/

void CImgTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    c_bDragging = TRUE;
    }

/***************************************************************************/

void CImgTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    c_bDragging = FALSE;

    if (m_bIsUndoable)
        DirtyImg(pImgWnd->m_pImg);
    }

/***************************************************************************/

void CImgTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    ASSERT(c_bDragging);
    }

/***************************************************************************/

void CImgTool::OnMove(CImgWnd* pImgWnd, MTI* pmti)
    {
//   ASSERT(!c_bDragging);

    if (UsesBrush())
        {
        fDraggingBrush = TRUE;
        pImgWnd->ShowBrush(pmti->pt);
        }

    SetStatusBarPosition(pmti->pt);
    }

/***************************************************************************/

void CImgTool::OnTimer(CImgWnd* pImgWnd, MTI* pmti)
    {
    // Tools should not have started a timer unless it overrides this!
    ASSERT(FALSE);
    }

/***************************************************************************/

void CImgTool::OnCancel(CImgWnd* pImgWnd)
    {
    c_bDragging = FALSE;
    }

/***************************************************************************/

void CImgTool::OnPaintOptions(CDC* pDC, const CRect& paintRect,
                                        const CRect& optionsRect)
    {
    }

/***************************************************************************/

void CImgTool::PaintStdPattern(CDC* pDC, const CRect& paintRect,
                                         const CRect& optionsRect)
    {
    CBrush brush;
    CPalette *pcPaletteOld = NULL;
    CPalette *pcPaletteOld2 = NULL;

    CDC dc;
    if (!dc.CreateCompatibleDC(pDC))
        return;

    CBitmap bitmap, * pOldBitmap;
    if (!bitmap.CreateCompatibleBitmap(pDC, 8, 8))
        return;

    pOldBitmap = dc.SelectObject(&bitmap);

    if (theApp.m_pPalette)
        {
        pcPaletteOld = pDC->SelectPalette( theApp.m_pPalette, FALSE );
        pDC->RealizePalette();

        pcPaletteOld2 = dc.SelectPalette( theApp.m_pPalette, FALSE );
        dc.RealizePalette();
        }

    CBrush* pOldBrush = NULL;

    COLORREF rgb = crLeft;

    if (pImgCur->m_pBitmapObj->m_nColors == 0)
        {
        BOOL MonoRect(CDC* pDC, const CRect& rect, COLORREF rgb, BOOL bFrame);
        MonoRect(&dc, CRect(0, 0, 9, 9), rgb, FALSE);
        }
    else
        {
        brush.CreateSolidBrush(rgb);
        pOldBrush = dc.SelectObject(&brush);
        dc.PatBlt(0, 0, 8, 8, PATCOPY);
        dc.SelectObject(pOldBrush);
        brush.DeleteObject();
        }


    // Draw a black grid...
    for (int i = 0; i < 9; i++)
        {
        pDC->PatBlt(optionsRect.left + 2 + i * 7, optionsRect.top + 3,
            1, 8 * 7 + 1, BLACKNESS);
        pDC->PatBlt(optionsRect.left + 2, optionsRect.top + 3 + i * 7,
            8 * 7 + 1, 1, BLACKNESS);
        }


    // Fill in the boxes...
    COLORREF curColor = (COLORREF)0xffffffff;

    for (int y = 0; y < 8; y++)
        {
        for (int x = 0; x < 8; x++)
            {
            COLORREF color = dc.GetPixel(x, y) | 0x02000000L;

            if (color != curColor)
                {
                if (pOldBrush != NULL)
                    pDC->SelectObject(pOldBrush);

                brush.DeleteObject();
                brush.CreateSolidBrush(color);

                pOldBrush = pDC->SelectObject(&brush);
                curColor = color;
                }

            pDC->PatBlt(optionsRect.left + 2 + 1 + x * 7,
                        optionsRect.top  + 3 + 1 + y * 7, 6, 6, PATCOPY);
            }
        }

    ASSERT(pOldBrush != NULL);
    pDC->SelectObject(pOldBrush);

    dc.SelectObject(pOldBitmap);

    if (pcPaletteOld)
        pDC->SelectPalette(pcPaletteOld, FALSE);

    if (pcPaletteOld2)
        dc.SelectPalette(pcPaletteOld2, FALSE);
    }

/***************************************************************************/

void CImgTool::ClickStdPattern(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    CImgTool::OnClickOptions(pWnd, optionsRect, clickPoint);
    }

/***************************************************************************/

void CImgTool::PaintStdBrushes(CDC* pDC, const CRect& paintRect,
                                         const CRect& optionsRect)
    {
    int cxBrush = optionsRect.Width() / 3;
    int cyBrush = optionsRect.Height() / 4;

    for (UINT nBrushShape = 0; nBrushShape < 4; nBrushShape++)
        {
        int x = 0;
        for (UINT nStrokeWidth = 8 - (nBrushShape == 0);
            (int)nStrokeWidth > 0; nStrokeWidth -= 3, x += cxBrush)
            {
            CRect rect;
            rect.left = optionsRect.left + x;
            rect.top = optionsRect.top + cyBrush * nBrushShape;
            rect.right = rect.left + cxBrush;
            rect.bottom = rect.top + cyBrush;
            rect.InflateRect(-3, -3);

            if ((paintRect & rect).IsRectEmpty())
                continue;

            BOOL bCur = (nStrokeWidth == m_nStrokeWidth
                       && nBrushShape == m_nStrokeShape);

            CBrush* pOldBrush = pDC->SelectObject(GetSysBrush(bCur ?
                                        COLOR_HIGHLIGHT : COLOR_BTNFACE));
            if ((nStrokeWidth & 1) != 0)
                {
                // Adjust hilight rect so brush will be centered
                rect.right -= 1;
                rect.bottom -= 1;
                }
            pDC->PatBlt(rect.left + 1, rect.top - 1,
                rect.Width() - 2, rect.Height() + 2, PATCOPY);
            pDC->SelectObject(pOldBrush);

            CPoint pt(optionsRect.left + (cxBrush - nStrokeWidth) / 2 + x,
                      optionsRect.top +
                      (cyBrush - nStrokeWidth) / 2 + nBrushShape * cyBrush);

            pOldBrush = pDC->SelectObject(GetSysBrush(bCur ?
                                      COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
            BrushLine(pDC, pt, pt, nStrokeWidth, nBrushShape);
            pDC->SelectObject(pOldBrush);
            }
        }
    }

/***************************************************************************/

void CImgTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    MessageBeep(0);
    }

/******************************************************************************/

void CImgTool::OnUpdateColors (CImgWnd* pImgWnd)
    {
    }

/******************************************************************************/

BOOL CImgTool::CanEndMultiptOperation(MTI* pmti )
    {
    return (! m_bMultPtOpInProgress);  // if not in progress (FALSE) => can end (TRUE)
    }

/******************************************************************************/

void CImgTool::EndMultiptOperation(BOOL bAbort)
    {
    m_bMultPtOpInProgress = FALSE;
    }

/******************************************************************************/

BOOL CImgTool::IsToolModal(void)
{
        return(IsDragging() || m_bMultPtOpInProgress || m_bToggleWithPrev);
}

/******************************************************************************/

BOOL CImgTool::IsUndoable()
    {
    if (m_bMultPtOpInProgress)
        {
        return FALSE;  // cannot undo in the middle of a multi-point operation.
        }
    else
        {
        return m_bIsUndoable;
        }
    }

/******************************************************************************/

void CImgTool::ClickStdBrushes(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    HideBrush();

    g_bCustomBrush = FALSE;
    m_nStrokeWidth = 2 + 3 * (2 - (clickPoint.x / (optionsRect.Width() / 3)));
    m_nStrokeShape = clickPoint.y / (optionsRect.Height() / 4);

    if (m_nStrokeShape == 0)
        m_nStrokeWidth -= 1;

    pWnd->InvalidateOptions(FALSE);
    }

/******************************************************************************/

UINT CImgTool::GetCursorID()
    {
    return m_nCursorID;
    }

/******************************************************************************/

CRect  CRubberTool::rcPrev;
// UINT       CRubberTool::m_nStrokeWidth;

CRubberTool::CRubberTool()
    {
    }

/******************************************************************************/

void CRubberTool::OnPaintOptions(CDC* pDC, const CRect& paintRect,
                                           const CRect& optionsRect)
    {
    if (m_bFilled)
        {
        PaintStdPattern(pDC, paintRect, optionsRect);
        return;
        }

    #define nLineWidths 5

    int cyEach = (optionsRect.Height() - 4) / nLineWidths;

    for (int i = 0; i < nLineWidths; i++)
        {
        UINT cyHeight = i + 1;

        CBrush* pOldBrush;
        BOOL bCur = (cyHeight == GetStrokeWidth());

        pOldBrush = pDC->SelectObject( GetSysBrush(bCur ?
                                       COLOR_HIGHLIGHT : COLOR_BTNFACE));
        pDC->PatBlt(optionsRect.left + 2,
                    optionsRect.top  + 3 + i * cyEach,
                    optionsRect.Width() - 4, cyEach - 2, PATCOPY);
        pDC->SelectObject(pOldBrush);

        pOldBrush = pDC->SelectObject(GetSysBrush(bCur ?
                                      COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
        pDC->PatBlt(optionsRect.left + 6,
                    optionsRect.top + 2 + cyEach * i + (cyEach - cyHeight) / 2,
                    optionsRect.Width() - 12, cyHeight, PATCOPY);

        pDC->SelectObject(pOldBrush);
        }
    }

/******************************************************************************/

void CRubberTool::OnClickOptions( CImgToolWnd* pWnd, const CRect& optionsRect,
                                                     const CPoint& clickPoint )
    {
    if (m_bFilled)
        {
        CImgTool::OnClickOptions( pWnd, optionsRect, clickPoint );
        return;
        }

    m_nStrokeWidth =  1 + clickPoint.y /
        ((optionsRect.Height() - 4) / nLineWidths);

    // fix for rounding errors
    if (m_nStrokeWidth > nLineWidths)
        {
        m_nStrokeWidth = nLineWidths;
        }

    pWnd->InvalidateOptions(FALSE);
    }

/******************************************************************************/

void CClosedFormTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                                const CRect& optionsRect )
    {

    // Option 0 is Outlined Shape (border and no fill)
    // Option 1 is Filled Shape with border
    // Option 2 is Filled Shape NO border

    #define NUM_CLOSED_FORM_OPTIONS 3 //number of options high

    //*DK* Select Palette into DC
    CBrush*   pOldBrush;
    CRect     cRectOptionSel; // selection rectangle
    CRect     cRectOption;    //rectangle
    int       cyEach = (optionsRect.Height() - 4) / NUM_CLOSED_FORM_OPTIONS; // max height of each option
    int       cyHeight = cyEach - cyEach/2;  //rectangle is 1/2 max height
    int       bCurrSelected = FALSE;
    BOOL      bFilled = CImgTool::GetCurrent()->IsFilled();
    BOOL      bBorder = CImgTool::GetCurrent()->HasBorder();
    int       i;

    for (i = 0; i < NUM_CLOSED_FORM_OPTIONS; i++)
        {
        // Setup the Rectangles for painting and for selection
        //Selection Rectangle
        cRectOptionSel.SetRect(optionsRect.left + 2,
                               optionsRect.top  + 3  + (i * cyEach),
                              (optionsRect.left + 2) + optionsRect.Width() - 4,
                              (optionsRect.top  + 3  + (i* cyEach)) + cyEach - 2);

        //Option Rectangle
        cRectOption.SetRect(optionsRect.left + 6,
                  optionsRect.top  + 2  + i * cyEach + (cyEach - cyHeight) / 2,
                 (optionsRect.left + 6) + optionsRect.Width() - 12,
                 (optionsRect.top  + 2  + i * cyEach + (cyEach - cyHeight) / 2)
                         + cyHeight);

        // Determine the Selection state for the current item.
        bCurrSelected = FALSE;

        switch (i)
            {
            case 0: //Outlined Shape (border, no fill)
                if (! bFilled && bBorder)
                    {
                    bCurrSelected = TRUE;
                    }
                break;

            case 1: // Filled Shape (border and fill)
                if ( (bFilled) && (bBorder) )
                    {
                    bCurrSelected = TRUE;
                    }
                break;
            case 2: // Filled Shape No Border (no border, fill)
                if (bFilled && ! bBorder)
                    {
                    bCurrSelected = TRUE;
                    }
                break;
            default:
                bCurrSelected = FALSE;
                break;
            }
        // Draw the selection State
        // If selected, use COLOR_HIGHLIGHT else use CMP_COLOR_LTGRAY
        pOldBrush = pDC->SelectObject( GetSysBrush( bCurrSelected ?
                                       COLOR_HIGHLIGHT : COLOR_BTNFACE ) );
        pDC->PatBlt( cRectOptionSel.left, cRectOptionSel.top,
                     cRectOptionSel.Width(),cRectOptionSel.Height(), PATCOPY );
        pDC->SelectObject(pOldBrush);


        CBrush* pborderBrush;
        CBrush* pfillBrush;

        pborderBrush = GetSysBrush(bCurrSelected ?
                                   COLOR_BTNHIGHLIGHT : COLOR_BTNTEXT);
        pfillBrush = GetSysBrush(COLOR_BTNSHADOW);

        // Draw the Option
        switch (i)
            {
            case 0: //Outlined Shape (no border, no fill)
                pDC->FrameRect(&cRectOption, pborderBrush);
                break;

            case 1: // Filled Shape (border and fill)
                // using fillrect then frame rect instead of rectangle, since
                // don't have getsyspen facility in this program.
                pDC->FillRect(&cRectOption, pfillBrush);
                pDC->FrameRect(&cRectOption, pborderBrush);
                break;

            case 2: // Filled Shape No Border (no border, fill)
                pDC->FillRect(&cRectOption, pfillBrush);
                break;

            default:
                break;
            }
        }
    }


/******************************************************************************/
// clickpoint is from top of optionsrect (i.e. clickpoint if from 0 to optionsrect.height()
// and thus clickpoint is always less than optionsrec.top

void CClosedFormTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
                                     const CPoint& clickPoint)
    {
    int  cyEach = (optionsRect.Height() - 4) / NUM_CLOSED_FORM_OPTIONS; // max height of each option
//  BOOL bCurrSelected = FALSE;
    int       i;

    for (i = 0; i < NUM_CLOSED_FORM_OPTIONS; i++)
        {
        if ( clickPoint.y <  3 + ((i+1) * cyEach) )
            {
//          bCurrSelected = TRUE;

            switch (i)
                {
                default: // default is same as initial
                case 0: //Outlined Shape (border, no fill)
                    m_bFilled = FALSE;
                    m_bBorder = TRUE;
                    break;

                case 1: // Filled Shape (border and fill)
                    m_bFilled = TRUE;
                    m_bBorder = TRUE;
                    break;

                case 2: // Filled Shape No Border (no border, fill)
                    m_bFilled = TRUE;
                    m_bBorder = FALSE;
                    break;
                }

            break;   // point found, break out of loop test
            }
        }

    pWnd->InvalidateOptions(FALSE);
    }

/******************************************************************************/

void CRubberTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CImgTool::OnStartDrag(pImgWnd, pmti);

    SetupRubber(pImgWnd->m_pImg);
    OnDrag(pImgWnd, pmti);
    }

/******************************************************************************/

void CRubberTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    OnDrag(pImgWnd, pmti);


    CRect rc(pmti->ptDown.x, pmti->ptDown.y, pmti->pt.x, pmti->pt.y);

    Render(CDC::FromHandle(pImgWnd->m_pImg->hDC), rc, pmti->fLeft, TRUE, pmti->fCtrlDown);
    InvalImgRect(pImgWnd->m_pImg, &rc);
    CommitImgRect(pImgWnd->m_pImg, &rc);
    pImgWnd->FinishUndo(rc);

    ClearStatusBarSize();

    CImgTool::OnEndDrag(pImgWnd, pmti);
    }

/******************************************************************************/

void CRubberTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    HPALETTE hpalOld = NULL;

    if (theApp.m_pPalette &&  theApp.m_pPalette->m_hObject)
        {
        hpalOld = SelectPalette( hRubberDC,
                       (HPALETTE)theApp.m_pPalette->m_hObject, FALSE ); // Background ??
        RealizePalette( hRubberDC );
        }

    BitBlt(pImgWnd->m_pImg->hDC, rcPrev.left   , rcPrev.top,
                                 rcPrev.Width(), rcPrev.Height(),
                      hRubberDC, rcPrev.left   , rcPrev.top, SRCCOPY);

    if (hpalOld != NULL)
        SelectPalette( hRubberDC, hpalOld, FALSE ); // Background ??

    InvalImgRect(pImgWnd->m_pImg, &rcPrev);

    PreProcessPoints(pmti);

    CRect rc(pmti->ptDown.x, pmti->ptDown.y, pmti->pt.x, pmti->pt.y);

    Render(CDC::FromHandle(pImgWnd->m_pImg->hDC), rc, pmti->fLeft, FALSE, pmti->fCtrlDown);
    InvalImgRect(pImgWnd->m_pImg, &rc);
    rcPrev = rc;

    if (m_nCmdID != IDMB_POLYGONTOOL)
        {
        CSize size( pmti->pt - pmti->ptDown );

        if (size.cx < 0)
            size.cx -= 1;
        else
            size.cx += 1;
        if (size.cy < 0)
            size.cy -= 1;
        else
            size.cy += 1;

        SetStatusBarPosition( pmti->ptDown );
        SetStatusBarSize    ( size );
        }
    }

/******************************************************************************/

void CRubberTool::AdjustPointsForConstraint(MTI *pmti)
    {
    if (pmti != NULL)
        {
        int iWidthHeight = min( abs(pmti->ptDown.x - pmti->pt.x),
                                abs(pmti->ptDown.y - pmti->pt.y));
        // Set the x value
        if (pmti->pt.x < pmti->ptDown.x)
            {
            pmti->pt.x = pmti->ptDown.x - iWidthHeight;
            }
        else
            {
            pmti->pt.x = pmti->ptDown.x + iWidthHeight;
            }

        // Set the y value
        if (pmti->pt.y < pmti->ptDown.y)
            {
            pmti->pt.y = pmti->ptDown.y - iWidthHeight;
            }
        else
            {
            pmti->pt.y = pmti->ptDown.y + iWidthHeight;
            }

        }
    }

/******************************************************************************/

void CRubberTool::Render( CDC* pDC, CRect& rect, BOOL bLeft, BOOL bCommit, BOOL bCtrlDown )
    {
    int    sx;
    int    sy;
    int    ex;
    int    ey;
    HBRUSH hBr     = NULL;
    HPEN   hPen    = NULL;
    HPEN   hOldPen = NULL;
    HBRUSH hOldBr  = NULL;
    CPoint pt1;
    CPoint pt2;
    HDC    hDC = pDC->m_hDC;

    enum SHAPE { rectangle, roundRect, ellipse } shape;

    switch (m_nCmdID)
        {
        default:
            ASSERT(FALSE);

        case IDMB_RECTTOOL:
            shape = rectangle;
            break;

        case IDMB_FRECTTOOL:
            shape = rectangle;
            break;

        case IDMB_RNDRECTTOOL:
            shape = roundRect;
            break;

        case IDMB_FRNDRECTTOOL:
            shape = roundRect;
            break;

        case IDMB_ELLIPSETOOL:
            shape = ellipse;
            break;

        case IDMB_FELLIPSETOOL:
            shape = ellipse;
            break;
        }

    FixRect(&rect);

    pt1.x = rect.left;
    pt1.y = rect.top;
    pt2.x = rect.right;
    pt2.y = rect.bottom;

    StandardiseCoords(&pt1, &pt2);

    sx = pt1.x;
    sy = pt1.y;
    ex = pt2.x;
    ey = pt2.y;

    SetupPenBrush(hDC, bLeft, TRUE, bCtrlDown);

    CRect rc(sx, sy, ex, ey);

    switch (shape)
        {
        case rectangle:
            Rectangle(hDC, sx, sy, ex, ey);
            break;

        case roundRect:
            RoundRect(hDC, sx, sy, ex, ey, 16, 16);
// The below draws an RoundRect with a mask first then bitblt
//          MyRoundRect(hDC, sx, sy, ex, ey, 16, 16, m_bFilled);
//          // if border and fill, draw border after fill
//          if ( (m_bBorder) && (m_bFilled) )
//          {
//              MyRoundRect(hDC, sx, sy, ex, ey, 16, 16, !m_bFilled);
//          }
            break;

        case ellipse:
            Ellipse(hDC, sx, sy, ex, ey);
// The below draws an Elipse with a mask first then bitblt
//          Mylipse(hDC, sx, sy, ex, ey, m_bFilled);
//          // if border and fill, draw border after fill
//          if ( (m_bBorder) && (m_bFilled) )
//          {
//              Mylipse(hDC, sx, sy, ex, ey, !m_bFilled);
//          }
            break;
        }

    SetupPenBrush(hDC, bLeft, FALSE, bCtrlDown);
    }


void CRubberTool::OnActivate( BOOL bActivate )
    {
        if (bActivate)
        {
                m_nStrokeWidth = g_nStrokeWidth;
        }
        else
        {
                g_nStrokeWidth = m_nStrokeWidth;
        }

    CImgTool::OnActivate( bActivate );
    }


/******************************************************************************/
/*bSetup is true to setup, False to Cleanup                                   */

BOOL CImgTool::SetupPenBrush(HDC hDC, BOOL bLeft, BOOL bSetup, BOOL bCtrlDown)
    {

    COLORREF colorBorder;
    COLORREF colorFill;

    if (bCtrlDown && crTrans != TRANS_COLOR_NONE)
    {
       if (HasBorder ())
       {
          colorBorder = bLeft ? crTrans : crRight;
          colorFill   = bLeft ? crRight : crTrans;
       }
       else
       {
          colorBorder = bLeft ? crRight : crTrans;
          colorFill   = bLeft ? crTrans : crRight;
       }

    }
    else
    {
       if (HasBorder())
       {
          colorBorder = bLeft ? crLeft : crRight;
          colorFill   = bLeft ? crRight: crLeft;
       }
       else
       {
          colorBorder = bLeft ? crRight : crLeft;
          colorFill   = bLeft ? crLeft: crRight;
       }

    }

    static HBRUSH hBr             = NULL;
    static HPEN   hPen            = NULL;
    static HPEN   hOldPen         = NULL;
    static HBRUSH hOldBr          = NULL;
    static BOOL   bCurrentlySetup = FALSE;
    BOOL bRC = TRUE;

    if (bSetup)
        {
        if (! bCurrentlySetup)
            {
            bCurrentlySetup = TRUE;
            // select null objects into DC.  Depending on drawing mode,
            // either or both will be re-selected in to override
            hPen    = NULL;
            hBr     = NULL;
            hOldPen =   (HPEN)SelectObject( hDC, GetStockObject( NULL_PEN ) );
            hOldBr  = (HBRUSH)SelectObject( hDC, GetStockObject( NULL_BRUSH ) );

            if (m_bFilled)
                {
                hBr = CreateSolidBrush( colorFill );
                SelectObject( hDC, hBr );
                }

            if (m_bBorder)
                {
                hPen = CreatePen( PS_INSIDEFRAME, m_nStrokeWidth, colorBorder );
                SelectObject(hDC, hPen);
                }
            else
                {
                //simulate no border by drawing the border the same as the fill.
                // since GDI does not draw small elipses, roundrects correctly
                // with NULL brush for no border.
                // Note the width is 2 so we will dither correctly
                hPen = CreatePen(PS_INSIDEFRAME, 2, colorFill);
                SelectObject(hDC, hPen);
                }
            }
        else
            {
            // Error: Will lose allocated Brush/Pen
            bRC = FALSE;
            }
        }
    else
        {
        if (bCurrentlySetup)
            {
            bCurrentlySetup = FALSE;

            SelectObject(hDC, hOldPen);

            if (hPen != NULL)
                {
                DeleteObject(hPen);
                }

            SelectObject(hDC, hOldBr);

            if (hBr != NULL)
                {
                DeleteObject( hBr );
                }
            }
        else
            {
            // Error: Cannot Free/cleanup Brush/Pen -- Never allocated.
            bRC = FALSE;
            }
        }

    return bRC;
    }

#if 0 // unused code
/******************************************************************************/
/*bSetup is true to setup, False to Cleanup                                   */

BOOL CRubberTool::SetupMaskPenBrush(HDC hDC, BOOL bLeft, BOOL bSetup)
    {
    BOOL bRC = TRUE;

    static HBRUSH hBr = NULL;
    static HPEN hPen = NULL;
    static HPEN hOldPen = NULL;
    static HBRUSH hOldBr = NULL;
    static BOOL bCurrentlySetup = FALSE;

    if (bSetup)
        {
        if (bCurrentlySetup)
            {
            // Error: Will lose allocated Brush/Pen
            bRC = FALSE;
            }
        else
            {
            bCurrentlySetup = TRUE;
            // draw the shape on the mask:
            // select null objects into DC.  Depending on drawing mode,
            // either or both will be re-selected in to override
            hPen = NULL;
            hBr  = NULL;
            hOldPen = (HPEN)SelectObject(hDC, GetStockObject(NULL_PEN));
            hOldBr  = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH));


            if (m_bFilled)
                {
                SelectObject(hDC, GetStockObject( BLACK_BRUSH ));
                }
            if (m_bBorder)
                {
                hPen = CreatePen(PS_INSIDEFRAME, m_nStrokeWidth, (COLORREF)0L );
                SelectObject(hDC, hPen);
                }
            }
        }
    else
        {
        if (bCurrentlySetup)
            {
            bCurrentlySetup = FALSE;

            SelectObject(hDC, hOldPen);
            if (hPen != NULL)
                {
                DeleteObject(hPen);
                }

            SelectObject(hDC, hOldBr);
            if (hBr != NULL)
                {
                DeleteObject(hBr);
                }
            }
        else
            {
            // Error: Cannot Free/cleanup Brush/Pen -- Never allocated.
            bRC = FALSE;
            }
        }

    return bRC;
    }
  #endif // unused code
/******************************************************************************/

CRect  CFreehandTool::c_undoRect;

/***************************************************************************/

CFreehandTool::CFreehandTool()
    {
    m_bUsesBrush = TRUE;
    }

/******************************************************************************/

void CFreehandTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CImgTool::OnStartDrag(pImgWnd, pmti);

    c_undoRect.TopLeft() = c_undoRect.BottomRight() = pmti->pt;
    OnDrag(pImgWnd, pmti);
    }

/******************************************************************************/

void CFreehandTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    if (g_bCustomBrush)
        {
        c_undoRect.left   -= theImgBrush.m_size.cx + theImgBrush.m_handle.cx;
        c_undoRect.top    -= theImgBrush.m_size.cy + theImgBrush.m_handle.cy;
        c_undoRect.right  += theImgBrush.m_size.cx - theImgBrush.m_handle.cx;
        c_undoRect.bottom += theImgBrush.m_size.cy - theImgBrush.m_handle.cy;
        }
    else
        {
        // HACK: +1s are to cover bug in slanted line brushes
        c_undoRect.left   -=  m_nStrokeWidth / 2 + 1;
        c_undoRect.top    -=  m_nStrokeWidth / 2 + 1;
        c_undoRect.right  += (m_nStrokeWidth + 1) / 2 + 1;
        c_undoRect.bottom += (m_nStrokeWidth + 1) / 2 + 1;
        }

    pImgWnd->FinishUndo(c_undoRect);

    CImgTool::OnEndDrag(pImgWnd, pmti);
    }

/******************************************************************************/

CSketchTool::CSketchTool()
    {
    m_nCursorID      = IDC_BRUSH;
    m_nCmdID         = IDMZ_BRUSHTOOL;
    m_bCanBePrevTool = FALSE;
    }

/******************************************************************************/

void CSketchTool::OnDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    fDraggingBrush = FALSE;

    DrawBrush( pImgWnd->m_pImg, pmti->pt, pmti->fLeft );

    if (pmti->pt.x < c_undoRect.left)
        c_undoRect.left = pmti->pt.x;
    else
        if (pmti->pt.x > c_undoRect.right)
            c_undoRect.right = pmti->pt.x;

    if (pmti->pt.y < c_undoRect.top)
        c_undoRect.top = pmti->pt.y;
    else
        if (pmti->pt.y > c_undoRect.bottom)
            c_undoRect.bottom = pmti->pt.y;

    SetStatusBarPosition( pmti->pt );
    }

/******************************************************************************/

void CSketchTool::OnCancel(CImgWnd* pImgWnd)
    {
    HideBrush();
    g_bCustomBrush = FALSE;
    SelectPrevious();
    CImgTool::OnCancel( pImgWnd );
    }

/******************************************************************************/

CBrushTool::CBrushTool()
    {
    m_nCursorID    = IDC_BRUSH;
    m_nCmdID       = IDMB_CBRUSHTOOL;
    m_nStrokeWidth = 4;
    }

/***************************************************************************/

void CBrushTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                           const CRect& optionsRect )
    {
    PaintStdBrushes(pDC, paintRect, optionsRect);
    }

/***************************************************************************/

void CBrushTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    ClickStdBrushes(pWnd, optionsRect, clickPoint);
    }

/***************************************************************************/

void CBrushTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    g_bCustomBrush = FALSE;

    CPoint pt1, pt2;

    fDraggingBrush = FALSE;

    pt1 = pmti->ptPrev;
    pt2 = pmti->pt;

    // use transparent color if defined
    if (pmti->fCtrlDown && crTrans != TRANS_COLOR_NONE)
    {
       DrawImgLine( pImgWnd->m_pImg, pt1, pt2, crTrans,
                    m_nStrokeWidth, m_nStrokeShape, TRUE);
    }
    else
    {
       DrawImgLine( pImgWnd->m_pImg, pt1, pt2,
                 pmti->fLeft ? crLeft : crRight,
                 m_nStrokeWidth, m_nStrokeShape, TRUE);

    }

    if (pmti->pt.x < c_undoRect.left)
        c_undoRect.left = pmti->pt.x;
    else if (pmti->pt.x > c_undoRect.right)
        c_undoRect.right = pmti->pt.x;
    if (pmti->pt.y < c_undoRect.top)
        c_undoRect.top = pmti->pt.y;
    else if (pmti->pt.y > c_undoRect.bottom)
        c_undoRect.bottom = pmti->pt.y;

    SetStatusBarPosition(pmti->pt);
    }

/***************************************************************************/

void CBrushTool::OnMove(CImgWnd* pImgWnd, MTI* pmti)
    {
    g_bCustomBrush = FALSE;
    CImgTool::OnMove(pImgWnd, pmti);
    }

/***************************************************************************/

CPencilTool::CPencilTool()
    {
    m_nCursorID    = IDC_PENCIL;
    m_nCmdID       = IDMB_PENCILTOOL;
    m_bUsesBrush   = FALSE;
    m_nStrokeWidth = 1;
    }

/***************************************************************************/

void CPencilTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CFreehandTool::OnStartDrag(pImgWnd, pmti);
    m_eDrawDirection = eFREEHAND; // initialize to not have a direction

    }

/***************************************************************************/

void CPencilTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    g_bCustomBrush = FALSE;
    fDraggingBrush = FALSE;


    PreProcessPoints(pmti);

    // use transparent color if defined
    if (pmti->fCtrlDown && crTrans != TRANS_COLOR_NONE)
    {
       DrawImgLine (pImgWnd->m_pImg, pmti->ptPrev, pmti->pt,
                    crTrans, m_nStrokeWidth, m_nStrokeShape, TRUE);
    }
    else
    {
       DrawImgLine(pImgWnd->m_pImg, pmti->ptPrev, pmti->pt,
                                 pmti->fLeft ? crLeft : crRight,
                          m_nStrokeWidth, m_nStrokeShape, TRUE);
    }

    if (pmti->pt.x < c_undoRect.left)
        c_undoRect.left = pmti->pt.x;
    else if (pmti->pt.x > c_undoRect.right)
        c_undoRect.right = pmti->pt.x;
    if (pmti->pt.y < c_undoRect.top)
        c_undoRect.top = pmti->pt.y;
    else if (pmti->pt.y > c_undoRect.bottom)
        c_undoRect.bottom = pmti->pt.y;

    SetStatusBarPosition(pmti->pt);
    }

/***************************************************************************/

void CPencilTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    c_undoRect.right += 1;
    c_undoRect.bottom += 1;
    pImgWnd->FinishUndo(c_undoRect);

    CImgTool::OnEndDrag(pImgWnd, pmti); // Bypass CFreehandTool
    }

/******************************************************************************/

void CPencilTool::AdjustPointsForConstraint(MTI *pmti)
    {
    eDRAWCONSTRAINTDIRECTION eDrawDirection = DetermineDrawDirection(pmti);
    int iWidthHeight = min( abs(pmti->ptPrev.x - pmti->pt.x),
                            abs(pmti->ptPrev.y - pmti->pt.y));


    switch (m_eDrawDirection)
        {
        case eEAST_WEST:
             pmti->pt.y = pmti->ptPrev.y;
             break;
        case eNORTH_SOUTH:
             pmti->pt.x = pmti->ptPrev.x;
             break;

        case eNORTH_WEST:
        case eSOUTH_EAST:
             // Set the SE movement
             if ( (pmti->pt.x > pmti->ptPrev.x) ||
                  (pmti->pt.y > pmti->ptPrev.y)    )
                 {
                 pmti->pt.x = pmti->ptPrev.x + iWidthHeight;
                 pmti->pt.y = pmti->ptPrev.y + iWidthHeight;
                 }
             else
                 {
                 // Set the NW movement
                 if ( (pmti->pt.x < pmti->ptPrev.x) ||
                      (pmti->pt.y < pmti->ptPrev.y)    )
                     {
                     pmti->pt.x = pmti->ptPrev.x - iWidthHeight;
                     pmti->pt.y = pmti->ptPrev.y - iWidthHeight;
                     }
                 else
                    {
                    //invalid movement, set to last known position
                    pmti->pt.x = pmti->ptPrev.x;
                    pmti->pt.y = pmti->ptPrev.y;
                    }
                 }
             break;

        case eNORTH_EAST:
        case eSOUTH_WEST:
             // Set the NE movement
             if ( (pmti->pt.x > pmti->ptPrev.x) ||
                  (pmti->pt.y < pmti->ptPrev.y)    )
                 {
                 pmti->pt.x = pmti->ptPrev.x + iWidthHeight;
                 pmti->pt.y = pmti->ptPrev.y - iWidthHeight;
                 }
             else
                 {
                 // Set the SW movement
                 if ( (pmti->pt.x < pmti->ptPrev.x) ||
                      (pmti->pt.y > pmti->ptPrev.y)    )
                     {
                     pmti->pt.x = pmti->ptPrev.x - iWidthHeight;
                     pmti->pt.y = pmti->ptPrev.y + iWidthHeight;
                     }
                 else
                    {
                    //invalid movement, set to last known position
                    pmti->pt.x = pmti->ptPrev.x;
                    pmti->pt.y = pmti->ptPrev.y;
                    }
                 }
             break;


        default: // not in constraint mode yet => do nothing.
                 // Default is freehand
            break;
        }
    }

/***************************************************************************/

CEraserTool::CEraserTool()
    {
    m_nCmdID       = IDMB_ERASERTOOL;
    m_nStrokeWidth = 8;
    m_nStrokeShape = squareBrush;
    m_nCursorID    = NULL;
    }

/***************************************************************************/

void CEraserTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                            const CRect& optionsRect )
    {
    CRect rect;
    int cxOctant = (optionsRect.Width() + 1);
    int cyOctant = (optionsRect.Height() + 1) / 4;

    rect.left = optionsRect.left;
    rect.top = optionsRect.top;
    rect.right = rect.left + cxOctant;
    rect.bottom = rect.top + cyOctant;

    for (UINT nSize = 4; nSize <= 10; nSize += 2)
        {
        CBrush* pOldBrush;

        if (nSize == m_nStrokeWidth)
            {
            pOldBrush = pDC->SelectObject(GetSysBrush(COLOR_HIGHLIGHT));
            pDC->PatBlt(rect.left + (cxOctant - 14) / 2,
                rect.top + (cyOctant - 14) / 2, 14, 14, PATCOPY);
            pDC->SelectObject(pOldBrush);
            }

        pOldBrush = pDC->SelectObject(GetSysBrush(nSize == m_nStrokeWidth ?
            COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
        pDC->PatBlt(rect.left + (cxOctant - nSize) / 2,
            rect.top + (cyOctant - nSize) / 2, nSize, nSize, PATCOPY);
        pDC->SelectObject(pOldBrush);

        rect.top += cyOctant;
        rect.bottom += cyOctant;
        }
    }

/***************************************************************************/

void CEraserTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    int iOptionNumber;
    int cyOctant = (optionsRect.Height() + 1) / 4;
    iOptionNumber = (clickPoint.y / cyOctant);
    if (iOptionNumber > 3)  // there are 4 options, numbered 0,1,2,3
        {
        iOptionNumber = 3;
        }

    m_nStrokeWidth = 4 + 2 * iOptionNumber;

//    int cyOctant = (optionsRect.Height() + 1) / 4;
//    m_nStrokeWidth = 4 + 2 * (clickPoint.y / cyOctant);
    pWnd->InvalidateOptions();
    }


void CEraserTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    if (pmti->fLeft)
        {
        COLORREF crRealLeftColor = crLeft;
        COLORREF crRealRightColor = crRight;

        crLeft = crRight;

        g_bCustomBrush = FALSE;
        fDraggingBrush = FALSE;

        DrawImgLine(pImgWnd->m_pImg, pmti->ptPrev, pmti->pt, crRight,
                                  m_nStrokeWidth, squareBrush, TRUE);

        crLeft  = crRealLeftColor;
        crRight = crRealRightColor;
        }
    else
        {
        // Just erase pixels that match the drawing color...

        g_bCustomBrush = FALSE;
        fDraggingBrush = FALSE;

        HideBrush();

        CDC* pImageDC = CDC::FromHandle(pImgWnd->m_pImg->hDC);

        CRect rc;

        // Call with NULL DC to get the CRect to use
        DrawDCLine(NULL, pmti->ptPrev, pmti->pt, RGB(255, 255, 255),
            m_nStrokeWidth, squareBrush, rc);

        CTempBitmap monoBitmap;
        CDC monoDc;

         // Create the mono DC and bitmap
        if (!monoDc.CreateCompatibleDC(NULL) ||
            !monoBitmap.CreateBitmap(rc.Width(), rc.Height(), 1, 1, NULL))
            {
            theApp.SetGdiEmergency();
            return;
            }

        // Select the bitmap and change the window origin so the mono DC has
        // the same coordinate system as the image
        CBitmap* pOldMonoBitmap = monoDc.SelectObject(&monoBitmap);
        monoDc.SetWindowOrg(rc.left, rc.top);

        // Clear the mono DC and then draw the area that will be changed
        monoDc.PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), BLACKNESS);
        DrawDCLine(monoDc.m_hDC, pmti->ptPrev, pmti->pt, RGB(255, 255, 255),
            m_nStrokeWidth, squareBrush, rc);
        DebugShowBitmap(monoDc.m_hDC, rc.left, rc.top, rc.Width(), rc.Height());

        // Select the proper palette, and make sure the brush origin is set
        // correctly for pattern brushes
        CPalette* pcPaletteOld = theImgBrush.SetBrushPalette(pImageDC, FALSE);
        pImageDC->SetBrushOrg(0, 0);

        CBrush rightBrush;
        rightBrush.CreateSolidBrush(crRight);

        if (!QuickColorToMono(&monoDc, rc.left, rc.top, rc.Width(), rc.Height(),
            pImageDC, rc.left, rc.top, SRCAND, crLeft))
        {
            // We will get her for DDB's (in which case we could be using a
            // dithered brush) or for high color images (so no palette problems)

            // Create the brush to erase
            CBrush leftBrush;
            leftBrush.CreateSolidBrush(crLeft);
            leftBrush.UnrealizeObject();

//#define DPSxna  0x00820c49L
// #define PSDPxax 0x00B8074AL

            // XOR with the pattern so black is where the pattern was
            CBrush* pOldBrush = pImageDC->SelectObject(&leftBrush);
            pImageDC->PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), PATINVERT);
            DebugShowBitmap(pImageDC->m_hDC, rc.left, rc.top, rc.Width(), rc.Height());

            // Color to mono bitblt to get the final mask
            // The ROP will take all pixels in the source that match the pattern
            // and and them with the white pixels in the dest
            theImgBrush.ColorToMonoBitBlt(&monoDc, rc.left, rc.top, rc.Width(), rc.Height(),
                pImageDC, rc.left, rc.top, SRCAND, RGB(0, 0, 0));
            DebugShowBitmap(monoDc.m_hDC, rc.left, rc.top, rc.Width(), rc.Height());

            // XOR again to put the original back
            pImageDC->PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), PATINVERT);
            DebugShowBitmap(pImageDC->m_hDC, rc.left, rc.top, rc.Width(), rc.Height());

            pImageDC->SelectObject(pOldBrush);
        }

        // Copy the pattern back into the image where the bitmap has white
        CBrush *pOldBrush = pImageDC->SelectObject(&rightBrush);

        COLORREF crNewBk, crNewText;
        GetMonoBltColors(pImageDC->m_hDC, NULL, crNewBk, crNewText);
        COLORREF crOldBk = pImageDC->SetBkColor(crNewBk);
        COLORREF crOldText = pImageDC->SetTextColor(crNewText);
        pImageDC->BitBlt(rc.left, rc.top, rc.Width(), rc.Height(),
            &monoDc, rc.left, rc.top, DSPDxax);
        pImageDC->SetBkColor(crOldBk);
        pImageDC->SetTextColor(crOldText);
        DebugShowBitmap(pImageDC->m_hDC, rc.left, rc.top, rc.Width(), rc.Height());

        // Clean up stuff we have selected
        pImageDC->SelectObject(pOldBrush);

        monoDc.SelectObject(pOldMonoBitmap);

        if (pcPaletteOld)
            pImageDC->SelectPalette(pcPaletteOld, FALSE);

        InvalImgRect(pImgWnd->m_pImg, &rc);
        CommitImgRect(pImgWnd->m_pImg, &rc);
        }

    if (pmti->pt.x < c_undoRect.left)
        c_undoRect.left = pmti->pt.x;
    else if (pmti->pt.x > c_undoRect.right)
        c_undoRect.right = pmti->pt.x;
    if (pmti->pt.y < c_undoRect.top)
        c_undoRect.top = pmti->pt.y;
    else if (pmti->pt.y > c_undoRect.bottom)
        c_undoRect.bottom = pmti->pt.y;

    SetStatusBarPosition(pmti->pt);

    fDraggingBrush = TRUE;

    pImgWnd->ShowBrush(pmti->pt);
    }

/***************************************************************************/

void CEraserTool::OnMove(CImgWnd* pImgWnd, MTI* pmti)
    {
    COLORREF crRealLeftColor;
    COLORREF crRealRightColor;

    crRealLeftColor  = crLeft;
    crRealRightColor = crRight;

    crLeft = crRight;

    g_bCustomBrush = FALSE;

    CImgTool::OnMove(pImgWnd, pmti);

    crLeft  = crRealLeftColor;
    crRight = crRealRightColor;
    }

/***************************************************************************/

void CEraserTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    c_undoRect.left   -=  m_nStrokeWidth / 2;
    c_undoRect.top    -=  m_nStrokeWidth / 2;
    c_undoRect.right  += (m_nStrokeWidth + 1) / 2;
    c_undoRect.bottom += (m_nStrokeWidth + 1) / 2;
    pImgWnd->FinishUndo(c_undoRect);

    CImgTool::OnEndDrag(pImgWnd, pmti); // Bypass CFreehandTool
    }

/***************************************************************************/

void CEraserTool::OnShowDragger(CImgWnd* pImgWnd, BOOL bShow)
    {
    if (bShow && g_bBrushVisible)
        {
        CClientDC dc(pImgWnd);

        CRect imageRect;
        pImgWnd->GetImageRect(imageRect);
        dc.IntersectClipRect(&imageRect);

        BOOL bGrid = pImgWnd->IsGridVisible();

        CRect rect = rcDragBrush;
        pImgWnd->ImageToClient(rect);
        dc.PatBlt(rect.left, rect.top,
            rect.Width() + bGrid, 1, BLACKNESS);
        dc.PatBlt(rect.left, rect.top + 1,
            1, rect.Height() - 2 + bGrid, BLACKNESS);
        dc.PatBlt(rect.right - 1 + bGrid, rect.top + 1,
            1, rect.Height() - 2 + bGrid, BLACKNESS);
        dc.PatBlt(rect.left, rect.bottom - 1 + bGrid,
            rect.Width() + bGrid, 1, BLACKNESS);
        }
    }

/***************************************************************************/

UINT CEraserTool::GetCursorID()
    {
    CPoint point;
    GetCursorPos(&point);

    CRect rc;

    CPBView* pcbView = (CPBView*)((CFrameWnd*)AfxGetMainWnd())->GetActiveView();
    CImgWnd* pImgWnd = pcbView->m_pImgWnd;

    pImgWnd->ScreenToClient(&point);
    pImgWnd->GetClientRect(&rc);
    if (!rc.PtInRect(point))
    {
        // Return crosshair outside the client rect of the image window
        return LOWORD(IDC_CROSSHAIR);
    }

    pImgWnd->ClientToImage(point);
    if (point.x > pImgWnd->m_pImg->cxWidth ||
        point.y > pImgWnd->m_pImg->cyHeight)
    {
        // Return crosshair outside the drawing area
        return LOWORD(IDC_CROSSHAIR);
    }

    return m_nCursorID;
    }

/***************************************************************************/

CImageWell  CAirBrushTool::c_imageWell(IDB_AIROPT, CSize(24, 24));

CAirBrushTool::CAirBrushTool()
    {
    m_nCmdID       = IDMB_AIRBSHTOOL;
    m_nStrokeWidth = 8;
    m_nCursorID    = IDCUR_AIRBRUSH;
    m_bUsesBrush   = FALSE;
    m_bFilled      = TRUE;
    }

/***************************************************************************/

void CAirBrushTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                              const CRect& optionsRect )
    {
    CPoint pt(optionsRect.left + (optionsRect.Width() / 2 - 24) / 2,
        optionsRect.top + (optionsRect.Height() / 2 - 24) / 2);

    c_imageWell.Open();

    pDC->SetTextColor(GetSysColor(
        m_nStrokeWidth == 8 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        m_nStrokeWidth == 8 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
    c_imageWell.DrawImage(pDC, pt, 0, SRCCOPY);
    pt.x += optionsRect.Width() / 2;

    pDC->SetTextColor(GetSysColor(
        m_nStrokeWidth == 16 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        m_nStrokeWidth == 16 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
    c_imageWell.DrawImage(pDC, pt, 1, SRCCOPY);
    pt.x = optionsRect.left + (optionsRect.Width() - 24) / 2;

    pDC->SetTextColor(GetSysColor(
        m_nStrokeWidth == 24 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        m_nStrokeWidth == 24 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
    pt.y += optionsRect.Height() / 2;
    c_imageWell.DrawImage(pDC, pt, 2, SRCCOPY);

    c_imageWell.Close();
    }

/***************************************************************************/

void CAirBrushTool::OnClickOptions(CImgToolWnd* pWnd,
    const CRect& optionsRect, const CPoint& clickPoint)
    {
    UINT nNewStrokeWidth;

    if (clickPoint.y > optionsRect.Height() / 2)
        nNewStrokeWidth = 24;
    else if (clickPoint.x > optionsRect.Width() / 2)
        nNewStrokeWidth = 16;
    else
        nNewStrokeWidth = 8;

    if (nNewStrokeWidth != m_nStrokeWidth)
        SetStrokeWidth(nNewStrokeWidth);
    }

/***************************************************************************/

void CAirBrushTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    pImgWnd->SetTimer(1, 0, NULL); // FUTURE: rate should be adjustable
    CFreehandTool::OnStartDrag(pImgWnd, pmti);
    }

/***************************************************************************/

void CAirBrushTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CPoint pt;
    CRect rect;

    fDraggingBrush = FALSE;

    int nDiam = (m_nStrokeWidth + 1) & ~1; // nDiam must be even
    if (nDiam < 4)
        nDiam = 4;
    int nRadius = nDiam / 2;
    int nRadiusSquared = (nDiam / 2) * (nDiam / 2);

    // Start a bounding rect for changes made in the following loop
    rect.left = rect.right = pmti->pt.x;
    rect.top = rect.bottom = pmti->pt.y;

    m_bCtrlDown = pmti->fCtrlDown; // save it for the timer
    SetupPenBrush(pImgWnd->m_pImg->hDC, !pmti->fLeft, TRUE, m_bCtrlDown);

    for (int i = 0; i < 10; i++)
        {
        // Loop here until we randomly pick a point inside a circle
        // centered around the mouse with a diameter of m_nStrokeWidth...
#ifdef _DEBUG
        int nTrys = 0;
#endif
        do
            {
#ifdef _DEBUG
            if (nTrys++ > 10)
                {
                TRACE(TEXT("The airbrush is clogged!\n"));
                break;
                }
#endif
            pt = pmti->pt;
            pt.x += (rand() % (nDiam + 1)) - nRadius;
            pt.y += (rand() % (nDiam + 1)) - nRadius;
            }
        while (((pt.x - pmti->pt.x) * (pt.x - pmti->pt.x) +
                (pt.y - pmti->pt.y) * (pt.y - pmti->pt.y)) > nRadiusSquared);

        PatBlt(pImgWnd->m_pImg->hDC, pt.x, pt.y, 1, 1, PATCOPY);

        if (pt.x < rect.left)
            rect.left = pt.x;
        else if (pt.x + 1 > rect.right)
            rect.right = pt.x + 1;
        if (pt.y < rect.top)
            rect.top = pt.y;
        else if (pt.y + 1 > rect.bottom)
            rect.bottom = pt.y + 1;
        }

    SetupPenBrush(pImgWnd->m_pImg->hDC, !pmti->fLeft, FALSE, m_bCtrlDown);

    c_undoRect |= rect;

    InvalImgRect(pImgWnd->m_pImg, &rect);
    CommitImgRect(pImgWnd->m_pImg, &rect);

    SetStatusBarPosition(pmti->pt);
    }

/***************************************************************************/

void CAirBrushTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    pImgWnd->KillTimer(1);
    CFreehandTool::OnEndDrag(pImgWnd, pmti);
    }

/***************************************************************************/

void CAirBrushTool::OnTimer(CImgWnd* pImgWnd, MTI* pmti)
    {
       pmti->fCtrlDown = m_bCtrlDown;
    OnDrag(pImgWnd, pmti);
    }

/***************************************************************************/

void CAirBrushTool::OnCancel(CImgWnd* pImgWnd)
    {
    pImgWnd->KillTimer(1);
    CImgTool::OnCancel(pImgWnd);
    }

/***************************************************************************/

CLineTool::CLineTool()
    {
    m_bUsesBrush   = FALSE;
    m_nStrokeWidth = 1;
    m_nCmdID       = IDMB_LINETOOL;
    }

/***************************************************************************/

void CLineTool::Render(CDC* pDC, CRect& rect, BOOL bLeft, BOOL bCommit, BOOL bCtrlDown)
    {

    COLORREF color;

    // use transparent color if defined
    if (bCtrlDown && crTrans != TRANS_COLOR_NONE)
    {
       color = crTrans;
    }
    else
    {
       color = bLeft ? crLeft : crRight;
    }

    int sx = rect.left;
    int sy = rect.top;
    int ex = rect.right;
    int ey = rect.bottom;

    DrawImgLine( pImgCur, rect.TopLeft(), rect.BottomRight(), color,
                  m_nStrokeWidth, m_nStrokeShape, FALSE );
    CRect rc;
    if (sx < ex)
        {
        rc.left = sx;
        rc.right = ex + 1;
        }
    else
        {
        rc.left = ex;
        rc.right = sx + 1;
        }

    if (sy < ey)
        {
        rc.top = sy;
        rc.bottom = ey + 1;
        }
    else
        {
        rc.top = ey;
        rc.bottom = sy + 1;
        }

    rc.left   -= m_nStrokeWidth;
    rc.top    -= m_nStrokeWidth;
    rc.right  += m_nStrokeWidth;
    rc.bottom += m_nStrokeWidth;

    rect = rc;
    }

/******************************************************************************/
// Given an x and y coordinate, we can calculate the angle from the x axis in
// the right triangle using the Tan(a) algorithm.  Where
// tan(a) = opposite/adjacent or y/x.
//
// In order to constrain the line drawing, we need to determine the angle
// from the x axis and constrain it to the nearest 45 degree line (0 degree,
// 45 degree, 90 degree,....).
//
// Thus we can use the following rule :
//
//         0 Degrees <=   Angle   <     45/2 Degrees  Constrained to  0 Degrees
//      45/2 Degrees <=   Angle   <  45+45/2 Degrees  Constrained to 45 Degrees
//   45+45/2 Degrees <=   Angle   <       90 Degrees  Constrained to 90 Degrees
//
//
// We can translate this rule into the below using tan(angle) = y/x and the
// fact that Tan(0) = 0, Tan(22.5) = .414, tan(67.5) = 2.414, tan(90) = infinity
//
//         0 <=   y/x   <     .414   Constrained to  0 Degrees
//      .414 <=   y/x   <    2.414   Constrained to 45 Degrees
//     2.414 <=   y/x                Constrained to 90 Degrees
//
// For more precision, we will multiply everything by 1000 to give us finally
// the following table
//
//         0 <=   (1000*y)/x  <     414   Constrained to  0 Degrees
//       414 <=   (1000*y)/x  <    2414   Constrained to 45 Degrees
//      2414 <=   (1000*y)/x              Constrained to 90 Degrees

void CLineTool::AdjustPointsForConstraint(MTI *pmti)
    {
    if (pmti != NULL)
        {
        int iAngle = 0;

        long lcy = abs( (pmti->ptDown).y - (pmti->pt).y );
        long lcx = abs( (pmti->ptDown).x - (pmti->pt).x );
        long lResult;

        if (lcx != 0)
            {
            lResult = (lcy*1000)/lcx;
            }
        else
            {
            lResult = 2414; // default to 90 degrees if x value is 0.
            }

        if (lResult >= 2414)
            {
            iAngle = 90;
            }
        else
            {
            if (lResult >= 414)
                {
                iAngle = 45;
                }
            else
                {
                iAngle = 0;
                }
            }


//      int iWidthHeight = min( abs(pmti->ptDown.x - pmti->pt.x),
//                              abs(pmti->ptDown.y - pmti->pt.y));
        int iWidthHeight = ( abs(pmti->ptDown.x - pmti->pt.x) +
                             abs(pmti->ptDown.y - pmti->pt.y) ) / 2 ;

        switch (iAngle)
            {
            default: //if for some reason, angle is not valid case, use 0
            case 0:
                pmti->pt.y = pmti->ptDown.y;
                break;

            case 45:
                if (pmti->pt.x < pmti->ptDown.x)
                    {
                    pmti->pt.x = pmti->ptDown.x - iWidthHeight;
                    }
                else
                    {
                    pmti->pt.x = pmti->ptDown.x + iWidthHeight;
                    }

                if (pmti->pt.y < pmti->ptDown.y)
                    {
                    pmti->pt.y = pmti->ptDown.y - iWidthHeight;
                    }
                else
                    {
                    pmti->pt.y = pmti->ptDown.y + iWidthHeight;
                    }

                break;

            case 90:
                pmti->pt.x = pmti->ptDown.x;
                break;
            }
        }
    }

/***************************************************************************/

CRectTool::CRectTool()
    {
    m_nCmdID = IDMB_RECTTOOL;
    }

/***************************************************************************/

CRoundRectTool::CRoundRectTool()
    {
    m_nCmdID = IDMB_RNDRECTTOOL;
    }

/***************************************************************************/

CEllipseTool::CEllipseTool()
    {
    m_nCmdID = IDMB_ELLIPSETOOL;
    }

/***************************************************************************/

CPickColorTool::CPickColorTool()
    {
    m_bIsUndoable     = FALSE;
    m_bCanBePrevTool  = FALSE;
    m_bToggleWithPrev = TRUE;
    m_Color           = ::GetSysColor( COLOR_BTNFACE );
    m_nCursorID       = IDC_EYEDROP;
    m_nCmdID          = IDMY_PICKCOLOR;
    }

/***************************************************************************/

void CPickColorTool::OnActivate(BOOL bActivate)
    {
    g_bPickingColor = bActivate;

    m_Color = ::GetSysColor( COLOR_BTNFACE );

    CImgTool::OnActivate(bActivate);
    }

/***************************************************************************/

void CPickColorTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CImgTool::OnStartDrag(pImgWnd, pmti);
    OnDrag(pImgWnd, pmti);
    }

/***************************************************************************/

void CPickColorTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    COLORREF cr = GetPixel(pImgWnd->m_pImg->hDC, pmti->pt.x, pmti->pt.y);

    BYTE red   = GetRValue( cr );
    BYTE green = GetGValue( cr );
    BYTE blue  = GetBValue( cr );

    if (theApp.m_bPaletted)
        m_Color = PALETTERGB( red, green, blue );
    else
        m_Color =        RGB( red, green, blue );

    if (g_pImgToolWnd && g_pImgToolWnd->m_hWnd &&
        IsWindow(g_pImgToolWnd->m_hWnd) )
        g_pImgToolWnd->InvalidateOptions();
    }

/***************************************************************************/

void CPickColorTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    if (pmti->fCtrlDown) // pick transparent color
    {
       SetTransColor (m_Color);
    }
    else if (pmti->fLeft)
        SetDrawColor ( m_Color );
    else
        SetEraseColor( m_Color );

    m_Color = ::GetSysColor( COLOR_BTNFACE );

    if (g_pImgToolWnd && g_pImgToolWnd->m_hWnd &&
        IsWindow(g_pImgToolWnd->m_hWnd) )
        g_pImgToolWnd->InvalidateOptions();

    SelectPrevious();
    CImgTool::OnEndDrag( pImgWnd, pmti );
    }

/***************************************************************************/

void CPickColorTool::OnCancel(CImgWnd* pImgWnd)
    {
    SelectPrevious();
    CImgTool::OnCancel(pImgWnd);
    }

/***************************************************************************/

void CPickColorTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                               const CRect& optionsRect )
    {
    CPalette* pOldPal = NULL;

    if (theApp.m_pPalette)
        {
        pOldPal = pDC->SelectPalette( theApp.m_pPalette, FALSE );
        pDC->RealizePalette();
        }

    CBrush br;

    if (br.CreateSolidBrush( m_Color ))
        {
        pDC->FillRect( &paintRect, &br );

        br.DeleteObject();
        }

    if (pOldPal)
        pDC->SelectPalette( pOldPal, FALSE );
    }

/***************************************************************************/

CFloodTool::CFloodTool()
    {
    m_nCursorID = IDC_FLOOD;
    m_nCmdID    = IDMB_FILLTOOL;
    m_bFilled   = TRUE;
    }

/***************************************************************************/

void CFloodTool::OnPaintOptions(CDC* pDC, const CRect& paintRect,
                                          const CRect& optionsRect)
    {
//  PaintStdPattern(pDC, paintRect, optionsRect);
    }

/***************************************************************************/

void CFloodTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    CImgTool::OnClickOptions(pWnd, optionsRect, clickPoint);
    }

/***************************************************************************/

void CFloodTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CImgTool::OnStartDrag( pImgWnd, pmti );

    CPalette *pcPaletteOld = NULL;


    IMG* pimg  = pImgWnd->m_pImg;

    CDC* pDC = CDC::FromHandle( pimg->hDC );

    CBrush  brush;
    CBrush* pOldBrush = NULL;

    COLORREF color;
    if (pmti->fCtrlDown && crTrans != TRANS_COLOR_NONE)
    {
       color = crTrans;
    }
    else
    {
       color = pmti->fLeft ? crLeft : crRight;
    }
    if (theApp.m_pPalette)
        {
        pcPaletteOld = pDC->SelectPalette( theApp.m_pPalette, FALSE );
        pDC->RealizePalette();
        }

    if (brush.CreateSolidBrush( color ))
        {
        pOldBrush = pDC->SelectObject( &brush );

        COLORREF crFillThis = pDC->GetPixel( pmti->pt.x, pmti->pt.y );

        BYTE iRed   = GetRValue( crFillThis );
        BYTE iGreen = GetGValue( crFillThis );
        BYTE iBlue  = GetBValue( crFillThis );

        if (theApp.m_bPaletted)
            crFillThis = PALETTERGB( iRed, iGreen, iBlue );
        else
            crFillThis =        RGB( iRed, iGreen, iBlue );

        pDC->ExtFloodFill( pmti->pt.x,
                           pmti->pt.y, crFillThis, FLOODFILLSURFACE );

        pDC->SelectObject( pOldBrush );

        InvalImgRect ( pimg, NULL );
        CommitImgRect( pimg, NULL );
        }
    else
        {
        theApp.SetGdiEmergency();
        }

    if (pcPaletteOld)
        pDC->SelectPalette( pcPaletteOld, FALSE );
    }

/***************************************************************************/

void CFloodTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    pImgWnd->FinishUndo(CRect(0, 0,
         pImgWnd->m_pImg->cxWidth, pImgWnd->m_pImg->cyHeight));

    CImgTool::OnEndDrag(pImgWnd, pmti);
    }

/***************************************************************************/

CRect  CSelectTool::c_selectRect;
CImageWell  CSelectTool::c_imageWell(IDB_SELOPT, CSize(37, 23));

CSelectTool::CSelectTool()
    {
    m_bIsUndoable    = FALSE;
    m_nCmdID         = IDMB_PICKTOOL;
    m_bCanBePrevTool = FALSE;
    }

/***************************************************************************/

void CSelectTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                            const CRect& optionsRect )
    {
    CPoint pt(optionsRect.left + (optionsRect.Width()      - 37) / 2,
              optionsRect.top  + (optionsRect.Height() / 2 - 23) / 2);

    CRect selRect(pt.x - 3, pt.y - 3, pt.x + 37 + 3, pt.y + 23 + 3);

    CBrush* pOldBrush;

    pOldBrush = pDC->SelectObject( GetSysBrush(theImgBrush.m_bOpaque ?
                                    COLOR_HIGHLIGHT : COLOR_BTNFACE));

    pDC->PatBlt(selRect.left, selRect.top,
                selRect.Width(), selRect.Height(), PATCOPY);

    pDC->SelectObject(pOldBrush);

    selRect.OffsetRect(0, optionsRect.Height() / 2);

    pOldBrush = pDC->SelectObject(GetSysBrush(theImgBrush.m_bOpaque ?
                                  COLOR_BTNFACE : COLOR_HIGHLIGHT));

    pDC->PatBlt(selRect.left, selRect.top,
                selRect.Width(), selRect.Height(), PATCOPY);

    pDC->SelectObject(pOldBrush);

    c_imageWell.Open();

    c_imageWell.DrawImage(pDC, pt, 0);

    pt.y += optionsRect.Height() / 2;

    c_imageWell.DrawImage(pDC, pt, 1);

    c_imageWell.Close();
    }

/***************************************************************************/

void CSelectTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
                                                    const CPoint& clickPoint)
    {
    BOOL bNewOpaque = clickPoint.y < optionsRect.Height() / 2;

    if (bNewOpaque != theImgBrush.m_bOpaque)
        {
        HideBrush();

        theImgBrush.m_bOpaque = bNewOpaque;
        theImgBrush.RecalcMask(crRight);

        CImgWnd::GetCurrent()->MoveBrush(theImgBrush.m_rcSelection);

        pWnd->InvalidateOptions();
        }
    }

/***************************************************************************/

void CSelectTool::InvertSelectRect(CImgWnd* pImgWnd)
    {
    if (c_selectRect.IsRectEmpty())
        return;

    CClientDC dc( pImgWnd );

    CBrush* pOldBrush = NULL;
    int iLineWidth = pImgWnd->GetZoom();

    if (g_brSelectHorz.m_hObject != NULL)
        pOldBrush = dc.SelectObject( &g_brSelectHorz );
    else
        pOldBrush = (CBrush*)dc.SelectStockObject( BLACK_BRUSH );

    CRect invertRect = c_selectRect;

    pImgWnd->ImageToClient( invertRect );

    int iWidth  = invertRect.Width();
    int iHeight = invertRect.Height();

    dc.PatBlt( invertRect.left, invertRect.top, iWidth - iLineWidth, iLineWidth, PATINVERT );
    dc.PatBlt( invertRect.left, invertRect.top + iHeight - iLineWidth, iWidth - iLineWidth, iLineWidth, PATINVERT );

    if (g_brSelectVert.m_hObject != NULL)
        dc.SelectObject( &g_brSelectVert );

    dc.PatBlt( invertRect.left, invertRect.top + iLineWidth * 2, iLineWidth, iHeight - iLineWidth * 3, PATINVERT );
    dc.PatBlt( invertRect.right - iLineWidth, invertRect.top, iLineWidth, iHeight, PATINVERT );

    if (pOldBrush != NULL)
        dc.SelectObject( pOldBrush );
    }

/***************************************************************************/

void CSelectTool::OnShowDragger(CImgWnd* pImgWnd, BOOL bShow)
    {
    if (!bShow)
        {
        InvertSelectRect(pImgWnd);
        c_selectRect.SetRect(0, 0, 0, 0);
        }
    }

/***************************************************************************/

void CSelectTool::OnActivate(BOOL bActivate)
    {
    if (!bActivate)
        {
        if (theImgBrush.m_pImg != NULL)
            {
            if (! theImgBrush.m_bFirstDrag)
                CommitSelection(TRUE);

            InvalImgRect(theImgBrush.m_pImg, NULL); // erase selection tracker
            theImgBrush.m_pImg = NULL;
            }
        }

    CImgTool::OnActivate(bActivate);
    }

/***************************************************************************/

void CSelectTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CImgTool::OnStartDrag(pImgWnd, pmti);

    CommitSelection(TRUE);

    pImgWnd->EraseTracker();

    theImgBrush.m_bMakingSelection = TRUE;
    }

/***************************************************************************/

void CSelectTool::OnDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CRect newSelectRect(pmti->ptDown.x, pmti->ptDown.y,
        pmti->pt.x, pmti->pt.y);
    FixRect(&newSelectRect);
    newSelectRect.right += 1;
    newSelectRect.bottom += 1;

    if (newSelectRect.left < 0)
        newSelectRect.left = 0;
    if (newSelectRect.top < 0)
        newSelectRect.top = 0;
    if (newSelectRect.right > pImgWnd->GetImg()->cxWidth)
        newSelectRect.right = pImgWnd->GetImg()->cxWidth;
    if (newSelectRect.bottom > pImgWnd->GetImg()->cyHeight)
        newSelectRect.bottom = pImgWnd->GetImg()->cyHeight;

    if (newSelectRect != c_selectRect)
        {
        InvertSelectRect(pImgWnd);
        c_selectRect = newSelectRect;
        InvertSelectRect(pImgWnd);
        }

    SetStatusBarPosition(pmti->ptDown);
    SetStatusBarSize(c_selectRect.Size());
    }

/***************************************************************************/

void CSelectTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    InvertSelectRect(pImgWnd);
    c_selectRect.SetRect(0, 0, 0, 0);

    CRect rcPick;

    theImgBrush.m_bMakingSelection = FALSE;

    if (pmti->ptDown.x > pmti->pt.x)
        {
        rcPick.left = pmti->pt.x;
        rcPick.right = pmti->ptDown.x;
        }
    else
        {
        rcPick.left = pmti->ptDown.x;
        rcPick.right = pmti->pt.x;
        }

    if (pmti->ptDown.y > pmti->pt.y)
        {
        rcPick.top = pmti->pt.y;
        rcPick.bottom = pmti->ptDown.y;
        }
    else
        {
        rcPick.top = pmti->ptDown.y;
        rcPick.bottom = pmti->pt.y;
        }

    if (rcPick.left < 0)
        rcPick.left = 0;
    if (rcPick.top < 0)
        rcPick.top = 0;
    if (rcPick.right > pImgWnd->m_pImg->cxWidth - 1)
        rcPick.right = pImgWnd->m_pImg->cxWidth - 1;
    if (rcPick.bottom > pImgWnd->m_pImg->cyHeight - 1)
        rcPick.bottom = pImgWnd->m_pImg->cyHeight - 1;

    if (rcPick.Width() == 0 || rcPick.Height() == 0)
        {
        theImgBrush.TopLeftHandle();

        theImgBrush.m_bMoveSel = theImgBrush.m_bSmearSel = FALSE;
        g_bCustomBrush = FALSE;
        SetCombineMode(combineColor);

        InvalImgRect(pImgWnd->m_pImg, NULL);  // redraw selection
        theImgBrush.m_pImg = NULL;
        }
    else
        {
        rcPick.right += 1;
        rcPick.bottom += 1;

        pImgWnd->MakeBrush(pImgWnd->m_pImg->hDC, rcPick );
        }

    ClearStatusBarSize();

    CImgTool::OnEndDrag(pImgWnd, pmti);

    if (pmti->fRight && !pmti->fLeft)
    {
        CPoint pt = pmti->pt;

        pImgWnd->OnRButtonDownInSel(&pt);
    }

    }

/***************************************************************************/

void CSelectTool::OnCancel(CImgWnd* pImgWnd)
    {
    if (! theImgBrush.m_bMakingSelection && CWnd::GetCapture() != pImgWnd)
        {
        // We were not selecting or dragging, just cancel the select tool...
        CommitSelection(TRUE);

        theImgBrush.TopLeftHandle();

        theImgBrush.m_bMoveSel = theImgBrush.m_bSmearSel = FALSE;
        g_bCustomBrush = FALSE;
        SetCombineMode(combineColor);

        if (theImgBrush.m_pImg != NULL)
            InvalImgRect(theImgBrush.m_pImg, NULL);  // redraw selection

        theImgBrush.m_pImg = NULL;
        CImgTool::OnCancel(pImgWnd);
        return;
        }

    if (!theImgBrush.m_bMakingSelection && CWnd::GetCapture() == pImgWnd)
        {
        HideBrush();

        if (!theImgBrush.m_bMoveSel && !theImgBrush.m_bSmearSel)
            {
            if (g_bCustomBrush)
                {
                theImgBrush.TopLeftHandle();

                g_bCustomBrush = FALSE;
                SetCombineMode(combineColor);
                }
            else
                {
                if (theImgBrush.m_pImg)
                    CommitSelection(TRUE);
                InvalImgRect(pImgWnd->m_pImg, NULL); // erase the dragger
                }
            }
        }

    InvertSelectRect(pImgWnd);
    c_selectRect.SetRect(0, 0, 0, 0);

    theImgBrush.TopLeftHandle();

    g_bCustomBrush = FALSE;
    theImgBrush.m_pImg = NULL;
    theImgBrush.m_bMoveSel = theImgBrush.m_bSmearSel = FALSE;
    theImgBrush.m_bMakingSelection = FALSE;

    InvalImgRect(pImgWnd->m_pImg, NULL);

    CImgTool::OnCancel(pImgWnd);
    }

/***************************************************************************/

BOOL CSelectTool::IsToolModal(void)
{
        if (theImgBrush.m_pImg)
        {
                return(TRUE);
        }

        return(CImgTool::IsToolModal());
}

/***************************************************************************/

UINT CSelectTool::GetCursorID()
    {
    CPoint point;
    GetCursorPos(&point);
    CImgWnd* pImgWnd = (CImgWnd*)CWnd::WindowFromPoint(point);

    if (pImgWnd->IsKindOf(RUNTIME_CLASS(CImgWnd))
    &&  pImgWnd->GetImg() == pImgCur
    &&  theImgBrush.m_pImg != NULL)
        {
        pImgWnd->ScreenToClient(&point);
        pImgWnd->ClientToImage(point);

        if (theImgBrush.m_rcSelection.PtInRect(point))
            return IDCUR_MOVE;
        }

    return m_nCursorID;
    }

/***************************************************************************/

CRect  CZoomTool::c_zoomRect;
CImgWnd* CZoomTool::c_pImgWnd;
CImageWell  CZoomTool::c_imageWell(IDB_ZOOMOPT, CSize(23, 9));

/***************************************************************************/

CZoomTool::CZoomTool()
    {
    m_bIsUndoable     = FALSE;
    m_bCanBePrevTool  = FALSE;
    m_bToggleWithPrev = TRUE;

    m_nCursorID       = IDC_ZOOMIN;
    m_nCmdID          = IDMB_ZOOMTOOL;
    }

/***************************************************************************/

void CZoomTool::OnPaintOptions( CDC* pDC, const CRect& paintRect,
                                          const CRect& optionsRect )
    {
    int nCurZoom = CImgWnd::GetCurrent()->GetZoom();
    int dy = optionsRect.Height() / 4;
    CPoint pt(optionsRect.left + (optionsRect.Width() - 23) / 2,
        optionsRect.top + optionsRect.Height() / dy);

    c_imageWell.Open();

    if (nCurZoom == 1)
        {
        CBrush* pOldBrush;
        pOldBrush = pDC->SelectObject(GetSysBrush(COLOR_HIGHLIGHT));
        pDC->PatBlt(pt.x - 8, pt.y - 2, 23 + 16, 9 + 4,
            PATCOPY);
        pDC->SelectObject(pOldBrush);
        }
    pDC->SetTextColor(GetSysColor(
        nCurZoom == 1 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        nCurZoom == 1 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
    c_imageWell.DrawImage(pDC, pt, 0, SRCCOPY);
    pt.y += dy;

    if (nCurZoom == 2)
        {
        CBrush* pOldBrush;
        pOldBrush = pDC->SelectObject(GetSysBrush(COLOR_HIGHLIGHT));
        pDC->PatBlt(pt.x - 8, pt.y - 2, 23 + 16, 9 + 4, PATCOPY);
        pDC->SelectObject(pOldBrush);
        }
    pDC->SetTextColor(GetSysColor(
        nCurZoom == 2 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        nCurZoom == 2 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
    c_imageWell.DrawImage(pDC, pt, 1, SRCCOPY);
    pt.y += dy;

    if (nCurZoom == 6)
        {
        CBrush* pOldBrush;
        pOldBrush = pDC->SelectObject(GetSysBrush(COLOR_HIGHLIGHT));
        pDC->PatBlt(pt.x - 8, pt.y - 2, 23 + 16, 9 + 4, PATCOPY);
        pDC->SelectObject(pOldBrush);
        }
    pDC->SetTextColor(GetSysColor(
        nCurZoom == 6 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        nCurZoom == 6 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
    c_imageWell.DrawImage(pDC, pt, 2, SRCCOPY);
    pt.y += dy;

    if (nCurZoom == 8)
        {
        CBrush* pOldBrush;
        pOldBrush = pDC->SelectObject(GetSysBrush(COLOR_HIGHLIGHT));
        pDC->PatBlt(pt.x - 8, pt.y - 2, 23 + 16, 9 + 4, PATCOPY);
        pDC->SelectObject(pOldBrush);
        }
    pDC->SetTextColor(GetSysColor(
        nCurZoom == 8 ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
    pDC->SetBkColor(GetSysColor(
        nCurZoom == 8 ? COLOR_HIGHLIGHT : COLOR_BTNFACE));;
    c_imageWell.DrawImage(pDC, pt, 3, SRCCOPY);

    c_imageWell.Close();
    }

/***************************************************************************/

void CZoomTool::OnClickOptions(CImgToolWnd* pWnd, const CRect& optionsRect,
    const CPoint& clickPoint)
    {
    int nNewZoom = clickPoint.y / (optionsRect.Height() / 4) + 1;
    if (nNewZoom >= 3)
        nNewZoom *= 2;

    if (nNewZoom != CImgWnd::GetCurrent()->GetZoom())
        {
        CImgWnd::GetCurrent()->SetZoom(nNewZoom);
        CImgWnd::GetCurrent()->CheckScrollBars();

        pWnd->InvalidateOptions();
        }

    SelectPrevious();
    }

/***************************************************************************/

void CZoomTool::OnLeave(CImgWnd* pImgWnd, MTI* pmti)
    {
    InvertZoomRect();
    c_zoomRect.SetRect(0, 0, 0, 0);
    }

/***************************************************************************/

void CZoomTool::OnShowDragger(CImgWnd* pImgWnd, BOOL bShow)
    {
    InvertZoomRect();
    }

/***************************************************************************/

void CZoomTool::InvertZoomRect()
    {
    if (c_zoomRect.IsRectEmpty())
        return;

    CClientDC dc(c_pImgWnd);
    CBrush* pOldBrush = (CBrush*)dc.SelectStockObject(NULL_BRUSH);
    dc.SetROP2(R2_NOT);
    CRect invertRect = c_zoomRect;
    c_pImgWnd->ImageToClient(invertRect);
    dc.Rectangle(&invertRect);
    dc.SelectObject(pOldBrush);
    }

/***************************************************************************/

void CZoomTool::OnMove(CImgWnd* pImgWnd, MTI* pmti)
    {
    if (pImgWnd->GetZoom() > 1)
        return;

    CRect viewRect;
    pImgWnd->GetClientRect(&viewRect);
    int nPrevZoom = pImgWnd->GetPrevZoom();

    CRect newZoomRect;
    CSize viewSize = viewRect.Size();
    if (viewSize.cx > pImgWnd->m_pImg->cxWidth * nPrevZoom)
        viewSize.cx = pImgWnd->m_pImg->cxWidth * nPrevZoom;
    if (viewSize.cy > pImgWnd->m_pImg->cyHeight * nPrevZoom)
        viewSize.cy = pImgWnd->m_pImg->cyHeight * nPrevZoom;
    newZoomRect.left = pmti->pt.x;
    newZoomRect.top = pmti->pt.y;
    newZoomRect.right = newZoomRect.left + viewSize.cx / nPrevZoom;
    newZoomRect.bottom = newZoomRect.top + viewSize.cy / nPrevZoom;
    newZoomRect.OffsetRect(-newZoomRect.Width() / 2,
        -newZoomRect.Height() / 2);

    int xAdjust = 0;
    int yAdjust = 0;

    if (newZoomRect.left < 0)
        xAdjust = -newZoomRect.left;
    else if ((xAdjust = pImgWnd->m_pImg->cxWidth - newZoomRect.right) > 0)
        xAdjust = 0;

    if (newZoomRect.top < 0)
        yAdjust = -newZoomRect.top;
    else if ((yAdjust = pImgWnd->m_pImg->cyHeight - newZoomRect.bottom) > 0)
        yAdjust = 0;

    newZoomRect.OffsetRect(xAdjust, yAdjust);

    if (newZoomRect != c_zoomRect)
        {
        InvertZoomRect();
        c_pImgWnd = pImgWnd;
        c_zoomRect = newZoomRect;
        InvertZoomRect();
        }
    }

/***************************************************************************/

void CZoomTool::OnStartDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    CImgTool::OnStartDrag(pImgWnd, pmti);

    c_pImgWnd = pImgWnd;
    InvertZoomRect();

    if (pImgWnd->GetZoom() == 1)
        {
        pImgWnd->SetZoom( pImgWnd->GetPrevZoom() );
        pImgWnd->CheckScrollBars();
        pImgWnd->SetScroll(-c_zoomRect.left - 1, -c_zoomRect.top - 1);
        }
    else
        {
        pImgWnd->SetZoom(1);
        pImgWnd->CheckScrollBars();
        }

    c_zoomRect.SetRect(0, 0, 0, 0);
    }

/***************************************************************************/

void CZoomTool::OnEndDrag(CImgWnd* pImgWnd, MTI* pmti)
    {
    SelectPrevious();
    CImgTool::OnEndDrag(pImgWnd, pmti);
    }

/***************************************************************************/

void CZoomTool::OnCancel(CImgWnd* pImgWnd)
    {
    InvertZoomRect();
    c_zoomRect.SetRect(0, 0, 0, 0);
    SelectPrevious();
    CImgTool::OnCancel(pImgWnd);
    }

/***************************************************************************/
