// MainFrm.cpp : implementation of the CMainFrame class
//
// CMainFrame implements most menu handling code, is registered in
// the clipboard viewer chain, provides OLE D&D support.
//

#include "stdafx.h"
#include "Server.h"
#include "NetClip.h"
#include "Doc.h"
#include "View.h"

#include "MainFrm.h"
#include "SvrDlg.h"
#ifdef _USE_OLEVIEWER
//#include "..\\ole2view\\iviewers\\iview.h"
#endif
#include "guids.h"
#include "DataObj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

// Because WM_DRAWCLIPBOARD is an Input Async message we cannot
// make calls on out-of-proc OLE interfaces during processing. To
// work around this we post a message to ourselves and do the
// appropriate processing on it. With the help of a semaphore
// (m_fRefreshPosted) this has the added benefit of eliminating
// multiple re-paints and clipboard accesses during D&D operations.
//
#define WM_REFRESH (WM_USER+1)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_COMMAND(ID_DISPLAY_AUTO, OnDisplayAuto)
	ON_UPDATE_COMMAND_UI(ID_DISPLAY_AUTO, OnUpdateDisplayAuto)
	ON_WM_INITMENUPOPUP()
	ON_WM_DRAWCLIPBOARD()
	ON_WM_CHANGECBCHAIN()
	ON_COMMAND(ID_EDIT_DELETE, OnEditClear)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditClear)
	ON_COMMAND(ID_CONNECT_CONNECT, OnConnectConnect)
	ON_UPDATE_COMMAND_UI(ID_CONNECT_CONNECT, OnUpdateConnectConnect)
	ON_COMMAND(ID_CONNECT_DISCONNECT, OnConnectDisconnect)
	ON_UPDATE_COMMAND_UI(ID_CONNECT_DISCONNECT, OnUpdateConnectDisconnect)
	ON_WM_INITMENU()
	ON_COMMAND(ID_EDIT_PASTELOCAL, OnEditPaste)
	ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
	//}}AFX_MSG_MAP
#ifdef _USE_OLEVIEWER
#ifdef _DEBUG
	ON_COMMAND(ID_VIEWDATAOBJECT, OnViewDataObject)
	ON_UPDATE_COMMAND_UI(ID_VIEWDATAOBJECT, OnUpdateViewDataObject)
#endif
#endif
    ON_COMMAND_RANGE(ID_DISPLAY_FIRST, ID_DISPLAY_LAST, OnOtherFormat)
	ON_UPDATE_COMMAND_UI_RANGE(ID_DISPLAY_FIRST, ID_DISPLAY_LAST, OnUpdateOtherFormat)
    ON_MESSAGE(WM_REFRESH, OnRefresh)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
//	ID_INDICATOR_CAPS,
//	ID_INDICATOR_NUM,
//	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_pNetClipView = NULL;
    m_fDisplayAsIcon = FALSE;	
    m_cfDisplay = 0;
    m_hwndNextCB = NULL;
    m_pClipboard = NULL;
    m_dwConnectionCookie = NULL;
    m_pConnectionPt = NULL;

    m_fRefreshPosted = FALSE;// prevent multiple refreshes.
}

CMainFrame::~CMainFrame()
{
    // Make sure we've released
    ASSERT(m_pConnectionPt == NULL);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return 0;

    // Register our d&d handler
    m_dropTarget.Register( this ) ;

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
    UINT    nResources = IDR_MAINFRAME;
    if (g_fDCOM == FALSE)
        nResources = IDR_NONETOLE;

	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(nResources))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY );

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	//m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	//EnableDocking(CBRS_ALIGN_ANY);
	//DockControlBar(&m_wndToolBar);

#ifdef _DEBUG
    // For debug we add a top leve menu item "View[Debug]..." that
    // pops up the Ole2View 2.0 UDT interface viewer.
    CMenu* pMenu = GetMenu();
    ASSERT(pMenu);
    pMenu->AppendMenu(MF_STRING , ID_VIEWDATAOBJECT, _T("[De&bugView...]"));
#endif

    //TRACE("m_hWnd=%08X\n", m_hWnd);

    m_hwndNextCB = SetClipboardViewer();

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return TRUE;

    // default create client will create a view if asked for it
	if (pContext != NULL && pContext->m_pNewViewClass != NULL)
	{
        pContext->m_pNewViewClass = RUNTIME_CLASS(CNetClipView) ;
		m_pNetClipView = (CNetClipView*)CreateView(pContext, AFX_IDW_PANE_FIRST);
        if (m_pNetClipView == NULL)
			return FALSE;
        ASSERT(m_pNetClipView->IsKindOf(RUNTIME_CLASS(CNetClipView)));
    }
    return TRUE;
}

