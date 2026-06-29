/*
 * Schema test
 *
 * Copyright 2007 Huw Davies
 * Copyright 2010 Adam Martinson for CodeWeavers
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
#include <assert.h>
#define COBJMACROS

#include "ole2.h"
#include "msxml6.h"
#include "msxml6did.h"
#include "dispex.h"

#include "wine/test.h"

#include "initguid.h"

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static const WCHAR xsd_schema1_uri[] = L"x-schema:test1.xsd";
static const WCHAR xsd_schema1_xml[] =
L"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test1.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";

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

static BSTR alloced_bstrs[256];
static int alloced_bstrs_count;

static BSTR _bstr_(const WCHAR *str)
{
    assert(alloced_bstrs_count < ARRAY_SIZE(alloced_bstrs));
    alloced_bstrs[alloced_bstrs_count] = SysAllocString(str);
    return alloced_bstrs[alloced_bstrs_count++];
}

static void free_bstrs(void)
{
    int i;
    for (i = 0; i < alloced_bstrs_count; i++)
        SysFreeString(alloced_bstrs[i]);
    alloced_bstrs_count = 0;
}

static IXMLDOMDocument2 *create_document(void)
{
    IXMLDOMDocument2 *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&obj);
    ok(hr == S_OK, "Failed to create a document object, hr %#lx.\n", hr);

    return obj;
}

static void *create_cache(REFIID riid)
{
    void *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLSchemaCache60, NULL, CLSCTX_INPROC_SERVER, riid, &obj);
    ok(hr == S_OK, "Failed to create a document object, hr %#lx.\n", hr);

    return obj;
}

static HRESULT validate_regex_document(IXMLDOMDocument2 *doc, IXMLDOMDocument2 *schema, IXMLDOMSchemaCollection* cache,
    const WCHAR *regex, const WCHAR *input)
{
    static const WCHAR regex_doc[] =
L""
"<?xml version='1.0'?>"
"<root xmlns='urn:test'>%s</root>";

    static const WCHAR regex_schema[] =
L"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='urn:test'>"
"    <element name='root'>"
"        <simpleType>"
"            <restriction base='string'>"
"                <pattern value='%s'/>"
"            </restriction>"
"        </simpleType>"
"    </element>"
"</schema>";

    WCHAR buffer[1024];
    IXMLDOMParseError* err;
    BSTR namespace, bstr;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;

    VariantInit(&v);

    swprintf(buffer, ARRAY_SIZE(buffer), regex_doc, input);
    bstr = SysAllocString(buffer);
    hr = IXMLDOMDocument2_loadXML(doc, bstr, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    SysFreeString(bstr);

    swprintf(buffer, ARRAY_SIZE(buffer), regex_schema, regex);
    bstr = SysAllocString(buffer);
    hr = IXMLDOMDocument2_loadXML(schema, bstr, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    SysFreeString(bstr);

    /* add the schema to the cache */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMDocument2_QueryInterface(schema, &IID_IDispatch, (void**)&V_DISPATCH(&v));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    namespace = SysAllocString(L"urn:test");
    hr = IXMLDOMSchemaCollection_add(cache, namespace, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(namespace);
    VariantClear(&v);
/*
    if (FAILED(hr))
        return hr;
*/
    /* associate the cache to the doc */
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;
    hr = IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IDispatch, (void**)&V_DISPATCH(&v));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_DISPATCH(&v) != NULL, "failed to get IDispatch interface\n");
    hr = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    VariantClear(&v);

    /* validate the doc
     * only declared elements in the declared order
     * this is fine */
    err = NULL;
    bstr = NULL;
    hr = IXMLDOMDocument2_validate(doc, &err);
    ok(err != NULL, "domdoc_validate() should always set err\n");
    if (IXMLDOMParseError_get_reason(err, &bstr) != S_FALSE)
        trace("got error: %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);
    IXMLDOMParseError_Release(err);

    return hr;
}

static void test_regex(void)
{
    static const struct regex_test
    {
        const WCHAR *regex;
        const WCHAR *input;
    }
    tests[] =
    {
        { L"\\!", L"!" },
        { L"\\\"", L"\"" },
        { L"\\#", L"#" },
        { L"\\$", L"$" },
        { L"\\%", L"%" },
        { L"\\,", L"," },
        { L"\\/", L"/" },
        { L"\\:", L":" },
        { L"\\;", L";" },
        { L"\\=", L"=" },
        { L"\\>", L">" },
        { L"\\@", L"@" },
        { L"\\`", L"`" },
        { L"\\~", L"~" },
        { L"\\uCAFE", L"\xCAFE" },
        /* non-BMP character in surrogate pairs: */
        { L"\\uD83D\\uDE00", L"\xD83D\xDE00" },
        /* "x{,2}" is non-standard and only works on libxml2 <= v2.9.10 */
        { L"x{0,2}", L"x" }
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const struct regex_test *test = &tests[i];
        IXMLDOMDocument2 *doc, *schema;
        IXMLDOMSchemaCollection *cache;
        HRESULT hr;

        winetest_push_context("Test %s", wine_dbgstr_w(test->regex));

        doc = create_document();
        schema = create_document();
        cache = create_cache(&IID_IXMLDOMSchemaCollection);

        hr = validate_regex_document(doc, schema, cache, tests->regex, tests->input);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (doc)
            IXMLDOMDocument2_Release(doc);
        if (schema)
            IXMLDOMDocument2_Release(schema);
        if (cache)
            IXMLDOMSchemaCollection_Release(cache);

        winetest_pop_context();
    }
}

