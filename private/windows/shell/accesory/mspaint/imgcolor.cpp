/******************************************************************************/
/*                                                                            */
/* Class Implementations in this file                                         */
/*      CFloatImgColorsWnd                                                    */
/*      CImgColorsWnd                                                         */
/*                                                                            */
/******************************************************************************/

#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "docking.h"
#include "imgwnd.h"
#include "imgsuprt.h"
#include "imgwell.h"
#include "imgtools.h"
#include "toolbox.h"
#include "imgcolor.h"
#include "props.h"
#include "colorsrc.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

#define cxColorBox 16
#define cyColorBox 16

#define TRYANYTHING



/******************************************************************************/

CImgColorsWnd* NEAR g_pImgColorsWnd = NULL;

/******************************************************************************/
//
// MonoRect -- draw a dithered monochrome rectangle with any intensity
//

BOOL MonoRect( CDC* pDC, const CRect& rect, COLORREF rgb, BOOL bFrame )
    {
    CDC      monoDC;
    CBitmap  monoBitmap;
    CBrush   brush;
    CBrush*  pOldBrush;
    CBitmap* pOldBitmap;
        CPen*    pOldPen = NULL;

        //BUGBUG-This can leak DCs and Bitmaps
    if (! monoDC.CreateCompatibleDC( pDC )
    ||  ! monoBitmap.CreateBitmap( rect.Width(), rect.Height(), 1, 1, NULL )
    ||  ! brush.CreateSolidBrush( rgb ))
        {
        return FALSE;
        }

    pOldBitmap = monoDC.SelectObject( &monoBitmap );
    pOldBrush  = monoDC.SelectObject( &brush      );

    if (! bFrame)
        pOldPen = (CPen *)monoDC.SelectStockObject( NULL_PEN );

    monoDC.Rectangle( 0, 0, rect.Width(), rect.Height() );

    pDC->BitBlt( rect.left, rect.top, rect.Width(), rect.Height(),
                                         &monoDC, 0, 0, SRCCOPY );
    monoDC.SelectObject( pOldBrush  );
    monoDC.SelectObject( pOldBitmap );
        if ( pOldPen )
                monoDC.SelectObject(pOldPen);
    monoBitmap.DeleteObject();
    brush.DeleteObject();

    return TRUE;
    }

/******************************************************************************/


/******************************************************************************/

CImgColorsWnd::CImgColorsWnd()
{
    ASSERT( g_pColors );  // just to make sure

    m_nOffsetY       = cyColorBox / 2;
    m_nDisplayColorsInitial = 28;
    m_nDisplayColors = min( g_pColors->GetColorCount(), m_nDisplayColorsInitial );
    m_rectColors.SetRectEmpty();

    // Number of colors / 2 rows + 2 for fore/back area (which is size of 2 wide 2 high)
    //        * size of color  see below for + 3
    // 2 rows * hight for 1 row   +3 is 1 for border above 1st row, 1 for border 2nd row 1 for border on bottom
    m_rectColors.right  = (m_nDisplayColors / 2 + 2) * cxColorBox + 3;
    m_rectColors.bottom = 2 * cyColorBox + 3;

    m_nCols = m_rectColors.Width()  / cxColorBox;
    m_nRows = m_rectColors.Height() / cyColorBox;

}

/******************************************************************************/

BEGIN_MESSAGE_MAP(CImgColorsWnd, CControlBar)
    //{{AFX_MSG_MAP(CImgColorsWnd)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
    ON_WM_GETMINMAXINFO()
        ON_WM_MOUSEMOVE()
        ON_WM_KEYDOWN()
        ON_WM_LBUTTONUP()
        ON_WM_CLOSE()
        //}}AFX_MSG_MAP

    ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()

/******************************************************************************/

BOOL CImgColorsWnd::Create( const TCHAR* pWindowName, DWORD dwStyle, CWnd* pParentWnd )
    {
        // save the style
        m_dwStyle  = (UINT)dwStyle;
      dwStyle &= ~WS_VISIBLE;

    m_rectColors.bottom += (2 * m_nOffsetY);

        // Create the window offscreen initially, since it will get moved to the
        // proper location later
        CRect rcInit(-m_rectColors.right, -m_rectColors.bottom, 0, 0);

        BOOL bCreate = CControlBar::Create( NULL, pWindowName, dwStyle, rcInit,
                                pParentWnd, ID_VIEW_COLOR_BOX, NULL );

        if (m_dwStyle & WS_VISIBLE)
        {
        g_pImgColorsWnd->ShowWindow(SW_SHOW);
        g_pImgColorsWnd->UpdateWindow();
        }

        return bCreate;
    }

