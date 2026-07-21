/*
 * Copyright 2008-2009 Jacek Caban for CodeWeavers
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

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "shlwapi.h"
#include "wininet.h"
#include "docobj.h"
#include "dispex.h"
#include "hlink.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "initguid.h"
#include "activscp.h"
#include "activdbg.h"
#include "objsafe.h"
#include "mshtmdid.h"
#include "mshtml_test.h"

DEFINE_GUID(CLSID_IdentityUnmarshal,0x0000001b,0x0000,0x0000,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

#ifdef _WIN64

#define CTXARG_T DWORDLONG
#define IActiveScriptParseVtbl IActiveScriptParse64Vtbl
#define IActiveScriptParseProcedure2Vtbl IActiveScriptParseProcedure2_64Vtbl
#define IActiveScriptSiteDebug_Release IActiveScriptSiteDebug64_Release

#else

#define CTXARG_T DWORD
#define IActiveScriptParseVtbl IActiveScriptParse32Vtbl
#define IActiveScriptParseProcedure2Vtbl IActiveScriptParseProcedure2_32Vtbl
#define IActiveScriptSiteDebug_Release IActiveScriptSiteDebug32_Release

#endif

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

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

#define CHECK_CALLED_BROKEN(func) \
    do { \
        ok(called_ ## func || broken(!called_ ## func), "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE


DEFINE_EXPECT(CreateInstance);
DEFINE_EXPECT(CreateInstance2);
DEFINE_EXPECT(GetInterfaceSafetyOptions);
DEFINE_EXPECT(SetInterfaceSafetyOptions);
DEFINE_EXPECT(GetInterfaceSafetyOptions2);
DEFINE_EXPECT(SetInterfaceSafetyOptions2);
DEFINE_EXPECT(InitNew);
DEFINE_EXPECT(InitNew2);
DEFINE_EXPECT(Close);
DEFINE_EXPECT(Close2);
DEFINE_EXPECT(SetProperty_HACK_TRIDENTEVENTSINK);
DEFINE_EXPECT(SetProperty_INVOKEVERSIONING);
DEFINE_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
DEFINE_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_TRUE);
DEFINE_EXPECT(SetProperty2_HACK_TRIDENTEVENTSINK);
DEFINE_EXPECT(SetProperty2_INVOKEVERSIONING);
DEFINE_EXPECT(SetProperty2_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
DEFINE_EXPECT(SetScriptSite);
DEFINE_EXPECT(SetScriptSite2);
DEFINE_EXPECT(GetScriptState);
DEFINE_EXPECT(SetScriptState_STARTED);
DEFINE_EXPECT(SetScriptState_CONNECTED);
DEFINE_EXPECT(SetScriptState_DISCONNECTED);
DEFINE_EXPECT(AddNamedItem);
DEFINE_EXPECT(AddNamedItem2);
DEFINE_EXPECT(ParseScriptText_script);
DEFINE_EXPECT(ParseScriptText_script2);
DEFINE_EXPECT(ParseScriptText_execScript);
DEFINE_EXPECT(GetScriptDispatch);
DEFINE_EXPECT(funcDisp);
DEFINE_EXPECT(script_divid_d);
DEFINE_EXPECT(script_testprop_d);
DEFINE_EXPECT(script_testprop_i);
DEFINE_EXPECT(script_testprop2_d);
DEFINE_EXPECT(AXQueryInterface_IActiveScript);
DEFINE_EXPECT(AXQueryInterface_IObjectSafety);
DEFINE_EXPECT(AXGetInterfaceSafetyOptions);
DEFINE_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
DEFINE_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_data);
DEFINE_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
DEFINE_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller);
DEFINE_EXPECT(external_success);
DEFINE_EXPECT(QS_VariantConversion);
DEFINE_EXPECT(QS_IActiveScriptSite);
DEFINE_EXPECT(QS_GetCaller);
DEFINE_EXPECT(QS_IActiveScriptSite2);
DEFINE_EXPECT(QS_GetCaller2);
DEFINE_EXPECT(ChangeType_bstr);
DEFINE_EXPECT(ChangeType_dispatch);
DEFINE_EXPECT(cmdtarget_Exec);
DEFINE_EXPECT(GetTypeInfo);

#define TESTACTIVEX_CLSID "{178fc163-f585-4e24-9c13-4bb7faf80646}"

#define DISPID_SCRIPT_TESTPROP   0x100000
#define DISPID_SCRIPT_TESTPROP2  0x100001
#define DISPID_REFTEST_GET       0x100000
#define DISPID_REFTEST_REF       0x100001

#define DISPID_EXTERNAL_OK             0x300000
#define DISPID_EXTERNAL_TRACE          0x300001
#define DISPID_EXTERNAL_REPORTSUCCESS  0x300002
#define DISPID_EXTERNAL_TODO_WINE_OK   0x300003
#define DISPID_EXTERNAL_BROKEN         0x300004
#define DISPID_EXTERNAL_WIN_SKIP       0x300005
#define DISPID_EXTERNAL_WRITESTREAM    0x300006
#define DISPID_EXTERNAL_GETVT          0x300007
#define DISPID_EXTERNAL_NULL_DISP      0x300008
#define DISPID_EXTERNAL_IS_ENGLISH     0x300009
#define DISPID_EXTERNAL_LIST_SEP       0x30000A
#define DISPID_EXTERNAL_TEST_VARS      0x30000B
#define DISPID_EXTERNAL_TESTHOSTCTX    0x30000C
#define DISPID_EXTERNAL_GETMIMETYPE    0x30000D
#define DISPID_EXTERNAL_SETVIEWSIZE    0x30000E
#define DISPID_EXTERNAL_NEWREFTEST     0x30000F

static const GUID CLSID_TestScript[] = {
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x07,0x46}},
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x08,0x46}},
};
static const GUID CLSID_TestActiveX =
    {0x178fc163,0xf585,0x4e24,{0x9c,0x13,0x4b,0xb7,0xfa,0xf8,0x06,0x46}};

static DWORD main_thread_id;
static BOOL is_ie9plus, is_english;
static IHTMLDocument2 *notif_doc;
static IOleDocumentView *view;
static IDispatchEx *window_dispex;
static IHTMLDocument2 *doc_obj;
static BOOL doc_complete;
static IDispatch *script_disp;
static BOOL ax_objsafe;
static HWND container_hwnd;
static HRESULT ax_getopt_hres = S_OK, ax_setopt_dispex_hres = S_OK;
static HRESULT ax_setopt_disp_caller_hres = S_OK, ax_setopt_disp_data_hres = S_OK;
static BOOL skip_loadobject_tests;

static IActiveScriptSite *site, *site2;
static SCRIPTSTATE state, state2;

static BOOL iface_cmp(void *iface1, void *iface2)
{
    IUnknown *unk1, *unk2;

    if(iface1 == iface2)
        return TRUE;

    IUnknown_QueryInterface((IUnknown*)iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_Release(unk1);
    IUnknown_QueryInterface((IUnknown*)iface2, &IID_IUnknown, (void**)&unk2);
    IUnknown_Release(unk2);

    return unk1 == unk2;
}

static BOOL init_key(const WCHAR *key_name, const WCHAR *def_value, BOOL init)
{
    HKEY hkey;
    DWORD res;

    if(!init) {
        RegDeleteKeyW(HKEY_CLASSES_ROOT, key_name);
        return TRUE;
    }

    res = RegCreateKeyW(HKEY_CLASSES_ROOT, key_name, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    if(def_value)
        res = RegSetValueW(hkey, NULL, REG_SZ, def_value, wcslen(def_value));

    RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static BSTR get_mime_type_display_name(const WCHAR *content_type)
{
    WCHAR buffer[128], ext[128], *str, *progid;
    HKEY key, type_key = NULL;
    DWORD type, len;
    LSTATUS status;
    HRESULT hres;
    CLSID clsid;
    BSTR ret;

    status = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"MIME\\Database\\Content Type", 0, KEY_READ, &key);
    if(status != ERROR_SUCCESS)
        goto fail;

    status = RegOpenKeyExW(key, content_type, 0, KEY_QUERY_VALUE, &type_key);
    RegCloseKey(key);
    if(status != ERROR_SUCCESS)
        goto fail;

    len = sizeof(ext);
    status = RegQueryValueExW(type_key, L"Extension", NULL, &type, (BYTE*)ext, &len);
    if(status != ERROR_SUCCESS || type != REG_SZ) {
        len = sizeof(buffer);
        status = RegQueryValueExW(type_key, L"CLSID", NULL, &type, (BYTE*)buffer, &len);

        if(status != ERROR_SUCCESS || type != REG_SZ || CLSIDFromString(buffer, &clsid) != S_OK ||
           ProgIDFromCLSID(&clsid, &progid) != S_OK)
            goto fail;
    }else {
        /* For some reason w1064v1809 testbot VM uses .htm here, despite .html being set in the database */
        if(!wcscmp(ext, L".html"))
            wcscpy(ext, L".htm");
        progid = ext;
    }

    len = ARRAY_SIZE(buffer);
    str = buffer;
    for(;;) {
        hres = AssocQueryStringW(ASSOCF_NOTRUNCATE, ASSOCSTR_FRIENDLYDOCNAME, progid, NULL, str, &len);
        if(hres == S_OK && len)
            break;
        if(str != buffer)
            free(str);
        if(hres != E_POINTER) {
            if(progid != ext) {
                CoTaskMemFree(progid);
                goto fail;
            }

            /* Try from CLSID */
            len = sizeof(buffer);
            status = RegQueryValueExW(type_key, L"CLSID", NULL, &type, (BYTE*)buffer, &len);

            if(status != ERROR_SUCCESS || type != REG_SZ || CLSIDFromString(buffer, &clsid) != S_OK ||
               ProgIDFromCLSID(&clsid, &progid) != S_OK)
                goto fail;

            len = ARRAY_SIZE(buffer);
            str = buffer;
            continue;
        }
        str = malloc(len * sizeof(WCHAR));
    }
    if(progid != ext)
        CoTaskMemFree(progid);
    RegCloseKey(type_key);

    ret = SysAllocString(str);
    if(str != buffer)
        free(str);
    return ret;

fail:
    RegCloseKey(type_key);
    trace("Did not find MIME in database for %s\n", debugstr_w(content_type));
    return SysAllocString(L"File");
}

static void test_sp_caller(IServiceProvider *sp)
{
    IOleCommandTarget *cmdtarget;
    IServiceProvider *caller;
    IHTMLWindow2 *window;
    HRESULT hres;
    VARIANT var;

    hres = IServiceProvider_QueryService(sp, &SID_GetCaller, &IID_IServiceProvider, (void**)&caller);
    ok(hres == S_OK, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
    ok(!caller, "caller != NULL\n");

    hres = IServiceProvider_QueryService(sp, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&cmdtarget);
    ok(hres == S_OK, "QueryService(IActiveScriptSite->IOleCommandTarget) failed: %08lx\n", hres);
    ok(cmdtarget != NULL, "IOleCommandTarget is NULL\n");

    V_VT(&var) = VT_EMPTY;
    hres = IOleCommandTarget_Exec(cmdtarget, &CGID_ScriptSite, CMDID_SCRIPTSITE_SECURITY_WINDOW, 0, NULL, &var);
    ok(hres == S_OK, "Exec failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(CMDID_SCRIPTSITE_SECURITY_WINDOW) = %d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(CMDID_SCRIPTSITE_SECURITY_WINDOW) = NULL\n");
    IOleCommandTarget_Release(cmdtarget);

    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IHTMLWindow2, (void**)&window);
    ok(hres == S_OK, "QueryInterface(IHTMLWindow2) failed: %08lx\n", hres);
    ok(window != NULL, "window is NULL\n");
    IHTMLWindow2_Release(window);
    VariantClear(&var);
}

static void test_script_vars(unsigned argc, VARIANTARG *argv)
{
    static const WCHAR *const jsobj_names[] = { L"abc", L"foO", L"bar", L"TostRing", L"hasownpropERty" };
    IHTMLBodyElement *body;
    IDispatchEx *disp;
    DISPID id, id2;
    HRESULT hres;
    unsigned i;
    BSTR bstr;

    ok(argc == 2, "argc = %d\n", argc);
    ok(V_VT(&argv[0]) == VT_DISPATCH, "VT = %d\n", V_VT(&argv[0]));
    ok(V_VT(&argv[1]) == VT_DISPATCH, "VT = %d\n", V_VT(&argv[1]));

    /* JS object disp */
    hres = IDispatch_QueryInterface(V_DISPATCH(&argv[0]), &IID_IDispatchEx, (void**)&disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    hres = IDispatchEx_QueryInterface(disp, &IID_IHTMLBodyElement, (void**)&body);
    ok(hres == E_NOINTERFACE, "Got IHTMLBodyElement iface on JS object? %08lx\n", hres);

    for(i = 0; i < ARRAY_SIZE(jsobj_names); i++) {
        bstr = SysAllocString(jsobj_names[i]);
        hres = IDispatchEx_GetIDsOfNames(disp, &IID_NULL, &bstr, 1, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetIDsOfNames(%s) returned %08lx, expected %08lx\n", debugstr_w(bstr), hres, DISP_E_UNKNOWNNAME);

        hres = IDispatchEx_GetDispID(disp, bstr, 0, &id);
        ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(%s) returned %08lx, expected %08lx\n", debugstr_w(bstr), hres, DISP_E_UNKNOWNNAME);

        hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive, &id);
        ok(hres == S_OK, "GetDispID(%s) with fdexNameCaseInsensitive failed: %08lx\n", debugstr_w(bstr), hres);
        ok(id > 0, "Unexpected DISPID for %s: %ld\n", debugstr_w(bstr), id);
        SysFreeString(bstr);
    }

    bstr = SysAllocString(L"foo");
    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    /* Native picks one "arbitrarily" here, depending how it's laid out, so can't compare exact id */
    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);

    hres = IDispatchEx_GetIDsOfNames(disp, &IID_NULL, &bstr, 1, 0, &id2);
    ok(hres == S_OK, "GetIDsOfNames failed: %08lx\n", hres);
    ok(id == id2, "id != id2\n");

    hres = IDispatchEx_DeleteMemberByName(disp, bstr, fdexNameCaseInsensitive);
    ok(hres == S_OK, "DeleteMemberByName failed: %08lx\n", hres);

    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id == id2, "id != id2\n");

    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive | fdexNameEnsure, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id == id2, "id != id2\n");
    SysFreeString(bstr);

    bstr = SysAllocString(L"fOo");
    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id == id2, "id != id2\n");

    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive | fdexNameEnsure, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id == id2, "id != id2\n");

    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameEnsure, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id != id2, "id == id2\n");

    hres = IDispatchEx_GetDispID(disp, bstr, fdexNameCaseInsensitive | fdexNameEnsure, &id2);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(bstr);

    IDispatchEx_Release(disp);

    /* Body element disp */
    hres = IDispatch_QueryInterface(V_DISPATCH(&argv[1]), &IID_IDispatchEx, (void**)&disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    hres = IDispatchEx_QueryInterface(disp, &IID_IHTMLBodyElement, (void**)&body);
    ok(hres == S_OK, "Could not get IHTMLBodyElement iface: %08lx\n", hres);
    IHTMLBodyElement_Release(body);

    IDispatchEx_Release(disp);
}

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    return 2;
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    return 1;
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    if(dispID == DISPID_READYSTATE){
        BSTR state;
        HRESULT hres;

        static const WCHAR completeW[] = {'c','o','m','p','l','e','t','e',0};

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08lx\n", hres);

        if(!lstrcmpW(state, completeW))
            doc_complete = TRUE;

        SysFreeString(state);
    }

    return S_OK;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

static IOleCommandTarget cmdtarget;

