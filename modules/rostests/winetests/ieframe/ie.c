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

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "exdisp.h"
#include "exdispid.h"
#include "mshtml.h"
#include "initguid.h"
#include "ieautomation.h"

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

DEFINE_EXPECT(Invoke_BEFORENAVIGATE2);
DEFINE_EXPECT(Invoke_NAVIGATECOMPLETE2);
DEFINE_EXPECT(Invoke_NAVIGATEERROR);
DEFINE_EXPECT(Invoke_DOCUMENTCOMPLETE);

static BOOL navigate_complete, navigation_timed_out;
static const WCHAR *navigate_url;

static BSTR get_window_url(IDispatch *webbrowser)
{
    IHTMLPrivateWindow *priv_window;
    IHTMLLocation *location;
    IHTMLWindow2 *window;
    IServiceProvider *sp;
    IHTMLDocument2 *doc;
    HRESULT hres;
    BSTR url;

    hres = IDispatch_QueryInterface(webbrowser, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "QueryInterface(IServiceProvider) failed: %08lx\n", hres);

    hres = IServiceProvider_QueryService(sp, &SID_SHTMLWindow, &IID_IHTMLWindow2, (void**)&window);
    ok(hres == S_OK, "Could not get SHTMLWindow service: %08lx\n", hres);
    ok(window != NULL, "window = NULL\n");
    IServiceProvider_Release(sp);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLPrivateWindow, (void**)&priv_window);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IHTMLPrivateWindow) returned: %08lx\n", hres);

    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08lx\n", hres);
    ok(doc != NULL, "doc = NULL\n");
    IHTMLWindow2_Release(window);

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08lx\n", hres);
    ok(window != NULL, "window = NULL\n");
    IHTMLDocument2_Release(doc);

    hres = IHTMLWindow2_get_location(window, &location);
    ok(hres == S_OK, "get_location failed: %08lx\n", hres);
    ok(location != NULL, "location = NULL\n");
    IHTMLWindow2_Release(window);

    hres = IHTMLLocation_get_href(location, &url);
    ok(hres == S_OK, "get_href failed: %08lx\n", hres);
    ok(url != NULL, "url = NULL\n");
    IHTMLLocation_Release(location);

    return url;
}

#define test_url(a) _test_url(__LINE__,a)
static void _test_url(unsigned line, const WCHAR *url)
{
    /* If error page, it actually returns the error page's resource URL, followed by #, followed by the original URL.
       Since the error page's location varies on native, and depends on the error itself, just check for res:// here. */
    if(called_Invoke_NAVIGATEERROR) {
        ok_(__FILE__,line)(!wcsncmp(url, L"res://", ARRAY_SIZE(L"res://")-1), "url is not a local resource: %s\n", wine_dbgstr_w(url));
        url = wcschr(url, '#');
        ok_(__FILE__,line)(url != NULL, "url has no fragment: %s\n", wine_dbgstr_w(url));
        ok_(__FILE__,line)(!wcscmp(url + 1, navigate_url), "url after fragment = %s\n", wine_dbgstr_w(url + 1));
    }else {
        ok_(__FILE__,line)(!wcscmp(url, navigate_url), "url = %s\n", wine_dbgstr_w(url));
    }
}

static HRESULT WINAPI Dispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
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

