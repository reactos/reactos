//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       strhash.cxx
//
//  Contents:   A hash table for strings
//
//  History:    7-Nov-94    BruceFo Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "strhash.hxx"

const DWORD g_cMinElemsRehash = 3;
const DWORD g_cMinBuckets     = 13;

//////////////////////////////////////////////////////////////////////////////
// Methods for the CStrHashBucketElem class
//////////////////////////////////////////////////////////////////////////////

CStrHashBucketElem::CStrHashBucketElem(
    IN const WCHAR* pszKey
    )
    :
    m_pszKey(pszKey),
    m_next(NULL)
{
    INIT_SIG(CStrHashBucketElem);
}

CStrHashBucketElem::~CStrHashBucketElem()
{
    CHECK_SIG(CStrHashBucketElem);
}

BOOL
CStrHashBucketElem::IsEqual(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashBucketElem);
    appAssert(NULL != pszKey);
    appAssert(NULL != m_pszKey);

    // NOTE: case-insensitive compare. This affects the hash function!
    return (0 == _wcsicmp(pszKey, m_pszKey));
}

//////////////////////////////////////////////////////////////////////////////
// Methods for the CStrHashBucket class
//////////////////////////////////////////////////////////////////////////////

CStrHashBucket::CStrHashBucket(
    VOID
    )
    :
    m_head(NULL)
{
    INIT_SIG(CStrHashBucket);
}

CStrHashBucket::~CStrHashBucket(
    VOID
    )
{
    CHECK_SIG(CStrHashBucket);

    for (CStrHashBucketElem* x = m_head; NULL != x; )
    {
        CStrHashBucketElem* tmp = x->m_next;
        delete x;
        x = tmp;
    }
}

HRESULT
CStrHashBucket::Insert(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashBucket);
    appAssert(NULL != pszKey);

    CStrHashBucketElem* x = new CStrHashBucketElem(pszKey);
    if (NULL == x)
    {
        return E_OUTOFMEMORY;
    }

    x->m_next = m_head;
    m_head = x;
    return S_OK;
}

// return TRUE if it was found and removed, FALSE if it wasn't even found
BOOL
CStrHashBucket::Remove(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashBucket);
    appAssert(NULL != pszKey);

    CStrHashBucketElem** pPrev = &m_head;

    for (CStrHashBucketElem* x = m_head; NULL != x; x = x->m_next)
    {
        if (x->IsEqual(pszKey)) // found it
        {
            *pPrev = x->m_next;
            delete x;
            return TRUE;
        }

        pPrev = &x->m_next;
    }

    return FALSE; // didn't find it
}

BOOL
CStrHashBucket::IsMember(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashBucket);
    appAssert(NULL != pszKey);

    for (CStrHashBucketElem* x = m_head; NULL != x; x = x->m_next)
    {
        if (x->IsEqual(pszKey))
        {
            return TRUE; // found it
        }
    }
    return FALSE; // didn't find it
}

//////////////////////////////////////////////////////////////////////////////
// Methods for the CStrHashTable class
//////////////////////////////////////////////////////////////////////////////

CStrHashTable::CStrHashTable(
    IN DWORD cNumBuckets
    )
    :
    m_cElems(0),
    m_cMinNumBuckets(cNumBuckets)
{
    INIT_SIG(CStrHashTable);

    m_cNumBuckets = max(cNumBuckets, g_cMinBuckets);
    m_ht = new CStrHashBucket[m_cNumBuckets];
    if (NULL == m_ht)
    {
        appDebugOut((DEB_ERROR,
            "Failed to allocate hash table with %d buckets\n",
            m_cNumBuckets));

        m_cMinNumBuckets = 0;
        m_cNumBuckets = 0;
    }
}

HRESULT
CStrHashTable::QueryError(
    VOID
    )
{
    if (NULL == m_ht)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

CStrHashTable::~CStrHashTable(
    VOID
    )
{
    CHECK_SIG(CStrHashTable);

    delete[] m_ht;
}

HRESULT
CStrHashTable::Insert(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashTable);

    if (NULL == pszKey)
    {
        return E_INVALIDARG;
    }

    appAssert(NULL != m_ht);

    DWORD key = HashFunction(pszKey);
    if (!(m_ht[key].IsMember(pszKey)))
    {
        // only insert if the key isn't already in the table.
        HRESULT hr = m_ht[key].Insert(pszKey);
        CHECK_HRESULT(hr);
        if (FAILED(hr))
        {
            return hr;
        }
        ++m_cElems;
        return CheckRehash();
    }

    return S_OK;
}

HRESULT
CStrHashTable::Remove(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashTable);

    if (NULL == pszKey)
    {
        return E_INVALIDARG;
    }

    appAssert(NULL != m_ht);

    if (m_ht[HashFunction(pszKey)].Remove(pszKey))
    {
        --m_cElems;
        return CheckRehash();
    }

    return S_OK;    // key was not found and hence not deleted
}

BOOL
CStrHashTable::IsMember(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashTable);

    if (NULL == pszKey)
    {
        return FALSE;   // invalid argument, really
    }

    appAssert(NULL != m_ht);

    return m_ht[HashFunction(pszKey)].IsMember(pszKey);
}

