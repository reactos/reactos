/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include <wine/test.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "mshtml.h"
#include "mshtmcid.h"
#include "mshtmhst.h"
#include "docobj.h"
#include "dispex.h"

static const char doc_blank[] = "<html></html>";
static const char doc_str1[] = "<html><body>test</body></html>";
static const char range_test_str[] =
    "<html><body>test \na<font size=\"2\">bc\t123<br /> it's\r\n  \t</font>text<br /></body></html>";
static const char range_test2_str[] =
    "<html><body>abc<hr />123<br /><hr />def</body></html>";
static const char elem_test_str[] =
    "<html><head><title>test</title><style>.body { margin-right: 0px; }</style>"
    "<body>text test<!-- a comment -->"
    "<a href=\"http://test\" name=\"x\">link</a>"
    "<input id=\"in\" class=\"testclass\" tabIndex=\"2\" title=\"test title\" />"
    "<select id=\"s\"><option id=\"x\" value=\"val1\">opt1</option><option id=\"y\">opt2</option></select>"
    "<textarea id=\"X\">text text</textarea>"
    "<table><tbody></tbody></table>"
    "<script id=\"sc\" type=\"text/javascript\"></script>"
    "<test />"
    "<img id=\"imgid\"/>"
    "</body></html>";
static const char indent_test_str[] =
    "<html><head><title>test</title></head><body>abc<br /><a href=\"about:blank\">123</a></body></html>";

static const WCHAR noneW[] = {'N','o','n','e',0};

static WCHAR characterW[] = {'c','h','a','r','a','c','t','e','r',0};
static WCHAR texteditW[] = {'t','e','x','t','e','d','i','t',0};
static WCHAR wordW[] = {'w','o','r','d',0};

static const WCHAR text_javascriptW[] = {'t','e','x','t','/','j','a','v','a','s','c','r','i','p','t',0};

static const WCHAR idW[] = {'i','d',0};

typedef enum {
    ET_NONE,
    ET_HTML,
    ET_HEAD,
    ET_TITLE,
    ET_BODY,
    ET_A,
    ET_INPUT,
    ET_SELECT,
    ET_TEXTAREA,
    ET_OPTION,
    ET_STYLE,
    ET_BLOCKQUOTE,
    ET_P,
    ET_BR,
    ET_TABLE,
    ET_TBODY,
    ET_SCRIPT,
    ET_TEST,
    ET_TESTG,
    ET_COMMENT,
    ET_IMG
} elem_type_t;

static REFIID const none_iids[] = {
    &IID_IUnknown,
    NULL
};

static REFIID const elem_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const body_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLTextContainer,
    &IID_IHTMLBodyElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const anchor_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLAnchorElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const input_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLInputElement,
    &IID_IHTMLInputTextElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const select_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLSelectElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const textarea_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLTextAreaElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const option_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLOptionElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const table_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLTable,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const script_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLScriptElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const text_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLDOMTextNode,
    NULL
};

static REFIID const location_iids[] = {
    &IID_IDispatch,
    &IID_IHTMLLocation,
    NULL
};

static REFIID const window_iids[] = {
    &IID_IDispatch,
    &IID_IHTMLWindow2,
    &IID_IHTMLWindow3,
    &IID_IDispatchEx,
    NULL
};

static REFIID const comment_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLCommentElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const img_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IDispatchEx,
    &IID_IHTMLImgElement,
    &IID_IConnectionPointContainer,
    NULL
};

static REFIID const generic_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLElement,
    &IID_IHTMLElement2,
    &IID_IHTMLGenericElement,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    NULL
};

typedef struct {
    const char *tag;
    REFIID *iids;
    const IID *dispiid;
} elem_type_info_t;

static const elem_type_info_t elem_type_infos[] = {
    {"",          none_iids,        NULL},
    {"HTML",      elem_iids,        NULL},
    {"HEAD",      elem_iids,        NULL},
    {"TITLE",     elem_iids,        NULL},
    {"BODY",      body_iids,        NULL},
    {"A",         anchor_iids,      NULL},
    {"INPUT",     input_iids,       &DIID_DispHTMLInputElement},
    {"SELECT",    select_iids,      &DIID_DispHTMLSelectElement},
    {"TEXTAREA",  textarea_iids,    NULL},
    {"OPTION",    option_iids,      &DIID_DispHTMLOptionElement},
    {"STYLE",     elem_iids,        NULL},
    {"BLOCKQUOTE",elem_iids,        NULL},
    {"P",         elem_iids,        NULL},
    {"BR",        elem_iids,        NULL},
    {"TABLE",     table_iids,       NULL},
    {"TBODY",     elem_iids,        NULL},
    {"SCRIPT",    script_iids,      NULL},
    {"TEST",      elem_iids,        &DIID_DispHTMLUnknownElement},
    {"TEST",      generic_iids,     &DIID_DispHTMLGenericElement},
    {"!",         comment_iids,     &DIID_DispHTMLCommentElement},
    {"IMG",       img_iids,         &DIID_DispHTMLImg}
};

static const char *dbgstr_w(LPCWSTR str)
{
    static char buf[512];
    if(!str)
        return "(null)";
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), NULL, NULL);
    return buf;
}

static const char *dbgstr_guid(REFIID riid)
{
    static char buf[50];

    sprintf(buf, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            riid->Data1, riid->Data2, riid->Data3, riid->Data4[0],
            riid->Data4[1], riid->Data4[2], riid->Data4[3], riid->Data4[4],
            riid->Data4[5], riid->Data4[6], riid->Data4[7]);

    return buf;
}

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    WCHAR buf[512];
    MultiByteToWideChar(CP_ACP, 0, stra, -1, buf, sizeof(buf)/sizeof(WCHAR));
    return lstrcmpW(strw, buf);
}

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);

    return doc;
}

#define test_ifaces(i,ids) _test_ifaces(__LINE__,i,ids)
static void _test_ifaces(unsigned line, IUnknown *iface, REFIID *iids)
{
    const IID * const *piid;
    IUnknown *unk;
    HRESULT hres;

     for(piid = iids; *piid; piid++) {
        hres = IDispatch_QueryInterface(iface, *piid, (void**)&unk);
        ok_(__FILE__,line) (hres == S_OK, "Could not get %s interface: %08x\n", dbgstr_guid(*piid), hres);
        if(SUCCEEDED(hres))
            IUnknown_Release(unk);
    }
}

#define test_disp(u,id) _test_disp(__LINE__,u,id)
static void _test_disp(unsigned line, IUnknown *unk, const IID *diid)
{
    IDispatchEx *dispex;
    ITypeInfo *typeinfo;
    UINT ticnt;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IDispatch: %08x\n", hres);
    if(FAILED(hres))
        return;

    ticnt = 0xdeadbeef;
    hres = IDispatchEx_GetTypeInfoCount(dispex, &ticnt);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfoCount failed: %08x\n", hres);
    ok_(__FILE__,line) (ticnt == 1, "ticnt=%u\n", ticnt);

    hres = IDispatchEx_GetTypeInfo(dispex, 0, 0, &typeinfo);
    ok_(__FILE__,line) (hres == S_OK, "GetTypeInfo failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        TYPEATTR *type_attr;

        hres = ITypeInfo_GetTypeAttr(typeinfo, &type_attr);
        ok_(__FILE__,line) (hres == S_OK, "GetTypeAttr failed: %08x\n", hres);
        ok_(__FILE__,line) (IsEqualGUID(&type_attr->guid, diid), "unexpected guid %s\n", dbgstr_guid(&type_attr->guid));

        ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
        ITypeInfo_Release(typeinfo);
    }

    IDispatchEx_Release(dispex);
}

#define get_elem_iface(u) _get_elem_iface(__LINE__,u)
static IHTMLElement *_get_elem_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Coule not get IHTMLElement: %08x\n", hres);
    return elem;
}

#define get_elem2_iface(u) _get_elem2_iface(__LINE__,u)
static IHTMLElement2 *_get_elem2_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement2, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Coule not get IHTMLElement2: %08x\n", hres);
    return elem;
}

#define get_node_iface(u) _get_node_iface(__LINE__,u)
static IHTMLDOMNode *_get_node_iface(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMNode, (void**)&node);
    ok_(__FILE__,line) (hres == S_OK, "Coule not get IHTMLDOMNode: %08x\n", hres);
    return node;
}

#define get_img_iface(u) _get_img_iface(__LINE__,u)
static IHTMLImgElement *_get_img_iface(unsigned line, IUnknown *unk)
{
    IHTMLImgElement *img;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLImgElement, (void**)&img);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLImgElement: %08x\n", hres);
    return img;
}

#define test_node_name(u,n) _test_node_name(__LINE__,u,n)
static void _test_node_name(unsigned line, IUnknown *unk, const char *exname)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    BSTR name;
    HRESULT hres;

    hres = IHTMLDOMNode_get_nodeName(node, &name);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__, line) (hres == S_OK, "get_nodeName failed: %08x\n", hres);
    ok_(__FILE__, line) (!strcmp_wa(name, exname), "got name: %s, expected %s\n", dbgstr_w(name), exname);

    SysFreeString(name);
}

#define test_elem_tag(u,n) _test_elem_tag(__LINE__,u,n)
static void _test_elem_tag(unsigned line, IUnknown *unk, const char *extag)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR tag;
    HRESULT hres;

    hres = IHTMLElement_get_tagName(elem, &tag);
    IHTMLElement_Release(elem);
    ok_(__FILE__, line) (hres == S_OK, "get_tagName failed: %08x\n", hres);
    ok_(__FILE__, line) (!strcmp_wa(tag, extag), "got tag: %s, expected %s\n", dbgstr_w(tag), extag);

    SysFreeString(tag);
}

