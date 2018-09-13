
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
#include "imgcolor.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "tedit.h"
#include "t_text.h"
#include "t_fhsel.h"
#include "toolbox.h"
#include "undo.h"
#include "props.h"
#include "cmpmsg.h"
#include "imgdlgs.h"
#include "ferr.h"
#include "thumnail.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CImgWnd, CWnd)

#include "memtrace.h"

/***************************************************************************/
// helper fns

static CTedit *_GetTextEdit()
{
    if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
        {
        CTextTool* pTextTool = (CTextTool*)CImgTool::GetCurrent();

        if ((pTextTool != NULL) &&
            pTextTool->IsKindOf(RUNTIME_CLASS( CTextTool )))
            {
            CTedit* pTextEdit = pTextTool->GetTextEditField();

            if ((pTextEdit != NULL) &&
                pTextEdit->IsKindOf(RUNTIME_CLASS( CTedit )))
                {
                    return pTextEdit;
                }
            }
        }
    return NULL;
}

BOOL IsUserEditingText()
    {
    return (_GetTextEdit() != NULL);
    }

BOOL TextToolProcessed( UINT nMessage )
    {
    CTedit *pTextEdit = _GetTextEdit();
    if (pTextEdit)
        {
        pTextEdit->SendMessage( WM_COMMAND, nMessage );
        return TRUE;
        }
    return FALSE;
    }

/***************************************************************************/

BEGIN_MESSAGE_MAP(CImgWnd, CWnd)
    ON_WM_CREATE()
#if 0
    ON_WM_DESTROY()
#endif
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_LBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_RBUTTONDBLCLK()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_KEYDOWN()
    ON_WM_KEYUP()
    ON_WM_TIMER()
    ON_WM_CANCELMODE()
    ON_WM_WINDOWPOSCHANGING()
    ON_WM_DESTROYCLIPBOARD()
    ON_WM_PALETTECHANGED()
    ON_WM_SETCURSOR()
    ON_WM_MOUSEWHEEL ()
END_MESSAGE_MAP()

/***************************************************************************/

CRect  rcDragBrush;

CImgBrush  theBackupBrush;

CImgWnd*  g_pMouseImgWnd  = NULL;
CImgWnd*  g_pDragBrushWnd = NULL;

// Current Image Viewer
CImgWnd*          CImgWnd::c_pImgWndCur     = NULL;
CDragger*         CImgWnd::c_pResizeDragger = NULL;
CTracker::STATE   CImgWnd::c_dragState      = CTracker::nil;

// Mouse Tracking Information
MTI   mti;
BOOL  bIgnoreMouse;

/***************************************************************************/

CImgWnd::CImgWnd(IMG* pImg)
    {
    m_pNextImgWnd = NULL;
    m_nZoom       = 1;
    m_nZoomPrev   = 4;
    m_xScroll     = 0;
    m_yScroll     = 0;
    m_LineX       = 1;
    m_LineY       = 1;
    m_ptDispPos.x = -1;
    m_ptDispPos.y = -1;
    c_pImgWndCur  = this;
    m_pwndThumbNailView = NULL;
    m_wClipboardFormat = 0;
    m_hPoints     = NULL;
    m_WheelDelta = 0;
    ASSERT(pImg != NULL);
    m_pImg = pImg;
    m_pImg->m_nLastChanged = -1;
    }

/***************************************************************************/

CImgWnd::CImgWnd(CImgWnd *pImgWnd)
    {
    CWnd::CWnd();       // BUGBUG - This shouldn't be necessary...

    m_pImg        = pImgWnd->m_pImg;
    m_pNextImgWnd = pImgWnd->m_pNextImgWnd;
    m_nZoom       = pImgWnd->m_nZoom;
    m_nZoomPrev   = pImgWnd->m_nZoomPrev;
    m_xScroll     = pImgWnd->m_xScroll,
    m_yScroll     = pImgWnd->m_yScroll;
    m_ptDispPos   = pImgWnd->m_ptDispPos;
    m_pwndThumbNailView = NULL;
    m_wClipboardFormat = 0;
    m_hPoints     = NULL;
    }

/***************************************************************************/

CImgWnd::~CImgWnd()
    {
    if (c_pImgWndCur == this)
        c_pImgWndCur = NULL;

    if (g_pMouseImgWnd == this)
        g_pMouseImgWnd = NULL;

    HideBrush();
    fDraggingBrush = FALSE;

    g_bBrushVisible = FALSE;

    if (g_pDragBrushWnd == this)
        {
        g_pDragBrushWnd = NULL;
        }

    if (m_hPoints)
        {
        ::GlobalFree( m_hPoints );
        m_hPoints = NULL;
        }
    }

/***************************************************************************/

BOOL CImgWnd::Create( DWORD dwStyle, const RECT& rect,
                      CWnd* pParentWnd, UINT nID)
    {
    static CString  sImgWndClass;

    if (sImgWndClass.IsEmpty())
        sImgWndClass = AfxRegisterWndClass( CS_DBLCLKS );

    ASSERT( ! sImgWndClass.IsEmpty() );

    dwStyle |= WS_CLIPSIBLINGS;

    return CWnd::Create( sImgWndClass, NULL, dwStyle, rect, pParentWnd, nID );
    }

/***************************************************************************/

int CImgWnd::OnCreate( LPCREATESTRUCT lpCreateStruct )
    {
    if (m_pImg)
        AddImgWnd( m_pImg, this );

    return CWnd::OnCreate(lpCreateStruct);
    }

/***************************************************************************/

#if 0
void CImgWnd::OnDestroy()
    {
    if (c_pImgWndCur == this)
        c_pImgWndCur = NULL;

    HideBrush();
    fDraggingBrush = FALSE;

    CWnd::OnDestroy();
    }
#endif

/***************************************************************************/

void CImgWnd::OnPaletteChanged(CWnd *pPaletteWnd)
{
#if 0
    // obviously this never gets hit or somebody would have realized by now...
    CImgWnd::OnPaletteChanged(pPaletteWnd);
#endif
    Invalidate();
}


/***************************************************************************/

BOOL CImgWnd::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message)
{
        if (nHitTest==HTCLIENT && pWnd->m_hWnd==m_hWnd)
        {
                // We do our own cursor stuff in our own client area, but not in the
                // text box
                return(TRUE);
        }

        return((BOOL)Default());
}

/***************************************************************************/

void CImgWnd::OnWindowPosChanging( WINDOWPOS FAR* lpwndpos )
    {
    CWnd::OnWindowPosChanging( lpwndpos );
    }

/***************************************************************************/
// Image View Painting Functions
//

void CImgWnd::OnPaint()
    {
    CPaintDC dc(this);

    if (dc.m_hDC == NULL)
        {
        theApp.SetGdiEmergency();
        return;
        }

    if (m_pImg == NULL)
        return;

    if (g_pMouseImgWnd == this)
        CImgTool::HideDragger( this );

    CPalette* ppalOld = SetImgPalette( &dc, FALSE );

    DrawBackground( &dc, (CRect*)&dc.m_ps.rcPaint );
    DrawImage     ( &dc, (CRect*)&dc.m_ps.rcPaint );
    DrawTracker   ( &dc, (CRect*)&dc.m_ps.rcPaint );

    if (g_pMouseImgWnd == this)
        CImgTool::ShowDragger( this );

    if (m_pwndThumbNailView != NULL)
        m_pwndThumbNailView->RefreshImage();

    if (ppalOld)
        dc.SelectPalette( ppalOld, FALSE );
    }

/***************************************************************************/

BOOL CImgWnd::OnCmdMsg( UINT nID, int nCode, void* pExtra,
                        AFX_CMDHANDLERINFO* pHandlerInfo )
    {
    if (nCode == CN_COMMAND)
        {
        switch (nID)
            {
            case IDMX_VS_PAGEUP:
                SendMessage( WM_VSCROLL, SB_PAGEUP, 0L );
                return TRUE;


            case IDMX_VS_PAGEDOWN:
                SendMessage( WM_VSCROLL, SB_PAGEDOWN, 0L );
                return TRUE;


            case IDMX_HS_PAGEUP:
                SendMessage( WM_HSCROLL, SB_PAGEUP, 0L );
                return TRUE;


            case IDMX_HS_PAGEDOWN:
                SendMessage( WM_HSCROLL, SB_PAGEDOWN, 0L );
                return TRUE;

            }

        CImgTool* pImgTool = CImgTool::FromID( nID );

        if (pImgTool != NULL)
            {
            pImgTool->Select();
            return TRUE;
            }
        }
    return CWnd::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo );
    }


void CImgWnd::GetDrawRects(const CRect* pPaintRect, const CRect* pReqDestRect,
                CRect& srcRect, CRect& destRect)
{
        // Find the sub-rectangle of srcRect that corresponds to
        // the pPaintRect sub-rectangle of destRect.
        srcRect         = *pPaintRect;
        srcRect.right  += m_nZoom - 1;
        srcRect.bottom += m_nZoom - 1;

        ClientToImage( srcRect );

        srcRect.left = max(0, srcRect.left);
        srcRect.top  = max(0, srcRect.top );

        srcRect.right  = min(m_pImg->cxWidth , srcRect.right );
        srcRect.bottom = min(m_pImg->cyHeight, srcRect.bottom);

        if (pReqDestRect == NULL)
        {
                destRect = srcRect;
                ImageToClient( destRect );
        }
        else
        {
                destRect = *pReqDestRect;
        }
}


/***************************************************************************/
// Draw the actual image 'a bitmap'.  Drawing is
// optimized to only deal with the pixels inside paintRect.  This function
// reduces flashing by drawing the image and optional grid in an off-screen
// bitmap and then transfering that bitmap to the screen.
//
void CImgWnd::DrawImage( CDC* pDC, const CRect* pPaintRect,
                                         CRect* pDestRect, BOOL bDoGrid )
    {
    ASSERT(    pDC != NULL );
    ASSERT( m_pImg != NULL );

    CRect destRect;
    CRect srcRect;

    GetDrawRects(pPaintRect, pDestRect, srcRect, destRect);

    if (srcRect.Width() <= 0 || srcRect.Height() <= 0)
        {
        // Nothing to paint...
        return;
        }

    if (! IsGridVisible() && m_nZoom == 1)
        {
        // Optimize the easy case...  (Can't speed up magnified views
        // because of the bogus hack we have to do in StretchCopy.)

        if (theApp.m_pPalette
        && ((m_pImg->cPlanes * m_pImg->cBitCount) == 1))
            {
            pDC->SetTextColor( PALETTEINDEX( 0 ) );
            pDC->SetBkColor  ( PALETTEINDEX( 1 ) );
            }


        BitBlt(pDC->m_hDC, destRect.left   , destRect.top,
                           destRect.Width(), destRect.Height(),
              m_pImg->hDC,  srcRect.left   , srcRect.top, SRCCOPY);

        return;
        }

    CDC tempDC;
    CBitmap      tempBitmap;
    CBitmap* pOldTempBitmap;

    if (! tempDC.CreateCompatibleDC(pDC)
    ||  ! tempBitmap.CreateCompatibleBitmap(pDC, destRect.Width() + 1,
                                                 destRect.Height() + 1))
        {
        theApp.SetGdiEmergency(FALSE);
        return;
        }

    pOldTempBitmap = tempDC.SelectObject(&tempBitmap);

    ASSERT(pOldTempBitmap != NULL);

    CPalette* pOldPalette = SetImgPalette( &tempDC, FALSE ); // Background ??

    // If we're zoomed in, use COLORONCOLOR for easy pixel-by-pixel editing
    // Otherwise use HALFTONE for nice appearance
    if (m_nZoom < 2)
    {
        tempDC.SetStretchBltMode(HALFTONE);
    }
    else
    {
        tempDC.SetStretchBltMode(COLORONCOLOR);
    }


    if (m_pImg->cPlanes * m_pImg->cBitCount == 1)
        {
        tempDC.SetTextColor( RGB( 0x00, 0x00, 0x00 ));
        tempDC.SetBkColor  ( RGB( 0xFF, 0xFF, 0xFF ));
        }

    // Bitmaps...
    StretchCopy(tempDC.m_hDC, 0, 0, destRect.Width(), destRect.Height(),
                 m_pImg->hDC,       srcRect.left, srcRect.top,
                                    srcRect.Width(), srcRect.Height());
    // Draw the grid...
    if (IsGridVisible() && bDoGrid)
        DrawGrid( &tempDC, srcRect, destRect );

    // Transfer to the screen...
    pDC->BitBlt(destRect.left, destRect.top, destRect.Width(),
                               destRect.Height(), &tempDC, 0, 0, SRCCOPY);
    // Cleanup...
    if (pOldPalette)
        tempDC.SelectPalette( pOldPalette, FALSE ); // Background ??

    tempDC.SelectObject(pOldTempBitmap);
    }

