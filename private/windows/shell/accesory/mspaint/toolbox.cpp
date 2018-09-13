/******************************************************************************/
/*                                                                            */
/* Class Implementations in this file                                         */
/*      CFloatImgToolWnd                                                      */
/*      CImgToolWnd                                                           */
/*      CToolboxWnd                                                           */
/*      CTool                                                                 */
/*                                                                            */
/******************************************************************************/

#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "ipframe.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgwell.h"
#include "imgtools.h"
#include "toolbox.h"
#include "imgcolor.h"
#include "docking.h"
#include "t_Text.h"

#define TRYANYTHING

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CToolboxWnd, CControlBar)

#include "memtrace.h"

CImgToolWnd* NEAR g_pImgToolWnd = NULL;

#define BPR(br, rop)        \
 { dc.SelectObject((br));   \
   dc.PatBlt(rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, rop); }

#define iidmMac ( sizeof (rgidm) / sizeof (rgidm[0]) )

static UINT NEAR rgidm [] =
    {
    IDMB_PICKRGNTOOL,
    IDMB_PICKTOOL,

    IDMB_ERASERTOOL,
    IDMB_FILLTOOL,

    IDMY_PICKCOLOR,
    IDMB_ZOOMTOOL,

    IDMB_PENCILTOOL,
    IDMB_CBRUSHTOOL,

    IDMB_AIRBSHTOOL,
    IDMX_TEXTTOOL,

    IDMB_LINETOOL,
    IDMB_CURVETOOL,

    IDMB_RECTTOOL,
    IDMB_POLYGONTOOL,

    IDMB_ELLIPSETOOL,
    IDMB_RNDRECTTOOL
    };

/******************************************************************************/

BEGIN_MESSAGE_MAP(CImgToolWnd, CToolboxWnd)
        ON_WM_SYSCOLORCHANGE()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONDOWN()
    ON_WM_PAINT()
    ON_WM_KEYDOWN()
    ON_WM_KEYUP()
    ON_WM_CHAR()
    ON_WM_NCHITTEST()
END_MESSAGE_MAP()


/******************************************************************************/

//
// The ToolChildProc passes all mouse clicks along to the parent
//
LRESULT CALLBACK ToolChildProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{

   switch (msg)
   {
      // Pretend to be a "button", so that CWnd::OnToolHitTest will not 
      // add TTF_CENTERTIP flag to the tooltips. 
      case WM_GETDLGCODE:
          return DefWindowProc(hwnd, msg, wp, lp) | DLGC_BUTTON;

      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
         {
            POINT pt;
            pt.x = LOWORD(lp);
            pt.y = HIWORD(lp);
            ::ClientToScreen (hwnd, &pt);
            ::ScreenToClient (GetParent (hwnd), &pt);
            lp = MAKELPARAM ((WORD)pt.x, WORD(pt.y));
            PostMessage (GetParent (hwnd), msg, wp, lp);
         }

         return 0;
   }

   return DefWindowProc (hwnd, msg, wp, lp);
}

BOOL CImgToolWnd::Create(const TCHAR* pWindowName, DWORD dwStyle,
                         const RECT& rect, const POINT& btnSize, WORD wWide,
                         CWnd* pParentWnd, BOOL bDkRegister)
    {
    if (! CToolboxWnd::Create( pWindowName, dwStyle, rect,
                               btnSize, NUM_TOOLS_WIDE, pParentWnd, IDB_IMGTOOLS ))
        {
        return FALSE;
        }

    for (int iidm = 0; iidm < iidmMac; iidm += 1)
        {
        CTool* pTool = new CTool(this, (WORD)rgidm[iidm], iidm, TS_CMD | TS_STICKY,
                rgidm[iidm] == CImgTool::GetCurrentID() ? TF_DOWN : 0);

        if (pTool == NULL)
            {
            DestroyWindow();
            return FALSE;
            }
        AddTool(pTool);

        }


    m_nOffsetX = m_btnsize.x / 4;

    CSize size = GetSize();

    MoveWindow( rect.left, rect.top, size.cx, size.cy );
    //
    // HACK alert!
    // MFC 4 now requires a CControlBar to have actual child windows
    // in order to support tool tips. So create a transparent window over each
    // tool button with the appropriate ID.
    //
    WNDCLASS wc;
    LONG left, top;
    ZeroMemory (&wc, sizeof (wc));
    wc.lpszClassName = TEXT("ToolChild");
    wc.hCursor = ::LoadCursor (NULL, IDC_ARROW);
    wc.lpfnWndProc = ::ToolChildProc;
    wc.hInstance = AfxGetInstanceHandle();
    ::RegisterClass (&wc);
    HWND hwnd;
    for (iidm=0;iidm < iidmMac;iidm++)
    {
       left   = (iidm % m_wWide) * m_btnsize.x + m_nOffsetX;
       top    = (iidm / m_wWide) * m_btnsize.y;
       hwnd = ::CreateWindowEx (WS_EX_TRANSPARENT, wc.lpszClassName,
                       TEXT(""), WS_VISIBLE | WS_CHILD, left, top,
                       m_btnsize.x, m_btnsize.y, GetSafeHwnd(),
                       (HMENU)IntToPtr(rgidm[iidm]),
                       AfxGetInstanceHandle(), NULL);
       ::ShowWindow (hwnd, SW_SHOW);
    }


    return TRUE;
    }

