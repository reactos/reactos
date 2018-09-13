/******************************************************************************/
/* T_CURVE.CPP: IMPLEMENTATION OF THE CCurveTool CLASS                        */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Methods in this file                                                       */
/*                                                                            */
/*  CCurve Tool Class Object                                                  */
/*     CCurveTool::CCurveTool                                                 */
/*     CCurveTool::~CCurveTool                                                */
/*     CCurveTool::AdjustBoundingRect                                         */
/*     CCurveTool::AddPoint                                                   */
/*     CCurveTool::SetCurrentPoint                                            */
/*     CCurveTool::DrawCurve                                                  */
/*     CCurveTool::AdjustPointsForConstraint                                  */
/*     CCurveTool::PreProcessPoints                                           */
/*     CCurveTool::Render                                                     */
/*     CCurveTool::OnStartDrag                                                */
/*     CCurveTool::OnEndDrag                                                  */
/*     CCurveTool::OnDrag                                                     */
/*     CCurveTool::OnCancel                                                   */
/*     CCurveTool::CanEndMultiptOperation                                     */
/*     CCurveTool::EndMultiptOperation                                        */
/******************************************************************************/
/*                                                                            */
/* Briefly, this Object draws a curve from 4 (currently) points. It generates */
/* a list of points which are placed in the array, and then calls polyline    */
/* to draw line segments to build a curve.                                    */
/*                                                                            */
/* The array is divided into 2 pieces.  The first piece is the anchor points, */
/* the 2nd piece is the array of points which will be passed to polyline.     */
/* The anchor points are placed in the array in the following order           */
/* 2,3,4,...1. See the addpoint method below for info on this order.          */
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
#include "pbrusvw.h"
#include "t_curve.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CCurveTool, CRubberTool)

#include "memtrace.h"

extern CLineTool NEAR g_lineTool;

CCurveTool       NEAR g_curveTool;


/******************************************************************************/
CCurveTool::CCurveTool()
    {
    m_nCmdID = IDMB_CURVETOOL;
    m_iNumAnchorPoints = 0;
    m_cRectBounding.SetRectEmpty();
    }

/******************************************************************************/
CCurveTool::~CCurveTool()
    {
    m_cRectBounding.SetRectEmpty();
    }

/******************************************************************************/
/* recalculate the bounding rectangle for the polyline/curve                  */
void CCurveTool::AdjustBoundingRect(void)
    {
    int iStrokeWidth = GetStrokeWidth();
    int i;

    if (m_iNumAnchorPoints >= 1)
        {
        //set the rect to equal the 1st value
        m_cRectBounding.SetRect(m_PolyPoints[0].x, m_PolyPoints[0].y,
                                m_PolyPoints[0].x, m_PolyPoints[0].y);
        }

    for (i=1; i < m_iNumAnchorPoints; i++)
        {
        m_cRectBounding.SetRect( min(m_PolyPoints[i].x, m_cRectBounding.left),
                                 min(m_PolyPoints[i].y, m_cRectBounding.top),
                                 max(m_PolyPoints[i].x, m_cRectBounding.right),
                                 max(m_PolyPoints[i].y, m_cRectBounding.bottom));
        }

        // Adjust rectangle for Windows GDI (Non-inclusive right/bottom)
        m_cRectBounding.bottom++; m_cRectBounding.right++;

    // Adjust for width of current drawing line/border
    m_cRectBounding.OffsetRect(-(iStrokeWidth/2),-(iStrokeWidth/2));
    m_cRectBounding.InflateRect(iStrokeWidth, iStrokeWidth);
    }
/******************************************************************************/
// This method adds a new point into the array and increases the number of
// anchor points currently in the array.  If there  are no points in the
// array, it adds a point to the 1st position (index 0).  If there are any
// points currently in the array, it copies the last point to the new
// location, and then adds the new point where the old last point was (1 point
// before the last point.  The 1st point added is always the last point in the
// array, and the  2nd point added is always the 1st point.
// The order of the points in the array are: 2,3,4,....,1
void CCurveTool::AddPoint(POINT ptNewPoint)
    {
    BOOL bRC = TRUE;

    if (m_iNumAnchorPoints == 0)
        {
        m_PolyPoints[m_iNumAnchorPoints] = ptNewPoint;
        m_iNumAnchorPoints++;
        }
    else
        {
        if (m_iNumAnchorPoints < MAX_ANCHOR_POINTS)
            {
            m_PolyPoints[m_iNumAnchorPoints] = m_PolyPoints[m_iNumAnchorPoints-1];
            m_PolyPoints[m_iNumAnchorPoints-1] = ptNewPoint;
            m_iNumAnchorPoints++;
            }
        }


    AdjustBoundingRect();
    }
