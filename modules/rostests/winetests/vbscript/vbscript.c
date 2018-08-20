/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
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
DEFINE_EXPECT(GetItemInfo_global);
DEFINE_EXPECT(GetItemInfo_visible);
DEFINE_EXPECT(testCall);

DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4,0xb0, 0x00,0xaa,0x00,0x4a,0x55,0xe8);
DEFINE_GUID(CLSID_VBScriptRegExp, 0x3f4daca4, 0x160d, 0x11d2, 0xa8,0xe9, 0x00,0x10,0x4b,0x36,0x5c,0x9f);

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

#define test_state(s,ss) _test_state(__LINE__,s,ss)
static void _test_state(unsigned line, IActiveScript *script, SCRIPTSTATE exstate)
{
    SCRIPTSTATE state = -1;
    HRESULT hres;

    hres = IActiveScript_GetScriptState(script, &state);
    ok_(__FILE__,line) (hres == S_OK, "GetScriptState failed: %08x\n", hres);
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

static ULONG global_named_item_ref, visible_named_item_ref;

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
    ok(!wcscmp(names[0], L"testCall"), "names[0] = %s\n", wine_dbgstr_w(names[0]));
    *ids = 1;
    return S_OK;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID id, REFIID riid, LCID lcid, WORD flags,
                                      DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, UINT *err)
{
    CHECK_EXPECT(testCall);
    ok(id == 1, "id = %u\n", id);
    ok(flags == DISPATCH_METHOD, "flags = %x\n", flags);
    ok(!dp->cArgs, "cArgs = %u\n", dp->cArgs);
    ok(!res, "res = %p\n", res);
    return S_OK;
}

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
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch visible_named_item = { &visible_named_item_vtbl };

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
    ok(return_mask == SCRIPTINFO_IUNKNOWN, "return_mask = %x\n", return_mask);
    if(!wcscmp(name, L"globalItem")) {
        CHECK_EXPECT(GetItemInfo_global);
        IDispatch_AddRef(&global_named_item);
        *item_unk = (IUnknown*)&global_named_item;
        return S_OK;
    }
    if(!wcscmp(name, L"visibleItem")) {
        CHECK_EXPECT(GetItemInfo_visible);
        IDispatch_AddRef(&visible_named_item);
        *item_unk = (IUnknown*)&visible_named_item;
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
    ok(0, "unexpected call\n");
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
    ok(hres == S_OK, "Could not get IObjectSafety: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, &supported, NULL);
    ok(hres == E_POINTER, "GetInterfaceSafetyOptions failed: %08x, expected E_POINTER\n", hres);
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, NULL, &enabled);
    ok(hres == E_POINTER, "GetInterfaceSafetyOptions failed: %08x, expected E_POINTER\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_NULL, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScript, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER
                |INTERFACESAFE_FOR_UNTRUSTED_CALLER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == E_FAIL, "SetInterfaceSafetyOptions failed: %08x, expected E_FAIL\n", hres);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, INTERFACESAFE_FOR_UNTRUSTED_DATA, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == (INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER), "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    hres = IObjectSafety_SetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse,
            INTERFACE_USES_DISPEX, 0);
    ok(hres == S_OK, "SetInterfaceSafetyOptions failed: %08x\n", hres);

    supported = enabled = 0xdeadbeef;
    hres = IObjectSafety_GetInterfaceSafetyOptions(safety, &IID_IActiveScriptParse, &supported, &enabled);
    ok(hres == S_OK, "GetInterfaceSafetyOptions failed: %08x\n", hres);
    ok(supported == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "supported=%x\n", supported);
    ok(enabled == INTERFACE_USES_DISPEX, "enabled=%x\n", enabled);

    IObjectSafety_Release(safety);
}

static IDispatchEx *get_script_dispatch(IActiveScript *script)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)(ULONG_PTR)0xdeadbeefdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08x\n", hres);
    if(FAILED(hres))
        return NULL;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);
    return dispex;
}

static void parse_script(IActiveScriptParse *parse, const char *src)
{
    BSTR str;
    HRESULT hres;

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);

    str = a2bstr(src);
    hres = IActiveScriptParse_ParseScriptText(parse, str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(str);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);

    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);
}