/***************************************************************************/
// Draw a border and bevel around the image and fill the rest of
// the window with gray.  If pPaintRect is not NULL, painting is
// optimized to only draw with the rectangle.
//
void CImgWnd::DrawBackground(CDC* pDC, const CRect* pPaintRect)
    {
    ASSERT( pDC != NULL );

    CRect clientRect;

    if (pPaintRect == NULL)
        {
        // Draw everything...
        GetClientRect( &clientRect );
        pPaintRect = &clientRect;
        }

    CRect srcRect;
    CRect imageRect;

    GetDrawRects(pPaintRect, NULL, srcRect, imageRect);

    // Erase area around image, border, and bevel...
    CBrush* pOldBrush = pDC->SelectObject( GetSysBrush( COLOR_APPWORKSPACE ) );

    if (imageRect.top > pPaintRect->top)
        {
        // Top...

        pDC->PatBlt(pPaintRect->left, pPaintRect->top, pPaintRect->Width(),
                      imageRect.top - pPaintRect->top, PATCOPY);
        }

    if (imageRect.left > pPaintRect->left)
        {
        // Left...

        pDC->PatBlt(pPaintRect->left, imageRect.top,
            imageRect.left - pPaintRect->left, imageRect.Height(), PATCOPY);
        }

    if (imageRect.right < pPaintRect->right)
        {
        // Right...

        pDC->PatBlt(imageRect.right, imageRect.top,
            pPaintRect->right - imageRect.right, imageRect.Height(), PATCOPY);
        }

    if (imageRect.bottom < pPaintRect->bottom)
        {
        // Bottom...

        pDC->PatBlt(pPaintRect->left, imageRect.bottom, pPaintRect->Width(),
            pPaintRect->bottom - imageRect.bottom, PATCOPY);
        }

    pDC->SelectObject(pOldBrush);
    }

/***************************************************************************/

void CImgWnd::SetImg(IMG* pImg)
    {
    m_pNextImgWnd = pImg->m_pFirstImgWnd;
    pImg->m_pFirstImgWnd = this;
    m_pImg = pImg;
    }

/***************************************************************************/

CPalette* CImgWnd::SetImgPalette( CDC* pdc, BOOL bForce )
    {
    CPalette* ppal = NULL;

        // If we do not realize as a background brush when in-place, we can get
        // an infinite recursion of the container and us trying to realize the
        // palette
        if (theApp.m_pwndInPlaceFrame)
        {
                bForce = TRUE;
        }

    if (theApp.m_pPalette
    &&  theApp.m_pPalette->m_hObject)
        {
        ppal = pdc->SelectPalette( theApp.m_pPalette, bForce );

        pdc->RealizePalette();
        }
    return ppal;
    }

/***************************************************************************/

HPALETTE CImgWnd::SetImgPalette( HDC hdc, BOOL bForce )
    {
    HPALETTE hpal = NULL;

        // If we do not realize as a background brush when in-place, we can get
        // an infinite recursion of the container and us trying to realize the
        // palette
        if (theApp.m_pwndInPlaceFrame)
        {
                bForce = TRUE;
        }

    if (theApp.m_pPalette
    &&  theApp.m_pPalette->m_hObject)
        {
        hpal = ::SelectPalette( hdc, (HPALETTE)theApp.m_pPalette->m_hObject, bForce );

        ::RealizePalette( hdc );
        }
    return hpal;
    }

/***************************************************************************/

void CImgWnd::SetZoom(int nZoom)
    {
    if (m_nZoom > 1)
        m_nZoomPrev = m_nZoom;

    CommitSelection(TRUE);

    if (nZoom > 1)
        {
        // deselect the text tool if it's around
        CImgTool* pImgTool = CImgTool::GetCurrent();

        if (pImgTool != NULL && CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
            {
            CImgTool::Select(IDMB_PENCILTOOL);
            }
        }

    HideBrush();
    SetupRubber( m_pImg );
    EraseTracker();
    theImgBrush.m_pImg = NULL;
    DrawTracker();

    CPBView* pView = (CPBView*)GetParent();

    if (pView != NULL && pView->IsKindOf( RUNTIME_CLASS( CPBView ) ))
        if (nZoom == 1)
            pView->HideThumbNailView();
        else
            pView->ShowThumbNailView();

        Invalidate(FALSE);

    m_nZoom = nZoom;
    }

/***************************************************************************/

void CImgWnd::SetScroll(int xPos, int yPos)
    {
    if (xPos > 0)
        xPos = 0;
    else
        if (xPos < -m_pImg->cxWidth)
            xPos = -m_pImg->cxWidth;

    if (yPos > 0)
        yPos = 0;
    else
        if (yPos < -m_pImg->cyHeight)
            yPos = -m_pImg->cyHeight;

    m_xScroll = xPos;
    m_yScroll = yPos;

    Invalidate( FALSE );

    CheckScrollBars();
    }

/***************************************************************************/

void CImgWnd::CheckScrollBars()
    {
    // Tacky recursion blocker is required because this is called from
    // the OnSize handler and turning scroll bars on or off changes
    // the size of our window...
    static BOOL  bInHere = FALSE;

    if (bInHere)
        return;

    bInHere = TRUE;

    int cxVScrollBar = GetSystemMetrics( SM_CXVSCROLL );
    int cyHScrollBar = GetSystemMetrics( SM_CYHSCROLL );

    // Figure the client area size if there were no scroll bars...
    CRect clientRect;

    GetClientRect( &clientRect );

    int cxWidth  = clientRect.Width();
    int cyHeight = clientRect.Height();

    BOOL hHasHBar = ((GetStyle() & WS_HSCROLL) != 0);
    BOOL bHasVBar = ((GetStyle() & WS_VSCROLL) != 0);

    if (hHasHBar)
        cyHeight += cyHScrollBar;

    if (bHasVBar)
        cxWidth += cxVScrollBar;

    // Figure the size of the thing we are scrolling (the subject)...
    CSize subjectSize;

    GetImgSize( m_pImg, subjectSize );

    int iTrackerSize = 2 * CTracker::HANDLE_SIZE;

    subjectSize.cx = (subjectSize.cx * m_nZoom ) + iTrackerSize;
    subjectSize.cy = (subjectSize.cy * m_nZoom ) + iTrackerSize;

    m_LineX = (subjectSize.cx + 31) / 32;
    m_LineY = (subjectSize.cy + 31) / 32;

    // Nasty loop takes care of case where we only need a vertical
    // scroll bar because we added a horizontal scroll bar and
    // vice versa...  (Will only ever loop twice.)
    BOOL bNeedHBar = FALSE;
    BOOL bNeedVBar = FALSE;
    BOOL bChange;

    do  {
        bChange = FALSE;

        if (! bNeedVBar && subjectSize.cy > cyHeight)
            {
            bChange   = TRUE;
            bNeedVBar = TRUE;
            cxWidth  -= cxVScrollBar;
            }

        if (! bNeedHBar && subjectSize.cx > cxWidth)
            {
            bChange   = TRUE;
            bNeedHBar = TRUE;
            cyHeight -= cyHScrollBar;
            }
        } while (bChange);

    SetRedraw( FALSE );

    SCROLLINFO si;

    si.cbSize = sizeof( si );
    si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin   = 0;

    // We subtract 1 because subjectSize is the size we want, so the range
    // should be 0 to subjectSize-1
    si.nMax   = (subjectSize.cx - 1) / m_nZoom;
    si.nPage  = cxWidth / m_nZoom;
    si.nPos   = -m_xScroll;
    SetScrollInfo( SB_HORZ, &si, FALSE );

    si.nMax   = (subjectSize.cy - 1) / m_nZoom;
    si.nPage  = cyHeight / m_nZoom;
    si.nPos   = -m_yScroll;
    SetScrollInfo( SB_VERT, &si, FALSE );

        si.fMask = SIF_POS;
        GetScrollInfo( SB_HORZ, &si );
        if ( -m_xScroll != si.nPos )
                m_xScroll = -si.nPos ;
        GetScrollInfo( SB_VERT, &si );
        if ( -m_yScroll != si.nPos )
                m_yScroll = -si.nPos;

    SetRedraw ( TRUE  );
    Invalidate( FALSE );

    bInHere = FALSE;
    }

/***************************************************************************/

void CImgWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar*)
    {
    OnScroll(FALSE, nSBCode, nPos);
    }

/***************************************************************************/

void CImgWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*)
    {
    OnScroll(TRUE, nSBCode, nPos);
    }

/***************************************************************************/

void CImgWnd::OnScroll(BOOL bVert, UINT nSBCode, UINT nPos)
    {
    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof( ScrollInfo );
    ScrollInfo.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;

    GetScrollInfo( (bVert? SB_VERT: SB_HORZ), &ScrollInfo );
        int iScroll = ScrollInfo.nPage/4;
    int iNewPos = ScrollInfo.nPos;

    switch (nSBCode)
        {
        case SB_TOP:
            iNewPos = 0;
            break;

        case SB_BOTTOM:
            iNewPos = ScrollInfo.nMax;
            break;

        case SB_LINEDOWN:
            iNewPos += iScroll;
            break;

        case SB_LINEUP:
            iNewPos -= iScroll;
            break;

        case SB_PAGEDOWN:
            iNewPos += iScroll * 4;
            break;

        case SB_PAGEUP:
            iNewPos -= iScroll * 4;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            iNewPos = nPos;
            break;
        }

    if (iNewPos < ScrollInfo.nMin)
        iNewPos = 0;
    else
        if (iNewPos > ScrollInfo.nMax-(int)ScrollInfo.nPage+1)
            iNewPos = ScrollInfo.nMax-(int)ScrollInfo.nPage+1;

    iScroll = -(iNewPos - ScrollInfo.nPos);
        Invalidate(FALSE);

    if (bVert)
        m_yScroll = -iNewPos;
    else
        m_xScroll = -iNewPos;

    ScrollInfo.fMask = SIF_POS;
    ScrollInfo.nPos  = iNewPos;
    SetScrollInfo( (bVert? SB_VERT: SB_HORZ), &ScrollInfo, TRUE );
    }

