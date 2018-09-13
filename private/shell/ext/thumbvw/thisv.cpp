 /* sample source code for IE4 view extension
 *
 * Copyright Microsoft 1996
 *
 * This file implements the IShellView2 interface
 */

#include "precomp.h"
#include "wininet.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetWindow ( HWND * lphwnd)
{
    HRESULT hr = NOERROR;

    if ( m_hWnd != NULL )
    {
        *lphwnd = m_hWnd;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::ContextSensitiveHelp ( BOOL fEnterMode)
{
    return E_NOTIMPL;
}

//***   IsVK_TABCycler -- is key a TAB-equivalent
// ENTRY/EXIT
//  dir     0 if not a TAB, non-0 if a TAB
// NOTES
//  NYI: -1 for shift+tab, 1 for tab
//
int IsVK_TABCycler(MSG *pMsg)
{
    int nDir = 0;

    if (!pMsg)
        return nDir;

    if (pMsg->message != WM_KEYDOWN)
        return nDir;
    if (! (pMsg->wParam == VK_TAB || pMsg->wParam == VK_F6))
        return nDir;

    nDir = (GetKeyState(VK_SHIFT) < 0) ? -1 : 1;

    return nDir;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::TranslateAccelerator ( LPMSG lpmsg)
{
    HRESULT hr = S_FALSE; // Default has it that message isn't processed.

    if ( m_fTranslateAccel )
    {
        // in label edit mode, disabled explorers accelerator
        // process this msg so the exploer does not get to translate
        TranslateMessage(lpmsg);
        DispatchMessageWrapW(lpmsg);
        hr = S_OK;
    }

    else if ( m_hAccel )
    {
        // this table needs to be reduced for the new view as the def-view should pick up
        // most keyboard accelerators...
        if ( ::TranslateAcceleratorWrapW( m_hWnd, m_hAccel, lpmsg ) )
        {
            hr = S_OK;
        }
    }
    else if (IsVK_TABCycler(lpmsg))
    {
        SHELLSTATE  ss;

        // If it's a <tab> with WebView off and we don't have focus then take focus.
        // Trident handles the WebView on focus case.

        SHGetSetSettings(&ss, SSF_WEBVIEW, FALSE);
        if (!ss.fWebView && (GetFocus() != m_hWndListView))
        {
            SetFocus(m_hWndListView);
        }
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::EnableModeless ( BOOL fEnable)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::UIActivate ( UINT uState)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::Refresh ()
{
    // empty the view
    Clear( m_hWndListView );

    CheckViewOptions();

    // remove all tasks currently in the queue ..
    m_pScheduler->Status( ITSSFLAG_KILL_ON_DESTROY, ITSS_THREAD_TIMEOUT_NO_CHANGE );

    m_pScheduler->RemoveTasks( TOID_NULL, ITSAT_DEFAULT_LPARAM, TRUE );

    // clear out the thumbnail cache...
    Assert( m_pImageCache );
    m_pImageCache->Flush( FALSE );

    // this needs moving on the background somehow....
    EnumFolder( );

    SortBy( m_iSortBy, TRUE );
    FocusOnSomething();
    UpdateStatusBar();

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::CreateViewWindow ( IShellView  *lpPrevView,
                                                LPCFOLDERSETTINGS lpfs,
                                                IShellBrowser  * psb,
                                                RECT * prcView,
                                                HWND *phWnd )
{
    // delegate the call to the new IShellView2 method ..

    SV2CVW2_PARAMS cParams;

    cParams.cbSize   = sizeof(SV2CVW2_PARAMS);
    cParams.psvPrev  = lpPrevView;
    cParams.pfs      = lpfs;
    cParams.psbOwner = psb;
    cParams.prcView  = prcView;
    cParams.pvid     = NULL;

    HRESULT hr = CreateViewWindow2( &cParams );

    *phWnd = cParams.hwndView;

    return (hr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::DestroyViewWindow ()
{
    // flag we are destroying.....
    m_fDestroying = TRUE;

    if ( m_pScheduler )
    {
        m_pScheduler->Status( ITSSFLAG_KILL_ON_DESTROY, ITSS_THREAD_TIMEOUT_NO_CHANGE );
        m_pScheduler->Release();
        m_pScheduler = NULL;
    }

    if ( m_pDiskCache )
    {
        // at this point we assume that we have no lock,
        m_pDiskCache->Close( NULL );
        m_pDiskCache->Release();
        m_pDiskCache = NULL;
    }

    // release ourselves from the drag drop ..
    RevokeDragDrop( m_hWndListView );

    Assert( m_pDropTarget );
    m_pDropTarget->Release();
    m_pDropTarget = NULL;

    if (m_pDragImages)
    {
        m_pDragImages->Release();
        m_pDragImages = NULL;
    }

    // free the global lock on this object !!
    IUnknown *pUnk = NULL;
    HRESULT hr = ((IShellView2 *)this)->QueryInterface( IID_IUnknown, (void **) & pUnk );
    Assert( SUCCEEDED( hr ));

    CoLockObjectExternal( pUnk, FALSE, FALSE );
    pUnk->Release();

    // NULL out the HWNDs, so that if we are accidentally in label edit mode, we know to fail quick 
    // and gracefully...
    HWND hwndTemp = m_hWnd;
    HWND hwndListViewTemp = m_hWndListView;
    m_hWnd = NULL;
    m_hWndListView = NULL;
    
    Clear( hwndListViewTemp );

    SetAutomationObject(NULL);
    
    // destroy the window, this should inturn destroy the Listview
    DestroyWindow( hwndTemp );

    return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetCurrentInfo ( LPFOLDERSETTINGS lpfs)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::AddPropertySheetPages ( DWORD dwReserved,
                                                     LPFNADDPROPSHEETPAGE lpfn,
                                                     LPARAM lparam)
{
    /*[TODO: need to add one here ]*/
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SaveViewState ()
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SelectItem ( LPCITEMIDLIST pidlItem,
                                          UINT uFlags)
{
    if (!pidlItem)
    {
        if (uFlags != SVSI_DESELECTOTHERS)
        {
            // I only know how to deselect everything
            return(E_INVALIDARG);
        }

        ListView_SetItemState(m_hWndListView, -1, 0, LVIS_SELECTED);
        return(S_OK);
    }

    int i = FindItem(pidlItem);
    if (i != -1)
    {
        // The SVSI_EDIT flag also contains SVSI_SELECT and as such
        // a simple & wont work!
        if ((uFlags & SVSI_EDIT) == SVSI_EDIT)
        {
            // Grab focus if the listview (or any of it's children) don't already have focus
            HWND hwndFocus = GetFocus();
            if (!hwndFocus ||
                !(hwndFocus == m_hWndListView || IsChild(m_hWndListView, hwndFocus)))
            {
                SetFocus(m_hWnd);
            }
            ListView_EditLabel(m_hWndListView, i);
        }
        else
        {
            UINT stateMask = LVIS_SELECTED;
            UINT state = (uFlags & SVSI_SELECT) ? LVIS_SELECTED : 0;
            if (uFlags & SVSI_FOCUSED)
            {
                state |= LVIS_FOCUSED;
                stateMask |= LVIS_FOCUSED;
            }

            // See if we should first deselect everything else
            if (uFlags & SVSI_DESELECTOTHERS)
                ListView_SetItemState(m_hWndListView, -1, 0, LVIS_SELECTED);

            ListView_SetItemState(m_hWndListView, i, state, stateMask);

            if (uFlags & SVSI_ENSUREVISIBLE)
                ListView_EnsureVisible(m_hWndListView, i, FALSE);
        }

        return S_OK;
    }

    return (E_FAIL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetItemObject ( UINT uItem,
                                             REFIID riid,
                                             void **ppv)
{
    if ( ppv == NULL )
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;

    switch( uItem )
    {
        case SVGIO_BACKGROUND:
        {
            if ( riid == IID_IContextMenu )
            {
                return this->CreateBackgroundMenu( (LPCONTEXTMENU *) ppv );
            }
        }
        break;

        case SVGIO_SELECTION:
        {
            // use the context menu wrapper so that we get the capture item on the
            // file menu as well....
            int iCount = ListView_GetSelectedCount( m_hWndListView );
            if ( iCount == 0 )
            {
                return E_FAIL;
            }

            LPCITEMIDLIST * apidl = new LPCITEMIDLIST[iCount];
            GetSelectionPidlList( m_hWndListView, iCount, apidl, -1 );
            UINT uiFlags = 0;
            HRESULT hr = NOERROR;

            if ( riid == IID_IContextMenu )
            {
                CComObject<CThumbnailMenu> * pMenuTmp = new CComObject<CThumbnailMenu>;
                if ( pMenuTmp == NULL )
                {
                    return E_OUTOFMEMORY;
                }

                hr = pMenuTmp->QueryInterface( riid, ppv );
                if ( SUCCEEDED( hr ))
                {
                    hr = pMenuTmp->Init( this, & uiFlags, apidl, iCount );
                }

                if ( FAILED( hr ))
                {
                    delete pMenuTmp;
                }
            }
            else
            {
                hr = m_pFolder->GetUIObjectOf( m_hWnd,
                                               iCount,
                                               apidl,
                                               riid,
                                               &uiFlags,
                                               ppv );
            }

            delete [] apidl;
            return hr;
        }
        break;

        case SVGIO_ALLVIEW:
            /*[TODO: this needs doing if there is a thread for background extraction ]*/
        break;
    }

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetView ( SHELLVIEWID* pvid, ULONG uView)
{
    if ( pvid == NULL )
    {
        return E_INVALIDARG;
    }

    // as a view extension, if we get called, just return the VID for
    // our view....
    *pvid = VID_Thumbnails;
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::CreateViewWindow2 ( LPSV2CVW2_PARAMS lpParams)
{
    if ( m_pFolderCB == NULL )
    {
        // just incase so we don't die horribly......
        return E_FAIL;
    }

    Assert( lpParams != NULL );
    if ( lpParams == NULL )
    {
        return E_INVALIDARG;
    }

    Assert( lpParams->psbOwner != NULL );
    Assert( lpParams->psvPrev != NULL );

    // do some funky stuff to get the arrange parameter from the defview
    DWORD   dwSortBy = DETAILSCOL_DEFAULT;
    IShellFolderView *pSFV;
    HRESULT hr = lpParams->psvPrev->QueryInterface( IID_IShellFolderView, ( void ** )&pSFV );
    if ( SUCCEEDED( hr ) )
    {
        LPARAM  lParam;
        pSFV->GetArrangeParam( &lParam );
        dwSortBy = ( DWORD ) lParam;
        pSFV->Release( );
    }

    if ( lpParams->pvid != NULL )
    {
        // are we are being asked to create a view which isn't ours ?
        if ( *(lpParams->pvid) != VID_Thumbnails )
            return E_INVALIDARG;
    }

    // get the parent window from the IShellView of the def-view
    m_pDefView = lpParams->psvPrev;
    m_pDefView->AddRef();

    hr = m_pDefView->GetWindow( &m_hWndParent );
    Assert( SUCCEEDED( hr ));
    Assert( m_hWndParent != NULL );

    // cache the setting so we can use them on our child window...
    m_rgSettings = *(lpParams->pfs);

    RECT * prcRect = lpParams->prcView;

    // create the image cache (before we do the CreateWindow)....
    hr = CoCreateInstance( CLSID_ImageListCache, NULL, CLSCTX_INPROC, 
                           IID_IImageCache2, (void **) &m_pImageCache );
    if ( FAILED( hr ))
    {
        return hr;
    }
    UINT uBytesPerPix;
    IMAGECACHEINITINFO rgInfo;
    rgInfo.cbSize = sizeof( rgInfo );
    rgInfo.dwMask = ICIIFLAG_LARGE;
    rgInfo.rgSizeLarge.cx = m_iXSizeThumbnail;
    rgInfo.rgSizeLarge.cy = m_iYSizeThumbnail;
    rgInfo.iStart = 0;
    rgInfo.iGrow = 5;
    m_dwRecClrDepth = rgInfo.dwFlags = GetCurrentColorFlags( &uBytesPerPix );

    hr = m_pImageCache->GetImageList( &rgInfo );
    if ( FAILED( hr ))
    {
        return hr;
    }

    m_himlThumbs = rgInfo.himlLarge;
    Assert( m_himlThumbs );

    // GetImageList() will return S_FALSE if it was already created...
    if ( m_dwRecClrDepth <= 8 )
    {
        // init the color table so that it matches The "special halftone palette"
        m_hpal = SHCreateShellPalette( NULL );
        PALETTEENTRY rgColours[256];
        RGBQUAD rgDIBColours[256];

        Assert( m_hpal );
        int nColours = GetPaletteEntries(m_hpal, 0, ARRAYSIZE(rgColours), rgColours );

        // SHGetShellPalette should always return a 256 colour palette
        Assert( nColours == ARRAYSIZE( rgColours ));

        // translate from the LOGPALETTE structure to the RGBQUAD structure ...
        for ( int iColour = 0; iColour < nColours; iColour ++ )
        {
            rgDIBColours[iColour].rgbRed = rgColours[iColour].peRed;
            rgDIBColours[iColour].rgbBlue = rgColours[iColour].peBlue;
            rgDIBColours[iColour].rgbGreen = rgColours[iColour].peGreen;
            rgDIBColours[iColour].rgbReserved = 0;
        }

        ImageList_SetColorTable( m_himlThumbs, 0, nColours, rgDIBColours );
    }

    m_iMaxCacheSize = CalcCacheMaxSize( &rgInfo.rgSizeLarge, uBytesPerPix );

    // create everything because we are a new window .....
    DWORD   dwStyle =  WS_CHILD | WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    if (!(m_rgSettings.fFlags & FWF_NOVISIBLE))
        dwStyle |= WS_VISIBLE;

    m_hWnd = CreateWindowExWrapW( 0, VIEWCLASSNAME, L"",
        dwStyle,
        prcRect->left, prcRect->top,
        prcRect->right - prcRect->left,
        prcRect->bottom - prcRect->top,
        m_hWndParent, NULL, g_hinstDll, this );

    if ( m_hWnd == NULL )
    {
        return E_FAIL;
    }

    UpdateWindow( m_hWnd );

    // hold a ref to the browser interface
    lpParams->psbOwner->AddRef();
    m_pBrowser = lpParams->psbOwner;

    // check to see if we are in the common dialog, if so, get the interface ...
    // ignore the return result, the pointer will still be null if it
    // fails
    m_pBrowser->QueryInterface( IID_ICommDlgBrowser, (void **) & m_pCommDlg );

    HWND hWndTree = NULL;

    // check to see if we are in explore mode ..
    hr = m_pBrowser->GetControlWindow( FCW_TREE, & hWndTree );

    m_fExploreMode = ( hr == NOERROR && hWndTree != NULL );

    // NOTE: the DefView owns the Toolbar and the menus, so we do not alter these
    // NOTE: in a view extension

    // ask the folder for the pidl...
    IPersistFolder2 * ppf2;
    hr = m_pFolder->QueryInterface( IID_IPersistFolder2, (void **) & ppf2 );
    if (SUCCEEDED(hr))
    {
        ppf2->GetCurFolder(&m_pidl);
        ppf2->Release();
    }
    else
    {
        // ask the callback for the pidl ...
        hr = m_pFolderCB->MessageSFVCB( SFVM_THISIDLIST, NULL, (LPARAM) &m_pidl );
        if ( FAILED( hr ))
        {
            SHChangeNotifyEntry rgEntry;
            UINT uFlags  = 0;
            m_ulShellRegId = 0;

            // ask the callback about notifications ...
            // this is another way of getting the pidl....
            hr = m_pFolderCB->MessageSFVCB( SFVM_GETNOTIFY,
                                            (WPARAM) &( rgEntry.pidl ),
                                            (LPARAM) &uFlags );
            if ( FAILED( hr ) || !rgEntry.pidl )
            {
                return E_FAIL;
            }
            // cache the pidl...
            m_pidl = ILClone( rgEntry.pidl );
        }
    }

    hr = CoCreateInstance( CLSID_ShellThumbnailDiskCache, NULL, CLSCTX_INPROC,
                           IID_IShellImageStore, (void **) & m_pDiskCache );
    if ( SUCCEEDED( hr ))
    {
        IPersistFolder * ppf;
        hr = m_pDiskCache->QueryInterface( IID_IPersistFolder, (void **) & ppf );
        if (SUCCEEDED(hr))
        {
            hr = ppf->Initialize( m_pidl );
            ppf->Release();
        }

        if ( FAILED( hr ))
        {
            // can't get the disk cache, live without it...
            m_pDiskCache->Release();
            m_pDiskCache = NULL;
        }
    }

    // do the enum after we have registered the cache's full path
    EnumFolder();
    SortBy( dwSortBy );
    m_iSortBy = (int) dwSortBy;

    IDropTarget *pDrop = NULL;      // create a tearoff DropTarget

    hr = NOERROR;
    m_pDropTarget = new CComObject<CViewDropTarget>;
    if ( m_pDropTarget!= NULL )
    {
        hr = m_pDropTarget->Init(( CDropTargetClient *) this, m_pFolder, m_hWnd );
        if ( FAILED( hr ) )
        {
            delete m_pDropTarget;
            m_pDropTarget = NULL;
        }
        else
        {
            hr = m_pDropTarget->QueryInterface( IID_IDropTarget, (void **) & pDrop );
            Assert( SUCCEEDED( hr ));

            // addref our pointer because the objects are created with a zero ref count
            m_pDropTarget->AddRef();
        }
    }

    // This is allowed to fail in Non-NT5 cases.
    CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDragSourceHelper, (void**)&m_pDragImages);

    // if the above fails, then we are probably low on memory, allow it to succeed
    // it just means the view will not have a droptarget ....
    if ( pDrop != NULL )
    {
        IUnknown *pUnk = NULL;
        hr = this->_InternalQueryInterface( IID_IUnknown, (void **) &pUnk );
        Assert( SUCCEEDED( hr ));

        CoLockObjectExternal( pUnk, TRUE, FALSE );
        pUnk->Release();

        RegisterDragDrop( m_hWndListView, pDrop );
        pDrop->Release();
    }

    lpParams->hwndView = m_hWnd;

    FocusOnSomething();

    hr = CoCreateInstance( CLSID_ShellTaskScheduler,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IShellTaskScheduler,
                           (void **) & m_pScheduler );

    if ( SUCCEEDED( hr ))
    {
#ifdef  DEBUG
        // provide a small timeout to ensure the thread dies and gets restarted often...
        m_pScheduler->Status( ITSSFLAG_KILL_ON_DESTROY, 1000 );
#else
        m_pScheduler->Status( ITSSFLAG_KILL_ON_DESTROY, 5*60*1000 );
#endif
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::HandleRename(LPCITEMIDLIST pidlNew)
{
    return SelectItem( pidlNew, SVSI_EDIT );
}


////////////////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::CheckViewOptions()
{
    DWORD dwMask = (LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE | LVS_EX_UNDERLINECOLD |
                    LVS_EX_UNDERLINEHOT);
                                                                        
    DWORD dwSelection = 0;
    SHELLSTATE ss;

    ZeroMemory( &ss, sizeof( ss ));

    SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW | SSF_SHOWALLOBJECTS | SSF_SHOWCOMPCOLOR, FALSE );
    if ( !ss.fDoubleClickInWebView )
    {
        dwSelection = LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE;
    }

    m_fShowCompColor = !(!(ss.fShowCompColor));
    
    DWORD cb;
    DWORD dwUnderline = ICON_IE;
    DWORD dwExStyle;
    
    //
    // Read the icon underline settings.
    //
    cb = SIZEOF(dwUnderline);
    SHRegGetUSValueA("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                     "IconUnderline",
                     NULL,
                     &dwUnderline,
                     &cb,
                     FALSE,
                     &dwUnderline,
                     cb);

    //
    // If it says to use the IE link settings, read them in.
    //
    if (dwUnderline == ICON_IE)
    {
        dwUnderline = ICON_YES;

        TCHAR szUnderline[8];
        cb = SIZEOF(szUnderline);
        SHRegGetUSValueA("Software\\Microsoft\\Internet Explorer\\Main",
                        "Anchor Underline",
                        NULL,
                        szUnderline,
                        &cb,
                        FALSE,
                        szUnderline,
                        cb);

        //
        // Convert the string to an ICON_ value.
        //
        if (!lstrcmpiA(szUnderline, "hover"))
            dwUnderline = ICON_HOVER;
        else if (!lstrcmpiA(szUnderline, "no"))
            dwUnderline = ICON_NO;
        else
            dwUnderline = ICON_YES;
    }

    //
    // Convert the ICON_ value into an LVS_EX value.
    //
    switch (dwUnderline)
    {
    case ICON_NO:
        dwExStyle = 0;
        break;

    case ICON_HOVER:
        dwExStyle = LVS_EX_UNDERLINEHOT;
        break;

    case ICON_YES:
        dwExStyle = LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD;
        break;
    }

    // WPARAM is the mask, LPARAM is the new values
    SendMessageA( m_hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM) dwMask, 
                 (LPARAM) (dwSelection | dwExStyle ));

    m_fShowAllObjects = ss.fShowAllObjects;

    // by default we are "connected"
    // for Beta2 we are offline...
    m_fOffline = TRUE;

    DWORD dwState, dwSize = sizeof( dwState );

    // check the global online/offline mode...
    if (InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
    {
        if (dwState & INTERNET_STATE_DISCONNECTED_BY_USER)
            m_fOffline = TRUE;
    }

    // we still think we are connected, then check to see if we really have a connection
    if ( !m_fOffline )
    {
        DWORD dwFlags;
        DWORD dwMask = INTERNET_CONNECTION_MODEM | INTERNET_CONNECTION_LAN | INTERNET_CONNECTION_PROXY;
        dwFlags = dwMask;
        if ( InternetGetConnectedState( &dwFlags, 0 ) && ( dwFlags & dwMask ))
        {
            // we are connected
        }
        else
        {
            // no connection even though we are no in offline mode...
            m_fOffline = TRUE;
        }
    }
}

// *** IShellChangeNotify methods ***
////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::OnChange(LONG lEvent,
                                      LPCITEMIDLIST pidl1,
                                      LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST pidlLast1 = (LPITEMIDLIST) FindLastPidl( pidl1 );

    // lParam contains the notification code ...
    switch ( lEvent )
    {
        case SHCNE_CREATE:
        case SHCNE_MKDIR:
            {
                int iItem = FindInView( m_hWndListView, m_pFolder, pidlLast1 );
                if ( iItem == -1 )
                {
                    LPITEMIDLIST pidlNew = NULL;
                    SimpleIDLISTToRealIDLIST( m_pFolder, pidlLast1, &pidlNew );
                    if (pidlNew)
                    {
                        iItem = AddItem( pidlNew );
                        if (iItem < 0)
                        {
                            SHFree(pidlNew);
                        }
                    }
                }
            }
            UpdateStatusBar( NULL, NULL );
            break;

        case SHCNE_DELETE:
        case SHCNE_RMDIR:
            {
                UINT iItem;
                RemoveObject( (LPITEMIDLIST) pidlLast1, &iItem );
            }
            UpdateStatusBar( NULL, NULL );
            break;

        case SHCNE_UPDATEIMAGE:
            Refresh( );
            break;

        case SHCNE_NETSHARE:
        case SHCNE_NETUNSHARE:
        case SHCNE_ATTRIBUTES:
        case SHCNE_UPDATEITEM:
            {
                // find the listview item based on the pidl.
                int iItem = FindInView( m_hWndListView, m_pFolder, pidlLast1 );
                if ( iItem != -1 )
                {
                    // now get the item info from the listview for the image index.
                    LV_ITEMW lvItem;
                    lvItem.mask = LVIF_IMAGE | LVIF_NORECOMPUTE | LVIF_PARAM;
                    lvItem.iItem = iItem;
                    lvItem.iSubItem = 0;
                    if ( ListView_GetItemWrapW( m_hWndListView, &lvItem ) )
                    {
                        // if the item's icon hasn't been shown yet, then there
                        // is no need to update the image in the icon cache.
                        if ( lvItem.iImage != I_IMAGECALLBACK )
                        {
                            // find out if the image is a NOUSAGE one
                            IMAGECACHEINFO rgInfo;
                            rgInfo.dwMask = ICIFLAG_NOUSAGE;

                            // ignore zero usage items
                            HRESULT hr = m_pImageCache->GetImageInfo( lvItem.iImage, &rgInfo );
                            if (( SUCCEEDED( hr ) && !(rgInfo.dwMask & ICIFLAG_NOUSAGE ))
                                || FAILED( hr ))
                            {
                                // remove it from the thumbnail cache, forcing a re-fetch.
                                Assert( m_pImageCache );
                                m_pImageCache->DeleteImage( lvItem.iImage );
                            }
                        }

                        // reset the item info, so that you will get called
                        // back to retrieve the new image when it becomes visible.
                        lvItem.mask = LVIF_IMAGE;
                        lvItem.iImage = I_IMAGECALLBACK;
                        
                        // check the items state 
                        LPITEMIDLIST pidl = (LPITEMIDLIST) lvItem.lParam;
                        LPITEMIDLIST pidlNew = NULL;
                        SimpleIDLISTToRealIDLIST( m_pFolder, pidlLast1, &pidlNew );
                        if ( pidlNew != NULL )
                        {
                            lvItem.lParam = (LPARAM) pidlNew;
                            lvItem.mask |= LVIF_PARAM;
                        
                	        ULONG ulAttrs = SFGAO_GHOSTED;
	                        HRESULT hr = m_pFolder->GetAttributesOf( 1, (LPCITEMIDLIST *)&pidlNew, &ulAttrs );
    	                    if ( SUCCEEDED( hr ))
        	                {
            	                lvItem.stateMask = LVIS_CUT;
                	            lvItem.state = ((ulAttrs & SFGAO_GHOSTED) ? LVIS_CUT : 0);
                    	        lvItem.mask |= LVIF_STATE;
                        	}
						}
						
                        ListView_SetItemWrapW(m_hWndListView, &lvItem );

                        if ( pidlNew )
                        {
                            SHFree((LPITEMIDLIST) pidl );
                        }
                    }
                }
            }
            break;

        case SHCNE_RENAMEITEM:
        case SHCNE_RENAMEFOLDER:
            {
                // need to detect if it has been renamed into this folder, or out of.....
                LPITEMIDLIST pidlLast = (LPITEMIDLIST) FindLastPidl( pidl2 );
                USHORT cbSize = pidlLast->mkid.cb;
                BOOL fStartedHere = FALSE;
                BOOL fEndedHere = FALSE;
                UINT iItem;

                // remove the last item for now...
                pidlLast->mkid.cb = 0;

                IShellFolder *pDesktop;
                HRESULT hr = SHGetDesktopFolder( &pDesktop );
                if ( SUCCEEDED( hr ))
                {
                    // see if it landed here...
                    hr = pDesktop->CompareIDs( 0, m_pidl, pidl2 );
                    pidlLast->mkid.cb = cbSize;
                    if ( hr == 0 )
                    {
                        fEndedHere = TRUE;
                    }

                    // see if it started in this folder...
                    cbSize = pidlLast1->mkid.cb;
                    pidlLast1->mkid.cb = 0;
                    hr = pDesktop->CompareIDs( 0, m_pidl, pidl1 );
                    pidlLast1->mkid.cb = cbSize;
                    if ( hr == 0 )
                    {
                        fStartedHere = TRUE;
                    }

                    if ( fEndedHere )
                    {
                        hr = E_FAIL;
                        LPITEMIDLIST pidlNew = NULL;
                        SimpleIDLISTToRealIDLIST( m_pFolder, pidlLast, &pidlNew );
                        if ( pidlNew )
                        {
                            if ( fStartedHere )
                            {
                                hr = UpdateObject( pidlLast1, pidlNew, &iItem );
                            }
                            else
                            {
                                if (FindInView( m_hWndListView, m_pFolder, pidlNew ) == -1 )
                                {
                                    if (AddItem( pidlNew ) != -1)
                                    {
                                        hr = S_OK;
                                    }
                                }
                            }

                            if ( hr != S_OK )
                            {
                                SHFree( pidlNew );
                            }
                        }
                    }
                    else if ( fStartedHere )
                    {
                        RemoveObject( pidlLast1, &iItem );
                    }
                }
            }
            UpdateStatusBar( NULL, NULL );
            break;

        default:
            {
                LPRUNNABLETASK pTask;
                HRESULT hr = CUpdateDirTask_Create( this, &pTask );
                if ( SUCCEEDED( hr ))
                {
                    Assert( m_pScheduler );
                    m_pScheduler->RemoveTasks( TOID_UpdateDirHandler, ITSAT_DEFAULT_LPARAM, FALSE );

                    // add with a low prority, but higher than HTML extraction...
                    hr = m_pScheduler->AddTask( pTask, TOID_UpdateDirHandler, ITSAT_DEFAULT_LPARAM, 0x1000000 );
                    if ( hr != S_OK )
                    {
                        // run on the current thread ... REAL SLOW.....
                        pTask->Run();
                    }
                    pTask->Release();
                }
                            
                UpdateStatusBar( NULL, NULL );
            }
            break;
    }

    return NOERROR;
}

STDMETHODIMP CThumbnailView::SelectAndPositionItem(LPCITEMIDLIST pidlItem,
                                                   UINT uFlags, POINT* ppt)
{
    // See if we should first deselect everything else
    if (!pidlItem)
    {
        if (uFlags != SVSI_DESELECTOTHERS)
        {
            // I only know how to deselect everything
            return(E_INVALIDARG);
        }

        ListView_SetItemState(this->m_hWndListView, -1, 0, LVIS_SELECTED);
        return(S_OK);
    }

    if (uFlags & SVSI_TRANSLATEPT)
    {
        //The caller is asking us to take this point and convert it from screen Coords
        // to the Client of the Listview.
        ScreenToClient(m_hWndListView, ppt);

        POINT ptOrigin;
    
        if (ListView_GetOrigin(m_hWndListView, &ptOrigin))
        {
            ppt->x += ptOrigin.x;
            ppt->y += ptOrigin.y;
        }
    }

    int i = FindItem(pidlItem);
    if (i != -1)
    {
        // set the position first so that the ensure visible scrolls to
        // the new position
        if (ppt)
        {
            ListView_SetItemPosition32(this->m_hWndListView, i, ppt->x, ppt->y);
        }

        // The SVSI_EDIT flag also contains SVSI_SELECT and as such
        // a simple & wont work!
        if ((uFlags & SVSI_EDIT) == SVSI_EDIT)
        {
            // Grab focus if the listview (or any of it's children) don't already have focus
            HWND hwndFocus = GetFocus();
            if (!hwndFocus ||
                !(hwndFocus == this->m_hWndListView || IsChild(this->m_hWndListView, hwndFocus)))
            {
                SetFocus(this->m_hWndListView);
            }
            ListView_EditLabel(this->m_hWndListView, i);
        }
        else
        {
            UINT stateMask = LVIS_SELECTED;
            UINT state = (uFlags & SVSI_SELECT) ? LVIS_SELECTED : 0;
            if (uFlags & SVSI_FOCUSED)
            {
                state |= LVIS_FOCUSED;
                stateMask |= LVIS_FOCUSED;
            }

            // See if we should first deselect everything else
            if (uFlags & SVSI_DESELECTOTHERS)
               ListView_SetItemState(this->m_hWndListView, -1, 0, LVIS_SELECTED);

            ListView_SetItemState(this->m_hWndListView, i, state, stateMask);

            if (uFlags & SVSI_ENSUREVISIBLE)
                ListView_EnsureVisible(this->m_hWndListView, i, FALSE);

            SetFocus(this->m_hWndListView);
        }

    }
    else if ((uFlags & SVSI_EDIT) == SVSI_EDIT && m_fUpdateDir )
    {
        // cache away the pidl until the updateDir is done
        EnterCriticalSection( &m_csAddLock );
        m_pidlRename = ILClone( pidlItem );
        LeaveCriticalSection( &m_csAddLock );
    }
    return NOERROR;
}

