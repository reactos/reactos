// PreviewDlg.cpp : Implementation of CPreviewWnd
#include "precomp.h"

#include "PrevWnd.h"
#include "PrevCtrl.h"
#include "resource.h"

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))


/////////////////////////////////////////////////////////////////////////////
// CPreviewWnd

CPreviewWnd::CPreviewWnd(bool bShowToolbar /*=true*/) :
        m_ctlToolbar(NULL, this, 1)
{
    m_fOwnsHandles = true;
    m_fHideFullscreen = false;
    m_fShowToolbar = bShowToolbar;
    m_fAllowContextMenu = true;
    m_fAllowGoOnline = false;
    m_fHidePrintBtn = false;
    m_fPrintable = false;
    m_pcwndDetachedPreview = 0;
    m_pControl = 0;
    m_hbitmap = NULL;
    m_haccel = NULL;
    m_pImgCtx = NULL;
    m_pImgCtxReady = NULL;
    m_pszFilename = NULL;
    m_hpal = NULL;
    m_hWininet = NULL;
    m_pfnInternetQueryOptionA = NULL;
    m_pfnInternetGetConnectedState = NULL;
}

CPreviewWnd::CPreviewWnd(CPreviewWnd & other) :
        m_ctlToolbar(NULL, this, 1)
{
    m_fOwnsHandles = false;         // full screen window never owns image handles
    m_fHideFullscreen = true;       // full screen window never has a full screen button
    m_fShowToolbar = true;          // full screen window always has a toolbar
    m_fAllowContextMenu = true;     // full screen window always has a context menu
    m_fAllowGoOnline = other.m_fAllowGoOnline;
    m_fHidePrintBtn = other.m_fHidePrintBtn;
    m_fPrintable = other.m_fPrintable;
    m_pcwndDetachedPreview = 0;
    m_pControl = 0;                 // full screen window never sends control events
    m_hbitmap = NULL;
    m_haccel = other.m_haccel;
    m_pImgCtx = NULL;
    m_pImgCtxReady = NULL;
    if ( other.m_pszFilename )
    {
        m_pszFilename = new TCHAR[lstrlen(other.m_pszFilename)+1];
        if ( m_pszFilename )
        {
            StrCpy(m_pszFilename,other.m_pszFilename);
        }
    }
    else
    {
        m_pszFilename = NULL;
    }
    m_hpal = other.m_hpal;

    m_hWininet = NULL;
    m_pfnInternetQueryOptionA = NULL;
    m_pfnInternetGetConnectedState = NULL;
}

CPreviewWnd::~CPreviewWnd()
{
    if ( m_pImgCtx )
    {
        m_pImgCtx->Release();
    }
    if ( m_pImgCtxReady )
    {
        m_pImgCtxReady->Release();
    }
    if ( m_fOwnsHandles )
    {
        if ( m_pcwndDetachedPreview )
        {
            if (m_pcwndDetachedPreview->m_hWnd)
                m_pcwndDetachedPreview->DestroyWindow();
            delete m_pcwndDetachedPreview;
        }
        if ( m_hbitmap )
        {
            DeleteObject( m_hbitmap );
        }
    }
    else
    {
        ASSERT( m_pcwndDetachedPreview == NULL );
    }
    if ( m_pszFilename )
    {
        delete [] m_pszFilename;
    }

    if ( m_hWininet )
    {
        FreeLibrary( m_hWininet );
    }
}

LRESULT CPreviewWnd::OnCreate(UINT , WPARAM , LPARAM , BOOL& )
{
    RECT rcWnd;
    HINSTANCE hinst = _Module.GetModuleInstance();

    GetClientRect( &rcWnd );

    m_haccel = LoadAccelerators( hinst, MAKEINTRESOURCE(IDR_PREVWND) );
    if ( !m_fOwnsHandles )
    {
        HICON hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_FULLSCREEN));
        SendMessage(WM_SETICON, ICON_BIG,   (LPARAM)hicon);
        SendMessage(WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    }

    if ( m_fShowToolbar )
    {
        // Create a toolbar control and then subclass it
        RECT rcToolbar;
        if ( !CreateToolbar() )
            return -1;

        m_ctlToolbar.GetClientRect( &rcToolbar );
        rcWnd.top = rcToolbar.bottom;
    }

    // Create the preview window
    if ( m_ctlPreview.Create(m_hWnd, rcWnd, NULL,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS))
    {
        // When the window is created it's default mode should be ZOOMIN.  This call is needed
        // because the object might have a longer life cycle than the window.  If a new window
        // is created for the same object we want to reset the state.
        m_ctlPreview.SetMode(CZoomWnd::MODE_ZOOMIN);
        return 0;
    }
    return -1;
}
    
LRESULT CPreviewWnd::OnEraseBkgnd(UINT , WPARAM , LPARAM , BOOL& )
{
    return TRUE;
}

