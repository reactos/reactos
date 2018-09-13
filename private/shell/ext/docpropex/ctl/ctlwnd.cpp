//-------------------------------------------------------------------------//
// CtlWnd.cpp : Implementation of CPropertyTreeCtl ActiveX control window
//-------------------------------------------------------------------------//

#include "pch.h"
#include "PropTree.h"

#include "TreeItems.h"
#include "Ctl.h"

//-------------------------------------------------------------------------//
//  Common control registration flags.
const DWORD dwICC    = ICC_COOL_CLASSES|ICC_TREEVIEW_CLASSES|ICC_LISTVIEW_CLASSES|
                       ICC_DATE_CLASSES|ICC_BAR_CLASSES ;

enum RBBAND_ID
{
    RBBID_TOOL,
    RBBID_QTIP
} ;

//-------------------------------------------------------------------------//
CMetrics::CMetrics( CWindow* pHostCtl )
    :    m_pHostCtl( pHostCtl ),
         m_cxItemIndent(0),
         m_cxBestFitDividerPos(0),
         m_cxDividerPos(0),
         m_cxEllipsis(0),
         m_ptsizeHeader(8),
         m_nBorderWidth(0),
         m_cxMinColWidth(5),
         m_cxImageCellWidth(16),
         m_cxImageMargin(3),
         m_cxTextMargin(2),
         m_cxDividerMargin(2),
         m_cyComboEditBox(0),
         m_cyComboDropList(150),
         m_nEmptyCaptionMargin(10),
         m_hilTree(NULL),
         m_hilHdr(NULL),
         m_hrgnExclude(NULL),
         m_hfHdr(NULL),
         m_hfFolderItem(NULL),
         m_hfDirty(NULL),
         m_hbrEmptyCaption(NULL)
{
}

//-------------------------------------------------------------------------//
HFONT CMetrics::CreateHeaderFont( HFONT hfTree )
{
    HDC          hdc ;
    HFONT        hf ;
    LOGFONT      lf ;           

    //  Try creating the font based on the tree view's font.
    if( GetObject( hfTree, sizeof(lf), &lf ) > 0 &&
        (hf = CreateFontIndirect( &lf ))!=NULL )
        return hf ;
    else if( ( hdc = GetDC( HWND_DESKTOP ))!=NULL )
    {
        //  Setting the tree view's font failed;
        //  fabricate a default font.
        hf = CreateFont( MulDiv( HeaderPtSize(), GetDeviceCaps( hdc, LOGPIXELSY ), 72 ),
                         0, 0, 0, 
                         FW_NORMAL, 
                         FALSE, FALSE, FALSE, 
                         ANSI_CHARSET, 
                         OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, 
                         DEFAULT_QUALITY,
                         DEFAULT_PITCH | (FF_SWISS << 4), 
                         TEXT("MS Sans Serif") ) ;
        
        ReleaseDC( HWND_DESKTOP, hdc ) ;

        //  Stash the font handle until the header control is destroyed.
        if( m_hfHdr ) DeleteObject( m_hfHdr ) ;
        return (m_hfHdr = hf) ;
    }

    return NULL ;
}

//-------------------------------------------------------------------------//
//  Creates a folder item derivative of the tree 
//  view's default font.  
HFONT CMetrics::CreateFolderItemFont( HFONT hfTree )
{
    BOOL    bRet = FALSE ;
    LOGFONT lf ;
    HFONT   hf ;

    if( GetObject( hfTree, sizeof(lf), &lf )>0 )
    {
        //lf.lfWeight     = FW_BOLD ;
        lf.lfUnderline  = TRUE ;

        if( (hf = CreateFontIndirect( &lf ))!=NULL )
        {
            if( m_hfFolderItem )
                DeleteObject( m_hfFolderItem ) ;
            m_hfFolderItem = hf ;
        }
    }
    return m_hfFolderItem ;
}

//-------------------------------------------------------------------------//
//  Creates a folder item derivative of the tree 
//  view's default font.  
HFONT CMetrics::CreateDirtyItemFont( HFONT hfTree )
{
    BOOL    bRet = FALSE ;
    LOGFONT lf ;
    HFONT   hf ;

    if( GetObject( hfTree, sizeof(lf), &lf )>0 )
    {
        lf.lfWeight     = FW_BOLD ;

        if( (hf = CreateFontIndirect( &lf ))!=NULL )
        {
            if( m_hfDirty )
                DeleteObject( m_hfDirty ) ;
            m_hfDirty = hf ;
        }
    }
    return m_hfDirty ;
}

//-------------------------------------------------------------------------//
HBRUSH CMetrics::EmptyCaptionBrush()
{
    if( m_hbrEmptyCaption == NULL )
        m_hbrEmptyCaption = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) ) ;

    return m_hbrEmptyCaption ;
}

//-------------------------------------------------------------------------//
//  We need to create an exclusion region to prevent the default SysTreeView32
//  window proc from drawing over our custom drawing, which it will freely
//  do under some scenarios, in some regions of the control.  This helper
//  member does the cleanup and GDI drudgery.
HRGN CMetrics::CreateItemExclusionRegion( LPCRECT prc )
{
    if( m_hrgnExclude )
    {
        DeleteObject( m_hrgnExclude ) ;
        m_hrgnExclude = NULL ;
    }
    if( prc )
        m_hrgnExclude = CreateRectRgnIndirect( prc ) ;
    return m_hrgnExclude ;
}

//-------------------------------------------------------------------------//
HIMAGELIST CMetrics::LoadTreeImageList()
{
    ASSERT( m_hilTree==NULL ) ;
    
    return (m_hilTree = ImageList_LoadImage( 
                _Module.m_hInstResource, 
                MAKEINTRESOURCE( IDB_TREE_IMAGELIST ),
                ImageCellWidth(), 0, RGB(255,0,255), 
                IMAGE_BITMAP, LR_SHARED )) ;
}

//-------------------------------------------------------------------------//
HIMAGELIST CMetrics::LoadHdrImageList()
{
    ASSERT( m_hilHdr==NULL ) ;
    
    return (m_hilHdr = ImageList_LoadImage( 
                _Module.m_hInstResource, 
                MAKEINTRESOURCE( IDB_TREEHDR_IMAGELIST ),
                10, 0, RGB(192,192,192), 
                IMAGE_BITMAP, LR_SHARED )) ;
}

//-------------------------------------------------------------------------//
void CMetrics::DestroyResources()
{
    if( m_hfHdr )
    {
        DeleteObject( m_hfHdr ) ;
        m_hfHdr = NULL ;
    }

    if( m_hfFolderItem )
    {
        DeleteObject( m_hfFolderItem ) ;
        m_hfFolderItem = NULL ;
    }

    if( m_hfDirty )
    {
        DeleteObject( m_hfDirty ) ;
        m_hfDirty = NULL ;
    }

    if( m_hbrEmptyCaption )
    {
        DeleteObject( m_hbrEmptyCaption ) ;
        m_hbrEmptyCaption = NULL ;
    }

    if( m_hilTree )
    {
        ImageList_Destroy( m_hilTree ) ;
        m_hilTree = NULL ;
    }

    if( m_hilHdr )
    {
        ImageList_Destroy( m_hilHdr ) ;
        m_hilHdr = NULL ;
    }

    CreateItemExclusionRegion( NULL ) ;
}

//-------------------------------------------------------------------------//
//  Warning: don't call any virtual members from the constructor, since
//  ATL has implemented compiler optimization on this class such that no
//  v-table exists when the constructor is invoked; call virtuals from
//  FinalConstruct() instead.
CPropertyTreeCtl::CPropertyTreeCtl()
    :   m_metrics( this ),
        m_wndTree(     WC_TREEVIEW,      this, TREE_ALTMSGMAP ),
        m_wndHdr(      WC_HEADER,        this, HDR_ALTMSGMAP ),
#ifdef _PROPERTYTREE_TOOLBAR
        m_wndRebarTop( REBARCLASSNAME,   this, REBARTOP_ALTMSGMAP ),
        m_wndTool(     TOOLBARCLASSNAME, this, TOOL_ALTMSGMAP ),
#endif _PROPERTYTREE_TOOLBAR
        m_bInfobarVisible( FALSE ),
        m_wndInfobar( REBARCLASSNAME,   this, REBARBTM_ALTMSGMAP ),
        m_wndQtip(     TEXT("Edit"),     this, QTIP_ALTMSGMAP ),
        m_hwndEmpty(NULL),
        m_hwndToolTip(NULL),
        m_hToolTipTarget(NULL),
        m_pDefaultServer(NULL),
        m_clsidServer( CLSID_NULL ),
        m_bstrNoSourcesCaption(NULL),
        m_bstrNoPropertiesCaption(NULL),
        m_bstrNoCommonsCaption(NULL),
        m_nEmptyStatus(EMPTY_NOSOURCES),
        m_bDestroyed(FALSE)
{
	memset( &m_iChildSortDir, 0, sizeof(m_iChildSortDir) ) ;
    m_bWindowOnly = TRUE; 
}

//-------------------------------------------------------------------------//
//  Ensure aggregates are properly constructed, and to call
//  any virtual member functions involved in object construction.
HRESULT CPropertyTreeCtl::FinalConstruct()
{
    USES_CONVERSION ;
    TCHAR  szText[128] ;

    if( LoadString( _Module.GetModuleInstance(), IDS_NOSOURCES_CAPTION,
                    szText, sizeof(szText) ) )
        put_NoSourcesCaption( SysAllocString( T2W(szText) ) ) ;

    if( LoadString( _Module.GetModuleInstance(), IDS_NOPROPERTIES_CAPTION,
                    szText, sizeof(szText) ) )
        put_NoPropertiesCaption( SysAllocString( T2W(szText) ) ) ;

    if( LoadString( _Module.GetModuleInstance(), IDS_NOCOMMONS_CAPTION,
                    szText, sizeof(szText) ) )
        put_NoCommonsCaption( SysAllocString( T2W(szText) ) ) ;

    return S_OK;
}

//-------------------------------------------------------------------------//
//  Ensures aggregates are properly destroyed.
void CPropertyTreeCtl::FinalRelease()
{
    //  Remove all property sources
    RemoveAllSources( 0 ) ; 

    //  Clear string allocations
    put_NoSourcesCaption( NULL ) ;
    put_NoPropertiesCaption( NULL ) ;
    put_NoCommonsCaption( NULL ) ;

    //  Release server(s)
    if( m_pDefaultServer ) 
        m_pDefaultServer->Release() ;

    //  BUGBUG:
    //  Need to unregister window class; when hosted by successive rooted
    //  explorers, failure to unregister prevents registration by
    //  next explorer process, because of ATL2.1 bug
    UnregisterClass( GetWndClassInfo().m_wc.lpszClassName, 
                     GetWndClassInfo().m_wc.hInstance ) ;
    GetWndClassInfo().m_atom = 0 ;
}

//-------------------------------------------------------------------------//
//  Provides a window class for the control.  ATL will register 
//  this class prior to window creation.
CWndClassInfo& CPropertyTreeCtl::GetWndClassInfo()
{
    static CWndClassInfo wc =
    {
        { sizeof(WNDCLASSEX), 0, StartWindowProc,
          0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, TEXT("PropertyTree32"), 0 },
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
    };
    return wc;
}

