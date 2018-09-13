#include "precomp.h"

#include "ZoomWnd.h"
#include "resource.h"

#define MIN(x,y)    ((x<y)?x:y)
#define MAX(x,y)    ((x>y)?x:y)

/////////////////////////////////////////////////////////////////////////////
// CZoomWnd

CZoomWnd::CZoomWnd()
{
    m_modeDefault = MODE_ZOOMIN;
    m_fPanning = false;
    m_fCtrlDown = false;
    m_fShiftDown = false;
    m_bBestFit = true;

    m_cxImage = 1;
    m_cyImage = 1;
    m_cxCenter = 1;
    m_cyCenter = 1;
    m_hbitmap = 0;
    m_pImgCtx = NULL;

    m_cyHScroll = GetSystemMetrics( SM_CYHSCROLL );
    m_cxVScroll = GetSystemMetrics( SM_CXVSCROLL );

    m_iStrID = IDS_NOPREVIEW;

    m_hpal = NULL;
    m_fTransparent = false;
}

CZoomWnd::~CZoomWnd()
{
    if ( m_pImgCtx )
    {
        // Do not disconnect, other zoom windows might be using this same interface
        m_pImgCtx->Release();
    }
}

// OnEraseBkgnd
//
// Handles WM_ERASEBKGND messages sent to the window

LRESULT CZoomWnd::OnEraseBkgnd(UINT , WPARAM wParam, LPARAM , BOOL& )
{
#define COLOR_PREVIEWBKGND COLOR_WINDOW

    RECT rcFill;    // rect to fill with background color
    HDC hdc = (HDC)wParam;

    if ( m_fTransparent )
    {
        // when we have a transparent image we are forced to erase the entire background.
        // This can lead to visual flicker when panning or zooming transparent images
        // but I don't know of any other way to handle this.
        rcFill.left = 0;
        rcFill.top = 0;
        rcFill.right = m_cxWindow;
        rcFill.bottom = m_cyWindow;
        
        FillRect( hdc, &rcFill, (HBRUSH)(COLOR_PREVIEWBKGND+1));
    }
    else
    {
        // There are four possible regions that might need to be erased:
        //      +-----------------------+
        //      |       Erase Top       |
        //      +-------+-------+-------+
        //      |       |       |       |
        //      | Erase | Image | Erase |
        //      | Left  |       | Right |
        //      +-------+-------+-------+
        //      |     Erase Bottom      |
        //      +-----------------------+

        // erase the left region
        rcFill.left = 0;
        rcFill.top = m_ptszDest.y;
        rcFill.right = m_ptszDest.x;
        rcFill.bottom = m_ptszDest.y + m_ptszDest.cy;
        if ( rcFill.right > rcFill.left )
            FillRect( hdc, &rcFill, (HBRUSH)(COLOR_PREVIEWBKGND+1));

        // erase the right region
        rcFill.left = m_ptszDest.x + m_ptszDest.cx;
        rcFill.right = m_cxWindow;
        if ( rcFill.right > rcFill.left )
            FillRect( hdc, &rcFill, (HBRUSH)(COLOR_PREVIEWBKGND+1));

        // erase the top region
        rcFill.left = 0;
        rcFill.top = 0;
        rcFill.right = m_cxWindow;
        rcFill.bottom = m_ptszDest.y;
        if ( rcFill.bottom > rcFill.top )
            FillRect( hdc, &rcFill, (HBRUSH)(COLOR_PREVIEWBKGND+1));

        // erase the bottom region
        rcFill.top = m_ptszDest.y + m_ptszDest.cy;
        rcFill.bottom = m_cyWindow;
        if ( rcFill.bottom > rcFill.top )
            FillRect( hdc, &rcFill, (HBRUSH)(COLOR_PREVIEWBKGND+1));
    }

    return TRUE;

#undef COLOR_PREVIEWBKGND
}

// OnPaint
//
// Handles WM_PAINT messages sent to the window

