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
    #include "atltest.h"
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
    ok_long(testObject.m_dwRef, 1);
    ok_long(g_QI, 0);

    {
        DECLARE_QIPTR(IPersist) ppPersist(unk);
        ok_long(testObject.m_dwRef, 2);
        ok_long(g_QI, 1);

        DECLARE_QIPTR(IStdMarshalInfo) ppMarshal(ppPersist);
        ok_long(testObject.m_dwRef, 3);
        ok_long(g_QI, 2);
    }
    ok_long(testObject.m_dwRef, 1);
    {
        DECLARE_QIPTR(IStdMarshalInfo) ppMarshal;
        ok_long(testObject.m_dwRef, 1);
        ok_long(g_QI, 2);

        ppMarshal = unk;
        ok_long(testObject.m_dwRef, 2);
        ok_long(g_QI, 3);

        ppMarshal = static_cast<IUnknown*>(NULL);
        ok_long(testObject.m_dwRef, 1);
        ok_long(g_QI, 3);

        CComPtr<IUnknown> spUnk(unk);
        ok_long(testObject.m_dwRef, 2);
        ok_long(g_QI, 3);

        ppMarshal = spUnk;
        ok_long(testObject.m_dwRef, 3);
        ok_long(g_QI, 4);

        spUnk.Release();
        ok_long(testObject.m_dwRef, 2);
        ok_long(g_QI, 4);

        spUnk = ppMarshal;
        ok_long(testObject.m_dwRef, 3);
#ifdef __REACTOS__
        // CORE-12710
        todo_if(1)
#endif
        ok_long(g_QI, 5);
    }
}