/***************************************************************************/
BOOL CImgWnd::OnMouseWheel (UINT nFlags, short zDelta, CPoint pt)
    {
    //
    // Don't handle zoom and datazoom.
    //

    if (nFlags & (MK_SHIFT | MK_CONTROL))
        {
        return FALSE;
        }

    SCROLLINFO ScrollInfo;

    ScrollInfo.cbSize = sizeof( ScrollInfo );
    ScrollInfo.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;

    GetScrollInfo( SB_VERT, &ScrollInfo );
    m_WheelDelta -= zDelta;
    if (abs(m_WheelDelta) >= WHEEL_DELTA)
        {
        int iScroll = ScrollInfo.nPage/4 * (m_WheelDelta/WHEEL_DELTA);
        int iNewPos = ScrollInfo.nPos + iScroll;


        if (iNewPos < ScrollInfo.nMin)
           iNewPos = 0;
        else if (iNewPos > ScrollInfo.nMax-(int)ScrollInfo.nPage+1)
           iNewPos = ScrollInfo.nMax-(int)ScrollInfo.nPage+1;

        Invalidate(FALSE);

        m_yScroll = -iNewPos;

        ScrollInfo.fMask = SIF_POS;
        ScrollInfo.nPos  = iNewPos;
        SetScrollInfo( SB_VERT, &ScrollInfo, TRUE );
        m_WheelDelta= m_WheelDelta % WHEEL_DELTA;
        CImgTool* pImgTool = CImgTool::GetCurrent();
        mti.ptPrev = mti.pt;
        mti.pt = pt;
        pImgTool->OnMove (this, &mti);
    }


    return TRUE;
    }
/***************************************************************************/

void CImgWnd::PrepareForBrushChange(BOOL bPickup, BOOL bErase)
    {
    if (theImgBrush.m_pImg != NULL
    &&  theImgBrush.m_bFirstDrag)
        {
        if (bPickup)
            PickupSelection();

        SetUndo(m_pImg);

        theImgBrush.m_bLastDragWasFirst = TRUE;
        theImgBrush.m_bFirstDrag        = FALSE;
        theImgBrush.m_rcDraggedFrom     = rcDragBrush;

        if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
            {
            HideBrush();
            bErase = FALSE;
            }

        if (bErase)
            {
            // Clear the background...
            HideBrush();

            FillImgRect( m_pImg->hDC, &theImgBrush.m_rcDraggedFrom, crRight );

            CommitImgRect(m_pImg, &theImgBrush.m_rcDraggedFrom);
            InvalImgRect (m_pImg, &theImgBrush.m_rcDraggedFrom);

            FinishUndo(theImgBrush.m_rcDraggedFrom);

            MoveBrush(theImgBrush.m_rcSelection);
            }
        }
    }

/***************************************************************************/

void CImgWnd::OnCancelMode()
    {
    CmdCancel();
    }

/***************************************************************************/