/******************************************************************************/
//

void CImgToolWnd::OnSysColorChange()
        {
        CToolboxWnd::OnSysColorChange();
        InvalidateRect(NULL, FALSE);
        }

/******************************************************************************/

void CImgToolWnd::OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler )
    {
    }

/******************************************************************************/

CSize CImgToolWnd::CalcFixedLayout( BOOL bStretch, BOOL bHorz )
    {
#ifdef TRYANYTHING
        return GetSize();
#else
    CSize size = CControlBar::CalcFixedLayout( bStretch, bHorz );

    CSize sizeBar = GetSize();

    size.cx = sizeBar.cx;
    return size;
#endif
    }

/******************************************************************************/

UINT CImgToolWnd::OnNcHitTest(CPoint point)
    {
    return CToolboxWnd::OnNcHitTest(point);
    }

/******************************************************************************/

CSize CImgToolWnd::GetSize()
    {
    // Leave room in the toolbox for the brushes...
    CRect clientRect;
    CRect windowRect;
    CSize sizeDiff;

    GetWindowRect( &windowRect );
    GetClientRect( &clientRect );

    sizeDiff = windowRect.Size() - clientRect.Size();
    int nTools = GetToolCount();

    clientRect.right  = (m_btnsize.x - 1) * NUM_TOOLS_WIDE + 1 + m_nOffsetX * 2;
    clientRect.bottom = (nTools / NUM_TOOLS_WIDE + (!!(nTools % NUM_TOOLS_WIDE)))
                                                         * (m_btnsize.y - 1) + 1;
    clientRect.bottom += 10;

    m_rcBrushes.left   = clientRect.left   + (4 + m_nOffsetX);
    m_rcBrushes.top    = clientRect.bottom;
    m_rcBrushes.right  = clientRect.right  - (4 + m_nOffsetX);
    m_rcBrushes.bottom = m_rcBrushes.top  + 66;

    clientRect.bottom += m_rcBrushes.Height() + 4;

    m_rcTools = clientRect;
    m_rcTools.left   = m_nOffsetX;
    m_rcTools.right -= m_nOffsetX;

    return clientRect.Size() + sizeDiff;
    }

/******************************************************************************/

void CImgToolWnd::OnLButtonDown(UINT nFlags, CPoint pt)
    {
    BOOL bInBrushes = m_rcBrushes.PtInRect(pt);

    if (bInBrushes)
        {
        CRect optionsRect = m_rcBrushes;

        optionsRect.InflateRect(-1, -1);
        pt -= (CSize)optionsRect.TopLeft();

        CImgTool::GetCurrent()->OnClickOptions(this, optionsRect, pt);
        }
    else
        CToolboxWnd::OnLButtonDown(nFlags, pt);
    }

/******************************************************************************/

void CImgToolWnd::InvalidateOptions(BOOL bErase)
    {
    // NOTE: bErase is now ignored since we do drawing off-screen and
    // blt the whole thing...

    CRect optionsRect = m_rcBrushes;
    optionsRect.InflateRect(-1, -1);

    InvalidateRect(&optionsRect, FALSE);
    }

/******************************************************************************/

void CImgToolWnd::OnLButtonDblClk(UINT nFlags, CPoint pt)
    {
    CToolboxWnd::OnLButtonDblClk(nFlags, pt);
    }

/******************************************************************************/

void CImgToolWnd::OnRButtonDown(UINT nFlags, CPoint pt)
    {
    CToolboxWnd::OnRButtonDown(nFlags, pt);
    }

/******************************************************************************/

BOOL CImgToolWnd::OnEraseBkgnd( CDC* pDC )
    {
    CRect rect;
    GetClientRect( rect );
    pDC->FillRect( rect, GetSysBrush( COLOR_BTNFACE ) );

        return CControlBar::OnEraseBkgnd( pDC );
    }

/******************************************************************************/

void CImgToolWnd::OnPaint()
    {
    CPaintDC dc(this);

    if (dc.m_hDC == NULL)
        {
        theApp.SetGdiEmergency();
        return;
        }

    if (CImgWnd::c_pImgWndCur == NULL)
        {
        // Chances are, we're are going to be hidden soon, so don't
        // bother painting...
        return;
        }

    DrawButtons(dc, &dc.m_ps.rcPaint);

    ASSERT(CImgWnd::c_pImgWndCur->m_pImg != NULL);

    // Brush Shapes
    if (!(m_rcBrushes & dc.m_ps.rcPaint).IsRectEmpty())
        {
        Draw3dRect(dc.m_hDC, &m_rcBrushes );
        CRect optionsRect = m_rcBrushes;
        optionsRect.InflateRect(-1, -1);

        CRect rc(0, 0, optionsRect.Width(), optionsRect.Height());
        CDC memDC;
        CBitmap memBM;

        if (!memDC.CreateCompatibleDC(&dc) ||
            !memBM.CreateCompatibleBitmap(&dc, rc.right, rc.bottom))
            {
            theApp.SetGdiEmergency();
            return;
            }
        CBitmap* pOldBitmap = memDC.SelectObject(&memBM);

        CBrush* pOldBrush = memDC.SelectObject(GetSysBrush( COLOR_BTNFACE ));

        memDC.PatBlt(0, 0, rc.right, rc.bottom, PATCOPY);

        CRect rcPaint = dc.m_ps.rcPaint;
        rcPaint.OffsetRect(-optionsRect.left, -optionsRect.top);

        CImgTool::GetCurrent()->OnPaintOptions(&memDC, rcPaint, rc);

        dc.BitBlt(optionsRect.left, optionsRect.top, optionsRect.Width(),
                  optionsRect.Height(), &memDC, 0, 0, SRCCOPY);

        memDC.SelectObject(pOldBitmap);
        memDC.SelectObject(pOldBrush);
        }
    }

