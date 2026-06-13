// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#pragma once

#ifdef _MANAGED
#pragma unmanaged
#endif

//------------------------------------------------------------------------------
// Meter Support ---------------------------------------------------------------
MtExtern(Mem)
MtExtern(OpNew)

// When this guy is true, we don't verify that allocations happen on meters that
// are eventual children to the "Mem" meter.
#if DBG==1
extern BOOL g_fNoMeterChecks;
#endif

EXTERN_C HANDLE g_hProcessHeap;

// LSTRINGIZE: Produces wide character string of whatever expression is given
//  Use this instead of direct stringize operator # when the expression may
//  itself contain a macro which is useful to expand in the string.
#define LSTRINGIZE(x) L#x

//
// WPF allocation macros
//
//  These are the preferred routines to call to make allocations.
//
//  The macros use stringized MeterTag (mt) to annotate symbols file.  This
//  allows tools to look up what of allocation is made in a particular stack.
//

#define WPFAlloc(h, mt, cb) ( __annotation(L"Allocate", LSTRINGIZE(mt)), WPF::Alloc(h, mt, cb)); __annotation(L"Allocate", L"<END>", LSTRINGIZE(mt))
#define WPFAllocType(type, h, mt, cb) ( __annotation(L"Allocate", LSTRINGIZE(mt)), reinterpret_cast<type>(WPF::Alloc(h, mt, cb))); __annotation(L"Allocate", L"<END>", LSTRINGIZE(mt))
#define WPFAllocClear(h, mt, cb) ( __annotation(L"Allocate", LSTRINGIZE(mt), L"ZeroInit"), WPF::AllocClear(h, mt, cb)); __annotation(L"Allocate", L"<END>", LSTRINGIZE(mt), L"ZeroInit")
#define WPFAllocTypeClear(type, h, mt, cb) ( __annotation(L"Allocate", LSTRINGIZE(mt), L"ZeroInit"), reinterpret_cast<type>(WPF::AllocClear(h, mt, cb))); __annotation(L"Allocate", L"<END>", LSTRINGIZE(mt), L"ZeroInit")
#define WPFRealloc(h, mt, ppv, cb) ( __annotation(L"Allocate", LSTRINGIZE(mt), L"ReAlloc"), WPF::ReallocAnnotationHelper(h, mt, cb, ppv))
#define WPFFree(h, pv) ( WPF::Free(h, pv) )


namespace WPF
{

//
// WPF HRESULT returning allocation routines
//
//  Note: These routines are also annotated with macros of the same name.  For
//        this reason do not use WPF namespace to call these.  Macros are
//        squirreled away in MemUtils.inl if needed.
//
// The HrMalloc* routines are also size overflow safe.
//

__checkReturn __allocator
HRESULT HrMalloc(
    PERFMETERTAG mt,
    size_t cbElementSize,
    size_t cElements,
    __deref_bcount(cbElementSize*cElements) void **ppvmemblock
    );

__checkReturn __allocator
HRESULT HrAlloc(
    PERFMETERTAG mt,
    size_t cbSize,
    __deref_bcount(cbSize) void **ppvmemblock
    );

__checkReturn __allocator
HRESULT HrMallocAlign(
    PERFMETERTAG mt,
    size_t cbElementSize,
    size_t cElements,
    __deref_bcount(cbElementSize*cElements) void **ppvmemblockAligned,
    __deref_bcount(cbElementSize*cElements + 128) void **ppvmemblock
    );


//------------------------------------------------------------------------------
//  Heap Notes:
//  ReallocClear:   These methods are problematic.  With heaps like the system
//                  heap, you have to know the old size so that you can clear
//                  the difference from the old size to the new size.  However,
//                  it is not all heaps support this intrinsically and GetSize
//                  may not return the correct size for doing this operation. If
//                  we find that we really need these routines, we can work out
//                  a way to do this with heaps for which we have enough
//                  information.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Heap abstract base classes --------------------------------------------------
class HeapBase
{
public:
    virtual ~HeapBase() {};

