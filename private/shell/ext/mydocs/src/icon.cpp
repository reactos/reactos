/*----------------------------------------------------------------------------
/ Title;
/   icon.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   IExtractIcon implementation for My Documents.
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ CMyDocsExtractIcon
/----------------------------------------------------------------------------*/
CMyDocsExtractIcon::CMyDocsExtractIcon( IShellFolder * psf,
                                        HWND hwndOwner,
                                        LPCITEMIDLIST pidl,
                                        folder_type type
                                       )
{
    HRESULT hr = E_FAIL;

    MDTraceEnter(TRACE_ICON, "CMyDocsExtractIcon::CMyDocsExtractIcon");

    m_psf = NULL;
    m_pei = NULL;
    m_pidl = ILClone(pidl);
    m_type = type;

    if ( (m_type == FOLDER_IS_ROOT) ||
         (m_type == FOLDER_IS_ROOT_PATH) ||
         (m_type == FOLDER_IS_UNBLESSED_ROOT_PATH) ||
         ILIsEmpty( m_pidl ))
    {
        hr = S_OK;
        MDTrace( TEXT("It's the root item (blessed or unblessed)") );
    }
    else if (!MDIsSpecialIDL( m_pidl ))
    {
        TCHAR szPath[ MAX_PATH ];
        WCHAR szItem[ MAX_PATH ];
        ULONG ulEaten;
        LPTSTR pFileName;
        LPITEMIDLIST pidlLast;
        LPITEMIDLIST pidlItem;

        m_psf = psf;
        m_psf->AddRef();

        //
        // Get shell's idlist from ours...
        //

        pidlItem = SHIDLFromMDIDL( (LPITEMIDLIST)m_pidl );
        if (!pidlItem)
            ExitGracefully( hr, E_FAIL, "couldn't get shell IDL from MDIDL" );

        pidlLast = ILFindLastID( pidlItem );

        if (!SHGetPathFromIDList( pidlLast, szPath ) )
            ExitGracefully( hr, E_FAIL, "couldn't get path from PIDL" );

        MDTrace( TEXT("Path we're being called for is '%s'"), szPath );

        pFileName = PathFindFileName( szPath );

        MDTrace( TEXT("File we're being called for is '%s'"), pFileName );

        SHTCharToUnicode(pFileName, szItem, ARRAYSIZE(szItem));

        hr = m_psf->ParseDisplayName( hwndOwner,
                                      NULL,
                                      szItem,
                                      &ulEaten,
                                      &pidlItem,
                                      NULL
                                     );
        FailGracefully( hr, "Couldn't ParseDisplayName");

        hr = m_psf->GetUIObjectOf( hwndOwner,
                                   1,
                                   (LPCITEMIDLIST *)&pidlItem,
                                   IID_IExtractIcon,
                                   NULL,
                                   (void **)&m_pei
                                  );
#ifdef UNICODE
        m_fAnsiExtractIcon = FALSE;

        //
        // Have to deal with the possibility that the item in question
        // only supports IExtractIconA.
        //

        if (FAILED(hr)) {
            m_fAnsiExtractIcon = TRUE;

            MDTrace(TEXT("IExtractIconW failed...Trying IExtractIconA..."));
            hr = m_psf->GetUIObjectOf(
                hwndOwner,
                1,
                (LPCITEMIDLIST *)&pidlItem,
                IID_IExtractIconA,
                NULL,
                (void **)&m_pei
            );

        } // if
#endif // UNICODE        

        DoILFree( pidlItem );

        FailGracefully( hr, "Couldn't get IExtractIcon UI Object");
    }
    else
    {
        //
        // At this point, it should be a special item...
        //
        MDTraceAssert(MDIsSpecialIDL( m_pidl ));

        hr = S_OK;
    }

exit_gracefully:

    MDTraceLeaveResultNoRet(hr);
}


CMyDocsExtractIcon::~CMyDocsExtractIcon()
{
    DoILFree( m_pidl );
    DoRelease( m_pei );
    DoRelease( m_psf );
}


// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CMyDocsExtractIcon
#include "unknown.inc"

