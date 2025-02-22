// PROJECT:        ReactOS ATL Simple Collection
// LICENSE:        Public Domain
// PURPOSE:        Provides compatibility to Microsoft ATL
// PROGRAMMERS:    Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)

#ifndef __ATLSIMPCOLL_H__
#define __ATLSIMPCOLL_H__

#pragma once

#include "atlcore.h"    // for ATL Core

namespace ATL
{
template <typename T>
class CSimpleArrayEqualHelper
{
public:
    static bool IsEqual(const T& t1, const T& t2)
    {
        return t1 == t2;
    }
};

// This class exists for the element types of no comparison.
template <typename T>
class CSimpleArrayEqualHelperFalse
{
public:
    static bool IsEqual(const T&, const T&)
    {
        ATLASSERT(FALSE);
        return false;
    }
};

template <typename T, typename TEqual = CSimpleArrayEqualHelper<T> >
class CSimpleArray
{
public:
    typedef T _ArrayElementType;

    CSimpleArray() : m_pData(NULL), m_nCount(0), m_nCapacity(0)
    {
    }

    CSimpleArray(const CSimpleArray<T, TEqual>& src) :
        m_pData(NULL), m_nCount(0), m_nCapacity(0)
    {
        *this = src;
    }

    ~CSimpleArray()
    {
        RemoveAll();
    }

    BOOL Add(const T& t)
    {
        // is the capacity enough?
        if (m_nCapacity < m_nCount + 1)
        {
            // allocate extra capacity for optimization
            const int nNewCapacity = (m_nCount + 1) + c_nGrow;
            T *pNewData = (T *)realloc(static_cast<void *>(m_pData), nNewCapacity * sizeof(T));
            if (pNewData == NULL)
                return FALSE;   // failure

            m_pData = pNewData;
            m_nCapacity = nNewCapacity;
        }

        // call constructor
        ConstructItemInPlace(m_nCount, t);

        // increment
        ++m_nCount;

        return TRUE;
    }

    int Find(const T& t) const
    {
        for (int nIndex = 0; nIndex < m_nCount; ++nIndex)
        {
            if (TEqual::IsEqual(m_pData[nIndex], t))
            {
                return nIndex;  // success
            }
        }
        return -1;  // failure
    }

    T* GetData()
    {
        return m_pData;
    }

    const T* GetData() const
    {
        return m_pData;
    }

    int GetSize() const
    {
        return m_nCount;
    }

    BOOL Remove(const T& t)
    {
        return RemoveAt(Find(t));
    }

    void RemoveAll()
    {
        if (m_pData)
        {
            // call destructor
            const int nCount = m_nCount;
            for (int nIndex = 0; nIndex < nCount; ++nIndex)
            {
                DestructItem(nIndex);
            }

            free(m_pData);
            m_pData = NULL;
        }
        m_nCount = 0;
        m_nCapacity = 0;
    }

    BOOL RemoveAt(int nIndex)
    {
        // boundary check
        if (nIndex < 0 || m_nCount <= nIndex)
            return FALSE;   // failure

        // call destructor
        DestructItem(nIndex);

        // move range [nIndex + 1, m_nCount) to nIndex
        const int nRightCount = m_nCount - (nIndex + 1);
        const int nRightSize = nRightCount * sizeof(T);
        memmove(static_cast<void *>(&m_pData[nIndex]), &m_pData[nIndex + 1], nRightSize);

        // decrement
        --m_nCount;

        return TRUE;
    }

    BOOL SetAtIndex(int nIndex, const T& t)
    {
        // boundary check
        if (nIndex < 0 || m_nCount <= nIndex)
            return FALSE;   // failure

        // store it
        m_pData[nIndex] = t;
        return TRUE;
    }

    T& operator[](int nIndex)
    {
        ATLASSERT(0 <= nIndex && nIndex < m_nCount);
        return m_pData[nIndex];
    }

    const T& operator[](int nIndex) const
    {
        ATLASSERT(0 <= nIndex && nIndex < m_nCount);
        return m_pData[nIndex];
    }

    CSimpleArray<T, TEqual>& operator=(const CSimpleArray<T, TEqual>& src)
    {
        // don't copy if two objects are same
        if (this == &src)
            return *this;

        if (src.GetSize() != GetSize())
        {
            RemoveAll();

            int nNewCount = src.GetSize();

            T *pNewData = (T *)realloc(static_cast<void *>(m_pData), nNewCount * sizeof(T));
            ATLASSERT(pNewData);
            if (pNewData == NULL)
                return *this;   // failure

            // store new
            m_pData = pNewData;
            m_nCount = nNewCount;
            m_nCapacity = nNewCount;
        }
        else
        {
            for (int nIndex = 0; nIndex < m_nCount; ++nIndex)
            {
                DestructItem(nIndex);
            }
        }

        ATLASSERT(GetSize() == src.GetSize());
        for (int nIndex = 0; nIndex < src.GetSize(); ++nIndex)
        {
            ConstructItemInPlace(nIndex, src[nIndex]);
        }

        return *this;
    }

protected:
    T *     m_pData;                // malloc'ed
    int     m_nCount;               // # of items of type T
    int     m_nCapacity;            // for optimization
    static const int c_nGrow = 8;   // for optimization

