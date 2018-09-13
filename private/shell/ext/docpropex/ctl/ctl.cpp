//-------------------------------------------------------------------------//
// Ctl.cpp : Implementation of CPropertyTreeCtl ActiveX control interface
//-------------------------------------------------------------------------//
#include "pch.h"
#include "PropTree.h"
#include "PTsrv32.h"
#include "PTutil.h"
#include "PTsniff.h"
#include "TreeItems.h"
#include "Ctl.h"

//-------------------------------------------------------------------------//
//  IOleInPlaceActiveObject methods
//-------------------------------------------------------------------------//

HRESULT CPropertyTreeCtl::TranslateAccelerator( LPMSG pMsg )
{
    if( pMsg->message==WM_KEYDOWN )
    {
        HWND hwndFocus = ::GetFocus();
    
        if( hwndFocus==m_hWnd || IsChild( hwndFocus ) )
        {
            pMsg->hwnd = hwndFocus;
            TranslateMessage( pMsg );
            DispatchMessage( pMsg );
            return S_OK;
        }
    }

    return E_NOTIMPL;
}

//-------------------------------------------------------------------------//
//  IOleInPlaceObject::SetObjectRects filter
STDMETHODIMP CPropertyTreeCtl::SetObjectRects( LPCRECT prcPos, LPCRECT prcClip )
{
    //  Some containers will pass us a NULL for prcClip, 
    //  and ATL's impl won't accept that.
    RECT    rcClip; 
    LPCRECT prcClipToUse = prcClip;

    if( !prcPos )
        return E_POINTER;

    if( prcClip==NULL )
    {
        rcClip = *prcPos;
        prcClipToUse = &rcClip;
    }
    return IOleInPlaceObjectWindowlessImpl<CPropertyTreeCtl>::SetObjectRects( 
                prcPos, prcClipToUse );
}

//-------------------------------------------------------------------------//
//  Handles OLE drawing tasks
HRESULT CPropertyTreeCtl::OnDraw(ATL_DRAWINFO& di)
{
    UNREFERENCED_PARAMETER( di );
    //RepositionEditControl();
    return S_OK;
}

//-------------------------------------------------------------------------//
//  ISupportErrorInfo methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Implements standard COM error handling for the IPropertyTreeCtl interface.
STDMETHODIMP CPropertyTreeCtl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IPropertyTreeCtl,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if( InlineIsEqualGUID(*arr[i],riid ))
            return S_OK;
    }
    return S_FALSE;
}

