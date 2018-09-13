//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       map_kv.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-29-1996   JohannP (Johann Posch)   taken from OLE
//
//----------------------------------------------------------------------------
#ifndef __MAP_KV_H__
#define __MAP_KV_H__

typedef void FAR* POSITION;
#define BEFORE_START_POSITION ((POSITION)(ULONG)-1L)


/////////////////////////////////////////////////////////////////////////////
// class CMapKeyToValue - a mapping from 'KEY's to 'VALUE's, passed in as
// pv/cb pairs.  The keys can be variable length, although we optmizize the
// case when they are all the same.
//
/////////////////////////////////////////////////////////////////////////////

STDAPI_(UINT) MKVDefaultHashKey(LPVOID pKey, UINT cbKey);
DECLARE_HANDLE(HMAPKEY);
typedef UINT (STDAPICALLTYPE FAR* LPFNHASHKEY)(LPVOID, UINT);

class FAR CMapKeyToValue
{
public:
        CMapKeyToValue(UINT cbValue, UINT cbKey = 0,
                int nBlockSize=10,
                LPFNHASHKEY lpfnHashKey = NULL,
                UINT nHashSize = 17);
        ~CMapKeyToValue();

        // number of elements
        int     GetCount() const { return m_nCount; }
        BOOL    IsEmpty() const { return m_nCount == 0; }

        // Lookup; return FALSE if not found
        BOOL    Lookup(LPVOID pKey, UINT cbKey, LPVOID pValue) const;
        BOOL    LookupHKey(HMAPKEY hKey, LPVOID pValue) const;
        BOOL    LookupAdd(LPVOID pKey, UINT cbKey, LPVOID pValue) const;

        // add a new (key, value) pair; return FALSE if out of memory
        BOOL    SetAt(LPVOID pKey, UINT cbKey, LPVOID pValue);
        BOOL    SetAtHKey(HMAPKEY hKey, LPVOID pValue);

        // removing existing (key, ?) pair; return FALSE if no such key
        BOOL    RemoveKey(LPVOID pKey, UINT cbKey);
        BOOL    RemoveHKey(HMAPKEY hKey);
        void    RemoveAll();

        // iterating all (key, value) pairs
        POSITION GetStartPosition() const
                        { return (m_nCount == 0) ? (POSITION)NULL : BEFORE_START_POSITION; }
        void    GetNextAssoc(POSITION FAR* pNextPosition, LPVOID pKey,
                                UINT FAR* pcbKey, LPVOID pValue) const;

        // return HMAPKEY for given key; returns NULL if not currently in map
        HMAPKEY GetHKey(LPVOID pKey, UINT cbKey) const;

        void    AssertValid() const;
#ifndef unix
private:
#else
    // If this was not made public we get complier warnings
    // that CKeyWrap is not accessible from CAssoc declared below
    // which means global functions cant return CNode pointers
public:
#endif /* unix */
        // abstracts, somewhat, variable and fixed sized keys; size is really
        // m_cbKeyInAssoc.
        union CKeyWrap
        {
                BYTE rgbKey[sizeof(LPVOID) + sizeof(UINT)];
                struct
                {
                        LPVOID pKey;
                        UINT cbKey;
                }
#ifdef unix
                n 
#endif /* unix */
                ;
        };
        // Association of one key and one value; NOTE: even though in general
        // the size of the key and value varies, for any given map,
        // the size of an assoc is fixed.
        struct CAssoc
        {
                CAssoc  FAR* pNext;
                UINT    nHashValue; // needed for efficient iteration
                CKeyWrap key;           // size is really m_cbKeyInAssoc
                // BYTE rgbValue[m_cbValue];
        };
#ifdef unix
private:
#endif /* unix */
        UINT    SizeAssoc() const
                { return sizeof(CAssoc)-sizeof(CKeyWrap) + m_cbKeyInAssoc + m_cbValue; }
        CAssoc  FAR* NewAssoc(UINT hash, LPVOID pKey, UINT cbKey, LPVOID pValue);
        void    FreeAssoc(CAssoc FAR* pAssoc);
        BOOL    CompareAssocKey(CAssoc FAR* pAssoc, LPVOID pKey, UINT cbKey) const;
        CAssoc  FAR* GetAssocAt(LPVOID pKey, UINT cbKey, UINT FAR& nHash) const;

        BOOL    SetAssocKey(CAssoc FAR* pAssoc, LPVOID pKey, UINT cbKey) const;
        void    GetAssocKeyPtr(CAssoc FAR* pAssoc, LPVOID FAR* ppKey,UINT FAR* pcbKey) const;
        void    FreeAssocKey(CAssoc FAR* pAssoc) const;
        void    GetAssocValuePtr(CAssoc FAR* pAssoc, LPVOID FAR* ppValue) const;
        void    GetAssocValue(CAssoc FAR* pAssoc, LPVOID pValue) const;
        void    SetAssocValue(CAssoc FAR* pAssoc, LPVOID pValue) const;

        BOOL    InitHashTable();

        UINT    m_cbValue;
        UINT    m_cbKey;                        // variable length if 0
        UINT    m_cbKeyInAssoc;         // always non-zero

        CAssoc  FAR* FAR* m_pHashTable;
        UINT    m_nHashTableSize;
        LPFNHASHKEY m_lpfnHashKey;

        int     m_nCount;
        CAssoc  FAR* m_pFreeList;
        struct CPlex FAR* m_pBlocks;
        int     m_nBlockSize;
};


#endif // !__MAP_KV_H__
 

