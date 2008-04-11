/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    const IOmNavigatorVtbl  *lpIOmNavigatorVtbl;

    LONG ref;
} OmNavigator;

#define OMNAVIGATOR(x)  ((IOmNavigator*) &(x)->lpIOmNavigatorVtbl)

#define OMNAVIGATOR_THIS(iface) DEFINE_THIS(OmNavigator, IOmNavigator, iface)

static HRESULT WINAPI OmNavigator_QueryInterface(IOmNavigator *iface, REFIID riid, void **ppv)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = OMNAVIGATOR(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = OMNAVIGATOR(This);
    }else if(IsEqualGUID(&IID_IOmNavigator, riid)) {
        TRACE("(%p)->(IID_IOmNavigator %p)\n", This, ppv);
        *ppv = OMNAVIGATOR(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI OmNavigator_AddRef(IOmNavigator *iface)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI OmNavigator_Release(IOmNavigator *iface)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI OmNavigator_GetTypeInfoCount(IOmNavigator *iface, UINT *pctinfo)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_GetTypeInfo(IOmNavigator *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_GetIDsOfNames(IOmNavigator *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_Invoke(IOmNavigator *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_appCodeName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_appName(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_appVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_userAgent(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_javaEnabled(IOmNavigator *iface, VARIANT_BOOL *enabled)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_taintEnabled(IOmNavigator *iface, VARIANT_BOOL *enabled)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, enabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_mimeTypes(IOmNavigator *iface, IHTMLMimeTypesCollection **p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_plugins(IOmNavigator *iface, IHTMLPluginsCollection **p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_cookieEnabled(IOmNavigator *iface, VARIANT_BOOL *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_opsProfile(IOmNavigator *iface, IHTMLOpsProfile **p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_toString(IOmNavigator *iface, BSTR *String)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_cpuClass(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_systemLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_browserLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_userLanguage(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_platform(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_appMinorVersion(IOmNavigator *iface, BSTR *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_connectionSpeed(IOmNavigator *iface, long *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_onLine(IOmNavigator *iface, VARIANT_BOOL *p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI OmNavigator_get_userProfile(IOmNavigator *iface, IHTMLOpsProfile **p)
{
    OmNavigator *This = OMNAVIGATOR_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef OMNAVIGATOR_THIS

static const IOmNavigatorVtbl OmNavigatorVtbl = {
    OmNavigator_QueryInterface,
    OmNavigator_AddRef,
    OmNavigator_Release,
    OmNavigator_GetTypeInfoCount,
    OmNavigator_GetTypeInfo,
    OmNavigator_GetIDsOfNames,
    OmNavigator_Invoke,
    OmNavigator_get_appCodeName,
    OmNavigator_get_appName,
    OmNavigator_get_appVersion,
    OmNavigator_get_userAgent,
    OmNavigator_javaEnabled,
    OmNavigator_taintEnabled,
    OmNavigator_get_mimeTypes,
    OmNavigator_get_plugins,
    OmNavigator_get_cookieEnabled,
    OmNavigator_get_opsProfile,
    OmNavigator_toString,
    OmNavigator_get_cpuClass,
    OmNavigator_get_systemLanguage,
    OmNavigator_get_browserLanguage,
    OmNavigator_get_userLanguage,
    OmNavigator_get_platform,
    OmNavigator_get_appMinorVersion,
    OmNavigator_get_connectionSpeed,
    OmNavigator_get_onLine,
    OmNavigator_get_userProfile
};

IOmNavigator *OmNavigator_Create(void)
{
    OmNavigator *ret;

    ret = heap_alloc(sizeof(*ret));
    ret->lpIOmNavigatorVtbl = &OmNavigatorVtbl;
    ret->ref = 1;

    return OMNAVIGATOR(ret);
}