LRESULT CMainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = CFrameWnd::OnSetMessageString(wParam, lParam);
	CWnd* pMessageBar = GetMessageBar();
    pMessageBar->InvalidateRect(NULL, TRUE);
    pMessageBar->UpdateWindow();
    return lr ;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
    // Turn off auto update of title bar
    dwDefaultStyle &= ~((DWORD)FWS_ADDTOTITLE) ;
    BOOL f = CFrameWnd::LoadFrame(nIDResource, dwDefaultStyle,
                pParentWnd, pContext);

    return f ;
}

void CMainFrame::OnDestroy()
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (!theApp.m_fServing)
    {
        m_dropTarget.Revoke( ) ;
        SavePosition(m_strMachine) ;
    }
    Disconnect();

    //if (m_hwndNextCB)
    {
        ASSERT(m_hwndNextCB!= m_hWnd);
        TRACE(_T("ChangeClipboardChain(m_hwndNextCB = %08X)..."), m_hwndNextCB);
        ChangeClipboardChain(m_hwndNextCB);
        TRACE(_T("ChangeClipboardChain\n"));
        m_hwndNextCB = NULL;
    }
    CFrameWnd::OnDestroy();
}

// Saves the window settings. Settings are saved per
// machinename, allowing the user to have separate windows in
// saved positions for each machine they are viewing.
//
// TODO: Save m_strMachineName.
//
BOOL CMainFrame::SavePosition(LPCTSTR szName)
{
    CString szSection("Default") ;
    CString szKey("WndPos") ;

    if (szName && *szName)
    {
        if (*szName == '\\') szName++;
        if (*szName == '\\') szName++;
        szSection = szName;
    }

    WINDOWPLACEMENT wp;
    CString szValue ;

    wp.length = sizeof( WINDOWPLACEMENT );
    GetWindowPlacement( &wp );

    LPTSTR p = szValue.GetBuffer( 255 ) ;
    wsprintf( p, _T("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d"),
        wp.showCmd, wp.ptMinPosition.x, wp.ptMinPosition.y,
        wp.ptMaxPosition.x, wp.ptMaxPosition.y,
        wp.rcNormalPosition.left, wp.rcNormalPosition.top,
        wp.rcNormalPosition.right, wp.rcNormalPosition.bottom,
        (m_wndToolBar.GetSafeHwnd() && (m_wndToolBar.GetStyle() & WS_VISIBLE)) ? TRUE : FALSE,
        (m_wndStatusBar.GetSafeHwnd() && (m_wndStatusBar.GetStyle() & WS_VISIBLE)) ? TRUE : FALSE);

    szValue.ReleaseBuffer() ;
    theApp.WriteProfileString( szSection, szKey, szValue );
    return TRUE ;
}

// Restores window settings
// TODO: Save m_strMachineName.
BOOL CMainFrame::RestorePosition(LPCTSTR szName, int nCmdShow)
{
    CString sz ;
    CString szSection("Default") ;
    CString szKey("WndPos") ;
    BOOL fToolBar = TRUE ;
    BOOL fStatusBar = TRUE ;

    if (szName && *szName)
    {
        if (*szName == '\\') szName++;
        if (*szName == '\\') szName++;
        szSection = szName ;
    }

    WINDOWPLACEMENT wp;
    int     nConv;

    wp.length = sizeof( WINDOWPLACEMENT );
    wp.flags = 0 ;

    try
    {
        sz = theApp.GetProfileString(szSection, szKey, _T("") ) ;
        if (sz.IsEmpty())
            AfxThrowMemoryException();

        LPTSTR   lp = (LPTSTR)sz.GetBuffer( 255 );

        wp.showCmd = (WORD)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.ptMinPosition.x = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.ptMinPosition.y = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.ptMaxPosition.x = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.ptMaxPosition.y = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.rcNormalPosition.left = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.rcNormalPosition.top = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.rcNormalPosition.right = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        wp.rcNormalPosition.bottom = (int)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        fToolBar = (BOOL)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        fStatusBar = (BOOL)ParseOffNumber( (LPTSTR FAR *)&lp, &nConv );
        if (!nConv)
            AfxThrowMemoryException();

        // Always strip off minimize.
        //
        if (wp.showCmd == SW_SHOWMINIMIZED)
            wp.showCmd = SW_SHOWNORMAL ;

        if (nCmdShow != SW_SHOWNORMAL || nCmdShow != SW_NORMAL)
            wp.showCmd = nCmdShow ;
    }
    catch(CException* e)
    {
        fToolBar = TRUE ;
        fStatusBar = TRUE ;
        ShowControlBar( &m_wndToolBar, fToolBar, TRUE ) ;
        ShowControlBar( &m_wndStatusBar, fStatusBar, TRUE ) ;
        ShowWindow( SW_SHOWNORMAL );
        e->Delete();
        return FALSE ;
    };

    ShowControlBar( &m_wndToolBar, fToolBar, TRUE ) ;
    ShowControlBar( &m_wndStatusBar, fStatusBar, TRUE ) ;
    return (BOOL)SetWindowPlacement( &wp ) ;
}