LRESULT CPreviewWnd::OnSize(UINT , WPARAM , LPARAM lParam, BOOL& )
{
    int y  = 0;
    int cx = LOWORD( lParam );
    int cy = HIWORD( lParam );

    if ( m_fShowToolbar )
    {
        RECT rcToolbar;

        m_ctlToolbar.SendMessage(TB_AUTOSIZE, 0, 0);

        // make sure the toolbar has the correct size and location
        m_ctlToolbar.GetClientRect( &rcToolbar );
   
        // start the zoom window at the bottom of the toolbar
        y  = rcToolbar.bottom;
        // cy = hieght of rect - hieght of toolbar
        cy -= y;
    }

    ::SetWindowPos(m_ctlPreview.m_hWnd, NULL, 0,
            y, cx, cy, SWP_NOZORDER);
    return 0;
}

// OnToolbarCommand
//
// Handles WM_COMMAND messages sent from the toolbar control

LRESULT CPreviewWnd::OnToolbarCommand(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& bHandled)
{   
    switch ( wID )
    {
    case ID_ZOOMINCMD:
        // Zoom In
        if ( !m_ctlPreview.SetMode( CZoomWnd::MODE_ZOOMIN ) )
        {
            // SetMode will return false if the mode didn't change.  This means we
            // are already in the desired mode which means we should go ahead and
            // do the operations
            m_ctlPreview.ZoomIn();
        }
        else
        {
            m_ctlToolbar.SendMessage( TB_SETSTATE, ID_ZOOMOUTCMD, TBSTATE_ENABLED );
            m_ctlToolbar.SendMessage( TB_SETSTATE, ID_ZOOMINCMD, TBSTATE_ENABLED|TBSTATE_CHECKED );
        }
        break;

    case ID_ZOOMOUTCMD:
        // Zoom Out
        if ( !m_ctlPreview.SetMode( CZoomWnd::MODE_ZOOMOUT ) )
        {
            // SetMode will return false if the mode didn't change.  This means we
            // are already in the desired mode which means we should go ahead and
            // do the operation.
            m_ctlPreview.ZoomOut();
        }
        else
        {
            m_ctlToolbar.SendMessage( TB_SETSTATE, ID_ZOOMINCMD, TBSTATE_ENABLED );
            m_ctlToolbar.SendMessage( TB_SETSTATE, ID_ZOOMOUTCMD, TBSTATE_ENABLED|TBSTATE_CHECKED );
        }
        break;

    case ID_ACTUALSIZECMD:
        // Actual Size
        m_ctlPreview.ActualSize();
        break;

    case ID_BESTFITCMD:
        // Best fit
        m_ctlPreview.BestFit();
        break;

    case ID_FULLSCREENCMD:
        // The fullscreen command does nothing if we are already a full screen window,
        // oterwise you could have an infinate chain of these
        if ( GetParent() == NULL )
        {
            break;
        }

        // Full Screen
        if (m_pcwndDetachedPreview && m_pcwndDetachedPreview->m_hWnd)
        {
            // show the window and activate it
            // if the window is minimized, restore it
            if ( ::IsIconic(m_pcwndDetachedPreview->m_hWnd) )
            {
                ::ShowWindow(m_pcwndDetachedPreview->m_hWnd, SW_RESTORE);
            }
            ::SetForegroundWindow(m_pcwndDetachedPreview->m_hWnd);
        }
        else
        {
            TCHAR szTitle[MAX_PATH];

            GetFSPTitle( szTitle, MAX_PATH );

            // create the window
            if ( !m_pcwndDetachedPreview )
            {
                m_pcwndDetachedPreview = new CPreviewWnd(*this);
                if ( !m_pcwndDetachedPreview )
                {
                    // out of memory
                    break;
                }
            }

            // TODO: If the system contains multiple monitors, create the window on the
            // secondary monitor.

            RECT rc = { 0,0,640,480 };  // REVIEW: What should the default window size be?
            m_pcwndDetachedPreview->m_fPrintable = m_fPrintable;
            m_pcwndDetachedPreview->m_fHidePrintBtn = m_fHidePrintBtn;
            m_pcwndDetachedPreview->Create(
                    NULL,
                    rc,
                    szTitle,
                    WS_VISIBLE|WS_OVERLAPPEDWINDOW|WS_MAXIMIZE );

            if ( m_pImgCtxReady )
            {
                // Setting an ImgCtx should cause the Full Screen Preview to use
                // the ImgCtx palette since the FSP has a NULL m_pControl.  In
                // order to get this behavior we would need to call 
                // m_pcwndDetachedPreview->IV_OnSetImgCtx instead of bypassing
                // this and setting the zoom window directly.
                m_pcwndDetachedPreview->m_ctlPreview.SetImgCtx( m_pImgCtxReady );
            }
            else if ( m_hbitmap )
            {
                // For bitmaps it's a rather involved process to calculate the
                // palette so we will simply use the container's palette.  Perhaps
                // I should call m_pcwndDetachedPreview->IV_OnSetBitmap instead
                // so that it will calculate the correct palette based on the
                // bitmap being used.  Also, we should really be passing this
                // message through m_pcwndDetachedPreview anyway so that it's
                // object will be in the correct state.
                m_pcwndDetachedPreview->m_ctlPreview.SetBitmap( m_hbitmap );
            }
            else
            {
                m_pcwndDetachedPreview->StatusUpdate(m_ctlPreview.QueryStatus());
            }
            m_pcwndDetachedPreview->m_ctlPreview.SetPalette(m_hpal);
        }
        break;

    case ID_PRINTCMD:
        ShellExecute(m_hWnd, TEXT("print"), m_pszFilename, NULL, NULL, SW_SHOWDEFAULT );
        break;
/*
    case ID_DOCURRENTCMD:
        {
            int index;
            index = m_ctlToolbar.SendMessage( TB_GETHOTITEM );
            if ( -1 != index )
            {
                // launch a command based on the index
                ASSERT((index >= 0) && (index <= 7) && (index != 2));
                if ( index > 2 ) index--;
                OnToolbarCommand( 1, (WORD)(ID_ZOOMINCMD+index), hwnd, bHandled );
            }
        }
        break;
*/
    }
    return 0;
}       

