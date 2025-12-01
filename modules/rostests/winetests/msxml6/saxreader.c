/*
 * Copyright 2008 Piotr Caban
 * Copyright 2011 Thomas Mullaly
 * Copyright 2012 Nikolay Sivov
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
#include "msxml6.h"
#include "msxml6did.h"
#include "ocidl.h"
#include "initguid.h"
#include "dispex.h"

#include "wine/test.h"

struct class_support
{
    const GUID *clsid;
    const char *name;
    const IID *iid;
    BOOL supported;
};

static struct class_support class_support[] =
{
    { &CLSID_MXXMLWriter60, "MXXMLWriter60", &IID_IMXWriter },
    { &CLSID_SAXAttributes60, "SAXAttributes60", &IID_IMXAttributes },
    { NULL }
};

static void get_class_support_data(void)
{
    struct class_support *table = class_support;

    while (table->clsid)
    {
        IUnknown *unk;
        HRESULT hr;

        hr = CoCreateInstance(table->clsid, NULL, CLSCTX_INPROC_SERVER, table->iid, (void **)&unk);
        if (hr == S_OK) IUnknown_Release(unk);

        table->supported = hr == S_OK;
        if (hr != S_OK) win_skip("class %s not supported\n", table->name);

        table++;
    }
}

static BOOL is_class_supported(const CLSID *clsid)
{
    struct class_support *table = class_support;

    while (table->clsid)
    {
        if (table->clsid == clsid) return table->supported;
        table++;
    }
    return FALSE;
}

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
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

static LONG get_refcount(void *iface)
{
    IUnknown *unk = iface;
    LONG ref;

    ref = IUnknown_AddRef(unk);
    IUnknown_Release(unk);
    return ref-1;
}

static BSTR alloc_str_from_narrow(const char *str)
{
    int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    BSTR ret = SysAllocStringLen(NULL, len - 1);  /* NUL character added automatically */
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static BSTR alloced_bstrs[512];
static int alloced_bstrs_count;

