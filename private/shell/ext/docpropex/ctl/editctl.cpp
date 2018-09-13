//-------------------------------------------------------------------------//
//
//  EditCtl.cpp
//
//-------------------------------------------------------------------------//

#include "pch.h"
#include "PropVar.h"
#include "EditCtl.h"

//-------------------------------------------------------------------------//
const TCHAR  szEDITCLASS[]           = TEXT("Edit"),
             szBUTTONCLASS[]         = TEXT("Button"),
             szCOMBOBOXCLASS[]       = TEXT("Combobox"),
             szDROPWINDOWCLASS[]     = TEXT("DropWindow");

//-------------------------------------------------------------------------//
void GetNcBorderSize( HWND hwnd, SIZE* pSize )
{
    ASSERT( hwnd );
    ASSERT( pSize );
    RECT rcWnd, rcClient;
    
    ::GetWindowRect( hwnd, &rcWnd );
    ::GetClientRect( hwnd, &rcClient );

    pSize->cx = RECTWIDTH( &rcWnd ) - RECTWIDTH( &rcClient );
    pSize->cy = RECTHEIGHT( &rcWnd ) - RECTHEIGHT( &rcClient );
}

//-------------------------------------------------------------------------//
//  Formats in place a buffer containing multiline text to
//  a form suitable for displaying as a single line.
void MakeSingleLine( LPTSTR pszText, int cchText, int cchBuf )
{
    LPCTSTR pszEllipsis = TEXT(" ...");
    ASSERT( cchBuf - cchText > lstrlen(pszEllipsis) );

    if( pszText && *pszText )    {
        for( int i = 0; i<cchText; i++ )   {
            if( pszText[i]==TEXT('\r') || pszText[i]==TEXT('\n') )
            {
                lstrcpy( &pszText[i], pszEllipsis );
                return;
            }
        }
    }
}

//-------------------------------------------------------------------------//
//  CInPlaceBase - base class for in-place edit controls.
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
typedef struct tagENUMCHILD
{
    CInPlaceBase* pThis;
    BOOL          bSubclassed;
} ENUMCHILD, *PENUMCHILD;