LRESULT CZoomWnd::OnPaint(UINT , WPARAM , LPARAM , BOOL& )
{
    PAINTSTRUCT ps;
    HDC hdcDraw = BeginPaint( &ps );

    // setup the destination DC:
    SetMapMode( hdcDraw, MM_TEXT );
    SetStretchBltMode( hdcDraw, COLORONCOLOR );

    if ( m_hpal )
    {
        SelectPalette(hdcDraw, m_hpal, TRUE);
        RealizePalette(hdcDraw);
    }

    if ( m_pImgCtx )
    {
        m_pImgCtx->StretchBlt(
            hdcDraw,
            m_ptszDest.x,
            m_ptszDest.y,
            m_ptszDest.cx,
            m_ptszDest.cy,
            0,0,
            m_cxImage,
            m_cyImage,
            SRCCOPY );
    }
    else if ( m_hbitmap)
    {
        // create the source DC:
        HDC hdcBitmap = CreateCompatibleDC( hdcDraw );
        SelectObject( hdcBitmap, m_hbitmap );

        // and blit using the destination rectangle info
        StretchBlt(
            hdcDraw,
            m_ptszDest.x,
            m_ptszDest.y,
            m_ptszDest.cx,
            m_ptszDest.cy,
            hdcBitmap,
            0,0,
            m_cxImage,
            m_cyImage,
            SRCCOPY );
        DeleteDC( hdcBitmap );
    }
    else
    {
        RECT rc = { 0,0,m_cxWindow,m_cyWindow };
        TCHAR szBuf[80];
        LOGFONT lf;
        HFONT hFont;
        HFONT hFontOld;

        LoadString( _Module.GetModuleInstance(),
                    m_iStrID,
                    szBuf,
                    sizeof(szBuf) );

        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
        hFont = CreateFontIndirect(&lf);
        hFontOld = (HFONT)SelectObject(hdcDraw, hFont);

        SetTextColor(hdcDraw, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdcDraw, GetSysColor(COLOR_WINDOW));

        FillRect(hdcDraw, &rc, (HBRUSH)(COLOR_WINDOW+1));
        DrawText(hdcDraw, szBuf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdcDraw, hFontOld);
        DeleteObject(hFont);
    }

    EndPaint( &ps );
    return 0;
}

// OnSetCursor
//
// Handles WM_SETCURSOR messages sent to the window.
//
// This function is a total HackMaster job.  I have overloaded it's functionality to the point
// of abserdity.  Here's what the parameters mean:
//
// uMsg == WM_SETCURSOR
//      wParam  Standard value sent during a WM_SETCURSOR messge.
//      lParam  Standard value sent during a WM_SETCURSOR messge.
//
// uMsg == 0
//      wParam  0
//      lParam  If this value is non-zero then it is a packed x,y cursor location.
//              If it's zero then we need to query the cursor location

LRESULT CZoomWnd::OnSetCursor(UINT uMsg, WPARAM, LPARAM lParam, BOOL& bHandled)
{
    // if this is a legitimate message but isn't intended for the client area, we ignore it.
    // we also ignore set cursor when we have no valid bitmap
    if ( ((WM_SETCURSOR == uMsg) && (HTCLIENT != LOWORD(lParam))) || (!m_hbitmap && !m_pImgCtx) )
    {
        bHandled = FALSE;
        return 0;
    }
    else if ( 0 == uMsg )
    {
        // Since this is one of our fake messages we need to do our own check to test for HTCLIENT.
        // we need to find the cursor location
        POINT pt;
        GetCursorPos( &pt );
        lParam = MAKELONG(pt.x, pt.y);
        if ( HTCLIENT != SendMessage( WM_NCHITTEST, 0, lParam ) )
        {
            bHandled = FALSE;
            return 0;
        }
    }

    WORD idCur;

    if ( m_fPanning )
    {
        idCur = IDC_CLOSEDHAND;
    }
    else if ( m_fCtrlDown )
    {
        idCur = IDC_OPENHAND;
    }
    else if ( (m_modeDefault == MODE_ZOOMIN) ^ m_fShiftDown )
    {
        idCur = IDC_ZOOMIN;
    }
    else
    {
        idCur = IDC_ZOOMOUT;
    }

    SetCursor( LoadCursor(_Module.GetModuleInstance(), MAKEINTRESOURCE(idCur)) );
    return TRUE;
}

// OnKeyUp
//
// Handles WM_KEYUP messages sent to the window
LRESULT CZoomWnd::OnKeyUp(UINT , WPARAM wParam, LPARAM , BOOL& bHandled)
{
    if ( VK_CONTROL == wParam )
    {
        m_fCtrlDown = false;
        OnSetCursor( 0,0,0, bHandled );
    }
    else if (VK_SHIFT == wParam)
    {
        m_fShiftDown = false;
        OnSetCursor( 0,0,0, bHandled );
    }
    bHandled = FALSE;
    return 0;
}
  
