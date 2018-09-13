/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop 

#include "teb.hxx"

#ifdef PRFDATA
#include "core/prfdata/msxmlprfcounters.h"
#endif

DWORD g_dwTlsIndex;
TLSDATA * g_ptlsdata;
TLSDATA * g_ptlsdataExtra;
DWORD   g_dwPlatformId;

#if _X86_
__declspec(naked) PVOID WINAPI
Win95_InterlockedCompareExchange(PVOID *Destination, PVOID Exchange, PVOID Comperand)
{
    _asm    mov         ecx,dword ptr [esp+4]
    _asm    mov         edx,dword ptr [esp+8]
    _asm    mov         eax,dword ptr [esp+0Ch]
    _asm    cmpxchg     dword ptr [ecx],edx
    _asm    ret         0Ch
}
#endif

#if _X86_
__declspec(naked) LONG WINAPI
Win95_InterlockedExchangeAdd(LPLONG Addend, LONG Value)
{
    _asm    mov         ecx,dword ptr [esp+4]
    _asm    mov         eax,dword ptr [esp+8]
    _asm    xadd        dword ptr [ecx],eax
    _asm    ret         08h
}
#endif

PFN_INTERLOCKEDCOMPAREEXCHANGE g_pfnInterlockedCompareExchange;
PFN_INTERLOCKEDEXCHANGEADD g_pfnInterlockedExchangeAdd;

PFN_ENTRY g_pfnEntry;
PFN_EXIT g_pfnExit;


void TlsInit()
{
    g_pfnEntry = Base::StackEntryNormal;
    g_pfnExit = Base::StackExitNormal;
    g_dwTlsIndex = TlsAlloc();
    Assert((g_dwTlsIndex != 0xFFFFFFFF) && "We are in trouble if this fails");
#if _X86_
    OSVERSIONINFOA vi;
    vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    if (!GetVersionExA(&vi))
        Exception::throwLastError();
    g_dwPlatformId = vi.dwPlatformId;
    if (g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)    // win95 or win98
    {
        g_pfnInterlockedCompareExchange = Win95_InterlockedCompareExchange;
        g_pfnInterlockedExchangeAdd = Win95_InterlockedExchangeAdd;
    }
    else
    {
#endif
        HINSTANCE s_hKernel = GetModuleHandleA("kernel32.dll");
        if (s_hKernel)
        {
#ifdef _WIN64
//			g_pfnInterlockedCompareExchange = (PFN_INTERLOCKEDCOMPAREEXCHANGE)GetProcAddress(s_hKernel, "InterlockedCompareExchangePointer");
#else
			g_pfnInterlockedCompareExchange = (PFN_INTERLOCKEDCOMPAREEXCHANGE)GetProcAddress(s_hKernel, "InterlockedCompareExchange");
            if ((FARPROC)g_pfnInterlockedCompareExchange == NULL)
                Exception::throwLastError();
#endif
            g_pfnInterlockedExchangeAdd = (PFN_INTERLOCKEDEXCHANGEADD)GetProcAddress(s_hKernel, "InterlockedExchangeAdd");
            if ((FARPROC)g_pfnInterlockedExchangeAdd == NULL)
                Exception::throwLastError();
        }
        else
            Exception::throwLastError();
#if _X86_
    }
#endif
    Assert(TlsGetValue(g_dwTlsIndex) == 0);
    g_ptlsdataExtra = AllocTlsData();
}

void TlsExit()
{
    TLSDATA * ptlsdata;
    while (ptlsdata = g_ptlsdata)
    {
        g_ptlsdata = ptlsdata->_pNext;
        delete ptlsdata;
    }
        
    TlsFree(g_dwTlsIndex);
}

void TlsClear()
{
    TLSDATA * ptlsdata = g_ptlsdata;
    while (ptlsdata)
    {
        release(&ptlsdata->_pException);
        ptlsdata = ptlsdata->_pNext;
    }
}

DLLEXPORT DWORD GetTlsIndex()
{
    return g_dwTlsIndex;
}

extern ShareMutex * g_pMutexGC;
BOOL g_fInShutDown = FALSE;

TLSDATA * AllocTlsData()
{
    TLSDATA * ptlsdata;

    TRY
    {
        ptlsdata = new TLSDATA;
        Assert(ptlsdata && "Exception handling");
        TlsSetValue(g_dwTlsIndex, ptlsdata);
    }
    CATCH
    {
        if (g_fInShutDown && g_ptlsdataExtra)
        {
            ptlsdata = g_ptlsdataExtra;
            ptlsdata->reinit();
        }
        else
            ptlsdata = null;
    }
    ENDTRY

    if (ptlsdata)
    {
        TLSDATA * ptlstemp;
        do
        {
            ptlstemp = ptlsdata->_pNext = g_ptlsdata;
        }
        while (InterlockedCompareExchange((void**)&g_ptlsdata, ptlsdata, ptlstemp) != ptlstemp);
    }
    return ptlsdata;
}


