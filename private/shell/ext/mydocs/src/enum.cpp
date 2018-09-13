/*----------------------------------------------------------------------------
/ Title;
/   enum.cpp
/
/ Authors;
/   Rick Turner (ricktu)
/
/ Notes;
/   Enumerates the My Documents folders for the shell
/----------------------------------------------------------------------------*/
#include "precomp.hxx"
#pragma hdrstop

/*-----------------------------------------------------------------------------
/ CMyDocsEnum
/----------------------------------------------------------------------------*/
CMyDocsEnum::CMyDocsEnum( IShellFolder * psf,
                          HWND hwndOwner,
                          DWORD grfFlags,
                          LPITEMIDLIST pidlRoot,
                          BOOL bRoot
                         )
{
    HRESULT hr = E_FAIL;

    MDTraceEnter(TRACE_ENUM, "CMyDocsEnum::CMyDocsEnum");

    m_peidl    = NULL;
    m_pidlRoot = ILClone(pidlRoot);
    m_hidl     = NULL;
    m_bRoot    = bRoot;
    m_cFetched = 0;

    if (!psf)
        ExitGracefully( hr, E_NOINTERFACE, "psf is not defined!" );

    m_hwndOwner = hwndOwner;

    //
    // Try to get enumerator from the shell...
    //

    hr = psf->EnumObjects( hwndOwner, grfFlags, &m_peidl );
    FailGracefully( hr, "Couldn't get enumerator from the shell!" );

exit_gracefully:

    MDTraceLeaveResultNoRet(hr);

}


//
// Destructor and its callback, we discard that DPA by using the DPA_DestroyCallBack
// which gives us a chance to free the contents of each pointer as we go.
//
INT _EnumDestoryCB(LPVOID pVoid, LPVOID pData)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)pVoid;

    MDTraceEnter(TRACE_ENUM, "_EnumDestroyCB");
    DoILFree(pidl);
    MDTraceLeaveValue(TRUE);
}