    // NOTE: Range m_pData[0] .. m_pData[m_nCapacity - 1] are accessible.
    // NOTE: Range [0, m_nCount) are constructed.
    // NOTE: Range [m_nCount, m_nCapacity) are not constructed.
    // NOTE: 0 <= m_nCount && m_nCount <= m_nCapacity.

    // call constructor at nIndex
    void ConstructItemInPlace(int nIndex, const T& src)
    {
        new(&m_pData[nIndex]) ConstructImpl(src);
    }

    // call destructor at nIndex
    void DestructItem(int nIndex)
    {
        m_pData[nIndex].~T();
    }

private:

    struct ConstructImpl
    {
        ConstructImpl(const T& obj)
            :m_ConstructHelper(obj)
        {
        }

        static void *operator new(size_t, void *ptr)
        {
            return ptr;
        }

        static void operator delete(void *p, void* )
        {
        }

        T m_ConstructHelper;
    };

};

template <typename TKey, typename TVal>
class CSimpleMapEqualHelper
{
public:
    static bool IsEqualKey(const TKey& k1, const TKey& k2)
    {
        return k1 == k2;
    }

    static bool IsEqualValue(const TVal& v1, const TVal& v2)
    {
        return v1 == v2;
    }
};

// This class exists for the keys and the values of no comparison.
template <typename TKey, typename TVal>
class CSimpleMapEqualHelperFalse
{
public:
    static bool IsEqualKey(const TKey& k1, const TKey& k2)
    {
        ATLASSERT(FALSE);
        return false;
    }

    static bool IsEqualValue(const TVal& v1, const TVal& v2)
    {
        ATLASSERT(FALSE);
        return false;
    }
};

template <typename TKey, typename TVal,
          typename TEqual = CSimpleMapEqualHelper<TKey, TVal> >
class CSimpleMap
{
public:
    typedef TKey _ArrayKeyType;
    typedef TVal _ArrayElementType;

    CSimpleMap()
    {
    }

    ~CSimpleMap()
    {
    }

    BOOL Add(const TKey& key, const TVal& val)
    {
        Pair pair(key, val);
        return m_Pairs.Add(pair);
    }

    int FindKey(const TKey& key) const
    {
        const int nCount = GetSize();
        for (int nIndex = 0; nIndex < nCount; ++nIndex)
        {
            if (TEqual::IsEqualKey(m_Pairs[nIndex].key, key))
            {
                return nIndex;  // success
            }
        }
        return -1;  // failure
    }

    int FindVal(const TVal& val) const
    {
        const int nCount = GetSize();
        for (int nIndex = 0; nIndex < nCount; ++nIndex)
        {
            if (TEqual::IsEqualValue(m_Pairs[nIndex].val, val))
            {
                return nIndex;  // success
            }
        }
        return -1;  // failure
    }

    TKey& GetKeyAt(int nIndex)
    {
        ATLASSERT(0 <= nIndex && nIndex < GetSize());
        return m_Pairs[nIndex].key;
    }

    const TKey& GetKeyAt(int nIndex) const
    {
        ATLASSERT(0 <= nIndex && nIndex < GetSize());
        return m_Pairs[nIndex].key;
    }

    int GetSize() const
    {
        return m_Pairs.GetSize();
    }

    TVal& GetValueAt(int nIndex)
    {
        ATLASSERT(0 <= nIndex && nIndex < GetSize());
        return m_Pairs[nIndex].val;
    }

    const TVal& GetValueAt(int nIndex) const
    {
        ATLASSERT(0 <= nIndex && nIndex < GetSize());
        return m_Pairs[nIndex].val;
    }

    TVal Lookup(const TKey& key) const
    {
        int nIndex = FindKey(key);
        if (nIndex < 0)
            return TVal();
        return m_Pairs[nIndex].val;
    }

    BOOL Remove(const TKey& key)
    {
        int nIndex = FindKey(key);
        return RemoveAt(nIndex);
    }

    void RemoveAll()
    {
        m_Pairs.RemoveAll();
    }

    BOOL RemoveAt(int nIndex)
    {
        return m_Pairs.RemoveAt(nIndex);
    }

    TKey ReverseLookup(const TVal& val) const
    {
        int nIndex = FindVal(val);
        if (nIndex < 0)
            return TKey();
        return m_Pairs[nIndex].key;
    }

    BOOL SetAt(const TKey& key, const TVal& val)
    {
        int nIndex = FindKey(key);
        if (nIndex < 0)
            return Add(key, val);

        m_Pairs[nIndex].val = val;
        return TRUE;
    }

    BOOL SetAtIndex(int nIndex, const TKey& key, const TVal& val)
    {
        // boundary check
        if (nIndex < 0 || GetSize() <= nIndex)
            return FALSE;

        m_Pairs[nIndex].key = key;
        m_Pairs[nIndex].val = val;
        return TRUE;
    }

protected:
    struct Pair
    {
        TKey key;
        TVal val;

        Pair()
        {
        }

        Pair(const TKey& k, const TVal& v) : key(k), val(v)
        {
        }

        Pair(const Pair& pair) : key(pair.key), val(pair.val)
        {
        }

        Pair& operator=(const Pair& pair)
        {
            key = pair.key;
            val = pair.val;
            return *this;
        }
    };

    CSimpleArray<Pair, CSimpleArrayEqualHelperFalse<Pair> > m_Pairs;
};

}

#endif
