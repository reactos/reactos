//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       map_kv.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-07-1996   JohannP (Johann Posch)  taken from ole
//
//----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// class CMapKeyToValue - a mapping from 'KEY's to 'VALUE's, passed in as
// pv/cb pairs.  The keys can be variable length, although we optmizize the
// case when they are all the same.
//
/////////////////////////////////////////////////////////////////////////////

#include <urlint.h>
#include <clist.hxx>

#include <map_kv.h>
#include <map_dwp.h>

#ifdef unix
#ifdef MW_MSCOMPATIBLE_CMAPKEY
#define CMAPKEY(x,y) x.n.y
#else
#define CMAPKEY(x,y) x.y
#endif /* MW_MSCOMPATIBLE_CMAPKEY */
#endif /* unix */

//CMapKeyToValue::CMapKeyToValue(DWORD memctx, UINT cbValue, UINT cbKey,
CMapKeyToValue::CMapKeyToValue(UINT cbValue, UINT cbKey,
   int nBlockSize, LPFNHASHKEY lpfnHashKey, UINT nHashSize)
{
   Assert(nBlockSize > 0);

   m_cbValue = cbValue;
   m_cbKey = cbKey;
   m_cbKeyInAssoc = cbKey == 0 ? sizeof(CKeyWrap) : cbKey;

   m_pHashTable = NULL;
   m_nHashTableSize = nHashSize;
   m_lpfnHashKey = lpfnHashKey;

   m_nCount = 0;
   m_pFreeList = NULL;
   m_pBlocks = NULL;
   m_nBlockSize = nBlockSize;
   //if (memctx == MEMCTX_SAME)
   //   memctx = CoMemctxOf(this);
   //m_memctx = memctx;
   //Assert(m_memctx != MEMCTX_UNKNOWN);
}

CMapKeyToValue::~CMapKeyToValue()
{
   ASSERT_VALID(this);
   RemoveAll();
   Assert(m_nCount == 0);
}


// simple, default hash function
// REVIEW: need to check the value in this for GUIDs and strings
STDAPI_(UINT) MKVDefaultHashKey(LPVOID pKey, UINT cbKey)
{
   UINT hash = 0;
   BYTE FAR* lpb = (BYTE FAR*)pKey;

   while (cbKey-- != 0)
        hash = 257 * hash + *lpb++;

   return hash;
}


BOOL CMapKeyToValue::InitHashTable()
{
   ASSERT_VALID(this);
   Assert(m_nHashTableSize  > 0);

   if (m_pHashTable != NULL)
        return TRUE;

   Assert(m_nCount == 0);

   if ((m_pHashTable = (CAssoc FAR* FAR*)CoTaskMemAlloc(m_nHashTableSize * sizeof(CAssoc FAR*))) == NULL)
        return FALSE;

   memset(m_pHashTable, 0, sizeof(CAssoc FAR*) * m_nHashTableSize);

   ASSERT_VALID(this);

   return TRUE;
}


void CMapKeyToValue::RemoveAll()
{
   ASSERT_VALID(this);

   // free all key values and then hash table
   if (m_pHashTable != NULL)
   {
        // destroy assocs
        for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
        {
                register CAssoc FAR* pAssoc;
                for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
                  pAssoc = pAssoc->pNext)
                        // assoc itself is freed by FreeDataChain below
                        FreeAssocKey(pAssoc);
        }

        // free hash table
        CoTaskMemFree(m_pHashTable);
        m_pHashTable = NULL;
   }

   m_nCount = 0;
   m_pFreeList = NULL;
   m_pBlocks->FreeDataChain();
   m_pBlocks = NULL;

   ASSERT_VALID(this);
}

/////////////////////////////////////////////////////////////////////////////
// Assoc helpers
// CAssoc's are singly linked all the time

CMapKeyToValue::CAssoc  FAR*
    CMapKeyToValue::NewAssoc(UINT hash, LPVOID pKey, UINT cbKey, LPVOID pValue)
{
   if (m_pFreeList == NULL)
   {
        // add another block
        CPlex FAR* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, SizeAssoc());

        if (newBlock == NULL)
                return NULL;

        // chain them into free list
        register BYTE  FAR* pbAssoc = (BYTE FAR*) newBlock->data();
        // free in reverse order to make it easier to debug
        pbAssoc += (m_nBlockSize - 1) * SizeAssoc();
        for (int i = m_nBlockSize-1; i >= 0; i--, pbAssoc -= SizeAssoc())
        {
                ((CAssoc FAR*)pbAssoc)->pNext = m_pFreeList;
                m_pFreeList = (CAssoc FAR*)pbAssoc;
        }
   }
   Assert(m_pFreeList != NULL); // we must have something

   CMapKeyToValue::CAssoc  FAR* pAssoc = m_pFreeList;

   // init all fields except pNext while still on free list
   pAssoc->nHashValue = hash;
   if (!SetAssocKey(pAssoc, pKey, cbKey))
        return NULL;

   SetAssocValue(pAssoc, pValue);

   // remove from free list after successfully initializing it (except pNext)
   m_pFreeList = m_pFreeList->pNext;
   m_nCount++;
   Assert(m_nCount > 0);       // make sure we don't overflow

   return pAssoc;
}


