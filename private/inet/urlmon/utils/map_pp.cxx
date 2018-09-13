
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
//
// Implementation of parmeterized Map
//
/////////////////////////////////////////////////////////////////////////////


#include <urlint.h>
#include <map_kv.h>
#include "coll.hxx"
#define ASSERT(x)

/*
#include "stdafx.h"

#ifdef AFX_COLL2_SEG
#pragma code_seg(AFX_COLL2_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define new DEBUG_NEW
*/

/////////////////////////////////////////////////////////////////////////////

CMapPtrToPtr::CMapPtrToPtr(int nBlockSize)
{
        ASSERT(nBlockSize > 0);

        m_pHashTable = NULL;
        m_nHashTableSize = 17;  // default size
        m_nCount = 0;
        m_pFreeList = NULL;
        m_pBlocks = NULL;
        m_nBlockSize = nBlockSize;
}

inline UINT CMapPtrToPtr::HashKey(void* key) const
{
        // default identity hash - works for most primitive values
        return PtrToUlong((void*)(DWORD_PTR)key) >> 4;
}

void CMapPtrToPtr::InitHashTable(
        UINT nHashSize, BOOL bAllocNow)
//
// Used to force allocation of a hash table or to override the default
//   hash table size of (which is fairly small)
{
        ASSERT_VALID(this);
        ASSERT(m_nCount == 0);
        ASSERT(nHashSize > 0);

        if (m_pHashTable != NULL)
        {
                // free hash table
                delete[] m_pHashTable;
                m_pHashTable = NULL;
        }

        if (bAllocNow)
        {
                m_pHashTable = new CAssoc* [nHashSize];
                memset(m_pHashTable, 0, sizeof(CAssoc*) * nHashSize);
        }
        m_nHashTableSize = nHashSize;
}

void CMapPtrToPtr::RemoveAll()
{
        ASSERT_VALID(this);



        if (m_pHashTable != NULL)
        {
                // free hash table
                delete[] m_pHashTable;
                m_pHashTable = NULL;
        }

        m_nCount = 0;
        m_pFreeList = NULL;
        m_pBlocks->FreeDataChain();
        m_pBlocks = NULL;
}

CMapPtrToPtr::~CMapPtrToPtr()
{
        RemoveAll();
        ASSERT(m_nCount == 0);
}

/////////////////////////////////////////////////////////////////////////////
// Assoc helpers
// same as CList implementation except we store CAssoc's not CNode's
//    and CAssoc's are singly linked all the time

CMapPtrToPtr::CAssoc*
CMapPtrToPtr::NewAssoc()
{
        if (m_pFreeList == NULL)
        {
                // add another block
                CPlex* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, sizeof(CMapPtrToPtr::CAssoc));
                // chain them into free list
                CMapPtrToPtr::CAssoc* pAssoc = (CMapPtrToPtr::CAssoc*) newBlock->data();
                // free in reverse order to make it easier to debug
                pAssoc += m_nBlockSize - 1;
                for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
                {
                        pAssoc->pNext = m_pFreeList;
                        m_pFreeList = pAssoc;
                }
        }
        ASSERT(m_pFreeList != NULL);  // we must have something

        CMapPtrToPtr::CAssoc* pAssoc = m_pFreeList;
        m_pFreeList = m_pFreeList->pNext;
        m_nCount++;
        ASSERT(m_nCount > 0);  // make sure we don't overflow


        pAssoc->key = 0;




        pAssoc->value = 0;

        return pAssoc;
}

void CMapPtrToPtr::FreeAssoc(CMapPtrToPtr::CAssoc* pAssoc)
{

        pAssoc->pNext = m_pFreeList;
        m_pFreeList = pAssoc;
        m_nCount--;
        ASSERT(m_nCount >= 0);  // make sure we don't underflow

        // if no more elements, cleanup completely
        if (m_nCount == 0)
                RemoveAll();
}

CMapPtrToPtr::CAssoc*
CMapPtrToPtr::GetAssocAt(void* key, UINT& nHash) const
// find association (or return NULL)
{
        nHash = HashKey(key) % m_nHashTableSize;

        if (m_pHashTable == NULL)
                return NULL;

        // see if it exists
        CAssoc* pAssoc;
        for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
                if (pAssoc->key == key)
                        return pAssoc;
        }
        return NULL;
}


