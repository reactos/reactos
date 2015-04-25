/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "vbscript.h"

#include <math.h>
#include <wingdi.h>
#include <mshtmhst.h>

#define round(x) (((x) < 0) ? (int)((x) - 0.5) : (int)((x) + 0.5))

#define VB_E_CANNOT_CREATE_OBJ 0x800a01ad
#define VB_E_MK_PARSE_ERROR    0x800a01b0

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

static const WCHAR emptyW[] = {0};
static const WCHAR vbscriptW[] = {'V','B','S','c','r','i','p','t',0};

static IInternetHostSecurityManager *get_sec_mgr(script_ctx_t *ctx)
{
    IInternetHostSecurityManager *secmgr;
    IServiceProvider *sp;
    HRESULT hres;

    if(!ctx->site)
        return NULL;

    if(ctx->secmgr)
        return ctx->secmgr;

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IServiceProvider, (void**)&sp);
    if(FAILED(hres))
        return NULL;

    hres = IServiceProvider_QueryService(sp, &SID_SInternetHostSecurityManager, &IID_IInternetHostSecurityManager,
            (void**)&secmgr);
    IServiceProvider_Release(sp);
    if(FAILED(hres))
        return NULL;

    return ctx->secmgr = secmgr;
}

static HRESULT return_string(VARIANT *res, const WCHAR *str)
{
    BSTR ret;

    if(!res)
        return S_OK;

    ret = SysAllocString(str);
    if(!ret)
        return E_OUTOFMEMORY;

    V_VT(res) = VT_BSTR;
    V_BSTR(res) = ret;
    return S_OK;
}

static HRESULT return_bstr(VARIANT *res, BSTR str)
{
    if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT return_bool(VARIANT *res, BOOL val)
{
    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = val ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT return_short(VARIANT *res, short val)
{
    if(res) {
        V_VT(res) = VT_I2;
        V_I2(res) = val;
    }

    return S_OK;
}

static HRESULT return_int(VARIANT *res, int val)
{
    if(res) {
        V_VT(res) = VT_I4;
        V_I4(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_double(VARIANT *res, double val)
{
    if(res) {
        V_VT(res) = VT_R8;
        V_R8(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_float(VARIANT *res, float val)
{
    if(res) {
        V_VT(res) = VT_R4;
        V_R4(res) = val;
    }

    return S_OK;
}

static inline HRESULT return_null(VARIANT *res)
{
    if(res)
        V_VT(res) = VT_NULL;
    return S_OK;
}

static inline HRESULT return_date(VARIANT *res, double date)
{
    if(res) {
        V_VT(res) = VT_DATE;
        V_DATE(res) = date;
    }
    return S_OK;
}

HRESULT to_int(VARIANT *v, int *ret)
{
    VARIANT r;
    HRESULT hres;

    V_VT(&r) = VT_EMPTY;
    hres = VariantChangeType(&r, v, 0, VT_I4);
    if(FAILED(hres))
        return hres;

    *ret = V_I4(&r);
    return S_OK;
}

static HRESULT to_double(VARIANT *v, double *ret)
{
    VARIANT dst;
    HRESULT hres;

    V_VT(&dst) = VT_EMPTY;
    hres = VariantChangeType(&dst, v, 0, VT_R8);
    if(FAILED(hres))
        return hres;

    *ret = V_R8(&dst);
    return S_OK;
}

static HRESULT to_string(VARIANT *v, BSTR *ret)
{
    VARIANT dst;
    HRESULT hres;

    V_VT(&dst) = VT_EMPTY;
    hres = VariantChangeType(&dst, v, VARIANT_LOCALBOOL, VT_BSTR);
    if(FAILED(hres))
        return hres;

    *ret = V_BSTR(&dst);
    return S_OK;
}

static HRESULT set_object_site(script_ctx_t *ctx, IUnknown *obj)
{
    IObjectWithSite *obj_site;
    IUnknown *ax_site;
    HRESULT hres;

    hres = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void**)&obj_site);
    if(FAILED(hres))
        return S_OK;

    ax_site = create_ax_site(ctx);
    if(ax_site) {
        hres = IObjectWithSite_SetSite(obj_site, ax_site);
        IUnknown_Release(ax_site);
    }
    else
        hres = E_OUTOFMEMORY;
    IObjectWithSite_Release(obj_site);
    return hres;
}

static IUnknown *create_object(script_ctx_t *ctx, const WCHAR *progid)
{
    IInternetHostSecurityManager *secmgr = NULL;
    struct CONFIRMSAFETY cs;
    IClassFactoryEx *cfex;
    IClassFactory *cf;
    DWORD policy_size;
    BYTE *bpolicy;
    IUnknown *obj;
    DWORD policy;
    GUID guid;
    HRESULT hres;

    hres = CLSIDFromProgID(progid, &guid);
    if(FAILED(hres))
        return NULL;

    TRACE("GUID %s\n", debugstr_guid(&guid));

    if(ctx->safeopt & INTERFACE_USES_SECURITY_MANAGER) {
        secmgr = get_sec_mgr(ctx);
        if(!secmgr)
            return NULL;

        policy = 0;
        hres = IInternetHostSecurityManager_ProcessUrlAction(secmgr, URLACTION_ACTIVEX_RUN,
                (BYTE*)&policy, sizeof(policy), (BYTE*)&guid, sizeof(GUID), 0, 0);
        if(FAILED(hres) || policy != URLPOLICY_ALLOW)
            return NULL;
    }

    hres = CoGetClassObject(&guid, CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER, NULL, &IID_IClassFactory, (void**)&cf);
    if(FAILED(hres))
        return NULL;

    hres = IClassFactory_QueryInterface(cf, &IID_IClassFactoryEx, (void**)&cfex);
    if(SUCCEEDED(hres)) {
        FIXME("Use IClassFactoryEx\n");
        IClassFactoryEx_Release(cfex);
    }

    hres = IClassFactory_CreateInstance(cf, NULL, &IID_IUnknown, (void**)&obj);
    if(FAILED(hres))
        return NULL;

    if(secmgr) {
        cs.clsid = guid;
        cs.pUnk = obj;
        cs.dwFlags = 0;
        hres = IInternetHostSecurityManager_QueryCustomPolicy(secmgr, &GUID_CUSTOM_CONFIRMOBJECTSAFETY,
                &bpolicy, &policy_size, (BYTE*)&cs, sizeof(cs), 0);
        if(SUCCEEDED(hres)) {
            policy = policy_size >= sizeof(DWORD) ? *(DWORD*)bpolicy : URLPOLICY_DISALLOW;
            CoTaskMemFree(bpolicy);
        }

        if(FAILED(hres) || policy != URLPOLICY_ALLOW) {
            IUnknown_Release(obj);
            return NULL;
        }
    }

    hres = set_object_site(ctx, obj);
    if(FAILED(hres)) {
        IUnknown_Release(obj);
        return NULL;
    }

    return obj;
}

static HRESULT show_msgbox(script_ctx_t *ctx, BSTR prompt, unsigned type, BSTR orig_title, VARIANT *res)
{
    SCRIPTUICHANDLING uic_handling = SCRIPTUICHANDLING_ALLOW;
    IActiveScriptSiteUIControl *ui_control;
    IActiveScriptSiteWindow *acts_window;
    WCHAR *title_buf = NULL;
    const WCHAR *title;
    HWND hwnd = NULL;
    int ret = 0;
    HRESULT hres;

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IActiveScriptSiteUIControl, (void**)&ui_control);
    if(SUCCEEDED(hres)) {
        hres = IActiveScriptSiteUIControl_GetUIBehavior(ui_control, SCRIPTUICITEM_MSGBOX, &uic_handling);
        IActiveScriptSiteUIControl_Release(ui_control);
        if(FAILED(hres))
            uic_handling = SCRIPTUICHANDLING_ALLOW;
    }

    switch(uic_handling) {
    case SCRIPTUICHANDLING_ALLOW:
        break;
    case SCRIPTUICHANDLING_NOUIDEFAULT:
        return return_short(res, 0);
    default:
        FIXME("blocked\n");
        return E_FAIL;
    }

    hres = IActiveScriptSite_QueryInterface(ctx->site, &IID_IActiveScriptSiteWindow, (void**)&acts_window);
    if(FAILED(hres)) {
        FIXME("No IActiveScriptSiteWindow\n");
        return hres;
    }

    if(ctx->safeopt & INTERFACE_USES_SECURITY_MANAGER) {
        if(orig_title && *orig_title) {
            WCHAR *ptr;

            title = title_buf = heap_alloc(sizeof(vbscriptW) + (strlenW(orig_title)+2)*sizeof(WCHAR));
            if(!title)
                return E_OUTOFMEMORY;

            memcpy(title_buf, vbscriptW, sizeof(vbscriptW));
            ptr = title_buf + sizeof(vbscriptW)/sizeof(WCHAR)-1;

            *ptr++ = ':';
            *ptr++ = ' ';
            strcpyW(ptr, orig_title);
        }else {
            title = vbscriptW;
        }
    }else {
        title = orig_title ? orig_title : emptyW;
    }

    hres = IActiveScriptSiteWindow_GetWindow(acts_window, &hwnd);
    if(SUCCEEDED(hres)) {
        hres = IActiveScriptSiteWindow_EnableModeless(acts_window, FALSE);
        if(SUCCEEDED(hres)) {
            ret = MessageBoxW(hwnd, prompt, title, type);
            hres = IActiveScriptSiteWindow_EnableModeless(acts_window, TRUE);
        }
    }

    heap_free(title_buf);
    IActiveScriptSiteWindow_Release(acts_window);
    if(FAILED(hres)) {
        FIXME("failed: %08x\n", hres);
        return hres;
    }

    return return_short(res, ret);
}

static HRESULT Global_CCur(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_CY);
    if(FAILED(hres))
        return hres;

    if(!res) {
        VariantClear(&v);
        return DISP_E_BADVARTYPE;
    }

    *res = v;
    return S_OK;
}

static HRESULT Global_CInt(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_I2);
    if(FAILED(hres))
        return hres;

    if(!res)
        return DISP_E_BADVARTYPE;
    else {
        *res = v;
        return S_OK;
    }
}

static HRESULT Global_CLng(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    int i;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = to_int(arg, &i);
    if(FAILED(hres))
        return hres;
    if(!res)
        return DISP_E_BADVARTYPE;

    return return_int(res, i);
}

static HRESULT Global_CBool(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, VARIANT_LOCALBOOL, VT_BOOL);
    if(FAILED(hres))
        return hres;

    if(res)
        *res = v;
    else
        VariantClear(&v);
    return S_OK;
}

static HRESULT Global_CByte(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, VARIANT_LOCALBOOL, VT_UI1);
    if(FAILED(hres))
        return hres;

    if(!res) {
        VariantClear(&v);
        return DISP_E_BADVARTYPE;
    }

    *res = v;
    return S_OK;
}

static HRESULT Global_CDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CDbl(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_R8);
    if(FAILED(hres))
        return hres;

    if(!res)
        return DISP_E_BADVARTYPE;
    else {
        *res = v;
        return S_OK;
    }
}

