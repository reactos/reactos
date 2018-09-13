//-------------------------------------------------------------------------//
//  Enum.cpp
//-------------------------------------------------------------------------//

#include "pch.h"
#include "PTsrv32.h"
#include "Enum.h"
#include "DefProp.h"
#include "MruProp.h"
#include "DefSrv32.h"

//-------------------------------------------------------------------------//
//  Standard IUnknown implementation:
#define IMPLEMENT_ENUM_UNKNOWN( classname, iid ) \
    ULONG classname::m_cObj = 0 ; \
    STDMETHODIMP classname::QueryInterface( REFIID _iid, void** ppvObj ) { \
        if( ppvObj == NULL ) return E_POINTER ; *ppvObj = NULL ; \
        if( IsEqualGUID( _iid, IID_IUnknown ) )   *ppvObj = this ; \
        else if( IsEqualGUID( _iid, iid ) ) *ppvObj = this ; \
        if( *ppvObj )    { AddRef() ; return S_OK ;  } \
        return E_NOINTERFACE ; } \
    STDMETHODIMP_( ULONG ) classname::AddRef() { \
        if( InterlockedIncrement( (LONG*)&m_cRef )==1 )  \
            _Module.Lock() ; \
        return m_cRef ; } \
    STDMETHODIMP_( ULONG ) classname::Release() { \
        if( InterlockedDecrement( (LONG*)&m_cRef )==0 ) \
            { _Module.Unlock() ; delete this ; return 0 ; } \
        return m_cRef ;  } \


