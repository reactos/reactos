/*
 * Copyright 2011 Jacek Caban for CodeWeavers
 * Copyright 2019 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#define CONST_VTABLE

#include <initguid.h>
#include <ole2.h>
#include <activscp.h>
#include <objsafe.h>
#include <dispex.h>

#include "vbsregexp55.h"

#include "wine/test.h"

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release \
    IActiveScriptParseProcedure2_64_Release

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release \
    IActiveScriptParseProcedure2_32_Release

#endif

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(IID_IScriptTypeInfo, 0xc59c6b12, 0xf6c1, 0x11cf, 0x88,0x35, 0x00,0xa0,0xc9,0x11,0xe8,0xb2);

#define DEFINE_EXPECT(func) \
    static int expect_ ## func = 0, called_ ## func = 0

#define SET_EXPECT(func) \
    expect_ ## func = 1

#define SET_EXPECT_MULTI(func, num) \
    expect_ ## func = num

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func++; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func--; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

#define CHECK_CALLED_MULTI(func, num) \
    do { \
        ok(called_ ## func == num, "expected " #func " %d times (got %d)\n", num, called_ ## func); \
        expect_ ## func = called_ ## func = 0; \
    }while(0)

DEFINE_EXPECT(GetLCID);
DEFINE_EXPECT(OnStateChange_UNINITIALIZED);
DEFINE_EXPECT(OnStateChange_STARTED);
DEFINE_EXPECT(OnStateChange_CONNECTED);
DEFINE_EXPECT(OnStateChange_DISCONNECTED);
DEFINE_EXPECT(OnStateChange_CLOSED);
DEFINE_EXPECT(OnStateChange_INITIALIZED);
DEFINE_EXPECT(OnEnterScript);
DEFINE_EXPECT(OnLeaveScript);
DEFINE_EXPECT(OnScriptError);
DEFINE_EXPECT(GetIDsOfNames);
DEFINE_EXPECT(GetIDsOfNames_visible);
DEFINE_EXPECT(GetIDsOfNames_persistent);
DEFINE_EXPECT(GetItemInfo_global);
DEFINE_EXPECT(GetItemInfo_global_code);
DEFINE_EXPECT(GetItemInfo_visible);
DEFINE_EXPECT(GetItemInfo_visible_code);
DEFINE_EXPECT(GetItemInfo_persistent);
DEFINE_EXPECT(testCall);

DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4,0xb0, 0x00,0xaa,0x00,0x4a,0x55,0xe8);
DEFINE_GUID(CLSID_VBScriptRegExp, 0x3f4daca4, 0x160d, 0x11d2, 0xa8,0xe9, 0x00,0x10,0x4b,0x36,0x5c,0x9f);

#define test_state(s,ss) _test_state(__LINE__,s,ss)
static void _test_state(unsigned line, IActiveScript *script, SCRIPTSTATE exstate)
{
    SCRIPTSTATE state = -1;
    HRESULT hres;

    hres = IActiveScript_GetScriptState(script, &state);
    ok_(__FILE__,line) (hres == S_OK, "GetScriptState failed: %08lx\n", hres);
    ok_(__FILE__,line) (state == exstate, "state=%d, expected %d\n", state, exstate);
}

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        IDispatch_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Dispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI Dispatch_Release(IDispatch *iface)
{
    return 1;
}

static ULONG global_named_item_ref, visible_named_item_ref, visible_code_named_item_ref, persistent_named_item_ref;

static ULONG WINAPI global_AddRef(IDispatch *iface)
{
    return ++global_named_item_ref;
}

static ULONG WINAPI global_Release(IDispatch *iface)
{
    return --global_named_item_ref;
}

static ULONG WINAPI visible_AddRef(IDispatch *iface)
{
    return ++visible_named_item_ref;
}

static ULONG WINAPI visible_Release(IDispatch *iface)
{
    return --visible_named_item_ref;
}

static ULONG WINAPI visible_code_AddRef(IDispatch *iface)
{
    return ++visible_code_named_item_ref;
}

static ULONG WINAPI visible_code_Release(IDispatch *iface)
{
    return --visible_code_named_item_ref;
}

static ULONG WINAPI persistent_AddRef(IDispatch *iface)
{
    return ++persistent_named_item_ref;
}

static ULONG WINAPI persistent_Release(IDispatch *iface)
{
    return --persistent_named_item_ref;
}

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return DISP_E_BADINDEX;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names, UINT name_cnt,
                                            LCID lcid, DISPID *ids)
{
    ok(name_cnt == 1, "name_cnt = %u\n", name_cnt);
    if(!wcscmp(names[0], L"testCall")) {
        *ids = 1;
        return S_OK;
    }

    CHECK_EXPECT2(GetIDsOfNames);
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI visible_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names, UINT name_cnt,
                                            LCID lcid, DISPID *ids)
{
    ok(name_cnt == 1, "name_cnt = %u\n", name_cnt);
    if(!wcscmp(names[0], L"testCall")) {
        *ids = 1;
        return S_OK;
    }

    CHECK_EXPECT2(GetIDsOfNames_visible);
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI persistent_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names, UINT name_cnt,
                                               LCID lcid, DISPID *ids)
{
    ok(name_cnt == 1, "name_cnt = %u\n", name_cnt);

    CHECK_EXPECT2(GetIDsOfNames_persistent);
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID id, REFIID riid, LCID lcid, WORD flags,
                                      DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, UINT *err)
{
    CHECK_EXPECT(testCall);
    ok(id == 1, "id = %lu\n", id);
    ok(flags == DISPATCH_METHOD, "flags = %x\n", flags);
    ok(!dp->cArgs, "cArgs = %u\n", dp->cArgs);
    ok(!res, "res = %p\n", res);
    return S_OK;
}

static const IDispatchVtbl dispatch_vtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch dispatch_object = { &dispatch_vtbl };

static const IDispatchVtbl global_named_item_vtbl = {
    Dispatch_QueryInterface,
    global_AddRef,
    global_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch global_named_item = { &global_named_item_vtbl };

static const IDispatchVtbl visible_named_item_vtbl = {
    Dispatch_QueryInterface,
    visible_AddRef,
    visible_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    visible_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch visible_named_item = { &visible_named_item_vtbl };

static const IDispatchVtbl visible_code_named_item_vtbl = {
    Dispatch_QueryInterface,
    visible_code_AddRef,
    visible_code_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch visible_code_named_item = { &visible_code_named_item_vtbl };

static const IDispatchVtbl persistent_named_item_vtbl = {
    Dispatch_QueryInterface,
    persistent_AddRef,
    persistent_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    persistent_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch persistent_named_item = { &persistent_named_item_vtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSite, riid))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ActiveScriptSite_AddRef(IActiveScriptSite *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSite_Release(IActiveScriptSite *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSite_GetLCID(IActiveScriptSite *iface, LCID *plcid)
{
    CHECK_EXPECT(GetLCID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR name,
        DWORD return_mask, IUnknown **item_unk, ITypeInfo **item_ti)
{
    ok(return_mask == SCRIPTINFO_IUNKNOWN, "return_mask = %lx\n", return_mask);
    if(!wcscmp(name, L"globalItem")) {
        CHECK_EXPECT(GetItemInfo_global);
        IDispatch_AddRef(&global_named_item);
        *item_unk = (IUnknown*)&global_named_item;
        return S_OK;
    }
    if(!wcscmp(name, L"globalCodeItem")) {
        CHECK_EXPECT(GetItemInfo_global_code);
        IDispatch_AddRef(&dispatch_object);
        *item_unk = (IUnknown*)&dispatch_object;
        return S_OK;
    }
    if(!wcscmp(name, L"visibleItem")) {
        CHECK_EXPECT(GetItemInfo_visible);
        IDispatch_AddRef(&visible_named_item);
        *item_unk = (IUnknown*)&visible_named_item;
        return S_OK;
    }
    if(!wcscmp(name, L"visibleCodeItem")) {
        CHECK_EXPECT(GetItemInfo_visible_code);
        IDispatch_AddRef(&visible_code_named_item);
        *item_unk = (IUnknown*)&visible_code_named_item;
        return S_OK;
    }
    if(!wcscmp(name, L"persistent")) {
        CHECK_EXPECT(GetItemInfo_persistent);
        IDispatch_AddRef(&persistent_named_item);
        *item_unk = (IUnknown*)&persistent_named_item;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    switch(ssScriptState) {
    case SCRIPTSTATE_UNINITIALIZED:
        CHECK_EXPECT(OnStateChange_UNINITIALIZED);
        return S_OK;
    case SCRIPTSTATE_STARTED:
        CHECK_EXPECT(OnStateChange_STARTED);
        return S_OK;
    case SCRIPTSTATE_CONNECTED:
        CHECK_EXPECT(OnStateChange_CONNECTED);
        return S_OK;
    case SCRIPTSTATE_DISCONNECTED:
        CHECK_EXPECT(OnStateChange_DISCONNECTED);
        return S_OK;
    case SCRIPTSTATE_CLOSED:
        CHECK_EXPECT(OnStateChange_CLOSED);
        return S_OK;
    case SCRIPTSTATE_INITIALIZED:
        CHECK_EXPECT(OnStateChange_INITIALIZED);
        return S_OK;
    default:
        ok(0, "unexpected call %d\n", ssScriptState);
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    CHECK_EXPECT(OnScriptError);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    CHECK_EXPECT(OnEnterScript);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    CHECK_EXPECT(OnLeaveScript);
    return S_OK;
}

static const IActiveScriptSiteVtbl ActiveScriptSiteVtbl = {
    ActiveScriptSite_QueryInterface,
    ActiveScriptSite_AddRef,
    ActiveScriptSite_Release,
    ActiveScriptSite_GetLCID,
    ActiveScriptSite_GetItemInfo,
    ActiveScriptSite_GetDocVersionString,
    ActiveScriptSite_OnScriptTerminate,
    ActiveScriptSite_OnStateChange,
    ActiveScriptSite_OnScriptError,
    ActiveScriptSite_OnEnterScript,
    ActiveScriptSite_OnLeaveScript
};

static IActiveScriptSite ActiveScriptSite = { &ActiveScriptSiteVtbl };

static void test_safety(IActiveScript *script)
{
    IObjectSafety *safety;
    DWORD supported, enabled;
    HRESULT hres;

    hres = IActiveScript_QueryInterface(script, &IID_IObjectSafety, (void**)&safety);
    ok(hres == S_OK, "Could not get IObjectSafety: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, &supported, NULL);
    ok(hres == E_POINTER, "GetInterfaceSafetyOptions failed: %08lx, expected E_POINTER\n", hres);
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, NULL, &enabled);
    ok(hres == E_POINTER, "GetInterfaceSafetyOptions failed: %08lx, expected E_POINTER\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%lx\n", enabled);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScript, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%lx\n", enabled);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%lx\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER
                |INTERFACESAFE_FOR_UNTRUSTED_CALLER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == E_FAIL, "SetInterfaceSafetyOptions failed: %08lx, expected E_FAIL\n", hres);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08lx\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "enabled=%lx\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, INTERFACESAFE_FOR_UNTRUSTED_DATA, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08lx\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == (INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER), "enabled=%lx\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08lx\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%lx\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACE_USES_DISPEX, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08lx\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08lx\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%lx\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%lx\n", enabled);

    IObjectSafety_Release(safety);
}

static IDispatchEx *get_script_dispatch(IActiveScript *script, const WCHAR *item_name)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)0xdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, item_name, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08lx\n", hres);
    if(FAILED(hres))
        return NULL;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);
    return dispex;
}

static void parse_script(IActiveScriptParse *parse, const WCHAR *src)
{
    BSTR str;
    HRESULT hres;

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);

    str = SysAllocString(src);
    hres = IActiveScriptParse_ParseScriptText(parse, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
}

#define get_disp_id(a,b,c,d) _get_disp_id(__LINE__,a,b,c,d)
static void _get_disp_id(unsigned line, IDispatchEx *dispex, const WCHAR *name, HRESULT exhres, DISPID *id)
{
    DISPID id2;
    BSTR str;
    HRESULT hres;

    str = SysAllocString(name);
    hres = IDispatchEx_GetDispID(dispex, str, 0, id);
    ok_(__FILE__,line)(hres == exhres, "GetDispID(%s) returned %08lx, expected %08lx\n",
            wine_dbgstr_w(name), hres, exhres);

    hres = IDispatchEx_GetIDsOfNames(dispex, &IID_NULL, &str, 1, 0, &id2);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == exhres, "GetIDsOfNames(%s) returned %08lx, expected %08lx\n",
            wine_dbgstr_w(name), hres, exhres);
    ok_(__FILE__,line)(*id == id2, "GetIDsOfNames(%s) id != id2\n", wine_dbgstr_w(name));
}

static void test_no_script_dispatch(IActiveScript *script)
{
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)0xdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    ok(hres == E_UNEXPECTED, "hres = %08lx, expected E_UNEXPECTED\n", hres);
    ok(!disp, "disp != NULL\n");
}

static IActiveScript *create_vbscript(void)
{
    IActiveScript *ret;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&ret);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);

    return ret;
}

static void test_scriptdisp(void)
{
    IDispatchEx *script_disp, *script_disp2;
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    DISPID id, id2;
    DISPPARAMS dp;
    EXCEPINFO ei;
    VARIANT v;
    ULONG ref;
    HRESULT hres;

    vbscript = create_vbscript();

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08lx\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);

    script_disp2 = get_script_dispatch(vbscript, NULL);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    script_disp = get_script_dispatch(vbscript, NULL);
    ok(script_disp == script_disp2, "script_disp != script_disp2\n");
    IDispatchEx_Release(script_disp2);

    id = 100;
    get_disp_id(script_disp, L"LCase", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);

    get_disp_id(script_disp, L"globalVariable", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, L"dim globalVariable\nglobalVariable = 3");
    get_disp_id(script_disp, L"globalVariable", S_OK, &id);

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(script_disp, id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 3, "V_I2(v) = %d\n", V_I2(&v));

    get_disp_id(script_disp, L"globalVariable2", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, L"globalVariable2 = 4");
    get_disp_id(script_disp, L"globalVariable2", S_OK, &id);

    get_disp_id(script_disp, L"globalFunction", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, L"function globalFunction()\nglobalFunction=5\nend function");
    get_disp_id(script_disp, L"globalFunction", S_OK, &id);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(script_disp, id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 5, "V_I2(v) = %d\n", V_I2(&v));

    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_Invoke(script_disp, id, &IID_NULL, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 5, "V_I2(v) = %d\n", V_I2(&v));

    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    get_disp_id(script_disp, L"globalSub", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, L"sub globalSub()\nend sub");
    get_disp_id(script_disp, L"globalSub", S_OK, &id);
    get_disp_id(script_disp, L"globalSub", S_OK, &id2);
    ok(id == id2, "id != id2\n");

    get_disp_id(script_disp, L"constVariable", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, L"const constVariable = 6");
    get_disp_id(script_disp, L"ConstVariable", S_OK, &id);
    get_disp_id(script_disp, L"Constvariable", S_OK, &id2);
    ok(id == id2, "id != id2\n");

    IDispatchEx_Release(script_disp);

    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(vbscript);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %ld\n", ref);
}

static void test_param_ids(void)
{
    static const WCHAR *const names1[] = { L"test", L"c", L"foo", L"b", L"a" };
    static const WCHAR *const names2[] = { L"test", L"bar" };
    static const WCHAR *const names3[] = { L"bar", L"test" };
    DISPID id[ARRAY_SIZE(names1)];
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    IDispatchEx *script_disp;
    HRESULT hr;

    vbscript = create_vbscript();

    hr = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hr == S_OK, "Could not get IActiveScriptParse iface: %08lx\n", hr);

    SET_EXPECT(GetLCID);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hr = IActiveScriptParse_InitNew(parser);
    ok(hr == S_OK, "InitNew failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_CONNECTED);

    parse_script(parser, L"function test(byval a, byval b, byval c, byval foo)\ntest = a + b + c - foo\nend function\n"
                         "function bar\nend function");
    script_disp = get_script_dispatch(vbscript, NULL);

    hr = IDispatchEx_GetIDsOfNames(script_disp, &IID_NULL, (WCHAR**)names1, ARRAY_SIZE(names1), 0, id);
    ok(hr == DISP_E_UNKNOWNNAME, "GetIDsOfNames returned %08lx, expected %08lx\n", hr, DISP_E_UNKNOWNNAME);
    ok(id[0] > 0, "Unexpected DISPID for \"test\": %ld\n", id[0]);
    ok(id[4] == DISPID_UNKNOWN, "Unexpected DISPID for \"a\" parameter: %ld\n", id[4]);
    ok(id[3] == DISPID_UNKNOWN, "Unexpected DISPID for \"b\" parameter: %ld\n", id[3]);
    ok(id[1] == DISPID_UNKNOWN, "Unexpected DISPID for \"c\" parameter: %ld\n", id[1]);
    ok(id[2] == DISPID_UNKNOWN, "Unexpected DISPID for \"foo\" parameter: %ld\n", id[2]);

    hr = IDispatchEx_GetIDsOfNames(script_disp, &IID_NULL, (WCHAR**)names2, ARRAY_SIZE(names2), 0, id);
    ok(hr == DISP_E_UNKNOWNNAME, "GetIDsOfNames returned %08lx, expected %08lx\n", hr, DISP_E_UNKNOWNNAME);
    ok(id[0] > 0, "Unexpected DISPID for \"test\": %ld\n", id[0]);
    ok(id[1] == DISPID_UNKNOWN, "Unexpected DISPID for \"bar\": %ld\n", id[1]);

    hr = IDispatchEx_GetIDsOfNames(script_disp, &IID_NULL, (WCHAR**)names3, ARRAY_SIZE(names3), 0, id);
    ok(hr == DISP_E_UNKNOWNNAME, "GetIDsOfNames returned %08lx, expected %08lx\n", hr, DISP_E_UNKNOWNNAME);
    ok(id[0] > 0, "Unexpected DISPID for \"bar\": %ld\n", id[0]);
    ok(id[1] == DISPID_UNKNOWN, "Unexpected DISPID for \"test\": %ld\n", id[1]);

    IDispatchEx_Release(script_disp);
    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hr = IActiveScript_Close(vbscript);
    ok(hr == S_OK, "Close failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    IActiveScript_Release(vbscript);
}

static void test_code_persistence(void)
{
    IActiveScriptParse *parser;
    IDispatchEx *script_disp;
    IActiveScript *vbscript;
    VARIANT var;
    HRESULT hr;
    DISPID id;
    ULONG ref;

    vbscript = create_vbscript();

    hr = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hr == S_OK, "Could not get IActiveScriptParse iface: %08lx\n", hr);
    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hr = IActiveScriptParse_InitNew(parser);
    ok(hr == S_OK, "InitNew failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    hr = IActiveScriptParse_ParseScriptText(parser, L""
        "x = 1\n"
        "dim y\ny = 2\n",
        NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);

    hr = IActiveScriptParse_ParseScriptText(parser, L""
        "dim z\n"
        "y = 42\n"
        "var = 10\n",
        NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);

    /* Pending code does not add identifiers to the global scope */
    script_disp = get_script_dispatch(vbscript, NULL);
    id = 0;
    get_disp_id(script_disp, L"x", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    id = 0;
    get_disp_id(script_disp, L"y", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    id = 0;
    get_disp_id(script_disp, L"z", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    IDispatchEx_Release(script_disp);

    /* Uninitialized state removes code without SCRIPTTEXT_ISPERSISTENT */
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_UNINITIALIZED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);
    test_no_script_dispatch(vbscript);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hr = IActiveScriptParse_ParseScriptText(parser, L"var = 20\n", NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);

    SET_EXPECT(OnStateChange_CONNECTED);
    SET_EXPECT_MULTI(OnEnterScript, 2);
    SET_EXPECT_MULTI(OnLeaveScript, 2);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_CONNECTED);
    CHECK_CALLED_MULTI(OnEnterScript, 2);
    CHECK_CALLED_MULTI(OnLeaveScript, 2);
    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    script_disp = get_script_dispatch(vbscript, NULL);
    id = 0;
    get_disp_id(script_disp, L"x", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    id = 0;
    get_disp_id(script_disp, L"y", S_OK, &id);
    ok(id != -1, "id = -1\n");
    id = 0;
    get_disp_id(script_disp, L"z", S_OK, &id);
    ok(id != -1, "id = -1\n");
    IDispatchEx_Release(script_disp);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hr = IActiveScriptParse_ParseScriptText(parser, L"y", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);
    ok(V_VT(&var) == VT_I2 && V_I2(&var) == 42, "V_VT(y) = %d, V_I2(y) = %d\n", V_VT(&var), V_I2(&var));
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hr = IActiveScriptParse_ParseScriptText(parser, L"var", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);
    ok(V_VT(&var) == VT_I2 && V_I2(&var) == 20, "V_VT(var) = %d, V_I2(var) = %d\n", V_VT(&var), V_I2(&var));
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    /* Uninitialized state does not remove persistent code, even if it was executed */
    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_UNINITIALIZED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);
    test_no_script_dispatch(vbscript);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    script_disp = get_script_dispatch(vbscript, NULL);
    id = 0;
    get_disp_id(script_disp, L"z", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    IDispatchEx_Release(script_disp);

    SET_EXPECT(OnStateChange_CONNECTED);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_CONNECTED);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    script_disp = get_script_dispatch(vbscript, NULL);
    id = 0;
    get_disp_id(script_disp, L"z", S_OK, &id);
    ok(id != -1, "id = -1\n");
    IDispatchEx_Release(script_disp);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hr = IActiveScriptParse_ParseScriptText(parser, L"y", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);
    ok(V_VT(&var) == VT_I2 && V_I2(&var) == 42, "V_VT(y) = %d, V_I2(y) = %d\n", V_VT(&var), V_I2(&var));
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hr = IActiveScriptParse_ParseScriptText(parser, L"var", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);
    ok(V_VT(&var) == VT_I2 && V_I2(&var) == 10, "V_VT(var) = %d, V_I2(var) = %d\n", V_VT(&var), V_I2(&var));
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_UNINITIALIZED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hr = IActiveScriptParse_ParseScriptText(parser, L"dim y\ny = 2\n", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hr == S_OK, "ParseScriptText failed: %08lx\n", hr);

    /* Closing the script engine removes all code (even if it's pending and persistent) */
    SET_EXPECT(OnStateChange_CLOSED);
    hr = IActiveScript_Close(vbscript);
    ok(hr == S_OK, "Close failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_CLOSED);
    test_state(vbscript, SCRIPTSTATE_CLOSED);
    test_no_script_dispatch(vbscript);

    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(GetLCID);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(GetLCID);
    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_CONNECTED);
    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    script_disp = get_script_dispatch(vbscript, NULL);
    id = 0;
    get_disp_id(script_disp, L"y", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    id = 0;
    get_disp_id(script_disp, L"z", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %ld, expected -1\n", id);
    IDispatchEx_Release(script_disp);

    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %ld\n", ref);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);
}

static void test_script_typeinfo(void)
{
    static struct
    {
        const WCHAR *name;
        VARTYPE ret_type;
        UINT num_args;
    } func[] =
    {
        { L"foobar",   VT_VARIANT, 0 },
        { L"test",     VT_VOID,    0 },
        { L"subtract", VT_VARIANT, 2 },
        { L"emptyfn",  VT_VARIANT, 0 }
    };
    static struct
    {
        const WCHAR *name;
    } var[] =
    {
        { L"global_var" },
        { L"obj"        },
        { L"const_var"  },
        { L"implicit"   }
    };
    ITypeInfo *typeinfo, *typeinfo2;
    ITypeComp *typecomp, *typecomp2;
    IActiveScriptParse *parser;
    IDispatchEx *script_disp;
    IActiveScript *vbscript;
    FUNCDESC *funcdesc;
    VARDESC  *vardesc;
    DESCKIND desckind;
    INT implTypeFlags;
    UINT count, index;
    HREFTYPE reftype;
    BINDPTR bindptr;
    MEMBERID memid;
    TYPEATTR *attr;
    HRESULT hr;
    WCHAR str[64], *names = str;
    BSTR bstr, bstrs[5];
    void *obj;
    int i;

    vbscript = create_vbscript();

    hr = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hr == S_OK, "Could not get IActiveScriptParse iface: %08lx\n", hr);

    SET_EXPECT(GetLCID);
    hr = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hr == S_OK, "SetScriptSite failed: %08lx\n", hr);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hr = IActiveScriptParse_InitNew(parser);
    ok(hr == S_OK, "InitNew failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hr = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hr == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_CONNECTED);

    parse_script(parser,
        L"dim global_var\n"
        "const const_var = 1337\n"

        "function foobar\n"
        "    foobar = \"foobar\"\n"
        "end function\n"

        "sub test\nend sub\n"
        "private sub private_sub\nend sub\n"

        "function subtract(byref x, byval y)\n"
        "    subtract = x - y\n"
        "end function\n"

        "function emptyfn\nend function\n"

        "class C\n"
        "    dim x\n"
        "    public sub method\nend sub\n"
        "    private function strret\n"
        "        strret = \"ret\"\n"
        "    end function\n"
        "end class\n"

        "implicit = 10\n"
        "dim obj\nset obj = new C\n");

    script_disp = get_script_dispatch(vbscript, NULL);
    hr = IDispatchEx_QueryInterface(script_disp, &IID_ITypeInfo, (void**)&typeinfo);
    ok(hr == E_NOINTERFACE, "QueryInterface(IID_ITypeInfo) returned: %08lx\n", hr);
    hr = IDispatchEx_GetTypeInfo(script_disp, 1, LOCALE_USER_DEFAULT, &typeinfo);
    ok(hr == DISP_E_BADINDEX, "GetTypeInfo returned: %08lx\n", hr);
    hr = IDispatchEx_GetTypeInfo(script_disp, 0, LOCALE_USER_DEFAULT, &typeinfo);
    ok(hr == S_OK, "GetTypeInfo failed: %08lx\n", hr);
    hr = IDispatchEx_GetTypeInfo(script_disp, 0, LOCALE_USER_DEFAULT, &typeinfo2);
    ok(hr == S_OK, "GetTypeInfo failed: %08lx\n", hr);
    ok(typeinfo != typeinfo2, "TypeInfo was not supposed to be shared.\n");
    ITypeInfo_Release(typeinfo2);

    obj = (void*)0xdeadbeef;
    hr = ITypeInfo_CreateInstance(typeinfo, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "CreateInstance returned: %08lx\n", hr);
    hr = ITypeInfo_CreateInstance(typeinfo, NULL, NULL, &obj);
    ok(hr == TYPE_E_BADMODULEKIND, "CreateInstance returned: %08lx\n", hr);
    hr = ITypeInfo_CreateInstance(typeinfo, NULL, &IID_IDispatch, &obj);
    ok(hr == TYPE_E_BADMODULEKIND, "CreateInstance returned: %08lx\n", hr);
    ok(!obj, "Unexpected non-null obj %p.\n", obj);

    hr = ITypeInfo_GetDocumentation(typeinfo, MEMBERID_NIL, &bstr, NULL, NULL, NULL);
    ok(hr == S_OK, "GetDocumentation(MEMBERID_NIL) failed: %08lx\n", hr);
    ok(!lstrcmpW(bstr, L"VBScriptTypeInfo"), "Unexpected TypeInfo name %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
    ok(hr == S_OK, "GetTypeAttr failed: %08lx\n", hr);
    ok(IsEqualGUID(&attr->guid, &IID_IScriptTypeInfo), "Unexpected GUID %s\n", wine_dbgstr_guid(&attr->guid));
    ok(attr->lcid == LOCALE_USER_DEFAULT, "Unexpected LCID %lu\n", attr->lcid);
    ok(attr->memidConstructor == MEMBERID_NIL, "Unexpected constructor memid %lu\n", attr->memidConstructor);
    ok(attr->memidDestructor == MEMBERID_NIL, "Unexpected destructor memid %lu\n", attr->memidDestructor);
    ok(attr->cbSizeInstance == 4, "Unexpected cbSizeInstance %lu\n", attr->cbSizeInstance);
    ok(attr->typekind == TKIND_DISPATCH, "Unexpected typekind %u\n", attr->typekind);
    ok(attr->cFuncs == ARRAY_SIZE(func), "Unexpected cFuncs %u\n", attr->cFuncs);
    ok(attr->cVars == ARRAY_SIZE(var), "Unexpected cVars %u\n", attr->cVars);
    ok(attr->cImplTypes == 1, "Unexpected cImplTypes %u\n", attr->cImplTypes);
    ok(attr->cbSizeVft == sizeof(IDispatchVtbl), "Unexpected cbSizeVft %u\n", attr->cbSizeVft);
    ok(attr->cbAlignment == 4, "Unexpected cbAlignment %u\n", attr->cbAlignment);
    ok(attr->wTypeFlags == TYPEFLAG_FDISPATCHABLE, "Unexpected wTypeFlags 0x%x\n", attr->wTypeFlags);
    ok(attr->tdescAlias.vt == VT_EMPTY, "Unexpected tdescAlias.vt %d\n", attr->tdescAlias.vt);
    ok(attr->idldescType.wIDLFlags == IDLFLAG_NONE, "Unexpected idldescType.wIDLFlags 0x%x\n", attr->idldescType.wIDLFlags);
    ITypeInfo_ReleaseTypeAttr(typeinfo, attr);

    /* The type inherits from IDispatch */
    hr = ITypeInfo_GetImplTypeFlags(typeinfo, 0, NULL);
    ok(hr == E_INVALIDARG, "GetImplTypeFlags returned: %08lx\n", hr);
    hr = ITypeInfo_GetImplTypeFlags(typeinfo, 1, &implTypeFlags);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetImplTypeFlags returned: %08lx\n", hr);
    hr = ITypeInfo_GetImplTypeFlags(typeinfo, -1, &implTypeFlags);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetImplTypeFlags returned: %08lx\n", hr);
    hr = ITypeInfo_GetImplTypeFlags(typeinfo, 0, &implTypeFlags);
    ok(hr == S_OK, "GetImplTypeFlags failed: %08lx\n", hr);
    ok(implTypeFlags == 0, "Unexpected implTypeFlags 0x%x\n", implTypeFlags);

    hr = ITypeInfo_GetRefTypeOfImplType(typeinfo, 0, NULL);
    ok(hr == E_INVALIDARG, "GetRefTypeOfImplType returned: %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeOfImplType(typeinfo, 1, &reftype);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetRefTypeOfImplType returned: %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeOfImplType(typeinfo, -1, &reftype);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetRefTypeOfImplType failed: %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeOfImplType(typeinfo, 0, &reftype);
    ok(hr == S_OK, "GetRefTypeOfImplType failed: %08lx\n", hr);
    ok(reftype == 1, "Unexpected reftype %ld\n", reftype);

    hr = ITypeInfo_GetRefTypeInfo(typeinfo, reftype, NULL);
    ok(hr == E_INVALIDARG, "GetRefTypeInfo returned: %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(typeinfo, -1, &typeinfo2);
    ok(hr == E_INVALIDARG, "GetRefTypeInfo returned: %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(typeinfo, 4, &typeinfo2);
    ok(hr == E_FAIL, "GetRefTypeInfo returned: %08lx\n", hr);
    hr = ITypeInfo_GetRefTypeInfo(typeinfo, 0, &typeinfo2);
    ok(hr == S_OK, "GetRefTypeInfo failed: %08lx\n", hr);
    ok(typeinfo == typeinfo2, "Unexpected TypeInfo %p (expected %p)\n", typeinfo2, typeinfo);
    ITypeInfo_Release(typeinfo2);
    hr = ITypeInfo_GetRefTypeInfo(typeinfo, reftype, &typeinfo2);
    ok(hr == S_OK, "GetRefTypeInfo failed: %08lx\n", hr);
    hr = ITypeInfo_GetDocumentation(typeinfo2, MEMBERID_NIL, &bstr, NULL, NULL, NULL);
    ok(hr == S_OK, "GetDocumentation(MEMBERID_NIL) failed: %08lx\n", hr);
    ok(!lstrcmpW(bstr, L"IDispatch"), "Unexpected TypeInfo name %s\n", wine_dbgstr_w(bstr));
    ITypeInfo_Release(typeinfo2);
    SysFreeString(bstr);

    /* GetIDsOfNames looks into the inherited types as well */
    wcscpy(str, L"queryinterface");
    hr = ITypeInfo_GetIDsOfNames(typeinfo, NULL, 1, &memid);
    ok(hr == E_INVALIDARG, "GetIDsOfNames returned: %08lx\n", hr);
    hr = ITypeInfo_GetIDsOfNames(typeinfo, &names, 1, NULL);
    ok(hr == E_INVALIDARG, "GetIDsOfNames returned: %08lx\n", hr);
    hr = ITypeInfo_GetIDsOfNames(typeinfo, &names, 0, &memid);
    ok(hr == E_INVALIDARG, "GetIDsOfNames returned: %08lx\n", hr);
    hr = ITypeInfo_GetIDsOfNames(typeinfo, &names, 1, &memid);
    ok(hr == S_OK, "GetIDsOfNames failed: %08lx\n", hr);
    ok(!lstrcmpW(str, L"queryinterface"), "Unexpected string %s\n", wine_dbgstr_w(str));
    wcscpy(str, L"C");
    hr = ITypeInfo_GetIDsOfNames(typeinfo, &names, 1, &memid);
    ok(hr == DISP_E_UNKNOWNNAME, "GetIDsOfNames returned: %08lx\n", hr);
    wcscpy(str, L"SUBtract");
    hr = ITypeInfo_GetIDsOfNames(typeinfo, &names, 1, &memid);
    ok(hr == S_OK, "GetIDsOfNames failed: %08lx\n", hr);
    ok(!lstrcmpW(str, L"SUBtract"), "Unexpected string %s\n", wine_dbgstr_w(str));

    hr = ITypeInfo_GetNames(typeinfo, memid, NULL, 1, &count);
    ok(hr == E_INVALIDARG, "GetNames returned: %08lx\n", hr);
    hr = ITypeInfo_GetNames(typeinfo, memid, bstrs, 1, NULL);
    ok(hr == E_INVALIDARG, "GetNames returned: %08lx\n", hr);
    hr = ITypeInfo_GetNames(typeinfo, memid, bstrs, 0, &count);
    ok(hr == S_OK, "GetNames failed: %08lx\n", hr);
    ok(count == 0, "Unexpected count %u\n", count);
    hr = ITypeInfo_GetNames(typeinfo, memid, bstrs, ARRAY_SIZE(bstrs), &count);
    ok(hr == S_OK, "GetNames failed: %08lx\n", hr);
    ok(count == 3, "Unexpected count %u\n", count);
    ok(!lstrcmpW(bstrs[0], L"subtract"), "Unexpected function name %s\n", wine_dbgstr_w(bstrs[0]));
    ok(!lstrcmpW(bstrs[1], L"x"), "Unexpected function first param name %s\n", wine_dbgstr_w(bstrs[1]));
    ok(!lstrcmpW(bstrs[2], L"y"), "Unexpected function second param name %s\n", wine_dbgstr_w(bstrs[2]));
    for (i = 0; i < count; i++) SysFreeString(bstrs[i]);

    hr = ITypeInfo_GetMops(typeinfo, memid, NULL);
    ok(hr == E_INVALIDARG, "GetMops returned: %08lx\n", hr);
    hr = ITypeInfo_GetMops(typeinfo, memid, &bstr);
    ok(hr == S_OK, "GetMops failed: %08lx\n", hr);
    ok(!bstr, "Unexpected non-null string %s\n", wine_dbgstr_w(bstr));
    hr = ITypeInfo_GetMops(typeinfo, MEMBERID_NIL, &bstr);
    ok(hr == S_OK, "GetMops failed: %08lx\n", hr);
    ok(!bstr, "Unexpected non-null string %s\n", wine_dbgstr_w(bstr));

    /* These always fail */
    obj = (void*)0xdeadbeef;
    hr = ITypeInfo_AddressOfMember(typeinfo, memid, INVOKE_FUNC, NULL);
    ok(hr == E_INVALIDARG, "AddressOfMember returned: %08lx\n", hr);
    hr = ITypeInfo_AddressOfMember(typeinfo, memid, INVOKE_FUNC, &obj);
    ok(hr == TYPE_E_BADMODULEKIND, "AddressOfMember returned: %08lx\n", hr);
    ok(!obj, "Unexpected non-null obj %p.\n", obj);
    bstr = (BSTR)0xdeadbeef;
    hr = ITypeInfo_GetDllEntry(typeinfo, memid, INVOKE_FUNC, &bstr, NULL, NULL);
    ok(hr == TYPE_E_BADMODULEKIND, "GetDllEntry returned: %08lx\n", hr);
    ok(!bstr, "Unexpected non-null str %p.\n", bstr);
    wcscpy(str, L"Invoke");
    hr = ITypeInfo_GetIDsOfNames(typeinfo, &names, 1, &memid);
    ok(hr == S_OK, "GetIDsOfNames failed: %08lx\n", hr);
    obj = (void*)0xdeadbeef;
    hr = ITypeInfo_AddressOfMember(typeinfo, memid, INVOKE_FUNC, &obj);
    ok(hr == TYPE_E_BADMODULEKIND, "AddressOfMember returned: %08lx\n", hr);
    ok(!obj, "Unexpected non-null obj %p.\n", obj);
    bstr = (BSTR)0xdeadbeef;
    hr = ITypeInfo_GetDllEntry(typeinfo, memid, INVOKE_FUNC, &bstr, NULL, NULL);
    ok(hr == TYPE_E_BADMODULEKIND, "GetDllEntry returned: %08lx\n", hr);
    ok(!bstr, "Unexpected non-null str %p.\n", bstr);

    /* Check variable descriptions */
    hr = ITypeInfo_GetVarDesc(typeinfo, 0, NULL);
    ok(hr == E_INVALIDARG, "GetVarDesc returned: %08lx\n", hr);
    hr = ITypeInfo_GetVarDesc(typeinfo, 1337, &vardesc);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetVarDesc returned: %08lx\n", hr);
    for (i = 0; i < ARRAY_SIZE(var); i++)
    {
        hr = ITypeInfo_GetVarDesc(typeinfo, i, &vardesc);
        ok(hr == S_OK, "GetVarDesc(%u) failed: %08lx\n", i, hr);
        hr = ITypeInfo_GetDocumentation(typeinfo, vardesc->memid, &bstr, &bstrs[0], NULL, NULL);
        ok(hr == S_OK, "[%u] GetDocumentation failed: %08lx\n", i, hr);
        ok(!lstrcmpW(bstr, var[i].name), "[%u] Unexpected variable name %s (expected %s)\n",
            i, wine_dbgstr_w(bstr), wine_dbgstr_w(var[i].name));
        ok(!bstrs[0], "[%u] Unexpected doc string %s\n", i, wine_dbgstr_w(bstrs[0]));
        SysFreeString(bstr);
        ok(vardesc->lpstrSchema == NULL, "[%u] Unexpected lpstrSchema %p\n", i, vardesc->lpstrSchema);
        ok(vardesc->oInst == 0, "[%u] Unexpected oInst %lu\n", i, vardesc->oInst);
        ok(vardesc->varkind == VAR_DISPATCH, "[%u] Unexpected varkind %d\n", i, vardesc->varkind);
        ok(vardesc->wVarFlags == 0, "[%u] Unexpected wVarFlags 0x%x\n", i, vardesc->wVarFlags);
        ok(vardesc->elemdescVar.tdesc.vt == VT_VARIANT,
            "[%u] Unexpected variable type vt %d (expected %d)\n", i, vardesc->elemdescVar.tdesc.vt, 0);
        ok(vardesc->elemdescVar.paramdesc.pparamdescex == NULL,
            "[%u] Unexpected variable type pparamdescex %p\n", i, vardesc->elemdescVar.paramdesc.pparamdescex);
        ok(vardesc->elemdescVar.paramdesc.wParamFlags == PARAMFLAG_NONE,
            "[%u] Unexpected variable type wParamFlags 0x%x\n", i, vardesc->elemdescVar.paramdesc.wParamFlags);
        ITypeInfo_ReleaseVarDesc(typeinfo, vardesc);
    }

    /* Check function descriptions */
    hr = ITypeInfo_GetFuncDesc(typeinfo, 0, NULL);
    ok(hr == E_INVALIDARG, "GetFuncDesc returned: %08lx\n", hr);
    hr = ITypeInfo_GetFuncDesc(typeinfo, 1337, &funcdesc);
    ok(hr == TYPE_E_ELEMENTNOTFOUND, "GetFuncDesc returned: %08lx\n", hr);
    for (i = 0; i < ARRAY_SIZE(func); i++)
    {
        hr = ITypeInfo_GetFuncDesc(typeinfo, i, &funcdesc);
        ok(hr == S_OK, "GetFuncDesc(%u) failed: %08lx\n", i, hr);
        hr = ITypeInfo_GetDocumentation(typeinfo, funcdesc->memid, &bstr, &bstrs[0], NULL, NULL);
        ok(hr == S_OK, "[%u] GetDocumentation failed: %08lx\n", i, hr);
        ok(!lstrcmpW(bstr, func[i].name), "[%u] Unexpected function name %s (expected %s)\n",
            i, wine_dbgstr_w(bstr), wine_dbgstr_w(func[i].name));
        ok(!bstrs[0], "[%u] Unexpected doc string %s\n", i, wine_dbgstr_w(bstrs[0]));
        SysFreeString(bstr);
        ok(funcdesc->lprgscode == NULL, "[%u] Unexpected lprgscode %p\n", i, funcdesc->lprgscode);
        ok(func[i].num_args ? (funcdesc->lprgelemdescParam != NULL) : (funcdesc->lprgelemdescParam == NULL),
            "[%u] Unexpected lprgelemdescParam %p\n", i, funcdesc->lprgelemdescParam);
        ok(funcdesc->funckind == FUNC_DISPATCH, "[%u] Unexpected funckind %u\n", i, funcdesc->funckind);
        ok(funcdesc->invkind == INVOKE_FUNC, "[%u] Unexpected invkind %u\n", i, funcdesc->invkind);
        ok(funcdesc->callconv == CC_STDCALL, "[%u] Unexpected callconv %u\n", i, funcdesc->callconv);
        ok(funcdesc->cParams == func[i].num_args, "[%u] Unexpected cParams %d (expected %d)\n",
            i, funcdesc->cParams, func[i].num_args);
        ok(funcdesc->cParamsOpt == 0, "[%u] Unexpected cParamsOpt %d\n", i, funcdesc->cParamsOpt);
        ok(funcdesc->cScodes == 0, "[%u] Unexpected cScodes %d\n", i, funcdesc->cScodes);
        ok(funcdesc->wFuncFlags == 0, "[%u] Unexpected wFuncFlags 0x%x\n", i, funcdesc->wFuncFlags);
        ok(funcdesc->elemdescFunc.tdesc.vt == func[i].ret_type,
            "[%u] Unexpected return type vt %d (expected %d)\n", i, funcdesc->elemdescFunc.tdesc.vt, func[i].ret_type);
        ok(funcdesc->elemdescFunc.paramdesc.pparamdescex == NULL,
            "[%u] Unexpected return type pparamdescex %p\n", i, funcdesc->elemdescFunc.paramdesc.pparamdescex);
        ok(funcdesc->elemdescFunc.paramdesc.wParamFlags == PARAMFLAG_NONE,
            "[%u] Unexpected return type wParamFlags 0x%x\n", i, funcdesc->elemdescFunc.paramdesc.wParamFlags);
        if (funcdesc->lprgelemdescParam)
            for (index = 0; index < funcdesc->cParams; index++)
            {
                ok(funcdesc->lprgelemdescParam[index].tdesc.vt == VT_VARIANT,
                    "[%u] Unexpected parameter %u vt %d\n", i, index, funcdesc->lprgelemdescParam[index].tdesc.vt);
                ok(funcdesc->lprgelemdescParam[index].paramdesc.pparamdescex == NULL,
                    "[%u] Unexpected parameter %u pparamdescex %p\n", i, index, funcdesc->lprgelemdescParam[index].paramdesc.pparamdescex);
                ok(funcdesc->lprgelemdescParam[index].paramdesc.wParamFlags == PARAMFLAG_NONE,
                    "[%u] Unexpected parameter %u wParamFlags 0x%x\n", i, index, funcdesc->lprgelemdescParam[index].paramdesc.wParamFlags);
            }
        ITypeInfo_ReleaseFuncDesc(typeinfo, funcdesc);
    }

    /* Test TypeComp Binds */
    hr = ITypeInfo_QueryInterface(typeinfo, &IID_ITypeComp, (void**)&typecomp);
    ok(hr == S_OK, "QueryInterface(IID_ITypeComp) failed: %08lx\n", hr);
    hr = ITypeInfo_GetTypeComp(typeinfo, NULL);
    ok(hr == E_INVALIDARG, "GetTypeComp returned: %08lx\n", hr);
    hr = ITypeInfo_GetTypeComp(typeinfo, &typecomp2);
    ok(hr == S_OK, "GetTypeComp failed: %08lx\n", hr);
    ok(typecomp == typecomp2, "QueryInterface(IID_ITypeComp) and GetTypeComp returned different TypeComps\n");
    ITypeComp_Release(typecomp2);
    wcscpy(str, L"not_found");
    hr = ITypeComp_Bind(typecomp, NULL, 0, 0, &typeinfo2, &desckind, &bindptr);
    ok(hr == E_INVALIDARG, "Bind returned: %08lx\n", hr);
    hr = ITypeComp_Bind(typecomp, str, 0, 0, NULL, &desckind, &bindptr);
    ok(hr == E_INVALIDARG, "Bind returned: %08lx\n", hr);
    hr = ITypeComp_Bind(typecomp, str, 0, 0, &typeinfo2, NULL, &bindptr);
    ok(hr == E_INVALIDARG, "Bind returned: %08lx\n", hr);
    hr = ITypeComp_Bind(typecomp, str, 0, 0, &typeinfo2, &desckind, NULL);
    ok(hr == E_INVALIDARG, "Bind returned: %08lx\n", hr);

    hr = ITypeComp_Bind(typecomp, str, 0, 0, &typeinfo2, &desckind, &bindptr);
    ok(hr == S_OK, "Bind failed: %08lx\n", hr);
    ok(desckind == DESCKIND_NONE, "Unexpected desckind %u\n", desckind);
    wcscpy(str, L"GLOBAL_VAR");
    hr = ITypeComp_Bind(typecomp, str, 0, INVOKE_FUNC, &typeinfo2, &desckind, &bindptr);
    ok(hr == TYPE_E_TYPEMISMATCH, "Bind returned: %08lx\n", hr);
    ok(!lstrcmpW(str, L"GLOBAL_VAR"), "Unexpected string %s\n", wine_dbgstr_w(str));
    wcscpy(str, L"C");
    hr = ITypeComp_Bind(typecomp, str, 0, 0, &typeinfo2, &desckind, &bindptr);
    ok(hr == S_OK, "Bind failed: %08lx\n", hr);
    ok(desckind == DESCKIND_NONE, "Unexpected desckind %u\n", desckind);
    wcscpy(str, L"addRef");
    hr = ITypeComp_Bind(typecomp, str, 0, 0, &typeinfo2, &desckind, &bindptr);
    ok(hr == S_OK, "Bind failed: %08lx\n", hr);
    ok(desckind == DESCKIND_FUNCDESC, "Unexpected desckind %u\n", desckind);
    ok(!lstrcmpW(str, L"addRef"), "Unexpected string %s\n", wine_dbgstr_w(str));
    ITypeInfo_ReleaseFuncDesc(typeinfo2, bindptr.lpfuncdesc);
    ITypeInfo_Release(typeinfo2);
    for (i = 0; i < ARRAY_SIZE(var); i++)
    {
        wcscpy(str, var[i].name);
        hr = ITypeComp_Bind(typecomp, str, 0, INVOKE_PROPERTYGET, &typeinfo2, &desckind, &bindptr);
        ok(hr == S_OK, "Bind failed: %08lx\n", hr);
        ok(desckind == DESCKIND_VARDESC, "Unexpected desckind %u\n", desckind);
        ITypeInfo_ReleaseVarDesc(typeinfo2, bindptr.lpvardesc);
        ITypeInfo_Release(typeinfo2);
    }
    for (i = 0; i < ARRAY_SIZE(func); i++)
    {
        wcscpy(str, func[i].name);
        hr = ITypeComp_Bind(typecomp, str, 0, INVOKE_FUNC, &typeinfo2, &desckind, &bindptr);
        ok(hr == S_OK, "Bind failed: %08lx\n", hr);
        ok(desckind == DESCKIND_FUNCDESC, "Unexpected desckind %u\n", desckind);
        ITypeInfo_ReleaseFuncDesc(typeinfo2, bindptr.lpfuncdesc);
        ITypeInfo_Release(typeinfo2);
    }
    wcscpy(str, L"VBScriptTypeInfo");
    hr = ITypeComp_BindType(typecomp, NULL, 0, &typeinfo2, &typecomp2);
    ok(hr == E_INVALIDARG, "BindType returned: %08lx\n", hr);
    hr = ITypeComp_BindType(typecomp, str, 0, NULL, &typecomp2);
    ok(hr == E_INVALIDARG, "BindType returned: %08lx\n", hr);
    hr = ITypeComp_BindType(typecomp, str, 0, &typeinfo2, NULL);
    ok(hr == E_INVALIDARG, "BindType returned: %08lx\n", hr);
    hr = ITypeComp_BindType(typecomp, str, 0, &typeinfo2, &typecomp2);
    ok(hr == S_OK, "BindType failed: %08lx\n", hr);
    ok(!typeinfo2, "Unexpected TypeInfo %p (expected null)\n", typeinfo2);
    ok(!typecomp2, "Unexpected TypeComp %p (expected null)\n", typecomp2);
    wcscpy(str, L"C");
    hr = ITypeComp_BindType(typecomp, str, 0, &typeinfo2, &typecomp2);
    ok(hr == S_OK, "BindType failed: %08lx\n", hr);
    ok(!typeinfo2, "Unexpected TypeInfo %p (expected null)\n", typeinfo2);
    ok(!typecomp2, "Unexpected TypeComp %p (expected null)\n", typecomp2);
    wcscpy(str, L"IDispatch");
    hr = ITypeComp_BindType(typecomp, str, 0, &typeinfo2, &typecomp2);
    ok(hr == S_OK, "BindType failed: %08lx\n", hr);
    ok(!typeinfo2, "Unexpected TypeInfo %p (expected null)\n", typeinfo2);
    ok(!typecomp2, "Unexpected TypeComp %p (expected null)\n", typecomp2);

    ITypeComp_Release(typecomp);

    /* Updating the script won't update the typeinfo obtained before,
       but it will be reflected in any typeinfo obtained afterwards. */
    parse_script(parser,
        L"dim new_var\nnew_var = 10\n"
        "sub new_sub\nend sub\n"

        /* Replace the function foobar with more args */
        "function foobar(x, y, z)\nend function\n");

    hr = IDispatchEx_GetTypeInfo(script_disp, 0, LOCALE_USER_DEFAULT, &typeinfo2);
    ok(hr == S_OK, "GetTypeInfo failed: %08lx\n", hr);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &attr);
    ok(hr == S_OK, "GetTypeAttr failed: %08lx\n", hr);
    ok(attr->cFuncs == ARRAY_SIZE(func), "Unexpected cFuncs %u\n", attr->cFuncs);
    ok(attr->cVars == ARRAY_SIZE(var), "Unexpected cVars %u\n", attr->cVars);
    ITypeInfo_ReleaseTypeAttr(typeinfo, attr);
    hr = ITypeInfo_GetTypeAttr(typeinfo2, &attr);
    ok(hr == S_OK, "GetTypeAttr failed: %08lx\n", hr);
    ok(attr->cFuncs == ARRAY_SIZE(func) + 1, "Unexpected cFuncs %u\n", attr->cFuncs);
    ok(attr->cVars == ARRAY_SIZE(var) + 1, "Unexpected cVars %u\n", attr->cVars);
    ITypeInfo_ReleaseTypeAttr(typeinfo2, attr);
    hr = ITypeInfo_GetVarDesc(typeinfo2, ARRAY_SIZE(var), &vardesc);
    ok(hr == S_OK, "GetVarDesc failed: %08lx\n", hr);
    hr = ITypeInfo_GetDocumentation(typeinfo2, vardesc->memid, &bstr, &bstrs[0], NULL, NULL);
    ok(hr == S_OK, "GetDocumentation failed: %08lx\n", hr);
    ok(!lstrcmpW(bstr, L"new_var"), "Unexpected variable name %s\n", wine_dbgstr_w(bstr));
    ok(!bstrs[0], "Unexpected doc string %s\n", wine_dbgstr_w(bstrs[0]));
    ITypeInfo_ReleaseVarDesc(typeinfo2, vardesc);
    SysFreeString(bstr);
    hr = ITypeInfo_GetFuncDesc(typeinfo, 0, &funcdesc);
    ok(hr == S_OK, "GetFuncDesc failed: %08lx\n", hr);
    ok(funcdesc->cParams == 0, "Unexpected cParams %d\n", funcdesc->cParams);
    ITypeInfo_ReleaseFuncDesc(typeinfo, funcdesc);
    hr = ITypeInfo_GetFuncDesc(typeinfo2, 0, &funcdesc);
    ok(hr == S_OK, "GetFuncDesc failed: %08lx\n", hr);
    ok(funcdesc->cParams == 3, "Unexpected cParams %d\n", funcdesc->cParams);
    ITypeInfo_ReleaseFuncDesc(typeinfo2, funcdesc);
    ITypeInfo_Release(typeinfo2);

    ITypeInfo_Release(typeinfo);
    IDispatchEx_Release(script_disp);
    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hr = IActiveScript_Close(vbscript);
    ok(hr == S_OK, "Close failed: %08lx\n", hr);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    IActiveScript_Release(vbscript);
}

static void test_vbscript(void)
{
    IActiveScriptParseProcedure2 *parse_proc;
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    ULONG ref;
    HRESULT hres;

    vbscript = create_vbscript();

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08lx\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);
    test_no_script_dispatch(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08lx, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(vbscript);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(vbscript, SCRIPTSTATE_CLOSED);
    test_no_script_dispatch(vbscript);

    IActiveScriptParse_Release(parser);

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParseProcedure, (void**)&parse_proc);
    ok(hres == E_NOINTERFACE, "Got IActiveScriptParseProcedure interface, expected E_NOTIMPL\n");

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Could not get IActiveScriptParseProcedure2 interface\n");
    IActiveScriptParseProcedure2_Release(parse_proc);

    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %ld\n", ref);
}

static void test_vbscript_uninitializing(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    IDispatchEx *dispex;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);

    test_no_script_dispatch(script);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08lx\n", hres);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_INITIALIZED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_INITIALIZED) failed: %08lx\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScriptParse_ParseScriptText(parse, L"Function f\nEnd Function\n", NULL, NULL, NULL, 0, 1, 0x42, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08lx, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    test_state(script, SCRIPTSTATE_CONNECTED);

    dispex = get_script_dispatch(script, NULL);
    ok(dispex != NULL, "dispex == NULL\n");
    if(dispex)
        IDispatchEx_Release(dispex);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx, expected E_UNEXPECTED\n", hres);

    test_state(script, SCRIPTSTATE_CLOSED);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_STARTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_STARTED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_INITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_INITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08lx\n", hres);

    /* initialize again and use SetScriptState(SCRIPTSTATE_CLOSED) to uninitialize it */

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CLOSED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CLOSED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CLOSED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CLOSED) failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_INITIALIZED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_INITIALIZED) failed: %08lx\n", hres);

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CLOSED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CLOSED) failed: %08lx\n", hres);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_INITIALIZED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_INITIALIZED) failed: %08lx\n", hres);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %ld\n", ref);

    script = create_vbscript();

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CLOSED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_CLOSED) failed: %08lx\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %ld\n", ref);
}

