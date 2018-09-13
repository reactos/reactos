// pbrusfrm.cpp : implementation of the CPBFrame class
//

#include "stdafx.h"
#include "resource.h"
#include "global.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "bmobject.h"
#include "docking.h"
#include "minifwnd.h"
#include "imgwnd.h"
#include "imgwell.h"
#include "imgtools.h"
#include "imgdlgs.h"
#include "cmpmsg.h"
#include "props.h"
#include "colorsrc.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE( CPBFrame, CFrameWnd )

#include "memtrace.h"

#define UM_FILE_ERROR     WM_USER + 1001

/*************************** CPBFrame **************************************/

BEGIN_MESSAGE_MAP( CPBFrame, CFrameWnd )
    //{{AFX_MSG_MAP(CPBFrame)
        ON_WM_ACTIVATEAPP()
        ON_WM_CREATE()
        ON_WM_DESTROY()
        ON_WM_SETFOCUS()
        ON_WM_PALETTECHANGED()
        ON_WM_QUERYNEWPALETTE()
        ON_WM_GETMINMAXINFO()
        ON_WM_MOVE()
        ON_WM_SIZE()
        ON_WM_ERASEBKGND()
        ON_WM_DEVMODECHANGE()
        ON_WM_WININICHANGE()
        ON_COMMAND(ID_HELP, OnHelp)
        ON_WM_SYSCOLORCHANGE()
        ON_WM_CLOSE()
        //}}AFX_MSG_MAP

    ON_MESSAGE(UM_FILE_ERROR, OnFileError)

    // Global help commands
    ON_COMMAND(ID_HELP_INDEX, CFrameWnd::OnHelpIndex)
    ON_COMMAND(ID_HELP_USING, CFrameWnd::OnHelpUsing)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpIndex)

        ON_UPDATE_COMMAND_UI(ID_VIEW_TOOL_BOX, CFrameWnd::OnUpdateControlBarMenu)
        ON_COMMAND_EX(ID_VIEW_TOOL_BOX, CFrameWnd::OnBarCheck)
        ON_UPDATE_COMMAND_UI(ID_VIEW_COLOR_BOX, CFrameWnd::OnUpdateControlBarMenu)
        ON_COMMAND_EX(ID_VIEW_COLOR_BOX, CFrameWnd::OnBarCheck)
END_MESSAGE_MAP()

/***************************************************************************/

/*********************** CPBFrame construction/destruction *****************/

CPBFrame::CPBFrame()
    {
    // Just small enough so that the control bars fit
    m_szFrameMin = CSize( 275, 400 );
    }

/***************************************************************************/

CPBFrame::~CPBFrame()
    {
    // also can't delete objects  derived from class cframewnd, must destroy their
    // window => deletion indirectly
    }

/*************************** CPBFrame diagnostics **************************/

#ifdef _DEBUG
void CPBFrame::AssertValid() const
    {
    CFrameWnd::AssertValid();
    }

/***************************************************************************/

void CPBFrame::Dump(CDumpContext& dc) const
    {
    CFrameWnd::Dump(dc);
    }

#endif //_DEBUG

/***************************************************************************/

TCHAR mszMSPaintClass[] = TEXT("MSPaintApp");

BOOL CPBFrame::PreCreateWindow( CREATESTRUCT& cs )
    {
        cs.dwExStyle |= WS_EX_WINDOWEDGE;
    cs.style |= WS_CLIPCHILDREN;
    BOOL bResult = CFrameWnd::PreCreateWindow( cs );

    if (bResult)
        {
        WINDOWPLACEMENT& wpSaved = theApp.m_wpPlacement;
        RECT& rcSaved = wpSaved.rcNormalPosition;

        CPoint ptPBrush(rcSaved.left, rcSaved.top);
        CSize sizePBrush(rcSaved.right - rcSaved.left, rcSaved.bottom - rcSaved.top);

        CPoint pt = theApp.CheckWindowPosition( ptPBrush, sizePBrush );
        if (pt.x || pt.y)
        {
            cs.x = pt.x;
            cs.y = pt.y;
        }

        sizePBrush.cx = max(sizePBrush.cx, m_szFrameMin.cx);
        sizePBrush.cy = max(sizePBrush.cy, m_szFrameMin.cy);
        if (sizePBrush.cx && sizePBrush.cy)
        {
            cs.cx = sizePBrush.cx;
            cs.cy = sizePBrush.cy;
        }

        rcSaved.left = cs.x;
        rcSaved.top  = cs.y;
        rcSaved.right = rcSaved.left + cs.cx;
        rcSaved.bottom = rcSaved.top + cs.cy;

        WNDCLASS  wndcls;
        HINSTANCE hInst = AfxGetInstanceHandle();

        // see if the class already exists
        if (! ::GetClassInfo( hInst, mszMSPaintClass, &wndcls ))
            {
            // get default stuff
            ::GetClassInfo( hInst, cs.lpszClass, &wndcls );

            // register a new class
            wndcls.lpszClassName = mszMSPaintClass;
            wndcls.hIcon         = ::LoadIcon( hInst, MAKEINTRESOURCE( IDI_PAINT_DOC) );

            ASSERT( wndcls.hIcon != NULL );

            if (! AfxRegisterClass( &wndcls ))
                AfxThrowResourceException();
            }
        cs.lpszClass = mszMSPaintClass;
        }
    return bResult;
    }

