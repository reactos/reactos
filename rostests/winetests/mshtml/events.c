/*
 * Copyright 2008-2011 Jacek Caban for CodeWeavers
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
#include <mshtmdid.h>
#include <docobj.h>
#include <hlink.h>
//#include "dispex.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    do { called_ ## func = FALSE; expect_ ## func = TRUE; } while(0)

#define CHECK_EXPECT2(func) \
    do { \
    trace(#func "\n"); \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_EXPECT(document_onclick);
DEFINE_EXPECT(body_onclick);
DEFINE_EXPECT(doc_onclick_attached);
DEFINE_EXPECT(div_onclick);
DEFINE_EXPECT(div_onclick_attached);
DEFINE_EXPECT(timeout);
DEFINE_EXPECT(doccp_onclick);
DEFINE_EXPECT(doccp_onclick_cancel);
DEFINE_EXPECT(div_onclick_disp);
DEFINE_EXPECT(invoke_onclick);
DEFINE_EXPECT(iframe_onreadystatechange_loading);
DEFINE_EXPECT(iframe_onreadystatechange_interactive);
DEFINE_EXPECT(iframe_onreadystatechange_complete);
DEFINE_EXPECT(iframedoc_onreadystatechange);
DEFINE_EXPECT(img_onload);
DEFINE_EXPECT(img_onerror);
DEFINE_EXPECT(input_onfocus);
DEFINE_EXPECT(input_onblur);
DEFINE_EXPECT(form_onsubmit);
DEFINE_EXPECT(form_onclick);
DEFINE_EXPECT(submit_onclick);
DEFINE_EXPECT(submit_onclick_cancel);
DEFINE_EXPECT(submit_onclick_attached);
DEFINE_EXPECT(submit_onclick_attached_check_cancel);
DEFINE_EXPECT(submit_onclick_setret);
DEFINE_EXPECT(elem2_cp_onclick);
DEFINE_EXPECT(iframe_onload);

static HWND container_hwnd = NULL;
static IHTMLWindow2 *window;
static IOleDocumentView *view;
static BOOL is_ie9plus;

typedef struct {
    LONG x;
    LONG y;
    LONG offset_x;
    LONG offset_y;
} xy_test_t;

static const xy_test_t no_xy = {-10,-10,-10,-10};

static const char empty_doc_str[] =
    "<html></html>";

static const char click_doc_str[] =
    "<html><body>"
    "<div id=\"clickdiv\" style=\"text-align: center; background: red; font-size: 32\">click here</div>"
    "</body></html>";

static const char readystate_doc_str[] =
    "<html><body><iframe id=\"iframe\"></iframe></body></html>";

static const char img_doc_str[] =
    "<html><body><img id=\"imgid\"></img></body></html>";

static const char input_doc_str[] =
    "<html><body><input id=\"inputid\"></input></body></html>";    

static const char iframe_doc_str[] =
    "<html><body><iframe id=\"ifr\">Testing</iframe></body></html>";

static const char form_doc_str[] =
    "<html><body><form id=\"formid\" method=\"post\" action=\"about:blank\">"
    "<input type=\"text\" value=\"test\" name=\"i\"/>"
    "<input type=\"submit\" id=\"submitid\" />"
    "</form></body></html>";

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

static BSTR a2bstr(const char *str)
{
    BSTR ret;
    int len;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = SysAllocStringLen(NULL, len-1);
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

static BOOL iface_cmp(IUnknown *iface1, IUnknown *iface2)
{
    IUnknown *unk1, *unk2;

    if(iface1 == iface2)
        return TRUE;

    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk1);
    IUnknown_Release(unk1);
    IUnknown_QueryInterface(iface1, &IID_IUnknown, (void**)&unk2);
    IUnknown_Release(unk2);

    return unk1 == unk2;
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
        ok_(__FILE__,line) (IsEqualGUID(&type_attr->guid, diid), "unexpected guid %s\n",
                            wine_dbgstr_guid(&type_attr->guid));

        ITypeInfo_ReleaseTypeAttr(typeinfo, type_attr);
        ITypeInfo_Release(typeinfo);
    }

    IDispatchEx_Release(dispex);
}

#define get_doc3_iface(u) _get_doc3_iface(__LINE__,u)
static IHTMLDocument3 *_get_doc3_iface(unsigned line, IUnknown *unk)
{
    IHTMLDocument3 *doc3;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLDocument3, (void**)&doc3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLDocument3 iface: %08x\n", hres);

    return doc3;
}

#define get_elem_iface(u) _get_elem_iface(__LINE__,u)
static IHTMLElement *_get_elem_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement, (void**)&elem);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement iface: %08x\n", hres);

    return elem;
}

#define get_elem2_iface(u) _get_elem2_iface(__LINE__,u)
static IHTMLElement2 *_get_elem2_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement2 *elem2;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement2, (void**)&elem2);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement2 iface: %08x\n", hres);

    return elem2;
}

#define get_elem3_iface(u) _get_elem3_iface(__LINE__,u)
static IHTMLElement3 *_get_elem3_iface(unsigned line, IUnknown *unk)
{
    IHTMLElement3 *elem3;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLElement3, (void**)&elem3);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement3 iface: %08x\n", hres);

    return elem3;
}

#define get_iframe_iface(u) _get_iframe_iface(__LINE__,u)
static IHTMLIFrameElement *_get_iframe_iface(unsigned line, IUnknown *unk)
{
    IHTMLIFrameElement *iframe;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLIFrameElement, (void**)&iframe);
    ok_(__FILE__,line)(hres == S_OK, "QueryInterface(IID_IHTMLIFrameElement) failed: %08x\n", hres);

    return iframe;
}

#define doc_get_body(d) _doc_get_body(__LINE__,d)
static IHTMLElement *_doc_get_body(unsigned line, IHTMLDocument2 *doc)
{
    IHTMLElement *body = NULL;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &body);
    ok_(__FILE__,line) (hres == S_OK, "get_body failed: %08x\n", hres);
    ok_(__FILE__,line) (body != NULL, "body == NULL\n");

    return body;
}

#define get_elem_id(d,i) _get_elem_id(__LINE__,d,i)
static IHTMLElement *_get_elem_id(unsigned line, IHTMLDocument2 *doc, const char *id)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, (IUnknown*)doc);
    IHTMLElement *elem;
    BSTR str;
    HRESULT hres;

    str = a2bstr(id);
    hres = IHTMLDocument3_getElementById(doc3, str, &elem);
    SysFreeString(str);
    IHTMLDocument3_Release(doc3);
    ok_(__FILE__,line) (hres == S_OK, "getElementById failed: %08x\n", hres);

    return elem;
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

#define get_event_obj() _get_event_obj(__LINE__)
static IHTMLEventObj *_get_event_obj(unsigned line)
{
    IHTMLEventObj *event = NULL;
    HRESULT hres;

    hres = IHTMLWindow2_get_event(window, &event);
    ok_(__FILE__,line) (hres == S_OK, "get_event failed: %08x\n", hres);
    ok_(__FILE__,line) (event != NULL, "event = NULL\n");
    _test_disp(line, (IUnknown*)event, &DIID_DispCEventObj);

    return event;
}

#define elem_fire_event(a,b,c) _elem_fire_event(__LINE__,a,b,c)
static void _elem_fire_event(unsigned line, IUnknown *unk, const char *event, VARIANT *evobj)
{
    IHTMLElement3 *elem3 = _get_elem3_iface(line, unk);
    VARIANT_BOOL b;
    BSTR str;
    HRESULT hres;

    b = 100;
    str = a2bstr(event);
    hres = IHTMLElement3_fireEvent(elem3, str, evobj, &b);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "fireEvent failed: %08x\n", hres);
    ok_(__FILE__,line)(b == VARIANT_TRUE, "fireEvent returned %x\n", b);
}

#define test_event_args(a,b,c,d,e,f,g) _test_event_args(__LINE__,a,b,c,d,e,f,g)
static void _test_event_args(unsigned line, const IID *dispiid, DISPID id, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok_(__FILE__,line) (id == DISPID_VALUE, "id = %d\n", id);
    ok_(__FILE__,line) (wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok_(__FILE__,line) (pdp != NULL, "pdp == NULL\n");
    ok_(__FILE__,line) (pdp->cArgs == 1, "pdp->cArgs = %d\n", pdp->cArgs);
    ok_(__FILE__,line) (pdp->cNamedArgs == 1, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok_(__FILE__,line) (pdp->rgdispidNamedArgs[0] == DISPID_THIS, "pdp->rgdispidNamedArgs[0] = %d\n",
                        pdp->rgdispidNamedArgs[0]);
    ok_(__FILE__,line) (V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(rgvarg) = %d\n", V_VT(pdp->rgvarg));
    ok_(__FILE__,line) (pvarRes != NULL, "pvarRes == NULL\n");
    ok_(__FILE__,line) (pei != NULL, "pei == NULL");
    ok_(__FILE__,line) (!pspCaller, "pspCaller != NULL\n");

    if(dispiid)
        _test_disp(line, (IUnknown*)V_DISPATCH(pdp->rgvarg), dispiid);
}

#define test_attached_event_args(a,b,c,d,e) _test_attached_event_args(__LINE__,a,b,c,d,e)
static void _test_attached_event_args(unsigned line, DISPID id, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei)
{
    IHTMLEventObj *event;

    ok(id == DISPID_VALUE, "id = %d\n", id);
    ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok(pdp != NULL, "pDispParams == NULL\n");
    ok(pdp->cArgs == 1, "pdp->cArgs = %d\n", pdp->cArgs);
    ok(!pdp->cNamedArgs, "pdp->cNamedArgs = %d\n", pdp->cNamedArgs);
    ok(!pdp->rgdispidNamedArgs, "pdp->rgdispidNamedArgs = %p\n", pdp->rgdispidNamedArgs);
    ok(pdp->rgvarg != NULL, "rgvarg = NULL\n");
    ok(pvarRes != NULL, "pvarRes = NULL\n");
    ok(pei != NULL, "pei = NULL\n");
    ok(V_VT(pvarRes) == VT_EMPTY, "V_VT(pVarRes) = %d\n", V_VT(pvarRes));
    ok(V_VT(pdp->rgvarg) == VT_DISPATCH, "V_VT(pdp->rgvarg) = %d\n", V_VT(pdp->rgvarg));
    ok(V_DISPATCH(pdp->rgvarg) != NULL, "V_DISPATCH(pdp->rgvarg) = %p\n", V_DISPATCH(pdp->rgvarg));

    event = _get_event_obj(line);
    ok(iface_cmp((IUnknown*)event, (IUnknown*)V_DISPATCH(pdp->rgvarg)), "event != arg0\n");
    IHTMLEventObj_Release(event);
}

#define get_event_src() _get_event_src(__LINE__)
static IHTMLElement *_get_event_src(unsigned line)
{
    IHTMLEventObj *event = _get_event_obj(line);
    IHTMLElement *src_elem = NULL;
    HRESULT hres;

    hres = IHTMLEventObj_get_srcElement(event, &src_elem);
    IHTMLEventObj_Release(event);
    ok_(__FILE__,line) (hres == S_OK, "get_srcElement failed: %08x\n", hres);

    return src_elem;
}

#define test_event_src(t) _test_event_src(__LINE__,t)
static void _test_event_src(unsigned line, const char *src_tag)
{
    IHTMLElement *src_elem = _get_event_src(line);

    if(src_tag) {
        ok_(__FILE__,line) (src_elem != NULL, "src_elem = NULL\n");
        _test_elem_tag(line, (IUnknown*)src_elem, src_tag);
        IHTMLElement_Release(src_elem);
    }else {
        ok_(__FILE__,line) (!src_elem, "src_elem != NULL\n");
    }
}

static void _test_event_altkey(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_altKey(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_altKey failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exval, "altKey = %x, expected %x\n", b, exval);
}

static void _test_event_ctrlkey(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_ctrlKey(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_ctrlKey failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exval, "ctrlKey = %x, expected %x\n", b, exval);
}

static void _test_event_shiftkey(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_shiftKey(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_shiftKey failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exval, "shiftKey = %x, expected %x\n", b, exval);
}

#define test_event_cancelbubble(a,b) _test_event_cancelbubble(__LINE__,a,b)
static void _test_event_cancelbubble(unsigned line, IHTMLEventObj *event, VARIANT_BOOL exval)
{
    VARIANT_BOOL b;
    HRESULT hres;

    hres = IHTMLEventObj_get_cancelBubble(event, &b);
    ok_(__FILE__,line)(hres == S_OK, "get_cancelBubble failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exval, "cancelBubble = %x, expected %x\n", b, exval);
}

static void _test_event_fromelem(unsigned line, IHTMLEventObj *event, const char *from_tag)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLEventObj_get_fromElement(event, &elem);
    ok_(__FILE__,line)(hres == S_OK, "get_fromElement failed: %08x\n", hres);
    if(from_tag)
        _test_elem_tag(line, (IUnknown*)elem, from_tag);
    else
        ok_(__FILE__,line)(elem == NULL, "fromElement != NULL\n");
    if(elem)
        IHTMLElement_Release(elem);
}

static void _test_event_toelem(unsigned line, IHTMLEventObj *event, const char *to_tag)
{
    IHTMLElement *elem;
    HRESULT hres;

    hres = IHTMLEventObj_get_toElement(event, &elem);
    ok_(__FILE__,line)(hres == S_OK, "get_toElement failed: %08x\n", hres);
    if(to_tag)
        _test_elem_tag(line, (IUnknown*)elem, to_tag);
    else
        ok_(__FILE__,line)(elem == NULL, "toElement != NULL\n");
    if(elem)
        IHTMLElement_Release(elem);
}

static void _test_event_keycode(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_keyCode(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_keyCode failed: %08x\n", hres);
    ok_(__FILE__,line)(l == exl, "keyCode = %x, expected %x\n", l, exl);
}

static void _test_event_button(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_button(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_button failed: %08x\n", hres);
    ok_(__FILE__,line)(l == exl, "button = %x, expected %x\n", l, exl);
}

static void _test_event_reason(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_reason(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_reason failed: %08x\n", hres);
    ok_(__FILE__,line)(l == exl, "reason = %x, expected %x\n", l, exl);
}

static void _test_event_x(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_x(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_x failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "x = %d, expected %d\n", l, exl);
}

static void _test_event_y(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_y(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_y failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "y = %d, expected %d\n", l, exl);
}

static void _test_event_clientx(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_clientX(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_clientX failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "clientX = %d, expected %d\n", l, exl);
}

static void _test_event_clienty(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_clientY(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_clientY failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "clientY = %d, expected %d\n", l, exl);
}

static void _test_event_offsetx(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_offsetX(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_offsetX failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "offsetX = %d, expected %d\n", l, exl);
}

static void _test_event_offsety(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_offsetY(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_offsetY failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "offsetY = %d, expected %d\n", l, exl);
}

static void _test_event_screenx(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_screenX(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_screenX failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value */
        ok_(__FILE__,line)(l == exl, "screenX = %d, expected %d\n", l, exl);
}

