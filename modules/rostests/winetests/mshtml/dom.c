/*
 * Copyright 2007-2011 Jacek Caban for CodeWeavers
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
//#include <stdarg.h>
#include <stdio.h>

//#include "windef.h"
//#include "winbase.h"
//#include "ole2.h"
#include <mshtml.h>
#include <mshtmcid.h>
#include <mshtmhst.h>
//#include "docobj.h"
#include <hlink.h>
//#include "dispex.h"
#include "mshtml_test.h"
#include <objsafe.h>
#include <htiface.h>
#include <tlogstg.h>

static INT (WINAPI *pLCIDToLocaleName)(LCID,LPWSTR,INT,DWORD);
static LANGID (WINAPI *pGetUserDefaultUILanguage)(void);

static const char doc_blank[] = "<html></html>";
static const char doc_str1[] = "<html><body>test</body></html>";
static const char range_test_str[] =
    "<html><body>test \na<font size=\"2\">bc\t123<br /> it's\r\n  \t</font>text<br /></body></html>";
static const char range_test2_str[] =
    "<html><body>abc<hr />123<br /><hr />def</body></html>";
static const char elem_test_str[] =
    "<html><head><title>test</title><style id=\"styleid\">.body { margin-right: 0px; }</style>"
    "<meta id=\"metaid\" name=\"meta name\" http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
    "<link id=\"linkid\"></head>"
    "<body onload=\"Testing()\">text test<!-- a comment -->"
    "<a id=\"a\" href=\"http://test\" name=\"x\">link</a>"
    "<label for=\"in\" id=\"labelid\">Label:</label>"
    "<input id=\"in\" class=\"testclass\" tabIndex=\"2\" title=\"test title\" />"
    "<button id=\"btnid\"></button>"
    "<select id=\"s\"><option id=\"x\" value=\"val1\">opt1</option><option id=\"y\">opt2</option></select>"
    "<textarea id=\"X\">text text</textarea>"
    "<table id=\"tbl\"><tbody><tr></tr><tr id=\"row2\"><td id=\"td1\">td1 text</td><td id=\"td2\">td2 text</td></tr></tbody></table>"
    "<script id=\"sc\" type=\"text/javascript\"><!--\nfunction Testing() {}\n// -->\n</script>"
    "<test /><object id=\"objid\" name=\"objname\" vspace=100></object><embed />"
    "<img id=\"imgid\" name=\"WineImg\"/>"
    "<iframe src=\"about:blank\" id=\"ifr\"></iframe>"
    "<form id=\"frm\"></form>"
    "<div id=\"attr\" attr1=\"attr1\" attr2 attr3=\"attr3\"></div>"
    "</body></html>";
static const char elem_test2_str[] =
    "<html><head><title>test</title><style>.body { margin-right: 0px; }</style>"
    "<link id=\"linkid\" rel=\"stylesheet\" href=\"about:blank\" type=\"text/css\"></head>"
    "<body><div id=\"divid\" emptyattr=\"\" onclick=\"parseInt();\"></div></body>"
    "</html>";

static const char indent_test_str[] =
    "<html><head><title>test</title></head><body>abc<br /><a href=\"about:blank\">123</a></body></html>";
static const char cond_comment_str[] =
    "<html><head><title>test</title></head><body>"
    "<!--[if gte IE 4]> <br> <![endif]-->"
    "</body></html>";
static const char frameset_str[] =
    "<html><head><title>frameset test</title></head><frameset rows=\"25, 25, *\">"
    "<frame src=\"about:blank\" name=\"nm1\" id=\"fr1\"><frame src=\"about:blank\" name=\"nm2\" id=\"fr2\">"
    "<frame src=\"about:blank\" id=\"fr3\">"
    "</frameset></html>";
static const char emptydiv_str[] =
    "<html><head><title>emptydiv test</title></head>"
    "<body><div id=\"divid\"></div></body></html>";
static const char noscript_str[] =
    "<html><head><title>noscript test</title><noscript><style>.body { margin-right: 0px; }</style></noscript></head>"
    "<body><noscript><div>test</div></noscript></body></html>";
static const char doctype_str[] =
    "<!DOCTYPE html>"
    "<html><head><title>emptydiv test</title></head>"
    "<body><div id=\"divid\"></div></body></html>";

static WCHAR characterW[] = {'c','h','a','r','a','c','t','e','r',0};
static WCHAR texteditW[] = {'t','e','x','t','e','d','i','t',0};
static WCHAR wordW[] = {'w','o','r','d',0};

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
    ET_IMG,
    ET_TR,
    ET_TD,
    ET_IFRAME,
    ET_FORM,
    ET_FRAME,
    ET_OBJECT,
    ET_EMBED,
    ET_DIV,
    ET_META,
    ET_NOSCRIPT,
    ET_LINK,
    ET_LABEL,
    ET_BUTTON
} elem_type_t;

static const IID * const none_iids[] = {
    &IID_IUnknown,
    NULL
};

static const IID * const doc_node_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLDocument,
    &IID_IHTMLDocument2,
    &IID_IHTMLDocument3,
    &IID_IHTMLDocument4,
    &IID_IHTMLDocument5,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    &IID_IInternetHostSecurityManager,
    &IID_IOleContainer,
    &IID_IObjectSafety,
    &IID_IProvideClassInfo,
    NULL
};

static const IID * const doc_obj_iids[] = {
    &IID_IHTMLDocument,
    &IID_IHTMLDocument2,
    &IID_IHTMLDocument3,
    &IID_IHTMLDocument4,
    &IID_IHTMLDocument5,
    &IID_IDispatchEx,
    &IID_IConnectionPointContainer,
    &IID_ICustomDoc,
    &IID_IOleContainer,
    &IID_IObjectSafety,
    &IID_IProvideClassInfo,
    &IID_ITargetContainer,
    NULL
};

#define ELEM_IFACES \
    &IID_IHTMLDOMNode,  \
    &IID_IHTMLDOMNode2, \
    &IID_IHTMLElement,  \
    &IID_IHTMLElement2, \
    &IID_IHTMLElement3, \
    &IID_IHTMLElement4, \
    &IID_IDispatchEx

static const IID * const elem_iids[] = {
    ELEM_IFACES,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const body_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLTextContainer,
    &IID_IHTMLBodyElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const anchor_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLAnchorElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const input_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLInputElement,
    &IID_IHTMLInputTextElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID *const button_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLButtonElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const label_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLLabelElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const select_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLSelectElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const textarea_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLTextAreaElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const option_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLOptionElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const table_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLTable,
    &IID_IHTMLTable2,
    &IID_IHTMLTable3,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const script_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLScriptElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const text_iids[] = {
    &IID_IHTMLDOMNode,
    &IID_IHTMLDOMNode2,
    &IID_IHTMLDOMTextNode,
    &IID_IHTMLDOMTextNode2,
    NULL
};

static const IID * const attr_iids[] = {
    &IID_IHTMLDOMAttribute,
    &IID_IHTMLDOMAttribute2,
    &IID_IDispatchEx,
    NULL
};

static const IID * const location_iids[] = {
    &IID_IDispatch,
    &IID_IHTMLLocation,
    NULL
};

static const IID * const window_iids[] = {
    &IID_IDispatch,
    &IID_IHTMLWindow2,
    &IID_IHTMLWindow3,
    &IID_IDispatchEx,
    &IID_IServiceProvider,
    NULL
};

static const IID * const comment_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLCommentElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const img_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLImgElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const tr_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLTableRow,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const td_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLTableCell,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const frame_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLFrameBase,
    &IID_IHTMLFrameBase2,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const head_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLHeadElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const title_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLTitleElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const meta_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLMetaElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const link_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLLinkElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const object_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLObjectElement,
    &IID_IHTMLObjectElement2,
    /* FIXME: No IConnectionPointContainer */
    NULL
};

static const IID * const embed_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLEmbedElement,
    /* FIXME: No IConnectionPointContainer */
    NULL
};

static const IID * const iframe_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLFrameBase,
    &IID_IHTMLFrameBase2,
    &IID_IHTMLIFrameElement,
    &IID_IHTMLIFrameElement2,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const form_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLFormElement,
    &IID_IConnectionPointContainer,
    &DIID_DispHTMLFormElement,
    NULL
};

static const IID * const styleelem_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLStyleElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const generic_iids[] = {
    ELEM_IFACES,
    &IID_IHTMLGenericElement,
    &IID_IConnectionPointContainer,
    NULL
};

static const IID * const style_iids[] = {
    &IID_IUnknown,
    &IID_IDispatch,
    &IID_IDispatchEx,
    &IID_IHTMLStyle,
    &IID_IHTMLStyle2,
    &IID_IHTMLStyle3,
    &IID_IHTMLStyle4,
    NULL
};

static const IID * const cstyle_iids[] = {
    &IID_IUnknown,
    &IID_IDispatch,
    &IID_IDispatchEx,
    &IID_IHTMLCurrentStyle,
    &IID_IHTMLCurrentStyle2,
    &IID_IHTMLCurrentStyle3,
    NULL
};

static const IID * const img_factory_iids[] = {
    &IID_IUnknown,
    &IID_IDispatch,
    &IID_IDispatchEx,
    &IID_IHTMLImageElementFactory,
    NULL
};

static const IID * const selection_iids[] = {
    &IID_IUnknown,
    &IID_IDispatch,
    &IID_IDispatchEx,
    &IID_IHTMLSelectionObject,
    &IID_IHTMLSelectionObject2,
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
    {"HEAD",      head_iids,        &DIID_DispHTMLHeadElement},
    {"TITLE",     title_iids,       &DIID_DispHTMLTitleElement},
    {"BODY",      body_iids,        &DIID_DispHTMLBody},
    {"A",         anchor_iids,      &DIID_DispHTMLAnchorElement},
    {"INPUT",     input_iids,       &DIID_DispHTMLInputElement},
    {"SELECT",    select_iids,      &DIID_DispHTMLSelectElement},
    {"TEXTAREA",  textarea_iids,    &DIID_DispHTMLTextAreaElement},
    {"OPTION",    option_iids,      &DIID_DispHTMLOptionElement},
    {"STYLE",     styleelem_iids,   &DIID_DispHTMLStyleElement},
    {"BLOCKQUOTE",elem_iids,        NULL},
    {"P",         elem_iids,        NULL},
    {"BR",        elem_iids,        NULL},
    {"TABLE",     table_iids,       &DIID_DispHTMLTable},
    {"TBODY",     elem_iids,        NULL},
    {"SCRIPT",    script_iids,      &DIID_DispHTMLScriptElement},
    {"TEST",      elem_iids,        &DIID_DispHTMLUnknownElement},
    {"TEST",      generic_iids,     &DIID_DispHTMLGenericElement},
    {"!",         comment_iids,     &DIID_DispHTMLCommentElement},
    {"IMG",       img_iids,         &DIID_DispHTMLImg},
    {"TR",        tr_iids,          &DIID_DispHTMLTableRow},
    {"TD",        td_iids,          &DIID_DispHTMLTableCell},
    {"IFRAME",    iframe_iids,      &DIID_DispHTMLIFrame},
    {"FORM",      form_iids,        &DIID_DispHTMLFormElement},
    {"FRAME",     frame_iids,       &DIID_DispHTMLFrameElement},
    {"OBJECT",    object_iids,      &DIID_DispHTMLObjectElement},
    {"EMBED",     embed_iids,       &DIID_DispHTMLEmbed},
    {"DIV",       elem_iids,        NULL},
    {"META",      meta_iids,        &DIID_DispHTMLMetaElement},
    {"NOSCRIPT",  elem_iids,        NULL /*&DIID_DispHTMLNoShowElement*/},
    {"LINK",      link_iids,        &DIID_DispHTMLLinkElement},
    {"LABEL",     label_iids,       &DIID_DispHTMLLabelElement},
    {"BUTTON",    button_iids,      &DIID_DispHTMLButtonElement}
};

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

static BOOL is_prefix_wa(const WCHAR *strw, const char *prefix)
{
    int len, prefix_len;
    CHAR buf[512];

    len = WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL)-1;
    prefix_len = lstrlenA(prefix);
    if(len < prefix_len)
        return FALSE;

    buf[prefix_len] = 0;
    return !lstrcmpA(buf, prefix);
}

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    if(!str)
        return NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static const char *debugstr_variant(const VARIANT *var)
{
    static char buf[400];

    if (!var)
        return "(null)";

    switch (V_VT(var))
    {
    case VT_EMPTY:
        return "{VT_EMPTY}";
    case VT_BSTR:
        sprintf(buf, "{VT_BSTR: %s}", wine_dbgstr_w(V_BSTR(var)));
        break;
    case VT_BOOL:
        sprintf(buf, "{VT_BOOL: %x}", V_BOOL(var));
        break;
    case VT_UI4:
        sprintf(buf, "{VT_UI4: %u}", V_UI4(var));
        break;
    default:
        sprintf(buf, "{vt %d}", V_VT(var));
        break;
    }

    return buf;
}

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;

    if(iface1 == iface2)
        return TRUE;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_Release(unk1);
    IUnknown_QueryInterface(iface2, &IID_IUnknown, (void**)&unk2);
    IUnknown_Release(unk2);

    return unk1 == unk2;
}

static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    IHTMLDocument5 *doc5;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    if(FAILED(hres))
        return NULL;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument5, (void**)&doc5);
    if(FAILED(hres)) {
        win_skip("Could not get IHTMLDocument5, probably too old IE\n");
        IHTMLDocument2_Release(doc);
        return NULL;
    }

    IHTMLDocument5_Release(doc5);
    return doc;
}

#define get_dispex_iface(u) _get_dispex_iface(__LINE__,u)
static IDispatchEx *_get_dispex_iface(unsigned line, IUnknown *unk)
{
    IDispatchEx *dispex;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IDispatchEx, (void**)&dispex);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IDispatchEx: %08x\n", hres);
    return dispex;
}

#define test_ifaces(i,ids) _test_ifaces(__LINE__,i,ids)
static void _test_ifaces(unsigned line, IUnknown *iface, REFIID *iids)
{
    const IID * const *piid;
    IUnknown *unk;
    HRESULT hres;

     for(piid = iids; *piid; piid++) {
        hres = IUnknown_QueryInterface(iface, *piid, (void**)&unk);
        ok_(__FILE__,line) (hres == S_OK, "Could not get %s interface: %08x\n", wine_dbgstr_guid(*piid), hres);
        if(SUCCEEDED(hres))
            IUnknown_Release(unk);
    }
}

#define test_no_iface(a,b) _test_no_iface(__LINE__,a,b)
static void _test_no_iface(unsigned line, IUnknown *iface, REFIID iid)
{
    IUnknown *unk;
    HRESULT hres;

    unk = (void*)0xdeadbeef;
    hres = IUnknown_QueryInterface(iface, iid, (void**)&unk);
    ok_(__FILE__,line)(hres == E_NOINTERFACE, "hres = %08x, expected E_NOINTERFACE\n", hres);
    ok_(__FILE__,line)(!unk, "unk = %p\n", unk);
}

#define test_get_dispid(u,id) _test_get_dispid(__LINE__,u,id)
static BOOL _test_get_dispid(unsigned line, IUnknown *unk, IID *iid)
{
    IDispatchEx *dispex = _get_dispex_iface(line, unk);
    ITypeInfo *typeinfo;
    BOOL ret = FALSE;
    UINT ticnt;
    HRESULT hres;

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
        if(hres == S_OK) {
            *iid = type_attr->guid;
            ret = TRUE;
        }

        ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
        ITypeInfo_Release(typeinfo);
    }

    IDispatchEx_Release(dispex);
    return ret;
}

#define test_disp_value(u) _test_disp_value(__LINE__,u,v)
static void _test_disp_value(unsigned line, IUnknown *unk, const char *val)
{
    IDispatchEx *dispex = _get_dispex_iface(line, unk);
    DISPPARAMS dp  = {NULL,NULL,0,0};
    EXCEPINFO ei;
    VARIANT var;
    HRESULT hres;

    hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, 0, DISPATCH_PROPERTYGET, &dp, &var, &ei, NULL);
    IDispatchEx_Release(dispex);
    ok_(__FILE__,line)(hres == S_OK, "InvokeEx(DISPID_VALUE) returned: %08x\n", hres);

    ok_(__FILE__,line)(V_VT(&var) == VT_BSTR, "V_VT(value) = %d\n", V_VT(&var));
    ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&var), val), "value = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&var)), val);
    VariantClear(&var);
}

#define test_disp(u,id,v) _test_disp(__LINE__,u,id,v)
static void _test_disp(unsigned line, IUnknown *unk, const IID *diid, const char *val)
{
    IID iid;

    if(_test_get_dispid(line, unk, &iid))
        ok_(__FILE__,line) (IsEqualGUID(&iid, diid), "unexpected guid %s\n", wine_dbgstr_guid(&iid));

    if(val)
        _test_disp_value(line, unk, val);
}

#define test_disp2(u,id,id2,v) _test_disp2(__LINE__,u,id,id2,v)
static void _test_disp2(unsigned line, IUnknown *unk, const IID *diid, const IID *diid2, const char *val)
{
    IID iid;

    if(_test_get_dispid(line, unk, &iid))
        ok_(__FILE__,line) (IsEqualGUID(&iid, diid) || broken(IsEqualGUID(&iid, diid2)),
                "unexpected guid %s\n", wine_dbgstr_guid(&iid));

    if(val)
        _test_disp_value(line, unk, val);
}

#define test_class_info(u) _test_class_info(__LINE__,u)
static void _test_class_info(unsigned line, IUnknown *unk)
{
    IProvideClassInfo *classinfo;
    ITypeInfo *typeinfo;
    TYPEATTR *type_attr;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IProvideClassInfo, (void**)&classinfo);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IProvideClassInfo interface: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IProvideClassInfo_GetClassInfo(classinfo, &typeinfo);
    ok_(__FILE__,line)(hres == S_OK, "Could not get ITypeInfo interface: %08x\n", hres);
    if(FAILED(hres))
    {
        IProvideClassInfo_Release(classinfo);
        return;
    }

    hres = ITypeInfo_GetTypeAttr(typeinfo, &type_attr);
    ok_(__FILE__,line)(hres == S_OK, "GetTypeAttr failed: %08x\n", hres);
    if(SUCCEEDED(hres))
    {
        ok_(__FILE__,line)(IsEqualGUID(&type_attr->guid, &CLSID_HTMLDocument),
                "unexpected guid %s\n", wine_dbgstr_guid(&type_attr->guid));
        ok_(__FILE__,line)(type_attr->typekind == TKIND_COCLASS,
                "unexpected typekind %d\n", type_attr->typekind);
        ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
    }

    ITypeInfo_Release(typeinfo);
    IProvideClassInfo_Release(classinfo);
}

#define set_dispex_value(a,b,c) _set_dispex_value(__LINE__,a,b,c)
static void _set_dispex_value(unsigned line, IUnknown *unk, const char *name, VARIANT *val)
{
    IDispatchEx *dispex = _get_dispex_iface(line, unk);
    DISPPARAMS dp = {val, NULL, 1, 0};
    EXCEPINFO ei;
    DISPID id;
    BSTR str;
    HRESULT hres;

    str = a2bstr(name);
    hres = IDispatchEx_GetDispID(dispex, str, fdexNameEnsure|fdexNameCaseInsensitive, &id);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "GetDispID failed: %08x\n", hres);

    memset(&ei, 0, sizeof(ei));
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, INVOKE_PROPERTYPUT, &dp, NULL, &ei, NULL);
    ok_(__FILE__,line)(hres == S_OK, "InvokeEx failed: %08x\n", hres);

}

#define get_elem_iface(u) _get_elem_iface(__LINE__,u)
static IHTMLElement *_get_elem_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement: %08x\n", hres);
    return elem;
}

#define get_elem2_iface(u) _get_elem2_iface(__LINE__,u)
static IHTMLElement2 *_get_elem2_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement2, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement2: %08x\n", hres);
    return elem;
}

#define get_elem3_iface(u) _get_elem3_iface(__LINE__,u)
static IHTMLElement3 *_get_elem3_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement3 *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement3, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement3: %08x\n", hres);
    return elem;
}

#define get_elem4_iface(u) _get_elem4_iface(__LINE__,u)
static IHTMLElement4 *_get_elem4_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement4 *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement4, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement4: %08x\n", hres);
    return elem;
}

#define get_doc3_iface(u) _get_doc3_iface(__LINE__,u)
static IHTMLDocument3 *_get_doc3_iface(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDocument3 interface: %08x\n", hres);

    return doc3;
}

#define get_node_iface(u) _get_node_iface(__LINE__,u)
static IHTMLDOMNode *_get_node_iface(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMNode, (void**)&node);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDOMNode: %08x\n", hres);
    return node;
}

#define get_node2_iface(u) _get_node2_iface(__LINE__,u)
static IHTMLDOMNode2 *_get_node2_iface(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode2 *node;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMNode2, (void**)&node);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDOMNode2: %08x\n", hres);
    return node;
}

#define get_htmldoc5_iface(u) _get_htmldoc5_iface(__LINE__,u)
static IHTMLDocument5 *_get_htmldoc5_iface(unsigned line, IUnknown *unk)
{
    IHTMLDocument5 *doc;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument5, (void**)&doc);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDocument5: %08x\n", hres);
    return doc;
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

#define get_anchor_iface(u) _get_anchor_iface(__LINE__,u)
static IHTMLAnchorElement *_get_anchor_iface(unsigned line, IUnknown *unk)
{
    IHTMLAnchorElement *anchor;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLAnchorElement, (void**)&anchor);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLAnchorElement: %08x\n", hres);
    return anchor;
}

#define get_textarea_iface(u) _get_textarea_iface(__LINE__,u)
static IHTMLTextAreaElement *_get_textarea_iface(unsigned line, IUnknown *unk)
{
    IHTMLTextAreaElement *textarea;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLTextAreaElement, (void**)&textarea);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLTextAreaElement: %08x\n", hres);
    return textarea;
}

#define get_select_iface(u) _get_select_iface(__LINE__,u)
static IHTMLSelectElement *_get_select_iface(unsigned line, IUnknown *unk)
{
    IHTMLSelectElement *select;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLSelectElement, (void**)&select);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLSelectElement: %08x\n", hres);
    return select;
}

#define get_option_iface(u) _get_option_iface(__LINE__,u)
static IHTMLOptionElement *_get_option_iface(unsigned line, IUnknown *unk)
{
    IHTMLOptionElement *option;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLOptionElement, (void**)&option);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLOptionElement: %08x\n", hres);
    return option;
}

#define get_form_iface(u) _get_form_iface(__LINE__,u)
static IHTMLFormElement *_get_form_iface(unsigned line, IUnknown *unk)
{
    IHTMLFormElement *form;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLFormElement, (void**)&form);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLFormElement: %08x\n", hres);
    return form;
}

#define get_text_iface(u) _get_text_iface(__LINE__,u)
static IHTMLDOMTextNode *_get_text_iface(unsigned line, IUnknown *unk)
{
    IHTMLDOMTextNode *text;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMTextNode, (void**)&text);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDOMTextNode: %08x\n", hres);
    return text;
}

#define get_text2_iface(u) _get_text2_iface(__LINE__,u)
static IHTMLDOMTextNode2 *_get_text2_iface(unsigned line, IUnknown *unk)
{
    IHTMLDOMTextNode2 *text2;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMTextNode2, (void**)&text2);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDOMTextNode2: %08x\n", hres);
    return text2;
}

#define get_comment_iface(u) _get_comment_iface(__LINE__,u)
static IHTMLCommentElement *_get_comment_iface(unsigned line, IUnknown *unk)
{
    IHTMLCommentElement *comment;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLCommentElement, (void**)&comment);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLCommentElement: %08x\n", hres);
    return comment;
}

#define get_object_iface(u) _get_object_iface(__LINE__,u)
static IHTMLObjectElement *_get_object_iface(unsigned line, IUnknown *unk)
{
    IHTMLObjectElement *obj;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLObjectElement, (void**)&obj);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLObjectElement: %08x\n", hres);
    return obj;
}

#define get_style_iface(u) _get_style_iface(__LINE__,u)
static IHTMLStyleElement *_get_style_iface(unsigned line, IUnknown *unk)
{
    IHTMLStyleElement *obj;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLStyleElement, (void**)&obj);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLStyleElement: %08x\n", hres);
    return obj;
}

#define get_metaelem_iface(u) _get_metaelem_iface(__LINE__,u)
static IHTMLMetaElement *_get_metaelem_iface(unsigned line, IUnknown *unk)
{
    IHTMLMetaElement *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLMetaElement, (void**)&ret);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLMetaElement: %08x\n", hres);
    return ret;
}

#define get_link_iface(u) _get_link_iface(__LINE__,u)
static IHTMLLinkElement *_get_link_iface(unsigned line, IUnknown *unk)
{
    IHTMLLinkElement *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLLinkElement, (void**)&ret);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLLinkElement: %08x\n", hres);
    return ret;
}

#define get_iframe2_iface(u) _get_iframe2_iface(__LINE__,u)
static IHTMLIFrameElement2 *_get_iframe2_iface(unsigned line, IUnknown *unk)
{
    IHTMLIFrameElement2 *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLIFrameElement2, (void**)&ret);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLIFrameElement: %08x\n", hres);
    return ret;
}

#define get_button_iface(u) _get_button_iface(__LINE__,u)
static IHTMLButtonElement *_get_button_iface(unsigned line, IUnknown *unk)
{
    IHTMLButtonElement *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLButtonElement, (void**)&ret);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLButtonElement: %08x\n", hres);
    return ret;
}

#define get_label_iface(u) _get_label_iface(__LINE__,u)
static IHTMLLabelElement *_get_label_iface(unsigned line, IUnknown *unk)
{
    IHTMLLabelElement *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLLabelElement, (void**)&ret);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLLabelElement: %08x\n", hres);
    return ret;
}

#define get_attr2_iface(u) _get_attr2_iface(__LINE__,u)
static IHTMLDOMAttribute2 *_get_attr2_iface(unsigned line, IUnknown *unk)
{
    IHTMLDOMAttribute2 *ret;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDOMAttribute2, (void**)&ret);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDOMAttribute2: %08x\n", hres);
    return ret;
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
    ok_(__FILE__, line) (!strcmp_wa(name, exname), "got name: %s, expected %s\n", wine_dbgstr_w(name), exname);

    SysFreeString(name);
}

#define get_owner_doc(u) _get_owner_doc(__LINE__,u)
static IHTMLDocument2 *_get_owner_doc(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode2 *node = _get_node2_iface(line, unk);
    IDispatch *disp = (void*)0xdeadbeef;
    IHTMLDocument2 *doc = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode2_get_ownerDocument(node, &disp);
    IHTMLDOMNode2_Release(node);
    ok_(__FILE__,line)(hres == S_OK, "get_ownerDocument failed: %08x\n", hres);

    if(disp) {
        hres = IDispatch_QueryInterface(disp, &IID_IHTMLDocument2, (void**)&doc);
        IDispatch_Release(disp);
        ok_(__FILE__,line)(hres == S_OK, "Could not get IHTMLDocument2 iface: %08x\n", hres);
    }

    return doc;
}

#define get_doc_window(d) _get_doc_window(__LINE__,d)
static IHTMLWindow2 *_get_doc_window(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    HRESULT hres;

    window = NULL;
    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok_(__FILE__,line)(hres == S_OK, "get_parentWindow failed: %08x\n", hres);
    ok_(__FILE__,line)(window != NULL, "window == NULL\n");

    return window;
}

#define clone_node(n,d) _clone_node(__LINE__,n,d)
static IHTMLDOMNode *_clone_node(unsigned line, IUnknown *unk, VARIANT_BOOL deep)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *ret = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode_cloneNode(node, deep, &ret);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line)(hres == S_OK, "cloneNode failed: %08x\n", hres);
    ok_(__FILE__,line)(ret != NULL, "ret == NULL\n");

    return ret;

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
    ok_(__FILE__, line) (!strcmp_wa(tag, extag), "got tag: %s, expected %s\n", wine_dbgstr_w(tag), extag);

    SysFreeString(tag);
}

#define test_elem_type(ifc,t) _test_elem_type(__LINE__,ifc,t)
static void _test_elem_type(unsigned line, IUnknown *unk, elem_type_t type)
{
    _test_elem_tag(line, unk, elem_type_infos[type].tag);
    _test_ifaces(line, unk, elem_type_infos[type].iids);

    if(elem_type_infos[type].dispiid && type != ET_A)
        _test_disp(line, unk, elem_type_infos[type].dispiid, "[object]");
}

#define get_node_type(n) _get_node_type(__LINE__,n)
static LONG _get_node_type(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    LONG type = -1;
    HRESULT hres;

    hres = IHTMLDOMNode_get_nodeType(node, &type);
    ok(hres == S_OK, "get_nodeType failed: %08x\n", hres);

    IHTMLDOMNode_Release(node);

    return type;
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

#define get_child_item(c,i) _get_child_item(__LINE__,c,i)
static IHTMLDOMNode *_get_child_item(unsigned line, IHTMLDOMChildrenCollection *col, LONG idx)
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

#define test_elem_attr(e,n,v) _test_elem_attr(__LINE__,e,n,v)
static void _test_elem_attr(unsigned line, IHTMLElement *elem, const char *name, const char *exval)
{
    VARIANT value;
    BSTR tmp;
    HRESULT hres;

    VariantInit(&value);

    tmp = a2bstr(name);
    hres = IHTMLElement_getAttribute(elem, tmp, 0, &value);
    SysFreeString(tmp);
    ok_(__FILE__,line) (hres == S_OK, "getAttribute failed: %08x\n", hres);

    if(exval) {
        ok_(__FILE__,line) (V_VT(&value) == VT_BSTR, "vt=%d\n", V_VT(&value));
        ok_(__FILE__,line) (!strcmp_wa(V_BSTR(&value), exval), "unexpected value %s\n", wine_dbgstr_w(V_BSTR(&value)));
    }else {
        ok_(__FILE__,line) (V_VT(&value) == VT_NULL, "vt=%d\n", V_VT(&value));
    }

    VariantClear(&value);
}

#define test_elem_offset(a,b) _test_elem_offset(__LINE__,a,b)
static void _test_elem_offset(unsigned line, IUnknown *unk, const char *parent_tag)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    IHTMLElement *off_parent;
    LONG l;
    HRESULT hres;

    hres = IHTMLElement_get_offsetTop(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_offsetTop failed: %08x\n", hres);

    hres = IHTMLElement_get_offsetHeight(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_offsetHeight failed: %08x\n", hres);

    hres = IHTMLElement_get_offsetWidth(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_offsetWidth failed: %08x\n", hres);

    hres = IHTMLElement_get_offsetLeft(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_offsetLeft failed: %08x\n", hres);

    hres = IHTMLElement_get_offsetParent(elem, &off_parent);
    ok_(__FILE__,line) (hres == S_OK, "get_offsetParent failed: %08x\n", hres);

    _test_elem_tag(line, (IUnknown*)off_parent, parent_tag);
    IHTMLElement_Release(off_parent);

    IHTMLElement_Release(elem);
}

#define test_elem_source_index(a,b) _test_elem_source_index(__LINE__,a,b)
static void _test_elem_source_index(unsigned line, IHTMLElement *elem, int index)
{
    LONG l = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLElement_get_sourceIndex(elem, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_sourceIndex failed: %08x\n", hres);
    ok_(__FILE__,line)(l == index, "sourceIndex = %d, expected %d\n", l, index);
}

#define get_doc_node(d) _get_doc_node(__LINE__,d)
static IHTMLDocument2 *_get_doc_node(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    IHTMLDocument2 *ret;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok_(__FILE__,line)(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_get_document(window, &ret);
    ok_(__FILE__,line)(hres == S_OK, "get_document failed: %08x\n", hres);
    ok_(__FILE__,line)(ret != NULL, "document = NULL\n");

    return ret;
}

#define test_window_name(d,e) _test_window_name(__LINE__,d,e)
static void _test_window_name(unsigned line, IHTMLWindow2 *window, const char *exname)
{
    BSTR name;
    HRESULT hres;

    hres = IHTMLWindow2_get_name(window, &name);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    if(exname)
        ok_(__FILE__,line)(!strcmp_wa(name, exname), "name = %s\n", wine_dbgstr_w(name));
    else
        ok_(__FILE__,line)(!name, "name = %s\n", wine_dbgstr_w(name));
    SysFreeString(name);
}

#define set_window_name(w,n) _set_window_name(__LINE__,w,n)
static void _set_window_name(unsigned line, IHTMLWindow2 *window, const char *name)
{
    BSTR str;
    HRESULT hres;

    str = a2bstr(name);
    hres = IHTMLWindow2_put_name(window, str);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "put_name failed: %08x\n", hres);

    _test_window_name(line, window, name);
}

#define test_window_status(d) _test_window_status(__LINE__,d)
static void _test_window_status(unsigned line, IHTMLWindow2 *window)
{
    BSTR status;
    HRESULT hres;

    status = (void*)0xdeadbeef;
    hres = IHTMLWindow2_get_status(window, &status);
    ok_(__FILE__,line)(hres == S_OK, "get_status failed: %08x\n", hres);
    ok_(__FILE__,line)(!status, "status = %s\n", wine_dbgstr_w(status));
    SysFreeString(status);
}

#define set_window_status(w,n) _set_window_status(__LINE__,w,n)
static void _set_window_status(unsigned line, IHTMLWindow2 *window, const char *status)
{
    BSTR str;
    HRESULT hres;

    str = a2bstr(status);
    hres = IHTMLWindow2_put_status(window, str);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "put_status failed: %08x\n", hres);
}

#define test_window_length(w,l) _test_window_length(__LINE__,w,l)
static void _test_window_length(unsigned line, IHTMLWindow2 *window, LONG exlen)
{
    LONG length = -1;
    HRESULT hres;

    hres = IHTMLWindow2_get_length(window, &length);
    ok_(__FILE__,line)(hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line)(length == exlen, "length = %d, expected %d\n", length, exlen);
}

#define get_frame_content_window(e) _get_frame_content_window(__LINE__,e)
static IHTMLWindow2 *_get_frame_content_window(unsigned line, IUnknown *elem)
{
    IHTMLFrameBase2 *base2;
    IHTMLWindow2 *window;
    HRESULT hres;

    hres = IUnknown_QueryInterface(elem, &IID_IHTMLFrameBase2, (void**)&base2);
    ok(hres == S_OK, "Could not get IHTMFrameBase2 iface: %08x\n", hres);

    window = NULL;
    hres = IHTMLFrameBase2_get_contentWindow(base2, &window);
    IHTMLFrameBase2_Release(base2);
    ok(hres == S_OK, "get_contentWindow failed: %08x\n", hres);
    ok(window != NULL, "contentWindow = NULL\n");

    return window;
}

static void test_get_set_attr(IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    IHTMLDocument3 *doc3;
    HRESULT hres;
    BSTR bstr;
    VARIANT val;

    /* grab an element to test with */
    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument3) failed: %08x\n", hres);

    hres = IHTMLDocument3_get_documentElement(doc3, &elem);
    IHTMLDocument3_Release(doc3);
    ok(hres == S_OK, "get_documentElement failed: %08x\n", hres);

    /* get a non-present attribute */
    bstr = a2bstr("notAnAttribute");
    hres = IHTMLElement_getAttribute(elem, bstr, 0, &val);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&val) == VT_NULL, "variant type should have been VT_NULL (0x%x), was: 0x%x\n", VT_NULL, V_VT(&val));
    VariantClear(&val);
    SysFreeString(bstr);

    /* get a present attribute */
    bstr = a2bstr("scrollHeight");
    hres = IHTMLElement_getAttribute(elem, bstr, 0, &val);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&val) == VT_I4, "variant type should have been VT_I4 (0x%x), was: 0x%x\n", VT_I4, V_VT(&val));
    VariantClear(&val);
    SysFreeString(bstr);

    /* create a new BSTR attribute */
    bstr = a2bstr("newAttribute");

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = a2bstr("the value");
    hres = IHTMLElement_setAttribute(elem, bstr, val, 0);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);
    VariantClear(&val);

    hres = IHTMLElement_getAttribute(elem, bstr, 0, &val);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&val) == VT_BSTR, "variant type should have been VT_BSTR (0x%x), was: 0x%x\n", VT_BSTR, V_VT(&val));
    ok(strcmp_wa(V_BSTR(&val), "the value") == 0, "variant value should have been L\"the value\", was %s\n", wine_dbgstr_w(V_BSTR(&val)));
    VariantClear(&val);

    /* overwrite the attribute with a BOOL */
    V_VT(&val) = VT_BOOL;
    V_BOOL(&val) = VARIANT_TRUE;
    hres = IHTMLElement_setAttribute(elem, bstr, val, 0);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);
    VariantClear(&val);

    hres = IHTMLElement_getAttribute(elem, bstr, 0, &val);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&val) == VT_BOOL, "variant type should have been VT_BOOL (0x%x), was: 0x%x\n", VT_BOOL, V_VT(&val));
    ok(V_BOOL(&val) == VARIANT_TRUE, "variant value should have been VARIANT_TRUE (0x%x), was %d\n", VARIANT_TRUE, V_BOOL(&val));
    VariantClear(&val);

    SysFreeString(bstr);

    /* case-insensitive */
    bstr = a2bstr("newattribute");
    hres = IHTMLElement_getAttribute(elem, bstr, 0, &val);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&val) == VT_BOOL, "variant type should have been VT_BOOL (0x%x), was: 0x%x\n", VT_BOOL, V_VT(&val));
    ok(V_BOOL(&val) == VARIANT_TRUE, "variant value should have been VARIANT_TRUE (0x%x), was %d\n", VARIANT_TRUE, V_BOOL(&val));
    VariantClear(&val);
    SysFreeString(bstr);

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

#define test_anchor_href(a,h) _test_anchor_href(__LINE__,a,h)
static void _test_anchor_href(unsigned line, IUnknown *unk, const char *exhref)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_href(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_href failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(str, exhref), "href = %s, expected %s\n", wine_dbgstr_w(str), exhref);
    SysFreeString(str);

    _test_disp_value(line, unk, exhref);
}

