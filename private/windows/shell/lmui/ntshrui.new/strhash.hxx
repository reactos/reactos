//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       strhash.hxx
//
//  Contents:   A hash table for strings
//
//  History:    7-Nov-94    BruceFo Created
//
//----------------------------------------------------------------------------

#ifndef __STRHASH_HXX__
#define __STRHASH_HXX__

//////////////////////////////////////////////////////////////////////////////
#define HASH_DEFAULT_NUM_BUCKETS    47

//////////////////////////////////////////////////////////////////////////////
// Forward definitions

class CStrHashBucketElem;
class CStrHashBucket;
class CStrHashTable;

//////////////////////////////////////////////////////////////////////////////
// Class definitions

#define ITERATE_END ((DWORD)-1)

class CIterateData
{
    friend class CStrHashTable;

public:
    CIterateData(
        VOID
        )
        :
        m_pCurrentElem(NULL),
        m_CurrentBucket(0)
    {
    }

private:
    CStrHashBucketElem* m_pCurrentElem;
    DWORD               m_CurrentBucket;
};

//////////////////////////////////////////////////////////////////////////////

class CStrHashBucketElem
{
    DECLARE_SIG;

    friend class CStrHashBucket;
    friend class CStrHashTable;

public:

    CStrHashBucketElem(
        IN const WCHAR* pszKey
        );

    ~CStrHashBucketElem();

    BOOL
    IsEqual(
        IN const WCHAR* pszKey
        );

#if DBG == 1
    VOID Print()
    {
        appAssert(NULL != m_pszKey);
        appDebugOut((DEB_TRACE,"%ws\n", m_pszKey));
    }
#endif DBG == 1

private:

    CStrHashBucketElem* m_next;
    const WCHAR*        m_pszKey;
};

//////////////////////////////////////////////////////////////////////////////

class CStrHashBucket
{
    DECLARE_SIG;

    friend class CStrHashTable;

public:

    CStrHashBucket(
        VOID
        );

    ~CStrHashBucket();

    HRESULT
    Insert(
        IN const WCHAR* pszKey
        );

    BOOL
    Remove(
        IN const WCHAR* pszKey
        );

    BOOL
    IsMember(
        IN const WCHAR* pszKey
        );

#if DBG == 1
    VOID Print()
    {
        for (CStrHashBucketElem* x = m_head; NULL != x; x = x->m_next)
        {
            x->Print();
        }
    }
#endif DBG == 1

private:

    CStrHashBucketElem* m_head;
};

//////////////////////////////////////////////////////////////////////////////

class CStrHashTable
{
    DECLARE_SIG;

public:

    CStrHashTable(
        IN DWORD cNumBuckets = HASH_DEFAULT_NUM_BUCKETS
        );

    // Returns the status of the object after construction: S_OK or an error
    // code.
    HRESULT
    QueryError(
        VOID
        );

    ~CStrHashTable();

    // Three basic operations: insert, remove, and check membership. The
    // strings may not be NULL.

    HRESULT
    Insert(
        IN const WCHAR* pszKey
        );

    HRESULT
    Remove(
        IN const WCHAR* pszKey
        );

    BOOL
    IsMember(
        IN const WCHAR* pszKey
        );

    DWORD
    Count(
        VOID
        );

    // Iteration routines

    CIterateData*
    IterateStart(
        VOID
        );

    VOID
    IterateGetNext(
        IN OUT CIterateData* pCurrent
        );

    VOID
    IterateEnd(
        IN CIterateData* pCurrent
        );

    BOOL
    IterateDone(
        IN CIterateData* pCurrent
        );

    const WCHAR*
    IterateGetData(
        IN OUT CIterateData* pCurrent
        );

#if DBG == 1
    VOID Print()
    {
        appAssert(NULL != m_ht);

        for (DWORD i = 0; i < m_cNumBuckets; i++)
        {
            appDebugOut((DEB_TRACE,"======== bucket %d\n",i));
            m_ht[i].Print();
        }
    }
#endif DBG == 1

private:

    VOID
    IterateGetBucket(
        IN OUT CIterateData* pCurrent
        );

    DWORD
    HashFunction(
        IN const WCHAR* pszKey
        );

    HRESULT
    CheckRehash(
        VOID
        );

    HRESULT
    Rehash(
        IN DWORD cNumBuckets
        );

    CStrHashBucket*     m_ht;
    DWORD               m_cMinNumBuckets;
    DWORD               m_cNumBuckets;
    DWORD               m_cElems;
};

#endif // __STRHASH_HXX__
