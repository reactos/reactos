//-------------------------------------------------------------------------//
//
//	MruProp.h
//
//-------------------------------------------------------------------------//
#ifndef __MRUPROP_H__
#define __MRUPROP_H__

#include "ptserver.h"
const BYTE MRU_COUNT = 20 ;  // nice if this stays 8-byte aligned.

//-------------------------------------------------------------------------//
//  Maintains PROPVARIANT mru lists to and from the registry.
//  Important note: The iteration features of this class and lack of 
//  thread synchronization limit its use to the stack on an as-needed basis.  
//  Instances should never be static, global, a data member of a container 
//  class, nor shared among or passed between threads.
//
class CPropMruStor
//-------------------------------------------------------------------------//
{
public:
    //  Construction
    CPropMruStor() ;
    CPropMruStor( HKEY hKeyRoot, LPCTSTR pszSubKey ) ;
    virtual ~CPropMruStor() ;

    //  Reference counting
    LONG    AddRef()          { return InterlockedIncrement( &m_cRef ) ; }
    LONG    Release()         { return InterlockedDecrement( &m_cRef ) ; }
    LONG    GetRef() const    { return m_cRef ; }

    //  Initialization
    HRESULT SetRoot( HKEY hKeyRoot, LPCTSTR pszRootSubKey ) ;

    //  Assignment methods
    HRESULT PushMruVal( REFFMTID fmtid, PROPID propid, VARTYPE vt, IN const PROPVARIANT& var ) ;
    
    //  Enumeration methods
    HRESULT BeginFetch( REFFMTID fmtid, PROPID propid, VARTYPE vt ) ;
    HRESULT FetchNext( LONG& iStart, PROPVARIANT_DISPLAY& var ) ;

    //  Attributes
    ULONG   IndexCount() const ;

#if 0 // (implemented, but not used)
    HANDLE  EnumFirst( REFFMTID fmtid, PROPID propid, VARTYPE vt, OUT PROPVARIANT& var ) ;
    BOOL    EnumNext( HANDLE hEnum, PROPVARIANT& var ) const ;
    void    EndEnum( HANDLE& hEnum ) const ;
#endif

protected:
    //  Impelementation helpers
    void    Construct() ;
    HRESULT Open( REGSAM = KEY_READ|KEY_WRITE ) ;
    void    Close() ;
    BOOL    IsOpen() const ;
    HRESULT LoadIndex( PVOID pvIndex ) ;
    HRESULT UpdateIndex() ;
    ULONG   GetRecNo( ULONG iPos ) const ;
    ULONG   GetUnusedRecNo() const ;
    BOOL    GetRecTitle( ULONG iPos, LPTSTR pszBuf ) const ;

    HRESULT FindVal( IN const PROPVARIANT& var, OUT ULONG& iPos ) ;
    HRESULT PromoteMruVal( IN ULONG iPos ) ;
    HRESULT PopMruVal() ;

    void    ClearRoot() ;
    void    ClearRecKeyName() ;
    HRESULT MakeRecKeyName( REFFMTID fmtid, PROPID propid, VARTYPE vt ) ;

    //  Internal structures
    struct ENUM {
        ULONG   iPos ;
    } ;

    struct INDEX    {
        ULONG   nRec ;
        ULONG   dwHash ;
    } ;

    //  Data
    HKEY    m_hKeyRoot ;
    HKEY    m_hKey ;
    LPTSTR  m_pszRootSubKey,
            m_pszRecSubKey ;
    INDEX   m_index[MRU_COUNT] ;
    LONG    m_cRef ;
} ;

//-------------------------------------------------------------------------//
inline ULONG CPropMruStor::IndexCount() const
{
    return sizeof(m_index)/sizeof(INDEX) ;
}
inline BOOL CPropMruStor::IsOpen() const
{
    return m_hKey != NULL ;
}


#endif __MRUPROP_H__