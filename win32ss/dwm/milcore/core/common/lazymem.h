// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//      Defines classes CLazyMemBlock and CGlyphPainterMemory
//      to provide simple memory management for text rendering.
//
//----------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
//  Class:
//      CLazyMemBlock
//
//  Synopsis:
//      A simplest temporary memory allocator.
//      The instance on CLazyMemBlock holds single block of memory.
//      It can substitute the pair of alloc-and-free calls in some
//      routine that need temporary memory block. The idea is
//      lazy deallocation. Instead of immediate freeing the memory,
//      CLazyMemBlock continues keeping it for possible reusing,
//      thus decreasing allocation cost and memory fragmentation.
//------------------------------------------------------------------------------
class CLazyMemBlock
{
public:
    CLazyMemBlock()
    {
        m_pData = NULL;
        m_size = 0;
    }
    ~CLazyMemBlock() {Clean();}
    void Clean();

    __allocator __success(return!=NULL) __bcount(size) void* EnsureSize(
        __out_range(<=, this->m_size) UINT size
        )
    {
        return m_size >= size ? m_pData : Reallocate(size);
    }

    __success(return!=NULL) __bcount(this->m_size) void* GetData() const { return m_pData; }
    __range(==, this->m_size) UINT GetSize() const { return m_size; }

private:
    __field_bcount(m_size) void* m_pData;
    UINT m_size;

private:
    __allocator __success(return!=NULL) __bcount(size) void* Reallocate(
        __out_range(==, this->m_size) UINT size
        );
};

struct GlyphBitmap;

//------------------------------------------------------------------------------
//  Class:
//      CGlyphPainterMemory
//
//  Synopsis:
//      A pack of lazy memory allocators for text rendering.
//------------------------------------------------------------------------------
class CGlyphPainterMemory
{
public:
    __success(return!=NULL) __ecount(usSize) POINT* AllocPositions(USHORT usSize)
    {
        UINT uSize = static_cast<UINT>(usSize) * sizeof(POINT);
        void* pMemory = m_positions.EnsureSize(uSize);
        return reinterpret_cast<POINT*>(pMemory);
    }

    __success(return!=NULL) __ecount(usSize) GlyphBitmap const** AllocGlyphBitmapRefs(USHORT usSize)
    {
        UINT uSize = static_cast<UINT>(usSize) * sizeof(GlyphBitmap const*);
        void* pMemory = m_glyphBitmapRefs.EnsureSize(uSize);
        return reinterpret_cast<GlyphBitmap const**>(pMemory);
    }

    __success(return!=NULL) __ecount(cbSize) BYTE* AllocRunBitmap(UINT cbSize)
    {
        void* pMemory = m_runBitmap.EnsureSize(cbSize);
        return reinterpret_cast<BYTE*>(pMemory);
    }

    __success(return!=NULL) __ecount(uSize) BYTE* AllocAlphaArray(UINT uSize)
    {
        void* pMemory = m_alphaArray.EnsureSize(uSize);
        return reinterpret_cast<BYTE*>(pMemory);
    }

    void Clean();
    void CleanHuge();

private:
    CLazyMemBlock m_positions;
    CLazyMemBlock m_glyphBitmapRefs;
    CLazyMemBlock m_runBitmap;
    CLazyMemBlock m_alphaArray;
};