/******************************************************************************/

BOOL CImgToolWnd::PreTranslateMessage(MSG* pMsg)
    {
    switch (pMsg->message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            if (CImgWnd::c_pImgWndCur != NULL)
                {
                pMsg->hwnd = CImgWnd::c_pImgWndCur->m_hWnd;
                return CImgWnd::c_pImgWndCur->PreTranslateMessage(pMsg);
                }
            return FALSE;
        }

        return CToolboxWnd::PreTranslateMessage(pMsg);
    }

/******************************************************************************/
// default button size

const POINT NEAR CToolboxWnd::ptDefButton = { 26, 26 };

/*DK*/
BEGIN_MESSAGE_MAP(CToolboxWnd, CControlBar)
        ON_WM_SYSCOLORCHANGE()
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_CLOSE()
    ON_WM_WININICHANGE()
    ON_WM_KEYDOWN()
    ON_MESSAGE(TM_TOOLDOWN, OnToolDown)
    ON_MESSAGE(TM_TOOLUP, OnToolUp)
    /*DK*/
//  ON_MESSAGE(WM_HELPHITTEST, OnHelpHitTest)
END_MESSAGE_MAP()

/******************************************************************************/

CToolboxWnd::CToolboxWnd()
    {
    m_Tools    = new CObArray;
    m_bmStuck  = NULL;
    m_bmPushed = NULL;
    m_bmPopped = NULL;
    m_tCapture = NULL;
    m_bInside  = FALSE;
    m_nOffsetX = 0;
    }

/******************************************************************************/

CToolboxWnd::~CToolboxWnd()
    {
    if (m_bmStuck != NULL)
        delete m_bmStuck;

    if (m_bmPushed != NULL)
        delete m_bmPushed;

    if (m_bmPopped != NULL)
        delete m_bmPopped;

    if (m_Tools != NULL)
        {
        int nTools = (int)m_Tools->GetSize();

        for (int iTool = 0; iTool < nTools; iTool += 1)
            {
            CTool* pTool = (CTool*)m_Tools->GetAt(iTool);
            delete pTool;
            }

        delete m_Tools;
        }
    }

/******************************************************************************/

BOOL CToolboxWnd::Create(const TCHAR FAR* lpWindowName,
                         DWORD dwStyle, const RECT& rect,
                         const POINT& btnsize /* = ptDefButton */, WORD wWide /* = 1 */,
                         CWnd* pParentWnd /* = NULL */, int nImageWellID /* = 0 */)
    {
    // This routine is a lot more complicated than the typical Create, so
    // because (1) we aren't a built-in Windows window type, and (2) we
    // want to specify the client size with the btnsize and wWide parameters.
    // (We ignore the width, height of the rect parameter.)

    if (nImageWellID != 0 && !m_imageWell.Load(nImageWellID, CSize(16, 16)))
        return FALSE;

        // save the style
        m_dwStyle = (UINT)dwStyle | CBRS_TOOLTIPS | CBRS_FLYBY;

    DWORD dwS = (m_dwStyle & ~WS_VISIBLE);

    CRect t = rect;

    t.right  = t.left + 20;
    t.bottom = t.top  + 20;

    BOOL bRet = CControlBar::Create( NULL, lpWindowName, dwS, t, pParentWnd,
                                                         ID_VIEW_TOOL_BOX );
    if (! bRet)
        return FALSE;

    m_wWide   = wWide;
    m_btnsize = btnsize;

#ifdef TRYANYTHING
        SizeByButtons( -1, TRUE );
#else
    SizeByButtons( 0 );
#endif

    if (! DrawStockBitmaps())
        {
        DestroyWindow();
        return FALSE;
        }

    if (dwStyle & WS_VISIBLE)
        {
        ShowWindow(SW_SHOW);
        UpdateWindow();
        }

    return TRUE;
    }

/******************************************************************************/
// private DrawStockBitmaps:
// Draws the three states of button, given the desired button size of this
// CToolboxWnd.  These have no graphic on them; the buttons have bitmap
// glyphs to be added to these blank forms.
//
// The three states:
//   m_bmPopped  This is the normal look of a button.  Note that this is
//               also used as the basis of a grayed (disabled) button, by
//               changing how the button's glyph is drawn on it.
//   m_bmPushed  This is the button-down state for non-sticky tools (tools
//               that pop back out as soon as you let go.
//   m_bmStuck   This is the button-down state for sticky tools.  This has
//               a distinct look that is more easily visible, per UISG.
//

