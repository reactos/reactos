/******************************************************************************/
/* T_POLY.CPP: IMPLEMENTATION OF THE CPolygonTool CLASS                       */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Methods in this file                                                       */
/*                                                                            */
/*  Polygon Tool Class Object                                                 */
/*     CPolygonTool::CPolygonTool                                             */
/*     CPolygonTool::~CPolygonTool                                            */
/*     CPolygonTool::DeleteArrayContents                                      */
/*     CPolygonTool::AdjustBoundingRect                                       */
/*     CPolygonTool::CopyPointsToMemArray                                     */
/*     CPolygonTool::AddPoint                                                 */
/*     CPolygonTool::SetCurrentPoint                                          */
/*     CPolygonTool::RenderInProgress                                         */
/*     CPolygonTool::RenderFinal                                              */
/*     CPolygonTool::SetupPenBrush                                            */
/*     CPolygonTool::AdjustPointsForConstraint                                */
/*     CPolygonTool::PreProcessPoints                                         */
/*     CPolygonTool::Render                                                   */
/*     CPolygonTool::OnStartDrag                                              */
/*     CPolygonTool::OnEndDrag                                                */
/*     CPolygonTool::OnDrag                                                   */
/*     CPolygonTool::OnCancel                                                 */
/*     CPolygonTool::CanEndMultiptOperation                                   */
/*     CPolygonTool::EndMultiptOperation                                      */
/******************************************************************************/
/*                                                                            */
/* Briefly, This object stores the points of the polygon in a CObArray of     */
/* CPoint Objects.  For the in progress drawing, it calls PolyLine.  When the */
/* polygon is closed or completed (by the user doubleclicking => asking us to */
/* close it), Polygon is called on the same points.                           */
/*                                                                            */
/* The last point in the array of points is always the point the current line */
/* is being drawn to.  The first time 2 points are added (the Anchor/first    */
/* point, and the point the line is being drawn to) It does happen that this  */
/* first time, they are the same point.  It is necessary that the first time  */
/* 2 points are added, since subsequent times, new points are not added, but  */
/* the last point is reset.                                                   */
/*                                                                            */
/******************************************************************************/
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgwell.h"
#include "t_poly.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CPolygonTool, CClosedFormTool)

#include "memtrace.h"

extern CLineTool NEAR g_lineTool;

CPolygonTool     NEAR g_polygonTool;

extern MTI NEAR mti;

/******************************************************************************/

CPolygonTool::CPolygonTool()
    {
    m_nCmdID              = IDMB_POLYGONTOOL;
    m_cRectBounding.SetRectEmpty();
    m_bMultPtOpInProgress = FALSE;
    m_nStrokeWidth        = 1;
    }

/******************************************************************************/

CPolygonTool::~CPolygonTool()
    {
    m_cRectBounding.SetRectEmpty();
    DeleteArrayContents();
    }

/******************************************************************************/
/* delete all cpoint objects allocated and stored in the array                */
/* also free any memory associated with the array                             */

void CPolygonTool::DeleteArrayContents(void)
    {
    int iSize = (int)m_cObArrayPoints.GetSize();

    CPoint *pcPoint;

    for (int i = 0; i < iSize; i++)
        {
        pcPoint= (CPoint *)m_cObArrayPoints.GetAt( i );
        delete pcPoint;
        }
    m_cObArrayPoints.RemoveAll();
    }

/******************************************************************************/
/* recalculate the bounding rectangle for the polyline/polygon                */

void CPolygonTool::AdjustBoundingRect(void)
    {
    int iSize = (int)m_cObArrayPoints.GetSize();
    CPoint *pcPoint;
    int iStrokeWidth = GetStrokeWidth();
    int i;

    if (iSize >= 1)
        {
        pcPoint= (CPoint*)m_cObArrayPoints.GetAt( 0 );
        //set the rect to equal the 1st value
        m_cRectBounding.SetRect(pcPoint->x, pcPoint->y, pcPoint->x, pcPoint->y);
        }

    for (i = 1; i < iSize; i++)
        {
        pcPoint = (CPoint *)m_cObArrayPoints.GetAt( i );

        m_cRectBounding.SetRect( min( pcPoint->x, m_cRectBounding.left   ),
                                 min( pcPoint->y, m_cRectBounding.top    ),
                                 max( pcPoint->x, m_cRectBounding.right  ),
                                 max( pcPoint->y, m_cRectBounding.bottom ) );
        }
    // Adjust for width of current drawing line/border
    m_cRectBounding.OffsetRect ( -(iStrokeWidth / 2), -(iStrokeWidth / 2) );
    m_cRectBounding.InflateRect(   iStrokeWidth     ,   iStrokeWidth);
    }

/******************************************************************************/
/* This method will copy the CObArray structure of CPoints to a contiguous    */
/* memory block of CPoint Structures                                          */