// Automatic selection of clipboard format. Selection is dependent
// on the Rich Edit control.
//
void CMainFrame::OnDisplayAuto()
{
    if (m_cfDisplay != 0)
        m_cfDisplay = 0;  // auto
    else
    {
        // menu item 2 is &Display
        CMenu* pMenu = GetMenu()->GetSubMenu(2);
        ASSERT(pMenu);
        if (pMenu)
        {
            m_cfDisplay = m_rgFormats[0];
            if (!m_pNetClipView->CanDisplay(m_cfDisplay))
                m_cfDisplay = 0;
        }
    }
    m_pNetClipView->OnUpdate(NULL, 0, NULL);
}

void CMainFrame::OnUpdateDisplayAuto(CCmdUI* pCmdUI)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
    {
        pCmdUI->Enable(FALSE);
        return;
    }

    pCmdUI->SetCheck(m_cfDisplay == 0 ? 1 : 0);
}

// The Windows API GetClipboardFormatName() only returns strings
// for formats registered through RegisterClipboardFormat(), not
// the predefined set.  This little function handles the predefined
// format names.
//
LPTSTR CMainFrame::GetNameOfClipboardFormat(CLIPFORMAT cf)
{
    static TCHAR sz[256] ;
    switch( cf )
    {
        case CF_UNICODETEXT: lstrcpy( sz, _T( "Unicode Text" ) ) ; break ;
        case CF_ENHMETAFILE: lstrcpy( sz, _T( "Enhanced Metafile" ) ) ; break ;
        case CF_TEXT: lstrcpy( sz, _T( "Text" ) ) ; break ;
        case CF_BITMAP: lstrcpy( sz, _T( "Bitmap" ) ) ; break ;
        case CF_METAFILEPICT: lstrcpy( sz, _T( "Metafile" ) ) ; break ;
        case CF_SYLK: lstrcpy( sz, _T( "SLYK" ) ) ; break ;
        case CF_DIF: lstrcpy( sz, _T( "DIF" ) ) ; break ;
        case CF_TIFF: lstrcpy( sz, _T( "TIFF" ) ) ; break ;
        case CF_OEMTEXT: lstrcpy( sz, _T( "OEM Text" ) ) ; break ;
        case CF_DIB: lstrcpy( sz, _T( "Device Independent Bitmap" ) ) ; break ;
        case CF_PALETTE: lstrcpy( sz, _T( "Palette" ) ) ; break ;
        case CF_PENDATA: lstrcpy( sz, _T( "Pen Data" ) ) ; break ;
        case CF_RIFF: lstrcpy( sz, _T( "RIFF" ) ) ; break ;
        case CF_WAVE: lstrcpy( sz, _T( "Wave Data" ) ) ; break ;
        default:
            {
#if 0
                // BUGBUG:
                if (m_pClipboard)
                {
                    WCHAR* pwsz = NULL;
                    HRESULT hr = m_pClipboard->GetClipboardFormatName(cf, &pwsz) ;
                    if (SUCCEEDED(hr))
                    {
                        if (hr == S_OK)
                        {
                            ASSERT(pwsz);
#ifdef _UNICODE
                            wcscpy(sz, pwsz);
#else
                            WideCharToMultiByte(CP_ACP, 0, pwsz, -1, sz, 256, NULL, NULL);
#endif
                            CoTaskMemFree(pwsz);
                        }
                        else
                        {
                            if (!GetClipboardFormatName( (UINT)cf, sz, 254 ))
                                *sz = '\0';
                        }
                    }
                    else
                        *sz = '\0';
                }
                else
#endif
                    if (!GetClipboardFormatName( (UINT)cf, sz, 254 ))
                        *sz = '\0';
            }
        break ;
    }

    return sz ;
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;

    if (bSysMenu)
		return;     // don't support system menu

	AfxLockTempMaps();  // prevent temp CMenu from being deleted too early

    // 2 is the index of the Display menu
    if (nIndex != 2)
    {
    	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
        AfxUnlockTempMaps();
        return;
    }

    // Starting at item 2 (0 == &Auto, 1 == separator)
    // delete all items
    while (pPopupMenu->DeleteMenu(2, MF_BYPOSITION))
        ;

    IDataObject* pdo = NULL;
    HRESULT hr;

    BeginWaitCursor();
    if (m_pClipboard)
        hr = m_pClipboard->GetClipboard(&pdo);
    else
        hr = OleGetClipboard(&pdo);

    if (SUCCEEDED(hr))
    {
        // Fill menu with all formats, enabling only those which
        // we know we can view (graying out the others).
        //
        FORMATETC       fetc;
        IEnumFORMATETC* penum=NULL;
        if (SUCCEEDED(hr = pdo->EnumFormatEtc(DATADIR_GET, &penum)))
        {
            ULONG   ulFetched;
            TCHAR   sz[256];
            UINT    uiFormat = ID_DISPLAY_FIRST;
            while (S_OK == penum->Next(1, &fetc, &ulFetched))
            {
                lstrcpy(sz, GetNameOfClipboardFormat(fetc.cfFormat));
                if (*sz)
                {
                    m_rgFormats[uiFormat-ID_DISPLAY_FIRST] = fetc.cfFormat;
                    pPopupMenu->AppendMenu(MF_STRING, uiFormat, sz);
                    uiFormat++;
                    ASSERT(uiFormat<ID_DISPLAY_LAST);
                }
            }
            penum->Release();
        }
        pdo->Release();

        // Must let default do it's thing
	    CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
    }
    EndWaitCursor();
    AfxUnlockTempMaps();
}

