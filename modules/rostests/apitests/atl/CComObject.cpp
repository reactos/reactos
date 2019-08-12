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
    #include "atltest.h"
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
}