BOOL CPolygonTool::CopyPointsToMemArray(CPoint **pcPoint, int *piNumElements)
    {
    BOOL bRC = TRUE;
    int i;
    int iSize = (int)m_cObArrayPoints.GetSize();

    if (! iSize)
        {
        *piNumElements = 0;
        *pcPoint = NULL;
        return TRUE;
        }
    TRY
        {
        *pcPoint = new CPoint[iSize];

        if (*pcPoint == NULL)
            {
            AfxThrowMemoryException();
            }

        for (i=0; i < iSize; i++)
            {
            (*pcPoint)[i] = *((CPoint*) (m_cObArrayPoints[i]));
            }

        *piNumElements = iSize;
        }

    CATCH(CMemoryException,e)
        {
        *piNumElements = 0;
        bRC = FALSE;
        }

    END_CATCH

    return bRC;
    }

/******************************************************************************/
/* This routine can Throw a CMemoryException!!                                */
/* It adds a new point to the end of the array, possibly increasing the size  */

void CPolygonTool::AddPoint(POINT ptNewPoint)
    {

    CPoint *pcPoint;

    pcPoint = new CPoint(ptNewPoint);
    if (pcPoint == NULL)
        {
        AfxThrowMemoryException();
        }

    m_cObArrayPoints.Add((CObject *)pcPoint);
    AdjustBoundingRect();
    }

/******************************************************************************/
/* This method changes the value of the last point in the array.  It does not */
/* remove the point and add a new one.  It just modifies it in place          */

void CPolygonTool::SetCurrentPoint(POINT ptNewPoint)
    {
    int iLast = (int)m_cObArrayPoints.GetUpperBound();

    if (iLast >= 0)
        {
        CPoint *pcPoint = (CPoint *) m_cObArrayPoints[iLast];

        pcPoint->x = ptNewPoint.x;
        pcPoint->y = ptNewPoint.y;

        AdjustBoundingRect();
        }
    }

/******************************************************************************/
/* Render In Progress is called for all drawing during the multi-pt operation */
/* The only difference between this method and RenderFinal is that it calls   */
/* polyline and RenderFinal calls polygon.                                    */

void CPolygonTool::RenderInProgress(CDC* pDC)
    {

    CPoint *pcPointArray;
    int     iNumElements;



    if (CopyPointsToMemArray( &pcPointArray, &iNumElements ))
        {
        pDC->Polyline(pcPointArray, iNumElements);

        delete [] pcPointArray;
        }
    }

/******************************************************************************/
/* Render Final is called at the end of the multi-pt drawing mode.  The only  */
/* difference between this method and RenderInProgress is that it calls       */
/* polygon and RenderInProgress calls polyline.                               */

void CPolygonTool::RenderFinal(CDC* pDC)
    {

    CPoint *pcPointArray;
    int     iNumElements;


    if (CopyPointsToMemArray(&pcPointArray, &iNumElements))
        {
        // Remove RIP with only 2 points
        if (iNumElements > 2)
            pDC->Polygon(pcPointArray, iNumElements);
        delete [] pcPointArray;
        }

    }

/******************************************************************************/
/* This routine is called before rendering onto the DC.  It basically, calls  */
/* the default setup to setup the pen and brush, and then overrides the Pen if*/
/* drawing in progress and drawing without any border.  This case is necessary*/
/* since if you do not have a border, you need to see something during the in */
/* progress drawing mode.  It uses the inverse (not) of the screen color as   */
/* the border in this mode.                                                   */

BOOL CPolygonTool::SetupPenBrush(HDC hDC, BOOL bLeftButton, BOOL bSetup, BOOL bCtrlDown)
    {
    static int  iOldROP2Code;
    static BOOL bCurrentlySetup = FALSE;

    BOOL bRC = CClosedFormTool::SetupPenBrush(hDC, bLeftButton, bSetup, bCtrlDown);

    // for multipt operations in progress (e.g. drawing outline, not fill yet
    // if there is no border, use the not of the screen color for the border.
    // When bMultiptopinprogress == FALSE, final drawing, we will use a null
    // pen and thus have no border.
    if (m_bMultPtOpInProgress)
        {
        if (bSetup)
            {
            if (! bCurrentlySetup)
               {
               bCurrentlySetup = TRUE;

               // if no border, draw inprogress border as inverse of screen color
               if (! m_bBorder)
                   iOldROP2Code = SetROP2(hDC, R2_NOT);
               }
            else
                // Error: Will lose allocated Brush/Pen
                bRC = FALSE;
            }
        else
            {
            if (bCurrentlySetup)
                {
                bCurrentlySetup = FALSE;

                // if no border, restore drawing mode
                if (! m_bBorder)
                    SetROP2(hDC, iOldROP2Code);
                }
            else
                // Error: Cannot Free/cleanup Brush/Pen -- Never allocated.
                bRC = FALSE;
            }
        }

    return bRC;
    }

