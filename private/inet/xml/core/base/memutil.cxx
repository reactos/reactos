/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
//+------------------------------------------------------------------------
//
//  File:       memutil.cxx
//
//  Contents:   Memory utilities
//
//  History:    30-Oct-94   GaryBu  Consolidated from places far & wide.
//              06-Jul-95   PaulG   Macintosh compiles now use Global instead
//                                  of Heap memory functions
//
//-------------------------------------------------------------------------

#include "core.hxx"
#pragma hdrstop

#if DBG == 1
DWORD g_dwFALSE = 0;
#endif

#define MULTITHREADED 1

//define MEMSTRESS_ENABLE 1

#undef SMALLBLOCKHEAP

#ifndef MSXMLPRFCOUNTERS_H
#include "core/prfdata/msxmlprfcounters.h"  // PrfHeapAlloc, PrfHeapFree
#endif

#if MPHEAP
#include "core/base/mpheap.hxx"
extern HANDLE g_hMpHeap;
#else
#define SMALLBLOCKHEAP 1
#endif

#if MULTITHREADED
#define ENTERCRITICALSECTION(CS) EnterCriticalSection(CS)
#define LEAVECRITICALSECTION(CS) LeaveCriticalSection(CS)
#else
#define ENTERCRITICALSECTION(CS)
#define LEAVECRITICALSECTION(CS) 
#endif

#if DBG == 1
DeclareTag(tagDebugMemory, "Memory", "Enable heap checking");
#endif
#if DBG == 1 || defined(MEMSTRESS_ENABLE)
// turn on OutOfMemory emulation by setting the threshold above 0 
static unsigned long ulMemAllocFailThreshold = 0;
static unsigned long ulMemAllocFailCount = 1;
#endif

#if DBG==1 && !defined(_WIN64) && (defined(_X86_) || defined(_PPC_) || defined(_MIPS_) || defined(_ALPHA_) || defined(_M_SH))
#   define VIRTUALMEM
#endif

//+------------------------------------------------------------------------
// Allocation functions not implemented in this file:
//
//      CDUTIL.HXX
//          operator new
//          operator delete
//
//      OLE's OBJBASE.H
//          CoTaskMemAlloc, CoTaskMemFree
//
//-------------------------------------------------------------------------

#if SMALLBLOCKHEAP

#define _CRTBLD 1
#include "../crt/winheap.h"

CRITICAL_SECTION g_csHeap;
__sbh_heap_t * g_psbh_heap;

DeclareTag(tagSmallBlockHeap, "!Memory", "Check small block heap every time")
DeclareTag(tagSmallBlockHeapDisable, "!Memory", "Disable small block heap");

#if DBG == 1
#define CHECKSBH if (IsTagEnabled(tagSmallBlockHeap)) {Assert(CheckSmallBlockHeap() && "Small block heap corrupt");};
BOOL __stdcall IsSmallBlockHeapEnabled()
{
    return TRUE;
/*
    static int g_fSmallBlockHeap = -1;
    if (g_fSmallBlockHeap == -1)
        g_fSmallBlockHeap = IsTagEnabled(tagSmallBlockHeapDisable) ? 0 : 1;
    return(g_fSmallBlockHeap == 1);
*/
}
BOOL __stdcall CheckSmallBlockHeap()
{
#if MPHEAP
    return MpHeapValidate(g_hMpHeap, NULL);
#else
    if (IsSmallBlockHeapEnabled())
    {
        ENTERCRITICALSECTION(&g_csHeap);
        BOOL f = __sbh_heap_check(g_psbh_heap) >= 0;
        LEAVECRITICALSECTION(&g_csHeap);
        return f;
    }
    return TRUE;
#endif
}
#else
#define CHECKSBH
#endif

#else

#if DBG == 1
BOOL __stdcall CheckSmallBlockHeap()
{
    return TRUE;
}
#endif

#endif SMALLBLOCKHEAP

/**
 * Memory manager initialization and exit
 */

HANDLE g_hProcessHeap;
#if MPHEAP
extern BOOL MpHeapInit();
extern void MpHeapExit();
#endif
extern void TlsInit();
extern void TlsExit();
extern BOOL GCInit();
extern void GCExit();

