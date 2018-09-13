#include "precomp.h"

WCHAR const c_szTVerb[] = L"CaptureThumbnail";
CHAR const c_szVerb[] = "CaptureThumbnail";

/////////////////////////////////////////////////////////////////////////////////////////////
CThumbnailMenu::CThumbnailMenu()
{
    m_pMenu = NULL;
    m_pMenu2 = NULL;
    m_pView = NULL;
    m_apidl = NULL;
    m_cidl = NULL;
    m_fCaptureAvail = FALSE;
    m_wID = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CThumbnailMenu::~CThumbnailMenu()
{
    if ( m_pMenu )
    {
        m_pMenu->Release();
    }

    if ( m_pMenu2 )
    {
        m_pMenu2->Release();
    }

    if ( m_pView )
    {
        m_pView->InternalRelease();
    }
    
    if ( m_apidl )
    {
        LocalFree( m_apidl );
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbnailMenu::Init( CThumbnailView * pView,
                              UINT * prgfFlags,
                              LPCITEMIDLIST * apidl,
                              UINT cidl )
{
    if ( pView == NULL || apidl == NULL || cidl == 0 )
    {
        return E_INVALIDARG;
    }
    
    m_pView = pView;
    pView->InternalAddRef();
    
    // duplicate the array that holds the pointers ..
    m_apidl = DuplicateIDArray( apidl, cidl );
    m_cidl  = cidl;

    // scan the pidl array and check for Extractors ...
    for ( int iPidl = 0; iPidl < (int) m_cidl; iPidl ++ )
    {
        LPEXTRACTIMAGE pExtract = NULL;
        UINT rgfFlags = 0;
        HRESULT hr = pView->m_pFolder->GetUIObjectOf( pView->m_hWnd,
                                                      1,
                                                      m_apidl + iPidl,
                                                      IID_IExtractImage,
                                                      &rgfFlags,
                                                      (LPVOID *) &pExtract );
        if ( SUCCEEDED( hr ))
        {
            WCHAR szPath[MAX_PATH];
            DWORD dwFlags = 0;
            SIZE rgThumbSize = {pView->m_iXSizeThumbnail, pView->m_iYSizeThumbnail};

            hr = pExtract->GetLocation( szPath, MAX_PATH, NULL, &rgThumbSize, pView->m_dwRecClrDepth, &dwFlags );
            pExtract->Release();
            if ( dwFlags & IEIFLAG_CACHE )
            {
                m_fCaptureAvail = TRUE;
                break;
            }
        }
        else
        {
            // blank it out so we don't bother trying it if the user choses the command
            m_apidl[iPidl] = NULL;
        }
    }
    
    HRESULT hr = pView->m_pFolder->GetUIObjectOf( pView->m_hWnd, cidl, apidl, IID_IContextMenu,
                                           prgfFlags, (LPVOID *) & m_pMenu );
     if ( SUCCEEDED( hr ))
     {
        m_pMenu->QueryInterface( IID_IContextMenu2, (LPVOID *) & m_pMenu2 );
     }

     return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailMenu::QueryContextMenu ( HMENU hmenu,
                                                UINT indexMenu,
                                                UINT idCmdFirst,
                                                UINT idCmdLast,
                                                UINT uFlags )
{
    Assert( m_pMenu != NULL );
    
    // generate the proper menu 
    HRESULT hr = m_pMenu->QueryContextMenu( hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags );
    if ( SUCCEEDED( hr ) && m_fCaptureAvail )
    {
        // find the first separator and insert the menu text after it....
        int cMenuSize = GetMenuItemCount( hmenu );
        for ( int iIndex = 0; iIndex < cMenuSize; iIndex ++ )
        {
            WCHAR szText[80];
            MENUITEMINFOW mii;
            mii.cbSize = sizeof( mii );
            mii.fMask = MIIM_TYPE;
            mii.fType = 0;
            mii.dwTypeData = szText;
            mii.cch = 80;

            GetMenuItemInfoWrapW( hmenu, iIndex, TRUE, &mii );
            if ( mii.fType & MFT_SEPARATOR )
            {

                szText[0] = 0;
                LoadStringWrapW( g_hinstDll, IDS_CREATETHUMBNAIL, szText, 80 );
                
                mii.fMask = MIIM_ID | MIIM_TYPE;
                mii.fType = MFT_STRING;
                mii.dwTypeData = szText;
                mii.cch = 0;

                // assuming 0 is the first id, therefore the next one = the count they returned
                m_wID = HRESULT_CODE( hr );
                mii.wID = idCmdFirst + m_wID;

                InsertMenuItemWrapW( hmenu, iIndex, TRUE, & mii );

                // we used an extra ID.
                hr ++;
                
                break;
            }
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailMenu::InvokeCommand( LPCMINVOKECOMMANDINFO lpici )
{
    HRESULT hr = E_FAIL;
    
    Assert( m_pMenu != NULL );

    if ( lpici->lpVerb != (LPCSTR) m_wID )
    {
        hr = m_pMenu->InvokeCommand( lpici );
    }
    else
    {
        // capture thumbnails .....
        for ( UINT iPidl = 0; iPidl < m_cidl; iPidl ++ )
        {
            if ( m_apidl[iPidl] != NULL )
            {
                m_pView->ExtractItem( NULL, -1, m_apidl[iPidl], FALSE, TRUE );
            }
        }
    }
    return hr;    
}

/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailMenu::GetCommandString( UINT_PTR idCmd,
                                               UINT uType,
                                               UINT * pwReserved,
                                               LPSTR pszName,
                                               UINT cchMax )
{
    if ( !IS_INTRESOURCE( idCmd ))
    {
        // it is really a text verb ...
        LPSTR pszCommand = (LPSTR) idCmd;
        if ( lstrcmpA( pszCommand, c_szVerb ) == 0 )
        {
            return NOERROR;
        }
    }
    else
    {
        if ( idCmd == m_wID )
        {
            // it is ours ...
            switch( uType )
            {
                case GCS_VERBA:
                    StrCpyNA(( LPSTR ) pszName, "CaptureThumbnail", cchMax );
                    break;
                case GCS_VERBW:
                    StrCpyNW(( LPWSTR ) pszName, L"CaptureThumbnail", cchMax );
                    break;
                    
                case GCS_HELPTEXTW:
                    LoadStringWrapW( g_hinstDll, IDS_CREATETHUMBNAILHELP, ( LPWSTR ) pszName, cchMax );
                    break;
                case GCS_HELPTEXTA:
                    LoadStringA( g_hinstDll, IDS_CREATETHUMBNAILHELP, ( LPSTR ) pszName, cchMax );
                    break;
                    
                case GCS_VALIDATE:
                    pszName[0] = 0;
            }

            return NOERROR;
        }
    }
    return m_pMenu->GetCommandString( idCmd, uType, pwReserved, pszName, cchMax );
}

STDMETHODIMP CThumbnailMenu::HandleMenuMsg2 ( UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam,
                                             LRESULT* plRes)
{
    HRESULT hres = E_NOTIMPL;
    if (uMsg == WM_MENUCHAR && m_pMenu2)
    {
        IContextMenu3* pcm3;
        hres = m_pMenu2->QueryInterface(IID_IContextMenu3, (void**)&pcm3);
        if (SUCCEEDED(hres))
        {
            hres = pcm3->HandleMenuMsg2 ( uMsg, wParam, lParam, plRes);
            pcm3->Release();
        }
    }
    else
    {
        hres = HandleMenuMsg(uMsg, wParam, lParam);
    }

    return hres;
}

                             
/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbnailMenu::HandleMenuMsg ( UINT uMsg,
                                             WPARAM wParam,
                                             LPARAM lParam )
{
    if ( m_pMenu2 == NULL )
    {
        return E_NOTIMPL;
    }
    
    switch( uMsg )
    {
        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;

            if ( pdi->CtlType == ODT_MENU && pdi->itemID == m_wID ) 
            {
                return E_NOTIMPL;
            }
        }
        break;

    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;

            if ( pmi->CtlType == ODT_MENU && pmi->itemID == m_wID ) 
            {
                return E_NOTIMPL;
            }
        }
        break;
    }
    return m_pMenu2->HandleMenuMsg( uMsg, wParam, lParam );
}