/******************************************************************************/
/* Call the line's adjustpointsforconstraint member function                  */
void CPolygonTool::AdjustPointsForConstraint(MTI *pmti)
    {
    g_lineTool.AdjustPointsForConstraint(pmti);
    }

/******************************************************************************/
// ptDown must be anchor point for our line, not where we did mouse button down

void CPolygonTool::PreProcessPoints(MTI *pmti)
    {
    int iLast = (int)m_cObArrayPoints.GetUpperBound();

    if (iLast > 0)
        iLast--;

    CPoint* pcPoint;

    if (iLast >= 0)
        {
        pcPoint = (CPoint *)m_cObArrayPoints[iLast];
        pmti->ptDown = *pcPoint;
        }
    CClosedFormTool::PreProcessPoints(pmti);
    }

/******************************************************************************/
/* Render sets up the pen and brush, and then calls either RenderInProgress   */
/* or RenderFinal.  RenderInProgress is called if in the middle of a multipt  */
/* operation, and RenderFinal is called when a multipt operation is complete  */
/* The pen and brush is set up exactly the same as the parent routine in      */
/* CRubberTool */

void CPolygonTool::Render(CDC* pDC, CRect& rect, BOOL bDraw, BOOL bCommit, BOOL bCtrlDown)
    {
    // Setup Pen/Brush
    SetupPenBrush(pDC->m_hDC, bDraw, TRUE, bCtrlDown);

    if (m_bMultPtOpInProgress)
        {
        RenderInProgress(pDC);
        }
    else
        {
        RenderFinal(pDC);
        }
    // Cleanup Pen/Brush
    SetupPenBrush(pDC->m_hDC, bDraw,  FALSE, bCtrlDown);

    // Need to return the bounding rect
    rect = m_cRectBounding;
    }

/******************************************************************************/

void CPolygonTool::OnActivate( BOOL bActivate )
    {
    if (! bActivate && m_bMultPtOpInProgress)
        {
        if (m_pImgWnd != NULL)
            if (m_cObArrayPoints.GetSize() > 1)
                {
                OnStartDrag( m_pImgWnd, &m_MTI );
                OnEndDrag  ( m_pImgWnd, &m_MTI );

                m_MTI.ptPrev = m_MTI.pt;

                EndMultiptOperation(); // end the multipt operation

                OnEndDrag( m_pImgWnd, &m_MTI );

                mti.fLeft  = FALSE;
                mti.fRight = FALSE;
                }
            else
                OnCancel( m_pImgWnd );
        else
            EndMultiptOperation( TRUE );
        }
    m_pImgWnd = NULL;

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

void CPolygonTool::OnEnter( CImgWnd* pImgWnd, MTI* pmti )
    {
    m_pImgWnd = NULL;
    }

/******************************************************************************/

void CPolygonTool::OnLeave( CImgWnd* pImgWnd, MTI* pmti )
    {
    m_pImgWnd = pImgWnd;
    }

/******************************************************************************/
/* On Start Drag is called on mouse button down.  We basically call on Start  */
/* Drag of the parent (default) class after adding in our point(s) into the   */
/* array of points.  If this is the first point (i.e. bMultiptOpInProgress == */
/* False, then we need 2 points in our array, and we can call the default     */
/* OnStartDrag.  If it is not the first point, then we just add the new point */
/* and call our OnDrag.  In either case, OnDrag is called which eventually    */
/* calls render to do our drawing on the mouse down                           */

void CPolygonTool::OnStartDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    TRY {
        if (m_bMultPtOpInProgress)
            {
            CRect rect;

            CPoint pt = pmti->pt;

            pImgWnd->ImageToClient( pt );
            pImgWnd->GetClientRect( &rect );

            if (rect.PtInRect( pt ))
                {
                AddPoint( pmti->pt );
                OnDrag( pImgWnd, pmti );
                }
            }
        else
            {
            DeleteArrayContents();
            m_cRectBounding.SetRectEmpty();

            AddPoint( pmti->pt );
            // must set m_bmultptopinprogress prior to calling onstartdrag
            // since that calls render,and render will call renderinprogress
            // or renderfinal depending on the sate of this variable.
            m_bMultPtOpInProgress = TRUE;
            // No Mult Pt In Progress => 1st Click
            //
            // add a 2nd point, last point is what we are draing to
            // 1st point is anchor.  1st time, need 2 points to draw a line
            // subsequent times, just re-use last point as anchor and only one
            // more point is added (above outside test for m_bmultptopinprogress)
            AddPoint( pmti->pt );
            CClosedFormTool::OnStartDrag( pImgWnd, pmti );
            }
        }

    CATCH(CMemoryException,e)
        {
        }

    END_CATCH
    }

