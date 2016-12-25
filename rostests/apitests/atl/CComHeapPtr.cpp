/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CComHeapPtr
 * PROGRAMMER:      Mark Jansen
 */

#include <apitest.h>
#include <atlbase.h>
#include <atlcom.h>

static PDWORD test_Alloc(DWORD value)
{
    PDWORD ptr = (PDWORD)::CoTaskMemAlloc(sizeof(DWORD));
    *ptr = value;
    return ptr;
}


static LONG g_OpenAllocations = 0;
static LONG g_Reallocations = 0;

struct CMallocSpy : public IMallocSpy
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
    virtual ULONG STDMETHODCALLTYPE PreAlloc(ULONG cbRequest) { return cbRequest; }
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
    virtual ULONG STDMETHODCALLTYPE PreRealloc(LPVOID pRequest, ULONG cbRequest, LPVOID *ppNewRequest, BOOL)
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
    virtual ULONG STDMETHODCALLTYPE PostGetSize(ULONG cbActual, BOOL) { return cbActual; }
    virtual LPVOID STDMETHODCALLTYPE PreDidAlloc(LPVOID pRequest, BOOL) { return pRequest; }
    virtual int STDMETHODCALLTYPE PostDidAlloc(LPVOID, BOOL, int fActual) { return fActual; }
    virtual void STDMETHODCALLTYPE PreHeapMinimize() {}
    virtual void STDMETHODCALLTYPE PostHeapMinimize() {}
};

static CMallocSpy g_Spy;


START_TEST(CComHeapPtr)
{
    HRESULT hr = CoRegisterMallocSpy(&g_Spy);
    ok(SUCCEEDED(hr), "Expected CoRegisterMallocSpy to succeed, but it failed: 0x%lx\n", hr);

    {
        ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);
        CComHeapPtr<DWORD> heapPtr1;
        ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);
        CComHeapPtr<DWORD> heapPtr2(test_Alloc(0x11111111));
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);

        ok((PDWORD)heapPtr1 == NULL, "Expected heapPtr1 to be NULL, was: 0x%p\n", (PDWORD)heapPtr1);
        ok((PDWORD)heapPtr2 != NULL, "Expected heapPtr2 to not be NULL\n");
        ok(*heapPtr2 == 0x11111111, "Expected *heapPtr2 to be 0x11111111, but was: 0x%lx\n", *heapPtr2);

        {
            ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
            CComHeapPtr<DWORD> heapPtrSteal1(heapPtr1);
            ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
            ok((PDWORD)heapPtr1 == NULL, "Expected heapPtr1 to be NULL, was: 0x%p\n", (PDWORD)heapPtr1);
            ok((PDWORD)heapPtrSteal1 == NULL, "Expected heapPtrSteal1 to be NULL, was: 0x%p\n", (PDWORD)heapPtrSteal1);
            CComHeapPtr<DWORD> heapPtrSteal2(heapPtr2);
            ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
            ok((PDWORD)heapPtr2 == NULL, "Expected heapPtr2 to be NULL, was: 0x%p\n", (PDWORD)heapPtr2);
            ok((PDWORD)heapPtrSteal2 != NULL, "Expected heapPtrSteal2 to not be NULL\n");
            ok(*heapPtrSteal2 == 0x11111111, "Expected *heapPtrSteal2 to be 0x11111111, but was: 0x%lx\n", *heapPtrSteal2);
        }
        ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);

        ok(heapPtr1.Allocate(1), "Expected Allocate to succeed\n");
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
        ok(g_Reallocations == 0, "Expected there to be 0 reallocations, was: %ld\n", g_Reallocations);

        *heapPtr1 = 0x22222222;
        ok(*heapPtr1 == 0x22222222, "Expected *heapPtr1 to be 0x22222222, but was: 0x%lx\n", *heapPtr1);

        ok(heapPtr1.Reallocate(2), "Expected Reallocate to succeed\n");
        heapPtr1[1] = 0x33333333;
        ok(*heapPtr1 == 0x22222222, "Expected *heapPtr1 to be 0x22222222, but was: 0x%lx\n", *heapPtr1);
        ok(g_Reallocations == 1, "Expected there to be 1 reallocations, was: %ld\n", g_Reallocations);
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);

        heapPtr2 = heapPtr1;
        ok(g_OpenAllocations == 1, "Expected there to be 1 allocations, was: %ld\n", g_OpenAllocations);
        ok(*heapPtr2 == 0x22222222, "Expected *heapPtr2 to be 0x33333333, but was: 0x%lx\n", *heapPtr2);
        ok(heapPtr2[1] == 0x33333333, "Expected heapPtr2[1] to be 0x33333333, but was: 0x%lx\n", heapPtr2[1]);
        ok((PDWORD)heapPtr1 == NULL, "Expected heapPtr1 to be NULL, was: 0x%p\n", (PDWORD)heapPtr1);
    }
    ok(g_OpenAllocations == 0, "Expected there to be 0 allocations, was: %ld\n", g_OpenAllocations);

    hr = CoRevokeMallocSpy();
    ok(SUCCEEDED(hr), "Expected CoRevokeMallocSpy to succeed, but it failed: 0x%lx\n", hr);
}
