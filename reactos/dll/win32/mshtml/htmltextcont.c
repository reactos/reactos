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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLTEXTCONT_THIS(iface) DEFINE_THIS(HTMLTextContainer, HTMLTextContainer, iface)

static HRESULT WINAPI HTMLTextContainer_QueryInterface(IHTMLTextContainer *iface,
                                                       REFIID riid, void **ppv)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    return IHTMLElement_QueryInterface(HTMLELEM(&This->element), riid, ppv);
}

static ULONG WINAPI HTMLTextContainer_AddRef(IHTMLTextContainer *iface)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    return IHTMLElement_AddRef(HTMLELEM(&This->element));
}

static ULONG WINAPI HTMLTextContainer_Release(IHTMLTextContainer *iface)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    return IHTMLElement_Release(HTMLELEM(&This->element));
}

static HRESULT WINAPI HTMLTextContainer_GetTypeInfoCount(IHTMLTextContainer *iface, UINT *pctinfo)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_GetTypeInfo(IHTMLTextContainer *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_GetIDsOfNames(IHTMLTextContainer *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_Invoke(IHTMLTextContainer *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_createControlRange(IHTMLTextContainer *iface,
                                                           IDispatch **range)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_get_scrollHeight(IHTMLTextContainer *iface, long *p)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollHeight(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollWidth(IHTMLTextContainer *iface, long *p)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollWidth(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLTextContainer_put_scrollTop(IHTMLTextContainer *iface, long v)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return IHTMLElement2_put_scrollTop(HTMLELEM2(&This->element), v);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollTop(IHTMLTextContainer *iface, long *p)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollTop(HTMLELEM2(&This->element), p);
}

static HRESULT WINAPI HTMLTextContainer_put_scrollLeft(IHTMLTextContainer *iface, long v)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return IHTMLElement2_put_scrollLeft(HTMLELEM2(&This->element), v);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollLeft(IHTMLTextContainer *iface, long *p)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_put_onscroll(IHTMLTextContainer *iface, VARIANT v)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_get_onscroll(IHTMLTextContainer *iface, VARIANT *p)
{
    HTMLTextContainer *This = HTMLTEXTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLTEXTCONT_THIS

static const IHTMLTextContainerVtbl HTMLTextContainerVtbl = {
    HTMLTextContainer_QueryInterface,
    HTMLTextContainer_AddRef,
    HTMLTextContainer_Release,
    HTMLTextContainer_GetTypeInfoCount,
    HTMLTextContainer_GetTypeInfo,
    HTMLTextContainer_GetIDsOfNames,
    HTMLTextContainer_Invoke,
    HTMLTextContainer_createControlRange,
    HTMLTextContainer_get_scrollHeight,
    HTMLTextContainer_get_scrollWidth,
    HTMLTextContainer_put_scrollTop,
    HTMLTextContainer_get_scrollTop,
    HTMLTextContainer_put_scrollLeft,
    HTMLTextContainer_get_scrollLeft,
    HTMLTextContainer_put_onscroll,
    HTMLTextContainer_get_onscroll
};

void HTMLTextContainer_Init(HTMLTextContainer *This)
{
    HTMLElement_Init(&This->element);

    This->lpHTMLTextContainerVtbl = &HTMLTextContainerVtbl;

    ConnectionPoint_Init(&This->cp, &This->element.cp_container, &DIID_HTMLTextContainerEvents);
}