//-------------------------------------------------------------------------//
//  IPropertyTreeCtl methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Adds a property source to the control
STDMETHODIMP CPropertyTreeCtl::AddSource( 
    const VARIANT* pvarSrc, 
    const VARIANT* pvarServer, 
    ULONG dwDisposition )
{
    HRESULT hr = E_FAIL;
    int     cFoldersCreated = 0,
            cPropsCreated   = 0;
    IAdvancedPropertyServer* pServer = NULL;

    UNREFERENCED_PARAMETER( dwDisposition );

    if( !pvarSrc )
        return E_POINTER;

    hr = ServerFromVariant( pvarServer, &pServer );

    if( FAILED( hr ) &&
        FAILED( (hr = CreateServerInstance( CLSID_PTDefaultServer32, &pServer )) ) )
            return hr;

    LPARAM               lParam = 0L;
    IEnumPROPFOLDERITEM*   pEnumFolders;
    IEnumPROPERTYITEM* pEnumProps;
    
    if( SUCCEEDED( (hr = pServer->AcquireAdvanced( pvarSrc, &lParam )) ) )
    {
        if( FAILED( (hr = m_srcs.Insert( pvarSrc, pServer, lParam )) ) )
        {
            pServer->ReleaseAdvanced( lParam, PTREL_SOURCE_REMOVED );
            pServer->Release();
            return hr;
        }

        //  Enumerate the server's folders for this source.
        if( SUCCEEDED( (hr = pServer->EnumFolderItems( lParam, &pEnumFolders )) ) )
        {
            PROPFOLDERITEM   folder;
            ULONG          cFolders =0;
            InitPropFolderItem( &folder );

            while( SUCCEEDED( pEnumFolders->Next( 1, &folder, &cFolders ) ) && cFolders==1 )
            {
                if( !IsValidPropFolderItem( &folder ) )
                {
                    ASSERT( FALSE );
                    continue;
                }

                if( SUCCEEDED( CreateFolder( pvarSrc, folder ) ) )
                {
                    cFoldersCreated++;
                    ULONG cFolderProps = 0L; // tally of property items for the folder

                    //  Enumerate folder's properties
                    if( SUCCEEDED( pServer->EnumPropertyItems( &folder, &pEnumProps )) )
                    {
                        PROPERTYITEM propitem;
                        ULONG          cProps = 0;
                        InitPropertyItem( &propitem );

                        while( SUCCEEDED( pEnumProps->Next( 1, &propitem, &cProps ) ) && cProps==1 )
                        {
                            if( !IsValidPropertyItem( &propitem ) )
                            {
                                ASSERT( FALSE );
                                ClearPropertyItem( &propitem );
                                continue;
                            }

                            BOOL bExisted        = FALSE;
                            CProperty* pProperty = NULL;

                            if( SUCCEEDED( CreateProperty( pvarSrc, propitem, &bExisted, &pProperty ) ) )
                            {
                                cPropsCreated++;
                                cFolderProps++;

                                //  Allow property to initialize his selection values
                                if( pProperty->SelectionValueCount() <= 0 )
                                    pProperty->InitializeSelectionValues();

                                //  Add selection values to the property
                                IEnumPROPVARIANT_DISPLAY* pEnumVals = NULL;
                                if( !bExisted && SUCCEEDED( pServer->EnumValidValues( &propitem, &pEnumVals ) ) )
                                {
                                    PROPVARIANT_DISPLAY var;
                                    ULONG               cVars = 0;
                                    
                                    while( SUCCEEDED( pEnumVals->Next( 1, &var, &cVars ) ) && cVars==1 )
                                    {
                                        if( propitem.puid.vt != var.val.vt )
                                        {
                                            TRACE( TEXT("Warning: data type mismatch encountered in property acquisition\n") );
                                            ClearPropertyItem( &propitem );
                                            ClearPropVariantDisplay( &var );
                                            continue;
                                        }
                                        pProperty->AddSelectionValue( &var.val, var.bstrDisplay );
                                        ClearPropVariantDisplay( &var );
                                    }
                                    pEnumVals->Release();
                                }
                            }

                            ClearPropertyItem( &propitem );
                            cProps = 0;
                        }
                        pEnumProps->Release();
                    }
                    
                    //  If the folder has no children, discard it
                    if( 0 == cFolderProps )
                        m_folders.DeleteKey( folder.pfid );

                }
                ClearPropFolderItem( &folder );
                cFolders = 0;
            }
            pEnumFolders->Release();
        }

        //  Finished enumerating; allow server to release resources.
        pServer->ReleaseAdvanced( lParam, 
                                  PTREL_DONE_ENUMERATING|PTREL_CLOSE_SOURCE );
    }

    if( cFoldersCreated || cPropsCreated )   
        ReconcileTreeItems( cFoldersCreated>0, cPropsCreated>0 );

    if( NULL == GetSelection() )
        SetDefaultSelection();

    pServer->Release();
    
    return hr;
}

//-------------------------------------------------------------------------//
//  Instantiates the server specified by pvarServer.
STDMETHODIMP CPropertyTreeCtl::ServerFromVariant( 
    const VARIANT* pvarServer, 
    IAdvancedPropertyServer** ppServer )
{
    HRESULT hr = E_NOINTERFACE;
    
    if( NULL == pvarServer )
        return hr;

    ASSERT( ppServer );
    *ppServer = NULL;

    if( VT_BSTR == pvarServer->vt )
    {
        CLSID clsid;
        if( FAILED( (hr = CLSIDFromString( pvarServer->bstrVal, &clsid )) ) )
            return hr;

        return CreateServerInstance( clsid, ppServer );

    }
    else if( VT_UNKNOWN == pvarServer->vt )
    {
        if( NULL == pvarServer->punkVal )
            return E_POINTER;

        return pvarServer->punkVal->QueryInterface( IID_IAdvancedPropertyServer, 
                                                    (LPVOID*)&ppServer );
    }
    
    return hr;
}

