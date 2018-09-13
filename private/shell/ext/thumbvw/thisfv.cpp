/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * This file implements the IShellFolderView interface
 */

#include "precomp.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::Rearrange ( LPARAM lParamSort)
{
    SortBy( lParamSort, TRUE );
    m_iSortBy = (int) lParamSort;
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetArrangeParam ( LPARAM *plParamSort)
{
    if ( plParamSort == NULL )
    {
        return E_INVALIDARG;
    }

    *plParamSort = (LPARAM) m_iSortBy;
        
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::ArrangeGrid ()
{
    ListView_Arrange( m_hWndListView, LVA_SNAPTOGRID );
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::AutoArrange ()
{
    DWORD dwMainStyle = GetWindowLongWrapW( m_hWndListView, GWL_STYLE );
    DWORD dwArrange = dwMainStyle & LVS_AUTOARRANGE;
    
    dwMainStyle = dwMainStyle & ~LVS_AUTOARRANGE;
    dwArrange = ~dwArrange;
    
    SetWindowLongWrapW( m_hWndListView, GWL_STYLE, 
        dwMainStyle | ( dwArrange & LVS_AUTOARRANGE ) );

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetAutoArrange ()
{
    DWORD dwMainStyle = GetWindowLongWrapW( m_hWndListView, GWL_STYLE );
    DWORD dwArrange = dwMainStyle & LVS_AUTOARRANGE;

    HRESULT hr = S_FALSE;
    
    if ( dwArrange != 0 )
    {
        hr = S_OK;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::AddObject(LPITEMIDLIST pidl, UINT *puItem)
{
    int iItem = FindInView( m_hWndListView, m_pFolder, pidl );
    if ( iItem == -1 )
    {
        LPITEMIDLIST pidlCopy = ILClone( pidl );
        if (pidlCopy)
        {
            iItem = AddItem( pidlCopy );
            if ( iItem == -1 )
                SHFree(pidlCopy);
        }
        else
            return E_OUTOFMEMORY;
    }
    
    *puItem = iItem;
    
    return ( iItem != - 1) ? NOERROR : E_FAIL ;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetObject(LPITEMIDLIST *ppidl, UINT uItem)
{
    // should we return a pointer to our cached pidl, or should we duplicate it ?
    
    // Worse hack, if -42 then return our own pidl...
    if (uItem == (UINT)-42)
    {
        *ppidl = (LPITEMIDLIST) m_pidl;
        return *ppidl ? NOERROR : E_UNEXPECTED;
    }
    
    // Hack, if item is -2, this implies return the focused item
    if (uItem == (UINT)-2)
        uItem = ListView_GetNextItem(m_hWndListView, -1, LVNI_FOCUSED);

    LV_ITEMW rgItem;
    ZeroMemory( &rgItem, sizeof( rgItem ));
    rgItem.mask = LVIF_PARAM;
    rgItem.iItem = uItem;

    BOOL fFetch = ListView_GetItemWrapW( m_hWndListView, & rgItem );

    if ( fFetch )
    {
        *ppidl = (LPITEMIDLIST) rgItem.lParam;
        return NOERROR;
    }
    
    return E_UNEXPECTED;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::RemoveObject ( LPITEMIDLIST pidl, UINT *puItem)
{
    // Non null go look for item.
    int iItem = FindInView( m_hWndListView, m_pFolder, pidl );
    int iFocus = -1;

    if ( iItem < 0 )
    {
        return E_UNEXPECTED;
    }

    if ( puItem != NULL )
    {
        *puItem = (UINT) iItem;
    }
    
    UINT uState = ListView_GetItemState( m_hWndListView, iItem, LVIS_ALL);
    RECT rc;
    if (uState & LVIS_FOCUSED) 
    {
        ListView_GetItemRect( m_hWndListView, iItem, &rc, LVIR_ICON );
    }

    LV_ITEMW rgItem;
    ZeroMemory( &rgItem, sizeof( rgItem ));
    rgItem.mask = LVIF_PARAM | LVIF_IMAGE;
    rgItem.iItem = iItem;

    BOOL fFetch = ListView_GetItemWrapW( m_hWndListView, &rgItem );
    Assert( fFetch );
    
    // do the actual delete
    ListView_DeleteItem( m_hWndListView, iItem );

    // we deleted the focused item.. replace the focus to the nearest item.
    if (uState & LVIS_FOCUSED)
    {
        int iFocus = iItem;
        LV_FINDINFO lvfi;

        lvfi.flags = LVFI_NEARESTXY;
        lvfi.pt.x = rc.left;
        lvfi.pt.y = rc.top;
        lvfi.vkDirection = 0;
        iFocus = ListView_FindItem( m_hWndListView, -1, &lvfi );
    }
    else 
    {
        if ( ListView_GetItemCount( m_hWndListView ) >= iFocus )
        {
            iFocus --;
        }
    }

    if ( iFocus != -1 ) 
    {
        ListView_SetItemState( m_hWndListView, iFocus, LVIS_FOCUSED, LVIS_FOCUSED );
        ListView_EnsureVisible( m_hWndListView, iFocus, FALSE );
    }

    SHFree( (LPITEMIDLIST) rgItem.lParam );
    if ( rgItem.iImage != I_IMAGECALLBACK )
        m_pImageCache->FreeImage( rgItem.iImage );
        
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetObjectCount ( UINT *puCount)
{
    if ( puCount == NULL )
    {
        return E_INVALIDARG;
    }
    
    *puCount = ListView_GetItemCount( m_hWndListView );
    
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetObjectCount ( UINT uCount, UINT dwFlags)
{
    DWORD dw = dwFlags;
    
    if ((dwFlags & SFVSOC_INVALIDATE_ALL) == 0)
        dw |= LVSICF_NOINVALIDATEALL; // gross transform

    return (HRESULT) SendMessageA ( m_hWndListView,
                                   LVM_SETITEMCOUNT,
                                   (WPARAM)uCount,
                                   (LPARAM)dw );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::UpdateObject ( LPITEMIDLIST pidlOld, LPITEMIDLIST pidlNew,UINT *puItem)
{
    HRESULT hr = E_INVALIDARG;
    
    int iItem = FindInView( m_hWndListView, m_pFolder, pidlOld );
    if ( iItem >= 0 )
    {
        *puItem = iItem;
        
        LV_ITEMW rgItem;
        ZeroMemory( &rgItem, sizeof( rgItem ));

        rgItem.mask = LVIF_PARAM | LVIF_IMAGE | LVIF_NORECOMPUTE;
        rgItem.iItem = iItem;

        BOOL fRes = ListView_GetItemWrapW( m_hWndListView, &rgItem );
        Assert( fRes );

        // save the old pidl...
        LPITEMIDLIST pidlSave = (LPITEMIDLIST) rgItem.lParam;

        if ( rgItem.iImage != I_IMAGECALLBACK )
        {
            // force the image to be refected so the icon is right ...
            Assert( m_pImageCache );
            m_pImageCache->DeleteImage( rgItem.iImage );
        }
        
        rgItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        rgItem.iItem = iItem;
        rgItem.iImage = I_IMAGECALLBACK;
        rgItem.iSubItem = 0;  // REVIEW: bug in listview?
        rgItem.lParam = (LPARAM)pidlNew;


        WCHAR szTextBuffer[MAX_PATH];
        rgItem.pszText = szTextBuffer;

        // use IShellFolder to get the display name .....
        STRRET rgStringRet;
        hr = m_pFolder->GetDisplayNameOf( pidlNew, SHGDN_NORMAL, &rgStringRet );
        if ( FAILED( hr ) )
        {
            // NOTE: if we failed to get the name of a element, then
            // NOTE: don't change the display name....
            // NOTE: a name....
            rgItem.mask &= ~LVIF_TEXT;
        }
    
        szTextBuffer[0] = 0;
        StrRetToBufW( &rgStringRet, pidlNew, szTextBuffer, ARRAYSIZE(szTextBuffer) );

        fRes = ListView_SetItemWrapW(m_hWndListView, &rgItem);
        Assert( fRes );

        if ( fRes )
        {
            // Free the old pidl after we've added the new one
            SHFree( pidlSave );
        }

        hr = NOERROR;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::RefreshObject ( LPITEMIDLIST pidl, UINT *puItem)
{
    if ( puItem == NULL )
    {
        return E_INVALIDARG;
    }
    
    int iItem = FindInView( m_hWndListView, m_pFolder, pidl );
    if (iItem >= 0)
    {
        ListView_RedrawItems( m_hWndListView, iItem, iItem );
    }

    *puItem = iItem;
    
    return (iItem >= 0) ? NOERROR : E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetRedraw ( BOOL bRedraw)
{
    SendMessageA( m_hWndListView, WM_SETREDRAW, (WPARAM)bRedraw, 0);
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetSelectedCount ( UINT *puSelected )
{
    if ( puSelected == NULL )
    {
        return E_INVALIDARG;
    }

    * puSelected = ListView_GetSelectedCount( m_hWndListView );

    return NOERROR;

}

STDMETHODIMP CThumbnailView::GetSelectedObjects(LPCITEMIDLIST **pppidl, UINT *puItems)
{
    HRESULT hr;
    int iCount = ListView_GetSelectedCount( m_hWndListView );
    if (iCount)
    {
        *pppidl = (LPCITEMIDLIST *) LocalAlloc(LPTR, sizeof(LPCITEMIDLIST) * iCount);
        if (*pppidl)
        {
            int iFocused = ListView_GetNextItem( m_hWndListView, -1, LVNI_FOCUSED );
        
            hr = GetSelectionPidlList( m_hWndListView, iCount, *pppidl, iFocused );
            if (FAILED( hr ))
            {
                LocalFree(*pppidl);
                *pppidl = NULL;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        *pppidl = NULL;

    *puItems = iCount;
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::IsDropOnSource ( IDropTarget *pDropTarget)
{
    return ( m_pDropTarget->GetBackgrndDT() == pDropTarget && m_fDragStarted ) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetDragPoint ( POINT *ppt)
{
    if ( ppt == NULL )
    {
        return E_INVALIDARG;
    }

    if ( m_pDropTarget == NULL )
    {
        return E_UNEXPECTED;
    }
    
    if ( m_fDragStarted == FALSE )
    {
        return E_FAIL;
    }
    
    *ppt = m_ptDragStart;
    
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetDropPoint ( POINT *ppt)
{
    if ( ppt == NULL )
    {
        return E_INVALIDARG;
    }

    if ( m_pDropTarget == NULL )
    {
        return E_UNEXPECTED;
    }
    
    if ( m_pDropTarget->DropOnBackGrnd() == FALSE )
    {
        return E_FAIL;
    }

    m_pDropTarget->DropPoint( ppt );
    
    return NOERROR;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::MoveIcons ( IDataObject *pDataObject)
{
    m_fItemsMoved = TRUE;
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetItemPos ( LPCITEMIDLIST pidl, POINT *ppt)
{
    if ( ppt == NULL )
    {
        return E_INVALIDARG;
    }
    
    int iItem = FindInView( m_hWndListView, m_pFolder, pidl );
    if ( iItem >= 0 )
    {
        m_fItemsMoved = TRUE;
        ListView_SetItemPosition32( m_hWndListView, iItem, ppt->x, ppt->y );
    }

    return ( iItem < 0 ? E_INVALIDARG : NOERROR );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::IsBkDropTarget ( IDropTarget *pDropTarget)
{
    if ( m_pDropTarget == NULL )
    {
        return E_UNEXPECTED;
    }
    
    return ( m_pDropTarget->DropOnBackGrnd() ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetClipboard ( BOOL bMove)
{
    if ( bMove )
    {
        //
        //  mark all selected items as being "cut"
        //
        int i = -1;
        while ((i = ListView_GetNextItem( m_hWndListView, i, LVIS_SELECTED )) != -1)
        {
            ListView_SetItemState( m_hWndListView, i, LVIS_CUT, LVIS_CUT );
        }
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetPoints ( IDataObject *pDataObject)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::GetItemSpacing ( ITEMSPACING *pSpacing)
{
    DWORD dwSize;

    dwSize = ListView_GetItemSpacing(m_hWndListView, FALSE);
    
    pSpacing->cxLarge = GetSystemMetrics( SM_CXICONSPACING );
    pSpacing->cyLarge = GetSystemMetrics( SM_CYICONSPACING );
    pSpacing->cxSmall = GET_X_LPARAM(dwSize);
    pSpacing->cySmall = GET_Y_LPARAM(dwSize);

    // we are always in large icon mode..
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetCallback ( IShellFolderViewCB* pNewCB,IShellFolderViewCB** ppOldCB)
{
    if ( pNewCB == NULL )
    {
        return E_INVALIDARG;
    }

    if ( ppOldCB != NULL )
    {
        *ppOldCB = m_pFolderCB;
    }

    // if we had a ref on an old Callback, it goes out in the out param.
    
    m_pFolderCB = pNewCB;
    m_pFolderCB->AddRef();
    
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::Select ( UINT dwFlags )
{
    HRESULT hr = NOERROR;
    switch( dwFlags )
    {
        case SFVS_SELECT_ALLITEMS:
        {
            DECLAREWAITCURSOR;
            
            if (m_pFolderCB->MessageSFVCB( SFVM_SELECTALL, 0, 0) != (S_FALSE)) 
            {
                SetWaitCursor();
                SetFocus( m_hWndListView );
                ListView_SetItemState(m_hWndListView, -1, LVIS_SELECTED, LVIS_SELECTED );
                ResetWaitCursor();
            }
            break;
        }
            
        case SFVS_SELECT_NONE:
            ListView_SetItemState(m_hWndListView, -1, 0, LVIS_SELECTED);
            break;
            
        case SFVS_SELECT_INVERT:
        {
            DECLAREWAITCURSOR;
            SetWaitCursor();
            SetFocus(m_hWndListView);
            int iItem = -1;
            while ((iItem = ListView_GetNextItem(m_hWndListView, iItem, 0)) != -1)
            {
                UINT flag;

                // flip the selection bit on each item
                flag = ListView_GetItemState( m_hWndListView, iItem, LVIS_SELECTED );
                flag ^= LVNI_SELECTED;
                ListView_SetItemState( m_hWndListView, iItem, flag, LVIS_SELECTED );
            }
            ResetWaitCursor();
            break;
        }
        
        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::QuerySupport (UINT * pdwSupport )
{
    if ( pdwSupport == NULL )
    {
        return E_INVALIDARG;
    }
    
    *pdwSupport &= ( SFVQS_SELECT_ALL | SFVQS_SELECT_NONE
                   | SFVQS_SELECT_INVERT | SFVQS_AUTO_ARRANGE
                   | SFVQS_ARRANGE_GRID );

    return (*pdwSupport ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailView::SetAutomationObject ( IDispatch* pdisp)
{
    // Release any previous automation objects we may have...
    if (m_pAuto)
        ATOMICRELEASE(m_pAuto);

    if (pdisp)
    {
        // Hold onto the object...
        m_pAuto = pdisp;
        pdisp->AddRef();
    }

    return NOERROR;
}