static HRESULT WINAPI VariantChangeType_QueryInterface(IVariantChangeType *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI VariantChangeType_AddRef(IVariantChangeType *iface)
{
    return 2;
}

static ULONG WINAPI VariantChangeType_Release(IVariantChangeType *iface)
{
    return 1;
}

static HRESULT WINAPI VariantChangeType_ChangeType(IVariantChangeType *iface, VARIANT *dst, VARIANT *src, LCID lcid, VARTYPE vt)
{
    ok(dst != NULL, "dst = NULL\n");
    ok(V_VT(dst) == VT_EMPTY, "V_VT(dst) = %d\n", V_VT(dst));
    ok(src != NULL, "src = NULL\n");
    ok(lcid == LOCALE_NEUTRAL, "lcid = %ld\n", lcid);

    switch(vt) {
    case VT_BSTR:
        CHECK_EXPECT(ChangeType_bstr);
        ok(V_VT(src) == VT_I4, "V_VT(src) = %d\n", V_VT(src));
        ok(V_I4(src) == 0xf0f0f0, "V_I4(src) = %lx\n", V_I4(src));
        V_VT(dst) = VT_BSTR;
        V_BSTR(dst) = SysAllocString(L"red");
        break;
    case VT_DISPATCH:
        CHECK_EXPECT(ChangeType_dispatch);
        ok(V_VT(src) == VT_NULL, "V_VT(src) = %d\n", V_VT(src));
        /* native jscript returns E_NOTIMPL, use a "valid" error to test that it doesn't matter */
        return E_OUTOFMEMORY;
    default:
        ok(0, "unexpected vt %d\n", vt);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const IVariantChangeTypeVtbl VariantChangeTypeVtbl = {
    VariantChangeType_QueryInterface,
    VariantChangeType_AddRef,
    VariantChangeType_Release,
    VariantChangeType_ChangeType
};

static IVariantChangeType VChangeType = { &VariantChangeTypeVtbl };

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(guidService, &SID_VariantConversion)) {
        CHECK_EXPECT(QS_VariantConversion);
        ok(IsEqualGUID(riid, &IID_IVariantChangeType), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        *ppv = &VChangeType;
        return S_OK;
    }

    if(IsEqualGUID(guidService, &IID_IActiveScriptSite)) {
        CHECK_EXPECT(QS_IActiveScriptSite);
        ok(IsEqualGUID(riid, &IID_IOleCommandTarget), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        return IActiveScriptSite_QueryInterface(site, riid, ppv);
    }

    if(IsEqualGUID(guidService, &SID_GetCaller)) {
        CHECK_EXPECT(QS_GetCaller);
        ok(IsEqualGUID(riid, &IID_IServiceProvider), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider caller_sp = { &ServiceProviderVtbl };

static HRESULT WINAPI ServiceProvider2_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ServiceProvider2_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider2_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider2_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(guidService, &IID_IActiveScriptSite)) {
        CHECK_EXPECT(QS_IActiveScriptSite2);
        ok(IsEqualGUID(riid, &IID_IOleCommandTarget), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        *ppv = &cmdtarget;
        return S_OK;
    }

    if(IsEqualGUID(guidService, &SID_GetCaller)) {
        CHECK_EXPECT(QS_GetCaller2);
        ok(IsEqualGUID(riid, &IID_IServiceProvider), "unexpected riid %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return S_OK;
    }

    ok(0, "unexpected service %s\n", wine_dbgstr_guid(guidService));
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProvider2Vtbl = {
    ServiceProvider2_QueryInterface,
    ServiceProvider2_AddRef,
    ServiceProvider2_Release,
    ServiceProvider2_QueryService
};

static IServiceProvider caller_sp2 = { &ServiceProvider2Vtbl };

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    CHECK_EXPECT2(GetTypeInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call %s %lx\n", wine_dbgstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI funcDisp_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(funcDisp);

    ok(id == DISPID_VALUE, "id = %ld\n", id);
    ok(lcid == 0, "lcid = %lx\n", lcid);
    ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok(pdp != NULL, "pdp == NULL\n");
    ok(pdp->cArgs == 2, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(pdp->cNamedArgs == 1, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok(pdp->rgdispidNamedArgs[0] == DISPID_THIS, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
    ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
    ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(rgvarg[1]) = %d\n", V_VT(pdp->rgvarg));
    ok(V_BOOL(pdp->rgvarg+1) == VARIANT_TRUE, "V_BOOL(rgvarg[1]) = %x\n", V_BOOL(pdp->rgvarg));
    ok(pvarRes != NULL, "pvarRes == NULL\n");
    ok(pei != NULL, "pei == NULL\n");
    ok(!pspCaller, "pspCaller != NULL\n");

    V_VT(pvarRes) = VT_I4;
    V_I4(pvarRes) = 100;
    return S_OK;
}

static IDispatchExVtbl testObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    funcDisp_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx funcDisp = { &testObjVtbl };

static HRESULT WINAPI scriptDisp_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"testProp")) {
        CHECK_EXPECT(script_testprop_d);
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_SCRIPT_TESTPROP;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"testProp2")) {
        CHECK_EXPECT(script_testprop2_d);
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        *pid = DISPID_SCRIPT_TESTPROP2;
        return S_OK;
    }

    if(!lstrcmpW(bstrName, L"divid")) {
        CHECK_EXPECT(script_divid_d);
        ok(grfdex == fdexNameCaseSensitive, "grfdex = %lx\n", grfdex);
        return E_FAIL;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI scriptDisp_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_SCRIPT_TESTPROP:
        CHECK_EXPECT(script_testprop_i);

        ok(lcid == 0, "lcid = %lx\n", lcid);
        ok(wFlags == DISPATCH_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->cArgs == 0, "pdp->cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 0, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pdp->rgdispidNamedArgs, "pdp->rgdispidNamedArgs != NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");
        ok(!pspCaller, "pspCaller != NULL\n");

        V_VT(pvarRes) = VT_NULL;
        break;
    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static IDispatchExVtbl scriptDispVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    scriptDisp_GetDispID,
    scriptDisp_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx scriptDisp = { &scriptDispVtbl };

static HRESULT WINAPI testHostContextDisp_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(id == DISPID_VALUE, "id = %ld\n", id);
    ok(wFlags == (DISPATCH_PROPERTYGET | DISPATCH_METHOD), "wFlags = %x\n", wFlags);
    ok(pdp != NULL, "pdp == NULL\n");
    ok(pdp->cArgs == 4, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(pdp->cNamedArgs == 1, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok(pdp->rgdispidNamedArgs[0] == DISPID_THIS, "pdp->rgdispidNamedArgs[0] = %ld\n", pdp->rgdispidNamedArgs[0]);
    ok(V_VT(&pdp->rgvarg[0]) == VT_DISPATCH, "V_VT(rgvarg[0]) = %d\n", V_VT(&pdp->rgvarg[0]));
    ok(V_DISPATCH(&pdp->rgvarg[0]) != NULL, "V_DISPATCH(rgvarg[0]) = NULL\n");
    ok(V_VT(&pdp->rgvarg[1]) == VT_DISPATCH, "V_VT(rgvarg[1]) = %d\n", V_VT(&pdp->rgvarg[1]));
    ok(V_DISPATCH(&pdp->rgvarg[1]) != NULL, "V_DISPATCH(rgvarg[1]) = NULL\n");
    ok(V_VT(&pdp->rgvarg[2]) == VT_I4, "V_VT(rgvarg[2]) = %d\n", V_VT(&pdp->rgvarg[2]));
    ok(V_I4(&pdp->rgvarg[2]) == 0, "V_I4(rgvarg[2]) = %ld\n", V_I4(&pdp->rgvarg[2]));
    ok(V_VT(&pdp->rgvarg[3]) == VT_I4, "V_VT(rgvarg[3]) = %d\n", V_VT(&pdp->rgvarg[3]));
    ok(V_I4(&pdp->rgvarg[3]) == 137, "V_I4(rgvarg[3]) = %ld\n", V_I4(&pdp->rgvarg[3]));
    V_VT(pvarRes) = VT_EMPTY;
    return S_OK;
}

static IDispatchExVtbl testHostContextDispVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    testHostContextDisp_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testHostContextDisp = { &testHostContextDispVtbl };

static HRESULT WINAPI testHostContextDisp_no_this_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(id == DISPID_VALUE, "id = %ld\n", id);
    ok(wFlags == (DISPATCH_PROPERTYGET | DISPATCH_METHOD), "wFlags = %x\n", wFlags);
    ok(pdp != NULL, "pdp == NULL\n");
    ok(pdp->cArgs == 3, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(V_VT(&pdp->rgvarg[0]) == VT_DISPATCH, "V_VT(rgvarg[1]) = %d\n", V_VT(&pdp->rgvarg[1]));
    ok(V_DISPATCH(&pdp->rgvarg[0]) != NULL, "V_DISPATCH(rgvarg[1]) = NULL\n");
    ok(V_VT(&pdp->rgvarg[1]) == VT_I4, "V_VT(rgvarg[2]) = %d\n", V_VT(&pdp->rgvarg[2]));
    ok(V_I4(&pdp->rgvarg[1]) == 0, "V_I4(rgvarg[2]) = %ld\n", V_I4(&pdp->rgvarg[2]));
    ok(V_VT(&pdp->rgvarg[2]) == VT_I4, "V_VT(rgvarg[3]) = %d\n", V_VT(&pdp->rgvarg[3]));
    ok(V_I4(&pdp->rgvarg[2]) == 137, "V_I4(rgvarg[3]) = %ld\n", V_I4(&pdp->rgvarg[3]));
    ok(pvarRes != NULL, "pvarRes == NULL\n");
    ok(pei != NULL, "pei == NULL\n");
    ok(pspCaller != NULL, "pspCaller == NULL\n");
    V_VT(pvarRes) = VT_EMPTY;
    return S_OK;
}

static IDispatchExVtbl testHostContextDisp_no_this_vtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    testHostContextDisp_no_this_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testHostContextDisp_no_this = { &testHostContextDisp_no_this_vtbl };

struct refTestObj
{
    IDispatchEx IDispatchEx_iface;
    LONG ref;
};

struct refTest
{
    IDispatchEx IDispatchEx_iface;
    LONG ref;
    struct refTestObj *obj;
};

static inline struct refTestObj *refTestObj_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct refTestObj, IDispatchEx_iface);
}

static inline struct refTest *refTest_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct refTest, IDispatchEx_iface);
}

static HRESULT WINAPI refTestObj_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    struct refTestObj *This = refTestObj_from_IDispatchEx(iface);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = &This->IDispatchEx_iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IDispatchEx_AddRef(&This->IDispatchEx_iface);
    return S_OK;
}

static ULONG WINAPI refTestObj_AddRef(IDispatchEx *iface)
{
    struct refTestObj *This = refTestObj_from_IDispatchEx(iface);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI refTestObj_Release(IDispatchEx *iface)
{
    struct refTestObj *This = refTestObj_from_IDispatchEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref)
        free(This);

    return ref;
}

static IDispatchExVtbl refTestObj_vtbl = {
    refTestObj_QueryInterface,
    refTestObj_AddRef,
    refTestObj_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static HRESULT WINAPI refTest_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    struct refTest *This = refTest_from_IDispatchEx(iface);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDispatch) || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = &This->IDispatchEx_iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IDispatchEx_AddRef(&This->IDispatchEx_iface);
    return S_OK;
}

static ULONG WINAPI refTest_AddRef(IDispatchEx *iface)
{
    struct refTest *This = refTest_from_IDispatchEx(iface);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI refTest_Release(IDispatchEx *iface)
{
    struct refTest *This = refTest_from_IDispatchEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        IDispatchEx_Release(&This->obj->IDispatchEx_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI refTest_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!wcscmp(bstrName, L"get")) {
        *pid = DISPID_REFTEST_GET;
        return S_OK;
    }
    if(!wcscmp(bstrName, L"ref")) {
        *pid = DISPID_REFTEST_REF;
        return S_OK;
    }
    ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return E_NOTIMPL;
}

static HRESULT WINAPI refTest_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    struct refTest *This = refTest_from_IDispatchEx(iface);

    ok(pdp != NULL, "pdp == NULL\n");
    ok(!pdp->cArgs, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(pvarRes != NULL, "pvarRes == NULL\n");
    ok(pei != NULL, "pei == NULL\n");
    ok(pspCaller != NULL, "pspCaller == NULL\n");

    switch(id) {
    case DISPID_REFTEST_GET: {
        ok(wFlags == DISPATCH_METHOD, "DISPID_REFTEST_GET wFlags = %x\n", wFlags);
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&This->obj->IDispatchEx_iface;
        IDispatchEx_AddRef(&This->obj->IDispatchEx_iface);
        break;
    }
    case DISPID_REFTEST_REF:
        ok(wFlags == DISPATCH_PROPERTYGET, "DISPID_REFTEST_REF wFlags = %x\n", wFlags);
        V_VT(pvarRes) = VT_I4;
        V_I4(pvarRes) = This->obj->ref;
        break;

    default:
        ok(0, "id = %ld", id);
        V_VT(pvarRes) = VT_EMPTY;
        break;
    }

    return S_OK;
}

static IDispatchExVtbl refTest_vtbl = {
    refTest_QueryInterface,
    refTest_AddRef,
    refTest_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    refTest_GetDispID,
    refTest_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static HRESULT WINAPI externalDisp_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!lstrcmpW(bstrName, L"ok")) {
        *pid = DISPID_EXTERNAL_OK;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"trace")) {
        *pid = DISPID_EXTERNAL_TRACE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"reportSuccess")) {
        *pid = DISPID_EXTERNAL_REPORTSUCCESS;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"todo_wine_ok")) {
        *pid = DISPID_EXTERNAL_TODO_WINE_OK;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"broken")) {
        *pid = DISPID_EXTERNAL_BROKEN;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"win_skip")) {
        *pid = DISPID_EXTERNAL_WIN_SKIP;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"writeStream")) {
        *pid = DISPID_EXTERNAL_WRITESTREAM;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"getVT")) {
        *pid = DISPID_EXTERNAL_GETVT;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"nullDisp")) {
        *pid = DISPID_EXTERNAL_NULL_DISP;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"isEnglish")) {
        *pid = DISPID_EXTERNAL_IS_ENGLISH;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"listSeparator")) {
        *pid = DISPID_EXTERNAL_LIST_SEP;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testVars")) {
        *pid = DISPID_EXTERNAL_TEST_VARS;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"testHostContext")) {
        *pid = DISPID_EXTERNAL_TESTHOSTCTX;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"getExpectedMimeType")) {
        *pid = DISPID_EXTERNAL_GETMIMETYPE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"setViewSize")) {
        *pid = DISPID_EXTERNAL_SETVIEWSIZE;
        return S_OK;
    }
    if(!lstrcmpW(bstrName, L"newRefTest")) {
        *pid = DISPID_EXTERNAL_NEWREFTEST;
        return S_OK;
    }

    ok(0, "unexpected name %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static void stream_write(const WCHAR*,const WCHAR*);

static HRESULT WINAPI externalDisp_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_EXTERNAL_OK: {
        VARIANT *b, *m;

        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        m = pdp->rgvarg;
        if(V_VT(m) == (VT_BYREF|VT_VARIANT))
            m = V_BYREF(m);
        ok(V_VT(m) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));

        b = pdp->rgvarg+1;
        if(V_VT(b) == (VT_BYREF|VT_VARIANT))
            b = V_BYREF(b);
        ok(V_VT(b) == VT_BOOL, "V_VT(b) = %d\n", V_VT(b));

        ok(V_BOOL(b), "%s\n", wine_dbgstr_w(V_BSTR(m)));
        return S_OK;
    }

     case DISPID_EXTERNAL_TRACE:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        if(V_VT(pdp->rgvarg) == VT_BSTR)
            trace("%s\n", wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

    case DISPID_EXTERNAL_REPORTSUCCESS:
        CHECK_EXPECT(external_success);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        test_sp_caller(pspCaller);
        return S_OK;

    case DISPID_EXTERNAL_TODO_WINE_OK:
        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BOOL, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg));
        todo_wine
        ok(V_BOOL(pdp->rgvarg+1), "%s\n", wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

    case DISPID_EXTERNAL_BROKEN:
        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BOOL, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = broken(V_BOOL(pdp->rgvarg)) ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;

     case DISPID_EXTERNAL_WIN_SKIP:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        if(V_VT(pdp->rgvarg) == VT_BSTR)
            win_skip("%s\n", wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

     case DISPID_EXTERNAL_WRITESTREAM:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(pdp->rgvarg+1) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));

        stream_write(V_BSTR(pdp->rgvarg+1), V_BSTR(pdp->rgvarg));
        return S_OK;

    case DISPID_EXTERNAL_GETVT:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BSTR;
        switch(V_VT(pdp->rgvarg)) {
        case VT_EMPTY:
            V_BSTR(pvarRes) = SysAllocString(L"VT_EMPTY");
            break;
        case VT_NULL:
            V_BSTR(pvarRes) = SysAllocString(L"VT_NULL");
            break;
        case VT_I4:
            V_BSTR(pvarRes) = SysAllocString(L"VT_I4");
            break;
        case VT_R8:
            V_BSTR(pvarRes) = SysAllocString(L"VT_R8");
            break;
        case VT_BSTR:
            V_BSTR(pvarRes) = SysAllocString(L"VT_BSTR");
            break;
        case VT_DISPATCH:
            V_BSTR(pvarRes) = SysAllocString(L"VT_DISPATCH");
            break;
        case VT_BOOL:
            V_BSTR(pvarRes) = SysAllocString(L"VT_BOOL");
            break;
        case VT_DATE:
            V_BSTR(pvarRes) = SysAllocString(L"VT_DATE");
            break;
        default:
            ok(0, "unknown vt %d\n", V_VT(pdp->rgvarg));
            return E_FAIL;
        }

        return S_OK;

    case DISPID_EXTERNAL_NULL_DISP:
        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = NULL;
        return S_OK;

    case DISPID_EXTERNAL_IS_ENGLISH:
        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = is_english ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;

    case DISPID_EXTERNAL_LIST_SEP: {
        WCHAR buf[4];
        int len;

        ok(wFlags == INVOKE_PROPERTYGET, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        if(!(len = GetLocaleInfoW(GetUserDefaultLCID(), LOCALE_SLIST, buf, ARRAY_SIZE(buf))))
            buf[len++] = ',';
        else
            len--;

        V_VT(pvarRes) = VT_BSTR;
        V_BSTR(pvarRes) = SysAllocStringLen(buf, len);
        return S_OK;
    }

    case DISPID_EXTERNAL_TEST_VARS:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        test_script_vars(pdp->cArgs, pdp->rgvarg);
        return S_OK;

    case DISPID_EXTERNAL_TESTHOSTCTX:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(V_VT(pdp->rgvarg) == VT_BOOL, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)(V_BOOL(pdp->rgvarg) ? &testHostContextDisp : &testHostContextDisp_no_this);
        return S_OK;

    case DISPID_EXTERNAL_GETMIMETYPE:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(V_VT(pdp->rgvarg) == VT_BSTR, "VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");
        V_BSTR(pvarRes) = get_mime_type_display_name(V_BSTR(pdp->rgvarg));
        V_VT(pvarRes) = V_BSTR(pvarRes) ? VT_BSTR : VT_NULL;
        return S_OK;

    case DISPID_EXTERNAL_SETVIEWSIZE: {
        RECT rect = { 0 };

        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        ok(V_VT(&pdp->rgvarg[1]) == VT_I4, "width VT = %d\n", V_VT(&pdp->rgvarg[1]));
        ok(V_VT(&pdp->rgvarg[0]) == VT_I4, "height VT = %d\n", V_VT(&pdp->rgvarg[0]));

        rect.right = V_I4(&pdp->rgvarg[1]);
        rect.bottom = V_I4(&pdp->rgvarg[0]);
        return IOleDocumentView_SetRect(view, &rect);
    }

    case DISPID_EXTERNAL_NEWREFTEST: {
        struct refTest *obj = malloc(sizeof(*obj));

        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");

        obj->IDispatchEx_iface.lpVtbl = &refTest_vtbl;
        obj->ref = 1;
        obj->obj = malloc(sizeof(*obj->obj));
        obj->obj->IDispatchEx_iface.lpVtbl = &refTestObj_vtbl;
        obj->obj->ref = 1;

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&obj->IDispatchEx_iface;
        return S_OK;
    }

    default:
        ok(0, "unexpected call\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

static IDispatchExVtbl externalDispVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    externalDisp_GetDispID,
    externalDisp_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx externalDisp = { &externalDispVtbl };

static HRESULT WINAPI DispatchExStub_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static IDispatchExVtbl DispatchExStubVtbl = {
    DispatchExStub_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx DispatchExStub = { &DispatchExStubVtbl };

static HRESULT WINAPI cmdtarget_QueryInterface(IOleCommandTarget *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IOleCommandTarget))
        *ppv = &cmdtarget;
    else {
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}

static ULONG WINAPI cmdtarget_AddRef(IOleCommandTarget *iface)
{
    return 2;
}

static ULONG WINAPI cmdtarget_Release(IOleCommandTarget *iface)
{
    return 1;
}

static HRESULT WINAPI cmdtarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ok(0, "unexpected call\n");
    return OLECMDERR_E_UNKNOWNGROUP;
}

static HRESULT WINAPI cmdtarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    CHECK_EXPECT2(cmdtarget_Exec);
    ok(pguidCmdGroup && IsEqualGUID(pguidCmdGroup, &CGID_ScriptSite), "pguidCmdGroup = %s\n", wine_dbgstr_guid(pguidCmdGroup));
    ok(nCmdID == CMDID_SCRIPTSITE_SECURITY_WINDOW, "nCmdID = %lu\n", nCmdID);
    ok(!nCmdexecopt, "nCmdexecopt = %lu\n", nCmdexecopt);
    ok(!pvaIn, "pvaIn != NULL\n");
    ok(pvaOut != NULL, "pvaOut = NULL\n");

    /* Looks like native just uses this for some sort of hardcoded security check
     * without actually using the IDispatchEx interface or QI? (for ACCESSDENIED) */
    V_VT(pvaOut) = VT_DISPATCH;
    V_DISPATCH(pvaOut) = (IDispatch*)&DispatchExStub;
    return S_OK;
}