#define test_elem_type(ifc,t) _test_elem_type(__LINE__,ifc,t)
static void _test_elem_type(unsigned line, IUnknown *unk, elem_type_t type)
{
    _test_elem_tag(line, unk, elem_type_infos[type].tag);
    _test_ifaces(line, unk, elem_type_infos[type].iids);

    if(elem_type_infos[type].dispiid)
        _test_disp(line, unk, elem_type_infos[type].dispiid);
}

#define test_elem_attr(e,n,v) _test_elem_attr(__LINE__,e,n,v)
static void _test_elem_attr(unsigned line, IHTMLElement *elem, LPCWSTR name, LPCWSTR exval)
{
    VARIANT value;
    BSTR tmp;
    HRESULT hres;

    VariantInit(&value);

    tmp = SysAllocString(name);
    hres = IHTMLElement_getAttribute(elem, tmp, 0, &value);
    SysFreeString(tmp);
    ok_(__FILE__,line) (hres == S_OK, "getAttribute failed: %08x\n", hres);

    if(exval) {
        ok_(__FILE__,line) (V_VT(&value) == VT_BSTR, "vt=%d\n", V_VT(&value));
        ok_(__FILE__,line) (!lstrcmpW(exval, V_BSTR(&value)), "unexpected value %s\n", dbgstr_w(V_BSTR(&value)));
    }else {
        ok_(__FILE__,line) (V_VT(&value) == VT_NULL, "vt=%d\n", V_VT(&value));
    }

    VariantClear(&value);
}

#define test_elem_offset(u) _test_elem_offset(__LINE__,u)
static void _test_elem_offset(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    long l;
    HRESULT hres;

    hres = IHTMLElement_get_offsetTop(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_offsetTop failed: %08x\n", hres);

    IHTMLElement_Release(elem);
}

static void test_doc_elem(IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument3) failed: %08x\n", hres);

    hres = IHTMLDocument3_get_documentElement(doc3, &elem);
    IHTMLDocument3_Release(doc3);
    ok(hres == S_OK, "get_documentElement failed: %08x\n", hres);

    test_node_name((IUnknown*)elem, "HTML");
    test_elem_tag((IUnknown*)elem, "HTML");

    IHTMLElement_Release(elem);
}

#define get_doc_elem(d) _get_doc_elem(__LINE__,d)
static IHTMLElement *_get_doc_elem(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDocument3 interface: %08x\n", hres);
    hres = IHTMLDocument3_get_documentElement(doc3, &elem);
    ok_(__FILE__,line) (hres == S_OK, "get_documentElement failed: %08x\n", hres);
    IHTMLDocument3_Release(doc3);

    return elem;
}