DWORD
CStrHashTable::Count(
    VOID
    )
{
    CHECK_SIG(CStrHashTable);

    return m_cElems;
}

// This returns the iteration data, or NULL on failure
CIterateData*
CStrHashTable::IterateStart(
    VOID
    )
{
    CHECK_SIG(CStrHashTable);

    CIterateData* pData = new CIterateData;
    if (NULL != pData)
    {
        IterateGetNext(pData);
    }
    return pData;
}

const WCHAR*
CStrHashTable::IterateGetData(
    IN OUT CIterateData* pCurrent
    )
{
    CHECK_SIG(CStrHashTable);
    appAssert(NULL != pCurrent);
    appAssert(ITERATE_END != pCurrent->m_CurrentBucket);

    return pCurrent->m_pCurrentElem->m_pszKey;
}

BOOL
CStrHashTable::IterateDone(
    IN CIterateData* pCurrent
    )
{
    CHECK_SIG(CStrHashTable);
    appAssert(NULL != pCurrent);

    return (ITERATE_END == pCurrent->m_CurrentBucket);
}

VOID
CStrHashTable::IterateEnd(
    IN CIterateData* pCurrent
    )
{
    CHECK_SIG(CStrHashTable);

    delete pCurrent;
}

VOID
CStrHashTable::IterateGetNext(
    IN OUT CIterateData* pCurrent
    )
{
    CHECK_SIG(CStrHashTable);
    appAssert(NULL != pCurrent);
    appAssert(ITERATE_END != pCurrent->m_CurrentBucket);

    if (NULL != pCurrent->m_pCurrentElem)
    {
        if (NULL != pCurrent->m_pCurrentElem->m_next)
        {
            // just get next element in bucket
            pCurrent->m_pCurrentElem = pCurrent->m_pCurrentElem->m_next;
        }
        else
        {
            // need to search to new bucket
            ++pCurrent->m_CurrentBucket;
            IterateGetBucket(pCurrent);
        }
    }
    else
    {
        IterateGetBucket(pCurrent);
    }
}

VOID
CStrHashTable::IterateGetBucket(
    IN OUT CIterateData* pCurrent
    )
{
    CHECK_SIG(CStrHashTable);
    appAssert(NULL != m_ht);

    for (DWORD i = pCurrent->m_CurrentBucket; i < m_cNumBuckets; i++)
    {
        if (NULL != m_ht[i].m_head)
        {
            pCurrent->m_pCurrentElem  = m_ht[i].m_head;
            pCurrent->m_CurrentBucket = i;
            return;
        }
    }
    pCurrent->m_CurrentBucket = ITERATE_END; // we've iterated through all!
}

DWORD
CStrHashTable::HashFunction(
    IN const WCHAR* pszKey
    )
{
    CHECK_SIG(CStrHashTable);

    appAssert(NULL != pszKey);
    appAssert(m_cNumBuckets > 0);

    DWORD total = 0;
    for (const WCHAR* p = pszKey; L'\0' != *p; p++)
    {
        // lower case it, so case-insensitive IsEqual works
        total += towlower(*p);
    }
    return total % m_cNumBuckets;
}

HRESULT
CStrHashTable::CheckRehash(
    VOID
    )
{
    CHECK_SIG(CStrHashTable);

    if (m_cElems > g_cMinElemsRehash && m_cElems > m_cMinNumBuckets)
    {
        if (   (m_cElems > m_cNumBuckets)
            || (m_cElems < m_cNumBuckets / 4) )
        {
            // add one to at least make it odd (for better hashing behavior)
            return Rehash(m_cElems * 2 + 1);
        }
    }
    return S_OK;
}

HRESULT
CStrHashTable::Rehash(
    IN DWORD cNumBuckets
    )
{
    CHECK_SIG(CStrHashTable);

    cNumBuckets = max(cNumBuckets, g_cMinBuckets);
    CStrHashBucket* ht = new CStrHashBucket[cNumBuckets];
    if (NULL == ht)
    {
        // return error, but don't delete the existing table
        return E_OUTOFMEMORY;
    }

    appAssert(NULL != m_ht);

    // now transfer all data from old to new

    ULONG cOldNumBuckets = m_cNumBuckets;
    m_cNumBuckets = cNumBuckets;    // set this so HashFunction() uses it

    for (ULONG i=0; i < cOldNumBuckets; i++)
    {
        for (CStrHashBucketElem* x = m_ht[i].m_head; NULL != x; )
        {
            CStrHashBucketElem* tmp = x->m_next;

            // now, just put this bucket in the right place in the new
            // hash table. Avoid performing new's and copying the keys.

            DWORD bucket = HashFunction(x->m_pszKey);
            x->m_next = ht[bucket].m_head;
            ht[bucket].m_head = x;

            x = tmp;
        }

        m_ht[i].m_head = NULL; // there isn't anything left in the list!
    }

    // the data is transfered; complete the switchover

    delete[] m_ht;
    m_ht = ht;

    return S_OK;
}