static const IOleCommandTargetVtbl cmdtarget_vtbl = {
    cmdtarget_QueryInterface,
    cmdtarget_AddRef,
    cmdtarget_Release,
    cmdtarget_QueryStatus,
    cmdtarget_Exec
};

static IOleCommandTarget cmdtarget = { &cmdtarget_vtbl };

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    return 2;
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    return 1;
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface, DWORD dwID, POINT *ppt,
        IUnknown *pcmdtReserved, IDispatch *pdicpReserved)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface, DOCHOSTUIINFO *pInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface, BOOL fEnable)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface, BOOL fActivate)
{
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface, LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface, LPMSG lpMsg,
        const GUID *pguidCmdGroup, DWORD nCmdID)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface, IDispatch **ppDispatch)
{
    *ppDispatch = (IDispatch*)&externalDisp;
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface, DWORD dwTranslate,
        OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface, IDataObject *pDO,
        IDataObject **ppPORet)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    return E_NOTIMPL;
}

static const IDocHostUIHandler2Vtbl DocHostUIHandlerVtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

static IDocHostUIHandler2 DocHostUIHandler = { &DocHostUIHandlerVtbl };

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT rect = {0,0,300,300};

    *ppFrame = &InPlaceFrame;
    *ppDoc = (IOleInPlaceUIWindow*)&InPlaceFrame;
    *lprcPosRect = rect;
    *lprcClipRect = rect;

    ok(lpFrameInfo->cb == sizeof(*lpFrameInfo), "lpFrameInfo->cb = %u, expected %u\n", lpFrameInfo->cb, (unsigned)sizeof(*lpFrameInfo));
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = container_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtant)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}

static const IOleInPlaceSiteVtbl InPlaceSiteVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange,
};

static IOleInPlaceSite InPlaceSite = { &InPlaceSiteVtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI DocumentSite_QueryInterface(IOleDocumentSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI DocumentSite_AddRef(IOleDocumentSite *iface)
{
    return 2;
}

static ULONG WINAPI DocumentSite_Release(IOleDocumentSite *iface)
{
    return 1;
}

static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    RECT rect = {0,0,300,300};
    IOleDocument *document;
    HRESULT hres;

    hres = IOleDocumentView_QueryInterface(pViewToActivate, &IID_IOleDocument, (void**)&document);
    ok(hres == S_OK, "could not get IOleDocument: %08lx\n", hres);

    hres = IOleDocument_CreateView(document, &InPlaceSite, NULL, 0, &view);
    IOleDocument_Release(document);
    ok(hres == S_OK, "CreateView failed: %08lx\n", hres);

    hres = IOleDocumentView_SetInPlaceSite(view, &InPlaceSite);
    ok(hres == S_OK, "SetInPlaceSite failed: %08lx\n", hres);

    hres = IOleDocumentView_UIActivate(view, TRUE);
    ok(hres == S_OK, "UIActivate failed: %08lx\n", hres);

    hres = IOleDocumentView_SetRect(view, &rect);
    ok(hres == S_OK, "SetRect failed: %08lx\n", hres);

    hres = IOleDocumentView_Show(view, TRUE);
    ok(hres == S_OK, "Show failed: %08lx\n", hres);

    return S_OK;
}

static const IOleDocumentSiteVtbl DocumentSiteVtbl = {
    DocumentSite_QueryInterface,
    DocumentSite_AddRef,
    DocumentSite_Release,
    DocumentSite_ActivateMe
};

static IOleDocumentSite DocumentSite = { &DocumentSiteVtbl };

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IOleWindow, riid) || IsEqualGUID(&IID_IOleInPlaceSite, riid))
        *ppv = &InPlaceSite;
    else if(IsEqualGUID(&IID_IDocHostUIHandler, riid) || IsEqualGUID(&IID_IDocHostUIHandler2, riid))
        *ppv = &DocHostUIHandler;

    return *ppv ? S_OK : E_NOINTERFACE;
}

static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
#if !defined(__i386__) && !defined(__x86_64__)
    todo_wine
#endif
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    doc_obj = SUCCEEDED(hres) ? doc : NULL;
    return doc_obj;
}

static void load_string(IHTMLDocument2 *doc, const char *str)
{
    IPersistStreamInit *init;
    IStream *stream;
    HRESULT hres;
    HGLOBAL mem;
    SIZE_T len;

    doc_complete = FALSE;
    len = strlen(str);
    mem = GlobalAlloc(0, len);
    memcpy(mem, str, len);
    hres = CreateStreamOnHGlobal(mem, TRUE, &stream);
    ok(hres == S_OK, "Failed to create a stream, hr %#lx.\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);
    ok(hres == S_OK, "Failed to get IPersistStreamInit, hr %#lx.\n", hres);

    IPersistStreamInit_Load(init, stream);
    IPersistStreamInit_Release(init);
    IStream_Release(stream);
}

static void do_advise(IHTMLDocument2 *doc, REFIID riid, IUnknown *unk_advise)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);

    notif_doc = doc;

    hres = IConnectionPoint_Advise(cp, unk_advise, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08lx\n", hres);
}

static void set_client_site(IHTMLDocument2 *doc, BOOL set)
{
    IOleObject *oleobj;
    HRESULT hres;

    if(!set && view) {
        IOleDocumentView_Show(view, FALSE);
        IOleDocumentView_CloseView(view, 0);
        IOleDocumentView_SetInPlaceSite(view, NULL);
        IOleDocumentView_Release(view);
        view = NULL;
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not et IOleObject: %08lx\n", hres);

    hres = IOleObject_SetClientSite(oleobj, set ? &ClientSite : NULL);
    ok(hres == S_OK, "SetClientSite failed: %08lx\n", hres);

    if(set) {
        IHlinkTarget *hlink;

        hres = IOleObject_QueryInterface(oleobj, &IID_IHlinkTarget, (void**)&hlink);
        ok(hres == S_OK, "Could not get IHlinkTarget iface: %08lx\n", hres);

        hres = IHlinkTarget_Navigate(hlink, 0, NULL);
        ok(hres == S_OK, "Navgate failed: %08lx\n", hres);

        IHlinkTarget_Release(hlink);
    }

    IOleObject_Release(oleobj);
}

typedef void (*domtest_t)(IHTMLDocument2*);

static void load_doc(IHTMLDocument2 *doc, const char *str)
{
    IHTMLElement *body = NULL;
    MSG msg;
    HRESULT hres;
    static const WCHAR ucPtr[] = {'b','a','c','k','g','r','o','u','n','d',0};
    DISPID dispID = -1;
    OLECHAR *name;

    load_string(doc, str);
    do_advise(doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08lx\n", hres);

    /* Check we can query for function on the IHTMLElementBody interface */
    name = (WCHAR*)ucPtr;
    hres = IHTMLElement_GetIDsOfNames(body, &IID_NULL, &name, 1, LOCALE_USER_DEFAULT, &dispID);
    ok(hres == S_OK, "GetIDsOfNames(background) failed %08lx\n", hres);
    ok(dispID == DISPID_IHTMLBODYELEMENT_BACKGROUND, "Incorrect dispID got (%ld)\n", dispID);

    IHTMLElement_Release(body);
}

static HRESULT WINAPI ObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ObjectSafety_AddRef(IObjectSafety *iface)
{
    return 2;
}

static ULONG WINAPI ObjectSafety_Release(IObjectSafety *iface)
{
    return 1;
}

static HRESULT WINAPI ObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    CHECK_EXPECT(GetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(pdwSupportedOptions != NULL, "pdwSupportedOptions == NULL\n");
    ok(pdwEnabledOptions != NULL, "pdwEnabledOptions == NULL\n");

    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER;
    *pdwEnabledOptions = INTERFACE_USES_DISPEX;

    return S_OK;
}

static HRESULT WINAPI ObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    CHECK_EXPECT(SetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));

    ok(dwOptionSetMask == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "dwOptionSetMask=%lx\n", dwOptionSetMask);
    ok(dwEnabledOptions == (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER),
       "dwEnabledOptions=%lx\n", dwOptionSetMask);

    return S_OK;
}

static const IObjectSafetyVtbl ObjectSafetyVtbl = {
    ObjectSafety_QueryInterface,
    ObjectSafety_AddRef,
    ObjectSafety_Release,
    ObjectSafety_GetInterfaceSafetyOptions,
    ObjectSafety_SetInterfaceSafetyOptions
};

static IObjectSafety ObjectSafety = { &ObjectSafetyVtbl };

static HRESULT WINAPI AXObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IActiveScript, riid)) {
        CHECK_EXPECT(AXQueryInterface_IActiveScript);
        return E_NOINTERFACE;
    }

    if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        CHECK_EXPECT2(AXQueryInterface_IObjectSafety);
        if(!ax_objsafe)
            return E_NOINTERFACE;
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI AXObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    CHECK_EXPECT(AXGetInterfaceSafetyOptions);

    ok(IsEqualGUID(&IID_IDispatchEx, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(pdwSupportedOptions != NULL, "pdwSupportedOptions == NULL\n");
    ok(pdwEnabledOptions != NULL, "pdwEnabledOptions == NULL\n");

    if(SUCCEEDED(ax_getopt_hres)) {
        *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER;
        *pdwEnabledOptions = INTERFACE_USES_DISPEX;
    }

    return ax_getopt_hres;
}

static HRESULT WINAPI AXObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        switch(dwEnabledOptions) {
        case INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACE_USES_SECURITY_MANAGER:
            CHECK_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
            break;
        case INTERFACESAFE_FOR_UNTRUSTED_CALLER:
            CHECK_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller);
            break;
        default:
            ok(0, "unexpected dwEnabledOptions %lx\n", dwEnabledOptions);
        }

        ok(dwOptionSetMask == dwEnabledOptions, "dwOptionSetMask=%lx, expected %lx\n", dwOptionSetMask, dwEnabledOptions);
        return ax_setopt_dispex_hres;
    }

    if(IsEqualGUID(&IID_IDispatch, riid)) {
        HRESULT hres;

        switch(dwEnabledOptions) {
        case INTERFACESAFE_FOR_UNTRUSTED_CALLER:
            CHECK_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
            hres = ax_setopt_disp_caller_hres;
            break;
        case INTERFACESAFE_FOR_UNTRUSTED_DATA:
            CHECK_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_data);
            hres = ax_setopt_disp_data_hres;
            break;
        default:
            ok(0, "unexpected dwEnabledOptions %lx\n", dwEnabledOptions);
            hres = E_FAIL;
        }
        ok(dwOptionSetMask == dwEnabledOptions, "dwOptionSetMask=%lx, expected %lx\n", dwOptionSetMask, dwEnabledOptions);
        return hres;
    }

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static const IObjectSafetyVtbl AXObjectSafetyVtbl = {
    AXObjectSafety_QueryInterface,
    ObjectSafety_AddRef,
    ObjectSafety_Release,
    AXObjectSafety_GetInterfaceSafetyOptions,
    AXObjectSafety_SetInterfaceSafetyOptions
};

static IObjectSafety AXObjectSafety = { &AXObjectSafetyVtbl };

static BOOL set_safe_reg(BOOL safe_call, BOOL safe_data)
{
    return init_key(L"CLSID\\"TESTACTIVEX_CLSID"\\Implemented Categories\\{7dd95801-9882-11cf-9fa9-00aa006c42c4}",
                    NULL, safe_call)
        && init_key(L"CLSID\\"TESTACTIVEX_CLSID"\\Implemented Categories\\{7dd95802-9882-11cf-9fa9-00aa006c42c4}",
                    NULL, safe_data);
}

#define check_custom_policy(a,b,c,d) _check_custom_policy(__LINE__,a,b,c,d)
static void _check_custom_policy(unsigned line, HRESULT hres, BYTE *ppolicy, DWORD policy_size, DWORD expolicy)
{
    ok_(__FILE__,line)(hres == S_OK, "QueryCusromPolicy failed: %08lx\n", hres);
    ok_(__FILE__,line)(policy_size == sizeof(DWORD), "policy_size = %ld\n", policy_size);
    ok_(__FILE__,line)(*(DWORD*)ppolicy == expolicy, "policy = %lx, expected %lx\n", *(DWORD*)ppolicy, expolicy);
    CoTaskMemFree(ppolicy);
}

static void test_security_reg(IInternetHostSecurityManager *sec_mgr, DWORD policy_caller, DWORD policy_load)
{
    struct CONFIRMSAFETY cs;
    DWORD policy_size;
    BYTE *ppolicy;
    HRESULT hres;

    cs.clsid = CLSID_TestActiveX;
    cs.pUnk = (IUnknown*)&AXObjectSafety;

    cs.dwFlags = 0;
    ax_objsafe = FALSE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    check_custom_policy(hres, ppolicy, policy_size, policy_caller);

    ax_objsafe = TRUE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_ALLOW);

    if(skip_loadobject_tests)
        return;

    cs.dwFlags = CONFIRMSAFETYACTION_LOADOBJECT;
    ax_objsafe = FALSE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    check_custom_policy(hres, ppolicy, policy_size, policy_load);

    ax_objsafe = TRUE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_data);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_data);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_ALLOW);
}

static void test_security(void)
{
    IInternetHostSecurityManager *sec_mgr, *sec_mgr2;
    IServiceProvider *sp;
    IHTMLWindow2 *window;
    IHTMLDocument2 *doc;
    DWORD policy, policy_size;
    struct CONFIRMSAFETY cs;
    BYTE *ppolicy;
    HRESULT hres;

    hres = IActiveScriptSite_QueryInterface(site, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

    hres = IServiceProvider_QueryService(sp, &SID_SInternetHostSecurityManager,
            &IID_IInternetHostSecurityManager, (void**)&sec_mgr);
    IServiceProvider_Release(sp);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);

    hres = IDispatchEx_QueryInterface(window_dispex, &IID_IHTMLWindow2, (void**)&window);
    ok(hres == S_OK, "Could not get IHTMLWindow2 iface: %08lx\n", hres);
    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);
    hres = IHTMLDocument2_QueryInterface(doc, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    IHTMLWindow2_Release(window);
    IHTMLDocument2_Release(doc);

    hres = IServiceProvider_QueryService(sp, &SID_SInternetHostSecurityManager, &IID_IInternetHostSecurityManager, (void**)&sec_mgr2);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    ok(iface_cmp(sec_mgr, sec_mgr2), "sec_mgr != sec_mgr2\n");
    IInternetHostSecurityManager_Release(sec_mgr2);
    IServiceProvider_Release(sp);

    hres = IInternetHostSecurityManager_ProcessUrlAction(sec_mgr, URLACTION_ACTIVEX_RUN, (BYTE*)&policy, sizeof(policy),
                                                         (BYTE*)&CLSID_TestActiveX, sizeof(CLSID), 0, 0);
    ok(hres == S_OK, "ProcessUrlAction failed: %08lx\n", hres);
    ok(policy == URLPOLICY_ALLOW, "policy = %lx\n", policy);

    cs.clsid = CLSID_TestActiveX;
    cs.pUnk = (IUnknown*)&AXObjectSafety;
    cs.dwFlags = 0;

    ax_objsafe = TRUE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_ALLOW);

    cs.dwFlags = CONFIRMSAFETYACTION_LOADOBJECT;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_data);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    if(called_AXSetInterfaceSafetyOptions_IDispatch_data) {
        CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_data);
    }else {
        win_skip("CONFIRMSAFETYACTION_LOADOBJECT flag not supported\n");
        skip_loadobject_tests = TRUE;
        CLEAR_CALLED(AXSetInterfaceSafetyOptions_IDispatch_data);
    }
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_ALLOW);

    cs.dwFlags = 0;
    ax_objsafe = FALSE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_DISALLOW);

    if(!skip_loadobject_tests) {
        cs.dwFlags = CONFIRMSAFETYACTION_LOADOBJECT;
        ax_objsafe = FALSE;
        SET_EXPECT(AXQueryInterface_IActiveScript);
        SET_EXPECT(AXQueryInterface_IObjectSafety);
        hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
                &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
        CHECK_CALLED(AXQueryInterface_IActiveScript);
        CHECK_CALLED(AXQueryInterface_IObjectSafety);
        check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_DISALLOW);
    }

    if(set_safe_reg(TRUE, FALSE)) {
        test_security_reg(sec_mgr, URLPOLICY_ALLOW, URLPOLICY_DISALLOW);

        set_safe_reg(FALSE, TRUE);
        test_security_reg(sec_mgr, URLPOLICY_DISALLOW, URLPOLICY_DISALLOW);

        set_safe_reg(TRUE, TRUE);
        test_security_reg(sec_mgr, URLPOLICY_ALLOW, URLPOLICY_ALLOW);

        set_safe_reg(FALSE, FALSE);
    }else {
        skip("Could not set safety registry\n");
    }

    ax_objsafe = TRUE;

    cs.dwFlags = 0;
    ax_setopt_dispex_hres = E_NOINTERFACE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_caller);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_ALLOW);

    ax_setopt_dispex_hres = E_FAIL;
    ax_setopt_disp_caller_hres = E_NOINTERFACE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_caller);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_DISALLOW);

    if(!skip_loadobject_tests) {
        cs.dwFlags = CONFIRMSAFETYACTION_LOADOBJECT;
        ax_setopt_dispex_hres = E_FAIL;
        ax_setopt_disp_caller_hres = E_NOINTERFACE;
        SET_EXPECT(AXQueryInterface_IActiveScript);
        SET_EXPECT(AXQueryInterface_IObjectSafety);
        SET_EXPECT(AXGetInterfaceSafetyOptions);
        SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
        SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
        hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
                &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
        CHECK_CALLED(AXQueryInterface_IActiveScript);
        CHECK_CALLED(AXQueryInterface_IObjectSafety);
        CHECK_CALLED(AXGetInterfaceSafetyOptions);
        CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller_secmgr);
        CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_caller);
        check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_DISALLOW);
    }

    cs.dwFlags = 0;
    ax_setopt_dispex_hres = E_FAIL;
    ax_setopt_disp_caller_hres = S_OK;
    ax_getopt_hres = E_NOINTERFACE;
    SET_EXPECT(AXQueryInterface_IActiveScript);
    SET_EXPECT(AXQueryInterface_IObjectSafety);
    SET_EXPECT(AXGetInterfaceSafetyOptions);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller);
    SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
    hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
            &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
    CHECK_CALLED(AXQueryInterface_IActiveScript);
    CHECK_CALLED(AXQueryInterface_IObjectSafety);
    CHECK_CALLED(AXGetInterfaceSafetyOptions);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller);
    CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_caller);
    check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_ALLOW);

    if(!skip_loadobject_tests) {
        cs.dwFlags = CONFIRMSAFETYACTION_LOADOBJECT;
        ax_setopt_dispex_hres = E_FAIL;
        ax_setopt_disp_caller_hres = S_OK;
        ax_setopt_disp_data_hres = E_FAIL;
        ax_getopt_hres = E_NOINTERFACE;
        SET_EXPECT(AXQueryInterface_IActiveScript);
        SET_EXPECT(AXQueryInterface_IObjectSafety);
        SET_EXPECT(AXGetInterfaceSafetyOptions);
        SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatchEx_caller);
        SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_caller);
        SET_EXPECT(AXSetInterfaceSafetyOptions_IDispatch_data);
        hres = IInternetHostSecurityManager_QueryCustomPolicy(sec_mgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
                &ppolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
        CHECK_CALLED(AXQueryInterface_IActiveScript);
        CHECK_CALLED(AXQueryInterface_IObjectSafety);
        CHECK_CALLED(AXGetInterfaceSafetyOptions);
        CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatchEx_caller);
        CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_caller);
        CHECK_CALLED(AXSetInterfaceSafetyOptions_IDispatch_data);
        check_custom_policy(hres, ppolicy, policy_size, URLPOLICY_DISALLOW);
    }

    IInternetHostSecurityManager_Release(sec_mgr);
}

