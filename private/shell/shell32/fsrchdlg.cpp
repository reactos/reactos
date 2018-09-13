// fsrchdlg.cpp : Implementation of CFindFilesDlg
#include "shellprv.h"
#include "fsearch.h"
#include "fsrchdlg.h"
#include "asuggest.h"   // IAutoCompleteDropDown
#include "docfind.h"    // DFW_xxx warning flags.

//  shellprv.h brings in a bogus SubclassWindow macro def
#ifdef SubclassWindow
#undef SubclassWindow
#endif SubclassWindow

//  HACK bust: this should be decl'd in a header file!
extern "C" HIMAGELIST WINAPI _GetSystemImageListSmallIcons();

#define MAX_EDIT                256
#define SUBDLG_BORDERWIDTH      0
#define FOLDER_IMAGELIST_INDEX  3
#define MIN_NAMESPACELIST_WIDTH 140
#define MIN_FILEASSOCLIST_WIDTH 175
#define CSIDL_DEFAULT_PIDL      CSIDL_DRIVES
#define FILE_ATTRIBUTE_NOTFOUND 0xFFFFFFFF

#define FINDHELPFILE            TEXT("find.chm")
#define FINDFILES_HELPTOPIC     NULL //TEXT("win_tray_find_file.htm")
#define FINDCOMPUTER_HELPTOPIC  NULL //TEXT("find_computer.htm")

#define INDEXSERVICE_HELPFILE   TEXT("isconcepts.chm")
#define INDEXSERVICE_HELPTOPIC  NULL

#define LSUIS_WARNING  1  // LoadSaveUIState warning flags
#define AUTOSUGGEST_MRUCOUNT    25

//  Activate load, save of subdialog state by undefing this:
// #define _LOADSAVE_SUBDLG_UI__


//-------------------------------------------------------------------------//
//  Forwards:
//-------------------------------------------------------------------------//
int  CSearchWarningDlg_DoModal( HWND hwndParent, USHORT uDlgTemplate, BOOL* pbNoWarn );
int  CCISettingsDlg_DoModal( HWND hwndParent );
HWND CCISettingsDlg_CreateModeless( HWND hwndParent );


//-------------------------------------------------------------------------//
//  Helpers
//-------------------------------------------------------------------------//

//  constraint names (must be kept in sync w/ enumeration FSB_CONSTRAINT).
const LPCWSTR constraint_names[] = 
{
    L"SearchFor",
    L"IndexedSearch",
    L"Named",
    L"LookIn",
    L"ContainingText",
    L"SizeLE",
    L"SizeGE",
    L"WhichDate",
    L"DateNMonths",
    L"DateNDays",
    L"DateLE",
    L"DateGE",
    L"FileType",
    L"IncludeSubFolders",
    L"CaseSensitive",
    L"RegularExpressions",
    L"SearchSlowFiles",
    L"QueryDialect",
    L"WarningFlags",
};

LPCWSTR GetConstraintName( FSB_CONSTRAINT constraint )
{
    if( constraint >= 0 && constraint < _fsbc_count )
        return constraint_names[constraint];
    return NULL;     
}