static void test_vbscript_release(void)
{
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    ULONG ref;
    HRESULT hres;

    vbscript = create_vbscript();

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08lx\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %ld\n", ref);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);
}

static void test_vbscript_simplecreate(void)
{
    IActiveScript *script;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %ld\n", ref);
}

static void test_vbscript_initializing(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08lx, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %ld\n", ref);
}

static void test_named_items(void)
{
    static const WCHAR *global_idents[] =
    {
        L"global_me",
        L"globalCode_me",
        L"testSub_global",
        L"testExplicitVar_global",
        L"testVar_global"
    };
    static const WCHAR *global_code_test[] =
    {
        L"testSub_global\n",
        L"if testExplicitVar_global <> 10 then err.raise 500\n",
        L"if testVar_global <> 5 then err.raise 500\n",
        L"set x = new testClass_global\n"
    };
    static const WCHAR *context_idents[] =
    {
        L"testSub",
        L"testExplicitVar",
        L"testVar"
    };
    static const WCHAR *context_code_test[] =
    {
        L"testSub\n",
        L"if testExplicitVar <> 42 then err.raise 500\n",
        L"if testVar <> 99 then err.raise 500\n",
        L"set x = new testClass\n"
    };
    IDispatchEx *script_disp, *script_disp2;
    IActiveScriptParse *parse;
    IActiveScript *script;
    IDispatch *disp;
    VARIANT var;
    unsigned i;
    DISPID id;
    ULONG ref;
    BSTR bstr;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08lx\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_AddNamedItem(script, L"visibleItem", SCRIPTITEM_ISVISIBLE);
    ok(hres == E_UNEXPECTED, "AddNamedItem returned: %08lx\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"globalItem", SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == E_UNEXPECTED, "AddNamedItem returned: %08lx\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"codeOnlyItem", SCRIPTITEM_CODEONLY);
    ok(hres == E_UNEXPECTED, "AddNamedItem returned: %08lx\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"persistent", SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_CODEONLY);
    ok(hres == E_UNEXPECTED, "AddNamedItem returned: %08lx\n", hres);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(GetItemInfo_global);
    hres = IActiveScript_AddNamedItem(script, L"globalItem", SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    CHECK_CALLED(GetItemInfo_global);

    hres = IActiveScript_AddNamedItem(script, L"visibleItem", SCRIPTITEM_ISVISIBLE);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"visibleCodeItem", SCRIPTITEM_ISVISIBLE | SCRIPTITEM_CODEONLY);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"codeOnlyItem", SCRIPTITEM_CODEONLY);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"persistent", SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_CODEONLY);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    ok(global_named_item_ref > 0, "global_named_item_ref = %lu\n", global_named_item_ref);
    ok(visible_named_item_ref == 0, "visible_named_item_ref = %lu\n", visible_named_item_ref);
    ok(visible_code_named_item_ref == 0, "visible_code_named_item_ref = %lu\n", visible_code_named_item_ref);
    ok(persistent_named_item_ref == 0, "persistent_named_item_ref = %lu\n", persistent_named_item_ref);

    hres = IActiveScript_GetScriptDispatch(script, L"noContext", &disp);
    ok(hres == E_INVALIDARG, "GetScriptDispatch returned: %08lx\n", hres);

    SET_EXPECT(GetItemInfo_global_code);
    hres = IActiveScript_AddNamedItem(script, L"globalCodeItem", SCRIPTITEM_GLOBALMEMBERS | SCRIPTITEM_CODEONLY);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);
    CHECK_CALLED(GetItemInfo_global_code);

    script_disp = get_script_dispatch(script, NULL);
    script_disp2 = get_script_dispatch(script, L"globalItem");
    ok(script_disp == script_disp2, "get_script_dispatch returned different dispatch objects.\n");
    IDispatchEx_Release(script_disp2);
    script_disp2 = get_script_dispatch(script, L"globalCodeItem");
    ok(script_disp == script_disp2, "get_script_dispatch returned different dispatch objects.\n");
    IDispatchEx_Release(script_disp2);
    script_disp2 = get_script_dispatch(script, L"codeONLYitem");
    ok(script_disp != script_disp2, "get_script_dispatch returned same dispatch objects.\n");

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    SET_EXPECT(testCall);
    parse_script(parse, L"testCall\n");
    CHECK_CALLED(testCall);

    SET_EXPECT(GetItemInfo_visible);
    SET_EXPECT(testCall);
    parse_script(parse, L"visibleItem.testCall\n");
    CHECK_CALLED(GetItemInfo_visible);
    CHECK_CALLED(testCall);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(testCall);
    hres = IActiveScriptParse_ParseScriptText(parse, L"testCall\n", L"visibleCodeItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(testCall);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(GetIDsOfNames);
    SET_EXPECT(OnScriptError);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"codeOnlyItem\n", L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(FAILED(hres), "ParseScriptText returned: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(GetIDsOfNames);
    CHECK_CALLED(OnScriptError);
    CHECK_CALLED(OnLeaveScript);

    hres = IActiveScript_GetScriptDispatch(script, L"visibleCodeItem", &disp);
    ok(hres == S_OK, "GetScriptDispatch returned: %08lx\n", hres);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"me", L"visibleCodeItem", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == disp,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    IDispatch_Release(disp);

    SET_EXPECT(GetItemInfo_visible_code);
    SET_EXPECT(testCall);
    parse_script(parse, L"visibleCodeItem.testCall\n");
    CHECK_CALLED(GetItemInfo_visible_code);
    CHECK_CALLED(testCall);

    ok(global_named_item_ref > 0, "global_named_item_ref = %lu\n", global_named_item_ref);
    ok(visible_named_item_ref == 1, "visible_named_item_ref = %lu\n", visible_named_item_ref);
    ok(visible_code_named_item_ref == 1, "visible_code_named_item_ref = %lu\n", visible_code_named_item_ref);
    ok(persistent_named_item_ref == 0, "persistent_named_item_ref = %lu\n", persistent_named_item_ref);

    SET_EXPECT(testCall);
    parse_script(parse, L"visibleItem.testCall\n");
    CHECK_CALLED(testCall);

    hres = IActiveScriptParse_ParseScriptText(parse, L"sub testSub\nend sub\n", L"noContext", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == E_INVALIDARG, "ParseScriptText returned: %08lx\n", hres);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(GetIDsOfNames);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L""
        "dim global_me\nglobal_me = 0\n"
        "dim globalCode_me\nglobalCode_me = 0\n"
        "sub testSub_global\nend sub\n"
        "dim testExplicitVar_global\ntestExplicitVar_global = 10\n"
        "testVar_global = 10\n"
        "class testClass_global\nend class\n",
        NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(GetIDsOfNames);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L""
        "sub testSub\nend sub\n"
        "dim testExplicitVar\ntestExplicitVar = 42\n"
        "class testClass\nend class\n",
        L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(GetIDsOfNames);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L""
        "testVar = 99\n"
        "testVar_global = 5\n",
        L"CodeOnlyITEM", NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(GetIDsOfNames);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(GetItemInfo_visible);
    SET_EXPECT(GetIDsOfNames_visible);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"dim abc\n", L"visibleItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    todo_wine CHECK_CALLED(GetItemInfo_visible);
    todo_wine CHECK_CALLED(GetIDsOfNames_visible);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"abc = 5\n", L"visibleItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(GetIDsOfNames_visible);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"testVar_global = 5\n", L"visibleItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(GetIDsOfNames_visible);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"dim abc\ntestVar_global = 5\n", L"visibleCodeItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"set global_me = me\n", L"globalItem", NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"set globalCode_me = me\n", L"globalCodeItem", NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    for (i = 0; i < ARRAY_SIZE(global_idents); i++)
    {
        bstr = SysAllocString(global_idents[i]);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp, bstr, 0, &id);
        ok(hres == S_OK, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(global_idents[i]), hres);
        ok(id != -1, "[%s] id = -1\n", wine_dbgstr_w(global_idents[i]));
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp2, bstr, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(global_idents[i]), hres);
        ok(id == -1, "[%s] id = %ld, expected -1\n", wine_dbgstr_w(global_idents[i]), id);
        SysFreeString(bstr);
    }
    for (i = 0; i < ARRAY_SIZE(context_idents); i++)
    {
        bstr = SysAllocString(context_idents[i]);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp, bstr, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(context_idents[i]), hres);
        ok(id == -1, "[%s] id = %ld, expected -1\n", wine_dbgstr_w(context_idents[i]), id);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp2, bstr, 0, &id);
        ok(hres == S_OK, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(context_idents[i]), hres);
        ok(id != -1, "[%s] id = -1\n", wine_dbgstr_w(context_idents[i]));
        SysFreeString(bstr);
    }

    for (i = 0; i < ARRAY_SIZE(global_code_test); i++)
    {
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, global_code_test[i], NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(hres == S_OK, "ParseScriptText(%s) failed: %08lx\n", wine_dbgstr_w(global_code_test[i]), hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, global_code_test[i], L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(hres == S_OK, "ParseScriptText(%s) failed: %08lx\n", wine_dbgstr_w(global_code_test[i]), hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
    }
    for (i = 0; i < ARRAY_SIZE(context_code_test); i++)
    {
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(OnScriptError);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, context_code_test[i], NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(FAILED(hres), "ParseScriptText(%s) returned: %08lx\n", wine_dbgstr_w(context_code_test[i]), hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnScriptError);
        CHECK_CALLED(OnLeaveScript);
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(GetIDsOfNames);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, context_code_test[i], L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(hres == S_OK, "ParseScriptText(%s) failed: %08lx\n", wine_dbgstr_w(context_code_test[i]), hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
    }
    SET_EXPECT(OnScriptError);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"testSub_global = 10\n", L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(FAILED(hres), "ParseScriptText returned: %08lx\n", hres);
    CHECK_CALLED(OnScriptError);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"me", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == &global_named_item,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"me", L"globalItem", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == &global_named_item,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"me", L"codeOnlyItem", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == (IDispatch*)script_disp2,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"globalCode_me", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == &global_named_item,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    IDispatchEx_Release(script_disp2);
    IDispatchEx_Release(script_disp);

    script_disp = get_script_dispatch(script, L"persistent");
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"me", L"persistent", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == (IDispatch*)script_disp,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    IDispatchEx_Release(script_disp);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"x = 13\n", L"persistent", NULL, NULL, 0, 0, SCRIPTTEXT_ISPERSISTENT, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"x = 10\n", L"persistent", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"x", L"persistent", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I2 && V_I2(&var) == 10, "Unexpected 'x': V_VT = %d, V_I2 = %d\n", V_VT(&var), V_I2(&var));
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    script_disp = get_script_dispatch(script, L"persistent");

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);
    test_no_script_dispatch(script);

    ok(global_named_item_ref == 0, "global_named_item_ref = %lu\n", global_named_item_ref);
    ok(visible_named_item_ref == 0, "visible_named_item_ref = %lu\n", visible_named_item_ref);
    ok(visible_code_named_item_ref == 0, "visible_code_named_item_ref = %lu\n", visible_code_named_item_ref);
    ok(persistent_named_item_ref == 0, "persistent_named_item_ref = %lu\n", persistent_named_item_ref);

    hres = IActiveScript_GetScriptDispatch(script, L"codeOnlyItem", &disp);
    ok(hres == E_UNEXPECTED, "hres = %08lx, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(GetItemInfo_persistent);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08lx\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(GetItemInfo_persistent);
    ok(persistent_named_item_ref > 0, "persistent_named_item_ref = %lu\n", persistent_named_item_ref);

    hres = IActiveScript_AddNamedItem(script, L"codeOnlyItem", SCRIPTITEM_CODEONLY);
    ok(hres == S_OK, "AddNamedItem failed: %08lx\n", hres);

    SET_EXPECT(OnStateChange_CONNECTED);
    SET_EXPECT_MULTI(OnEnterScript, 5);
    SET_EXPECT_MULTI(OnLeaveScript, 5);
    SET_EXPECT(GetIDsOfNames_persistent);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);
    CHECK_CALLED_MULTI(OnEnterScript, 5);
    CHECK_CALLED_MULTI(OnLeaveScript, 5);
    CHECK_CALLED(GetIDsOfNames_persistent);
    test_state(script, SCRIPTSTATE_CONNECTED);

    script_disp2 = get_script_dispatch(script, L"persistent");
    ok(script_disp != script_disp2, "Same script dispatch returned for \"persistent\" named item\n");
    IDispatchEx_Release(script_disp2);
    IDispatchEx_Release(script_disp);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"x", L"persistent", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I2 && V_I2(&var) == 13, "Unexpected 'x': V_VT = %d, V_I2 = %d\n", V_VT(&var), V_I2(&var));
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    /* This object is set to named item when persistent items are re-initialized, even for CODEONLY items */
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"me", L"persistent", NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == &persistent_named_item,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    /* Lookups also query named items */
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(GetIDsOfNames_persistent);
    hres = IActiveScriptParse_ParseScriptText(parse, L"abc123 = 10\n", L"persistent", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(GetIDsOfNames_persistent);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    SET_EXPECT(GetIDsOfNames_persistent);
    SET_EXPECT(OnScriptError);
    hres = IActiveScriptParse_ParseScriptText(parse, L"testCall\n", L"persistent", NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(FAILED(hres), "ParseScriptText returned: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    CHECK_CALLED(GetIDsOfNames_persistent);
    CHECK_CALLED(OnScriptError);

    script_disp = get_script_dispatch(script, NULL);
    for (i = 0; i < ARRAY_SIZE(global_idents); i++)
    {
        bstr = SysAllocString(global_idents[i]);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp, bstr, 0, &id);
        ok(hres == S_OK, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(global_idents[i]), hres);
        ok(id != -1, "[%s] id = -1\n", wine_dbgstr_w(global_idents[i]));
        SysFreeString(bstr);
    }
    for (i = 0; i < ARRAY_SIZE(context_idents); i++)
    {
        bstr = SysAllocString(context_idents[i]);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp, bstr, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(context_idents[i]), hres);
        ok(id == -1, "[%s] id = %ld, expected -1\n", wine_dbgstr_w(context_idents[i]), id);
        SysFreeString(bstr);
    }
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"global_me", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == (IDispatch*)script_disp,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"globalCode_me", NULL, NULL, NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, &var, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH && V_DISPATCH(&var) == (IDispatch*)script_disp,
        "Unexpected 'me': V_VT = %d, V_DISPATCH = %p\n", V_VT(&var), V_DISPATCH(&var));
    VariantClear(&var);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScriptParse_ParseScriptText(parse, L"global_me = 0\nglobalCode_me = 0\n", NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08lx\n", hres);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
    IDispatchEx_Release(script_disp);

    script_disp = get_script_dispatch(script, L"codeOnlyItem");
    for (i = 0; i < ARRAY_SIZE(global_idents); i++)
    {
        bstr = SysAllocString(global_idents[i]);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp, bstr, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(global_idents[i]), hres);
        ok(id == -1, "[%s] id = %ld, expected -1\n", wine_dbgstr_w(global_idents[i]), id);
        SysFreeString(bstr);
    }
    for (i = 0; i < ARRAY_SIZE(context_idents); i++)
    {
        bstr = SysAllocString(context_idents[i]);
        id = 0;
        hres = IDispatchEx_GetDispID(script_disp, bstr, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) returned %08lx\n", wine_dbgstr_w(context_idents[i]), hres);
        ok(id == -1, "[%s] id = %ld, expected -1\n", wine_dbgstr_w(context_idents[i]), id);
        SysFreeString(bstr);
    }
    IDispatchEx_Release(script_disp);

    for (i = 0; i < ARRAY_SIZE(global_code_test); i++)
    {
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, global_code_test[i], NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(hres == S_OK, "ParseScriptText(%s) failed: %08lx\n", wine_dbgstr_w(global_code_test[i]), hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, global_code_test[i], L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(hres == S_OK, "ParseScriptText(%s) failed: %08lx\n", wine_dbgstr_w(global_code_test[i]), hres);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
    }
    for (i = 0; i < ARRAY_SIZE(context_code_test); i++)
    {
        SET_EXPECT(OnScriptError);
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, context_code_test[i], NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(FAILED(hres), "ParseScriptText(%s) returned: %08lx\n", wine_dbgstr_w(context_code_test[i]), hres);
        CHECK_CALLED(OnScriptError);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
        SET_EXPECT(OnScriptError);
        SET_EXPECT(OnEnterScript);
        SET_EXPECT(OnLeaveScript);
        hres = IActiveScriptParse_ParseScriptText(parse, context_code_test[i], L"codeOnlyItem", NULL, NULL, 0, 0, 0, NULL, NULL);
        ok(FAILED(hres), "ParseScriptText(%s) returned: %08lx\n", wine_dbgstr_w(context_code_test[i]), hres);
        CHECK_CALLED(OnScriptError);
        CHECK_CALLED(OnEnterScript);
        CHECK_CALLED(OnLeaveScript);
    }

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08lx\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    ok(global_named_item_ref == 0, "global_named_item_ref = %lu\n", global_named_item_ref);
    ok(visible_named_item_ref == 0, "visible_named_item_ref = %lu\n", visible_named_item_ref);
    ok(visible_code_named_item_ref == 0, "visible_code_named_item_ref = %lu\n", visible_code_named_item_ref);
    ok(persistent_named_item_ref == 0, "persistent_named_item_ref = %lu\n", persistent_named_item_ref);

    test_state(script, SCRIPTSTATE_CLOSED);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %ld\n", ref);
}