//-------------------------------------------------------------------------//
//  Creates and initializes child windows and loads resources.associated with
//  the control's window.
LRESULT CPropertyTreeCtl::OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    INITCOMMONCONTROLSEX iccx = { sizeof(INITCOMMONCONTROLSEX) , dwICC } ;
    LPCREATESTRUCT       lpCS = (LPCREATESTRUCT)lParam ;
    RECT                 rc ; 
    HFONT                hfTree, hfHdr ;
    TCHAR                szCaption[128];

    if( !InitCommonControlsEx( &iccx ) )
        return -1 ;

    ModifyStyle( 0, WS_CLIPCHILDREN ) ;
    ModifyStyleEx( 0, WS_EX_CLIENTEDGE ) ;

    //  Load image list bitmaps.
    HIMAGELIST hILTree, hILHdr ;

    if( (hILTree = m_metrics.LoadTreeImageList())==NULL )
        return -1 ;
    if( (hILHdr = m_metrics.LoadHdrImageList())==NULL )
        return -1 ;

    m_bDestroyed = FALSE ;
    memset( &rc, 0, sizeof(rc) ) ;

    *szCaption = 0;
    LoadString( HINST_THISDLL, IDS_PROPTREE_CAPTION, szCaption, ARRAYSIZE(szCaption) );

    
    //  Create a standard header control
    if( m_wndHdr.Create( m_hWnd, rc, NULL,
                         WS_VISIBLE|WS_CHILD|HDS_BUTTONS|HDS_HORZ|HDS_FULLDRAG, 
                         0, IDC_HDR )==NULL )
        return -1 ;

    //  Create a standard tree view control
    if( m_wndTree.Create( m_hWnd, rc, szCaption,
                          WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|TVS_SHOWSELALWAYS|
                          TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT,
                          0, IDC_TREE )==NULL )
        return -1 ;

    //  Initialize windows, state
    //  The order is important especially since fonts must be assigned
    //  to child windows before metrics are calculated.
    hfTree = m_wndTree.GetFont() ;
    hfHdr  = m_metrics.CreateHeaderFont( hfTree ) ;
    m_wndHdr.SendMessage( WM_SETFONT, (WPARAM)hfHdr, 0L ) ;
    m_metrics.CreateFolderItemFont( hfTree ) ;
    m_metrics.CreateDirtyItemFont( hfTree ) ;

    if( CreateEmptyPrompt( hfHdr )==NULL )
        return -1 ;

#ifdef _PROPERTYTREE_TOOLBAR
    //  Create the toolbar control
    if( CreateToolbar( hfHdr )==NULL )
        return -1 ;
#endif _PROPERTYTREE_TOOLBAR

    if( m_bInfobarVisible )
    {
        //  Create the quick-tip control
        if( CreateQtip( hfHdr )==NULL )
            return -1 ;
    }
    else
    {
        if( CreateToolTips( m_wndTree, hfHdr )==NULL )
            return -1 ;
    }
    
    Header_SetImageList( m_wndHdr, hILHdr ) ;
    AddHeaderColumns( lpCS->cx ) ;
    PositionControls( lpCS->cx, lpCS->cy ) ;

    TreeView_SetImageList( m_wndTree, m_metrics.TreeImageList(), TVSIL_NORMAL ) ;
    m_metrics.SetItemIndent( TreeView_GetIndent( m_wndTree ) ) ;

    GetBestFitHdrWidth() ;
    UpdateEmptyStatus( TRUE ) ;

    bHandled = TRUE ;
    return 0 ;
}

//-------------------------------------------------------------------------//
//  Frees resources associated with the control's window
LRESULT CPropertyTreeCtl::OnDestroy( UINT, WPARAM, LPARAM, BOOL& )
{
    //TRACE( TEXT("CPropertyTreeCtl received WM_DESTROY\n") ) ;
    
    if( m_hwndToolTip )
    {
        ::DestroyWindow( m_hwndToolTip ) ;
        m_hwndToolTip = NULL;
    }
    
    Header_SetImageList( m_wndHdr, NULL ) ;
    TreeView_SetImageList( m_wndTree, NULL, TVSIL_NORMAL ) ;

    m_bDestroyed = TRUE ;
    m_metrics.DestroyResources() ;
    
    return 0L ;
}

//-------------------------------------------------------------------------//
//  WM_NOTIFYFORMAT handler. If we don't respond with the correct WM_NOTIFY 
//  format (ansi vs unicode), USER will decide for us without regard to 
//  what we're prepared to deal with.  By default, USER looks at what was
//  defined for the dialog resource template of the control, 
//  but of course we're not using a template.
LRESULT CPropertyTreeCtl::OnNotifyFormat( UINT, WPARAM, LPARAM lParam, BOOL& ) 
{
#if defined(_UNICODE) || defined(UNICODE)
    return NFR_UNICODE ;
#else
    return NFR_ANSI ;
#endif
}

//-------------------------------------------------------------------------//
//  WM_GETDLGCODE message handler for both PropertyTree32 and tree view subwindow.  
//  We want it all.
LRESULT CPropertyTreeCtl::OnGetDlgCode( UINT, WPARAM, LPARAM lParam, BOOL& bHandled )
{
    if( 0L != lParam && 
        WM_KEYDOWN == ((LPMSG)lParam)->message && VK_TAB == ((LPMSG)lParam)->wParam )
    {
        bHandled = FALSE ;
        return 0L ;
    }
    
    return DLGC_WANTALLKEYS ;
}

//-------------------------------------------------------------------------//
//  BUGBUG: Posted window invalidation message: 
//  necessary because of failure of NM_CUSTDRAW in tree control to properly
//  erase background after add, delete tree items.
void CPropertyTreeCtl::PendRedraw( HWND hwnd, UINT nFlags )
{
    ::PostMessage( m_hWnd, WMU_REDRAW, (WPARAM)nFlags, (LPARAM)hwnd ) ;
}

LRESULT CPropertyTreeCtl::OnRedraw( UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = TRUE ;
    HWND     hwnd = lParam ? (HWND)lParam : m_hWnd ;

    ::RedrawWindow( hwnd, NULL, NULL, (UINT)wParam ) ;
    return 0L ;
}

#ifdef _PROPERTYTREE_TOOLBAR
//-------------------------------------------------------------------------//
//  Creates the toolbar child window
HWND CPropertyTreeCtl::CreateToolbar( HFONT hfTool )
{
    HWND        hwndTool = NULL ;
    const int   cButtons = 2 ;
    TBBUTTON    tbb[cButtons];
    DWORD       dwBtnSize ;
    memset( tbb, 0, sizeof(tbb) ) ;

    // Assign button commands
    tbb[1].idCommand = ID_APPLY ;
    tbb[2].idCommand = ID_CUSTOMIZE ;
    
    // Fill the TBBUTTON array with button information, and add the 
    // buttons to the toolbar.
    for( int i=0; i<sizeof(tbb)/sizeof(TBBUTTON); i++ )
    {
        tbb[i].iString   = 
        tbb[i].iBitmap   = i ; 
        tbb[i].fsStyle   = TBSTYLE_BUTTON|(i==0 ? TBSTYLE_GROUP : 0) ;
        tbb[i].fsState   = TBSTATE_ENABLED ; 
    }

    if( (hwndTool = CreateToolbarEx( m_hWnd, WS_VISIBLE|WS_CHILD|
                            WS_CLIPCHILDREN|WS_CLIPSIBLINGS|
                            CCS_NORESIZE|CCS_NODIVIDER|CCS_NOPARENTALIGN|
                            TBSTYLE_TOOLTIPS|TBSTYLE_TRANSPARENT|TBSTYLE_FLAT,
                            IDC_TOOL, cButtons,
                            _Module.GetResourceInstance(), IDR_TOOLBAR,
                            tbb, cButtons, 27, 16, 54, 16, sizeof(TBBUTTON) ))!= NULL )
    {
        if( !m_wndTool.SubclassWindow( hwndTool ) )
        {
            ::DestroyWindow( hwndTool ) ;
            return NULL ;
        }
        m_wndTool.ModifyStyleEx( 0, WS_EX_TOOLWINDOW ) ;
        if( hfTool ) m_wndTool.SendMessage( WM_SETFONT, (WPARAM)hfTool, TRUE ) ;
        dwBtnSize = ::SendMessage( hwndTool, TB_GETBUTTONSIZE, 0, 0 ) ;
    }

    //  Create a rebar to house the toolbar (and whatever we come up with later).
    if( CreateRebar( m_wndRebarTop, IDC_REBARTOP, CCS_TOP )==NULL )
    {
        m_wndTool.DestroyWindow() ;
        return NULL ;
    }

    REBARBANDINFO   bandInfo ;
    memset( &bandInfo, 0, sizeof(bandInfo) ) ;
    bandInfo.cbSize = sizeof(bandInfo) ;
    bandInfo.fMask  = RBBIM_ID|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE ;
    bandInfo.fStyle = RBBS_GRIPPERALWAYS|RBBS_CHILDEDGE|RBBIM_HEADERSIZE ;

    bandInfo.hwndChild  = hwndTool ;
    bandInfo.cxMinChild = LOWORD( dwBtnSize ) * 2 ;
    bandInfo.cyMinChild = HIWORD( dwBtnSize ) ;
    bandInfo.cx         = LOWORD( dwBtnSize ) * 2 ;
    bandInfo.wID        = RBBID_TOOL ;

    m_wndRebarTop.SendMessage( RB_INSERTBAND, (WPARAM)-1, (LPARAM)&bandInfo ) ;

    return (HWND)m_wndTool ;
}
#endif _PROPERTYTREE_TOOLBAR

//-------------------------------------------------------------------------//
HWND CPropertyTreeCtl::CreateQtip( HFONT hfQtip )
{
    RECT rc ;
    HDC  hdc ;
    memset( &rc, 0, sizeof(rc) ) ;

    //  Calculate height sufficient to accommodate two lines of text.
    if( (hdc = ::GetDC( HWND_DESKTOP ))!=NULL )
    {
        static LPCTSTR pszText = TEXT("|\r\n|") ;
        HFONT          hfOld = (HFONT)SelectObject( hdc, hfQtip ) ;
        
        DrawText( hdc, pszText, lstrlen( pszText ), &rc, 
                  DT_CALCRECT|DT_LEFT|DT_NOCLIP|DT_EDITCONTROL ) ;
        
        SelectObject( hdc, hfOld ) ;
        ::ReleaseDC( HWND_DESKTOP, hdc ) ;

    }
    rc.right = 200 ;

    if( !m_wndQtip.Create( m_hWnd, rc, NULL,
                          WS_VISIBLE|WS_CHILD|WS_VSCROLL|
                          ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY,
                          0, IDC_QTIP ) )
        return NULL ;
    
    //  Create a rebar to house the toolbar (and whatever we come up with later).
    if( CreateRebar( m_wndInfobar, IDC_REBARBTM, CCS_BOTTOM )==NULL )
    {
        m_wndQtip.DestroyWindow() ;
        return NULL ;
    }
    if( hfQtip ) m_wndQtip.SendMessage( WM_SETFONT, (WPARAM)hfQtip, TRUE ) ;


    REBARBANDINFO   bandInfo ;
    memset( &bandInfo, 0, sizeof(bandInfo) ) ;
    bandInfo.cbSize = sizeof(bandInfo) ;
    bandInfo.fMask  = RBBIM_ID|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE ;
    bandInfo.fStyle = RBBS_GRIPPERALWAYS|RBBS_CHILDEDGE ;

    RECT rcQtip ;
    m_wndQtip.GetClientRect( &rcQtip ) ;
    bandInfo.hwndChild  = m_wndQtip ;
    bandInfo.cxMinChild = 0 ;
    bandInfo.cyMinChild = rcQtip.bottom-rcQtip.top ;
    bandInfo.cx         = rcQtip.right-rcQtip.left ;
    bandInfo.wID        = RBBID_QTIP ;

    m_wndInfobar.SendMessage( RB_INSERTBAND, (WPARAM)-1, (LPARAM)&bandInfo ) ;

    return (HWND)m_wndQtip ;
}

