/******************************************************************************/
/* T_FHSEL.CPP: IMPLEMENTATION OF THE CFreehandSelectTool CLASS               */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Methods in this file                                                       */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgbrush.h"
#include "imgwnd.h"
#include "imgwell.h"
#include "t_fhsel.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CFreehandSelectTool, CPolygonTool )

#include "memtrace.h"

extern CSelectTool  NEAR g_selectTool;
CFreehandSelectTool NEAR g_freehandselectTool;

/******************************************************************************/

CFreehandSelectTool::CFreehandSelectTool()
    {
    m_bIsUndoable     = FALSE;
    m_nCmdID          = IDMB_PICKRGNTOOL;
    m_bCanBePrevTool  = FALSE;
    m_bFilled         = FALSE;
    m_bBorder         = FALSE;
    m_nStrokeWidth    = 1;
    m_pcRgnPoly       = &(theImgBrush.m_cRgnPolyFreeHandSel);
    m_pcRgnPolyBorder = &(theImgBrush.m_cRgnPolyFreeHandSelBorder);
    }

/******************************************************************************/

CFreehandSelectTool::~CFreehandSelectTool()
    {
    }

/******************************************************************************/

void CFreehandSelectTool::AdjustPointsForZoom(int iZoom)
    {
    int iSize = (int)m_cObArrayPoints.GetSize();
    CPoint *pcPoint;

    for (int i = 0; i < iSize; i++)
        {
        pcPoint= (CPoint *)m_cObArrayPoints.GetAt(i);
        pcPoint->x *= iZoom;
        pcPoint->y *= iZoom;
        }
    }

/******************************************************************************/

BOOL CFreehandSelectTool::CreatePolyRegion( int iZoom )
    {
    BOOL bRC = TRUE;
    CPoint *pcPointArray;

    // cleanup old region if exists
    if (m_pcRgnPoly->GetSafeHandle())
        m_pcRgnPoly->DeleteObject();

    // cleanup old region if exists
    if (m_pcRgnPolyBorder->GetSafeHandle())
        m_pcRgnPolyBorder->DeleteObject();

    bRC = CopyPointsToMemArray( &pcPointArray, &m_iNumPoints );

    if (! bRC)
        {
        theApp.SetMemoryEmergency();
        return FALSE;
        }

    bRC = m_pcRgnPoly->CreatePolygonRgn( pcPointArray, m_iNumPoints, ALTERNATE );

    delete [] pcPointArray;

    if (! bRC)  // offset for selection boundary
        {
        theApp.SetGdiEmergency();
        return FALSE;
        }

    m_pcRgnPoly->OffsetRgn( -m_cRectBounding.left,
                            -m_cRectBounding.top );
    //
// This adjustment appears to be unnecessary. removed it 5/1/1997
//    AdjustPointsForZoom( iZoom );

    bRC = CopyPointsToMemArray( &pcPointArray, &m_iNumPoints );

    if (bRC)
        {
        bRC = m_pcRgnPolyBorder->CreatePolygonRgn( pcPointArray, m_iNumPoints, ALTERNATE );

        delete [] pcPointArray;

        if (bRC) // offset for selection boundary
            m_pcRgnPolyBorder->OffsetRgn( -(m_cRectBounding.left * iZoom),
                                          -(m_cRectBounding.top  * iZoom) );
        }
    if (! bRC)
        m_pcRgnPoly->DeleteObject();

    return bRC;
    }

/******************************************************************************/

BOOL CFreehandSelectTool::CreatePolyRegion( int iZoom, LPPOINT lpPoints, int iPoints )
    {
    if (! lpPoints || iPoints < 3)
        return FALSE;

    DeleteArrayContents();

    TRY {
        CPoint* pPt;

        for (int i = 0; i < iPoints; i++)
            {
            pPt = new CPoint( lpPoints[i] );

            m_cObArrayPoints.Add( (CObject *)pPt );
            }
        }
    CATCH( CMemoryException, e )
        {
        DeleteArrayContents();

        theApp.SetMemoryEmergency();

        return FALSE;
        }
    END_CATCH

    m_iNumPoints = iPoints;

    AdjustBoundingRect();

    rcPrev = m_cRectBounding;
    m_bMultPtOpInProgress = FALSE;

    theImgBrush.m_bMakingSelection = FALSE;
    theImgBrush.m_bMoveSel         = FALSE;
    theImgBrush.m_bSmearSel        = FALSE;

    if (! CreatePolyRegion( iZoom ))
        return FALSE;

    return TRUE;
    }

