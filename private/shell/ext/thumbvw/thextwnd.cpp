/* sample source code for IE4 view extension
 *
 * Copyright Microsoft 1996
 *
 * This file implements the Window routines
 */

#include "precomp.h"
#include "shellp.h"
#include "regstr.h"
#ifdef DEBUG
#define SMALLCACHE 1
#endif

WCHAR const c_szDefault[] = L"Default";

// the view wind proc ....
LRESULT CALLBACK CThumbnailView_WndProc( HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam );
DWORD GetAltColor();

#define SUBIDSTART  0x0000
#define SUBIDEND    0x7fff

////////////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::RegisterWindowClass( )
{
    // register the window class that will sit as a wrapper for the ListView.
    WNDCLASSW wc;

    if (!GetClassInfoWrapW(g_hinstDll, VIEWCLASSNAME, &wc))
    {
        // don't want vredraw and hredraw because that causes horrible
        // flicker expecially with full drag
        wc.style         = CS_PARENTDC;
        wc.lpfnWndProc   = (WNDPROC) CThumbnailView_WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(CThumbnailView *);
        wc.hInstance     = g_hinstDll;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursorA(NULL, (LPCSTR) IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = VIEWCLASSNAME;

        RegisterClassWrapW(&wc);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK CThumbnailView_WndProc( HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam )
{
    CThumbnailView * pThis = (CThumbnailView *) GetWindowLongPtrA( hWnd, 0 );
    LRESULT lRes = TRUE;

    switch( iMessage )
    {
        case WM_NOTIFYFORMAT:
            // we are now a unicode window....
            return NFR_UNICODE;
            
        case WM_MENUCHAR:
            if ( pThis->m_pCMCache )
            {
                lRes = 0;
                IContextMenu3* pcm3;
                HRESULT hr = pThis->m_pCMCache->QueryInterface(IID_IContextMenu3, (void **) &pcm3);
                if (SUCCEEDED( hr))
                {
                    hr = pcm3->HandleMenuMsg2( iMessage, wParam, lParam, &lRes);
                    pcm3->Release();

                    if (SUCCEEDED(hr))
                    {
                        return lRes;
                    }
                }
                else
                {
                    goto HandleDefWnd;
                }
            }
            break;

        case WM_INITMENUPOPUP:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
            if ( pThis->m_pCMCache )
            {
                // if we have a cached context menu, then we have it displayed,
                // so we need to forward these three message to the Menu handler instead
                // of doing the default action....
                LPCONTEXTMENU2 pcm2;
                HRESULT hr = pThis->m_pCMCache->QueryInterface(IID_IContextMenu2, (void **) &pcm2);
                if (SUCCEEDED( hr))
                {
                    pcm2->HandleMenuMsg( iMessage, wParam, lParam );
                    pcm2->Release();
                    return 0;
                }
            }
            break;

        case WM_ERASEBKGND:
            if (ListView_GetBkColor(pThis->m_hWndListView ) == CLR_NONE)
            {
                return SendMessageA(pThis->m_hWndParent, iMessage, wParam, lParam);
            }
            // We want to reduce flash
            lRes = TRUE;
            break;

        case WM_CREATE:
        {
            // cast the create struct pointer to the object (as that is what we passed )
            LPCREATESTRUCT pCS = (LPCREATESTRUCT) lParam;
            pThis = (CThumbnailView * ) pCS->lpCreateParams;

            lRes = pThis->OnWmCreate( hWnd, pCS );
           break;
        }

        case WM_DESTROY:
            lRes = pThis->OnWmDestroy( hWnd );
            break;

        // stupid . . . handle case where windows only sends WM_WINDOWPOSCHANGING and not
        // WM_SIZE to handle sizing of tv. . .
        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS *pWPos = (WINDOWPOS *) lParam;
            if ( pWPos->cx && pWPos->cy )
                lRes = pThis->OnWmSize(hWnd, 0, pWPos->cx, pWPos->cy);
            break;
        }
        case WM_SIZE:
            // handle it if we need to ...
            lRes = HANDLE_WM_SIZE( hWnd, wParam, lParam, pThis->OnWmSize );
            break;

        case WM_MENUSELECT:
            lRes = pThis->OnWmMenuSelect( hWnd, wParam, lParam );
            break;

        case WM_ACTIVATE:
            // force update on inactive to not ruin save bits
            lRes = HANDLE_WM_ACTIVATE( hWnd, wParam, lParam, pThis->OnWmActivate);
            break;

        case WM_CLIPBOARD_CUTCOPY:
            lRes = pThis->OnWmClipboardUpdate( );
            break;

        case WM_TIMER:
        {
            if ( GetTickCount() - pThis->m_dwCacheTickCount > 2000 )
            {
                DWORD dwMode;
                if ( pThis->m_pDiskCache->GetMode( &dwMode ) == S_OK && pThis->m_pDiskCache->IsLocked() == S_FALSE )
                {
                    // two seconds since last access, close the cache.
                    pThis->m_pDiskCache->Close( NULL );
                }

                if ( pThis->m_pScheduler->CountTasks( TOID_NULL ) == 0 )
                {
                    // there is nothing in the queue pending, so quit listening...
                    KillTimer( hWnd, TIMER_DISKCACHE );
                }
            }
        }
        break;
        
        case WM_QUERYNEWPALETTE:
        {
            HDC hdc = GetDC( pThis->m_hWndListView );
            HPALETTE hpalOld = SelectPalette( hdc, pThis->m_hpal, FALSE );
            RealizePalette( hdc );
            SelectPalette( hdc, hpalOld, TRUE );
            ReleaseDC( pThis->m_hWndListView, hdc );
            return TRUE;
        }

        case WM_PALETTECHANGED:
        {
            if ( (HWND) wParam != pThis->m_hWndListView )
            {
                HDC hdc = GetDC( pThis->m_hWndListView );
                HPALETTE hpalOld = SelectPalette( hdc, pThis->m_hpal, TRUE );
                RealizePalette( hdc );
                SelectPalette( hdc, hpalOld, TRUE );
                ReleaseDC( pThis->m_hWndListView, hdc );
            }
            // we don't bother forwarding because the list view does nothing unless it
            // has a background image, so just invalidate instead..
            InvalidateRect(pThis->m_hWndListView, NULL, FALSE);
        }
        return TRUE;

        case WM_SETFOCUS:
        {
            // NOTE: we use the defView as the IShellView to set as active ..
            // NOTE: this allows the defview to handle issues such as menus
            // NOTE: and to delegate them to view extensions where necessary
            if ( pThis->m_pBrowser != NULL )
            {
                pThis->m_pBrowser->OnViewWindowActive( pThis->m_pDefView );

                SetFocus( pThis->m_hWndListView );
            }

            lRes = FALSE;
            break;
        }

        case WM_NOTIFY:
            lRes = HANDLE_WM_NOTIFY( hWnd, wParam, lParam, pThis->OnWmNotify );
            break;

        case WM_CONTEXTMENU:
            lRes = pThis->OnWmContextMenu( hWnd, GET_X_LPARAM( lParam ), GET_Y_LPARAM(lParam ));
            break;

        case GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC):
            SetBkColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, iMessage),
            GetSysColor(COLOR_WINDOW));
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);


        case WM_WININICHANGE:
        case WM_SYSCOLORCHANGE:
            if ( pThis->m_hWndListView!= NULL )
            {
                SendMessageA(pThis->m_hWndListView, iMessage, wParam, lParam);
            }
            lRes = FALSE;
            break;

        case CWM_GETISHELLBROWSER:
            // NOTE: the shell does not Addref this, so neither will we
            lRes = (LRESULT) pThis->m_pBrowser;
            break;

        case WM_UPDATEITEMIMAGE:
            {
                LV_ITEMW rgItem;
                rgItem.mask = LVIF_IMAGE;
                rgItem.iItem = (int)wParam;
                rgItem.iSubItem = 0;

                if ( pThis->m_hWndListView && ListView_GetItemWrapW( pThis->m_hWndListView, &rgItem ))
                {
                    if ( rgItem.iImage != I_IMAGECALLBACK )
                    {
                        Assert( pThis->m_pImageCache );

                        // free the previous image in the cache ...
                        pThis->m_pImageCache->FreeImage((UINT) rgItem.iImage );
                    }

                    if ( lParam == rgItem.iImage )
                    {
                        ListView_RedrawItems( pThis->m_hWndListView, wParam, wParam );
                    }
                    else
                    {
                        rgItem.iImage = (int)lParam;
                        ListView_SetItemWrapW( pThis->m_hWndListView, &rgItem );
                    }
                }
            }
            break;

        case WM_STATUSBARUPDATE:
        {
            if ( lParam == 0xffffffff )
            {
                pThis->UpdateStatusBar( NULL, NULL );
            }
            else if ( pThis->m_hWndListView )
            {
                WCHAR szText[MAX_PATH];
                LV_ITEMW rgItem;
                rgItem.mask = LVIF_TEXT;
                rgItem.pszText = szText;
                rgItem.cchTextMax = ARRAYSIZE(szText);
                rgItem.iItem = (int)lParam;
                rgItem.iSubItem = 0;

                if ( ListView_GetItemWrapW( pThis->m_hWndListView, &rgItem ))
                {
                    WCHAR szMessage[MAX_PATH];
                    WCHAR szMessageFinal[MAX_PATH * 2];
                    LoadStringWrapW( g_hinstDll, (UINT)wParam, szMessage, ARRAYSIZE(szMessage) );
                    wnsprintfW( szMessageFinal, ARRAYSIZE( szMessageFinal), szMessage, szText );
                    pThis->UpdateStatusBar( szMessageFinal, NULL );
                }
            }
        }
        break;

        case WM_VIEWREFRESH:
            pThis->Refresh();
            lRes = FALSE;
            break;


        case WM_PROCESSITEMS:
            pThis->OnWmProcessItems();
            lRes = FALSE;
            break;

        case WM_HANDLESELCHANGE:
        {
            BOOL fSingular = TRUE;
            BOOL fSelChange = (BOOL) wParam;

            MSG msgTmp;
            
            while (PeekMessage( &msgTmp, hWnd, WM_HANDLESELCHANGE, WM_HANDLESELCHANGE, PM_REMOVE))
            {
                if (fSingular)
                {
                    fSingular = FALSE;
                }
            }

            // if there are multiple queued, then...
            if ( pThis->m_fSelChanges > 1 )
            {
                fSingular = FALSE;
            }
            
            if (( pThis->m_pCommDlg != NULL ) && (fSelChange || !fSingular))
            {
                pThis->m_pCommDlg->OnStateChange( pThis->m_pDefView, CDBOSC_SELCHANGE );
            }

            pThis->UpdateStatusBar( );

            if ( fSingular && !fSelChange )
            {
                // figure out if the item is one of the selection,
                // if so, then send the notification, otherwise don't
                LV_ITEMW rgItem;
                rgItem.iItem = (int) lParam;
                rgItem.mask = LVIF_NORECOMPUTE | LVIF_STATE;
                rgItem.iSubItem = 0;
                rgItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;

                int iItem = ListView_GetItemWrapW( pThis->m_hWndListView, &rgItem );

                if ( rgItem.state & (LVIS_FOCUSED | LVIS_SELECTED))
                {
                    fSelChange = TRUE;
                }
            }

            if (pThis->m_pAuto && ( !fSingular || fSelChange ))
            {
                // only automate if you have the automaton
                lRes = Invoke_OnConnectionPointerContainer((IUnknown *)pThis->m_pAuto
                        , DIID_DShellFolderViewEvents, DISPID_SELECTIONCHANGED, IID_NULL
                        , 0, DISPATCH_METHOD, NULL, NULL, NULL, NULL);

            }
            
            pThis->m_fSelChanges = 0;
            break;
        }
        
        case WM_MOUSEWHEEL:
        default:
            // Handle the magellan mousewheel message.
            if (iMessage == g_msgMSWheel && pThis)
            {
                lRes = SendMessageA(pThis->m_hWndListView, iMessage, wParam, lParam);
            }
            else
            {
HandleDefWnd:
                lRes = DefWindowProcWrapW( hWnd, iMessage, wParam, lParam );
            }
            break;
    }

    return lRes;
}

