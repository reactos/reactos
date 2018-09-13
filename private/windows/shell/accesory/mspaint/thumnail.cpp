/******************************************************************************/
/* THUMNAIL.CPP: IMPLEMENTATION OF THE CThumbNailView and CFloatThumNailView  */
/*               and CFullScreenThumbNailView Classes                         */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Methods in this file                                                       */
/*                                                                            */
/*  CThumbNailView Class Object                                               */
/*     CThumbNailView::CThumbNailView                                         */
/*     CThumbNailView::CThumbNailView                                         */
/*     CThumbNailView::~CThumbNailView                                        */
/*     CThumbNailView::Create                                                 */
/*     CThumbNailView::OnSize                                                 */
/*     CThumbNailView::OnPaint                                                */
/*     CThumbNailView::DrawImage                                              */
/*     CThumbNailView::DrawTracker                                            */
/*     CThumbNailView::RefreshImage                                           */
/*     CThumbNailView::GetImgWnd                                              */
/*     CThumbNailView::OnKeyDown                                              */
/*     CThumbNailView::OnLButtonDown                                          */
/*     CThumbNailView::OnRButtonDown                                          */
/*     CThumbNailView::OnThumbnailThumbnail                                   */
/*     CThumbNailView::OnUpdateThumbnailThumbnail                             */
/*                                                                            */
/*  CFloatThumbNailView Class Object                                          */
/*     CFloatThumbNailView::CFloatThumbNailView                               */
/*     CFloatThumbNailView::~CFloatThumbNailView                              */
/*     CFloatThumbNailView::Create                                            */
/*     CFloatThumbNailView::OnClose                                           */
/*     CFloatThumbNailView::OnSize                                            */
/*                                                                            */
/*  CFullScreenThumbNailView Class Object                                     */
/*     CFullScreenThumbNailView::CFullScreenThumbNailView                     */
/*     CFullScreenThumbNailView::CFullScreenThumbNailView                     */
/*     CFullScreenThumbNailView::~CFullScreenThumbNailView                    */
/*     CFullScreenThumbNailView::Create                                       */
/*     CFullScreenThumbNailView::OnLButtonDown                                */
/*     CFullScreenThumbNailView::OnKeyDown                                    */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  These 3 objects provide a layer around the thumbnail view window, which   */
/*  allow it to easily be a child, floating or a full screen. The ThumbNail   */
/*  View Window is just a CWnd Window which on paints does a BitBlt from the  */
/*  CImgWnd it was passsed on construction.                                   */
/*                                                                            */
/*  The structure of the objects is as follows:                               */
/*                                                                            */
/*  CFullScreenThumbNailView is a Frame Window  (with no border and sized to  */
/*      full screen).  It destroys itself on any keystroke or button click     */
/*      while visible, it dissables the main application window.  It contains */
/*      a CThumbNailView object as a child window.                            */
/*                                                                            */
/*  CFloatThumbNailView is a MiniFrame Window                                 */
/*     CThumbNailView is a Child Window (which is sizable) A child of the     */
/*                    the CFloatThumbNailView window.  This can be created    */
/*                    independent if a floating window is not desired (i.e.   */
/*                    for the docked view). It is this window which has the   */
/*                    image drawn into it.                                    */
/*                                                                            */
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
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgcolor.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "imgwnd.h"
#include "thumnail.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CThumbNailView, CWnd)
IMPLEMENT_DYNAMIC(CFloatThumbNailView, CMiniFrmWnd)
IMPLEMENT_DYNAMIC(CFullScreenThumbNailView, CFrameWnd)


#include "memtrace.h"

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

BEGIN_MESSAGE_MAP(CThumbNailView, CWnd)
    //{{AFX_MSG_MAP(CThumbNailView)
    ON_WM_PAINT()
    ON_WM_KEYDOWN()
    ON_WM_LBUTTONDOWN()
    ON_WM_RBUTTONDOWN()
    ON_COMMAND(ID_THUMBNAIL_THUMBNAIL, OnThumbnailThumbnail)
    ON_UPDATE_COMMAND_UI(ID_THUMBNAIL_THUMBNAIL, OnUpdateThumbnailThumbnail)
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************/

CThumbNailView::CThumbNailView(CImgWnd *pcImgWnd)
    {
    m_pcImgWnd = pcImgWnd;
    }

/******************************************************************************/

CThumbNailView::CThumbNailView()
    {
    m_pcImgWnd = NULL;
    }

/******************************************************************************/

CThumbNailView::~CThumbNailView(void)
    {
    }

/******************************************************************************/

