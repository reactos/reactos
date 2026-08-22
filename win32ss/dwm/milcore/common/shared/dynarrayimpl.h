// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************\
* 

*
* Module Name:
*
*   Dynamic array implementation class
*
* Abstract:
*
*   This class contains definitions of functions which can't (or shouldn't)
*   be inlined. We use a separate implementation class because this allows us
*   to avoid the code bloat template classes produce; every instance of the
*   DynArray template can use the same version of each out-of-line member.
*
*   The DynArray data members need to be declared here, because they are
*   used by DynArrayImpl.
*
\**************************************************************************/

#pragma once

template <bool fZeroMemory = false> class DynArrayImpl
{
protected:
    
    // Constructor
    //

    DynArrayImpl(
        __inout_bcount_part_opt(allocSize*eltSize, count*eltSize) void *initialAllocation,
            // the initial allocation, which can be global,
            // static or dynamic memory (or NULL)
        __out_range(==, this->Capacity) __out_range(==, this->AllocSize) UINT allocSize,
            // size of the initial allocation (0 if there is none)
        __in_range(<=, allocSize) __out_range(==, this->Count) UINT count,
            // initial number of elements
        __out_range(==, this->ElementSize) UINT eltSize
            // size of an element (used only if zeroing memory)
        );
    
    // Destructor
    // NOT virtual to avoid vtable.
    // inheriting classes must be careful

    ~DynArrayImpl();

    // Shrink the buffer so that it is just big enough for the items
    // the dynamic array holds.

    VOID ShrinkToSize(__in_range(==, this->ElementSize) UINT eltSize);
    
    // Add space for new elements (if necessary). Does not update Count.

    HRESULT Grow(
        __in_range(==, this->ElementSize) UINT eltSize,
            // size of each element
        __out_range(<=, this->Capacity-this->Count) UINT newElements,
            // number of new elements
        BOOL exactSize = FALSE
            // no exponential growth
        );

    // Add new, uninitialized elements, and return a pointer to them.

    HRESULT AddMultiple(
        __in_range(==, this->ElementSize) UINT eltSize,
            // size of each element
        __out_range(==, this->Count-pre(this->Count)) __out_range(<=, this->Capacity-pre(this->Count)) UINT newElements,
            // number of new elements
        __deref_opt_out_xcount_part(newElements*eltSize,
                                    fZeroMemory ? newElements*eltSize : 0)
            void **ppData
            // pointer to the first of the new elements copied to the new space
        );
    
    // Add new elements, initializing them with the given data.
    
    HRESULT AddMultipleAndSet(
        __in_range(==, this->ElementSize) UINT eltSize,
            // size of each element
        __out_range(==, this->Count-pre(this->Count)) __out_range(<=, this->Capacity-pre(this->Count)) UINT newElements,
            // number of new elements
        __in_bcount(newElements*eltSize) const void *newData
            // the data to be copied into the new space
        );
    
    // Detach the data buffer from the dynamic array.
    // Allocates the buffer to detatch if it is the initial allocation.

    HRESULT DetachData(
        __in_range(==, this->ElementSize) UINT eltSize,
        __deref_out_bcount_part(this->Capacity*eltSize, this->Count*eltSize) void **buffer
        );
   
    // Capacity, StepSize, AllocSize and Count are all measured in elements,
    // not bytes.

    enum {
        kMinCapacityGrowth = 16,
        kMaxCapacityGrowth = 8092
    };

    __field_bcount(Capacity*ElementSize)
    BYTE *m_pData;            // Pointer to the data buffer (initial or later heap-allocated)

    __field_bcount(AllocSize*ElementSize)
    void * const InitialAllocation; // Buffer of elements in the initial allocation

    UINT const AllocSize;     // The number of elements in the initial allocation
    UINT Capacity;            // The number of elements in that will fit in the current buffer
    __field_range(<=, Capacity)
    UINT Count;               // The current number of elements in the array

    // Future Consideration:   Consider always storing DynArray element size
    //  since there is always 4 bytes to spare for an 8-byte aligned class.
#if ANALYSIS
    UINT const ElementSize;   // The size in bytes of a single element.
#endif
};