static void _test_event_screeny(unsigned line, IHTMLEventObj *event, LONG exl)
{
    LONG l;
    HRESULT hres;

    hres = IHTMLEventObj_get_screenY(event, &l);
    ok_(__FILE__,line)(hres == S_OK, "get_screenY failed: %08x\n", hres);
    if(exl != -10) /* don't test the exact value for -10 */
        ok_(__FILE__,line)(l == exl, "screenY = %d, expected %d\n", l, exl);
}

static void _test_event_type(unsigned line, IHTMLEventObj *event, const char *exstr)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLEventObj_get_type(event, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_type failed: %08x\n", hres);
    ok_(__FILE__,line)(!strcmp_wa(str, exstr), "type = %s, expected %s\n", wine_dbgstr_w(str), exstr);
}

static void _test_event_qualifier(unsigned line, IHTMLEventObj *event, const char *exstr)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLEventObj_get_qualifier(event, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_qualifier failed: %08x\n", hres);
    if(exstr)
        ok_(__FILE__,line)(!strcmp_wa(str, exstr), "qualifier = %s, expected %s\n", wine_dbgstr_w(str), exstr);
    else
        ok_(__FILE__,line)(!str, "qualifier != NULL\n");
}

static void _test_event_srcfilter(unsigned line, IHTMLEventObj *event)
{
    IDispatch *disp;
    HRESULT hres;

    hres = IHTMLEventObj_get_srcFilter(event, &disp);
    ok_(__FILE__,line)(hres == S_OK, "get_srcFilter failed: %08x\n", hres);
    ok_(__FILE__,line)(!disp, "srcFilter != NULL\n");
}