EXTERN_C BOOL 
Memory_init()
{
    g_hProcessHeap = GetProcessHeap();
    if (g_hProcessHeap == NULL)
        return FALSE;
    
    if (!GCInit())
        return FALSE;

    // init TLS storage manager
    TlsInit();

#if MPHEAP
    if (!MpHeapInit())
        return FALSE;
#else

#if SMALLBLOCKHEAP
    InitializeCriticalSection(&g_csHeap);

    g_psbh_heap = __sbh_create_heap();
    if (!g_psbh_heap)
        return FALSE;

    _set_sbh_heap_threshold(g_psbh_heap, _DEFAULT_THRESHOLD);
#endif

#endif

    // make sure TLS is allocated for this thread
    EnsureTlsData();

#if DBG == 1
    {
        char achMem[32];
        WCHAR awchMem[32];

        if (GetEnvironmentVariableA("MSXML_MEMORYFAILTHRESHOLD", achMem, LENGTH(achMem)))
        {
            MultiByteToWideChar(CP_ACP, 0, achMem, -1, awchMem, LENGTH(awchMem));
            ulMemAllocFailThreshold = _ttoi(awchMem);
        }
        if (GetEnvironmentVariableA("MSXML_MEMORYFAILCOUNT", achMem, LENGTH(achMem)))
        {
            MultiByteToWideChar(CP_ACP, 0, achMem, -1, awchMem, LENGTH(awchMem));
            ulMemAllocFailCount = _ttoi(awchMem);
        }
    }
#endif

    return TRUE;
}

extern TLSDATA * g_ptlsdata;
#if DBG == 1
extern BOOL g_fMemoryLeak;
#endif

EXTERN_C void 
Memory_exit()
{
    // release all exception objects here in case Runtime_exit was not called (CTRL-C)
    TLSDATA * ptlsdata;
    for (ptlsdata = g_ptlsdata; ptlsdata; ptlsdata = ptlsdata->_pNext)
    {
        release(&ptlsdata->_pException);
    }

#if SMALLBLOCKHEAP
    if (g_psbh_heap)
        __sbh_destroy_heap(g_psbh_heap);
    DeleteCriticalSection(&g_csHeap);
#endif

#if MPHEAP
#if DBG == 1
    MpHeapValidate(g_hMpHeap, NULL);
#endif
    // clear memory manager
    MpHeapExit();
#endif

    // clear pointer cache
    ClearPointerCache();

    // free TLS manager
    TlsExit();

    GCExit();
}


//+------------------------------------------------------------------------
// Strict virtual memory allocation functions
//-------------------------------------------------------------------------

#ifdef VIRTUALMEM

#ifdef _ALPHA_
#define PAGE_SIZE       8192
#else
#define PAGE_SIZE        4096
#endif

// WIN64 - this is  0xFFFFFFFFFFFF0000
// WIN32 - this is	        0xFFFF0000
#define PvToVMBase(pv)    ((void *)((ULONG_PTR)pv & ~(2 << 16 - 1)))

static BOOL g_fVMInitialized = FALSE;
static BOOL g_fVMEnabled     = FALSE;
static BOOL g_cbAlign        = 1;

BOOL __stdcall IsVirtualEnabled()
{
    if (!g_fVMInitialized)
    {
        g_fVMEnabled = IsTagEnabled(tagMemoryStrict);

//        if (IsTagEnabled(tagMemoryStrictAligned))
//            g_cbAlign = 8;

        g_fVMInitialized = TRUE;
    }

    return(g_fVMEnabled);
}

BOOL VMValidatePv(void *pv)
{
    void *    pvBase = PvToVMBase(pv);
    BYTE *    pb;

    pb = (BYTE *)pvBase + sizeof(ULONG);

    while (pb < (BYTE *)pv)
    {
        if (*pb++ != 0xAD)
        {
            AssertSz(FALSE, "Block leader has been overwritten");
            return(FALSE);
        }
    }

    if (g_cbAlign != 1)
    {
        ULONG cb = *((ULONG *)pvBase);
        ULONG cbPad = 0;

        if (cb % g_cbAlign)
            cbPad = (g_cbAlign - (cb % g_cbAlign));

        if (cbPad)
        {
            BYTE *pbMac;

            pb = (BYTE *)pv + cb;
            pbMac = pb + cbPad;

            while (pb < pbMac)
            {
                if (*pb++ != 0xBC)
                {
                    AssertSz(FALSE, "Block trailer has been overwritten");
                    return(FALSE);
                }
            }
        }
    }

    return(TRUE);
}

