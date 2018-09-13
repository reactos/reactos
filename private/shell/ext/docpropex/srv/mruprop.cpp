//-------------------------------------------------------------------------//
//
//	MruProp.cpp
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include "MruProp.h"
#include "propvar.h"

//-------------------------------------------------------------------------//
static LPCTSTR szRECKEYNAMEFMT  = TEXT("%s\\mru\\%s\\%02ld-%02ld"),    // subkey, fmtid, propid, vt
               szINDEXVALNAME   = TEXT("index"),
               szRECTITLEFMT    = TEXT("%02ld") ;
static const int  cchVALNAME       = 4 ;
static const ULONG INVALID_INDEX_VALUE   = (ULONG)-1 ;

//-------------------------------------------------------------------------//
#define INIT_INDEX_ELEMENT( iPos )  \
    { m_index[iPos].nRec   = INVALID_INDEX_VALUE ; \
      m_index[iPos].dwHash = 0 ; }
#define VALID_INDEX_ELEMENT( iPos ) \
      ((iPos)>=0 && (iPos)<IndexCount())

//-------------------------------------------------------------------------//
CPropMruStor::CPropMruStor()
{
    Construct() ;
}

//-------------------------------------------------------------------------//
CPropMruStor::CPropMruStor( HKEY hKeyRoot, LPCTSTR pszSubKey )
{
    Construct() ;
    SetRoot( hKeyRoot, pszSubKey ) ;
}

//-------------------------------------------------------------------------//
void CPropMruStor::Construct()
{
    m_hKey = 
    m_hKeyRoot = NULL ;
    m_pszRootSubKey = NULL ;
	m_pszRecSubKey = NULL ;
    m_cRef = 0 ;
    
    for( ULONG i = 0; i < IndexCount(); i++ )
        INIT_INDEX_ELEMENT( i ) ;
}