#define test_anchor_put_href(a,h) _test_anchor_put_href(__LINE__,a,h)
static void _test_anchor_put_href(unsigned line, IUnknown *unk, const char *exhref)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = a2bstr(exhref);
    hres = IHTMLAnchorElement_put_href(anchor, str);
    ok_(__FILE__,line)(hres == S_OK, "get_href failed: %08x\n", hres);
    SysFreeString(str);

    _test_disp_value(line, unk, exhref);
}

#define test_anchor_rel(a,h) _test_anchor_rel(__LINE__,a,h)
static void _test_anchor_rel(unsigned line, IUnknown *unk, const char *exrel)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_rel(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_rel failed: %08x\n", hres);
    if(exrel)
        ok_(__FILE__,line)(!strcmp_wa(str, exrel), "rel = %s, expected %s\n", wine_dbgstr_w(str), exrel);
    else
        ok_(__FILE__,line)(!str, "rel = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define test_anchor_put_rel(a,h) _test_anchor_put_rel(__LINE__,a,h)
static void _test_anchor_put_rel(unsigned line, IUnknown *unk, const char *exrel)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = a2bstr(exrel);
    hres = IHTMLAnchorElement_put_rel(anchor, str);
    ok_(__FILE__,line)(hres == S_OK, "get_rel failed: %08x\n", hres);
    SysFreeString(str);
}

#define test_anchor_get_target(a,h) _test_anchor_get_target(__LINE__,a,h)
static void _test_anchor_get_target(unsigned line, IUnknown *unk, const char *target)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_target(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_target failed: %08x\n", hres);
    if(target)
        ok_(__FILE__,line)(!strcmp_wa(str, target), "target = %s, expected %s\n", wine_dbgstr_w(str), target);
    else
        ok_(__FILE__,line)(str == NULL, "target = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define test_anchor_put_target(a,h) _test_anchor_put_target(__LINE__,a,h)
static void _test_anchor_put_target(unsigned line, IUnknown *unk, const char *target)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = target ? a2bstr(target) : NULL;
    hres = IHTMLAnchorElement_put_target(anchor, str);
    ok_(__FILE__,line)(hres == S_OK, "put_target failed: %08x\n", hres);
    SysFreeString(str);
}

#define test_anchor_name(a,h) _test_anchor_name(__LINE__,a,h)
static void _test_anchor_name(unsigned line, IUnknown *unk, const char *name)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_name(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    if(name)
        ok_(__FILE__,line)(!strcmp_wa(str, name), "name = %s, expected %s\n", wine_dbgstr_w(str), name);
    else
        ok_(__FILE__,line)(str == NULL, "name = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define test_anchor_put_name(a,h) _test_anchor_put_name(__LINE__,a,h)
static void _test_anchor_put_name(unsigned line, IUnknown *unk, const char *name)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = name ? a2bstr(name) : NULL;
    hres = IHTMLAnchorElement_put_name(anchor, str);
    ok_(__FILE__,line)(hres == S_OK, "put_name failed: %08x\n", hres);
    SysFreeString(str);

    _test_anchor_name(line, unk, name);
}

#define test_anchor_hostname(a,h) _test_anchor_hostname(__LINE__,a,h)
static void _test_anchor_hostname(unsigned line, IUnknown *unk, const char *hostname)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_hostname(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    if(hostname)
        ok_(__FILE__,line)(!strcmp_wa(str, hostname), "hostname = %s, expected %s\n", wine_dbgstr_w(str), hostname);
    else
        ok_(__FILE__,line)(str == NULL, "hostname = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define test_anchor_search(a,h,n) _test_anchor_search(__LINE__,a,h,n)
static void _test_anchor_search(unsigned line, IUnknown *elem, const char *search, BOOL allowbroken)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, elem);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_search(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_search failed: %08x\n", hres);
    if ( ! str && allowbroken)
        win_skip("skip ie6 incorrect behavior\n");
    else if(search)
        ok_(__FILE__,line)(!strcmp_wa(str, search), "search = %s, expected %s\n", wine_dbgstr_w(str), search);
    else
        ok_(__FILE__,line)(!str, "search = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define test_anchor_put_search(a,h) _test_anchor_put_search(__LINE__,a,h)
static void _test_anchor_put_search(unsigned line, IUnknown *unk, const char *search)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = search ? a2bstr(search) : NULL;
    hres = IHTMLAnchorElement_put_search(anchor, str);
    ok_(__FILE__,line)(hres == S_OK, "put_search failed: %08x\n", hres);
    SysFreeString(str);
}

#define test_anchor_hash(a,h) _test_anchor_hash(__LINE__,a,h)
static void _test_anchor_hash(unsigned line, IHTMLElement *elem, const char *exhash)
{
    IHTMLAnchorElement *anchor = _get_anchor_iface(line, (IUnknown*)elem);
    BSTR str;
    HRESULT hres;

    hres = IHTMLAnchorElement_get_hash(anchor, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_hash failed: %08x\n", hres);
    if(exhash)
        ok_(__FILE__,line)(!strcmp_wa(str, exhash), "hash = %s, expected %s\n", wine_dbgstr_w(str), exhash);
    else
        ok_(__FILE__,line)(!str, "hash = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define test_option_text(o,t) _test_option_text(__LINE__,o,t)
static void _test_option_text(unsigned line, IHTMLOptionElement *option, const char *text)
{
    BSTR bstr;
    HRESULT hres;

    hres = IHTMLOptionElement_get_text(option, &bstr);
    ok_(__FILE__,line) (hres == S_OK, "get_text failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(bstr, text), "text=%s\n", wine_dbgstr_w(bstr));
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
    ok_(__FILE__,line) (!strcmp_wa(bstr, value), "value=%s\n", wine_dbgstr_w(bstr));
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

#define test_option_selected(o,s) _test_option_selected(__LINE__,o,s)
static void _test_option_selected(unsigned line, IHTMLOptionElement *option, VARIANT_BOOL ex)
{
    VARIANT_BOOL b = 0x100;
    HRESULT hres;

    hres = IHTMLOptionElement_get_selected(option, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_selected failed: %08x\n", hres);
    ok_(__FILE__,line)(b == ex, "selected = %x, expected %x\n", b, ex);
}

#define test_option_put_selected(o,s) _test_option_put_selected(__LINE__,o,s)
static void _test_option_put_selected(unsigned line, IHTMLOptionElement *option, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLOptionElement_put_selected(option, b);
    ok_(__FILE__,line)(hres == S_OK, "put_selected failed: %08x\n", hres);
    _test_option_selected(line, option, b);
}

#define test_option_get_index(o,s) _test_option_get_index(__LINE__,o,s)
static void _test_option_get_index(unsigned line, IHTMLOptionElement *option, LONG exval)
{
    HRESULT hres;
    LONG val;

    hres = IHTMLOptionElement_get_index(option, NULL);
    ok_(__FILE__,line)(hres == E_INVALIDARG, "Expect E_INVALIDARG, got %08x\n", hres);

    val = 12345678;
    hres = IHTMLOptionElement_get_index(option, &val);
    ok_(__FILE__,line)(hres == S_OK, "get_index failed: %08x\n", hres);
    ok_(__FILE__,line)(val == exval || broken(val == 12345678),  /* Win2k doesn't touch it*/
        "value = %d, expected = %d\n", val, exval);
}

#define test_option_put_defaultSelected(o,d) _test_option_put_defaultSelected(__LINE__,o,d)
static void _test_option_put_defaultSelected(unsigned line, IHTMLOptionElement *option, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLOptionElement_put_defaultSelected(option, b);
    ok_(__FILE__,line)(hres == S_OK, "put_defaultSelected %08x\n", hres);
}

#define test_option_defaultSelected(o,e) _test_option_defaultSelected(__LINE__,o,e)
static void _test_option_defaultSelected(unsigned line, IHTMLOptionElement *option, VARIANT_BOOL ex)
{
    HRESULT hres;
    VARIANT_BOOL b;

    hres = IHTMLOptionElement_get_defaultSelected(option, NULL);
    ok_(__FILE__,line)(hres == E_POINTER, "Expect E_POINTER, got %08x\n", hres);

    b = 0x100;
    hres = IHTMLOptionElement_get_defaultSelected(option, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_defaultSelected failed: %08x\n", hres);
    ok_(__FILE__,line)(b == ex, "b = %x, expected = %x\n", b, ex);
}

static void test_option_defaultSelected_property(IHTMLOptionElement *option)
{
    test_option_defaultSelected(option, VARIANT_FALSE);
    test_option_selected(option, VARIANT_FALSE);

    test_option_put_defaultSelected(option, 0x100); /* Invalid value */
    test_option_defaultSelected(option, VARIANT_FALSE);
    test_option_selected(option, VARIANT_FALSE);

    test_option_put_defaultSelected(option, VARIANT_TRUE);
    test_option_defaultSelected(option, VARIANT_TRUE);
    test_option_selected(option, VARIANT_FALSE);

    test_option_put_defaultSelected(option, 0x100); /* Invalid value */
    test_option_defaultSelected(option, VARIANT_FALSE);
    test_option_selected(option, VARIANT_FALSE);

    test_option_put_selected(option, VARIANT_TRUE);
    test_option_selected(option, VARIANT_TRUE);
    test_option_defaultSelected(option, VARIANT_FALSE);

    test_option_put_defaultSelected(option, VARIANT_TRUE);
    test_option_defaultSelected(option, VARIANT_TRUE);
    test_option_selected(option, VARIANT_TRUE);

    /* Restore defaultSelected */
    test_option_put_defaultSelected(option, VARIANT_TRUE);
    test_option_put_selected(option, VARIANT_FALSE);
}

#define test_textarea_value(t,v) _test_textarea_value(__LINE__,t,v)
static void _test_textarea_value(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    BSTR value = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTextAreaElement_get_value(textarea, &value);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "get_value failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(value, exval), "value = %s, expected %s\n", wine_dbgstr_w(value), exval);
    else
        ok_(__FILE__,line)(!value, "value = %p\n", value);
    SysFreeString(value);
}

#define test_textarea_put_value(t,v) _test_textarea_put_value(__LINE__,t,v)
static void _test_textarea_put_value(unsigned line, IUnknown *unk, const char *value)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    BSTR tmp = a2bstr(value);
    HRESULT hres;

    hres = IHTMLTextAreaElement_put_value(textarea, tmp);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "put_value failed: %08x\n", hres);
    SysFreeString(tmp);

    _test_textarea_value(line, unk, value);
}

#define test_textarea_defaultvalue(t,v) _test_textarea_defaultvalue(__LINE__,t,v)
static void _test_textarea_defaultvalue(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    BSTR value = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTextAreaElement_get_defaultValue(textarea, &value);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "get_defaultValue failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(value, exval), "defaultValue = %s, expected %s\n", wine_dbgstr_w(value), exval);
    else
        ok_(__FILE__,line)(!value, "value = %p\n", value);
    SysFreeString(value);
}

#define test_textarea_put_defaultvalue(t,v) _test_textarea_put_defaultvalue(__LINE__,t,v)
static void _test_textarea_put_defaultvalue(unsigned line, IUnknown *unk, const char *value)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    BSTR tmp = a2bstr(value);
    HRESULT hres;

    hres = IHTMLTextAreaElement_put_defaultValue(textarea, tmp);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "put_defaultValue failed: %08x\n", hres);
    SysFreeString(tmp);

    _test_textarea_defaultvalue(line, unk, value);
}

#define test_textarea_readonly(t,v) _test_textarea_readonly(__LINE__,t,v)
static void _test_textarea_readonly(unsigned line, IUnknown *unk, VARIANT_BOOL ex)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    VARIANT_BOOL b = 0x100;
    HRESULT hres;

    hres = IHTMLTextAreaElement_get_readOnly(textarea, &b);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "get_readOnly failed: %08x\n", hres);
    ok_(__FILE__,line)(b == ex, "readOnly = %x, expected %x\n", b, ex);
}

#define test_textarea_put_readonly(t,v) _test_textarea_put_readonly(__LINE__,t,v)
static void _test_textarea_put_readonly(unsigned line, IUnknown *unk, VARIANT_BOOL b)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    HRESULT hres;

    hres = IHTMLTextAreaElement_put_readOnly(textarea, b);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "put_readOnly failed: %08x\n", hres);

    _test_textarea_readonly(line, unk, b);
}

#define test_textarea_type(t) _test_textarea_type(__LINE__,t)
static void _test_textarea_type(unsigned line, IUnknown *unk)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    BSTR type = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTextAreaElement_get_type(textarea, &type);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "get_type failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(type, "textarea"), "type = %s, expected textarea\n", wine_dbgstr_w(type));
    SysFreeString(type);
}

#define get_textarea_form(t) _get_textarea_form(__LINE__,t)
static IHTMLFormElement *_get_textarea_form(unsigned line, IUnknown *unk)
{
    IHTMLTextAreaElement *textarea = _get_textarea_iface(line, unk);
    IHTMLFormElement *form;
    HRESULT hres;

    hres = IHTMLTextAreaElement_get_form(textarea, &form);
    IHTMLTextAreaElement_Release(textarea);
    ok_(__FILE__,line)(hres == S_OK, "get_type failed: %08x\n", hres);

    return form;
}

#define test_comment_text(c,t) _test_comment_text(__LINE__,c,t)
static void _test_comment_text(unsigned line, IUnknown *unk, const char *extext)
{
    IHTMLCommentElement *comment = _get_comment_iface(__LINE__,unk);
    BSTR text;
    HRESULT hres;

    text = a2bstr(extext);
    hres = IHTMLCommentElement_get_text(comment, &text);
    ok_(__FILE__,line)(hres == S_OK, "get_text failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(text, extext), "text = \"%s\", expected \"%s\"\n", wine_dbgstr_w(text), extext);

    IHTMLCommentElement_Release(comment);
    SysFreeString(text);
}

#define test_attr_specified(a,b) _test_attr_specified(__LINE__,a,b)
static void _test_attr_specified(unsigned line, IHTMLDOMAttribute *attr, VARIANT_BOOL expected)
{
    VARIANT_BOOL specified;
    HRESULT hres;

    hres = IHTMLDOMAttribute_get_specified(attr, &specified);
    ok_(__FILE__,line)(hres == S_OK, "get_specified failed: %08x\n", hres);
    ok_(__FILE__,line)(specified == expected, "specified = %x, expected %x\n", specified, expected);
}

#define test_attr_expando(a,b) _test_attr_expando(__LINE__,a,b)
static void _test_attr_expando(unsigned line, IHTMLDOMAttribute *attr, VARIANT_BOOL expected)
{
    IHTMLDOMAttribute2 *attr2 = _get_attr2_iface(line, (IUnknown*)attr);
    VARIANT_BOOL expando;
    HRESULT hres;

    hres = IHTMLDOMAttribute2_get_expando(attr2, &expando);
    ok_(__FILE__,line)(hres == S_OK, "get_expando failed: %08x\n", hres);
    ok_(__FILE__,line)(expando == expected, "expando = %x, expected %x\n", expando, expected);

    IHTMLDOMAttribute2_Release(attr2);
}

#define test_attr_value(a,b) _test_attr_value(__LINE__,a,b)
static void _test_attr_value(unsigned line, IHTMLDOMAttribute *attr, const char *exval)
{
    IHTMLDOMAttribute2 *attr2 = _get_attr2_iface(line, (IUnknown*)attr);
    BSTR val;
    HRESULT hres;

    hres = IHTMLDOMAttribute2_get_value(attr2, &val);
    ok_(__FILE__,line)(hres == S_OK, "get_value failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(val, exval), "value = %s, expected %s\n", wine_dbgstr_w(val), exval);
    else
        ok_(__FILE__,line)(!val, "value = %s, expected NULL\n", wine_dbgstr_w(val));

    IHTMLDOMAttribute2_Release(attr2);
    SysFreeString(val);
}

#define test_comment_attrs(c) _test_comment_attrs(__LINE__,c)
static void _test_comment_attrs(unsigned line, IUnknown *unk)
{
    IHTMLCommentElement *comment = _get_comment_iface(__LINE__,unk);
    IHTMLElement *elem = _get_elem_iface(__LINE__,unk);
    IHTMLElement4 *elem4 = _get_elem4_iface(__LINE__,unk);
    IHTMLDOMAttribute *attr;
    BSTR name = a2bstr("test");
    VARIANT val;
    HRESULT hres;

    hres = IHTMLElement4_getAttributeNode(elem4, name, &attr);
    ok(hres == S_OK, "getAttributeNode failed: %08x\n", hres);
    ok(attr == NULL, "attr != NULL\n");

    V_VT(&val) = VT_I4;
    V_I4(&val) = 1234;
    hres = IHTMLElement_setAttribute(elem, name, val, 0);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);

    hres = IHTMLElement4_getAttributeNode(elem4, name, &attr);
    ok(hres == S_OK, "getAttributeNode failed: %08x\n", hres);
    ok(attr != NULL, "attr == NULL\n");

    test_attr_expando(attr, VARIANT_TRUE);

    IHTMLDOMAttribute_Release(attr);
    IHTMLCommentElement_Release(comment);
    IHTMLElement_Release(elem);
    IHTMLElement4_Release(elem4);
    SysFreeString(name);
}

#define test_object_vspace(u,s) _test_object_vspace(__LINE__,u,s)
static void _test_object_vspace(unsigned line, IUnknown *unk, LONG exl)
{
    IHTMLObjectElement *object = _get_object_iface(line, unk);
    LONG l;
    HRESULT hres;

    l = 0xdeadbeef;
    hres = IHTMLObjectElement_get_vspace(object, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_vspace failed: %08x\n", hres);
    ok_(__FILE__,line)(l == exl, "vspace=%d, expected %d\n", l, exl);
    IHTMLObjectElement_Release(object);
}

#define test_object_name(a,b) _test_object_name(__LINE__,a,b)
static void _test_object_name(unsigned line, IHTMLElement *elem, const char *exname)
{
    IHTMLObjectElement *object = _get_object_iface(line, (IUnknown*)elem);
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLObjectElement_get_name(object, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    if(exname)
        ok_(__FILE__,line)(!strcmp_wa(str, exname), "name=%s, expected %s\n", wine_dbgstr_w(str), exname);
    else
        ok_(__FILE__,line)(!str, "name=%s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IHTMLObjectElement_Release(object);
}

#define set_object_name(a,b) _set_object_name(__LINE__,a,b)
static void _set_object_name(unsigned line, IHTMLElement *elem, const char *name)
{
    IHTMLObjectElement *object = _get_object_iface(line, (IUnknown*)elem);
    BSTR str;
    HRESULT hres;

    str = a2bstr(name);
    hres = IHTMLObjectElement_put_name(object, str);
    ok_(__FILE__,line)(hres == S_OK, "put_name failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLObjectElement_Release(object);

    _test_object_name(line, elem, name);
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

    test_disp((IUnknown*)factory, &IID_IHTMLOptionElementFactory, "[object]");

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
    _test_option_selected(line, option, VARIANT_FALSE);

    return option;
}

#define test_img_width(o,w) _test_img_width(__LINE__,o,w)
static void _test_img_width(unsigned line, IHTMLImgElement *img, const LONG exp)
{
    LONG found = -1;
    HRESULT hres;

    hres = IHTMLImgElement_get_width(img, &found);
    ok_(__FILE__,line) (hres == S_OK, "get_width failed: %08x\n", hres);
    ok_(__FILE__,line) (found == exp, "width=%d\n", found);
}

#define test_img_put_width(o,w) _test_img_put_width(__LINE__,o,w)
static void _test_img_put_width(unsigned line, IHTMLImgElement *img, const LONG width)
{
    HRESULT hres;

    hres = IHTMLImgElement_put_width(img, width);
    ok(hres == S_OK, "put_width failed: %08x\n", hres);

    _test_img_width(line, img, width);
}

#define test_img_height(o,h) _test_img_height(__LINE__,o,h)
static void _test_img_height(unsigned line, IHTMLImgElement *img, const LONG exp)
{
    LONG found = -1;
    HRESULT hres;

    hres = IHTMLImgElement_get_height(img, &found);
    ok_(__FILE__,line) (hres == S_OK, "get_height failed: %08x\n", hres);
    ok_(__FILE__,line) (found == exp, "height=%d\n", found);
}

#define test_img_put_height(o,w) _test_img_put_height(__LINE__,o,w)
static void _test_img_put_height(unsigned line, IHTMLImgElement *img, const LONG height)
{
    HRESULT hres;

    hres = IHTMLImgElement_put_height(img, height);
    ok(hres == S_OK, "put_height failed: %08x\n", hres);

    _test_img_height(line, img, height);
}

#define create_img_elem(d,t,v) _create_img_elem(__LINE__,d,t,v)
static IHTMLImgElement *_create_img_elem(unsigned line, IHTMLDocument2 *doc,
        LONG wdth, LONG hght)
{
    IHTMLImageElementFactory *factory;
    IHTMLImgElement *img;
    IHTMLWindow2 *window;
    VARIANT width, height;
    char buf[16];
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok_(__FILE__,line) (hres == S_OK, "get_parentElement failed: %08x\n", hres);

    hres = IHTMLWindow2_get_Image(window, &factory);
    IHTMLWindow2_Release(window);
    ok_(__FILE__,line) (hres == S_OK, "get_Image failed: %08x\n", hres);

    test_ifaces((IUnknown*)factory, img_factory_iids);
    test_disp((IUnknown*)factory, &IID_IHTMLImageElementFactory, "[object]");

    if(wdth >= 0){
        snprintf(buf, 16, "%d", wdth);
        V_VT(&width) = VT_BSTR;
        V_BSTR(&width) = a2bstr(buf);
    }else{
        V_VT(&width) = VT_EMPTY;
        wdth = 0;
    }

    if(hght >= 0){
        snprintf(buf, 16, "%d", hght);
        V_VT(&height) = VT_BSTR;
        V_BSTR(&height) = a2bstr(buf);
    }else{
        V_VT(&height) = VT_EMPTY;
        hght = 0;
    }

    hres = IHTMLImageElementFactory_create(factory, width, height, &img);
    ok_(__FILE__,line) (hres == S_OK, "create failed: %08x\n", hres);

    IHTMLImageElementFactory_Release(factory);
    VariantClear(&width);
    VariantClear(&height);

    if(SUCCEEDED(hres)) {
        _test_img_width(line, img, wdth);
        _test_img_height(line, img, hght);
        return img;
    }

    return NULL;
}

#define test_select_length(s,l) _test_select_length(__LINE__,s,l)
static void _test_select_length(unsigned line, IHTMLSelectElement *select, LONG length)
{
    LONG len = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLSelectElement_get_length(select, &len);
    ok_(__FILE__,line) (hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line) (len == length, "len=%d, expected %d\n", len, length);
}

#define test_select_put_length(s,l) _test_select_put_length(__LINE__,s,l)
static void _test_select_put_length(unsigned line, IUnknown *unk, LONG length)
{
    IHTMLSelectElement *select = _get_select_iface(line, unk);
    HRESULT hres;

    hres = IHTMLSelectElement_put_length(select, length);
    ok_(__FILE__,line) (hres == S_OK, "put_length failed: %08x\n", hres);
    _test_select_length(line, select, length);
    IHTMLSelectElement_Release(select);
}

#define test_select_selidx(s,i) _test_select_selidx(__LINE__,s,i)
static void _test_select_selidx(unsigned line, IHTMLSelectElement *select, LONG index)
{
    LONG idx = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLSelectElement_get_selectedIndex(select, &idx);
    ok_(__FILE__,line) (hres == S_OK, "get_selectedIndex failed: %08x\n", hres);
    ok_(__FILE__,line) (idx == index, "idx=%d, expected %d\n", idx, index);
}

#define test_select_put_selidx(s,i) _test_select_put_selidx(__LINE__,s,i)
static void _test_select_put_selidx(unsigned line, IHTMLSelectElement *select, LONG index)
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
        ok_(__FILE__,line) (!strcmp_wa(val, exval), "unexpected value %s\n", wine_dbgstr_w(val));
    else
        ok_(__FILE__,line) (val == NULL, "val=%s, expected NULL\n", wine_dbgstr_w(val));
    SysFreeString(val);
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
    ok_(__FILE__,line) (!strcmp_wa(type, extype), "type=%s, expected %s\n", wine_dbgstr_w(type), extype);
    SysFreeString(type);
}

#define test_select_multiple(s,t) _test_select_multiple(__LINE__,s,t)
static void _test_select_multiple(unsigned line, IHTMLSelectElement *select, VARIANT_BOOL exmultiple)
{
    VARIANT_BOOL b = 100;
    HRESULT hres;

    hres = IHTMLSelectElement_get_multiple(select, &b);
    ok_(__FILE__,line) (hres == S_OK, "get_multiple failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exmultiple, "multiple=%x, expected %x\n", b, exmultiple);
}

#define test_select_set_multiple(s,v) _test_select_set_multiple(__LINE__,s,v)
static void _test_select_set_multiple(unsigned line, IHTMLSelectElement *select, VARIANT_BOOL val)
{
    HRESULT hres;

    hres = IHTMLSelectElement_put_multiple(select, val);
    ok_(__FILE__,line) (hres == S_OK, "put_multiple failed: %08x\n", hres);

    _test_select_multiple(line, select, val);
}

#define test_select_size(s,v) _test_select_size(__LINE__,s,v)
static void _test_select_size(unsigned line, IHTMLSelectElement *select, LONG exval)
{
    HRESULT hres;
    LONG val;

    hres = IHTMLSelectElement_get_size(select, NULL);
    ok_(__FILE__,line) (hres == E_INVALIDARG, "got %08x, expected E_INVALIDARG\n", hres);

    val = 0xdeadbeef;
    hres = IHTMLSelectElement_get_size(select, &val);
    ok_(__FILE__,line) (hres == S_OK, "get_size failed: %08x\n", hres);
    ok_(__FILE__,line) (val == exval, "size = %d, expected %d\n", val, exval);
}

#define test_select_set_size(s,v,e) _test_select_set_size(__LINE__,s,v,e)
static void _test_select_set_size(unsigned line, IHTMLSelectElement *select, LONG val, HRESULT exhres)
{
    HRESULT hres;

    hres = IHTMLSelectElement_put_size(select, val);
    ok_(__FILE__,line) (hres == exhres, "put_size(%d) got %08x, expect %08x\n", val, hres, exhres);
}

#define test_select_name(s,v) _test_select_name(__LINE__,s,v)
static void _test_select_name(unsigned line, IHTMLSelectElement *select, const char *extext)
{
    HRESULT hres;
    BSTR text;

    text = NULL;
    hres = IHTMLSelectElement_get_name(select, &text);
    ok_(__FILE__,line) (hres == S_OK, "get_name failed: %08x\n", hres);
    if(extext) {
        ok_(__FILE__,line) (text != NULL, "text == NULL\n");
        ok_(__FILE__,line) (!strcmp_wa(text, extext), "name = %s, expected %s\n",
            wine_dbgstr_w(text), extext);
        SysFreeString(text);
    } else
        ok_(__FILE__,line) (text == NULL, "text(%p) = %s\n", text, wine_dbgstr_w(text));
}

#define test_select_set_name(s,v) _test_select_set_name(__LINE__,s,v)
static void _test_select_set_name(unsigned line, IHTMLSelectElement *select, const char *text)
{
    HRESULT hres;
    BSTR bstr;

    bstr = a2bstr(text);

    hres = IHTMLSelectElement_put_name(select, bstr);
    ok_(__FILE__,line) (hres == S_OK, "put_name(%s) failed: %08x\n", wine_dbgstr_w(bstr), hres);
    SysFreeString(bstr);
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
        ok_(__FILE__, line) (!strcmp_wa(text, extext), "text=%s, expected %s\n", wine_dbgstr_w(text), extext);
    }else {
        ok_(__FILE__, line) (text == NULL, "text=%s, expected NULL\n", wine_dbgstr_w(text));
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
static void _test_range_move(unsigned line, IHTMLTxtRange *range, LPWSTR unit, LONG cnt, LONG excnt)
{
    LONG c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_move(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%d, expected %d\n", c, excnt);
    _test_range_text(line, range, NULL);
}

#define test_range_movestart(r,u,c,e) _test_range_movestart(__LINE__,r,u,c,e)
static void _test_range_movestart(unsigned line, IHTMLTxtRange *range,
        LPWSTR unit, LONG cnt, LONG excnt)
{
    LONG c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_moveStart(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%d, expected %d\n", c, excnt);
}

#define test_range_moveend(r,u,c,e) _test_range_moveend(__LINE__,r,u,c,e)
static void _test_range_moveend(unsigned line, IHTMLTxtRange *range, LPWSTR unit, LONG cnt, LONG excnt)
{
    LONG c = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLTxtRange_moveEnd(range, unit, cnt, &c);
    ok_(__FILE__,line) (hres == S_OK, "move failed: %08x\n", hres);
    ok_(__FILE__,line) (c == excnt, "count=%d, expected %d\n", c, excnt);
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

#define test_range_paste_html(a,b) _test_range_paste_html(__LINE__,a,b)
static void _test_range_paste_html(unsigned line, IHTMLTxtRange *range, const char *html)
{
    BSTR str = a2bstr(html);
    HRESULT hres;

    hres = IHTMLTxtRange_pasteHTML(range, str);
    ok_(__FILE__,line)(hres == S_OK, "pasteHTML failed: %08x\n", hres);
    SysFreeString(str);
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

#define get_elem_col_item_idx(a,b) _get_elem_col_item_idx(__LINE__,a,b)
static IHTMLElement *_get_elem_col_item_idx(unsigned line, IHTMLElementCollection *col, int i)
{
    VARIANT name, index;
    IHTMLElement *elem;
    IDispatch *disp;
    HRESULT hres;

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_I4;
    V_I4(&name) = i;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line)(hres == S_OK, "item failed: %08x\n", hres);
    ok_(__FILE__,line)(disp != NULL, "disp == NULL\n");

    elem = _get_elem_iface(line, (IUnknown*)disp);
    IDispatch_Release(disp);
    return elem;
}

#define test_elem_collection(c,t,l) _test_elem_collection(__LINE__,c,t,l)
static void _test_elem_collection(unsigned line, IUnknown *unk,
        const elem_type_t *elem_types, LONG exlen)
{
    IHTMLElementCollection *col;
    IEnumVARIANT *enum_var;
    IUnknown *enum_unk;
    ULONG fetched;
    LONG len;
    DWORD i;
    VARIANT name, index, v, vs[5];
    IDispatch *disp, *disp2;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElementCollection, (void**)&col);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElementCollection: %08x\n", hres);

    test_disp((IUnknown*)col, &DIID_DispHTMLElementCollection, "[object]");

    hres = IHTMLElementCollection_get_length(col, &len);
    ok_(__FILE__,line) (hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line) (len == exlen, "len=%d, expected %d\n", len, exlen);

    if(len > exlen)
        len = exlen;

    V_VT(&index) = VT_EMPTY;

    hres = IHTMLElementCollection_get__newEnum(col, &enum_unk);
    ok_(__FILE__,line)(hres == S_OK, "_newEnum failed: %08x\n", hres);

    hres = IUnknown_QueryInterface(enum_unk, &IID_IEnumVARIANT, (void**)&enum_var);
    IUnknown_Release(enum_unk);
    ok_(__FILE__,line)(hres == S_OK, "Could not get IEnumVARIANT iface: %08x\n", hres);

    for(i=0; i<len; i++) {
        V_VT(&name) = VT_I4;
        V_I4(&name) = i;
        disp = (void*)0xdeadbeef;
        hres = IHTMLElementCollection_item(col, name, index, &disp);
        ok_(__FILE__,line) (hres == S_OK, "item(%d) failed: %08x\n", i, hres);
        ok_(__FILE__,line) (disp != NULL, "item returned NULL\n");
        if(FAILED(hres) || !disp)
            continue;

        _test_elem_type(line, (IUnknown*)disp, elem_types[i]);

        if(!i) {
            V_VT(&name) = VT_UINT;
            V_I4(&name) = 0;
            disp2 = (void*)0xdeadbeef;
            hres = IHTMLElementCollection_item(col, name, index, &disp2);
            ok_(__FILE__,line) (hres == S_OK, "item(%d) failed: %08x\n", i, hres);
            ok_(__FILE__,line) (iface_cmp((IUnknown*)disp, (IUnknown*)disp2), "disp != disp2\n");
            if(disp2)
                IDispatch_Release(disp2);
        }

        fetched = 0;
        V_VT(&v) = VT_ERROR;
        hres = IEnumVARIANT_Next(enum_var, 1, &v, i ? &fetched : NULL);
        ok_(__FILE__,line)(hres == S_OK, "Next failed: %08x\n", hres);
        if(i)
            ok_(__FILE__,line)(fetched == 1, "fetched = %d\n", fetched);
        ok_(__FILE__,line)(V_VT(&v) == VT_DISPATCH && V_DISPATCH(&v), "V_VT(v) = %d\n", V_VT(&v));
        ok_(__FILE__,line)(iface_cmp((IUnknown*)disp, (IUnknown*)V_DISPATCH(&v)), "disp != V_DISPATCH(v)\n");
        IDispatch_Release(V_DISPATCH(&v));

        IDispatch_Release(disp);
    }

    fetched = 0xdeadbeef;
    V_VT(&v) = VT_BOOL;
    hres = IEnumVARIANT_Next(enum_var, 1, &v, &fetched);
    ok_(__FILE__,line)(hres == S_FALSE, "Next returned %08x, expected S_FALSE\n", hres);
    ok_(__FILE__,line)(fetched == 0, "fetched = %d\n", fetched);
    ok_(__FILE__,line)(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    hres = IEnumVARIANT_Reset(enum_var);
    ok_(__FILE__,line)(hres == S_OK, "Reset failed: %08x\n", hres);

    fetched = 0xdeadbeef;
    V_VT(&v) = VT_BOOL;
    hres = IEnumVARIANT_Next(enum_var, 0, &v, &fetched);
    ok_(__FILE__,line)(hres == S_OK, "Next returned %08x, expected S_FALSE\n", hres);
    ok_(__FILE__,line)(fetched == 0, "fetched = %d\n", fetched);
    ok_(__FILE__,line)(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    hres = IEnumVARIANT_Skip(enum_var, len > 2 ? len-2 : 0);
    ok_(__FILE__,line)(hres == S_OK, "Skip failed: %08x\n", hres);

    memset(vs, 0, sizeof(vs));
    fetched = 0;
    hres = IEnumVARIANT_Next(enum_var, sizeof(vs)/sizeof(*vs), vs, &fetched);
    ok_(__FILE__,line)(hres == S_FALSE, "Next failed: %08x\n", hres);
    ok_(__FILE__,line)(fetched == (len > 2 ? 2 : len), "fetched = %d\n", fetched);
    if(len) {
        ok_(__FILE__,line)(V_VT(vs) == VT_DISPATCH && V_DISPATCH(vs), "V_VT(vs[0]) = %d\n", V_VT(vs));
        IDispatch_Release(V_DISPATCH(vs));
    }
    if(len > 1) {
        ok_(__FILE__,line)(V_VT(vs+1) == VT_DISPATCH && V_DISPATCH(vs+1), "V_VT(vs[1]) = %d\n", V_VT(vs+1));
        IDispatch_Release(V_DISPATCH(vs+1));
    }

    hres = IEnumVARIANT_Reset(enum_var);
    ok_(__FILE__,line)(hres == S_OK, "Reset failed: %08x\n", hres);

    hres = IEnumVARIANT_Skip(enum_var, len+1);
    ok_(__FILE__,line)(hres == S_FALSE, "Skip failed: %08x\n", hres);

    IEnumVARIANT_Release(enum_var);

    V_VT(&name) = VT_I4;
    V_I4(&name) = len;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == S_OK, "item failed: %08x\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    V_VT(&name) = VT_UI4;
    V_I4(&name) = len;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == S_OK, "item failed: %08x\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    V_VT(&name) = VT_INT;
    V_I4(&name) = len;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == S_OK, "item failed: %08x\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    V_VT(&name) = VT_UINT;
    V_I4(&name) = len;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == S_OK, "item failed: %08x\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    V_VT(&name) = VT_I4;
    V_I4(&name) = -1;
    disp = (void*)0xdeadbeef;
    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok_(__FILE__,line) (hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
    ok_(__FILE__,line) (disp == NULL, "disp != NULL\n");

    IHTMLElementCollection_Release(col);
}

#define test_elem_all(c,t,l) _test_elem_all(__LINE__,c,t,l)
static void _test_elem_all(unsigned line, IUnknown *unk, const elem_type_t *elem_types, LONG exlen)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    IDispatch *disp;
    HRESULT hres;

    hres = IHTMLElement_get_all(elem, &disp);
    IHTMLElement_Release(elem);
    ok_(__FILE__,line)(hres == S_OK, "get_all failed: %08x\n", hres);

    _test_elem_collection(line, (IUnknown*)disp, elem_types, exlen);
    IDispatch_Release(disp);
}

#define test_doc_all(a,b,c) _test_doc_all(__LINE__,a,b,c)
static void _test_doc_all(unsigned line, IHTMLDocument2 *doc, const elem_type_t *elem_types, LONG exlen)
{
    IHTMLElementCollection *col;
    HRESULT hres;

    hres = IHTMLDocument2_get_all(doc, &col);
    ok_(__FILE__,line)(hres == S_OK, "get_all failed: %08x\n", hres);

    _test_elem_collection(line, (IUnknown*)col, elem_types, exlen);
    IHTMLElementCollection_Release(col);
}

#define test_elem_getelembytag(a,b,c,d) _test_elem_getelembytag(__LINE__,a,b,c,d)
static void _test_elem_getelembytag(unsigned line, IUnknown *unk, elem_type_t type, LONG exlen, IHTMLElement **ret)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    IHTMLElementCollection *col = NULL;
    elem_type_t *types = NULL;
    BSTR tmp;
    int i;
    HRESULT hres;

    tmp = a2bstr(elem_type_infos[type].tag);
    hres = IHTMLElement2_getElementsByTagName(elem, tmp, &col);
    SysFreeString(tmp);
    IHTMLElement2_Release(elem);
    ok_(__FILE__,line) (hres == S_OK, "getElementByTagName failed: %08x\n", hres);
    ok_(__FILE__,line) (col != NULL, "col == NULL\n");

    if(exlen) {
        types = HeapAlloc(GetProcessHeap(), 0, exlen*sizeof(elem_type_t));
        for(i=0; i<exlen; i++)
            types[i] = type;
    }

    _test_elem_collection(line, (IUnknown*)col, types, exlen);

    HeapFree(GetProcessHeap(), 0, types);

    if(ret)
        *ret = get_elem_col_item_idx(col, 0);
    IHTMLElementCollection_Release(col);
}

#define test_elem_innertext(e,t) _test_elem_innertext(__LINE__,e,t)
static void _test_elem_innertext(unsigned line, IHTMLElement *elem, const char *extext)
{
    BSTR text = NULL;
    HRESULT hres;

    hres = IHTMLElement_get_innerText(elem, &text);
    ok_(__FILE__,line) (hres == S_OK, "get_innerText failed: %08x\n", hres);
    if(extext)
        ok_(__FILE__,line) (!strcmp_wa(text, extext), "get_innerText returned %s expected %s\n",
                            wine_dbgstr_w(text), extext);
    else
        ok_(__FILE__,line) (!text, "get_innerText returned %s expected NULL\n", wine_dbgstr_w(text));
    SysFreeString(text);
}

#define test_elem_set_innertext(e,t) _test_elem_set_innertext(__LINE__,e,t)
static void _test_elem_set_innertext(unsigned line, IHTMLElement *elem, const char *text)
{
    IHTMLDOMChildrenCollection *col;
    BSTR str;
    HRESULT hres;

    str = a2bstr(text);
    hres = IHTMLElement_put_innerText(elem, str);
    ok_(__FILE__,line) (hres == S_OK, "put_innerText failed: %08x\n", hres);
    SysFreeString(str);

    _test_elem_innertext(line, elem, text);


    col = _get_child_nodes(line, (IUnknown*)elem);
    ok(col != NULL, "col == NULL\n");
    if(col) {
        LONG length = 0, type;
        IHTMLDOMNode *node;

        hres = IHTMLDOMChildrenCollection_get_length(col, &length);
        ok(hres == S_OK, "get_length failed: %08x\n", hres);
        ok(length == 1, "length = %d\n", length);

        node = _get_child_item(line, col, 0);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            type = _get_node_type(line, (IUnknown*)node);
            ok(type == 3, "type=%d\n", type);
            IHTMLDOMNode_Release(node);
        }

        IHTMLDOMChildrenCollection_Release(col);
    }

}

#define test_elem_innerhtml(e,t) _test_elem_innerhtml(__LINE__,e,t)
static void _test_elem_innerhtml(unsigned line, IUnknown *unk, const char *inner_html)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR html;
    HRESULT hres;

    hres = IHTMLElement_get_innerHTML(elem, &html);
    ok_(__FILE__,line)(hres == S_OK, "get_innerHTML failed: %08x\n", hres);
    if(inner_html)
        ok_(__FILE__,line)(!strcmp_wa(html, inner_html), "unexpected innerHTML: %s\n", wine_dbgstr_w(html));
    else
        ok_(__FILE__,line)(!html, "innerHTML = %s\n", wine_dbgstr_w(html));

    IHTMLElement_Release(elem);
    SysFreeString(html);
}

#define test_elem_set_innerhtml(e,t) _test_elem_set_innerhtml(__LINE__,e,t)
static void _test_elem_set_innerhtml(unsigned line, IUnknown *unk, const char *inner_html)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR html;
    HRESULT hres;

    html = a2bstr(inner_html);
    hres = IHTMLElement_put_innerHTML(elem, html);
    ok_(__FILE__,line)(hres == S_OK, "put_innerHTML failed: %08x\n", hres);

    IHTMLElement_Release(elem);
    SysFreeString(html);
}

#define test_elem_set_outerhtml(e,t) _test_elem_set_outerhtml(__LINE__,e,t)
static void _test_elem_set_outerhtml(unsigned line, IUnknown *unk, const char *outer_html)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR html;
    HRESULT hres;

    html = a2bstr(outer_html);
    hres = IHTMLElement_put_outerHTML(elem, html);
    ok_(__FILE__,line)(hres == S_OK, "put_outerHTML failed: %08x\n", hres);

    IHTMLElement_Release(elem);
    SysFreeString(html);
}

#define test_elem_outerhtml(e,t) _test_elem_outerhtml(__LINE__,e,t)
static void _test_elem_outerhtml(unsigned line, IUnknown *unk, const char *outer_html)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    BSTR html;
    HRESULT hres;

    hres = IHTMLElement_get_outerHTML(elem, &html);
    ok_(__FILE__,line)(hres == S_OK, "get_outerHTML failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(html, outer_html), "outerHTML = '%s', expected '%s'\n", wine_dbgstr_w(html), outer_html);

    IHTMLElement_Release(elem);
    SysFreeString(html);
}

#define test_elem_contains(a,b,c) _test_elem_contains(__LINE__,a,b,c)
static void _test_elem_contains(unsigned line, IHTMLElement *elem, IHTMLElement *elem2, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IHTMLElement_contains(elem, elem2, &b);
    ok_(__FILE__,line)(hres == S_OK, "contains failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exval, "contains returned %x, expected %x\n", b, exval);
}

#define test_elem_istextedit(a,b) _test_elem_istextedit(__LINE__,a,b)
static void _test_elem_istextedit(unsigned line, IHTMLElement *elem, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    b = 100;
    hres = IHTMLElement_get_isTextEdit(elem, &b);
    ok_(__FILE__,line)(hres == S_OK, "isTextEdit failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exval, "isTextEdit = %x\n", b);
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

#define node_get_next(u) _node_get_next(__LINE__,u)
static IHTMLDOMNode *_node_get_next(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *next;
    HRESULT hres;

    hres = IHTMLDOMNode_get_nextSibling(node, &next);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line) (hres == S_OK, "get_nextSiblibg failed: %08x\n", hres);

    return next;
}

#define node_get_prev(u) _node_get_prev(__LINE__,u)
static IHTMLDOMNode *_node_get_prev(unsigned line, IUnknown *unk)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *prev;
    HRESULT hres;

    hres = IHTMLDOMNode_get_previousSibling(node, &prev);
    IHTMLDOMNode_Release(node);
    ok_(__FILE__,line) (hres == S_OK, "get_previousSibling failed: %08x\n", hres);

    return prev;
}

#define test_elem_get_parent(u) _test_elem_get_parent(__LINE__,u)
static IHTMLElement *_test_elem_get_parent(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    IHTMLElement *parent;
    HRESULT hres;

    hres = IHTMLElement_get_parentElement(elem, &parent);
    IHTMLElement_Release(elem);
    ok_(__FILE__,line) (hres == S_OK, "get_parentElement failed: %08x\n", hres);

    return parent;
}

#define test_elem3_get_disabled(i,b) _test_elem3_get_disabled(__LINE__,i,b)
static void _test_elem3_get_disabled(unsigned line, IUnknown *unk, VARIANT_BOOL exb)
{
    IHTMLElement3 *elem3 = _get_elem3_iface(line, unk);
    VARIANT_BOOL disabled = 100;
    HRESULT hres;

    if (!elem3) return;
    hres = IHTMLElement3_get_disabled(elem3, &disabled);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);
    ok_(__FILE__,line) (disabled == exb, "disabled=%x, expected %x\n", disabled, exb);
    IHTMLElement3_Release(elem3);
}

#define test_elem3_set_disabled(i,b) _test_elem3_set_disabled(__LINE__,i,b)
static void _test_elem3_set_disabled(unsigned line, IUnknown *unk, VARIANT_BOOL b)
{
    IHTMLElement3 *elem3 = _get_elem3_iface(line, unk);
    HRESULT hres;

    if (!elem3) return;
    hres = IHTMLElement3_put_disabled(elem3, b);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);

    IHTMLElement3_Release(elem3);
    _test_elem3_get_disabled(line, unk, b);
}

#define test_select_get_disabled(i,b) _test_select_get_disabled(__LINE__,i,b)
static void _test_select_get_disabled(unsigned line, IHTMLSelectElement *select, VARIANT_BOOL exb)
{
    VARIANT_BOOL disabled = 100;
    HRESULT hres;

    hres = IHTMLSelectElement_get_disabled(select, &disabled);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);
    ok_(__FILE__,line) (disabled == exb, "disabled=%x, expected %x\n", disabled, exb);

    _test_elem3_get_disabled(line, (IUnknown*)select, exb);
}

static void test_select_remove(IHTMLSelectElement *select)
{
    HRESULT hres;

    hres = IHTMLSelectElement_remove(select, 3);
    ok(hres == S_OK, "remove failed: %08x, expected S_OK\n", hres);
    test_select_length(select, 2);

    hres = IHTMLSelectElement_remove(select, -1);
    ok(hres == E_INVALIDARG, "remove failed: %08x, expected E_INVALIDARG\n", hres);
    test_select_length(select, 2);

    hres = IHTMLSelectElement_remove(select, 0);
    ok(hres == S_OK, "remove failed:%08x\n", hres);
    test_select_length(select, 1);
}

#define test_text_length(u,l) _test_text_length(__LINE__,u,l)
static void _test_text_length(unsigned line, IUnknown *unk, LONG l)
{
    IHTMLDOMTextNode *text = _get_text_iface(line, unk);
    LONG length;
    HRESULT hres;

    hres = IHTMLDOMTextNode_get_length(text, &length);
    ok_(__FILE__,line)(hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line)(length == l, "length = %d, expected %d\n", length, l);
    IHTMLDOMTextNode_Release(text);
}

#define test_text_data(a,b) _test_text_data(__LINE__,a,b)
static void _test_text_data(unsigned line, IUnknown *unk, const char *exdata)
{
    IHTMLDOMTextNode *text = _get_text_iface(line, unk);
    BSTR str;
    HRESULT hres;

    hres = IHTMLDOMTextNode_get_data(text, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_data failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(str, exdata), "data = %s, expected %s\n", wine_dbgstr_w(str), exdata);
    IHTMLDOMTextNode_Release(text);
    SysFreeString(str);
}

#define set_text_data(a,b) _set_text_data(__LINE__,a,b)
static void _set_text_data(unsigned line, IUnknown *unk, const char *data)
{
    IHTMLDOMTextNode *text = _get_text_iface(line, unk);
    BSTR str = a2bstr(data);
    HRESULT hres;

    hres = IHTMLDOMTextNode_put_data(text, str);
    ok_(__FILE__,line)(hres == S_OK, "get_data failed: %08x\n", hres);
    IHTMLDOMTextNode_Release(text);
    SysFreeString(str);
}

#define text_append_data(a,b) _text_append_data(__LINE__,a,b)
static void _text_append_data(unsigned line, IUnknown *unk, const char *data)
{
    IHTMLDOMTextNode2 *text = _get_text2_iface(line, unk);
    BSTR str = a2bstr(data);
    HRESULT hres;

    hres = IHTMLDOMTextNode2_appendData(text, str);
    ok_(__FILE__,line)(hres == S_OK, "appendData failed: %08x\n", hres);
    IHTMLDOMTextNode2_Release(text);
    SysFreeString(str);
}

#define test_select_set_disabled(i,b) _test_select_set_disabled(__LINE__,i,b)
static void _test_select_set_disabled(unsigned line, IHTMLSelectElement *select, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLSelectElement_put_disabled(select, b);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);

    _test_select_get_disabled(line, select, b);
}

#define test_elem_dir(u,n) _test_elem_dir(__LINE__,u,n)
static void _test_elem_dir(unsigned line, IUnknown *unk, const char *exdir)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    BSTR dir;
    HRESULT hres;

    hres = IHTMLElement2_get_dir(elem, &dir);
    IHTMLElement2_Release(elem);
    ok_(__FILE__, line) (hres == S_OK, "get_dir failed: %08x\n", hres);
    if(exdir)
        ok_(__FILE__, line) (!strcmp_wa(dir, exdir), "got dir: %s, expected %s\n", wine_dbgstr_w(dir), exdir);
    else
        ok_(__FILE__, line) (!dir, "got dir: %s, expected NULL\n", wine_dbgstr_w(dir));

    SysFreeString(dir);
}

#define set_elem_dir(u,n) _set_elem_dir(__LINE__,u,n)
static void _set_elem_dir(unsigned line, IUnknown *unk, const char *dira)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    BSTR dir = a2bstr(dira);
    HRESULT hres;

    hres = IHTMLElement2_put_dir(elem, dir);
    IHTMLElement2_Release(elem);
    ok_(__FILE__, line) (hres == S_OK, "put_dir failed: %08x\n", hres);
    SysFreeString(dir);

    _test_elem_dir(line, unk, dira);
}

#define elem_get_scroll_height(u) _elem_get_scroll_height(__LINE__,u)
static LONG _elem_get_scroll_height(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    IHTMLTextContainer *txtcont;
    LONG l = -1, l2 = -1;
    HRESULT hres;

    hres = IHTMLElement2_get_scrollHeight(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_scrollHeight failed: %08x\n", hres);
    IHTMLElement2_Release(elem);

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLTextContainer, (void**)&txtcont);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLTextContainer: %08x\n", hres);

    hres = IHTMLTextContainer_get_scrollHeight(txtcont, &l2);
    IHTMLTextContainer_Release(txtcont);
    ok_(__FILE__,line) (hres == S_OK, "IHTMLTextContainer::get_scrollHeight failed: %d\n", l2);
    ok_(__FILE__,line) (l == l2, "unexpected height %d, expected %d\n", l2, l);

    return l;
}

#define elem_get_scroll_width(u) _elem_get_scroll_width(__LINE__,u)
static LONG _elem_get_scroll_width(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    IHTMLTextContainer *txtcont;
    LONG l = -1, l2 = -1;
    HRESULT hres;

    hres = IHTMLElement2_get_scrollWidth(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_scrollWidth failed: %08x\n", hres);
    IHTMLElement2_Release(elem);

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLTextContainer, (void**)&txtcont);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLTextContainer: %08x\n", hres);

    hres = IHTMLTextContainer_get_scrollWidth(txtcont, &l2);
    IHTMLTextContainer_Release(txtcont);
    ok_(__FILE__,line) (hres == S_OK, "IHTMLTextContainer::get_scrollWidth failed: %d\n", l2);
    ok_(__FILE__,line) (l == l2, "unexpected width %d, expected %d\n", l2, l);

    return l;
}

#define elem_get_scroll_top(u) _elem_get_scroll_top(__LINE__,u)
static LONG _elem_get_scroll_top(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    IHTMLTextContainer *txtcont;
    LONG l = -1, l2 = -1;
    HRESULT hres;

    hres = IHTMLElement2_get_scrollTop(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_scrollTop failed: %08x\n", hres);
    IHTMLElement2_Release(elem);

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLTextContainer, (void**)&txtcont);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLTextContainer: %08x\n", hres);

    hres = IHTMLTextContainer_get_scrollTop(txtcont, &l2);
    IHTMLTextContainer_Release(txtcont);
    ok_(__FILE__,line) (hres == S_OK, "IHTMLTextContainer::get_scrollTop failed: %d\n", l2);
    ok_(__FILE__,line) (l == l2, "unexpected top %d, expected %d\n", l2, l);

    return l;
}

#define elem_get_scroll_left(u) _elem_get_scroll_left(__LINE__,u)
static void _elem_get_scroll_left(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    IHTMLTextContainer *txtcont;
    LONG l = -1, l2 = -1;
    HRESULT hres;

    hres = IHTMLElement2_get_scrollLeft(elem, NULL);
    ok(hres == E_INVALIDARG, "expect E_INVALIDARG got 0x%08x\n", hres);

    hres = IHTMLElement2_get_scrollLeft(elem, &l);
    ok(hres == S_OK, "get_scrollTop failed: %08x\n", hres);
    IHTMLElement2_Release(elem);

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLTextContainer, (void**)&txtcont);
    ok(hres == S_OK, "Could not get IHTMLTextContainer: %08x\n", hres);

    hres = IHTMLTextContainer_get_scrollLeft(txtcont, &l2);
    IHTMLTextContainer_Release(txtcont);
    ok(hres == S_OK, "IHTMLTextContainer::get_scrollLeft failed: %d\n", l2);
    ok(l == l2, "unexpected left %d, expected %d\n", l2, l);
}

#define test_img_src(a,b,c) _test_img_src(__LINE__,a,b,c)
static void _test_img_src(unsigned line, IUnknown *unk, const char *exsrc, const char *broken_src)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    BSTR src;
    HRESULT hres;

    hres = IHTMLImgElement_get_src(img, &src);
    IHTMLImgElement_Release(img);
    ok_(__FILE__,line) (hres == S_OK, "get_src failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(src, exsrc) || (broken_src && broken(!strcmp_wa(src, broken_src))),
        "get_src returned %s expected %s\n", wine_dbgstr_w(src), exsrc);
    SysFreeString(src);
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
        ok_(__FILE__,line) (!strcmp_wa(alt, exalt), "unexpected alt %s\n", wine_dbgstr_w(alt));
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

#define test_img_align(u,a) _test_img_align(__LINE__,u,a)
static void _test_img_align(unsigned line, IUnknown *unk, const char *align)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(align);
    hres = IHTMLImgElement_put_align(img, tmp);
    ok_(__FILE__,line) (hres == S_OK, "put_align failed: %08x\n", hres);
    SysFreeString(tmp);

    hres = IHTMLImgElement_get_align(img, &tmp);
    ok_(__FILE__,line) (hres == S_OK, "put_align failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(tmp, align), "Expect %s, got %s\n", align, wine_dbgstr_w(tmp));
    SysFreeString(tmp);
}

#define test_img_name(u, c) _test_img_name(__LINE__,u, c)
static void _test_img_name(unsigned line, IUnknown *unk, const char *pValue)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    BSTR sName;
    HRESULT hres;

    hres = IHTMLImgElement_get_name(img, &sName);
    ok_(__FILE__,line) (hres == S_OK, "get_Name failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa (sName, pValue), "expected '%s' got '%s'\n", pValue, wine_dbgstr_w(sName));
    IHTMLImgElement_Release(img);
    SysFreeString(sName);
}

#define test_img_complete(a,b) _test_img_complete(__LINE__,a,b)
static void _test_img_complete(unsigned line, IHTMLElement *elem, VARIANT_BOOL exb)
{
    IHTMLImgElement *img = _get_img_iface(line, (IUnknown*)elem);
    VARIANT_BOOL b = 100;
    HRESULT hres;

    hres = IHTMLImgElement_get_complete(img, &b);
    ok_(__FILE__,line) (hres == S_OK, "get_complete failed: %08x\n", hres);
    ok_(__FILE__,line) (b == exb, "complete = %x, expected %x\n", b, exb);
    IHTMLImgElement_Release(img);
}

#define test_img_isMap(u, c) _test_img_isMap(__LINE__,u, c)
static void _test_img_isMap(unsigned line, IUnknown *unk, VARIANT_BOOL v)
{
    IHTMLImgElement *img = _get_img_iface(line, unk);
    VARIANT_BOOL b = 100;
    HRESULT hres;

    hres = IHTMLImgElement_put_isMap(img, v);
    ok_(__FILE__,line) (hres == S_OK, "put_isMap failed: %08x\n", hres);

    hres = IHTMLImgElement_get_isMap(img, &b);
    ok_(__FILE__,line) (hres == S_OK, "get_isMap failed: %08x\n", hres);
    ok_(__FILE__,line) (b == v, "isMap = %x, expected %x\n", b, v);

    hres = IHTMLImgElement_get_isMap(img, NULL);
    ok_(__FILE__,line) (hres == E_INVALIDARG, "ret = %08x, expected E_INVALIDARG\n", hres);
    IHTMLImgElement_Release(img);
}

static void test_dynamic_properties(IHTMLElement *elem)
{
    static const WCHAR attr1W[] = {'a','t','t','r','1',0};
    IDispatchEx *dispex;
    BSTR name, attr1 = SysAllocString(attr1W);
    VARIANT_BOOL succ;
    VARIANT val;
    int checked_no = 0;
    DISPID id = DISPID_STARTENUM;
    HRESULT hres;

    hres = IHTMLElement_QueryInterface(elem, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

    hres = IHTMLElement_removeAttribute(elem, attr1, 0, &succ);
    ok(hres == S_OK, "removeAttribute failed: %08x\n", hres);
    ok(succ, "removeAttribute set succ to FALSE\n");

    while(1) {
        hres = IDispatchEx_GetNextDispID(dispex, fdexEnumAll, id, &id);
        ok(hres==S_OK || hres==S_FALSE, "GetNextDispID failed: %08x\n", hres);
        if(hres != S_OK)
            break;

        hres = IDispatchEx_GetMemberName(dispex, id, &name);
        ok(hres == S_OK, "GetMemberName failed: %08x\n", hres);

        if(!strcmp_wa(name, "attr1"))
            ok(0, "attr1 should be removed\n");
        else if(!strcmp_wa(name, "attr2") || !strcmp_wa(name, "attr3"))
            checked_no++;
        SysFreeString(name);
    }
    ok(checked_no == 2, "checked_no=%d, expected 2\n", checked_no);
    IDispatchEx_Release(dispex);

    V_VT(&val) = VT_BSTR;
    V_BSTR(&val) = attr1;
    hres = IHTMLElement_setAttribute(elem, attr1, val, 0);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);
    SysFreeString(attr1);
}

#define test_attr_node_name(a,b) _test_attr_node_name(__LINE__,a,b)
static void _test_attr_node_name(unsigned line, IHTMLDOMAttribute *attr, const char *exname)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLDOMAttribute_get_nodeName(attr, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_nodeName failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(str, exname), "node name is %s, expected %s\n", wine_dbgstr_w(str), exname);
    SysFreeString(str);
}