void CImgWnd::CmdCancel()
    {
    // This will:
    //  Erase the size indicator on the status bar.
    //  Reset the mouse cursor to an arrow.
    //  Release the capture.
    //  Cancel (and undo) any drawing operation in progress.
    //  Cancel the Pick Color command if it's active.
    //  If there's a selection, will set to whole image and select prev tool
    ClearStatusBarSize();

    mti.fLeft = mti.fRight = FALSE;

    if (c_pResizeDragger != NULL)
        {
        EndResizeOperation();
        bIgnoreMouse = TRUE;
        return;
        }

    CImgTool* pImgTool = CImgTool::GetCurrent();

    if (GetCapture() == this || pImgTool->IsMultPtOpInProgress())
        {
        // Cancel dragging or multi-point operation in progress
        BOOL bWasMakingSelection = theImgBrush.m_bMakingSelection;

        ZoomedInDP(WM_CANCEL, 0, CPoint(0, 0));

        SetCursor(LoadCursor(NULL, IDC_ARROW + 11));

        if (! bWasMakingSelection)
            CancelPainting();

        bIgnoreMouse = TRUE;
        }
    else
        if (pImgTool->IsToggle()
        ||  CImgTool::GetCurrentID() == IDMB_PICKTOOL
        ||  CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL
        ||  CImgTool::GetCurrentID() == IDMZ_BRUSHTOOL
        ||  CImgTool::GetCurrentID() == IDMB_POLYGONTOOL
        ||  CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
            {
            pImgTool->OnCancel( this );
            }

    if (GetKeyState( VK_LBUTTON ) < 0 || GetKeyState( VK_RBUTTON) < 0 )
        bIgnoreMouse = TRUE;

    SetToolCursor();
    }

/***************************************************************************/

void CImgWnd::CmdSel2Bsh()
    {
    if (! g_bCustomBrush)
        {
        if (theImgBrush.m_pImg == NULL)
            {
            // No selection, turn the whole image into a brush!
            MakeBrush( m_pImg->hDC, CRect( 0, 0, m_pImg->cxWidth,
                                                 m_pImg->cyHeight ) );
            }

        if (theImgBrush.m_bFirstDrag)
            {
            // Time to pick up the bits!
            ASSERT(theImgBrush.m_pImg == m_pImg);

            PickupSelection();
            }

        InvalImgRect(theImgBrush.m_pImg, NULL); // erase the selection tracker
        CImgTool::Select(IDMZ_BRUSHTOOL);
        SetCombineMode(combineMatte);

        g_bCustomBrush = TRUE;

        theImgBrush.m_pImg = NULL;
        theImgBrush.CenterHandle();
        }
    else
        if (CImgTool::GetCurrentID() == IDMZ_BRUSHTOOL)
            {
            CImgTool::GetCurrent()->OnCancel(this);
            }
    }

/***************************************************************************/
// Coordinate Translation and Calculation Functions
//
//
// Convert a point or rect in image view client coordinates to image
// coordinates taking magnification and scrolling into account.
//
void CImgWnd::ClientToImage(CPoint& point)
    {
    int iHandleSize = CTracker::HANDLE_SIZE;

    point.x = (point.x - iHandleSize) / m_nZoom - m_xScroll;
    point.y = (point.y - iHandleSize) / m_nZoom - m_yScroll;
    }

/***************************************************************************/

void CImgWnd::ClientToImage(CRect& rect)
    {
    ClientToImage(rect.TopLeft());
    ClientToImage(rect.BottomRight());
    }

/***************************************************************************/
// Convert a point or rect in image coordinates to image view client
// coordinates taking magnification and scrolling into account.
//
void CImgWnd::ImageToClient(CPoint& point)
    {
    int iHandleSize = CTracker::HANDLE_SIZE;

    point.x = (point.x + m_xScroll) * m_nZoom + iHandleSize;
    point.y = (point.y + m_yScroll) * m_nZoom + iHandleSize;
    }

/***************************************************************************/

void CImgWnd::ImageToClient(CRect& rect)
    {
    ImageToClient(rect.TopLeft());
    ImageToClient(rect.BottomRight());
    }

/***************************************************************************/
// Return a rectangle in image view coordinates surrounding the image
// taking magnification, scrolling, and the grid into account.

void CImgWnd::GetImageRect( CRect& imageRect )
    {
    imageRect.SetRect( 0, 0, m_pImg->cxWidth, m_pImg->cyHeight );

    ImageToClient( imageRect );

    if (IsGridVisible())
        {
        imageRect.right  += 1;
        imageRect.bottom += 1;
        }
    }

/***************************************************************************/

CRect CImgWnd::GetDrawingRect( void )
    {
    CRect rectImage;
    CRect rectClient;

    GetImageRect (  rectImage );
    GetClientRect( &rectClient );

    rectImage &= rectClient;
    rectImage.InflateRect( CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE );

    return ( rectImage );
    }

/***************************************************************************/

void CImgWnd::OnSetFocus(CWnd* pOldWnd)
    {
    if (m_pImg == NULL)
        {
        // Time to die...  (Our img was deleted, so we'll be disappearing
        // soon.  Don't bother to do any of the rest of this function...
        return;
        }

        Invalidate();
        BringWindowToTop(); // so updates happen here first

    if (c_pImgWndCur != this
    &&  c_pImgWndCur != NULL)
        c_pImgWndCur->EraseTracker();

    c_pImgWndCur = this;

    SelectImg( m_pImg );
        UpdateWindow();

    CWnd::OnSetFocus( pOldWnd );

    DrawTracker();
    }

/***************************************************************************/

void CImgWnd::OnKillFocus(CWnd* pNewWnd)
    {
        Invalidate();

    if (theImgBrush.m_pImg == NULL)
        HideBrush();

    if (GetCapture() == this)
        CmdCancel();

    CWnd::OnKillFocus(pNewWnd);
    }

/***************************************************************************/

void CImgWnd::OnSize(UINT nType, int cx, int cy)
    {
    CheckScrollBars();

    CWnd::OnSize(nType, cx, cy);
    }

/***************************************************************************/

BOOL CImgWnd::OnMouseDown(UINT nFlags)
    {
    if (GetFocus() != this)
        {
        SetFocus();
        SetActiveWindow();
        }

    if ((nFlags & (MK_LBUTTON | MK_RBUTTON)) == (MK_LBUTTON | MK_RBUTTON))
        {
        ClearStatusBarSize();

        BOOL bWasMakingSelection = theImgBrush.m_bMakingSelection;

        ZoomedInDP(WM_CANCEL, 0, CPoint(0, 0));

        SetCursor(LoadCursor(NULL, IDC_ARROW + 11));

        if (! bWasMakingSelection)
            CancelPainting();

        bIgnoreMouse = TRUE;
        return FALSE;
        }

    return TRUE;
    }

/***************************************************************************/

BOOL CImgWnd::OnMouseMessage( UINT nFlags )
    {
    if (bIgnoreMouse /*|| GetFocus() != this*/)
        {
        if ((nFlags & (MK_LBUTTON | MK_RBUTTON)) == 0)
            {
            bIgnoreMouse = FALSE;
            SetToolCursor();
            }
        else
            {
            SetCursor( LoadCursor( NULL, IDC_ARROW ) );
            }

        return FALSE;
        }

    if ((CImgTool::GetCurrentID() != IDMB_PICKTOOL)
    &&  (CImgTool::GetCurrentID() != IDMB_PICKRGNTOOL))
        SetupRubber(m_pImg);

    const MSG* pMsg = GetCurrentMessage();
    mti.fCtrlDown = (nFlags & MK_CONTROL);
    ZoomedInDP( pMsg->message, (DWORD)pMsg->wParam, CPoint( (DWORD)pMsg->lParam ) );

    return TRUE;
    }

/***************************************************************************/

void CImgWnd::OnLButtonDown( UINT nFlags, CPoint point )
    {
    CWnd::OnLButtonDown( nFlags, point );

    if (OnMouseDown( nFlags ))
        {
        OnMouseMessage( nFlags );
        }
    }

/***************************************************************************/

void CImgWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
    {
    CRect rect;
    GetImageRect(rect);

    // When inside the image, a double click is the same as a single click
    OnLButtonDown(nFlags, point);
    }

/***************************************************************************/

void CImgWnd::OnLButtonUp(UINT nFlags, CPoint point)
    {
    CWnd::OnLButtonUp(nFlags, point);

    OnMouseMessage(nFlags);
    }

/***************************************************************************/

void CImgWnd::OnRButtonDown(UINT nFlags, CPoint point)
    {
    CWnd::OnRButtonDown(nFlags, point);

    if (OnMouseDown(nFlags))
        {
        OnMouseMessage(nFlags);
        }
    }


/***************************************************************************/

void CImgWnd::OnRButtonDblClk(UINT nFlags, CPoint point)
    {
    // A right button double click is the same as a right button single click
    OnRButtonDown(nFlags, point);
    }


/***************************************************************************/

void CImgWnd::OnRButtonUp(UINT nFlags, CPoint point)
    {
    CWnd::OnRButtonUp(nFlags, point);

    OnMouseMessage(nFlags);
    }


/***************************************************************************/

void CImgWnd::OnMouseMove(UINT nFlags, CPoint point)
    {
    CWnd::OnMouseMove(nFlags, point);

    if (g_pMouseImgWnd != this
    &&  g_pMouseImgWnd != NULL)
        {
        CImgTool::GetCurrent()->OnLeave( g_pMouseImgWnd, &mti );
        g_pMouseImgWnd = NULL;
        }

    ClientToImage( point );
    m_ptDispPos = point;

    if (g_pMouseImgWnd == NULL)
        {
        MTI mtiEnter;

        mtiEnter.pt        = point;
        mtiEnter.ptDown    = point;
        mtiEnter.ptPrev    = point;
        mtiEnter.fLeft     = FALSE;
        mtiEnter.fRight    = FALSE;
        mtiEnter.fCtrlDown = FALSE;

        CImgTool::GetCurrent()->OnEnter( g_pMouseImgWnd, &mtiEnter );

        g_pMouseImgWnd  = this;
        }
    OnMouseMessage( nFlags );
    }

/***************************************************************************/

void CImgWnd::OnTimer(UINT nIDEvent)
    {
    OnMouseMessage( 0 );
    }

/***************************************************************************/

void CImgWnd::SetToolCursor()
    {
    UINT nCursorID = CImgTool::GetCurrent()->GetCursorID();
    HCURSOR hCursor = NULL;

    if (nCursorID != 0)
        {
        hCursor = LoadCursor(nCursorID < 32512 ?
            AfxGetResourceHandle() : NULL, MAKEINTRESOURCE( nCursorID ));
        }

    SetCursor(hCursor);
    }


/***************************************************************************/

void CImgWnd::EndResizeOperation()
    {
    ReleaseCapture();
    delete c_pResizeDragger;
    c_pResizeDragger = NULL;
    c_dragState = CTracker::nil;
    ClearStatusBarSize();
    }


/***************************************************************************/

void CImgWnd::ResizeMouseHandler(unsigned code, CPoint imagePt)
    {
    CRect imageRect = c_pResizeDragger->m_rect;
    ClientToImage(imageRect);

    switch (code)
        {
        case WM_CANCEL:
            EndResizeOperation();
            return;

        case WM_LBUTTONUP:
            // resizing whole bitmap
            if  (m_pImg != theImgBrush.m_pImg
            &&   m_pwndThumbNailView)
                {
                m_pwndThumbNailView->Invalidate();
                }

            EndResizeOperation();

            if (theImgBrush.m_pImg == NULL)
                {
                // User was resizing the whole image...
                CPBView* pView = (CPBView*)((CFrameWnd*)AfxGetMainWnd())->GetActiveView();
                CPBDoc*  pDoc  = (pView == NULL)? NULL: pView->GetDocument();

                if (pDoc != NULL)
                    {
                    theUndo.BeginUndo( TEXT("Property Edit") );

                    if (GetKeyState( VK_SHIFT ) < 0)
                        pDoc->m_pBitmapObj->SetIntProp( P_Shrink, 1 );

                    theApp.m_sizeBitmap = imageRect.Size();

                    pDoc->m_pBitmapObj->SetSizeProp( P_Size, theApp.m_sizeBitmap );
                    pDoc->m_pBitmapObj->SetIntProp ( P_Shrink, 0 );

                    theUndo.EndUndo();
                    }
                }
            else
                {
                // User was resizing the selection...
                HideBrush();
                theImgBrush.SetSize( imageRect.Size(), TRUE );
                MoveBrush( imageRect );
                }
            return;

        case WM_MOUSEMOVE:
            switch (c_dragState)
                {
                default:
                    ASSERT(FALSE);

                case CTracker::resizingTop:
                    imageRect.top = imagePt.y;
                    if (imageRect.top >= imageRect.bottom)
                        imageRect.top = imageRect.bottom - 1;
                    break;

                case CTracker::resizingLeft:
                    imageRect.left = imagePt.x;
                    if (imageRect.left >= imageRect.right)
                        imageRect.left = imageRect.right - 1;
                    break;

                case CTracker::resizingRight:
                    imageRect.right = imagePt.x;
                    if (imageRect.right <= imageRect.left)
                        imageRect.right = imageRect.left + 1;
                    break;

                case CTracker::resizingBottom:
                    imageRect.bottom = imagePt.y;
                    if (imageRect.bottom <= imageRect.top)
                        imageRect.bottom = imageRect.top + 1;
                    break;

                case CTracker::resizingTopLeft:
                    imageRect.left = imagePt.x;
                    imageRect.top = imagePt.y;
                    if (imageRect.top >= imageRect.bottom)
                        imageRect.top = imageRect.bottom - 1;
                    if (imageRect.left >= imageRect.right)
                        imageRect.left = imageRect.right - 1;
                    break;

                case CTracker::resizingTopRight:
                    imageRect.top = imagePt.y;
                    imageRect.right = imagePt.x;
                    if (imageRect.top >= imageRect.bottom)
                        imageRect.top = imageRect.bottom - 1;
                    if (imageRect.right <= imageRect.left)
                        imageRect.right = imageRect.left + 1;
                    break;

                case CTracker::resizingBottomLeft:
                    imageRect.left = imagePt.x;
                    imageRect.bottom = imagePt.y;
                    if (imageRect.left >= imageRect.right)
                        imageRect.left = imageRect.right - 1;
                    if (imageRect.bottom <= imageRect.top)
                        imageRect.bottom = imageRect.top + 1;
                    break;

                case CTracker::resizingBottomRight:
                    imageRect.right = imagePt.x;
                    imageRect.bottom = imagePt.y;
                    if (imageRect.right <= imageRect.left)
                        imageRect.right = imageRect.left + 1;
                    if (imageRect.bottom <= imageRect.top)
                        imageRect.bottom = imageRect.top + 1;
                    break;
                }

            if (theImgBrush.m_pImg == NULL && m_pImg->m_bTileGrid)
                {
                // Snap to tile grid...

                int cxTile = m_pImg->m_cxTile;
                if (cxTile != 1 && cxTile <= m_pImg->cxWidth)
                    {
                    imageRect.right = ((imageRect.right + cxTile / 2) /
                        cxTile) * cxTile;
                    }

                int cyTile = m_pImg->m_cyTile;
                if (cyTile != 1 && cyTile <= m_pImg->cyHeight)
                    {
                    imageRect.bottom = ((imageRect.bottom + cyTile / 2) /
                        cyTile) * cyTile;
                    }
                }

            SetStatusBarSize(imageRect.Size());

            ImageToClient(imageRect);

            c_pResizeDragger->Move(imageRect, TRUE);
            break;
        }
    }


/***************************************************************************/

void CImgWnd::StartSelectionDrag(unsigned code, CPoint newPt)
    {
    theImgBrush.CopyTo(theBackupBrush);

    newPt.x /= m_nZoom;
    newPt.y /= m_nZoom;

    mti.pt = mti.ptDown = mti.ptPrev = newPt;

    SetCapture();
    SetCombineMode(theImgBrush.m_bOpaque ? combineReplace : combineMatte);

    if (theImgBrush.m_bFirstDrag)
        {
        ASSERT(theImgBrush.m_pImg == m_pImg);

        PickupSelection();
        }
    else
        if (! theImgBrush.m_bOpaque)
            theImgBrush.RecalcMask( crRight );

    theImgBrush.TopLeftHandle();
    theImgBrush.m_dragOffset = mti.pt - theImgBrush.m_rcSelection.TopLeft();

    EraseTracker();

    if (GetKeyState(VK_CONTROL) < 0)
        {
        // Copy the selection and start moving...

        if (theImgBrush.m_bFirstDrag)
            {
            // The first time, the bits are already in
            // the bitmap, so just copy them to the
            // selection (which has already been done).

            theImgBrush.m_bFirstDrag = FALSE;
            theImgBrush.m_bLastDragWasFirst = TRUE;
            }
        else
            {
            CommitSelection(TRUE);
            }

        theImgBrush.m_bMoveSel = TRUE;
        }
    else
        if (GetKeyState(VK_SHIFT) < 0)
            {
            // Start a smear operation...
            HideBrush();

            if (theImgBrush.m_bLastDragWasFirst)
                CommitSelection(TRUE);

            SetUndo(m_pImg);
            theImgBrush.m_bSmearSel = TRUE;
            theImgBrush.m_bFirstDrag = FALSE;
            theImgBrush.m_bLastDragWasFirst = TRUE;
            }
        else
            {
            // Start a move operation...
            theImgBrush.m_bMoveSel = TRUE;
            }

    g_bCustomBrush = TRUE;
    }

/***************************************************************************/

void CImgWnd::CancelSelectionDrag()
    {
    if (!theImgBrush.m_bSmearSel && !theImgBrush.m_bMoveSel)
        {
        TRACE(TEXT("Extraneous CancelSelectionDrag!\n"));
        return;
        }

    ReleaseCapture();

    theImgBrush.m_rcSelection = theImgBrush.m_rcDraggedFrom;

    theImgBrush.m_bMoveSel = theImgBrush.m_bSmearSel = FALSE;
    g_bCustomBrush = FALSE;
    SetCombineMode(combineColor);

    theBackupBrush.CopyTo(theImgBrush);
    rcDragBrush = theImgBrush.m_rcSelection;
    rcDragBrush.right += 1;
    rcDragBrush.bottom += 1;

    CancelPainting();

    InvalImgRect(theImgBrush.m_pImg, NULL); // draw selection tracker

    // "Opaque" mode may have changed...
    if ((CImgTool::GetCurrentID() == IDMB_PICKTOOL)
    ||  (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL))
        g_pImgToolWnd->InvalidateOptions(FALSE);

    // Cancel all the way now...
    theImgBrush.m_pImg = NULL;
    }


/***************************************************************************/

void CImgWnd::SelectionDragHandler(unsigned code, CPoint newPt)
    {
    switch (code)
        {
        case WM_CANCEL:
            CancelSelectionDrag();
            break;

        case WM_MOUSEMOVE:
            if (theImgBrush.m_bMoveSel)
                PrepareForBrushChange(FALSE);

            mti.ptPrev = mti.pt;
            mti.pt = newPt;

            theImgBrush.m_rcSelection.OffsetRect(
                        -theImgBrush.m_rcSelection.TopLeft()
                           + (CSize)mti.pt - theImgBrush.m_dragOffset);

            // Make sure the selection stays at least along the edge
            // of the actual image so we don't lose the tracker...
            if (theImgBrush.m_rcSelection.left > m_pImg->cxWidth)
                theImgBrush.m_rcSelection.OffsetRect(-theImgBrush.m_rcSelection.left + m_pImg->cxWidth, 0);
            if (theImgBrush.m_rcSelection.top > m_pImg->cyHeight)
                theImgBrush.m_rcSelection.OffsetRect(0, -theImgBrush.m_rcSelection.top + m_pImg->cyHeight);
            if (theImgBrush.m_rcSelection.right < 0)
                theImgBrush.m_rcSelection.OffsetRect(-theImgBrush.m_rcSelection.right, 0);
            if (theImgBrush.m_rcSelection.bottom < 0)
                theImgBrush.m_rcSelection.OffsetRect(0, -theImgBrush.m_rcSelection.bottom);

            if (theImgBrush.m_bSmearSel)
                DrawBrush(m_pImg, theImgBrush.m_rcSelection.TopLeft(), TRUE);
            else
                ShowBrush(theImgBrush.m_rcSelection.TopLeft());
            break;

        case WM_LBUTTONUP:
            theImgBrush.m_bLastDragWasASmear = theImgBrush.m_bSmearSel;

            if (theImgBrush.m_bSmearSel)
                {
                IMG* pImg = m_pImg;

                CommitSelection(FALSE);

                FinishUndo(CRect(0, 0, pImg->cxWidth, pImg->cyHeight));
                }
            ReleaseCapture();

            theImgBrush.m_bMoveSel = theImgBrush.m_bSmearSel = FALSE;

            g_bCustomBrush = FALSE;
            SetCombineMode(combineColor);

            InvalImgRect(theImgBrush.m_pImg, NULL); // draw selection tracker
            break;
        }
    }

/******************************************************************************/

BOOL CImgWnd::PtInTracker( CPoint cptLocation )
    {
    CRect selRect = theImgBrush.m_rcSelection;
    BOOL  bPtInTracker = FALSE;

    selRect.left   *= m_nZoom;
    selRect.top    *= m_nZoom;
    selRect.right  *= m_nZoom;
    selRect.bottom *= m_nZoom;

    selRect.InflateRect( CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE );

    bPtInTracker = selRect.PtInRect( cptLocation );

    return bPtInTracker;
    }

/******************************************************************************/

void CImgWnd::OnRButtonDownInSel(CPoint *pcPointDown)
    {
    CMenu cMenuPopup;
    BOOL bRC = cMenuPopup.LoadMenu(IDR_SELECTION_POPUP);

    ASSERT(bRC);
    if (bRC)
        {
        CMenu *pcContextMenu = cMenuPopup.GetSubMenu(0);
        ASSERT(pcContextMenu != NULL);
        if (pcContextMenu != NULL)
            {
            CPoint cPointDown = *pcPointDown;
            ImageToClient(cPointDown);
            ClientToScreen(&cPointDown);

            CRect cRectClient;
            GetClientRect(&cRectClient);
            ClientToScreen(&cRectClient);

            // the frame actually has a clue about what items to enable...
            CWnd *notify = GetParentFrame();

            if (!notify)
                notify = GetParent(); // oh well...

            pcContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                cPointDown.x, cPointDown.y, notify, &cRectClient);
            }
        }
    }