static HRESULT WINAPI Dispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Dispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    VARIANT *arg;
    BSTR url;

    switch(dispIdMember) {
    case DISPID_BEFORENAVIGATE2:
        CHECK_EXPECT(Invoke_BEFORENAVIGATE2);

        arg = pDispParams->rgvarg + 5;
        ok(V_VT(arg) == (VT_BYREF | VT_VARIANT), "VT = %d\n", V_VT(arg));
        ok(V_VT(V_VARIANTREF(arg)) == VT_BSTR, "VT = %d\n", V_VT(V_VARIANTREF(arg)));

        test_url(V_BSTR(V_VARIANTREF(arg)));
        return S_OK;
    case DISPID_NAVIGATECOMPLETE2:
        CHECK_EXPECT(Invoke_NAVIGATECOMPLETE2);

        arg = pDispParams->rgvarg;
        ok(V_VT(arg) == (VT_BYREF | VT_VARIANT), "VT = %d\n", V_VT(arg));
        ok(V_VT(V_VARIANTREF(arg)) == VT_BSTR, "VT = %d\n", V_VT(V_VARIANTREF(arg)));
        ok(!wcscmp(V_BSTR(V_VARIANTREF(arg)), navigate_url), "url = %s\n", wine_dbgstr_w(V_BSTR(V_VARIANTREF(arg))));

        arg = pDispParams->rgvarg + 1;
        ok(V_VT(arg) == VT_DISPATCH, "VT = %d\n", V_VT(arg));
        ok(V_DISPATCH(arg) != NULL, "V_DISPATCH = NULL\n");
        url = get_window_url(V_DISPATCH(arg));
        test_url(url);
        SysFreeString(url);
        return S_OK;
    case DISPID_NAVIGATEERROR:
        CHECK_EXPECT(Invoke_NAVIGATEERROR);
        ok(!called_Invoke_NAVIGATECOMPLETE2, "NAVIGATECOMPLETE2 called before NAVIGATEERROR\n");

        arg = pDispParams->rgvarg;
        ok(V_VT(arg) == (VT_BYREF | VT_BOOL), "VT = %d\n", V_VT(arg));
        ok(*V_BOOLREF(arg) == VARIANT_FALSE, "cancel = %#x\n", *V_BOOLREF(arg));

        arg = pDispParams->rgvarg + 3;
        ok(V_VT(arg) == (VT_BYREF | VT_VARIANT), "VT = %d\n", V_VT(arg));
        ok(V_VT(V_VARIANTREF(arg)) == VT_BSTR, "VT = %d\n", V_VT(V_VARIANTREF(arg)));
        ok(!wcscmp(V_BSTR(V_VARIANTREF(arg)), navigate_url), "url = %s\n", wine_dbgstr_w(V_BSTR(V_VARIANTREF(arg))));
        return S_OK;
    case DISPID_DOCUMENTCOMPLETE:
        CHECK_EXPECT(Invoke_DOCUMENTCOMPLETE);
        navigate_complete = TRUE;

        arg = pDispParams->rgvarg;
        ok(V_VT(arg) == (VT_BYREF | VT_VARIANT), "VT = %d\n", V_VT(arg));
        ok(V_VT(V_VARIANTREF(arg)) == VT_BSTR, "VT = %d\n", V_VT(V_VARIANTREF(arg)));
        ok(!wcscmp(V_BSTR(V_VARIANTREF(arg)), navigate_url), "url = %s\n", wine_dbgstr_w(V_BSTR(V_VARIANTREF(arg))));

        arg = pDispParams->rgvarg + 1;
        ok(V_VT(arg) == VT_DISPATCH, "VT = %d\n", V_VT(arg));
        ok(V_DISPATCH(arg) != NULL, "V_DISPATCH = NULL\n");
        url = get_window_url(V_DISPATCH(arg));
        test_url(url);
        SysFreeString(url);
        return S_OK;
    }

    return E_NOTIMPL;
}

static IDispatchVtbl DispatchVtbl = {
    Dispatch_QueryInterface,
    Dispatch_AddRef,
    Dispatch_Release,
    Dispatch_GetTypeInfoCount,
    Dispatch_GetTypeInfo,
    Dispatch_GetIDsOfNames,
    Dispatch_Invoke
};

static IDispatch Dispatch = { &DispatchVtbl };

static void advise_cp(IUnknown *unk, BOOL init)
{
    IConnectionPointContainer *container;
    IConnectionPoint *point;
    HRESULT hres;

    static DWORD cookie = 100;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IConnectionPointContainer_FindConnectionPoint(container, &DIID_DWebBrowserEvents2, &point);
    IConnectionPointContainer_Release(container);
    if(!navigation_timed_out)
        ok(hres == S_OK, "FindConnectionPoint failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    if(init) {
        hres = IConnectionPoint_Advise(point, (IUnknown*)&Dispatch, &cookie);
        ok(hres == S_OK, "Advise failed: %08lx\n", hres);
    }else {
        hres = IConnectionPoint_Unadvise(point, cookie);
        ok(hres == S_OK, "Unadvise failed: %08lx\n", hres);
    }

    IConnectionPoint_Release(point);

}

static void test_visible(IWebBrowser2 *wb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0x100;
    hres = IWebBrowser2_get_Visible(wb, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "Visible = %x\n", b);

    hres = IWebBrowser2_put_Visible(wb, VARIANT_TRUE);
    ok(hres == S_OK, "put_Visible failed: %08lx\n", hres);

    b = 0x100;
    hres = IWebBrowser2_get_Visible(wb, &b);
    ok(hres == S_OK, "get_Visible failed: %08lx\n", hres);
    ok(b == VARIANT_TRUE, "Visible = %x\n", b);

    hres = IWebBrowser2_put_Visible(wb, VARIANT_FALSE);
    ok(hres == S_OK, "put_Visible failed: %08lx\n", hres);
}

static void test_html_window(IWebBrowser2 *wb)
{
    IHTMLWindow2 *html_window;
    IServiceProvider *sp;
    HRESULT hres;

    hres = IWebBrowser2_QueryInterface(wb, &IID_IServiceProvider, (void**)&sp);
    ok(hres == S_OK, "Could not get IServiceProvider iface: %08lx\n", hres);

    hres = IServiceProvider_QueryService(sp, &SID_SHTMLWindow, &IID_IHTMLWindow2, (void**)&html_window);
    IServiceProvider_Release(sp);
    ok(hres == S_OK, "Could not get SHTMLWindow service: %08lx\n", hres);

    IHTMLWindow2_Release(html_window);
}