// OnKeyDown
//
// Handles WM_KEYDOWN messages sent to the window
LRESULT CZoomWnd::OnKeyDown(UINT , WPARAM wParam, LPARAM , BOOL& bHandled)
{
    // when we return, we want to call the DefWindowProc
    bHandled = false;

    switch ( wParam )
    {
    case VK_PRIOR:
        OnScroll(WM_VSCROLL, m_fCtrlDown?SB_TOP:SB_PAGEUP, 0, bHandled);
        break;

    case VK_NEXT:
        OnScroll(WM_VSCROLL, m_fCtrlDown?SB_BOTTOM:SB_PAGEDOWN, 0, bHandled);
        break;

    case VK_END:
        OnScroll(WM_HSCROLL, m_fCtrlDown?SB_BOTTOM:SB_PAGEDOWN, 0, bHandled);
        break;

    case VK_HOME:
        OnScroll(WM_HSCROLL, m_fCtrlDown?SB_TOP:SB_PAGEUP, 0, bHandled);
        break;

    case VK_CONTROL:
    case VK_SHIFT:
        // if m_fPanning is true then we are already in the middle of an operation so we
        // should maintain the cursor for that operation
        if ( !m_fPanning )
        {
            if ( VK_CONTROL == wParam )
            {
                m_fCtrlDown = true;
            }
            else if (VK_SHIFT == wParam)
            {
                m_fShiftDown = true;
            }

            // Update the cursor based on the key states set above only if we are over our window
            OnSetCursor( 0,0,0, bHandled );
        }
        break;

    default:
        // if in run screen preview mode any key other than Shift and Control will dismiss the window
        if ( NULL == GetParent() )
        {
            DestroyWindow();
        }
        return 1;   // return non-zero to indicate unprocessed message
    }

    return 0;
}


// OnMouseUp
//
// Handles WM_LBUTTONUP messages sent to the window

LRESULT CZoomWnd::OnMouseUp(UINT , WPARAM , LPARAM , BOOL& bHandled)
{
    if ( m_fPanning )
        ReleaseCapture();
    m_fPanning = false;
    bHandled = FALSE;
    return 0;
}

// OnMouseDown
//
// Handles WM_LBUTTONDOWN and WM_MBUTTONDOWN messages sent to the window
LRESULT CZoomWnd::OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_xPosMouse = (short)LOWORD( lParam );
    m_yPosMouse = (short)HIWORD( lParam );

    ASSERT( m_fPanning == false );

    // Holding the CTRL key makes a pan into a zoom and vise-versa.
    // The middle mouse button always pans regardless of default mode and key state.
    if ( (wParam & MK_CONTROL) || (uMsg == WM_MBUTTONDOWN) )
    {
        // REVIEW: check for pan being valid here?  Should be more efficient than all the checks
        // I have to do in OnMouseMove.
        m_fPanning = true;

        OnSetCursor(0,0,0,bHandled);
        SetCapture();
    }
    else
    {
        // Holding down the shift key turns a zoomin into a zoomout and vise-versa.
        // The "default" zoom mode is zoom in (if mode = pan and ctrl key is down we zoom in).
        bool bZoomIn = (m_modeDefault != MODE_ZOOMOUT) ^ ((wParam & MK_SHIFT)?1:0);

        // Find the point we want to stay centered on:
        m_cxCenter = MulDiv( m_xPosMouse-m_ptszDest.x, m_cxImage, m_ptszDest.cx);
        m_cyCenter = MulDiv( m_yPosMouse-m_ptszDest.y, m_cyImage, m_ptszDest.cy);

        bZoomIn?ZoomIn():ZoomOut();
    }
    bHandled = FALSE;
    return 0;
}

void CZoomWnd::Zoom( WPARAM wParam, LPARAM lParam )
{
    switch (wParam&0xFF)
    {
    case IVZ_CENTER:
        break;
    case IVZ_POINT:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            if ( x<0 ) x=0;
            else if ( x>=m_cxImage ) x = m_cxImage-1;
            if ( y<0 ) y=0;
            else if ( y>=m_cyImage ) y = m_cyImage-1;

            m_cxCenter = x;
            m_cyCenter = y;
        }
        break;
    case IVZ_RECT:
        {
            LPRECT prc = (LPRECT)lParam;
            int x = (prc->left+prc->right)/2;
            int y = (prc->top+prc->bottom)/2;

            if ( x<0 ) x=0;
            else if ( x>=m_cxImage ) x = m_cxImage-1;
            if ( y<0 ) y=0;
            else if ( y>=m_cyImage ) y = m_cyImage-1;

            m_cxCenter = x;
            m_cyCenter = y;
            // TODO: This should really completely adjust the dest rect but I have to
            // check for any assumptions about aspect ratio before I allow this absolute
            // aspect ignoring zoom mode.
        }
        break;
    }
    if ( wParam&IVZ_ZOOMOUT )
        ZoomOut();
    else
        ZoomIn();
}

