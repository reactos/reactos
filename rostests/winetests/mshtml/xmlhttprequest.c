/*
 * Copyright 2015 Zhenbo Li
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

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "mshtml.h"

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
    trace(#func "\n"); \
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

static IHTMLXMLHttpRequest *xhr = NULL;
static BSTR content_type = NULL;
static int loading_cnt = 0;
static int readystatechange_cnt = 0;

DEFINE_EXPECT(xmlhttprequest_onreadystatechange_opened);
DEFINE_EXPECT(xmlhttprequest_onreadystatechange_headers_received);
DEFINE_EXPECT(xmlhttprequest_onreadystatechange_loading);
DEFINE_EXPECT(xmlhttprequest_onreadystatechange_done);

#define test_disp(u,id) _test_disp(__LINE__,u,id)
static void _test_disp(unsigned line, IUnknown *unk, const IID *diid)
{
    IDispatchEx *dispex;
    ITypeInfo *typeinfo;
    UINT ticnt;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IDispatch: %08x\n", hres);
    if(FAILED(hres))
        return;

    ticnt = 0xdeadbeef;
    hres = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfoCount failed: %08x\n", hres);
    ok_(__FILE__,line) (ticnt == 1, "ticnt=%u\n", ticnt);

    hres = IDispatchEx_GetTypeInfo(dispex, 0, 0, &typeinfo);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfo failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        TYPEATTR *type_attr;

        hres = ITypeInfo_GetTypeAttr(typeinfo, &type_attr);
        ok_(__FILE__,line) (hres == S_OK, "GetTypeAttr failed: %08x\n", hres);
        ok_(__FILE__,line) (IsEqualGUID(&type_attr->guid, diid), "unexpected guid %s\n",
                            wine_dbgstr_guid(&type_attr->guid));

        ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
        ITypeInfo_Release(typeinfo);
    }

    IDispatchEx_Release(dispex);
}

#define test_event_args(a,b,c,d,e,f,g) _test_event_args(__LINE__,a,b,c,d,e,f,g)
static void _test_event_args(unsigned line, const IID *dispiid, DISPID id, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok_(__FILE__,line) (id == DISPID_VALUE, "id = %d\n", id);
    ok_(__FILE__,line) (wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok_(__FILE__,line) (pdp != NULL, "pdp == NULL\n");
    ok_(__FILE__,line) (pdp->cArgs == 1, "pdp->cArgs = %d\n", pdp->cArgs);
    ok_(__FILE__,line) (pdp->cNamedArgs == 1, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok_(__FILE__,line) (pdp->rgdispidNamedArgs[0] == DISPID_THIS, "pdp->rgdispidNamedArgs[0] = %d\n",
                        pdp->rgdispidNamedArgs[0]);
    ok_(__FILE__,line) (V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
    ok_(__FILE__,line) (pvarRes != NULL, "pvarRes == NULL\n");
    ok_(__FILE__,line) (pei != NULL, "pei == NULL");
    ok_(__FILE__,line) (!pspCaller, "pspCaller != NULL\n");

    if(dispiid)
        _test_disp(line, (IUnknown*)V_DISPATCH(pdp->rgvarg), dispiid);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else {
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }

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

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
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

static HRESULT WINAPI xmlhttprequest_onreadystatechange(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    LONG val;
    HRESULT hres;

    test_event_args(&DIID_DispHTMLXMLHttpRequest, id, wFlags, pdp, pvarRes, pei, pspCaller);

    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    readystatechange_cnt++;

    switch(val) {
        case 1:
            CHECK_EXPECT(xmlhttprequest_onreadystatechange_opened);
            break;
        case 2:
            CHECK_EXPECT(xmlhttprequest_onreadystatechange_headers_received);
            break;
        case 3:
            loading_cnt++;
            CHECK_EXPECT2(xmlhttprequest_onreadystatechange_loading);
            break;
        case 4:
            CHECK_EXPECT(xmlhttprequest_onreadystatechange_done);
            break;
        default:
            ok(0, "unexpected readyState: %d\n", val);
    }
    return S_OK;
}

static IDispatchExVtbl xmlhttprequest_onreadystatechangeFuncVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    xmlhttprequest_onreadystatechange,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};
static IDispatchEx xmlhttprequest_onreadystatechange_obj = { &xmlhttprequest_onreadystatechangeFuncVtbl };

static BOOL doc_complete;
static IHTMLDocument2 *notif_doc;

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
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

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);

        if(!strcmp_wa(state, "complete"))
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

static void do_advise(IUnknown *unk, REFIID riid, IUnknown *unk_advise)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    hres = IConnectionPoint_Advise(cp, unk_advise, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);
}

static void pump_msgs(BOOL *b)
{
    MSG msg;

    if(b) {
        while(!*b && GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }else {
        while(PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}


struct HEADER_TYPE {
    const char *key;
    const char *value;
};

static void create_xmlhttprequest(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    IHTMLWindow5 *window5;
    VARIANT var;
    IHTMLXMLHttpRequestFactory *factory;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);
    ok(window != NULL, "window == NULL\n");

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLWindow5, (void**)&window5);
    IHTMLWindow2_Release(window);
    if(FAILED(hres)) {
        win_skip("IHTMLWindow5 not supported\n");
        return;
    }

    VariantInit(&var);
    hres = IHTMLWindow5_get_XMLHttpRequest(window5, &var);
    IHTMLWindow5_Release(window5);
    ok(hres == S_OK, "get_XMLHttpRequest failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(&var) is %08x, expected VT_DISPATCH\n", V_VT(&var));

    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IHTMLXMLHttpRequestFactory, (void**)&factory);
    VariantClear(&var);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLXMLHttpRequestFactory) failed: %08x\n", hres);
    ok(factory != NULL, "factory == NULL\n");

    hres = IHTMLXMLHttpRequestFactory_create(factory, &xhr);
    IHTMLXMLHttpRequestFactory_Release(factory);
    ok(hres == S_OK, "create failed: %08x\n", hres);
    ok(xhr != NULL, "xhr == NULL\n");
}

static void test_header(const struct HEADER_TYPE expect[], int num)
{
    int i;
    BSTR key, text, all_header;
    HRESULT hres;
    char all[4096], buf[512];

    all_header = NULL;
    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, &all_header);
    ok(hres == S_OK, "getAllResponseHeader failed: %08x\n", hres);
    ok(all_header != NULL, "all_header == NULL\n");

    WideCharToMultiByte(CP_UTF8, 0, all_header, -1, all, sizeof(all), NULL, NULL);
    SysFreeString(all_header);

    for(i = 0; i < num; ++i) {
        text = NULL;
        key = a2bstr(expect[i].key);
        hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, key, &text);
        ok(hres == S_OK, "getResponseHeader failed, got %08x\n", hres);
        ok(text != NULL, "text == NULL\n");
        ok(!strcmp_wa(text, expect[i].value),
            "Expect %s: %s, got %s\n", expect[i].key, expect[i].value, wine_dbgstr_w(text));
        SysFreeString(key);
        SysFreeString(text);

        strcpy(buf, expect[i].key);
        strcat(buf, ": ");
        strcat(buf, expect[i].value);
        ok(strstr(all, buf) != NULL, "AllResponseHeaders(%s) don't have expected substr(%s)\n", all, buf);
    }
}

static void test_sync_xhr(IHTMLDocument2 *doc, const char *xml_url, const char *expect_text)
{
    VARIANT vbool, vempty, var;
    BSTR method, url;
    BSTR text;
    LONG val;
    HRESULT hres;
    static const struct HEADER_TYPE expect_headers[] = {
        {"Server", "Apache"},
        {"Accept-Ranges", "bytes"},
        {"Content-Length", "51"},
        {"Content-Type", "application/xml"}
    };

    create_xmlhttprequest(doc);
    if(!xhr)
        return;

    V_VT(&var) = VT_EMPTY;
    hres = IHTMLXMLHttpRequest_get_onreadystatechange(xhr, &var);
    ok(hres == S_OK, "get_onreadystatechange failed: %08x\n", hres);
    ok(V_VT(&var) == VT_NULL, "V_VT(onreadystatechange) = %d\n", V_VT(&var));

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)&xmlhttprequest_onreadystatechange_obj;
    hres = IHTMLXMLHttpRequest_put_onreadystatechange(xhr, var);
    ok(hres == S_OK, "put_onreadystatechange failed: %08x\n", hres);

    V_VT(&var) = VT_EMPTY;
    hres = IHTMLXMLHttpRequest_get_onreadystatechange(xhr, &var);
    ok(hres == S_OK, "get_onreadystatechange failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(onreadystatechange) = %d\n", V_VT(&var));
    ok(V_DISPATCH(&var) == (IDispatch*)&xmlhttprequest_onreadystatechange_obj, "unexpected onreadystatechange value\n");

    hres = IHTMLXMLHttpRequest_get_readyState(xhr, NULL);
    ok(hres == E_POINTER, "Expect E_POINTER, got %08x\n", hres);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 0, "Expect UNSENT, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_status(xhr, NULL);
    ok(hres == E_POINTER, "Expect E_POINTER, got %08x\n", hres);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(val == 0, "Expect 0, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_statusText(xhr, NULL);
    ok(hres == E_POINTER, "Expect E_POINTER, got %08x\n", hres);

    hres = IHTMLXMLHttpRequest_get_statusText(xhr, &text);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(text == NULL, "Expect NULL, got %p\n", text);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, content_type, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    method = a2bstr("GET");
    url = a2bstr(xml_url);
    V_VT(&vbool) = VT_BOOL;
    V_BOOL(&vbool) = VARIANT_FALSE;
    V_VT(&vempty) = VT_EMPTY;

    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    hres = IHTMLXMLHttpRequest_open(xhr, method, url, vbool, vempty, vempty);
    todo_wine ok(hres == S_OK, "open failed: %08x\n", hres); /* Gecko 30+ only supports async */
    todo_wine CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);

    SysFreeString(method);
    SysFreeString(url);

    if(FAILED(hres)) {
        IHTMLXMLHttpRequest_Release(xhr);
        xhr = NULL;
        return;
    }

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, content_type, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(val == 0, "Expect 0, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_statusText(xhr, &text);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(text == NULL, "Expect NULL, got %p\n", text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 1, "Expect OPENED, got %d\n", val);

    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    SET_EXPECT(xmlhttprequest_onreadystatechange_headers_received);
    SET_EXPECT(xmlhttprequest_onreadystatechange_loading);
    SET_EXPECT(xmlhttprequest_onreadystatechange_done);
    loading_cnt = 0;
    hres = IHTMLXMLHttpRequest_send(xhr, vempty);
    ok(hres == S_OK, "send failed: %08x\n", hres);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_headers_received);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_loading);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_done);
    ok(loading_cnt == 1, "loading_cnt = %d\n", loading_cnt);

    text = NULL;
    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, content_type, &text);
    ok(hres == S_OK, "getResponseHeader failed, got %08x\n", hres);
    ok(text != NULL, "text == NULL\n");
    SysFreeString(text);

    if(expect_text)
        test_header(expect_headers, sizeof(expect_headers)/sizeof(expect_headers[0]));

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == S_OK, "get_status failed: %08x\n", hres);
    ok(val == 200, "Expect 200, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_statusText(xhr, &text);
    ok(hres == S_OK, "get_statusText failed: %08x\n", hres);
    ok(text != NULL, "text == NULL\n");
    ok(!strcmp_wa(text, "OK"),
        "Expected \"OK\", got %s\n", wine_dbgstr_w(text));
    SysFreeString(text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 4, "Expect DONE, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_responseText(xhr, &text);
    ok(hres == S_OK, "get_responseText failed: %08x\n", hres);
    ok(text != NULL, "test == NULL\n");
    if(expect_text)
        ok(!strcmp_wa(text, expect_text), "expect %s, got %s\n",
            expect_text, wine_dbgstr_w(text));
    SysFreeString(text);

    IHTMLXMLHttpRequest_Release(xhr);
    xhr = NULL;
}

static void test_async_xhr(IHTMLDocument2 *doc, const char *xml_url, const char *expect_text)
{
    VARIANT vbool, vempty, var;
    BSTR method, url;
    BSTR text;
    LONG val;
    HRESULT hres;
    static const struct HEADER_TYPE expect_headers[] = {
        {"Content-Length", "51"},
        {"Content-Type", "application/xml"}
    };

    create_xmlhttprequest(doc);
    if(!xhr)
        return;

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)&xmlhttprequest_onreadystatechange_obj;
    hres = IHTMLXMLHttpRequest_put_onreadystatechange(xhr, var);
    ok(hres == S_OK, "put_onreadystatechange failed: %08x\n", hres);

    V_VT(&var) = VT_EMPTY;
    hres = IHTMLXMLHttpRequest_get_onreadystatechange(xhr, &var);
    ok(hres == S_OK, "get_onreadystatechange failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(onreadystatechange) = %d\n", V_VT(&var));
    ok(V_DISPATCH(&var) == (IDispatch*)&xmlhttprequest_onreadystatechange_obj, "unexpected onreadystatechange value\n");

    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, NULL, &text);
    ok(hres == E_INVALIDARG, "Expect E_INVALIDARG, got %08x\n", hres);

    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, content_type, NULL);
    ok(hres == E_POINTER, "Expect E_POINTER, got %08x\n", hres);

    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, NULL, NULL);
    ok(hres == E_POINTER || broken(hres == E_INVALIDARG), /* Vista and before */
        "Expect E_POINTER, got %08x\n", hres);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, content_type, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, NULL);
    ok(hres == E_POINTER, "Expect E_POINTER, got %08x\n", hres);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(val == 0, "Expect 0, got %d\n", val);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_statusText(xhr, &text);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(text == NULL, "Expect NULL, got %p\n", text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 0, "Expect UNSENT, got %d\n", val);

    method = a2bstr("GET");
    url = a2bstr(xml_url);
    V_VT(&vbool) = VT_BOOL;
    V_BOOL(&vbool) = VARIANT_TRUE;
    V_VT(&vempty) = VT_EMPTY;

    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    hres = IHTMLXMLHttpRequest_open(xhr, method, url, vbool, vempty, vempty);
    ok(hres == S_OK, "open failed: %08x\n", hres);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);

    SysFreeString(method);
    SysFreeString(url);

    if(FAILED(hres)) {
        IHTMLXMLHttpRequest_Release(xhr);
        xhr = NULL;
        return;
    }

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    text = (BSTR)0xdeadbeef;
    hres = IHTMLXMLHttpRequest_getResponseHeader(xhr, content_type, &text);
    ok(hres == E_FAIL, "got %08x\n", hres);
    ok(text == NULL, "text = %p\n", text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(val == 0, "Expect 0, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_statusText(xhr, &text);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(text == NULL, "Expect NULL, got %p\n", text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 1, "Expect OPENED, got %d\n", val);

    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    SET_EXPECT(xmlhttprequest_onreadystatechange_headers_received);
    SET_EXPECT(xmlhttprequest_onreadystatechange_loading);
    SET_EXPECT(xmlhttprequest_onreadystatechange_done);
    loading_cnt = 0;
    hres = IHTMLXMLHttpRequest_send(xhr, vempty);

    ok(hres == S_OK, "send failed: %08x\n", hres);
    if(SUCCEEDED(hres))
        pump_msgs(&called_xmlhttprequest_onreadystatechange_done);
    todo_wine CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_headers_received);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_loading);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_done);
    /* Workaround for loading large files */
    if(expect_text)
        ok(loading_cnt == 1, "loading_cnt = %d\n", loading_cnt);
    else
        todo_wine ok(loading_cnt == 1, "loading_cnt = %d\n", loading_cnt);

    if(FAILED(hres)) {
        IHTMLXMLHttpRequest_Release(xhr);
        xhr = NULL;
        return;
    }

    text = NULL;
    hres = IHTMLXMLHttpRequest_getAllResponseHeaders(xhr, &text);
    ok(hres == S_OK, "getAllResponseHeader failed, got %08x\n", hres);
    ok(text != NULL, "text == NULL\n");
    SysFreeString(text);

    if(expect_text)
        test_header(expect_headers, sizeof(expect_headers)/sizeof(expect_headers[0]));

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == S_OK, "get_status failed: %08x\n", hres);
    ok(val == 200, "Expect 200, got %d\n", val);

    text = NULL;
    hres = IHTMLXMLHttpRequest_get_statusText(xhr, &text);
    ok(hres == S_OK, "get_statusText failed: %08x\n", hres);
    ok(text != NULL, "text == NULL\n");
    ok(!strcmp_wa(text, "OK"), "Expected \"OK\", got %s\n", wine_dbgstr_w(text));
    SysFreeString(text);

    val = 0xdeadbeef;
    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 4, "Expect DONE, got %d\n", val);

    text = NULL;
    hres = IHTMLXMLHttpRequest_get_responseText(xhr, &text);
    ok(hres == S_OK, "get_responseText failed: %08x\n", hres);
    ok(text != NULL, "test == NULL\n");
    if(expect_text)
        ok(!strcmp_wa(text, expect_text), "expect %s, got %s\n",
            expect_text, wine_dbgstr_w(text));
    SysFreeString(text);

    IHTMLXMLHttpRequest_Release(xhr);
    xhr = NULL;
}