//-------------------------------------------------------------------------//
BOOL CInPlaceBase::enumChildProc( HWND hwnd, LPARAM lParam )
{
    PENUMCHILD pEC = (PENUMCHILD)lParam;

    if( (pEC->bSubclassed = pEC->pThis->subclassCtl( hwnd )) )
        return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CInPlaceBase::subclassCtl( HWND hwnd )
{
    TCHAR szBuf[128];
    *szBuf = 0;

    if( GetClassName( hwnd, szBuf, sizeof(szBuf)/sizeof(TCHAR) ) )
    {
        if( lstrcmpi( szBuf, TEXT("Edit") )==0 )
        {
            if( CWindowImpl<CInPlaceBase>::SubclassWindow( hwnd ) )
            {
                m_type = CT_EDIT;
                return TRUE;
            }
        }

        if( lstrcmpi( szBuf, TEXT("ComboBox") )==0 )
        {
            //  BUGBUG: Sublassing a combo box results in a crash
            //  when it is repositioned.  Why?  We need to sublass
            //  the droplist type to process directional keys!  The
            //  code is commented out here; but need to get this working.
            DWORD dwStyle = ::GetWindowLong( hwnd, GWL_STYLE );
            if( (dwStyle & 0xF)==CBS_DROPDOWNLIST /*&&
                CWindowImpl<CInPlaceBase>::SubclassWindow( hwnd )*/ )
            {
                //TRACE( TEXT("Sublassing CBS_DROPDOWNLIST combo\n") );
                m_type = CT_CBDROPLIST;
                return TRUE;
            }
            else if( (dwStyle & 0xF)==CBS_DROPDOWN )
            {
                m_type = CT_CBDROPDOWN;
            }
        }
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CInPlaceBase::SubclassWindow( HWND hwnd, BOOL bSearchChildren ) 
{
    if( !( subclassCtl( hwnd ) || bSearchChildren ) )
        return FALSE;

    ENUMCHILD ec;
    memset( &ec, 0, sizeof(ec) );
    ec.pThis = this;

    EnumChildWindows( hwnd, enumChildProc, (LPARAM)&ec );
    return ec.bSubclassed;
}

//-------------------------------------------------------------------------//
//  Transmits a WM_COMMAND message to its source child in-place control
LRESULT CInPlaceBase::ReflectWM_COMMAND( HWND hwndCtl, UINT nCode, BOOL* pbHandled )
{
    *pbHandled = FALSE;
    return ::SendMessage( hwndCtl, WMU_SELFCOMMAND, (WPARAM)nCode, (LPARAM)pbHandled );
}

//-------------------------------------------------------------------------//
//  Transmits a WM_NOTIFY message back to its source child in-place control
LRESULT CInPlaceBase::ReflectWM_NOTIFY( NMHDR* pHdr, BOOL* pbHandled )
{
    REFLECTNOTIFY notify;
    *pbHandled = FALSE;
    notify.pbHandled = pbHandled;
    notify.pHdr      = pHdr;
    return ::SendMessage( pHdr->hwndFrom, WMU_SELFNOTIFY, 0L, (LPARAM)&notify );
}

//-------------------------------------------------------------------------//
//  WMU_SELFCOMMAND message handler
LRESULT CInPlaceBase::_selfCommand( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    BOOL* pbHandled = (BOOL*)lParam;
    bHandled = TRUE;

    *pbHandled = FALSE;
    return OnSelfCommand( (UINT)wParam, *pbHandled );
}

//-------------------------------------------------------------------------//
//  WMU_SELFNOTIFY message handler
LRESULT CInPlaceBase::_selfNotify( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = TRUE;
    REFLECTNOTIFY* pNotify = (REFLECTNOTIFY*)lParam;
    return OnSelfNotify( pNotify->pHdr, *pNotify->pbHandled );
}

//-------------------------------------------------------------------------//
//  Handles WM_COMMAND messages reflected back to control
LRESULT CInPlaceBase::OnSelfCommand( UINT nCode, BOOL& bHandled )
{
    bHandled = FALSE;
    return 0L;
}
//-------------------------------------------------------------------------//
//  Handles WM_NOTIFY messages reflected back to control
LRESULT CInPlaceBase::OnSelfNotify( NMHDR* pHdr, BOOL& bHandled )
{
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceBase::OnGetDlgCode( UINT nMsg, WPARAM virtKey, LPARAM lParam, BOOL& bHandled )
{
    if( 0L != lParam && 
        WM_KEYDOWN == ((LPMSG)lParam)->message && VK_TAB == ((LPMSG)lParam)->wParam )
    {
        bHandled = FALSE;
        return 0L;
    }
    
    return DLGC_WANTALLKEYS;
}

//-------------------------------------------------------------------------//
UINT CInPlaceBase::GetDropState() const
{
    if( m_hWnd && m_type == CT_EDIT )
    {
        HWND    hwndParent;
        TCHAR   szClass[64];
        if( (hwndParent = GetParent())!=NULL &&
            GetClassName( hwndParent, szClass, sizeof(szClass) ) &&
            lstrcmpi( szClass, szCOMBOBOXCLASS )==0 )
        {
            if( ::SendMessage( hwndParent, CB_GETDROPPEDSTATE, 0, 0L ) )
                return DROPSTATE_DROPPED;
        }
    }
    return DROPSTATE_CLOSED;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceBase::OnKey( UINT nMsg, WPARAM virtKey, LPARAM lParam, BOOL& bHandled )
{
    NAVIGATION_KEY_INFO nki;
    UINT nDropState = GetDropState();

    //  If we've subclassed a droplist combo box and its listbox is dropped, 
    //  we'll let the control's default wndproc handle all keys.
    if( nDropState & DROPSTATE_DROPPED )
    {
        bHandled = FALSE;
        return 0L;
    }

    //  Otherwise, defer to control window for processing
    InitNavigationKeyInfo( &nki, m_hWnd, nMsg, virtKey, lParam );

    LRESULT lRet = ::SendMessage( m_hwndTree, WMU_NAVIGATION_KEY, 
                                  GetDlgCtrlID(), (LPARAM)&nki );

    if( (bHandled = nki.bHandled) )
        return lRet;

    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceBase::OnFocusChange( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = DefWindowProc( nMsg, wParam, lParam );

    HWND hwndOther = (HWND)wParam;
    if( nMsg == WM_SETFOCUS || !IsChild( hwndOther ) )
        ::SendMessage( m_hwndTree, WMU_CTLFOCUS, wParam, (LPARAM)nMsg );
    
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceBase::OnGetObject( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HRESULT hr = E_INVALIDARG;
    bHandled = FALSE;

    if( OBJID_CLIENT == lParam )
    {
        IAccessible* paccProxy;
        if( SUCCEEDED( (hr = _CreateStdAccessibleProxy( m_hWnd, wParam, &paccProxy )) ) )
        {
            CAccessibleBase* pacc = new CAccessibleBase;
            if( pacc )
            {
                pacc->Initialize( m_hwndTree, paccProxy );
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
LRESULT CInPlaceBase::OnPostNcDestroy( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_hwndTree = NULL;
    LRESULT lRet = DefWindowProc( nMsg, wParam, lParam );
    bHandled = TRUE;
    delete this;
    return 0L;
}

//-------------------------------------------------------------------------//
//  CInPlaceDropList  (standard CBS_DROPLIST combobox)
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
BOOL CInPlaceDropList::SubclassWindow( HWND hwnd, BOOL bSearchChildren )
{
    //  Bypass base class's subclassing routine, because droplist combos
    //  have no embedded edit control.
    UNREFERENCED_PARAMETER( bSearchChildren );
    if( CWindowImpl< CInPlaceBase >::SubclassWindow( hwnd ) )   {
        m_type = CT_CBDROPLIST;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
UINT CInPlaceDropList::GetDropState() const
{
    if( m_hWnd && ::SendMessage( m_hWnd, CB_GETDROPPEDSTATE, 0, 0L ) )
        return DROPSTATE_DROPPED;
    return DROPSTATE_CLOSED;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceDropList::OnKey( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( (bHandled = (BOOL)SendMessage( CB_GETDROPPEDSTATE, 0,0 )) )
        return DefWindowProc( nMsg, wParam, lParam );
    
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceDropList::OnSelfCommand( UINT nCode, BOOL& bHandled )
{
    return CInPlaceBase::OnSelfCommand( nCode, bHandled );
}


//-------------------------------------------------------------------------//
//  CCaptureTracker class implementation
//-------------------------------------------------------------------------//
UINT CCaptureTracker::m_registeredMsg = 0;

//-------------------------------------------------------------------------//
UINT CCaptureTracker::NotifyMsg()
{
    if( !m_registeredMsg )
    {
        m_registeredMsg = RegisterWindowMessage( TEXT("CaptureTrackerMsg") );
        ASSERT( m_registeredMsg!=NULL );
    }
    return m_registeredMsg;
}

//-------------------------------------------------------------------------//
BOOL CCaptureTracker::Track( HWND hwndChild, HWND hwndClient )
{
    ASSERT( m_hWnd==NULL );

    if( hwndChild == NULL )
        return FALSE;
    
    m_hwndClient = hwndClient;
    return CWindowImpl<CCaptureTracker>::SubclassWindow( hwndChild );
}

//-------------------------------------------------------------------------//
LRESULT CCaptureTracker::OnSomeButtonDown( 
    UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HWND hwndTarget;
    POINT  pt;
    POINTS pts = MAKEPOINTS( lParam );
    POINTSTOPOINT( pt, pts );

    MapWindowPoints( HWND_DESKTOP, &pt, 1 );
    if( (hwndTarget = WindowFromPoint( pt ))==NULL ||
        (hwndTarget != m_hwndClient && !::IsChild( m_hwndClient, hwndTarget )) )
    {
        //  Some other non-child window got clicked on
        NotifyClient( ForeignClick, hwndTarget );
    }
    
    bHandled = TRUE;
    return DefWindowProc( nMsg, wParam, lParam );
}

//-------------------------------------------------------------------------//
LRESULT CCaptureTracker::OnCaptureChanged( 
    UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = DefWindowProc( nMsg, wParam, lParam );
    bHandled = TRUE;

    HWND hwndCaptureOwner = (HWND)lParam;

    //  We've lost capture, so stop filtering messages
    UnsubclassWindow();

    //  Evaluate who we lost capture to, and act accordingly.
    if( hwndCaptureOwner == NULL )
    {
        //  the subclassed window called ReleaseCapture()
        //TRACE(TEXT("CaptureTracker: child released capture.\n"));
        AsyncNotifyClient( Released, NULL );
    }
    else if( ::IsChild( m_hwndClient, hwndCaptureOwner ) )
    {
        //  Some other child window captured the mouse;
        //  we now turn our attention to this window...
        //TRACE(TEXT("CaptureTracking shifted to child control %08lX\n"), hwndCaptureOwner);
        CWindowImpl<CCaptureTracker>::SubclassWindow( hwndCaptureOwner );
    }
    else
    {
        //  Some non-child window captured the mouse
        //TRACE(TEXT("CaptureTracker: non-child window has taken capture.\n"));
        AsyncNotifyClient( Lost, hwndCaptureOwner );
    }
    return lRet;
}

//-------------------------------------------------------------------------//
LRESULT CCaptureTracker::OnDestroy( 
    UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    UnsubclassWindow();
    NotifyClient( Released, NULL );

    BOOL bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  CInPlaceDropWindow - custom combo box look-alike window object
//-------------------------------------------------------------------------//
//  BUGBUG: need to replace drop down pushbutton with GDI-drawn button that
//  exactly mimics combo box button.

static const TCHAR  szCLOSED[] = TEXT("+"), // BUGBUG: needless if mimics combo box button (see above).
                    szOPEN[]   = TEXT("-");// BUGBUG: needless if mimics combo box button (see above).

static const UINT   IDC_DROPWINDOWCHILDBASE  = 4001,    // arbitrary
                    IDC_DROPBTN  = IDC_DROPWINDOWCHILDBASE + 1, // BUGBUG: needless if mimics combo box button (see above).
                    IDC_DROPHOST = IDC_DROPWINDOWCHILDBASE + 2;

static const UINT   DWHN_APPDEACTIVATE = 1;   // drop window notification code

//-------------------------------------------------------------------------//
CInPlaceDropWindow::CInPlaceDropWindow( HWND hwndTree )
    :   CInPlaceBase(hwndTree),
        m_fBtnPressed(FALSE),
        m_fDestroyed(FALSE),
        m_nDropState(DROPSTATE_CLOSED),
        m_nDropAnchor(DROPANCHOR_TOP),
        m_hfText(NULL),
        m_lParam(0L),
        m_cyDrop(150),
        m_bExtendedUI(FALSE),
        m_cy(21)
{
    SetRect( &m_rcBtn, 0,0,0,0 );
}

//-------------------------------------------------------------------------//
//  WM_CREATE handler.
LRESULT CInPlaceDropWindow::OnCreate( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
    bHandled = TRUE;

    //  Create the dropdown push button.
    //  BUGBUG: needless if mimics combo box button (see above).
    PositionControls( lpcs->cx, lpcs->cy );

    //  Calculate rectangle for drop host window.
    RECT rc;
    SetRect( &rc, lpcs->x, lpcs->y, lpcs->x + lpcs->cx, lpcs->x + lpcs->cx );
    OffsetRect( &rc, 0, RECTHEIGHT( &rc ) );
    rc.bottom += m_cyDrop;

    m_fDestroyed = FALSE;

    return 0;
}

//-------------------------------------------------------------------------//
//  WM_CREATE handler.
LRESULT CInPlaceDropWindow::OnDestroy( UINT, WPARAM, LPARAM, BOOL& )    
{ 
    m_fDestroyed = TRUE;

    HWND hwndDrop;
    if( NULL != (hwndDrop = DropHwnd()) )
        ::DestroyWindow( hwndDrop );

    return 0L; 
}

//-------------------------------------------------------------------------//
void CInPlaceDropWindow::OnClick( 
    HWND hwndTarget, 
    UINT uMsg, 
    const POINT& pt, 
    UINT nDropState )
{
    if( hwndTarget == m_hWnd  )
    {
        ClickButton();
        ToggleDrop( FALSE );
    }
    else if( IsChild( hwndTarget ) )
    {
        if( nDropState & DROPSTATE_CLOSED )
            ClickButton();
        ToggleDrop( 0 == (nDropState & DROPSTATE_CLOSED) );
    }
}

//-------------------------------------------------------------------------//
//  Handle mouse button down messages.  Note: we may or may not be the
//  focus window when this takes place; we may have mouse capture, so
//  we must be prepared to propagate the message to its true destination.
LRESULT CInPlaceDropWindow::OnLButtonDown( UINT nMsg, WPARAM nFlags, LPARAM lParam, BOOL& bHandled )
{
    HWND    hwndTarget = NULL,
            hwndDrop = NULL;

    POINT  pt;
    POINTS pts = MAKEPOINTS( lParam );
    POINTSTOPOINT( pt, pts );

    if( (hwndTarget = ChildWindowFromPoint( pt ))!=NULL )
    {
        if( hwndTarget == m_hWnd || IsChild( hwndTarget ) )
        {
            DefWindowProc( nMsg, nFlags, lParam );
            OnClick( hwndTarget, nMsg, pt, GetDropState() );
        }
        else
        {
            ::MapWindowPoints( HWND_DESKTOP, hwndTarget, &pt, 1 );
            ::SendMessage( hwndTarget, WM_LBUTTONDOWN, nFlags, MAKELPARAM( pt.x, pt.y ) );
        }
    }
    else 
    {
        MapWindowPoints( HWND_DESKTOP, &pt, 1 );
        if( (hwndTarget = WindowFromPoint( pt )) !=NULL )
        {
            if( (hwndDrop = DropHwnd())!=NULL )
            {
                if( hwndTarget != hwndDrop && !IsChildOfDrop( hwndTarget ) )
                    ShowDrop( FALSE, TRUE );
            }
            
            ::MapWindowPoints( HWND_DESKTOP, hwndTarget, &pt, 1 );
            ::PostMessage( hwndTarget, WM_LBUTTONDOWN, nFlags, MAKELPARAM( pt.x, pt.y ) );
        }
    }
    bHandled = FALSE;
    return 0L;
}


//-------------------------------------------------------------------------//
//  WM_SIZE handler.
LRESULT CInPlaceDropWindow::OnSize( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = CInPlaceDropWindow::DefWindowProc( nMsg, wParam, lParam );
    int     cx = LOWORD( lParam ),
            cy = HIWORD( lParam );
    
    PositionControls( cx, cy );

    bHandled = TRUE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_PAINT handler.
LRESULT CInPlaceDropWindow::OnPaint( 
    UINT nMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    PAINTSTRUCT ps;
    HDC         hdc;

    bHandled = FALSE;
    if( (hdc = BeginPaint( &ps ))!=NULL )
    {
        HWND        hwndFocus = ::GetFocus();
        BOOL        bFocus  = hwndFocus == m_hWnd || IsChild( hwndFocus );
        LPTSTR      pszText = NULL;
        RECT        rcText;

        //  Draw dropdown button
        DrawFrameControl( hdc, &m_rcBtn, DFC_SCROLL, 
                          DFCS_SCROLLDOWN | (m_fBtnPressed ? DFCS_PUSHED : 0) );
        
        if( DrawTextBox() )
        {
            COLORREF rgbBk   = SetBkColor(   hdc, GetSysColor( bFocus ? COLOR_HIGHLIGHT :     COLOR_WINDOW ) ), 
                     rgbText = SetTextColor( hdc, GetSysColor( bFocus ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT ) );
        
            //  Draw highlight box
            GetTextBox( &rcText );
            InflateRect( &rcText, -1, -1 );
            ExtTextOut( hdc, rcText.left, rcText.top, ETO_OPAQUE, &rcText, NULL, 0, NULL ); 

            //  Draw focus rectangle
            if( bFocus )
                DrawFocusRect( hdc, &rcText );

            //  Make a copy of the text
            int cch = GetWindowTextLength();
            if( (pszText = new TCHAR[cch+5])!=NULL ) 
            {
                *pszText = 0;
                GetWindowText( pszText, cch+1 );
            
                //  If multiline, prepare for display as single line.
                if( IsMultiline() )
                    MakeSingleLine( pszText, cch, cch+5 );

                if( *pszText )
                {
                    HFONT   hfOld = (HFONT)SelectObject( hdc, m_hfText );
                    InflateRect( &rcText, -1, -1 );
                    DrawText( hdc, pszText, lstrlen(pszText), &rcText, 
                              DT_END_ELLIPSIS|DT_NOPREFIX|DT_SINGLELINE|DT_TOP );
                    SelectObject( hdc, hfOld );
                }

                delete [] pszText;
            }

            SetBkColor( hdc, rgbBk );
            SetTextColor( hdc, rgbText );
        }

        EndPaint( &ps );
        bHandled = TRUE;
    }
    return 0L;
}
//-------------------------------------------------------------------------//
LRESULT CInPlaceDropWindow::OnSetFocus( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    Invalidate( FALSE );
    
    bHandled = FALSE;  // let CInPlaceBase have a crack
    return 0L;
}
//-------------------------------------------------------------------------//
LRESULT CInPlaceDropWindow::OnKillFocus( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    Invalidate( FALSE );
    
    bHandled = FALSE;  // let CInPlaceBase have a crack
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceDropWindow::OnShowWindow( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( wParam==FALSE )    
    {
        if( m_nDropState & DROPSTATE_DROPPED )
            ShowDrop( FALSE, TRUE );
    }
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  Handle keyboard input directly for drop down acceleration.
LRESULT CInPlaceDropWindow::OnKey( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //TRACE( TEXT("CInPlaceDropWindow keydown/up/char\n") );
    bHandled = FALSE;

    if( nMsg == WM_KEYDOWN && wParam==VK_F4 && !m_bExtendedUI )
    {
        ShowDrop( (m_nDropState & DROPSTATE_CLOSED)!=0, FALSE );
        bHandled = TRUE;
        return 0L;
    }

    bHandled = (m_nDropState & DROPSTATE_DROPPED) ? TRUE : FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  Enforce optimum control height.
LRESULT CInPlaceDropWindow::OnWindowPosChanging( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    WINDOWPOS* pWP = (WINDOWPOS*)lParam;
    if( (pWP->flags & SWP_NOSIZE)==0 && pWP->cy < m_cy )
    {
        if( (pWP->flags & SWP_NOMOVE)==0 )
            pWP->y -= (m_cy - pWP->cy)/2;
        
        pWP->cy = m_cy;
        bHandled = TRUE;
        return 0L;
    }

    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  When our app loses activation, close the drop window.
LRESULT CInPlaceDropWindow::OnActivateApp( 
    UINT nMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    bHandled = FALSE;
    
    if( wParam==FALSE )
        ShowDrop( FALSE, TRUE );

    return 0L;
}

//-------------------------------------------------------------------------//
//  ALT-Down and ALT-Up open and close the drop host window, resp.
LRESULT CInPlaceDropWindow::OnSysKeyDown( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( wParam==VK_DOWN || wParam==VK_UP )
    {
        ShowDrop( (m_nDropState & DROPSTATE_DROPPED)==0 ? TRUE : FALSE, FALSE );
        bHandled = TRUE;
        return 0L;
    }

    bHandled = FALSE;
    return 0L;
}
//-------------------------------------------------------------------------//
//  Intercept font assignment to calculate and enforce control height.
LRESULT CInPlaceDropWindow::OnSetFont( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    m_hfText = (HFONT)wParam;
    CalcHeight( NULL );
    AdjustSize();
    Invalidate( TRUE );
    
    bHandled = FALSE;    
    return 0L;
}

//-------------------------------------------------------------------------//
//  Intercept text assignment to calculate ideal control height.
LRESULT CInPlaceDropWindow::OnSetText( 
    UINT nMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled )
{
    CalcHeight( (LPCTSTR)lParam );
    Invalidate( TRUE );

    
    bHandled = FALSE;
    return 0L;
}
//-------------------------------------------------------------------------//
//  WM_COMMAND handler.
LRESULT CInPlaceDropWindow::OnCommand( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = FALSE;

    switch( LOWORD( wParam ) )
    {
        case IDC_DROPHOST:
            bHandled = TRUE;
            ShowDrop( FALSE, TRUE );
            return TRUE;
    }

    return 0L;
}
//-------------------------------------------------------------------------//
LRESULT CInPlaceDropWindow::OnSelfCommand( UINT nCode, BOOL& bHandled )
{
    return CInPlaceBase::OnSelfCommand( nCode, bHandled );
}

//-------------------------------------------------------------------------//
//  Shows or hides the control's drop down window, and updates related
//  state information.
//
//  BUGBUG: Need to adjust drop host rectangle to contain it
//          within virtual desktop window boundaries.
BOOL CInPlaceDropWindow::ShowDrop( BOOL bDrop, BOOL bCanceled )
{
    HWND hwndDrop = DropHwnd();
    RECT rc;

    memset( &rc, 0, sizeof(rc) );

    //  If we're dropping...
    if( bDrop && (m_nDropState & DROPSTATE_CLOSED)!=0 )
    {
        //  Calculate rectangle for drop host window.
        GetWindowRect( &rc );
        OffsetRect( &rc, 0, RECTHEIGHT( &rc ) );
        rc.bottom += m_cyDrop;
        m_nDropAnchor = DROPANCHOR_TOP;

        //  Allow derivatives a chance at modifiying the drop host and/or
        //  vetoing its display.
        bCanceled = FALSE;
        if( OnShowDrop( hwndDrop, TRUE, bCanceled, &rc ) )
        {
            m_nDropState = DROPSTATE_DROPPED;

            if( hwndDrop )
            {
                ::SetWindowPos( hwndDrop, HWND_TOPMOST, 
                                rc.left, rc.top, 
                                RECTWIDTH( &rc ), RECTHEIGHT( &rc ), 0 );
                AnimateDrop( hwndDrop );
            }

            Capture( TRUE );                    // take over the mouse
            PostNotifyCommand( CBN_DROPDOWN );  // notify our parent 
            return TRUE;
        }
        return FALSE;
    }
    
    else 
    //  If we're closing...
    if( bDrop==FALSE && (m_nDropState & DROPSTATE_DROPPED)!=0 )
    {
        BOOL fCloseUp = TRUE;
             
        if( !m_fDestroyed )
        {
            //  Allow derivatives an opportunity to modifiy the drop host, 
            //  veto its closing, and/or adjust the canceled status before closing up.
            if( IsWindow( hwndDrop ) )
            {
                ::GetWindowRect( hwndDrop, &rc );

                if( (fCloseUp = OnShowDrop( hwndDrop, FALSE, bCanceled, &rc )) )
                    ::ShowWindow( hwndDrop, SW_HIDE );
            }
        }

        if( fCloseUp )
        {
            m_nDropState = DROPSTATE_CLOSED|
                           (bCanceled ? DROPSTATE_CANCEL : DROPSTATE_OK );
            Capture( FALSE );                  // release the mouse.
            if( !m_fDestroyed )
                PostNotifyCommand( CBN_CLOSEUP );  // notify our parent
        }
        
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL CInPlaceDropWindow::ToggleDrop( BOOL bCanceled )
{
    if( m_nDropState & DROPSTATE_CLOSED )
        return ShowDrop( TRUE, FALSE );
    else if( m_nDropState & DROPSTATE_DROPPED )
        return ShowDrop( FALSE, bCanceled );

    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CInPlaceDropWindow::AnimateDrop( HWND hwndDrop, ULONG dwTime )
{
    ASSERT( hwndDrop );
    ULONG dwDirection = (m_nDropAnchor == DROPANCHOR_TOP) ?
                        AW_VER_POSITIVE : AW_VER_NEGATIVE;
    return AnimateWindow( hwndDrop, dwTime, AW_SLIDE | dwDirection );
}

#define IPDW_CLICKTIMER 1
//-------------------------------------------------------------------------//
void CInPlaceDropWindow::ClickButton()
{
    if( m_fBtnPressed )
    {
        KillTimer( IPDW_CLICKTIMER );
        m_fBtnPressed = FALSE;
    }

    m_fBtnPressed = TRUE;
    InvalidateRect( &m_rcBtn, TRUE );
    UpdateWindow();
    SetTimer( IPDW_CLICKTIMER, 125, NULL );
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceDropWindow::OnTimer( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( wParam == IPDW_CLICKTIMER )
    {
        KillTimer( IPDW_CLICKTIMER );
        m_fBtnPressed = FALSE;
        InvalidateRect( &m_rcBtn, TRUE );
        UpdateWindow();
        bHandled = TRUE;
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  Message handler: reports the drop state of the control
LRESULT CInPlaceDropWindow::OnCBGetDropState( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    bHandled = TRUE;
    return (m_nDropState & DROPSTATE_DROPPED) ? TRUE : FALSE;
}

//-------------------------------------------------------------------------//
BOOL CInPlaceDropWindow::GetMonitorRect( OUT RECT& rcMonitor ) const
{
    HMONITOR hMonitor;

    if( (hMonitor = MonitorFromWindow( *this, MONITOR_DEFAULTTONEAREST ))!=NULL )
    {
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if( GetMonitorInfo( hMonitor, &mi ) )
        {
            rcMonitor = mi.rcMonitor;
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void CInPlaceDropWindow::NormalizeDropRect( 
    IN OUT RECT& rcDrop, 
    OUT OPTIONAL UINT* pnAnchorBorder ) const
{
    RECT rcThis, rcMonitor;

    if( pnAnchorBorder ) *pnAnchorBorder = DROPANCHOR_TOP;

    if( GetWindowRect( &rcThis ) && GetMonitorRect( rcMonitor ) )
    {
        if( rcDrop.bottom > rcMonitor.bottom )
        {
            OffsetRect( &rcDrop, 0, rcThis.top - rcDrop.bottom );
            if( pnAnchorBorder ) *pnAnchorBorder = DROPANCHOR_BOTTOM;
        }
    }
}

//-------------------------------------------------------------------------//
//  Retrieves text rectangle within control
BOOL CInPlaceDropWindow::GetTextBox( LPRECT prc )
{
    RECT rcText;
    
    //  Retrieve text box metrics in client coordinates
    if( !GetClientRect( &rcText ) )
        return FALSE;
    return GetTextBox( RECTWIDTH( &rcText ), RECTHEIGHT( &rcText ), prc );
}

//-------------------------------------------------------------------------//
//  Calculates available text rectangle within control
BOOL CInPlaceDropWindow::GetTextBox( int cx, int cy, LPRECT prc )
{
    ASSERT( prc );

    RECT rcText;
    SetRect( &rcText, 0, 0, cx, cy );

    //  Adjust subtract button rectangle from text rectangle
    rcText.right = m_rcBtn.left;

    if( rcText.left < rcText.right || rcText.top < rcText.bottom )
    {
        *prc = rcText;
        return TRUE;
    }
    
    SetRectEmpty( prc );
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Calculates optimum window height based on current window font
void CInPlaceDropWindow::CalcHeight( LPCTSTR pszWindowText )
{
    HDC hdc;

    if( (hdc = GetDC())!=NULL )
    {
        SIZE    sizeText;
        sizeText.cx = sizeText.cy = 0;
        if( !(pszWindowText && *pszWindowText) ) 
            pszWindowText = TEXT("|");

        HFONT hfText = (HFONT)SelectObject( hdc, m_hfText );
        if( GetTextExtentPoint32( hdc, pszWindowText, lstrlen(pszWindowText), &sizeText ) )
            m_cy = sizeText.cy + 8 /*(2-pixel NC border, 2-pixel client border)*/;

        SelectObject( hdc, hfText );
        ReleaseDC( hdc );
    }
}

//-------------------------------------------------------------------------//
//  Adjusts window height to precalculated value
void CInPlaceDropWindow::AdjustSize()
{
    RECT rcSave, rc;

    GetWindowRect( &rc );
    ::MapWindowPoints( HWND_DESKTOP, GetParent(), (LPPOINT)&rc, 2 );
    CopyRect( &rcSave, &rc );
    rc.bottom = rc.top + m_cy; // adjust for text height.

    if( !EqualRect( &rcSave, &rc ) )
        SetWindowPos( NULL, 0,0, RECTWIDTH( &rc ), RECTHEIGHT( &rc ),
                      SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
}
//-------------------------------------------------------------------------//
//  Adjusts window layout
void CInPlaceDropWindow::PositionControls( int cx, int cy )
{
    SetRect( &m_rcBtn, 
             max( 0, cx - GetSystemMetrics( SM_CXVSCROLL )), 0, 
             cx, cy );
    Invalidate( TRUE );
}

//-------------------------------------------------------------------------//
//  Internal helper: captures or releases the mouse.
BOOL CInPlaceDropWindow::Capture( BOOL bCapture )
{
    if( bCapture )
    {
        //TRACE( TEXT("CInPlaceDropWindow %08lX capturing mouse\n"), m_hWnd );
        SetCapture();
        return TRUE;
    }
    //TRACE( TEXT("CInPlaceDropWindow %08lX releasing mouse\n"), m_hWnd );
    return ReleaseCapture();
}

//-------------------------------------------------------------------------//
//  Message handler: if mouse capture is lost to a child window, 
//  we must subclass that window in order to continue to be apprised of
//  mouse activity.
LRESULT CInPlaceDropWindow::OnCaptureChanged( UINT, WPARAM, LPARAM lParam, BOOL& bHandled )
{
    HWND hwndCapture = (HWND)lParam;
    
    bHandled = FALSE;
    //TRACE( TEXT("CInPlaceDropWindow %08lX losing capture to %08lX\n"), m_hWnd, lParam );

    if( hwndCapture && hwndCapture != m_hWnd && IsChildOfDrop( hwndCapture ) )
    {
        //TRACE( TEXT("CInPlaceDropWindow %08lX subclassing capture window %08lX\n"), m_hWnd, lParam );
        m_wndCaptureAgent.Track( (HWND)lParam, m_hWnd );
        bHandled = TRUE;
    }

    return 0L;
}

//-------------------------------------------------------------------------//
//  Message handler: recieves notifications of mouse activity from a
//  subclassed child window which has directly or indirectly acquired mouse 
//  capture from us.
LRESULT CInPlaceDropWindow::OnCaptureMsg( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    switch( wParam )
    {
        case CCaptureTracker::Released:
            if( m_nDropState & DROPSTATE_DROPPED )
                Capture( TRUE );
            break;

        case CCaptureTracker::Lost:
        case CCaptureTracker::ForeignClick:
            ShowDrop( FALSE, TRUE );
            break;
    }
    bHandled = TRUE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  class CInPlaceDropCalendar : public CInPlaceDropWindow
//  (specialized to manipulate a CCalendarDrop dropdown host window)
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Constructor 
CInPlaceDropCalendar::CInPlaceDropCalendar( HWND hwndTree ) 
    :   CInPlaceDropWindow( hwndTree ),
        m_wndPick( DATETIMEPICK_CLASS, this, DATEPICK_MSGMAP ) 
{
    m_wndDrop.SetOwner( this );
    memset( &m_st, 0, sizeof(m_st) );
    SystemTimeMakeTimeless( &m_st );
}

//-------------------------------------------------------------------------//
//  WM_CREATE handler; creates child window controls
LRESULT CInPlaceDropCalendar::OnCreate( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT        lRet;
    LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam;

    if( (lRet = CInPlaceDropWindow::OnCreate( nMsg, wParam, lParam, bHandled ))!=0L )
        return lRet;
    bHandled = TRUE;

    RECT rc, rcText;
    GetClientRect( &rc );
    GetTextBox( RECTWIDTH( &rc ), RECTHEIGHT( &rc ), &rcText );

    //  Create date picker
    if( m_wndPick.Create( m_hWnd, rcText, NULL,  
                          DTS_SHORTDATEFORMAT|DTS_UPDOWN|WS_CHILD|WS_VISIBLE, 
                          0L, IDC_DATEPICK )==NULL )
        return -1;

    //  Remove bordered styles
    DWORD dwStyle   = m_wndPick.GetWindowLong( GWL_STYLE ),
          dwStyleEx = m_wndPick.GetWindowLong( GWL_EXSTYLE );
    dwStyle     &= ~ (WS_BORDER|WS_OVERLAPPED);
    dwStyleEx   &= ~ (WS_EX_CLIENTEDGE|WS_EX_WINDOWEDGE);
    m_wndPick.SetWindowLong( GWL_STYLE, dwStyle );
    m_wndPick.SetWindowLong( GWL_EXSTYLE, dwStyleEx );

    HWND hwndSpinner;
    for( hwndSpinner = m_wndPick.GetWindow( GW_CHILD );
         hwndSpinner != NULL;
         hwndSpinner = ::GetWindow( hwndSpinner, GW_HWNDNEXT ) )
    {
        ::ShowWindow( hwndSpinner, SW_HIDE );
    }

    //  Create drop window
    if( m_wndDrop.Create( m_hWnd, NULL )==NULL )
        return -1;

    m_wndPick.SetFocus();

    PositionControls( RECTWIDTH( &rc ), RECTHEIGHT( &rc ) );
    return 0L;
}

//-------------------------------------------------------------------------//
//  Date time picker control's WM_GETDLGCODE handler 
LRESULT CInPlaceDropCalendar::OnPickerGetDlgCode( UINT, WPARAM, LPARAM lParam, BOOL& bHandled )
{
    if( 0L != lParam && 
        WM_KEYDOWN == ((LPMSG)lParam)->message && VK_TAB == ((LPMSG)lParam)->wParam )
    {
        bHandled = FALSE;
        return 0L;
    }
    
    return DLGC_WANTALLKEYS;
}

//-------------------------------------------------------------------------//
//  Date time picker control's WM_SHOWWINDOW hander
LRESULT CInPlaceDropCalendar::OnPickerShowWindow( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = DefWindowProc( nMsg, wParam, lParam );
    bHandled = TRUE;
    
    HWND hwndChild = NULL;
    for( hwndChild = GetWindow( GW_CHILD );
         hwndChild != NULL;
         hwndChild = ::GetWindow( hwndChild, GW_HWNDNEXT ) )
    {
        ::ShowWindow( hwndChild, SW_HIDE );
    }
    return lRet; 
}

//-------------------------------------------------------------------------//
//  WM_NOTIFY handler for IDC_DATEPICK
LRESULT CInPlaceDropCalendar::OnPickerNotify( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    SYSTEMTIME st;
    bHandled = FALSE;

    switch( pnmh->code )
    {
        case DTN_DATETIMECHANGE:    
            if( !GetPickDate( st ) )
                memset( &st, 0, sizeof(st) );

            SystemTimeMakeTimeless( &st );
            m_wndDrop.SetDate( st );
            UpdateDisplayDate( st, UDDF_WINDOWTEXT /*avoid circular logic w/SetPickDate()*/ );
            bHandled = TRUE;
            break;
            
        case NM_SETFOCUS:
            ::SendMessage( m_hwndTree, WMU_CTLFOCUS, 0, (LPARAM)WM_SETFOCUS );
            break;

        case NM_KILLFOCUS:
            ::SendMessage( m_hwndTree, WMU_CTLFOCUS, 0, (LPARAM)WM_KILLFOCUS );
            break;
    }

    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_SIZE handler.
LRESULT CInPlaceDropCalendar::OnSize( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    CInPlaceDropWindow::OnSize( nMsg, wParam, lParam, bHandled );
    PositionControls( LOWORD(lParam), HIWORD(lParam) );
    return 0L;
}

//-------------------------------------------------------------------------//
void CInPlaceDropCalendar::PositionControls( int cx, int cy )
{
    if( ::IsWindow( m_wndPick.m_hWnd ) )
    {
        RECT rc;
        GetTextBox( cx, cy, &rc );
        m_wndPick.SetWindowPos( NULL, 0, 0, 
                                RECTWIDTH( &rc ), RECTHEIGHT( &rc ),
                                SWP_NOZORDER|SWP_NOACTIVATE );
    }
    InvalidateRect( &m_rcBtn );
}

//-------------------------------------------------------------------------//
//  WM_SETFOCUS handler.
LRESULT CInPlaceDropCalendar::OnSetFocus( UINT, WPARAM, LPARAM, BOOL& bHandled )
{
    bHandled = FALSE;
    m_wndPick.SetFocus();  // give it to date picker.
    return 0L;
}

//-------------------------------------------------------------------------//
//  WMU_SETITEMDATA handler; caches the assigned date.
LRESULT CInPlaceDropCalendar::OnSetItemData( UINT, WPARAM, LPARAM lParam, BOOL& bHandled )
{
    SYSTEMTIME* pst = (SYSTEMTIME*)lParam;

    if( pst )
    {
        m_st = *pst;
        SystemTimeMakeTimeless( &m_st );

        SetPickDate( m_st );
        m_wndDrop.SetDate( m_st );
    }

    bHandled = TRUE;
    return 1L;
}

//-------------------------------------------------------------------------//
//  Overriden to adjust drop window, recalc child layout, veto drop/closeup
BOOL CInPlaceDropCalendar::OnShowDrop( HWND hwndDrop, BOOL bDrop, BOOL& bCanceled, LPRECT prcDrop )
{
    if( bDrop )
    {
        SIZE sizeCal, sizeDropNc;

        //  Reinitialize layout (who knows where and how we're being dropped!) before
        //  being displayed
        m_wndDrop.GetClientSize( &sizeCal );
        GetNcBorderSize( hwndDrop, &sizeDropNc );
        
        prcDrop->left   = prcDrop->right - (sizeCal.cx + sizeDropNc.cx);
        prcDrop->bottom = prcDrop->top   + (sizeCal.cy + sizeDropNc.cy);

        NormalizeDropRect( *prcDrop, &m_nDropAnchor );
        
        m_wndDrop.SetDate( m_st );
    }
        
    return CInPlaceDropWindow::OnShowDrop( hwndDrop, bDrop, bCanceled, prcDrop );
}

//-------------------------------------------------------------------------//
//  Keyboard message handler; passes appropriate keys to common calendar
//  control child window.
LRESULT CInPlaceDropCalendar::OnKey( UINT nMsg, WPARAM virtKey, LPARAM lParam, BOOL& bHandled )
{
    //  Route key messages to child controls
    return HandleKeyMessage( m_hWnd, nMsg, virtKey, lParam, bHandled );
}


//-------------------------------------------------------------------------//
//  WM_CHAR, WM_KEYUP, WM_KEYDOWN handler for Date/Time Picker control.
LRESULT CInPlaceDropCalendar::OnPickerKey( UINT nMsg, WPARAM virtKey, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = 0;
    bHandled = FALSE;

    //  Pass directional keys to calendar drop
    lRet = HandleKeyMessage( m_wndPick, nMsg, virtKey, lParam, bHandled );
    if( bHandled ) 
        return lRet;

    //  Let input (i.e., numeric) keys go to picker control.
    if( IsInputKey( virtKey ) )
    {
        bHandled = TRUE;
        return m_wndPick.DefWindowProc( nMsg, virtKey, lParam );
    }

    //  Pass other keys to CInPlaceDropCalendar (picker's parent)
    bHandled = TRUE;
    return SendMessage( nMsg, virtKey, lParam );
}

//-------------------------------------------------------------------------//
//  WM_SYSKEYDOWN handler for Date/Time Picker control.
LRESULT CInPlaceDropCalendar::OnPickerSysKeyDown( UINT nMsg, WPARAM virtKey, LPARAM lParam, BOOL& bHandled )
{
    //  Pass WM_SYSKEYDOWN messages to CInPlaceDropCalendar (picker's parent)
    bHandled = TRUE;
    return SendMessage( nMsg, virtKey, lParam );
}

//-------------------------------------------------------------------------//
//  Determines whether the specified virtual key code represents a directional key.
BOOL CInPlaceDropCalendar::IsDirectionalKey( WPARAM virtKey ) const
{
    switch( virtKey )
    {
        case VK_PRIOR:
        case VK_NEXT:
        case VK_END:
        case VK_HOME:
        case VK_LEFT:
        case VK_UP:
        case VK_RIGHT:
        case VK_DOWN:
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Determines whether the specified virtual key code represents a numeric key.
BOOL CInPlaceDropCalendar::IsInputKey( WPARAM virtKey ) const
{
    if( (virtKey >= '0' && virtKey <= '9') || 
        (virtKey >= VK_NUMPAD0 && virtKey <= VK_NUMPAD9) )
        return TRUE;

    if( virtKey == VK_RIGHT || virtKey == VK_LEFT )
        return TRUE;

    return FALSE;
}
//-------------------------------------------------------------------------//
LRESULT CInPlaceDropCalendar::HandleKeyMessage( HWND hwndFrom, UINT nMsg, WPARAM virtKey, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = 0L;
    bHandled = FALSE;

    //  calendar drop key?
    if( 0 !=(m_nDropState & DROPSTATE_DROPPED) &&
        IsDirectionalKey( virtKey ) )
    {
        //  Pass directional keys to calendar drop control
        bHandled = TRUE;
        return ::SendMessage( m_wndDrop.CalHwnd(), nMsg, virtKey, lParam );
    }
    else // calendar is closed up...
    {
        //  ESC or ENTER?
        if( nMsg==WM_KEYDOWN )
        {
            switch( virtKey )
            {
                case VK_ESCAPE:
                    ShowDrop( FALSE, TRUE );
                    UpdateDisplayDate( m_st, UDDF_PICKERDATE|UDDF_WINDOWTEXT );
                    break;
                case VK_RETURN:
                    ShowDrop( FALSE, FALSE );
                    if( hwndFrom == m_wndPick )
                    {
                        GetPickDate( m_st );
                        UpdateDisplayDate( m_st, UDDF_WINDOWTEXT );
                    }
                    else
                    {
                        m_wndDrop.GetDate( m_st );
                        UpdateDisplayDate( m_st, UDDF_PICKERDATE|UDDF_WINDOWTEXT );
                    }
                    break;
            }
        }

        NAVIGATION_KEY_INFO nki;
        InitNavigationKeyInfo( &nki, m_hWnd, nMsg, virtKey, lParam );
        lRet = ::SendMessage( m_hwndTree, WMU_NAVIGATION_KEY, 
                              GetDlgCtrlID(), (LPARAM)&nki );

        bHandled = nki.bHandled;
    }
    return lRet;
}

//-------------------------------------------------------------------------//
//  Extracts and assigns calendar date members from the provided SYSTEMTIME object.
void CInPlaceDropCalendar::AssignCalendarDate( IN SYSTEMTIME& stSrc, OUT SYSTEMTIME& stDest )
{
    stDest.wYear      = stSrc.wYear;
    stDest.wMonth     = stSrc.wMonth;
    stDest.wDay       = stSrc.wDay;
    stDest.wDayOfWeek = stSrc.wDayOfWeek;
    SystemTimeMakeTimeless( &stDest );
}

//-------------------------------------------------------------------------//
//  Displays the specified date in the drop calendard window.
//  Flags member: UDDF_WINDOWTEXT, UDDF_PICKERDATE
void CInPlaceDropCalendar::UpdateDisplayDate( IN SYSTEMTIME& stSrc, ULONG dwFlags )
{
    DATE date = 0;
    SystemTimeMakeTimeless( &stSrc );

    if( dwFlags & UDDF_WINDOWTEXT )
    {
        if( SystemTimeToVariantTime( &stSrc, &date ) )
        {
            BSTR bstrDate;
            if( SUCCEEDED( VarBstrFromDate( date, GetUserDefaultLCID(), 
                                            VAR_DATEVALUEONLY, &bstrDate ) ) )
            {
                USES_CONVERSION;
                SetWindowText( W2T( bstrDate ) );

                Invalidate();
                SysFreeString( bstrDate );
            }
        }
    }

    if( dwFlags & UDDF_PICKERDATE )
        SetPickDate( stSrc );
}

//-------------------------------------------------------------------------//
BOOL CInPlaceDropCalendar::GetPickDate( OUT SYSTEMTIME& stDest )
{
    if( IsWindow( m_wndPick.m_hWnd ) &&
        DateTime_GetSystemtime( m_wndPick, &stDest )==GDT_VALID )
    {
        SystemTimeMakeTimeless( &stDest );
        return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL CInPlaceDropCalendar::SetPickDate( IN SYSTEMTIME& stSrc ) 
{
    if( !::IsWindow( m_wndPick.m_hWnd ) )
        return FALSE;
    
    WPARAM wPickFlag = (stSrc.wMonth==0 || stSrc.wDay==0 || stSrc.wYear == 0) ? 
                        GDT_NONE : GDT_VALID;

    SystemTimeMakeTimeless( &stSrc );
    return DateTime_SetSystemtime( m_wndPick, wPickFlag, &stSrc );
}

//-------------------------------------------------------------------------//
//  class CCalendarDrop : public CDialogImpl<CCalendarDrop>
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
//  Creates calendar drop child window and subordinates
HWND CCalendarDrop::Create( HWND hwndParent, LPPOINT ptMouseActivate )
{
    HWND hwndThis;
    RECT rc = {0,0,0,0};

    if( (hwndThis = CDialogImpl<CCalendarDrop>::Create( hwndParent ))==NULL )
        return hwndThis;

    //  Fake out USER and add illegal style combinations after creation...
    DWORD dwStyle   = ::GetWindowLong( hwndThis, GWL_STYLE ),
          dwExStyle = ::GetWindowLong( hwndThis, GWL_EXSTYLE );

    dwStyle   |= WS_CHILD;
    dwExStyle |= WS_EX_TOOLWINDOW;
   
    ::SetWindowLong( hwndThis, GWL_STYLE, dwStyle );
    ::SetWindowLong( hwndThis, GWL_EXSTYLE, dwExStyle );
    SetWindowPos( HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE );
    SetParent( HWND_DESKTOP );
    
    if( (m_hwndCal = GetDlgItem( IDC_CALENDAR ))==NULL )
    {
        DestroyWindow();
        return NULL;
    }
    
    MonthCal_SetColor( m_hwndCal, MCSC_BACKGROUND, GetSysColor( COLOR_3DFACE ) );
    MonthCal_SetColor( m_hwndCal, MCSC_TITLEBK ,   GetSysColor( COLOR_HIGHLIGHT ) );
    MonthCal_SetColor( m_hwndCal, MCSC_TITLETEXT,  GetSysColor( COLOR_HIGHLIGHTTEXT ) );
    MonthCal_SetColor( m_hwndCal, MCSC_MONTHBK,    GetSysColor( COLOR_WINDOW ) );
    MonthCal_SetColor( m_hwndCal, MCSC_TEXT,       GetSysColor( COLOR_WINDOWTEXT ) ); 
    
    SIZE sizeCal;
    if( GetClientSize( &sizeCal ) )
    {
       ::SetWindowPos( m_hwndCal, NULL, 0, 0, sizeCal.cx, sizeCal.cy,
                       SWP_NOZORDER|SWP_NOACTIVATE );
    }

    return hwndThis;
}

//-------------------------------------------------------------------------//
//  Calculates optimum client size
BOOL CCalendarDrop::GetClientSize( OUT LPSIZE pSize )
{
    RECT rcClient;
    ASSERT( m_hWnd );
    SIZE sizeNc;
    
    if( MonthCal_GetMinReqRect( m_hwndCal, &rcClient ) )
    {
        GetNcBorderSize( m_hwndCal, &sizeNc );
        pSize->cx = RECTWIDTH( &rcClient ) + sizeNc.cx;
        pSize->cy = RECTHEIGHT( &rcClient ) + sizeNc.cy;
        return TRUE;
    }

    pSize->cx = pSize->cy = 0;
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Assigns a date/time to the calendar
BOOL CCalendarDrop::SetDate( IN SYSTEMTIME&  st )
{
    CInPlaceDropCalendar::AssignCalendarDate( st, m_st );
    if( !m_hWnd ) 
        return TRUE;
    return Update();
}

//-------------------------------------------------------------------------//
//  Retrieves the date/time from the calendar
BOOL CCalendarDrop::GetDate( OUT SYSTEMTIME& st )
{
    if( m_hWnd )
    {
        SYSTEMTIME stCurSel;
        if( MonthCal_GetCurSel( m_hwndCal, &stCurSel ) )
            CInPlaceDropCalendar::AssignCalendarDate( stCurSel, m_st );
    }
    st = m_st;
    return TRUE;
}

//-------------------------------------------------------------------------//
BOOL CCalendarDrop::Update()
{
    return MonthCal_SetCurSel( m_hwndCal, &m_st );
}

//-------------------------------------------------------------------------//
//  MCN_SELCHANGE handler.  
//  Responds to calendar selection changes by updating the owner drop window's
//  text.
LRESULT CCalendarDrop::OnCalSelChange( int nIDCtl, LPNMHDR pNMH, BOOL& bHandled )
{
    LPNMSELCHANGE pNMS = (LPNMSELCHANGE)pNMH;

    CInPlaceDropCalendar::AssignCalendarDate( pNMS->stSelStart, m_st );
    m_pOwner->UpdateDisplayDate( m_st, UDDF_PICKERDATE|UDDF_WINDOWTEXT );

    bHandled = TRUE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  MCN_SELECT handler.  
LRESULT CCalendarDrop::OnCalSelect( int nIDCtl, LPNMHDR pNMH, BOOL& bHandled )
{
    OnCalSelChange( nIDCtl, pNMH, bHandled );
    m_pOwner->ShowDrop( FALSE, FALSE );
    return 0L;
}


//-------------------------------------------------------------------------//
//  class CInPlaceDropEdit : public CInPlaceDropWindow
//  (specialized to manipulate a CEditDrop dropdown host window)
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Constructor
CInPlaceDropEdit::CInPlaceDropEdit( HWND hwndTree ) 
    :   CInPlaceDropWindow( hwndTree )
{
    m_wndDrop.SetOwner( this );
}

//#define  __MODELESS_EDITDLG_
//-------------------------------------------------------------------------//
//  Overriden to present CEditDrop as modal dialog.
BOOL CInPlaceDropEdit::OnShowDrop( HWND hwndDrop, BOOL bDrop, BOOL& bCanceled, LPRECT prcDrop )
{

#ifdef __MODELESS_EDITDLG_
    if( bDrop )
        return m_wndDrop.Create( *this )!=NULL;
    
    return m_wndDrop.DestroyWindow();
#else
    m_wndDrop.DoModal( *this );
    SetFocus();
    return FALSE;

#endif __MODELESS_EDITDLG_
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceDropEdit::OnChar( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //TRACE( TEXT("CInPlaceDropEdit::OnChar( %08lX, '%04X' )\n"), nMsg, wParam );
    bHandled = FALSE;
    
    if( GetDropState() & DROPSTATE_CLOSED )
    {
        switch( wParam )
        {
            case VK_BACK:
            case VK_TAB:
            case VK_CLEAR:
            case VK_RETURN:
            case VK_ESCAPE:
            case 0x0A:  // line feed
                return 0L;
        }

        m_wndDrop.QueueCharMsg( nMsg, wParam, lParam );
        ShowDrop( TRUE, FALSE );
        bHandled = TRUE;
    }
    return 0L;    
}

//-------------------------------------------------------------------------//
LRESULT CInPlaceDropEdit::OnImeComposition( UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    //  The IME window is about to submit its composition string to us.
    //  If we don't intercept this message and handle it ourselves, DefWindowProc
    //  will start feeding us the string in a series of WM_CHARs, which will 
    //  errantly open our drop-down window and cause us to hang.
    //  (NT raid# 295062).   

    //  We can either get the string ourselves and assign it as our window text, or
    //  we can simply drop the edit control and have the IME latch to that.   We tried
    //  the first way, which worked with all IMEs except CHS.   So now we're opening
    //  the edit drop as soon as we get IME input, which makes the drop similar in 
    //  behavior to direct input.
    
    bHandled = FALSE;

#ifdef _DEAL_WITH_IME_COMPOSITION_STRING_WITHOUT_DROPDOWN 
 
    if( GetDropState() & DROPSTATE_CLOSED )
    {
        HIMC himc;
        if( (himc = ImmGetContext( m_hWnd )) != NULL )
        {
            LPWSTR pwsz = NULL;
            int    cch  = ImmGetCompositionStringW( himc, GCS_RESULTSTR, NULL, 0 )/sizeof(WCHAR);

            if( cch && (pwsz = new WCHAR[cch+1]) != NULL )
            {
                ImmGetCompositionStringW( himc, GCS_RESULTSTR, pwsz, (cch + 1) * sizeof(WCHAR) );
                pwsz[cch] = L'\0'; // null terminate the string
                SetWindowText( pwsz );
                delete [] pwsz;
            }
            ImmReleaseContext( m_hWnd, himc );
        }
        bHandled = TRUE;
    }

#else _DEAL_WITH_IME_COMPOSITION_STRING_WITHOUT_DROPDOWN 

    //  Drop the edit drop if it's not dropped already, 
    //  like we do upon receiving direct input characters.
    if( GetDropState() & DROPSTATE_CLOSED )
        ShowDrop( TRUE, FALSE );
        
#endif _DEAL_WITH_IME_COMPOSITION_STRING_WITHOUT_DROPDOWN 

    return 0L;
}


//-------------------------------------------------------------------------//
//  class CEditDrop
//-------------------------------------------------------------------------//
const int nEditDropCtlMargin = 2;

CEditDrop::CEditDrop()
    :   m_pOwner(NULL), 
        m_pszUndo(NULL),
        m_fEndDlg(FALSE)
{
    memset( &m_msgChar, 0, sizeof(m_msgChar) );
}

//-------------------------------------------------------------------------//
void CEditDrop::QueueCharMsg( UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    ASSERT( nMsg == WM_CHAR || nMsg == WM_DEADCHAR );
    m_msgChar.message = nMsg;
    m_msgChar.wParam = wParam;
    m_msgChar.lParam = lParam;
}

//-------------------------------------------------------------------------//
//  Computes minimum drop window width.
void CEditDrop::EnforceMinWidth( LPRECT prcThis /* in screen coords */)
{
    ASSERT( prcThis );
    ASSERT( IsWindow( GetDlgItem( IDOK ) ) );
    ASSERT( IsWindow( GetDlgItem( IDOK ) ) );

    RECT rcOK, rcCancel, rc, rcClient;
    ::GetWindowRect( GetDlgItem( IDOK ), &rcOK );
    ::GetWindowRect( GetDlgItem( IDCANCEL ), &rcCancel );
    
    GetWindowRect( &rc );
    GetClientRect( &rcClient );
    int cxMin = (rcCancel.right - rcOK.left) + nEditDropCtlMargin +
                (RECTWIDTH(&rc) - RECTWIDTH(&rcClient)) /* non-client dim */ ;

    if( RECTWIDTH(prcThis) < cxMin )
        prcThis->right = prcThis->left + cxMin;
}

//-------------------------------------------------------------------------//
//  WM_INITDLG handler; initializes the dialog
LRESULT CEditDrop::OnInitDlg( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    RECT rcDlg, rcClient, rcOwner;
    HWND hwndEdit = GetDlgItem( IDC_EDIT );

    m_pOwner->GetWindowRect( &rcOwner );
    GetWindowRect( &rcDlg );
    
    //  Modify screen size, position
    rcDlg.left  = rcOwner.left;
    rcDlg.right = rcOwner.right; 
    EnforceMinWidth( &rcDlg );
    OffsetRect( &rcDlg, 0, rcOwner.bottom - rcDlg.top );
    
    //  adjust drop rect to fit the screen.
    m_pOwner->NormalizeDropRect( rcDlg, &m_pOwner->GetDropAnchor() );
    
    SetWindowPos( NULL, rcDlg.left, rcDlg.top, 
                  RECTWIDTH( &rcDlg ), RECTHEIGHT( &rcDlg ), 
                  SWP_NOACTIVATE|SWP_NOZORDER );

    //  Do internal layout
    GetClientRect( &rcClient );
    PositionControls( RECTWIDTH( &rcClient ), RECTHEIGHT( &rcClient ) );

    //  Assign text
    TextFromOwner();

    //  Reveal ourselves
    m_pOwner->AnimateDrop( *this );

    SetWindowPos( HWND_TOPMOST, 0,0,0,0, 
                  SWP_NOMOVE|SWP_NOSIZE );

    //  Dequeue pending character keystrokes
    if( m_msgChar.message )
        ::PostMessage( hwndEdit, m_msgChar.message, m_msgChar.wParam, m_msgChar.lParam );
    memset( &m_msgChar, 0, sizeof(m_msgChar) );

    //  Remind ourselves to set mouse capture. (USER won't let us have it right now).
    PostMessage( WMU_SETCAPTURE );

    bHandled = TRUE;
    return TRUE;
}

//-------------------------------------------------------------------------//
//  WM_SIZE handler; invokes layout management routine
LRESULT CEditDrop::OnSize( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    PositionControls( LOWORD( lParam ), HIWORD( lParam ) );
        
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_WINDOWPOSCHANGING handler; restricts resizing to within logical
//  bounds.
LRESULT CEditDrop::OnWindowPosChanging( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LPWINDOWPOS pWP = (LPWINDOWPOS)lParam;

    if( (pWP->flags & SWP_NOSIZE)==0 )
    {
        RECT rcOwner;
        m_pOwner->GetWindowRect( &rcOwner );

        if( pWP->x > rcOwner.right )
        {
            RECT rcThis;
            GetWindowRect( &rcThis );

            pWP->x = rcOwner.right;
            pWP->cx= rcThis.right - pWP->x;

        }

        if( (pWP->x + pWP->cx) < rcOwner.left )
            pWP->cx = rcOwner.left - pWP->x;
    }

    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  Child window layout management routine.
void CEditDrop::PositionControls( int cx, int cy )
{
    HWND hwndEdit   = GetDlgItem( IDC_EDIT ), 
         hwndOk     = GetDlgItem( IDOK ), 
         hwndCancel = GetDlgItem( IDCANCEL );
    RECT rcEdit, rcOk, rcCancel;
    HDWP hdwp;

    if( !( hwndEdit && hwndOk && hwndCancel ) )
        return;
        
    ::GetWindowRect( hwndEdit,   &rcEdit );
    ::GetWindowRect( hwndCancel, &rcCancel );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcEdit, 2 );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcCancel, 2 );

    SetRect( &rcEdit,   nEditDropCtlMargin, nEditDropCtlMargin, cx - nEditDropCtlMargin * 2, 
                        cy - (RECTHEIGHT( &rcCancel ) + nEditDropCtlMargin * 2) );

    SetRect( &rcCancel, cx - ( RECTWIDTH( &rcCancel ) + nEditDropCtlMargin ), rcEdit.bottom + nEditDropCtlMargin,
                        cx - nEditDropCtlMargin, cy - nEditDropCtlMargin );

    SetRect( &rcOk,     rcCancel.left - ( RECTWIDTH( &rcCancel ) + nEditDropCtlMargin ), rcCancel.top,
                        rcCancel.left - nEditDropCtlMargin, cy - nEditDropCtlMargin );
    

    if( (hdwp = ::BeginDeferWindowPos( 3 )) != NULL )
    {
        //  Lazy man's macro...
        #define EDITDLG_SETCTLPOS( hwnd, rc ) ::DeferWindowPos( hdwp,(hwnd),NULL,\
                (rc).left,(rc).top, RECTWIDTH( &(rc) ), RECTHEIGHT( &(rc) ), \
                SWP_NOZORDER|SWP_NOACTIVATE )

        EDITDLG_SETCTLPOS( hwndEdit,    rcEdit );
        EDITDLG_SETCTLPOS( hwndOk,      rcOk );
        EDITDLG_SETCTLPOS( hwndCancel,  rcCancel );

        ::EndDeferWindowPos( hdwp );
    }
}

//-------------------------------------------------------------------------//
//  WM_ACTIVATEAPP handler: Closes the dialog when host thread loses
//  activation.
LRESULT CEditDrop::OnActivateApp( UINT, WPARAM wParam, LPARAM, BOOL& bHandled )
{
    BOOL fActive = (BOOL)wParam;
    bHandled = FALSE;

    if( !fActive )
    {
        bHandled = TRUE;
        EndDialogCancel();
    }
    return 0L;
}

BOOL _ClientToNcMouseMsg( 
    HWND hwndCapture,
    LRESULT nHitTest,
    UINT nMsgIn, UINT* pnMsgOut,
    WPARAM wParamIn, WPARAM* pwParamOut,
    LPARAM lParamIn, LPARAM* plParamOut )
{
    *pnMsgOut = 0;
    switch( nMsgIn )
    {
        case WM_MOUSEMOVE:
            *pnMsgOut = WM_NCMOUSEMOVE;
            break;
        
        case WM_LBUTTONDOWN:
            *pnMsgOut = WM_NCLBUTTONDOWN;
            break;
        
        case WM_LBUTTONUP:
            *pnMsgOut = WM_NCLBUTTONUP;
            break;
        
        case WM_LBUTTONDBLCLK:
            *pnMsgOut = WM_NCLBUTTONDBLCLK;
            break;
    }

    if( *pnMsgOut )
    {
        POINTS ptsIn = MAKEPOINTS(lParamIn);
        POINT  pt;
        POINTSTOPOINT(pt, ptsIn);
        MapWindowPoints( hwndCapture, HWND_DESKTOP, &pt, 1 );
        *pwParamOut = nHitTest;
        *plParamOut = POINTTOPOINTS(pt);
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  WM_MOUSEFIRST - WM_MOUSELAST message handler.  Note: most are all messages 
//  arise from captured mouse activity.
LRESULT CEditDrop::OnMouseMsg( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = 0L;
    POINT   ptScrn;
    POINTS  pts = MAKEPOINTS( lParam );

    bHandled = FALSE;

    //  Map points to screen and determine what window the message may
    //  have been destined for:
    POINTSTOPOINT( ptScrn, pts );
    ::MapWindowPoints( m_hWnd, HWND_DESKTOP, &ptScrn, 1 );
    HWND hwndTarget = WindowFromPoint( ptScrn );

    //  If it's not for us, pass it off to the target window.
    if( hwndTarget != NULL && hwndTarget != m_hWnd )
    {
        //  Determine the hittest code
        LRESULT nHitTest   = ::SendMessage( hwndTarget, WM_NCHITTEST, 0L, MAKELPARAM( ptScrn.x, ptScrn.y ) );

        //  First, on a WM_MOUSEMOVE, establish the correct cursor.
        if( nMsg == WM_MOUSEMOVE )
            ::SendMessage( hwndTarget, WM_SETCURSOR, (WPARAM)hwndTarget, MAKELPARAM( nHitTest, nMsg ) );

        //  If it's a non-client hit test, convert to a non-client message and 
        //  send down to target window.
        if( nHitTest != HTCLIENT )
        {
            //  BUGBUG: This non-client chit doesn't work.  
            //  If you have a scroll bar in a multiline edit child, the bar is unresponsive to
            //  mouse movement. We've got to figure out what the message sequence is to make 
            //  this work!
            UINT   nNcMsg;
            WPARAM wNcParam;
            LPARAM lNcParam;
            if( _ClientToNcMouseMsg( m_hWnd, nHitTest, nMsg, &nNcMsg, wParam, &wNcParam, lParam, &lNcParam ) )
            {
                lRet = ::SendMessage( hwndTarget, nNcMsg, wNcParam, lNcParam );

                UINT uSysCmd = (nHitTest == HTVSCROLL) ? SC_VSCROLL :
                               (nHitTest == HTHSCROLL) ? SC_HSCROLL : 0;
                if( uSysCmd )
                    ::SendMessage( hwndTarget, WM_SYSCOMMAND, uSysCmd, lNcParam );
                bHandled = TRUE;
            }
        }
        
        //  If the user clicked outside us, close the dialog.
        if( !IsChild( hwndTarget ) )
        {
            switch( nMsg )
            {
                case WM_LBUTTONDOWN:
                case WM_RBUTTONDOWN:
                case WM_MBUTTONDOWN:
                    EndDialogCancel();
                    bHandled = TRUE;
            }
        }

        //  If we didn't send a non-client message, deal with it as a client message:
        if( !bHandled )
        {
            POINT pt = ptScrn;
            ::MapWindowPoints( HWND_DESKTOP, hwndTarget, &pt, 1 );
            lRet = ::SendMessage( hwndTarget, nMsg, wParam, MAKELPARAM( pt.x, pt.y ) );
            bHandled = TRUE;
        }

    }

    return lRet;
}

//-------------------------------------------------------------------------//
//  WMU_SETCURSOR private message handler.  This self-posted message
//  instructs the dialog to capture mouse activity.  
LRESULT CEditDrop::OnSetCapture( UINT, WPARAM, LPARAM, BOOL& bHandled )
{
    SetCapture(); 
    bHandled = TRUE; 
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_CAPTURECHANGED handler.  If the capture is lost to a child window,
//  we'll light up our capture tracker to retain knowledge of mouse activity.
LRESULT CEditDrop::OnCaptureChanged( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    HWND hwndOther = (HWND)lParam;

    if( hwndOther && IsChild( hwndOther ) )
    {
        //TRACE(TEXT("Tracking capture lost to child control %08lX\n"), hwndOther);
        m_captureTrack.Track( hwndOther, m_hWnd );
    }

    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  CCaptureTracker::NotifyMsg() handler.  Handles mouse activity messages
//  sent by our capture tracker.
LRESULT CEditDrop::OnCaptureMsg( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    switch( wParam )
    {
        case CCaptureTracker::ForeignClick:
        case CCaptureTracker::Lost:
            EndDialogCancel();
            break;
        case CCaptureTracker::Released:
            PostMessage( WMU_SETCAPTURE );
            break;
    }
    
    bHandled = TRUE;
    return 0L;
}

//-------------------------------------------------------------------------//
void CEditDrop::CommonEndDialog( int nResult )
{
    m_fEndDlg = TRUE ;
    ReleaseCapture();
    EndDialog( nResult );
}
        
//-------------------------------------------------------------------------//
//  WM_DESTROY handler.
LRESULT CEditDrop::OnDestroy( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    if( GetCapture()== m_hWnd )
        ReleaseCapture();
    
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_NCHITTEST handler; prevents resizing when mouse is tracking locked border.
LRESULT CEditDrop::OnNcHitTest( UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    LRESULT lRet = DefWindowProc( m_hWnd, nMsg, wParam, lParam );
    UINT    nDropAnchor = m_pOwner->GetDropAnchor();
    if( nDropAnchor == DROPANCHOR_TOP )
    {
        switch( lRet ) {
            case HTTOPLEFT: case HTTOP: case HTTOPRIGHT:
                lRet = HTNOWHERE;    
        }
    }
    else if( nDropAnchor == DROPANCHOR_BOTTOM )
    {
        switch( lRet ) {
            case HTBOTTOMLEFT: case HTBOTTOM: case HTBOTTOMRIGHT:
                lRet = HTNOWHERE;    
        }
    }

    bHandled = TRUE;
    return lRet;
}

//-------------------------------------------------------------------------//
//  Transfers text from the owner CInPlaceDropWindow object to child edit control.
void CEditDrop::TextFromOwner()
{
    LPTSTR  pszBuf = NULL;
    int     cchBuf = m_pOwner->GetWindowTextLength();

    if( m_pszUndo )
    {
        delete [] m_pszUndo;
        m_pszUndo = NULL;
    }

    if( cchBuf > 0 &&
        (pszBuf = new TCHAR[cchBuf+1]) )
    {
        m_pOwner->GetWindowText( pszBuf, cchBuf + 1 );
        SetDlgItemText( IDC_EDIT, pszBuf );
        m_pszUndo = pszBuf;
    }
    
}

//-------------------------------------------------------------------------//
//  Transfers text from the child edit control to the owner CInPlaceDropWindow
//  object.  Returns TRUE if a change was detected, otherwise FALSE.
BOOL CEditDrop::TextToOwner()
{
    HWND    hwndEdit = GetDlgItem( IDC_EDIT );
    LPTSTR  pszEdit  = NULL;
    int     cchEdit  = hwndEdit ? ::GetWindowTextLength( hwndEdit ) : 0L;
    BOOL    bRet     = TRUE;

    if( cchEdit > 0 && (pszEdit = new TCHAR[cchEdit+1]) )
        GetDlgItemText( IDC_EDIT, pszEdit, cchEdit + 1 );

    if( m_pszUndo && pszEdit )
    {
        if( lstrcmp( m_pszUndo, pszEdit )==0 )
            bRet = FALSE;
    }
    else if( m_pszUndo == pszEdit /*NULL*/)
        bRet = FALSE;

    if( bRet )
    {
        m_pOwner->SetWindowText( pszEdit );

        if( m_pszUndo )
        {
            delete [] m_pszUndo;
            m_pszUndo = NULL;
        }
        if( pszEdit && *pszEdit )
        {
            m_pszUndo = pszEdit;
            pszEdit = NULL;
        }
    }

    if( pszEdit ) delete [] pszEdit;

    return bRet;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND:EN_CHANGE handler.
LRESULT CEditDrop::OnEditChange( WORD, WORD, HWND hwndEdit, BOOL& bHandled )
{
    if( !m_fEndDlg ) // avoid processing any edit changes when we're on our way down.
    {
        LPTSTR  pszBuf = NULL;
        int     cch       = ::GetWindowTextLength( hwndEdit );

        if( (pszBuf = new TCHAR[cch+1])!=NULL )
        {
            *pszBuf = 0;
            if( cch )
                ::GetWindowText( hwndEdit, pszBuf, cch+1 );

            m_pOwner->SetWindowText( pszBuf );
            delete [] pszBuf;
        }
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND:IDCANCEL handler.
LRESULT CEditDrop::OnCancel( WORD, WORD, HWND, BOOL& bHandled )
{
    bHandled = TRUE;
    EndDialogCancel();
    return 0L;
}

//-------------------------------------------------------------------------//
//  WM_COMMAND:IDOK handler.
LRESULT CEditDrop::OnOk( WORD, WORD, HWND, BOOL& bHandled )
{
    bHandled = TRUE;
    EndDialogOK();
    return 0L;
}