static void test_attr_collection_disp(IDispatch *disp)
{
    IDispatchEx *dispex;
    IHTMLDOMAttribute *attr;
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    VARIANT var;
    EXCEPINFO ei;
    DISPID id;
    BSTR bstr;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

    bstr = a2bstr("0");
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08x\n", hres);
    SysFreeString(bstr);

    VariantInit(&var);
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(var) == NULL\n");
    VariantClear(&var);

    bstr = a2bstr("attr1");
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == S_OK, "GetDispID failed: %08x\n", hres);
    SysFreeString(bstr);

    VariantInit(&var);
    hres = IDispatchEx_InvokeEx(dispex, id, LOCALE_NEUTRAL, INVOKE_PROPERTYGET, &dp, &var, &ei, NULL);
    ok(hres == S_OK, "InvokeEx failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "V_VT(var)=%d\n", V_VT(&var));
    ok(V_DISPATCH(&var) != NULL, "V_DISPATCH(var) == NULL\n");
    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IHTMLDOMAttribute, (void**)&attr);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

    test_attr_node_name(attr, "attr1");

    IHTMLDOMAttribute_Release(attr);
    VariantClear(&var);

    IDispatchEx_Release(dispex);
}

static void test_attr_collection(IHTMLElement *elem)
{
    static const WCHAR testW[] = {'t','e','s','t',0};

    IHTMLDOMNode *node;
    IDispatch *disp, *attr;
    IHTMLDOMAttribute *dom_attr;
    IHTMLAttributeCollection *attr_col;
    BSTR name = SysAllocString(testW);
    VARIANT id, val;
    LONG i, len, checked;
    HRESULT hres;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLDOMNode, (void**)&node);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

    hres = IHTMLDOMNode_get_attributes(node, &disp);
    ok(hres == S_OK, "get_attributes failed: %08x\n", hres);

    hres = IHTMLDOMNode_get_attributes(node, &attr);
    ok(hres == S_OK, "get_attributes failed: %08x\n", hres);
    ok(iface_cmp((IUnknown*)disp, (IUnknown*)attr), "disp != attr\n");
    IDispatch_Release(attr);
    IHTMLDOMNode_Release(node);

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLAttributeCollection, (void**)&attr_col);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);

    hres = IHTMLAttributeCollection_get_length(attr_col, &i);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);

    V_VT(&val) = VT_I4;
    V_I4(&val) = 1;
    hres = IHTMLElement_setAttribute(elem, name, val, 0);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);
    SysFreeString(name);

    hres = IHTMLAttributeCollection_get_length(attr_col, &len);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(len == i+1, "get_length returned %d, expected %d\n", len, i+1);

    checked = 0;
    for(i=0; i<len; i++) {
        V_VT(&id) = VT_I4;
        V_I4(&id) = i;
        hres = IHTMLAttributeCollection_item(attr_col, &id, &attr);
        ok(hres == S_OK, "%d) item failed: %08x\n", i, hres);

        hres = IDispatch_QueryInterface(attr, &IID_IHTMLDOMAttribute, (void**)&dom_attr);
        ok(hres == S_OK, "%d) QueryInterface failed: %08x\n", i, hres);
        IDispatch_Release(attr);

        hres = IHTMLDOMAttribute_get_nodeName(dom_attr, &name);
        ok(hres == S_OK, "%d) get_nodeName failed: %08x\n", i, hres);

        if(!strcmp_wa(name, "id")) {
            checked++;
            hres = IHTMLDOMAttribute_get_nodeValue(dom_attr, &val);
            ok(hres == S_OK, "%d) get_nodeValue failed: %08x\n", i, hres);
            ok(V_VT(&val) == VT_BSTR, "id: V_VT(&val) = %d\n", V_VT(&val));
            ok(!strcmp_wa(V_BSTR(&val), "attr"), "id: V_BSTR(&val) = %s\n", wine_dbgstr_w(V_BSTR(&val)));
            test_attr_expando(dom_attr, VARIANT_FALSE);
            test_attr_value(dom_attr, "attr");
        } else if(!strcmp_wa(name, "attr1")) {
            checked++;
            hres = IHTMLDOMAttribute_get_nodeValue(dom_attr, &val);
            ok(hres == S_OK, "%d) get_nodeValue failed: %08x\n", i, hres);
            ok(V_VT(&val) == VT_BSTR, "attr1: V_VT(&val) = %d\n", V_VT(&val));
            ok(!strcmp_wa(V_BSTR(&val), "attr1"), "attr1: V_BSTR(&val) = %s\n", wine_dbgstr_w(V_BSTR(&val)));
            test_attr_expando(dom_attr, VARIANT_TRUE);
            test_attr_value(dom_attr, "attr1");
        } else if(!strcmp_wa(name, "attr2")) {
            checked++;
            hres = IHTMLDOMAttribute_get_nodeValue(dom_attr, &val);
            ok(hres == S_OK, "%d) get_nodeValue failed: %08x\n", i, hres);
            ok(V_VT(&val) == VT_BSTR, "attr2: V_VT(&val) = %d\n", V_VT(&val));
            ok(!V_BSTR(&val), "attr2: V_BSTR(&val) != NULL\n");
            test_attr_value(dom_attr, "");
        } else if(!strcmp_wa(name, "attr3")) {
            checked++;
            hres = IHTMLDOMAttribute_get_nodeValue(dom_attr, &val);
            ok(hres == S_OK, "%d) get_nodeValue failed: %08x\n", i, hres);
            ok(V_VT(&val) == VT_BSTR, "attr3: V_VT(&val) = %d\n", V_VT(&val));
            ok(!strcmp_wa(V_BSTR(&val), "attr3"), "attr3: V_BSTR(&val) = %s\n", wine_dbgstr_w(V_BSTR(&val)));
            test_attr_value(dom_attr, "attr3");
        } else if(!strcmp_wa(name, "test")) {
            checked++;
            hres = IHTMLDOMAttribute_get_nodeValue(dom_attr, &val);
            ok(hres == S_OK, "%d) get_nodeValue failed: %08x\n", i, hres);
            ok(V_VT(&val) == VT_I4, "test: V_VT(&val) = %d\n", V_VT(&val));
            ok(V_I4(&val) == 1, "test: V_I4(&val) = %d\n", V_I4(&val));
            test_attr_value(dom_attr, "1");
        }

        IHTMLDOMAttribute_Release(dom_attr);
        SysFreeString(name);
        VariantClear(&val);
    }
    ok(checked==5, "invalid number of specified attributes (%d)\n", checked);

    V_I4(&id) = len;
    hres = IHTMLAttributeCollection_item(attr_col, &id, &attr);
    ok(hres == E_INVALIDARG, "item failed: %08x\n", hres);

    V_VT(&id) = VT_BSTR;
    V_BSTR(&id) = a2bstr("nonexisting");
    hres = IHTMLAttributeCollection_item(attr_col, &id, &attr);
    ok(hres == E_INVALIDARG, "item failed: %08x\n", hres);
    VariantClear(&id);

    test_attr_collection_disp(disp);

    IDispatch_Release(disp);
    IHTMLAttributeCollection_Release(attr_col);
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
        ok_(__FILE__,line) (!strcmp_wa(id, exid), "unexpected id %s\n", wine_dbgstr_w(id));
    else
        ok_(__FILE__,line) (!id, "id=%s\n", wine_dbgstr_w(id));

    SysFreeString(id);
}

#define test_elem_language(e,i) _test_elem_language(__LINE__,e,i)
static void _test_elem_language(unsigned line, IHTMLElement *elem, const char *exlang)
{
    BSTR lang = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLElement_get_language(elem, &lang);
    ok_(__FILE__,line) (hres == S_OK, "get_language failed: %08x\n", hres);

    if(exlang)
        ok_(__FILE__,line) (!strcmp_wa(lang, exlang), "unexpected language %s\n", wine_dbgstr_w(lang));
    else
        ok_(__FILE__,line) (!lang, "language=%s\n", wine_dbgstr_w(lang));

    SysFreeString(lang);
}

#define set_elem_language(e,i) _set_elem_language(__LINE__,e,i)
static void _set_elem_language(unsigned line, IHTMLElement *elem, const char *lang)
{
    BSTR str = a2bstr(lang);
    HRESULT hres;

    hres = IHTMLElement_put_language(elem, str);
    ok_(__FILE__,line) (hres == S_OK, "get_language failed: %08x\n", hres);
    SysFreeString(str);

    _test_elem_language(line, elem, lang);
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

static void test_contenteditable(IUnknown *unk)
{
    IHTMLElement3 *elem3 = get_elem3_iface(unk);
    HRESULT hres;
    BSTR str, strDefault;

    hres = IHTMLElement3_get_contentEditable(elem3, &strDefault);
    ok(hres == S_OK, "get_contentEditable failed: 0x%08x\n", hres);

    str = a2bstr("true");
    hres = IHTMLElement3_put_contentEditable(elem3, str);
    ok(hres == S_OK, "put_contentEditable(%s) failed: 0x%08x\n", wine_dbgstr_w(str), hres);
    SysFreeString(str);
    hres = IHTMLElement3_get_contentEditable(elem3, &str);
    ok(hres == S_OK, "get_contentEditable failed: 0x%08x\n", hres);
    ok(!strcmp_wa(str, "true"), "Got %s, expected %s\n", wine_dbgstr_w(str), "true");

    /* Restore origin contentEditable */
    hres = IHTMLElement3_put_contentEditable(elem3, strDefault);
    ok(hres == S_OK, "put_contentEditable(%s) failed: 0x%08x\n", wine_dbgstr_w(strDefault), hres);
    SysFreeString(strDefault);

    IHTMLElement3_Release(elem3);
}

#define test_input_type(i,t) _test_input_type(__LINE__,i,t)
static void _test_input_type(unsigned line, IHTMLInputElement *input, const char *extype)
{
    BSTR type;
    HRESULT hres;

    hres = IHTMLInputElement_get_type(input, &type);
    ok_(__FILE__,line) (hres == S_OK, "get_type failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(type, extype), "type=%s, expected %s\n", wine_dbgstr_w(type), extype);
    SysFreeString(type);
}

#define test_input_name(u, c) _test_input_name(__LINE__,u, c)
static void _test_input_name(unsigned line, IHTMLInputElement *input, const char *exname)
{
    BSTR name = (BSTR)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLInputElement_get_name(input, &name);
    ok_(__FILE__,line) (hres == S_OK, "get_name failed: %08x\n", hres);
    if(exname)
        ok_(__FILE__,line) (!strcmp_wa (name, exname), "name=%s, expected %s\n", wine_dbgstr_w(name), exname);
    else
        ok_(__FILE__,line) (!name, "name=%p, expected NULL\n", name);
    SysFreeString(name);
}

#define test_input_set_name(u, c) _test_input_set_name(__LINE__,u, c)
static void _test_input_set_name(unsigned line, IHTMLInputElement *input, const char *name)
{
    BSTR tmp = a2bstr(name);
    HRESULT hres;

    hres = IHTMLInputElement_put_name(input, tmp);
    ok_(__FILE__,line) (hres == S_OK, "put_name failed: %08x\n", hres);
    SysFreeString(tmp);

    _test_input_name(line, input, name);
}

#define test_input_get_disabled(i,b) _test_input_get_disabled(__LINE__,i,b)
static void _test_input_get_disabled(unsigned line, IHTMLInputElement *input, VARIANT_BOOL exb)
{
    VARIANT_BOOL disabled = 100;
    HRESULT hres;

    hres = IHTMLInputElement_get_disabled(input, &disabled);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);
    ok_(__FILE__,line) (disabled == exb, "disabled=%x, expected %x\n", disabled, exb);

    _test_elem3_get_disabled(line, (IUnknown*)input, exb);
}

#define test_input_set_disabled(i,b) _test_input_set_disabled(__LINE__,i,b)
static void _test_input_set_disabled(unsigned line, IHTMLInputElement *input, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLInputElement_put_disabled(input, b);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);

    _test_input_get_disabled(line, input, b);
}

#define test_input_get_defaultchecked(i,b) _test_input_get_defaultchecked(__LINE__,i,b)
static void _test_input_get_defaultchecked(unsigned line, IHTMLInputElement *input, VARIANT_BOOL exb)
{
    VARIANT_BOOL checked = 100;
    HRESULT hres;

    hres = IHTMLInputElement_get_defaultChecked(input, &checked);
    ok_(__FILE__,line) (hres == S_OK, "get_defaultChecked failed: %08x\n", hres);
    ok_(__FILE__,line) (checked == exb, "checked=%x, expected %x\n", checked, exb);
}

#define test_input_set_defaultchecked(i,b) _test_input_set_defaultchecked(__LINE__,i,b)
static void _test_input_set_defaultchecked(unsigned line, IHTMLInputElement *input, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLInputElement_put_defaultChecked(input, b);
    ok_(__FILE__,line) (hres == S_OK, "get_defaultChecked failed: %08x\n", hres);

    _test_input_get_defaultchecked(line, input, b);
}

#define test_input_get_checked(i,b) _test_input_get_checked(__LINE__,i,b)
static void _test_input_get_checked(unsigned line, IHTMLInputElement *input, VARIANT_BOOL exb)
{
    VARIANT_BOOL checked = 100;
    HRESULT hres;

    hres = IHTMLInputElement_get_checked(input, &checked);
    ok_(__FILE__,line) (hres == S_OK, "get_checked failed: %08x\n", hres);
    ok_(__FILE__,line) (checked == exb, "checked=%x, expected %x\n", checked, exb);
}

#define test_input_set_checked(i,b) _test_input_set_checked(__LINE__,i,b)
static void _test_input_set_checked(unsigned line, IHTMLInputElement *input, VARIANT_BOOL b)
{
    HRESULT hres;

    hres = IHTMLInputElement_put_checked(input, b);
    ok_(__FILE__,line) (hres == S_OK, "put_checked failed: %08x\n", hres);

    _test_input_get_checked(line, input, b);
}

#define test_input_maxlength(i,b) _test_input_maxlength(__LINE__,i,b)
static void _test_input_maxlength(unsigned line, IHTMLInputElement *input, LONG exl)
{
    LONG maxlength = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLInputElement_get_maxLength(input, &maxlength);
    ok_(__FILE__,line) (hres == S_OK, "get_maxLength failed: %08x\n", hres);
    ok_(__FILE__,line) (maxlength == exl, "maxLength=%x, expected %d\n", maxlength, exl);
}

#define test_input_set_maxlength(i,b) _test_input_set_maxlength(__LINE__,i,b)
static void _test_input_set_maxlength(unsigned line, IHTMLInputElement *input, LONG l)
{
    HRESULT hres;

    hres = IHTMLInputElement_put_maxLength(input, l);
    ok_(__FILE__,line) (hres == S_OK, "put_maxLength failed: %08x\n", hres);

    _test_input_maxlength(line, input, l);
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
        ok_(__FILE__,line) (!strcmp_wa(bstr, exval), "value=%s\n", wine_dbgstr_w(bstr));
    else
        ok_(__FILE__,line) (!bstr, "exval != NULL\n");
    SysFreeString(bstr);
    IHTMLInputElement_Release(input);
}

#define test_input_get_form(o, t)  _test_input_get_form(__LINE__, o, t)
static void _test_input_get_form(unsigned line, IUnknown *unk, const char *id)
{
    IHTMLInputElement *input;
    IHTMLFormElement *form;
    IHTMLElement *elem;
    HRESULT hres;

    ok_(__FILE__,line) (unk != NULL, "unk is NULL!\n");
    hres = IUnknown_QueryInterface(unk, &IID_IHTMLInputElement, (void**)&input);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);
    ok_(__FILE__,line) (input != NULL, "input == NULL\n");
    if(FAILED(hres) || input == NULL)
        return;

    hres = IHTMLInputElement_get_form(input, &form);
    ok_(__FILE__, line) (hres == S_OK, "get_form failed: %08x\n", hres);
    ok_(__FILE__, line) (form != NULL, "form == NULL\n");
    if(FAILED(hres) || form == NULL){
        IHTMLInputElement_Release(input);
        return;
    }

    hres = IHTMLFormElement_QueryInterface(form, &IID_IHTMLElement, (void **)&elem);
    ok_(__FILE__, line) (hres == S_OK, "QueryInterface(IID_IHTMLElement) failed: %08x\n", hres);
    ok_(__FILE__, line) (elem != NULL, "elem == NULL\n");
    if(FAILED(hres) || elem == NULL){
        IHTMLInputElement_Release(input);
        IHTMLFormElement_Release(form);
        return;
    }

    _test_elem_id(line, (IUnknown*)elem, id);

    IHTMLInputElement_Release(input);
    IHTMLFormElement_Release(form);
    IHTMLElement_Release(elem);
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
    hres = IHTMLInputElement_put_value(input, bstr);
    ok_(__FILE__,line) (hres == S_OK, "get_value failed: %08x\n", hres);
    SysFreeString(bstr);
    IHTMLInputElement_Release(input);

    _test_input_value(line, unk, val);
}