static void test_async_xhr_abort(IHTMLDocument2 *doc, const char *xml_url)
{
    VARIANT vbool, vempty, var;
    BSTR method, url;
    LONG val;
    HRESULT hres;

    method = a2bstr("GET");
    url = a2bstr(xml_url);
    V_VT(&vbool) = VT_BOOL;
    V_BOOL(&vbool) = VARIANT_TRUE;
    V_VT(&vempty) = VT_EMPTY;

    trace("abort before send() is fired\n");
    create_xmlhttprequest(doc);
    if(!xhr)
        return;

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)&xmlhttprequest_onreadystatechange_obj;
    hres = IHTMLXMLHttpRequest_put_onreadystatechange(xhr, var);

    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    hres = IHTMLXMLHttpRequest_open(xhr, method, url, vbool, vempty, vempty);
    ok(hres == S_OK, "open failed: %08x\n", hres);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);

    hres = IHTMLXMLHttpRequest_abort(xhr);
    ok(hres == S_OK, "abort failed: %08x\n", hres);

    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(val == 0, "Expect 0, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 0, "Expect UNSENT, got %d\n", val);

    IHTMLXMLHttpRequest_Release(xhr);
    xhr = NULL;

    trace("abort after send() is fired\n");
    create_xmlhttprequest(doc);
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)&xmlhttprequest_onreadystatechange_obj;
    hres = IHTMLXMLHttpRequest_put_onreadystatechange(xhr, var);

    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    hres = IHTMLXMLHttpRequest_open(xhr, method, url, vbool, vempty, vempty);
    ok(hres == S_OK, "open failed: %08x\n", hres);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);

    loading_cnt = 0;
    readystatechange_cnt = 0;
    SET_EXPECT(xmlhttprequest_onreadystatechange_opened);
    SET_EXPECT(xmlhttprequest_onreadystatechange_done);
    hres = IHTMLXMLHttpRequest_send(xhr, vempty);
    ok(hres == S_OK, "send failed: %08x\n", hres);
    todo_wine CHECK_CALLED(xmlhttprequest_onreadystatechange_opened);

    hres = IHTMLXMLHttpRequest_abort(xhr);
    ok(hres == S_OK, "abort failed: %08x\n", hres);
    CHECK_CALLED(xmlhttprequest_onreadystatechange_done);

    hres = IHTMLXMLHttpRequest_get_readyState(xhr, &val);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(val == 0, "Expect UNSENT, got %d\n", val);

    hres = IHTMLXMLHttpRequest_get_status(xhr, &val);
    ok(hres == E_FAIL, "Expect E_FAIL, got: %08x\n", hres);
    ok(val == 0, "Expect 0, got %d\n", val);

    ok(loading_cnt == 0, "loading_cnt = %d, expect 0, loading_cnt\n", loading_cnt);
    todo_wine ok(readystatechange_cnt == 2, "readystatechange_cnt = %d, expect 2\n", readystatechange_cnt);

    IHTMLXMLHttpRequest_Release(xhr);
    xhr = NULL;

    SysFreeString(method);
    SysFreeString(url);
}