static void test_get(void)
{
    IXMLDOMSchemaCollection2 *cache;
    IXMLDOMNode *node;
    HRESULT hr;

    cache = create_cache(&IID_IXMLDOMSchemaCollection2);

    hr = IXMLDOMSchemaCollection2_get(cache, NULL, NULL);
    ok(hr == E_NOTIMPL || hr == E_POINTER /* win8 */, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection2_get(cache, _bstr_(L"uri"), &node);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    IXMLDOMSchemaCollection2_Release(cache);
    free_bstrs();
}

static void test_ifaces(void)
{
    IXMLDOMSchemaCollection2 *cache;
    IUnknown *unk;
    HRESULT hr;

    cache = create_cache(&IID_IXMLDOMSchemaCollection2);

    /* CLSID_XMLSchemaCache60 is returned as an interface (the same as IXMLDOMSchemaCollection2). */
    hr = IXMLDOMSchemaCollection2_QueryInterface(cache, &CLSID_XMLSchemaCache60, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown *)cache, "Unexpected pointer %p.\n", unk);
    IUnknown_Release(unk);

    check_interface(cache, &IID_IXMLDOMSchemaCollection, TRUE);
    check_interface(cache, &IID_IXMLDOMSchemaCollection2, TRUE);
    check_interface(cache, &IID_IDispatch, TRUE);
    check_interface(cache, &IID_IDispatchEx, TRUE);

    IXMLDOMSchemaCollection2_Release(cache);
}

static void test_remove(void)
{
    IXMLDOMSchemaCollection2 *cache;
    IXMLDOMDocument2 *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;
    LONG len;

    cache = create_cache(&IID_IXMLDOMSchemaCollection2);

    doc = create_document();
    ok(doc != NULL, "got %p\n", doc);

    hr = IXMLDOMDocument2_loadXML(doc, _bstr_(xsd_schema1_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)doc;
    hr = IXMLDOMSchemaCollection2_add(cache, _bstr_(xsd_schema1_uri), v);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMSchemaCollection2_get_length(cache, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 1, "Unexpected length %ld.\n", len);

    /* ::remove() is a stub for version 6 */
    hr = IXMLDOMSchemaCollection2_remove(cache, NULL);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection2_remove(cache, _bstr_(L"invaliduri"));
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection2_remove(cache, _bstr_(xsd_schema1_uri));
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMSchemaCollection2_get_length(cache, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 1, "Unexpected length %ld.\n", len);

    IXMLDOMDocument2_Release(doc);
    IXMLDOMSchemaCollection2_Release(cache);
    free_bstrs();
}

static void test_obj_dispex(IUnknown *obj)
{
    DISPID dispid = DISPID_SAX_XMLREADER_GETFEATURE;
    IDispatchEx *dispex;
    IUnknown *unk;
    DWORD props;
    UINT ticnt;
    HRESULT hr;
    BSTR name;

    hr = IUnknown_QueryInterface(obj, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    if (FAILED(hr)) return;

    ticnt = 0;
    hr = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(ticnt == 1, "ticnt=%u\n", ticnt);

    name = SysAllocString(L"*");
    hr = IDispatchEx_DeleteMemberByName(dispex, name, fdexNameCaseSensitive);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    hr = IDispatchEx_DeleteMemberByDispID(dispex, dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    props = 0;
    hr = IDispatchEx_GetMemberProperties(dispex, dispid, grfdexPropCanAll, &props);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(props == 0, "expected 0 got %ld\n", props);

    hr = IDispatchEx_GetMemberName(dispex, dispid, &name);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) SysFreeString(name);

    hr = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, DISPID_XMLDOM_SCHEMACOLLECTION_ADD, &dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    unk = (IUnknown*)0xdeadbeef;
    hr = IDispatchEx_GetNameSpaceParent(dispex, &unk);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown*)0xdeadbeef, "got %p\n", unk);

    name = SysAllocString(L"testprop");
    hr = IDispatchEx_GetDispID(dispex, name, fdexNameEnsure, &dispid);
    ok(hr == DISP_E_UNKNOWNNAME, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    IDispatchEx_Release(dispex);
}

static void test_dispex(void)
{
    IXMLDOMSchemaCollection *cache;
    IUnknown *unk;
    HRESULT hr;

    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    hr = IXMLDOMSchemaCollection_QueryInterface(cache, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    test_obj_dispex(unk);
    IUnknown_Release(unk);

    IXMLDOMSchemaCollection_Release(cache);
}

static void test_validate_on_load(void)
{
    IXMLDOMSchemaCollection2 *cache;
    VARIANT_BOOL b;
    HRESULT hr;

    cache = create_cache(&IID_IXMLDOMSchemaCollection2);

    hr = IXMLDOMSchemaCollection2_get_validateOnLoad(cache, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_FALSE;
    hr = IXMLDOMSchemaCollection2_get_validateOnLoad(cache, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    IXMLDOMSchemaCollection2_Release(cache);
}

START_TEST(schema)
{
    IUnknown *obj;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&obj);
    if (FAILED(hr))
    {
        win_skip("DOMDocument60 is not supported.\n");
        CoUninitialize();
        return;
    }
    IUnknown_Release(obj);

    test_regex();
    test_get();
    test_ifaces();
    test_remove();
    test_dispex();
    test_validate_on_load();

    CoUninitialize();
}