// OnSetFocus
//
// Handles WM_SETFOCUS messages sent to the window

LRESULT CPreviewWnd::OnSetFocus(UINT , WPARAM , LPARAM , BOOL& )
{
    ATLTRACE(_T("CPreviewWnd::OnSetFocus\n"));
    if ( m_fShowToolbar )
    {
        m_ctlToolbar.SetFocus();
    }
    return 0;
}

// CreateToolbar
//
// Creates a standard windows toolbar control.
bool CPreviewWnd::CreateToolbar()
{
    HWND hwndTB;                // the main toolbar
    const int iButtonCount = 7; // the total number of buttons including seperators
    const int cxBitmap = 24;
    const int cyBitmap = 24;
    TBBUTTON tbbuttons[iButtonCount];

    HINSTANCE hinst = _Module.GetModuleInstance();
   
    // ensure that the common controls are initialized
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    hwndTB = CreateWindowEx(
            0,
            TOOLBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
            WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_TOP |
            TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
            0,0,0,0,  // Make it zero, auto resize it later.
            m_hWnd,
            NULL,
            hinst,
            NULL);

    ::SendMessage(hwndTB, CCM_SETVERSION, COMCTL32_VERSION, 0);

    // Sets the size of the TBBUTTON structure.
    ::SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    // Set the maximum number of text rows and bitmap size.
    ::SendMessage(hwndTB, TB_SETMAXTEXTROWS, 1, 0L);
    ::SendMessage(hwndTB, TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(cxBitmap, cyBitmap));

    // Create, fill, and assign the image list for default buttons.
    HBITMAP hbCold = (HBITMAP)LoadImage(hinst, MAKEINTRESOURCE(IDB_TOOLBAR),
        IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS|LR_LOADTRANSPARENT );
    HBITMAP hbMask = (HBITMAP)LoadImage(hinst, MAKEINTRESOURCE(IDB_TOOLBARMASK),
        IMAGE_BITMAP, 0, 0, LR_MONOCHROME );
    HIMAGELIST himl = ImageList_Create(cxBitmap, cyBitmap, ILC_COLOR8|ILC_MASK, 0, iButtonCount);
    ImageList_Add(himl, hbCold, hbMask);
    ::SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);
    ::DeleteObject( hbCold );

    // Create, fill, and assign the image list for hot buttons.
    HBITMAP hbHot = (HBITMAP)LoadImage(hinst, MAKEINTRESOURCE(IDB_TOOLBARHOT),
        IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS|LR_LOADTRANSPARENT );
    HIMAGELIST himlHot = ImageList_Create(cxBitmap, cyBitmap, ILC_COLOR8|ILC_MASK, 0, iButtonCount);
    ImageList_Add(himlHot, hbHot, hbMask);
    ::SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)himlHot);   
    ::DeleteObject( hbHot );
    ::DeleteObject( hbMask );

    for ( int i=0; i < iButtonCount; i++ )
    {
        switch ( i )
        {
        case 0:     // Zoom In
        case 1:     // Zoom Out
            tbbuttons[i].iBitmap    = i;
            tbbuttons[i].idCommand  = FIRST_CMDID+i;
            tbbuttons[i].fsState    = (BYTE)(TBSTATE_ENABLED | (i?0:TBSTATE_CHECKED));
            tbbuttons[i].fsStyle    = TBSTYLE_CHECKGROUP;
            tbbuttons[i].iString    = 0;
            break;

        case 2:     // Seperator
            tbbuttons[i].iBitmap    = 0;
            tbbuttons[i].idCommand  = 0;
            tbbuttons[i].fsState    = TBSTATE_ENABLED;
            tbbuttons[i].fsStyle    = TBSTYLE_SEP;
            tbbuttons[i].iString    = 0;
            break;

        case 3:     // Actual Size
        case 4:     // Best Fit
            tbbuttons[i].iBitmap    = i-1;
            tbbuttons[i].idCommand  = FIRST_CMDID+i-1;
            tbbuttons[i].fsState    = TBSTATE_ENABLED;
            tbbuttons[i].fsStyle    = TBSTYLE_BUTTON;
            tbbuttons[i].iString    = 0;
            break;

        case 5:     // Full Screen
            tbbuttons[i].iBitmap    = i-1;
            tbbuttons[i].idCommand  = FIRST_CMDID+i-1;
            tbbuttons[i].fsState    = m_fHideFullscreen?TBSTATE_HIDDEN:TBSTATE_ENABLED;
            tbbuttons[i].fsStyle    = TBSTYLE_BUTTON;
            tbbuttons[i].iString    = 0;
            break;

        case 6:     // Print Button
            tbbuttons[i].iBitmap    = i-1;
            tbbuttons[i].idCommand  = FIRST_CMDID+i-1;
            tbbuttons[i].fsState    = m_fHidePrintBtn?TBSTATE_HIDDEN:(m_fPrintable?TBSTATE_ENABLED:0);
            tbbuttons[i].fsStyle    = TBSTYLE_BUTTON;
            tbbuttons[i].iString    = 0;
            break;

        default:
            ASSERT( 0 && "Menu Item Added" );
        }

        tbbuttons[i].dwData     = 0;
    }

    // Add the buttons, and then set the minimum and maximum button widths.
    ::SendMessage(hwndTB, TB_ADDBUTTONS, (UINT)iButtonCount, (LPARAM)tbbuttons);
    ::SendMessage(hwndTB, TB_SETBUTTONWIDTH, 0, (LPARAM)MAKELONG(cxBitmap, cyBitmap));

    // Send an AUTOSIZE message to ensure that the toolbar takes on it's default size.
    ::SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);

    m_ctlToolbar.SubclassWindow( hwndTB );
    return (NULL != hwndTB);
}