/******************************************************************************/

BOOL CFreehandSelectTool::ExpandPolyRegion( int iNewSizeX, int iNewSizeY )
    {
    CPoint* pcPointArray;
    int    iNumPts;

    if (! CopyPointsToMemArray( &pcPointArray, &iNumPts ))
        return FALSE;

    int iWidth  = m_cRectBounding.Width()  + 1;
    int iHeight = m_cRectBounding.Height() + 1;
    int iDeltaX = ((iNewSizeX - iWidth ) * 10) / iWidth;
    int iDeltaY = ((iNewSizeY - iHeight) * 10) / iHeight;

    CPoint* pPtArray = pcPointArray;
    int     iPts     = iNumPts;

    while (iPts--)
        {
        pPtArray->x = (((pPtArray->x * 10) + (pPtArray->x * iDeltaX)) + 5) / 10;
        pPtArray->y = (((pPtArray->y * 10) + (pPtArray->y * iDeltaY)) + 5) / 10;

        pPtArray++;
        }

    BOOL bReturn = CreatePolyRegion( CImgWnd::GetCurrent()->GetZoom(),
                                     pcPointArray, iNumPts );
    delete [] pcPointArray;

    return bReturn;
    }

/******************************************************************************/
/* This routine is called before rendering onto the DC.  It basically, calls  */
/* the default setup to setup the pen and brush, and then overrides the Pen if*/
/* drawing in progress and drawing without any border.  This case is necessary*/
/* since if you do not have a border, you need to see something during the in */
/* progress drawing mode.  It uses the inverse (not) of the screen color as   */
/* the border in this mode.                                                   */

BOOL CFreehandSelectTool::SetupPenBrush(HDC hDC, BOOL bLeftButton, BOOL bSetup, BOOL bCtrlDown)
    {
    static int iOldROP2Code;
    static BOOL bCurrentlySetup = FALSE;

    m_nStrokeWidth = 1;  // override any changes

    BOOL bRC = CClosedFormTool::SetupPenBrush(hDC, bLeftButton, bSetup, bCtrlDown);

    // for multipt operations in progress (e.g. drawing outline, not fill yet
    // if there is no border, use the not of the screen color for the border.
    // When bMultiptopinprogress == FALSE, final drawing, we will use a null
    // pen and thus have no border.
    if (bSetup)
        {
        if (bCurrentlySetup)
            bRC = FALSE;
        else
            {
            bCurrentlySetup = TRUE;
            iOldROP2Code = SetROP2(hDC, R2_NOT);
            }
        }
    else
        {
        if (bCurrentlySetup)
            {
            bCurrentlySetup = FALSE;

            // if no border, restore drawing mode
            SetROP2(hDC, iOldROP2Code);
            }
        else
            // Error: Cannot Free/cleanup Brush/Pen -- Never allocated.
            bRC = FALSE;
        }

    return bRC;
    }

/******************************************************************************/
/* Call the line's adjustpointsforconstraint member function                  */

void CFreehandSelectTool::AdjustPointsForConstraint(MTI *pmti)
    {
    CClosedFormTool::AdjustPointsForConstraint(pmti);
    }

/******************************************************************************/
// ptDown must be anchor point for our line, not where we did mouse button down

void CFreehandSelectTool::PreProcessPoints(MTI *pmti)
    {
    CClosedFormTool::PreProcessPoints(pmti);
    }

/***************************************************************************/

void CFreehandSelectTool::OnPaintOptions ( CDC* pDC,
                                           const CRect& paintRect,
                                           const CRect& optionsRect )
    {
    g_selectTool.OnPaintOptions( pDC, paintRect, optionsRect );
    }

/******************************************************************************/

void CFreehandSelectTool::OnClickOptions ( CImgToolWnd* pWnd,
                                           const CRect& optionsRect,
                                           const CPoint& clickPoint )
    {
    g_selectTool.OnClickOptions(pWnd, optionsRect, clickPoint);
    }

/******************************************************************************/

void CFreehandSelectTool::OnStartDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    HideBrush();
    OnActivate( FALSE );
//  CommitSelection( TRUE );

    pImgWnd->EraseTracker();
    theImgBrush.m_bMakingSelection = TRUE;

    // simulate multipt op in progress, until button up or asked.  This will
    // allow us to draw differently for duration and end.
    m_bMultPtOpInProgress = TRUE;

    DeleteArrayContents();

    CClosedFormTool::OnStartDrag( pImgWnd, pmti );
    }

