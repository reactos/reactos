//-------------------------------------------------------------------------//
//
//  Dictionary.h  - Various single and multi-key dictionary collections
//                  used by the Property Tree Control or its subelements.
//
//-------------------------------------------------------------------------//

#ifndef __DICTONARY_H__
#define __DICTONARY_H__

//-------------------------------------------------------------------------//
//  Forwards
class CPropertyTreeItem ;
class CPropertyFolder ;
class CProperty ;
#include "DictBase.h"
#include "PriorityLst.h"
#include "propVar.h"
#include "PTserver.h"

//-------------------------------------------------------------------------//
//  Property tree item type: Folder or Property
enum PROPTREE_ITEM_TYPE
{
    PIT_NIL,
    PIT_FOLDER,
    PIT_PROPERTY
} ;

//-------------------------------------------------------------------------//
//  Helper functions
ULONG VariantHash( const VARIANT& val ) ;


//-------------------------------------------------------------------------//
//  Collection of CPropertyFolder items, keyed on PROPERTY_FOLDER_ID.
typedef TDictionaryBase< PFID, CPropertyFolder*, 7 /*buckets*/ >  
        CFolderDictionaryBase ;

class   CFolderDictionary : public CFolderDictionaryBase
//-------------------------------------------------------------------------//
{
public:
    CFolderDictionary()
        :   CFolderDictionaryBase( 4 /*allocation block size*/ ) {}
//  Overrides of TDictionaryBase<>    
protected:
	virtual ULONG HashKey( const PFID& key ) const { return HashBytes( &key, sizeof(key) ) ; }
    virtual void OnDelete( PFID& pfid, CPropertyFolder*& pFolder ) ;
} ;

//-------------------------------------------------------------------------//
//  Collection of CPropertyFolder items, keyed on PROPERTYUID.
typedef TDictionaryBase< CPropertyUID, CProperty* /*default # of buckets*/ >  
        CPropertyDictionaryBase ;

class   CPropertyDictionary 
    :   public CPropertyDictionaryBase
//-------------------------------------------------------------------------//
{
public:
    CPropertyDictionary() 
        :   CPropertyDictionaryBase( 8 /*allocation block size*/ ) {}

//  Overrides of TDictionaryBase<>    
protected:
	virtual ULONG HashKey( const CPropertyUID& key ) const { 
        return key.Hash() ;
    }
    void OnDelete( CPropertyUID& key, CProperty*& val ) ;
} ;

//-------------------------------------------------------------------------//
//  Property source types:
typedef CComVariant* LPCOMVARIANT ;
typedef CPropVariant* LPCOMPROPVARIANT ;

typedef struct tagPTSERVER  {
    IAdvancedPropertyServer* pServer ;
    LPARAM               lParamSrc ;
} PTSERVER, *PPTSERVER, *LPPTSERVER ;

//-------------------------------------------------------------------------//
//  File extension wrapper
typedef struct tagPTFILEEXT
{
    TCHAR _szExt[MAX_PATH/2] ;


    struct tagPTFILEEXT() { *_szExt = 0 ; }
    struct tagPTFILEEXT( LPCTSTR pszExt ) { 
        if( pszExt ) lstrcpyn( _szExt, pszExt, ARRAYSIZE(_szExt) ) ; 
        else *_szExt = 0 ;
    }
    BOOL operator == ( const struct tagPTFILEEXT& other ) const   {
        return 0 == lstrcmpi( _szExt, other._szExt ) ;
    }
    ULONG Hash() const    {
        return HashStringi( (LPTSTR)_szExt ) ;
    }
} PTFILEEXT, *PPTFILEEXT, *LPPTFILEEXT ;

//-------------------------------------------------------------------------//
//  Collection of CComVariant* to PTSERVER mappings.
//  Contained in CPropertyTreeCtl object to serve as master source dictionary.
typedef TDictionaryBase< PTFILEEXT, IAdvancedPropertyServer* >  
        CFileAssocMapBase ;

class CFileAssocMap : public CFileAssocMapBase
//-------------------------------------------------------------------------//
{
public:
    CFileAssocMap() 
        :   CFileAssocMapBase( 4 /*allocation block size*/ ) {}

    BOOL    Lookup( LPCTSTR pszExt, IAdvancedPropertyServer*& pServer ) const ;
    BOOL    Insert( LPCTSTR pszExt, IAdvancedPropertyServer* pServer ) ;

protected:
	virtual ULONG HashKey( const struct tagPTFILEEXT& ext ) const { 
        return ext.Hash() ;
    }
    virtual BOOLEAN IsEqual( PTFILEEXT& key1, PTFILEEXT& key2, BOOLEAN& bHandled ) const { 
        bHandled = TRUE ;  return (BOOLEAN)(key1 == key2) ;
    }        
    virtual void OnDelete( PTFILEEXT& pKey, IAdvancedPropertyServer*& pData ) ;
} ;

//-------------------------------------------------------------------------//
//  Collection of CComVariant* to PTSERVER mappings.
//  Contained in CPropertyTreeCtl object to serve as master source dictionary.
typedef TDictionaryBase< LPCOMVARIANT, PTSERVER >  
        CMasterSourceDictionaryBase ;