#define test_event_obj(t,x) _test_event_obj(__LINE__,t,x)
static void _test_event_obj(unsigned line, const char *type, const xy_test_t *xy)
{
    IHTMLEventObj *event = _get_event_obj(line);
    VARIANT v;
    HRESULT hres;

    _test_event_altkey(line, event, VARIANT_FALSE);
    _test_event_ctrlkey(line, event, VARIANT_FALSE);
    _test_event_shiftkey(line, event, VARIANT_FALSE);
    _test_event_cancelbubble(line, event, VARIANT_FALSE);
    _test_event_fromelem(line, event, NULL);
    _test_event_toelem(line, event, NULL);
    _test_event_keycode(line, event, 0);
    _test_event_button(line, event, 0);
    _test_event_type(line, event, type);
    _test_event_qualifier(line, event, NULL);
    _test_event_reason(line, event, 0);
    _test_event_srcfilter(line, event);
    _test_event_x(line, event, xy->x);
    _test_event_y(line, event, xy->y);
    _test_event_clientx(line, event, -10);
    _test_event_clienty(line, event, -10);
    _test_event_offsetx(line, event, xy->offset_x);
    _test_event_offsety(line, event, xy->offset_y);
    _test_event_screenx(line, event, -10);
    _test_event_screeny(line, event, -10);

    V_VT(&v) = VT_NULL;
    hres = IHTMLEventObj_get_returnValue(event, &v);
    ok_(__FILE__,line)(hres == S_OK, "get_returnValue failed: %08x\n", hres);
    ok_(__FILE__,line)(V_VT(&v) == VT_EMPTY, "V_VT(returnValue) = %d\n", V_VT(&v));

    IHTMLEventObj_Release(event);
}

#define elem_attach_event(a,b,c) _elem_attach_event(__LINE__,a,b,c)
static void _elem_attach_event(unsigned line, IUnknown *unk, const char *namea, IDispatch *disp)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    VARIANT_BOOL res;
    BSTR name;
    HRESULT hres;

    name = a2bstr(namea);
    hres = IHTMLElement2_attachEvent(elem, name, disp, &res);
    IHTMLElement2_Release(elem);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "attachEvent failed: %08x\n", hres);
    ok_(__FILE__,line)(res == VARIANT_TRUE, "attachEvent returned %x\n", res);
}

#define elem_detach_event(a,b,c) _elem_detach_event(__LINE__,a,b,c)
static void _elem_detach_event(unsigned line, IUnknown *unk, const char *namea, IDispatch *disp)
{
    IHTMLElement2 *elem = _get_elem2_iface(line, unk);
    BSTR name;
    HRESULT hres;

    name = a2bstr(namea);
    hres = IHTMLElement2_detachEvent(elem, name, disp);
    IHTMLElement2_Release(elem);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "detachEvent failed: %08x\n", hres);
}

#define doc_attach_event(a,b,c) _doc_attach_event(__LINE__,a,b,c)
static void _doc_attach_event(unsigned line, IHTMLDocument2 *doc, const char *namea, IDispatch *disp)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, (IUnknown*)doc);
    VARIANT_BOOL res;
    BSTR name;
    HRESULT hres;

    name = a2bstr(namea);
    hres = IHTMLDocument3_attachEvent(doc3, name, disp, &res);
    IHTMLDocument3_Release(doc3);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "attachEvent failed: %08x\n", hres);
    ok_(__FILE__,line)(res == VARIANT_TRUE, "attachEvent returned %x\n", res);
}

#define doc_detach_event(a,b,c) _doc_detach_event(__LINE__,a,b,c)
static void _doc_detach_event(unsigned line, IHTMLDocument2 *doc, const char *namea, IDispatch *disp)
{
    IHTMLDocument3 *doc3 = _get_doc3_iface(line, (IUnknown*)doc);
    BSTR name;
    HRESULT hres;

    name = a2bstr(namea);
    hres = IHTMLDocument3_detachEvent(doc3, name, disp);
    IHTMLDocument3_Release(doc3);
    SysFreeString(name);
    ok_(__FILE__,line)(hres == S_OK, "detachEvent failed: %08x\n", hres);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)
       || IsEqualGUID(riid, &IID_IDispatchEx))
        *ppv = iface;
    else {
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }

    return S_OK;
}

static HRESULT WINAPI Dispatch_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown)
       || IsEqualGUID(riid, &IID_IDispatch)) {
        *ppv = iface;
    }else if(IsEqualGUID(riid, &IID_IDispatchEx)) {
        return E_NOINTERFACE;
    }else {
        ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }

    return S_OK;
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    return 2;
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    return 1;
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    ok(0, "unexpected call %s %x\n", wine_dbgstr_w(bstrName), grfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

#define EVENT_HANDLER_FUNC_OBJ(event) \
    static IDispatchExVtbl event ## FuncVtbl = { \
        DispatchEx_QueryInterface, \
        DispatchEx_AddRef, \
        DispatchEx_Release, \
        DispatchEx_GetTypeInfoCount, \
        DispatchEx_GetTypeInfo, \
        DispatchEx_GetIDsOfNames, \
        DispatchEx_Invoke, \
        DispatchEx_GetDispID, \
        event, \
        DispatchEx_DeleteMemberByName, \
        DispatchEx_DeleteMemberByDispID, \
        DispatchEx_GetMemberProperties, \
        DispatchEx_GetMemberName, \
        DispatchEx_GetNextDispID, \
        DispatchEx_GetNameSpaceParent \
    }; \
    static IDispatchEx event ## _obj = { &event ## FuncVtbl };

static HRESULT WINAPI document_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLDocument3 *doc3;
    CHECK_EXPECT(document_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    doc3 = get_doc3_iface((IUnknown*)V_DISPATCH(pdp->rgvarg));
    IHTMLDocument3_Release(doc3);
    test_event_src("DIV");
    test_event_obj("click", &no_xy);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(document_onclick);

static HRESULT WINAPI div_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onclick);