// OnOtherFormat is the handler for all items on the display
// menu below the 2nd one ("&Auto" and the separator).
//
// m_rgFormats is populated with the clipboard formats
// returned through IDataObject::EnumFormatEtc, in the
// order returned.
//
void CMainFrame::OnOtherFormat( UINT nID )
{
    if (nID >= ID_DISPLAY_FIRST && nID < ID_DISPLAY_LAST)
    {
        m_cfDisplay = m_rgFormats[nID-ID_DISPLAY_FIRST];
        m_pNetClipView->OnUpdate(NULL, 0, NULL);
    }
}

void CMainFrame::OnUpdateOtherFormat(CCmdUI* pCmdUI)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
    {
        pCmdUI->Enable(FALSE);
        return;
    }
	
    UINT nID = pCmdUI->m_nID;

    if (nID >= ID_DISPLAY_FIRST && nID < ID_DISPLAY_LAST)
    {
        // Do we know how to display this format?
        if (m_pNetClipView->CanDisplay(m_rgFormats[nID-ID_DISPLAY_FIRST]))
            pCmdUI->Enable(TRUE);
        else
        {
            pCmdUI->Enable(FALSE);
            if (m_cfDisplay == m_rgFormats[nID-ID_DISPLAY_FIRST])
                m_cfDisplay = 0;
        }

        // Check the active one.
        if (m_cfDisplay == m_rgFormats[nID-ID_DISPLAY_FIRST])
            pCmdUI->SetCheck(1);
        else
            pCmdUI->SetCheck(0);
    }
}

void CMainFrame::OnDrawClipboard()
{
    // If we're registered in the clipboard format viewer chain
    // then we must call the next viewer. m_hwndNextCB will
    // only be non-NULL if we're viewing the local clipboard
    //TRACE("In OnDrawClipboard (m_hwndNextCB = %08X)...", m_hwndNextCB);
    //ASSERT(m_hwndNextCB!= m_hWnd);
    if (m_hwndNextCB)
    {
        ::SendMessage(m_hwndNextCB, WM_DRAWCLIPBOARD, 0, 0);
        //TRACE("Sent WM_DRAWCLIPBOARD.\n");
    }
    else
    {
        //TRACE("Did not send WM_DRAWCLIPBOARD.\n");
    }

    // BUGBUG: The check for m_Serving is not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (!theApp.m_fServing && m_pClipboard == NULL && m_fRefreshPosted == FALSE && theApp.m_fNoUpdate==FALSE)
    {
        // WM_DRAWCLIPBOARD is an INPUTASYNC call.  OnUpdate may
        // try to call into an in-place active item, so we post ourselves
        // a message (WM_USER+1 == OnRefresh()).
        // m_pNetClipView->OnUpdate(NULL, 0, NULL);
        m_fRefreshPosted = TRUE;
        PostMessage(WM_REFRESH);
    }
}