STDMETHODIMP CMyDocsExtractIcon::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IExtractIcon, (LPEXTRACTICON)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IExtractIcon methods
/----------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsExtractIcon::GetIconLocation( UINT uFlags,
                                     LPTSTR szIconFile,
                                     UINT cchMax,
                                     int* pIndex,
                                     UINT* pwFlags
                                    )
{
    HRESULT hr = E_FAIL;

    MDTraceEnter(TRACE_ICON, "CMyDocsExtractIcon::GetIconLocation");


    if ( ILIsEmpty( m_pidl ) ||
         (m_type == FOLDER_IS_ROOT) ||
         (m_type == FOLDER_IS_ROOT_PATH)
        )
    {
        MDTrace( TEXT("It's the root item") );

        ManageMyDocsIconPathAndIndex( szIconFile, cchMax, pIndex, FALSE );
        hr = S_OK;
    }
    else if (m_type == FOLDER_IS_UNBLESSED_ROOT_PATH)
    {
        MDTrace( TEXT("It's an unblessed My Docs Folder") );

        if (cchMax >= 12)   // 12 == lstrlen("Shell32.dll")+1
        {
            lstrcpy( szIconFile, TEXT("shell32.dll") );
            if (pIndex)
            {
                *pIndex = II_FOLDER;
            }

            hr = S_OK;
        }
    }
    else
    {
        hr = MDGetIconInfoFromIDL( m_pidl, szIconFile, cchMax, (UINT *)pIndex );
        if (SUCCEEDED(hr))
        {
            if (pwFlags)
            {
                *pwFlags = 0;
            }

            ExitGracefully( hr, S_OK, "It's a special item" );
        }

        if ( !m_pei )
            ExitGracefully(hr, E_FAIL, "m_pei undefined!");

        //
        // call shell for this one...
        //

        hr = m_pei->GetIconLocation( uFlags, szIconFile, cchMax, pIndex, pwFlags );

#ifdef UNICODE
        //
        // Have to deal with IExtractIconA possibility
        //
        if (m_fAnsiExtractIcon) {
            LPTSTR szBuf = (LPTSTR) LocalAlloc(LPTR, cchMax * sizeof(TCHAR));
            if (!szBuf) {
                ExitGracefully(hr, E_OUTOFMEMORY,  "unable to allocate string buffer");
            } // if

            SHAnsiToTChar(
                (LPSTR) szIconFile,
                szBuf,
                cchMax
            );

            StrCpyN(szIconFile, szBuf, cchMax);

            LocalFree((HLOCAL)szBuf);
        } // if (m_fAnsiExtractIcon)
#endif // UNICODE

        
    }

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/
STDMETHODIMP
CMyDocsExtractIcon::Extract( LPCTSTR pszIconFile,
                             UINT nIconIndex,
                             HICON* pLargeIcon,
                             HICON* pSmallIcon,
                             UINT nIconSize
                            )
{
    HRESULT hr = E_NOTIMPL;

    MDTraceEnter(TRACE_ICON, "CMyDocsExtractIcon::Extract");

    MDTraceAssert(pLargeIcon);
    MDTraceAssert(pSmallIcon);

    if ( MDIsSpecialIDL( m_pidl ) || (!m_pei) )
    {
        hr = SHDefExtractIcon( pszIconFile, nIconIndex, 0, pLargeIcon, pSmallIcon, nIconSize );
        FailGracefully( hr, "SHDefExtractIcon failed!" );
        ExitGracefully( hr, S_OK, "Extracting icon..." );
    }

    MDTraceAssert( m_pei );
    if ( !m_pei )
        ExitGracefully( hr, E_FAIL, "m_pei undefined!" );

#ifdef UNICODE
    //
    // More IExtractIconA silliness
    //
    if (m_fAnsiExtractIcon) {
        ULONG cch = (lstrlen(pszIconFile) + 1);
        LPSTR szBuf = (LPSTR) LocalAlloc(LPTR, (lstrlen(pszIconFile) + 1) * sizeof(CHAR));
        if (!szBuf) {
            ExitGracefully(hr, E_OUTOFMEMORY, "unable to allocate string buffer");
        } // if (!szBuf)

        SHTCharToAnsi(pszIconFile, szBuf, cch);

        hr = m_pei->Extract( (LPTSTR) szBuf, nIconIndex, pLargeIcon, pSmallIcon, nIconSize );

        LocalFree((HLOCAL) szBuf);
    } // if
    else {
        hr = m_pei->Extract( pszIconFile, nIconIndex, pLargeIcon, pSmallIcon, nIconSize );
    } // else

#else // UNICODE
    hr = m_pei->Extract( pszIconFile, nIconIndex, pLargeIcon, pSmallIcon, nIconSize );
#endif // UNICODE

exit_gracefully:

    MDTraceLeaveResult(hr);
}
