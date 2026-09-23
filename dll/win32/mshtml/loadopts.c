/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "optary.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct load_opt {
    DWORD option;
    PVOID buffer;
    DWORD size;

    struct load_opt *next;
} load_opt;

typedef struct {
    IHtmlLoadOptions IHtmlLoadOptions_iface;

    LONG ref;

    load_opt *opts;
} HTMLLoadOptions;

static inline HTMLLoadOptions *impl_from_IHtmlLoadOptions(IHtmlLoadOptions *iface)
{
    return CONTAINING_RECORD(iface, HTMLLoadOptions, IHtmlLoadOptions_iface);
}

static HRESULT WINAPI HtmlLoadOptions_QueryInterface(IHtmlLoadOptions *iface,
        REFIID riid, void **ppv)
{
    HTMLLoadOptions *This = impl_from_IHtmlLoadOptions(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHtmlLoadOptions_iface;
    }else if(IsEqualGUID(&IID_IOptionArray, riid)) {
        *ppv = &This->IHtmlLoadOptions_iface;
    }else if(IsEqualGUID(&IID_IHtmlLoadOptions, riid)) {
        *ppv = &This->IHtmlLoadOptions_iface;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HtmlLoadOptions_AddRef(IHtmlLoadOptions *iface)
{
    HTMLLoadOptions *This = impl_from_IHtmlLoadOptions(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HtmlLoadOptions_Release(IHtmlLoadOptions *iface)
{
    HTMLLoadOptions *This = impl_from_IHtmlLoadOptions(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        load_opt *iter = This->opts, *last;

        while(iter) {
            last = iter;
            iter = iter->next;

            free(last->buffer);
            free(last);
        }

        free(This);
    }

    return ref;
}

static HRESULT WINAPI HtmlLoadOptions_QueryOption(IHtmlLoadOptions *iface, DWORD dwOption,
        LPVOID pBuffer, ULONG *pcbBuf)
{
    HTMLLoadOptions *This = impl_from_IHtmlLoadOptions(iface);
    load_opt *iter;

    TRACE("(%p)->(%ld %p %p)\n", This, dwOption, pBuffer, pcbBuf);

    for(iter = This->opts; iter; iter = iter->next) {
        if(iter->option == dwOption)
            break;
    }

    if(!iter) {
        *pcbBuf = 0;
        return S_OK;
    }

    if(*pcbBuf < iter->size) {
        *pcbBuf = iter->size;
        return E_FAIL;
    }

    memcpy(pBuffer, iter->buffer, iter->size);
    *pcbBuf = iter->size;

    return S_OK;
}

static HRESULT WINAPI HtmlLoadOptions_SetOption(IHtmlLoadOptions *iface, DWORD dwOption,
        LPVOID pBuffer, ULONG cbBuf)
{
    HTMLLoadOptions *This = impl_from_IHtmlLoadOptions(iface);
    load_opt *iter = NULL;

    TRACE("(%p)->(%ld %p %ld)\n", This, dwOption, pBuffer, cbBuf);

    for(iter = This->opts; iter; iter = iter->next) {
        if(iter->option == dwOption)
            break;
    }

    if(!iter) {
        iter = malloc(sizeof(load_opt));
        iter->next = This->opts;
        This->opts = iter;

        iter->option = dwOption;
    }else {
        free(iter->buffer);
    }

    if(!cbBuf) {
        iter->buffer = NULL;
        iter->size = 0;

        return S_OK;
    }

    iter->size = cbBuf;
    iter->buffer = malloc(cbBuf);
    memcpy(iter->buffer, pBuffer, iter->size);

    return S_OK;
}

static const IHtmlLoadOptionsVtbl HtmlLoadOptionsVtbl = {
    HtmlLoadOptions_QueryInterface,
    HtmlLoadOptions_AddRef,
    HtmlLoadOptions_Release,
    HtmlLoadOptions_QueryOption,
    HtmlLoadOptions_SetOption
};

HRESULT HTMLLoadOptions_Create(IUnknown *pUnkOuter, REFIID riid, void** ppv)
{
    HTMLLoadOptions *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_mshtml_guid(riid), ppv);

    ret = malloc(sizeof(HTMLLoadOptions));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHtmlLoadOptions_iface.lpVtbl = &HtmlLoadOptionsVtbl;
    ret->ref = 1;
    ret->opts = NULL;

    hres = IHtmlLoadOptions_QueryInterface(&ret->IHtmlLoadOptions_iface, riid, ppv);
    IHtmlLoadOptions_Release(&ret->IHtmlLoadOptions_iface);
    return hres;
}