void * VMAlloc(ULONG cb)
{
    ULONG    cbAlloc;
    void *    pvR;
    void *    pvC;
    ULONG     cbPad    = 0;

    if (cb > 0x100000)
        return(0);

    if (cb % g_cbAlign)
        cbPad = (g_cbAlign - (cb % g_cbAlign));

    cbAlloc    = sizeof(ULONG) + cb + cbPad + PAGE_SIZE - 1;
    cbAlloc -= cbAlloc % PAGE_SIZE;
    cbAlloc    += PAGE_SIZE;

    pvR = VirtualAlloc(0, cbAlloc, MEM_RESERVE, PAGE_NOACCESS);

    if (pvR == 0)
        return(0);

    pvC = VirtualAlloc(pvR, cbAlloc - PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);

    if (pvC != pvR)
    {
        VirtualFree(pvR, 0, MEM_RELEASE);
        return(0);
    }

    *(ULONG *)pvC = cb;

    memset((BYTE *)pvC + sizeof(ULONG), 0xAD,
        (UINT) cbAlloc - cb - cbPad - sizeof(ULONG) - PAGE_SIZE);

    if (cbPad)
        memset((BYTE *)pvC + cbAlloc - PAGE_SIZE - cbPad, 0xBC,
            (UINT) cbPad);

    return((BYTE *)pvC + (cbAlloc - cb - cbPad - PAGE_SIZE));
}

void * VMAllocClear(ULONG cb)
{
    void *pv = VMAlloc(cb);

    if (pv)
    {
        memset(pv, 0, cb);
    }

    return(pv);
}

void VMFree(void *pv)
{
    VMValidatePv(pv);
    Verify(VirtualFree(PvToVMBase(pv), 0, MEM_RELEASE));
}

HRESULT VMRealloc(void **ppv, ULONG cb)
{
    void *  pvOld  = *ppv;
    void *    pvNew  = 0;
    ULONG    cbCopy = 0;

    if (pvOld)
    {
        VMValidatePv(pvOld);
        cbCopy = *(ULONG *)PvToVMBase(pvOld);
        if (cbCopy > cb)
            cbCopy = cb;
    }

    if (cb)
    {
        pvNew = VMAlloc(cb);

        if (pvNew == 0)
            return(E_OUTOFMEMORY);

        if (cbCopy)
        {
            memcpy(pvNew, pvOld, cbCopy);
        }
    }

    if (pvOld)
    {
        VMFree(pvOld);
    }

    *ppv = pvNew;
    return(S_OK);
}

ULONG VMGetSize(void *pv)
{
    VMValidatePv(pv);
    return(*(ULONG *)PvToVMBase(pv));
}

#endif


#if DBG==1
#ifndef MEMSTRESS_ENABLE
#define MEMSTRESS_ENABLE 1
#endif // MEMSTRESS_ENABLE
#endif // DBG

#ifdef MEMSTRESS_ENABLE

/* MemAlloc Fail implementation

   This code is used to help simulate out of memory situations for stress testing
   It uses it's own pseudo-random number generator to isolate it from other code which
   might use the clib's rand/srand methods.
*/

/* Linear Congruential Method, the "minimal standard generator"
   Park & Miller, 1988, Comm of the ACM, 31(10), pp. 1192-1201

   Note (derekdb): we use this so that we are guaranteed that
   no other portion of the program resets our seed
*/
#ifndef LONG_MAX
#define LONG_MAX 2147483647
#endif

static const long lMemAllocFailQuotient  = LONG_MAX / 16807L;
static const long lMemAllocFailRemainder = LONG_MAX % 16807L;

static long lMemAllocFailSeed = 1L;

unsigned long MemAllocFail_randlcg()       /* returns a random unsigned integer */
{
    if ( lMemAllocFailSeed <= lMemAllocFailQuotient )
    {
        lMemAllocFailSeed = (lMemAllocFailSeed * 16807L) % LONG_MAX;
    }
    else
    {
        long high_part = lMemAllocFailSeed / lMemAllocFailQuotient;
        long low_part  = lMemAllocFailSeed % lMemAllocFailQuotient;
        long test = 16807L * low_part - lMemAllocFailRemainder * high_part;
        if ( test > 0 )
            lMemAllocFailSeed = test;
        else
            lMemAllocFailSeed = test + LONG_MAX;
    }

    return lMemAllocFailSeed;
}

unsigned long ulMemAllocFailDisable = 0;