/***************************************************************************/

CWnd* CPBFrame::GetMessageBar()
    {
    if (m_statBar.m_hWnd != NULL)
        return &m_statBar;

    return NULL;
    }

/***************************************************************************/

void CPBFrame::OnHelp()
    {
    if (m_dwPromptContext)
        CFrameWnd::OnHelp();
    else
        ::HtmlHelpA( ::GetDesktopWindow(), "mspaint.chm", HH_DISPLAY_TOPIC, 0L );
    }

/***************************************************************************/

int CPBFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
    {
    if (CFrameWnd::OnCreate( lpCreateStruct ) == -1)
        return -1;
    g_pStatBarWnd = &m_statBar;
    g_pImgToolWnd = &m_toolBar;
    g_pImgColorsWnd = &m_colorBar;
    return 0;
    }

/***************************************************************************/

void CPBFrame::OnDestroy()
    {
    CFrameWnd::OnDestroy();

    theApp.m_wpPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(&theApp.m_wpPlacement);

    theApp.SaveProfileSettings();
    }

/***************************************************************************/

BOOL CPBFrame::OnEraseBkgnd(CDC* pDC)
    {


        if ( !m_pViewActive )   //fix gray background on screen while IME disappear problem
        {


    CRect cRectClient;

    GetClientRect( &cRectClient );
    pDC->FillRect( &cRectClient, GetSysBrush( COLOR_BTNFACE ) );


    }


    return CFrameWnd::OnEraseBkgnd( pDC );
    }

/***************************************************************************/

void CPBFrame::OnActivateApp(BOOL bActive, HTASK hTask)
    {
    theApp.m_bActiveApp = bActive;

    CFrameWnd::OnActivateApp(bActive, hTask);
    }

/***************************************************************************/

void CPBFrame::OnMove( int x, int y )
    {
    CRect cRectWindow;

    GetWindowRect( &cRectWindow );
    m_ptPosition.x = cRectWindow.left;
    m_ptPosition.y = cRectWindow.top;

    CWnd::OnMove( x, y );
    }

/***************************************************************************/

void CPBFrame::OnSize( UINT nType, int cx, int cy )
    {
    CFrameWnd::OnSize( nType, cx, cy );

    CRect rect;

    GetWindowRect( &rect );

    m_szFrame = rect.Size();
    }

/***************************************************************************/

void CPBFrame::OnSetFocus(CWnd* pOldWnd)
    {
    CFrameWnd::OnSetFocus( pOldWnd );

        // We need to update the window here because the SetFocus below will update
        // the image window, and then some async paints can come after that which
        // will cause us to put the background color over parts of the window (see
        // WIN95C bug #4080).
    UpdateWindow();

    CPBView* pView = (CPBView*)GetActiveView();

    if (pView
    &&  pView->IsKindOf( RUNTIME_CLASS( CPBView ) )
    &&  pView->m_pImgWnd != NULL
    &&  ::IsWindow(pView->m_pImgWnd->m_hWnd) )
        pView->m_pImgWnd->SetFocus();
    }

/***************************************************************************/

void CPBFrame::OnPaletteChanged( CWnd* pFocusWnd )
    {
    CFrameWnd::OnPaletteChanged( pFocusWnd );

    CPBView* pView = (CPBView*)GetActiveView();

    if (pView != NULL && pView->IsKindOf( RUNTIME_CLASS( CPBView ) ))
        {
        pView->OnPaletteChanged( pFocusWnd );
        }
    }

/***************************************************************************/

