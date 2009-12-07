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
#include "ocidl.h"

#include "wine/test.h"

/* Deprecated Error Code */
#define XML_E_INVALIDATROOTLEVEL    0xc00ce556

static void create_xml_file(LPCSTR filename)
{
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    static const char data[] =
        "<?xml version=\"1.0\" ?>\n"
        "<BankAccount>\n"
        "  <Number>1234</Number>\n"
        "  <Name>Captain Ahab</Name>\n"
        "</BankAccount>";

    WriteFile(hf, data, sizeof(data) - 1, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void test_xmldoc(void)
{
    HRESULT hr;
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL, *child = NULL, *value = NULL;
    IXMLElementCollection *collection = NULL, *inner = NULL;
    IPersistStreamInit *psi = NULL;
    IStream *stream = NULL;
    HGLOBAL hglobal;
    HANDLE hfile;
    LPVOID ptr;
    DWORD file_size, read;
    CHAR path[MAX_PATH];
    LONG type, num_child;
    VARIANT vIndex, vName;
    BSTR name = NULL;

    static const WCHAR szBankAccount[] = {'B','A','N','K','A','C','C','O','U','N','T',0};
    static const WCHAR szNumber[] = {'N','U','M','B','E','R',0};
    static const WCHAR szNumVal[] = {'1','2','3','4',0};
    static const WCHAR szName[] = {'N','A','M','E',0};
    static const WCHAR szNameVal[] = {'C','a','p','t','a','i','n',' ','A','h','a','b',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    if (FAILED(hr))
    {
        skip("Failed to create XMLDocument instance\n");
        return;
    }

    create_xml_file("bank.xml");
    GetFullPathNameA("bank.xml", MAX_PATH, path, NULL);

    hfile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "Expected a valid file handle\n");
    file_size = GetFileSize(hfile, NULL);

    hglobal = GlobalAlloc(GHND, file_size);
    ptr = GlobalLock(hglobal);

    ReadFile(hfile, ptr, file_size, &read, NULL);
    ok(file_size == read, "Expected to read the whole file, read %d\n", read);

    hr = CreateStreamOnHGlobal(hglobal, TRUE, &stream);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(stream != NULL, "Expected non-NULL stream\n");

    CloseHandle(hfile);
    GlobalUnlock(hglobal);

    hr = IXMLDocument_QueryInterface(doc, &IID_IPersistStreamInit, (LPVOID *)&psi);
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

    hr = IXMLDocument_get_root(doc, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

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
    if (FAILED(hr))
    {
        skip("Failed to create XMLDocument instance\n");
        return;
    }

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

START_TEST(xmldoc)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    test_xmldoc();
    test_createElement();

    CoUninitialize();
}