void CZoomWnd::ZoomIn()
{
    if ( m_pImgCtx || m_hbitmap )
    {
        m_bBestFit = false;

        // first, the height is adjusted by the amount the mouse cursor moved.
        m_ptszDest.cy = (LONG)/*ceil*/(m_ptszDest.cy*1.200);  // ceil is required in order to zoom in
                                                                // on 4px high or less image

        // we don't allow zooming beyond 16x the full size of the image
        if ( m_ptszDest.cy >= m_cyImage*16 )
        {
            m_ptszDest.cy = m_cyImage*16;
        }

        // next, a new width is calculated based on the original image dimentions and the new height
        m_ptszDest.cx = MulDiv( m_ptszDest.cy, m_cxImage, m_cyImage );
        AdjustRectPlacement();
    }
}

void CZoomWnd::ZoomOut()
{
    if ( m_pImgCtx || m_hbitmap )
    {
        // if the destination rect already fits within the window, don't allow a zoom out.
        // This check is to prevent a redraw that would occure otherwise 
        if ((m_ptszDest.cx <= MIN(m_cxWindow,m_cxImage)) &&
            (m_ptszDest.cy <= MIN(m_cyWindow,m_cyImage)))
        {
            m_bBestFit = true;
            return;
        }

        // first, the height is adjusted by the amount the mouse cursor moved.
        m_ptszDest.cy = (LONG)/*floor*/(m_ptszDest.cy*0.833); // floor is default behavior
        // next, a new width is calculated based on the original image dimentions and the new height
        m_ptszDest.cx = MulDiv( m_ptszDest.cy, m_cxImage, m_cyImage );
        AdjustRectPlacement();
    }
}

// OnMouseMove
//
// Handles WM_MOUSEMOVE messages sent to the control

LRESULT CZoomWnd::OnMouseMove(UINT , WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // This is something of a hack since I never recieve the keyboard focus
    if ( wParam & MK_CONTROL )
        m_fCtrlDown = true;
    else
        m_fCtrlDown = false;    
    if ( wParam & MK_SHIFT )
        m_fShiftDown = true;
    else
        m_fShiftDown = false;

    // we only care about mouse move when the middle or left button is down
    // and we have a valid bitmap handle and we are panning
    if ( !(wParam & (MK_LBUTTON|MK_MBUTTON)) || !m_fPanning || (!m_hbitmap && !m_pImgCtx) )
    {
        bHandled = false;
        return TRUE;
    }

    // we know we are panning when we reach this point
    ASSERT( m_fPanning );

    POINTS pt = MAKEPOINTS( lParam );
    PTSZ ptszDest;

    ptszDest.cx = m_ptszDest.cx;
    ptszDest.cy = m_ptszDest.cy;

    // only allow side-to-side panning if it's needed
    if ( m_ptszDest.cx > m_cxWindow )
    {
        ptszDest.x = m_ptszDest.x + pt.x - m_xPosMouse;
    }
    else
    {
        ptszDest.x = m_ptszDest.x;
    }

    // only allow up-and-down panning if it's needed
    if ( m_ptszDest.cy > m_cyWindow )
    {
        ptszDest.y = m_ptszDest.y + pt.y - m_yPosMouse;
    }
    else
    {
        ptszDest.y = m_ptszDest.y;
    }

    // if the image is now smaller than the window, center it
    // if the image is now panned when it shouldn't be, adjust the possition
    if ( ptszDest.cx < m_cxWindow )
        ptszDest.x = (m_cxWindow-ptszDest.cx)/2;
    else
    {
        if ( ptszDest.x < (m_cxWindow - ptszDest.cx) )
            ptszDest.x = m_cxWindow - ptszDest.cx;
        if ( ptszDest.x > 0 )
            ptszDest.x = 0;
    }
    if ( ptszDest.cy < m_cyWindow )
        ptszDest.y = (m_cyWindow-ptszDest.cy)/2;
    else
    {
        if ( ptszDest.y < (m_cyWindow - ptszDest.cy) )
            ptszDest.y = m_cyWindow - ptszDest.cy;
        if ( ptszDest.y > 0 )
            ptszDest.y = 0;
    }

    m_xPosMouse = pt.x;
    m_yPosMouse = pt.y;

    // ensure the scroll bars are correct
    SetScrollBars();

    // if anything has changed, we must invalidate the window to force a repaint
    if ( (ptszDest.x != m_ptszDest.x) || (ptszDest.y != m_ptszDest.y) ||
         (ptszDest.cx != m_ptszDest.cx) || (ptszDest.y != m_ptszDest.y ))
    {
        // REVIEW: Is it worth it to calculate rcInvalid?  Should I use NULL instead?
        RECT rcInvalid;
        rcInvalid.left = MIN( ptszDest.x, m_ptszDest.x );
        rcInvalid.top  = MIN( ptszDest.y, m_ptszDest.y );
        rcInvalid.right = MAX( ptszDest.x + ptszDest.cx, m_ptszDest.x + m_ptszDest.cx );
        rcInvalid.bottom = MAX( ptszDest.y + ptszDest.cy, m_ptszDest.y + m_ptszDest.cy );
        m_ptszDest = ptszDest;
        InvalidateRect( &rcInvalid );
    }

    // Update m_cxCenter and m_cyCenter so that a zoom after a pan will zoom in
    // on the correct area.  This is majorly annoying otherwise.  We want the
    // new center to be whatever is in the center of the window after we pan.
    m_cxCenter = MulDiv( m_cxWindow/2-m_ptszDest.x, m_cxImage, m_ptszDest.cx);
    m_cyCenter = MulDiv( m_cyWindow/2-m_ptszDest.y, m_cyImage, m_ptszDest.cy);

    return TRUE;
}

