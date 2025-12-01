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
#include "msxml2.h"

#include "wine/test.h"

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

static const WCHAR xsd_schema2_uri[] = L"x-schema:test2.xsd";
static const WCHAR xsd_schema2_xml[] =
L"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test2.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";

static const WCHAR xsd_schema3_uri[] = L"x-schema:test3.xsd";
static const WCHAR xsd_schema3_xml[] =
L"<?xml version='1.0'?>"
"<schema xmlns='http://www.w3.org/2001/XMLSchema'"
"            targetNamespace='x-schema:test3.xsd'>"
"   <element name='root'>"
"       <complexType>"
"           <sequence maxOccurs='unbounded'>"
"               <any/>"
"           </sequence>"
"       </complexType>"
"   </element>"
"</schema>";

static const WCHAR xdr_schema1_uri[] = L"x-schema:test1.xdr";
static const WCHAR xdr_schema1_xml[] =
L"<?xml version='1.0'?>"
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'"
"        xmlns:dt='urn:schemas-microsoft-com:datatypes'"
"        name='test1.xdr'>"
"   <ElementType name='x' dt:type='boolean'/>"
"   <ElementType name='y'>"
"       <datatype dt:type='int'/>"
"   </ElementType>"
"   <ElementType name='z'/>"
"   <ElementType name='root' content='eltOnly' model='open' order='seq'>"
"       <element type='x'/>"
"       <element type='y'/>"
"       <element type='z'/>"
"   </ElementType>"
"</Schema>";

static const WCHAR xdr_schema2_uri[] = L"x-schema:test2.xdr";
static const WCHAR xdr_schema2_xml[] =
L"<?xml version='1.0'?>"
"<Schema xmlns='urn:schemas-microsoft-com:xml-data'"
"        xmlns:dt='urn:schemas-microsoft-com:datatypes'"
"        name='test2.xdr'>"
"   <ElementType name='x' dt:type='bin.base64'/>"
"   <ElementType name='y' dt:type='uuid'/>"
"   <ElementType name='z'/>"
"   <ElementType name='root' content='eltOnly' model='closed' order='one'>"
"       <element type='x'/>"
"       <element type='y'/>"
"       <element type='z'/>"
"   </ElementType>"
"</Schema>";

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

static VARIANT _variantdoc_(void* doc)
{
    VARIANT v;
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)doc;
    return v;
}

static IXMLDOMDocument2 *create_document(void)
{
    IXMLDOMDocument2 *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&obj);
    ok(hr == S_OK, "Failed to create a document object, hr %#lx.\n", hr);

    return obj;
}

static void *create_cache(REFIID riid)
{
    void *obj = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLSchemaCache40, NULL, CLSCTX_INPROC_SERVER, riid, &obj);
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
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection2_get(cache, _bstr_(L"uri"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IXMLDOMSchemaCollection2_Release(cache);
    free_bstrs();
}

static void test_remove(void)
{
    IXMLDOMSchemaCollection2 *cache;
    IXMLDOMDocument2 *doc;
    VARIANT_BOOL b;
    HRESULT hr;
    VARIANT v;
    LONG len;

    /* ::remove() works for version 4 */
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

    hr = IXMLDOMSchemaCollection2_remove(cache, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMSchemaCollection2_remove(cache, _bstr_(L"invaliduri"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMSchemaCollection2_get_length(cache, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 1, "Unexpected length %ld.\n", len);

    hr = IXMLDOMSchemaCollection2_remove(cache, _bstr_(xsd_schema1_uri));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = IXMLDOMSchemaCollection2_get_length(cache, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "Unexpected length %ld.\n", len);

    IXMLDOMDocument2_Release(doc);
    IXMLDOMSchemaCollection2_Release(cache);

    free_bstrs();
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

static void test_collection_content(void)
{
    IXMLDOMDocument2 *schema1, *schema2, *schema3, *schema4, *schema5;
    IXMLDOMSchemaCollection *cache;
    BSTR content[5] = { 0 };
    VARIANT_BOOL b;
    LONG length;
    HRESULT hr;
    BSTR bstr;
    int i, j;

    schema1 = create_document();
    schema2 = create_document();
    schema3 = create_document();
    schema4 = create_document();
    schema5 = create_document();
    cache = create_cache(&IID_IXMLDOMSchemaCollection);

    hr = IXMLDOMDocument2_loadXML(schema1, _bstr_(xdr_schema1_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    hr = IXMLDOMDocument2_loadXML(schema2, _bstr_(xdr_schema2_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    hr = IXMLDOMDocument2_loadXML(schema3, _bstr_(xsd_schema1_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    hr = IXMLDOMDocument2_loadXML(schema4, _bstr_(xsd_schema2_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");
    hr = IXMLDOMDocument2_loadXML(schema5, _bstr_(xsd_schema3_xml), &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "failed to load XML\n");

    /* combining XDR and XSD schemas in the same cache is fine */
    hr = IXMLDOMSchemaCollection_add(cache, _bstr_(xdr_schema1_uri), _variantdoc_(schema1));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMSchemaCollection_add(cache, _bstr_(xdr_schema2_uri), _variantdoc_(schema2));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMSchemaCollection_add(cache, _bstr_(xsd_schema1_uri), _variantdoc_(schema3));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMSchemaCollection_add(cache, _bstr_(xsd_schema2_uri), _variantdoc_(schema4));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMSchemaCollection_add(cache, _bstr_(xsd_schema3_uri), _variantdoc_(schema5));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    length = -1;
    hr = IXMLDOMSchemaCollection_get_length(cache, &length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(length == 5, "Unexpected length %ld.\n", length);

    for (i = 0; i < 5; ++i)
    {
        bstr = NULL;
        hr = IXMLDOMSchemaCollection_get_namespaceURI(cache, i, &bstr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(bstr != NULL && *bstr, "expected non-empty string\n");

        for (j = 0; j < i; ++j)
            ok(wcscmp(content[j], bstr), "got duplicate entry\n");
        content[i] = bstr;
    }

    for (i = 0; i < 5; ++i)
    {
        SysFreeString(content[i]);
        content[i] = NULL;
    }

    IXMLDOMDocument2_Release(schema1);
    IXMLDOMDocument2_Release(schema2);
    IXMLDOMDocument2_Release(schema3);
    IXMLDOMDocument2_Release(schema4);
    IXMLDOMDocument2_Release(schema5);

    IXMLDOMSchemaCollection_Release(cache);
    free_bstrs();
}

START_TEST(schema)
{
    IUnknown *obj;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&obj);
    if (FAILED(hr))
    {
        win_skip("DOMDocument40 is not supported.\n");
        CoUninitialize();
        return;
    }
    IUnknown_Release(obj);

    test_regex();
    test_get();
    test_remove();
    test_validate_on_load();
    test_collection_content();

    CoUninitialize();
}
