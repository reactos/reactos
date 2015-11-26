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

#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <ole2.h>
#include <dispex.h>
#include <activscp.h>

#include <vbsregexp55.h>

#include <wine/test.h>

#ifdef _WIN64

#define IActiveScriptParse_QueryInterface IActiveScriptParse64_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_64_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_64_ParseProcedureText

#else

#define IActiveScriptParse_QueryInterface IActiveScriptParse32_QueryInterface
#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText
#define IActiveScriptParseProcedure2_Release IActiveScriptParseProcedure2_32_Release
#define IActiveScriptParseProcedure2_ParseProcedureText IActiveScriptParseProcedure2_32_ParseProcedureText

#endif

extern const CLSID CLSID_VBScript;
extern const CLSID CLSID_VBScriptRegExp;

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

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(global_success_d);
DEFINE_EXPECT(global_success_i);
DEFINE_EXPECT(global_vbvar_d);
DEFINE_EXPECT(global_vbvar_i);
DEFINE_EXPECT(global_letobj_i);
DEFINE_EXPECT(global_setobj_i);
DEFINE_EXPECT(testobj_propget_d);
DEFINE_EXPECT(testobj_propget_i);
DEFINE_EXPECT(testobj_propput_d);
DEFINE_EXPECT(testobj_propput_i);
DEFINE_EXPECT(testobj_value_i);
DEFINE_EXPECT(global_propargput_d);
DEFINE_EXPECT(global_propargput_i);
DEFINE_EXPECT(global_propargput1_d);
DEFINE_EXPECT(global_propargput1_i);
DEFINE_EXPECT(global_testoptionalarg_i);
DEFINE_EXPECT(collectionobj_newenum_i);
DEFINE_EXPECT(Next);
DEFINE_EXPECT(GetWindow);
DEFINE_EXPECT(GetUIBehavior);
DEFINE_EXPECT(EnableModeless);
DEFINE_EXPECT(OnScriptError);

#define DISPID_GLOBAL_REPORTSUCCESS 1000
#define DISPID_GLOBAL_TRACE         1001
#define DISPID_GLOBAL_OK            1002
#define DISPID_GLOBAL_GETVT         1003
#define DISPID_GLOBAL_ISENGLANG     1004
#define DISPID_GLOBAL_VBVAR         1005
#define DISPID_GLOBAL_TESTOBJ       1006
#define DISPID_GLOBAL_ISNULLDISP    1007
#define DISPID_GLOBAL_TESTDISP      1008
#define DISPID_GLOBAL_REFOBJ        1009
#define DISPID_GLOBAL_COUNTER       1010
#define DISPID_GLOBAL_PROPARGPUT    1011
#define DISPID_GLOBAL_PROPARGPUT1   1012
#define DISPID_GLOBAL_COLLOBJ       1013
#define DISPID_GLOBAL_DOUBLEASSTRING 1014
#define DISPID_GLOBAL_TESTARRAY     1015
#define DISPID_GLOBAL_THROWINT      1016
#define DISPID_GLOBAL_TESTOPTIONALARG 1017
#define DISPID_GLOBAL_LETOBJ        1018
#define DISPID_GLOBAL_SETOBJ        1019

#define DISPID_TESTOBJ_PROPGET      2000
#define DISPID_TESTOBJ_PROPPUT      2001

#define DISPID_COLLOBJ_RESET        3000

#define FACILITY_VBS 0xa
#define MAKE_VBSERROR(code) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_VBS, code)

static const WCHAR testW[] = {'t','e','s','t',0};
static const WCHAR emptyW[] = {0};

static BOOL strict_dispid_check, is_english, allow_ui;
static const char *test_name = "(null)";
static int test_counter;
static SCRIPTUICHANDLING uic_handling = SCRIPTUICHANDLING_NOUIERROR;
static IDispatchEx testObj;
static HRESULT onerror_hres = E_NOTIMPL;

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), 0, 0);
    return lstrcmpA(buf, stra);
}

static const char *vt2a(VARIANT *v)
{
    if(V_VT(v) == (VT_BYREF|VT_VARIANT)) {
        static char buf[64];
        sprintf(buf, "%s*", vt2a(V_BYREF(v)));
        return buf;
    }

    switch(V_VT(v)) {
    case VT_EMPTY:
        return "VT_EMPTY";
    case VT_NULL:
        return "VT_NULL";
    case VT_I2:
        return "VT_I2";
    case VT_I4:
        return "VT_I4";
    case VT_R4:
        return "VT_R4";
    case VT_R8:
        return "VT_R8";
    case VT_CY:
        return "VT_CY";
    case VT_DATE:
        return "VT_DATE";
    case VT_BSTR:
        return "VT_BSTR";
    case VT_DISPATCH:
        return "VT_DISPATCH";
    case VT_BOOL:
        return "VT_BOOL";
    case VT_ARRAY|VT_VARIANT:
        return "VT_ARRAY|VT_VARIANT";
    case VT_ARRAY|VT_BYREF|VT_VARIANT:
        return "VT_ARRAY|VT_BYREF|VT_VARIANT";
    case VT_UI1:
        return "VT_UI1";
    default:
        ok(0, "unknown vt %d\n", V_VT(v));
        return NULL;
    }
}

/* Returns true if the user interface is in English. Note that this does not
 * presume of the formatting of dates, numbers, etc.
 */
static BOOL is_lang_english(void)
{
    static HMODULE hkernel32 = NULL;
    static LANGID (WINAPI *pGetThreadUILanguage)(void) = NULL;
    static LANGID (WINAPI *pGetUserDefaultUILanguage)(void) = NULL;

    if (!hkernel32)
    {
        hkernel32 = GetModuleHandleA("kernel32.dll");
        pGetThreadUILanguage = (void*)GetProcAddress(hkernel32, "GetThreadUILanguage");
        pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");
    }
    if (pGetThreadUILanguage && PRIMARYLANGID(pGetThreadUILanguage()) != LANG_ENGLISH)
        return FALSE;
    if (pGetUserDefaultUILanguage && PRIMARYLANGID(pGetUserDefaultUILanguage()) != LANG_ENGLISH)
        return FALSE;

    return PRIMARYLANGID(GetUserDefaultLangID()) == LANG_ENGLISH;
}

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