// OnSize
//
// Handles WM_SIZE messages set to the window

LRESULT CZoomWnd::OnSize(UINT , WPARAM , LPARAM lParam, BOOL& )
{
    m_cxWindow = LOWORD(lParam);
    m_cyWindow = HIWORD(lParam);

    if ( m_bBestFit )
    {
        BestFit();
    }
    else
    {
        // The size of the rect doesn't change in this case, so just reposition
        AdjustRectPlacement();
    }

    return TRUE;
}

// SetMode
//
// Sets the current mouse mode to one of the values specified in the MODE enumeration.
// Currently there are two modes, pan and zoom.  The mode effects the default mouse
// cursor when moving over the zoom window and the behavior of a click-and-drag with the
// left mouse button.  Holding the shift key effects the result of a click-and-drag but
// does not effect m_mode, which is the default when the shift key isn't down.

bool CZoomWnd::SetMode(MODE modeNew)
{
    if ( m_modeDefault == modeNew )
        return false;

    m_modeDefault = modeNew;
    return true;
}

// ActualSize
//
// Displays image zoomed to its full size
void CZoomWnd::ActualSize()
{
    m_bBestFit = FALSE;

    // actual size means same sixe as the image
    m_ptszDest.cx = m_cxImage;
    m_ptszDest.cy = m_cyImage;

    // we center the image
    m_ptszDest.x = (m_cxWindow-m_cxImage)/2;
    m_ptszDest.y = (m_cyWindow-m_cyImage)/2;

    // Setting actual size is a zoom operation.  Whenever we zoom we update our centerpoint.
    m_cxCenter = m_cxImage/2;
    m_cyCenter = m_cyImage/2;

    // turn scoll bars on/off as needed
    SetScrollBars();

    InvalidateRect( NULL );
}

// BestFit
//
// Computes the default location for the destination rectangle.  This rectangle is a
// best fit while maintaining aspect ratio within a window of the given width and height.
// If the window is larger than the image, the image is centered, otherwise it is scaled
// to fit within the window.  The destination rectange is computed in the client coordinates
// of the window whose width and height are passed as arguments (ie we assume the point 0,0
// is the upper left corner of the window).
//
void CZoomWnd::BestFit()
{
    m_bBestFit = true;

    // if scroll bars are on, adjust the client size to what it will be once they are off
    DWORD dwStyle = GetWindowLong(GWL_STYLE);
    if ( dwStyle & (WS_VSCROLL|WS_HSCROLL) )
    {
        m_cxWindow += (dwStyle&WS_VSCROLL)?m_cxVScroll:0;
        m_cyWindow += (dwStyle&WS_HSCROLL)?m_cyHScroll:0;
    }

    // Determine the limiting axis, if any.
    if ( m_cxImage <= m_cxWindow && m_cyImage <= m_cyWindow )
    {
        // item fits centered within window
        m_ptszDest.x = (m_cxWindow-m_cxImage)/2;
        m_ptszDest.y = (m_cyWindow-m_cyImage)/2;
        m_ptszDest.cx = m_cxImage;
        m_ptszDest.cy = m_cyImage;
    }
    else if ( m_cxImage * m_cyWindow < m_cxWindow * m_cyImage )
    {
        // height is the limiting factor
        int iNewWidth = MulDiv(m_cyWindow, m_cxImage, m_cyImage);
        m_ptszDest.x = (m_cxWindow-iNewWidth)/2;
        m_ptszDest.y = 0;
        m_ptszDest.cx = iNewWidth;
        m_ptszDest.cy = m_cyWindow;
    }
    else
    {
        // width is the limiting factor
        int iNewHeight = MulDiv(m_cxWindow, m_cyImage, m_cxImage);
        m_ptszDest.x = 0;
        m_ptszDest.y = (m_cyWindow-iNewHeight)/2;
        m_ptszDest.cx = m_cxWindow;
        m_ptszDest.cy = iNewHeight;
    }

    // this should turn off the scroll bars if they are on
    if ( dwStyle & (WS_VSCROLL|WS_HSCROLL) )
    {
        SetScrollBars();
    }
    // ensure the scroll bars are now off
    ASSERT( 0 == (GetWindowLong(GWL_STYLE)&(WS_VSCROLL|WS_HSCROLL)) );

    InvalidateRect( NULL );
}