// free individual assoc by freeing key and putting on free list
void CMapKeyToValue::FreeAssoc(CMapKeyToValue::CAssoc  FAR* pAssoc)
{
   pAssoc->pNext = m_pFreeList;
   m_pFreeList = pAssoc;
   m_nCount--;
   Assert(m_nCount >= 0);      // make sure we don't underflow

   FreeAssocKey(pAssoc);
}


// find association (or return NULL)
CMapKeyToValue::CAssoc  FAR*
CMapKeyToValue::GetAssocAt(LPVOID pKey, UINT cbKey, UINT FAR& nHash) const
{
   if (m_lpfnHashKey)
       nHash = (*m_lpfnHashKey)(pKey, cbKey) % m_nHashTableSize;
   else
       nHash = MKVDefaultHashKey(pKey, cbKey) % m_nHashTableSize;

   if (m_pHashTable == NULL)
        return NULL;

   // see if it exists
   register CAssoc  FAR* pAssoc;
   for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
   {
        if (CompareAssocKey(pAssoc, pKey, cbKey))
                return pAssoc;
   }
   return NULL;
}


BOOL CMapKeyToValue::CompareAssocKey(CAssoc FAR* pAssoc, LPVOID pKey2, UINT cbKey2) const
{
   LPVOID pKey1;
   UINT cbKey1;

   GetAssocKeyPtr(pAssoc, &pKey1, &cbKey1);
   return cbKey1 == cbKey2 && memcmp(pKey1, pKey2, cbKey1) == 0;
}


BOOL CMapKeyToValue::SetAssocKey(CAssoc FAR* pAssoc, LPVOID pKey, UINT cbKey) const
{
   Assert(cbKey == m_cbKey || m_cbKey == 0);

   if (m_cbKey == 0)
   {
        Assert(m_cbKeyInAssoc == sizeof(CKeyWrap));

        // alloc, set size and pointer
#ifndef unix
        if ((pAssoc->key.pKey = CoTaskMemAlloc(cbKey)) == NULL)
#else
        if ((pAssoc->CMAPKEY(key,pKey) = CoTaskMemAlloc(cbKey)) == NULL)
#endif /* unix */
                return FALSE;
#ifndef unix
        pAssoc->key.cbKey = cbKey;
#else
        pAssoc->CMAPKEY(key,cbKey) = cbKey;
#endif /* unix */
   }

   LPVOID pKeyTo;

   GetAssocKeyPtr(pAssoc, &pKeyTo, &cbKey);

   memcpy(pKeyTo, pKey, cbKey);

   return TRUE;
}


// gets pointer to key and its length
void CMapKeyToValue::GetAssocKeyPtr(CAssoc FAR* pAssoc, LPVOID FAR* ppKey,UINT FAR* pcbKey) const
{
   if (m_cbKey == 0)
   {
        // variable length key; go indirect
#ifndef unix
        *ppKey = pAssoc->key.pKey;
        *pcbKey = pAssoc->key.cbKey;
#else
        *ppKey = pAssoc->CMAPKEY(key,pKey);
        *pcbKey = pAssoc->CMAPKEY(key,cbKey);
#endif /* unix */ 
   }
   else
   {
        // fixed length key; key in assoc
        *ppKey = (LPVOID)&pAssoc->key;
        *pcbKey = m_cbKey;
   }
}


void CMapKeyToValue::FreeAssocKey(CAssoc FAR* pAssoc) const
{
   if (m_cbKey == 0)
#ifndef unix
        CoTaskMemFree(pAssoc->key.pKey);
#else
        CoTaskMemFree(pAssoc->CMAPKEY(key,pKey));
#endif /* unix */
}


void CMapKeyToValue::GetAssocValuePtr(CAssoc FAR* pAssoc, LPVOID FAR* ppValue) const
{
   *ppValue = (char FAR*)&pAssoc->key + m_cbKeyInAssoc;
}