/***************************************************************************/

void CImgWnd::ZoomedInDP(unsigned code, unsigned mouseKeys, CPoint newPt)
    {
    CPoint clientPt = newPt;
    CPoint imagePt  = clientPt;

    ClientToImage( imagePt );

    if (c_pResizeDragger != NULL)
        {
        ResizeMouseHandler( code, imagePt );
        return;
        }

    int iHandleSize = CTracker::HANDLE_SIZE;

    newPt.x -= iHandleSize + m_xScroll * m_nZoom;
    newPt.y -= iHandleSize + m_yScroll * m_nZoom;

//  AdjustPointForGrid(&newPt);

    IMG* pImg = m_pImg;

    int cxImage = pImg->cxWidth;
    int cyImage = pImg->cyHeight;

    CRect imageRect;

    GetImageRect( imageRect );

    // Check for selection manipulations...
    if (GetCapture() != this
    &&  c_pImgWndCur == this
    &&  theImgBrush.m_pImg == m_pImg)
        {
        CRect selRect = theImgBrush.m_rcSelection;
        BOOL bPtInTracker = FALSE;

        selRect.left   *= m_nZoom;
        selRect.top    *= m_nZoom;
        selRect.right  *= m_nZoom;
        selRect.bottom *= m_nZoom;

        selRect.InflateRect( CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE );

        bPtInTracker = PtInTracker(newPt);

        if (bPtInTracker)
            {
            // Mouse is within the outer border of the tracker...


            // We don't set the rubber for every mouse message when the
            // selection tool is active, but we'd better set it up now!
            if (pRubberImg != m_pImg)
                SetupRubber(m_pImg);

            ClearStatusBarPosition();

            CTracker::STATE state;

            selRect.InflateRect( -CTracker::HANDLE_SIZE,
                                 -CTracker::HANDLE_SIZE );

            state = CTracker::HitTest(selRect, newPt, CTracker::nil);

            if (bPtInTracker && state == CTracker::nil)
                {
                // Actually inside the selection...
                SetCursor(theApp.LoadCursor(IDCUR_MOVE));

                if (code == WM_LBUTTONDOWN || code == WM_LBUTTONDBLCLK)
                    {
                    StartSelectionDrag(code, newPt);
                    }
                else
                    {
                    if (code == WM_RBUTTONDOWN || code == WM_RBUTTONDBLCLK)
                        // some of the menu commands don't work for free form selections
                        OnRButtonDownInSel( &imagePt );
                    }
                }
            else
                {
                // In the tracker frame...

                SetCursor(HCursorFromTrackerState(state));

                if (code == WM_LBUTTONDOWN || code == WM_LBUTTONDBLCLK)
                    {
                    // Start a resize operation...
                    SetCapture();
                    PrepareForBrushChange();

                    ASSERT(c_pResizeDragger == NULL);
                    CRect rect = theImgBrush.m_rcSelection;
                    ImageToClient(rect);

                    c_pResizeDragger = new CDragger(this, &rect);
                    ASSERT(c_pResizeDragger != NULL);
                    c_dragState = state;
                    }
                }

            return;
            }
        }

    if (! imageRect.PtInRect( clientPt )
    &&    code         != WM_CANCEL
    &&    GetCapture() == NULL)
        {
        // The mouse is not inside the image and we're not in any
        // special mode, so hide the brush...
        if (g_pDragBrushWnd    == this
        &&  theImgBrush.m_pImg == NULL)
            HideBrush();

        CRect selRect = imageRect;

        selRect.InflateRect( CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE );

        if (theImgBrush.m_pImg != NULL || ! selRect.PtInRect( clientPt ))
            {
            // The mouse is not in the whole image tracker
            if (WM_LBUTTONDOWN == code)
            {
               if (CImgTool::GetCurrentID() != IDMX_TEXTTOOL)
               {
                  CmdCancel ();
               }

            }
            else
            {
               SetCursor( LoadCursor(NULL, IDC_ARROW ));
            }

            return;
            }

        // The mouse is in the whole image tracker, so set the cursor
        // as appropriate
        CTracker::STATE state = CTracker::nil;

        if (c_pImgWndCur == this)
            state = CTracker::HitTest(imageRect, clientPt, CTracker::nil);

        switch (state)
            {
            case CTracker::resizingTop:
            case CTracker::resizingLeft:
            case CTracker::resizingTopLeft:
            case CTracker::resizingTopRight:
            case CTracker::resizingBottomLeft:
               state = CTracker::nil;
               break;
            }

        SetCursor( HCursorFromTrackerState( state ) );

        // Handle mouse messages for tracker...
        if (state != CTracker::nil
        &&  (code == WM_LBUTTONDOWN || code == WM_LBUTTONDBLCLK))
            {
            SetCapture();

            ASSERT( c_pResizeDragger == NULL );

            c_pResizeDragger = new CDragger( this, &imageRect );

            ASSERT( c_pResizeDragger != NULL );

            c_dragState = state;
            }

        return;
        }

    newPt.x /= m_nZoom;
    newPt.y /= m_nZoom;

    if (! CImgTool::IsDragging())
        SetStatusBarPosition( m_ptDispPos );

    // Moving the selection??

    if (theImgBrush.m_bMoveSel
    ||  theImgBrush.m_bSmearSel)
        {
        SelectionDragHandler( code, newPt );

        return;
        }

    AdjustPointForGrid( &newPt );

    // Dispatch the event off to the current tool...

    CImgTool* pImgTool = CImgTool::GetCurrent();

    switch (code)
        {
        case WM_CANCEL:
            ReleaseCapture();
            pImgTool->OnCancel(this);
            mti.fLeft = mti.fRight = FALSE;
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            // We don't set the rubber for every mouse message when the
            // selection tool is active, but we'd better set it up now!
            if (pRubberImg != m_pImg)
                SetupRubber( m_pImg );

            mti.fLeft  = (code == WM_LBUTTONDOWN || code == WM_LBUTTONDBLCLK);
            mti.fRight = (code == WM_RBUTTONDOWN || code == WM_RBUTTONDBLCLK);

            mti.pt = mti.ptDown = mti.ptPrev = newPt;

            // if in the polygon tool, double clicks will end operation

            if (CImgTool::GetCurrentID() == IDMB_POLYGONTOOL
            &&  ((code == WM_LBUTTONDBLCLK) || (code == WM_RBUTTONDBLCLK)))
                {
                mti.ptPrev = mti.pt;
                mti.pt     = newPt;

                pImgTool->EndMultiptOperation(); // end the multipt operation
                pImgTool->OnEndDrag( this, &mti );

                mti.fLeft  = FALSE;
                mti.fRight = FALSE;

                break;
                }

            SetCapture();

            if (CImgTool::GetCurrentID() != IDMB_PICKRGNTOOL)
                HideBrush();

            if (pImgTool->IsUndoable())
                SetUndo(m_pImg);

            pImgTool->OnStartDrag( this, &mti );
            break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
            mti.ptPrev = mti.pt;
            mti.pt     = newPt;

            if (GetCapture() != this)
                break;

            ReleaseCapture();

            pImgTool->CanEndMultiptOperation( &mti );
            pImgTool->OnEndDrag( this, &mti );

            if (code == WM_LBUTTONUP)
                {
                mti.fLeft = FALSE;
                }
            if (code == WM_RBUTTONUP)
                {
                mti.fRight = FALSE;
                }
            break;

        case WM_MOUSEMOVE:
            mti.ptPrev = mti.pt;
            mti.pt = newPt;

            if (mti.fLeft || mti.fRight)
                pImgTool->OnDrag(this, &mti);
            else
                pImgTool->OnMove(this, &mti);
            break;

        case WM_TIMER:
            pImgTool->OnTimer( this, &mti );
            break;
        }

    UpdateWindow(); // For immediate feedback in active window
    SetToolCursor();
    }

/***************************************************************************/

void CImgWnd::FinishUndo(const CRect& rectUndo)
    {
        if ( EnsureUndoSize(m_pImg) )
        m_pImg->m_pBitmapObj->FinishUndo(&rectUndo);
        else
        {
        TRACE(TEXT("Problem: Can NOT ensure undo capability!\n"));
        MessageBeep(0);
        }
    }

/***************************************************************************/

void CImgWnd::CancelPainting()
    {
    if (g_hUndoImgBitmap == NULL)
        return; // nothing to cancel!

    IMG*     pimg;
    HDC      hTempDC;
    HBITMAP  hOldBM;
    HPALETTE hOldPalette = NULL;

    pimg = m_pImg;

    if ((hTempDC = CreateCompatibleDC( pimg->hDC )) == NULL)
        {
        TRACE(TEXT("Not enough memory to undo!\n"));
        MessageBeep(0);
        return;
        }

    HideBrush();

    if (g_hUndoPalette)
        {
        if (pimg->m_hPalOld)
            {
            ::SelectPalette( pimg->hDC, pimg->m_hPalOld, FALSE );
            pimg->m_hPalOld = NULL;
            }

        if (pimg->m_pPalette)
            pimg->m_pPalette->DeleteObject();

        pimg->m_pPalette->Attach( g_hUndoPalette );
        g_hUndoPalette = NULL;

        pimg->m_hPalOld = ::SelectPalette( pimg->hDC,
                                 (HPALETTE)pimg->m_pPalette->GetSafeHandle(), FALSE );
        ::RealizePalette( pimg->hDC );
        }

    hOldBM = (HBITMAP)SelectObject( hTempDC, g_hUndoImgBitmap );

    if (pimg->m_pPalette)
        {
        hOldPalette = ::SelectPalette( hTempDC,
                            (HPALETTE)pimg->m_pPalette->GetSafeHandle(), FALSE );
        ::RealizePalette( hTempDC );
        }

    ASSERT( hOldBM != NULL );

    BitBlt( pimg->hDC, 0, 0, pimg->cxWidth, pimg->cyHeight, hTempDC, 0, 0, SRCCOPY );

    if (hOldPalette != NULL)
        ::SelectPalette( hTempDC,  hOldPalette, FALSE );

    SelectObject( hTempDC, hOldBM );
    DeleteDC    ( hTempDC );

    InvalImgRect ( m_pImg, NULL );
    CommitImgRect( m_pImg, NULL );
    }