#define test_option_text(o,t) _test_option_text(__LINE__,o,t)
static void _test_option_text(unsigned line, IHTMLOptionElement *option, const char *text)
{
    BSTR bstr;
    HRESULT hres;

    hres = IHTMLOptionElement_get_text(option, &bstr);
    ok_(__FILE__,line) (hres == S_OK, "get_text failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(bstr, text), "text=%s\n", dbgstr_w(bstr));
    SysFreeString(bstr);
}

#define test_option_put_text(o,t) _test_option_put_text(__LINE__,o,t)
static void _test_option_put_text(unsigned line, IHTMLOptionElement *option, const char *text)
{
    BSTR bstr;
    HRESULT hres;

    bstr = a2bstr(text);
    hres = IHTMLOptionElement_put_text(option, bstr);
    SysFreeString(bstr);
    ok(hres == S_OK, "put_text failed: %08x\n", hres);

    _test_option_text(line, option, text);
}

#define test_option_value(o,t) _test_option_value(__LINE__,o,t)
static void _test_option_value(unsigned line, IHTMLOptionElement *option, const char *value)
{
    BSTR bstr;
    HRESULT hres;

    hres = IHTMLOptionElement_get_value(option, &bstr);
    ok_(__FILE__,line) (hres == S_OK, "get_value failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(bstr, value), "value=%s\n", dbgstr_w(bstr));
    SysFreeString(bstr);
}

#define test_option_put_value(o,t) _test_option_put_value(__LINE__,o,t)
static void _test_option_put_value(unsigned line, IHTMLOptionElement *option, const char *value)
{
    BSTR bstr;
    HRESULT hres;

    bstr = a2bstr(value);
    hres = IHTMLOptionElement_put_value(option, bstr);
    SysFreeString(bstr);
    ok(hres == S_OK, "put_value failed: %08x\n", hres);

    _test_option_value(line, option, value);
}

#define create_option_elem(d,t,v) _create_option_elem(__LINE__,d,t,v)
static IHTMLOptionElement *_create_option_elem(unsigned line, IHTMLDocument2 *doc,
        const char *txt, const char *val)
{
    IHTMLOptionElementFactory *factory;
    IHTMLOptionElement *option;
    IHTMLWindow2 *window;
    VARIANT text, value, empty;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok_(__FILE__,line) (hres == S_OK, "get_parentElement failed: %08x\n", hres);

    hres = IHTMLWindow2_get_Option(window, &factory);
    IHTMLWindow2_Release(window);
    ok_(__FILE__,line) (hres == S_OK, "get_Option failed: %08x\n", hres);

    V_VT(&text) = VT_BSTR;
    V_BSTR(&text) = a2bstr(txt);
    V_VT(&value) = VT_BSTR;
    V_BSTR(&value) = a2bstr(val);
    V_VT(&empty) = VT_EMPTY;

    hres = IHTMLOptionElementFactory_create(factory, text, value, empty, empty, &option);
    ok_(__FILE__,line) (hres == S_OK, "create failed: %08x\n", hres);

    IHTMLOptionElementFactory_Release(factory);
    VariantClear(&text);
    VariantClear(&value);

    _test_option_text(line, option, txt);
    _test_option_value(line, option, val);

    return option;
}

#define test_select_length(s,l) _test_select_length(__LINE__,s,l)
static void _test_select_length(unsigned line, IHTMLSelectElement *select, long length)
{
    long len = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLSelectElement_get_length(select, &len);
    ok_(__FILE__,line) (hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line) (len == length, "len=%ld, expected %ld\n", len, length);
}

#define test_select_selidx(s,i) _test_select_selidx(__LINE__,s,i)
static void _test_select_selidx(unsigned line, IHTMLSelectElement *select, long index)
{
    long idx = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLSelectElement_get_selectedIndex(select, &idx);
    ok_(__FILE__,line) (hres == S_OK, "get_selectedIndex failed: %08x\n", hres);
    ok_(__FILE__,line) (idx == index, "idx=%ld, expected %ld\n", idx, index);
}

#define test_select_put_selidx(s,i) _test_select_put_selidx(__LINE__,s,i)
static void _test_select_put_selidx(unsigned line, IHTMLSelectElement *select, long index)
{
    HRESULT hres;

    hres = IHTMLSelectElement_put_selectedIndex(select, index);
    ok_(__FILE__,line) (hres == S_OK, "get_selectedIndex failed: %08x\n", hres);
    _test_select_selidx(line, select, index);
}

#define test_select_value(s,v) _test_select_value(__LINE__,s,v)
static void _test_select_value(unsigned line, IHTMLSelectElement *select, const char *exval)
{
    BSTR val;
    HRESULT hres;

    hres = IHTMLSelectElement_get_value(select, &val);
    ok_(__FILE__,line) (hres == S_OK, "get_value failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line) (!strcmp_wa(val, exval), "unexpected value %s\n", dbgstr_w(val));
    else
        ok_(__FILE__,line) (val == NULL, "val=%s, expected NULL\n", dbgstr_w(val));
}

#define test_select_set_value(s,v) _test_select_set_value(__LINE__,s,v)
static void _test_select_set_value(unsigned line, IHTMLSelectElement *select, const char *val)
{
    BSTR bstr;
    HRESULT hres;

    bstr = a2bstr(val);
    hres = IHTMLSelectElement_put_value(select, bstr);
    SysFreeString(bstr);
    ok_(__FILE__,line) (hres == S_OK, "put_value failed: %08x\n", hres);
}

#define test_select_type(s,t) _test_select_type(__LINE__,s,t)
static void _test_select_type(unsigned line, IHTMLSelectElement *select, const char *extype)
{
    BSTR type;
    HRESULT hres;

    hres = IHTMLSelectElement_get_type(select, &type);
    ok_(__FILE__,line) (hres == S_OK, "get_type failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(type, extype), "type=%s, expected %s\n", dbgstr_w(type), extype);
}

#define test_range_text(r,t) _test_range_text(__LINE__,r,t)
static void _test_range_text(unsigned line, IHTMLTxtRange *range, const char *extext)
{
    BSTR text;
    HRESULT hres;

    hres = IHTMLTxtRange_get_text(range, &text);
    ok_(__FILE__, line) (hres == S_OK, "get_text failed: %08x\n", hres);

    if(extext) {
        ok_(__FILE__, line) (text != NULL, "text == NULL\n");
        ok_(__FILE__, line) (!strcmp_wa(text, extext), "text=\"%s\", expected \"%s\"\n", dbgstr_w(text), extext);
    }else {
        ok_(__FILE__, line) (text == NULL, "text=\"%s\", expected NULL\n", dbgstr_w(text));
    }

    SysFreeString(text);

}

#define test_range_collapse(r,b) _test_range_collapse(__LINE__,r,b)
static void _test_range_collapse(unsigned line, IHTMLTxtRange *range, BOOL b)
{
    HRESULT hres;

    hres = IHTMLTxtRange_collapse(range, b);
    ok_(__FILE__, line) (hres == S_OK, "collapse failed: %08x\n", hres);
    _test_range_text(line, range, NULL);
}

#define test_range_expand(r,u,b,t) _test_range_expand(__LINE__,r,u,b,t)
static void _test_range_expand(unsigned line, IHTMLTxtRange *range, LPWSTR unit,
        VARIANT_BOOL exb, const char *extext)
{
    VARIANT_BOOL b = 0xe0e0;
    HRESULT hres;

    hres = IHTMLTxtRange_expand(range, unit, &b);
    ok_(__FILE__,line) (hres == S_OK, "expand failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "b=%x, expected %x\n", b, exb);
    _test_range_text(line, range, extext);
}

#define test_range_move(r,u,c,e) _test_range_move(__LINE__,r,u,c,e)
static void _test_range_move(unsigned line, IHTMLTxtRange *range, LPWSTR unit, long cnt, long excnt)
{
    long c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_move(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%ld, expected %ld\n", c, excnt);
    _test_range_text(line, range, NULL);
}

#define test_range_movestart(r,u,c,e) _test_range_movestart(__LINE__,r,u,c,e)
static void _test_range_movestart(unsigned line, IHTMLTxtRange *range,
        LPWSTR unit, long cnt, long excnt)
{
    long c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_moveStart(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%ld, expected %ld\n", c, excnt);
}

#define test_range_moveend(r,u,c,e) _test_range_moveend(__LINE__,r,u,c,e)
static void _test_range_moveend(unsigned line, IHTMLTxtRange *range, LPWSTR unit, long cnt, long excnt)
{
    long c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_moveEnd(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%ld, expected %ld\n", c, excnt);
}

#define test_range_put_text(r,t) _test_range_put_text(__LINE__,r,t)
static void _test_range_put_text(unsigned line, IHTMLTxtRange *range, const char *text)
{
    HRESULT hres;
    BSTR bstr = a2bstr(text);

    hres = IHTMLTxtRange_put_text(range, bstr);
    ok_(__FILE__,line) (hres == S_OK, "put_text failed: %08x\n", hres);
    SysFreeString(bstr);
    _test_range_text(line, range, NULL);
}

#define test_range_inrange(r1,r2,b) _test_range_inrange(__LINE__,r1,r2,b)
static void _test_range_inrange(unsigned line, IHTMLTxtRange *range1, IHTMLTxtRange *range2, VARIANT_BOOL exb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0xe0e0;
    hres = IHTMLTxtRange_inRange(range1, range2, &b);
    ok_(__FILE__,line) (hres == S_OK, "(1->2) isEqual failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "(1->2) b=%x, expected %x\n", b, exb);
}

#define test_range_isequal(r1,r2,b) _test_range_isequal(__LINE__,r1,r2,b)
static void _test_range_isequal(unsigned line, IHTMLTxtRange *range1, IHTMLTxtRange *range2, VARIANT_BOOL exb)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 0xe0e0;
    hres = IHTMLTxtRange_isEqual(range1, range2, &b);
    ok_(__FILE__,line) (hres == S_OK, "(1->2) isEqual failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "(1->2) b=%x, expected %x\n", b, exb);

    b = 0xe0e0;
    hres = IHTMLTxtRange_isEqual(range2, range1, &b);
    ok_(__FILE__,line) (hres == S_OK, "(2->1) isEqual failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "(2->1) b=%x, expected %x\n", b, exb);

    if(exb) {
        test_range_inrange(range1, range2, VARIANT_TRUE);
        test_range_inrange(range2, range1, VARIANT_TRUE);
    }
}

#define test_range_parent(r,t) _test_range_parent(__LINE__,r,t)
static void _test_range_parent(unsigned line, IHTMLTxtRange *range, elem_type_t type)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLTxtRange_parentElement(range, &elem);
    ok_(__FILE__,line) (hres == S_OK, "parentElement failed: %08x\n", hres);

    _test_elem_type(line, (IUnknown*)elem, type);

    IHTMLElement_Release(elem);
}

#define test_elem_collection(c,t,l) _test_elem_collection(__LINE__,c,t,l)
static void _test_elem_collection(unsigned line, IUnknown *unk,
        const elem_type_t *elem_types, long exlen)
{
    IHTMLElementCollection *col;
    long len;
    DWORD i;
    VARIANT name, index;
    IDispatch *disp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElementCollection, (void**)&col);
    ok(hres == S_OK, "Could not get IHTMLElementCollection: %08x\n", hres);

    test_disp((IUnknown*)col, &DIID_DispHTMLElementCollection);

    hres = IHTMLElementCollection_get_length(col, &len);
    ok_(__FILE__,line) (hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line) (len == exlen, "len=%ld, expected %ld\n", len, exlen);

    if(len > exlen)
        len = exlen;

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_I4;

    for(i=0; i<len; i++) {
        V_I4(&name) = i;
        disp = (void*)0xdeadbeef;
        hres = IHTMLElementCollection_item(col, name, index, &disp);
        ok_(__FILE__,line) (hres == S_OK, "item(%d) failed: %08x\n", i, hres);
        ok_(__FILE__,line) (disp != NULL, "item returned NULL\n");
        if(FAILED(hres) || !disp)
            continue;

        _test_elem_type(line, (IUnknown*)disp, elem_types[i]);
        IDispatch_Release(disp);
    }

    V_I4(&name) = len;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == S_OK, "item failed: %08x\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    V_I4(&name) = -1;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    IHTMLElementCollection_Release(col);
}

#define get_first_child(n) _get_first_child(__LINE__,n)
static IHTMLDOMNode *_get_first_child(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *child = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode_get_firstChild(node, &child);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line) (hres == S_OK, "get_firstChild failed: %08x\n", hres);

    return child;
}

#define test_node_has_child(u,b) _test_node_has_child(__LINE__,u,b)
static void _test_node_has_child(unsigned line, IUnknown *unk, VARIANT_BOOL exb)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    VARIANT_BOOL b = 0xdead;
    HRESULT hres;

    hres = IHTMLDOMNode_hasChildNodes(node, &b);
    ok_(__FILE__,line) (hres == S_OK, "hasChildNodes failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "hasChildNodes=%x, expected %x\n", b, exb);

    IHTMLDOMNode_Release(node);
}

#define test_node_get_parent(u) _test_node_get_parent(__LINE__,u)
static IHTMLDOMNode *_test_node_get_parent(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *parent;
    HRESULT hres;

    hres = IHTMLDOMNode_get_parentNode(node, &parent);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line) (hres == S_OK, "get_parentNode failed: %08x\n", hres);

    return parent;
}

#define get_node_type(n) _get_node_type(__LINE__,n)
static long _get_node_type(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    long type = -1;
    HRESULT hres;

    hres = IHTMLDOMNode_get_nodeType(node, &type);
    ok(hres == S_OK, "get_nodeType failed: %08x\n", hres);

    IHTMLDOMNode_Release(node);

    return type;
}

#define test_img_set_src(u,s) _test_img_set_src(__LINE__,u,s)
static void _test_img_set_src(unsigned line, IUnknown *unk, const char *src)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(src);
    hres = IHTMLImgElement_put_src(img, tmp);
    IHTMLImgElement_Release(img);
    SysFreeString(tmp);
    ok_(__FILE__,line) (hres == S_OK, "put_src failed: %08x\n", hres);
}

#define test_img_alt(u,a) _test_img_alt(__LINE__,u,a)
static void _test_img_alt(unsigned line, IUnknown *unk, const char *exalt)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    BSTR alt;
    HRESULT hres;

    hres = IHTMLImgElement_get_alt(img, &alt);
    ok_(__FILE__,line) (hres == S_OK, "get_alt failed: %08x\n", hres);
    if(exalt)
        ok_(__FILE__,line) (!strcmp_wa(alt, exalt), "inexopected alt %s\n", dbgstr_w(alt));
    else
        ok_(__FILE__,line) (!alt, "alt != NULL\n");
    SysFreeString(alt);
}

#define test_img_set_alt(u,a) _test_img_set_alt(__LINE__,u,a)
static void _test_img_set_alt(unsigned line, IUnknown *unk, const char *alt)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(alt);
    hres = IHTMLImgElement_put_alt(img, tmp);
    ok_(__FILE__,line) (hres == S_OK, "get_alt failed: %08x\n", hres);
    SysFreeString(tmp);

    _test_img_alt(line, unk, alt);
}

#define test_input_get_disabled(i,b) _test_input_get_disabled(__LINE__,i,b)
static void _test_input_get_disabled(unsigned line, IHTMLInputElement *input, VARIANT_BOOL exb)
{
    VARIANT_BOOL disabled = 100;
    HRESULT hres;

    hres = IHTMLInputElement_get_disabled(input, &disabled);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);
    ok_(__FILE__,line) (disabled == exb, "disabled=%x, expected %x\n", disabled, exb);
}

#define test_input_set_disabled(i,b) _test_input_set_disabled(__LINE__,i,b)
static void _test_input_set_disabled(unsigned line, IHTMLInputElement *input, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLInputElement_put_disabled(input, b);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);

    _test_input_get_disabled(line, input, b);
}

#define test_input_value(o,t) _test_input_value(__LINE__,o,t)
static void _test_input_value(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLInputElement *input;
    BSTR bstr;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLInputElement, (void**)&input);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLInputElement_get_value(input, &bstr);
    ok_(__FILE__,line) (hres == S_OK, "get_value failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line) (!strcmp_wa(bstr, exval), "value=%s\n", dbgstr_w(bstr));
    else
        ok_(__FILE__,line) (!exval, "exval != NULL\n");
    SysFreeString(bstr);
    IHTMLInputElement_Release(input);
}

#define test_input_put_value(o,v) _test_input_put_value(__LINE__,o,v)
static void _test_input_put_value(unsigned line, IUnknown *unk, const char *val)
{
    IHTMLInputElement *input;
    BSTR bstr;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLInputElement, (void**)&input);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);
    if(FAILED(hres))
        return;

    bstr = a2bstr(val);
    hres = IHTMLInputElement_get_value(input, &bstr);
    ok_(__FILE__,line) (hres == S_OK, "get_value failed: %08x\n", hres);
    SysFreeString(bstr);
    IHTMLInputElement_Release(input);
}