BOOL CToolboxWnd::DrawStockBitmaps()
    {
    CWindowDC wdc(this);
    if (wdc.m_hDC == NULL)
        {
        theApp.SetGdiEmergency(TRUE);
        return FALSE;
        }

    CBitmap* obm;
    CBrush* obr;
    CRect rc;
    CDC dc;

    if (!dc.CreateCompatibleDC(&wdc))
        {
        theApp.SetGdiEmergency(TRUE);
        return FALSE;
        }

    obr = dc.SelectObject(GetSysBrush(COLOR_WINDOWFRAME));

    // bmPopped:
    //
    if (m_bmPopped)
        delete m_bmPopped;
    m_bmPopped = new CBitmap;
    if (!m_bmPopped->CreateCompatibleBitmap(&wdc, m_btnsize.x, m_btnsize.y))
        {
        theApp.SetMemoryEmergency(TRUE);
        return FALSE;
        }
    obm = dc.SelectObject(m_bmPopped);
    rc = CRect(0, 0, m_btnsize.x, m_btnsize.y);
    BPR(GetSysBrush(COLOR_WINDOWFRAME), PATCOPY);
#ifdef OLDBUTTONS
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNSHADOW), PATCOPY);
#endif
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNHIGHLIGHT), PATCOPY);
    rc.left++; rc.top++;
    BPR(GetSysBrush(COLOR_BTNSHADOW), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNFACE), PATCOPY);

    // bmPushed:
    //
    if (m_bmPushed)
        delete m_bmPushed;
    m_bmPushed = new CBitmap;
    if (!m_bmPushed->CreateCompatibleBitmap(&wdc, m_btnsize.x, m_btnsize.y))
        {
        theApp.SetMemoryEmergency(TRUE);
        return FALSE;
        }
    dc.SelectObject(m_bmPushed);
    rc = CRect(0, 0, m_btnsize.x, m_btnsize.y);
#ifndef OLDBUTTONS
    BPR(GetSysBrush(COLOR_BTNHIGHLIGHT), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_WINDOWFRAME), PATCOPY);
    rc.left++; rc.top++;
    BPR(GetSysBrush(COLOR_BTNFACE), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNSHADOW), PATCOPY);
    rc.left++; rc.top++;
    BPR(GetSysBrush(COLOR_BTNFACE), PATCOPY);
#else
    BPR(GetSysBrush(COLOR_WINDOWFRAME), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNSHADOW), PATCOPY);
    rc.left += 2; rc.top += 2;
    BPR(GetSysBrush(COLOR_BTNFACE), PATCOPY);
#endif

    // bmStuck:
    //
    if (m_bmStuck)
        delete m_bmStuck;
    m_bmStuck = new CBitmap;
    if (!m_bmStuck->CreateCompatibleBitmap(&wdc, m_btnsize.x, m_btnsize.y))
        {
        theApp.SetMemoryEmergency(TRUE);
        return FALSE;
        }
    dc.SelectObject(m_bmStuck);
    rc = CRect(0, 0, m_btnsize.x, m_btnsize.y);
#ifndef OLDBUTTONS
    BPR(GetSysBrush(COLOR_BTNHIGHLIGHT), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_WINDOWFRAME), PATCOPY);
    rc.left++; rc.top++;
    BPR(GetSysBrush(COLOR_BTNFACE), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNSHADOW), PATCOPY);
    rc.left++; rc.top++;
#else
    BPR(GetSysBrush(COLOR_WINDOWFRAME), PATCOPY);
    rc.right--; rc.bottom--;
    BPR(GetSysBrush(COLOR_BTNSHADOW), PATCOPY);
    rc.left += 2; rc.top += 2;
#endif

    dc.SelectObject(GetHalftoneBrush());
#ifdef OLDBUTTONS
    dc.SetTextColor(RGB(255, 255, 255));
    dc.SetBkColor(RGB(192, 192, 192));
#else
        dc.SetTextColor(GetSysColor(COLOR_BTNFACE));
        dc.SetBkColor(GetSysColor(COLOR_BTNHIGHLIGHT));
#endif
    dc.PatBlt(rc.left, rc.top, rc.Width(), rc.Height(), PATCOPY);

    dc.SelectObject(obm);
    dc.SelectObject(obr);
    dc.DeleteDC();

    return TRUE;
    }

/******************************************************************************/
//

afx_msg void CToolboxWnd::OnSysColorChange()
        {
        CControlBar::OnSysColorChange();
        DrawStockBitmaps();
        InvalidateRect(NULL, FALSE);
        }

/******************************************************************************/
//
// SizeByButtons
//
// Sizes the window according to the current (or a specified) number of
// buttons.  If there are no buttons, the window makes room for one button.
// Unfilled button slots show through to the background.
//

