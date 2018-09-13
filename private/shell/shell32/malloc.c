#include "shellprv.h"
#pragma  hdrstop

extern const IMallocVtbl c_CShellMallocVtbl;

const IMalloc c_mem = { &c_CShellMallocVtbl };

STDMETHODIMP CShellMalloc_QueryInterface(IMalloc *pmem, REFIID riid, LPVOID * ppvObj)
{
    ASSERT(pmem == &c_mem);
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMalloc))
    {
        *ppvObj = pmem;
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellMalloc_AddRefRelease(IMalloc *pmem)
{
    ASSERT(pmem == &c_mem);
    return 1; // static object
}

STDMETHODIMP_(void *) CShellMalloc_Alloc(IMalloc *pmem, SIZE_T cb)
{
    ASSERT(pmem == &c_mem);
    return (void*)LocalAlloc(LPTR, cb);
}

//
//  IMalloc::Realloc is slightly different from LocalRealloc.
//
//  IMalloc::Realloc(NULL, 0) = return NULL
//  IMalloc::Realloc(pv, 0) = IMalloc::Free(pv)
//  IMalloc::Realloc(NULL, cb) = IMalloc::Alloc(cb)
//  IMalloc::Realloc(pv, cb) = LocalRealloc()
//
STDMETHODIMP_(void *) CShellMalloc_Realloc(IMalloc *pmem, void *pv, SIZE_T cb)
{
    ASSERT(pmem == &c_mem);

    if (cb == 0)
    {
        if (pv) LocalFree(pv);
        return NULL;
    }
    else if (pv == NULL)
    {
        return LocalAlloc(LPTR, cb);
    }
    else
        return LocalReAlloc(pv, cb, LMEM_MOVEABLE|LMEM_ZEROINIT);
}

//
//  IMalloc::Free is slightly different from LocalFree.
//
//  IMalloc::Free(NULL) - nop
//  IMalloc::Free(pv)   - LocalFree()
//
STDMETHODIMP_(void) CShellMalloc_Free(IMalloc *pmem, void *pv)
{
    ASSERT(pmem == &c_mem);
    if (pv) LocalFree(pv);
}

STDMETHODIMP_(SIZE_T) CShellMalloc_GetSize(IMalloc *pmem, void *pv)
{
    ASSERT(pmem == &c_mem);
    return LocalSize(pv);
}

STDMETHODIMP_(int) CShellMalloc_DidAlloc(IMalloc *pmem, void *pv)
{
    ASSERT(pmem == &c_mem);
    return -1;  // don't know
}

STDMETHODIMP_(void) CShellMalloc_HeapMinimize(IMalloc *pmem)
{
    ASSERT(pmem == &c_mem);
}

const IMallocVtbl c_CShellMallocVtbl = {
    CShellMalloc_QueryInterface, CShellMalloc_AddRefRelease, CShellMalloc_AddRefRelease,
    CShellMalloc_Alloc,
    CShellMalloc_Realloc,
    CShellMalloc_Free,
    CShellMalloc_GetSize,
    CShellMalloc_DidAlloc,
    CShellMalloc_HeapMinimize,
};


typedef HRESULT (STDAPICALLTYPE * LPFNCOGETMALLOC)(DWORD dwMemContext, IMalloc **ppmem);

IMalloc *g_pmemTask = NULL;     // No default task allocator.

#ifdef DEBUG
extern void WINAPI DbgRegisterMallocSpy();
#endif

// on DEBUG builds, mostly for NT, we force using OLE's task allocator at all times.
// for retail we only use OLE if ole32 is already loaded in this process.
//
// this is because OLE's DEBUG allocator will complain if we pass it LocalAlloc()ed
// memory. this can happen if we start up without OLE loaded, then delay load it.
// retail OLE uses LocalAlloc() so we can use our own allocator and switch
// on the fly with no complains from OLE in retail. a common case here would be
// using the file dialogs with notepad

void _GetTaskAllocator(IMalloc **ppmem)
{
    if (g_pmemTask == NULL)
    {
#ifndef DEBUG
        if (GetModuleHandle(TEXT("OLE32.DLL"))) // retail
#endif
        {
            CoGetMalloc(MEMCTX_TASK, &g_pmemTask);
            ASSERT(g_pmemTask);
#ifdef DEBUG
            {
                HANDLE hmodSpy = LoadLibrary(TEXT("ALLOCSPY.DLL"));
                if (hmodSpy)
                {
                    IShellMallocSpy *pms;
                    PFNGSMS pfnGetShellMallocSpy = (PFNGSMS)GetProcAddress(hmodSpy, "GetShellMallocSpy");
                    if (pfnGetShellMallocSpy(&pms))
                    {
                        pms->lpVtbl->RegisterSpy(pms);
                        pms->lpVtbl->Release(pms); // the object is static, so it doesn't really matter
                    }
                }
            }
#endif
        }

        if (g_pmemTask == NULL)
        {
            // use the shell task allocator (which is LocalAlloc).
            g_pmemTask = (IMalloc *)&c_mem; // const -> non const
        }
    }
    else
    {
        // handing out cached version, add ref it first
        g_pmemTask->lpVtbl->AddRef(g_pmemTask);
    }

    *ppmem = g_pmemTask;
}

//
// To be exported
//
STDAPI SHGetMalloc(IMalloc **ppmem)
{
    _GetTaskAllocator(ppmem);
    return NOERROR;
}

// BOGUS, NT redefines these to HeapAlloc variants...
#ifdef Alloc
#undef Alloc
#undef Free
#undef GetSize
#endif

__inline void FAST_GetTaskAllocator()
{
    IMalloc *pmem;
    if (g_pmemTask == NULL) {
        // perf: avoid calling unless really need to
        _GetTaskAllocator(&pmem);
        ASSERT(g_pmemTask != NULL);
        ASSERT(g_pmemTask == pmem);
    }
    // else n.b. no AddRef!  but we have a refcnt of >=1, and we never Release
    // so who cares...
    return;
}

STDAPI_(void *) SHAlloc(SIZE_T cb)
{
    FAST_GetTaskAllocator();
    return g_pmemTask->lpVtbl->Alloc(g_pmemTask, cb);
}

STDAPI_(void *) SHRealloc(LPVOID pv, SIZE_T cbNew)
{
    IMalloc *pmem;
    _GetTaskAllocator(&pmem);
    return pmem->lpVtbl->Realloc(pmem, pv, cbNew);
}

STDAPI_(void) SHFree(LPVOID pv)
{
    FAST_GetTaskAllocator();
    g_pmemTask->lpVtbl->Free(g_pmemTask, pv);
}

STDAPI_(SIZE_T) SHGetSize(LPVOID pv)
{
    IMalloc *pmem;
    _GetTaskAllocator(&pmem);
    return (SIZE_T) pmem->lpVtbl->GetSize(pmem, pv);
}

#ifdef DEBUG
void TaskMem_MakeInvalid(void)
{
    static IMalloc c_memDummy = { &c_CShellMallocVtbl };
    //
    // so we can catch calls to the allocator after PROCESS_DETATCH
    // which should be illegal because OLE32.DLL can be unloaded before our
    // DLL (unload order is not deterministic) we switch the allocator
    // to this dummy one that will cause our asserts to trip.
    //
    // note, runnin the dummy alllocator is actually fine as it will free
    // memory with LocalAlloc(), which is what the OLE alocator uses in all
    // cases except debug. and besides, our process is about to go away and
    // all process memory will be freed anyway!
    //
    g_pmemTask = &c_memDummy;
}
#endif