/******************************************************************************/
/* On End Drag is sent on a mouse button up.  This basically is a clone of the*/
/* CRubberTool::OnEndDrag method, except that we use our bounding rect for all*/
/* the image invalidation, and commit, and undo function calls.               */
/* if we are in the middle of a multipoint operation, we do not want to call  */
/* all the routines to fix the drawing (e.g. invalImgRect, CommitImgRect,     */
/* FinishUndo).  We just want to save the current point, render, and return   */

void CPolygonTool::OnEndDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    PreProcessPoints(pmti);
    SetCurrentPoint(pmti->pt);

    if (m_bMultPtOpInProgress)
        {
        m_MTI = *pmti;
        // can't call OnDrag for this object/class, since it calls preprocesspt
        // again, and then onDrag.  If you call preproces again, you will lose
        // bounding rectange box prev, and not be able to invalidate / repaint
        // Still have to invalidate bounding rect, since if rect is larger than
        // current rect, must invalidate to paint. E.g. If let off shift, then
        // let off button, end point would be adjusted and bouning rect would
        // also be correct, but rect calculated in CClosedFormTool::OnDrag is
        // incorrect.
        InvalImgRect(pImgWnd->m_pImg, &m_cRectBounding);

        CClosedFormTool::OnDrag(pImgWnd, pmti);
        return;
        }


    if (! m_cObArrayPoints.GetSize())
        return;

    OnDrag(pImgWnd, pmti); // one last time to refresh display in prep for final render
    Render(CDC::FromHandle(pImgWnd->m_pImg->hDC), m_cRectBounding, pmti->fLeft, TRUE, pmti->fCtrlDown);
    InvalImgRect(pImgWnd->m_pImg, &m_cRectBounding);
    CommitImgRect(pImgWnd->m_pImg, &m_cRectBounding);
    pImgWnd->FinishUndo(m_cRectBounding);

    ClearStatusBarSize();

    CImgTool::OnEndDrag(pImgWnd, pmti);
    }

/******************************************************************************/
/* On Drag is sent when the mouse is moved with the button down.  We basically*/
/* save the current point, and call the base class processing.  Since the base*/
/* class processing invalidates the rect on the screen and cleans it up so we */
/* can paint a new line, we have to adjust the previous rectangle to be the   */
/* bounding rectangle of our polyline.  If we did not do this, our previous   */
/* drawing would not get erased, and we would be drawing our new line over    */
/* part of the previous line.  The default processing finally calls Render    */
/* which since our render is virtual, will call our render method above.      */

void CPolygonTool::OnDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    PreProcessPoints    ( pmti     );
    SetCurrentPoint     ( pmti->pt );
    SetStatusBarPosition( pmti->pt );
    SetStatusBarSize    ( m_cRectBounding.Size() );

    CClosedFormTool::OnDrag(pImgWnd, pmti);
    }

/******************************************************************************/
/* On Cancel is sent when the user aborts an operation while in progress      */
/* EndMultiptOperation with TRUE will do all our cleanup                      */

void CPolygonTool::OnCancel(CImgWnd* pImgWnd)
    {
    InvalImgRect( pImgWnd->m_pImg, &m_cRectBounding );
    EndMultiptOperation(TRUE);
    CClosedFormTool::OnCancel(pImgWnd);
    }

/******************************************************************************/
/* If point is on 1st point (i.e. closes the polygon) then can end is true    */
// Use the stroke width to determine the width of the line and whether the    */
/* end point touches the beginning point because of the line thickness        */

BOOL CPolygonTool::CanEndMultiptOperation(MTI* pmti )
    {
    CPoint *pcPoint = (CPoint *) m_cObArrayPoints[0];

    CSize cSizeDiff = (*pcPoint) - pmti->pt;

    int iStrokeWidth = GetStrokeWidth() * 2;

    m_bMultPtOpInProgress = ! ((abs( cSizeDiff.cx ) <= iStrokeWidth)
                            && (abs( cSizeDiff.cy ) <= iStrokeWidth));
    return ( TRUE );
    }

/******************************************************************************/
/* If bAbort is true, this means an error occurred, or the user cancelled the */
/* multipoint operation in the middle of it.  We need to clean up the         */
/* allocated memory in our array of points.                                   */

void CPolygonTool::EndMultiptOperation( BOOL bAbort )
    {
    if (bAbort)
        {
        DeleteArrayContents();
        }

    CClosedFormTool::EndMultiptOperation();
    }

/******************************************************************************/

void CPolygonTool::OnUpdateColors( CImgWnd* pImgWnd )
    {
    if (m_cObArrayPoints.GetSize() && m_bMultPtOpInProgress)
        {
        OnStartDrag( pImgWnd, &m_MTI );
        OnEndDrag  ( pImgWnd, &m_MTI );
        }
    }

/******************************************************************************/