///////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmSize( HWND hwnd, UINT fFlags, int iWidth, int iHeight )
{
    // resize the enhanced list view.
    ::MoveWindow( m_hWndListView, 0, 0, iWidth, iHeight, TRUE );

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmClipboardUpdate( void )
{
    int     iIndex = -1;

    // go through everything in the list view.
    do
    {
        iIndex = ListView_GetNextItem( m_hWndListView, iIndex, LVNI_ALL );
        if ( iIndex == -1 )
        {
            break;
        }

        LV_ITEMW rgItem;
        rgItem.iItem = iIndex;
        rgItem.mask = LVIF_PARAM;
        rgItem.iSubItem = 0;

        int iItem = ListView_GetItemWrapW( m_hWndListView, &rgItem );
        // just incase. ...
        if ( iItem != -1 )
        {
            LPCITEMIDLIST pidl = ( LPCITEMIDLIST )rgItem.lParam;

            // check if the item should be ghosted or not.
            DWORD   dwGhosted = SFGAO_GHOSTED;
            WORD    wCut = 0x0000;
            WORD    wStateOld = ListView_GetItemState( m_hWndListView,
                rgItem.iItem, LVIS_CUT );

            m_pFolder->GetAttributesOf( 1, &pidl, &dwGhosted );
            if ( dwGhosted & SFGAO_GHOSTED )
                wCut = LVIS_CUT;

            if ( wStateOld != wCut )
            {
                // update and redraw the items that have changed.
                ListView_SetItemState( m_hWndListView, rgItem.iItem, wCut, LVIS_CUT );
                ListView_RedrawItems( m_hWndListView, rgItem.iItem, rgItem.iItem );
            }
        }
    } while ( TRUE );

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmNotify( HWND hWnd, int iID, NMHDR * pHdr )
{
    LRESULT lRes = FALSE;
    HRESULT hr = NOERROR;

    switch( pHdr->code )
    {
        case NM_CUSTOMDRAW:
            return OnCustomDraw(( NMCUSTOMDRAW *) pHdr );

        case LVN_GETDISPINFOA:
        {
            LV_DISPINFOA * pInfo = (LV_DISPINFOA * ) pHdr ;

            LPCITEMIDLIST pidl = NULL;
            // if the enhanced view is looking for its thumbnnails.
            if ( pInfo->item.mask & LVIF_IMAGE )
            {
                lRes = OnViewGetThumbnail( pHdr );

                pInfo->item.mask |= LVIF_DI_SETITEM;
            }
            break;
        }
        
        case LVN_GETDISPINFOW:
        {
            LV_DISPINFOW * pInfo = (LV_DISPINFOW * ) pHdr ;

            LPCITEMIDLIST pidl = NULL;
            // if the enhanced view is looking for its thumbnnails.
            if ( pInfo->item.mask & LVIF_IMAGE )
            {
                lRes = OnViewGetThumbnail( pHdr );

                pInfo->item.mask |= LVIF_DI_SETITEM;
            }
            break;
        }

        case LVN_GETINFOTIPW:
            lRes = OnInfoTipText((NMLVGETINFOTIPW *)pHdr);
            break;


        case LVN_COLUMNCLICK:
        {
            NM_LISTVIEW *   pnmv = ( NM_LISTVIEW * )pHdr;
            this->SortBy( pnmv->iSubItem, TRUE );
            lRes = FALSE;
            break;
        }

        case LVN_ITEMCHANGED:
        {
            NM_LISTVIEW *   pnmv = ( NM_LISTVIEW * )pHdr;
            BOOL fSelChange = (pnmv->uChanged & LVIF_STATE) != 0;

            if ( !m_fSelChanges )
            {
                PostMessage( m_hWnd, WM_HANDLESELCHANGE, (WPARAM) fSelChange, (LPARAM) pnmv->iItem );
            }

            m_fSelChanges ++;
            break;
        }

        case LVN_BEGINLABELEDITW:
        {
            // failure case stop editing ...
            lRes = TRUE;
            LV_DISPINFOW * pInfo = (LV_DISPINFOW * ) pHdr ;

            LPCITEMIDLIST pidl = (LPCITEMIDLIST)pInfo->item.lParam;

            ULONG rgFlags = SFGAO_CANRENAME;
            if (pidl == NULL ||
                FAILED(m_pFolder->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &rgFlags)) || 
                !(rgFlags & SFGAO_CANRENAME))
                break;

            lRes = FALSE;
            HWND hwndEdit = ListView_GetEditControl(m_hWndListView);
            if (hwndEdit)
            {
                int cchMax = 0;
                m_pFolderCB->MessageSFVCB(SFVM_GETCCHMAX, (WPARAM)pidl, (LPARAM)&cchMax);
                if (cchMax)
                {
                    Assert(cchMax < 1024);
                    SendMessageA(hwndEdit, EM_LIMITTEXT, cchMax, 0);
                }

                WCHAR szName[MAX_PATH];
                STRRET str;
                if (SUCCEEDED(m_pFolder->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FOREDITING, &str)) &&
                    SUCCEEDED(StrRetToBufW(&str, pidl, szName, ARRAYSIZE(szName))))
                {
                    SetWindowTextWrapW(hwndEdit, szName);
                }
            }
 
            m_fTranslateAccel = TRUE;
            break;
        }

        case LVN_ENDLABELEDITW:
        {
            m_fTranslateAccel = FALSE;

            // check the HWND to see if we have been destroyed.... (this happens if someone
            // navigates while the edit box is up)
            if ( m_hWnd )
            {
                // AddRef, rename, then release, so that the
                // view object stays around until the rename function returns.
                // this in case the scope changes in the middle of the rename, the
                // view would be released and deleted before the rename returned.
                this->InternalAddRef( );
                this->OnLVNEndLabelEdit( pHdr );
                this->InternalRelease( );
            }
            lRes = FALSE;
            break;
        }

        case LVN_BEGINDRAG:
        case LVN_BEGINRDRAG:
        {
            HRESULT hr = OleInitialize(NULL);
            if ( SUCCEEDED( hr ))
            {
                StartDragDrop( (NM_LISTVIEW * ) pHdr );
                OleUninitialize();
            }

            SHChangeNotifyHandleEvents();
            break;
        }

        case NM_SETFOCUS:
            // NOTE: notify the browser we just got focus, we use the defview
            // NOTE: for this so that it can handle the menu merging.
            Assert( m_pDefView != NULL );

            m_pBrowser->OnViewWindowActive( m_pDefView );
            if ( m_pCommDlg != NULL )
            {
                m_pCommDlg->OnStateChange( m_pDefView, CDBOSC_SETFOCUS );
            }

            m_pDefView->UIActivate( SVUIA_ACTIVATE_FOCUS );
            break;

        case NM_KILLFOCUS:
            if ( m_pCommDlg != NULL )
            {
                m_pCommDlg->OnStateChange( m_pDefView, CDBOSC_KILLFOCUS );
            }
            break;

        case LVN_ITEMACTIVATE:
        {
            // make sure there is no context menu up...
            if (m_hWndListView)
                SendMessageA(m_hWndListView, WM_CANCELMODE, 0, 0);
            HRESULT hRes = S_FALSE;

            if ( m_pCommDlg != NULL )
            {
                // addref the def-view incase it decides to unwind us in the meantime.
                IShellView *pView = m_pDefView;
                pView->AddRef();
                hRes = m_pCommDlg->OnDefaultCommand( pView );
                pView->Release();
            }

            if ( hRes == S_FALSE )
            {
                lRes = this->OnDefaultAction( FALSE );
            }
            lRes = FALSE;
            break;
        }
    }
    return lRes;
}

//////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmActivate( HWND hWnd,
                                      int fActive,
                                      HWND hWndPrev,
                                      BOOL fMinimized )
{
    if (fActive == WA_INACTIVE)
    {
        UpdateWindow( m_hWndListView );
    }

    return (0);
}

//////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmContextMenu( HWND hWnd, int iX, int iY )
{
    int iItem = -1;
    HMENU hCMenu = NULL;
    LPCONTEXTMENU pICMenu = NULL;
    LPCITEMIDLIST * apidl = NULL;
    HRESULT hRes;
    LRESULT lRes = FALSE;
    int iSelectedCount = 0;
    UINT id;
    UINT grfFlags;

    POINT pt = {iX, iY};
    RECT rc;
    LV_HITTESTINFO rgInfo;
    rgInfo.flags = 0;
    rgInfo.iItem = -1;
    BOOL fItemsFromSelection = TRUE;

    // check the loword, as it was a lparam that was broken apart.
    if (( LOWORD( iX ) == 0xffff ) && ( LOWORD( iY )== 0xffff ))
    {
        // it must have come from the keyboard contextmenu button
        // we need to find an appropriate position....

        // find the focussed item
        int iItem = ListView_GetNextItem( m_hWndListView, -1, LVNI_FOCUSED | LVNI_SELECTED );
        if ( iItem != -1 )
        {
            RECT rcItem;
            ListView_GetItemRect(m_hWndListView, iItem, &rcItem, LVIR_ICON);
            pt.x = (rcItem.left + rcItem.right) / 2;
            pt.y = (rcItem.top + rcItem.bottom) / 2;

            rgInfo.flags = LVHT_ONITEM;
            rgInfo.iItem = iItem;
        }
        else
        {
            pt.x = 0;
            pt.y = 0;
        }

        MapWindowPoints(m_hWndListView, HWND_DESKTOP, (LPPOINT)&pt, 1);

        iX = pt.x;
        iY = pt.y;
    }
    else
    {
        GetClientRect( m_hWndListView, &rc );
        ScreenToClient( m_hWndListView, &pt );

        rgInfo.pt = pt;
        rgInfo.flags = 0;

        ListView_HitTest( m_hWndListView, & rgInfo );
    }

    hCMenu = CreatePopupMenu();
    if ( hCMenu == NULL )
    {
        return FALSE;
    }

    BOOL fItem = FALSE;

    // did we hit anything ?
    if ( !(rgInfo.flags & LVHT_ONITEM ) || (rgInfo.iItem == -1 ))
    {
        // background context menu .....
        hRes = this->CreateBackgroundMenu( &pICMenu );
        if ( FAILED( hRes ))
        {
            goto ContextCleanup;
        }
    }
    else
    {
        iSelectedCount = ListView_GetSelectedCount( m_hWndListView );
        if ( iSelectedCount == 0 )
        {
            // we must catch this because sometimes the listview doesn't
            // always have an item selected when you context menu and item
            fItemsFromSelection = FALSE;
            iSelectedCount = 1;
        }

        // fetch the pidl from the view ...
        Assert( iSelectedCount > 0 );

        apidl = new LPCITEMIDLIST[iSelectedCount];
        if ( apidl == NULL )
        {
            // do something about low memory ...
            MessageBoxWrapW( m_hWnd,
                        (LPWSTR)MAKEINTRESOURCE( IDS_ERR_OUTOFMEM ),
                        (LPWSTR)MAKEINTRESOURCE( IDS_ERR_SHELLTITLE ),
                        MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );

            goto ContextCleanup;
        }

        if (fItemsFromSelection)
        {
            // there is a selection
            GetSelectionPidlList(m_hWndListView, iSelectedCount, apidl, rgInfo.iItem);
        }
        else
        {
            // we failed to get a selection, but we know what item we are
            // over ...
            LV_ITEMW rgItem;
            rgItem.mask = LVIF_PARAM;
            rgItem.iItem = rgInfo.iItem;
            rgItem.iSubItem = 0;
            int iItem = ListView_GetItemWrapW( m_hWndListView, &rgItem );
            if ( iItem == -1 )
            {
                goto ContextCleanup;
            }

            apidl[0] = (LPCITEMIDLIST) rgItem.lParam;
        }


        // get the context menu interface for the object ....
        UINT rgfInOut = 0;
        CComObject<CThumbnailMenu> * pMenuTmp = new CComObject<CThumbnailMenu>;
        if ( pMenuTmp == NULL )
        {
            goto ContextCleanup;
        }

        hRes = pMenuTmp->QueryInterface( IID_IContextMenu, (void ** ) & pICMenu );
        Assert( SUCCEEDED( hRes ));

        hRes = pMenuTmp->Init( this, & rgfInOut, apidl, iSelectedCount );
        if ( FAILED(hRes ))
        {
            goto ContextCleanup;
        }

        // it is an item not the background...
        fItem = TRUE;
    }

    grfFlags = CMF_NORMAL | CMF_CANRENAME;
    if ( m_fExploreMode != FALSE )
    {
        grfFlags |= CMF_EXPLORE;
    }

    Assert( hCMenu != NULL );

    m_idCMStartOffset = SUBIDSTART;
    pICMenu->QueryContextMenu( hCMenu, 0, SUBIDSTART, SUBIDEND, grfFlags );

    // set site on the context menu if it will let us...
    IUnknown_SetSite(pICMenu, (IUnknown *)(IShellView *)this);
    
    // If this is the common dialog browser, we need to make the
    // default command "Select" so that double-clicking (which is
    // open in common dialog) makes sense.
    if ( m_pCommDlg && fItem )
    {
        HMENU hmSelect = LoadPopupMenu( g_hinstDll, IDM_COMMDLG_POPUPMERGE );

        // NOTE: Since commdlg always eats the default command,
        // we don't care what id we assign hmSelect, as long as it
        // doesn't conflict with any other context menu id.
        // SUBIDSTART won't conflict with anyone.
        MergeMenus( hCMenu, hmSelect, (UINT)(SUBIDSTART - 1), 0 );

        SetMenuDefaultItem( hCMenu, 0, MF_BYPOSITION );
        DestroyMenu(hmSelect);
    }

    // cache the context menu so we can do the status bar stuff ...

    // This function needs to be reentrant because the TrackPopupMenu call below blocks in it's own message
    // pump.  If we already have an m_pCMCache that means there is another WM_CONTEXTMENU message that is still
    // being processed and is currently blocked in the TrackPopupMenu call.  We need to discard the previously
    // stored m_pCMCache and use the new one.  Note that we aren't dealing with multi-threaded reentrance, just
    // with recursion in our window proc due to the message pump in TrackPopupMenu.
    if ( m_pCMCache )
    {
        m_pCMCache->Release();
    }

    m_pCMCache = pICMenu;
    m_pCMCache->AddRef();

    id = TrackPopupMenu( hCMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                         iX, iY, 0, m_hWnd, NULL );

    // At this point we might have already released the m_pCMCache because we could have recursed into this
    // function.  If we reentered then we would have already released the interface and set m_pCMCache to
    // NULL.  Notice that we aren't dealing with multiple threads, just a recursion in our window proc due to
    // the message pump in TrackPopupMenu.  None of this code is thread safe but it doesn't need to be.
    if ( m_pCMCache )
    {
        m_pCMCache->Release();
        m_pCMCache = NULL;
    }

    // did we get a valid menu selection ?
    if ( id > 0 )
    {
        if ( id >= SUBIDSTART  && id <= SUBIDEND )
        {
            // need to see if it was the rename command....
            CHAR szBuffer[MAX_PATH];

            szBuffer[0] = TEXT('\0');
            hRes = pICMenu->GetCommandString( id - SUBIDSTART, 
                GCS_VERBA, NULL, (LPSTR)szBuffer, ARRAYSIZE(szBuffer));

            // check for the unicode version...
            if ( FAILED( hRes ))
            {
                WCHAR szCommand[60];
                szCommand[0] = L'\0';
                hRes = pICMenu->GetCommandString(id - SUBIDSTART, 
                    GCS_VERBW, NULL, (LPSTR)szCommand, ARRAYSIZE(szCommand));
                SHUnicodeToAnsi(szCommand, szBuffer, ARRAYSIZE(szBuffer));
            }

            if (SUCCEEDED( hRes ) && StrCmpIA(szBuffer, "Rename") == 0)
            {
                // force the view to rename it instead...
                int iItem = rgInfo.iItem;
                if (fItemsFromSelection && iItem == -1)
                    iItem = ListView_GetNextItem( m_hWndListView, -1, LVNI_SELECTED );

                ListView_EditLabel( m_hWndListView, iItem );
            }
            else
            {
                Assert( m_pidl );
                WCHAR szWBuffer[MAX_PATH];
                
                SHGetPathFromIDListWrapW( m_pidl, szWBuffer );
                SHUnicodeToAnsi( szWBuffer, szBuffer, ARRAYSIZE( szBuffer ));

                if ( IsOS( OS_NT ))
                {
                    // pass the command to the element ....
                    CMINVOKECOMMANDINFOEX rgCommand;
                    memset( &rgCommand, 0, sizeof( rgCommand ));
                    rgCommand.cbSize = sizeof( rgCommand );
                    rgCommand.lpVerb = (LPCSTR) MAKEINTRESOURCE( id - SUBIDSTART );
                    rgCommand.lpVerbW = (LPWSTR) rgCommand.lpVerb;
                    rgCommand.fMask = CMIC_MASK_UNICODE;
                    m_pBrowser->GetWindow( &(rgCommand.hwnd));
                    rgCommand.nShow = SW_NORMAL;
                    rgCommand.lpDirectory = szBuffer;
                    rgCommand.lpDirectoryW = szWBuffer;
                    
                    pICMenu->InvokeCommand( (CMINVOKECOMMANDINFO *)&rgCommand );
                }
                else
                {
                    // pass the command to the element ....
                    CMINVOKECOMMANDINFO rgCommand;
                    memset( &rgCommand, 0, sizeof( rgCommand ));
                    rgCommand.cbSize = sizeof( rgCommand );
                    rgCommand.lpVerb = (LPCSTR) MAKEINTRESOURCE( id - SUBIDSTART );
                    m_pBrowser->GetWindow( &(rgCommand.hwnd));
                    rgCommand.nShow = SW_NORMAL;
                    rgCommand.lpDirectory = szBuffer;
                    pICMenu->InvokeCommand( &rgCommand );
                }
            }
        }
        else if (( id == SUBIDSTART - 1 ) && ( m_pCommDlg != NULL ))
        {
            // we are in the common dialog, and we are being told to do the default ...
            // NOTE: we pass the Defview as the IshellView (it will delegate back)
            hRes = m_pCommDlg->OnDefaultCommand( m_pDefView );
        }
    }

    // we handled the command
    lRes = FALSE;

ContextCleanup:
    // clean up .....
    // release the menu first incase it needs to tidy up the menu....
    IUnknown_SetSite(pICMenu, NULL);
    if ( pICMenu != NULL )
        pICMenu->Release();

    if ( hCMenu != NULL )
        DestroyMenu( hCMenu );

    if ( apidl != 0 )
        delete [] apidl;

    return lRes;
}

