/*
 * Copyright 2009 Jacek Caban for CodeWeavers
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


#include "jscript.h"
#include "objsafe.h"
#include "mshtmhst.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

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

static IUnknown *create_activex_object(script_ctx_t *ctx, const WCHAR *progid)
{
    IInternetHostSecurityManager *secmgr = NULL;
    IObjectWithSite *obj_site;
    struct CONFIRMSAFETY cs;
    IClassFactoryEx *cfex;
    IClassFactory *cf;
    DWORD policy_size;
    BYTE *bpolicy;
    IUnknown *obj;
    DWORD policy;
    GUID guid;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(progid));

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

    hres = IUnknown_QueryInterface(obj, &IID_IObjectWithSite, (void**)&obj_site);
    if(SUCCEEDED(hres)) {
        IUnknown *ax_site;

        ax_site = create_ax_site(ctx);
        if(ax_site) {
            hres = IObjectWithSite_SetSite(obj_site, ax_site);
            IUnknown_Release(ax_site);
        }
        IObjectWithSite_Release(obj_site);
        if(!ax_site || FAILED(hres)) {
            IUnknown_Release(obj);
            return NULL;
        }
    }

    return obj;
}

static HRESULT ActiveXObject_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsstr_t * progid_str;
    const WCHAR *progid;
    IDispatch *disp;
    IUnknown *obj;
    HRESULT hres;

    TRACE("\n");

    if(flags != DISPATCH_CONSTRUCT) {
        FIXME("unsupported flags %x\n", flags);
        return E_NOTIMPL;
    }

    if(ctx->safeopt != (INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_DISPEX|INTERFACE_USES_SECURITY_MANAGER)
        && ctx->safeopt != INTERFACE_USES_DISPEX) {
        FIXME("Unsupported safeopt %lx\n", ctx->safeopt);
        return E_NOTIMPL;
    }

    if(argc != 1) {
        FIXME("unsupported argc %d\n", argc);
        return E_NOTIMPL;
    }

    hres = to_flat_string(ctx, argv[0], &progid_str, &progid);
    if(FAILED(hres))
        return hres;

    obj = create_activex_object(ctx, progid);
    jsstr_release(progid_str);
    if(!obj)
        return JS_E_CANNOT_CREATE_OBJ;

    hres = IUnknown_QueryInterface(obj, &IID_IDispatch, (void**)&disp);
    IUnknown_Release(obj);
    if(FAILED(hres)) {
        FIXME("Object does not support IDispatch\n");
        return E_NOTIMPL;
    }

    if(r) *r = jsval_disp(disp);
    else  IDispatch_Release(disp);
    return S_OK;
}

HRESULT create_activex_constr(script_ctx_t *ctx, jsdisp_t **ret)
{
    jsdisp_t *prototype;
    HRESULT hres;

    hres = create_object(ctx, NULL, &prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_function(ctx, ActiveXObject_value, L"ActiveXObject", NULL,
            PROPF_CONSTR|1, prototype, ret);

    jsdisp_release(prototype);
    return hres;
}