//-------------------------------------------------------------------------//
//  Retrieves the appropriate server for the indicated source file.
HRESULT CPropertyTreeCtl::FindServerForSource( 
    const VARIANT * pvarSrc, 
    IAdvancedPropertyServer** pppts )
{
    HRESULT hr = E_FAIL;
    LPCTSTR pszExt = NULL;
    USES_CONVERSION;
    ASSERT( pppts );
    ASSERT( pvarSrc );
    *pppts = NULL;

    if( VT_BSTR == pvarSrc->vt )
    {
        TCHAR   szPath[MAX_PATH];
        lstrcpyn( szPath, W2T( pvarSrc->bstrVal ), ARRAYSIZE(szPath) );
        pszExt = PathFindExtension( szPath );

        if( pszExt )
        {
            CLSID clsid;
            if( SUCCEEDED( (hr = GetPropServerClassForFile( szPath, TRUE, &clsid )) ) &&
                SUCCEEDED( (hr = CreateServerInstance( clsid, pppts )) ) )
            {
                
#ifdef _DEBUG       
                WCHAR szClsid[128];
                StringFromGUID2( clsid, szClsid, ARRAYSIZE(szClsid) );
                TRACE( TEXT("Using registered PropertyTreeServer %s for %s.\n"), 
                             W2T(szClsid), PathFindExtension( szPath ) );
#endif _DEBUG                    
            }
        }
    }

    if( !*pppts )
        //  No registered server class found; use default.
        hr = CreateServerInstance( CLSID_PTDefaultServer32, pppts );
	
    return hr;
}