#define get_disp_id(a,b,c,d) _get_disp_id(__LINE__,a,b,c,d)
static void _get_disp_id(unsigned line, IDispatchEx *dispex, const char *name, HRESULT exhres, DISPID *id)
{
    DISPID id2;
    BSTR str;
    HRESULT hres;

    str = a2bstr(name);
    hres = IDispatchEx_GetDispID(dispex, str, 0, id);
    ok_(__FILE__,line)(hres == exhres, "GetDispID(%s) returned %08x, expected %08x\n", name, hres, exhres);

    hres = IDispatchEx_GetIDsOfNames(dispex, &IID_NULL, &str, 1, 0, &id2);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == exhres, "GetIDsOfNames(%s) returned %08x, expected %08x\n", name, hres, exhres);
    ok_(__FILE__,line)(*id == id2, "GetIDsOfNames(%s) id != id2\n", name);
}

static void test_no_script_dispatch(IActiveScript *script)
{
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)(ULONG_PTR)0xdeadbeefdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    ok(hres == E_UNEXPECTED, "hres = %08x, expected E_UNEXPECTED\n", hres);
    ok(!disp, "disp != NULL\n");
}

static IActiveScript *create_vbscript(void)
{
    IActiveScript *ret;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&ret);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

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
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08x\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    script_disp2 = get_script_dispatch(vbscript);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    script_disp = get_script_dispatch(vbscript);
    ok(script_disp == script_disp2, "script_disp != script_disp2\n");
    IDispatchEx_Release(script_disp2);

    id = 100;
    get_disp_id(script_disp, "LCase", DISP_E_UNKNOWNNAME, &id);
    ok(id == -1, "id = %d, expected -1\n", id);

    get_disp_id(script_disp, "globalVariable", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, "dim globalVariable\nglobalVariable = 3");
    get_disp_id(script_disp, "globalVariable", S_OK, &id);

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(script_disp, id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 3, "V_I2(v) = %d\n", V_I2(&v));

    get_disp_id(script_disp, "globalVariable2", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, "globalVariable2 = 4");
    get_disp_id(script_disp, "globalVariable2", S_OK, &id);

    get_disp_id(script_disp, "globalFunction", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, "function globalFunction()\nglobalFunction=5\nend function");
    get_disp_id(script_disp, "globalFunction", S_OK, &id);

    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(script_disp, id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
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
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 5, "V_I2(v) = %d\n", V_I2(&v));

    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    get_disp_id(script_disp, "globalSub", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, "sub globalSub()\nend sub");
    get_disp_id(script_disp, "globalSub", S_OK, &id);
    get_disp_id(script_disp, "globalSub", S_OK, &id2);
    ok(id == id2, "id != id2\n");

    get_disp_id(script_disp, "constVariable", DISP_E_UNKNOWNNAME, &id);
    parse_script(parser, "const constVariable = 6");
    get_disp_id(script_disp, "ConstVariable", S_OK, &id);
    get_disp_id(script_disp, "Constvariable", S_OK, &id2);
    ok(id == id2, "id != id2\n");

    IDispatchEx_Release(script_disp);

    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(vbscript);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %d\n", ref);
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
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08x\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);
    test_no_script_dispatch(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(vbscript);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
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
    ok(!ref, "ref = %d\n", ref);
}

static void test_vbscript_uninitializing(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    IDispatchEx *dispex;
    ULONG ref;
    HRESULT hres;

    static const WCHAR script_textW[] =
        {'F','u','n','c','t','i','o','n',' ','f','\n','E','n','d',' ','F','u','n','c','t','i','o','n','\n',0};

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    test_no_script_dispatch(script);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScriptParse_ParseScriptText(parse, script_textW, NULL, NULL, NULL, 0, 1, 0x42, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    SET_EXPECT(OnEnterScript);
    SET_EXPECT(OnLeaveScript);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);
    CHECK_CALLED(OnEnterScript);
    CHECK_CALLED(OnLeaveScript);

    test_state(script, SCRIPTSTATE_CONNECTED);

    dispex = get_script_dispatch(script);
    ok(dispex != NULL, "dispex == NULL\n");
    if(dispex)
        IDispatchEx_Release(dispex);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_UNINITIALIZED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_UNINITIALIZED);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_UNINITIALIZED);
    ok(hres == E_UNEXPECTED, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x, expected E_UNEXPECTED\n", hres);

    test_state(script, SCRIPTSTATE_CLOSED);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08x\n", hres);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
}