    __allocator virtual __bcount(cbSize) void *  Alloc(size_t cbSize) = 0;
    __allocator virtual __bcount(cbSize) void *  AllocClear(size_t cbSize) = 0;
    __allocator virtual HRESULT Realloc(__deref_bcount(cbSize) void ** ppv, size_t cbSize) = 0;
    virtual void Free(void * pv) = 0;
};

class MeterHeapBase
{
public:
    virtual ~MeterHeapBase() {};

    __allocator virtual __bcount(cbSize) void *  Alloc(PERFMETERTAG mt, size_t cbSize) = 0;
    __allocator virtual __bcount(cbSize) void *  AllocClear(PERFMETERTAG mt, size_t cbSize) = 0;
    __allocator virtual HRESULT Realloc(PERFMETERTAG mt, __deref_bcount(cbSize) void ** ppv, size_t cbSize) = 0;
    virtual void Free(void * pv) = 0;
};

// Heap typedef - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(PERFMETER)
typedef MeterHeapBase Heap;
#else
typedef HeapBase Heap;
#endif

//------------------------------------------------------------------------------
// Heap inlines ----------------------------------------------------------------
// Note:    We want to have inlines for all of the heap operations so that when
//          we aren't doing a metered build the compiler can know that these
//          parameters aren't being used and we won't push any values on the
//          stack.
__allocator inline __bcount(cbSize) void *  Alloc(__inout_ecount(1) Heap* pheap, PERFMETERTAG mt, size_t cbSize);
__allocator inline __bcount(cbSize) void *  AllocClear(__inout_ecount(1) Heap* pheap, PERFMETERTAG mt, size_t cbSize);
__allocator inline HRESULT Realloc(__inout_ecount(1) Heap* pheap, PERFMETERTAG mt, __deref_bcount(cbSize) void ** ppv, size_t cbSize);
inline void Free(__inout_ecount(1) Heap* pheap, void * pv);

} // namespace WPF

//------------------------------------------------------------------------------
// operator new inlines --------------------------------------------------------

// The regular new and delete will (a) take memory from the global ProcessHeap
// and (b) assert if there is no default meter.  The default meter exists only
// for integration with outside code that we cannot meter/heapify.
// Here is how you would use a default meter:
//
//  {
//      MtSetDefault(Mt(foo));
//      SomeRandomLibraryCallThatUsesNew();
//  }
_Ret_notnull_ _Post_writable_byte_size_(cbSize) __allocator inline __bcount(cbSize) void * __cdecl operator new(size_t cbSize);
_Ret_notnull_ _Post_writable_byte_size_(cbSize) __allocator inline __bcount(cbSize) void * __cdecl operator new[](size_t cbSize);
inline void __cdecl operator delete(void * pv);
inline void __cdecl operator delete[](void *pv);

//------------------------------------------------------------------------------
// Alternatives to new and delete ----------------------------------------------
//
// We can't use new and delete because we want to be able to allocate and
// deallocate on specific heaps.  While you are able to pass parameters (like a
// meter or heap) into new, there is no way to pass similar parameters into
// delete.  The only solution for this is to override new and delete on a class
// by class basis and have those specific implementations reference a global
// variable to get the right heap (see the DECLARE* macros below) or to
// instead use placement new and call the destructor directly.  I would suggest
// a pattern similiar to this:
//
//  class Foo
//  {
//  public:
//      static HRESULT NewFoo(IN Heap *pHeap, OUT Foo** ppFoo, ...);
//      static void DeleteFoo(IN Heap *pHeap, IN Foo* pFoo);
//  private:
//      Foo(...);
//      ~Foo(...);
//      HRESULT Init(...);
//  };
//
//  HRESULT Foo::NewFoo(IN Heap *pHeap, OUT Foo** ppFoo, ...)
//  {
//      HRESULT hr;
//      void * pv = WPFAlloc(pHeap, Mt(Foo), sizeof(Foo));
//      if (!pv)
//          return E_OUTOFMEMORY;
//
//      *ppFoo = new (pv) Foo(...);
//
//      hr = (*ppFoo)->Init(...);
//      if (FAILED(hr))
//      {
//          DeleteFoo(pHeap, *ppFoo);
//          *ppFoo = NULL;
//          return hr;
//      }
//
//      return S_OK;
//  }
//
//  void Foo::DeleteFoo(IN Heap *pHeap, IN Foo* pFoo)
//  {
//      pFoo->~Foo();
//      WPFFree(pHeap, pFoo);
//  }

