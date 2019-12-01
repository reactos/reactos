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

#include <assert.h>
#include <math.h>

#include "vbscript.h"
#include "vbscript_defs.h"

#include "mshtmhst.h"
#include "objsafe.h"

#include "wine/debug.h"

#ifdef __REACTOS__
#include <wingdi.h>
#include <winnls.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

#define VB_E_CANNOT_CREATE_OBJ 0x800a01ad
#define VB_E_MK_PARSE_ERROR    0x800a01b0

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

static const WCHAR emptyW[] = {0};
static const WCHAR vbscriptW[] = {'V','B','S','c','r','i','p','t',0};

#define BP_GET      1
#define BP_GETPUT   2

typedef struct {
    UINT16 len;
    WCHAR buf[7];
} string_constant_t;

struct _builtin_prop_t {
    const WCHAR *name;
    HRESULT (*proc)(BuiltinDisp*,VARIANT*,unsigned,VARIANT*);
    DWORD flags;
    unsigned min_args;
    UINT_PTR max_args;
};

static inline BuiltinDisp *impl_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, BuiltinDisp, IDispatch_iface);
}

static HRESULT WINAPI Builtin_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Builtin_AddRef(IDispatch *iface)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Builtin_Release(IDispatch *iface)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->ctx);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI Builtin_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 0;
    return S_OK;
}

static HRESULT WINAPI Builtin_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return DISP_E_BADINDEX;
}

HRESULT get_builtin_id(BuiltinDisp *disp, const WCHAR *name, DISPID *id)
{
    size_t min = 1, max = disp->member_cnt - 1, i;
    int r;

    while(min <= max) {
        i = (min + max) / 2;
        r = wcsicmp(disp->members[i].name, name);
        if(!r) {
            *id = i;
            return S_OK;
        }
        if(r < 0)
            min = i+1;
        else
            max = i-1;
    }

    return DISP_E_MEMBERNOTFOUND;

}

static HRESULT WINAPI Builtin_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *names, UINT name_cnt,
                                            LCID lcid, DISPID *ids)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    unsigned i;
    HRESULT hres;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), names, name_cnt, lcid, ids);

    if(!This->ctx) {
        FIXME("NULL context\n");
        return E_UNEXPECTED;
    }

    for(i = 0; i < name_cnt; i++) {
        hres = get_builtin_id(This, names[i], &ids[i]);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI Builtin_Invoke(IDispatch *iface, DISPID id, REFIID riid, LCID lcid, WORD flags,
                                     DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, UINT *err)
{
    BuiltinDisp *This = impl_from_IDispatch(iface);
    const builtin_prop_t *prop;
    VARIANT args[8];
    unsigned argn, i;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, id, debugstr_guid(riid), lcid, flags, dp, res, ei, err);

    if(!This->ctx) {
        FIXME("NULL context\n");
        return E_UNEXPECTED;
    }

    if(id >= This->member_cnt || (!This->members[id].proc && !This->members[id].flags))
        return DISP_E_MEMBERNOTFOUND;
    prop = This->members + id;

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        if(!(prop->flags & (BP_GET|BP_GETPUT))) {
            FIXME("property does not support DISPATCH_PROPERTYGET\n");
            return E_FAIL;
        }
        break;
    case DISPATCH_PROPERTYGET|DISPATCH_METHOD:
        if(!prop->proc && prop->flags == BP_GET) {
            const int vt = prop->min_args, val = prop->max_args;
            switch(vt) {
            case VT_I2:
                V_VT(res) = VT_I2;
                V_I2(res) = val;
                break;
            case VT_I4:
                V_VT(res) = VT_I4;
                V_I4(res) = val;
                break;
            case VT_BSTR: {
                const string_constant_t *str = (const string_constant_t*)prop->max_args;
                BSTR ret;

                ret = SysAllocStringLen(str->buf, str->len);
                if(!ret)
                    return E_OUTOFMEMORY;

                V_VT(res) = VT_BSTR;
                V_BSTR(res) = ret;
                break;
            }
            DEFAULT_UNREACHABLE;
            }
            return S_OK;
        }
        break;
    case DISPATCH_METHOD:
        if(prop->flags & (BP_GET|BP_GETPUT)) {
            FIXME("Call on property\n");
            return E_FAIL;
        }
        break;
    case DISPATCH_PROPERTYPUT:
        if(!(prop->flags & BP_GETPUT)) {
            FIXME("property does not support DISPATCH_PROPERTYPUT\n");
            return E_FAIL;
        }

        FIXME("call put\n");
        return E_NOTIMPL;
    default:
        FIXME("unsupported flags %x\n", flags);
        return E_NOTIMPL;
    }

    argn = arg_cnt(dp);

    if(argn < prop->min_args || argn > (prop->max_args ? prop->max_args : prop->min_args)) {
        WARN("invalid number of arguments\n");
        return MAKE_VBSERROR(VBSE_FUNC_ARITY_MISMATCH);
    }

    assert(argn < ARRAY_SIZE(args));

    for(i=0; i < argn; i++) {
        if(V_VT(dp->rgvarg+dp->cArgs-i-1) == (VT_BYREF|VT_VARIANT))
            args[i] = *V_VARIANTREF(dp->rgvarg+dp->cArgs-i-1);
        else
            args[i] = dp->rgvarg[dp->cArgs-i-1];
    }

    return prop->proc(This, args, dp->cArgs, res);
}

