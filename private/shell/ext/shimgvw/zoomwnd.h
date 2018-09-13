
#ifndef __ZOOMWND_H_
#define __ZOOMWND_H_

#include <IImgCtx.h>

typedef struct tagPOINTSIZE
{
    LONG    x;
    LONG    y;
    LONG    cx;
    LONG    cy;
} PTSZ, *PPTSZ, NEAR *NPPTSZ, FAR *LPPTSZ;

/////////////////////////////////////////////////////////////////////////////
// CZoomWnd
class CZoomWnd : 
    public CWindowImpl<CZoomWnd>
{
public:
    enum MODE { MODE_PAN, MODE_ZOOMIN, MODE_ZOOMOUT };

    // public accessor functions
    void ZoomIn();      // Does a zoom in, handles contraints
    void ZoomOut();     // does a zoom out, handles boundry conditions and constraints
    void ActualSize();                  // show image at full size (crop if needed)
    void BestFit();                     // show full image in window (scale down if needed)

    void SetBitmap( HBITMAP hbitmap );  // used to set a bitmap for display
    void SetImgCtx( IImgCtx * pImg );   // used to set a IImgCtx object for display
    void SetPalette( HPALETTE hpal );   // If in palette mode, set this to the palette to use
    void StatusUpdate( int iStatus );   // used to set m_iStrID to display correct status message
    void Zoom( WPARAM wParam, LPARAM lParam );
    bool SetMode( MODE modeNew );       // used to input toolbar commands
    int  QueryStatus() { return m_iStrID; }

    CZoomWnd();
    ~CZoomWnd();

    DECLARE_WND_CLASS( TEXT("ShImgVw:CZoomWnd") );

protected:
BEGIN_MSG_MAP(CZoomWnd)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseDown)
    MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMouseDown)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseUp)
    MESSAGE_HANDLER(WM_MBUTTONUP, OnMouseUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
    MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnWheelTurn)
END_MSG_MAP()

    // message handlers
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//    LRESULT OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//    LRESULT OnVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnWheelTurn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:
    // This is lazy.  These are used by CPreview so I made them public when they probably shouldn't be.
    int m_cxImage;      // width of bitmap referenced by m_hbitmap
    int m_cyImage;      // height of bitmap referenced by m_hbitmap

protected:
    bool m_bBestFit;    // True if we are in Bets Fit mode
    IImgCtx * m_pImgCtx;// Handle to an IImgCtx interface if we're using one
    HBITMAP m_hbitmap;  // handle to the bitmap we've created for the display image.
                        // we DO NOT own this bitmap, it's a duplicate handle.

    int m_cxCenter;     // point to center on relative to image
    int m_cyCenter;     // point to center on relative to image

    int m_cxVScroll;    // width of a scroll bar
    int m_cyHScroll;    // height of a scroll bar
    int m_cxWindow;     // width of our client area + scroll width if scroll bar is visible
    int m_cyWindow;     // height of our client area + scroll height if scroll bar is visible

    int m_xPosMouse;    // used to track mouse movement when dragging the LMB
    int m_yPosMouse;    // used to track mouse movement when dragging the LMB

    MODE m_modeDefault; // The zoom or pan mode when the shift key isn't pressed
    PTSZ m_ptszDest;    // the point and size of the destination rectangle
    bool m_fPanning;    // true when we are panning (implies left mouse button is down)
    bool m_fCtrlDown;   // a mode modifier ( zoom <=> pan ), true if Ctrl Key is down
    bool m_fShiftDown;  // a mode modifier (zoom in <=> zoom out), true if Shift is down

    int m_iStrID;       // string to display when no bitmap available

    HPALETTE m_hpal;

    bool m_fTransparent;// true if the decoded image has transparency data.  Only IImgCtx images can be transparent.

    // protected methods
    void AdjustRectPlacement(); // applies constraints for centering, ensureinging maximum visibility, etc
    void SetScrollBars();       // ensures scroll bar state is correct.  Used after window resize or zoom.
};

#include "prevwnd.h"

#endif