void CToolboxWnd::SizeByButtons(int nButtons /* = -1 */,
                                BOOL bRepaint /* = FALSE */)
    {
    if (nButtons == -1)
        nButtons = (int)m_Tools->GetSize();
    if (nButtons == 0)
        nButtons = 1;

    // Can't use the hokey Windows' AdjustWindowRect() to work this out,
    // so we do it ourselves by adapting the window based on the difference
    // between GetWindowRect and ClientRect results.
    //
    CRect w, c;
    GetWindowRect(&w);
    w.right -= w.left;
    w.bottom -= w.top;
    GetClientRect(&c);

    if (bRepaint)
        Invalidate(TRUE);

    MoveWindow(w.left, w.top,
                m_btnsize.x * m_wWide + (w.right - c.right) - 1,
                m_btnsize.y * ((nButtons / m_wWide) + (!!(nButtons % m_wWide)))
                        + (w.bottom-c.bottom) - 1,
                bRepaint);
    }

/******************************************************************************/
// OnWinIniChange:
//
void CToolboxWnd::OnWinIniChange(LPCTSTR lpSection)

    {
        lpSection;
#ifdef TRYANYTHING
        CControlBar::OnWinIniChange( lpSection );
#endif
    DrawStockBitmaps();
    Invalidate(TRUE);
    }

/******************************************************************************/
//
// OnKeyDown
//
// Implements keyboard handling for the toolbox window.. this allows
// trapping of the ESC key, for moving the selected to back to the
// arrow.
//
void CToolboxWnd::OnKeyDown(UINT nKey, UINT nRepCnt, UINT nFlags)
    {
    if (nKey == VK_ESCAPE && m_tCapture)
        CancelDrag();
    else
        CControlBar::OnKeyDown(nKey, nRepCnt, nFlags);
    }

/******************************************************************************/

void CToolboxWnd::CancelDrag()
    {
#if 0
    // this is bogus as dragging is presently disabled
    if (m_tCapture != NULL)
        m_tCapture->m_wState |= TF_DRAG; // so select will cancel it
#endif

    m_bInside = FALSE;

#if 0
    // whoever tries to make drag/drop work will have to handle the fact
    // that our client does not get notified by SelectTool
    SelectTool( IDMB_ARROW );
#endif

    m_tCapture = NULL;
    ReleaseCapture();
    }

/******************************************************************************/
// AddTool:
//
void CToolboxWnd::AddTool(CTool* tool)
    {
    m_Tools->Add((CObject*)tool);

    if ((m_Tools->GetSize() + m_wWide - 1) / m_wWide > 11)  // only have 11 high if more increase the width
        m_wWide += 1;

    SizeByButtons(-1, TRUE);

    }


/******************************************************************************/
// RemoveTool:
//
void CToolboxWnd::RemoveTool(CTool* tool)
    {
    for (int nTool = GetToolCount() - 1; GetToolAt(nTool) != tool; nTool -= 1)
        ASSERT(nTool >= 0);

    m_Tools->RemoveAt(nTool);

    if ((m_Tools->GetSize() + m_wWide - 2) / (m_wWide - 1) <= 11 && m_wWide > 1)
        m_wWide -= 1;

    SizeByButtons(-1, TRUE);
    }


/******************************************************************************/
// private GetTool:
//
CTool* CToolboxWnd::GetTool(WORD wID)
    {
    int nTools = (int)m_Tools->GetSize();
    for (int i = 0; i < nTools; i++)
        {
        CTool* t = (CTool*)m_Tools->GetAt(i);
        if (t && t->m_wID == wID)
            return t;
        }

    return NULL;
    }

/******************************************************************************/
//
// SetToolState
//
// Used by the owner of a button to modify the state of the button.
// This does not notify the owner of the new state; presumably the
// owner knows what it's doing to its own buttons.  This allows the
// owner to use this API during a WM_TOOLDOWN, etc., without getting
// into a shouting match with the toolbox.
//
WORD CToolboxWnd::SetToolState(WORD wID, WORD wState)
    {
    CRect rect;
    CTool* t = GetTool(wID);
    if (t && !(t->m_wState & TF_NYI))
        {
        WORD w = t->m_wState;
        t->m_wState = wState;

        //
        // if state hasn't changed, return to avoid invalidate and
        // associated flicker.
        //

        if (w == wState)
            return w;

        //
        // Calculate the rectangle of the button whose state is changing,
        // and invalidate it.
        //
        // replaces ed's simplistic (and flickering) code:
        //
        //      Invalidate(FALSE)
        //

        for (int i = 0; (CTool*)m_Tools->GetAt(i) != t; i += 1)
            {
            ASSERT(i != m_Tools->GetSize());
            }

        rect.left   = (i % m_wWide) * m_btnsize.x + m_nOffsetX;
        rect.top    = (i / m_wWide) * m_btnsize.y;
        rect.right  = rect.left + m_btnsize.x;
        rect.bottom = rect.top  + m_btnsize.y;

        InvalidateRect(&rect, FALSE);
        return w;
        }

    return 0;
    }

/******************************************************************************/
// SetToolStyle:
// Used by the owner of a button to modify the style of the button.
// This forces the state of the button to be enabled and released.
// This does not notify the owner of the new state; presumably the
// owner knows what it's doing to its own buttons.  This allows the
// owner to use this API during a WM_TOOLDOWN, etc., without getting
// into a shouting match with the toolbox.
//
WORD CToolboxWnd::SetToolStyle(WORD wID, WORD wStyle)
    {
    CTool* t = GetTool(wID);
    if (t)
        {
        WORD w = t->m_wStyle;
        t->m_wStyle = wStyle;
        t->m_wState = 0;
        Invalidate(FALSE);
        return w;
        }

    return 0;
    }