//-------------------------------------------------------------------------//
//  Instantiates a server
STDMETHODIMP CPropertyTreeCtl::CreateServerInstance( 
    REFCLSID clsid, 
    IAdvancedPropertyServer** pppts )
{
    HRESULT hr;
    *pppts = NULL;

    //  Create server
    if( SUCCEEDED( (hr = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER,
                                           IID_IAdvancedPropertyServer, (void**)pppts )) ) )
    {
        if( !m_pDefaultServer && IsEqualCLSID( clsid, CLSID_PTDefaultServer32 ) )
        {
            m_pDefaultServer = *pppts;
            m_pDefaultServer->AddRef();
        }
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Removes a property source from the control
STDMETHODIMP CPropertyTreeCtl::RemoveSource(const VARIANT * pvarSrc, ULONG dwDisposition )
{
    CPropertyUID    puid;
    PTSERVER        serverInfo;
    CProperty       *pProperty;
    PFID            pfid;
    CPropertyFolder *pFolder;
    BOOL            bFoldersRemoved = FALSE,
                    bPropsRemoved = FALSE;
    HANDLE          hEnum;
    BOOLEAN         bEnum;
    

    if( !pvarSrc ) return E_POINTER;
    CComVariant  varSrc( *pvarSrc );

    //  Lookup source in the master dictionary.
    if( m_srcs.Lookup( &varSrc, serverInfo ) )
    {
        //  Instruct each property to remove the source.
        for( hEnum = m_properties.EnumFirst( puid, pProperty ), bEnum = TRUE;
             hEnum && bEnum;
             bEnum = m_properties.EnumNext( hEnum, puid, pProperty ) )
        {
            if( SUCCEEDED( pProperty->RemoveSource( &varSrc ) ) )
                bPropsRemoved = TRUE;
        }
        m_properties.EndEnum( hEnum );

        //  Instruct each folder to remove the source.
        for( hEnum = m_folders.EnumFirst( pfid, pFolder ), bEnum = TRUE;
             hEnum && bEnum;
             bEnum = m_folders.EnumNext( hEnum, pfid, pFolder ) )
        {
            if( SUCCEEDED( pFolder->RemoveSource( &varSrc ) ) )
                bFoldersRemoved = TRUE;
        }
        m_folders.EndEnum( hEnum );

        //  Remove entry from the source dictionary
        m_srcs.Delete( pvarSrc );  // remove from sources dictionary.

        if( m_srcs.Count()==0 )
            RemoveAllSources( dwDisposition );
        else
            //  Merge and reconcile the tree items.
            ReconcileTreeItems( bFoldersRemoved, bPropsRemoved );
    }
	return S_OK;
}

//-------------------------------------------------------------------------//
//  Removes all property sources from the control.
STDMETHODIMP CPropertyTreeCtl::RemoveAllSources(ULONG dwDisposition)
{
    UNREFERENCED_PARAMETER( dwDisposition );
    
    TreeView_SelectItem( m_wndTree, NULL ); // this hides the edit control
    TreeView_DeleteAllItems( m_wndTree );   // clear tree items
    m_properties.Clear();           // Destroy property items.
    m_folders.Clear();             // Destroy folder items.
    m_srcs.Clear();                         // sweep sources dictionary.

    PendRedraw( m_wndTree );
    UpdateEmptyStatus();
	return S_OK;
}


//-------------------------------------------------------------------------//
//  Creates, initializes and maps a new property folder element.
//  Returns S_OK if the folder was successfully created, S_FALSE if the
//  folder already exists and was updated, or an error code
//  describing a failure.
HRESULT CPropertyTreeCtl::CreateFolder( 
    IN const VARIANT* pvarSrc,
    IN const PROPFOLDERITEM& item, 
    OUT CPropertyFolder** ppFolder )
{
    CPropertyFolder* pFolder = NULL;
    HRESULT          hr = S_OK;
    if( !ppFolder )  ppFolder = &pFolder;

    //  Ensure that the folder hasn't already been created
    if( m_folders.Lookup( item.pfid, *ppFolder ) )
    {
        if( SUCCEEDED( (hr = (*ppFolder)->AddSource( pvarSrc, item.dwAccess )) ) )
            return S_FALSE;
    }
    else
    {
        //  Folder is new to the tree;
        //  allocate, initialize and map.
        if( (*ppFolder = new CPropertyFolder( this, item ))!=NULL )
        {
            (*ppFolder)->Initialize();
            if( SUCCEEDED( (hr = (*ppFolder)->AddSource( pvarSrc, item.dwAccess )) ) )
                m_folders[item.pfid] = *ppFolder;
            else
            {
                delete *ppFolder;
                *ppFolder = NULL;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Creates, initializes and maps a new property element.
//  Returns S_OK if the property was successfully created, S_FALSE if the
//  property already exists and was updated, or an error code
//  describing a failure.
HRESULT CPropertyTreeCtl::CreateProperty( 
    IN const VARIANT* pvarSrc,
    IN const PROPERTYITEM& item, 
    OUT LPBOOL pbExisted, 
    OUT CProperty** ppProperty )
{
    CPropertyUID    puid( item.puid ); //  dictionary search key
    PROPERTYITEM    propitem;              //  working PROPERTYITEM.
    CProperty*      pProperty = NULL;      
    HRESULT         hr = E_FAIL;

    if( !ppProperty ) ppProperty = &pProperty;

    InitPropertyItem( &propitem );
    CopyPropertyItem( &propitem, &item );

    if( pbExisted ) *pbExisted = FALSE;
    
    //  Ensure that the folder hasn't already been created
    if( m_properties.Lookup( puid, *ppProperty ) )
    {
        //  Yes it has...
        if( pbExisted ) *pbExisted = TRUE;
        if( SUCCEEDED( (hr = (*ppProperty)->AddSource( pvarSrc, propitem.dwAccess, &propitem.val )) ) )
            hr = S_FALSE;
    }
    else
    {
        //  Property is new to the tree; allocate and initialize.
        hr = S_OK;

        switch( item.ctlID )
        {
	        case PTCTLID_DROPDOWN_COMBO:
	        case PTCTLID_DROPLIST_COMBO:
                switch( propitem.dwFlags & PTPIF_TYPEMASK )
                {
                    case PTPIF_BASIC:
                        *ppProperty = (CProperty*)new CComboBoxProperty( this, propitem );
                        break;
                    
                    case PTPIF_MRU:
                        *ppProperty = (CProperty*)new CMruProperty( this, propitem );
                        break;

                    case PTPIF_ENUM:
                        propitem.ctlID = PTCTLID_DROPLIST_COMBO; // enforce this.
                        *ppProperty = (CProperty*)new CEnumProperty( this, propitem );
                        break;

                    default: 
                        hr = ERROR_INVALID_FLAGS;
                }
                break;

	         case PTCTLID_CALENDARTIME:
	         case PTCTLID_CALENDAR:
	         case PTCTLID_TIME:
                *ppProperty = (CProperty*)new CDateProperty( this, propitem );
                break;

             case PTCTLID_MULTILINE_EDIT:
             case PTCTLID_SINGLELINE_EDIT:
                *ppProperty = (CProperty*)new CTextProperty( this, propitem );
                break;
        }


        if( SUCCEEDED( hr ) )
        {
            if( NULL != *ppProperty )
            {
                //  Initialize and add to the dictionary
                (*ppProperty)->Initialize();
                
                if( SUCCEEDED( (hr = (*ppProperty)->AddSource( pvarSrc, propitem.dwAccess, &propitem.val )) ) )
                    m_properties[puid] = *ppProperty;
                else
                {
                    delete *ppProperty;
                    *ppProperty = NULL;
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }

    ClearPropertyItem( &propitem );
    return hr;
}

//-------------------------------------------------------------------------//
//  Causes each folder and property treeitem to evaluate whether it should 
//  be displayed in the tree view, and either insert or remove itself accordingly.
void CPropertyTreeCtl::ReconcileTreeItems( BOOL bFolders, BOOL bProperties )
{
    HANDLE           hEnum;
    BOOLEAN          bEnum;
    int              cSources = m_srcs.Count();
    HTREEITEM        hFirstFolder = NULL;
    BOOL             bFoldersAdded = FALSE,
                     bPropsAdded = FALSE;

    if( bFolders )
    {
        PFID             pfid;
        CPropertyFolder* pFolder;
        for( hEnum = m_folders.EnumFirst( pfid, pFolder ), bEnum = TRUE;
             hEnum && bEnum;
             bEnum = m_folders.EnumNext( hEnum, pfid, pFolder ) )
        {
            ASSERT( pFolder );
            if( pFolder->Reconcile( m_wndTree, cSources ) )
            {
                bFoldersAdded = TRUE;
            }
            if( hFirstFolder==NULL )
                hFirstFolder = (HTREEITEM)*pFolder;
        }
        m_folders.EndEnum( hEnum );
    }

    if( bProperties )
    {
        CPropertyUID     puid;
        CProperty*       pProperty;
        int              i, cProps = m_properties.Count();
        for( hEnum = m_properties.EnumFirst( puid, pProperty ), bEnum = TRUE, i=0;
             hEnum && bEnum;
             bEnum = m_properties.EnumNext( hEnum, puid, pProperty ), i++ )
        {
            ASSERT( pProperty );
            
            //TRACE( TEXT("CPropertyTreeCtl - Reconciling '%s' (%d of %d)\n"), pProperty->GetName(), i+1, cProps );

            if( pProperty->Reconcile( m_wndTree, cSources ) )
                bPropsAdded = TRUE;
        }
        m_properties.EndEnum( hEnum );
    }

    if( bFoldersAdded && hFirstFolder )
        Sort( hFirstFolder, 0, FALSE );
    
    if( bPropsAdded )
    {
        //  For each folder, determine whether a child sort is pending.
        for( HTREEITEM hFolder = TreeView_GetChild( m_wndTree, TVI_ROOT );
             hFolder;
             hFolder = TreeView_GetNextSibling( m_wndTree, hFolder ) )
        {
            CPropertyTreeItem* pFolder;
            HTREEITEM          hChild;
            
            if( (pFolder = GetTreeItem( hFolder ))!=NULL )
            {
                if( (hChild = TreeView_GetChild( m_wndTree, hFolder)) != NULL )
                {
                    if( pFolder->ChildSortPending() )
                    {
                        //  Sort the children.
                        Sort( hChild, 0, FALSE );
                    }
                }
                else // no tree children; the folder shouldn't be displayed (NT raid# 300280)
                    TreeView_DeleteItem( m_wndTree, hFolder );
                
                //  clear the child-sort-pending bit.
                pFolder->UnpendChildSort();
            }
        }
    }
    
    PendRedraw( m_wndTree );
    UpdateEmptyStatus();
}

//-------------------------------------------------------------------------//
HRESULT CPropertyTreeCtl::GetServerForSource( 
    IN LPCOMVARIANT pvarSrc, 
    OUT PTSERVER& server ) const
{
    ASSERT( pvarSrc );
    return m_srcs.Lookup( pvarSrc, server ) ? S_OK : E_FAIL;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::Apply()
{
	TV_ITEM     tvi;
    HTREEITEM   hFolder;
    HRESULT     hr = S_FALSE;

    memset( &tvi, 0, sizeof(tvi) );
    tvi.mask = TVIF_PARAM|TVIF_HANDLE;

    //  Instruct each dirty property to apply changes
    for( hFolder = TreeView_GetChild( m_wndTree, TVI_ROOT );
         hFolder;
         hFolder = TreeView_GetNextSibling( m_wndTree, hFolder ) )
    {
        for( tvi.hItem = TreeView_GetChild( m_wndTree, hFolder );
             tvi.hItem;
             tvi.hItem = TreeView_GetNextSibling( m_wndTree, tvi.hItem ) )
        {
            tvi.lParam = 0L;    
            if( TreeView_GetItem( m_wndTree, &tvi ) )
            {
                CProperty* pProperty;
                if( (pProperty = (CProperty*)tvi.lParam)!=NULL )
                {
                    ASSERT( pProperty->Type()==PIT_PROPERTY );
                    if( pProperty->IsDirty() )
                    {
                        if( FAILED( (hr = pProperty->Apply()) ) )
                            return hr;
                    }
                }
            }
        }
    }
    Release( PTREL_DONE_ENUMERATING|PTREL_CLOSE_SOURCE );

	return hr;
}

//-------------------------------------------------------------------------//
HRESULT CPropertyTreeCtl::Release( ULONG dwFlags )
{
    HANDLE hEnum;
    BOOLEAN bEnum = TRUE;
    LPCOMVARIANT pvarSrc;
    PTSERVER    server;
    HRESULT hr = S_OK;

    for( hEnum = m_srcs.EnumFirst( pvarSrc, server );
         hEnum && bEnum;
         bEnum = m_srcs.EnumNext(  hEnum, pvarSrc, server ) )
    {
        HRESULT hrSrc = server.pServer->ReleaseAdvanced( server.lParamSrc, dwFlags );
        if( FAILED( hrSrc ) )
            hr = hrSrc;
    }
    m_srcs.EndEnum( hEnum );

	return hr;
}

//-------------------------------------------------------------------------//
//  IPropertyTreeCtl::SetPropertyValue
//  
//  Retrives the value of the specified property.
HRESULT CPropertyTreeCtl::GetPropertyValue( 
    BSTR bstrFmtID, 
    LONG nPropID, 
    LONG nDataType,
    BSTR* pbstrVal, 
    VARIANT_BOOL* pbDirty )
{
    HRESULT         hr = E_UNEXPECTED;
    CPropertyUID    puid;
    CProperty*      pProperty;

    if( NULL == bstrFmtID || 0 == *bstrFmtID || nPropID < 0x02 || NULL == pbstrVal )
        return E_INVALIDARG;

    *pbstrVal = NULL;
    if( pbDirty ) *pbDirty = false;

    if( FAILED( (hr = CLSIDFromString( bstrFmtID, &puid.fmtid )) ) )
        return hr;

    puid.propid = nPropID;
    puid.vt     = (VARTYPE)nDataType;

    if( m_properties.Lookup( puid,  pProperty ) && NULL != pProperty )
    {
        USES_CONVERSION;
        *pbstrVal = SysAllocString( T2W( (LPTSTR)pProperty->ValueText() ) );
        if( pbDirty )
            *pbDirty = (VARIANT_BOOL)pProperty->IsDirty();
        
        hr = S_OK;
    }
    else
        hr = E_FAIL;

    return hr;
}

//-------------------------------------------------------------------------//
//  IPropertyTreeCtl::SetPropertyValue
//  
//  Assigns a new value to the specified property.
HRESULT CPropertyTreeCtl::SetPropertyValue( 
    BSTR bstrFmtID, 
    LONG nPropID, 
    LONG nDataType,
    BSTR bstrVal, 
    VARIANT_BOOL bMakeDirty )
{
    HRESULT         hr = E_UNEXPECTED;
    CPropertyUID    puid;
    CProperty*      pProperty;

    if( NULL == bstrFmtID || 0 == *bstrFmtID || 
        nPropID < 0x02 )
        return E_INVALIDARG;

    if( FAILED( (hr = CLSIDFromString( bstrFmtID, &puid.fmtid )) ) )
        return hr;

    puid.propid = nPropID;
    puid.vt     = (VARTYPE)nDataType;

    if( m_properties.Lookup( puid,  pProperty ) && NULL != pProperty )
        hr = pProperty->AssignValueFromText( bstrVal, bMakeDirty );
    else
        hr = E_FAIL;

    return hr;
}

//-------------------------------------------------------------------------//
//  IPropertyTreeCtl::IsPropertyDirty
//  
//  Reports whether the value of the specified property has been 
//  modified without being persisted.
STDMETHODIMP CPropertyTreeCtl::IsPropertyDirty(
    /*[in]*/ BSTR bstrFmtID, 
    /*[in]*/ LONG nPropID, 
    /*[in]*/ LONG nDataType, 
    /*[out]*/ VARIANT_BOOL* pbDirty )
{
    HRESULT hr;
    CPropertyUID    puid;
    CProperty*      pProperty;

    if( !pbDirty ) return E_POINTER;
    *pbDirty = VARIANT_FALSE;

    if( FAILED( (hr = CLSIDFromString( bstrFmtID, &puid.fmtid )) ) )
        return hr;

    puid.propid = nPropID;
    puid.vt     = (VARTYPE)nDataType;

    if( m_properties.Lookup( puid,  pProperty ) && NULL != pProperty )
    {
        USES_CONVERSION;
        *pbDirty = pProperty->IsDirty() ? VARIANT_TRUE : VARIANT_FALSE;
        hr = S_OK;
    }
    else
        hr = E_FAIL;

    return hr;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_NoSourcesCaption( BSTR* pVal )
{
    if( !pVal ) return E_POINTER;

    *pVal = NULL;
    if( m_bstrNoSourcesCaption )
    {
        if( (*pVal = SysAllocString( m_bstrNoSourcesCaption ))==NULL )
            return E_OUTOFMEMORY;
    }

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::put_NoSourcesCaption( BSTR newVal )
{
	if( m_bstrNoSourcesCaption )
    {
        SysFreeString( m_bstrNoSourcesCaption );
        m_bstrNoPropertiesCaption = NULL;
    }

    m_bstrNoSourcesCaption = newVal;
	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_NoPropertiesCaption( BSTR* pVal )
{
    if( !pVal ) return E_POINTER;

    *pVal = NULL;
    if( m_bstrNoPropertiesCaption )
    {
        if( (*pVal = SysAllocString( m_bstrNoPropertiesCaption ))==NULL )
            return E_OUTOFMEMORY;
    }

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::put_NoPropertiesCaption( BSTR newVal )
{
	if( m_bstrNoPropertiesCaption )
    {
        SysFreeString( m_bstrNoPropertiesCaption );
        m_bstrNoPropertiesCaption = NULL;
    }

    m_bstrNoPropertiesCaption = newVal;
	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_NoCommonsCaption( BSTR* pVal )
{
    if( !pVal ) return E_POINTER;

    *pVal = NULL;
    if( m_bstrNoCommonsCaption && (*pVal = SysAllocString( m_bstrNoCommonsCaption ))==NULL )
        return E_OUTOFMEMORY;

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::put_NoCommonsCaption( BSTR newVal )
{
	if( m_bstrNoCommonsCaption )
    {
        SysFreeString( m_bstrNoCommonsCaption );
        m_bstrNoCommonsCaption = NULL;
    }

    m_bstrNoCommonsCaption = newVal;
	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_DirtyCount( long * pVal )
{
    HRESULT hr;
    long    cDirty, cDirtyVis;

    if( !pVal ) 
        return E_POINTER;

    if( SUCCEEDED( (hr = GetDirtyCount( cDirty, cDirtyVis )) ) )
        *pVal = cDirty;
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT CPropertyTreeCtl::GetDirtyCount( OUT LONG& cDirty, OUT LONG& cDirtyVis )
{
	HANDLE       hEnum;
    BOOLEAN      bEnum;
    CPropertyUID puid;
    CProperty*   pProperty;

    cDirty = cDirtyVis = 0;

    for( hEnum = m_properties.EnumFirst( puid, pProperty ), bEnum = TRUE;
         hEnum && bEnum;
         bEnum = m_properties.EnumNext( hEnum, puid, pProperty ) )
    {
        ASSERT( pProperty );
        if( pProperty->IsDirty() )
        {
            cDirty++;
            if( ((HTREEITEM)(*pProperty)) != NULL )
                cDirtyVis++;
        }
    }
    m_properties.EndEnum( hEnum );

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_FolderCount(long * pVal)
{
	if( !pVal ) return E_POINTER;
    *pVal = m_folders.Count();
	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_PropertyCount(long * pVal)
{
	if( !pVal ) return E_POINTER;
    *pVal = m_properties.Count();
	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_FolderCountVisible(long * pVal)
{
	HANDLE           hEnum;
    BOOLEAN          bEnum;
    PFID             pfid;
    CPropertyFolder* pFolder;

    if( !pVal ) return E_POINTER;

    *pVal = 0;
    for( hEnum = m_folders.EnumFirst( pfid, pFolder ), bEnum = TRUE;
         hEnum && bEnum;
         bEnum = m_folders.EnumNext( hEnum, pfid, pFolder ) )
    {
        ASSERT( pFolder );
        if( (HTREEITEM)*pFolder )
            (*pVal)++;
    }
    m_folders.EndEnum( hEnum );

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_PropertyCountVisible(long * pVal)
{
	HANDLE           hEnum;
    BOOLEAN          bEnum;
    CPropertyUID     puid;
    CProperty*       pProperty;

    if( !pVal ) return E_POINTER;

    *pVal = 0;
    for( hEnum = m_properties.EnumFirst( puid, pProperty ), bEnum = TRUE;
         hEnum && bEnum;
         bEnum = m_properties.EnumNext( hEnum, puid, pProperty ) )
    {
        ASSERT( pProperty );
        if( (HTREEITEM)*pProperty )
            (*pVal)++;
    }
    m_properties.EndEnum( hEnum );

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_Empty(BOOL * pVal)
{
    if( !pVal ) return E_POINTER;
	*pVal = TreeView_GetCount( m_wndTree )==0 &&
            m_folders.Count()==0 &&
            m_properties.Count()==0;

	return S_OK;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CPropertyTreeCtl::get_EmptyVisible(BOOL * pVal)
{
    if( !pVal ) return E_POINTER;
    *pVal = TreeView_GetCount( m_wndTree )==0;

	return S_OK;
}