/* This is the external method exposed to seed the random number generator */
void SeedMemAllocFail( long nSeed, unsigned long ulThresh)
{
    lMemAllocFailSeed = nSeed;
    ulMemAllocFailThreshold = ulThresh; // eg, if == 10 then fail 10 out of every 2^32 allocs 
}

bool MemAllocFailTest()
{
    if (ulMemAllocFailCount)
        ulMemAllocFailCount--;
    if (ulMemAllocFailThreshold && !ulMemAllocFailDisable && !ulMemAllocFailCount &&
        MemAllocFail_randlcg() < ulMemAllocFailThreshold)
        return true;
    return false;
}
       
#endif // DBG (for MemAllocFail funcs

DeclareTag(tagMemAlloc, "MemAlloc", "Memory Allocate/Free");

//+------------------------------------------------------------------------
//
//  Function:   MemAllocNe
//
//  Synopsis:   Allocate block of memory, no exception thrown.
//
//              The contents of the block are undefined.  If the requested size
//              is zero, this function returns a valid pointer.  The returned
//              pointer is guaranteed to be suitably aligned for storage of any
//              object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void *
MemAllocNe(size_t cb)
{
#ifdef MEMSTRESS_ENABLE
    if (MemAllocFailTest())
        return NULL;
#endif

    CHECK_HEAP();
    AssertSz (cb, "Requesting zero sized block.");

#ifdef VIRTUALMEM
    if (IsVirtualEnabled())
        return(VMAlloc(cb));
#endif

    Assert(g_hProcessHeap);

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        /* round up to the nearest paragraph */
        if (!cb)
            cb = 1;
        size_t cbr = (DbgPreAlloc(cb) + _PARASIZE - 1) & ~(_PARASIZE - 1);

        if (cbr < _DEFAULT_THRESHOLD)
        {
            CHECKSBH;
            ENTERCRITICALSECTION(&g_csHeap);
            void * pv = DbgPostAlloc(__sbh_alloc_block(g_psbh_heap, cbr >> _PARASHIFT));
            LEAVECRITICALSECTION(&g_csHeap);
            if (pv)
                return pv;
        }
    }
#endif

#if MPHEAP
    void* pv = DbgPostAlloc(MpHeapAlloc(g_hMpHeap, 0, DbgPreAlloc(cb)));
#else
    void* pv = DbgPostAlloc(PrfHeapAlloc(g_hProcessHeap, 0, DbgPreAlloc(cb)));
#endif

    TraceTag((tagMemAlloc, "%p Alloc [%X]", pv, cb));

    return pv;
}

//+------------------------------------------------------------------------
//
//  Function:   MemAlloc
//
//  Synopsis:   Allocate block of memory, throw exception if failed.
//
//              The contents of the block are undefined.  If the requested size
//              is zero, this function returns a valid pointer.  The returned
//              pointer is guaranteed to be suitably aligned for storage of any
//              object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void *
MemAlloc(size_t cb)
{
    void * pv = MemAllocNe(cb);
    if (pv == null)
        Exception::throwEOutOfMemory();
    return pv;
}


//+------------------------------------------------------------------------
//  Function:   MemAllocClear
//
//  Synopsis:   Allocate a zero filled block of memory, throw exception if failed
//
//              If the requested size is zero, this function returns a valid
//              pointer. The returned pointer is guaranteed to be suitably
//              aligned for storage of any object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void *
MemAllocClear(size_t cb)
{
#ifdef MEMSTRESS_ENABLE
    if (MemAllocFailTest())
        Exception::throwEOutOfMemory();
#endif

    CHECK_HEAP();
    AssertSz (cb, "Allocating zero sized block.");

#ifdef VIRTUALMEM
    if (IsVirtualEnabled())
        return(VMAllocClear(cb));
#endif
    void * pv;

    Assert(g_hProcessHeap);

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        /* round up to the nearest paragraph */
        if (!cb)
            cb = 1;
        size_t cbr = (DbgPreAlloc(cb) + _PARASIZE - 1) & ~(_PARASIZE - 1);

        if (cbr < _DEFAULT_THRESHOLD)
        {
            CHECKSBH;
            ENTERCRITICALSECTION(&g_csHeap);
            pv = DbgPostAlloc(__sbh_alloc_block(g_psbh_heap, cbr >> _PARASHIFT));
            LEAVECRITICALSECTION(&g_csHeap);
            if (pv)
            {
                memset(pv, 0, cb);
                return pv;
            }
        }
    }
#endif

