#include "precomp.h"
#include "shellp.h"
#include "dllload.h"



UINT GetPidlLength( LPCITEMIDLIST pidl )
{
    UINT cbSize = 0;
    while ( pidl->mkid.cb != 0 )
    {
        cbSize += pidl->mkid.cb;
        pidl = (LPCITEMIDLIST) ( (BYTE *) pidl + pidl->mkid.cb);
    }
    return cbSize;
}

////////////////////////////////////////////////////////////////////////////////
// used the the ListView for sorting items.
int CALLBACK ListViewCompare( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    ListViewCompareStruct * pStruct = (ListViewCompareStruct * ) lParamSort;

    Assert( pStruct->m_pFolder != NULL );

    HRESULT hr = pStruct->m_pFolder->CompareIDs( pStruct->m_iCompareFlag,
                                                 (LPCITEMIDLIST) lParam1,
                                                 (LPCITEMIDLIST) lParam2 );
    int iRes = (int) (short) SCODE_CODE( GetScode( hr ));
    return iRes * pStruct->m_iAscend;
}


/////////////////////////////////////////////////////////////////////////////////
HRESULT GetSelectionPidlList( HWND hwnd,
                              int cItems,
                              LPCITEMIDLIST * apidl,
                              int iItemHit )
{
    if ( apidl == NULL )
    {
        return E_INVALIDARG;
    }
    
    int iItem = -1;

    // assume that the get function doesn't mangle this structure too bad ...
    LV_ITEMW rgItem;
    ZeroMemory( &rgItem, sizeof( LV_ITEM ) );
    rgItem.mask = LVIF_PARAM;

    int iCtr = 0;
    
    for ( int iIndex = 0; iIndex < (int) cItems; iIndex ++ )
    {
        iItem = ListView_GetNextItem( hwnd, iItem, LVNI_SELECTED );

        // Assert that we were not passed a bogus view selection count...
        Assert( iItem != -1 );

        rgItem.iItem = iItem;
        ListView_GetItemWrapW( hwnd, &rgItem);

        Assert( rgItem.mask & LVIF_PARAM );

    // NOTE: we must put the item that was FOCUSED as the
    // NOTE: top item in the list. there are context menu extensions that depend on

		// NOTE: this behaviour ....etc
        // Check if the item is the focused one or not.
        if ( iItem == ( int )iItemHit )
        {
            // Yes, put it at the top.
            apidl[iCtr] = apidl[0];
            apidl[0] = (LPCITEMIDLIST) rgItem.lParam;
        }
        else
            // No, put it at the end of the list.
            apidl[iCtr] = (LPCITEMIDLIST) rgItem.lParam;
        iCtr++;
    }

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////
int FindInView( HWND hWnd, LPSHELLFOLDER pFolder, LPCITEMIDLIST pidl )
{
    if ( pFolder == NULL || pidl == NULL )
    {
        return -1;
    }
    
    LV_ITEMW rgItem;
    int     iItem = -1;

    memset( &rgItem, 0, sizeof( rgItem ) );
    // search through the list view looking for a matching lParam       
    do
    {
        iItem = ListView_GetNextItem( hWnd, iItem, LVNI_ALL );
        if ( iItem != -1 )
        {
            rgItem.mask = LVIF_PARAM;
            rgItem.iItem = iItem;
            BOOL bRes = (BOOL) SendMessageA( hWnd, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW *)(&rgItem));
            Assert( bRes );

            // ask the folder to kindly compare the Id's for us
            if ( pFolder->CompareIDs( 0, pidl, (LPCITEMIDLIST) rgItem.lParam ) == 0 )
            {
                break;
            }
        }
    }
    while ( iItem != -1 );
    
    return iItem;
}

/////////////////////////////////////////////////////////////////////////////////////////
void Clear( HWND hWnd )
{
    // because we cache an instance of the pidl as the LPARAM, 
    // it must be freed, so we must enum the vew first..
    int iItem = -1;

    LV_ITEMW rgItem;
    ZeroMemory ( &rgItem, sizeof( rgItem ));
    
    rgItem.mask = LVIF_PARAM;
    
    do
    {
        iItem = ListView_GetNextItem( hWnd, iItem, LVNI_ALL );

        if ( iItem == -1 )
            break;

        rgItem.iItem = iItem;
        rgItem.lParam = 0;
        
        if ( ListView_GetItemWrapW( hWnd, &rgItem))
        {
            Assert( rgItem.lParam != NULL );
            SHFree( (LPVOID ) rgItem.lParam );
        }
    }
    while ( iItem != -1 );

    ListView_DeleteAllItems( hWnd );
}


