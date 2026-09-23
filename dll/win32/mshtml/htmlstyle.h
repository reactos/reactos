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

typedef struct CSSStyle CSSStyle;

struct CSSStyle {
    DispatchEx dispex;
    IHTMLCSSStyleDeclaration IHTMLCSSStyleDeclaration_iface;
    IHTMLCSSStyleDeclaration2 IHTMLCSSStyleDeclaration2_iface;

    nsIDOMCSSStyleDeclaration *nsstyle;
};

struct HTMLStyle {
    CSSStyle css_style;
    IHTMLStyle  IHTMLStyle_iface;
    IHTMLStyle2 IHTMLStyle2_iface;
    IHTMLStyle3 IHTMLStyle3_iface;
    IHTMLStyle4 IHTMLStyle4_iface;
    IHTMLStyle5 IHTMLStyle5_iface;
    IHTMLStyle6 IHTMLStyle6_iface;
    IWineCSSProperties IWineCSSProperties_iface;

    HTMLElement *elem;
};

/* NOTE: Make sure to keep in sync with style_tbl in htmlstyle.c */
typedef enum {
    STYLEID_MSTRANSFORM,
    STYLEID_MSTRANSITION,
    STYLEID_ANIMATION,
    STYLEID_ANIMATION_NAME,
    STYLEID_BACKGROUND,
    STYLEID_BACKGROUND_ATTACHMENT,
    STYLEID_BACKGROUND_CLIP,
    STYLEID_BACKGROUND_COLOR,
    STYLEID_BACKGROUND_IMAGE,
    STYLEID_BACKGROUND_POSITION,
    STYLEID_BACKGROUND_POSITION_X,
    STYLEID_BACKGROUND_POSITION_Y,
    STYLEID_BACKGROUND_REPEAT,
    STYLEID_BACKGROUND_SIZE,
    STYLEID_BORDER,
    STYLEID_BORDER_BOTTOM,
    STYLEID_BORDER_BOTTOM_COLOR,
    STYLEID_BORDER_BOTTOM_STYLE,
    STYLEID_BORDER_BOTTOM_WIDTH,
    STYLEID_BORDER_COLLAPSE,
    STYLEID_BORDER_COLOR,
    STYLEID_BORDER_LEFT,
    STYLEID_BORDER_LEFT_COLOR,
    STYLEID_BORDER_LEFT_STYLE,
    STYLEID_BORDER_LEFT_WIDTH,
    STYLEID_BORDER_RIGHT,
    STYLEID_BORDER_RIGHT_COLOR,
    STYLEID_BORDER_RIGHT_STYLE,
    STYLEID_BORDER_RIGHT_WIDTH,
    STYLEID_BORDER_SPACING,
    STYLEID_BORDER_STYLE,
    STYLEID_BORDER_TOP,
    STYLEID_BORDER_TOP_COLOR,
    STYLEID_BORDER_TOP_STYLE,
    STYLEID_BORDER_TOP_WIDTH,
    STYLEID_BORDER_WIDTH,
    STYLEID_BOTTOM,
    STYLEID_BOX_SIZING,
    STYLEID_CLEAR,
    STYLEID_CLIP,
    STYLEID_COLOR,
    STYLEID_COLUMN_COUNT,
    STYLEID_COLUMN_FILL,
    STYLEID_COLUMN_GAP,
    STYLEID_COLUMN_RULE,
    STYLEID_COLUMN_RULE_COLOR,
    STYLEID_COLUMN_RULE_STYLE,
    STYLEID_COLUMN_RULE_WIDTH,
    STYLEID_COLUMN_SPAN,
    STYLEID_COLUMN_WIDTH,
    STYLEID_CURSOR,
    STYLEID_DIRECTION,
    STYLEID_DISPLAY,
    STYLEID_FILTER,
    STYLEID_FLOAT,
    STYLEID_FONT_FAMILY,
    STYLEID_FONT_SIZE,
    STYLEID_FONT_STYLE,
    STYLEID_FONT_VARIANT,
    STYLEID_FONT_WEIGHT,
    STYLEID_HEIGHT,
    STYLEID_LEFT,
    STYLEID_LETTER_SPACING,
    STYLEID_LINE_HEIGHT,
    STYLEID_LIST_STYLE,
    STYLEID_LISTSTYLEPOSITION,
    STYLEID_LISTSTYLETYPE,
    STYLEID_MARGIN,
    STYLEID_MARGIN_BOTTOM,
    STYLEID_MARGIN_LEFT,
    STYLEID_MARGIN_RIGHT,
    STYLEID_MARGIN_TOP,
    STYLEID_MAX_HEIGHT,
    STYLEID_MAX_WIDTH,
    STYLEID_MIN_HEIGHT,
    STYLEID_MIN_WIDTH,
    STYLEID_OPACITY,
    STYLEID_OUTLINE,
    STYLEID_OVERFLOW,
    STYLEID_OVERFLOW_X,
    STYLEID_OVERFLOW_Y,
    STYLEID_PADDING,
    STYLEID_PADDING_BOTTOM,
    STYLEID_PADDING_LEFT,
    STYLEID_PADDING_RIGHT,
    STYLEID_PADDING_TOP,
    STYLEID_PAGE_BREAK_AFTER,
    STYLEID_PAGE_BREAK_BEFORE,
    STYLEID_PERSPECTIVE,
    STYLEID_POSITION,
    STYLEID_RIGHT,
    STYLEID_TABLE_LAYOUT,
    STYLEID_TEXT_ALIGN,
    STYLEID_TEXT_DECORATION,
    STYLEID_TEXT_INDENT,
    STYLEID_TEXT_TRANSFORM,
    STYLEID_TOP,
    STYLEID_TRANSFORM,
    STYLEID_TRANSITION,
    STYLEID_VERTICAL_ALIGN,
    STYLEID_VISIBILITY,
    STYLEID_WHITE_SPACE,
    STYLEID_WIDTH,
    STYLEID_WORD_SPACING,
    STYLEID_WORD_WRAP,
    STYLEID_Z_INDEX,
    STYLEID_MAX_VALUE
} styleid_t;

HRESULT HTMLStyle_Create(HTMLElement*,HTMLStyle**);
HRESULT create_computed_style(nsIDOMCSSStyleDeclaration*,DispatchEx*,IHTMLCSSStyleDeclaration**);
void init_css_style(CSSStyle*,nsIDOMCSSStyleDeclaration*,dispex_static_data_t*,DispatchEx*);

void *CSSStyle_query_interface(DispatchEx*,REFIID);
void CSSStyle_traverse(DispatchEx*,nsCycleCollectionTraversalCallback*);
void CSSStyle_unlink(DispatchEx*);
void CSSStyle_destructor(DispatchEx*);
HRESULT CSSStyle_get_dispid(DispatchEx*,const WCHAR*,DWORD,DISPID*);
void MSCSSProperties_init_dispex_info(dispex_data_t *info, compat_mode_t mode);

HRESULT get_style_property(CSSStyle*,styleid_t,BSTR*);
HRESULT get_style_property_var(CSSStyle*,styleid_t,VARIANT*);

HRESULT get_elem_style(HTMLElement*,styleid_t,BSTR*);
HRESULT set_elem_style(HTMLElement*,styleid_t,const WCHAR*);

#define CSSSTYLE_DISPEX_VTBL_ENTRIES           \
    .destructor        = CSSStyle_destructor,  \
    .get_dispid        = CSSStyle_get_dispid
