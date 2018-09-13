//-------------------------------------------------------------------------//
//
//  DictBase.h
//
//  Copyright © 1997, Scott R. Hanggie
//  All rights under U.S. and international trademark and copyright 
//  law reserved.
//
//-------------------------------------------------------------------------//

#ifndef __DICTBASE_H__
#define __DICTBASE_H__

//  Forwards
//------------//
#include "CPool.h"

#if defined( _NTDDK_ ) || defined( _NTIFS_ )
#   define NT_KERNELMODE
//# pragma message( "\t\tTDictionary.h - NT Kernel Mode build" )
#elif defined( _WIN32 )
#   define WIN32_USERMODE
//# pragma message( "\t\tTDictionary.h - Win32 User Mode build" )
#endif

//-------------------------------------------------------------------------//
//  Hash value generators for common types.
//  You can use these in your dictionary class's implementation of 
//  the hashing routine.

//-------------------------------//
//  Unsigned long has
ULONG HashUlong( ULONG ) ;
//-------------------------------//
//  Unsigned long has
ULONG HashPointer( void* ) ;
//-------------------------------//
//  Case-sensitive ANSI string hash
ULONG HashStringA( char* ) ;
//-------------------------------//
//  Case-insensitive ANSI string hash
ULONG HashStringiA( char* ) ;
//-------------------------------//
//  Case-sensitive Unicode string hash
ULONG HashStringW( WCHAR* ) ;
//-------------------------------//
//  Case-insensitive Unicode string hash
ULONG HashStringiW( WCHAR* ) ;
//-------------------------------//
//  Hashes a byte array.
ULONG HashBytes( BYTE [], int cb ) ;
//-------------------------------//
//  System Address hash
#ifdef NT_KERNELMODE
ULONG HashSystemAddr( PVOID ) ;
#endif

#if defined(UNICODE) || defined(_UNICODE)
#define HashString      HashStringW
#define HashStringi     HashStringiW
#else //UNICODE
#define HashString      HashStringA
#define HashStringi     HashStringiA
#endif //UNICODE

//-------------------------------//
//  Comparison helpers
#define NUMERIC_COMPARE( a, b ) (((a)<(b)) ? -1 : ((a)>(b)) ? 1 : 0 )

//-------------------------------------------------------------------------//
//  TDictionaryBase Template macros
#define TDICTIONARYBASE_ARGLIST <class KEY_TYPE, class VALUE_TYPE, ULONG cBuckets>
#define TDICTIONARYBASE TDictionaryBase< KEY_TYPE, VALUE_TYPE, cBuckets>

//-------------------------------------------------------------------------//
//  The TDictionaryBase class - Simple hash table collection class.
template <class KEY_TYPE, class VALUE_TYPE, ULONG cBuckets=17>
class TDictionaryBase
//-------------------------------------------------------------------------//
{
public:
//  Construction, Destruction
#ifdef WIN32_USERMODE
    TDictionaryBase( ULONG nBlockSize = 10 )  ;
#endif

#ifdef NT_KERNELMODE
    TDictionaryBase( POOL_TYPE poolType, ULONG nBlockSize = 10 )  ;
#endif

    virtual ~TDictionaryBase()  ;

//  Attributes
    // number of elements
    int     Count() const ;
    ULONG   BucketCount() const ;
    BOOLEAN IsEmpty() const ;

//  Thread Synchronization methods
    //  Acquires a mutual exclusion lock on the dictionary
    //  This should be done immediately prior to each call to
    //  a non-const method.
    void AcquireMutex() ;
    //  Releases a mutual exclusion lock on the dictionary
    //  This should be done immediately following each call to
    //  a non-const method.
    void ReleaseMutex() ;

//  Data Access Methods
    //  assigns a key-value pair. (e.g., dict[key] = value)
    VALUE_TYPE& operator[]( const KEY_TYPE& key ) ;

    //  Determines whether an entry matching the specified key exists in the dictionary.
    BOOLEAN Exists( const KEY_TYPE& key ) const ;

    //  finds the value corresponding to the specified key.
    BOOLEAN     Lookup( const KEY_TYPE& key, VALUE_TYPE& value ) const ;
    VALUE_TYPE& Value( const KEY_TYPE& key ) const ;

    //  clears dictionary contents
    void    Clear() ;

    //  removes an existing entry
    BOOLEAN DeleteKey( const KEY_TYPE& key ) ;

    //  enumerates key-value entries
    HANDLE  EnumFirst( KEY_TYPE& key, VALUE_TYPE& value ) const ;
    BOOLEAN EnumNext( HANDLE hEnum, KEY_TYPE& key, VALUE_TYPE& value ) const ;
    void    EndEnum( HANDLE hEnum ) ;

    //  Hashes keys.  Specialized implementation to be provided by
    //  derived class; default hashing is handled by base class.
    virtual ULONG   HashKey( const KEY_TYPE& key ) const = 0 ;
    virtual BOOLEAN IsEqual( const KEY_TYPE&, const KEY_TYPE&, BOOLEAN& bHandled ) const ;
    virtual void    OnDelete( KEY_TYPE& key, VALUE_TYPE& value ) {
        UNREFERENCED_PARAMETER( key ) ;
        UNREFERENCED_PARAMETER( value ) ;
    }

// Implementation
protected:
    
    //  hash entry type
    struct _entry   {
        _entry* pNext ; // collision chain.
        KEY_TYPE key ;  
        VALUE_TYPE value ;
    } ;

    //  enumeration block
    struct _enum    {
        ULONG   cbStruct ;
        _entry* pEntry ;
        ULONG   nBucket ;
    } ;

    //  data members
    _entry              *m_hashtable[cBuckets],
                        *m_pFreeList ;
    ULONG               m_cEntries ;

    struct CPool*       m_pBlocks ;
    ULONG               m_nBlockSize ;

#ifdef WIN32_USERMODE
    CRITICAL_SECTION    m_critsect ;
#endif

#ifdef NT_KERNELMODE 
    POOL_TYPE           m_poolType ;
    BOOLEAN             m_fPassive ;
    KSPIN_LOCK          m_spinLock ;
    KIRQL               m_spinLockIrql ;
    FAST_MUTEX          m_fastmutex ;
#endif

    //  entry helpers
    _entry* AllocEntry() ;
    void    FreeEntry( _entry* ) ;

    //  retrieves first entry in bucket.
    _entry* Entry( const KEY_TYPE&, ULONG& nBucket, ULONG* pnHash = NULL ) const ;

    //  determines whether the specified keys match.
    BOOLEAN KeysMatch( const KEY_TYPE& key1, const KEY_TYPE& key2 ) const ;
} ;