BOOL CPBFrame::OnQueryNewPalette()
    {
    CPBView* pView = (CPBView*)GetActiveView();

    if (pView != NULL && ::IsWindow(pView->m_hWnd) && pView->IsKindOf( RUNTIME_CLASS( CPBView ) ))
        {
        pView->OnQueryNewPalette();
        }

    return CFrameWnd::OnQueryNewPalette();
    }

/***************************************************************************/

void CPBFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI)
    {
    lpMMI->ptMinTrackSize.x = m_szFrameMin.cx;
    lpMMI->ptMinTrackSize.y = m_szFrameMin.cy;

    CFrameWnd::OnGetMinMaxInfo(lpMMI);
    }

/*****************************************************************************/

void CPBFrame::OnDevModeChange( LPTSTR lpDeviceName )
    {
    CClientDC dc( this );

    if (dc.m_hDC != NULL)
        theApp.GetSystemSettings( &dc );

    CFrameWnd::OnDevModeChange( lpDeviceName );
    }

/*****************************************************************************/

void CPBFrame::OnWinIniChange( LPCTSTR lpszSection )
    {
    CClientDC dc( this );
    CDocument *pDoc;

    if (dc.m_hDC != NULL)
        theApp.GetSystemSettings( &dc );

    if (         m_statBar.m_hWnd    != NULL
    &&  theApp.m_fntStatus.m_hObject != NULL)
        m_statBar.SetFont( &theApp.m_fntStatus, FALSE );

    CFrameWnd::OnWinIniChange( lpszSection );

    if (pDoc = GetActiveDocument())
         {
         pDoc->UpdateAllViews( NULL );
         GetActiveView()->UpdateWindow();
         }
    }

/*****************************************************************************/

void CPBFrame::ActivateFrame( int nCmdShow )
{
        WINDOWPLACEMENT& wpSaved = theApp.m_wpPlacement;

        if (theApp.m_bPrintOnly)
        {
                nCmdShow = SW_HIDE;
        }
        else if (!IsWindowVisible())
        {
                switch (nCmdShow)
                {
                case SW_SHOW:
                case SW_SHOWNORMAL:
                        switch (wpSaved.showCmd)
                        {
                        case SW_HIDE:
                        case SW_MINIMIZE:
                        case SW_SHOWMINIMIZED:
                        case SW_SHOWMINNOACTIVE:
                                break;

                        default:
                                nCmdShow = wpSaved.showCmd;
                                break;
                        }
                        break;
                }

                wpSaved.showCmd = nCmdShow;

                wpSaved.length = sizeof(WINDOWPLACEMENT);
                SetWindowPlacement(&wpSaved);
        }
        //
        // We have to reassign the global toolbar pointers here, in case
        // they were pointing to the inplace frame's toolbars and that window
        // was deleted.
        //
        g_pStatBarWnd = &m_statBar;
        g_pImgToolWnd = &m_toolBar;
        g_pImgColorsWnd = &m_colorBar;
        CFrameWnd::ActivateFrame( nCmdShow );
}

/*****************************************************************************/
#ifdef xyzzyz
void CPBFrame::OnUpdateFrameTitle( BOOL bAddToTitle )
    {
    if (theApp.m_bEmbedded && ! theApp.m_bLinked)
        {
        CFrameWnd::OnUpdateFrameTitle( bAddToTitle );
        return;
        }

    // get old text for comparison against new text
    CString sOld;
    CString sText;

    GetWindowText( sOld );

    CPBDoc* pDocument = (CPBDoc*)GetActiveDocument();

    if (bAddToTitle && pDocument != NULL)
        {
        const TCHAR* psTitle = pDocument->GetTitle();

        if (psTitle != NULL)
            {
            sText += GetName( psTitle );
            sText += TEXT(" - ");

            sText.MakeLower();
            }
        }
    sText += m_strTitle;

    // set title if changed, but don't remove completely
    if (sText != sOld)
        SetWindowText( sText );
    }
#endif
/*****************************************************************************/

LRESULT CPBFrame::OnFileError( WPARAM, LPARAM )
    {
    theApp.FileErrorMessageBox();

    return 0;
    }

/***************************************************************************/

void CPBFrame::OnSysColorChange()
{
        CFrameWnd ::OnSysColorChange();

        ResetSysBrushes();
}

void CPBFrame::OnClose()
{
        SaveBarState(TEXT("General"));
        CFrameWnd ::OnClose();
}

