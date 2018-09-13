//-------------------------------------------------------------------------//
// ext.cpp : CShellExt impl
//-------------------------------------------------------------------------//
#include "pch.h"
#include "ext.h"
#include "ptsniff.h"
#include "propvar.h"
#include "ptserver.h"
#include "ptsrv32.h"
#include "page.h"

//-------------------------------------------------------------------------//
const TCHAR szEXTENSION_SETTINGS_REGKEY[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\PropSummary");
const TCHAR szUIMODE_REGVAL[]             = TEXT("Advanced");

const FMTID* basic_fmtids[] = {
    &FMTID_SummaryInformation,
    &FMTID_DocSummaryInformation,
};

//  Note: members of the following arrays must be 
//  synch'd and ordered according to BASICPROPERTY enum members.
const PROPID basic_propids[] = {
    PIDSI_TITLE,
    PIDSI_SUBJECT,
    PIDSI_AUTHOR,
    PIDSI_KEYWORDS,
    PIDSI_COMMENTS,
    PIDDSI_CATEGORY,
};

const VARTYPE basic_propvts[] = {
    VT_LPWSTR,
    VT_LPWSTR,
    VT_LPWSTR,
    VT_LPWSTR,
    VT_LPWSTR,
    VT_LPWSTR,
};

const UINT  basic_ctlIDs[] = {
    IDC_TITLE,
    IDC_SUBJECT,
    IDC_AUTHOR,
    IDC_KEYWORDS,
    IDC_COMMENTS,
    IDC_CATEGORY,
};

//-------------------------------------------------------------------------//
inline HRESULT IsValidProp( BASICPROPERTY nProp )
{
    return (nProp >= 0 && nProp < BASICPROP_COUNT) ?
                S_OK : E_INVALIDARG;
}

//-------------------------------------------------------------------------//
//  CBasicPropertySource  // basic property block.
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CBasicPropertySource::CBasicPropertySource()
    :   _pbps(NULL),
        _rgItems(NULL)
{
    VariantInit( &_varSource );
}

//-------------------------------------------------------------------------//
void CBasicPropertySource::Free()
{
    VariantClear( &_varSource );

    if( _rgItems )
    {
        delete [] _rgItems;
        _rgItems = NULL;
    }

    if( _pbps )
    {
        _pbps->Release();
        _pbps = NULL;
    }
}

//-------------------------------------------------------------------------//
HRESULT CBasicPropertySource::Acquire( const VARIANT* pvarSource )
{
    BASICPROPITEM* rgItems = NULL;
    HRESULT hr;
    IBasicPropertyServer* pbps;

    hr = CoCreateInstance( CLSID_PTDefaultServer32,
                           NULL, CLSCTX_INPROC_SERVER,
                           IID_IBasicPropertyServer,
                           (LPVOID*)&pbps );
    if( SUCCEEDED( hr ) )
    {
        if( (rgItems = new BASICPROPITEM[BASICPROP_COUNT]) != NULL )
        {
            for( int f=0; f < ARRAYSIZE(basic_fmtids); f++ )
            {
                int iPropFirst, iPropLast, cProps;
                if( CShellExt::GetBasicPropInfo( *basic_fmtids[f], iPropFirst, iPropLast, cProps ) )
                {
                    for( int i = iPropFirst; i <= iPropLast; i++ )
                    {
                        InitBasicPropertyItem( &rgItems[i] );
                        ASSERT( FALSE == rgItems[i].bDirty );
                        rgItems[i].dwAccess = PTIA_READWRITE;
                        rgItems[i].puid.fmtid = *basic_fmtids[f];
                        rgItems[i].puid.propid= basic_propids[i];
                        rgItems[i].puid.vt    = basic_propvts[i];
                    }
                }
            }
            
            if( SUCCEEDED( (hr = pbps->AcquireBasic( pvarSource, rgItems, BASICPROP_COUNT )) ) )
            {
                Free();
                VariantCopy( &_varSource, (VARIANT*)pvarSource );
                _pbps = pbps;
                _pbps->AddRef();
                _rgItems = rgItems;
            }
            else
                delete [] rgItems;
        }
        pbps->Release();
    }
    return hr;
}

//-------------------------------------------------------------------------//
void CBasicPropertySource::SetDirty( BASICPROPERTY nProp, BOOL bDirty )
{
    ASSERT( _pbps != NULL && _rgItems != NULL );
    ASSERT( BASICPROP_COUNT > (UINT)nProp );

    _rgItems[nProp].bDirty = bDirty;
}

//-------------------------------------------------------------------------//
HRESULT CBasicPropertySource::Persist()
{
    if( !(_pbps && _rgItems) )
    {
        ASSERT( _pbps && _rgItems );
        return E_UNEXPECTED;
    }

    return _pbps->PersistBasic( &_varSource, _rgItems, BASICPROP_COUNT );
}

//-------------------------------------------------------------------------//
int CBasicPropertySource::Compare( BASICPROPERTY nProp, CBasicPropertySource* pOther )
{
    ASSERT( IsValidProp( nProp ) );
    ASSERT( _rgItems != NULL );
    ASSERT( pOther );
    
    return PropVariantCompare( _rgItems[nProp].val, pOther->_rgItems[nProp].val, STRICT_COMPARE );
}

//-------------------------------------------------------------------------//
BSTR CBasicPropertySource::MakeDisplayString( BASICPROPERTY nProp )
{
    ASSERT( IsValidProp( nProp ) );
    ASSERT( _rgItems != NULL );
    BSTR bstrRet;

    if( SUCCEEDED( PropVariantToBstr( &_rgItems[nProp].val, GetACP(), 0L, NULL, &bstrRet ) ) )
        return bstrRet;
    return NULL;
}

//-------------------------------------------------------------------------//
HRESULT CBasicPropertySource::SetPropertyValue( BASICPROPERTY nProp, IN BSTR bstrValue )
{
    if( NULL == _rgItems )
        return E_FAIL;

    if( FAILED( IsValidProp( nProp ) ) || NULL == bstrValue )
        return E_INVALIDARG;

    if( VT_EMPTY == _rgItems[nProp].val.vt )
        _rgItems[nProp].val.vt = _rgItems[nProp].puid.vt;
        
    return PropVariantFromBstr( bstrValue, GetACP(), 0L, NULL, &_rgItems[nProp].val );
}

//-------------------------------------------------------------------------//
// CShellExt
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  IShellExtInit::Initialize
STDMETHODIMP CShellExt::Initialize( 
    LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT pIDataObject, 
    HKEY hkeyProgID )
{
    HRESULT     hr;
    STGMEDIUM   stgmed;
    FORMATETC   fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1,
                           TYMED_HGLOBAL };
    int         cAdvanced = 0, cBasic = 0; // specialized server file type

    if( SUCCEEDED( (hr = pIDataObject->GetData( &fmtetc, &stgmed )) ) )
    {
        TCHAR szPath[MAX_PATH];
        UINT  i, max = DragQueryFile( (HDROP)stgmed.hGlobal, 0xFFFFFFFF, szPath, 
                                     sizeof(szPath)/sizeof(TCHAR) );

        //  Add path names to file list as VARIANTs
        for( i=0; i < max; i++ )
        {
            if( DragQueryFile( (HDROP)stgmed.hGlobal, i, szPath, 
                               sizeof(szPath)/sizeof(TCHAR) ) )
            {
                WIN32_FIND_DATA wfd;
                int             fAdvanced, fBasic;
                TARGET          t;

                if( FAILED( (hr = FilterFileFindData(szPath, &wfd )) ) ||
                    0xFFFFFFFF == wfd.dwFileAttributes )
                    continue;

                t.ftAccess = wfd.ftLastAccessTime;

                //  Is this a supported property source?
                fAdvanced = IsSourceSupported( szPath, TRUE /*advanced*/) ? 1 : 0;
                fBasic    = IsSourceSupported( szPath, FALSE /*basic*/ ) ? 1 : 0;

                if( fAdvanced || fBasic )
                {
                    USES_CONVERSION;
                    VARIANT var;

                    VariantInit( &t.varFile );
                    if( (t.varFile.bstrVal = SysAllocString( T2W( szPath ) ))==NULL )
                    {
                        hr = E_FAIL;
                        ReleaseStgMedium(&stgmed);
                        goto ret;
                    }
                    t.varFile.vt = VT_BSTR;

                    if( m_filelist.AppendTail( t ) )
                    {
                        if( wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
                            m_cReadOnly++;
                        if( fAdvanced )
                            cAdvanced++;
                        if( fBasic )
                            cBasic++;
                    }
                    else
                        VariantClear( &t.varFile );
                }
                else
                {
                    m_filelist.Clear();
                    break;
                }
            }
        }

        hr = m_filelist.Count() > 0 ? S_OK : E_FAIL;
        ReleaseStgMedium(&stgmed);
    }

    //  Determine if we have something to show; if so, increment refcount
    if( SUCCEEDED( hr ) )
    {
        m_fBasicProperties    = SUCCEEDED( (hr = Acquire()) );    // try acquiring basic.
        m_fAdvancedProperties = cAdvanced > 0 && (cAdvanced == m_filelist.Count());

        //  if we found either basic and/or advanced properties, set
        //  outselves up to present.
        if( m_fBasicProperties || m_fAdvancedProperties )
        {
            hr = S_OK;
            InternalAddRef();  // InternalRelease() when page detaches.
        }
    }

ret:
    return hr;
}

//-------------------------------------------------------------------------//
//  IShellPropSheetExt::AddPages
STDMETHODIMP CShellExt::AddPages( 
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam	)
{
    if( (m_pPage0 = new CPage0)==NULL )
        return E_OUTOFMEMORY;

    return m_pPage0->Add( this, lpfnAddPage, lParam );
}

//-------------------------------------------------------------------------//
//  IShellPropSheetExt::ReplacePage
STDMETHODIMP CShellExt::ReplacePage( 
    UINT uPageID, 
    LPFNADDPROPSHEETPAGE lpfnReplacePage, 
    LPARAM lParam )
{
    return E_NOTIMPL;
}

//-------------------------------------------------------------------------//
//  ctor
CShellExt::CShellExt()  
    :   m_pPage0(NULL),
        m_fAdvancedProperties(FALSE),
        m_fBasicProperties(TRUE),
        m_cReadOnly(0),
        m_rgBasicSrc(NULL)
{
    //  Initialize
    for( int i = 0; i < BASICPROP_COUNT; i++ )
    {
        m_rgvarFlags[i] = 0L;
        m_rgbstrDisplay[i] = NULL;
        m_rgbDirty[i] = FALSE;
    }
}

//-------------------------------------------------------------------------//
//  dtor
CShellExt::~CShellExt()
{
    FinalRelease();
}

//-------------------------------------------------------------------------//
void CShellExt::FinalRelease()
{
    if( m_rgBasicSrc )
    {
        delete [] m_rgBasicSrc;
        m_rgBasicSrc = NULL;
    }

    for( int i = 0; i < BASICPROP_COUNT; i++ )
    {
        SysFreeString( m_rgbstrDisplay[i] );
        m_rgbstrDisplay[i] = NULL;
    }
}

//-------------------------------------------------------------------------//
//  Determines the support status for the specified file
BOOL CShellExt::IsSourceSupported( 
    LPTSTR pszPath, 
    BOOL   bAdvanced,
    OUT OPTIONAL LPCLSID pclsid )
{
    PTSRV_FILECLASS fileClass;
    PTSRV_FILETYPE  fileType;
    CLSID           clsid;

    if( !pclsid )
        pclsid = &clsid;

    //  If this file type isknown to our default property server
    //  and the default server supports the type...
    if( IsPTsrvKnownFileType( pszPath, &fileType, &fileClass ) &&
        fileClass != PTSFCLASS_UNSUPPORTED )
    {
        BOOL bSupported = PTSFCLASS_OFFICEDOC == fileClass ?
                            /*don't trust office doc based on filename ext alone*/
                            IsPropSetStgFmt( pszPath, STGFMT_ANY, NULL ) :
                            bAdvanced;

        if( bSupported )
        {
            if( pclsid )
                *pclsid = CLSID_PTDefaultServer32;
            return TRUE;
        }
    }

    //  Try determining if there is a registered property server
    //  object for the type
    if( S_OK == GetPropServerClassForFile( pszPath, bAdvanced, pclsid ) )
        return TRUE;

    //  If the default server has rejected the type, bail now
    if( PTSFCLASS_UNSUPPORTED == fileClass )
        return FALSE;

    //  Last chance: check if we can open property streams on the file
    if( IsPropSetStgFmt( pszPath, STGFMT_ANY, NULL ) )
        return TRUE;    

    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves basic properties (title, subject, category, author, keywords, comments).
//  If this fails for any source file, the property sheet will not appear.
HRESULT CShellExt::Acquire()
{
    HANDLE  hEnum;
    BOOL    bEnum;
    int     iSrc, cSrcs = FileList().Count();
    TARGET  t;
    HRESULT hr = S_OK;

    if( cSrcs <=0 )
        return E_FAIL;

    CBasicPropertySource* rgBasicSrc = new CBasicPropertySource[cSrcs];
    if( NULL == rgBasicSrc )
        return E_OUTOFMEMORY;

    //  Initialize array of property sources.
    for( iSrc = 0, hEnum = FileList().EnumHead( t ), bEnum = TRUE; 
         hEnum && bEnum && iSrc < cSrcs;
         iSrc++, bEnum = FileList().EnumNext( hEnum, t ) )
    {
        hr = rgBasicSrc[iSrc].Acquire( &t.varFile );
#ifdef RESTORE_ACCESS_TIMES
        _RestoreAccessTime( t );
#endif RESTORE_ACCESS_TIMES
        
        if( FAILED( hr ) )
        {
            delete [] rgBasicSrc;
            rgBasicSrc = NULL;
            break;
        }
    }
    FileList().EndEnum( hEnum );

    if( SUCCEEDED( hr ) )
    {
        if( m_rgBasicSrc )
            delete m_rgBasicSrc;
        m_rgBasicSrc = rgBasicSrc;

        //  For each property, resolve value mismatches among multiple sources.
        for( int prop = 0; prop < BASICPROP_COUNT; prop++ )
        {
            
            SysFreeString( m_rgbstrDisplay[prop] );           // clear display text.
            m_rgvarFlags[prop] &= ~AMPF_COMPOSITE_MISMATCH; // clear mismatch flag
            for( int i = 0; i < cSrcs; i++ )
            {
                for( int j = 0; j< cSrcs; j++ )
                {            
                    if( i != j && rgBasicSrc[i].Compare( (BASICPROPERTY)prop, &rgBasicSrc[j] ) != 0 )
                    {
                        m_rgvarFlags[prop] |= AMPF_COMPOSITE_MISMATCH; // set mismatch flag
                        break;
                    }
                }
                if( m_rgvarFlags[prop] & AMPF_COMPOSITE_MISMATCH ) // test mismatch flag
                    break;
            }

            if( !(m_rgvarFlags[prop] & AMPF_COMPOSITE_MISMATCH) )
                m_rgbstrDisplay[prop] = rgBasicSrc[0].MakeDisplayString( (BASICPROPERTY)prop ); // regen display text.
        }
    }

    return hr;
}
  
//-------------------------------------------------------------------------//
//  Persists changes to standard properties (title, subject, 
//  category, author).
HRESULT CShellExt::Persist()
{
    HANDLE  hEnum;
    BOOL    bEnum;
    int     iSrc, iProp, cSrcs = FileList().Count();
    HRESULT hr = S_OK, 
            hrSrc;

    if( cSrcs > 0 && m_rgBasicSrc != NULL )
    {
        //  Update property values
        for( iProp = 0; iProp < BASICPROP_COUNT; iProp++ )
        {
            if( m_rgbDirty[iProp] )
            {
                //  iterate sources and assign new value
                for( iSrc = 0; iSrc < cSrcs; iSrc++ )
                {
                    hrSrc = m_rgBasicSrc[iSrc].SetPropertyValue( (BASICPROPERTY)iProp, m_rgbstrDisplay[iProp] );
                    m_rgBasicSrc[iSrc].SetDirty( (BASICPROPERTY)iProp, TRUE );
                    if( FAILED( hrSrc ) )
                        hr = hrSrc;  // but keep going.
                }
            }
        }            

        for( iSrc = 0; iSrc < cSrcs; iSrc++ )
        {
            hrSrc = m_rgBasicSrc[iSrc].Persist();
            if( FAILED( hrSrc ) )
                hr = hrSrc;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        for( iProp = 0; iProp < BASICPROP_COUNT; iProp++ )
            SetDirty( (BASICPROPERTY)iProp, FALSE );
    }
    return hr;
}

//-------------------------------------------------------------------------//
//  Caches property values from the user interface
void CShellExt::CacheUIValues( HWND hwndPage )
{
    LPTSTR  pszBuf;    
    int     cchBuf = GetMaxPropertyTextLength( hwndPage )+1;

    if( NULL != (pszBuf = new TCHAR[cchBuf]) )
    {
        for( int i=0; i < BASICPROP_COUNT; i++ )
        {
            if( !IsCompositeMismatch( (BASICPROPERTY)i ) )
            {
                *pszBuf = (TCHAR)0;
                GetDlgItemText( hwndPage, basic_ctlIDs[i], pszBuf, cchBuf );
                CacheDisplayText( (BASICPROPERTY)i, pszBuf );
            }
        }
        delete [] pszBuf;
    }
}

//-------------------------------------------------------------------------//
//  Caches property values from the user interface
void CShellExt::CacheUIValue( HWND hwndPage, BASICPROPERTY nProp )
{
    HWND    hwndCtl;
    if( (hwndCtl = GetDlgItem( hwndPage, basic_ctlIDs[nProp] )) )
    {
        LPTSTR  pszBuf;    
        int     cchBuf = GetWindowTextLength( hwndCtl )+1;

        if( NULL != (pszBuf = new TCHAR[cchBuf]) )
        {
            *pszBuf = (TCHAR)0;
            GetWindowText( hwndCtl, pszBuf, cchBuf );
            CacheDisplayText( nProp, pszBuf );
            delete [] pszBuf;
        }
    }
}

//-------------------------------------------------------------------------//
//  Transfers cached property values to the user interface
void CShellExt::UncacheUIValues( HWND hwndPage )
{
    for( int i=0; i<BASICPROP_COUNT; i++ )
        UncacheUIValue( hwndPage, (BASICPROPERTY)i );
}

//-------------------------------------------------------------------------//
//  Transfers a cached property value to the user interface
void CShellExt::UncacheUIValue( HWND hwndPage, BASICPROPERTY nProp )
{
    USES_CONVERSION;
    SetDlgItemTextW( hwndPage, basic_ctlIDs[nProp], m_rgbstrDisplay[nProp] );
}

//-------------------------------------------------------------------------//
//  Retrieves the maximum length of the properties cached value text.
int CShellExt::GetMaxPropertyTextLength( HWND hwndDlg )
{
    int cchMax = 0;

    for( int i=0; i < BASICPROP_COUNT; i++ )
    {
        int cch = 0;

        if( hwndDlg )
        {
            HWND hwndCtl = GetDlgItem( hwndDlg, basic_ctlIDs[i] );
            ASSERT( NULL != hwndCtl );
            cch = GetWindowTextLength( hwndCtl );
        }
        else if( m_rgbstrDisplay[i] && *m_rgbstrDisplay[i] )
            cch = lstrlenW( m_rgbstrDisplay[i] );

        if( cch > cchMax )
            cchMax = cch;
    }
    return cchMax;
}

//-------------------------------------------------------------------------//
//  Retrieves the value text for the indicated property.
//  If hwndDlg == NULL, the text will be copied from cached data.  Otherwise,
//  the data will be retrieved from the dialog.
BOOL CShellExt::GetPropertyText( HWND hwndDlg, BASICPROPERTY nProp, LPTSTR pszBuf, int cchBuf )
{
    HWND    hwndCtl;
    BSTR    bstrRet = NULL;

    ASSERT( pszBuf );
    *pszBuf = 0;

    if( hwndDlg )
    {
        //  If mismatched composite value, return empty string
        if( IsCompositeMismatch( nProp ) )
            return TRUE;

        if( NULL != (hwndCtl = ::GetDlgItem( hwndDlg, basic_ctlIDs[nProp]) ) )
        {
            GetWindowText( hwndCtl, pszBuf, cchBuf );
            return TRUE;
        }
    }
    else
    {
        USES_CONVERSION;
        if( m_rgbstrDisplay[nProp] )
            lstrcpyn( pszBuf, W2T( m_rgbstrDisplay[nProp] ), cchBuf );
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Assigns the UI text
void CShellExt::SetPropertyText( HWND hwndDlg, BASICPROPERTY nProp, LPCTSTR pszText )
{
    HWND    hwndCtl;
    BSTR    bstrRet = NULL;

    if( NULL != (hwndCtl = ::GetDlgItem( hwndDlg, basic_ctlIDs[nProp]) ) )
    {
        USES_CONVERSION;
        SetWindowText( hwndCtl, pszText );
    }

    CacheDisplayText( nProp, pszText );
}

//-------------------------------------------------------------------------//
void CShellExt::CacheDisplayText( BASICPROPERTY nProp, LPCTSTR pszText )
{
    USES_CONVERSION;
    
    if( m_rgbstrDisplay[nProp] && pszText )
        if( lstrcmp( W2T( m_rgbstrDisplay[nProp] ), pszText )==0 )
            return;    // no change
    
    if( m_rgbstrDisplay[nProp] )
    {
        SysFreeString( m_rgbstrDisplay[nProp] );
        m_rgbstrDisplay[nProp] = NULL;
    }

    if( pszText )
    {
        m_rgbstrDisplay[nProp] = SysAllocString( T2W( (LPTSTR)pszText ) );
    }
}

//-------------------------------------------------------------------------//
//  Returns the identifier of the shell prop sheet extension's property 
//  given a dlg control identifier.
BOOL CShellExt::GetBasicPropFromIDC( UINT nIDC, BASICPROPERTY* pnProp ) const
{
    for( int i=0; i<BASICPROP_COUNT; i++ )
    {
        if( basic_ctlIDs[i] == nIDC )
        {
            if( pnProp ) *pnProp = (BASICPROPERTY)i;
            return TRUE;
        }
    }
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves array parameters for the specified FMTID
BOOL CShellExt::GetBasicPropInfo( REFFMTID fmtid, OUT int& iFirst, OUT int& iLast, OUT int& cProps ) 
{
    if( IsEqualGUID( fmtid, FMTID_SummaryInformation ) )
    {
        iFirst = BASICPROP_SUMMARYINFO_FIRST;
        iLast  = BASICPROP_SUMMARYINFO_LAST;
        cProps = (iLast - iFirst) + 1;
        return TRUE;
    }
    else
    if( IsEqualGUID( fmtid, FMTID_DocSummaryInformation ) )
    {
        iFirst = BASICPROP_DOCSUMMARYINFO_FIRST;
        iLast  = BASICPROP_DOCSUMMARYINFO_LAST;
        cProps = (iLast - iFirst) + 1;
        return TRUE;
    }

    iFirst = cProps = -1;
    return FALSE;
}

//-------------------------------------------------------------------------//
//  Retrieves prop ID parameters for the specified basic property
BOOL CShellExt::GetBasicPropInfo( BASICPROPERTY nProp, OUT const FMTID*& pFmtid, OUT PROPID& nPropID, OUT VARTYPE& vt )
{
    if( nProp >= BASICPROP_SUMMARYINFO_FIRST && 
        nProp <= BASICPROP_SUMMARYINFO_LAST )
        pFmtid = &FMTID_SummaryInformation;
    else
    if( nProp >= BASICPROP_DOCSUMMARYINFO_FIRST && 
        nProp <= BASICPROP_DOCSUMMARYINFO_LAST )
        pFmtid = &FMTID_DocSummaryInformation;
    else
        return FALSE;

    nPropID = basic_propids[nProp];
    vt      = basic_propvts[nProp];
    return TRUE;
}

//-------------------------------------------------------------------------//
//  Reports whether the indicated property's value is a composite of
//  multiple, mismatched values.
BOOL CShellExt::IsCompositeMismatch( BASICPROPERTY nProp ) const
{
    return (m_rgvarFlags[nProp] & AMPF_COMPOSITE_MISMATCH) != 0;
}

//-------------------------------------------------------------------------//
void CShellExt::SetDirty( BASICPROPERTY nProp, BOOL bDirty )
{
    if( (m_rgbDirty[nProp] = bDirty) && IsCompositeMismatch( nProp ) )
        m_rgvarFlags[nProp] &= ~AMPF_COMPOSITE_MISMATCH;
}

//-------------------------------------------------------------------------//
//  issues a shell change notification.
void CShellExt::ChangeNotify( LONG wEventID )
{
    HANDLE  hEnum;
    BOOL    bEnum;
    TARGET  t;

    for( hEnum = FileList().EnumHead( t ), bEnum = TRUE; 
         hEnum && bEnum;
         bEnum = FileList().EnumNext( hEnum, t ) )
    {
        TCHAR szPath[MAX_PATH];
        USES_CONVERSION;
        lstrcpy( szPath, W2T(t.varFile.bstrVal) );
        SHChangeNotify( wEventID, SHCNF_PATH, szPath, NULL );
    }
    FileList().EndEnum( hEnum );
    
    SHChangeNotifyHandleEvents();
}

//-------------------------------------------------------------------------//
HRESULT _RestoreAccessTime( const TARGET& t )
{
    TCHAR szPath[MAX_PATH];
    USES_CONVERSION;
    lstrcpy( szPath, W2T(t.varFile.bstrVal) );

    DWORD dwErr = ERROR_SUCCESS ;

    HANDLE hFile = CreateFile( szPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 
                               FILE_FLAG_OPEN_NO_RECALL, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
    {
        dwErr = GetLastError(); 
    }
    else
    {
        if( !SetFileTime( hFile, NULL, &t.ftAccess, NULL ) )
            dwErr = GetLastError();
        CloseHandle( hFile );
    }

    HRESULT hr = HRESULT_FROM_WIN32( dwErr );
    return hr;
}

//-------------------------------------------------------------------------//
//  Restores file access times for target files.
void CShellExt::RestoreAccessTimes()
{
    HANDLE  hEnum;
    BOOL    bEnum;
    TARGET  t;

    for( hEnum = FileList().EnumHead( t ), bEnum = TRUE; 
         hEnum && bEnum;
         bEnum = FileList().EnumNext( hEnum, t ) )
    {
        _RestoreAccessTime( t );
    }
    FileList().EndEnum( hEnum );
}


//-------------------------------------------------------------------------//
//  Displays an error message
int CShellExt::DisplayError( HWND hwndOwner, UINT nIDCaption, ERROR_CONTEXT errctx, HRESULT hr )
{
    UINT nIDErr = 0;
    UINT uType  = MB_OK;
    int  nRet   = IDOK;
    
    #define DEFINE_ERROR( hr, idErr, type ) \
                case hr: {nIDErr = idErr; uType=type; break;}
    #define DEFINE_MULTIERROR( hr, idErr1, idErrN, type ) \
                case hr: {nIDErr = m_filelist.Count() > 1 ? idErrN : idErr1; uType=type; break;}

    switch( errctx )
    {
        case ERRCTX_PERSIST_APPLY:
        case ERRCTX_PERSIST_OK:
        {
            UINT uBtnFlags = (ERRCTX_PERSIST_OK == errctx) ? MB_OKCANCEL : MB_OK;
            switch( hr )
            {
                DEFINE_MULTIERROR( STG_E_ACCESSDENIED, \
                                   IDS_ERR_ACCESSDENIED_1, IDS_ERR_ACCESSDENIED_N, \
                                   uBtnFlags|MB_ICONEXCLAMATION );

                DEFINE_MULTIERROR( STG_E_LOCKVIOLATION, \
                                   IDS_ERR_LOCKVIOLATION_1, IDS_ERR_LOCKVIOLATION_N, \
                                   uBtnFlags|MB_ICONEXCLAMATION );
            }
        }
        break;

        case ERRCTX_ACQUIRE:
        default:
            break;
    }
    
    if( nIDErr )
    {
        TCHAR szErr[256];
        TCHAR szCaption[128];
        LoadString( HINST_RESDLL, nIDErr,  szErr, ARRAYSIZE(szErr) );
        LoadString( HINST_RESDLL, nIDCaption, szCaption, ARRAYSIZE(szCaption) );
        
        nRet = ::MessageBox( hwndOwner, szErr, szCaption, uType );
    }

    return nRet;
}

HRESULT CShellExt::FilterFileFindData(LPCTSTR pszPath, WIN32_FIND_DATA* pwfd)
{
    HRESULT hr = S_OK;
    HANDLE hfind = FindFirstFile( pszPath, pwfd );
    if (INVALID_HANDLE_VALUE != hfind)
    {
        FindClose(hfind);

#if 0   //  NT raid# 327281: shouldn't avoid encrypted files
        if( pwfd->dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED )
        {
            hr = STG_E_INVALIDFUNCTION;
        }
        else 
#endif 0
        if( pwfd->dwFileAttributes & FILE_ATTRIBUTE_OFFLINE )
        {
            hr = HRESULT_FROM_WIN32( ERROR_FILE_OFFLINE );
        }
        else
        {
            //  Enforce read-only bit if we can't get write access for any reason.
            if ( 0 == (pwfd->dwFileAttributes & FILE_ATTRIBUTE_READONLY) && 
                      !PTCheckWriteAccess( pszPath ) )
                pwfd->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
        }
    }
    else
    {
        pwfd->dwFileAttributes = 0xFFFFFFFF;
        hr = STG_E_FILENOTFOUND;
    }
    return hr;
}