/******************************************************************************/
// This method changes the value of the last point in the array.  If there are
// 2 points, then it modifies the 2nd point, knowing that when there are only
// 2 points, we are draing a straight line (between 2 points).  If there are
// more than 2 points, it modified the second to last point in the array,
// which is actually the last point dropped/placed.  See above for expl of
// order of points in the array.
void CCurveTool::SetCurrentPoint(POINT ptNewPoint)
    {
    if (m_iNumAnchorPoints == 2)
        {
        m_PolyPoints[m_iNumAnchorPoints-1] = ptNewPoint;
        }
    else
        {
        if (m_iNumAnchorPoints > 2)
            {
            m_PolyPoints[m_iNumAnchorPoints-2] = ptNewPoint;
            }
        }
    AdjustBoundingRect();
    }
/******************************************************************************/
BOOL CCurveTool::DrawCurve(CDC* pDC)
    {
                POINT ptCurve[MAX_ANCHOR_POINTS];
                UINT uPoints = m_iNumAnchorPoints;
                int i;

                for (i=uPoints-1; i>=0; --i)
                {
                        ptCurve[i] = m_PolyPoints[i];
                }

                // HACK: PolyBezier cannot handle 3 points, so repeat the middle point
                if (uPoints == 3)
                {
                        ptCurve[3] = ptCurve[2];
                        ptCurve[2] = ptCurve[1];
                        uPoints = 4;
                }

                PolyBezier(pDC->m_hDC, ptCurve, uPoints);

                return(TRUE);
    }
/******************************************************************************/
/* Call the line's adjustpointsforconstraint member function                  */
/* only do this if there are 2 points (i.e. drawing a straight line           */
void CCurveTool::AdjustPointsForConstraint(MTI *pmti)
    {
    if (m_iNumAnchorPoints == 2)
        {
        g_lineTool.AdjustPointsForConstraint(pmti);
        }
    }

/******************************************************************************/
// ptDown must be anchor point for our line, not where we did mouse button down
// on a subsequent point in the multipt operation
void CCurveTool::PreProcessPoints(MTI *pmti)
    {
    pmti->ptDown = m_PolyPoints[0];
    CRubberTool::PreProcessPoints(pmti);
    }

/******************************************************************************/
/* Render sets up the pen and brush, and then calls either Render             */
/* The pen and brush is set up exactly the same as the parent routine in      */
/* CRubberTool.  If there are only 2 points, do the standard line drawing     */
/* using moveto and lineto, instead of trying to create a curve between 2 pts */

void CCurveTool::Render(CDC* pDC, CRect& rect, BOOL bDraw, BOOL bCommit, BOOL bCtrlDown)
    {
    // Setup Pen/Brush
    SetupPenBrush( pDC->m_hDC, bDraw, TRUE, bCtrlDown );

    if (m_iNumAnchorPoints == 2)
        {
        pDC->MoveTo( m_PolyPoints[0].x, m_PolyPoints[0].y );
        pDC->LineTo( m_PolyPoints[1].x, m_PolyPoints[1].y );
        }
    else
        {
        if (m_iNumAnchorPoints > 2)
            {
            DrawCurve( pDC );
            }
        }
    // Cleanup Pen/Brush
    SetupPenBrush( pDC->m_hDC, bDraw,  FALSE, bCtrlDown );

    // Need to return the bounding rect
    rect = m_cRectBounding;
    }

void CCurveTool::OnActivate( BOOL bActivate )
{
        if (!bActivate && m_bMultPtOpInProgress)
        {
                CPBView* pView = (CPBView*)((CFrameWnd*)AfxGetMainWnd())->GetActiveView();

                // Stolen from CPBView::OnEscape
                // I don't think this can ever be NULL, but just in case
                if (pView->m_pImgWnd != NULL)
                {
                        pView->m_pImgWnd->CmdCancel();
                }
        }

        CRubberTool::OnActivate( bActivate );
}


