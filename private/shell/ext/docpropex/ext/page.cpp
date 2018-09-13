//-------------------------------------------------------------------------//
// page.cpp : property page impl(s)
//-------------------------------------------------------------------------//

#include "pch.h"
#include "ext.h"
#include "page.h"
#include "proptree.h"

//-------------------------------------------------------------------------//
//  Help constants
const TCHAR szHELPFILE[] = TEXT("imgmgt.hlp");

#define IDH_ADVANCED_DLG    100
#define IDH_TITLE           101
#define IDH_SUBJECT         102
#define IDH_CATEGORY        103
#define IDH_AUTHOR          104
#define IDH_ADVANCED        105
#define IDH_KEYWORDS        106
#define IDH_COMMENTS        107
#define IDH_SIMPLE          108

static DWORD rgdwPage0Help[] =
{
    IDC_TITLE,          IDH_TITLE,
    IDC_TITLE_LABEL,    IDH_TITLE,
    IDC_SUBJECT,        IDH_SUBJECT,
    IDC_SUBJECT_LABEL,  IDH_SUBJECT,
    IDC_CATEGORY,       IDH_CATEGORY,
    IDC_CATEGORY_LABEL, IDH_CATEGORY,
    IDC_AUTHOR,         IDH_AUTHOR,
    IDC_AUTHOR_LABEL,   IDH_AUTHOR,
    IDC_KEYWORDS,       IDH_KEYWORDS,
    IDC_KEYWORDS_LABEL, IDH_KEYWORDS,
    IDC_COMMENTS,       IDH_COMMENTS,
    IDC_COMMENTS_LABEL, IDH_COMMENTS,
    IDC_ADVANCED,       IDH_ADVANCED, 
    IDC_SIMPLE,         IDH_SIMPLE,
    0,                  0
};

TCHAR CPropEditCtl::m_szCompositeMismatch[] = TEXT("\0");  // "(multiple values)"

//-------------------------------------------------------------------------//
// CPropEditCtl impl
//-------------------------------------------------------------------------//
CPropEditCtl::CPropEditCtl() \
    :   m_bReadOnly(FALSE),
        m_fCompositeMismatch(FALSE),
        m_nIDLabel(0)
{
    if( !m_szCompositeMismatch[0] )
        LoadString( _Module.GetModuleInstance(), IDS_COMPOSITE_MISMATCH,
                    m_szCompositeMismatch, sizeof(m_szCompositeMismatch)/sizeof(TCHAR) );
};