void CMapKeyToValue::GetAssocValue(CAssoc FAR* pAssoc, LPVOID pValue) const
{
   LPVOID pValueFrom;
   GetAssocValuePtr(pAssoc, &pValueFrom);
   Assert(pValue != NULL);
   memcpy(pValue, pValueFrom, m_cbValue);
}


void CMapKeyToValue::SetAssocValue(CAssoc FAR* pAssoc, LPVOID pValue) const
{
   LPVOID pValueTo;
   GetAssocValuePtr(pAssoc, &pValueTo);
   if (pValue == NULL)
        memset(pValueTo, 0, m_cbValue);
   else
        memcpy(pValueTo, pValue, m_cbValue);
}


/////////////////////////////////////////////////////////////////////////////

// lookup value given key; return FALSE if key not found; in that
// case, the value is set to all zeros
BOOL CMapKeyToValue::Lookup(LPVOID pKey, UINT cbKey, LPVOID pValue) const
{
   UINT nHash;
   HMAPKEY hmapkey = (HMAPKEY)GetAssocAt(pKey, cbKey, nHash);
   return LookupHKey(hmapkey, pValue);
   //return LookupHKey((HMAPKEY)GetAssocAt(pKey, cbKey, nHash), pValue);
}


// lookup value given key; return FALSE if NULL (or bad) key; in that
// case, the value is set to all zeros
BOOL CMapKeyToValue::LookupHKey(HMAPKEY hKey, LPVOID pValue) const
{
   // REVIEW: would like some way to verify that hKey is valid
   register CAssoc  FAR* pAssoc = (CAssoc FAR*)hKey;
   if (pAssoc == NULL)
   {
        memset(pValue, 0, m_cbValue);
        return FALSE;       // not in map
   }

   ASSERT_VALID(this);

   GetAssocValue(pAssoc, pValue);
   return TRUE;
}


// lookup and if not found add; returns FALSE only if OOM; if added,
// value added and pointer passed are set to zeros.
BOOL CMapKeyToValue::LookupAdd(LPVOID pKey, UINT cbKey, LPVOID pValue) const
{
   if (Lookup(pKey, cbKey, pValue))
        return TRUE;

   // value set to zeros since lookup failed

   return ((CMapKeyToValue FAR*)this)->SetAt(pKey, cbKey, NULL);
}


// the only place new assocs are created; return FALSE if OOM;
// never returns FALSE if keys already exists
BOOL CMapKeyToValue::SetAt(LPVOID pKey, UINT cbKey, LPVOID pValue)
{
   UINT nHash;
   register CAssoc  FAR* pAssoc;

   ASSERT_VALID(this);

   if ((pAssoc = GetAssocAt(pKey, cbKey, nHash)) == NULL)
   {
        if (!InitHashTable())
                // out of memory
                return FALSE;

        // it doesn't exist, add a new Association
        if ((pAssoc = NewAssoc(nHash, pKey, cbKey, pValue)) == NULL)
                return FALSE;

        // put into hash table
        pAssoc->pNext = m_pHashTable[nHash];
        m_pHashTable[nHash] = pAssoc;

        ASSERT_VALID(this);
   }
   else
   {
        SetAssocValue(pAssoc, pValue);
   }

   return TRUE;
}


// set existing hkey to value; return FALSE if NULL or bad key
BOOL CMapKeyToValue::SetAtHKey(HMAPKEY hKey, LPVOID pValue)
{
   // REVIEW: would like some way to verify that hKey is valid
   register CAssoc  FAR* pAssoc = (CAssoc FAR*)hKey;
   if (pAssoc == NULL)
        return FALSE;       // not in map

   ASSERT_VALID(this);

   SetAssocValue(pAssoc, pValue);
   return TRUE;
}


// remove key - return TRUE if removed
BOOL CMapKeyToValue::RemoveKey(LPVOID pKey, UINT cbKey)
{
   ASSERT_VALID(this);

   if (m_pHashTable == NULL)
        return FALSE;       // nothing in the table

   register CAssoc  FAR* FAR* ppAssocPrev;
   UINT i;
   if (m_lpfnHashKey)
       i = (*m_lpfnHashKey)(pKey, cbKey) % m_nHashTableSize;
   else
       i = MKVDefaultHashKey(pKey, cbKey) % m_nHashTableSize;

   ppAssocPrev = &m_pHashTable[i];

   CAssoc  FAR* pAssoc;
   for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
   {
        if (CompareAssocKey(pAssoc, pKey, cbKey))
        {
                // remove it
                *ppAssocPrev = pAssoc->pNext;       // remove from list
                FreeAssoc(pAssoc);
                ASSERT_VALID(this);
                return TRUE;
        }
        ppAssocPrev = &pAssoc->pNext;
   }
   return FALSE;   // not found
}