#ifdef  GRIDOPTIONS
/***************************************************************************/

void CImgWnd::CmdGridOptions()
    {
    CImgGridDlg dlg;

    dlg.m_bPixelGrid = theApp.m_bShowGrid;
    dlg.m_bTileGrid  = m_pImg->m_bTileGrid;
    dlg.m_nWidth     = m_pImg->m_cxTile;
    dlg.m_nHeight    = m_pImg->m_cyTile;

    if (dlg.DoModal() != IDOK)
        return;

    // Hide the current is dependant on the state of the grid...

    BOOL bOldShowGrid = theApp.m_bShowGrid;
    theApp.m_bShowGrid = dlg.m_bPixelGrid;
    m_pImg->m_bTileGrid = dlg.m_bTileGrid;
    m_pImg->m_cxTile = dlg.m_nWidth;
    m_pImg->m_cyTile = dlg.m_nHeight;

    InvalImgRect(m_pImg, NULL);

    if (bOldShowGrid != theApp.m_bShowGrid)
        {
        if (c_pImgWndCur != NULL)
            c_pImgWndCur->Invalidate(FALSE); // Redraw tracker
        }
    }
#endif  // GRIDOPTIONS

/***************************************************************************/

void CImgWnd::CmdShowGrid()
    {
    // Hide the current cross hair since the width of the lines
    // is dependant on the state of the grid...
    theApp.m_bShowGrid = ! theApp.m_bShowGrid;

    InvalImgRect(m_pImg, NULL);

    if (c_pImgWndCur != NULL)
        c_pImgWndCur->Invalidate(FALSE); // Redraw tracker
    }

/***************************************************************************/
// Draw a grid over the image already in the bitmap in pDC.  Drawing
// is optimized by restricting it to destRect.
//
void CImgWnd::DrawGrid(CDC* pDC, const CRect& srcRect, CRect& destRect)
    {
    ASSERT(pDC != NULL);
    ASSERT(m_pImg != NULL);

    pDC->SetTextColor(RGB(192, 192, 192));
    pDC->SetBkColor(RGB(128, 128, 128));

    CBrush* pOldBrush = pDC->SelectObject(GetHalftoneBrush());

    CRect gridRect(0, 0, m_pImg->cxWidth * m_nZoom + 1,
        m_pImg->cyHeight * m_nZoom + 1);

    for (int x = gridRect.left; x <= gridRect.right; x += m_nZoom)
        pDC->PatBlt(x, gridRect.top, 1, gridRect.Height(), PATCOPY);

    for (int y = gridRect.top; y <= gridRect.bottom; y += m_nZoom)
        pDC->PatBlt(gridRect.left, y, gridRect.Width(), 1, PATCOPY);

    if (m_pImg->m_bTileGrid)
        {
        pDC->SetTextColor(RGB(0, 0, 255));
        pDC->SetBkColor(RGB(0, 0, 128));

        int nWidth = destRect.Width();
        int nHeight = destRect.Height();
        int nStep;

        if (m_pImg->m_cxTile > 1 && m_pImg->m_cxTile <= m_pImg->cxWidth)
            {
            nStep = m_nZoom * m_pImg->m_cxTile;
            for (x = (m_pImg->m_cxTile - srcRect.left % m_pImg->m_cxTile -
                m_pImg->m_cxTile) * m_nZoom; x <= nWidth; x += nStep)
                {
                pDC->PatBlt(x, 0, 1, nHeight, PATCOPY);
                }
            }

        if (m_pImg->m_cyTile > 1 && m_pImg->m_cyTile <= m_pImg->cyHeight)
            {
            nStep = m_nZoom * m_pImg->m_cyTile;
            for (y = (m_pImg->m_cyTile - srcRect.top % m_pImg->m_cyTile -
            m_pImg->m_cyTile) * m_nZoom; y <= nHeight; y += nStep)
                {
                pDC->PatBlt(0, y, nWidth, 1, PATCOPY);
                }
            }
        }

    pDC->SelectObject(pOldBrush);

    destRect.right += 1;
    destRect.bottom += 1;
    }

#ifdef  GRIDOPTIONS
/***************************************************************************/

void CImgWnd::CmdShowTileGrid()
    {
    extern BOOL  g_bDefaultTileGrid;
    // If neither grid is visible, show both.  Otherwise leave the pixel
    // grid alone and toggle the tile grid.

    if (! theApp.m_bShowGrid)
        {
        m_pImg->m_bTileGrid = TRUE;
        theApp.m_bShowGrid = TRUE;
        }
    else
        {
        m_pImg->m_bTileGrid = !m_pImg->m_bTileGrid;
        }

    g_bDefaultTileGrid = m_pImg->m_bTileGrid;

    InvalImgRect(m_pImg, NULL);

    if (c_pImgWndCur != NULL)
        c_pImgWndCur->Invalidate(FALSE); // Redraw tracker
    }
#endif  // GRIDOPTIONS

/***************************************************************************/

void CImgWnd::MoveBrush( const CRect& newSelRect )
    {
    if (! theImgBrush.m_pImg)
        return;

    theImgBrush.m_rcSelection = newSelRect;
    InvalImgRect( theImgBrush.m_pImg, NULL );


    theImgBrush.m_handle.cx = theImgBrush.m_handle.cy = 0;

    BOOL bOldCustomBrush = g_bCustomBrush;

    g_bCustomBrush = TRUE;

    int wOldCombineMode = wCombineMode;

    SetCombineMode( theImgBrush.m_bOpaque ? combineReplace : combineMatte );
    ShowBrush( theImgBrush.m_rcSelection.TopLeft() );

    g_bCustomBrush = bOldCustomBrush;

    SetCombineMode( wOldCombineMode );
    }

/***************************************************************************/

BOOL CImgWnd::MakeBrush( HDC hSourceDC, CRect rcSource )
    {
    int       cxWidth;
    int       cyHeight;
    int       iToolID = CImgTool::GetCurrentID();


    if (rcSource.IsRectEmpty())
        {
        ASSERT( FALSE );

        return FALSE;
        }

    theImgBrush.m_size = rcSource.Size();

    cxWidth  = theImgBrush.m_size.cx;
    cyHeight = theImgBrush.m_size.cy;

    if (theImgBrush.m_hbmOld)
        ::SelectObject( theImgBrush.m_dc.m_hDC, theImgBrush.m_hbmOld );

    if (theImgBrush.m_hbmMaskOld)
        ::SelectObject( theImgBrush.m_dc.m_hDC, theImgBrush.m_hbmMaskOld );

    theImgBrush.m_hbmOld     = NULL;
    theImgBrush.m_hbmMaskOld = NULL;

    theImgBrush.m_dc.DeleteDC();
    theImgBrush.m_bitmap.DeleteObject();
    theImgBrush.m_maskDC.DeleteDC();
    theImgBrush.m_maskBitmap.DeleteObject();

    CDC* pdcSource = CDC::FromHandle( hSourceDC );
    CDC* pdcBitmap = CDC::FromHandle( m_pImg->hDC );

    if (! theImgBrush.m_bitmap.CreateCompatibleBitmap( pdcBitmap, cxWidth, cyHeight )
    ||  ! theImgBrush.m_dc.CreateCompatibleDC        ( pdcBitmap )
    ||  ! theImgBrush.m_maskBitmap.CreateBitmap      (            cxWidth, cyHeight, 1, 1, NULL)
    ||  ! theImgBrush.m_maskDC.CreateCompatibleDC    ( pdcBitmap ))
        {
        theApp.SetGdiEmergency();
        return FALSE;
        }

    theImgBrush.m_pImg       = m_pImg;
    theImgBrush.m_hbmOld     = (HBITMAP)((theImgBrush.m_dc.SelectObject(
                                         &theImgBrush.m_bitmap ))->GetSafeHandle());
    theImgBrush.m_hbmMaskOld = (HBITMAP)((theImgBrush.m_maskDC.SelectObject(
                                         &theImgBrush.m_maskBitmap ))->GetSafeHandle());

    CPalette* pcOldPalette = SetImgPalette( &theImgBrush.m_dc, FALSE );

    if (iToolID == IDMB_PICKRGNTOOL)
        {
        // Using StretchBlt to ensure palette mapping occurs
        TRY {
            CBrush cBrushWhite( PALETTERGB( 0xff, 0xff, 0xff ) );
            CRect  cRectTmp( 0, 0, cxWidth, cyHeight );

            theImgBrush.m_dc.FillRect( &cRectTmp, &cBrushWhite );
            }
        CATCH(CResourceException, e)
            {
            theApp.SetGdiEmergency();
            return FALSE;
            }
        END_CATCH

        if (theImgBrush.m_cRgnPolyFreeHandSel.GetSafeHandle())
            theImgBrush.m_dc.FillRgn( &theImgBrush.m_cRgnPolyFreeHandSel,
                                      CBrush::FromHandle( (HBRUSH)::GetStockObject( BLACK_BRUSH ) ) );

        theImgBrush.m_dc.StretchBlt( 0, 0, cxWidth, cyHeight,
                                        pdcSource,
                                        rcSource.left, rcSource.top,
                                        cxWidth, cyHeight, SRCERASE);
        }
    else
        {
        // Using StretchBlt to ensure palette mapping occurs
        theImgBrush.m_dc.StretchBlt( 0, 0, cxWidth, cyHeight,
                                           pdcSource,
                                           rcSource.left, rcSource.top,
                                           cxWidth, cyHeight, SRCCOPY );
        }

    theImgBrush.RecalcMask( crRight );

    if (pcOldPalette)
        theImgBrush.m_dc.SelectPalette( pcOldPalette, FALSE );

    theImgBrush.m_rcSelection = rcSource;

    rcSource.right  += 1;
    rcSource.bottom += 1;

    InvalImgRect( m_pImg, NULL ); // Redraw selection tracker

    rcDragBrush     = rcSource;
    g_bBrushVisible = TRUE;
    g_pDragBrushWnd = this;

    theImgBrush.m_bFirstDrag        = TRUE;
    theImgBrush.m_bLastDragWasFirst = FALSE;

    return TRUE;
    }

/***************************************************************************/

void CImgWnd::CmdClear()
    {
    if (TextToolProcessed( ID_EDIT_CLEAR ))
        return;

    HPALETTE hOldPalette = NULL;
    HBRUSH hNewBrush, hOldBrush;
    IMG* pImg = m_pImg;

    if ((hNewBrush = CreateSolidBrush( crRight )) == NULL)
        {
        theApp.SetGdiEmergency();
        return;
        }

    HideBrush();

    CRect clearRect;

    if (theImgBrush.m_pImg == NULL)
        clearRect.SetRect(0, 0, pImg->cxWidth, pImg->cyHeight);
    else
        {
        clearRect         = rcDragBrush;
        clearRect.right  -= 1;
        clearRect.bottom -= 1;
        }

    BOOL bUndo = FALSE;

    if (!theImgBrush.m_pImg || theImgBrush.m_bFirstDrag
            || theImgBrush.m_bCuttingFromImage)
        {
        bUndo = TRUE;
        SetUndo(m_pImg);

        if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
            {
            int     iPoints;
            CPoint* pptArray;
            BOOL    bData = ((CFreehandSelectTool*)CImgTool::GetCurrent())->CopyPointsToMemArray( &pptArray, &iPoints );
            if (bData && iPoints)
                {

                HRGN hrgn = ::CreatePolygonRgn( pptArray, iPoints, ALTERNATE );

                if (hrgn)
                    ::FillRgn( pImg->hDC, hrgn, hNewBrush );

                delete [] pptArray;
                }
            else
                {
                theApp.SetMemoryEmergency();
                return;
                }
            }
        else
            {
            hOldBrush = (HBRUSH)SelectObject(pImg->hDC, hNewBrush);

            PatBlt( pImg->hDC, clearRect.left,
                            clearRect.top,
                            clearRect.Width(),
                            clearRect.Height(), PATCOPY );
            SelectObject( pImg->hDC, hOldBrush );
            }
        }

    InvalImgRect ( m_pImg, &clearRect );
    CommitImgRect( m_pImg, &clearRect );

    if (bUndo)
        FinishUndo(clearRect);

    DirtyImg     ( m_pImg );
    DeleteObject ( hNewBrush );

    // If we have a selection, nuke it since it's useless now...
    if (theImgBrush.m_pImg != NULL)
        {
        if (theImgBrush.m_bLastDragWasFirst)
            {
            theImgBrush.m_bLastDragWasFirst = FALSE;
            FinishUndo(theImgBrush.m_rcDraggedFrom);
            }

        theImgBrush.m_handle.cx = 0;
        theImgBrush.m_handle.cy = 0;
        theImgBrush.m_bMoveSel = theImgBrush.m_bSmearSel = FALSE;
        g_bCustomBrush = FALSE;
        SetCombineMode(combineColor);

        InvalImgRect(theImgBrush.m_pImg, NULL);  // redraw selection
        theImgBrush.m_pImg = NULL;
        }
    }


