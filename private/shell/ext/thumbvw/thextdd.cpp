/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * This file implements the IShellFolderView interface
 */

#include "precomp.h"

//////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::SortBy( LPARAM dwArrange, BOOL fAscend )
{
    if (m_hWndListView != NULL )
    {
        // now sort the view ...
        ListViewCompareStruct   rgCompare;

        // the shell always loses the sort item, so shall we :-)
        rgCompare.m_iCompareFlag = dwArrange;
        rgCompare.m_iAscend = ( fAscend ? 1 : -1 );
        rgCompare.m_pFolder = m_pFolder;
        m_pFolder->AddRef();

        ListView_SortItems( m_hWndListView, ListViewCompare, ( LPARAM ) &rgCompare );
            
        rgCompare.m_pFolder->Release();
    }
}

//////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::EnumFolder( )
{
    ULONG ulType = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

    if ( m_fShowAllObjects )
       ulType |= SHCONTF_INCLUDEHIDDEN;
    
    IEnumIDList *pEnum;
    HRESULT hr = m_pFolder->EnumObjects(this->m_hWnd, ulType, &pEnum);
    if (hr == S_OK)
    {
        for (;;)
        {
            ULONG celtFetched;
            LPITEMIDLIST pidlList;
            hr = pEnum->Next( 1, &pidlList, &celtFetched );
            if ((hr == S_OK) && (celtFetched == 1))
            {
                int iRes = this->AddItem(pidlList);
                if ( iRes == -1 )
                    SHFree(pidlList);  // AddItem() failed, clean up
            }
            else
                break;      // end of loop ....
        }

        pEnum->Release();
    }
    else if (hr == S_FALSE)
    {
        IShellFolderView *psfv;
        hr = m_pDefView->QueryInterface(IID_IShellFolderView, (void **)&psfv);
        if (SUCCEEDED(hr))
        {
            UINT cItems;
            hr = psfv->GetObjectCount(&cItems);
            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; i < cItems ; i++)
                {
                    LPITEMIDLIST pidl;
                    if (SUCCEEDED(psfv->GetObject(&pidl, i)))
                    {
                        LPITEMIDLIST pidlAdd = ILClone(pidl);
                        if (pidlAdd)
                        {
                            if (-1 == this->AddItem(pidlAdd))
                                ILFree(pidlAdd);
                        }
                    }
                }
            }
        }
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::FocusOnSomething( )
{
    ListView_SetItemState( m_hWndListView, 0, LVIS_FOCUSED, LVIS_FOCUSED );
}

int CThumbnailView::AddItem( LPCITEMIDLIST pidl )
{
    int iPos = -1;

    // NOTE: the IShellView we pass is the one for the Def-View, if it 
    // NOTE: needs to delegate to us, it will.
    if (m_pCommDlg == NULL || 
        S_FALSE != m_pCommDlg->IncludeObject(m_pDefView, pidl))
    {
        // note: the icon is done on a callback so that we only fetch those displayed
        WCHAR szTextBuffer[MAX_PATH];
        LV_ITEMW rgData;
        ZeroMemory( &rgData, sizeof( rgData ));
        rgData.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        rgData.iItem = 0x7fffffff - 1;
        rgData.lParam = (LPARAM) pidl;

        rgData.iImage = I_IMAGECALLBACK;
        rgData.pszText = szTextBuffer;

        ULONG ulAttrs = SFGAO_GHOSTED;
        if ( SUCCEEDED(m_pFolder->GetAttributesOf( 1, &pidl, &ulAttrs )) && ulAttrs & SFGAO_GHOSTED )
        {
            rgData.stateMask = LVIS_CUT;
            rgData.state = LVIS_CUT;
            rgData.mask |= LVIF_STATE;
        }
   
        STRRET strret;
        if (SUCCEEDED(m_pFolder->GetDisplayNameOf( pidl, SHGDN_NORMAL, &strret )))
        {
            StrRetToBufW(&strret, pidl, szTextBuffer, ARRAYSIZE(szTextBuffer));
            iPos = ListView_InsertItemWrapW( m_hWndListView, &rgData);
        }
    }
    return iPos;
}

///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////CDropTargetClient////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::PreScrolling( WORD wVertical, WORD wHorizontal )
{
    // do nothing...
}

//////////////////////////////////////////////////////////////////////////////////////////
void CThumbnailView::GetOrigin( POINT * prgOrigin )
{
    Assert( prgOrigin != NULL );
    ListView_GetOrigin( m_hWndListView, prgOrigin );
}

//////////////////////////////////////////////////////////////////////////////////////////
HWND CThumbnailView::GetWindow()
{
    return m_hWndListView;
}

//////////////////////////////////////////////////////////////////////////////////////////
BOOL CThumbnailView::WasDragStartedHere()
{
    return m_fDragStarted;
}

//////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailView::MoveSelectedItems( int iDx, int iDy )
{
    // nothing to move behind the scenes ...
    m_fItemsMoved = TRUE;
    return NOERROR;
}