// Because WM_DRAWCLIPBOARD is an Input Async message we cannot
// make calls on out-of-proc OLE interfaces during processing. To
// work around this we post a message to ourselves and do the
// appropriate processing on it. With the help of a semaphore
// (m_fRefreshPosted) this has the added benefit of eliminating
// multiple re-paints and clipboard accesses during D&D operations.
//
LRESULT CMainFrame::OnRefresh(WPARAM, LPARAM)
{
    BeginWaitCursor();
    m_fRefreshPosted=FALSE;
    m_pNetClipView->OnUpdate(NULL, 0, NULL);
    EndWaitCursor();
    return 0;
}

void CMainFrame::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
    // Pass on to next in chain.
    //ASSERT(!(hWndRemove == m_hWnd && hWndAfter == m_hWnd));
    //ASSERT(m_hwndNextCB!= m_hWnd);
    //TRACE("In OnChangeCbChain(%08X, %08X) (m_hwndNextCB = %08X)", hWndRemove, hWndAfter, m_hwndNextCB);
    if (m_hwndNextCB)
    {
        ::SendMessage(m_hwndNextCB, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndAfter);
        //TRACE("Sent WM_CHANGERCBCHAIN.\n");
    }
    else
    {
        //TRACE("Did not send WM_CHANGERCBCHAIN.\n");
    }
}


void CMainFrame::OnEditClear()
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;

    BeginWaitCursor();
    if (m_pClipboard)
        m_pClipboard->SetClipboard(NULL);
    else
        OleSetClipboard(NULL);

    m_pNetClipView->OnUpdate(NULL, 0, NULL);
    EndWaitCursor();
}

void CMainFrame::OnUpdateEditClear(CCmdUI* pCmdUI)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;

    pCmdUI->Enable(1);	
}

void CMainFrame::OnEditPaste() 
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;
    BeginWaitCursor();
    if (m_pClipboard)
    {
        IDataObject* pdo;
        SetMessageText(IDS_STATUS_GETLOCAL);
        HRESULT hr=OleGetClipboard(&pdo);
        if (SUCCEEDED(hr))
        {
            SetMessageText(IDS_STATUS_SETREMOTE);
            m_pClipboard->SetClipboard(pdo);
            pdo->Release();
        }
        SetMessageText(AFX_IDS_IDLEMESSAGE);
    }
    EndWaitCursor();
}

void CMainFrame::OnConnectConnect() 
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;

    CServerInfoDlg dlg(this);
    dlg.m_strMachine = m_strMachine;
    if (IDOK==dlg.DoModal())
    {
        m_strMachine = dlg.m_strMachine;
        if (SUCCEEDED(Connect(dlg.m_strMachine)))
        {
            SetWindowText(m_strMachine + _T(" - NetClip"));
            // Force an update of the display.
            m_pNetClipView->OnUpdate(NULL, 0, NULL);
        }
    }
}

void CMainFrame::OnUpdateConnectConnect(CCmdUI* pCmdUI)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;
    pCmdUI->Enable(m_pClipboard==NULL);
}

void CMainFrame::OnConnectDisconnect()
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;

    Disconnect();	
    SetWindowText(_T("Local Clipboard"));
    /*
    TRACE("SetClipBoardViewer...");
    m_hwndNextCB = SetClipboardViewer();
    TRACE("SetClipboardViewer returned m_hwndNextCB = %08X\n", m_hwndNextCB);
    //ASSERT(m_hwndNextCB!= m_hWnd);
    */
    m_pNetClipView->OnUpdate(NULL, 0, NULL);
}

void CMainFrame::OnUpdateConnectDisconnect(CCmdUI* pCmdUI)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return;
    pCmdUI->Enable(m_pClipboard!=NULL);
}