#define test_input_defaultValue(o,t) _test_input_defaultValue(__LINE__,o,t)
static void _test_input_defaultValue(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLInputElement *input;
    BSTR str;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLInputElement, (void**)&input);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLInputElement_get_defaultValue(input, &str);
    ok_(__FILE__,line) (hres == S_OK, "get_defaultValue failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line) (!strcmp_wa(str, exval), "defaultValue=%s\n", wine_dbgstr_w(str));
    else
        ok_(__FILE__,line) (!str, "exval != NULL\n");
    SysFreeString(str);
    IHTMLInputElement_Release(input);
}

#define test_input_put_defaultValue(o,v) _test_input_put_defaultValue(__LINE__,o,v)
static void _test_input_put_defaultValue(unsigned line, IUnknown *unk, const char *val)
{
    IHTMLInputElement *input;
    BSTR str;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLInputElement, (void**)&input);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);
    if(FAILED(hres))
        return;

    str = a2bstr(val);
    hres = IHTMLInputElement_put_defaultValue(input, str);
    ok_(__FILE__,line) (hres == S_OK, "get_defaultValue failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLInputElement_Release(input);

    _test_input_defaultValue(line, unk, val);
}

#define test_input_src(i,s) _test_input_src(__LINE__,i,s)
static void _test_input_src(unsigned line, IHTMLInputElement *input, const char *exsrc)
{
    BSTR src;
    HRESULT hres;

    hres = IHTMLInputElement_get_src(input, &src);
    ok_(__FILE__,line) (hres == S_OK, "get_src failed: %08x\n", hres);
    if(exsrc)
        ok_(__FILE__,line) (!strcmp_wa(src, exsrc), "get_src returned %s expected %s\n", wine_dbgstr_w(src), exsrc);
    else
        ok_(__FILE__,line) (!src, "get_src returned %s expected NULL\n", wine_dbgstr_w(src));
    SysFreeString(src);
}

#define test_input_set_src(u,s) _test_input_set_src(__LINE__,u,s)
static void _test_input_set_src(unsigned line, IHTMLInputElement *input, const char *src)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(src);
    hres = IHTMLInputElement_put_src(input, tmp);
    SysFreeString(tmp);
    ok_(__FILE__,line) (hres == S_OK, "put_src failed: %08x\n", hres);

    _test_input_src(line, input, src);
}

#define test_input_set_size(u,s,r) _test_input_set_size(__LINE__,u,s,r)
static void _test_input_set_size(unsigned line, IHTMLInputElement *input, LONG size, HRESULT exret)
{
    HRESULT hres;

    hres = IHTMLInputElement_put_size(input, size);
    ok_(__FILE__,line) (hres == exret, "Expect ret = %08x, got: %08x\n", exret, hres);
}

#define test_input_get_size(u,s) _test_input_get_size(__LINE__,u,s)
static void _test_input_get_size(unsigned line, IHTMLInputElement *input, LONG exsize)
{
    HRESULT hres;
    LONG size;

    hres = IHTMLInputElement_get_size(input, &size);
    ok_(__FILE__,line) (hres == S_OK, "get_size failed: %08x\n", hres);
    ok_(__FILE__,line) (size == exsize, "Expect %d, got %d\n", exsize, size);

    hres = IHTMLInputElement_get_size(input, NULL);
    ok_(__FILE__,line) (hres == E_INVALIDARG, "Expect ret E_INVALIDARG, got: %08x\n", hres);
}

#define test_input_readOnly(u,b) _test_input_readOnly(__LINE__,u,b)
static void _test_input_readOnly(unsigned line, IHTMLInputElement *input, VARIANT_BOOL v)
{
    HRESULT hres;
    VARIANT_BOOL b = 100;

    hres = IHTMLInputElement_put_readOnly(input, v);
    ok_(__FILE__,line)(hres == S_OK, "put readOnly failed: %08x\n", hres);

    hres = IHTMLInputElement_get_readOnly(input, &b);
    ok_(__FILE__,line)(hres == S_OK, "get readOnly failed: %08x\n", hres);
    ok_(__FILE__,line)(v == b, "Expect %x, got %x\n", v, b);
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
        ok_(__FILE__,line) (!strcmp_wa(class, exclass), "unexpected className %s\n", wine_dbgstr_w(class));
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

#define test_style_media(s,m) _test_style_media(__LINE__,s,m)
static void _test_style_media(unsigned line, IUnknown *unk, const char *exmedia)
{
    IHTMLStyleElement *style = _get_style_iface(line, unk);
    BSTR media;
    HRESULT hres;

    hres = IHTMLStyleElement_get_media(style, &media);
    ok_(__FILE__,line)(hres == S_OK, "get_media failed: %08x\n", hres);
    if(exmedia)
        ok_(__FILE__,line)(!strcmp_wa(media, exmedia), "media = %s, expected %s\n", wine_dbgstr_w(media), exmedia);
    else
        ok_(__FILE__,line)(!media, "media = %s, expected NULL\n", wine_dbgstr_w(media));

    IHTMLStyleElement_Release(style);
    SysFreeString(media);
}

#define test_style_put_media(s,m) _test_style_put_media(__LINE__,s,m)
static void _test_style_put_media(unsigned line, IUnknown *unk, const char *media)
{
    IHTMLStyleElement *style = _get_style_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = a2bstr(media);
    hres = IHTMLStyleElement_put_media(style, str);
    ok_(__FILE__,line)(hres == S_OK, "put_media failed: %08x\n", hres);
    IHTMLStyleElement_Release(style);
    SysFreeString(str);

    _test_style_media(line, unk, media);
}

#define test_style_type(s,m) _test_style_type(__LINE__,s,m)
static void _test_style_type(unsigned line, IUnknown *unk, const char *extype)
{
    IHTMLStyleElement *style = _get_style_iface(line, unk);
    BSTR type;
    HRESULT hres;

    hres = IHTMLStyleElement_get_type(style, &type);
    ok_(__FILE__,line)(hres == S_OK, "get_type failed: %08x\n", hres);
    if(extype)
        ok_(__FILE__,line)(!strcmp_wa(type, extype), "type = %s, expected %s\n", wine_dbgstr_w(type), extype);
    else
        ok_(__FILE__,line)(!type, "type = %s, expected NULL\n", wine_dbgstr_w(type));

    IHTMLStyleElement_Release(style);
    SysFreeString(type);
}

#define test_style_put_type(s,m) _test_style_put_type(__LINE__,s,m)
static void _test_style_put_type(unsigned line, IUnknown *unk, const char *type)
{
    IHTMLStyleElement *style = _get_style_iface(line, unk);
    BSTR str;
    HRESULT hres;

    str = a2bstr(type);
    hres = IHTMLStyleElement_put_type(style, str);
    ok_(__FILE__,line)(hres == S_OK, "put_type failed: %08x\n", hres);
    IHTMLStyleElement_Release(style);
    SysFreeString(str);

    _test_style_type(line, unk, type);
}

#define test_elem_filters(u) _test_elem_filters(__LINE__,u)
static void _test_elem_filters(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    HRESULT hres;
    IHTMLFiltersCollection *filters;

    hres = IHTMLElement_get_filters(elem, &filters);
    ok_(__FILE__,line) (hres == S_OK || broken(hres == REGDB_E_CLASSNOTREG) /* NT4 */,
                        "get_filters failed: %08x\n", hres);
    if(hres == S_OK)
    {
        LONG len;
        IDispatchEx *dispex;

        hres = IHTMLFiltersCollection_get_length(filters, &len);
        ok_(__FILE__,line) (hres == S_OK, "get_length failed: %08x\n", hres);
        ok_(__FILE__,line) (len == 0, "expect 0 got %d\n", len);

        hres = IHTMLFiltersCollection_QueryInterface(filters, &IID_IDispatchEx, (void**)&dispex);
        ok_(__FILE__,line) (hres == S_OK || broken(hres == E_NOINTERFACE),
                            "Could not get IDispatchEx interface: %08x\n", hres);
        if(SUCCEEDED(hres)) {
            test_disp((IUnknown*)filters, &IID_IHTMLFiltersCollection, "[object]");
            IDispatchEx_Release(dispex);
        }

        IHTMLFiltersCollection_Release(filters);
    }

    IHTMLElement_Release(elem);
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
        ok_(__FILE__,line) (!strcmp_wa(title, extitle), "unexpected title %s\n", wine_dbgstr_w(title));
    else
        ok_(__FILE__,line) (!title, "title=%s, expected NULL\n", wine_dbgstr_w(title));

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
        ok_(__FILE__,line) (!strcmp_wa(V_BSTR(&var), exval), "unexpected value %s\n", wine_dbgstr_w(V_BSTR(&var)));
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
    LONG l;
    HRESULT hres;

    hres = IHTMLElement2_get_clientWidth(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_clientWidth failed: %08x\n", hres);
    hres = IHTMLElement2_get_clientHeight(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_clientHeight failed: %08x\n", hres);

    IHTMLElement2_Release(elem);
}

#define test_elem_client_rect(u) _test_elem_client_rect(__LINE__,u)
static void _test_elem_client_rect(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    LONG l;
    HRESULT hres;

    hres = IHTMLElement2_get_clientLeft(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_clientLeft failed: %08x\n", hres);
    ok_(__FILE__,line) (!l, "clientLeft = %d\n", l);

    hres = IHTMLElement2_get_clientTop(elem, &l);
    ok_(__FILE__,line) (hres == S_OK, "get_clientTop failed: %08x\n", hres);
    ok_(__FILE__,line) (!l, "clientTop = %d\n", l);

    IHTMLElement2_Release(elem);
}

#define test_form_length(e,l) _test_form_length(__LINE__,e,l)
static void _test_form_length(unsigned line, IUnknown *unk, LONG exlen)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    LONG len = 0xdeadbeef;
    HRESULT hres;

    hres = IHTMLFormElement_get_length(form, &len);
    ok_(__FILE__,line)(hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line)(len == exlen, "length=%d, expected %d\n", len, exlen);

    IHTMLFormElement_Release(form);
}

#define test_form_action(f,a) _test_form_action(__LINE__,f,a)
static void _test_form_action(unsigned line, IUnknown *unk, const char *ex)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR action = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLFormElement_get_action(form, &action);
    ok_(__FILE__,line)(hres == S_OK, "get_action failed: %08x\n", hres);
    if(ex)
        ok_(__FILE__,line)(!strcmp_wa(action, ex), "action=%s, expected %s\n", wine_dbgstr_w(action), ex);
    else
        ok_(__FILE__,line)(!action, "action=%p\n", action);

    SysFreeString(action);
    IHTMLFormElement_Release(form);
}

#define test_form_put_action(f,a) _test_form_put_action(__LINE__,f,a)
static void _test_form_put_action(unsigned line, IUnknown *unk, const char *action)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR tmp = a2bstr(action);
    HRESULT hres;

    hres = IHTMLFormElement_put_action(form, tmp);
    ok_(__FILE__,line)(hres == S_OK, "put_action failed: %08x\n", hres);
    SysFreeString(tmp);
    IHTMLFormElement_Release(form);

    _test_form_action(line, unk, action);
}

#define test_form_method(f,a) _test_form_method(__LINE__,f,a)
static void _test_form_method(unsigned line, IUnknown *unk, const char *ex)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR method = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLFormElement_get_method(form, &method);
    ok_(__FILE__,line)(hres == S_OK, "get_method failed: %08x\n", hres);
    if(ex)
        ok_(__FILE__,line)(!strcmp_wa(method, ex), "method=%s, expected %s\n", wine_dbgstr_w(method), ex);
    else
        ok_(__FILE__,line)(!method, "method=%p\n", method);

    SysFreeString(method);
    IHTMLFormElement_Release(form);
}

#define test_form_put_method(f,r,a) _test_form_put_method(__LINE__,f,r,a)
static void _test_form_put_method(unsigned line, IUnknown *unk, HRESULT exp_hres, const char *method)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR tmp = a2bstr(method);
    HRESULT hres;

    hres = IHTMLFormElement_put_method(form, tmp);
    ok_(__FILE__,line)(hres == exp_hres, "put_method returned: %08x, expected %08x\n", hres, exp_hres);
    SysFreeString(tmp);
    IHTMLFormElement_Release(form);

    if(exp_hres == S_OK)
        _test_form_method(line, unk, method);
}

#define test_form_name(f,a) _test_form_name(__LINE__,f,a)
static void _test_form_name(unsigned line, IUnknown *unk, const char *ex)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR name = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLFormElement_get_name(form, &name);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    if(ex)
        ok_(__FILE__,line)(!strcmp_wa(name, ex), "name=%s, expected %s\n", wine_dbgstr_w(name), ex);
    else
        ok_(__FILE__,line)(!name, "name=%p\n", name);

    SysFreeString(name);
    IHTMLFormElement_Release(form);
}

#define test_form_put_name(f,a) _test_form_put_name(__LINE__,f,a)
static void _test_form_put_name(unsigned line, IUnknown *unk, const char *name)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR tmp = a2bstr(name);
    HRESULT hres;

    hres = IHTMLFormElement_put_name(form, tmp);
    ok_(__FILE__,line)(hres == S_OK, "put_name failed: %08x\n", hres);
    SysFreeString(tmp);
    IHTMLFormElement_Release(form);

    _test_form_name(line, unk, name);
}

#define test_form_encoding(f,a) _test_form_encoding(__LINE__,f,a)
static void _test_form_encoding(unsigned line, IUnknown *unk, const char *ex)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR encoding = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLFormElement_get_encoding(form, &encoding);
    ok_(__FILE__,line)(hres == S_OK, "get_encoding failed: %08x\n", hres);
    if(ex)
        ok_(__FILE__,line)(!strcmp_wa(encoding, ex), "encoding=%s, expected %s\n", wine_dbgstr_w(encoding), ex);
    else
        ok_(__FILE__,line)(!encoding, "encoding=%p\n", encoding);

    SysFreeString(encoding);
    IHTMLFormElement_Release(form);
}

#define test_form_put_encoding(f,r,a) _test_form_put_encoding(__LINE__,f,r,a)
static void _test_form_put_encoding(unsigned line, IUnknown *unk, HRESULT exp_hres, const char *encoding)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    BSTR tmp = a2bstr(encoding);
    HRESULT hres;

    hres = IHTMLFormElement_put_encoding(form, tmp);
    ok_(__FILE__,line)(hres == exp_hres, "put_encoding returned: %08x, expected %08x\n", hres, exp_hres);
    SysFreeString(tmp);
    IHTMLFormElement_Release(form);

    if(exp_hres == S_OK)
        _test_form_encoding(line, unk, encoding);
}

#define test_form_elements(a) _test_form_elements(__LINE__,a)
static void _test_form_elements(unsigned line, IUnknown *unk)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    IDispatch *disp;
    HRESULT hres;

    disp = NULL;
    hres = IHTMLFormElement_get_elements(form, &disp);
    ok_(__FILE__,line)(hres == S_OK, "get_elements failed: %08x\n", hres);
    ok_(__FILE__,line)(disp != NULL, "disp = NULL\n");
    ok_(__FILE__,line)(iface_cmp((IUnknown*)form, (IUnknown*)disp), "disp != form\n");

    IDispatch_Release(disp);
    IHTMLFormElement_Release(form);
}

#define test_form_reset(a) _test_form_reset(__LINE__,a)
static void _test_form_reset(unsigned line, IUnknown *unk)
{
    IHTMLFormElement *form = _get_form_iface(line, unk);
    HRESULT hres;

    hres = IHTMLFormElement_reset(form);
    ok_(__FILE__,line)(hres == S_OK, "reset failed: %08x\n", hres);

    IHTMLFormElement_Release(form);
}

static void test_form_target(IUnknown *unk)
{
    IHTMLFormElement *form = get_form_iface(unk);
    HRESULT hres;
    BSTR str;
    static const char target[] = "_blank";

    str = a2bstr(target);
    hres = IHTMLFormElement_put_target(form, str);
    ok(hres == S_OK, "put_target(%s) failed: %08x\n", target, hres);
    SysFreeString(str);

    hres = IHTMLFormElement_get_target(form, &str);
    ok(hres == S_OK, "get_target failed: %08x\n", hres);
    ok(!strcmp_wa(str, target), "Expected %s, got %s\n", target, wine_dbgstr_w(str));
    SysFreeString(str);

    IHTMLFormElement_Release(form);
}

static void test_select_form(IUnknown *uselect, IUnknown  *uform)
{
    IHTMLSelectElement *select = get_select_iface(uselect);
    IHTMLFormElement *form;
    HRESULT hres;

    hres = IHTMLSelectElement_get_form(select, NULL);
    ok(hres == E_POINTER, "got %08x\n, expected E_POINTER\n", hres);

    hres = IHTMLSelectElement_get_form(select, &form);
    ok(hres == S_OK, "get_form failed: %08x\n", hres);
    ok(form != NULL, "form == NULL\n");

    test_form_length((IUnknown*)form, 1);
    test_form_elements((IUnknown*)form);
    test_form_name((IUnknown*)form, "form_name");

    ok(iface_cmp(uform, (IUnknown*)form), "Expected %p, got %p\n", uform, form);

    IHTMLSelectElement_Release(select);
    IHTMLFormElement_Release(form);
}

static void test_select_form_notfound(IHTMLSelectElement *select)
{
    IHTMLFormElement *form;
    HRESULT hres;

    form = (IHTMLFormElement*)0xdeadbeef;
    hres = IHTMLSelectElement_get_form(select, &form);
    ok(hres == S_OK, "get_form failed: %08x\n", hres);
    ok(form == NULL, "got %p\n", form);
}

#define test_meta_name(a,b) _test_meta_name(__LINE__,a,b)
static void _test_meta_name(unsigned line, IUnknown *unk, const char *exname)
{
    IHTMLMetaElement *meta;
    BSTR name = NULL;
    HRESULT hres;

    meta = _get_metaelem_iface(line, unk);
    hres = IHTMLMetaElement_get_name(meta, &name);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(name, exname), "name = %s, expected %s\n", wine_dbgstr_w(name), exname);
    SysFreeString(name);
    IHTMLMetaElement_Release(meta);
}

#define test_meta_content(a,b) _test_meta_content(__LINE__,a,b)
static void _test_meta_content(unsigned line, IUnknown *unk, const char *excontent)
{
    IHTMLMetaElement *meta;
    BSTR content = NULL;
    HRESULT hres;

    meta = _get_metaelem_iface(line, unk);
    hres = IHTMLMetaElement_get_content(meta, &content);
    ok_(__FILE__,line)(hres == S_OK, "get_content failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(content, excontent), "content = %s, expected %s\n", wine_dbgstr_w(content), excontent);
    SysFreeString(content);
    IHTMLMetaElement_Release(meta);
}

#define test_meta_httpequiv(a,b) _test_meta_httpequiv(__LINE__,a,b)
static void _test_meta_httpequiv(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLMetaElement *meta;
    BSTR val = NULL;
    HRESULT hres;

    meta = _get_metaelem_iface(line, unk);
    hres = IHTMLMetaElement_get_httpEquiv(meta, &val);
    ok_(__FILE__,line)(hres == S_OK, "get_httpEquiv failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(val, exval), "httpEquiv = %s, expected %s\n", wine_dbgstr_w(val), exval);
    SysFreeString(val);
    IHTMLMetaElement_Release(meta);
}

#define test_meta_charset(a,b) _test_meta_charset(__LINE__,a,b)
static void _test_meta_charset(unsigned line, IUnknown *unk, const char *exval)
{
    IHTMLMetaElement *meta;
    BSTR val = NULL;
    HRESULT hres;

    meta = _get_metaelem_iface(line, unk);
    hres = IHTMLMetaElement_get_charset(meta, &val);
    ok_(__FILE__,line)(hres == S_OK, "get_charset failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(val, exval), "charset = %s, expected %s\n", wine_dbgstr_w(val), exval);
    else
        ok_(__FILE__,line)(!val, "charset = %s, expected NULL\n", wine_dbgstr_w(val));
    SysFreeString(val);
    IHTMLMetaElement_Release(meta);
}

#define set_meta_charset(a,b) _set_meta_charset(__LINE__,a,b)
static void _set_meta_charset(unsigned line, IUnknown *unk, const char *vala)
{
    BSTR val = a2bstr(vala);
    IHTMLMetaElement *meta;
    HRESULT hres;

    meta = _get_metaelem_iface(line, unk);
    hres = IHTMLMetaElement_put_charset(meta, val);
    ok_(__FILE__,line)(hres == S_OK, "put_charset failed: %08x\n", hres);
    SysFreeString(val);
    IHTMLMetaElement_Release(meta);

    _test_meta_charset(line, unk, vala);
}

#define test_link_media(a,b) _test_link_media(__LINE__,a,b)
static void _test_link_media(unsigned line, IHTMLElement *elem, const char *exval)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    HRESULT hres;
    BSTR str;

    str = a2bstr(exval);
    hres = IHTMLLinkElement_put_media(link, str);
    ok_(__FILE__,line)(hres == S_OK, "put_media(%s) failed: %08x\n", exval, hres);
    SysFreeString(str);

    hres = IHTMLLinkElement_get_media(link, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_media failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(str, exval), "got %s, expected %s\n", wine_dbgstr_w(str), exval);
    SysFreeString(str);
    IHTMLLinkElement_Release(link);
}

#define test_link_disabled(a,b) _test_link_disabled(__LINE__,a,b)
static void _test_link_disabled(unsigned line, IHTMLElement *elem, VARIANT_BOOL v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    VARIANT_BOOL b = 10;
    HRESULT hres;

    hres = IHTMLLinkElement_get_disabled(link, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_disabled failed: %08x\n", hres);
    ok_(__FILE__,line)(b == v, "disabled = %x, expected %x\n", b, v);

    IHTMLLinkElement_Release(link);
}

#define link_put_disabled(a,b) _link_put_disabled(__LINE__,a,b)
static void _link_put_disabled(unsigned line, IHTMLElement *elem, VARIANT_BOOL v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    HRESULT hres;

    hres = IHTMLLinkElement_put_disabled(link, v);
    ok_(__FILE__,line)(hres == S_OK, "put_disabled failed: %08x\n", hres);
    IHTMLLinkElement_Release(link);
    _test_link_disabled(line, elem, v);
}

#define test_link_rel(a,b) _test_link_rel(__LINE__,a,b)
static void _test_link_rel(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR rel;
    HRESULT hres;

    hres = IHTMLLinkElement_get_rel(link, &rel);
    ok_(__FILE__,line)(hres == S_OK, "get_rel failed: %08x\n", hres);
    if(v)
        ok_(__FILE__,line)(!strcmp_wa(rel, v), "rel = %s, expected %s\n", wine_dbgstr_w(rel), v);
    else
        ok_(__FILE__,line)(!rel, "rel = %s, expected NULL\n", wine_dbgstr_w(rel));

    IHTMLLinkElement_Release(link);
}

#define link_put_rel(a,b) _link_put_rel(__LINE__,a,b)
static void _link_put_rel(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR str = a2bstr(v);
    HRESULT hres;

    hres = IHTMLLinkElement_put_rel(link, str);
    ok_(__FILE__,line)(hres == S_OK, "put_disabled failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLLinkElement_Release(link);
    _test_link_rel(line, elem, v);
}

#define test_link_rev(a,b) _test_link_rev(__LINE__,a,b)
static void _test_link_rev(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR rev;
    HRESULT hres;

    hres = IHTMLLinkElement_get_rev(link, &rev);
    ok_(__FILE__,line)(hres == S_OK, "get_rev failed: %08x\n", hres);
    if(v)
        ok_(__FILE__,line)(!strcmp_wa(rev, v), "rev = %s, expected %s\n", wine_dbgstr_w(rev), v);
    else
        ok_(__FILE__,line)(!rev, "rev = %s, expected NULL\n", wine_dbgstr_w(rev));

    IHTMLLinkElement_Release(link);
}

#define link_put_rev(a,b) _link_put_rev(__LINE__,a,b)
static void _link_put_rev(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR str = a2bstr(v);
    HRESULT hres;

    hres = IHTMLLinkElement_put_rev(link, str);
    ok_(__FILE__,line)(hres == S_OK, "put_disabled failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLLinkElement_Release(link);
    _test_link_rev(line, elem, v);
}

#define test_link_type(a,b) _test_link_type(__LINE__,a,b)
static void _test_link_type(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR type;
    HRESULT hres;

    hres = IHTMLLinkElement_get_type(link, &type);
    ok_(__FILE__,line)(hres == S_OK, "get_type failed: %08x\n", hres);
    if(v)
        ok_(__FILE__,line)(!strcmp_wa(type, v), "type = %s, expected %s\n", wine_dbgstr_w(type), v);
    else
        ok_(__FILE__,line)(!type, "type = %s, expected NULL\n", wine_dbgstr_w(type));

    IHTMLLinkElement_Release(link);
}

#define test_script_text(a,b) _test_script_text(__LINE__,a,b)
static void _test_script_text(unsigned line, IHTMLScriptElement *script, const char *extext)
{
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLScriptElement_get_text(script, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_text failed: %08x\n", hres);
    ok(!strcmp_wa(str, extext), "text = %s, expected \"%s\"\n", wine_dbgstr_w(str), extext);
    SysFreeString(str);
}

#define link_put_type(a,b) _link_put_type(__LINE__,a,b)
static void _link_put_type(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR str = a2bstr(v);
    HRESULT hres;

    hres = IHTMLLinkElement_put_type(link, str);
    ok_(__FILE__,line)(hres == S_OK, "put_disabled failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLLinkElement_Release(link);
    _test_link_type(line, elem, v);
}

#define test_link_href(a,b) _test_link_href(__LINE__,a,b)
static void _test_link_href(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR href;
    HRESULT hres;

    hres = IHTMLLinkElement_get_href(link, &href);
    ok_(__FILE__,line)(hres == S_OK, "get_href failed: %08x\n", hres);
    if(v)
        ok_(__FILE__,line)(!strcmp_wa(href, v), "href = %s, expected %s\n", wine_dbgstr_w(href), v);
    else
        ok_(__FILE__,line)(!href, "href = %s, expected NULL\n", wine_dbgstr_w(href));

    IHTMLLinkElement_Release(link);
}

#define link_put_href(a,b) _link_put_href(__LINE__,a,b)
static void _link_put_href(unsigned line, IHTMLElement *elem, const char *v)
{
    IHTMLLinkElement *link = _get_link_iface(line, (IUnknown*)elem);
    BSTR str = a2bstr(v);
    HRESULT hres;

    hres = IHTMLLinkElement_put_href(link, str);
    ok_(__FILE__,line)(hres == S_OK, "put_disabled failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLLinkElement_Release(link);
    _test_link_href(line, elem, v);
}

#define get_elem_doc(e) _get_elem_doc(__LINE__,e)
static IHTMLDocument2 *_get_elem_doc(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem = _get_elem_iface(line, unk);
    IHTMLDocument2 *doc;
    IDispatch *disp;
    HRESULT hres;

    disp = NULL;
    hres = IHTMLElement_get_document(elem, &disp);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);
    ok(disp != NULL, "disp == NULL\n");

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLDocument2, (void**)&doc);
    IDispatch_Release(disp);
    ok(hres == S_OK, "Could not get IHTMLDocument2 iface: %08x\n", hres);

    return doc;
}

#define get_elem_attr_node(a,b,c) _get_elem_attr_node(__LINE__,a,b,c)
static IHTMLDOMAttribute *_get_elem_attr_node(unsigned line, IUnknown *unk, const char *attr_name, BOOL expect_success)
{
    IHTMLElement4 *elem = _get_elem4_iface(line, unk);
    BSTR str = a2bstr(attr_name);
    IHTMLDOMAttribute *attr;
    HRESULT hres;

    attr = (void*)0xdeadbeef;
    hres = IHTMLElement4_getAttributeNode(elem, str, &attr);
    ok_(__FILE__,line)(hres == S_OK, "getAttributeNode failed: %08x\n", hres);
    if(expect_success)
        ok_(__FILE__,line)(attr != NULL, "attr = NULL\n");
    else
        ok_(__FILE__,line)(!attr, "attr = %p\n", attr);

    IHTMLElement4_Release(elem);
    SysFreeString(str);
    return attr;
}

#define get_attr_node_value(a,b,c) _get_attr_node_value(__LINE__,a,b,c)
static void _get_attr_node_value(unsigned line, IHTMLDOMAttribute *attr, VARIANT *v, VARTYPE vt)
{
    HRESULT hres;

    hres = IHTMLDOMAttribute_get_nodeValue(attr, v);
    ok_(__FILE__,line) (hres == S_OK, "get_nodeValue failed: %08x\n", hres);
    ok_(__FILE__,line) (V_VT(v) == vt, "vt=%d, expected %d\n", V_VT(v), vt);
}

#define put_attr_node_value(a,b) _put_attr_node_value(__LINE__,a,b)
static void _put_attr_node_value(unsigned line, IHTMLDOMAttribute *attr, VARIANT v)
{
    HRESULT hres;

    hres = IHTMLDOMAttribute_put_nodeValue(attr, v);
    ok_(__FILE__,line) (hres == S_OK, "put_nodeValue failed: %08x\n", hres);
}

#define get_window_doc(e) _get_window_doc(__LINE__,e)
static IHTMLDocument2 *_get_window_doc(unsigned line, IHTMLWindow2 *window)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    doc = NULL;
    hres = IHTMLWindow2_get_document(window, &doc);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);
    ok(doc != NULL, "disp == NULL\n");

    return doc;
}

#define doc_get_body(d) _doc_get_body(__LINE__,d)
static IHTMLElement *_doc_get_body(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok_(__FILE__,line)(hres == S_OK, "get_body failed: %08x\n", hres);
    ok_(__FILE__,line)(elem != NULL, "body == NULL\n");

    return elem;
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
    SysFreeString(tmp);

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
    SysFreeString(tmp);
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

#define test_node_insertbefore(n,c,v) _test_node_insertbefore(__LINE__,n,c,v)
static IHTMLDOMNode *_test_node_insertbefore(unsigned line, IUnknown *node_unk, IHTMLDOMNode *child, VARIANT *var)
{
    IHTMLDOMNode *node = _get_node_iface(line, node_unk);
    IHTMLDOMNode *new_child = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode_insertBefore(node, child, *var, &new_child);
    ok_(__FILE__,line) (hres == S_OK, "insertBefore failed: %08x\n", hres);
    ok_(__FILE__,line) (new_child != NULL, "new_child == NULL\n");
    /* TODO  ok_(__FILE__,line) (new_child != child, "new_child == child\n"); */

    IHTMLDOMNode_Release(node);

    return new_child;
}

#define test_node_remove_child(n,c) _test_node_remove_child(__LINE__,n,c)
static void _test_node_remove_child(unsigned line, IUnknown *unk, IHTMLDOMNode *child)
{
    IHTMLDOMNode *node = _get_node_iface(line, unk);
    IHTMLDOMNode *new_node = NULL;
    HRESULT hres;

    hres = IHTMLDOMNode_removeChild(node, child, &new_node);
    ok_(__FILE__,line) (hres == S_OK, "removeChild failed: %08x\n", hres);
    ok_(__FILE__,line) (new_node != NULL, "new_node == NULL\n");
    /* TODO ok_(__FILE__,line) (new_node != child, "new_node == child\n"); */

    IHTMLDOMNode_Release(node);
    IHTMLDOMNode_Release(new_node);
}

#define test_doc_title(d,t) _test_doc_title(__LINE__,d,t)
static void _test_doc_title(unsigned line, IHTMLDocument2 *doc, const char *extitle)
{
    BSTR title = NULL;
    HRESULT hres;

    hres = IHTMLDocument2_get_title(doc, &title);
    ok_(__FILE__,line) (hres == S_OK, "get_title failed: %08x\n", hres);
    ok_(__FILE__,line) (!strcmp_wa(title, extitle), "unexpected title %s\n", wine_dbgstr_w(title));
    SysFreeString(title);
}

#define test_doc_set_title(d,t) _test_doc_set_title(__LINE__,d,t)
static void _test_doc_set_title(unsigned line, IHTMLDocument2 *doc, const char *title)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(title);
    hres = IHTMLDocument2_put_title(doc, tmp);
    ok_(__FILE__,line) (hres == S_OK, "get_title failed: %08x\n", hres);
    SysFreeString(tmp);
}

static void test_elem_bounding_client_rect(IUnknown *unk)
{
    IHTMLRect *rect, *rect2;
    IHTMLElement2 *elem2;
    LONG l;
    HRESULT hres;

    elem2 = get_elem2_iface(unk);
    hres = IHTMLElement2_getBoundingClientRect(elem2, &rect);
    ok(hres == S_OK, "getBoundingClientRect failed: %08x\n", hres);
    hres = IHTMLElement2_getBoundingClientRect(elem2, &rect2);
    IHTMLElement2_Release(elem2);
    ok(hres == S_OK, "getBoundingClientRect failed: %08x\n", hres);
    ok(rect != NULL, "rect == NULL\n");
    ok(rect != rect2, "rect == rect2\n");
    IHTMLRect_Release(rect2);

    test_disp((IUnknown*)rect, &IID_IHTMLRect, "[object]");

    l = 0xdeadbeef;
    hres = IHTMLRect_get_top(rect, &l);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);
    ok(l != 0xdeadbeef, "l = 0xdeadbeef\n");

    l = 0xdeadbeef;
    hres = IHTMLRect_get_left(rect, &l);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(l != 0xdeadbeef, "l = 0xdeadbeef\n");

    l = 0xdeadbeef;
    hres = IHTMLRect_get_bottom(rect, &l);
    ok(hres == S_OK, "get_bottom failed: %08x\n", hres);
    ok(l != 0xdeadbeef, "l = 0xdeadbeef\n");

    l = 0xdeadbeef;
    hres = IHTMLRect_get_right(rect, &l);
    ok(hres == S_OK, "get_right failed: %08x\n", hres);
    ok(l != 0xdeadbeef, "l = 0xdeadbeef\n");

    IHTMLRect_Release(rect);
}

static void test_elem_col_item(IHTMLElementCollection *col, const char *n,
        const elem_type_t *elem_types, LONG len)
{
    IDispatch *disp;
    VARIANT name, index;
    DWORD i;
    HRESULT hres;

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_BSTR;
    V_BSTR(&name) = a2bstr(n);

    hres = IHTMLElementCollection_item(col, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);

    test_elem_collection((IUnknown*)disp, elem_types, len);
    IDispatch_Release(disp);

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

    SysFreeString(V_BSTR(&name));
}

static IHTMLElement *get_elem_by_id(IHTMLDocument2 *doc, const char *id, BOOL expect_success)
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
    V_BSTR(&name) = a2bstr(id);

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

static IHTMLElement *get_doc_elem_by_id(IHTMLDocument2 *doc, const char *id)
{
    IHTMLDocument3 *doc3;
    IHTMLElement *elem;
    BSTR tmp;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);

    tmp = a2bstr(id);
    hres = IHTMLDocument3_getElementById(doc3, tmp, &elem);
    SysFreeString(tmp);
    ok(hres == S_OK, "getElementById(%s) failed: %08x\n", id, hres);

    IHTMLDocument3_Release(doc3);

    return elem;
}

static void test_select_elem(IHTMLSelectElement *select)
{
    IDispatch *disp, *disp2;
    VARIANT name, index;
    HRESULT hres;

    test_select_type(select, "select-one");
    test_select_length(select, 2);
    test_select_selidx(select, 0);
    test_select_put_selidx(select, 1);

    test_select_set_value(select, "val1");
    test_select_value(select, "val1");

    test_select_size(select, 0);
    test_select_set_size(select, 1, S_OK);
    test_select_size(select, 1);

    test_select_set_size(select, -1, CTL_E_INVALIDPROPERTYVALUE);
    test_select_size(select, 1);
    test_select_set_size(select, 3, S_OK);
    test_select_size(select, 3);

    test_select_name(select, NULL);
    test_select_set_name(select, "select-name");
    test_select_name(select, "select-name");
    test_select_form_notfound(select);

    test_select_get_disabled(select, VARIANT_FALSE);
    test_select_set_disabled(select, VARIANT_TRUE);
    test_select_set_disabled(select, VARIANT_FALSE);

    disp = NULL;
    hres = IHTMLSelectElement_get_options(select, &disp);
    ok(hres == S_OK, "get_options failed: %08x\n", hres);
    ok(disp != NULL, "options == NULL\n");
    ok(iface_cmp((IUnknown*)disp, (IUnknown*)select), "disp != select\n");
    IDispatch_Release(disp);

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_I4;
    V_I4(&name) = -1;
    disp = (void*)0xdeadbeef;
    hres = IHTMLSelectElement_item(select, name, index, &disp);
    ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
    ok(!disp, "disp = %p\n", disp);

    V_I4(&name) = 2;
    disp = (void*)0xdeadbeef;
    hres = IHTMLSelectElement_item(select, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(!disp, "disp = %p\n", disp);

    V_I4(&name) = 1;
    hres = IHTMLSelectElement_item(select, name, index, NULL);
    ok(hres == E_POINTER || broken(hres == E_INVALIDARG), "item failed: %08x, expected E_POINTER\n", hres);

    disp = NULL;
    hres = IHTMLSelectElement_item(select, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(disp != NULL, "disp = NULL\n");
    test_disp((IUnknown*)disp, &DIID_DispHTMLOptionElement, NULL);

    V_VT(&index) = VT_I4;
    V_I4(&index) = 1;
    disp2 = NULL;
    hres = IHTMLSelectElement_item(select, name, index, &disp2);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(disp2 != NULL, "disp = NULL\n");
    ok(iface_cmp((IUnknown*)disp, (IUnknown*)disp2), "disp != disp2\n");
    IDispatch_Release(disp2);
    IDispatch_Release(disp);

    test_select_multiple(select, VARIANT_FALSE);
    test_select_set_multiple(select, VARIANT_TRUE);
    test_select_remove(select);
}

static void test_form_item(IHTMLElement *elem)
{
    IHTMLFormElement *form = get_form_iface((IUnknown*)elem);
    IDispatch *disp, *disp2;
    VARIANT name, index;
    HRESULT hres;

    V_VT(&index) = VT_EMPTY;
    V_VT(&name) = VT_I4;
    V_I4(&name) = -1;
    disp = (void*)0xdeadbeef;
    hres = IHTMLFormElement_item(form, name, index, &disp);
    ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);
    ok(!disp, "disp = %p\n", disp);

    V_I4(&name) = 2;
    disp = (void*)0xdeadbeef;
    hres = IHTMLFormElement_item(form, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(!disp, "disp = %p\n", disp);

    V_I4(&name) = 1;
    hres = IHTMLFormElement_item(form, name, index, NULL);
    ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);

    disp = NULL;
    hres = IHTMLFormElement_item(form, name, index, &disp);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(disp != NULL, "disp = NULL\n");
    test_disp((IUnknown*)disp, &DIID_DispHTMLInputElement, NULL);

    V_VT(&index) = VT_I4;
    V_I4(&index) = 1;
    disp2 = NULL;
    hres = IHTMLFormElement_item(form, name, index, &disp2);
    ok(hres == S_OK, "item failed: %08x\n", hres);
    ok(disp2 != NULL, "disp = NULL\n");
    ok(iface_cmp((IUnknown*)disp, (IUnknown*)disp2), "disp != disp2\n");
    IDispatch_Release(disp2);
    IDispatch_Release(disp);
}

static void test_create_option_elem(IHTMLDocument2 *doc)
{
    IHTMLOptionElement *option;

    option = create_option_elem(doc, "test text", "test value");

    test_option_put_text(option, "new text");
    test_option_put_value(option, "new value");
    test_option_get_index(option, 0);
    test_option_defaultSelected_property(option);
    test_option_put_selected(option, VARIANT_TRUE);
    test_option_put_selected(option, VARIANT_FALSE);

    IHTMLOptionElement_Release(option);
}

static void test_option_form(IUnknown *uoption, IUnknown  *uform)
{
    IHTMLOptionElement *option = get_option_iface(uoption);
    IHTMLFormElement *form;
    HRESULT hres;

    hres = IHTMLOptionElement_get_form(option, NULL);
    ok(hres == E_POINTER, "got %08x\n, expected E_POINTER\n", hres);

    hres = IHTMLOptionElement_get_form(option, &form);
    ok(hres == S_OK, "get_form failed: %08x\n", hres);
    ok(form != NULL, "form == NULL\n");

    ok(iface_cmp(uform, (IUnknown*)form), "Expected %p, got %p\n", uform, form);

    IHTMLOptionElement_Release(option);
    IHTMLFormElement_Release(form);
}

static void test_create_img_elem(IHTMLDocument2 *doc)
{
    IHTMLImgElement *img;

    img = create_img_elem(doc, 10, 15);

    if(img){
        test_img_put_width(img, 5);
        test_img_put_height(img, 20);

        IHTMLImgElement_Release(img);
        img = NULL;
    }

    img = create_img_elem(doc, -1, -1);

    if(img){
        test_img_put_width(img, 5);
        test_img_put_height(img, 20);

        IHTMLImgElement_Release(img);
    }
}

#define insert_adjacent_elem(a,b,c) _insert_adjacent_elem(__LINE__,a,b,c)
static void _insert_adjacent_elem(unsigned line, IHTMLElement *parent, const char *where, IHTMLElement *elem)
{
    IHTMLElement2 *elem2 = _get_elem2_iface(line, (IUnknown*)parent);
    IHTMLElement *ret_elem = NULL;
    BSTR str = a2bstr(where);
    HRESULT hres;

    hres = IHTMLElement2_insertAdjacentElement(elem2, str, elem, &ret_elem);
    IHTMLElement2_Release(elem2);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "insertAdjacentElement failed: %08x\n", hres);
    ok_(__FILE__,line)(ret_elem == elem, "ret_elem != elem\n");
    IHTMLElement_Release(ret_elem);
}

static void test_insert_adjacent_elems(IHTMLDocument2 *doc, IHTMLElement *parent)
{
    IHTMLElement *elem, *elem2;

    static const elem_type_t br_br[] = {ET_BR, ET_BR};
    static const elem_type_t br_div_br[] = {ET_BR, ET_DIV, ET_BR};

    elem = test_create_elem(doc, "BR");
    insert_adjacent_elem(parent, "BeforeEnd", elem);
    IHTMLElement_Release(elem);

    test_elem_all((IUnknown*)parent, br_br, 1);

    elem = test_create_elem(doc, "BR");
    insert_adjacent_elem(parent, "beforeend", elem);

    test_elem_all((IUnknown*)parent, br_br, 2);

    elem2 = test_create_elem(doc, "DIV");
    insert_adjacent_elem(elem, "beforebegin", elem2);
    IHTMLElement_Release(elem2);
    IHTMLElement_Release(elem);

    test_elem_all((IUnknown*)parent, br_div_br, 3);
}

static IHTMLTxtRange *test_create_body_range(IHTMLDocument2 *doc)
{
    IHTMLBodyElement *body;
    IHTMLTxtRange *range;
    IHTMLElement *elem;
    HRESULT hres;

    elem = doc_get_body(doc);
    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLBodyElement, (void**)&body);
    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);
    IHTMLElement_Release(elem);

    hres = IHTMLBodyElement_createTextRange(body, &range);
    IHTMLBodyElement_Release(body);
    ok(hres == S_OK, "createTextRange failed: %08x\n", hres);

    return range;
}