#define get_child_nodes(u) _get_child_nodes(__LINE__,u)
static IHTMLDOMChildrenCollection *_get_child_nodes(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMChildrenCollection *col = NULL;
    IDispatch *disp;
    HRESULT hres;

    hres = IHTMLDOMNode_get_childNodes(node, &disp);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line) (hres == S_OK, "get_childNodes failed: %08x\n", hres);
    if(FAILED(hres))
        return NULL;

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLDOMChildrenCollection, (void**)&col);
    IDispatch_Release(disp);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDOMChildrenCollection: %08x\n", hres);

    return col;
}

#define test_elem_class(u,c) _test_elem_class(__LINE__,u,c)
static void _test_elem_class(unsigned line, IUnknown *unk, const char *exclass)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR class = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLElement_get_className(elem, &class);
    IHTMLElement_Release(elem);
    ok_(__FILE__,line) (hres == S_OK, "get_className failed: %08x\n", hres);
    if(exclass)
        ok_(__FILE__,line) (!strcmp_wa(class, exclass), "unexpected className %s\n", dbgstr_w(class));
    else
        ok_(__FILE__,line) (!class, "class != NULL\n");
    SysFreeString(class);
}

#define test_elem_tabindex(u,i) _test_elem_tabindex(__LINE__,u,i)
static void _test_elem_tabindex(unsigned line, IUnknown *unk, short exindex)
{
    IHTMLElement2 *elem2 = _get_elem2_iface(line, unk);
    short index = -3;
    HRESULT hres;

    hres = IHTMLElement2_get_tabIndex(elem2, &index);
    IHTMLElement2_Release(elem2);
    ok_(__FILE__,line) (hres == S_OK, "get_tabIndex failed: %08x\n", hres);
    ok_(__FILE__,line) (index == exindex, "unexpected index %d\n", index);
}

#define test_elem_set_tabindex(u,i) _test_elem_set_tabindex(__LINE__,u,i)
static void _test_elem_set_tabindex(unsigned line, IUnknown *unk, short index)
{
    IHTMLElement2 *elem2 = _get_elem2_iface(line, unk);
    HRESULT hres;

    hres = IHTMLElement2_put_tabIndex(elem2, index);
    IHTMLElement2_Release(elem2);
    ok_(__FILE__,line) (hres == S_OK, "get_tabIndex failed: %08x\n", hres);

    _test_elem_tabindex(line, unk, index);
}

#define test_elem_set_class(u,c) _test_elem_set_class(__LINE__,u,c)
static void _test_elem_set_class(unsigned line, IUnknown *unk, const char *class)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR tmp;
    HRESULT hres;

    tmp = class ? a2bstr(class) : NULL;
    hres = IHTMLElement_put_className(elem, tmp);
    IHTMLElement_Release(elem);
    ok_(__FILE__,line) (hres == S_OK, "put_className failed: %08x\n", hres);
    SysFreeString(tmp);

    _test_elem_class(line, unk, class);
}

#define get_child_item(c,i) _get_child_item(__LINE__,c,i)
static IHTMLDOMNode *_get_child_item(unsigned line, IHTMLDOMChildrenCollection *col, long idx)
{
    IHTMLDOMNode *node = NULL;
    IDispatch *disp;
    HRESULT hres;

    hres = IHTMLDOMChildrenCollection_item(col, idx, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);

    node = _get_node_iface(line, (IUnknown*)disp);
    IDispatch_Release(disp);

    return node;
}

#define test_elem_id(e,i) _test_elem_id(__LINE__,e,i)
static void _test_elem_id(unsigned line, IUnknown *unk, const char *exid)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR id = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLElement_get_id(elem, &id);
    IHTMLElement_Release(elem);
    ok_(__FILE__,line) (hres == S_OK, "get_id failed: %08x\n", hres);

    if(exid)
        ok_(__FILE__,line) (!strcmp_wa(id, exid), "unexpected id %s\n", dbgstr_w(id));
    else
        ok_(__FILE__,line) (!id, "id=%s\n", dbgstr_w(id));

    SysFreeString(id);
}

#define test_elem_put_id(u,i) _test_elem_put_id(__LINE__,u,i)
static void _test_elem_put_id(unsigned line, IUnknown *unk, const char *new_id)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR tmp = a2bstr(new_id);
    HRESULT hres;

    hres = IHTMLElement_put_id(elem, tmp);
    IHTMLElement_Release(elem);
    SysFreeString(tmp);
    ok_(__FILE__,line) (hres == S_OK, "put_id failed: %08x\n", hres);

    _test_elem_id(line, unk, new_id);
}

#define test_elem_title(u,t) _test_elem_title(__LINE__,u,t)
static void _test_elem_title(unsigned line, IUnknown *unk, const char *extitle)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR title;
    HRESULT hres;

    hres = IHTMLElement_get_title(elem, &title);
    IHTMLElement_Release(elem);
    ok_(__FILE__,line) (hres == S_OK, "get_title failed: %08x\n", hres);
    if(extitle)
        ok_(__FILE__,line) (!strcmp_wa(title, extitle), "unexpected title %s\n", dbgstr_w(title));
    else
        ok_(__FILE__,line) (!title, "title=%s, expected NULL\n", dbgstr_w(title));

    SysFreeString(title);
}

#define test_elem_set_title(u,t) _test_elem_set_title(__LINE__,u,t)
static void _test_elem_set_title(unsigned line, IUnknown *unk, const char *title)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(title);
    hres = IHTMLElement_put_title(elem, tmp);
    ok_(__FILE__,line) (hres == S_OK, "get_title failed: %08x\n", hres);

    IHTMLElement_Release(elem);
    SysFreeString(tmp);
}

#define test_node_get_value_str(u,e) _test_node_get_value_str(__LINE__,u,e)
static void _test_node_get_value_str(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    VARIANT var;
    HRESULT hres;

    hres = IHTMLDOMNode_get_nodeValue(node, &var);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line) (hres == S_OK, "get_nodeValue failed: %08x, expected VT_BSTR\n", hres);

    if(exval) {
        ok_(__FILE__,line) (V_VT(&var) == VT_BSTR, "vt=%d\n", V_VT(&var));
        ok_(__FILE__,line) (!strcmp_wa(V_BSTR(&var), exval), "unexpected value %s\n", dbgstr_w(V_BSTR(&var)));
    }else {
        ok_(__FILE__,line) (V_VT(&var) == VT_NULL, "vt=%d, expected VT_NULL\n", V_VT(&var));
    }

    VariantClear(&var);
}

#define test_node_put_value_str(u,v) _test_node_put_value_str(__LINE__,u,v)
static void _test_node_put_value_str(unsigned line, IUnknown *unk, const char *val)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    VARIANT var;
    HRESULT hres;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = a2bstr(val);

    hres = IHTMLDOMNode_put_nodeValue(node, var);
    ok_(__FILE__,line) (hres == S_OK, "get_nodeValue failed: %08x, expected VT_BSTR\n", hres);
    IHTMLDOMNode_Release(node);
    VariantClear(&var);
}

#define test_elem_client_size(u) _test_elem_client_size(__LINE__,u)
static void _test_elem_client_size(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    long l;
    HRESULT hres;

    hres = IHTMLElement2_get_clientWidth(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_clientWidth failed: %08x\n", hres);
    hres = IHTMLElement2_get_clientHeight(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_clientHeight failed: %08x\n", hres);

    IHTMLElement2_Release(elem);
}

#define test_create_elem(d,t) _test_create_elem(__LINE__,d,t)
static IHTMLElement *_test_create_elem(unsigned line, IHTMLDocument2 *doc, const char *tag)
{
    IHTMLElement *elem = NULL;
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(tag);
    hres = IHTMLDocument2_createElement(doc, tmp, &elem);
    ok_(__FILE__,line) (hres == S_OK, "createElement failed: %08x\n", hres);
    ok_(__FILE__,line) (elem != NULL, "elem == NULL\n");

    return elem;
}

#define test_create_text(d,t) _test_create_text(__LINE__,d,t)
static IHTMLDOMNode *_test_create_text(unsigned line, IHTMLDocument2 *doc, const char *text)
{
    IHTMLDocument3 *doc3;
    IHTMLDOMNode *node = NULL;
    BSTR tmp;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDocument3: %08x\n", hres);

    tmp = a2bstr(text);
    hres = IHTMLDocument3_createTextNode(doc3, tmp, &node);
    IHTMLDocument3_Release(doc3);
    ok_(__FILE__,line) (hres == S_OK, "createElement failed: %08x\n", hres);
    ok_(__FILE__,line) (node != NULL, "node == NULL\n");

    return node;
}

#define test_node_append_child(n,c) _test_node_append_child(__LINE__,n,c)
static IHTMLDOMNode *_test_node_append_child(unsigned line, IUnknown *node_unk, IUnknown *child_unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, node_unk);
    IHTMLDOMNode *child = _get_node_iface(line, child_unk);
    IHTMLDOMNode *new_child = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode_appendChild(node, child, &new_child);
    ok_(__FILE__,line) (hres == S_OK, "appendChild failed: %08x\n", hres);
    ok_(__FILE__,line) (new_child != NULL, "new_child == NULL\n");
    /* TODO  ok_(__FILE__,line) (new_child != child, "new_child == child\n"); */

    IHTMLDOMNode_Release(node);
    IHTMLDOMNode_Release(child);

    return new_child;
}

#define test_node_remove_child(n,c) _test_node_remove_child(__LINE__,n,c)
static void _test_node_remove_child(unsigned line, IUnknown *unk, IHTMLDOMNode *child)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *new_node = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode_removeChild(node, child, &new_node);
    ok_(__FILE__,line) (hres == S_OK, "appendChild failed: %08x\n", hres);
    ok_(__FILE__,line) (new_node != NULL, "new_node == NULL\n");
    /* TODO ok_(__FILE__,line) (new_node != child, "new_node == child\n"); */

    IHTMLDOMNode_Release(node);
    IHTMLDOMNode_Release(new_node);
}