/******************************************************************************/
/* On Start Drag is called on mouse button down.  We basically call on Start  */
/* Drag of the parent (default) class after adding in our point(s) into the   */
/* array of points.  If this is the first point (i.e. bMultiptOpInProgress == */
/* False, then we need 2 points in our array, and we can call the default     */
/* OnStartDrag.  If it is not the first point, then we just add the new point */
/* and call our OnDrag.  In either case, OnDrag is called which eventually    */
/* calls render to do our drawing on the mouse down                           */
/* We only call the parent OnStartDrag  the first time, because it does some  */
/* setup which we do not want done each time                                  */
void CCurveTool::OnStartDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    if (m_bMultPtOpInProgress)
        {
        AddPoint(pmti->pt);
        OnDrag(pImgWnd, pmti);
        }
    else
        {
        // must reset numAnchorPoints before calling addpoint the 1st time.
        m_iNumAnchorPoints = 0;
        AddPoint(pmti->pt);
        m_bMultPtOpInProgress = TRUE;
        // No Mult Pt In Progress => 1st Click
        //
        // add a 2nd point, last point is what we are draing to
        // 1st point is anchor.  1st time, need 2 points to draw a line
        // subsequent times, just add 1 point in array of points.
        AddPoint(pmti->pt);
        CRubberTool::OnStartDrag(pImgWnd, pmti);
        }

    }
/******************************************************************************/
/* On End Drag is sent on a mouse button up.  This basically is a clone of the*/
/* CRubberTool::OnEndDrag method, except that we use our bounding rect for all*/
/* the image invalidation, and commit, and undo function calls.               */
/* if we are in the middle of a multipoint operation, we do not want to call  */
/* all the routines to fix the drawing (e.g. invalImgRect, CommitImgRect,     */
/* FinishUndo).  We just want to save the current point, render, and return   */
void CCurveTool::OnEndDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    PreProcessPoints(pmti);
    SetCurrentPoint(pmti->pt);

    if (m_bMultPtOpInProgress)
        {
        // can't call OnDrag for this object/class, since it calls preprocesspt
        // again, and then onDrag.  If you call preproces again, you will lose
        // bounding rectange box prev, and not be able to invalidate / repaint
        // Still have to invalidate bounding rect, since if rect is larger than
        // current rect, must invalidate to paint. E.g. If let off shift, then
        // let off button, end point would be adjusted and bouning rect would
        // also be correct, but rect calculated in CRubberTool::OnDrag is
        // incorrect.
        InvalImgRect(pImgWnd->m_pImg, &m_cRectBounding);
        CRubberTool::OnDrag(pImgWnd, pmti);
        }
    else
        {
        OnDrag(pImgWnd, pmti); // one last time to refresh display in prep for final render
        Render(CDC::FromHandle(pImgWnd->m_pImg->hDC), m_cRectBounding, pmti->fLeft, TRUE, pmti->fCtrlDown);
        InvalImgRect(pImgWnd->m_pImg, &m_cRectBounding);
        CommitImgRect(pImgWnd->m_pImg, &m_cRectBounding);
        pImgWnd->FinishUndo(m_cRectBounding);

        ClearStatusBarSize();

        CImgTool::OnEndDrag(pImgWnd, pmti);
        }
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
void CCurveTool::OnDrag( CImgWnd* pImgWnd, MTI* pmti )
    {
    PreProcessPoints(pmti);
    SetCurrentPoint(pmti->pt);
    CRubberTool::OnDrag(pImgWnd, pmti);
    }

/******************************************************************************/
/* On Cancel is sent when the user aborts an operation while in progress      */
/* EndMultiptOperation with TRUE will do all our cleanup                      */
void CCurveTool::OnCancel(CImgWnd* pImgWnd)
    {
    EndMultiptOperation(TRUE);
    CImgTool::OnCancel(pImgWnd);
    }

/******************************************************************************/
/* we can only end if the number of maximum points was entered.  We must stay */
/* in capture/multiptmode until we get EXACTLY the desired number of anchor   */
/* points                                                                     */
BOOL CCurveTool::CanEndMultiptOperation(MTI* pmti )
    {

    if (m_iNumAnchorPoints == MAX_ANCHOR_POINTS)
        {
        m_bMultPtOpInProgress = FALSE;
        }
    else
        {
        m_bMultPtOpInProgress = TRUE;
        }

    return (CRubberTool::CanEndMultiptOperation(pmti));
    }

/******************************************************************************/
/* If bAbort is true, this means an error occurred, or the user cancelled the */
/* multipoint operation in the middle of it.  We just set the num of anchor   */
/* points to 0 to stop drawing and call the default endmultiptoperation       */
void CCurveTool::EndMultiptOperation(BOOL bAbort)
    {
    if (bAbort)
        {
        m_iNumAnchorPoints = 0;
        m_cRectBounding.SetRectEmpty();
        }

    CRubberTool::EndMultiptOperation();
    }