static void test_disp(IDispatch *disp)
{
    DISPID id, public_prop_id, public_prop2_id, public_func_id, public_sub_id, defvalget_id, gs_getter_id;
    DISPID named_args[5] = {DISPID_PROPERTYPUT};
    VARIANT v, args[5];
    DISPPARAMS dp = {args, named_args};
    IDispatchEx *dispex;
    EXCEPINFO ei = {0};
    BSTR str;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);

    str = a2bstr("publicProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08x\n", hres);

    str = a2bstr("PUBLICPROP");
    hres = IDispatchEx_GetDispID(dispex, str, 0, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(PUBLICPROP) failed: %08x\n", hres);
    ok(public_prop_id == id, "id = %d\n", public_prop_id);

    str = a2bstr("publicPROP2");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_prop2_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp2) failed: %08x\n", hres);

    str = a2bstr("defValGet");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &defvalget_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(defValGet) failed: %08x\n", hres);
    ok(defvalget_id == DISPID_VALUE, "id = %d\n", defvalget_id);

    str = a2bstr("privateProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateProp) failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %d\n", id);

    str = a2bstr("class_initialize");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp2) failed: %08x\n", hres);

    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cArgs = dp.cNamedArgs = 1;
    V_VT(&v) = VT_BOOL;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v), "V_BOOL(v) = %x\n", V_BOOL(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_Invoke(dispex, public_prop_id, &IID_NULL, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v), "V_BOOL(v) = %x\n", V_BOOL(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v), "V_BOOL(v) = %x\n", V_BOOL(&v));

    dp.cArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx failed: %08x, expected DISP_E_MEMBERNOTFOUND\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    SET_EXPECT(testobj_value_i);
    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&testObj;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    CHECK_CALLED(testobj_value_i);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 0, "V_I2(v) = %d\n", V_I2(&v));

    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&testObj;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    V_VT(args) = VT_DISPATCH;
    V_DISPATCH(args) = (IDispatch*)&testObj;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&testObj, "V_DISPATCH(v) != testObj\n");

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_BOOL(&v) == VARIANT_TRUE, "V_BOOL(v) = %x\n", V_BOOL(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_FALSE;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUTREF, &dp, NULL, &ei, NULL);
    ok(hres == DISP_E_EXCEPTION, "InvokeEx failed: %08x, expected DISP_E_EXCEPTION\n", hres);

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_FALSE;
    dp.cArgs = 1;
    dp.cNamedArgs = 0;
    V_VT(&v) = VT_BOOL;
    hres = IDispatchEx_InvokeEx(dispex, public_prop_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(hres == DISP_E_PARAMNOTOPTIONAL, "InvokeEx failed: %08x, expected DISP_E_PARAMNOTOPTIONAL\n", hres);

    str = a2bstr("publicFunction");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_func_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicFunction) failed: %08x\n", hres);
    ok(public_func_id != -1, "public_func_id = -1\n");

    str = a2bstr("publicSub");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &public_sub_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicSub) failed: %08x\n", hres);
    ok(public_sub_id != -1, "public_func_id = -1\n");

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx failed: %08x, expected DISP_E_MEMBERNOTFOUND\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == DISP_E_MEMBERNOTFOUND, "InvokeEx failed: %08x, expected DISP_E_MEMBERNOTFOUND\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cArgs = dp.cNamedArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok(FAILED(hres), "InvokeEx succeeded: %08x\n", hres);

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_func_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_Invoke(dispex, public_func_id, &IID_NULL, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I2, "V_VT(v) = %d\n", V_VT(&v));
    ok(V_I2(&v) == 4, "V_I2(v) = %d\n", V_I2(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    dp.cArgs = dp.cNamedArgs = 0;
    hres = IDispatchEx_InvokeEx(dispex, public_sub_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(v) = %d\n", V_VT(&v));

    str = a2bstr("privateSub");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateSub) failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %d\n", id);

    str = a2bstr("dynprop");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive|fdexNameEnsure, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateProp) failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %d\n", id);
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameEnsure, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID(privateProp) failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    ok(id == -1, "id = %d\n", id);
    SysFreeString(str);

    str = a2bstr("publicProp");
    hres = IDispatchEx_GetDispID(dispex, str, 0x80000000|fdexNameCaseInsensitive, &public_prop_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08x\n", hres);

    id = 0xdeadbeef;
    str = a2bstr("publicProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08x\n", hres);
    ok(id == public_prop_id, "id = %d, expected %d\n", id, public_prop_id);

    id = 0xdeadbeef;
    str = a2bstr("publicprop");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseSensitive, &id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicProp) failed: %08x\n", hres);
    ok(id == public_prop_id, "id = %d, expected %d\n", id, public_prop_id);

    str = a2bstr("gsGetProp");
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameCaseInsensitive, &gs_getter_id);
    SysFreeString(str);
    ok(hres == S_OK, "GetDispID(publicFunction) failed: %08x\n", hres);
    ok(gs_getter_id != -1, "gs_getter_id = -1\n");

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    dp.cNamedArgs = 0;
    dp.cArgs = 1;
    V_VT(&v) = VT_I8;
    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_PROPERTYGET, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL && V_BOOL(&v), "V_VT(v) = %d\n", V_VT(&v));

    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_PROPERTYGET, &dp, NULL, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_FALSE;
    dp.cArgs = 1;
    V_VT(&v) = VT_I8;
    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL && !V_BOOL(&v), "V_VT(v) = %d\n", V_VT(&v));

    V_VT(args) = VT_BOOL;
    V_BOOL(args) = VARIANT_TRUE;
    V_VT(&v) = VT_I8;
    dp.cArgs = 1;
    hres = IDispatchEx_InvokeEx(dispex, gs_getter_id, 0, DISPATCH_METHOD, &dp, &v, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BOOL && V_BOOL(&v), "V_VT(v) = %d\n", V_VT(&v));

    IDispatchEx_Release(dispex);
}

static void test_safearray(SAFEARRAY *safearray, unsigned indims)
{
    int i, exdims = indims;

    if(!exdims)
        exdims = 1;
    ok(safearray->cDims == exdims, "safearray->cDims = %d, expected %d\n", safearray->cDims, exdims);
    todo_wine
    ok(safearray->fFeatures == (FADF_VARIANT|FADF_HAVEVARTYPE|FADF_FIXEDSIZE|FADF_STATIC),
       "safearray->fFeatures = %x\n", safearray->fFeatures);
    ok(safearray->cbElements == sizeof(VARIANT), "safearray->cbElements = %x\n", safearray->cbElements);
    ok(!safearray->cLocks, "safearray->cLocks = %x\n", safearray->cLocks);

    for(i=0; i < safearray->cDims; i++) {
        ok(safearray->rgsabound[i].cElements == indims ? i+4 : 1, "safearray->rgsabound[%d].cElements = %d\n", i,
           safearray->rgsabound[i].cElements);
        ok(!safearray->rgsabound[i].lLbound, "safearray->rgsabound[%d].lLbound = %d\n", i, safearray->rgsabound[i].lLbound);
    }
}