//////////////////////////////////////////////////////////////////////////////////////
UINT MergeMenus( HMENU hOriginal, HMENU hNew, UINT idStart, UINT idPos )
{
    UINT idMax = idStart;


    MENUITEMINFOW rgMenuItem;
    WCHAR szMenuItem[MAX_PATH];
    
    int iMenuSize = GetMenuItemCount( hOriginal );

    for (int iItem = GetMenuItemCount(hNew)-1; iItem >= 0; -- iItem)
    {
        rgMenuItem.cbSize = sizeof(rgMenuItem);
        rgMenuItem.fMask = MIIM_ID | MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
        rgMenuItem.dwTypeData = szMenuItem;
        rgMenuItem.cch = MAX_PATH; 
        rgMenuItem.dwItemData = 0;
        BOOL bRes = GetMenuItemInfoWrapW(hNew, iItem, TRUE, &rgMenuItem);
        if ( bRes == FALSE )
        {
            continue;
        }

        // only adjust the ID if it is not a submenu and is not a separator ...
        if (( rgMenuItem.fMask & MIIM_ID ) && 
            ( rgMenuItem.hSubMenu == NULL ) &&
            ( rgMenuItem.fType != MFT_SEPARATOR ))
        {
            rgMenuItem.wID += idStart;
            if ( idMax < rgMenuItem.wID )
            {
                idMax = rgMenuItem.wID;
            }
        }
        if ( rgMenuItem.hSubMenu != NULL )
        {
            // must fix up the ID's of the cascading items ...
            MENUITEMINFOW miiSub;

            // assume that there is only one level of cascading menu.....
            for ( int iSubItem = GetMenuItemCount(rgMenuItem.hSubMenu) -1; iSubItem >= 0; -- iSubItem )
            {
                miiSub.cbSize = sizeof( miiSub );
                miiSub.fMask = MIIM_ID | MIIM_TYPE;
                miiSub.cch = 0;
                miiSub.hSubMenu = NULL;
                miiSub.fType = 0;

                BOOL bRes = GetMenuItemInfoWrapW( rgMenuItem.hSubMenu, iSubItem, TRUE, &miiSub );
                if ( bRes == TRUE )
                {
                    // only adjust the menu id if it really is a menu item not a submenu or
                    // separator...
                    if (( miiSub.fMask & MIIM_ID ) &&
                        ( miiSub.hSubMenu == NULL ) &&
                        ( miiSub.fType != MFT_SEPARATOR ))
                    {
                        miiSub.wID += idStart;
                        if ( idMax < miiSub.wID )
                        {
                            idMax = miiSub.wID;
                        }

                        // we only want to change the ID
                        miiSub.fMask = MIIM_ID;
                        SetMenuItemInfoWrapW( rgMenuItem.hSubMenu, iSubItem, TRUE, & miiSub );
                    }
                }
            }
            // remove the menu from the original so that it is not destroyed later.....
            // otherwise there seems to be a menu ref count lying around and the opertunity
            // that when the original menu is destroyed it will destroy all the sub-menus
            // with it.
            RemoveMenu( hNew, iItem, MF_BYPOSITION );
        }
        
        InsertMenuItemWrapW( hOriginal, idPos, TRUE, &rgMenuItem );
    }

    return(idMax);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// add a separator to the menu in the position specified
void AddMenuSeparator( HMENU hMenu, int iIndex )
{
    AddMenuSeparatorWithID( hMenu, iIndex, 0 );
}

void AddMenuSeparatorWithID( HMENU hMenu, int iIndex, UINT iID )
{
    MENUITEMINFOW mii;

    mii.cbSize = sizeof( mii );
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.wID = iID;
    mii.fType = MFT_SEPARATOR;
    mii.cch = 0;
    InsertMenuItemWrapW( hMenu, iIndex, TRUE, &mii ); 
}

//////////////////////////////////////////////////////////////////////////////////////
HMENU LoadPopupMenu( HINSTANCE hDllInst, int iResID )
{
    HMENU hParent = LoadMenuA(hDllInst, (LPCSTR) MAKEINTRESOURCE(iResID));

    if (hParent) 
    {
        HMENU hpopup = GetSubMenu(hParent, 0);
        RemoveMenu(hParent, 0, MF_BYPOSITION);
        
        DestroyMenu(hParent);
        return hpopup;
    }

    return NULL;
}


//////////////////////////////////////////////////////////////////////////////////////
// get color resolution of the current display.
UINT GetCurColorRes( void )
{
    HDC hdc;
    UINT uColorRes;

    hdc = GetDC( NULL );
    uColorRes = GetDeviceCaps( hdc, PLANES ) * GetDeviceCaps( hdc, BITSPIXEL );
    ReleaseDC( NULL , hdc );

    return uColorRes;
}


//////////////////////////////////////////////////////////////////////////////////////
LPCITEMIDLIST * DuplicateIDArray( LPCITEMIDLIST * apidl, UINT cidl )
{
    LPCITEMIDLIST * apidlNew = (LPCITEMIDLIST *) LocalAlloc( LPTR, cidl * sizeof( LPCITEMIDLIST ));

    if ( apidlNew )
    {
        CopyMemory( apidlNew, apidl, cidl * sizeof( LPCITEMIDLIST ));
    }

    return apidlNew;
}

//////////////////////////////////////////////////////////////////////////////////////////
HRESULT SHCLSIDFromStringA( LPCSTR szCLSID, CLSID * pCLSID )
{
    WCHAR szWCLSID[50];

    MultiByteToWideChar( CP_ACP, 0, szCLSID, -1, szWCLSID, 50);
    return CLSIDFromString( szWCLSID, pCLSID );
}

//////////////////////////////////////////////////////////////////////////////////////////
HRESULT SHStringFromCLSIDA( LPSTR szCLSID, DWORD cSize, REFCLSID rCLSID )
{
    if ( cSize < 39 )
    {
        return E_INVALIDARG;
    }
    
    WCHAR szWCLSID[40];

    HRESULT hr = StringFromGUID2( rCLSID, szWCLSID, 40 );
    if ( FAILED( hr ))
    {
        return hr;
    }
    
    WideCharToMultiByte( CP_ACP, 0, szWCLSID, -1, szCLSID, cSize, 0, 0);
    return hr;
}

///////////////////////////////////////////////////////////////////
LPCITEMIDLIST FindLastPidl(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    if (pidl == NULL)
        return NULL;

    // Find the last one
    while (pidlNext->mkid.cb)
    {
        pidlLast = pidlNext;
        pidlNext = NextPIDL(pidlLast);
    }

    return (LPCITEMIDLIST)pidlLast;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT SimpleIDLISTToRealIDLIST( IShellFolder * psf, LPCITEMIDLIST pidlSimple, LPITEMIDLIST * ppidlReal )
{
    Assert( psf && pidlSimple && ppidlReal );

    STRRET rgStr;

    // check to make sure that we have a single level pidl
    LPCITEMIDLIST pidlNext = NextPIDL( pidlSimple );
    if ( pidlNext && pidlNext->mkid.cb != 0 )
    {
        return E_INVALIDARG;
    }
    
    HRESULT hres = psf->GetDisplayNameOf( pidlSimple, SHGDN_FORPARSING | SHGDN_INFOLDER, &rgStr );
    if ( SUCCEEDED( hres ))
    {
        WCHAR szPath[MAX_PATH];
        StrRetToBufW( &rgStr, pidlSimple, szPath, ARRAYSIZE(szPath) );

        hres = psf->ParseDisplayName( NULL, NULL, szPath, NULL, ppidlReal, NULL );
    }
    return hres;    
}

HRESULT Invoke_OnConnectionPointerContainer(IUnknown * punk, REFIID riidCP, DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr = S_OK;     // Assume no errors.
    IConnectionPointContainer * pcpc; 

    hr = punk->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pcpc);
    if (SUCCEEDED(hr))
    {
        IConnectionPoint * pcp;

        hr = pcpc->FindConnectionPoint(riidCP, &pcp);
        if (SUCCEEDED(hr))
        {
            IEnumConnections * pec;

            hr = pcp->EnumConnections(&pec);
            if (SUCCEEDED(hr))
            {
                CONNECTDATA cd;
                ULONG cFetched;

                while (S_OK == (hr = pec->Next(1, &cd, &cFetched)))
                {
                    LPDISPATCH pdisp;

                    Assert(1 == cFetched);
                    hr = cd.pUnk->QueryInterface(IID_IDispatch, (LPVOID *) &pdisp);
                    if (SUCCEEDED(hr))
                    {
                        DISPPARAMS dispparams = {0};
                        
                        if (!pdispparams)
                            pdispparams = &dispparams;

                        hr = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
                        pdisp->Release();
                    }
                    else
                        Assert(FALSE);              
                }
                pec->Release();
            }
            else
                Assert(FALSE);              

            pcp->Release();
        }
        else
            Assert(FALSE);              
        pcpc->Release();
    }
    else    
        Assert(FALSE);

    return hr;
}

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject)
{
    RECT    rc;
    POINT   pt;

    GetWindowRect(hwndTarget, &rc);
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragEnterEx2(hwndTarget, pt, pdtObject);
    return;
}

void _DragMove(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    POINT pt;

    GetWindowRect(hwndTarget, &rc);
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragMove(pt);
    return; 
}
