/*
 * XML test
 *
 * Copyright 2010-2012 Nikolay Sivov for CodeWeavers
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

#include "msxml2.h"
#include "msxml2did.h"
#include "dispex.h"

#include "initguid.h"
#include "objsafe.h"
#include "mshtml.h"

#include "wine/test.h"

#define EXPECT_REF(node,ref) _expect_ref((IUnknown*)node, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__, line)(rc == ref, "expected refcount %ld, got %ld.\n", ref, rc);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static const char xmltestA[] = "http://test.winehq.org/tests/xmltest.xml";
static const CHAR xmltestbodyA[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<a>TEST</a>\n";

DEFINE_GUID(SID_SContainerDispatch, 0xb722be00, 0x4e68, 0x101b, 0xa2, 0xbc, 0x00, 0xaa, 0x00, 0x40, 0x47, 0x70);
DEFINE_GUID(SID_UnknownSID, 0x75dd09cb, 0x6c40, 0x11d5, 0x85, 0x43, 0x00, 0xc0, 0x4f, 0xa0, 0xfb, 0xa3);

static BOOL g_enablecallchecks;

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        if (g_enablecallchecks) \
            ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

/* object site */
DEFINE_EXPECT(site_qi_IServiceProvider);
DEFINE_EXPECT(site_qi_IXMLDOMDocument);
DEFINE_EXPECT(site_qi_IOleClientSite);

DEFINE_EXPECT(sp_queryservice_SID_SBindHost);
DEFINE_EXPECT(sp_queryservice_SID_SContainerDispatch_htmldoc2);
DEFINE_EXPECT(sp_queryservice_SID_secmgr_htmldoc2);
DEFINE_EXPECT(sp_queryservice_SID_secmgr_xmldomdoc);
DEFINE_EXPECT(sp_queryservice_SID_secmgr_secmgr);

DEFINE_EXPECT(htmldoc2_get_all);
DEFINE_EXPECT(htmldoc2_get_url);
DEFINE_EXPECT(collection_get_length);

static int g_unexpectedcall, g_expectedcall;

