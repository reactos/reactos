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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "objsafe.h"
#include "activscp.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

/* Defined as extern in urlmon.idl, but not exported by uuid.lib */
const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY =
    {0x10200490,0xfa38,0x11d0,{0xac,0x0e,0x00,0xa0,0xc9,0xf,0xff,0xc0}};

#define HOSTSECMGR_THIS(iface) DEFINE_THIS(HTMLDocumentNode, IInternetHostSecurityManager, iface)

static HRESULT WINAPI InternetHostSecurityManager_QueryInterface(IInternetHostSecurityManager *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->node), riid, ppv);
}

static ULONG WINAPI InternetHostSecurityManager_AddRef(IInternetHostSecurityManager *iface)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->node));
}

static ULONG WINAPI InternetHostSecurityManager_Release(IInternetHostSecurityManager *iface)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->node));
}

static HRESULT WINAPI InternetHostSecurityManager_GetSecurityId(IInternetHostSecurityManager *iface,  BYTE *pbSecurityId,
        DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    FIXME("(%p)->(%p %p %lx)\n", This, pbSecurityId, pcbSecurityId, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetHostSecurityManager_ProcessUrlAction(IInternetHostSecurityManager *iface, DWORD dwAction,
        BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    const WCHAR *url;

    TRACE("(%p)->(%d %p %d %p %d %x %x)\n", This, dwAction, pPolicy, cbPolicy, pContext, cbContext, dwFlags, dwReserved);

    url = This->basedoc.window->url ? This->basedoc.window->url : about_blankW;

    return IInternetSecurityManager_ProcessUrlAction(This->secmgr, url, dwAction, pPolicy, cbPolicy,
            pContext, cbContext, dwFlags, dwReserved);
}

static HRESULT confirm_safety(HTMLDocumentNode *This, const WCHAR *url, struct CONFIRMSAFETY *cs, DWORD *ret)
{
    DWORD policy, enabled_opts, supported_opts;
    IObjectSafety *obj_safety;
    HRESULT hres;

    TRACE("%s %p %s\n", debugstr_w(url), cs->pUnk, debugstr_guid(&cs->clsid));

    /* FIXME: Check URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY */

    hres = IInternetSecurityManager_ProcessUrlAction(This->secmgr, url, URLACTION_SCRIPT_SAFE_ACTIVEX,
            (BYTE*)&policy, sizeof(policy), NULL, 0, 0, 0);
    if(FAILED(hres) || policy != URLPOLICY_ALLOW) {
        *ret = URLPOLICY_DISALLOW;
        return S_OK;
    }

    hres = IUnknown_QueryInterface(cs->pUnk, &IID_IObjectSafety, (void**)&obj_safety);
    if(FAILED(hres)) {
        CATID scripting_catid = CATID_SafeForScripting;

        if(!This->catmgr) {
            hres = CoCreateInstance(&CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER,
                    &IID_ICatInformation, (void**)&This->catmgr);
            if(FAILED(hres))
                return hres;
        }

        hres = ICatInformation_IsClassOfCategories(This->catmgr, &cs->clsid, 1, &scripting_catid, 0, NULL);
        if(FAILED(hres))
            return hres;

        *ret = hres == S_OK ? URLPOLICY_ALLOW : URLPOLICY_DISALLOW;
        return S_OK;
    }

    hres = IObjectSafety_GetInterfaceSafetyOptions(obj_safety, &IID_IDispatchEx, &supported_opts, &enabled_opts);
    if(FAILED(hres))
        supported_opts = 0;

    enabled_opts = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
    if(supported_opts & INTERFACE_USES_SECURITY_MANAGER)
        enabled_opts |= INTERFACE_USES_SECURITY_MANAGER;

    hres = IObjectSafety_SetInterfaceSafetyOptions(obj_safety, &IID_IDispatchEx, enabled_opts, enabled_opts);
    if(FAILED(hres)) {
        enabled_opts &= ~INTERFACE_USES_SECURITY_MANAGER;
        hres = IObjectSafety_SetInterfaceSafetyOptions(obj_safety, &IID_IDispatch, enabled_opts, enabled_opts);
    }
    IObjectSafety_Release(obj_safety);

    *ret = SUCCEEDED(hres) ? URLPOLICY_ALLOW : URLPOLICY_DISALLOW;
    return S_OK;
}

static HRESULT WINAPI InternetHostSecurityManager_QueryCustomPolicy(IInternetHostSecurityManager *iface, REFGUID guidKey,
        BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved)
{
    HTMLDocumentNode *This = HOSTSECMGR_THIS(iface);
    const WCHAR *url;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p %p %d %x)\n", This, debugstr_guid(guidKey), ppPolicy, pcbPolicy, pContext, cbContext, dwReserved);

    url = This->basedoc.window->url ? This->basedoc.window->url : about_blankW;

    hres = IInternetSecurityManager_QueryCustomPolicy(This->secmgr, url, guidKey, ppPolicy, pcbPolicy,
            pContext, cbContext, dwReserved);
    if(hres != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        return hres;

    if(IsEqualGUID(&GUID_CUSTOM_CONFIRMOBJECTSAFETY, guidKey)) {
        IActiveScript *active_script;
        struct CONFIRMSAFETY *cs;
        DWORD policy;

        if(cbContext != sizeof(struct CONFIRMSAFETY)) {
            FIXME("wrong context size\n");
            return E_FAIL;
        }

        cs = (struct CONFIRMSAFETY*)pContext;
        hres = IUnknown_QueryInterface(cs->pUnk, &IID_IActiveScript, (void**)&active_script);
        if(SUCCEEDED(hres)) {
            FIXME("Got IAciveScript iface\n");
            IActiveScript_Release(active_script);
            return E_FAIL;
        }

        hres = confirm_safety(This, url, cs, &policy);
        if(FAILED(hres))
            return hres;

        *ppPolicy = CoTaskMemAlloc(sizeof(policy));
        if(!*ppPolicy)
            return E_OUTOFMEMORY;

        *(DWORD*)*ppPolicy = policy;
        *pcbPolicy = sizeof(policy);
        TRACE("policy %x\n", policy);
        return S_OK;
    }

    FIXME("Unknown guidKey %s\n", debugstr_guid(guidKey));
    return hres;
}

#undef HOSTSECMGR_THIS

static const IInternetHostSecurityManagerVtbl InternetHostSecurityManagerVtbl = {
    InternetHostSecurityManager_QueryInterface,
    InternetHostSecurityManager_AddRef,
    InternetHostSecurityManager_Release,
    InternetHostSecurityManager_GetSecurityId,
    InternetHostSecurityManager_ProcessUrlAction,
    InternetHostSecurityManager_QueryCustomPolicy
};

void HTMLDocumentNode_SecMgr_Init(HTMLDocumentNode *This)
{
    This->lpIInternetHostSecurityManagerVtbl = &InternetHostSecurityManagerVtbl;
}