/******************************************************************************/

void CImgColorsWnd::OnClose()
    {
#ifdef TRYANYTHING
        CControlBar::OnClose();
#endif
    }

/******************************************************************************/

void CImgColorsWnd::OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler )
    {
    }

/******************************************************************************/

WORD CImgColorsWnd::GetHelpOffset()
    {
    return 0; // REVIEW: NYI!
    }

/******************************************************************************/

CImgColorsWnd::HitZone CImgColorsWnd::HitTest(const CPoint& point)
    {
    CRect rect;

    for (HitZone hitZone = curColor;
                 hitZone < (HitZone)(firstColor + m_nDisplayColors);
                 hitZone = (HitZone)(hitZone + 1))
        if (GetHitRect( hitZone, rect ) && rect.PtInRect( point ))
            return hitZone;

    return none;
    }

/******************************************************************************/

BOOL CImgColorsWnd::GetHitRect( HitZone hitZone, CRect& rect )
    {
    CRect client = m_rectColors;

    client.InflateRect( -1, -(m_nOffsetY + 1) );

    switch (hitZone)
        {
        case none:
            return FALSE;

        case curColor:
            rect.SetRect( client.left, client.top,
                          client.left + cxColorBox * 2 + 1,
                          client.top  + cyColorBox * 2 + 1);
            break;

        default:
            {
            int nColor = (int)hitZone;

            if (nColor < 0 || nColor > m_nDisplayColors)
                return FALSE;

            int row = nColor / (m_nCols - 2);
            int col = nColor % (m_nCols - 2);

            rect.SetRect( client.left + (2 + col    ) * cxColorBox,
                          client.top  +      row      * cyColorBox,
                          client.left + (2 + col + 1) * cxColorBox,
                          client.top  + (    row + 1) * cyColorBox );
            }
            break;
        }

    return TRUE;
    }

/******************************************************************************/

BOOL CImgColorsWnd::OnEraseBkgnd( CDC* pDC )
    {
    CRect rect;

    GetClientRect( rect );

    pDC->FillRect( rect, GetSysBrush( COLOR_BTNFACE ) );

    rect = m_rectColors;

    rect.InflateRect( -1, -(m_nOffsetY + 1) );

    pDC->FillRect( rect, GetSysBrush( COLOR_BTNSHADOW ) );

    GetHitRect( curColor, rect );

    pDC->FillRect(rect, GetSysBrush( COLOR_BTNFACE ) );

        return CControlBar::OnEraseBkgnd( pDC );
    }

/******************************************************************************/

void CImgColorsWnd::OnPaint()
    {
    CPaintDC dc( this );
    CPalette *pcOldPalette = NULL;
    m_nDisplayColors = min( g_pColors->GetColorCount(), m_nDisplayColorsInitial );
    m_rectColors.right  = (m_nDisplayColors / 2 + 2) * cxColorBox + 3;
    m_nCols = m_rectColors.Width()  / cxColorBox;

    if (! dc.m_hDC)
        {
        theApp.SetGdiEmergency();
        return;
        }

    if (theApp.m_pPalette)
        {
                BOOL bForce = FALSE;

                // If we do not realize as a background brush when in-place, we can get
                // an infinite recursion of the container and us trying to realize the
                // palette
                if (theApp.m_pwndInPlaceFrame)
                {
                        bForce = TRUE;
                }

        pcOldPalette = dc.SelectPalette( theApp.m_pPalette, bForce );
        dc.RealizePalette();
        }
    dc.FillRect( (CRect*)&dc.m_ps.rcPaint, GetSysBrush(COLOR_BTNFACE) );
    PaintCurColors( &dc, (CRect*)&dc.m_ps.rcPaint );
    PaintColors   ( &dc, (CRect*)&dc.m_ps.rcPaint );

    if (pcOldPalette)
        dc.SelectPalette( pcOldPalette, FALSE );
    }

/******************************************************************************/