#if MPHEAP
    pv = DbgPostAlloc(MpHeapAlloc(g_hMpHeap, HEAP_ZERO_MEMORY,
                    DbgPreAlloc(cb)));
#else
    pv = DbgPostAlloc(PrfHeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY,
                    DbgPreAlloc(cb)));
#endif

    // In debug, DbgPostAlloc set the memory so we need to clear it again.
    // On the Mac, HpAlloc doesn't support HEAP_ZERO_MEMORY.

#if DBG==1 || defined(_MAC)
    if (pv)
    {
        memset(pv, 0, cb);
    }
#endif

    TraceTag((tagMemAlloc, "%p AllocClear [%X]", pv, cb));

    if (pv == null)
        Exception::throwEOutOfMemory();
    return pv;
}


//+------------------------------------------------------------------------
//  Function:   MemAllocObject
//
//  Synopsis:   Allocate a memory for an object, throw exception if failed
//
//              If the requested size is zero, this function returns a valid
//              pointer. The returned pointer is guaranteed to be suitably
//              aligned for storage of any object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void *
MemAllocObject(size_t cb)
{
    CHECK_HEAP();

#ifdef SPECIAL_OBJECT_ALLOCATION

#ifdef MEMSTRESS_ENABLE
    if (MemAllocFailTest())
        Exception::throwEOutOfMemory();
#endif

    CHECK_HEAP();
    AssertSz (cb, "Allocating zero sized block.");

#ifdef VIRTUALMEM
    if (IsVirtualEnabled())
        return(VMAllocClear(cb));
#endif
    void * pv;

    Assert(g_hProcessHeap);

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        /* round up to the nearest paragraph */
        if (!cb)
            cb = 1;
        size_t cbr = (DbgPreAlloc(cb) + _PARASIZE - 1) & ~(_PARASIZE - 1);

        if (cbr < _DEFAULT_THRESHOLD)
        {
            CHECKSBH;
            ENTERCRITICALSECTION(&g_csHeap);
            pv = DbgPostAlloc(__sbh_alloc_block(g_psbh_heap, cbr >> _PARASHIFT));
            LEAVECRITICALSECTION(&g_csHeap);
            if (pv)
            {
                memset(pv, 0, cb);
                return pv;
            }
        }
    }
#endif

#if MPHEAP
    pv = DbgPostAlloc(MpHeapAlloc(g_hMpHeap, MPHEAP_ZERO_MEMORY | MPHEAP_OBJECT,
                    DbgPreAlloc(cb)));
#else
    pv = DbgPostAlloc(PrfHeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY,
                    DbgPreAlloc(cb)));
#endif

    // In debug, DbgPostAlloc set the memory so we need to clear it again.
    // On the Mac, HpAlloc doesn't support HEAP_ZERO_MEMORY.

#if DBG==1 || defined(_MAC)
    if (pv)
    {
        memset(pv, 0, cb);
    }
#endif

    if (pv == null)
        Exception::throwEOutOfMemory();

#else // SPECIAL_OBJECT_ALLOCATION

    void * pv = MemAllocClear(cb);

#endif // SPECIAL_OBJECT_ALLOCATION

    // start refcount locked !
    TLSDATA * ptlsdata = GetTlsData();
    ((Base *)pv)->_refs = (ptlsdata->_reModel == Rental) ? REF_RENTAL 
                                                         : REF_LOCKED; 
#if DBG == 1
    ((Base *)pv)->_dwTID = GetTlsData()->_dwTID;
#endif

    Base::s_lcbAllocated += cb;
    TraceTag((tagMemAlloc, "%p AllocObject [%X]", pv, cb));
    

#ifdef RENTAL_MODEL
    Assert(((UINT_PTR)pv & REF_RENTAL) == 0);
#endif

    return pv;
}

//+------------------------------------------------------------------------
//
//  Function:   MemFree
//
//  Synopsis:   Free a block of memory allocated with MemAlloc,
//              MemAllocFree or MemRealloc.
//
//  Arguments:  [pv] - Pointer to block to free.  A value of zero is
//              is ignored.
//
//-------------------------------------------------------------------------