HRESULT CMainFrame::Connect(CString &strMachine)
{
    // BUGBUG: These checks are not really necessary because CMainFrame
    // is never created when we are run as a server (or service)
    //
    if (theApp.m_fServing)
        return E_FAIL;

    BeginWaitCursor();
    // If we're already connected for some reason just make
    // sure we disconnect first.
    //
    Disconnect();

    CString strMsg;
    strMsg.FormatMessage(IDS_STATUS_CONNECTING, strMachine);
    SetMessageText(strMsg);

    HRESULT hr = S_OK;
    MULTI_QI rgMQI[] = 
    { 
        //{*pIID, *pItf, hr}
        {&IID_IClipboard, NULL, E_FAIL}, 
        {&IID_IConnectionPointContainer, NULL, E_FAIL}
    };

    USES_CONVERSION;

    if (strMachine.CompareNoCase(_T("%inproc%")) != 0)
    {
        COSERVERINFO info;
	    memset(&info, 0, sizeof(info)); // MH 8/8/96: fix for NT 4.0 final
        info.pwszName = T2OLE(strMachine.GetBuffer(_MAX_PATH));
        info.pAuthInfo = NULL;
        hr = CoCreateInstanceEx(CNetClipServer::guid, NULL, CLSCTX_SERVER | CLSCTX_REMOTE_SERVER, &info, 
                                (sizeof(rgMQI)/sizeof(rgMQI[0])), rgMQI);
        strMachine.ReleaseBuffer();
    }
    else
    {
        CNetClipServer* p = new CNetClipServer;
        p->CreateEx(0, AfxRegisterWndClass(0), _T("Serving Clipboard"), WS_OVERLAPPEDWINDOW, 0,0,0,0, NULL, 0);
        rgMQI[0].pItf = (IClipboard*)p->GetInterface(&IID_IClipboard);
        rgMQI[0].hr = S_OK;

        ASSERT(rgMQI[0].pItf);
        if (rgMQI[0].pItf == NULL)
        {
            EndWaitCursor();
            ErrorMessage("Could not create instance of inproc server.", hr);
            SetMessageText(AFX_IDS_IDLEMESSAGE);
            return E_FAIL;
        }
        rgMQI[1].hr = rgMQI[0].pItf->QueryInterface(IID_IConnectionPointContainer, (void**)&rgMQI[1].pItf);
        hr = rgMQI[1].hr;
    }

    strMsg += _T(".");
    SetMessageText(strMsg);

    if (FAILED(hr))
    {
        EndWaitCursor();
        // Pop up error message
        CString str = _T("CoCreateInstanceEx failed.");
        switch (hr)
        {
            case REGDB_E_CLASSNOTREG:
            case CO_E_CREATEPROCESS_FAILURE:
            case CO_E_SERVER_EXEC_FAILURE:
                str = _T("The Remote Clipboard Viewer is not be installed correctly on the remote computer.");
            break;

            case CO_E_CANT_REMOTE:
                str = _T("Possible reason: Distributed COM is disabled.");
            break;

            case CO_E_BAD_SERVER_NAME:
                str = _T("The machine name is invalid.");
            break;

            case CO_E_WRONG_SERVER_IDENTITY:
            case CO_E_RUNAS_CREATEPROCESS_FAILURE:
            case CO_E_RUNAS_LOGON_FAILURE:
            case CO_E_LAUNCH_PERMSSION_DENIED:
            case E_ACCESSDENIED:
                str = _T("Permission denied. Make sure the same user is logged on to both computers.");
            break;

            case CO_E_REMOTE_COMMUNICATION_FAILURE:
                str = _T("This computer was unable to communicate with the remote computer.");
            break;

        }
        CString strError = _T("Could not connect to ") + strMachine + _T(". ") + str;
        ErrorMessage(strError,hr);
        SetMessageText(AFX_IDS_IDLEMESSAGE);
        return hr;
    }

    if (SUCCEEDED(rgMQI[0].hr) && rgMQI[0].pItf != NULL)
        m_pClipboard = (IClipboard*)rgMQI[0].pItf;
    else
    {
        // Pop up error message
        if (rgMQI[1].hr && rgMQI[1].pItf != NULL)
            rgMQI[1].pItf->Release();
        EndWaitCursor();
        ErrorMessage("Could not connect to remote server. QueryInterface for IClipboard failed.",hr);
        SetMessageText(AFX_IDS_IDLEMESSAGE);
        return rgMQI[0].hr ;
    }
    
    IConnectionPointContainer* pcpc=NULL;
    if (SUCCEEDED(rgMQI[1].hr) && rgMQI[1].pItf != NULL)
        pcpc = (IConnectionPointContainer*)rgMQI[1].pItf;
    else
    {
        // Pop up error message
        EndWaitCursor();
        if (rgMQI[0].hr && rgMQI[0].pItf != NULL)
            rgMQI[0].pItf->Release();
        ErrorMessage("Could not connect to remote server. QueryInterface for IConnectionPointContainer failed.",hr);
        SetMessageText(AFX_IDS_IDLEMESSAGE);
        return rgMQI[1].hr ;
    }

    ASSERT(m_pClipboard);
    ASSERT(pcpc);
    ASSERT(m_pConnectionPt == NULL);

    strMsg += _T(".");
    SetMessageText(strMsg);

    hr = pcpc->FindConnectionPoint(IID_IClipboardNotify, &m_pConnectionPt);
    if (FAILED(hr))
    {
        // Pop up error message
        EndWaitCursor();
        ErrorMessage("Find Connection Point failed.",hr);

        // Release and get out
        pcpc->Release();
        m_pClipboard->Release();
        m_pClipboard = NULL;
        SetMessageText(AFX_IDS_IDLEMESSAGE);
        return hr;
    }
    pcpc->Release();

    strMsg += _T(".");
    SetMessageText(strMsg);

    // Get our IUnknown
    //
    IUnknown* pClipboardNotify = (IUnknown*)GetInterface(&IID_IUnknown);
    ASSERT(pClipboardNotify);

#ifdef _TEST
    // Test to make sure we can actually get the clipboard
    IDataObject* pdo=NULL;
    hr = m_pClipboard->GetClipboard(&pdo);
    if (FAILED(hr))
    {
        EndWaitCursor();
        // Pop up error message
        CString str = _T("Could not get the remote clipboard.");
        switch (hr)
        {
            case E_ACCESSDENIED:
                str = _T("Access to remote clipboard denied. Make sure the same user is logged on to both computers.");
            break;
        }
        CString strError = _T("Could not connect to ") + strMachine + _T(". ") + str;
        ErrorMessage(strError,hr);

        // Release and get out
        m_pConnectionPt->Release();
        m_pConnectionPt = NULL;
        m_dwConnectionCookie = 0;
        m_pClipboard->Release();
        m_pClipboard = NULL;
        return hr;
    }
    else
        pdo->Release();
#endif

    strMsg += _T(".");
    SetMessageText(strMsg);

    // Call IConnectionPoint::Advise to setup the callbacks
    //
    hr = m_pConnectionPt->Advise(pClipboardNotify, &m_dwConnectionCookie);
    if (FAILED(hr))
    {
        EndWaitCursor();
        // Pop up error message
        CString str = _T("Connection point advise failed.");
        switch (hr)
        {
            case E_ACCESSDENIED:
                str = _T("Access to remote clipboard denied. Make sure the same user is logged on to both computers.");
            break;
        }
        CString strError = _T("Could not connect to ") + strMachine + _T(". ") + str;
        ErrorMessage(strError,hr);

        // Release and get out
        if (m_dwConnectionCookie != NULL)
            m_pConnectionPt->Unadvise(m_dwConnectionCookie);
        m_pConnectionPt->Release();
        m_pConnectionPt = NULL;
        m_dwConnectionCookie = 0;
        m_pClipboard->Release();
        m_pClipboard = NULL;
        SetMessageText(AFX_IDS_IDLEMESSAGE);
        return hr;
    }
    SetMessageText(AFX_IDS_IDLEMESSAGE);
    EndWaitCursor();
    return S_OK;
}

