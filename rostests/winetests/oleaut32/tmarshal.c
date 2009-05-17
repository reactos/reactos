/*
 * Copyright (C) 2005-2006 Robert Shearman for CodeWeavers
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
 *
 */

#define COBJMACROS
#define CONST_VTABLE

#include <windows.h>
#include <ocidl.h>
#include <stdio.h>

#include "wine/test.h"

#include "tmarshal.h"
#include "tmarshal_dispids.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08lx\n", (unsigned long int)hr)

/* ULL suffix is not portable */
#define ULL_CONST(dw1, dw2) ((((ULONGLONG)dw1) << 32) | (ULONGLONG)dw2)

const MYSTRUCT MYSTRUCT_BYVAL = {0x12345678, ULL_CONST(0xdeadbeef, 0x98765432)};
const MYSTRUCT MYSTRUCT_BYPTR = {0x91827364, ULL_CONST(0x88776655, 0x44332211)};
const MYSTRUCT MYSTRUCT_ARRAY[5] = {
    {0x1a1b1c1d, ULL_CONST(0x1e1f1011, 0x12131415)},
    {0x2a2b2c2d, ULL_CONST(0x2e2f2021, 0x22232425)},
    {0x3a3b3c3d, ULL_CONST(0x3e3f3031, 0x32333435)},
    {0x4a4b4c4d, ULL_CONST(0x4e4f4041, 0x42434445)},
    {0x5a5b5c5d, ULL_CONST(0x5e5f5051, 0x52535455)},
};

/* Debugging functions from wine/libs/wine/debug.c */

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_tmp_space( int size )
{
    static char *list[32];
    static long pos;
    char *ret;
    int idx;

    idx = ++pos % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc( list[idx], size ))) list[idx] = ret;
    return ret;
}

/* default implementation of wine_dbgstr_wn */
static const char *default_dbgstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;

    if (!HIWORD(str))
    {
        if (!str) return "(null)";
        res = get_tmp_space( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = lstrlenW(str);
    if (n < 0) n = 0;
    else if (n > 200) n = 200;
    dst = res = get_tmp_space( n * 5 + 7 );
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = (char)c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
            }
        }
    }
    *dst++ = '"';
    if (*str)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst = 0;
    return res;
}

const char *wine_dbgstr_wn( const WCHAR *s, int n )
{
    return default_dbgstr_wn(s, n);
}

const char *wine_dbgstr_w( const WCHAR *s )
{
    return default_dbgstr_wn( s, -1 );
}


#define RELEASEMARSHALDATA WM_USER

struct host_object_data
{
    IStream *stream;
    IID iid;
    IUnknown *object;
    MSHLFLAGS marshal_flags;
    HANDLE marshal_event;
    IMessageFilter *filter;
};

static DWORD CALLBACK host_object_proc(LPVOID p)
{
    struct host_object_data *data = p;
    HRESULT hr;
    MSG msg;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (data->filter)
    {
        IMessageFilter * prev_filter = NULL;
        hr = CoRegisterMessageFilter(data->filter, &prev_filter);
        if (prev_filter) IMessageFilter_Release(prev_filter);
        ok_ole_success(hr, CoRegisterMessageFilter);
    }

    hr = CoMarshalInterface(data->stream, &data->iid, data->object, MSHCTX_INPROC, NULL, data->marshal_flags);
    ok_ole_success(hr, CoMarshalInterface);

    /* force the message queue to be created before signaling parent thread */
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(data->marshal_event);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == RELEASEMARSHALDATA)
        {
            trace("releasing marshal data\n");
            CoReleaseMarshalData(data->stream);
            SetEvent((HANDLE)msg.lParam);
        }
        else
            DispatchMessage(&msg);
    }

    HeapFree(GetProcessHeap(), 0, data);

    CoUninitialize();

    return hr;
}

static DWORD start_host_object2(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, IMessageFilter *filter, HANDLE *thread)
{
    DWORD tid = 0;
    HANDLE marshal_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    struct host_object_data *data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data));

    data->stream = stream;
    data->iid = *riid;
    data->object = object;
    data->marshal_flags = marshal_flags;
    data->marshal_event = marshal_event;
    data->filter = filter;

    *thread = CreateThread(NULL, 0, host_object_proc, data, 0, &tid);

    /* wait for marshaling to complete before returning */
    WaitForSingleObject(marshal_event, INFINITE);
    CloseHandle(marshal_event);

    return tid;
}

static DWORD start_host_object(IStream *stream, REFIID riid, IUnknown *object, MSHLFLAGS marshal_flags, HANDLE *thread)
{
    return start_host_object2(stream, riid, object, marshal_flags, NULL, thread);
}

#if 0 /* not used */
/* asks thread to release the marshal data because it has to be done by the
 * same thread that marshaled the interface in the first place. */
static void release_host_object(DWORD tid)
{
    HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
    PostThreadMessage(tid, RELEASEMARSHALDATA, 0, (LPARAM)event);
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
}
#endif