// AdjustRectPlacement
//
// This function determines the optimal placement of the destination rectangle.  This may
// include resizing the destination rectangle if it is smaller than the "best fit" rectangle
// but it is primarily intended for repossitioning the rectange due to a change in the window
// size or destination rectangle size.  The window is repositioned so that the centered point
// remains in the center of the window.
//
void CZoomWnd::AdjustRectPlacement()
{
    // if we have scroll bars ...
    DWORD dwStyle = GetWindowLong(GWL_STYLE);
    if ( dwStyle&(WS_VSCROLL|WS_HSCROLL) )
    {
        // .. and if removing scroll bars would allow the image to fit ...
        if ( (m_ptszDest.cx < (m_cxWindow + ((dwStyle&WS_VSCROLL)?m_cxVScroll:0))) &&
             (m_ptszDest.cy < (m_cyWindow + ((dwStyle&WS_HSCROLL)?m_cyHScroll:0))) )
        {
            // ... remove the scroll bars
            m_cxWindow += (dwStyle&WS_VSCROLL)?m_cxVScroll:0;
            m_cyWindow += (dwStyle&WS_HSCROLL)?m_cyHScroll:0;
            SetScrollBars();
        }
    }

    // If the dest rect is smaller than the window ...
    if ( (m_ptszDest.cx < m_cxWindow) && (m_ptszDest.cy < m_cyWindow) )
    {
        // ... then it must be larger than the image.  Oterwise we switch
        // to "best fit" mode.
        if ( (m_ptszDest.cx < m_cxImage) && (m_ptszDest.cy < m_cyImage) )
        {
            BestFit();
            return;
        }
    }

    // given the window size, client area size, and dest rect size calculate the 
    // dest rect position.  This position is then restrained by the limits below.
    m_ptszDest.x = (m_cxWindow/2) - MulDiv( m_cxCenter, m_ptszDest.cx, m_cxImage);
    m_ptszDest.y = (m_cyWindow/2) - MulDiv( m_cyCenter, m_ptszDest.cy, m_cyImage);

    // if the image is now narrower than the window ...
    if ( m_ptszDest.cx < m_cxWindow )
    {
        // ... center the image
        m_ptszDest.x = (m_cxWindow-m_ptszDest.cx)/2;
    }
    else
    {
        // if the image is now panned when it shouldn't be, adjust the possition
        if ( m_ptszDest.x < (m_cxWindow - m_ptszDest.cx) )
            m_ptszDest.x = m_cxWindow - m_ptszDest.cx;
        if ( m_ptszDest.x > 0 )
            m_ptszDest.x = 0;
    }
    // if the image is now shorter than the window ...
    if ( m_ptszDest.cy < m_cyWindow )
    {
        // ... center the image
        m_ptszDest.y = (m_cyWindow-m_ptszDest.cy)/2;
    }
    else
    {
        // if the image is now panned when it shouldn't be, adjust the possition
        if ( m_ptszDest.y < (m_cyWindow - m_ptszDest.cy) )
            m_ptszDest.y = m_cyWindow - m_ptszDest.cy;
        if ( m_ptszDest.y > 0 )
            m_ptszDest.y = 0;
    }

    SetScrollBars();
    InvalidateRect( NULL );
}

// StatusUpdate
//
// Sent when the image generation status has changed, once when the image is first
// being created and again if there is an error of any kind.
void CZoomWnd::StatusUpdate( int iStatus )
{
    if ( m_pImgCtx )
    {
        m_pImgCtx->Release();
        m_pImgCtx = 0;
    }
    m_hbitmap = 0;
    m_fTransparent = false;
    m_iStrID = iStatus;

    if ( m_hWnd )
    {
        InvalidateRect(NULL);
    }
}

