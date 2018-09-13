#include "precomp.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////
CLnkThumb::CLnkThumb()
{
    Assert( !m_pidl );
    Assert( !m_pidlLast);
    Assert( !m_pFolder);
    Assert( !m_pExtract);
    Assert( !m_pExtract2);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CLnkThumb::~CLnkThumb()
{
    if ( m_pidl )
    {
        SHFree((LPVOID) m_pidl );
    }

    if ( m_pFolder )
    {
        m_pFolder->Release();
    }

    if( m_pExtract )
    {
        m_pExtract->Release();
    }

    if ( m_pExtract2 )
    {
        m_pExtract2->Release();
    }

    if ( m_pRunnable )
    {
        m_pRunnable->Release();
    }
}

// IRunnableTask
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Run ()
{
    Assert( m_pRunnable );
    return m_pRunnable->Run();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Kill ( BOOL fWait )
{
    Assert( m_pRunnable );
    return m_pRunnable->Kill( fWait);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Suspend( )
{
    Assert( m_pRunnable );
    return m_pRunnable->Suspend();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Resume( )
{
    Assert( m_pRunnable );
    return m_pRunnable->Resume();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CLnkThumb:: IsRunning ()
{
    Assert( m_pRunnable );
    return m_pRunnable->IsRunning();
}

// IExtractImage
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::GetLocation ( LPWSTR pszPathBuffer,
                                      DWORD cch,
                                      DWORD * pdwPriority,
                                      const SIZE * prgSize,
                                      DWORD dwRecClrDepth,
                                      DWORD *pdwFlags )
{
    Assert( m_pExtract );
    return m_pExtract->GetLocation( pszPathBuffer, cch, pdwPriority, prgSize, dwRecClrDepth, pdwFlags );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Extract( HBITMAP * phBmpThumbnail)
{
    Assert( m_pExtract );
    return m_pExtract->Extract( phBmpThumbnail );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::GetDateStamp( FILETIME * pftDateStamp )
{
    Assert( m_pExtract2 );
    return m_pExtract2->GetDateStamp( pftDateStamp );
}

// IPersistFile
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::GetClassID (CLSID *pClassID)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::IsDirty ()
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Load ( LPCOLESTR pszFileName, DWORD dwMode)
{
    // get the pidl from the shortcut, bind to it, and check the extensions
    // at the other end...
    LPPERSISTFILE pFile = NULL;
    IShellLink * pLink = NULL;

    DWORD dwAttrs = GetFileAttributesWrapW( pszFileName );
    if (( dwAttrs != (DWORD) -1) && (dwAttrs & FILE_ATTRIBUTE_OFFLINE ))
    {
        return E_FAIL;
    }

    HRESULT hr = CoCreateInstance( CLSID_ShellLink,
                                   NULL,
                                   CLSCTX_INPROC,
                                   IID_IPersistFile,
                                   (LPVOID *) &pFile );

    if ( FAILED( hr ))
    {
        return E_FAIL;
    }

    hr = pFile->Load( pszFileName, dwMode );
    if ( FAILED( hr ))
    {
        pFile->Release();
        return hr;
    }

    hr = pFile->QueryInterface( IID_IShellLink, (LPVOID *) &pLink );
    pFile->Release();
    if ( FAILED( hr ))
    {
        return E_FAIL;
    }

    // at his point, we need to try and resolve the shortcut, otherwise
    // we will mess it up by fetching the path (if we are on a UNC drive,
    // then it will end up pointing to a local drive...)

    if ( !IsOS( OS_NT5 ))
    {
        // only on previous os's. On nt5 this will hang because of webview..if 
        // the server is not present.
        hr = pLink->Resolve( NULL, SLR_NO_UI | SLR_UPDATE );
    }
    
    hr = pLink->GetIDList( & m_pidl );
    pLink->Release();
    if ( FAILED( hr ) || !m_pidl )
    {
        return E_FAIL;
    }

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::Save ( LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::SaveCompleted ( LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLnkThumb::GetCurFile ( LPOLESTR *ppszFileName)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CLnkThumb::BindToFolder()
{
    if ( !m_pidl )
    {
        return E_UNEXPECTED;
    }

    // have pidl, will travel ...
    HRESULT hr = SHGetDesktopFolder( & m_pFolder );

    if ( FAILED( hr ) || !m_pFolder )
    {
        return E_FAIL;
    }

    // need to bind to the folder. ..
    USHORT cb;
    m_pidlLast = (LPITEMIDLIST) FindLastPidl( m_pidl );
    if ( m_pidlLast != m_pidl )
    {
        IShellFolder * pFolder = NULL;

        // terminate the pidl
        cb = m_pidlLast->mkid.cb;
        m_pidlLast->mkid.cb = 0;

        // can we reach the folder....
        hr = m_pFolder->BindToObject( m_pidl, NULL, IID_IShellFolder, (LPVOID *) & pFolder );
        if ( FAILED( hr ))
        {
            return E_FAIL;
        }

        m_pFolder->Release();
        m_pFolder = pFolder;
        m_pidlLast->mkid.cb = cb;
    }

    return NOERROR;
}


////////////////////////////////////////////////////////////////////////////////////
// delay binding to the object until we have to, IE when we are asked for the
// specific interface ....
HRESULT CLnkThumb::_InternalQueryInterface( REFIID riid, LPVOID * ppvObj )
{
    HRESULT hr = E_NOINTERFACE;

    if (IsEqualIID( riid, IID_IUnknown ))
    {
        *ppvObj = (IUnknown *)(IRunnableTask *) this;
        hr = NOERROR;
    }
    else if ( IsEqualIID( riid, IID_IPersistFile ))
    {
        *ppvObj = (IPersistFile *) this;
        hr = NOERROR;
    }
    else if ( riid == IID_IRunnableTask )
    {
        if ( !m_pExtract )
        {
            // this is unexpected because the GetUIObjectOF handler should have
            // QI'd for Either of the other two first...
            hr = E_UNEXPECTED;
        }
        else if ( m_pRunnable )
        {
            *ppvObj = (IRunnableTask *) this;
            hr = NOERROR;
        }
    }
    else if ( riid == IID_IExtractImage2 )
    {
        if ( !m_pExtract )
        {
            // this is unexpected because the GetUIObjectOF handler should have
            // QI'd for Either of the other two first...
            hr = E_UNEXPECTED;
        }
        else if ( m_pExtract2 )
        {
            *ppvObj = (IExtractImage2 *) this;
            hr = NOERROR;
        }
    }
    else if (IsEqualIID( riid, IID_IExtractImage))
    {
        hr = BindToFolder();
        if ( SUCCEEDED( hr ))
        {
            Assert( m_pidlLast != NULL );

            UINT rgfFlags = 0;
            hr = m_pFolder->GetUIObjectOf( NULL,
                                           1,
                                           (LPCITEMIDLIST *) &m_pidlLast,
                                           riid,
                                           &rgfFlags,
                                           (LPVOID *) &m_pExtract );
            if ( SUCCEEDED( hr ))
            {
                m_pExtract->QueryInterface( IID_IRunnableTask, (LPVOID *) & m_pRunnable );
                m_pExtract->QueryInterface( IID_IExtractImage2, (LPVOID *) &m_pExtract2 );

                *ppvObj = (LPEXTRACTIMAGE) this;
            }
        }

        if ( FAILED( hr ))
        {
            hr = E_NOINTERFACE;
        }
    }

    if ( SUCCEEDED( hr ))
    {
        this->InternalAddRef();
    }
    return hr;
}