static void test_vbscript_release(void)
{
    IActiveScriptParse *parser;
    IActiveScript *vbscript;
    ULONG ref;
    HRESULT hres;

    vbscript = create_vbscript();

    hres = IActiveScript_QueryInterface(vbscript, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse iface: %08x\n", hres);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);
    test_safety(vbscript);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(vbscript, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(vbscript, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(vbscript, SCRIPTSTATE_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(vbscript, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(vbscript, SCRIPTSTATE_CONNECTED);

    IActiveScriptParse_Release(parser);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    ref = IActiveScript_Release(vbscript);
    ok(!ref, "ref = %d\n", ref);
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
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_UNINITIALIZED) failed: %08x\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
}

static void test_vbscript_initializing(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
}

static void test_named_items(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    ULONG ref;
    HRESULT hres;

    script = create_vbscript();

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    hres = IActiveScript_AddNamedItem(script, L"visibleItem", SCRIPTITEM_ISVISIBLE);
    ok(hres == E_UNEXPECTED, "AddNamedItem returned: %08x\n", hres);
    hres = IActiveScript_AddNamedItem(script, L"globalItem", SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == E_UNEXPECTED, "AddNamedItem returned: %08x\n", hres);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    SET_EXPECT(GetItemInfo_global);
    hres = IActiveScript_AddNamedItem(script, L"globalItem", SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);
    CHECK_CALLED(GetItemInfo_global);

    hres = IActiveScript_AddNamedItem(script, L"visibleItem", SCRIPTITEM_ISVISIBLE);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    ok(global_named_item_ref > 0, "global_named_item_ref = %u\n", global_named_item_ref);
    ok(visible_named_item_ref == 0, "visible_named_item_ref = %u\n", visible_named_item_ref);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    SET_EXPECT(testCall);
    parse_script(parse, "testCall\n");
    CHECK_CALLED(testCall);

    SET_EXPECT(GetItemInfo_visible);
    SET_EXPECT(testCall);
    parse_script(parse, "visibleItem.testCall\n");
    CHECK_CALLED(GetItemInfo_visible);
    CHECK_CALLED(testCall);

    ok(global_named_item_ref > 0, "global_named_item_ref = %u\n", global_named_item_ref);
    ok(visible_named_item_ref == 1, "visible_named_item_ref = %u\n", visible_named_item_ref);

    SET_EXPECT(testCall);
    parse_script(parse, "visibleItem.testCall\n");
    CHECK_CALLED(testCall);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    ok(global_named_item_ref == 0, "global_named_item_ref = %u\n", global_named_item_ref);
    ok(visible_named_item_ref == 0, "visible_named_item_ref = %u\n", visible_named_item_ref);

    test_state(script, SCRIPTSTATE_CLOSED);

    IActiveScriptParse_Release(parse);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref = %d\n", ref);
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
    ok(hres == S_OK, "CoCreateInstance(CLSID_VBScriptRegExp) failed: %x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IRegExp2, (void**)&regexp);
    if(hres == E_NOINTERFACE) {
        win_skip("IRegExp2 interface is not available\n");
        return;
    }
    ok(hres == S_OK, "QueryInterface(IID_IRegExp2) failed: %x\n", hres);
    IUnknown_Release(unk);

    hres = IRegExp2_QueryInterface(regexp, &IID_IRegExp, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IRegExp) returned %x\n", hres);
    IUnknown_Release(unk);

    hres = IRegExp2_QueryInterface(regexp, &IID_IDispatchEx, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IDispatchEx) returned %x\n", hres);

    hres = IRegExp2_get_Pattern(regexp, &bstr);
    ok(bstr == NULL, "bstr != NULL\n");
    ok(hres == S_OK, "get_Pattern returned %x, expected S_OK\n", hres);

    hres = IRegExp2_get_Pattern(regexp, NULL);
    ok(hres == E_POINTER, "get_Pattern returned %x, expected E_POINTER\n", hres);

    hres = IRegExp2_get_IgnoreCase(regexp, NULL);
    ok(hres == E_POINTER, "get_IgnoreCase returned %x, expected E_POINTER\n", hres);

    hres = IRegExp2_get_Global(regexp, NULL);
    ok(hres == E_POINTER, "get_Global returned %x, expected E_POINTER\n", hres);

    hres = IRegExp2_Execute(regexp, NULL, &disp);
    ok(hres == S_OK, "Execute returned %x, expected S_OK\n", hres);
    hres = IDispatch_QueryInterface(disp, &IID_IMatchCollection2, (void**)&mc);
    ok(hres == S_OK, "QueryInterface(IID_IMatchCollection2) returned %x\n", hres);
    IDispatch_Release(disp);

    hres = IMatchCollection2_QueryInterface(mc, &IID_IMatchCollection, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IMatchCollection) returned %x\n", hres);
    IUnknown_Release(unk);

    hres = IMatchCollection2_get_Count(mc, NULL);
    ok(hres == E_POINTER, "get_Count returned %x, expected E_POINTER\n", hres);

    hres = IMatchCollection2_get_Count(mc, &count);
    ok(hres == S_OK, "get_Count returned %x, expected S_OK\n", hres);
    ok(count == 1, "count = %d\n", count);

    hres = IMatchCollection2_get_Item(mc, 1, &disp);
    ok(hres == E_INVALIDARG, "get_Item returned %x, expected E_INVALIDARG\n", hres);

    hres = IMatchCollection2_get_Item(mc, 1, NULL);
    ok(hres == E_POINTER, "get_Item returned %x, expected E_POINTER\n", hres);

    hres = IMatchCollection2_get_Item(mc, 0, &disp);
    ok(hres == S_OK, "get_Item returned %x, expected S_OK\n", hres);
    hres = IDispatch_QueryInterface(disp, &IID_IMatch2, (void**)&match);
    ok(hres == S_OK, "QueryInterface(IID_IMatch2) returned %x\n", hres);
    IDispatch_Release(disp);

    hres = IMatch2_QueryInterface(match, &IID_IMatch, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IMatch) returned %x\n", hres);
    IUnknown_Release(unk);

    hres = IMatch2_get_Value(match, NULL);
    ok(hres == E_POINTER, "get_Value returned %x, expected E_POINTER\n", hres);

    hres = IMatch2_get_FirstIndex(match, NULL);
    ok(hres == E_POINTER, "get_FirstIndex returned %x, expected E_POINTER\n", hres);

    hres = IMatch2_get_Length(match, NULL);
    ok(hres == E_POINTER, "get_Length returned %x, expected E_POINTER\n", hres);

    hres = IMatch2_get_SubMatches(match, NULL);
    ok(hres == E_POINTER, "get_SubMatches returned %x, expected E_POINTER\n", hres);

    hres = IMatch2_get_SubMatches(match, &disp);
    ok(hres == S_OK, "get_SubMatches returned %x, expected S_OK\n", hres);
    IMatch2_Release(match);
    hres = IDispatch_QueryInterface(disp, &IID_ISubMatches, (void**)&sm);
    ok(hres == S_OK, "QueryInterface(IID_ISubMatches) returned %x\n", hres);
    IDispatch_Release(disp);

    hres = ISubMatches_get_Item(sm, 0, &v);
    ok(hres == E_INVALIDARG, "get_Item returned %x, expected E_INVALIDARG\n", hres);

    hres = ISubMatches_get_Item(sm, 0, NULL);
    ok(hres == E_POINTER, "get_Item returned %x, expected E_POINTER\n", hres);

    hres = ISubMatches_get_Count(sm, NULL);
    ok(hres == E_POINTER, "get_Count returned %x, expected E_POINTER\n", hres);
    ISubMatches_Release(sm);

    hres = IMatchCollection2_get__NewEnum(mc, &unk);
    ok(hres == S_OK, "get__NewEnum returned %x, expected S_OK\n", hres);
    hres = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (void**)&ev);
    ok(hres == S_OK, "QueryInterface(IID_IEnumVARIANT) returned %x\n", hres);
    IUnknown_Release(unk);
    IMatchCollection2_Release(mc);

    hres = IEnumVARIANT_Skip(ev, 2);
    ok(hres == S_OK, "Skip returned %x\n", hres);

    hres = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hres == S_FALSE, "Next returned %x, expected S_FALSE\n", hres);
    ok(fetched == 0, "fetched = %d\n", fetched);

    hres = IEnumVARIANT_Skip(ev, -1);
    ok(hres == S_OK, "Skip returned %x\n", hres);

    hres = IEnumVARIANT_Next(ev, 1, &v, &fetched);
    ok(hres == S_OK, "Next returned %x\n", hres);
    ok(fetched == 1, "fetched = %d\n", fetched);
    VariantClear(&v);
    IEnumVARIANT_Release(ev);

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
        test_named_items();
        test_scriptdisp();
        test_RegExp();
    }else {
        win_skip("VBScript engine not available or too old\n");
    }

    CoUninitialize();
}