BOOL CThumbNailView::Create(DWORD dwStyle, CRect cRectWindow, CWnd *pcParentWnd)
    {
    return( CWnd::Create(NULL, TEXT(""), dwStyle, cRectWindow, pcParentWnd, NULL) );
    }

/***************************************************************************/

void CThumbNailView::OnClose()
    {
    ShowWindow(SW_HIDE);
    }

/******************************************************************************/

void CThumbNailView::OnPaint()
    {
    CPaintDC dc(this); // device context for painting

    // TODO: Add your message handler code here

#ifdef USE_MIRRORING
    //
    // Disable RTL mirroring on full screen window
    //
    if (PBGetLayout(dc.GetSafeHdc()) & LAYOUT_RTL)
    {
        PBSetLayout(dc.GetSafeHdc(), 0);
    }
#endif

    // Do not call CWnd::OnPaint() for painting messages
    DrawImage(&dc);
    }

/******************************************************************************/

void CThumbNailView::DrawImage(CDC* pDC)
    {
    /*
    **  when there is nothing to do, then don't do it
    */
    if (! theApp.m_bShowThumbnail || m_pcImgWnd         == NULL
                                  || m_pcImgWnd->m_pImg == NULL )
        return;

    CRect crectClient;
    int   iMinWidth;
    int   iMinHeight;
    int   iLeft;
    int   iTop;

    CSize cSizeScrollPos = m_pcImgWnd->GetScrollPos();

    cSizeScrollPos.cx = abs( cSizeScrollPos.cx ) - CTracker::HANDLE_SIZE;
    cSizeScrollPos.cy = abs( cSizeScrollPos.cy ) - CTracker::HANDLE_SIZE;

    GetClientRect(crectClient);

    // find the smaller of the two the real image or the thumbnail window.

    iMinWidth  = min( crectClient.Width() , m_pcImgWnd->m_pImg->cxWidth  );
    iMinHeight = min( crectClient.Height(), m_pcImgWnd->m_pImg->cyHeight );

    if (crectClient.Width() >= m_pcImgWnd->m_pImg->cxWidth)
        {
        iLeft = 0; // can fit the whole image width into the thumbnail
        }
    else // image width greater than thumbnail width
        {
        // does thumbnail extend past end if started at scroll pos?
        if (cSizeScrollPos.cx + crectClient.Width() > m_pcImgWnd->m_pImg->cxWidth)
            {
            iLeft = cSizeScrollPos.cx - ( (cSizeScrollPos.cx
                                          + crectClient.Width()
                                          - m_pcImgWnd->m_pImg->cxWidth));
            }
        else
            {
            iLeft = cSizeScrollPos.cx;
            }
        }

    if (crectClient.Height() >= m_pcImgWnd->m_pImg->cyHeight)
        {
        iTop = 0; // can fit the whole image height into the thumbnail
        }
    else // image height greater than thumbnail height
        {
        // does thumbnail extend past bottom if started at scroll pos?
        if (cSizeScrollPos.cy + crectClient.Height() > m_pcImgWnd->m_pImg->cyHeight)
            {
            iTop = cSizeScrollPos.cy - ( (cSizeScrollPos.cy
                                          + crectClient.Height()
                                          - m_pcImgWnd->m_pImg->cyHeight));
            }
        else
            {
            iTop = cSizeScrollPos.cy;
            }
        }

    CDC cDC;
    cDC.Attach(m_pcImgWnd->m_pImg->hDC);

    CPalette* ppalOldSrc = theImgBrush.SetBrushPalette(&cDC, FALSE);
    CPalette* ppalOldDst = theImgBrush.SetBrushPalette( pDC, FALSE);

    pDC->BitBlt(0, 0, iMinWidth, iMinHeight,
                &cDC, iLeft, iTop, SRCCOPY);

    if (ppalOldDst)
    {
        pDC->SelectPalette(ppalOldDst, FALSE);
    }
    if (ppalOldSrc)
    {
        cDC.SelectPalette(ppalOldSrc, FALSE);
    }

    cDC.Detach();

    DrawTracker(pDC);
    }

/******************************************************************************/
/* basically the same processing as the imgwnd::drawtracker method, without   */
/* the zoom */