static HRESULT Global_CSng(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARIANT v;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    V_VT(&v) = VT_EMPTY;
    hres = VariantChangeType(&v, arg, 0, VT_R4);
    if(FAILED(hres))
        return hres;

    if(!res)
        return DISP_E_BADVARTYPE;

   *res = v;
   return S_OK;
}

static HRESULT Global_CStr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, str);
}

static inline WCHAR hex_char(unsigned n)
{
    return n < 10 ? '0'+n : 'A'+n-10;
}

static HRESULT Global_Hex(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    WCHAR buf[17], *ptr;
    DWORD n;

    TRACE("%s\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_I2:
        n = (WORD)V_I2(arg);
        break;
    case VT_I4:
        n = V_I4(arg);
        break;
    case VT_EMPTY:
        n = 0;
        break;
    case VT_NULL:
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        FIXME("unsupported type %s\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    buf[16] = 0;
    ptr = buf+15;

    if(n) {
        do {
            *ptr-- = hex_char(n & 0xf);
            n >>= 4;
        }while(n);
        ptr++;
    }else {
        *ptr = '0';
    }

    return return_string(res, ptr);
}

static HRESULT Global_Oct(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_VarType(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(V_VT(arg) & ~VT_TYPEMASK) {
        FIXME("not supported %s\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    return return_short(res, V_VT(arg));
}

static HRESULT Global_IsDate(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsEmpty(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_EMPTY ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_IsNull(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_IsNumeric(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = to_double(arg, &d);

    return return_bool(res, SUCCEEDED(hres));
}

static HRESULT Global_IsArray(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_DISPATCH ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_Atn(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, atan(d));
}

static HRESULT Global_Cos(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, cos(d));
}

static HRESULT Global_Sin(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, sin(d));
}

static HRESULT Global_Tan(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, tan(d));
}

static HRESULT Global_Exp(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, exp(d));
}

static HRESULT Global_Log(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    if(d <= 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    else
        return return_double(res, log(d));
}

static HRESULT Global_Sqr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    if(d < 0)
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    else
        return return_double(res, sqrt(d));
}

static HRESULT Global_Randomize(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Rnd(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Timer(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    double sec;

    GetLocalTime(&lt);
    sec = lt.wHour * 3600 + lt.wMinute * 60 + lt.wSecond + lt.wMilliseconds / 1000.0;
    return return_float(res, sec);

}

static HRESULT Global_LBound(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_UBound(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_RGB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    int i, color[3];

    TRACE("%s %s %s\n", debugstr_variant(arg), debugstr_variant(arg + 1), debugstr_variant(arg + 2));

    assert(args_cnt == 3);

    for(i = 0; i < 3; i++) {
        hres = to_int(arg + i, color + i);
        if(FAILED(hres))
            return hres;
        if(color[i] > 255)
            color[i] = 255;
        if(color[i] < 0)
            return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    return return_int(res, RGB(color[0], color[1], color[2]));
}

static HRESULT Global_Len(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    DWORD len;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL)
        return return_null(res);

    if(V_VT(arg) != VT_BSTR) {
        BSTR str;

        hres = to_string(arg, &str);
        if(FAILED(hres))
            return hres;

        len = SysStringLen(str);
        SysFreeString(str);
    }else {
        len = SysStringLen(V_BSTR(arg));
    }

    return return_int(res, len);
}

static HRESULT Global_LenB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Left(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR str, ret, conv_str = NULL;
    int len, str_len;
    HRESULT hres;

    TRACE("(%s %s)\n", debugstr_variant(args+1), debugstr_variant(args));

    if(V_VT(args) == VT_BSTR) {
        str = V_BSTR(args);
    }else {
        hres = to_string(args, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    hres = to_int(args+1, &len);
    if(FAILED(hres))
        return hres;

    if(len < 0) {
        FIXME("len = %d\n", len);
        return E_FAIL;
    }

    str_len = SysStringLen(str);
    if(len > str_len)
        len = str_len;

    ret = SysAllocStringLen(str, len);
    SysFreeString(conv_str);
    if(!ret)
        return E_OUTOFMEMORY;

    return return_bstr(res, ret);
}

static HRESULT Global_LeftB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Right(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR str, ret, conv_str = NULL;
    int len, str_len;
    HRESULT hres;

    TRACE("(%s %s)\n", debugstr_variant(args), debugstr_variant(args+1));

    if(V_VT(args+1) == VT_BSTR) {
        str = V_BSTR(args);
    }else {
        hres = to_string(args, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    hres = to_int(args+1, &len);
    if(FAILED(hres))
        return hres;

    if(len < 0) {
        FIXME("len = %d\n", len);
        return E_FAIL;
    }

    str_len = SysStringLen(str);
    if(len > str_len)
        len = str_len;

    ret = SysAllocStringLen(str+str_len-len, len);
    SysFreeString(conv_str);
    if(!ret)
        return E_OUTOFMEMORY;

    return return_bstr(res, ret);
}

static HRESULT Global_RightB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Mid(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int len = -1, start, str_len;
    BSTR str;
    HRESULT hres;

    TRACE("(%s %s ...)\n", debugstr_variant(args), debugstr_variant(args+1));

    assert(args_cnt == 2 || args_cnt == 3);

    if(V_VT(args) != VT_BSTR) {
        FIXME("args[0] = %s\n", debugstr_variant(args));
        return E_NOTIMPL;
    }

    str = V_BSTR(args);

    hres = to_int(args+1, &start);
    if(FAILED(hres))
        return hres;

    if(args_cnt == 3) {
        hres = to_int(args+2, &len);
        if(FAILED(hres))
            return hres;

        if(len < 0) {
            FIXME("len = %d\n", len);
            return E_FAIL;
        }
    }


    str_len = SysStringLen(str);
    start--;
    if(start > str_len)
        start = str_len;

    if(len == -1)
        len = str_len-start;
    else if(len > str_len-start)
        len = str_len-start;

    if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = SysAllocStringLen(str+start, len);
        if(!V_BSTR(res))
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT Global_MidB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrComp(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_LCase(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL) {
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    }

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    if(res) {
        WCHAR *ptr;

        for(ptr = str; *ptr; ptr++)
            *ptr = tolowerW(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_UCase(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL) {
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    }

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    if(res) {
        WCHAR *ptr;

        for(ptr = str; *ptr; ptr++)
            *ptr = toupperW(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_LTrim(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str, conv_str = NULL;
    WCHAR *ptr;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_BSTR) {
        str = V_BSTR(arg);
    }else {
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    for(ptr = str; *ptr && isspaceW(*ptr); ptr++);

    str = SysAllocString(ptr);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_RTrim(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str, conv_str = NULL;
    WCHAR *ptr;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_BSTR) {
        str = V_BSTR(arg);
    }else {
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    for(ptr = str+SysStringLen(str); ptr-1 > str && isspaceW(*(ptr-1)); ptr--);

    str = SysAllocStringLen(str, ptr-str);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_Trim(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str, conv_str = NULL;
    WCHAR *begin_ptr, *end_ptr;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_BSTR) {
        str = V_BSTR(arg);
    }else {
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    for(begin_ptr = str; *begin_ptr && isspaceW(*begin_ptr); begin_ptr++);
    for(end_ptr = str+SysStringLen(str); end_ptr-1 > begin_ptr && isspaceW(*(end_ptr-1)); end_ptr--);

    str = SysAllocStringLen(begin_ptr, end_ptr-begin_ptr);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_Space(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    int n, i;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    hres = to_int(arg, &n);
    if(FAILED(hres))
        return hres;

    if(n < 0) {
        FIXME("n = %d\n", n);
        return E_NOTIMPL;
    }

    if(!res)
        return S_OK;

    str = SysAllocStringLen(NULL, n);
    if(!str)
        return E_OUTOFMEMORY;

    for(i=0; i<n; i++)
        str[i] = ' ';

    V_VT(res) = VT_BSTR;
    V_BSTR(res) = str;
    return S_OK;
}

static HRESULT Global_String(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InStr(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    VARIANT *startv, *str1v, *str2v;
    BSTR str1, str2;
    int start, ret;
    HRESULT hres;

    TRACE("\n");

    assert(2 <= args_cnt && args_cnt <= 4);

    switch(args_cnt) {
    case 2:
        startv = NULL;
        str1v = args;
        str2v = args+1;
        break;
    case 3:
        startv = args;
        str1v = args+1;
        str2v = args+2;
        break;
    case 4:
        FIXME("unsupported compare argument %s\n", debugstr_variant(args));
        return E_NOTIMPL;
    DEFAULT_UNREACHABLE;
    }

    if(startv) {
        hres = to_int(startv, &start);
        if(FAILED(hres))
            return hres;
        if(--start < 0) {
            FIXME("start %d\n", start);
            return E_FAIL;
        }
    }else {
        start = 0;
    }

    if(V_VT(str1v) == VT_NULL || V_VT(str2v) == VT_NULL)
        return return_null(res);

    if(V_VT(str1v) != VT_BSTR) {
        FIXME("Unsupported str1 type %s\n", debugstr_variant(str1v));
        return E_NOTIMPL;
    }
    str1 = V_BSTR(str1v);

    if(V_VT(str2v) != VT_BSTR) {
        FIXME("Unsupported str2 type %s\n", debugstr_variant(str2v));
        return E_NOTIMPL;
    }
    str2 = V_BSTR(str2v);

    if(start < SysStringLen(str1)) {
        WCHAR *ptr;

        ptr = strstrW(str1+start, str2);
        ret = ptr ? ptr-str1+1 : 0;
    }else {
        ret = 0;
    }

    return return_int(res, ret);
}

static HRESULT Global_InStrB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_AscB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrB(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Asc(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/* The function supports only single-byte and double-byte character sets. It
 * ignores language specified by IActiveScriptSite::GetLCID. The argument needs
 * to be in range of short or unsigned short. */
static HRESULT Global_Chr(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    int cp, c, len = 0;
    CPINFO cpi;
    WCHAR ch;
    char buf[2];
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    hres = to_int(arg, &c);
    if(FAILED(hres))
        return hres;

    cp = GetACP();
    if(!GetCPInfo(cp, &cpi))
        cpi.MaxCharSize = 1;

    if((c!=(short)c && c!=(unsigned short)c) ||
            (unsigned short)c>=(cpi.MaxCharSize>1 ? 0x10000 : 0x100)) {
        WARN("invalid arg %d\n", c);
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    }

    if(c>>8)
        buf[len++] = c>>8;
    if(!len || IsDBCSLeadByteEx(cp, buf[0]))
        buf[len++] = c;
    if(!MultiByteToWideChar(0, 0, buf, len, &ch, 1)) {
        WARN("invalid arg %d, cp %d\n", c, cp);
        return E_FAIL;
    }

    if(res) {
        V_VT(res) = VT_BSTR;
        V_BSTR(res) = SysAllocStringLen(&ch, 1);
        if(!V_BSTR(res))
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

static HRESULT Global_AscW(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrW(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Abs(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    VARIANT dst;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = VarAbs(arg, &dst);
    if(FAILED(hres))
        return hres;

    if (res)
        *res = dst;
    else
        VariantClear(&dst);

    return S_OK;
}

static HRESULT Global_Fix(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    VARIANT dst;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = VarFix(arg, &dst);
    if(FAILED(hres))
        return hres;

    if (res)
        *res = dst;
    else
        VariantClear(&dst);

    return S_OK;
}

static HRESULT Global_Int(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    VARIANT dst;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = VarInt(arg, &dst);
    if(FAILED(hres))
        return hres;

    if (res)
        *res = dst;
    else
        VariantClear(&dst);

    return S_OK;
}

static HRESULT Global_Sgn(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    double v;
    short val;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(V_VT(arg) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_double(arg, &v);
    if (FAILED(hres))
        return hres;

    val = v == 0 ? 0 : (v > 0 ? 1 : -1);
    return return_short(res, val);
}

static HRESULT Global_Now(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    double date;

    TRACE("\n");

    GetLocalTime(&lt);
    SystemTimeToVariantTime(&lt, &date);
    return return_date(res, date);
}

static HRESULT Global_Date(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    UDATE ud;
    DATE date;
    HRESULT hres;

    TRACE("\n");

    GetLocalTime(&lt);
    ud.st = lt;
    ud.wDayOfYear = 0;
    hres = VarDateFromUdateEx(&ud, 0, VAR_DATEVALUEONLY, &date);
    if(FAILED(hres))
        return hres;
    return return_date(res, date);
}

static HRESULT Global_Time(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    UDATE ud;
    DATE time;
    HRESULT hres;

    TRACE("\n");

    GetLocalTime(&lt);
    ud.st = lt;
    ud.wDayOfYear = 0;
    hres = VarDateFromUdateEx(&ud, 0, VAR_TIMEVALUEONLY, &time);
    if(FAILED(hres))
        return hres;
    return return_date(res, time);
}

static HRESULT Global_Day(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Month(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Weekday(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Year(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Hour(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Minute(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Second(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateValue(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeValue(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateSerial(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeSerial(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InputBox(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_MsgBox(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR prompt, title = NULL;
    int type = MB_OK;
    HRESULT hres;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 5);

    hres = to_string(args, &prompt);
    if(FAILED(hres))
        return hres;

    if(args_cnt > 1)
        hres = to_int(args+1, &type);

    if(SUCCEEDED(hres) && args_cnt > 2)
        hres = to_string(args+2, &title);

    if(SUCCEEDED(hres) && args_cnt > 3) {
        FIXME("unsupported arg_cnt %d\n", args_cnt);
        hres = E_NOTIMPL;
    }

    if(SUCCEEDED(hres))
        hres = show_msgbox(This->desc->ctx, prompt, type, title, res);

    SysFreeString(prompt);
    SysFreeString(title);
    return hres;
}

static HRESULT Global_CreateObject(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    IUnknown *obj;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    if(V_VT(arg) != VT_BSTR) {
        FIXME("non-bstr arg\n");
        return E_INVALIDARG;
    }

    obj = create_object(This->desc->ctx, V_BSTR(arg));
    if(!obj)
        return VB_E_CANNOT_CREATE_OBJ;

    if(res) {
        hres = IUnknown_QueryInterface(obj, &IID_IDispatch, (void**)&V_DISPATCH(res));
        if(FAILED(hres))
            return hres;

        V_VT(res) = VT_DISPATCH;
    }

    IUnknown_Release(obj);
    return S_OK;
}

static HRESULT Global_GetObject(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    IBindCtx *bind_ctx;
    IUnknown *obj_unk;
    IDispatch *disp;
    ULONG eaten = 0;
    IMoniker *mon;
    HRESULT hres;

    TRACE("%s %s\n", args_cnt ? debugstr_variant(args) : "", args_cnt > 1 ? debugstr_variant(args+1) : "");

    if(args_cnt != 1 || V_VT(args) != VT_BSTR) {
        FIXME("unsupported args\n");
        return E_NOTIMPL;
    }

    if(This->desc->ctx->safeopt & (INTERFACE_USES_SECURITY_MANAGER|INTERFACESAFE_FOR_UNTRUSTED_DATA)) {
        WARN("blocked in current safety mode\n");
        return VB_E_CANNOT_CREATE_OBJ;
    }

    hres = CreateBindCtx(0, &bind_ctx);
    if(FAILED(hres))
        return hres;

    hres = MkParseDisplayName(bind_ctx, V_BSTR(args), &eaten, &mon);
    if(SUCCEEDED(hres)) {
        hres = IMoniker_BindToObject(mon, bind_ctx, NULL, &IID_IUnknown, (void**)&obj_unk);
        IMoniker_Release(mon);
    }else {
        hres = MK_E_SYNTAX;
    }
    IBindCtx_Release(bind_ctx);
    if(FAILED(hres))
        return hres;

    hres = set_object_site(This->desc->ctx, obj_unk);
    if(FAILED(hres)) {
        IUnknown_Release(obj_unk);
        return hres;
    }

    hres = IUnknown_QueryInterface(obj_unk, &IID_IDispatch, (void**)&disp);
    if(SUCCEEDED(hres)) {
        if(res) {
            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = disp;
        }else {
            IDispatch_Release(disp);
        }
    }else {
        FIXME("object does not support IDispatch\n");
    }

    return hres;
}

static HRESULT Global_DateAdd(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateDiff(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DatePart(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TypeName(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    static const WCHAR ByteW[]     = {'B', 'y', 't', 'e', 0};
    static const WCHAR IntegerW[]  = {'I', 'n', 't', 'e', 'g', 'e', 'r', 0};
    static const WCHAR LongW[]     = {'L', 'o', 'n', 'g', 0};
    static const WCHAR SingleW[]   = {'S', 'i', 'n', 'g', 'l', 'e', 0};
    static const WCHAR DoubleW[]   = {'D', 'o', 'u', 'b', 'l', 'e', 0};
    static const WCHAR CurrencyW[] = {'C', 'u', 'r', 'r', 'e', 'n', 'c', 'y', 0};
    static const WCHAR DecimalW[]  = {'D', 'e', 'c', 'i', 'm', 'a', 'l', 0};
    static const WCHAR DateW[]     = {'D', 'a', 't', 'e', 0};
    static const WCHAR StringW[]   = {'S', 't', 'r', 'i', 'n', 'g', 0};
    static const WCHAR BooleanW[]  = {'B', 'o', 'o', 'l', 'e', 'a', 'n', 0};
    static const WCHAR EmptyW[]    = {'E', 'm', 'p', 't', 'y', 0};
    static const WCHAR NullW[]     = {'N', 'u', 'l', 'l', 0};

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    switch(V_VT(arg)) {
        case VT_UI1:
            return return_string(res, ByteW);
        case VT_I2:
            return return_string(res, IntegerW);
        case VT_I4:
            return return_string(res, LongW);
        case VT_R4:
            return return_string(res, SingleW);
        case VT_R8:
            return return_string(res, DoubleW);
        case VT_CY:
            return return_string(res, CurrencyW);
        case VT_DECIMAL:
            return return_string(res, DecimalW);
        case VT_DATE:
            return return_string(res, DateW);
        case VT_BSTR:
            return return_string(res, StringW);
        case VT_BOOL:
            return return_string(res, BooleanW);
        case VT_EMPTY:
            return return_string(res, EmptyW);
        case VT_NULL:
            return return_string(res, NullW);
        default:
            FIXME("arg %s not supported\n", debugstr_variant(arg));
            return E_NOTIMPL;
        }
}

static HRESULT Global_Array(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Erase(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Filter(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Join(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Split(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Replace(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrReverse(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    WCHAR *ptr1, *ptr2, ch;
    BSTR ret;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    hres = to_string(arg, &ret);
    if(FAILED(hres))
        return hres;

    ptr1 = ret;
    ptr2 = ret + SysStringLen(ret)-1;
    while(ptr1 < ptr2) {
        ch = *ptr1;
        *ptr1++ = *ptr2;
        *ptr2-- = ch;
    }

    return return_bstr(res, ret);
}

static HRESULT Global_InStrRev(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int start, ret = 0;
    BSTR str1, str2;
    HRESULT hres;

    TRACE("%s %s arg_cnt=%u\n", debugstr_variant(args), debugstr_variant(args+1), args_cnt);

    if(args_cnt > 3) {
        FIXME("Unsupported args\n");
        return E_NOTIMPL;
    }

    assert(2 <= args_cnt && args_cnt <= 4);

    if(V_VT(args) == VT_NULL || V_VT(args+1) == VT_NULL || V_VT(args+2) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_string(args, &str1);
    if(FAILED(hres))
        return hres;

    hres = to_string(args+1, &str2);
    if(SUCCEEDED(hres)) {
        if(args_cnt > 2) {
            hres = to_int(args+2, &start);
            if(SUCCEEDED(hres) && start <= 0) {
                FIXME("Unsupported start %d\n", start);
                hres = E_NOTIMPL;
            }
        }else {
            start = SysStringLen(str1);
        }
    } else {
        str2 = NULL;
    }

    if(SUCCEEDED(hres)) {
        const WCHAR *ptr;
        size_t len;

        len = SysStringLen(str2);
        if(start >= len && start <= SysStringLen(str1)) {
            for(ptr = str1+start-SysStringLen(str2); ptr >= str1; ptr--) {
                if(!memcmp(ptr, str2, len*sizeof(WCHAR))) {
                    ret = ptr-str1+1;
                    break;
                }
            }
        }
    }

    SysFreeString(str1);
    SysFreeString(str2);
    if(FAILED(hres))
        return hres;

    return return_int(res, ret);
}

static HRESULT Global_LoadPicture(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngine(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_string(res, vbscriptW);
}

static HRESULT Global_ScriptEngineMajorVersion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_MAJOR_VERSION);
}

static HRESULT Global_ScriptEngineMinorVersion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_MINOR_VERSION);
}

static HRESULT Global_ScriptEngineBuildVersion(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_BUILD_VERSION);
}

static HRESULT Global_FormatNumber(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatCurrency(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatPercent(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatDateTime(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_WeekdayName(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int weekday, first_day = 1, abbrev = 0;
    BSTR ret;
    HRESULT hres;

    TRACE("\n");

    assert(1 <= args_cnt && args_cnt <= 3);

    hres = to_int(args, &weekday);
    if(FAILED(hres))
        return hres;

    if(args_cnt > 1) {
        hres = to_int(args+1, &abbrev);
        if(FAILED(hres))
            return hres;

        if(args_cnt == 3) {
            hres = to_int(args+2, &first_day);
            if(FAILED(hres))
                return hres;
        }
    }

    hres = VarWeekdayName(weekday, abbrev, first_day, 0, &ret);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, ret);
}

static HRESULT Global_MonthName(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    int month, abbrev = 0;
    BSTR ret;
    HRESULT hres;

    TRACE("\n");

    assert(args_cnt == 1 || args_cnt == 2);

    hres = to_int(args, &month);
    if(FAILED(hres))
        return hres;

    if(args_cnt == 2) {
        hres = to_int(args+1, &abbrev);
        if(FAILED(hres))
            return hres;
    }

    hres = VarMonthName(month, abbrev, 0, &ret);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, ret);
}

static HRESULT Global_Round(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    double n;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(!res)
        return S_OK;

    switch(V_VT(arg)) {
    case VT_I2:
    case VT_I4:
    case VT_BOOL:
        *res = *arg;
        return S_OK;
    case VT_R8:
        n = V_R8(arg);
        break;
    default:
        hres = to_double(arg, &n);
        if(FAILED(hres))
            return hres;
    }

    return return_double(res, round(n));
}

static HRESULT Global_Escape(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Unescape(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Eval(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Execute(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ExecuteGlobal(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_GetRef(vbdisp_t *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const string_constant_t vbCr          = {1, {'\r'}};
static const string_constant_t vbCrLf        = {2, {'\r','\n'}};
static const string_constant_t vbNewLine     = {2, {'\r','\n'}};
static const string_constant_t vbFormFeed    = {1, {0xc}};
static const string_constant_t vbLf          = {1, {'\n'}};
static const string_constant_t vbNullChar    = {1};
static const string_constant_t vbNullString  = {0};
static const string_constant_t vbTab         = {1, {'\t'}};
static const string_constant_t vbVerticalTab = {1, {0xb}};

static const builtin_prop_t global_props[] = {
    {DISPID_GLOBAL_VBUSESYSTEM,        NULL, BP_GET, VT_I2, 0},
    {DISPID_GLOBAL_USESYSTEMDAYOFWEEK, NULL, BP_GET, VT_I2, 0},
    {DISPID_GLOBAL_VBSUNDAY,           NULL, BP_GET, VT_I2, 1},
    {DISPID_GLOBAL_VBMONDAY,           NULL, BP_GET, VT_I2, 2},
    {DISPID_GLOBAL_VBTUESDAY,          NULL, BP_GET, VT_I2, 3},
    {DISPID_GLOBAL_VBWEDNESDAY,        NULL, BP_GET, VT_I2, 4},
    {DISPID_GLOBAL_VBTHURSDAY,         NULL, BP_GET, VT_I2, 5},
    {DISPID_GLOBAL_VBFRIDAY,           NULL, BP_GET, VT_I2, 6},
    {DISPID_GLOBAL_VBSATURDAY,         NULL, BP_GET, VT_I2, 7},
    {DISPID_GLOBAL_VBFIRSTJAN1,        NULL, BP_GET, VT_I2, 1},
    {DISPID_GLOBAL_VBFIRSTFOURDAYS,    NULL, BP_GET, VT_I2, 2},
    {DISPID_GLOBAL_VBFIRSTFULLWEEK,    NULL, BP_GET, VT_I2, 3},
    {DISPID_GLOBAL_VBOKONLY,           NULL, BP_GET, VT_I2, MB_OK},
    {DISPID_GLOBAL_VBOKCANCEL,         NULL, BP_GET, VT_I2, MB_OKCANCEL},
    {DISPID_GLOBAL_VBABORTRETRYIGNORE, NULL, BP_GET, VT_I2, MB_ABORTRETRYIGNORE},
    {DISPID_GLOBAL_VBYESNOCANCEL,      NULL, BP_GET, VT_I2, MB_YESNOCANCEL},
    {DISPID_GLOBAL_VBYESNO,            NULL, BP_GET, VT_I2, MB_YESNO},
    {DISPID_GLOBAL_VBRETRYCANCEL,      NULL, BP_GET, VT_I2, MB_RETRYCANCEL},
    {DISPID_GLOBAL_VBCRITICAL,         NULL, BP_GET, VT_I2, MB_ICONHAND},
    {DISPID_GLOBAL_VBQUESTION,         NULL, BP_GET, VT_I2, MB_ICONQUESTION},
    {DISPID_GLOBAL_VBEXCLAMATION,      NULL, BP_GET, VT_I2, MB_ICONEXCLAMATION},
    {DISPID_GLOBAL_VBINFORMATION,      NULL, BP_GET, VT_I2, MB_ICONASTERISK},
    {DISPID_GLOBAL_VBDEFAULTBUTTON1,   NULL, BP_GET, VT_I2, MB_DEFBUTTON1},
    {DISPID_GLOBAL_VBDEFAULTBUTTON2,   NULL, BP_GET, VT_I2, MB_DEFBUTTON2},
    {DISPID_GLOBAL_VBDEFAULTBUTTON3,   NULL, BP_GET, VT_I2, MB_DEFBUTTON3},
    {DISPID_GLOBAL_VBDEFAULTBUTTON4,   NULL, BP_GET, VT_I2, MB_DEFBUTTON4},
    {DISPID_GLOBAL_VBAPPLICATIONMODAL, NULL, BP_GET, VT_I2, MB_APPLMODAL},
    {DISPID_GLOBAL_VBSYSTEMMODAL,      NULL, BP_GET, VT_I2, MB_SYSTEMMODAL},
    {DISPID_GLOBAL_VBOK,               NULL, BP_GET, VT_I2, IDOK},
    {DISPID_GLOBAL_VBCANCEL,           NULL, BP_GET, VT_I2, IDCANCEL},
    {DISPID_GLOBAL_VBABORT,            NULL, BP_GET, VT_I2, IDABORT},
    {DISPID_GLOBAL_VBRETRY,            NULL, BP_GET, VT_I2, IDRETRY},
    {DISPID_GLOBAL_VBIGNORE,           NULL, BP_GET, VT_I2, IDIGNORE},
    {DISPID_GLOBAL_VBYES,              NULL, BP_GET, VT_I2, IDYES},
    {DISPID_GLOBAL_VBNO,               NULL, BP_GET, VT_I2, IDNO},
    {DISPID_GLOBAL_VBEMPTY,            NULL, BP_GET, VT_I2, VT_EMPTY},
    {DISPID_GLOBAL_VBNULL,             NULL, BP_GET, VT_I2, VT_NULL},
    {DISPID_GLOBAL_VBINTEGER,          NULL, BP_GET, VT_I2, VT_I2},
    {DISPID_GLOBAL_VBLONG,             NULL, BP_GET, VT_I2, VT_I4},
    {DISPID_GLOBAL_VBSINGLE,           NULL, BP_GET, VT_I2, VT_R4},
    {DISPID_GLOBAL_VBDOUBLE,           NULL, BP_GET, VT_I2, VT_R8},
    {DISPID_GLOBAL_VBCURRENCY,         NULL, BP_GET, VT_I2, VT_CY},
    {DISPID_GLOBAL_VBDATE,             NULL, BP_GET, VT_I2, VT_DATE},
    {DISPID_GLOBAL_VBSTRING,           NULL, BP_GET, VT_I2, VT_BSTR},
    {DISPID_GLOBAL_VBOBJECT,           NULL, BP_GET, VT_I2, VT_DISPATCH},
    {DISPID_GLOBAL_VBERROR,            NULL, BP_GET, VT_I2, VT_ERROR},
    {DISPID_GLOBAL_VBBOOLEAN,          NULL, BP_GET, VT_I2, VT_BOOL},
    {DISPID_GLOBAL_VBVARIANT,          NULL, BP_GET, VT_I2, VT_VARIANT},
    {DISPID_GLOBAL_VBDATAOBJECT,       NULL, BP_GET, VT_I2, VT_UNKNOWN},
    {DISPID_GLOBAL_VBDECIMAL,          NULL, BP_GET, VT_I2, VT_DECIMAL},
    {DISPID_GLOBAL_VBBYTE,             NULL, BP_GET, VT_I2, VT_UI1},
    {DISPID_GLOBAL_VBARRAY,            NULL, BP_GET, VT_I2, VT_ARRAY},
    {DISPID_GLOBAL_VBTRUE,             NULL, BP_GET, VT_I2, VARIANT_TRUE},
    {DISPID_GLOBAL_VBFALSE,            NULL, BP_GET, VT_I2, VARIANT_FALSE},
    {DISPID_GLOBAL_VBUSEDEFAULT,       NULL, BP_GET, VT_I2, -2},
    {DISPID_GLOBAL_VBBINARYCOMPARE,    NULL, BP_GET, VT_I2, 0},
    {DISPID_GLOBAL_VBTEXTCOMPARE,      NULL, BP_GET, VT_I2, 1},
    {DISPID_GLOBAL_VBDATABASECOMPARE,  NULL, BP_GET, VT_I2, 2},
    {DISPID_GLOBAL_VBGENERALDATE,      NULL, BP_GET, VT_I2, 0},
    {DISPID_GLOBAL_VBLONGDATE,         NULL, BP_GET, VT_I2, 1},
    {DISPID_GLOBAL_VBSHORTDATE,        NULL, BP_GET, VT_I2, 2},
    {DISPID_GLOBAL_VBLONGTIME,         NULL, BP_GET, VT_I2, 3},
    {DISPID_GLOBAL_VBSHORTTIME,        NULL, BP_GET, VT_I2, 4},
    {DISPID_GLOBAL_VBOBJECTERROR,      NULL, BP_GET, VT_I4, 0x80040000},
    {DISPID_GLOBAL_VBBLACK,            NULL, BP_GET, VT_I4, 0x000000},
    {DISPID_GLOBAL_VBBLUE,             NULL, BP_GET, VT_I4, 0xff0000},
    {DISPID_GLOBAL_VBCYAN,             NULL, BP_GET, VT_I4, 0xffff00},
    {DISPID_GLOBAL_VBGREEN,            NULL, BP_GET, VT_I4, 0x00ff00},
    {DISPID_GLOBAL_VBMAGENTA,          NULL, BP_GET, VT_I4, 0xff00ff},
    {DISPID_GLOBAL_VBRED,              NULL, BP_GET, VT_I4, 0x0000ff},
    {DISPID_GLOBAL_VBWHITE,            NULL, BP_GET, VT_I4, 0xffffff},
    {DISPID_GLOBAL_VBYELLOW,           NULL, BP_GET, VT_I4, 0x00ffff},
    {DISPID_GLOBAL_VBCR,               NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbCr},
    {DISPID_GLOBAL_VBCRLF,             NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbCrLf},
    {DISPID_GLOBAL_VBNEWLINE,          NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNewLine},
    {DISPID_GLOBAL_VBFORMFEED,         NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbFormFeed},
    {DISPID_GLOBAL_VBLF,               NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbLf},
    {DISPID_GLOBAL_VBNULLCHAR,         NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNullChar},
    {DISPID_GLOBAL_VBNULLSTRING,       NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNullString},
    {DISPID_GLOBAL_VBTAB,              NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbTab},
    {DISPID_GLOBAL_VBVERTICALTAB,      NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbVerticalTab},
    {DISPID_GLOBAL_CCUR,                      Global_CCur, 0, 1},
    {DISPID_GLOBAL_CINT,                      Global_CInt, 0, 1},
    {DISPID_GLOBAL_CLNG,                      Global_CLng, 0, 1},
    {DISPID_GLOBAL_CBOOL,                     Global_CBool, 0, 1},
    {DISPID_GLOBAL_CBYTE,                     Global_CByte, 0, 1},
    {DISPID_GLOBAL_CDATE,                     Global_CDate, 0, 1},
    {DISPID_GLOBAL_CDBL,                      Global_CDbl, 0, 1},
    {DISPID_GLOBAL_CSNG,                      Global_CSng, 0, 1},
    {DISPID_GLOBAL_CSTR,                      Global_CStr, 0, 1},
    {DISPID_GLOBAL_HEX,                       Global_Hex, 0, 1},
    {DISPID_GLOBAL_OCT,                       Global_Oct, 0, 1},
    {DISPID_GLOBAL_VARTYPE,                   Global_VarType, 0, 1},
    {DISPID_GLOBAL_ISDATE,                    Global_IsDate, 0, 1},
    {DISPID_GLOBAL_ISEMPTY,                   Global_IsEmpty, 0, 1},
    {DISPID_GLOBAL_ISNULL,                    Global_IsNull, 0, 1},
    {DISPID_GLOBAL_ISNUMERIC,                 Global_IsNumeric, 0, 1},
    {DISPID_GLOBAL_ISARRAY,                   Global_IsArray, 0, 1},
    {DISPID_GLOBAL_ISOBJECT,                  Global_IsObject, 0, 1},
    {DISPID_GLOBAL_ATN,                       Global_Atn, 0, 1},
    {DISPID_GLOBAL_COS,                       Global_Cos, 0, 1},
    {DISPID_GLOBAL_SIN,                       Global_Sin, 0, 1},
    {DISPID_GLOBAL_TAN,                       Global_Tan, 0, 1},
    {DISPID_GLOBAL_EXP,                       Global_Exp, 0, 1},
    {DISPID_GLOBAL_LOG,                       Global_Log, 0, 1},
    {DISPID_GLOBAL_SQR,                       Global_Sqr, 0, 1},
    {DISPID_GLOBAL_RANDOMIZE,                 Global_Randomize, 0, 1},
    {DISPID_GLOBAL_RND,                       Global_Rnd, 0, 1},
    {DISPID_GLOBAL_TIMER,                     Global_Timer, 0, 0},
    {DISPID_GLOBAL_LBOUND,                    Global_LBound, 0, 1},
    {DISPID_GLOBAL_UBOUND,                    Global_UBound, 0, 1},
    {DISPID_GLOBAL_RGB,                       Global_RGB, 0, 3},
    {DISPID_GLOBAL_LEN,                       Global_Len, 0, 1},
    {DISPID_GLOBAL_LENB,                      Global_LenB, 0, 1},
    {DISPID_GLOBAL_LEFT,                      Global_Left, 0, 2},
    {DISPID_GLOBAL_LEFTB,                     Global_LeftB, 0, 2},
    {DISPID_GLOBAL_RIGHT,                     Global_Right, 0, 2},
    {DISPID_GLOBAL_RIGHTB,                    Global_RightB, 0, 2},
    {DISPID_GLOBAL_MID,                       Global_Mid, 0, 2, 3},
    {DISPID_GLOBAL_MIDB,                      Global_MidB, 0, 2, 3},
    {DISPID_GLOBAL_STRCOMP,                   Global_StrComp, 0, 2, 3},
    {DISPID_GLOBAL_LCASE,                     Global_LCase, 0, 1},
    {DISPID_GLOBAL_UCASE,                     Global_UCase, 0, 1},
    {DISPID_GLOBAL_LTRIM,                     Global_LTrim, 0, 1},
    {DISPID_GLOBAL_RTRIM,                     Global_RTrim, 0, 1},
    {DISPID_GLOBAL_TRIM,                      Global_Trim, 0, 1},
    {DISPID_GLOBAL_SPACE,                     Global_Space, 0, 1},
    {DISPID_GLOBAL_STRING,                    Global_String, 0, 0, 2},
    {DISPID_GLOBAL_INSTR,                     Global_InStr, 0, 2, 4},
    {DISPID_GLOBAL_INSTRB,                    Global_InStrB, 0, 3, 4},
    {DISPID_GLOBAL_ASCB,                      Global_AscB, 0, 1},
    {DISPID_GLOBAL_CHRB,                      Global_ChrB, 0, 1},
    {DISPID_GLOBAL_ASC,                       Global_Asc, 0, 1},
    {DISPID_GLOBAL_CHR,                       Global_Chr, 0, 1},
    {DISPID_GLOBAL_ASCW,                      Global_AscW, 0, 1},
    {DISPID_GLOBAL_CHRW,                      Global_ChrW, 0, 1},
    {DISPID_GLOBAL_ABS,                       Global_Abs, 0, 1},
    {DISPID_GLOBAL_FIX,                       Global_Fix, 0, 1},
    {DISPID_GLOBAL_INT,                       Global_Int, 0, 1},
    {DISPID_GLOBAL_SGN,                       Global_Sgn, 0, 1},
    {DISPID_GLOBAL_NOW,                       Global_Now, 0, 0},
    {DISPID_GLOBAL_DATE,                      Global_Date, 0, 0},
    {DISPID_GLOBAL_TIME,                      Global_Time, 0, 0},
    {DISPID_GLOBAL_DAY,                       Global_Day, 0, 1},
    {DISPID_GLOBAL_MONTH,                     Global_Month, 0, 1},
    {DISPID_GLOBAL_WEEKDAY,                   Global_Weekday, 0, 1, 2},
    {DISPID_GLOBAL_YEAR,                      Global_Year, 0, 1},
    {DISPID_GLOBAL_HOUR,                      Global_Hour, 0, 1},
    {DISPID_GLOBAL_MINUTE,                    Global_Minute, 0, 1},
    {DISPID_GLOBAL_SECOND,                    Global_Second, 0, 1},
    {DISPID_GLOBAL_DATEVALUE,                 Global_DateValue, 0, 1},
    {DISPID_GLOBAL_TIMEVALUE,                 Global_TimeValue, 0, 1},
    {DISPID_GLOBAL_DATESERIAL,                Global_DateSerial, 0, 3},
    {DISPID_GLOBAL_TIMESERIAL,                Global_TimeSerial, 0, 3},
    {DISPID_GLOBAL_INPUTBOX,                  Global_InputBox, 0, 1, 7},
    {DISPID_GLOBAL_MSGBOX,                    Global_MsgBox, 0, 1, 5},
    {DISPID_GLOBAL_CREATEOBJECT,              Global_CreateObject, 0, 1},
    {DISPID_GLOBAL_GETOBJECT,                 Global_GetObject, 0, 0, 2},
    {DISPID_GLOBAL_DATEADD,                   Global_DateAdd, 0, 3},
    {DISPID_GLOBAL_DATEDIFF,                  Global_DateDiff, 0, 3, 5},
    {DISPID_GLOBAL_DATEPART,                  Global_DatePart, 0, 2, 4},
    {DISPID_GLOBAL_TYPENAME,                  Global_TypeName, 0, 1},
    {DISPID_GLOBAL_ARRAY,                     Global_Array, 0, 1},
    {DISPID_GLOBAL_ERASE,                     Global_Erase, 0, 1},
    {DISPID_GLOBAL_FILTER,                    Global_Filter, 0, 2, 4},
    {DISPID_GLOBAL_JOIN,                      Global_Join, 0, 1, 2},
    {DISPID_GLOBAL_SPLIT,                     Global_Split, 0, 1, 4},
    {DISPID_GLOBAL_REPLACE,                   Global_Replace, 0, 3, 6},
    {DISPID_GLOBAL_STRREVERSE,                Global_StrReverse, 0, 1},
    {DISPID_GLOBAL_INSTRREV,                  Global_InStrRev, 0, 2, 4},
    {DISPID_GLOBAL_LOADPICTURE,               Global_LoadPicture, 0, 1},
    {DISPID_GLOBAL_SCRIPTENGINE,              Global_ScriptEngine, 0, 0},
    {DISPID_GLOBAL_SCRIPTENGINEMAJORVERSION,  Global_ScriptEngineMajorVersion, 0, 0},
    {DISPID_GLOBAL_SCRIPTENGINEMINORVERSION,  Global_ScriptEngineMinorVersion, 0, 0},
    {DISPID_GLOBAL_SCRIPTENGINEBUILDVERSION,  Global_ScriptEngineBuildVersion, 0, 0},
    {DISPID_GLOBAL_FORMATNUMBER,              Global_FormatNumber, 0, 1, 5},
    {DISPID_GLOBAL_FORMATCURRENCY,            Global_FormatCurrency, 0, 1, 5},
    {DISPID_GLOBAL_FORMATPERCENT,             Global_FormatPercent, 0, 1, 5},
    {DISPID_GLOBAL_FORMATDATETIME,            Global_FormatDateTime, 0, 1, 2},
    {DISPID_GLOBAL_WEEKDAYNAME,               Global_WeekdayName, 0, 1, 3},
    {DISPID_GLOBAL_MONTHNAME,                 Global_MonthName, 0, 1, 2},
    {DISPID_GLOBAL_ROUND,                     Global_Round, 0, 1, 2},
    {DISPID_GLOBAL_ESCAPE,                    Global_Escape, 0, 1},
    {DISPID_GLOBAL_UNESCAPE,                  Global_Unescape, 0, 1},
    {DISPID_GLOBAL_EVAL,                      Global_Eval, 0, 1},
    {DISPID_GLOBAL_EXECUTE,                   Global_Execute, 0, 1},
    {DISPID_GLOBAL_EXECUTEGLOBAL,             Global_ExecuteGlobal, 0, 1},
    {DISPID_GLOBAL_GETREF,                    Global_GetRef, 0, 1},
    {DISPID_GLOBAL_VBMSGBOXHELPBUTTON,     NULL, BP_GET, VT_I4, MB_HELP},
    {DISPID_GLOBAL_VBMSGBOXSETFOREGROUND,  NULL, BP_GET, VT_I4, MB_SETFOREGROUND},
    {DISPID_GLOBAL_VBMSGBOXRIGHT,          NULL, BP_GET, VT_I4, MB_RIGHT},
    {DISPID_GLOBAL_VBMSGBOXRTLREADING,     NULL, BP_GET, VT_I4, MB_RTLREADING}
};

static HRESULT Err_Description(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_HelpContext(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_HelpFile(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_Number(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;

    TRACE("\n");

    if(!This->desc)
        return E_UNEXPECTED;

    if(args_cnt) {
        FIXME("setter not implemented\n");
        return E_NOTIMPL;
    }

    hres = This->desc->ctx->err_number;
    return return_int(res, HRESULT_FACILITY(hres) == FACILITY_VBS ? HRESULT_CODE(hres) : hres);
}

static HRESULT Err_Source(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Err_Clear(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    if(!This->desc)
        return E_UNEXPECTED;

    This->desc->ctx->err_number = S_OK;
    return S_OK;
}

static HRESULT Err_Raise(vbdisp_t *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t err_props[] = {
    {DISPID_ERR_DESCRIPTION,  Err_Description, BP_GETPUT},
    {DISPID_ERR_HELPCONTEXT,  Err_HelpContext, BP_GETPUT},
    {DISPID_ERR_HELPFILE,     Err_HelpFile, BP_GETPUT},
    {DISPID_ERR_NUMBER,       Err_Number, BP_GETPUT},
    {DISPID_ERR_SOURCE,       Err_Source, BP_GETPUT},
    {DISPID_ERR_CLEAR,        Err_Clear},
    {DISPID_ERR_RAISE,        Err_Raise, 0, 5},
};

HRESULT init_global(script_ctx_t *ctx)
{
    HRESULT hres;

    ctx->global_desc.ctx = ctx;
    ctx->global_desc.builtin_prop_cnt = sizeof(global_props)/sizeof(*global_props);
    ctx->global_desc.builtin_props = global_props;

    hres = get_typeinfo(GlobalObj_tid, &ctx->global_desc.typeinfo);
    if(FAILED(hres))
        return hres;

    hres = create_vbdisp(&ctx->global_desc, &ctx->global_obj);
    if(FAILED(hres))
        return hres;

    hres = create_script_disp(ctx, &ctx->script_obj);
    if(FAILED(hres))
        return hres;

    ctx->err_desc.ctx = ctx;
    ctx->err_desc.builtin_prop_cnt = sizeof(err_props)/sizeof(*err_props);
    ctx->err_desc.builtin_props = err_props;

    hres = get_typeinfo(ErrObj_tid, &ctx->err_desc.typeinfo);
    if(FAILED(hres))
        return hres;

    return create_vbdisp(&ctx->err_desc, &ctx->err_obj);
}