HRESULT CMainFrame::Disconnect()
{
    HRESULT hr = S_OK;
    BeginWaitCursor();
    if (m_pClipboard)
    {
        // We need to tell the remote guy to flush the clipboard.
        // Otherwise he'll still have a pointer to an object
        // we might have passed him.
        // TODO: We can be smarter about this and detect if
        // we've passed out any references to objects?  Set a
        // flag whereever we do m_pClipboard->SetClipboard()?
        //
        // if (m_fSetRemoteClipboard)
            m_pClipboard->FlushClipboard();

        if (m_dwConnectionCookie && m_pConnectionPt)
        {
            m_pConnectionPt->Unadvise(m_dwConnectionCookie);
            m_pConnectionPt->Release();
            m_pConnectionPt = NULL;
            m_dwConnectionCookie=0;
        }
        m_pClipboard->Release();
        m_pClipboard=NULL;
    }
    EndWaitCursor();
    return hr;
}

// We implement the notification interface on the main window
// for convenience.
//
BEGIN_INTERFACE_MAP(CMainFrame, CFrameWnd)
	INTERFACE_PART(CMainFrame, IID_IClipboardNotify, ClipboardNotify)
END_INTERFACE_MAP()

ULONG CMainFrame::XClipboardNotify::AddRef()
{
    METHOD_PROLOGUE(CMainFrame, ClipboardNotify)
    return pThis->ExternalAddRef();
}

ULONG CMainFrame::XClipboardNotify::Release()
{
    METHOD_PROLOGUE(CMainFrame, ClipboardNotify)
    return pThis->ExternalRelease();
}

HRESULT CMainFrame::XClipboardNotify::QueryInterface(
    REFIID iid, void** ppvObj)
{
    METHOD_PROLOGUE(CMainFrame, ClipboardNotify)
    return (HRESULT)pThis->ExternalQueryInterface(&iid, ppvObj);
}