LRESULT CPreviewWnd::OnEraseToolbar(UINT , WPARAM wParam, LPARAM , BOOL& )
{
    HDC hdc = (HDC)wParam;
    RECT rcFill;
    m_ctlToolbar.GetClientRect( &rcFill );

    rcFill.bottom -= 2;
    FillRect( hdc, &rcFill, (HBRUSH)(COLOR_3DFACE+1));
    
    rcFill.top = rcFill.bottom;
    rcFill.bottom += 2;
    FillRect( hdc, &rcFill, (HBRUSH)(COLOR_WINDOW+1));

    return TRUE;
}

LRESULT CPreviewWnd::OnNeedText(int , LPNMHDR pnmh, BOOL& )
{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pnmh;

    // tooltip text messages have the same string ID as the control ID
    pTTT->lpszText = MAKEINTRESOURCE(pTTT->hdr.idFrom);
    pTTT->hinst = _Module.GetModuleInstance();

    return TRUE;
}

void CPreviewWnd::SetNotify( CPreview * pControl )
{
    m_pControl = pControl;
    if ( m_pControl )
    {
        m_pControl->GetAmbientPalette( m_hpal );
    }
}

BOOL CPreviewWnd::GetPrintable( )
{
    return m_fPrintable;
}

LRESULT CPreviewWnd::OnWheelTurn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    // REVIEW: Shouldn't this just be translated into a command?

    // this message is ALWAYS forwarded to the zoom window
    m_ctlPreview.SendMessage( uMsg, wParam, lParam );
    return 0;
}

// OnKeyEvent
//
// Forwards WM_KEYUP and WM_KEYDOWN events to the zoom window but only if they are keys
// that the zoom window cares about.
LRESULT CPreviewWnd::OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch ( wParam )
    {
        case VK_SHIFT:
        case VK_CONTROL:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_HOME:
        case VK_END:
            // these are forwarded to the zoom window
            m_ctlPreview.SendMessage( uMsg, wParam, lParam );
            break;
        default:
            // these keys are left for the toolbar control (which has focus if it exists)
            break;
    }
    if ( WM_KEYDOWN == uMsg )
    {
        MSG msg;
        msg.hwnd = m_hWnd;
        msg.message = uMsg;
        msg.wParam = wParam;
        msg.lParam = lParam;

        if ( TranslateAccelerator( &msg ) )
        {
            return 0;
        }
    }
    bHandled = FALSE;
    return 0;
}

void CALLBACK ImgCtxCallback( void *, VOID * pv )
{
    HWND hwnd = (HWND)pv;
    ::PostMessage(hwnd, IV_SETIMGCTX, 0, 0);
}

