// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Contents:   Memory utilities
//------------------------------------------------------------------------------

#include "Pch.h"

// Globals ---------------------------------------------------------------------
HANDLE g_hProcessHeap;
#if PERFMETER
// Don't check meters while this is TRUE ---------------------------------------
BOOL    g_fNoMeterChecks = FALSE;
#endif


// Standard Top Level Meters ---------------------------------------------------
MtDefineF(PerfPigs, NULL, "Performance Pigs", METER_NO_MEMALLOC)
MtDefineF(Metrics, NULL, "Metrics", METER_NO_MEMALLOC)
MtDefineF(WorkingSet, NULL, "Working Set", METER_NO_MEMALLOC|METER_MT_VERIFIED)
MtDefineF(Mem, WorkingSet, "MemAlloc", METER_NO_MEMALLOC|METER_MT_VERIFIED)
MtDefineF(OpNew, Mem, "operator new", METER_NO_MEMALLOC)
MtDefine(Locals, Mem, "Per Function Local")
MtDefine(Utilities, Mem, "Utilities")


#if 0
// Debug support ---------------------------------------------------------------
#if DBG==1
DeclareTag(tagNoCheckHeap, "!Memory", "Disable CHECK_HEAP() macro");

void WINAPI
CheckHeap()
{
    if (!DbgExIsTagEnabled(tagNoCheckHeap))
    {
        AssertMsg(DbgExValidateKnownAllocations(), "Internal heap is corrupt!");
    }
}
#endif // DBG
#endif

namespace WPF
{

#if DBG==1
void MtValidateMeter(PERFMETERTAG mt);
#endif

#if defined(PERFMETER)
#define PERFMETERPARAM PERFMETERTAG mt,
#else
#define PERFMETERPARAM
#endif

//------------------------------------------------------------------------------
//-------------------------------- Process Heap --------------------------------
//------------------------------------------------------------------------------

class ProcessHeapImpl
    : public Heap
{
public:
    __allocator __bcount(cbSize) void *  Alloc(PERFMETERPARAM size_t cbSize) override;
    __allocator __bcount(cbSize) void *  AllocClear(PERFMETERPARAM size_t cbSize) override;
    __allocator HRESULT Realloc(PERFMETERPARAM __deref_bcount(cbSize) void ** ppv, size_t cbSize) override;
    void    Free(IN void * pv) override;

private:
    __allocator __forceinline __bcount(cbSize) static void * AllocImpl(PERFMETERPARAM size_t cbSize);
};

// Instantiation ---------------------------------------------------------------
ProcessHeapImpl * g_pProcessHeap;

} // namespace WPF

extern "C"
{

/***************************************************************************\
*
* AvCreateProcessHeap
*
* AvCreateProcessHeap() initializes the process "heap wrapper" to be used
* inside a given module.  Each process "heap wrapper" points to the common
* Win32 process heap, allowing memory to safely be transfered between
* modules, even after one module is unloaded.  Before this point, the
* process heap may not be used.
*
\***************************************************************************/

HRESULT AvCreateProcessHeap()
{
    AssertMsg(WPF::g_pProcessHeap == NULL, "Can only setup once");

    g_hProcessHeap = GetProcessHeap();
    WPF::g_pProcessHeap = reinterpret_cast<WPF::ProcessHeapImpl*>(
            HeapAlloc(g_hProcessHeap, 0, sizeof(WPF::ProcessHeapImpl)));
    if (WPF::g_pProcessHeap == NULL) {
        return E_OUTOFMEMORY;
    }

    new (WPF::g_pProcessHeap) WPF::ProcessHeapImpl();

    return S_OK;
}



/***************************************************************************\
*
* AvDestroyProcessHeap
*
* AvDestroyProcessHeap() cleans up the process "heap wrapper" previously
* created using AvCreateProcessHeap().  After this point, the process heap
* may not be used.
*
\***************************************************************************/

HRESULT AvDestroyProcessHeap()
{
    //
    // AvCreateProcessHeap can fail and we can still get the heap removed
    // during Dll shutdown.
    //
    if (NULL != WPF::g_pProcessHeap)
    {
        WPF::g_pProcessHeap->~ProcessHeapImpl();
        HeapFree(g_hProcessHeap, 0, WPF::g_pProcessHeap);
        WPF::g_pProcessHeap = NULL;

        g_hProcessHeap = NULL;
    }

    return S_OK;
}

} // extern "C"


