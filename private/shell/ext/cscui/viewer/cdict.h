//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cdict.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CDICT_H
#define _INC_CSCVIEW_CDICT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: cdict.h

    Description: A simple dictionary template class for storing and retrieving
        any object indexed on a string value.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_CSCVIEW_CLIST_H
#   include "clist.h"
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

template <class T> class CDictionary; // fwd decl.

template <class T>
class CDictEntry
{
    public:
        CDictEntry(void) { }
            
        CDictEntry(const CString& strKey, const T& item)
            : m_strKey(strKey),
              m_item(item) { }

        CDictEntry(const CDictEntry<T>& rhs);

        CDictEntry& operator = (const CDictEntry<T>& rhs);

        ~CDictEntry(void) { }

        CString m_strKey;
        T m_item;
};


template <class T>
class CDictionary
{
    public:
        explicit CDictionary(int cHashBuckets = 101);
        ~CDictionary(void) { delete[] m_prgBuckets; };

        int Count(void) const
            { return m_cEntries; }

        void Store(const CString& strKey, const T& item);
        bool Retrieve(const CString& strKey, T *pItem) const;
        T* Locate(const CString& strKey) const;

    private:
        CList< CDictEntry<T> > *m_prgBuckets;
        int m_cEntries;
        int m_cHashBuckets;

        int ComputeHashValue(const CString& str) const;
        T* FindInBucket(int iBucket, const CString& str) const;

        //
        // Prevent copy.
        //
        CDictionary(const CDictionary<T>& rhs);
        CDictionary& operator = (const CDictionary<T>& rhs);
};


template <class T>
CDictionary<T>::CDictionary(int cHashBuckets)
            : m_prgBuckets(NULL),
              m_cEntries(0),
              m_cHashBuckets(cHashBuckets)
{
    m_prgBuckets = new CList< CDictEntry<T> >[m_cHashBuckets];
}


template <class T>
CDictEntry<T>::CDictEntry(
    const CDictEntry<T>& rhs
    )
{
    *this = rhs;
}

template <class T>
CDictEntry<T>&
CDictEntry<T>::operator = (
    const CDictEntry<T>& rhs
    )
{
    if (this != &rhs)
    {
        m_strKey = rhs.m_strKey;
        m_item   = rhs.m_item;
    }
    return *this;
}

template <class T>
int
CDictionary<T>::ComputeHashValue(
    const CString& str
    ) const
{
    LPCTSTR psz = (LPCTSTR)str;
    int n = 0;
    while(*psz)
        n += int(*psz++);

    return n % m_cHashBuckets;
}


template <class T>
T*
CDictionary<T>::FindInBucket(
    int iBucket,
    const CString& str
    ) const
{
    DBGASSERT((NULL != m_prgBuckets));
    DBGASSERT((iBucket < m_cHashBuckets));
    CListIterator< CDictEntry<T> >iter(m_prgBuckets[iBucket]);
    CDictEntry<T> *pBucketEntry;

    while(iter.Next(&pBucketEntry))
    {
        DBGASSERT((NULL != pBucketEntry));
        if (pBucketEntry->m_strKey == str)
            return &pBucketEntry->m_item;
    }
    return NULL;
}


template <class T>
void 
CDictionary<T>::Store(
    const CString& strKey, 
    const T& item
    )
{
    int iBucket = ComputeHashValue(strKey);
    T* pItem = FindInBucket(iBucket, strKey);
    if (NULL != pItem)
    {
        *pItem = item;
    }
    else
    {
        DBGASSERT((NULL != m_prgBuckets));
        m_prgBuckets[iBucket].Insert(CDictEntry<T>(strKey, item));
    }
}


template <class T>
bool 
CDictionary<T>::Retrieve(
    const CString& strKey, 
    T *pItemOut
    ) const
{
    DBGASSERT((NULL != pItemOut));
    T *pItem = Locate(strKey);
    if (NULL != pItem)
    {
        *pItemOut = *pItem;
        return true;
    }
    return false;
}


template <class T>
T*
CDictionary<T>::Locate(
    const CString& strKey
    ) const
{
    int iBucket = ComputeHashValue(strKey);
    T* pItem = FindInBucket(iBucket, strKey);
    if (NULL != pItem)
    {
        return pItem;
    }
    return NULL;
}



#endif // __DICTIONARY_H

