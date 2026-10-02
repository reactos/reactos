// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

namespace WPF
{

//------------------------------------------------------------------------------
// Heap inlines ----------------------------------------------------------------
// Note:    We want to have inlines for all of the heap operations so that when
//          we aren't doing a metered build the compiler can know that these
//          parameters aren't being used and we won't push any values on the
//          stack.

#if defined(PERFMETER)

__allocator inline __bcount(cbSize) void *
Alloc(__inout_ecount(1) Heap * pheap, PERFMETERTAG mt, size_t cbSize)
{
    return pheap->Alloc(mt, cbSize);
}

__allocator inline __bcount(cbSize) void *
AllocClear(__inout_ecount(1) Heap * pheap, PERFMETERTAG mt, size_t cbSize)
{
    return pheap->AllocClear(mt, cbSize);
}

__allocator inline HRESULT
Realloc(__inout_ecount(1) Heap * pheap, PERFMETERTAG mt, __deref_bcount(cbSize) void ** ppv, size_t cbSize)
{
    return pheap->Realloc(mt, ppv, cbSize);
}

inline void
Free(__inout_ecount(1) Heap * pheap, void * pv)
{
    pheap->Free(pv);
}

__allocator inline __bcount(cbSize) void * _cdecl UseOperatorNewWithMemoryMeterInstead(size_t cbSize)
{
    PERFMETERTAG mtDefault = DbgExMtGetDefaultMeter();

    if (!mtDefault && DbgExIsFullDebug())
    {
        //--------------------------------------------------------------------------
        //  Note to users:
        //
        //  If you hit this assert then you are note playing nice in the meter
        //  system.  You should be allocating on a meter. 
        //--------------------------------------------------------------------------

        RIPW(L"Invalid use of global operator new.  "
             L"Use either the version which requires a meter tag and heap "
             L"or if this allocation is out of your control, set a default meter.");

        mtDefault = Mt(OpNew);
    }


    return(Alloc(ProcessHeap, mtDefault, cbSize));
}

#else // defined(PERFMETER)

__allocator inline __bcount(cbSize) void *
Alloc(__inout_ecount(1) Heap * pheap, PERFMETERTAG, size_t cbSize)
{
    return pheap->Alloc(cbSize);
}

__allocator inline __bcount(cbSize) void *
AllocClear(__inout_ecount(1) Heap * pheap, PERFMETERTAG mt, size_t cbSize)
{
    return pheap->AllocClear(cbSize);
    mt;
}

__allocator inline HRESULT
Realloc(__inout_ecount(1) Heap * pheap, PERFMETERTAG, __deref_bcount(cbSize) void ** ppv, size_t cbSize)
{
    return pheap->Realloc(ppv, cbSize);
}

inline void
Free(__inout_ecount(1) Heap * pheap, void * pv)
{
    pheap->Free(pv);
}
#endif // defined(PERFMETER)

// Realloc, HrMalloc, HrAlloc, & HrMallocAligned annotation helpers - since
// these are often wrapped by a macro like IFC it is impossible to just mark
// the end of a Realloc in WPFRealloc macro.  This helper method is forced
// inline and has a further special annotation marking that the prior
// annotation holds the most detailed tag.

__forceinline HRESULT ReallocAnnotationHelper(
    __in_ecount(1) Heap* pheap,
    PERFMETERTAG mt,
    size_t cbSize,
    __deref_bcount(cbSize) void ** ppv
    )
{
    return Realloc(pheap, mt, ppv, cbSize);
    // Direct reader to look at prior ReAlloc annotation.
    __annotation(L"Allocate", L"<END>", L"<UsePriorAnnotation>", L"ReAlloc");
}

__forceinline HRESULT HrMallocAnnotationHelper(
    PERFMETERTAG mt,
    size_t cbElementSize,
    size_t cElements,
    __deref_bcount(cbElementSize*cElements) void **ppvmemblock
    )
{
    return HrMalloc(mt, cbElementSize, cElements, ppvmemblock);
    // Direct reader to look at prior HrMalloc annotation.
    __annotation(L"Allocate", L"<END>", L"<UsePriorAnnotation>", L"HrMalloc");
}

__forceinline HRESULT HrAllocAnnotationHelper(
    PERFMETERTAG mt,
    size_t cbSize,
    __deref_bcount(cbSize) void **ppvmemblock
    )
{
    return HrAlloc(mt, cbSize, ppvmemblock);
    // Direct reader to look at prior HrAlloc annotation.
    __annotation(L"Allocate", L"<END>", L"<UsePriorAnnotation>", L"HrAlloc");
}

__forceinline HRESULT HrMallocAlignAnnotationHelper(
    PERFMETERTAG mt,
    size_t cbElementSize,
    size_t cElements,
    __deref_bcount(cbElementSize*cElements) void **ppvmemblockAligned,
    __deref_bcount(cbElementSize*cElements + 128) void **ppvmemblock
    )
{
    return HrMallocAlign(mt, cbElementSize, cElements, ppvmemblockAligned, ppvmemblock);
    // Direct reader to look at prior HrMallocAligned annotation.
    __annotation(L"Allocate", L"<END>", L"<UsePriorAnnotation>", L"HrMallocAlign");
}

} // namespace WPF

//
// HRESULT returning allocation routine wrapper macros
//
//  The macros use stringized MeterTag (mt) to annotate symbols file.  This
//  allows tools to look up what of allocation is made in a particular stack.
//

#define HrMalloc(mt, cbElementSize, cElements, ppvmemblock)                     \
    ( __annotation(L"Allocate", LSTRINGIZE(mt), L"HrMalloc"),                   \
      WPF::HrMallocAnnotationHelper(mt, cbElementSize, cElements, ppvmemblock) )

#define HrAlloc(mt, cbSize, ppvmemblock)                        \
    ( __annotation(L"Allocate", LSTRINGIZE(mt), L"HrAlloc"),    \
      WPF::HrAllocAnnotationHelper(mt, cbSize, ppvmemblock)  )

#define HrMallocAlign(mt, cbElementSize, cElements, ppvmemblockAligned, ppvmemblock)    \
    ( __annotation(L"Allocate", LSTRINGIZE(mt), L"HrMallocAlign"),                      \
      WPF::HrMallocAlignAnnotationHelper(mt, cbElementSize, cElements, ppvmemblockAligned, ppvmemblock))

#include <new>