/******************************************************************************/
//
// SelectTool
//
// Selects a given tool and deselects all the other tools.      So, for instance,
// to select the arrow, call pToolbox->SelectTool(IDMB_ARROW);
//
void CToolboxWnd::SelectTool(WORD wID)
    {
    //
    // first clear all the tools except the one we want pressed, then
    // select the one we want.
    //
    for (int i = 0; i < m_Tools->GetSize(); i += 1)
        {
        CTool* pTool = (CTool*)m_Tools->GetAt(i);

        if (pTool->m_wID != wID)
            SetToolState(pTool->m_wID, 0);
        }

    SetToolState( wID, TF_SELECTED );
    }

/******************************************************************************/
/* CToolboxWnd::CurrentTool
 *
 * Returns the ID of the currently selected tool.
 */
WORD CToolboxWnd::CurrentToolID()
    {
    int nTools = (int)m_Tools->GetSize();
    for (int i = 0; i < nTools; i++)
        {
        CTool* t = (CTool*)m_Tools->GetAt(i);
        if (t && t->m_wState == TF_SELECTED)
            return t->m_wID;
        }
    return IDMB_ARROW;
    }

/******************************************************************************/

#define HITTYPE_SUCCESS         0               // hit an item in the control bar
#define HITTYPE_NOTHING         (-1)    // hit nothing, but hit the control bar itself
#define HITTYPE_OUTSIDE         (-2)    // hit a window outside of the control bar
#define HITTYPE_TRACKING        (-3)    // this app is has the focus (is tracking)
#define HITTYPE_INACTIVE        (-4)    // the app is not active
#define HITTYPE_DISABLED        (-5)    // the control bar is disabled
#define HITTYPE_FOCUS           (-6)    // the control bar has focus

int CToolboxWnd::HitTestToolTip( CPoint point, UINT* pHit )
    {
    if (pHit)
        *pHit = (UINT)-1;    // assume it won't hit anything

    int iReturn = HITTYPE_INACTIVE;

    // make sure this app is active
    if (theApp.m_bActiveApp)
        {
        // check for this application tracking (capture set)
        if (! m_tCapture)
            {
            // finally do the hit test on the items within the control bar
            ScreenToClient( &point );

            CRect  rect;
            CTool* pTool = ToolFromPoint( &rect, &point );

            if (pTool && rect.PtInRect( point ))
                {
                iReturn = HITTYPE_SUCCESS;

                if (pHit)
                    *pHit = pTool->m_wID;
                }
            else
                iReturn = HITTYPE_OUTSIDE;
            }
        else
            iReturn = HITTYPE_TRACKING;
                }

    #ifdef _DEBUG
    TRACE2( "HitTestToolType %d - %u\n", iReturn, pHit );
    #endif

    return iReturn;
    }

/******************************************************************************/

UINT CToolboxWnd::OnCmdHitTest( CPoint point, CPoint* pCenter )
    {
    ASSERT_VALID( this );

    // now hit test against CToolBar buttons
    CRect  rect;
    UINT   nHit  = (UINT)-1;
    CTool* pTool = ToolFromPoint( &rect, &point );

    if (pTool)
        nHit = pTool->m_wID;

    return nHit;
    }

/******************************************************************************/
// private ToolFromPoint:
// Given a CPoint in client coordinates, this function returns the tool
// associated with the button at that point, if any.  If a tool is found,
// the given CRect (if not NULL) is filled with the bounds of the tool's
// button.
//
CTool* CToolboxWnd::ToolFromPoint(CRect* rect, CPoint* pt)
    {
    CRect  c = m_rcTools;
    CPoint p = *pt;

    if (p.x < c.left || p.x >= c.right
    ||  p.y < c.top  || p.y >= c.bottom)
        return NULL;

    int x = p.x / (m_btnsize.x + m_nOffsetX);
    int y = p.y /  m_btnsize.y;
    int i = x + (y * m_wWide);

    if (i >= m_Tools->GetSize())
        return NULL;

    CTool* t = (CTool*)(m_Tools->GetAt( i ));

    if (t && rect)
        {
        rect->left   = m_btnsize.x * x + m_nOffsetX;
        rect->top    = m_btnsize.y * y;
        rect->right  = rect->left + m_btnsize.x;
        rect->bottom = rect->top  + m_btnsize.y;
        }

    return t;
    }

/******************************************************************************/
// OnLButtonDown:

