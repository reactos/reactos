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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/**********************************************************
 * IHlinkTarget implementation
 */

#define HLINKTRG_THIS(iface) DEFINE_THIS(HTMLDocument, HlinkTarget, iface)

static HRESULT WINAPI HlinkTarget_QueryInterface(IHlinkTarget *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI HlinkTarget_AddRef(IHlinkTarget *iface)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI HlinkTarget_Release(IHlinkTarget *iface)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI HlinkTarget_SetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext *pihlbc)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetBrowseContext(IHlinkTarget *iface, IHlinkBrowseContext **ppihlbc)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_Navigate(IHlinkTarget *iface, DWORD grfHLNF, LPCWSTR pwzJumpLocation)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);

    TRACE("(%p)->(%08x %s)\n", This, grfHLNF, debugstr_w(pwzJumpLocation));

    if(grfHLNF)
        FIXME("Unsupported grfHLNF=%08x\n", grfHLNF);
    if(pwzJumpLocation)
        FIXME("JumpLocation not supported\n");

    return IOleObject_DoVerb(OLEOBJ(This), OLEIVERB_SHOW, NULL, NULL, -1, NULL, NULL);
}

static HRESULT WINAPI HlinkTarget_GetMoniker(IHlinkTarget *iface, LPCWSTR pwzLocation, DWORD dwAssign,
        IMoniker **ppimkLocation)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
    FIXME("(%p)->(%s %08x %p)\n", This, debugstr_w(pwzLocation), dwAssign, ppimkLocation);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkTarget_GetFriendlyName(IHlinkTarget *iface, LPCWSTR pwzLocation,
        LPWSTR *ppwzFriendlyName)
{
    HTMLDocument *This = HLINKTRG_THIS(iface);
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
    This->lpHlinkTargetVtbl = &HlinkTargetVtbl;
}