static IHTMLDocument2 *create_doc_from_url(const char *start_url)
{
    BSTR url;
    IBindCtx *bc;
    IMoniker *url_mon;
    IPersistMoniker *persist_mon;
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CreateBindCtx(0, &bc);
    ok(hres == S_OK, "CreateBindCtx failed: 0x%08x\n", hres);

    url = a2bstr(start_url);
    hres = CreateURLMoniker(NULL, url, &url_mon);
    ok(hres == S_OK, "CreateURLMoniker failed: 0x%08x\n", hres);

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER, &IID_IHTMLDocument2,
            (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: 0x%08x\n", hres);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IPersistMoniker,
            (void**)&persist_mon);
    ok(hres == S_OK, "IHTMLDocument2_QueryInterface failed: 0x%08x\n", hres);

    hres = IPersistMoniker_Load(persist_mon, FALSE, url_mon, bc,
            STGM_SHARE_EXCLUSIVE | STGM_READWRITE);
    ok(hres == S_OK, "IPersistMoniker_Load failed: 0x%08x\n", hres);

    IPersistMoniker_Release(persist_mon);
    IMoniker_Release(url_mon);
    IBindCtx_Release(bc);
    SysFreeString(url);

    doc_complete = FALSE;
    notif_doc = doc;
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);
    pump_msgs(&doc_complete);

    return doc;
}

START_TEST(xmlhttprequest)
{
    IHTMLDocument2 *doc;
    static const char start_url[] = "http://test.winehq.org/tests/hello.html";
    static const char xml_url[] = "http://test.winehq.org/tests/xmltest.xml";
    static const char large_page_url[] = "http://test.winehq.org/tests/data.php";
    static const char expect_response_text[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<a>TEST</a>\n";

    CoInitialize(NULL);

    content_type = a2bstr("Content-Type");
    doc = create_doc_from_url(start_url);
    if(doc) {
        test_sync_xhr(doc, xml_url, expect_response_text);
        test_sync_xhr(doc, large_page_url, NULL);
        test_async_xhr(doc, xml_url, expect_response_text);
        test_async_xhr(doc, large_page_url, NULL);
        test_async_xhr_abort(doc, large_page_url);
        IHTMLDocument2_Release(doc);
    }
    SysFreeString(content_type);

    CoUninitialize();
}
