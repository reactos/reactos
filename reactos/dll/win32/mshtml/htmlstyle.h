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

struct HTMLStyle {
    DispatchEx dispex;
    const IHTMLStyleVtbl    *lpHTMLStyleVtbl;
    const IHTMLStyle2Vtbl   *lpHTMLStyle2Vtbl;
    const IHTMLStyle3Vtbl   *lpHTMLStyle3Vtbl;
    const IHTMLStyle4Vtbl   *lpHTMLStyle4Vtbl;

    LONG ref;

    nsIDOMCSSStyleDeclaration *nsstyle;
};

#define HTMLSTYLE(x)     ((IHTMLStyle*)                   &(x)->lpHTMLStyleVtbl)
#define HTMLSTYLE2(x)    ((IHTMLStyle2*)                  &(x)->lpHTMLStyle2Vtbl)
#define HTMLSTYLE3(x)    ((IHTMLStyle3*)                  &(x)->lpHTMLStyle3Vtbl)
#define HTMLSTYLE4(x)    ((IHTMLStyle4*)                  &(x)->lpHTMLStyle4Vtbl)

/* NOTE: Make sure to keep in sync with style_tbl in htmlstyle.c */
typedef enum {
    STYLEID_BACKGROUND,
    STYLEID_BACKGROUND_COLOR,
    STYLEID_BACKGROUND_IMAGE,
    STYLEID_BACKGROUND_POSITION_X,
    STYLEID_BACKGROUND_POSITION_Y,
    STYLEID_BACKGROUND_REPEAT,
    STYLEID_BORDER,
    STYLEID_BORDER_BOTTOM_STYLE,
    STYLEID_BORDER_BOTTOM_WIDTH,
    STYLEID_BORDER_COLOR,
    STYLEID_BORDER_LEFT,
    STYLEID_BORDER_LEFT_STYLE,
    STYLEID_BORDER_RIGHT_STYLE,
    STYLEID_BORDER_RIGHT_WIDTH,
    STYLEID_BORDER_STYLE,
    STYLEID_BORDER_TOP_STYLE,
    STYLEID_BORDER_TOP_WIDTH,
    STYLEID_BORDER_WIDTH,
    STYLEID_COLOR,
    STYLEID_CURSOR,
    STYLEID_DISPLAY,
    STYLEID_FILTER,
    STYLEID_FONT_FAMILY,
    STYLEID_FONT_SIZE,
    STYLEID_FONT_STYLE,
    STYLEID_FONT_VARIANT,
    STYLEID_FONT_WEIGHT,
    STYLEID_HEIGHT,
    STYLEID_LEFT,
    STYLEID_MARGIN,
    STYLEID_MARGIN_LEFT,
    STYLEID_MARGIN_RIGHT,
    STYLEID_MIN_HEIGHT,
    STYLEID_OVERFLOW,
    STYLEID_PADDING_LEFT,
    STYLEID_POSITION,
    STYLEID_TEXT_ALIGN,
    STYLEID_TEXT_DECORATION,
    STYLEID_TOP,
    STYLEID_VERTICAL_ALIGN,
    STYLEID_VISIBILITY,
    STYLEID_WIDTH,
    STYLEID_Z_INDEX
} styleid_t;

void HTMLStyle2_Init(HTMLStyle*);
void HTMLStyle3_Init(HTMLStyle*);

HRESULT get_nsstyle_attr(nsIDOMCSSStyleDeclaration*,styleid_t,BSTR*);
HRESULT set_nsstyle_attr(nsIDOMCSSStyleDeclaration*,styleid_t,LPCWSTR,DWORD);

HRESULT set_nsstyle_attr_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *value, DWORD flags);
HRESULT get_nsstyle_attr_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *p, DWORD flags);

#define ATTR_FIX_PX      1
#define ATTR_FIX_URL     2
#define ATTR_STR_TO_INT  4
