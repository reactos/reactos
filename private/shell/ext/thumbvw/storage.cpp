#include "precomp.h"
#include <ntdef.h>
#include "dllload.h"

#define THUMB_FILENAME      L"Thumbs.db"
#define CATALOG_STREAM      L"Catalog"

#define CATALOG_VERSION     0x0004
#define STREAMFLAGS_JPEG    0x0001

struct StreamHeader
{
    DWORD cbSize;
    DWORD dwFlags;
    ULONG ulSize;
};

void GenerateStreamName( LPWSTR pszBuffer, DWORD cbSize, DWORD dwNumber );
HRESULT ReadImage( LPSTREAM pStream, HBITMAP * phImage );
HRESULT WriteImage( LPSTREAM pStream, HBITMAP hImage );
LPBITMAPINFO BitmapToDIB( HBITMAP hBmp );

/////////////////////////////////////////////////////////////////////////////////////////////
CThumbStore::CThumbStore()
{
    m_szPath[0] = 0;
    m_rgHeader.dwEntryCount = 0;
    m_rgHeader.wVersion = CATALOG_VERSION;
    m_rgHeader.cbSize = sizeof( m_rgHeader );
    m_dwMaxIndex = 0;

    // this counter is inc'd everytime the catalog changes so that we know when it
    // must be committed and so enumerators can detect the list has changed...
    m_dwCatalogChange = 0;

    m_fLocked = 0;
    InitializeCriticalSection( & m_csLock );
    InitializeCriticalSection( &m_csInternals );
}

//////////////////////////////////////////////////////////////////////////////////////////////
CThumbStore::~CThumbStore()
{
    CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
    while ( pCur != NULL )
    {
        PrivCatalogEntry * pNode = m_rgCatalog.GetNext( pCur );
        Assert( pNode != NULL );

        LocalFree(( LPVOID ) pNode );
    }

    m_rgCatalog.RemoveAll();

    if ( m_pStorage )
    {
        m_pStorage->Release();
    }

    if ( m_pJPEGCodec )
    {
        delete m_pJPEGCodec;
    }

    // assume these are free, we are at ref count zero, no one should still be calling us...
    DeleteCriticalSection( &m_csLock );
    DeleteCriticalSection( &m_csInternals );
}

//////////////////////////////////////////////////////////////////////////////////////////////
DWORD CThumbStore::AquireLock( void )
{
    EnterCriticalSection( &m_csLock );

    // inc the lock (we use a counter because we may reenter this on the same thread)
    m_fLocked ++;

    // Never return a lock signature of zero, because that means "not locked".
    if (++m_dwLock == 0) ++m_dwLock;
    return m_dwLock;
}