//-------------------------------------------------------------------------//
BOOL CPropEditCtl::SubclassWindow( HWND hwnd, UINT nIDLabel )
{
    if( CWindowImpl<CPropEditCtl>::SubclassWindow( hwnd ) )
    {
        m_nIDLabel = nIDLabel;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CPropEditCtl::ShowWindow( UINT nShowCmd )
{
    BOOL bRet = CWindowImpl<CPropEditCtl>::ShowWindow( nShowCmd );

    HWND hwndLabel = ::GetDlgItem( GetParent(), m_nIDLabel );
    if( hwndLabel )
        ::ShowWindow( hwndLabel, nShowCmd );

    return bRet;
}

//-------------------------------------------------------------------------//
void CPropEditCtl::SetCompositeMismatch( BOOL fCompositeMismatch, BOOL bRedraw )
{
    m_fCompositeMismatch = fCompositeMismatch;
    if( bRedraw )
        InvalidateRect( NULL );
}

//-------------------------------------------------------------------------//
LRESULT CPropEditCtl::OnPaint( 
    UINT nMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL&  bHandled )
{
    if( !m_fCompositeMismatch )
        return DefWindowProc( nMsg, wParam, lParam );

    PAINTSTRUCT ps;
    HDC         hdc;
    if( NULL != (hdc = BeginPaint( &ps )) )
    {
        PaintCompositeMismatch( hdc );
        EndPaint( &ps );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
void CPropEditCtl::PaintCompositeMismatch( HDC hdc )
{
    ASSERT( m_fCompositeMismatch );
    ASSERT( hdc );

    RECT     rc;
    GetClientRect( &rc );

    HFONT    hf = (HFONT)SendMessage( WM_GETFONT, 0, 0L ),
             hfOld = (HFONT)SelectObject( hdc, hf );

    COLORREF rgbText = SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) ),
             rgbBack = SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );

    FillRect( hdc, &rc, (HBRUSH)(COLOR_WINDOW+1) );
    
    InflateRect( &rc, -1, -1 );
    DrawText( hdc, m_szCompositeMismatch, lstrlen( m_szCompositeMismatch ), &rc,
              DT_TOP|DT_SINGLELINE );
    
    SetBkColor( hdc, rgbBack );
    SetTextColor( hdc, rgbText );
    SelectObject( hdc, hfOld );
}

//-------------------------------------------------------------------------//
LRESULT CPropEditCtl::OnKeyDown( 
    UINT nMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL&  bHandled )
{
    LRESULT lRet = 0L;

    if( m_bReadOnly )
    {
        // let directional nav keys pass through,
        // eat everything else
        switch( wParam )
        {
            case VK_UP: case VK_DOWN:
            case VK_LEFT: case VK_RIGHT:
            case VK_HOME: case VK_END:
            case VK_PRIOR: case VK_NEXT:
                lRet = DefWindowProc(nMsg, wParam, lParam );
        }
    }
    else
        lRet = DefWindowProc(nMsg, wParam, lParam );

    if( m_fCompositeMismatch )
        InvalidateRect( NULL, FALSE );
        
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CPropEditCtl::OnChar( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( !m_bReadOnly )
        return DefWindowProc( nMsg, wParam, lParam );
    return 0L; // eat it.
}

//-------------------------------------------------------------------------//
LRESULT CPropEditCtl::OnLButton( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = DefWindowProc( uMsg, wParam, lParam );

    if( m_fCompositeMismatch && GetCapture() == m_hWnd )
    {
        InvalidateRect(NULL, FALSE);
        UpdateWindow();
    }
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CPropEditCtl::OnMouseMove( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = DefWindowProc( uMsg, wParam, lParam );
    if( m_fCompositeMismatch && GetCapture() == m_hWnd )
    {
        InvalidateRect(NULL, FALSE);
        UpdateWindow();
    }
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CPropEditCtl::OnSetReadOnly( UINT, WPARAM wParam, LPARAM, BOOL& bHandled )
{
    m_bReadOnly = (BOOL)wParam;
    bHandled = TRUE;
    return 1L;
}

//-------------------------------------------------------------------------//
// CPage0 impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CPage0::CPage0()
    :   m_pExt( NULL ),
        m_hPage(NULL),
        m_fInitializing(FALSE),
        m_cWinHelp(0),
        m_pIPropTree(NULL),
        m_hwndSite(NULL),
        m_hwndTree(NULL),
                m_cAdvDirty(0),
        m_pAdvDlg(NULL),
        m_hbrComposite(NULL),
        m_fAdvanced(FALSE),
        m_nSelectedMode(UIMODE_NONE)
{
    memset( &m_psp, 0, sizeof(m_psp) );
    m_psp.dwSize       = sizeof(m_psp);
    m_psp.dwFlags      = PSP_USECALLBACK;
    m_psp.hInstance    = _Module.GetResourceInstance();
    m_psp.pszTemplate  = MAKEINTRESOURCE( IDD );
    m_psp.pfnDlgProc   = (DLGPROC)StartDialogProc; // ATL dlg bootstrap
    m_psp.lParam       = (LPARAM)this;
    m_psp.pfnCallback  = PageCallback;
}

//-------------------------------------------------------------------------//
CPage0::~CPage0()
{
    Detach();
}

//-------------------------------------------------------------------------//
HRESULT CPage0::Add( CShellExt* pExt, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam )
{
    //  Stash pointer to hosting object
    Attach( pExt );

    //  Create Win32 property page, call client's page installation entrypoint.
    m_hPage = CreatePropertySheetPage( &m_psp );

    if( m_hPage == NULL || !lpfnAddPage( m_hPage, lParam ) )
    {
        m_hPage = NULL;
        Detach();
        return E_FAIL;
    }

    return S_OK;
}

//-------------------------------------------------------------------------//
//  Page attaches to shell extension object and bumps its ref count
void CPage0::Attach( CShellExt* pExt )
{
    ASSERT( pExt && !m_pExt );
    m_pExt = pExt;
    CreateCompositeMismatchBrush();
}

//-------------------------------------------------------------------------//
//  Page detaches from shell extension object and decrements its ref count
void CPage0::Detach( )
{
    if( m_pExt )
    {
        ((IShellExtInit*)m_pExt)->Release(); // AddRef() in IShellExtInit::Initialize()
        m_pExt = NULL;
    }

    if( m_pIPropTree )
    {
        m_pIPropTree->Release();
        m_pIPropTree = NULL;
        m_hwndTree = NULL;
    }

    if( m_hbrComposite )
    {
        DeleteObject( m_hbrComposite );
        m_hbrComposite = NULL;
    }
}

//-------------------------------------------------------------------------//
//  property sheet page callback
UINT CPage0::PageCallback( 
    HWND hwnd, 
    UINT uMsg, 
    LPPROPSHEETPAGE ppsp )
{
    CPage0*  pPage;
    UINT           uRet = 0;
    
    if( (pPage = (CPage0*)ppsp->lParam) != NULL ) 
    {
        if( PSPCB_CREATE == uMsg )
        {
            //  Initialize ATL window thunk block in preparation for
            //  window creation
	        _Module.AddCreateWndData( &pPage->m_thunk.cd, pPage );
            uRet = TRUE;
        }
        //  Auto-delete if we're being released
        else if( PSPCB_RELEASE == uMsg )
        {
            pPage->m_hPage = NULL;
            delete pPage;
        }
    }

    return uRet;
}

//-------------------------------------------------------------------------//
//  WM_DESTROY handler
LRESULT CPage0::OnDestroy(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    //  Close down help.
    if( m_cWinHelp > 0 )
    {
        ::WinHelp( m_hWnd, szHELPFILE, HELP_QUIT, 0L );
        m_cWinHelp = 0;
    }

    bHandled = FALSE;  // perit default processing.
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_INITDIALOG handler
LRESULT CPage0::OnInitDialog(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    //  Assign cached property values to UI.
    m_fInitializing = TRUE;
    m_pExt->UncacheUIValues( m_hWnd );

    //  Retrieve starting UI mode.
    //  If we don't have an advanced property src(s), then we'll force
    //  simple mode and hide the 'advanced' button.
    BOOL bHasAdvanced = m_pExt->HasAdvancedProperties();
    BOOL bHasBasic    = m_pExt->HasBasicProperties();

    ASSERT( bHasBasic || bHasAdvanced );   // if neither is true, we shouldn't have created the page.
    
    if( (m_fAdvanced = bHasAdvanced) == TRUE )
        RecallMode();
    
    Toggle( m_fAdvanced );

    ::ShowWindow( GetDlgItem( IDC_ADVANCED ), !m_fAdvanced && bHasAdvanced ? SW_SHOW : SW_HIDE );
    ::ShowWindow( GetDlgItem( IDC_SIMPLE ),    m_fAdvanced && bHasBasic ? SW_SHOW : SW_HIDE );

    m_fInitializing = FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------//
//  WM_SIZE handler
LRESULT CPage0::OnSize(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    POINTS pts = MAKEPOINTS( lParam );
    PositionControls( UIMODE_EITHER, pts.x, pts.y );    
    return 0L;
}

//-------------------------------------------------------------------------//
void CPage0::PositionControls( UIMODE mode, int cx, int cy )
{
    const SIZE sizeMargin = { 6, 6 };
    RECT  rcBtn, rcSibling;
    POINT ptBtn;
    HWND  hwndBtn, hwndSibling;

    //  Handle unspecified client dimensions.
    if( cx < 0 || cy < 0 )
    {
        RECT rc;
        GetClientRect( &rc );
        cx = RECTWIDTH( &rc );
        cy = RECTHEIGHT( &rc );
    }

    //  Reposition advanced mode controls
    if( (UIMODE_ADVANCED == mode || UIMODE_EITHER == mode) && 
        IsWindow( m_hwndSite ) )
    {
        //  Place "<< Simple" button and resize tree control.

        hwndBtn = GetDlgItem( IDC_SIMPLE );
        hwndSibling = m_hwndSite;  // ochost
        GetClientRect( &rcSibling );
        ::GetWindowRect( hwndBtn, &rcBtn );
        ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcBtn, 2 );
    
        //  property tree host is sized to page, then adjusted to 
        //  make room for 'Simple' in lower-right corner
        ptBtn.x = rcSibling.right - (sizeMargin.cx + RECTWIDTH(&rcBtn)),
        ptBtn.y = rcSibling.bottom- (sizeMargin.cy + RECTHEIGHT(&rcBtn)),
        InflateRect( &rcSibling, -sizeMargin.cx, -sizeMargin.cy );
        rcSibling.bottom -= (RECTHEIGHT(&rcBtn) + sizeMargin.cy);

        ::SetWindowPos( hwndSibling, NULL, 
                        rcSibling.left, rcSibling.top, 
                        RECTWIDTH(&rcSibling), RECTHEIGHT(&rcSibling),
                        SWP_NOZORDER|SWP_NOACTIVATE );

        ::SetWindowPos( hwndBtn, hwndSibling, ptBtn.x, ptBtn.y, 0, 0,
                        SWP_NOSIZE|SWP_NOACTIVATE );
    }
}

//-------------------------------------------------------------------------//
//  Initializes Basic mode
void CPage0::SetBasicMode( BOOL fBasic )
{
    HWND hwndCtl;
    BOOL bInitial = NULL == m_edit[0].m_hWnd;
    BOOL bReadOnly = m_pExt->ReadOnlyCount() > 0;
    int  cSources  = m_pExt->FileList().Count();
    int  i;
        
    if( bInitial )  // once!
    {
        m_edit[BASICPROP_TITLE].SubclassWindow( GetDlgItem( IDC_TITLE ), IDC_TITLE_LABEL );
        m_edit[BASICPROP_SUBJECT].SubclassWindow( GetDlgItem( IDC_SUBJECT ), IDC_SUBJECT_LABEL );
        m_edit[BASICPROP_CATEGORY].SubclassWindow( GetDlgItem( IDC_CATEGORY ), IDC_CATEGORY_LABEL );
        m_edit[BASICPROP_AUTHOR].SubclassWindow( GetDlgItem( IDC_AUTHOR ), IDC_AUTHOR_LABEL );
        m_edit[BASICPROP_KEYWORDS].SubclassWindow( GetDlgItem( IDC_KEYWORDS ), IDC_KEYWORDS_LABEL );
        m_edit[BASICPROP_COMMENTS].SubclassWindow( GetDlgItem( IDC_COMMENTS ), IDC_COMMENTS_LABEL );

        InitCtlDisplay( IDC_TITLE );
        InitCtlDisplay( IDC_SUBJECT );
        InitCtlDisplay( IDC_CATEGORY );
        InitCtlDisplay( IDC_AUTHOR );
        InitCtlDisplay( IDC_KEYWORDS );
        InitCtlDisplay( IDC_COMMENTS );

        for( i = 0; i<BASICPROP_COUNT; i++ )
        {
            m_edit[i].SendMessage( EM_LIMITTEXT, CCH_PROPERTYTEXT_MAX-1, 0L );
            m_edit[i].SendMessage( EM_SETREADONLY, bReadOnly, 0L );
        }
    }

    PositionControls( UIMODE_BASIC, -1, -1 );
    
    for( i = 0; i<BASICPROP_COUNT; i++ )
    {
        if( m_edit[i].m_hWnd )
            m_edit[i].ShowWindow( fBasic ? SW_SHOW : SW_HIDE );
    }

    ::ShowWindow( GetDlgItem( IDC_SEP1 ), fBasic ? SW_SHOW : SW_HIDE );
    ::ShowWindow( GetDlgItem( IDC_READONLY_SINGLE ),
                  fBasic && bReadOnly && cSources == 1 ? SW_SHOW : SW_HIDE );
    ::ShowWindow( GetDlgItem( IDC_READONLY_MULTIPLE ),
                  fBasic && bReadOnly && cSources > 1 ? SW_SHOW : SW_HIDE );

    if( NULL != (hwndCtl = GetDlgItem( IDC_ADVANCED )) )
        ::EnableWindow( hwndCtl, m_pExt->HasAdvancedProperties() );
}

//-------------------------------------------------------------------------//
//  Creates the property tree control for advanced mode.
HRESULT CPage0::CreateAdvancedPropertyUI()
{
    LPOLESTR pwszCaption;
    LRESULT  lRet = TRUE;
    HRESULT  hr = E_FAIL;

    if( NULL == m_hwndSite )
    {
        ASSERT( m_pIPropTree == NULL );
        ASSERT( !IsWindow( m_hwndTree ) );
    }
    else
    {
        ASSERT( m_pIPropTree != NULL );
        ASSERT( IsWindow( m_hwndTree ) );
        return S_OK;
    }

    //  Register necessary window class(es)
    SHDRC shdrc; 
    shdrc.cbSize  = sizeof(shdrc);
    shdrc.dwFlags = SHDRCF_OCHOST;

    if( DllRegisterWindowClasses( &shdrc ) )
    {
        //  Create OCHost window with caption as the CLSID of the Property Tree control we want hosted.
        if( SUCCEEDED( StringFromCLSID( CLSID_PropertyTreeCtl, &pwszCaption ) ) )
        {
            USES_CONVERSION;
            m_hwndSite = ::CreateWindow( OCHOST_CLASS, W2T(pwszCaption), 
                                         WS_CHILD|WS_TABSTOP|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 
                                         0,0,100,100,
                                         m_hWnd, (HMENU)IDC_CTL, 
                                         _Module.GetModuleInstance(), NULL );
            CoTaskMemFree( pwszCaption );
        }

        if( IsWindow( m_hwndSite ) )
        {
            //  Retrieve an IPropertyTreeCtl interface pointer 
            if( SUCCEEDED( (hr = OCHost_QueryInterface( m_hwndSite, IID_IPropertyTreeCtl, (PVOID*)&m_pIPropTree )) ) )
            {
                //   Enable event notification
                OCHost_EnableEvents( m_hwndSite, TRUE );

                //  Add property sources to control
                HANDLE  hEnum;
                BOOLEAN bEnum;
                TARGET  t;

                for( hEnum = m_pExt->FileList().EnumHead( t ), bEnum = TRUE; 
                     hEnum && bEnum;
                     bEnum = m_pExt->FileList().EnumNext( hEnum, t ) )
                {
                    m_pIPropTree->AddSource( &t.varFile, NULL, 0L );
#ifdef RESTORE_ACCESS_TIMES
                    _RestoreAccessTime( t ) ;
#endif RESTORE_ACCESS_TIMES
                }
                m_pExt->FileList().EndEnum( hEnum );

                LONG lTree = NULL;
                if( SUCCEEDED( (hr = m_pIPropTree->get_Window( &lTree )) ) )
                {
                    if( IsWindow( (HWND)lTree ) )
                        m_hwndTree = (HWND)lTree;                        
                    else
                        hr = E_FAIL;
                }
            }

            PositionControls( UIMODE_ADVANCED, -1, -1 );
        }
    }

#ifdef DEBUG
    if( SUCCEEDED(hr) )
    {
        ASSERT( IsWindow( m_hwndSite ) );
        ASSERT( IsWindow( m_hwndTree ) );
        ASSERT( m_pIPropTree != NULL );
    }
#endif//DEBUG

    return hr;
}

//-------------------------------------------------------------------------//
//  Initializes advanced mode
void CPage0::SetAdvancedMode( BOOL fAdvanced )
{
    if( !IsWindow( m_hwndSite ) && fAdvanced )
        if( FAILED( CreateAdvancedPropertyUI() ) )
            return ;

    if( fAdvanced )
    {
        ::ShowWindow( m_hwndSite, SW_SHOW );
        ::UpdateWindow( m_hwndSite );
    }
    else
        ::ShowWindow( m_hwndSite, SW_HIDE );
}

const TCHAR c_szEXTENSION_SETTINGS_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\PropSummary");
const TCHAR c_szUIMODE_REGVAL[]             = TEXT("Advanced");

//-------------------------------------------------------------------------//
//  Persists the UI mode settings for the page
void CPage0::PersistMode()
{
    HKEY hKey;
    DWORD dwDisposition;
    
    if( m_nSelectedMode != UIMODE_NONE )
    {
        if( RegCreateKeyEx( HKEY_CURRENT_USER, c_szEXTENSION_SETTINGS_REGKEY, 0L, NULL, 0L,
                            KEY_WRITE, NULL, &hKey, &dwDisposition ) == ERROR_SUCCESS )
        {
            ASSERT( dwDisposition == REG_OPENED_EXISTING_KEY ); \
                // if this asserts, someone changed the version-independent progID for
                // the extension object without changing szEXTENSION_SUBKEY.

            DWORD dwAdvanced = m_fAdvanced;
            RegSetValueEx( hKey, c_szUIMODE_REGVAL, 0L, REG_DWORD,
                           (LPBYTE)&dwAdvanced, sizeof(dwAdvanced) );

            RegCloseKey( hKey );
        }
    }
}

//-------------------------------------------------------------------------//
//  Retrieves the UI mode settings for the page
BOOL CPage0::RecallMode()
{
    HKEY hKey;
    BOOL bRet = FALSE;

    if( RegOpenKeyEx( HKEY_CURRENT_USER, c_szEXTENSION_SETTINGS_REGKEY, 
                      0L, KEY_READ, &hKey ) == ERROR_SUCCESS )
    {
        DWORD dwAdvanced = FALSE,
              cbVal = sizeof(dwAdvanced),
              dwType,
              dwErr;

        RegQueryValueEx( hKey, c_szUIMODE_REGVAL, NULL, &dwType,
                         (LPBYTE)&dwAdvanced, &cbVal );

        if( TRUE == dwAdvanced || FALSE == dwAdvanced )
        {
            m_fAdvanced = dwAdvanced;
            bRet = TRUE;
        }

        RegCloseKey( hKey );
    }
    return bRet;
}

//-------------------------------------------------------------------------//
void CPage0::InitCtlDisplay( UINT nIDC, IN OPTIONAL BASICPROPERTY* pnProp )
{
    BASICPROPERTY nProp;
    if( NULL == pnProp )
    {
        if( !m_pExt->GetBasicPropFromIDC( nIDC, &nProp ) )
            return;
        pnProp = &nProp;
    }
    ASSERT( pnProp );
    
    if( m_pExt->IsCompositeMismatch( *pnProp ) && !IsDirty( *pnProp ) )
    {
        BOOL fInitializing = m_fInitializing;
        
        m_fInitializing = TRUE;
        m_edit[*pnProp].SetCompositeMismatch( TRUE, FALSE );
        m_fInitializing = fInitializing;
    }
}

//-------------------------------------------------------------------------//
HBRUSH CPage0::CreateCompositeMismatchBrush( BOOL bRecreate )
{
    if( bRecreate && NULL != m_hbrComposite )
    {
        DeleteObject( m_hbrComposite );
        m_hbrComposite = NULL;
    }

    if( NULL == m_hbrComposite )
        m_hbrComposite = CreateSolidBrush( GetSysColor( COLOR_WINDOW ) );

    return m_hbrComposite;

}


//-------------------------------------------------------------------------//
//  WM_WININICHANGE handler
LRESULT CPage0::OnWinIniChange(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    CreateCompositeMismatchBrush( TRUE );
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_HELP handler
LRESULT CPage0::OnHelp(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    HWND hwndTarget = (HWND)((LPHELPINFO)lParam)->hItemHandle;

    if( hwndTarget == m_hwndSite || ::IsChild( m_hwndSite, hwndTarget ) )
    {
        if( ::WinHelp( m_hwndSite, szHELPFILE, 
                       HELP_CONTEXTPOPUP, IDH_ADVANCED_DLG ) )
            m_cWinHelp++;
    }
    else
    if( ::WinHelp( hwndTarget, szHELPFILE, 
                    HELP_WM_HELP, (DWORD_PTR)rgdwPage0Help ) )
        m_cWinHelp++;
       
    return TRUE;
}

//  EnumChildProc
BOOL HelpEnumProc( HWND hwnd, LPARAM lParam )
{
    PROPTREE_HELPTOPICS* piht = (PROPTREE_HELPTOPICS*)lParam;
    if( piht->iTopic < (MAX_PROPTREE_CHILDREN-1) )
    {
        piht->rgTopics[piht->iTopic * 2] = GetDlgCtrlID( hwnd );
        piht->rgTopics[(piht->iTopic * 2) + 1] = IDH_ADVANCED_DLG;
        piht->iTopic++;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_CONTEXTMENU handler.
LRESULT CPage0::OnContextMenu(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    if( (HWND)wParam == m_hwndSite || ::IsChild( m_hwndSite, (HWND)wParam ) )
    {
        //  roll an array of ctlID - topicID pairs for the 
        //  property tree control on the fly...
        ZeroMemory( &m_iht, sizeof(m_iht) );
        HelpEnumProc( m_hwndSite, (LPARAM)&m_iht );
        EnumChildWindows( m_hwndSite, HelpEnumProc, (LPARAM)&m_iht );

        if( ::WinHelp( m_hwndSite, szHELPFILE, 
                       HELP_CONTEXTMENU, (DWORD_PTR)m_iht.rgTopics ) )
            m_cWinHelp++;
    }
    else
    if( ::WinHelp( (HWND)wParam, szHELPFILE, HELP_CONTEXTMENU, (LPARAM)rgdwPage0Help) )
        m_cWinHelp++;

    return 0;    
}


//-------------------------------------------------------------------------//
//  WM_COMMAND:EN_CHANGE handler
LRESULT CPage0::OnEditChange( 
    WORD wCode,
    WORD wID,
    HWND hwndCtl,
    BOOL& bHandled )
{
    BASICPROPERTY nProp;

    if( !m_fInitializing && m_pExt->GetBasicPropFromIDC( wID, &nProp ) )
    {
        if( m_pExt->IsCompositeMismatch( nProp ) )
            m_edit[nProp].SetCompositeMismatch( FALSE, TRUE );

        //  Update some dirty flags
        m_pExt->SetDirty( nProp, TRUE );
        UpdateControls();
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_CTLCOLOREDIT handler
LRESULT CPage0::OnCtlColorEdit( 
    UINT,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled )
{
    HDC  hdcEdit  = (HDC)wParam;
    HWND hwndEdit = (HWND)lParam;
    BASICPROPERTY nProp;
    UINT nIDC = ::GetDlgCtrlID( hwndEdit );

    if( m_pExt->GetBasicPropFromIDC( nIDC, &nProp ) )
    {
        if( !IsDirty( nProp ) &&  m_pExt->IsCompositeMismatch( nProp ) )
            SetTextColor( hdcEdit, GetSysColor( COLOR_GRAYTEXT ) );
    }
    
    return (LRESULT)m_hbrComposite;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND:EN_SETFOCUS handler
LRESULT CPage0::OnEditSetFocus( 
    WORD wCode,
    WORD wID,
    HWND hwndCtl,
    BOOL& bHandled )
{
    BASICPROPERTY nProp;
    if( m_pExt->GetBasicPropFromIDC( wID, &nProp ) &&
        m_pExt->IsCompositeMismatch( nProp ) )
    {
        m_fInitializing = TRUE;
        m_pExt->UncacheUIValue( m_hWnd, nProp );
        m_fInitializing = FALSE;
    }

    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND:EN_KILLFOCUS handler
LRESULT CPage0::OnEditKillFocus( 
    WORD wCode,
    WORD wID,
    HWND hwndCtl,
    BOOL& bHandled )
{
    BASICPROPERTY nProp;

    if( m_pExt->GetBasicPropFromIDC( wID, &nProp ) )
    {
        m_pExt->CacheUIValue( m_hWnd, nProp );
        InitCtlDisplay( wID, &nProp );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND : IDC_ADVANCED handler
LRESULT CPage0::OnAdvanced( 
    WORD wCode,
    WORD wID,
    HWND hwndCtl,
    BOOL& bHandled )
{
    m_pExt->CacheUIValues( m_hWnd );
    Toggle( TRUE );
    m_nSelectedMode = UIMODE_ADVANCED;
    PersistMode();
    
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND : IDC_ADVANCED handler
LRESULT CPage0::OnBasic( 
    WORD wCode,
    WORD wID,
    HWND hwndCtl,
    BOOL& bHandled )
{
    Toggle( FALSE );
    m_nSelectedMode = UIMODE_BASIC;
    PersistMode();

    return 0L;
}

#ifdef _TESTDUMP_
//-------------------------------------------------------------------------//
BOOL Dump( BOOL fAdvanced )
{
    // Check registry for dump enable

    // Check mode


    if( fAdvanced )
    {
        //  Advanced mode dump

    }
    else
    {
        //  Simple mode dump

    }
}

#endif _TESTDUMP_

//-------------------------------------------------------------------------//
void CPage0::Toggle( BOOL fAdvanced )
{
    m_fAdvanced = fAdvanced;

    if( m_fAdvanced )
    {
        SetBasicMode( FALSE );
        if( !IsWindow( m_hwndSite ) )
            CreateAdvancedPropertyUI();
        UpdateAdvancedValues( TRUE );
        SetAdvancedMode( TRUE );
    }
    else
    {
        SetAdvancedMode( FALSE );
        UpdateAdvancedValues( FALSE );
        SetBasicMode( TRUE );
    }

#ifdef _TESTDUMP_
    Dump( !fAdvanced );
#endif _TESTDUMP_

    ::ShowWindow( GetDlgItem( IDC_ADVANCED ), !m_fAdvanced ? SW_SHOW : SW_HIDE );
    ::ShowWindow( GetDlgItem( IDC_SIMPLE ),    m_fAdvanced ? SW_SHOW : SW_HIDE );
}

//-------------------------------------------------------------------------//
HRESULT CPage0::UpdateAdvancedValue( ULONG nProp, BOOL bUpdateTree )
{
    HRESULT             hr  = E_UNEXPECTED;
    FMTID*              pFmtID = NULL;
    PROPID              propID = 0;
    VARTYPE             vt    = 0;
    int                 cchPageValue = 0;
    LPTSTR              pszPageValue = NULL;
    LPOLESTR            pwszFmtid;

    //  Do initial validation
    if( NULL == m_pIPropTree )
        return hr;

    if( !m_pExt->GetBasicPropInfo( (BASICPROPERTY)nProp, pFmtID, propID, vt ) )
        return hr;

    //  Retrieve the page's value text for the indicated property
    cchPageValue = m_pExt->GetMaxPropertyTextLength( m_hWnd ) + 1;
    if( NULL == (pszPageValue = new TCHAR[cchPageValue]) )
        return E_OUTOFMEMORY;
    m_pExt->GetPropertyText( m_hWnd, (BASICPROPERTY)nProp, pszPageValue, cchPageValue );

    //  Create a BSTR from the FMTID for the property,
    if( SUCCEEDED( (hr = StringFromCLSID( *pFmtID, &pwszFmtid )) ) )
    {
        BSTR bstrFmtid;
        if( NULL != (bstrFmtid = SysAllocString( pwszFmtid )) )
        {
            USES_CONVERSION;

            if( bUpdateTree )
            {
                BSTR bstrPageValue  = SysAllocString( T2W(pszPageValue) );

                //  Assign the tree's value for the property
                hr = m_pIPropTree->SetPropertyValue( bstrFmtid, propID, vt, bstrPageValue, TRUE );
                
                if( bstrPageValue )
                    SysFreeString( bstrPageValue );
            }
            else
            {
                //  Retrieve the tree's value for the property and assign
                //  to the page.
                BSTR bstrTreeValue = NULL;
                VARIANT_BOOL bDirty = FALSE;
                
                if( SUCCEEDED( (hr = m_pIPropTree->GetPropertyValue( bstrFmtid, propID, vt, 
                                                             &bstrTreeValue, &bDirty )) ) )
                {
                    BOOL bChanged = bstrTreeValue && pszPageValue ? 
                                        0 != lstrcmp( W2T(bstrTreeValue), pszPageValue ) : 
                                    (EMPTY_STRING( bstrTreeValue ) && EMPTY_STRING( pszPageValue )) ?
                                        FALSE : TRUE;
                
                    if( bChanged )
                        SetBasicPropertyText( (BASICPROPERTY)nProp, W2T(bstrTreeValue) );

                    if( bstrTreeValue )
                        SysFreeString( bstrTreeValue );
                }
            }
            
            SysFreeString( bstrFmtid );
        }
        else
            hr = E_OUTOFMEMORY;

        CoTaskMemFree( pwszFmtid );
    }

    delete pszPageValue;

    return hr;
}

//-------------------------------------------------------------------------//
void CPage0::SetBasicPropertyText( BASICPROPERTY nProp, LPCTSTR pszText )
{
    ASSERT(m_pExt != NULL);
    m_fInitializing = TRUE; // disable EN_CHANGE handling
    m_pExt->SetPropertyText( m_hWnd, nProp, pszText );
    m_fInitializing = FALSE; // enable EN_CHANGE handling
}

//-------------------------------------------------------------------------//
void CPage0::UpdateAdvancedValues( BOOL bUpdateTree )
{
    for( int i=0; i<BASICPROP_COUNT; i++ )
    {
        if( bUpdateTree == FALSE || IsDirty( (BASICPROPERTY)i )  )
            UpdateAdvancedValue( (BASICPROPERTY)i, bUpdateTree );
    }
}

//-------------------------------------------------------------------------//
//  WM_NOTIFY : OCN_OCEVENT handler.
LRESULT CPage0::OnCtlEvent(
    int nID,
    LPNMHDR pNMH,
    BOOL& bHandled )
{
    ASSERT( IDC_CTL == nID );
    LPNMOCEVENT pe = (LPNMOCEVENT)pNMH;

    switch( pe->dispID )
    {
        case PTDISPID_PROPERTYDIRTY:
            ASSERT( m_pIPropTree );
            m_pIPropTree->get_DirtyCount( (LONG*)&m_cAdvDirty );
            UpdateControls();
            break;
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_NOTIFY : PSN_APPLY handler
LRESULT CPage0::OnApply(
    int nID,
    LPNMHDR pnmh,
    BOOL& bHandled )
{
    HRESULT       hr;
    BOOL          bAnyDirty       = FALSE,
                  bErrorDisplayed = FALSE,
                  bForceExit      = FALSE;
    ERROR_CONTEXT errctx = ((LPPSHNOTIFY)pnmh)->lParam ? 
                           ERRCTX_PERSIST_OK : ERRCTX_PERSIST_APPLY;

    //  if we're displaying advanced, transfer values to basic UI;
    //  if we're displaying basic UI, transfer values to advanced UI.
    UpdateAdvancedValues( !m_fAdvanced );

    //  Persist advanced properties via ctl.
    if( m_pIPropTree )
    {
        m_pIPropTree->get_DirtyCount( (LONG*)&m_cAdvDirty );

        if( m_cAdvDirty )
        {
            bAnyDirty = TRUE;
            hr = m_pIPropTree->Apply();
            m_pIPropTree->get_DirtyCount( (LONG*)&m_cAdvDirty );

            if( FAILED(hr) && !bErrorDisplayed)
            {
                if( IDOK == DisplayError( errctx, hr ) && ERRCTX_PERSIST_OK == errctx )
                    bForceExit = TRUE;

                bErrorDisplayed = TRUE;
            }
        }
    }

    //  Persist lite properties.
    if( m_pExt->IsDirty() )
    {
        bAnyDirty = TRUE;
        m_pExt->CacheUIValues( m_hWnd );
        hr = m_pExt->Persist();

        if( FAILED( hr ) && !bErrorDisplayed )
        {
            if( IDOK == DisplayError( errctx, hr ) && ERRCTX_PERSIST_OK == errctx )
                bForceExit = TRUE;

            bErrorDisplayed = TRUE;
        }
    }

    if( bForceExit )
    {
        //  Dismiss the sheet.
        return PSNRET_NOERROR;
    }

    UpdateControls();

    if( bErrorDisplayed )
    {
        //  Disable Apply button. (see NT raid#: 253327).
        ::PostMessage( GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0L );
        ::PostMessage( GetParent(), PSM_SETCURSEL, (WPARAM)-1, (LPARAM)m_hPage );
        return PSNRET_INVALID;
    }

    LRESULT lRet = (FALSE == m_pExt->IsDirty() && (0 == m_cAdvDirty)) ? 
                        PSNRET_NOERROR : PSNRET_INVALID;

    if( bAnyDirty && PSNRET_NOERROR == lRet )
        m_pExt->ChangeNotify( SHCNE_UPDATEITEM );

    return lRet;
}

//-------------------------------------------------------------------------//
void CPage0::UpdateControls()
{
    //  Update the 'Apply' button
    ::PostMessage( GetParent(), 
                   IsDirty() ? PSM_CHANGED : PSM_UNCHANGED, 
                   (WPARAM)m_hWnd, 
                   0L );            
}
