//-------------------------------------------------------------------------//
//
//  Dictionary.cpp - Various single and multi-key dictionary collections
//                  used by the Property Tree Control or its subelements.
//
//-------------------------------------------------------------------------//
#include "pch.h"
#include "Dictionary.h"
#include "TreeItems.h"

//-------------------------------------------------------------------------//
BOOL CFileAssocMap::Lookup( LPCTSTR pszExt, IAdvancedPropertyServer*& pServer ) const
{
    PTFILEEXT ext( pszExt ) ;
    return (pszExt && *pszExt) ? CFileAssocMapBase::Lookup( ext, pServer ) : FALSE ;
}

//-------------------------------------------------------------------------//
BOOL CFileAssocMap::Insert( LPCTSTR pszExt, IAdvancedPropertyServer* pServer )
{
    PTFILEEXT ext( pszExt ) ;
    int       c = Count() ;

    if( pszExt && *pszExt && pServer )
    {
        if( Lookup( pszExt, pServer ) )
            return TRUE ;

        pServer->AddRef() ;
        (*this)[ext] = pServer ;
        return c < Count() ;
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
void CFileAssocMap::OnDelete( PTFILEEXT& key, IAdvancedPropertyServer*& pServer )
{
    if( pServer )
    {
        pServer->Release() ;
        pServer = NULL ;
    }
}

//-------------------------------------------------------------------------//
void CFolderDictionary::OnDelete( PFID& pfid, CPropertyFolder*& pFolder )
{
    ASSERT( pFolder ) ;
    delete pFolder ;
    pFolder = NULL ;
}

//-------------------------------------------------------------------------//
void CPropertyDictionary::OnDelete( CPropertyUID& propID, CProperty*& pProperty )
{
    ASSERT( pProperty ) ;
    delete pProperty ;
    pProperty = NULL ;
}

//-------------------------------------------------------------------------//
HRESULT CMasterSourceDictionary::Insert( const VARIANT* pvarSrc, IAdvancedPropertyServer* pServer, LPARAM lParamSrc )
{
    //  We'll allocate a key and AddRef() the server prior to insertion.
    LPCOMVARIANT key ;
    PTSERVER     val ;

    //  Validate arguments
    if( !( pvarSrc && pServer ) )
        return E_INVALIDARG ;

    //  Alloc and initialize our key
    if( (key = new CComVariant( *pvarSrc ))==NULL )
        return E_OUTOFMEMORY ;
    
    //  Make sure the key doesn't already exist.
    if( CMasterSourceDictionaryBase::Lookup( key, val ) )
    {
        delete key ;
        return E_ACCESSDENIED ;
    }
    
    //  Initialize the value and assign.
    memset( &val, 0, sizeof(val) ) ;
    val.pServer    = pServer ;
    val.lParamSrc  = lParamSrc ;
    (*this)[key] = val ;
    pServer->AddRef() ;
    return S_OK ;
}

//-------------------------------------------------------------------------//
void CMasterSourceDictionary::Delete( const VARIANT* pvarSrc )
{
    ASSERT( pvarSrc ) ;
    CComVariant var( *pvarSrc ) ;
    DeleteKey( &var ) ;
}

//-------------------------------------------------------------------------//
void CMasterSourceDictionary::OnDelete( LPCOMVARIANT& key, PTSERVER& val )
{ 
    ASSERT( key ) ; 
    ASSERT( val.pServer ) ;

    if( val.pServer )
    {
        //  Instruct server to release properties
        val.pServer->ReleaseAdvanced( val.lParamSrc, PTREL_SOURCE_REMOVED ) ;
        //  Release the server
        val.pServer->Release() ;
        val.pServer = NULL ;
    }
    if( key ) delete key ;
}

//-------------------------------------------------------------------------//
void CSourceDictionaryLite::OnDelete( LPCOMVARIANT& pvarSrc, LPARAM& val )
{
    ASSERT( pvarSrc ) ;
    delete pvarSrc ;    pvarSrc = NULL ;
}

//-------------------------------------------------------------------------//
void CValueDictionary::OnDelete( LPCOMVARIANT& pvarSrc, LPCOMPROPVARIANT& val )
{
    ASSERT( pvarSrc ) ;
    ASSERT( val ) ;
    delete pvarSrc ;    pvarSrc = NULL ;
    delete val ;        val = NULL ;
}


//-------------------------------------------------------------------------//
BOOLEAN CSelectionList::Lookup( 
    IN const PDISPLAYPROPVARIANT pKey,
    OUT PDISPLAYPROPVARIANT* ppMatch ) const
{
    BOOLEAN bRet = FALSE ;
    
    PDISPLAYPROPVARIANT pVar ;
    HANDLE              hEnum ;
    BOOL                bEnum ;

    ASSERT( pKey ) ;
    if( ppMatch )
        *ppMatch = NULL ;

    for( hEnum = EnumHead( pVar ), bEnum = TRUE ;
         hEnum && bEnum ;
         bEnum = EnumNext( hEnum, pVar ) )
    {
        ASSERT( pVar ) ;
        if( pKey->Compare( *pVar, STRICT_COMPARE )==0 )
        {
            bRet = TRUE ;
            if( ppMatch )
                *ppMatch = pVar ;
            break ;
        }
    }
    EndEnum( hEnum ) ;

    return bRet ;
}

//-------------------------------------------------------------------------//
ULONG VariantHash( const VARIANT& var )
{
    switch( var.vt )
    {
        case VT_EMPTY:   //nothing
        case VT_NULL:    //SQL style Null
            return var.vt ;
        case VT_I2:      //2 byte signed int
            return var.iVal + var.vt ;
        case VT_I4:      //4 byte signed int
            return var.lVal + var.vt ;
        case VT_R4:      //4 byte real
            return HashBytes( &var.fltVal, sizeof(var.fltVal) ) + var.vt ;
        case VT_R8:      //8 byte real
            return HashBytes( &var.dblVal, sizeof(var.dblVal) ) + var.vt ;
        case VT_CY:      //currency
            return HashBytes( &var.cyVal, sizeof(var.cyVal) ) + var.vt ;
        case VT_DATE:    //date
            return HashBytes( &var.date, sizeof(var.date) ) + var.vt ;
        case VT_BSTR:    //OLE Automation string
            return HashStringW( var.bstrVal ) + var.vt ;
        case VT_DISPATCH://IDispatch *
            return HashPointer( var.pdispVal ) + var.vt ;
        case VT_ERROR:   //SCODE
            return var.scode + var.vt ;
        case VT_BOOL:    //True=-1, False=0
            return (var.boolVal ? 1 : 0) + var.vt ;
        case VT_VARIANT: //VARIANT *
            return (var.pvarVal ? VariantHash( *var.pvarVal ) : 0) + var.vt ;
        case VT_UNKNOWN: //IUnknown *
            return HashPointer( var.punkVal ) + var.vt ;
        case VT_DECIMAL: //16 byte fixed point
            return HashBytes( &var.decVal, sizeof(var.decVal) ) + var.vt ;
        case VT_I1:      //signed char
            return var.cVal + var.vt ;
        case VT_UI1:     //unsigned char
            return var.bVal + var.vt ;
        case VT_UI2:     //unsigned short
            return var.uiVal + var.vt ;
        case VT_UI4:     //unsigned short
            return var.ulVal + var.vt ;
        case VT_INT:     //signed machine int
            return var.intVal + var.vt ;

        case VT_UINT:    //unsigned machine int
            return var.uintVal + var.vt ;

        case VT_BYREF:   //void* for local use
            return HashPointer( var.byref ) + var.vt ;

        default:
            break ;
    } ;
    return (ULONG)-1 ;
}