////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::CreateBackgroundMenu( IContextMenu ** ppMenu )
{
    Assert( m_pDefView != NULL );

    // Ask the def-view IShellView for the context-menu
    return m_pDefView->GetItemObject( SVGIO_BACKGROUND,
                                      IID_IContextMenu,
                                      (void **) ppMenu );
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::UpdateStatusBar( LPCWSTR pszText, LPCWSTR pszText2 )
{
    int     iSelCount;
    WCHAR   szStatusText[MAX_PATH+1];
    LRESULT lRes;

    // make sure we have a pointer to the browser.
    if ( m_pBrowser == NULL )
        return;

    if ( !pszText )
    {
        // get selection count from view that is being displayed.
        iSelCount = ListView_GetSelectedCount( m_hWndListView );

        UINT idMsg = 0;
        if ( iSelCount > 0 )
        {
            idMsg = MSG_STATUS_OBJECTS_SELECTED;
        }
        else
        {
            idMsg = MSG_STATUS_OBJECTS;
            iSelCount = ListView_GetItemCount( m_hWndListView );
        }

        void *pArg = (LPVOID) iSelCount;

        FormatMessageWrapW( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       g_hinstDll,
                       idMsg,
                       0,
                       szStatusText,
                       MAX_PATH * 2,
                       (va_list *) &pArg );
        pszText = szStatusText;
    }
    else if ( IS_INTRESOURCE( pszText ))
    {
        LoadStringWrapW( g_hinstDll, PtrToUlong(pszText), szStatusText, ARRAYSIZE(szStatusText) );
        pszText = szStatusText;
    }

    // display the message.
    m_pBrowser->SendControlMsg( FCW_STATUS, SB_SETTEXTW, 0, (LPARAM)pszText, &lRes );

    if ( pszText2 && IS_INTRESOURCE( pszText2 ))
    {
        LoadStringWrapW( g_hinstDll, PtrToUlong(pszText), szStatusText, ARRAYSIZE(szStatusText) );
        pszText2 = szStatusText;
    }

    // display the message.
    m_pBrowser->SendControlMsg( FCW_STATUS, SB_SETTEXTW, 1, ( LPARAM )pszText2, &lRes );
}


////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmMenuSelect( HWND hWnd, WPARAM wParam, LPARAM lParam )
{
    if ( m_pCMCache == NULL )
    {
        // there is no cached menu, forward it to the parent window
        return SendMessageA( m_hWndParent, WM_MENUSELECT, wParam, lParam );
    }
    else
    {
        if ( lParam != NULL && HIWORD( wParam ) != 0xffff )
        {
            // the user has not canceled the menu
            WCHAR szMessage[MAX_PATH];

            szMessage[0] = 0;
            UINT idCmd = LOWORD( wParam ) - m_idCMStartOffset;
            UINT rgfFlags = HIWORD( wParam );

            if ( LOWORD( wParam ) > m_idCMStartOffset
                && !( rgfFlags & MF_SEPARATOR ))
            {
                HRESULT hRes = m_pCMCache->GetCommandString( idCmd,
                                                             GCS_HELPTEXTW,
                                                             NULL,
                                                             (LPSTR) szMessage,
                                                             ARRAYSIZE(szMessage) );
                                                             
                // we have tried the uncode stuff, now try the ansi stuff.....
                if ( hRes != S_OK )
                {
                    CHAR szTmp[MAX_PATH];
                    szTmp[0] = 0;

                    hRes = m_pCMCache->GetCommandString( idCmd,
                                                         GCS_HELPTEXTA,
                                                         NULL,
                                                         szTmp,
                                                         ARRAYSIZE(szTmp) );
                    if ( hRes == S_OK )
                    {
                        MultiByteToWideChar( CP_ACP, 0, szTmp, -1, szMessage, MAX_PATH );
                    }
                }
            }

            LRESULT lRes = 0;
            // send the text to the status bar ...
            m_pBrowser->SendControlMsg( FCW_STATUS,
                                        SB_SETTEXTW,
                                        ( WPARAM )0,
                                        ( LPARAM )( LPWSTR )szMessage,
                                        &lRes );


            m_pBrowser->SendControlMsg( FCW_STATUS,
                                        SB_SETTEXTA,
                                        ( WPARAM )1,
                                        ( LPARAM )( LPTSTR )"",
                                        &lRes );
        }

        return FALSE;
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnLVNEndLabelEdit( NMHDR * pHdr )
{
    LV_DISPINFOW * pInfo = (LV_DISPINFOW * ) pHdr ;
    WCHAR szName[MAX_PATH];
    LPITEMIDLIST pPidl = (LPITEMIDLIST) pInfo->item.lParam;
    LPITEMIDLIST pNewPidl = NULL;
    int iImage = pInfo->item.iImage;

    if ( pInfo->item.pszText == NULL || pInfo->item.iItem == -1 )
    {
        // the user has cancelled the edit....
        return FALSE;
    }

    LV_ITEMW rgItem;
    memset( &rgItem, 0, sizeof( LV_ITEM ) );
    if ( !(pInfo->item.mask & (LVIF_PARAM|LVIF_IMAGE)))
    {
        // if we weren't give the pidl, go get it....
        rgItem.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_NORECOMPUTE;
        rgItem.iItem = pInfo->item.iItem;
        ListView_GetItemWrapW( m_hWndListView, &rgItem );
        pPidl = (LPITEMIDLIST) pInfo->item.lParam;
        iImage = rgItem.iImage;
    }

    // all UNICODE now ...
    // copy the new name the user entered
    // MultiByteToWideChar( CP_ACP, 0, pInfo->item.pszText, -1, szName, MAX_PATH );
    StrCpyW( szName, pInfo->item.pszText );

    // the pidl might have dispappeared from under us if they put up a dialog box
    // on failure because of the disk notifications.
    LPITEMIDLIST pTmpPidl = ILClone( pPidl );
    if ( pTmpPidl == NULL )
    {
        MessageBoxWrapW( m_hWndListView,
                    (LPCWSTR)MAKEINTRESOURCE( IDS_ERR_OUTOFMEM ),
                    (LPCWSTR)MAKEINTRESOURCE( IDS_RENAME_TITLE ),
                    MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );
        return FALSE;
    }

    // the folder should handle the renaming including the old extension, as it is
    // the only one that knows what the old extension was ....
    HRESULT hRes = m_pFolder->SetNameOf( m_hWnd, pPidl, szName, 0, &pNewPidl );
    if ( SUCCEEDED( hRes ) )
    {
        // if we asked for a new pidl, then we should have one ....
        Assert( pNewPidl != NULL );

        // update the text on screen (I'm surprised it doesn't do this
        // automatically ...

        // update the pidl ..
        rgItem.iItem = pInfo->item.iItem;
        rgItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        rgItem.lParam = (LPARAM) pNewPidl;

        Assert( m_pImageCache );
        m_pImageCache->DeleteImage( iImage );
        
        // use the text we were given ...
        rgItem.pszText = pInfo->item.pszText;
        rgItem.iImage = I_IMAGECALLBACK;
        ListView_SetItemWrapW( m_hWndListView, &rgItem );

        // should free the old pidl
        SHFree(pPidl);
    }
    else
    {
        // we have failed, we must find the item again because if they put up a dialog
        // telling the user, it is highly likely that shell change notifications have
        // been processed and we might not be on the same item. .....
        int iItem = FindInView( m_hWndListView, m_pFolder, pTmpPidl );

        if ( iItem >= 0 )
        {
            SendMessageWrapW( m_hWndListView,
                              LVM_EDITLABEL,
                              pInfo->item.iItem,
                              (LPARAM)pInfo->item.pszText);
        }
    }

    SHFree(pTmpPidl);

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnDefaultAction( BOOL fDoubleClick )
{
    int iHitItem = -1;
    BOOL fAlt = (GetAsyncKeyState( VK_MENU ) < 0);

    if ( fDoubleClick )
    {
        // only do a hit-test if there was a double-click.
        POINT rgPoint;
        DWORD dwPos = GetMessagePos();
        rgPoint.x = LOWORD(dwPos);
        rgPoint.y = HIWORD(dwPos);

        BOOL bRes = ScreenToClient( m_hWndListView, &rgPoint );
        LV_HITTESTINFO rgHit;
        rgHit.pt = rgPoint;
        rgHit.flags = LVHT_ONITEM;
        // if there was a double click on the view, ignore it.
        int iRes = ListView_HitTest( m_hWndListView, &rgHit );
        if ( iRes == -1 )
        {
            return FALSE;
        }
        iHitItem = iRes;
    }

    LPCONTEXTMENU pMenu = NULL;
    HRESULT hRes = NOERROR;

    // we hit something, use the selection from here on it....
    int iCount = ListView_GetSelectedCount( m_hWndListView );
    // if there is nothing selected ( this would only happen on enter ),
    // then ignore it.
    if ( iCount <= 0 )
    {
        return FALSE;
    }

    LPCITEMIDLIST * apidl = new LPCITEMIDLIST[iCount];
    if ( apidl == NULL )
    {
        // TODO: low memory situations
        MessageBoxWrapW( m_hWnd,
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_OUTOFMEM ),
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_SHELLTITLE ),
                    MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );

    }

    GetSelectionPidlList( m_hWndListView, iCount, apidl, iHitItem );

    UINT rgfFlags = 0;

    // now generate the context menu for it ...
    hRes = m_pFolder->GetUIObjectOf( m_hWnd,
                                     iCount,
                                     apidl,
                                     IID_IContextMenu,
                                     & rgfFlags,
                                     (void **) & pMenu );

    if ( FAILED( hRes ))
    {
        MessageBoxWrapW( m_hWnd,
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_CONTEXTMENUFAILED),
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_SHELLTITLE ),
                    MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );
    }

    HMENU hMenu = CreatePopupMenu();
    if ( hMenu == NULL )
    {
        MessageBoxWrapW( m_hWnd,
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_CONTEXTMENUFAILED ),
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_SHELLTITLE ),
                    MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );
    }

    UINT grfFlags = CMF_DEFAULTONLY;
    if ( m_fExploreMode != FALSE )
    {
        grfFlags |= CMF_EXPLORE;
    }
    hRes = pMenu->QueryContextMenu( hMenu,
                                    0,
                                    SUBIDSTART,
                                    SUBIDEND,
                                    grfFlags );
    if ( FAILED( hRes ))
    {
        MessageBoxWrapW( m_hWnd,
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_CONTEXTMENUFAILED ),
                    (LPWSTR)MAKEINTRESOURCE( IDS_ERR_SHELLTITLE ),
                    MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );
    }


    int idCmd = -1;
    if ( !fAlt )
    {
        // find the default command ID ..
        int iSize = GetMenuItemCount( hMenu );

        for ( int iItem = 0; iItem < iSize; iItem ++ )
        {
            MENUITEMINFOW mii;
            mii.cbSize = sizeof( mii );
            mii.fMask = MIIM_ID | MIIM_STATE;
            GetMenuItemInfoWrapW(hMenu, iItem, TRUE, &mii);

            if ((mii.fState & MFS_DEFAULT) && !(mii.fState & (MFS_GRAYED | MFS_DISABLED)))
            {
                idCmd = (int) mii.wID;
                break;
            }
        }

        // use the first item anyway if there is no default ....
        if ( idCmd == -1 )
        {
            MENUITEMINFOW mii;
            mii.cbSize = sizeof( mii );
            mii.fMask = MIIM_ID | MIIM_STATE;
            GetMenuItemInfoWrapW(hMenu, 0, TRUE, &mii);
            idCmd = mii.wID;

            if ( mii.fState & ( MFS_GRAYED | MFS_DISABLED ))
            {
                idCmd = -1;
            }
        }
    }
    
    if ( idCmd != -1 || fAlt )
    {
        WCHAR szWDirectory[MAX_PATH];
        CHAR szDirectory[MAX_PATH];

        Assert( m_pidl );
        SHGetPathFromIDListWrapW( m_pidl, szWDirectory );
        SHUnicodeToAnsi( szWDirectory, szDirectory, ARRAYSIZE( szDirectory));
            
        if ( IsOS(OS_NT))
        {
            CMINVOKECOMMANDINFOEX rgCmd;
            memset( & rgCmd, 0, sizeof( rgCmd ));
            rgCmd.cbSize = sizeof( rgCmd );

            if ( fAlt )
            {
                rgCmd.lpVerb = "Properties";
                rgCmd.lpVerbW = L"Properties";
            }
            else
            {
                rgCmd.lpVerb = (LPSTR) idCmd - SUBIDSTART;
                rgCmd.lpVerbW = (LPWSTR) rgCmd.lpVerb;
            }
            
            rgCmd.nShow = SW_NORMAL;
            m_pBrowser->GetWindow( &(rgCmd.hwnd));
            rgCmd.fMask = CMIC_MASK_UNICODE;
            
            // execute the command finally !!
            hRes = pMenu->InvokeCommand( (CMINVOKECOMMANDINFO *) &rgCmd );

        }
        else
        {
            CMINVOKECOMMANDINFO rgCmd;
            memset( & rgCmd, 0, sizeof( rgCmd ));
            rgCmd.cbSize = sizeof( rgCmd );

            if ( fAlt )
            {
                rgCmd.lpVerb = "Properties";
            }
            else
            {
                rgCmd.lpVerb = (LPSTR) idCmd - SUBIDSTART;
            }
            rgCmd.nShow = SW_NORMAL;
            m_pBrowser->GetWindow( &(rgCmd.hwnd));
            rgCmd.lpDirectory = szDirectory;

            // execute the command finally !!
            hRes = pMenu->InvokeCommand( &rgCmd );
        }
    }

    DestroyMenu( hMenu );
    pMenu->Release();
    delete [] apidl;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmCreate( HWND hWnd, LPCREATESTRUCT pCreate )
{
    SetWindowLongPtrA( hWnd, 0, (LONG_PTR) this );

    // remember the hWnd
    m_hWnd = hWnd;

    // create the Enhanced view with a size of zero (remember the move is coming next ...)
    DWORD dwStyle = LVS_ICON | LVS_SHAREIMAGELISTS | LVS_EDITLABELS | LVS_SHOWSELALWAYS;
    if ( m_rgSettings.fFlags & FWF_SINGLESEL )
    {
        dwStyle |= LVS_SINGLESEL;
    }

    if ( m_rgSettings.fFlags & FWF_NOSCROLL )
    {
        dwStyle |= LVS_NOSCROLL;
    }

    if ( m_rgSettings.fFlags & FWF_ALIGNLEFT )
    {
        dwStyle |= LVS_ALIGNLEFT;
    }

    m_hWndListView = CreateWindowExWrapW( WS_EX_CLIENTEDGE,
        WC_LISTVIEWW, NULL,
        WS_CHILD | WS_CLIPCHILDREN | WS_VISIBLE | dwStyle,
        0, 0,
        0, 0, m_hWnd, NULL, g_hinstDll, NULL);

    if ( m_hWndListView == NULL )
    {
        // fail the create...
        return -1;
    }

    // get the imagelists we will use, here we just grab the system ones
    SHFILEINFO rgInfo;
    m_hSysLargeImgLst = (HIMAGELIST) SHGetFileInfo( TEXT("*.exe"), 0, &rgInfo, sizeof(rgInfo),
                            SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_LARGEICON );

    m_hSysSmallImgLst = (HIMAGELIST) SHGetFileInfo( TEXT("*.exe"), 0, &rgInfo, sizeof(rgInfo),
                            SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_SMALLICON );

    Assert( m_himlThumbs );
    ListView_SetImageList( m_hWndListView, m_himlThumbs, LVSIL_NORMAL );

    ListView_Arrange( m_hWndListView, LVA_SNAPTOGRID );

    int cxIcon, cyIcon;
    ImageList_GetIconSize( m_hSysLargeImgLst, &cxIcon, &cyIcon );

    ListView_SetIconSpacing(m_hWndListView, cxIcon / 4 + m_iXSizeThumbnail + 1, 0);

    // create the window's accelerator table.
    // TODO:
    // m_hAccel = LoadAccelerators( g_hinstDll, MAKEINTRESOURCE( IDR_ACCELERATOR1 ));
    DWORD dwMainStyle;
    if ( !(m_rgSettings.fFlags & FWF_AUTOARRANGE) )
    {
        // turn off autoarrange if not asked for ....
        dwMainStyle = GetWindowStyle( m_hWndListView) & ~LVS_AUTOARRANGE;
        SetWindowLongWrapW( m_hWndListView, GWL_STYLE, dwMainStyle );
    }
    else
    {
        // turn on auto arrange in enhanced view.
        // this has to be done after setting the imagelist for the enhanced view,
        // so that the auto-arrange will work with the appropriate thumbnail size.
        dwMainStyle = GetWindowStyle( m_hWndListView) | LVS_AUTOARRANGE;
        SetWindowLongWrapW( m_hWndListView, GWL_STYLE, dwMainStyle );
    }

    ListView_SetExtendedListViewStyle( m_hWndListView, LVS_EX_INFOTIP | LVS_EX_BORDERSELECT );

    CheckViewOptions();

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnWmDestroy( HWND hWnd )
{
    if ( m_hWndListView != NULL )
    {
        // ensure it has been emptied ....
        HWND hWndList = m_hWndListView;
        m_hWndListView = NULL;
        Clear( hWndList );
        DestroyWindow( hWndList );
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::StartDragDrop( NM_LISTVIEW * pNMHdr )
{
    LPDATAOBJECT pDataObject = NULL;
    LPDROPSOURCE pDropSrc = NULL;
    HRESULT hRes = NOERROR;
    DWORD dwEffect = DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY;
    UINT rgfFlags;
    CComObject<CViewDropSource> * pSrc = NULL;
    LPCITEMIDLIST * apidl = NULL;
    int iSelCount = 0;

    // get the view origin
    this->GetOrigin( & m_ptDragStart );

    // add on the current position within the view, therefore getting the true view start pos
    m_ptDragStart.x += pNMHdr->ptAction.x;
    m_ptDragStart.y += pNMHdr->ptAction.y;

    m_pDropTarget->DragStartHere( & m_ptDragStart );

    // how many elements selected ?
    iSelCount = ListView_GetSelectedCount( m_hWndListView );
    apidl = new LPCITEMIDLIST[iSelCount];
    if ( apidl == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // pidl list big enough for the selection
    hRes = GetSelectionPidlList( m_hWndListView, iSelCount, apidl, pNMHdr->iItem );
    if ( FAILED( hRes ) )
    {
        hRes = E_FAIL;
        goto SDDErrorRecovery;
    }

    // fetch the attributes of the selection
    hRes = m_pFolder->GetAttributesOf( iSelCount, apidl, &dwEffect );
    if ( FAILED( hRes ) )
    {
        // use the hRes fail result
        goto SDDErrorRecovery;
    }

    // ignore all other bits returned....
    dwEffect &= (DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY );

    hRes = NOERROR;
    pSrc = new CComObject<CViewDropSource>;
    if ( pSrc != NULL )
    {
        if ( FAILED( hRes ))
        {
            delete pSrc;
            goto SDDErrorRecovery;
        }

        // get a ref on the object...
        hRes = pSrc->QueryInterface( IID_IDropSource, (void ** ) & pDropSrc );
        Assert( SUCCEEDED( hRes ));
    }
    else
    {
        hRes = E_OUTOFMEMORY;
        goto SDDErrorRecovery;
    }

    rgfFlags = SVGIO_SELECTION;
    // get the DataObject covering the selection...
    hRes = m_pFolder->GetUIObjectOf( m_hWnd, iSelCount, apidl, IID_IDataObject,
        &rgfFlags, (void ** ) &pDataObject);
    if ( SUCCEEDED( hRes ) )
    {
        // used to detect if we are dropping in the same view ...
        m_fDragStarted = TRUE;

        // try and set the item positions in the data object...
        SetPointData( pDataObject, apidl, iSelCount );
        
        // Before we enter drag drop, try and initialize the drag images.
        if (m_pDragImages)
            m_pDragImages->InitializeFromWindow(m_hWndListView, 0, pDataObject);

        // here we go, ole drag and drop ....
        hRes = DoDragDrop( pDataObject, pDropSrc, dwEffect, &dwEffect);

        m_fDragStarted = FALSE;

        pDropSrc->Release();
        pDropSrc = NULL;
        pDataObject->Release();
    }

SDDErrorRecovery:
    if ( pDropSrc != NULL )
    {
        pDropSrc->Release();
    }

    if ( apidl != NULL )
    {
        delete [] (LPITEMIDLIST *) apidl;
    }

    m_fDragStarted = FALSE;

    return hRes;
}

//////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnViewGetThumbnail( NMHDR * pHdr )
{
    LV_DISPINFO * pInfo = (LV_DISPINFO * ) pHdr ;
    WCHAR szFileName[MAX_PATH];
    HBITMAP hBmpThumb = NULL;
    UINT iImageIndex = (UINT) I_IMAGECALLBACK;
    LPCITEMIDLIST pidl = ( LPCITEMIDLIST )pInfo->item.lParam;
    UINT rgFlags = 0;
    UINT iIndex = (UINT) I_IMAGECALLBACK;
    HRESULT hr = NOERROR;
    BOOL fStampIcon = FALSE;

    if ( pidl == NULL )
    {
        // we should always get a pidl...
        Assert( FALSE );
        return TRUE;
    }

    DWORD dwMask = GetOverlayMask( pidl );

    DWORD dwStoreFlags = 0;

    // is the thumbnail cache at its limit ? If so, we had better try and make space..
    UINT uTmp = 0;
    m_pImageCache->GetCacheSize( &uTmp);
    int iCacheSpace = m_iMaxCacheSize - uTmp;

    int iQueuedFetch = 0;
    if ( m_pScheduler )
    {
        iQueuedFetch = m_pScheduler->CountTasks( TOID_ExtractImageTask );
        iQueuedFetch += m_pScheduler->CountTasks( TOID_DiskCacheTask );
        iQueuedFetch -= m_pScheduler->CountTasks( TOID_ImgCacheTidyup );
    }

    iCacheSpace -= iQueuedFetch;

    // add enough scroungers to shrink the cache to the right size...
    while ((iCacheSpace ++) < 0)
    {
        IRunnableTask *pTask;
        hr = CImgCacheTidyup_Create( m_pImageCache, FALSE, m_hWndListView, &m_iLastCtr, &pTask );
        if ( SUCCEEDED( hr ))
        {
            if ( m_pScheduler )
            {
                m_pScheduler->AddTask( pTask,
                                       TOID_ImgCacheTidyup,
                                       ITSAT_DEFAULT_LPARAM,
                                       PRIORITY_NORMAL );
            }
            else
            {
                pTask->Run();
                pTask->Release();
            }
        }
    }

    hr = ExtractItem( &iImageIndex, pInfo->item.iItem, pidl, TRUE, FALSE );
    if ( hr != S_OK )
    {
        // create the default one for that file type,
        // the index into the sys image list is used to detect items of the
        // same type, thus we only generate one default thumbnail for each
        // particular icon needed
        iIndex = (UINT) ViewGetIconIndex( pidl );

        if ( iIndex == (UINT) I_IMAGECALLBACK )
            iIndex = II_DOCNOASSOC;

        // check if the image is already in the image cache.
        IMAGECACHEINFO rgInfo;
        rgInfo.cbSize = sizeof( rgInfo );
        rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_INDEX;
        rgInfo.pszName = c_szDefault;
        rgInfo.dwFlags = dwMask;
        rgInfo.iIndex = (int) iIndex;

        hr = m_pImageCache->FindImage( &rgInfo, &iImageIndex );
        if ( hr != S_OK )
        {
            hr = CreateDefaultThumbnail( iIndex, &hBmpThumb, m_fDrawBorder );
            StrCpyW( szFileName, c_szDefault );
            iImageIndex = (UINT) I_IMAGECALLBACK;
        }
    }

    if ( hBmpThumb != NULL )
    {
        // we are creating a new one, so we shouldn't have an index yet ..
        Assert( iImageIndex == I_IMAGECALLBACK );

#if 0   // never gets called, fStampIcon always false . . .
        if ( fStampIcon )
        {
            // a small icon is stamped in bottom right-hand corner of thumbnail.
            StampIconOnThumbnail( pidl, hBmpThumb );
        }
#endif

        // copy thumbnail into the imagelist.
        IMAGECACHEINFO rgInfo;
        rgInfo.cbSize = sizeof( rgInfo );
        rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_INDEX | ICIFLAG_LARGE | ICIFLAG_BITMAP;
        rgInfo.pszName = szFileName;
        rgInfo.dwFlags = dwMask;
        rgInfo.iIndex = (int) iIndex;
        rgInfo.hBitmapLarge = hBmpThumb;
        rgInfo.hMaskLarge = NULL;

        if ( !fStampIcon )
        {
            rgInfo.dwMask |= ICIFLAG_NOUSAGE;
        }
        if(IS_BIDI_LOCALIZED_SYSTEM())
        {
            rgInfo.dwMask |= ICIFLAG_MIRROR;
        }
        hr = m_pImageCache->AddImage( &rgInfo, &iImageIndex );
        DeleteObject( hBmpThumb );
    }

    pInfo->item.iImage = (int) iImageIndex;

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////////////////
DWORD CThumbnailView::GetOverlayMask( LPCITEMIDLIST pidl )
{
    DWORD   dwLink = SFGAO_LINK | SFGAO_SHARE | SFGAO_GHOSTED;

    m_pFolder->GetAttributesOf( 1, &pidl, &dwLink );
    return dwLink;
}

////////////////////////////////////////////////////////////////////////////////////////////
int CThumbnailView::ViewGetIconIndex( LPCITEMIDLIST pidl )
{
    // do we have an IShellIcon interface ?
    if ( m_pIcon != NULL )
    {
        int iIndex = -1;

        HRESULT hRes = m_pIcon->GetIconOf( pidl, 0, &iIndex );
        // check to see if we succeeded and we weren't told to extract the icon
        // ourselves ...

        if ( SUCCEEDED( hRes ) && hRes != S_FALSE )
        {
            IShellIconOverlay * pio;
            if ( SUCCEEDED( m_pFolder->QueryInterface( IID_IShellIconOverlay, (LPVOID *) & pio )))
            {
                int iOverlay;
                if ( SUCCEEDED( pio->GetOverlayIndex( pidl, &iOverlay )))
                {
                    iIndex |= iOverlay << 24;
                }
                pio->Release();
            }
            return iIndex;
        }
    }

    SHFILEINFO rgInfo;
    LPITEMIDLIST pidlFull;

    rgInfo.iIcon = -1;
    pidlFull = ILCombine( m_pidl, pidl );
    if (pidlFull)
    {
        SHGetFileInfo( (LPCTSTR) pidlFull, 0, &rgInfo, sizeof( rgInfo ),
            SHGFI_SYSICONINDEX | SHGFI_LARGEICON | SHGFI_PIDL | SHGFI_OVERLAYINDEX );
        ILFree( pidlFull );
    }

    return ( rgInfo.iIcon >= 0 ) ? rgInfo.iIcon : II_DOCNOASSOC;
}

////////////////////////////////////////////////////////////////////////////////////////////
//puts the associated icon in bottom left-hand side of thumbnail.
void CThumbnailView::StampIconOnThumbnail( LPCITEMIDLIST pidl, HBITMAP hBmpThumb, BOOL fBorder )
{
    HDC hDC = GetDC( m_hWndListView );
    HDC hDCMem = CreateCompatibleDC( hDC );
    int iImage = ViewGetIconIndex( pidl );

    // select the thumbnail bitmap into the memory DC.
    HGDIOBJ hTmp = SelectObject( hDCMem, hBmpThumb );

    if (m_fIconStamp)
    {
        ImageList_Draw( m_hSysSmallImgLst, (iImage & 0x00ffffff) , hDCMem, 
                    m_iXSizeThumbnail - 20, // left.
                    m_iYSizeThumbnail - 20, // top.
                    ILD_TRANSPARENT | (INDEXTOOVERLAYMASK(iImage >> 24)) );
    }
    if ( fBorder )
    {
        // put a black shadow border on it...
        // assume the bitmap is the size we specified...
        DrawShadowBorder(hDCMem, 0, 0, m_iXSizeThumbnail, m_iYSizeThumbnail );
    }

    // get the bitmap back.
    SelectObject( hDCMem, hTmp );

    // release stuff.
    ReleaseDC( m_hWndListView, hDC );
    DeleteDC( hDCMem );
}

#define COLOR_BLACK     RGB(0, 0, 0)
#define COLOR_DKGRAY    RGB(128, 128, 128)
#define COLOR_LTGRAY    RGB(192, 192, 192)
#define COLOR_WHITE     RGB(255, 255, 255)
void CThumbnailView::DrawShadowBorder(HDC hdc, int x, int y, int dx, int dy)
{
    RECT rc;
    COLORREF clrSave = SetBkColor(hdc, COLOR_DKGRAY);
    int iOldMM = SetMapMode( hdc, MM_TEXT );
    //left
    rc.left = x;
    rc.top = y;
    rc.right = x + 1;
    rc.bottom = y + dy;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    //top
    rc.left = rc.right;
    rc.right = rc.left + dx - 2;
    rc.bottom = rc.top + 1;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    //right inner
    SetBkColor(hdc, COLOR_LTGRAY);
    rc.left = rc.right - 1;
    rc.top = rc.bottom;
    rc.bottom = rc.top + dy - 3; 
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    // right outer
    SetBkColor(hdc, COLOR_BLACK);
    rc.left = rc.right;
    rc.right = rc.left + 1;
    rc.top-=1;
    rc.bottom = rc.top + dy; 
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    // bottom inner
    SetBkColor(hdc, COLOR_LTGRAY);
    rc.left = x + 1;
    rc.right= rc.left + dx - 2;
    rc.bottom-=1;
    rc.top = rc.bottom - 1;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    // bottom outer
    SetBkColor(hdc, COLOR_BLACK);
    rc.left-=1;
    rc.right+=1;
    rc.top = rc.bottom;
    rc.bottom= rc.top + 1;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);

    SetMapMode( hdc, iOldMM );
    SetBkColor(hdc, clrSave);
    return;
}


/////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::CreateDefaultThumbnail( int iIndex, HBITMAP * phBmpThumbnail, BOOL fDrawBorder)
{
    
    HDC         hdc = GetDC(NULL);
    HDC         hMemDC = CreateCompatibleDC(hdc);
    HRESULT     hr = E_FAIL;

    // get the background for the default thumbnail.
    if (!hMemDC)
        goto LGone;
    *phBmpThumbnail = CreateCompatibleBitmap(hdc, m_iXSizeThumbnail, m_iYSizeThumbnail);
    if ( *phBmpThumbnail )
    {
        HGDIOBJ hTmp = SelectObject( hMemDC, *phBmpThumbnail );
        COLORREF clrOld = SetBkColor(hMemDC, GetSysColor(COLOR_WINDOW));
        RECT rc = {0, 0, m_iXSizeThumbnail, m_iYSizeThumbnail};

        ExtTextOut(hMemDC,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
        SetBkColor(hMemDC, clrOld);

        int cxIcon, cyIcon, x, y, dx, dy;

        // calculate position and width of icon.
        ImageList_GetIconSize( m_hSysLargeImgLst, &cxIcon, &cyIcon );
        if ( cxIcon < m_iXSizeThumbnail)
        {
            x = ( m_iXSizeThumbnail - cxIcon ) / 2;
            dx = cxIcon;
        }
        else
        {
            // in case icon size is larger than thumbnail size.
            x = 0;
            dx = m_iXSizeThumbnail;
        }

        if ( cyIcon < m_iYSizeThumbnail )
        {
            y = ( m_iYSizeThumbnail - cyIcon ) / 2;
            dy = cyIcon;
        }
        else
        {
            // in case icon size is larger than thumbnail size.
            y = 0;
            dy = m_iYSizeThumbnail;
        }

        ImageList_DrawEx( m_hSysLargeImgLst, (iIndex & 0x00ffffff), hMemDC, 
                    x, // left.
                    y, // top.
                    dx,
                    dy,
                    CLR_DEFAULT,
                    CLR_DEFAULT,
                    ILD_TRANSPARENT | (INDEXTOOVERLAYMASK(iIndex >> 24)));

        if (m_fDrawBorder)
        {
            DrawShadowBorder(hMemDC, 0, 0, m_iXSizeThumbnail, m_iYSizeThumbnail );
        }
        
        // get the bitmap produced so that it will be returned.
        *phBmpThumbnail = (HBITMAP) SelectObject( hMemDC, hTmp );
        hr = S_OK;
    }

LGone:
    if (hMemDC)
      DeleteDC(hMemDC);
    ReleaseDC(NULL, hdc);
    return hr;
}


int CThumbnailView::FindItem( LPCITEMIDLIST pidl )
{
    return FindInView( m_hWndListView, m_pFolder, pidl );
}


HRESULT CThumbnailView::TaskUpdateItem( LPCITEMIDLIST pidl,
                                        int iItem,
                                        DWORD dwMask,
                                        LPCWSTR pszPath,
                                        const FILETIME * pftDateStamp,
                                        int iThumbnail,
                                        HBITMAP hBmp )
{
    // check the size of the bitmap to make sure it is big enough, if it is not, then 
    // we must center it on a background...
    BITMAP rgBitmap;
    HBITMAP hBmpCleanup = NULL;
    HRESULT hr = E_FAIL;

    if ( ::GetObjectWrapW( (HGDIOBJ) hBmp, sizeof( rgBitmap ), (LPVOID) &rgBitmap ))
    {
        // if the image is the wrong size, or the wrong colour depth, then do the funky stuff on it..
        if ( rgBitmap.bmWidth != m_iXSizeThumbnail || 
             rgBitmap.bmHeight != m_iYSizeThumbnail ||
             rgBitmap.bmBitsPixel > m_dwRecClrDepth )
        {
            // alloc the colour table just incase....
            LPBITMAPINFO pInfo = (LPBITMAPINFO) LocalAlloc( LPTR, sizeof( BITMAPINFO ) + sizeof(RGBQUAD)  * 256 );
            if ( pInfo )
            {
                // get a DC for this operation...
                HDC hdcMem = CreateCompatibleDC( NULL );
                if (hdcMem)
                {
                    pInfo->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
                    if ( GetDIBits( hdcMem, hBmp, 0, 0, NULL, pInfo, DIB_RGB_COLORS ))
                    {
                        // we have the header, now get the data....
                        void *pBits = LocalAlloc( LPTR, pInfo->bmiHeader.biSizeImage );
                        if ( pBits )
                        {
                            if ( GetDIBits( hdcMem, hBmp, 0, pInfo->bmiHeader.biHeight, pBits, pInfo, DIB_RGB_COLORS ))
                            {
                                SIZE rgSize = {m_iXSizeThumbnail, m_iYSizeThumbnail};
                                RECT rgRect = {0, 0, rgBitmap.bmWidth, rgBitmap.bmHeight};

                                CalculateAspectRatio( &rgSize, &rgRect );

                                if ( FactorAspectRatio( pInfo, pBits, &rgSize, rgRect, m_dwRecClrDepth, m_hpal, FALSE, &hBmpCleanup))
                                {
                                    // finally success :-) we have the new image we can abandon the old one...
                                    hBmp = hBmpCleanup;
                                    hr = S_OK;
                                }
                            }

                            LocalFree( pBits );
                        }
                    }

                    // cleanup ....
                    DeleteDC( hdcMem );
                }

                LocalFree( pInfo );
           }
        }
        else
        {
            // the original bitmap is fine
            hr = S_OK;
        }
    }

    UINT iImage;
    if ( SUCCEEDED(hr) )
    {
        // a small icon is stamped in bottom right-hand corner of thumbnail.
        StampIconOnThumbnail( pidl, hBmp, m_fDrawBorder );

        // check if we are going away, if so, then don't use Sendmessage because it will block the
        // destructor of the scheduler...
        if ( m_fDestroying )
        {
            hr = E_FAIL;
        }
        else
        {
            // copy thumbnail into the cache.
            IMAGECACHEINFO rgInfo;
            rgInfo.cbSize = sizeof( rgInfo );
            rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_INDEX | ICIFLAG_LARGE | ICIFLAG_BITMAP;
            rgInfo.pszName = pszPath;
            rgInfo.dwFlags = dwMask;
            rgInfo.iIndex = (int) iThumbnail;
            rgInfo.hBitmapLarge = hBmp;
            rgInfo.hMaskLarge = NULL;

            if ( pftDateStamp )
            {
                rgInfo.dwMask |= ICIFLAG_DATESTAMP;
                rgInfo.ftDateStamp = *pftDateStamp;
            }

            if (IS_BIDI_LOCALIZED_SYSTEM())
            {
                rgInfo.dwMask |= ICIFLAG_MIRROR;
            }

            hr = m_pImageCache->AddImage( &rgInfo, &iImage );
        }
    }

    if (hBmpCleanup)
    {
        DeleteObject( hBmpCleanup );
    }
    
    if ( hr == S_OK )
    {
        // check if we are going away, if so, then don't use Sendmessage because it will block the
        // destructor of the scheduler...
        if ( m_fDestroying )
        {
            return E_FAIL;
        }

        // post a message to the main thread so we don't block....
        ::PostMessageA( m_hWnd, WM_UPDATEITEMIMAGE, iItem, iImage );
    }

    return hr;
}

HRESULT CThumbnailView::ViewUpdateThumbnail( LPCITEMIDLIST pidl, LPCWSTR pszPath, HBITMAP hBitmap )
{
    int iItem = FindInView( m_hWndListView, m_pFolder, pidl );
    if ( iItem == -1 )
    {
        return E_INVALIDARG;
    }

    DWORD dwMask = GetOverlayMask( pidl );

    LV_ITEMW rgItem;
    ZeroMemory( &rgItem, sizeof( rgItem ));
    rgItem.mask = LVIF_IMAGE | LVIF_NORECOMPUTE;
    rgItem.iItem = iItem;

    BOOL bRes = ListView_GetItemWrapW( m_hWndListView, &rgItem );
    Assert( bRes );

    /*[TODO: do something about persisting the image ... and color reduction ]*/

    // a small icon is stamped in bottom right-hand corner of thumbnail.
    StampIconOnThumbnail( pidl, hBitmap, m_fDrawBorder );

    if ( rgItem.iImage != I_IMAGECALLBACK )
    {
        // if it is a shared thumbnail, then the cache will handle if it is to be
        // deleted or not.
        m_pImageCache->FreeImage((UINT) rgItem.iItem );
    }

    // there is no item in the cache for it, so we can tag it on the end
    UINT iImage;
    IMAGECACHEINFO rgInfo;
    rgInfo.cbSize = sizeof( rgInfo );
    rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS | ICIFLAG_BITMAP | ICIFLAG_LARGE;
    rgInfo.pszName = pszPath;
    rgInfo.dwFlags = dwMask;
    rgInfo.hBitmapLarge = hBitmap;
    rgInfo.hMaskLarge = NULL;

    if(IS_BIDI_LOCALIZED_SYSTEM())
    {
        rgInfo.dwMask |= ICIFLAG_MIRROR;
    }

    HRESULT hr = m_pImageCache->AddImage( &rgInfo, &iImage );
    if ( FAILED( hr ))
    {
        return hr;
    }
    rgItem.iImage = (int) iImage;

    ListView_SetItemWrapW( m_hWndListView, & rgItem );

    RECT rcImage;
    ListView_GetItemRect( m_hWndListView, rgItem.iItem, &rcImage, LVIR_BOUNDS );
    InvalidateRect( m_hWndListView, &rcImage, FALSE );

    return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////////////////
#define BLOCKSIZE 100

void CThumbnailView::UpdateWithoutRefresh()
{
    // which one are we looking at ?
    LPENUMIDLIST pEnum = NULL;

    // on error do a refresh
    BOOL fRefresh = FALSE;
    int iIndex = -1;
    ULONG celtFetched = 0;
    ULONG celtThisBlock = 0;
    HRESULT hRes = NOERROR;
    ULONG ulType = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

    LPITEMIDLIST * apCurBlock = NULL;
    LPITEMIDLIST * apElems = new LPITEMIDLIST[BLOCKSIZE + 1];
    if ( apElems == NULL )
    {
        fRefresh = TRUE;
        goto UpdateCleanup;
    }

    apCurBlock = apElems;
    apCurBlock[BLOCKSIZE] = NULL;

    if ( m_fShowAllObjects )
    {
       ulType |= SHCONTF_INCLUDEHIDDEN;
    }

    hRes = m_pFolder->EnumObjects( this->m_hWnd, ulType,
        &pEnum);

    if ( FAILED( hRes ) )
    {
        fRefresh = TRUE;
        goto UpdateCleanup;
    }

    // fetch all the new elements ...
    do
    {
        // we cannot assume that Next() can return more than one element at
        // a time, as most of the IShellFolder implementers of this only
        // support one at a time... (so much for relying on the spec....)
        celtFetched = 0;
        hRes = pEnum->Next( BLOCKSIZE - celtThisBlock, apCurBlock + celtThisBlock, &celtFetched );
        if ( celtFetched == 0 )
        {
            break;
        }

        // add this lot to the current block count
        celtThisBlock += celtFetched;

        if ( celtThisBlock == BLOCKSIZE )
        {
            // allocate next block
            apCurBlock[BLOCKSIZE] = (LPITEMIDLIST) new LPITEMIDLIST[BLOCKSIZE + 1];
            if ( apCurBlock[BLOCKSIZE] == NULL )
            {
                fRefresh = TRUE;
                goto UpdateCleanup;
            }
            else
            {
                celtThisBlock = 0;
                apCurBlock = (LPITEMIDLIST *) apCurBlock[BLOCKSIZE];
                apCurBlock[BLOCKSIZE] = NULL;
            }
        }
    }
    while( celtFetched != 0 );

    // if we reached here, we got a whole load of pidls in an array ...
    // now walk the view checking to see if what we have are valid or not ...
    do
    {
        iIndex = ListView_GetNextItem( m_hWndListView, iIndex, LVNI_ALL );
        if ( iIndex == -1 )
        {
            break;
        }

        LV_ITEMW rgItem;
        rgItem.iItem = iIndex;
        rgItem.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_NORECOMPUTE;
        rgItem.iSubItem = 0;

        int iItem = ListView_GetItemWrapW( m_hWndListView, &rgItem );
        // just incase. ...
        if ( iItem != -1 )
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST) rgItem.lParam;

            // must walk our data chain to find the pidl ...
            BOOL fFound = FALSE;
            LPITEMIDLIST * apSearch = apElems;
            while ( apSearch != NULL )
            {
                int iMaxSearch = BLOCKSIZE;

                // the last block may not be full
                if ( apSearch[BLOCKSIZE] == NULL )
                {
                    iMaxSearch = celtThisBlock;
                }

                int iSearch;
                for ( iSearch = 0; iSearch < iMaxSearch; iSearch ++ )
                {
                    // skip used pidls.
                    if ( apSearch[iSearch] == NULL )
                    {
                        continue;
                    }

                    // try the legal method...
                    if ( this->m_pFolder->CompareIDs( 0, pidl, apSearch[iSearch] ) == 0 )
                    {
                        fFound = TRUE;
                        break;
                    }
                }
                if ( fFound )
                {
                    // now we need to check if the image is out of date...
                    if ( rgItem.iImage != I_IMAGECALLBACK )
                    {
                        IExtractImage *pExtract;
                        IExtractImage2 *pExtract2;
                        
                        UINT rgfFlags = 0;
                        FILETIME ftDateStamp;
                        
                        hRes = m_pFolder->GetUIObjectOf( m_hWnd, 1, (LPCITEMIDLIST *) &pidl,
                                                         IID_IExtractImage,
                                                         &rgfFlags,
                                                         (void **) &pExtract );
                        if ( SUCCEEDED( hRes ))
                        {
                            hRes = pExtract->QueryInterface( IID_IExtractImage, (void **) & pExtract2 );
                            pExtract->Release();
                        }

                        if ( SUCCEEDED( hRes ))
                        {
                            hRes = pExtract2->GetDateStamp( &ftDateStamp );
                            pExtract2->Release();
                        }
                        if ( SUCCEEDED( hRes ))
                        {
                            // now fetch the datestamp from the icon cache so we can see if it has changed...
                            IMAGECACHEINFO rgInfo;
                            rgInfo.cbSize = sizeof( rgInfo );
                            rgInfo.dwMask = ICIFLAG_DATESTAMP;

                            hRes = m_pImageCache->GetImageInfo( rgItem.iImage, & rgInfo );
                            if ( SUCCEEDED( hRes ))
                            {
                                if ( rgInfo.ftDateStamp.dwLowDateTime != ftDateStamp.dwLowDateTime ||
                                     rgInfo.ftDateStamp.dwHighDateTime != ftDateStamp.dwHighDateTime )
                                {
                                    // Delete The image so that it will have to be re-fetched....
                                    m_pImageCache->DeleteImage( rgItem.iImage );
                                    
                                    rgItem.iImage = I_IMAGECALLBACK;
                                    rgItem.mask = LVIF_IMAGE;
                                    ListView_SetItemWrapW( m_hWndListView, &rgItem );
                                }
                            }
                        }
                    }
                    
                    // free the pidl as don't need it
                    SHFree( (LPVOID) apSearch[iSearch] );

                    // mark it as NULL
                    apSearch[iSearch] = NULL;

                    break;
                }
                // next block of pidls...
                apSearch = (LPITEMIDLIST *) apSearch[BLOCKSIZE];
            }

            if ( !fFound )
            {
                // remove thumbnail from the cache.
                if ( rgItem.iImage != I_IMAGECALLBACK )
                {
                    // now remove it from the thumbnail cache.
                    m_pImageCache->FreeImage( (UINT) rgItem.iImage );
                }
                ListView_DeleteItem( m_hWndListView, iIndex );

                // move back an item in the listview ...
                iIndex --;

                // free the pidl...
                SHFree( (LPVOID) pidl );
            }
        }
    }
    while ( TRUE );

    // now search the blocks for pidls we don't have and add them ....
    apCurBlock = apElems;
    while ( apCurBlock != NULL )
    {
        int iMaxIndex = BLOCKSIZE;

        // the last block may not be full
        if ( apCurBlock[BLOCKSIZE] == NULL )
        {
            iMaxIndex = celtThisBlock;
        }

        for ( int iSearch = 0; iSearch < iMaxIndex; iSearch ++ )
        {
            // skip used pidls.
            if ( apCurBlock[iSearch] == NULL )
            {
                continue;
            }

            AddItem( apCurBlock[iSearch] );
            // the view now owns the pidl...
            apCurBlock[iSearch] = NULL;
        }

        // next block of pidls...
        LPITEMIDLIST * apNext = (LPITEMIDLIST *) apCurBlock[BLOCKSIZE];
        delete [] apCurBlock;
        apCurBlock = apNext;
    }

    apElems = NULL;

UpdateCleanup:

    // walk the list freeing and releasing memory.....
    if ( apElems != NULL )
    {
        do
        {
            apCurBlock = apElems;

            int iMaxIndex = BLOCKSIZE;
            if ( apCurBlock[BLOCKSIZE] == NULL )
            {
                iMaxIndex = celtThisBlock;
            }

            for ( int iElem = 0; iElem < iMaxIndex; iElem ++ )
            {
                if ( apCurBlock[iElem] != NULL )
                {
                    SHFree( apCurBlock[iElem] );
                }
            }
            apElems = (LPITEMIDLIST *) apElems[BLOCKSIZE];
            delete [] apCurBlock;
        }
        while ( apElems != NULL );
    }

    if ( fRefresh == TRUE )
    {
        // a last resort, refresh...
        Refresh();
    }

    if ( pEnum ) pEnum->Release( );
 }


////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::ExtractItem( UINT * puIndex, int iItem, LPCITEMIDLIST pidl, BOOL fBackground, BOOL fForce )
{
    // this will do an extract and put it in the queue if possible

    if( iItem == -1 && ! pidl )
    {
        // failure....
        return FALSE;
    }

    if ( iItem == -1 )
    {
        iItem = FindItem( pidl );
        if ( iItem == -1 )
        {
            return FALSE;
        }
    }

    UINT uIndex = 0;
    if ( !puIndex )
    {
        puIndex = &uIndex;
    }

    if ( !pidl )
    {
        LV_ITEMW rgItem;
        ZeroMemory( &rgItem, sizeof( rgItem ));
        rgItem.mask = LVIF_PARAM | LVIF_NORECOMPUTE;
        rgItem.iItem = iItem;

        if ( ListView_GetItemWrapW( m_hWndListView, & rgItem ) == -1 )
        {
            return FALSE;
        }

        pidl = (LPCITEMIDLIST) rgItem.lParam;
    }

    DWORD dwMask = GetOverlayMask( pidl );

    // now create a task
    IRunnableTask *pTask = NULL;
    IExtractImage2 *pExtract = NULL;
    WCHAR szPath[MAX_PATH];
    HRESULT hr = NOERROR;
    UINT rgfFlags = 0;

    // try for a thumbnail handler ...
    hr = m_pFolder->GetUIObjectOf( m_hWnd,
                                   1,
                                   &pidl,
                                   IID_IExtractImage,
                                   &rgfFlags,
                                   (void **) &pExtract );
    if ( SUCCEEDED( hr ))
    {
        Assert( pExtract != NULL );
        DWORD dwFlags = IEIFLAG_ASYNC | IEIFLAG_ORIGSIZE;
        DWORD dwPriority = PRIORITY_NORMAL;
        SIZE rgThumbSize = {m_iXSizeThumbnail, m_iYSizeThumbnail};
        FILETIME ftImageTimeStamp;
        BOOL fNoDateStamp = TRUE;
        IExtractImage2 *pExtract2;

        // od they support date stamps....
        if ( SUCCEEDED( pExtract->QueryInterface( IID_IExtractImage2, (void **) &pExtract2 )))
        {
            if ( FAILED( pExtract2->GetDateStamp( & ftImageTimeStamp )))
            {
                ZeroMemory( &ftImageTimeStamp, sizeof( ftImageTimeStamp ));
            }
            else
            {
                // Houston, we have a date stamp..
                fNoDateStamp = FALSE;
            }
                
            pExtract2->Release();
        }
        
        if ( m_fOffline )
        {
            dwFlags |= IEIFLAG_OFFLINE;
        }

        // always extract at 24 bit incase we have to cache it ...
        hr = pExtract->GetLocation( szPath, ARRAYSIZE(szPath), &dwPriority, &rgThumbSize, 24, &dwFlags );
        if ( SUCCEEDED( hr ) || hr == E_PENDING )
        {
            BOOL fAsync = (hr == E_PENDING );
            hr = E_FAIL;

            if ( !fForce )
            {
                // check if the image is already in the image cache.
                IMAGECACHEINFO rgInfo;
                rgInfo.cbSize = sizeof( rgInfo );
                rgInfo.dwMask = ICIFLAG_NAME | ICIFLAG_FLAGS;
                rgInfo.pszName = szPath;
                rgInfo.dwFlags = dwMask;

                if ( !fNoDateStamp )
                {
                    rgInfo.dwMask |= ICIFLAG_DATESTAMP;
                    rgInfo.ftDateStamp = ftImageTimeStamp;
                }
                
                hr = m_pImageCache->FindImage( &rgInfo, puIndex);
           }

           if ( hr != S_OK )
           {
                // get the full path for the extraction...
                WCHAR szFullPath[MAX_PATH];
                STRRET rgStr;
                
                hr = m_pFolder->GetDisplayNameOf( pidl, SHGDN_FORPARSING, &rgStr );
                if ( SUCCEEDED( hr ))
                {
                    StrRetToBufW( &rgStr, pidl, szFullPath, ARRAYSIZE(szFullPath) );
                }
                else
                {
                    szFullPath[0] = 0;
                }
                
                hr = CTestCacheTask_Create( this, pExtract, szPath, szFullPath,
                                            (fNoDateStamp ? NULL : &ftImageTimeStamp),
                                            pidl, iItem, dwFlags, 
                                            dwPriority, fAsync, fBackground, fForce, &pTask );
                if ( SUCCEEDED( hr ))
                {
                    // does it not support Async, or were we told to run it forground ?
                    if ( !fAsync || !fBackground )
                    {
                        if ( !fBackground )
                        {
                            // make sure there is no extract task already underway as we
                            // are not adding this to the queue...
                            m_pScheduler->RemoveTasks( TOID_ExtractImageTask, (DWORD)iItem, TRUE );
                        }

                        hr = pTask->Run();
                    }
                    else
                    {
                        // add the task to the scheduler...
                        hr = m_pScheduler->AddTask( pTask,
                                                    TOID_CheckCacheTask,
                                                   (DWORD)iItem,
                                                    dwPriority );

                        // signify we want a default icon for now....
                        hr = S_FALSE;
                    }
                    pTask->Release();
                }
            }
        }
        pExtract->Release( );
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////
HRESULT CDiskCacheTask_Create( CThumbnailView * pView,
                               int iItem,
                               LPCITEMIDLIST pidl,
                               LPCWSTR pszCache,
                               LPCWSTR pszPath,
                               const FILETIME * pftDateStamp,
                               IRunnableTask ** ppTask )
{
    if ( !ppTask )
    {
        return E_INVALIDARG;
    }

    CDiskCacheTask *pTask = new CComObject<CDiskCacheTask>;
    if ( pTask == NULL )
    {
        return E_OUTOFMEMORY;
    }

    pTask->m_pidl = ILClone( pidl );
    if ( !(pTask->m_pidl))
    {
        delete pTask;
        return E_OUTOFMEMORY;
    }

    StrCpyNW( pTask->m_szPath, pszPath, ARRAYSIZE( pTask->m_szPath ));
    StrCpyNW( pTask->m_szCache, pszCache, ARRAYSIZE( pTask->m_szCache ));
    
    pTask->m_pView = pView;
    pView->InternalAddRef();

    pTask->m_iItem = iItem;

    pTask->m_fNoDateStamp = ( pftDateStamp == NULL );
    if ( pftDateStamp )
    {
        pTask->m_ftDateStamp = *pftDateStamp;
    }
    
    pTask->AddRef();

    *ppTask = (LPRUNNABLETASK) pTask;
    return NOERROR;
}


STDMETHODIMP CDiskCacheTask::Run ( )
{
    if ( m_lState == IRTIR_TASK_RUNNING )
    {
        return S_FALSE;
    }

    if ( m_lState == IRTIR_TASK_PENDING )
    {
        // it is about to die, so fail
        return E_FAIL;
    }

   LONG lRes = InterlockedExchange( & m_lState, IRTIR_TASK_RUNNING);
   if ( lRes == IRTIR_TASK_PENDING )
   {
       m_lState = IRTIR_TASK_FINISHED;
       return NOERROR;
   }


    // otherwise, run the task ....
    HBITMAP hBmp;
    DWORD dwLock;

    // at this point, we assume that it IS in the cache...
    HRESULT hr = m_pView->m_pDiskCache->Open( STGM_READ, &dwLock );
    if ( SUCCEEDED( hr ))
    {
        hr = m_pView->m_pDiskCache->GetEntry( m_szPath, STGM_READ, &hBmp );

        // set the tick count so we know when we last accessed the disk cache
        m_pView->m_dwCacheTickCount = GetTickCount();
        
        // release the lock, we don't need it...
        m_pView->m_pDiskCache->ReleaseLock( &dwLock );
    }

    if ( SUCCEEDED( hr ))
    {
        hr = m_pView->UpdateImageForItem( hBmp, m_iItem, m_pidl, m_szCache, m_szPath, 
                                          ( m_fNoDateStamp ? NULL : &m_ftDateStamp ), FALSE );
        DeleteObject( hBmp );
    }

    m_lState = IRTIR_TASK_FINISHED;
    return hr;
}

STDMETHODIMP CDiskCacheTask::Kill( BOOL fWait )
{
    return E_NOTIMPL;
}

STDMETHODIMP CDiskCacheTask::Suspend( void )
{
    // not supported....
    return E_NOTIMPL;
}

STDMETHODIMP CDiskCacheTask::Resume( void )
{
    // not supported....
    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CDiskCacheTask::IsRunning()
{
    return (ULONG) m_lState;
}

CDiskCacheTask::CDiskCacheTask()
{
    m_iItem = -1;
    m_lState = IRTIR_TASK_NOT_RUNNING;
    m_pidl = 0;
    m_pView = NULL;
}

CDiskCacheTask::~CDiskCacheTask()
{
    if ( m_pidl )
    {
        SHFree((LPVOID) m_pidl);
    }

    if ( m_pView )
    {
        m_pView->InternalRelease();
    }
}

/////////////////////////////////////////////////////////////////////////////////////
LRESULT CThumbnailView::OnInfoTipText(NMLVGETINFOTIPW *plvn)
{
    LRESULT lRes = FALSE;
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) plvn->lParam;

    plvn->pszText[0] = 0;

    if ( !pidl )
    {
        LV_ITEMW rgItem;
        ZeroMemory( &rgItem, sizeof( rgItem ));
        rgItem.iItem = plvn->iItem;
        rgItem.iSubItem = plvn->iSubItem;
        rgItem.mask = LVIF_PARAM;

        ListView_GetItemWrapW( m_hWndListView, &rgItem );
        pidl = (LPCITEMIDLIST) rgItem.lParam;
    }

    if (pidl)
    {
        IQueryInfo *pqi;

        if (SUCCEEDED(m_pFolder->GetUIObjectOf(NULL, 1, &pidl, IID_IQueryInfo, NULL, (void**)&pqi)))
        {
            WCHAR *pwszTip = NULL;
            pqi->GetInfoTip(0, &pwszTip);
            if (pwszTip)
            {
                StrCpyNW(plvn->pszText, pwszTip, plvn->cchTextMax);
                SHFree(pwszTip);
            }
            pqi->Release();
        }
    }

    return lRes;
}


///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::UpdateImageForItem( HBITMAP hImage,
                                            int iItem,
                                            LPCITEMIDLIST pidl,
                                            LPCWSTR pszCache,
                                            LPCWSTR pszFullPath,
                                            const FILETIME * pftDateStamp,
                                            BOOL fCache )
{
    if ( !pszCache )
    {
        return E_INVALIDARG;
    }

    DWORD dwLock;
    BOOL fLock = FALSE;
    if ( fCache )
    {
        Assert( m_pDiskCache );
        DWORD dwMode = 0;

        HRESULT hr = m_pDiskCache->Open( STGM_WRITE, &dwLock );
        if ( hr == STG_E_FILENOTFOUND )
        {
            hr = m_pDiskCache->Create( STGM_WRITE, &dwLock );
        }

        fLock = SUCCEEDED( hr );

        if ( SUCCEEDED( hr ))
        {
            hr = m_pDiskCache->AddEntry(pszFullPath, pftDateStamp, STGM_WRITE, hImage );

            // set the tick count so that when the timer goes off, we can know when we
            // last used it...
            m_dwCacheTickCount = GetTickCount();
            if ( fLock)
            {
                hr = m_pDiskCache->ReleaseLock( &dwLock );
            }   
        }
    }

    DWORD dwMask = GetOverlayMask( pidl );
    TaskUpdateItem( pidl,
                    iItem,
                    dwMask,
                    pszCache,
                    pftDateStamp,
                    0,
                    hImage );

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD GetCurrentColorFlags( UINT * puBytesPerPixel )
{
    DWORD dwFlags = 0;
    UINT uBytesPerPix = 1;
    int res = ( int )GetCurColorRes( );
    switch ( res )
    {
        case 16 :   dwFlags = ILC_COLOR16;
                    uBytesPerPix = 2;
                    break;
        case 24 :
        case 32 :   dwFlags = ILC_COLOR24;
                    uBytesPerPix = 3;
                    break;
        default :   dwFlags = ILC_COLOR8;
                    uBytesPerPix = 1;
    }
    if ( puBytesPerPixel )
    {
        *puBytesPerPixel = uBytesPerPix;
    }
    return dwFlags;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CalcCacheMaxSize( const SIZE * prgSize, UINT uBytesPerPix )
{
    MEMORYSTATUS    rgMemStat;
    UINT uMaxCache = 0;

    // calculate the maximum number of thumbnails in the cache.
    rgMemStat.dwLength = sizeof( MEMORYSTATUS );
    GlobalMemoryStatus( &rgMemStat );

    // the minimum in the cache is the number of thumbnails visible on the screen at once.
    HDC hdc = GetDC( NULL );
    int iWidth = GetDeviceCaps( hdc, HORZRES );
    int iHeight = GetDeviceCaps( hdc, VERTRES );
    ReleaseDC( NULL , hdc );

    // the minimum number of thumbnails in the cache, is set to the maximum amount
    // of thumbnails that can be diplayed by a single view at once.
    int     iRow = iWidth / ( prgSize->cx + DEFSIZE_BORDER );
    int     iCol = iHeight / ( prgSize->cy + DEFSIZE_VERTBDR );
    UINT    iMinThumbs = iRow * iCol;

    // set the thumbnail maximum by calculating the memory required for a single thumbnail.
    // then, divide half the available memory by the memory needed per thumbnail.
    int iMemReqThumb = prgSize->cx * prgSize->cy * uBytesPerPix;

    // the larger of the minimum cache required and the number calculated
    // from the available memory.
    UINT    uiThumbsPerMem = UINT( ( rgMemStat.dwAvailPhys / 3 ) / iMemReqThumb );
    uMaxCache = __max( uiThumbsPerMem, iMinThumbs + 1 );

#if defined(SMALLCACHE)
    // restrict the size of the thumbnail cache to force it to have to grovel
    // entries..
#pragma message("Warning: possible Image Cache limiting in effect")
    uMaxCache = 20;
#endif

    return uMaxCache;
}

void CThumbnailView::ThreadUpdateStatusBar( UINT idMsg, int idItem )
{
    PostMessageA( m_hWnd, WM_STATUSBARUPDATE, (WPARAM) idMsg, (LPARAM) idItem );
}

LRESULT CThumbnailView::OnCustomDraw( NMCUSTOMDRAW * pNM )
{
    NMLVCUSTOMDRAW * plvcd = (NMLVCUSTOMDRAW *)pNM;
    LRESULT lres = 0;
    static HPALETTE hpalOld = NULL;

    switch (plvcd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        if (m_fShowCompColor)
             lres |= CDRF_NOTIFYITEMDRAW;
        if ( m_hpal && plvcd->nmcd.hdc )
        {
            hpalOld = SelectPalette( plvcd->nmcd.hdc, m_hpal, TRUE );
            RealizePalette( plvcd->nmcd.hdc );
            lres |= CDRF_NOTIFYPOSTPAINT;
        }
        break;
    case CDDS_ITEMPREPAINT:
    {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST) plvcd->nmcd.lItemlParam;
        //
        // We hit a case (only in classic mode) in which pidl is bogus data.  This happenes because we had
        // alread called Clear and were in the process of calling ListView_DeleteAllItems when we were asked
        // to paint an item.  Dismissing the label edit box while the view was being emptied caused the item
        // to be repainted and we faulted doing a GetAttributesOf on the already freed pidl.  We protect
        // against this case by ensureing that m_hWndListView is non-null (since we null it out before calling
        // Clear).
        //
        if (pidl && m_hWndListView)
        {
            DWORD uFlags = SFGAO_COMPRESSED;
            HRESULT hres = m_pFolder->GetAttributesOf(1, &pidl, &uFlags);

            if (SUCCEEDED(hres) && (uFlags & SFGAO_COMPRESSED))
            {
                plvcd->clrText = GetAltColor();
            }
        }

        return CDRF_DODEFAULT;
    }
    case CDDS_POSTPAINT:
        if ( m_hpal && hpalOld && plvcd->nmcd.hdc )
        {
            SelectPalette( plvcd->nmcd.hdc, hpalOld, TRUE );
            RealizePalette( plvcd->nmcd.hdc );
        }
        break;
    }
    return lres;
}


///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CUpdateDirTask_Create( CThumbnailView * pView, 
                               IRunnableTask ** ppTask )
{
    CUpdateDirTask * pTask = new CComObject<CUpdateDirTask>;
    if ( pTask == NULL )
    {
        return E_OUTOFMEMORY;
    }

    pTask->m_pView = pView;
    Assert( pView );

    if ( !pTask->m_hEvent || !pTask->m_apElems )
    {
        delete pTask;
        return E_OUTOFMEMORY;
    }

    pTask->AddRef();
    
    *ppTask = (LPRUNNABLETASK) pTask;
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdateDirTask::Run ()
{
    if ( m_lState == IRTIR_TASK_RUNNING )
    {
        return S_FALSE;
    }

    if ( m_lState == IRTIR_TASK_PENDING )
    {
        // it is about to die, so fail
        return E_FAIL;
    }

   LONG lRes = InterlockedExchange( & m_lState, IRTIR_TASK_RUNNING);
   if ( lRes == IRTIR_TASK_PENDING )
   {
       m_lState = IRTIR_TASK_FINISHED;
       return NOERROR;
   }


    // otherwise, run the task ....
    HRESULT hr = InternalResume();
    if ( hr != E_PENDING )
        m_lState = IRTIR_TASK_FINISHED;
        
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdateDirTask::Suspend ( )
{
    if ( m_lState != IRTIR_TASK_RUNNING )
    {
        return E_FAIL;
    }

    // suspend ourselves
    LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_SUSPENDED);
    if ( lRes == IRTIR_TASK_FINISHED )
    {
        m_lState = lRes;
        return NOERROR;
    }

    // if it is running, then there is an Event Handle, if we have passed where
    // we are using it, then we are close to finish, so it will ignore the suspend
    // request
    Assert( m_hEvent );
    SetEvent( m_hEvent );
    
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdateDirTask::Resume( )
{
    if ( m_lState != IRTIR_TASK_SUSPENDED )
    {
        return E_FAIL;
    }
    
    ResetEvent( m_hEvent );
    m_lState = IRTIR_TASK_RUNNING;

    HRESULT hr = InternalResume();
    if ( hr != E_PENDING )
    {
        m_lState= IRTIR_TASK_FINISHED;
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CUpdateDirTask::Kill( BOOL fWait )
{
    if ( m_lState == IRTIR_TASK_RUNNING )
    {
        LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_PENDING );
        if ( lRes == IRTIR_TASK_FINISHED )
        {
            m_lState = lRes;
        }
        else if ( m_hEvent )
        {
            // signal the event it is likely to be waiting on
            SetEvent( m_hEvent );
        }
        
        return NOERROR;
    }
    else if ( m_lState == IRTIR_TASK_PENDING || m_lState == IRTIR_TASK_FINISHED )
    {
        return S_FALSE;
    }

    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CUpdateDirTask::IsRunning()
{
    return m_lState;
}

///////////////////////////////////////////////////////////////////////////////////////
CUpdateDirTask::CUpdateDirTask( )
{
    m_hEvent = CreateEventA( NULL, FALSE, FALSE, NULL );
    
    m_apElems = new LPITEMIDLIST[BLOCKSIZE + 1];
    Assert( !m_apElems || m_apElems[BLOCKSIZE] == NULL );

    m_iIndex = -1;
}

///////////////////////////////////////////////////////////////////////////////////////
CUpdateDirTask::~CUpdateDirTask()
{
    if ( m_hEvent )
    {
        CloseHandle( m_hEvent );
    }
    if ( m_apElems )
    {
        // empty the pidl array ...
        do
        {
            LPITEMIDLIST * apCurBlock = m_apElems;

            int iMaxIndex = BLOCKSIZE;
            if ( apCurBlock[BLOCKSIZE] == NULL )
            {
                iMaxIndex = m_celtThisBlock;
            }

            for ( int iElem = 0; iElem < iMaxIndex; iElem ++ )
            {
                if ( apCurBlock[iElem] != NULL )
                {
                    SHFree( apCurBlock[iElem] );
                }
            }
            m_apElems = (LPITEMIDLIST *) m_apElems[BLOCKSIZE];
            delete [] apCurBlock;
        }
        while ( m_apElems != NULL );
    }
    if ( m_pEnum)
    {
        m_pEnum->Release();
    }
    m_pView->m_fUpdateDir = FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
HRESULT CUpdateDirTask::InternalResume()
{
    m_pView->m_fUpdateDir = TRUE;
    
    // we have yet to do the enum.....
    if ( m_pEnum == NULL )
    {
        HRESULT hRes;
        ULONG ulType = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
        
        // the blocks are empty right now...
        Assert( m_apElems[0] == NULL );
        
        m_apCurBlock = m_apElems;
        if ( m_pView->m_fShowAllObjects )
        {
           ulType |= SHCONTF_INCLUDEHIDDEN;
        }

        hRes = m_pView->m_pFolder->EnumObjects( NULL, ulType, &m_pEnum);
        if ( hRes != S_OK )
        {
            PostMessageA( m_pView->m_hWnd, WM_VIEWREFRESH, 0, 0 );
            return E_FAIL;
        }

        // we are starting again, empty the update lists...
        m_pView->ThreadEmptyUpdateLists();
    }

    if ( m_pEnum && !m_fEnumDone )
    {
        // fetch all the new elements ...
        ULONG celtFetched;
        do
        {
            // we cannot assume that Next() can return more than one element at
            // a time, as most of the IShellFolder implementers of this only
            // support one at a time... (so much for relying on the spec....)
            celtFetched = 0;
            m_pEnum->Next( BLOCKSIZE - m_celtThisBlock, m_apCurBlock + m_celtThisBlock, &celtFetched );
            if ( celtFetched == 0 )
            {
                break;
            }

            // add this lot to the current block count
            m_celtThisBlock += celtFetched;

            if ( m_celtThisBlock == BLOCKSIZE )
            {
                // allocate next block
                m_apCurBlock[BLOCKSIZE] = (LPITEMIDLIST) new LPITEMIDLIST[BLOCKSIZE + 1];
                if ( m_apCurBlock[BLOCKSIZE] == NULL )
                {
                    // out of memory, resort to the F5 key :-)
                    PostMessageA( m_pView->m_hWnd, WM_VIEWREFRESH, 0, 0 );
                    return E_FAIL;
                }
                else
                {
                    m_celtThisBlock = 0;
                    m_apCurBlock = (LPITEMIDLIST *) m_apCurBlock[BLOCKSIZE];
                    m_apCurBlock[BLOCKSIZE] = NULL;
                }
            }
            if ( WaitForSingleObject( m_hEvent, 0 ) == WAIT_OBJECT_0 )
            {
                // why were we signalled ...
                if ( m_lState == IRTIR_TASK_SUSPENDED )
                {
                    return E_PENDING;
                }
                else
                {
                    return E_FAIL;
                }
            }
        }
        while( celtFetched != 0 );

        m_fEnumDone = TRUE;
    }
    if ( m_fEnumDone )
    {
        // if we reached here, we got a whole load of pidls in an array ...
        // now walk the view checking to see if what we have are valid or not ...
        do
        {
            m_iIndex = ListView_GetNextItem( m_pView->m_hWndListView, m_iIndex, LVNI_ALL );
            if ( m_iIndex == -1 )
            {
                break;
            }

            LV_ITEMW rgItem;
            rgItem.iItem = m_iIndex;
            rgItem.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_NORECOMPUTE;
            rgItem.iSubItem = 0;

            int iItem = ListView_GetItemWrapW( m_pView->m_hWndListView, &rgItem );
            // just incase. ...
            if ( iItem != -1 )
            {
                LPITEMIDLIST pidl = (LPITEMIDLIST) rgItem.lParam;

                // must walk our data chain to find the pidl ...
                BOOL fFound = FALSE;
                LPITEMIDLIST * apSearch = m_apElems;
                while ( apSearch != NULL )
                {
                    int iMaxSearch = BLOCKSIZE;

                    // the last block may not be full
                    if ( apSearch[BLOCKSIZE] == NULL )
                    {
                        iMaxSearch = m_celtThisBlock;
                    }

                    int iSearch;
                    for ( iSearch = 0; iSearch < iMaxSearch; iSearch ++ )
                    {
                        // skip used pidls.
                        if ( apSearch[iSearch] == NULL )
                        {
                            continue;
                        }

                        // try the legal method...
                        if ( m_pView->m_pFolder->CompareIDs( 0, pidl, apSearch[iSearch] ) == 0 )
                        {
                            fFound = TRUE;
                            break;
                        }
                    }
                    if ( fFound )
                    {
                        rgItem.mask = 0;
                        
                        // now we need to check if the image is out of date...
                        if ( rgItem.iImage != I_IMAGECALLBACK )
                        {
                            IExtractImage *pExtract;
                            IExtractImage2 *pExtract2;
                            
                            UINT rgfFlags = 0;
                            FILETIME ftDateStamp;
                            
                            HRESULT hRes = m_pView->m_pFolder->GetUIObjectOf( m_pView->m_hWnd, 1, (LPCITEMIDLIST *) &pidl,
                                                                     IID_IExtractImage,
                                                                     &rgfFlags,
                                                                     (void **) &pExtract );
                            if ( SUCCEEDED( hRes ))
                            {
                                hRes = pExtract->QueryInterface( IID_IExtractImage2, (void **) & pExtract2 );
                                pExtract->Release();
                            }

                            if ( SUCCEEDED( hRes ))
                            {
                                hRes = pExtract2->GetDateStamp( &ftDateStamp );
                                pExtract2->Release();
                            }
                            if ( SUCCEEDED( hRes ))
                            {
                                // now fetch the datestamp from the icon cache so we can see if it has changed...
                                IMAGECACHEINFO rgInfo;
                                rgInfo.cbSize = sizeof( rgInfo );
                                rgInfo.dwMask = ICIFLAG_DATESTAMP;

                                hRes = m_pView->m_pImageCache->GetImageInfo( rgItem.iImage, & rgInfo );
                                if ( SUCCEEDED( hRes ))
                                {
                                    if ( rgInfo.ftDateStamp.dwLowDateTime != ftDateStamp.dwLowDateTime ||
                                         rgInfo.ftDateStamp.dwHighDateTime != ftDateStamp.dwHighDateTime )
                                    {
                                        m_pView->m_pImageCache->FreeImage( rgItem.iImage );

                                        rgItem.mask |= LVIF_IMAGE;
                                        rgItem.iImage = I_IMAGECALLBACK;
                                    }
                                }
                            }
                        }

                        int iLen1 = GetPidlLength(apSearch[iSearch]);
                        int iLen2 = GetPidlLength(pidl);

                        // has the pidl changed ?
                        if ( iLen1 != iLen2 || memcmp( apSearch[iSearch], pidl, iLen1 ) != 0)
                        {
                            // swap the pidls....
                            rgItem.mask |= LVIF_PARAM;
                           rgItem.lParam = (LPARAM) apSearch[iSearch];
                           
                            // put the old pidl in the array so it gets freed
                            apSearch[iSearch] = pidl;
                        }

                        if ( rgItem.mask != 0 )
                        {
                            ListView_SetItemWrapW( m_pView->m_hWndListView, &rgItem );
                        }

                        // free the pidl as don't need it
                        SHFree( (LPVOID) apSearch[iSearch] );

                        // mark it as NULL
                        apSearch[iSearch] = NULL;

                        break;
                    }
                    // next block of pidls...
                    apSearch = (LPITEMIDLIST *) apSearch[BLOCKSIZE];
                }

                if ( !fFound )
                {
                    m_pView->ThreadDeleteItem( ILClone( (LPITEMIDLIST) rgItem.lParam ));
                }
            }

            // check to see if we are told to give up....
            if ( WaitForSingleObject( m_hEvent, 0 ) == WAIT_OBJECT_0 )
            {
                // why were we signalled ...
                if ( m_lState == IRTIR_TASK_SUSPENDED )
                {
                    // give the view a chance to update
                    m_pView->m_fUpdateDir = FALSE;
                    PostMessageA( m_pView->m_hWnd, WM_PROCESSITEMS, 0, 0 );
   
                    return E_PENDING;
                }
                else
                {
                    return E_FAIL;
                }
            }

        }
        while ( TRUE );

        m_fViewPassDone = TRUE;
    }
    
    if ( m_fViewPassDone )
    {
        LPITEMIDLIST * apCurBlock = m_apElems;
        // now search the blocks for pidls we don't have and add them ....
        while ( apCurBlock != NULL )
        {
            int iMaxIndex = BLOCKSIZE;

            // the last block may not be full
            if ( apCurBlock[BLOCKSIZE] == NULL )
            {
                iMaxIndex = m_celtThisBlock;
            }

            for ( int iSearch = 0; iSearch < iMaxIndex; iSearch ++ )
            {
                // skip used pidls.
                if ( apCurBlock[iSearch] == NULL )
                {
                    continue;
                }

                m_pView->ThreadAddItem( apCurBlock[iSearch] );
                
                // the view now owns the pidl...
                apCurBlock[iSearch] = NULL;
            }

            // next block of pidls...
            LPITEMIDLIST * apNext = (LPITEMIDLIST *) apCurBlock[BLOCKSIZE];
            delete [] apCurBlock;
            apCurBlock = apNext;
        }

        m_apElems = NULL;

        m_pView->m_fUpdateDir = FALSE;

        PostMessageA( m_pView->m_hWnd, WM_PROCESSITEMS, 0, 0 );
    }

    return NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::ThreadAddItem( LPCITEMIDLIST pidl )
{
    if ( !pidl )
        return E_INVALIDARG;

    EnterCriticalSection( &m_csAddLock );
    DPA_AppendPtr( m_hAddList, (LPVOID) pidl );
    LeaveCriticalSection( &m_csAddLock );
    
    return NOERROR;
}

HRESULT CThumbnailView::ThreadDeleteItem( LPCITEMIDLIST pidl )
{
    if ( !pidl )
        return E_INVALIDARG;
        
    EnterCriticalSection( &m_csAddLock );
    DPA_AppendPtr( m_hDeleteList, (LPVOID) pidl );
    LeaveCriticalSection( &m_csAddLock );

    return NOERROR;
}

HRESULT CThumbnailView::OnWmProcessItems( )
{
    EnterCriticalSection( &m_csAddLock );

    // do the add items first ....
    for ( int iLoop = DPA_GetPtrCount( m_hAddList ) - 1; iLoop >= 0; iLoop -- )
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST) DPA_GetPtr( m_hAddList, iLoop );
        if (this->AddItem( pidl ) < 0)
        {
            SHFree(pidl);   // failed to add it to the view, destroy it ...
        }
        else if ( m_pidlRename )
        {
            if ( m_pFolder->CompareIDs( 0, pidl, m_pidlRename ) == 0 )
            {
                this->SelectAndPositionItem( pidl, SVSI_EDIT, NULL );
                SHFree(m_pidlRename );
                m_pidlRename = NULL;
            }
        }
        DPA_DeletePtr( m_hAddList, iLoop );
    }

    if ( !m_fUpdateDir )
    {
        // now do the delete items...
        for ( iLoop = DPA_GetPtrCount( m_hDeleteList ) - 1; iLoop >= 0; iLoop -- )
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST) DPA_GetPtr( m_hDeleteList, iLoop );
            this->RemoveObject( pidl, NULL );
            
            // we failed to add it to the view, destroy it ...
            SHFree(pidl);
            DPA_DeletePtr(m_hDeleteList, iLoop);
        }
    }
    else
    {
        // we are currently doing an update dir, skip deleting things for a moment otherwise we'll screw up teh list order.
        PostMessageA( m_hWnd, WM_PROCESSITEMS, 0, 0 );
    }

    // if we still think we need a rename, cleanit up...
    if (m_pidlRename)
    {
        SHFree(m_pidlRename);
        m_pidlRename = NULL;
    }
    LeaveCriticalSection( &m_csAddLock );

    return NOERROR;
}
       

HRESULT CThumbnailView::ThreadEmptyUpdateLists( )
{
	EnterCriticalSection( &m_csAddLock );

    for ( int iLoop = DPA_GetPtrCount( m_hAddList ) - 1; iLoop >= 0; iLoop -- )
    {
        LPCITEMIDLIST pidl = (LPCITEMIDLIST) DPA_GetPtr( m_hAddList, iLoop );
        SHFree( (LPITEMIDLIST) pidl );
        DPA_DeletePtr( m_hAddList, iLoop );
    }

    // now do the delete items...
    for ( iLoop = DPA_GetPtrCount( m_hDeleteList ) - 1; iLoop >= 0; iLoop -- )
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST) DPA_GetPtr( m_hAddList, iLoop );
        SHFree( (LPITEMIDLIST) pidl );
        DPA_DeletePtr( m_hAddList, iLoop );
    }
    
    LeaveCriticalSection( &m_csAddLock );

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CTestCacheTask_Create( CThumbnailView * pView,
                               IExtractImage * pExtract,
                               LPCWSTR pszCache,
                               LPCWSTR pszPath,
                               const FILETIME * pftDateStamp,
                               LPCITEMIDLIST pidl,
                               int iItem,
                               DWORD dwFlags,
                               DWORD dwPriority,
                               BOOL fAsync,
                               BOOL fBackground,
                               BOOL fForce,
                               IRunnableTask ** ppTask )
{
    *ppTask = NULL;
    
    CTestCacheTask * pNew = new CTestCacheTask;
    if ( !pNew )
        return E_OUTOFMEMORY;

    pNew->m_pidl = ILClone( pidl );

    if ( pNew->m_pidl == NULL )
    {
        delete pNew;
        return E_OUTOFMEMORY;
    }
    
    StrCpyNW( pNew->m_szFullPath, pszPath, ARRAYSIZE( pNew->m_szFullPath ));
    StrCpyNW( pNew->m_szCache, pszCache, ARRAYSIZE( pNew->m_szCache ));

    if ( pftDateStamp )
    {
        pNew->m_fDateStamp = TRUE;
        pNew->m_ftDateStamp = *pftDateStamp;
    }

    pNew->m_iItem = iItem;
    pNew->m_dwFlags = dwFlags;
    pNew->m_dwPriority = dwPriority;
    pNew->m_fAsync = fAsync;
    pNew->m_fBackground = fBackground;
    pNew->m_fForce = fForce;

    pNew->m_pExtract = pExtract;
    pExtract->AddRef();

    pNew->m_pView = pView;
    pView->InternalAddRef();
    
    
    *ppTask = (IRunnableTask *) pNew;

    return NOERROR;
}                               

STDMETHODIMP CTestCacheTask::RunInitRT()
{
    DWORD dwLock = 0;
    BOOL fLock = FALSE;
    HRESULT hr = E_FAIL;
    if ( !m_fForce )
    {
        if ( m_pView->m_pDiskCache )
        {
            // make sure the disk cache is open for reading.
            hr = m_pView->m_pDiskCache->Open( STGM_READ, &dwLock );
            fLock = SUCCEEDED( hr );
        }
        else
        {
            // no disk cache ...
            hr = E_FAIL;
        }

        if ( SUCCEEDED( hr ))
        {
            if ( !m_pView->m_fTimerActive )
            {
                // start the timer, once every two seconds....
                SetTimer( m_pView->m_hWnd, TIMER_DISKCACHE, 2000, NULL );
                m_pView->m_dwCacheTickCount = GetTickCount();
                m_pView->m_fTimerActive = TRUE;
            }

            // is it in the cache....
            FILETIME ftCacheTimeStamp;
            hr = m_pView->m_pDiskCache->IsEntryInStore( m_szFullPath, &ftCacheTimeStamp );

            // if it is in the cache, and it is an uptodate image, then fetch from disk....
            // if the timestamps are wrong, then the extract code further down will then try
            // and write its image back to the cache to update it anyway.....
            if ( hr == S_OK && 
                 (( ftCacheTimeStamp.dwLowDateTime == m_ftDateStamp.dwLowDateTime && 
                    ftCacheTimeStamp.dwHighDateTime == m_ftDateStamp.dwHighDateTime ) || !m_fDateStamp))
            {
                // try it in the background...
                IRunnableTask *pTask = NULL;
                hr = CDiskCacheTask_Create( m_pView, m_iItem, m_pidl, m_szCache, m_szFullPath, 
                                            ( !m_fDateStamp ? NULL : &m_ftDateStamp ), &pTask );
                if ( SUCCEEDED( hr ) && !m_pView->m_fDestroying )
                {
                    Assert( m_pView->m_pScheduler );
                    
                    // add the task to the scheduler...
                    hr = m_pView->m_pScheduler->AddTask( pTask,
                                                         TOID_DiskCacheTask,
                                                         (DWORD)m_iItem,
                                                         PRIORITY_NORMAL );
                    if ( SUCCEEDED( hr ))
                        hr = S_FALSE;

                    pTask->Release();
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
        if ( fLock )
        {
            // free the disk cache lock...
            m_pView->m_pDiskCache->ReleaseLock( &dwLock );
        }
    }
    if ( FAILED( hr ))
    {
        // Extract It....

        IRunnableTask *pTask = NULL;
        hr = CExtractImageTask_Create( m_pView,
                                       m_pExtract,
                                       m_szCache,
                                       m_szFullPath, 
                                       m_pidl,
                                       ( !m_fDateStamp ? NULL : &m_ftDateStamp ),
                                       m_iItem,
                                       m_dwFlags,
                                       &pTask );
        if ( SUCCEEDED( hr ))
        {
            // does it not support Async, or were we told to run it forground ?
            if ( !m_fAsync || !m_fBackground )
            {
                if ( !m_fBackground )
                {
                    // make sure there is no extract task already underway as we
                    // are not adding this to the queue...
                    m_pView->m_pScheduler->RemoveTasks( TOID_ExtractImageTask, (DWORD)m_iItem, TRUE );
                }

                hr = pTask->Run();
            }
            else
            {
                // add the task to the scheduler...
                hr = m_pView->m_pScheduler->AddTask( pTask,
                                                     TOID_ExtractImageTask,
                                                     (DWORD)m_iItem,
                                                     m_dwPriority );

                // signify we want a default icon for now....
                hr = S_FALSE;
            }
            pTask->Release();
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

CTestCacheTask::CTestCacheTask()
    : CRunnableTask(RTF_DEFAULT)
{
}

CTestCacheTask::~CTestCacheTask()
{
    if ( m_pidl )
    {
        SHFree(( LPVOID) m_pidl );
    }

    if ( m_pExtract )
    {
        m_pExtract->Release();
    }

    if ( m_pView )
    {
        m_pView->InternalRelease();
    }
}

HRESULT CThumbnailView::SetPointData( IDataObject * ptdObj, LPCITEMIDLIST * ppPidl, int cidl )
{
    static UINT s_cfOffsets = 0;

    ITEMSPACING rgSpacing;
    
    // scaling is basically converted to large icon spacing...
    HRESULT hres = GetItemSpacing( & rgSpacing );
    Assert ( SUCCEEDED( hres ));

    // the offsets format
    if (!s_cfOffsets)
    {
        s_cfOffsets = RegisterClipboardFormat( CFSTR_SHELLIDLISTOFFSET);
    }

    POINT *ppt = (POINT *)GlobalAlloc(GPTR, SIZEOF(POINT) * (1 + cidl));
    if (ppt)
    {
        int i;

        // Grab the anchor point
        Assert ( m_fDragStarted );
        ppt[0] = m_ptDragStart;

        for (i = 1; i <= cidl; i++)
        {
            int iItem = FindInView( m_hWndListView, m_pFolder, ppPidl[i-1] );

            Assert( iItem != -1 );
            
            ListView_GetItemPosition( m_hWndListView, iItem, &ppt[i] );
            ppt[i].x -= m_ptDragStart.x;
            ppt[i].y -= m_ptDragStart.y;
            ppt[i].x = (ppt[i].x * rgSpacing.cxLarge) / rgSpacing.cxSmall;
            ppt[i].y = (ppt[i].y * rgSpacing.cyLarge) / rgSpacing.cySmall;
        }

        FORMATETC fmte = {(CLIPFORMAT) s_cfOffsets, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium;

        medium.tymed = TYMED_HGLOBAL;
        medium.hGlobal = ppt;
        medium.pUnkForRelease = NULL;

        // give the data object ownership of ths
        hres = ptdObj->SetData(&fmte, &medium, TRUE);

        if (FAILED(hres))
            GlobalFree((HGLOBAL)ppt);
    }
    return hres;
}

HRESULT CThumbnailView::AutoAutoArrange( DWORD dwReserved)
{
    if (!m_fItemsMoved )
    {
        ListView_Arrange(m_hWndListView, LVA_DEFAULT);
    }
    return NOERROR;
}

HRESULT CThumbnailView::SetStatusText(LPCWSTR pwszStatusText)
{
    UpdateStatusBar();
    return NOERROR;
}

// CLR_NONE is a special value that never matches a valid RGB
COLORREF g_crAltColor = CLR_NONE;   // uninitialized magic value

DWORD GetAltColor()
{
    // Fetch the alternate color (for compression) if supplied.
    if (g_crAltColor == CLR_NONE)   // initialized yet?
    {
        DWORD cbData = sizeof(COLORREF);
        DWORD dwType;
        HKEY hkey;
        LONG lRes;

        lRes = RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &hkey);
        if( lRes == ERROR_SUCCESS && hkey ) 
        {
            if (SHQueryValueEx(hkey,  TEXT("AltColor"), NULL, &dwType, (LPBYTE)&g_crAltColor, &cbData) != ERROR_SUCCESS)
                g_crAltColor = RGB(0, 0, 255);  // default value
        RegCloseKey(hkey);
        }
    }
    return g_crAltColor;
}