static const IDispatchVtbl BuiltinDispVtbl = {
    Builtin_QueryInterface,
    Builtin_AddRef,
    Builtin_Release,
    Builtin_GetTypeInfoCount,
    Builtin_GetTypeInfo,
    Builtin_GetIDsOfNames,
    Builtin_Invoke
};

static HRESULT create_builtin_dispatch(script_ctx_t *ctx, const builtin_prop_t *members, size_t member_cnt, BuiltinDisp **ret)
{
    BuiltinDisp *disp;

    if(!(disp = heap_alloc(sizeof(*disp))))
        return E_OUTOFMEMORY;

    disp->IDispatch_iface.lpVtbl = &BuiltinDispVtbl;
    disp->ref = 1;
    disp->members = members;
    disp->member_cnt = member_cnt;
    disp->ctx = ctx;

    *ret = disp;
    return S_OK;
}

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

static HRESULT to_system_time(VARIANT *v, SYSTEMTIME *st)
{
    VARIANT date;
    HRESULT hres;

    V_VT(&date) = VT_EMPTY;
    hres = VariantChangeType(&date, v, 0, VT_DATE);
    if(FAILED(hres))
        return hres;

    return VariantTimeToSystemTime(V_DATE(&date), st);
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

            title = title_buf = heap_alloc(sizeof(vbscriptW) + (lstrlenW(orig_title)+2)*sizeof(WCHAR));
            if(!title)
                return E_OUTOFMEMORY;

            memcpy(title_buf, vbscriptW, sizeof(vbscriptW));
            ptr = title_buf + ARRAY_SIZE(vbscriptW)-1;

            *ptr++ = ':';
            *ptr++ = ' ';
            lstrcpyW(ptr, orig_title);
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

static HRESULT Global_CCur(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CInt(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CLng(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CBool(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CByte(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CDate(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_CDbl(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CSng(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_CStr(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    TRACE("%s\n", debugstr_variant(arg));

    if(V_VT(arg) == VT_NULL)
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);

    hres = to_string(arg, &str);
    if(FAILED(hres))
        return hres;

    return return_bstr(res, str);
}

static inline WCHAR hex_char(unsigned n)
{
    return n < 10 ? '0'+n : 'A'+n-10;
}

static HRESULT Global_Hex(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    WCHAR buf[17], *ptr;
    DWORD n;
    HRESULT hres;
    int ret;

    TRACE("%s\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_I2:
        n = (WORD)V_I2(arg);
        break;
    case VT_NULL:
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        hres = to_int(arg, &ret);
        if(FAILED(hres))
            return hres;
        else
            n = ret;
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

static HRESULT Global_Oct(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    WCHAR buf[23], *ptr;
    DWORD n;
    int ret;

    TRACE("%s\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_I2:
        n = (WORD)V_I2(arg);
        break;
    case VT_NULL:
        if(res)
            V_VT(res) = VT_NULL;
        return S_OK;
    default:
        hres = to_int(arg, &ret);
        if(FAILED(hres))
            return hres;
        else
            n = ret;
    }

    buf[22] = 0;
    ptr = buf + 21;

    if(n) {
        do {
            *ptr-- = '0' + (n & 0x7);
            n >>= 3;
        }while(n);
        ptr++;
    }else {
        *ptr = '0';
    }

    return return_string(res, ptr);
}

static HRESULT Global_VarType(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    VARTYPE vt;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    vt = V_VT(arg) & ~VT_BYREF;
    if(vt & ~(VT_TYPEMASK | VT_ARRAY)) {
        FIXME("not supported %s\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    return return_short(res, vt);
}

static HRESULT Global_IsDate(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsEmpty(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_EMPTY ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_IsNull(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_NULL ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_IsNumeric(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    hres = to_double(arg, &d);

    return return_bool(res, SUCCEEDED(hres));
}

static HRESULT Global_IsArray(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_IsObject(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("(%s)\n", debugstr_variant(arg));

    assert(args_cnt == 1);

    if(res) {
        V_VT(res) = VT_BOOL;
        V_BOOL(res) = V_VT(arg) == VT_DISPATCH ? VARIANT_TRUE : VARIANT_FALSE;
    }
    return S_OK;
}

static HRESULT Global_Atn(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, atan(d));
}

static HRESULT Global_Cos(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, cos(d));
}

static HRESULT Global_Sin(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, sin(d));
}

static HRESULT Global_Tan(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, tan(d));
}

static HRESULT Global_Exp(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;
    double d;

    hres = to_double(arg, &d);
    if(FAILED(hres))
        return hres;

    return return_double(res, exp(d));
}

static HRESULT Global_Log(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Sqr(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Randomize(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Rnd(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Timer(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    double sec;

    GetLocalTime(&lt);
    sec = lt.wHour * 3600 + lt.wMinute * 60 + lt.wSecond + lt.wMilliseconds / 1000.0;
    return return_float(res, sec);

}

static HRESULT Global_LBound(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_UBound(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SAFEARRAY *sa;
    HRESULT hres;
    LONG ubound;
    int dim;

    assert(args_cnt == 1 || args_cnt == 2);

    TRACE("%s %s\n", debugstr_variant(arg), args_cnt == 2 ? debugstr_variant(arg + 1) : "1");

    switch(V_VT(arg)) {
    case VT_VARIANT|VT_ARRAY:
        sa = V_ARRAY(arg);
        break;
    case VT_VARIANT|VT_ARRAY|VT_BYREF:
        sa = *V_ARRAYREF(arg);
        break;
    default:
        FIXME("arg %s not supported\n", debugstr_variant(arg));
        return E_NOTIMPL;
    }

    if(args_cnt == 2) {
        hres = to_int(arg + 1, &dim);
        if(FAILED(hres))
            return hres;
    }else {
        dim = 1;
    }

    hres = SafeArrayGetUBound(sa, dim, &ubound);
    if(FAILED(hres))
        return hres;

    return return_int(res, ubound);
}

static HRESULT Global_RGB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Len(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_LenB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Left(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_LeftB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Right(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_RightB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Mid(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_MidB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrComp(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR left, right;
    int mode, ret;
    HRESULT hres;
    short val;

    TRACE("(%s %s ...)\n", debugstr_variant(args), debugstr_variant(args+1));

    assert(args_cnt == 2 || args_cnt == 3);

    if (args_cnt == 3) {
        hres = to_int(args+2, &mode);
        if(FAILED(hres))
            return hres;

        if (mode != 0 && mode != 1) {
            FIXME("unknown compare mode = %d\n", mode);
            return E_FAIL;
        }
    }
    else
        mode = 0;

    hres = to_string(args, &left);
    if(FAILED(hres))
        return hres;

    hres = to_string(args+1, &right);
    if(FAILED(hres))
    {
        SysFreeString(left);
        return hres;
    }

    ret = mode ? wcsicmp(left, right) : wcscmp(left, right);
    val = ret < 0 ? -1 : (ret > 0 ? 1 : 0);

    SysFreeString(left);
    SysFreeString(right);
    return return_short(res, val);
}

static HRESULT Global_LCase(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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
            *ptr = towlower(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_UCase(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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
            *ptr = towupper(*ptr);

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
    }else {
        SysFreeString(str);
    }
    return S_OK;
}

static HRESULT Global_LTrim(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

    for(ptr = str; *ptr && iswspace(*ptr); ptr++);

    str = SysAllocString(ptr);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_RTrim(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

    for(ptr = str+SysStringLen(str); ptr-1 > str && iswspace(*(ptr-1)); ptr--);

    str = SysAllocStringLen(str, ptr-str);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_Trim(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

    for(begin_ptr = str; *begin_ptr && iswspace(*begin_ptr); begin_ptr++);
    for(end_ptr = str+SysStringLen(str); end_ptr-1 > begin_ptr && iswspace(*(end_ptr-1)); end_ptr--);

    str = SysAllocStringLen(begin_ptr, end_ptr-begin_ptr);
    SysFreeString(conv_str);
    if(!str)
        return E_OUTOFMEMORY;

    return return_bstr(res, str);
}

static HRESULT Global_Space(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_String(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InStr(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

        ptr = wcsstr(str1+start, str2);
        ret = ptr ? ptr-str1+1 : 0;
    }else {
        ret = 0;
    }

    return return_int(res, ret);
}

static HRESULT Global_InStrB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_AscB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrB(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Asc(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    BSTR conv_str = NULL, str;
    HRESULT hres = S_OK;

    TRACE("(%s)\n", debugstr_variant(arg));

    switch(V_VT(arg)) {
    case VT_NULL:
        return MAKE_VBSERROR(VBSE_ILLEGAL_NULL_USE);
    case VT_EMPTY:
        return MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    case VT_BSTR:
        str = V_BSTR(arg);
        break;
    default:
        hres = to_string(arg, &conv_str);
        if(FAILED(hres))
            return hres;
        str = conv_str;
    }

    if(!SysStringLen(str) || *str >= 0x100)
        hres = MAKE_VBSERROR(VBSE_ILLEGAL_FUNC_CALL);
    else if(res)
        hres = return_short(res, *str);
    SysFreeString(conv_str);
    return hres;
}

/* The function supports only single-byte and double-byte character sets. It
 * ignores language specified by IActiveScriptSite::GetLCID. The argument needs
 * to be in range of short or unsigned short. */
static HRESULT Global_Chr(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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
    if(!MultiByteToWideChar(CP_ACP, 0, buf, len, &ch, 1)) {
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

static HRESULT Global_AscW(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ChrW(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Abs(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Fix(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Int(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Sgn(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Now(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME lt;
    double date;

    TRACE("\n");

    GetLocalTime(&lt);
    SystemTimeToVariantTime(&lt, &date);
    return return_date(res, date);
}

static HRESULT Global_Date(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Time(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Day(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wDay);
}

static HRESULT Global_Month(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wMonth);
}

static HRESULT Global_Weekday(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Year(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wYear);
}

static HRESULT Global_Hour(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wHour);
}

static HRESULT Global_Minute(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wMinute);
}

static HRESULT Global_Second(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SYSTEMTIME st;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    hres = to_system_time(arg, &st);
    return FAILED(hres) ? hres : return_short(res, st.wSecond);
}

static HRESULT Global_DateValue(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeValue(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateSerial(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TimeSerial(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_InputBox(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_MsgBox(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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
        hres = show_msgbox(This->ctx, prompt, type, title, res);

    SysFreeString(prompt);
    SysFreeString(title);
    return hres;
}

static HRESULT Global_CreateObject(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    IUnknown *obj;
    HRESULT hres;

    TRACE("(%s)\n", debugstr_variant(arg));

    if(V_VT(arg) != VT_BSTR) {
        FIXME("non-bstr arg\n");
        return E_INVALIDARG;
    }

    obj = create_object(This->ctx, V_BSTR(arg));
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

static HRESULT Global_GetObject(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

    if(This->ctx->safeopt & (INTERFACE_USES_SECURITY_MANAGER|INTERFACESAFE_FOR_UNTRUSTED_DATA)) {
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

    hres = set_object_site(This->ctx, obj_unk);
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

static HRESULT Global_DateAdd(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DateDiff(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_DatePart(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_TypeName(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Array(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    SAFEARRAYBOUND bounds;
    SAFEARRAY *sa;
    VARIANT *data;
    HRESULT hres;
    unsigned i;

    TRACE("arg_cnt=%u\n", args_cnt);

    bounds.lLbound = 0;
    bounds.cElements = args_cnt;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
    if(!sa)
        return E_OUTOFMEMORY;

    hres = SafeArrayAccessData(sa, (void**)&data);
    if(FAILED(hres)) {
        SafeArrayDestroy(sa);
        return hres;
    }

    for(i=0; i<args_cnt; i++) {
        hres = VariantCopyInd(data+i, arg+i);
        if(FAILED(hres)) {
            SafeArrayUnaccessData(sa);
            SafeArrayDestroy(sa);
            return hres;
        }
    }
    SafeArrayUnaccessData(sa);

    if(res) {
        V_VT(res) = VT_ARRAY|VT_VARIANT;
        V_ARRAY(res) = sa;
    }else {
        SafeArrayDestroy(sa);
    }

    return S_OK;
}

static HRESULT Global_Erase(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Filter(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Join(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Split(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Replace(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_StrReverse(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_InStrRev(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

    if(V_VT(args) == VT_NULL || V_VT(args+1) == VT_NULL || (args_cnt > 2 && V_VT(args+2) == VT_NULL))
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

static HRESULT Global_LoadPicture(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ScriptEngine(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_string(res, vbscriptW);
}

static HRESULT Global_ScriptEngineMajorVersion(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_MAJOR_VERSION);
}

static HRESULT Global_ScriptEngineMinorVersion(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_MINOR_VERSION);
}

static HRESULT Global_ScriptEngineBuildVersion(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("%s\n", debugstr_variant(arg));

    assert(args_cnt == 0);

    return return_int(res, VBSCRIPT_BUILD_VERSION);
}

static HRESULT Global_FormatNumber(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatCurrency(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatPercent(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_FormatDateTime(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_WeekdayName(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_MonthName(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Round(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
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

static HRESULT Global_Escape(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Unescape(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Eval(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Execute(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_ExecuteGlobal(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_GetRef(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT Global_Err(BuiltinDisp *This, VARIANT *arg, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    if(args_cnt) {
        FIXME("Setter not supported\n");
        return E_NOTIMPL;
    }

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = &This->ctx->err_obj->IDispatch_iface;
    IDispatch_AddRef(V_DISPATCH(res));
    return S_OK;
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
    {NULL}, /* no default value */
    {L"Abs",                       Global_Abs, 0, 1},
    {L"Array",                     Global_Array, 0, 0, MAXDWORD},
    {L"Asc",                       Global_Asc, 0, 1},
    {L"AscB",                      Global_AscB, 0, 1},
    {L"AscW",                      Global_AscW, 0, 1},
    {L"Atn",                       Global_Atn, 0, 1},
    {L"CBool",                     Global_CBool, 0, 1},
    {L"CByte",                     Global_CByte, 0, 1},
    {L"CCur",                      Global_CCur, 0, 1},
    {L"CDate",                     Global_CDate, 0, 1},
    {L"CDbl",                      Global_CDbl, 0, 1},
    {L"Chr",                       Global_Chr, 0, 1},
    {L"ChrB",                      Global_ChrB, 0, 1},
    {L"ChrW",                      Global_ChrW, 0, 1},
    {L"CInt",                      Global_CInt, 0, 1},
    {L"CLng",                      Global_CLng, 0, 1},
    {L"Cos",                       Global_Cos, 0, 1},
    {L"CreateObject",              Global_CreateObject, 0, 1},
    {L"CSng",                      Global_CSng, 0, 1},
    {L"CStr",                      Global_CStr, 0, 1},
    {L"Date",                      Global_Date, 0, 0},
    {L"DateAdd",                   Global_DateAdd, 0, 3},
    {L"DateDiff",                  Global_DateDiff, 0, 3, 5},
    {L"DatePart",                  Global_DatePart, 0, 2, 4},
    {L"DateSerial",                Global_DateSerial, 0, 3},
    {L"DateValue",                 Global_DateValue, 0, 1},
    {L"Day",                       Global_Day, 0, 1},
    {L"Erase",                     Global_Erase, 0, 1},
    {L"Err",                       Global_Err, BP_GETPUT},
    {L"Escape",                    Global_Escape, 0, 1},
    {L"Eval",                      Global_Eval, 0, 1},
    {L"Execute",                   Global_Execute, 0, 1},
    {L"ExecuteGlobal",             Global_ExecuteGlobal, 0, 1},
    {L"Exp",                       Global_Exp, 0, 1},
    {L"Filter",                    Global_Filter, 0, 2, 4},
    {L"Fix",                       Global_Fix, 0, 1},
    {L"FormatCurrency",            Global_FormatCurrency, 0, 1, 5},
    {L"FormatDateTime",            Global_FormatDateTime, 0, 1, 2},
    {L"FormatNumber",              Global_FormatNumber, 0, 1, 5},
    {L"FormatPercent",             Global_FormatPercent, 0, 1, 5},
    {L"GetObject",                 Global_GetObject, 0, 0, 2},
    {L"GetRef",                    Global_GetRef, 0, 1},
    {L"Hex",                       Global_Hex, 0, 1},
    {L"Hour",                      Global_Hour, 0, 1},
    {L"InputBox",                  Global_InputBox, 0, 1, 7},
    {L"InStr",                     Global_InStr, 0, 2, 4},
    {L"InStrB",                    Global_InStrB, 0, 3, 4},
    {L"InStrRev",                  Global_InStrRev, 0, 2, 4},
    {L"Int",                       Global_Int, 0, 1},
    {L"IsArray",                   Global_IsArray, 0, 1},
    {L"IsDate",                    Global_IsDate, 0, 1},
    {L"IsEmpty",                   Global_IsEmpty, 0, 1},
    {L"IsNull",                    Global_IsNull, 0, 1},
    {L"IsNumeric",                 Global_IsNumeric, 0, 1},
    {L"IsObject",                  Global_IsObject, 0, 1},
    {L"Join",                      Global_Join, 0, 1, 2},
    {L"LBound",                    Global_LBound, 0, 1},
    {L"LCase",                     Global_LCase, 0, 1},
    {L"Left",                      Global_Left, 0, 2},
    {L"LeftB",                     Global_LeftB, 0, 2},
    {L"Len",                       Global_Len, 0, 1},
    {L"LenB",                      Global_LenB, 0, 1},
    {L"LoadPicture",               Global_LoadPicture, 0, 1},
    {L"Log",                       Global_Log, 0, 1},
    {L"LTrim",                     Global_LTrim, 0, 1},
    {L"Mid",                       Global_Mid, 0, 2, 3},
    {L"MidB",                      Global_MidB, 0, 2, 3},
    {L"Minute",                    Global_Minute, 0, 1},
    {L"Month",                     Global_Month, 0, 1},
    {L"MonthName",                 Global_MonthName, 0, 1, 2},
    {L"MsgBox",                    Global_MsgBox, 0, 1, 5},
    {L"Now",                       Global_Now, 0, 0},
    {L"Oct",                       Global_Oct, 0, 1},
    {L"Randomize",                 Global_Randomize, 0, 1},
    {L"Replace",                   Global_Replace, 0, 3, 6},
    {L"RGB",                       Global_RGB, 0, 3},
    {L"Right",                     Global_Right, 0, 2},
    {L"RightB",                    Global_RightB, 0, 2},
    {L"Rnd",                       Global_Rnd, 0, 1},
    {L"Round",                     Global_Round, 0, 1, 2},
    {L"RTrim",                     Global_RTrim, 0, 1},
    {L"ScriptEngine",              Global_ScriptEngine, 0, 0},
    {L"ScriptEngineBuildVersion",  Global_ScriptEngineBuildVersion, 0, 0},
    {L"ScriptEngineMajorVersion",  Global_ScriptEngineMajorVersion, 0, 0},
    {L"ScriptEngineMinorVersion",  Global_ScriptEngineMinorVersion, 0, 0},
    {L"Second",                    Global_Second, 0, 1},
    {L"Sgn",                       Global_Sgn, 0, 1},
    {L"Sin",                       Global_Sin, 0, 1},
    {L"Space",                     Global_Space, 0, 1},
    {L"Split",                     Global_Split, 0, 1, 4},
    {L"Sqr",                       Global_Sqr, 0, 1},
    {L"StrComp",                   Global_StrComp, 0, 2, 3},
    {L"String",                    Global_String, 0, 0, 2},
    {L"StrReverse",                Global_StrReverse, 0, 1},
    {L"Tan",                       Global_Tan, 0, 1},
    {L"Time",                      Global_Time, 0, 0},
    {L"Timer",                     Global_Timer, 0, 0},
    {L"TimeSerial",                Global_TimeSerial, 0, 3},
    {L"TimeValue",                 Global_TimeValue, 0, 1},
    {L"Trim",                      Global_Trim, 0, 1},
    {L"TypeName",                  Global_TypeName, 0, 1},
    {L"UBound",                    Global_UBound, 0, 1, 2},
    {L"UCase",                     Global_UCase, 0, 1},
    {L"Unescape",                  Global_Unescape, 0, 1},
    {L"VarType",                   Global_VarType, 0, 1},
    {L"vbAbort",                   NULL, BP_GET, VT_I2, IDABORT},
    {L"vbAbortRetryIgnore",        NULL, BP_GET, VT_I2, MB_ABORTRETRYIGNORE},
    {L"vbApplicationModal",        NULL, BP_GET, VT_I2, MB_APPLMODAL},
    {L"vbArray",                   NULL, BP_GET, VT_I2, VT_ARRAY},
    {L"vbBinaryCompare",           NULL, BP_GET, VT_I2, 0},
    {L"vbBlack",                   NULL, BP_GET, VT_I4, 0x000000},
    {L"vbBlue",                    NULL, BP_GET, VT_I4, 0xff0000},
    {L"vbBoolean",                 NULL, BP_GET, VT_I2, VT_BOOL},
    {L"vbByte",                    NULL, BP_GET, VT_I2, VT_UI1},
    {L"vbCancel",                  NULL, BP_GET, VT_I2, IDCANCEL},
    {L"vbCr",                      NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbCr},
    {L"vbCritical",                NULL, BP_GET, VT_I2, MB_ICONHAND},
    {L"vbCrLf",                    NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbCrLf},
    {L"vbCurrency",                NULL, BP_GET, VT_I2, VT_CY},
    {L"vbCyan",                    NULL, BP_GET, VT_I4, 0xffff00},
    {L"vbDatabaseCompare",         NULL, BP_GET, VT_I2, 2},
    {L"vbDataObject",              NULL, BP_GET, VT_I2, VT_UNKNOWN},
    {L"vbDate",                    NULL, BP_GET, VT_I2, VT_DATE},
    {L"vbDecimal",                 NULL, BP_GET, VT_I2, VT_DECIMAL},
    {L"vbDefaultButton1",          NULL, BP_GET, VT_I2, MB_DEFBUTTON1},
    {L"vbDefaultButton2",          NULL, BP_GET, VT_I2, MB_DEFBUTTON2},
    {L"vbDefaultButton3",          NULL, BP_GET, VT_I2, MB_DEFBUTTON3},
    {L"vbDefaultButton4",          NULL, BP_GET, VT_I2, MB_DEFBUTTON4},
    {L"vbDouble",                  NULL, BP_GET, VT_I2, VT_R8},
    {L"vbEmpty",                   NULL, BP_GET, VT_I2, VT_EMPTY},
    {L"vbError",                   NULL, BP_GET, VT_I2, VT_ERROR},
    {L"vbExclamation",             NULL, BP_GET, VT_I2, MB_ICONEXCLAMATION},
    {L"vbFalse",                   NULL, BP_GET, VT_I2, VARIANT_FALSE},
    {L"vbFirstFourDays",           NULL, BP_GET, VT_I2, 2},
    {L"vbFirstFullWeek",           NULL, BP_GET, VT_I2, 3},
    {L"vbFirstJan1",               NULL, BP_GET, VT_I2, 1},
    {L"vbFormFeed",                NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbFormFeed},
    {L"vbFriday",                  NULL, BP_GET, VT_I2, 6},
    {L"vbGeneralDate",             NULL, BP_GET, VT_I2, 0},
    {L"vbGreen",                   NULL, BP_GET, VT_I4, 0x00ff00},
    {L"vbIgnore",                  NULL, BP_GET, VT_I2, IDIGNORE},
    {L"vbInformation",             NULL, BP_GET, VT_I2, MB_ICONASTERISK},
    {L"vbInteger",                 NULL, BP_GET, VT_I2, VT_I2},
    {L"vbLf",                      NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbLf},
    {L"vbLong",                    NULL, BP_GET, VT_I2, VT_I4},
    {L"vbLongDate",                NULL, BP_GET, VT_I2, 1},
    {L"vbLongTime",                NULL, BP_GET, VT_I2, 3},
    {L"vbMagenta",                 NULL, BP_GET, VT_I4, 0xff00ff},
    {L"vbMonday",                  NULL, BP_GET, VT_I2, 2},
    {L"vbMsgBoxHelpButton",        NULL, BP_GET, VT_I4, MB_HELP},
    {L"vbMsgBoxRight",             NULL, BP_GET, VT_I4, MB_RIGHT},
    {L"vbMsgBoxRtlReading",        NULL, BP_GET, VT_I4, MB_RTLREADING},
    {L"vbMsgBoxSetForeground",     NULL, BP_GET, VT_I4, MB_SETFOREGROUND},
    {L"vbNewLine",                 NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNewLine},
    {L"vbNo",                      NULL, BP_GET, VT_I2, IDNO},
    {L"vbNull",                    NULL, BP_GET, VT_I2, VT_NULL},
    {L"vbNullChar",                NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNullChar},
    {L"vbNullString",              NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbNullString},
    {L"vbObject",                  NULL, BP_GET, VT_I2, VT_DISPATCH},
    {L"vbObjectError",             NULL, BP_GET, VT_I4, 0x80040000},
    {L"vbOK",                      NULL, BP_GET, VT_I2, IDOK},
    {L"vbOKCancel",                NULL, BP_GET, VT_I2, MB_OKCANCEL},
    {L"vbOKOnly",                  NULL, BP_GET, VT_I2, MB_OK},
    {L"vbQuestion",                NULL, BP_GET, VT_I2, MB_ICONQUESTION},
    {L"vbRed",                     NULL, BP_GET, VT_I4, 0x0000ff},
    {L"vbRetry",                   NULL, BP_GET, VT_I2, IDRETRY},
    {L"vbRetryCancel",             NULL, BP_GET, VT_I2, MB_RETRYCANCEL},
    {L"vbSaturday",                NULL, BP_GET, VT_I2, 7},
    {L"vbShortDate",               NULL, BP_GET, VT_I2, 2},
    {L"vbShortTime",               NULL, BP_GET, VT_I2, 4},
    {L"vbSingle",                  NULL, BP_GET, VT_I2, VT_R4},
    {L"vbString",                  NULL, BP_GET, VT_I2, VT_BSTR},
    {L"vbSunday",                  NULL, BP_GET, VT_I2, 1},
    {L"vbSystemModal",             NULL, BP_GET, VT_I2, MB_SYSTEMMODAL},
    {L"vbTab",                     NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbTab},
    {L"vbTextCompare",             NULL, BP_GET, VT_I2, 1},
    {L"vbThursday",                NULL, BP_GET, VT_I2, 5},
    {L"vbTrue",                    NULL, BP_GET, VT_I2, VARIANT_TRUE},
    {L"vbTuesday",                 NULL, BP_GET, VT_I2, 3},
    {L"vbUseDefault",              NULL, BP_GET, VT_I2, -2},
    {L"vbUseSystem",               NULL, BP_GET, VT_I2, 0},
    {L"vbUseSystemDayOfWeek",      NULL, BP_GET, VT_I2, 0},
    {L"vbVariant",                 NULL, BP_GET, VT_I2, VT_VARIANT},
    {L"vbVerticalTab",             NULL, BP_GET, VT_BSTR, (UINT_PTR)&vbVerticalTab},
    {L"vbWednesday",               NULL, BP_GET, VT_I2, 4},
    {L"vbWhite",                   NULL, BP_GET, VT_I4, 0xffffff},
    {L"vbYellow",                  NULL, BP_GET, VT_I4, 0x00ffff},
    {L"vbYes",                     NULL, BP_GET, VT_I2, IDYES},
    {L"vbYesNo",                   NULL, BP_GET, VT_I2, MB_YESNO},
    {L"vbYesNoCancel",             NULL, BP_GET, VT_I2, MB_YESNOCANCEL},
    {L"Weekday",                   Global_Weekday, 0, 1, 2},
    {L"WeekdayName",               Global_WeekdayName, 0, 1, 3},
    {L"Year",                      Global_Year, 0, 1}
};

static HRESULT err_string_prop(BSTR *prop, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR str;
    HRESULT hres;

    if(!args_cnt)
        return return_string(res, *prop ? *prop : L"");

    hres = to_string(args, &str);
    if(FAILED(hres))
        return hres;

    SysFreeString(*prop);
    *prop = str;
    return S_OK;
}

static HRESULT Err_Description(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return err_string_prop(&This->ctx->ei.bstrDescription, args, args_cnt, res);
}

static HRESULT Err_HelpContext(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    if(args_cnt) {
        FIXME("setter not implemented\n");
        return E_NOTIMPL;
    }

    return return_int(res, This->ctx->ei.dwHelpContext);
}

static HRESULT Err_HelpFile(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return err_string_prop(&This->ctx->ei.bstrHelpFile, args, args_cnt, res);
}

static HRESULT Err_Number(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    HRESULT hres;

    TRACE("\n");

    if(args_cnt) {
        FIXME("setter not implemented\n");
        return E_NOTIMPL;
    }

    hres = This->ctx->ei.scode;
    return return_int(res, HRESULT_FACILITY(hres) == FACILITY_VBS ? HRESULT_CODE(hres) : hres);
}

static HRESULT Err_Source(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");
    return err_string_prop(&This->ctx->ei.bstrSource, args, args_cnt, res);
}

static HRESULT Err_Clear(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    TRACE("\n");

    clear_ei(&This->ctx->ei);
    return S_OK;
}

static HRESULT Err_Raise(BuiltinDisp *This, VARIANT *args, unsigned args_cnt, VARIANT *res)
{
    BSTR source = NULL, description = NULL, helpfile = NULL;
    int code,  helpcontext = 0;
    HRESULT hres, error;

    TRACE("%s %u...\n", debugstr_variant(args), args_cnt);

    hres = to_int(args, &code);
    if(FAILED(hres))
        return hres;
    if(code > 0 && code > 0xffff)
        return E_INVALIDARG;

    if(args_cnt >= 2)
        hres = to_string(args + 1, &source);
    if(args_cnt >= 3 && SUCCEEDED(hres))
        hres = to_string(args + 2, &description);
    if(args_cnt >= 4 && SUCCEEDED(hres))
        hres = to_string(args + 3, &helpfile);
    if(args_cnt >= 5 && SUCCEEDED(hres))
        hres = to_int(args + 4, &helpcontext);

    if(SUCCEEDED(hres)) {
        script_ctx_t *ctx = This->ctx;

        error = (code & ~0xffff) ? map_hres(code) : MAKE_VBSERROR(code);

        if(source) {
            if(ctx->ei.bstrSource) SysFreeString(ctx->ei.bstrSource);
            ctx->ei.bstrSource = source;
        }
        if(!ctx->ei.bstrSource)
            ctx->ei.bstrSource = get_vbscript_string(VBS_RUNTIME_ERROR);
        if(description) {
            if(ctx->ei.bstrDescription) SysFreeString(ctx->ei.bstrDescription);
            ctx->ei.bstrDescription = description;
        }
        if(!ctx->ei.bstrDescription)
            ctx->ei.bstrDescription = get_vbscript_error_string(error);
        if(helpfile) {
            if(ctx->ei.bstrHelpFile) SysFreeString(ctx->ei.bstrHelpFile);
            ctx->ei.bstrHelpFile = helpfile;
        }
        if(args_cnt >= 5)
            ctx->ei.dwHelpContext = helpcontext;

        ctx->ei.scode = error;
        hres = SCRIPT_E_RECORDED;
    }else {
        SysFreeString(source);
        SysFreeString(description);
        SysFreeString(helpfile);
    }

    return hres;
}

static const builtin_prop_t err_props[] = {
    {NULL,            Err_Number, BP_GETPUT},
    {L"Clear",        Err_Clear},
    {L"Description",  Err_Description, BP_GETPUT},
    {L"HelpContext",  Err_HelpContext, BP_GETPUT},
    {L"HelpFile",     Err_HelpFile, BP_GETPUT},
    {L"Number",       Err_Number, BP_GETPUT},
    {L"Raise",        Err_Raise, 0, 1, 5},
    {L"Source",       Err_Source, BP_GETPUT}
};

void detach_global_objects(script_ctx_t *ctx)
{
    if(ctx->err_obj) {
        ctx->err_obj->ctx = NULL;
        IDispatch_Release(&ctx->err_obj->IDispatch_iface);
        ctx->err_obj = NULL;
    }

    if(ctx->global_obj) {
        ctx->global_obj->ctx = NULL;
        IDispatch_Release(&ctx->global_obj->IDispatch_iface);
        ctx->global_obj = NULL;
    }
}

HRESULT init_global(script_ctx_t *ctx)
{
    HRESULT hres;

    hres = create_builtin_dispatch(ctx, global_props, ARRAY_SIZE(global_props), &ctx->global_obj);
    if(FAILED(hres))
        return hres;

    return create_builtin_dispatch(ctx, err_props, ARRAY_SIZE(err_props), &ctx->err_obj);
}