static BSTR _bstr_(const char *str)
{
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

static HRESULT WINAPI isaxattributes_QueryInterface(
        ISAXAttributes* iface,
        REFIID riid,
        void **ppvObject)
{
    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISAXAttributes))
    {
        *ppvObject = iface;
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI isaxattributes_AddRef(ISAXAttributes* iface)
{
    return 2;
}

static ULONG WINAPI isaxattributes_Release(ISAXAttributes* iface)
{
    return 1;
}

static HRESULT WINAPI isaxattributes_getLength(ISAXAttributes* iface, int *length)
{
    *length = 3;
    return S_OK;
}

static HRESULT WINAPI isaxattributes_getURI(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pUrl,
    int *pUriSize)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getLocalName(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pLocalName,
    int *pLocalNameLength)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getQName(
    ISAXAttributes* iface,
    int index,
    const WCHAR **QName,
    int *QNameLength)
{
    static const WCHAR attrqnamesW[][15] = {L"a:attr1junk",
                                            L"attr2junk",
                                            L"attr3"};
    static const int attrqnamelen[] = {7, 5, 5};

    ok(index >= 0 && index <= 2, "invalid index received %d\n", index);

    if (index >= 0 && index <= 2) {
        *QName = attrqnamesW[index];
        *QNameLength = attrqnamelen[index];
    } else {
        *QName = NULL;
        *QNameLength = 0;
    }

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getName(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR **pUri,
    int * pUriLength,
    const WCHAR ** pLocalName,
    int * pLocalNameSize,
    const WCHAR ** pQName,
    int * pQNameLength)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getIndexFromName(
    ISAXAttributes* iface,
    const WCHAR * pUri,
    int cUriLength,
    const WCHAR * pLocalName,
    int cocalNameLength,
    int * index)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getIndexFromQName(
    ISAXAttributes* iface,
    const WCHAR * pQName,
    int nQNameLength,
    int * index)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getType(
    ISAXAttributes* iface,
    int nIndex,
    const WCHAR ** pType,
    int * pTypeLength)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromName(
    ISAXAttributes* iface,
    const WCHAR * pUri,
    int nUri,
    const WCHAR * pLocalName,
    int nLocalName,
    const WCHAR ** pType,
    int * nType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromQName(
    ISAXAttributes* iface,
    const WCHAR * pQName,
    int nQName,
    const WCHAR ** pType,
    int * nType)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValue(ISAXAttributes* iface, int index,
    const WCHAR **value, int *nValue)
{
    static const WCHAR attrvaluesW[][10] = {L"a1junk",
                                            L"a2junk",
                                            L"<&\">'"};
    static const int attrvalueslen[] = {2, 2, 5};

    ok(index >= 0 && index <= 2, "invalid index received %d\n", index);

    if (index >= 0 && index <= 2) {
        *value = attrvaluesW[index];
        *nValue = attrvalueslen[index];
    } else {
        *value = NULL;
        *nValue = 0;
    }

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getValueFromName(
    ISAXAttributes* iface,
    const WCHAR * pUri,
    int nUri,
    const WCHAR * pLocalName,
    int nLocalName,
    const WCHAR ** pValue,
    int * nValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValueFromQName(
    ISAXAttributes* iface,
    const WCHAR * pQName,
    int nQName,
    const WCHAR ** pValue,
    int * nValue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const ISAXAttributesVtbl SAXAttributesVtbl =
{
    isaxattributes_QueryInterface,
    isaxattributes_AddRef,
    isaxattributes_Release,
    isaxattributes_getLength,
    isaxattributes_getURI,
    isaxattributes_getLocalName,
    isaxattributes_getQName,
    isaxattributes_getName,
    isaxattributes_getIndexFromName,
    isaxattributes_getIndexFromQName,
    isaxattributes_getType,
    isaxattributes_getTypeFromName,
    isaxattributes_getTypeFromQName,
    isaxattributes_getValue,
    isaxattributes_getValueFromName,
    isaxattributes_getValueFromQName
};

static ISAXAttributes saxattributes = { &SAXAttributesVtbl };

static void test_mxwriter_handlers(void)
{
    IMXWriter *writer;
    HRESULT hr;
    int i;

    static const IID *riids[] =
    {
        &IID_ISAXContentHandler,
        &IID_ISAXLexicalHandler,
        &IID_ISAXDeclHandler,
        &IID_ISAXDTDHandler,
        &IID_ISAXErrorHandler,
        &IID_IVBSAXDeclHandler,
        &IID_IVBSAXLexicalHandler,
        &IID_IVBSAXContentHandler,
        &IID_IVBSAXDTDHandler,
        &IID_IVBSAXErrorHandler
    };

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    EXPECT_REF(writer, 1);

    for (i = 0; i < ARRAY_SIZE(riids); i++)
    {
        IUnknown *handler;
        IMXWriter *writer2;

        /* handler from IMXWriter */
        hr = IMXWriter_QueryInterface(writer, riids[i], (void**)&handler);
        ok(hr == S_OK, "%s, unexpected hr %#lx.\n", wine_dbgstr_guid(riids[i]), hr);
        EXPECT_REF(writer, 2);

        /* IMXWriter from a handler */
        hr = IUnknown_QueryInterface(handler, &IID_IMXWriter, (void**)&writer2);
        ok(hr == S_OK, "%s, unexpected hr %#lx.\n", wine_dbgstr_guid(riids[i]), hr);
        ok(writer2 == writer, "got %p, expected %p\n", writer2, writer);
        EXPECT_REF(writer, 3);
        IMXWriter_Release(writer2);
        IUnknown_Release(handler);
    }

    IMXWriter_Release(writer);
}

static void test_mxwriter_default_properties(void)
{
    IMXWriter *writer;
    VARIANT_BOOL b;
    BSTR encoding;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_FALSE;
    hr = IMXWriter_get_byteOrderMark(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "Unexpected value %d.\n", b);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_disableOutputEscaping(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "Unexpected value %d.\n", b);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_indent(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "Unexpected value %d.\n", b);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_omitXMLDeclaration(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "Unexpected value %d.\n", b);

    b = VARIANT_TRUE;
    hr = IMXWriter_get_standalone(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_FALSE, "Unexpected value %d.\n", b);

    hr = IMXWriter_get_encoding(writer, &encoding);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(encoding, L"UTF-16"), "Unexpected value %s.\n", wine_dbgstr_w(encoding));
    SysFreeString(encoding);

    IMXWriter_Release(writer);
}

static void test_mxwriter_properties(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT_BOOL b;
    HRESULT hr;
    BSTR str, str2;
    VARIANT dest;

    test_mxwriter_default_properties();

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_disableOutputEscaping(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_byteOrderMark(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_indent(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_omitXMLDeclaration(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_standalone(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    /* set and check */
    hr = IMXWriter_put_standalone(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    b = VARIANT_FALSE;
    hr = IMXWriter_get_standalone(writer, &b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(b == VARIANT_TRUE, "got %d\n", b);

    hr = IMXWriter_get_encoding(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"UTF-16"), "Unexpected string %s.\n", wine_dbgstr_w(str));

    str2 = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(str != str2, "expected newly allocated, got same %p\n", str);

    SysFreeString(str2);
    SysFreeString(str);

    /* put empty string */
    str = SysAllocString(L"");
    hr = IMXWriter_put_encoding(writer, str);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"UTF-16"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* invalid encoding name */
    str = SysAllocString(L"test");
    hr = IMXWriter_put_encoding(writer, str);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    /* test case sensitivity */
    hr = IMXWriter_put_encoding(writer, _bstr_("utf-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"utf-8"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IMXWriter_put_encoding(writer, _bstr_("uTf-16"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    str = (void*)0xdeadbeef;
    hr = IMXWriter_get_encoding(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"uTf-16"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* how it affects document creation */
    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<?xml version=\"1.0\" standalone=\"yes\"?>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);
    ISAXContentHandler_Release(content);

    hr = IMXWriter_get_version(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    /* default version is 'surprisingly' 1.0 */
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"1.0"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* store version string as is */
    hr = IMXWriter_put_version(writer, NULL);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_version(writer, _bstr_("1.0"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_version(writer, _bstr_(""));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!lstrcmpW(str, L"1.0"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IMXWriter_put_version(writer, _bstr_("a.b"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"a.b"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hr = IMXWriter_put_version(writer, _bstr_("2.0"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXWriter_get_version(writer, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(str, L"2.0"), "got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_flush(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    LARGE_INTEGER pos;
    ULARGE_INTEGER pos2;
    IStream *stream;
    VARIANT dest;
    HRESULT hr;
    char *buff;
    LONG ref;
    int len;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    /* detach when nothing was attached */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* attach stream */
    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(stream, 3);

    /* detach setting VT_EMPTY destination */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* flush() doesn't detach a stream */
    hr = IMXWriter_flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine EXPECT_REF(stream, 3);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "expected stream beginning\n");

    /* already started */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* flushed on endDocument() */
    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "expected stream position moved\n");

    IStream_Release(stream);

    /* auto-flush feature */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_byteOrderMark(writer, VARIANT_FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("a"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* internal buffer is flushed automatically on certain threshold */
    pos.QuadPart = 0;
    pos2.QuadPart = 1;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    len = 2048;
    buff = malloc(len + 1);
    memset(buff, 'A', len);
    buff[len] = 0;
    hr = ISAXContentHandler_characters(content, _bstr_(buff), len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 0;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart != 0, "unexpected stream beginning\n");

    hr = IMXWriter_get_output(writer, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    ref = get_refcount(stream);
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_UNKNOWN, "got vt type %d\n", V_VT(&dest));
    ok(V_UNKNOWN(&dest) == (IUnknown*)stream, "got pointer %p\n", V_UNKNOWN(&dest));
    ok(ref+1 == get_refcount(stream), "expected increased refcount\n");
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IStream_Release(stream);

    /* test char count lower than threshold */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    EXPECT_REF(stream, 1);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("a"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 1;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    memset(buff, 'A', len);
    buff[len] = 0;
    hr = ISAXContentHandler_characters(content, _bstr_(buff), len - 8);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    pos.QuadPart = 0;
    pos2.QuadPart = 1;
    hr = IStream_Seek(stream, pos, STREAM_SEEK_CUR, &pos2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(pos2.QuadPart == 0, "expected stream beginning\n");

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* test auto-flush function when stream is not set */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("a"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(buff, 'A', len);
    buff[len] = 0;
    hr = ISAXContentHandler_characters(content, _bstr_(buff), len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    len += strlen("<a>");
    ok(SysStringLen(V_BSTR(&dest)) == len, "got len=%d, expected %d\n", SysStringLen(V_BSTR(&dest)), len);
    VariantClear(&dest);

    free(buff);
    ISAXContentHandler_Release(content);
    IStream_Release(stream);
    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_startenddocument(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n", V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* now try another startDocument */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    /* and get duplicated prolog */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(_bstr_("<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"
                        "<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n"), V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    /* now with omitted declaration */
    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

enum startendtype
{
    StartElement    = 0x001,
    EndElement      = 0x010,
    StartEndElement = 0x011,
    DisableEscaping = 0x100
};

struct writer_startendelement_t
{
    enum startendtype type;
    const char *uri;
    const char *local_name;
    const char *qname;
    const char *output;
    HRESULT hr;
    ISAXAttributes *attr;
};

static const char startelement_xml[] = "<uri:local a:attr1=\"a1\" attr2=\"a2\" attr3=\"&lt;&amp;&quot;&gt;\'\">";
static const char startendelement_xml[] = "<uri:local a:attr1=\"a1\" attr2=\"a2\" attr3=\"&lt;&amp;&quot;&gt;\'\"/>";

static const struct writer_startendelement_t writer_startendelement[] =
{
    { StartElement, NULL, NULL, NULL, "<>", S_OK },
    { StartElement, "uri", NULL, NULL, "<>", S_OK },
    { StartElement, NULL, "local", NULL, "<>", S_OK },
    { StartElement, NULL, NULL, "qname", "<qname>", S_OK },
    { StartElement, "uri", "local", "qname", "<qname>", S_OK },
    { StartElement, "uri", "local", NULL, "<>", S_OK },
    { StartElement, "uri", "local", "uri:local", "<uri:local>", S_OK },
    { StartElement, "uri", "local", "uri:local2", "<uri:local2>", S_OK },
    { EndElement, NULL, NULL, NULL, "</>", S_OK },
    { EndElement, "uri", NULL, NULL, "</>", S_OK },
    { EndElement, NULL, "local", NULL, "</>", S_OK },
    { EndElement, NULL, NULL, "qname", "</qname>", S_OK },
    { EndElement, "uri", "local", "qname", "</qname>", S_OK },
    { EndElement, "uri", "local", NULL, "</>", S_OK },
    { EndElement, "uri", "local", "uri:local", "</uri:local>", S_OK },
    { EndElement, "uri", "local", "uri:local2", "</uri:local2>", S_OK },
    { StartElement, "uri", "local", "uri:local", startelement_xml, S_OK, &saxattributes },
    { StartEndElement, "uri", "local", "uri:local", startendelement_xml, S_OK, &saxattributes },
    { StartEndElement, "", "", "", "</>", S_OK },
    { StartEndElement | DisableEscaping, "uri", "local", "uri:local", startendelement_xml, S_OK, &saxattributes },
};

static void test_mxwriter_startendelement_batch(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(writer_startendelement); ++i)
    {
        const struct writer_startendelement_t *table = &writer_startendelement[i];
        ISAXContentHandler *content;
        IMXWriter *writer;
        HRESULT hr;

        hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        if (table->type & DisableEscaping)
        {
            hr = IMXWriter_put_disableOutputEscaping(writer, VARIANT_TRUE);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        }

        if (table->type & StartElement)
        {
            hr = ISAXContentHandler_startElement(content, _bstr_(table->uri), table->uri ? strlen(table->uri) : 0,
                _bstr_(table->local_name), table->local_name ? strlen(table->local_name) : 0, _bstr_(table->qname),
                table->qname ? strlen(table->qname) : 0, table->attr);
            ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);
        }

        if (table->type & EndElement)
        {
            hr = ISAXContentHandler_endElement(content, _bstr_(table->uri), table->uri ? strlen(table->uri) : 0,
                _bstr_(table->local_name), table->local_name ? strlen(table->local_name) : 0, _bstr_(table->qname),
                table->qname ? strlen(table->qname) : 0);
            ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);
        }

        /* test output */
        if (hr == S_OK)
        {
            VARIANT dest;

            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(_bstr_(table->output), V_BSTR(&dest)),
                "test %d: got wrong content %s, expected %s\n", i, wine_dbgstr_w(V_BSTR(&dest)), table->output);
            VariantClear(&dest);
        }

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);
    }

    free_bstrs();
}

/* point of these test is to start/end element with different names and name lengths */
struct writer_startendelement2_t
{
    const char *qnamestart;
    int qnamestart_len;
    const char *qnameend;
    int qnameend_len;
    const char *output;
    HRESULT hr;
};

static const struct writer_startendelement2_t writer_startendelement2[] =
{
    { "a", -1, "b", -1, "<a/>", E_INVALIDARG },
    { "a", 1, "b", 1, "<a/>", S_OK },
};

static void test_mxwriter_startendelement_batch2(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(writer_startendelement2); ++i)
    {
        const struct writer_startendelement2_t *table = &writer_startendelement2[i];
        ISAXContentHandler *content;
        IMXWriter *writer;
        HRESULT hr;

        hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0,
            _bstr_(table->qnamestart), table->qnamestart_len, NULL);
        ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);

        hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0,
            _bstr_(table->qnameend), table->qnameend_len);
        ok(hr == table->hr, "test %d: got %#lx, expected %#lx\n", i, hr, table->hr);

        /* test output */
        if (hr == S_OK)
        {
            VARIANT dest;

            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(_bstr_(table->output), V_BSTR(&dest)),
                "test %d: got wrong content %s, expected %s\n", i, wine_dbgstr_w(V_BSTR(&dest)), table->output);
            VariantClear(&dest);
        }

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);

        free_bstrs();
    }
}

static void test_mxwriter_startendelement(void)
{
    ISAXContentHandler *content;
    IVBSAXContentHandler *vb_content;
    IMXWriter *writer;
    VARIANT dest;
    BSTR bstr_null = NULL, bstr_empty, bstr_a, bstr_b, bstr_ab;
    HRESULT hr;

    test_mxwriter_startendelement_batch();
    test_mxwriter_startendelement_batch2();

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void**)&vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_startDocument(vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    bstr_empty = SysAllocString(L"");
    bstr_a = SysAllocString(L"a");
    bstr_b = SysAllocString(L"b");
    bstr_ab = SysAllocString(L"a:b");

    hr = IVBSAXContentHandler_startElement(vb_content, &bstr_null, &bstr_empty, &bstr_b, NULL);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_startElement(vb_content, &bstr_empty, &bstr_b, &bstr_empty, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<b><>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = IVBSAXContentHandler_startElement(vb_content, &bstr_empty, &bstr_empty, &bstr_b, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<b><><b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_null, &bstr_null, &bstr_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_null, &bstr_a, &bstr_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_a, &bstr_b, &bstr_null);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_empty, &bstr_null, &bstr_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_empty, &bstr_b, &bstr_null);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_endElement(vb_content, &bstr_empty, &bstr_empty, &bstr_b);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<b><><b></b></b></></b></></b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    SysFreeString(bstr_empty);
    SysFreeString(bstr_a);
    SysFreeString(bstr_b);
    SysFreeString(bstr_ab);

    hr = IVBSAXContentHandler_endDocument(vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IVBSAXContentHandler_Release(vb_content);
    IMXWriter_Release(writer);

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* all string pointers should be not null */
    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_("b"), 1, _bstr_(""), 0, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("b"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<><b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endElement(content, NULL, 0, NULL, 0, _bstr_("a:b"), 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, NULL, 0, _bstr_("b"), 1, _bstr_("a:b"), 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* only local name is an error too */
    hr = ISAXContentHandler_endElement(content, NULL, 0, _bstr_("b"), 1, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("b"), 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<><b></a:b></a:b></></b>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("abcdef"), 3, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<abc>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IMXWriter_flush(writer);

    hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("abdcdef"), 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<abc></abd>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* length -1 */
    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("a"), -1, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);
    free_bstrs();
}

struct writer_characters_t
{
    const char *data;
    const char *output;
};

static const struct writer_characters_t writer_characters[] =
{
    { "< > & \" \'", "&lt; &gt; &amp; \" \'" },
};

static void test_mxwriter_characters(void)
{
    static const WCHAR embedded_nullbytes[] = L"a\0b\0\0\0c";
    IVBSAXContentHandler *vb_content;
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    BSTR str;
    HRESULT hr;
    int i = 0;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void**)&vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"TESTCHARDATA .", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = _bstr_("VbChars");
    hr = IVBSAXContentHandler_characters(vb_content, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"TESTCHARDATA .", 14);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"VbCharsTESTCHARDATA .", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ISAXContentHandler_Release(content);
    IVBSAXContentHandler_Release(vb_content);
    IMXWriter_Release(writer);

    /* try empty characters data to see if element is closed */
    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("a"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"TESTCHARDATA .", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("a"), 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<a></a>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    /* test embedded null bytes */
    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(SysStringLen(V_BSTR(&dest)) == ARRAY_SIZE(embedded_nullbytes), "unexpected len %d\n", SysStringLen(V_BSTR(&dest)));
    ok(!memcmp(V_BSTR(&dest), embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes)),
       "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXContentHandler, (void**)&vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXContentHandler_startDocument(vb_content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    str = SysAllocStringLen(embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes));
    hr = IVBSAXContentHandler_characters(vb_content, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    SysFreeString(str);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(SysStringLen(V_BSTR(&dest)) == 1, "Unexpected length %d.\n", SysStringLen(V_BSTR(&dest)));
    ok(!memcmp(V_BSTR(&dest), embedded_nullbytes, ARRAY_SIZE(embedded_nullbytes)),
       "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    IVBSAXContentHandler_Release(vb_content);
    IMXWriter_Release(writer);

    /* batch tests */
    for (i = 0; i < ARRAY_SIZE(writer_characters); ++i)
    {
        const struct writer_characters_t *table = &writer_characters[i];
        ISAXContentHandler *content;
        IMXWriter *writer;
        VARIANT dest;
        HRESULT hr;

        hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_characters(content, _bstr_(table->data), strlen(table->data));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* test output */
        if (hr == S_OK)
        {
            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(_bstr_(table->output), V_BSTR(&dest)),
                "test %d: got wrong content %s, expected \"%s\"\n", i, wine_dbgstr_w(V_BSTR(&dest)), table->output);
            VariantClear(&dest);
        }

        /* with disabled escaping */
        V_VT(&dest) = VT_EMPTY;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_disableOutputEscaping(writer, VARIANT_TRUE);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_characters(content, _bstr_(table->data), strlen(table->data));
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* test output */
        if (hr == S_OK)
        {
            V_VT(&dest) = VT_EMPTY;
            hr = IMXWriter_get_output(writer, &dest);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
            ok(!lstrcmpW(_bstr_(table->data), V_BSTR(&dest)),
                "test %d: got wrong content %s, expected \"%s\"\n", i, wine_dbgstr_w(V_BSTR(&dest)), table->data);
            VariantClear(&dest);
        }

        ISAXContentHandler_Release(content);
        IMXWriter_Release(writer);
    }

    free_bstrs();
}

static void test_mxwriter_domdoc(void)
{
    ISAXContentHandler *content;
    IXMLDOMDocument *domdoc;
    IMXWriter *writer;
    HRESULT hr;
    VARIANT dest;
    IXMLDOMElement *root = NULL;
    IXMLDOMNodeList *node_list = NULL;
    IXMLDOMNode *node = NULL;
    LONG list_length = 0;
    BSTR str;

    /* Create writer and attach DOMDocument output */
    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void**)&writer);
    ok(hr == S_OK, "Failed to create a writer, hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_DOMDocument60, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&domdoc);
    ok(hr == S_OK, "Failed to create a document, hr %#lx.\n", hr);

    V_VT(&dest) = VT_DISPATCH;
    V_DISPATCH(&dest) = (IDispatch *)domdoc;

    hr = IMXWriter_put_output(writer, dest);
    todo_wine
    ok(hr == S_OK, "Failed to set writer output, hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        IXMLDOMDocument_Release(domdoc);
        IMXWriter_Release(writer);
        return;
    }

    /* Add root element to document. */
    hr = IXMLDOMDocument_createElement(domdoc, _bstr_("TestElement"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IXMLDOMDocument_appendChild(domdoc, (IXMLDOMNode *)root, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(root);

    hr = IXMLDOMDocument_get_documentElement(domdoc, &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(root != NULL, "Unexpected document root.\n");
    IXMLDOMElement_Release(root);

    /* startDocument clears root element and disables methods. */
    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(domdoc, &root);
    todo_wine
    ok(hr == S_FALSE, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(domdoc, _bstr_("TestElement"), &root);
    todo_wine
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    /* startElement allows document root node to be accessed. */
    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"BankAccount", 11, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_get_documentElement(domdoc, &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(root != NULL, "Unexpected document root.\n");

    hr = IXMLDOMElement_get_nodeName(root, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!lstrcmpW(L"BankAccount", str), "Unexpected name %s.\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* startElement immediately updates previous node. */
    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"Number", 6, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMElement_get_childNodes(root, &node_list);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_length(node_list, &list_length);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(list_length == 1, "list length %ld, expected 1\n", list_length);

    hr = IXMLDOMNodeList_get_item(node_list, 0, &node);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_nodeName(node, &str);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"Number", str), "got %s\n", wine_dbgstr_w(str));
}
    SysFreeString(str);

    /* characters not immediately visible. */
    hr = ISAXContentHandler_characters(content, L"12345", 5);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_text(node, &str);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"", str), "got %s\n", wine_dbgstr_w(str));
}
    SysFreeString(str);

    /* characters visible after endElement. */
    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"Number", 6);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_text(node, &str);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"12345", str), "got %s\n", wine_dbgstr_w(str));
}
    SysFreeString(str);

    IXMLDOMNode_Release(node);

    /* second startElement updates the existing node list. */

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, L"Name", 4, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, L"Captain Ahab", 12);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"Name", 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, L"BankAccount", 11);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNodeList_get_length(node_list, &list_length);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(2 == list_length, "list length %ld, expected 2\n", list_length);
}
    hr = IXMLDOMNodeList_get_item(node_list, 1, &node);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMNode_get_nodeName(node, &str);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"Name", str), "got %s\n", wine_dbgstr_w(str));
}
    SysFreeString(str);

    hr = IXMLDOMNode_get_text(node, &str);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(L"Captain Ahab", str), "got %s\n", wine_dbgstr_w(str));
}
    SysFreeString(str);

    IXMLDOMNode_Release(node);
    IXMLDOMNodeList_Release(node_list);
    IXMLDOMElement_Release(root);

    /* endDocument makes document modifiable again. */

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLDOMDocument_createElement(domdoc, _bstr_("TestElement"), &root);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IXMLDOMElement_Release(root);

    /* finally check doc output */
    hr = IXMLDOMDocument_get_xml(domdoc, &str);
todo_wine {
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(
            L"<BankAccount>"
            "<Number>12345</Number>"
            "<Name>Captain Ahab</Name>"
            "</BankAccount>\r\n",
            str),
        "got %s\n", wine_dbgstr_w(str));
}
    SysFreeString(str);

    IXMLDOMDocument_Release(domdoc);
    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

static const char *encoding_names[] = {
    "iso-8859-1",
    "iso-8859-2",
    "iso-8859-3",
    "iso-8859-4",
    "iso-8859-5",
    "iso-8859-7",
    "iso-8859-9",
    "iso-8859-13",
    "iso-8859-15",
    NULL
};

static void test_mxwriter_encoding(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    IStream *stream;
    const char *enc;
    VARIANT dest;
    HRESULT hr;
    HGLOBAL g;
    char *ptr;
    BSTR s;
    int i;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* The content is always re-encoded to UTF-16 when the output is
     * retrieved as a BSTR.
     */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<?xml version=\"1.0\" standalone=\"no\"?>\r\n", V_BSTR(&dest)),
            "got wrong content: %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* switch encoding when something is written already */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_UNKNOWN;
    V_UNKNOWN(&dest) = (IUnknown*)stream;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-8"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* write empty element */
    hr = ISAXContentHandler_startElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("a"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, _bstr_(""), 0, _bstr_(""), 0, _bstr_("a"), 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* switch */
    hr = IMXWriter_put_encoding(writer, _bstr_("UTF-16"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_flush(writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = GetHGlobalFromStream(stream, &g);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ptr = GlobalLock(g);
    ok(!strncmp(ptr, "<a/>", 4), "got %c%c%c%c\n", ptr[0],ptr[1],ptr[2],ptr[3]);
    GlobalUnlock(g);

    /* so output is unaffected, encoding name is stored however */
    hr = IMXWriter_get_encoding(writer, &s);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!lstrcmpW(s, L"UTF-16"), "got %s\n", wine_dbgstr_w(s));
    SysFreeString(s);

    IStream_Release(stream);

    i = 0;
    enc = encoding_names[i];
    while (enc)
    {
        char expectedA[200];

        hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        V_VT(&dest) = VT_UNKNOWN;
        V_UNKNOWN(&dest) = (IUnknown*)stream;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_put_encoding(writer, _bstr_(enc));
        ok(hr == S_OK || broken(hr != S_OK) /* old win versions do not support certain encodings */,
            "%s: encoding not accepted\n", enc);
        if (hr != S_OK)
        {
            enc = encoding_names[++i];
            IStream_Release(stream);
            continue;
        }

        hr = ISAXContentHandler_startDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = ISAXContentHandler_endDocument(content);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXWriter_flush(writer);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        /* prepare expected string */
        *expectedA = 0;
        strcat(expectedA, "<?xml version=\"1.0\" encoding=\"");
        strcat(expectedA, enc);
        strcat(expectedA, "\" standalone=\"no\"?>\r\n");

        hr = GetHGlobalFromStream(stream, &g);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        ptr = GlobalLock(g);
        ok(!strncmp(ptr, expectedA, strlen(expectedA)), "%s: got %s, expected %.50s\n", enc, ptr, expectedA);
        GlobalUnlock(g);

        V_VT(&dest) = VT_EMPTY;
        hr = IMXWriter_put_output(writer, dest);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        IStream_Release(stream);

        enc = encoding_names[++i];
    }

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

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
    DISPID did;

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
    ok(props == 0, "Unexpected value %ld.\n", props);

    hr = IDispatchEx_GetMemberName(dispex, dispid, &name);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr)) SysFreeString(name);

    hr = IDispatchEx_GetNextDispID(dispex, fdexEnumDefault, DISPID_SAX_XMLREADER_GETFEATURE, &dispid);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);

    unk = (IUnknown*)0xdeadbeef;
    hr = IDispatchEx_GetNameSpaceParent(dispex, &unk);
    ok(hr == E_NOTIMPL, "Unexpected hr %#lx.\n", hr);
    ok(unk == (IUnknown*)0xdeadbeef, "got %p\n", unk);

    name = SysAllocString(L"testprop");
    hr = IDispatchEx_GetDispID(dispex, name, fdexNameEnsure, &did);
    ok(hr == DISP_E_UNKNOWNNAME, "Unexpected hr %#lx.\n", hr);
    SysFreeString(name);

    IDispatchEx_Release(dispex);
}

static void test_mxwriter_dispex(void)
{
    IDispatchEx *dispex;
    IMXWriter *writer;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_QueryInterface(dispex, &IID_IUnknown, (void**)&unk);
    test_obj_dispex(unk);
    IUnknown_Release(unk);
    IDispatchEx_Release(dispex);
    IMXWriter_Release(writer);

    if (is_class_supported(&CLSID_MXXMLWriter60))
    {
        hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&unk);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        test_obj_dispex(unk);
        IUnknown_Release(unk);
    }
}

static void test_mxwriter_comment(void)
{
    IVBSAXLexicalHandler *vblexical;
    ISAXContentHandler *content;
    ISAXLexicalHandler *lexical;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXLexicalHandler, (void**)&lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXLexicalHandler, (void**)&vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_comment(lexical, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXLexicalHandler_comment(vblexical, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_comment(lexical, L"comment", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!---->\r\n<!---->\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXLexicalHandler_comment(lexical, L"comment", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!---->\r\n<!---->\r\n<!--comment-->\r\n", V_BSTR(&dest)), "Unexpected content %s.\n",
            wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    ISAXLexicalHandler_Release(lexical);
    IVBSAXLexicalHandler_Release(vblexical);
    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_cdata(void)
{
    IVBSAXLexicalHandler *vblexical;
    ISAXContentHandler *content;
    ISAXLexicalHandler *lexical;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXLexicalHandler, (void**)&lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXLexicalHandler, (void**)&vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startCDATA(lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<![CDATA[", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = IVBSAXLexicalHandler_startCDATA(vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* all these are escaped for text nodes */
    hr = ISAXContentHandler_characters(content, _bstr_("< > & \""), 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_endCDATA(lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<![CDATA[<![CDATA[< > & \"]]>", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    ISAXLexicalHandler_Release(lexical);
    IVBSAXLexicalHandler_Release(vblexical);
    IMXWriter_Release(writer);
    free_bstrs();
}

static void test_mxwriter_pi(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, NULL, 0, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 6, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<?\?>\r\n<?\?>\r\n<?target?>\r\n", V_BSTR(&dest)), "Unexpected content %s.\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 4, L"data", 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<?\?>\r\n<?\?>\r\n<?target?>\r\n<?targ data?>\r\n", V_BSTR(&dest)), "Unexpected content %s.\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_processingInstruction(content, L"target", 6, L"data", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?target?>\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);


    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);
}

static void test_mxwriter_ignorablespaces(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, L"data", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, L"data", 4);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_ignorableWhitespace(content, L"data", 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"datad", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);
}

static void test_mxwriter_dtd(void)
{
    IVBSAXLexicalHandler *vblexical;
    ISAXContentHandler *content;
    ISAXLexicalHandler *lexical;
    IVBSAXDeclHandler *vbdecl;
    ISAXDeclHandler *decl;
    ISAXDTDHandler *dtd;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXLexicalHandler, (void**)&lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXDeclHandler, (void**)&decl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXDeclHandler, (void**)&vbdecl);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_IVBSAXLexicalHandler, (void**)&vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_omitXMLDeclaration(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, NULL, 0, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXLexicalHandler_startDTD(vblexical, NULL, NULL, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, L"pub", 3, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, NULL, 0, L"sys", 3);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, NULL, 0, L"pub", 3, L"sys", 3);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, L"name", 4, NULL, 0, NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!DOCTYPE  [\r\n<!DOCTYPE  PUBLIC \"pub\"<!DOCTYPE  SYSTEM \"sys\" [\r\n"
            "<!DOCTYPE  PUBLIC \"pub\" \"sys\" [\r\n<!DOCTYPE name [\r\n", V_BSTR(&dest)), "Unexpected content %s.\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* system id is required if public is present */
    hr = ISAXLexicalHandler_startDTD(lexical, L"name", 4, L"pub", 3, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXLexicalHandler_startDTD(lexical, L"name", 4, L"pub", 3, L"sys", 3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!DOCTYPE  [\r\n<!DOCTYPE  PUBLIC \"pub\"<!DOCTYPE  SYSTEM \"sys\" [\r\n"
            "<!DOCTYPE  PUBLIC \"pub\" \"sys\" [\r\n<!DOCTYPE name [\r\n<!DOCTYPE name PUBLIC \"pub\"<!DOCTYPE name PUBLIC \"pub\" \"sys\" [\r\n",
            V_BSTR(&dest)), "Unexpected content %s.\n", debugstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXLexicalHandler_endDTD(lexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXLexicalHandler_endDTD(vblexical);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!DOCTYPE  [\r\n<!DOCTYPE  PUBLIC \"pub\"<!DOCTYPE  SYSTEM \"sys\" [\r\n"
            "<!DOCTYPE  PUBLIC \"pub\" \"sys\" [\r\n<!DOCTYPE name [\r\n<!DOCTYPE name PUBLIC \"pub\"<!DOCTYPE name PUBLIC \"pub\" \"sys\" [\r\n]>\r\n]>\r\n",
            V_BSTR(&dest)), "Unexpected content %s.\n", debugstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* element declaration */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, NULL, 0, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXDeclHandler_elementDecl(vbdecl, NULL, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, L"name", 4, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, L"name", 4, L"content", 7);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!ELEMENT  >\r\n<!ELEMENT name >\r\n<!ELEMENT name content>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_elementDecl(decl, L"name", 4, L"content", 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<!ELEMENT name >\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* attribute declaration */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_attributeDecl(decl, _bstr_("element"), strlen("element"),
        _bstr_("attribute"), strlen("attribute"), _bstr_("CDATA"), strlen("CDATA"),
        _bstr_("#REQUIRED"), strlen("#REQUIRED"), _bstr_("value"), strlen("value"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!ATTLIST element attribute CDATA #REQUIRED>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    hr = ISAXDeclHandler_attributeDecl(decl, _bstr_("element"), strlen("element"),
        _bstr_("attribute2"), strlen("attribute2"), _bstr_("CDATA"), strlen("CDATA"),
        _bstr_("#REQUIRED"), strlen("#REQUIRED"), _bstr_("value2"), strlen("value2"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_attributeDecl(decl, _bstr_("element2"), strlen("element2"),
        _bstr_("attribute3"), strlen("attribute3"), _bstr_("CDATA"), strlen("CDATA"),
        _bstr_("#REQUIRED"), strlen("#REQUIRED"), _bstr_("value3"), strlen("value3"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!ATTLIST element attribute CDATA #REQUIRED>\r\n"
            "<!ATTLIST element attribute2 CDATA #REQUIRED>\r\n<!ATTLIST element2 attribute3 CDATA #REQUIRED>\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* internal entities */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_internalEntityDecl(decl, NULL, 0, NULL, 0);
    todo_wine
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXDeclHandler_internalEntityDecl(vbdecl, NULL, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_internalEntityDecl(decl, _bstr_("name"), -1, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_internalEntityDecl(decl, _bstr_("name"), strlen("name"), _bstr_("value"), strlen("value"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!ENTITY  \"\">\r\n<!ENTITY name \"value\">\r\n", V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    /* external entities */
    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, NULL, 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IVBSAXDeclHandler_externalEntityDecl(vbdecl, NULL, NULL, NULL);
    todo_wine
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, _bstr_("name"), 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, _bstr_("name"), -1, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, _bstr_("name"), strlen("name"), _bstr_("pubid"), strlen("pubid"),
        _bstr_("sysid"), strlen("sysid"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, _bstr_("name"), strlen("name"), NULL, 0, _bstr_("sysid"), strlen("sysid"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDeclHandler_externalEntityDecl(decl, _bstr_("name"), strlen("name"), _bstr_("pubid"), strlen("pubid"),
        NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!ENTITY <!ENTITY <!ENTITY name PUBLIC \"pubid\" \"sysid\">\r\n"
            "<!ENTITY name SYSTEM \"sysid\">\r\n<!ENTITY name PUBLIC \"pubid\"",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));

    VariantClear(&dest);

    /* notation declaration */
    hr = IMXWriter_QueryInterface(writer, &IID_ISAXDTDHandler, (void**)&dtd);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_put_output(writer, dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, NULL, 0, NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, _bstr_("name"), strlen("name"), NULL, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, _bstr_("name"), strlen("name"), _bstr_("pubid"), strlen("pubid"), NULL, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, _bstr_("name"), strlen("name"), _bstr_("pubid"), strlen("pubid"), _bstr_("sysid"), strlen("sysid"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXDTDHandler_notationDecl(dtd, _bstr_("name"), strlen("name"), NULL, 0, _bstr_("sysid"), strlen("sysid"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    todo_wine
    ok(!lstrcmpW(L"<!NOTATION <!NOTATION name<!NOTATION name PUBLIC \"pubid\">\r\n"
            "<!NOTATION name PUBLIC \"pubid\" \"sysid\">\r\n"
            "<!NOTATION name SYSTEM \"sysid\">\r\n",
        V_BSTR(&dest)), "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));

    VariantClear(&dest);

    ISAXDTDHandler_Release(dtd);

    ISAXContentHandler_Release(content);
    ISAXLexicalHandler_Release(lexical);
    IVBSAXLexicalHandler_Release(vblexical);
    IVBSAXDeclHandler_Release(vbdecl);
    ISAXDeclHandler_Release(decl);
    IMXWriter_Release(writer);
    free_bstrs();
}

typedef struct
{
    const char *uri;
    const char *local;
    const char *qname;
    const char *type;
    const char *value;
    HRESULT hr;
} addattribute_test_t;

static const addattribute_test_t addattribute_data[] =
{
    { NULL, NULL, "ns:qname", NULL, "value", S_OK },
    { NULL, "qname", "ns:qname", NULL, "value", S_OK },
    { "uri", "qname", "ns:qname", NULL, "value", S_OK },
    { "uri", "qname", "ns:qname", "type", "value", S_OK },
};

static void test_mxattr_addAttribute(void)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(addattribute_data); ++i)
    {
        const addattribute_test_t *table = &addattribute_data[i];
        ISAXAttributes *saxattr;
        IMXAttributes *mxattr;
        const WCHAR *value;
        int len, index;
        HRESULT hr;

        hr = CoCreateInstance(&CLSID_SAXAttributes60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXAttributes, (void **)&mxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        hr = IMXAttributes_QueryInterface(mxattr, &IID_ISAXAttributes, (void**)&saxattr);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        len = -1;
        hr = ISAXAttributes_getLength(saxattr, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(len == 0, "got %d\n", len);

        hr = ISAXAttributes_getValue(saxattr, 0, &value, &len);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getValue(saxattr, 0, NULL, &len);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getValue(saxattr, 0, &value, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getValue(saxattr, 0, NULL, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, &value, &len);
        ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, NULL, &len);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, &value, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = ISAXAttributes_getType(saxattr, 0, NULL, NULL);
        ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

        hr = IMXAttributes_addAttribute(mxattr, _bstr_(table->uri), _bstr_(table->local),
            _bstr_(table->qname), _bstr_(table->type), _bstr_(table->value));
        ok(hr == table->hr, "%d: got %#lx, expected %#lx.\n", i, hr, table->hr);

        if (hr == S_OK)
        {
            /* Crashes. */
            if (0)
            {
               hr = ISAXAttributes_getValue(saxattr, 0, NULL, &len);
               ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

               hr = ISAXAttributes_getValue(saxattr, 0, &value, NULL);
               ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

               hr = ISAXAttributes_getValue(saxattr, 0, NULL, NULL);
               ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

               hr = ISAXAttributes_getType(saxattr, 0, NULL, &len);
               ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

               hr = ISAXAttributes_getType(saxattr, 0, &value, NULL);
               ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

               hr = ISAXAttributes_getType(saxattr, 0, NULL, NULL);
               ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
            }

            len = -1;
            hr = ISAXAttributes_getValue(saxattr, 0, &value, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(_bstr_(table->value), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                table->value);
            ok(lstrlenW(value) == len, "%d: got wrong value length %d\n", i, len);

            len = -1;
            value = (void*)0xdeadbeef;
            hr = ISAXAttributes_getType(saxattr, 0, &value, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

            if (table->type)
            {
                ok(!lstrcmpW(_bstr_(table->type), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                    table->type);
                ok(lstrlenW(value) == len, "%d: got wrong type value length %d\n", i, len);
            }
            else
            {
                ok(*value == 0, "%d: got type value %s\n", i, wine_dbgstr_w(value));
                ok(len == 0, "%d: got wrong type value length %d\n", i, len);
            }

            hr = ISAXAttributes_getIndexFromQName(saxattr, NULL, 0, NULL);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getIndexFromQName(saxattr, NULL, 0, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_("nonexistent"), 11, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            ok(index == -1, "%d: got wrong index %d\n", i, index);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_(table->qname), 0, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            ok(index == -1, "%d: got wrong index %d\n", i, index);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_(table->qname), strlen(table->qname), &index);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(index == 0, "%d: got wrong index %d\n", i, index);

            index = -1;
            hr = ISAXAttributes_getIndexFromQName(saxattr, _bstr_(table->qname), strlen(table->qname)-1, &index);
            ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
            ok(index == -1, "%d: got wrong index %d\n", i, index);

            hr = ISAXAttributes_getValueFromQName(saxattr, NULL, 0, NULL, NULL);
            ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), 0, NULL, NULL);
            ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), 0, &value, NULL);
            ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, NULL, 0, NULL, 0, NULL, NULL);
            ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), 0, NULL, 0, NULL, NULL);
            ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), 0, NULL, 0, &value, NULL);
            ok(hr == E_POINTER /* win8 */ || hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

            hr = ISAXAttributes_getValueFromQName(saxattr, _bstr_(table->qname), strlen(table->qname), &value, &len);
            ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
            ok(!lstrcmpW(_bstr_(table->value), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                table->value);
            ok(lstrlenW(value) == len, "%d: got wrong value length %d\n", i, len);

            if (table->uri) {
                hr = ISAXAttributes_getValueFromName(saxattr, _bstr_(table->uri), strlen(table->uri),
                    _bstr_(table->local), strlen(table->local), &value, &len);
                ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
                ok(!lstrcmpW(_bstr_(table->value), value), "%d: got %s, expected %s\n", i, wine_dbgstr_w(value),
                    table->value);
                ok(lstrlenW(value) == len, "%d: got wrong value length %d\n", i, len);
            }
        }

        len = -1;
        hr = ISAXAttributes_getLength(saxattr, &len);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (table->hr == S_OK)
            ok(len == 1, "%d: got %d length, expected 1\n", i, len);
        else
            ok(len == 0, "%d: got %d length, expected 0\n", i, len);

        ISAXAttributes_Release(saxattr);
        IMXAttributes_Release(mxattr);
    }

    free_bstrs();
}

static void test_mxattr_clear(void)
{
    ISAXAttributes *saxattr;
    IMXAttributes *mxattr;
    const WCHAR *ptr;
    HRESULT hr;
    int len;

    hr = CoCreateInstance(&CLSID_SAXAttributes60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXAttributes, (void **)&mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_QueryInterface(mxattr, &IID_ISAXAttributes, (void**)&saxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getQName(saxattr, 0, NULL, NULL);
    todo_wine
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_clear(mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_addAttribute(mxattr, _bstr_("uri"), _bstr_("local"),
        _bstr_("qname"), _bstr_("type"), _bstr_("value"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = ISAXAttributes_getLength(saxattr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 1, "got %d\n", len);

    len = -1;
    hr = ISAXAttributes_getQName(saxattr, 0, NULL, &len);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    ok(len == -1, "got %d\n", len);

    ptr = (void*)0xdeadbeef;
    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!ptr, "Unexpected pointer %p.\n", ptr);

    len = 0;
    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 5, "got %d\n", len);
    ok(!lstrcmpW(ptr, L"qname"), "got %s\n", wine_dbgstr_w(ptr));

    hr = IMXAttributes_clear(mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    len = -1;
    hr = ISAXAttributes_getLength(saxattr, &len);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(len == 0, "got %d\n", len);

    len = -1;
    ptr = (void*)0xdeadbeef;
    hr = ISAXAttributes_getQName(saxattr, 0, &ptr, &len);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    todo_wine
    ok(!len, "Unexpected length %d.\n", len);
    todo_wine
    ok(!ptr, "Unexpected pointer %p.\n", ptr);

    IMXAttributes_Release(mxattr);
    ISAXAttributes_Release(saxattr);
    free_bstrs();
}

static void test_mxattr_dispex(void)
{
    IMXAttributes *mxattr;
    IDispatchEx *dispex;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXAttributes60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXAttributes, (void **)&mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_QueryInterface(mxattr, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IDispatchEx_QueryInterface(dispex, &IID_IUnknown, (void**)&unk);
    test_obj_dispex(unk);
    IUnknown_Release(unk);
    IDispatchEx_Release(dispex);

    IMXAttributes_Release(mxattr);
}

static void test_mxattr_qi(void)
{
    IMXAttributes *mxattr;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SAXAttributes60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXAttributes, (void **)&mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    check_interface(mxattr, &IID_IMXAttributes, TRUE);
    check_interface(mxattr, &IID_ISAXAttributes, TRUE);
    check_interface(mxattr, &IID_IVBSAXAttributes, TRUE);
    check_interface(mxattr, &IID_IDispatch, TRUE);
    check_interface(mxattr, &IID_IDispatchEx, TRUE);

    IMXAttributes_Release(mxattr);
}

static void test_mxattr_localname(void)
{
    ISAXAttributes *saxattr;
    IMXAttributes *mxattr;
    HRESULT hr;
    int index;

    hr = CoCreateInstance(&CLSID_SAXAttributes60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXAttributes, (void **)&mxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXAttributes_QueryInterface(mxattr, &IID_ISAXAttributes, (void**)&saxattr);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, NULL, 0, &index);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* add some ambiguous attribute names */
    hr = IMXAttributes_addAttribute(mxattr, _bstr_("uri"), _bstr_("localname"),
        _bstr_("a:localname"), _bstr_(""), _bstr_("value"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    hr = IMXAttributes_addAttribute(mxattr, _bstr_("uri"), _bstr_("localname"),
        _bstr_("b:localname"), _bstr_(""), _bstr_("value"));
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    index = -1;
    hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname", 9, &index);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!index, "Got index %d.\n", index);

    index = -1;
    hr = ISAXAttributes_getIndexFromName(saxattr, L"uri1", 4, L"localname", 9, &index);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(index == -1, "Got index %d.\n", index);

    index = -1;
    hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname1", 10, &index);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);
    ok(index == -1, "Got index %d.\n", index);

    hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, L"localname1", 10, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getIndexFromName(saxattr, L"uri", 3, NULL, 0, &index);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = ISAXAttributes_getIndexFromName(saxattr, NULL, 0, L"localname1", 10, &index);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    ISAXAttributes_Release(saxattr);
    IMXAttributes_Release(mxattr);
    free_bstrs();
}

static void test_mxwriter_indent(void)
{
    ISAXContentHandler *content;
    IMXWriter *writer;
    VARIANT dest;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MXXMLWriter60, NULL, CLSCTX_INPROC_SERVER, &IID_IMXWriter, (void **)&writer);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_put_indent(writer, VARIANT_TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IMXWriter_QueryInterface(writer, &IID_ISAXContentHandler, (void**)&content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("a"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_characters(content, _bstr_(""), 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("b"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_startElement(content, L"", 0, L"", 0, _bstr_("c"), 1, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, _bstr_("c"), 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, _bstr_("b"), 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endElement(content, L"", 0, L"", 0, _bstr_("a"), 1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = ISAXContentHandler_endDocument(content);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    V_VT(&dest) = VT_EMPTY;
    hr = IMXWriter_get_output(writer, &dest);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(V_VT(&dest) == VT_BSTR, "got %d\n", V_VT(&dest));
    ok(!lstrcmpW(L"<?xml version=\"1.0\" encoding=\"UTF-16\" standalone=\"no\"?>\r\n<a><b>\r\n\t\t<c/>\r\n\t</b>\r\n</a>", V_BSTR(&dest)),
        "got wrong content %s\n", wine_dbgstr_w(V_BSTR(&dest)));
    VariantClear(&dest);

    ISAXContentHandler_Release(content);
    IMXWriter_Release(writer);

    free_bstrs();
}

START_TEST(saxreader)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "Failed to initialize COM, hr %#lx.\n", hr);

    get_class_support_data();

    if (is_class_supported(&CLSID_MXXMLWriter60))
    {
        test_mxwriter_handlers();
        test_mxwriter_startenddocument();
        test_mxwriter_startendelement();
        test_mxwriter_characters();
        test_mxwriter_comment();
        test_mxwriter_cdata();
        test_mxwriter_pi();
        test_mxwriter_ignorablespaces();
        test_mxwriter_dtd();
        test_mxwriter_properties();
        test_mxwriter_flush();
        test_mxwriter_domdoc();
        test_mxwriter_encoding();
        test_mxwriter_dispex();
        test_mxwriter_indent();
    }

    if (is_class_supported(&CLSID_SAXAttributes60))
    {
        test_mxattr_qi();
        test_mxattr_addAttribute();
        test_mxattr_clear();
        test_mxattr_localname();
        test_mxattr_dispex();
    }

    CoUninitialize();
}