/***************************************************************************/

void CImgWnd::CmdFlipBshH()
    {
    IMG* pImg = m_pImg;

    HideBrush();

    CRect flipRect;

    if (theImgBrush.m_pImg == NULL && !g_bCustomBrush)
        {
        flipRect.SetRect(0, 0, pImg->cxWidth, pImg->cyHeight);
        }
    else
        {
        flipRect = rcDragBrush;
        flipRect.right -= 1;
        flipRect.bottom -= 1;
        }

    if (  theImgBrush.m_pImg != NULL
    &&  ! theImgBrush.m_bFirstDrag || g_bCustomBrush)
        {
        CPalette* ppal = SetImgPalette( &theImgBrush.m_dc, FALSE );
        //
        // Don't do halftone blts when just moving bits around
        //
        theImgBrush.m_dc.SetStretchBltMode (COLORONCOLOR);

        StretchCopy(theImgBrush.m_dc.m_hDC, 0, 0,
                    theImgBrush.m_size.cx,
                    theImgBrush.m_size.cy,
                    theImgBrush.m_dc.m_hDC,
                    theImgBrush.m_size.cx - 1, 0,
                   -theImgBrush.m_size.cx,
                    theImgBrush.m_size.cy);

        StretchCopy(theImgBrush.m_maskDC.m_hDC, 0, 0,
                    theImgBrush.m_size.cx,
                    theImgBrush.m_size.cy,
                    theImgBrush.m_maskDC.m_hDC,
                    theImgBrush.m_size.cx - 1, 0,
                   -theImgBrush.m_size.cx,
                    theImgBrush.m_size.cy);

        if (ppal)
            theImgBrush.m_dc.SelectPalette( ppal, FALSE ); // Background ??

        MoveBrush(theImgBrush.m_rcSelection);
        }
    else
        {
        SetUndo(m_pImg);
        SetStretchBltMode (pImg->hDC, COLORONCOLOR);
        StretchCopy(pImg->hDC, flipRect.left,
                               flipRect.top,
                               flipRect.Width(),
                               flipRect.Height(),
                    pImg->hDC, flipRect.left + flipRect.Width() - 1,
                               flipRect.top,
                              -flipRect.Width(),
                               flipRect.Height());

        InvalImgRect (m_pImg, &flipRect);
        CommitImgRect(m_pImg, &flipRect);
        FinishUndo   (flipRect);
        DirtyImg     (m_pImg);
        }
    }

/***************************************************************************/

void CImgWnd::CmdFlipBshV()
    {
    IMG* pImg = m_pImg;

    HideBrush();

    CRect flipRect;

    if (theImgBrush.m_pImg == NULL && !g_bCustomBrush)
        {
        flipRect.SetRect(0, 0, pImg->cxWidth, pImg->cyHeight);
        }
    else
        {
        flipRect = rcDragBrush;
        flipRect.right -= 1;
        flipRect.bottom -= 1;
        }

    if (  theImgBrush.m_pImg != NULL
    &&  ! theImgBrush.m_bFirstDrag || g_bCustomBrush)
        {
        CPalette* ppal = SetImgPalette( &theImgBrush.m_dc, FALSE ); // Background ??
        theImgBrush.m_dc.SetStretchBltMode (COLORONCOLOR);
        StretchCopy(theImgBrush.m_dc.m_hDC, 0, 0,
                    theImgBrush.m_size.cx,
                    theImgBrush.m_size.cy,
                    theImgBrush.m_dc.m_hDC, 0,
                    theImgBrush.m_size.cy - 1,
                    theImgBrush.m_size.cx,
                   -theImgBrush.m_size.cy);

        StretchCopy(theImgBrush.m_maskDC.m_hDC, 0, 0,
                    theImgBrush.m_size.cx,
                    theImgBrush.m_size.cy,
                    theImgBrush.m_maskDC.m_hDC, 0,
                    theImgBrush.m_size.cy - 1,
                    theImgBrush.m_size.cx,
                   -theImgBrush.m_size.cy);

        if (ppal)
            theImgBrush.m_dc.SelectPalette( ppal, FALSE ); // Background ??

        MoveBrush(theImgBrush.m_rcSelection);
        }
    else
        {
        SetUndo(m_pImg);
        SetStretchBltMode (pImg->hDC, COLORONCOLOR);
        StretchCopy(pImg->hDC, flipRect.left,    flipRect.top,
                               flipRect.Width(), flipRect.Height(),
                    pImg->hDC, flipRect.left,    flipRect.top + flipRect.Height() - 1,
                               flipRect.Width(), -flipRect.Height());

        InvalImgRect (m_pImg, &flipRect);
        CommitImgRect(m_pImg, &flipRect);
        FinishUndo   (flipRect);
        DirtyImg     (m_pImg);
        }
    }

/***************************************************************************/

void CImgWnd::CmdDoubleBsh()
    {
    if (!g_bCustomBrush && theImgBrush.m_pImg == NULL)
        {
        MessageBeep(0);
        return;
        }

    PrepareForBrushChange(TRUE, FALSE);

    CRect rc  =           theImgBrush.m_rcSelection;
    rc.left  -=           theImgBrush.m_size.cx / 2;
    rc.right  = rc.left + theImgBrush.m_size.cx * 2;
    rc.top   -=           theImgBrush.m_size.cy / 2;
    rc.bottom = rc.top  + theImgBrush.m_size.cy * 2;

    HideBrush();

    theImgBrush.SetSize( CSize( theImgBrush.m_size.cx * 2,
                                theImgBrush.m_size.cy * 2 ) );
    MoveBrush(rc);

    if (g_bCustomBrush)
        theImgBrush.CenterHandle();
    }

/***************************************************************************/

void CImgWnd::CmdHalfBsh()
    {
    if (! g_bCustomBrush
    &&  ! theImgBrush.m_pImg)
        {
        MessageBeep(0);
        return;
        }

    PrepareForBrushChange( TRUE, FALSE );

    CRect rc  =            theImgBrush.m_rcSelection;
    rc.left  +=            theImgBrush.m_size.cx / 4;
    rc.right  = rc.left + (theImgBrush.m_size.cx + 1) / 2;
    rc.top   +=            theImgBrush.m_size.cy / 4;
    rc.bottom = rc.top  + (theImgBrush.m_size.cy + 1) / 2;

    HideBrush();

    theImgBrush.SetSize( CSize( (theImgBrush.m_size.cx + 1) / 2,
                                (theImgBrush.m_size.cy + 1) / 2 ) );
    MoveBrush( rc );

    if (g_bCustomBrush)
        theImgBrush.CenterHandle();
    }

/***************************************************************************/

CPalette* CImgWnd::FixupDibPalette( LPSTR lpDib, CPalette* ppalDib )
    {
    CPBView* pView = (CPBView*)GetParent();
    CPBDoc*  pDoc  = pView->GetDocument();

    if (pDoc == NULL || lpDib == NULL || ppalDib == NULL || pDoc->m_pBitmapObj->m_pImg == NULL)
        return ppalDib;

    IMG* pImg         = pDoc->m_pBitmapObj->m_pImg;
    int  iColorBits   = pImg->cBitCount * pImg->cPlanes;
    BOOL bFixupDib    = TRUE;
    BOOL bSwapPalette = TRUE;

    // only if dealing with palettes
    if (iColorBits != 8)
        return ppalDib;

    CPalette* ppalPic = theApp.m_pPalette;
    CPalette* ppalNew = NULL;
        BOOL      bMergedPalette = FALSE;

    if (ppalPic)
        {
        int iAdds;

        if ( ppalNew = MergePalettes( ppalPic, ppalDib, iAdds ) )
                        bMergedPalette = TRUE;

        if (ppalNew)
            {
            if (! iAdds)
                {
                bSwapPalette = FALSE;
                                if ( bMergedPalette )
                                        {
                                        delete ppalNew;
                                        ppalNew = FALSE;
                                        bMergedPalette = FALSE;
                                        }
                ppalNew = ppalPic;
                }
            }
        else
            {
            bSwapPalette = FALSE;
                        if ( bMergedPalette )
                                {
                                delete ppalNew;
                                ppalNew = FALSE;
                                bMergedPalette = FALSE;
                                }
            ppalNew = ppalPic;
            }
        }
    else
        {
                if ( bMergedPalette )
                        {
                        delete ppalNew;
                        ppalNew = FALSE;
                        bMergedPalette = FALSE;
                        }
        ppalNew   = ppalDib;
        bFixupDib = FALSE;
        }

    if (bFixupDib)
        {
        LOGPALETTE256    palette;
        COLORREF         crCurColor;
        UINT             uColorIndex;
        int              iDibColors   = DIBNumColors( lpDib );
        BOOL             bWinStyleDIB = IS_WIN30_DIB( lpDib );
        LPBITMAPINFO     lpDibInfo    = (LPBITMAPINFO)lpDib;
        LPBITMAPCOREINFO lpCoreInfo   = (LPBITMAPCOREINFO)lpDib;

        palette.palVersion    = 0x300;
        palette.palNumEntries = (WORD)ppalNew->GetPaletteEntries( 0, 256,
                                                              &palette.palPalEntry[0] );
                                ppalNew->GetPaletteEntries( 0, palette.palNumEntries,
                                                              &palette.palPalEntry[0] );
        for (int iLoop = 0; iLoop < iDibColors; iLoop++)
            {
            if (bWinStyleDIB)
                {
                crCurColor = PALETTERGB( lpDibInfo->bmiColors[iLoop].rgbRed,
                                         lpDibInfo->bmiColors[iLoop].rgbGreen,
                                         lpDibInfo->bmiColors[iLoop].rgbBlue );
                }
            else
                {
                crCurColor = PALETTERGB( lpCoreInfo->bmciColors[iLoop].rgbtRed,
                                         lpCoreInfo->bmciColors[iLoop].rgbtGreen,
                                         lpCoreInfo->bmciColors[iLoop].rgbtBlue );
                }
            uColorIndex = ppalNew->GetNearestPaletteIndex( crCurColor );

            if (bWinStyleDIB)
                {
                lpDibInfo->bmiColors[iLoop].rgbRed   = palette.palPalEntry[uColorIndex].peRed;
                lpDibInfo->bmiColors[iLoop].rgbGreen = palette.palPalEntry[uColorIndex].peGreen;
                lpDibInfo->bmiColors[iLoop].rgbBlue  = palette.palPalEntry[uColorIndex].peBlue;
                }
            else
                {
                lpCoreInfo->bmciColors[iLoop].rgbtRed   = palette.palPalEntry[uColorIndex].peRed;
                lpCoreInfo->bmciColors[iLoop].rgbtGreen = palette.palPalEntry[uColorIndex].peGreen;
                lpCoreInfo->bmciColors[iLoop].rgbtBlue  = palette.palPalEntry[uColorIndex].peBlue;
                }
            }
        if (! bSwapPalette)
                        {
                        if ( bMergedPalette )
                                {
                                delete ppalNew;
                                bMergedPalette = FALSE;
                                }
            ppalNew = NULL;
                        }
        }

    if (bSwapPalette)
        {
        if (pImg->m_hPalOld)
            {
            ::SelectPalette( pImg->hDC, pImg->m_hPalOld, FALSE );
            pImg->m_hPalOld = NULL;
            }

        if (pImg->m_pPalette)
            delete pImg->m_pPalette;

        pImg->m_pPalette = ppalNew;
        pImg->m_hPalOld  = ::SelectPalette( pImg->hDC,
                                     (HPALETTE)ppalNew->GetSafeHandle(), FALSE );
        ::RealizePalette( pImg->hDC );
        InvalImgRect( pImg, NULL );

        // Return NULL since we swapped the new palette into the pImg!
        ppalNew = NULL;

        theApp.m_pPalette = pImg->m_pPalette;

        //
        // now that we changed the app palette update the DIB Section
        // color table too.
        //
        DWORD rgb[256];
        int i,n;

        n = theApp.m_pPalette->GetPaletteEntries(0, 256, (LPPALETTEENTRY)rgb);
        for (i=0; i<n; i++)
            rgb[i] = RGB(GetBValue(rgb[i]),GetGValue(rgb[i]),GetRValue(rgb[i]));
        SetDIBColorTable(pImg->hDC, 0, n, (LPRGBQUAD)rgb);
        }

        // Delete any orphaned ppalDib pointers.
        if ( ppalDib && ppalDib != ppalNew )
                delete ppalDib;

    return ppalNew;
    }