void CToolboxWnd::OnLButtonDown(UINT wFlags, CPoint point)
    {
        wFlags; // Avoid unused arg warnings
    m_tCapture = ToolFromPoint( &m_lasttool, &point );
    m_downpt   = point;

    if (m_tCapture)
        {
        CRect   rect = m_lasttool;
        CString strPrompt;

        m_bInside = FALSE;

        rect.InflateRect( -1, -1 );

        if (rect.PtInRect( point ))
            {
            if (m_tCapture->m_wID <= IDMB_USERBTN)
                GetOwner()->SendMessage( WM_SETMESSAGESTRING, (WPARAM)m_tCapture->m_wID );

            if (m_tCapture->m_wState & TF_DISABLED)
                m_tCapture = NULL;
            else
                m_bInside = TRUE;
            }
        else
            m_tCapture = NULL;
        }
    else
        {
                CControlBar::OnLButtonDown(wFlags,point);
        }

    if (m_tCapture )
        {
        SetCapture();

        if (m_tCapture)
            InvalidateRect( &m_lasttool, FALSE );
        }
    }

/******************************************************************************/

void CToolboxWnd::OnRButtonDown(UINT wFlags, CPoint point)
    {
        wFlags;
        point;
    if (GetCapture() == this)
        CancelDrag();
    }

/******************************************************************************/
/*DK*/
//  LRESULT CToolboxWnd::OnHelpHitTest(WPARAM wParam, LPARAM lParam)
//      {
//      CPoint pt(lParam);
//      CTool* t = ToolFromPoint(&m_lasttool, &pt);
//
//      if (t == NULL)
//          return CMiniFrmWnd::OnHelpHitTest(wParam, lParam);
//      else
//          {
//          ASSERT( t->m_wID );
//          return HID_BASE_BUTTON + t->m_wID;
//          }
//      }


// OnLButtonDblClk:  FUTURE: Maybe just not use CS_DBLCLKS?
//
void CToolboxWnd::OnLButtonDblClk(UINT wFlags, CPoint point)
    {
    OnLButtonDown(wFlags, point);
    }

/******************************************************************************/
// OnMouseMove:
//
void CToolboxWnd::OnMouseMove(UINT wFlags, CPoint point)
    {
    CTool* t = m_tCapture;

    if (! t || (t->m_wState & TF_DISABLED))
        {
        CControlBar::OnMouseMove( wFlags, point );
        return;
        }

    BOOL bWasInside = m_bInside;
    CRect rect = m_lasttool;

    rect.InflateRect( -1, -1 );

    m_bInside = ((! (t->m_wState & TF_DRAG)) && rect.PtInRect( point ));

    if (bWasInside != m_bInside)
        InvalidateRect( &m_lasttool, FALSE );

    if (t && !(t->m_wState & TF_DISABLED))
        {
        // if it's a mousemove and we're draggable, then see how far it
        // is from the original mousedown -- if it's a fair distance,
        // then drag it.
        if ((t->m_wStyle & TS_DRAG) &&
                    (((point.x - m_downpt.x) > 3) ||
                     ((point.y - m_downpt.y) > 3) ||
                     ((m_downpt.x - point.x) > 3) ||
                     ((m_downpt.y - point.y) > 3)))
            {
            t->m_wState |= TF_DRAG;

            if (t->m_wStyle & TS_STICKY)
                {
                if (!(t->m_wState & TF_SELECTED))
                    {
                    t->m_wState |= TF_SELECTED;

                    if (t->m_pOwner)
                        t->m_pOwner->SendMessage(TM_TOOLDOWN, t->m_wID);
                    }

                if (m_bInside)
                    InvalidateRect(&m_lasttool, FALSE);

                m_bInside = FALSE; // looks stuck immediately!
                }
            }
        if (t->m_pOwner && (t->m_wState & TF_DRAG))
            {
            CPoint spt = point;
            ClientToScreen(&spt);

            // if the drag and drop began ok, release the captured tool
//          if (t->m_pOwner->BeginDragDrop( t, spt ))
//              m_tCapture = NULL;
            }
        }
    }


/******************************************************************************/
// OnLButtonUp:

void CToolboxWnd::OnLButtonUp(UINT wFlags, CPoint point)
    {
    if (! m_tCapture )
        {
        CControlBar::OnLButtonUp( wFlags, point );
        return;
        }

    CTool* t = m_tCapture;

    if (t && ! (t->m_wState & TF_DISABLED))
        {
        m_bInside = (point.x >= m_lasttool.left
                  && point.x <  m_lasttool.right
                  && point.y >= m_lasttool.top
                  && point.y <  m_lasttool.bottom);

        if (m_bInside)
            {
            if (t->m_wStyle & TS_STICKY)
                {
                if (! (t->m_wState & TF_DRAG))
                    {
                    t->m_wState ^= TF_SELECTED;

                    InvalidateRect(&m_lasttool, FALSE);

                    if (t->m_pOwner)
                        t->m_pOwner->SendMessage( t->m_wState & TF_SELECTED?
                                         TM_TOOLDOWN : TM_TOOLUP, t->m_wID );
                    }
                }

            if (t->m_wStyle & TS_CMD)
                {
                if (t->m_pOwner)
                    AfxGetMainWnd()->SendMessage( WM_COMMAND, t->m_wID );
                }
            }
        }
    ReleaseCapture();
    m_tCapture = NULL;
    m_bInside  = FALSE;
    }

/******************************************************************************/

BOOL CToolboxWnd::OnCommand(UINT wParam, LONG lParam)
    {
    AfxGetMainWnd()->SendMessage(WM_COMMAND, wParam, lParam);
    return TRUE;
    }