HRESULT CMainFrame::XClipboardNotify::OnClipboardChanged()
{
    METHOD_PROLOGUE(CMainFrame, ClipboardNotify)
    pThis->OnRefresh(0,0);
    return S_OK;
}

#ifdef _USE_OLEVIEWER
#ifdef _DEBUG
// Pops up the Ole2View 2.0 Uniform Data Transfer viewer on
// the current clipboard.
//
void CMainFrame::OnViewDataObject()
{
    IInterfaceViewer* piv= NULL;
    HRESULT hr = CoCreateInstance(CLSID_IDataObjectViewer, NULL, CLSCTX_SERVER, IID_IInterfaceViewer, (void**)&piv);
    if (SUCCEEDED(hr))
    {
        IDataObject* pdo;
        if (m_pClipboard)
            hr= m_pClipboard->GetClipboard(&pdo);
        else
            hr=OleGetClipboard(&pdo);
        piv->View(AfxGetMainWnd()->GetSafeHwnd(), IID_IDataObject, pdo);
        pdo->Release();
        piv->Release();
    }
    else
        ErrorMessage("Could not load viewer. You must have Ole2View 2.0 installed.", hr);
}

void CMainFrame::OnUpdateViewDataObject(CCmdUI* pCmdUI)
{
}
#endif
#endif

//////////////////////////////////////////////////////////////////
// The main frame supports OLE D&D to/from the clipboard; CDropTarget
// implements our support.
//
CDropTarget::CDropTarget ()
{
}

CDropTarget::~CDropTarget()
{
}

DROPEFFECT CDropTarget::OnDragEnter(CWnd* , COleDataObject* , DWORD , CPoint )
{
    DROPEFFECT de = DROPEFFECT_COPY ;
    return de ;
}

DROPEFFECT CDropTarget::OnDragOver(CWnd*, COleDataObject* , DWORD dwKeyState, CPoint )
{
    DROPEFFECT de ;

    // Prevent d&d'ing on self
    CMainFrame*   pfrm = (CMainFrame*)CWnd::FromHandle(m_hWnd) ;
    CNetClipView* pview = (CNetClipView*)pfrm->GetActiveView();

    // TODO: We should check to make sure the dataobject passed in
    // has a format we can support.
    //

    // check for force link
    if ((dwKeyState & (MK_CONTROL|MK_SHIFT)) == (MK_CONTROL|MK_SHIFT))
        de = DROPEFFECT_LINK;
    // check for force copy
    else if ((dwKeyState & MK_CONTROL) == MK_CONTROL)
        de = DROPEFFECT_COPY;
    // check for force move
    else if ((dwKeyState & MK_ALT) == MK_ALT)
        de = DROPEFFECT_MOVE;
    // default -- recommended action is move
    else
        de = DROPEFFECT_MOVE;
    return de;
}

BOOL CDropTarget::OnDrop(CWnd* , COleDataObject* pDataObject, DROPEFFECT , CPoint )
{
    CMainFrame*  pfrm = (CMainFrame*)CWnd::FromHandle(m_hWnd) ;
    CNetClipDoc* pdoc = (CNetClipDoc*)pfrm->GetActiveView()->GetDocument();

    HRESULT hr ;

    // We must make a copy of the data object.  We do this because
    // the provider will have no way of notifying us when he shuts down
    // and we won't know to call OleFlushClipboard.  By making a copy
    // of the data object, *we* get to call OleFushClipboard when *we*
    // shutdown.
    //
    // CGenericDataObject is a simple IDataObject implementation that
    // will accept any data format for ::SetData, copying the
    // data and making it available via ::GetData. It's very similar
    // to COleDataSource, except that it works with ANY formatetc
    // and stgmedium.
    //
    // BUGBUG: This means that there is a difference between doing
    // a Edit.Copy in an application and draging onto the viewer. Edit.Copy
    // means delayed rendering, D&D means cached rendering. The only way
    // for this to be fixed is for OLE to provide a mechanism (unlikely).
    //
    CGenericDataObject* pGeneric = new CGenericDataObject(pDataObject->m_lpDataObject);
    ASSERT(pGeneric);
    if (pGeneric==NULL)
        return FALSE;

    IDataObject* pdo = (IDataObject*)pGeneric->GetInterface(&IID_IDataObject);
    ASSERT(pdo);

    if (pfrm->m_pClipboard)
        hr = pfrm->m_pClipboard->SetClipboard(pdo);
    else
        hr = OleSetClipboard(pdo);

    // If OleSetClipboard succeeded, it AddRef'd pdo.  If it failed, it didn't.
    // In either case, we release
    //
    pdo->Release();

    return SUCCEEDED(hr);
}

