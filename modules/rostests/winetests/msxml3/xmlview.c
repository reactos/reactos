/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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

#include <stdio.h>
#include <assert.h>

#include "windows.h"
#include "ole2.h"
#include "mshtml.h"
#include "mshtmdid.h"
#include "initguid.h"
#include "perhist.h"
#include "docobj.h"
#include "urlmon.h"
#include "xmlparser.h"

#include "wine/test.h"

HRESULT (WINAPI *pCreateURLMoniker)(IMoniker*, LPCWSTR, IMoniker**);

static const char xmlview_html[] =
"\r\n"
"<BODY><H2>Generated HTML</H2>\r\n"
"<TABLE>\r\n"
"<TBODY>\r\n"
"<TR bgColor=green>\r\n"
"<TH>Title</TH>\r\n"
"<TH>Value</TH></TR>\r\n"
"<TR>\r\n"
"<TD>title1</TD>\r\n"
"<TD>value1</TD></TR>\r\n"
"<TR>\r\n"
"<TD>title2</TD>\r\n"
"<TD>value2</TD></TR></TBODY></TABLE></BODY>";

IHTMLDocument2 *html_doc;
BOOL loaded;

static int html_src_compare(LPCWSTR strw, const char *stra)
{
    char buf[2048], *p1;
    const char *p2;

    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);

    p1 = buf;
    p2 = stra;
    while(1) {
        while(*p1=='\r' || *p1=='\n' || *p1=='\"') p1++;
        while(*p2=='\r' || *p2=='\n') p2++;

        if(!*p1 || !*p2 || tolower(*p1)!=tolower(*p2))
            break;

        p1++;
        p2++;
    }

    return *p1 != *p2;
}

static HRESULT WINAPI HTMLEvents_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "Unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLEvents_AddRef(IDispatch *iface)
{
    return 2;
}

static ULONG WINAPI HTMLEvents_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI HTMLEvents_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEvents_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEvents_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEvents_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    if (dispIdMember == DISPID_HTMLDOCUMENTEVENTS2_ONREADYSTATECHANGE)
    {
        static const WCHAR completeW[] = L"complete";
        HRESULT hr;
        BSTR state;

        hr = IHTMLDocument2_get_readyState(html_doc, &state);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (!memcmp(state, completeW, sizeof(completeW)))
            loaded = TRUE;
        SysFreeString(state);
    }

    return S_OK;
}

static const IDispatchVtbl HTMLEventsVtbl = {
    HTMLEvents_QueryInterface,
    HTMLEvents_AddRef,
    HTMLEvents_Release,
    HTMLEvents_GetTypeInfoCount,
    HTMLEvents_GetTypeInfo,
    HTMLEvents_GetIDsOfNames,
    HTMLEvents_Invoke
};

static IDispatch HTMLEvents = { &HTMLEventsVtbl };

static void test_QueryInterface(void)
{
    IUnknown *xmlview, *unk;
    IHTMLDocument *htmldoc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_XMLView, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IUnknown, (void**)&xmlview);
    if(FAILED(hres)) {
        win_skip("Failed to create XMLView instance\n");
        return;
    }
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);

    hres = IUnknown_QueryInterface(xmlview, &IID_IPersistMoniker, (void**)&unk);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IPersistHistory, (void**)&unk);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IOleCommandTarget, (void**)&unk);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IOleObject, (void**)&unk);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    IUnknown_Release(unk);

    hres = IUnknown_QueryInterface(xmlview, &IID_IHTMLDocument, (void**)&htmldoc);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = IHTMLDocument_QueryInterface(htmldoc, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    ok(unk == xmlview, "Aggregation is not working as expected\n");
    IUnknown_Release(unk);
    IHTMLDocument_Release(htmldoc);

    IUnknown_Release(xmlview);
}

static void test_Load(void)
{
    WCHAR buf[1024];
    IPersistMoniker *pers_mon;
    IConnectionPointContainer *cpc;
    IConnectionPoint *cp;
    IMoniker *mon;
    IBindCtx *bctx;
    IHTMLElement *elem;
    HRESULT hres;
    MSG msg;
    BSTR source;

    lstrcpyW(buf, L"res://");
    GetModuleFileNameW(NULL, buf+lstrlenW(buf), ARRAY_SIZE(buf)-ARRAY_SIZE(L"res://"));
    lstrcatW(buf, L"/xml/xmlview.xml");

    if(!pCreateURLMoniker) {
        win_skip("CreateURLMoniker not available\n");
        return;
    }

    hres = CoCreateInstance(&CLSID_XMLView, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IPersistMoniker, (void**)&pers_mon);
    if(FAILED(hres)) {
        win_skip("Failed to create XMLView instance\n");
        return;
    }
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);

    hres = IPersistMoniker_QueryInterface(pers_mon, &IID_IHTMLDocument2, (void**)&html_doc);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = IPersistMoniker_QueryInterface(pers_mon, &IID_IConnectionPointContainer, (void**)&cpc);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = IConnectionPointContainer_FindConnectionPoint(cpc, &IID_IDispatch, &cp);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = IConnectionPoint_Advise(cp, (IUnknown*)&HTMLEvents, NULL);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    IConnectionPoint_Release(cp);
    IConnectionPointContainer_Release(cpc);

    hres = CreateBindCtx(0, &bctx);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = pCreateURLMoniker(NULL, buf, &mon);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    loaded = FALSE;
    hres = IPersistMoniker_Load(pers_mon, TRUE, mon, bctx, 0);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    IBindCtx_Release(bctx);
    IMoniker_Release(mon);

    while(!loaded && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    hres = IHTMLDocument2_get_body(html_doc, &elem);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    hres = IHTMLElement_get_outerHTML(elem, &source);
    ok(hres == S_OK, "Unexpected hr %#lx.\n", hres);
    ok(!html_src_compare(source, xmlview_html), "Incorrect HTML source: %s\n", wine_dbgstr_w(source));
    IHTMLElement_Release(elem);
    SysFreeString(source);

    IHTMLDocument2_Release(html_doc);
    html_doc = NULL;
    IPersistMoniker_Release(pers_mon);
}

START_TEST(xmlview)
{
    HMODULE urlmon = LoadLibraryA("urlmon.dll");
    pCreateURLMoniker = (void*)GetProcAddress(urlmon, "CreateURLMoniker");

    CoInitialize(NULL);
    test_QueryInterface();
    test_Load();
    CoUninitialize();
}