/******************************************************************************/

void CToolboxWnd::DrawButtons(CDC& dc, RECT* rcPaint)
    {
    CRect rect;
    CRect updateRect;
    int i, n;

    if (rcPaint == NULL)
        {
        GetClientRect( &updateRect );
        rcPaint = &updateRect;
        }

    CDC memdc;
    memdc.CreateCompatibleDC(&dc);

        // Force the buttons to be rebuilt here
        DrawStockBitmaps();

    CBitmap* obm = memdc.SelectObject( m_bmPopped );
    CBrush*  obr = memdc.SelectObject( GetSysBrush( COLOR_BTNTEXT ) );

    n = (int)m_Tools->GetSize();

    BOOL bUsedImageWell = FALSE;

    for (i = 0; i < n; i++)
        {
        CTool* t = (CTool*)m_Tools->GetAt(i);

        if (! t)
            continue;

        rect.left   = (i % m_wWide) * m_btnsize.x + m_nOffsetX;
        rect.top    = (i / m_wWide) * m_btnsize.y;

        rect.right  = rect.left + m_btnsize.x;
        rect.bottom = rect.top  + m_btnsize.y;

        CRect ir;

        if (! ir.IntersectRect( rcPaint, &rect ))
            continue;

        // Select the right stock bitmap, and remember to
        // shove the graphic if it's in a pushed state.
        //
        CBitmap* bmStock = m_bmPopped;
        int xshove = 0, yshove = 0;

        if (t->m_wState & (TF_SELECTED | TF_DRAG))
            {
            bmStock = m_bmStuck;
            xshove = 1; yshove = 1;
            }
        if ((t == m_tCapture && m_bInside) && !(t->m_wState & TF_DRAG))
            {
            bmStock = m_bmPushed;
            xshove = 2; yshove = 2;
            }

        // Draw a blank button first...
        ::DrawBitmap(&dc, bmStock, &rect, SRCCOPY, &memdc);

        // Now draw the glyph on top...
        rect.OffsetRect( xshove, yshove );

        if (! bUsedImageWell)
            {
            if (! m_imageWell.Open())
                goto LReturn;

            bUsedImageWell = TRUE;

            if (! m_imageWell.CalculateMask())
                goto LReturn;
            }

        CPoint pt( rect.left + (rect.Width()  - 16) / 2,
                   rect.top  + (rect.Height() - 16) / 2 );

        m_imageWell.DrawImage( &dc, pt, t->m_nImage );
        }

LReturn:
    if (bUsedImageWell)
        m_imageWell.Close();

    memdc.SelectObject(obr);
    memdc.SelectObject(obm);
    memdc.DeleteDC();
    }

/******************************************************************************/
// OnPaint:
//

void CToolboxWnd::OnPaint()
    {
    CPaintDC dc(this);
    if (dc.m_hDC == NULL)
        {
        theApp.SetGdiEmergency();
        return;
        }
    DrawButtons(dc, &dc.m_ps.rcPaint);
    }

/******************************************************************************/
// OnClose
//
// A toolbox is usally created by the parent, and will be destroyed
// specifically by the parent upon leaving the app.  When the user closes
// the toolbox, it is simply hidden.  The parent can then reshow it without
// recreating it.
//
// This also changes the menu test to "show" rather than "hide"

void CToolboxWnd::OnClose()
    {
#ifdef TRYANYTHING
        CControlBar::OnClose();
#endif
//      ShowWindow(SW_HIDE);
    }

/******************************************************************************/
// OnToolDown:
//
LONG CToolboxWnd::OnToolDown(UINT wID, LONG /* lParam */)
    {
    for (int i = 0; i < m_Tools->GetSize(); i += 1)
        {
        CTool* pTool = (CTool*)m_Tools->GetAt(i);

        if (pTool->m_wID != wID)
            SetToolState(pTool->m_wID, 0);
        }

    return (LONG)TRUE;
    }

/******************************************************************************/
// OnToolUp:
//
LONG CToolboxWnd::OnToolUp(UINT /* wID */, LONG /* lParam */)
    {
    for (int i = 0; i < m_Tools->GetSize(); i += 1)
        {
        CTool* pTool = (CTool*)m_Tools->GetAt(i);
        SetToolState(pTool->m_wID, 0);
        }
    SetToolState(IDMB_ARROW, TF_SELECTED);

    return (LONG)TRUE;
    }
#ifdef XYZZYZ
/******************************************************************************/
// OnSwitch:
//
LONG CToolboxWnd::OnSwitch(UINT /* wID */, LONG /* point */)
    {
    return (LONG)TRUE;
    }

/******************************************************************************/
// OnQueryDrop:
//
BOOL CToolboxWnd::BeginDragDrop (CTool* /*pTool*/, CPoint /*pt*/)
    {
    return FALSE;
    }
#endif
/******************************************************************************/

CTool::CTool(CToolboxWnd* pOwner, WORD wID, int nImage,
        WORD wStyle /* = 0 */, WORD wState /* = 0 */)
    {
    m_pOwner = pOwner;
    m_wID    = wID;
    m_nImage = nImage;
    m_wStyle = wStyle;
    m_wState = wState;
    }

/******************************************************************************/



