/*
 * IXMLDocument tests
 *
 * Copyright 2007 James Hawkins
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

#include <stdio.h>
#include "windows.h"
#include "ole2.h"
#include "msxml2.h"
#include "msxml2did.h"
#include "ocidl.h"

#include "wine/test.h"

#define EXPECT_HR(hr,hr_exp) \
    ok(hr == hr_exp, "got 0x%08x, expected 0x%08x\n", hr, hr_exp)

/* Deprecated Error Code */
#define XML_E_INVALIDATROOTLEVEL    0xc00ce556

static void create_xml_file(LPCSTR filename)
{
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    static const char data[] =
        "<?xml version=\"1.0\" ?>\n"
        "<!DOCTYPE BankAccount>\n"
        "<BankAccount>\n"
        "  <Number>1234</Number>\n"
        "  <Name>Captain Ahab</Name>\n"
        "</BankAccount>";

    WriteFile(hf, data, sizeof(data) - 1, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void create_stream_on_file(IStream **stream, LPCSTR path)
{
    HANDLE hfile;
    HGLOBAL hglobal;
    LPVOID ptr;
    HRESULT hr;
    DWORD file_size, read;

    hfile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "Expected a valid file handle\n");
    file_size = GetFileSize(hfile, NULL);

    hglobal = GlobalAlloc(GHND, file_size);
    ptr = GlobalLock(hglobal);

    ReadFile(hfile, ptr, file_size, &read, NULL);
    ok(file_size == read, "Expected to read the whole file, read %d\n", read);

    hr = CreateStreamOnHGlobal(hglobal, TRUE, stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(*stream != NULL, "Expected non-NULL stream\n");

    CloseHandle(hfile);
    GlobalUnlock(hglobal);
}

static void test_xmldoc(void)
{
    IXMLElement *element = NULL, *child = NULL, *value = NULL;
    IXMLElementCollection *collection = NULL, *inner = NULL;
    IPersistStreamInit *psi = NULL;
    IXMLDocument *doc = NULL;
    IStream *stream = NULL;
    VARIANT vIndex, vName;
    LONG type, num_child;
    CHAR path[MAX_PATH];
    IDispatch *disp;
    ITypeInfo *ti;
    HRESULT hr;
    BSTR name;

    static const WCHAR szBankAccount[] = {'B','A','N','K','A','C','C','O','U','N','T',0};
    static const WCHAR szNumber[] = {'N','U','M','B','E','R',0};
    static const WCHAR szNumVal[] = {'1','2','3','4',0};
    static const WCHAR szName[] = {'N','A','M','E',0};
    static const WCHAR szNameVal[] = {'C','a','p','t','a','i','n',' ','A','h','a','b',0};
    static const WCHAR szVersion[] = {'1','.','0',0};
    static const WCHAR rootW[] = {'r','o','o','t',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (void**)&doc);
    EXPECT_HR(hr, S_OK);

    /* IDispatch */
    hr = IXMLDocument_QueryInterface(doc, &IID_IDispatch, (void**)&disp);
    EXPECT_HR(hr, S_OK);

    /* just to make sure we're on right type data */
    hr = IDispatch_GetTypeInfo(disp, 0, 0, &ti);
    EXPECT_HR(hr, S_OK);
    name = NULL;
    hr = ITypeInfo_GetDocumentation(ti, DISPID_XMLDOCUMENT_ROOT, &name, NULL, NULL, NULL);
    EXPECT_HR(hr, S_OK);
    ok(!lstrcmpW(name, rootW), "got name %s\n", wine_dbgstr_w(name));
    SysFreeString(name);

    ITypeInfo_Release(ti);
    IDispatch_Release(disp);

    hr = IXMLDocument_QueryInterface(doc, &IID_IXMLDOMDocument, (void**)&disp);
    EXPECT_HR(hr, E_NOINTERFACE);

    create_xml_file("bank.xml");
    GetFullPathNameA("bank.xml", MAX_PATH, path, NULL);
    create_stream_on_file(&stream, path);

    hr = IXMLDocument_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&psi);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(psi != NULL, "Expected non-NULL psi\n");

    hr = IXMLDocument_get_root(doc, &element);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);
    ok(element == NULL, "Expected NULL element\n");

    hr = IPersistStreamInit_Load(psi, stream);
    ok(hr == S_OK || hr == XML_E_INVALIDATROOTLEVEL, "Expected S_OK, got %08x\n", hr);
    if(hr == XML_E_INVALIDATROOTLEVEL)
        goto cleanup;

    ok(stream != NULL, "Expected non-NULL stream\n");

    /* version field */
    hr = IXMLDocument_get_version(doc, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    name = NULL;
    hr = IXMLDocument_get_version(doc, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szVersion), "Expected 1.0, got %s\n", wine_dbgstr_w(name));
    SysFreeString(name);

    /* doctype */
    hr = IXMLDocument_get_doctype(doc, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXMLDocument_get_doctype(doc, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szBankAccount), "Expected BANKACCOUNT, got %s\n", wine_dbgstr_w(name));
    SysFreeString(name);

    hr = IXMLDocument_get_root(doc, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

    /* ::root() returns new instance each time */
    hr = IXMLDocument_get_root(doc, &child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL element\n");
    ok(child != element, "Expected new element instance\n");
    IXMLElement_Release(child);

    hr = IXMLElement_get_type(element, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(element, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szBankAccount), "Expected BANKACCOUNT\n");
    SysFreeString(name);

    hr = IXMLElement_get_children(element, &collection);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(collection != NULL, "Expected non-NULL collection\n");

    hr = IXMLElementCollection_get_length(collection, &num_child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(num_child == 2, "Expected 2, got %d\n", num_child);

    V_VT(&vIndex) = VT_I4;
    V_I4(&vIndex) = 0;
    V_VT(&vName) = VT_ERROR;
    V_ERROR(&vName) = DISP_E_PARAMNOTFOUND;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_get_type(child, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(child, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szNumber), "Expected NUMBER\n");
    SysFreeString(name);

    hr = IXMLElement_get_children(child, &inner);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(inner != NULL, "Expected non-NULL inner\n");

    hr = IXMLElementCollection_get_length(inner, &num_child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(num_child == 1, "Expected 1, got %d\n", num_child);

    hr = IXMLElementCollection_item(inner, vIndex, vName, (IDispatch **)&value);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(value != NULL, "Expected non-NULL value\n");

    hr = IXMLElement_get_type(value, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_TEXT, "Expected XMLELEMTYPE_TEXT, got %d\n", type);

    hr = IXMLElement_get_text(value, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szNumVal), "Expected '1234'\n");
    SysFreeString(name);

    IXMLElementCollection_Release(inner);

    inner = (IXMLElementCollection *)0xdeadbeef;
    hr = IXMLElement_get_children(value, &inner);
    ok(hr == 1, "Expected 1, got %08x\n", hr);
    ok(inner == NULL, "Expected NULL inner, got %p\n", inner);

    IXMLElement_Release(value);
    IXMLElement_Release(child);
    value = NULL;
    child = NULL;

    V_I4(&vIndex) = 1;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_get_type(child, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(child, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szName), "Expected NAME\n");
    SysFreeString(name);

    hr = IXMLElement_get_children(child, &inner);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(inner != NULL, "Expected non-NULL inner\n");

    hr = IXMLElementCollection_get_length(inner, &num_child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(num_child == 1, "Expected 1, got %d\n", num_child);

    V_I4(&vIndex) = 0;
    hr = IXMLElementCollection_item(inner, vIndex, vName, (IDispatch **)&value);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(value != NULL, "Expected non-NULL value\n");

    hr = IXMLElement_get_type(value, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_TEXT, "Expected XMLELEMTYPE_TEXT, got %d\n", type);

    hr = IXMLElement_get_text(value, &name);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(name, szNameVal), "Expected 'Captain Ahab'\n");
    SysFreeString(name);

    IXMLElementCollection_Release(inner);

    inner = (IXMLElementCollection *)0xdeadbeef;
    hr = IXMLElement_get_children(value, &inner);
    ok(hr == 1, "Expected 1, got %08x\n", hr);
    ok(inner == NULL, "Expected NULL inner, got %p\n", inner);

    IXMLElement_Release(value);
    IXMLElement_Release(child);
    IXMLElementCollection_Release(collection);
    IXMLElement_Release(element);
cleanup:
    IStream_Release(stream);
    IPersistStreamInit_Release(psi);
    IXMLDocument_Release(doc);

    DeleteFileA("bank.xml");
}

static void test_createElement(void)
{
    HRESULT hr;
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL, *root = NULL;
    VARIANT vType, vName;
    LONG type;

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* invalid vType type */
    V_VT(&vType) = VT_NULL;
    V_VT(&vName) = VT_NULL;
    element = (IXMLElement *)0xdeadbeef;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok(element == NULL, "Expected NULL element\n");

    /* invalid vType value */
    V_VT(&vType) = VT_I4;
    V_I4(&vType) = -1;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    /* Up to and including SP7, createElement returns an element. */
    if(hr == S_OK)
    {
        ok(element != NULL, "Expected element\n");
        if (element != NULL)
        {
            hr = IXMLElement_get_type(element, &type);
            ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
            /* SP7 returns an XMLELEMTYPE_ELEMENT */
            ok(type == XMLELEMTYPE_OTHER || type == XMLELEMTYPE_ELEMENT,
                         "Expected XMLELEMTYPE_OTHER || XMLELEMTYPE_ELEMENT, got %d\n", type);

            IXMLElement_Release(element);
        }
    }
    else
    {
        ok(hr == E_NOTIMPL, "Expected E_NOTIMPL, got %08x\n", hr);
        ok(element == NULL, "Expected NULL element\n");
    }

    /* invalid vName type */
    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_ELEMENT;
    V_VT(&vName) = VT_I4;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

    hr = IXMLElement_get_type(element, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    IXMLElement_Release(element);

    /* NULL element */
    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_ELEMENT;
    V_VT(&vName) = VT_I4;
    hr = IXMLDocument_createElement(doc, vType, vName, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    root = (IXMLElement *)0xdeadbeef;
    hr = IXMLDocument_get_root(doc, &root);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);
    ok(root == NULL, "Expected NULL root\n");

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_ELEMENT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

    /* createElement does not set the new element as root */
    root = (IXMLElement *)0xdeadbeef;
    hr = IXMLDocument_get_root(doc, &root);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);
    ok(root == NULL, "Expected NULL root\n");

    IXMLElement_Release(element);
    IXMLDocument_Release(doc);
}

static void test_persiststreaminit(void)
{
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL;
    IPersistStreamInit *psi = NULL;
    IStream *stream = NULL;
    STATSTG stat;
    HRESULT hr;
    ULARGE_INTEGER size;
    CHAR path[MAX_PATH];
    CLSID id;
    BSTR str;
    static const WCHAR testW[] = {'t','e','s','t',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXMLDocument_QueryInterface(doc, &IID_IPersistStreamInit, (LPVOID *)&psi);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(psi != NULL, "Expected non-NULL psi\n");

    /* null arguments */
    hr = IPersistStreamInit_GetSizeMax(psi, NULL);
    ok(hr == E_NOTIMPL, "Expected E_NOTIMPL, got %08x\n", hr);

    hr = IPersistStreamInit_Load(psi, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IPersistStreamInit_Save(psi, NULL, FALSE);
    todo_wine ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IPersistStreamInit_GetClassID(psi, NULL);
    ok(hr == E_POINTER, "Expected E_POINTER, got %08x\n", hr);

    hr = IPersistStreamInit_IsDirty(psi);
    todo_wine ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);

    create_xml_file("bank.xml");
    GetFullPathNameA("bank.xml", MAX_PATH, path, NULL);
    create_stream_on_file(&stream, path);

    /* GetSizeMax not implemented */
    size.QuadPart = 0;
    hr = IPersistStreamInit_GetSizeMax(psi, &size);
    ok(hr == E_NOTIMPL, "Expected E_NOTIMPL, got %08x\n", hr);
    ok(size.QuadPart == 0, "Expected 0\n");

    hr = IPersistStreamInit_Load(psi, stream);
    IStream_Release(stream);
    ok(hr == S_OK || hr == XML_E_INVALIDATROOTLEVEL, "Expected S_OK, got %08x\n", hr);
    if(hr == XML_E_INVALIDATROOTLEVEL)
        goto cleanup;

    hr = IPersistStreamInit_IsDirty(psi);
    todo_wine ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);

    /* try to save document */
    stream = NULL;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IPersistStreamInit_Save(psi, stream, FALSE);
    todo_wine ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stat.cbSize.QuadPart = 0;
    hr = IStream_Stat(stream, &stat, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    todo_wine ok(stat.cbSize.QuadPart > 0, "Expected >0\n");
    IStream_Release(stream);

    str = SysAllocString(testW);
    hr = IXMLDocument_get_root(doc, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IXMLElement_put_text(element, str);
    ok(hr == E_NOTIMPL, "Expected E_NOTIMPL, got %08x\n", hr);
    IXMLElement_Release(element);
    SysFreeString(str);

    hr = IPersistStreamInit_IsDirty(psi);
    todo_wine ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);

    create_stream_on_file(&stream, path);
    hr = IPersistStreamInit_Load(psi, stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    IStream_Release(stream);

    hr = IPersistStreamInit_IsDirty(psi);
    todo_wine ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);

    /* reset internal stream */
    hr = IPersistStreamInit_InitNew(psi);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IPersistStreamInit_IsDirty(psi);
    todo_wine ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);

    stream = NULL;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IPersistStreamInit_Save(psi, stream, FALSE);
    todo_wine ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    stat.cbSize.QuadPart = 0;
    hr = IStream_Stat(stream, &stat, 0);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    todo_wine ok(stat.cbSize.QuadPart > 0, "Expected >0\n");
    IStream_Release(stream);

    memset(&id, 0, sizeof(id));
    hr = IPersistStreamInit_GetClassID(psi, &id);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(IsEqualCLSID(&id, &CLSID_XMLDocument), "Expected CLSID_XMLDocument\n");

cleanup:
    IPersistStreamInit_Release(psi);
    IXMLDocument_Release(doc);
    DeleteFileA("bank.xml");
}

static BOOL test_try_xmldoc(void)
{
    IXMLDocument *doc = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    if (FAILED(hr))
    {
        win_skip("Failed to create XMLDocument instance\n");
        return FALSE;
    }

    IXMLDocument_Release(doc);
    return TRUE;
}

static void test_xmlelem_children(void)
{
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL, *child = NULL, *child2 = NULL;
    IXMLElementCollection *collection = NULL;
    VARIANT vType, vName, vIndex;
    LONG length;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_ELEMENT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_TEXT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_TEXT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &child2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child2 != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_addChild(element, child, 0, -1);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXMLElement_get_children(element, &collection);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(collection != NULL, "Expected non-NULL collection\n");

    length = 0;
    hr = IXMLElementCollection_get_length(collection, &length);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(length == 1, "Expected 1, got %08x\n", length);

    /* remove/add child and check what happens with collection */
    hr = IXMLElement_removeChild(element, child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    length = -1;
    hr = IXMLElementCollection_get_length(collection, &length);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(length == 0, "Expected 0, got %08x\n", length);
    IXMLElementCollection_Release(collection);

    hr = IXMLElement_AddRef(child);
    ok(hr == 2, "Expected 2, got %08x\n", hr);
    IXMLElement_Release(child);
    hr = IXMLElement_addChild(element, child, 0, -1);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    hr = IXMLElement_AddRef(child);
    ok(hr == 2, "Expected 2, got %08x\n", hr);
    IXMLElement_Release(child);
    hr = IXMLElement_addChild(element, child2, 0, -1);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXMLElement_get_children(element, &collection);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(collection != NULL, "Expected non-NULL collection\n");

    hr = IXMLElement_AddRef(child);
    ok(hr == 2, "Expected 2, got %08x\n", hr);
    IXMLElement_Release(child);

    length = 0;
    hr = IXMLElementCollection_get_length(collection, &length);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(length == 2, "Expected 2, got %08x\n", length);

    IXMLElement_Release(child2);

    length = 0;
    hr = IXMLElementCollection_get_length(collection, &length);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(length == 2, "Expected 2, got %08x\n", length);

    V_VT(&vIndex) = VT_I4;
    V_I4(&vIndex) = 1;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child2 != NULL, "Expected not NULL child\n");
    IXMLElementCollection_Release(collection);
    IXMLElement_Release(child2);

    /* add element->child->child2 structure, then remove child2 from node */
    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_TEXT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &child2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child2 != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_addChild(child, child2, 0, -1);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXMLElement_removeChild(element, child2);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXMLElement_removeChild(child, child2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    hr = IXMLElement_removeChild(child, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    IXMLElement_Release(element);
    IXMLElement_Release(child);
    IXMLElement_Release(child2);
    IXMLDocument_Release(doc);
}

static void test_xmlelem_collection(void)
{
    HRESULT hr;
    IUnknown *unk = NULL;
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL, *child;
    IXMLElementCollection *collection = NULL;
    IEnumVARIANT *enumVar = NULL;
    CHAR pathA[MAX_PATH];
    WCHAR path[MAX_PATH];
    LONG length, type;
    ULONG num_vars;
    VARIANT var, dummy, vIndex, vName;
    BSTR url, str;
    static const CHAR szBankXML[] = "bank.xml";
    static const WCHAR szNumber[] = {'N','U','M','B','E','R',0};
    static const WCHAR szName[] = {'N','A','M','E',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);

    create_xml_file(szBankXML);
    GetFullPathNameA(szBankXML, MAX_PATH, pathA, NULL);
    MultiByteToWideChar(CP_ACP, 0, pathA, -1, path, MAX_PATH);

    url = SysAllocString(path);
    hr = IXMLDocument_put_URL(doc, url);
    ok(hr == S_OK, "Expected S_OK, got 0x%08x\n", hr);
    SysFreeString(url);

    if(hr != S_OK)
        goto cleanup;

    hr = IXMLDocument_get_root(doc, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

    hr = IXMLElement_get_children(element, &collection);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(collection != NULL, "Expected non-NULL collection\n");

    hr = IXMLElementCollection_get_length(collection, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);

    hr = IXMLElementCollection_get_length(collection, &length);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(length == 2, "Expected 2, got %d\n", length);

    /* IXMLElementCollection:put_length does nothing */
    hr = IXMLElementCollection_put_length(collection, -1);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);

    hr = IXMLElementCollection_put_length(collection, 0);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);

    hr = IXMLElementCollection_put_length(collection, 1);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);

    hr = IXMLElementCollection_put_length(collection, 2);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);

    hr = IXMLElementCollection_put_length(collection, 3);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);

    hr = IXMLElementCollection_put_length(collection, 50);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);

    /* make sure the length hasn't changed */
    hr = IXMLElementCollection_get_length(collection, &length);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(length == 2, "Expected 2, got %d\n", length);

    /* IXMLElementCollection implements IEnumVARIANT */
    hr = IXMLElementCollection_QueryInterface(collection, &IID_IEnumVARIANT, (LPVOID *)&enumVar);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(enumVar != NULL, "Expected non-NULL enumVar\n");
    IEnumVARIANT_Release(enumVar);

    hr = IXMLElementCollection_get__newEnum(collection, &unk);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(unk != NULL, "Expected non-NULL unk\n");

    enumVar = (void *)0xdeadbeef;
    hr = IUnknown_QueryInterface(unk, &IID_IXMLElementCollection, (LPVOID *)&enumVar);
    ok(hr == E_NOINTERFACE, "Expected E_NOINTERFACE, got %08x\n", hr);
    ok(enumVar == NULL || broken(enumVar == (void *)0xdeadbeef) /* XP */, "Expected NULL, got %p\n", enumVar);

    hr = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (LPVOID *)&enumVar);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(enumVar != NULL, "Expected non-NULL enumVar\n");
    IUnknown_Release(unk);

    /* <Number>1234</Number> */
    hr = IEnumVARIANT_Next(enumVar, 1, &var, &num_vars);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(V_VT(&var) == VT_DISPATCH, "Expected VT_DISPATCH, got %d\n", V_VT(&var));
    ok(num_vars == 1, "Expected 1, got %d\n", num_vars);

    hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IXMLElement, (LPVOID *)&child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    VariantClear(&var);

    hr = IXMLElement_get_type(child, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(child, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, szNumber), "Expected NUMBER\n");
    SysFreeString(str);
    IXMLElement_Release(child);

    /* <Name>Captain Ahab</Name> */
    hr = IEnumVARIANT_Next(enumVar, 1, &var, &num_vars);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(V_VT(&var) == VT_DISPATCH, "Expected VT_DISPATCH, got %d\n", V_VT(&var));
    ok(num_vars == 1, "Expected 1, got %d\n", num_vars);

    /* try advance further, no children left */
    V_VT(&dummy) = VT_I4;
    hr = IEnumVARIANT_Next(enumVar, 1, &dummy, &num_vars);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(V_VT(&dummy) == VT_EMPTY, "Expected 0, got %d\n", V_VT(&dummy));
    ok(num_vars == 0, "Expected 0, got %d\n", num_vars);

    V_VT(&dummy) = VT_I4;
    hr = IEnumVARIANT_Next(enumVar, 1, &dummy, NULL);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(V_VT(&dummy) == VT_EMPTY, "Expected 0, got %d\n", V_VT(&dummy));

    hr = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IXMLElement, (LPVOID *)&child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    VariantClear(&var);

    hr = IXMLElement_get_type(child, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(child, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, szName), "Expected NAME\n");
    SysFreeString(str);
    IXMLElement_Release(child);

    /* <Number>1234</Number> */
    V_VT(&vIndex) = VT_I4;
    V_I4(&vIndex) = 0;
    V_VT(&vName) = VT_ERROR;
    V_ERROR(&vName) = DISP_E_PARAMNOTFOUND;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_get_type(child, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(child, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, szNumber), "Expected NUMBER\n");
    SysFreeString(str);
    IXMLElement_Release(child);

    /* <Name>Captain Ahab</Name> */
    V_VT(&vIndex) = VT_I4;
    V_I4(&vIndex) = 1;
    V_VT(&vName) = VT_ERROR;
    V_ERROR(&vName) = DISP_E_PARAMNOTFOUND;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_get_type(child, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_tagName(child, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, szName), "Expected NAME\n");
    SysFreeString(str);
    IXMLElement_Release(child);

    V_I4(&vIndex) = 100;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child);
    ok(hr == E_FAIL, "Expected E_FAIL, got %08x\n", hr);
    ok(child == NULL, "Expected NULL child\n");

    V_I4(&vIndex) = -1;
    child = (IXMLElement *)0xdeadbeef;
    hr = IXMLElementCollection_item(collection, vIndex, vName, (IDispatch **)&child);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %08x\n", hr);
    ok(child == NULL, "Expected NULL child\n");

    IEnumVARIANT_Release(enumVar);
    IXMLElement_Release(element);
    IXMLElementCollection_Release(collection);