namespace WPF
{

// Implementation --------------------------------------------------------------

__allocator __forceinline __bcount(cbSize) void *
ProcessHeapImpl::AllocImpl(PERFMETERPARAM size_t cbSize)
{
    Assert(g_hProcessHeap);

    void * pvRet;

    WHEN_DBG( MtValidateMeter(mt) );

#if defined(PERFMETER)
    if (MtSimulateOutOfMemory(mt, -1))
        return NULL;
#endif

    // Don't let zero sized allocations in.  This can expose
    // bugs in some heaps
    if (0 == cbSize)
    {
        cbSize = 1;
    }

    #pragma prefast (suppress: __WARNING_IDENTITY_ASSIGNMENT, "cbSize=cbSize when not meter - should optimize away")
    cbSize = DbgPreAlloc(cbSize, mt);

    // Make sure we still have a valid allocation after DbgPreAlloc.
    // DbgPreAlloc will return a size of zero if the allocation is too large
    // for current instrumentation to handle.  In such case the allocation is
    // likely to fail anyway.  This check should be optimized away in the
    // non-instrumented case because sizes of zero have already been promoted
    // to a size of 1.
    if (cbSize)
    {
        pvRet = DbgPostAlloc(HeapAlloc(g_hProcessHeap, 0, cbSize));
    }
    else
    {
        pvRet = NULL;
    }

    return pvRet;
}


__allocator __bcount(cbSize) void *
ProcessHeapImpl::Alloc(PERFMETERPARAM size_t cbSize)
{
#if DBG==1
    //
    // We shouldn't be doing zero sized allocations.  However, if we are
    // calling external code, they can allocate 0 bytes of memory and we do
    // round up to 1 below, so it's safe to ignore this assert in those cases.
    //
    // Note that checking for default meter is only valid for full debug
    // builds, so only do it in those csaes.
    //

    if (DbgExIsFullDebug() && DbgExMtGetDefaultMeter() == NULL)
    {
        AssertMsg(cbSize, "Requesting zero sized block");
    }
#endif

    return AllocImpl(
#if defined(PERFMETER)
        mt,
#endif
        cbSize
        );
}

__allocator __bcount(cbSize) void *
ProcessHeapImpl::AllocClear(PERFMETERPARAM size_t cbSize)
{
    void * pvRet = AllocImpl(
#if defined(PERFMETER)
        mt,
#endif
        cbSize
        );

    if (pvRet)
    {
        ZeroMemory(pvRet, cbSize);
    }

    return pvRet;
}

__allocator HRESULT
ProcessHeapImpl::Realloc(PERFMETERPARAM __deref_bcount(cbSize) void ** ppv, size_t cbSize)
{
    Assert(g_hProcessHeap);

    if (NULL == *ppv)
    {
        *ppv = ProcessHeapImpl::Alloc(
#if defined(PERFMETER)
                    mt,
#endif
                    cbSize);
        if (NULL == (*ppv))
            return E_OUTOFMEMORY;
    }
    else
    {
        void * pv = *ppv;
        size_t cbSizeToHeapRealloc = DbgPreRealloc(*ppv, cbSize, &pv, mt);

        // Make sure we still have a valid allocation after DbgPreRealloc.
        // DbgPreRealloc will return pv == NULL if the allocation is too large
        // for current instrumentation to handle.  In such case the allocation
        // is likely to fail anyway.  This check should be optimized away in
        // the non-instrumented case because pv won't be modified and we
        // already know it is non-NULL.
        if (pv == NULL)
        {
            // DbgPreRealloc failed - don't try actual HeapReAlloc
        }
#if defined(PERFMETER)
        else if (MtSimulateOutOfMemory(mt, -1))
        {
            pv = NULL;
        }
#endif
        else
        {
            pv = HeapReAlloc(g_hProcessHeap, 0, pv, cbSizeToHeapRealloc);
        }

        #pragma prefast (suppress: __WARNING_IDENTITY_ASSIGNMENT, "pv=pv when not meter - should optimize away")
        pv = DbgPostRealloc(pv);

        if (pv == NULL)
            return E_OUTOFMEMORY;

        *ppv = pv;
    }

    return S_OK;
}

void
ProcessHeapImpl::Free(IN void * pv)
{
    Assert(g_hProcessHeap);

    // The null check is required for HeapFree.
    if (pv == NULL)
        return;

    HeapFree(g_hProcessHeap, 0, DbgPreFree(pv));

    DbgPostFree();
}


// Meter Debug Support ---------------------------------------------------------

#if DBG==1

#define STACKKEYDEPTH 6

template <int _cMax>
class CDbgMeterStackArray
{
protected:
    int _cUsed;         // used entries
    int _rgcrc[_cMax];  // the array