#define test_grfdex(a,b) _test_grfdex(__LINE__,a,b)
static void _test_grfdex(unsigned line, DWORD grfdex, DWORD expect)
{
    ok_(__FILE__,line)(grfdex == expect, "grfdex = %x, expected %x\n", grfdex, expect);
}

static IDispatchEx enumDisp;

static HRESULT WINAPI EnumVARIANT_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IEnumVARIANT)) {
        *ppv = iface;
        return S_OK;
    }

    if(IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = &enumDisp;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumVARIANT_AddRef(IEnumVARIANT *iface)
{
    return 2;
}

static ULONG WINAPI EnumVARIANT_Release(IEnumVARIANT *iface)
{
    return 1;
}

static unsigned next_cnt;

static HRESULT WINAPI EnumVARIANT_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    if(strict_dispid_check)
        CHECK_EXPECT2(Next);

    ok(celt == 1, "celt = %d\n", celt);
    ok(V_VT(rgVar) == VT_EMPTY, "V_VT(rgVar) = %d\n", V_VT(rgVar));
    ok(!pCeltFetched, "pCeltFetched = %p\n", pCeltFetched);

    if(next_cnt++ < 3) {
        V_VT(rgVar) = VT_I2;
        V_I2(rgVar) = next_cnt;
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT WINAPI EnumVARIANT_Skip(IEnumVARIANT *iface, ULONG celt)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumVARIANT_Reset(IEnumVARIANT *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumVARIANT_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl EnumVARIANTVtbl = {
    EnumVARIANT_QueryInterface,
    EnumVARIANT_AddRef,
    EnumVARIANT_Release,
    EnumVARIANT_Next,
    EnumVARIANT_Skip,
    EnumVARIANT_Reset,
    EnumVARIANT_Clone
};

static IEnumVARIANT enumObj = { &EnumVARIANTVtbl };

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else {
        trace("QI %s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
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
    ok(0, "unexpected call\n");
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
    ok(0, "unexpected call %s %x\n", wine_dbgstr_w(bstrName), grfdex);
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
    ok(0, "unexpected call %d\n", id);
    return E_NOTIMPL;
}

static HRESULT WINAPI testObj_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "propget")) {
        CHECK_EXPECT(testobj_propget_d);
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_TESTOBJ_PROPGET;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "propput")) {
        CHECK_EXPECT(testobj_propput_d);
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_TESTOBJ_PROPPUT;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI testObj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_VALUE: {
        VARIANT *arg;
        int i;

        CHECK_EXPECT(testobj_value_i);

        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        for(i=0; i<pdp->cArgs; i++) {
            arg = pdp->rgvarg+pdp->cArgs-i-1;
            ok(V_VT(arg) == VT_I2, "V_VT(arg) = %d\n", V_VT(arg));
            ok(V_I2(arg) == i+1, "V_I2(arg) = %d\n", V_I2(arg));
        }

        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = pdp->cArgs;
        return S_OK;
    }
    case DISPID_TESTOBJ_PROPGET:
        CHECK_EXPECT(testobj_propget_i);

        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = 10;
        return S_OK;
    case DISPID_TESTOBJ_PROPPUT:
        CHECK_EXPECT(testobj_propput_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 1, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));
        return S_OK;
    }

    ok(0, "unexpected call %d\n", id);
    return E_FAIL;
}

static IDispatchExVtbl testObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    testObj_GetDispID,
    testObj_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx testObj = { &testObjVtbl };

static HRESULT WINAPI enumDisp_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    return IEnumVARIANT_QueryInterface(&enumObj, riid, ppv);
}

static IDispatchExVtbl enumDispVtbl = {
    enumDisp_QueryInterface,
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

static IDispatchEx enumDisp = { &enumDispVtbl };

static HRESULT WINAPI collectionObj_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "reset")) {
        *pid = DISPID_COLLOBJ_RESET;
        return S_OK;
    }

    ok(0, "unexpected call %s\n", wine_dbgstr_w(bstrName));
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI collectionObj_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_NEWENUM:
        if(strict_dispid_check)
            CHECK_EXPECT(collectionobj_newenum_i);

        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_UNKNOWN;
        V_UNKNOWN(pvarRes) = (IUnknown*)&enumObj;
        return S_OK;
    case DISPID_COLLOBJ_RESET:
        next_cnt = 0;
        return S_OK;
    }

    ok(0, "unexpected call %d\n", id);
    return E_NOTIMPL;
}

static IDispatchExVtbl collectionObjVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    collectionObj_GetDispID,
    collectionObj_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx collectionObj = { &collectionObjVtbl };

static ULONG refobj_ref;

static ULONG WINAPI RefObj_AddRef(IDispatchEx *iface)
{
    return ++refobj_ref;
}

static ULONG WINAPI RefObj_Release(IDispatchEx *iface)
{
    return --refobj_ref;
}