HRESULT CPreviewWnd::PreviewFromFile(LPCTSTR pszFilename, INT * iResultReturnCode)
{
    WCHAR szURL[MAX_PATH + 7]; // path + "file://"
    HRESULT hr;

    if ( !pszFilename )
    {
        *iResultReturnCode = IDS_BADFILENAME;
        return E_INVALIDARG;
    }

    if ( lstrlen( pszFilename ) > MAX_PATH )
    {
        *iResultReturnCode = IDS_BADFILENAME;
        return E_FAIL;
    }

    ASSERT( 0==m_pImgCtx );

    // Check to ensure we don't accidentally go online.  If the m_fAllowGoOnline flag is set then
    // we don't need to do this check because we're supposed to go online if needed.
    if ( !m_fAllowGoOnline && PathIsURL( pszFilename ) )
    {
        if ( !m_hWininet )
        {
            m_hWininet = LoadLibrary(TEXT("wininet.dll"));
        }

        if ( m_hWininet )
        {
            if ( !m_pfnInternetQueryOptionA )
            {
                m_pfnInternetQueryOptionA = (IQOFN)GetProcAddress(m_hWininet, "InternetQueryOptionA");
            }

            if ( !m_pfnInternetGetConnectedState )
            {
                m_pfnInternetGetConnectedState = (IGCSFN)GetProcAddress(m_hWininet, "InternetGetConnectedState");
            }

            if ( m_pfnInternetQueryOptionA && m_pfnInternetGetConnectedState )
            {
                // assume we are in "online" mode
                BOOL fOffline = FALSE;

                DWORD dwState, dwSize = sizeof( dwState );

                // check the global online/offline mode...
                if (m_pfnInternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
                {
                    if (dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
                        fOffline = TRUE;
                }

                // we still think we are connected, then check to see if we really have a connection
                if ( !fOffline )
                {
                    DWORD dwFlags;
                    DWORD dwMask = INTERNET_CONNECTION_MODEM | INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_PROXY;
                    dwFlags = dwMask;
                    if ( m_pfnInternetGetConnectedState( &dwFlags, 0 ) && ( dwFlags & dwMask ))
                    {
                        // we are connected
                    }
                    else
                    {
                        // no connection even though we are not in offline mode...
                        fOffline = TRUE;
                    }
                }

                if ( fOffline )
                {
                    // don't go online to fetch the image
                    *iResultReturnCode = IDS_SETUPFAILED;
                    return E_FAIL;
                }
            }
        }
    }

    SHTCharToUnicode(pszFilename, szURL, ARRAYSIZE(szURL));
    DWORD cbSize = ARRAYSIZE(szURL);
    hr = UrlCreateFromPathW(szURL, szURL, &cbSize, 0);
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_IImgCtx, NULL, CLSCTX_INPROC_SERVER, IID_IImgCtx, (LPVOID*)&m_pImgCtx);
        if (SUCCEEDED(hr))
        {
            // start loading the new image
            hr = m_pImgCtx->Load(szURL, 0);
            if ( SUCCEEDED( hr ))
            {
                // set the function that will handle downloads
                hr = m_pImgCtx->SetCallback( ImgCtxCallback, m_hWnd );
                if ( SUCCEEDED( hr ))
                {
                    // set to recieve Completion notifications
                    hr = m_pImgCtx->SelectChanges( IMGCHG_COMPLETE, 0, TRUE);
                }
            }
        }
    }

    if ( FAILED( hr ))
    {
        *iResultReturnCode = IDS_SETUPFAILED;
    }
    else
    {
        *iResultReturnCode = IDS_LOADING;
    }

    return hr;
}

LRESULT CPreviewWnd::IV_OnSetImgCtx(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;
    SIZE  rgSize;
    DWORD fState;

    // By the time we recieve this message there are several possibilities.  We could have
    // released m_pImgCtx and it's now NULL, or we could have released and created a new
    // ImgCtx in which case it's not NULL but it also isn't finished rendering yet.  In the
    // case where m_pImgCtx is NULL,

    if ( m_pImgCtx )
    {
        hr = m_pImgCtx->GetStateInfo(&fState, &rgSize, FALSE);

        if ( SUCCEEDED(hr) && (fState & IMGLOAD_COMPLETE) )
        {
            if ( m_pImgCtxReady )
            {
                m_pImgCtxReady->Release();
            }
            m_pImgCtxReady = m_pImgCtx;
            m_pImgCtx = NULL;

            // this message indicates a successful result
            m_ctlPreview.SetImgCtx( m_pImgCtxReady );

            // if we are a control we always use the containers ambient palette.  Only if we
            // are not a control do we instead use the IImgCtx palette (which happens to be
            // the halftone palette which happens to be the same as the containers ambient
            // palette in most, but not all, cases).
            if (!m_pControl)
            {
                m_pImgCtxReady->GetPalette(&m_hpal);
            }
            // Even if m_hpal is NULL it is still correct, so we always go ahead and set it.
            m_ctlPreview.SetPalette(m_hpal);

            if ( m_pcwndDetachedPreview && m_pcwndDetachedPreview->m_hWnd )
            {
                TCHAR szTitle[MAX_PATH];
                m_pcwndDetachedPreview->SetWindowText( GetFSPTitle(szTitle,MAX_PATH) );
                m_pcwndDetachedPreview->m_ctlPreview.SetImgCtx( m_pImgCtxReady );

                m_pcwndDetachedPreview->m_ctlPreview.SetPalette(m_hpal);
            }

            // Notify anyone listening to our events that a preview has been completed
            // we only fire this upon success
            if ( m_pControl )
            {
                m_pControl->OnPreviewReady();
            }

            return TRUE;
        }
    }
    else
    {
        // This should be impossbile, but is likely enough to happen.  For now I want to
        // look at what cases cause this to happen but eventually this assert should be
        // removed and we should consider this an acceptable possibility.
        ASSERT(0);
    }

    // this message means an error occured, wParam is which error.
    if ( m_pcwndDetachedPreview )
    {
        m_pcwndDetachedPreview->m_ctlPreview.StatusUpdate(IDS_LOADFAILED);
    }
    m_ctlPreview.StatusUpdate(IDS_LOADFAILED);

    // REVIEW: Would it be helpful to fire an event in the case of an error?
    // This is most likely useful but isn't needed right now.
    // if ( m_pControl )
    //     m_pControl->OnError();

    return TRUE;
}