class CMasterSourceDictionary : public CMasterSourceDictionaryBase
//-------------------------------------------------------------------------//
{
public:
    CMasterSourceDictionary() 
        :   CMasterSourceDictionaryBase( 4 /*allocation block size*/ ) {}

    HRESULT Insert( const VARIANT* pvarSrc, IAdvancedPropertyServer* pServer, LPARAM lParamSrc ) ;
    void    Delete( const VARIANT* pvarSrc ) ;

protected:
	virtual ULONG HashKey( const LPCOMVARIANT& pvarSrc ) const { 
        ASSERT( pvarSrc ) ;
        return VariantHash( *pvarSrc ) ;
    }
    virtual BOOLEAN IsEqual( const LPCOMVARIANT& key1, const LPCOMVARIANT& key2, BOOLEAN& bHandled ) const { 
        bHandled = TRUE ;  return (key1 && key2) ? (BOOLEAN)key1->operator==( *key2 ) : (BOOLEAN)FALSE ;
    }        
    virtual void OnDelete( LPCOMVARIANT& pvarSrc, PTSERVER& serverInfo ) ;
} ;

//-------------------------------------------------------------------------//
//  Collection of CComVariant* to CPropVariant* mappings.
//  Contained in CProperty object to track host sources.
typedef TDictionaryBase< LPCOMVARIANT, LPCOMPROPVARIANT >  CValueDictionaryBase ;
class CValueDictionary : public CValueDictionaryBase
//-------------------------------------------------------------------------//
{
public:
    CValueDictionary() {}
    virtual ULONG HashKey( const LPCOMVARIANT& pvarSrc ) const  {
        return VariantHash( *pvarSrc ) ;
    }
    virtual BOOLEAN IsEqual( const LPCOMVARIANT& key1, const LPCOMVARIANT& key2, BOOLEAN& bHandled ) const { 
        bHandled = TRUE ;  return (key1 && key2) ? (BOOLEAN)key1->operator==( *key2 ) : (BOOLEAN)FALSE ;
    }        
    virtual void OnDelete( LPCOMVARIANT& key, LPCOMPROPVARIANT& val ) ;
} ;

//-------------------------------------------------------------------------//
//  Collection of CComVariant* to ULONG mappings.
//  Contained in CPropertyFolder object to track host sources.
typedef TDictionaryBase< LPCOMVARIANT, LPARAM >  CSourceDictionaryLiteBase ;
class CSourceDictionaryLite : public CSourceDictionaryLiteBase
//-------------------------------------------------------------------------//
{
public:
    CSourceDictionaryLite() {}
    virtual ULONG HashKey( const LPCOMVARIANT& pvarSrc ) const  {
        ASSERT( pvarSrc ) ;
        return VariantHash( *pvarSrc ) ;
    }
    virtual BOOLEAN IsEqual( const LPCOMVARIANT& key1, const LPCOMVARIANT& key2, BOOLEAN& bHandled ) const { 
        bHandled = TRUE ;  return (key1 && key2) ? (BOOLEAN)key1->operator==( *key2 ) : (BOOLEAN)FALSE ;
    }        
    virtual void OnDelete( LPCOMVARIANT& key, LPARAM& val ) ;
} ;

#if 0 
//-------------------------------------------------------------------------//
//  Collection of property selection values mapped to user-friendly display text.
//  Used to maintain MRU and enum lists.
typedef TDictionaryBase< PCOMPROPVARIANT, BSTR > CSelectionListBase ;
class CSelectionList : public CSelectionListBase
//-------------------------------------------------------------------------//
{
public:
    CSelectionList()
        :   CSelectionListBase( 8 /*allocation block size*/ ) {}

//  Overrides of TDictionaryBase<>    
protected:
	virtual ULONG HashKey( const PCOMPROPVARIANT& key ) const ;
    virtual void OnDelete( PCOMPROPVARIANT& pPropVar, BSTR& bstrDisplayText ) ;
} ;

//-------------------------------------------------------------------------//
inline ULONG CSelectionList::HashKey( const PCOMPROPVARIANT& key ) const  {
    if( key ) return ( key->Hash() ) ; 
    return 0L ;
}
inline void CSelectionList::OnDelete( PCOMPROPVARIANT& pPropVar, BSTR& bstrDisplayText )   {
    if( pPropVar )        delete pPropVar ;
    if( bstrDisplayText ) SysFreeString( bstrDisplayText ) ;
}
#endif

//-------------------------------------------------------------------------//
//  Collection of property selection values mapped to user-friendly display text.
//  Used to maintain MRU and enum lists.
typedef TPriorityList< PDISPLAYPROPVARIANT > CSelectionListBase ;
class CSelectionList : public CSelectionListBase
//-------------------------------------------------------------------------//
{
public:
    CSelectionList()
        :   CSelectionListBase( 8 /*allocation block size*/ ) {}

    BOOLEAN Lookup( IN const PDISPLAYPROPVARIANT key, OUT PDISPLAYPROPVARIANT* ppMatch = NULL ) const ;

//  Overrides of TPriorityList<>    
protected:
    virtual void OnFreeElement(  PDISPLAYPROPVARIANT& p )    {
        if( p ) {
            delete p ; p = NULL ;
        }
    }
} ;

#endif  __DICTONARY_H__