static void test_elem_col_item(IHTMLElementCollection *col, LPCWSTR n,
        const elem_type_t *elem_types, long len)
{
    IDispatch *disp;
    VARIANT name, index;
    DWORD i;
    HRESULT hres;

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_BSTR;
    V_BSTR(&name) = SysAllocString(n);

    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);

    test_elem_collection((IUnknown*)disp, elem_types, len);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IHTMLElementCollection interface: %08x\n", hres);
    if(hres != S_OK)
        goto cleanup;

    V_VT(&index) = VT_I4;

    for(i=0; i<len; i++) {
        V_I4(&index) = i;
        disp = (void*)0xdeadbeef;
        hres = IHTMLElementCollection_item(col, name, index, &disp);
        ok(hres == S_OK, "item failed: %08x\n", hres);
        ok(disp != NULL, "disp == NULL\n");
        if(FAILED(hres) || !disp)
            continue;

        test_elem_type((IUnknown*)disp, elem_types[i]);

        IDispatch_Release(disp);
    }

    V_I4(&index) = len;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(disp == NULL, "disp != NULL\n");

    V_I4(&index) = -1;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
    ok(disp == NULL, "disp != NULL\n");

cleanup:
    SysFreeString(V_BSTR(&name));
}

static IHTMLElement *get_elem_by_id(IHTMLDocument2 *doc, LPCWSTR id, BOOL expect_success)
{
    IHTMLElementCollection *col;
    IHTMLElement *elem;
    IDispatch *disp = (void*)0xdeadbeef;
    VARIANT name, index;
    HRESULT hres;

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    ok(col != NULL, "col == NULL\n");
    if(FAILED(hres) || !col)
        return NULL;

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_BSTR;
    V_BSTR(&name) = SysAllocString(id);

    hres = IHTMLElementCollection_item(col, name, index, &disp);
    IHTMLElementCollection_Release(col);
    SysFreeString(V_BSTR(&name));
    ok(hres == S_OK, "item failed: %08x\n", hres);
    if(!expect_success) {
        ok(disp == NULL, "disp != NULL\n");
        return NULL;
    }

    ok(disp != NULL, "disp == NULL\n");
    if(!disp)
        return NULL;

    elem = get_elem_iface((IUnknown*)disp);
    IDispatch_Release(disp);

    return elem;
}

static IHTMLElement *get_doc_elem_by_id(IHTMLDocument2 *doc, LPCWSTR id)
{
    IHTMLDocument3 *doc3;
    IHTMLElement *elem;
    BSTR tmp;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);

    tmp = SysAllocString(id);
    hres = IHTMLDocument3_getElementById(doc3, tmp, &elem);
    SysFreeString(tmp);
    ok(hres == S_OK, "getElementById(%s) failed: %08x\n", dbgstr_w(id), hres);

    IHTMLDocument3_Release(doc3);

    return elem;
}

static void test_select_elem(IHTMLSelectElement *select)
{
    test_select_type(select, "select-one");
    test_select_length(select, 2);
    test_select_selidx(select, 0);
    test_select_put_selidx(select, 1);

    test_select_set_value(select, "val1");
    test_select_value(select, "val1");
}

static void test_create_option_elem(IHTMLDocument2 *doc)
{
    IHTMLOptionElement *option;

    option = create_option_elem(doc, "test text", "test value");

    test_option_put_text(option, "new text");
    test_option_put_value(option, "new value");

    IHTMLOptionElement_Release(option);
}

static IHTMLTxtRange *test_create_body_range(IHTMLDocument2 *doc)
{
    IHTMLBodyElement *body;
    IHTMLTxtRange *range;
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLBodyElement, (void**)&body);
    IHTMLElement_Release(elem);

    hres = IHTMLBodyElement_createTextRange(body, &range);
    IHTMLBodyElement_Release(body);
    ok(hres == S_OK, "createTextRange failed: %08x\n", hres);

    return range;
}

static void test_txtrange(IHTMLDocument2 *doc)
{
    IHTMLTxtRange *body_range, *range, *range2;
    IHTMLSelectionObject *selection;
    IDispatch *disp_range;
    HRESULT hres;

    body_range = test_create_body_range(doc);

    test_range_text(body_range, "test abc 123\r\nit's text");

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    hres = IHTMLTxtRange_duplicate(body_range, &range2);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);
    test_range_isequal(range, range2, VARIANT_TRUE);

    test_range_text(range, "test abc 123\r\nit's text");
    test_range_text(body_range, "test abc 123\r\nit's text");

    test_range_collapse(range, TRUE);
    test_range_isequal(range, range2, VARIANT_FALSE);
    test_range_inrange(range, range2, VARIANT_FALSE);
    test_range_inrange(range2, range, VARIANT_TRUE);
    IHTMLTxtRange_Release(range2);

    test_range_expand(range, wordW, VARIANT_TRUE, "test ");
    test_range_expand(range, wordW, VARIANT_FALSE, "test ");
    test_range_move(range, characterW, 2, 2);
    test_range_expand(range, wordW, VARIANT_TRUE, "test ");

    test_range_collapse(range, FALSE);
    test_range_expand(range, wordW, VARIANT_TRUE, "abc ");

    test_range_collapse(range, FALSE);
    test_range_expand(range, wordW, VARIANT_TRUE, "123");
    test_range_expand(range, wordW, VARIANT_FALSE, "123");
    test_range_move(range, characterW, 2, 2);
    test_range_expand(range, wordW, VARIANT_TRUE, "123");
    test_range_moveend(range, characterW, -5, -5);
    test_range_text(range, NULL);
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "c 1");
    test_range_expand(range, texteditW, VARIANT_TRUE, "test abc 123\r\nit's text");
    test_range_collapse(range, TRUE);
    test_range_move(range, characterW, 4, 4);
    test_range_moveend(range, characterW, 1, 1);
    test_range_text(range, " ");
    test_range_move(range, wordW, 1, 1);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "ab");

    IHTMLTxtRange_Release(range);

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    test_range_text(range, "test abc 123\r\nit's text");
    test_range_move(range, characterW, 3, 3);
    test_range_moveend(range, characterW, 1, 1);
    test_range_text(range, "t");
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "t ab");
    test_range_moveend(range, characterW, -2, -2);
    test_range_text(range, "t ");
    test_range_move(range, characterW, 6, 6);
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "123");
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "123\r\ni");

    IHTMLTxtRange_Release(range);

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    test_range_move(range, wordW, 1, 1);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "ab");

    test_range_move(range, characterW, -2, -2);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "t ");

    test_range_move(range, wordW, 3, 3);
    test_range_move(range, wordW, -2, -2);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "ab");

    test_range_move(range, characterW, -6, -5);
    test_range_moveend(range, characterW, -1, 0);
    test_range_moveend(range, characterW, -6, 0);
    test_range_move(range, characterW, 2, 2);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "st");
    test_range_moveend(range, characterW, -6, -4);
    test_range_moveend(range, characterW, 2, 2);

    IHTMLTxtRange_Release(range);

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    test_range_move(range, wordW, 2, 2);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "12");

    test_range_move(range, characterW, 15, 14);
    test_range_move(range, characterW, -2, -2);
    test_range_moveend(range, characterW, 3, 2);
    test_range_text(range, "t");
    test_range_moveend(range, characterW, -1, -1);
    test_range_text(range, "t");
    test_range_expand(range, wordW, VARIANT_TRUE, "text");
    test_range_move(range, characterW, -2, -2);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "s ");
    test_range_move(range, characterW, 100, 7);
    test_range_move(range, wordW, 1, 0);
    test_range_move(range, characterW, -2, -2);
    test_range_moveend(range, characterW, 3, 2);
    test_range_text(range, "t");

    IHTMLTxtRange_Release(range);

    hres = IHTMLTxtRange_duplicate(body_range, &range);
    ok(hres == S_OK, "duplicate failed: %08x\n", hres);

    test_range_collapse(range, TRUE);
    test_range_expand(range, wordW, VARIANT_TRUE, "test ");
    test_range_put_text(range, "word");
    test_range_text(body_range, "wordabc 123\r\nit's text");
    test_range_text(range, NULL);
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "abc");
    test_range_movestart(range, characterW, -2, -2);
    test_range_text(range, "rdabc");
    test_range_movestart(range, characterW, 3, 3);
    test_range_text(range, "bc");
    test_range_movestart(range, characterW, 4, 4);
    test_range_text(range, NULL);
    test_range_movestart(range, characterW, -3, -3);
    test_range_text(range, "c 1");
    test_range_movestart(range, characterW, -7, -6);
    test_range_text(range, "wordabc 1");
    test_range_movestart(range, characterW, 100, 22);
    test_range_text(range, NULL);

    IHTMLTxtRange_Release(range);
    IHTMLTxtRange_Release(body_range);

    hres = IHTMLDocument2_get_selection(doc, &selection);
    ok(hres == S_OK, "IHTMLDocument2_get_selection failed: %08x\n", hres);

    hres = IHTMLSelectionObject_createRange(selection, &disp_range);
    ok(hres == S_OK, "IHTMLSelectionObject_createRange failed: %08x\n", hres);
    IHTMLSelectionObject_Release(selection);

    hres = IDispatch_QueryInterface(disp_range, &IID_IHTMLTxtRange, (void **)&range);
    ok(hres == S_OK, "Could not get IID_IHTMLTxtRange interface: 0x%08x\n", hres);
    IDispatch_Release(disp_range);

    test_range_text(range, NULL);
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "wor");
    test_range_parent(range, ET_BODY);
    test_range_expand(range, texteditW, VARIANT_TRUE, "wordabc 123\r\nit's text");
    test_range_expand(range, texteditW, VARIANT_TRUE, "wordabc 123\r\nit's text");
    test_range_move(range, characterW, 3, 3);
    test_range_expand(range, wordW, VARIANT_TRUE, "wordabc ");
    test_range_moveend(range, characterW, -4, -4);
    test_range_put_text(range, "abc def ");
    test_range_expand(range, texteditW, VARIANT_TRUE, "abc def abc 123\r\nit's text");
    test_range_move(range, wordW, 1, 1);
    test_range_movestart(range, characterW, -1, -1);
    test_range_text(range, " ");
    test_range_move(range, wordW, 1, 1);
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "def");
    test_range_put_text(range, "xyz");
    test_range_moveend(range, characterW, 1, 1);
    test_range_move(range, wordW, 1, 1);
    test_range_moveend(range, characterW, 2, 2);
    test_range_text(range, "ab");

    IHTMLTxtRange_Release(range);
}

