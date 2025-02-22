/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
 * Copyright 2016 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

class CCRTAllocator
{
public:
    static void* Allocate(_In_ size_t size)
    {
        return malloc(size);
    }

    static void* Reallocate(_In_opt_ void* ptr, _In_ size_t size)
    {
        return realloc(ptr, size);
    }

    static void Free(_In_opt_ void* ptr)
    {
        free(ptr);
    }
};

class CLocalAllocator
{
public:
    static void* Allocate(_In_ size_t size)
    {
        return ::LocalAlloc(LMEM_FIXED, size);
    }

    static void* Reallocate(_In_opt_ void* ptr, _In_ size_t size)
    {
        if (!ptr)
            return Allocate(size);
        if (size == 0)
        {
            Free(ptr);
            return NULL;
        }
        return ::LocalReAlloc(ptr, size, 0);
    }

    static void Free(_In_opt_ void* ptr)
    {
        ::LocalFree(ptr);
    }
};

class CGlobalAllocator
{
public:
    static void* Allocate(_In_ size_t size)
    {
        return ::GlobalAlloc(GMEM_FIXED, size);
    }

    static void* Reallocate(_In_opt_ void* ptr, _In_ size_t size)
    {
        if (!ptr)
            return Allocate(size);
        if (size == 0)
        {
            Free(ptr);
            return NULL;
        }
        return ::GlobalReAlloc(ptr, size, 0);
    }

    static void Free(_In_opt_ void* ptr)
    {
        GlobalFree(ptr);
    }
};


template<class T, class Allocator = CCRTAllocator>
class CHeapPtr
{
public:
    CHeapPtr() :
        m_pData(NULL)
    {
    }

    explicit CHeapPtr(T *lp) :
        m_pData(lp)
    {
    }

    explicit CHeapPtr(CHeapPtr<T, Allocator> &lp)
    {
        m_pData = lp.Detach();
    }

    ~CHeapPtr()
    {
        Free();
    }

    CHeapPtr<T, Allocator>& operator = (CHeapPtr<T, Allocator> &lp)
    {
        if (lp.m_pData != m_pData)
            Attach(lp.Detach());
        return *this;
    }

    bool AllocateBytes(_In_ size_t nBytes)
    {
        ATLASSERT(m_pData == NULL);
        m_pData = static_cast<T*>(Allocator::Allocate(nBytes));
        return m_pData != NULL;
    }

    bool ReallocateBytes(_In_ size_t nBytes)
    {
        T* newData = static_cast<T*>(Allocator::Reallocate(m_pData, nBytes));
        if (newData == NULL)
            return false;
        m_pData = newData;
        return true;
    }

    bool Allocate(_In_ size_t nElements = 1)
    {
        return AllocateBytes(nElements * sizeof(T));
    }

    bool Reallocate(_In_ size_t nElements)
    {
        return ReallocateBytes(nElements * sizeof(T));
    }

    void Free()
    {
        if (m_pData)
        {
            Allocator::Free(m_pData);
            m_pData = NULL;
        }
    }

    void Attach(T *lp)
    {
        Allocator::Free(m_pData);
        m_pData = lp;
    }

    T *Detach()
    {
        T *saveP = m_pData;
        m_pData = NULL;
        return saveP;
    }

    T **operator &()
    {
        ATLASSERT(m_pData == NULL);
        return &m_pData;
    }

    operator T* () const
    {
        return m_pData;
    }

    T* operator->() const
    {
        return m_pData;
    }

public:
    T *m_pData;
};