// GetFSPTitle
//
// This function take the name of the file being previewed and converts it into a
// title for the Full Screen Preview window.  In converting the title it take into
// account user preference settings for how to display the filename.
TCHAR * CPreviewWnd::GetFSPTitle( TCHAR * szTitle, int cchMax )
{
    TCHAR * szTemp;
    int cch = 0;
    SHELLFLAGSTATE sfs;

    if ( m_pszFilename && *(m_pszFilename) )
    {
        StrCpyN(szTitle, PathFindFileName(m_pszFilename), cchMax);
        SHGetSettings( &sfs, SSF_SHOWEXTENSIONS );
        if ( !sfs.fShowExtensions )
        {
            // remove the file extention
            *PathFindExtension(szTitle) = NULL;
        }

        StrNCat( szTitle, _T(" - "), cchMax );
        cch = lstrlen( szTitle );
    }
    szTemp = szTitle + cch;
    LoadString( _Module.GetModuleInstance(), IDS_PROJNAME, szTemp, cchMax - cch );

    return szTitle;
}

// FlushBitmapMessages
//
// Creation of the bitmaps via IImgCtx is asynchronous.  When our callback function
// is called it respondes by placing the new bitmap handle into a message and posting
// it to the window.  As a result, we must flush these messages when the window is
// destroyed to prevent leaking any handles.

void CPreviewWnd::FlushBitmapMessages()
{
    MSG msg;

    // IV_SETBITMAP and IV_SETIMGCTX must be consecutive for this peek message
    // to be valid
    ASSERT( IV_SETIMGCTX == IV_SETBITMAP+1 );

    while ( PeekMessage(&msg, m_hWnd, IV_SETBITMAP, IV_SETIMGCTX, PM_REMOVE) )
    {
        if ( msg.message == IV_SETBITMAP )
        {
            HBITMAP hbm = (HBITMAP)(msg.lParam);
            if ( hbm )
            {
                DeleteObject( hbm );
            }
        }
    }
}

LRESULT CPreviewWnd::OnDestroy(UINT , WPARAM , LPARAM , BOOL& bHandled)
{
    FlushBitmapMessages();
    bHandled = FALSE;

    // release the image lists used by the toolbar.
    HWND hwndTB = m_ctlToolbar.m_hWnd;
    HIMAGELIST himl = (HIMAGELIST)::SendMessage(hwndTB, TB_GETHOTIMAGELIST, 0, 0);
    ::SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, NULL);
    ImageList_Destroy(himl);

    himl = (HIMAGELIST)::SendMessage(hwndTB, TB_GETIMAGELIST, 0, 0);
    ::SendMessage(hwndTB, TB_SETIMAGELIST, 0, NULL);
    ImageList_Destroy(himl);

    return 0;
}

LRESULT CPreviewWnd::OnContextMenu(UINT , WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;
    if ( ((HWND)wParam != m_hWnd) && m_fAllowContextMenu )
    {
        HMENU hmenu = LoadMenu( _Module.GetModuleInstance(), MAKEINTRESOURCE(IDM_CONTEXTMENU) );
        if ( hmenu )
        {
            HMENU hpopup = GetSubMenu( hmenu, 0 );

            if ( hpopup )
            {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);

                if ((0xFFFFFFFF == lParam))
                {
                    // message is from the keyboard, figure out where to place the window
                    RECT rc;
                    m_ctlPreview.GetWindowRect( &rc );
                    x = rc.left;
                    y = rc.top;
                }

                int pos = (int)m_ctlToolbar.SendMessage( TB_GETSTATE, ID_ZOOMOUTCMD, 0 ) & TBSTATE_CHECKED;
                CheckMenuRadioItem( hpopup, 0, 1, pos, MF_BYPOSITION );

                if ( m_fHideFullscreen )
                {
                    RemoveMenu( hpopup, ID_FULLSCREENCMD, MF_BYCOMMAND );
                }

                if ( m_fHidePrintBtn )
                {
                    RemoveMenu( hpopup, ID_PRINTCMD, MF_BYCOMMAND );
                }
                else if ( !m_fPrintable )
                {
                    EnableMenuItem(hpopup, ID_PRINTCMD, MF_BYCOMMAND|MF_GRAYED );
                }

                bHandled = TRUE;
                TrackPopupMenuEx(hpopup, 0, x, y, m_hWnd, NULL);
            }
            DestroyMenu( hmenu );
        }
    }
    return 0;
}