static HRESULT WINAPI div_onclick_attached(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(div_onclick_attached);

    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src("DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(div_onclick_attached);

static HRESULT WINAPI doc_onclick_attached(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(doc_onclick_attached);

    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src("DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(doc_onclick_attached);

static HRESULT WINAPI body_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(body_onclick);
    test_event_args(&DIID_DispHTMLBody, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("DIV");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(body_onclick);

static HRESULT WINAPI img_onload(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(img_onload);
    test_event_args(&DIID_DispHTMLImg, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("IMG");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(img_onload);

static HRESULT WINAPI img_onerror(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(img_onerror);
    test_event_args(&DIID_DispHTMLImg, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("IMG");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(img_onerror);

static HRESULT WINAPI input_onfocus(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(input_onfocus);
    test_event_args(&DIID_DispHTMLInputElement, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("INPUT");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(input_onfocus);

static HRESULT WINAPI input_onblur(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(input_onblur);
    test_event_args(&DIID_DispHTMLInputElement, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("INPUT");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(input_onblur);

static HRESULT WINAPI form_onsubmit(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(form_onsubmit);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("FORM");

    V_VT(pvarRes) = VT_BOOL;
    V_BOOL(pvarRes) = VARIANT_FALSE;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(form_onsubmit);

static HRESULT WINAPI form_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(form_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);

    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(form_onclick);

static HRESULT WINAPI submit_onclick(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(submit_onclick);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("INPUT");

    V_VT(pvarRes) = VT_BOOL;
    V_BOOL(pvarRes) = VARIANT_FALSE;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick);

static HRESULT WINAPI iframe_onload(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(iframe_onload);
    test_event_args(&DIID_DispHTMLIFrame, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("IFRAME");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(iframe_onload);

static HRESULT WINAPI submit_onclick_attached(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    CHECK_EXPECT(submit_onclick_attached);
    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src("INPUT");

    V_VT(pvarRes) = VT_BOOL;
    V_BOOL(pvarRes) = VARIANT_FALSE;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_attached);

static HRESULT WINAPI submit_onclick_attached_check_cancel(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event;
    HRESULT hres;

    CHECK_EXPECT(submit_onclick_attached_check_cancel);
    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);
    test_event_src("INPUT");

    event = NULL;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08x\n", hres);
    ok(event != NULL, "event == NULL\n");

    test_event_cancelbubble(event, VARIANT_TRUE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_attached_check_cancel);

static VARIANT onclick_retval, onclick_event_retval;

static HRESULT WINAPI submit_onclick_setret(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event;
    HRESULT hres;

    CHECK_EXPECT(submit_onclick_setret);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("INPUT");

    event = NULL;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08x\n", hres);
    ok(event != NULL, "event == NULL\n");

    hres = IHTMLEventObj_put_returnValue(event, onclick_event_retval);
    ok(hres == S_OK, "put_returnValue failed: %08x\n", hres);
    IHTMLEventObj_Release(event);

    *pvarRes = onclick_retval;
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_setret);

static HRESULT WINAPI submit_onclick_cancel(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event;
    HRESULT hres;

    CHECK_EXPECT(submit_onclick_cancel);
    test_event_args(NULL, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("INPUT");

    event = NULL;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08x\n", hres);
    ok(event != NULL, "event == NULL\n");

    test_event_cancelbubble(event, VARIANT_FALSE);

    hres = IHTMLEventObj_put_cancelBubble(event, VARIANT_TRUE);
    ok(hres == S_OK, "put_returnValue failed: %08x\n", hres);
    IHTMLEventObj_Release(event);

    test_event_cancelbubble(event, VARIANT_TRUE);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(submit_onclick_cancel);

static HRESULT WINAPI iframedoc_onreadystatechange(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLEventObj *event = NULL;
    HRESULT hres;

    CHECK_EXPECT2(iframedoc_onreadystatechange);
    test_event_args(&DIID_DispHTMLDocument, id, wFlags, pdp, pvarRes, pei, pspCaller);

    event = (void*)0xdeadbeef;
    hres = IHTMLWindow2_get_event(window, &event);
    ok(hres == S_OK, "get_event failed: %08x\n", hres);
    ok(!event, "event = %p\n", event);

    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(iframedoc_onreadystatechange);

static HRESULT WINAPI iframe_onreadystatechange(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    IHTMLWindow2 *iframe_window;
    IHTMLDocument2 *iframe_doc;
    IHTMLFrameBase2 *iframe;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str, str2;
    HRESULT hres;

    test_event_args(&DIID_DispHTMLIFrame, id, wFlags, pdp, pvarRes, pei, pspCaller);
    test_event_src("IFRAME");

    elem = get_event_src();
    elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_readyState(elem2, &v);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(readyState) = %d\n", V_VT(&v));

    hres = IHTMLElement2_QueryInterface(elem2, &IID_IHTMLFrameBase2, (void**)&iframe);
    IHTMLElement2_Release(elem2);
    ok(hres == S_OK, "Could not get IHTMLFrameBase2 iface: %08x\n", hres);

    str = NULL;
    hres = IHTMLFrameBase2_get_readyState(iframe, &str);
    ok(hres == S_OK, "get_readyState failed: %08x\n", hres);
    ok(str != NULL, "readyState == NULL\n");
    ok(!lstrcmpW(str, V_BSTR(&v)), "ready states differ\n");
    VariantClear(&v);

    hres = IHTMLFrameBase2_get_contentWindow(iframe, &iframe_window);
    ok(hres == S_OK, "get_contentDocument failed: %08x\n", hres);

    hres = IHTMLWindow2_get_document(iframe_window, &iframe_doc);
    IHTMLWindow2_Release(iframe_window);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);

    hres = IHTMLDocument2_get_readyState(iframe_doc, &str2);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);
    ok(!lstrcmpW(str, str2), "unexpected document readyState %s\n", wine_dbgstr_w(str2));
    SysFreeString(str2);

    if(!strcmp_wa(str, "loading")) {
        CHECK_EXPECT(iframe_onreadystatechange_loading);

        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = (IDispatch*)&iframedoc_onreadystatechange_obj;
        hres = IHTMLDocument2_put_onreadystatechange(iframe_doc, v);
        ok(hres == S_OK, "put_onreadystatechange: %08x\n", hres);
    }else if(!strcmp_wa(str, "interactive"))
        CHECK_EXPECT(iframe_onreadystatechange_interactive);
    else if(!strcmp_wa(str, "complete"))
        CHECK_EXPECT(iframe_onreadystatechange_complete);
    else
        ok(0, "unexpected state %s\n", wine_dbgstr_w(str));

    SysFreeString(str);
    IHTMLDocument2_Release(iframe_doc);
    IHTMLFrameBase2_Release(iframe);
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(iframe_onreadystatechange);

static HRESULT WINAPI nocall(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    ok(0, "unexpected call\n");
    return S_OK;
}

EVENT_HANDLER_FUNC_OBJ(nocall);

#define CONNECTION_POINT_OBJ(cpname, diid) \
    static HRESULT WINAPI cpname ## _QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv) \
    { \
        *ppv = NULL; \
        if(IsEqualGUID(riid, &IID_IUnknown) \
           || IsEqualGUID(riid, &IID_IDispatch) \
           || IsEqualGUID(riid, &diid)) \
            *ppv = iface; \
        else { \
            ok(0, "unexpected riid %s\n", wine_dbgstr_guid(riid)); \
            return E_NOINTERFACE; \
        } \
        return S_OK; \
    } \
    static IDispatchExVtbl cpname ## Vtbl = { \
        cpname ## _QueryInterface, \
        DispatchEx_AddRef,  \
        DispatchEx_Release, \
        DispatchEx_GetTypeInfoCount, \
        DispatchEx_GetTypeInfo, \
        DispatchEx_GetIDsOfNames, \
        cpname, \
        DispatchEx_GetDispID, \
        DispatchEx_InvokeEx, \
        DispatchEx_DeleteMemberByName, \
        DispatchEx_DeleteMemberByDispID, \
        DispatchEx_GetMemberProperties, \
        DispatchEx_GetMemberName, \
        DispatchEx_GetNextDispID, \
        DispatchEx_GetNameSpaceParent \
    }; \
    static IDispatchEx cpname ## _obj = { &cpname ## Vtbl }

#define test_cp_args(a,b,c,d,e,f) _test_cp_args(__LINE__,a,b,c,d,e,f)
static void _test_cp_args(unsigned line, REFIID riid, WORD flags, DISPPARAMS *dp, VARIANT *vres, EXCEPINFO *ei, UINT *argerr)
{
    ok_(__FILE__,line)(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok_(__FILE__,line)(flags == DISPATCH_METHOD, "flags = %x\n", flags);
    ok_(__FILE__,line)(dp != NULL, "dp == NULL\n");
    ok_(__FILE__,line)(!dp->cArgs, "dp->cArgs = %d\n", dp->cArgs);
    ok_(__FILE__,line)(!dp->rgvarg, "dp->rgvarg = %p\n", dp->rgvarg);
    ok_(__FILE__,line)(!dp->cNamedArgs, "dp->cNamedArgs = %d\n", dp->cNamedArgs);
    ok_(__FILE__,line)(!dp->rgdispidNamedArgs, "dp->rgdispidNamedArgs = %p\n", dp->rgdispidNamedArgs);
    ok_(__FILE__,line)(vres != NULL, "vres == NULL\n");
    ok_(__FILE__,line)(V_VT(vres) == VT_EMPTY, "V_VT(vres) = %d\n", V_VT(vres));
    ok_(__FILE__,line)(ei != NULL, "ei == NULL\n");
    ok_(__FILE__,line)(argerr != NULL, "argerr == NULL\n");
}

#define test_cp_eventarg(a,b,c,d,e,f) _test_cp_eventarg(__LINE__,a,b,c,d,e,f)
static void _test_cp_eventarg(unsigned line, REFIID riid, WORD flags, DISPPARAMS *dp, VARIANT *vres, EXCEPINFO *ei, UINT *argerr)
{
    IHTMLEventObj *event;

    ok_(__FILE__,line)(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok_(__FILE__,line)(flags == DISPATCH_METHOD, "flags = %x\n", flags);
    ok_(__FILE__,line)(dp != NULL, "dp == NULL\n");
    ok_(__FILE__,line)(dp->cArgs == 1, "dp->cArgs = %d\n", dp->cArgs);
    ok_(__FILE__,line)(dp->rgvarg != NULL, "dp->rgvarg = %p\n", dp->rgvarg);
    ok_(__FILE__,line)(!dp->cNamedArgs, "dp->cNamedArgs = %d\n", dp->cNamedArgs);
    ok_(__FILE__,line)(!dp->rgdispidNamedArgs, "dp->rgdispidNamedArgs = %p\n", dp->rgdispidNamedArgs);
    ok_(__FILE__,line)(vres != NULL, "vres == NULL\n");
    ok_(__FILE__,line)(V_VT(vres) == VT_EMPTY, "V_VT(vres) = %d\n", V_VT(vres));
    ok_(__FILE__,line)(ei != NULL, "ei == NULL\n");
    ok_(__FILE__,line)(argerr != NULL, "argerr == NULL\n");

    ok(V_VT(dp->rgvarg) == VT_DISPATCH, "V_VT(dp->rgvarg) = %d\n", V_VT(dp->rgvarg));
    ok(V_DISPATCH(dp->rgvarg) != NULL, "V_DISPATCH(dp->rgvarg) = %p\n", V_DISPATCH(dp->rgvarg));

    event = _get_event_obj(line);
    ok(iface_cmp((IUnknown*)event, (IUnknown*)V_DISPATCH(dp->rgvarg)), "event != arg0\n");
    IHTMLEventObj_Release(event);
}

static HRESULT WINAPI doccp(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
                            VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(doccp_onclick);
        test_cp_args(riid, wFlags, pdp, pVarResult, pei, puArgErr);
        break;
    default:
        ok(0, "unexpected call %d\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(doccp, DIID_HTMLDocumentEvents);

static HRESULT WINAPI doccp_onclick_cancel(IDispatchEx *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(doccp_onclick_cancel);
        test_cp_args(riid, wFlags, pdp, pVarResult, pei, puArgErr);
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = VARIANT_FALSE;
        break;
    default:
        ok(0, "unexpected call %d\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(doccp_onclick_cancel, DIID_HTMLDocumentEvents);

static HRESULT WINAPI elem2_cp(IDispatchEx *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pdp, VARIANT *pVarResult, EXCEPINFO *pei, UINT *puArgErr)
{
    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT(elem2_cp_onclick);
        test_cp_eventarg(riid, wFlags, pdp, pVarResult, pei, puArgErr);
        break;
    default:
        ok(0, "unexpected call %d\n", dispIdMember);
        return E_NOTIMPL;
    }

    return S_OK;
}

CONNECTION_POINT_OBJ(elem2_cp, DIID_HTMLElementEvents2);

static HRESULT WINAPI timeoutFunc_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    CHECK_EXPECT(timeout);

    ok(dispIdMember == DISPID_VALUE, "dispIdMember = %d\n", dispIdMember);
    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(wFlags == DISPATCH_METHOD, "wFlags = %x\n", wFlags);
    ok(!lcid, "lcid = %x\n", lcid);
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(!pDispParams->cArgs, "pdp->cArgs = %d\n", pDispParams->cArgs);
    ok(!pDispParams->cNamedArgs, "pdp->cNamedArgs = %d\n", pDispParams->cNamedArgs);
    ok(!pDispParams->rgdispidNamedArgs, "pdp->rgdispidNamedArgs = %p\n", pDispParams->rgdispidNamedArgs);
    ok(!pDispParams->rgvarg, "rgvarg = %p\n", pDispParams->rgvarg);
    ok(pVarResult != NULL, "pVarResult = NULL\n");
    ok(pExcepInfo != NULL, "pExcepInfo = NULL\n");
    ok(!puArgErr, "puArgErr = %p\n", puArgErr);
    ok(V_VT(pVarResult) == VT_EMPTY, "V_VT(pVarResult) = %d\n", V_VT(pVarResult));

    return S_OK;
}

static const IDispatchExVtbl timeoutFuncVtbl = {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    timeoutFunc_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
};

static IDispatchEx timeoutFunc = { &timeoutFuncVtbl };

static HRESULT WINAPI div_onclick_disp_Invoke(IDispatchEx *iface, DISPID id,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, UINT *puArgErr)
{
    CHECK_EXPECT(div_onclick_disp);

    test_attached_event_args(id, wFlags, pdp, pvarRes, pei);

    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(!puArgErr, "puArgErr = %p\n", puArgErr);

    return S_OK;
}

static const IDispatchExVtbl div_onclick_dispVtbl = {
    Dispatch_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    div_onclick_disp_Invoke,
};

static IDispatchEx div_onclick_disp = { &div_onclick_dispVtbl };

static void pump_msgs(BOOL *b)
{
    MSG msg;

    if(b) {
        while(!*b && GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }else {
        while(!b && PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

static IConnectionPoint *get_cp(IUnknown *unk, REFIID riid)
{
    IConnectionPointContainer *cp_container;
    IConnectionPoint *cp;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IConnectionPointContainer, (void**)&cp_container);
    ok(hres == S_OK, "Could not get IConnectionPointContainer: %08x\n", hres);

    hres = IConnectionPointContainer_FindConnectionPoint(cp_container, riid, &cp);
    IConnectionPointContainer_Release(cp_container);
    ok(hres == S_OK, "FindConnectionPoint failed: %08x\n", hres);

    return cp;
}

static DWORD register_cp(IUnknown *unk, REFIID riid, IUnknown *sink)
{
    IConnectionPoint *cp;
    DWORD cookie;
    HRESULT hres;

    cp = get_cp(unk, riid);
    hres = IConnectionPoint_Advise(cp, sink, &cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Advise failed: %08x\n", hres);

    return cookie;
}

static void unregister_cp(IUnknown *unk, REFIID riid, DWORD cookie)
{
    IConnectionPoint *cp;
    HRESULT hres;

    cp = get_cp(unk, riid);
    hres = IConnectionPoint_Unadvise(cp, cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Unadvise failed: %08x\n", hres);
}

static HRESULT WINAPI EventDispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "Unexpected call\n");
    return E_NOINTERFACE;
}

static DWORD WINAPI EventDispatch_AddRef(IDispatch *iface)
{
    return 2;
}

static DWORD WINAPI EventDispatch_Release(IDispatch *iface)
{
    return 1;
}

static HRESULT WINAPI EventDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EventDispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EventDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ok(0, "Unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI EventDispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ok(IsEqualGUID(&IID_NULL, riid), "riid = %s\n", wine_dbgstr_guid(riid));
    ok(pDispParams != NULL, "pDispParams == NULL\n");
    ok(pExcepInfo != NULL, "pExcepInfo == NULL\n");
    ok(puArgErr != NULL, "puArgErr == NULL\n");
    ok(V_VT(pVarResult) == 0, "V_VT(pVarResult) = %d\n", V_VT(pVarResult));
    ok(wFlags == DISPATCH_METHOD, "wFlags = %d\n", wFlags);

    switch(dispIdMember) {
    case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
        CHECK_EXPECT2(invoke_onclick);
        break;
    case DISPID_HTMLDOCUMENTEVENTS2_ONPROPERTYCHANGE:
    case DISPID_HTMLDOCUMENTEVENTS2_ONREADYSTATECHANGE:
    case 1027:
    case 1034:
    case 1037:
    case 1044:
    case 1045:
    case 1047:
    case 1048:
    case 1049:
        break; /* TODO */
    default:
        ok(0, "Unexpected call: %d\n", dispIdMember);
    }

    return S_OK;
}

static const IDispatchVtbl EventDispatchVtbl = {
    EventDispatch_QueryInterface,
    EventDispatch_AddRef,
    EventDispatch_Release,
    EventDispatch_GetTypeInfoCount,
    EventDispatch_GetTypeInfo,
    EventDispatch_GetIDsOfNames,
    EventDispatch_Invoke
};

static IDispatch EventDispatch = { &EventDispatchVtbl };

static void test_onclick(IHTMLDocument2 *doc)
{
    DWORD cp_cookie, elem2_cp_cookie;
    IHTMLElement *div, *body;
    VARIANT v;
    HRESULT hres;

    register_cp((IUnknown*)doc, &IID_IDispatch, (IUnknown*)&EventDispatch);

    div = get_elem_id(doc, "clickdiv");

    elem_attach_event((IUnknown*)div, "abcde", (IDispatch*)&nocall_obj);
    elem_attach_event((IUnknown*)div, "onclick", (IDispatch*)&div_onclick_attached_obj);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08x\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onclick) = %d\n", V_VT(&v));

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == E_NOTIMPL, "put_onclick failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("function();");
    hres = IHTMLElement_put_onclick(div, v);
    todo_wine ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    if(hres == S_OK) {
        V_VT(&v) = VT_EMPTY;
        hres = IHTMLElement_get_onclick(div, &v);
        ok(hres == S_OK, "get_onclick failed: %08x\n", hres);
        ok(V_VT(&v) == VT_BSTR, "V_VT(onclick) = %d\n", V_VT(&v));
        ok(!strcmp_wa(V_BSTR(&v), "function();"), "V_BSTR(onclick) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    }
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&div_onclick_obj;
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLElement_put_ondblclick(div, v);
    ok(hres == S_OK, "put_ondblclick failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onclick) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&div_onclick_obj, "V_DISPATCH(onclick) != onclickFunc\n");
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLDocument2_get_onclick(doc, &v);
    ok(hres == S_OK, "get_onclick failed: %08x\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onclick) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&document_onclick_obj;
    hres = IHTMLDocument2_put_onclick(doc, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLDocument2_get_onclick(doc, &v);
    ok(hres == S_OK, "get_onclick failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onclick) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&document_onclick_obj, "V_DISPATCH(onclick) != onclickFunc\n");
    VariantClear(&v);

    body = doc_get_body(doc);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&body_onclick_obj;
    hres = IHTMLElement_put_onclick(body, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    if(winetest_interactive) {
        SET_EXPECT(div_onclick);
        SET_EXPECT(div_onclick_attached);
        SET_EXPECT(body_onclick);
        SET_EXPECT(document_onclick);
        SET_EXPECT(invoke_onclick);
        pump_msgs(&called_document_onclick);
        CHECK_CALLED(div_onclick);
        CHECK_CALLED(div_onclick_attached);
        CHECK_CALLED(body_onclick);
        CHECK_CALLED(document_onclick);
        CHECK_CALLED(invoke_onclick);
    }

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(invoke_onclick);

    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08x\n", hres);

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(invoke_onclick);

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(invoke_onclick);

    V_VT(&v) = VT_EMPTY;
    elem_fire_event((IUnknown*)div, "onclick", &v);

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(invoke_onclick);

    cp_cookie = register_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, (IUnknown*)&doccp_obj);
    elem_attach_event((IUnknown*)div, "onclick", (IDispatch*)&div_onclick_disp);
    doc_attach_event(doc, "onclick", (IDispatch*)&doc_onclick_attached_obj);

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_disp);
    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(doc_onclick_attached);
    SET_EXPECT(doccp_onclick);
    SET_EXPECT(invoke_onclick);

    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08x\n", hres);

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_disp);
    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(doc_onclick_attached);
    CHECK_CALLED(doccp_onclick);
    CHECK_CALLED(invoke_onclick);

    elem2_cp_cookie = register_cp((IUnknown*)div, &DIID_HTMLElementEvents2, (IUnknown*)&elem2_cp_obj);

    SET_EXPECT(div_onclick);
    SET_EXPECT(div_onclick_disp);
    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(elem2_cp_onclick);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(doc_onclick_attached);
    SET_EXPECT(doccp_onclick);
    SET_EXPECT(invoke_onclick);

    trace("click >>>\n");
    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    trace("click <<<\n");

    CHECK_CALLED(div_onclick);
    CHECK_CALLED(div_onclick_disp);
    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(elem2_cp_onclick);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(doc_onclick_attached);
    CHECK_CALLED(doccp_onclick);
    CHECK_CALLED(invoke_onclick);

    unregister_cp((IUnknown*)div, &DIID_HTMLElementEvents2, elem2_cp_cookie);
    unregister_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, cp_cookie);

    V_VT(&v) = VT_NULL;
    hres = IHTMLElement_put_onclick(div, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    hres = IHTMLElement_get_onclick(div, &v);
    ok(hres == S_OK, "get_onclick failed: %08x\n", hres);
    ok(V_VT(&v) == VT_NULL, "get_onclick returned vt %d\n", V_VT(&v));

    elem_detach_event((IUnknown*)div, "onclick", (IDispatch*)&div_onclick_disp);
    elem_detach_event((IUnknown*)div, "onclick", (IDispatch*)&div_onclick_disp);
    elem_detach_event((IUnknown*)div, "test", (IDispatch*)&div_onclick_disp);
    doc_detach_event(doc, "onclick", (IDispatch*)&doc_onclick_attached_obj);

    SET_EXPECT(div_onclick_attached);
    SET_EXPECT(body_onclick);
    SET_EXPECT(document_onclick);
    SET_EXPECT(invoke_onclick);

    hres = IHTMLElement_click(div);
    ok(hres == S_OK, "click failed: %08x\n", hres);

    CHECK_CALLED(div_onclick_attached);
    CHECK_CALLED(body_onclick);
    CHECK_CALLED(document_onclick);
    CHECK_CALLED(invoke_onclick);

    IHTMLElement_Release(div);
    IHTMLElement_Release(body);
}

static void test_onreadystatechange(IHTMLDocument2 *doc)
{
    IHTMLFrameBase *iframe;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    elem = get_elem_id(doc, "iframe");
    elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onreadystatechange(elem2, &v);
    ok(hres == S_OK, "get_onreadystatechange failed: %08x\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onreadystatechange) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&iframe_onreadystatechange_obj;
    hres = IHTMLElement2_put_onreadystatechange(elem2, v);
    ok(hres == S_OK, "put_onreadystatechange failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onreadystatechange(elem2, &v);
    ok(hres == S_OK, "get_onreadystatechange failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onreadystatechange) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&iframe_onreadystatechange_obj, "unexpected onreadystatechange value\n");

    hres = IHTMLElement2_QueryInterface(elem2, &IID_IHTMLFrameBase, (void**)&iframe);
    IHTMLElement2_Release(elem2);
    ok(hres == S_OK, "Could not get IHTMLFrameBase iface: %08x\n", hres);

    hres = IHTMLFrameBase_put_src(iframe, (str = a2bstr("about:blank")));
    SysFreeString(str);
    ok(hres == S_OK, "put_src failed: %08x\n", hres);

    SET_EXPECT(iframe_onreadystatechange_loading);
    SET_EXPECT(iframedoc_onreadystatechange);
    SET_EXPECT(iframe_onreadystatechange_interactive);
    SET_EXPECT(iframe_onreadystatechange_complete);
    pump_msgs(&called_iframe_onreadystatechange_complete);
    CHECK_CALLED(iframe_onreadystatechange_loading);
    CHECK_CALLED(iframedoc_onreadystatechange);
    CHECK_CALLED(iframe_onreadystatechange_interactive);
    CHECK_CALLED(iframe_onreadystatechange_complete);

    IHTMLFrameBase_Release(iframe);
}

static void test_imgload(IHTMLDocument2 *doc)
{
    IHTMLImgElement *img;
    IHTMLElement *elem;
    VARIANT v;
    BSTR str;
    HRESULT hres;

    elem = get_elem_id(doc, "imgid");
    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLImgElement, (void**)&img);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLImgElement iface: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onload(img, &v);
    ok(hres == S_OK, "get_onload failed: %08x\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onload) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&img_onload_obj;
    hres = IHTMLImgElement_put_onload(img, v);
    ok(hres == S_OK, "put_onload failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onload(img, &v);
    ok(hres == S_OK, "get_onload failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onload) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&img_onload_obj, "V_DISPATCH(onload) != onloadkFunc\n");
    VariantClear(&v);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&img_onerror_obj;
    hres = IHTMLImgElement_put_onerror(img, v);
    ok(hres == S_OK, "put_onerror failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLImgElement_get_onerror(img, &v);
    ok(hres == S_OK, "get_onerror failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onerror) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&img_onerror_obj, "V_DISPATCH(onerror) != onerrorFunc\n");
    VariantClear(&v);

    str = a2bstr("https://www.winehq.org/images/winehq_logo_text.png");
    hres = IHTMLImgElement_put_src(img, str);
    ok(hres == S_OK, "put_src failed: %08x\n", hres);
    SysFreeString(str);

    SET_EXPECT(img_onload);
    pump_msgs(&called_img_onload);
    CHECK_CALLED(img_onload);

    SET_EXPECT(img_onerror);

    str = a2bstr("about:blank");
    hres = IHTMLImgElement_put_src(img, str);
    ok(hres == S_OK, "put_src failed: %08x\n", hres);
    SysFreeString(str);

    pump_msgs(&called_img_onerror); /* FIXME: should not be needed */

    CHECK_CALLED(img_onerror);

    IHTMLImgElement_Release(img);
}

static void test_focus(IHTMLDocument2 *doc)
{
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    VARIANT v;
    HRESULT hres;

    elem = get_elem_id(doc, "inputid");
    elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLElement_Release(elem);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onfocus(elem2, &v);
    ok(hres == S_OK, "get_onfocus failed: %08x\n", hres);
    ok(V_VT(&v) == VT_NULL, "V_VT(onfocus) = %d\n", V_VT(&v));

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&input_onfocus_obj;
    hres = IHTMLElement2_put_onfocus(elem2, v);
    ok(hres == S_OK, "put_onfocus failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLElement2_get_onfocus(elem2, &v);
    ok(hres == S_OK, "get_onfocus failed: %08x\n", hres);
    ok(V_VT(&v) == VT_DISPATCH, "V_VT(onfocus) = %d\n", V_VT(&v));
    ok(V_DISPATCH(&v) == (IDispatch*)&input_onfocus_obj, "V_DISPATCH(onfocus) != onfocusFunc\n");
    VariantClear(&v);

    if(!winetest_interactive)
        ShowWindow(container_hwnd, SW_SHOW);

    SetFocus(NULL);
    ok(!IsChild(container_hwnd, GetFocus()), "focus belongs to document window\n");

    hres = IHTMLWindow2_focus(window);
    ok(hres == S_OK, "focus failed: %08x\n", hres);

    ok(IsChild(container_hwnd, GetFocus()), "focus does not belong to document window\n");
    pump_msgs(NULL);

    SET_EXPECT(input_onfocus);
    hres = IHTMLElement2_focus(elem2);
    pump_msgs(NULL);
    CHECK_CALLED(input_onfocus);
    ok(hres == S_OK, "focus failed: %08x\n", hres);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&input_onblur_obj;
    hres = IHTMLElement2_put_onblur(elem2, v);
    ok(hres == S_OK, "put_onblur failed: %08x\n", hres);

    SET_EXPECT(input_onblur);
    hres = IHTMLElement2_blur(elem2);
    pump_msgs(NULL);
    CHECK_CALLED(input_onblur);
    ok(hres == S_OK, "blur failed: %08x\n", hres);

    if(!winetest_interactive)
        ShowWindow(container_hwnd, SW_HIDE);

    IHTMLElement2_Release(elem2);
}

static void test_submit(IHTMLDocument2 *doc)
{
    IHTMLElement *elem, *submit;
    IHTMLFormElement *form;
    VARIANT v;
    DWORD cp_cookie;
    HRESULT hres;

    elem = get_elem_id(doc, "formid");

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&form_onclick_obj;
    hres = IHTMLElement_put_onclick(elem, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLFormElement, (void**)&form);
    IHTMLElement_Release(elem);
    ok(hres == S_OK, "Could not get IHTMLFormElement iface: %08x\n", hres);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&form_onsubmit_obj;
    hres = IHTMLFormElement_put_onsubmit(form, v);
    ok(hres == S_OK, "put_onsubmit failed: %08x\n", hres);

    IHTMLFormElement_Release(form);

    submit = get_elem_id(doc, "submitid");

    SET_EXPECT(form_onclick);
    SET_EXPECT(form_onsubmit);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(form_onsubmit);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&submit_onclick_obj;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    SET_EXPECT(form_onclick);
    SET_EXPECT(submit_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(submit_onclick);

    elem_attach_event((IUnknown*)submit, "onclick", (IDispatch*)&submit_onclick_attached_obj);

    SET_EXPECT(form_onclick);
    SET_EXPECT(submit_onclick);
    SET_EXPECT(submit_onclick_attached);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(submit_onclick);
    CHECK_CALLED(submit_onclick_attached);

    V_VT(&v) = VT_NULL;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    SET_EXPECT(form_onclick);
    SET_EXPECT(submit_onclick_attached);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(submit_onclick_attached);

    elem_detach_event((IUnknown*)submit, "onclick", (IDispatch*)&submit_onclick_attached_obj);

    cp_cookie = register_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, (IUnknown*)&doccp_onclick_cancel_obj);

    SET_EXPECT(form_onclick);
    SET_EXPECT(doccp_onclick_cancel);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(doccp_onclick_cancel);

    unregister_cp((IUnknown*)doc, &DIID_HTMLDocumentEvents, cp_cookie);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&submit_onclick_setret_obj;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    V_VT(&onclick_retval) = VT_BOOL;
    V_BOOL(&onclick_retval) = VARIANT_TRUE;
    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_TRUE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    SET_EXPECT(form_onsubmit);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);
    CHECK_CALLED(form_onsubmit);

    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_FALSE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);

    V_VT(&onclick_retval) = VT_BOOL;
    V_BOOL(&onclick_retval) = VARIANT_FALSE;
    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_TRUE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);

    V_VT(&onclick_event_retval) = VT_BOOL;
    V_BOOL(&onclick_event_retval) = VARIANT_FALSE;

    SET_EXPECT(submit_onclick_setret);
    SET_EXPECT(form_onclick);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(submit_onclick_setret);
    CHECK_CALLED(form_onclick);

    elem_attach_event((IUnknown*)submit, "onclick", (IDispatch*)&submit_onclick_attached_obj);
    elem_attach_event((IUnknown*)submit, "onclick", (IDispatch*)&submit_onclick_attached_check_cancel_obj);

    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = (IDispatch*)&submit_onclick_cancel_obj;
    hres = IHTMLElement_put_onclick(submit, v);
    ok(hres == S_OK, "put_onclick failed: %08x\n", hres);

    SET_EXPECT(submit_onclick_cancel);
    SET_EXPECT(submit_onclick_attached_check_cancel);
    SET_EXPECT(submit_onclick_attached);
    hres = IHTMLElement_click(submit);
    ok(hres == S_OK, "click failed: %08x\n", hres);
    CHECK_CALLED(submit_onclick_cancel);
    CHECK_CALLED(submit_onclick_attached_check_cancel);
    CHECK_CALLED(submit_onclick_attached);

    if(1)pump_msgs(NULL);

    IHTMLElement_Release(submit);
}

static void test_timeout(IHTMLDocument2 *doc)
{
    IHTMLWindow3 *win3;
    VARIANT expr, var;
    LONG id;
    HRESULT hres;

    hres = IHTMLWindow2_QueryInterface(window, &IID_IHTMLWindow3, (void**)&win3);
    ok(hres == S_OK, "Could not get IHTMLWindow3 iface: %08x\n", hres);

    V_VT(&expr) = VT_DISPATCH;
    V_DISPATCH(&expr) = (IDispatch*)&timeoutFunc;
    V_VT(&var) = VT_EMPTY;
    id = 0;
    hres = IHTMLWindow3_setTimeout(win3, &expr, 0, &var, &id);
    ok(hres == S_OK, "setTimeout failed: %08x\n", hres);
    ok(id, "id = 0\n");

    SET_EXPECT(timeout);
    pump_msgs(&called_timeout);
    CHECK_CALLED(timeout);

    V_VT(&expr) = VT_DISPATCH;
    V_DISPATCH(&expr) = (IDispatch*)&timeoutFunc;
    V_VT(&var) = VT_EMPTY;
    id = 0;
    hres = IHTMLWindow3_setTimeout(win3, &expr, 0, &var, &id);
    ok(hres == S_OK, "setTimeout failed: %08x\n", hres);
    ok(id, "id = 0\n");

    hres = IHTMLWindow2_clearTimeout(window, id);
    ok(hres == S_OK, "clearTimeout failed: %08x\n", hres);

    IHTMLWindow3_Release(win3);
}

static IHTMLElement* find_element_by_id(IHTMLDocument2 *doc, const char *id)
{
    HRESULT hres;
    IHTMLDocument3 *doc3;
    IHTMLElement *result;
    BSTR idW = a2bstr(id);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument3, (void**)&doc3);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLDocument3) failed: %08x\n", hres);

    hres = IHTMLDocument3_getElementById(doc3, idW, &result);
    ok(hres == S_OK, "getElementById failed: %08x\n", hres);
    ok(result != NULL, "result == NULL\n");
    SysFreeString(idW);

    IHTMLDocument3_Release(doc3);
    return result;
}

static IHTMLDocument2* get_iframe_doc(IHTMLIFrameElement *iframe)
{
    HRESULT hres;
    IHTMLFrameBase2 *base;
    IHTMLDocument2 *result = NULL;

    hres = IHTMLIFrameElement_QueryInterface(iframe, &IID_IHTMLFrameBase2, (void**)&base);
    ok(hres == S_OK, "QueryInterface(IID_IHTMLFrameBase2) failed: %08x\n", hres);
    if(hres == S_OK) {
        IHTMLWindow2 *window;

        hres = IHTMLFrameBase2_get_contentWindow(base, &window);
        ok(hres == S_OK, "get_contentWindow failed: %08x\n", hres);
        ok(window != NULL, "window == NULL\n");
        if(window) {
            hres = IHTMLWindow2_get_document(window, &result);
            ok(hres == S_OK, "get_document failed: %08x\n", hres);
            ok(result != NULL, "result == NULL\n");
            IHTMLWindow2_Release(window);
        }
    }
    if(base) IHTMLFrameBase2_Release(base);

    return result;
}

static void test_iframe_connections(IHTMLDocument2 *doc)
{
    IHTMLIFrameElement *iframe;
    IHTMLDocument2 *iframes_doc;
    DWORD cookie;
    IConnectionPoint *cp;
    IHTMLElement *element;
    BSTR str;
    HRESULT hres;

    trace("iframe tests...\n");

    element = find_element_by_id(doc, "ifr");
    iframe = get_iframe_iface((IUnknown*)element);
    IHTMLElement_Release(element);

    iframes_doc = get_iframe_doc(iframe);
    IHTMLIFrameElement_Release(iframe);

    cookie = register_cp((IUnknown*)iframes_doc, &IID_IDispatch, (IUnknown*)&div_onclick_disp);

    cp = get_cp((IUnknown*)doc, &IID_IDispatch);
    hres = IConnectionPoint_Unadvise(cp, cookie);
    IConnectionPoint_Release(cp);
    ok(hres == CONNECT_E_NOCONNECTION, "Unadvise returned %08x, expected CONNECT_E_NOCONNECTION\n", hres);

    unregister_cp((IUnknown*)iframes_doc, &IID_IDispatch, cookie);

    if(is_ie9plus) {
        IHTMLFrameBase2 *frame_base2;
        VARIANT v;

        hres = IHTMLIFrameElement_QueryInterface(iframe, &IID_IHTMLFrameBase2, (void**)&frame_base2);
        ok(hres == S_OK, "Could not get IHTMLFrameBase2 iface: %08x\n", hres);

        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = (IDispatch*)&iframe_onload_obj;
        hres = IHTMLFrameBase2_put_onload(frame_base2, v);
        ok(hres == S_OK, "put_onload failed: %08x\n", hres);

        IHTMLFrameBase2_Release(frame_base2);

        str = a2bstr("about:blank");
        hres = IHTMLDocument2_put_URL(iframes_doc, str);
        ok(hres == S_OK, "put_URL failed: %08x\n", hres);
        SysFreeString(str);

        SET_EXPECT(iframe_onload);
        pump_msgs(&called_iframe_onload);
        CHECK_CALLED(iframe_onload);

        str = a2bstr("about:test");
        hres = IHTMLDocument2_put_URL(iframes_doc, str);
        ok(hres == S_OK, "put_URL failed: %08x\n", hres);
        SysFreeString(str);

        SET_EXPECT(iframe_onload);
        pump_msgs(&called_iframe_onload);
        CHECK_CALLED(iframe_onload);
    }else {
        win_skip("Skipping iframe onload tests on IE older than 9.\n");
    }

    IHTMLDocument2_Release(iframes_doc);
}

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI InPlaceFrame_QueryInterface(IOleInPlaceFrame *iface, REFIID riid, void **ppv)
{
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

static HRESULT WINAPI InPlaceUIWindow_SetActiveObject(IOleInPlaceFrame *iface,
        IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
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

static const IOleInPlaceFrameVtbl InPlaceUIWindowVtbl = {
    InPlaceFrame_QueryInterface,
    InPlaceFrame_AddRef,
    InPlaceFrame_Release,
    InPlaceFrame_GetWindow,
    InPlaceFrame_ContextSensitiveHelp,
    InPlaceFrame_GetBorder,
    InPlaceFrame_RequestBorderSpace,
    InPlaceFrame_SetBorderSpace,
    InPlaceUIWindow_SetActiveObject,
};

static IOleInPlaceFrame InPlaceUIWindow = { &InPlaceUIWindowVtbl };

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
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
    static const RECT rect = {0,0,300,300};

    *ppFrame = &InPlaceFrame;
    *ppDoc = (IOleInPlaceUIWindow*)&InPlaceUIWindow;
    *lprcPosRect = rect;
    *lprcClipRect = rect;

    ok(lpFrameInfo->cb == sizeof(*lpFrameInfo), "lpFrameInfo->cb = %u, expected %u\n", lpFrameInfo->cb, (unsigned)sizeof(*lpFrameInfo));
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
    return QueryInterface(riid, ppv);
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
    return QueryInterface(riid, ppv);
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
    RECT rect = {0,0,300,300};
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

static HRESULT QueryInterface(REFIID riid, void **ppv)
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

static const IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PropertyNotifySink_QueryInterface,
    PropertyNotifySink_AddRef,
    PropertyNotifySink_Release,
    PropertyNotifySink_OnChanged,
    PropertyNotifySink_OnRequestEdit
};

static IPropertyNotifySink PropertyNotifySink = { &PropertyNotifySinkVtbl };

static void doc_load_string(IHTMLDocument2 *doc, const char *str)
{
    IPersistStreamInit *init;
    IStream *stream;
    HGLOBAL mem;
    SIZE_T len;

    notif_doc = doc;

    doc_complete = FALSE;
    len = strlen(str);
    mem = GlobalAlloc(0, len);
    memcpy(mem, str, len);
    CreateStreamOnHGlobal(mem, TRUE, &stream);

    IHTMLDocument2_QueryInterface(doc, &IID_IPersistStreamInit, (void**)&init);

    IPersistStreamInit_Load(init, stream);
    IPersistStreamInit_Release(init);
    IStream_Release(stream);
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
static IHTMLDocument2 *create_document(void)
{
    IHTMLDocument2 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IHTMLDocument2, (void**)&doc);
    ok(hres == S_OK, "CoCreateInstance failed: %08x\n", hres);
    return SUCCEEDED(hres) ? doc : NULL;
}


typedef void (*testfunc_t)(IHTMLDocument2*);

static void run_test(const char *str, testfunc_t test)
{
    IHTMLDocument2 *doc;
    IHTMLElement *body = NULL;
    MSG msg;
    HRESULT hres;

    doc = create_document();
    if (!doc)
        return;
    set_client_site(doc, TRUE);
    doc_load_string(doc, str);
    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    hres = IHTMLDocument2_get_body(doc, &body);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    if(body) {
        IHTMLElement_Release(body);

        hres = IHTMLDocument2_get_parentWindow(doc, &window);
        ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);
        ok(window != NULL, "window == NULL\n");

        test(doc);

        IHTMLWindow2_Release(window);
        window = NULL;
    }else {
        skip("Could not get document body. Assuming no Gecko installed.\n");
    }

    set_client_site(doc, FALSE);
    IHTMLDocument2_Release(doc);
}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static HWND create_container_window(void)
{
    static const CHAR szHTMLDocumentTest[] = "HTMLDocumentTest";
    static WNDCLASSEXA wndclass = {
        sizeof(WNDCLASSEXA),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        szHTMLDocumentTest,
        NULL
    };

    RegisterClassExA(&wndclass);
    return CreateWindowA(szHTMLDocumentTest, szHTMLDocumentTest,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            300, 300, NULL, NULL, NULL, NULL);
}

static void test_empty_document(void)
{
    HRESULT hres;
    IHTMLWindow2 *window;
    IHTMLDocument2 *windows_doc, *doc;
    IConnectionPoint *cp;
    DWORD cookie;

    doc = create_document();
    if(!doc)
        return;

    hres = IHTMLDocument2_get_parentWindow(doc, &window);
    ok(hres == S_OK, "get_parentWindow failed: %08x\n", hres);

    hres = IHTMLWindow2_get_document(window, &windows_doc);
    IHTMLWindow2_Release(window);
    ok(hres == S_OK, "get_document failed: %08x\n", hres);

    cookie = register_cp((IUnknown*)windows_doc, &IID_IDispatch, (IUnknown*)&div_onclick_disp);

    cp = get_cp((IUnknown*)doc, &IID_IDispatch);
    hres = IConnectionPoint_Unadvise(cp, cookie);
    IConnectionPoint_Release(cp);
    ok(hres == S_OK, "Unadvise failed: %08x\n", hres);

    IHTMLDocument2_Release(windows_doc);
    IHTMLDocument2_Release(doc);
}

static BOOL check_ie(void)
{
    IHTMLDocument2 *doc;
    IHTMLDocument5 *doc5;
    IHTMLDocument7 *doc7;
    HRESULT hres;

    doc = create_document();
    if(!doc)
        return FALSE;

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument7, (void**)&doc7);
    if(SUCCEEDED(hres)) {
        is_ie9plus = TRUE;
        IHTMLDocument7_Release(doc7);
    }

    trace("is_ie9plus %x\n", is_ie9plus);

    hres = IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDocument5, (void**)&doc5);
    if(SUCCEEDED(hres))
        IHTMLDocument5_Release(doc5);

    IHTMLDocument2_Release(doc);
    return SUCCEEDED(hres);
}

START_TEST(events)
{
    CoInitialize(NULL);

    if(check_ie()) {
        container_hwnd = create_container_window();

        if(winetest_interactive)
            ShowWindow(container_hwnd, SW_SHOW);

        run_test(empty_doc_str, test_timeout);
        run_test(click_doc_str, test_onclick);
        run_test(readystate_doc_str, test_onreadystatechange);
        run_test(img_doc_str, test_imgload);
        run_test(input_doc_str, test_focus);
        run_test(form_doc_str, test_submit);
        run_test(iframe_doc_str, test_iframe_connections);

        test_empty_document();

        DestroyWindow(container_hwnd);
    }else {
        win_skip("Too old IE\n");
    }

    CoUninitialize();
}