cleanup:
    IXMLDocument_Release(doc);
    DeleteFileA("bank.xml");
}

static void test_xmlelem(void)
{
    HRESULT hr;
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL, *parent;
    IXMLElement *child, *child2;
    IXMLElementCollection *children;
    VARIANT vType, vName;
    VARIANT vIndex, vValue;
    BSTR str, val, name;
    LONG type, num_child;
    IDispatch *disp;
    ITypeInfo *ti;

    static const WCHAR propName[] = {'p','r','o','p',0};
    static const WCHAR propVal[] = {'v','a','l',0};
    static const WCHAR nextVal[] = {'n','e','x','t',0};
    static const WCHAR noexist[] = {'n','o','e','x','i','s','t',0};
    static const WCHAR crazyCase1[] = {'C','R','a','z','Y','c','A','S','E',0};
    static const WCHAR crazyCase2[] = {'C','R','A','Z','Y','C','A','S','E',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    EXPECT_HR(hr, S_OK);

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_ELEMENT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    EXPECT_HR(hr, S_OK);
    ok(element != NULL, "Expected non-NULL element\n");

    /* test for IDispatch */
    disp = NULL;
    hr = IXMLElement_QueryInterface(element, &IID_IDispatch, (void**)&disp);
    EXPECT_HR(hr, S_OK);

    hr = IDispatch_GetTypeInfo(disp, 0, 0, &ti);
    EXPECT_HR(hr, S_OK);

    name = NULL;
    hr = ITypeInfo_GetDocumentation(ti, DISPID_XMLELEMENT_TAGNAME, &name, NULL, NULL, NULL);
    EXPECT_HR(hr, S_OK);
    SysFreeString(name);

    ITypeInfo_Release(ti);
    IDispatch_Release(disp);

    hr = IXMLElement_get_tagName(element, &str);
    EXPECT_HR(hr, S_OK);
    ok(!str, "Expected empty tag name, got %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    parent = (IXMLElement *)0xdeadbeef;
    hr = IXMLElement_get_parent(element, &parent);
    ok(hr == 1, "Expected 1, got %08x\n", hr);
    ok(parent == NULL, "Expected NULL parent\n");

    str = SysAllocString(noexist);
    hr = IXMLElement_getAttribute(element, str, &vValue);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);
    ok(V_VT(&vValue) == VT_EMPTY, "Expected VT_EMPTY, got %d\n", V_VT(&vValue));
    ok(V_BSTR(&vValue) == NULL, "Expected null value\n");
    VariantClear(&vValue);
    SysFreeString(str);

    str = SysAllocString(crazyCase1);
    val = SysAllocString(propVal);
    V_VT(&vValue) = VT_BSTR;
    V_BSTR(&vValue) = val;
    hr = IXMLElement_setAttribute(element, str, vValue);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    SysFreeString(str);
    SysFreeString(val);

    str = SysAllocString(crazyCase2);
    hr = IXMLElement_getAttribute(element, str, &vValue);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(V_VT(&vValue) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&vValue));
    ok(!lstrcmpW(V_BSTR(&vValue), propVal), "Expected 'val'\n");
    VariantClear(&vValue);
    SysFreeString(str);

    str = SysAllocString(propName);
    val = SysAllocString(propVal);
    V_VT(&vValue) = VT_BSTR;
    V_BSTR(&vValue) = val;
    hr = IXMLElement_setAttribute(element, str, vValue);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    SysFreeString(val);

    hr = IXMLElement_getAttribute(element, str, &vValue);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(V_VT(&vValue) == VT_BSTR, "Expected VT_BSTR, got %d\n", V_VT(&vValue));
    ok(!lstrcmpW(V_BSTR(&vValue), propVal), "Expected 'val'\n");
    VariantClear(&vValue);

    hr = IXMLElement_removeAttribute(element, str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    /* remove now nonexistent attribute */
    hr = IXMLElement_removeAttribute(element, str);
    ok(hr == S_FALSE, "Expected S_FALSE, got %08x\n", hr);

    hr = IXMLElement_getAttribute(element, str, &vValue);
    ok(hr == 1, "Expected 1, got %08x\n", hr);
    ok(V_VT(&vValue) == VT_EMPTY, "Expected VT_EMPTY, got %d\n", V_VT(&vValue));
    ok(V_BSTR(&vValue) == NULL, "Expected null value\n");
    SysFreeString(str);
    VariantClear(&vValue);

    children = (IXMLElementCollection *)0xdeadbeef;
    hr = IXMLElement_get_children(element, &children);
    ok(hr == 1, "Expected 1, got %08x\n", hr);
    ok(children == NULL, "Expected NULL collection\n");

    hr = IXMLElement_get_type(element, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    hr = IXMLElement_get_text(element, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lstrlenW(str) == 0, "Expected empty text\n");
    SysFreeString(str);

    /* put_text with an ELEMENT */
    str = SysAllocString(propVal);
    hr = IXMLElement_put_text(element, str);
    ok(hr == E_NOTIMPL, "Expected E_NOTIMPL, got %08x\n", hr);
    SysFreeString(str);

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_TEXT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_addChild(element, child, 0, -1);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);

    str = SysAllocString(propVal);
    hr = IXMLElement_put_text(child, str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    SysFreeString(str);

    parent = (IXMLElement *)0xdeadbeef;
    hr = IXMLElement_get_parent(child, &parent);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(parent != element, "Expected parent != element\n");

    hr = IXMLElement_get_type(parent, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_ELEMENT, "Expected XMLELEMTYPE_ELEMENT, got %d\n", type);

    children = (IXMLElementCollection *)0xdeadbeef;
    hr = IXMLElement_get_children(element, &children);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(children != NULL, "Expected non-NULL collection\n");

    hr = IXMLElementCollection_get_length(children, &num_child);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(num_child == 1, "Expected 1, got %d\n", num_child);

    V_VT(&vIndex) = VT_I4;
    V_I4(&vIndex) = 0;
    V_VT(&vName) = VT_ERROR;
    V_ERROR(&vName) = DISP_E_PARAMNOTFOUND;
    hr = IXMLElementCollection_item(children, vIndex, vName, (IDispatch **)&child2);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(child2 != NULL, "Expected non-NULL child\n");

    hr = IXMLElement_get_type(child2, &type);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(type == XMLELEMTYPE_TEXT, "Expected XMLELEMTYPE_TEXT, got %d\n", type);

    hr = IXMLElement_get_text(element, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, propVal), "Expected 'val'\n");
    SysFreeString(str);

    hr = IXMLElement_get_text(child2, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, propVal), "Expected 'val'\n");
    SysFreeString(str);

    /* try put_text on ELEMENT again, now that it has a text child */
    str = SysAllocString(nextVal);
    hr = IXMLElement_put_text(element, str);
    ok(hr == E_NOTIMPL, "Expected E_NOTIMPL, got %08x\n", hr);
    SysFreeString(str);

    str = SysAllocString(nextVal);
    hr = IXMLElement_put_text(child2, str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    SysFreeString(str);

    hr = IXMLElement_get_text(element, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(!lstrcmpW(str, nextVal), "Expected 'val'\n");
    SysFreeString(str);

    IXMLElement_Release(child2);
    IXMLElementCollection_Release(children);
    IXMLElement_Release(parent);
    IXMLElement_Release(child);
    IXMLElement_Release(element);
    IXMLDocument_Release(doc);
}

START_TEST(xmldoc)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    if (!test_try_xmldoc())
    {
        CoUninitialize();
        return;
    }

    test_xmldoc();
    test_createElement();
    test_persiststreaminit();
    test_xmlelem();
    test_xmlelem_collection();
    test_xmlelem_children();

    CoUninitialize();
}