// remove key based on pAssoc (HMAPKEY)
BOOL CMapKeyToValue::RemoveHKey(HMAPKEY hKey)
{
   ASSERT_VALID(this);

   if (m_pHashTable == NULL)
        return FALSE;       // nothing in the table

   // REVIEW: would like some way to verify that hKey is valid
   CAssoc  FAR* pAssoc = (CAssoc FAR*)hKey;
   if (pAssoc == NULL || pAssoc->nHashValue >= m_nHashTableSize)
        // null hkey or bad hash value
        return FALSE;

   register CAssoc  FAR* FAR* ppAssocPrev;
   ppAssocPrev = &m_pHashTable[pAssoc->nHashValue];

   while (*ppAssocPrev != NULL)
   {
        if (*ppAssocPrev == pAssoc)
        {
                // remove it
                *ppAssocPrev = pAssoc->pNext;       // remove from list
                FreeAssoc(pAssoc);
                ASSERT_VALID(this);
                return TRUE;
        }
        ppAssocPrev = &(*ppAssocPrev)->pNext;
   }

   return FALSE;   // not found (must have a screwed up list or passed
                                // a key from another list)
}


HMAPKEY CMapKeyToValue::GetHKey(LPVOID pKey, UINT cbKey) const
{
   UINT nHash;

   ASSERT_VALID(this);

   return (HMAPKEY)GetAssocAt(pKey, cbKey, nHash);
}


/////////////////////////////////////////////////////////////////////////////
// Iterating

// for fixed length keys, copies key to pKey; pcbKey can be NULL;
// for variable length keys, copies pointer to key to pKey; sets pcbKey.

void CMapKeyToValue::GetNextAssoc(POSITION FAR* pNextPosition,
        LPVOID pKey, UINT FAR* pcbKey, LPVOID pValue) const
{
   ASSERT_VALID(this);

   Assert(m_pHashTable != NULL);       // never call on empty map

   register CAssoc  FAR* pAssocRet = (CAssoc  FAR*)*pNextPosition;
   Assert(pAssocRet != NULL);

   if (pAssocRet == (CAssoc  FAR*) BEFORE_START_POSITION)
   {
        // find the first association
        for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
                if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                        break;
        Assert(pAssocRet != NULL);  // must find something
   }

   // find next association
   CAssoc  FAR* pAssocNext;
   if ((pAssocNext = pAssocRet->pNext) == NULL)
   {
        // go to next bucket
        for (UINT nBucket = pAssocRet->nHashValue + 1;
          nBucket < m_nHashTableSize; nBucket++)
                if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                        break;
   }

   // fill in return data
   *pNextPosition = (POSITION) pAssocNext;

   // fill in key/pointer to key
   LPVOID pKeyFrom;
   UINT cbKey;
   GetAssocKeyPtr(pAssocRet, &pKeyFrom, &cbKey);
   if (m_cbKey == 0)
        // variable length key; just return pointer to key itself
        *(void FAR* FAR*)pKey = pKeyFrom;
   else
        memcpy(pKey, pKeyFrom, cbKey);

   if (pcbKey != NULL)
        *pcbKey = cbKey;

   // get value
   GetAssocValue(pAssocRet, pValue);
}

/////////////////////////////////////////////////////////////////////////////

void CMapKeyToValue::AssertValid() const
{
#if DBG==1
   Assert(m_cbKeyInAssoc == (m_cbKey == 0 ? sizeof(CKeyWrap) : m_cbKey));

   Assert(m_nHashTableSize > 0);
   Assert(m_nCount == 0 || m_pHashTable != NULL);

   if (m_pHashTable != NULL)
        Assert(!IsBadReadPtr(m_pHashTable, m_nHashTableSize * sizeof(CAssoc FAR*)));

   if (m_lpfnHashKey)
       Assert(!IsBadCodePtr((FARPROC)m_lpfnHashKey));

   if (m_pFreeList != NULL)
        Assert(!IsBadReadPtr(m_pFreeList, SizeAssoc()));

   if (m_pBlocks != NULL)
        Assert(!IsBadReadPtr(m_pBlocks, SizeAssoc() * m_nBlockSize));

   // some collections live as global variables in the libraries, but
   // have their existance in some context.  Also, we can't check shared
   // collections since we might be checking the etask collection
   // which would cause an infinite recursion.
   // REVIEW: Assert(m_memctx == MEMCTX_SHARED ||
   // CoMemctxOf(this) == MEMCTX_UNKNOWN || CoMemctxOf(this) == m_memctx);
#endif // DBG==1
}

