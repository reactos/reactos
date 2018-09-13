/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * This file implements a tear off Drag-Drop interface for Listview windows
 */

#include "precomp.h"
#include "shellp.h"
#define DECLARE_DEBUG
#include "debug.h"

// default set of drag drop types
#define DEFAULT_ATTRIBUTES  (DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY | SFGAO_CANDELETE | SFGAO_CANRENAME | SFGAO_HASPROPSHEET)

// the flags that can be returned from the DragDirection() method
#define SCROLL_NONE     0x0000
#define SCROLL_LEFT     0x0001
#define SCROLL_RIGHT    0x0002
#define SCROLL_UP       0x0004
#define SCROLL_DOWN     0x0008


// system settings tags
CHAR const c_szWindows[] = "Windows";
CHAR const c_szDragDelay[] = "DragScrollDelay";
CHAR const c_szDragInset[] = "DragScrollInset";
CHAR const c_szRegPathCustomPrefs[] =  "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CustomPrefs";
CHAR const c_szDropGrid[] = "DropGrid";

//////////////////////////////////////////////////////////////////////////////////////////////
CViewDropTarget::CViewDropTarget()
{
    m_pBkgrndDT = NULL;
    m_pCurDT = NULL;
    m_pDataObj = NULL;
    m_pFolder = NULL;
    m_fDropOnBack = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////
CViewDropTarget::~CViewDropTarget( )
{
    if ( m_pBkgrndDT != NULL )
    {
        m_pBkgrndDT->DragLeave();
        m_pBkgrndDT->Release();
    }

    if ( m_pCurDT != NULL )
    {
        m_pCurDT->DragLeave();
        m_pCurDT->Release();
    }

    if ( m_pDataObj != NULL )
    {
        m_pDataObj->Release();
    }

    if ( m_pFolder != NULL )
    {
        m_pFolder->Release();
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CViewDropTarget::Init( CDropTargetClient * pParent,
                               LPSHELLFOLDER pFolder,
                               HWND hwnd )
{
    /*[TODO: we need to ref count this ... ]*/
    m_pParent = pParent;
    Assert( pParent != NULL );

    m_iCurItem = -1;
    m_pCurDT = NULL;
    m_pDataObj = NULL;
    m_iFlags = 0;

    // used to remember which way we were scrolling last if the user held the mouse
    // over the official, yes official drag drop scrolling inset region...
    m_dwScrollFlags = SCROLL_NONE;
    m_dwDragDropScrollDelay = 0;
    
    // ask the system what the current default scroll delay time is ...
    m_dwDragDropDelay = GetProfileIntA( c_szWindows, 
                                        c_szDragDelay, 
                                        DD_DEFSCROLLDELAY );
    m_dwDragDropInset = GetProfileIntA( c_szWindows,
                                        c_szDragInset,
                                        DD_DEFSCROLLINSET );

    m_grfKeyState = 0;

    m_hWnd = hwnd;

    Assert( pFolder != NULL );
    m_pFolder = pFolder;
    pFolder->AddRef();

    // This is allowed to fail in Non-NT5 cases.
    CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (void**)&m_pDragImages);

    m_pBkgrndDT = NULL;
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////////////////
void CViewDropTarget::GetItemUnder( LV_HITTESTINFO * prgInfo )
{
    BOOL bRes = ScreenToClient( m_hWndListView, &(prgInfo->pt ) );

    // do a hit test on the list view ...
    prgInfo->iItem = -1;
    prgInfo->flags = 0;
    
    ListView_HitTest( m_hWndListView, prgInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////
void CViewDropTarget::FocusItem( int iItem, BOOL fFocus )
{
    int iState = ( fFocus != FALSE ) ? LVIS_DROPHILITED : 0;
    ListView_SetItemState( m_hWndListView, iItem, iState, LVIS_DROPHILITED ); 
}

//////////////////////////////////////////////////////////////////////////////////////////
void CViewDropTarget::CreateDTForItemUnder( LV_HITTESTINFO * prgInfo )
{
    if ( prgInfo->iItem == m_iCurItem )
    {
        return;
    }

    // we are over a valid item
    Assert( prgInfo->iItem != -1 );

    if ( m_pCurDT != NULL )
    {
        // are we over the item we have cached ?
        if ( m_iCurItem == prgInfo->iItem )
        {
            return;
        }
        else
        {
            m_pCurDT->DragLeave();
            m_pCurDT->Release();
            m_pCurDT = NULL;
            m_iCurItem = -1;
        }
    }

    // get the item's pidl ...
    // (and its state.)
    LV_ITEMW rgItem;
    memset( &rgItem, 0, sizeof( rgItem ));

    Assert( prgInfo->iItem != -1 );
    rgItem.iItem = prgInfo->iItem;
    rgItem.mask = LVIF_PARAM | LVIF_STATE;
    rgItem.stateMask = LVIS_SELECTED;
    ListView_GetItemWrapW( m_hWndListView, &rgItem);

    // is the item selected and did we start the drag here ? If so, then
    // treat it as if it were a background drop...
    if (( rgItem.state & LVIS_SELECTED ) && m_pParent->WasDragStartedHere())
    {
        return;
    }
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) rgItem.lParam;
    if ( pidl == NULL )
    {
        return;
    }
    
    // ask if a drop target is supported...
    ULONG rgfFlags = SFGAO_DROPTARGET;
    HRESULT hRes = m_pFolder->GetAttributesOf( 1, &pidl, &rgfFlags );
    if ( FAILED( hRes ) || !(rgfFlags & SFGAO_DROPTARGET ))
    {
        return;
    }
    
    Assert( pidl != NULL );
    hRes = m_pFolder->GetUIObjectOf( m_hWnd, 1, (LPCITEMIDLIST * )&pidl,
        IID_IDropTarget, NULL, (void **) & m_pCurDT );
    m_iCurItem = prgInfo->iItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CViewDropTarget::DragEnter ( IDataObject *pDataObj,
                                          DWORD grfKeyState,
                                          POINTL ptl,
                                          DWORD *pdwEffect )
{
    m_hWndListView = m_pParent->GetWindow();
    HWND hwndT;

    // get the background DT
    if ( m_pBkgrndDT == NULL )
    {
        m_pFolder->CreateViewObject( m_hWnd, IID_IDropTarget, (void **) & m_pBkgrndDT );
    }

    m_hwndDD = m_hWndListView;
    while (hwndT = GetParent(m_hwndDD))
        m_hwndDD = hwndT;
    Assert(pDataObj);
    if (m_pDragImages)
        m_pDragImages->DragEnter(m_hwndDD, pDataObj, (POINT*)&ptl, *pdwEffect);
    else
        _DragEnter(m_hwndDD, ptl, pDataObj);
    // cache the current listview window (to save having to call this function
    // everytime we need it ...
    m_fDropOnBack = FALSE;
    
    BOOL fScrolling = FALSE;

    // the window handle of the active list view ...
    HRESULT hRes = NOERROR;

    // remember the key state, this will tell us whether we are doing a left drag, or a 
    // a right mouse drag
    m_grfKeyState = grfKeyState & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON );

    // this shouldn't happen, but seems to occasionally, so rather than an Assertion that
    // will cause a GPF, we'll try and handle it nicely....
    if ( m_pDataObj != NULL )
    {
        // Drag leave wasn't called, so call it now...
        this->DragLeave();
    }

    m_pDataObj = pDataObj;
    m_pDataObj->AddRef();
        
    m_dwDragDropScrollDelay = 0;
    
    // reset the current item just incase...
    m_iCurItem = -1;
    
    // do a hit test on the list view ...
    LV_HITTESTINFO rgInfo;
    rgInfo.pt.x = (int) ptl.x;
    rgInfo.pt.y = (int) ptl.y;
    
    GetItemUnder( &rgInfo );

    // we are currently not scrolling....(although we might be real soon now.)
    m_dwScrollFlags = SCROLL_NONE;
    
    // we assume that we are entering the view from outside, or initiating a drag
    // from within

    // this long statement is because the listview seems able to do a hit test a return we are over an item
    // get it returns -1 as the item (i.e. there is no item), if we detect this eroneous return value
    // then tell ourselves we are over the background and wait for a drag-over....

    // NOTE: this clause will need to be adjusted to take account of the above, left, right ...etc
    // NOTE: flags that mean we are supposed to scroll the window instead.....;
    if ((( rgInfo.flags & LVHT_ONITEM ) && ( rgInfo.iItem != -1 )))
    {
        m_iFlags = rgInfo.flags;
    }
    else
    {
        // we are in the scroll region...
        m_dwScrollFlags = DragDirection( &ptl );

        if ( m_dwScrollFlags != SCROLL_NONE )
        {
            m_dwStartTickCount = GetTickCount();

            // set the current timeout to the Drag Drop inset delay
            m_dwDragDropScrollDelay = m_dwDragDropDelay;
            fScrolling = TRUE;
        }

        // we are therefore not over an object...
        m_iFlags = rgInfo.flags;
    }
    
    // are we over an item .....
    if ( rgInfo.flags & LVHT_ONITEM && rgInfo.iItem != -1 )
    {
        CreateDTForItemUnder( & rgInfo );

        if ( m_pCurDT != NULL )
        {
            FocusItem( m_iCurItem, TRUE );
            
            hRes = m_pCurDT->DragEnter( pDataObj, grfKeyState, ptl, pdwEffect );
        }
        else
        {
            // nothing to drop onto, so we are still over the background...
            rgInfo.iItem = -1;
        }
    }
    
    // are we over the background. ...
    if ( !( rgInfo.flags & LVHT_ONITEM ) || rgInfo.iItem == -1 )
    {
        // use the folder to get the DropTarget for the view ....
        if ( m_pBkgrndDT != NULL )
        {
            hRes = m_pBkgrndDT->DragEnter( pDataObj,
                                           grfKeyState,
                                           ptl,
                                           pdwEffect );
            Assert( SUCCEEDED( hRes ));
        }
    }
    // if we are not over either of the above, we must be outside the client region

    if ( fScrolling != FALSE )
    {
        hRes |= DROPEFFECT_SCROLL;
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CViewDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hRes = NOERROR;
    BOOL fScrolling = FALSE;
    if (m_pDragImages)
        m_pDragImages->DragOver((POINT*)&pt, *pdwEffect);
    else
        _DragMove(m_hwndDD, pt);
    // sometimes if we are dragging in the same window we started in, DragEnter
    // doesn't seem to be called, so catch it here...
    if ( m_hWndListView == NULL )
    {
        m_hWndListView = m_pParent->GetWindow();
    }
    
    // work out the scrolling first, otherwise the scrolling stops when you are over an
    // item.

    // the scroll flags for this point
    DWORD dwCurScrollFlags = DragDirection( &pt );

    // we are in a scroll region.....
    if ( dwCurScrollFlags != SCROLL_NONE )
    {
        // we already might be scrolling .....
        if ( m_dwScrollFlags != SCROLL_NONE )
        {
            // check the counter ...are we past the delay period ?
            if ( GetTickCount() - m_dwStartTickCount > m_dwDragDropScrollDelay )
            {
                WORD iDx = 0xffff;
                WORD iDy = 0xffff;
                
                // now work out which way to scroll.....
                if ( dwCurScrollFlags & SCROLL_LEFT )
                    iDx = SB_LINELEFT;
                    
                if ( dwCurScrollFlags & SCROLL_RIGHT )
                    iDx = SB_LINERIGHT;
                    
                if ( dwCurScrollFlags & SCROLL_UP )
                    iDy = SB_LINEUP;
                    
                if ( dwCurScrollFlags & SCROLL_DOWN )
                    iDy = SB_LINEDOWN;

                if (( iDx != 0xffff ) || ( iDy != 0xffff ))
                {  
                    // notify the owner that we are about to start scrolling
                    m_pParent->PreScrolling( iDx, iDy );
                }
                
                if ( iDx != 0xffff )
                {
                    // tell the view to scroll horizontally
                    SendMessageA( m_hWndListView, WM_HSCROLL, iDx, 0L );
                    SendMessageA( m_hWndListView, WM_HSCROLL, SB_ENDSCROLL, 0L );
                }
                if ( iDy != 0xffff )
                {
                    SendMessageA( m_hWndListView, WM_VSCROLL, iDy, 0L );
                    SendMessageA( m_hWndListView, WM_VSCROLL, SB_ENDSCROLL, 0L );
                }

                // make sure we are on the new scroll delay, not the inset one....
                // the delay expression may look arbitrary, but that is because it
                // is. This is the one the shell uses, so shall we.
                m_dwDragDropScrollDelay = GetDoubleClickTime() / 2;

                // get the new tick count, i.e. we are starting a new delay cycle, just
                // a different one. Mr brockscmidt recomments using the OLE pulsed call
                // of DragOver, however if you do this it scroll past REAL fast.
                m_dwStartTickCount = GetTickCount();
            }
            fScrolling = TRUE;
        }
        else
        {
            // get the start count of the ticker.....
            m_dwStartTickCount = GetTickCount();
        }
    }

    // remember the current scroll flags...     
    m_dwScrollFlags = dwCurScrollFlags;
    
    // do a hit test on the list view ...
    LV_HITTESTINFO rgInfo;
    rgInfo.pt.x = (int) pt.x;
    rgInfo.pt.y = (int) pt.y;
    
    GetItemUnder( &rgInfo );

    // check to see if we are over an item ...
    // NOTE: note the second clause checking for -1, this is because there is a bug in the 
    // NOTE: commcontrol header files that maps two return flags onto the same value .....
    if ( ( rgInfo.flags & LVHT_ONITEM ) && ( rgInfo.iItem != -1 ) )
    {
        BOOL fRestart = FALSE;

        if (( m_iFlags & LVHT_ONITEM ) && ( m_iCurItem != rgInfo.iItem ))
        {
            // we jumped onto another item so deselect it and we will need to drag
            // enter the next one next .... at this point, we may have an old drop 
            // target and we may not have one, if we have one it is because the last
            // item was a drop target, it we don't it means it wasn't either way,
            // defocussing will not do any harm as there is a special drop hilite focus
            FocusItem( m_iCurItem, FALSE );
            fRestart = TRUE;
        }
        
        // create the drop target for the current item ....
        CreateDTForItemUnder( & rgInfo );

        if ( m_pCurDT != NULL )
        {
            // we were over the background, but now we have found a new droptarget
            if (( m_iFlags & LVHT_NOWHERE ) && ( m_pBkgrndDT != NULL ))
            {
                // we must have walked off the background onto an item ....
                m_pBkgrndDT->DragLeave();
                fRestart = TRUE;
            }
            
            //if this item is a new droptarget
            if ( fRestart == TRUE )
            {
                Assert ( m_pDataObj != NULL );
                
                // we must have a new drop target, 
                hRes = m_pCurDT->DragEnter( m_pDataObj, grfKeyState, pt, pdwEffect );
                m_iCurItem = rgInfo.iItem;
                FocusItem( m_iCurItem, TRUE );
            }
            else
            {
                // same old drop target, just drag over it ....
                hRes = m_pCurDT->DragOver( grfKeyState, pt, pdwEffect );
            }
        }
        else 
        {   
            if ( m_pBkgrndDT )
            {
                hRes = m_pBkgrndDT->DragOver( grfKeyState, pt, pdwEffect );
            }
            else
            {
                hRes = NOERROR;
                if ( pdwEffect != NULL )
                {
                    *pdwEffect = 0;
                }
            }
            // change the flag so we know who to drag-leave...
            rgInfo.flags = LVHT_NOWHERE;
        }
    }
    // are we over the background. ...
    else if ( rgInfo.flags & LVHT_NOWHERE )
    {
        BOOL fRestart = FALSE;
        
        if ( m_iFlags & LVHT_ONITEM )
        {
            if ( m_pCurDT != NULL )
            {
                m_pCurDT->DragLeave();
                FocusItem( m_iCurItem, FALSE );
            }
            
            fRestart = TRUE;
        }

        // attempt to use the background DT
        if ( m_pBkgrndDT != NULL )
        {
            if ( fRestart == TRUE )
            {
                hRes = m_pBkgrndDT->DragEnter( m_pDataObj, grfKeyState,
                    pt, pdwEffect );
            }
            else
            {
                hRes = m_pBkgrndDT->DragOver( grfKeyState, pt, pdwEffect );
            }       
        }
        else
        {
            hRes = NOERROR;
            if ( pdwEffect != NULL )
            {
                *pdwEffect = 0;
            }
        }
    }
    else
    {
        *pdwEffect = 0;
    }

    // remember the last set of flags ...
    m_iFlags = rgInfo.flags;

    if ( fScrolling == TRUE )
    {
        *pdwEffect |= DROPEFFECT_SCROLL;
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CViewDropTarget::DragLeave ( void )
{
    if (m_pDragImages)
        m_pDragImages->DragLeave();
    else
        DAD_DragLeave();

    if ( m_pBkgrndDT != NULL )
    {
        if ( m_iFlags & LVHT_NOWHERE )
        {
            m_pBkgrndDT->DragLeave();
        }
        m_pBkgrndDT->Release();
        m_pBkgrndDT = NULL;
    }

    if ( m_pCurDT != NULL )
    {
        if ( m_iFlags & LVHT_ONITEM )
        {
            m_pCurDT->DragLeave();
            FocusItem( m_iCurItem, FALSE );
        }
        m_pCurDT->Release();
        m_pCurDT = NULL;
    }

    if ( m_pDataObj != NULL )
    {
        m_pDataObj->Release();
        m_pDataObj = NULL;
    }

    m_iCurItem = -1;
    m_hWndListView = NULL;
    
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CViewDropTarget::Drop ( IDataObject *pDataObj,
                                     DWORD grfKeyState,
                                     POINTL pt,
                                     DWORD *pdwEffect )
{
    HRESULT hRes = NOERROR;

    if  ( pdwEffect == NULL )
    {
        return E_INVALIDARG;
    }

    if ( m_pDataObj != NULL )
    {
        m_pDataObj->Release();
        m_pDataObj = NULL;
    }

    // incase we move the icons....
    m_ptDragEnd.x = pt.x;
    m_ptDragEnd.y = pt.y;
    
    ScreenToClient( m_hWndListView, &m_ptDragEnd );

    // we dropped on the background....
    m_fDropOnBack = TRUE;
    
    // do a hit test on the list view ...
    LV_HITTESTINFO rgInfo;
    rgInfo.pt.x = (int) pt.x;
    rgInfo.pt.y = (int) pt.y;

    GetItemUnder( &rgInfo );

    // changed it to be same as drag enter - pedro.
    // this long statement is because the listview seems able to do a hit test
    // a return we are over an item get it returns -1 as the item (i.e. there 
    // is no item), if we detect this eroneous return value
    // then tell ourselves we are over the background and wait for a drag-over....
    if ((( rgInfo.flags & LVHT_ONITEM ) && ( rgInfo.iItem != -1 )))
    {
        m_iFlags = rgInfo.flags;
    }
    else
    {
        m_iFlags = rgInfo.flags = LVHT_NOWHERE;
    }

    if ( rgInfo.flags & LVHT_ONITEM )
    {
        CreateDTForItemUnder( & rgInfo );

        if ( m_pCurDT != NULL )
        {
            hRes = m_pCurDT->Drop( pDataObj, grfKeyState, pt, pdwEffect );
        }
        else
        {
            // we are really over the background...
            rgInfo.flags = LVHT_NOWHERE;
        }
    }
    // are we over the background. ...
    if (( rgInfo.flags & LVHT_NOWHERE ))
    {
        Assert( pdwEffect != NULL );

        // are we in the same window, and not using the right mouse button ?
        // do we not have any of the other keys pressed ?
        if ( m_pParent->WasDragStartedHere() != FALSE 
            && (*pdwEffect & DROPEFFECT_MOVE ) 
            && !( m_grfKeyState & MK_RBUTTON ) 
            && !( grfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT )))
        {
            POINT rgOrigin = {0, 0};

            m_pParent->GetOrigin( &rgOrigin );
            
            // move the icons ...
            int dx = m_ptDragEnd.x - m_ptDragStart.x + rgOrigin.x;
            int dy = m_ptDragEnd.y - m_ptDragStart.y + rgOrigin.y;

            // move the items in the view...
            hRes = this->MoveSelectedItems( dx, dy );

            // Don't forget to call this so we stop drawing the images!!!!
            if (m_pDragImages)
                m_pDragImages->DragLeave();

            if (m_pBkgrndDT)
                m_pBkgrndDT->DragLeave();

            return hRes;
        }
        else
        {
            if (m_pBkgrndDT)
            {
                // use the folder DT. 
                hRes = m_pBkgrndDT->Drop( pDataObj, grfKeyState, pt, pdwEffect );
            }
        }
    }

    if (m_pDragImages)
        m_pDragImages->Drop(pDataObj, (POINT*)&pt, *pdwEffect);

    // ole doesn't call drag leave if we drop, so we must tidy up
    // instead. this is best done by calling DragLeave !
    this->DragLeave();
    return hRes;
}

//////////////////////////////////////////////////////////////////////////////////////////
DWORD CViewDropTarget::DragDirection( const POINTL * ppt )
{
    POINT ptPoint;

    ptPoint.x = ppt->x;
    ptPoint.y = ppt->y;
    
    if ( ScreenToClient( m_hWndListView, & ptPoint ) == FALSE )
    {
        return SCROLL_NONE;
    }
    
    RECT rcOuter, rcInner;
    DWORD dwScrollDir = SCROLL_NONE;
    
    DWORD dwStyle = GetWindowLongWrapW(m_hWndListView, GWL_STYLE);

    GetClientRect(m_hWndListView, &rcInner);

    // the explorer forwards us drag/drop things outside of our client area
    // so we need to explictly test for that before we do things
    //
    rcOuter = rcInner;

    InflateRect(&rcInner, -((int) m_dwDragDropInset), -((int) m_dwDragDropInset));

    if (!PtInRect(&rcInner, ptPoint) && PtInRect(&rcOuter, ptPoint))
    {
        // Yep - can we scroll horizontally ?
        if (dwStyle & WS_HSCROLL)
        {
            if (ptPoint.x < rcInner.left)
            {
                if (CanScroll(m_hWndListView, SB_HORZ, FALSE))
                    dwScrollDir |= SCROLL_LEFT;
            }
            else if (ptPoint.x > rcInner.right)
            {
                if (CanScroll(m_hWndListView, SB_HORZ, TRUE))
                    dwScrollDir |= SCROLL_RIGHT;
            }
        }
        if (dwStyle & WS_VSCROLL)
        {
            if (ptPoint.y < rcInner.top)
            {
                if (CanScroll(m_hWndListView, SB_VERT, FALSE))
                    dwScrollDir |= SCROLL_UP;
            }
            else if (ptPoint.y > rcInner.bottom)
            {
                if (CanScroll(m_hWndListView, SB_VERT, TRUE))
                    dwScrollDir |= SCROLL_DOWN;
            }
        }
    }
    return dwScrollDir;

}


//////////////////////////////////////////////////////////////////////////////////////////
// checks to see if we are at the end position of a scroll bar
// to avoid scrolling when not needed (avoid flashing)
//
// in:
//      code        SB_VERT or SB_HORZ
//      bDown       FALSE is up or left
//                  TRUE  is down or right

BOOL CViewDropTarget::CanScroll(HWND hWnd, int code, BOOL bDown)
{
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hWnd, code, &si);

    if (bDown)
    {
        if (si.nPage)
            si.nMax -= si.nPage - 1;
        return si.nPos < si.nMax;
    }
    else
    {
        return si.nPos > si.nMin;
    }
}


HRESULT CViewDropTarget::MoveSelectedItems( int iDx, int iDy )
{
    // move the items that are currently selected in the view....
    // we are in the same view, so we are just moving the icons....
    // because we are in the same window, we have cahced the 
    // selection....

    POINT pt;
    int iItem = ListView_GetNextItem(m_hWndListView, -1, LVNI_SELECTED);
    while ( iItem >= 0 )
    {
        ListView_GetItemPosition(m_hWndListView, iItem, &pt);

        pt.x += iDx;
        pt.y += iDy;

        //
        // Adjust the drop point to correspond to line up on the drop grid,
        // if the user has the custom prefs setting for "DropGrid"
        //

        HKEY hKey;
        if (ERROR_SUCCESS == RegOpenKeyA( HKEY_CURRENT_USER,
                                         c_szRegPathCustomPrefs,
                                         &hKey))
        {
            DWORD dwType = 0;
            DWORD dwGrid = 0;
            DWORD dwSize = sizeof( dwGrid );

            if (ERROR_SUCCESS == RegQueryValueExA( hKey,
                                                  c_szDropGrid,
                                                  NULL,
                                                  &dwType,
                                                  (LPBYTE) &dwGrid,
                                                  &dwSize))
            {
                pt.x += (dwGrid / 2);
                pt.y += (dwGrid / 2);
                pt.x -= pt.x % dwGrid;
                pt.y -= pt.y % dwGrid;
            }
            RegCloseKey(hKey);
        }

        ListView_SetItemPosition32(m_hWndListView, iItem, pt.x, pt.y);

        iItem = ListView_GetNextItem(m_hWndListView, iItem, LVNI_SELECTED);
    }

    // tell the parent that we have moved the items
    m_pParent->MoveSelectedItems( iDx, iDy );
    
    return NOERROR;
}


IDropTarget *CViewDropTarget::GetBackgrndDT( void )
{
    return m_pBkgrndDT;
}

void CViewDropTarget::DragStartHere( const POINT * prgStart )
{
    m_ptDragStart = *prgStart;
}

void CViewDropTarget::DropPoint( POINT * pDrop )
{
    *pDrop = m_ptDragEnd;
}

BOOL CViewDropTarget::DropOnBackGrnd( void )
{
    return m_fDropOnBack;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////DropSource////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

CViewDropSource::CViewDropSource( )
{
    // we start with no keys pressed ...
    m_grfInitialKeyState = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
CViewDropSource::~CViewDropSource( )
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CViewDropSource::QueryContinueDrag ( BOOL fEscapePressed, DWORD 
grfKeyState )
{
    HRESULT hres = S_OK;
    // the current state of the three buttons only
    DWORD grfButtonState = grfKeyState & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON
);
        

    if (fEscapePressed || ( m_grfInitialKeyState == 0 && grfButtonState == 0 ))
    {
        hres = DRAGDROP_S_CANCEL;
    }
    else
    {
        // initialize ourself with the drag begin button
        if ( m_grfInitialKeyState == 0 )
        {
            m_grfInitialKeyState = grfButtonState;
        }

        Assert( m_grfInitialKeyState != 0);

        if ( (grfKeyState & m_grfInitialKeyState) == 0 )
        {
            //
            // A button is released.
            //
            hres = DRAGDROP_S_DROP; 
        }
        else if ( m_grfInitialKeyState != grfButtonState )
        {
            //
            //  If the button state is changed (except the drop case, which we handle
            // above, cancel the drag&drop.
            //
            hres = DRAGDROP_S_CANCEL;
        }
    }

    if (hres != S_OK)
    {
        // reset the cursor back....
        SetCursor(LoadCursorA(NULL, (LPCSTR) IDC_ARROW));

    }

    return hres;
}

//////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CViewDropSource::GiveFeedback ( DWORD dwEffect )
{
    // for now at least, use the standard OLE cursors .....
    /*[TODO: immitate the shell's approach to showing cursors for drag drop .... ]*/
    return DRAGDROP_S_USEDEFAULTCURSORS;
}
