/* Thumbnail extractor for web pages..
 * uses the current MSHTML control to 
 * render the thumbnails
 */

#include "precomp.h"

#include <urlmon.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <idispids.h>
#include <ocidl.h>
#include <optary.h>

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

// the default size to render to ....
#define DEFSIZE_RENDERWIDTH     640
#define DEFSIZE_RENDERHEIGHT    640

WCHAR const c_szProfile[] = L"InternetShortcut";
WCHAR const c_szURLValue[] = L"URL";
WCHAR const c_szNULL[] = L"";

CHAR const c_szRegValueTimeout[] = "HTMLExtractTimeout";

#define PROGID_HTML	    L"htmlfile"
#define PROGID_MHTML    L"mhtmlfile"
#define PROGID_XML      L"xmlfile"

#define DLCTL_DOWNLOAD_FLAGS  ( DLCTL_DLIMAGES | \
                                DLCTL_VIDEOS | \
                                DLCTL_NO_DLACTIVEXCTLS | \
                                DLCTL_NO_RUNACTIVEXCTLS | \
                                DLCTL_NO_JAVA | \
                                DLCTL_NO_SCRIPTS | \
                                DLCTL_SILENT )

// Register the classes.

///////////////////////////////////////////////////////////////////////////////////////////
CHtmlThumb::CHtmlThumb( )
{
    m_lState = IRTIR_TASK_NOT_RUNNING;
    Assert( !m_fAsync );
    Assert( !m_hDone );
    Assert( !m_pHTML );
    Assert( !m_pOleObject );
    Assert( !m_idError );
    Assert( !m_pMoniker );

    m_dwXRenderSize = DEFSIZE_RENDERWIDTH;
    m_dwYRenderSize = DEFSIZE_RENDERHEIGHT;
}