static int strcmp_wa(const WCHAR *strw, const char *stra)
{
    WCHAR buf[512];
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, ARRAY_SIZE(buf));
    return lstrcmpW(strw, buf);
}

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len-1);
    return ret;
}

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR _bstr_(const char *str)
{
    if(!str)
        return NULL;

    assert(alloced_bstrs_count < ARRAY_SIZE(alloced_bstrs));
    alloced_bstrs[alloced_bstrs_count] = alloc_str_from_narrow(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}


/* test IHTMLElementCollection */
static HRESULT WINAPI htmlecoll_QueryInterface(IHTMLElementCollection *iface, REFIID riid, void **ppvObject)
{
    ok(0, "unexpected call\n");
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI htmlecoll_AddRef(IHTMLElementCollection *iface)
{
    return 2;
}

static ULONG WINAPI htmlecoll_Release(IHTMLElementCollection *iface)
{
    return 1;
}

static HRESULT WINAPI htmlecoll_GetTypeInfoCount(IHTMLElementCollection *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_GetTypeInfo(IHTMLElementCollection *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_GetIDsOfNames(IHTMLElementCollection *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_Invoke(IHTMLElementCollection *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_toString(IHTMLElementCollection *iface, BSTR *String)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_put_length(IHTMLElementCollection *iface, LONG v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_get_length(IHTMLElementCollection *iface, LONG *v)
{
    CHECK_EXPECT2(collection_get_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_get__newEnum(IHTMLElementCollection *iface, IUnknown **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_item(IHTMLElementCollection *iface, VARIANT name, VARIANT index, IDispatch **pdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmlecoll_tags(IHTMLElementCollection *iface, VARIANT tagName, IDispatch **pdisp)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IHTMLElementCollectionVtbl TestHTMLECollectionVtbl = {
    htmlecoll_QueryInterface,
    htmlecoll_AddRef,
    htmlecoll_Release,
    htmlecoll_GetTypeInfoCount,
    htmlecoll_GetTypeInfo,
    htmlecoll_GetIDsOfNames,
    htmlecoll_Invoke,
    htmlecoll_toString,
    htmlecoll_put_length,
    htmlecoll_get_length,
    htmlecoll_get__newEnum,
    htmlecoll_item,
    htmlecoll_tags
};

static IHTMLElementCollection htmlecoll = { &TestHTMLECollectionVtbl };

/* test IHTMLDocument2 */
static HRESULT WINAPI htmldoc2_QueryInterface(IHTMLDocument2 *iface, REFIID riid, void **ppvObject)
{
   *ppvObject = NULL;
   return E_NOINTERFACE;
}

static ULONG WINAPI htmldoc2_AddRef(IHTMLDocument2 *iface)
{
    return 2;
}

static ULONG WINAPI htmldoc2_Release(IHTMLDocument2 *iface)
{
    return 1;
}

static HRESULT WINAPI htmldoc2_GetTypeInfoCount(IHTMLDocument2 *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_GetTypeInfo(IHTMLDocument2 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_GetIDsOfNames(IHTMLDocument2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_Invoke(IHTMLDocument2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_Script(IHTMLDocument2 *iface, IDispatch **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_all(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    CHECK_EXPECT2(htmldoc2_get_all);
    *p = &htmlecoll;
    return S_OK;
}

static HRESULT WINAPI htmldoc2_get_body(IHTMLDocument2 *iface, IHTMLElement **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_activeElement(IHTMLDocument2 *iface, IHTMLElement **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_images(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_applets(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_links(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_forms(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_anchors(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_title(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_title(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_scripts(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_designMode(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_designMode(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_selection(IHTMLDocument2 *iface, IHTMLSelectionObject **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_readyState(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_frames(IHTMLDocument2 *iface, IHTMLFramesCollection2 **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_embeds(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_plugins(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_alinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_alinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_bgColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_bgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_fgColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_linkColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_linkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_vlinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_vlinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_referrer(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_location(IHTMLDocument2 *iface, IHTMLLocation **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_lastModified(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_URL(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_URL(IHTMLDocument2 *iface, BSTR *p)
{
    CHECK_EXPECT2(htmldoc2_get_url);
    *p = SysAllocString(L"http://test.winehq.org/");
    return S_OK;
}

static HRESULT WINAPI htmldoc2_put_domain(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_domain(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_cookie(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_cookie(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_expando(IHTMLDocument2 *iface, VARIANT_BOOL v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_expando(IHTMLDocument2 *iface, VARIANT_BOOL *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_charset(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_charset(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_defaultCharset(IHTMLDocument2 *iface, BSTR v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_defaultCharset(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_mimeType(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileSize(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileCreatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileModifiedDate(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_fileUpdatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_security(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_protocol(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_nameProp(IHTMLDocument2 *iface, BSTR *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_write(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_writeln(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_open(IHTMLDocument2 *iface, BSTR url, VARIANT name,
                        VARIANT features, VARIANT replace, IDispatch **pomWindowResult)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_close(IHTMLDocument2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_clear(IHTMLDocument2 *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandSupported(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandEnabled(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandState(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandIndeterm(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandText(IHTMLDocument2 *iface, BSTR cmdID,
                                                        BSTR *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_queryCommandValue(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_execCommand(IHTMLDocument2 *iface, BSTR cmdID,
                                VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_execCommandShowHelp(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_createElement(IHTMLDocument2 *iface, BSTR eTag,
                                                 IHTMLElement **newElem)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onhelp(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onhelp(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onclick(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onclick(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_ondblclick(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_ondblclick(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onkeyup(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onkeyup(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onkeydown(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onkeydown(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onkeypress(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onkeypress(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmouseup(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmouseup(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmousedown(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmousedown(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmousemove(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmousemove(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmouseout(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmouseout(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onmouseover(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onmouseover(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onreadystatechange(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onreadystatechange(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onafterupdate(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onafterupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onrowexit(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onrowexit(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onrowenter(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onrowenter(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_ondragstart(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_ondragstart(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onselectstart(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onselectstart(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_elementFromPoint(IHTMLDocument2 *iface, LONG x, LONG y,
                                                        IHTMLElement **elementHit)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_parentWindow(IHTMLDocument2 *iface, IHTMLWindow2 **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_styleSheets(IHTMLDocument2 *iface,
                                                   IHTMLStyleSheetsCollection **p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onbeforeupdate(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onbeforeupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_put_onerrorupdate(IHTMLDocument2 *iface, VARIANT v)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_get_onerrorupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_toString(IHTMLDocument2 *iface, BSTR *String)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI htmldoc2_createStyleSheet(IHTMLDocument2 *iface, BSTR bstrHref,
                                            LONG lIndex, IHTMLStyleSheet **ppnewStyleSheet)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IHTMLDocument2Vtbl TestHTMLDocumentVtbl = {
    htmldoc2_QueryInterface,
    htmldoc2_AddRef,
    htmldoc2_Release,
    htmldoc2_GetTypeInfoCount,
    htmldoc2_GetTypeInfo,
    htmldoc2_GetIDsOfNames,
    htmldoc2_Invoke,
    htmldoc2_get_Script,
    htmldoc2_get_all,
    htmldoc2_get_body,
    htmldoc2_get_activeElement,
    htmldoc2_get_images,
    htmldoc2_get_applets,
    htmldoc2_get_links,
    htmldoc2_get_forms,
    htmldoc2_get_anchors,
    htmldoc2_put_title,
    htmldoc2_get_title,
    htmldoc2_get_scripts,
    htmldoc2_put_designMode,
    htmldoc2_get_designMode,
    htmldoc2_get_selection,
    htmldoc2_get_readyState,
    htmldoc2_get_frames,
    htmldoc2_get_embeds,
    htmldoc2_get_plugins,
    htmldoc2_put_alinkColor,
    htmldoc2_get_alinkColor,
    htmldoc2_put_bgColor,
    htmldoc2_get_bgColor,
    htmldoc2_put_fgColor,
    htmldoc2_get_fgColor,
    htmldoc2_put_linkColor,
    htmldoc2_get_linkColor,
    htmldoc2_put_vlinkColor,
    htmldoc2_get_vlinkColor,
    htmldoc2_get_referrer,
    htmldoc2_get_location,
    htmldoc2_get_lastModified,
    htmldoc2_put_URL,
    htmldoc2_get_URL,
    htmldoc2_put_domain,
    htmldoc2_get_domain,
    htmldoc2_put_cookie,
    htmldoc2_get_cookie,
    htmldoc2_put_expando,
    htmldoc2_get_expando,
    htmldoc2_put_charset,
    htmldoc2_get_charset,
    htmldoc2_put_defaultCharset,
    htmldoc2_get_defaultCharset,
    htmldoc2_get_mimeType,
    htmldoc2_get_fileSize,
    htmldoc2_get_fileCreatedDate,
    htmldoc2_get_fileModifiedDate,
    htmldoc2_get_fileUpdatedDate,
    htmldoc2_get_security,
    htmldoc2_get_protocol,
    htmldoc2_get_nameProp,
    htmldoc2_write,
    htmldoc2_writeln,
    htmldoc2_open,
    htmldoc2_close,
    htmldoc2_clear,
    htmldoc2_queryCommandSupported,
    htmldoc2_queryCommandEnabled,
    htmldoc2_queryCommandState,
    htmldoc2_queryCommandIndeterm,
    htmldoc2_queryCommandText,
    htmldoc2_queryCommandValue,
    htmldoc2_execCommand,
    htmldoc2_execCommandShowHelp,
    htmldoc2_createElement,
    htmldoc2_put_onhelp,
    htmldoc2_get_onhelp,
    htmldoc2_put_onclick,
    htmldoc2_get_onclick,
    htmldoc2_put_ondblclick,
    htmldoc2_get_ondblclick,
    htmldoc2_put_onkeyup,
    htmldoc2_get_onkeyup,
    htmldoc2_put_onkeydown,
    htmldoc2_get_onkeydown,
    htmldoc2_put_onkeypress,
    htmldoc2_get_onkeypress,
    htmldoc2_put_onmouseup,
    htmldoc2_get_onmouseup,
    htmldoc2_put_onmousedown,
    htmldoc2_get_onmousedown,
    htmldoc2_put_onmousemove,
    htmldoc2_get_onmousemove,
    htmldoc2_put_onmouseout,
    htmldoc2_get_onmouseout,
    htmldoc2_put_onmouseover,
    htmldoc2_get_onmouseover,
    htmldoc2_put_onreadystatechange,
    htmldoc2_get_onreadystatechange,
    htmldoc2_put_onafterupdate,
    htmldoc2_get_onafterupdate,
    htmldoc2_put_onrowexit,
    htmldoc2_get_onrowexit,
    htmldoc2_put_onrowenter,
    htmldoc2_get_onrowenter,
    htmldoc2_put_ondragstart,
    htmldoc2_get_ondragstart,
    htmldoc2_put_onselectstart,
    htmldoc2_get_onselectstart,
    htmldoc2_elementFromPoint,
    htmldoc2_get_parentWindow,
    htmldoc2_get_styleSheets,
    htmldoc2_put_onbeforeupdate,
    htmldoc2_get_onbeforeupdate,
    htmldoc2_put_onerrorupdate,
    htmldoc2_get_onerrorupdate,
    htmldoc2_toString,
    htmldoc2_createStyleSheet
};

static IHTMLDocument2 htmldoc2 = { &TestHTMLDocumentVtbl };

static HRESULT WINAPI sp_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IServiceProvider))
    {
        *ppvObject = iface;
        IServiceProvider_AddRef(iface);
        return S_OK;
    }

    ok(0, "unexpected query interface: %s\n", wine_dbgstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI sp_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI sp_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI sp_QueryService(IServiceProvider *iface, REFGUID service, REFIID riid, void **obj)
{
    *obj = NULL;

    if (IsEqualGUID(service, &SID_SBindHost) &&
        IsEqualGUID(riid, &IID_IBindHost))
    {
        CHECK_EXPECT2(sp_queryservice_SID_SBindHost);
    }
    else if (IsEqualGUID(service, &SID_SContainerDispatch) &&
             IsEqualGUID(riid, &IID_IHTMLDocument2))
    {
        CHECK_EXPECT2(sp_queryservice_SID_SContainerDispatch_htmldoc2);
    }
    else if (IsEqualGUID(service, &SID_SInternetHostSecurityManager) &&
             IsEqualGUID(riid, &IID_IHTMLDocument2))
    {
        CHECK_EXPECT2(sp_queryservice_SID_secmgr_htmldoc2);
        *obj = &htmldoc2;
        return S_OK;
    }
    else if (IsEqualGUID(service, &SID_SInternetHostSecurityManager) &&
             IsEqualGUID(riid, &IID_IXMLDOMDocument))
    {
        CHECK_EXPECT2(sp_queryservice_SID_secmgr_xmldomdoc);
    }
    else if (IsEqualGUID(service, &SID_SInternetHostSecurityManager) &&
             IsEqualGUID(riid, &IID_IInternetHostSecurityManager))
    {
        CHECK_EXPECT2(sp_queryservice_SID_secmgr_secmgr);
    }
    else if (IsEqualGUID(service, &SID_UnknownSID) &&
             IsEqualGUID(riid, &IID_IStream))
    {
        /* FIXME: unidentified service id */
    }
    else if ((IsEqualGUID(service, &IID_IInternetProtocol) && IsEqualGUID(riid, &IID_IInternetProtocol)) ||
             (IsEqualGUID(service, &IID_IHttpNegotiate2) && IsEqualGUID(riid, &IID_IHttpNegotiate2)) ||
             (IsEqualGUID(service, &IID_IGetBindHandle) && IsEqualGUID(riid, &IID_IGetBindHandle)) ||
             (IsEqualGUID(service, &IID_IBindStatusCallback) && IsEqualGUID(riid, &IID_IBindStatusCallback)) ||
             (IsEqualGUID(service, &IID_IWindowForBindingUI) && IsEqualGUID(riid, &IID_IWindowForBindingUI)))
    {
    }
    else
        ok(0, "unexpected request: sid %s, riid %s\n", wine_dbgstr_guid(service), wine_dbgstr_guid(riid));

    return E_NOTIMPL;
}

static const IServiceProviderVtbl testprovVtbl =
{
    sp_QueryInterface,
    sp_AddRef,
    sp_Release,
    sp_QueryService
};

static IServiceProvider testprov = { &testprovVtbl };

static HRESULT WINAPI site_QueryInterface(IUnknown *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IServiceProvider))
        CHECK_EXPECT2(site_qi_IServiceProvider);

    if (IsEqualGUID(riid, &IID_IXMLDOMDocument))
        CHECK_EXPECT2(site_qi_IXMLDOMDocument);

    if (IsEqualGUID(riid, &IID_IOleClientSite))
        CHECK_EXPECT2(site_qi_IOleClientSite);

    if (IsEqualGUID(riid, &IID_IUnknown))
         *ppvObject = iface;
    else if (IsEqualGUID(riid, &IID_IServiceProvider))
         *ppvObject = &testprov;

    if (*ppvObject) IUnknown_AddRef(iface);

    return *ppvObject ? S_OK : E_NOINTERFACE;
}

static ULONG WINAPI site_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI site_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl testsiteVtbl =
{
    site_QueryInterface,
    site_AddRef,
    site_Release
};

static IUnknown testsite = { &testsiteVtbl };

typedef struct
{
    IDispatch IDispatch_iface;
    LONG ref;
} dispevent;

static IXMLHttpRequest *httpreq;

static inline dispevent *impl_from_IDispatch( IDispatch *iface )
{
    return CONTAINING_RECORD(iface, dispevent, IDispatch_iface);
}

static HRESULT WINAPI dispevent_QueryInterface(IDispatch *iface, REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = iface;
    }
    else
        return E_NOINTERFACE;

    IDispatch_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI dispevent_AddRef(IDispatch *iface)
{
    dispevent *This = impl_from_IDispatch( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI dispevent_Release(IDispatch *iface)
{
    dispevent *This = impl_from_IDispatch( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI dispevent_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    g_unexpectedcall++;
    *pctinfo = 0;
    return S_OK;
}

static HRESULT WINAPI dispevent_GetTypeInfo(IDispatch *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    g_unexpectedcall++;
    return S_OK;
}

static HRESULT WINAPI dispevent_GetIDsOfNames(IDispatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    g_unexpectedcall++;
    return S_OK;
}

static HRESULT WINAPI dispevent_Invoke(IDispatch *iface, DISPID member, REFIID riid,
        LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *result,
        EXCEPINFO *excepInfo, UINT *argErr)
{
    LONG state;
    HRESULT hr;

    ok(!member, "expected 0 member, got %ld.\n", member);
    ok(lcid == LOCALE_SYSTEM_DEFAULT, "expected LOCALE_SYSTEM_DEFAULT, got lcid %lx.\n", lcid);
    ok(flags == DISPATCH_METHOD, "expected DISPATCH_METHOD, got %d\n", flags);

    ok(params->cArgs == 0, "got %d\n", params->cArgs);
    ok(params->cNamedArgs == 0, "got %d\n", params->cNamedArgs);
    ok(params->rgvarg == NULL, "got %p\n", params->rgvarg);
    ok(params->rgdispidNamedArgs == NULL, "got %p\n", params->rgdispidNamedArgs);

    ok(result == NULL, "got %p\n", result);
    ok(excepInfo == NULL, "got %p\n", excepInfo);
    ok(argErr == NULL, "got %p\n", argErr);

    g_expectedcall++;

    state = READYSTATE_UNINITIALIZED;
    hr = IXMLHttpRequest_get_readyState(httpreq, &state);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (state == READYSTATE_COMPLETE)
    {
        BSTR text = NULL;

        hr = IXMLHttpRequest_get_responseText(httpreq, &text);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        SysFreeString(text);
    }

    return E_FAIL;
}

static const IDispatchVtbl dispeventVtbl =
{
    dispevent_QueryInterface,
    dispevent_AddRef,
    dispevent_Release,
    dispevent_GetTypeInfoCount,
    dispevent_GetTypeInfo,
    dispevent_GetIDsOfNames,
    dispevent_Invoke
};

static IDispatch* create_dispevent(void)
{
    dispevent *event = malloc(sizeof(*event));

    event->IDispatch_iface.lpVtbl = &dispeventVtbl;
    event->ref = 1;

    return &event->IDispatch_iface;
}

static IXMLHttpRequest *create_xhr(void)
{
    IXMLHttpRequest *ret;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER,
        &IID_IXMLHttpRequest, (void**)&ret);

    return SUCCEEDED(hr) ? ret : NULL;
}

static IServerXMLHTTPRequest *create_server_xhr(void)
{
    IServerXMLHTTPRequest *ret;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ServerXMLHTTP30, NULL, CLSCTX_INPROC_SERVER, &IID_IServerXMLHTTPRequest, (void **)&ret);

    return SUCCEEDED(hr) ? ret : NULL;
}

static void set_safety_opt(IUnknown *unk, DWORD mask, DWORD opts)
{
    IObjectSafety *obj_safety;
    HRESULT hr;

    hr = IUnknown_QueryInterface(unk, &IID_IObjectSafety, (void**)&obj_safety);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IObjectSafety_SetInterfaceSafetyOptions(obj_safety, &IID_IDispatch, mask, mask&opts);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IObjectSafety_Release(obj_safety);
}

static void set_xhr_site(IXMLHttpRequest *xhr)
{
    IObjectWithSite *obj_site;
    HRESULT hr;

    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_IObjectWithSite, (void**)&obj_site);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    g_enablecallchecks = TRUE;

    SET_EXPECT(site_qi_IServiceProvider);
    SET_EXPECT(sp_queryservice_SID_SBindHost);
    SET_EXPECT(sp_queryservice_SID_SContainerDispatch_htmldoc2);
    SET_EXPECT(sp_queryservice_SID_secmgr_htmldoc2);
    SET_EXPECT(sp_queryservice_SID_secmgr_xmldomdoc);
    SET_EXPECT(sp_queryservice_SID_secmgr_secmgr);

    /* calls to IHTMLDocument2 */
    SET_EXPECT(htmldoc2_get_all);
    SET_EXPECT(collection_get_length);
    SET_EXPECT(htmldoc2_get_url);

    SET_EXPECT(site_qi_IXMLDOMDocument);
    SET_EXPECT(site_qi_IOleClientSite);

    hr = IObjectWithSite_SetSite(obj_site, &testsite);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    CHECK_CALLED(site_qi_IServiceProvider);
    todo_wine
    CHECK_CALLED(sp_queryservice_SID_SBindHost);
    CHECK_CALLED(sp_queryservice_SID_SContainerDispatch_htmldoc2);
    CHECK_CALLED(sp_queryservice_SID_secmgr_htmldoc2);
    todo_wine
    CHECK_CALLED(sp_queryservice_SID_secmgr_xmldomdoc);
    /* this one isn't very reliable
    CHECK_CALLED(sp_queryservice_SID_secmgr_secmgr); */
todo_wine {
    CHECK_CALLED(htmldoc2_get_all);
    CHECK_CALLED(collection_get_length);
}
    CHECK_CALLED(htmldoc2_get_url);

todo_wine {
    CHECK_CALLED(site_qi_IXMLDOMDocument);
    CHECK_CALLED(site_qi_IOleClientSite);
}

    g_enablecallchecks = FALSE;

    IObjectWithSite_Release(obj_site);
}

#define test_open(a,b,c,d) _test_open(__LINE__,a,b,c,d)
static void _test_open(unsigned line, IXMLHttpRequest *xhr, const char *method, const char *url, HRESULT exhres)
{
    VARIANT empty, vfalse;
    HRESULT hr;

    V_VT(&empty) = VT_EMPTY;
    V_VT(&vfalse) = VT_BOOL;
    V_BOOL(&vfalse) = VARIANT_FALSE;

    hr = IXMLHttpRequest_open(xhr, _bstr_(method), _bstr_(url), vfalse, empty, empty);
    ok_(__FILE__,line)(hr == exhres, "open(%s %s) failed: %#lx, expected %#lx.\n", method, url, hr, exhres);
}

#define test_server_open(a,b,c,d) _test_server_open(__LINE__,a,b,c,d)
static void _test_server_open(unsigned line, IServerXMLHTTPRequest *xhr, const char *method, const char *url, HRESULT exhres)
{
    VARIANT empty, vfalse;
    HRESULT hr;

    V_VT(&empty) = VT_EMPTY;
    V_VT(&vfalse) = VT_BOOL;
    V_BOOL(&vfalse) = VARIANT_FALSE;

    hr = IServerXMLHTTPRequest_open(xhr, _bstr_(method), _bstr_(url), vfalse, empty, empty);
    ok_(__FILE__,line)(hr == exhres, "open(%s %s) failed: %#lx, expected %#lx.\n", method, url, hr, exhres);
}

static void test_XMLHTTP(void)
{
    static const char bodyA[] = "mode=Test";
    static const char urlA[] = "http://test.winehq.org/tests/post.php";
    static const char referertesturl[] = "http://test.winehq.org/tests/referer.php";

    IXMLHttpRequest *xhr;
    IObjectWithSite *obj_site, *obj_site2;
    BSTR bstrResponse, str, str1;
    VARIANT varbody;
    VARIANT dummy;
    LONG state, status, bound;
    IDispatch *event;
    void *ptr;
    HRESULT hr;
    HGLOBAL g;

    xhr = create_xhr();

    check_interface(xhr, &IID_IXMLHttpRequest, TRUE);
    check_interface(xhr, &IID_IDispatch, TRUE);
    check_interface(xhr, &IID_IDispatchEx, FALSE);

    VariantInit(&dummy);
    V_VT(&dummy) = VT_ERROR;
    V_ERROR(&dummy) = DISP_E_MEMBERNOTFOUND;

    hr = IXMLHttpRequest_put_onreadystatechange(xhr, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_abort(xhr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&varbody) = VT_I2;
    V_I2(&varbody) = 1;
    hr = IXMLHttpRequest_get_responseBody(xhr, &varbody);
    ok(hr == E_PENDING, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&varbody) == VT_EMPTY, "got type %d\n", V_VT(&varbody));

    V_VT(&varbody) = VT_I2;
    V_I2(&varbody) = 1;
    hr = IXMLHttpRequest_get_responseStream(xhr, &varbody);
    ok(hr == E_PENDING, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&varbody) == VT_EMPTY, "got type %d\n", V_VT(&varbody));

    /* send before open */
    hr = IXMLHttpRequest_send(xhr, dummy);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "Unexpected hr %#lx.\n", hr);

    /* initial status code */
    hr = IXMLHttpRequest_get_status(xhr, NULL);
    ok(hr == E_POINTER || broken(hr == E_INVALIDARG) /* <win8 */, "Unexpected hr %#lx.\n", hr);

    status = 0xdeadbeef;
    hr = IXMLHttpRequest_get_status(xhr, &status);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "Unexpected hr %#lx.\n", hr);
    ok(status == READYSTATE_UNINITIALIZED || broken(status == 0xdeadbeef) /* <win8 */, "Unexpected state %ld.\n", status);

    hr = IXMLHttpRequest_get_statusText(xhr, &str);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* invalid parameters */
    test_open(xhr, NULL, NULL, E_INVALIDARG);
    test_open(xhr, "POST", NULL, E_INVALIDARG);
    test_open(xhr, NULL, urlA, E_INVALIDARG);

    hr = IXMLHttpRequest_setRequestHeader(xhr, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_("header1"), NULL);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_setRequestHeader(xhr, NULL, _bstr_("value1"));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_("header1"), _bstr_("value1"));
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_get_readyState(xhr, NULL);
    ok(hr == E_POINTER || broken(hr == E_INVALIDARG) /* <win8 */, "Unexpected hr %#lx.\n", hr);

    state = -1;
    hr = IXMLHttpRequest_get_readyState(xhr, &state);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(state == READYSTATE_UNINITIALIZED, "Unexpected state %ld.\n", state);

    httpreq = xhr;
    event = create_dispevent();

    EXPECT_REF(event, 1);
    hr = IXMLHttpRequest_put_onreadystatechange(xhr, event);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(event, 2);

    g_unexpectedcall = g_expectedcall = 0;

    test_open(xhr, "POST", urlA, S_OK);

    ok(g_unexpectedcall == 0, "unexpected disp event call\n");
    ok(g_expectedcall == 1 || broken(g_expectedcall == 0) /* win2k */, "no expected disp event call\n");

    /* status code after ::open() */
    status = 0xdeadbeef;
    hr = IXMLHttpRequest_get_status(xhr, &status);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "Unexpected hr %#lx.\n", hr);
    ok(status == READYSTATE_UNINITIALIZED || broken(status == 0xdeadbeef) /* <win8 */, "Unexpected state %ld.\n", status);

    state = -1;
    hr = IXMLHttpRequest_get_readyState(xhr, &state);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(state == READYSTATE_LOADING, "Unexpected state %ld.\n", state);

    hr = IXMLHttpRequest_abort(xhr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    state = -1;
    hr = IXMLHttpRequest_get_readyState(xhr, &state);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(state == READYSTATE_UNINITIALIZED || broken(state == READYSTATE_LOADING) /* win2k */,
        "Unexpected state %ld.\n", state);

    test_open(xhr, "POST", urlA, S_OK);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_("header1"), _bstr_("value1"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_setRequestHeader(xhr, NULL, _bstr_("value1"));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_setRequestHeader(xhr, _bstr_(""), _bstr_("value1"));
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&varbody) = VT_BSTR;
    V_BSTR(&varbody) = _bstr_(bodyA);

    hr = IXMLHttpRequest_send(xhr, varbody);
    if (hr == INET_E_RESOURCE_NOT_FOUND)
    {
        skip("No connection could be made with test.winehq.org\n");
        IXMLHttpRequest_Release(xhr);
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* response headers */
    hr = IXMLHttpRequest_getAllResponseHeaders(xhr, NULL);
    ok(hr == E_POINTER || broken(hr == E_INVALIDARG) /* <win8 */, "Unexpected hr %#lx.\n", hr);
    hr = IXMLHttpRequest_getAllResponseHeaders(xhr, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* status line is stripped already */
    ok(memcmp(str, L"HTTP", 4*sizeof(WCHAR)), "got response headers %s\n", wine_dbgstr_w(str));
    ok(*str, "got empty headers\n");
    hr = IXMLHttpRequest_getAllResponseHeaders(xhr, &str1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(str1 != str, "got %p\n", str1);
    SysFreeString(str1);
    SysFreeString(str);

    hr = IXMLHttpRequest_getResponseHeader(xhr, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    hr = IXMLHttpRequest_getResponseHeader(xhr, _bstr_("Date"), NULL);
    ok(hr == E_POINTER || broken(hr == E_INVALIDARG) /* <win8 */, "Unexpected hr %#lx.\n", hr);
    hr = IXMLHttpRequest_getResponseHeader(xhr, _bstr_("Date"), &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(*str != ' ', "got leading space in header %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* status code after ::send() */
    status = 0;
    hr = IXMLHttpRequest_get_status(xhr, &status);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(status == 200, "Unexpected status %ld.\n", status);

    hr = IXMLHttpRequest_get_statusText(xhr, NULL);
    ok(hr == E_POINTER || broken(hr == E_INVALIDARG) /* <win8 */, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_get_statusText(xhr, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"OK"), "got status %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* another ::send() after completed request */
    V_VT(&varbody) = VT_BSTR;
    V_BSTR(&varbody) = _bstr_(bodyA);

    hr = IXMLHttpRequest_send(xhr, varbody);
    ok(hr == E_FAIL || broken(hr == E_UNEXPECTED) /* win2k */, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_get_responseText(xhr, &bstrResponse);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* the server currently returns "FAILED" because the Content-Type header is
     * not what the server expects */
    if(hr == S_OK)
    {
        ok(!memcmp(bstrResponse, L"FAILED", 7 * sizeof(WCHAR)), "Unexpected response %s.\n", wine_dbgstr_w(bstrResponse));
        SysFreeString(bstrResponse);
    }

    /* POST: VT_VARIANT body */
    /* VT_VARIANT|VT_BYREF fails on Windows 10 */
    test_open(xhr, "POST", urlA, S_OK);

    hr = IXMLHttpRequest_send(xhr, varbody);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* GET request */
    test_open(xhr, "GET", xmltestA, S_OK);

    V_VT(&varbody) = VT_EMPTY;

    hr = IXMLHttpRequest_send(xhr, varbody);
    if (hr == INET_E_RESOURCE_NOT_FOUND)
    {
        skip("No connection could be made with test.winehq.org\n");
        IXMLHttpRequest_Release(xhr);
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_get_responseText(xhr, NULL);
    ok(hr == E_POINTER || broken(hr == E_INVALIDARG) /* <win8 */, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_get_responseText(xhr, &bstrResponse);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!memcmp(bstrResponse, _bstr_(xmltestbodyA), sizeof(xmltestbodyA)*sizeof(WCHAR)),
        "expected %s, got %s\n", xmltestbodyA, wine_dbgstr_w(bstrResponse));
    SysFreeString(bstrResponse);

    hr = IXMLHttpRequest_get_responseBody(xhr, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_get_responseBody(xhr, &varbody);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&varbody) == (VT_ARRAY|VT_UI1), "got type %d, expected %d\n", V_VT(&varbody), VT_ARRAY|VT_UI1);
    ok(SafeArrayGetDim(V_ARRAY(&varbody)) == 1, "got %d, expected one dimension\n", SafeArrayGetDim(V_ARRAY(&varbody)));

    bound = -1;
    hr = SafeArrayGetLBound(V_ARRAY(&varbody), 1, &bound);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!bound, "Unexpected bound %ld.\n", bound);

    hr = SafeArrayAccessData(V_ARRAY(&varbody), &ptr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(memcmp(ptr, xmltestbodyA, sizeof(xmltestbodyA)-1) == 0, "got wrong body data\n");
    SafeArrayUnaccessData(V_ARRAY(&varbody));

    VariantClear(&varbody);

    /* get_responseStream */
    hr = IXMLHttpRequest_get_responseStream(xhr, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_get_responseStream(xhr, &varbody);
    ok(V_VT(&varbody) == VT_UNKNOWN, "got type %d\n", V_VT(&varbody));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(V_UNKNOWN(&varbody), 1);

    g = NULL;
    hr = GetHGlobalFromStream((IStream*)V_UNKNOWN(&varbody), &g);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(g != NULL, "got %p\n", g);
    VariantClear(&varbody);

    IDispatch_Release(event);

    /* test if referrer header is sent */
    test_open(xhr, "GET", referertesturl, S_OK);

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_send(xhr, varbody);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLHttpRequest_get_responseText(xhr, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"no referer set"), "got response text %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* interaction with object site */
    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_IObjectWithSite, (void**)&obj_site);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IObjectWithSite_SetSite(obj_site, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_IObjectWithSite, (void**)&obj_site2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(obj_site == obj_site2 || broken(obj_site != obj_site2), "got new instance\n");
    IObjectWithSite_Release(obj_site2);

    set_xhr_site(xhr);

    test_open(xhr, "GET", "tests/referer.php", S_OK);
    str1 = SysAllocString(L"http://test.winehq.org/");

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_send(xhr, varbody);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLHttpRequest_get_responseText(xhr, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, str1), "got response text %s, expected %s\n", wine_dbgstr_w(str), wine_dbgstr_w(str1));
    SysFreeString(str);
    SysFreeString(str1);

    /* try to set site another time */
    hr = IObjectWithSite_SetSite(obj_site, &testsite);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IObjectWithSite_Release(obj_site);

    /* HEAD request */
    hr = IXMLHttpRequest_put_onreadystatechange(xhr, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    test_open(xhr, "HEAD", xmltestA, S_OK);

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_send(xhr, varbody);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = NULL;
    hr = IXMLHttpRequest_get_responseText(xhr, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!*str, "Unexpected text %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IXMLHttpRequest_getAllResponseHeaders(xhr, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(str && *str, "Expected response headers.\n");
    SysFreeString(str);

    IXMLHttpRequest_Release(xhr);

    /* invalid host */
    xhr = create_xhr();

    test_open(xhr, "GET", "http://invalid.host.test.winehq.org/test/path", S_OK);

    V_VT(&varbody) = VT_EMPTY;
    hr = IXMLHttpRequest_send(xhr, varbody);
    todo_wine
    ok(hr == INET_E_RESOURCE_NOT_FOUND, "Unexpected hr %#lx.\n", hr);

    IXMLHttpRequest_Release(xhr);
    free_bstrs();
}

static void test_server_xhr(void)
{
    IServerXMLHTTPRequest *xhr;
    BSTR response;
    VARIANT body;
    HRESULT hr;

    /* GET request */
    xhr = create_server_xhr();

    test_server_open(xhr, "GET", xmltestA, S_OK);

    V_VT(&body) = VT_EMPTY;

    hr = IServerXMLHTTPRequest_send(xhr, body);
    if (hr == INET_E_RESOURCE_NOT_FOUND || hr == WININET_E_NAME_NOT_RESOLVED)
    {
        skip("No connection could be made with test.winehq.org\n");
        IServerXMLHTTPRequest_Release(xhr);
        return;
    }
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IServerXMLHTTPRequest_get_responseText(xhr, &response);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp_wa(response, xmltestbodyA), "got %s\n", wine_dbgstr_w(response));
    SysFreeString(response);

    IServerXMLHTTPRequest_Release(xhr);

    /* invalid host */
    xhr = create_server_xhr();

    test_server_open(xhr, "GET", "http://invalid.host.test.winehq.org/test/path", S_OK);

    V_VT(&body) = VT_EMPTY;
    hr = IServerXMLHTTPRequest_send(xhr, body);
    todo_wine
    ok(hr == WININET_E_NAME_NOT_RESOLVED, "Unexpected hr %#lx.\n", hr);

    IServerXMLHTTPRequest_Release(xhr);
    free_bstrs();
}

static void test_safe_httpreq(void)
{
    IXMLHttpRequest *xhr;

    xhr = create_xhr();

    set_safety_opt((IUnknown*)xhr, INTERFACESAFE_FOR_UNTRUSTED_DATA, -1);
    set_xhr_site(xhr);

    /* different scheme */
    test_open(xhr, "GET", "https://test.winehq.org/tests/hello.html", E_ACCESSDENIED);

    /* different host */
    test_open(xhr, "GET", "http://tests.winehq.org/tests/hello.html", E_ACCESSDENIED);
    test_open(xhr, "GET", "http://www.test.winehq.org/tests/hello.html", E_ACCESSDENIED);

    IXMLHttpRequest_Release(xhr);
    free_bstrs();
}

static void test_supporterrorinfo(void)
{
    HRESULT hr;
    IXMLHttpRequest *xhr;
    IServerXMLHTTPRequest *server_xhr;
    ISupportErrorInfo *errorinfo, *errorinfo2;

    xhr = create_xhr();

    EXPECT_REF(xhr, 1);
    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_ISupportErrorInfo, (void **)&errorinfo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(xhr, 2);

    hr = IXMLHttpRequest_QueryInterface(xhr, &IID_ISupportErrorInfo, (void **)&errorinfo2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(errorinfo == errorinfo2, "Unexpected error info instance.\n");
    EXPECT_REF(xhr, 3);

    ISupportErrorInfo_Release(errorinfo2);
    ISupportErrorInfo_Release(errorinfo);

    IXMLHttpRequest_Release(xhr);

    /* ServerXMLHTTP */
    server_xhr = create_server_xhr();

    EXPECT_REF(server_xhr, 1);
    hr = IServerXMLHTTPRequest_QueryInterface(server_xhr, &IID_ISupportErrorInfo, (void **)&errorinfo);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(server_xhr, 2);

    hr = IServerXMLHTTPRequest_QueryInterface(server_xhr, &IID_ISupportErrorInfo, (void **)&errorinfo2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(errorinfo == errorinfo2, "Unexpected error info instance.\n");
    EXPECT_REF(server_xhr, 3);

    ISupportErrorInfo_Release(errorinfo2);
    ISupportErrorInfo_Release(errorinfo);

    IServerXMLHTTPRequest_Release(server_xhr);
}

START_TEST(httpreq)
{
    IXMLHttpRequest *xhr;

    CoInitialize(NULL);

    if (!(xhr = create_xhr()))
    {
        win_skip("IXMLHTTPRequest is not available\n");
        CoUninitialize();
        return;
    }

    IXMLHttpRequest_Release(xhr);

    test_XMLHTTP();
    test_server_xhr();
    test_safe_httpreq();
    test_supporterrorinfo();

    CoUninitialize();
}
