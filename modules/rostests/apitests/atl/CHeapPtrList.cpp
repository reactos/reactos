/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for CHeapPtrList
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#ifdef HAVE_APITEST
#include <apitest.h>
#else
#include "atltest.h"
#endif
#include <atlcoll.h>

static PDWORD
test_Alloc(DWORD value)
{
    PDWORD ptr = (PDWORD)::CoTaskMemAlloc(sizeof(DWORD));
    *ptr = value;
    return ptr;
}

// We use the CComAllocator, so we can easily spy on it
template <typename T>
class CComHeapPtrList :
    public CHeapPtrList<T, CComAllocator>
{
public:
    CComHeapPtrList(_In_ UINT nBlockSize = 10) throw()
        :CHeapPtrList<T, CComAllocator>(nBlockSize)
    {
    }
};



static LONG g_OpenAllocations = 0;
static LONG g_Reallocations = 0;

struct CHeapPtrListMallocSpy : public IMallocSpy
{
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
    {
        if (IsEqualGUID(riid, IID_IMallocSpy))
        {
            *ppvObject = this;
        }
        return S_OK;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef() { return 1; }
    virtual ULONG STDMETHODCALLTYPE Release() { return 1; }
    virtual SIZE_T STDMETHODCALLTYPE PreAlloc(SIZE_T cbRequest) { return cbRequest; }
    virtual LPVOID STDMETHODCALLTYPE PostAlloc(LPVOID pActual)
    {
        InterlockedIncrement(&g_OpenAllocations);
        return pActual;
    }
    virtual LPVOID STDMETHODCALLTYPE PreFree(LPVOID pRequest, BOOL) { return pRequest; }
    virtual void STDMETHODCALLTYPE PostFree(BOOL fSpyed)
    {
        if (fSpyed)
            InterlockedDecrement(&g_OpenAllocations);
    }
    virtual SIZE_T STDMETHODCALLTYPE PreRealloc(LPVOID pRequest, SIZE_T cbRequest, LPVOID *ppNewRequest, BOOL)
    {
        *ppNewRequest = pRequest;
        return cbRequest;
    }
    virtual LPVOID STDMETHODCALLTYPE PostRealloc(LPVOID pActual, BOOL fSpyed)
    {
        if (fSpyed)
            InterlockedIncrement(&g_Reallocations);
        return pActual;
    }
    virtual LPVOID STDMETHODCALLTYPE PreGetSize(LPVOID pRequest, BOOL) { return pRequest; }
    virtual SIZE_T STDMETHODCALLTYPE PostGetSize(SIZE_T cbActual, BOOL) { return cbActual; }
    virtual LPVOID STDMETHODCALLTYPE PreDidAlloc(LPVOID pRequest, BOOL) { return pRequest; }
    virtual int STDMETHODCALLTYPE PostDidAlloc(LPVOID, BOOL, int fActual) { return fActual; }
    virtual void STDMETHODCALLTYPE PreHeapMinimize() {}
    virtual void STDMETHODCALLTYPE PostHeapMinimize() {}
};

static CHeapPtrListMallocSpy g_Spy;


START_TEST(CHeapPtrList)
{
    HRESULT hr = CoRegisterMallocSpy(&g_Spy);
    ok(SUCCEEDED(hr), "Expected CoRegisterMallocSpy to succeed, but it failed: 0x%lx\n", hr);

    {
        ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);
        CComHeapPtrList<DWORD> heapPtr1;
        ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);
        PDWORD Ptr = test_Alloc(0x11111111);
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
        CComHeapPtr<DWORD> tmp(Ptr);
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
        heapPtr1.AddTail(tmp);
        ok(tmp.m_pData == NULL, "Expected m_pData to be transfered\n");
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
        Ptr = test_Alloc(0x22222222);
        ok(g_OpenAllocations == 2, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
#if defined(_MSC_VER) && !defined(__clang__)
        heapPtr1.AddTail(CComHeapPtr<DWORD>(Ptr));
#else
        CComHeapPtr<DWORD> xxx(Ptr);
        heapPtr1.AddTail(xxx);
#endif
        ok(g_OpenAllocations == 2, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
    }
    ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);

    hr = CoRevokeMallocSpy();
    ok(SUCCEEDED(hr), "Expected CoRevokeMallocSpy to succeed, but it failed: 0x%lx\n", hr);
}
