/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for CComQIPtr
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
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


static LONG g_QI = 0;

class CQITestObject :
    public IPersist,
    public IStdMarshalInfo
{
public:
    LONG m_dwRef;

    CQITestObject()
        :m_dwRef(1)
    {
    }
    ~CQITestObject()
    {
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        InterlockedIncrement(&m_dwRef);
        return 2;
    }

    STDMETHOD_(ULONG, Release)()
    {
        InterlockedDecrement(&m_dwRef);
        return 1;
    }

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
    {
        InterlockedIncrement(&g_QI);
        if (iid == IID_IUnknown || iid == IID_IPersist)
        {
            AddRef();
            *ppvObject = static_cast<IPersist*>(this);
            return S_OK;
        }
        else if (iid == IID_IStdMarshalInfo)
        {
            AddRef();
            *ppvObject = static_cast<IStdMarshalInfo*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID)
    {
        return E_NOTIMPL;
    }

    // *** IStdMarshalInfo methods ***
    STDMETHOD(GetClassForHandler)(DWORD dwDestContext, void *pvDestContext, CLSID *pClsid)
    {
        return E_NOTIMPL;
    }
};

// Yes this sucks, but we have to support GCC. (CORE-12710)
#ifdef __REACTOS__
#define DECLARE_QIPTR(type)     CComQIIDPtr<I_ID(type)>
#elif defined(__GNUC__)
#define DECLARE_QIPTR(type)     CComQIIDPtr<I_ID(type)>
#else
#define DECLARE_QIPTR(type)     CComQIPtr<type>
#endif

START_TEST(CComQIPtr)
{
    CQITestObject testObject;
    IUnknown* unk = static_cast<IPersist*>(&testObject);
    ok(testObject.m_dwRef == 1, "Expected m_dwRef 1, got %lu\n", testObject.m_dwRef);
    ok(g_QI == 0, "Expected g_QI 0, got %lu\n", g_QI);

    {
        DECLARE_QIPTR(IPersist) ppPersist(unk);
        ok(testObject.m_dwRef == 2, "Expected m_dwRef 2, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 1, "Expected g_QI 1, got %lu\n", g_QI);

        DECLARE_QIPTR(IStdMarshalInfo) ppMarshal(ppPersist);
        ok(testObject.m_dwRef == 3, "Expected m_dwRef 3, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 2, "Expected g_QI 2, got %lu\n", g_QI);
    }
    ok(testObject.m_dwRef == 1, "Expected m_dwRef 1, got %lu\n", testObject.m_dwRef);
    {
        DECLARE_QIPTR(IStdMarshalInfo) ppMarshal;
        ok(testObject.m_dwRef == 1, "Expected m_dwRef 1, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 2, "Expected g_QI 2, got %lu\n", g_QI);

        ppMarshal = unk;
        ok(testObject.m_dwRef == 2, "Expected m_dwRef 2, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 3, "Expected g_QI 3, got %lu\n", g_QI);

        ppMarshal = static_cast<IUnknown*>(NULL);
        ok(testObject.m_dwRef == 1, "Expected m_dwRef 1, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 3, "Expected g_QI 3, got %lu\n", g_QI);

        CComPtr<IUnknown> spUnk(unk);
        ok(testObject.m_dwRef == 2, "Expected m_dwRef 2, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 3, "Expected g_QI 3, got %lu\n", g_QI);

        ppMarshal = spUnk;
        ok(testObject.m_dwRef == 3, "Expected m_dwRef 3, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 4, "Expected g_QI 4, got %lu\n", g_QI);

        spUnk.Release();
        ok(testObject.m_dwRef == 2, "Expected m_dwRef 2, got %lu\n", testObject.m_dwRef);
        ok(g_QI == 4, "Expected g_QI 4, got %lu\n", g_QI);

        spUnk = ppMarshal;
        ok(testObject.m_dwRef == 3, "Expected m_dwRef 3, got %lu\n", testObject.m_dwRef);
#ifdef __REACTOS__
        // CORE-12710
        todo_if(1)
#endif
        ok(g_QI == 5, "Expected g_QI 5, got %lu\n", g_QI);
    }

#ifndef HAVE_APITEST
    printf("CComQIPtr: %i tests executed (0 marked as todo, %i failures), 0 skipped.\n", g_tests_executed, g_tests_failed);
    return g_tests_failed;
#endif
}