CSize CImgColorsWnd::CalcFixedLayout( BOOL bStretch, BOOL bHorz )
    {
#ifdef TRYANYTHING
        return m_rectColors.Size();
#else
    CSize size = CControlBar::CalcFixedLayout( bStretch, bHorz );

    size.cy = m_rectColors.Height();

    return size;
#endif
    }

/******************************************************************************/

void CImgColorsWnd::OnLButtonDown(UINT nFlags, CPoint point)
    {
    HitZone hitZone = HitTest( point );

    switch (hitZone)
       {
       case none:
          CControlBar::OnLButtonDown(nFlags,point);
          break;

       case curColor:
          break;

       default:
          if (nFlags & MK_CONTROL)
          {
             SetTransColor (hitZone);
          }
          else
          {
             SetDrawColor( (int)hitZone );
          }

          break;
        }
    }

/******************************************************************************/

void CImgColorsWnd::OnRButtonDown(UINT nFlags, CPoint point)
 {
    if (GetCapture() == this)
        {
        CancelDrag();

        return;
        }

    HitZone hitZone = HitTest( point );

    switch (hitZone)
    {
       case none:
       case curColor:
          break;

       default:
          if (nFlags & MK_CONTROL)
          {
             SetTransColor( hitZone );
          }
          else
          {
             SetEraseColor( hitZone );
          }

            break;
    }
 }

/******************************************************************************/

void CImgColorsWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
    {
    HitZone hitZone = HitTest( point );

    switch (hitZone)
        {
        case none:
        case curColor:
            break;

        default:
            if (g_pColors)
                g_pColors->EditColor( TRUE, nFlags&MK_CONTROL );
            break;
        }
    }

/******************************************************************************/

void CImgColorsWnd::OnRButtonDblClk(UINT nFlags, CPoint point)
    {
    HitZone hitZone = HitTest( point );

    switch (hitZone)
        {
        case none:
        case curColor:
            break;

        default:
            if (g_pColors)
                g_pColors->EditColor( FALSE, nFlags&MK_CONTROL );
            break;
        }
    }

/******************************************************************************/

void CImgColorsWnd::OnLButtonUp( UINT nFlags, CPoint point )
    {
    CControlBar::OnLButtonUp( nFlags, point );
    }

/******************************************************************************/

void CImgColorsWnd::OnGetMinMaxInfo( MINMAXINFO FAR* lpMMI )
    {
    CWnd::OnGetMinMaxInfo(lpMMI);

    lpMMI->ptMinTrackSize.x = 37;
    lpMMI->ptMinTrackSize.y = 37;
    lpMMI->ptMaxTrackSize.x = 37 + 128 * 16;
    lpMMI->ptMaxTrackSize.y = 37 + 128 * 16;
    }

/******************************************************************************/

void CImgColorsWnd::PaintCurColors(CDC* pDC, const CRect* pPaintRect)
    {
        CBrush  highlight(GetSysColor(COLOR_BTNHIGHLIGHT));
        CBrush  lowlight(GetSysColor(COLOR_BTNTEXT));
        CBrush  shadow(GetSysColor(COLOR_BTNSHADOW));
        CBrush  face(GetSysColor(COLOR_BTNFACE));

    CRect rect;

    GetHitRect( curColor, rect );

    // Draw current color indicators

#ifdef OLDBORDER
    // Box around colors
    rect.InflateRect( -2, -2 );

    Draw3dRect( pDC->m_hDC, &rect );
    CBrush* pOldBrush = (CBrush*)pDC->SelectStockObject( NULL_BRUSH );

    rect.InflateRect( -1, -1 );

    pDC->Rectangle( &rect );
    pDC->SelectObject( pOldBrush );

#else
    rect.InflateRect( -1, -1 ); rect.top--;
        pDC->FrameRect(&rect,&highlight);
        rect.right--; rect.bottom--;
        pDC->FrameRect(&rect,&shadow);
        rect.left++; rect.top++;
        pDC->FrameRect(&rect,&face);
        rect.right--; rect.bottom--;
        pDC->FrameRect(&rect,&lowlight);
        rect.left++; rect.top++;


        COLORREF        oldTextColor = pDC->SetTextColor(GetSysColor(COLOR_BTNFACE));
        COLORREF        oldBkColor = pDC->SetBkColor(GetSysColor(COLOR_BTNHIGHLIGHT));
        // Draw the transparent color box if set
        if (crTrans != TRANS_COLOR_NONE) // not default
        {
           if (g_pColors->GetMonoFlag())
           {
              MonoRect ( pDC, rect, crTrans, TRUE);
           }
           else
           {
              CBrush brTrans(crTrans);
              pDC->FillRect(&rect, &brTrans);
           }
        }
        else
        {
           pDC->FillRect(&rect,GetHalftoneBrush());
        }
        pDC->SetTextColor(oldTextColor);
        pDC->SetBkColor(oldBkColor);
#endif


    // Draw the overlapping foreground/background color boxes...

    PaintCurColorBox( pDC, TRUE  ); // Background color
    PaintCurColorBox( pDC, FALSE ); // Foreground color
    }