//-------------------------------------------------------------------------//
HWND CPropertyTreeCtl::CreateRebar( CContainedWindow& wndRebar, UINT nID, DWORD dwAlignStyle )
{
    RECT            rc ;
    REBARINFO       barInfo ;
    
    memset( &rc, 0, sizeof(rc) ) ;
    memset( &barInfo, 0, sizeof(barInfo) ) ;

    if( !wndRebar.Create( m_hWnd, rc, NULL, 
                           WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_BORDER|
                           RBS_DBLCLKTOGGLE|RBS_VARHEIGHT|RBS_BANDBORDERS|
                           CCS_NODIVIDER|dwAlignStyle,
                           WS_EX_DLGMODALFRAME|WS_EX_LEFT|WS_EX_RIGHTSCROLLBAR|WS_EX_TOOLWINDOW,
                           nID ) )
        return NULL ;

    //  Handshake w/ rebar
    barInfo.cbSize = sizeof(barInfo) ;
    if( !wndRebar.SendMessage( RB_SETBARINFO, 0, (LPARAM)&barInfo ) )
    {
        wndRebar.DestroyWindow() ;
        return NULL ;
    }

    return (HWND)wndRebar ;
}

//-------------------------------------------------------------------------//
HWND CPropertyTreeCtl::CreateToolTips( CContainedWindow& wndTree, HFONT hf )
{
    DWORD exStyle = 0L ; //WS_EX_MW_UNMANAGED_WINDOW ;

    HWND hwndToolTip = CreateWindowEx( 
                          exStyle, TOOLTIPS_CLASS, NULL,
                          WS_POPUP | TTS_NOPREFIX,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          wndTree, NULL, _Module.GetModuleInstance(),
                          NULL );
    if (hwndToolTip)
    {
        TOOLINFO ti;

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT;
        ti.hwnd = wndTree.m_hWnd ;
        ti.uId = (UINT_PTR)wndTree.m_hWnd ; 
        ti.lpszText = LPSTR_TEXTCALLBACK ;
        ti.lParam = 0 ;
        ::SendMessage( hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti ) ;

        ::SendMessage( hwndToolTip, WM_SETFONT, (WPARAM)hf, (LPARAM)TRUE ) ;
#if 0
        ::SendMessage( hwndToolTip, TTM_SETTIPBKCOLOR, (WPARAM)GetSysColor(COLOR_WINDOW), 0 ) ;
        ::SendMessage( hwndToolTip, TTM_SETTIPTEXTCOLOR, (WPARAM)GetSysColor(COLOR_WINDOWTEXT),0 ) ;
#endif
        ::SendMessage( hwndToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM)500 ) ;
        ::SendMessage( hwndToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)3000 ) ;

        if( m_hwndToolTip )
            ::DestroyWindow( m_hwndToolTip ) ;

        m_hwndToolTip = hwndToolTip ;
        return m_hwndToolTip ;
    }

    return NULL ;
}

//-------------------------------------------------------------------------//
HWND CPropertyTreeCtl::CreateEmptyPrompt( HFONT hFont )
{
    RECT  rc ;
    SetRect( &rc, 20, 40, 200, 60 ) ;

    m_hwndEmpty = CreateWindow( TEXT("Static"), NULL, WS_CHILD|WS_VISIBLE, 0,0,0,0,
                                m_wndTree, (HMENU)1, _Module.GetModuleInstance(), NULL ) ;

    if( m_hwndEmpty )
        ::SendMessage( m_hwndEmpty, WM_SETFONT, (WPARAM)hFont, 0L ) ;
    return m_hwndEmpty ;
}

//-------------------------------------------------------------------------//
SHORT CPropertyTreeCtl::CalcEmptyStatus( BSTR *pbstrCaption )
{
    BSTR  bstrCaption = NULL ;
    SHORT nEmptyStatus = 0 ;

    if( TreeView_GetCount( m_wndTree ) != 0 )
    {
        nEmptyStatus = EMPTY_NOTEMPTY ;
    }
    else if( m_srcs.Count()==0 )
    {
        nEmptyStatus = EMPTY_NOSOURCES ;
        bstrCaption = m_bstrNoSourcesCaption ;
    }
    else if( m_folders.Count()==0 && m_properties.Count()==0 )
    {
        nEmptyStatus = EMPTY_NOPROPERTIES ;
        bstrCaption  = m_bstrNoPropertiesCaption ;
    }                         
    else
    {
        nEmptyStatus = EMPTY_NOCOMMONS ;
        bstrCaption  = m_bstrNoCommonsCaption ;
    }

    if( pbstrCaption )
        *pbstrCaption = bstrCaption;

    return nEmptyStatus;
}
           
//-------------------------------------------------------------------------//
void CPropertyTreeCtl::UpdateEmptyStatus( BOOL bInit )
{
    BSTR  bstrCaption = NULL ;
    SHORT nEmptyStatus = CalcEmptyStatus( &bstrCaption );

    if( IsWindow( m_hwndEmpty ) && 
        (m_nEmptyStatus != nEmptyStatus || bInit ) )
    {
        m_nEmptyStatus = nEmptyStatus ;

        if( m_nEmptyStatus == EMPTY_NOTEMPTY )
            ::ShowWindow( m_hwndEmpty, SW_HIDE ) ;    
        else
        {
            USES_CONVERSION ;
            ASSERT( bstrCaption ) ;
            ::SetWindowText( m_hwndEmpty, W2T( bstrCaption ) ) ;
            ::ShowWindow( m_hwndEmpty, SW_SHOW ) ;
            Fire_Emptied( m_nEmptyStatus ) ;
        }
    }
}

//-------------------------------------------------------------------------//
//  Initializes property tree header control
BOOL CPropertyTreeCtl::AddHeaderColumns( int cx )
{
    HD_ITEM hdi ;
    memset( &hdi, 0, sizeof(hdi) ) ;
    TCHAR   szText[48] ;

    hdi.mask    = HDI_TEXT|HDI_FORMAT|HDI_WIDTH|HDI_IMAGE|HDI_LPARAM  ;
    hdi.pszText = szText ;
    hdi.fmt     = HDF_LEFT | HDF_BITMAP_ON_RIGHT ;

    //  Note: HDN_GETDISPINFO chokes on image callback w/ some 
    //  older vers of ComCtl32.dll so we've got to set the image index explicitly.

    //  Column 0
    *szText = 0 ;
    if( !LoadString( _Module.GetResourceInstance(), IDS_PROPERTY_HEADER_ITEM,
                    szText, sizeof(szText)/sizeof(TCHAR) ) )
    {
        ASSERT( FALSE ) ;   // missing string resource
    }
    hdi.cxy     = m_metrics.SetDividerPos( 135 ) ;
    hdi.lParam  = 0 ; // iItem (col#)
    hdi.iImage  = m_iChildSortDir[hdi.lParam] + 1 ;
    Header_InsertItem( m_wndHdr, hdi.lParam, &hdi ) ;

    //  Column 1
    *szText = 0 ;
    if( !LoadString( _Module.GetResourceInstance(), IDS_VALUE_HEADER_ITEM,
                    szText, sizeof(szText)/sizeof(TCHAR) ) )
    {
        ASSERT( FALSE ) ;   // missing string resource
    }
    hdi.cxy     = max( cx - m_metrics.DividerPos(), m_metrics.MinColWidth() ) ;
    hdi.lParam  = 1 ; // iItem (col#)
    hdi.iImage  = m_iChildSortDir[hdi.lParam] + 1 ;
    Header_InsertItem( m_wndHdr, hdi.lParam, &hdi ) ;

    return Header_GetItemCount( m_wndHdr )==2 ;
}

//-------------------------------------------------------------------------//
//  Helper to snag the CPropertyTreeItem pointer associated with the
//  the specified HTREEITEM.
CPropertyTreeItem* CPropertyTreeCtl::GetTreeItem( HTREEITEM hItem )
{
    if( hItem==NULL ) return NULL ;

    TV_ITEM tvi ;
    tvi.mask    = TVIF_HANDLE|TVIF_PARAM ;
    tvi.hItem   = hItem ;
    tvi.lParam  = 0L ;

    if( TreeView_GetItem( m_wndTree, &tvi ) )
        return GetTreeItem( tvi ) ;
    return NULL ;
}

//-------------------------------------------------------------------------//
//  Helper to snag the CPropertyTreeItem pointer associated with the
//  the specified HTREEITEM.
CPropertyTreeItem* CPropertyTreeCtl::GetTreeItem( const TV_ITEM& tvi )
{
    return (CPropertyTreeItem*)tvi.lParam ; 
}

//-------------------------------------------------------------------------//
//  Helper to snag the CPropertyTreeItem pointer associated with the
//  the currently selected HTREEITEM.
CPropertyTreeItem* CPropertyTreeCtl::GetSelection()
{
    return GetTreeItem( TreeView_GetSelection( m_wndTree ) ) ;
}