static void test_txtrange2(IHTMLDocument2 *doc)
{
    IHTMLTxtRange *range;

    range = test_create_body_range(doc);

    test_range_text(range, "abc\r\n\r\n123\r\n\r\n\r\ndef");
    test_range_move(range, characterW, 5, 5);
    test_range_moveend(range, characterW, 1, 1);
    test_range_text(range, "2");
    test_range_move(range, characterW, -3, -3);
    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "c\r\n\r\n1");
    test_range_collapse(range, VARIANT_FALSE);
    test_range_moveend(range, characterW, 4, 4);
    test_range_text(range, "23");
    test_range_moveend(range, characterW, 1, 1);
    test_range_text(range, "23\r\n\r\n\r\nd");
    test_range_moveend(range, characterW, -1, -1);
    test_range_text(range, "23");
    test_range_moveend(range, characterW, -1, -1);
    test_range_text(range, "23");
    test_range_moveend(range, characterW, -2, -2);
    test_range_text(range, "2");

    IHTMLTxtRange_Release(range);
}

static void test_compatmode(IHTMLDocument2 *doc)
{
    IHTMLDocument5 *doc5;
    BSTR mode;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument5, (void**)&doc5);
    ok(hres == S_OK, "Could not get IHTMLDocument5 interface: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLDocument5_get_compatMode(doc5, &mode);
    IHTMLDocument5_Release(doc5);
    ok(hres == S_OK, "get_compatMode failed: %08x\n", hres);
    ok(!strcmp_wa(mode, "BackCompat"), "compatMode=%s\n", dbgstr_w(mode));
    SysFreeString(mode);
}

static void test_location(IHTMLDocument2 *doc)
{
    IHTMLLocation *location, *location2;
    ULONG ref;
    HRESULT hres;

    hres = IHTMLDocument2_get_location(doc, &location);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);

    hres = IHTMLDocument2_get_location(doc, &location2);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);

    ok(location == location2, "location != location2\n");

    test_ifaces((IUnknown*)location, location_iids);

    IHTMLLocation_Release(location2);
    ref = IHTMLLocation_Release(location);
    ok(!ref, "location chould be destroyed here\n");
}

static void test_navigator(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    IOmNavigator *navigator, *navigator2;
    ULONG ref;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "parentWidnow failed: %08x\n", hres);

    hres = IHTMLWindow2_get_navigator(window, &navigator);
    ok(hres == S_OK, "get_navigator failed: %08x\n", hres);
    ok(navigator != NULL, "navigator == NULL\n");
    test_disp((IUnknown*)navigator, &IID_IOmNavigator);

    hres = IHTMLWindow2_get_navigator(window, &navigator2);
    ok(hres == S_OK, "get_navigator failed: %08x\n", hres);
    ok(navigator != navigator2, "navigator2 != navihgator\n");

    IHTMLWindow2_Release(window);
    IOmNavigator_Release(navigator2);
    ref = IOmNavigator_Release(navigator);
    ok(!ref, "navigator should be destroyed here\n");
}