void CThumbNailView::DrawTracker(CDC *pDC)
    {
//  BOOL bDrawTrackerRgn = FALSE;

    if (m_pcImgWnd->GetCurrent() != m_pcImgWnd
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

    GetClientRect(&clientRect);

    CRect trackerRect;

    m_pcImgWnd->GetImageRect(trackerRect);

    trackerRect.InflateRect(CTracker::HANDLE_SIZE, CTracker::HANDLE_SIZE);

    CTracker::EDGES edges = (CTracker::EDGES)(CTracker::right | CTracker::bottom);

//  if (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
//      {
//      bDrawTrackerRgn = TRUE;
//      }

    if (m_pcImgWnd->m_pImg == theImgBrush.m_pImg)
        {
        edges = CTracker::all;
        CSize cSzScroll = m_pcImgWnd->GetScrollPos();

        trackerRect = theImgBrush.m_rcSelection;

//      trackerRect.InflateRect( CTracker::HANDLE_SIZE,
//                               CTracker::HANDLE_SIZE);
        trackerRect.OffsetRect(  cSzScroll.cx, cSzScroll.cy);

        }

    if (m_pcImgWnd->m_pImg == theImgBrush.m_pImg)
        {
//      if (bDrawTrackerRgn)
//          {
//          CTracker::DrawBorderRgn( pDC, trackerRect, &(theImgBrush.m_cRgnPolyFreeHandSel) );
//          }
//      else
//          {
            CTracker::DrawBorder( pDC, trackerRect );
//          }
        }

    if (bReleaseDC)
        {
        ReleaseDC(pDC);
        }
    }

/******************************************************************************/
/* Basically Do a paint without an erase background to prevent blinking       */

void CThumbNailView::RefreshImage(void)
    {
    if (theApp.m_bShowThumbnail)
        {
        TRY
            {
            CClientDC dc(this);
            DrawImage(&dc);
            }
        CATCH(CResourceException,e)
            {
            }
        END_CATCH
        }
    }

/******************************************************************************/

CImgWnd* CThumbNailView::GetImgWnd(void)
    {
    return m_pcImgWnd;
    }

/******************************************************************************/

void CThumbNailView::UpdateThumbNailView()
    {
    CPBView* pcbActiveView = (CPBView*)((CFrameWnd*)AfxGetMainWnd())->GetActiveView();

    m_pcImgWnd = pcbActiveView->m_pImgWnd;
    }

/******************************************************************************/

void CThumbNailView::OnKeyDown(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
    {
    const MSG* pmsg = GetCurrentMessage();

    GetParent()->SendMessage( pmsg->message, pmsg->wParam, pmsg->lParam );
    }

/******************************************************************************/

void CThumbNailView::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/)
    {
    const MSG* pmsg = GetCurrentMessage();

    GetParent()->SendMessage(pmsg->message, pmsg->wParam, pmsg->lParam);
    }

/******************************************************************************/

void CThumbNailView::OnRButtonDown(UINT /*nFlags*/, CPoint point)
    {
    HWND  hwnd = GetSafeHwnd();  // must do this before calling SendMsg to parent, since it could delete us,
    const MSG* pmsg = GetCurrentMessage();

    GetParent()->SendMessage(pmsg->message, pmsg->wParam, pmsg->lParam);
    // the window is destroyed by the parent if FullScreenView

    if (::IsWindow(hwnd) != FALSE)  // window still exists => object still valid, put up pop up menu.
        {
        CMenu cMenuPopup;
        CMenu *pcContextMenu;
        BOOL  bRC;
        CRect cRectClient;

        GetClientRect(&cRectClient);

        bRC = cMenuPopup.LoadMenu( IDR_THUMBNAIL_POPUP );

        ASSERT(bRC != 0);

        if (bRC != 0)
            {
            pcContextMenu = cMenuPopup.GetSubMenu(0);

            ASSERT(pcContextMenu != NULL);

            if (pcContextMenu != NULL)
                {
                // update the check marks
                ClientToScreen(&point);
                ClientToScreen(&cRectClient);
                pcContextMenu->CheckMenuItem(ID_THUMBNAIL_THUMBNAIL, MF_BYCOMMAND | MF_CHECKED);
                pcContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, &cRectClient);
                }
            }
        }
    }

/******************************************************************************/

void CThumbNailView::OnThumbnailThumbnail()
    {
    CPBView* pView = (CPBView*)(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());

    if (pView != NULL && pView->IsKindOf( RUNTIME_CLASS( CPBView ) ))
        pView->HideThumbNailView();
    }

/******************************************************************************/

void CThumbNailView::OnUpdateThumbnailThumbnail(CCmdUI* pCmdUI)
    {
    pCmdUI->SetCheck();
    }

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

BEGIN_MESSAGE_MAP(CFloatThumbNailView, CMiniFrmWnd)
    //{{AFX_MSG_MAP(CFloatThumbNailView)
    ON_WM_CLOSE()
    ON_WM_SIZE()
        ON_WM_GETMINMAXINFO()
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************/

