/*
 * Schema test
 *
 * Copyright 2007 Huw Davies
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
#define COBJMACROS

#include "initguid.h"
#include "windows.h"
#include "ole2.h"
#include "xmldom.h"
#include "msxml2.h"
#include "dispex.h"

#include "wine/test.h"

static const WCHAR schema_uri[] = {'x','-','s','c','h','e','m','a',':','t','e','s','t','.','x','m','l',0};

static const WCHAR schema_xml[] = {
    '<','S','c','h','e','m','a',' ','x','m','l','n','s','=','\"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','x','m','l','-','d','a','t','a','\"','\n',
    'x','m','l','n','s',':','d','t','=','\"','u','r','n',':','s','c','h','e','m','a','s','-','m','i','c','r','o','s','o','f','t','-','c','o','m',':','d','a','t','a','t','y','p','e','s','\"','>','\n',
    '<','/','S','c','h','e','m','a','>','\n',0
};

static void test_schema_refs(void)
{
    IXMLDOMDocument2 *doc;
    IXMLDOMSchemaCollection *schema;
    HRESULT r;
    LONG ref;
    VARIANT v;
    VARIANT_BOOL b;
    BSTR str;

    r = CoCreateInstance( &CLSID_DOMDocument, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument2, (LPVOID*)&doc );
    if( r != S_OK )
        return;

    r = CoCreateInstance( &CLSID_XMLSchemaCache, NULL,
        CLSCTX_INPROC_SERVER, &IID_IXMLDOMSchemaCollection, (LPVOID*)&schema );
    if( r != S_OK )
    {
        IXMLDOMDocument2_Release(doc);
        return;
    }

    str = SysAllocString(schema_xml);
    r = IXMLDOMDocument2_loadXML(doc, str, &b);
    ok(r == S_OK, "ret %08x\n", r);
    ok(b == VARIANT_TRUE, "b %04x\n", b);
    SysFreeString(str);

    ref = IXMLDOMDocument2_AddRef(doc);
    ok(ref == 2, "ref %d\n", ref);
    VariantInit(&v);
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)doc;

    str = SysAllocString(schema_uri);
    r = IXMLDOMSchemaCollection_add(schema, str, v);
    ok(r == S_OK, "ret %08x\n", r);

    /* IXMLDOMSchemaCollection_add doesn't add a ref on doc */
    ref = IXMLDOMDocument2_AddRef(doc);
    ok(ref == 3, "ref %d\n", ref);
    IXMLDOMDocument2_Release(doc);

    SysFreeString(str);
    VariantClear(&v);

    V_VT(&v) = VT_INT;
    r = IXMLDOMDocument2_get_schemas(doc, &v);
    ok(r == S_FALSE, "ret %08x\n", r);
    ok(V_VT(&v) == VT_NULL, "vt %x\n", V_VT(&v));

    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 2, "ref %d\n", ref);
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)schema;

    /* check that putref_schemas takes a ref */
    r = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(r == S_OK, "ret %08x\n", r);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 4, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);
    VariantClear(&v);

    /* refs now 2 */
    V_VT(&v) = VT_INT;
    /* check that get_schemas adds a ref */
    r = IXMLDOMDocument2_get_schemas(doc, &v);
    ok(r == S_OK, "ret %08x\n", r);
    ok(V_VT(&v) == VT_DISPATCH, "vt %x\n", V_VT(&v));
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 4, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);

    /* refs now 3 */
    /* get_schemas doesn't release a ref if passed VT_DISPATCH - ie it doesn't call VariantClear() */
    r = IXMLDOMDocument2_get_schemas(doc, &v);
    ok(r == S_OK, "ret %08x\n", r);
    ok(V_VT(&v) == VT_DISPATCH, "vt %x\n", V_VT(&v));
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 5, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);

    /* refs now 4 */
    /* release the two refs returned by get_schemas */
    IXMLDOMSchemaCollection_Release(schema);
    IXMLDOMSchemaCollection_Release(schema);

    /* refs now 2 */

    /* check that taking another ref on the document doesn't change the schema's ref count */
    IXMLDOMDocument2_AddRef(doc);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 3, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);
    IXMLDOMDocument2_Release(doc);


    /* refs now 2 */
    /* call putref_schema with some odd variants */
    V_VT(&v) = VT_INT;
    r = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(r == E_FAIL, "ret %08x\n", r);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 3, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);

    /* refs now 2 */
    /* calling with VT_EMPTY releases the schema */
    V_VT(&v) = VT_EMPTY;
    r = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(r == S_OK, "ret %08x\n", r);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 2, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);

    /* refs now 1 */
    /* try setting with VT_UNKNOWN */
    IXMLDOMSchemaCollection_AddRef(schema);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)schema;
    r = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(r == S_OK, "ret %08x\n", r);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 4, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);
    VariantClear(&v);

    /* refs now 2 */
    /* calling with VT_NULL releases the schema */
    V_VT(&v) = VT_NULL;
    r = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(r == S_OK, "ret %08x\n", r);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 2, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);

    /* refs now 1 */
    /* set again */
    IXMLDOMSchemaCollection_AddRef(schema);
    V_VT(&v) = VT_UNKNOWN;
    V_UNKNOWN(&v) = (IUnknown*)schema;
    r = IXMLDOMDocument2_putref_schemas(doc, v);
    ok(r == S_OK, "ret %08x\n", r);
    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 4, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);
    VariantClear(&v);

    /* refs now 2 */

    /* release the final ref on the doc which should release its ref on the schema */
    IXMLDOMDocument2_Release(doc);

    ref = IXMLDOMSchemaCollection_AddRef(schema);
    ok(ref == 2, "ref %d\n", ref);
    IXMLDOMSchemaCollection_Release(schema);
    IXMLDOMSchemaCollection_Release(schema);
}

START_TEST(schema)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    test_schema_refs();

    CoUninitialize();
}