//////////////////////////////////////////////////////////////////////////////////////////////
void CThumbStore::ReleaseLock( DWORD dwLock )
{
    if (dwLock) {
        Assert(m_fLocked);
        m_fLocked --;
        LeaveCriticalSection( &m_csLock );
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////
// the structure of the catalog is simple, it is a just a header stream
HRESULT CThumbStore::LoadCatalog( )
{
    Assert( !SupportsStreams());

    if ( m_pStorage == NULL )
    {
        return E_UNEXPECTED;
    }

    if ( m_rgHeader.dwEntryCount != 0 )
    {
        // it is already loaded....
        return NOERROR;
    }

    // open the catalog stream...
    LPSTREAM pCatalog = NULL;

    HRESULT hr = m_pStorage->OpenStream( CATALOG_STREAM,
                                         NULL,
                                         GetAccessMode( STGM_READ, TRUE ),
                                         NULL,
                                         &pCatalog );

    if ( FAILED( hr ))
    {
        return hr;
    }

    EnterCriticalSection( &m_csInternals );
    // now read in the catalog from the stream ...
    DWORD cbRead;
    UINT iEntry;
    hr = pCatalog->Read( & m_rgHeader, sizeof( m_rgHeader ), & cbRead);
    if ( FAILED( hr ) || ( cbRead < sizeof( m_rgHeader )))
    {
        goto loadCleanup;
    }

    if ( m_rgHeader.cbSize != sizeof( m_rgHeader ) || ( m_rgHeader.wVersion != CATALOG_VERSION ))
    {
        hr = STG_E_OLDFORMAT;

        // reset the catalog header...
        m_rgHeader.wVersion = CATALOG_VERSION;
        m_rgHeader.cbSize = sizeof( m_rgHeader );
        m_rgHeader.dwEntryCount = 0;

        goto loadCleanup;
    }

    for ( iEntry = 0; iEntry < m_rgHeader.dwEntryCount; iEntry ++ )
    {
        DWORD cbSize = 0;
        hr = pCatalog->Read( &cbSize, sizeof( DWORD ), &cbRead);
        if ( FAILED( hr ) || ( cbRead < sizeof( DWORD )))
        {
            m_rgHeader.dwEntryCount = iEntry;
            goto loadCleanup;
        }

        // Assert we do not have a bogus number......
        Assert( cbSize <= sizeof( PrivCatalogEntry ) + sizeof( WCHAR ) * MAX_PATH );

        PrivCatalogEntry * pEntry = (PrivCatalogEntry *) LocalAlloc( LPTR, cbSize );
        if ( pEntry == NULL )
        {
            m_rgHeader.dwEntryCount = iEntry;
            // out of mem...
            hr = E_OUTOFMEMORY;
            goto loadCleanup;
        }

        pEntry->cbSize = cbSize;

        // read the rest with out the size on the front...
        hr = pCatalog->Read( ((LPBYTE)pEntry + sizeof(DWORD)),
            cbSize - sizeof( DWORD ),
            &cbRead);
        if ( FAILED( hr ) || ( cbRead < cbSize - sizeof( DWORD )))
        {
            m_rgHeader.dwEntryCount = iEntry;
            goto loadCleanup;
        }

        CLISTPOS pCur = m_rgCatalog.AddTail( pEntry );
        if ( pCur == NULL )
        {
            m_rgHeader.dwEntryCount = iEntry;
            hr = E_OUTOFMEMORY;
            goto loadCleanup;
        }

        if ( m_dwMaxIndex < pEntry->dwIndex )
        {
            m_dwMaxIndex = pEntry->dwIndex;
        }
    }

loadCleanup:
    m_dwCatalogChange = 0;

    LeaveCriticalSection( &m_csInternals );

    if ( pCatalog )
    {
        pCatalog->Release();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbStore::SaveCatalog( )
{
    Assert( !SupportsStreams());

    if ( m_pStorage == NULL )
    {
        return E_UNEXPECTED;
    }

    // open the catalog stream...
    LPSTREAM pCatalog = NULL;

    HRESULT hr = m_pStorage->DestroyElement( CATALOG_STREAM );

    hr = m_pStorage->CreateStream( CATALOG_STREAM,
                                   GetAccessMode( STGM_WRITE, TRUE ),
                                   NULL,
                                   NULL,
                                   &pCatalog );
    if ( FAILED( hr ))
    {
        return hr;
    }

    DWORD cbWritten;
    UINT iEntry;
    CLISTPOS pCur = NULL;

    // don't want anyone messing with our internals while we are saving...
    EnterCriticalSection( &m_csInternals );

    // now read in the catalog from the stream ...
    hr = pCatalog->Write( & m_rgHeader, sizeof( m_rgHeader ), &cbWritten);
    if ( FAILED( hr ) || ( cbWritten != sizeof( m_rgHeader )))
    {
        goto saveCleanup;
    }

    pCur = m_rgCatalog.GetHeadPosition();
    for ( iEntry = 0; iEntry < m_rgHeader.dwEntryCount; iEntry ++ )
    {
        Assert( pCur != NULL );
        PrivCatalogEntry * pEntry = m_rgCatalog.GetNext( pCur );
        Assert( pEntry != NULL );

        hr = pCatalog->Write( pEntry, pEntry->cbSize, &cbWritten );
        if ( FAILED( hr ) || ( cbWritten != pEntry->cbSize ))
        {
            goto saveCleanup;
        }
    }
    // we should have reached the end of the list....
    Assert( pCur == NULL );

    m_dwCatalogChange = 0;

saveCleanup:
    LeaveCriticalSection( &m_csInternals );
    if ( pCatalog )
    {
        pCatalog->Release();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
void GenerateStreamName( LPWSTR pszBuffer, DWORD cbSize, DWORD dwNumber )
{
    UINT cPos = 0;
    while ( dwNumber > 0 )
    {
        DWORD dwRem = dwNumber % 10;

        // based the fact that UNICODE chars 0-9 are the same as the ANSI chars 0 - 9
        pszBuffer[cPos ++] = (WCHAR) ( dwRem + '0' );
        dwNumber /= 10;
    }
    pszBuffer[cPos] = 0;
}

// *** IPersist methods ****
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::GetClassID(CLSID *pClsid)
{
    *pClsid = CLSID_ShellThumbnailDiskCache;
    return NOERROR;
}

// *** IPersistFolder methods ***
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::Initialize(LPCITEMIDLIST pidl)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr;

    hr = SHGetPathFromIDListWrapW( pidl, szPath );
    if ( SUCCEEDED( hr ))
    {
        if (PathCombineW( m_szPath, szPath, THUMB_FILENAME ))
        {
            // given the path, do a quick check to see if the directory is on NTFS or not..
            CheckSupportsStreams( szPath );
            hr = S_OK;
        }
        else
            hr = E_INVALIDARG;
    }
    return hr;
}


// *** IPersistFile methods ***
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::IsDirty ( void)
{
    return m_dwCatalogChange ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::Load(LPCWSTR pszFileName, DWORD dwMode)
{
    // the dwMode parameter is ignored, we just use this to give us the path to the DB
    DWORD dwAttrs = GetFileAttributesWrapW( pszFileName );
    if ( dwAttrs != (DWORD) -1 && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY ))
    {
        PathCombineW( m_szPath, pszFileName, THUMB_FILENAME );
    }
    else
    {
        StrCpyW( m_szPath, pszFileName );
    }

    CheckSupportsStreams( m_szPath );

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::Save ( LPCWSTR pszFileName, BOOL fRemember )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::SaveCompleted ( LPCWSTR pszFileName )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::GetCurFile ( LPWSTR *ppszFileName )
{
    return SHStrDupW(m_szPath, ppszFileName);
}

// *** IShellImageStore methods ****
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: Open ( DWORD dwMode, DWORD * pdwLock )
{
    if (pdwLock)
    {
        *pdwLock = 0;   // Make sure we set a return value
    }
    if ( m_szPath[0] == 0 )
    {
        return E_UNEXPECTED;
    }

    // NTFS support...
    if ( SupportsStreams())
    {
        // we can always open the streams folder because there is no catalog...
        return NOERROR;
    }

    // at this point we have the lock if we need it, so we can close and reopen if we
    // don't have it open with the right permissions...
    if ( m_pStorage )
    {
        if ((m_dwFlags != dwMode ))
        {
            // we are open and the mode is different, so close it. Note, no lock is passed, we already
            // have it
            HRESULT hr = Close( NULL );
            if ( FAILED( hr ))
            {
                return hr;
            }
        }
        else
        {
            // we already have it open...
            if ( pdwLock )
            {
                *pdwLock = AquireLock();
            }
            return S_FALSE;
        }
    }

    DWORD dwLock = AquireLock();

    DWORD dwFlags = GetAccessMode( dwMode, FALSE );

    // now open the DocFile ...
    HRESULT hr = StgOpenStorage( m_szPath, NULL, dwFlags, NULL, NULL, & m_pStorage );
    if ( SUCCEEDED( hr ))
    {
        m_dwFlags = dwMode & 0x3;

        SetFileAttributesWrapW(m_szPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

        // BUGBUG should detect if the catalog has changed since we last loaded it..
        LoadCatalog();

        if ( pdwLock )
        {
            *pdwLock = dwLock;
        }
    }
    else if (STG_E_DOCFILECORRUPT == hr)
    {
        DeleteFileWrapW(m_szPath);
    }

    if ( FAILED( hr ) || !pdwLock )
    {
        ReleaseLock( dwLock );
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: Create ( DWORD dwMode, DWORD *pdwLock )
{
    if (pdwLock)
    {
        *pdwLock = 0;   // Make sure we set a return value
    }
    if ( m_szPath[0] == 0 )
    {
        return E_UNEXPECTED;
    }

    if ( SupportsStreams() )
    {
        if ( pdwLock )
        {
            *pdwLock = 0;
        }
        return NOERROR;
    }

    if ( m_pStorage )
    {
        // we already have it open, so we can't create it ...
        return STG_E_ACCESSDENIED;
    }

    DWORD dwLock = AquireLock();

    DWORD dwFlags = GetAccessMode( dwMode, FALSE );

    HRESULT hr = StgCreateDocfile( m_szPath, dwFlags, NULL, & m_pStorage );
    if ( SUCCEEDED( hr ))
    {
        SetFileAttributesWrapW(m_szPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        m_dwFlags = dwMode & 0x3;
        if ( pdwLock )
        {
            *pdwLock = dwLock;
        }
    }

    if ( FAILED( hr ) || !pdwLock )
    {
        ReleaseLock( dwLock );
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::ReleaseLock( DWORD const * pdwLock )
{
    if ( !pdwLock )
    {
        return E_INVALIDARG;
    }

    ReleaseLock( *pdwLock );
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore::IsLocked ()
{
    Assert(( SupportsStreams() && !m_fLocked) || !SupportsStreams());

    return ( m_fLocked > 0 ? S_OK : S_FALSE );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: Close ( DWORD const * pdwLock )
{
    DWORD dwLock;

    // NTFS support
    if ( SupportsStreams() )
    {
        return S_OK;
    }

    DWORD const * pdwRel = pdwLock;

    if ( !pdwLock )
    {
        dwLock = AquireLock();
        pdwRel = &dwLock;
    }

    HRESULT hr = S_FALSE;
    if ( m_pStorage != NULL )
    {
        if ( m_dwFlags != STGM_READ )
        {
            // write out the new catalog...
            hr = Commit( pdwLock);

            m_pStorage->Commit(0);
        }

        m_pStorage->Release();
        m_pStorage = NULL;
    }

    ReleaseLock( *pdwRel );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: Commit ( DWORD const * pdwLock )
{
    // NTFS support, there is no catalog...
    if ( SupportsStreams())
    {
        return NOERROR;
    }

    DWORD dwLock;
    if ( !pdwLock )
    {
        dwLock = AquireLock();
        pdwLock = &dwLock;
    }

    HRESULT hr = S_FALSE;

    if ( m_pStorage != NULL && m_dwFlags != STGM_READ )
    {
        if ( m_dwCatalogChange )
        {
            SaveCatalog();
        }
        hr = NOERROR;
    }

    ReleaseLock( *pdwLock );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: GetMode ( DWORD * pdwMode )
{
    if ( !pdwMode )
    {
        return E_INVALIDARG;
    }

    if ( m_pStorage )
    {
        *pdwMode = m_dwFlags;
        return NOERROR;
    }

    *pdwMode = 0;
    return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: GetCapabilities( DWORD * pdwMode )
{
    Assert( pdwMode );

    if ( SupportsStreams())
    {
        // NTFS needs neither locable or purgeable.
        *pdwMode = 0;
    }
    else
    {
        // right now, both are needed/supported for thumbs.db
        *pdwMode = SHIMSTCAPFLAG_LOCKABLE | SHIMSTCAPFLAG_PURGEABLE;
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: AddEntry ( LPCWSTR pszName, const FILETIME * pftTimeStamp, DWORD dwMode, HBITMAP hImage )
{
    Assert( pszName );

    if ( SupportsStreams())
    {
        // for NTFS call our helper...
        return WriteToStream( pszName, pftTimeStamp, hImage );
    }

    if ( !m_pStorage )
    {
        return E_UNEXPECTED;
    }

    if ( m_dwFlags == STGM_READ )
    {
        // can't modify in this mode...
        return E_ACCESSDENIED;
    }

    // this will block unless we already have the lock on this thread...
    DWORD dwLock = AquireLock();

    DWORD dwStream = 0;
    CLISTPOS pCur = NULL;
    PrivCatalogEntry * pNode = NULL;

    EnterCriticalSection( &m_csInternals );

    if ( FindStreamID( pszName, dwStream, &pNode ) != S_OK )
    {
        // needs adding to the catalog...
        UINT cbSize = sizeof( PrivCatalogEntry ) + lstrlenW( pszName ) * sizeof( WCHAR );

        pNode = ( PrivCatalogEntry * ) LocalAlloc( LPTR, cbSize );
        if ( pNode == NULL )
        {
            LeaveCriticalSection( &m_csInternals );
            ReleaseLock( dwLock );
            return E_OUTOFMEMORY;
        }

        pNode->cbSize = cbSize;
        if ( pftTimeStamp )
        {
            pNode->ftTimeStamp = *pftTimeStamp;
        }
        dwStream = pNode->dwIndex = ++ m_dwMaxIndex;
        StrCpyW( pNode->szName, pszName );

        pCur = m_rgCatalog.AddTail( pNode );
        if ( pCur == NULL )
        {
            LocalFree( pNode );
            LeaveCriticalSection( &m_csInternals );
            ReleaseLock( dwLock );
            return E_OUTOFMEMORY;
        }

        m_rgHeader.dwEntryCount ++;
    }
    else
    {
        // update the timestamp .....
        if ( pftTimeStamp )
        {
            pNode->ftTimeStamp = *pftTimeStamp;
        }
    }

    LeaveCriticalSection( &m_csInternals );

    LPSTREAM pStream = NULL;
    HRESULT hr = GetEntryStream( dwStream, dwMode, &pStream );
    if ( SUCCEEDED( hr ))
    {
        Assert( pStream );
        hr = WriteImage( pStream, hImage );
        pStream->Release();
    }

    if ( FAILED( hr ) && pCur )
    {
        // take it back out of the list if we added it...
        EnterCriticalSection( &m_csInternals );
        m_rgCatalog.RemoveAt( pCur );
        LeaveCriticalSection( &m_csInternals );
        LocalFree( pNode );
    }

    if ( SUCCEEDED( hr ))
    {
        // catalog change....
        m_dwCatalogChange ++;
    }

    ReleaseLock( dwLock );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: GetEntry ( LPCWSTR pszName, DWORD dwMode, HBITMAP * phImage )
{
    if ( !phImage )
    {
        return E_INVALIDARG;
    }

    if ( SupportsStreams() )
    {
        // for NTFS use our helper....
        return ReadFromStream( pszName, phImage );
    }

    if ( !m_pStorage )
    {
        return E_UNEXPECTED;
    }

    DWORD dwStream;

    if ( FindStreamID( pszName, dwStream, NULL ) != S_OK )
    {
        return E_FAIL;
    }

    LPSTREAM pStream = NULL;
    HRESULT hr = GetEntryStream( dwStream, dwMode, &pStream );
    if ( SUCCEEDED( hr ))
    {
        Assert( pStream );
        hr = ReadImage( pStream, phImage );
        pStream->Release();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: DeleteEntry ( LPCWSTR pszName )
{
    if ( !pszName )
    {
        return E_INVALIDARG;
    }

    if ( SupportsStreams())
    {
        return DeleteFromStream( pszName );
    }

    if ( !m_pStorage )
    {
        return E_UNEXPECTED;
    }

    if ( m_dwFlags == STGM_READ )
    {
        // can't modify in this mode...
        return E_ACCESSDENIED;
    }

    DWORD dwLock = AquireLock();

    EnterCriticalSection( &m_csInternals );

    // check to see if it already exists.....
    PrivCatalogEntry * pNode = NULL;

    CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
    while ( pCur != NULL )
    {
        CLISTPOS pDel = pCur;
        pNode = m_rgCatalog.GetNext( pCur );
        Assert( pNode != NULL );

        if ( StrCmpIW( pNode->szName, pszName ) == 0 )
        {
            m_rgCatalog.RemoveAt( pDel );
            m_rgHeader.dwEntryCount --;
            m_dwCatalogChange ++;
            if ( pNode->dwIndex == m_dwMaxIndex )
            {
                m_dwMaxIndex --;
            }
            LeaveCriticalSection( &m_csInternals );

            WCHAR szStream[30];
            GenerateStreamName( szStream, ARRAYSIZE(szStream), pNode->dwIndex );
            m_pStorage->DestroyElement( szStream );

            LocalFree( pNode );
            ReleaseLock( dwLock );
            return S_OK;
        }
    }

    LeaveCriticalSection( &m_csInternals );
    ReleaseLock( dwLock );

    return E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: IsEntryInStore ( LPCWSTR pszName, FILETIME * pftTimeStamp )
{
    if ( SupportsStreams())
    {
        return IsEntryInStream( pszName, pftTimeStamp );
    }

    if ( !m_pStorage )
    {
        return E_UNEXPECTED;
    }

    DWORD dwStream = 0;
    PrivCatalogEntry * pNode = NULL;
    EnterCriticalSection( &m_csInternals );
    HRESULT hr = FindStreamID( pszName, dwStream, &pNode );
    if ( pftTimeStamp && SUCCEEDED( hr ))
    {
        Assert( pNode );
        *pftTimeStamp = pNode->ftTimeStamp;
    }
    LeaveCriticalSection( &m_csInternals );

    return ( hr == S_OK ) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CThumbStore:: Enum ( LPENUMSHELLIMAGESTORE * ppEnum )
{
    if ( SupportsStreams())
    {
        return E_NOTIMPL;
    }

    return CEnumThumbStore_Create( this, ppEnum );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbStore::FindStreamID( LPCWSTR pszName, DWORD & dwStream, PrivCatalogEntry ** ppNode )
{
    // check to see if it already exists in the catalog.....
    PrivCatalogEntry * pNode = NULL;

    CLISTPOS pCur = m_rgCatalog.GetHeadPosition();
    while ( pCur != NULL )
    {
        pNode = m_rgCatalog.GetNext( pCur );
        Assert( pNode != NULL );


        if ( StrCmpIW( pNode->szName, pszName ) == 0)
        {
            dwStream = pNode->dwIndex;

            if ( ppNode != NULL )
            {
                *ppNode = pNode;
            }
            return S_OK;
        }
    }

    return E_FAIL;
}


//////////////////////////////////////////////////////////////////////////////////////////
CEnumThumbStore::CEnumThumbStore()
{
    m_pStore = NULL;
    m_pPos = 0;
    m_dwCatalogChange = 0;
}

CEnumThumbStore::~CEnumThumbStore()
{
    if ( m_pStore )
    {
        ((IPersistFile *)m_pStore)->Release();
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumThumbStore:: Reset ( void )
{
    m_pPos = m_pStore->m_rgCatalog.GetHeadPosition();
    m_dwCatalogChange = m_pStore->m_dwCatalogChange;

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumThumbStore::Next ( ULONG celt, PENUMSHELLIMAGESTOREDATA * prgElt, ULONG * pceltFetched )
{
    if (( celt > 1 && !pceltFetched ) || !celt )
    {
        return E_INVALIDARG;
    }

    if ( m_dwCatalogChange != m_pStore->m_dwCatalogChange )
    {
        return E_UNEXPECTED;
    }

    ULONG celtFetched = 0;

    while ( celtFetched < celt && m_pPos )
    {
        CThumbStore::PrivCatalogEntry * pNode = m_pStore->m_rgCatalog.GetNext( m_pPos );

        Assert( pNode );
        UINT cLen = lstrlenW( pNode->szName ) + 1;
        PENUMSHELLIMAGESTOREDATA pszData = (PENUMSHELLIMAGESTOREDATA) CoTaskMemAlloc( cLen * sizeof( WCHAR ));
        if ( !pszData )
        {
            // cleanup others...
            for ( ULONG celtCleanup = 0; celtCleanup < celtFetched; celtCleanup ++ )
            {
                CoTaskMemFree( (LPVOID) prgElt[celtCleanup] );
                prgElt[celtCleanup] = NULL;
            }

            return E_OUTOFMEMORY;
        }

        StrCpyW( pszData->szPath, pNode->szName );

        Assert( !IsBadWritePtr(( LPVOID ) prgElt[celtFetched], sizeof( LPCWSTR )));
        prgElt[celtFetched] = pszData;

        celtFetched ++;
    }

    if ( pceltFetched )
    {
        *pceltFetched = celtFetched;
    }

    if ( !celtFetched )
    {
        return E_FAIL;
    }
    return ( celtFetched < celt ) ? S_FALSE : S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumThumbStore:: Skip ( ULONG celt )
{
    if ( !celt )
    {
        return E_INVALIDARG;
    }

    if ( m_dwCatalogChange != m_pStore->m_dwCatalogChange )
    {
        return E_UNEXPECTED;
    }

    ULONG celtSkipped = 0;
    while ( celtSkipped < celt && m_pPos )
    {
        m_pStore->m_rgCatalog.GetNext( m_pPos );
    }

    if ( !celtSkipped )
    {
        return E_FAIL;
    }

    return ( celtSkipped < celt ) ? S_FALSE : S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CEnumThumbStore:: Clone ( IEnumShellImageStore ** ppEnum )
{
    if ( !ppEnum )
    {
        return E_INVALIDARG;
    }

    CEnumThumbStore * pEnum = new CComObject<CEnumThumbStore>;
    if ( !pEnum )
    {
        return E_OUTOFMEMORY;
    }

    ((IPersistFile *)m_pStore)->AddRef();

    pEnum->m_pStore = m_pStore;
    pEnum->m_dwCatalogChange = m_dwCatalogChange;

    // created with zero ref count....
    pEnum->AddRef();

    *ppEnum = (LPENUMSHELLIMAGESTORE) pEnum;

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CEnumThumbStore_Create( CThumbStore * pThis, LPENUMSHELLIMAGESTORE * ppEnum )
{
    if ( !pThis || !ppEnum )
    {
        return E_INVALIDARG;
    }

    CEnumThumbStore * pEnum = new CComObject<CEnumThumbStore>;
    if ( !pEnum )
    {
        return E_OUTOFMEMORY;
    }

    ((IPersistFile * )pThis)->AddRef();

    pEnum->m_pStore = pThis;

    // created with zero ref count....
    pEnum->AddRef();

    *ppEnum = (LPENUMSHELLIMAGESTORE) pEnum;

    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbStore::ReadImage( LPSTREAM pStream, HBITMAP * phImage )
{
    if ( pStream == NULL )
    {
        Assert( FALSE );
        return E_INVALIDARG;
    }

    LPVOID pBits;
    DWORD cbRead;
    HRESULT hr;
    StreamHeader rgHead;

    hr = pStream->Read( &rgHead, sizeof( rgHead), &cbRead );
    if ( FAILED( hr ) || ( cbRead != sizeof( rgHead)) || ( rgHead.cbSize != sizeof( rgHead )) || ( rgHead.dwFlags != STREAMFLAGS_JPEG ))
    {
        return E_FAIL;
    }

    pBits = LocalAlloc( LMEM_FIXED, rgHead.ulSize );
    if ( !pBits )
    {
        return E_OUTOFMEMORY;
    }

    hr = pStream->Read( pBits, rgHead.ulSize, &cbRead );
    if ( SUCCEEDED( hr ) && ( cbRead == rgHead.ulSize ))
    {
        if ( !DecompressImage( pBits, rgHead.ulSize, phImage ))
        {
            hr = E_UNEXPECTED;
        }
    }

    LocalFree( pBits );

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT Version1ReadImage( LPSTREAM pStream, HBITMAP * phImage )
{
    if ( pStream == NULL )
    {
        Assert( FALSE );
        return E_INVALIDARG;
    }

    DWORD cbSize = 0;
    DWORD cbRead = 0;

    HRESULT hr = pStream->Read( &cbSize, sizeof(DWORD), &cbRead );
    if ( FAILED( hr ) || ( cbRead != sizeof( DWORD )))
    {
        return E_FAIL;
    }

    LPBITMAPINFO pbi = (LPBITMAPINFO) LocalAlloc( LPTR, cbSize );
    if ( pbi == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = pStream->Read( pbi, cbSize, &cbRead );
    if ( FAILED( hr ) || ( cbRead != cbSize))
    {
        LocalFree( pbi );
        return E_FAIL;
    }

    // for now, all the bitmaps must be the same size....
    Assert( pbi->bmiHeader.biWidth <= DEFSIZE_THUMBNAIL );
    Assert( pbi->bmiHeader.biHeight <= DEFSIZE_THUMBNAIL );

    HDC hdc = GetDC( GetDesktopWindow( ) );

    LPVOID pBits = CalcBitsOffsetInDIB( pbi );
    *phImage = CreateDIBitmap( hdc, &( pbi->bmiHeader ), CBM_INIT, pBits, pbi, DIB_RGB_COLORS );

    ReleaseDC( GetDesktopWindow(), hdc );

    LocalFree( pbi );

    return ( *phImage != NULL ) ? NOERROR : E_UNEXPECTED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CThumbStore::WriteImage( LPSTREAM pStream, HBITMAP hImage )
{
    if ( pStream == NULL )
    {
        Assert( FALSE );
        return E_INVALIDARG;
    }

    LPVOID pBits;
    ULONG ulBuffer;
    DWORD cbWritten;

    StreamHeader rgHead;

    if ( CompressImage( hImage, &pBits, &ulBuffer ))
    {
        HRESULT hr;

        rgHead.cbSize = sizeof( rgHead );
        rgHead.dwFlags = STREAMFLAGS_JPEG;
        rgHead.ulSize = ulBuffer;

        hr = pStream->Write( &rgHead, sizeof( rgHead ), &cbWritten );
        if ( SUCCEEDED( hr ) && cbWritten == sizeof( rgHead ))
        {
            hr = pStream->Write( pBits, ulBuffer, &cbWritten );
            if ( cbWritten != ulBuffer )
            {
                hr = E_FAIL;
            }
        }

        CoTaskMemFree( pBits );
        return hr;
    }

    return E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT Version1WriteImage( LPSTREAM pStream, HBITMAP hImage )
{
    if ( pStream == NULL )
    {
        Assert( FALSE );
        return E_INVALIDARG;
    }

    LPBITMAPINFO pBitmap = BitmapToDIB( hImage );
    if ( pBitmap == NULL )
    {
        return E_UNEXPECTED;
    }

    int ncolors = pBitmap->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBitmap->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBitmap->bmiHeader.biBitCount;

    if (pBitmap->bmiHeader.biBitCount == 16 ||
        pBitmap->bmiHeader.biBitCount == 32)
    {
        if (pBitmap->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    int iOffset = ncolors * sizeof(RGBQUAD);

    DWORD cbWrite = pBitmap->bmiHeader.biSize + iOffset + pBitmap->bmiHeader.biSizeImage;

    DWORD cbWritten = 0;

    HRESULT hr = pStream->Write( &cbWrite, sizeof(DWORD), &cbWritten );
    if ( FAILED( hr ) || ( cbWritten != sizeof( DWORD )))
    {
        LocalFree( pBitmap );
        return E_FAIL;
    }

    hr = pStream->Write( pBitmap, cbWrite, &cbWritten );
    if ( FAILED( hr ) || ( cbWritten != cbWrite ))
    {
        LocalFree( pBitmap );
        return E_FAIL;
    }

    LocalFree( pBitmap );

    return NOERROR;
}


HRESULT CThumbStore::GetEntryStream( DWORD dwStream, DWORD dwMode, LPSTREAM *ppStream )
{
    WCHAR szStream[30];

    GenerateStreamName( szStream, ARRAYSIZE( szStream ), dwStream );

    // leave only the STG_READ | STGM_READWRITE | STGM_WRITE modes
    dwMode &= 0x3;

    if ( !m_pStorage )
    {
        return E_UNEXPECTED;
    }

    if ( dwMode != m_dwFlags )
    {
        return E_ACCESSDENIED;
    }

    DWORD dwFlags = GetAccessMode( dwMode, TRUE );
    if ( dwFlags & STGM_WRITE )
    {
        m_pStorage->DestroyElement( szStream );
        return m_pStorage->CreateStream( szStream, dwFlags, NULL, NULL, ppStream );
    }
    else
    {
        return m_pStorage->OpenStream( szStream, NULL, dwFlags, NULL, ppStream );
    }
}


DWORD CThumbStore::GetAccessMode( DWORD dwMode, BOOL fStream )
{
    dwMode &= 0x3;

    DWORD dwFlags = dwMode;

    // the root only needs Deny_Write, streams need exclusive....
    if ( dwMode == STGM_READ && !fStream )
    {
        dwFlags |= STGM_SHARE_DENY_WRITE;
    }
    else
    {
        dwFlags |= STGM_SHARE_EXCLUSIVE;
    }

    return dwFlags;
}

///////////////////////////////////////////////////////////////////////////////////////
LPBITMAPINFO BitmapToDIB( HBITMAP hBmp )
{
    HWND        hwnd = GetDesktopWindow( );
    HDC         hdcWnd = GetDC( hwnd );
    HDC         hMemDC = CreateCompatibleDC( hdcWnd );
    BITMAPINFO  bi;
    BITMAP      Bitmap;

    GetObjectWrapW( hBmp, sizeof( Bitmap ), ( LPSTR )&Bitmap );

    bi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bi.bmiHeader.biBitCount = 0;
    int iVal = GetDIBits( hMemDC, hBmp, 0, Bitmap.bmHeight,
                                  NULL, &bi, DIB_RGB_COLORS );

    int ncolors = bi.bmiHeader.biClrUsed;
    if (ncolors == 0 && bi.bmiHeader.biBitCount <= 8)
        ncolors = 1 << bi.bmiHeader.biBitCount;

    if (bi.bmiHeader.biBitCount == 16 ||
        bi.bmiHeader.biBitCount == 32)
    {
        if (bi.bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    int iOffset = ncolors * sizeof(RGBQUAD);

    LPVOID lpBuffer = LocalAlloc( LPTR, sizeof( BITMAPINFOHEADER ) +
                                       iOffset +
                                       bi.bmiHeader.biSizeImage );
    if ( lpBuffer )
    {
        LPBITMAPINFO    pbi = ( LPBITMAPINFO )lpBuffer;

        LPVOID lpBits = ( LPBYTE )lpBuffer + iOffset + sizeof( BITMAPINFOHEADER );

        // copy members of what was returned in last GetDIBits call.
        CopyMemory( &( pbi->bmiHeader ), &( bi.bmiHeader ), sizeof( BITMAPINFOHEADER ) );
        iVal = GetDIBits( hMemDC, hBmp, 0, Bitmap.bmHeight, lpBits,
                          pbi, DIB_RGB_COLORS );
    }

    DeleteDC( hMemDC );
    ReleaseDC( hwnd, hdcWnd );
    return ( LPBITMAPINFO )lpBuffer;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CThumbStore::InitCodec( void )
{
    if ( NULL == m_pJPEGCodec )
    {
        m_pJPEGCodec = new CComObject<CThumbnailFCNContainer>;
    }

    return ( NULL != m_pJPEGCodec );
}

BOOL CThumbStore::CompressImage( HBITMAP hBmp, LPVOID * ppvOutBuffer, ULONG * plBufSize )
{
    // given an HBITMAP, get its data....
    HBITMAP hBmpOut = hBmp;
    LPVOID pBits;
    SIZE rgBmpSize;

    *ppvOutBuffer = NULL;

    if ( !InitCodec())
    {
        return FALSE;
    }

    HRESULT hr = PrepImage( &hBmpOut, &rgBmpSize, &pBits );
    if ( SUCCEEDED( hr ))
    {
        Assert( pBits );

        hr = m_pJPEGCodec->EncodeThumbnail( pBits, rgBmpSize.cx, rgBmpSize.cy,
                                ppvOutBuffer, plBufSize);

        if ( hBmpOut != hBmp )
        {
            // free the DIBSECTION we were passed back...
            DeleteObject( hBmpOut );
        }
    }

    return SUCCEEDED( hr );
}

BOOL CThumbStore::DecompressImage( LPVOID pvInBuffer, ULONG ulBufferSize, HBITMAP * phBmp )
{
    if ( !InitCodec())
    {
        return FALSE;
    }

    ULONG ulWidth;
    ULONG ulHeight;

    HRESULT hr = m_pJPEGCodec->DecodeThumbnail( phBmp, &ulWidth, &ulHeight, pvInBuffer, ulBufferSize );
    return SUCCEEDED( hr );
}


HRESULT CThumbStore::PrepImage( HBITMAP *phBmp, SIZE * prgSize, LPVOID * ppBits )
{
    Assert( phBmp && &phBmp );
    Assert( prgSize );
    Assert( ppBits );

    DIBSECTION rgDIB;

    *ppBits = NULL;

    HRESULT hr = E_FAIL;

    // is the image the wrong colour depth or not a DIBSECTION
    if ( !GetObjectWrapW( *phBmp, sizeof( rgDIB ), &rgDIB ) || ( rgDIB.dsBmih.biBitCount != 32 ))
    {
        HBITMAP hBmp;
        BITMAP rgBmp;

        GetObjectWrapW( *phBmp, sizeof( rgBmp), &rgBmp );

        prgSize->cx = rgBmp.bmWidth;
        prgSize->cy = rgBmp.bmHeight;

        // generate a 32 bit DIB of the right size
        if ( !CreateSizedDIBSECTION( prgSize, 32, NULL, NULL, &hBmp, NULL, ppBits ))
        {
            return E_OUTOFMEMORY;
        }

        HDC hDCMem1 = CreateCompatibleDC( NULL );
        HDC hDCMem2 = CreateCompatibleDC( NULL );

        HBITMAP hBmpOld1 = (HBITMAP) SelectObject( hDCMem1, *phBmp );
        HBITMAP hBmpOld2 = (HBITMAP) SelectObject( hDCMem2, hBmp );

        // copy the image accross to generate the right sized DIB
        BitBlt( hDCMem2, 0, 0, prgSize->cx, prgSize->cy, hDCMem1, 0, 0, SRCCOPY );

        SelectObject( hDCMem1, hBmpOld1 );
        SelectObject( hDCMem2, hBmpOld2 );

        DeleteDC( hDCMem1 );
        DeleteDC( hDCMem2 );

        // pass back the BMP so it can be destroyed later...
        *phBmp = hBmp;
        return S_OK;
    }
    else
    {
        // HOUSTON, we have a DIBSECTION, this is quicker.....
        *ppBits = rgDIB.dsBm.bmBits;

        prgSize->cx = rgDIB.dsBm.bmWidth;
        prgSize->cy = rgDIB.dsBm.bmHeight;

        return S_OK;
    }
}
BOOL CThumbStore::SupportsStreams()
{
    return m_fSupportsStreams;
}

HRESULT CThumbStore::WriteToStream( LPCWSTR pszPath, const FILETIME * pftFileTimeStamp, HBITMAP hBmp )
{
    if ( !InitCodec() )
    {
        return E_UNEXPECTED;
    }

    IPropertySetStorage *pPropSetStorage;

    HRESULT hr = E_FAIL;
    __try
    {
        DWORD grfMode = STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
        hr = StgOpenStorageEx( pszPath, grfMode, STGFMT_FILE,
                               0, NULL, NULL,
                               IID_IPropertySetStorage,
                               (LPVOID *) & pPropSetStorage );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        // do nothing, just don't fault...
    }

    if ( FAILED ( hr ))
    {
        return hr;
    }

    LPVOID pvData = NULL;
    ULONG cbSize = 0;

    if ( CompressImage( hBmp, &pvData, &cbSize ))
    {
        Assert( pvData );
        Assert( cbSize );

        hr = m_pJPEGCodec->WriteThumbnail( pvData, cbSize, pPropSetStorage, TRUE );

        LocalFree( pvData );
    }
    else
    {
        hr = E_FAIL;
    }

    ATOMICRELEASE( pPropSetStorage );

    return hr;
}

HRESULT CThumbStore::ReadFromStream( LPCWSTR pszPath, HBITMAP * phBmp )
{
    if ( !InitCodec() )
    {
        return E_UNEXPECTED;
    }

    IPropertySetStorage *pPropSetStorage;
    HRESULT hr = E_FAIL;
    __try
    {
        DWORD grfMode = STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
        hr = StgOpenStorageEx( pszPath, grfMode, STGFMT_FILE,
                               0, NULL, NULL,
                               IID_IPropertySetStorage,
                               (LPVOID *) & pPropSetStorage );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        // do nothing, just don't fault...
    }

    if ( FAILED ( hr ))
    {
        return hr;
    }

    LPVOID pvData = NULL;
    ULONG cbSize = 0;

    hr = m_pJPEGCodec->ReadThumbnail(&pvData, &cbSize, pPropSetStorage );

    Assert( FAILED(hr) || (SUCCEEDED(hr) && pvData) );

    ATOMICRELEASE( pPropSetStorage );

    if ( SUCCEEDED( hr ))
    {
        if ( ! DecompressImage( pvData, cbSize, phBmp ))
        {
            hr = E_FAIL;
        }
        CoTaskMemFree( pvData );
    }

    return hr;
}

HRESULT CThumbStore::DeleteFromStream( LPCWSTR pszPath )
{
    if ( !InitCodec() )
    {
        return E_UNEXPECTED;
    }

    return E_NOTIMPL;
}

HRESULT CThumbStore::IsEntryInStream( LPCWSTR pszPath, FILETIME * pftTimeStamp )
{
    if ( !InitCodec() )
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = E_FAIL;
    IPropertySetStorage *pPropSetStorage;
    __try
    {
        DWORD grfMode = STGM_READ | STGM_DIRECT | STGM_SHARE_EXCLUSIVE;
        hr = StgOpenStorageEx( pszPath, grfMode, STGFMT_FILE,
                               0, NULL, NULL,
                               IID_IPropertySetStorage,
                               (LPVOID *) & pPropSetStorage );
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        // do nothing, just don't fault...
    }

    if ( FAILED ( hr ))
    {
        return hr;
    }

    LPVOID pvData = NULL;
    ULONG cbSize = 0;

    // really inefficient right now....
    hr = m_pJPEGCodec->ReadThumbnail(&pvData, &cbSize, pPropSetStorage );

    Assert( FAILED(hr) || (SUCCEEDED(hr) && pvData) );

    if ( SUCCEEDED( hr ) && pftTimeStamp )
    {
        hr = m_pJPEGCodec->GetTimeStamp( pPropSetStorage, pftTimeStamp );
    }

    ATOMICRELEASE( pPropSetStorage );

    CoTaskMemFree( pvData );

    return ( SUCCEEDED( hr ) ? S_OK : S_FALSE );
}

void CThumbStore::CheckSupportsStreams( LPCWSTR pszPath )
{
    m_fSupportsStreams = FALSE;
    
    if ( IsOS( OS_NT5 ))
    {
        TCHAR szRoot[MAX_PATH], szFSName[128];
        SHUnicodeToTChar(pszPath, szRoot, ARRAYSIZE(szRoot));
        
        PathStripToRoot(szRoot);
        PathAddBackslash(szRoot);	// make sure UNC path has trailing slash
        
        if (GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, NULL, szFSName, ARRAYSIZE(szFSName)))
        {
            m_fSupportsStreams = StrStrI(szFSName, TEXT("NTFS")) != NULL;
        }
    }
}