static void test_window(IWebBrowser2 *wb)
{
    SHANDLE_PTR handle = 0;
    HWND hwnd = NULL;
    char buf[100];
    HRESULT hres;

    hres = IWebBrowser2_get_HWND(wb, &handle);
    ok(hres == S_OK, "get_HWND failed: %08lx\n", hres);
    ok(handle, "handle == 0\n");

    hwnd = (HWND)handle;
    GetClassNameA(hwnd, buf, sizeof(buf));
    ok(!strcmp(buf, "IEFrame"), "Unexpected class name %s\n", buf);
}

static void CALLBACK navigate_timeout(HWND hwnd, UINT msg, UINT_PTR timer, DWORD time)
{
    win_skip("Navigation timed out, skipping tests...\n");
    called_Invoke_BEFORENAVIGATE2 = TRUE;
    called_Invoke_NAVIGATECOMPLETE2 = TRUE;
    called_Invoke_DOCUMENTCOMPLETE = TRUE;
    if(expect_Invoke_NAVIGATEERROR)
        CHECK_EXPECT(Invoke_NAVIGATEERROR);
    navigation_timed_out = TRUE;
    navigate_complete = TRUE;
}

static void test_navigate(IWebBrowser2 *wb, const WCHAR *url, DWORD timeout)
{
    VARIANT urlv, emptyv;
    UINT_PTR timer = 0;
    MSG msg;
    HRESULT hres;

    SET_EXPECT(Invoke_BEFORENAVIGATE2);
    SET_EXPECT(Invoke_NAVIGATECOMPLETE2);
    SET_EXPECT(Invoke_DOCUMENTCOMPLETE);
    navigation_timed_out = FALSE;
    navigate_complete = FALSE;
    navigate_url = url;

    V_VT(&urlv) = VT_BSTR;
    V_BSTR(&urlv) = SysAllocString(url);
    V_VT(&emptyv) = VT_EMPTY;
    hres = IWebBrowser2_Navigate2(wb, &urlv, &emptyv, &emptyv, &emptyv, &emptyv);
    ok(hres == S_OK, "Navigate2 failed: %08lx\n", hres);
    SysFreeString(V_BSTR(&urlv));

    if(timeout)
        timer = SetTimer(NULL, 0, timeout, navigate_timeout);

    while(!navigate_complete && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if(timer)
        KillTimer(NULL, timer);

    CHECK_CALLED(Invoke_BEFORENAVIGATE2);
    CHECK_CALLED(Invoke_NAVIGATECOMPLETE2);
    CHECK_CALLED(Invoke_DOCUMENTCOMPLETE);
}

static void test_busy(IWebBrowser2 *wb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0xdead;
    hres = IWebBrowser2_get_Busy(wb, &b);
    ok(hres == S_OK, "get_Busy failed: %08lx\n", hres);
    ok(b == VARIANT_FALSE, "Busy = %x\n", b);
}

static void test_InternetExplorer(void)
{
    IWebBrowser2 *wb;
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_InternetExplorer, NULL, CLSCTX_SERVER,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Could not create InternetExplorer instance: %08lx\n", hres);

    if(hres != S_OK)
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IWebBrowser2, (void**)&wb);
    ok(hres == S_OK, "Could not get IWebBrowser2 interface: %08lx\n", hres);
    if (hres != S_OK) {
        IUnknown_Release(unk);
        return;
    }

    advise_cp(unk, TRUE);

    test_busy(wb);
    test_visible(wb);
    test_html_window(wb);
    test_window(wb);
    test_navigate(wb, L"http://test.winehq.org/tests/hello.html", 0);

    SET_EXPECT(Invoke_NAVIGATEERROR);
    test_navigate(wb, L"http://0.0.0.0:1234/#frag?query=foo&wine=bar", 6000);
    CHECK_CALLED(Invoke_NAVIGATEERROR);

    advise_cp(unk, FALSE);

    IWebBrowser2_Release(wb);
    ref = IUnknown_Release(unk);
    ok(!ref, "object not destroyed, ref=%lu\n", ref);
}

static void test_InternetExplorerManager(void)
{
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_InternetExplorerManager, NULL, CLSCTX_LOCAL_SERVER,
            &IID_IInternetExplorerManager, (void**)&unk);
    ok(hres == S_OK || broken(hres == REGDB_E_CLASSNOTREG), "Could not create InternetExplorerManager instance: %08lx\n", hres);

    if(hres != S_OK)
    {
        win_skip("InternetExplorerManager not available\n");
        return;
    }

    ref = IUnknown_Release(unk);
    ok(!ref, "object not destroyed, ref=%lu\n", ref);
}

START_TEST(ie)
{
    CoInitialize(NULL);

    test_InternetExplorerManager();

    test_InternetExplorer();

    CoUninitialize();
}