void
DeleteTlsData()
{
    TLSDATA * ptlsdata = (TLSDATA *)TlsGetValue(g_dwTlsIndex);
    if (ptlsdata)
    {
        //It is bad to call testForGC, since this function is called
        // from DLLMain at thread detach time.  While that function is
        // being executed no new threads can be created... so make things
        // very short and sweet
        //Base::testForGC(0); // clean up any remaining objects.

#ifdef _DEBUG
#ifdef RENTAL_MODEL
        // WE should never have any rental objects left
        if (ptlsdata->_uRentals != 0)
        {
            DebugBreak(); // Assert hangs if we do it here.
        }
#endif
#endif
        // The GC will remove the structure when it is safe...
        ptlsdata->_fThreadExited = true;
        // and since we're going to clean this up in the next GC cycle,
        // we remove it from the thread local storage so that the next
        // call the TlsGetValue on the same threadid doesn't return
        // a dead pointer.
        TlsSetValue(g_dwTlsIndex,0); 
    }
}


// use process heap because this is used at the very early in the startup code
EXTERN_C HANDLE g_hProcessHeap;

void * TLSDATA::operator new(size_t s)
{
#ifdef PRFDATA
    PrfCountHeapSize(s);
#endif
#ifdef RENTAL_MODEL
    // make sure allocated on 8 byte boundary
    void * p = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, s + REF_RENTAL);
    if ((INT_PTR)p & REF_RENTAL)
    {
        p = (BYTE *)p + REF_RENTAL;
        ((TLSDATA *)p)->_fMisAligned = true;
    }
    Assert(((UINT_PTR)p & REF_RENTAL) == 0);
#else
    void * p = HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, s);
#endif
    return p;
}

void TLSDATA::operator delete(void * p)
{
#ifdef PRFDATA
    long size = (long)HeapSize(g_hProcessHeap, 0, p);
    PrfCountHeapSize(-size);
#endif
#ifdef RENTAL_MODEL
    if (((TLSDATA *)p)->_fMisAligned == true)
    {
        p = (BYTE *)p - REF_RENTAL;
    }
#endif
    HeapFree(g_hProcessHeap, 0, p);
}

TLSDATA::TLSDATA()
{
    init();
}

void
TLSDATA::init()
{
    BOOL f = ::DuplicateHandle(
        GetCurrentProcess(),                            // handle to process with handle to duplicate
        GetCurrentThread(),                                // handle to duplicate
        GetCurrentProcess(),                            // handle to process to duplicate to
        &_hThread,                                        // pointer to duplicate handle
        THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION|THREAD_SUSPEND_RESUME ,    // access for duplicate handle
        FALSE,                                            // handle inheritance flag
        0);                                             // optional actions);
#if DBG == 1
    Assert(f && "Couldn't duplicate thread handle");
#endif
    _dwTID = ::GetCurrentThreadId();
    _fThreadExited = false;
    Assert(_baseHead._pfnvtable == null);
    Assert(_baseHeadLocked._pfnvtable == null);
    _baseHead._refs = (ULONG_PTR)&_baseHead;
    _baseHeadLocked._refs = (ULONG_PTR)&_baseHead;
    _pTEB = getTEB();
#ifdef RENTAL_MODEL
    _reModel = MultiThread;
#endif
}

void
TLSDATA::reinit()
{
    CloseHandle(_hThread);
    release(&_pException);
    init();
}

TLSDATA::~TLSDATA()
{
    CloseHandle(_hThread);
    release(&_pException);
}
#ifdef RENTAL_MODEL

Model::Model(TLSDATA * ptlsdata, RentalEnum re)
{
    init(ptlsdata, re);
}

Model::Model(RentalEnum re)
{
    init(GetTlsData(), re);
}

Model::Model(TLSDATA * ptlsdata, Base * pBase)
{
    init(ptlsdata, pBase->model());
}

Model::Model(TLSDATA * ptlsdata, Object * pObject)
{
    init(ptlsdata, pObject->getBase()->model());
}

void
Model::init(TLSDATA * ptlsdata, RentalEnum re)
{
    _ptlsdata = ptlsdata;
    _reSavedModel = _ptlsdata->_reModel;
    _ptlsdata->_reModel = _reModel = re;
}

Model::~Model()
{
    if (_ptlsdata)
        Release();
}

void
Model::Release()
{
    _ptlsdata->_reModel = _reSavedModel;
    _ptlsdata = null;
}

#endif
