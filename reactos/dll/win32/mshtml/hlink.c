/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

/**********************************************************
 * IHlinkTarget implementation
 */

static inline HTMLDocument *impl_from_IHlinkTarget(IHlinkTarget *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHlinkTarget_iface);
}

static HRESULT WINAPI HlinkTarget_QueryInterface(IHlinkTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HlinkTarget_AddRef(IHlinkTarget *iface)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HlinkTarget_Release(IHlinkTarget *iface)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HlinkTarget_SetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext *pihlbc)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext **ppihlbc)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_Navigate(IHlinkTarget *iface, DWORD grfHLNF, LPCWSTR pwzJumpLocation)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);

    TRACE("(%p)->(%08x %s)\n", This, grfHLNF, debugstr_w(pwzJumpLocation));

    if(grfHLNF)
        FIXME("Unsupported grfHLNF=%08x\n", grfHLNF);
    if(pwzJumpLocation)
        FIXME("JumpLocation not supported\n");

    if(!This->doc_obj->client)
        return navigate_new_window(This->window, This->window->uri, NULL, NULL);

    return IOleObject_DoVerb(&This->IOleObject_iface, OLEIVERB_SHOW, NULL, NULL, -1, NULL, NULL);
}

static HRESULT WINAPI HlinkTarget_GetMoniker(IHlinkTarget *iface, LPCWSTR pwzLocation, DWORD dwAssign,
        IMoniker **ppimkLocation)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    FIXME("(%p)->(%s %08x %p)\n", This, debugstr_w(pwzLocation), dwAssign, ppimkLocation);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetFriendlyName(IHlinkTarget *iface, LPCWSTR pwzLocation,
        LPWSTR *ppwzFriendlyName)
{
    HTMLDocument *This = impl_from_IHlinkTarget(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(pwzLocation), ppwzFriendlyName);
    return E_NOTIMPL;
}

static const IHlinkTargetVtbl HlinkTargetVtbl = {
    HlinkTarget_QueryInterface,
    HlinkTarget_AddRef,
    HlinkTarget_Release,
    HlinkTarget_SetBrowseContext,
    HlinkTarget_GetBrowseContext,
    HlinkTarget_Navigate,
    HlinkTarget_GetMoniker,
    HlinkTarget_GetFriendlyName
};

void HTMLDocument_Hlink_Init(HTMLDocument *This)
{
    This->IHlinkTarget_iface.lpVtbl = &HlinkTargetVtbl;
}