static void test_RegExp(void)
{
    IRegExp2 *regexp;
    IMatchCollection2 *mc;
    IMatch2 *match;
    ISubMatches *sm;
    IEnumVARIANT *ev;
    IUnknown *unk;
    IDispatch *disp;
    HRESULT hres;
    BSTR bstr;
    LONG count;
    VARIANT v;
    ULONG fetched;

    hres = CoCreateInstance(&CLSID_VBScriptRegExp, NULL,
            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if(hres == REGDB_E_CLASSNOTREG) {
        win_skip("VBScriptRegExp is not registered\n");
        return;
    }
    ok(hres == S_OK, "CoCreateInstance(CLSID_VBScriptRegExp) failed: %lx\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IRegExp2, (void**)&regexp);
    if(hres == E_NOINTERFACE) {
        win_skip("IRegExp2 interface is not available\n");
        return;
    }
    ok(hres == S_OK, "QueryInterface(IID_IRegExp2) failed: %lx\n", hres);
    IUnknown_Release(unk);

    hres = IRegExp2_QueryInterface(regexp, &IID_IRegExp, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IRegExp) returned %lx\n", hres);
    IUnknown_Release(unk);

    hres = IRegExp2_QueryInterface(regexp, &IID_IDispatchEx, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IDispatchEx) returned %lx\n", hres);

    hres = IRegExp2_get_Pattern(regexp, &bstr);
    ok(bstr == NULL, "bstr != NULL\n");
    ok(hres == S_OK, "get_Pattern returned %lx, expected S_OK\n", hres);

    hres = IRegExp2_get_Pattern(regexp, NULL);
    ok(hres == E_POINTER, "get_Pattern returned %lx, expected E_POINTER\n", hres);

    hres = IRegExp2_get_IgnoreCase(regexp, NULL);
    ok(hres == E_POINTER, "get_IgnoreCase returned %lx, expected E_POINTER\n", hres);

    hres = IRegExp2_get_Global(regexp, NULL);
    ok(hres == E_POINTER, "get_Global returned %lx, expected E_POINTER\n", hres);

    hres = IRegExp2_Execute(regexp, NULL, &disp);
    ok(hres == S_OK, "Execute returned %lx, expected S_OK\n", hres);
    hres = IDispatch_QueryInterface(disp, &IID_IMatchCollection2, (void**)&mc);
    ok(hres == S_OK, "QueryInterface(IID_IMatchCollection2) returned %lx\n", hres);
    IDispatch_Release(disp);

    hres = IMatchCollection2_QueryInterface(mc, &IID_IMatchCollection, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IMatchCollection) returned %lx\n", hres);
    IUnknown_Release(unk);

    hres = IMatchCollection2_get_Count(mc, NULL);
    ok(hres == E_POINTER, "get_Count returned %lx, expected E_POINTER\n", hres);

    hres = IMatchCollection2_get_Count(mc, &count);
    ok(hres == S_OK, "get_Count returned %lx, expected S_OK\n", hres);
    ok(count == 1, "count = %ld\n", count);

    hres = IMatchCollection2_get_Item(mc, 1, &disp);
    ok(hres == E_INVALIDARG, "get_Item returned %lx, expected E_INVALIDARG\n", hres);

    hres = IMatchCollection2_get_Item(mc, 1, NULL);
    ok(hres == E_POINTER, "get_Item returned %lx, expected E_POINTER\n", hres);

    hres = IMatchCollection2_get_Item(mc, 0, &disp);
    ok(hres == S_OK, "get_Item returned %lx, expected S_OK\n", hres);
    hres = IDispatch_QueryInterface(disp, &IID_IMatch2, (void**)&match);
    ok(hres == S_OK, "QueryInterface(IID_IMatch2) returned %lx\n", hres);
    IDispatch_Release(disp);

    hres = IMatch2_QueryInterface(match, &IID_IMatch, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IMatch) returned %lx\n", hres);
    IUnknown_Release(unk);

    hres = IMatch2_get_Value(match, NULL);
    ok(hres == E_POINTER, "get_Value returned %lx, expected E_POINTER\n", hres);

    hres = IMatch2_get_FirstIndex(match, NULL);
    ok(hres == E_POINTER, "get_FirstIndex returned %lx, expected E_POINTER\n", hres);

    hres = IMatch2_get_Length(match, NULL);
    ok(hres == E_POINTER, "get_Length returned %lx, expected E_POINTER\n", hres);

    hres = IMatch2_get_SubMatches(match, NULL);
    ok(hres == E_POINTER, "get_SubMatches returned %lx, expected E_POINTER\n", hres);

    hres = IMatch2_get_SubMatches(match, &disp);
    ok(hres == S_OK, "get_SubMatches returned %lx, expected S_OK\n", hres);
    IMatch2_Release(match);
    hres = IDispatch_QueryInterface(disp, &IID_ISubMatches, (void**)&sm);
    ok(hres == S_OK, "QueryInterface(IID_ISubMatches) returned %lx\n", hres);
    IDispatch_Release(disp);

    hres = ISubMatches_get_Item(sm, 0, &v);
    ok(hres == E_INVALIDARG, "get_Item returned %lx, expected E_INVALIDARG\n", hres);

    hres = ISubMatches_get_Item(sm, 0, NULL);
    ok(hres == E_POINTER, "get_Item returned %lx, expected E_POINTER\n", hres);

    hres = ISubMatches_get_Count(sm, NULL);
    ok(hres == E_POINTER, "get_Count returned %lx, expected E_POINTER\n", hres);
    ISubMatches_Release(sm);

    hres = IMatchCollection2_get__NewEnum(mc, &unk);
    ok(hres == S_OK, "get__NewEnum returned %lx, expected S_OK\n", hres);
    hres = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (void**)&ev);
    ok(hres == S_OK, "QueryInterface(IID_IEnumVARIANT) returned %lx\n", hres);
    IUnknown_Release(unk);
    IMatchCollection2_Release(mc);

    hres = IEnumVARIANT_Skip(ev, 2);
    ok(hres == S_OK, "Skip returned %lx\n", hres);

    hres = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hres == S_FALSE, "Next returned %lx, expected S_FALSE\n", hres);
    ok(fetched == 0, "fetched = %ld\n", fetched);

    hres = IEnumVARIANT_Skip(ev, -1);
    ok(hres == S_OK, "Skip returned %lx\n", hres);

    hres = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hres == S_OK, "Next returned %lx\n", hres);
    ok(fetched == 1, "fetched = %ld\n", fetched);
    VariantClear(&v);
    IEnumVARIANT_Release(ev);

    IRegExp2_Release(regexp);
}