static void test_default_style(IHTMLStyle *style)
{
    VARIANT_BOOL b;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    test_disp((IUnknown*)style, &DIID_DispHTMLStyle);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_fontFamily(style, &str);
    ok(hres == S_OK, "get_fontFamily failed: %08x\n", hres);
    ok(!str, "fontFamily = %s\n", dbgstr_w(str));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_fontWeight(style, &str);
    ok(hres == S_OK, "get_fontWeight failed: %08x\n", hres);
    ok(!str, "fontWeight = %s\n", dbgstr_w(str));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_display(style, &str);
    ok(hres == S_OK, "get_display failed: %08x\n", hres);
    ok(!str, "display = %s\n", dbgstr_w(str));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_visibility(style, &str);
    ok(hres == S_OK, "get_visibility failed: %08x\n", hres);
    ok(!str, "visibility = %s\n", dbgstr_w(str));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_fontSize(style, &v);
    ok(hres == S_OK, "get_fontSize failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(fontSize) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(fontSize) = %s\n", dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_color(style, &v);
    ok(hres == S_OK, "get_color failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(color) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(color) = %s\n", dbgstr_w(V_BSTR(&v)));

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationUnderline(style, &b);
    ok(hres == S_OK, "get_textDecorationUnderline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationUnderline = %x\n", b);

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationLineThrough(style, &b);
    ok(hres == S_OK, "get_textDecorationLineThrough failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationLineThrough = %x\n", b);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_width(style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v)=%p\n", V_BSTR(&v));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("auto");
    hres = IHTMLStyle_put_width(style, v);
    ok(hres == S_OK, "put_width failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_width(style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "auto"), "V_BSTR(v)=%s\n", dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

static void test_default_selection(IHTMLDocument2 *doc)
{
    IHTMLSelectionObject *selection;
    IHTMLTxtRange *range;
    IDispatch *disp;
    BSTR str;
    HRESULT hres;

    hres = IHTMLDocument2_get_selection(doc, &selection);
    ok(hres == S_OK, "get_selection failed: %08x\n", hres);

    hres = IHTMLSelectionObject_get_type(selection, &str);
    ok(hres == S_OK, "get_type failed: %08x\n", hres);
    ok(!lstrcmpW(str, noneW), "type = %s\n", dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLSelectionObject_createRange(selection, &disp);
    IHTMLSelectionObject_Release(selection);
    ok(hres == S_OK, "createRange failed: %08x\n", hres);

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLTxtRange, (void**)&range);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IHTMLTxtRange interface: %08x\n", hres);

    test_range_text(range, NULL);
    IHTMLTxtRange_Release(range);
}

static void test_default_body(IHTMLBodyElement *body)
{
    BSTR bstr;
    HRESULT hres;

    bstr = (void*)0xdeadbeef;
    hres = IHTMLBodyElement_get_background(body, &bstr);
    ok(hres == S_OK, "get_background failed: %08x\n", hres);
    ok(bstr == NULL, "bstr != NULL\n");
}

static void test_window(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window, *window2, *self;
    IHTMLDocument2 *doc2 = NULL;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentElement failed: %08x\n", hres);
    test_ifaces((IUnknown*)window, window_iids);
    test_disp((IUnknown*)window, &DIID_DispHTMLWindow2);

    hres = IHTMLWindow2_get_document(window, &doc2);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);
    ok(doc2 != NULL, "doc2 == NULL\n");

    IHTMLDocument_Release(doc2);

    hres = IHTMLWindow2_get_window(window, &window2);
    ok(hres == S_OK, "get_window failed: %08x\n", hres);
    ok(window2 != NULL, "window2 == NULL\n");

    hres = IHTMLWindow2_get_self(window, &self);
    ok(hres == S_OK, "get_window failed: %08x\n", hres);
    ok(window2 != NULL, "self == NULL\n");

    ok(self == window2, "self != window2\n");

    IHTMLWindow2_Release(window2);
    IHTMLWindow2_Release(self);

    IHTMLWindow2_Release(window);
}

static void test_defaults(IHTMLDocument2 *doc)
{
    IHTMLStyleSheetsCollection *stylesheetcol;
    IHTMLBodyElement *body;
    IHTMLElement *elem;
    IHTMLStyle *style;
    long l;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLBodyElement, (void**)&body);
    ok(hres == S_OK, "Could not get IHTMBodyElement: %08x\n", hres);
    test_default_body(body);
    IHTMLBodyElement_Release(body);

    hres = IHTMLElement_get_style(elem, &style);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    test_default_style(style);
    test_window(doc);
    test_compatmode(doc);
    test_location(doc);
    test_navigator(doc);

    IHTMLStyle_Release(style);

    hres = IHTMLDocument2_get_styleSheets(doc, &stylesheetcol);
    ok(hres == S_OK, "get_styleSheets failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyleSheetsCollection_get_length(stylesheetcol, &l);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(l == 0, "length = %ld\n", l);

    IHTMLStyleSheetsCollection_Release(stylesheetcol);

    test_default_selection(doc);
}

static void test_stylesheet(IDispatch *disp)
{
    IHTMLStyleSheetRulesCollection *col = NULL;
    IHTMLStyleSheet *stylesheet;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLStyleSheet, (void**)&stylesheet);
    ok(hres == S_OK, "Could not get IHTMLStyleSheet: %08x\n", hres);

    hres = IHTMLStyleSheet_get_rules(stylesheet, &col);
    ok(hres == S_OK, "get_rules failed: %08x\n", hres);
    ok(col != NULL, "col == NULL\n");

    IHTMLStyleSheetRulesCollection_Release(col);
    IHTMLStyleSheet_Release(stylesheet);
}

static void test_stylesheets(IHTMLDocument2 *doc)
{
    IHTMLStyleSheetsCollection *col = NULL;
    VARIANT idx, res;
    long len = 0;
    HRESULT hres;

    hres = IHTMLDocument2_get_styleSheets(doc, &col);
    ok(hres == S_OK, "get_styleSheets failed: %08x\n", hres);
    ok(col != NULL, "col == NULL\n");

    hres = IHTMLStyleSheetsCollection_get_length(col, &len);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(len == 1, "len=%ld\n", len);

    VariantInit(&res);
    V_VT(&idx) = VT_I4;
    V_I4(&idx) = 0;

    hres = IHTMLStyleSheetsCollection_item(col, &idx, &res);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(V_VT(&res) == VT_DISPATCH, "V_VT(res) = %d\n", V_VT(&res));
    ok(V_DISPATCH(&res) != NULL, "V_DISPATCH(&res) == NULL\n");
    test_stylesheet(V_DISPATCH(&res));
    VariantClear(&res);

    V_VT(&res) = VT_I4;
    V_VT(&idx) = VT_I4;
    V_I4(&idx) = 1;

    hres = IHTMLStyleSheetsCollection_item(col, &idx, &res);
    ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
    ok(V_VT(&res) == VT_EMPTY, "V_VT(res) = %d\n", V_VT(&res));
    ok(V_DISPATCH(&res) != NULL, "V_DISPATCH(&res) == NULL\n");
    VariantClear(&res);

    IHTMLStyleSheetsCollection_Release(col);
}

static void test_child_col_disp(IHTMLDOMChildrenCollection *col)
{
    IDispatchEx *dispex;
    IHTMLDOMNode *node;
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    VARIANT var;
    EXCEPINFO ei;
    long type;
    DISPID id;
    BSTR bstr;
    HRESULT hres;

    static const WCHAR w0[] = {'0',0};
    static const WCHAR w100[] = {'1','0','0',0};

    hres = IHTMLDOMChildrenCollection_QueryInterface(col, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "Could not get IDispatchEx: %08x\n", hres);

    bstr = SysAllocString(w0);
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08x\n", hres);
    SysFreeString(bstr);

    VariantInit(&var);
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(var) == NULL\n");
    node = get_node_iface((IUnknown*)V_DISPATCH(&var));
    type = get_node_type((IUnknown*)node);
    ok(type == 3, "type=%ld\n", type);
    IHTMLDOMNode_Release(node);
    VariantClear(&var);

    bstr = SysAllocString(w100);
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    SysFreeString(bstr);

    IDispatchEx_Release(dispex);
}



static void test_elems(IHTMLDocument2 *doc)
{
    IHTMLElementCollection *col;
    IHTMLDOMChildrenCollection *child_col;
    IHTMLElement *elem;
    IHTMLDOMNode *node, *node2;
    IDispatch *disp;
    long type;
    HRESULT hres;

    static const WCHAR imgidW[] = {'i','m','g','i','d',0};
    static const WCHAR inW[] = {'i','n',0};
    static const WCHAR xW[] = {'x',0};
    static const WCHAR sW[] = {'s',0};
    static const WCHAR scW[] = {'s','c',0};
    static const WCHAR xxxW[] = {'x','x','x',0};

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_STYLE,
        ET_BODY,
        ET_COMMENT,
        ET_A,
        ET_INPUT,
        ET_SELECT,
        ET_OPTION,
        ET_OPTION,
        ET_TEXTAREA,
        ET_TABLE,
        ET_TBODY,
        ET_SCRIPT,
        ET_TEST,
        ET_IMG
    };

    static const elem_type_t item_types[] = {
        ET_A,
        ET_OPTION,
        ET_TEXTAREA
    };

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    test_elem_col_item(col, xW, item_types, sizeof(item_types)/sizeof(item_types[0]));
    IHTMLElementCollection_Release(col);

    elem = get_doc_elem(doc);
    ok(hres == S_OK, "get_documentElement failed: %08x\n", hres);
    hres = IHTMLElement_get_all(elem, &disp);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLElementCollection, (void**)&col);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IHTMLElementCollection: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types+1, sizeof(all_types)/sizeof(all_types[0])-1);
    IHTMLElementCollection_Release(col);

    get_elem_by_id(doc, xxxW, FALSE);
    elem = get_doc_elem_by_id(doc, xxxW);
    ok(!elem, "elem != NULL\n");

    elem = get_doc_elem_by_id(doc, sW);
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_elem_type((IUnknown*)elem, ET_SELECT);
        test_elem_attr(elem, xxxW, NULL);
        test_elem_attr(elem, idW, sW);
        test_elem_class((IUnknown*)elem, NULL);
        test_elem_set_class((IUnknown*)elem, "cl");
        test_elem_set_class((IUnknown*)elem, NULL);
        test_elem_tabindex((IUnknown*)elem, 0);
        test_elem_set_tabindex((IUnknown*)elem, 1);

        node = test_node_get_parent((IUnknown*)elem);
        ok(node != NULL, "node == NULL\n");
        test_node_name((IUnknown*)node, "BODY");
        node2 = test_node_get_parent((IUnknown*)node);
        IHTMLDOMNode_Release(node);
        ok(node2 != NULL, "node == NULL\n");
        test_node_name((IUnknown*)node2, "HTML");
        node = test_node_get_parent((IUnknown*)node2);
        IHTMLDOMNode_Release(node2);
        ok(node != NULL, "node == NULL\n");
        test_node_name((IUnknown*)node, "#document");
        type = get_node_type((IUnknown*)node);
        ok(type == 9, "type=%ld, expected 9\n", type);
        node2 = test_node_get_parent((IUnknown*)node);
        IHTMLDOMNode_Release(node);
        ok(node2 == NULL, "node != NULL\n");

        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, sW, TRUE);
    if(elem) {
        IHTMLSelectElement *select;

        hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLSelectElement, (void**)&select);
        ok(hres == S_OK, "Could not get IHTMLSelectElement interface: %08x\n", hres);

        test_select_elem(select);

        test_elem_title((IUnknown*)select, NULL);
        test_elem_set_title((IUnknown*)select, "Title");
        test_elem_title((IUnknown*)select, "Title");
        test_elem_offset((IUnknown*)select);

        node = get_first_child((IUnknown*)select);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            test_elem_type((IUnknown*)node, ET_OPTION);
            IHTMLDOMNode_Release(node);
        }

        type = get_node_type((IUnknown*)select);
        ok(type == 1, "type=%ld\n", type);

        IHTMLSelectElement_Release(select);
        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, scW, TRUE);
    if(elem) {
        IHTMLScriptElement *script;
        BSTR type;

        hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLScriptElement, (void**)&script);
        ok(hres == S_OK, "Could not get IHTMLScriptElement interface: %08x\n", hres);

        hres = IHTMLScriptElement_get_type(script, &type);
        ok(hres == S_OK, "get_type failed: %08x\n", hres);
        ok(!lstrcmpW(type, text_javascriptW), "Unexpected type %s\n", dbgstr_w(type));
        SysFreeString(type);

        IHTMLScriptElement_Release(script);
    }

    elem = get_elem_by_id(doc, inW, TRUE);
    if(elem) {
        IHTMLInputElement *input;

        hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLInputElement, (void**)&input);
        ok(hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);

        test_elem_id((IUnknown*)elem, "in");
        test_elem_put_id((IUnknown*)elem, "newin");
        test_input_get_disabled(input, VARIANT_FALSE);
        test_input_set_disabled(input, VARIANT_TRUE);
        test_input_set_disabled(input, VARIANT_FALSE);
        test_elem_client_size((IUnknown*)elem);

        test_node_get_value_str((IUnknown*)elem, NULL);
        test_node_put_value_str((IUnknown*)elem, "test");
        test_node_get_value_str((IUnknown*)elem, NULL);
        test_input_value((IUnknown*)elem, NULL);
        test_input_put_value((IUnknown*)elem, "test");
        test_input_value((IUnknown*)elem, NULL);
        test_elem_class((IUnknown*)elem, "testclass");
        test_elem_tabindex((IUnknown*)elem, 2);
        test_elem_set_tabindex((IUnknown*)elem, 3);
        test_elem_title((IUnknown*)elem, "test title");

        IHTMLInputElement_Release(input);
        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, imgidW, TRUE);
    if(elem) {
        test_img_set_src((IUnknown*)elem, "about:blank");
        test_img_alt((IUnknown*)elem, NULL);
        test_img_set_alt((IUnknown*)elem, "alt test");
        IHTMLElement_Release(elem);
    }

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    node = get_first_child((IUnknown*)elem);
    ok(node != NULL, "node == NULL\n");
    if(node) {
        test_ifaces((IUnknown*)node, text_iids);
        test_disp((IUnknown*)node, &DIID_DispHTMLDOMTextNode);

        node2 = get_first_child((IUnknown*)node);
        ok(!node2, "node2 != NULL\n");

        type = get_node_type((IUnknown*)node);
        ok(type == 3, "type=%ld\n", type);

        test_node_get_value_str((IUnknown*)node, "text test");
        test_node_put_value_str((IUnknown*)elem, "test text");
        test_node_get_value_str((IUnknown*)node, "text test");

        IHTMLDOMNode_Release(node);
    }

    child_col = get_child_nodes((IUnknown*)elem);
    ok(child_col != NULL, "child_coll == NULL\n");
    if(child_col) {
        long length = 0;

        test_disp((IUnknown*)child_col, &DIID_DispDOMChildrenCollection);

        hres = IHTMLDOMChildrenCollection_get_length(child_col, &length);
        ok(hres == S_OK, "get_length failed: %08x\n", hres);
        ok(length, "length=0\n");

        node = get_child_item(child_col, 0);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            type = get_node_type((IUnknown*)node);
            ok(type == 3, "type=%ld\n", type);
            IHTMLDOMNode_Release(node);
        }

        node = get_child_item(child_col, 1);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            type = get_node_type((IUnknown*)node);
            ok(type == 8, "type=%ld\n", type);

            test_elem_id((IUnknown*)node, NULL);
            IHTMLDOMNode_Release(node);
        }

        disp = (void*)0xdeadbeef;
        hres = IHTMLDOMChildrenCollection_item(child_col, 6000, &disp);
        ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
        ok(disp == (void*)0xdeadbeef, "disp=%p\n", disp);

        disp = (void*)0xdeadbeef;
        hres = IHTMLDOMChildrenCollection_item(child_col, length, &disp);
        ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
        ok(disp == (void*)0xdeadbeef, "disp=%p\n", disp);

        test_child_col_disp(child_col);

        IHTMLDOMChildrenCollection_Release(child_col);
    }

    IHTMLElement_Release(elem);

    test_stylesheets(doc);
    test_create_option_elem(doc);
}

