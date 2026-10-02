// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Simple memory allocation for CProgram.
//
//-----------------------------------------------------------------------------
#pragma once

//+------------------------------------------------------------------------------
//
//  Class:
//      CFlushMemory
//
//  Synopsis:
//      Provides a storage for a number of small object.
//      Allocate memory blocks of requested size in relatively big chanks.
//      Does not provide means to free allocated blocks.
//      Instances of CFlushMemory are assumed to be short-term objects
//      so that the memory allocated for chunks will be freed
//      soon enough.
//
//-------------------------------------------------------------------------------
class CFlushMemory
{
public:
    CFlushMemory()
    {
        m_pChunks          = NULL;
        m_pAvailableMemory = NULL;
        m_uAvailableSize   = 0;
        m_fOverflow        = FALSE;
    }

    // construct and inherit memory ownershop
    CFlushMemory(CFlushMemory & holder)
    {
        m_pChunks          = holder.m_pChunks         ;
        m_pAvailableMemory = holder.m_pAvailableMemory;
        m_uAvailableSize   = holder.m_uAvailableSize  ;
        m_fOverflow        = holder.m_fOverflow       ;

        holder.m_pChunks          = NULL;
        holder.m_pAvailableMemory = NULL;
        holder.m_uAvailableSize   = 0;
        holder.m_fOverflow        = FALSE;
    }

    ~CFlushMemory()
    {
        Flush();
    }

    __bcount(cbSize) UINT8* Alloc(UINT32 cbSize);
    BOOL WasOverflow() const { return m_fOverflow; }
    void Flush();

private:
    class CChunk
    {
    public:
        // Define placement operator new and do-nothing delete
        __bcount(cbSize) void * __cdecl operator new(size_t cbSize, void * pvMemory)
        {
            UNREFERENCED_PARAMETER(cbSize);
            return pvMemory;
        }
        void __cdecl operator delete(void*, void*)
        {
            WarpError("CChunk::operator delete");
        }
        CChunk(CChunk * pNext) : m_pNext(pNext)
        {
        }

        CChunk * const m_pNext;
    } * m_pChunks;

    UINT8 *m_pAvailableMemory;
    UINT32 m_uAvailableSize;
    BOOL m_fOverflow;
};


//+------------------------------------------------------------------------------
//
//  Class:
//      CFlushObject
//
//  Synopsis:
//      Base class for objects that use flush memory.
//
//-------------------------------------------------------------------------------
class CFlushObject
{
private:
    //
    // Define private operators "new" and "delete" to protect
    // against using standard allocation.
    // These operators are not implemented on purpose, so that
    // unintentional use of "new Foo", where Foo is derived from
    // CFlushObject, will cause build failures.
    //
    __bcount(cbSize) void * __cdecl operator new(size_t cbSize);
    __bcount(cbSize) void * __cdecl operator new[](size_t cbSize);
    void __cdecl operator delete(void*)
    {
        WarpError("CFlushObject::operator delete");
    }
    void __cdecl operator delete[](void*)
    {
        WarpError("CFlushObject::operator delete[]");
    }

public:
    // Define placement operator new and do-nothing delete
    __bcount(cbSize) void * __cdecl operator new(size_t cbSize, void * pvMemory)
    {
        UNREFERENCED_PARAMETER(cbSize);
        return pvMemory;
    }
    void __cdecl operator delete(void*, void*)
    {
        WarpError("CFlushObject::operator delete");
    }
};