int CPreviewWnd::TranslateAccelerator( LPMSG lpmsg )
{
    if ( m_haccel )
    {
        return ::TranslateAccelerator( m_hWnd, m_haccel, lpmsg );
    }
    return FALSE;
}

// StatusUpdate
//
// Sent when the image generation status has changed, once when the image is first
// being created and again if there is an error of any kind.  This should invalidate
// and free any left over bitmap and the cached copy of the previous m_ImgCtx
void CPreviewWnd::StatusUpdate( int iStatus )
{
    if ( m_hbitmap )
    {
        if ( m_fOwnsHandles )
            DeleteObject( m_hbitmap );
        m_hbitmap = 0;
    }

    if ( m_pImgCtxReady )
    {
        m_pImgCtxReady->Release();
        m_pImgCtxReady = NULL;
    }

    m_ctlPreview.StatusUpdate( iStatus );
}


// OnSetBitmap
//
// This function is called in response to a IV_SETBITMAP message.  These messages are status
// messages for the asynchronous image generation.  lParam contains the HBITMAP for the
// new image if everything was successful.  If lParam is NULL then wParam contains a failure
// code, otherwise wParam contains the packed width and height of the bitmap.
LRESULT CPreviewWnd::IV_OnSetBitmap(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    // if we already have a bitmap, nuke it.
    if ( m_hbitmap && m_fOwnsHandles )
    {
        DeleteObject( m_hbitmap );
    }

    // we store the bitmap so that we can delete it or pass it to an external window later
    m_hbitmap = (HBITMAP)lParam;

    if ( m_hbitmap )
    {
        // this message indicates a successful result
        m_ctlPreview.SetBitmap( m_hbitmap );

        if (!m_pControl)
        {
            // TODO: At this point we should construct a palette based on the colors used
            // in the bitmap.
            // CalculatePaletteFromBitmap();
        }
        // Even if m_hpal is NULL it is still correct, so we always go ahead and set it.
        m_ctlPreview.SetPalette(m_hpal);

        if ( m_pcwndDetachedPreview && m_pcwndDetachedPreview->m_hWnd )
        {
            TCHAR szTitle[MAX_PATH];
            m_pcwndDetachedPreview->SetWindowText( GetFSPTitle(szTitle,MAX_PATH) );
            m_pcwndDetachedPreview->SendMessage(uMsg, wParam, lParam);
        }
    }
    else
    {
        // this message means an error occured, wParam is which error.
        if ( m_pcwndDetachedPreview )
        {
            m_pcwndDetachedPreview->m_ctlPreview.StatusUpdate((int)wParam);
        }
        m_ctlPreview.StatusUpdate((int)wParam);
    }

    // Notify anyone listening to our events that a preview has been completed
    // we only fire this upon success
    if ( m_pControl )
    {
        if ( m_hbitmap )
        {
            m_pControl->OnPreviewReady();
        }
        // REVIEW: Would it be helpful to fire an event in the case of an error?
        // This is most likely useful but isn't needed right now.
        // else
        //     m_pControl->OnError();
    }

    return 0;
}

LRESULT CPreviewWnd::IV_OnShowFileA(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR szFilename[MAX_PATH];
    SHAnsiToTChar( lParam ? (LPSTR)lParam : "", szFilename, ARRAYSIZE(szFilename) );

    return OnShowFile(szFilename, wParam);
}

LRESULT CPreviewWnd::IV_OnShowFileW(UINT , WPARAM wParam, LPARAM lParam, BOOL& )
{
    TCHAR szFilename[MAX_PATH];
    SHUnicodeToTChar( lParam ? (LPWSTR)lParam : L"", szFilename, ARRAYSIZE(szFilename) );

    return OnShowFile(szFilename, wParam);
}

LRESULT CPreviewWnd::OnShowFile(LPTSTR pszFilename, WPARAM iCount)
{
    HRESULT hr = S_FALSE;
    int iRetCode;

    // cancel any previous request
    if ( m_pImgCtx )
    {
        m_pImgCtx->Release();
        m_pImgCtx = NULL;

    }

    // It is possible that there is already a bitmap message in our queue form the previous rendering.
    // If this is the case we should remove that message and release it's bitmap before we continue.
    // If we do not then that message will get processed and will send the OnPreviewReady event to the
    // obejct container but this event might no longer be valid.
    FlushBitmapMessages();

    // update m_pszFilename:
    if ( m_pszFilename )
    {
        delete [] m_pszFilename;
        m_pszFilename = NULL;
    }


    if (pszFilename && *pszFilename)
    {
        // We store the original filename
        m_pszFilename = new TCHAR[lstrlen(pszFilename)+1];
        if ( m_pszFilename )
        {
            StrCpy(m_pszFilename, pszFilename );
        }

        hr = PreviewFromFile( pszFilename, &iRetCode );
    }
    else
    {
        if ( iCount > 1 )
        {
            iRetCode = IDS_MULTISELECT;
        }
        else
        {
            iRetCode = IDS_NOPREVIEW;
        }
    }

    // Set the Return Code into all owned zoom windows.  This instucts these windows to disregard
    // their previous m_hBitmap's and display the status message instead.
    if ( m_pcwndDetachedPreview )
    {
        m_pcwndDetachedPreview->StatusUpdate(iRetCode);
    }
    StatusUpdate(iRetCode);

    return hr;
}

