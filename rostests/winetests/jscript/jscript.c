/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include "wine/test.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static const CLSID CLSID_JScript =
    {0xf414c260,0x6ac0,0x11cf,{0xb6,0xd1,0x00,0xaa,0x00,0xbb,0xbb,0x58}};

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define SET_CALLED(func) \
    called_ ## func = TRUE

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
DEFINE_EXPECT(OnStateChange_STARTED);
DEFINE_EXPECT(OnStateChange_CONNECTED);
DEFINE_EXPECT(OnStateChange_DISCONNECTED);
DEFINE_EXPECT(OnStateChange_CLOSED);
DEFINE_EXPECT(OnStateChange_INITIALIZED);
DEFINE_EXPECT(OnEnterScript);
DEFINE_EXPECT(OnLeaveScript);

#define test_state(s,ss) _test_state(__LINE__,s,ss)
static void _test_state(unsigned line, IActiveScript *script, SCRIPTSTATE exstate)
{
    SCRIPTSTATE state = -1;
    HRESULT hres;

    hres = IActiveScript_GetScriptState(script, &state);
    ok_(__FILE__,line) (hres == S_OK, "GetScriptState failed: %08x\n", hres);
    ok_(__FILE__,line) (state == exstate, "state=%d, expected %d\n", state, exstate);
}

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

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(0, "unexpected call\n");
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

#undef ACTSCPSITE_THIS

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

static void test_script_dispatch(IActiveScript *script, BOOL initialized)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    HRESULT hres;

    disp = (void*)0xdeadbeef;
    hres = IActiveScript_GetScriptDispatch(script, NULL, &disp);
    if(!initialized) {
        ok(hres == E_UNEXPECTED, "hres = %08x, expected E_UNEXPECTED\n", hres);
        ok(!disp, "disp != NULL\n");
        return;
    }

    ok(hres == S_OK, "GetScriptDispatch failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    ok(disp != NULL, "disp == NULL\n");
    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx interface: %08x\n", hres);

    IDispatchEx_Release(dispex);
}

static void test_safety(IUnknown *unk)
{
    IObjectSafety *safety;
    DWORD supported, enabled;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IObjectSafety, (void**)&safety);
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

    IObjectSafety_Release(safety);
}

static void test_jscript(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "Could not get IActiveScript: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(script);
        return;
    }

    test_state(script, SCRIPTSTATE_UNINITIALIZED);
    test_safety(unk);

    hres = IActiveScriptParse64_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScriptParse64_InitNew(parse);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08x, expected E_UNEXPECTED\n", hres);

    hres = IActiveScript_SetScriptSite(script, NULL);
    ok(hres == E_POINTER, "SetScriptSite failed: %08x, expected E_POINTER\n", hres);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);
    test_script_dispatch(script, FALSE);

    SET_EXPECT(GetLCID);
    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    test_state(script, SCRIPTSTATE_INITIALIZED);

    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == E_UNEXPECTED, "SetScriptSite failed: %08x, expected E_UNEXPECTED\n", hres);

    test_script_dispatch(script, TRUE);

    SET_EXPECT(OnStateChange_STARTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_STARTED);

    test_state(script, SCRIPTSTATE_STARTED);

    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);
    test_script_dispatch(script, FALSE);

    IUnknown_Release(parse);
    IActiveScript_Release(script);

    ref = IUnknown_Release(unk);
    ok(!ref, "ref = %d\n", ref);
}

static void test_jscript2(void)
{
    IActiveScriptParse *parse;
    IActiveScript *script;
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_JScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "Could not get IActiveScript: %08x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IActiveScriptParse, (void**)&parse);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(script);
        return;
    }

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(GetLCID);
    hres = IActiveScript_SetScriptSite(script, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);
    CHECK_CALLED(GetLCID);

    test_state(script, SCRIPTSTATE_UNINITIALIZED);

    SET_EXPECT(OnStateChange_INITIALIZED);
    hres = IActiveScriptParse64_InitNew(parse);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_INITIALIZED);

    hres = IActiveScriptParse64_InitNew(parse);
    ok(hres == E_UNEXPECTED, "InitNew failed: %08x, expected E_UNEXPECTED\n", hres);

    SET_EXPECT(OnStateChange_CONNECTED);
    hres = IActiveScript_SetScriptState(script, SCRIPTSTATE_CONNECTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_CONNECTED) failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_CONNECTED);

    test_state(script, SCRIPTSTATE_CONNECTED);

    SET_EXPECT(OnStateChange_DISCONNECTED);
    SET_EXPECT(OnStateChange_INITIALIZED);
    SET_EXPECT(OnStateChange_CLOSED);
    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);
    CHECK_CALLED(OnStateChange_DISCONNECTED);
    CHECK_CALLED(OnStateChange_INITIALIZED);
    CHECK_CALLED(OnStateChange_CLOSED);

    test_state(script, SCRIPTSTATE_CLOSED);
    test_script_dispatch(script, FALSE);

    IUnknown_Release(parse);
    IActiveScript_Release(script);

    ref = IUnknown_Release(unk);
    ok(!ref, "ref = %d\n", ref);
}

START_TEST(jscript)
{
    CoInitialize(NULL);

    test_jscript();
    test_jscript2();

    CoUninitialize();
}