//-------------------------------------------------------------------------//
// CEnumFolderItem
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
CEnumFolderItem::CEnumFolderItem( CPropertySource* pSrc )
    :   m_iPos(0),
        m_pEnumSpecialized(NULL),
        m_cMax(DefFolderCount()),
        m_cRef(0),
        m_pSrc( pSrc )
{
    ASSERT( m_pSrc ) ;

    //  Contain specialized enumerator, if any.
    IAdvancedPropertyServer* pSpecialized ;
    LPARAM lParamSpecialized ;
    if( (pSpecialized = m_pSrc->GetSpecializedServer( &lParamSpecialized )) )
        pSpecialized->EnumFolderItems( lParamSpecialized, &m_pEnumSpecialized ) ;

}
IMPLEMENT_ENUM_UNKNOWN( CEnumFolderItem, IID_IEnumPROPFOLDERITEM ) ;

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumFolderItem::Next( ULONG cItems, PROPFOLDERITEM * rgFolderItem, ULONG * pcItemsFetched )
{
    if( !( pcItemsFetched && rgFolderItem ) ) 
        return E_POINTER ;
    *pcItemsFetched = 0 ;

    //  If we have a specialized server, give him a crack
    if( m_pEnumSpecialized )
    {
        if( SUCCEEDED( m_pEnumSpecialized->Next( cItems, rgFolderItem, pcItemsFetched ) ) )
        {
            //  we need to hijack his data cookie...
            for( ULONG i = 0; i< *pcItemsFetched; i++ )
                rgFolderItem[i].lParam = (LPARAM)m_pSrc ;
        }
    }

    //  Do our own thing
    while( *pcItemsFetched < cItems && m_iPos < m_cMax )
    {
        HRESULT hr ;
        if( m_pSrc->UsesFolder( m_iPos ) )
        {
            if( SUCCEEDED( (hr = MakeDefFolderItem( m_iPos, &rgFolderItem[*pcItemsFetched], (LPARAM)m_pSrc )) ) )
                (*pcItemsFetched)++ ;
        }
        InterlockedIncrement( &m_iPos ) ;
    }

    return cItems == *pcItemsFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumFolderItem::Reset()
{
    if( m_pEnumSpecialized )
        m_pEnumSpecialized->Reset() ;

	InterlockedExchange( &m_iPos, 0 ) ;
    return S_OK;
}

//-------------------------------------------------------------------------//
const CEnumFolderItem& CEnumFolderItem::operator=( const CEnumFolderItem& src )
{
    if( &src==this ) 
        return *this ;

    InterlockedExchange( &m_iPos, src.m_iPos ) ;
    InterlockedExchange( &m_cMax, src.m_cMax ) ;
    return *this ;
}

//-------------------------------------------------------------------------//
// CEnumPropertyItem
//-------------------------------------------------------------------------//
IMPLEMENT_ENUM_UNKNOWN( CEnumPropertyItem, IID_IEnumPROPERTYITEM ) ;

//-------------------------------------------------------------------------//
CEnumPropertyItem::CEnumPropertyItem( CPropertySource* pSrc, const PROPFOLDERITEM* pFolder )
    :   m_cRef(0),
        m_hIterator(NULL),
        m_pEnumSpecialized(NULL),
        m_pfid(pFolder->pfid),
        m_pSrc( pSrc ),
        m_bEnumSpecialized(FALSE)
{
    ASSERT( pSrc ) ;

    //  Contain specialized enumerator, if any.
    IAdvancedPropertyServer* pSpecialized ;
    LPARAM lParamSpecialized ;
    if( (pSpecialized = m_pSrc->GetSpecializedServer( &lParamSpecialized )) )
    {
        ((PROPFOLDERITEM*)pFolder)->lParam = lParamSpecialized ;
        m_bEnumSpecialized = S_OK == pSpecialized->EnumPropertyItems( pFolder, &m_pEnumSpecialized ) ;
        ((PROPFOLDERITEM*)pFolder)->lParam = (LPARAM)pSrc ;
    }
}

//-------------------------------------------------------------------------//
CEnumPropertyItem::~CEnumPropertyItem()
{
    Reset() ;
}

//-------------------------------------------------------------------------//
BOOL CEnumPropertyItem::Iterate( OUT PROPERTYITEM& propitem )
{
    CPropertyMap& map = m_pSrc->PropertyMap() ;

    if( m_hIterator )
        return map.LookupNext( m_hIterator, propitem ) ;

    if( (m_hIterator = map.LookupFirst( CPropertyMap::iFolderID, &m_pfid, propitem ))==NULL )
        return FALSE ;
    
    return TRUE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumPropertyItem::Next( 
    ULONG cItems, 
    PROPERTYITEM * rgPropertyItem, 
    ULONG * pcItemsFetched )
{
    if( !( pcItemsFetched && rgPropertyItem ) ) 
        return E_POINTER ;
    *pcItemsFetched = 0 ;

    //  If we have a specialized server, give him a crack
    if( m_pEnumSpecialized && m_bEnumSpecialized )
    {
        HRESULT hr = m_pEnumSpecialized->Next( cItems, rgPropertyItem, pcItemsFetched ) ;
        if( SUCCEEDED( hr ) )
        {
            for( ULONG i = 0; i< *pcItemsFetched; i++ )
            {
                //  If the specialized server is serving up a property that
                //  we were ready to supply, remove our own version.
                CPropertyMap& map = m_pSrc->PropertyMap() ;
                if( map.Exists( CPropertyMap.iShColID, (SHCOLUMNID*)&rgPropertyItem[i].puid ) )
                {
                    ASSERT( NULL == m_hIterator ) ; // don't delete unless we know we're reset.
                    map.Delete( CPropertyMap.iShColID, (SHCOLUMNID*)&rgPropertyItem[i].puid ) ;
                }
                
                //  hijack his data cookie...
                rgPropertyItem[i].lParam = (LPARAM)m_pSrc ;
            }
        }
        else
            m_bEnumSpecialized = FALSE ;    // no need to ask again.
    }

    while( *pcItemsFetched < cItems )
    {
        if( Iterate( rgPropertyItem[*pcItemsFetched] ) )
            (*pcItemsFetched)++ ;
        else
            break ;
    }

    return cItems == *pcItemsFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumPropertyItem::Reset()
{
    if( m_pEnumSpecialized )
    {
        m_pEnumSpecialized->Reset() ;
        m_bEnumSpecialized = TRUE ;
    }
    
    if( m_hIterator )
    {
        CPropertyMap& map = m_pSrc->PropertyMap() ;
        map.EndLookup( m_hIterator ) ;
        m_hIterator = NULL ;
    }
    return S_OK;
}

//-------------------------------------------------------------------------//
const CEnumPropertyItem& CEnumPropertyItem::operator= ( const CEnumPropertyItem& src )
{
    if( &src == this )
        return src ;

    m_pfid = src.m_pfid ;
    m_pSrc = src.m_pSrc ;

    return *this ;
}

//-------------------------------------------------------------------------//
// CEnumMruValues
//-------------------------------------------------------------------------//
IMPLEMENT_ENUM_UNKNOWN( CEnumMruValues, IID_IEnumPROPVARIANT_DISPLAY ) ;

//-------------------------------------------------------------------------//
CEnumMruValues::CEnumMruValues( const tagPUID& puid )
    :   m_puid(puid),
        m_iPos(0), 
        m_cMax(0),
        m_cRef(0)
{
    for( int i=0; i<5; i++ )
    {
        PROPVARIANT v ;
        ULONG       dwHash = 0 ;
        v.vt = VT_LPSTR ;
        v.pszVal = "Scott Hanggie" ;
        PropVariantHash( v, dwHash ) ;
    }
}

//-------------------------------------------------------------------------//
const CEnumMruValues& CEnumMruValues::operator= ( const CEnumMruValues& src )
{
    if( this != &src )
    {
        m_puid      = src.m_puid ;
        m_iPos      = src.m_iPos ;
        m_cMax      = src.m_cMax ;
    }
    return *this ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumMruValues::Next( ULONG cItems, PROPVARIANT_DISPLAY * rgVar, ULONG * pcItemsFetched )
{
    CPropMruStor stor( PTREGROOTKEY, PTREGSUBKEY ) ;
    HRESULT      hr ;
    long         iStart = m_iPos ;
    
    if( !( pcItemsFetched && rgVar ) ) 
        return E_POINTER ;
    *pcItemsFetched = 0 ;

    if( !SUCCEEDED( (hr = stor.BeginFetch( m_puid.fmtid, m_puid.propid, m_puid.vt )) ) )
        return S_FALSE ;

    m_cMax = stor.IndexCount() ;
    if( m_iPos >= m_cMax )
        return S_FALSE ;

    for( hr = stor.FetchNext( m_iPos, rgVar[m_iPos - iStart] ) ;
         SUCCEEDED( hr ) ; )
    {
        if( ++(*pcItemsFetched) >= cItems ) 
            break ;
        hr = stor.FetchNext( m_iPos, rgVar[m_iPos - iStart] ) ;
    }

    return cItems == *pcItemsFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumMruValues::Reset()
{
    InterlockedExchange( &m_iPos, 0 ) ;
    return S_OK;
}

//-------------------------------------------------------------------------//
// CEnumHardValues
//-------------------------------------------------------------------------//
IMPLEMENT_ENUM_UNKNOWN( CEnumHardValues, IID_IEnumPROPVARIANT_DISPLAY ) ;

//-------------------------------------------------------------------------//
CEnumHardValues::CEnumHardValues( const DEFVAL* pVals, int cVals )
    : m_cMax( cVals ),
      m_iPos(0),
      m_pVals( pVals )
{
}

//-------------------------------------------------------------------------//
CEnumHardValues::~CEnumHardValues()
{
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumHardValues::Next( 
    ULONG cItems,
    PROPVARIANT_DISPLAY * rgVar, 
    ULONG * pcItemsFetched )
{
    if( !( pcItemsFetched && rgVar ) ) 
        return E_POINTER ;
    *pcItemsFetched = 0 ;

    if( m_iPos >= m_cMax )
        return S_FALSE ;

    for( long iStart = m_iPos ;
         m_iPos < min( m_cMax, iStart + (long)cItems ); )
    {
        InitPropVariantDisplay( &rgVar[m_iPos-iStart] ) ;
        //  BUGBUG: the following line is type-unsafe, and works only for simple VARTYPES.
        //  Guaranteed to fail for pointer, string and array types.
        //  Need to shore up at least the string types, and implement string resource loading.
        PropVariantCopy( &rgVar[m_iPos-iStart].val, (PROPVARIANT*)&m_pVals[m_iPos] ) ;

        //  Allocate display string, if any.
        if( 0L != m_pVals[m_iPos].lpDisplay )
        {
            if( m_pVals[m_iPos].fDisplayStrRes )
            {
                WCHAR wszDisplay[MAX_PATH+1] ;
                if( LoadStringW( _Module.GetModuleInstance(), (UINT)m_pVals[m_iPos].lpDisplay,
                                 wszDisplay, sizeof(wszDisplay)/sizeof(WCHAR) ) )
                {
                    rgVar[m_iPos - iStart].bstrDisplay = SysAllocString( wszDisplay ) ;
                }
            }
            else
            {
                USES_CONVERSION ;
                rgVar[m_iPos - iStart].bstrDisplay = 
                    SysAllocString( T2W( (LPTSTR)m_pVals[m_iPos].lpDisplay ) ) ;
            }
        }

        (*pcItemsFetched)++ ;
        InterlockedIncrement( &m_iPos ) ;
    }

    return cItems == *pcItemsFetched ? S_OK : S_FALSE ;
}

//-------------------------------------------------------------------------//
STDMETHODIMP CEnumHardValues::Reset()
{
    InterlockedExchange( &m_iPos, 0 ) ;
    return S_OK;
}

//-------------------------------------------------------------------------//
const CEnumHardValues& CEnumHardValues::operator=( const CEnumHardValues& src )
{
    if( &src==this ) 
        return *this ;

    InterlockedExchange( &m_iPos, src.m_iPos ) ;
    InterlockedExchange( &m_cMax, src.m_cMax ) ;
    m_pVals = src.m_pVals ;

    return *this ;
}