//-------------------------------------------------------------------------//
//  TMultiKeyDictionaryBase Template macros
#define TMULTIKEYDICTIONARYBASE_ARGLIST <class VALUE_TYPE, UCHAR cKeys, ULONG cBuckets>
#define TMULTIKEYDICTIONARYBASE TMultiKeyDictionaryBase<VALUE_TYPE, cKeys, cBuckets>

//-------------------------------------------------------------------------//
//  The TMultiKeyDictionaryBase class 
//  Hash table collection with multiple key searches.
template < class VALUE_TYPE, UCHAR cKeys, ULONG cBuckets=17 >
class TMultiKeyDictionaryBase
//-------------------------------------------------------------------------//
{
public:
//  Construction, Destruction

#ifdef WIN32_USERMODE
    TMultiKeyDictionaryBase( ULONG nBlockSize = 16 ) ; // number of dictionary items to allocate at a time
#endif

#ifdef NT_KERNELMODE
    TMultiKeyDictionaryBase( POOL_TYPE poolType,       //   memory type
                             ULONG nBlockSize = 16 )  ;//   number of dictionary items to allocate at a time
#endif

    virtual ~TMultiKeyDictionaryBase()  ;

//  Attributes
    // number of elements
    int     Count() const ;
    ULONG   BucketCount() const ;
    BOOLEAN IsEmpty() const ;

//  Methods
    
    //  Thread safety: acquires dictionary's mutual exclusion rights
    //  This should be done immediately prior to each call to
    //  a non-const method.
    void AcquireMutex() ;

    //  Thread safety: releases dictionary's mutual exclusion rights
    //  This should be done immediately following each call to
    //  a non-const method.
    void ReleaseMutex() ;

    //  Inserts a value element into the dictionary
    BOOLEAN Insert( const VALUE_TYPE& val ) ;

    //  Determines whether an entry matching the specified key exists in the dictionary.
    BOOLEAN Exists( UCHAR iKey, PVOID pvKey ) const ;

    //  finds the value corresponding to the specified key.  If the
    //  key supports duplicates, returns the most recently inserted value.
    BOOLEAN Lookup( UCHAR iKey, PVOID pvkey, VALUE_TYPE& value ) const ;

    //  Enumerates the value elements which match on the indicated key.
    HANDLE  LookupFirst( UCHAR iKey, PVOID pvKey, VALUE_TYPE& value ) const ;
    BOOLEAN LookupNext( HANDLE hLookup, VALUE_TYPE& value ) const ;
    void    EndLookup( HANDLE hLookup ) const ;

    //  clears dictionary contents
    void    Clear() ;

    //  removes an existing entry
    BOOLEAN Delete( const VALUE_TYPE& val ) ;
    BOOLEAN Delete( UCHAR iKey, PVOID pvkey ) ;

//  Mandatory overridables:
protected:
    //  Should return the hash value for the indicated key based on the provided value element,
    //  or (ULONG)-1 if the hash value cannot be generated.
    virtual ULONG   HashValue( UCHAR iKey, const VALUE_TYPE& val ) const=0 ;
    
    //  Should return the hash value for the indicated key based on the provided key value.
    //  or (ULONG)-1 if the hash value cannot be generated.
    virtual ULONG   HashKey( UCHAR iKey, const void* pvKey ) const=0 ;
    
    //  Should return -1 if the key's value is less than that provided by the value element, 1 if
    //  the key's value is greater that provided by the provided element, or 0 if they are equal.
    virtual int     Compare( UCHAR iKey, const void* pvKey, const VALUE_TYPE& val ) const=0 ;

    //  Should return the address of the indicated key's value based on the
    //  provided value element, or NULL if the address cannot be determined.
    virtual PVOID   GetKey( UCHAR iKey, const VALUE_TYPE& val ) const=0 ;

    //  Should return TRUE if the indicated key supports duplicates, otherwise FALSE.
    //  Note: it is important that this function should return consistent values 
    //  throughout the lifetime of the object; failure to do so will result in
    //  unpredictable search and delete results.
    virtual BOOLEAN AllowDuplicates( UCHAR iKey ) const=0 ;

//  Optional overrideables
protected:

    //  Allows the derived class to perform any cleanup on the element prior to freeing.
    virtual void  OnDelete( VALUE_TYPE& val ) {
        UNREFERENCED_PARAMETER( val ) ;
    }

// Implementation
private:
    //  Hash entry type
    struct HASH_ENTRY   {
        HASH_ENTRY* pNext ; // collision chain.
    } ;

    //  In-memory storage for each dictionary item
    struct ITEM {
        HASH_ENTRY  entries[cKeys] ;
        VALUE_TYPE  value ;
    } ;

    //  Iterative lookup block (see LookupFirst()/LookupNext()/EndLookup() impl).
    struct LOOKUP    {
        WORD        cbStruct ;
        UCHAR       iKey ;
        BOOLEAN     fDupl ;
        ULONG       bucket ;
        PVOID       pvKey ;
        HASH_ENTRY  *pEntry,
                    *pNext ;
        ULONG       Reserved0 ;
    } ;

    //  data members
    HASH_ENTRY          *m_hashmatrix[cKeys][cBuckets] ;
    HASH_ENTRY          *m_pFreeList ;
    ULONG               m_cEntries ;

    struct CPool        *m_pBlocks ;
    ULONG               m_nBlockSize ;

#ifdef WIN32_USERMODE
    CRITICAL_SECTION    m_critsect ;
#endif

#ifdef NT_KERNELMODE 
    POOL_TYPE           m_poolType ;
    BOOLEAN             m_fPassive ;
    KSPIN_LOCK          m_spinLock ;
    KIRQL               m_spinLockIrql ;
    FAST_MUTEX          m_fastmutex ;
#endif

    //  storage helpers
    ITEM*   AllocItem() ;
    void    FreeItem( ITEM* ) ;

    //  Search helpers...

    //  Returns address of a dictionary item
    VALUE_TYPE* _Lookup( UCHAR iKey, PVOID pvKey ) const ;
    //  Calculates the bucket number for the indicated key
    BOOLEAN GetBucket( UCHAR iKey, PVOID pvKey, ULONG& bucket ) const ;
    //  Calculates the bucket number for the indicated key and value
    BOOLEAN GetBucket( UCHAR iKey, const VALUE_TYPE& val, ULONG& bucket ) const ;
    //  Calculates bucket numbers for all keys
    BOOLEAN GetBuckets( const VALUE_TYPE& val, ULONG* buckets ) const ;
    //  Returns a value based on the key index and hash entry 
    VALUE_TYPE* GetValueFromEntry( UCHAR iKey, HASH_ENTRY* pEntry ) const ;
    //  Returns an item block based on the key index and hash entry
    ITEM* GetItemFromEntry( UCHAR iKey, HASH_ENTRY* pEntry ) const ;
    //  retrieves first entry in bucket.
    BOOLEAN CanInsertItem( const VALUE_TYPE& val, ULONG* buckets ) const ;
    //  Does the work of removing an entry from the dictionary.
    BOOLEAN Delete( HASH_ENTRY* pEntry ) ;
} ;