static IDispatchExVtbl RefObjVtbl = {
    DispatchEx_QueryInterface,
    RefObj_AddRef,
    RefObj_Release,
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

static IDispatchEx RefObj = { &RefObjVtbl };

static HRESULT WINAPI Global_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if(!strcmp_wa(bstrName, "ok")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_OK;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "trace")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_TRACE;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "reportSuccess")) {
        CHECK_EXPECT(global_success_d);
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_REPORTSUCCESS;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "getVT")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_GETVT;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "isEnglishLang")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_ISENGLANG;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testObj")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_TESTOBJ;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "collectionObj")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_COLLOBJ;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "vbvar")) {
        CHECK_EXPECT(global_vbvar_d);
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_VBVAR;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "letobj")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_LETOBJ;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "setobj")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_SETOBJ;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "isNullDisp")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_ISNULLDISP;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testDisp")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_TESTDISP;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "RefObj")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_REFOBJ;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "propargput")) {
        CHECK_EXPECT(global_propargput_d);
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_PROPARGPUT;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "propargput1")) {
        CHECK_EXPECT(global_propargput1_d);
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_PROPARGPUT1;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "counter")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_COUNTER;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "doubleAsString")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_DOUBLEASSTRING;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testArray")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_TESTARRAY;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "throwInt")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_THROWINT;
        return S_OK;
    }
    if(!strcmp_wa(bstrName, "testOptionalArg")) {
        test_grfdex(grfdex, fdexNameCaseInsensitive);
        *pid = DISPID_GLOBAL_TESTOPTIONALARG;
        return S_OK;
    }

    if(strict_dispid_check && strcmp_wa(bstrName, "x"))
        ok(0, "unexpected call %s %x\n", wine_dbgstr_w(bstrName), grfdex);
    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI Global_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    switch(id) {
    case DISPID_GLOBAL_OK: {
        VARIANT *b;

        ok(wFlags == INVOKE_FUNC || wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        if(wFlags & INVOKE_PROPERTYGET)
            ok(pvarRes != NULL, "pvarRes == NULL\n");
        else
            ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_BSTR, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));

        b = pdp->rgvarg+1;
        if(V_VT(b) == (VT_BYREF|VT_VARIANT))
            b = V_BYREF(b);

        ok(V_VT(b) == VT_BOOL, "V_VT(b) = %d\n", V_VT(b));

        ok(V_BOOL(b), "%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));
        return S_OK;
    }

    case DISPID_GLOBAL_TRACE:
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
            trace("%s: %s\n", test_name, wine_dbgstr_w(V_BSTR(pdp->rgvarg)));

        return S_OK;

    case DISPID_GLOBAL_REPORTSUCCESS:
        CHECK_EXPECT(global_success_i);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        return S_OK;

    case DISPID_GLOBAL_GETVT:
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BSTR;
        V_BSTR(pvarRes) = a2bstr(vt2a(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_ISENGLANG:
        ok(wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 0, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = is_english ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;

    case DISPID_GLOBAL_VBVAR:
        CHECK_EXPECT(global_vbvar_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 3, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_LETOBJ:
        CHECK_EXPECT(global_letobj_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) == (IDispatch*)&testObj, "V_DISPATCH(psp->rgvargs) != testObj\n");
        return S_OK;

    case DISPID_GLOBAL_SETOBJ:
        CHECK_EXPECT(global_setobj_i);

        ok(wFlags == DISPATCH_PROPERTYPUTREF, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_DISPATCH(pdp->rgvarg) == (IDispatch*)&testObj, "V_DISPATCH(psp->rgvargs) != testObj\n");
        return S_OK;

    case DISPID_GLOBAL_TESTOBJ:
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&testObj;
        return S_OK;

    case DISPID_GLOBAL_COLLOBJ:
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg != NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&collectionObj;
        return S_OK;

    case DISPID_GLOBAL_REFOBJ:
        ok(wFlags == (DISPATCH_PROPERTYGET|DISPATCH_METHOD), "wFlags = %x\n", wFlags);

        ok(pdp != NULL, "pdp == NULL\n");
        ok(!pdp->rgvarg, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        IDispatchEx_AddRef(&RefObj);
        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = (IDispatch*)&RefObj;
        return S_OK;

    case DISPID_GLOBAL_ISNULLDISP: {
        VARIANT *v;

        ok(wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        v = pdp->rgvarg;
        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        ok(V_VT(v) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        V_VT(pvarRes) = VT_BOOL;
        V_BOOL(pvarRes) = V_DISPATCH(v) ? VARIANT_FALSE : VARIANT_TRUE;
        return S_OK;
    }

    case DISPID_GLOBAL_TESTDISP:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        test_disp(V_DISPATCH(pdp->rgvarg));
        return S_OK;

    case DISPID_GLOBAL_PROPARGPUT:
        CHECK_EXPECT(global_propargput_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 0, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I2(pdp->rgvarg+1) == 2, "V_I2(psp->rgvargs+1) = %d\n", V_I2(pdp->rgvarg+1));

        ok(V_VT(pdp->rgvarg+2) == VT_I2, "V_VT(psp->rgvargs+2) = %d\n", V_VT(pdp->rgvarg+2));
        ok(V_I2(pdp->rgvarg+2) == 1, "V_I2(psp->rgvargs+2) = %d\n", V_I2(pdp->rgvarg+2));
        return S_OK;

    case DISPID_GLOBAL_PROPARGPUT1:
        CHECK_EXPECT(global_propargput1_i);

        ok(wFlags == DISPATCH_PROPERTYPUT, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(pdp->rgdispidNamedArgs != NULL, "rgdispidNamedArgs == NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(pdp->cNamedArgs == 1, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pdp->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT, "pdp->rgdispidNamedArgs[0] = %d\n", pdp->rgdispidNamedArgs[0]);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_I2(pdp->rgvarg) == 0, "V_I2(psp->rgvargs) = %d\n", V_I2(pdp->rgvarg));

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_I2(pdp->rgvarg+1) == 1, "V_I2(psp->rgvargs+1) = %d\n", V_I2(pdp->rgvarg+1));

        return S_OK;

    case DISPID_GLOBAL_COUNTER:
        ok(pdp != NULL, "pdp == NULL\n");
        todo_wine ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(!pdp->cArgs, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pvarRes != NULL, "pvarRes == NULL\n");
        ok(V_VT(pvarRes) ==  VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
        ok(pei != NULL, "pei == NULL\n");

        V_VT(pvarRes) = VT_I2;
        V_I2(pvarRes) = test_counter++;
        return S_OK;

    case DISPID_GLOBAL_DOUBLEASSTRING:
        ok(wFlags == (INVOKE_FUNC|INVOKE_PROPERTYGET), "wFlags = %x\n", wFlags);
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(V_VT(pdp->rgvarg) == VT_R8, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        ok(pvarRes != NULL, "pvarRes == NULL\n");

        V_VT(pvarRes) = VT_BSTR;
        return VarBstrFromR8(V_R8(pdp->rgvarg), 0, 0, &V_BSTR(pvarRes));

    case DISPID_GLOBAL_TESTARRAY:
        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 2, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg+1) == VT_I2, "V_VT(psp->rgvargs+1) = %d\n", V_VT(pdp->rgvarg+1));
        ok(V_VT(pdp->rgvarg) == (VT_BYREF|VT_VARIANT), "V_VT(psp->rgvargs) = %d\n", V_VT(pdp->rgvarg));
        ok(V_VT(V_VARIANTREF(pdp->rgvarg)) == (VT_ARRAY|VT_BYREF|VT_VARIANT),
           "V_VT(V_VARIANTREF(psp->rgvargs)) = %d\n", V_VT(V_VARIANTREF(pdp->rgvarg)));
        if(V_I2(pdp->rgvarg+1) == -1)
            ok(!*V_ARRAYREF(V_VARIANTREF(pdp->rgvarg)), "*V_ARRAYREF(V_VARIANTREF(pdp->rgvarg)) != NULL\n");
        else
            test_safearray(*V_ARRAYREF(V_VARIANTREF(pdp->rgvarg)), V_I2(pdp->rgvarg+1));
        return S_OK;

    case DISPID_GLOBAL_THROWINT: {
        VARIANT *v = pdp->rgvarg;
        HRESULT hres;

        ok((wFlags & ~INVOKE_PROPERTYGET) == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 1, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(pei != NULL, "pei == NULL\n");
        if(pvarRes) {
            ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pvarRes) = %d\n", V_VT(pvarRes));
            V_VT(pvarRes) = VT_BOOL;
            V_BOOL(pvarRes) = VARIANT_FALSE;
        }

        if(V_VT(v) == (VT_VARIANT|VT_BYREF))
            v = V_VARIANTREF(v);

        switch(V_VT(v)) {
        case VT_I2:
            hres = V_I2(v);
            break;
        case VT_I4:
            hres = V_I4(v);
            break;
        default:
            ok(0, "unexpected vt %d\n", V_VT(v));
            return E_INVALIDARG;
        }

        return hres;
    }

    case DISPID_GLOBAL_TESTOPTIONALARG: {
        VARIANT *v;
        int opt;

        CHECK_EXPECT(global_testoptionalarg_i);

        ok(wFlags == INVOKE_FUNC, "wFlags = %x\n", wFlags);
        ok(pdp != NULL, "pdp == NULL\n");
        ok(pdp->rgvarg != NULL, "rgvarg == NULL\n");
        ok(!pdp->rgdispidNamedArgs, "rgdispidNamedArgs != NULL\n");
        ok(pdp->cArgs == 3, "cArgs = %d\n", pdp->cArgs);
        ok(!pdp->cNamedArgs, "cNamedArgs = %d\n", pdp->cNamedArgs);
        ok(!pvarRes, "pvarRes != NULL\n");
        ok(pei != NULL, "pei == NULL\n");

        ok(V_VT(pdp->rgvarg) == VT_I2, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
        opt = V_I2(pdp->rgvarg);
        ok(opt == 1 || opt == 2, "opt = %d\n", opt);
        v = pdp->rgvarg+pdp->cArgs-opt;
        ok(V_VT(v) == VT_ERROR, "V_VT(v) = %d\n", V_VT(v));
        ok(V_ERROR(v) == DISP_E_PARAMNOTFOUND, "V_ERROR(v) = %08x\n", V_ERROR(v));
        return S_OK;
    }
    }

    ok(0, "unexpected call %d\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static IDispatchExVtbl GlobalVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    Global_GetDispID,
    Global_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx Global = { &GlobalVtbl };

static HRESULT WINAPI ActiveScriptSiteWindow_QueryInterface(IActiveScriptSiteWindow *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptSiteWindow_AddRef(IActiveScriptSiteWindow *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSiteWindow_Release(IActiveScriptSiteWindow *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSiteWindow_GetWindow(IActiveScriptSiteWindow *iface, HWND *phwnd)
{
    if(!allow_ui)
        CHECK_EXPECT(GetWindow);
    *phwnd = NULL;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSiteWindow_EnableModeless(IActiveScriptSiteWindow *iface, BOOL fEnable)
{
    if(allow_ui)
        return S_OK;

    CHECK_EXPECT(EnableModeless);
    ok(!fEnable, "fEnable = %x\n", fEnable);
    return E_FAIL;
}

static const IActiveScriptSiteWindowVtbl ActiveScriptSiteWindowVtbl = {
    ActiveScriptSiteWindow_QueryInterface,
    ActiveScriptSiteWindow_AddRef,
    ActiveScriptSiteWindow_Release,
    ActiveScriptSiteWindow_GetWindow,
    ActiveScriptSiteWindow_EnableModeless
};

static IActiveScriptSiteWindow ActiveScriptSiteWindow = { &ActiveScriptSiteWindowVtbl };

static HRESULT WINAPI ActiveScriptSiteUIControl_QueryInterface(IActiveScriptSiteUIControl *iface, REFIID riid, void **ppv)
{
    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI ActiveScriptSiteUIControl_AddRef(IActiveScriptSiteUIControl *iface)
{
    return 2;
}

static ULONG WINAPI ActiveScriptSiteUIControl_Release(IActiveScriptSiteUIControl *iface)
{
    return 1;
}

static HRESULT WINAPI ActiveScriptSiteUIControl_GetUIBehavior(IActiveScriptSiteUIControl *iface, SCRIPTUICITEM UicItem,
        SCRIPTUICHANDLING *pUicHandling)
{
    if(!allow_ui) {
        CHECK_EXPECT(GetUIBehavior);
        ok(UicItem == SCRIPTUICITEM_MSGBOX, "UidItem = %d\n", UicItem);
    }
    *pUicHandling = uic_handling;
    return S_OK;
}

static const IActiveScriptSiteUIControlVtbl ActiveScriptSiteUIControlVtbl = {
    ActiveScriptSiteUIControl_QueryInterface,
    ActiveScriptSiteUIControl_AddRef,
    ActiveScriptSiteUIControl_Release,
    ActiveScriptSiteUIControl_GetUIBehavior
};

static IActiveScriptSiteUIControl ActiveScriptSiteUIControl = { &ActiveScriptSiteUIControlVtbl };

static HRESULT WINAPI ActiveScriptSite_QueryInterface(IActiveScriptSite *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSite, riid))
        *ppv = iface;
    else if(IsEqualGUID(&IID_IActiveScriptSiteWindow, riid))
        *ppv = &ActiveScriptSiteWindow;
    else if(IsEqualGUID(&IID_IActiveScriptSiteUIControl, riid))
        *ppv = &ActiveScriptSiteUIControl;
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
    *plcid = GetUserDefaultLCID();
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetItemInfo(IActiveScriptSite *iface, LPCOLESTR pstrName,
        DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
    ok(dwReturnMask == SCRIPTINFO_IUNKNOWN, "unexpected dwReturnMask %x\n", dwReturnMask);
    ok(!ppti, "ppti != NULL\n");

    if(strcmp_wa(pstrName, "test"))
        ok(0, "unexpected pstrName %s\n", wine_dbgstr_w(pstrName));

    *ppiunkItem = (IUnknown*)&Global;
    return S_OK;
}

static HRESULT WINAPI ActiveScriptSite_GetDocVersionString(IActiveScriptSite *iface, BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptTerminate(IActiveScriptSite *iface,
        const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnStateChange(IActiveScriptSite *iface, SCRIPTSTATE ssScriptState)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnScriptError(IActiveScriptSite *iface, IActiveScriptError *pscripterror)
{
    HRESULT hr = onerror_hres;
    CHECK_EXPECT(OnScriptError);

    onerror_hres = E_NOTIMPL;

    return hr;
}

static HRESULT WINAPI ActiveScriptSite_OnEnterScript(IActiveScriptSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ActiveScriptSite_OnLeaveScript(IActiveScriptSite *iface)
{
    return E_NOTIMPL;
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

static IActiveScript *create_script(void)
{
    IActiveScript *script;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScript, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IActiveScript, (void**)&script);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    return script;
}

static IActiveScript *create_and_init_script(DWORD flags)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return NULL;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    IActiveScriptParse_Release(parser);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|flags);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    return engine;
}

static void close_script(IActiveScript *script)
{
    ULONG ref;
    HRESULT hres;

    hres = IActiveScript_Close(script);
    ok(hres == S_OK, "Close failed: %08x\n", hres);

    ref = IActiveScript_Release(script);
    ok(!ref, "ref=%u\n", ref);
}

static HRESULT parse_script(DWORD flags, BSTR script_str, const WCHAR *delim)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    IDispatch *script_disp;
    LONG ref;
    HRESULT hres;

    engine = create_and_init_script(flags);
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return hres;
    }

    hres = IActiveScript_GetScriptDispatch(engine, NULL, &script_disp);
    ok(hres == S_OK, "GetScriptDispatch failed: %08x\n", hres);
    ok(script_disp != NULL, "script_disp == NULL\n");
    ok(script_disp != (IDispatch*)&Global, "script_disp == Global\n");

    test_counter = 0;

    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, delim, 0, 0, 0, NULL, NULL);

    IActiveScript_Close(engine);

    IDispatch_Release(script_disp);
    IActiveScript_Release(engine);

    ref = IActiveScriptParse_Release(parser);
    ok(!ref, "ref=%d\n", ref);
    return hres;
}

static void parse_script_af(DWORD flags, const char *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(src);
    hres = parse_script(flags, tmp, NULL);
    SysFreeString(tmp);
    ok(hres == S_OK, "parse_script failed: %08x\n", hres);
}

static HRESULT parse_script_ar(const char *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(src);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, tmp, NULL);
    SysFreeString(tmp);
    return hres;
}

static void parse_script_a(const char *src)
{
    parse_script_af(SCRIPTITEM_GLOBALMEMBERS, src);
}

#define parse_htmlscript_a(a) _parse_htmlscript_a(__LINE__,a)
static void _parse_htmlscript_a(unsigned line, const char *src)
{
    BSTR tmp;
    HRESULT hres;

    static const WCHAR script_delimW[] = {'<','/','S','C','R','I','P','T','>',0};

    tmp = a2bstr(src);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, tmp, script_delimW);
    SysFreeString(tmp);
    ok_(__FILE__,line)(hres == S_OK, "parse_script failed: %08x\n", hres);
}

static IDispatchEx *parse_procedure(IActiveScriptParseProcedure2 *parse_proc, const char *src)
{
    IDispatchEx *dispex;
    IDispatch *disp;
    BSTR str;
    HRESULT hres;

    static const WCHAR delimiterW[] = {'\"',0};

    str = a2bstr(src);
    hres = IActiveScriptParseProcedure2_ParseProcedureText(parse_proc, str, NULL, emptyW, NULL, NULL, delimiterW, 0, 0,
            SCRIPTPROC_HOSTMANAGESSOURCE|SCRIPTPROC_IMPLICIT_THIS|SCRIPTPROC_IMPLICIT_PARENTS, &disp);
    SysFreeString(str);
    ok(hres == S_OK, "ParseProcedureText failed: %08x\n", hres);
    ok(disp != NULL, "disp = NULL\n");

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IDispatchEx iface: %08x\n", hres);

    return dispex;
}


static void test_procedures(void)
{
    IActiveScriptParseProcedure2 *parse_proc;
    DISPPARAMS dp = {NULL};
    IActiveScript *script;
    IDispatchEx *proc;
    EXCEPINFO ei = {0};
    VARIANT v;
    HRESULT hres;

    script = create_and_init_script(0);

    hres = IActiveScript_QueryInterface(script, &IID_IActiveScriptParseProcedure2, (void**)&parse_proc);
    ok(hres == S_OK, "Could not get IActiveScriptParseProcedure2 iface: %08x\n", hres);

    proc = parse_procedure(parse_proc, "dim x\nif true then x=false");

    V_VT(&v) = VT_EMPTY;
    hres = IDispatchEx_InvokeEx(proc, DISPID_VALUE, 0, DISPATCH_METHOD, &dp, &v, &ei, &caller_sp);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);

    IDispatchEx_Release(proc);

    IActiveScriptParseProcedure2_Release(parse_proc);

    close_script(script);
}

static void test_gc(void)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR src;
    HRESULT hres;

    strict_dispid_check = FALSE;

    engine = create_script();
    if(!engine)
        return;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW,
            SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    src = a2bstr(
            "class C\n"
            "    Public ref\n"
            "    Public Sub Class_Terminate\n"
            "        Call reportSuccess()\n"
            "    End Sub\n"
            "End Class\n"
            "Dim x\n"
            "set x = new C\n"
            "set x.ref = x\n"
            "set x = nothing\n");

    hres = IActiveScriptParse_ParseScriptText(parser, src, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(hres == S_OK, "ParseScriptText failed: %08x\n", hres);
    SysFreeString(src);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    IActiveScript_Close(engine);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    IActiveScript_Release(engine);
    IActiveScriptParse_Release(parser);
}

static void test_msgbox(void)
{
    HRESULT hres;

    uic_handling = SCRIPTUICHANDLING_NOUIDEFAULT;

    SET_EXPECT(GetUIBehavior);
    SET_EXPECT(GetWindow);
    SET_EXPECT(EnableModeless);
    hres = parse_script_ar("MsgBox \"testing...\"");
    CLEAR_CALLED(GetUIBehavior);
    CLEAR_CALLED(GetWindow);
    CLEAR_CALLED(EnableModeless);
    if(FAILED(hres)) {
        win_skip("Skipping MsgBox tests, broken (probably too old) vbscript\n");
        return;
    }

    SET_EXPECT(GetUIBehavior);
    parse_script_a("dim r\n r=MsgBox(\"testing...\")\n Call ok(r=0, \"r=\"&r)");
    CHECK_CALLED(GetUIBehavior);

    SET_EXPECT(GetUIBehavior);
    parse_script_a("MsgBox 10");
    CHECK_CALLED(GetUIBehavior);

    uic_handling = SCRIPTUICHANDLING_ALLOW;

    SET_EXPECT(GetUIBehavior);
    SET_EXPECT(GetWindow);
    SET_EXPECT(EnableModeless);
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("MsgBox \"testing...\"");
    ok(FAILED(hres), "script not failed\n");
    CHECK_CALLED(GetUIBehavior);
    CHECK_CALLED(GetWindow);
    CHECK_CALLED(EnableModeless);
    todo_wine CHECK_CALLED(OnScriptError);

    uic_handling = SCRIPTUICHANDLING_NOUIERROR;

    SET_EXPECT(GetUIBehavior);
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("MsgBox \"testing...\"");
    ok(FAILED(hres), "script not failed\n");
    CHECK_CALLED(GetUIBehavior);
    todo_wine CHECK_CALLED(OnScriptError);
}

static HRESULT test_global_vars_ref(BOOL use_close)
{
    IActiveScriptParse *parser;
    IActiveScript *engine;
    BSTR script_str;
    LONG ref;
    HRESULT hres;

    engine = create_script();
    if(!engine)
        return S_OK;

    hres = IActiveScript_QueryInterface(engine, &IID_IActiveScriptParse, (void**)&parser);
    ok(hres == S_OK, "Could not get IActiveScriptParse: %08x\n", hres);
    if (FAILED(hres))
    {
        IActiveScript_Release(engine);
        return hres;
    }

    hres = IActiveScriptParse_InitNew(parser);
    ok(hres == S_OK, "InitNew failed: %08x\n", hres);

    hres = IActiveScript_SetScriptSite(engine, &ActiveScriptSite);
    ok(hres == S_OK, "SetScriptSite failed: %08x\n", hres);

    hres = IActiveScript_AddNamedItem(engine, testW, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE|SCRIPTITEM_GLOBALMEMBERS);
    ok(hres == S_OK, "AddNamedItem failed: %08x\n", hres);

    hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_STARTED);
    ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);

    refobj_ref = 0;

    script_str = a2bstr("Dim x\nset x = RefObj\n");
    hres = IActiveScriptParse_ParseScriptText(parser, script_str, NULL, NULL, NULL, 0, 0, 0, NULL, NULL);
    SysFreeString(script_str);

    ok(refobj_ref, "refobj_ref = 0\n");

    if(use_close) {
        hres = IActiveScript_Close(engine);
        ok(hres == S_OK, "Close failed: %08x\n", hres);
    }else {
        hres = IActiveScript_SetScriptState(engine, SCRIPTSTATE_UNINITIALIZED);
        ok(hres == S_OK, "SetScriptState(SCRIPTSTATE_STARTED) failed: %08x\n", hres);
    }

    ok(!refobj_ref, "refobj_ref = %d\n", refobj_ref);

    IActiveScript_Release(engine);

    ref = IActiveScriptParse_Release(parser);
    ok(!ref, "ref=%d\n", ref);
    return hres;
}

static BSTR get_script_from_file(const char *filename)
{
    DWORD size, len;
    HANDLE file, map;
    const char *file_map;
    BSTR ret;

    file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE) {
        trace("Could not open file: %u\n", GetLastError());
        return NULL;
    }

    size = GetFileSize(file, NULL);

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(map == INVALID_HANDLE_VALUE) {
        trace("Could not create file mapping: %u\n", GetLastError());
        return NULL;
    }

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map) {
        trace("MapViewOfFile failed: %u\n", GetLastError());
        return NULL;
    }

    len = MultiByteToWideChar(CP_ACP, 0, file_map, size, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, file_map, size, ret, len);

    UnmapViewOfFile(file_map);

    return ret;
}

static void run_from_file(const char *filename)
{
    BSTR script_str;
    HRESULT hres;

    script_str = get_script_from_file(filename);
    if(!script_str)
        return;

    strict_dispid_check = FALSE;
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, script_str, NULL);
    SysFreeString(script_str);
    ok(hres == S_OK, "parse_script failed: %08x\n", hres);
}

static void run_from_res(const char *name)
{
    const char *data;
    DWORD size, len;
    BSTR str;
    HRSRC src;
    HRESULT hres;

    strict_dispid_check = FALSE;
    test_name = name;

    src = FindResourceA(NULL, name, (LPCSTR)40);
    ok(src != NULL, "Could not find resource %s\n", name);

    size = SizeofResource(NULL, src);
    data = LoadResource(NULL, src);

    len = MultiByteToWideChar(CP_ACP, 0, data, size, NULL, 0);
    str = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, data, size, str, len);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    hres = parse_script(SCRIPTITEM_GLOBALMEMBERS, str, NULL);
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    ok(hres == S_OK, "parse_script failed: %08x\n", hres);
    SysFreeString(str);
}

static void run_tests(void)
{
    HRESULT hres;

    strict_dispid_check = TRUE;

    parse_script_a("");
    parse_script_a("' empty ;");

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("reportSuccess");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("reportSuccess()");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("Call reportSuccess");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_script_a("test.reportSuccess()");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    SET_EXPECT(global_vbvar_d);
    SET_EXPECT(global_vbvar_i);
    parse_script_a("Option Explicit\nvbvar = 3");
    CHECK_CALLED(global_vbvar_d);
    CHECK_CALLED(global_vbvar_i);

    SET_EXPECT(global_vbvar_d);
    SET_EXPECT(global_vbvar_i);
    parse_script_a("Option Explicit\nvbvar() = 3");
    CHECK_CALLED(global_vbvar_d);
    CHECK_CALLED(global_vbvar_i);

    SET_EXPECT(global_letobj_i);
    parse_script_a("Option Explicit\nletobj = testObj");
    CHECK_CALLED(global_letobj_i);

    SET_EXPECT(global_letobj_i);
    parse_script_a("Option Explicit\ntest.letobj = testObj");
    CHECK_CALLED(global_letobj_i);

    SET_EXPECT(global_setobj_i);
    parse_script_a("Option Explicit\nset setobj = testObj");
    CHECK_CALLED(global_setobj_i);

    SET_EXPECT(global_setobj_i);
    parse_script_a("Option Explicit\nset test.setobj = testObj");
    CHECK_CALLED(global_setobj_i);

    SET_EXPECT(testobj_propget_d);
    SET_EXPECT(testobj_propget_i);
    parse_script_a("dim x\nx = testObj.propget");
    CHECK_CALLED(testobj_propget_d);
    CHECK_CALLED(testobj_propget_i);

    SET_EXPECT(testobj_propput_d);
    SET_EXPECT(testobj_propput_i);
    parse_script_a("testObj.propput = 1");
    CHECK_CALLED(testobj_propput_d);
    CHECK_CALLED(testobj_propput_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    parse_script_a("propargput(counter(), counter()) = counter()");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargput_d);
    SET_EXPECT(global_propargput_i);
    parse_script_a("test.propargput(counter(), counter()) = counter()");
    CHECK_CALLED(global_propargput_d);
    CHECK_CALLED(global_propargput_i);

    SET_EXPECT(global_propargput1_d);
    SET_EXPECT(global_propargput1_i);
    parse_script_a("propargput1 (counter()) = counter()");
    CHECK_CALLED(global_propargput1_d);
    CHECK_CALLED(global_propargput1_i);

    SET_EXPECT(global_propargput1_d);
    SET_EXPECT(global_propargput1_i);
    parse_script_a("test.propargput1(counter()) = counter()");
    CHECK_CALLED(global_propargput1_d);
    CHECK_CALLED(global_propargput1_i);

    parse_htmlscript_a("<!--");
    parse_htmlscript_a(" -->");
    parse_htmlscript_a("<!--\ndim x\nx=1\n-->\n");
    parse_htmlscript_a("<!--\ndim x\n-->\n<!--\nx=1\n-->\n");

    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("<!--");
    ok(FAILED(hres), "script didn't fail\n");
    todo_wine CHECK_CALLED(OnScriptError);

    SET_EXPECT(global_success_d);
    SET_EXPECT(global_success_i);
    parse_htmlscript_a("<!--\n<!-- ignore this <> <>\n--> <>\nCall reportSuccess()\n-->\n");
    CHECK_CALLED(global_success_d);
    CHECK_CALLED(global_success_i);

    next_cnt = 0;
    SET_EXPECT(collectionobj_newenum_i);
    SET_EXPECT(Next);
    parse_script_a("for each x in collectionObj\nnext");
    CHECK_CALLED(collectionobj_newenum_i);
    CHECK_CALLED(Next);
    ok(next_cnt == 4, "next_cnt = %d\n", next_cnt);

    parse_script_a("x = 1\n Call ok(x = 1, \"x = \" & x)");

    parse_script_a("x = _    \n3");

    test_global_vars_ref(TRUE);
    test_global_vars_ref(FALSE);

    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("throwInt(&h80080008&)");
    ok(hres == 0x80080008, "hres = %08x\n", hres);
    todo_wine CHECK_CALLED(OnScriptError);

    /* DISP_E_BADINDEX */
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("throwInt(&h8002000b&)");
    ok(hres == MAKE_VBSERROR(9), "hres = %08x\n", hres);
    todo_wine CHECK_CALLED(OnScriptError);

    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("throwInt(&h800a0009&)");
    ok(hres == MAKE_VBSERROR(9), "hres = %08x\n", hres);
    todo_wine CHECK_CALLED(OnScriptError);

    onerror_hres = S_OK;
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("throwInt(&h800a0009&)");
    todo_wine ok(hres == SCRIPT_E_REPORTED, "hres = %08x\n", hres);
    todo_wine CHECK_CALLED(OnScriptError);

    /* E_NOTIMPL */
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("throwInt(&h80004001&)");
    ok(hres == MAKE_VBSERROR(445), "hres = %08x\n", hres);
    todo_wine CHECK_CALLED(OnScriptError);

    onerror_hres = S_OK;
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("throwInt(&h80004001&)");
    todo_wine ok(hres == SCRIPT_E_REPORTED, "hres = %08x\n", hres);
    todo_wine CHECK_CALLED(OnScriptError);

    SET_EXPECT(global_testoptionalarg_i);
    parse_script_a("call testOptionalArg(1,,2)");
    CHECK_CALLED(global_testoptionalarg_i);

    SET_EXPECT(global_testoptionalarg_i);
    parse_script_a("call testOptionalArg(,1,1)");
    CHECK_CALLED(global_testoptionalarg_i);

    SET_EXPECT(global_testoptionalarg_i);
    parse_script_a("testOptionalArg 1,,2");
    CHECK_CALLED(global_testoptionalarg_i);

    strict_dispid_check = FALSE;

    SET_EXPECT(testobj_value_i);
    parse_script_a("dim n,o\n set o = testObj\n n = o(1,2)\n call ok(n=2, \"n = \" & n)\n");
    CHECK_CALLED(testobj_value_i);

    SET_EXPECT(testobj_value_i);
    parse_script_a("dim n,o\n set o = testObj\n n = o\n call ok(n=0, \"n = \" & n)\n");
    CHECK_CALLED(testobj_value_i);

    parse_script_a("Sub testsub\n"
                   "x = 1\n"
                   "Call ok(x = 1, \"x = \" & x)\n"
                   "End Sub\n"
                   "Call testsub()");

    parse_script_a("Call ok(getVT(x) = \"VT_EMPTY*\", \"getVT(x) = \" & getVT(x))\n");
    parse_script_a("Call ok(x = \"\", \"x = \" & x)\n");
    parse_script_a("x = y\n"
                   "Call ok(getVT(x) = \"VT_EMPTY*\", \"getVT(x) = \" & getVT(x))\n"
                   "Call ok(getVT(y) = \"VT_EMPTY*\", \"getVT(y) = \" & getVT(y))");
    SET_EXPECT(OnScriptError);
    hres = parse_script_ar("x = y(\"a\")");
    ok(FAILED(hres), "script didn't fail\n");
    todo_wine CHECK_CALLED(OnScriptError);

    run_from_res("lang.vbs");
    run_from_res("api.vbs");
    run_from_res("regexp.vbs");
    run_from_res("error.vbs");

    test_procedures();
    test_gc();
    test_msgbox();
}

static BOOL check_vbscript(void)
{
    IRegExp2 *regexp;
    IUnknown *unk;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_VBScriptRegExp, NULL,
            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&unk);
    if(hres == REGDB_E_CLASSNOTREG)
        return FALSE;
    ok(hres == S_OK, "CoCreateInstance(CLSID_VBScriptRegExp) failed: %x\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IRegExp2, (void**)&regexp);
    if(SUCCEEDED(hres))
        IRegExp2_Release(regexp);
    IUnknown_Release(unk);

    return hres == S_OK;
}

START_TEST(run)
{
    int argc;
    char **argv;

    is_english = is_lang_english();
    if(!is_english)
        skip("Skipping some tests in non-English locale\n");

    argc = winetest_get_mainargs(&argv);

    CoInitialize(NULL);

    if(!check_vbscript()) {
        win_skip("Broken engine, probably too old\n");
    }else if(argc > 2) {
        allow_ui = TRUE;
        uic_handling = SCRIPTUICHANDLING_ALLOW;
        run_from_file(argv[2]);
    }else {
        run_tests();
    }

    CoUninitialize();
}