// SetBitmap
//
// Called to pass in the handle to the bitmap we draw.  We DO NOT own this bitmap, it
// is a duplicate handle.  We use it to paint only, it is freed by the parent window.
//
void CZoomWnd::SetBitmap(HBITMAP hbitmap)
{
    if ( m_pImgCtx )
    {
        m_pImgCtx->Release();
        m_pImgCtx = 0;
    }
    m_fTransparent = false;

    m_hbitmap = hbitmap;
    if (m_hbitmap)
    {
        BITMAP bm;
        if (GetObject(m_hbitmap, sizeof(bm), &bm))
        {
            // this message indicates a successful result
            m_cxImage = bm.bmWidth;
            m_cyImage = bm.bmHeight;
            m_cxCenter = m_cxImage/2;
            m_cyCenter = m_cyImage/2;

            // this message could be recieved before the window is created.  
            // Make sure m_hWnd is valid before calling BestFit or InvalidateRect
            if (m_hWnd)
            {
                BestFit();
                InvalidateRect(NULL);
            }
            return;
        }
    }

    m_iStrID = IDS_LOADFAILED;
}

// SetImgCtx
//
// Called to pass in the pointer to the IImgCtx we draw.  We hold a reference to this
// object so that we can use it to paint.
//
void CZoomWnd::SetImgCtx(IImgCtx * pImg)
{
    m_hbitmap = 0;
    if ( m_pImgCtx )
    {
        m_pImgCtx->Release();
    }
    m_fTransparent = false;

    m_pImgCtx = pImg;

    if (m_pImgCtx)
    {
        SIZE size;
        DWORD fState;

        m_pImgCtx->AddRef();
        if ( SUCCEEDED(m_pImgCtx->GetStateInfo(&fState, &size, FALSE)) )
        {
            ASSERT( fState & IMGLOAD_COMPLETE );
            m_fTransparent = (0 == (fState & IMGTRANS_OPAQUE));

            m_cxImage = size.cx;
            m_cyImage = size.cy;
            m_cxCenter = m_cxImage/2;
            m_cyCenter = m_cyImage/2;

            if (m_hWnd)
            {
                BestFit();
                InvalidateRect(NULL);
            }
        }
        return;
    }

    m_iStrID = IDS_LOADFAILED;
}

void CZoomWnd::SetPalette( HPALETTE hpal )
{
    m_hpal = hpal;
}

void CZoomWnd::SetScrollBars()
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = m_ptszDest.cx;
    si.nPage = m_cxWindow+1;
    si.nPos = 0-m_ptszDest.x;
    si.nTrackPos = 0;

    SetScrollInfo( m_hWnd, SB_HORZ, &si, true );

    si.nMax = m_ptszDest.cy;
    si.nPage = m_cyWindow+1;
    si.nPos = 0-m_ptszDest.y;

    SetScrollInfo( m_hWnd, SB_VERT, &si, true );
}

