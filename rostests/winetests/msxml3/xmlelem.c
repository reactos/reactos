/*
 * IXMLElement tests
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
#include "xmldom.h"
#include "msxml2.h"
#include "ocidl.h"

#include "wine/test.h"

#define ERROR_URL_NOT_FOUND 0x800c0006

static void test_xmlelem(void)
{
    HRESULT hr;
    IXMLDocument *doc = NULL;
    IXMLElement *element = NULL, *parent;
    IXMLElement *child, *child2;
    IXMLElementCollection *children;
    VARIANT vType, vName;
    VARIANT vIndex, vValue;
    BSTR str, val;
    LONG type, num_child;

    static const WCHAR propName[] = {'p','r','o','p',0};
    static const WCHAR propVal[] = {'v','a','l',0};
    static const WCHAR nextVal[] = {'n','e','x','t',0};
    static const WCHAR noexist[] = {'n','o','e','x','i','s','t',0};
    static const WCHAR crazyCase1[] = {'C','R','a','z','Y','c','A','S','E',0};
    static const WCHAR crazyCase2[] = {'C','R','A','Z','Y','C','A','S','E',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    if (FAILED(hr))
    {
        skip("Failed to create XMLDocument instance\n");
        return;
    }

    V_VT(&vType) = VT_I4;
    V_I4(&vType) = XMLELEMTYPE_ELEMENT;
    V_VT(&vName) = VT_NULL;
    hr = IXMLDocument_createElement(doc, vType, vName, &element);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(element != NULL, "Expected non-NULL element\n");

    hr = IXMLElement_get_tagName(element, &str);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(lstrlenW(str) == 0, "Expected empty tag name\n");
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
        "</BankAccount>\n";

    WriteFile(hf, data, sizeof(data) - 1, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
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
    VARIANT var, vIndex, vName;
    BSTR url, str;
    static const CHAR szBankXML[] = "bank.xml";
    static const WCHAR szNumber[] = {'N','U','M','B','E','R',0};
    static const WCHAR szName[] = {'N','A','M','E',0};

    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IXMLDocument, (LPVOID*)&doc);
    if (FAILED(hr))
    {
        skip("Failed to create XMLDocument instance\n");
        return;
    }

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

    hr = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (LPVOID *)&enumVar);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(enumVar != NULL, "Expected non-NULL enumVar\n");
    IUnknown_Release(unk);

    /* <Number>1234</Number> */
    hr = IEnumVARIANT_Next(enumVar, 1, &var, &num_vars);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    ok(V_VT(&var) == VT_DISPATCH, "Expected VT_DISPATCH, got %d\n", V_VT(&var));
    ok(num_vars == 1, "Expected 1, got %d\n", num_vars);

    hr = IUnknown_QueryInterface(V_DISPATCH(&var), &IID_IXMLElement, (LPVOID *)&child);
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

    hr = IUnknown_QueryInterface(V_DISPATCH(&var), &IID_IXMLElement, (LPVOID *)&child);
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

START_TEST(xmlelem)
{
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(hr == S_OK, "failed to init com\n");

    test_xmlelem();
    test_xmlelem_collection();

    CoUninitialize();
}
