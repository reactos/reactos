/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CComObject
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */


#include <atlbase.h>
#include <atlcom.h>

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <stdarg.h>
    int g_tests_executed = 0;
    int g_tests_failed = 0;
    void ok_func(const char *file, int line, BOOL value, const char *fmt, ...)
    {
        va_list va;
        va_start(va, fmt);
        if (!value)
        {
            printf("%s (%d): ", file, line);
            vprintf(fmt, va);
            g_tests_failed++;
        }
        g_tests_executed++;
        va_end(va);
    }
    #undef ok
    #define ok(value, ...)  ok_func(__FILE__, __LINE__, value, __VA_ARGS__)
    #define START_TEST(x)   int main(void)
#endif


static LONG g_CTOR = 0;
static LONG g_DTOR = 0;
static LONG g_BLIND = 0;

class CTestObject :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IPersist,
    public IStdMarshalInfo
{
public:
    CTestObject()
    {
        InterlockedIncrement(&g_CTOR);
    }
    ~CTestObject()
    {
        InterlockedIncrement(&g_DTOR);
    }

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID)
    {
        return E_NOTIMPL;
    }

    // *** IStdMarshalInfo methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassForHandler(DWORD dwDestContext, void *pvDestContext, CLSID *pClsid)
    {
        return E_NOTIMPL;
    }

    static HRESULT WINAPI FuncBlind(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
    {
        InterlockedIncrement(&g_BLIND);
        return E_FAIL;
    }

    DECLARE_NOT_AGGREGATABLE(CTestObject)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CTestObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist) /* First entry has to be a simple entry, otherwise ATL asserts */
        COM_INTERFACE_ENTRY_FUNC_BLIND(0, FuncBlind)    /* Showing that even after a Blind func, entryies can be found */
        COM_INTERFACE_ENTRY_IID(IID_IStdMarshalInfo, IStdMarshalInfo)
    END_COM_MAP()
};


class CDumExe: public CAtlExeModuleT<CDumExe>
{

};
CDumExe dum;


START_TEST(CComObject)
{
    g_CTOR = g_DTOR = g_BLIND = 0;

    CComObject<CTestObject>* pTest;
    HRESULT hr = CComObject<CTestObject>::CreateInstance(&pTest);

    ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);

    ok(g_CTOR == 1, "Expected 1, got %lu\n", g_CTOR);
    ok(g_DTOR == 0, "Expected 0, got %lu\n", g_DTOR);
    ok(g_BLIND == 0, "Expected 0, got %lu\n", g_BLIND);

    if (hr == S_OK)
    {
        ULONG ref = pTest->AddRef();
        ok(ref == 1, "Expected 1, got %lu\n", ref);

        {
            CComPtr<IUnknown> ppv;
            hr = pTest->QueryInterface(IID_IUnknown, (void **) &ppv);
            ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
            ok(g_CTOR == 1, "Expected 1, got %lu\n", g_CTOR);
            ok(g_DTOR == 0, "Expected 0, got %lu\n", g_DTOR);
            ok(g_BLIND == 0, "Expected 0, got %lu\n", g_BLIND);

            CComPtr<IPersist> ppersist;
            hr = pTest->QueryInterface(IID_IPersist, (void **)&ppersist);
            ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
            ok(g_CTOR == 1, "Expected 1, got %lu\n", g_CTOR);
            ok(g_DTOR == 0, "Expected 0, got %lu\n", g_DTOR);
            ok(g_BLIND == 0, "Expected 0, got %lu\n", g_BLIND);

        }

        {
            CComPtr<IStdMarshalInfo> pstd;
            hr = pTest->QueryInterface(IID_IStdMarshalInfo, (void **)&pstd);
            ok(hr == S_OK, "Expected S_OK, got 0x%lx\n", hr);
            ok(g_CTOR == 1, "Expected 1, got %lu\n", g_CTOR);
            ok(g_DTOR == 0, "Expected 0, got %lu\n", g_DTOR);
            ok(g_BLIND == 1, "Expected 1, got %lu\n", g_BLIND);
        }

        ref = pTest->Release();
        ok(ref == 0, "Expected 0, got %lu\n", ref);
        ok(g_CTOR == 1, "Expected 1, got %lu\n", g_CTOR);
        ok(g_DTOR == 1, "Expected 1, got %lu\n", g_DTOR);
        ok(g_BLIND == 1, "Expected 1, got %lu\n", g_BLIND);
    }

#ifndef HAVE_APITEST
    printf("CComObject: %i tests executed (0 marked as todo, %i failures), 0 skipped.\n", g_tests_executed, g_tests_failed);
    return g_tests_failed;
#endif
}