LRESULT CZoomWnd::OnScroll(UINT uMsg, WPARAM wParam, LPARAM , BOOL& )
{
    int iScrollBar;
    int iWindow;     // width or height of the window
    LONG * piTL;     // pointer to top or left point
    LONG   iWH;      // the width or height of the dest rect

    if ( (!m_hbitmap && !m_pImgCtx) )
        return 0;

    // handle both which direction we're scrolling
    if ( WM_HSCROLL==uMsg )
    {
        iScrollBar = SB_HORZ;
        iWindow = m_cxWindow;
        piTL = &m_ptszDest.x;
        iWH = m_ptszDest.cx;
    }
    else
    {
        iScrollBar = SB_VERT;
        iWindow = m_cyWindow;
        piTL = &m_ptszDest.y;
        iWH = m_ptszDest.cy;
    }

    // Using the keyboard we can get scroll messages when we don't have scroll bars.
    // Ignore these messages.
    if ( iWindow >= iWH )
    {
        // window is larger than the image, don't allow scrolling
        return 0;
    }

    // handle all possible scroll cases
    switch ( LOWORD(wParam) )
    {
    case SB_TOP:
        *piTL = 0;
        break;
    case SB_PAGEUP:
        *piTL += iWindow;
        break;
    case SB_LINEUP:
        (*piTL)++;
        break;
    case SB_LINEDOWN:
        (*piTL)--;
        break;
    case SB_PAGEDOWN:
        *piTL -= iWindow;
        break;
    case SB_BOTTOM:
        *piTL = iWindow-iWH;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        *piTL = -HIWORD(wParam);
        break;
    case SB_ENDSCROLL:
        return 0;
    }

    // apply limits
    if ( 0 < *piTL )
        *piTL = 0;
    else if ( (iWindow-iWH) > *piTL )
        *piTL = iWindow-iWH;

    // adjust scrollbars 
    SetScrollPos(iScrollBar, -(*piTL), true);

    // calculate new center point relative to image
    if ( WM_HSCROLL==uMsg )
    {
        m_cxCenter = MulDiv( (m_cxWindow/2)-m_ptszDest.x, m_cxImage, m_ptszDest.cx);
    }
    else
    {
        m_cyCenter = MulDiv( (m_cyWindow/2)-m_ptszDest.y, m_cyImage, m_ptszDest.cy);
    }

    InvalidateRect( NULL );
    return 0;
}
/*
LRESULT CZoomWnd::OnHScroll(UINT , WPARAM wParam, LPARAM , BOOL& )
{
    if ( !m_hbitmap )
        return 0;

    // handle all possible scroll cases
    switch ( LOWORD(wParam) )
    {
    case SB_TOP:
        m_ptszDest.x = 0;
        break;
    case SB_PAGEUP:
        m_ptszDest.x += m_cxWindow;
        break;
    case SB_LINEUP:
        m_ptszDest.x++;
        break;
    case SB_LINEDOWN:
        m_ptszDest.x--;
        break;
    case SB_PAGEDOWN:
        m_ptszDest.x -= m_cxWindow;
        break;
    case SB_BOTTOM:
        m_ptszDest.x = m_cxWindow-m_ptszDest.cx;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        m_ptszDest.x = -HIWORD(wParam);
        break;
    case SB_ENDSCROLL:
        return 0;
    }

    // apply limits
    if ( 0 < m_ptszDest.x )
        m_ptszDest.x = 0;
    else if ( m_cxWindow-m_ptszDest.cx > m_ptszDest.x )
        m_ptszDest.x = m_cxWindow-m_ptszDest.cx;

    // adjust scrollbars 
    SetScrollPos(SB_HORZ, -m_ptszDest.x, true);

    // calculate new center point relative to image
    m_cxCenter = MulDiv( (m_cxWindow/2)-m_ptszDest.x, m_cxImage, m_ptszDest.cx);

    // redraw
    InvalidateRect( NULL );
    return 0;
}

LRESULT CZoomWnd::OnVScroll(UINT , WPARAM wParam, LPARAM , BOOL& )
{
    if ( !m_hbitmap )
        return 0;

    // handle all possible scroll cases
    switch ( LOWORD(wParam) )
    {
    case SB_TOP:
        m_ptszDest.y = 0;
        break;
    case SB_PAGEUP:
        m_ptszDest.y += m_cyWindow;
        break;
    case SB_LINEUP:
        m_ptszDest.y++;
        break;
    case SB_LINEDOWN:
        m_ptszDest.y--;
        break;
    case SB_PAGEDOWN:
        m_ptszDest.y -= m_cyWindow;
        break;
    case SB_BOTTOM:
        m_ptszDest.y = m_cyWindow-m_ptszDest.cy;
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        m_ptszDest.y = -HIWORD(wParam);
        break;
    case SB_ENDSCROLL:
        return 0;
    }

    // apply limits
    if ( 0 < m_ptszDest.y )
        m_ptszDest.y = 0;
    else if ( m_cyWindow-m_ptszDest.cy > m_ptszDest.y )
        m_ptszDest.y = m_cyWindow-m_ptszDest.cy;

    // adjust scrollbars 
    SetScrollPos(SB_VERT, -m_ptszDest.y, true);

    // calculate new center point relative to image
    m_cyCenter = MulDiv( (m_cyWindow/2)-m_ptszDest.y, m_cyImage, m_ptszDest.cy);

    // redraw
    InvalidateRect( NULL );
    return 0;
}
*/
// OnWheelTurn
//
// Respondes to WM_MOUSEWHEEL messages sent to the parent window (then redirected here)

LRESULT CZoomWnd::OnWheelTurn(UINT , WPARAM wParam, LPARAM , BOOL& )
{
    bool bZoomIn = ((short)HIWORD(wParam) > 0);

    bZoomIn?ZoomIn():ZoomOut();

    return TRUE;
}

LRESULT CZoomWnd::OnSetFocus(UINT , WPARAM , LPARAM , BOOL& )
{
    HWND hwndParent = GetParent();
    ::SetFocus( hwndParent );
    return 0;
}
