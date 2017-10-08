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
//#include <stdio.h>

//#include "windef.h"
//#include "winbase.h"
//#include "ole2.h"
#include <mshtml.h>
//#include "mshtmhst.h"
//#include "docobj.h"

static int strcmp_wa(LPCWSTR strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return lstrcmpA(stra, buf);
}

static BOOL wstr_contains(const WCHAR *strw, const char *stra)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, strw, -1, buf, sizeof(buf), NULL, NULL);
    return strstr(buf, stra) != NULL;
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

static const WCHAR *strstr_wa(const WCHAR *str, const char *suba)
{
    BSTR sub;
    const WCHAR *ret = NULL;
    sub = a2bstr(suba);
    while (*str)
    {
        const WCHAR *p1 = str, *p2 = sub;
        while (*p1 && *p2 && *p1 == *p2) { p1++; p2++; }
        if (!*p2) {ret = str; break;}
        str++;
    }
    SysFreeString(sub);
    return ret;
}

#define test_var_bstr(a,b) _test_var_bstr(__LINE__,a,b)
static void _test_var_bstr(unsigned line, const VARIANT *v, const char *expect)
{
    ok_(__FILE__,line)(V_VT(v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(v));
    if(expect)
        ok_(__FILE__,line)(!strcmp_wa(V_BSTR(v), expect), "V_BSTR(v) = %s, expected %s\n", wine_dbgstr_w(V_BSTR(v)), expect);
    else
        ok_(__FILE__,line)(!V_BSTR(v), "V_BSTR(v) = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(v)));
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

#define get_current_style2_iface(u) _get_current_style2_iface(__LINE__,u)
static IHTMLCurrentStyle2 *_get_current_style2_iface(unsigned line, IUnknown *unk)
{
    IHTMLCurrentStyle2 *current_style2;
    HRESULT hres;

    hres = IUnknown_QueryInterface(unk, &IID_IHTMLCurrentStyle2, (void**)&current_style2);
    ok_(__FILE__,line) (hres == S_OK, "Could not get IHTMLElement2: %08x\n", hres);
    return current_style2;
}

static IHTMLElement *get_element_by_id(IHTMLDocument2 *doc, const char *id)
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

#define get_current_style(e) _get_current_style(__LINE__,e)
static IHTMLCurrentStyle *_get_current_style(unsigned line, IHTMLElement *elem)
{
    IHTMLCurrentStyle *cstyle;
    IHTMLElement2 *elem2;
    HRESULT hres;

    hres = IHTMLElement_QueryInterface(elem, &IID_IHTMLElement2, (void**)&elem2);
    ok(hres == S_OK, "Could not get IHTMLElement2 iface: %08x\n", hres);

    cstyle = NULL;
    hres = IHTMLElement2_get_currentStyle(elem2, &cstyle);
    ok(hres == S_OK, "get_currentStyle failed: %08x\n", hres);
    ok(cstyle != NULL, "cstyle = %p\n", cstyle);

    IHTMLElement2_Release(elem2);
    return cstyle;
}

#define test_border_styles(p, n) _test_border_styles(__LINE__, p, n)
static void _test_border_styles(unsigned line, IHTMLStyle *pStyle, BSTR Name)
{
    HRESULT hres;
    DISPID dispid;

    hres = IHTMLStyle_GetIDsOfNames(pStyle, &IID_NULL, &Name, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    ok_(__FILE__,line) (hres == S_OK, "GetIDsOfNames: %08x\n", hres);
    if(hres == S_OK)
    {
        DISPPARAMS params = {NULL,NULL,0,0};
        DISPID dispidNamed = DISPID_PROPERTYPUT;
        VARIANT ret;
        VARIANT vDefault;
        VARIANTARG arg;

        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &params, &vDefault, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "get_default. ret: %08x\n", hres);

        params.cArgs = 1;
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidNamed;
        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("none");
        params.rgvarg = &arg;
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "none. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("dotted");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "dotted. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("dashed");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
        DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "dashed. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("solid");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "solid. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("double");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "double. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("groove");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "groove. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("ridge");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "ridge. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("inset");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "inset. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("outset");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "outset. ret: %08x\n", hres);
        VariantClear(&arg);

        V_VT(&arg) = VT_BSTR;
        V_BSTR(&arg) = a2bstr("invalid");
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (FAILED(hres), "invalid value passed.\n");
        VariantClear(&arg);

        params.rgvarg = &vDefault;
        hres = IHTMLStyle_Invoke(pStyle, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
        ok_(__FILE__,line) (hres == S_OK, "default. ret: %08x\n", hres);
        VariantClear(&vDefault);
    }
}

#define test_style_csstext(s,t) _test_style_csstext(__LINE__,s,t)
static void _test_style_csstext(unsigned line, IHTMLStyle *style, const char *extext)
{
    BSTR text = (void*)0xdeadbeef;
    HRESULT hres;

    hres = IHTMLStyle_get_cssText(style, &text);
    ok_(__FILE__,line)(hres == S_OK, "get_cssText failed: %08x\n", hres);
    if(extext)
        ok_(__FILE__,line)(!strcmp_wa(text, extext), "cssText = %s\n", wine_dbgstr_w(text));
    else
        ok_(__FILE__,line)(!text, "cssText = %s\n", wine_dbgstr_w(text));

    SysFreeString(text);
}

#define test_style_set_csstext(s,t) _test_style_set_csstext(__LINE__,s,t)
static void _test_style_set_csstext(unsigned line, IHTMLStyle *style, const char *text)
{
    BSTR tmp;
    HRESULT hres;

    tmp = a2bstr(text);
    hres = IHTMLStyle_put_cssText(style, tmp);
    ok_(__FILE__,line)(hres == S_OK, "put_cssText failed: %08x\n", hres);
    SysFreeString(tmp);
}

#define test_style_remove_attribute(a,b,c) _test_style_remove_attribute(__LINE__,a,b,c)
static void _test_style_remove_attribute(unsigned line, IHTMLStyle *style, const char *attr, VARIANT_BOOL exb)
{
    BSTR str = a2bstr(attr);
    VARIANT_BOOL b = 100;
    HRESULT hres;

    hres = IHTMLStyle_removeAttribute(style, str, 1, &b);
    SysFreeString(str);
    ok_(__FILE__,line)(hres == S_OK, "removeAttribute failed: %08x\n", hres);
    ok_(__FILE__,line)(b == exb, "removeAttribute returned %x, expected %x\n", b, exb);
}

#define set_text_decoration(a,b) _set_text_decoration(__LINE__,a,b)
static void _set_text_decoration(unsigned line, IHTMLStyle *style, const char *v)
{
    BSTR str;
    HRESULT hres;

    str = a2bstr(v);
    hres = IHTMLStyle_put_textDecoration(style, str);
    ok_(__FILE__,line)(hres == S_OK, "put_textDecoration failed: %08x\n", hres);
    SysFreeString(str);
}

#define test_text_decoration(a,b) _test_text_decoration(__LINE__,a,b)
static void _test_text_decoration(unsigned line, IHTMLStyle *style, const char *exdec)
{
    BSTR str;
    HRESULT hres;

    hres = IHTMLStyle_get_textDecoration(style, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_textDecoration failed: %08x\n", hres);
    if(exdec)
        ok_(__FILE__,line)(!strcmp_wa(str, exdec), "textDecoration = %s, expected %s\n", wine_dbgstr_w(str), exdec);
    else
        ok_(__FILE__,line)(!str, "textDecoration = %s, expected NULL\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_set_csstext(IHTMLStyle *style)
{
    VARIANT v;
    HRESULT hres;

    test_style_set_csstext(style, "background-color: black;");

    hres = IHTMLStyle_get_backgroundColor(style, &v);
    ok(hres == S_OK, "get_backgroundColor: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "type failed: %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "black"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

static void test_style2(IHTMLStyle2 *style2)
{
    VARIANT v;
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle2_get_position(style2, &str);
    ok(hres == S_OK, "get_position failed: %08x\n", hres);
    ok(!str, "str != NULL\n");

    str = a2bstr("absolute");
    hres = IHTMLStyle2_put_position(style2, str);
    ok(hres == S_OK, "put_position failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle2_get_position(style2, &str);
    ok(hres == S_OK, "get_position failed: %08x\n", hres);
    ok(!strcmp_wa(str, "absolute"), "get_position returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* Test right */
    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle2_get_right(style2, &v);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(right)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(right) != NULL\n");
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("3px");
    hres = IHTMLStyle2_put_right(style2, v);
    ok(hres == S_OK, "put_right failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle2_get_right(style2, &v);
    ok(hres == S_OK, "get_right failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "3px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* direction */
    str = (void*)0xdeadbeef;
    hres = IHTMLStyle2_get_direction(style2, &str);
    ok(hres == S_OK, "get_direction failed: %08x\n", hres);
    ok(!str, "str = %s\n", wine_dbgstr_w(str));

    str = a2bstr("ltr");
    hres = IHTMLStyle2_put_direction(style2, str);
    ok(hres == S_OK, "put_direction failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle2_get_direction(style2, &str);
    ok(hres == S_OK, "get_direction failed: %08x\n", hres);
    ok(!strcmp_wa(str, "ltr"), "str = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* bottom */
    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle2_get_bottom(style2, &v);
    ok(hres == S_OK, "get_bottom failed: %08x\n", hres);
    test_var_bstr(&v, NULL);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 4;
    hres = IHTMLStyle2_put_bottom(style2, v);
    ok(hres == S_OK, "put_bottom failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle2_get_bottom(style2, &v);
    ok(hres == S_OK, "get_bottom failed: %08x\n", hres);
    test_var_bstr(&v, "4px");

    /* overflowX */
    str = (void*)0xdeadbeef;
    hres = IHTMLStyle2_get_overflowX(style2, &str);
    ok(hres == S_OK, "get_overflowX failed: %08x\n", hres);
    ok(!str, "overflowX = %s\n", wine_dbgstr_w(str));

    str = a2bstr("hidden");
    hres = IHTMLStyle2_put_overflowX(style2, str);
    ok(hres == S_OK, "put_overflowX failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle2_get_overflowX(style2, &str);
    ok(hres == S_OK, "get_overflowX failed: %08x\n", hres);
    ok(!strcmp_wa(str, "hidden"), "overflowX = %s\n", wine_dbgstr_w(str));

    /* overflowY */
    str = (void*)0xdeadbeef;
    hres = IHTMLStyle2_get_overflowY(style2, &str);
    ok(hres == S_OK, "get_overflowY failed: %08x\n", hres);
    ok(!str, "overflowY = %s\n", wine_dbgstr_w(str));

    str = a2bstr("hidden");
    hres = IHTMLStyle2_put_overflowY(style2, str);
    ok(hres == S_OK, "put_overflowY failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle2_get_overflowY(style2, &str);
    ok(hres == S_OK, "get_overflowY failed: %08x\n", hres);
    ok(!strcmp_wa(str, "hidden"), "overflowX = %s\n", wine_dbgstr_w(str));

    /* tableLayout */
    str = a2bstr("fixed");
    hres = IHTMLStyle2_put_tableLayout(style2, str);
    ok(hres == S_OK, "put_tableLayout failed: %08x\n", hres);
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle2_get_tableLayout(style2, &str);
    ok(hres == S_OK, "get_tableLayout failed: %08x\n", hres);
    ok(!strcmp_wa(str, "fixed"), "tableLayout = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_style3(IHTMLStyle3 *style3)
{
    VARIANT v;
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle3_get_wordWrap(style3, &str);
    ok(hres == S_OK, "get_wordWrap failed: %08x\n", hres);
    ok(!str, "str != NULL\n");

    str = a2bstr("break-word");
    hres = IHTMLStyle3_put_wordWrap(style3, str);
    ok(hres == S_OK, "put_wordWrap failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle3_get_wordWrap(style3, &str);
    ok(hres == S_OK, "get_wordWrap failed: %08x\n", hres);
    ok(!strcmp_wa(str, "break-word"), "get_wordWrap returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_VT(&v) = VT_ERROR;
    hres = IHTMLStyle3_get_zoom(style3, &v);
    ok(hres == S_OK, "get_zoom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(zoom) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(zoom) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("100%");
    hres = IHTMLStyle3_put_zoom(style3, v);
    ok(hres == S_OK, "put_zoom failed: %08x\n", hres);

    V_VT(&v) = VT_ERROR;
    hres = IHTMLStyle3_get_zoom(style3, &v);
    ok(hres == S_OK, "get_zoom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(zoom) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "100%"), "V_BSTR(zoom) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 1;
    hres = IHTMLStyle3_put_zoom(style3, v);
    ok(hres == S_OK, "put_zoom failed: %08x\n", hres);

    V_VT(&v) = VT_ERROR;
    hres = IHTMLStyle3_get_zoom(style3, &v);
    ok(hres == S_OK, "get_zoom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(zoom) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "1"), "V_BSTR(zoom) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);
}

static void test_style4(IHTMLStyle4 *style4)
{
    HRESULT hres;
    VARIANT v;
    VARIANT vdefault;

    hres = IHTMLStyle4_get_minHeight(style4, &vdefault);
    ok(hres == S_OK, "get_minHeight failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10px");
    hres = IHTMLStyle4_put_minHeight(style4, v);
    ok(hres == S_OK, "put_minHeight failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle4_get_minHeight(style4, &v);
    ok(hres == S_OK, "get_minHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok( !strcmp_wa(V_BSTR(&v), "10px"), "expect 10px got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle4_put_minHeight(style4, vdefault);
    ok(hres == S_OK, "put_minHeight failed: %08x\n", hres);
    VariantClear(&vdefault);
}

static void test_style5(IHTMLStyle5 *style5)
{
    HRESULT hres;
    VARIANT v;
    VARIANT vdefault;

    /* minWidth */
    hres = IHTMLStyle5_get_minWidth(style5, &vdefault);
    ok(hres == S_OK, "get_minWidth failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("12px");
    hres = IHTMLStyle5_put_minWidth(style5, v);
    ok(hres == S_OK, "put_minWidth failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_minWidth(style5, &v);
    ok(hres == S_OK, "get_minWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "12px"), "expect 12px got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10%");
    hres = IHTMLStyle5_put_minWidth(style5, v);
    ok(hres == S_OK, "put_minWidth failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_minWidth(style5, &v);
    ok(hres == S_OK, "get_minWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "10%"), "expect 10%% got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10");
    hres = IHTMLStyle5_put_minWidth(style5, v);
    ok(hres == S_OK, "put_minWidth failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_minWidth(style5, &v);
    ok(hres == S_OK, "get_minWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expect 10px got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle5_put_minWidth(style5, vdefault);
    ok(hres == S_OK, "put_minWidth failed: %08x\n", hres);
    VariantClear(&vdefault);

    /* maxWidth */
    hres = IHTMLStyle5_get_maxWidth(style5, &vdefault);
    ok(hres == S_OK, "get_maxWidth failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("200px");
    hres = IHTMLStyle5_put_maxWidth(style5, v);
    ok(hres == S_OK, "put_maxWidth failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_maxWidth(style5, &v);
    ok(hres == S_OK, "get_maxWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n",V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "200px"), "expect 200px got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("70%");
    hres = IHTMLStyle5_put_maxWidth(style5, v);
    ok(hres == S_OK, "put_maxWidth failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_maxWidth(style5, &v);
    ok(hres == S_OK, "get maxWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "70%"), "expect 70%% got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle5_put_maxWidth(style5,vdefault);
    ok(hres == S_OK, "put_maxWidth failed: %08x\n", hres);
    VariantClear(&vdefault);

    /* maxHeight */
    hres = IHTMLStyle5_get_maxHeight(style5, &vdefault);
    ok(hres == S_OK, "get maxHeight failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("200px");
    hres = IHTMLStyle5_put_maxHeight(style5, v);
    ok(hres == S_OK, "put maxHeight failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_maxHeight(style5, &v);
    ok(hres == S_OK, "get maxHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "200px"), "expect 200px got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("70%");
    hres = IHTMLStyle5_put_maxHeight(style5, v);
    ok(hres == S_OK, "put maxHeight failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_maxHeight(style5, &v);
    ok(hres == S_OK, "get_maxHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "70%"), "expect 70%% got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("100");
    hres = IHTMLStyle5_put_maxHeight(style5, v);
    ok(hres == S_OK, "put maxHeight failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle5_get_maxHeight(style5, &v);
    ok(hres == S_OK, "get_maxHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "100px"), "expect 100 got (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle5_put_maxHeight(style5, vdefault);
    ok(hres == S_OK, "put maxHeight failed:%08x\n", hres);
    VariantClear(&vdefault);
}

static void test_style6(IHTMLStyle6 *style)
{
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle6_get_outline(style, &str);
    ok(hres == S_OK, "get_outline failed: %08x\n", hres);
    ok(str && !*str, "outline = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("1px");
    hres = IHTMLStyle6_put_outline(style, str);
    ok(hres == S_OK, "put_outline failed: %08x\n", hres);
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle6_get_outline(style, &str);
    ok(hres == S_OK, "get_outline failed: %08x\n", hres);
    ok(wstr_contains(str, "1px"), "outline = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle6_get_boxSizing(style, &str);
    ok(hres == S_OK, "get_boxSizing failed: %08x\n", hres);
    ok(!str, "boxSizing = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("border-box");
    hres = IHTMLStyle6_put_boxSizing(style, str);
    ok(hres == S_OK, "put_boxSizing failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle6_get_boxSizing(style, &str);
    ok(hres == S_OK, "get_boxSizing failed: %08x\n", hres);
    ok(!strcmp_wa(str, "border-box"), "boxSizing = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);
}

static void test_body_style(IHTMLStyle *style)
{
    IHTMLStyle2 *style2;
    IHTMLStyle3 *style3;
    IHTMLStyle4 *style4;
    IHTMLStyle5 *style5;
    IHTMLStyle6 *style6;
    VARIANT_BOOL b;
    VARIANT v;
    BSTR str;
    HRESULT hres;
    float f;
    BSTR sOverflowDefault;
    BSTR sDefault;
    LONG l;
    VARIANT vDefault;

    test_style_csstext(style, NULL);

    hres = IHTMLStyle_get_position(style, &str);
    ok(hres == S_OK, "get_position failed: %08x\n", hres);
    ok(!str, "str=%s\n", wine_dbgstr_w(str));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_marginRight(style, &v);
    ok(hres == S_OK, "get_marginRight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginRight) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(marginRight) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_marginRight(style, v);
    ok(hres == S_OK, "put_marginRight failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_marginRight(style, &v);
    ok(hres == S_OK, "get_marginRight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginRight) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(marginRight) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_marginBottom(style, &v);
    ok(hres == S_OK, "get_marginBottom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginBottom) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(marginBottom) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_marginBottom(style, v);
    ok(hres == S_OK, "put_marginBottom failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_marginBottom(style, &v);
    ok(hres == S_OK, "get_marginBottom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginBottom) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(marginBottom) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_marginLeft(style, &v);
    ok(hres == S_OK, "get_marginLeft failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginLeft) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(marginLeft) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_marginLeft(style, v);
    ok(hres == S_OK, "put_marginLeft failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_marginLeft(style, &v);
    ok(hres == S_OK, "get_marginLeft failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginLeft) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(marginLeft) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_fontFamily(style, &str);
    ok(hres == S_OK, "get_fontFamily failed: %08x\n", hres);
    ok(!str, "fontFamily = %s\n", wine_dbgstr_w(str));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_fontWeight(style, &str);
    ok(hres == S_OK, "get_fontWeight failed: %08x\n", hres);
    ok(!str, "fontWeight = %s\n", wine_dbgstr_w(str));

    hres = IHTMLStyle_get_fontWeight(style, &sDefault);
    ok(hres == S_OK, "get_fontWeight failed: %08x\n", hres);

    str = a2bstr("test");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == E_INVALIDARG, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("bold");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("bolder");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("lighter");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("100");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("200");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("300");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("400");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("500");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("600");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("700");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("800");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("900");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_fontWeight(style, &str);
    ok(hres == S_OK, "get_fontWeight failed: %08x\n", hres);
    ok(!strcmp_wa(str, "900"), "str != style900\n");
    SysFreeString(str);

    str = a2bstr("");
    hres = IHTMLStyle_put_fontWeight(style, str);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_fontWeight(style, &str);
    ok(hres == S_OK, "get_fontWeight failed: %08x\n", hres);
    ok(!str, "str != NULL\n");
    SysFreeString(str);

    hres = IHTMLStyle_put_fontWeight(style, sDefault);
    ok(hres == S_OK, "put_fontWeight failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* font Variant */
    hres = IHTMLStyle_get_fontVariant(style, NULL);
    ok(hres == E_INVALIDARG, "get_fontVariant failed: %08x\n", hres);

    hres = IHTMLStyle_get_fontVariant(style, &sDefault);
    ok(hres == S_OK, "get_fontVariant failed: %08x\n", hres);

    str = a2bstr("test");
    hres = IHTMLStyle_put_fontVariant(style, str);
    ok(hres == E_INVALIDARG, "fontVariant failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("small-caps");
    hres = IHTMLStyle_put_fontVariant(style, str);
    ok(hres == S_OK, "fontVariant failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("normal");
    hres = IHTMLStyle_put_fontVariant(style, str);
    ok(hres == S_OK, "fontVariant failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_put_fontVariant(style, sDefault);
    ok(hres == S_OK, "fontVariant failed: %08x\n", hres);
    SysFreeString(sDefault);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_display(style, &str);
    ok(hres == S_OK, "get_display failed: %08x\n", hres);
    ok(!str, "display = %s\n", wine_dbgstr_w(str));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_visibility(style, &str);
    ok(hres == S_OK, "get_visibility failed: %08x\n", hres);
    ok(!str, "visibility = %s\n", wine_dbgstr_w(str));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_fontSize(style, &v);
    ok(hres == S_OK, "get_fontSize failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(fontSize) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(fontSize) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 12;
    hres = IHTMLStyle_put_fontSize(style, v);
    ok(hres == S_OK, "put_fontSize failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_fontSize(style, &v);
    ok(hres == S_OK, "get_fontSize failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(fontSize) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "12px"), "V_BSTR(fontSize) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_color(style, &v);
    ok(hres == S_OK, "get_color failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(color) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(color) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 0xfdfd;
    hres = IHTMLStyle_put_color(style, v);
    ok(hres == S_OK, "put_color failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_color(style, &v);
    ok(hres == S_OK, "get_color failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(color) = %d\n", V_VT(&v));
    todo_wine
    ok(!strcmp_wa(V_BSTR(&v), "#00fdfd"), "V_BSTR(color) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 3;
    hres = IHTMLStyle_put_lineHeight(style, v);
    ok(hres == S_OK, "put_lineHeight failed: %08x\n", hres);

    hres = IHTMLStyle_get_lineHeight(style, &v);
    ok(hres == S_OK, "get_lineHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(lineHeight) = %d, expect VT_BSTR\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "3"), "V_BSTR(lineHeight) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("300%");
    hres = IHTMLStyle_put_lineHeight(style, v);
    ok(hres == S_OK, "put_lineHeight failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_lineHeight(style, &v);
    ok(hres == S_OK, "get_lineHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(lineHeight) = %d, expect VT_BSTR\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "300%"), "V_BSTR(lineHeight) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationUnderline(style, &b);
    ok(hres == S_OK, "get_textDecorationUnderline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationUnderline = %x\n", b);

    hres = IHTMLStyle_put_textDecorationUnderline(style, VARIANT_TRUE);
    ok(hres == S_OK, "put_textDecorationUnderline failed: %08x\n", hres);

    hres = IHTMLStyle_get_textDecorationUnderline(style, &b);
    ok(hres == S_OK, "get_textDecorationUnderline failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "textDecorationUnderline = %x\n", b);

    hres = IHTMLStyle_put_textDecorationUnderline(style, VARIANT_FALSE);
    ok(hres == S_OK, "put_textDecorationUnderline failed: %08x\n", hres);

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationLineThrough(style, &b);
    ok(hres == S_OK, "get_textDecorationLineThrough failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationLineThrough = %x\n", b);

    hres = IHTMLStyle_put_textDecorationLineThrough(style, VARIANT_TRUE);
    ok(hres == S_OK, "put_textDecorationLineThrough failed: %08x\n", hres);

    hres = IHTMLStyle_get_textDecorationLineThrough(style, &b);
    ok(hres == S_OK, "get_textDecorationLineThrough failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "textDecorationLineThrough = %x\n", b);

    hres = IHTMLStyle_put_textDecorationLineThrough(style, VARIANT_FALSE);
    ok(hres == S_OK, "put_textDecorationLineThrough failed: %08x\n", hres);

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationNone(style, &b);
    ok(hres == S_OK, "get_textDecorationNone failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationNone = %x\n", b);

    hres = IHTMLStyle_put_textDecorationNone(style, VARIANT_TRUE);
    ok(hres == S_OK, "put_textDecorationNone failed: %08x\n", hres);

    hres = IHTMLStyle_get_textDecorationNone(style, &b);
    ok(hres == S_OK, "get_textDecorationNone failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "textDecorationNone = %x\n", b);

    hres = IHTMLStyle_put_textDecorationNone(style, VARIANT_FALSE);
    ok(hres == S_OK, "put_textDecorationNone failed: %08x\n", hres);

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationOverline(style, &b);
    ok(hres == S_OK, "get_textDecorationOverline failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationOverline = %x\n", b);

    hres = IHTMLStyle_put_textDecorationOverline(style, VARIANT_TRUE);
    ok(hres == S_OK, "put_textDecorationOverline failed: %08x\n", hres);

    hres = IHTMLStyle_get_textDecorationOverline(style, &b);
    ok(hres == S_OK, "get_textDecorationOverline failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "textDecorationOverline = %x\n", b);

    hres = IHTMLStyle_put_textDecorationOverline(style, VARIANT_FALSE);
    ok(hres == S_OK, "put_textDecorationOverline failed: %08x\n", hres);

    b = 0xfefe;
    hres = IHTMLStyle_get_textDecorationBlink(style, &b);
    ok(hres == S_OK, "get_textDecorationBlink failed: %08x\n", hres);
    ok(b == VARIANT_FALSE, "textDecorationBlink = %x\n", b);

    hres = IHTMLStyle_put_textDecorationBlink(style, VARIANT_TRUE);
    ok(hres == S_OK, "put_textDecorationBlink failed: %08x\n", hres);

    hres = IHTMLStyle_get_textDecorationBlink(style, &b);
    ok(hres == S_OK, "get_textDecorationBlink failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "textDecorationBlink = %x\n", b);

    hres = IHTMLStyle_put_textDecorationBlink(style, VARIANT_FALSE);
    ok(hres == S_OK, "textDecorationBlink failed: %08x\n", hres);

    hres = IHTMLStyle_get_textDecoration(style, &sDefault);
    ok(hres == S_OK, "get_textDecoration failed: %08x\n", hres);

    str = a2bstr("invalid");
    hres = IHTMLStyle_put_textDecoration(style, str);
    ok(hres == E_INVALIDARG, "put_textDecoration failed: %08x\n", hres);
    SysFreeString(str);

    set_text_decoration(style, "none");
    test_text_decoration(style, "none");
    set_text_decoration(style, "underline");
    set_text_decoration(style, "overline");
    set_text_decoration(style, "line-through");
    set_text_decoration(style, "blink");
    set_text_decoration(style, "overline");
    set_text_decoration(style, "blink");
    test_text_decoration(style, "blink");

    hres = IHTMLStyle_put_textDecoration(style, sDefault);
    ok(hres == S_OK, "put_textDecoration failed: %08x\n", hres);
    SysFreeString(sDefault);

    hres = IHTMLStyle_get_posWidth(style, NULL);
    ok(hres == E_POINTER, "get_posWidth failed: %08x\n", hres);

    hres = IHTMLStyle_get_posWidth(style, &f);
    ok(hres == S_OK, "get_posWidth failed: %08x\n", hres);
    ok(f == 0.0f, "f = %f\n", f);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_width(style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v)=%p\n", V_BSTR(&v));

    hres = IHTMLStyle_put_posWidth(style, 2.2);
    ok(hres == S_OK, "put_posWidth failed: %08x\n", hres);

    hres = IHTMLStyle_get_posWidth(style, &f);
    ok(hres == S_OK, "get_posWidth failed: %08x\n", hres);
    ok(f == 2.0f ||
       f == 2.2f, /* IE8 */
       "f = %f\n", f);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelWidth(style, &l);
    ok(hres == S_OK, "get_pixelWidth failed: %08x\n", hres);
    ok(l == 2, "pixelWidth = %d\n", l);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("auto");
    hres = IHTMLStyle_put_width(style, v);
    ok(hres == S_OK, "put_width failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_width(style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "auto"), "V_BSTR(v)=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 100;
    hres = IHTMLStyle_put_width(style, v);
    ok(hres == S_OK, "put_width failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelWidth(style, &l);
    ok(hres == S_OK, "get_pixelWidth failed: %08x\n", hres);
    ok(l == 100, "pixelWidth = %d\n", l);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_width(style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "100px"), "V_BSTR(v)=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_pixelWidth(style, 50);
    ok(hres == S_OK, "put_pixelWidth failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelWidth(style, &l);
    ok(hres == S_OK, "get_pixelWidth failed: %08x\n", hres);
    ok(l == 50, "pixelWidth = %d\n", l);

    hres = IHTMLStyle_get_pixelWidth(style, NULL);
    ok(hres == E_POINTER, "get_pixelWidth failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_width(style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "50px"), "V_BSTR(v)=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* margin tests */
    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_margin(style, &str);
    ok(hres == S_OK, "get_margin failed: %08x\n", hres);
    ok(!str, "margin = %s\n", wine_dbgstr_w(str));

    str = a2bstr("1");
    hres = IHTMLStyle_put_margin(style, str);
    ok(hres == S_OK, "put_margin failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_margin(style, &str);
    ok(hres == S_OK, "get_margin failed: %08x\n", hres);
    ok(!strcmp_wa(str, "1px"), "margin = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_margin(style, NULL);
    ok(hres == S_OK, "put_margin failed: %08x\n", hres);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_marginTop(style, &v);
    ok(hres == S_OK, "get_marginTop failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginTop) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(marginTop) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("6px");
    hres = IHTMLStyle_put_marginTop(style, v);
    SysFreeString(V_BSTR(&v));
    ok(hres == S_OK, "put_marginTop failed: %08x\n", hres);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_marginTop(style, &v);
    ok(hres == S_OK, "get_marginTop failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginTop) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(marginTop) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 5;
    hres = IHTMLStyle_put_marginTop(style, v);
    ok(hres == S_OK, "put_marginTop failed: %08x\n", hres);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_marginTop(style, &v);
    ok(hres == S_OK, "get_marginTop failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(marginTop) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "5px"), "V_BSTR(marginTop) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    str = NULL;
    hres = IHTMLStyle_get_border(style, &str);
    ok(hres == S_OK, "get_border failed: %08x\n", hres);
    ok(!str || !*str, "str is not empty\n");
    SysFreeString(str);

    str = a2bstr("1px");
    hres = IHTMLStyle_put_border(style, str);
    ok(hres == S_OK, "put_border failed: %08x\n", hres);
    SysFreeString(str);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_left(style, &v);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) != NULL\n");
    VariantClear(&v);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelLeft(style, &l);
    ok(hres == S_OK, "get_pixelLeft failed: %08x\n", hres);
    ok(!l, "pixelLeft = %d\n", l);

    /* Test posLeft */
    hres = IHTMLStyle_get_posLeft(style, NULL);
    ok(hres == E_POINTER, "get_posLeft failed: %08x\n", hres);

    f = 1.0f;
    hres = IHTMLStyle_get_posLeft(style, &f);
    ok(hres == S_OK, "get_posLeft failed: %08x\n", hres);
    ok(f == 0.0, "expected 0.0 got %f\n", f);

    hres = IHTMLStyle_put_posLeft(style, 4.9f);
    ok(hres == S_OK, "put_posLeft failed: %08x\n", hres);

    hres = IHTMLStyle_get_posLeft(style, &f);
    ok(hres == S_OK, "get_posLeft failed: %08x\n", hres);
    ok(f == 4.0 ||
       f == 4.9f, /* IE8 */
       "expected 4.0 or 4.9 (IE8) got %f\n", f);

    /* Ensure left is updated correctly. */
    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_left(style, &v);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "4px") ||
       !strcmp_wa(V_BSTR(&v), "4.9px"), /* IE8 */
       "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* Test left */
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("3px");
    hres = IHTMLStyle_put_left(style, v);
    ok(hres == S_OK, "put_left failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_posLeft(style, &f);
    ok(hres == S_OK, "get_posLeft failed: %08x\n", hres);
    ok(f == 3.0, "expected 3.0 got %f\n", f);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelLeft(style, &l);
    ok(hres == S_OK, "get_pixelLeft failed: %08x\n", hres);
    ok(l == 3, "pixelLeft = %d\n", l);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_left(style, &v);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "3px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("4.99");
    hres = IHTMLStyle_put_left(style, v);
    ok(hres == S_OK, "put_left failed: %08x\n", hres);
    VariantClear(&v);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelLeft(style, &l);
    ok(hres == S_OK, "get_pixelLeft failed: %08x\n", hres);
    ok(l == 4, "pixelLeft = %d\n", l);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_put_left(style, v);
    ok(hres == S_OK, "put_left failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_left(style, &v);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) != NULL\n");
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_top(style, &v);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) != NULL\n");
    VariantClear(&v);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelLeft(style, &l);
    ok(hres == S_OK, "get_pixelLeft failed: %08x\n", hres);
    ok(!l, "pixelLeft = %d\n", l);

    hres = IHTMLStyle_put_pixelLeft(style, 6);
    ok(hres == S_OK, "put_pixelLeft failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelLeft(style, &l);
    ok(hres == S_OK, "get_pixelLeft failed: %08x\n", hres);
    ok(l == 6, "pixelLeft = %d\n", l);

    hres = IHTMLStyle_get_pixelLeft(style, NULL);
    ok(hres == E_POINTER, "get_pixelLeft failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_left(style, &v);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* Test posTop */
    hres = IHTMLStyle_get_posTop(style, NULL);
    ok(hres == E_POINTER, "get_posTop failed: %08x\n", hres);

    f = 1.0f;
    hres = IHTMLStyle_get_posTop(style, &f);
    ok(hres == S_OK, "get_posTop failed: %08x\n", hres);
    ok(f == 0.0, "expected 0.0 got %f\n", f);

    hres = IHTMLStyle_put_posTop(style, 4.9f);
    ok(hres == S_OK, "put_posTop failed: %08x\n", hres);

    hres = IHTMLStyle_get_posTop(style, &f);
    ok(hres == S_OK, "get_posTop failed: %08x\n", hres);
    ok(f == 4.0 ||
       f == 4.9f, /* IE8 */
       "expected 4.0 or 4.9 (IE8) got %f\n", f);

    hres = IHTMLStyle_put_pixelTop(style, 6);
    ok(hres == S_OK, "put_pixelTop failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelTop(style, &l);
    ok(hres == S_OK, "get_pixelTop failed: %08x\n", hres);
    ok(l == 6, "pixelTop = %d\n", l);

    hres = IHTMLStyle_get_pixelTop(style, NULL);
    ok(hres == E_POINTER, "get_pixelTop failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("3px");
    hres = IHTMLStyle_put_top(style, v);
    ok(hres == S_OK, "put_top failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_top(style, &v);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "3px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_posTop(style, &f);
    ok(hres == S_OK, "get_posTop failed: %08x\n", hres);
    ok(f == 3.0, "expected 3.0 got %f\n", f);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelTop(style, &l);
    ok(hres == S_OK, "get_pixelTop failed: %08x\n", hres);
    ok(l == 3, "pixelTop = %d\n", l);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_put_top(style, v);
    ok(hres == S_OK, "put_top failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_top(style, &v);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) != NULL\n");
    VariantClear(&v);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelTop(style, &l);
    ok(hres == S_OK, "get_pixelTop failed: %08x\n", hres);
    ok(!l, "pixelTop = %d\n", l);

    /* Test posHeight */
    hres = IHTMLStyle_get_posHeight(style, NULL);
    ok(hres == E_POINTER, "get_posHeight failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_height(style, &v);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) != NULL\n");
    VariantClear(&v);

    f = 1.0f;
    hres = IHTMLStyle_get_posHeight(style, &f);
    ok(hres == S_OK, "get_posHeight failed: %08x\n", hres);
    ok(f == 0.0, "expected 0.0 got %f\n", f);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelHeight(style, &l);
    ok(hres == S_OK, "get_pixelHeight failed: %08x\n", hres);
    ok(!l, "pixelHeight = %d\n", l);

    hres = IHTMLStyle_put_posHeight(style, 4.9f);
    ok(hres == S_OK, "put_posHeight failed: %08x\n", hres);

    hres = IHTMLStyle_get_posHeight(style, &f);
    ok(hres == S_OK, "get_posHeight failed: %08x\n", hres);
    ok(f == 4.0 ||
       f == 4.9f, /* IE8 */
       "expected 4.0 or 4.9 (IE8) got %f\n", f);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelHeight(style, &l);
    ok(hres == S_OK, "get_pixelHeight failed: %08x\n", hres);
    ok(l == 4 ||
       l == 5, /* IE8 */
       "pixelHeight = %d\n", l);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("70px");
    hres = IHTMLStyle_put_height(style, v);
    ok(hres == S_OK, "put_height failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_height(style, &v);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "70px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelHeight(style, &l);
    ok(hres == S_OK, "get_pixelHeight failed: %08x\n", hres);
    ok(l == 70, "pixelHeight = %d\n", l);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_put_height(style, v);
    ok(hres == S_OK, "put_height failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_height(style, &v);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) = %s, expected NULL\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelHeight(style, &l);
    ok(hres == S_OK, "get_pixelHeight failed: %08x\n", hres);
    ok(!l, "pixelHeight = %d\n", l);

    hres = IHTMLStyle_put_pixelHeight(style, 50);
    ok(hres == S_OK, "put_pixelHeight failed: %08x\n", hres);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelHeight(style, &l);
    ok(hres == S_OK, "get_pixelHeight failed: %08x\n", hres);
    ok(l == 50, "pixelHeight = %d\n", l);

    hres = IHTMLStyle_get_pixelHeight(style, NULL);
    ok(hres == E_POINTER, "get_pixelHeight failed: %08x\n", hres);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 64;
    hres = IHTMLStyle_put_height(style, v);
    ok(hres == S_OK, "put_height failed: %08x\n", hres);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_height(style, &v);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "64px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_posHeight(style, &f);
    ok(hres == S_OK, "get_posHeight failed: %08x\n", hres);
    ok(f == 64.0, "expected 64.0 got %f\n", f);

    l = 0xdeadbeef;
    hres = IHTMLStyle_get_pixelHeight(style, &l);
    ok(hres == S_OK, "get_pixelHeight failed: %08x\n", hres);
    ok(l == 64, "pixelHeight = %d\n", l);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_cursor(style, &str);
    ok(hres == S_OK, "get_cursor failed: %08x\n", hres);
    ok(!str, "get_cursor != NULL\n");
    SysFreeString(str);

    str = a2bstr("default");
    hres = IHTMLStyle_put_cursor(style, str);
    ok(hres == S_OK, "put_cursor failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle_get_cursor(style, &str);
    ok(hres == S_OK, "get_cursor failed: %08x\n", hres);
    ok(!strcmp_wa(str, "default"), "get_cursor returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_verticalAlign(style, &v);
    ok(hres == S_OK, "get_vertivalAlign failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) != NULL\n");
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("middle");
    hres = IHTMLStyle_put_verticalAlign(style, v);
    ok(hres == S_OK, "put_vertivalAlign failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_verticalAlign(style, &v);
    ok(hres == S_OK, "get_verticalAlign failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "middle"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_I4;
    V_I4(&v) = 100;
    hres = IHTMLStyle_put_verticalAlign(style, v);
    ok(hres == S_OK, "put_vertivalAlign failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_verticalAlign(style, &v);
    ok(hres == S_OK, "get_verticalAlign failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "100px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_textAlign(style, &str);
    ok(hres == S_OK, "get_textAlign failed: %08x\n", hres);
    ok(!str, "textAlign != NULL\n");

    str = a2bstr("center");
    hres = IHTMLStyle_put_textAlign(style, str);
    ok(hres == S_OK, "put_textAlign failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle_get_textAlign(style, &str);
    ok(hres == S_OK, "get_textAlign failed: %08x\n", hres);
    ok(!strcmp_wa(str, "center"), "textAlign = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    SysFreeString(str);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_textIndent(style, &v);
    ok(hres == S_OK, "get_textIndent failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(textIndent) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(textIndent) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_textIndent(style, v);
    ok(hres == S_OK, "put_textIndent failed: %08x\n", hres);

    V_VT(&v) = VT_NULL;
    hres = IHTMLStyle_get_textIndent(style, &v);
    ok(hres == S_OK, "get_textIndent failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(textIndent) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(textIndent) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_textTransform(style, &str);
    ok(hres == S_OK, "get_textTransform failed: %08x\n", hres);
    ok(!str, "textTransform != NULL\n");

    str = a2bstr("lowercase");
    hres = IHTMLStyle_put_textTransform(style, str);
    ok(hres == S_OK, "put_textTransform failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle_get_textTransform(style, &str);
    ok(hres == S_OK, "get_textTransform failed: %08x\n", hres);
    ok(!strcmp_wa(str, "lowercase"), "textTransform = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    SysFreeString(str);

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_filter(style, &str);
    ok(hres == S_OK, "get_filter failed: %08x\n", hres);
    ok(!str, "filter != NULL\n");

    str = a2bstr("alpha(opacity=100)");
    hres = IHTMLStyle_put_filter(style, str);
    ok(hres == S_OK, "put_filter failed: %08x\n", hres);
    SysFreeString(str);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_zIndex(style, &v);
    ok(hres == S_OK, "get_zIndex failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_I4(&v), "V_I4(v) != 0\n");
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("1");
    hres = IHTMLStyle_put_zIndex(style, v);
    ok(hres == S_OK, "put_zIndex failed: %08x\n", hres);
    VariantClear(&v);

    V_VT(&v) = VT_EMPTY;
    hres = IHTMLStyle_get_zIndex(style, &v);
    ok(hres == S_OK, "get_zIndex failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v)=%d\n", V_VT(&v));
    ok(V_I4(&v) == 1, "V_I4(v) = %d\n", V_I4(&v));
    VariantClear(&v);

    /* fontStyle */
    hres = IHTMLStyle_get_fontStyle(style, &sDefault);
    ok(hres == S_OK, "get_fontStyle failed: %08x\n", hres);

    str = a2bstr("test");
    hres = IHTMLStyle_put_fontStyle(style, str);
    ok(hres == E_INVALIDARG, "put_fontStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("italic");
    hres = IHTMLStyle_put_fontStyle(style, str);
    ok(hres == S_OK, "put_fontStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("oblique");
    hres = IHTMLStyle_put_fontStyle(style, str);
    ok(hres == S_OK, "put_fontStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("normal");
    hres = IHTMLStyle_put_fontStyle(style, str);
    ok(hres == S_OK, "put_fontStyle failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_put_fontStyle(style, sDefault);
    ok(hres == S_OK, "put_fontStyle failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* overflow */
    hres = IHTMLStyle_get_overflow(style, NULL);
    ok(hres == E_INVALIDARG, "get_overflow failed: %08x\n", hres);

    hres = IHTMLStyle_get_overflow(style, &sOverflowDefault);
    ok(hres == S_OK, "get_overflow failed: %08x\n", hres);

    str = a2bstr("test");
    hres = IHTMLStyle_put_overflow(style, str);
    ok(hres == E_INVALIDARG, "put_overflow failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("visible");
    hres = IHTMLStyle_put_overflow(style, str);
    ok(hres == S_OK, "put_overflow failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("scroll");
    hres = IHTMLStyle_put_overflow(style, str);
    ok(hres == S_OK, "put_overflow failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("hidden");
    hres = IHTMLStyle_put_overflow(style, str);
    ok(hres == S_OK, "put_overflow failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("auto");
    hres = IHTMLStyle_put_overflow(style, str);
    ok(hres == S_OK, "put_overflow failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_overflow(style, &str);
    ok(hres == S_OK, "get_overflow failed: %08x\n", hres);
    ok(!strcmp_wa(str, "auto"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("");
    hres = IHTMLStyle_put_overflow(style, str);
    ok(hres == S_OK, "put_overflow failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_overflow(style, &str);
    ok(hres == S_OK, "get_overflow failed: %08x\n", hres);
    ok(!str, "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* restore overflow default */
    hres = IHTMLStyle_put_overflow(style, sOverflowDefault);
    ok(hres == S_OK, "put_overflow failed: %08x\n", hres);
    SysFreeString(sOverflowDefault);

    /* Attribute Tests*/
    hres = IHTMLStyle_getAttribute(style, NULL, 1, &v);
    ok(hres == E_INVALIDARG, "getAttribute failed: %08x\n", hres);

    str = a2bstr("position");
    hres = IHTMLStyle_getAttribute(style, str, 1, NULL);
    ok(hres == E_INVALIDARG, "getAttribute failed: %08x\n", hres);

    hres = IHTMLStyle_getAttribute(style, str, 1, &v);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "type failed: %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLStyle_setAttribute(style, NULL, v, 1);
    ok(hres == E_INVALIDARG, "setAttribute failed: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("absolute");
    hres = IHTMLStyle_setAttribute(style, str, v, 1);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_getAttribute(style, str, 1, &v);
    ok(hres == S_OK, "getAttribute failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "type failed: %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "absolute"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_setAttribute(style, str, v, 1);
    ok(hres == S_OK, "setAttribute failed: %08x\n", hres);
    VariantClear(&v);

    SysFreeString(str);

    str = a2bstr("borderLeftStyle");
    test_border_styles(style, str);
    SysFreeString(str);

    str = a2bstr("borderbottomstyle");
    test_border_styles(style, str);
    SysFreeString(str);

    str = a2bstr("borderrightstyle");
    test_border_styles(style, str);
    SysFreeString(str);

    str = a2bstr("bordertopstyle");
    test_border_styles(style, str);
    SysFreeString(str);

    hres = IHTMLStyle_get_borderStyle(style, &sDefault);
    ok(hres == S_OK, "get_borderStyle failed: %08x\n", hres);

    str = a2bstr("none dotted dashed solid");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == S_OK, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("none dotted dashed solid");
    hres = IHTMLStyle_get_borderStyle(style, &str);
    ok(hres == S_OK, "get_borderStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none dotted dashed solid"),
        "expected (none dotted dashed solid) = (%s)\n", wine_dbgstr_w(V_BSTR(&v)));
    SysFreeString(str);

    str = a2bstr("double groove ridge inset");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == S_OK, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("window-inset outset ridge inset");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == S_OK, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("window-inset");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == S_OK, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("none none none none none");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == S_OK, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("invalid none none none");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == E_INVALIDARG, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("none invalid none none");
    hres = IHTMLStyle_put_borderStyle(style, str);
    ok(hres == E_INVALIDARG, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_put_borderStyle(style, sDefault);
    ok(hres == S_OK, "put_borderStyle failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* backgoundColor */
    hres = IHTMLStyle_get_backgroundColor(style, &v);
    ok(hres == S_OK, "get_backgroundColor: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "type failed: %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("red");
    hres = IHTMLStyle_put_backgroundColor(style, v);
    ok(hres == S_OK, "put_backgroundColor failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_backgroundColor(style, &v);
    ok(hres == S_OK, "get_backgroundColor: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "type failed: %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "red"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    str = a2bstr("fixed");
    hres = IHTMLStyle_put_backgroundAttachment(style, str);
    ok(hres == S_OK, "put_backgroundAttachment failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_backgroundAttachment(style, &str);
    ok(hres == S_OK, "get_backgroundAttachment failed: %08x\n", hres);
    ok(!strcmp_wa(str, "fixed"), "ret = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* padding */
    hres = IHTMLStyle_get_padding(style, &str);
    ok(hres == S_OK, "get_padding failed: %08x\n", hres);
    ok(!str, "padding = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_get_paddingTop(style, &v);
    ok(hres == S_OK, "get_paddingTop: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_paddingTop(style, v);
    ok(hres == S_OK, "put_paddingTop failed: %08x\n", hres);

    hres = IHTMLStyle_get_paddingTop(style, &v);
    ok(hres == S_OK, "get_paddingTop: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_paddingRight(style, &v);
    ok(hres == S_OK, "get_paddingRight: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_paddingRight(style, v);
    ok(hres == S_OK, "put_paddingRight failed: %08x\n", hres);

    hres = IHTMLStyle_get_paddingRight(style, &v);
    ok(hres == S_OK, "get_paddingRight: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_paddingBottom(style, &v);
    ok(hres == S_OK, "get_paddingBottom: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!V_BSTR(&v), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));

    V_VT(&v) = VT_I4;
    V_I4(&v) = 6;
    hres = IHTMLStyle_put_paddingBottom(style, v);
    ok(hres == S_OK, "put_paddingBottom failed: %08x\n", hres);

    hres = IHTMLStyle_get_paddingBottom(style, &v);
    ok(hres == S_OK, "get_paddingBottom: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "6px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    str = a2bstr("1");
    hres = IHTMLStyle_put_padding(style, str);
    ok(hres == S_OK, "put_padding failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_padding(style, &str);
    ok(hres == S_OK, "get_padding failed: %08x\n", hres);
    ok(!strcmp_wa(str, "1px"), "padding = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* PaddingLeft */
    hres = IHTMLStyle_get_paddingLeft(style, &vDefault);
    ok(hres == S_OK, "get_paddingLeft: %08x\n", hres);
    ok(V_VT(&vDefault) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&vDefault));
    ok(!strcmp_wa(V_BSTR(&vDefault), "1px"), "V_BSTR(v) = %s\n", wine_dbgstr_w(V_BSTR(&vDefault)));

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10");
    hres = IHTMLStyle_put_paddingLeft(style, v);
    ok(hres == S_OK, "put_paddingLeft: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_paddingLeft(style, &v);
    ok(hres == S_OK, "get_paddingLeft: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expecte 10 = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_paddingLeft(style, vDefault);
    ok(hres == S_OK, "put_paddingLeft: %08x\n", hres);

    /* BackgroundRepeat */
    hres = IHTMLStyle_get_backgroundRepeat(style, &sDefault);
    ok(hres == S_OK, "get_backgroundRepeat failed: %08x\n", hres);

    str = a2bstr("invalid");
    hres = IHTMLStyle_put_backgroundRepeat(style, str);
    ok(hres == E_INVALIDARG, "put_backgroundRepeat failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("repeat");
    hres = IHTMLStyle_put_backgroundRepeat(style, str);
    ok(hres == S_OK, "put_backgroundRepeat failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("no-repeat");
    hres = IHTMLStyle_put_backgroundRepeat(style, str);
    ok(hres == S_OK, "put_backgroundRepeat failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("repeat-x");
    hres = IHTMLStyle_put_backgroundRepeat(style, str);
    ok(hres == S_OK, "put_backgroundRepeat failed: %08x\n", hres);
    SysFreeString(str);

    str = a2bstr("repeat-y");
    hres = IHTMLStyle_put_backgroundRepeat(style, str);
    ok(hres == S_OK, "put_backgroundRepeat failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_backgroundRepeat(style, &str);
    ok(hres == S_OK, "get_backgroundRepeat failed: %08x\n", hres);
    ok(!strcmp_wa(str, "repeat-y"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_backgroundRepeat(style, sDefault);
    ok(hres == S_OK, "put_backgroundRepeat failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* BorderColor */
    hres = IHTMLStyle_get_borderColor(style, &sDefault);
    ok(hres == S_OK, "get_borderColor failed: %08x\n", hres);

    str = a2bstr("red green red blue");
    hres = IHTMLStyle_put_borderColor(style, str);
    ok(hres == S_OK, "put_borderColor failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_borderColor(style, &str);
    ok(hres == S_OK, "get_borderColor failed: %08x\n", hres);
    ok(!strcmp_wa(str, "red green red blue"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_borderColor(style, sDefault);
    ok(hres == S_OK, "put_borderColor failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* BorderRight */
    hres = IHTMLStyle_get_borderRight(style, &sDefault);
    ok(hres == S_OK, "get_borderRight failed: %08x\n", hres);

    str = a2bstr("thick dotted red");
    hres = IHTMLStyle_put_borderRight(style, str);
    ok(hres == S_OK, "put_borderRight failed: %08x\n", hres);
    SysFreeString(str);

    /* IHTMLStyle_get_borderRight appears to have a bug where
        it returns the first letter of the color.  So we check
        each style individually.
     */
    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderRightColor(style, &v);
    ok(hres == S_OK, "get_borderRightColor failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "red"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderRightWidth(style, &v);
    ok(hres == S_OK, "get_borderRightWidth failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "thick"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_borderRightStyle(style, &str);
    ok(hres == S_OK, "get_borderRightStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "dotted"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_borderRight(style, sDefault);
    ok(hres == S_OK, "put_borderRight failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* BorderTop */
    hres = IHTMLStyle_get_borderTop(style, &sDefault);
    ok(hres == S_OK, "get_borderTop failed: %08x\n", hres);

    str = a2bstr("thick dotted red");
    hres = IHTMLStyle_put_borderTop(style, str);
    ok(hres == S_OK, "put_borderTop failed: %08x\n", hres);
    SysFreeString(str);

    /* IHTMLStyle_get_borderTop appears to have a bug where
        it returns the first letter of the color.  So we check
        each style individually.
     */
    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderTopColor(style, &v);
    ok(hres == S_OK, "get_borderTopColor failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "red"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderTopWidth(style, &v);
    ok(hres == S_OK, "get_borderTopWidth failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "thick"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_borderTopStyle(style, &str);
    ok(hres == S_OK, "get_borderTopStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "dotted"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_borderTop(style, sDefault);
    ok(hres == S_OK, "put_borderTop failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* BorderBottom */
    hres = IHTMLStyle_get_borderBottom(style, &sDefault);
    ok(hres == S_OK, "get_borderBottom failed: %08x\n", hres);

    str = a2bstr("thick dotted red");
    hres = IHTMLStyle_put_borderBottom(style, str);
    ok(hres == S_OK, "put_borderBottom failed: %08x\n", hres);
    SysFreeString(str);

    /* IHTMLStyle_get_borderBottom appears to have a bug where
        it returns the first letter of the color.  So we check
        each style individually.
     */
    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderBottomColor(style, &v);
    ok(hres == S_OK, "get_borderBottomColor failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "red"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderBottomWidth(style, &v);
    ok(hres == S_OK, "get_borderBottomWidth failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "thick"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_borderBottomStyle(style, &str);
    ok(hres == S_OK, "get_borderBottomStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "dotted"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_borderBottom(style, sDefault);
    ok(hres == S_OK, "put_borderBottom failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* BorderLeft */
    hres = IHTMLStyle_get_borderLeft(style, &sDefault);
    ok(hres == S_OK, "get_borderLeft failed: %08x\n", hres);

    str = a2bstr("thick dotted red");
    hres = IHTMLStyle_put_borderLeft(style, str);
    ok(hres == S_OK, "put_borderLeft failed: %08x\n", hres);
    SysFreeString(str);

    /* IHTMLStyle_get_borderLeft appears to have a bug where
        it returns the first letter of the color.  So we check
        each style individually.
     */
    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderLeftColor(style, &v);
    ok(hres == S_OK, "get_borderLeftColor failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "red"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_BSTR(&v) = NULL;
    hres = IHTMLStyle_get_borderLeftWidth(style, &v);
    ok(hres == S_OK, "get_borderLeftWidth failed: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "thick"), "str=%s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_borderLeftStyle(style, &str);
    ok(hres == S_OK, "get_borderLeftStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "dotted"), "str=%s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_put_borderLeft(style, sDefault);
    ok(hres == S_OK, "put_borderLeft failed: %08x\n", hres);
    SysFreeString(sDefault);

    /* backgroundPositionX */
    hres = IHTMLStyle_get_backgroundPositionX(style, &v);
    ok(hres == S_OK, "get_backgroundPositionX failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!V_BSTR(&v), "backgroundPositionX = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10px");
    hres = IHTMLStyle_put_backgroundPositionX(style, v);
    ok(hres == S_OK, "put_backgroundPositionX failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_backgroundPositionX(style, &v);
    ok(hres == S_OK, "get_backgroundPositionX failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "backgroundPositionX = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* backgroundPositionY */
    hres = IHTMLStyle_get_backgroundPositionY(style, &v);
    ok(hres == S_OK, "get_backgroundPositionY failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    VariantClear(&v);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("15px");
    hres = IHTMLStyle_put_backgroundPositionY(style, v);
    ok(hres == S_OK, "put_backgroundPositionY failed: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_backgroundPositionY(style, &v);
    ok(hres == S_OK, "get_backgroundPositionY failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "15px"), "backgroundPositionY = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    /* backgroundPosition */
    str = NULL;
    hres = IHTMLStyle_get_backgroundPosition(style, &str);
    ok(hres == S_OK, "get_backgroundPosition failed: %08x\n", hres);
    ok(!strcmp_wa(str, "10px 15px"), "backgroundPosition = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("center 20%");
    hres = IHTMLStyle_put_backgroundPosition(style, str);
    ok(hres == S_OK, "put_backgroundPosition failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle_get_backgroundPosition(style, &str);
    ok(hres == S_OK, "get_backgroundPosition failed: %08x\n", hres);
    ok(!strcmp_wa(str, "center 20%"), "backgroundPosition = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLStyle_get_backgroundPositionX(style, &v);
    ok(hres == S_OK, "get_backgroundPositionX failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "center"), "backgroundPositionX = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_get_backgroundPositionY(style, &v);
    ok(hres == S_OK, "get_backgroundPositionY failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "20%"), "backgroundPositionY = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

     /* borderTopWidth */
    hres = IHTMLStyle_get_borderTopWidth(style, &vDefault);
    ok(hres == S_OK, "get_borderTopWidth: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10px");
    hres = IHTMLStyle_put_borderTopWidth(style, v);
    ok(hres == S_OK, "put_borderTopWidth: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderTopWidth(style, &v);
    ok(hres == S_OK, "get_borderTopWidth: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expected 10px = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderTopWidth(style, vDefault);
    ok(hres == S_OK, "put_borderTopWidth: %08x\n", hres);
    VariantClear(&vDefault);

    /* borderRightWidth */
    hres = IHTMLStyle_get_borderRightWidth(style, &vDefault);
    ok(hres == S_OK, "get_borderRightWidth: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10");
    hres = IHTMLStyle_put_borderRightWidth(style, v);
    ok(hres == S_OK, "put_borderRightWidth: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderRightWidth(style, &v);
    ok(hres == S_OK, "get_borderRightWidth: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expected 10px = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderRightWidth(style, vDefault);
    ok(hres == S_OK, "put_borderRightWidth: %08x\n", hres);
    VariantClear(&vDefault);

    /* borderBottomWidth */
    hres = IHTMLStyle_get_borderBottomWidth(style, &vDefault);
    ok(hres == S_OK, "get_borderBottomWidth: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10");
    hres = IHTMLStyle_put_borderBottomWidth(style, v);
    ok(hres == S_OK, "put_borderBottomWidth: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderBottomWidth(style, &v);
    ok(hres == S_OK, "get_borderBottomWidth: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expected 10px = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderBottomWidth(style, vDefault);
    ok(hres == S_OK, "put_borderBottomWidth: %08x\n", hres);
    VariantClear(&vDefault);

    /* borderLeftWidth */
    hres = IHTMLStyle_get_borderLeftWidth(style, &vDefault);
    ok(hres == S_OK, "get_borderLeftWidth: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10");
    hres = IHTMLStyle_put_borderLeftWidth(style, v);
    ok(hres == S_OK, "put_borderLeftWidth: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderLeftWidth(style, &v);
    ok(hres == S_OK, "get_borderLeftWidth: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expected 10px = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderLeftWidth(style, vDefault);
    ok(hres == S_OK, "put_borderLeftWidth: %08x\n", hres);
    VariantClear(&vDefault);

    /* wordSpacing */
    hres = IHTMLStyle_get_wordSpacing(style, &vDefault);
    ok(hres == S_OK, "get_wordSpacing: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("10");
    hres = IHTMLStyle_put_wordSpacing(style, v);
    ok(hres == S_OK, "put_wordSpacing: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_wordSpacing(style, &v);
    ok(hres == S_OK, "get_wordSpacing: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "10px"), "expected 10px = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_wordSpacing(style, vDefault);
    ok(hres == S_OK, "put_wordSpacing: %08x\n", hres);
    VariantClear(&vDefault);

    /* letterSpacing */
    hres = IHTMLStyle_get_letterSpacing(style, &vDefault);
    ok(hres == S_OK, "get_letterSpacing: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("11");
    hres = IHTMLStyle_put_letterSpacing(style, v);
    ok(hres == S_OK, "put_letterSpacing: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_letterSpacing(style, &v);
    ok(hres == S_OK, "get_letterSpacing: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v)=%d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "11px"), "expected 10px = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_letterSpacing(style, vDefault);
    ok(hres == S_OK, "put_letterSpacing: %08x\n", hres);
    VariantClear(&vDefault);

    /* borderTopColor */
    hres = IHTMLStyle_get_borderTopColor(style, &vDefault);
    ok(hres == S_OK, "get_borderTopColor: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("red");
    hres = IHTMLStyle_put_borderTopColor(style, v);
    ok(hres == S_OK, "put_borderTopColor: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderTopColor(style, &v);
    ok(hres == S_OK, "get_borderTopColor: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "red"), "expecte red = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderTopColor(style, vDefault);
    ok(hres == S_OK, "put_borderTopColor: %08x\n", hres);

	/* borderRightColor */
    hres = IHTMLStyle_get_borderRightColor(style, &vDefault);
    ok(hres == S_OK, "get_borderRightColor: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("blue");
    hres = IHTMLStyle_put_borderRightColor(style, v);
    ok(hres == S_OK, "put_borderRightColor: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderRightColor(style, &v);
    ok(hres == S_OK, "get_borderRightColor: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "blue"), "expected blue = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderRightColor(style, vDefault);
    ok(hres == S_OK, "putborderRightColorr: %08x\n", hres);

    /* borderBottomColor */
    hres = IHTMLStyle_get_borderBottomColor(style, &vDefault);
    ok(hres == S_OK, "get_borderBottomColor: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("cyan");
    hres = IHTMLStyle_put_borderBottomColor(style, v);
    ok(hres == S_OK, "put_borderBottomColor: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderBottomColor(style, &v);
    ok(hres == S_OK, "get_borderBottomColor: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "cyan"), "expected cyan = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderBottomColor(style, vDefault);
    ok(hres == S_OK, "put_borderBottomColor: %08x\n", hres);

    /* borderLeftColor */
    hres = IHTMLStyle_get_borderLeftColor(style, &vDefault);
    ok(hres == S_OK, "get_borderLeftColor: %08x\n", hres);

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = a2bstr("cyan");
    hres = IHTMLStyle_put_borderLeftColor(style, v);
    ok(hres == S_OK, "put_borderLeftColor: %08x\n", hres);
    VariantClear(&v);

    hres = IHTMLStyle_get_borderLeftColor(style, &v);
    ok(hres == S_OK, "get_borderLeftColor: %08x\n", hres);
    ok(!strcmp_wa(V_BSTR(&v), "cyan"), "expected cyan = %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLStyle_put_borderLeftColor(style, vDefault);
    ok(hres == S_OK, "put_borderLeftColor: %08x\n", hres);

    /* clip */
    hres = IHTMLStyle_get_clip(style, &str);
    ok(hres == S_OK, "get_clip failed: %08x\n", hres);
    ok(!str, "clip = %s\n", wine_dbgstr_w(str));

    str = a2bstr("rect(0px 1px 500px 505px)");
    hres = IHTMLStyle_put_clip(style, str);
    ok(hres == S_OK, "put_clip failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_clip(style, &str);
    ok(hres == S_OK, "get_clip failed: %08x\n", hres);
    ok(!strcmp_wa(str, "rect(0px 1px 500px 505px)"), "clip = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* clear */
    hres = IHTMLStyle_get_clear(style, &str);
    ok(hres == S_OK, "get_clear failed: %08x\n", hres);
    ok(!str, "clear = %s\n", wine_dbgstr_w(str));

    str = a2bstr("both");
    hres = IHTMLStyle_put_clear(style, str);
    ok(hres == S_OK, "put_clear failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_clear(style, &str);
    ok(hres == S_OK, "get_clear failed: %08x\n", hres);
    ok(!strcmp_wa(str, "both"), "clear = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* pageBreakAfter */
    hres = IHTMLStyle_get_pageBreakAfter(style, &str);
    ok(hres == S_OK, "get_pageBreakAfter failed: %08x\n", hres);
    ok(!str, "pageBreakAfter = %s\n", wine_dbgstr_w(str));

    str = a2bstr("always");
    hres = IHTMLStyle_put_pageBreakAfter(style, str);
    ok(hres == S_OK, "put_pageBreakAfter failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_pageBreakAfter(style, &str);
    ok(hres == S_OK, "get_pageBreakAfter failed: %08x\n", hres);
    ok(!strcmp_wa(str, "always"), "pageBreakAfter = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* pageBreakBefore */
    hres = IHTMLStyle_get_pageBreakBefore(style, &str);
    ok(hres == S_OK, "get_pageBreakBefore failed: %08x\n", hres);
    ok(!str, "pageBreakBefore = %s\n", wine_dbgstr_w(str));

    str = a2bstr("always");
    hres = IHTMLStyle_put_pageBreakBefore(style, str);
    ok(hres == S_OK, "put_pageBreakBefore failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_pageBreakBefore(style, &str);
    ok(hres == S_OK, "get_pageBreakBefore failed: %08x\n", hres);
    ok(!strcmp_wa(str, "always"), "pageBreakBefore = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    test_style_remove_attribute(style, "pageBreakBefore", VARIANT_TRUE);
    test_style_remove_attribute(style, "pageBreakBefore", VARIANT_FALSE);

    hres = IHTMLStyle_get_pageBreakBefore(style, &str);
    ok(hres == S_OK, "get_pageBreakBefore failed: %08x\n", hres);
    ok(!str, "pageBreakBefore = %s\n", wine_dbgstr_w(str));

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_whiteSpace(style, &str);
    ok(hres == S_OK, "get_whiteSpace failed: %08x\n", hres);
    ok(!str, "whiteSpace = %s\n", wine_dbgstr_w(str));

    str = a2bstr("nowrap");
    hres = IHTMLStyle_put_whiteSpace(style, str);
    SysFreeString(str);
    ok(hres == S_OK, "put_whiteSpace failed: %08x\n", hres);

    str = NULL;
    hres = IHTMLStyle_get_whiteSpace(style, &str);
    ok(hres == S_OK, "get_whiteSpace failed: %08x\n", hres);
    ok(!strcmp_wa(str, "nowrap"), "whiteSpace = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("normal");
    hres = IHTMLStyle_put_whiteSpace(style, str);
    SysFreeString(str);
    ok(hres == S_OK, "put_whiteSpace failed: %08x\n", hres);

    str = NULL;
    hres = IHTMLStyle_get_whiteSpace(style, &str);
    ok(hres == S_OK, "get_whiteSpace failed: %08x\n", hres);
    ok(!strcmp_wa(str, "normal"), "whiteSpace = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    /* listStyleType */
    hres = IHTMLStyle_get_listStyleType(style, &str);
    ok(hres == S_OK, "get_listStyleType failed: %08x\n", hres);
    ok(!str, "listStyleType = %s\n", wine_dbgstr_w(str));

    str = a2bstr("square");
    hres = IHTMLStyle_put_listStyleType(style, str);
    ok(hres == S_OK, "put_listStyleType failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle_get_listStyleType(style, &str);
    ok(hres == S_OK, "get_listStyleType failed: %08x\n", hres);
    ok(!strcmp_wa(str, "square"), "listStyleType = %s\n", wine_dbgstr_w(str));

    str = a2bstr("inside");
    hres = IHTMLStyle_put_listStylePosition(style, str);
    ok(hres == S_OK, "put_listStylePosition failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLStyle_get_listStylePosition(style, &str);
    ok(hres == S_OK, "get_listStylePosition failed: %08x\n", hres);
    ok(!strcmp_wa(str, "inside"), "listStyleType = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    str = a2bstr("decimal-leading-zero none inside");
    hres = IHTMLStyle_put_listStyle(style, str);
    ok(hres == S_OK || broken(hres == E_INVALIDARG), /* win 2000 */
        "put_listStyle(%s) failed: %08x\n", wine_dbgstr_w(str), hres);
    SysFreeString(str);

    if (hres != E_INVALIDARG) {
        hres = IHTMLStyle_get_listStyle(style, &str);
        ok(hres == S_OK, "get_listStyle failed: %08x\n", hres);
        ok(strstr_wa(str, "decimal-leading-zero") &&
           strstr_wa(str, "none") != NULL &&
           strstr_wa(str, "inside") != NULL,
            "listStyle = %s\n", wine_dbgstr_w(str));
        SysFreeString(str);
    }  else {
        win_skip("IHTMLStyle_put_listStyle already failed\n");
    }

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_styleFloat(style, &str);
    ok(hres == S_OK, "get_styleFloat failed: %08x\n", hres);
    ok(!str, "styleFloat = %s\n", wine_dbgstr_w(str));

    str = a2bstr("left");
    hres = IHTMLStyle_put_styleFloat(style, str);
    ok(hres == S_OK, "put_styleFloat failed: %08x\n", hres);
    SysFreeString(str);

    str = NULL;
    hres = IHTMLStyle_get_styleFloat(style, &str);
    ok(hres == S_OK, "get_styleFloat failed: %08x\n", hres);
    ok(!strcmp_wa(str, "left"), "styleFloat = %s\n", wine_dbgstr_w(str));

    hres = IHTMLStyle_QueryInterface(style, &IID_IHTMLStyle2, (void**)&style2);
    ok(hres == S_OK, "Could not get IHTMLStyle2 iface: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        test_style2(style2);
        IHTMLStyle2_Release(style2);
    }

    hres = IHTMLStyle_QueryInterface(style, &IID_IHTMLStyle3, (void**)&style3);
    ok(hres == S_OK, "Could not get IHTMLStyle3 iface: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        test_style3(style3);
        IHTMLStyle3_Release(style3);
    }

    hres = IHTMLStyle_QueryInterface(style, &IID_IHTMLStyle4, (void**)&style4);
    ok(hres == S_OK, "Could not get IHTMLStyle4 iface: %08x\n", hres);
    if(SUCCEEDED(hres)) {
        test_style4(style4);
        IHTMLStyle4_Release(style4);
    }

    hres = IHTMLStyle_QueryInterface(style, &IID_IHTMLStyle5, (void**)&style5);
    if(SUCCEEDED(hres)) {
        test_style5(style5);
        IHTMLStyle5_Release(style5);
    }else {
        win_skip("IHTMLStyle5 not available\n");
    }

    hres = IHTMLStyle_QueryInterface(style, &IID_IHTMLStyle6, (void**)&style6);
    if(SUCCEEDED(hres)) {
        test_style6(style6);
        IHTMLStyle6_Release(style6);
    }else {
        win_skip("IHTMLStyle6 not available\n");
    }
}

#define test_style_filter(a,b) _test_style_filter(__LINE__,a,b)
static void _test_style_filter(unsigned line, IHTMLStyle *style, const char *exval)
{
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLStyle_get_filter(style, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_filter failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line)(str && !strcmp_wa(str, exval), "filter = %s, expected %s\n", wine_dbgstr_w(str), exval);
    else
        ok_(__FILE__,line)(!str, "str = %s, expected NULL\n", wine_dbgstr_w(str));

    SysFreeString(str);
}

#define test_current_style_filter(a,b) _test_current_style_filter(__LINE__,a,b)
static void _test_current_style_filter(unsigned line, IHTMLCurrentStyle2 *style, const char *exval)
{
    BSTR str;
    HRESULT hres;

    str = (void*)0xdeadbeef;
    hres = IHTMLCurrentStyle2_get_filter(style, &str);
    ok_(__FILE__,line)(hres == S_OK, "get_filter failed: %08x\n", hres);
    if(exval)
        ok_(__FILE__,line)(str && !strcmp_wa(str, exval), "filter = %s, expected %s\n", wine_dbgstr_w(str), exval);
    else
        ok_(__FILE__,line)(!str, "str = %s, expected NULL\n", wine_dbgstr_w(str));

    SysFreeString(str);
}

#define set_style_filter(a,b) _set_style_filter(__LINE__,a,b)
static void _set_style_filter(unsigned line, IHTMLStyle *style, const char *val)
{
    BSTR str = a2bstr(val);
    HRESULT hres;

    hres = IHTMLStyle_put_filter(style, str);
    ok_(__FILE__,line)(hres == S_OK, "put_filter failed: %08x\n", hres);
    SysFreeString(str);

    _test_style_filter(line, style, val);
}

static void test_style_filters(IHTMLElement *elem)
{
    IHTMLElement2 *elem2 = get_elem2_iface((IUnknown*)elem);
    IHTMLCurrentStyle2 *current_style2;
    IHTMLCurrentStyle *current_style;
    IHTMLStyle *style;
    HRESULT hres;

    hres = IHTMLElement_get_style(elem, &style);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    hres = IHTMLElement2_get_currentStyle(elem2, &current_style);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    current_style2 = get_current_style2_iface((IUnknown*)current_style);

    test_style_filter(style, NULL);
    test_current_style_filter(current_style2, NULL);
    set_style_filter(style, "alpha(opacity=50.0040)");
    test_current_style_filter(current_style2, "alpha(opacity=50.0040)");
    set_style_filter(style, "alpha(opacity=100)");

    IHTMLStyle_Release(style);

    hres = IHTMLElement_get_style(elem, &style);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    test_style_filter(style, "alpha(opacity=100)");
    set_style_filter(style, "xxx(a,b,c) alpha(opacity=100)");
    set_style_filter(style, NULL);
    set_style_filter(style, "alpha(opacity=100)");
    test_style_remove_attribute(style, "filter", VARIANT_TRUE);
    test_style_remove_attribute(style, "filter", VARIANT_FALSE);
    test_style_filter(style, NULL);


    IHTMLCurrentStyle2_Release(current_style2);
    IHTMLStyle_Release(style);
    IHTMLElement2_Release(elem2);
}

static void test_current_style(IHTMLCurrentStyle *current_style)
{
    IHTMLCurrentStyle2 *current_style2;
    IHTMLCurrentStyle3 *current_style3;
    VARIANT_BOOL b;
    BSTR str;
    HRESULT hres;
    VARIANT v;

    hres = IHTMLCurrentStyle_get_display(current_style, &str);
    ok(hres == S_OK, "get_display failed: %08x\n", hres);
    ok(!strcmp_wa(str, "block"), "get_display returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_position(current_style, &str);
    ok(hres == S_OK, "get_position failed: %08x\n", hres);
    ok(!strcmp_wa(str, "absolute"), "get_position returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_fontFamily(current_style, &str);
    ok(hres == S_OK, "get_fontFamily failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_fontStyle(current_style, &str);
    ok(hres == S_OK, "get_fontStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "normal"), "get_fontStyle returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_backgroundImage(current_style, &str);
    ok(hres == S_OK, "get_backgroundImage failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none"), "get_backgroundImage returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_fontVariant(current_style, &str);
    ok(hres == S_OK, "get_fontVariant failed: %08x\n", hres);
    ok(!strcmp_wa(str, "normal"), "get_fontVariant returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderTopStyle(current_style, &str);
    ok(hres == S_OK, "get_borderTopStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none"), "get_borderTopStyle returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderRightStyle(current_style, &str);
    ok(hres == S_OK, "get_borderRightStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none"), "get_borderRightStyle returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderBottomStyle(current_style, &str);
    ok(hres == S_OK, "get_borderBottomStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none"), "get_borderBottomStyle returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderLeftStyle(current_style, &str);
    ok(hres == S_OK, "get_borderLeftStyle failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none"), "get_borderLeftStyle returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_textAlign(current_style, &str);
    ok(hres == S_OK, "get_textAlign failed: %08x\n", hres);
    ok(!strcmp_wa(str, "center"), "get_textAlign returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_textDecoration(current_style, &str);
    ok(hres == S_OK, "get_textDecoration failed: %08x\n", hres);
    ok(!strcmp_wa(str, "none"), "get_textDecoration returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_cursor(current_style, &str);
    ok(hres == S_OK, "get_cursor failed: %08x\n", hres);
    ok(!strcmp_wa(str, "default"), "get_cursor returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_backgroundRepeat(current_style, &str);
    ok(hres == S_OK, "get_backgroundRepeat failed: %08x\n", hres);
    ok(!strcmp_wa(str, "repeat"), "get_backgroundRepeat returned %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderColor(current_style, &str);
    ok(hres == S_OK, "get_borderColor failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderStyle(current_style, &str);
    ok(hres == S_OK, "get_borderStyle failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_visibility(current_style, &str);
    ok(hres == S_OK, "get_visibility failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_overflow(current_style, &str);
    ok(hres == S_OK, "get_overflow failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_borderWidth(current_style, &str);
    ok(hres == S_OK, "get_borderWidth failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_margin(current_style, &str);
    ok(hres == S_OK, "get_margin failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_padding(current_style, &str);
    ok(hres == S_OK, "get_padding failed: %08x\n", hres);
    SysFreeString(str);

    hres = IHTMLCurrentStyle_get_fontWeight(current_style, &v);
    ok(hres == S_OK, "get_fontWeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok( V_I4(&v) == 400, "expect 400 got (%d)\n", V_I4(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_fontSize(current_style, &v);
    ok(hres == S_OK, "get_fontSize failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_left(current_style, &v);
    ok(hres == S_OK, "get_left failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_top(current_style, &v);
    ok(hres == S_OK, "get_top failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_width(current_style, &v);
    ok(hres == S_OK, "get_width failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_height(current_style, &v);
    ok(hres == S_OK, "get_height failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_paddingLeft(current_style, &v);
    ok(hres == S_OK, "get_paddingLeft failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_zIndex(current_style, &v);
    ok(hres == S_OK, "get_zIndex failed: %08x\n", hres);
    ok(V_VT(&v) == VT_I4, "V_VT(v) = %d\n", V_VT(&v));
    ok( V_I4(&v) == 1, "expect 1 got (%d)\n", V_I4(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_verticalAlign(current_style, &v);
    ok(hres == S_OK, "get_verticalAlign failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    ok(!strcmp_wa(V_BSTR(&v), "100px"), "get_verticalAlign returned %s\n", wine_dbgstr_w(V_BSTR(&v)));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_marginRight(current_style, &v);
    ok(hres == S_OK, "get_marginRight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_marginLeft(current_style, &v);
    ok(hres == S_OK, "get_marginLeft failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderLeftWidth(current_style, &v);
    ok(hres == S_OK, "get_borderLeftWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    V_BSTR(&v) = NULL;
    hres = IHTMLCurrentStyle_get_borderRightWidth(current_style, &v);
    ok(hres == S_OK, "get_borderRightWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderBottomWidth(current_style, &v);
    ok(hres == S_OK, "get_borderBottomWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderTopWidth(current_style, &v);
    ok(hres == S_OK, "get_borderTopWidth failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_color(current_style, &v);
    ok(hres == S_OK, "get_color failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_backgroundColor(current_style, &v);
    ok(hres == S_OK, "get_backgroundColor failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderLeftColor(current_style, &v);
    ok(hres == S_OK, "get_borderLeftColor failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderTopColor(current_style, &v);
    ok(hres == S_OK, "get_borderTopColor failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderRightColor(current_style, &v);
    ok(hres == S_OK, "get_borderRightColor failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_borderBottomColor(current_style, &v);
    ok(hres == S_OK, "get_borderBottomColor failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_paddingTop(current_style, &v);
    ok(hres == S_OK, "get_paddingTop failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_paddingRight(current_style, &v);
    ok(hres == S_OK, "get_paddingRight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_paddingBottom(current_style, &v);
    ok(hres == S_OK, "get_paddingRight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_letterSpacing(current_style, &v);
    ok(hres == S_OK, "get_letterSpacing failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_marginTop(current_style, &v);
    ok(hres == S_OK, "get_marginTop failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_marginBottom(current_style, &v);
    ok(hres == S_OK, "get_marginBottom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_right(current_style, &v);
    ok(hres == S_OK, "get_Right failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_bottom(current_style, &v);
    ok(hres == S_OK, "get_bottom failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_lineHeight(current_style, &v);
    ok(hres == S_OK, "get_lineHeight failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_textIndent(current_style, &v);
    ok(hres == S_OK, "get_textIndent failed: %08x\n", hres);
    ok(V_VT(&v) == VT_BSTR, "V_VT(v) = %d\n", V_VT(&v));
    VariantClear(&v);

    hres = IHTMLCurrentStyle_get_textTransform(current_style, &str);
    ok(hres == S_OK, "get_textTransform failed: %08x\n", hres);
    SysFreeString(str);

    current_style2 = get_current_style2_iface((IUnknown*)current_style);

    b = 100;
    hres = IHTMLCurrentStyle2_get_hasLayout(current_style2, &b);
    ok(hres == S_OK, "get_hasLayout failed: %08x\n", hres);
    ok(b == VARIANT_TRUE, "hasLayout = %x\n", b);

    IHTMLCurrentStyle2_Release(current_style2);

    hres = IHTMLCurrentStyle_QueryInterface(current_style, &IID_IHTMLCurrentStyle3, (void**)&current_style3);
    ok(hres == S_OK, "Could not get IHTMLCurrentStyle3 iface: %08x\n", hres);

    hres = IHTMLCurrentStyle3_get_whiteSpace(current_style3, &str);
    ok(hres == S_OK, "get_whiteSpace failed: %08x\n", hres);
    ok(!strcmp_wa(str, "normal"), "whiteSpace = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IHTMLCurrentStyle3_Release(current_style3);
}

static const char basic_test_str[] = "<html><body><div id=\"divid\"></div/</body></html>";

static void basic_style_test(IHTMLDocument2 *doc)
{
    IHTMLCurrentStyle *cstyle;
    IHTMLElement *elem;
    IHTMLStyle *style;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    hres = IHTMLElement_get_style(elem, &style);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    test_body_style(style);

    cstyle = get_current_style(elem);
    test_current_style(cstyle);
    IHTMLCurrentStyle_Release(cstyle);
    IHTMLElement_Release(elem);

    elem = get_element_by_id(doc, "divid");
    test_style_filters(elem);

    test_set_csstext(style);
    IHTMLStyle_Release(style);
    IHTMLElement_Release(elem);
}

static const char runtimestyle_test_str[] =
    "<html><head><style>body {text-decoration: auto}</style></head><body></body></html>";

static void runtimestyle_test(IHTMLDocument2 *doc)
{
    IHTMLStyle *style, *runtime_style;
    IHTMLElement2 *elem2;
    IHTMLElement *elem;
    BSTR str;
    HRESULT hres;

    hres = IHTMLDocument2_get_body(doc, &elem);
    ok(hres == S_OK, "get_body failed: %08x\n", hres);

    elem2 = get_elem2_iface((IUnknown*)elem);

    hres = IHTMLElement2_get_runtimeStyle(elem2, &runtime_style);
    ok(hres == S_OK, "get_runtimeStyle failed: %08x\n", hres);

    hres = IHTMLElement_get_style(elem, &style);
    ok(hres == S_OK, "get_style failed: %08x\n", hres);

    test_text_decoration(style, NULL);
    test_text_decoration(runtime_style, NULL);
    set_text_decoration(style, "underline");
    test_text_decoration(style, "underline");

    hres = IHTMLStyle_get_textDecoration(style, &str);
    ok(hres == S_OK, "get_textDecoration failed: %08x\n", hres);
    ok(broken(!str) || !strcmp_wa(str, "underline"), "textDecoration = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    set_text_decoration(runtime_style, "blink");
    test_text_decoration(runtime_style, "blink");

    hres = IHTMLStyle_get_textDecoration(style, &str);
    ok(hres == S_OK, "get_textDecoration failed: %08x\n", hres);
    todo_wine
    ok(!strcmp_wa(str, "underline"), "textDecoration = %s\n", wine_dbgstr_w(str));
    SysFreeString(str);

    IHTMLStyle_Release(runtime_style);
    IHTMLStyle_Release(style);
    IHTMLElement2_Release(elem2);
    IHTMLElement_Release(elem);
}

static IHTMLDocument2 *notif_doc;
static BOOL doc_complete;

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

typedef void (*style_test_t)(IHTMLDocument2*);

static void run_test(const char *str, style_test_t test)
{
    IHTMLDocument2 *doc;
    ULONG ref;
    MSG msg;

    doc = create_doc_with_string(str);
    if(!doc)
        return;

    do_advise((IUnknown*)doc, &IID_IPropertyNotifySink, (IUnknown*)&PropertyNotifySink);

    while(!doc_complete && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    test(doc);

    ref = IHTMLDocument2_Release(doc);
    ok(!ref || broken(ref == 1), /* Vista */
       "ref = %d\n", ref);
}


START_TEST(style)
{
    CoInitialize(NULL);

    run_test(basic_test_str, basic_style_test);
    run_test(runtimestyle_test_str, runtimestyle_test);

    CoUninitialize();
}