//-------------------------------------------------------------------------//
BOOL IsConstraintName( FSB_CONSTRAINT constraint, LPCWSTR pwszName )
{
    LPCWSTR pwszConstraint = GetConstraintName( constraint );
    if( pwszConstraint && pwszName )
        return 0 == StrCmpIW( pwszName, pwszConstraint );
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _GetWindowSize( HWND hwndDlg, LPSIZE pSize )
{
    RECT rc;
    if( ::GetClientRect( hwndDlg, &rc ) )
    {
        pSize->cx = RECTWIDTH( &rc );
        pSize->cy = RECTHEIGHT( &rc );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _ModifyWindowStyle( HWND hwnd, DWORD dwAdd, DWORD dwRemove )
{
    ASSERT( dwAdd || dwRemove );

    if( IsWindow( hwnd ) )
    {
        DWORD dwStyle = GetWindowLong( hwnd, GWL_STYLE );
        if( dwAdd )
            dwStyle |= dwAdd;
        if( dwRemove )
            dwStyle &= ~dwRemove;
        SetWindowLong( hwnd, GWL_STYLE, dwStyle );
        return TRUE;
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
BOOL _EnforceNumericEditRange( 
    IN HWND hwndDlg, 
    IN UINT nIDEdit, 
    IN UINT nIDSpin,
    IN LONG nLow, 
    IN LONG nHigh, 
    IN BOOL bSigned = FALSE )
{
    BOOL bRet   = FALSE;
    BOOL bReset = FALSE;
    LONG lRet   = -1;

    if( nIDSpin )
    {
        lRet = (LONG)SendDlgItemMessage( hwndDlg, nIDSpin, UDM_GETPOS, 0, 0L );
        bRet = 0 == HIWORD(lRet);
    }
    
    if( !bRet )
        lRet = (LONG)GetDlgItemInt( hwndDlg, nIDEdit, &bRet, bSigned );

    if( lRet < nLow )
    {
        lRet = nLow;
        bReset = TRUE;
    }
    else if( lRet > nHigh )
    {
        lRet = nHigh;
        bReset = TRUE;
    }

    if( bReset )
        SetDlgItemInt( hwndDlg, nIDEdit, lRet, bSigned );

    return bRet;
}

//-------------------------------------------------------------------------//
BOOL _IsDirectoryServiceAvailable()
{
    IShellDispatch2* psd;
    BOOL             bRet = FALSE;

    if( SUCCEEDED( CoCreateInstance( CLSID_Shell, 0, CLSCTX_INPROC_SERVER, 
                                     IID_IShellDispatch2, (void**) &psd )) )
    {
        BSTR bstrName = SysAllocString( L"DirectoryServiceAvailable");
        if( bstrName )
        {
            VARIANT varRet = {0};
            if( SUCCEEDED( psd->GetSystemInformation( bstrName, &varRet ) ) )
            {
                ASSERT( VT_BOOL == varRet.vt );
                bRet = varRet.boolVal;
            }
            SysFreeString( bstrName );
        }
        psd->Release();
    }
    return bRet ;
}

//-------------------------------------------------------------------------//
//  Calculates number of pixels for dialog template units
LONG _PixelsForDbu( HWND hwndDlg, LONG cDbu, BOOL bHorz )
{
    RECT rc = {0,0,0,0};
    if( bHorz )
        rc.right = cDbu;
    else
        rc.bottom = cDbu;

    if( MapDialogRect( hwndDlg, &rc ) )
        return bHorz ? RECTWIDTH(&rc) : RECTHEIGHT(&rc);
    return 0;
}

//-------------------------------------------------------------------------//
//  Retrieves a localizable horizontal or vertical metric value from
//  the resource module.
LONG _GetResourceMetric( HWND hwndDlg, UINT nIDResource, BOOL bHorz /*orientation of metric*/ )
{
    TCHAR szMetric[48];
    if( !EVAL( LoadString( HINST_THISDLL, nIDResource,
                           szMetric, ARRAYSIZE(szMetric) ) ) )
        return 0;

    LONG n = StrToInt(szMetric);
    return _PixelsForDbu( hwndDlg, n, bHorz );
}

//-------------------------------------------------------------------------//
//  Calculates the date nDays + nMonths from pstSrc.   nDays and/or nMonths
//  can be negative values.
BOOL _CalcDateOffset( const SYSTEMTIME* pstSrc, int nDays, int nMonths, OUT SYSTEMTIME* pstDest )
{
    ASSERT( pstSrc );
    ASSERT( pstDest );
    
    //  Subtract 90 days from current date and stuff in date low range
    FILETIME   ft;
    SystemTimeToFileTime( pstSrc, &ft );

    LARGE_INTEGER t;
    t.LowPart = ft.dwLowDateTime;
    t.HighPart = ft.dwHighDateTime;

    nDays += MulDiv( nMonths, 1461 /*days per 4 yrs*/, 48 /*months per 4 yrs*/ );   
    t.QuadPart += Int32x32To64( nDays * 24 /*hrs per day*/ * 3600 /*seconds per hr*/,
                                10000000 /*100 ns intervals per sec*/);
    ft.dwLowDateTime = t.LowPart;
    ft.dwHighDateTime = t.HighPart;
    FileTimeToSystemTime( &ft, pstDest );
    return TRUE;
}

//-------------------------------------------------------------------------//
//  Loads a string into a combo box and assigns the string resource ID value
//  to the combo item's data.
BOOL _LoadStringToCombo( HWND hwndCombo, int iPos, UINT idString, LPARAM lpData )
{
    TCHAR szText[MAX_EDIT];
    if( LoadString( HINST_THISDLL, idString, szText, ARRAYSIZE(szText) ) )
    {
        INT_PTR idx = ::SendMessage( hwndCombo, CB_INSERTSTRING, iPos, (LPARAM)szText );
        if( idx != CB_ERR )
        {
            ::SendMessage( hwndCombo, CB_SETITEMDATA, idx, lpData );
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves combo item's data.  If idx == CB_ERR, the currently selected
//  item's data will be retrieved.
LPARAM _GetComboData( HWND hwndCombo, INT_PTR idx = CB_ERR )
{
    if( CB_ERR == idx )
        idx = SendMessage( hwndCombo, CB_GETCURSEL, 0, 0L );
    if( CB_ERR == idx )
        return idx;

    return (LPARAM)::SendMessage( hwndCombo, CB_GETITEMDATA, idx, 0L );
}

//-------------------------------------------------------------------------//
//  Selects combo item with matching data, and returns the index
//  of the selected item.
INT_PTR _SelectComboData( HWND hwndCombo, LPARAM lpData )
{
    INT_PTR i;
    INT_PTR cnt = SendMessage( hwndCombo, CB_GETCOUNT, 0, 0L );

    for( i=0; i< cnt; i++ )
    {
        LPARAM lParam = SendMessage( hwndCombo, CB_GETITEMDATA, i, 0L );
        if( lParam != CB_ERR && lParam == lpData )
        {
            SendMessage( hwndCombo, CB_SETCURSEL, i, 0L );
            return i;
        }
    }
    return CB_ERR;
}

//-------------------------------------------------------------------------//
//  Draws a focus rect bounding the specified window.
void _DrawCtlFocus( HWND hwnd )
{
    HDC  hdc;
    if( (hdc = GetDC( hwnd )) != NULL )
    {
        RECT rc;
        GetClientRect( hwnd, &rc );
        DrawFocusRect( hdc, &rc );
        ReleaseDC( hwnd, hdc );
    }
}

//-------------------------------------------------------------------------//
BOOL _IsPathList( LPCTSTR pszPath )
{
    return pszPath ? StrChr( pszPath, TEXT(';') ) != NULL : FALSE;
}

//-------------------------------------------------------------------------//
HRESULT _IsPathValidUNC( HWND hWndParent, BOOL fNetValidate, LPCTSTR pszPath )
{
    HRESULT hr = S_OK;
    DWORD dwResult;
    NETRESOURCE nr;
    TCHAR pszPathBuffer[MAX_PATH];

    // Safe IsPathUNC
    if( ( TEXT( '\\' ) !=  pszPath[0] ) || ( TEXT( '\\' ) != pszPath[1] ) || ( TEXT( '\0' ) == pszPath[3] ) )
    {
        return S_FALSE;
    }

    if( fNetValidate )
    {
        memset( &nr, 0, sizeof( NETRESOURCE ) );
        StrCpyN( pszPathBuffer, pszPath, MAX_PATH );

        nr.dwType = RESOURCETYPE_DISK;
        nr.lpRemoteName = pszPathBuffer;

        dwResult = WNetAddConnection3( hWndParent, &nr, NULL, NULL, 
            CONNECT_TEMPORARY | CONNECT_INTERACTIVE );

        if ( NO_ERROR != dwResult )
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------//
BOOL _IsFullPathMinusDriveLetter( LPCTSTR pszPath )
{
    if( NULL == pszPath || PathIsUNC( pszPath ) )
        return FALSE;
    ASSERT( !PathIsRelative( pszPath ) );

    // Eat whitespace
    for( ; ( TEXT( '\0' ) != *pszPath ) && ( TEXT( ' ' ) == *pszPath ) ; pszPath = CharNext( pszPath ) );

    return TEXT('\\') == *pszPath &&
            -1 == PathGetDriveNumber( pszPath );
}

//-------------------------------------------------------------------------//
inline BOOL _PathLooksLikeFilePattern( LPCTSTR pszPath )
{
    return StrPBrk( pszPath, TEXT("?*") ) != NULL;
}

//-------------------------------------------------------------------------//
inline BOOL _PathIsDblSlash( LPCTSTR pszPath )
{
    if( pszPath )
        return (pszPath[0] == TEXT('\\') && pszPath[1] == TEXT('\\'));

    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _PathIsUNCServerShareOrSub( LPCTSTR pszPath )
{
    if( pszPath )
    {
        if( _PathIsDblSlash( pszPath ) )
        {
            int cSlash = 0;
            LPTSTR psz;
            for(psz = (LPTSTR)pszPath; psz && *psz; psz = CharNext(psz) )
            {
                if (*psz==TEXT('\\'))
                    cSlash++;
            }
            return (cSlash >= 3);
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _IsRegItemPath( LPCTSTR pszPath )
{
    if( pszPath && *pszPath )
    {
        if( lstrlen( pszPath ) > 3 && 
            TEXT(':') == pszPath[0] &&
            TEXT(':') == pszPath[1] &&
            TEXT('{') == pszPath[2] )
        {
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _IsPathLocalHarddrive( LPCTSTR pszPath )
{
    if( !PathIsUNC( pszPath ) )
    {
        int iDrive;
        if( (iDrive = PathGetDriveNumber( pszPath )) != -1 )
        {
            TCHAR szRoot[16];
            return DRIVE_FIXED == GetDriveType( PathBuildRoot( szRoot, iDrive ) );
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
BOOL _IsPathLocalRoot( LPCTSTR pszPath )
{
    if( !PathIsUNC( pszPath ) )
    {
        int iDrive;
        if( (iDrive = PathGetDriveNumber( pszPath )) != -1 )
        {
            TCHAR szRoot[16];
            if( PathBuildRoot( szRoot, iDrive ) )
            {
                if( 0 == StrCmpI( szRoot, pszPath ) )
                {
                    switch( GetDriveType( szRoot ) )
                    {
                        case DRIVE_REMOVABLE:
                        case DRIVE_FIXED:
                        case DRIVE_REMOTE:
                        case DRIVE_CDROM:
                        case DRIVE_RAMDISK:
                            return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves the name for the specified folder.
HRESULT _GetNameFromFolder( 
    IShellFolder* psf, 
    BOOL bForParsing, 
    LPTSTR pszName, 
    UINT cchName )
{
    HRESULT hr = E_FAIL;
    //  Use IPersistFolder2:
    IPersistFolder2* ppf2;
    if( SUCCEEDED( (hr = psf->QueryInterface( IID_IPersistFolder2, (void**)&ppf2 )) ) )
    {
        LPITEMIDLIST pidl = NULL;
        if( SUCCEEDED( (hr = ppf2->GetCurFolder( &pidl )) ) )
        {
            if( bForParsing )
            {
                ASSERT( cchName >= MAX_PATH );
                hr = SHGetPathFromIDList( pidl, pszName ) ? S_OK : E_FAIL;
            }
            else
            {
                hr = SHGetNameAndFlags( pidl, SHGDN_NORMAL, pszName, cchName, NULL );
            }

            ILFree( pidl );
        }
        ppf2->Release();
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Retrieves the path, and optionally, the display name for the specified folder object.
HRESULT _GetPathFromFolder( IShellFolder* psf, LPTSTR pszPath, UINT cchPath, LPTSTR pszName, UINT cchName )
{
    IPersistFolder3* ppf3;
    PERSIST_FOLDER_TARGET_INFO pfti;
    HRESULT hr = E_FAIL;

    *pszPath = 0;
    if( pszName ) *pszName = 0;

    //  We'll try IPersistFolder3 first to ensure that 
    //  we get the target of a folder shortcuts
    if( SUCCEEDED( (hr = psf->QueryInterface( IID_IPersistFolder3, (void**)&ppf3 ))) )
    {
        if( SUCCEEDED( (hr = ppf3->GetFolderTargetInfo(&pfti))) )
        {
            //  PERSIST_FOLDER_TARGET_INFO::pidlTargetFolder may sometimes
            //  be NULL, so rely first on the PERSIST_FOLDER_TARGET_INFO::szTargetParsingName.   
            //  The only case this should be empty is if we have a special folder,
            //  in which case we don't want to search it anyway.
            if( *pfti.szTargetParsingName )
                SHUnicodeToTChar( pfti.szTargetParsingName, pszPath, cchPath );
            else
                hr = E_FAIL;
            
            if( pfti.pidlTargetFolder )
            {
                if( 0 == *pszPath )
                {
                    ASSERT( cchPath >= MAX_PATH );
                    hr = SHGetPathFromIDList( pfti.pidlTargetFolder, pszPath ) ? S_OK : E_FAIL;
                }
                
                if( pszName )
                {
                    SHGetNameAndFlags( pfti.pidlTargetFolder, SHGDN_NORMAL, 
                                       pszName, cchName, NULL );
                }
                
                ILFree(pfti.pidlTargetFolder);
            }
        }
        ppf3->Release();
    }

    //  fall-back plan:
    if( 0 == *pszPath )
        hr = _GetNameFromFolder( psf, TRUE, pszPath, cchPath );

    if( pszName && 0 == *pszName )
        _GetNameFromFolder( psf, FALSE, pszName, cchName );

    return hr;
}

//-------------------------------------------------------------------------//
BOOL _GetCurrentPathAndNamespace( 
    IUnknown* punkSite, 
    OUT LPTSTR pszPath, 
    IN UINT cchPath, 
    OUT LPTSTR pszNamespace, 
    IN UINT cchNamespace )
{
    USES_CONVERSION;
    BOOL    bScoped = FALSE;
    ASSERT( punkSite );
    HRESULT hr = E_FAIL ;

    //  (1) Retrieve the file system path of the active defview's shellfolder
    IShellBrowser* psb;
    hr = IUnknown_QueryService( punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb );
    if( SUCCEEDED(hr) ) 
    {
        IShellView* psv;
        hr = psb->QueryActiveShellView( &psv );
        if( SUCCEEDED( hr ) ) 
        {
            IDefViewFrame* pdvf;            
            hr = psv->QueryInterface( IID_IDefViewFrame, (void**)&pdvf );
            if( SUCCEEDED( hr ) ) 
            {
                IShellFolder* psf;                
                hr = pdvf->GetShellFolder( &psf );
                if( SUCCEEDED( hr ) ) 
                {
                    hr = _GetPathFromFolder( psf, pszPath, cchPath, pszNamespace, cchNamespace );
                    
                    // don't treat failure as error;
                    if( FAILED(hr) )
                        hr = S_FALSE; 

                    psf->Release();
                }
                pdvf->Release();
            }
            psv->Release();
        }
        psb->Release();
    }
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT _PathValidate( LPCTSTR pszPath, OPTIONAL HWND hWndParent = NULL, OPTIONAL BOOL fNetValidate = FALSE, OUT OPTIONAL LPDWORD pdwAttr = NULL )
{
    DWORD dwAttr;
    HRESULT hr = S_OK;

    if( pdwAttr )
        *pdwAttr = 0L;

    hr = _IsPathValidUNC( hWndParent, fNetValidate, pszPath );
    if (S_OK == hr)
    {
        // We are done.
    }
    else if (E_FAIL == hr)
    {
        hr = E_FILE_NOT_FOUND;
    }
    else
    {
        if( _IsPathList( pszPath ) /*tokenized*/ || PathIsSlow( pszPath, FILE_ATTRIBUTE_NOTFOUND ) )
        {
            hr = S_OK;  // Skip check for slow files.
        }
        else
        {
            if( PathIsRelative( pszPath ) || _IsFullPathMinusDriveLetter( pszPath ) )
            {
                hr = E_FILE_NOT_FOUND; // don't accept anything but a fully qualified path at this point.
                dwAttr = FILE_ATTRIBUTE_NOTFOUND;
            }
            else
            {
                dwAttr = GetFileAttributes( pszPath );  // Does it exist?
    
                if( FILE_ATTRIBUTE_NOTFOUND == dwAttr )
                {
                    // Maybe the disk isn't inserted, so allow the user
                    // the chance to insert it.
                    if (hWndParent && SUCCEEDED(SHPathPrepareForWrite(hWndParent, NULL, pszPath, SHPPFW_IGNOREFILENAME)))
                    {
                        dwAttr = GetFileAttributes( pszPath );  // Does it exist now?
                    }

                    if( FILE_ATTRIBUTE_NOTFOUND == dwAttr )
                        hr = E_FILE_NOT_FOUND;    // It doesn't exist.
                    else
                        hr = S_OK;      // It exists now.
                }
            }

            if( pdwAttr )
                *pdwAttr = dwAttr;
        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
BOOL _FmtError( UINT nIDFmt, LPCTSTR pszSub, LPTSTR szBuf, int cchBuf )
{
    TCHAR szFmt[MAX_PATH];

    if( EVAL( LoadString( HINST_THISDLL, nIDFmt, szFmt, ARRAYSIZE(szFmt) ) ) )
    {
        wnsprintf( szBuf, cchBuf, szFmt, pszSub );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Converts a TCHAR string to a variant of type VT_BSTR
HRESULT _T2BstrVariant( LPCTSTR pszText, OUT VARIANT* pvar )
{
    VariantInit( pvar );
    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocStringT( pszText );
    return S_OK;
}

//-------------------------------------------------------------------------//
//  Concatenates a BSTR string token and delimiter to a variant of type VT_BSTR.
HRESULT _BstrAppendToken( IN OUT BSTR* pbstr, IN LPCTSTR pszDelim, IN LPCTSTR pszToken )
{
    USES_CONVERSION;
    ASSERT( pszToken );
    ASSERT( pbstr );

    int cch = lstrlen( pszToken ) + 
              (pszDelim ? lstrlen( pszDelim ) : 0) +
              (*pbstr ? lstrlenW( *pbstr ) : 0);

    LPWSTR pwszBuf;
    if( NULL == (pwszBuf = new WCHAR[cch+1]) )
        return E_OUTOFMEMORY;
    *pwszBuf = 0;

    if( *pbstr && **pbstr )
    {
        StrCatW( pwszBuf, *pbstr );
        if( *pwszBuf && pszDelim && *pszDelim )
            StrCatW( pwszBuf, T2W( (LPTSTR)pszDelim ) );
    }
    StrCatW( pwszBuf, T2W( (LPTSTR)pszToken ) );

    SysFreeString( *pbstr );
    *pbstr = SysAllocString( pwszBuf );
    delete [] pwszBuf;
    
    return *pbstr ? S_OK : E_OUTOFMEMORY; 
}

//-------------------------------------------------------------------------//
//  Retrieves the window text as a variant value of the specified type.
HRESULT _GetWindowValue( HWND hwndDlg, UINT nID, VARTYPE vt, VARIANT* pvar )
{
    TCHAR   szText[MAX_EDIT];
    HRESULT hr = S_FALSE;
    
    VariantInit( pvar );
    pvar->vt = vt;

    if( ::GetDlgItemText( hwndDlg, nID, szText, ARRAYSIZE(szText) ) )
    {
        switch( vt )
        {
            case VT_BSTR:
            default:
                hr = _T2BstrVariant( szText, pvar );
                break;
        }
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Loads the window text from a string resource.
BOOL _LoadWindowText( HWND hwnd, UINT nIDString )
{
    TCHAR szText[MAX_PATH];
    if( LoadString( HINST_THISDLL, nIDString, szText, ARRAYSIZE(szText) ) )
        return SetWindowText( hwnd, szText );
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves the window text as a variant value of the specified type.
HRESULT _SetWindowValue( HWND hwndDlg, UINT nID, const VARIANT* pvar )
{
    switch( pvar->vt )
    {
        case VT_BSTR:
            SetDlgItemTextW( hwndDlg, nID, pvar->bstrVal );
            return S_OK;

        case VT_UI4:
            SetDlgItemInt( hwndDlg, nID, pvar->uiVal, FALSE );
            return S_OK;
            
        case VT_I4:
            SetDlgItemInt( hwndDlg, nID, pvar->lVal, TRUE );
            return S_OK;
    }
    return E_NOTIMPL;
}

//-------------------------------------------------------------------------//
//  Adds a named constraint to the specified search command extension object
HRESULT _AddConstraint( ISearchCommandExt* pSrchCmd, LPCWSTR pwszConstraint, VARIANT* pvarValue )
{
    HRESULT hres;
    BSTR bstrConstraint = NULL;
    if( NULL == (bstrConstraint = SysAllocString( pwszConstraint )) )
        return E_OUTOFMEMORY;

    hres = pSrchCmd->AddConstraint( bstrConstraint, *pvarValue );
    SysFreeString( bstrConstraint );
    return hres;
}

//-------------------------------------------------------------------------//
void _PaintDlg( HWND hwndDlg, const CMetrics& metrics, HDC hdcPaint = NULL, LPCRECT prc = NULL )
{
    RECT rcPaint /* rcLine */;
    HDC  hdc = hdcPaint;

    if( NULL == hdcPaint )
        hdc = GetDC( hwndDlg );

    if( prc == NULL )
    {
        GetClientRect( hwndDlg, &rcPaint );
        prc = &rcPaint;
    }

    FillRect( hdc, prc, metrics.BkgndBrush() );
        
    if( NULL == hdcPaint )
        ReleaseDC( hwndDlg, hdc );
}

//-------------------------------------------------------------------------//
void _EnsureVisible( HWND hwndDlg, HWND hwndVis, CFileSearchBand* pfsb )
{
    ASSERT( pfsb );
    ASSERT( IsWindow( hwndDlg ) );
    ASSERT( IsWindow( hwndVis ) );
    
    RECT rcDlg, rcVis, rcX;
    GetWindowRect( hwndDlg, &rcDlg );
    GetWindowRect( hwndVis, &rcVis );

    if( IntersectRect( &rcX, &rcDlg, &rcVis ) )
        pfsb->EnsureVisible( &rcX );
}

//-------------------------------------------------------------------------//
inline BOOL _IsKeyPressed( int virtkey )
{
    return (GetKeyState( virtkey ) & 8000) != 0;
}

//-------------------------------------------------------------------------//
inline DWORD WaitForThreadCompletion( HANDLE hThread )
{
    if( INVALID_HANDLE_VALUE == hThread || NULL == hThread )
        return WAIT_OBJECT_0;

    return WaitForSingleObject( hThread, CBX_SNDMSG_TIMEOUT + 3000 );
}

//-------------------------------------------------------------------------//
HWND _CreateDivider( HWND hwndParent, UINT nIDC, const POINT& pt, int nThickness = 1, HWND hwndAfter = NULL )
{
    HWND hwndDiv = CreateWindowEx( 0, DIVWINDOW_CLASS, NULL,
                                   WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
                                   pt.x, pt.y, 400, 1, hwndParent, 
                                   (HMENU)nIDC, HINST_THISDLL, NULL );
    if( IsWindow( hwndDiv ) )
    {
        if( IsWindow( hwndAfter ) )
            SetWindowPos( hwndDiv, hwndAfter, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE );

        SendMessage( hwndDiv, DWM_SETHEIGHT, nThickness, 0L );
        return hwndDiv;
    }
    return NULL;
}

//-------------------------------------------------------------------------//
HWND _CreateLinkWindow( HWND hwndParent, UINT nIDC, const POINT& pt, UINT nIDCaption, BOOL bShow = TRUE )
{
    DWORD dwStyle = WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS;
    if( bShow )
        dwStyle |= WS_VISIBLE;
    
    HWND hwndCtl = CreateWindowEx( 0, LINKWINDOW_CLASS, NULL, dwStyle,
                                   pt.x, pt.y, 400, 18, hwndParent, 
                                   (HMENU)nIDC, HINST_THISDLL, NULL );
        
    if( IsWindow( hwndCtl ) )
    {
        _LoadWindowText( hwndCtl, nIDCaption  );
        return hwndCtl;
    }

    return NULL;
}

//-------------------------------------------------------------------------//
BOOL _EnableLink( HWND hwndLink, BOOL bEnable )
{
    LWITEM item;
    item.mask       = LWIF_ITEMINDEX|LWIF_STATE;
    item.stateMask  = LWIS_ENABLED;
    item.state      = bEnable ? LWIS_ENABLED : 0;
    item.iLink      = 0;

    return (BOOL)SendMessage( hwndLink, LWM_SETITEM, 0, (LPARAM)&item );
}

//-------------------------------------------------------------------------//
int _CreateSearchLinks( HWND hwndDlg, const POINT& pt, UINT nCtlIDdlg /* ctl ID of link to hwndDlg */ )
{
    const UINT rgCtlID[] = {
        IDC_SEARCHLINK_FILES,
        IDC_SEARCHLINK_COMPUTERS,
        IDC_SEARCHLINK_PRINTERS,
        IDC_SEARCHLINK_PEOPLE,
        IDC_SEARCHLINK_INTERNET,
    };
    const UINT rgCaptionID[] = {
        IDS_FSEARCH_SEARCHLINK_FILES,
        IDS_FSEARCH_SEARCHLINK_COMPUTERS,
        IDS_FSEARCH_SEARCHLINK_PRINTERS,
        IDS_FSEARCH_SEARCHLINK_PEOPLE,
        IDS_FSEARCH_SEARCHLINK_INTERNET,
    };
    int  cLinks = 0;
    BOOL bDSEnabled = _IsDirectoryServiceAvailable();

    for( int i = 0; i < ARRAYSIZE(rgCtlID); i++ )
    {
        //  block creation of csearch, psearch search links 
        //  if Directory Service is not available.
        if( ((IDC_SEARCHLINK_PRINTERS == rgCtlID[i]) && rgCtlID[i] != nCtlIDdlg && !bDSEnabled )
        ||  (IDC_SEARCHLINK_FILES == rgCtlID[i] && SHRestricted(REST_NOFIND)))
        {
            continue;
        }
        
        if( _CreateLinkWindow( hwndDlg, rgCtlID[i], pt, rgCaptionID[i] ) )
                cLinks++;
    }

    //  Disable link to current dialog:
    _EnableLink( GetDlgItem( hwndDlg, nCtlIDdlg ), FALSE );

    return cLinks;
}

//-------------------------------------------------------------------------//
void _LayoutLinkWindow( 
    IN HWND hwnd,     // parent window
    IN LONG left,     // left position of link
    IN LONG right,    // right position of link
    IN LONG yMargin,  // vertical padding between links
    IN OUT LONG& y,   // IN: where to start (RECT::top).  OUT: where the last link was positioned (RECT::bottom).
    IN const int nCtlID ) // ctl ID 
{
    HWND hwndLink;
    
    if( nCtlID > 0 )
    {
        hwndLink = GetDlgItem( hwnd, nCtlID );
        RECT rcLink;
        if( !IsWindow(hwndLink) )
            return;

        ::GetWindowRect( hwndLink, &rcLink );
        ::MapWindowPoints( HWND_DESKTOP, hwnd, (LPPOINT)&rcLink, POINTSPERRECT );
        rcLink.left  = left;
        rcLink.right = right;

        int cyIdeal = (int)::SendMessage( hwndLink, LWM_GETIDEALHEIGHT, RECTWIDTH( &rcLink ), 0L );
        if( cyIdeal >= 0 )
            rcLink.bottom = rcLink.top + cyIdeal;

        OffsetRect( &rcLink, 0, y - rcLink.top );
        y = rcLink.bottom;

        ::SetWindowPos( hwndLink, NULL, 
                        rcLink.left, rcLink.top, 
                        RECTWIDTH(&rcLink), RECTHEIGHT(&rcLink),
                        SWP_NOZORDER|SWP_NOACTIVATE );
    }
    else if( nCtlID < 0 )
    {
        //  this is a divider window
        hwndLink = GetDlgItem( hwnd, -nCtlID );
        ::SetWindowPos( hwndLink, NULL, left, y + yMargin/2, right - left, 1, 
                        SWP_NOZORDER|SWP_NOACTIVATE );
    }
    y += yMargin;
}

//-------------------------------------------------------------------------//
void _LayoutLinkWindows( 
    IN HWND hwnd,     // parent window
    IN LONG left,     // left position of links
    IN LONG right,    // right position of links
    IN LONG yMargin,  // vertical padding between links
    IN OUT LONG& y,   // IN: where to start (RECT::top).  OUT: where the last link was positioned (RECT::bottom).
    IN const int rgCtlID[],// array of link ctl IDs.  use IDC_SEPARATOR for separators
    IN LONG cCtlID )  // number of array elements to layout in rgCtlID.
{
    for( int i = 0; i < cCtlID; i++ )
        _LayoutLinkWindow( hwnd, left, right, yMargin, y, rgCtlID[i] );
}

//-------------------------------------------------------------------------//
//  Retrieves AutoComplete flags
//  BUGBUG: (this code was ripped off from shlwapi!util.cpp...
//
#define SZ_REGKEY_AUTOCOMPLETE_TAB          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_AUTOCOMPLETE_TAB        TEXT("Always Use Tab")
#define BOOL_NOT_SET                        0x00000005
DWORD _GetAutoCompleteSettings()
{
    DWORD dwACOptions = 0;

    if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
    {
        dwACOptions |= ACO_AUTOAPPEND;
    }

    if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
    {
        dwACOptions |= ACO_AUTOSUGGEST;
    }

    // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
    // operating systems that use AutoComplete have traditionally used the TAB key to
    // iterate thru the AutoComplete possibilities.  We need to default to disable the
    // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
    // turn it on 
    static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
    if (BOOL_NOT_SET == s_fAlwaysUseTab)
        s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_AUTOCOMPLETE_TAB, SZ_REGVALUE_AUTOCOMPLETE_TAB, FALSE, FALSE);
        
    if (s_fAlwaysUseTab)
        dwACOptions |= ACO_USETAB;

    return dwACOptions;
}

//-------------------------------------------------------------------------//
//  Initializes and enables an MRU autocomplete list on the specified
//  edit control
HRESULT _InitializeMru( 
    IN HWND hwndEdit, 
    IAutoComplete2** ppAutoComplete, 
    LPCTSTR pszSubKey,
    IStringMru** ppStringMru )
{
    ASSERT(ppAutoComplete);
    ASSERT(ppStringMru);

    *ppAutoComplete = NULL;
    *ppStringMru = NULL;

    HRESULT hr = CoCreateInstance( CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, 
                                   IID_IAutoComplete2, (void**)ppAutoComplete );
    if( SUCCEEDED(hr) )
    {
        TCHAR szKey[MAX_PATH];
        if( CFileSearchBand::MakeBandSubKey( pszSubKey, szKey, ARRAYSIZE(szKey) ) > 0 )
        {
            hr = CStringMru::CreateInstance( HKEY_CURRENT_USER, szKey,
                                             AUTOSUGGEST_MRUCOUNT, FALSE, 
                                             IID_IStringMru, (void**)ppStringMru );
            if( SUCCEEDED(hr) )
            {
                hr = (*ppAutoComplete)->Init( hwndEdit, *ppStringMru, NULL, NULL );
            }
        }
        else
            hr = E_FAIL;
    }

    if( SUCCEEDED(hr) )
    {
        (*ppAutoComplete)->SetOptions( _GetAutoCompleteSettings() );
        (*ppAutoComplete)->Enable( TRUE );
    }
    else
    {
        ATOMICRELEASE((*ppAutoComplete));        
        ATOMICRELEASE((*ppStringMru));        
    }

    return hr;
}

//-------------------------------------------------------------------------//
HRESULT _AddMruStringFromWindow( IStringMru* pmru, HWND hwnd )
{
    ASSERT( IsWindow( hwnd ) );
    HRESULT hr = E_FAIL;

    if( pmru != NULL )
    {
        UINT cch = GetWindowTextLength( hwnd );
        if( cch > 0 )
        {
            LPOLESTR pwszText = new OLECHAR[cch + 1];
            if( NULL == pwszText )
                return E_OUTOFMEMORY;

            if( GetWindowTextW( hwnd, pwszText, cch + 1 ) > 0 )
            {
                hr = pmru->Add( pwszText );
            }
            delete [] pwszText;
        }
        else
            hr = S_FALSE;
    }
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT _TestAutoCompleteDropDownState( IAutoComplete2* pac2, DWORD dwTest )
{
    IAutoCompleteDropDown* pacdd;
    HRESULT hr = pac2->QueryInterface( IID_IAutoCompleteDropDown, (void**)&pacdd );
    if( SUCCEEDED(hr) )
    {
        DWORD dwFlags;
        if( SUCCEEDED( (hr = pacdd->GetDropDownStatus( &dwFlags, NULL )) ) )
            hr = (dwFlags & dwTest) ? S_OK : S_FALSE;
        pacdd->Release();
    }
    return hr;
}

//-------------------------------------------------------------------------//
inline HWND _IndexServiceHelp( HWND hwndOwner )
{
    return ::HtmlHelp( hwndOwner, INDEXSERVICE_HELPFILE, HH_DISPLAY_TOPIC, 
                    (DWORD_PTR)INDEXSERVICE_HELPTOPIC );
}

//-------------------------------------------------------------------------//
//  CSubDlg impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnNcCalcsize( UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
    InflateRect( (LPRECT)lParam, -SUBDLG_BORDERWIDTH, -SUBDLG_BORDERWIDTH );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnNcPaint( UINT, WPARAM wParam, LPARAM lParam, BOOL& )
{
    RECT    rc;
    HDC     hdc;
    HBRUSH  hbr = _pfsb->GetMetrics().BorderBrush();
    
    GetWindowRect( Hwnd(), &rc );
    OffsetRect( &rc, -rc.left, -rc.top );
    
    if( hbr && (hdc = GetWindowDC( Hwnd() )) != NULL )
    {
        for( int i=0; i < SUBDLG_BORDERWIDTH; i++ )
        {
            FrameRect( hdc, &rc, hbr );
            InflateRect( &rc, -1, -1 );
        }

        ReleaseDC( Hwnd(), hdc );
    }
    
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnCtlColor(UINT, WPARAM wParam, LPARAM, BOOL& )
{
    SetTextColor( (HDC)wParam, _pfsb->GetMetrics().TextColor() );
    SetBkColor( (HDC)wParam, _pfsb->GetMetrics().BkgndColor() );
    return (LRESULT)_pfsb->GetMetrics().BkgndBrush();
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnPaint( UINT, WPARAM, LPARAM, BOOL& )
{
    HDC         hdc;
    PAINTSTRUCT ps;

    //  Just going to call BeginPaint and EndPaint.  All
    //  painting done in WM_ERASEBKGND handler to avoid flicker.
    if( (hdc = BeginPaint( _hwnd, &ps )) != NULL )
    {
        EndPaint( _hwnd, &ps );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnSize( UINT, WPARAM, LPARAM, BOOL& )
{
    ASSERT( IsWindow( Hwnd() ) ); // was _Attach() called, e.g. from WM_INITDIALOG?
    _PaintDlg( Hwnd(), _pfsb->GetMetrics() );
    ValidateRect( Hwnd(), NULL );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnEraseBkgnd( UINT, WPARAM wParam, LPARAM, BOOL& )
{
    _PaintDlg( Hwnd(), _pfsb->GetMetrics(), (HDC)wParam );
    ValidateRect( Hwnd(), NULL );
    return TRUE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CSubDlg::TranslateAccelerator( LPMSG lpmsg )
{
    if( _pfsb->IsKeyboardScroll( lpmsg ) )
        return S_OK;

    return _pfsb->IsDlgMessage( Hwnd(), lpmsg );
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnChildSetFocusCmd( 
    WORD wNotifyCode, 
    WORD wID, 
    HWND hwndCtl, 
    BOOL& bHandled )
{
    _EnsureVisible( _hwnd, hwndCtl, _pfsb );
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnChildSetFocusNotify( int, LPNMHDR pnmh, BOOL& bHandled )
{
    _EnsureVisible( _hwnd, pnmh->hwndFrom, _pfsb );
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnChildKillFocusCmd( WORD, WORD, HWND hwndCtl, BOOL& )
{
    if( _pBandDlg )
        _pBandDlg->RememberFocus( hwndCtl );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnChildKillFocusNotify( int, LPNMHDR pnmh, BOOL& )
{
    if( _pBandDlg )
        _pBandDlg->RememberFocus( pnmh->hwndFrom );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSubDlg::OnComboExEndEdit( int, LPNMHDR pnmh, BOOL& )
{
    if( CBENF_KILLFOCUS == ((NMCBEENDEDIT*)pnmh)->iWhy )
    {
        if( _pBandDlg )
            _pBandDlg->RememberFocus( pnmh->hwndFrom );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
// CDateDlg impl
//-------------------------------------------------------------------------//
#define RECENTMONTHSRANGE_HIGH      999  
#define RECENTDAYSRANGE_HIGH        RECENTMONTHSRANGE_HIGH
#define RECENTMONTHSRANGE_HIGH_LEN  3 // count of digits in RECENTMONTHSRANGE_HIGH
#define RECENTDAYSRANGE_HIGH_LEN    RECENTMONTHSRANGE_HIGH_LEN
#define RECENTMONTHSRANGE_LOW       1
#define RECENTDAYSRANGE_LOW         RECENTMONTHSRANGE_LOW

//-------------------------------------------------------------------------//
LRESULT CDateDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ASSERT( _pfsb );
    _Attach( m_hWnd );

    HWND hwndCombo = GetDlgItem( IDC_WHICH_DATE );

    SendDlgItemMessage( IDC_RECENT_MONTHS_SPIN, UDM_SETRANGE32, 
                        RECENTMONTHSRANGE_HIGH, RECENTMONTHSRANGE_LOW );
    SendDlgItemMessage( IDC_RECENT_DAYS_SPIN, UDM_SETRANGE32, 
                        RECENTDAYSRANGE_HIGH, RECENTDAYSRANGE_LOW );

    SendDlgItemMessage( IDC_RECENT_MONTHS, EM_LIMITTEXT, RECENTMONTHSRANGE_HIGH_LEN, 0L );
    SendDlgItemMessage( IDC_RECENT_DAYS,   EM_LIMITTEXT, RECENTDAYSRANGE_HIGH_LEN, 0L );

    SYSTEMTIME st[2] = {0};

    // lower limit -- dos date does not support anything before 1/1/1980
    st[0].wYear = 1980;
    st[0].wMonth = 1;
    st[0].wDay = 1;
    // upper limit
    st[1].wYear = 2099;
    st[1].wMonth = 12;
    st[1].wDay = 31;
    SendDlgItemMessage( IDC_DATERANGE_BEGIN, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    SendDlgItemMessage( IDC_DATERANGE_END,   DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    
    _LoadStringToCombo( hwndCombo, -1, 
                        IDS_FSEARCH_MODIFIED_DATE, 
                        IDS_FSEARCH_MODIFIED_DATE );
    _LoadStringToCombo( hwndCombo, -1, 
                        IDS_FSEARCH_CREATION_DATE, 
                        IDS_FSEARCH_CREATION_DATE );
    _LoadStringToCombo( hwndCombo, -1, 
                        IDS_FSEARCH_ACCESSED_DATE, 
                        IDS_FSEARCH_ACCESSED_DATE );

    Clear();

    return TRUE;  // Let the system set the focus
}

//-------------------------------------------------------------------------//
BOOL CDateDlg::Validate()
{
    return TRUE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CDateDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    VARIANT var;
    BOOL    bErr;
    UINT_PTR nIDString = _GetComboData( GetDlgItem( IDC_WHICH_DATE ) );

    var.vt = VT_UI4;
    var.ulVal = (IDS_FSEARCH_MODIFIED_DATE == nIDString) ? 1 :
                (IDS_FSEARCH_CREATION_DATE == nIDString) ? 2 :
                (IDS_FSEARCH_ACCESSED_DATE == nIDString) ? 3 : 0;
    ASSERT( var.ulVal );
    
    HRESULT hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_WHICHDATE ), &var );
    
    if( IsDlgButtonChecked( IDC_USE_RECENT_MONTHS ) )
    {
        var.vt = VT_I4;
        var.ulVal = (ULONG)SendDlgItemMessage( IDC_RECENT_MONTHS_SPIN, UDM_GETPOS32, 0, (LPARAM)&bErr );
        if( !bErr )
            hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_DATENMONTHS ), &var );

    }
    else if( IsDlgButtonChecked( IDC_USE_RECENT_DAYS ) )
    {
        var.vt = VT_I4;
        var.ulVal = (ULONG)SendDlgItemMessage( IDC_RECENT_DAYS_SPIN, UDM_GETPOS32, 0, (LPARAM)&bErr );
        if( !bErr )
            hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_DATENDAYS ), &var );
    }
    else if( IsDlgButtonChecked( IDC_USE_DATE_RANGE ) )     
    {
        SYSTEMTIME stBegin, stEnd;
        LRESULT    lRetBegin, lRetEnd;

        var.vt = VT_DATE;

        lRetBegin = SendDlgItemMessage( IDC_DATERANGE_BEGIN, DTM_GETSYSTEMTIME, 0L, (LPARAM)&stBegin );
        lRetEnd   = SendDlgItemMessage( IDC_DATERANGE_END,   DTM_GETSYSTEMTIME, 0L, (LPARAM)&stEnd );

        if( GDT_VALID == lRetBegin && GDT_VALID == lRetEnd )
        {
#ifdef DEBUG
            FILETIME ft;
            //validate that we were passed a correct date
            //SystemTimeToFileTime calls internal API IsValidSystemTime.
            //This will save us from OLE Automation bug# 322789

            // the only way we can get a date is through date/time picker
            // control which should validate the dates so just assert...
            ASSERT(SystemTimeToFileTime(&stBegin, &ft));
#endif
            SystemTimeToVariantTime( &stBegin, &var.date );
            hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_DATEGE ), &var );
#ifdef DEBUG
            ASSERT(SystemTimeToFileTime(&stEnd, &ft));
#endif

            SystemTimeToVariantTime( &stEnd, &var.date );
            hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_DATELE ), &var );
        }
    }
    
    return hr;
}

//-------------------------------------------------------------------------//
//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CDateDlg::RestoreConstraint( const BSTR bstrName, const VARIANT* pValue )
{
    HRESULT hr = E_FAIL;
    BOOL    bUseMonths = FALSE,
            bUseDays = FALSE,
            bUseRange = FALSE;
            
    if( IsConstraintName( FSBC_WHICHDATE, bstrName) )
    {
        ASSERT( VT_I4 == pValue->vt )
        UINT nIDS = pValue->lVal == 1 ? IDS_FSEARCH_MODIFIED_DATE :
                    pValue->lVal == 2 ? IDS_FSEARCH_CREATION_DATE :
                    pValue->lVal == 3 ? IDS_FSEARCH_ACCESSED_DATE : 0;

        if( nIDS != 0 )
            _SelectComboData( GetDlgItem( IDC_WHICH_DATE ), nIDS );

        return nIDS == IDS_FSEARCH_MODIFIED_DATE /*default*/ ? 
                       S_FALSE /* don't open */: S_OK /* open */;
    }
    
    if( IsConstraintName( FSBC_DATENMONTHS, bstrName ) )
    {
        ASSERT( VT_I4 == pValue->vt );
        bUseMonths = TRUE;
        _SetWindowValue( m_hWnd, IDC_RECENT_MONTHS, pValue );
        hr = S_OK;
    }
    else if( IsConstraintName( FSBC_DATENDAYS, bstrName ) )
    {
        ASSERT( VT_I4 == pValue->vt );
        bUseDays = TRUE;
        _SetWindowValue( m_hWnd, IDC_RECENT_DAYS, pValue );
        hr = S_OK;
    }
    else if( IsConstraintName( FSBC_DATEGE, bstrName ) )
    {
        ASSERT( VT_DATE == pValue->vt );
        bUseRange = TRUE;

        SYSTEMTIME st;
        VariantTimeToSystemTime( pValue->date, &st );
        SendDlgItemMessage( IDC_DATERANGE_BEGIN, DTM_SETSYSTEMTIME, 0L, (LPARAM)&st );
        hr = S_OK;
    }
    else if( IsConstraintName( FSBC_DATELE, bstrName ) )
    {
        ASSERT( VT_DATE == pValue->vt );
        bUseRange = TRUE;

        SYSTEMTIME st;
        VariantTimeToSystemTime( pValue->date, &st );
        SendDlgItemMessage( IDC_DATERANGE_END, DTM_SETSYSTEMTIME, 0L, (LPARAM)&st );
        hr = S_OK;
    }

    if( S_OK == hr )
    {
        CheckDlgButton( IDC_USE_RECENT_MONTHS, bUseMonths );
        CheckDlgButton( IDC_USE_RECENT_DAYS,   bUseDays );
        CheckDlgButton( IDC_USE_DATE_RANGE,    bUseRange );
        EnableControls();
    }

    return hr;
}

//-------------------------------------------------------------------------//
void CDateDlg::Clear()
{
    SendDlgItemMessage( IDC_WHICH_DATE, CB_SETCURSEL, 0, 0L );

    CheckDlgButton( IDC_USE_RECENT_MONTHS, 0 );
    SetDlgItemInt( IDC_RECENT_MONTHS, 1, FALSE );

    CheckDlgButton( IDC_USE_RECENT_DAYS, 0 );
    SetDlgItemInt( IDC_RECENT_DAYS, 1, FALSE );

    CheckDlgButton( IDC_USE_DATE_RANGE, 1 );

    SYSTEMTIME stNow, stPrev;
    GetLocalTime( &stNow );
    SendDlgItemMessage( IDC_DATERANGE_END, DTM_SETSYSTEMTIME, 0, (LPARAM)&stNow );

    //  Subtract 90 days from current date and stuff in date low range
    _CalcDateOffset( &stNow, 0, -1 /* 1 month ago */, &stPrev );
    SendDlgItemMessage( IDC_DATERANGE_BEGIN,  DTM_SETSYSTEMTIME, 0, (LPARAM)&stPrev );

    EnableControls();
}

//-------------------------------------------------------------------------//
LRESULT CDateDlg::OnSize( UINT, WPARAM wParam, LPARAM lParam, BOOL& )
{
    POINTS pts = MAKEPOINTS( lParam );

    _PaintDlg( m_hWnd, _pfsb->GetMetrics() );
    ValidateRect( NULL );

    RECT rc;
    HWND hwndCtl = GetDlgItem( IDC_WHICH_DATE );
    ASSERT( hwndCtl );
    
    ::GetWindowRect( hwndCtl, &rc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    rc.right = pts.x - _pfsb->GetMetrics().CtlMarginX();
    
    ::SetWindowPos( GetDlgItem( IDC_WHICH_DATE ), NULL, 0, 0, 
                    RECTWIDTH(&rc), RECTHEIGHT(&rc),
                    SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );

    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CDateDlg::OnMonthDaySpin( int nIDSpin, LPNMHDR pnmh, BOOL& bHandled )
{
    LPNMUPDOWN pud = (LPNMUPDOWN)pnmh;
    pud->iDelta *= -1; // increments of 1 month/day
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CDateDlg::OnBtnClick( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    EnableControls();
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CDateDlg::OnMonthsKillFocus( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    _EnforceNumericEditRange( m_hWnd, IDC_RECENT_MONTHS, IDC_RECENT_MONTHS_SPIN,
                              RECENTMONTHSRANGE_LOW, RECENTMONTHSRANGE_HIGH );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CDateDlg::OnDaysKillFocus( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    _EnforceNumericEditRange( m_hWnd, IDC_RECENT_DAYS, IDC_RECENT_DAYS_SPIN,
                             RECENTDAYSRANGE_LOW, RECENTDAYSRANGE_HIGH );
    return 0L;
}

//-------------------------------------------------------------------------//
void CDateDlg::EnableControls()
{
    UINT nSel = IsDlgButtonChecked( IDC_USE_RECENT_MONTHS ) ? IDC_USE_RECENT_MONTHS :
                IsDlgButtonChecked( IDC_USE_RECENT_DAYS ) ? IDC_USE_RECENT_DAYS :
                IsDlgButtonChecked( IDC_USE_DATE_RANGE ) ? IDC_USE_DATE_RANGE : 0;

    ::EnableWindow( GetDlgItem( IDC_RECENT_MONTHS ),      IDC_USE_RECENT_MONTHS == nSel );
    ::EnableWindow( GetDlgItem( IDC_RECENT_MONTHS_SPIN ), IDC_USE_RECENT_MONTHS == nSel );
    ::EnableWindow( GetDlgItem( IDC_RECENT_DAYS ),        IDC_USE_RECENT_DAYS == nSel );
    ::EnableWindow( GetDlgItem( IDC_RECENT_DAYS_SPIN ),   IDC_USE_RECENT_DAYS == nSel );
    ::EnableWindow( GetDlgItem( IDC_DATERANGE_BEGIN ),    IDC_USE_DATE_RANGE == nSel );
    ::EnableWindow( GetDlgItem( IDC_DATERANGE_END ),      IDC_USE_DATE_RANGE == nSel );
}

//-------------------------------------------------------------------------//
// CSizeDlg impl
//-------------------------------------------------------------------------//

#define FILESIZERANGE_LOW       0
#define FILESIZERANGE_HIGH      999999
#define FILESIZERANGE_HIGH_LEN  6 // count of digits in FILESIZERANGE_HIGH

//-------------------------------------------------------------------------//
LRESULT CSizeDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _Attach( m_hWnd );

    HWND hwndCombo = GetDlgItem( IDC_WHICH_SIZE );
    SendDlgItemMessage( IDC_FILESIZE_SPIN, UDM_SETRANGE32,
                        FILESIZERANGE_HIGH, FILESIZERANGE_LOW /*Kb*/ );
    SendDlgItemMessage( IDC_FILESIZE, EM_LIMITTEXT, FILESIZERANGE_HIGH_LEN, 0L );

    _LoadStringToCombo( hwndCombo, -1, 
                        IDS_FSEARCH_SIZE_GREATEREQUAL, 
                        IDS_FSEARCH_SIZE_GREATEREQUAL );
    _LoadStringToCombo( hwndCombo, -1, 
                        IDS_FSEARCH_SIZE_LESSEREQUAL, 
                        IDS_FSEARCH_SIZE_LESSEREQUAL );

    Clear();

    return TRUE;  // Let the system set the focus
}

//-------------------------------------------------------------------------//
STDMETHODIMP CSizeDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    VARIANT var;
    BOOL    bErr = FALSE;
    HRESULT hr = S_FALSE;
    UINT_PTR nIDString = _GetComboData( GetDlgItem( IDC_WHICH_SIZE ) );

    var.vt = VT_UI4;
    var.ulVal = (ULONG)SendDlgItemMessage( IDC_FILESIZE_SPIN, UDM_GETPOS32, 0, (LPARAM)&bErr );
    
    if( !bErr )
    {
        var.ulVal *= 1024; // KB to bytes.
        LPCWSTR pwszConstraint = (IDS_FSEARCH_SIZE_GREATEREQUAL == nIDString) ? 
                                    GetConstraintName( FSBC_SIZEGE ) :
                                 (IDS_FSEARCH_SIZE_LESSEREQUAL == nIDString) ? 
                                    GetConstraintName( FSBC_SIZELE ) : 
                                 NULL;

        if( pwszConstraint )
            hr = _AddConstraint( pSrchCmd, pwszConstraint, &var );
    }

    return hr;
}

//-------------------------------------------------------------------------//
//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CSizeDlg::RestoreConstraint( const BSTR bstrName, const VARIANT* pValue )
{
    HRESULT hr = E_FAIL;
    UINT    nID = 0;

    if( IsConstraintName( FSBC_SIZEGE, bstrName ) )
    {
        nID = IDS_FSEARCH_SIZE_GREATEREQUAL;
        hr =  S_OK;
    }
    else if( IsConstraintName( FSBC_SIZELE, bstrName ) )
    {
        nID = IDS_FSEARCH_SIZE_LESSEREQUAL;
        hr = S_OK;
    }

    if( S_OK == hr )
    {
        ASSERT( VT_UI4 == pValue->vt );
        ULONG ulSize = pValue->ulVal/1024; // convert bytes to KB
        SetDlgItemInt( IDC_FILESIZE, ulSize, FALSE );

        ASSERT( nID != 0 );
        _SelectComboData( GetDlgItem( IDC_WHICH_SIZE ), nID );
    }
    
    return hr;
}


//-------------------------------------------------------------------------//
void CSizeDlg::Clear()
{
    SendDlgItemMessage( IDC_WHICH_SIZE, CB_SETCURSEL, 0, 0L );
    SetDlgItemInt( IDC_FILESIZE, 0, FALSE );    
}

//-------------------------------------------------------------------------//
LRESULT CSizeDlg::OnSizeSpin( int nIDSpin, LPNMHDR pnmh, BOOL& bHandled )
{
    LPNMUPDOWN pud = (LPNMUPDOWN)pnmh;
    pud->iDelta *= -10;  // increments of 10KB
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CSizeDlg::OnSizeKillFocus( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    _EnforceNumericEditRange( m_hWnd, IDC_FILESIZE, IDC_FILESIZE_SPIN,
                              FILESIZERANGE_LOW, FILESIZERANGE_HIGH );
    return 0L;
}

//-------------------------------------------------------------------------//
// CTypeDlg impl
//-------------------------------------------------------------------------//


//-------------------------------------------------------------------------//
CTypeDlg::CTypeDlg( CFileSearchBand* pfsb )
    :   CSubDlg( pfsb ), _hFileAssocThread(INVALID_HANDLE_VALUE)
{
    *_szRestoredExt = 0;
}

//-------------------------------------------------------------------------//
CTypeDlg::~CTypeDlg()
{
    //  Wait for thread completion
    DWORD dwWait = WaitForThreadCompletion( _hFileAssocThread );
    ASSERT( WAIT_TIMEOUT != dwWait );

    if( WAIT_TIMEOUT == dwWait )
    {
        ASSERT( FALSE ); // file types combo population thread is hung.
    }
    CloseHandle(_hFileAssocThread);
}

//-------------------------------------------------------------------------//
LRESULT CTypeDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND        hwndCombo = GetDlgItem( IDC_FILE_TYPE );
    HIMAGELIST  hil = _GetSystemImageListSmallIcons();

    _Attach( m_hWnd );
    ::SendMessage( hwndCombo, CBEM_SETEXTENDEDSTYLE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE);
    
    ::SendMessage(hwndCombo, CBEM_SETIMAGELIST, 0, (LPARAM)hil);
    ::SendMessage(hwndCombo, CBEM_SETEXSTYLE, 0, 0);

    //  Launch thread to populate the file types combo.
    _threadState.hwndCtl = GetDlgItem( IDC_FILE_TYPE );
    _threadState.pvParam = this;
    _threadState.fComplete = FALSE;
    _threadState.fCancel   = FALSE;
    DWORD dwThreadID;
    _hFileAssocThread = CreateThread( NULL, 0L, FileAssocThreadProc, 
                                       &_threadState, 0, &dwThreadID );

    return TRUE;  // Let the system set the focus
}

//-------------------------------------------------------------------------//
STDMETHODIMP CTypeDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    LPTSTR  pszText;
    HWND    hwndCombo = GetDlgItem( IDC_FILE_TYPE );
    ASSERT( hwndCombo );
    HRESULT hr = S_FALSE;

    if( _GetFileAssocComboSelItemText( hwndCombo, &pszText ) >= 0 && pszText )
    {
        VARIANT var;
        if( *pszText &&
            SUCCEEDED( _T2BstrVariant( pszText, &var ) ) &&
            SUCCEEDED( _AddConstraint( pSrchCmd, GetConstraintName( FSBC_FILETYPE ), &var ) ) )
            hr = S_OK;
        LocalFree((HLOCAL)pszText);
    }
    
    return hr;
}

//-------------------------------------------------------------------------//
//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CTypeDlg::RestoreConstraint( const BSTR bstrName, const VARIANT* pValue )
{
    if( IsConstraintName( FSBC_FILETYPE, bstrName ) )
    {
        ASSERT( VT_BSTR == pValue->vt );
        
        USES_CONVERSION;
        lstrcpyn( _szRestoredExt, W2T( pValue->bstrVal ), ARRAYSIZE(_szRestoredExt ) );

        INT_PTR i = _FindExtension( GetDlgItem( IDC_FILE_TYPE ), _szRestoredExt );
        if( i != CB_ERR )
        {
            SendDlgItemMessage( IDC_FILE_TYPE, CB_SETCURSEL, i, 0L );
            *_szRestoredExt = 0;
        }

        return S_OK;
    }
    return E_FAIL;
}

//-------------------------------------------------------------------------//
INT_PTR CTypeDlg::_FindExtension( HWND hwndCombo, TCHAR* pszExt )
{
    INT_PTR i, cnt = ::SendMessage( hwndCombo, CB_GETCOUNT, 0, 0L );
    LPTSTR  pszData;
    BOOL    bAllFileTypes = pszExt && (lstrcmp(TEXT("*.*"), pszExt) == 0);
    TCHAR   szExt[MAX_PATH];

    if( !bAllFileTypes )
    {
        //  Remove wildcard characters.
        LPTSTR pszSrc = pszExt, pszDest = szExt;
        for(;; pszSrc = CharNext( pszSrc ) )
        {
            if( TEXT('*') == *pszSrc )
                pszSrc = CharNext( pszSrc );

            if( (*(pszDest++) = *pszSrc) == 0 )
                break;
        }
        pszExt = szExt;
    }

    if( pszExt && (bAllFileTypes || *pszExt) )
    {
        for( i = 0; i < cnt; i++ )
        {
            pszData = (LPTSTR)::SendMessage( hwndCombo, CB_GETITEMDATA, i, 0L );
            if( bAllFileTypes && (FILEASSOCIATIONSID_ALLFILETYPES == (UINT_PTR)pszData) )
                return i;

            if( pszData != (LPTSTR)FILEASSOCIATIONSID_ALLFILETYPES &&
                pszData != (LPTSTR)CB_ERR && 
                pszData != NULL )
            {
                if( 0 == StrCmpI( pszExt, pszData ) )
                    return i;
            }
        }
    }
    return CB_ERR;
}

//-------------------------------------------------------------------------//
void CTypeDlg::Clear()
{
    //  Assign combo selection to 'all file types':
    HWND hwndCombo = GetDlgItem( IDC_FILE_TYPE );
    for( INT_PTR i=0, cnt = ::SendMessage( hwndCombo, CB_GETCOUNT, 0, 0L );
         i < cnt; i++ )
    {
        if( FILEASSOCIATIONSID_ALLFILETYPES == _GetComboData( hwndCombo, i ) )
        {
            ::SendMessage( hwndCombo, CB_SETCURSEL, i, 0L );
            break;
        }
    }
    *_szRestoredExt = 0;
}

//-------------------------------------------------------------------------//
LRESULT CTypeDlg::OnFileTypeDeleteItem( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    return _DeleteFileAssocComboItem( pnmh );
}



//-------------------------------------------------------------------------//
LRESULT CTypeDlg::OnSize( UINT, WPARAM wParam, LPARAM lParam, BOOL& )
{
    POINTS pts = MAKEPOINTS( lParam );

    _PaintDlg( m_hWnd, _pfsb->GetMetrics() );
    ValidateRect( NULL );

    RECT rc;
    HWND hwndCtl = GetDlgItem( IDC_FILE_TYPE );
    ASSERT( hwndCtl );
    
    ::GetWindowRect( hwndCtl, &rc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    rc.right = pts.x - _pfsb->GetMetrics().CtlMarginX();
    
    ::SetWindowPos( hwndCtl, NULL, 0, 0, 
                    RECTWIDTH(&rc), RECTHEIGHT(&rc),
                    SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );

    return 0L;
}

//-------------------------------------------------------------------------//
DWORD CTypeDlg::FileAssocThreadProc( void* pvParam )
{
    PFSEARCHTHREADSTATE pState = (PFSEARCHTHREADSTATE)pvParam;

    HRESULT hrOleInit = SHCoInitialize();

    if( _PopulateFileAssocCombo( pState->hwndCtl, AddItemNotify, (LPARAM)pvParam ) != E_ABORT )
    {
        ::PostMessage( ::GetParent( pState->hwndCtl ), 
                       WMU_COMBOPOPULATIONCOMPLETE, 
                       (WPARAM)pState->hwndCtl, 0L );
    }

    pState->fComplete = TRUE;
    SHCoUninitialize(hrOleInit);
    return 0;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CTypeDlg::AddItemNotify( ULONG fAction, PCBXITEM pItem, LPARAM lParam )
{
    PFSEARCHTHREADSTATE pState = (PFSEARCHTHREADSTATE)lParam;
    ASSERT( pState );
    ASSERT( pState->hwndCtl );

    //  Do we want to abort combo population thread?
    if( fAction & CBXCB_ADDING && pState->fCancel )
        return E_ABORT;

    //  Set the current selection to 'all file types'
    if( fAction & CBXCB_ADDED )
    {
        BOOL    bAllTypesItem   = (FILEASSOCIATIONSID_ALLFILETYPES == pItem->lParam);
        
        CTypeDlg* pThis = (CTypeDlg*)pState->pvParam;
        ASSERT(pThis);
        
        //  If this item is the one restored from a saved query
        //  override any current selection and select it.
        if( *pThis->_szRestoredExt && !bAllTypesItem && pItem->lParam && 
            0 == lstrcmpi( (LPCTSTR)pItem->lParam, pThis->_szRestoredExt ) )
        {
            ::SendMessage( pState->hwndCtl, CB_SETCURSEL, pItem->iItem, 0L );
            *pThis->_szRestoredExt = 0;
        }
        //  If this item is the default ('all file types') and 
        //  nothing else is selected, select it.
        else if( bAllTypesItem )
        {
            if( CB_ERR == ::SendMessage( pState->hwndCtl, CB_GETCURSEL, 0, 0L ) )
                ::SendMessage( pState->hwndCtl, CB_SETCURSEL, pItem->iItem, 0L );
        }
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
LRESULT CTypeDlg::OnComboPopulationComplete( UINT, WPARAM, LPARAM, BOOL& )
{
    // remove briefcase from type combo because briefcases no longer use this
    // extension (now they store clsid in desktop.ini
    INT_PTR iBfc = _FindExtension( GetDlgItem( IDC_FILE_TYPE ), TEXT(".bfc") );
    if( iBfc != CB_ERR)
    {
        SendDlgItemMessage( IDC_FILE_TYPE, CB_DELETESTRING, (WPARAM)iBfc, 0 );
    }
    
    if( *_szRestoredExt )
    {
        INT_PTR iSel = _FindExtension( GetDlgItem( IDC_FILE_TYPE ), _szRestoredExt );
        if( iSel != CB_ERR )
        {
            SendDlgItemMessage( IDC_FILE_TYPE, CB_SETCURSEL, (WPARAM)iSel, 0L );
            *_szRestoredExt = 0;
        }
    }
        
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CTypeDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled )  
{ 
    _threadState.fCancel = TRUE; 
    bHandled = FALSE; 
    return 0L;
}

//-------------------------------------------------------------------------//
void CTypeDlg::OnWinIniChange()
{
    SendDlgItemMessage( IDC_FILE_TYPE, CB_SETDROPPEDWIDTH,
                        _PixelsForDbu( m_hWnd, MIN_FILEASSOCLIST_WIDTH, TRUE ), 0 );
}


//-------------------------------------------------------------------------//
// CAdvancedDlg impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
LRESULT CAdvancedDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _Attach( m_hWnd );

    Clear();

    return TRUE;  // Let the system set the focus
}

//-------------------------------------------------------------------------//
STDMETHODIMP CAdvancedDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    VARIANT var;
    VariantInit( &var );
    var.vt = VT_BOOL;
    HRESULT hr;

    var.boolVal = IsDlgButtonChecked( IDC_USE_SUBFOLDERS ) ? 1 : 0;
    hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_SEARCHSUBFOLDERS ), &var );
    
    var.boolVal = IsDlgButtonChecked( IDC_USE_CASE ) ? 1 : 0;
    hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_CASE ), &var );

#ifdef WINNT
    var.boolVal = IsDlgButtonChecked( IDC_USE_SLOW_FILES ) ? 1 : 0;
    hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_SLOWFILES ), &var );
#endif // WINNT    
    return S_OK;
}

//-------------------------------------------------------------------------//
//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CAdvancedDlg::RestoreConstraint( const BSTR bstrName, const VARIANT* pValue )
{
    if( IsConstraintName( FSBC_SEARCHSUBFOLDERS, bstrName ) )
    {
        ASSERT( VT_BOOL == pValue->vt || VT_I4 == pValue->vt );
        CheckDlgButton( IDC_USE_SUBFOLDERS, pValue->lVal );
        return S_FALSE;    // this is a default. don't force open the subdialog.
    }

    if( IsConstraintName( FSBC_CASE, bstrName ) )
    {
        ASSERT( VT_BOOL == pValue->vt || VT_I4 == pValue->vt );
        CheckDlgButton( IDC_USE_CASE, pValue->lVal );
        return pValue->lVal ? S_OK : S_FALSE;
    }
#ifdef WINNT
    if( IsConstraintName( FSBC_SLOWFILES, bstrName ) )
    {
        ASSERT( VT_BOOL == pValue->vt || VT_I4 == pValue->vt );
        CheckDlgButton( IDC_USE_SLOW_FILES, pValue->lVal );
        return pValue->lVal ? S_OK : S_FALSE;
    }
#endif // WINNT
    return E_FAIL;
}

//-------------------------------------------------------------------------//
void CAdvancedDlg::Clear()
{
    CheckDlgButton( IDC_USE_SUBFOLDERS, 1 );
    CheckDlgButton( IDC_USE_CASE, 0 );
#ifdef WINNT
    CheckDlgButton( IDC_USE_SLOW_FILES, 0 );
#endif // WINNT
}

//-------------------------------------------------------------------------//
// COptionsDlg impl
//-------------------------------------------------------------------------//
#ifdef WINNT
#define OPTIONSDLG_BOTTOMMOST	IDC_USE_SLOW_FILES
#else 
#define OPTIONSDLG_BOTTOMMOST	IDC_USE_CASE
#endif

//-------------------------------------------------------------------------//
COptionsDlg::COptionsDlg( CFileSearchBand* pfsb )
    :   CSubDlg( pfsb ),
        _dlgDate( pfsb ),
        _dlgSize( pfsb ),
        _dlgType( pfsb ),
        _dlgAdvanced( pfsb ),
        _nCIStatusText(0)
{
    ZeroMemory( _subdlgs, sizeof(_subdlgs) );
    #define SUBDLG_ENTRY( idx, idCheck, dlg )  \
        { _subdlgs[idx].nIDCheck = idCheck; _subdlgs[idx].pDlg = &dlg; }

    SUBDLG_ENTRY( SUBDLG_DATE, IDC_USE_DATE, _dlgDate );
    SUBDLG_ENTRY( SUBDLG_TYPE, IDC_USE_TYPE, _dlgType );
    SUBDLG_ENTRY( SUBDLG_SIZE, IDC_USE_SIZE, _dlgSize );
    SUBDLG_ENTRY( SUBDLG_ADVANCED, IDC_USE_ADVANCED, _dlgAdvanced );
}

//-------------------------------------------------------------------------//
LRESULT COptionsDlg::OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    _Attach( m_hWnd );
    _dlgDate.SetBandDlg( _pBandDlg );
    _dlgSize.SetBandDlg( _pBandDlg );
    _dlgType.SetBandDlg( _pBandDlg );
    _dlgAdvanced.SetBandDlg( _pBandDlg );

    //  Gather some metrics from the fresh dialog template...
    CMetrics&   metrics = _pfsb->GetMetrics();
    RECT        rc[3];

    ZeroMemory( rc, sizeof(rc) );
    ASSERT( IsWindow( GetDlgItem( IDC_USE_DATE ) ) );
    ASSERT( IsWindow( GetDlgItem( IDC_USE_TYPE ) ) );
    ASSERT( IsWindow( GetDlgItem( IDC_USE_ADVANCED ) ) );

    ::GetWindowRect( GetDlgItem( IDC_USE_DATE ), rc + 0 );
    ::GetWindowRect( GetDlgItem( IDC_USE_TYPE ), rc + 1 );
    ::GetWindowRect( GetDlgItem( IDC_USE_ADVANCED ), rc + 2 );
    for(int i =0; i < ARRAYSIZE(rc); i++)
    {
        // MapWindowPoints is mirroring aware only if you pass two points
        ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)(&rc[i]), POINTSPERRECT );
    }    

    metrics.ExpandOrigin().y = rc[0].top;
    metrics.CheckBoxRect()   = rc[2];
    OffsetRect( &metrics.CheckBoxRect(), -rc[0].left, -rc[0].top );
    
    //  Create subdialogs and collect native sizes.
    if( _dlgDate.Create( m_hWnd ) )
        _GetWindowSize( _dlgDate, &_subdlgs[SUBDLG_DATE].sizeDlg );

    if( _dlgSize.Create( m_hWnd ) )
        _GetWindowSize( _dlgSize, &_subdlgs[SUBDLG_SIZE].sizeDlg );

    if( _dlgType.Create( m_hWnd ) )
        _GetWindowSize( _dlgType, &_subdlgs[SUBDLG_TYPE].sizeDlg );

    if( _dlgAdvanced.Create( m_hWnd ) )
        _GetWindowSize( _dlgAdvanced, &_subdlgs[SUBDLG_ADVANCED].sizeDlg );

#ifdef WINNT
    //  Create index server link window    
    POINT pt;
    ZeroMemory( &pt, sizeof(pt) );
    HWND hwndCI = _CreateLinkWindow( m_hWnd, IDC_INDEX_SERVER, 
                                     pt, IDS_FSEARCH_CI_DISABLED_LINK );
    UpdateSearchCmdStateUI();
#endif WINNT

    //  Layout controls
    LayoutControls();

    return TRUE;
}

//-------------------------------------------------------------------------//
void COptionsDlg::OnWinIniChange()
{
    CSubDlg::OnWinIniChange();
    for( int i = 0; i< SUBDLG_Count; i++ )
        _subdlgs[i].pDlg->OnWinIniChange();
}

//-------------------------------------------------------------------------//
BOOL _SaveCheck( HWND hwnd, UINT nIDCheck, HKEY hkey, LPCTSTR pszValue )
{
    DWORD dwData = IsDlgButtonChecked( hwnd, nIDCheck );
    return ERROR_SUCCESS == RegSetValueEx( hkey, pszValue, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData) );
}

//-------------------------------------------------------------------------//
BOOL _LoadCheck( HWND hwnd, UINT nIDCheck, HKEY hkey, LPCTSTR pszValue )
{
    DWORD dwData = IsDlgButtonChecked( hwnd, nIDCheck );
    DWORD cbData = sizeof(dwData);
    DWORD dwType;

    if( ERROR_SUCCESS == RegQueryValueEx( hkey, pszValue, 0, &dwType, (LPBYTE)&dwData, &cbData ) )
    {
        CheckDlgButton( hwnd, nIDCheck, dwData );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void COptionsDlg::LoadSaveUIState( UINT nIDCtl, BOOL bSave ) 
{
#ifdef _LOADSAVE_SUBDLG_UI__
    if( 0 == nIDCtl )   // do all.
    {
        LoadSaveUIState( IDC_USE_DATE,     bSave );
        LoadSaveUIState( IDC_USE_TYPE,     bSave );
        LoadSaveUIState( IDC_USE_SIZE,     bSave );
        LoadSaveUIState( IDC_USE_ADVANCED, bSave );
        LayoutControls();
        SizeToFit( FALSE );
        return;
    }

    HKEY hkey;
    LPCTSTR pszVal = NULL;

    switch( nIDCtl )
    {
        case IDC_USE_DATE:      pszVal = TEXT("UseDate"); break;
        case IDC_USE_TYPE:      pszVal = TEXT("UseType"); break;
        case IDC_USE_SIZE:      pszVal = TEXT("UseSize"); break;
        case IDC_USE_ADVANCED:  pszVal = TEXT("UseAdvanced"); break;
    }

    ASSERT( pszVal != NULL ); // invalid ctl ID!
    if( NULL == pszVal || NULL == (hkey = _pfsb->GetBandRegKey(bSave)) )
        return;

    if( bSave )
        _SaveCheck( m_hWnd, nIDCtl, hkey, pszVal );
    else 
        _LoadCheck( m_hWnd, nIDCtl, hkey, pszVal );

    RegCloseKey( hkey );
#endif _LOADSAVE_SUBDLG_UI__
}

//-------------------------------------------------------------------------//
LRESULT COptionsDlg::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    POINTS pts = MAKEPOINTS( lParam );

    _PaintDlg( m_hWnd, _pfsb->GetMetrics() );
    LayoutControls( pts.x, pts.y );
    return 0L;
}

//-------------------------------------------------------------------------//
void COptionsDlg::LayoutControls( int cx, int cy )
{
    if( cx < 0 || cy < 0 )
    {
        RECT rc;
        GetClientRect( &rc );
        cx = RECTWIDTH( &rc );
        cy = RECTHEIGHT( &rc );
    }

    HDWP hdwp;
    if( (hdwp = BeginDeferWindowPos( 1 + (SUBDLG_Count * 2) )) != NULL )
    {
        CMetrics& metrics = _pfsb->GetMetrics();
        POINT ptOrigin = metrics.ExpandOrigin();

        //  For each checkbox and associated subdialog...
        for( int i = 0; i< SUBDLG_Count; i++ )
        {
            //  Calculate checkbox position
            HWND  hwndCheck = GetDlgItem( _subdlgs[i].nIDCheck );
            ASSERT( hwndCheck );
    
            SetRect( &_subdlgs[i].rcCheck, 
                     ptOrigin.x, ptOrigin.y,
                     ptOrigin.x + RECTWIDTH( &metrics.CheckBoxRect() ),
                     ptOrigin.y + RECTHEIGHT( &metrics.CheckBoxRect() ) );

            //  Calculate subdialog position
            ULONG dwDlgFlags = SWP_NOACTIVATE;

            if( IsDlgButtonChecked( _subdlgs[i].nIDCheck ) )
            {
                //  position the checkbox's dialog immediately below.
                SetRect( &_subdlgs[i].rcDlg, 
                         _subdlgs[i].rcCheck.left, _subdlgs[i].rcCheck.bottom,
                         cx - 1, _subdlgs[i].rcCheck.bottom  + _subdlgs[i].sizeDlg.cy );
                dwDlgFlags |= SWP_SHOWWINDOW;

                ptOrigin.y = _subdlgs[i].rcDlg.bottom + metrics.TightMarginY();
            }
            else
            {
                ptOrigin.y = _subdlgs[i].rcCheck.bottom + metrics.TightMarginY();        
                dwDlgFlags |= SWP_HIDEWINDOW;
            }

            //  Reposition the pair
            DeferWindowPos( hdwp, _subdlgs[i].pDlg->Hwnd(), hwndCheck, 
                            _subdlgs[i].rcDlg.left,
                            _subdlgs[i].rcDlg.top,
                            RECTWIDTH( &_subdlgs[i].rcDlg ),
                            RECTHEIGHT( &_subdlgs[i].rcDlg ),
                            dwDlgFlags );

            DeferWindowPos( hdwp, hwndCheck, NULL, 
                            _subdlgs[i].rcCheck.left,
                            _subdlgs[i].rcCheck.top,
                            RECTWIDTH( &_subdlgs[i].rcCheck ),
                            RECTHEIGHT( &_subdlgs[i].rcCheck ),
                            SWP_NOZORDER|SWP_NOACTIVATE );
        }

#ifdef WINNT
        _LayoutLinkWindow( m_hWnd, metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                           ptOrigin.y, IDC_INDEX_SERVER );
#endif WINNT

        EndDeferWindowPos( hdwp );
    }
}

//-------------------------------------------------------------------------//
//  Assigns focus to the options dialog.   This cannot be done by
//  simply setting focus to the options dialog, which is a child
//  of another dialog; USER will simply assign focus to the parent dialog.
//  So we need to explicitly set focus to our first child.
void COptionsDlg::TakeFocus()
{
    
    for( HWND hwndCtl = GetWindow( GW_CHILD );
         IsWindow( hwndCtl );
         hwndCtl = ::GetWindow( hwndCtl, GW_HWNDNEXT ) )
    {
        ULONG dwStyle = ::GetWindowLong( hwndCtl, GWL_STYLE );
        if( dwStyle & WS_TABSTOP )
        {
            ::SetFocus( hwndCtl );
            break;
        }
    }
}

//-------------------------------------------------------------------------//
LONG COptionsDlg::QueryHeight( LONG cx /* proposed width */, LONG cy /* proposed height */)
{
    HWND hwndBottommost = GetDlgItem( IDC_INDEX_SERVER );
    RECT rcThis, rcBottommost;

    //  Retrieve the current height of the bottommost link window.
    GetWindowRect( &rcThis );
    ::GetWindowRect( hwndBottommost, &rcBottommost );
    ::MapWindowPoints( HWND_DESKTOP, GetParent(), (LPPOINT)&rcThis,       POINTSPERRECT );
    ::MapWindowPoints( HWND_DESKTOP, GetParent(), (LPPOINT)&rcBottommost, POINTSPERRECT );

    //  If, at the specified width, we compute a height for the bottommost 
    //  linkwindow that is different from its current height (e.g, due to word wrap),
    //  we'll compute a new window rect that will 
    LONG cyBottommost = (LONG)SendDlgItemMessage( IDC_INDEX_SERVER, LWM_GETIDEALHEIGHT, 
                                                  cx - (_pfsb->GetMetrics().CtlMarginX() * 2), 0L );
    
    if( cyBottommost > 0 && cyBottommost != RECTHEIGHT(&rcBottommost) )
        rcThis.bottom = rcBottommost.top + cyBottommost + _pfsb->GetMetrics().TightMarginY();

    return RECTHEIGHT( &rcThis );
}

//-------------------------------------------------------------------------//
BOOL COptionsDlg::GetMinSize( LPSIZE pSize ) const
{
    pSize->cx = pSize->cy = 0;

    HWND hwndBottom = GetDlgItem( IDC_INDEX_SERVER );

    if( !IsWindow( hwndBottom ) )
    {
        hwndBottom = IsDlgButtonChecked( IDC_USE_ADVANCED ) ?
            _dlgAdvanced.GetDlgItem( OPTIONSDLG_BOTTOMMOST ) :
            GetDlgItem( IDC_USE_ADVANCED );
    }

    if( !IsWindow( hwndBottom ) )
        return FALSE;

    RECT rcBottom;
    ::GetWindowRect( hwndBottom, &rcBottom );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcBottom, POINTSPERRECT );

    pSize->cx = 0;
    pSize->cy = rcBottom.bottom;

    return TRUE;
}

//-------------------------------------------------------------------------//
void COptionsDlg::UpdateSearchCmdStateUI( DISPID dispid )
{
#ifdef WINNT
    BOOL fCiRunning  = FALSE, fCiIndexed = FALSE, fCiPermission = FALSE;
    UINT nStatusText = IDS_FSEARCH_CI_DISABLED_LINK;

    HRESULT hr = GetCIStatus( &fCiRunning, &fCiIndexed, &fCiPermission );

    if( fCiRunning )
    {
        if( fCiPermission )
            //  we have permission to distinguish between ready and busy
            nStatusText = fCiIndexed ? IDS_FSEARCH_CI_READY_LINK : IDS_FSEARCH_CI_BUSY_LINK;
        else
            //  no permission to distinguish between ready and busy; we'll
            //  just say it's enabled.
            nStatusText = IDS_FSEARCH_CI_ENABLED_LINK;
    }

    TCHAR szCaption[MAX_PATH];
    if( nStatusText != _nCIStatusText &&
        EVAL( LoadString( HINST_THISDLL, nStatusText, szCaption, ARRAYSIZE(szCaption) ) ) )
    {
        SetDlgItemText( IDC_INDEX_SERVER, szCaption );
        _nCIStatusText = nStatusText;
        LayoutControls();
        SizeToFit( FALSE );
    }

#endif WINNT
}

//-------------------------------------------------------------------------//
STDMETHODIMP COptionsDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    HRESULT hrRet = S_OK;
    //  have subdialogs add their constraints
    for( int i = 0; i< SUBDLG_Count; i++ )
    {
        if( ::IsWindowVisible( _subdlgs[i].pDlg->Hwnd() ) )
        {
            HRESULT hr = _subdlgs[i].pDlg->AddConstraints( pSrchCmd );
            if( FAILED(hr) )
                hrRet = hr;
        }       
    }
    return hrRet;
}

//-------------------------------------------------------------------------//
STDMETHODIMP COptionsDlg::RestoreConstraint( const BSTR bstrName, const VARIANT* pValue )
{
    //  Try subordinate dialogs.
    for( int i = 0; i < SUBDLG_Count; i++ )
    {
        HRESULT hr = _subdlgs[i].pDlg->RestoreConstraint( bstrName, pValue );

        if( S_OK == hr )  // open the dialog
        {
            CheckDlgButton( _subdlgs[i].nIDCheck, TRUE );
            LayoutControls();
            SizeToFit();
        }

        //  if success, we're done.
        if( SUCCEEDED( hr ) )
            return hr;

        //  otherwise, try next subdialog.
    }
    return E_FAIL;
}

//-------------------------------------------------------------------------//
STDMETHODIMP COptionsDlg::TranslateAccelerator( LPMSG lpmsg )
{
    if( S_OK == CSubDlg::TranslateAccelerator( lpmsg ) )
        return S_OK;

    //  Query subdialogs
    if( _dlgDate.IsChild( lpmsg->hwnd ) &&
        S_OK == _dlgDate.TranslateAccelerator( lpmsg ) )
        return S_OK;

    if( _dlgType.IsChild( lpmsg->hwnd ) &&
        S_OK == _dlgType.TranslateAccelerator( lpmsg ) )
        return S_OK;

    if( _dlgSize.IsChild( lpmsg->hwnd ) &&
        S_OK == _dlgSize.TranslateAccelerator( lpmsg ) )
        return S_OK;

    if( _dlgAdvanced.IsChild( lpmsg->hwnd ) &&
        S_OK == _dlgAdvanced.TranslateAccelerator( lpmsg ) )
        return S_OK;

    return _pfsb->IsDlgMessage( Hwnd(), lpmsg );
}

//-------------------------------------------------------------------------//
BOOL COptionsDlg::Validate()
{
    //  have subdialogs do validatation
    for( int i = 0; i< SUBDLG_Count; i++ )
    {
        if( ::IsWindowVisible( _subdlgs[i].pDlg->Hwnd() ) )
            if( !_subdlgs[i].pDlg->Validate() )
                return FALSE;
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
void COptionsDlg::Clear()
{
    //  have subdialogs clear themselves.
    for( int i = 0; i< SUBDLG_Count; i++ )
    {
        _subdlgs[i].pDlg->Clear();
        CheckDlgButton( _subdlgs[i].nIDCheck, FALSE );
    }
    LayoutControls();
    SizeToFit();
}

//-------------------------------------------------------------------------//
LRESULT COptionsDlg::OnBtnClick( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
#ifdef DEBUG
    //  Is this a sub-dialog expansion/contraction?
    BOOL bIsSubDlgBtn = FALSE;
    for( int i = 0; i< SUBDLG_Count && !bIsSubDlgBtn; i++ )
    {
        if( nID == _subdlgs[i].nIDCheck )
            bIsSubDlgBtn = TRUE;
    }
    ASSERT( bIsSubDlgBtn );
#endif DEBUG
    
    LoadSaveUIState( nID, TRUE ); // persist it.

    LayoutControls();
    SizeToFit( !IsDlgButtonChecked( nID ) );
        //  don't need to scroll the band if we've expanded a subdialog,
        //  but we do if we've contracted one.

    return 0L;
}

//-------------------------------------------------------------------------//
void COptionsDlg::SizeToFit( BOOL bScrollBand )
{
    SIZE size;
    GetMinSize( &size );
    size.cy += _pfsb->GetMetrics().TightMarginY();
    SetWindowPos( NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE );

    ULONG dwLayoutFlags = BLF_ALL;
    if( !bScrollBand )
        dwLayoutFlags &= ~BLF_SCROLLWINDOW;    
    
    ::SendMessage( GetParent(), WMU_UPDATELAYOUT, dwLayoutFlags, 0L );
}

//-------------------------------------------------------------------------//
LRESULT COptionsDlg::OnIndexServerClick( int idCtl, LPNMHDR pnmh, BOOL& )
{
    BOOL    fCiRunning, fCiIndexed, fCiPermission = FALSE;
    
    HRESULT hr = GetCIStatus( &fCiRunning, &fCiIndexed, &fCiPermission );
    if( SUCCEEDED(hr) && fCiPermission )
    {
        //  CI is idle or not runnning.  Show status dialog.
        if( IDOK == CCISettingsDlg_DoModal( GetDlgItem( IDC_INDEX_SERVER ) ) )
        {
            // reflect any state change in UI.
            ::PostMessage( GetParent(), WMU_STATECHANGE, 0, 0L ); 
        }
    }
    else
    {
        //  No permission? display CI help.
        _IndexServiceHelp( NULL );
    }
        
    return 0L;
}

//-------------------------------------------------------------------------//
// CBandDlg impl
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CBandDlg::CBandDlg( CFileSearchBand* pfsb )
    :   _pfsb(pfsb), _hwnd(NULL), _hwndLastFocus(NULL)
{
    VariantInit( &_varScope0 );
    VariantInit( &_varQueryFile0 );
}

//-------------------------------------------------------------------------//
CBandDlg::~CBandDlg()
{
    VariantClear( &_varScope0 );
    VariantClear( &_varQueryFile0 );
}

//-------------------------------------------------------------------------//
STDMETHODIMP CBandDlg::TranslateAccelerator( LPMSG lpmsg )
{
    if( WM_KEYDOWN == lpmsg->message || WM_KEYUP == lpmsg->message )
    {
        IAutoComplete2* pac2;
        if( GetAutoCompleteObjectForWindow( lpmsg->hwnd, &pac2 ) )
        {
            if( S_OK == _TestAutoCompleteDropDownState( pac2, ACDD_VISIBLE ) )
            {
                TranslateMessage( lpmsg );
                DispatchMessage( lpmsg );
                pac2->Release();
                return S_OK;
            }
            pac2->Release();
        }
    }
    
    //  Check for Ctrl+Nav Key:
    if( _pfsb->IsKeyboardScroll( lpmsg ) )
        return S_OK;
    return S_FALSE;
}

//-------------------------------------------------------------------------//
void CBandDlg::SetDefaultFocus()
{
    HWND hwndFirst = GetFirstTabItem();
    if( IsWindow( hwndFirst ) )
        SetFocus( hwndFirst );
}

//-------------------------------------------------------------------------//
void CBandDlg::RememberFocus( HWND hwndFocus )
{
    if( !IsWindow( hwndFocus ) )
    {
        _hwndLastFocus = NULL;
        hwndFocus = GetFocus();
    }

    if( IsChild( _hwnd, hwndFocus ) )
        _hwndLastFocus = hwndFocus;
}

//-------------------------------------------------------------------------//
BOOL CBandDlg::RestoreFocus()
{
    if( IsWindow( _hwndLastFocus ) )
    {
        if( IsWindowVisible( _hwndLastFocus ) && IsWindowEnabled( _hwndLastFocus ) )
        {
            SetFocus( _hwndLastFocus );
            return TRUE;
        }
    }
    else
        _hwndLastFocus = NULL;
    
    return FALSE;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnChildSetFocusCmd( WORD, WORD, HWND hwndCtl, BOOL& bHandled )
{
    _EnsureVisible( _hwnd, hwndCtl, _pfsb );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnChildSetFocusNotify( int, LPNMHDR pnmh, BOOL&)
{
    _EnsureVisible( _hwnd, pnmh->hwndFrom, _pfsb );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnChildKillFocusCmd( WORD, WORD, HWND hwndCtl, BOOL& )
{
    _hwndLastFocus = hwndCtl;
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnChildKillFocusNotify( int, LPNMHDR pnmh, BOOL& )
{
    _hwndLastFocus = pnmh->hwndFrom;
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnComboExEndEdit( int, LPNMHDR pnmh, BOOL& )
{
    if( CBEN_ENDEDIT == pnmh->code )
    {
        if( CBENF_KILLFOCUS == ((NMCBEENDEDIT*)pnmh)->iWhy )
            _hwndLastFocus = pnmh->hwndFrom;
    }
    return 0L;
}

//-------------------------------------------------------------------------//
void CBandDlg::WndPosChanging( HWND hwndOC, LPWINDOWPOS pwp )
{
    SIZE sizeMin;
    if( 0 == (pwp->flags & SWP_NOSIZE) && GetMinSize( hwndOC, &sizeMin ) )
    {
        if( pwp->cx < sizeMin.cx )
            pwp->cx = sizeMin.cx;

        if( pwp->cy < sizeMin.cy )
            pwp->cy = sizeMin.cy;
    }        
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnSize( UINT, WPARAM wParam, LPARAM lParam, BOOL& )
{
    POINTS pts = MAKEPOINTS( lParam );

    LayoutControls( pts.x, pts.y );
    return 0L;
}

//-------------------------------------------------------------------------//
void CBandDlg::LayoutControls( int cx, int cy )
{
    if( cx < 0 || cy < 0 )
    {
        RECT rc;
        GetClientRect( _hwnd, &rc );
        cx = RECTWIDTH(&rc);
        cy = RECTHEIGHT(&rc);
    }
    _LayoutCaption( GetIconID(), GetCaptionID(), GetCaptionDivID(), cx );
}

//-------------------------------------------------------------------------//
BOOL CBandDlg::GetIdealSize( HWND hwndOC, LPSIZE psize ) const
{
    ASSERT( psize );
    psize->cx = psize->cy = 0;

    if( !IsWindow( Hwnd() ) )
        return FALSE;

    SIZE sizeMin;
    if( GetMinSize( hwndOC, &sizeMin ) )
    {
        RECT rcClient;
        ::GetClientRect( hwndOC, &rcClient );

        psize->cx = (RECTWIDTH(&rcClient) < sizeMin.cx) ? sizeMin.cx : RECTWIDTH(&rcClient);
        psize->cy = sizeMin.cy;            
        return TRUE;
    }
    
    return FALSE;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnPaint( UINT, WPARAM, LPARAM, BOOL& )
{
    HDC         hdc;
    PAINTSTRUCT ps;

    //  Just going to call BeginPaint and EndPaint.  All
    //  painting done in WM_ERASEBKGND handler to avoid flicker.
    if( (hdc = BeginPaint( _hwnd, &ps )) != NULL )
    {
        EndPaint( _hwnd, &ps );
    }
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnEraseBkgnd( UINT, WPARAM wParam, LPARAM, BOOL& )
{
    ASSERT( IsWindow( _hwnd ) ); // was _Attach() called, e.g. from WM_INITDIALOG?
    _PaintDlg( _hwnd, _pfsb->GetMetrics(), (HDC)wParam );
    ValidateRect( _hwnd, NULL );
    return TRUE;   
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnCtlColorStatic(UINT, WPARAM wParam, LPARAM lParam, BOOL& )
{
    SetTextColor( (HDC)wParam, _pfsb->GetMetrics().TextColor() );
    SetBkColor( (HDC)wParam, _pfsb->GetMetrics().BkgndColor() );
    return (LRESULT)_pfsb->GetMetrics().BkgndBrush();
}

//-------------------------------------------------------------------------//
//  Hack method to remove turds left after showing band toolbar.
//  Methinks this is a USER issue. [scotthan]
void CBandDlg::RemoveToolbarTurds( int cyOffset )
{
    HWND hwndCtl;
    RECT rcUpdate;
    GetClientRect( _hwnd, &rcUpdate );
    
    if( (hwndCtl = GetDlgItem( _hwnd, GetCaptionDivID() )) != NULL )
    {
        RECT rc;
        GetWindowRect( hwndCtl, &rc );
        MapWindowPoints( HWND_DESKTOP, _hwnd, (LPPOINT)&rc, POINTSPERRECT );
        rcUpdate.bottom = rc.bottom;
        OffsetRect( &rcUpdate, 0, cyOffset );

        InvalidateRect( _hwnd, &rcUpdate, TRUE );
        InvalidateRect( hwndCtl, NULL, TRUE );
        UpdateWindow( hwndCtl );
    }
    if( (hwndCtl = GetDlgItem( _hwnd, GetIconID() )) != NULL )
    {
        InvalidateRect( hwndCtl, NULL, TRUE );
        UpdateWindow( hwndCtl );
    }

    if( (hwndCtl = GetDlgItem( _hwnd, GetCaptionID() )) != NULL )
    {
        InvalidateRect( hwndCtl, NULL, TRUE );
        UpdateWindow( hwndCtl );
    }

    UpdateWindow( _hwnd );
}

//-------------------------------------------------------------------------//
void CBandDlg::_BeautifyCaption( UINT nIDCaption, UINT nIDIcon, UINT nIDIconResource )
{
    //  Do some cosmetic and initialization stuff
    HFONT hf = _pfsb->GetMetrics().BoldFont( _hwnd );
    if( hf )
        SendDlgItemMessage( _hwnd, nIDCaption, WM_SETFONT, (WPARAM)hf, 0L );

    if( nIDIcon && nIDIconResource )
    {
        HICON hiconCaption = _pfsb->GetMetrics().CaptionIcon( nIDIconResource );
        if( hiconCaption )
            SendDlgItemMessage( _hwnd, nIDIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hiconCaption );
    }
}

//-------------------------------------------------------------------------//
void CBandDlg::_LayoutCaption( UINT nIDCaption, UINT nIDIcon, UINT nIDDiv, LONG cxDlg )
{
    RECT rcIcon, rcCaption;
    LONG cxMargin = _pfsb->GetMetrics().CtlMarginX();

    GetWindowRect( GetDlgItem( _hwnd, nIDIcon ), &rcIcon );
    GetWindowRect( GetDlgItem( _hwnd, nIDCaption ), &rcCaption );
    MapWindowPoints( HWND_DESKTOP, _hwnd, (LPPOINT)&rcIcon, POINTSPERRECT );
    MapWindowPoints( HWND_DESKTOP, _hwnd, (LPPOINT)&rcCaption, POINTSPERRECT );

    int nTop = max( rcIcon.bottom, rcCaption.bottom ) + _PixelsForDbu( _hwnd, 1, FALSE );

    SetWindowPos( GetDlgItem( _hwnd, nIDDiv ), GetDlgItem( _hwnd, nIDCaption ),
                  cxMargin, nTop, cxDlg - (cxMargin * 2), 2, SWP_NOACTIVATE );              
}


//-------------------------------------------------------------------------//
void CBandDlg::_LayoutSearchLinks( UINT nIDCaption, UINT nIDDiv, BOOL bShowDiv, LONG left, LONG right, LONG yMargin, 
                                   LONG& yStart, const int rgLinkIDs[], LONG cLinkIDs )
{
    //  Position divider
    if( bShowDiv != 0 )
    {
        RECT rcDiv;
        SetRect( &rcDiv, left, yStart, right, yStart + 1 );
        SetWindowPos( GetDlgItem( _hwnd, nIDDiv ), GetDlgItem( _hwnd, nIDCaption ),
                      rcDiv.left, rcDiv.top, RECTWIDTH(&rcDiv), RECTHEIGHT(&rcDiv),
                      SWP_NOACTIVATE|SWP_SHOWWINDOW );

        yStart += yMargin;
    }
    else
        ShowWindow( GetDlgItem( _hwnd, nIDDiv ), SW_HIDE );

    //  Position caption
    RECT rcCaption;
    GetWindowRect( GetDlgItem( _hwnd, nIDCaption ), &rcCaption );
    MapWindowPoints( HWND_DESKTOP, _hwnd, (LPPOINT)&rcCaption, POINTSPERRECT );
    OffsetRect( &rcCaption, left - rcCaption.left, yStart - rcCaption.top );
    SetWindowPos( GetDlgItem( _hwnd, nIDCaption ), NULL, 
                  left, yStart, 0,0,
                  SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE );
    yStart += RECTHEIGHT(&rcCaption) + yMargin;

    //  Position links
    _LayoutLinkWindows( _hwnd, left, right, yMargin, yStart, rgLinkIDs, cLinkIDs );
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnEditChange( WORD, WORD, HWND, BOOL& )
{
    _pfsb->SetDirty();
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CBandDlg::OnSearchLink( int nID, LPNMHDR, BOOL&)
{
    ASSERT( _pfsb );

    _pfsb->StopSearch();
    switch( nID )
    {
        case IDC_SEARCHLINK_FILES:
            _pfsb->FindFilesOrFolders( FALSE, TRUE );
            break;

        case IDC_SEARCHLINK_COMPUTERS:
            _pfsb->FindComputer( FALSE, TRUE );
            break;

        case IDC_SEARCHLINK_PRINTERS:
            _pfsb->FindPrinter( FALSE, TRUE );
            break;

        case IDC_SEARCHLINK_PEOPLE:
            _pfsb->FindPeople( FALSE, TRUE );
            break;

        case IDC_SEARCHLINK_INTERNET:
            _pfsb->FindOnWeb( FALSE, TRUE );
            break;
    }
    return 0L;
}

//-------------------------------------------------------------------------//
//  Invoked when a client calls IFileSearchBand::SetSearchParameters() 
HRESULT CBandDlg::SetScope( IN VARIANT* pvarScope, BOOL bTrack )
{
    VariantClear( &_varScope0 );
    
    //  cache the scope
    if( pvarScope )
    {
        VariantCopy( &_varScope0, pvarScope );
        VariantClear( pvarScope );
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
HRESULT CBandDlg::GetScope( OUT VARIANT* pvarScope )
{ 
    //  retrieve the scope
    if( !pvarScope )
        return E_INVALIDARG;
    VariantCopy( pvarScope, &_varScope0 );

    return VT_EMPTY == _varScope0.vt ? S_FALSE : S_OK;
}

//-------------------------------------------------------------------------//
HRESULT CBandDlg::SetQueryFile( IN VARIANT* pvarFile )
{
    //  cache the filename of the query to restore.
    VariantClear( &_varQueryFile0 );
    if( pvarFile )
        VariantCopy( &_varQueryFile0, pvarFile );

    return S_OK;
}

//-------------------------------------------------------------------------//
HRESULT CBandDlg::GetQueryFile( OUT VARIANT* pvarFile )
{
    //  retrieve the filename of the query to restore.
    if( !pvarFile )
        return E_INVALIDARG;
    VariantCopy( pvarFile, &_varQueryFile0 );

    return VT_EMPTY == _varQueryFile0.vt ? S_FALSE : S_OK;
}

//-------------------------------------------------------------------------//
// CFindFilesDlg impl
//-------------------------------------------------------------------------//
#define FSEARCHMAIN_TABFIRST      IDC_FILESPEC
#define FSEARCHMAIN_TABLAST       IDC_SEARCHLINK_INTERNET
#define FSEARCHMAIN_BOTTOMMOST    IDC_SEARCHLINK_INTERNET // bottom-most control
#define FSEARCHMAIN_RIGHTMOST     IDC_SEARCH_STOP         // right-most control
#define UISTATETIMER              1
#define UISTATETIMER_DELAY        4000

//-------------------------------------------------------------------------//
CFindFilesDlg::CFindFilesDlg( CFileSearchBand* pfsb )
    :   CSearchCmdDlg( pfsb ),
        _dlgOptions( pfsb ),
        _iCurNamespace( CB_ERR ),
        _hNamespaceThread( INVALID_HANDLE_VALUE ),
        _bScoped(FALSE),
        _fTrackScope(TRACKSCOPE_SPECIFIC),
        _fDisplayOptions(FALSE),
        _fDebuted(FALSE),
        _dwWarningFlags(DFW_DEFAULT),
        _dwRunOnceWarningFlags(DFW_DEFAULT),
        _fAdHocNamespace(FALSE),
        _pacGrepText(NULL),
        _pmruGrepText(NULL),
        _pacFileSpec(NULL),
        _pmruFileSpec(NULL)
{
    //  initialized scopes are blank
    *_szInitialPath = *_szInitialNamespace = *_szCurrentPath = *_szLocalDrives = 0;
}

//-------------------------------------------------------------------------//
CFindFilesDlg::~CFindFilesDlg()
{
    ATOMICRELEASE(_pacGrepText);
    ATOMICRELEASE(_pmruGrepText);
    ATOMICRELEASE(_pacFileSpec);
    ATOMICRELEASE(_pmruFileSpec);

    //  Wait for thread completion
    DWORD dwWait = WaitForThreadCompletion( _hNamespaceThread );
    ASSERT( WAIT_TIMEOUT != dwWait );

    if( WAIT_TIMEOUT == dwWait )
    {
        ASSERT( FALSE ); // namespace combo population thread is hung.
    }
    CloseHandle(_hNamespaceThread);
}

//-------------------------------------------------------------------------//
//  Scope to a default namespace.
BOOL CFindFilesDlg::SetDefaultScope()
{
    USES_CONVERSION;
    BOOL bScoped = FALSE;

    ASSERT( _pfsb->BandSite() );

    //  If we've already assigned a scope, bail early
    if( _bScoped ) 
        return TRUE;

    //  Try establiblishing the preassigned (_szInitialXXX) scope:
    if( !(bScoped = _SetPreassignedScope()) )
        //  Try setting scope to the current shell folder of the active view...
        if( !(bScoped = _SetFolderScope()) )
            //  set it to the hard-coded shell default folder
            bScoped = _SetLocalDefaultScope();

    return bScoped;
}

//-------------------------------------------------------------------------//
//  Assignes the namespace control to the preassigned scope saved in
//  _szInitialNamespace/_szInitialPath
BOOL CFindFilesDlg::_SetPreassignedScope()
{
    BOOL bScoped = FALSE;
    if( *_szInitialNamespace || *_szInitialPath )
        bScoped = AssignNamespace( _szInitialNamespace, _szInitialPath, FALSE );

    return bScoped;
}

//-------------------------------------------------------------------------//
//  Scope to the namespace of the current shell folder view
BOOL CFindFilesDlg::_SetFolderScope()
{
    USES_CONVERSION;
    BOOL    bScoped = FALSE;
    ASSERT( _pfsb->BandSite() );

    if( SUCCEEDED( _GetCurrentPathAndNamespace(  _pfsb->BandSite(), 
                                                 _szInitialPath, ARRAYSIZE(_szInitialPath),  
                                                 _szInitialNamespace, ARRAYSIZE(_szInitialNamespace) ) ) )
    {
        if( *_szInitialNamespace || *_szInitialPath )
        {
            //  if we're tracking the scope loosely...
            if( (TRACKSCOPE_GENERAL == _fTrackScope) && _IsPathLocalHarddrive( _szInitialPath ) )
            {
                //  scope up to Local Hard Drives on a local folder
                *_szInitialNamespace = *_szInitialPath = 0;
                bScoped = _SetLocalDefaultScope();
            }
            else if( _threadState.fComplete /* finished populating namespace combo */ )
            {
                bScoped = AssignNamespace( _szInitialNamespace, _szInitialPath, FALSE );        
            }
        }
    }

    return bScoped;
}

//-------------------------------------------------------------------------//
//  Scope to the hard-coded shell default namespace.
BOOL CFindFilesDlg::_SetLocalDefaultScope()
{
    BOOL bScoped = FALSE;

    //  Initialize fallback initial namespace

    //  Try Local Hard Drives
    if( *_szLocalDrives != 0 )
        bScoped = AssignNamespace( _szLocalDrives, NULL, FALSE );

    //  If we failed, this means that the namespace combo hasn't
    //  been populated yet.   
    //  We just sit tight, cuz the populating thread will fall back on
    //  the LocalDefaultScope.
    return bScoped;
}

//-------------------------------------------------------------------------//
//  search **band** show/hide handler
void CFindFilesDlg::OnBandShow( BOOL fShow )
{
    CSearchCmdDlg::OnBandShow( fShow ) ;
    if( fShow )
    {
        //  Establish the first showing's band width
        if( !_fDebuted && _pfsb->IsBandDebut() )
        {
            _pfsb->SetDeskbandWidth( GetIdealDeskbandWidth() );
            _fDebuted = TRUE;
        }
        
        //  If we're tracking the scope to the current folder shell view,
        //  update it now, as it may have changed.
        if( _fTrackScope != TRACKSCOPE_NONE )
        {
            _bScoped = FALSE;
            _SetFolderScope();
        }
        
        //  restart our UI state timer
        SetTimer( UISTATETIMER, UISTATETIMER_DELAY, NULL );
    }
    else
    {
        //  we're being hidden so stop updating our state indicators.
        KillTimer( UISTATETIMER );
    }
}

//-------------------------------------------------------------------------//
//  search band **dialog** show/hide handler
void CFindFilesDlg::OnBandDialogShow( BOOL fShow )
{
    CSearchCmdDlg::OnBandDialogShow( fShow );

    if( fShow )
    {
        //  If we're tracking the scope to the current folder shell view,
        //  update it now, as it may have changed.
        if( _fTrackScope != TRACKSCOPE_NONE )
        {
            _bScoped = FALSE;
            _SetFolderScope();
        }
    }
}

//-------------------------------------------------------------------------//
//  Explicit scoping method.   This will be called if a client
//  called IFileSearchBand::SetSearchParameters with a non-NULL scope.
HRESULT CFindFilesDlg::SetScope( IN VARIANT* pvarScope, BOOL bTrack )
{
    HRESULT hr = CBandDlg::SetScope( pvarScope, bTrack );
    
    if( S_OK != hr )
        return hr;

    LPITEMIDLIST pidlSearch = VariantToIDList( &_varScope0 );
    if( pidlSearch )
    {
        SHGetNameAndFlags(pidlSearch, SHGDN_FORPARSING,  _szInitialPath, SIZECHARS(_szInitialPath), NULL);
        SHGetNameAndFlags(pidlSearch, SHGDN_NORMAL, _szInitialNamespace, SIZECHARS(_szInitialNamespace), NULL);
        ILFree(pidlSearch);

        //  Did we get one?   
        if( *_szInitialNamespace || *_szInitialPath )
        {
            if( _bScoped )
            {
                //  If we've already scoped, update the namespace combo.
                //  Track if succeed and requested.
                if( AssignNamespace( _szInitialNamespace, _szInitialPath, FALSE ) && bTrack )
                    _fTrackScope = TRACKSCOPE_SPECIFIC ;
            }
            else 
            {
                //  Not already scoped.   We've assigned our initial namespace,
                //  let the namespace thread completion handler update
                //  the combo
                if( bTrack )
                    _fTrackScope = TRACKSCOPE_SPECIFIC ;
            }
        }
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _Attach( m_hWnd );
    _dlgOptions.SetBandDlg( this );

    //  Register specialty window classes.
    EVAL( DivWindow_RegisterClass() );
    EVAL( LinkWindow_RegisterClass() );
    EVAL( GroupButton_RegisterClass() );
    
    //  Initialize some metrics
    CMetrics&   metrics = _pfsb->GetMetrics();
    RECT        rc;
    int         cxBtn;

    _pfsb->GetMetrics().Init( m_hWnd );

    ::GetWindowRect( GetDlgItem( IDC_FILESPEC ), &rc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    metrics.ExpandOrigin().x = rc.left;

    //  Position start, stop buttons.
    //  BUGBUG: this should go in LayoutControls().
    ::GetWindowRect( GetDlgItem( IDC_SEARCH_START ), &rc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    if( (cxBtn = _GetResourceMetric( m_hWnd, IDS_FSEARCH_STARTSTOPWIDTH, TRUE )) > 0 )
    {
        rc.right = rc.left + cxBtn;
    
        ::SetWindowPos( GetDlgItem( IDC_SEARCH_START ), NULL, 
                        rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    
        OffsetRect( &rc, cxBtn + _PixelsForDbu( m_hWnd, 12, TRUE ), 0 );
        ::SetWindowPos( GetDlgItem( IDC_SEARCH_STOP ), NULL, 
                        rc.left, rc.top, RECTWIDTH(&rc), RECTHEIGHT(&rc),
                        SWP_NOZORDER|SWP_NOACTIVATE);
    }

    //  Create subdialogs and collect native sizes.
    _dlgOptions.Create( m_hWnd );
    ASSERT( IsWindow( _dlgOptions ) );

    //  Load settings
    LoadSaveUIState( 0, FALSE );

    //  Show/Hide the "Search" Options subdialog
    _dlgOptions.ShowWindow( _fDisplayOptions ? SW_SHOW : SW_HIDE );

    //  Create 'link' child controls
    POINT pt;
    pt.x = metrics.CtlMarginX();
    pt.y = 0;

    //  Create 'Search Options' link and group button
    _CreateLinkWindow( m_hWnd, IDC_SEARCHLINK_OPTIONS, pt, 
                       IDS_FSEARCH_SEARCHLINK_OPTIONS, !_fDisplayOptions );

    TCHAR szGroupBtn[128];
    EVAL( LoadString( HINST_THISDLL, IDS_FSEARCH_GROUPBTN_OPTIONS, 
                      szGroupBtn, ARRAYSIZE(szGroupBtn) ) );
    HWND hwndGrpBtn = CreateWindowEx( 0, GROUPBUTTON_CLASS, szGroupBtn, 
                                      WS_CHILD|WS_BORDER|WS_TABSTOP, pt.x, pt.y, 400, 18, 
                                      m_hWnd, (HMENU)IDC_GROUPBTN_OPTIONS, HINST_THISDLL, NULL );
    if( IsWindow( hwndGrpBtn ) )
    {
        ::SendMessage( hwndGrpBtn, GBM_SETBUDDY, 
                       (WPARAM)_dlgOptions.m_hWnd, (LPARAM)GBBF_HRESIZE|GBBF_VSLAVE );
        ::ShowWindow( GetDlgItem(IDC_GROUPBTN_OPTIONS), _fDisplayOptions ? SW_SHOW : SW_HIDE );
    }
                    
    //  Create cross-navigation links
    _CreateSearchLinks( m_hWnd, pt, IDC_SEARCHLINK_FILES );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV1, pt, 2, GetDlgItem( IDC_FSEARCH_CAPTION ) );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV2, pt, 1, GetDlgItem( IDC_SEARCHLINK_CAPTION ) );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV3, pt, 1, GetDlgItem( IDC_SEARCHLINK_PEOPLE ) );

    //  Do some cosmetic and initialization stuff
    OnWinIniChange();

    _InitializeMru( GetDlgItem(IDC_FILESPEC), &_pacFileSpec, 
                    TEXT("FilesNamedMRU"), &_pmruFileSpec );
    _InitializeMru( GetDlgItem(IDC_GREPTEXT), &_pacGrepText, 
                    TEXT("ContainingTextMRU"), &_pmruGrepText );

    SendDlgItemMessage( IDC_FILESPEC, EM_LIMITTEXT, MAX_EDIT, 0L );
    SendDlgItemMessage( IDC_GREPTEXT, EM_LIMITTEXT, MAX_EDIT, 0L );

    HWND hwndCombo = GetDlgItem( IDC_NAMESPACE );
    HIMAGELIST hil = _GetSystemImageListSmallIcons();

    ::SendMessage( hwndCombo, CBEM_SETEXTENDEDSTYLE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE);
    
    ::SendMessage(hwndCombo, CBEM_SETIMAGELIST, 0, (LPARAM)hil);
    ::SendMessage(hwndCombo, CBEM_SETEXSTYLE, 0, 0);

    //  Launch thread to populate the namespaces combo.
    _threadState.hwndCtl   = GetDlgItem( IDC_NAMESPACE );
    _threadState.pvParam   = this;
    _threadState.fComplete = FALSE;
    _threadState.fCancel   = FALSE;
    DWORD dwThreadID;
    _hNamespaceThread = CreateThread( NULL, 0L, NamespaceThreadProc, 
                                       &_threadState, 0, &dwThreadID );

    //  Layout our subdialogs and update state representation...
    LayoutControls();
    UpdateSearchCmdStateUI();

    SetTimer( UISTATETIMER, UISTATETIMER_DELAY, NULL );

    return TRUE;  // Let the system set the focus
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnEraseBkgnd( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& )
{
    // paint the background
    _PaintDlg( m_hWnd, _pfsb->GetMetrics(), (HDC)wParam ); 
    
    if( _fDisplayOptions )
        // ensure that the group button is updated.
        SendDlgItemMessage( IDC_GROUPBTN_OPTIONS, WM_NCPAINT, (WPARAM)1, 0L );
    
    //  validate our work.
    ValidateRect( NULL );
    return TRUE;   
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::OnWinIniChange()
{
    CBandDlg::OnWinIniChange();

    //  redisplay animated icon
    HWND hwndIcon = GetDlgItem( IDC_FSEARCH_ICON );
    Animate_Close( hwndIcon );
    Animate_OpenEx( hwndIcon, HINST_THISDLL, MAKEINTRESOURCE(IDA_FINDFILE) );
    SendDlgItemMessage( IDC_NAMESPACE, CB_SETDROPPEDWIDTH, 
                        _PixelsForDbu( m_hWnd, MIN_NAMESPACELIST_WIDTH, TRUE ), 0L );

    _BeautifyCaption( IDC_FSEARCH_CAPTION );

    _dlgOptions.OnWinIniChange();
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnDestroy( UINT, WPARAM, LPARAM, BOOL& bHandled )
{
    KillTimer( UISTATETIMER );
    StopSearch();
    if (_pSrchCmd)
    {
        DisconnectEvents();
        IUnknown_SetSite(_pSrchCmd, NULL);
    }
    _threadState.fCancel = TRUE;
    _fOnDestroy = TRUE;
    bHandled = FALSE;
    return 0L;
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::Validate()
{
    TCHAR   szPath[MAX_URL_STRING],
            szTest[128];
    UINT    nErrStr;

    if( S_OK == _GetTargetNamespace( szPath, ARRAYSIZE(szPath), NULL, &nErrStr ) )
    {
        EVAL( LoadString( HINST_THISDLL, IDS_FSEARCH_HTTP_SUBSTR, szTest, ARRAYSIZE(szTest) ) );
        if( StrStrI( szPath, szTest ) != NULL )
        {
            ShellMessageBox( HINST_THISDLL, _pfsb->m_hWnd, 
                             MAKEINTRESOURCE(IDS_FSEARCH_HTTP_NOT_SUPPORTED), NULL,
                             MB_OK|MB_ICONASTERISK );
            ::SetFocus( GetDlgItem( IDC_NAMESPACE ) );
            return FALSE;
        }

        EVAL( LoadString( HINST_THISDLL, IDS_FSEARCH_FTP_SUBSTR, szTest, ARRAYSIZE(szTest) ) );
        if( StrStrI( szPath, szTest ) != NULL )
        {
            ShellMessageBox( HINST_THISDLL, _pfsb->m_hWnd, 
                             MAKEINTRESOURCE(IDS_FSEARCH_FTP_NOT_SUPPORTED), NULL,
                             MB_OK|MB_ICONASTERISK );
            ::SetFocus( GetDlgItem( IDC_NAMESPACE ) );
            return FALSE;
        }

        //  Handle selection of "Browse..." item
        EVAL( LoadString( HINST_THISDLL, IDS_SNS_BROWSER_FOR_DIR, szTest, ARRAYSIZE(szTest) ) );
        if( 0 == StrCmp( szPath, szTest ) ) // "Browse..."
        {
            _BrowseAndAssignNamespace();
            if( _GetTargetNamespace( szPath, ARRAYSIZE(szPath), NULL, &nErrStr ) != S_OK )
            {
                if( nErrStr != 0 )
                {
                    ShellMessageBox( HINST_THISDLL, _pfsb->m_hWnd, MAKEINTRESOURCE(nErrStr),
                                     NULL, MB_OK|MB_ICONASTERISK );
                    ::SetFocus( GetDlgItem( IDC_NAMESPACE ) );
                }
            }
            return FALSE;
        }
    }
    else if( nErrStr != 0 )
    {
        ShellMessageBox( HINST_THISDLL, _pfsb->m_hWnd, MAKEINTRESOURCE(nErrStr),
                         NULL, MB_OK|MB_ICONASTERISK );
        ::SetFocus( GetDlgItem( IDC_NAMESPACE ) );
        return FALSE;
    }

    if( !_dlgOptions.Validate() )
        return FALSE;

    return TRUE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindFilesDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    HRESULT         hr;
    VARIANT         var;
    BOOL            bNamespace = FALSE; // mandatory constraint

    //  Add location (namespace) constraint(s)
    TCHAR           szPath[MAX_URL_STRING];
    COMBOBOXEXITEM  item;

    item.mask = CBEIF_INDENT;
    VariantInit( &var );

    // If the user enters a path as a filename, it will recognize it as a path and replace
    // the filename with just the file portion and the namespace with the path.
    if( ::GetDlgItemText(m_hWnd, IDC_FILESPEC, szPath, MAX_URL_STRING) > 0 )
    {
        if( StrChr(szPath, TEXT('\\')) != NULL )
        {
            if( !_PathLooksLikeFilePattern(szPath) &&
                ( PathIsUNCServer(szPath) /* string test: \\server */|| 
                 _PathIsUNCServerShareOrSub(szPath) /* string test: \\server\share */ ||
                  PathIsDirectory(szPath)) /* this actually tests existence */ )
            {
                ::SetDlgItemText(m_hWnd, IDC_FILESPEC, TEXT("*.*"));
                AssignNamespace(NULL, szPath, FALSE);
            }
            else
            {
                // just use the prefix for the file spec & the root for the location
                TCHAR szRoot[MAX_URL_STRING];
                lstrcpy(szRoot, szPath);
                if( PathRemoveFileSpec(szRoot) && szRoot[0] != TEXT('\0')) 
                {
                    PathStripPath(szPath);
                    ::SetDlgItemText(m_hWnd, IDC_FILESPEC, szPath);
                    AssignNamespace(NULL, szRoot, FALSE);                
                }
            }
        }
    }

    if( SUCCEEDED(_GetTargetNamespace( szPath, ARRAYSIZE(szPath), &item )) )
    {
        //  If this namespace is a namespace combo singleton, 
        //  add it as single constraint
        if( _IsPathSingleton( szPath ) )
        {
            if( _IsPathList( szPath ) )
            {
                //  todo: Validate each path token in the list
            }
            else
            {
                if( FAILED( (hr = _PathValidate( szPath, GetParent(), TRUE )) ) )
                {
                    TCHAR szMsg[MAX_URL_STRING];
                    if( _FmtError( IDS_FSEARCH_INVALIDFOLDER_FMT, szPath, szMsg, ARRAYSIZE(szMsg) ) )
                        ShellMessageBox( HINST_THISDLL, GetParent(), szMsg, NULL, MB_OK|MB_ICONASTERISK );

                    ::SetFocus( GetDlgItem( IDC_NAMESPACE ) );

                    return hr;
                }
            }

            if( SUCCEEDED( _T2BstrVariant( szPath, &var ) ) )
            {
                if( FAILED( (hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_LOOKIN ), &var )) ) )
                    VariantClear( &var );
                else
                    bNamespace = TRUE;
            }
            else
                VariantInit( &var );
        }
        //  Not a singleton, enumerate child items and add their paths instead
        else
        {
            int iIndent = item.iIndent;
            while( S_OK == _GetNextNamespace( szPath, ARRAYSIZE(szPath), &item ) &&
                   item.iIndent > iIndent )
            {
                _BstrAppendToken( &var.bstrVal, TEXT(";"), szPath );
            }

            if( NULL != var.bstrVal )
            {
                var.vt = VT_BSTR;
                if( FAILED( (hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_LOOKIN ), &var )) ) )
                    VariantClear( &var );
                else
                    bNamespace = TRUE;
            }
        }
    }

    if( !bNamespace )
    {
        return E_FAIL;
    }

    //  Add 'Files Named' constraint
    if( S_OK == _GetWindowValue( m_hWnd, IDC_FILESPEC, VT_BSTR, &var ) )
    {
        if( SUCCEEDED( (hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_NAMED ), &var )) ) )
            _AddMruStringFromWindow( _pmruFileSpec, GetDlgItem( IDC_FILESPEC ) );
        VariantClear( &var );
    }

    //  Add 'Containing Text' constraint
    if( S_OK == _GetWindowValue( m_hWnd, IDC_GREPTEXT, VT_BSTR, &var ) )
    {
        VARIANT varQuery;
        ULONG   ulDialect;
        BOOL    fCiQuery = IsCiQuery( &var, &varQuery, &ulDialect, FALSE );
        BOOL    bCaseSensitive = _dlgOptions.IsAdvancedOptionChecked( IDC_USE_CASE );

        //  Is this a CI query?
        if( fCiQuery )
        {
            if( bCaseSensitive )
            {
                //  todo: show dialog to warn that case-sensitivity will not be honored for
                //  tripolish searches.
            }

            if( SUCCEEDED( (hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_INDEXEDSEARCH ), &varQuery )) ) )
            {
                _AddMruStringFromWindow( _pmruGrepText, GetDlgItem( IDC_GREPTEXT ) );
                
                VariantClear( &var );
                var.vt = VT_UI4;
                var.ulVal = ulDialect;
                hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_QUERYDIALECT ), &var );
            }
        }
        else
        {
            //  add to 'containing text' constraint
            if( SUCCEEDED( (hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_CONTAININGTEXT ), &var )) ) )
                _AddMruStringFromWindow( _pmruGrepText, GetDlgItem( IDC_GREPTEXT ) );
        }
        VariantClear( &varQuery );
        VariantClear( &var );
    }

    //  Warning flags
    
    if( _dwRunOnceWarningFlags != DFW_DEFAULT ) 
    {
        // re-run the query w/ temporary warning bits.
        var.ulVal = _dwRunOnceWarningFlags;
        var.vt    = VT_UI4;
        //_dwRunOnceWarningFlags = DFW_DEFAULT; cannot reset it here in case of error, must preserve them
       
        hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_WARNINGFLAGS ), &var );
    }
    else if( _dwWarningFlags != DFW_DEFAULT )
    {
        var.ulVal = _dwWarningFlags;
        var.vt    = VT_UI4;
        hr = _AddConstraint( pSrchCmd, GetConstraintName( FSBC_WARNINGFLAGS ), &var );
    }
    
    VariantClear( &var );

    hr = _dlgOptions.AddConstraints( pSrchCmd );
    return hr;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindFilesDlg::RestoreConstraint( const BSTR bstrName, const VARIANT* pValue )
{
    if( IsConstraintName( FSBC_NAMED, bstrName) )
    {
        _SetWindowValue( m_hWnd, IDC_FILESPEC, pValue );
        return S_FALSE;
    }

    if( IsConstraintName( FSBC_INDEXEDSEARCH, bstrName) )
    {
        ASSERT( VT_BSTR == pValue->vt );
        if( pValue->bstrVal )
        {
            int cch = lstrlenW(pValue->bstrVal) + 2;
            LPWSTR pwszVal = new WCHAR[cch];
            if( pwszVal )
            {
                *pwszVal = L'!';
                StrCatW( pwszVal, pValue->bstrVal );
            }
        
            ::SetDlgItemTextW( m_hWnd, IDC_GREPTEXT, pwszVal );
            if( pwszVal )
                delete [] pwszVal;
        }
        return S_FALSE;
    }

    if( IsConstraintName( FSBC_CONTAININGTEXT, bstrName) )
    {
        _SetWindowValue( m_hWnd, IDC_GREPTEXT, pValue );
        return S_FALSE;
    }

    HRESULT hr = _dlgOptions.RestoreConstraint( bstrName, pValue );

    if( S_OK == hr ) // opened a dialog
        _ShowOptions( TRUE );

    if( SUCCEEDED( hr ) )
        return hr;   
   
    return E_FAIL;
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::RestoreSearch()
{
    DFConstraint* pdfc = NULL;
    HRESULT hr;
    BOOL    bMore = TRUE;
    ISearchCommandExt* pSrchCmd;

    if( NULL == (pSrchCmd = GetSearchCmd()) )
    {
        ASSERT( pSrchCmd );
        return;
    }

    CSearchCmdDlg::Clear();

    // we'll anchor to any restored scope, or the default
    _fTrackScope = TRACKSCOPE_GENERAL ;  

    for( hr = pSrchCmd->GetNextConstraint( TRUE, &pdfc );
         S_OK == hr && bMore;
         hr = pSrchCmd->GetNextConstraint( FALSE, &pdfc ) )
    {
        BSTR    bstrName = NULL;

        if( S_OK == (hr = pdfc->get_Name( &bstrName )) && bstrName )
        {
            if( *bstrName == 0 )
                bMore = FALSE;   // no more constraints.
            else
            {
                VARIANT varValue;
                VariantInit( &varValue );
                if( S_OK == (hr = pdfc->get_Value( &varValue )) )
                {
                    //  If this is the 'lookin' value, cache the path.
                    if( IsConstraintName( FSBC_LOOKIN, bstrName ) )
                    {
                        if( VT_BSTR == varValue.vt && varValue.bstrVal != 0 )
                        {
                            USES_CONVERSION;

                            //  Assign path and clear display name (which we don't know or care about).
                            if( _bScoped )
                                AssignNamespace( NULL, W2T( varValue.bstrVal ), FALSE );
                            else
                            {
                                lstrcpyn( _szInitialPath, W2T( varValue.bstrVal ), ARRAYSIZE(_szInitialPath) );
                                *_szInitialNamespace = 0;
                            }
                        }
                    }
                    else
                        RestoreConstraint( bstrName, &varValue );    
                    VariantClear( &varValue );
                }
            }
            SysFreeString( bstrName );
        }

        pdfc->Release();
    }
    LayoutControls();
    _pfsb->UpdateLayout();
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindFilesDlg::_GetTargetNamespace( 
    OUT LPTSTR pszPath, 
    IN int cchPath, 
    IN OUT OPTIONAL COMBOBOXEXITEM* pItem, 
    OUT OPTIONAL UINT* puErrStr )
{
    HWND hwndNamespace = GetDlgItem( IDC_NAMESPACE );
    TCHAR pszTempPathBuffer[MAX_PATH];
    LPTSTR pszTempPath = pszTempPathBuffer;
    ASSERT( IsWindow( hwndNamespace ) );
    ASSERT( pszPath );

    if( pItem )    pItem = NULL;
    if( puErrStr ) *puErrStr = 0;

    *pszPath = 0;
    INT_PTR iSel = _GetNamespaceComboSelItemText(
                    hwndNamespace, TRUE /*want path*/, pszTempPath, MAX_PATH );
    //  Note: if iSel == -1, the user browsed to or typed in a path.

    // Eat whitespace
    for( ; ( TEXT( '\0' ) != *pszTempPath ) && ( TEXT( ' ' ) == *pszTempPath ) ; pszTempPath = CharNext( pszTempPath ) );

    StrCpyN(pszPath, pszTempPath, cchPath);

    if( 0 == *pszPath )
    {
        if( puErrStr )
            *puErrStr = IDS_FSEARCH_EMPTYFOLDER;
        return E_FAIL;
    }

    if( NULL == pItem )     
        return S_OK;       // done.
    
    pItem->iItem = iSel;
    if( iSel < 0 )
        return S_FALSE;    // retrieval of COMBOBOXEXITEM data impossible

    //  Fetch the item.
    if( ::SendMessage( hwndNamespace, CBEM_GETITEM, iSel, (LPARAM)pItem ) )
        return S_OK;

    return S_FALSE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindFilesDlg::_GetNextNamespace( OUT LPTSTR pszPath, IN int cchPath, IN OUT COMBOBOXEXITEM* pItem )
{
    HRESULT hr = E_FAIL;
    HWND hwndNamespace = GetDlgItem( IDC_NAMESPACE );
    ASSERT( IsWindow( hwndNamespace ) );
    ASSERT( pItem );
    ASSERT( pszPath );

    *pszPath = 0;
    INT_PTR iItem = pItem->iItem;
    pItem->iItem++;    // increment item index

    if( CB_ERR != _GetNamespaceComboItemText( hwndNamespace, pItem->iItem, TRUE, pszPath, cchPath ) )
    {
        if( ::SendMessage( hwndNamespace, CBEM_GETITEM, pItem->iItem, (LPARAM)pItem ) )
        {
            hr = S_OK;
        }
    }

    if( FAILED( hr ) )
        pItem->iItem = iItem;  // failed; revert item index
    return hr;
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::Clear()
{
    CSearchCmdDlg::Clear();
    
    //  Clear edit fields
    SetDlgItemText( IDC_FILESPEC, NULL );
    SetDlgItemText( IDC_GREPTEXT, NULL );

    _dlgOptions.Clear();
    _pfsb->UpdateLayout( BLF_ALL );
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::LoadSaveUIState( UINT nIDCtl, BOOL bSave ) 
{
    if( 0 == nIDCtl )   // load/save all.
    {
        LoadSaveUIState( IDC_SEARCHLINK_OPTIONS, bSave );
        LoadSaveUIState( LSUIS_WARNING, bSave );
    }
    
    HKEY    hkey;
    if( (hkey = _pfsb->GetBandRegKey( bSave )) != NULL )
    {
        DWORD   dwData;
        DWORD   cbData;
        DWORD   dwType;
        LPCTSTR pszVal = NULL; 

        switch( nIDCtl )
        {
            case IDC_SEARCHLINK_OPTIONS:
                pszVal = TEXT("UseSearchOptions");
                dwData = _fDisplayOptions;
                cbData = sizeof(dwData);
                dwType = REG_DWORD;
                break ;
            
            case LSUIS_WARNING:
                pszVal = TEXT("Warnings");
                dwData = _dwWarningFlags;
                cbData = sizeof(_dwWarningFlags);
                dwType = REG_DWORD;
                break ;
        }

        if( bSave )
            RegSetValueEx( hkey, pszVal, 0, dwType, (LPBYTE)&dwData, cbData );
        else
        {
            if( ERROR_SUCCESS == RegQueryValueEx( hkey, pszVal, 0, &dwType, 
                                                  (LPBYTE)&dwData, &cbData ) )
            {
                switch( nIDCtl )
                {
                    case IDC_SEARCHLINK_OPTIONS:
                        _fDisplayOptions = BOOLIFY(dwData);
                        break;
                    case LSUIS_WARNING:
                        _dwWarningFlags = dwData;
                        break;
                }
            }
        }
        
        RegCloseKey( hkey );
    }

#ifdef _LOADSAVE_SUBDLG_UI__
    if( nIDCtl == 0 )
        _dlgOptions.LoadSaveUIState( 0, bSave );
#endif _LOADSAVE_SUBDLG_UI__
}

//-------------------------------------------------------------------------//
HWND CFindFilesDlg::GetFirstTabItem() const
{
    return GetDlgItem( FSEARCHMAIN_TABFIRST );
}

//-------------------------------------------------------------------------//
HWND CFindFilesDlg::GetLastTabItem() const
{
    return GetDlgItem( FSEARCHMAIN_TABLAST );
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::GetAutoCompleteObjectForWindow( HWND hwnd, IAutoComplete2** ppac2 )
{
    *ppac2 = NULL;

    if( hwnd == GetDlgItem( IDC_FILESPEC ) )
        *ppac2 = _pacFileSpec;
    else if( hwnd == GetDlgItem( IDC_GREPTEXT ) )
        *ppac2 = _pacGrepText;

    if( *ppac2 )
    {
        (*ppac2)->AddRef();
        return TRUE;
    }
    return CBandDlg::GetAutoCompleteObjectForWindow( hwnd, ppac2 );
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindFilesDlg::TranslateAccelerator( LPMSG lpmsg )
{
    //  Check for Ctrl+Nav Key:
    if( S_OK == CSearchCmdDlg::TranslateAccelerator( lpmsg ) )
        return S_OK;

    //  Check for VK_RETURN key.
    if( WM_KEYDOWN == lpmsg->message )
    {
        HWND hwndFocus = ::GetFocus();
        HWND hwndNamespace = GetDlgItem( IDC_NAMESPACE );
        if( hwndFocus == hwndNamespace || ::IsChild( hwndNamespace, hwndFocus ) )
        {
            if( VK_RETURN == lpmsg->wParam || VK_TAB == lpmsg->wParam || VK_F6 == lpmsg->wParam )
            {
                _UIReconcileAdHocNamespace( hwndNamespace );
            }
            else 
            {
                //  Hide edit image if this virtkey maps to a character,
                if( MapVirtualKey( (UINT)lpmsg->wParam, 2 ) != 0 /* it's a char */ )
                    _fAdHocNamespace = TRUE ;
                _ShowNamespaceEditImage( !_fAdHocNamespace );
            }
        }
        

    }
    if( _dlgOptions.IsChild( lpmsg->hwnd ) &&
        S_OK == _dlgOptions.TranslateAccelerator( lpmsg ) )
        return S_OK;

    //  Handle it ourselves...
    return _pfsb->IsDlgMessage( m_hWnd, lpmsg );
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::GetMinSize( HWND hwndOC, LPSIZE psize ) const
{
    CMetrics& metrics = _pfsb->GetMetrics();
    RECT rc;

    //  Calculate minimum tracking width.
    ASSERT( psize );
    psize->cx = psize->cy = 0;

    if( !IsWindow( m_hWnd ) )
        return FALSE;

        // determine mininum width
    HWND hwndLimit = GetDlgItem( FSEARCHMAIN_RIGHTMOST );
    if( !::GetWindowRect( hwndLimit, &rc ) )
    {
        ASSERT( hwndLimit != NULL );
        return FALSE;
    }
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    psize->cx = rc.right + metrics.CtlMarginX();

    // determine mininum height
    hwndLimit = GetDlgItem( FSEARCHMAIN_BOTTOMMOST );

    if( !(IsWindow( hwndLimit ) && ::GetWindowRect( hwndLimit, &rc )) )
        return FALSE;

    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    psize->cy = rc.bottom + metrics.TightMarginY();

    return TRUE;
}

//-------------------------------------------------------------------------//
int CFindFilesDlg::GetIdealDeskbandWidth() const
{
    LONG cx0 = _GetResourceMetric( m_hWnd, IDS_FSEARCH_BANDWIDTH, TRUE );
    ASSERT( cx0 >= 0 );

    return cx0 + (_pfsb->GetMetrics().CtlMarginX() * 2);
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::GetMinMaxInfo( HWND hwndOC, LPMINMAXINFO pmmi )
{
    SIZE sizeMin;
    if( GetMinSize( hwndOC, &sizeMin ) )
    {
        pmmi->ptMinTrackSize.x = sizeMin.cx;
        pmmi->ptMinTrackSize.y = sizeMin.cy;
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::LayoutControls( int cx, int cy )
{
    if( cx < 0 || cy < 0 )
    {
        RECT rcClient;
        GetClientRect( &rcClient );
        cx = RECTWIDTH( &rcClient );
        cy = RECTHEIGHT( &rcClient );
    }
    CBandDlg::LayoutControls( cx, cy );

    CMetrics& metrics = _pfsb->GetMetrics();
    POINT ptOrigin = metrics.ExpandOrigin();
    HDWP  hdwp = BeginDeferWindowPos( 6 );

    if( hdwp )
    {
        //  Resize edit, combo immediate children
        int i;
        enum {  ircFILESPEC,
                ircGREPTEXT,
                ircNAMESPACE,
                ircSEARCHSTART,
                ircOPTIONGRP,
                ircOPTIONSDLG,
                ircLINKCAPTION,
                ircDIV2,
                irc_count };
        RECT rcCtls[irc_count];

        ::GetWindowRect( GetDlgItem( IDC_FILESPEC ),            &rcCtls[ircFILESPEC] );
        ::GetWindowRect( GetDlgItem( IDC_GREPTEXT ),            &rcCtls[ircGREPTEXT] );
        ::GetWindowRect( GetDlgItem( IDC_NAMESPACE ),           &rcCtls[ircNAMESPACE] );
        ::GetWindowRect( GetDlgItem( IDC_SEARCH_START ),        &rcCtls[ircSEARCHSTART] );
        ::GetWindowRect( GetDlgItem( IDC_GROUPBTN_OPTIONS ),    &rcCtls[ircOPTIONGRP] );
        ::GetWindowRect( GetDlgItem( IDC_SEARCHLINK_CAPTION ),  &rcCtls[ircLINKCAPTION] );
        ::GetWindowRect( GetDlgItem( IDC_FSEARCH_DIV2 ),        &rcCtls[ircDIV2] );

        SIZE sizeOptions;
        _dlgOptions.GetWindowRect( &rcCtls[ircOPTIONSDLG] );
        _dlgOptions.GetMinSize( &sizeOptions );
        rcCtls[ircOPTIONSDLG].bottom = rcCtls[ircOPTIONSDLG].top + sizeOptions.cy;
        for(i = 0; i < ARRAYSIZE(rcCtls); i++)
        {
            // MapWindowPoints is mirroring aware only if you pass two points        
            ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)(&rcCtls[i]), POINTSPERRECT );
        }    

        //  Position caption elements
        _LayoutCaption( IDC_FSEARCH_CAPTION, IDC_FSEARCH_ICON, IDC_FSEARCH_DIV1, cx );

        //  Resize ctl widths
        for( i = 0; i < irc_count; i++ )
            rcCtls[i].right = cx - metrics.CtlMarginX();

        //  Stretch the 'Named' combo:
        DeferWindowPos( hdwp, GetDlgItem( IDC_FILESPEC ), NULL, 0, 0,
                        RECTWIDTH( rcCtls + ircFILESPEC ), RECTHEIGHT(rcCtls + ircFILESPEC),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );

        //  Stretch the 'Containing Text' combo:
        DeferWindowPos( hdwp, GetDlgItem( IDC_GREPTEXT ), NULL, 0, 0,
                        RECTWIDTH( rcCtls + ircGREPTEXT ), RECTHEIGHT(rcCtls + ircGREPTEXT),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );

        //  Stretch the 'Look In' combo
        DeferWindowPos( hdwp, GetDlgItem( IDC_NAMESPACE ), NULL, 0, 0,
                        RECTWIDTH( rcCtls + ircNAMESPACE ), RECTHEIGHT(rcCtls + ircNAMESPACE),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
        
        //  Arrange dynamically positioned controls.
        ptOrigin.y = rcCtls[ircSEARCHSTART].bottom + metrics.LooseMarginY();
        if( _fDisplayOptions )
        {
            OffsetRect( &rcCtls[ircOPTIONGRP], metrics.CtlMarginX() - rcCtls[ircOPTIONGRP].left, 
                                                ptOrigin.y - rcCtls[ircOPTIONGRP].top );
            rcCtls[ircOPTIONSDLG].right = cx - metrics.CtlMarginX();

            ::SetWindowPos( GetDlgItem( IDC_GROUPBTN_OPTIONS ), NULL, 
                            rcCtls[ircOPTIONGRP].left, rcCtls[ircOPTIONGRP].top,
                            RECTWIDTH( &rcCtls[ircOPTIONGRP] ), RECTHEIGHT( &rcCtls[ircOPTIONGRP] ),
                            SWP_NOZORDER|SWP_NOACTIVATE );
            
            ::GetWindowRect( GetDlgItem( IDC_GROUPBTN_OPTIONS ),    &rcCtls[ircOPTIONGRP] );
            ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcCtls[ircOPTIONGRP], POINTSPERRECT );
            
            ptOrigin.y = rcCtls[ircOPTIONGRP].bottom + metrics.TightMarginY();
        }
        else
        {
            //  Position the 'Options' link
            _LayoutLinkWindow( m_hWnd, metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                                ptOrigin.y, IDC_SEARCHLINK_OPTIONS );
        }

        ptOrigin.y += metrics.TightMarginY();

        //  Position the 'Search for Other Items' caption, divider and link windows
        const int rgLinkIDs[] = { 
            IDC_SEARCHLINK_FILES,
            IDC_SEARCHLINK_COMPUTERS,
            IDC_SEARCHLINK_PRINTERS,
            IDC_SEARCHLINK_PEOPLE,
            -IDC_FSEARCH_DIV3,
            IDC_SEARCHLINK_INTERNET, 
        };

        _LayoutSearchLinks( IDC_SEARCHLINK_CAPTION, IDC_FSEARCH_DIV2, !_fDisplayOptions,
                            metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                            ptOrigin.y, rgLinkIDs, ARRAYSIZE(rgLinkIDs) );

        EndDeferWindowPos( hdwp );
    }

}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnUpdateLayout( UINT, WPARAM wParam, LPARAM, BOOL& )
{
    LayoutControls();
    _pfsb->UpdateLayout( (ULONG)wParam );
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnTimer( UINT, WPARAM wParam, LPARAM, BOOL& )
{
    if( UISTATETIMER == wParam && IsWindowVisible() )
        UpdateSearchCmdStateUI();
    
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnOptions( int idCtl, LPNMHDR pnmh, BOOL& )
{
    _ShowOptions( !_fDisplayOptions );
    LoadSaveUIState( IDC_SEARCHLINK_OPTIONS, TRUE );

    if( _fDisplayOptions )
        _dlgOptions.TakeFocus();
    else
        ::SetFocus( GetDlgItem(IDC_SEARCHLINK_OPTIONS) );

    return 0L;
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::_ShowOptions( BOOL bShow )
{
    _fDisplayOptions = bShow;

    //  don't need to scroll if we've expanded a subdialog,
    //  but we do if we've contracted one.
    ULONG dwLayoutFlags = BLF_ALL;
    if( _fDisplayOptions )
        dwLayoutFlags &= ~BLF_SCROLLWINDOW;    

    LayoutControls();
    _pfsb->UpdateLayout( dwLayoutFlags );

    ::ShowWindow( GetDlgItem(IDC_GROUPBTN_OPTIONS), _fDisplayOptions ? SW_SHOW : SW_HIDE );
    ::ShowWindow( GetDlgItem(IDC_SEARCHLINK_OPTIONS), !_fDisplayOptions ? SW_SHOW : SW_HIDE );

}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnQueryOptionsHeight( int idCtl, LPNMHDR pnmh, BOOL& )
{
    GBNQUERYBUDDYSIZE* pqbs = (GBNQUERYBUDDYSIZE*)pnmh;
    pqbs->cy = _dlgOptions.QueryHeight( pqbs->cx, pqbs->cy );
    return TRUE;
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::UpdateSearchCmdStateUI( DISPID eventID )
{
    if ( _fOnDestroy )
        return;

    if ( DISPID_SEARCHCOMMAND_COMPLETE == eventID 
    ||   DISPID_SEARCHCOMMAND_ABORT == eventID )
        _dwRunOnceWarningFlags = DFW_DEFAULT;

    CSearchCmdDlg::UpdateSearchCmdStateUI( eventID );
    _dlgOptions.UpdateSearchCmdStateUI( eventID );
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::OnSearchCmdError( HRESULT hr, LPCTSTR pszError )
{
    if( SCEE_SCOPEMISMATCH == HRESULT_CODE(hr) 
    ||  SCEE_INDEXNOTCOMPLETE == HRESULT_CODE(hr) )
    {
        //  Set up checkbox
        BOOL fFlag = SCEE_SCOPEMISMATCH == HRESULT_CODE(hr)? DFW_IGNORE_CISCOPEMISMATCH :
                                                             DFW_IGNORE_INDEXNOTCOMPLETE ,
             fNoWarn = (_dwWarningFlags & fFlag) != 0,
             fNoWarnPrev = fNoWarn;
        USHORT uDlgT = SCEE_SCOPEMISMATCH == HRESULT_CODE(hr)? DLG_FSEARCH_SCOPEMISMATCH :
                                                               DLG_FSEARCH_INDEXNOTCOMPLETE ;
        int  nRet = CSearchWarningDlg_DoModal( m_hWnd, uDlgT, &fNoWarn );

        if( fNoWarn )
            _dwWarningFlags |= fFlag;
        else
            _dwWarningFlags &= ~fFlag;        
        
        if( fNoWarnPrev != fNoWarn )
            LoadSaveUIState( LSUIS_WARNING, TRUE );

        if( IDOK == nRet )
        {
            _dwRunOnceWarningFlags |= _dwWarningFlags | fFlag ; // preserve the old run once flags...
            //  hack one, hack two...  let's be USER!!! [scotthan]
            PostMessage( WM_COMMAND, MAKEWPARAM(IDC_SEARCH_START, BN_CLICKED),
                         (LPARAM)GetDlgItem( IDC_SEARCH_START ) );
        }
        else
            ::SetFocus( GetDlgItem( IDC_NAMESPACE ) );

        return TRUE;
    }
    return CSearchCmdDlg::OnSearchCmdError( hr, pszError );
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnBtnClick( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    switch( nID )
    {
        case IDC_SEARCH_START:
        {
            if( _ShouldReconcileAdHocNamespace() )
                _UIReconcileAdHocNamespace( GetDlgItem( IDC_NAMESPACE ), NULL, TRUE );

            EnableStartStopButton( hwndCtl, FALSE );
            StartStopAnimation( TRUE );
            
            if( FAILED( StartSearch() ) )
            {
                EnableStartStopButton( hwndCtl, TRUE );
                StartStopAnimation( FALSE );
            }
                
            break;
        }

        case IDC_SEARCH_STOP:
            StopSearch();
            break;
    }
    return 0L;
}

//-------------------------------------------------------------------------//
void CFindFilesDlg::NavigateToResults( IWebBrowser2* pwb2 )
{
    BSTR    bstrUrl = SysAllocString( L"::{e17d4fc0-5564-11d1-83f2-00a0c90dc849}" );
    VARIANT varNil;

    VariantInit( &varNil );
    pwb2->Navigate( bstrUrl, &varNil, &varNil, &varNil, &varNil );
    SysFreeString( bstrUrl );
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnStateChange( UINT, WPARAM, LPARAM, BOOL& )
{
    UpdateSearchCmdStateUI();
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnNamespaceSelEndOk( WORD nCode, WORD nID, HWND hwndCtl, BOOL& )
{
    LRESULT iSel;
    LPCVOID pvData = NULL;
    
    if( (iSel = SendDlgItemMessage( IDC_NAMESPACE, CB_GETCURSEL, 0, 0 )) != CB_ERR )
    {
        // Was this the "Browse..." item (pvData == INVALID_HANDLE_VALUE)?  
        if( INVALID_HANDLE_VALUE == 
                (pvData = (LPCVOID)_GetComboData( GetDlgItem( IDC_NAMESPACE ), iSel )) )
        {
            _BrowseAndAssignNamespace();
        }
        else
            _iCurNamespace = iSel;
    }

    _pfsb->SetDirty();
    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnNamespaceEditChange( WORD wID, WORD wCode, HWND hwndCtl, BOOL& bHandled )
{
    return OnEditChange( wID, wCode, hwndCtl, bHandled );
}

//-------------------------------------------------------------------------//
//  Handler for CBN_SELENDCANCEL, CBN_DROPDOWN, CBN_KILLFOCUS
LRESULT CFindFilesDlg::OnNamespaceReconcileCmd( WORD wID, WORD wCode, HWND hwndCtl, BOOL&)
{
    if( _ShouldReconcileAdHocNamespace() )
        _UIReconcileAdHocNamespace( hwndCtl, NULL, wCode != CBN_DROPDOWN );

    return 0L;
}

//-------------------------------------------------------------------------//
//  Handler for WM_NOTIFY::CBEN_ENDEDIT
LRESULT CFindFilesDlg::OnNamespaceReconcileNotify( int idCtl, LPNMHDR pnmh, BOOL& bHandled )
{
    if( _ShouldReconcileAdHocNamespace() )
    {
        //  Post ourselves a message to reconcile the ad hoc namespace.
        //  Note: We need to do this because ComboBoxEx won't update his window text if he
        //  is waiting for his CBEN_ENDEDIT notification message to return.
        PostMessage( WMU_NAMESPACERECONCILE, 0, 0L );
    }
    bHandled = FALSE; // let base class have a crack as well.
    return 0L;
}

//-------------------------------------------------------------------------//
//  WMU_NAMESPACERECONCILE handler
LRESULT CFindFilesDlg::OnNamespaceReconcileMsg( UINT, WPARAM, LPARAM, BOOL& )
{
    if( _ShouldReconcileAdHocNamespace() )
        _UIReconcileAdHocNamespace( GetDlgItem( IDC_NAMESPACE ), NULL, FALSE );
    return 0L;
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::_ShouldReconcileAdHocNamespace()
{
    return _fAdHocNamespace || 
           SendDlgItemMessage( IDC_NAMESPACE, CB_GETCURSEL, 0, 0L ) == CB_ERR;
}

//-------------------------------------------------------------------------//
//  Invokes lower Namespace reconciliation helper, updates some UI and 
//  instance state data. 
//  [BUGBUG] this was added as a late RC 'safe' delta, and should have actually
//  become part of _ReconcileAdHocNamespace() impl.
void CFindFilesDlg::_UIReconcileAdHocNamespace( HWND hwndNamespace, IN OPTIONAL LPCTSTR pszNamespace, BOOL bAsync )
{
    ASSERT( IsWindow( hwndNamespace ) );

    LRESULT iSel ;
    if( (iSel = _ReconcileAdHocNamespace( hwndNamespace, pszNamespace, bAsync )) != CB_ERR )
        _iCurNamespace = iSel;

    _ShowNamespaceEditImage( TRUE );
    _fAdHocNamespace = FALSE; // clear the ad hoc flag.    
}

//-------------------------------------------------------------------------//
//  Scans namespace combo for a matching namespace; if found, selects
//  the namespace item, otherwise adds an adhoc item and selects it.
//  
//  Important: don't call this directly, call _UIReconcileAdHocNamespace()
//  instead to ensure that instance state data is updated.
INT_PTR CFindFilesDlg::_ReconcileAdHocNamespace(
    IN HWND hwndCombo, 
    IN OPTIONAL LPCTSTR pszNamespace, 
    IN OPTIONAL BOOL bAsync )
{
    TCHAR szNamespace[MAX_URL_STRING];
    INT_PTR iSel = ::SendMessage( hwndCombo, CB_GETCURSEL, 0, 0L );

    if (iSel != CB_ERR)
    {
        void* pValue = (void*) ::SendMessage( hwndCombo, CB_GETITEMDATA, iSel, 0L );
        if( INVALID_HANDLE_VALUE == pValue )
        {
            // The user has selected the special Browse... item. 
            // Irreconcilable.  Return CB_ERR
            return CB_ERR;
        }
    }

    //  Don't know the namespace?  Use current window text.
    if( NULL == pszNamespace )
    {
        *szNamespace = NULL;
        ::GetWindowText( hwndCombo, szNamespace, ARRAYSIZE(szNamespace) );
        pszNamespace = szNamespace;
    }

    if( !(pszNamespace && *pszNamespace) )
        return CB_ERR;

    INT_PTR iFind;

    //  search display names
    if( CB_ERR == (iFind = _FindNamespace( hwndCombo, pszNamespace, FALSE )) )
    {
        // search paths
        TCHAR szTemp[MAX_URL_STRING];
        StrCpy( szTemp, pszNamespace );
        _PathFixup( szNamespace, szTemp ); // don't care if this fails, the path might be a path list
        pszNamespace = szNamespace;

        iFind = _FindNamespace( hwndCombo, pszNamespace, TRUE );
    }

    //  Not found in CB list? Add it if it's a valid path
    if( CB_ERR == iFind )
    {
        if( SUCCEEDED( _PathValidate( pszNamespace ) ) )
        {
            iSel = _AddAdHocNamespace( hwndCombo, pszNamespace, TRUE );
        }
        else
        {
            iSel = CB_ERR;
        }
    }
    else
    {    
        //  found in CB list? Select it.
        iSel = bAsync ?  ::PostMessage( hwndCombo, CB_SETCURSEL, iFind, 0L ): // this was needed in cases of reconcile following kill focus;
                                                                             // comboex is trying not to recurse.
                         ::SendMessage( hwndCombo, CB_SETCURSEL, iFind, 0L );
    }

    return iSel;
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::_PathFixup( LPTSTR pszDst, LPCTSTR pszSrc )
{
    ASSERT( pszDst );
    ASSERT( pszSrc );
    TCHAR szFull[MAX_PATH];

    if( _IsPathList( pszDst ) )
        return TRUE;

    *szFull = 0;
    BOOL bRelative     = PathIsRelative( pszSrc );
    BOOL bMissingDrive = bRelative ? FALSE : _IsFullPathMinusDriveLetter( pszSrc );
        // bMissingDrive =,e.g. "\foo", "\foo\bar", etc.  PathIsRelative() reports FALSE in this case.

    if( bRelative || bMissingDrive )
    {
        ASSERT( _pfsb );
        ASSERT( _pfsb->BandSite() );

        TCHAR szCurDir[MAX_PATH];
        if( SUCCEEDED( _GetCurrentPathAndNamespace( _pfsb->BandSite(), szCurDir, ARRAYSIZE(szCurDir), NULL, 0 ) ) )
        {
            if( *szCurDir && StrCmpI( szCurDir, _szCurrentPath ) )
                lstrcpy( _szCurrentPath, szCurDir );

            if( *_szCurrentPath )
            {
                if( bRelative )
                {
                    if( PathCombine( szFull, _szCurrentPath, pszSrc ) )
                        pszSrc = szFull;
                }
                else if( bMissingDrive )
                {
                    int iDrive;
                    if( (iDrive = PathGetDriveNumber( _szCurrentPath )) != -1 )
                    {
                        TCHAR szRoot[MAX_PATH];
                        if( PathCombine( szFull, PathBuildRoot( szRoot, iDrive ), pszSrc ) )
                            pszSrc = szFull;
                    }
                }
            }
        }
    }
    return PathCanonicalize( pszDst, pszSrc );
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnNamespaceDeleteItem( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
    return _DeleteNamespaceComboItem( pnmh );
}

//-------------------------------------------------------------------------//
DWORD CFindFilesDlg::NamespaceThreadProc( void* pvParam )
{
    PFSEARCHTHREADSTATE pState = (PFSEARCHTHREADSTATE)pvParam;
    ASSERT( pState );

    HRESULT hrInit = SHCoInitialize();

    if( _PopulateNamespaceCombo( pState->hwndCtl, AddNamespaceItemNotify, 
                                 (LPARAM)pvParam ) != E_ABORT )
    {
        ::PostMessage( ::GetParent( pState->hwndCtl ), 
                       WMU_COMBOPOPULATIONCOMPLETE, 
                       (WPARAM)pState->hwndCtl, 0L );
    }

    pState->fComplete = TRUE;
    SHCoUninitialize( hrInit );
    return 0L;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindFilesDlg::AddNamespaceItemNotify( ULONG fAction, PCBXITEM pItem, LPARAM lParam )
{
    PFSEARCHTHREADSTATE pState = (PFSEARCHTHREADSTATE)lParam;
    ASSERT( pState );
    
    if( fAction & CBXCB_ADDING && pState->fCancel )
        return E_ABORT;

    if( fAction & CBXCB_ADDED && CBX_CSIDL_LOCALDRIVES == pItem->iID )
    {
        CFindFilesDlg* pffd = (CFindFilesDlg*)pState->pvParam;
        ASSERT(pffd);

        lstrcpyn( pffd->_szLocalDrives, pItem->szText, ARRAYSIZE(pffd->_szLocalDrives) );
    }
        
    return S_OK;
}

//-------------------------------------------------------------------------//
LRESULT CFindFilesDlg::OnComboPopulationComplete( UINT, WPARAM wParam, LPARAM, BOOL& )
{
    _bScoped = SetDefaultScope();
    return 0L;
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::AssignNamespace( 
    LPCTSTR pszNamespace, 
    LPCTSTR pszPath, 
    BOOL bPassive /*TRUE = assign only if no current selection*/)
{
    INT_PTR iSel = CB_ERR ;
    HWND    hwndNamespace = GetDlgItem( IDC_NAMESPACE );
    
    //  If we don't yet have a current selection, establish it now.
    if( !bPassive || CB_ERR == (iSel = ::SendMessage( hwndNamespace, CB_GETCURSEL, 0, 0L )) )
    {
        if( pszPath && *pszPath ) // scan items by file system path
            iSel = _FindNamespace( hwndNamespace, pszPath, TRUE );
        
        if( CB_ERR == iSel && pszNamespace && *pszNamespace ) // scan items by display name
            iSel = _FindNamespace( hwndNamespace, pszNamespace, FALSE );

        //  Is this a folder we already know about?
        if( CB_ERR == iSel )
        {
            // no: add and select it
            if( *pszPath )
            {
                if( _IsRegItemPath( pszPath ) )
                    return FALSE;  // unsupported regitem

                if( (iSel = _AddAdHocNamespace( hwndNamespace, pszPath, TRUE )) != CB_ERR )
                    _iCurNamespace = iSel;
            }
            else
            {
                return FALSE ; // no path, nothing we can do.
            }
        }
        else
        {
            // yes: select it
            ::SendMessage( hwndNamespace, CB_SETCURSEL, iSel, 0L );
            _iCurNamespace = iSel;
        }
    }

    BOOL bRet = ::SendMessage( hwndNamespace, CB_GETCURSEL, 0, 0L ) != CB_ERR;
    return bRet;
}

//-------------------------------------------------------------------------//
HWND CFindFilesDlg::ShowHelp( HWND hwndOwner )
{
    return ::HtmlHelp( hwndOwner, FINDHELPFILE, HH_DISPLAY_TOPIC, 
                       (DWORD_PTR)FINDFILES_HELPTOPIC );
}

//-------------------------------------------------------------------------//
//  utility methods
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Reports whether a path is a namespace combo root item with 
//  child items that constitute the 'real' search paths.
BOOL CFindFilesDlg::_IsPathSingleton( IN LPCTSTR pszPath )
{
    return pszPath ? StrCmpI( pszPath, NAMESPACECOMBO_RECENT_PARAM ) !=0 : TRUE;
}

//-------------------------------------------------------------------------//
//  Appends a file system path item to the specified namespace combo box.
INT_PTR CFindFilesDlg::_AddAdHocNamespace( HWND hwndComboBox, IN LPCTSTR pszPath, BOOL bSelectItem )
{
    CBXITEM item;
    item.iItem = CB_ERR;
    if( pszPath && *pszPath )
    {
        void* pvData = NULL;
        Str_SetPtr( (LPTSTR*)&pvData, pszPath );
        if( SUCCEEDED( _MakeCbxItemKnownImage( &item, pszPath, pvData, 
                                               FOLDER_IMAGELIST_INDEX, 
                                               FOLDER_IMAGELIST_INDEX, 
                                               CB_ERR, 1 ) ) )
        {
            INT_PTR iSel = item.iItem;
            if( SUCCEEDED( _AddCbxItemToComboBox( hwndComboBox, &item, &iSel ) ) )
            {
                item.iItem = iSel;
                if( bSelectItem )
                    ::SendMessage( hwndComboBox, CB_SETCURSEL, iSel, 0L );
            }
            else
                item.iItem = CB_ERR;
        }

        if( CB_ERR == item.iItem )
            Str_SetPtr( (LPTSTR*)&pvData, NULL );    
    }
    
    return item.iItem;
}

#if _NECESSARY_BUT_PROBABLY_NOT_EVER_
//-------------------------------------------------------------------------//
INT_PTR _FindComboStringI( HWND hwndComboBox, LPCTSTR pszNamespace )
{
    if( IsWindow( hwndComboBox ) && (pszNamespace && *pszNamespace) )
    {
        TCHAR   szItem[MAX_URL_STRING];
        INT_PTR i, cnt = SendMessage( hwndComboBox, CB_GETCOUNT, 0, 0L );
        for( i=0; i<cnt; i++ )
        {
            if( SendMessage( hwndComboBox, CB_GETLBTEXT, i, (LPARAM)szItem ) != CB_ERR )
            {
                if( 0 == lstrcmpi( szItem, pszNamespace ) )
                    return i;
            }
        } 
    }
    return CB_ERR;
}
#endif

//-------------------------------------------------------------------------//
//  Scans a namespace comboboxex for the item with the indicated display name
INT_PTR CFindFilesDlg::_FindNamespace( HWND hwndComboBox, LPCTSTR pszNamespace, BOOL bForParsing )
{
    if( IsWindow( hwndComboBox ) && pszNamespace && *pszNamespace )
    {
        if( !bForParsing )
            return ::SendMessage( hwndComboBox, CB_FINDSTRINGEXACT, -1, (LPARAM)pszNamespace );

        INT_PTR i, cnt = ::SendMessage( hwndComboBox, CB_GETCOUNT, 0, 0L );

        for( i = 0; i < cnt; i++ )
        {
            LPCVOID pvData = (LPCVOID)_GetComboData( hwndComboBox, i );
            if( pvData && pvData != INVALID_HANDLE_VALUE )
                if( lstrcmpi( pszNamespace, (LPCTSTR)pvData )==0 )
                    return i;
        }
    }
    return CB_ERR;
}

//-------------------------------------------------------------------------//
INT_PTR CFindFilesDlg::_FindNamespace( IN HWND hwndComboBox, IN LPCITEMIDLIST pidl )
{
    TCHAR   szNamespace[MAX_URL_STRING];
    HRESULT hr = SHGetNameAndFlags( pidl, SHGDN_NORMAL, szNamespace, ARRAYSIZE(szNamespace), NULL );

    if( SUCCEEDED( hr ) )
        return _FindNamespace( hwndComboBox, szNamespace, FALSE );
    return CB_ERR;
}

//-------------------------------------------------------------------------//
//  _BrowseForNamespace - Invokes SHBrowseForFolder UI to select a namespace.
//
//  pszNamespace: Buffer (must be >= MAX_PATH chars) to receive shell folder display name
//
//  Returns:
//  S_OK if the user has selected a valid item and the pszNamespace contains
//  a valid shell folder display name.
//  E_ABORT if the user canceled his search
//  E_FAIL if an error occurred
STDMETHODIMP CFindFilesDlg::_BrowseForNamespace( 
    HWND hwndOwner,
    IN OUT LPTSTR pszNamespace, 
    IN UINT cchNamespace,
    OUT OPTIONAL LPBOOL pbForParsing,
    OUT OPTIONAL LPITEMIDLIST* ppidlRet )
{
    HRESULT hr = E_FAIL;

    *pszNamespace = 0;
    if( pbForParsing )
        *pbForParsing = FALSE;
    if( ppidlRet )
        *ppidlRet = NULL;

    TCHAR szTitle[MAX_PATH];

    if( EVAL( LoadString( HINST_THISDLL, IDS_SNS_BROWSERFORDIR_TITLE, szTitle, ARRAYSIZE(szTitle) ) ) )
    {
        BROWSEINFO bi = {0};
        LPITEMIDLIST pidl;

        bi.hwndOwner        = hwndOwner;
        // bi.pszDisplayName   = pszNamespace; - // If we want to display a friendly name then assign this value
        bi.pidlRoot         = NULL;
        bi.lpszTitle        = szTitle;
        bi.ulFlags          = (BIF_USENEWUI | BIF_EDITBOX | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS);
        bi.lpfn             = _BrowseCallback;
        bi.lParam           = (LPARAM)hwndOwner;

        pidl = SHBrowseForFolder( &bi );

        if( pidl )
        {
            hr = SHGetTargetFolderPath( pidl, pszNamespace, cchNamespace );
            if( SUCCEEDED(hr) && *pszNamespace )
            {
                if( pbForParsing )
                    *pbForParsing = TRUE;
            }
            else
            {
                IShellFolder* psfDesktop;
                if( SUCCEEDED( SHGetDesktopFolder( &psfDesktop ) ) )
                {
                    STRRET        strret;
                    hr = psfDesktop->GetDisplayNameOf( pidl, SHGDN_NORMAL, &strret );
                    if( SUCCEEDED( hr ) )
                    {
                        hr = StrRetToBuf( &strret, pidl,  pszNamespace, MAX_PATH );
                    }
                    psfDesktop->Release();
                }
            }

            if( ppidlRet )
                *ppidlRet = pidl;
            else
                ILFree(pidl);
        }
        else
            hr = E_ABORT;
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Invokes SHBrowserForFolder UI and assigns results.
void CFindFilesDlg::_BrowseAndAssignNamespace()
{
    TCHAR        szPath[MAX_PATH];
    BOOL         bForParsing;
    LPITEMIDLIST pidl = NULL;

    //  Present folder browse UI..
    if( SUCCEEDED( _BrowseForNamespace( m_hWnd, szPath, ARRAYSIZE(szPath), &bForParsing, &pidl ) ) )
    {
        INT_PTR iSel = _FindNamespace( GetDlgItem( IDC_NAMESPACE ), szPath, bForParsing );
        if( iSel != CB_ERR )
        {
            SendDlgItemMessage( IDC_NAMESPACE, CB_SETCURSEL, iSel, 0L );
            _iCurNamespace = iSel;
        }
        else
        {
            if( (iSel = _AddAdHocNamespace( GetDlgItem( IDC_NAMESPACE ), szPath, TRUE )) != CB_ERR )
                _iCurNamespace = iSel;
        }
    }
    else
        SendDlgItemMessage( IDC_NAMESPACE, CB_SETCURSEL, _iCurNamespace, 0L );
    
    if( pidl )
        ILFree( pidl );
}

//-------------------------------------------------------------------------//
BOOL CFindFilesDlg::_IsSearchableFolder( 
    IN LPCITEMIDLIST pidlFolder, 
    IN OPTIONAL HWND hwndOwner )
{
    BOOL    bSearchable = FALSE;
    TCHAR szFolder[MAX_PATH];

    //  Read through any folder shortcut to the target path.
    HRESULT hr = SHGetTargetFolderPath( pidlFolder, szFolder, ARRAYSIZE(szFolder) );
    if( SUCCEEDED(hr) )
    {
        bSearchable = TRUE; // Ah, a file system folder.
    }
    else if( IsWindow( hwndOwner ) )
    {
        SHGetNameAndFlags(pidlFolder, SHGDN_NORMAL, szFolder, SIZECHARS(szFolder), NULL);
        bSearchable = (CB_ERR != _FindNamespace( ::GetDlgItem( hwndOwner, IDC_NAMESPACE ), szFolder, FALSE ));
    }

    return bSearchable;
}

//-------------------------------------------------------------------------//
int CFindFilesDlg::_BrowseCallback( HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData )
{
    HWND hwndOwner = (HWND)lpData;
    ASSERT( IsWindow( hwndOwner ) );

    switch( msg )
    {
        case BFFM_INITIALIZED:  // initializing: set default selection
        {
            LPITEMIDLIST pidlDefault = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, TRUE);
            if( pidlDefault )
            {
                ::SendMessage( hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)pidlDefault );
                ILFree( pidlDefault );
            }
        }
        break;
    
        case BFFM_SELCHANGED:   // prevent non-searchable folder pidls from being selected.
        {
            BOOL bAllow = _IsSearchableFolder( (LPCITEMIDLIST)lParam, hwndOwner );
            ::SendMessage( hwnd, BFFM_ENABLEOK, 0, (LPARAM)bAllow );

        }
        break;
    }

    return 0;
}

//-------------------------------------------------------------------------//
class CSearchWarningDlg
//-------------------------------------------------------------------------//
{
private:    
    CSearchWarningDlg() : 
        _hwnd(NULL), _bNoWarn(FALSE) {}
    static BOOL_PTR WINAPI DlgProc( HWND, UINT, WPARAM, LPARAM );

    HWND    _hwnd;
    BOOL    _bNoWarn;

    friend int CSearchWarningDlg_DoModal( HWND hwndParent, USHORT uDlgT, BOOL* pbNoWarn );
};

//-------------------------------------------------------------------------//
int CSearchWarningDlg_DoModal( HWND hwndParent, USHORT uDlgTemplate, BOOL* pbNoWarn )
{
    ASSERT(pbNoWarn);

    CSearchWarningDlg dlg;
    dlg._bNoWarn = *pbNoWarn;
    int nRet = (int)DialogBoxParam( HINST_THISDLL, MAKEINTRESOURCE(uDlgTemplate),
                                    hwndParent, CSearchWarningDlg::DlgProc, (LPARAM)&dlg );    
    *pbNoWarn = dlg._bNoWarn;
    return nRet;
}

//-------------------------------------------------------------------------//
BOOL_PTR WINAPI CSearchWarningDlg::DlgProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CSearchWarningDlg* pdlg = NULL;

    if( WM_INITDIALOG == uMsg )
    {
        pdlg = (CSearchWarningDlg*)lParam;
        pdlg->_hwnd = hwnd;
        SetWindowPtr( hwnd, GWLP_USERDATA, pdlg );

        CheckDlgButton( hwnd, IDC_NOSCOPEWARNING, pdlg->_bNoWarn );
        MessageBeep( MB_ICONASTERISK );
        return TRUE;
    }
    else
        pdlg = (CSearchWarningDlg*)GetWindowPtr( hwnd, GWLP_USERDATA );

    if( NULL == pdlg )
        return FALSE;

    switch( uMsg )
    {
        case WM_COMMAND:
        {
            WORD wID         = LOWORD(wParam);
            BOOL bHandled    = TRUE ;

            switch( wID )
            {
                case IDOK:
                case IDCANCEL:
                    pdlg->_bNoWarn = IsDlgButtonChecked( hwnd, IDC_NOSCOPEWARNING );
                    EndDialog( hwnd, wID );
                    break ;
                default:
                    bHandled = FALSE;
            }
            return bHandled;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
class CCISettingsDlg
//-------------------------------------------------------------------------//
{
public:
    CCISettingsDlg() : _hwnd(NULL), _fHeapInst(FALSE),
                      _fCiIndexed(FALSE), _fCiRunning(FALSE), _fCiPermission(FALSE)
    {
        InitMMCHandles();
    }

    ~CCISettingsDlg()   {
        CloseMMCHandles();
    }

    static int  DoModal( HWND hwndParent );
    static HWND CreateModeless( HWND hwndParent );


protected:
    BOOL OnInitDialog();
    BOOL OnOK();

private:    
    static BOOL_PTR WINAPI DlgProc( HWND, UINT, WPARAM, LPARAM );

    void ShowAdvanced();
    void InitMMCHandles();
    void CloseMMCHandles();

    HWND    _hwnd;
    BOOL    _fHeapInst,
            _fCiIndexed,
            _fCiRunning,
            _fCiPermission;
    PROCESS_INFORMATION _mmc_process ;

    friend int  CCISettingsDlg_DoModal( HWND hwndParent );
    friend HWND CCISettingsDlg_CreateModeless( HWND hwndParent );
};

//-------------------------------------------------------------------------//
int CCISettingsDlg_DoModal( HWND hwndParent )
{
    CCISettingsDlg dlg;
    return (int)DialogBoxParam( HINST_THISDLL, MAKEINTRESOURCE(DLG_INDEXSERVER),
                           hwndParent, CCISettingsDlg::DlgProc, (LPARAM)&dlg );    
}

//-------------------------------------------------------------------------//
HWND CCISettingsDlg_CreateModeless( HWND hwndParent )
{
    HWND            hdlg = NULL;
    CCISettingsDlg* pdlg;

    if( (pdlg = new CCISettingsDlg) != NULL )
    {
        pdlg->_fHeapInst = TRUE;
        if( NULL == (hdlg = CreateDialogParam( 
                HINST_THISDLL, MAKEINTRESOURCE(DLG_INDEXSERVER),
                hwndParent, CCISettingsDlg::DlgProc, (LPARAM)&pdlg )) )
            delete pdlg;
    }
    return hdlg;
}

//-------------------------------------------------------------------------//
BOOL_PTR WINAPI CCISettingsDlg::DlgProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    CCISettingsDlg* pdlg = NULL;

    if( WM_INITDIALOG == nMsg )
    {
        pdlg = (CCISettingsDlg*)lParam;
        pdlg->_hwnd = hDlg;
        SetWindowPtr( hDlg, GWLP_USERDATA, pdlg );
        return pdlg->OnInitDialog();
    }
    else
        pdlg = (CCISettingsDlg*)GetWindowPtr( hDlg, GWLP_USERDATA );

    if( NULL == pdlg )
        return FALSE;

    switch( nMsg )
    {

        case WM_NCDESTROY:
            if( pdlg->_fHeapInst )
                delete pdlg;
            return TRUE;

        case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam);
            WORD wID         = LOWORD(wParam);
            HWND hwndCtl     = (HWND)lParam;
            BOOL bHandled    = TRUE ;

            switch( wID )
            {
                case IDC_CI_ADVANCED:
                {
                    pdlg->ShowAdvanced();
                    break ;
                }

                case IDOK:
                    if( pdlg->OnOK() )
                        EndDialog( hDlg, IDOK );
                    break ;

                case IDCANCEL:
                    EndDialog( hDlg, wID );
                    break ;

                case IDC_CI_HELP:
                    _IndexServiceHelp( hDlg );
                    break;

                default:
                    bHandled = FALSE;
            }

            return bHandled;
        }
    }

    return FALSE;
}

//-------------------------------------------------------------------------//
void CCISettingsDlg::ShowAdvanced()
{
    //  have we already spawned MMC?
    if( _mmc_process.hProcess != INVALID_HANDLE_VALUE )
    {
        HANDLE hProcess;
        if( (hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, 
                                    _mmc_process.dwProcessId )) != NULL )
        {
            //  it's alive.  bail
            CloseHandle( hProcess );
            return ;
        }
        InitMMCHandles();
    }

    //  Light up MMC w/ Index Service snap-in.
    static LPCTSTR szCmdLine0 = 
        TEXT("mmc.exe %systemroot%\\system32\\ciadv.msc computername=localmachine");
    TCHAR szCmdLine[MAX_PATH];
    if (SHExpandEnvironmentStrings(szCmdLine0, szCmdLine, ARRAYSIZE(szCmdLine)))
    {
        STARTUPINFO si = {0};
        si.cb = sizeof (si);
        if( !CreateProcess( NULL, szCmdLine, NULL, NULL, FALSE, 
                           0, NULL, NULL, &si, &_mmc_process ) )
            InitMMCHandles();
    }
}

//-------------------------------------------------------------------------//
void CCISettingsDlg::InitMMCHandles()
{
    _mmc_process.dwProcessId = 
    _mmc_process.dwThreadId  = 0;
    _mmc_process.hProcess    = 
    _mmc_process.hThread     = INVALID_HANDLE_VALUE;
}

//-------------------------------------------------------------------------//
void CCISettingsDlg::CloseMMCHandles()
{
    if( _mmc_process.hProcess != INVALID_HANDLE_VALUE )
        CloseHandle( _mmc_process.hProcess );
    if( _mmc_process.hThread != INVALID_HANDLE_VALUE )
        CloseHandle( _mmc_process.hThread );

    _mmc_process.hProcess = _mmc_process.hThread = INVALID_HANDLE_VALUE;
}

//-------------------------------------------------------------------------//
BOOL CCISettingsDlg::OnInitDialog() 
{ 
    TCHAR szStatusFmt[128], 
          szStatusText[MAX_PATH];
    UINT  nStatusText = IDS_FSEARCH_CI_DISABLED;

    GetCIStatus( &_fCiRunning, &_fCiIndexed, &_fCiPermission );
    
    if( _fCiRunning )
    {
        if( _fCiPermission )
            //  permission to distinguish between ready, busy.
            nStatusText = _fCiIndexed ? IDS_FSEARCH_CI_READY : IDS_FSEARCH_CI_BUSY;
        else
            //  no permission to distinguish between ready, busy; just say it's enabled.
            nStatusText = IDS_FSEARCH_CI_ENABLED;
    }

    if( LoadString( HINST_THISDLL, IDS_FSEARCH_CI_STATUSFMT, szStatusFmt, ARRAYSIZE(szStatusFmt) ))
    {
        if( LoadString( HINST_THISDLL, nStatusText, szStatusText, ARRAYSIZE(szStatusText) ))
        {
            TCHAR szStatus[MAX_PATH];
            wnsprintf(szStatus, ARRAYSIZE(szStatus), szStatusFmt, szStatusText);
            SetDlgItemText( _hwnd, IDC_CI_STATUS, szStatus );
        }
    }

    CheckDlgButton( _hwnd, IDC_ENABLE_CI,   _fCiRunning );
    CheckDlgButton( _hwnd, IDC_BLOWOFF_CI, !_fCiRunning );

    EnableWindow( GetDlgItem( _hwnd, IDC_CI_PROMPT ),  _fCiPermission );
    EnableWindow( GetDlgItem( _hwnd, IDC_ENABLE_CI),   _fCiPermission );
    EnableWindow( GetDlgItem( _hwnd, IDC_BLOWOFF_CI),  _fCiPermission );

    return TRUE; 
}

//-------------------------------------------------------------------------//
BOOL CCISettingsDlg::OnOK()
{
    BOOL fStart = IsDlgButtonChecked( _hwnd, IDC_ENABLE_CI ) ? TRUE : FALSE;

    //if( fStart != _fCiRunning )
        StartStopCI( fStart, TRUE );

    return TRUE;
}

#ifdef __PSEARCH_BANDDLG__
//-------------------------------------------------------------------------//
//  CFindPrintersDlg impl
//-------------------------------------------------------------------------//
#define PSEARCHDLG_TABFIRST   IDC_PSEARCH_NAME
#define PSEARCHDLG_TABLAST    IDC_SEARCHLINK_INTERNET
#define PSEARCHDLG_RIGHTMOST   IDC_SEARCH_START
#define PSEARCHDLG_BOTTOMMOST  IDC_SEARCHLINK_INTERNET

//-------------------------------------------------------------------------//
CFindPrintersDlg::CFindPrintersDlg( CFileSearchBand* pfsb )
    :   CBandDlg( pfsb )
{
}

//-------------------------------------------------------------------------//
CFindPrintersDlg::~CFindPrintersDlg()
{

}

//-------------------------------------------------------------------------//
LRESULT CFindPrintersDlg::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    _Attach( m_hWnd );
    ASSERT( Hwnd() );

    CMetrics&   metrics = _pfsb->GetMetrics();
    _pfsb->GetMetrics().Init( m_hWnd );

    EVAL( LinkWindow_RegisterClass() );
    POINT pt;
    pt.x = metrics.CtlMarginX();
    pt.y = 0;
    _CreateSearchLinks( m_hWnd, pt, IDC_SEARCHLINK_PRINTERS );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV1, pt, 2, GetDlgItem( IDC_PSEARCH_CAPTION ) );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV2, pt, 1, GetDlgItem( IDC_SEARCHLINK_CAPTION ) );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV3, pt, 1, GetDlgItem( IDC_SEARCHLINK_PEOPLE ) );

    OnWinIniChange();
    LayoutControls( -1, -1 );

    return TRUE;
}

//-------------------------------------------------------------------------//
void CFindPrintersDlg::LayoutControls( int cx, int cy )
{
    if( cx < 0 || cy < 0 )
    {
        RECT rc;
        GetClientRect( &rc );
        cx = RECTWIDTH( &rc );
        cy = RECTHEIGHT( &rc );
    }

    CBandDlg::LayoutControls( cx, cy );
    CMetrics& metrics = _pfsb->GetMetrics();

    const UINT nIDCtl[] = {
        IDC_PSEARCH_NAME,
        IDC_PSEARCH_LOCATION,
        IDC_PSEARCH_MODEL,
    };
    RECT rcCtl[ARRAYSIZE(nIDCtl)];
    
    //  Stretch edit boxes to fit horz
    for( int i = 0; i< ARRAYSIZE(nIDCtl); i++ )
    {
        HWND hwndCtl = GetDlgItem( nIDCtl[i] );
        if( hwndCtl && ::GetWindowRect( hwndCtl, &rcCtl[i] ) )
        {
            ::MapWindowPoints( HWND_DESKTOP, Hwnd(), (LPPOINT)&rcCtl[i], POINTSPERRECT );
            rcCtl[i].right = cx - metrics.CtlMarginX();
            ::SetWindowPos( hwndCtl, NULL, 0, 0, 
                          RECTWIDTH(rcCtl+i), RECTHEIGHT(rcCtl+i),
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
        }
        else
            SetRectEmpty( rcCtl + i );    
    }

    //  Position the 'Search for Other Items' caption, divider and link windows
    const int rgLinks[] = {
        IDC_SEARCHLINK_FILES,
        IDC_SEARCHLINK_COMPUTERS,
        IDC_SEARCHLINK_PRINTERS,
        IDC_SEARCHLINK_PEOPLE,
        -IDC_FSEARCH_DIV3,
        IDC_SEARCHLINK_INTERNET,
    };

    RECT rc;
    ::GetWindowRect( GetDlgItem( IDC_SEARCH_START ), &rc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    rc.bottom += metrics.LooseMarginY();

    _LayoutSearchLinks( IDC_SEARCHLINK_CAPTION, IDC_FSEARCH_DIV2, TRUE,
                        metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                        rc.bottom, rgLinks, ARRAYSIZE(rgLinks) );
}

//-------------------------------------------------------------------------//
BOOL CFindPrintersDlg::Validate()
{
    return TRUE;
}

//-------------------------------------------------------------------------//
void CFindPrintersDlg::Clear()
{
    SetDlgItemText( IDC_PSEARCH_NAME, NULL );
    SetDlgItemText( IDC_PSEARCH_LOCATION, NULL );
    SetDlgItemText( IDC_PSEARCH_MODEL, NULL );
}

//-------------------------------------------------------------------------//
BOOL CFindPrintersDlg::GetMinSize( HWND hwndOC, LPSIZE pSize ) const
{
    RECT rcRightmost, rcBottommost;
    HWND hwndRightmost = GetDlgItem( PSEARCHDLG_RIGHTMOST ), 
         hwndBottommost= GetDlgItem( PSEARCHDLG_BOTTOMMOST );
    
    ASSERT( IsWindow( hwndRightmost ) );
    ASSERT( IsWindow( hwndBottommost ) );

    ::GetWindowRect( hwndRightmost, &rcRightmost );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcRightmost, POINTSPERRECT );

    ::GetWindowRect( hwndBottommost, &rcBottommost );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcBottommost, POINTSPERRECT );

    pSize->cx = rcRightmost.right;
    pSize->cy = rcBottommost.bottom + _pfsb->GetMetrics().TightMarginY();

    return TRUE;
}

//-------------------------------------------------------------------------//
HWND CFindPrintersDlg::GetFirstTabItem() const
{
    return GetDlgItem( PSEARCHDLG_TABFIRST );
}

//-------------------------------------------------------------------------//
HWND CFindPrintersDlg::GetLastTabItem() const
{
    return GetDlgItem( PSEARCHDLG_TABLAST );
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindPrintersDlg::TranslateAccelerator( LPMSG lpmsg )
{
    if( S_OK == CBandDlg::TranslateAccelerator( lpmsg ) )
        return S_OK;

    //  Handle it ourselves...
    return _pfsb->IsDlgMessage( m_hWnd, lpmsg );
}

//-------------------------------------------------------------------------//
void CFindPrintersDlg::OnWinIniChange()
{
    _BeautifyCaption( IDC_PSEARCH_CAPTION, IDC_PSEARCH_ICON, IDI_PSEARCH );
}

//-------------------------------------------------------------------------//
LRESULT CFindPrintersDlg::OnSearchStartBtn( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    WCHAR wszName[MAX_PATH],
          wszLocation[MAX_PATH],
          wszModel[MAX_PATH];

    ::GetDlgItemTextW( m_hWnd, IDC_PSEARCH_NAME, wszName, ARRAYSIZE(wszName) );
    ::GetDlgItemTextW( m_hWnd, IDC_PSEARCH_LOCATION, wszLocation, ARRAYSIZE(wszLocation) );
    ::GetDlgItemTextW( m_hWnd, IDC_PSEARCH_MODEL, wszModel, ARRAYSIZE(wszModel) );

    ASSERT( _pfsb );
    ASSERT( _pfsb->BandSite() );

    IShellDispatch2* psd2;
    if( SUCCEEDED( CoCreateInstance( CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                                     IID_IShellDispatch2, (void**)&psd2 ) ) )
    {
        BSTR bstrName     = *wszName ? SysAllocString( wszName ) : NULL,
             bstrLocation = *wszLocation ? SysAllocString( wszLocation ) : NULL, 
             bstrModel    = *wszModel ? SysAllocString( wszModel ) : NULL;

        if( FAILED( psd2->FindPrinter( bstrName, bstrLocation, bstrModel ) ) )
        {
            SysFreeString( bstrName );
            SysFreeString( bstrLocation );
            SysFreeString( bstrModel );
        }
        
        psd2->Release();
    }
    
    return 0L;
}
#endif __PSEARCH_BANDDLG__


//-------------------------------------------------------------------------//
//  CFindComputersDlg impl
//-------------------------------------------------------------------------//
#define CSEARCHDLG_TABFIRST   IDC_CSEARCH_NAME
#define CSEARCHDLG_TABLAST    IDC_SEARCHLINK_INTERNET
#define CSEARCHDLG_RIGHTMOST   IDC_SEARCH_STOP
#define CSEARCHDLG_BOTTOMMOST  IDC_SEARCHLINK_INTERNET

//-------------------------------------------------------------------------//
CFindComputersDlg::CFindComputersDlg( CFileSearchBand* pfsb )
    :   CSearchCmdDlg( pfsb ),
        _pacComputerName(NULL),
        _pmruComputerName(NULL)
{

}

//-------------------------------------------------------------------------//
CFindComputersDlg::~CFindComputersDlg()
{
    ATOMICRELEASE(_pacComputerName);
    ATOMICRELEASE(_pmruComputerName);
}

//-------------------------------------------------------------------------//
LRESULT CFindComputersDlg::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    _Attach( m_hWnd );
    ASSERT( Hwnd() );

    CMetrics&   metrics = _pfsb->GetMetrics();
    _pfsb->GetMetrics().Init( m_hWnd );

    //  Register specialty window classes.
    EVAL( DivWindow_RegisterClass() );
    EVAL( LinkWindow_RegisterClass() );

    POINT pt;
    pt.x = metrics.CtlMarginX();
    pt.y = 0;
    _CreateSearchLinks( m_hWnd, pt, IDC_SEARCHLINK_COMPUTERS );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV1, pt, 2, GetDlgItem( IDC_CSEARCH_CAPTION ) );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV2, pt, 1, GetDlgItem( IDC_SEARCHLINK_CAPTION ) );
    _CreateDivider( m_hWnd, IDC_FSEARCH_DIV3, pt, 1, GetDlgItem( IDC_SEARCHLINK_PEOPLE ) );

    _InitializeMru( GetDlgItem(IDC_CSEARCH_NAME), &_pacComputerName, 
                    TEXT("ComputerNameMRU"), &_pmruComputerName );
    SendDlgItemMessage( IDC_CSEARCH_NAME, EM_LIMITTEXT, MAX_PATH, 0L );

    OnWinIniChange();
    LayoutControls( -1, -1 );

    return TRUE;
}

LRESULT CFindComputersDlg::OnDestroy( UINT, WPARAM, LPARAM, BOOL& bHandled )
{
    StopSearch();
    if (_pSrchCmd)
    {
        DisconnectEvents();
        IUnknown_SetSite(_pSrchCmd, NULL);
    }
    bHandled = FALSE;
    _fOnDestroy = TRUE;
    return 0L;
}


//-------------------------------------------------------------------------//
void CFindComputersDlg::LayoutControls( int cx, int cy )
{
    if( cx < 0 || cy < 0 )
    {
        RECT rc;
        GetClientRect( &rc );
        cx = RECTWIDTH( &rc );
        cy = RECTHEIGHT( &rc );
    }
    CBandDlg::LayoutControls( cx, cy );

    const UINT nIDCtl[] = {
        IDC_CSEARCH_NAME,
    };
    RECT rcCtl[ARRAYSIZE(nIDCtl)];

    CMetrics& metrics = _pfsb->GetMetrics();
    for( int i = 0; i< ARRAYSIZE(nIDCtl); i++ )
    {
        HWND hwndCtl = GetDlgItem( nIDCtl[i] );
        if( hwndCtl && ::GetWindowRect( hwndCtl, &rcCtl[i] ) )
        {
            ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcCtl[i], POINTSPERRECT );
            rcCtl[i].right = cx - metrics.CtlMarginX();
            ::SetWindowPos( hwndCtl, NULL, 0, 0, 
                          RECTWIDTH(rcCtl+i), RECTHEIGHT(rcCtl+i),
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE );
        }
        else
            SetRectEmpty( rcCtl + i );    
    }

    //  Position the 'Search for Other Items' caption, divider and link windows

    const int rgLinks[] = {
        IDC_SEARCHLINK_FILES,
        IDC_SEARCHLINK_COMPUTERS,
        IDC_SEARCHLINK_PRINTERS,
        IDC_SEARCHLINK_PEOPLE,
        -IDC_FSEARCH_DIV3,
        IDC_SEARCHLINK_INTERNET,
    };

    RECT rc;
    ::GetWindowRect( GetDlgItem( IDC_SEARCH_START ), &rc );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rc, POINTSPERRECT );
    rc.bottom += metrics.LooseMarginY();

    _LayoutSearchLinks( IDC_SEARCHLINK_CAPTION, IDC_FSEARCH_DIV2, TRUE,
                        metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                        rc.bottom, rgLinks, ARRAYSIZE(rgLinks) );
}

//-------------------------------------------------------------------------//
BOOL CFindComputersDlg::Validate()
{
    return TRUE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindComputersDlg::AddConstraints( ISearchCommandExt* pSrchCmd )
{
    HRESULT hr = E_FAIL;
    TCHAR   szName[MAX_PATH];
    if(::GetDlgItemText(m_hWnd, IDC_CSEARCH_NAME, szName, MAX_PATH) <= 0)
    {
        lstrcpy(szName, TEXT("*"));
    }

    VARIANT var;
    VariantInit( &var );
    if(SUCCEEDED(hr = _T2BstrVariant(szName, &var)))
    {
        if(SUCCEEDED((hr = _AddConstraint( pSrchCmd, GetConstraintName(FSBC_SEARCHFOR), &var))))
        {
            _AddMruStringFromWindow( _pmruComputerName, GetDlgItem( IDC_CSEARCH_NAME ) );
        }
        else
        {
            VariantClear(&var);
        }
    }

    return hr;
}

//-------------------------------------------------------------------------//
void CFindComputersDlg::UpdateStatusText()
{
    CSearchCmdDlg::UpdateStatusText();
}

//-------------------------------------------------------------------------//
void CFindComputersDlg::RestoreSearch()
{
    CSearchCmdDlg::RestoreSearch();
}

//-------------------------------------------------------------------------//
void CFindComputersDlg::Clear()
{
    CSearchCmdDlg::Clear();
    SetDlgItemText( IDC_CSEARCH_NAME, NULL );
}

//-------------------------------------------------------------------------//
BOOL CFindComputersDlg::GetMinSize( HWND hwndOC, LPSIZE pSize ) const
{
    RECT rcRightmost, rcBottommost;
    HWND hwndRightmost = GetDlgItem( CSEARCHDLG_RIGHTMOST ), 
         hwndBottommost= GetDlgItem( CSEARCHDLG_BOTTOMMOST );
    
    ASSERT( IsWindow( hwndRightmost ) );
    ASSERT( IsWindow( hwndBottommost ) );

    ::GetWindowRect( hwndRightmost, &rcRightmost );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcRightmost, POINTSPERRECT );

    ::GetWindowRect( hwndBottommost, &rcBottommost );
    ::MapWindowPoints( HWND_DESKTOP, m_hWnd, (LPPOINT)&rcBottommost, POINTSPERRECT );

    pSize->cx = rcRightmost.right;
    pSize->cy = rcBottommost.bottom + _pfsb->GetMetrics().TightMarginY();

    return TRUE;
}

//-------------------------------------------------------------------------//
void CFindComputersDlg::NavigateToResults( IWebBrowser2* pwb2 )
{
    BSTR    bstrUrl = SysAllocString( L"::{1f4de370-d627-11d1-ba4f-00a0c91eedba}" );
    VARIANT varNil;

    VariantInit( &varNil );
    pwb2->Navigate( bstrUrl, &varNil, &varNil, &varNil, &varNil );
    SysFreeString( bstrUrl );
}

//-------------------------------------------------------------------------//
HWND CFindComputersDlg::GetFirstTabItem() const
{
    return GetDlgItem( CSEARCHDLG_TABFIRST );
}

//-------------------------------------------------------------------------//
HWND CFindComputersDlg::GetLastTabItem() const
{
    return GetDlgItem( CSEARCHDLG_TABLAST );
}

//-------------------------------------------------------------------------//
BOOL CFindComputersDlg::GetAutoCompleteObjectForWindow( HWND hwnd, IAutoComplete2** ppac2 )
{
    if( hwnd == GetDlgItem( IDC_CSEARCH_NAME ) )
    {
        *ppac2 = _pacComputerName;
        (*ppac2)->AddRef();
        return TRUE;
    }
    return CBandDlg::GetAutoCompleteObjectForWindow( hwnd, ppac2 );
}

//-------------------------------------------------------------------------//
STDMETHODIMP CFindComputersDlg::TranslateAccelerator( LPMSG lpmsg )
{
    if( S_OK == CSearchCmdDlg::TranslateAccelerator( lpmsg ) )
        return S_OK;

    //  Handle it ourselves...
    return _pfsb->IsDlgMessage( m_hWnd, lpmsg );
}

//-------------------------------------------------------------------------//
void CFindComputersDlg::OnWinIniChange()
{
    //  redisplay animated icon
    HWND hwndIcon = GetDlgItem( IDC_CSEARCH_ICON );
    Animate_Close( hwndIcon );
    Animate_OpenEx( hwndIcon, HINST_THISDLL, MAKEINTRESOURCE(IDA_FINDCOMP) );

    _BeautifyCaption( IDC_CSEARCH_CAPTION );
}

//-------------------------------------------------------------------------//
LRESULT CFindComputersDlg::OnSearchStartBtn( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    EnableStartStopButton( hwndCtl, FALSE );
    StartStopAnimation( TRUE );

    if( FAILED( StartSearch() ) )
    {
        EnableStartStopButton( hwndCtl, TRUE );
        StartStopAnimation( FALSE );
    }

    return 0L;
}

//-------------------------------------------------------------------------//
LRESULT CFindComputersDlg::OnSearchStopBtn( WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    StopSearch();
    return 0L;
}

//-------------------------------------------------------------------------//
HWND CFindComputersDlg::ShowHelp( HWND hwndOwner )
{
    return ::HtmlHelp( hwndOwner, FINDHELPFILE, HH_DISPLAY_TOPIC, 
                       (DWORD_PTR)FINDCOMPUTER_HELPTOPIC );
}


//-------------------------------------------------------------------------//
//  CSearchCmdDlg object wrap and event sink
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CSearchCmdDlg::CSearchCmdDlg( CFileSearchBand* pfsb )
    :   CBandDlg( pfsb ),
        _pSrchCmd(NULL), 
        _pcp(NULL), 
        _dwConnection(0)
{
    ASSERT( pfsb );
}

//-------------------------------------------------------------------------//
CSearchCmdDlg::~CSearchCmdDlg()
{ 
    DisconnectEvents(); 
    if( _pSrchCmd )
    {
        _pSrchCmd->Release();
        _pSrchCmd = NULL;
    }
}

//-------------------------------------------------------------------------//
ISearchCommandExt* CSearchCmdDlg::GetSearchCmd()
{
    if ( _fOnDestroy )
        return NULL;
        
    ASSERT( _pfsb->BandSite() != NULL );

    //  Instantiate docfind command object
    if( NULL == _pSrchCmd )
    {
        ISearchCommandExt* pSrchCmd;
        DWORD              dwConnection = 0;

        if( FAILED( CoCreateInstance( CLSID_DocFindCommand, NULL, CLSCTX_INPROC_SERVER,
                                      IID_ISearchCommandExt, (void**)&pSrchCmd ) ) )
        {
            ASSERT( FALSE ); // error message
            return NULL;
        }

        //  Assign search type.
        if( FAILED( pSrchCmd->SearchFor( GetSearchType() ) ) )
        {
            pSrchCmd->Release();
            return NULL;
        }

        //  DocFindCommand refuses to work unless he's got a site.
        if( FAILED(IUnknown_SetSite( pSrchCmd, _pfsb->BandSite() ) ) )
        {
            pSrchCmd->Release();
            return NULL;
        }

        //  Connect events.
        if( FAILED( ConnectEvents( pSrchCmd ) ) )
        {
            IUnknown_SetSite( pSrchCmd, NULL ) ;
            pSrchCmd->Release();
            return NULL;
        }

        _pSrchCmd = pSrchCmd;
    }

    return _pSrchCmd;
}

//-------------------------------------------------------------------------//
HRESULT CSearchCmdDlg::ConnectEvents( IUnknown* punk )
{
    HRESULT hr = E_FAIL;
    IConnectionPointContainer* pcpc = NULL;
    if( SUCCEEDED( (hr = punk->QueryInterface( IID_IConnectionPointContainer, (void**)&pcpc )) ) )
    {
        IConnectionPoint* pcp = NULL;
        DWORD dwConnection = 0L;

        if( SUCCEEDED( (hr = pcpc->FindConnectionPoint( DIID_DSearchCommandEvents, &pcp )) ) )
        {
            if( SUCCEEDED( (hr = pcp->Advise( this, &dwConnection )) ) )
            {  
                _pcp = pcp;
                _pcp->AddRef();
                _dwConnection = dwConnection;
            }
            pcp->Release();
        }
        pcpc->Release();
    }
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT CSearchCmdDlg::DisconnectEvents()
{
    HRESULT hr = S_FALSE;
    if( _pcp )
    {
        _pcp->Unadvise( _dwConnection );
        _pcp->Release();
        _pcp = NULL;
        _dwConnection = 0L;
        hr = S_OK;
    }
    return hr;
}

//-------------------------------------------------------------------------//
HRESULT CSearchCmdDlg::StartSearch()
{
    HRESULT hr = S_OK;

    //  Validate input
    if( !Validate() )
        return E_INVALIDARG;

    ISearchCommandExt* pSrchCmd;
    if( NULL == (pSrchCmd = GetSearchCmd()) )
    {
        ASSERT( pSrchCmd );
        return E_FAIL;
    }

    //  Clear off current results
    pSrchCmd->ClearResults();

    if( FAILED( (hr = AddConstraints( pSrchCmd )) ) )
        return hr;

    return Execute(TRUE);
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::StartStopAnimation( BOOL bStart )
{
    HWND hwndAnimate = GetAnimation();
    if( IsWindow( hwndAnimate ) )
    {
        if( bStart )
            Animate_Play( hwndAnimate, 0, -1, -1 );
        else
            Animate_Stop( hwndAnimate );
    }
}

//-------------------------------------------------------------------------//
//  WMU_RESTORESEARCH handler
LRESULT CSearchCmdDlg::OnRestoreSearch( UINT, WPARAM, LPARAM, BOOL& )
{
    //  We've posted ourselves this message in response to the event 
    //  dispatch because we want to do our restoration on the
    //  band's primary thread rather than the OLE dispatch thread.   
    //  To do the work on the dispatch thread results in a premature 
    //  abort of the search restoration processing as the dispatch
    //  thread terminates.
    RestoreSearch();
    return 0L;
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::Clear()
{
    StopSearch();
    
    ISearchCommandExt* pSrchCmd;
    if( NULL == (pSrchCmd = GetSearchCmd()) )
    {
        ASSERT( pSrchCmd );
        return;
    }

    pSrchCmd->ClearResults();
}

//-------------------------------------------------------------------------//
HRESULT CSearchCmdDlg::Execute(BOOL bStart)
{
    ASSERT( _pSrchCmd );
    
    VARIANT varRecordsAffected,
            varParams;
    LONG    lOptions = bStart? 1L : 0L;

    VariantInit( &varRecordsAffected );
    VariantInit( &varParams );

    return _pSrchCmd->Execute( &varRecordsAffected, &varParams, lOptions );
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::StopSearch()
{
    if( SearchInProgress() )
        Execute(FALSE);
}

//-------------------------------------------------------------------------//
HRESULT CSearchCmdDlg::SetQueryFile( IN VARIANT* pvarFile )
{
    HRESULT hr = CBandDlg::SetQueryFile( pvarFile );
    if( hr != S_OK )
        return hr;

    ISearchCommandExt* pSrchCmd;
    if( NULL == (pSrchCmd = GetSearchCmd()) )
    {
        ASSERT( pSrchCmd );
        return E_FAIL;
    }
    return pSrchCmd->RestoreSavedSearch( pvarFile );
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::UpdateSearchCmdStateUI( DISPID eventID )
{
    if ( _fOnDestroy )
        return;
        
    BOOL bStopEvent = (DISPID_SEARCHCOMMAND_COMPLETE == eventID || 
                       DISPID_SEARCHCOMMAND_ERROR == eventID ||
                       DISPID_SEARCHCOMMAND_ABORT == eventID),
         bStartEvent = DISPID_SEARCHCOMMAND_START == eventID;
    
    HWND hwndStart = GetDlgItem( Hwnd(), IDC_SEARCH_START ),
         hwndStop  = GetDlgItem( Hwnd(), IDC_SEARCH_STOP );

    if( IsWindow( hwndStart ) )
    {
        EnableStartStopButton( hwndStart, !SearchInProgress() );
        if( bStopEvent && IsChild( Hwnd(), GetFocus() ) )
        {
            _pfsb->AutoActivate();
            SetFocus( hwndStart );
        }
    }

    if( IsWindow( hwndStop ) )
    {
        EnableStartStopButton( hwndStop, SearchInProgress() );
        if( bStartEvent )
        {
            _pfsb->AutoActivate();
            SetFocus( hwndStop );
        }
    }

    if( bStopEvent || !SearchInProgress() )
        StartStopAnimation( FALSE );
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::EnableStartStopButton( HWND hwndBtn, BOOL bEnable )
{
    if( IsWindow(hwndBtn) )
    {
        if( bEnable )
            _ModifyWindowStyle( hwndBtn, BS_DEFPUSHBUTTON, 0 );
        else
            _ModifyWindowStyle( hwndBtn, 0, BS_DEFPUSHBUTTON );

        ::EnableWindow( hwndBtn, bEnable );
    }
}

//-------------------------------------------------------------------------//
//  Extracts error information from ISearchCommandExt and 
//  propagate 
BOOL CSearchCmdDlg::ProcessCmdError()
{
    BOOL    bRet = FALSE;
    
    ISearchCommandExt* pSrchCmd;
    if( (pSrchCmd = GetSearchCmd()) != NULL )
    {
        HRESULT hr = S_OK;
        BSTR    bstrError = NULL;
        USES_CONVERSION;

        //  request error information through ISearchCommandExt
        if( SUCCEEDED( pSrchCmd->GetErrorInfo( &bstrError,  (int *)&hr ) ) )
            //  allow derivatives classes a crack at handling the error
            bRet = OnSearchCmdError( hr, bstrError ? W2T( bstrError ) : NULL );
    }
    else
    {
        ASSERT(pSrchCmd!=NULL);
    }

    return bRet ;
}

//-------------------------------------------------------------------------//
BOOL CSearchCmdDlg::OnSearchCmdError( HRESULT hr, LPCTSTR pszError )
{
    if( pszError )
    {
        ShellMessageBox( HINST_THISDLL, _pfsb->m_hWnd, pszError, NULL,
                         MB_OK|MB_ICONASTERISK );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::UpdateStatusText()
{
    if ( _fOnDestroy )
        return;
        
    ASSERT( _pfsb );
    ASSERT( _pfsb->BandSite() );

    //  BUGBUG: we probably should cache IWebBrowserApp*
    BSTR bstrStatus = NULL;

    ISearchCommandExt* pSrchCmd;
    if( NULL == (pSrchCmd = GetSearchCmd()) )
    {
        ASSERT( pSrchCmd );
        return;
    }

    if( SUCCEEDED( pSrchCmd->get_ProgressText( &bstrStatus ) ) )
    {
        IShellBrowser* psb;
        if( SUCCEEDED( IUnknown_QueryService( _pfsb->BandSite(), SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb ) ) ) 
        {
            IShellView* psv;
            if( SUCCEEDED( psb->QueryActiveShellView( &psv ) ) )
            {
                IWebBrowserApp*  pwba;
                if( SUCCEEDED( IUnknown_QueryService( psv, SID_SWebBrowserApp, IID_IWebBrowserApp, (void**)&pwba ) ) )
                {
                    pwba->put_StatusText( bstrStatus );
                    pwba->Release();
                    bstrStatus = NULL;
                }
                psv->Release();
            }
            psb->Release();
        }
        if( bstrStatus )
            SysFreeString( bstrStatus );
    }
}

//-------------------------------------------------------------------------//
void CSearchCmdDlg::OnBandShow( BOOL bShow ) 
{ 
    if ( !bShow ) 
        StopSearch() ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CSearchCmdDlg::TranslateAccelerator( LPMSG pmsg )
{
    if( S_OK == CBandDlg::TranslateAccelerator( pmsg ) )
        return S_OK;

    if( WM_KEYDOWN == pmsg->message &&
        VK_ESCAPE == pmsg->wParam && 
        SearchInProgress() &&
        0 == (GetKeyState( VK_CONTROL ) & 0x8000) )
    {
        StopSearch();
    }
    return S_FALSE;
}

//-----------------------------//
// ISearchCommandExt wrapping dlg

//  IUnknown
//-------------------------------------------------------------------------//
STDMETHODIMP CSearchCmdDlg::QueryInterface( REFIID riid, void** ppvObj )
{
    if ( riid == IID_IUnknown || 
         riid == IID_IDispatch || 
         riid == DIID_DSearchCommandEvents )
    {
        *ppvObj = (void*)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CSearchCmdDlg::AddRef()
{
    return ((IFileSearchBand*)_pfsb)->AddRef(); 
}

//-------------------------------------------------------------------------//
STDMETHODIMP_(ULONG) CSearchCmdDlg::Release()
{
    return ((IFileSearchBand*)_pfsb)->Release(); 
}

// IDispatch
//-------------------------------------------------------------------------//
STDMETHODIMP CSearchCmdDlg::Invoke(DISPID dispid, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)
{
    switch( dispid )
    {
        case DISPID_SEARCHCOMMAND_COMPLETE:
        case DISPID_SEARCHCOMMAND_ABORT:
        case DISPID_SEARCHCOMMAND_ERROR:
        case DISPID_SEARCHCOMMAND_START:
            _fSearchInProgress = (DISPID_SEARCHCOMMAND_START == dispid);
            _fSearchAborted =    (DISPID_SEARCHCOMMAND_ABORT == dispid);
            UpdateSearchCmdStateUI( dispid );
            if( DISPID_SEARCHCOMMAND_ERROR == dispid )
                ProcessCmdError();    
            break;

        case DISPID_SEARCHCOMMAND_PROGRESSTEXT:
            UpdateStatusText();
            break;

        case DISPID_SEARCHCOMMAND_RESTORE:
            PostMessage( Hwnd(), WMU_RESTORESEARCH, 0, 0L ); 
            //  See comments in the CSearchCmdDlg::OnRestoreSearch message handler.
            break;
            
#if 0
        case DISPID_SEARCHCOMMAND_UPDATE:
        break;
#endif
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
class CDivWindow
//-------------------------------------------------------------------------//
{
    //  All private members:
    CDivWindow();
    ~CDivWindow();
    
    static LRESULT WINAPI WndProc( HWND, UINT, WPARAM, LPARAM );
    LRESULT     EraseBkgnd( HDC hdc );
    LRESULT     WindowPosChanging( WINDOWPOS* pwp );
    LRESULT     SetHeight( LONG cy );
    LRESULT     SetBkColor( COLORREF rgb );


    static ATOM _atom;     // window class atom
    HWND        _hwnd;
    LONG        _cy;       // enforced height.
    COLORREF    _rgbBkgnd; // background color
    HBRUSH      _hbrBkgnd; // background brush

    friend BOOL WINAPI DivWindow_RegisterClass();
    friend BOOL WINAPI DivWindow_UnregisterClass( HINSTANCE );
};

ATOM CDivWindow::_atom = 0;

//-------------------------------------------------------------------------//
BOOL DivWindow_RegisterClass()
{
    if( CDivWindow::_atom != 0 )
        return CDivWindow::_atom;

    WNDCLASSEX wc;
    ZeroMemory( &wc, sizeof(wc) );
    
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_GLOBALCLASS;
    wc.lpfnWndProc    = CDivWindow::WndProc;
    wc.hInstance      = HINST_THISDLL;
    wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszClassName  = DIVWINDOW_CLASS;
    //wc.lpszMenuName 
    //wc.hIcon
    //wc.hIconSm

    return (CDivWindow::_atom = RegisterClassEx( &wc ));
}

//-------------------------------------------------------------------------//
BOOL DivWindow_UnregisterClass( HINSTANCE )
{
    return UnregisterClass( DIVWINDOW_CLASS, HINST_THISDLL );
}

//-------------------------------------------------------------------------//
inline CDivWindow::CDivWindow()
    :   _hwnd(NULL),
        _cy(1),
        _hbrBkgnd(NULL),
        _rgbBkgnd(COLOR_BTNFACE)
{
}
        
//-------------------------------------------------------------------------//
inline CDivWindow::~CDivWindow()
{
    if( _hbrBkgnd )
        DeleteObject( _hbrBkgnd );
}

//-------------------------------------------------------------------------//
LRESULT CDivWindow::EraseBkgnd( HDC hdc )
{
    if( !_hbrBkgnd )
        return DefWindowProc( _hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0L );

    RECT rc;
    GetClientRect( _hwnd, &rc );
    FillRect( hdc, &rc, _hbrBkgnd );
    return TRUE;
}

//-------------------------------------------------------------------------//
LRESULT CDivWindow::WindowPosChanging( WINDOWPOS* pwp )
{
    //  enforce height
    if( 0 == (pwp->flags & SWP_NOSIZE) )
        pwp->cy = _cy;
    return 0;
}

//-------------------------------------------------------------------------//
LRESULT CDivWindow::SetHeight( LONG cy )
{
    _cy = cy;
    return TRUE;
}

//-------------------------------------------------------------------------//
LRESULT CDivWindow::SetBkColor( COLORREF rgb )
{
    if( rgb != _rgbBkgnd )
    {
        _rgbBkgnd = rgb;
        if( _hbrBkgnd )
            DeleteObject( _hbrBkgnd );
        _hbrBkgnd = CreateSolidBrush( _rgbBkgnd );
        InvalidateRect( _hwnd, NULL, TRUE );
    }
    return TRUE;
}

//-------------------------------------------------------------------------//
LRESULT WINAPI CDivWindow::WndProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    CDivWindow* pThis = (CDivWindow*)GetWindowLongPtr( hwnd, GWLP_USERDATA );

    switch( nMsg )
    {
        case WM_ERASEBKGND:
            return pThis->EraseBkgnd( (HDC)wParam );

        case WM_WINDOWPOSCHANGING:
            return pThis->WindowPosChanging( (WINDOWPOS*)lParam );

        case WM_GETDLGCODE:
            return DLGC_STATIC;

        case DWM_SETHEIGHT:
            return pThis->SetHeight( (LONG)wParam );

        case DWM_SETBKCOLOR:
            return pThis->SetBkColor( (COLORREF)wParam );

        case WM_NCCREATE:
            if( NULL == (pThis = new CDivWindow) )
                return FALSE;
            pThis->_hwnd = hwnd;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)pThis );
            break;

        case WM_NCDESTROY:
        {
            LRESULT lRet = DefWindowProc( hwnd, nMsg, wParam, lParam );
            SetWindowPtr( hwnd, GWLP_USERDATA, 0 );
            pThis->_hwnd = NULL;
            delete pThis;
            return lRet;
        }
    }
    return DefWindowProc( hwnd, nMsg, wParam, lParam );
}

//-------------------------------------------------------------------------//
// {C8F945CB-327A-4330-BB2F-C04122959488}
static const IID IID_IStringMru = 
    { 0xc8f945cb, 0x327a, 0x4330, { 0xbb, 0x2f, 0xc0, 0x41, 0x22, 0x95, 0x94, 0x88 } };

//-------------------------------------------------------------------------//
//  Creates and initializes a CStringMru instance
HRESULT CStringMru::CreateInstance( 
    HKEY hKey, 
    LPCTSTR szSubKey,
    LONG cMaxStrings,
    BOOL  bCaseSensitive,
    REFIID riid, LPVOID* ppv )
{
    CStringMru* pmru = new CStringMru;
    if( NULL == pmru )
        return E_OUTOFMEMORY;

    pmru->_hKeyRoot = hKey;
    StrCpyN( pmru->_szSubKey, szSubKey, ARRAYSIZE(pmru->_szSubKey) );
    if( cMaxStrings > 0 )
        pmru->_cMax = cMaxStrings;
    pmru->_bCaseSensitive = bCaseSensitive;

    HRESULT hr = pmru->QueryInterface( riid, ppv );
    pmru->Release();

    return hr;
}

//-------------------------------------------------------------------------//
CStringMru::CStringMru()
    :   _hKeyRoot(NULL), 
        _hKey(NULL), 
        _cRef(1), 
        _hdpaStrings(NULL), 
        _cMax(25), 
        _bCaseSensitive(TRUE),
        _iString(-1)
{
    *_szSubKey = 0;
}

//-------------------------------------------------------------------------//
CStringMru::~CStringMru()
{
    _Close();
    _Clear();
}

//-------------------------------------------------------------------------//
//  Opens string MRU store
HRESULT CStringMru::_Open()
{
    if( _hKey )
        return S_OK;

    DWORD dwDisposition;
    DWORD dwErr = RegCreateKeyEx( _hKeyRoot, _szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                  NULL, &_hKey, &dwDisposition );
    return HRESULT_FROM_WIN32( dwErr );
}

//-------------------------------------------------------------------------//
//  Deletes string MRU store
void CStringMru::_Delete()
{
    if( _hKey )
        _Close();

    SHDeleteKey( _hKeyRoot, _szSubKey );
}

//-------------------------------------------------------------------------//
//  Reads string MRU store into memory
HRESULT CStringMru::_Read( OUT OPTIONAL LONG* pcRead )
{
    HRESULT hr = E_FAIL;
    if( pcRead )
        *pcRead = 0;

    if( SUCCEEDED( (hr = _Open()) ) )
    {
        _Clear();     // throw away existing cached strings
        _hdpaStrings = DPA_Create( 4 );  // allocate dynarray
        if( NULL == _hdpaStrings )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //  step through string values in registry.
            for( int iString = 0; iString < _cMax; iString++ )
            {
                TCHAR szVal[16];
                TCHAR szString[MAX_URL_STRING];
                ULONG dwType, cbString = sizeof(szString);

                wsprintf( szVal, TEXT("%03d"), iString );
                DWORD dwErr = RegQueryValueEx( _hKey, szVal, 0, &dwType, 
                                               (LPBYTE)szString, &cbString );

                if( dwErr == ERROR_SUCCESS && REG_SZ == dwType && *szString )
                {
                    UINT cch = lstrlen(szString) + 1;
                    LPOLESTR pwszAdd = new OLECHAR[cch];
                    if( pwszAdd )
                    {
                        SHTCharToUnicode( szString, pwszAdd, cch );
                        DPA_AppendPtr( _hdpaStrings, pwszAdd );
                    }
                }
            }
        }

        _Close();
        
        if( pcRead && _hdpaStrings )
            *pcRead = DPA_GetPtrCount( _hdpaStrings );
    }

    return hr;
}

//-------------------------------------------------------------------------//
//  Writes string MRU store from memory
HRESULT CStringMru::_Write( OUT OPTIONAL LONG* pcWritten )
{
    HRESULT hr = E_FAIL;
    LONG   cWritten = 0;
    
    if( pcWritten )
        *pcWritten = cWritten;

    //  Delete store and re-create.
    _Delete();
    if( NULL == _hdpaStrings )
        return S_FALSE;
    if( FAILED( (hr = _Open()) ) )
        return hr;

    ASSERT( DPA_GetPtrCount( _hdpaStrings ) <= _cMax );

    //  step through string values in registry.
    for( int iString = 0, cnt = DPA_GetPtrCount( _hdpaStrings ); 
         iString < cnt; iString++ )
    {
        TCHAR szVal[16];
        TCHAR szString[MAX_URL_STRING];

        wsprintf( szVal, TEXT("%03d"), iString );

        LPOLESTR pwszWrite = (LPOLESTR)DPA_FastGetPtr( _hdpaStrings, iString );
        SHUnicodeToTChar( pwszWrite, szString, ARRAYSIZE(szString) );

        DWORD dwErr = RegSetValueEx( _hKey, szVal, 0, REG_SZ, 
                                     (LPBYTE)szString, sizeof(szString) );

        if( ERROR_SUCCESS == dwErr )
            cWritten++;
    }

    _Close();

    if( pcWritten )
        *pcWritten = cWritten;

    return S_OK;
}

//-------------------------------------------------------------------------//
//  Closes string MRU store
void  CStringMru::_Close()
{
    if( _hKey )
    {
        RegCloseKey( _hKey );
        _hKey = NULL;
    }
}

//-------------------------------------------------------------------------//
//  Adds a string to the store
STDMETHODIMP CStringMru::Add( LPCOLESTR pwszAdd )
{
    if( !(pwszAdd && *pwszAdd) )
        return E_INVALIDARG;
    
    if( NULL == _hdpaStrings )
    {
        if( NULL == (_hdpaStrings = DPA_Create(4)) )
            return E_OUTOFMEMORY;
    }
        
    HRESULT hr = E_FAIL;
    LONG    iMatch = -1;

    for( LONG i = 0, cnt = DPA_GetPtrCount( _hdpaStrings ); i < cnt; i++ )
    {
        LPOLESTR pwsz = (LPOLESTR)DPA_FastGetPtr( _hdpaStrings, i );
        if( pwsz )
        {
            int nCompare = _bCaseSensitive ? 
                    StrCmpW( pwszAdd, pwsz ) : StrCmpIW( pwszAdd, pwsz );

            if( 0 == nCompare )
            {
                iMatch = i;
                break;
            }       
        }
    }

    if( -1 == iMatch )
    {
        //  Create a copy and add it to the list.
        LPOLESTR pwszCopy = new OLECHAR[lstrlenW( pwszAdd ) + 1];
        if( NULL == pwszCopy )
            return E_OUTOFMEMORY;
        
        StrCpyW( pwszCopy, pwszAdd );
        
        int iNew = DPA_InsertPtr( _hdpaStrings, 0, pwszCopy );
        if( iNew < 0 )
        {
            delete [] pwszCopy;
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ASSERT( 0 == iNew );
            hr = S_OK;
        }
    }
    else
        hr = _Promote( iMatch );

    if( S_OK == hr )
    {
        //  If we've grown too large, delete LRU string
        int cStrings = DPA_GetPtrCount( _hdpaStrings );
        while( cStrings > _cMax )
        {
            LPOLESTR pwsz = (LPOLESTR)DPA_DeletePtr( _hdpaStrings, cStrings - 1 );
            if( pwsz )
                delete [] pwsz;
            cStrings--;
        }
        hr = _Write();
    }

    return hr;
}

//-------------------------------------------------------------------------//
//  Promotes a string to MRU
HRESULT CStringMru::_Promote( LONG iString )
{
    if( 0 == iString )
        return S_OK;

    LONG cnt = _hdpaStrings ? DPA_GetPtrCount( _hdpaStrings ) : 0 ;
    
    if( iString >= cnt )
        return E_INVALIDARG;

    LPOLESTR pwsz = (LPOLESTR)DPA_DeletePtr( _hdpaStrings, iString );
    if( pwsz )
    {
        int iMru = DPA_InsertPtr( _hdpaStrings, 0, pwsz );

        if( iMru < 0 )
        {
            delete [] pwsz;
            return E_OUTOFMEMORY;
        }
        else
        {
            ASSERT( 0 == iMru );
            return S_OK;
        }
    }
    return E_FAIL;
}

//-------------------------------------------------------------------------//
//  Clears string MRU memory cache
void CStringMru::_Clear()
{
    if( _hdpaStrings )
    {
        for( int i = 0, cnt = DPA_GetPtrCount( _hdpaStrings ); i < cnt; i++ )
        {
            LPOLESTR pwsz;
            if( (pwsz = (LPOLESTR)DPA_FastGetPtr( _hdpaStrings, i )) != NULL )
                delete [] pwsz;
        }

        DPA_Destroy( _hdpaStrings );
        _hdpaStrings = NULL;
    }
}

//-------------------------------------------------------------------------//
    // *** IUnknown ***
STDMETHODIMP_(ULONG) CStringMru::AddRef(void)
{
    _cRef++;
    return 0;
}

STDMETHODIMP_(ULONG) CStringMru::Release(void)
{
    if( _cRef > 0 )
    {
        if( 0 == --_cRef )
            delete this;
    }
    return 0;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CStringMru::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if( !ppvObj )
        return E_POINTER;
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IUnknown*)(IStringMru*)this;
    }
    else if (IsEqualIID(riid, IID_IEnumString))
    {
        *ppvObj = (IEnumString*)this;
    }
    else if (IsEqualIID(riid, IID_IStringMru))
    {
        *ppvObj = (IStringMru*)this;
    }

    if( *ppvObj )
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//-------------------------------------------------------------------------//
// *** IEnumString ***
STDMETHODIMP CStringMru::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    ULONG cFetched = 0;

    if( pceltFetched )
        *pceltFetched = cFetched;

    if( NULL == _hdpaStrings )
    {
        HRESULT hr = _Read();
        if( FAILED(hr) )
            return hr;
    }

    for( int cnt =  _hdpaStrings ? DPA_GetPtrCount( _hdpaStrings ) : 0; 
         cFetched < celt && (_iString + 1) < cnt; )
    {
        _iString++;
        LPOLESTR pwsz = (LPOLESTR)DPA_FastGetPtr( _hdpaStrings, _iString );
        if( pwsz )
        {
            if( (rgelt[cFetched] = (LPOLESTR)CoTaskMemAlloc( (lstrlenW(pwsz) + 1) * sizeof(OLECHAR) )) != NULL )
            {
                StrCpyW( rgelt[cFetched], pwsz );
                cFetched++;
            }
        }
    }

    if( pceltFetched )
        *pceltFetched = cFetched;

    return cFetched == celt ? S_OK : S_FALSE ;
}

STDMETHODIMP CStringMru::Skip(ULONG celt)
{
    _iString += celt;
    if( _iString >= _cMax )
        _iString = _cMax - 1;
    return S_OK;
}

STDMETHODIMP CStringMru::Reset(void)
{
    _iString = -1;
    return S_OK;
}