static HRESULT WINAPI ActiveScriptProperty_QueryInterface(IActiveScriptProperty *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptProperty_AddRef(IActiveScriptProperty *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptProperty_Release(IActiveScriptProperty *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptProperty_GetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptProperty_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    switch(dwProperty) {
    case SCRIPTPROP_HACK_TRIDENTEVENTSINK:
        CHECK_EXPECT(SetProperty_HACK_TRIDENTEVENTSINK);
        ok(V_VT(pvarValue) == VT_BOOL, "V_VT(pvarValue)=%d\n", V_VT(pvarValue));
        ok(V_BOOL(pvarValue) == VARIANT_TRUE, "V_BOOL(pvarValue)=%x\n", V_BOOL(pvarValue));
        break;
    case SCRIPTPROP_INVOKEVERSIONING:
        CHECK_EXPECT(SetProperty_INVOKEVERSIONING);
        ok(V_VT(pvarValue) == VT_I4, "V_VT(pvarValue)=%d\n", V_VT(pvarValue));
        ok(V_I4(pvarValue) == 1, "V_I4(pvarValue)=%ld\n", V_I4(pvarValue));
        break;
    case SCRIPTPROP_ABBREVIATE_GLOBALNAME_RESOLUTION:
        if(V_BOOL(pvarValue))
            CHECK_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_TRUE);
        else
            CHECK_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
        ok(V_VT(pvarValue) == VT_BOOL, "V_VT(pvarValue)=%d\n", V_VT(pvarValue));
        break;
    case 0x70000003: /* Undocumented property set by IE10 */
        return E_NOTIMPL;
    default:
        ok(0, "unexpected property %lx\n", dwProperty);
        return E_NOTIMPL;
    }

    ok(!pvarIndex, "pvarIndex != NULL\n");
    ok(pvarValue != NULL, "pvarValue == NULL\n");

    return S_OK;
}

static const IActiveScriptPropertyVtbl ActiveScriptPropertyVtbl = {
    ActiveScriptProperty_QueryInterface,
    ActiveScriptProperty_AddRef,
    ActiveScriptProperty_Release,
    ActiveScriptProperty_GetProperty,
    ActiveScriptProperty_SetProperty
};

static IActiveScriptProperty ActiveScriptProperty = { &ActiveScriptPropertyVtbl };

static HRESULT WINAPI ActiveScriptParseProcedure_QueryInterface(IActiveScriptParseProcedure2 *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptParseProcedure_AddRef(IActiveScriptParseProcedure2 *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptParseProcedure_Release(IActiveScriptParseProcedure2 *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptParseProcedure_ParseProcedureText(IActiveScriptParseProcedure2 *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrFormalParams, LPCOLESTR pstrProcedureName,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags, IDispatch **ppdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IActiveScriptParseProcedure2Vtbl ActiveScriptParseProcedureVtbl = {
    ActiveScriptParseProcedure_QueryInterface,
    ActiveScriptParseProcedure_AddRef,
    ActiveScriptParseProcedure_Release,
    ActiveScriptParseProcedure_ParseProcedureText
};

static IActiveScriptParseProcedure2 ActiveScriptParseProcedure = { &ActiveScriptParseProcedureVtbl };

static HRESULT WINAPI ActiveScriptParse_QueryInterface(IActiveScriptParse *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptParse_AddRef(IActiveScriptParse *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptParse_Release(IActiveScriptParse *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptParse_InitNew(IActiveScriptParse *iface)
{
    CHECK_EXPECT(InitNew);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptParse_AddScriptlet(IActiveScriptParse *iface,
        LPCOLESTR pstrDefaultName, LPCOLESTR pstrCode, LPCOLESTR pstrItemName,
        LPCOLESTR pstrSubItemName, LPCOLESTR pstrEventName, LPCOLESTR pstrDelimiter,
        CTXARG_T dwSourceContextCookie, ULONG ulStartingLineNumber, DWORD dwFlags,
        BSTR *pbstrName, EXCEPINFO *pexcepinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT dispex_propput(IDispatchEx *obj, DISPID id, DWORD flags, VARIANT *var, IServiceProvider *caller_sp)
{
    DISPID propput_arg = DISPID_PROPERTYPUT;
    DISPPARAMS dp = {var, &propput_arg, 1, 1};
    EXCEPINFO ei = {0};

    return IDispatchEx_InvokeEx(obj, id, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT|flags, &dp, NULL, &ei, caller_sp);
}

static HRESULT dispex_propget(IDispatchEx *obj, DISPID id, VARIANT *res, IServiceProvider *caller_sp)
{
    DISPPARAMS dp = {NULL};
    EXCEPINFO ei = {0};

    return IDispatchEx_InvokeEx(obj, id, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dp, res, &ei, caller_sp);
}

static void test_func(IDispatchEx *obj)
{
    DISPID id;
    IDispatchEx *dispex;
    IDispatch *disp;
    EXCEPINFO ei;
    DISPPARAMS dp;
    BSTR str;
    VARIANT var;
    HRESULT hres;

    str = SysAllocString(L"toString");
    hres = IDispatchEx_GetDispID(obj, str, fdexNameCaseSensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id == DISPID_IOMNAVIGATOR_TOSTRING, "id = %lx\n", id);

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    VariantInit(&var);
    hres = IDispatchEx_InvokeEx(obj, id, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(var) == NULL\n");
    disp = V_DISPATCH(&var);

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    /* FIXME: Test InvokeEx(DISPATCH_METHOD) */

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    VariantInit(&var);
    hres = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, &var, &ei, NULL);
    ok(hres == S_OK || broken(hres == E_ACCESSDENIED), "InvokeEx failed: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        DISPID named_args[2] = { DISPID_THIS, 0xdeadbeef };
        VARIANT args[2];

        ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
        ok(!lstrcmpW(V_BSTR(&var), L"[object]"), "V_BSTR(var) = %s\n", wine_dbgstr_w(V_BSTR(&var)));
        VariantClear(&var);

        dp.rgdispidNamedArgs = named_args;
        dp.cNamedArgs = 2;
        dp.cArgs = 2;
        dp.rgvarg = &var;
        V_VT(args) = VT_DISPATCH;
        V_DISPATCH(args) = (IDispatch*)obj;
        V_VT(args+1) = VT_I4;
        V_I4(args+1) = 3;
        hres = IDispatchEx_Invoke(dispex, DISPID_VALUE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, &var, &ei, NULL);
        ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
        ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
        ok(!lstrcmpW(V_BSTR(&var), L"[object]"), "V_BSTR(var) = %s\n", wine_dbgstr_w(V_BSTR(&var)));
        VariantClear(&var);
    }

    V_VT(&var) = VT_I4;
    V_I4(&var) = 100;
    hres = dispex_propput(obj, id, 0, &var, NULL);
    todo_wine ok(hres == E_NOTIMPL, "InvokeEx failed: %08lx\n", hres);

    hres = dispex_propget(dispex, DISPID_VALUE, &var, NULL);
    ok(hres == E_ACCESSDENIED, "InvokeEx returned: %08lx, expected E_ACCESSDENIED\n", hres);
    if(SUCCEEDED(hres))
        VariantClear(&var);

    SET_EXPECT(QS_IActiveScriptSite);
    SET_EXPECT(QS_GetCaller);
    hres = dispex_propget(dispex, DISPID_VALUE, &var, &caller_sp);
    ok(hres == S_OK, "InvokeEx returned: %08lx, expected S_OK\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var) = %d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"\nfunction toString() {\n    [native code]\n}\n"),
       "V_BSTR(var) = %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);
    todo_wine CHECK_CALLED(QS_IActiveScriptSite);
    todo_wine CHECK_CALLED(QS_GetCaller);

    SET_EXPECT(QS_IActiveScriptSite2);
    SET_EXPECT(QS_GetCaller2);
    SET_EXPECT(cmdtarget_Exec);
    hres = dispex_propget(dispex, DISPID_VALUE, &var, &caller_sp2);
    ok(hres == S_OK, "InvokeEx returned: %08lx, expected S_OK\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var) = %d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"\nfunction toString() {\n    [native code]\n}\n"),
       "V_BSTR(var) = %s\n", wine_dbgstr_w(V_BSTR(&var)));
    VariantClear(&var);
    todo_wine CHECK_CALLED(QS_IActiveScriptSite2);
    todo_wine CHECK_CALLED(QS_GetCaller2);
    todo_wine CHECK_CALLED(cmdtarget_Exec);

    IDispatchEx_Release(dispex);
}

static void test_nextdispid(IDispatchEx *dispex)
{
    DISPID last_id = DISPID_STARTENUM, id, dyn_id;
    BSTR name;
    VARIANT var;
    HRESULT hres;

    name = SysAllocString(L"dynVal");
    hres = IDispatchEx_GetDispID(dispex, name, fdexNameCaseSensitive|fdexNameEnsure, &dyn_id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(name);

    V_VT(&var) = VT_EMPTY;
    hres = dispex_propput(dispex, dyn_id, 0, &var, NULL);
    ok(hres == S_OK, "dispex_propput failed: %08lx\n", hres);

    while(last_id != dyn_id) {
        hres = IDispatchEx_GetNextDispID(dispex, fdexEnumAll, last_id, &id);
        ok(hres == S_OK, "GetNextDispID returned: %08lx\n", hres);
        ok(id != DISPID_STARTENUM, "id == DISPID_STARTENUM\n");
        ok(id != DISPID_IOMNAVIGATOR_TOSTRING, "id == DISPID_IOMNAVIGATOR_TOSTRING\n");

        hres = IDispatchEx_GetMemberName(dispex, id, &name);
        ok(hres == S_OK, "GetMemberName failed: %08lx\n", hres);

        if(id == dyn_id)
            ok(!lstrcmpW(name, L"dynVal"), "name = %s\n", wine_dbgstr_w(name));
        else if(id == DISPID_IOMNAVIGATOR_PLATFORM)
            ok(!lstrcmpW(name, L"platform"), "name = %s\n", wine_dbgstr_w(name));

        SysFreeString(name);
        last_id = id;
    }

    hres = IDispatchEx_GetNextDispID(dispex, 0, id, &id);
    ok(hres == S_FALSE, "GetNextDispID returned: %08lx\n", hres);
    ok(id == DISPID_STARTENUM, "id != DISPID_STARTENUM\n");
}

static void test_global_id(void)
{
    VARIANT var;
    DISPPARAMS dp;
    EXCEPINFO ei;
    BSTR tmp;
    DISPID id;
    HRESULT hres;

    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(script_divid_d);
    tmp = SysAllocString(L"divid");
    hres = IDispatchEx_GetDispID(window_dispex, tmp, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    SysFreeString(tmp);
    CHECK_CALLED(GetScriptDispatch);
    CHECK_CALLED(script_divid_d);

    VariantInit(&var);
    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    hres = IDispatchEx_InvokeEx(window_dispex, id, 0, DISPATCH_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var) = %d\n", V_VT(&var));
    VariantClear(&var);
}

static void test_arg_conv(IHTMLWindow2 *window)
{
    DISPPARAMS dp = { 0 };
    IHTMLDocument2 *doc;
    IDispatchEx *dispex;
    IHTMLElement *elem;
    VARIANT v, args[2];
    DISPID id;
    BSTR bstr;
    HRESULT hres;

    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);

    hres = IHTMLDocument2_get_body(doc, &elem);
    IHTMLDocument2_Release(doc);
    ok(hres == S_OK, "get_body failed: %08lx\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IDispatchEx, (void**)&dispex);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    SET_EXPECT(QS_VariantConversion);
    SET_EXPECT(ChangeType_bstr);
    V_VT(&v) = VT_I4;
    V_I4(&v) = 0xf0f0f0;
    hres = dispex_propput(dispex, DISPID_IHTMLBODYELEMENT_BACKGROUND, 0, &v, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(QS_VariantConversion);
    CHECK_CALLED(ChangeType_bstr);

    V_VT(&v) = VT_EMPTY;
    hres = dispex_propget(dispex, DISPID_IHTMLBODYELEMENT_BGCOLOR, &v, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(&var) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    IDispatchEx_Release(dispex);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    SET_EXPECT(GetScriptDispatch);
    bstr = SysAllocString(L"attachEvent");
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    CHECK_CALLED(GetScriptDispatch);
    SysFreeString(bstr);

    SET_EXPECT(QS_VariantConversion);
    SET_EXPECT(ChangeType_dispatch);
    dp.cArgs = 2;
    dp.rgvarg = args;
    V_VT(&args[1]) = VT_BSTR;
    V_BSTR(&args[1]) = SysAllocString(L"onload");
    V_VT(&args[0]) = VT_NULL;
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, &v, NULL, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(var) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_FALSE, "V_BOOL(var) = %d\n", V_BOOL(&v));
    CHECK_CALLED(QS_VariantConversion);
    CHECK_CALLED(ChangeType_dispatch);

    SET_EXPECT(GetScriptDispatch);
    bstr = SysAllocString(L"detachEvent");
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    CHECK_CALLED(GetScriptDispatch);
    SysFreeString(bstr);

    SET_EXPECT(QS_VariantConversion);
    SET_EXPECT(ChangeType_dispatch);
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    CHECK_CALLED(QS_VariantConversion);
    CHECK_CALLED(ChangeType_dispatch);

    SysFreeString(V_BSTR(&args[1]));
    IDispatchEx_Release(dispex);
}

#define test_elem_disabled(a,b) _test_elem_disabled(__LINE__,a,b)
static void _test_elem_disabled(unsigned line, IHTMLElement *elem, VARIANT_BOOL exb)
{
    IHTMLElement3 *elem3;
    VARIANT_BOOL b = 100;
    HRESULT hres;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLElement3, (void**)&elem3);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IHTMLElement3 iface: %08lx\n", hres);

    hres = IHTMLElement3_get_disabled(elem3, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_disabled failed: %08lx\n", hres);
    ok_(__FILE__,line)(b == exb, "disabled = %x, expected %x\n", b, exb);

    IHTMLElement3_Release(elem3);
}

static void test_default_arg_conv(IHTMLWindow2 *window)
{
    IHTMLDocument2 *doc;
    IDispatchEx *dispex;
    IHTMLElement *elem;
    VARIANT v;
    HRESULT hres;

    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);

    hres = IHTMLDocument2_get_body(doc, &elem);
    IHTMLDocument2_Release(doc);
    ok(hres == S_OK, "get_body failed: %08lx\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    test_elem_disabled(elem, VARIANT_FALSE);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = SysAllocString(L"test");
    hres = dispex_propput(dispex, DISPID_IHTMLELEMENT3_DISABLED, 0, &v, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    SysFreeString(V_BSTR(&v));

    test_elem_disabled(elem, VARIANT_TRUE);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 0;
    hres = dispex_propput(dispex, DISPID_IHTMLELEMENT3_DISABLED, 0, &v, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    test_elem_disabled(elem, VARIANT_FALSE);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 1;
    hres = dispex_propput(dispex, DISPID_IHTMLELEMENT3_DISABLED, DISPATCH_PROPERTYPUTREF, &v, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    test_elem_disabled(elem, VARIANT_TRUE);

    IHTMLElement_Release(elem);
    IDispatchEx_Release(dispex);
}

static void test_named_args(IHTMLWindow2 *window)
{
    DISPID named_args[] = { 0, 2, 1, 1337, 0xdeadbeef, DISPID_THIS };
    IHTMLDocument2 *doc;
    IDispatchEx *dispex;
    IHTMLElement *elem;
    VARIANT args[4];
    DISPPARAMS dp;
    HRESULT hres;
    VARIANT var;
    DISPID id;
    BSTR bstr;

    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);

    bstr = SysAllocString(L"div");
    hres = IHTMLDocument2_createElement(doc, bstr, &elem);
    IHTMLDocument2_Release(doc);
    SysFreeString(bstr);
    ok(hres == S_OK, "createElement failed: %08lx\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    bstr = SysAllocString(L"setAttribute");
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    SysFreeString(bstr);

    dp.cArgs = 2;
    dp.cNamedArgs = 3;
    dp.rgvarg = args;
    dp.rgdispidNamedArgs = named_args;
    V_VT(&args[0]) = VT_BSTR;
    V_BSTR(&args[0]) = SysAllocString(L"testattr");
    V_VT(&args[1]) = VT_I4;
    V_I4(&args[1]) = 0;
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx returned: %08lx\n", hres);

    hres = IHTMLElement_getAttribute(elem, V_BSTR(&args[0]), 0, &var);
    ok(hres == S_OK, "getAttribute failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_NULL, "V_VT(var)=%d\n", V_VT(&var));

    bstr = SysAllocString(L"0");
    hres = IHTMLElement_getAttribute(elem, bstr, 0, &var);
    SysFreeString(bstr);
    ok(hres == S_OK, "getAttribute failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"testattr"), "V_BSTR(&var) = %s\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    dp.cArgs = 3;
    V_VT(&args[2]) = VT_BSTR;
    V_BSTR(&args[2]) = SysAllocString(L"testval");
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    VariantClear(&args[2]);
    ok(hres == DISP_E_TYPEMISMATCH, "InvokeEx returned: %08lx\n", hres);

    V_VT(&args[2]) = VT_I4;
    V_I4(&args[2]) = 0;
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == DISP_E_TYPEMISMATCH, "InvokeEx returned: %08lx\n", hres);

    args[2] = args[0];
    V_VT(&args[1]) = VT_BSTR;
    V_BSTR(&args[1]) = SysAllocString(L"testval");
    V_VT(&args[0]) = VT_I4;
    V_I4(&args[0]) = 0;
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    VariantClear(&args[1]);

    hres = IHTMLElement_getAttribute(elem, V_BSTR(&args[2]), 0, &var);
    ok(hres == S_OK, "getAttribute failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"testval"), "V_BSTR(&var) = %s\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    dp.cArgs = 2;
    dp.cNamedArgs = 1;
    args[1] = args[2];
    V_VT(&args[0]) = VT_BSTR;
    V_BSTR(&args[0]) = SysAllocString(L"newValue");
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    VariantClear(&args[0]);

    hres = IHTMLElement_getAttribute(elem, V_BSTR(&args[1]), 0, &var);
    ok(hres == S_OK, "getAttribute failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"newValue"), "V_BSTR(&var) = %s\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    dp.cArgs = 4;
    dp.cNamedArgs = ARRAY_SIZE(named_args);
    args[3] = args[1];
    V_VT(&args[2]) = VT_BSTR;
    V_BSTR(&args[2]) = SysAllocString(L"foobar");
    V_VT(&args[1]) = VT_I4;
    V_I4(&args[1]) = 1;
    V_VT(&args[0]) = VT_BSTR;
    V_BSTR(&args[0]) = SysAllocString(L"extra");
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    VariantClear(&args[2]);
    VariantClear(&args[0]);

    hres = IHTMLElement_getAttribute(elem, V_BSTR(&args[3]), 0, &var);
    ok(hres == S_OK, "getAttribute failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"foobar"), "V_BSTR(&var) = %s\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&var);

    dp.cNamedArgs = 1;
    named_args[0] = DISPID_THIS;
    V_VT(&args[0]) = VT_DISPATCH;
    V_DISPATCH(&args[0]) = (IDispatch*)&funcDisp;
    V_VT(&args[2]) = VT_BSTR;
    V_BSTR(&args[2]) = SysAllocString(L"withThis");
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, DISPATCH_METHOD, &dp, NULL, NULL, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    VariantClear(&args[2]);

    hres = IHTMLElement_getAttribute(elem, V_BSTR(&args[3]), 0, &var);
    ok(hres == S_OK, "getAttribute failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
    ok(!lstrcmpW(V_BSTR(&var), L"withThis"), "V_BSTR(&var) = %s\n", debugstr_w(V_BSTR(&var)));
    VariantClear(&args[3]);
    VariantClear(&var);

    IHTMLElement_Release(elem);
    IDispatchEx_Release(dispex);
}

static void test_ui(void)
{
    IActiveScriptSiteUIControl *ui_control;
    SCRIPTUICHANDLING uic_handling = 10;
    HRESULT hres;

    hres = IActiveScriptSite_QueryInterface(site, &IID_IActiveScriptSiteUIControl, (void**)&ui_control);
    if(hres == E_NOINTERFACE) {
        win_skip("IActiveScriptSiteUIControl not supported\n");
        return;
    }
    ok(hres == S_OK, "Could not get IActiveScriptSiteUIControl: %08lx\n", hres);

    hres = IActiveScriptSiteUIControl_GetUIBehavior(ui_control, SCRIPTUICITEM_MSGBOX, &uic_handling);
    ok(hres == S_OK, "GetUIBehavior failed: %08lx\n", hres);
    ok(uic_handling == SCRIPTUICHANDLING_ALLOW, "uic_handling = %d\n", uic_handling);

    IActiveScriptSiteUIControl_Release(ui_control);
}

static void test_sp(void)
{
    IServiceProvider *sp, *doc_sp, *doc_obj_sp, *window_sp;
    IHTMLWindow2 *window, *cmdtarget_window;
    IOleCommandTarget *cmdtarget;
    IHTMLDocument2 *doc;
    IUnknown *unk;
    HRESULT hres;
    VARIANT var;

    hres = IDispatchEx_QueryInterface(window_dispex, &IID_IHTMLWindow2, (void**)&window);
    ok(hres == S_OK, "QueryInterface(IHTMLWindow2) failed: %08lx\n", hres);
    ok(window != NULL, "window is NULL\n");

    hres = IHTMLWindow2_QueryInterface(window, &IID_IServiceProvider, (void**)&window_sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    ok(window_sp != NULL, "window service provider is NULL\n");

    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "QueryInterface(IHTMLDocument2) failed: %08lx\n", hres);
    ok(doc != NULL, "doc is NULL\n");
    ok(doc != doc_obj, "doc node == doc obj\n");

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IServiceProvider, (void**)&doc_sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    ok(doc_sp != NULL, "doc service provider is NULL\n");
    IHTMLDocument2_Release(doc);
    hres = IHTMLDocument2_QueryInterface(doc_obj, &IID_IServiceProvider, (void**)&doc_obj_sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    ok(doc_obj_sp != NULL, "doc_obj service provider is NULL\n");

    hres = IActiveScriptSite_QueryInterface(site, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

    hres = IServiceProvider_QueryService(sp, &SID_SContainerDispatch, &IID_IHTMLDocument, (void**)&unk);
    ok(hres == S_OK, "Could not get SID_SContainerDispatch service: %08lx\n", hres);
    IUnknown_Release(unk);

    hres = IServiceProvider_QueryService(sp, &SID_GetCaller, &IID_IServiceProvider, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
    hres = IServiceProvider_QueryService(doc_sp, &SID_GetCaller, &IID_IServiceProvider, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
    hres = IServiceProvider_QueryService(doc_obj_sp, &SID_GetCaller, &IID_IServiceProvider, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);
    hres = IServiceProvider_QueryService(window_sp, &SID_GetCaller, &IID_IServiceProvider, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryService(SID_GetCaller) returned: %08lx\n", hres);

    hres = IServiceProvider_QueryService(sp, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&cmdtarget);
    ok(hres == S_OK, "QueryService(IActiveScriptSite->IOleCommandTarget) failed: %08lx\n", hres);
    ok(cmdtarget != NULL, "IOleCommandTarget is NULL\n");

    hres = IActiveScriptSite_QueryInterface(site, &IID_IOleCommandTarget, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IOleCommandTarget) failed: %08lx\n", hres);
    ok(unk != NULL, "QueryInterface(IOleCommandTarget) is NULL\n");
    ok(cmdtarget == (IOleCommandTarget*)unk, "cmdtarget from QS not same as from QI\n");
    IUnknown_Release(unk);
    hres = IServiceProvider_QueryService(doc_sp, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&unk);
    ok(hres == S_OK, "QueryService(IActiveScriptSite->IOleCommandTarget) failed: %08lx\n", hres);
    ok(cmdtarget == (IOleCommandTarget*)unk, "IActiveScriptSite service from document provider not same as site's\n");
    IUnknown_Release(unk);
    hres = IServiceProvider_QueryService(doc_obj_sp, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryService(IActiveScriptSite->IOleCommandTarget) returned: %08lx\n", hres);
    hres = IServiceProvider_QueryService(window_sp, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryService(IActiveScriptSite->IOleCommandTarget) returned: %08lx\n", hres);

    if(site2) {
        IOleCommandTarget *cmdtarget2;
        IServiceProvider *sp2;

        hres = IActiveScriptSite_QueryInterface(site2, &IID_IServiceProvider, (void**)&sp2);
        ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

        hres = IServiceProvider_QueryService(sp2, &IID_IActiveScriptSite, &IID_IOleCommandTarget, (void**)&cmdtarget2);
        ok(hres == S_OK, "QueryService(IActiveScriptSite->IOleCommandTarget) failed: %08lx\n", hres);
        ok(cmdtarget2 != NULL, "IOleCommandTarget is NULL\n");

        hres = IActiveScriptSite_QueryInterface(site2, &IID_IOleCommandTarget, (void**)&unk);
        ok(hres == S_OK, "QueryInterface(IOleCommandTarget) failed: %08lx\n", hres);
        ok(unk != NULL, "QueryInterface(IOleCommandTarget) is NULL\n");
        ok(cmdtarget2 != (IOleCommandTarget*)unk, "cmdtarget from site2's QS same as from QI\n");
        ok(cmdtarget2 == cmdtarget, "site1's cmdtarget not same as site2's\n");
        IOleCommandTarget_Release(cmdtarget2);
        IServiceProvider_Release(sp2);
        IUnknown_Release(unk);
    }

    V_VT(&var) = VT_EMPTY;
    hres = IOleCommandTarget_Exec(cmdtarget, &CGID_ScriptSite, CMDID_SCRIPTSITE_SECURITY_WINDOW, 0, NULL, &var);
    ok(hres == S_OK, "Exec failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(CMDID_SCRIPTSITE_SECURITY_WINDOW) = %d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(CMDID_SCRIPTSITE_SECURITY_WINDOW) = NULL\n");

    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IHTMLWindow2, (void**)&cmdtarget_window);
    ok(hres == S_OK, "QueryInterface(IHTMLWindow2) failed: %08lx\n", hres);
    ok(cmdtarget_window != NULL, "cmdtarget_window is NULL\n");
    ok(window == cmdtarget_window, "window != cmdtarget_window\n");
    IHTMLWindow2_Release(cmdtarget_window);
    VariantClear(&var);

    IOleCommandTarget_Release(cmdtarget);
    IServiceProvider_Release(window_sp);
    IServiceProvider_Release(doc_obj_sp);
    IServiceProvider_Release(doc_sp);
    IServiceProvider_Release(sp);
    IHTMLWindow2_Release(window);
}

static void test_script_run(void)
{
    IDispatchEx *document, *dispex;
    IHTMLWindow2 *window;
    IOmNavigator *navigator;
    IUnknown *unk;
    VARIANT var, arg;
    DISPPARAMS dp;
    EXCEPINFO ei;
    DISPID id;
    BSTR tmp;
    HRESULT hres;

    static const WCHAR documentW[] = {'d','o','c','u','m','e','n','t',0};
    static const WCHAR testW[] = {'t','e','s','t',0};
    static const WCHAR funcW[] = {'f','u','n','c',0};

    SET_EXPECT(GetScriptDispatch);

    tmp = SysAllocString(documentW);
    hres = IDispatchEx_GetDispID(window_dispex, tmp, fdexNameCaseSensitive, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(document) failed: %08lx\n", hres);
    ok(id == DISPID_IHTMLWINDOW2_DOCUMENT, "id=%lx\n", id);

    CHECK_CALLED(GetScriptDispatch);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));

    hres = IDispatchEx_InvokeEx(window_dispex, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(&var) == NULL\n");

    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IDispatchEx, (void**)&document);
    VariantClear(&var);
    ok(hres == S_OK, "Could not get DispatchEx: %08lx\n", hres);

    tmp = SysAllocString(testW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(document) failed: %08lx, expected DISP_E_UNKNOWNNAME\n", hres);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive | fdexNameImplicit, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(document) failed: %08lx, expected DISP_E_UNKNOWNNAME\n", hres);
    SysFreeString(tmp);

    id = 0;
    tmp = SysAllocString(testW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive|fdexNameEnsure, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(document) failed: %08lx\n", hres);
    ok(id, "id == 0\n");

    V_VT(&var) = VT_I4;
    V_I4(&var) = 100;
    hres = dispex_propput(document, id, 0, &var, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    tmp = SysAllocString(testW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(document) failed: %08lx\n", hres);

    hres = IDispatchEx_DeleteMemberByDispID(document, id);
    ok(hres == E_NOTIMPL, "DeleteMemberByDispID failed = %08lx\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I4, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_I4(&var) == 100, "V_I4(&var) = %ld\n", V_I4(&var));

    V_VT(&var) = VT_I4;
    V_I4(&var) = 200;
    hres = dispex_propput(document, id, DISPATCH_PROPERTYPUTREF, &var, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I4, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_I4(&var) == 200, "V_I4(&var) = %ld\n", V_I4(&var));

    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&var) = VT_I4;
    V_I4(&var) = 300;
    dp.cArgs = 1;
    dp.rgvarg = &var;
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I4, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_I4(&var) == 300, "V_I4(&var) = %ld\n", V_I4(&var));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = NULL;
    hres = dispex_propput(document, id, 0, &var, NULL);
    ok(hres == S_OK, "dispex_propput failed: %08lx\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_BSTR, "V_VT(var)=%d\n", V_VT(&var));
    ok(!V_BSTR(&var), "V_BSTR(&var) = %s\n", wine_dbgstr_w(V_BSTR(&var)));

    unk = (void*)0xdeadbeef;
    hres = IDispatchEx_GetNameSpaceParent(window_dispex, &unk);
    ok(hres == S_OK, "GetNameSpaceParent failed: %08lx\n", hres);
    ok(!unk, "unk=%p, expected NULL\n", unk);

    id = 0;
    tmp = SysAllocString(funcW);
    hres = IDispatchEx_GetDispID(document, tmp, fdexNameCaseSensitive|fdexNameEnsure, &id);
    SysFreeString(tmp);
    ok(hres == S_OK, "GetDispID(func) failed: %08lx\n", hres);
    ok(id, "id == 0\n");

    dp.cArgs = 1;
    dp.rgvarg = &var;
    dp.cNamedArgs = 0;
    dp.rgdispidNamedArgs = NULL;
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)&funcDisp;
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);

    VariantInit(&var);
    memset(&dp, 0, sizeof(dp));
    memset(&ei, 0, sizeof(ei));
    V_VT(&arg) = VT_BOOL;
    V_BOOL(&arg) = VARIANT_TRUE;
    dp.cArgs = 1;
    dp.rgvarg = &arg;

    SET_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE); /* IE11 */
    SET_EXPECT(funcDisp);
    hres = IDispatchEx_InvokeEx(document, id, LOCALE_NEUTRAL, INVOKE_FUNC, &dp, &var, &ei, NULL);
    todo_wine
    CHECK_CALLED_BROKEN(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE); /* IE11 */
    CHECK_CALLED(funcDisp);

    ok(hres == S_OK, "InvokeEx(INVOKE_FUNC) failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_I4, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_I4(&var) == 100, "V_I4(&var) == NULL\n");

    IDispatchEx_Release(document);

    hres = IDispatchEx_QueryInterface(window_dispex, &IID_IHTMLWindow2, (void**)&window);
    ok(hres == S_OK, "Could not get IHTMLWindow2 iface: %08lx\n", hres);

    hres = IHTMLWindow2_get_navigator(window, &navigator);
    ok(hres == S_OK, "get_navigator failed: %08lx\n", hres);

    hres = IOmNavigator_QueryInterface(navigator, &IID_IDispatchEx, (void**)&dispex);
    IOmNavigator_Release(navigator);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08lx\n", hres);

    test_func(dispex);
    test_nextdispid(dispex);

    test_arg_conv(window);
    test_default_arg_conv(window);
    test_named_args(window);
    IHTMLWindow2_Release(window);

    tmp = SysAllocString(L"test");
    hres = IDispatchEx_DeleteMemberByName(dispex, tmp, fdexNameCaseSensitive);
    ok(hres == E_NOTIMPL, "DeleteMemberByName failed: %08lx\n", hres);

    IDispatchEx_Release(dispex);

    script_disp = (IDispatch*)&scriptDisp;

    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(script_testprop_d);
    tmp = SysAllocString(L"testProp");
    hres = IDispatchEx_GetDispID(window_dispex, tmp, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id != DISPID_SCRIPT_TESTPROP, "id == DISPID_SCRIPT_TESTPROP\n");
    CHECK_CALLED(GetScriptDispatch);
    CHECK_CALLED(script_testprop_d);
    SysFreeString(tmp);

    tmp = SysAllocString(L"testProp");
    hres = IDispatchEx_GetDispID(window_dispex, tmp, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id != DISPID_SCRIPT_TESTPROP, "id == DISPID_SCRIPT_TESTPROP\n");
    SysFreeString(tmp);

    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(script_testprop_i);
    memset(&ei, 0, sizeof(ei));
    memset(&dp, 0, sizeof(dp));
    hres = IDispatchEx_InvokeEx(window_dispex, id, 0, DISPATCH_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08lx\n", hres);
    ok(V_VT(&var) == VT_NULL, "V_VT(var) = %d\n", V_VT(&var));
    CHECK_CALLED(GetScriptDispatch);
    CHECK_CALLED(script_testprop_i);

    SET_EXPECT(GetScriptDispatch);
    SET_EXPECT(script_testprop2_d);
    tmp = SysAllocString(L"testProp2");
    hres = IDispatchEx_GetDispID(window_dispex, tmp, fdexNameCaseSensitive|fdexNameEnsure, &id);
    ok(hres == S_OK, "GetDispID failed: %08lx\n", hres);
    ok(id != DISPID_SCRIPT_TESTPROP2, "id == DISPID_SCRIPT_TESTPROP2\n");
    CHECK_CALLED(GetScriptDispatch);
    CHECK_CALLED(script_testprop2_d);
    SysFreeString(tmp);

    tmp = SysAllocString(L"test");
    hres = IDispatchEx_DeleteMemberByName(window_dispex, tmp, fdexNameCaseSensitive);
    ok(hres == E_NOTIMPL, "DeleteMemberByName failed: %08lx\n", hres);

    test_global_id();

    test_security();
    test_ui();
    test_sp();
}

static HRESULT WINAPI ActiveScriptParse_ParseScriptText(IActiveScriptParse *iface,
        LPCOLESTR pstrCode, LPCOLESTR pstrItemName, IUnknown *punkContext,
        LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie, ULONG ulStartingLine,
        DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    ok(pvarResult != NULL, "pvarResult == NULL\n");
    ok(pexcepinfo != NULL, "pexcepinfo == NULL\n");

    if(!lstrcmpW(pstrCode, L"execScript call")) {
        CHECK_EXPECT(ParseScriptText_execScript);
        ok(!pstrItemName, "pstrItemName = %s\n", wine_dbgstr_w(pstrItemName));
        ok(!lstrcmpW(pstrDelimiter, L"\""), "pstrDelimiter = %s\n", wine_dbgstr_w(pstrDelimiter));
        ok(dwFlags == SCRIPTTEXT_ISVISIBLE, "dwFlags = %lx\n", dwFlags);

        V_VT(pvarResult) = VT_I4;
        V_I4(pvarResult) = 10;
        return S_OK;
    }else if(!lstrcmpW(pstrCode, L"simple script")) {
        CHECK_EXPECT(ParseScriptText_script);
        ok(!lstrcmpW(pstrItemName, L"window"), "pstrItemName = %s\n", wine_dbgstr_w(pstrItemName));
        ok(!lstrcmpW(pstrDelimiter, L"</SCRIPT>"), "pstrDelimiter = %s\n", wine_dbgstr_w(pstrDelimiter));
        ok(dwFlags == (SCRIPTTEXT_ISVISIBLE|SCRIPTTEXT_HOSTMANAGESSOURCE), "dwFlags = %lx\n", dwFlags);

        test_script_run();
        return S_OK;
    }

    ok(0, "unexpected script %s\n", wine_dbgstr_w(pstrCode));
    return E_FAIL;
}

static const IActiveScriptParseVtbl ActiveScriptParseVtbl = {
    ActiveScriptParse_QueryInterface,
    ActiveScriptParse_AddRef,
    ActiveScriptParse_Release,
    ActiveScriptParse_InitNew,
    ActiveScriptParse_AddScriptlet,
    ActiveScriptParse_ParseScriptText
};

static IActiveScriptParse ActiveScriptParse = { &ActiveScriptParseVtbl };

static HRESULT WINAPI ActiveScript_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScript, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParse, riid)) {
        *ppv = &ActiveScriptParse;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParseProcedure2, riid)) {
        *ppv = &ActiveScriptParseProcedure;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptProperty, riid)) {
        *ppv = &ActiveScriptProperty;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        *ppv = &ObjectSafety;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptDebug, riid))
        return E_NOINTERFACE;

    trace("QI(%s)\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScript_AddRef(IActiveScript *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScript_Release(IActiveScript *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScript_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    IActiveScriptSiteInterruptPoll *poll;
    IActiveScriptSiteDebug *debug;
    IServiceProvider *service;
    ICanHandleException *canexception;
    LCID lcid;
    HRESULT hres;

    CHECK_EXPECT(SetScriptSite);

    ok(pass != NULL, "pass == NULL\n");

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteInterruptPoll, (void**)&poll);
    ok(hres == S_OK, "Could not get IActiveScriptSiteInterruptPoll interface: %08lx\n", hres);
    if(SUCCEEDED(hres))
        IActiveScriptSiteInterruptPoll_Release(poll);

    hres = IActiveScriptSite_GetLCID(pass, &lcid);
    ok(hres == S_OK, "GetLCID failed: %08lx\n", hres);

    hres = IActiveScriptSite_OnStateChange(pass, (state = SCRIPTSTATE_INITIALIZED));
    ok(hres == S_OK, "OnStateChange failed: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IActiveScriptSiteDebug, (void**)&debug);
    ok(hres == S_OK, "Could not get IActiveScriptSiteDebug interface: %08lx\n", hres);
    if(SUCCEEDED(hres))
        IActiveScriptSiteDebug_Release(debug);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_ICanHandleException, (void**)&canexception);
    ok(hres == E_NOINTERFACE, "Could not get IID_ICanHandleException interface: %08lx\n", hres);

    hres = IActiveScriptSite_QueryInterface(pass, &IID_IServiceProvider, (void**)&service);
    ok(hres == S_OK, "Could not get IServiceProvider interface: %08lx\n", hres);
    if(SUCCEEDED(hres))
        IServiceProvider_Release(service);

    site = pass;
    IActiveScriptSite_AddRef(site);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptSite(IActiveScript *iface, REFIID riid,
                                            void **ppvObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    HRESULT hres;

    switch(ss) {
    case SCRIPTSTATE_STARTED:
        CHECK_EXPECT(SetScriptState_STARTED);
        break;
    case SCRIPTSTATE_CONNECTED:
        CHECK_EXPECT(SetScriptState_CONNECTED);
        break;
    case SCRIPTSTATE_DISCONNECTED:
        CHECK_EXPECT(SetScriptState_DISCONNECTED);
        break;
    default:
        ok(0, "unexpected state %d\n", ss);
        return E_NOTIMPL;
    }

    hres = IActiveScriptSite_OnStateChange(site, (state = ss));
    ok(hres == S_OK, "OnStateChange failed: %08lx\n", hres);

    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    CHECK_EXPECT(GetScriptState);

    *pssState = state;
    return S_OK;
}

static HRESULT WINAPI ActiveScript_Close(IActiveScript *iface)
{
    CHECK_EXPECT(Close);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_AddNamedItem(IActiveScript *iface,
        LPCOLESTR pstrName, DWORD dwFlags)
{
    IDispatch *disp;
    IUnknown *unk = NULL, *unk2;
    HRESULT hres;

    static const WCHAR windowW[] = {'w','i','n','d','o','w',0};

    static const IID unknown_iid = {0x719C3050,0xF9D3,0x11CF,{0xA4,0x93,0x00,0x40,0x05,0x23,0xA8,0xA0}};

    CHECK_EXPECT(AddNamedItem);

    ok(!lstrcmpW(pstrName, windowW), "pstrName=%s\n", wine_dbgstr_w(pstrName));
    ok(dwFlags == (SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS), "dwFlags=%lx\n", dwFlags);

    hres = IActiveScriptSite_GetItemInfo(site, windowW, SCRIPTINFO_IUNKNOWN, &unk, NULL);
    ok(hres == S_OK, "GetItemInfo failed: %08lx\n", hres);
    ok(unk != NULL, "unk == NULL\n");

    hres = IUnknown_QueryInterface(unk, &IID_IDispatch, (void**)&disp);
    ok(hres == S_OK, "Could not get IDispatch interface: %08lx\n", hres);
    if(SUCCEEDED(hres))
        IDispatch_Release(disp);

    hres = IUnknown_QueryInterface(unk, &unknown_iid, (void**)&unk2);
    ok(hres == E_NOINTERFACE, "Got ?? interface: %p\n", unk2);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk2);

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&window_dispex);
    ok(hres == S_OK, "Could not get IDispatchEx interface: %08lx\n", hres);

    IUnknown_Release(unk);
    return S_OK;
}

static HRESULT WINAPI ActiveScript_AddTypeLib(IActiveScript *iface, REFGUID rguidTypeLib,
                                         DWORD dwMajor, DWORD dwMinor, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName,
                                                IDispatch **ppdisp)
{
    CHECK_EXPECT(GetScriptDispatch);

    ok(!lstrcmpW(pstrItemName, L"window"), "pstrItemName = %s\n", wine_dbgstr_w(pstrItemName));

    if(!script_disp)
        return E_NOTIMPL;

    *ppdisp = script_disp;
    return S_OK;
}

static HRESULT WINAPI ActiveScript_GetCurrentScriptThreadID(IActiveScript *iface,
                                                       SCRIPTTHREADID *pstridThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadID(IActiveScript *iface,
                                                DWORD dwWin32ThreadId, SCRIPTTHREADID *pstidThread)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_GetScriptThreadState(IActiveScript *iface,
        SCRIPTTHREADID stidThread, SCRIPTTHREADSTATE *pstsState)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_InterruptScriptThread(IActiveScript *iface,
        SCRIPTTHREADID stidThread, const EXCEPINFO *pexcepinfo, DWORD dwFlags)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript_Clone(IActiveScript *iface, IActiveScript **ppscript)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IActiveScriptVtbl ActiveScriptVtbl = {
    ActiveScript_QueryInterface,
    ActiveScript_AddRef,
    ActiveScript_Release,
    ActiveScript_SetScriptSite,
    ActiveScript_GetScriptSite,
    ActiveScript_SetScriptState,
    ActiveScript_GetScriptState,
    ActiveScript_Close,
    ActiveScript_AddNamedItem,
    ActiveScript_AddTypeLib,
    ActiveScript_GetScriptDispatch,
    ActiveScript_GetCurrentScriptThreadID,
    ActiveScript_GetScriptThreadID,
    ActiveScript_GetScriptThreadState,
    ActiveScript_InterruptScriptThread,
    ActiveScript_Clone
};

static IActiveScript ActiveScript = { &ActiveScriptVtbl };

static HRESULT WINAPI ObjectSafety2_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    CHECK_EXPECT(GetInterfaceSafetyOptions2);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    ok(pdwSupportedOptions != NULL, "pdwSupportedOptions == NULL\n");
    ok(pdwEnabledOptions != NULL, "pdwEnabledOptions == NULL\n");

    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_DISPEX | INTERFACE_USES_SECURITY_MANAGER;
    *pdwEnabledOptions = INTERFACE_USES_DISPEX;

    return S_OK;
}

static HRESULT WINAPI ObjectSafety2_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    CHECK_EXPECT(SetInterfaceSafetyOptions2);

    ok(IsEqualGUID(&IID_IActiveScriptParse, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));

    ok(dwOptionSetMask == (INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_DISPEX | INTERFACE_USES_SECURITY_MANAGER),
       "dwOptionSetMask = %08lx\n", dwOptionSetMask);
    ok(dwEnabledOptions == (INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACE_USES_DISPEX | INTERFACE_USES_SECURITY_MANAGER),
       "dwEnabledOptions = %08lx\n", dwOptionSetMask);

    return S_OK;
}

static const IObjectSafetyVtbl ObjectSafety2Vtbl = {
    ObjectSafety_QueryInterface,
    ObjectSafety_AddRef,
    ObjectSafety_Release,
    ObjectSafety2_GetInterfaceSafetyOptions,
    ObjectSafety2_SetInterfaceSafetyOptions
};

static IObjectSafety ObjectSafety2 = { &ObjectSafety2Vtbl };

static HRESULT WINAPI ActiveScriptProperty2_SetProperty(IActiveScriptProperty *iface, DWORD dwProperty,
        VARIANT *pvarIndex, VARIANT *pvarValue)
{
    switch(dwProperty) {
    case SCRIPTPROP_HACK_TRIDENTEVENTSINK:
        CHECK_EXPECT(SetProperty2_HACK_TRIDENTEVENTSINK);
        ok(V_VT(pvarValue) == VT_BOOL, "V_VT(pvarValue) = %d\n", V_VT(pvarValue));
        ok(V_BOOL(pvarValue) == VARIANT_TRUE, "V_BOOL(pvarValue) = %x\n", V_BOOL(pvarValue));
        break;
    case SCRIPTPROP_INVOKEVERSIONING:
        CHECK_EXPECT(SetProperty2_INVOKEVERSIONING);
        ok(V_VT(pvarValue) == VT_I4, "V_VT(pvarValue) = %d\n", V_VT(pvarValue));
        ok(V_I4(pvarValue) == 1, "V_I4(pvarValue) = %ld\n", V_I4(pvarValue));
        break;
    case SCRIPTPROP_ABBREVIATE_GLOBALNAME_RESOLUTION:
        CHECK_EXPECT(SetProperty2_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
        ok(V_VT(pvarValue) == VT_BOOL, "V_VT(pvarValue) = %d\n", V_VT(pvarValue));
        ok(!V_BOOL(pvarValue), "ABBREVIATE_GLOBALNAME_RESOLUTION is TRUE\n");
        break;
    case 0x70000003: /* Undocumented property set by IE10 */
        return E_NOTIMPL;
    default:
        ok(0, "unexpected property %08lx\n", dwProperty);
        return E_NOTIMPL;
    }

    ok(!pvarIndex, "pvarIndex != NULL\n");
    ok(pvarValue != NULL, "pvarValue == NULL\n");

    return S_OK;
}

static const IActiveScriptPropertyVtbl ActiveScriptProperty2Vtbl = {
    ActiveScriptProperty_QueryInterface,
    ActiveScriptProperty_AddRef,
    ActiveScriptProperty_Release,
    ActiveScriptProperty_GetProperty,
    ActiveScriptProperty2_SetProperty
};

static IActiveScriptProperty ActiveScriptProperty2 = { &ActiveScriptProperty2Vtbl };

static HRESULT WINAPI ActiveScriptParse2_InitNew(IActiveScriptParse *iface)
{
    CHECK_EXPECT(InitNew2);
    return S_OK;
}

static HRESULT WINAPI ActiveScriptParse2_ParseScriptText(IActiveScriptParse *iface, LPCOLESTR pstrCode,
        LPCOLESTR pstrItemName, IUnknown *punkContext, LPCOLESTR pstrDelimiter, CTXARG_T dwSourceContextCookie,
        ULONG ulStartingLine, DWORD dwFlags, VARIANT *pvarResult, EXCEPINFO *pexcepinfo)
{
    ok(pvarResult != NULL, "pvarResult == NULL\n");
    ok(pexcepinfo != NULL, "pexcepinfo == NULL\n");

    if(!lstrcmpW(pstrCode, L"second script")) {
        CHECK_EXPECT(ParseScriptText_script2);
        ok(!lstrcmpW(pstrItemName, L"window"), "pstrItemName = %s\n", wine_dbgstr_w(pstrItemName));
        ok(!lstrcmpW(pstrDelimiter, L"</SCRIPT>"), "pstrDelimiter = %s\n", wine_dbgstr_w(pstrDelimiter));
        ok(dwFlags == (SCRIPTTEXT_ISVISIBLE | SCRIPTTEXT_HOSTMANAGESSOURCE), "dwFlags = %08lx\n", dwFlags);
        test_sp();
        return S_OK;
    }

    ok(0, "unexpected script %s\n", wine_dbgstr_w(pstrCode));
    return E_FAIL;
}

static const IActiveScriptParseVtbl ActiveScriptParse2Vtbl = {
    ActiveScriptParse_QueryInterface,
    ActiveScriptParse_AddRef,
    ActiveScriptParse_Release,
    ActiveScriptParse2_InitNew,
    ActiveScriptParse_AddScriptlet,
    ActiveScriptParse2_ParseScriptText
};

static IActiveScriptParse ActiveScriptParse2 = { &ActiveScriptParse2Vtbl };

static HRESULT WINAPI ActiveScript2_QueryInterface(IActiveScript *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IActiveScript, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParse, riid)) {
        *ppv = &ActiveScriptParse2;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptParseProcedure2, riid)) {
        *ppv = &ActiveScriptParseProcedure;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptProperty, riid)) {
        *ppv = &ActiveScriptProperty2;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        *ppv = &ObjectSafety2;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IActiveScriptDebug, riid))
        return E_NOINTERFACE;

    trace("QI(%s)\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI ActiveScript2_SetScriptSite(IActiveScript *iface, IActiveScriptSite *pass)
{
    HRESULT hres;

    CHECK_EXPECT(SetScriptSite2);
    ok(pass != NULL, "pass == NULL\n");
    ok(pass != site, "pass == site\n");

    hres = IActiveScriptSite_OnStateChange(pass, (state2 = SCRIPTSTATE_INITIALIZED));
    ok(hres == S_OK, "OnStateChange failed: %08lx\n", hres);

    site2 = pass;
    IActiveScriptSite_AddRef(site2);
    return S_OK;
}

static HRESULT WINAPI ActiveScript2_SetScriptState(IActiveScript *iface, SCRIPTSTATE ss)
{
    HRESULT hres = IActiveScriptSite_OnStateChange(site2, (state2 = ss));
    ok(hres == S_OK, "OnStateChange failed: %08lx\n", hres);
    return S_OK;
}

static HRESULT WINAPI ActiveScript2_GetScriptState(IActiveScript *iface, SCRIPTSTATE *pssState)
{
    *pssState = state2;
    return S_OK;
}

static HRESULT WINAPI ActiveScript2_Close(IActiveScript *iface)
{
    CHECK_EXPECT(Close2);
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScript2_AddNamedItem(IActiveScript *iface, LPCOLESTR pstrName, DWORD dwFlags)
{
    IHTMLWindow2 *window, *window2;
    IUnknown *unk = NULL;
    HRESULT hres;

    CHECK_EXPECT(AddNamedItem2);
    ok(!wcscmp(pstrName, L"window"), "pstrName = %s\n", wine_dbgstr_w(pstrName));
    ok(dwFlags == (SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE | SCRIPTITEM_GLOBALMEMBERS), "dwFlags = %lx\n", dwFlags);

    hres = IActiveScriptSite_GetItemInfo(site2, L"window", SCRIPTINFO_IUNKNOWN, &unk, NULL);
    ok(hres == S_OK, "GetItemInfo failed: %08lx\n", hres);
    ok(unk != NULL, "unk == NULL\n");

    /* Native is pretty broken here, it gives a different IUnknown than first site's SCRIPTINFO_IUNKNOWN,
     * and querying for IDispatchEx gives different interfaces on both these *and* our window_dispex!
     * That said, querying for IHTMLWindow2 *does* give the same interface for both?!?
     */
    hres = IDispatchEx_QueryInterface(window_dispex, &IID_IHTMLWindow2, (void**)&window);
    ok(hres == S_OK, "Could not get IHTMLWindow2 interface: %08lx\n", hres);
    hres = IUnknown_QueryInterface(unk, &IID_IHTMLWindow2, (void**)&window2);
    ok(hres == S_OK, "Could not get IHTMLWindow2 interface: %08lx\n", hres);
    ok(window == window2, "first site window != second site window\n");
    IHTMLWindow2_Release(window2);
    IHTMLWindow2_Release(window);

    IUnknown_Release(unk);
    return S_OK;
}

static HRESULT WINAPI ActiveScript2_GetScriptDispatch(IActiveScript *iface, LPCOLESTR pstrItemName, IDispatch **ppdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IActiveScriptVtbl ActiveScript2Vtbl = {
    ActiveScript2_QueryInterface,
    ActiveScript_AddRef,
    ActiveScript_Release,
    ActiveScript2_SetScriptSite,
    ActiveScript_GetScriptSite,
    ActiveScript2_SetScriptState,
    ActiveScript2_GetScriptState,
    ActiveScript2_Close,
    ActiveScript2_AddNamedItem,
    ActiveScript_AddTypeLib,
    ActiveScript2_GetScriptDispatch,
    ActiveScript_GetCurrentScriptThreadID,
    ActiveScript_GetScriptThreadID,
    ActiveScript_GetScriptThreadState,
    ActiveScript_InterruptScriptThread,
    ActiveScript_Clone
};

static IActiveScript ActiveScript2 = { &ActiveScript2Vtbl };

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(&IID_IMarshal, riid))
        return E_NOINTERFACE;
    if(IsEqualGUID(&CLSID_IdentityUnmarshal, riid))
        return E_NOINTERFACE;

    ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
    return E_NOTIMPL;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IActiveScript, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = &ActiveScript;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT WINAPI ClassFactory2_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    CHECK_EXPECT(CreateInstance2);

    ok(!outer, "outer = %p\n", outer);
    ok(IsEqualGUID(&IID_IActiveScript, riid), "unexpected riid %s\n", wine_dbgstr_guid(riid));
    *ppv = &ActiveScript2;
    return S_OK;
}

static const IClassFactoryVtbl ClassFactory2Vtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory2_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory script_cf[] = { { &ClassFactoryVtbl }, { &ClassFactory2Vtbl } };

typedef struct {
    IInternetProtocolEx IInternetProtocolEx_iface;
    IWinInetHttpInfo IWinInetHttpInfo_iface;

    LONG ref;

    IInternetProtocolSink *sink;
    BINDINFO bind_info;

    HANDLE delay_event;
    BSTR content_type;
    IStream *stream;
    char *data;
    ULONG size;
    LONG delay;
    char *ptr;

    IUri *uri;
} ProtocolHandler;

static ProtocolHandler *delay_with_signal_handler;

static DWORD WINAPI delay_proc(void *arg)
{
    PROTOCOLDATA protocol_data = {PI_FORCE_ASYNC};
    ProtocolHandler *protocol_handler = arg;

    if(protocol_handler->delay_event)
        WaitForSingleObject(protocol_handler->delay_event, INFINITE);
    else
        Sleep(protocol_handler->delay);
    protocol_handler->delay = -1;
    IInternetProtocolSink_Switch(protocol_handler->sink, &protocol_data);
    return 0;
}

static void report_data(ProtocolHandler *This)
{
    IServiceProvider *service_provider;
    IHttpNegotiate *http_negotiate;
    WCHAR *addl_headers = NULL;
    WCHAR headers_buf[128];
    BSTR headers, url;
    HRESULT hres;

    static const WCHAR emptyW[] = {0};

    if(This->delay >= 0) {
        hres = IInternetProtocolSink_QueryInterface(This->sink, &IID_IServiceProvider, (void**)&service_provider);
        ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

        hres = IServiceProvider_QueryService(service_provider, &IID_IHttpNegotiate, &IID_IHttpNegotiate, (void**)&http_negotiate);
        IServiceProvider_Release(service_provider);
        if(This->delay && hres == E_FAIL)  /* aborted too quickly */
            return;
        ok(hres == S_OK, "Could not get IHttpNegotiate interface: %08lx\n", hres);

        hres = IUri_GetDisplayUri(This->uri, &url);
        ok(hres == S_OK, "Failed to get display uri: %08lx\n", hres);
        hres = IHttpNegotiate_BeginningTransaction(http_negotiate, url, emptyW, 0, &addl_headers);
        ok(hres == S_OK, "BeginningTransaction failed: %08lx\n", hres);
        SysFreeString(url);

        CoTaskMemFree(addl_headers);

        if(This->content_type)
            swprintf(headers_buf, ARRAY_SIZE(headers_buf), L"HTTP/1.1 200 OK\r\nContent-Type: %s\r\n", This->content_type);

        headers = SysAllocString(This->content_type ? headers_buf : L"HTTP/1.1 200 OK\r\n\r\n");
        hres = IHttpNegotiate_OnResponse(http_negotiate, 200, headers, NULL, NULL);
        ok(hres == S_OK, "OnResponse failed: %08lx\n", hres);
        SysFreeString(headers);

        IHttpNegotiate_Release(http_negotiate);

        if(This->delay || This->delay_event) {
            IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
            QueueUserWorkItem(delay_proc, This, 0);
            return;
        }
    }
    This->delay = 0;

    hres = IInternetProtocolSink_ReportData(This->sink, BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION,
                                            This->size, This->size);
    ok(hres == S_OK, "ReportData failed: %08lx\n", hres);

    hres = IInternetProtocolSink_ReportResult(This->sink, S_OK, 0, NULL);
    ok(hres == S_OK || broken(hres == 0x80ef0001), "ReportResult failed: %08lx\n", hres);
}

typedef struct js_stream_t {
    struct js_stream_t *next;
    ProtocolHandler *protocol_handler;
    WCHAR name[1];
} js_stream_t;

static js_stream_t *registered_stream_list;

static void register_stream(ProtocolHandler *protocol_handler, const WCHAR *name)
{
    js_stream_t *stream;
    size_t len;

    len = lstrlenW(name)+1;
    stream = HeapAlloc(GetProcessHeap(), 0, offsetof(js_stream_t, name[len+1]));

    IInternetProtocolEx_AddRef(&protocol_handler->IInternetProtocolEx_iface);
    stream->protocol_handler = protocol_handler;
    memcpy(stream->name, name, len*sizeof(WCHAR));

    stream->next = registered_stream_list;
    registered_stream_list = stream;
}

static void free_registered_streams(void)
{
    js_stream_t *iter;

    while((iter = registered_stream_list)) {
        ok(!iter->protocol_handler, "protocol handler still pending for %s\n", wine_dbgstr_w(iter->name));
        if(iter->protocol_handler)
            IInternetProtocolEx_Release(&iter->protocol_handler->IInternetProtocolEx_iface);

        registered_stream_list = iter->next;
        HeapFree(GetProcessHeap(), 0, iter);
    }
}

static DWORD WINAPI async_switch_proc(void *arg)
{
    PROTOCOLDATA protocol_data = {PI_FORCE_ASYNC};
    ProtocolHandler *protocol_handler = arg;

    IInternetProtocolSink_Switch(protocol_handler->sink, &protocol_data);
    return 0;
}

static void async_switch(ProtocolHandler *protocol_handler)
{
    IInternetProtocolEx_AddRef(&protocol_handler->IInternetProtocolEx_iface);
    QueueUserWorkItem(async_switch_proc, protocol_handler, 0);
}

static void stream_write(const WCHAR *name, const WCHAR *data)
{
    ProtocolHandler *protocol_handler;
    LARGE_INTEGER large_zero;
    js_stream_t *stream;
    HRESULT hres;

    static const BYTE bom_utf16[] = {0xff,0xfe};

    for(stream = registered_stream_list; stream; stream = stream->next) {
        if(!lstrcmpW(stream->name, name))
            break;
    }
    ok(stream != NULL, "stream %s not found\n", wine_dbgstr_w(name));
    if(!stream)
        return;

    protocol_handler = stream->protocol_handler;
    ok(protocol_handler != NULL, "protocol_handler is NULL\n");
    if(!protocol_handler)
        return;
    stream->protocol_handler = NULL;

    hres = CreateStreamOnHGlobal(NULL, TRUE, &protocol_handler->stream);
    ok(hres == S_OK, "CreateStreamOnHGlobal failed: %08lx\n", hres);

    hres = IStream_Write(protocol_handler->stream, bom_utf16, sizeof(bom_utf16), NULL);
    ok(hres == S_OK, "Write failed: %08lx\n", hres);

    hres = IStream_Write(protocol_handler->stream, data, lstrlenW(data)*sizeof(WCHAR), NULL);
    ok(hres == S_OK, "Write failed: %08lx\n", hres);

    large_zero.QuadPart = 0;
    hres = IStream_Seek(protocol_handler->stream, large_zero, STREAM_SEEK_SET, NULL);
    ok(hres == S_OK, "Seek failed: %08lx\n", hres);

    async_switch(protocol_handler);
    IInternetProtocolEx_Release(&protocol_handler->IInternetProtocolEx_iface);
}

static char index_html_data[4096];

static inline ProtocolHandler *impl_from_IInternetProtocolEx(IInternetProtocolEx *iface)
{
    return CONTAINING_RECORD(iface, ProtocolHandler, IInternetProtocolEx_iface);
}

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocolEx *iface, REFIID riid, void **ppv)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid) || IsEqualGUID(&IID_IInternetProtocolEx, riid)) {
        *ppv = &This->IInternetProtocolEx_iface;
    }else if(IsEqualGUID(&IID_IWinInetInfo, riid) || IsEqualGUID(&IID_IWinInetHttpInfo, riid)) {
        *ppv = &This->IWinInetHttpInfo_iface;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Protocol_AddRef(IInternetProtocolEx *iface)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Protocol_Release(IInternetProtocolEx *iface)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    if(!ref) {
        if(This->delay_event)
            CloseHandle(This->delay_event);
        if(This->sink)
            IInternetProtocolSink_Release(This->sink);
        if(This->stream)
            IStream_Release(This->stream);
        if(This->uri)
            IUri_Release(This->uri);
        ReleaseBindInfo(&This->bind_info);
        SysFreeString(This->content_type);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocolEx *iface, LPCWSTR szUrl, IInternetProtocolSink *pOIProtSink,
        IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE_PTR dwReserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocolEx *iface, PROTOCOLDATA *pProtocolData)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    report_data(This);
    IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
    return S_OK;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocolEx *iface, HRESULT hrReason, DWORD dwOptions)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    if(This->delay > 0) {
        ok(hrReason == E_ABORT, "Abort hrReason = %08lx\n", hrReason);
        ok(dwOptions == 0, "Abort dwOptions = %lx\n", dwOptions);
        return S_OK;
    }
    trace("Abort(%08lx %lx)\n", hrReason, dwOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Terminate(IInternetProtocolEx *iface, DWORD dwOptions)
{
    return S_OK;
}

static HRESULT WINAPI Protocol_Suspend(IInternetProtocolEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Resume(IInternetProtocolEx *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Read(IInternetProtocolEx *iface, void *pv, ULONG cb, ULONG *pcbRead)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    ULONG read;
    HRESULT hres;

    if(GetCurrentThreadId() == main_thread_id) {
        IHTMLDocument2 *doc_node;
        DISPPARAMS dp = { 0 };
        IHTMLWindow2 *window;
        VARIANT v;

        hres = IHTMLDocument2_get_parentWindow(notif_doc, &window);
        ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);

        hres = IHTMLWindow2_get_document(window, &doc_node);
        IHTMLWindow2_Release(window);
        ok(hres == S_OK, "get_document failed: %08lx\n", hres);

        V_VT(&v) = VT_EMPTY;
        V_I4(&v) = 1234;
        hres = IHTMLDocument2_Invoke(doc_node, DISPID_READYSTATE, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, NULL);
        ok(hres == S_OK, "Invoke(DISPID_READYSTATE) failed: %08lx\n", hres);
        ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
        IHTMLDocument2_Release(doc_node);
    }

    if(This->stream) {
        hres = IStream_Read(This->stream, pv, cb, &read);
        ok(SUCCEEDED(hres), "Read failed: %08lx\n", hres);
        if(FAILED(hres))
            return hres;
        if(pcbRead)
            *pcbRead = read;
        return read ? S_OK : S_FALSE;
    }

    read = This->size - (This->ptr - This->data);
    if(read > cb)
        read = cb;

    if(read) {
        memcpy(pv, This->ptr, read);
        This->ptr += read;
    }

    *pcbRead = read;
    return This->ptr == This->data+This->size ? S_FALSE : S_OK;
}

static HRESULT WINAPI Protocol_Seek(IInternetProtocolEx *iface,
        LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_LockRequest(IInternetProtocolEx *iface, DWORD dwOptions)
{
    return S_OK;
}

static HRESULT WINAPI Protocol_UnlockRequest(IInternetProtocolEx *iface)
{
    return S_OK;
}

static HRESULT WINAPI ProtocolEx_StartEx(IInternetProtocolEx *iface, IUri *uri, IInternetProtocolSink *pOIProtSink,
        IInternetBindInfo *pOIBindInfo, DWORD grfPI, HANDLE *dwReserved)
{
    ProtocolHandler *This = impl_from_IInternetProtocolEx(iface);
    BOOL block = FALSE;
    BSTR path, query;
    DWORD bindf;
    HRSRC src;
    HRESULT hres;

    static const WCHAR empty_prefixW[] = {'/','e','m','p','t','y'};

    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &This->bind_info);
    ok(hres == S_OK, "GetBindInfo failed: %08lx\n", hres);

    hres = IUri_GetPath(uri, &path);
    if(FAILED(hres))
        return hres;

    if(!lstrcmpW(path, L"/index.html")) {
        This->data = index_html_data;
        This->size = strlen(This->data);
    }else if(!lstrcmpW(path, L"/echo.php")) {
        ok(This->bind_info.dwBindVerb == BINDVERB_POST, "unexpected bind verb %d\n", This->bind_info.dwBindVerb == BINDVERB_POST);
        todo_wine ok(This->bind_info.stgmedData.tymed == TYMED_ISTREAM, "tymed = %lx\n", This->bind_info.stgmedData.tymed);
        switch(This->bind_info.stgmedData.tymed) {
        case TYMED_HGLOBAL:
            This->size = This->bind_info.cbstgmedData;
            This->data = This->bind_info.stgmedData.hGlobal;
            break;
        case TYMED_ISTREAM:
            This->stream = This->bind_info.stgmedData.pstm;
            IStream_AddRef(This->stream);
            break;
        default:
            ok(0, "unexpected tymed %ld\n", This->bind_info.stgmedData.tymed);
        }
    }else if(!lstrcmpW(path, L"/jsstream.php")) {
        BSTR query;

        hres = IUri_GetQuery(uri, &query);
        ok(hres == S_OK, "GetQuery failed: %08lx\n", hres);
        ok(SysStringLen(query) > 1 && *query == '?', "unexpected query %s\n", wine_dbgstr_w(query));
        register_stream(This, query+1);
        SysFreeString(query);
        block = TRUE;
    }else if(SysStringLen(path) >= ARRAY_SIZE(empty_prefixW) && !memcmp(path, empty_prefixW, sizeof(empty_prefixW))) {
        static char empty_data[] = " ";
        This->data = empty_data;
        This->size = strlen(This->data);
    }else {
        const WCHAR *type = (SysStringLen(path) > 4 && !wcsicmp(path + SysStringLen(path) - 4, L".png")) ? L"PNG" : (const WCHAR*)RT_HTML;
        src = FindResourceW(NULL, *path == '/' ? path+1 : path, type);
        if(src) {
            This->size = SizeofResource(NULL, src);
            This->data = LoadResource(NULL, src);
        }else {
            HANDLE file = CreateFileW(*path == '/' ? path+1 : path, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_READONLY, NULL);
            if(file != INVALID_HANDLE_VALUE) {
                This->size = GetFileSize(file, NULL);
                This->data = malloc(This->size);
                ReadFile(file, This->data, This->size, &This->size, NULL);
                CloseHandle(file);
            }else {
                ok(0, "Could not find %s\n", debugstr_w(path));
                hres = E_FAIL;
            }
        }
    }

    SysFreeString(path);
    if(FAILED(hres))
        return hres;

    hres = IUri_GetQuery(uri, &query);
    if(SUCCEEDED(hres)) {
        if(!wcscmp(query, L"?delay"))
            This->delay = 1000;
        else if(!wcscmp(query, L"?delay_with_signal")) {
            if(delay_with_signal_handler) {
                ProtocolHandler *delayed = delay_with_signal_handler;
                delay_with_signal_handler = NULL;
                SetEvent(delayed->delay_event);
                This->delay = 30;
            }else {
                delay_with_signal_handler = This;
                This->delay_event = CreateEventW(NULL, FALSE, FALSE, NULL);
                ok(This->delay_event != NULL, "CreateEvent failed: %08lx\n", GetLastError());
            }
        }
        else if(!wcsncmp(query, L"?content-type=", sizeof("?content-type=")-1))
            This->content_type = SysAllocString(query + sizeof("?content-type=")-1);
        SysFreeString(query);
    }

#ifdef __REACTOS__
    This->sink = pOIProtSink;
    IInternetProtocolSink_AddRef(This->sink);
    This->uri = uri;
    IUri_AddRef(This->uri);
#else
    IInternetProtocolSink_AddRef(This->sink = pOIProtSink);
    IUri_AddRef(This->uri = uri);
#endif

    This->ptr = This->data;
    if(!block)
        async_switch(This);
    return E_PENDING;
}

static const IInternetProtocolExVtbl ProtocolExVtbl = {
    Protocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release,
    Protocol_Start,
    Protocol_Continue,
    Protocol_Abort,
    Protocol_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    Protocol_Read,
    Protocol_Seek,
    Protocol_LockRequest,
    Protocol_UnlockRequest,
    ProtocolEx_StartEx
};

static inline ProtocolHandler *impl_from_IWinInetHttpInfo(IWinInetHttpInfo *iface)
{
    return CONTAINING_RECORD(iface, ProtocolHandler, IWinInetHttpInfo_iface);
}

static HRESULT WINAPI HttpInfo_QueryInterface(IWinInetHttpInfo *iface, REFIID riid, void **ppv)
{
    ProtocolHandler *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_QueryInterface(&This->IInternetProtocolEx_iface, riid, ppv);
}

static ULONG WINAPI HttpInfo_AddRef(IWinInetHttpInfo *iface)
{
    ProtocolHandler *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_AddRef(&This->IInternetProtocolEx_iface);
}

static ULONG WINAPI HttpInfo_Release(IWinInetHttpInfo *iface)
{
    ProtocolHandler *This = impl_from_IWinInetHttpInfo(iface);
    return IInternetProtocolEx_Release(&This->IInternetProtocolEx_iface);
}

static HRESULT WINAPI HttpInfo_QueryOption(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpInfo_QueryInfo(IWinInetHttpInfo *iface, DWORD dwOption,
        void *pBuffer, DWORD *pcbBuffer, DWORD *pdwFlags, DWORD *pdwReserved)
{
    return E_NOTIMPL;
}

static const IWinInetHttpInfoVtbl WinInetHttpInfoVtbl = {
    HttpInfo_QueryInterface,
    HttpInfo_AddRef,
    HttpInfo_Release,
    HttpInfo_QueryOption,
    HttpInfo_QueryInfo
};

static HRESULT WINAPI ProtocolCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI ProtocolCF_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    ProtocolHandler *protocol;
    HRESULT hres;

    protocol = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*protocol));
    protocol->IInternetProtocolEx_iface.lpVtbl = &ProtocolExVtbl;
    protocol->IWinInetHttpInfo_iface.lpVtbl = &WinInetHttpInfoVtbl;
    protocol->ref = 1;
    protocol->bind_info.cbSize = sizeof(protocol->bind_info);

    hres = IInternetProtocolEx_QueryInterface(&protocol->IInternetProtocolEx_iface, riid, ppv);
    IInternetProtocolEx_Release(&protocol->IInternetProtocolEx_iface);
    return hres;
}

static const IClassFactoryVtbl ProtocolCFVtbl = {
    ProtocolCF_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ProtocolCF_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory protocol_cf = { &ProtocolCFVtbl };

static const char simple_script_str[] =
    "<html><head></head><body>"
    "<div id=\"divid\"></div>"
    "<script language=\"TestScript1\">simple script</script>"
    "</body></html>";

static void test_insert_script_elem(IHTMLDocument2 *doc, const WCHAR *code, const WCHAR *lang)
{
    IHTMLDOMNode *node, *body_node, *inserted_node;
    IHTMLScriptElement *script;
    IHTMLElement *elem, *body;
    HRESULT hres;
    BSTR bstr;

    bstr = SysAllocString(L"script");
    hres = IHTMLDocument2_createElement(doc, bstr, &elem);
    ok(hres == S_OK, "createElement failed: %08lx\n", hres);
    SysFreeString(bstr);

    bstr = SysAllocString(lang);
    hres = IHTMLElement_put_language(elem, bstr);
    ok(hres == S_OK, "put_language failed: %08lx\n", hres);
    SysFreeString(bstr);

    bstr = SysAllocString(code);
    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLScriptElement, (void**)&script);
    ok(hres == S_OK, "Could not get IHTMLScriptElement iface: %08lx\n", hres);
    hres = IHTMLScriptElement_put_text(script, bstr);
    ok(hres == S_OK, "put_text failed: %08lx\n", hres);
    IHTMLScriptElement_Release(script);
    SysFreeString(bstr);

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08lx\n", hres);
    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLDOMNode, (void**)&node);
    ok(hres == S_OK, "Could not get IHTMLDOMNode iface: %08lx\n", hres);
    hres = IHTMLElement_QueryInterface(body, &IID_IHTMLDOMNode, (void**)&body_node);
    ok(hres == S_OK, "Could not get IHTMLDOMNode iface: %08lx\n", hres);
    hres = IHTMLDOMNode_appendChild(body_node, node, &inserted_node);
    ok(hres == S_OK, "appendChild failed: %08lx\n", hres);
    IHTMLDOMNode_Release(inserted_node);
    IHTMLDOMNode_Release(body_node);
    IHTMLDOMNode_Release(node);
    IHTMLElement_Release(body);
    IHTMLElement_Release(elem);
}

static void test_exec_script(IHTMLDocument2 *doc, const WCHAR *codew, const WCHAR *langw)
{
    IHTMLWindow2 *window;
    BSTR code, lang;
    VARIANT v;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);

    ok(iface_cmp(window, window_dispex), "window != dispex_window\n");

    code = SysAllocString(codew);
    lang = SysAllocString(langw);

    SET_EXPECT(ParseScriptText_execScript);
    hres = IHTMLWindow2_execScript(window, code, lang, &v);
    ok(hres == S_OK, "execScript failed: %08lx\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I4(&v) == 10, "V_I4(v) = %ld\n", V_I4(&v));
    CHECK_CALLED(ParseScriptText_execScript);
    SysFreeString(lang);

    lang = SysAllocString(L"invalid");
    V_VT(&v) = 100;
    hres = IHTMLWindow2_execScript(window, code, lang, &v);
    ok(hres == CO_E_CLASSSTRING, "execScript failed: %08lx, expected CO_E_CLASSSTRING\n", hres);
    ok(V_VT(&v) == 100, "V_VT(v) = %d\n", V_VT(&v));
    SysFreeString(lang);
    SysFreeString(code);

    IHTMLWindow2_Release(window);
}

static void test_simple_script(void)
{
    IInternetHostSecurityManager *sec_mgr, *sec_mgr2;
    IHTMLDocument2 *doc_node;
    IServiceProvider *sp;
    IHTMLWindow2 *window;
    IHTMLDocument2 *doc;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return;

    SET_EXPECT(CreateInstance);
    SET_EXPECT(GetInterfaceSafetyOptions);
    SET_EXPECT(SetInterfaceSafetyOptions);
    SET_EXPECT(SetProperty_INVOKEVERSIONING); /* IE8 */
    SET_EXPECT(SetProperty_HACK_TRIDENTEVENTSINK);
    SET_EXPECT(InitNew);
    SET_EXPECT(SetScriptSite);
    SET_EXPECT(GetScriptState);
    SET_EXPECT(SetScriptState_STARTED);
    SET_EXPECT(AddNamedItem);
    SET_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_TRUE); /* IE8 */
    SET_EXPECT(ParseScriptText_script);
    SET_EXPECT(SetScriptState_CONNECTED);

    load_doc(doc, simple_script_str);

    CHECK_CALLED(CreateInstance);
    CHECK_CALLED(GetInterfaceSafetyOptions);
    CHECK_CALLED(SetInterfaceSafetyOptions);
    CHECK_CALLED_BROKEN(SetProperty_INVOKEVERSIONING); /* IE8 */
    CHECK_CALLED(SetProperty_HACK_TRIDENTEVENTSINK);
    CHECK_CALLED(InitNew);
    CHECK_CALLED(SetScriptSite);
    CHECK_CALLED(GetScriptState);
    CHECK_CALLED(SetScriptState_STARTED);
    CHECK_CALLED(AddNamedItem);
    CHECK_CALLED_BROKEN(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_TRUE); /* IE8 */
    CHECK_CALLED(ParseScriptText_script);
    CHECK_CALLED(SetScriptState_CONNECTED);

    SET_EXPECT(CreateInstance2);
    SET_EXPECT(GetInterfaceSafetyOptions2);
    SET_EXPECT(SetInterfaceSafetyOptions2);
    SET_EXPECT(SetProperty2_INVOKEVERSIONING);
    SET_EXPECT(SetProperty2_HACK_TRIDENTEVENTSINK);
    SET_EXPECT(InitNew2);
    SET_EXPECT(SetScriptSite2);
    SET_EXPECT(AddNamedItem2);
    SET_EXPECT(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
    SET_EXPECT(SetProperty2_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
    SET_EXPECT(ParseScriptText_script2);

    test_insert_script_elem(doc, L"second script", L"TestScript2");

    CHECK_CALLED(CreateInstance2);
    CHECK_CALLED(GetInterfaceSafetyOptions2);
    CHECK_CALLED(SetInterfaceSafetyOptions2);
    CHECK_CALLED(SetProperty2_INVOKEVERSIONING);
    CHECK_CALLED(SetProperty2_HACK_TRIDENTEVENTSINK);
    CHECK_CALLED(InitNew2);
    CHECK_CALLED(SetScriptSite2);
    CHECK_CALLED(AddNamedItem2);
    CHECK_CALLED(SetProperty_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
    CHECK_CALLED(SetProperty2_ABBREVIATE_GLOBALNAME_RESOLUTION_FALSE);
    CHECK_CALLED(ParseScriptText_script2);

    test_exec_script(doc, L"execScript call", L"TestScript1");

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);

    hres = IHTMLWindow2_get_document(window, &doc_node);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    hres = IServiceProvider_QueryService(sp, &SID_SInternetHostSecurityManager, &IID_IInternetHostSecurityManager, (void**)&sec_mgr);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    IServiceProvider_Release(sp);

    hres = IHTMLDocument2_QueryInterface(doc_node, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);
    hres = IServiceProvider_QueryService(sp, &SID_SInternetHostSecurityManager, &IID_IInternetHostSecurityManager, (void**)&sec_mgr2);
    ok(hres == S_OK, "QueryService failed: %08lx\n", hres);
    ok(iface_cmp(sec_mgr, sec_mgr2), "sec_mgr != sec_mgr2\n");
    IInternetHostSecurityManager_Release(sec_mgr2);
    IInternetHostSecurityManager_Release(sec_mgr);
    IServiceProvider_Release(sp);

    SET_EXPECT(SetScriptState_DISCONNECTED);
    SET_EXPECT(Close);
    SET_EXPECT(Close2);

    IHTMLDocument2_Release(doc);

    CHECK_CALLED(SetScriptState_DISCONNECTED);
    CHECK_CALLED(Close);
    CHECK_CALLED(Close2);

    if(site)
        IActiveScriptSite_Release(site);
    if(site2)
        IActiveScriptSite_Release(site2);
    if(window_dispex)
        IDispatchEx_Release(window_dispex);
    site = NULL;
    site2 = NULL;
    window_dispex = NULL;

    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);
    ok(doc != doc_node, "doc == doc_node\n");

    IHTMLDocument2_Release(doc_node);
    IHTMLDocument2_Release(doc);
    IHTMLWindow2_Release(window);
}

static void run_from_moniker(IMoniker *mon)
{
    DISPID dispid = DISPID_UNKNOWN;
    IPersistMoniker *persist;
    IHTMLDocument2 *doc;
    BSTR bstr;
    MSG msg;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return;

    set_client_site(doc, TRUE);
    do_advise(doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker, (void**)&persist);
    ok(hres == S_OK, "Could not get IPersistMoniker iface: %08lx\n", hres);

    hres = IPersistMoniker_Load(persist, FALSE, mon, NULL, 0);
    ok(hres == S_OK, "Load failed: %08lx\n", hres);

    IPersistMoniker_Release(persist);

    SET_EXPECT(external_success);

    while(!called_external_success && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CHECK_CALLED(external_success);

    /* check prop set by events fired during document unload */
    bstr = SysAllocString(L"doc_unload_events_called");
    hres = IHTMLDocument2_GetIDsOfNames(doc, &IID_NULL, &bstr, 1, 0, &dispid);
    SysFreeString(bstr);
    if(hres == DISP_E_UNKNOWNNAME)
        dispid = DISPID_UNKNOWN;
    else
        ok(hres == S_OK, "GetIDsOfNames failed %08lx\n", hres);

    free_registered_streams();
    set_client_site(doc, FALSE);

    if(dispid != DISPID_UNKNOWN) {
        DISPPARAMS dp = { 0 };
        UINT argerr;
        VARIANT v;

        hres = IHTMLDocument2_Invoke(doc, dispid, &IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &v, NULL, &argerr);
        ok(hres == S_OK, "Invoke failed %08lx\n", hres);
        ok(V_VT(&v) == VT_BOOL, "V_VT(doc_unload_events_called) = %d\n", V_VT(&v));
        ok(V_BOOL(&v) == VARIANT_TRUE, "doc_unload_events_called is not true\n");
    }
    IHTMLDocument2_Release(doc);
}

static void run_js_script(const char *test_name)
{
    WCHAR url[INTERNET_MAX_URL_LENGTH] = {'r','e','s',':','/','/'}, *ptr;
    IMoniker *mon;
    HRESULT hres;

    trace("running %s...\n", test_name);

    ptr = url + lstrlenW(url);
    ptr += GetModuleFileNameW(NULL, ptr, url + ARRAY_SIZE(url) - ptr);
    *ptr++ = '/';
    MultiByteToWideChar(CP_ACP, 0, test_name, -1, ptr, url + ARRAY_SIZE(url) - ptr);

    hres = CreateURLMoniker(NULL, url, &mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08lx\n", hres);
    run_from_moniker(mon);

    IMoniker_Release(mon);
}

static void run_from_path(const WCHAR *path, const char *opt)
{
    WCHAR buf[255] = L"http://winetest.example.org";
    IMoniker *mon;
    BSTR url;
    HRESULT hres;

    lstrcatW(buf, path);
    if(opt)
        wsprintfW(buf + lstrlenW(buf), L"?%S", opt);
    url = SysAllocString(buf);
    hres = CreateURLMoniker(NULL, url, &mon);
    SysFreeString(url);
    ok(hres == S_OK, "CreateUrlMoniker failed: %08lx\n", hres);

    run_from_moniker(mon);

    IMoniker_Release(mon);
}

static void run_script_as_http_with_mode(const char *script, const char *opt, const char *document_mode)
{
    trace("Running %s script in %s mode...\n", script, document_mode ? document_mode : "quirks");

    sprintf(index_html_data,
            "%s"
            "<html>\n"
            "  <head>\n"
            "    %s%s%s\n"
            "    <script src=\"winetest.js\" type=\"text/javascript\"></script>\n"
            "    <script src=\"%s\" type=\"text/javascript\"></script>\n"
            "  </head>\n"
            "  <body onload=\"run_tests();\">\n"
            "  </body>\n"
            "</html>\n",
            document_mode ? "<!DOCTYPE html>\n" : "",
            document_mode ? "<meta http-equiv=\"x-ua-compatible\" content=\"Ie=" : "",
            document_mode ? document_mode : "",
            document_mode ? "\">" : "",
            script);

    run_from_path(L"/index.html", opt ? opt : script);
}

static void test_strict_mode(void)
{
    sprintf(index_html_data,
            "<!DOCTYPE html>\n"
            "<html>\n"
            "  <head>\n"
            "    <script src=\"winetest.js\" type=\"text/javascript\"></script>\n"
            "    <script type=\"text/javascript\">\n"
            "      function test() {\n"
            "        ok(document.documentMode === 7, 'document mode = ' + document.documentMode);\n"
            "        next_test();\n"
            "      }\n"
            "      var tests = [ test ];\n"
            "    </script>\n"
            "  </head>\n"
            "  <body onload=\"run_tests();\">\n"
            "  </body>\n"
            "</html>\n");

    run_from_path(L"/index.html", "test_strict_mode");
}

static void init_protocol_handler(void)
{
    IInternetSession *internet_session;
    HRESULT hres;

    static const WCHAR httpW[] = {'h','t','t','p',0};

    hres = CoInternetGetSession(0, &internet_session, 0);
    ok(hres == S_OK, "CoInternetGetSession failed: %08lx\n", hres);

    hres = IInternetSession_RegisterNameSpace(internet_session, &protocol_cf, &CLSID_HttpProtocol, httpW, 0, NULL, 0);
    ok(hres == S_OK, "RegisterNameSpace failed: %08lx\n", hres);

    IInternetSession_Release(internet_session);
}

static void run_js_tests(void)
{
    run_js_script("jstest.html");
    run_js_script("exectest.html");
    run_js_script("events.html");

    SET_EXPECT(GetTypeInfo);
    run_js_script("vbtest.html");
    CLEAR_CALLED(GetTypeInfo);

    if(!is_ie9plus) {
        win_skip("Skipping some script tests on IE older than 9.\n");
        return;
    }

    init_protocol_handler();

    test_strict_mode();
    run_script_as_http_with_mode("xhr.js", NULL, "9");
    run_script_as_http_with_mode("xhr.js", NULL, "10");
    run_script_as_http_with_mode("xhr.js", NULL, "11");
    run_script_as_http_with_mode("dom.js", NULL, "11");
    run_script_as_http_with_mode("es5.js", NULL, "11");
    run_script_as_http_with_mode("events.js", NULL, "9");
    run_script_as_http_with_mode("navigation.js", NULL, NULL);
    run_script_as_http_with_mode("navigation.js", NULL, "11");

    run_script_as_http_with_mode("documentmode.js", "0", NULL);
    run_script_as_http_with_mode("documentmode.js", "5", "5");
    run_script_as_http_with_mode("documentmode.js", "5", "6");
    run_script_as_http_with_mode("documentmode.js", "7", "7");
    run_script_as_http_with_mode("documentmode.js", "8", "8");
    run_script_as_http_with_mode("documentmode.js", "9", "9");
    run_script_as_http_with_mode("documentmode.js", "10", "10;abc");
    run_script_as_http_with_mode("documentmode.js", "11", "11");
    run_script_as_http_with_mode("documentmode.js", "11", "edge;123");

    run_script_as_http_with_mode("asyncscriptload.js", NULL, "9");
    run_script_as_http_with_mode("reload.js", NULL, "11");
}

static BOOL init_registry(BOOL init)
{
    static const WCHAR fmt[] = L"CLSID\\%s\\Implemented Categories\\{%08lX-9847-11CF-8F20-00805F2CD064}";
    WCHAR *clsid, buf[ARRAY_SIZE(fmt) + 40];
    BOOL ret = TRUE;
    HRESULT hres;
    unsigned i;

    for(i = 0; i < ARRAY_SIZE(CLSID_TestScript) && ret; i++) {
        hres = StringFromCLSID(&CLSID_TestScript[i], &clsid);
        ok(hres == S_OK, "StringFromCLSID failed: %08lx\n", hres);

        swprintf(buf, ARRAY_SIZE(buf), L"TestScript%u\\CLSID", i + 1);
        if((ret = init_key(buf, clsid, init))) {
            swprintf(buf, ARRAY_SIZE(buf), fmt, clsid, 0xf0b7a1a1);
            if((ret = init_key(buf, NULL, init))) {
                swprintf(buf, ARRAY_SIZE(buf), fmt, clsid, 0xf0b7a1a2);
                ret = init_key(buf, NULL, init);
            }
        }
        CoTaskMemFree(clsid);
    }
    return ret;
}

static BOOL register_script_engine(void)
{
    DWORD regid;
    unsigned i;
    HRESULT hres;

    if(!init_registry(TRUE)) {
        init_registry(FALSE);
        return FALSE;
    }

    for(i = 0; i < ARRAY_SIZE(CLSID_TestScript); i++) {
        hres = CoRegisterClassObject(&CLSID_TestScript[i], (IUnknown *)&script_cf[i],
                                     CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
        ok(hres == S_OK, "Could not register TestScript%u engine: %08lx\n", i + 1, hres);
    }

    return TRUE;
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static const CHAR szHTMLDocumentTest[] = "HTMLDocumentTest";
    static WNDCLASSEXA wndclass = {
        sizeof(WNDCLASSEXA),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        szHTMLDocumentTest,
        NULL
    };

    RegisterClassExA(&wndclass);
    return CreateWindowA(szHTMLDocumentTest, szHTMLDocumentTest,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            300, 300, NULL, NULL, NULL, NULL);
}

static void detect_locale(void)
{
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    LANGID (WINAPI *pGetThreadUILanguage)(void) = (void*)GetProcAddress(kernel32, "GetThreadUILanguage");

    is_english = ((!pGetThreadUILanguage || PRIMARYLANGID(pGetThreadUILanguage()) == LANG_ENGLISH) &&
                  PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_ENGLISH &&
                  PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH);

    if(!is_english)
        skip("Skipping some tests in non-English locale\n");
}

static BOOL check_ie(void)
{
    IHTMLDocument2 *doc;
    IHTMLDocument5 *doc5;
    IHTMLDocument7 *doc7;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return FALSE;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument7, (void**)&doc7);
    if(SUCCEEDED(hres)) {
        is_ie9plus = TRUE;
        IHTMLDocument7_Release(doc7);
    }

    trace("is_ie9plus %x\n", is_ie9plus);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument5, (void**)&doc5);
    if(SUCCEEDED(hres))
        IHTMLDocument5_Release(doc5);

    IHTMLDocument2_Release(doc);
    return SUCCEEDED(hres);
}

START_TEST(script)
{
    int argc;
    char **argv;

    argc = winetest_get_mainargs(&argv);
    CoInitialize(NULL);
    main_thread_id = GetCurrentThreadId();
    container_hwnd = create_container_window();

    detect_locale();
    if(argc > 2) {
        init_protocol_handler();
        run_script_as_http_with_mode(argv[2], NULL, "11");
    }else if(check_ie()) {
        if(winetest_interactive || ! is_ie_hardened()) {
            if(register_script_engine()) {
                test_simple_script();
                init_registry(FALSE);
            }else {
                skip("Could not register TestScript engines\n");
            }
            run_js_tests();
        }else {
            skip("IE running in Enhanced Security Configuration\n");
        }
    }else {
#if !defined(__i386__) && !defined(__x86_64__)
        todo_wine
#endif
        win_skip("Too old IE.\n");
    }

    DestroyWindow(container_hwnd);
    CoUninitialize();
}
