// docking.cpp : implementation of the CDocking class
//

#include "stdafx.h"
#include "pbrush.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "imgwell.h"
#include "toolbox.h"
#include "imgcolor.h"
#include "docking.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CDocking, CObject )

#include "memtrace.h"

/***************************************************************************/
// CDocking implementation

CDocking::CDocking()
    {
    m_bStarted   = FALSE;
    m_bDocking   = FALSE;
    m_iDockingX  = ::GetSystemMetrics( SM_CXICON );
    m_iDockingY  = ::GetSystemMetrics( SM_CYICON );
    m_iDockingX += m_iDockingX / 2;
    m_iDockingY += m_iDockingY / 2;
    }

/***************************************************************************/

BOOL CDocking::Create( CPoint ptDrop, CRect& rectCurrent, BOOL bDocked, CPBView::DOCKERS tool )
    {
    ASSERT( ! m_bStarted );

    m_Tool     = tool;
    m_bDocked  = bDocked;
    m_ptLast   = ptDrop;
    m_bDocking = ! bDocked;

    CRect rectTool;
    CRect rect = rectCurrent;
    CSize size = rectCurrent.Size();

    if (bDocked)
        {
        switch (tool)
            {
            case CPBView::toolbox:
            case CPBView::colorbox:
                rect.InflateRect( theApp.m_cxBorder, theApp.m_cyBorder );
                break;


            }

        rect.bottom += theApp.m_cyCaption;
        m_rectDocked = rectCurrent;
        m_rectFree   = rect;
        }
    else
        {
        switch (tool)
            {
            case CPBView::toolbox:
                g_pImgToolWnd->GetWindowRect( &rectTool );

                rect.right  = rect.left + rectTool.Width();
                rect.bottom = rect.top  + rectTool.Height();
                break;

            case CPBView::colorbox:
                g_pImgColorsWnd->GetWindowRect( &rectTool );

                rect.right  = rect.left + rectTool.Width();
                rect.bottom = rect.top  + rectTool.Height();
                break;

            }
        m_rectDocked = rect;
        m_rectFree   = rectCurrent;
        }

    CPBView* pView = (CPBView*)(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());

    ASSERT( pView != NULL );

    if (pView != NULL && pView->IsKindOf( RUNTIME_CLASS( CPBView ) ))
        {
        m_ptDocking = pView->GetDockedPos( tool, size );
        m_bStarted  = DrawFocusRect();

        m_rectDockingPort.SetRect( m_ptDocking.x - m_iDockingX,
                                   m_ptDocking.y - m_iDockingY,
                                   m_ptDocking.x + m_iDockingX,
                                   m_ptDocking.y + m_iDockingY );
        }
    return m_bStarted;
    }

/***************************************************************************/

BOOL CDocking::Move( CPoint ptNew, CRect& rectFrame )
    {
    Move( ptNew );

    rectFrame = m_bDocked? m_rectDocked: m_rectFree;

    return m_bDocked;
    }

/***************************************************************************/

void CDocking::Move( CPoint ptNew )
    {
    ASSERT( m_bStarted );

    if (DrawFocusRect())
        {
        CPoint pt = ptNew - m_ptLast;

        m_rectDocked.OffsetRect( pt );
        m_rectFree.OffsetRect( pt );

        pt = m_bDocked? m_rectDocked.TopLeft(): m_rectFree.TopLeft();

        m_bDocked = m_rectDockingPort.PtInRect( pt );

        m_ptLast = ptNew;

        DrawFocusRect();
        }
    }

/***************************************************************************/

BOOL CDocking::Clear( CRect* prectLast )
    {
    ASSERT( m_bStarted );

    DrawFocusRect();
    m_bStarted = FALSE;

    if (prectLast)
       *prectLast = m_bDocked? m_rectDocked: m_rectFree;

    if (!m_bDocked)
        {
        CPBView* pView = (CPBView*)(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());

        if (pView != NULL && pView->IsKindOf( RUNTIME_CLASS( CPBView ) ))
            pView->SetFloatPos( m_Tool, m_rectFree );
        }

    return m_bDocked;
    }

/***************************************************************************/

BOOL CDocking::DrawFocusRect()
    {
    if (m_bDocking)
        return TRUE;

    BOOL bReturn = FALSE;

    HDC hdc = ::GetDC( NULL );

    if (hdc)
        {
        ::DrawFocusRect( hdc, (m_bDocked? &m_rectDocked: &m_rectFree) );
        ::ReleaseDC( NULL, hdc );

        bReturn = TRUE;
        }

    return bReturn;
    }

/***************************************************************************/