/***************************************************************************/

void CImgWnd::ShowBrush(CPoint ptHandle)
    {
    IMG * pimg = m_pImg;

    HideBrush();

    COLORREF crRealLeftColor;
    COLORREF crRealRightColor;

    int nStrokeWidth = CImgTool::GetCurrent()->GetStrokeWidth();
    int nStrokeShape = CImgTool::GetCurrent()->GetStrokeShape();

    if (CImgTool::GetCurrentID() == IDMB_ERASERTOOL)
        {
        crRealRightColor = crRight;
        crRealLeftColor  = crLeft;

        crLeft = crRight;
        }

    g_pDragBrushWnd = this;

    if (g_bCustomBrush)
        {
        int nCombineMode = (theImgBrush.m_bOpaque) ? combineReplace : combineMatte;

        rcDragBrush.SetRect(ptHandle.x, ptHandle.y,
                            ptHandle.x + theImgBrush.m_size.cx,
                            ptHandle.y + theImgBrush.m_size.cy);
        rcDragBrush -= (CPoint)theImgBrush.m_handle;

        theImgBrush.m_rcSelection = rcDragBrush;
        rcDragBrush.right  += 1;
        rcDragBrush.bottom += 1;

        if (CImgTool::GetCurrentID() == IDMX_TEXTTOOL)
            {
//          extern CTextTool g_textTool;
//          g_textTool.Render(CDC::FromHandle(pimg->hDC),
//                          rcDragBrush, TRUE, FALSE);
            }
        else
            {
            switch (nCombineMode)
                {
                case combineColor:
                    theImgBrush.BltColor(pimg, rcDragBrush.TopLeft(), crLeft);
                    break;

                case combineMatte:
                    theImgBrush.BltMatte(pimg, rcDragBrush.TopLeft());
                    break;

                case combineReplace:
                    theImgBrush.BltReplace(pimg, rcDragBrush.TopLeft());
                    break;
                }
            }

        InvalImgRect(m_pImg, &rcDragBrush);
        }
    else
        {
        DrawImgLine(m_pImg, ptHandle, ptHandle, crLeft,
                                          nStrokeWidth, nStrokeShape, FALSE);
        rcDragBrush.left   = ptHandle.x - nStrokeWidth / 2;
        rcDragBrush.top    = ptHandle.y - nStrokeWidth / 2;
        rcDragBrush.right  = rcDragBrush.left + nStrokeWidth;
        rcDragBrush.bottom = rcDragBrush.top  + nStrokeWidth;
        }

    if (CImgTool::GetCurrentID() == IDMB_ERASERTOOL)
        {
        crLeft  = crRealLeftColor;
        crRight = crRealRightColor;
        }

    g_bBrushVisible = TRUE;
    }

/***************************************************************************/

void CImgWnd::CmdSmallBrush()
    {
    if (CImgTool::GetCurrent()->GetStrokeWidth() != 0)
        CImgTool::GetCurrent()->SetStrokeWidth(1);
    }

/***************************************************************************/

void CImgWnd::CmdSmallerBrush()
    {
    if (theImgBrush.m_pImg != NULL || g_bCustomBrush)
        {
        CmdHalfBsh();
        return;
        }

    UINT nStrokeWidth = CImgTool::GetCurrent()->GetStrokeWidth();

    if (nStrokeWidth > 1)
        CImgTool::GetCurrent()->SetStrokeWidth(nStrokeWidth - 1);
    }

/***************************************************************************/

void CImgWnd::CmdLargerBrush()
    {
    if (theImgBrush.m_pImg != NULL || g_bCustomBrush)
        {
        CmdDoubleBsh();
        return;
        }

    UINT nStrokeWidth = CImgTool::GetCurrent()->GetStrokeWidth();

    CImgTool::GetCurrent()->SetStrokeWidth(nStrokeWidth + 1);
    }

/***************************************************************************/

void CImgWnd::CmdOK()
    {
    if (GetCapture() != NULL)
        {
        MessageBeep(0);
        return;
        }
    }

/***************************************************************************/
// Draw the tracker for this view (if it's the active one) into pDC.
// If pDC is NULL, one will be provided.  Optimize drawing by limiting
// it to pPaintRect.  If pPaintRect is NULL, draw the whole tracker.
//
void CImgWnd::DrawTracker( CDC* pDC, const CRect* pPaintRect )
    {
    BOOL bDrawTrackerRgn = FALSE;

    if (c_pImgWndCur != this
    ||  theImgBrush.m_bMoveSel
    ||  theImgBrush.m_bSmearSel
    ||  theImgBrush.m_bMakingSelection)
        {
        // This is not the active view, or the user is doing something
        // to prevent the tracker from appearing.
        return;
        }

    BOOL bReleaseDC = FALSE;
    CRect clientRect;

    if (pDC == NULL)
        {
        pDC = GetDC();

        if (pDC == NULL)
            {
            theApp.SetGdiEmergency(FALSE);
            return;
            }
        bReleaseDC = TRUE;
        }

   if (pPaintRect == NULL)
        {
        GetClientRect(&clientRect);
        pPaintRect = &clientRect;
        }

    CRect trackerRect;

    GetImageRect( trackerRect );

    trackerRect.InflateRect( CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE );

    CTracker::EDGES edges = (CTracker::EDGES)(CTracker::right | CTracker::bottom);

    if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
        {
        bDrawTrackerRgn = TRUE;
        }

    if (m_pImg == theImgBrush.m_pImg)
        {
        edges = CTracker::all;

        trackerRect = theImgBrush.m_rcSelection;

        trackerRect.left   *= m_nZoom;
        trackerRect.top    *= m_nZoom;
        trackerRect.right  *= m_nZoom;
        trackerRect.bottom *= m_nZoom;

        trackerRect.InflateRect( CTracker::HANDLE_SIZE,
                                 CTracker::HANDLE_SIZE);
        trackerRect.OffsetRect(  CTracker::HANDLE_SIZE + m_xScroll * m_nZoom,
                                 CTracker::HANDLE_SIZE + m_yScroll * m_nZoom);

        if (IsGridVisible())
            {
            trackerRect.right  += 1;
            trackerRect.bottom += 1;
            }
        }

    CTracker::DrawBorder (pDC, trackerRect, edges );
    CTracker::DrawHandles(pDC, trackerRect, edges );

    if (bReleaseDC)
        ReleaseDC(pDC);
    }

/***************************************************************************/
// Erase the tracker from this window.  Handles whole image as well
// as selection trackers.
//
void CImgWnd::EraseTracker()
    {
    if (m_pImg == NULL)
        return;

    CClientDC dc(this);

    if (dc.m_hDC == NULL)
        {
        theApp.SetGdiEmergency(FALSE);
        return;
        }

    CRect trackerRect;

    if (m_pImg == theImgBrush.m_pImg)
        {
        // Tracker is a selection within the image

        trackerRect = theImgBrush.m_rcSelection;
        ImageToClient(trackerRect);
        trackerRect.InflateRect(CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE);

        if (IsGridVisible())
            {
            trackerRect.right += 1;
            trackerRect.bottom += 1;
            }

        InvalidateRect( &trackerRect, FALSE );
        }
    else
        {
        // Tracker is around entire image

        GetImageRect(trackerRect);
        trackerRect.InflateRect(CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE);
        DrawBackground(&dc, &trackerRect);
        }
    }

/***************************************************************************/

void CImgWnd::CmdTglOpaque()
    {
    HideBrush();
    theImgBrush.m_bOpaque = !theImgBrush.m_bOpaque;
    theImgBrush.RecalcMask( crRight );

    MoveBrush( theImgBrush.m_rcSelection );

    if ((CImgTool::GetCurrentID() == IDMB_PICKTOOL)
    ||  (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL))
        g_pImgToolWnd->InvalidateOptions( FALSE );
    }

/***************************************************************************/

void CImgWnd::CmdInvertColors()
    {
    IMG* pImg = m_pImg;

    HideBrush();

    CRect invertRect;
    if (theImgBrush.m_pImg == NULL && !g_bCustomBrush)
        {
        invertRect.SetRect(0, 0, pImg->cxWidth, pImg->cyHeight);
        }
    else
        {
        invertRect = rcDragBrush;
        invertRect.right -= 1;
        invertRect.bottom -= 1;
        }

    if (theImgBrush.m_pImg != NULL && ! theImgBrush.m_bFirstDrag || g_bCustomBrush)
        {
        CPalette* ppal = SetImgPalette( &theImgBrush.m_dc, FALSE );

        theImgBrush.m_dc.PatBlt(0, 0, theImgBrush.m_size.cx,
                                      theImgBrush.m_size.cy, DSTINVERT);

        if (ppal)
            theImgBrush.m_dc.SelectPalette( ppal, FALSE ); // Background ??

        theImgBrush.RecalcMask( crRight );
        MoveBrush( theImgBrush.m_rcSelection );
        }
    else
        {
        SetUndo( m_pImg );

        PatBlt( pImg->hDC, invertRect.left, invertRect.top,
                invertRect.Width(), invertRect.Height(), DSTINVERT );

        InvalImgRect ( m_pImg, &invertRect );
        CommitImgRect( m_pImg, &invertRect );
        FinishUndo( invertRect );
        DirtyImg( m_pImg );
        }
    }

/***************************************************************************/

void CImgWnd::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
    {
    CWnd::OnKeyDown( nChar, nRepCnt, nFlags );
    }

/***************************************************************************/

void CImgWnd::OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags )
    {
    CWnd::OnKeyUp( nChar, nRepCnt, nFlags );
    }

/***************************************************************************/