CFloatThumbNailView::CFloatThumbNailView(CImgWnd *pcMainImgWnd)
    {
    m_pcThumbNailView = new CThumbNailView(pcMainImgWnd);

    if (m_pcThumbNailView == NULL)
        {
        theApp.SetMemoryEmergency();
        TRACE( TEXT("New Thumbnail View faild\n") );
        }
    }

/******************************************************************************/

CFloatThumbNailView::CFloatThumbNailView()
    {
    m_pcThumbNailView = NULL;
    }

/******************************************************************************/

CFloatThumbNailView::~CFloatThumbNailView(void)
    {
    }

/******************************************************************************/

BOOL CFloatThumbNailView::Create(CWnd* pParentWnd)
    {
    BOOL bRC;
    CRect cWindowRect;

    pParentWnd->GetWindowRect( &cWindowRect );

    cWindowRect.BottomRight() = cWindowRect.TopLeft();
    cWindowRect.right   += 120;
    cWindowRect.bottom  += 120;
    cWindowRect.OffsetRect( 15, 15 );

    if (! theApp.m_rectFloatThumbnail.IsRectEmpty())
        {
        cWindowRect = theApp.m_rectFloatThumbnail;
        }

    CString pWindowName;

    pWindowName.LoadString( IDS_VIEW );

    bRC = CMiniFrmWnd::Create( pWindowName, WS_THICKFRAME, cWindowRect, pParentWnd );

    if (bRC)
        {
        ASSERT( m_pcThumbNailView );

        GetClientRect( &cWindowRect );

        if (!m_pcThumbNailView->Create( WS_CHILD | WS_VISIBLE, cWindowRect, this ))
            {
            bRC = FALSE;
            theApp.SetMemoryEmergency();
            TRACE( TEXT("New Thumbnail View faild\n") );
            }
        }

    GetWindowRect( &theApp.m_rectFloatThumbnail );

    return bRC;
    }

/******************************************************************************/
// OnClose
//
// A Colorsbox is usally created by the parent, and will be destroyed
// specifically by the parent upon leaving the app.  When the user closes
// the Colorsbox, it is simply hidden.  The parent can then reshow it without
// recreating it.
//
void CFloatThumbNailView::OnClose()
    {
    theApp.m_bShowThumbnail = FALSE;

    ShowWindow(SW_HIDE);
    }

/******************************************************************************/

void CFloatThumbNailView::PostNcDestroy()
    {
    if (m_pcThumbNailView != NULL)
        {
        delete m_pcThumbNailView;
        m_pcThumbNailView = NULL;
        }

    CWnd::PostNcDestroy();
    }

/******************************************************************************/

void CFloatThumbNailView::OnSize(UINT nType, int cx, int cy)
    {
    CMiniFrmWnd::OnSize(nType, cx, cy);

    if (m_pcThumbNailView                != NULL
    &&  m_pcThumbNailView->GetSafeHwnd() != NULL)
        {
        m_pcThumbNailView->SetWindowPos( &wndTop, 0, 0, cx, cy, SWP_NOACTIVATE );
        }

    theApp.m_rectFloatThumbnail.right  = theApp.m_rectFloatThumbnail.left + cx;
    theApp.m_rectFloatThumbnail.bottom = theApp.m_rectFloatThumbnail.top  + cy;
    }

/******************************************************************************/

void CFloatThumbNailView::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
    {
    lpMMI->ptMinTrackSize.x = 2 * GetSystemMetrics( SM_CXICON );
    lpMMI->ptMinTrackSize.y = 2 * GetSystemMetrics( SM_CYICON );

    CWnd::OnGetMinMaxInfo( lpMMI );
    }

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

BEGIN_MESSAGE_MAP(CFullScreenThumbNailView, CFrameWnd)
    //{{AFX_MSG_MAP(CFullScreenThumbNailView)
    ON_WM_LBUTTONDOWN()
    ON_WM_KEYDOWN()
    ON_WM_RBUTTONDOWN()
    ON_WM_ERASEBKGND()
    ON_WM_CLOSE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/******************************************************************************/

CFullScreenThumbNailView::CFullScreenThumbNailView(CImgWnd *pcMainImgWnd)
    {
    m_bSaveShowFlag = theApp.m_bShowThumbnail;
    theApp.m_bShowThumbnail = TRUE;


//  m_brBackground.CreateSolidBrush( ::GetSysColor( COLOR_BACKGROUND ) );

    m_pcThumbNailView = new CThumbNailView(pcMainImgWnd);

    if (m_pcThumbNailView == NULL)
        {
        theApp.SetMemoryEmergency();
        TRACE( TEXT("New Thumbnail View faild\n") );
        }
    }