#define range_duplicate(a) _range_duplicate(__LINE__,a)
static IHTMLTxtRange *_range_duplicate(unsigned line, IHTMLTxtRange *range)
{
    IHTMLTxtRange *ret;
    HRESULT hres;

    hres = IHTMLTxtRange_duplicate(range, &ret);
    ok_(__FILE__,line)(hres == S_OK, "duplicate failed: %08x\n", hres);

    return ret;
}

#define test_range_set_end_point(a,b,c,d) _test_range_set_end_point(__LINE__,a,b,c,d)
static void _test_range_set_end_point(unsigned line, IHTMLTxtRange *range, const char *how,
        IHTMLTxtRange *ref_range, HRESULT exhres)
{
    BSTR str = a2bstr(how);
    HRESULT hres;

    hres = IHTMLTxtRange_setEndPoint(range, str, ref_range);
    ok_(__FILE__,line)(hres == exhres, "setEndPoint failed: %08x, expected %08x\n", hres, exhres);
    SysFreeString(str);
}

static void test_txtrange(IHTMLDocument2 *doc)
{
    IHTMLTxtRange *body_range, *range, *range2;
    IHTMLSelectionObject *selection;
    IDispatch *disp_range;
    IHTMLElement *body;
    HRESULT hres;

    body_range = test_create_body_range(doc);

    test_disp((IUnknown*)body_range, &IID_IHTMLTxtRange, "[object]");

    test_range_text(body_range, "test abc 123\r\nit's text");

    range = range_duplicate(body_range);
    range2 = range_duplicate(body_range);

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

    range = range_duplicate(body_range);

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

    range = range_duplicate(body_range);

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

    range = range_duplicate(body_range);

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

    range = range_duplicate(body_range);

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

    hres = IHTMLDocument2_get_selection(doc, &selection);
    ok(hres == S_OK, "IHTMLDocument2_get_selection failed: %08x\n", hres);

    test_disp((IUnknown*)selection, &IID_IHTMLSelectionObject, "[object]");
    test_ifaces((IUnknown*)selection, selection_iids);

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

    body = doc_get_body(doc);

    hres = IHTMLTxtRange_moveToElementText(range, body);
    ok(hres == S_OK, "moveToElementText failed: %08x\n", hres);

    test_range_text(range, "abc xyz abc 123\r\nit's text");
    test_range_parent(range, ET_BODY);

    test_range_move(range, wordW, 1, 1);
    test_range_moveend(range, characterW, 12, 12);
    test_range_text(range, "xyz abc 123");

    test_range_collapse(range, VARIANT_TRUE);
    test_range_paste_html(range, "<br>paste<br>");
    test_range_text(range, NULL);

    test_range_moveend(range, characterW, 3, 3);
    test_range_text(range, "xyz");

    hres = IHTMLTxtRange_moveToElementText(range, body);
    ok(hres == S_OK, "moveToElementText failed: %08x\n", hres);

    test_range_text(range, "abc \r\npaste\r\nxyz abc 123\r\nit's text");

    test_range_move(range, wordW, 2, 2);
    test_range_collapse(range, VARIANT_TRUE);
    test_range_moveend(range, characterW, 5, 5);
    test_range_text(range, "paste");

    range2 = range_duplicate(range);

    test_range_set_end_point(range, "starttostart", body_range, S_OK);
    test_range_text(range, "abc \r\npaste");

    test_range_set_end_point(range, "endtoend", body_range, S_OK);
    test_range_text(range, "abc \r\npaste\r\nxyz abc 123\r\nit's text");

    test_range_set_end_point(range, "starttoend", range2, S_OK);
    test_range_text(range, "\r\nxyz abc 123\r\nit's text");

    test_range_set_end_point(range, "starttostart", body_range, S_OK);
    test_range_set_end_point(range, "endtostart", range2, S_OK);
    test_range_text(range, "abc ");

    test_range_set_end_point(range, "starttoend", body_range, S_OK);
    test_range_text(range, "paste\r\nxyz abc 123\r\nit's text");

    test_range_set_end_point(range, "EndToStart", body_range, S_OK);
    test_range_text(range, "abc ");

    test_range_set_end_point(range, "xxx", body_range, E_INVALIDARG);

    IHTMLTxtRange_Release(range);
    IHTMLTxtRange_Release(range2);
    IHTMLTxtRange_Release(body_range);
    IHTMLElement_Release(body);

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

#define test_compatmode(a,b) _test_compatmode(__LINE__,a,b)
static void _test_compatmode(unsigned  line, IHTMLDocument2 *doc2, const char *excompat)
{
    IHTMLDocument5 *doc = get_htmldoc5_iface((IUnknown*)doc2);
    BSTR str;
    HRESULT hres;

    hres = IHTMLDocument5_get_compatMode(doc, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_compatMode failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(str, excompat), "compatMode = %s, expected %s\n", wine_dbgstr_w(str), excompat);

    IHTMLDocument5_Release(doc);
}

static void test_location(IHTMLDocument2 *doc)
{
    IHTMLLocation *location, *location2;
    IHTMLWindow2 *window;
    BSTR str;
    ULONG ref;
    HRESULT hres;

    hres = IHTMLDocument2_get_location(doc, &location);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);

    hres = IHTMLDocument2_get_location(doc, &location2);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);

    ok(location == location2, "location != location2\n");
    IHTMLLocation_Release(location2);

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_get_location(window, &location2);
    ok(hres == S_OK, "get_location failed: %08x\n", hres);
    ok(location == location2, "location != location2\n");
    IHTMLLocation_Release(location2);

    test_ifaces((IUnknown*)location, location_iids);
    test_disp2((IUnknown*)location, &DIID_DispHTMLLocation, &IID_IHTMLLocation, "about:blank");

    hres = IHTMLLocation_get_pathname(location, &str);
    ok(hres == S_OK, "get_pathname failed: %08x\n", hres);
    ok(!strcmp_wa(str, "blank"), "unexpected pathname %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLLocation_get_href(location, NULL);
    ok(hres == E_POINTER, "get_href passed: %08x\n", hres);

    hres = IHTMLLocation_get_href(location, &str);
    ok(hres == S_OK, "get_href failed: %08x\n", hres);
    ok(!strcmp_wa(str, "about:blank"), "unexpected href %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    ref = IHTMLLocation_Release(location);
    ok(!ref, "location chould be destroyed here\n");
}

static void test_plugins_col(IHTMLDocument2 *doc)
{
    IHTMLPluginsCollection *col, *col2;
    IHTMLWindow2 *window;
    IOmNavigator *nav;
    ULONG ref;
    LONG len;
    HRESULT hres;

    window = get_doc_window(doc);
    hres = IHTMLWindow2_get_navigator(window, &nav);
    ok(hres == S_OK, "get_navigator failed: %08x\n", hres);
    IHTMLWindow2_Release(window);

    hres = IOmNavigator_get_plugins(nav, &col);
    ok(hres == S_OK, "get_plugins failed: %08x\n", hres);

    hres = IOmNavigator_get_plugins(nav, &col2);
    ok(hres == S_OK, "get_plugins failed: %08x\n", hres);
    ok(iface_cmp((IUnknown*)col, (IUnknown*)col2), "col != col2\n");
    IHTMLPluginsCollection_Release(col2);

    test_disp2((IUnknown*)col, &DIID_DispCPlugins, &IID_IHTMLPluginsCollection, "[object]");

    len = 0xdeadbeef;
    hres = IHTMLPluginsCollection_get_length(col, &len);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(!len, "length = %d\n", len);

    hres = IHTMLPluginsCollection_refresh(col, VARIANT_FALSE);
    ok(hres == S_OK, "refresh failed: %08x\n", hres);

    hres = IHTMLPluginsCollection_refresh(col, VARIANT_TRUE);
    ok(hres == S_OK, "refresh failed: %08x\n", hres);

    ref = IHTMLPluginsCollection_Release(col);
    ok(!ref, "ref=%d\n", ref);

    IOmNavigator_Release(nav);
}

static void test_mime_types_col(IOmNavigator *nav)
{
    IHTMLMimeTypesCollection *col, *col2;
    LONG length;
    ULONG ref;
    HRESULT hres;

    hres = IOmNavigator_get_mimeTypes(nav, &col);
    ok(hres == S_OK, "get_mimeTypes failed: %08x\n", hres);

    hres = IOmNavigator_get_mimeTypes(nav, &col2);
    ok(hres == S_OK, "get_mimeTypes failed: %08x\n", hres);
    ok(iface_cmp((IUnknown*)col, (IUnknown*)col2), "col != col2\n");
    IHTMLMimeTypesCollection_Release(col2);

    test_disp((IUnknown*)col, &IID_IHTMLMimeTypesCollection, "[object]");

    length = 0xdeadbeef;
    hres = IHTMLMimeTypesCollection_get_length(col, &length);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(!length, "length = %d\n", length);

    ref = IHTMLMimeTypesCollection_Release(col);
    ok(!ref, "ref=%d\n", ref);
}

#define test_framebase_name(a,b) _test_framebase_name(__LINE__,a,b)
static void _test_framebase_name(unsigned line, IHTMLElement *elem, const char *name)
{
    BSTR str = (void*)0xdeadbeef;
    IHTMLFrameBase *fbase;
    HRESULT hres;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLFrameBase, (void**)&fbase);
    ok(hres == S_OK, "Could not get IHTMLFrameBase interface: 0x%08x\n", hres);

    hres = IHTMLFrameBase_get_name(fbase, &str);
    ok_(__FILE__,line)(hres == S_OK, "IHTMLFrameBase_get_name failed: 0x%08x\n", hres);
    if(name)
        ok_(__FILE__,line)(!strcmp_wa(str, name), "name = %s, expected %s\n", wine_dbgstr_w(str), name);
    else
        ok_(__FILE__,line)(!str, "name = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IHTMLFrameBase_Release(fbase);
}

#define test_framebase_put_name(a,b) _test_framebase_put_name(__LINE__,a,b)
static void _test_framebase_put_name(unsigned line, IHTMLElement *elem, const char *name)
{
    IHTMLFrameBase *fbase;
    HRESULT hres;
    BSTR str;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLFrameBase, (void**)&fbase);
    ok(hres == S_OK, "Could not get IHTMLFrameBase interface: 0x%08x\n", hres);

    str = name ? a2bstr(name) : NULL;
    hres = IHTMLFrameBase_put_name(fbase, str);
    ok_(__FILE__,line)(hres == S_OK, "put_name failed: %08x\n", hres);
    SysFreeString(str);

    _test_framebase_name(line, elem, name);
    IHTMLFrameBase_Release(fbase);
}

#define test_framebase_src(a,b) _test_framebase_src(__LINE__,a,b)
static void _test_framebase_src(unsigned line, IHTMLElement *elem, const char *src)
{
    BSTR str = (void*)0xdeadbeef;
    IHTMLFrameBase *fbase;
    HRESULT hres;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLFrameBase, (void**)&fbase);
    ok(hres == S_OK, "Could not get IHTMLFrameBase interface: 0x%08x\n", hres);

    hres = IHTMLFrameBase_get_src(fbase, &str);
    ok_(__FILE__,line)(hres == S_OK, "IHTMLFrameBase_get_src failed: 0x%08x\n", hres);
    if(src)
        ok_(__FILE__,line)(!strcmp_wa(str, src), "src = %s, expected %s\n", wine_dbgstr_w(str), src);
    else
        ok_(__FILE__,line)(!str, "src = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IHTMLFrameBase_Release(fbase);
}