static void end_host_object(DWORD tid, HANDLE thread)
{
    BOOL ret = PostThreadMessage(tid, WM_QUIT, 0, 0);
    ok(ret, "PostThreadMessage failed with error %d\n", GetLastError());
    /* be careful of races - don't return until hosting thread has terminated */
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

static ItestDual TestDual, TestDualDisp;

static HRESULT WINAPI TestDual_QueryInterface(ItestDual *iface, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch)) {
        *ppvObject = &TestDualDisp;
        return S_OK;
    }else if(IsEqualGUID(riid, &IID_ItestDual)) {
        *ppvObject = &TestDual;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TestDual_AddRef(ItestDual *iface)
{
    return 2;
}

static ULONG WINAPI TestDual_Release(ItestDual *iface)
{
    return 1;
}

static HRESULT WINAPI TestDual_GetTypeInfoCount(ItestDual *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestDual_GetTypeInfo(ItestDual *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestDual_GetIDsOfNames(ItestDual *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI TestDual_Invoke(ItestDual *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ItestDualVtbl TestDualVtbl = {
    TestDual_QueryInterface,
    TestDual_AddRef,
    TestDual_Release,
    TestDual_GetTypeInfoCount,
    TestDual_GetTypeInfo,
    TestDual_GetIDsOfNames,
    TestDual_Invoke
};

static ItestDual TestDual = { &TestDualVtbl };
static ItestDual TestDualDisp = { &TestDualVtbl };

typedef struct Widget
{
    const IWidgetVtbl *lpVtbl;
    LONG refs;
    IUnknown *pDispatchUnknown;
} Widget;

static HRESULT WINAPI Widget_QueryInterface(
    IWidget *iface,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (IsEqualIID(riid, &IID_IWidget) || IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch))
    {
        IWidget_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI Widget_AddRef(
    IWidget *iface)
{
    Widget *This = (Widget *)iface;

    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI Widget_Release(
    IWidget *iface)
{
    Widget *This = (Widget *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        IUnknown_Release(This->pDispatchUnknown);
        memset(This, 0xcc, sizeof(*This));
        HeapFree(GetProcessHeap(), 0, This);
        trace("Widget destroyed!\n");
    }

    return refs;
}

static HRESULT WINAPI Widget_GetTypeInfoCount(
    IWidget *iface,
    /* [out] */ UINT __RPC_FAR *pctinfo)
{
    Widget *This = (Widget *)iface;
    IDispatch *pDispatch;
    HRESULT hr = IUnknown_QueryInterface(This->pDispatchUnknown, &IID_IDispatch, (void **)&pDispatch);
    if (SUCCEEDED(hr))
    {
        hr = IDispatch_GetTypeInfoCount(pDispatch, pctinfo);
        IDispatch_Release(pDispatch);
    }
    return hr;
}

static HRESULT WINAPI Widget_GetTypeInfo(
    IWidget __RPC_FAR * iface,
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    Widget *This = (Widget *)iface;
    IDispatch *pDispatch;
    HRESULT hr = IUnknown_QueryInterface(This->pDispatchUnknown, &IID_IDispatch, (void **)&pDispatch);
    if (SUCCEEDED(hr))
    {
        hr = IDispatch_GetTypeInfo(pDispatch, iTInfo, lcid, ppTInfo);
        IDispatch_Release(pDispatch);
    }
    return hr;
}

static HRESULT WINAPI Widget_GetIDsOfNames(
    IWidget __RPC_FAR * iface,
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    Widget *This = (Widget *)iface;
    IDispatch *pDispatch;
    HRESULT hr = IUnknown_QueryInterface(This->pDispatchUnknown, &IID_IDispatch, (void **)&pDispatch);
    if (SUCCEEDED(hr))
    {
        hr = IDispatch_GetIDsOfNames(pDispatch, riid, rgszNames, cNames, lcid, rgDispId);
        IDispatch_Release(pDispatch);
    }
    return hr;
}

static HRESULT WINAPI Widget_Invoke(
    IWidget __RPC_FAR * iface,
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
    /* [out] */ VARIANT __RPC_FAR *pVarResult,
    /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
    /* [out] */ UINT __RPC_FAR *puArgErr)
{
    Widget *This = (Widget *)iface;
    IDispatch *pDispatch;
    HRESULT hr = IUnknown_QueryInterface(This->pDispatchUnknown, &IID_IDispatch, (void **)&pDispatch);
    if (SUCCEEDED(hr))
    {
        hr = IDispatch_Invoke(pDispatch, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        IDispatch_Release(pDispatch);
    }
    return hr;
}

static HRESULT WINAPI Widget_put_Name(
    IWidget __RPC_FAR * iface,
    /* [in] */ BSTR name)
{
    trace("put_Name(%s)\n", wine_dbgstr_w(name));
    return S_OK;
}

static HRESULT WINAPI Widget_get_Name(
    IWidget __RPC_FAR * iface,
    /* [out] */ BSTR __RPC_FAR *name)
{
    static const WCHAR szCat[] = { 'C','a','t',0 };
    trace("get_Name()\n");
    *name = SysAllocString(szCat);
    return S_OK;
}

static HRESULT WINAPI Widget_DoSomething(
    IWidget __RPC_FAR * iface,
    /* [in] */ double number,
    /* [out] */ BSTR *str1,
    /* [defaultvalue][in] */ BSTR str2,
    /* [optional][in] */ VARIANT __RPC_FAR *opt)
{
    static const WCHAR szString[] = { 'S','t','r','i','n','g',0 };
    trace("DoSomething()\n");

    ok(number == 3.141, "number(%f) != 3.141\n", number);
    ok(*str2 == '\0', "str2(%s) != \"\"\n", wine_dbgstr_w(str2));
    ok(V_VT(opt) == VT_ERROR, "V_VT(opt) should be VT_ERROR instead of 0x%x\n", V_VT(opt));
    ok(V_ERROR(opt) == DISP_E_PARAMNOTFOUND, "V_ERROR(opt) should be DISP_E_PARAMNOTFOUND instead of 0x%08x\n", V_ERROR(opt));
    *str1 = SysAllocString(szString);

    return S_FALSE;
}

static HRESULT WINAPI Widget_get_State(
    IWidget __RPC_FAR * iface,
    /* [retval][out] */ STATE __RPC_FAR *state)
{
    trace("get_State() = STATE_WIDGETIFIED\n");
    *state = STATE_WIDGETIFIED;
    return S_OK;
}

static HRESULT WINAPI Widget_put_State(
    IWidget __RPC_FAR * iface,
    /* [in] */ STATE state)
{
    trace("put_State(%d)\n", state);
    return S_OK;
}

static HRESULT WINAPI Widget_Map(
    IWidget * iface,
    BSTR bstrId,
    BSTR *sValue)
{
    trace("Map(%s, %p)\n", wine_dbgstr_w(bstrId), sValue);
    *sValue = SysAllocString(bstrId);
    return S_OK;
}

static HRESULT WINAPI Widget_SetOleColor(
    IWidget * iface,
    OLE_COLOR val)
{
    trace("SetOleColor(0x%x)\n", val);
    return S_OK;
}

static HRESULT WINAPI Widget_GetOleColor(
    IWidget * iface,
    OLE_COLOR *pVal)
{
    trace("GetOleColor() = 0x8000000f\n");
    *pVal = 0x8000000f;
    return S_FALSE;
}

static HRESULT WINAPI Widget_Clone(
    IWidget *iface,
    IWidget **ppVal)
{
    trace("Clone()\n");
    return Widget_QueryInterface(iface, &IID_IWidget, (void **)ppVal);
}

static HRESULT WINAPI Widget_CloneDispatch(
    IWidget *iface,
    IDispatch **ppVal)
{
    trace("CloneDispatch()\n");
    return Widget_QueryInterface(iface, &IID_IWidget, (void **)ppVal);
}

static HRESULT WINAPI Widget_CloneCoclass(
    IWidget *iface,
    ApplicationObject2 **ppVal)
{
    trace("CloneCoclass()\n");
    return S_OK;
}

static HRESULT WINAPI Widget_Value(
    IWidget __RPC_FAR * iface,
    VARIANT *value,
    VARIANT *retval)
{
    trace("Value(%p, %p)\n", value, retval);
    ok(V_VT(value) == VT_I2, "V_VT(value) was %d instead of VT_I2\n", V_VT(value));
    ok(V_I2(value) == 1, "V_I2(value) was %d instead of 1\n", V_I2(value));
    V_VT(retval) = VT_I2;
    V_I2(retval) = 1234;
    return S_OK;
}

static HRESULT WINAPI Widget_Array(
    IWidget * iface,
    SAFEARRAY * values)
{
    trace("Array(%p)\n", values);
    return S_OK;
}

static HRESULT WINAPI Widget_VariantArrayPtr(
    IWidget * iface,
    SAFEARRAY ** values)
{
    trace("VariantArrayPtr(%p)\n", values);
    return S_OK;
}

static HRESULT WINAPI Widget_Variant(
    IWidget __RPC_FAR * iface,
    VARIANT var)
{
    trace("Variant()\n");
    ok(V_VT(&var) == VT_CY, "V_VT(&var) was %d\n", V_VT(&var));
    ok(S(V_CY(&var)).Hi == 0xdababe, "V_CY(&var).Hi was 0x%x\n", S(V_CY(&var)).Hi);
    ok(S(V_CY(&var)).Lo == 0xdeadbeef, "V_CY(&var).Lo was 0x%x\n", S(V_CY(&var)).Lo);
    return S_OK;
}

static HRESULT WINAPI Widget_VarArg(
    IWidget * iface,
    int numexpect,
    SAFEARRAY * values)
{
    LONG lbound, ubound, i;
    VARIANT * data;
    HRESULT hr;

    trace("VarArg(%p)\n", values);

    hr = SafeArrayGetLBound(values, 1, &lbound);
    ok(hr == S_OK, "SafeArrayGetLBound failed with %x\n", hr);
    ok(lbound == 0, "SafeArrayGetLBound returned %d\n", lbound);

    hr = SafeArrayGetUBound(values, 1, &ubound);
    ok(hr == S_OK, "SafeArrayGetUBound failed with %x\n", hr);
    ok(ubound == numexpect-1, "SafeArrayGetUBound returned %d, but expected %d\n", ubound, numexpect-1);

    hr = SafeArrayAccessData(values, (LPVOID)&data);
    ok(hr == S_OK, "SafeArrayAccessData failed with %x\n", hr);

    for (i=0; i<=ubound-lbound; i++)
    {
        ok(V_VT(&data[i]) == VT_I4, "V_VT(&data[%d]) was %d\n", i, V_VT(&data[i]));
        ok(V_I4(&data[i]) == i, "V_I4(&data[%d]) was %d\n", i, V_I4(&data[i]));
    }

    hr = SafeArrayUnaccessData(values);
    ok(hr == S_OK, "SafeArrayUnaccessData failed with %x\n", hr);

    return S_OK;
}

static HRESULT WINAPI Widget_StructArgs(
    IWidget * iface,
    MYSTRUCT byval,
    MYSTRUCT *byptr,
    MYSTRUCT arr[5])
{
    ok(memcmp(&byval, &MYSTRUCT_BYVAL, sizeof(MYSTRUCT))==0, "Struct parameter passed by value corrupted\n");
    ok(memcmp(byptr,  &MYSTRUCT_BYPTR, sizeof(MYSTRUCT))==0, "Struct parameter passed by pointer corrupted\n");
    ok(memcmp(arr,    MYSTRUCT_ARRAY,  sizeof(MYSTRUCT_ARRAY))==0, "Array of structs corrupted\n");
    return S_OK;
}


static HRESULT WINAPI Widget_Error(
    IWidget __RPC_FAR * iface)
{
    trace("Error()\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Widget_CloneInterface(
    IWidget __RPC_FAR * iface,
    ISomethingFromDispatch **ppVal)
{
    trace("CloneInterface()\n");
    *ppVal = 0;
    return S_OK;
}

static const struct IWidgetVtbl Widget_VTable =
{
    Widget_QueryInterface,
    Widget_AddRef,
    Widget_Release,
    Widget_GetTypeInfoCount,
    Widget_GetTypeInfo,
    Widget_GetIDsOfNames,
    Widget_Invoke,
    Widget_put_Name,
    Widget_get_Name,
    Widget_DoSomething,
    Widget_get_State,
    Widget_put_State,
    Widget_Map,
    Widget_SetOleColor,
    Widget_GetOleColor,
    Widget_Clone,
    Widget_CloneDispatch,
    Widget_CloneCoclass,
    Widget_Value,
    Widget_Array,
    Widget_VariantArrayPtr,
    Widget_Variant,
    Widget_VarArg,
    Widget_StructArgs,
    Widget_Error,
    Widget_CloneInterface
};

static HRESULT WINAPI StaticWidget_QueryInterface(IStaticWidget *iface, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(riid, &IID_IStaticWidget) || IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDispatch))
    {
        IStaticWidget_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI StaticWidget_AddRef(IStaticWidget *iface)
{
    return 2;
}

static ULONG WINAPI StaticWidget_Release(IStaticWidget *iface)
{
    return 1;
}

static HRESULT WINAPI StaticWidget_GetTypeInfoCount(IStaticWidget *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI StaticWidget_GetTypeInfo(IStaticWidget *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI StaticWidget_GetIDsOfNames(IStaticWidget *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI StaticWidget_Invoke(IStaticWidget *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
         UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI StaticWidget_TestDual(IStaticWidget *iface, ItestDual *p)
{
    trace("TestDual()\n");
    todo_wine
    ok(p == &TestDual, "wrong ItestDual\n");
    return S_OK;
}

static const IStaticWidgetVtbl StaticWidgetVtbl = {
    StaticWidget_QueryInterface,
    StaticWidget_AddRef,
    StaticWidget_Release,
    StaticWidget_GetTypeInfoCount,
    StaticWidget_GetTypeInfo,
    StaticWidget_GetIDsOfNames,
    StaticWidget_Invoke,
    StaticWidget_TestDual
};

static IStaticWidget StaticWidget = { &StaticWidgetVtbl };

typedef struct KindaEnum
{
    const IKindaEnumWidgetVtbl *lpVtbl;
    LONG refs;
} KindaEnum;

static HRESULT register_current_module_typelib(void)
{
    WCHAR path[MAX_PATH];
    CHAR pathA[MAX_PATH];
    HRESULT hr;
    ITypeLib *typelib;

    GetModuleFileNameA(NULL, pathA, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);

    hr = LoadTypeLib(path, &typelib);
    if (SUCCEEDED(hr))
    {
        hr = RegisterTypeLib(typelib, path, NULL);
        ITypeLib_Release(typelib);
    }
    return hr;
}

static ITypeInfo *get_type_info(REFIID riid)
{
    ITypeInfo *pTypeInfo;
    ITypeLib *pTypeLib;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_TestTypelib, 1, 0, LOCALE_NEUTRAL, &pTypeLib);
    ok_ole_success(hr, LoadRegTypeLib);
    if (FAILED(hr))
        return NULL;

    hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, riid, &pTypeInfo);
    ITypeLib_Release(pTypeLib);
    ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid);
    if (FAILED(hr))
        return NULL;

    return pTypeInfo;
}

static IWidget *Widget_Create(void)
{
    Widget *This;
    ITypeInfo *pTypeInfo;
    HRESULT hr = E_FAIL;

    pTypeInfo = get_type_info(&IID_IWidget);
    if(!pTypeInfo)
        return NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    This->lpVtbl = &Widget_VTable;
    This->refs = 1;
    This->pDispatchUnknown = NULL;

    hr = CreateStdDispatch((IUnknown *)&This->lpVtbl, This, pTypeInfo, &This->pDispatchUnknown);
    ok_ole_success(hr, CreateStdDispatch);
    ITypeInfo_Release(pTypeInfo);

    if (SUCCEEDED(hr))
        return (IWidget *)&This->lpVtbl;
    else
    {
        HeapFree(GetProcessHeap(), 0, This);
        return NULL;
    }
}

static HRESULT WINAPI KindaEnum_QueryInterface(
    IKindaEnumWidget *iface,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (IsEqualIID(riid, &IID_IKindaEnumWidget) || IsEqualIID(riid, &IID_IUnknown))
    {
        IKindaEnumWidget_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI KindaEnum_AddRef(
    IKindaEnumWidget *iface)
{
    KindaEnum *This = (KindaEnum *)iface;

    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI KindaEnum_Release(
    IKindaEnumWidget *iface)
{
    KindaEnum *This = (KindaEnum *)iface;
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        memset(This, 0xcc, sizeof(*This));
        HeapFree(GetProcessHeap(), 0, This);
        trace("KindaEnumWidget destroyed!\n");
    }

    return refs;
}

static HRESULT WINAPI KindaEnum_Next(
    IKindaEnumWidget *iface,
    /* [out] */ IWidget __RPC_FAR *__RPC_FAR *widget)
{
    *widget = Widget_Create();
    if (*widget)
        return S_OK;
    else
        return E_OUTOFMEMORY;
}

static HRESULT WINAPI KindaEnum_Count(
    IKindaEnumWidget *iface,
    /* [out] */ ULONG __RPC_FAR *count)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI KindaEnum_Reset(
    IKindaEnumWidget *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI KindaEnum_Clone(
    IKindaEnumWidget *iface,
    /* [out] */ IKindaEnumWidget __RPC_FAR *__RPC_FAR *ppenum)
{
    return E_NOTIMPL;
}

static const IKindaEnumWidgetVtbl KindaEnumWidget_VTable =
{
    KindaEnum_QueryInterface,
    KindaEnum_AddRef,
    KindaEnum_Release,
    KindaEnum_Next,
    KindaEnum_Count,
    KindaEnum_Reset,
    KindaEnum_Clone
};

static IKindaEnumWidget *KindaEnumWidget_Create(void)
{
    KindaEnum *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return NULL;
    This->lpVtbl = &KindaEnumWidget_VTable;
    This->refs = 1;
    return (IKindaEnumWidget *)This;
}

static HRESULT WINAPI NonOleAutomation_QueryInterface(INonOleAutomation *iface, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_INonOleAutomation))
    {
        *(INonOleAutomation **)ppv = iface;
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI NonOleAutomation_AddRef(INonOleAutomation *iface)
{
    return 2;
}

static ULONG WINAPI NonOleAutomation_Release(INonOleAutomation *iface)
{
    return 1;
}

static BSTR WINAPI NonOleAutomation_BstrRet(INonOleAutomation *iface)
{
    static const WCHAR wszTestString[] = {'T','h','i','s',' ','i','s',' ','a',' ','t','e','s','t',' ','s','t','r','i','n','g',0};
    return SysAllocString(wszTestString);
}

static INonOleAutomationVtbl NonOleAutomation_VTable =
{
    NonOleAutomation_QueryInterface,
    NonOleAutomation_AddRef,
    NonOleAutomation_Release,
    NonOleAutomation_BstrRet,
};

static INonOleAutomation NonOleAutomation = { &NonOleAutomation_VTable };

static ITypeInfo *NonOleAutomation_GetTypeInfo(void)
{
    ITypeLib *pTypeLib;
    HRESULT hr = LoadRegTypeLib(&LIBID_TestTypelib, 1, 0, LOCALE_NEUTRAL, &pTypeLib);
    ok_ole_success(hr, LoadRegTypeLib);
    if (SUCCEEDED(hr))
    {
        ITypeInfo *pTypeInfo;
        hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, &IID_INonOleAutomation, &pTypeInfo);
        ok_ole_success(hr, ITypeLib_GetTypeInfoOfGuid);
        return pTypeInfo;
    }
    return NULL;
}

static void test_typelibmarshal(void)
{
    static const WCHAR szCat[] = { 'C','a','t',0 };
    static const WCHAR szTestTest[] = { 'T','e','s','t','T','e','s','t',0 };
    static WCHAR szSuperman[] = { 'S','u','p','e','r','m','a','n',0 };
    HRESULT hr;
    IKindaEnumWidget *pKEW = KindaEnumWidget_Create();
    IWidget *pWidget;
    IStream *pStream;
    IDispatch *pDispatch;
    static const LARGE_INTEGER ullZero;
    EXCEPINFO excepinfo;
    VARIANT varresult;
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams;
    VARIANTARG vararg[4];
    STATE the_state;
    HANDLE thread;
    DWORD tid;
    BSTR bstr;
    ITypeInfo *pTypeInfo;
    MYSTRUCT mystruct;
    MYSTRUCT mystructArray[5];

    ok(pKEW != NULL, "Widget creation failed\n");

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    ok_ole_success(hr, CreateStreamOnHGlobal);
    tid = start_host_object(pStream, &IID_IKindaEnumWidget, (IUnknown *)pKEW, MSHLFLAGS_NORMAL, &thread);
    IKindaEnumWidget_Release(pKEW);

    IStream_Seek(pStream, ullZero, STREAM_SEEK_SET, NULL);
    hr = CoUnmarshalInterface(pStream, &IID_IKindaEnumWidget, (void **)&pKEW);
    ok_ole_success(hr, CoUnmarshalInterface);
    IStream_Release(pStream);

    hr = IKindaEnumWidget_Next(pKEW, &pWidget);
    ok_ole_success(hr, IKindaEnumWidget_Next);

    IKindaEnumWidget_Release(pKEW);

    hr = IWidget_QueryInterface(pWidget, &IID_IDispatch, (void **)&pDispatch);

    /* call put_Name */
    VariantInit(&vararg[0]);
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidNamed;
    dispparams.cArgs = 1;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_NAME, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(excepinfo.wCode == 0x0 && excepinfo.scode == S_OK,
        "EXCEPINFO differs from expected: wCode = 0x%x, scode = 0x%08x\n",
        excepinfo.wCode, excepinfo.scode);
    VariantClear(&varresult);

    /* call put_Name (direct) */
    bstr = SysAllocString(szSuperman);
    hr = IWidget_put_Name(pWidget, bstr);
    ok_ole_success(hr, IWidget_put_Name);
    SysFreeString(bstr);

    /* call get_Name */
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 0;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_NAME, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(excepinfo.wCode == 0x0 && excepinfo.scode == S_OK,
        "EXCEPINFO differs from expected: wCode = 0x%x, scode = 0x%08x\n",
        excepinfo.wCode, excepinfo.scode);
    trace("Name = %s\n", wine_dbgstr_w(V_BSTR(&varresult)));
    VariantClear(&varresult);

    /* call get_Name (direct) */
    bstr = NULL;
    hr = IWidget_get_Name(pWidget, &bstr);
    ok_ole_success(hr, IWidget_get_Name);
    ok(!lstrcmpW(bstr, szCat), "IWidget_get_Name should have returned string \"Cat\" instead of %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    /* call DoSomething */
    VariantInit(&vararg[0]);
    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_R8;
    V_R8(&vararg[1]) = 3.141;
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 2;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_DOSOMETHING, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(V_VT(&varresult) == VT_EMPTY, "varresult should be VT_EMPTY\n");
    VariantClear(&varresult);

    /* call get_State */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_STATE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok((V_VT(&varresult) == VT_I4) && (V_I4(&varresult) == STATE_WIDGETIFIED), "Return val mismatch\n");

    /* call get_State (direct) */
    hr = IWidget_get_State(pWidget, &the_state);
    ok_ole_success(hr, IWidget_get_state);
    ok(the_state == STATE_WIDGETIFIED, "should have returned WIDGET_WIDGETIFIED instead of %d\n", the_state);

    /* call put_State */
    the_state = STATE_WIDGETIFIED;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BYREF|VT_I4;
    V_I4REF(&vararg[0]) = (int *)&the_state;
    dispparams.cNamedArgs = 1;
    dispparams.cArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidNamed;
    dispparams.rgvarg = vararg;
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_STATE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);

    /* call Map */
    bstr = SysAllocString(szTestTest);
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BYREF|VT_BSTR;
    V_BSTRREF(&vararg[0]) = &bstr;
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 1;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_MAP, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(V_VT(&varresult) == VT_BSTR, "Return value should be of type BSTR instead of %d\n", V_VT(&varresult));
    ok(!lstrcmpW(V_BSTR(&varresult), szTestTest), "Return value should have been \"TestTest\" instead of %s\n", wine_dbgstr_w(V_BSTR(&varresult)));
    VariantClear(&varresult);

    /* call SetOleColor with large negative VT_I4 param */
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = 0x80000005;
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 1;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_SETOLECOLOR, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, NULL, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);

    /* call GetOleColor */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_GETOLECOLOR, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    VariantClear(&varresult);

    /* call StructArgs (direct) */
    mystruct = MYSTRUCT_BYPTR;
    memcpy(mystructArray, MYSTRUCT_ARRAY, sizeof(mystructArray));
    hr = IWidget_StructArgs(pWidget, MYSTRUCT_BYVAL, &mystruct, mystructArray);
    todo_wine {
    ok_ole_success(hr, IWidget_StructArgs);
    }

    /* call Clone */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_CLONE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(V_VT(&varresult) == VT_DISPATCH, "vt %x\n", V_VT(&varresult));
    VariantClear(&varresult);

    /* call CloneInterface */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_CLONEINTERFACE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(V_VT(&varresult) == VT_DISPATCH, "vt %x\n", V_VT(&varresult));
    VariantClear(&varresult);

    /* call CloneDispatch with automatic value getting */
    V_VT(&vararg[0]) = VT_I2;
    V_I2(&vararg[0]) = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_CLONEDISPATCH, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);

    ok(excepinfo.wCode == 0x0 && excepinfo.scode == S_OK,
        "EXCEPINFO differs from expected: wCode = 0x%x, scode = 0x%08x\n",
        excepinfo.wCode, excepinfo.scode);

    ok(V_VT(&varresult) == VT_I2, "V_VT(&varresult) was %d instead of VT_I2\n", V_VT(&varresult));
    ok(V_I2(&varresult) == 1234, "V_I2(&varresult) was %d instead of 1234\n", V_I2(&varresult));
    VariantClear(&varresult);

    /* call CloneCoclass */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_CLONECOCLASS, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);

    ok(excepinfo.wCode == 0x0 && excepinfo.scode == S_OK,
       "EXCEPINFO differs from expected: wCode = 0x%x, scode = 0x%08x\n",
       excepinfo.wCode, excepinfo.scode);

    ok(V_VT(&varresult) == VT_DISPATCH, "V_VT(&varresult) was %d instead of VT_DISPATCH\n", V_VT(&varresult));
    ok(!V_DISPATCH(&varresult), "V_DISPATCH(&varresult) should be NULL instead of %p\n", V_DISPATCH(&varresult));
    VariantClear(&varresult);

    /* call Value with a VT_VARIANT|VT_BYREF type */
    V_VT(&vararg[0]) = VT_VARIANT|VT_BYREF;
    V_VARIANTREF(&vararg[0]) = &vararg[1];
    V_VT(&vararg[1]) = VT_I2;
    V_I2(&vararg[1]) = 1;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_VALUE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);

    ok(excepinfo.wCode == 0x0 && excepinfo.scode == S_OK,
        "EXCEPINFO differs from expected: wCode = 0x%x, scode = 0x%08x\n",
        excepinfo.wCode, excepinfo.scode);

    ok(V_VT(&varresult) == VT_I2, "V_VT(&varresult) was %d instead of VT_I2\n", V_VT(&varresult));
    ok(V_I2(&varresult) == 1234, "V_I2(&varresult) was %d instead of 1234\n", V_I2(&varresult));
    VariantClear(&varresult);

    /* call Variant - exercises variant copying in ITypeInfo::Invoke and
     * handling of void return types */
    /* use a big type to ensure that the variant was properly copied into the
     * destination function's args */
    V_VT(&vararg[0]) = VT_CY;
    S(V_CY(&vararg[0])).Hi = 0xdababe;
    S(V_CY(&vararg[0])).Lo = 0xdeadbeef;
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 1;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_VARIANT, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    VariantClear(&varresult);

    /* call VarArg */
    VariantInit(&vararg[3]);
    V_VT(&vararg[3]) = VT_I4;
    V_I4(&vararg[3]) = 3;
    VariantInit(&vararg[2]);
    V_VT(&vararg[2]) = VT_I4;
    V_I4(&vararg[2]) = 0;
    VariantInit(&vararg[1]);
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = 1;
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_I4;
    V_I4(&vararg[0]) = 2;
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 4;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_VARARG, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok_ole_success(hr, ITypeInfo_Invoke);

    /* call VarArg, even one (non-optional, non-safearray) named argument is not allowed */
    dispidNamed = 0;
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidNamed;
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_VARARG, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
    ok(hr == DISP_E_NONAMEDARGS, "IDispatch_Invoke should have returned DISP_E_NONAMEDARGS instead of 0x%08x\n", hr);
    dispidNamed = DISPID_PROPERTYPUT;

    /* call Error */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_ERROR, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, NULL, &excepinfo, NULL);
    ok(hr == DISP_E_EXCEPTION, "IDispatch_Invoke should have returned DISP_E_EXCEPTION instead of 0x%08x\n", hr);
    ok(excepinfo.wCode == 0x0 && excepinfo.scode == E_NOTIMPL,
        "EXCEPINFO differs from expected: wCode = 0x%x, scode = 0x%08x\n",
        excepinfo.wCode, excepinfo.scode);
    VariantClear(&varresult);

    /* call BstrRet */
    pTypeInfo = NonOleAutomation_GetTypeInfo();
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = ITypeInfo_Invoke(pTypeInfo, &NonOleAutomation, DISPID_NOA_BSTRRET, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, ITypeInfo_Invoke);
    ok(V_VT(&varresult) == VT_BSTR, "V_VT(&varresult) should be VT_BSTR instead of %d\n", V_VT(&varresult));
    ok(V_BSTR(&varresult) != NULL, "V_BSTR(&varresult) should not be NULL\n");

    VariantClear(&varresult);
    ITypeInfo_Release(pTypeInfo);

    /* tests call put_Name without named arg */
    VariantInit(&vararg[0]);
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 1;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_NAME, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_PARAMNOTFOUND, "IDispatch_Invoke should have returned DISP_E_PARAMNOTFOUND instead of 0x%08x\n", hr);
    VariantClear(&varresult);

    /* tests param type that cannot be coerced */
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_UNKNOWN;
    V_UNKNOWN(&vararg[0]) = NULL;
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidNamed;
    dispparams.cArgs = 1;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
#if 0 /* NULL unknown not currently marshaled correctly */
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_NAME, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_TYPEMISMATCH, "IDispatch_Invoke should have returned DISP_E_TYPEMISMATCH instead of 0x%08x\n", hr);
#endif
    VariantClear(&varresult);

    /* tests bad param type */
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_CLSID;
    V_BYREF(&vararg[0]) = NULL;
    dispparams.cNamedArgs = 1;
    dispparams.rgdispidNamedArgs = &dispidNamed;
    dispparams.cArgs = 1;
    dispparams.rgvarg = vararg;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_NAME, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYPUT, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_BADVARTYPE, "IDispatch_Invoke should have returned DISP_E_BADVARTYPE instead of 0x%08x\n", hr);
    VariantClear(&varresult);

    /* tests too small param count */
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.cArgs = 0;
    dispparams.rgvarg = NULL;
    VariantInit(&varresult);
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_DOSOMETHING, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_METHOD, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_BADPARAMCOUNT, "IDispatch_Invoke should have returned DISP_E_BADPARAMCOUNT instead of 0x%08x\n", hr);
    VariantClear(&varresult);

    /* tests propget function with large param count */
    VariantInit(&vararg[0]);
    V_VT(&vararg[0]) = VT_BSTR;
    V_BSTR(&vararg[0]) = NULL;
    V_VT(&vararg[1]) = VT_I4;
    V_I4(&vararg[1]) = 1;
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 2;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    hr = IDispatch_Invoke(pDispatch, DISPID_TM_STATE, &IID_NULL, LOCALE_NEUTRAL, DISPATCH_PROPERTYGET, &dispparams, &varresult, &excepinfo, NULL);
    ok(hr == DISP_E_NOTACOLLECTION, "IDispatch_Invoke should have returned DISP_E_NOTACOLLECTION instead of 0x%08x\n", hr);

    IDispatch_Release(pDispatch);
    IWidget_Release(pWidget);

    trace("calling end_host_object\n");
    end_host_object(tid, thread);
}

static void test_DispCallFunc(void)
{
    static const WCHAR szEmpty[] = { 0 };
    VARTYPE rgvt[] = { VT_R8, VT_BSTR, VT_BSTR, VT_VARIANT|VT_BYREF };
    VARIANTARG vararg[4];
    VARIANTARG varref;
    VARIANTARG *rgpvarg[4] = { &vararg[0], &vararg[1], &vararg[2], &vararg[3] };
    VARIANTARG varresult;
    HRESULT hr;
    IWidget *pWidget = Widget_Create();
    V_VT(&vararg[0]) = VT_R8;
    V_R8(&vararg[0]) = 3.141;
    V_VT(&vararg[1]) = VT_BSTR;
    V_BSTR(&vararg[1]) = SysAllocString(szEmpty);
    V_VT(&vararg[2]) = VT_BSTR;
    V_BSTR(&vararg[2]) = SysAllocString(szEmpty);
    V_VT(&vararg[3]) = VT_VARIANT|VT_BYREF;
    V_VARIANTREF(&vararg[3]) = &varref;
    V_VT(&varref) = VT_ERROR;
    V_ERROR(&varref) = DISP_E_PARAMNOTFOUND;
    VariantInit(&varresult);
    hr = DispCallFunc(pWidget, 36, CC_STDCALL, VT_UI4, 4, rgvt, rgpvarg, &varresult);
    ok_ole_success(hr, DispCallFunc);
    VariantClear(&varresult);
    VariantClear(&vararg[1]);
    VariantClear(&vararg[2]);
    IWidget_Release(pWidget);
}

static void test_StaticWidget(void)
{
    ITypeInfo *type_info;
    DISPPARAMS dispparams;
    VARIANTARG vararg[4];
    EXCEPINFO excepinfo;
    VARIANT varresult;
    HRESULT hr;

    type_info = get_type_info(&IID_IStaticWidget);

    /* call TestDual */
    dispparams.cNamedArgs = 0;
    dispparams.cArgs = 1;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = vararg;
    V_VT(vararg) = VT_DISPATCH;
    V_DISPATCH(vararg) = (IDispatch*)&TestDualDisp;
    VariantInit(&varresult);
    hr = ITypeInfo_Invoke(type_info, &StaticWidget, DISPID_TM_TESTDUAL, DISPATCH_METHOD,
            &dispparams, &varresult, &excepinfo, NULL);
    ok_ole_success(hr, IDispatch_Invoke);
    ok(V_VT(&varresult) == VT_EMPTY, "vt %x\n", V_VT(&varresult));
    VariantClear(&varresult);

    ITypeInfo_Release(type_info);
}

START_TEST(tmarshal)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = register_current_module_typelib();
    ok_ole_success(hr, register_current_module_typelib);

    test_typelibmarshal();
    test_DispCallFunc();
    test_StaticWidget();

    hr = UnRegisterTypeLib(&LIBID_TestTypelib, 1, 0, LOCALE_NEUTRAL, 1);
    ok_ole_success(hr, UnRegisterTypeLib);

    CoUninitialize();
}
