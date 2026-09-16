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
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "htmlstyle.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLCurrentStyle {
    CSSStyle css_style;
    IHTMLCurrentStyle  IHTMLCurrentStyle_iface;
    IHTMLCurrentStyle2 IHTMLCurrentStyle2_iface;
    IHTMLCurrentStyle3 IHTMLCurrentStyle3_iface;
    IHTMLCurrentStyle4 IHTMLCurrentStyle4_iface;
    IWineCSSProperties IWineCSSProperties_iface;

    HTMLElement *elem;
};

static inline HRESULT get_current_style_property(HTMLCurrentStyle *current_style, styleid_t sid, BSTR *p)
{
    return get_style_property(&current_style->css_style, sid, p);
}

static inline HRESULT get_current_style_property_var(HTMLCurrentStyle *This, styleid_t sid, VARIANT *v)
{
    return get_style_property_var(&This->css_style, sid, v);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle(IHTMLCurrentStyle *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle_iface);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle2(IHTMLCurrentStyle2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle2_iface);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle3(IHTMLCurrentStyle3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle3_iface);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle4(IHTMLCurrentStyle4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle4_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLCurrentStyle, IHTMLCurrentStyle,
                      impl_from_IHTMLCurrentStyle(iface)->css_style.dispex)

static HRESULT WINAPI HTMLCurrentStyle_get_position(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_current_style_property(This, STYLEID_POSITION, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_styleFloat(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_current_style_property(This, STYLEID_FLOAT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_color(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BACKGROUND_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontFamily(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_current_style_property(This, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontVariant(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontWeight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_FONT_WEIGHT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontSize(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_FONT_SIZE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundImage(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundPositionX(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundPositionY(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundRepeat(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BACKGROUND_REPEAT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_LEFT_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_TOP_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_RIGHT_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_BOTTOM_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_TOP_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_RIGHT_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_BOTTOM_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BORDER_LEFT_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_left(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_top(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_width(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_height(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_PADDING_LEFT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_PADDING_TOP, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_PADDING_RIGHT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_PADDING_BOTTOM, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textAlign(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textDecoration(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_display(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_current_style_property(This, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_visibility(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_current_style_property(This, STYLEID_VISIBILITY, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_zIndex(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_Z_INDEX, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_letterSpacing(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_LETTER_SPACING, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_lineHeight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_LINE_HEIGHT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textIndent(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_TEXT_INDENT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_verticalAlign(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_VERTICAL_ALIGN, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundAttachment(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_MARGIN_TOP, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_MARGIN_RIGHT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_MARGIN_BOTTOM, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_MARGIN_LEFT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_clear(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStyleType(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStylePosition(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStyleImage(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflow(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_OVERFLOW, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_pageBreakBefore(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_pageBreakAfter(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_cursor(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_tableLayout(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderCollapse(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_direction(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_DIRECTION, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_behavior(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_getAttribute(IHTMLCurrentStyle *iface, BSTR strAttributeName,
        LONG lFlags, VARIANT *AttributeValue)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%s %lx %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_unicodeBidi(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_right(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_RIGHT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_bottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_BOTTOM, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_imeMode(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyAlign(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyPosition(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyOverhang(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textAutospace(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_lineBreak(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_wordBreak(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textJustify(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textJustifyTrim(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textKashida(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_blockDirection(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridChar(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridLine(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridMode(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridType(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderColor(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderWidth(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_BORDER_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_padding(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_PADDING, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_margin(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_MARGIN, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_accelerator(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflowX(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_OVERFLOW_X, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflowY(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_OVERFLOW_Y, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textTransform(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property(This, STYLEID_TEXT_TRANSFORM, p);
}

static const IHTMLCurrentStyleVtbl HTMLCurrentStyleVtbl = {
    HTMLCurrentStyle_QueryInterface,
    HTMLCurrentStyle_AddRef,
    HTMLCurrentStyle_Release,
    HTMLCurrentStyle_GetTypeInfoCount,
    HTMLCurrentStyle_GetTypeInfo,
    HTMLCurrentStyle_GetIDsOfNames,
    HTMLCurrentStyle_Invoke,
    HTMLCurrentStyle_get_position,
    HTMLCurrentStyle_get_styleFloat,
    HTMLCurrentStyle_get_color,
    HTMLCurrentStyle_get_backgroundColor,
    HTMLCurrentStyle_get_fontFamily,
    HTMLCurrentStyle_get_fontStyle,
    HTMLCurrentStyle_get_fontVariant,
    HTMLCurrentStyle_get_fontWeight,
    HTMLCurrentStyle_get_fontSize,
    HTMLCurrentStyle_get_backgroundImage,
    HTMLCurrentStyle_get_backgroundPositionX,
    HTMLCurrentStyle_get_backgroundPositionY,
    HTMLCurrentStyle_get_backgroundRepeat,
    HTMLCurrentStyle_get_borderLeftColor,
    HTMLCurrentStyle_get_borderTopColor,
    HTMLCurrentStyle_get_borderRightColor,
    HTMLCurrentStyle_get_borderBottomColor,
    HTMLCurrentStyle_get_borderTopStyle,
    HTMLCurrentStyle_get_borderRightStyle,
    HTMLCurrentStyle_get_borderBottomStyle,
    HTMLCurrentStyle_get_borderLeftStyle,
    HTMLCurrentStyle_get_borderTopWidth,
    HTMLCurrentStyle_get_borderRightWidth,
    HTMLCurrentStyle_get_borderBottomWidth,
    HTMLCurrentStyle_get_borderLeftWidth,
    HTMLCurrentStyle_get_left,
    HTMLCurrentStyle_get_top,
    HTMLCurrentStyle_get_width,
    HTMLCurrentStyle_get_height,
    HTMLCurrentStyle_get_paddingLeft,
    HTMLCurrentStyle_get_paddingTop,
    HTMLCurrentStyle_get_paddingRight,
    HTMLCurrentStyle_get_paddingBottom,
    HTMLCurrentStyle_get_textAlign,
    HTMLCurrentStyle_get_textDecoration,
    HTMLCurrentStyle_get_display,
    HTMLCurrentStyle_get_visibility,
    HTMLCurrentStyle_get_zIndex,
    HTMLCurrentStyle_get_letterSpacing,
    HTMLCurrentStyle_get_lineHeight,
    HTMLCurrentStyle_get_textIndent,
    HTMLCurrentStyle_get_verticalAlign,
    HTMLCurrentStyle_get_backgroundAttachment,
    HTMLCurrentStyle_get_marginTop,
    HTMLCurrentStyle_get_marginRight,
    HTMLCurrentStyle_get_marginBottom,
    HTMLCurrentStyle_get_marginLeft,
    HTMLCurrentStyle_get_clear,
    HTMLCurrentStyle_get_listStyleType,
    HTMLCurrentStyle_get_listStylePosition,
    HTMLCurrentStyle_get_listStyleImage,
    HTMLCurrentStyle_get_clipTop,
    HTMLCurrentStyle_get_clipRight,
    HTMLCurrentStyle_get_clipBottom,
    HTMLCurrentStyle_get_clipLeft,
    HTMLCurrentStyle_get_overflow,
    HTMLCurrentStyle_get_pageBreakBefore,
    HTMLCurrentStyle_get_pageBreakAfter,
    HTMLCurrentStyle_get_cursor,
    HTMLCurrentStyle_get_tableLayout,
    HTMLCurrentStyle_get_borderCollapse,
    HTMLCurrentStyle_get_direction,
    HTMLCurrentStyle_get_behavior,
    HTMLCurrentStyle_getAttribute,
    HTMLCurrentStyle_get_unicodeBidi,
    HTMLCurrentStyle_get_right,
    HTMLCurrentStyle_get_bottom,
    HTMLCurrentStyle_get_imeMode,
    HTMLCurrentStyle_get_rubyAlign,
    HTMLCurrentStyle_get_rubyPosition,
    HTMLCurrentStyle_get_rubyOverhang,
    HTMLCurrentStyle_get_textAutospace,
    HTMLCurrentStyle_get_lineBreak,
    HTMLCurrentStyle_get_wordBreak,
    HTMLCurrentStyle_get_textJustify,
    HTMLCurrentStyle_get_textJustifyTrim,
    HTMLCurrentStyle_get_textKashida,
    HTMLCurrentStyle_get_blockDirection,
    HTMLCurrentStyle_get_layoutGridChar,
    HTMLCurrentStyle_get_layoutGridLine,
    HTMLCurrentStyle_get_layoutGridMode,
    HTMLCurrentStyle_get_layoutGridType,
    HTMLCurrentStyle_get_borderStyle,
    HTMLCurrentStyle_get_borderColor,
    HTMLCurrentStyle_get_borderWidth,
    HTMLCurrentStyle_get_padding,
    HTMLCurrentStyle_get_margin,
    HTMLCurrentStyle_get_accelerator,
    HTMLCurrentStyle_get_overflowX,
    HTMLCurrentStyle_get_overflowY,
    HTMLCurrentStyle_get_textTransform
};

DISPEX_IDISPATCH_IMPL(HTMLCurrentStyle2, IHTMLCurrentStyle2,
                      impl_from_IHTMLCurrentStyle2(iface)->css_style.dispex)

static HRESULT WINAPI HTMLCurrentStyle2_get_layoutFlow(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_wordWrap(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_textUnderlinePosition(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_hasLayout(IHTMLCurrentStyle2 *iface, VARIANT_BOOL *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);

    FIXME("(%p)->(%p) returning true\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarBaseColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarFaceColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbar3dLightColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarShadowColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarHighlightColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarDarkShadowColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarArrowColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarTrackColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_writingMode(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_zoom(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_filter(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->elem->filter) {
        *p = SysAllocString(This->elem->filter);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_textAlignLast(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_textKashidaSpace(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_isBlock(IHTMLCurrentStyle2 *iface, VARIANT_BOOL *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCurrentStyle2Vtbl HTMLCurrentStyle2Vtbl = {
    HTMLCurrentStyle2_QueryInterface,
    HTMLCurrentStyle2_AddRef,
    HTMLCurrentStyle2_Release,
    HTMLCurrentStyle2_GetTypeInfoCount,
    HTMLCurrentStyle2_GetTypeInfo,
    HTMLCurrentStyle2_GetIDsOfNames,
    HTMLCurrentStyle2_Invoke,
    HTMLCurrentStyle2_get_layoutFlow,
    HTMLCurrentStyle2_get_wordWrap,
    HTMLCurrentStyle2_get_textUnderlinePosition,
    HTMLCurrentStyle2_get_hasLayout,
    HTMLCurrentStyle2_get_scrollbarBaseColor,
    HTMLCurrentStyle2_get_scrollbarFaceColor,
    HTMLCurrentStyle2_get_scrollbar3dLightColor,
    HTMLCurrentStyle2_get_scrollbarShadowColor,
    HTMLCurrentStyle2_get_scrollbarHighlightColor,
    HTMLCurrentStyle2_get_scrollbarDarkShadowColor,
    HTMLCurrentStyle2_get_scrollbarArrowColor,
    HTMLCurrentStyle2_get_scrollbarTrackColor,
    HTMLCurrentStyle2_get_writingMode,
    HTMLCurrentStyle2_get_zoom,
    HTMLCurrentStyle2_get_filter,
    HTMLCurrentStyle2_get_textAlignLast,
    HTMLCurrentStyle2_get_textKashidaSpace,
    HTMLCurrentStyle2_get_isBlock
};

DISPEX_IDISPATCH_IMPL(HTMLCurrentStyle3, IHTMLCurrentStyle3,
                      impl_from_IHTMLCurrentStyle3(iface)->css_style.dispex)

static HRESULT WINAPI HTMLCurrentStyle3_get_textOverflow(IHTMLCurrentStyle3 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle3_get_minHeight(IHTMLCurrentStyle3 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle3_get_wordSpacing(IHTMLCurrentStyle3 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle3_get_whiteSpace(IHTMLCurrentStyle3 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_current_style_property(This, STYLEID_WHITE_SPACE, p);
}

static const IHTMLCurrentStyle3Vtbl HTMLCurrentStyle3Vtbl = {
    HTMLCurrentStyle3_QueryInterface,
    HTMLCurrentStyle3_AddRef,
    HTMLCurrentStyle3_Release,
    HTMLCurrentStyle3_GetTypeInfoCount,
    HTMLCurrentStyle3_GetTypeInfo,
    HTMLCurrentStyle3_GetIDsOfNames,
    HTMLCurrentStyle3_Invoke,
    HTMLCurrentStyle3_get_textOverflow,
    HTMLCurrentStyle3_get_minHeight,
    HTMLCurrentStyle3_get_wordSpacing,
    HTMLCurrentStyle3_get_whiteSpace
};

DISPEX_IDISPATCH_IMPL(HTMLCurrentStyle4, IHTMLCurrentStyle4,
                      impl_from_IHTMLCurrentStyle4(iface)->css_style.dispex)

static HRESULT WINAPI HTMLCurrentStyle4_msInterpolationMode(IHTMLCurrentStyle4 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle4_get_maxHeight(IHTMLCurrentStyle4 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle4_get_minWidth(IHTMLCurrentStyle4 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_current_style_property_var(This, STYLEID_MIN_WIDTH, p);
}

static HRESULT WINAPI HTMLCurrentStyle4_get_maxWidth(IHTMLCurrentStyle4 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCurrentStyle4Vtbl HTMLCurrentStyle4Vtbl = {
    HTMLCurrentStyle4_QueryInterface,
    HTMLCurrentStyle4_AddRef,
    HTMLCurrentStyle4_Release,
    HTMLCurrentStyle4_GetTypeInfoCount,
    HTMLCurrentStyle4_GetTypeInfo,
    HTMLCurrentStyle4_GetIDsOfNames,
    HTMLCurrentStyle4_Invoke,
    HTMLCurrentStyle4_msInterpolationMode,
    HTMLCurrentStyle4_get_maxHeight,
    HTMLCurrentStyle4_get_minWidth,
    HTMLCurrentStyle4_get_maxWidth
};

static inline HTMLCurrentStyle *impl_from_IWineCSSProperties(IWineCSSProperties *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IWineCSSProperties_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLCurrentStyle_CSSProperties, IWineCSSProperties, impl_from_IWineCSSProperties(iface)->css_style.dispex)

static HRESULT WINAPI HTMLCurrentStyle_CSSProperties_getAttribute(IWineCSSProperties *iface, BSTR name, LONG flags, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IWineCSSProperties(iface);
    return HTMLCurrentStyle_getAttribute(&This->IHTMLCurrentStyle_iface, name, flags, p);
}

static HRESULT WINAPI HTMLCurrentStyle_CSSProperties_setAttribute(IWineCSSProperties *iface, BSTR name, VARIANT value, LONG flags)
{
    HTMLCurrentStyle *This = impl_from_IWineCSSProperties(iface);
    FIXME("(%p)->(%s %s %08lx)\n", This, debugstr_w(name), debugstr_variant(&value), flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_CSSProperties_removeAttribute(IWineCSSProperties *iface, BSTR name, LONG flags, VARIANT_BOOL *p)
{
    HTMLCurrentStyle *This = impl_from_IWineCSSProperties(iface);
    FIXME("(%p)->(%s %08lx %p)\n", This, debugstr_w(name), flags, p);
    return E_NOTIMPL;
}

static const IWineCSSPropertiesVtbl HTMLCurrentStyle_CSSPropertiesVtbl = {
    HTMLCurrentStyle_CSSProperties_QueryInterface,
    HTMLCurrentStyle_CSSProperties_AddRef,
    HTMLCurrentStyle_CSSProperties_Release,
    HTMLCurrentStyle_CSSProperties_GetTypeInfoCount,
    HTMLCurrentStyle_CSSProperties_GetTypeInfo,
    HTMLCurrentStyle_CSSProperties_GetIDsOfNames,
    HTMLCurrentStyle_CSSProperties_Invoke,
    HTMLCurrentStyle_CSSProperties_setAttribute,
    HTMLCurrentStyle_CSSProperties_getAttribute,
    HTMLCurrentStyle_CSSProperties_removeAttribute
};

static inline HTMLCurrentStyle *impl_from_DispatchEx(DispatchEx *dispex)
{
    return CONTAINING_RECORD(dispex, HTMLCurrentStyle, css_style.dispex);
}

static void *HTMLCurrentStyle_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLCurrentStyle *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLCurrentStyle, riid))
        return &This->IHTMLCurrentStyle_iface;
    if(IsEqualGUID(&IID_IHTMLCurrentStyle2, riid))
        return &This->IHTMLCurrentStyle2_iface;
    if(IsEqualGUID(&IID_IHTMLCurrentStyle3, riid))
        return &This->IHTMLCurrentStyle3_iface;
    if(IsEqualGUID(&IID_IHTMLCurrentStyle4, riid))
        return &This->IHTMLCurrentStyle4_iface;
    if(IsEqualGUID(&IID_IWineCSSProperties, riid))
        return &This->IWineCSSProperties_iface;
    return CSSStyle_query_interface(&This->css_style.dispex, riid);
}

static void HTMLCurrentStyle_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLCurrentStyle *This = impl_from_DispatchEx(dispex);
    CSSStyle_traverse(&This->css_style.dispex, cb);

    if(This->elem)
        note_cc_edge((nsISupports*)&This->elem->node.IHTMLDOMNode_iface, "elem", cb);
}

static void HTMLCurrentStyle_unlink(DispatchEx *dispex)
{
    HTMLCurrentStyle *This = impl_from_DispatchEx(dispex);
    CSSStyle_unlink(&This->css_style.dispex);

    if(This->elem) {
        HTMLElement *elem = This->elem;
        This->elem = NULL;
        IHTMLDOMNode_Release(&elem->node.IHTMLDOMNode_iface);
    }
}

static void MSCurrentStyleCSSProperties_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t currentstyle_ie11_hooks[] = {
        {DISPID_IHTMLCURRENTSTYLE_BEHAVIOR},
        {DISPID_UNKNOWN}
    };
    MSCSSProperties_init_dispex_info(info, mode);
    dispex_info_add_interface(info, IHTMLCurrentStyle_tid, mode >= COMPAT_MODE_IE11 ? currentstyle_ie11_hooks : NULL);
}

static const dispex_static_data_vtbl_t MSCurrentStyleCSSProperties_dispex_vtbl = {
    CSSSTYLE_DISPEX_VTBL_ENTRIES,
    .query_interface   = HTMLCurrentStyle_query_interface,
    .traverse          = HTMLCurrentStyle_traverse,
    .unlink            = HTMLCurrentStyle_unlink
};

static const tid_t MSCurrentStyleCSSProperties_iface_tids[] = {
    IHTMLCurrentStyle2_tid,
    IHTMLCurrentStyle3_tid,
    IHTMLCurrentStyle4_tid,
    0
};
dispex_static_data_t MSCurrentStyleCSSProperties_dispex = {
    .id           = PROT_MSCurrentStyleCSSProperties,
    .prototype_id = PROT_MSCSSProperties,
    .vtbl         = &MSCurrentStyleCSSProperties_dispex_vtbl,
    .disp_tid     = DispHTMLCurrentStyle_tid,
    .iface_tids   = MSCurrentStyleCSSProperties_iface_tids,
    .init_info    = MSCurrentStyleCSSProperties_init_dispex_info,
};

HRESULT HTMLCurrentStyle_Create(HTMLElement *elem, IHTMLCurrentStyle **p)
{
    nsIDOMCSSStyleDeclaration *nsstyle;
    mozIDOMWindowProxy *nsview;
    nsIDOMWindow *nswindow;
    nsAString nsempty_str;
    HTMLCurrentStyle *ret;
    nsresult nsres;

    if(!elem->node.doc->dom_document)  {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMDocument_GetDefaultView(elem->node.doc->dom_document, &nsview);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultView failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = mozIDOMWindowProxy_QueryInterface(nsview, &IID_nsIDOMWindow, (void**)&nswindow);
    mozIDOMWindowProxy_Release(nsview);
    assert(nsres == NS_OK);

    nsAString_Init(&nsempty_str, NULL);
    nsres = nsIDOMWindow_GetComputedStyle(nswindow, elem->dom_element, &nsempty_str, &nsstyle);
    nsAString_Finish(&nsempty_str);
    nsIDOMWindow_Release(nswindow);
    if(NS_FAILED(nsres)) {
        ERR("GetComputedStyle failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nsstyle) {
        ERR("GetComputedStyle returned NULL nsstyle\n");
        return E_FAIL;
    }

    ret = calloc(1, sizeof(HTMLCurrentStyle));
    if(!ret) {
        nsIDOMCSSStyleDeclaration_Release(nsstyle);
        return E_OUTOFMEMORY;
    }

    ret->IHTMLCurrentStyle_iface.lpVtbl  = &HTMLCurrentStyleVtbl;
    ret->IHTMLCurrentStyle2_iface.lpVtbl = &HTMLCurrentStyle2Vtbl;
    ret->IHTMLCurrentStyle3_iface.lpVtbl = &HTMLCurrentStyle3Vtbl;
    ret->IHTMLCurrentStyle4_iface.lpVtbl = &HTMLCurrentStyle4Vtbl;
    ret->IWineCSSProperties_iface.lpVtbl = &HTMLCurrentStyle_CSSPropertiesVtbl;

    init_css_style(&ret->css_style, nsstyle, &MSCurrentStyleCSSProperties_dispex, &elem->node.event_target.dispex);
    nsIDOMCSSStyleDeclaration_Release(nsstyle);

    IHTMLElement_AddRef(&elem->IHTMLElement_iface);
    ret->elem = elem;

    *p = &ret->IHTMLCurrentStyle_iface;
    return S_OK;
}