#define test_framebase_marginheight(a,b) _test_framebase_marginheight(__LINE__,a,b)
static void _test_framebase_marginheight(unsigned line, IHTMLFrameBase *framebase, const char *exval)
{
    VARIANT v;
    HRESULT hres;

    hres = IHTMLFrameBase_get_marginHeight(framebase, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_marginHeight failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_BSTR, "V_VT(marginHeight) = %d\n", V_VT(&v));
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&v), exval), "marginHeight = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), exval);
    else
        ok_(__FILE__,line)(!V_BSTR(&v), "marginHeight = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

#define set_framebase_marginheight(a,b) _set_framebase_marginheight(__LINE__,a,b)
static void _set_framebase_marginheight(unsigned line, IHTMLFrameBase *framebase, const char *val)
{
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr(val);
    hres = IHTMLFrameBase_put_marginHeight(framebase, v);
    ok_(__FILE__,line)(hres == S_OK, "put_marginHeight failed: %08x\n", hres);
    VariantClear(&v);
}

#define test_framebase_marginwidth(a,b) _test_framebase_marginwidth(__LINE__,a,b)
static void _test_framebase_marginwidth(unsigned line, IHTMLFrameBase *framebase, const char *exval)
{
    VARIANT v;
    HRESULT hres;

    hres = IHTMLFrameBase_get_marginWidth(framebase, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_marginWidth failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_BSTR, "V_VT(marginWidth) = %d\n", V_VT(&v));
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&v), exval), "marginWidth = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), exval);
    else
        ok_(__FILE__,line)(!V_BSTR(&v), "marginWidth = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

#define set_framebase_marginwidth(a,b) _set_framebase_marginwidth(__LINE__,a,b)
static void _set_framebase_marginwidth(unsigned line, IHTMLFrameBase *framebase, const char *val)
{
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr(val);
    hres = IHTMLFrameBase_put_marginWidth(framebase, v);
    ok_(__FILE__,line)(hres == S_OK, "put_marginWidth failed: %08x\n", hres);
    VariantClear(&v);
}

static void test_framebase(IUnknown *unk)
{
    IHTMLFrameBase *fbase;
    BSTR str;
    HRESULT hres;

    /* get/put scrolling */
    hres = IUnknown_QueryInterface(unk, &IID_IHTMLFrameBase, (void**)&fbase);
    ok(hres == S_OK, "Could not get IHTMLFrameBase interface: 0x%08x\n", hres);

    hres = IHTMLFrameBase_get_scrolling(fbase, &str);
    ok(hres == S_OK, "IHTMLFrameBase_get_scrolling failed: 0x%08x\n", hres);
    ok(!strcmp_wa(str, "auto"), "get_scrolling should have given 'auto', gave: %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("no");
    hres = IHTMLFrameBase_put_scrolling(fbase, str);
    ok(hres == S_OK, "IHTMLFrameBase_put_scrolling failed: 0x%08x\n", hres);
    SysFreeString(str);

    hres = IHTMLFrameBase_get_scrolling(fbase, &str);
    ok(hres == S_OK, "IHTMLFrameBase_get_scrolling failed: 0x%08x\n", hres);
    ok(!strcmp_wa(str, "no"), "get_scrolling should have given 'no', gave: %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("junk");
    hres = IHTMLFrameBase_put_scrolling(fbase, str);
    ok(hres == E_INVALIDARG, "IHTMLFrameBase_put_scrolling should have failed "
            "with E_INVALIDARG, instead: 0x%08x\n", hres);
    SysFreeString(str);

    hres = IHTMLFrameBase_get_scrolling(fbase, &str);
    ok(hres == S_OK, "IHTMLFrameBase_get_scrolling failed: 0x%08x\n", hres);
    ok(!strcmp_wa(str, "no"), "get_scrolling should have given 'no', gave: %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLFrameBase_get_frameBorder(fbase, &str);
    ok(hres == S_OK, "get_frameBorder failed: %08x\n", hres);
    ok(!str, "frameBorder = %s\n", wine_dbgstr_w(str));

    str = a2bstr("1");
    hres = IHTMLFrameBase_put_frameBorder(fbase, str);
    ok(hres == S_OK, "put_frameBorder failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLFrameBase_get_frameBorder(fbase, &str);
    ok(hres == S_OK, "get_frameBorder failed: %08x\n", hres);
    ok(!strcmp_wa(str, "1"), "frameBorder = %s, expected \"1\"\n", wine_dbgstr_w(str));

    test_framebase_marginheight(fbase, NULL);
    set_framebase_marginheight(fbase, "1px");
    test_framebase_marginheight(fbase, "1");

    test_framebase_marginwidth(fbase, NULL);
    set_framebase_marginwidth(fbase, "2px");
    test_framebase_marginwidth(fbase, "2");

    IHTMLFrameBase_Release(fbase);
}

#define test_language_string(a,b) _test_language_string(__LINE__,a,b)
static void _test_language_string(unsigned line, const WCHAR *lang, LCID lcid)
{
    WCHAR buf[64];
    int res;

    if(pLCIDToLocaleName) {
        res = pLCIDToLocaleName(lcid, buf, sizeof(buf)/sizeof(WCHAR), 0);
        ok_(__FILE__,line)(res, "LCIDToLocaleName failed: %u\n", GetLastError());
        ok_(__FILE__,line)(!lstrcmpW(lang, buf), "lang = %s, expected %s\n", wine_dbgstr_w(lang), wine_dbgstr_w(buf));
    }else {
        win_skip("LCIDToLocaleName not available, unable to test language string\n");
        ok_(__FILE__,line)(lang != NULL, "lang == NULL\n");
    }
}

#define test_table_length(t,l)  _test_table_length(__LINE__,t,l)
static void _test_table_length(unsigned line, IHTMLTable *table, LONG expect)
{
    IHTMLElementCollection *col;
    HRESULT hres;
    LONG len;

    hres = IHTMLTable_get_rows(table, &col);
    ok_(__FILE__,line)(hres == S_OK, "get_rows failed: %08x\n", hres);
    ok_(__FILE__,line)(col != NULL, "col = NULL\n");
    if (hres != S_OK || col == NULL)
        return;
    hres = IHTMLElementCollection_get_length(col, &len);
    ok_(__FILE__,line)(hres == S_OK, "get_length failed: %08x\n", hres);
    ok_(__FILE__,line)(len == expect, "Expect %d, got %d\n", expect, len);

    IHTMLElementCollection_Release(col);
}

static void test_navigator(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    IOmNavigator *navigator, *navigator2;
    VARIANT_BOOL b;
    char buf[512];
    DWORD size;
    ULONG ref;
    BSTR bstr;
    HRESULT hres;

    static const WCHAR v40[] = {'4','.','0'};

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "parentWidnow failed: %08x\n", hres);

    hres = IHTMLWindow2_get_navigator(window, &navigator);
    ok(hres == S_OK, "get_navigator failed: %08x\n", hres);
    ok(navigator != NULL, "navigator == NULL\n");
    test_disp2((IUnknown*)navigator, &DIID_DispHTMLNavigator, &IID_IOmNavigator, "[object]");

    hres = IHTMLWindow2_get_navigator(window, &navigator2);
    ok(hres == S_OK, "get_navigator failed: %08x\n", hres);
    ok(navigator != navigator2, "navigator2 != navihgator\n");

    IHTMLWindow2_Release(window);
    IOmNavigator_Release(navigator2);

    hres = IOmNavigator_get_appCodeName(navigator, &bstr);
    ok(hres == S_OK, "get_appCodeName failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, "Mozilla"), "Unexpected appCodeName %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = NULL;
    hres = IOmNavigator_get_appName(navigator, &bstr);
    ok(hres == S_OK, "get_appName failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, "Microsoft Internet Explorer"), "Unexpected appCodeName %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = NULL;
    hres = IOmNavigator_get_platform(navigator, &bstr);
    ok(hres == S_OK, "get_platform failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, sizeof(void*) == 8 ? "Win64" : "Win32")
       || (sizeof(void*) == 8 && broken(!strcmp_wa(bstr, "Win32") /* IE6 */)), "unexpected platform %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = NULL;
    hres = IOmNavigator_get_cpuClass(navigator, &bstr);
    ok(hres == S_OK, "get_cpuClass failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, sizeof(void*) == 8 ? "x64" : "x86"), "unexpected cpuClass %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = NULL;
    hres = IOmNavigator_get_appVersion(navigator, &bstr);
    ok(hres == S_OK, "get_appVersion failed: %08x\n", hres);
    ok(!memcmp(bstr, v40, sizeof(v40)), "appVersion is %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = NULL;
    hres = IOmNavigator_get_systemLanguage(navigator, &bstr);
    ok(hres == S_OK, "get_systemLanguage failed: %08x\n", hres);
    test_language_string(bstr, LOCALE_SYSTEM_DEFAULT);
    SysFreeString(bstr);

    if (pGetUserDefaultUILanguage)
    {
        bstr = NULL;
        hres = IOmNavigator_get_browserLanguage(navigator, &bstr);
        ok(hres == S_OK, "get_browserLanguage failed: %08x\n", hres);
        test_language_string(bstr, pGetUserDefaultUILanguage());
        SysFreeString(bstr);
    }
    else
        win_skip("GetUserDefaultUILanguage not available\n");

    bstr = NULL;
    hres = IOmNavigator_get_userLanguage(navigator, &bstr);
    ok(hres == S_OK, "get_userLanguage failed: %08x\n", hres);
    test_language_string(bstr, LOCALE_USER_DEFAULT);
    SysFreeString(bstr);

    hres = IOmNavigator_toString(navigator, NULL);
    ok(hres == E_INVALIDARG, "toString failed: %08x\n", hres);

    bstr = NULL;
    hres = IOmNavigator_toString(navigator, &bstr);
    ok(hres == S_OK, "toString failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, "[object]"), "toString returned %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    b = 100;
    hres = IOmNavigator_get_onLine(navigator, &b);
    ok(hres == S_OK, "get_onLine failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "onLine = %x\n", b);

    size = sizeof(buf);
    hres = ObtainUserAgentString(0, buf, &size);
    ok(hres == S_OK, "ObtainUserAgentString failed: %08x\n", hres);

    bstr = NULL;
    hres = IOmNavigator_get_userAgent(navigator, &bstr);
    ok(hres == S_OK, "get_userAgent failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, buf), "userAgent returned %s, expected \"%s\"\n", wine_dbgstr_w(bstr), buf);
    SysFreeString(bstr);

    if(!strncmp(buf, "Mozilla/", 8)) {
        bstr = NULL;
        hres = IOmNavigator_get_appVersion(navigator, &bstr);
        ok(hres == S_OK, "get_appVersion failed: %08x\n", hres);
        ok(!strcmp_wa(bstr, buf+8), "appVersion returned %s, expected \"%s\"\n", wine_dbgstr_w(bstr), buf+8);
        SysFreeString(bstr);
    }else {
        skip("nonstandard user agent\n");
    }

    bstr = NULL;
    hres = IOmNavigator_get_appMinorVersion(navigator, &bstr);
    ok(hres == S_OK, "get_appMonorVersion failed: %08x\n", hres);
    ok(bstr != NULL, "appMinorVersion returned NULL\n");
    SysFreeString(bstr);

    test_mime_types_col(navigator);

    ref = IOmNavigator_Release(navigator);
    ok(!ref, "navigator should be destroyed here\n");
}

static void test_screen(IHTMLWindow2 *window)
{
    IHTMLScreen *screen, *screen2;
    IDispatchEx *dispex;
    RECT work_area;
    LONG l, exl;
    HDC hdc;
    HRESULT hres;

    screen = NULL;
    hres = IHTMLWindow2_get_screen(window, &screen);
    ok(hres == S_OK, "get_screen failed: %08x\n", hres);
    ok(screen != NULL, "screen == NULL\n");

    screen2 = NULL;
    hres = IHTMLWindow2_get_screen(window, &screen2);
    ok(hres == S_OK, "get_screen failed: %08x\n", hres);
    ok(screen2 != NULL, "screen == NULL\n");
    ok(iface_cmp((IUnknown*)screen2, (IUnknown*)screen), "screen2 != screen\n");
    IHTMLScreen_Release(screen2);

    hres = IHTMLScreen_QueryInterface(screen, &IID_IDispatchEx, (void**)&dispex);
    ok(hres == S_OK || broken(hres == E_NOINTERFACE), "Could not get IDispatchEx interface: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        test_disp((IUnknown*)screen, &DIID_DispHTMLScreen, "[object]");
        IDispatchEx_Release(dispex);
    }

    hdc = CreateICA("DISPLAY", NULL, NULL, NULL);

    exl = GetDeviceCaps(hdc, HORZRES);
    l = 0xdeadbeef;
    hres = IHTMLScreen_get_width(screen, &l);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(l == exl, "width = %d, expected %d\n", l, exl);

    exl = GetDeviceCaps(hdc, VERTRES);
    l = 0xdeadbeef;
    hres = IHTMLScreen_get_height(screen, &l);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(l == exl, "height = %d, expected %d\n", l, exl);

    exl = GetDeviceCaps(hdc, BITSPIXEL);
    l = 0xdeadbeef;
    hres = IHTMLScreen_get_colorDepth(screen, &l);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(l == exl, "height = %d, expected %d\n", l, exl);

    DeleteObject(hdc);

    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0);

    l = 0xdeadbeef;
    hres = IHTMLScreen_get_availHeight(screen, &l);
    ok(hres == S_OK, "get_availHeight failed: %08x\n", hres);
    ok(l == work_area.bottom-work_area.top, "availHeight = %d, expected %d\n", l, work_area.bottom-work_area.top);

    l = 0xdeadbeef;
    hres = IHTMLScreen_get_availWidth(screen, &l);
    ok(hres == S_OK, "get_availWidth failed: %08x\n", hres);
    ok(l == work_area.right-work_area.left, "availWidth = %d, expected %d\n", l, work_area.right-work_area.left);

    IHTMLScreen_Release(screen);
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
    ok(!strcmp_wa(str, "None"), "type = %s\n", wine_dbgstr_w(str));
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

static void test_doc_elem(IHTMLDocument2 *doc)
{
    IHTMLDocument2 *doc_node, *owner_doc;
    IHTMLElement *elem;
    IHTMLDocument3 *doc3;
    HRESULT hres;
    BSTR bstr;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument3) failed: %08x\n", hres);

    hres = IHTMLDocument2_toString(doc, &bstr);
    ok(hres == S_OK, "toString failed: %08x\n", hres);
    ok(!strcmp_wa(bstr, "[object]"),
            "toString returned %s, expected [object]\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hres = IHTMLDocument3_get_documentElement(doc3, &elem);
    IHTMLDocument3_Release(doc3);
    ok(hres == S_OK, "get_documentElement failed: %08x\n", hres);

    test_node_name((IUnknown*)elem, "HTML");
    test_elem_tag((IUnknown*)elem, "HTML");

    doc_node = get_doc_node(doc);
    owner_doc = get_owner_doc((IUnknown*)elem);
    ok(iface_cmp((IUnknown *)doc_node, (IUnknown *)owner_doc), "doc_node != owner_doc\n");
    IHTMLDocument2_Release(owner_doc);

    owner_doc = get_owner_doc((IUnknown*)doc_node);
    ok(!owner_doc, "owner_doc = %p\n", owner_doc);
    IHTMLDocument2_Release(doc_node);

    test_elem_client_rect((IUnknown*)elem);

    IHTMLElement_Release(elem);
}

static void test_default_body(IHTMLBodyElement *body)
{
    LONG l;
    BSTR bstr;
    HRESULT hres;
    VARIANT v;

    bstr = (void*)0xdeadbeef;
    hres = IHTMLBodyElement_get_background(body, &bstr);
    ok(hres == S_OK, "get_background failed: %08x\n", hres);
    ok(bstr == NULL, "bstr != NULL\n");

    l = elem_get_scroll_height((IUnknown*)body);
    ok(l != -1, "scrollHeight == -1\n");
    l = elem_get_scroll_width((IUnknown*)body);
    ok(l != -1, "scrollWidth == -1\n");
    l = elem_get_scroll_top((IUnknown*)body);
    ok(!l, "scrollTop = %d\n", l);
    elem_get_scroll_left((IUnknown*)body);

    test_elem_dir((IUnknown*)body, NULL);
    set_elem_dir((IUnknown*)body, "ltr");

    /* get_text tests */
    hres = IHTMLBodyElement_get_text(body, &v);
    ok(hres == S_OK, "expect S_OK got 0x%08d\n", hres);
    ok(V_VT(&v) == VT_BSTR, "Expected VT_BSTR got %d\n", V_VT(&v));
    ok(V_BSTR(&v) == NULL, "bstr != NULL\n");

    /* get_text - Invalid Text */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("Invalid");
    hres = IHTMLBodyElement_put_text(body, v);
    ok(hres == S_OK, "expect S_OK got 0x%08d\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_NULL;
    hres = IHTMLBodyElement_get_text(body, &v);
    ok(hres == S_OK, "expect S_OK got 0x%08d\n", hres);
    ok(V_VT(&v) == VT_BSTR, "Expected VT_BSTR got %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "#00a0d0"), "v = %s, expected '#00a0d0'\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* get_text - Valid Text */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("#FF0000");
    hres = IHTMLBodyElement_put_text(body, v);
    ok(hres == S_OK, "expect S_OK got 0x%08d\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_NULL;
    hres = IHTMLBodyElement_get_text(body, &v);
    ok(hres == S_OK, "expect S_OK got 0x%08d\n", hres);
    ok(V_VT(&v) == VT_BSTR, "Expected VT_BSTR got %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "#ff0000"), "v = %s, expected '#ff0000'\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

#define test_body_scroll(a,b) _test_body_scroll(__LINE__,a,b)
static void _test_body_scroll(unsigned line, IHTMLBodyElement *body, const char *ex)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLBodyElement_get_scroll(body, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_scroll failed: %08x\n", hres);
    ok_(__FILE__,line)(ex ? !strcmp_wa(str, ex) : !str, "scroll = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

#define set_body_scroll(a,b) _set_body_scroll(__LINE__,a,b)
static void _set_body_scroll(unsigned line, IHTMLBodyElement *body, const char *val)
{
    BSTR str = a2bstr(val);
    HRESULT hres;

    hres = IHTMLBodyElement_put_scroll(body, str);
    ok_(__FILE__,line)(hres == S_OK, "put_scroll failed: %08x\n", hres);
    SysFreeString(str);

    _test_body_scroll(line, body, val);
}

static void test_body_funs(IHTMLBodyElement *body)
{
    VARIANT vbg, vDefaultbg;
    HRESULT hres;

    hres = IHTMLBodyElement_get_bgColor(body, &vDefaultbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vDefaultbg) == VT_BSTR, "bstr != NULL\n");
    ok(!V_BSTR(&vDefaultbg), "V_BSTR(bgColor) = %s\n", wine_dbgstr_w(V_BSTR(&vDefaultbg)));

    V_VT(&vbg) = VT_BSTR;
    V_BSTR(&vbg) = a2bstr("red");
    hres = IHTMLBodyElement_put_bgColor(body, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLBodyElement_get_bgColor(body, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    /* Restore Originial */
    hres = IHTMLBodyElement_put_bgColor(body, vDefaultbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vDefaultbg);

    test_body_scroll(body, NULL);
    set_body_scroll(body, "yes");
    set_body_scroll(body, "no");
    set_body_scroll(body, "auto");
}

static void test_history(IHTMLWindow2 *window)
{
    IOmHistory *history, *history2;
    HRESULT hres;

    history = NULL;
    hres = IHTMLWindow2_get_history(window, &history);
    ok(hres == S_OK, "get_history failed: %08x\n", hres);
    ok(history != NULL, "history = NULL\n");

    test_disp2((IUnknown*)history, &DIID_DispHTMLHistory, &IID_IOmHistory, "[object]");

    history2 = NULL;
    hres = IHTMLWindow2_get_history(window, &history2);
    ok(hres == S_OK, "get_history failed: %08x\n", hres);
    ok(history2 != NULL, "history2 = NULL\n");
    ok(iface_cmp((IUnknown*)history, (IUnknown*)history2), "history != history2\n");

    IOmHistory_Release(history2);
    IOmHistory_Release(history);
}

static void test_xmlhttprequest(IHTMLWindow5 *window)
{
    HRESULT hres;
    VARIANT var;
    IHTMLXMLHttpRequestFactory *factory;
    IHTMLXMLHttpRequest *xml;

    hres = IHTMLWindow5_get_XMLHttpRequest(window, &var);
    ok(hres == S_OK, "get_XMLHttpRequest failed: %08x\n", hres);
    ok(V_VT(&var) == VT_DISPATCH, "expect VT_DISPATCH, got %s\n", debugstr_variant(&var));

    factory = NULL;
    hres = IDispatch_QueryInterface(V_DISPATCH(&var), &IID_IHTMLXMLHttpRequestFactory, (void**)&factory);
    ok(hres == S_OK, "QueryInterface(&IID_IHTMLXMLHttpRequestFactory) failed: %08x\n", hres);
    ok(factory != NULL, "factory == NULL\n");

    xml = NULL;
    hres = IHTMLXMLHttpRequestFactory_create(factory, &xml);
    ok(hres == S_OK, "create failed: %08x\n", hres);
    ok(xml != NULL, "xml == NULL\n");
    test_disp((IUnknown*)xml, &DIID_DispHTMLXMLHttpRequest, "[object]");

    IHTMLXMLHttpRequest_Release(xml);
    IHTMLXMLHttpRequestFactory_Release(factory);
    VariantClear(&var);
}

static void test_window(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window, *window2, *self, *parent;
    IHTMLWindow5 *window5;
    IHTMLDocument2 *doc2 = NULL;
    IDispatch *disp;
    IUnknown *unk;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);
    test_ifaces((IUnknown*)window, window_iids);
    hres = IHTMLWindow2_QueryInterface(window, &IID_ITravelLogClient, (void**)&unk);
    if(hres == S_OK)
        IUnknown_Release(unk);
    else
        win_skip("IID_ITravelLogClient not supported\n");

    test_disp((IUnknown*)window, &DIID_DispHTMLWindow2, "[object]");

    hres = IHTMLWindow2_get_document(window, &doc2);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);
    ok(doc2 != NULL, "doc2 == NULL\n");

    test_ifaces((IUnknown*)doc2, doc_node_iids);
    test_disp((IUnknown*)doc2, &DIID_DispHTMLDocument, "[object]");
    test_class_info((IUnknown*)doc2);

    test_ifaces((IUnknown*)doc, doc_obj_iids);
    test_disp((IUnknown*)doc, &DIID_DispHTMLDocument, "[object]");
    test_class_info((IUnknown*)doc);

    unk = (void*)0xdeadbeef;
    hres = IHTMLDocument2_QueryInterface(doc2, &IID_ICustomDoc, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_ICustomDoc) returned: %08x\n", hres);
    ok(!unk, "unk = %p\n", unk);

    IHTMLDocument2_Release(doc2);

    hres = IHTMLWindow2_get_window(window, &window2);
    ok(hres == S_OK, "get_window failed: %08x\n", hres);
    ok(window2 != NULL, "window2 == NULL\n");

    hres = IHTMLWindow2_get_self(window, &self);
    ok(hres == S_OK, "get_self failed: %08x\n", hres);
    ok(window2 != NULL, "self == NULL\n");

    ok(self == window2, "self != window2\n");

    IHTMLWindow2_Release(window2);

    disp = NULL;
    hres = IHTMLDocument2_get_Script(doc, &disp);
    ok(hres == S_OK, "get_Script failed: %08x\n", hres);
    ok(disp == (void*)window, "disp != window\n");
    IDispatch_Release(disp);

    hres = IHTMLWindow2_toString(window, NULL);
    ok(hres == E_INVALIDARG, "toString failed: %08x\n", hres);

    str = NULL;
    hres = IHTMLWindow2_toString(window, &str);
    ok(hres == S_OK, "toString failed: %08x\n", hres);
    ok(!strcmp_wa(str, "[object]") ||
       !strcmp_wa(str, "[object Window]") /* win7 ie9 */, "toString returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_VT(&v) = VT_ERROR;
    hres = IHTMLWindow2_get_opener(window, &v);
    ok(hres == S_OK, "get_opener failed: %08x\n", hres);
    ok(V_VT(&v) == VT_EMPTY, "V_VT(opener) = %d\n", V_VT(&v));

    parent = NULL;
    hres = IHTMLWindow2_get_parent(window, &parent);
    ok(hres == S_OK, "get_parent failed: %08x\n", hres);
    ok(parent != NULL, "parent == NULL\n");
    ok(parent == self, "parent != window\n");
    IHTMLWindow2_Release(parent);
    IHTMLWindow2_Release(self);

    test_window_name(window, NULL);
    set_window_name(window, "test");
    test_window_length(window, 0);
    test_screen(window);
    test_window_status(window);
    set_window_status(window, "Test!");
    test_history(window);

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLWindow5, (void**)&window5);
    if(SUCCEEDED(hres)) {
        ok(window5 != NULL, "window5 == NULL\n");
        test_xmlhttprequest(window5);
        IHTMLWindow5_Release(window5);
    }else {
        win_skip("IHTMLWindow5 not supported!\n");
    }

    IHTMLWindow2_Release(window);
}

static void test_dom_implementation(IHTMLDocument2 *doc)
{
    IHTMLDocument5 *doc5 = get_htmldoc5_iface((IUnknown*)doc);
    IHTMLDOMImplementation *dom_implementation;
    VARIANT_BOOL b;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    hres = IHTMLDocument5_get_implementation(doc5, &dom_implementation);
    IHTMLDocument5_Release(doc5);
    ok(hres == S_OK, "get_implementation failed: %08x\n", hres);
    ok(dom_implementation != NULL, "dom_implementation == NULL\n");

    str = a2bstr("test");
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("1.0");
    b = 100;
    hres = IHTMLDOMImplementation_hasFeature(dom_implementation, str, v, &b);
    SysFreeString(str);
    VariantClear(&v);
    ok(hres == S_OK, "hasFeature failed: %08x\n", hres);
    ok(!b, "hasFeature returned %x\n", b);

    IHTMLDOMImplementation_Release(dom_implementation);
}

static void test_defaults(IHTMLDocument2 *doc)
{
    IHTMLStyleSheetsCollection *stylesheetcol;
    IHTMLCurrentStyle *cstyle;
    IHTMLBodyElement *body;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    IHTMLStyle *style;
    VARIANT v;
    BSTR str;
    LONG l;
    VARIANT_BOOL b;
    HRESULT hres;
    IHTMLElementCollection *collection;

    elem = doc_get_body(doc);

    hres = IHTMLDocument2_get_images(doc, NULL);
    ok(hres == E_INVALIDARG, "hres %08x\n", hres);

    hres = IHTMLDocument2_get_images(doc, &collection);
    ok(hres == S_OK, "get_images failed: %08x\n", hres);
    if(hres == S_OK)
    {
        test_elem_collection((IUnknown*)collection, NULL, 0);
        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_applets(doc, NULL);
    ok(hres == E_INVALIDARG, "hres %08x\n", hres);

    hres = IHTMLDocument2_get_applets(doc, &collection);
    ok(hres == S_OK, "get_applets failed: %08x\n", hres);
    if(hres == S_OK)
    {
        test_elem_collection((IUnknown*)collection, NULL, 0);
        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_links(doc, NULL);
    ok(hres == E_INVALIDARG, "hres %08x\n", hres);

    hres = IHTMLDocument2_get_links(doc, &collection);
    ok(hres == S_OK, "get_links failed: %08x\n", hres);
    if(hres == S_OK)
    {
        test_elem_collection((IUnknown*)collection, NULL, 0);
        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_forms(doc, NULL);
    ok(hres == E_INVALIDARG, "hres %08x\n", hres);

    hres = IHTMLDocument2_get_forms(doc, &collection);
    ok(hres == S_OK, "get_forms failed: %08x\n", hres);
    if(hres == S_OK)
    {
        test_elem_collection((IUnknown*)collection, NULL, 0);
        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_anchors(doc, NULL);
    ok(hres == E_INVALIDARG, "hres %08x\n", hres);

    hres = IHTMLDocument2_get_anchors(doc, &collection);
    ok(hres == S_OK, "get_anchors failed: %08x\n", hres);
    if(hres == S_OK)
    {
        test_elem_collection((IUnknown*)collection, NULL, 0);
        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLBodyElement, (void**)&body);
    ok(hres == S_OK, "Could not get IHTMBodyElement: %08x\n", hres);
    test_default_body(body);
    test_body_funs(body);
    IHTMLBodyElement_Release(body);

    test_elem_istextedit(elem, VARIANT_TRUE);

    hres = IHTMLElement_get_style(elem, &style);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    test_disp((IUnknown*)style, &DIID_DispHTMLStyle, "[object]");
    test_ifaces((IUnknown*)style, style_iids);
    IHTMLStyle_Release(style);

    str = NULL;
    hres = IHTMLDocument2_get_charset(doc, &str);
    ok(hres == S_OK, "get_charset failed: %08x\n", hres);
    ok(str && *str, "charset is empty\n"); /* FIXME: better tests */
    SysFreeString(str);

    test_window(doc);
    test_compatmode(doc, "BackCompat");
    test_location(doc);
    test_navigator(doc);
    test_plugins_col(doc);

    elem2 = get_elem2_iface((IUnknown*)elem);
    hres = IHTMLElement2_get_currentStyle(elem2, &cstyle);
    ok(hres == S_OK, "get_currentStyle failed: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        IUnknown *unk;

        test_disp((IUnknown*)cstyle, &DIID_DispHTMLCurrentStyle, "[object]");
        test_ifaces((IUnknown*)cstyle, cstyle_iids);

        hres = IHTMLCurrentStyle_QueryInterface(cstyle, &IID_IHTMLCurrentStyle4, (void**)&unk);
        if(SUCCEEDED(hres))
            IUnknown_Release(unk);
        else
        {
           /*IE6 doesn't have interface */
           win_skip("IID_IHTMLCurrentStyle4 not supported\n");
        }

        IHTMLCurrentStyle_Release(cstyle);
    }
    IHTMLElement2_Release(elem2);

    IHTMLElement_Release(elem);

    hres = IHTMLDocument2_get_styleSheets(doc, &stylesheetcol);
    ok(hres == S_OK, "get_styleSheets failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyleSheetsCollection_get_length(stylesheetcol, &l);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(l == 0, "length = %d\n", l);

    IHTMLStyleSheetsCollection_Release(stylesheetcol);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLFiltersCollection, (void**)&body);
    ok(hres == E_NOINTERFACE, "got interface IHTMLFiltersCollection\n");

    str = a2bstr("xxx");
    b = 100;
    V_VT(&v) = VT_EMPTY;
    hres = IHTMLDocument2_execCommand(doc, str, FALSE, v, &b);
    ok(hres == OLECMDERR_E_NOTSUPPORTED || hres == E_INVALIDARG,
       "execCommand failed: %08x, expected OLECMDERR_E_NOTSUPPORTED or E_INVALIDARG\n", hres);
    SysFreeString(str);

    str = a2bstr("respectvisibilityindesign");
    b = 100;
    V_VT(&v) = VT_BOOL;
    V_BOOL(&v) = VARIANT_TRUE;
    hres = IHTMLDocument2_execCommand(doc, str, FALSE, v, &b);
    ok(hres == S_OK, "execCommand failed: %08x, expected DRAGDROP_E_NOTREGISTERED\n", hres);
    SysFreeString(str);

    test_default_selection(doc);
    test_doc_title(doc, "");
    test_dom_implementation(doc);
}

#define test_button_name(a,b) _test_button_name(__LINE__,a,b)
static void _test_button_name(unsigned line, IHTMLElement *elem, const char *exname)
{
    IHTMLButtonElement *button = _get_button_iface(line, (IUnknown*)elem);
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLButtonElement_get_name(button, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    if(exname)
        ok_(__FILE__,line)(!strcmp_wa(str, exname), "name = %s, expected %s\n", wine_dbgstr_w(str), exname);
    else
        ok_(__FILE__,line)(!str, "name = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
    IHTMLButtonElement_Release(button);
}

#define set_button_name(a,b) _set_button_name(__LINE__,a,b)
static void _set_button_name(unsigned line, IHTMLElement *elem, const char *name)
{
    IHTMLButtonElement *button = _get_button_iface(line, (IUnknown*)elem);
    BSTR str = a2bstr(name);
    HRESULT hres;

    hres = IHTMLButtonElement_put_name(button, str);
    ok_(__FILE__,line)(hres == S_OK, "get_name failed: %08x\n", hres);
    SysFreeString(str);
    IHTMLButtonElement_Release(button);

    _test_button_name(line, elem, name);
}

#define test_button_get_disabled(i,b) _test_button_get_disabled(__LINE__,i,b)
static void _test_button_get_disabled(unsigned line, IHTMLElement *elem, VARIANT_BOOL exb)
{
    IHTMLButtonElement *button = _get_button_iface(line, (IUnknown*)elem);
    VARIANT_BOOL disabled = 100;
    HRESULT hres;

    hres = IHTMLButtonElement_get_disabled(button, &disabled);
    ok_(__FILE__,line) (hres == S_OK, "get_disabled failed: %08x\n", hres);
    ok_(__FILE__,line) (disabled == exb, "disabled=%x, expected %x\n", disabled, exb);
    IHTMLButtonElement_Release(button);

    _test_elem3_get_disabled(line, (IUnknown*)elem, exb);
}

#define test_button_set_disabled(i,b) _test_button_set_disabled(__LINE__,i,b)
static void _test_button_set_disabled(unsigned line, IHTMLElement *elem, VARIANT_BOOL b)
{
    IHTMLButtonElement *button = _get_button_iface(line, (IUnknown*)elem);
    HRESULT hres;

    hres = IHTMLButtonElement_put_disabled(button, b);
    ok_(__FILE__,line) (hres == S_OK, "put_disabled failed: %08x\n", hres);
    IHTMLButtonElement_Release(button);

    _test_button_get_disabled(line, elem, b);
}

static void test_button_elem(IHTMLElement *elem)
{
    test_button_name(elem, NULL);
    set_button_name(elem, "button name");

    test_elem_istextedit(elem, VARIANT_TRUE);
}

#define test_tr_possess(e,r,l,i) _test_tr_possess(__LINE__,e,r,l,i)
static void _test_tr_possess(unsigned line, IHTMLElement *elem,
                            IHTMLTableRow *row, LONG len, const char *id)
{
    IHTMLElementCollection *col;
    IDispatch *disp;
    HRESULT hres;
    LONG lval;
    VARIANT var;

    hres = IHTMLTableRow_get_cells(row, &col);
    ok_(__FILE__, line)(hres == S_OK, "get_cells failed: %08x\n", hres);
    ok_(__FILE__, line)(col != NULL, "get_cells returned NULL\n");

    hres = IHTMLElementCollection_get_length(col, &lval);
    ok_(__FILE__, line)(hres == S_OK, "get length failed: %08x\n", hres);
    ok_(__FILE__, line)(lval == len, "expected len = %d, got %d\n", len, lval);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = a2bstr(id);
    hres = IHTMLElementCollection_tags(col, var, &disp);
    ok_(__FILE__, line)(hres == S_OK, "search by tags(%s) failed: %08x\n", id, hres);
    ok_(__FILE__, line)(disp != NULL, "disp == NULL\n");

    VariantClear(&var);
    IDispatch_Release(disp);
    IHTMLElementCollection_Release(col);
}

static void test_tr_modify(IHTMLElement *elem, IHTMLTableRow *row)
{
    HRESULT hres;
    IDispatch *disp;
    IHTMLTableCell *cell;

    hres = IHTMLTableRow_deleteCell(row, 0);
    ok(hres == S_OK, "deleteCell failed: %08x\n", hres);
    test_tr_possess(elem, row, 1, "td2");

    hres = IHTMLTableRow_insertCell(row, 0, &disp);
    ok(hres == S_OK, "insertCell failed: %08x\n", hres);
    ok(disp != NULL, "disp == NULL\n");
    hres = IDispatch_QueryInterface(disp, &IID_IHTMLTableCell, (void **)&cell);
    ok(hres == S_OK, "Could not get IID_IHTMLTableCell interface: %08x\n", hres);
    ok(cell != NULL, "cell == NULL\n");
    if (SUCCEEDED(hres))
        IHTMLTableCell_Release(cell);
    test_tr_possess(elem, row, 2, "td2");
    IDispatch_Release(disp);
}

static void test_tr_elem(IHTMLElement *elem)
{
    IHTMLElementCollection *col;
    IHTMLTableRow *row;
    HRESULT hres;
    BSTR bstr;
    LONG lval;
    VARIANT vbg, vDefaultbg;

    static const elem_type_t cell_types[] = {ET_TD,ET_TD};

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLTableRow, (void**)&row);
    ok(hres == S_OK, "Could not get IHTMLTableRow iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    col = NULL;
    hres = IHTMLTableRow_get_cells(row, &col);
    ok(hres == S_OK, "get_cells failed: %08x\n", hres);
    ok(col != NULL, "get_cells returned NULL\n");

    test_elem_collection((IUnknown*)col, cell_types, sizeof(cell_types)/sizeof(*cell_types));
    IHTMLElementCollection_Release(col);

    bstr = a2bstr("left");
    hres = IHTMLTableRow_put_align(row, bstr);
    ok(hres == S_OK, "set_align failed: %08x\n", hres);
    SysFreeString(bstr);

    bstr = NULL;
    hres = IHTMLTableRow_get_align(row, &bstr);
    ok(hres == S_OK, "get_align failed: %08x\n", hres);
    ok(bstr != NULL, "get_align returned NULL\n");
    ok(!strcmp_wa(bstr, "left"), "get_align returned %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    bstr = a2bstr("top");
    hres = IHTMLTableRow_put_vAlign(row, bstr);
    ok(hres == S_OK, "set_valign failed: %08x\n", hres);
    SysFreeString(bstr);

    bstr = NULL;
    hres = IHTMLTableRow_get_vAlign(row, &bstr);
    ok(hres == S_OK, "get_valign failed: %08x\n", hres);
    ok(bstr != NULL, "get_valign returned NULL\n");
    ok(!strcmp_wa(bstr, "top"), "get_valign returned %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    lval = 0xdeadbeef;
    hres = IHTMLTableRow_get_rowIndex(row, &lval);
    ok(hres == S_OK, "get_rowIndex failed: %08x\n", hres);
    ok(lval == 1, "get_rowIndex returned %d\n", lval);

    lval = 0xdeadbeef;
    hres = IHTMLTableRow_get_sectionRowIndex(row, &lval);
    ok(hres == S_OK, "get_sectionRowIndex failed: %08x\n", hres);
    ok(lval == 1, "get_sectionRowIndex returned %d\n", lval);

    hres = IHTMLTableRow_get_bgColor(row, &vDefaultbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vDefaultbg) == VT_BSTR, "bstr != NULL\n");
    ok(!V_BSTR(&vDefaultbg), "V_BSTR(bgColor) = %s\n", wine_dbgstr_w(V_BSTR(&vDefaultbg)));

    V_VT(&vbg) = VT_BSTR;
    V_BSTR(&vbg) = a2bstr("red");
    hres = IHTMLTableRow_put_bgColor(row, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLTableRow_get_bgColor(row, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    V_VT(&vbg) = VT_I4;
    V_I4(&vbg) = 0xff0000;
    hres = IHTMLTableRow_put_bgColor(row, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLTableRow_get_bgColor(row, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    /* Restore Originial */
    hres = IHTMLTableRow_put_bgColor(row, vDefaultbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vDefaultbg);

    test_tr_modify(elem, row);

    IHTMLTableRow_Release(row);
}

static void test_td_elem(IHTMLElement *elem)
{
    IHTMLTableCell *cell;
    HRESULT hres;
    LONG lval;
    BSTR str;
    VARIANT vbg, vDefaultbg;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLTableCell, (void**)&cell);
    ok(hres == S_OK, "Could not get IHTMLTableRow iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    lval = 0xdeadbeef;
    hres = IHTMLTableCell_get_cellIndex(cell, &lval);
    ok(hres == S_OK, "get cellIndex failed: %08x\n", hres);
    ok(lval == 1, "Expected 1, got %d\n", lval);

    str = a2bstr("left");
    hres = IHTMLTableCell_put_align(cell, str);
    ok(hres == S_OK, "put_align failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLTableCell_get_align(cell, &str);
    ok(hres == S_OK, "get_align failed: %08x\n", hres);
    ok(str != NULL, "str is NULL\n");
    if (str != NULL && hres == S_OK) {
        ok(!strcmp_wa(str, "left"), "got %s\n", wine_dbgstr_w(str));
        SysFreeString(str);
    }

    hres = IHTMLTableCell_get_bgColor(cell, &vDefaultbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vDefaultbg) == VT_BSTR, "bstr != NULL\n");
    ok(!V_BSTR(&vDefaultbg), "V_BSTR(bgColor) = %s\n", wine_dbgstr_w(V_BSTR(&vDefaultbg)));

    V_VT(&vbg) = VT_BSTR;
    V_BSTR(&vbg) = a2bstr("red");
    hres = IHTMLTableCell_put_bgColor(cell, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLTableCell_get_bgColor(cell, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    V_VT(&vbg) = VT_I4;
    V_I4(&vbg) = 0xff0000;
    hres = IHTMLTableCell_put_bgColor(cell, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLTableCell_get_bgColor(cell, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    /* Restore Originial */
    hres = IHTMLTableCell_put_bgColor(cell, vDefaultbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vDefaultbg);

    IHTMLTableCell_Release(cell);
}

static void test_label_elem(IHTMLElement *elem)
{
    IHTMLLabelElement *label;
    BSTR str;
    HRESULT hres;

    label = get_label_iface((IUnknown*)elem);

    str = NULL;
    hres = IHTMLLabelElement_get_htmlFor(label, &str);
    ok(hres == S_OK, "get_htmlFor failed: %08x\n", hres);
    ok(!strcmp_wa(str, "in"), "htmlFor = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("");
    hres = IHTMLLabelElement_put_htmlFor(label, str);
    ok(hres == S_OK, "put_htmlFor failed: %08x\n", hres);
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hres = IHTMLLabelElement_get_htmlFor(label, &str);
    ok(hres == S_OK, "get_htmlFor failed: %08x\n", hres);
    ok(!strcmp_wa(str, ""), "htmlFor = %s\n", wine_dbgstr_w(str));

    str = a2bstr("abc");
    hres = IHTMLLabelElement_put_htmlFor(label, str);
    ok(hres == S_OK, "put_htmlFor failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLLabelElement_get_htmlFor(label, &str);
    ok(hres == S_OK, "get_htmlFor failed: %08x\n", hres);
    ok(!strcmp_wa(str, "abc"), "htmlFor = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IHTMLLabelElement_Release(label);
}

#define test_table_cell_spacing(a,b) _test_table_cell_spacing(__LINE__,a,b)
static void _test_table_cell_spacing(unsigned line, IHTMLTable *table, const char *exstr)
{
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_ERROR;
    hres = IHTMLTable_get_cellSpacing(table, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_cellSpacing failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    if(exstr)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&v), exstr), "cellSpacing = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), exstr);
    else
        ok_(__FILE__,line)(!V_BSTR(&v), "cellSpacing = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

#define test_table_cell_padding(a,b) _test_table_cell_padding(__LINE__,a,b)
static void _test_table_cell_padding(unsigned line, IHTMLTable *table, const char *exstr)
{
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_ERROR;
    hres = IHTMLTable_get_cellPadding(table, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_cellPadding failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    if(exstr)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&v), exstr), "cellPadding = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), exstr);
    else
        ok_(__FILE__,line)(!V_BSTR(&v), "cellPadding = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

static void test_table_modify(IHTMLTable *table)
{
    IDispatch *disp;
    IHTMLTableRow *row;
    HRESULT hres;
    LONG index;

    test_table_length(table, 2);

    hres = IHTMLTable_insertRow(table, 0, &disp);
    ok(hres == S_OK, "insertRow failed: %08x\n", hres);
    ok(disp != NULL, "disp == NULL\n");
    test_table_length(table, 3);
    if (hres != S_OK || disp == NULL)
        return;

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLTableRow, (void**)&row);
    IDispatch_Release(disp);

    ok(hres == S_OK, "QueryInterface failed: %08x\n", hres);
    ok(row != NULL, "row == NULL\n");

    index = 0xdeadbeef;
    hres = IHTMLTableRow_get_rowIndex(row, &index);
    ok(hres == S_OK, "get_rowIndex failed: %08x\n", hres);
    ok(index == 0, "index = %d, expected 0\n", index);

    IHTMLTableRow_Release(row);

    hres = IHTMLTable_deleteRow(table, 0);
    ok(hres == S_OK, "deleteRow failed: %08x\n", hres);
    test_table_length(table, 2);
}

static void test_table_elem(IHTMLElement *elem)
{
    IHTMLElementCollection *col;
    IHTMLTable *table;
    IHTMLTable3 *table3;
    IHTMLDOMNode *node;
    VARIANT v;
    HRESULT hres;
    BSTR bstr;
    VARIANT vbg, vDefaultbg;

    static const elem_type_t row_types[] = {ET_TR,ET_TR};
    static const elem_type_t all_types[] = {ET_TBODY,ET_TR,ET_TR,ET_TD,ET_TD};
    static const elem_type_t tbodies_types[] = {ET_TBODY};

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLTable, (void**)&table);
    ok(hres == S_OK, "Could not get IHTMLTable iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLTable3, (void**)&table3);
    ok(hres == S_OK, "Could not get IHTMLTable3 iface: %08x\n", hres);
    if(FAILED(hres))
        return;

    col = NULL;
    hres = IHTMLTable_get_rows(table, &col);
    ok(hres == S_OK, "get_rows failed: %08x\n", hres);
    ok(col != NULL, "get_rows returned NULL\n");

    test_elem_collection((IUnknown*)col, row_types, sizeof(row_types)/sizeof(*row_types));
    IHTMLElementCollection_Release(col);

    test_elem_all((IUnknown*)table, all_types, sizeof(all_types)/sizeof(*all_types));

    node = clone_node((IUnknown*)table, VARIANT_TRUE);
    test_elem_tag((IUnknown*)node, "TABLE");
    test_elem_all((IUnknown*)node, all_types, sizeof(all_types)/sizeof(*all_types));
    IHTMLDOMNode_Release(node);

    node = clone_node((IUnknown*)table, VARIANT_FALSE);
    test_elem_tag((IUnknown*)node, "TABLE");
    test_elem_all((IUnknown*)node, NULL, 0);
    IHTMLDOMNode_Release(node);

    col = NULL;
    hres = IHTMLTable_get_tBodies(table, &col);
    ok(hres == S_OK, "get_tBodies failed: %08x\n", hres);
    ok(col != NULL, "get_tBodies returned NULL\n");

    test_elem_collection((IUnknown*)col, tbodies_types, sizeof(tbodies_types)/sizeof(*tbodies_types));
    IHTMLElementCollection_Release(col);

    test_table_cell_spacing(table, NULL);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 10;
    hres = IHTMLTable_put_cellSpacing(table, v);
    ok(hres == S_OK, "put_cellSpacing = %08x\n", hres);
    test_table_cell_spacing(table, "10");

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("11");
    hres = IHTMLTable_put_cellSpacing(table, v);
    ok(hres == S_OK, "put_cellSpacing = %08x\n", hres);
    test_table_cell_spacing(table, "11");
    VariantClear(&v);

    test_table_cell_padding(table, NULL);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 10;
    hres = IHTMLTable_put_cellPadding(table, v);
    ok(hres == S_OK, "put_cellPadding = %08x\n", hres);
    test_table_cell_padding(table, "10");

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("11");
    hres = IHTMLTable_put_cellPadding(table, v);
    ok(hres == S_OK, "put_cellPadding = %08x\n", hres);
    test_table_cell_padding(table, "11");
    VariantClear(&v);

    V_VT(&v) = VT_R8;
    V_R8(&v) = 5;
    hres = IHTMLTable_put_cellPadding(table, v);
    ok(hres == S_OK, "put_cellPadding = %08x\n", hres);
    test_table_cell_padding(table, "5");

    bstr = a2bstr("left");
    hres = IHTMLTable_put_align(table, bstr);
    ok(hres == S_OK, "set_align failed: %08x\n", hres);
    SysFreeString(bstr);

    bstr = NULL;
    hres = IHTMLTable_get_align(table, &bstr);
    ok(hres == S_OK, "get_align failed: %08x\n", hres);
    ok(bstr != NULL, "get_align returned NULL\n");
    ok(!strcmp_wa(bstr, "left"), "get_align returned %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    hres = IHTMLTable_get_bgColor(table, &vDefaultbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vDefaultbg) == VT_BSTR, "bstr != NULL\n");
    ok(!V_BSTR(&vDefaultbg), "V_BSTR(bgColor) = %s\n", wine_dbgstr_w(V_BSTR(&vDefaultbg)));

    V_VT(&vbg) = VT_BSTR;
    V_BSTR(&vbg) = a2bstr("red");
    hres = IHTMLTable_put_bgColor(table, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLTable_get_bgColor(table, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    V_VT(&vbg) = VT_I4;
    V_I4(&vbg) = 0xff0000;
    hres = IHTMLTable_put_bgColor(table, vbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vbg);

    hres = IHTMLTable_get_bgColor(table, &vbg);
    ok(hres == S_OK, "get_bgColor failed: %08x\n", hres);
    ok(V_VT(&vbg) == VT_BSTR, "V_VT(&vbg) != VT_BSTR\n");
    ok(!strcmp_wa(V_BSTR(&vbg), "#ff0000"), "Unexpected bgcolor %s\n", wine_dbgstr_w(V_BSTR(&vbg)));
    VariantClear(&vbg);

    /* Restore Originial */
    hres = IHTMLTable_put_bgColor(table, vDefaultbg);
    ok(hres == S_OK, "put_bgColor failed: %08x\n", hres);
    VariantClear(&vDefaultbg);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("11");
    hres = IHTMLTable_put_width(table, v);
    ok(hres == S_OK, "put_width = %08x\n", hres);
    VariantClear(&v);
    hres = IHTMLTable_get_width(table, &v);
    ok(hres == S_OK, "get_width = %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "11"), "Expected 11, got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("11.9");
    hres = IHTMLTable_put_width(table, v);
    ok(hres == S_OK, "put_width = %08x\n", hres);
    VariantClear(&v);
    hres = IHTMLTable_get_width(table, &v);
    ok(hres == S_OK, "get_width = %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "11"), "Expected 11, got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("40.2%");
    hres = IHTMLTable_put_width(table, v);
    ok(hres == S_OK, "put_width = %08x\n", hres);
    VariantClear(&v);
    hres = IHTMLTable_get_width(table, &v);
    ok(hres == S_OK, "get_width = %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "40.2%"), "Expected 40.2%%, got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 11;
    hres = IHTMLTable_put_width(table, v);
    ok(hres == S_OK, "put_width = %08x\n", hres);
    hres = IHTMLTable_get_width(table, &v);
    ok(hres == S_OK, "get_width = %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "11"), "Expected 11, got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_R8;
    V_R8(&v) = 11.9;
    hres = IHTMLTable_put_width(table, v);
    ok(hres == S_OK, "put_width = %08x\n", hres);
    hres = IHTMLTable_get_width(table, &v);
    ok(hres == S_OK, "get_width = %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "11"), "Expected 11, got %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    bstr = a2bstr("box");
    hres = IHTMLTable_put_frame(table, bstr);
    ok(hres == S_OK, "put_frame = %08x\n", hres);
    SysFreeString(bstr);
    hres = IHTMLTable_get_frame(table, &bstr);
    ok(hres == S_OK, "get_frame = %08x\n", hres);
    ok(!strcmp_wa(bstr, "box"), "Expected box, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

	test_table_modify(table);
    bstr = a2bstr("summary");
    hres = IHTMLTable3_put_summary(table3, bstr);
    ok(hres == S_OK, "put_summary = %08x\n", hres);
    SysFreeString(bstr);

    hres = IHTMLTable3_get_summary(table3, &bstr);
    ok(hres == S_OK, "get_summary = %08x\n", hres);
    ok(!strcmp_wa(bstr, "summary"), "Expected summary, got %s\n", wine_dbgstr_w(bstr));
    SysFreeString(bstr);

    IHTMLTable3_Release(table3);
    IHTMLTable_Release(table);
}

static void doc_write(IHTMLDocument2 *doc, BOOL ln, const char *text)
{
    SAFEARRAYBOUND dim;
    SAFEARRAY *sa;
    VARIANT *var;
    HRESULT hres;

    dim.lLbound = 0;
    dim.cElements = 1;
    sa = SafeArrayCreate(VT_VARIANT, 1, &dim);
    SafeArrayAccessData(sa, (void**)&var);
    V_VT(var) = VT_BSTR;
    V_BSTR(var) = a2bstr(text);
    SafeArrayUnaccessData(sa);

    if(ln)
        hres = IHTMLDocument2_writeln(doc, sa);
    else
        hres = IHTMLDocument2_write(doc, sa);
    ok(hres == S_OK, "write failed: %08x\n", hres);

    SafeArrayDestroy(sa);
}

static void doc_complex_write(IHTMLDocument2 *doc)
{
    SAFEARRAYBOUND dim = {5, 0};
    SAFEARRAY *sa;
    VARIANT *args;
    HRESULT hres;

    sa = SafeArrayCreate(VT_VARIANT, 1, &dim);
    SafeArrayAccessData(sa, (void**)&args);

    V_VT(args) = VT_BSTR;
    V_BSTR(args) = a2bstr("<body i4val=\"");
    V_VT(args+1) = VT_I4;
    V_I4(args+1) = 4;
    V_VT(args+2) = VT_BSTR;
    V_BSTR(args+2) = a2bstr("\" r8val=\"");
    V_VT(args+3) = VT_R8;
    V_R8(args+3) = 3.14;
    V_VT(args+4) = VT_BSTR;
    V_BSTR(args+4) = a2bstr("\">");
    SafeArrayUnaccessData(sa);

    hres = IHTMLDocument2_write(doc, sa);
    ok(hres == S_OK, "write failed: %08x\n", hres);

    SafeArrayDestroy(sa);
}

static void test_frame_doc(IUnknown *frame_elem, BOOL iframe)
{
    IHTMLDocument2 *window_doc, *elem_doc;
    IHTMLFrameElement3 *frame_elem3;
    IHTMLWindow2 *content_window;
    HRESULT hres;

    content_window = get_frame_content_window(frame_elem);
    test_ifaces((IUnknown*)content_window, window_iids);
    window_doc = get_window_doc(content_window);
    IHTMLWindow2_Release(content_window);

    elem_doc = get_elem_doc(frame_elem);
    ok(iface_cmp((IUnknown*)window_doc, (IUnknown*)elem_doc), "content_doc != elem_doc\n");

    if(!iframe) {
        hres = IUnknown_QueryInterface(frame_elem, &IID_IHTMLFrameElement3, (void**)&frame_elem3);
        if(SUCCEEDED(hres)) {
            IDispatch *disp = NULL;

            hres = IHTMLFrameElement3_get_contentDocument(frame_elem3, &disp);
            ok(hres == S_OK, "get_contentDocument failed: %08x\n", hres);
            ok(disp != NULL, "contentDocument == NULL\n");
            ok(iface_cmp((IUnknown*)disp, (IUnknown*)window_doc), "contentDocument != contentWindow.document\n");

            IDispatch_Release(disp);
            IHTMLFrameElement3_Release(frame_elem3);
        }else {
            win_skip("IHTMLFrameElement3 not supported\n");
        }
    }

    IHTMLDocument2_Release(elem_doc);
    IHTMLDocument2_Release(window_doc);
}

#define test_iframe_height(a,b) _test_iframe_height(__LINE__,a,b)
static void _test_iframe_height(unsigned line, IHTMLElement *elem, const char *exval)
{
    IHTMLIFrameElement2 *iframe = _get_iframe2_iface(line, (IUnknown*)elem);
    VARIANT v;
    HRESULT hres;

    hres = IHTMLIFrameElement2_get_height(iframe, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_height failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_BSTR, "V_VT(height) = %d\n", V_VT(&v));
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&v), exval), "height = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), exval);
    else
        ok_(__FILE__,line)(!V_BSTR(&v), "height = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IHTMLIFrameElement2_Release(iframe);
}

#define set_iframe_height(a,b) _set_iframe_height(__LINE__,a,b)
static void _set_iframe_height(unsigned line, IHTMLElement *elem, const char *val)
{
    IHTMLIFrameElement2 *iframe = _get_iframe2_iface(line, (IUnknown*)elem);
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr(val);
    hres = IHTMLIFrameElement2_put_height(iframe, v);
    ok_(__FILE__,line)(hres == S_OK, "put_height failed: %08x\n", hres);
    VariantClear(&v);
    IHTMLIFrameElement2_Release(iframe);
}

#define test_iframe_width(a,b) _test_iframe_width(__LINE__,a,b)
static void _test_iframe_width(unsigned line, IHTMLElement *elem, const char *exval)
{
    IHTMLIFrameElement2 *iframe = _get_iframe2_iface(line, (IUnknown*)elem);
    VARIANT v;
    HRESULT hres;

    hres = IHTMLIFrameElement2_get_width(iframe, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_width failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_BSTR, "V_VT(width) = %d\n", V_VT(&v));
    if(exval)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(&v), exval), "width = %s, expected %s\n", wine_dbgstr_w(V_BSTR(&v)), exval);
    else
        ok_(__FILE__,line)(!V_BSTR(&v), "width = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
    IHTMLIFrameElement2_Release(iframe);
}

#define set_iframe_width(a,b) _set_iframe_width(__LINE__,a,b)
static void _set_iframe_width(unsigned line, IHTMLElement *elem, const char *val)
{
    IHTMLIFrameElement2 *iframe = _get_iframe2_iface(line, (IUnknown*)elem);
    VARIANT v;
    HRESULT hres;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr(val);
    hres = IHTMLIFrameElement2_put_width(iframe, v);
    ok_(__FILE__,line)(hres == S_OK, "put_width failed: %08x\n", hres);
    VariantClear(&v);
    IHTMLIFrameElement2_Release(iframe);
}

static void test_iframe_elem(IHTMLElement *elem)
{
    IHTMLDocument2 *content_doc, *owner_doc;
    IHTMLIFrameElement3 *iframe3;
    IHTMLElementCollection *col;
    IHTMLWindow2 *content_window;
    IHTMLElement *body;
    IDispatch *disp;
    VARIANT errv;
    BSTR str;
    HRESULT hres;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_BODY,
        ET_BR
    };

    test_frame_doc((IUnknown*)elem, TRUE);
    test_framebase((IUnknown*)elem);

    content_window = get_frame_content_window((IUnknown*)elem);
    test_ifaces((IUnknown*)content_window, window_iids);
    test_window_length(content_window, 0);

    content_doc = get_window_doc(content_window);
    IHTMLWindow2_Release(content_window);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLIFrameElement3, (void**)&iframe3);
    if(SUCCEEDED(hres)) {
        hres = IHTMLIFrameElement3_get_contentDocument(iframe3, &disp);
        ok(hres == S_OK, "get_contentDocument failed: %08x\n", hres);
        ok(iface_cmp((IUnknown*)content_doc, (IUnknown*)disp), "content_doc != disp\n");
        IDispatch_Release(disp);

        IHTMLIFrameElement3_Release(iframe3);
    }else {
        win_skip("IHTMLIFrameElement3 not supported\n");
    }

    test_iframe_height(elem, NULL);
    set_iframe_height(elem, "100px");
    set_iframe_height(elem, "50%");
    test_iframe_height(elem, "50%");

    test_iframe_width(elem, NULL);
    set_iframe_width(elem, "150px");
    set_iframe_width(elem, "70%");
    test_iframe_width(elem, "70%");
    test_framebase_src(elem, "about:blank");

    str = a2bstr("text/html");
    V_VT(&errv) = VT_ERROR;
    disp = NULL;
    hres = IHTMLDocument2_open(content_doc, str, errv, errv, errv, &disp);
    SysFreeString(str);
    ok(hres == S_OK, "open failed: %08x\n", hres);
    ok(disp != NULL, "disp == NULL\n");
    ok(iface_cmp((IUnknown*)disp, (IUnknown*)content_window), "disp != content_window\n");
    IDispatch_Release(disp);

    doc_write(content_doc, FALSE, "<html><head><title>test</title></head>");
    doc_complex_write(content_doc);
    doc_write(content_doc, TRUE, "<br />");
    doc_write(content_doc, TRUE, "</html>");

    hres = IHTMLDocument2_get_all(content_doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    IHTMLElementCollection_Release(col);

    body = doc_get_body(content_doc);
    test_elem_attr(body, "i4val", "4");
    test_elem_attr(body, "r8val", "3.14");
    IHTMLElement_Release(body);

    hres = IHTMLDocument2_close(content_doc);
    ok(hres == S_OK, "close failed: %08x\n", hres);

    owner_doc = get_owner_doc((IUnknown*)content_doc);
    ok(!owner_doc, "owner_doc = %p\n", owner_doc);

    IHTMLDocument2_Release(content_doc);
}

#define test_stylesheet_csstext(a,b,c) _test_stylesheet_csstext(__LINE__,a,b,c)
static void _test_stylesheet_csstext(unsigned line, IHTMLStyleSheet *stylesheet, const char *exstr, BOOL is_todo)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLStyleSheet_get_cssText(stylesheet, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_cssText failed: %08x\n", hres);
    if(!is_todo) {
        if(exstr)
            ok_(__FILE__,line)(is_prefix_wa(str, exstr), "cssText = %s\n", wine_dbgstr_w(str));
        else
            ok_(__FILE__,line)(!str, "cssText = %s\n", wine_dbgstr_w(str));
    }else todo_wine {
        if(exstr)
            ok_(__FILE__,line)(is_prefix_wa(str, exstr), "cssText = %s\n", wine_dbgstr_w(str));
        else
            ok_(__FILE__,line)(!str, "cssText = %s\n", wine_dbgstr_w(str));
    }

    SysFreeString(str);
}

#define set_stylesheet_csstext(a,b,c) _set_stylesheet_csstext(__LINE__,a,b,c)
static void _set_stylesheet_csstext(unsigned line, IHTMLStyleSheet *stylesheet, const char *csstext, BOOL is_todo)
{
    BSTR str = a2bstr(csstext);
    HRESULT hres;

    hres = IHTMLStyleSheet_put_cssText(stylesheet, str);
    if(!is_todo)
        ok_(__FILE__,line)(hres == S_OK, "put_cssText failed: %08x\n", hres);
    else
        todo_wine ok_(__FILE__,line)(hres == S_OK, "put_cssText failed: %08x\n", hres);
    SysFreeString(str);
}

static void test_stylesheet(IDispatch *disp)
{
    IHTMLStyleSheetRulesCollection *col = NULL;
    IHTMLStyleSheet *stylesheet;
    HRESULT hres;
    BSTR href;

    test_disp2((IUnknown*)disp, &DIID_DispHTMLStyleSheet, &IID_IHTMLStyleSheet, "[object]");

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLStyleSheet, (void**)&stylesheet);
    ok(hres == S_OK, "Could not get IHTMLStyleSheet: %08x\n", hres);

    hres = IHTMLStyleSheet_get_rules(stylesheet, &col);
    ok(hres == S_OK, "get_rules failed: %08x\n", hres);
    ok(col != NULL, "col == NULL\n");

    test_disp2((IUnknown*)col, &DIID_DispHTMLStyleSheetRulesCollection, &IID_IHTMLStyleSheetRulesCollection, "[object]");
    IHTMLStyleSheetRulesCollection_Release(col);

    href = (void*)0xdeadbeef;
    hres = IHTMLStyleSheet_get_href(stylesheet, &href);
    ok(hres == S_OK, "get_href failed: %08x\n", hres);
    ok(href == NULL, "got href != NULL\n");
    SysFreeString(href);

    test_stylesheet_csstext(stylesheet, ".body {", FALSE);
    set_stylesheet_csstext(stylesheet, ".div { margin-right: 1px; }\n.body { margin-right: 2px; }", TRUE);
    test_stylesheet_csstext(stylesheet, ".div {", TRUE);
    set_stylesheet_csstext(stylesheet, "", FALSE);
    test_stylesheet_csstext(stylesheet, NULL, FALSE);
    set_stylesheet_csstext(stylesheet, ".div { margin-right: 1px; }", FALSE);
    test_stylesheet_csstext(stylesheet, ".div {", FALSE);

    IHTMLStyleSheet_Release(stylesheet);
}

static void test_stylesheets(IHTMLDocument2 *doc)
{
    IHTMLStyleSheetsCollection *col = NULL;
    VARIANT idx, res;
    LONG len = 0;
    HRESULT hres;

    hres = IHTMLDocument2_get_styleSheets(doc, &col);
    ok(hres == S_OK, "get_styleSheets failed: %08x\n", hres);
    ok(col != NULL, "col == NULL\n");

    test_disp2((IUnknown*)col, &DIID_DispHTMLStyleSheetsCollection, &IID_IHTMLStyleSheetsCollection, "[object]");

    hres = IHTMLStyleSheetsCollection_get_length(col, &len);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    ok(len == 1, "len=%d\n", len);

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
    LONG type;
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
    ok(type == 3, "type=%d\n", type);
    IHTMLDOMNode_Release(node);
    VariantClear(&var);

    bstr = SysAllocString(w100);
    hres = IDispatchEx_GetDispID(dispex, bstr, fdexNameCaseSensitive, &id);
    ok(hres == DISP_E_UNKNOWNNAME, "GetDispID failed: %08x, expected DISP_E_UNKNOWNNAME\n", hres);
    SysFreeString(bstr);

    IDispatchEx_Release(dispex);
}

static void test_enum_children(IUnknown *unk, unsigned len)
{
    IEnumVARIANT *enum_var;
    ULONG i, fetched;
    VARIANT v;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IEnumVARIANT, (void**)&enum_var);
    ok(hres == S_OK, "Could not get IEnumVARIANT iface: %08x\n", hres);

    for(i=0; i<len; i++) {
        fetched = 0;
        V_VT(&v) = VT_ERROR;
        hres = IEnumVARIANT_Next(enum_var, 1, &v, i ? &fetched : NULL);
        ok(hres == S_OK, "Next failed: %08x\n", hres);
        if(i)
            ok(fetched == 1, "fetched = %d\n", fetched);
        ok(V_VT(&v) == VT_DISPATCH && V_DISPATCH(&v), "V_VT(v) = %d\n", V_VT(&v));
        IDispatch_Release(V_DISPATCH(&v));
    }

    fetched = 0xdeadbeef;
    V_VT(&v) = VT_BOOL;
    hres = IEnumVARIANT_Next(enum_var, 1, &v, &fetched);
    ok(hres == S_FALSE, "Next returned %08x, expected S_FALSE\n", hres);
    ok(fetched == 0, "fetched = %d\n", fetched);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    hres = IEnumVARIANT_Reset(enum_var);
    ok(hres == S_OK, "Reset failed: %08x\n", hres);

    fetched = 0xdeadbeef;
    V_VT(&v) = VT_BOOL;
    hres = IEnumVARIANT_Next(enum_var, 0, &v, &fetched);
    ok(hres == S_OK, "Next returned %08x, expected S_FALSE\n", hres);
    ok(fetched == 0, "fetched = %d\n", fetched);
    ok(V_VT(&v) == VT_BOOL, "V_VT(v) = %d\n", V_VT(&v));

    hres = IEnumVARIANT_Skip(enum_var, len > 2 ? len-2 : 0);
    ok(hres == S_OK, "Skip failed: %08x\n", hres);

    hres = IEnumVARIANT_Reset(enum_var);
    ok(hres == S_OK, "Reset failed: %08x\n", hres);

    hres = IEnumVARIANT_Skip(enum_var, len+1);
    ok(hres == S_FALSE, "Skip failed: %08x\n", hres);

    IEnumVARIANT_Release(enum_var);
}

static void test_elems(IHTMLDocument2 *doc)
{
    IHTMLElementCollection *col;
    IHTMLDOMChildrenCollection *child_col;
    IHTMLElement *elem, *elem2, *elem3;
    IHTMLDOMNode *node, *node2;
    IHTMLWindow2 *window;
    IDispatch *disp;
    LONG type;
    HRESULT hres;
    IHTMLElementCollection *collection;
    IHTMLDocument3 *doc3;
    BSTR str;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_STYLE,
        ET_META,
        ET_LINK,
        ET_BODY,
        ET_COMMENT,
        ET_A,
        ET_LABEL,
        ET_INPUT,
        ET_BUTTON,
        ET_SELECT,
        ET_OPTION,
        ET_OPTION,
        ET_TEXTAREA,
        ET_TABLE,
        ET_TBODY,
        ET_TR,
        ET_TR,
        ET_TD,
        ET_TD,
        ET_SCRIPT,
        ET_TEST,
        ET_OBJECT,
        ET_EMBED,
        ET_IMG,
        ET_IFRAME,
        ET_FORM,
        ET_DIV
    };

    static const elem_type_t item_types[] = {
        ET_A,
        ET_OPTION,
        ET_TEXTAREA
    };

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    test_elem_col_item(col, "x", item_types, sizeof(item_types)/sizeof(item_types[0]));

    elem = get_elem_col_item_idx(col, 0);
    test_elem_source_index(elem, 0);
    IHTMLElement_Release(elem);

    elem = get_elem_col_item_idx(col, 3);
    test_elem_source_index(elem, 3);
    IHTMLElement_Release(elem);

    IHTMLElementCollection_Release(col);

    hres = IHTMLDocument2_get_images(doc, &collection);
    ok(hres == S_OK, "get_images failed: %08x\n", hres);
    if(hres == S_OK)
    {
        static const elem_type_t images_types[] = {ET_IMG};
        test_elem_collection((IUnknown*)collection, images_types, 1);

        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_links(doc, &collection);
    ok(hres == S_OK, "get_links failed: %08x\n", hres);
    if(hres == S_OK)
    {
        static const elem_type_t images_types[] = {ET_A};
        test_elem_collection((IUnknown*)collection, images_types, 1);

        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_anchors(doc, &collection);
    ok(hres == S_OK, "get_anchors failed: %08x\n", hres);
    if(hres == S_OK)
    {
        static const elem_type_t anchor_types[] = {ET_A};
        test_elem_collection((IUnknown*)collection, anchor_types, 1);

        IHTMLElementCollection_Release(collection);
    }

    hres = IHTMLDocument2_get_scripts(doc, &collection);
    ok(hres == S_OK, "get_scripts failed: %08x\n", hres);
    if(hres == S_OK) {
        static const elem_type_t script_types[] = {ET_SCRIPT};
        test_elem_collection((IUnknown*)collection, script_types, 1);
        IHTMLElementCollection_Release(collection);
    }

    test_plugins_col(doc);

    elem = get_doc_elem(doc);
    test_elem_istextedit(elem, VARIANT_FALSE);
    test_elem_all((IUnknown*)elem, all_types+1, sizeof(all_types)/sizeof(all_types[0])-1);
    IHTMLElement_Release(elem);

    get_elem_by_id(doc, "xxx", FALSE);
    elem = get_doc_elem_by_id(doc, "xxx");
    ok(!elem, "elem != NULL\n");

    elem = get_doc_elem_by_id(doc, "s");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_elem_type((IUnknown*)elem, ET_SELECT);
        test_elem_attr(elem, "xxx", NULL);
        test_elem_attr(elem, "id", "s");
        test_elem_class((IUnknown*)elem, NULL);
        test_elem_set_class((IUnknown*)elem, "cl");
        test_elem_set_class((IUnknown*)elem, NULL);
        test_elem_tabindex((IUnknown*)elem, 0);
        test_elem_set_tabindex((IUnknown*)elem, 1);
        test_elem_filters((IUnknown*)elem);
        test_elem_istextedit(elem, VARIANT_FALSE);

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
        if (node)
        {
            test_node_name((IUnknown*)node, "#document");
            type = get_node_type((IUnknown*)node);
            ok(type == 9, "type=%d, expected 9\n", type);
            node2 = test_node_get_parent((IUnknown*)node);
            IHTMLDOMNode_Release(node);
            ok(node2 == NULL, "node != NULL\n");
        }

        elem2 = test_elem_get_parent((IUnknown*)elem);
        ok(elem2 != NULL, "elem2 == NULL\n");
        test_node_name((IUnknown*)elem2, "BODY");

        elem3 = test_elem_get_parent((IUnknown*)elem2);
        ok(elem3 != NULL, "elem3 == NULL\n");
        test_node_name((IUnknown*)elem3, "HTML");

        test_elem_contains(elem3, elem2, VARIANT_TRUE);
        test_elem_contains(elem3, elem, VARIANT_TRUE);
        test_elem_contains(elem2, elem, VARIANT_TRUE);
        test_elem_contains(elem2, elem3, VARIANT_FALSE);
        test_elem_contains(elem, elem3, VARIANT_FALSE);
        test_elem_contains(elem, elem2, VARIANT_FALSE);
        test_elem_contains(elem, elem, VARIANT_TRUE);
        test_elem_contains(elem, NULL, VARIANT_FALSE);
        IHTMLElement_Release(elem2);

        elem2 = test_elem_get_parent((IUnknown*)elem3);
        ok(elem2 == NULL, "elem2 != NULL\n");
        test_elem_source_index(elem3, 0);
        IHTMLElement_Release(elem3);

        test_elem_getelembytag((IUnknown*)elem, ET_OPTION, 2, NULL);
        test_elem_getelembytag((IUnknown*)elem, ET_SELECT, 0, NULL);
        test_elem_getelembytag((IUnknown*)elem, ET_HTML, 0, NULL);

        test_elem_innertext(elem, "opt1opt2");

        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, "s", TRUE);
    if(elem) {
        IHTMLSelectElement *select = get_select_iface((IUnknown*)elem);
        IHTMLDocument2 *doc_node, *elem_doc;

        test_select_elem(select);

        test_elem_istextedit(elem, VARIANT_FALSE);
        test_elem_title((IUnknown*)select, NULL);
        test_elem_set_title((IUnknown*)select, "Title");
        test_elem_title((IUnknown*)select, "Title");
        test_elem_offset((IUnknown*)select, "BODY");
        test_elem_bounding_client_rect((IUnknown*)select);

        node = get_first_child((IUnknown*)select);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            test_elem_type((IUnknown*)node, ET_OPTION);
            IHTMLDOMNode_Release(node);
        }

        type = get_node_type((IUnknown*)select);
        ok(type == 1, "type=%d\n", type);

        IHTMLSelectElement_Release(select);

        elem_doc = get_elem_doc((IUnknown*)elem);

        doc_node = get_doc_node(doc);
        ok(iface_cmp((IUnknown*)elem_doc, (IUnknown*)doc_node), "disp != doc\n");
        IHTMLDocument2_Release(doc_node);
        IHTMLDocument2_Release(elem_doc);

        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, "sc", TRUE);
    if(elem) {
        IHTMLScriptElement *script;
        BSTR type;

        hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLScriptElement, (void**)&script);
        ok(hres == S_OK, "Could not get IHTMLScriptElement interface: %08x\n", hres);

        test_elem_language(elem, NULL);
        test_elem_istextedit(elem, VARIANT_FALSE);

        if(hres == S_OK)
        {
            VARIANT_BOOL vb;

            hres = IHTMLScriptElement_put_type (script, NULL);
            ok(hres == S_OK, "put_type failed: %08x\n", hres);
            hres = IHTMLScriptElement_get_type(script, &type);
            ok(hres == S_OK, "get_type failed: %08x\n", hres);
            ok(type == NULL, "Unexpected type %s\n", wine_dbgstr_w(type));

            type = a2bstr("text/javascript");
            hres = IHTMLScriptElement_put_type (script, type);
            ok(hres == S_OK, "put_type failed: %08x\n", hres);
            SysFreeString(type);
            hres = IHTMLScriptElement_get_type(script, &type);
            ok(hres == S_OK, "get_type failed: %08x\n", hres);
            ok(!strcmp_wa(type, "text/javascript"), "Unexpected type %s\n", wine_dbgstr_w(type));
            SysFreeString(type);

            test_script_text(script, "<!--\nfunction Testing() {}\n// -->\n");

            /* test defer */
            hres = IHTMLScriptElement_put_defer(script, VARIANT_TRUE);
            ok(hres == S_OK, "put_defer failed: %08x\n", hres);

            hres = IHTMLScriptElement_get_defer(script, &vb);
            ok(hres == S_OK, "get_defer failed: %08x\n", hres);
            ok(vb == VARIANT_TRUE, "get_defer result is %08x\n", hres);

            hres = IHTMLScriptElement_put_defer(script, VARIANT_FALSE);
            ok(hres == S_OK, "put_defer failed: %08x\n", hres);

            str = (BSTR)0xdeadbeef;
            hres = IHTMLScriptElement_get_src(script, &str);
            ok(hres == S_OK, "get_src failed: %08x\n", hres);
            ok(!str, "src = %s\n", wine_dbgstr_w(str));
        }

        IHTMLScriptElement_Release(script);

        set_elem_language(elem, "vbscript");
        set_elem_language(elem, "xxx");
    }

    elem = get_elem_by_id(doc, "in", TRUE);
    if(elem) {
        IHTMLInputElement *input;

        hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLInputElement, (void**)&input);
        ok(hres == S_OK, "Could not get IHTMLInputElement: %08x\n", hres);

        test_elem_id((IUnknown*)elem, "in");
        test_elem_put_id((IUnknown*)elem, "newin");
        test_input_get_disabled(input, VARIANT_FALSE);
        test_input_set_disabled(input, VARIANT_TRUE);
        test_input_set_disabled(input, VARIANT_FALSE);
        test_elem3_set_disabled((IUnknown*)input, VARIANT_TRUE);
        test_input_get_disabled(input, VARIANT_TRUE);
        test_elem3_set_disabled((IUnknown*)input, VARIANT_FALSE);
        test_input_get_disabled(input, VARIANT_FALSE);
        test_elem_client_size((IUnknown*)elem);
        test_input_type(input, "text");
        test_elem_istextedit(elem, VARIANT_TRUE);

        test_node_get_value_str((IUnknown*)elem, NULL);
        test_node_put_value_str((IUnknown*)elem, "test");
        test_node_get_value_str((IUnknown*)elem, NULL);
        test_input_value((IUnknown*)elem, NULL);
        test_input_defaultValue((IUnknown*)elem, NULL);
        test_input_put_value((IUnknown*)elem, "test");
        test_input_defaultValue((IUnknown*)elem, NULL);
        test_elem_class((IUnknown*)elem, "testclass");
        test_elem_tabindex((IUnknown*)elem, 2);
        test_elem_set_tabindex((IUnknown*)elem, 3);
        test_elem_title((IUnknown*)elem, "test title");

        test_input_get_defaultchecked(input, VARIANT_FALSE);
        test_input_set_defaultchecked(input, VARIANT_TRUE);
        test_input_set_defaultchecked(input, VARIANT_FALSE);

        test_input_get_checked(input, VARIANT_FALSE);
        test_input_set_checked(input, VARIANT_TRUE);
        test_input_set_checked(input, VARIANT_FALSE);

        test_input_maxlength(input, 0x7fffffff);
        test_input_set_maxlength(input, 30);

        test_input_name(input, NULL);
        test_input_set_name(input, "test");

        test_input_src(input, NULL);
        test_input_set_src(input, "about:blank");

        test_input_set_size(input, 15, S_OK);
        test_input_get_size(input, 15);
        test_input_set_size(input, -100, CTL_E_INVALIDPROPERTYVALUE);
        test_input_get_size(input, 15);
        test_input_set_size(input, 0, CTL_E_INVALIDPROPERTYVALUE);
        test_input_get_size(input, 15);

        test_input_readOnly(input, VARIANT_TRUE);
        test_input_readOnly(input, VARIANT_FALSE);

        IHTMLInputElement_Release(input);
        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, "imgid", TRUE);
    if(elem) {
        test_img_align((IUnknown*)elem, "left");
        test_img_name((IUnknown*)elem, "WineImg");
        test_img_src((IUnknown*)elem, "", NULL);
        test_img_set_src((IUnknown*)elem, "about:blank");
        test_img_src((IUnknown*)elem, "about:blank", NULL);
        test_img_alt((IUnknown*)elem, NULL);
        test_img_set_alt((IUnknown*)elem, "alt test");
        test_img_name((IUnknown*)elem, "WineImg");
        test_img_complete(elem, VARIANT_FALSE);
        test_img_isMap((IUnknown*)elem, VARIANT_TRUE);
        test_img_isMap((IUnknown*)elem, VARIANT_FALSE);
        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, "attr", TRUE);
    if(elem) {
        test_dynamic_properties(elem);
        test_attr_collection(elem);
        test_contenteditable((IUnknown*)elem);
        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, "styleid", TRUE);
    if(elem) {
        test_style_media((IUnknown*)elem, NULL);
        test_style_put_media((IUnknown*)elem, "screen");
        test_style_type((IUnknown*)elem, NULL);
        test_style_put_type((IUnknown*)elem, "text/css");
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "tbl");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_table_elem(elem);
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "labelid");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_label_elem(elem);
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "td2");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_td_elem(elem);
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "row2");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_tr_elem(elem);
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "ifr");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_iframe_elem(elem);
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "btnid");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_button_elem(elem);
        test_button_get_disabled(elem, VARIANT_FALSE);
        test_button_set_disabled(elem, VARIANT_TRUE);
        test_elem3_set_disabled((IUnknown*)elem, VARIANT_FALSE);
        test_button_get_disabled(elem, VARIANT_FALSE);
        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "objid");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_object_vspace((IUnknown*)elem, 100);
        test_object_name(elem, "objname");
        set_object_name(elem, "test");
        set_object_name(elem, NULL);
        IHTMLElement_Release(elem);
    }

    elem = get_elem_by_id(doc, "a", TRUE);
    if(elem) {
        test_anchor_href((IUnknown*)elem, "http://test/");

        /* Change the href */
        test_anchor_put_href((IUnknown*)elem, "http://test1/");
        test_anchor_href((IUnknown*)elem, "http://test1/");
        test_anchor_hostname((IUnknown*)elem, "test1");

        /* Restore the href */
        test_anchor_put_href((IUnknown*)elem, "http://test/");
        test_anchor_href((IUnknown*)elem, "http://test/");
        test_anchor_hostname((IUnknown*)elem, "test");
        test_anchor_hash(elem, NULL);

        /* target */
        test_anchor_get_target((IUnknown*)elem, NULL);

        test_anchor_rel((IUnknown*)elem, NULL);
        test_anchor_put_rel((IUnknown*)elem, "Next");
        test_anchor_rel((IUnknown*)elem, "Next");

        /* Change the target */
        test_anchor_put_target((IUnknown*)elem, "wine");
        test_anchor_get_target((IUnknown*)elem, "wine");

        /* Restore the target */
        test_anchor_put_target((IUnknown*)elem, NULL);
        test_anchor_get_target((IUnknown*)elem, NULL);

        test_anchor_name((IUnknown*)elem, "x");
        test_anchor_put_name((IUnknown*)elem, "anchor name");
        test_anchor_put_name((IUnknown*)elem, NULL);
        test_anchor_put_name((IUnknown*)elem, "x");

        test_anchor_put_href((IUnknown*)elem, "http://test/?how#hash");
        test_anchor_hash(elem, "#hash");
        test_anchor_search((IUnknown*)elem, "?how", FALSE);

        test_anchor_put_search((IUnknown*)elem, "?word=press");
        test_anchor_search((IUnknown*)elem, "?word=press", FALSE);
        test_anchor_put_search((IUnknown*)elem, "?????word???press");
        test_anchor_search((IUnknown*)elem, "?????word???press", FALSE);

        test_anchor_put_search((IUnknown*)elem, "?q=%E4%BD%A0%E5%A5%BD"); /* encoded cjk characters */
        test_anchor_search((IUnknown*)elem, "?q=%E4%BD%A0%E5%A5%BD", FALSE);

        test_anchor_put_search((IUnknown*)elem, "?how?old=are");
        test_anchor_search((IUnknown*)elem, "?how?old=are", FALSE);

        /* due to incorrect behavior of ie6, search string without leading "?" is interpreted
        as part of the pathname, and cannot be accessed by get_search. */
        test_anchor_put_search((IUnknown*)elem, "word=abc");
        test_anchor_search((IUnknown*)elem, "?word=abc", TRUE);

        IHTMLElement_Release(elem);
    }

    elem = get_doc_elem_by_id(doc, "metaid");
    if(elem) {
        test_meta_name((IUnknown*)elem, "meta name");
        test_meta_content((IUnknown*)elem, "text/html; charset=utf-8");
        test_meta_httpequiv((IUnknown*)elem, "Content-Type");
        test_meta_charset((IUnknown*)elem, NULL);
        set_meta_charset((IUnknown*)elem, "utf-8");
        IHTMLElement_Release(elem);
    }

    elem = doc_get_body(doc);

    node = get_first_child((IUnknown*)elem);
    ok(node != NULL, "node == NULL\n");
    if(node) {
        test_ifaces((IUnknown*)node, text_iids);
        test_disp((IUnknown*)node, &DIID_DispHTMLDOMTextNode, "[object]");

        node2 = get_first_child((IUnknown*)node);
        ok(!node2, "node2 != NULL\n");

        type = get_node_type((IUnknown*)node);
        ok(type == 3, "type=%d\n", type);

        test_node_get_value_str((IUnknown*)node, "text test");
        test_node_put_value_str((IUnknown*)elem, "test text");
        test_node_get_value_str((IUnknown*)node, "text test");

        hres = IHTMLDOMNode_get_attributes(node, &disp);
        ok(hres == S_OK, "get_attributes failed: %08x\n", hres);
        ok(!disp, "disp != NULL\n");

        IHTMLDOMNode_Release(node);
    }

    child_col = get_child_nodes((IUnknown*)elem);
    ok(child_col != NULL, "child_coll == NULL\n");
    if(child_col) {
        IUnknown *enum_unk;
        LONG length = 0;

        test_disp((IUnknown*)child_col, &DIID_DispDOMChildrenCollection, "[object]");

        hres = IHTMLDOMChildrenCollection_get_length(child_col, &length);
        ok(hres == S_OK, "get_length failed: %08x\n", hres);
        ok(length, "length=0\n");

        node2 = NULL;
        node = get_child_item(child_col, 0);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            IHTMLDOMNode *prev;

            type = get_node_type((IUnknown*)node);
            ok(type == 3, "type=%d\n", type);
            node2 = node_get_next((IUnknown*)node);

            prev = node_get_prev((IUnknown*)node2);
            ok(iface_cmp((IUnknown*)node, (IUnknown*)prev), "node != prev\n");
            IHTMLDOMNode_Release(prev);

            IHTMLDOMNode_Release(node);
        }

        node = get_child_item(child_col, 1);
        ok(node != NULL, "node == NULL\n");
        if(node) {
            type = get_node_type((IUnknown*)node);
            ok(type == 8, "type=%d\n", type);

            test_elem_id((IUnknown*)node, NULL);
            ok(iface_cmp((IUnknown*)node2, (IUnknown*)node), "node2 != node\n");
            IHTMLDOMNode_Release(node2);
            IHTMLDOMNode_Release(node);
        }

        hres = IHTMLDOMChildrenCollection_item(child_col, length - 1, NULL);
        ok(hres == E_POINTER, "item failed: %08x, expected E_POINTER\n", hres);

        hres = IHTMLDOMChildrenCollection_item(child_col, length, NULL);
        ok(hres == E_POINTER, "item failed: %08x, expected E_POINTER\n", hres);

        hres = IHTMLDOMChildrenCollection_item(child_col, 6000, &disp);
        ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);

        hres = IHTMLDOMChildrenCollection_item(child_col, length, &disp);
        ok(hres == E_INVALIDARG, "item failed: %08x, expected E_INVALIDARG\n", hres);

        test_child_col_disp(child_col);

        hres = IHTMLDOMChildrenCollection_get__newEnum(child_col, &enum_unk);
        ok(hres == S_OK, "get__newEnum failed: %08x\n", hres);

        test_enum_children(enum_unk, length);

        IUnknown_Release(enum_unk);

        IHTMLDOMChildrenCollection_Release(child_col);
    }

    test_elem3_get_disabled((IUnknown*)elem, VARIANT_FALSE);
    test_elem3_set_disabled((IUnknown*)elem, VARIANT_TRUE);
    test_elem3_set_disabled((IUnknown*)elem, VARIANT_FALSE);

    IHTMLElement_Release(elem);

    elem = get_doc_elem_by_id(doc, "frm");
    ok(elem != NULL, "elem == NULL\n");
    if(elem) {
        test_form_length((IUnknown*)elem, 0);
        test_form_elements((IUnknown*)elem);
        IHTMLElement_Release(elem);
    }

    test_stylesheets(doc);
    test_create_option_elem(doc);
    test_create_img_elem(doc);

    elem = get_doc_elem_by_id(doc, "tbl");
    ok(elem != NULL, "elem = NULL\n");
    test_elem_set_innertext(elem, "inner text");
    IHTMLElement_Release(elem);

    test_doc_title(doc, "test");
    test_doc_set_title(doc, "test title");
    test_doc_title(doc, "test title");

    disp = NULL;
    hres = IHTMLDocument2_get_Script(doc, &disp);
    ok(hres == S_OK, "get_Script failed: %08x\n", hres);
    if(hres == S_OK)
    {
        IDispatchEx *dispex;
        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        ok(hres == S_OK, "IDispatch_QueryInterface failed: %08x\n", hres);
        if(hres == S_OK)
        {
            DISPID pid = -1;
            BSTR str = a2bstr("Testing");
            hres = IDispatchEx_GetDispID(dispex, str, 1, &pid);
            ok(hres == S_OK, "GetDispID failed: %08x\n", hres);
            ok(pid != -1, "pid == -1\n");
            SysFreeString(str);
            IDispatchEx_Release(dispex);
        }
    }
    IDispatch_Release(disp);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);

    str = a2bstr("Img");
    hres = IHTMLDocument3_getElementsByTagName(doc3, str, &col);
    ok(hres == S_OK, "getElementsByTagName(%s) failed: %08x\n", wine_dbgstr_w(str), hres);
    SysFreeString(str);
    if(hres == S_OK)
    {
        static const elem_type_t img_types[] = { ET_IMG };

        test_elem_collection((IUnknown*)col, img_types, sizeof(img_types)/sizeof(img_types[0]));
        IHTMLElementCollection_Release(col);
    }

    elem = get_doc_elem_by_id(doc, "y");
    test_elem_set_innerhtml((IUnknown*)elem, "inner html");
    test_elem_innerhtml((IUnknown*)elem, "inner html");
    test_elem_set_innerhtml((IUnknown*)elem, "");
    test_elem_innerhtml((IUnknown*)elem, NULL);
    node = node_get_next((IUnknown*)elem);
    ok(!node, "node = %p\n", node);

    elem2 = get_doc_elem_by_id(doc, "x");
    test_elem_tag((IUnknown*)elem2, "A");
    node = node_get_next((IUnknown*)elem2);
    IHTMLDOMNode_Release(node);
    IHTMLElement_Release(elem2);
    IHTMLElement_Release(elem);

    hres = IHTMLDocument3_recalc(doc3, VARIANT_TRUE);
    ok(hres == S_OK, "recalc failed: %08x\n", hres);

    IHTMLDocument3_Release(doc3);

    elem = get_elem_by_id(doc, "s", TRUE);
    if(elem) {
        static const elem_type_t select_types[] = { ET_OPTION, ET_OPTION, ET_OPTION };

        test_select_put_length((IUnknown*)elem, 3);
        test_elem_all((IUnknown*)elem, select_types, sizeof(select_types)/sizeof(*select_types));
        test_select_put_length((IUnknown*)elem, 1);
        test_elem_all((IUnknown*)elem, select_types, 1);
        IHTMLElement_Release(elem);
    }

    window = get_doc_window(doc);
    test_window_name(window, NULL);
    set_window_name(window, "test name");
    test_window_length(window, 1);
    IHTMLWindow2_Release(window);
}

static void test_attr(IHTMLElement *elem)
{
    IHTMLDOMAttribute *attr, *attr2;
    VARIANT v;

    get_elem_attr_node((IUnknown*)elem, "noattr", FALSE);

    attr = get_elem_attr_node((IUnknown*)elem, "id", TRUE);

    test_disp((IUnknown*)attr, &DIID_DispHTMLDOMAttribute, "[object]");
    test_ifaces((IUnknown*)attr, attr_iids);
    test_no_iface((IUnknown*)attr, &IID_IHTMLDOMNode);
    test_attr_specified(attr, VARIANT_TRUE);

    attr2 = get_elem_attr_node((IUnknown*)elem, "id", TRUE);
    ok(iface_cmp((IUnknown*)attr, (IUnknown*)attr2), "attr != attr2\n");
    IHTMLDOMAttribute_Release(attr2);

    get_attr_node_value(attr, &v, VT_BSTR);
    ok(!strcmp_wa(V_BSTR(&v), "divid"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("divid2");
    put_attr_node_value(attr, v);

    get_attr_node_value(attr, &v, VT_BSTR);
    ok(!strcmp_wa(V_BSTR(&v), "divid2"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    IHTMLDOMAttribute_Release(attr);

    attr = get_elem_attr_node((IUnknown*)elem, "emptyattr", TRUE);
    get_attr_node_value(attr, &v, VT_BSTR);
    ok(!V_BSTR(&v), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("newvalue");
    put_attr_node_value(attr, v);
    VariantClear(&v);

    attr = get_elem_attr_node((IUnknown*)elem, "emptyattr", TRUE);
    get_attr_node_value(attr, &v, VT_BSTR);
    ok(!strcmp_wa(V_BSTR(&v), "newvalue"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    test_attr_specified(attr, VARIANT_TRUE);
    IHTMLDOMAttribute_Release(attr);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 100;
    set_dispex_value((IUnknown*)elem, "dispprop", &v);
    attr = get_elem_attr_node((IUnknown*)elem, "dispprop", TRUE);
    get_attr_node_value(attr, &v, VT_I4);
    ok(V_I4(&v) == 100, "V_I4(v) = %d\n", V_I4(&v));
    test_attr_specified(attr, VARIANT_TRUE);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 150;
    put_attr_node_value(attr, v);

    get_attr_node_value(attr, &v, VT_I4);
    ok(V_I4(&v) == 150, "V_I4(v) = %d\n", V_I4(&v));

    IHTMLDOMAttribute_Release(attr);

    attr = get_elem_attr_node((IUnknown*)elem, "tabIndex", TRUE);
    test_attr_specified(attr, VARIANT_FALSE);
    test_attr_expando(attr, VARIANT_FALSE);
    IHTMLDOMAttribute_Release(attr);
}

static void test_blocked(IHTMLDocument2 *doc, IHTMLElement *outer_elem)
{
    IHTMLElement *elem;

    test_elem_set_innerhtml((IUnknown*)outer_elem,
            "<img id=\"imgid\" src=\"BLOCKED::http://www.winehq.org/img.png\" />");
    elem = get_elem_by_id(doc, "imgid", TRUE);
    if(elem) {
        test_img_src((IUnknown*)elem, "BLOCKED::", "blocked::http://www.winehq.org/img.png");
        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)outer_elem,
            "<img id=\"imgid\" src=\"BLOCKE::http://www.winehq.org/img.png\" />");
    elem = get_elem_by_id(doc, "imgid", TRUE);
    if(elem) {
        test_img_src((IUnknown*)elem, "blocke::http://www.winehq.org/img.png", NULL);
        test_img_set_src((IUnknown*)elem, "BLOCKED:http://www.winehq.org/img.png");
        test_img_src((IUnknown*)elem, "blocked:http://www.winehq.org/img.png", NULL);
        test_img_set_src((IUnknown*)elem, "blocked::http://www.winehq.org/img.png");
        test_img_src((IUnknown*)elem, "BLOCKED::", "blocked::http://www.winehq.org/img.png");
        IHTMLElement_Release(elem);
    }
}

#define doc_get_elems_by_name(a,b) _doc_get_elems_by_name(__LINE__,a,b)
static IHTMLElementCollection *_doc_get_elems_by_name(unsigned line, IHTMLDocument2 *doc, const char *name)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, doc);
    IHTMLElementCollection *col;
    BSTR str = a2bstr(name);
    HRESULT hres;

    hres = IHTMLDocument3_getElementsByName(doc3, str, &col);
    ok_(__FILE__,line)(hres == S_OK, "getElementsByName failed: %08x\n", hres);
    ok_(__FILE__,line)(col != NULL, "col = NULL\n");

    IHTMLDocument3_Release(doc3);
    SysFreeString(str);
    return col;
}

static void test_elem_names(IHTMLDocument2 *doc)
{
    IHTMLElementCollection *col;
    IHTMLElement *body;
    LONG len;
    HRESULT hres;

    static const elem_type_t test1_types[] = {ET_INPUT, ET_A, ET_DIV};

    body = doc_get_body(doc);

    test_elem_set_innerhtml((IUnknown*)body,
            "<input name=\"test\"><a name=\"test\"></a><a name=\"xxx\"></a><div id=\"test\"></div>");
    col = doc_get_elems_by_name(doc, "test");
    test_elem_collection((IUnknown*)col, test1_types, sizeof(test1_types)/sizeof(*test1_types));
    IHTMLElementCollection_Release(col);

    col = doc_get_elems_by_name(doc, "yyy");
    test_elem_collection((IUnknown*)col, NULL, 0);
    IHTMLElementCollection_Release(col);

    /* case insensivity test */
    col = doc_get_elems_by_name(doc, "Xxx");
    hres = IHTMLElementCollection_get_length(col, &len);
    ok(hres == S_OK, "get_length failed: %08x\n", hres);
    todo_wine ok(len == 1, "len = %d\n", len);
    IHTMLElementCollection_Release(col);

    IHTMLElement_Release(body);
}

static void test_elems2(IHTMLDocument2 *doc)
{
    IHTMLElement *elem, *elem2, *div;

    static const elem_type_t outer_types[] = {
        ET_BR,
        ET_A
    };

    div = get_doc_elem_by_id(doc, "divid");

    elem = get_elem_by_id(doc, "linkid", TRUE);
    if(elem) {
        test_link_disabled(elem, VARIANT_FALSE);
        test_link_rel(elem, "stylesheet");
        test_link_rev(elem, NULL);
        test_link_type(elem, "text/css");
        test_link_href(elem, "about:blank");
        test_link_media(elem, "all");
        link_put_disabled(elem, VARIANT_TRUE);
        link_put_rel(elem, "prev");
        link_put_rev(elem, "next");
        link_put_type(elem, "text/plain");
        link_put_href(elem, "about:prev");
        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)div, "<div id=\"innerid\"></div>");
    elem2 = get_doc_elem_by_id(doc, "innerid");
    ok(elem2 != NULL, "elem2 == NULL\n");
    test_elem_set_outerhtml((IUnknown*)elem2, "<br><a href=\"about:blank\" id=\"aid\">a</a>");
    test_elem_all((IUnknown*)div, outer_types, sizeof(outer_types)/sizeof(*outer_types));
    IHTMLElement_Release(elem2);

    elem2 = get_doc_elem_by_id(doc, "aid");
    ok(elem2 != NULL, "elem2 == NULL\n");
    test_elem_set_outerhtml((IUnknown*)elem2, "");
    test_elem_all((IUnknown*)div, outer_types, 1);
    IHTMLElement_Release(elem2);

    test_elem_set_innerhtml((IUnknown*)div, "<textarea id=\"ta\"></textarea>");
    elem = get_elem_by_id(doc, "ta", TRUE);
    if(elem) {
        IHTMLFormElement *form;

        test_textarea_value((IUnknown*)elem, NULL);
        test_textarea_put_value((IUnknown*)elem, "test");
        test_textarea_defaultvalue((IUnknown*)elem, NULL);
        test_textarea_put_defaultvalue((IUnknown*)elem, "defval text");
        test_textarea_put_value((IUnknown*)elem, "test");
        test_textarea_readonly((IUnknown*)elem, VARIANT_FALSE);
        test_textarea_put_readonly((IUnknown*)elem, VARIANT_TRUE);
        test_textarea_put_readonly((IUnknown*)elem, VARIANT_FALSE);
        test_textarea_type((IUnknown*)elem);

        form = get_textarea_form((IUnknown*)elem);
        ok(!form, "form = %p\n", form);

        test_elem_istextedit(elem, VARIANT_TRUE);

        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)div, "<textarea id=\"ta\">default text</textarea>");
    elem = get_elem_by_id(doc, "ta", TRUE);
    if(elem) {
        test_textarea_defaultvalue((IUnknown*)elem, "default text");
        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)div, "<form id=\"fid\"><textarea id=\"ta\"></textarea></form>");
    elem = get_elem_by_id(doc, "ta", TRUE);
    if(elem) {
        IHTMLFormElement *form;

        elem2 = get_elem_by_id(doc, "fid", TRUE);
        ok(elem2 != NULL, "elem2 == NULL\n");

        form = get_textarea_form((IUnknown*)elem);
        ok(form != NULL, "form = NULL\n");
        ok(iface_cmp((IUnknown*)form, (IUnknown*)elem2), "form != elem2\n");

        IHTMLFormElement_Release(form);
        IHTMLElement_Release(elem2);
        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)div,
            "<input value=\"val\" id =\"inputid\"  />");
    elem = get_elem_by_id(doc, "inputid", TRUE);
    if(elem) {
        test_input_defaultValue((IUnknown*)elem, "val");
        test_input_put_value((IUnknown*)elem, "test");
        test_input_put_defaultValue((IUnknown*)elem, "new val");
        test_input_value((IUnknown*)elem, "test");
        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)div, "");
    test_insert_adjacent_elems(doc, div);

    test_elem_set_innerhtml((IUnknown*)div,
            "<form id=\"form\"><input type=\"button\" /><div><input type=\"text\" id=\"inputid\"/></div></textarea>");
    elem = get_elem_by_id(doc, "form", TRUE);
    if(elem) {
        test_form_length((IUnknown*)elem, 2);
        test_form_item(elem);
        test_form_action((IUnknown*)elem, NULL);
        test_form_put_action((IUnknown*)elem, "about:blank");
        test_form_method((IUnknown*)elem, "get");
        test_form_put_method((IUnknown*)elem, S_OK, "post");
        test_form_put_method((IUnknown*)elem, E_INVALIDARG, "put");
        test_form_method((IUnknown*)elem, "post");
        test_form_name((IUnknown*)elem, NULL);
        test_form_put_name((IUnknown*)elem, "Name");
        test_form_encoding((IUnknown*)elem, "application/x-www-form-urlencoded");
        test_form_put_encoding((IUnknown*)elem, S_OK, "text/plain");
        test_form_put_encoding((IUnknown*)elem, S_OK, "multipart/form-data");
        test_form_put_encoding((IUnknown*)elem, E_INVALIDARG, "image/png");
        test_form_encoding((IUnknown*)elem, "multipart/form-data");
        test_form_elements((IUnknown*)elem);
        test_form_reset((IUnknown*)elem);
        test_form_target((IUnknown*)elem);
        IHTMLElement_Release(elem);

        elem = get_elem_by_id(doc, "inputid", TRUE);
        test_input_get_form((IUnknown*)elem, "form");
        IHTMLElement_Release(elem);
    }

    test_elem_set_innerhtml((IUnknown*)div,
            "<form id=\"form\" name=\"form_name\"><select id=\"sform\"><option id=\"oform\"></option></select></form>");
    elem = get_elem_by_id(doc, "sform", TRUE);
    elem2 = get_elem_by_id(doc, "form", TRUE);
    if(elem && elem2) {
        test_select_form((IUnknown*)elem, (IUnknown*)elem2);
        IHTMLElement_Release(elem);

        elem = get_elem_by_id(doc, "oform", TRUE);
        if(elem) {
            test_option_form((IUnknown*)elem, (IUnknown*)elem2);
            IHTMLElement_Release(elem);
        }
        IHTMLElement_Release(elem2);
    }

    test_attr(div);
    test_blocked(doc, div);
    test_elem_names(doc);

    IHTMLElement_Release(div);
}

static void test_create_elems(IHTMLDocument2 *doc)
{
    IHTMLElement *elem, *body, *elem2;
    IHTMLDOMNode *node, *node2, *node3, *comment;
    IHTMLDOMAttribute *attr;
    IHTMLDocument5 *doc5;
    IDispatch *disp;
    VARIANT var;
    LONG type;
    HRESULT hres;
    BSTR str;

    static const elem_type_t types1[] = { ET_TESTG };

    elem = test_create_elem(doc, "TEST");
    test_elem_tag((IUnknown*)elem, "TEST");
    type = get_node_type((IUnknown*)elem);
    ok(type == 1, "type=%d\n", type);
    test_ifaces((IUnknown*)elem, elem_iids);
    test_disp((IUnknown*)elem, &DIID_DispHTMLGenericElement, "[object]");
    test_elem_source_index(elem, -1);

    body = doc_get_body(doc);
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

    IHTMLElement_Release(elem);
    IHTMLDOMNode_Release(node);

    node = test_create_text(doc, "abc");
    test_ifaces((IUnknown*)node, text_iids);
    test_disp((IUnknown*)node, &DIID_DispHTMLDOMTextNode, "[object]");
    test_text_length((IUnknown*)node, 3);
    test_text_data((IUnknown*)node, "abc");
    set_text_data((IUnknown*)node, "test");
    test_text_data((IUnknown*)node, "test");
    text_append_data((IUnknown*)node, " append");
    test_text_data((IUnknown*)node, "test append");
    text_append_data((IUnknown*)node, NULL);
    test_text_data((IUnknown*)node, "test append");
    set_text_data((IUnknown*)node, "test");

    V_VT(&var) = VT_NULL;
    node2 = test_node_insertbefore((IUnknown*)body, node, &var);
    IHTMLDOMNode_Release(node);

    node = test_create_text(doc, "insert ");

    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = (IDispatch*)node2;
    node3 = test_node_insertbefore((IUnknown*)body, node, &var);
    IHTMLDOMNode_Release(node);
    IHTMLDOMNode_Release(node2);
    IHTMLDOMNode_Release(node3);

    test_elem_innertext(body, "insert test");
    test_elem_innerhtml((IUnknown*)body, "insert test");

    node = test_create_text(doc, " Test");
    V_VT(&var) = VT_DISPATCH;
    V_DISPATCH(&var) = NULL;
    test_node_insertbefore((IUnknown*)body, node, &var);
    test_elem_innertext(body, "insert test Test");
    IHTMLDOMNode_Release(node);

    doc5 = get_htmldoc5_iface((IUnknown*)doc);
    if(doc5) {
        str = a2bstr("testing");
        hres = IHTMLDocument5_createComment(doc5, str, &comment);
        SysFreeString(str);
        ok(hres == S_OK, "createComment failed: %08x\n", hres);
        if(hres == S_OK)
        {
            type = get_node_type((IUnknown*)comment);
            ok(type == 8, "type=%d, expected 8\n", type);

            test_node_get_value_str((IUnknown*)comment, "testing");
            test_elem_title((IUnknown*)comment, NULL);
            test_elem_set_title((IUnknown*)comment, "comment title");
            test_elem_title((IUnknown*)comment, "comment title");
            test_comment_text((IUnknown*)comment, "<!--testing-->");
            test_elem_outerhtml((IUnknown*)comment, "<!--testing-->");
            test_comment_attrs((IUnknown*)comment);

            IHTMLDOMNode_Release(comment);
        }

        str = a2bstr("Test");
        hres = IHTMLDocument5_createAttribute(doc5, str, &attr);
        ok(hres == S_OK, "createAttribute dailed: %08x\n", hres);
        SysFreeString(str);
        if(SUCCEEDED(hres)) {
            test_disp((IUnknown*)attr, &DIID_DispHTMLDOMAttribute, "[object]");
            test_ifaces((IUnknown*)attr, attr_iids);
            test_no_iface((IUnknown*)attr, &IID_IHTMLDOMNode);

            test_attr_node_name(attr, "Test");

            IHTMLDOMAttribute_Release(attr);
        }

        IHTMLDocument5_Release(doc5);
    }

    IHTMLElement_Release(body);
}

static void test_replacechild_elems(IHTMLDocument2 *doc)
{
    IHTMLElement *body;
    IHTMLDOMNode *node, *node2, *node3;
    IHTMLDOMNode *nodeBody, *nodeNew;
    HRESULT hres;
    VARIANT var;

    body = doc_get_body(doc);

    node = test_create_text(doc, "insert");

    V_VT(&var) = VT_NULL;
    V_DISPATCH(&var) = NULL;
    node2 = test_node_insertbefore((IUnknown*)body, node, &var);
    IHTMLDOMNode_Release(node);

    test_elem_innertext(body, "insert");

    node3 = test_create_text(doc, "replaced");

    nodeBody = _get_node_iface(__LINE__, (IUnknown *)body);

    hres = IHTMLDOMNode_replaceChild(nodeBody, node3, node2, &nodeNew);
    ok(hres == S_OK, "Expected S_OK, got 0x%08x\n", hres);

    test_elem_innertext(body, "replaced");

    IHTMLDOMNode_Release(node2);
    IHTMLDOMNode_Release(node3);
    IHTMLDOMNode_Release(nodeBody);

    IHTMLElement_Release(body);
}

static void test_noscript(IHTMLDocument2 *doc)
{
    IHTMLElementCollection *col;
    IHTMLElement *body;
    HRESULT hres;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_NOSCRIPT,
        ET_BODY,
        ET_NOSCRIPT
    };

    static const elem_type_t body_all_types[] = {
        ET_DIV,
        ET_NOSCRIPT
    };

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    IHTMLElementCollection_Release(col);

    body = doc_get_body(doc);
    test_elem_set_innerhtml((IUnknown*)body, "<div>test</div><noscript><a href=\"about:blank\">A</a></noscript>");
    test_elem_all((IUnknown*)body, body_all_types, sizeof(body_all_types)/sizeof(*body_all_types));
    IHTMLElement_Release(body);
}

static void test_doctype(IHTMLDocument2 *doc)
{
    IHTMLDocument2 *doc_node;
    IHTMLDOMNode *doctype;
    int type;

    doc_node = get_doc_node(doc);
    doctype = get_first_child((IUnknown*)doc_node);
    IHTMLDocument2_Release(doc_node);

    type = get_node_type((IUnknown*)doctype);
    ok(type == 8, "type = %d\n", type);

    test_comment_text((IUnknown*)doctype, "<!DOCTYPE html>");
    test_elem_type((IUnknown*)doctype, ET_COMMENT);
    IHTMLDOMNode_Release(doctype);
}

static void test_null_write(IHTMLDocument2 *doc)
{
    HRESULT hres;

    doc_write(doc, FALSE, NULL);
    doc_write(doc, TRUE, NULL);

    hres = IHTMLDocument2_write(doc, NULL);
    ok(hres == S_OK,
       "Expected IHTMLDocument2::write to return S_OK, got 0x%08x\n", hres);

    hres = IHTMLDocument2_writeln(doc, NULL);
    ok(hres == S_OK,
       "Expected IHTMLDocument2::writeln to return S_OK, got 0x%08x\n", hres);
}

static void test_create_stylesheet(IHTMLDocument2 *doc)
{
    IHTMLStyleSheet *stylesheet, *stylesheet2;
    IHTMLStyleElement *style_elem;
    IHTMLElement *doc_elem, *elem;
    HRESULT hres;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_BODY,
        ET_DIV
    };

    static const elem_type_t all_types2[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_STYLE,
        ET_BODY,
        ET_DIV
    };

    test_doc_all(doc, all_types, sizeof(all_types)/sizeof(*all_types));

    hres = IHTMLDocument2_createStyleSheet(doc, NULL, -1, &stylesheet);
    ok(hres == S_OK, "createStyleSheet failed: %08x\n", hres);

    test_doc_all(doc, all_types2, sizeof(all_types2)/sizeof(*all_types2));

    doc_elem = get_doc_elem(doc);

    test_elem_getelembytag((IUnknown*)doc_elem, ET_STYLE, 1, &elem);
    IHTMLElement_Release(doc_elem);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLStyleElement, (void**)&style_elem);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLStyleElement iface: %08x\n", hres);

    stylesheet2 = NULL;
    hres = IHTMLStyleElement_get_styleSheet(style_elem, &stylesheet2);
    ok(hres == S_OK, "get_styleSheet failed: %08x\n", hres);
    ok(stylesheet2 != NULL, "stylesheet2 == NULL\n");
    ok(iface_cmp((IUnknown*)stylesheet, (IUnknown*)stylesheet2), "stylesheet != stylesheet2\n");

    IHTMLStyleSheet_Release(stylesheet2);
    IHTMLStyleSheet_Release(stylesheet);

    IHTMLStyleElement_Release(style_elem);
}

static void test_exec(IUnknown *unk, const GUID *grpid, DWORD cmdid, VARIANT *in, VARIANT *out)
{
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IOleCommandTarget, (void**)&cmdtrg);
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

static void test_cond_comment(IHTMLDocument2 *doc)
{
    IHTMLElementCollection *col;
    HRESULT hres;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_BODY,
        ET_BR
    };

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    IHTMLElementCollection_Release(col);
}

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    ok(IsEqualGUID(riid, &IID_IServiceProvider), "riid = %s\n", wine_dbgstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI Unknown_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl UnknownVtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release,
};
static IUnknown obj_ident_test = { &UnknownVtbl };

static void test_frame(IDispatch *disp, const char *exp_id)
{
    IHTMLWindow2 *frame2, *parent, *top;
    IHTMLDocument2 *parent_doc, *top_doc;
    IHTMLWindow4 *frame;
    IHTMLFrameBase *frame_elem;
    IObjectIdentity *obj_ident;
    ITravelLogClient *tlc;
    HRESULT hres;

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLWindow4, (void**)&frame);
    ok(hres == S_OK, "Could not get IHTMLWindow4 interface: 0x%08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLWindow4_get_frameElement(frame, &frame_elem);
    ok(hres == S_OK, "IHTMLWindow4_get_frameElement failed: 0x%08x\n", hres);
    IHTMLWindow4_Release(frame);
    if(FAILED(hres))
        return;

    test_elem_type((IUnknown*)frame_elem, ET_FRAME);
    test_frame_doc((IUnknown*)frame_elem, FALSE);
    test_elem_id((IUnknown*)frame_elem, exp_id);
    IHTMLFrameBase_Release(frame_elem);

    hres = IDispatch_QueryInterface(disp, &IID_IHTMLWindow2, (void**)&frame2);
    ok(hres == S_OK, "Could not get IHTMLWindow2 interface: 0x%08x\n", hres);
    if(FAILED(hres))
        return;

    hres = IHTMLWindow2_get_parent(frame2, &parent);
    ok(hres == S_OK, "IHTMLWindow2_get_parent failed: 0x%08x\n", hres);
    if(FAILED(hres)){
        IHTMLWindow2_Release(frame2);
        return;
    }

    hres = IHTMLWindow2_QueryInterface(frame2, &IID_IObjectIdentity, (void**)&obj_ident);
    ok(hres == S_OK, "Could not get IObjectIdentity interface: %08x\n", hres);
    hres = IHTMLWindow2_QueryInterface(frame2, &IID_ITravelLogClient, (void**)&tlc);
    if(hres == E_NOINTERFACE) {
        win_skip("IID_ITravelLogClient not available\n");
        tlc = NULL;
    }else {
        ok(hres == S_OK, "Could not get ITravelLogClient interface: %08x\n", hres);

        hres = IObjectIdentity_IsEqualObject(obj_ident, (IUnknown*)tlc);
        ok(hres == S_OK, "IsEqualObject returned: 0x%08x\n", hres);
        ITravelLogClient_Release(tlc);
    }

    hres = IObjectIdentity_IsEqualObject(obj_ident, (IUnknown*)obj_ident);
    ok(hres == S_OK, "IsEqualObject returned: 0x%08x\n", hres);
    hres = IObjectIdentity_IsEqualObject(obj_ident, (IUnknown*)parent);
    ok(hres == S_FALSE, "IsEqualObject returned: 0x%08x\n", hres);
    hres = IObjectIdentity_IsEqualObject(obj_ident, &obj_ident_test);
    ok(hres == E_NOINTERFACE, "IsEqualObject returned: 0x%08x\n", hres);

    IObjectIdentity_Release(obj_ident);

    hres = IHTMLWindow2_get_document(parent, &parent_doc);
    ok(hres == S_OK, "IHTMLWindow2_get_document failed: 0x%08x\n", hres);
    IHTMLWindow2_Release(parent);
    if(FAILED(hres)){
        IHTMLWindow2_Release(frame2);
        return;
    }

    test_doc_title(parent_doc, "frameset test");
    IHTMLDocument2_Release(parent_doc);

    /* test get_top */
    hres = IHTMLWindow2_get_top(frame2, &top);
    ok(hres == S_OK, "IHTMLWindow2_get_top failed: 0x%08x\n", hres);
    IHTMLWindow2_Release(frame2);
    if(FAILED(hres))
        return;

    hres = IHTMLWindow2_get_document(top, &top_doc);
    ok(hres == S_OK, "IHTMLWindow2_get_document failed: 0x%08x\n", hres);
    IHTMLWindow2_Release(top);
    if(FAILED(hres))
        return;

    test_doc_title(top_doc, "frameset test");
    IHTMLDocument2_Release(top_doc);
}

static void test_frames_collection(IHTMLFramesCollection2 *frames, const char *frid)
{
    VARIANT index_var, result_var;
    LONG length;
    HRESULT hres;

    /* test result length */
    hres = IHTMLFramesCollection2_get_length(frames, &length);
    ok(hres == S_OK, "IHTMLFramesCollection2_get_length failed: 0x%08x\n", hres);
    ok(length == 3, "IHTMLFramesCollection2_get_length should have been 3, was: %d\n", length);

    /* test first frame */
    V_VT(&index_var) = VT_I4;
    V_I4(&index_var) = 0;
    hres = IHTMLFramesCollection2_item(frames, &index_var, &result_var);
    ok(hres == S_OK, "IHTMLFramesCollection2_item failed: 0x%08x\n", hres);
    if(SUCCEEDED(hres)) {
        ok(V_VT(&result_var) == VT_DISPATCH, "result type should have been VT_DISPATCH, was: 0x%x\n", V_VT(&result_var));
        test_frame((IDispatch*)V_DISPATCH(&result_var), "fr1");
    }
    VariantClear(&result_var);

    /* test second frame */
    V_I4(&index_var) = 1;
    hres = IHTMLFramesCollection2_item(frames, &index_var, &result_var);
    ok(hres == S_OK, "IHTMLFramesCollection2_item failed: 0x%08x\n", hres);
    if(SUCCEEDED(hres)) {
        ok(V_VT(&result_var) == VT_DISPATCH, "result type should have been VT_DISPATCH, was: 0x%x\n", V_VT(&result_var));
        test_frame((IDispatch*)V_DISPATCH(&result_var), "fr2");
    }
    VariantClear(&result_var);

    /* fail on next frame */
    V_I4(&index_var) = 3;
    hres = IHTMLFramesCollection2_item(frames, &index_var, &result_var);
    ok(hres == DISP_E_MEMBERNOTFOUND, "IHTMLFramesCollection2_item should have"
           "failed with DISP_E_MEMBERNOTFOUND, instead: 0x%08x\n", hres);
    VariantClear(&result_var);

    /* string argument (element id lookup) */
    V_VT(&index_var) = VT_BSTR;
    V_BSTR(&index_var) = a2bstr(frid);
    hres = IHTMLFramesCollection2_item(frames, &index_var, &result_var);
    ok(hres == S_OK, "IHTMLFramesCollection2_item failed: 0x%08x\n", hres);
    if(SUCCEEDED(hres)) {
        ok(V_VT(&result_var) == VT_DISPATCH, "result type should have been VT_DISPATCH, was: 0x%x\n", V_VT(&result_var));
        test_frame(V_DISPATCH(&result_var), frid);
    }
    VariantClear(&result_var);
    VariantClear(&index_var);

    /* invalid argument */
    V_VT(&index_var) = VT_BOOL;
    V_BOOL(&index_var) = VARIANT_TRUE;
    hres = IHTMLFramesCollection2_item(frames, &index_var, &result_var);
    ok(hres == E_INVALIDARG, "IHTMLFramesCollection2_item should have"
           "failed with E_INVALIDARG, instead: 0x%08x\n", hres);
    VariantClear(&result_var);
}

static void test_frameset(IHTMLDocument2 *doc)
{
    IHTMLWindow2 *window;
    IHTMLFramesCollection2 *frames;
    IHTMLElement *elem;
    HRESULT hres;

    window = get_doc_window(doc);

    /* test using IHTMLFramesCollection object */

    hres = IHTMLWindow2_get_frames(window, &frames);
    ok(hres == S_OK, "IHTMLWindow2_get_frames failed: 0x%08x\n", hres);
    if(FAILED(hres))
        return;

    test_frames_collection(frames, "fr1");
    IHTMLFramesCollection2_Release(frames);

    hres = IHTMLDocument2_get_frames(doc, &frames);
    ok(hres == S_OK, "IHTMLDocument2_get_frames failed: 0x%08x\n", hres);
    if(FAILED(hres))
        return;

    test_frames_collection(frames, "fr1");
    IHTMLFramesCollection2_Release(frames);

    /* test using IHTMLWindow2 inheritance */
    test_frames_collection((IHTMLFramesCollection2*)window, "fr2");

    /* getElementById with node name attributes */
    elem = get_doc_elem_by_id(doc, "nm1");
    test_elem_id((IUnknown*)elem, "fr1");

    test_framebase((IUnknown*)elem);
    test_framebase_name(elem, "nm1");
    test_framebase_put_name(elem, "frame name");
    test_framebase_put_name(elem, NULL);
    test_framebase_put_name(elem, "nm1");
    test_framebase_src(elem, "about:blank");
    IHTMLElement_Release(elem);

    /* get_name with no name attr */
    elem = get_doc_elem_by_id(doc, "fr3");
    test_framebase_name(elem, NULL);
    test_framebase_put_name(elem, "frame name");
    test_framebase_put_name(elem, NULL);
    IHTMLElement_Release(elem);

    IHTMLWindow2_Release(window);
}

static IHTMLDocument2 *create_docfrag(IHTMLDocument2 *doc)
{
    IHTMLDocument2 *frag;
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);

    hres = IHTMLDocument3_createDocumentFragment(doc3, &frag);
    IHTMLDocument3_Release(doc3);
    ok(hres == S_OK, "createDocumentFragment failed: %08x\n", hres);
    ok(frag != NULL, "frag == NULL\n");

    return frag;
}

static void test_docfrag(IHTMLDocument2 *doc)
{
    IHTMLDocument2 *frag, *owner_doc, *doc_node;
    IHTMLElement *div, *body, *br;
    IHTMLElementCollection *col;
    IHTMLLocation *location;
    HRESULT hres;

    static const elem_type_t all_types[] = {
        ET_HTML,
        ET_HEAD,
        ET_TITLE,
        ET_BODY,
        ET_DIV,
        ET_BR
    };

    frag = create_docfrag(doc);

    test_disp((IUnknown*)frag, &DIID_DispHTMLDocument, "[object]");

    body = (void*)0xdeadbeef;
    hres = IHTMLDocument2_get_body(frag, &body);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);
    ok(!body, "body != NULL\n");

    location = (void*)0xdeadbeef;
    hres = IHTMLDocument2_get_location(frag, &location);
    ok(hres == E_UNEXPECTED, "get_location failed: %08x\n", hres);
    ok(location == (void*)0xdeadbeef, "location changed\n");

    br = test_create_elem(doc, "BR");
    test_elem_source_index(br, -1);
    test_node_append_child((IUnknown*)frag, (IUnknown*)br);
    test_elem_source_index(br, 0);
    IHTMLElement_Release(br);

    div = get_elem_by_id(doc, "divid", TRUE);
    test_node_append_child((IUnknown*)div, (IUnknown*)frag);
    IHTMLElement_Release(div);

    hres = IHTMLDocument2_get_all(doc, &col);
    ok(hres == S_OK, "get_all failed: %08x\n", hres);
    test_elem_collection((IUnknown*)col, all_types, sizeof(all_types)/sizeof(all_types[0]));
    IHTMLElementCollection_Release(col);

    div = test_create_elem(frag, "div");
    owner_doc = get_owner_doc((IUnknown*)div);
    doc_node = get_doc_node(doc);
    ok(iface_cmp((IUnknown*)owner_doc, (IUnknown*)doc_node), "owner_doc != doc_node\n");
    IHTMLDocument2_Release(doc_node);
    IHTMLDocument2_Release(owner_doc);
    IHTMLElement_Release(div);

    IHTMLDocument2_Release(frag);
}

static void check_quirks_mode(IHTMLDocument2 *doc)
{
    test_compatmode(doc, "BackCompat");
}

static void check_strict_mode(IHTMLDocument2 *doc)
{
    test_compatmode(doc, "CSS1Compat");
}

static void test_quirks_mode_offsetHeight(IHTMLDocument2 *doc)
{
    IHTMLElement *elem;
    HRESULT hres;
    LONG oh;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body fauled: %08x\n", hres);

    /* body.offsetHeight value depends on window size in quirks mode */
    hres = IHTMLElement_get_offsetHeight(elem, &oh);
    ok(hres == S_OK, "get_offsetHeight failed: %08x\n", hres);
    todo_wine ok(oh == 500, "offsetHeight = %d\n", oh);
    IHTMLElement_Release(elem);
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

        hres = IHTMLDocument2_get_readyState(notif_doc, &state);
        ok(hres == S_OK, "get_readyState failed: %08x\n", hres);

        if(!strcmp_wa(state, "complete"))
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

static HRESULT cs_qi(REFIID,void **);
static IOleDocumentView *view;
static HWND container_hwnd;

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
    static const GUID undocumented_frame_iid = {0xfbece6c9,0x48d7,0x4a37,{0x8f,0xe3,0x6a,0xd4,0x27,0x2f,0xdd,0xac}};

    if(!IsEqualGUID(&undocumented_frame_iid, riid))
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InPlaceFrame_AddRef(IOleInPlaceFrame *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceFrame_Release(IOleInPlaceFrame *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceFrame_GetWindow(IOleInPlaceFrame *iface, HWND *phwnd)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_ContextSensitiveHelp(IOleInPlaceFrame *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_GetBorder(IOleInPlaceFrame *iface, LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RequestBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetBorderSpace(IOleInPlaceFrame *iface,
        LPCBORDERWIDTHS pborderwidths)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_InsertMenus(IOleInPlaceFrame *iface, HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetMenu(IOleInPlaceFrame *iface, HMENU hmenuShared,
        HOLEMENU holemenu, HWND hwndActiveObject)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_RemoveMenus(IOleInPlaceFrame *iface, HMENU hmenuShared)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_SetStatusText(IOleInPlaceFrame *iface, LPCOLESTR pszStatusText)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceFrame_EnableModeless(IOleInPlaceFrame *iface, BOOL fEnable)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceFrame_TranslateAccelerator(IOleInPlaceFrame *iface, LPMSG lpmsg, WORD wID)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleInPlaceFrameVtbl InPlaceFrameVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceFrame_SetActiveObject,
    InPlaceFrame_InsertMenus,
    InPlaceFrame_SetMenu,
    InPlaceFrame_RemoveMenus,
    InPlaceFrame_SetStatusText,
    InPlaceFrame_EnableModeless,
    InPlaceFrame_TranslateAccelerator
};

static IOleInPlaceFrame InPlaceFrame = { &InPlaceFrameVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    return 2;
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    return 1;
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    *phwnd = container_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
                LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    static const RECT rect = {0,0,500,500};

    *ppFrame = &InPlaceFrame;
    *ppDoc = NULL;
    *lprcPosRect = rect;
    *lprcClipRect = rect;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = container_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtant)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}

static const IOleInPlaceSiteVtbl InPlaceSiteVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange,
};

static IOleInPlaceSite InPlaceSite = { &InPlaceSiteVtbl };

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    return 2;
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    return 1;
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign, DWORD dwWhichMoniker,
        IMoniker **ppmon)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

static IOleClientSite ClientSite = { &ClientSiteVtbl };

static HRESULT WINAPI DocumentSite_QueryInterface(IOleDocumentSite *iface, REFIID riid, void **ppv)
{
    return cs_qi(riid, ppv);
}

static ULONG WINAPI DocumentSite_AddRef(IOleDocumentSite *iface)
{
    return 2;
}

static ULONG WINAPI DocumentSite_Release(IOleDocumentSite *iface)
{
    return 1;
}

static HRESULT WINAPI DocumentSite_ActivateMe(IOleDocumentSite *iface, IOleDocumentView *pViewToActivate)
{
    RECT rect = {0,0,500,500};
    IOleDocument *document;
    HRESULT hres;

    hres = IOleDocumentView_QueryInterface(pViewToActivate, &IID_IOleDocument, (void**)&document);
    ok(hres == S_OK, "could not get IOleDocument: %08x\n", hres);

    hres = IOleDocument_CreateView(document, &InPlaceSite, NULL, 0, &view);
    IOleDocument_Release(document);
    ok(hres == S_OK, "CreateView failed: %08x\n", hres);

    hres = IOleDocumentView_SetInPlaceSite(view, &InPlaceSite);
    ok(hres == S_OK, "SetInPlaceSite failed: %08x\n", hres);

    hres = IOleDocumentView_UIActivate(view, TRUE);
    ok(hres == S_OK, "UIActivate failed: %08x\n", hres);

    hres = IOleDocumentView_SetRect(view, &rect);
    ok(hres == S_OK, "SetRect failed: %08x\n", hres);

    hres = IOleDocumentView_Show(view, TRUE);
    ok(hres == S_OK, "Show failed: %08x\n", hres);

    return S_OK;
}

static const IOleDocumentSiteVtbl DocumentSiteVtbl = {
    DocumentSite_QueryInterface,
    DocumentSite_AddRef,
    DocumentSite_Release,
    DocumentSite_ActivateMe
};

static IOleDocumentSite DocumentSite = { &DocumentSiteVtbl };

static HRESULT cs_qi(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IOleClientSite, riid))
        *ppv = &ClientSite;
    else if(IsEqualGUID(&IID_IOleDocumentSite, riid))
        *ppv = &DocumentSite;
    else if(IsEqualGUID(&IID_IOleWindow, riid) || IsEqualGUID(&IID_IOleInPlaceSite, riid))
        *ppv = &InPlaceSite;

    return *ppv ? S_OK : E_NOINTERFACE;
}

static void set_client_site(IHTMLDocument2 *doc, BOOL set)
{
    IOleObject *oleobj;
    HRESULT hres;

    if(!set && view) {
        IOleDocumentView_Show(view, FALSE);
        IOleDocumentView_CloseView(view, 0);
        IOleDocumentView_SetInPlaceSite(view, NULL);
        IOleDocumentView_Release(view);
        view = NULL;
    }

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IOleObject, (void**)&oleobj);
    ok(hres == S_OK, "Could not et IOleObject: %08x\n", hres);

    hres = IOleObject_SetClientSite(oleobj, set ? &ClientSite : NULL);
    ok(hres == S_OK, "SetClientSite failed: %08x\n", hres);

    if(set) {
        IHlinkTarget *hlink;

        hres = IOleObject_QueryInterface(oleobj, &IID_IHlinkTarget, (void**)&hlink);
        ok(hres == S_OK, "Could not get IHlinkTarget iface: %08x\n", hres);

        hres = IHlinkTarget_Navigate(hlink, 0, NULL);
        ok(hres == S_OK, "Navgate failed: %08x\n", hres);

        IHlinkTarget_Release(hlink);
    }

    IOleObject_Release(oleobj);
}

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
    ULONG ref;
    MSG msg;

    doc = create_doc_with_string(str);
    if(!doc)
        return;

    set_client_site(doc, TRUE);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    test(doc);

    set_client_site(doc, FALSE);
    ref = IHTMLDocument2_Release(doc);
    ok(!ref || broken(ref == 1), /* Vista */
       "ref = %d\n", ref);
}

static void test_quirks_mode(void)
{
    run_domtest("<html></html>", check_quirks_mode);
    run_domtest("<!DOCTYPE html>\n<html></html>", check_strict_mode);
    run_domtest("<!-- comment --><!DOCTYPE html>\n<html></html>", check_quirks_mode);
    run_domtest("<html><body></body></html>", test_quirks_mode_offsetHeight);
}

START_TEST(dom)
{
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");
    pLCIDToLocaleName = (void*)GetProcAddress(hkernel32, "LCIDToLocaleName");
    pGetUserDefaultUILanguage = (void*)GetProcAddress(hkernel32, "GetUserDefaultUILanguage");

    CoInitialize(NULL);
    container_hwnd = CreateWindowA("static", NULL, WS_POPUP|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, NULL, NULL);

    run_domtest(doc_str1, test_doc_elem);
    run_domtest(doc_str1, test_get_set_attr);
    run_domtest(range_test_str, test_txtrange);
    run_domtest(range_test2_str, test_txtrange2);
    if (winetest_interactive || ! is_ie_hardened()) {
        run_domtest(elem_test_str, test_elems);
        run_domtest(elem_test2_str, test_elems2);
        run_domtest(noscript_str, test_noscript);
    }else {
        skip("IE running in Enhanced Security Configuration\n");
    }
    run_domtest(doc_blank, test_create_elems);
    run_domtest(doc_blank, test_defaults);
    run_domtest(doc_blank, test_null_write);
    run_domtest(emptydiv_str, test_create_stylesheet);
    run_domtest(indent_test_str, test_indent);
    run_domtest(cond_comment_str, test_cond_comment);
    run_domtest(frameset_str, test_frameset);
    run_domtest(emptydiv_str, test_docfrag);
    run_domtest(doc_blank, test_replacechild_elems);
    run_domtest(doctype_str, test_doctype);

    test_quirks_mode();

    DestroyWindow(container_hwnd);
    CoUninitialize();
}
