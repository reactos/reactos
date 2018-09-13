#include "precomp.h"

HRESULT GetMediaManagerThumbnail( IPropertyStorage * pPropStg,
                                  const SIZE * prgSize,
                                  DWORD dwClrDepth,
                                  HPALETTE hpal,
                                  BOOL fOrigSize,
                                  HBITMAP * phBmpThumbnail );
                                  
HRESULT GetDocFileThumbnail( IPropertyStorage * pPropStg,
                             const SIZE * prgSize,
                             DWORD dwClrDepth,
                             HPALETTE hpal,
                             BOOL fOrigSize,
                             HBITMAP * phBmpThumbnail );

// PACKEDMETA struct for DocFile thumbnails.
typedef struct
{
    WORD    mm;
    WORD    xExt;
    WORD    yExt;
    WORD    dummy;
} PACKEDMETA;

VOID CalcMetaFileSize( HDC hDC, PACKEDMETA * pMeta, const SIZEL * prgSize, RECT * pRect);

WCHAR const c_szMIC[] = L".MIC";
WCHAR c_szThumbnailProp[] = L"Thumbnail";

///////////////////////////////////////////////////////////////////////////////////////////////////
CDocFileHandler::CDocFileHandler()
{
    m_pszPath = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CDocFileHandler::~CDocFileHandler()
{
    if ( m_pszPath )
    {
        LocalFree( m_pszPath );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::GetLocation ( LPWSTR szFileName,
                                            DWORD cchMax,
                                            DWORD * pdwPriority,
                                            const SIZE * prgSize,
                                            DWORD dwRecClrDepth,
                                            DWORD *pdwFlags )
{
    if ( !szFileName || !pdwFlags || ((*pdwFlags & IEIFLAG_ASYNC) && !pdwPriority ) || !prgSize )
    {
        return E_INVALIDARG;
    }

    if ( !m_pszPath )
    {
        return E_UNEXPECTED;
    }

    m_rgSize = *prgSize;
    m_dwRecClrDepth = dwRecClrDepth;
    
    // just copy the current path into the buffer as we do not share thumbnails...
    StrCpyNW( szFileName, m_pszPath, cchMax );

    HRESULT hr = NOERROR;
    if ( *pdwFlags & IEIFLAG_ASYNC )
    {
        // we support async 
        hr = E_PENDING;
        *pdwPriority = PRIORITY_NORMAL;
    }

    m_fOrigSize = BOOLIFY(*pdwFlags & IEIFLAG_ORIGSIZE );
    
    // we don't want it cached....
    *pdwFlags &= ~IEIFLAG_CACHE;

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::Extract ( HBITMAP * phBmpThumbnail)
{
    if ( !m_pszPath )
    {
        return E_UNEXPECTED;
    }
    
    if ( !phBmpThumbnail )
    {
        return E_INVALIDARG;
    }

    // find the extension.....
    LPWSTR pszExt = StrRChrW( m_pszPath, NULL, WCHAR('.'));
    if ( !pszExt )
    {
        return E_UNEXPECTED;
    }
    
    LPSTORAGE pStorage = NULL;
    HRESULT hr = StgOpenStorage( m_pszPath,
                                 NULL,
                                 STGM_READ | STGM_SHARE_EXCLUSIVE,
                                 NULL,
                                 NULL,
                                 & pStorage );
    if ( FAILED( hr ))
    {
        return hr;
    }

    LPPROPERTYSETSTORAGE pPropSetStg = NULL;
    hr = pStorage->QueryInterface( IID_IPropertySetStorage, (LPVOID *) &pPropSetStg );
    if ( FAILED( hr ))
    {
        pStorage->Release();
        return hr;
    }
    
    FMTID fmtidPropSet = FMTID_SummaryInformation;
    BOOL fMediaManager = FALSE;

    // "MIC" Microsoft Image Composer files needs special casing because they use
    // the Media Manager internal thumbnail propertyset ... (by what it would be like
    // to be standard for once ....)
    if ( StrCmpIW( pszExt, c_szMIC ) == 0 )
    {
        // switch to the Media Manager thumbnail propertyset....
        fmtidPropSet = FMTID_CmsThumbnailPropertySet;
        fMediaManager = TRUE;
    }
    
    LPPROPERTYSTORAGE pPropSet;
    hr = pPropSetStg->Open( fmtidPropSet,
                            STGM_READ | STGM_SHARE_EXCLUSIVE,
                            &pPropSet );
    if ( FAILED( hr ))
    {
        pStorage->Release();
        pPropSetStg->Release();
        return hr;
    }

    HPALETTE hpal = NULL;
    if ( m_dwRecClrDepth == 8 )
    {
        hpal = SHCreateShellPalette( NULL );
    }
    else if ( m_dwRecClrDepth < 8 )
    {
        hpal = (HPALETTE) GetStockObject( DEFAULT_PALETTE );
    }
    
    if ( fMediaManager )
    {
        hr = GetMediaManagerThumbnail( pPropSet, &m_rgSize, m_dwRecClrDepth, hpal, m_fOrigSize, phBmpThumbnail );
    }
    else
    {
        hr = GetDocFileThumbnail( pPropSet, &m_rgSize, m_dwRecClrDepth, hpal, m_fOrigSize, phBmpThumbnail );
    }

    if ( hpal )
    {
        DeletePalette( hpal );
    }
    
    pPropSet->Release();
    pPropSetStg->Release();
    pStorage->Release();
    
    return hr;   
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::GetDateStamp ( FILETIME * pftDateStamp )
{
    Assert( pftDateStamp );
    
    HANDLE hFind;
    WIN32_FIND_DATAW rgData;
    
    hFind = FindFirstFileWrapW( m_pszPath, &rgData );
    if ( INVALID_HANDLE_VALUE != hFind )
    {
        *pftDateStamp = rgData.ftLastWriteTime;
        FindClose( hFind );
        return S_OK;
    }

    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    // ignore the DWORD dwMode, as we only ever open in read mode....
    if ( !pszFileName )
    {
        return E_INVALIDARG;
    }
    
    DWORD dwAttrs = GetFileAttributesWrapW( pszFileName );
    if (( dwAttrs != (DWORD) -1) && (dwAttrs & FILE_ATTRIBUTE_OFFLINE ))
    {
        return E_FAIL;
    }

    if ( m_pszPath )
    {
        LocalFree( m_pszPath );
    }

    UINT cLen = lstrlenW( pszFileName ) + 1;
    m_pszPath = (LPWSTR) LocalAlloc( LPTR, cLen * sizeof(WCHAR));
    if ( !m_pszPath )
    {
        return E_OUTOFMEMORY;
    }

    StrCpyW( m_pszPath, pszFileName );
    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetMediaManagerThumbnail( IPropertyStorage * pPropStg,
                                  const SIZE * prgSize,
                                  DWORD dwClrDepth,
                                  HPALETTE hpal,
                                  BOOL fOrigSize,
                                  HBITMAP * phBmpThumbnail )
{
    // current version of media manager simply stores the DIB data in a under a 
    // named property Thumbnail...
    PROPSPEC propSpec;
    PROPVARIANT pvarResult;

    ZeroMemory( &pvarResult, sizeof( pvarResult ));
    
    if ( !pPropStg || !prgSize )
    {
        return E_INVALIDARG;
    }
    
    // read the thumbnail property from the property storage.
    propSpec.ulKind = PRSPEC_LPWSTR;
    propSpec.lpwstr = c_szThumbnailProp;
    
    HRESULT hr = pPropStg->ReadMultiple( 1, &propSpec, &pvarResult );
    if( SUCCEEDED( hr ) )
    {
        BITMAPINFO * pbi;
        LPVOID pBits;

        pbi = ( BITMAPINFO * )pvarResult.blob.pBlobData;

        pBits = CalcBitsOffsetInDIB( pbi );
        
        hr = E_FAIL;
        if ( pbi->bmiHeader.biSize == sizeof( BITMAPINFOHEADER ) )
        {
            if ( ConvertDIBSECTIONToThumbnail( pbi, pBits, phBmpThumbnail, prgSize, dwClrDepth, hpal, 15, fOrigSize ))
            {
                hr = NOERROR;
            }
        }

        PropVariantClear( &pvarResult );
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
HRESULT GetDocFileThumbnail( IPropertyStorage * pPropStg,
                             const SIZE * prgSize,
                             DWORD dwClrDepth,
                             HPALETTE hpal,
                             BOOL fOrigSize,
                             HBITMAP * phBmpThumbnail )
{
    PROPSPEC propSpec;
    PROPVARIANT pvarResult;
    HDC hDC = GetDC( GetDesktopWindow( ) );
    HBITMAP hBmp = NULL;
    HRESULT hr = S_OK;

    if ( !pPropStg || !prgSize )
    {
        return E_INVALIDARG;
    }
    
    HDC hMemDC = CreateCompatibleDC( hDC );
    if ( hMemDC == NULL )
    {
        return E_OUTOFMEMORY;
    }

    // read the thumbnail property from the property storage.
    propSpec.ulKind = PRSPEC_PROPID;
    propSpec.propid = PIDSI_THUMBNAIL;
    hr = pPropStg->ReadMultiple( 1, &propSpec, &pvarResult );
    if( SUCCEEDED( hr ) )
    {
        // make sure we are dealing with a clipboard format. CLIPDATA
        if ( ( pvarResult.vt == VT_CF ) && ( pvarResult.pclipdata->ulClipFmt == -1 ) )
        {
            LPDWORD pdwCF = ( LPDWORD )pvarResult.pclipdata->pClipData;
            LPBYTE  pStruct = pvarResult.pclipdata->pClipData + sizeof( DWORD );

            if ( *pdwCF == CF_METAFILEPICT )
            {
                SetMapMode( hMemDC, MM_TEXT );
                
                // handle thumbnail that is a metafile.
                PACKEDMETA * pMeta = ( PACKEDMETA * )pStruct;
                LPBYTE pData = pStruct + sizeof( PACKEDMETA );
                HMETAFILE hMF;
                RECT rect;

                UINT cbSize = pvarResult.pclipdata->cbSize - sizeof( DWORD ) - sizeof( pMeta->mm ) - 
                sizeof( pMeta->xExt ) - sizeof( pMeta->yExt ) - sizeof( pMeta->dummy );

                // save as a metafile.
                hMF = SetMetaFileBitsEx( cbSize, pData );
                if ( hMF )
                {    
                    SIZE rgNewSize;
                    
                    // use the mapping mode to calc the current size
                    CalcMetaFileSize( hMemDC, pMeta, prgSize, & rect );
                    
                    CalculateAspectRatio( prgSize, &rect );

                    if ( fOrigSize )
                    {
                        // use the aspect rect to refigure the size...
                        rgNewSize.cx = rect.right - rect.left;
                        rgNewSize.cy = rect.bottom - rect.top;
                        prgSize = &rgNewSize;
                        
                        // adjust the rect to be the same as the size (which is the size of the metafile)
                        rect.right -= rect.left;
                        rect.bottom -= rect.top;
                        rect.left = 0;
                        rect.top = 0;
                    }

                    if ( CreateSizedDIBSECTION( prgSize, dwClrDepth, hpal, NULL, &hBmp, NULL, NULL ))
                    {
                        HGDIOBJ hOldBmp = SelectObject( hMemDC, hBmp );
                        HGDIOBJ hBrush = GetStockObject( THUMBNAIL_BACKGROUND_BRUSH );
                        HGDIOBJ hOldBrush = SelectObject( hMemDC, hBrush );
                        HGDIOBJ hPen = GetStockObject( THUMBNAIL_BACKGROUND_PEN );
                        HGDIOBJ hOldPen = SelectObject( hMemDC, hPen );

                        Rectangle( hMemDC, 0, 0, prgSize->cx, prgSize->cy );
        
                        SelectObject( hMemDC, hOldBrush );
                        SelectObject( hMemDC, hOldPen );
                    
                        int iXBorder = 0;
                        int iYBorder = 0;
                        if ( rect.left == 0 )
                        {
                            iXBorder ++;
                        }
                        if ( rect.top == 0 )
                        {
                            iYBorder ++;
                        }
                        
                        SetViewportExtEx( hMemDC, rect.right - rect.left - 2 * iXBorder, rect.bottom - rect.top - 2 * iYBorder, NULL );
                        SetViewportOrgEx( hMemDC, rect.left + iXBorder, rect.top + iYBorder, NULL );

                        SetMapMode( hMemDC, pMeta->mm );

                        // play the metafile.
                        BOOL bRet = PlayMetaFile( hMemDC, hMF );
                        if ( bRet )
                        {
                            *phBmpThumbnail = hBmp ;
                            if ( *phBmpThumbnail == NULL )
                            {
                                // unable to get thumbnail bitmap.
                                hr = E_FAIL;
                            }
                        }

                        DeleteMetaFile( hMF );
                        SelectObject( hMemDC, hOldBmp );

                        if ( FAILED( hr ) && hBmp )
                        {
                            DeleteObject( hBmp );
                        }
                    }
                    else
                    {
                        hr = DV_E_CLIPFORMAT;
                    }
                }
            }

            else if ( *pdwCF == CF_DIB )
            {
                // handle thumbnail that is a metafile.
                BITMAPINFO * pDib = (BITMAPINFO *) pStruct;
                if ( pDib->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
                {
                    LPVOID pBits = CalcBitsOffsetInDIB( pDib );
                    
                    if ( ConvertDIBSECTIONToThumbnail( pDib, pBits, phBmpThumbnail, prgSize, dwClrDepth, hpal, 15, fOrigSize ) == FALSE )
                    {
                        hr = DV_E_CLIPFORMAT;
                    }
                }
            }
            else
            {
                hr = DV_E_CLIPFORMAT;
            }
        }
        else
        {
            hr = DV_E_CLIPFORMAT;
        }
        PropVariantClear( &pvarResult );
    }

    DeleteDC( hMemDC );
    ReleaseDC( GetDesktopWindow( ), hDC );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
VOID CalcMetaFileSize( HDC hDC, PACKEDMETA * prgMeta, const SIZEL * prgSize, RECT * prgRect)
{
    Assert( prgMeta && prgRect );

    prgRect->left = 0;
    prgRect->top = 0;

    if ( !prgMeta->xExt || !prgMeta->yExt )
    {
        // no size, then just use the size rect ...
        prgRect->right = prgSize->cx;
        prgRect->bottom = prgSize->cy;
    }
    else
    {
        // set the mapping mode....
        SetMapMode( hDC, prgMeta->mm );

        if ( prgMeta->mm == MM_ISOTROPIC || prgMeta->mm == MM_ANISOTROPIC )
        {
            // we must set the ViewPortExtent and the window extent to get the scaling
            SetWindowExtEx( hDC, prgMeta->xExt, prgMeta->yExt, NULL );
            SetViewportExtEx( hDC, prgMeta->xExt, prgMeta->yExt, NULL );
        }

        POINT pt;
        pt.x = prgMeta->xExt;
        pt.y = prgMeta->yExt;

        // convert to pixels....
        LPtoDP( hDC, &pt, 1);

        prgRect->right = abs( pt.x );
        prgRect->bottom = abs( pt.y );
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::GetClassID(CLSID * pCLSID )
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::IsDirty( )
{
    return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDocFileHandler::GetCurFile(LPOLESTR * ppszFileName)
{
    return E_NOTIMPL;
}