void* CMapPtrToPtr::GetValueAt(void* key) const
// find value (or return NULL -- NULL values not different as a result)
{
        if (m_pHashTable == NULL)
                return NULL;

        UINT nHash = HashKey(key) % m_nHashTableSize;

        // see if it exists
        CAssoc* pAssoc;
        for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
                if (pAssoc->key == key)
                        return pAssoc->value;
        }
        return NULL;
}


/////////////////////////////////////////////////////////////////////////////

BOOL CMapPtrToPtr::Lookup(void* key, void*& rValue) const
{
        ASSERT_VALID(this);

        UINT nHash;
        CAssoc* pAssoc = GetAssocAt(key, nHash);
        if (pAssoc == NULL)
                return FALSE;  // not in map

        rValue = pAssoc->value;
        return TRUE;
}

void*& CMapPtrToPtr::operator[](void* key)
{
        ASSERT_VALID(this);

        UINT nHash;
        CAssoc* pAssoc;
        if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
        {
                if (m_pHashTable == NULL)
                        InitHashTable(m_nHashTableSize);

                // it doesn't exist, add a new Association
                pAssoc = NewAssoc();

                pAssoc->key = key;
                // 'pAssoc->value' is a constructed object, nothing more

                // put into hash table
                pAssoc->pNext = m_pHashTable[nHash];
                m_pHashTable[nHash] = pAssoc;
        }
        return pAssoc->value;  // return new reference
}


BOOL CMapPtrToPtr::RemoveKey(void* key)
// remove key - return TRUE if removed
{
        ASSERT_VALID(this);

        if (m_pHashTable == NULL)
                return FALSE;  // nothing in the table

        CAssoc** ppAssocPrev;
        ppAssocPrev = &m_pHashTable[HashKey(key) % m_nHashTableSize];

        CAssoc* pAssoc;
        for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
                if (pAssoc->key == key)
                {
                        // remove it
                        *ppAssocPrev = pAssoc->pNext;  // remove from list
                        FreeAssoc(pAssoc);
                        return TRUE;
                }
                ppAssocPrev = &pAssoc->pNext;
        }
        return FALSE;  // not found
}


/////////////////////////////////////////////////////////////////////////////
// Iterating

void CMapPtrToPtr::GetNextAssoc(POSITION& rNextPosition,
        void*& rKey, void*& rValue) const
{
        ASSERT_VALID(this);
        ASSERT(m_pHashTable != NULL);  // never call on empty map

        CAssoc* pAssocRet = (CAssoc*)rNextPosition;
        ASSERT(pAssocRet != NULL);

        if (pAssocRet == (CAssoc*) BEFORE_START_POSITION)
        {
                // find the first association
                for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
                        if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                                break;
                ASSERT(pAssocRet != NULL);  // must find something
        }

        // find next association
        ASSERT(AfxIsValidAddress(pAssocRet, sizeof(CAssoc)));
        CAssoc* pAssocNext;
        if ((pAssocNext = pAssocRet->pNext) == NULL)
        {

                // go to next bucket
                for (UINT nBucket = (HashKey(pAssocRet->key) % m_nHashTableSize) + 1;
                  nBucket < m_nHashTableSize; nBucket++)
                        if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                                break;

        }

        rNextPosition = (POSITION) pAssocNext;

        // fill in return data
        rKey = pAssocRet->key;
        rValue = pAssocRet->value;
}


/////////////////////////////////////////////////////////////////////////////
// Diagnostics

#ifdef _DEBUG
void CMapPtrToPtr::Dump(CDumpContext& dc) const
{
        CObject::Dump(dc);

        dc << "with " << m_nCount << " elements";
        if (dc.GetDepth() > 0)
        {
                // Dump in format "[key] -> value"
                void* key;
                void* val;

                POSITION pos = GetStartPosition();
                while (pos != NULL)
                {
                        GetNextAssoc(pos, key, val);
                        dc << "\n\t[" << key << "] = " << val;
                }
        }

        dc << "\n";
}

void CMapPtrToPtr::AssertValid() const
{
        CObject::AssertValid();

        ASSERT(m_nHashTableSize > 0);
        ASSERT(m_nCount == 0 || m_pHashTable != NULL);
                // non-empty map should have hash table
}
#endif //_DEBUG

#ifdef AFX_INIT_SEG
#pragma code_seg(AFX_INIT_SEG)
#endif


//IMPLEMENT_DYNAMIC(CMapPtrToPtr, CObject)

/////////////////////////////////////////////////////////////////////////////
