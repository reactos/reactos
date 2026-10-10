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

#include "ieframe.h"
#include "urlhist.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

static HRESULT WINAPI UrlHistoryStg_QueryInterface(IUrlHistoryStg2 *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(IID_IUnknown %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IUrlHistoryStg, riid)) {
        TRACE("(IID_IUrlHistoryStg %p)\n", ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IUrlHistoryStg2, riid)) {
        TRACE("(IID_IUrlHistoryStg2 %p)\n", ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%s %p)\n", debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI UrlHistoryStg_AddRef(IUrlHistoryStg2 *iface)
{
    lock_module();
    return 2;
}

static ULONG WINAPI UrlHistoryStg_Release(IUrlHistoryStg2 *iface)
{
    unlock_module();
    return 1;
}

static HRESULT WINAPI UrlHistoryStg_AddUrl(IUrlHistoryStg2 *iface, LPCOLESTR lpcsUrl,
        LPCOLESTR pocsTitle, DWORD dwFlags)
{
    FIXME("(%s %s %08lx)\n", debugstr_w(lpcsUrl), debugstr_w(pocsTitle), dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI UrlHistoryStg_DeleteUrl(IUrlHistoryStg2 *iface, LPCOLESTR lpcsUrl,
        DWORD dwFlags)
{
    FIXME("(%s %08lx)\n", debugstr_w(lpcsUrl), dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI UrlHistoryStg_QueryUrl(IUrlHistoryStg2 *iface, LPCOLESTR lpcsUrl,
        DWORD dwFlags, LPSTATURL lpSTATURL)
{
    FIXME("(%s %08lx %p)\n", debugstr_w(lpcsUrl), dwFlags, lpSTATURL);
    return E_NOTIMPL;
}

static HRESULT WINAPI UrlHistoryStg_BindToObject(IUrlHistoryStg2 *iface, LPCOLESTR lpcsUrl,
        REFIID riid, void **ppv)
{
    FIXME("(%s %s %p)\n", debugstr_w(lpcsUrl), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI UrlHistoryStg_EnumUrls(IUrlHistoryStg2 *iface, IEnumSTATURL **ppEnum)
{
    FIXME("(%p)\n", ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI UrlHistoryStg_AddUrlAndNotify(IUrlHistoryStg2 *iface, LPCOLESTR pocsUrl,
        LPCOLESTR pocsTitle, DWORD dwFlags, BOOL fWriteHistory, IOleCommandTarget *poctNotify,
        IUnknown *punkISFolder)
{
    FIXME("(%s %s %08lx %x %p %p)\n", debugstr_w(pocsUrl), debugstr_w(pocsTitle),
          dwFlags, fWriteHistory, poctNotify, punkISFolder);
    return E_NOTIMPL;
}

static HRESULT WINAPI UrlHistoryStg_ClearHistory(IUrlHistoryStg2 *iface)
{
    FIXME("()\n");
    return E_NOTIMPL;
}

static const IUrlHistoryStg2Vtbl UrlHistoryStg2Vtbl = {
    UrlHistoryStg_QueryInterface,
    UrlHistoryStg_AddRef,
    UrlHistoryStg_Release,
    UrlHistoryStg_AddUrl,
    UrlHistoryStg_DeleteUrl,
    UrlHistoryStg_QueryUrl,
    UrlHistoryStg_BindToObject,
    UrlHistoryStg_EnumUrls,
    UrlHistoryStg_AddUrlAndNotify,
    UrlHistoryStg_ClearHistory
};

static IUrlHistoryStg2 UrlHistoryStg2 = { &UrlHistoryStg2Vtbl };

HRESULT WINAPI CUrlHistory_Create(IClassFactory *iface, IUnknown *pOuter, REFIID riid, void **ppv)
{
    if(pOuter)
        return CLASS_E_NOAGGREGATION;

    return IUrlHistoryStg2_QueryInterface(&UrlHistoryStg2, riid, ppv);
}