/******************************************************************************/

void CFreehandSelectTool::OnEndDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    int iZoom = pImgWnd->GetZoom();

    theImgBrush.m_bMakingSelection = FALSE;
    theImgBrush.m_bMoveSel         = theImgBrush.m_bSmearSel = FALSE;

    OnDrag(pImgWnd, pmti); // one last time to refresh display in prep for final render

    Render( CDC::FromHandle(pImgWnd->m_pImg->hDC), m_cRectBounding, pmti->fLeft, TRUE, pmti->fCtrlDown );

    m_iNumPoints = (int)m_cObArrayPoints.GetSize();

    if (m_iNumPoints > 2)
        if (! CreatePolyRegion( iZoom ))
            return;

    if (pmti->ptDown.x == pmti->pt.x
    &&  pmti->ptDown.y == pmti->pt.y)
        {
        if (m_iNumPoints > 3) // 3 is min points.  If click down/up get 2
            {
            // must fool selectTool.OnEndDrag to think width of selection is
            // greater than 0.  If 0, thinks selection is done/place it (i.e.
            // just clicked down/up.  We only do this if the end point is the
            // same as the beginning point.  This case will have width=height=0,
            // but number of points > 2
            pmti->pt.x++;
            pmti->pt.y++;
            }
        }

    pmti->ptDown = m_cRectBounding.TopLeft();
    pmti->pt     = m_cRectBounding.BottomRight();

    g_selectTool.OnEndDrag(pImgWnd, pmti);
    }

/******************************************************************************/

void CFreehandSelectTool::OnDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    // Must set rcPrev to m_cRectBoundingRect prior to calling SetCurrentPoint
    // Since SetCurrentPoint will adjust m_cRectBounding, and we want the
    // previous bounding rect.
    rcPrev = m_cRectBounding;

    if (pmti->pt.x > pImgWnd->m_pImg->cxWidth)
        pmti->pt.x = pImgWnd->m_pImg->cxWidth;

    if (pmti->pt.y > pImgWnd->m_pImg->cyHeight)
        pmti->pt.y = pImgWnd->m_pImg->cyHeight;

    if (pmti->pt.x < 0)
        pmti->pt.x = 0;

    if (pmti->pt.y < 0)
        pmti->pt.y = 0;

    TRY {
        AddPoint(pmti->pt);
        }

    CATCH(CMemoryException,e)
        {
        theApp.SetMemoryEmergency();
        return;
        }
    END_CATCH

    CClosedFormTool::OnDrag(pImgWnd, pmti);
    }

/******************************************************************************/

void CFreehandSelectTool::OnCancel(CImgWnd* pImgWnd)
    {
    // We were not selecting or dragging, just cancel the select tool...
    CommitSelection( TRUE );

    //render one last time to turn off/invert the line if any drawn
        if (theImgBrush.m_bMakingSelection)
        {
                Render( CDC::FromHandle( pImgWnd->m_pImg->hDC ), m_cRectBounding,
                        TRUE, TRUE, FALSE );
        }
    theImgBrush.TopLeftHandle();

    g_bCustomBrush = FALSE;
    theImgBrush.m_pImg             = NULL;
    theImgBrush.m_bMoveSel         = FALSE;
    theImgBrush.m_bSmearSel        = FALSE;
    theImgBrush.m_bMakingSelection = FALSE;

    InvalImgRect( pImgWnd->m_pImg, NULL );

    DeleteArrayContents();

    CPolygonTool::OnCancel(pImgWnd);
    }

/***************************************************************************/

BOOL CFreehandSelectTool::IsToolModal(void)
{
        if (theImgBrush.m_pImg)
        {
                return(TRUE);
        }

        return(CPolygonTool::IsToolModal());
}

/******************************************************************************/

void CFreehandSelectTool::OnActivate(BOOL bActivate)
    {
    g_selectTool.OnActivate(bActivate);
    }

/******************************************************************************/
/* this class really isn't a multipt operation, but is derived from one thus  */
/* we can always end the multipt operation if anyone asks                     */

BOOL CFreehandSelectTool::CanEndMultiptOperation(MTI* pmti )
    {
    m_bMultPtOpInProgress = FALSE;
    return (CClosedFormTool::CanEndMultiptOperation(pmti));
    }

/******************************************************************************/