/******************************************************************************/

CFullScreenThumbNailView::CFullScreenThumbNailView()
    {
    m_hOldIcon = 0;
    m_pcThumbNailView = NULL;
    }

/******************************************************************************/

CFullScreenThumbNailView::~CFullScreenThumbNailView(void)
    {
    if (m_hOldIcon)
    {
       SetClassLongPtr (((CFrameWnd*)this)->GetSafeHwnd(), GCLP_HICON, (LONG_PTR)m_hOldIcon);
    }
    if (m_pcThumbNailView != NULL)
        {
        delete m_pcThumbNailView;

        theApp.m_bShowThumbnail = m_bSaveShowFlag;
        }
//  if (m_brBackground.m_hObject != NULL)
//      m_brBackground.DeleteObject();
    }

/******************************************************************************/

BOOL CFullScreenThumbNailView::Create(LPCTSTR szCaption)
    {
    ASSERT( m_pcThumbNailView );
    TCHAR szFileName[MAX_PATH];
    HICON hIcon;

    CRect cWindowRect( 0, 0, ::GetSystemMetrics( SM_CXSCREEN ),
                             ::GetSystemMetrics( SM_CYSCREEN ) );
    //
    // Use the current file name as the caption of the window so
    // it shows up in alt-tab
    if (szCaption && *szCaption)
    {
       GetFileTitle (szCaption, szFileName, MAX_PATH);
    }
    else
    {
       LoadString (GetModuleHandle (NULL), AFX_IDS_UNTITLED, szFileName, MAX_PATH);
    }

    BOOL bRC = CFrameWnd::Create( NULL, szFileName,  WS_POPUP|WS_VISIBLE | WS_CLIPCHILDREN,
                                                                       cWindowRect );
    //
    // This window needs a Paint icon instead of a boring icon
    // So set the class's icon to the Paint icon
    // We want alt-tab to work decently
    hIcon = LoadIcon (GetModuleHandle (NULL), MAKEINTRESOURCE(ID_MAINFRAME));
    m_hOldIcon = SetClassLongPtr (((CFrameWnd*)this)->GetSafeHwnd(), GCLP_HICON, (LONG_PTR)hIcon);

    if (bRC)
        {
        ASSERT( m_pcThumbNailView );

        AfxGetMainWnd()->EnableWindow( FALSE );

        CImgWnd* pcImgWnd = m_pcThumbNailView->GetImgWnd();

        if (pcImgWnd != NULL)
            {
            // find the smaller of the two the real image or the full screen window size.
            int iMinWidth  = min( cWindowRect.Width(),  pcImgWnd->m_pImg->cxWidth  );
            int iMinHeight = min( cWindowRect.Height(), pcImgWnd->m_pImg->cyHeight );

            // center the image in the full screen window.
            cWindowRect.left   =  (cWindowRect.Width()  - iMinWidth)  / 2;
            cWindowRect.top    =  (cWindowRect.Height() - iMinHeight) / 2;
            cWindowRect.right  =   cWindowRect.left     + iMinWidth;
            cWindowRect.bottom =   cWindowRect.top      + iMinHeight;

            m_pcThumbNailView->Create( WS_CHILD | WS_VISIBLE, cWindowRect, this );
            }
        }

    return bRC;
    }

/******************************************************************************/

BOOL CFullScreenThumbNailView::OnEraseBkgnd( CDC* pDC )
    {
    CBrush* pbr = GetSysBrush( COLOR_BACKGROUND );

//  if (m_brBackground.m_hObject == NULL)
    if (! pbr)
            return CFrameWnd::OnEraseBkgnd( pDC );

    CRect cRectClient;

    GetClientRect( &cRectClient );
    pDC->FillRect( &cRectClient, pbr /* &m_brBackground */ );

    return TRUE;
    }


/******************************************************************************/
void CFullScreenThumbNailView::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/)
    {
    PostMessage (WM_CLOSE, 0, 0);
    }

/******************************************************************************/

void CFullScreenThumbNailView::OnKeyDown(UINT /*nChar*/, UINT /*nRepCnt*/, UINT /*nFlags*/)
    {
    PostMessage (WM_CLOSE, 0, 0);
    }

/******************************************************************************/


void CFullScreenThumbNailView::OnClose ()
    {
    AfxGetMainWnd()->EnableWindow( TRUE );
    ::DestroyWindow( m_hWnd );
    }
/******************************************************************************/

