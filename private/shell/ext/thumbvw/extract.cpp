#include "precomp.h"
#include "ocmm.h"

CHAR const c_szShellEx[] = "ShellEx";

UINT CalcImageSize( const SIZE * prgSize, DWORD dwClrDepth );

CThumbnailShrinker::CThumbnailShrinker()
{
}

CThumbnailShrinker::~CThumbnailShrinker()
{
}

STDMETHODIMP CThumbnailShrinker::ScaleSharpen2( BITMAPINFO * pbi,
                                                LPVOID pBits,
                                                HBITMAP * phBmpThumbnail,
                                                const SIZE * prgSize,
                                                DWORD dwRecClrDepth,
                                                HPALETTE hpal,
                                                UINT uiSharpPct,
                                                BOOL fOrigSize )
{
    BOOL fRes = ConvertDIBSECTIONToThumbnail( pbi,
                                              pBits,
                                              phBmpThumbnail,
                                              prgSize,
                                              dwRecClrDepth,
                                              hpal,
                                              uiSharpPct,
                                              fOrigSize );
    return ( fRes ) ? NOERROR : E_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////
BOOL ConvertDIBSECTIONToThumbnail( BITMAPINFO * pbi,
                                   LPVOID pBits,
                                   HBITMAP * phBmpThumbnail,
                                   const SIZE * prgSize,
                                   DWORD dwRecClrDepth,
                                   HPALETTE hpal,
                                   UINT uiSharpPct,
                                   BOOL fOrigSize)
{
    IThumbnailMaker *   pThumbMaker = NULL;
    LPBITMAPINFO        pbiScaled = pbi, pbiUsed = pbi;
    LPBITMAPINFOHEADER  pbih = ( LPBITMAPINFOHEADER )pbi;
    BOOL                bRetVal = FALSE, bInverted = FALSE;
    RECT                rect;
    HRESULT             hr;
    LPVOID              pScaledBits = pBits;

    // the scaling code doesn't handle inverted bitmaps, so we treat
    // them as if they were normal, by inverting the height here and
    // then setting it back before doing a paint.
    if ( pbi->bmiHeader.biHeight < 0 )
    {
        pbi->bmiHeader.biHeight *= -1;
        bInverted = TRUE;
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = pbih->biWidth;
    rect.bottom = pbih->biHeight;
    
    CalculateAspectRatio( prgSize, &rect );

    // only both with the scaling and sharpening if we are messing with the size...
    if (( rect.right - rect.left != pbih->biWidth ) || ( rect.bottom - rect.top != pbih->biHeight ))
    {
        hr = ThumbnailMaker_Create( &pThumbMaker );
        if ( SUCCEEDED( hr ))
        {
            // initialize thumbnail maker. 
            hr = pThumbMaker->Init( rect.right - rect.left, rect.bottom - rect.top, 
                                    pbi->bmiHeader.biWidth, abs( pbi->bmiHeader.biHeight ) );
        }

        if ( SUCCEEDED( hr ) )
        {
            // scale image.
            hr = pThumbMaker->AddDIBSECTION( pbiUsed, pBits );
            if ( SUCCEEDED( hr ) )
            {
                DWORD   dwSize;
                hr = pThumbMaker->GetSharpenedBITMAPINFO( uiSharpPct, &pbiScaled, &dwSize );
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            pScaledBits = ( LPBYTE )pbiScaled + sizeof( BITMAPINFOHEADER );
        }

        if ( pThumbMaker )
        {
            pThumbMaker->Release( );
        }

        if ( FAILED( hr ))
        {
            return hr;
        }
    }

    // set the height back to negative if that's the way it was before.
    if ( bInverted == TRUE )
        pbiScaled->bmiHeader.biHeight *= -1;

    // now if they have asked for origsize rather than the boxed one, and the colour depth is OK, then 
    // return it...
    if ( fOrigSize && pbiScaled->bmiHeader.biBitCount <= dwRecClrDepth )
    {
        SIZE rgCreateSize = { pbiScaled->bmiHeader.biWidth, pbiScaled->bmiHeader.biHeight };
        LPVOID pNewBits;
        
        // turn the PbiScaled DIB into a HBITMAP...., note we pass the old biInfo so that it can get the palette form
        // it if need be.
        bRetVal = CreateSizedDIBSECTION( & rgCreateSize, pbiScaled->bmiHeader.biBitCount, NULL, pbiScaled, phBmpThumbnail, NULL, &pNewBits );

        if ( bRetVal )
        {
            // copy the image data accross...
            CopyMemory( pNewBits, pScaledBits, CalcImageSize( &rgCreateSize, pbiScaled->bmiHeader.biBitCount )); 
        }
        
        return bRetVal;
    }
    
    bRetVal = FactorAspectRatio( pbiScaled,
                                 pScaledBits,
                                 prgSize,
                                 rect,
                                 dwRecClrDepth,
                                 hpal,
                                 fOrigSize,
                                 phBmpThumbnail );

    if ( pbiScaled != pbi )
    {
        // free the allocated image...
        CoTaskMemFree( pbiScaled );
    }

    return bRetVal;
}

////////////////////////////////////////////////////////////////////////////////////
// This function makes no assumption about whether the thumbnail is square, so 
// it calculates the scaling ratio for both dimensions and the uses that as
// the scaling to maintain the aspect ratio.
void CalcAspectScaledRect( const SIZE * prgSize, RECT * pRect )
{
    Assert( pRect->left == 0 );
    Assert( pRect->top == 0 );

    int iWidth = pRect->right;
    int iHeight = pRect->bottom;
    int iXRatio = (iWidth * 1000) / prgSize->cx;
    int iYRatio = (iHeight * 1000) / prgSize->cy;

    if ( iXRatio > iYRatio )
    {
        pRect->right = prgSize->cx;
        
        // work out the blank space and split it evenly between the top and the bottom...
        int iNewHeight = (( iHeight * 1000 ) / iXRatio); 
        if ( iNewHeight == 0 )
        {
            iNewHeight = 1;
        }
        
        int iRemainder = prgSize->cy - iNewHeight;

        pRect->top = iRemainder / 2;
        pRect->bottom = iNewHeight + pRect->top;
    }
    else
    {
        pRect->bottom = prgSize->cy;

        // work out the blank space and split it evenly between the left and the right...
        int iNewWidth = (( iWidth * 1000 ) / iYRatio);
        if ( iNewWidth == 0 )
        {
            iNewWidth = 1;
        }
        int iRemainder = prgSize->cx - iNewWidth;
        
        pRect->left = iRemainder / 2;
        pRect->right = iNewWidth + pRect->left;
    }
}
    
void CalculateAspectRatio( const SIZE * prgSize, RECT * pRect )
{
    int iHeight = abs( pRect->bottom - pRect->top );
    int iWidth = abs( pRect->right - pRect->left );

    // check if the initial bitmap is larger than the size of the thumbnail.
    if ( iWidth > prgSize->cx || iHeight > prgSize->cy )
    {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = iWidth;
        pRect->bottom = iHeight;

        CalcAspectScaledRect( prgSize, pRect );
    }
    else
    {
        // if the bitmap was smaller than the thumbnail, just center it.
        pRect->left = ( prgSize->cx - iWidth ) / 2;
        pRect->top = ( prgSize->cy- iHeight ) / 2;
        pRect->right = pRect->left + iWidth;
        pRect->bottom = pRect->top + iHeight;
    }
}

////////////////////////////////////////////////////////////////////////////////////
LPBYTE g_pbCMAP = NULL;
#define CMAP_SIZE 32768

BOOL FactorAspectRatio( LPBITMAPINFO pbiScaled, LPVOID pScaledBits, const SIZE * prgSize, 
                        RECT rect, DWORD dwClrDepth, HPALETTE hpal, BOOL fOrigSize, HBITMAP * phBmpThumbnail )
{
    HWND                hwnd = GetDesktopWindow( );
    HDC                 hdcWnd = GetDC( hwnd );
    HDC                 hdc = CreateCompatibleDC( hdcWnd );
    PBITMAPINFOHEADER   pbih = ( PBITMAPINFOHEADER )pbiScaled;
    BOOL                bRetVal = FALSE;
    int                 iRetVal = GDI_ERROR;
    BITMAPINFO *        pDitheredInfo = NULL;
    LPVOID              pDitheredBits = NULL;
    HBITMAP             hbmpDithered = NULL;

    if ( dwClrDepth == 8 )
    {
        RGBQUAD * pSrcColors = NULL;
        LONG nSrcPitch = pbiScaled->bmiHeader.biWidth;
        
        // we are going to 8 bits per pixel, we had better dither everything 
        // to the same palette, otherwise we are going to suck.
        GUID guidType = CLSID_NULL;
        switch( pbiScaled->bmiHeader.biBitCount )
        {
            case 32:
                guidType = BFID_RGB_32;
                nSrcPitch *= sizeof( DWORD );
                break;
                
            case 24:
                guidType = BFID_RGB_24;
                nSrcPitch *= 3;
                break;
                
            case 16:
                // default is 555
                guidType = BFID_RGB_555;

                // 5-6-5 bitfields has the second DWORD (the green component) as 0x7e00
                if ( pbiScaled->bmiHeader.biCompression == BI_BITFIELDS && 
                     pbiScaled->bmiColors[1].rgbGreen == 0x7E )
                {
                    guidType = BFID_RGB_565;
                }
                nSrcPitch *= sizeof( WORD );
                break;
                
            case 8:
                guidType = BFID_RGB_8;
                pSrcColors = pbiScaled->bmiColors;

                // nSrcPitch is already in bytes...
                break;
        };

        if ( nSrcPitch % 4 )
        {
            // round up to the nearest DWORD...
            nSrcPitch = nSrcPitch + 4 - (nSrcPitch %4);
        }

        // we are going to 8bpp
        LONG nDestPitch = pbiScaled->bmiHeader.biWidth;
        if ( nDestPitch % 4 )
        {
            // round up to the nearest DWORD...
            nDestPitch = nDestPitch + 4 - (nDestPitch % 4);
        }
        
        if ( guidType != CLSID_NULL )
        {

            if ( g_pbCMAP == NULL )
            {
                g_pbCMAP = (LPBYTE) LocalAlloc( LPTR, CMAP_SIZE );
                if ( !g_pbCMAP )
                {
                    return FALSE;
                }

                // we are always going to the shell halftone palette right now, otherwise
                // computing this inverse colour map sucks a lot of time (approx 2 seconds on
                // a p200 )
                if ( FAILED( SHGetInverseCMAP( g_pbCMAP, CMAP_SIZE )))
                {
                    return FALSE;
                }
            }   

            SIZE rgDithered = {pbiScaled->bmiHeader.biWidth, pbiScaled->bmiHeader.biHeight};
            if ( rgDithered.cy < 0 )
            {
                // invert it
                rgDithered.cy = -rgDithered.cy;
            }
            
            if ( CreateSizedDIBSECTION( &rgDithered, dwClrDepth, hpal, NULL, &hbmpDithered, &pDitheredInfo, &pDitheredBits ))
            {
                Assert( pDitheredInfo && pDitheredBits );
                
                // dither....
                IIntDitherer * pDither;
                HRESULT hr = CoCreateInstance( CLSID_IntDitherer,
                                               NULL,
                                               CLSCTX_INPROC_SERVER,
                                               IID_IIntDitherer,
                                               (LPVOID *) &pDither );

                if ( SUCCEEDED( hr ))
                {
                    hr = pDither->DitherTo8bpp ( (LPBYTE) pDitheredBits, nDestPitch, 
                                                 (LPBYTE) pScaledBits, nSrcPitch, guidType, 
                                                 pDitheredInfo->bmiColors, pSrcColors,
                                                 g_pbCMAP, 0, 0, rgDithered.cx, rgDithered.cy,
                                                 -1, -1 );

                    pDither->Release();
                }
                if ( SUCCEEDED( hr ))
                {
                    // if the height was inverted, then invert it in the destination bitmap
                    if ( rgDithered.cy != pbiScaled->bmiHeader.biHeight )
                    {
                        pDitheredInfo->bmiHeader.biHeight = - rgDithered.cy;
                    }
                    
                    // switch to the new image .....
                    pbiScaled = pDitheredInfo;
                    pScaledBits = pDitheredBits;
                }
            }
        }
    }
    
    // create thumbnail bitmap and copy image into it.
    if (CreateSizedDIBSECTION( prgSize, dwClrDepth, hpal, NULL, phBmpThumbnail, NULL, NULL ))
    {
        
        HBITMAP hBmpOld = (HBITMAP) SelectObject( hdc, *phBmpThumbnail );

        SetStretchBltMode( hdc, COLORONCOLOR);

        HGDIOBJ hBrush = GetStockObject( THUMBNAIL_BACKGROUND_BRUSH );
        HGDIOBJ hPen = GetStockObject( THUMBNAIL_BACKGROUND_PEN );

        HGDIOBJ hOldBrush = SelectObject( hdc, hBrush );
        HGDIOBJ hOldPen = SelectObject( hdc, hPen );

        HPALETTE hpalOld;
        if ( hpal )
        {
            hpalOld = SelectPalette( hdc, hpal, TRUE );
            RealizePalette( hdc);
        }
        
        SetMapMode( hdc, MM_TEXT );
        
        Rectangle( hdc, 0, 0, prgSize->cx, prgSize->cy );

        int iDstHt = rect.bottom - rect.top;
        int iDstTop = rect.top, iSrcTop = 0;
        if ( pbih->biHeight < 0 )
        {
            iDstHt *= -1;
            iDstTop = rect.bottom;
            iSrcTop = abs( pbih->biHeight );
        }

        iRetVal = StretchDIBits( hdc, 
                                     rect.left, iDstTop, rect.right - rect.left, iDstHt, 
                                     0, iSrcTop, pbih->biWidth, pbih->biHeight, 
                                     pScaledBits, 
                                     pbiScaled,
                                     DIB_RGB_COLORS, 
                                     SRCCOPY );

        SelectObject( hdc, hOldBrush );
        SelectObject( hdc, hOldPen );
        if ( hpal )
        {
            SelectPalette( hdc, hpalOld, TRUE );
            RealizePalette( hdc );
        }
        
        SelectObject( hdc, hBmpOld );
        ReleaseDC( hwnd, hdcWnd );
        DeleteDC( hdc );
    }

    if ( hbmpDithered )
    {
        DeleteObject( hbmpDithered );
    }
    if ( pDitheredInfo )
    {
        LocalFree( pDitheredInfo );
    }
    
    return ( iRetVal != GDI_ERROR );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT RegisterHandler( LPCSTR pszExts, UINT cExts, UINT cEntrySize, LPCSTR pszIID, LPCSTR pszCLSID )
{
    for ( UINT cKey = 0; cKey < cExts; cKey ++ )
    {
        HKEY hKey1;
        DWORD dwAction;
        LONG lRes = RegCreateKeyExA( HKEY_CLASSES_ROOT,
                                    pszExts,
                                    0,
                                    NULL,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_WRITE | KEY_READ,
                                    NULL,
                                    &hKey1,
                                    &dwAction );

        if ( lRes != ERROR_SUCCESS )
        {
            return E_FAIL;
        }

        // specific APP hack for office95
        CHAR szProgID[MAX_PATH];
        szProgID[0] = 0;
        DWORD cbSize = sizeof( szProgID );

        if ( StrCmpA( pszExts, ".pot") == 0 || StrCmpA( pszExts, ".ppt") == 0 )
        {
            // if they key was created, check to see if the progID is empty
            // this might have been left by us in Beta1
            if ( dwAction != REG_CREATED_NEW_KEY )
            {
                lRes = RegQueryValueExA( hKey1, "", NULL, NULL, (BYTE *) szProgID, &cbSize );
                if ( szProgID[0] == 0 )
                {
                    dwAction = REG_CREATED_NEW_KEY;
                }
            }
            if ( dwAction == REG_CREATED_NEW_KEY )
            {
                const CHAR c_szPPT[] = "Powerpoint.Show.7";
                const CHAR c_szPOT[] = "Powerpoint.Template";

                LPCSTR pszText = ( StrCmpA( pszExts, ".pot") == 0 ) ? c_szPOT : c_szPPT;
                
                RegSetValueExA( hKey1, "", 0, REG_SZ, (const BYTE *)pszText, (lstrlenA( pszText ) + 1 ) * sizeof( CHAR ));
            }
        }
        
        HKEY hKey2;
        lRes = RegCreateKeyA( hKey1, c_szShellEx, &hKey2 );
        RegCloseKey( hKey1 );
        
        if ( lRes != ERROR_SUCCESS )
        {
            return E_FAIL;
        }

        // to check for old versions....
        RegDeleteValueA( hKey2, pszIID );
        lRes = RegSetValueA( hKey2,
                            pszIID,
                            REG_SZ,
                            pszCLSID,
                            lstrlenA( pszCLSID ) * sizeof(CHAR));
        RegCloseKey( hKey2 );
        if ( lRes != ERROR_SUCCESS )
        {
            return E_FAIL;
        }

        pszExts += cEntrySize;
    }
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT UnregisterHandler( LPCSTR pszExts, UINT cExts, UINT cEntrySize, LPCSTR pszIID, LPCSTR pszCLSID )
{
    for ( UINT cKey = 0; cKey < cExts; cKey ++ )
    {
        HKEY hKey1;
        LONG lRes = RegOpenKeyA( HKEY_CLASSES_ROOT, pszExts, &hKey1 );
        if ( lRes != ERROR_SUCCESS )
        {
            continue;
        }

        HKEY hKey2;
        lRes = RegOpenKeyA( hKey1, c_szShellEx, &hKey2 );
        RegCloseKey( hKey1 );
        
        if ( lRes != ERROR_SUCCESS )
        {
            continue;
        }

        lRes = RegDeleteKeyA( hKey2, pszIID );
        RegCloseKey( hKey2 );

        pszExts += cEntrySize;
    }
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////CExtractImageTask///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CExtractImageTask_Create( CThumbnailView * pView,
                                  LPEXTRACTIMAGE pExtract,
                                  LPCWSTR pszCache,
                                  LPCWSTR pszFullPath,
                                  LPCITEMIDLIST pidl,
                                  const FILETIME * pftNewDateStamp,
                                  int iItem,
                                  DWORD dwFlags,
                                  LPRUNNABLETASK * ppTask )
{
    Assert( ppTask );
    Assert( pView );
    Assert( pExtract );
    Assert( pidl );
    
    CExtractImageTask * pNewTask = new CComObject<CExtractImageTask>;
    if ( !pNewTask )
    {
        return E_OUTOFMEMORY;
    }

    pNewTask->m_pView = pView;
    pNewTask->m_pExtract = pExtract;
    pView->InternalAddRef();
    pExtract->AddRef();

    StrCpyNW( pNewTask->m_szCache, pszCache, ARRAYSIZE( pNewTask->m_szCache));
    StrCpyNW( pNewTask->m_szFullPath, pszFullPath, ARRAYSIZE( pNewTask->m_szFullPath ));
    
    pNewTask->m_pidl = ILClone( pidl );
    if (!(pNewTask->m_pidl))
    {
        return E_OUTOFMEMORY;
    }

    pNewTask->m_dwMask = pView->GetOverlayMask( pidl );
    pNewTask->m_dwFlags = dwFlags;

    if ( pftNewDateStamp )
    {
        pNewTask->m_ftDateStamp = * pftNewDateStamp;
    }

    pNewTask->m_fNoDateStamp = (pftNewDateStamp == NULL );

    if ( iItem == -1 )
    {
        pNewTask->m_iItem = pView->FindItem( pidl );
    }
    else
    {
        pNewTask->m_iItem = iItem;
    }

    pNewTask->AddRef();

    *ppTask = pNewTask;
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtractImageTask::Run ( void )
{
    HRESULT hr = E_FAIL;
    if ( m_lState == IRTIR_TASK_RUNNING )
    {
        hr = S_FALSE;
    }
    else if ( m_lState == IRTIR_TASK_PENDING )
    {
        hr = E_FAIL;
    }
    else if ( m_lState == IRTIR_TASK_NOT_RUNNING )
    {
        LONG lRes = InterlockedExchange( & m_lState, IRTIR_TASK_RUNNING);
        if ( lRes == IRTIR_TASK_PENDING )
        {
            m_lState = IRTIR_TASK_FINISHED;
            return NOERROR;
        }

        // see if it supports IRunnableTask
        m_pExtract->QueryInterface( IID_IRunnableTask, (LPVOID *) & m_pTask );

        m_pView->ThreadUpdateStatusBar(IDS_EXTRACTING, m_iItem );
        
        if ( m_lState == IRTIR_TASK_RUNNING )
        {
            // start the extractor....
            hr = m_pExtract->Extract( &m_hBmp );
        }

        if ( SUCCEEDED( hr ) && m_lState == IRTIR_TASK_RUNNING )
        {
            hr = InternalResume();
        }

        if ( SUCCEEDED( hr ) && m_lState == IRTIR_TASK_RUNNING )
        {
            // reset the status bar....
            m_pView->ThreadUpdateStatusBar();
        }
        
        if ( m_lState != IRTIR_TASK_SUSPENDED || hr != E_PENDING )
        {
            m_lState = IRTIR_TASK_FINISHED;
        }
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtractImageTask::Kill ( BOOL fWait )
{
    if ( m_lState != IRTIR_TASK_RUNNING )
    {
        return S_FALSE;
    }

    LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_PENDING );
    if ( lRes == IRTIR_TASK_FINISHED )
    {
        m_lState = lRes;
        return NOERROR;
    }

    // does it support IRunnableTask ? Can we kill it ?
    if ( m_pExtract != NULL )
    {
        LPRUNNABLETASK pTask = NULL;
        HRESULT hr = m_pExtract->QueryInterface( IID_IRunnableTask, (LPVOID *) &pTask );
        if ( SUCCEEDED( hr ))
        {
            hr = pTask->Kill( FALSE );
            pTask->Release();
        }
    }
    return NOERROR;
}

///////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtractImageTask::Suspend( void )
{
    if ( !m_pTask )
    {
        return E_NOTIMPL;
    }

    if ( m_lState != IRTIR_TASK_RUNNING )
    {
        return E_FAIL;
    }

    
    LONG lRes = InterlockedExchange( &m_lState, IRTIR_TASK_SUSPENDED );
    HRESULT hr = m_pTask->Suspend();
    if ( SUCCEEDED( hr ))
    {
        lRes = (LONG) m_pTask->IsRunning();
        if ( lRes == IRTIR_TASK_SUSPENDED )
        {
            if ( lRes != IRTIR_TASK_RUNNING )
            {
                m_lState = lRes;
            }
        }
    }
    else
    {
        m_lState = lRes;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CExtractImageTask::Resume( void )
{
    if ( !m_pTask )
    {
        return E_NOTIMPL;
    }

    if ( m_lState != IRTIR_TASK_SUSPENDED )
    {
        return E_FAIL;
    }

    m_lState = IRTIR_TASK_RUNNING;

    m_pView->ThreadUpdateStatusBar(IDS_EXTRACTING, m_iItem );
    
    HRESULT hr = m_pTask->Resume();
    if ( SUCCEEDED( hr ))
    {
        hr = InternalResume();
    }
    if ( SUCCEEDED( hr ) && m_lState == IRTIR_TASK_RUNNING)
    {
        // reset the status bar....
        m_pView->ThreadUpdateStatusBar( );
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CExtractImageTask::InternalResume()
{
    Assert( m_hBmp != NULL );

    HRESULT hr = m_pView->UpdateImageForItem( m_hBmp, 
                                              m_iItem,
                                              m_pidl,
                                              m_szCache,
                                              m_szFullPath,
                                              ( m_fNoDateStamp ? NULL : &m_ftDateStamp),
                                              (m_dwFlags & IEIFLAG_CACHE) ? TRUE : FALSE );
    
    // reset the status bar....
    m_pView->ThreadUpdateStatusBar();
    
    m_lState = IRTIR_TASK_FINISHED;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CExtractImageTask:: IsRunning ( void )
{
    return m_lState;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CExtractImageTask::~CExtractImageTask()
{
    if ( m_pView )
    {
        m_pView->InternalRelease();
    }
    if ( m_pExtract )
    {
        m_pExtract->Release();
    }
    if ( m_pTask )
    {
        m_pTask->Release();
    }
    
    if ( m_pidl )
    {
        SHFree((LPVOID) m_pidl );
    }

    if ( m_hBmp )
    {
        DeleteObject( m_hBmp );
    }
    
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
CExtractImageTask::CExtractImageTask()
{
    m_lState = IRTIR_TASK_NOT_RUNNING;

    m_pView = NULL;
    m_pExtract = NULL;
    m_pidl = NULL;
    m_hBmp = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CalcImageSize( const SIZE * prgSize, DWORD dwClrDepth )
{
    UINT uSize = prgSize->cx * dwClrDepth;
    
    uSize *= ( prgSize->cy < 0 ) ? (- prgSize->cy) : prgSize->cy;
    // divide by 8
    UINT uRetVal = uSize >> 3;

    if ( uSize & 7 )
    {
        uRetVal ++;
    }

    return uRetVal;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CreateSizedDIBSECTION( const SIZE * prgSize, 
     DWORD dwClrDepth,
     HPALETTE hpal,
     const BITMAPINFO * pCurInfo,
     HBITMAP * phBmp, 
     BITMAPINFO ** ppBMI,
     LPVOID * ppBits )
{
    *phBmp = NULL;
    
    HDC hdc = GetDC( NULL );
    HDC hdcBmp = CreateCompatibleDC( hdc );
    if ( hdcBmp && hdc )
    {
        struct {
            BITMAPINFOHEADER bi;
            DWORD            ct[256];
        } dib;

        dib.bi.biSize            = sizeof(BITMAPINFOHEADER);
        dib.bi.biWidth           = prgSize->cx;
        dib.bi.biHeight          = prgSize->cy;
        dib.bi.biPlanes          = 1;
        dib.bi.biBitCount        = (WORD) dwClrDepth;
        dib.bi.biCompression     = BI_RGB;
        dib.bi.biSizeImage       = CalcImageSize( prgSize, dwClrDepth );
        dib.bi.biXPelsPerMeter   = 0;
        dib.bi.biYPelsPerMeter   = 0;
        dib.bi.biClrUsed         = ( dwClrDepth <= 8 ) ? (1 << dwClrDepth) : 0;
        dib.bi.biClrImportant    = 0;

        HPALETTE hpalOld = NULL;
        
        if ( dwClrDepth <= 8 )
        {
            // if they passed us the old structure with colour info, and we are the same bit depth, then copy it...
            if ( pCurInfo && pCurInfo->bmiHeader.biBitCount == dwClrDepth )
            {
                // use the passed in colour info to generate the DIBSECTION
                int iColours = pCurInfo->bmiHeader.biClrUsed;

                if ( !iColours )
                {
                    iColours = dib.bi.biClrUsed;
                }

                // copy the data accross...
                CopyMemory( dib.ct, pCurInfo->bmiColors, sizeof( RGBQUAD) * iColours );
            }
            else
            {
                // need to get the right palette....
                hpalOld = SelectPalette( hdcBmp, hpal, TRUE );
                RealizePalette( hdcBmp );
            
                int n = GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)&dib.ct[0]);

                Assert( n >= (int) dib.bi.biClrUsed );

                // now convert the PALETTEENTRY to RGBQUAD
                for (int i = 0; i < (int)dib.bi.biClrUsed; i ++)
                {
                    dib.ct[i] = RGB(GetBValue(dib.ct[i]),GetGValue(dib.ct[i]),GetRValue(dib.ct[i]));
                }
            }
        }
 
        LPVOID lpBits;
        *phBmp = CreateDIBSection(hdcBmp, (LPBITMAPINFO)&dib, DIB_RGB_COLORS, &lpBits, NULL, 0);
        if ( *phBmp )
        {
            if ( ppBMI )
            {
                *ppBMI = (BITMAPINFO *) LocalAlloc( LPTR, sizeof( dib ));
                if ( *ppBMI )
                {
                    CopyMemory( *ppBMI, &dib, sizeof( dib ));
                }
            }
            if ( ppBits )
            {
                *ppBits = lpBits;
            }
        }
        DeleteDC( hdcBmp );
        ReleaseDC( NULL, hdc );
    }
    return (*phBmp != NULL );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LPVOID CalcBitsOffsetInDIB( LPBITMAPINFO pBMI )
{
    int ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;
        
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    return (LPVOID) ((UCHAR *)&pBMI->bmiColors[0] + ncolors * sizeof(RGBQUAD));
}

