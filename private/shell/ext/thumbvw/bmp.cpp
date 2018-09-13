#include "precomp.h"
#include <IImgCtx.h>

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

CHAR const c_szGraphicsFilters[] = "Software\\Microsoft\\Shared Tools\\Graphics Filters\\Import";
CHAR const c_szExts[] = "Extensions";
CHAR const c_szShowProgressDialog[] = "ShowProgressDialog";
CHAR const c_szShowOptionsDialog[] = "ShowOptionsDialog";
CHAR const c_szPath[] = "Path";
CHAR const c_szYes[] = "Yes";
CHAR const c_szNo[] = "No";

// Register the classes.
UINT FormatMessageBox( HWND hwnd, UINT idMsg, UINT idTitle, 
                       LPVOID * pArgsMsg, LPVOID * pArgsTitle,
                       UINT idIconResource, UINT uFlags );

BOOL CheckStringForExtension( LPSTR pszString, LPCSTR pszExt );

#define MAX( a, b ) ( a > b ? a : b )


//////////////////////////////////////////////////////////////////////////////////////////////////

typedef UINT ( FAR PASCAL *LPFILTERINFO )( short v, LPSTR szFilterExten,
                                           HANDLE FAR * fph1, HANDLE FAR * fph2 );
typedef UINT ( FAR PASCAL *LPIMPORTFUNC )( HDC hdc, FILESPEC FAR *lpfs,
                                           GRPI FAR *p, HANDLE hPref );
typedef int ( FAR PASCAL *LPSETFILTERPREF )( HANDLE hPrefMem, LPSTR szOption, LPVOID pvValue,
                                             ULONG dwSize, ULONG dwType );
//////////////////////////////////////////////////////////////////////////////////////////////////

BOOL HasGraphicsFilter( LPCWSTR pszExt, LPSTR szHandler, DWORD * pcbSize )
{
    CHAR szExt[MAX_PATH];
    BOOL fRet = FALSE;
    HKEY hkFilters = NULL;
    int iCtr = 0;
    
    // make sure we have a file extension.
    if ( pszExt == NULL ) return FALSE;

    // remove the leading dot from file extension, because graphics filters are not
    // registered with the dot for extensions.
    pszExt = CharNextWrapW( pszExt );
    
    SHUnicodeToAnsi( pszExt, szExt, ARRAYSIZE( szExt ));
    
    // open the key where graphics filters are registered.
    LRESULT lRes = RegOpenKeyExA( HKEY_LOCAL_MACHINE, c_szGraphicsFilters, NULL, KEY_READ,
                                 &hkFilters );
    if ( lRes == ERROR_SUCCESS )
    {
        CHAR   szBuffer[MAX_PATH];
        
        // enumerate through all of the registered filters.
        do
        {
            DWORD dwSize = MAX_PATH;
            DWORD dwType = REG_SZ;
            lRes = RegEnumKeyExA( hkFilters, iCtr ++, szBuffer, &dwSize, NULL, NULL, NULL, 
                              NULL );
            if ( lRes != ERROR_SUCCESS )
            {
                break;
            }
            
            HKEY    hkType = NULL;
            lRes = RegOpenKeyExA( hkFilters, szBuffer, NULL, KEY_READ, &hkType );
            if ( lRes == ERROR_SUCCESS )
            {
                dwSize = MAX_PATH;

                // check their file extension agains the one passed in.
                lRes = RegQueryValueExA( hkType, c_szExts, NULL, &dwType, ( LPBYTE )szBuffer, 
                                       &dwSize );
                if ( lRes == ERROR_SUCCESS )
                {
                    fRet = CheckStringForExtension( szBuffer, szExt );
                    if ( fRet && szHandler )
                    {
                        RegQueryValueExA( hkType, c_szPath, NULL, NULL, ( LPBYTE )szHandler, pcbSize );
                    }
                }
                RegCloseKey( hkType );
            }
        }
        while ( !fRet );
    }
    RegCloseKey( hkFilters );

    return fRet;
}