static void test_create_elems(IHTMLDocument2 *doc)
{
    IHTMLElement *elem, *body, *elem2;
    IHTMLDOMNode *node;
    IDispatch *disp;
    long type;
    HRESULT hres;

    static const elem_type_t types1[] = { ET_TESTG };

    elem = test_create_elem(doc, "TEST");
    test_elem_tag((IUnknown*)elem, "TEST");
    type = get_node_type((IUnknown*)elem);
    ok(type == 1, "type=%ld\n", type);
    test_ifaces((IUnknown*)elem, elem_iids);
    test_disp((IUnknown*)elem, &DIID_DispHTMLGenericElement);

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);
    test_node_has_child((IUnknown*)body, VARIANT_FALSE);

    node = test_node_append_child((IUnknown*)body, (IUnknown*)elem);
    test_node_has_child((IUnknown*)body, VARIANT_TRUE);
    elem2 = get_elem_iface((IUnknown*)node);
    IHTMLElement_Release(elem2);

    hres = IHTMLElement_get_all(body, &disp);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)disp, types1, sizeof(types1)/sizeof(types1[0]));
    IDispatch_Release(disp);

    test_node_remove_child((IUnknown*)body, node);

    hres = IHTMLElement_get_all(body, &disp);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)disp, NULL, 0);
    IDispatch_Release(disp);
    test_node_has_child((IUnknown*)body, VARIANT_FALSE);

    IHTMLElement_Release(body);
    IHTMLElement_Release(elem);
    IHTMLDOMNode_Release(node);

    node = test_create_text(doc, "test");
    test_ifaces((IUnknown*)node, text_iids);
    test_disp((IUnknown*)node, &DIID_DispHTMLDOMTextNode);
    IHTMLDOMNode_Release(node);
}

static void test_exec(IUnknown *unk, const GUID *grpid, DWORD cmdid, VARIANT *in, VARIANT *out)
{
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    hres = IHTMLTxtRange_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
    ok(hres == S_OK, "Could not get IOleCommandTarget interface: %08x\n", hres);

    hres = IOleCommandTarget_Exec(cmdtrg, grpid, cmdid, 0, in, out);
    ok(hres == S_OK, "Exec failed: %08x\n", hres);

    IOleCommandTarget_Release(cmdtrg);
}

static void test_indent(IHTMLDocument2 *doc)
{
    IHTMLElementCollection *col;
    IHTMLTxtRange *range;
    HRESULT hres;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_BODY,
        ET_BR,
        ET_A,
    };

    static const elem_type_t indent_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_BODY,
        ET_BLOCKQUOTE,
        ET_P,
        ET_BR,
        ET_A,
    };

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    IHTMLElementCollection_Release(col);

    range = test_create_body_range(doc);
    test_exec((IUnknown*)range, &CGID_MSHTML, IDM_INDENT, NULL, NULL);
    IHTMLTxtRange_Release(range);

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, indent_types, sizeof(indent_types)/sizeof(indent_types[0]));
    IHTMLElementCollection_Release(col);
}

static IHTMLDocument2 *notif_doc;
static BOOL doc_complete;

static HRESULT WINAPI PropertyNotifySink_QueryInterface(IPropertyNotifySink *iface,
        REFIID riid, void**ppv)
{
    if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI PropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    return 2;
}

static ULONG WINAPI PropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    return 1;
}

static HRESULT WINAPI PropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    if(dispID == DISPID_READYSTATE){
        BSTR state;
        HRESULT hres;

        static const WCHAR completeW[] = {'c','o','m','p','l','e','t','e',0};

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);

        if(!lstrcmpW(state, completeW))
            doc_complete = TRUE;

        SysFreeString(state);
    }

    return S_OK;
}

static HRESULT WINAPI PropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

static IHTMLDocument2 *create_doc_with_string(const char *str)
{
    IPersistStreamInit *init;
    IStream *stream;
    IHTMLDocument2 *doc;
    HGLOBAL mem;
    SIZE_T len;

    notif_doc = doc = create_document();
    if(!doc)
        return NULL;

    doc_complete = FALSE;
    len = strlen(str);
    mem = GlobalAlloc(0, len);
    memcpy(mem, str, len);
    CreateStreamOnHGlobal(mem, TRUE, &stream);

    IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);

    IPersistStreamInit_Load(init, stream);
    IPersistStreamInit_Release(init);
    IStream_Release(stream);

    return doc;
}

static void do_advise(IUnknown *unk, REFIID riid, IUnknown *unk_advise)
{
    IConnectionPointContainer *container;
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&container);
    ok(hres == S_OK, "QueryInterface(IID_IConnectionPointContainer) failed: %08x\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(container, riid, &cp);
    IConnectionPointContainer_Release(container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    hres = IConnectionPoint_Advise(cp, unk_advise, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);
}

typedef void (*domtest_t)(IHTMLDocument2*);

static void run_domtest(const char *str, domtest_t test)
{
    IHTMLDocument2 *doc;
    IHTMLElement *body = NULL;
    ULONG ref;
    MSG msg;
    HRESULT hres;

    doc = create_doc_with_string(str);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    if(body) {
        IHTMLElement_Release(body);
        test(doc);
    }else {
        skip("Could not get document body. Assuming no Gecko installed.\n");
    }

    ref = IHTMLDocument2_Release(doc);
    ok(!ref, "ref = %d\n", ref);
}

static void gecko_installer_workaround(BOOL disable)
{
    HKEY hkey;
    DWORD res;

    static BOOL has_url = FALSE;
    static char url[2048];

    if(!disable && !has_url)
        return;

    res = RegOpenKey(HKEY_CURRENT_USER, "Software\\Wine\\MSHTML", &hkey);
    if(res != ERROR_SUCCESS)
        return;

    if(disable) {
        DWORD type, size = sizeof(url);

        res = RegQueryValueEx(hkey, "GeckoUrl", NULL, &type, (PVOID)url, &size);
        if(res == ERROR_SUCCESS && type == REG_SZ)
            has_url = TRUE;

        RegDeleteValue(hkey, "GeckoUrl");
    }else {
        RegSetValueEx(hkey, "GeckoUrl", 0, REG_SZ, (PVOID)url, lstrlenA(url)+1);
    }

    RegCloseKey(hkey);
}

START_TEST(dom)
{
    gecko_installer_workaround(TRUE);
    CoInitialize(NULL);

    run_domtest(doc_str1, test_doc_elem);
    run_domtest(range_test_str, test_txtrange);
    run_domtest(range_test2_str, test_txtrange2);
    run_domtest(elem_test_str, test_elems);
    run_domtest(doc_blank, test_create_elems);
    run_domtest(doc_blank, test_defaults);
    run_domtest(indent_test_str, test_indent);

    CoUninitialize();
    gecko_installer_workaround(FALSE);
}