LRESULT CPreviewWnd::IV_OnZoom(UINT , WPARAM wParam, LPARAM lParam, BOOL& )
{
    m_ctlPreview.Zoom(wParam, lParam);
    return 0;
}

LRESULT CPreviewWnd::IV_OnIVScroll(UINT , WPARAM , LPARAM lParam, BOOL& )
{
    DWORD nHCode = LOWORD(lParam);
    DWORD nVCode = HIWORD(lParam);
    if (nHCode)
    {
        m_ctlPreview.SendMessage( WM_HSCROLL, nHCode, NULL );
    }
    if (nVCode)
    {
        m_ctlPreview.SendMessage( WM_VSCROLL, nVCode, NULL );
    }
    // TODO: It would be nice to return a bitmask that gives some status about the viewable
    // region of the image, such as IVS_ATLEFT, _ATRIGHT, _ATTOP, and _ATBOTTOM to show
    // which directions the image can still be scrolled.
    return 0;
}

LRESULT CPreviewWnd::IV_OnBestFit(UINT , WPARAM , LPARAM , BOOL& )
{
    m_ctlPreview.BestFit();
    return 0;
}

LRESULT CPreviewWnd::IV_OnActualSize(UINT , WPARAM , LPARAM , BOOL& )
{
    m_ctlPreview.ActualSize();
    return 0;
}

// IV_OnSetOptions
//
// This message is sent to turn on or off all the optional features of the image preview control.
// NOTE: When used as a control this function is called BEFORE the window is created.  Don't do
// anything in this function that will fail without a window unless you check for this condition.
LRESULT CPreviewWnd::IV_OnSetOptions(UINT , WPARAM wParam, LPARAM lParam, BOOL& )
{
    BOOL bResult = TRUE;

    // Boolify lParam just to be safe.
    lParam = lParam?1:0;

    switch (wParam)
    {
    case IVO_TOOLBAR:
        if ( (BOOL)lParam != m_fShowToolbar )
        {
            m_fShowToolbar = (BOOL)lParam;
            if ( m_hWnd )
            {
                RECT rect;
                BOOL bDummy = TRUE;
                if ( m_fShowToolbar )
                {
                    if ( !m_ctlToolbar )
                    {
                        bResult = CreateToolbar();
                        if ( !bResult )
                        {
                            // toolbar creation failed
                            m_fShowToolbar = false;
                            // because the toolbar creation failed there's no reason to continue
                            break;
                        }
                    }
                }
                else
                {
                    if ( m_ctlToolbar )
                    {
                        m_ctlToolbar.DestroyWindow();
                    }
                }
                // Force a phony resize.  This will cause our otherwise hidden toolbar to be displayed
                GetClientRect(&rect);
                OnSize( WM_SIZE, 0, MAKELONG(rect.right,rect.bottom), bDummy );
            }
        }
        break;

    case IVO_PRINTBTN:
        if ( (BOOL)lParam != m_fHidePrintBtn )
        {
            m_fHidePrintBtn = (BOOL)lParam;
            if ( m_hWnd && m_ctlToolbar )
            {
                m_ctlToolbar.SendMessage( TB_HIDEBUTTON,ID_PRINTCMD,lParam );
            }
            // Print button state is reflected on full screen preview windows.  This isn't true
            // for the hidden/shown state of other buttons.
            if ( m_pcwndDetachedPreview )
            {
                m_pcwndDetachedPreview->SendMessage(IV_SETOPTIONS,wParam,lParam);
            }
        }
        break;

    case IVO_FULLSCREENBTN:
        if ( (BOOL)lParam != m_fHideFullscreen )
        {
            m_fHideFullscreen = (BOOL)lParam;
            if ( m_hWnd && m_ctlToolbar )
            {
                m_ctlToolbar.SendMessage( TB_HIDEBUTTON,ID_FULLSCREENCMD,lParam );
            }
            // REVIEW: If a full screen preview is currently open should we close it?  I think not.
        }
        break;

    case IVO_CONTEXTMENU:
        m_fAllowContextMenu = (BOOL)lParam;
        break;

    case IVO_PRINTABLE:
        m_fPrintable = (BOOL)lParam;
        if ( m_hWnd && m_ctlToolbar )
        {
            m_ctlToolbar.SendMessage(TB_ENABLEBUTTON,ID_PRINTCMD,lParam);
        }
        // This changes whenever a new file is selected.  Any open full screen window needs to
        // get updated with this new information.
        if ( m_pcwndDetachedPreview )
        {
            m_pcwndDetachedPreview->SendMessage(IV_SETOPTIONS,wParam,lParam);
        }
        break;

    case IVO_ALLOWGOONLINE:
        m_fAllowGoOnline = (BOOL)lParam;
        break;

    default:
        // TODO: output a debug trace warning thingy
        break;
    }

    return bResult;
}
