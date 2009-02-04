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

    LONG ref;

    nsIDOMCSSStyleDeclaration *nsstyle;
};

#define HTMLSTYLE(x)     ((IHTMLStyle*)                   &(x)->lpHTMLStyleVtbl)
#define HTMLSTYLE2(x)    ((IHTMLStyle2*)                  &(x)->lpHTMLStyle2Vtbl)

/* NOTE: Make sure to keep in sync with style_tbl in htmlstyle.c */
typedef enum {
    STYLEID_BACKGROUND,
    STYLEID_BACKGROUND_COLOR,
    STYLEID_BACKGROUND_IMAGE,
    STYLEID_BORDER,
    STYLEID_BORDER_BOTTOM_STYLE,
    STYLEID_BORDER_LEFT,
    STYLEID_BORDER_LEFT_STYLE,
    STYLEID_BORDER_RIGHT_STYLE,
    STYLEID_BORDER_TOP_STYLE,
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

HRESULT get_nsstyle_attr(nsIDOMCSSStyleDeclaration*,styleid_t,BSTR*);
HRESULT set_nsstyle_attr(nsIDOMCSSStyleDeclaration*,styleid_t,LPCWSTR,DWORD);
