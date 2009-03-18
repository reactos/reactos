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

#include "mshtml_private.h"
#include "htmlstyle.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLSTYLE2_THIS(iface) DEFINE_THIS(HTMLStyle, HTMLStyle2, iface)

static HRESULT WINAPI HTMLStyle2_QueryInterface(IHTMLStyle2 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);

    return IHTMLStyle_QueryInterface(HTMLSTYLE(This), riid, ppv);
}

static ULONG WINAPI HTMLStyle2_AddRef(IHTMLStyle2 *iface)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);

    return IHTMLStyle_AddRef(HTMLSTYLE(This));
}

static ULONG WINAPI HTMLStyle2_Release(IHTMLStyle2 *iface)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);

    return IHTMLStyle_Release(HTMLSTYLE(This));
}

static HRESULT WINAPI HTMLStyle2_GetTypeInfoCount(IHTMLStyle2 *iface, UINT *pctinfo)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI HTMLStyle2_GetTypeInfo(IHTMLStyle2 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle2_GetIDsOfNames(IHTMLStyle2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle2_Invoke(IHTMLStyle2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle2_put_tableLayout(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_tableLayout(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_borderCollapse(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_borderCollapse(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_direction(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_direction(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_behavior(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_behavior(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_setExpression(IHTMLStyle2 *iface, BSTR propname, BSTR expression, BSTR language)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(propname), debugstr_w(expression), debugstr_w(language));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_getExpression(IHTMLStyle2 *iface, BSTR propname, VARIANT *expression)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), expression);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_removeExpression(IHTMLStyle2 *iface, BSTR propname, VARIANT_BOOL *pfSuccess)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), pfSuccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_position(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_POSITION, v, 0);
}

static HRESULT WINAPI HTMLStyle2_get_position(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_POSITION, p);
}

static HRESULT WINAPI HTMLStyle2_put_unicodeBidi(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_unicodeBidi(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_bottom(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_bottom(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_right(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_right(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_pixelBottom(IHTMLStyle2 *iface, long v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_pixelBottom(IHTMLStyle2 *iface, long *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_pixelRight(IHTMLStyle2 *iface, long v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_pixelRight(IHTMLStyle2 *iface, long *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_posBottom(IHTMLStyle2 *iface, float v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_posBottom(IHTMLStyle2 *iface, float *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_posRight(IHTMLStyle2 *iface, float v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_posRight(IHTMLStyle2 *iface, float *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_imeMode(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_imeMode(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyAlign(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyAlign(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyPosition(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyPosition(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyOverhang(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyOverhang(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridChar(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridChar(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridLine(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridLine(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridMode(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridMode(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridType(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridType(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGrid(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGrid(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_wordBreak(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_wordBreak(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_lineBreak(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_lineBreak(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textJustify(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textJustify(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textJustifyTrim(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textJustifyTrim(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textKashida(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textKashida(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textAutospace(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textAutospace(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_overflowX(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_overflowX(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_overflowY(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_overflowY(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_accelerator(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_accelerator(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE2_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle2Vtbl HTMLStyle2Vtbl = {
    HTMLStyle2_QueryInterface,
    HTMLStyle2_AddRef,
    HTMLStyle2_Release,
    HTMLStyle2_GetTypeInfoCount,
    HTMLStyle2_GetTypeInfo,
    HTMLStyle2_GetIDsOfNames,
    HTMLStyle2_Invoke,
    HTMLStyle2_put_tableLayout,
    HTMLStyle2_get_tableLayout,
    HTMLStyle2_put_borderCollapse,
    HTMLStyle2_get_borderCollapse,
    HTMLStyle2_put_direction,
    HTMLStyle2_get_direction,
    HTMLStyle2_put_behavior,
    HTMLStyle2_get_behavior,
    HTMLStyle2_setExpression,
    HTMLStyle2_getExpression,
    HTMLStyle2_removeExpression,
    HTMLStyle2_put_position,
    HTMLStyle2_get_position,
    HTMLStyle2_put_unicodeBidi,
    HTMLStyle2_get_unicodeBidi,
    HTMLStyle2_put_bottom,
    HTMLStyle2_get_bottom,
    HTMLStyle2_put_right,
    HTMLStyle2_get_right,
    HTMLStyle2_put_pixelBottom,
    HTMLStyle2_get_pixelBottom,
    HTMLStyle2_put_pixelRight,
    HTMLStyle2_get_pixelRight,
    HTMLStyle2_put_posBottom,
    HTMLStyle2_get_posBottom,
    HTMLStyle2_put_posRight,
    HTMLStyle2_get_posRight,
    HTMLStyle2_put_imeMode,
    HTMLStyle2_get_imeMode,
    HTMLStyle2_put_rubyAlign,
    HTMLStyle2_get_rubyAlign,
    HTMLStyle2_put_rubyPosition,
    HTMLStyle2_get_rubyPosition,
    HTMLStyle2_put_rubyOverhang,
    HTMLStyle2_get_rubyOverhang,
    HTMLStyle2_put_layoutGridChar,
    HTMLStyle2_get_layoutGridChar,
    HTMLStyle2_put_layoutGridLine,
    HTMLStyle2_get_layoutGridLine,
    HTMLStyle2_put_layoutGridMode,
    HTMLStyle2_get_layoutGridMode,
    HTMLStyle2_put_layoutGridType,
    HTMLStyle2_get_layoutGridType,
    HTMLStyle2_put_layoutGrid,
    HTMLStyle2_get_layoutGrid,
    HTMLStyle2_put_wordBreak,
    HTMLStyle2_get_wordBreak,
    HTMLStyle2_put_lineBreak,
    HTMLStyle2_get_lineBreak,
    HTMLStyle2_put_textJustify,
    HTMLStyle2_get_textJustify,
    HTMLStyle2_put_textJustifyTrim,
    HTMLStyle2_get_textJustifyTrim,
    HTMLStyle2_put_textKashida,
    HTMLStyle2_get_textKashida,
    HTMLStyle2_put_textAutospace,
    HTMLStyle2_get_textAutospace,
    HTMLStyle2_put_overflowX,
    HTMLStyle2_get_overflowX,
    HTMLStyle2_put_overflowY,
    HTMLStyle2_get_overflowY,
    HTMLStyle2_put_accelerator,
    HTMLStyle2_get_accelerator
};

void HTMLStyle2_Init(HTMLStyle *This)
{
    This->lpHTMLStyle2Vtbl = &HTMLStyle2Vtbl;
}