///////////////////////////////////////////////////////////////////////////////////////////
CHtmlThumb::~CHtmlThumb()
{
    // make sure we are not registered for callbacks...
    if ( m_pConPt )
    {
        m_pConPt->Unadvise( m_dwPropNotifyCookie );
        ATOMICRELEASE( m_pConPt );
    }
    
    if ( m_hDone )
    {
        CloseHandle( m_hDone );
    }

    ATOMICRELEASE( m_pHTML );
    ATOMICRELEASE( m_pOleObject );
    ATOMICRELEASE( m_pViewObject );
    ATOMICRELEASE( m_pMoniker );
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Run()
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Kill( BOOL fWait )
{
    if ( m_lState == IRTIR_TASK_RUNNING )
    {
        LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_PENDING );
        if ( lRes == IRTIR_TASK_FINISHED )
        {
            m_lState = lRes;
        }
        else if ( m_hDone )
        {
            // signal the event it is likely to be waiting on
            SetEvent( m_hDone );
        }
        
        return NOERROR;
    }
    else if ( m_lState == IRTIR_TASK_PENDING || m_lState == IRTIR_TASK_FINISHED )
    {
        return S_FALSE;
    }

    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Suspend( )
{
    if ( m_lState != IRTIR_TASK_RUNNING )
    {
        return E_FAIL;
    }

    // suspend ourselves
    LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_SUSPENDED);
    if ( lRes == IRTIR_TASK_FINISHED )
    {
        m_lState = lRes;
        return NOERROR;
    }

    // if it is running, then there is an Event Handle, if we have passed where
    // we are using it, then we are close to finish, so it will ignore the suspend
    // request
    Assert( m_hDone );
    SetEvent( m_hDone );
    
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Resume( )
{
    if ( m_lState != IRTIR_TASK_SUSPENDED )
    {
        return E_FAIL;
    }
    
    ResetEvent( m_hDone );
    m_lState = IRTIR_TASK_RUNNING;
    
    // the only point we allowed for suspension was in the wait loop while
    // trident is doing its stuff, so we just restart where we left off...
    SetWaitCursor();
    HRESULT hr = InternalResume();   
    ResetWaitCursor();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CHtmlThumb::IsRunning()
{
    return m_lState;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::OnChanged( DISPID dispID)
{
    if ( DISPID_READYSTATE == dispID && m_pHTML && m_hDone )
    {
        CheckReadyState();
    }

    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::OnRequestEdit ( DISPID dispID)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////////////////
// IExtractImage
STDMETHODIMP CHtmlThumb::GetLocation ( LPWSTR pszPathBuffer,
                                       DWORD cch,
                                       DWORD * pdwPriority,
                                       const SIZE * prgSize,
                                       DWORD dwRecClrDepth,
                                       DWORD *pdwFlags )
{
    if ( !pszPathBuffer || !pdwFlags || ((*pdwFlags & IEIFLAG_ASYNC) && !pdwPriority ) || !prgSize)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = NOERROR;

    m_rgSize = *prgSize;
    m_dwClrDepth = dwRecClrDepth;

    // fix the max colour depth at 8 bit, otherwise we are going to allocate a boat load of
    // memory just to scale the thing down.
    DWORD dwColorRes = GetCurColorRes();
    
    if ( m_dwClrDepth > dwColorRes && dwColorRes >= 8)
    {
        m_dwClrDepth = dwColorRes;
    }
    
    if ( *pdwFlags & IEIFLAG_SCREEN )
    {
        HDC hdc = GetDC( NULL );
        
        m_dwXRenderSize = GetDeviceCaps( hdc, HORZRES );
        m_dwYRenderSize = GetDeviceCaps( hdc, VERTRES );

        ReleaseDC( NULL, hdc );
    }
    if ( *pdwFlags & IEIFLAG_ASPECT )
    {
        m_fAspect = TRUE;
        
        // scale the rect to the same proportions as the new one passed in
        if ( m_rgSize.cx > m_rgSize.cy )
        {
            // make the Y size bigger
            m_dwYRenderSize = (m_dwYRenderSize * m_rgSize.cy)/ m_rgSize.cx;
        }
        else
        {
            // make the X size bigger
            m_dwXRenderSize = (m_dwXRenderSize * m_rgSize.cx)/ m_rgSize.cy;
        }            
    }

    // scale our drawing size to match the in
    if ( *pdwFlags & IEIFLAG_ASYNC )
    {
        hr = E_PENDING;

        // much lower priority, this task could take ages ...
        *pdwPriority = 0x00010000;
        m_fAsync = TRUE;
    }

    m_Host.m_fOffline = BOOLIFY( *pdwFlags & IEIFLAG_OFFLINE );
    
    if ( m_pMoniker )
    {
        LPOLESTR pszName = NULL;
        hr = m_pMoniker->GetDisplayName( NULL, NULL, &pszName );
        if ( SUCCEEDED( hr ))
        {
            StrCpyNW( pszPathBuffer, pszName, cch );
            CoTaskMemFree( pszName );
        }
    }
    else
    {
        StrCpyNW( pszPathBuffer, m_szPath, cch );
    }

    *pdwFlags = IEIFLAG_CACHE;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Extract ( HBITMAP * phBmpThumbnail)
{
    if ( m_lState != IRTIR_TASK_NOT_RUNNING )
    {
        return E_FAIL;
    }

    // check we were initialized somehow..
    if ( m_szPath[0] == 0 && !m_pMoniker )
    {
        return E_FAIL;
    }

    // we use manual reset so that once fired we will always get it until we reset it...
    m_hDone = CreateEventA( NULL, TRUE, TRUE, NULL );
    if ( !m_hDone )
    {
        return E_OUTOFMEMORY;
    }
    ResetEvent( m_hDone );
    
    // the one thing we cache is the place where the result goes....
    m_phBmp = phBmpThumbnail; 
    
    LPMONIKER pURLMoniker = NULL;
    CLSID clsid;
    LPUNKNOWN pUnk = NULL;
    IConnectionPointContainer * pConPtCtr = NULL;
    LPCWSTR pszDot = NULL;
    BOOL fUrl = FALSE;
    LPCWSTR pszProgID = NULL;
    
    if ( !m_pMoniker )
    {
        // work out what the extension is....
        pszDot = StrRChrW( m_szPath, NULL, WCHAR('.') );
        if ( pszDot == NULL )
        {
            return E_UNEXPECTED;
        }

        // check for what file type it is ....
        if ( StrCmpIW( pszDot, L".htm" ) == 0 || 
             StrCmpIW( pszDot, L".html" ) == 0 ||
             StrCmpIW( pszDot, L".url" ) == 0 )
        {
            pszProgID = PROGID_HTML;
        }
        else if ( StrCmpIW( pszDot, L".mht" ) == 0 ||
                  StrCmpIW( pszDot, L".mhtml" ) == 0 ||
                  StrCmpIW( pszDot, L".eml" ) == 0 ||
                  StrCmpIW( pszDot, L".nws" ) == 0 )
        {
            pszProgID = PROGID_MHTML;
        }
        else if ( StrCmpIW( pszDot, L".xml" ) == 0 )
        {
            pszProgID = PROGID_XML;
        }
        else
            return E_INVALIDARG;
    }
    
    HRESULT hr = NOERROR;

    LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_RUNNING );
    if ( lRes == IRTIR_TASK_PENDING )
    {
        ResetWaitCursor();
        m_lState = IRTIR_TASK_FINISHED;
        return E_FAIL;
    }

    LPWSTR pszFullURL = NULL;
        
    if ( m_pMoniker )
    {
        pURLMoniker = m_pMoniker;
        pURLMoniker->AddRef();
    }
    else if ( StrCmpIW( pszDot, L".url" ) == 0 )
    {
        hr = Create_URL_Moniker( &pURLMoniker );
        if ( FAILED( hr ))
        {
            return hr;
        }
        fUrl = TRUE;
    }

    SetWaitCursor();
    
    // reached here with a valid URL Moniker hopefully.....
    // or we are to use the text string and load it from a file ...
    // now fish around in the registry for the data for the MSHTML control ...

    m_idError = MSG_THUMBNAIL_ERROR;
    hr = CLSIDFromProgID( pszProgID, &clsid );
    if (hr == S_OK)
    {
        m_idError = MSG_THUMBNAIL_ERROR;
        hr = CoCreateInstance( clsid,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IUnknown,
                               (void **)&pUnk);
    }

    if ( SUCCEEDED( hr ))
    {
        // now set the extent of the object ....
        hr = pUnk->QueryInterface( IID_IOleObject, (LPVOID * ) &m_pOleObject );
    }

    if ( SUCCEEDED( hr ))
    {
        // give trident to our hosting sub-object...
        hr = m_Host.SetTrident( m_pOleObject );
    }
    
    if ( SUCCEEDED( hr ))
    {
        m_idError = MSG_THUMBNAIL_ERROR;
        hr = pUnk->QueryInterface( IID_IViewObject,
                                   (void **) &m_pViewObject);
    }

    // try and load the file, either through the URL moniker or
    // via IPersistFile::Load()
    if ( SUCCEEDED( hr ))
    {
        m_idError = MSG_THUMBNAIL_ERROR;
        if ( pURLMoniker != NULL )
        {
            IBindCtx *pbc = NULL;
            IPersistMoniker * pPersistMon = NULL;
                
            // have reached here with the interface I need ...  
            hr = pUnk->QueryInterface( IID_IPersistMoniker,
                                        (void **)&pPersistMon );

            if ( SUCCEEDED( hr ))
            {
                hr = CreateBindCtx(0, &pbc);
            }
            if ( SUCCEEDED( hr ) && fUrl )
            {
                IHtmlLoadOptions *phlo;
                hr = CoCreateInstance(CLSID_HTMLLoadOptions, NULL, CLSCTX_INPROC_SERVER,
                    IID_IHtmlLoadOptions, (LPVOID *)&phlo);

                if (SUCCEEDED(hr))
                {
                    // deliberately ignore failures here
                    phlo->SetOption(HTMLLOADOPTION_INETSHORTCUTPATH, m_szPath, (lstrlenW(m_szPath) + 1)*sizeof(WCHAR));
                    pbc->RegisterObjectParam(L"__HTMLLOADOPTIONS", phlo);
                }
                phlo->Release();
            }
            
            m_idError = MSG_HTML_OUTOFMEMORY;
            if ( SUCCEEDED( hr ))
            {            
                //tell MSHTML to start to load the page given
                hr = pPersistMon->Load(TRUE, pURLMoniker, pbc, NULL);
            }

            if ( pPersistMon )
            {
                pPersistMon->Release();
            }

            if ( pbc )
            {
                pbc->Release();
            }
        }
        else
        {
            LPPERSISTFILE pPersistFile = NULL;
            hr = pUnk->QueryInterface( IID_IPersistFile, (void **)&pPersistFile);
            if ( SUCCEEDED( hr ))
            {
                hr = pPersistFile->Load( m_szPath, STGM_READ | STGM_SHARE_DENY_NONE );
                pPersistFile->Release();
            }
        }
    }

    if ( pURLMoniker != NULL )
    {
        pURLMoniker->Release();
    }

    if ( SUCCEEDED( hr ))
    {
        SIZEL rgSize;
        rgSize.cx = m_dwXRenderSize;
        rgSize.cy = m_dwYRenderSize;
        
        HDC hDesktopDC = GetDC( GetDesktopWindow());
 
        // convert the size to himetric
        rgSize.cx = ( rgSize.cx * 2540 ) / GetDeviceCaps( hDesktopDC, LOGPIXELSX );
        rgSize.cy = ( rgSize.cy * 2540 ) / GetDeviceCaps( hDesktopDC, LOGPIXELSY );
            
        hr = m_pOleObject->SetExtent( DVASPECT_CONTENT, & rgSize );
        ReleaseDC( GetDesktopWindow(), hDesktopDC );
    }

    if ( SUCCEEDED( hr ))
    {
        hr = pUnk->QueryInterface( IID_IHTMLDocument2, (LPVOID *) & m_pHTML );
    }

    if ( pUnk )
    {
        pUnk->Release();
    }
    
    if ( SUCCEEDED( hr ))
    {
        // get the timeout from the registry....
        m_dwTimeout = 0;
        DWORD cbSize = sizeof( m_dwTimeout );
        HKEY hKey;
            
        lRes = RegOpenKeyA( HKEY_CURRENT_USER,
                           REGSTR_THUMBNAILVIEW,
                           & hKey );

        if ( lRes == ERROR_SUCCESS )
        {
            lRes = RegQueryValueExA( hKey,
                                    c_szRegValueTimeout,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &m_dwTimeout,
                                    &cbSize );
            RegCloseKey( hKey );
        }

        if ( m_dwTimeout == 0 )
        {
            m_dwTimeout = TIME_DEFAULT;
        }

        // adjust to milliseconds
        m_dwTimeout *= 1000;
 
        // register the connection point for notification of the readystate
        hr = m_pOleObject->QueryInterface( IID_IConnectionPointContainer, (LPVOID*) &pConPtCtr);
    }

    if ( SUCCEEDED( hr ))
    {
        hr = pConPtCtr->FindConnectionPoint( IID_IPropertyNotifySink, &m_pConPt );
    }
    if ( pConPtCtr )
    {
        pConPtCtr->Release();
    }

    if ( SUCCEEDED( hr ))
    {
        hr = m_pConPt->Advise((IPropertyNotifySink *) this, &m_dwPropNotifyCookie );
    }

    if ( SUCCEEDED( hr ))
    {
        m_dwCurrentTick = 0;

        // delegate to the shared function
        hr = InternalResume();
    }

    ResetWaitCursor();

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// this function is called from either Create or from 
HRESULT CHtmlThumb::InternalResume( )
{
    HRESULT hr = WaitForRender();

    // if we getE_PENDING, we will drop out of Run()
    
    // all errors and succeeds excepting Suspend() need to Unadvise the 
    // connection point
    if ( hr != E_PENDING )
    {
        // unregister the connection point ...
        m_pConPt->Unadvise( m_dwPropNotifyCookie );
        ATOMICRELEASE( m_pConPt );
    }
            
    if ( m_lState == IRTIR_TASK_PENDING )
    {
        // we were told to quit, so do it...
        hr = E_FAIL;
    }

    if ( SUCCEEDED( hr ))
    {
        hr = Finish( m_phBmp, &m_rgSize, m_dwClrDepth);
    }

    if ( FAILED( hr ) && !m_fAsync )
    {
        ReportError(NULL);
    }

    if ( m_lState != IRTIR_TASK_SUSPENDED )
    {
        m_lState = IRTIR_TASK_FINISHED;
    }

    if ( hr != E_PENDING )
    {
        // we are not being suspended, so we don't need any of this stuff anymore so ...
        // let it go here so that its on the same thread as where we created it...
        ATOMICRELEASE( m_pHTML );
        ATOMICRELEASE( m_pOleObject );
        ATOMICRELEASE( m_pViewObject );
        ATOMICRELEASE( m_pMoniker );
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
HRESULT CHtmlThumb::WaitForRender( )
{
    DWORD dwLastTick = GetTickCount();
    CheckReadyState();

    do
    {
        MSG msg;
        while ( PeekMessageWrapW( &msg, NULL, 0, 0, PM_REMOVE ))
        {
            if (( msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST ) ||
                ( msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST  && msg.message != WM_MOUSEMOVE ))
            {
                continue;
            }
            TranslateMessage( &msg );
            DispatchMessageWrapW( &msg );
        }

        HANDLE rgWait[1] = {m_hDone};
        DWORD dwRet = MsgWaitForMultipleObjects( 1,
                                                 rgWait,
                                                 FALSE,
                                                 m_dwTimeout - m_dwCurrentTick,
                                                 QS_ALLINPUT );

        if ( dwRet != WAIT_OBJECT_0 )
        {
            // check the handle, TRIDENT pumps LOADS of messages, so we may never
            // detect the handle fired, even though it has...
            DWORD dwTest = WaitForSingleObject( m_hDone, 0 );
            if ( dwTest == WAIT_OBJECT_0 )
            {
                break;
            }
        }
        
        if ( dwRet == WAIT_OBJECT_0 )
        {
            // Done signalled (either killed, or complete)
            break;
        }

        // was it a message that needed processing ?
        if ( dwRet != WAIT_TIMEOUT )
        {
            DWORD dwNewTick = GetTickCount();
            if ( dwNewTick < dwLastTick )
            {
                // it wrapped to zero, 
                m_dwCurrentTick += dwNewTick + (0xffffffff - dwLastTick );
            }
            else
            {
                m_dwCurrentTick += (dwNewTick - dwLastTick);
            }
            dwLastTick = dwNewTick;
        }

        if (( m_dwCurrentTick > m_dwTimeout ) || ( dwRet == WAIT_TIMEOUT ))
        {
            Assert( m_pOleObject );
            
            m_pOleObject->Close( OLECLOSE_NOSAVE );
            
            m_idError = MSG_HTML_TIMEOUT;
            
            return E_FAIL;
        }
    }
    while ( TRUE );

    if ( m_lState == IRTIR_TASK_SUSPENDED )
    {
        return E_PENDING;
    }

    if ( m_lState == IRTIR_TASK_PENDING )
    {
        // it is being killed,
        // close the renderer down...
        m_pOleObject->Close( OLECLOSE_NOSAVE );
    }
    
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////
HRESULT CHtmlThumb::Finish( HBITMAP * phBmp, const SIZE * prgSize, DWORD dwClrDepth )
{
    HRESULT hr = NOERROR;
    m_idError = MSG_HTML_OUTOFRESOURCES;
    HWND hwndDesktop = GetDesktopWindow();
    HDC hDesktopDC = GetDC( hwndDesktop );
    RECTL rcLRect;
    HBITMAP hOldBmp = NULL;
    HBITMAP hHTMLBmp = NULL;
    BITMAPINFO * pBMI = NULL;
    LPVOID pBits;
    HPALETTE hpal = NULL;;
    
        // size is in pixels...
    SIZEL rgSize;
    rgSize.cx = m_dwXRenderSize;
    rgSize.cy = m_dwYRenderSize;
            
    //draw into temp DC
    HDC hdcHTML = CreateCompatibleDC( hDesktopDC );
    if ( hdcHTML == NULL )
    {
        hr = E_OUTOFMEMORY;
    }

    if ( dwClrDepth == 8 )
    {
        // use the shell's palette as the default
        hpal = SHCreateShellPalette( hDesktopDC );
    }
    else if ( dwClrDepth == 4 )
    {
        // use the stock 16 colour palette...
        hpal = (HPALETTE) GetStockObject( DEFAULT_PALETTE );
    }
    
    if ( SUCCEEDED( hr ))
    {
        CreateSizedDIBSECTION( &rgSize, dwClrDepth, hpal, NULL, &hHTMLBmp, &pBMI, &pBits );
        if ( hHTMLBmp == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED( hr ))
    {
        m_idError = MSG_THUMBNAIL_ERROR;
        hOldBmp = (HBITMAP) SelectObject(hdcHTML, hHTMLBmp);
            
        BitBlt( hdcHTML, 
                0, 
                0, rgSize.cx, 
                rgSize.cy, 
                NULL, 0, 0, WHITENESS);

        /****************** RENDER HTML PAGE to MEMORY DC *******************
        
          We will now call IViewObject::Draw in MSHTML to render the loaded
          URL into the passed in hdcMem.  The extent of the rectangle to
          render in is in RectForViewObj.  This is the call that gives
          us the image to scroll, animate, or whatever!
            
        ********************************************************************/
        rcLRect.left = 0;
        rcLRect.right = rgSize.cx;
        rcLRect.top = 0;
        rcLRect.bottom = rgSize.cy;

        hr = m_pViewObject->Draw(DVASPECT_CONTENT,
                            NULL, NULL, NULL, NULL,
                            hdcHTML, &rcLRect,
                            NULL, NULL, NULL);         

                            
        SelectObject( hdcHTML, hOldBmp );
    }

    if ( SUCCEEDED( hr ))
    {
        SIZEL rgCur;
        rgCur.cx = rcLRect.bottom;
        rgCur.cy = rcLRect.right;

        Assert( pBMI );
        Assert( pBits );
        hr = ConvertDIBSECTIONToThumbnail( pBMI, pBits, phBmp, prgSize, dwClrDepth, hpal, 50, FALSE );
    }

    if ( hHTMLBmp )
    {
        DeleteObject( hHTMLBmp );
    }
    
    if ( hDesktopDC )
    {
        ReleaseDC( hwndDesktop, hDesktopDC );
    }

    if ( hdcHTML )
    {
        DeleteDC( hdcHTML );
    }

    if ( pBMI )
    {
        LocalFree( pBMI );
    }
    if ( hpal )
    {
        DeletePalette( hpal );
    }
    
    return hr;
}
        
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CHtmlThumb::CheckReadyState( )
{
    VARIANT     varState;
    DISPPARAMS  dp;


    if ( !m_pHTML )
    {
        Assert( FALSE );
        return E_UNEXPECTED;
    }    
    
    VariantInit(&varState);

    if (SUCCEEDED(m_pHTML->Invoke(DISPID_READYSTATE, 
                          IID_NULL, 
                          GetUserDefaultLCID(), 
                          DISPATCH_PROPERTYGET, 
                          &dp, 
                          &varState, NULL, NULL)) &&
        V_VT(&varState)==VT_I4 && 
        V_I4(&varState)== READYSTATE_COMPLETE)
    {
        // signal the end ..
        SetEvent( m_hDone );
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////
HRESULT CHtmlThumb::Create_URL_Moniker( LPMONIKER * ppMoniker )
{
    // we are dealing with a URL file
    WCHAR szURLData[8196];

    // URL files are actually ini files that
    // have a section [InternetShortcut]
    // and a Value URL=.....
    int iRet = SHGetIniStringW(c_szProfile, c_szURLValue,
            szURLData, ARRAYSIZE(szURLData), m_szPath);

    if ( iRet == 0 )
    {
        if ( !m_fAsync )
        {
            m_idError = MSG_THUMBNAIL_ERROR;
            ReportError(NULL);
        }
                
        return E_FAIL;
    }

    HRESULT hr = CreateURLMoniker( 0, szURLData, ppMoniker );

    if ( FAILED( hr ))
    {
        if ( !m_fAsync )
        {
            m_idError = MSG_THUMBNAIL_ERROR;

            ReportError( NULL );
        }            
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
void CHtmlThumb::ReportError( LPVOID * pDefMsgArgs )
{
    LPVOID pMsgArgs[3];
    LPVOID pTitleArgs[1];
    
    pTitleArgs[0] = NULL;

    if ( !m_idError )
    {
        m_idError = MSG_THUMBNAIL_ERROR;
    }

    if ( !pDefMsgArgs )
    {
        // if no args, then use the file name ...
        pDefMsgArgs = pMsgArgs;
    }

    pMsgArgs[0] = PathFindFileNameW(m_szPath);

    FormatMessageBox( NULL,
                      m_idError,
                      MSG_ERROR_TITLE,
                      pDefMsgArgs,
                      pTitleArgs,
                      m_idErrorIcon,
                      MB_OK );
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::IsDirty()
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Load( LPCOLESTR pszFileName, DWORD dwMode)
{
    if ( !pszFileName )
    {
        return E_INVALIDARG;
    }
    
    StrCpyW( m_szPath, pszFileName );
    DWORD dwAttrs = GetFileAttributesWrapW( m_szPath );
    if (( dwAttrs != (DWORD) -1) && (dwAttrs & FILE_ATTRIBUTE_OFFLINE ))
    {
        return E_FAIL;
    }
    
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Save( LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::SaveCompleted( LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::GetCurFile( LPOLESTR *ppszFileName)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
HRESULT UnregisterHTMLExtractor()
{
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////
HRESULT RegisterHTMLExtractor()
{
    
    LONG lRes = 0;
    HKEY hKey;

    lRes = RegCreateKeyA( HKEY_CURRENT_USER, REGSTR_THUMBNAILVIEW, &hKey );
    if ( lRes != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    DWORD dwTimeout = TIME_DEFAULT;
    
    lRes = RegSetValueExA( hKey,
                          c_szRegValueTimeout,
                          NULL,
                          REG_DWORD,
                          (const LPBYTE) &dwTimeout,
                          sizeof( dwTimeout ));

    RegCloseKey( hKey );
    if ( lRes != ERROR_SUCCESS )
    {
        return E_FAIL;
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////
UINT FormatMessageBox( HWND hwnd, UINT idMsg, UINT idTitle, 
                       LPVOID * pArgsMsg, LPVOID * pArgsTitle,
                       UINT idIconResource, UINT uFlags )
{
    WCHAR szTitle[MAX_PATH*4];
    WCHAR szMsg[MAX_PATH * 8];

    szTitle[0] = 0;
    szMsg[0] = 0;

    DWORD dwWidth = 50;

    FormatMessageWrapW( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
               g_hinstDll,
               idTitle,
               0,
               szTitle,
               MAX_PATH * 4,
               (va_list *) pArgsTitle );

    FormatMessageWrapW( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | dwWidth,
               g_hinstDll,
               idMsg,
               0,
               szMsg,
               MAX_PATH * 8,
               (va_list *) pArgsMsg );

    if ( idIconResource != 0 )
    {
        // use MessageBoxIndirect
        MSGBOXPARAMSW rgParams;
        ZeroMemory( & rgParams, sizeof( rgParams ));
        rgParams.cbSize = sizeof( rgParams );
        rgParams.hwndOwner = hwnd;
        rgParams.hInstance = g_hinstDll;
        rgParams.lpszText = szMsg;
        rgParams.lpszCaption = szTitle;
        rgParams.dwStyle = MB_USERICON | MB_SETFOREGROUND | uFlags;
        rgParams.lpszIcon = (LPWSTR) MAKEINTRESOURCE( idIconResource );
        rgParams.dwContextHelpId = NULL;
        rgParams.lpfnMsgBoxCallback = NULL;
        rgParams.dwLanguageId = GetSystemDefaultLangID();
        
        return MessageBoxIndirectWrapW( &rgParams );
    }
    else
    {
        return MessageBoxWrapW( hwnd, szMsg, szTitle, uFlags | MB_SETFOREGROUND );
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////TridentHost///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
CTridentHost::CTridentHost()
{
    m_cRef = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////
CTridentHost::~CTridentHost()
{
    // all refs should have been released...
    Assert( m_cRef == 1 );
}

///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CTridentHost::SetTrident( IOleObject * pTrident )
{
    Assert( pTrident );

    return pTrident->SetClientSite(( IOleClientSite *) this );
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost::QueryInterface( REFIID riid, LPVOID * ppvObj )
{
    if ( !ppvObj )
    {
        return E_INVALIDARG;
    }

    if ( IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_IOleClientSite ) 
         || IsEqualIID(riid, IID_IThumbnailView ))
    {
        *ppvObj = (IOleClientSite *) this;
    }
    else if ( IsEqualIID( riid, IID_IDispatch ))
    {
        *ppvObj = (IDispatch *) this;
    }
    else if ( IsEqualIID( riid, IID_IDocHostUIHandler ))
    {
        *ppvObj = (IDocHostUIHandler *) this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    this->AddRef();
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CTridentHost::AddRef ( void )
{
    m_cRef ++;

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CTridentHost:: Release ( void )
{
    m_cRef --;

    // because we are created with our parent, we do not do a delete here..
    Assert( m_cRef > 0 );

    return m_cRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost::GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                                  DISPPARAMS *pdispparams, VARIANT *pvarResult,
                                  EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    if (!pvarResult)
        return E_INVALIDARG;

    Assert(pvarResult->vt == VT_EMPTY);

    if (wFlags == DISPATCH_PROPERTYGET)
    {
        switch (dispidMember)
        {
        case DISPID_AMBIENT_DLCONTROL :
            pvarResult->vt = VT_I4;
            pvarResult->lVal = DLCTL_DOWNLOAD_FLAGS;
            if ( m_fOffline )
            {
                pvarResult->lVal |= DLCTL_OFFLINE;
            }
            return NOERROR;
            
         case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
            pvarResult->vt = VT_BOOL;
            pvarResult->boolVal = m_fOffline ? TRUE : FALSE;
            return NOERROR;

        case DISPID_AMBIENT_SILENT:
            pvarResult->vt = VT_BOOL;
            pvarResult->boolVal = TRUE;
            return NOERROR;
        }
    }

    return DISP_E_MEMBERNOTFOUND;
}


// IOleClientSite
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: SaveObject ()
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: GetMoniker (DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
	return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: GetContainer (IOleContainer **ppContainer)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: ShowObject ()
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: OnShowWindow (BOOL fShow)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: RequestNewObjectLayout ()
{
	return E_NOTIMPL;
}

// IPersistMoniker stuff
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CHtmlThumb::Load ( BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    if ( !pimkName )
    {
        return E_INVALIDARG;
    }
    if ( pibc || grfMode != STGM_READ )
    {
        return E_NOTIMPL;
    }

    if ( m_pMoniker )
    {
        m_pMoniker->Release();
    }

    m_pMoniker = pimkName;
    Assert( m_pMoniker );
    m_pMoniker->AddRef();

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::Save ( IMoniker *pimkName, LPBC pbc, BOOL fRemember)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::SaveCompleted ( IMoniker *pimkName, LPBC pibc)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CHtmlThumb::GetCurMoniker ( IMoniker **ppimkName)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: ShowContextMenu ( DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: GetHostInfo ( DOCHOSTUIINFO *pInfo)
{
    if ( !pInfo )
    {
        return E_INVALIDARG;
    }

    DWORD   dwIE = URL_ENCODING_NONE;
    DWORD   dwOutLen = sizeof(DWORD);
    
    UrlMkGetSessionOption(URLMON_OPTION_URL_ENCODING, &dwIE, sizeof(DWORD), &dwOutLen, NULL);

    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
    pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
    if (dwIE == URL_ENCODING_ENABLE_UTF8)
    {
        pInfo->dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8;
    }
    else
    {
        pInfo->dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8;
    }
    
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: ShowUI ( DWORD dwID,
                                     IOleInPlaceActiveObject *pActiveObject,
                                     IOleCommandTarget *pCommandTarget,
                                     IOleInPlaceFrame *pFrame,
                                     IOleInPlaceUIWindow *pDoc)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: HideUI ( void)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: UpdateUI ( void)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: EnableModeless ( BOOL fEnable)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: OnDocWindowActivate ( BOOL fActivate)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: OnFrameWindowActivate ( BOOL fActivate)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: ResizeBorder ( LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: TranslateAccelerator ( LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: GetOptionKeyPath ( LPOLESTR *pchKey, DWORD dw)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: GetDropTarget ( IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: GetExternal ( IDispatch **ppDispatch)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: TranslateUrl ( DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTridentHost:: FilterDataObject ( IDataObject *pDO, IDataObject **ppDORet)
{
    return E_NOTIMPL;
}