CMyDocsEnum::~CMyDocsEnum()
{
    DoILFree( m_pidlRoot );
    if (m_hidl)
    {
        DPA_DestroyCallback(m_hidl, _EnumDestoryCB, NULL);
        m_hidl = NULL;
    }
    DoRelease( m_peidl );
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CMyDocsEnum
#include "unknown.inc"

STDMETHODIMP CMyDocsEnum::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IEnumIDList, (LPENUMIDLIST)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IEnumIDList methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnum::Next(ULONG celt, LPITEMIDLIST* rgelt, ULONG* pceltFetched)
{
    ULONG uExtra = 0;
    HRESULT hr = S_OK;
    BOOL fFirst = FALSE;
    UINT count = 0;

    MDTraceEnter(TRACE_ENUM, "CMyDocsEnum::Next");

    // Validate the arguments and attempt to build the enumerator we
    // are going to be using.
    if (!m_peidl)
        ExitGracefully(hr, E_NOINTERFACE, "m_peidl is NULL" );

    if ( !celt || !rgelt )
        ExitGracefully(hr, E_INVALIDARG, "Nothing to return, or nowhere to return items to");

    if (( celt > 1) && (!pceltFetched))
        ExitGracefully(hr, E_INVALIDARG, "celt > 1 && pceltFetched == NULL");

    //
    // do we need to do first time initialization?
    //

    if (m_bRoot && !m_cFetched)
    {
        DoFirstTimeInitialization( );
    }

    if ( pceltFetched )
    {
        *pceltFetched = 0;
    }

    if (m_hidl)
    {
        count = DPA_GetPtrCount( m_hidl );
    }

    //
    // See if there are any special items to return still...
    //

    while (m_cFetched < count)
    {
        rgelt[uExtra] = ILClone((LPITEMIDLIST)DPA_FastGetPtr( m_hidl, m_cFetched ));
        uExtra++;
        m_cFetched++;
        if (uExtra == celt)
            goto exit_and_return;
    }

    hr = m_peidl->Next( celt - uExtra, rgelt, pceltFetched );
    FailGracefully( hr, "m_peidl->Next failed" );

exit_and_return:
    if (pceltFetched)
    {
        m_cFetched += *pceltFetched;
        *pceltFetched += uExtra;
    }


exit_gracefully:
#ifdef DEBUG
    if ( pceltFetched )
        MDTrace(TEXT("pceltFetched %d"), *pceltFetched);

    {
        INT i;

        if (pceltFetched)
        {
            i = *pceltFetched;
        }
        else
        {
            i = celt;
        }

        while( i )
        {
            MDTrace(TEXT("returning pidl %d as %08X"), i, rgelt[i-1]);
            i--;
        }

    }

#endif

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnum::Skip(ULONG celt)
{
    HRESULT hr;

    MDTraceEnter(TRACE_ENUM, "CMyDocsEnum::Skip");
    if (!m_peidl)
        ExitGracefully(hr, E_NOINTERFACE, "m_peidl is NULL" );

    hr = m_peidl->Skip( celt );
    FailGracefully( hr, "shell failed while trying to skip" );
    m_cFetched++;

exit_gracefully:

    MDTraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnum::Reset()
{
    HRESULT hr;

    MDTraceEnter(TRACE_ENUM, "CMyDocsEnum::Reset");

    if (!m_peidl)
        ExitGracefully(hr, E_NOINTERFACE, "m_peidl is NULL" );

    hr = m_peidl->Reset();
    FailGracefully( hr, "couldn't reset shell's enumerator" );
    m_cFetched = 0;

exit_gracefully:

    MDTraceLeaveResult(hr);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CMyDocsEnum::Clone(LPENUMIDLIST* ppenum)
{
    MDTraceEnter(TRACE_ENUM, "CMyDocsEnum::Clone");
    MDTraceLeaveResult(E_NOTIMPL);
}


/*-----------------------------------------------------------------------------
/ FindSpecialItem
/
/ scans the special item hidl for an entry w/the same name as this item.
/
/----------------------------------------------------------------------------*/
LPITEMIDLIST CMyDocsEnum::FindSpecialItem( LPTSTR pName )
{
    INT count = 0, i;
    LPITEMIDLIST pidl;
    TCHAR szName[ MAX_PATH ];
    UINT cch;

    if (m_hidl && pName && *pName)
    {
        count = DPA_GetPtrCount( m_hidl );
    }

    if (!count)
    {
        return NULL;
    }

    for( i = 0; i < count; i++ )
    {
        pidl = (LPITEMIDLIST)DPA_FastGetPtr( m_hidl, i );
        cch = ARRAYSIZE(szName);
        if (SUCCEEDED(MDGetNameFromIDL( pidl, szName, &cch, FALSE )))
        {
            if (lstrcmpi( szName, pName )==0)
            {
                return pidl;
            }
        }
    }

    return NULL;

}

/*-----------------------------------------------------------------------------
/ _GetSpecialItems
/
/ Given an hkey, scan for special item entries and add them the the hidl
/ if they are found (and are unique)
/
/----------------------------------------------------------------------------*/
VOID CMyDocsEnum::_GetSpecialItems( HKEY hkey  )
{

    DWORD cbName,dwIndex = 0;
    LONG lres = ERROR_SUCCESS;
    TCHAR szName[ MAX_PATH ];
    FILETIME ft;


    MDTraceEnter( TRACE_ENUM, "_GetSpecialItems" );

    //
    // Enumerate the special items, if there are any,
    // and create pidls for them now...
    //

    while( lres == ERROR_SUCCESS )
    {
        cbName = ARRAYSIZE(szName);
        lres = RegEnumKeyEx( hkey,
                             dwIndex,
                             szName,
                             &cbName,
                             NULL,      // lpReserved
                             NULL,      // lpClass
                             NULL,      // lpcbClass
                             &ft
                            );

        if (lres == ERROR_SUCCESS)
        {
            HKEY hkey2;
            LPTSTR pIconString = NULL;
            UINT uIndex = 0;
            LPTSTR pPath = NULL;

            //
            // found one!  Get the rest of the information needed
            //

            if (RegOpenKey( hkey, szName, &hkey2 )==ERROR_SUCCESS)
            {

                //
                // Get the icon information & real path...
                //

                LocalQueryString( &pIconString, hkey2, TEXT("DefaultIcon") );
                LocalQueryString( &pPath, hkey2, TEXT("RealPath" ) );


                //
                // Parse the icon information (it's in the form: blah.dll,1)
                //

                if (pIconString)
                {
                    uIndex = PathParseIconLocation( pIconString );
                    MDTrace( TEXT("special DefaultIcon = %s,%d"), pIconString, uIndex );
                }

                //
                // If we've got a real path (which is necessary!), create
                // an entry for this special item...
                //

                if (pPath)
                {
                    LPITEMIDLIST pidl;

                    MDTrace( TEXT("special RealPath = %s"), pPath );
                    //
                    // We've got enough info to create one now...
                    //

                    if (!m_hidl)
                    {
                        m_hidl = DPA_Create( 4 );
                    }

                    pidl = MDCreateSpecialIDL( pPath,
                                               szName,
                                               pIconString,
                                               uIndex
                                              );

                    if (m_hidl && pidl  && (!FindSpecialItem( szName )))
                    {
                        if ( -1 == DPA_AppendPtr( m_hidl, pidl) )
                        {
                            MDTraceMsg("Failed to insert into the DPA");
                            DoILFree(pidl);
                        }
                    }

                } // if (pPath)

                LocalFreeString( &pPath );
                LocalFreeString( &pIconString );
                RegCloseKey( hkey2 );

            }  // if RegOpenKey

        } // if RegEnumKey

        dwIndex++;
    }

    MDTraceLeave();
}


/*-----------------------------------------------------------------------------
/ DoFirstTimeInitialization
/
/ Query registry for special places.  First check the user's preferences,
/ and then check the policy key, followed by the machine preferences and
/ policy.
/
/----------------------------------------------------------------------------*/
VOID
CMyDocsEnum::DoFirstTimeInitialization( VOID )
{

    HKEY hkey = NULL;

    MDTraceEnter( TRACE_ENUM, "DoFirstTimeInitialization" );


    //
    // Get the user preferences first
    //

    MDTrace( TEXT("Trying key %s"), c_szDocumentSettings );
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER, c_szDocumentSettings, 0, KEY_READ, &hkey ))
    {
        _GetSpecialItems( hkey );
        RegCloseKey( hkey );
    }


    //
    // Now get the user policies
    //

    MDTrace( TEXT("Trying key %s"), c_szPolicyDocumentSettings );
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER, c_szPolicyDocumentSettings, 0, KEY_READ, &hkey ))
    {
        _GetSpecialItems( hkey );
        RegCloseKey( hkey );
    }


    //
    // Get the machine preferences
    //

    MDTrace( TEXT("Trying key %s"), c_szDocumentSettings );
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szDocumentSettings, 0, KEY_READ, &hkey ))
    {
        _GetSpecialItems( hkey );
        RegCloseKey( hkey );
    }


    //
    // Now get the machine policies policies
    //

    MDTrace( TEXT("Trying key %s"), c_szPolicyDocumentSettings );
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, c_szPolicyDocumentSettings, 0, KEY_READ, &hkey ))
    {
        _GetSpecialItems( hkey );
        RegCloseKey( hkey );
    }


    MDTraceLeave();

}