/******************************************************************************/

void CImgColorsWnd::PaintColors(CDC* pDC, const CRect* pPaintRect)
    {
    BOOL bMono = g_pColors->GetMonoFlag();

    CRect    r( 0, 0, 0, 0 );
    COLORREF color;

        CBrush  highlight(GetSysColor(COLOR_BTNHIGHLIGHT));
        CBrush  lowlight(GetSysColor(COLOR_BTNTEXT));
        CBrush  shadow(GetSysColor(COLOR_BTNSHADOW));
        CBrush  face(GetSysColor(COLOR_BTNFACE));

    for (int iLoop = 0; iLoop < m_nDisplayColors; iLoop++)
        if (GetHitRect( (HitZone)iLoop, r ))
            {
               color = g_pColors->GetColor( iLoop );

               pDC->FrameRect(&r,&highlight);
               r.right--; r.bottom--;
               pDC->FrameRect(&r,&shadow);
               r.left++; r.top++;
               pDC->FrameRect(&r,&face);
               r.right--; r.bottom--;
               pDC->FrameRect(&r,&lowlight);

            if (bMono)
                MonoRect( pDC, r, color, TRUE );
            else
                {
                   r.left++; r.top++;
                   CBrush   brush(color);
                   pDC->FillRect( &r, &brush );
                }
            }
    }

/******************************************************************************/

void CImgColorsWnd::InvalidateCurColors()
    {
    CRect rect;

    GetHitRect( curColor, rect );
    InvalidateRect( &rect, FALSE );

    if (CImgTool::GetCurrent()->IsFilled())
        g_pImgToolWnd->InvalidateOptions( FALSE );
    }

/******************************************************************************/

void CImgColorsWnd::PaintCurColorBox(CDC* pDC, BOOL bRight)
    {
    BOOL bMono = g_pColors->GetMonoFlag();

        CBrush  highlight(GetSysColor(COLOR_BTNHIGHLIGHT));
        CBrush  shadow(GetSysColor(COLOR_BTNSHADOW));
        CBrush  face(GetSysColor(COLOR_BTNFACE));

    CRect rc(0, 0, 15, 15);

    COLORREF rgb;
    CBrush   brush;
    CRect    curColorRect;

    GetHitRect( curColor, curColorRect );

    if (bRight)
        {
        rgb = crRight;
        rc.OffsetRect( curColorRect.left + 12, curColorRect.top + 12 );
        }
    else
        {
        rgb = crLeft;
        rc.OffsetRect( curColorRect.left + 5, curColorRect.top + 5 );
        }

        rc.right--; rc.bottom--;
        pDC->FrameRect(&rc,&highlight);
        rc.OffsetRect(1,1);
        pDC->FrameRect(&rc,&shadow);
        rc.right--; rc.bottom--;
        pDC->FrameRect(&rc,&face);
    rc.InflateRect( -1, -1 );

    if (bMono)
        {
        MonoRect( pDC, rc, rgb, TRUE );
                }
    else
                {
                CBrush  colorWell(rgb);
                pDC->FillRect(&rc,&colorWell);
                }
    }

/******************************************************************************/

void CImgColorsWnd::OnMouseMove(UINT nFlags, CPoint point)
    {
    CControlBar::OnMouseMove( nFlags, point );
    }

/******************************************************************************/

void CImgColorsWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
    CControlBar::OnKeyDown( nChar, nRepCnt, nFlags );
    }

/******************************************************************************/

void CImgColorsWnd::CancelDrag()
    {
    }

/******************************************************************************/
