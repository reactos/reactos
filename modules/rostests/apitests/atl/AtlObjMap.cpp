/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for OBJECT_ENTRY_AUTO
 * COPYRIGHT:   Copyright 2023 Mark Jansen <mark.jansen@reactos.org>
 */

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif

#include <atlbase.h>
#include <atlcom.h>

class CAtlObjMapModule : public ATL::CAtlExeModuleT<CAtlObjMapModule>
{

} _Module;

CLSID CLSID_ObjMapTestObject = {0xeae5616c, 0x1e7f, 0x4a00, {0x92, 0x27, 0x1a, 0xad, 0xad, 0x66, 0xbb, 0x85}};

static LONG g_Created = 0;
static LONG g_Destroyed = 0;


START_TEST(AtlObjMap)
{
    HRESULT hr = CoInitialize(NULL);
    ok_hex(hr, S_OK);

    {
        CComPtr<IUnknown> spTestObject;
        hr = CoCreateInstance(CLSID_ObjMapTestObject, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&spTestObject);
        ok_hex(hr, REGDB_E_CLASSNOTREG);
    }

    ok_int(g_Created, 0);
    ok_int(g_Destroyed, 0);

    // Register the com objects added to the auto-map in _AtlComModule
    hr = _Module.RegisterClassObjects(CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE);
    ok_hex(hr, S_OK);

    ok_int(g_Created, 0);
    ok_int(g_Destroyed, 0);

    {
        CComPtr<IUnknown> spTestObject;
        hr = CoCreateInstance(CLSID_ObjMapTestObject, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID *)&spTestObject);
        ok_hex(hr, S_OK);
        ok_int(g_Created, 1);
        ok_int(g_Destroyed, 0);
    }
    ok_int(g_Created, 1);
    ok_int(g_Destroyed, 1);
}



struct CObjMapTestObject : public CComObjectRootEx<CComSingleThreadModel>,
                           public CComCoClass<CObjMapTestObject, &CLSID_ObjMapTestObject>,
                          public IUnknown
{
    CObjMapTestObject()
    {
        InterlockedIncrement(&g_Created);
    }
    ~CObjMapTestObject()
    {
        InterlockedIncrement(&g_Destroyed);
    }


    DECLARE_PROTECT_FINAL_CONSTRUCT();
    DECLARE_NO_REGISTRY();
    DECLARE_NOT_AGGREGATABLE(CObjMapTestObject)

    BEGIN_COM_MAP(CObjMapTestObject)
        COM_INTERFACE_ENTRY_IID(IID_IUnknown, IUnknown)
    END_COM_MAP()
};

OBJECT_ENTRY_AUTO(CLSID_ObjMapTestObject, CObjMapTestObject)