void
MemFree(void *pv)
{
    TraceTag((tagMemAlloc, "%p Free", pv));
    CHECK_HEAP();
    // The null check is required for HeapFree.
    if (pv == NULL)
        return;

#ifdef VIRTUALMEM
    if (IsVirtualEnabled())
    {
        VMFree(pv);
        return;
    }
#endif

    Assert(g_hProcessHeap);

#if DBG == 1
    pv = DbgPreFree(pv);
#endif

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        __sbh_region_t *preg;
        __sbh_page_t *  ppage;
        __page_map_t *  pmap;

        CHECKSBH;
        ENTERCRITICALSECTION(&g_csHeap);
        if ( (pmap = __sbh_find_block(g_psbh_heap, pv, &preg, &ppage)) != NULL ) {
            __sbh_free_block(g_psbh_heap, preg, ppage, pmap);
            LEAVECRITICALSECTION(&g_csHeap);
            DbgPostFree();
            return;
        }
        LEAVECRITICALSECTION(&g_csHeap);
    }
#endif

#if MPHEAP
    MpHeapFree(g_hMpHeap, pv);
#else
    PrfHeapFree(g_hProcessHeap, 0, pv);
#endif
    DbgPostFree();
}

#ifdef DEBUG_OLEAUT

#undef SysAllocString
#undef SysAllocStringLen
#undef SysFreeString
#undef SysStringLen
#undef VariantClear

HANDLE hFile = NULL;

#include "core/util/chartype.hxx" // s_ByteOrderMark

void Log(WCHAR* type, const OLECHAR* address, UINT len)
{
    DWORD written;
    const WCHAR* spacer = L"\t";
    const WCHAR* newline = L"\r\n";
    WCHAR buf[80];

    if (hFile == NULL)
    {
        hFile = CreateFileA(
              "c:\\temp\\ole.log",  // LPCTSTR lpFileName,          // pointer to name of the file
              GENERIC_WRITE,        //DWORD dwDesiredAccess,       // access (read-write) mode
              FILE_SHARE_READ,      // DWORD dwShareMode,           // share mode
              NULL,                 // LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                           // pointer to security attributes
              CREATE_ALWAYS,        // DWORD dwCreationDisposition,  // how to create
              FILE_ATTRIBUTE_ARCHIVE, // DWORD dwFlagsAndAttributes,  // file attributes
              NULL                 // HANDLE hTemplateFile         // handle to file with attributes to 
                                           // copy
            ); 

        WriteFile(hFile, s_ByteOrderMark, sizeof(s_ByteOrderMark), &written, NULL);
    }

    WriteFile(hFile, type, _tcslen(type)*sizeof(WCHAR), &written, NULL);
    WriteFile(hFile, spacer, _tcslen(spacer)*sizeof(WCHAR), &written, NULL);
        
    _ltow((long)address, buf, 16);
    WriteFile(hFile, buf, _tcslen(buf)*sizeof(WCHAR), &written, NULL);

    WriteFile(hFile, spacer, _tcslen(spacer)*sizeof(WCHAR), &written, NULL);

    _ltow(len, buf, 16);
    WriteFile(hFile, buf, _tcslen(buf)*sizeof(WCHAR), &written, NULL);

    WriteFile(hFile, spacer, _tcslen(spacer)*sizeof(WCHAR), &written, NULL);

    if (address != null && len > 0)
    {
        WriteFile(hFile, address, len*sizeof(WCHAR), &written, NULL);
    }

    WriteFile(hFile, newline, _tcslen(newline)*sizeof(WCHAR), &written, NULL);

}

BSTR DebugSysAllocString(const OLECHAR *psz)
{
    Log(L"SysAllocString", psz, psz == NULL ? 0 : _tcslen(psz));
    return SysAllocString(psz);
}

BSTR DebugSysAllocStringLen(const OLECHAR *pch, UINT len)
{
    Log(L"SysAllocStringLen", pch, len);
    return SysAllocStringLen(pch,len);
}

void DebugSysFreeString(BSTR bstr)
{
    Log(L"SysFreeString", bstr, bstr == NULL ? 0 : _tcslen(bstr));
    ::SysFreeString(bstr);
}

UINT DebugSysStringLen(BSTR bstr)
{
    Log(L"SysStringLen", bstr, bstr == NULL ? 0 : _tcslen(bstr));
    return ::SysStringLen(bstr);
}

HRESULT DebugVariantClear(VARIANTARG * pvarg)
{
    if (pvarg->vt == VT_BSTR)
    {
        BSTR bstr = V_BSTR(pvarg);
        Log(L"VariantClear", bstr, bstr == NULL ? 0 : _tcslen(bstr));
    }
    return ::VariantClear(pvarg);
}

#endif