//-------------------------------------------------------------------------//
//  TDictionaryBase implementation
//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
#ifdef WIN32_USERMODE
    TDICTIONARYBASE::TDictionaryBase( ULONG nBlockSize )
#endif WIN32_USERMODE
#ifdef NT_KERNELMODE
    TDICTIONARYBASE::TDictionaryBase( POOL_TYPE poolType, ULONG nBlockSize )
#endif
{
    ASSERT( nBlockSize > 0 ) ;

    RtlZeroMemory( m_hashtable, sizeof(m_hashtable) ) ;
    m_cEntries = 0 ;
    m_pFreeList = NULL ;
    m_pBlocks = NULL ;
    m_nBlockSize = nBlockSize ;

#ifdef WIN32_USERMODE
    InitializeCriticalSection( &m_critsect ) ;
#endif
#ifdef NT_KERNELMODE
    m_poolType = poolType ;
    m_fPassive = poolType==PagedPool ||
                 poolType==PagedPoolCacheAligned ;
    m_spinLockIrql = PASSIVE_LEVEL ;
    if( m_fPassive )    {
        ExInitializeFastMutex( &m_fastmutex ) ;
    }
    else    {
        KeInitializeSpinLock( &m_spinLock ) ;
    }
#endif
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
inline TDICTIONARYBASE::~TDictionaryBase()
{
    Clear() ;
    ASSERT( m_cEntries == 0 ) ;

#ifdef WIN32_USERMODE
    DeleteCriticalSection( &m_critsect ) ;
#endif
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
void TDICTIONARYBASE::AcquireMutex()
{
#ifdef WIN32_USERMODE
    EnterCriticalSection( &m_critsect ) ;   
#endif

#ifdef NT_KERNELMODE 
    if( m_fPassive )
        ExAcquireFastMutex( &m_fastmutex ) ;
    else
        KeAcquireSpinLock( &m_spinLock, &m_spinLockIrql ) ;
#endif
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
void TDICTIONARYBASE::ReleaseMutex()
{
#ifdef WIN32_USERMODE
    LeaveCriticalSection( &m_critsect ) ;   
#endif

#ifdef NT_KERNELMODE 
    if( m_fPassive )
        ExReleaseFastMutex( &m_fastmutex ) ;
    else
        KeReleaseSpinLock( &m_spinLock, m_spinLockIrql ) ;
#endif
}


//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
inline int TDICTIONARYBASE::Count() const   { 
    return m_cEntries ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
inline BOOLEAN TDICTIONARYBASE::IsEmpty() const { 
    return m_cEntries == 0  ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
inline ULONG TDICTIONARYBASE::BucketCount() const   { 
    return cBuckets ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
inline ULONG TDICTIONARYBASE::HashKey( const KEY_TYPE& key ) const
{
    // default identity hash - works for most primitive values
    return ( *(ULONG*)&key ) >> 4 ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
void TDICTIONARYBASE::Clear()
{
    HANDLE      hEnum ;
    BOOLEAN     bEnum ;
    KEY_TYPE    key ;
    VALUE_TYPE  val ;

    for( hEnum = EnumFirst( key, val ), bEnum = TRUE ;
         hEnum && bEnum ;
         bEnum = EnumNext( hEnum, key, val ) )
    {
        OnDelete( key, val ) ;
    }
    EndEnum( hEnum ) ;
    
    m_cEntries = 0 ;
    m_pFreeList = NULL ;
    m_pBlocks->FreeChain() ;
    m_pBlocks = NULL ;
    RtlZeroMemory( m_hashtable, sizeof(m_hashtable) ) ;
}

//-------------------------------------------------------------------------//
// Entry allocation helper
template TDICTIONARYBASE_ARGLIST
TDICTIONARYBASE::_entry* TDICTIONARYBASE::AllocEntry()
{
    if( m_pFreeList == NULL )
    {

        // add another block

#ifdef WIN32_USERMODE
        CPool* newBlock = CPool::Create( m_pBlocks, m_nBlockSize, sizeof( TDICTIONARYBASE::_entry ) ) ;
#endif

#ifdef NT_KERNELMODE
        CPool* newBlock = CPool::Create( m_poolType, m_pBlocks, m_nBlockSize, sizeof( TDICTIONARYBASE::_entry ) ) ;
#endif
        
        // chain them into free list
        TDICTIONARYBASE::_entry* pEntry = ( TDICTIONARYBASE::_entry* ) newBlock->data() ;
        
        // free in reverse order to make it easier to debug
        pEntry += m_nBlockSize - 1 ;
        for ( int i = m_nBlockSize-1 ; i >= 0 ; i--, pEntry-- )
        {
            pEntry->pNext = m_pFreeList ;
            m_pFreeList = pEntry ;
        }
    }
    ASSERT( m_pFreeList != NULL ) ;  // we must have a list of free entries!

    TDICTIONARYBASE::_entry* pEntry = m_pFreeList ;
    m_pFreeList = m_pFreeList->pNext ;
    m_cEntries++ ;
    ASSERT( m_cEntries > 0 ) ;      // make sure we don't overflow

    RtlZeroMemory( &pEntry->key, sizeof(pEntry->key) ) ;
    RtlZeroMemory( &pEntry->value, sizeof(pEntry->value) ) ;
    return pEntry ;
}

//-------------------------------------------------------------------------//
// Entry deallocation helper
template TDICTIONARYBASE_ARGLIST
void TDICTIONARYBASE::FreeEntry( TDICTIONARYBASE::_entry* pEntry )
{
    OnDelete( pEntry->key, pEntry->value ) ;
    pEntry->pNext = m_pFreeList ;
    m_pFreeList = pEntry ;
    m_cEntries-- ;
    ASSERT( m_cEntries >= 0 ) ;  // make sure we don't underflow

    // if no more elements, cleanup completely
    if( m_cEntries == 0 )
        Clear() ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
BOOLEAN TDICTIONARYBASE::KeysMatch( const KEY_TYPE& key1, const KEY_TYPE& key2 ) const
{
    BOOLEAN bHandled = FALSE,
            fMatch = IsEqual( key1, key2, bHandled ) ;

    if( bHandled )
        return fMatch ;
    
    return (BOOLEAN)(key1 == key2) ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
TDICTIONARYBASE::_entry* TDICTIONARYBASE::Entry( const KEY_TYPE& key, ULONG& nBucket, OPTIONAL ULONG* pnHash ) const
{
    ULONG nHash = HashKey( key ) ;
    nBucket = nHash % cBuckets ;

    if( pnHash ) *pnHash = nHash ;
    
    // see if it exists
    _entry* pEntry ;

    for ( pEntry = m_hashtable[nBucket] ; pEntry != NULL ; pEntry = pEntry->pNext )
    {
        if( KeysMatch( pEntry->key, key ) )
            return pEntry ;
    }
    return NULL ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
VALUE_TYPE& TDICTIONARYBASE::Value( const KEY_TYPE& key ) const
// find value ( or return NULL -- NULL values not different as a result )
{
    ULONG nBucket = HashKey( key ) % cBuckets ;

    // see if it exists
    _entry* pEntry ;
    for ( pEntry = m_hashtable[nBucket] ; pEntry != NULL ; pEntry = pEntry->pNext )
    {
        if( KeysMatch( pEntry->key, key ) )
            return pEntry->value ;
    }
    return NULL ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
BOOLEAN TDICTIONARYBASE::Exists( const KEY_TYPE& key ) const
{
    ULONG nBucket ;
    return Entry( key, nBucket ) != NULL ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
BOOLEAN TDICTIONARYBASE::Lookup( const KEY_TYPE& key, VALUE_TYPE& value ) const
{
    ULONG nBucket ;
    _entry* pEntry = Entry( key, nBucket ) ;
    if( pEntry == NULL )
        return FALSE ;  // not in map

    value = pEntry->value ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
VALUE_TYPE& TDICTIONARYBASE::operator[]( const KEY_TYPE& key )
{
    ULONG nBucket, nHash = 0 ;
    _entry* pEntry ;

    //  If the hash table is empty...
    if( ( pEntry = Entry( key, nBucket, &nHash ) ) == NULL )
    {
        // it doesn't exist, so create a new Association
        if( (pEntry = AllocEntry()) != NULL )
        {
            pEntry->key = key ;

            // Insert into table
    #ifdef TRACE_DICTIONARY
            if( m_hashtable[nBucket] )  {
                TRACE( TEXT("TDictionaryBase[Key=%08lX] collision at bucket %ld, hash value %ld\n"), 
                       *(ULONG*)&key, nBucket, nHash ) ;
            }
    #endif TRACE_DICTIONARY

            pEntry->pNext = m_hashtable[nBucket] ;  // chain collision .
            m_hashtable[nBucket] = pEntry ;     // assign to table.
        }
    #ifdef TRACE_DICTIONARY
        else
        {
            TRACE( TEXT("TDICTIONARYBASE::operator[] failed allocation\n") ) ;
        }
    #endif TRACE_DICTIONARY
    }
#ifdef TRACE_DICTIONARY
    else
    {
        TRACE( TEXT("TDictionaryBase::operator[] - collision on assignment\n") ) ;
    }
#endif TRACE_DICTIONARY ;

    return pEntry->value ;  // return new reference
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
BOOLEAN TDICTIONARYBASE::DeleteKey( const KEY_TYPE& key )
// remove key - return TRUE if removed
{
    _entry** ppEntryPrev ;
    ppEntryPrev = &m_hashtable[ HashKey(key) % cBuckets ] ;

    _entry* pEntry ;
    for ( pEntry = *ppEntryPrev ; pEntry != NULL ; pEntry = pEntry->pNext )
    {
        if( KeysMatch( pEntry->key, key ) )
        {
            // remove it
            *ppEntryPrev = pEntry->pNext ;  // remove from list
            FreeEntry( pEntry ) ;

            return TRUE ;
        }
        ppEntryPrev = &pEntry->pNext ;
    }
    return FALSE ;  // not found
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
HANDLE TDICTIONARYBASE::EnumFirst( KEY_TYPE& key, VALUE_TYPE& value ) const
{ 
    _enum*  pEnum = NULL ;
    _entry*  pEntry = NULL ;

    //  Scan buckets for first association.
    for ( ULONG nBucket = 0 ; nBucket < cBuckets ; nBucket++ )
    {
        if( (pEntry = m_hashtable[nBucket]) != NULL )
        {
            if( (pEnum = new _enum) != NULL )
            {
                pEnum->cbStruct = sizeof(*pEnum) ;
                pEnum->pEntry = pEntry ;
                pEnum->nBucket= nBucket ;
                key     = pEntry->key ;
                value   = pEntry->value ;
                break ;
            }
        }
    }
    return pEnum ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
BOOLEAN TDICTIONARYBASE::EnumNext( HANDLE hEnum,
    KEY_TYPE& key, VALUE_TYPE& value ) const
{
    _enum*  pEnum = (_enum*)hEnum ;
    _entry* pEntryNext ;
    
    if( !(  pEnum && 
            pEnum->cbStruct==sizeof(*pEnum) && 
            pEnum->pEntry  ) )
    {
        RtlZeroMemory( &key, sizeof(key) ) ;
        RtlZeroMemory( &value, sizeof(value) ) ;
        return FALSE ;
    }

    //  check collision chain
    if( (pEntryNext = pEnum->pEntry->pNext)==NULL )
    {
        // empty or terminated, so go to next bucket
        for( pEnum->nBucket++ ; pEnum->nBucket < cBuckets ; pEnum->nBucket++ )
        {
            if( (pEntryNext = m_hashtable[pEnum->nBucket]) != NULL )
                break ;
        }   
    }

    if( !pEntryNext )
    {
        pEnum->pEntry   = NULL ;
        pEnum->nBucket  = cBuckets ;
        RtlZeroMemory( &key, sizeof(key) ) ;
        RtlZeroMemory( &value, sizeof(value) ) ;
        return FALSE ;
    }

    // copy out association
    pEnum->pEntry = pEntryNext ;
    key     = pEnum->pEntry->key ;
    value   = pEnum->pEntry->value ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
void TDICTIONARYBASE::EndEnum( HANDLE hEnum )
{
    _enum* pEnum = (_enum*)hEnum ;
    if( pEnum && pEnum->cbStruct==sizeof(*pEnum) )
    {
        RtlZeroMemory( pEnum, sizeof(*pEnum) ) ;
        delete pEnum ;
    }
}

//-------------------------------------------------------------------------//
template TDICTIONARYBASE_ARGLIST
inline BOOLEAN TDICTIONARYBASE::IsEqual( 
    const KEY_TYPE&, 
    const KEY_TYPE&, 
    BOOLEAN& ) const 
{ 
    return FALSE ;
}

//-------------------------------------------------------------------------//
//  TMultiKeyDictionaryBase implementation
//-------------------------------------------------------------------------//
#define VERIFY_VALID_KEY(key, ret)  if( !(key>=0 && key<cKeys) ) { ASSERT( FALSE ); return ret ; }
#define VERIFY_VALID_ENTRY(pEntry)  ASSERT( pEntry ? pEntry != pEntry->pNext : TRUE ) ;

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
#ifdef WIN32_USERMODE
TMULTIKEYDICTIONARYBASE::TMultiKeyDictionaryBase( ULONG nBlockSize )
#endif
#ifdef NT_KERNELMODE 
TMULTIKEYDICTIONARYBASE::TMultiKeyDictionaryBase( POOL_TYPE poolType, ULONG nBlockSize )
#endif
{
    ASSERT( nBlockSize > 0 ) ;
    
    RtlZeroMemory( m_hashmatrix, sizeof(m_hashmatrix) ) ;
    m_cEntries = 0 ;
    m_pFreeList = NULL ;
    m_pBlocks = NULL ;
    m_nBlockSize = nBlockSize ;

#ifdef WIN32_USERMODE
    InitializeCriticalSection( &m_critsect ) ;
#endif

#ifdef NT_KERNELMODE 
    m_poolType = poolType ;
    m_fPassive = poolType==PagedPool ||
                 poolType==PagedPoolCacheAligned ;
    m_spinLockIrql = PASSIVE_LEVEL ;
    if( m_fPassive )    {
        ExInitializeFastMutex( &m_fastmutex ) ;
    }
    else    {
        KeInitializeSpinLock( &m_spinLock ) ;
    }
#endif
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline TMULTIKEYDICTIONARYBASE::~TMultiKeyDictionaryBase()
{
    Clear() ;
    ASSERT( m_cEntries == 0 ) ;

#ifdef WIN32_USERMODE
    DeleteCriticalSection( &m_critsect ) ;
#endif
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline int TMULTIKEYDICTIONARYBASE::Count() const   { 
    return m_cEntries ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::IsEmpty() const { 
    return m_cEntries == 0  ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline ULONG TMULTIKEYDICTIONARYBASE::BucketCount() const   { 
    return cBuckets ;
}

//-------------------------------------------------------------------------//
// Entry allocation helper
template TMULTIKEYDICTIONARYBASE_ARGLIST
TMULTIKEYDICTIONARYBASE::ITEM* TMULTIKEYDICTIONARYBASE::AllocItem()
{
    TMULTIKEYDICTIONARYBASE::ITEM* pItem ;

    if( m_pFreeList == NULL )
    {
        // add another block

#ifdef WIN32_USERMODE
        CPool* newBlock = CPool::Create( m_pBlocks, m_nBlockSize, sizeof(TMULTIKEYDICTIONARYBASE::ITEM) ) ;
#endif

#ifdef NT_KERNELMODE 
        CPool* newBlock = CPool::Create( m_poolType, m_pBlocks, m_nBlockSize, sizeof( TMULTIKEYDICTIONARYBASE::ITEM ) ) ;
#endif
        // chain them into free list
        pItem = (ITEM*) newBlock->data() ;
        
        // free in reverse order to make it easier to debug
        pItem += m_nBlockSize - 1 ;
        for ( int i = m_nBlockSize-1 ; i >= 0 ; i--, pItem-- )
        {
            pItem->entries[0].pNext = m_pFreeList ;
            m_pFreeList = (HASH_ENTRY*)pItem ;
        }
    }
    ASSERT( m_pFreeList != NULL ) ;  // we must have a list of free entries!

    pItem = (ITEM*)m_pFreeList ;
    m_pFreeList = pItem->entries[0].pNext ;
    m_cEntries++ ;
    ASSERT( m_cEntries > 0 ) ;      // make sure we don't overflow

    return pItem ;
}

//-------------------------------------------------------------------------//
// Entry deallocation helper
template TMULTIKEYDICTIONARYBASE_ARGLIST
void TMULTIKEYDICTIONARYBASE::FreeItem( TMULTIKEYDICTIONARYBASE::ITEM* pItem )
{
    OnDelete( pItem->value ) ;
    
    //  shove onto free list
    pItem->entries[0].pNext = m_pFreeList ; 
    m_pFreeList = &pItem->entries[0] ;      

    //  update our count.
    m_cEntries-- ;
    ASSERT( m_cEntries >= 0 ) ;  // make sure we don't underflow

    // if no more elements, cleanup completely
    if( m_cEntries == 0 )
        Clear() ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
void TMULTIKEYDICTIONARYBASE::Clear()
{
    HASH_ENTRY* pEntry  ;
    ULONG       nBucket ;
    
    //  Give the derived class a crack at cleaning up his member elements.
    //  The number of entries for any given key should be exactly equal to the
    //  the number of ITEMs in the dictionary; so we'll arbitrarily use key 0 ...
    for( nBucket=0, pEntry = m_hashmatrix[0][nBucket]; 
         nBucket<cBuckets; 
         nBucket++, pEntry = m_hashmatrix[0][nBucket] )
    {
        while( pEntry )
        {
            //  Retrieve the entry's value.  
            ITEM* pItem ;
            if( (pItem = GetItemFromEntry( 0, pEntry ))!=NULL )
                OnDelete( pItem->value ) ;
            pEntry = pEntry->pNext ; //  Walk the collision chain at this bucket:
        }
    }
    
    //  Kill our pools and reinitialize the table.
    m_cEntries = 0 ;
    m_pFreeList = NULL ;
    m_pBlocks->FreeChain() ;
    m_pBlocks = NULL ;
    RtlZeroMemory( m_hashmatrix, sizeof(m_hashmatrix) ) ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::GetBucket( UCHAR iKey, PVOID pvKey, ULONG& bucket ) const
{
    VERIFY_VALID_KEY( iKey, FALSE ) ;
    if( !pvKey ) return FALSE ;

    ULONG nHash ;
    if( (nHash = HashKey( iKey, pvKey))==(ULONG)-1 )
        return FALSE ;

    bucket = nHash % cBuckets ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::GetBucket( UCHAR iKey, const VALUE_TYPE& val, ULONG& bucket ) const
{
    VERIFY_VALID_KEY( iKey, FALSE ) ;

    ULONG nHash ;
    if( (nHash = HashValue( iKey, val ))==(ULONG)-1 )
        return FALSE ;

    bucket = nHash % cBuckets ;
    return TRUE ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::GetBuckets( const VALUE_TYPE& val, ULONG* buckets ) const
{
    //  Want buckets? I give you buckets
    for( UCHAR iKey=0; iKey < cKeys; iKey++ )
    {
        if( !GetBucket( iKey, val, buckets[iKey] ) )
            return FALSE ;
    }
    return TRUE ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline VALUE_TYPE* TMULTIKEYDICTIONARYBASE::GetValueFromEntry( UCHAR iKey, HASH_ENTRY* pEntry ) const
{
    VERIFY_VALID_KEY( iKey, NULL ) ;
    VERIFY_VALID_ENTRY( pEntry ) ;

    //  Retrive address of value based on entry address and key number
    //  (see struct ITEM def)
    if( pEntry )
    {
        BYTE* pbVal = (BYTE*)&pEntry[-iKey] + FIELD_OFFSET( TMULTIKEYDICTIONARYBASE::ITEM, value ) ;
        return (VALUE_TYPE*)pbVal ;
    }
    return NULL ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline TMULTIKEYDICTIONARYBASE::ITEM* TMULTIKEYDICTIONARYBASE::GetItemFromEntry( UCHAR iKey, HASH_ENTRY* pEntry ) const
{
    VERIFY_VALID_KEY( iKey, NULL ) ;
    VERIFY_VALID_ENTRY( pEntry ) ;

    //  Retrive address of item based on entry address and key number
    return (ITEM*)(&pEntry[-iKey]) ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
BOOLEAN TMULTIKEYDICTIONARYBASE::CanInsertItem( const VALUE_TYPE& val, ULONG* pBuckets ) const
{
    ITEM*       pItem = NULL ;
    HASH_ENTRY* pEntry ;
    ULONG       buckets[cKeys] ;

    if( pBuckets==NULL ) pBuckets = buckets ;

    memset( pBuckets, -1, sizeof(buckets) ) ;

    //  Want buckets? I give you buckets
    if( !GetBuckets( val, pBuckets ) )
        return FALSE ;

    //  For each key...
    for( UCHAR iKey=0; iKey<cKeys; iKey++ )
    {
        //  If uniqueness is enforced on this key,..
        if( AllowDuplicates( iKey )==FALSE )
        {
            //  Retrieve address of key
            PVOID pvKey ;
            if( (pvKey = GetKey( iKey, val )) ==NULL )
                return FALSE ;

            //  Step through collision chain looking for a match.
            for( pEntry = m_hashmatrix[iKey][pBuckets[iKey]]; pEntry;
                 pEntry = pEntry->pNext )
            {
                VERIFY_VALID_ENTRY( pEntry ) ;

                VALUE_TYPE* pVal ;

                if( (pVal = GetValueFromEntry( iKey, pEntry )) != NULL &&
                    Compare( iKey, pvKey, *pVal )==0 )
                {
                    return FALSE ;
                }
            }
        }
    }
    return TRUE ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::Exists( UCHAR iKey, PVOID pvKey ) const
{
    return _Lookup( iKey, pvKey ) != NULL ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::Lookup( UCHAR iKey, PVOID pvKey, VALUE_TYPE& value ) const
{
    VALUE_TYPE* pRetVal ;

    if( (pRetVal = _Lookup( iKey, pvKey ))!=NULL )
    {
        value = *pRetVal ;
        return TRUE ;
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
VALUE_TYPE* TMULTIKEYDICTIONARYBASE::_Lookup( UCHAR iKey, PVOID pvKey ) const
{
    VERIFY_VALID_KEY( iKey, FALSE ) ;

    ULONG       bucket = 0 ;
    HASH_ENTRY* pEntry ;
    VALUE_TYPE* pRetVal = NULL ;
    
    //  Get the bucket in the matrix
    if( !GetBucket( iKey, pvKey, bucket ) )
        return NULL ;

    for( pEntry = m_hashmatrix[iKey][bucket]; pEntry;
         pEntry = pEntry->pNext )
    {
        VERIFY_VALID_ENTRY( pEntry ) ;
        VALUE_TYPE* pVal ;
        if( (pVal = GetValueFromEntry( iKey, pEntry ))!=NULL )
        {
            if( Compare( iKey, pvKey, *pVal )==0 )
                return pVal ;
        }
    }
    return NULL ;
}

//-------------------------------------------------------------------------//
//  Begins enumeration of value elements that match on the indicated key.
template TMULTIKEYDICTIONARYBASE_ARGLIST
HANDLE TMULTIKEYDICTIONARYBASE::LookupFirst( UCHAR iKey, PVOID pvKey, VALUE_TYPE& value ) const
{
    VERIFY_VALID_KEY( iKey, FALSE ) ;

    ULONG       bucket = 0 ;
    HASH_ENTRY* pEntry ;
    VALUE_TYPE* pRetVal = NULL ;
    
    //  Get the bucket in the matrix
    if( !(pvKey && GetBucket( iKey, pvKey, bucket )) )
        return NULL ;

    for( pEntry = m_hashmatrix[iKey][bucket]; pEntry;
         pEntry = pEntry->pNext )
    {
        VERIFY_VALID_ENTRY( pEntry ) ;
        VALUE_TYPE* pVal ;
        if( (pVal = GetValueFromEntry( iKey, pEntry ))!=NULL )
        {
            if( Compare( iKey, pvKey, *pVal )==0 )
            {
                LOOKUP* pLookup ;
                if( (pLookup = new LOOKUP)==NULL )
                    return NULL ;
                RtlZeroMemory( pLookup, sizeof(*pLookup) ) ;
                pLookup->cbStruct   = sizeof(*pLookup) ;
                pLookup->iKey       = iKey ;
                pLookup->fDupl      = AllowDuplicates( iKey ) ;
                pLookup->bucket     = bucket ;
                pLookup->pvKey      = pvKey ;
                pLookup->pEntry     = pEntry ;
                pLookup->pNext      = pEntry->pNext ;
                value = *pVal ;
                return pLookup ;
            }
        }
    }
    return NULL ;
}

//-------------------------------------------------------------------------//
//  Continues enumeration of value elements that match on the indicated key.
template TMULTIKEYDICTIONARYBASE_ARGLIST
BOOLEAN TMULTIKEYDICTIONARYBASE::LookupNext( HANDLE hLookup, VALUE_TYPE& value ) const
{
    LOOKUP* pLookup ;
    if( !( (pLookup = (LOOKUP*)hLookup)!=NULL && 
            pLookup->cbStruct == sizeof(*pLookup) ) )
        return FALSE ;

    //  Preliminary check to see if there's no use in continuing
    if( pLookup->fDupl && pLookup->pEntry!=NULL && pLookup->pvKey )
    {        
        for( pLookup->pEntry = pLookup->pNext; pLookup->pEntry;
             pLookup->pEntry = pLookup->pEntry->pNext )
        {
            pLookup->pNext = pLookup->pEntry->pNext ;

            VERIFY_VALID_ENTRY( pLookup->pEntry ) ;
            VALUE_TYPE* pVal ;
            if( (pVal = GetValueFromEntry( pLookup->iKey, pLookup->pEntry ))!=NULL )
            {
                if( Compare( pLookup->iKey, pLookup->pvKey, *pVal )==0 )
                {
                    value = *pVal ;
                    return TRUE ;
                }
            }
        }
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
//  Terminates enumeration of value elements that match on the indicated key
//  and frees associated resources.
template TMULTIKEYDICTIONARYBASE_ARGLIST
void TMULTIKEYDICTIONARYBASE::EndLookup( HANDLE hLookup ) const
{
    LOOKUP* pLookup ;
    if( !( (pLookup = (LOOKUP*)hLookup)!=NULL && 
            pLookup->cbStruct == sizeof(*pLookup) ) )
        return ;

    RtlZeroMemory( pLookup, sizeof(*pLookup) ) ;
    delete pLookup ;
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
BOOLEAN TMULTIKEYDICTIONARYBASE::Insert( const VALUE_TYPE& val )

{
    ULONG       buckets[cKeys] ;
    ITEM*       pItem = NULL ;

    if( !CanInsertItem( val, buckets ) )
        return FALSE ;

    //  Look for first item 
    // it doesn't exist, so create a new item
    if( (pItem = AllocItem())==NULL )
        return FALSE ;

    pItem->value = val ;

    // Insert into hash tables
    for( UCHAR iKey=0; iKey<cKeys; iKey++ )
    {

#ifdef TRACE_DICTIONARY    //  If a collision, trace
        if( m_hashmatrix[iKey][buckets[iKey]] ) {
            TRACE( TEXT("TMultiKeyDictionaryBase[key %d][bucket %d] collision.\n"), 
                    (int)iKey, (int)buckets[iKey] ) ;
        }
#endif TRACE_DICTIONARY
        //  Insert at head of collision chain
        pItem->entries[iKey].pNext = m_hashmatrix[iKey][buckets[iKey]] ;
        m_hashmatrix[iKey][buckets[iKey]] = &pItem->entries[iKey] ;
    }
    return TRUE ;
}

//-------------------------------------------------------------------------//
// Returns TRUE if removed
template TMULTIKEYDICTIONARYBASE_ARGLIST
inline BOOLEAN TMULTIKEYDICTIONARYBASE::Delete( UCHAR iKey, PVOID pvKey )
{
    VALUE_TYPE* pVal ;

    while( (pVal = _Lookup( iKey, pvKey ))!=NULL )
    {
        if( !Delete(*pVal) )
            return FALSE ;
    }
    return TRUE ;
}

//-------------------------------------------------------------------------//
// Returns TRUE if removed
template TMULTIKEYDICTIONARYBASE_ARGLIST
BOOLEAN TMULTIKEYDICTIONARYBASE::Delete( const VALUE_TYPE& val )
{
    //  Our task here is to remove the key entry in each hash table in the
    //  matrix that points to the specified item.  This means finding the 
    //  entry corresponding to each key, walking collision chains, if necessary.
    //  While we're doing this, we might as well do a sanity check on our data; 
    //  viz., make sure that all entries point to one and only one item.
    
    UCHAR       iKey ;
    ITEM*       pItem = NULL ;

    VOID        *pvKeys[cKeys] ;
    ULONG       buckets[cKeys] ;
    HASH_ENTRY  *pKeyEntry[cKeys], 
                **ppKeyEntryPrev[cKeys] ;

    RtlZeroMemory( pvKeys, sizeof(pvKeys) ) ;
    RtlZeroMemory( pKeyEntry, sizeof(pKeyEntry) ) ;
    RtlZeroMemory( ppKeyEntryPrev, sizeof(ppKeyEntryPrev) ) ;
    
    //  Retrieve all keys, associated buckets and entries
    for( iKey=0; iKey<cKeys; iKey++ )
    {
        if( (pvKeys[iKey] = GetKey( iKey, val ))==NULL )
        {
            ASSERT( FALSE ) ;
            return FALSE ;
        }
        if( !GetBucket( iKey, pvKeys[iKey], buckets[iKey] ) )
        {
            ASSERT( FALSE ) ;
            return FALSE ;
        }

        for ( ppKeyEntryPrev[iKey] = &m_hashmatrix[iKey][buckets[iKey]], pKeyEntry[iKey] = *ppKeyEntryPrev[iKey] ; 
              pKeyEntry[iKey] != NULL ; 
              pKeyEntry[iKey] = pKeyEntry[iKey]->pNext )
        {
            VALUE_TYPE* pVal ;
            if( (pVal = GetValueFromEntry( iKey, pKeyEntry[iKey] )) && 
                Compare( iKey, pvKeys[iKey], *pVal )==0 )
            {
                //  We need to save the item the entry points to...
                if( iKey==0 )
                {
                    //  This is the first key, so let's save the 
                    //  item its entry points to
                    pItem = GetItemFromEntry( iKey, pKeyEntry[iKey] ) ;
                    ASSERT( pItem ) ;
                }
                else
                {
                    //  Sanity check: all key entries should point to the same item.
                    if( GetItemFromEntry( iKey, pKeyEntry[iKey] )!=pItem )
                    {
                        ASSERT( FALSE ) ;
                        return FALSE ;
                    }
                }
                
                break ; // found the entry and item for this key. next key?
            }
            ppKeyEntryPrev[iKey] = &pKeyEntry[iKey]->pNext ;
        }
    }

    // Pinch each entry out of its collision chains.
    for( iKey=0; iKey<cKeys; iKey++ )
    {
        //  Validate that we've found entries to delete!
        if( !(ppKeyEntryPrev[iKey] && pKeyEntry[iKey]) )
            return FALSE ;
        
        //  Pinch out the entry...
        *ppKeyEntryPrev[iKey] = pKeyEntry[iKey]->pNext ;
    }

    FreeItem( pItem ) ;

    return TRUE ;  // not found
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
void TMULTIKEYDICTIONARYBASE::AcquireMutex()
{
#ifdef WIN32_USERMODE
    EnterCriticalSection( &m_critsect ) ;   
#endif

#ifdef NT_KERNELMODE 
    if( m_fPassive )
        ExAcquireFastMutex( &m_fastmutex ) ;
    else
        KeAcquireSpinLock( &m_spinLock, &m_spinLockIrql ) ;
#endif
}

//-------------------------------------------------------------------------//
template TMULTIKEYDICTIONARYBASE_ARGLIST
void TMULTIKEYDICTIONARYBASE::ReleaseMutex()
{
#ifdef WIN32_USERMODE
    LeaveCriticalSection( &m_critsect ) ;   
#endif

#ifdef NT_KERNELMODE 
    if( m_fPassive )
        ExReleaseFastMutex( &m_fastmutex ) ;
    else
        KeReleaseSpinLock( &m_spinLock, m_spinLockIrql ) ;
#endif
}

//-------------------------------------------------------------------------//
//  Hash value generators' implementation
//-------------------------------------------------------------------------//

//-------------------------------------------------------------------------//
//  Integer hash
inline ULONG HashUlong( ULONG val )
{
    return val ;
}

//-------------------------------------------------------------------------//
inline ULONG HashPointer( void* p )
{
    return PtrToUlong(p) ;
}

//-------------------------------------------------------------------------//
#ifdef NT_KERNELMODE
//  System Address hash
inline ULONG HashSystemAddr( PVOID pv )
{
    return PtrToUlong(pv) & 0xFFFFFF ;
}
#endif

#pragma warning( disable : 4244 )   // conversion from 'typeA' to 'typeA', possible loss of data
//-------------------------------------------------------------------------//
//  Case-sensitive string hash
inline ULONG HashStringA( char* psz )
{
    ULONG cch ;
    UCHAR hash[4] ;

    if( (cch = lstrlenA( psz ))<=0 )  return (ULONG)-1 ;

    *((ULONG*)hash) = 0L ;
    for( UCHAR i=sizeof(hash); cch>0; i--, cch-- )  {
        if( i<=0 ) i=sizeof(hash) ;
        hash[i-1] += (UCHAR)psz[cch-1] ;
    }
    return *(ULONG*)hash ;
}

//-------------------------------------------------------------------------//
//  Case-insensitive string hash
inline ULONG HashStringiA( char* psz )
{
    ULONG   cch ;
    UCHAR   hash[4] ;
    CHAR    ch ;

    if( (cch = lstrlenA( psz ))<=0 )
        return (ULONG)-1 ;

    *((ULONG*)hash) = 0L ;
    for( UCHAR i=sizeof(hash); cch>0; i--, cch-- )  {
        if( i<=0 ) 
            i=sizeof(hash) ;
        if( (ch=psz[cch-1]) >='a' && ch <='z' )  
            ch -= 'a'-'A' ; // make uppercase
        hash[i-1] += (UCHAR)ch ;
    }
    return *(ULONG*)hash ;
}

//-------------------------------------------------------------------------//
//  Case-sensitive Unicode string hash
inline ULONG HashStringW( WCHAR* psz )
{
    ULONG cch = 0 ;
    UCHAR hash[4] ;

#ifdef WIN32_USERMODE
    cch = lstrlenW( psz ) ;
#endif
#ifdef NT_KERNELMODE
    cch = ntkstrlen( psz ) ;
#endif 
    
    if( cch <=0 )        
        return (ULONG)-1 ;

    *((ULONG*)hash) = 0L ;
    for( UCHAR i=sizeof(hash); cch>0; i--, cch-- )  {
        if( i<=0 ) i=sizeof(hash) ;
        hash[i-1] += psz[cch-1] ;
    }
    return *(ULONG*)hash ;
}

//-------------------------------------------------------------------------//
//  Case-insensitive Unicode Hstring hash
inline ULONG HashStringiW( WCHAR* psz )
{
    ULONG   cch ;
    UCHAR   hash[4] ;
    WCHAR   ch ;

#ifdef WIN32_USERMODE
    cch = lstrlenW( psz ) ;
#endif
#ifdef NT_KERNELMODE
    cch = ntkstrlen( psz ) ;
#endif 
    
    if( cch <=0 )        
        return (ULONG)-1 ;

    *((ULONG*)hash) = 0L ;
    for( UCHAR i=sizeof(hash); cch>0; i--, cch-- )  {
        if( i<=0 ) 
            i=sizeof(hash) ;
        if( (ch=psz[cch-1]) >=L'a' && ch <=L'z' )    
            ch -= L'a'-L'A' ;   // make uppercase
        hash[i-1] += (UCHAR)ch ;
    }
    return *(ULONG*)hash ;
}

//-------------------------------//
//  Hashes a byte array.
inline ULONG HashBytes( const void* pBuf, int cb )
{
    BYTE    *pPos = (BYTE*)pBuf ;
    ULONG   nHash = 0 ;
    while( cb-- )
        nHash += *pPos++ ;
    return  nHash ;
}
#pragma warning( default : 4244 )   // conversion from 'typeA' to 'typeA', possible loss of data


#ifdef NT_KERNELMODE
#   undef NT_KERNELMODE
#endif
#ifdef WIN32_USERMODE
#   undef WIN32_USERMODE
#endif

#endif __DICTBASE_H__