// Placement new - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
_Ret_notnull_ _Post_writable_byte_size_(cb) _Post_satisfies_(return == pv)
inline __bcount(cb) void * __cdecl operator new(size_t cb, _Writable_bytes_(cb) void * pv);

// Per class new/delete override macros - - - - - - - - - - - - - - - - - - - -

// These don't make users pass in anything, both heap and meter are predefined
// This will only work with heaps that have global scope
#define DECLARE_METERHEAP_ALLOC(pheap, mt) \
    __allocator inline __bcount(cb) void * __cdecl operator new(size_t cb)    { return WPFAlloc((pheap), mt, cb); } \
    __allocator inline __bcount(cb) void * __cdecl operator new[](size_t cb)  { return WPFAlloc((pheap), mt, cb); } \
    inline void __cdecl operator delete(void * pv)   { WPFFree((pheap), pv); } \
    inline void __cdecl operator delete[](void * pv) { WPFFree((pheap), pv); } \
    inline __bcount(cb) void * __cdecl operator new(size_t cb, __bcount(cb) void * pv) { return pv; cb; } \
    inline  void __cdecl operator delete(void* pv, void*) { WPFFree(pheap, pv); }

#define DECLARE_METERHEAP_CLEAR(pheap, mt) \
    __allocator inline __bcount(cb) void * __cdecl operator new(size_t cb)    { return WPFAllocClear((pheap), mt, cb); } \
    __allocator inline __bcount(cb) void * __cdecl operator new[](size_t cb)  { return WPFAllocClear((pheap), mt, cb); } \
    inline void __cdecl operator delete(void * pv)   { WPFFree((pheap), pv); } \
    inline void __cdecl operator delete[](void * pv) { WPFFree((pheap), pv); } \
    inline __bcount(cb) void * __cdecl operator new(size_t cb, __bcount(cb) void * pv) { return memset(pv, 0, cb); } \
    inline  void __cdecl operator delete(void* pv, void*) { WPFFree(pheap, pv); }

namespace WPF
{

// Standard Heaps --------------------------------------------------------------
class ProcessHeapImpl;
extern ProcessHeapImpl * g_pProcessHeap;

#define ProcessHeap (reinterpret_cast <WPF::Heap*>(WPF::g_pProcessHeap))

// Debug Support ---------------------------------------------------------------
#if DBG != 1 && RETAILDEBUGLIB != 1
    #if defined(PERFMETER)
        #define DbgPreAlloc                 DbgExMtPreAlloc
        #define DbgPostAlloc                DbgExMtPostAlloc
        #define DbgPreFree                  DbgExMtPreFree
        #define DbgPostFree                 DbgExMtPostFree
        #define DbgPreRealloc               DbgExMtPreRealloc
        #define DbgPostRealloc              DbgExMtPostRealloc
        #define DbgPreGetSize               DbgExMtPreGetSize
        #define DbgPostGetSize              DbgExMtPostGetSize
    #else
        #define DbgPreAlloc(cb, mt)             cb
        #define DbgPostAlloc(pv)                pv
        #define DbgPreFree(pv)                  pv
        #define DbgPostFree()
        #define DbgPreRealloc(pv, cb, ppv, mt)  cb
        #define DbgPostRealloc(pv)              pv
        #define DbgPreGetSize(pv)               pv
        #define DbgPostGetSize(cb)              cb
    #endif
#else
    #define DbgPreAlloc                 DbgExPreAlloc
    #define DbgPostAlloc                DbgExPostAlloc
    #define DbgPreFree                  DbgExPreFree
    #define DbgPostFree                 DbgExPostFree
    #define DbgPreRealloc               DbgExPreRealloc
    #define DbgPostRealloc              DbgExPostRealloc
    #define DbgPreGetSize               DbgExPreGetSize
    #define DbgPostGetSize              DbgExPostGetSize
#endif

} // namespace WPF

// Inline definitions ----------------------------------------------------------
#include "MemUtils.inl"