    static int CrcAddInt(unsigned int crc, unsigned int i) { return (crc ^ i) * 16807 % 0x7fffffff; }
    int CrcFromPPV(void **ppv, int cpv)
    {
        int crc = 0;
        while (cpv-- > 0)
            crc = CrcAddInt(crc, (unsigned int)((UINT_PTR)*(ppv++)));
        return crc;
    }

public:
    BOOL IsMarked(void ** ppv, int cpv)
    {
        int crc = CrcFromPPV( ppv, cpv );

        for (int i = 0; i < _cUsed; i++)
        {
            if (_rgcrc[i] == crc)
                return TRUE;
        }
        return FALSE;
    }

    BOOL Mark(void ** ppv, int cpv)
    {
        if (_cUsed >= _cMax || IsMarked(ppv, cpv))
            return TRUE;

        _rgcrc[_cUsed++] = CrcFromPPV(ppv, cpv);

        return FALSE;
    }
};

CDbgMeterStackArray<1000> rgmtstkDisableMeterValidate;

void MtValidateMeter(PERFMETERTAG mt)
{
    if (!DbgExIsFullDebug() || g_fNoMeterChecks)
        goto Success;

    BOOL fLoopError = FALSE;

    // All memory meters must roll up to WorkingSet at least -- it
    // would be even better if the resolved to something that
    // actually made sense too!
    //
    // The METER_NO_MEMALLOC flag means that you can't allocate on this tag. If
    // METER_MT_VERIFIED is not set, then you can't allocate on children either.
    // If METER_MT_VERIFIED *is* set then you can allocate on children (if they
    // don't have METER_NO_MEMALLOC set).
    //
    // The METER_MT_VERIFIED flag stops the walk up the tree.  You can only
    // allocate if you or one of your parents has this flag set.

    // First check the incoming meter -- in this case only, METER_NO_MEMALLOC takes
    // precidence over METER_MT_VERIFIED
    if (DbgExMtGetFlags(mt) & METER_NO_MEMALLOC)
        goto Error;

    // No look up the parent chain.  For each parent, VERIFIED takes precidence
    // over NO_MEMALLOC
    {
        PERFMETERTAG mtVal = mt;

        // Adding mtValTrace to traverse the parent chain at double-rate
        // to detect cycles.
        PERFMETERTAG mtValTrace = mt;

        while(mtVal)
        {
            DWORD dwFlagsVal = DbgExMtGetFlags(mtVal);

            if (dwFlagsVal & METER_MT_VERIFIED)
                goto Success;

            if (dwFlagsVal & METER_NO_MEMALLOC)
                goto Error;

            mtVal = DbgExMtGetParent(mtVal);

            // This gets the "grandparent" of mtValTrace's value at the top of this while loop.
            // DbgExMtGetParent returns NULL on NULL input, so it's ok if this goes off into space
            // by hitting the end of an ancestor chain.
            mtValTrace = DbgExMtGetParent(mtValTrace);
            mtValTrace = DbgExMtGetParent(mtValTrace);

            // Check for a loop - this would be bad, as this while-loop would never terminate

            if (mtVal == mtValTrace)
            {
                fLoopError = TRUE;
                goto Error;
            }
        }
    }

Error:
    {
        void * pvStack[STACKKEYDEPTH];

        DbgExGetStackAddresses(pvStack, 2, STACKKEYDEPTH);

        if (rgmtstkDisableMeterValidate.IsMarked(pvStack, STACKKEYDEPTH))
            goto Success;

        TraceTag((tagError, "Allocation made without a valid meter tag.\nThis is just an informative error and will not be the source of real bugs."
                            "\nTag: %s", DbgExMtGetDesc(mt)));

        TraceCallers(tagError, 2, 12);

        //----------------------------------------------------------------------
        //  Note to users:
        //
        //  If you hit this assert then you are not playing nice in the meter
        //  system.  You should make sure that you are allocating on a meter and
        //  that the meter is an eventual child to the "Mem" meter.  Talk to
        //  JBeda with questions.
        //----------------------------------------------------------------------

        if (fLoopError)
        {
            RIP("Allocation made with a meter tag which is parented in such a way as to cause a cycle.  Check that this tag's owner isn't itself and is eventually is parented up to the heap.");
        }
        else
        {
            RIP("Allocation made without a valid meter tag.");
        }

        // Then ignore this stack from now on --
        // the dev has now seen enough to fix this problem
        // and we don't have to keep bugging him/her

        rgmtstkDisableMeterValidate.Mark(pvStack, STACKKEYDEPTH);
    }
    return;


Success:
    DbgExMtSetFlags( mt, DbgExMtGetFlags(mt) | METER_MT_VERIFIED );

    return;

}
#endif


/**************************************************************************\
*
* Function Description:
*
*   Allocate a block of memory. This function is designed to handle an
*   array-type allocation where there is some notion of a number of elements
*   of a given size. The routine specifically checks for multiplication
*   overflow (a common security bug with allocations).
*
* Arguments:
*
*   mt            - tag
*   cbElementSize - size of each element.
*   cElements     - number of elements.
*   ppvmemblock   - output allocated memory block. Must be address of
*                   NULL initialized pointer.
*
* Return:
*
*   HRESULT       - returns E_INVALIDARG for bad input, multiplication overflow,
*                   E_OUTOFMEMORY for allocation failure.
*
*
\**************************************************************************/

#undef HrMalloc
__checkReturn __allocator
HRESULT HrMalloc(
    PERFMETERTAG mt,
    size_t cbElementSize,
    size_t cElements,
    __deref_bcount(cbElementSize*cElements) void **ppvmemblock
    )
{
    // Callers must ensure a NULL initialized pointer location for the
    // output buffer.

    Assert(ppvmemblock);
    Assert(NULL == *ppvmemblock);
    Assert(cbElementSize > 0);
    Assert(cElements > 0);

    HRESULT hr = S_OK;

    // validate our input parameters.
    // make sure our size computation doesn't overflow.

    if (ppvmemblock == NULL ||
        cElements == 0 ||
        cbElementSize == 0 ||
        cbElementSize >= SIZE_T_MAX / cElements)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        size_t cbSize = cbElementSize * cElements;
        // cbElementSize and cElements are limited above, suppress prefast overflow warnings
        #pragma prefast(suppress: 37001 37002 37003)
        *ppvmemblock = Alloc(g_pProcessHeap, mt, cbSize);

        if (NULL == *ppvmemblock)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Allocate a block of memory.
*
* Arguments:
*
*   mt            - tag
*   cbSize        - size of the memory block.
*   ppvmemblock   - output allocated memory block. Must be address of
*                   NULL initialized pointer.
*
* Return:
*
*   HRESULT       - E_OUTOFMEMORY for allocation failure.
*
* Note:
*
*   If you find yourself writing HrAlloc( ... , a * b, &p), call
*   HrMalloc (above) to properly check for multiplication overflow.
*
*
\**************************************************************************/

#undef HrAlloc
__checkReturn __allocator
HRESULT HrAlloc(
    PERFMETERTAG mt,
    size_t cbSize,
    __deref_bcount(cbSize) void **ppvmemblock
    )
{
    // Callers must ensure a NULL initialized pointer location for the
    // output buffer.

    Assert(ppvmemblock);
    Assert(NULL == *ppvmemblock);
    Assert(cbSize > 0);

    HRESULT hr = S_OK;

    if (ppvmemblock == NULL || cbSize == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppvmemblock = Alloc(g_pProcessHeap, mt, cbSize);

        if (NULL == *ppvmemblock)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/**************************************************************************\
*
* Function Description:
*
*   Allocate an aligned block of memory. This function is designed to handle an
*   array-type allocation where there is some notion of a number of elements
*   of a given size. The routine specifically checks for multiplication
*   overflow (a common security bug with allocations).
*
*   This routine is primarily used to allocate working buffers for SSE/SSE2
*   routines which require 128-byte aligned memory.
*
* Further Commentary Intel [Mark J Buxton]:
*
*   To avoid the alignment exception, we need 16 byte alignment. However, there
*   is some advantage to minimizing the number of accessed cachelines, so it
*   helps to have the start of the buffer cacheline aligned (64 byte).
*   Intel (via the he P4 optimization guide) recommends 128 byte alignement,
*   because we effectively have 128-byte L2 cachelines (even though the cache
*   coherency size is 64 bytes).
*
*   Second, there are modes where it's very helpful to be cacheline aligned -
*   such as when using streaming writes. In this case not having partial
*   cachlines will make any streaming stores _much_ faster by avoiding partial
*   writes.
*
*   For general purpose code (such as the code allocating the dest buffers for
*   the Fant Scaler, which uses streaming stores on the output), 128-byte
*   alignment is the optimal size.
*
* Arguments:
*
*   mt                 - tag
*   cbElementSize      - size of each element.
*   cElements          - number of elements.
*   ppvmemblockAligned - pointer to 128 byte aligned location within the memory
*                        block. DO NOT FREE THIS POINTER.
*   ppvmemblock        - output allocated memory block. Must be address of
*                        NULL initialized pointer. This pointer is the one
*                        which should be passed to GpFree.
*
* Return:
*
*   HRESULT       - returns E_INVALIDARG for bad input, multiplication overflow,
*                   E_OUTOFMEMORY for allocation failure.
*
* Note:
*
*   Do not free the aligned output pointer. Free the base memory block instead
*   (ppvmemblock).
*
*
\**************************************************************************/

#undef HrMallocAlign
__checkReturn __allocator
HRESULT HrMallocAlign(
    PERFMETERTAG mt,
    size_t cbElementSize,
    size_t cElements,
    __deref_bcount(cbElementSize*cElements) void **ppvmemblockAligned,
    __deref_bcount(cbElementSize*cElements + 128) void **ppvmemblock
    )
{
    // Callers must ensure a NULL initialized pointer location for the
    // output buffer.

    Assert(ppvmemblock);
    Assert(NULL == *ppvmemblock);
    Assert(NULL == *ppvmemblockAligned);
    Assert(cbElementSize > 0);
    Assert(cElements > 0);

    HRESULT hr = S_OK;

    // validate our input parameters.
    // make sure our size computation doesn't overflow.

    if (ppvmemblock == NULL ||
        ppvmemblockAligned == NULL ||
        cElements == 0 ||
        cbElementSize == 0 ||
        cbElementSize >= (SIZE_T_MAX - 128) / cElements)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        size_t cbSize = cbElementSize * cElements + 128;
        // cbElementSize and cElements are limited above, suppress prefast overflow warnings
        #pragma prefast(suppress: 37001 37002 37003)
        *ppvmemblock = Alloc(g_pProcessHeap, mt, cbSize);

        if (NULL == *ppvmemblock)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            *ppvmemblockAligned = (void*)((((UINT_PTR)*ppvmemblock)+127)&~127);
        }
    }

    return hr;
}

} // namespace WPF

//------------------------------------------------------------------------------
// new/delete that asserts if info isn't carried through - - - - - - - - - - -
// NOTE:    We may want to do something here with a default meter/heap so that
//          we can integrate with third party .libs (like D3DX) when they link
//          to our operator new.  Indications are that this will be a problem
//          with GDI+ too.
#if defined(PERFMETER)
_Ret_notnull_ _Post_writable_byte_size_(cbSize)
__allocator inline __bcount(cbSize) void * __cdecl
operator new(size_t cbSize)
{
    return WPF::UseOperatorNewWithMemoryMeterInstead(cbSize);
}
_Ret_notnull_
_Post_writable_byte_size_(cbSize)
__allocator inline __bcount(cbSize) void * __cdecl
operator new[](size_t cbSize)
{
    return WPF::UseOperatorNewWithMemoryMeterInstead(cbSize);
}

void  __cdecl
operator delete(void * pv)
{
    WPFFree(ProcessHeap, pv);
}

void  __cdecl
operator delete[](void * pv)
{
    WPFFree(ProcessHeap, pv);
}
#else
__allocator inline __bcount(cbSize) void * __cdecl
operator new(size_t cbSize)
{
    return WPFAlloc(ProcessHeap, Mt(OpNew), cbSize);
}

__allocator inline __bcount(cbSize) void * __cdecl
operator new[](size_t cbSize)
{
    return WPFAlloc(ProcessHeap, Mt(OpNew), cbSize);
}

void  __cdecl
operator delete(void * pv)
{
    WPFFree(ProcessHeap, pv);
}

void  __cdecl
operator delete[](void * pv)
{
    WPFFree(ProcessHeap, pv);
}
#endif