BOOL CheckStringForExtension( LPSTR pszString, LPCSTR pszExt )
{
    LPSTR pszTmp = pszString;
    do
    {
        LPSTR pszTok = StrStrIA( pszTmp, pszExt );

        if ( pszTok )
        {
            // check that we don't have a partial
            int iEndPos = lstrlenA( pszExt );

            if ( pszTmp[iEndPos] == CHAR('\0') || pszTmp[iEndPos] == CHAR('\t')
                || pszTmp[iEndPos] == CHAR(','))
            {
                // we found the token ...
                return TRUE;
            }

            // skip this token. 
            pszTok = pszTok + iEndPos;
        }

        pszTmp = pszTok;
    }
    while ( pszTmp );
    return FALSE;
}
///////////////////////////////////////////////////////////////////////////////////////////
COfficeThumb::COfficeThumb( )
{
    m_szPath[0] = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
COfficeThumb::~COfficeThumb()
{
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::GetLocation ( LPWSTR pszPathBuffer,
                                         DWORD cch,
                                         DWORD * pdwPriority,
                                         const SIZE * prgSize,
                                         DWORD dwRecClrDepth,
                                         DWORD *pdwFlags )
{
    if ( !pdwFlags || !pszPathBuffer || !prgSize )
    {
        return E_INVALIDARG;
    }

    m_rgSize = * prgSize;
    m_dwRecClrDepth = dwRecClrDepth;
    
    HRESULT hr = NOERROR;
    if ( *pdwFlags & IEIFLAG_ASYNC )
    {
        if ( !pdwPriority )
        {
            return E_INVALIDARG;
        }
        
        hr = E_PENDING;
        // lower than normal priority
        *pdwPriority = 0x01000000;
    }

    m_fOrigSize = BOOLIFY( *pdwFlags & IEIFLAG_ORIGSIZE );
    
    *pdwFlags = IEIFLAG_CACHE;

    StrCpyNW( pszPathBuffer, m_szPath, cch );
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::Extract ( HBITMAP * phBmpThumbnail)
{
    HMODULE hModule = NULL;
    FILESPEC fileSpec;               // file to load
    GRPI pict;
    UINT rc;                     // return code
    HANDLE hPrefMem = NULL;        // filter-supplied preferences
    UINT wFilterType;            // 2 = graphics filter
    CHAR szHandler[MAX_PATH];
    LPBITMAPINFOHEADER lpbi = NULL;
    LPBITMAPINFO lpbInfo = NULL;
    LPVOID lpBits = NULL;
    WORD nNumColors;
    HRESULT hr = E_FAIL;
    WORD nVersion = 3;
    DWORD cbSize = MAX_PATH;

    LPFILTERINFO    GetFilterInfo = NULL;
    LPIMPORTFUNC    ImportGR = NULL;
    LPSETFILTERPREF SetFilterPref = NULL;
    DWORD   dwErr = 0L;

    LPCWSTR pszExt = PathFindExtensionW( m_szPath );
    if ( !pszExt )
    {
        return E_INVALIDARG;
    }

    if ( !HasGraphicsFilter( pszExt, szHandler, & cbSize ))
    {
        /*[TODO: report error ]*/
        return E_UNEXPECTED;
    }

    Assert (szHandler[0] != 0);

    hModule = LoadLibraryA( szHandler );

    if ( hModule == NULL )
    {
        /*[TODO: report error ]*/
        return E_UNEXPECTED;
    }

    // get a pointer to the ImportGR function.
    ImportGR = ( LPIMPORTFUNC )GetProcAddress( hModule, "ImportGr" );
    if ( ImportGR == NULL ) 
    {
        // try the other name (they stupidly exported some of the functions with the
        // wrong capitalisation)
        ImportGR = ( LPIMPORTFUNC )GetProcAddress( hModule, "ImportGR" );
    }
    
    GetFilterInfo = ( LPFILTERINFO )GetProcAddress( hModule, "GetFilterInfo" );
    SetFilterPref = ( LPSETFILTERPREF )GetProcAddress( hModule, "SetFilterPref" );

    if ( ( GetFilterInfo == NULL ) || ( ImportGR == NULL ) )
    {
        GetFilterInfo = ( LPFILTERINFO )GetProcAddress( hModule, "GetFilterInfo@16" );
        ImportGR = ( LPIMPORTFUNC )GetProcAddress( hModule, "ImportGr@16" );
        SetFilterPref = NULL;
        nVersion = 2;
    }

    if ( ImportGR == NULL || GetFilterInfo == NULL )
    {
        /*[TODO: report error  ]*/
        FreeLibrary( hModule );
        return E_UNEXPECTED;
    }

    wFilterType = ( *GetFilterInfo )(( short ) 2,                   // interface version no.
                                     LPSTR(""),                     // end of .INI entry
                                     ( HANDLE FAR * ) &hPrefMem,    // fill in: preferences
                                     ( HANDLE FAR * ) NULL );       // unused in Windows

    /* the return value is the type of filter: 0=error,
     * 1=text-filter, 2=graphics-filter
     */
    if ( wFilterType == 2 )
    {
        fileSpec.slippery = FALSE;      // TRUE if file may disappear
        fileSpec.write = FALSE;         // TRUE if open for write
        fileSpec.unnamed = FALSE;       // TRUE if unnamed
        fileSpec.linked = FALSE;        // Linked to an FS FCB
        fileSpec.mark = FALSE;          // Generic mark bit
        fileSpec.handle = 0;            // MS-DOS open file handle
        fileSpec.filePos = 0L;

        SHUnicodeToAnsi( m_szPath, fileSpec.fullName, ARRAYSIZE( fileSpec.fullName ));

        pict.h = NULL;
        pict.inch = 0;
    
        // if something throws an exception in the filter grab it and show an error.
        __try
        { 
            // turn off the progress bar if possible (version 3 feature).
            if ( SetFilterPref )
            {
                // LOC: the use of strlen here is fine because both the strings are
                // LOC: compiled into the code page and are ANSI
                SetFilterPref( hPrefMem, ( LPSTR )c_szShowProgressDialog, ( LPSTR )c_szNo, 
                               strlen( c_szNo ), REG_SZ );
                SetFilterPref( hPrefMem, ( LPSTR )c_szShowOptionsDialog, ( LPSTR )c_szNo, 
                               strlen( c_szNo ), REG_SZ );
            }

            rc = ( *ImportGR ) ( NULL,                          // "the target DC" (printer?)
                                ( FILESPEC FAR * ) &fileSpec,   // file to read
                                ( GRPI FAR * ) &pict,           // fill in: result metafile
                                ( HANDLE ) hPrefMem );          // preferences memory

            // turn progress bar back on when we're done.
            if ( SetFilterPref )
            {
                SetFilterPref( hPrefMem, ( LPSTR )c_szShowProgressDialog, ( LPSTR )c_szYes, 
                               lstrlenA( c_szYes ) + 1, REG_SZ );
            }
            
            if ( rc != 0 || pict.h == NULL )
            {
                /*[TODO: error report ]*/
                FreeLibrary( hModule );
                goto ErrorCleanup;
            }
        }
        __except( EXCEPTION_EXECUTE_HANDLER )
        {
            /*[TODO: report error ]*/
            goto ErrorCleanup;
        }

        if ( nVersion < 3 )
        {
            // find the BITMAPINFO in the returned metafile
            // this saves us from creating a metafile and duplicating
            // all the memory.
            LPMETAHEADER lphMeta = NULL;
            lphMeta = ( LPMETAHEADER )GlobalLock( pict.h );
            lpbi = MetaHeaderToBitmapInfo( lphMeta );
            if ( !lpbi )
            {
                GlobalUnlock( pict.h );
            }
        }
        else
        {
            // this is a HMETAFILE (not a pointer to the metafile struct),
            // so it must be handled differently.
            lpbi = HMetafileToBitmapInfo( pict );
            DeleteMetaFile(( HMETAFILE ) pict.h );
            pict.h = NULL;
        }
        
        if ( lpbi != NULL )
        {
            if ( !( nNumColors = ( WORD ) lpbi->biClrUsed ))
            {
                // no color table for 24-bit, default size otherwise 
                if ( lpbi->biBitCount != 24 )
                {
                    nNumColors = 1 << lpbi->biBitCount;     // standard size table
                }
            }

            //  fill in some default values if they are zero.
            if ( lpbi->biClrUsed == 0 )
            {
                lpbi->biClrUsed = ( DWORD )nNumColors;
            }

            // prepare pointers for conversion call.
            lpbInfo = ( LPBITMAPINFO )lpbi;

            HPALETTE hpal = NULL;
            if ( m_dwRecClrDepth == 8 )
            {
                hpal = SHCreateShellPalette( NULL );
            }
            else if ( m_dwRecClrDepth < 8 )
            {
                hpal = (HPALETTE) GetStockObject( DEFAULT_PALETTE );
            }
            
           // converts bitmap into a 120x120 thumbnail and returns it in the pointer.
            if ( ConvertDIBSECTIONToThumbnail( lpbInfo, CalcBitsOffsetInDIB( lpbInfo ),
                                               phBmpThumbnail, &m_rgSize, m_dwRecClrDepth, hpal, 15, m_fOrigSize ) == FALSE )
            {
                /*[TODO: report error ]*/
            }
            else
            {
                hr = S_OK;
            }

            if ( hpal )
            {
                DeletePalette( hpal );
            }
        }
    }

ErrorCleanup:

    
    if ( pict.h )
    {
        if ( lpbi )
        {
            GlobalUnlock( pict.h );
        }
        
        // free metafile picture.
        GlobalFree( pict.h );
    }
    else if ( lpbi )
    {
        // otherwise, if pict.h was a handle, then must release the 
        // dib that was created from it.
        LocalFree( lpbi );
    }

    if ( hPrefMem != NULL )
    {
        GlobalFree( hPrefMem );
    }

    if ( hModule )
    {
        FreeLibrary( hModule );
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
LPBITMAPINFOHEADER COfficeThumb::HMetafileToBitmapInfo( GRPI pict )
{
    HDC hDC = GetDC( GetDesktopWindow( ) );
    HDC hMemDC = CreateCompatibleDC( hDC );
    int iWidth = int( ( ( pict.bbox.right - pict.bbox.left ) * 72L ) / pict.inch );
    int iHeight = int( ( ( pict.bbox.bottom - pict.bbox.top ) * 72L ) / pict.inch );
    LPVOID pBits = NULL;
    LPBITMAPINFO pbi = NULL;
    BITMAPINFO bi;
    
    if ( ( iWidth > 640 ) || ( iHeight > 640 ) )
    {
        // don't create too big a bitmap; nothing greater than 640 resolution.
        int iTmp = MAX( iWidth, iHeight );
        iWidth = ( iWidth * 640 )/ iTmp;
        iHeight = ( iHeight * 640 ) / iTmp;
    }
    
    ZeroMemory( &bi, sizeof( BITMAPINFO ) );
    bi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bi.bmiHeader.biWidth = iWidth;
    bi.bmiHeader.biHeight = iHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    
    DWORD dwBPL = ( ( ( bi.bmiHeader.biWidth * 24 ) + 31 ) >> 3 ) & ~3;
    bi.bmiHeader.biSizeImage = dwBPL * bi.bmiHeader.biHeight;
    
    HBITMAP hBmp = CreateDIBSection( hDC, &bi, DIB_RGB_COLORS, &pBits, NULL, NULL );
    if ( hBmp == NULL || pBits == NULL )
    {
        return NULL;
    }
    HGDIOBJ hOldBmp = SelectObject( hMemDC, hBmp );
    
    // set draw parameters.
    SetMapMode( hMemDC, MM_ANISOTROPIC );
    SetWindowOrgEx( hMemDC, pict.bbox.left, pict.bbox.top, NULL );
    SetWindowExtEx( hMemDC, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, NULL );
    SetViewportOrgEx( hMemDC, 0, 0, NULL );
    SetViewportExtEx( hMemDC, bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, NULL );

    // play the metafile.
    BOOL bRet = PlayMetaFile( hMemDC, ( HMETAFILE )pict.h );
    if ( bRet )
    {
        GdiFlush( );
        hBmp = (HBITMAP) SelectObject( hMemDC, hOldBmp );
        pbi = ( LPBITMAPINFO )LocalAlloc( LPTR, bi.bmiHeader.biSize + bi.bmiHeader.biSizeImage );
        if ( pbi )
        {
            CopyMemory( pbi, &bi, bi.bmiHeader.biSize );
            LPVOID  pTmp = ( LPBYTE )pbi + bi.bmiHeader.biSize;
            CopyMemory( pTmp, pBits, bi.bmiHeader.biSizeImage );
        }
    }

    DeleteObject( hBmp );
    DeleteDC( hMemDC );
    ReleaseDC( GetDesktopWindow( ), hDC );
    return ( LPBITMAPINFOHEADER )pbi;
}

////////////////////////////////////////////////////////////////////////////////////////
// find the DIB bitmap in a memory meta file...
LPBITMAPINFOHEADER COfficeThumb::MetaHeaderToBitmapInfo( LPMETAHEADER pmh )
{
    LPMETARECORD pmr;

    for ( pmr = ( LPMETARECORD )( ( LPBYTE )pmh + pmh->mtHeaderSize * 2 );
          pmr < ( LPMETARECORD )( ( LPBYTE )pmh + pmh->mtSize * 2 );
          pmr = ( LPMETARECORD )( ( LPBYTE )pmr + pmr->rdSize * 2 ) )
    {
        switch ( pmr->rdFunction )
        {
            case META_DIBBITBLT:
                return ( LPBITMAPINFOHEADER )&( pmr->rdParm[8] );

            case META_DIBSTRETCHBLT:
                return ( LPBITMAPINFOHEADER )&( pmr->rdParm[10] );

            case META_STRETCHDIB:
                return ( LPBITMAPINFOHEADER )&( pmr->rdParm[11] );

            case META_SETDIBTODEV:
                return ( LPBITMAPINFOHEADER )&( pmr->rdParm[9] );
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::IsDirty()
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::Load( LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT hr = E_FAIL;
    
    if ( !pszFileName )
    {
        return E_INVALIDARG;
    }

    StrCpyNW( m_szPath, pszFileName, ARRAYSIZE( m_szPath ));
    
    // check if we have a filter ....
    LPCWSTR pszExt = PathFindExtensionW( m_szPath );
    if ( pszExt != NULL )
    {
        if ( HasGraphicsFilter( pszExt, NULL, NULL ))
        {
            hr = NOERROR;
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::Save( LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::SaveCompleted( LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP COfficeThumb::GetCurFile( LPOLESTR *ppszFileName)
{
    return E_NOTIMPL;
}