//-------------------------------------------------------------------------//
CPropMruStor::~CPropMruStor()
{
    Close() ;
    ClearRecKeyName() ;
    ClearRoot() ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::SetRoot( HKEY hKeyRoot, LPCTSTR pszRootSubKey )
{
    LPTSTR  psz = NULL ;
    int     cch = pszRootSubKey ? lstrlen( pszRootSubKey ) : 0 ;

    if( cch )
    {
        if( (psz = new TCHAR[cch+1])==NULL )
            return E_OUTOFMEMORY ;
        lstrcpyn( psz, pszRootSubKey, cch+1 ) ;
    }
    
    Close() ;
    ClearRoot() ;
    ClearRecKeyName() ;
    m_hKeyRoot = hKeyRoot ;
    m_pszRootSubKey = psz ;

    return S_OK ;
}

//-------------------------------------------------------------------------//
void CPropMruStor::ClearRoot()
{
    if( m_pszRootSubKey )
    {
        delete m_pszRootSubKey ;
        m_pszRootSubKey = NULL ;
    }
    m_hKeyRoot = NULL ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::PushMruVal( REFFMTID fmtid, PROPID propid, VARTYPE vt, IN const PROPVARIANT& var )
{
    HRESULT hr ;
    
    //  Retrieve length of data
    ULONG cbVal ;
    if( !SUCCEEDED( (hr = PropVariantSize( var, cbVal, NULL )) ) )
        return hr ;
    cbVal += sizeof( var.vt ) ;

    //  Build the key name
    if( !SUCCEEDED( (hr = MakeRecKeyName( fmtid, propid, vt )) ) )
        return hr ;

    //  Open the key
    if( !SUCCEEDED( (hr = Open()) ) )
        return hr ;

    //  Load the MRU index
    LoadIndex( m_index ) ;

    //  If the PROPVARIANT already exists, just promote it and get out.
    ULONG iPosMatch ;
    if( FindVal( var, iPosMatch )==S_OK )
        return PromoteMruVal( iPosMatch ) ;

    //  Determine PROPVARIANT's hash value
    if( !SUCCEEDED( (hr = PropVariantHash( var, m_index[0].dwHash )) ) )
        return hr ;

    //  Pop the LRU value
    if( !SUCCEEDED( (hr = PopMruVal()) ) )
        return hr ;

    //  Assign new index value
    m_index[0].nRec     = GetUnusedRecNo() ;


    TCHAR  szValName[cchVALNAME] ;
    if( !GetRecTitle( 0, szValName ) )
    {
        INIT_INDEX_ELEMENT( 0 ) ;
        return E_UNEXPECTED ;
    }
    
    //  Persist the new index and PROPVARIANT value
    if( !SUCCEEDED( (hr = PropVariantWriteRegistry( m_hKey, szValName, var )) ) )
    {
        INIT_INDEX_ELEMENT( 0 ) ;
        return hr ;
    }
    
    //  Persist the index
    hr = UpdateIndex() ;

    return hr ;
}

//-------------------------------------------------------------------------//
//  Removes the LRU item.
HRESULT CPropMruStor::PopMruVal()
{
    ASSERT( IsOpen() ) ;
    HRESULT hr = S_OK ;

    if( GetRecNo( 0 )==INVALID_INDEX_VALUE )
        return hr ;

    TCHAR szValName[cchVALNAME] ;
    if( GetRecTitle( IndexCount()-1, szValName ) )
    {
        DWORD dwRet = RegDeleteValue( m_hKey, szValName ) ;
        if( !SUCCEEDED( (hr = HRESULT_FROM_WIN32( dwRet )) ) )
            return hr ;
    }

    //  Advance indices by one position.
    for( int i= IndexCount()-1; i>0; i-- )
        m_index[i] = m_index[i-1] ;
    
    //  Clear head
    INIT_INDEX_ELEMENT( 0 ) ;
    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::FindVal( IN const PROPVARIANT& var, OUT ULONG& iPos )
{
    ASSERT( IsOpen() ) ;

    HRESULT hr ;
    ULONG   dwHash = 0 ;

    iPos = INVALID_INDEX_VALUE ;

    if( !SUCCEEDED( (hr = PropVariantHash( var, dwHash )) ) )
        return hr ;

    for( ULONG i=0; i < IndexCount(); i++ )
    {
        PROPVARIANT varOther ;
        TCHAR       szValName[cchVALNAME] ;
        

        if( m_index[i].dwHash != dwHash )
            continue ;

        if( !GetRecTitle( i, szValName ) )
            continue ;

        if( !SUCCEEDED( (hr = PropVariantReadRegistry( m_hKey, szValName, varOther )) ) )
            continue ;
        
        if( PropVariantCompare( var, varOther, STRICT_COMPARE )!=0 )
            continue ;
        
        iPos = i ;
        return S_OK ;
    }
    return S_FALSE ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::PromoteMruVal( IN ULONG iPosSrc )
{
    if( !VALID_INDEX_ELEMENT( iPosSrc ) )
    {
        ASSERT( FALSE ) ;
        return E_INVALIDARG ;
    }
           
    if( iPosSrc==0 )
        return S_OK ;
        
    INDEX index = m_index[iPosSrc] ;

    for( ULONG iPos = iPosSrc; iPos > 0; iPos-- )
        m_index[iPos] = m_index[iPos-1] ;
    
    m_index[0] = index ;
    return UpdateIndex() ;
}

//-------------------------------------------------------------------------//
ULONG CPropMruStor::GetUnusedRecNo() const
{
    BYTE    iPos, bUsed ;
    ULONG   nRec ;
    
    //  For each posssible value between 0 and the limit
    for( nRec=0; nRec < IndexCount(); nRec++ )
    {
        //  For each array element
        for( iPos = 0, bUsed = FALSE; iPos < IndexCount(); iPos++ )
        {
            //  If the value ofthe element equals our test value,
            //  go to next possible value
            if( m_index[iPos].nRec == nRec )
            {
                bUsed = TRUE ;
                break ;
            }
        }
        
        if( bUsed ) continue ;
        return nRec ;
    }
    
    ASSERT( FALSE ) ;   // should never happen
    return INVALID_INDEX_VALUE ;
}

//-------------------------------------------------------------------------//
ULONG CPropMruStor::GetRecNo( ULONG iPos ) const
{
    ASSERT( VALID_INDEX_ELEMENT( iPos ) ) ;
    return m_index[iPos].nRec ;
}

//-------------------------------------------------------------------------//
BOOL CPropMruStor::GetRecTitle( ULONG iPos, LPTSTR pszBuf ) const
{
    ASSERT( pszBuf ) ;
    ULONG  nRec ;
    
    if( (nRec = GetRecNo( iPos ))==INVALID_INDEX_VALUE )
    {
        *pszBuf = 0 ;
        return FALSE ;
    }
    
    wsprintf( pszBuf, szRECTITLEFMT, nRec ) ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::BeginFetch( REFFMTID fmtid, PROPID propid, VARTYPE vt )
{
    HRESULT hr ;

    //  Build the key name and open the key
    if( !SUCCEEDED( (hr = MakeRecKeyName( fmtid, propid, vt )) ) )
        return hr ;

    //  Open the key
    if( !SUCCEEDED( (hr = Open()) ) )
        return hr ;

    //  Load MRU order index.
    return LoadIndex( m_index ) ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::FetchNext( LONG& iStart, PROPVARIANT_DISPLAY& var )
{
    TCHAR  szValName[cchVALNAME] ;

    //  Ensure stor is open
    if( !IsOpen() )
        return E_ACCESSDENIED ;

    var.bstrDisplay = NULL ;

    //  Move through records beginning with requested position.
    for( LONG iPos = iStart; iPos < (LONG)IndexCount(); iPos++ )
    {    
        if( !GetRecTitle( iPos, szValName ) )
            continue ;
    
        if( !SUCCEEDED( PropVariantReadRegistry( m_hKey, szValName, var.val ) ) )
            continue ;

        iStart = ++iPos ;
        return S_OK ;

    }
    return E_FAIL ;
}

#if 0 // (implemented, but not used)
//-------------------------------------------------------------------------//
HANDLE CPropMruStor::EnumFirst( REFFMTID fmtid, PROPID propid, VARTYPE vt, OUT PROPVARIANT& var )
{
    HRESULT hr ;    
    ENUM*  pEnum = NULL ;

    //  Build the key name
    if( !SUCCEEDED( (hr = MakeRecKeyName( fmtid, propid, vt )) ) )
        return pEnum ;

    //  Open the key
    if( !SUCCEEDED( (hr = Open()) ) )
        return pEnum ;

    //  Load the MRU index
    LoadIndex( m_index ) ;

    for( ULONG iPos =0; iPos< IndexCount(); iPos++ )
    {
        TCHAR  szValName[cchVALNAME] ;
        if( !GetRecTitle( iPos, szValName ) )
            continue ;
        
        if( !SUCCEEDED( PropVariantReadRegistry( m_hKey, szValName, var ) ) )
            continue ;

        if( (pEnum = new ENUM )!=NULL )
            pEnum->iPos = iPos ;
        break ;
    }

    return pEnum ;
}

//-------------------------------------------------------------------------//
BOOL CPropMruStor::EnumNext( HANDLE hEnum, PROPVARIANT& var ) const
{
    BOOL    bRet = FALSE ;
    ENUM*  pEnum = (ENUM*)hEnum ;

    if( (m_hKey && pEnum) ) 
    {
        for( pEnum->iPos++; pEnum->iPos < IndexCount(); pEnum->iPos++ )
        {
            TCHAR  szValName[cchVALNAME] ;
            if( !GetRecTitle( pEnum->iPos, szValName ) )
                continue ;
        
            if( !SUCCEEDED( PropVariantReadRegistry( m_hKey, szValName, var ) ) )
                continue ;
            
            bRet = TRUE ;
            break ;
        }
    }
    return bRet ;
}

//-------------------------------------------------------------------------//
void CPropMruStor::EndEnum( HANDLE& hEnum )	const
{
	ENUM* pEnum = (ENUM*)hEnum ;
	if( pEnum )
	{
		delete pEnum ;
		hEnum = NULL ;
	}
}

#endif //0

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::MakeRecKeyName( REFFMTID fmtid, PROPID propid, VARTYPE vt )
{
    HRESULT hr ;
    LPOLESTR pwszFmtID    = NULL ;
    LPTSTR   psz          = NULL ;
    USES_CONVERSION ;

    if( SUCCEEDED( (hr = StringFromCLSID( fmtid, &pwszFmtID )) ) )
    {
        int cch = lstrlen( m_pszRootSubKey ) +
                  lstrlenW( pwszFmtID ) + 16 +
                  lstrlen( szRECKEYNAMEFMT ) ;
                  
        if( (psz = new TCHAR[cch])!=NULL )
        {
            wsprintf( psz, szRECKEYNAMEFMT,
                      m_pszRootSubKey, W2T( pwszFmtID ), propid, vt ) ;

            ClearRecKeyName() ;
            m_pszRecSubKey = psz ;
        }            
        else
            hr = E_OUTOFMEMORY ;
    }

    if( pwszFmtID ) CoTaskMemFree( pwszFmtID ) ;
    return hr ;
}

//-------------------------------------------------------------------------//
void CPropMruStor::ClearRecKeyName()
{
    if( m_pszRecSubKey )
    {
        delete [] m_pszRecSubKey ;
        m_pszRecSubKey = NULL ;
    }
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::LoadIndex( PVOID pvDest )
{
    ASSERT( IsOpen() ) ;

    HRESULT hr ;
    DWORD   dwType = REG_BINARY,
            cbDest = sizeof(m_index),
            dwRet ;

    dwRet = RegQueryValueEx( m_hKey, szINDEXVALNAME, 0L, &dwType, 
                             (LPBYTE)pvDest, &cbDest ) ;

    if( S_OK == (hr = HRESULT_FROM_WIN32( dwRet )) )
    {
        if( pvDest != &m_index )
            memcpy( m_index, pvDest, sizeof(m_index) ) ;
        return S_OK ;
    }
    else if( ERROR_MORE_DATA == dwRet )
    {
        // registry value corruption.
        RegDeleteValue( m_hKey, szINDEXVALNAME ) ;
        ASSERT( FALSE ) ; 
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ;
    }
    return hr ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::UpdateIndex()
{
    ASSERT( IsOpen() ) ;
    DWORD dwRet = RegSetValueEx( m_hKey, szINDEXVALNAME, 0L, REG_BINARY,
                       (LPBYTE)m_index, sizeof(m_index) ) ;

    return HRESULT_FROM_WIN32( dwRet ) ;
}

//-------------------------------------------------------------------------//
HRESULT CPropMruStor::Open( REGSAM sam )
{
    ASSERT( m_hKeyRoot ) ;
    ASSERT( m_pszRecSubKey && *m_pszRecSubKey ) ;

    HRESULT hr ;
    HKEY    hKey = NULL ;
    DWORD   dwDisp = 0L,
            dwRet ;

    dwRet = RegCreateKeyEx( m_hKeyRoot, m_pszRecSubKey, 0L, NULL, 
                            REG_OPTION_NON_VOLATILE, sam, NULL, 
                            &hKey, &dwDisp ) ;

    if( S_OK == (hr = HRESULT_FROM_WIN32( dwRet )) )
    {
        Close() ;
        m_hKey = hKey ;
    }
    return hr ;
}

//-------------------------------------------------------------------------//
void CPropMruStor::Close()
{
    if( m_hKey )
    {
        RegCloseKey( m_hKey ) ;
        m_hKey = NULL ;
    }
}
