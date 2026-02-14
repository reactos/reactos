/*
 * XML test
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2007-2008 Alistair Leslie-Hughes
 * Copyright 2010-2011 Adam Martinson for CodeWeavers
 * Copyright 2010-2013 Nikolay Sivov for CodeWeavers
 * Copyright 2023 Daniel Lehman
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

#include "initguid.h"
#include "msxml2.h"

#include "wine/test.h"

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

struct attrtest_t {
    const WCHAR *name;
    const WCHAR *uri;
    const WCHAR *prefix;
    const WCHAR *href;
};

static struct attrtest_t attrtests[] = {
    { L"xmlns", L"http://www.w3.org/2000/xmlns/", L"xmlns", L"xmlns" },
    { L"xmlns", L"nondefaulturi", L"xmlns", L"xmlns" },
    { L"c", L"http://www.w3.org/2000/xmlns/", NULL, L"http://www.w3.org/2000/xmlns/" },
    { L"c", L"nsref1", NULL, L"nsref1" },
    { L"ns:c", L"nsref1", L"ns", L"nsref1" },
    { L"xmlns:c", L"http://www.w3.org/2000/xmlns/", L"xmlns", L"" },
    { L"xmlns:c", L"nondefaulturi", L"xmlns", L"" },
    { 0 }
};

/* see dlls/msxml[36]/tests/domdoc.c */
static void test_create_attribute(void)
{
    struct attrtest_t *ptr = attrtests;
    IXMLDOMElement *el;
    IXMLDOMDocument2 *doc;
    IXMLDOMNode *node;
    VARIANT var;
    HRESULT hr;
    int i = 0;
    BSTR str;

    hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
    ok(hr == S_OK, "Failed to create DOMDocument40, hr %#lx.\n", hr);

    while (ptr->name)
    {
        V_VT(&var) = VT_I1;
        V_I1(&var) = NODE_ATTRIBUTE;
        hr = IXMLDOMDocument2_createNode(doc, var, _bstr_(ptr->name), _bstr_(ptr->uri), &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        str = NULL;
        hr = IXMLDOMNode_get_prefix(node, &str);
        if (ptr->prefix)
        {
            /* MSXML4 can report different results with different service packs */
            ok(hr == S_OK || broken(hr == S_FALSE), "Failed to get prefix, hr %#lx.\n", hr);
            ok(!lstrcmpW(str, _bstr_(ptr->prefix)) || broken(!str), "got %s\n", wine_dbgstr_w(str));
        }
        else
        {
            ok(hr == S_FALSE, "%d: unexpected hr %#lx\n", i, hr);
            ok(str == NULL, "%d: got prefix %s\n", i, wine_dbgstr_w(str));
        }
        SysFreeString(str);

        str = NULL;
        hr = IXMLDOMNode_get_namespaceURI(node, &str);
        ok(hr == S_OK, "%d: unexpected hr %#lx\n", i, hr);
        ok(!lstrcmpW(str, _bstr_(ptr->href)), "%d: got uri %s, expected %s\n",
            i, wine_dbgstr_w(str), wine_dbgstr_w(ptr->href));
        SysFreeString(str);

        IXMLDOMNode_Release(node);
        free_bstrs();

        i++;
        ptr++;
    }

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ELEMENT;
    hr = IXMLDOMDocument2_createNode(doc, var, _bstr_(L"e"), NULL, &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&el);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMNode_Release(node);

    V_VT(&var) = VT_I1;
    V_I1(&var) = NODE_ATTRIBUTE;
    hr = IXMLDOMDocument2_createNode(doc, var, _bstr_(L"xmlns:a"),
        _bstr_(L"http://www.w3.org/2000/xmlns/"), &node);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_setAttributeNode(el, (IXMLDOMAttribute*)node, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* for some reason default namespace uri is not reported */
    hr = IXMLDOMNode_get_namespaceURI(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L""), "got uri %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IXMLDOMNode_Release(node);
    IXMLDOMElement_Release(el);
    IXMLDOMDocument2_Release(doc);
    free_bstrs();
}

/* see dlls/msxml[36]/tests/domdoc.c */
static void test_namespaces_as_attributes(void)
{
    struct test
    {
        const WCHAR *xml;
        int explen;
        const WCHAR *names[3];
        const WCHAR *prefixes[3];
        const WCHAR *basenames[3];
        const WCHAR *uris[3];
        const WCHAR *texts[3];
        const WCHAR *xmls[3];
    };
    static const struct test tests[] =
    {
        {
            L"<a ns:b=\"b attr\" d=\"d attr\" xmlns:ns=\"nshref\" />", 3,
            { L"ns:b",   L"d",      L"xmlns:ns" }, /* nodeName */
            { L"ns",     NULL,      L"xmlns" },    /* prefix */
            { L"b",      L"d",      L"ns" },       /* baseName */
            { L"nshref", NULL,      L"" },         /* namespaceURI */
            { L"b attr", L"d attr", L"nshref" },   /* text */
            { L"ns:b=\"b attr\"", L"d=\"d attr\"", L"xmlns:ns=\"nshref\"" }, /* xml */
        },
        /* property only */
        {
            L"<a d=\"d attr\" />", 1,
            { L"d" },                   /* nodeName */
            { NULL },                   /* prefix */
            { L"d" },                   /* baseName */
            { NULL },                   /* namespaceURI */
            { L"d attr" },              /* text */
            { L"d=\"d attr\"" },        /* xml */
        },
        /* namespace only */
        {
            L"<a xmlns:ns=\"nshref\" />", 1,
            { L"xmlns:ns" },            /* nodeName */
            { L"xmlns" },               /* prefix */
            { L"ns" },                  /* baseName */
            { L"" },                    /* namespaceURI */
            { L"nshref" },              /* text */
            { L"xmlns:ns=\"nshref\"" }, /* xml */
        },
        /* default namespace */
        {
            L"<a xmlns=\"nshref\" />", 1,
            { L"xmlns" },               /* nodeName */
            { L"xmlns" },               /* prefix */
            { L"" },                    /* baseName */
            { L"" },                    /* namespaceURI */
            { L"nshref" },              /* text */
            { L"xmlns=\"nshref\"" },    /* xml */
        },
        /* no properties or namespaces */
        {
            L"<a />", 0,
        },

        { NULL }
    };
    const struct test *test;
    IXMLDOMNamedNodeMap *map;
    IXMLDOMNode *node, *item;
    IXMLDOMDocument2 *doc;
    VARIANT_BOOL b;
    LONG len, i;
    HRESULT hr;
    BSTR str;

    test = tests;
    while (test->xml)
    {
        hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMDocument2_loadXML(doc, _bstr_(test->xml), &b);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        node = NULL;
        hr = IXMLDOMDocument2_get_firstChild(doc, &node);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IXMLDOMNode_get_attributes(node, &map);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        len = -1;
        hr = IXMLDOMNamedNodeMap_get_length(map, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len == test->explen, "got %ld\n", len);

        item = NULL;
        hr = IXMLDOMNamedNodeMap_get_item(map, test->explen+1, &item);
        ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);
        ok(!item, "Item should be NULL\n");

        for (i = 0; i < len; i++)
        {
            item = NULL;
            hr = IXMLDOMNamedNodeMap_get_item(map, i, &item);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            str = NULL;
            hr = IXMLDOMNode_get_nodeName(item, &str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->names[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            str = NULL;
            hr = IXMLDOMNode_get_prefix(item, &str);
            if (test->prefixes[i])
            {
                /* MSXML4 can report different results with different service packs */
                ok(hr == S_OK || broken(hr == S_FALSE), "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(str, test->prefixes[i]) || broken(!str),
                   "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);
            }
            else
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

            str = NULL;
            hr = IXMLDOMNode_get_baseName(item, &str);
            /* MSXML4 can report different results with different service packs */
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->basenames[i]) || broken(!lstrcmpW(str, L"xmlns")),
                "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            str = NULL;
            hr = IXMLDOMNode_get_namespaceURI(item, &str);
            if (test->uris[i])
            {
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                if (test->prefixes[i] && !lstrcmpW(test->prefixes[i], L"xmlns"))
                    ok(!lstrcmpW(str, L""), "got %s\n", wine_dbgstr_w(str));
                else
                    ok(!lstrcmpW(str, test->uris[i]), "got %s\n", wine_dbgstr_w(str));
                SysFreeString(str);
            }
            else
                ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr );

            str = NULL;
            hr = IXMLDOMNode_get_text(item, &str);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->texts[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            str = NULL;
            hr = IXMLDOMNode_get_xml(item, &str);
            ok(SUCCEEDED(hr), "Failed to get node xml, hr %#lx.\n", hr);
            ok(!lstrcmpW(str, test->xmls[i]), "got %s\n", wine_dbgstr_w(str));
            SysFreeString(str);

            IXMLDOMNode_Release(item);
        }

        IXMLDOMNamedNodeMap_Release(map);
        IXMLDOMNode_Release(node);
        IXMLDOMDocument2_Release(doc);

        test++;
    }
    free_bstrs();
}

START_TEST(domdoc)
{
    HRESULT hr;
    IXMLDOMDocument2 *doc;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    hr = CoCreateInstance(&CLSID_DOMDocument40, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (void **)&doc);
    if (hr != S_OK)
    {
        win_skip("class &CLSID_DOMDocument40 not supported\n");
        return;
    }
    IXMLDOMDocument2_Release(doc);

    test_namespaces_as_attributes();
    test_create_attribute();

    CoUninitialize();
}