static void test_RegExp_Replace(void)
{
    static const struct
    {
        const WCHAR *pattern;
        const WCHAR *replace;
        const WCHAR *source;
        const WCHAR *result;
        BOOL global;
    } test[] =
    {
        { L"abc", L"", L"123abc456", L"123456", FALSE },
        { L"abc", L"dcba", L"123abc456", L"123dcba456", FALSE },
        { L"[\r\n\t\f]+", L" ", L"\nHello\rNew\fWorld\t!", L" Hello\rNew\fWorld\t!", FALSE },
        { L"[\r\n\t\f]+", L" ", L"\nHello\rNew\fWorld\t!", L" Hello New World !", TRUE },
    };
    HRESULT hr;
    IRegExp2 *regexp;
    VARIANT var;
    BSTR str, ret;
    int i;

    hr = CoCreateInstance(&CLSID_VBScriptRegExp, NULL,
                          CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                          &IID_IRegExp2, (void **)&regexp);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("VBScriptRegExp is not registered\n");
        return;
    }
    ok(hr == S_OK, "got %#lx\n", hr);

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
        hr = IRegExp2_put_Global(regexp, test[i].global ? VARIANT_TRUE : VARIANT_FALSE);
        ok(hr == S_OK, "got %#lx\n", hr);

        str = SysAllocString(test[i].pattern);
        hr = IRegExp2_put_Pattern(regexp, str);
        ok(hr == S_OK, "got %#lx\n", hr);
        SysFreeString(str);

        str = SysAllocString(test[i].source);
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(test[i].replace);
        hr = IRegExp2_Replace(regexp, str, var, &ret);
        ok(hr == S_OK, "got %#lx\n", hr);
        ok(!wcscmp(ret, test[i].result), "got %s, expected %s\n", wine_dbgstr_w(ret),
                wine_dbgstr_w(test[i].result));
        SysFreeString(ret);
        SysFreeString(V_BSTR(&var));
        SysFreeString(str);
    }

    IRegExp2_Release(regexp);
}

static BOOL check_vbscript(void)
{
    IActiveScriptParseProcedure2 *vbscript;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScriptParseProcedure2, (void**)&vbscript);
    if(SUCCEEDED(hres))
        IActiveScriptParseProcedure2_Release(vbscript);

    return hres == S_OK;
}

START_TEST(vbscript)
{
    CoInitialize(NULL);

    if(check_vbscript()) {
        test_vbscript();
        test_vbscript_uninitializing();
        test_vbscript_release();
        test_vbscript_simplecreate();
        test_vbscript_initializing();
        test_param_ids();
        test_named_items();
        test_scriptdisp();
        test_code_persistence();
        test_script_typeinfo();
        test_RegExp();
        test_RegExp_Replace();
    }else {
        win_skip("VBScript engine not available or too old\n");
    }

    CoUninitialize();
}