//-------------------------------------------------------------------------//
//  Handles tooltip notifications
LRESULT CPropertyTreeCtl::OnToolbarToolTip( int, NMHDR* pNMH, BOOL& bHandled )
{
    
    LPTOOLTIPTEXT pTTT = (LPTOOLTIPTEXT)pNMH ;

    switch( pNMH->idFrom )
    {
        case IDC_APPLY:
        case IDC_CUSTOMIZE:
            pTTT->hinst    = _Module.GetResourceInstance() ;
            pTTT->lpszText = MAKEINTRESOURCE( pNMH->idFrom ) ;
            break ;
    }

    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Handles tooltip notifications
LRESULT CPropertyTreeCtl::OnTreeGetInfoTip( int, NMHDR* pNMH, BOOL& bHandled )
{
    NMTVGETINFOTIP* ptip = (NMTVGETINFOTIP*)pNMH ;
    CPropertyTreeItem* pItem = (CPropertyTreeItem*)ptip->lParam ;

    if( pItem )
        lstrcpyn( ptip->pszText, pItem->GetDescription(), ptip->cchTextMax ) ;

    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  WM_GETOBJECT handler for treeview child.
LRESULT CPropertyTreeCtl::OnTreeGetObject( UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HRESULT hr = E_INVALIDARG;
    bHandled = FALSE;

    if( OBJID_CLIENT == lParam )
    {
        IAccessible* paccProxy;
        if( SUCCEEDED( (hr = _CreateStdAccessibleProxy( m_wndTree, wParam, &paccProxy )) ) )
        {
            CAccessibleBase* pacc = new CAccessibleBase;
            if( pacc )
            {
                pacc->Initialize( m_wndTree, paccProxy );
                hr = (HRESULT)_LresultFromObject( IID_IAccessible, wParam, (IAccessible*)pacc );
                bHandled = SUCCEEDED(hr);
            }
            else
                hr = E_OUTOFMEMORY;
            
            paccProxy->Release();
        }
    }
    return (LRESULT)hr;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnTreeGetAccString( UINT, WPARAM wParam, LPARAM lParam, BOOL& )
{
    CPropertyTreeItem* pItem = lParam ? GetTreeItem( (HTREEITEM)lParam ) :
                                        GetSelection();
    if( NULL == pItem )
    {
        if( DISPID_ACC_VALUE == wParam && 
            IsWindow( m_hwndEmpty ) && ::IsWindowVisible( m_hwndEmpty ) )
        {
            BSTR bstrEmpty = NULL;
            if( CalcEmptyStatus( &bstrEmpty ) != EMPTY_NOTEMPTY )
                return (LRESULT)SysAllocString( bstrEmpty );
        }
    }
    else
    {
        LPCTSTR psz = NULL;

        switch( (DISPID)wParam )
        {
        case DISPID_ACC_NAME:
            psz = pItem->GetName(); break;

        case DISPID_ACC_DESCRIPTION:
            psz = pItem->GetDescription(); break;

        case DISPID_ACC_VALUE:
            psz = pItem->ValueText(); break;
        }

        if( psz && *psz )
            return (LRESULT)SysAllocString( T2CW(psz) );
    }
    return NULL;
}

//-------------------------------------------------------------------------//
//  Filters and adjusts results of TVM_HITTEST messages sent to the tree control
//  to support accurate hit testing on custom drawn item elements.
LRESULT CPropertyTreeCtl::OnTreeViewHitTest( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT             lRet = m_wndTree.DefWindowProc( nMsg, wParam, lParam ) ;
    HTREEITEM           hItem = (HTREEITEM)lRet ;
    CPropertyTreeItem*  pItem = GetTreeItem( hItem ) ;
    LPTVHITTESTINFO     pHTI  = (LPTVHITTESTINFO)lParam ;
    
    if( pItem )
    {
        //TRACE( TEXT("Hit test on item %s\n"), pItem->GetDisplayName() ) ;
        pItem->HitTest( pHTI ) ;
    }
    else
    {
        //TRACE( TEXT("Hit test miss\n") ) ;
    }

    bHandled = TRUE ;
    return lRet ;
}

//-------------------------------------------------------------------------//
//  If a button down occurs on an item, we need to shunt default processing
//  becasue it results in some sloppy and redundant repaint paint messages.
LRESULT CPropertyTreeCtl::OnTreeLButtonDown( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    TVHITTESTINFO hti ;
    memset( &hti, 0, sizeof(hti) ) ;
    hti.pt.x = LOWORD( lParam ) ;
    hti.pt.y = HIWORD( lParam ) ;

    m_wndTree.SetFocus() ;
    bHandled = FALSE;

    if( TreeView_HitTest( m_wndTree, &hti )!=NULL &&
        (hti.flags & TVHT_ONITEM || hti.flags & TVHT_ONITEMRIGHT) )
    {
        if( TreeView_GetSelection( m_wndTree ) != hti.hItem )
        {
            TreeView_SelectItem( m_wndTree, hti.hItem );
        }

        if( hti.flags & TVHT_ONITEM )
        {
            CPropertyTreeItem*  pItem = NULL ;
            TV_ITEM             tvi ;

            memset( &tvi, 0, sizeof(tvi) ) ;
            tvi.mask = TVIF_HANDLE|TVIF_PARAM ;
            tvi.hItem = hti.hItem ;

            if( TreeView_GetItem( m_wndTree, &tvi ) &&
                (pItem = GetTreeItem( tvi )) != NULL &&
                pItem->Type()==PIT_FOLDER )
            {
                TreeView_Expand( m_wndTree, hti.hItem, TVE_TOGGLE ) ;
                RepositionEditControl() ;
            }
        }
        bHandled = TRUE;
    }
    
    return 0L;
}

//-------------------------------------------------------------------------//
//  If a button double click occurs on an item, we need to shunt default processing
//  becasue it results in some sloppy and redundant repaint paint messages.
LRESULT CPropertyTreeCtl::OnTreeLButtonDblClk( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    LRESULT lRet = ::DefWindowProc( m_wndTree, nMsg, wParam, lParam );
    m_wndTree.SetFocus() ;

    TVHITTESTINFO hti ;
    memset( &hti, 0, sizeof(hti) ) ;
    hti.pt.x = LOWORD( lParam ) ;
    hti.pt.y = HIWORD( lParam ) ;

    if( TreeView_HitTest( m_wndTree, &hti )!=NULL && hti.flags & TVHT_ONITEM )
    {
        CPropertyTreeItem* pItem ;
        if( (pItem = GetTreeItem( hti.hItem ))!=NULL && 
             pItem->Type()==PIT_PROPERTY )
        {
            if( TreeView_GetSelection( m_wndTree ) != hti.hItem )
            {
                TreeView_SelectItem( m_wndTree, hti.hItem ) ;
            }
        }
    }
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Because we need to shunt the tree view's lbuttondown and lbuttondlbclk, 
//  we need to also make sure that 
LRESULT CPropertyTreeCtl::OnTreeMouseActivate( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    LRESULT lRet        = DefWindowProc( nMsg, wParam, lParam );

    HWND    hwndFocus   = GetFocus();
    BOOL    bActivating = !IsChild( hwndFocus ) && (hwndFocus != m_wndTree);
    UINT    uMsg        = HIWORD(lParam);

    //TRACE( TEXT("WM_MOUSEACTIVATE: hwndTree: %08lX, hwndFocus: %08lX\n"), m_wndTree.m_hWnd, hwndFocus );
    
    if( bActivating && (WM_LBUTTONDOWN == uMsg || WM_LBUTTONDBLCLK == uMsg) )
    {
        PendRedraw( m_wndTree, RDW_ERASE|RDW_INVALIDATE|RDW_FRAME );
    }
    
    return lRet ;
}

//-------------------------------------------------------------------------//
//  Tree view scrolling invalidates the position of the edit control
//  so, we need to update its position.
LRESULT CPropertyTreeCtl::OnTreeVScroll( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //  Pass the message along to the tree control, so that his metrics are updated.
    LRESULT lRet = m_wndTree.DefWindowProc( nMsg, wParam, lParam ) ;
    bHandled     = TRUE ;

    //  Ensure edit control is properly placed.
    RepositionEditControl() ;

    return lRet ; 
}

//-------------------------------------------------------------------------//
//  Tree view scrolling invalidates the position of the edit control
//  so, we need to update its position
LRESULT CPropertyTreeCtl::OnTreeHScroll( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //  Pass the message along to the tree control, so that his metrics are updated.
    LRESULT lRet = m_wndTree.DefWindowProc( nMsg, wParam, lParam ) ;
    bHandled = TRUE ;

    //  Ensure edit control is properly placed.
    RepositionEditControl() ;

    return lRet ; 
}

//-------------------------------------------------------------------------//
//  Tree view scrolling invalidates the position of the edit control
//  so, we need to update its position
LRESULT CPropertyTreeCtl::OnTreeMouseWheel( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //  Pass the message along to the tree control, so that his metrics are updated.
    LRESULT lRet = m_wndTree.DefWindowProc( nMsg, wParam, lParam ) ;
    bHandled = TRUE ;

    //  Ensure edit control is properly placed.
    RepositionEditControl() ;

    return lRet ; 
}

//-------------------------------------------------------------------------//
//  Processes key input.
LRESULT CPropertyTreeCtl::OnTreeKey( 
    UINT nMsg, 
    WPARAM virtKey, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    switch( virtKey )
    {
        case VK_TAB:
            return SendMessage( nMsg, virtKey, lParam ) ;

        case VK_RETURN:
        {
            NAVIGATION_KEY_INFO nki ;
            InitNavigationKeyInfo( &nki, m_hWnd, nMsg, virtKey, lParam ) ;

            LRESULT lRet = m_wndTree.SendMessage( WMU_NAVIGATION_KEY, 
                                                  GetDlgCtrlID(), (LPARAM)&nki ) ;
            bHandled = nki.bHandled ;

            return lRet ;
        }
    }

    bHandled = FALSE ;
    return 0L  ;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnTreeMouseMove( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    POINTS pts = MAKEPOINTS( lParam ) ;
    POINT  pt ;

    pt.x = pts.x ;
    pt.y = pts.y ;

    if( m_hwndToolTip && UpdateToolTipTarget( FALSE, pt ) )
    {
         MSG msg ;
         msg.lParam     = lParam;
         msg.wParam     = wParam;
         msg.message    = nMsg ;
         msg.hwnd       = m_wndTree ;
         ::SendMessage( m_hwndToolTip, TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
    }
    return 0L ;
}

//-------------------------------------------------------------------------//
HTREEITEM CPropertyTreeCtl::UpdateToolTipTarget( BOOL bClear, const POINT& ptTreeItem )
{
    TVHITTESTINFO hti ;

    if( bClear )
    {
        hti.hItem = m_hToolTipTarget ;
        m_hToolTipTarget = NULL ;
        return hti.hItem ;
    }

    memset( &hti, 0, sizeof(hti) ) ;
    hti.pt = ptTreeItem ;

    if( ptTreeItem.x < m_metrics.DividerPos() /* over first column */ &&
        TreeView_HitTest( m_wndTree, &hti ) &&
        hti.flags & (TVHT_ONITEMLABEL|TVHT_ONITEMICON) )
    {
        if( m_hToolTipTarget != hti.hItem )
        {
            ::ShowWindow( m_hwndToolTip, SW_HIDE ) ;
            ::UpdateWindow( m_hwndToolTip ) ;
            m_hToolTipTarget = hti.hItem ;
            ::SendMessage( m_hwndToolTip, TTM_UPDATE, 0, 0);

            //TRACE( TEXT("CPropertyTreeCtl::UpdateToolTipTarget( ) - updating\n") ) ;
        }
    }
    else
    {
        ::SendMessage( m_hwndToolTip, TTM_POP, 0, 0 ) ;
        //TRACE( TEXT("CPropertyTreeCtl::UpdateToolTipTarget( ) - popping\n") ) ;
        return NULL ;
    }   

    return m_hToolTipTarget ;
}



//-------------------------------------------------------------------------//
//  Processes and filters keystrokes targeted at child controls.
LRESULT CPropertyTreeCtl::OnNavigationKey( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    NAVIGATION_KEY_INFO* pNVI  = (NAVIGATION_KEY_INFO*)lParam ;
    CPropertyTreeItem*   pItem = GetSelection() ;
    LRESULT              lRet  = 0L ;
    BOOL                 bKeyDown = (pNVI->nMsg == WM_KEYDOWN) ;// || pNVI->nMsg == WM_CHAR) ;
    BOOL                 bShiftPressed = GetKeyState( VK_SHIFT ) & 0x8000;
    
    ASSERT( pItem ) ;
    bHandled = TRUE ;

    switch( pNVI->virtKey )
    {
        case VK_RETURN:
        {
            if( !bShiftPressed )
            {
                //  Treat these as a means to skip to next/prev property
                if( pItem && bKeyDown )
                {
                    CPropertyTreeItem* pOther ;
            
                    if( NULL != (pOther = GetNextTreeItem( pItem, TRUE )) )
                        TreeView_SelectItem( m_wndTree, (HTREEITEM)(*pOther) ) ;
                }
                pNVI->bHandled = TRUE ;
            }
            break ;
        }

        case VK_UP:
        case VK_DOWN:
            if( !bShiftPressed )
            {
                //  We'll treat the simple vertical navigation keys as
                //  tree control navigation keys.
                lRet = m_wndTree.SendMessage( pNVI->nMsg, pNVI->virtKey, pNVI->lKeyData ) ;
                if( GetKeyState( VK_CONTROL ) & 0x8000 )
                    RepositionEditControl();

                pNVI->bHandled = TRUE ;
            }
            break ;

        case VK_ESCAPE:
            if( bKeyDown )
                pNVI->bHandled = pItem!=NULL ? pItem->OnValueRestore() : FALSE ;
            break ;
    }
     
    return lRet ;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnSetFocus( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = CComControl<CPropertyTreeCtl>::OnSetFocus( nMsg, wParam, lParam, bHandled ) ;
    m_wndTree.SetFocus() ;
    return lRet ;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnTreeSetFocus( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CPropertyTreeItem* pItem ;
    if( (pItem = GetSelection()) != NULL )
    {
        //  Force the item to repaint.
        RECT rcLabel ;
        if( pItem->GetLabelRect( &rcLabel, FALSE ) )
            m_wndTree.InvalidateRect( &rcLabel ) ;

        HWND hwndCtl ;
        if( NULL != (hwndCtl = pItem->EditControl()) )
            ::SetFocus( hwndCtl ) ;
    }
    return 0L ;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnCtlFocus( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    UINT nFocusMsg = (UINT)lParam ;
    CPropertyTreeItem* pItem ;

    if( (pItem = GetSelection())!=NULL )
    {
        //  If we're losing the focus, the item must be informed.
        if( nFocusMsg == WM_KILLFOCUS )
            pItem->OnKillFocus( ) ;

        //  Force the item to repaint.
        RECT rcLabel ;
        if( pItem->GetLabelRect( &rcLabel, FALSE ) )
            m_wndTree.InvalidateRect( &rcLabel ) ;
    }
    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Handles WM_COMMANDs send by the in place edit control to its tree view control
LRESULT CPropertyTreeCtl::OnTreeChildCommand( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CPropertyTreeItem   *pItem ;
    bHandled = FALSE ;

    //  Reflect the command to the CInPlaceBase object representing the control
    LRESULT lRet = CInPlaceBase::ReflectWM_COMMAND( (HWND)lParam, HIWORD(wParam), &bHandled ) ;

    //  If not handled, give the treeitem a crack at it..
    if( !bHandled && (pItem = GetSelection())!=NULL )
        lRet = pItem->OnEditControlCommand( LOWORD(wParam), HIWORD(wParam), (HWND)lParam, 
                                            &m_metrics, bHandled ) ;
    
    return lRet ;
}

//-------------------------------------------------------------------------//
//  Handles WM_NOTIFYs send by the in place edit control to its tree view control
LRESULT CPropertyTreeCtl::OnTreeChildNotify( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CPropertyTreeItem   *pItem ;
    NMHDR*              pHdr = (NMHDR*)lParam ;
    LRESULT             lRet  = 0L ;
    bHandled = FALSE ;

    if( m_hwndToolTip && m_hwndToolTip == pHdr->hwndFrom )
    {
        switch( pHdr->code )
        {
            case TTN_NEEDTEXT:
            {
                LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pHdr ;

                pItem = GetTreeItem( m_hToolTipTarget ) ;

                if( (pItem = GetTreeItem( m_hToolTipTarget )) != NULL )
                    lstrcpyn( pdi->szText, pItem->GetDescription(), ARRAYSIZE( pdi->szText ) ) ;
                
                bHandled = TRUE ;
                break ;
            }
        }
    }
    else
    {
        //  Reflect the command to the CInPlaceBase object representing the control
        lRet = CInPlaceBase::ReflectWM_NOTIFY( pHdr, &bHandled ) ;

        //  If not handled, give the treeitem a crack at it..
        if( !bHandled && (pItem = GetSelection())!=NULL )
            lRet = pItem->OnEditControlNotify( (NMHDR*)lParam, &m_metrics, bHandled ) ;

        
    }

    return lRet ;
}

//-------------------------------------------------------------------------//
//  Retrieves the edit box rectangle
BOOL CPropertyTreeCtl::GetItemValueRect( IN OPTIONAL HTREEITEM hSelItem, OUT LPRECT prcRect )
{
    ASSERT( prcRect ) ;
    memset( prcRect, 0, sizeof(*prcRect) ) ;
    
    //  If we're not supplied with an item handle, 
    //  attempt to retrieve that of the selected item
    if( hSelItem == NULL && (hSelItem = TreeView_GetSelection( m_wndTree )) == NULL )
        return FALSE ;

    //  Retrieve the item's rectangle and adjust it as needed.
    if( TreeView_GetItemRect( m_wndTree, hSelItem, prcRect, FALSE ) )
    {
        prcRect->left = m_metrics.DividerPos() + m_metrics.DividerMargin() ;
        if( prcRect->left > prcRect->right )
            prcRect->left = prcRect->right ;
        return TRUE ;
    }

    return FALSE ;
}

//-------------------------------------------------------------------------//
//  Instructs the tree item associated with an edit control to adjust the
//  edit control's position.
void CPropertyTreeCtl::RepositionEditControl()
{
    CPropertyTreeItem* pItem ;
    if( (pItem = GetSelection()) != NULL )
    {
        RECT rcCtl ;
        GetItemValueRect( (HTREEITEM)(*pItem), &rcCtl ) ;
        pItem->RepositionEditControl( &rcCtl, &m_metrics ) ;
    }
}

//-------------------------------------------------------------------------//
//  The tree view window calls back for text, image list indices, etc.
//  We pass these requests to the tree item for handling.
LRESULT CPropertyTreeCtl::OnTreeItemCallback( int, NMHDR* pNMH, BOOL& bHandled )
{
    TV_DISPINFO*        pDI = (TV_DISPINFO*)pNMH ;
    CPropertyTreeItem*  pItem  ;

    if( (pItem = GetTreeItem( pDI->item ))!=NULL )
        return pItem->OnTreeItemCallback( pDI, bHandled ) ;

    return 0L ;
}

//-------------------------------------------------------------------------//
//  Asks the focus item for permission to transfer focus.
LRESULT CPropertyTreeCtl::OnTreeItemSelChanging( int, NMHDR* pNMH, BOOL& bHandled )
{
    NM_TREEVIEW*        pNMTV = (NM_TREEVIEW*)pNMH ;
    LRESULT             lRet = 0L ;

    CPropertyTreeItem   *pItemOld, *pItemNew ;

    pItemOld = pNMTV->itemOld.hItem ? GetTreeItem( pNMTV->itemOld ) : NULL ;
    pItemNew = pNMTV->itemNew.hItem ? GetTreeItem( pNMTV->itemNew ) : NULL ;

    if( pItemOld && !pItemOld->OnKillFocus( pNMTV->action, pItemOld ) )
        lRet = TRUE ;

    bHandled = TRUE ;
    return lRet ;
}

//-------------------------------------------------------------------------//
//  Handles edit control updating when the tree view selection changes.
//  For the most part, the losing and gaining tree items do most of the
//  work.  We'll assign focus as appropriate.
LRESULT CPropertyTreeCtl::OnTreeItemSelChanged( int, NMHDR* pNMH, BOOL& bHandled )
{
    NM_TREEVIEW*        pNMTV = (NM_TREEVIEW*)pNMH ;
    HWND                hwndCtl ;
    CPropertyTreeItem   *pItemOld, *pItemNew ;
    RECT                rcOld, rcNew ;
    int                 nCmdShowOld = SW_HIDE, 
                        nCmdShowNew = SW_SHOW ;

    if( m_bDestroyed )
        return 0L ;

    pItemOld = pNMTV->itemOld.hItem ? GetTreeItem( pNMTV->itemOld ) : NULL ;
    pItemNew = pNMTV->itemNew.hItem ? GetTreeItem( pNMTV->itemNew ) : NULL ;

    //  Get value rectangles for old and new items.
    if( pItemOld )
    {
        nCmdShowOld = ((pNMTV->itemOld.state & TVIS_SELECTED)==0) ?
                      SW_HIDE : SW_SHOW ;
        GetItemValueRect( pNMTV->itemOld.hItem, &rcOld ) ;
    }
    if( pItemNew )
    {
        nCmdShowNew = ((pNMTV->itemNew.state & TVIS_SELECTED)==0) ?
                      SW_HIDE : SW_SHOW ;
        GetItemValueRect( pNMTV->itemNew.hItem, &rcNew ) ;
    }

    //  Instruct old item to hide its edit control
    if( pItemOld )
    {
        pItemOld->ShowEditControl( nCmdShowOld, &rcOld, &m_metrics ) ;
        if( !pItemNew && m_bInfobarVisible )
            m_wndQtip.SetWindowText( NULL ) ;
    }
    
    //  Instruct new item to show its edit control.
    if( pItemNew )
    {
        if( (hwndCtl = pItemNew->ShowEditControl( nCmdShowNew, &rcNew, &m_metrics )) != NULL )
            ::SetFocus( hwndCtl ) ;
        else
            m_wndTree.SetFocus() ;

        if( m_bInfobarVisible )
            pItemNew->DisplayQtipText( m_wndQtip ) ;
    }
    else
    {
        m_wndTree.SetFocus() ;
        
        if( m_bInfobarVisible )
            m_wndQtip.SetWindowText( NULL ) ;
    }

    m_wndHdr.Invalidate() ;
    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Instructs the tree item that's expanding to insert it's children into
//  the tree view.
LRESULT CPropertyTreeCtl::OnTreeItemExpanding( int, NMHDR* pNMH, BOOL& bHandled )
{
    NM_TREEVIEW*    pNMTV    = (NM_TREEVIEW*)pNMH ;
   
    if( pNMTV->action == TVE_EXPAND && (pNMTV->itemNew.state & TVIS_EXPANDEDONCE)==0 )
    {
        CPropertyTreeItem* pItem ;
        if( (pItem = GetTreeItem( pNMTV->itemNew ))!=NULL )
            pItem->InsertChildren() ;

        bHandled = TRUE ;
    }
    return 0L ;
}

//-------------------------------------------------------------------------//
//  When the tree is expanded, the tree view does some vertical scrolling
//  that's sorta unpredictable, so we need to adjust the position of the 
//  edit control.
LRESULT CPropertyTreeCtl::OnTreeItemExpanded( int, NMHDR* pNMH, BOOL& bHandled )
{
    RepositionEditControl() ;
    bHandled = TRUE ;
    return FALSE ;
}

//-------------------------------------------------------------------------//
//  The user has invoked tree label mode.  For now, we block it.
LRESULT CPropertyTreeCtl::OnTreeBeginLabelEdit( int, NMHDR* pHdr, BOOL& bHandled )
{
    bHandled = TRUE ;
    return TRUE ;   // BUGBUG: need to implement editing property names.
}

//-------------------------------------------------------------------------//
//  Handles termination of tree item label editing.
LRESULT CPropertyTreeCtl::OnTreeEndLabelEdit( int, NMHDR* pHdr, BOOL& bHandled )
{
    LPNMTVDISPINFO pDI = (LPNMTVDISPINFO)pHdr ;
    
    if( pDI->item.pszText )
    {
        CPropertyTreeItem* pItem ;
        if( (pItem = GetTreeItem( pDI->item ))!=NULL )
        {
            //pItem->SetDisplayText( pDI->item.pszText ) ;
        }
    }
    bHandled = TRUE ;
    return TRUE ;   // accept the change
}

//-------------------------------------------------------------------------//
//  Instructs item to draw itself.upon receiving an item prepaint request
LRESULT CPropertyTreeCtl::OnTreeItemCustomDraw( int, NMHDR* pNMH, BOOL& bHandled )
{
    LPNMTVCUSTOMDRAW    pDraw = (LPNMTVCUSTOMDRAW)pNMH ;
    CPropertyTreeItem*  pItem ; 
    
    bHandled = TRUE ;

    //  Respond to request to prepaint entire tree view
    if( pDraw->nmcd.dwDrawStage == CDDS_PREPAINT )
        return CDRF_NOTIFYITEMDRAW ;

    //  Respont to request to prepaint item
    if( pDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT &&
        (pItem = (CPropertyTreeItem*)pDraw->nmcd.lItemlParam) != NULL )
        return pItem->Draw( pDraw, &m_metrics ) ;

    return CDRF_DODEFAULT ;
}

//-------------------------------------------------------------------------//
//  TVN_QUERYITEMWIDTH handler
LRESULT CPropertyTreeCtl::OnTreeQueryItemWidth( int, NMHDR* pNMH, BOOL& bHandled )
{
#ifdef TVN_QUERYITEMWIDTH
    LPTVQUERYITEMWIDTH pTQW = (LPTVQUERYITEMWIDTH)pNMH ;
    CPropertyTreeItem* pItem ;
    bHandled = TRUE ;

    if( (pItem = GetTreeItem( pTQW->item )) != NULL )
    {
        pTQW->cx = pItem->ItemWidth( TRUE ) ;
        pTQW->flags = TVIF_TEXT ;
        return TRUE ;
    }
#endif TVN_QUERYITEMWIDTH
    return 0L ;
}

//-------------------------------------------------------------------------//
//  TVN_DELETEITEM handler.
//  The call to CPropertyTreeItem::Remove() is very important, as it causes 
//  the item to logically dissociate from the tree.
LRESULT CPropertyTreeCtl::OnTreeItemDelete( int, NMHDR* pNMH, BOOL& bHandled )
{
    NM_TREEVIEW* pNMTV = (NM_TREEVIEW*)pNMH ;
    CPropertyTreeItem*  pItem ;

    //  Clear as tool tip target
    if( pNMTV->itemOld.hItem == m_hToolTipTarget )
        m_hToolTipTarget = NULL ;

    //  Instruct item to detach from tree.
    if( (pItem = GetTreeItem( pNMTV->itemOld ))!=NULL )
    {
        pItem->Remove() ;
    }
    
    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Our task here is to build up a collection of visible rectanges 
//  associated with 2nd-level tree items (excludes those of folder items.)
//  These rectangles will be used for horizontal scrolling of property
//  items when the column divider is moved by the user.
//
//  Our approach is to first create a rectangle encompassing all visible items,
//  and then we'll punch out the rectangles of the root items.
int CPropertyTreeCtl::GetVisiblePropertyItemRects( RECT rects[], int cRects )
{
    RECT        rcFirst, 
                rcLast ;
    HTREEITEM   hItemFirst  = TreeView_GetFirstVisible( m_wndTree ), 
                hItemLast   = TreeView_GetLastVisible( m_wndTree ),
                hItemRoot ;

    //  Retrieve and merge rectangles of first and last visible items
    TreeView_GetItemRect( m_wndTree, hItemFirst, &rcFirst, FALSE ) ;
    TreeView_GetItemRect( m_wndTree, hItemLast, &rcLast, FALSE ) ;

    UnionRect( &rects[0], &rcFirst, &rcLast ) ;

    //  Everything to the left of the following is painted by the tree control
    rects[0].left = m_metrics.ItemIndent( PROPERTY_TIER ) + m_metrics.ImageCellWidth() + m_metrics.ImageMargin() ;

    //  Determine whether our first visible is a root item. If not, get it's root item.
    if( (hItemRoot = TreeView_GetParent( m_wndTree, hItemFirst ))==NULL )
        hItemRoot = hItemFirst ;

    //  Walk the root items, punching their rectangles out of the block.
    for( int cnt=1; cnt<cRects-1 && hItemRoot; 
         hItemRoot = TreeView_GetNextSibling( m_wndTree, hItemRoot ) )
    {
        RECT rcRoot, rcX ;
        if( TreeView_GetItemRect( m_wndTree, hItemRoot, &rcRoot, FALSE ) &&
            IntersectRect( &rcX, &rcRoot, &rects[cnt-1] ) )
        {
            if( rects[cnt-1].top >= rcRoot.top )        
                // root item overlaps upper terminus
                rects[cnt-1].top = rcRoot.bottom ;
            else 
            if ( rects[cnt-1].bottom <= rcRoot.bottom ) 
                // root item overlaps lower terminus
                rects[cnt-1].bottom = rcRoot.top ;
            else  
            {
                // root item to needs to be punched out of middle
                CopyRect( &rects[cnt], &rects[cnt-1] ) ;
                rects[cnt-1].bottom = rcRoot.top ;
                rects[cnt].top = rcRoot.bottom ;
                cnt++ ;
            }
        }
    }
    return cnt ;
}

//-------------------------------------------------------------------------//
//  Handles WM_SETCURSOR messages when the mouse is over the header control.  
//  We want to  prevent the sizing cursor from appearing when the cursor is over
//  the rightmost item divider.
LRESULT CPropertyTreeCtl::OnHeaderSetCursor( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HD_HITTESTINFO hti ;
    static HCURSOR hcurStd = NULL ;

    bHandled = TRUE ;
    memset( &hti, 0, sizeof(hti) ) ;
    GetCursorPos( &hti.pt ) ;
    ::MapWindowPoints( HWND_DESKTOP, m_wndHdr, &hti.pt, 1 ) ;
    
    if( m_wndHdr.SendMessage( HDM_HITTEST, 0, (LPARAM)&hti )!=-1 )
    {
        if( (hti.flags & HHT_ONDIVIDER)!=0 && hti.iItem==1 )
        {
            if( hcurStd == NULL )
                hcurStd = ::LoadCursor( NULL, IDC_ARROW ) ;
            SetCursor( hcurStd ) ;
            return TRUE ;
        }
    }            
    return m_wndHdr.DefWindowProc( nMsg, wParam, lParam ) ;        
}

#if _ATL_VER <= 0x0300
//-------------------------------------------------------------------------//
//  HANDLES WM_NCDESTROY sent to header control child window.
//  Note: this works around a bug in ATL2.1
LRESULT CPropertyTreeCtl::OnHeaderNcDestroy( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //  Don't let ATL 2x call DefWindowProc, it NULLs the HWND before passing to 
    //  the subclassee.
    bHandled = TRUE ;
    return m_wndHdr.DefWindowProc( nMsg, wParam, lParam ) ;
}
#endif //_ATL_VER <= 0x0300

//-------------------------------------------------------------------------//
//  Handles changes occuring to the column headers, the most important of 
//  which is column width.
LRESULT CPropertyTreeCtl::OnHeaderItemChanging( int, NMHDR* pNMH, BOOL& bHandled )
{
    NMHEADER* pHdr = (NMHEADER*)pNMH ;
    bHandled = TRUE ;

    if( pHdr->iItem==0 && pHdr->pitem->mask & HDI_WIDTH )
    {
        //  The user is resizing the column width
        int  cxOld          = m_metrics.DividerPos(), // save the old column width
             cxIndent       = m_metrics.LabelBoxLeft( PROPERTY_TIER ) ;
             
        //  Ignore cases when the width has not changed
        if( m_metrics.DividerPos() == pHdr->pitem->cxy )
            return 0L ;

        //  First column must be as least as wide as the property text indent
        if( pHdr->pitem->cxy < cxIndent )
        {
            if( cxOld == cxIndent )  // have we min'd out?
                return 1L ; // nothing more to do, but block any further retraction
            pHdr->pitem->cxy = cxIndent ;
        }

        RECT rcPropItems[10],
             rcThis, 
             rcClient, 
             rcScroll, 
             rcPaint ;

        GetClientRect( &rcThis );
        if( pHdr->pitem->cxy > (RECTWIDTH(&rcThis) - m_metrics.DividerMargin()) )
            pHdr->pitem->cxy = (RECTWIDTH(&rcThis) - m_metrics.DividerMargin());

        //  Stash the new column width
        m_metrics.SetDividerPos( pHdr->pitem->cxy ) ;

        //  Scroll/repaint columns...
        m_wndTree.GetClientRect( &rcClient ) ;
        
        //  Scroll the property items.
        for( int i=0,
             cRects = GetVisiblePropertyItemRects( rcPropItems, sizeof(rcPropItems)/sizeof(RECT) ) ;
             i<cRects; i++ )
        {
            //  Scroll the left (value) column
            rcScroll = rcPropItems[i] ;
            rcScroll.left = max( cxOld, cxIndent ) + m_metrics.DividerMargin() ;
            rcScroll.right= rcClient.right ;
            m_wndTree.ScrollWindow( m_metrics.DividerPos() - cxOld, 0, &rcScroll, NULL ) ;

            //  Repaint the right (property name) column
            rcPaint = rcPropItems[i] ;
            rcPaint.left = max( min( cxOld, m_metrics.DividerPos() ) - (m_metrics.EllipsisWidth()*2), cxIndent ) ;
            rcPaint.right= max( cxOld, m_metrics.DividerPos() ) ;
            if( rcPaint.right > rcPaint.left )
                m_wndTree.InvalidateRect( &rcPaint, TRUE ) ;
            m_wndTree.UpdateWindow() ;
        }

        //  Ensure that the trailing column header resizes appropriately.
        RECT rcHdr ;
        int  iLastCol = Header_GetItemCount( m_wndHdr )-1 ;

        if( iLastCol > 0 )
        {
            HDITEM hdi ;

            m_wndHdr.GetClientRect( &rcHdr ) ;
            hdi.mask = HDI_WIDTH ;
            hdi.cxy = (rcHdr.right - rcHdr.left) - pHdr->pitem->cxy ;

            Header_SetItem( m_wndHdr, iLastCol, &hdi ) ;
        }

        //  Finally, ensure that the edit control is positioned correctly.
        RepositionEditControl() ;
    }
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Blocks right-hand resizing of the terminal column.
LRESULT CPropertyTreeCtl::OnHeaderItemBeginTrack( int, NMHDR* pNMH, BOOL& bHandled )
{
    NMHEADER*   pHdr = (NMHEADER*)pNMH ;
    int         iLastItem ;

    bHandled  = TRUE ;

    if( (iLastItem = Header_GetItemCount( m_wndHdr )-1) >=0 && 
         pHdr->iItem == iLastItem )
         return TRUE ;  // block resizing tracking on last item

    return FALSE ;
}

//-------------------------------------------------------------------------//
//  Calculates and caches a 'best fit' column width based on the maximal 
//  property label text width.
int CPropertyTreeCtl::GetBestFitHdrWidth( BOOL bRecalc )
{
    int cxBestFit = m_metrics.BestFitDividerPos() ;
    
    if( bRecalc || cxBestFit <=0 )
    {
        int cxTextMax = 0 ;
        HDC hdcTree ;
        
        if( (hdcTree = m_wndTree.GetDC()) != NULL )
        {
            HFONT     hf, hfOld = NULL ;
            HTREEITEM hRoot ;
            
            //  Select correct font into device context
            if( (hf = (HFONT)m_wndTree.SendMessage( WM_GETFONT, 0, 0 ))!=NULL )
                hfOld = (HFONT)SelectObject( hdcTree, hf ) ;
            
            //  Find the width of the longest property name...
            for( hRoot = TreeView_GetRoot( m_wndTree ) ;
                 hRoot!=NULL ;
                 hRoot = TreeView_GetNextSibling( m_wndTree, hRoot ) )
            {
                TV_ITEM tvi ;
                tvi.mask      = TVIF_HANDLE|TVIF_PARAM|TVIF_STATE ;
                tvi.stateMask = (DWORD)-1 ;

                //  For each expanded property folder
                tvi.hItem = hRoot ;
                if( TreeView_GetItem( m_wndTree, &tvi ) && (tvi.state & TVIS_EXPANDED) )
                {
                    for( tvi.hItem = TreeView_GetChild( m_wndTree, hRoot ) ;
                         tvi.hItem!=NULL ;
                         tvi.hItem = TreeView_GetNextSibling( m_wndTree, tvi.hItem ) )
                    {
                        //  For each property in folder
                        CPropertyTreeItem* pItem ;
                        if( TreeView_GetItem( m_wndTree, &tvi ) &&
                            (pItem = GetTreeItem( tvi ))!=NULL )
                        {
                            LPCTSTR pszText ;
                            SIZE    sizeText ;
                        
                            //  Calculate the width of the display name
                            if( (pszText = pItem->GetDisplayName())!=NULL && 
                                *pszText &&
                                GetTextExtentPoint32( hdcTree, pszText, lstrlen( pszText ), &sizeText ) &&
                                sizeText.cx > cxTextMax )
                            {
                                cxTextMax = sizeText.cx ;
                            }
                        }
                    }
                }
            }

            //  While we're here, grab the size of an ellipsis string.
            SIZE sizeEllipsis ;
            if( GetTextExtentPoint32( hdcTree, TEXT("..."), 3, &sizeEllipsis ) )
                m_metrics.SetEllipsisWidth( sizeEllipsis.cx ) ;
            
            //  Restore previous font
            if( hf ) SelectObject( hdcTree, hfOld ) ;
            
            m_wndTree.ReleaseDC( hdcTree ) ;
        }
        
        //  Calculate the best fit property header width
        m_metrics.SetBestFitDividerPos( cxTextMax, PROPERTY_TIER ) ;
    }
    return m_metrics.BestFitDividerPos() ;
}

//-------------------------------------------------------------------------//
//  Implements a 'best fit' column width based on the maximal property label 
//  text width.
LRESULT CPropertyTreeCtl::OnHeaderDividerDblClick( int, NMHDR* pNMH, BOOL& bHandled )
{
    if( GetBestFitHdrWidth( TRUE ) >= m_metrics.MinColWidth() )
    {
        RECT rcClient;
        HDITEM hdi ;
        hdi.mask = HDI_WIDTH ;
        GetClientRect( &rcClient );
        hdi.cxy  = min( GetBestFitHdrWidth( FALSE ), 
                        (RECTWIDTH(&rcClient) - m_metrics.DividerMargin()) );
        Header_SetItem( m_wndHdr, 0, &hdi ) ;

        //  Force all visible property items to repaint
        RECT rc[10] ;
        int  i, cRects ;
        for( i = 0, cRects = GetVisiblePropertyItemRects( rc, 10 ) ;
             i<cRects; i++ )
        {
            m_wndTree.InvalidateRect( &rc[i], TRUE ) ;
        }
        m_wndTree.UpdateWindow() ;

        //  Finally, ensure that the edit control is positioned correctly.
        RepositionEditControl() ;
    }

    bHandled = TRUE ;
    return 0L ;
}

typedef struct tagITEMSORT
{
	int					iCol ;        // column being sorted
    int                 *childSortDir ; // 2-element integer array, one element per column, 
                                      // each specifying current column sort order.
} ITEMSORT, *PITEMSORT ;

//-------------------------------------------------------------------------//
//  Handles column header click by sorting the tree view items.
LRESULT CPropertyTreeCtl::OnHeaderItemClick( int, NMHDR* pNMH, BOOL& bHandled )
{
    NMHEADER*  pHdr = (NMHEADER*)pNMH ;

	Sort( NULL, pHdr->iItem, TRUE ) ;
    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Sorts siblings of the indicated tree item.  If no item handle is provided,
//  sorts the siblings of the selected item.
void CPropertyTreeCtl::Sort( 
    IN OPTIONAL HTREEITEM hItem, 
    IN int iCol, 
    IN BOOL bAdvanceDirection )
{
	ITEMSORT  sort ;
	TV_SORTCB tvsort ;
	
	sort.iCol = iCol ;
    sort.childSortDir = NULL ;

	memset( &tvsort, 0, sizeof(tvsort) ) ;
	tvsort.hParent     = hItem ;
	tvsort.lpfnCompare = SortItemProc ;
    tvsort.lParam	   = (LPARAM)&sort ;

    if( (sort.childSortDir = GetSortDirection( iCol, hItem, &tvsort.hParent ))==NULL )
        return ;

 	if( bAdvanceDirection )
    {
        //	Advance the column's chld sort direction (0 == sort in default order)
        if( ++sort.childSortDir[sort.iCol] > 1 )
		     sort.childSortDir[sort.iCol] = -1 ;
    }
    
    //  Set the other column's sort direction to the default order.
    int   iColOther = sort.iCol==0 ? 1 : 0 ;
    sort.childSortDir[iColOther] = 0 ;

    SetHeaderImage( sort.iCol, sort.childSortDir[sort.iCol]+1 ) ;
    SetHeaderImage( iColOther, sort.childSortDir[iColOther]+1 ) ;

	//  Do the column sort
    TreeView_SortChildrenCB( m_wndTree, &tvsort, FALSE ) ;
    RepositionEditControl() ; 
}

//-------------------------------------------------------------------------//
//  Retrieve the sort direction array for the specified item and it's siblings.
//  If the requested item is NULL, the currently selected item will be used.
int* CPropertyTreeCtl::GetSortDirection(
    IN int iCol, 
    IN OPTIONAL HTREEITEM hItem, 
    OUT OPTIONAL HTREEITEM* phParent )
{
	int*      aiRet   = NULL ;
    HTREEITEM hParent = TVI_ROOT ;

    if( phParent ) *phParent = NULL ;  // initialize output

    //  If we've no tree, caller can't do anything anyway so bail.
    if( m_wndTree.m_hWnd==NULL )
        return NULL ;

    //  If we weren't given an item, try determining the target item
    //  via the selected item. And if that somehow fails, bail.
    if( hItem == NULL )
    {
        //  Get the sort target for the column.
        CPropertyTreeItem* pItem;
        if( NULL == (pItem = GetSelection()) )
            return NULL;
            
        if( NULL == (hItem = pItem->GetSortTarget( iCol )) )
            return NULL;
    }

	//  Get the item's parent
    if( (hParent = TreeView_GetParent( m_wndTree, hItem ))==NULL )
		hParent  = TVI_ROOT ;

    if( hParent == TVI_ROOT )
        // Parent is root; return top-level sort direction array.
        aiRet = m_iChildSortDir ;
    else
    {
        //  Parent is non-root; retrieve the sort direction array for its children
        CPropertyTreeItem* pItem ;
        if( (pItem = GetTreeItem( hParent ))==NULL )
            return NULL ;
        aiRet = pItem->ChildSortDir() ;
    }

    if( phParent ) *phParent = hParent ;    // assign output
    return aiRet ;
}

//-------------------------------------------------------------------------//
//  Implements item compare logic tree view item sorting.
int CPropertyTreeCtl::SortItemProc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
	CPropertyTreeItem  *pItem1    = (CPropertyTreeItem*)lParam1,
					   *pItem2    = (CPropertyTreeItem*)lParam2 ;
	PITEMSORT          pItemSort  = (PITEMSORT)lParamSort ;
	int				   iDirection = pItemSort->childSortDir[pItemSort->iCol],
                       iRet       = 0 ;
	
    ASSERT( pItem1 && pItem2 ) ;
	
    if( iDirection==0 ) // default order 
    {
        return  (pItem1->Order() > pItem2->Order()) ? 1 :
                (pItem1->Order() < pItem2->Order()) ? -1 : 0 ;
    }
    else                // 1 or -1 (ascending or descending)
    {
        LPCTSTR psz1 = pItemSort->iCol==0 ? pItem1->GetDisplayName() :
                                            pItem1->ValueText(),
                psz2 = pItemSort->iCol==0 ? pItem2->GetDisplayName() :
                                            pItem2->ValueText() ;
        if( psz1 && psz2 )
            iRet = lstrcmpi( psz1, psz2 ) ;
        else if( psz1 )
            iRet = 1 ;
        else if( psz2 )
            iRet = -1 ;

        iRet *= iDirection ;
    }
    return iRet ;
}

//-------------------------------------------------------------------------//
BOOL CPropertyTreeCtl::SetHeaderImage( int iCol, int iImage )
{
    HD_ITEM hdi ;
    hdi.mask   = HDI_IMAGE ;
    hdi.iImage = iImage ;
    return Header_SetItem( m_wndHdr, iCol, &hdi ) ;
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnRebarBtmChildSize( int, NMHDR* pNMH, BOOL& bHandled )
{
    LPNMREBARCHILDSIZE pRBCS = (LPNMREBARCHILDSIZE)pNMH ;

    //  We want the qtip control flush on the right 
    //  with the band's border.
    if( pRBCS->wID == RBBID_QTIP )
        pRBCS->rcChild.right = pRBCS->rcBand.right ;
    
    bHandled = TRUE ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Handles control window resizing
LRESULT CPropertyTreeCtl::OnSize( UINT, WPARAM, LPARAM lParam, BOOL& )
{
    PositionControls( LOWORD(lParam), HIWORD(lParam) ) ;
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Implements subelement layout.
void CPropertyTreeCtl::PositionControls( int cx, int cy )
{
    RECT        rcThis, rcTree, rcEmpty, rcBarBtm, rcHdrItem ;
#ifdef _PROPERTYTREE_TOOLBAR
    RECT        rcBarTop ;
#endif _PROPERTYTREE_TOOLBAR
    HDWP        hdwp ;
    HDLAYOUT    hdrLayout ;
    WINDOWPOS   hdrPos ;

    if( IsIconic() || 
        !(  IsWindow( m_wndTree ) && IsWindow( m_wndHdr ) && 
#ifdef _PROPERTYTREE_TOOLBAR
            IsWindow( m_wndRebarTop ) && 
#endif _PROPERTYTREE_TOOLBAR
            (m_bInfobarVisible == FALSE || IsWindow( m_wndInfobar )) ) )
        return ;

    SetRect( &rcThis, 0, 0, cx, cy ) ;
    InflateRect( &rcThis, -m_metrics.BorderWidth(), -m_metrics.BorderWidth() ) ;

#ifdef _PROPERTYTREE_TOOLBAR
    CalcAvailableClientRect( rcThis, m_wndRebarTop, rcBarTop ) ;
#endif _PROPERTYTREE_TOOLBAR

    if( m_bInfobarVisible )
        CalcAvailableClientRect( rcThis, m_wndInfobar, rcBarBtm ) ;

    //  Retrieve optimum position of header control
    memset( &hdrLayout, 0, sizeof(hdrLayout) ) ;
    hdrLayout.prc   = &rcThis ;
    hdrLayout.pwpos = &hdrPos ;
    Header_Layout( m_wndHdr, &hdrLayout ) ;
#ifdef _PROPERTYTREE_TOOLBAR
    hdrPos.y-=2 ;       // ignore top rebar's 'header' pixels
#endif _PROPERTYTREE_TOOLBAR

    rcTree.left  = rcThis.left ;
    rcTree.right = rcThis.right ;
    rcTree.top   = hdrPos.y + hdrPos.cy ;
    rcTree.bottom= rcThis.bottom ;
    
    if( m_bInfobarVisible )
        rcTree.bottom += 2; // ignore bottom rebar's 'header' pixels
    
    CopyRect( &rcEmpty, &rcTree ) ;
    InflateRect( &rcEmpty, -m_metrics.EmptyCaptionMargin(),
                           -m_metrics.EmptyCaptionMargin() ) ;
    rcEmpty.bottom = rcTree.bottom ;
    MapWindowPoints( m_wndTree, &rcEmpty ) ;

    if( (hdwp = BeginDeferWindowPos( 5 ))!=NULL )
    {
#ifdef _PROPERTYTREE_TOOLBAR
        ::DeferWindowPos( hdwp, m_wndRebarTop, NULL, rcBarTop.left, rcBarTop.top,
                          rcBarTop.right - rcBarTop.left, rcBarTop.bottom - rcBarTop.top,
                          SWP_NOZORDER|SWP_NOACTIVATE ) ;
#endif _PROPERTYTREE_TOOLBAR

        ::DeferWindowPos( hdwp, m_wndHdr, NULL, 
                          hdrPos.x, hdrPos.y, hdrPos.cx, hdrPos.cy,
                          SWP_NOZORDER|SWP_NOACTIVATE ) ;

        ::DeferWindowPos( hdwp, m_wndTree, NULL, rcTree.left, rcTree.top,
                          rcTree.right - rcTree.left, rcTree.bottom - rcTree.top,
                          SWP_NOZORDER|SWP_NOACTIVATE ) ;
        
        if( m_bInfobarVisible )
        {
            ::DeferWindowPos( hdwp, m_wndInfobar, NULL, rcBarBtm.left, rcBarBtm.top,
                              rcBarBtm.right - rcBarBtm.left, rcBarBtm.bottom - rcBarBtm.top,
                              SWP_NOZORDER|SWP_NOACTIVATE ) ;
        }

        EndDeferWindowPos( hdwp ) ;

        ::SetWindowPos( m_hwndEmpty, NULL, rcEmpty.left,rcEmpty.top,
                        rcEmpty.right-rcEmpty.left,rcEmpty.bottom-rcEmpty.top,
                        SWP_NOZORDER|SWP_NOACTIVATE ) ;
        RepositionEditControl() ;
    }

    //  Ensure that the rightmost header item is resized.
    if( Header_GetItemRect( m_wndHdr, 1, &rcHdrItem ) )
    {
        int nHdrRight = hdrPos.x + hdrPos.cx ;

        m_wndHdr.MapWindowPoints( m_hWnd, (LPPOINT)&rcHdrItem, 2 ) ;
        if( rcHdrItem.right != nHdrRight )
        {
            HDITEM hdi ;
            hdi.mask = HDI_WIDTH ;
            hdi.cxy  = nHdrRight - rcHdrItem.left ;
            Header_SetItem( m_wndHdr, 1, &hdi ) ;
        }
    }
}

//-------------------------------------------------------------------------//
void CPropertyTreeCtl::CalcAvailableClientRect( 
    IN OUT RECT& rcClient, 
    IN CContainedWindow& wndRebar, 
    OUT RECT& rcBar ) const
{
    DWORD dwBarStyle = 0L ;
    SIZE  sizeBar    = {0,0} ;
    SetRect( &rcBar, 0,0,0,0); 

    if( (dwBarStyle = wndRebar.GetStyle()) & WS_VISIBLE )
    {
        //  Defer to rebar's height calculation, but adjust it's 
        //  position and width.
        wndRebar.GetWindowRect( &rcBar ) ;
        ScreenToClient( &rcBar ) ;

        //  Stash innate height, width
        sizeBar.cx = rcBar.right - rcBar.left,
        sizeBar.cy = rcBar.bottom- rcBar.top ;
        rcBar = rcClient ;

        //  Adust available client area according to position of rebar:
        if( (dwBarStyle & 0xF)==CCS_BOTTOM )
        {
            if( dwBarStyle & CCS_VERT )
            {
                rcBar.left = rcBar.right - sizeBar.cx ;
                rcClient.right  = rcBar.left ;
            }
            else
            {
                rcBar.top = rcBar.bottom - sizeBar.cy ;
                rcClient.bottom = rcBar.top ;
            }
        }
        else if( (dwBarStyle & 0xF) == CCS_TOP )
        {
            if( dwBarStyle & CCS_VERT )
            {
                rcBar.right = rcBar.left + sizeBar.cx ;
                rcClient.left  = rcBar.right ;
            }
            else
            {
                rcBar.bottom = rcBar.top + sizeBar.cy ;
                rcClient.top = rcBar.bottom ; 
            }
        }
    }
}

//-------------------------------------------------------------------------//
LRESULT CPropertyTreeCtl::OnCtlColorStatic( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = FALSE ;

    if( m_hwndEmpty == (HWND)lParam )
    {
        bHandled = TRUE ;
        return (LRESULT)m_metrics.EmptyCaptionBrush() ;
    }
    return 0L ;
}

//-------------------------------------------------------------------------//
//  Walks the tree, returning item's next relation in the following order:
//  (1) child, (2) sibling, (3) parent's next sibling.
CPropertyTreeItem* CPropertyTreeCtl::GetNextTreeItem( 
    CPropertyTreeItem* pSrc /*NULL: get root item*/,
    BOOL bCycle )
{
    TV_ITEM     tvi ;
    memset( &tvi, 0, sizeof(tvi) ) ;
    tvi.mask = TVIF_PARAM|TVIF_HANDLE ;

    if( !pSrc )
    {
        //  fetch root item handle
        if( NULL == (tvi.hItem = TreeView_GetChild( m_wndTree, TVI_ROOT )) )
            return NULL ;
    }
    else
    {
        //  Has a child?
        if( NULL == (tvi.hItem = TreeView_GetChild( m_wndTree, (HTREEITEM)*pSrc )) )
        {
            //  No; has a sibling?
            if( NULL == (tvi.hItem = TreeView_GetNextSibling( m_wndTree, (HTREEITEM)*pSrc )) )
            {
                //  No; has a parent?
                if( NULL != (tvi.hItem = TreeView_GetParent( m_wndTree, (HTREEITEM)*pSrc )) )
                {
                    HTREEITEM hParent = tvi.hItem ;
                    //  Yes ; return's parent's next sibling
                    while( NULL != hParent && NULL == (tvi.hItem = TreeView_GetNextSibling( m_wndTree, hParent )) )
                        hParent = TreeView_GetParent( m_wndTree, hParent ) ;
                }
            }
        }
    }

    //  cycle back to root if necessary
    if( bCycle && NULL == tvi.hItem )
        tvi.hItem = TreeView_GetChild( m_wndTree, TVI_ROOT ) ;

    if( NULL != tvi.hItem &&
        TreeView_GetItem( m_wndTree, &tvi ) &&
        tvi.lParam != (LPARAM)pSrc )
        return (CPropertyTreeItem*)tvi.lParam ;

    return NULL ;
}

#if 0  // this is untested and currently unneeded.
//-------------------------------------------------------------------------//
//  Walks the tree, returning item's previous relation in the following order:
//  (1) previous sibling, (2) parent
CPropertyTreeItem* CPropertyTreeCtl::GetPrevTreeItem( 
    CPropertyTreeItem* pSrc,
    BOOL bCycle )
{
    TV_ITEM     tvi ;
    HTREEITEM   hItem ;
    HRESULT     hr ;

    memset( &tvi, 0, sizeof(tvi) ) ;
    tvi.mask = TVIF_PARAM|TVIF_HANDLE ;

    if( !pSrc )
    {
        return NULL ;
    }
    else
    {
        //  Has a previous sibling?
        if( NULL == (tvi.hItem = TreeView_GetPrevSibling( m_wndTree, (HTREEITEM)*pSrc )) )
            //  No; has a parent?
            tvi.hItem = TreeView_GetParent( m_wndTree, (HTREEITEM)*pSrc ) ;
    }

    if( NULL != tvi.hItem &&
        TreeView_GetItem( m_wndTree, &tvi ) )
        return (CPropertyTreeItem*)tvi.lParam ;

    return NULL ;
}
#endif

//-------------------------------------------------------------------------//
//  Assigns selection focus to the first read-write property
CPropertyTreeItem* CPropertyTreeCtl::SetDefaultSelection()
{
    CPropertyTreeItem* pItem ;

    for( pItem = GetNextTreeItem( NULL, FALSE );
         NULL != pItem ;
         pItem = GetNextTreeItem( pItem, FALSE ) )
    {
        if( PIT_PROPERTY == pItem->Type() &&
            /* pItem->GetAccess() & PTIA_WRITE && */
            TreeView_SelectItem( m_wndTree, (HTREEITEM)*pItem ) )

            return (CProperty*)pItem ;
    }
    return NULL ;
}

