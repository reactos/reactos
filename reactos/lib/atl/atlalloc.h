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
        m_Data(NULL)
    {
    }

    explicit CHeapPtr(T *lp) :
        m_Data(lp)
    {
    }

    explicit CHeapPtr(CHeapPtr<T, Allocator> &lp)
    {
        m_Data = lp.Detach();
    }

    ~CHeapPtr()
    {
        Free();
    }

    CHeapPtr<T, Allocator>& operator = (CHeapPtr<T, Allocator> &lp)
    {
        if (lp.m_Data != m_Data)
            Attach(lp.Detach());
        return *this;
    }

    bool AllocateBytes(_In_ size_t nBytes)
    {
        ATLASSERT(m_Data == NULL);
        m_Data = static_cast<T*>(Allocator::Allocate(nBytes));
        return m_Data != NULL;
    }

    bool ReallocateBytes(_In_ size_t nBytes)
    {
        T* newData = static_cast<T*>(Allocator::Reallocate(m_Data, nBytes));
        if (newData == NULL)
            return false;
        m_Data = newData;
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
        if (m_Data)
        {
            Allocator::Free(m_Data);
            m_Data = NULL;
        }
    }

    void Attach(T *lp)
    {
        Allocator::Free(m_Data);
        m_Data = lp;
    }

    T *Detach()
    {
        T *saveP = m_Data;
        m_Data = NULL;
        return saveP;
    }

    T **operator &()
    {
        ATLASSERT(m_Data == NULL);
        return &m_Data;
    }

    operator T* () const
    {
        return m_Data;
    }

    T* operator->() const
    {
        return m_Data;
    }

protected:
    T *m_Data;
};

