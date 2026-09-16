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
#include <math.h>

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

static const WCHAR *font_style_values[] = {
    L"italic",
    L"normal",
    L"oblique",
    NULL
};

static const WCHAR *font_variant_values[] = {
    L"small-caps",
    L"normal",
    NULL
};

static const WCHAR *font_weight_values[] = {
    L"100",
    L"200",
    L"300",
    L"400",
    L"500",
    L"600",
    L"700",
    L"800",
    L"900",
    L"bold",
    L"bolder",
    L"lighter",
    L"normal",
    NULL
};

static const WCHAR *background_repeat_values[] = {
    L"no-repeat",
    L"repeat",
    L"repeat-x",
    L"repeat-y",
    NULL
};

static const WCHAR *text_decoration_values[] = {
    L"blink",
    L"line-through",
    L"none",
    L"overline",
    L"underline",
    NULL
};

static const WCHAR *border_style_values[] = {
    L"dashed",
    L"dotted",
    L"double",
    L"groove",
    L"inset",
    L"none",
    L"outset",
    L"ridge",
    L"solid",
    NULL
};

static const WCHAR *overflow_values[] = {
    L"auto",
    L"hidden",
    L"scroll",
    L"visible",
    NULL
};

#define ATTR_FIX_PX         0x0001
#define ATTR_FIX_URL        0x0002
#define ATTR_STR_TO_INT     0x0004
#define ATTR_HEX_INT        0x0008
#define ATTR_REMOVE_COMMA   0x0010
#define ATTR_NO_NULL        0x0020
#define ATTR_COMPAT_IE10    0x0040
#define ATTR_BUILTIN_NAME   0x0080

typedef struct {
    const WCHAR *name;
    DISPID dispid;
    DISPID compat_dispid;
    unsigned flags;
    const WCHAR **allowed_values;
} style_tbl_entry_t;

static const style_tbl_entry_t style_tbl[] = {
    {
        L"-ms-transform",
        DISPID_IHTMLCSSSTYLEDECLARATION_MSTRANSFORM,
        DISPID_UNKNOWN
    },
    {
        L"-ms-transition",
        DISPID_IHTMLCSSSTYLEDECLARATION2_MSTRANSITION,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"animation",
        DISPID_IHTMLCSSSTYLEDECLARATION2_ANIMATION,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10 | ATTR_BUILTIN_NAME,
    },
    {
        L"animation-name",
        DISPID_IHTMLCSSSTYLEDECLARATION2_ANIMATIONNAME,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"background",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUND,
        DISPID_A_BACKGROUND,
        ATTR_BUILTIN_NAME
    },
    {
        L"background-attachment",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDATTACHMENT,
        DISPID_A_BACKGROUNDATTACHMENT
    },
    {
        L"background-clip",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDCLIP,
        DISPID_UNKNOWN
    },
    {
        L"background-color",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDCOLOR,
        DISPID_BACKCOLOR,
        ATTR_HEX_INT
    },
    {
        L"background-image",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDIMAGE,
        DISPID_A_BACKGROUNDIMAGE,
        ATTR_FIX_URL
    },
    {
        L"background-position",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDPOSITION,
        DISPID_A_BACKGROUNDPOSITION
    },
    {
        L"background-position-x",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDPOSITIONX,
        DISPID_A_BACKGROUNDPOSX,
        ATTR_FIX_PX
    },
    {
        L"background-position-y",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDPOSITIONY,
        DISPID_A_BACKGROUNDPOSY,
        ATTR_FIX_PX
    },
    {
        L"background-repeat",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDREPEAT,
        DISPID_A_BACKGROUNDREPEAT,
        0, background_repeat_values
    },
    {
        L"background-size",
        DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDSIZE,
        DISPID_A_IE9_BACKGROUNDSIZE,
    },
    {
        L"border",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDER,
        DISPID_A_BORDER,
        ATTR_BUILTIN_NAME
    },
    {
        L"border-bottom",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERBOTTOM,
        DISPID_A_BORDERBOTTOM,
        ATTR_FIX_PX
    },
    {
        L"border-bottom-color",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERBOTTOMCOLOR,
        DISPID_A_BORDERBOTTOMCOLOR,
        ATTR_HEX_INT
    },
    {
        L"border-bottom-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERBOTTOMSTYLE,
        DISPID_A_BORDERBOTTOMSTYLE,
        0, border_style_values
    },
    {
        L"border-bottom-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERBOTTOMWIDTH,
        DISPID_A_BORDERBOTTOMWIDTH,
        ATTR_FIX_PX
    },
    {
        L"border-collapse",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERCOLLAPSE,
        DISPID_A_BORDERCOLLAPSE
    },
    {
        L"border-color",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERCOLOR,
        DISPID_A_BORDERCOLOR
    },
    {
        L"border-left",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERLEFT,
        DISPID_A_BORDERLEFT,
        ATTR_FIX_PX
    },
    {
        L"border-left-color",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERLEFTCOLOR,
        DISPID_A_BORDERLEFTCOLOR,
        ATTR_HEX_INT
    },
    {
        L"border-left-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERLEFTSTYLE,
        DISPID_A_BORDERLEFTSTYLE,
        0, border_style_values
    },
    {
        L"border-left-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERLEFTWIDTH,
        DISPID_A_BORDERLEFTWIDTH,
        ATTR_FIX_PX
    },
    {
        L"border-right",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERRIGHT,
        DISPID_A_BORDERRIGHT,
        ATTR_FIX_PX
    },
    {
        L"border-right-color",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERRIGHTCOLOR,
        DISPID_A_BORDERRIGHTCOLOR,
        ATTR_HEX_INT
    },
    {
        L"border-right-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERRIGHTSTYLE,
        DISPID_A_BORDERRIGHTSTYLE,
        0, border_style_values
    },
    {
        L"border-right-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERRIGHTWIDTH,
        DISPID_A_BORDERRIGHTWIDTH,
        ATTR_FIX_PX
    },
    {
        L"border-spacing",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERSPACING,
        DISPID_A_BORDERSPACING
    },
    {
        L"border-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERSTYLE,
        DISPID_A_BORDERSTYLE
    },
    {
        L"border-top",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERTOP,
        DISPID_A_BORDERTOP,
        ATTR_FIX_PX
    },
    {
        L"border-top-color",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERTOPCOLOR,
        DISPID_A_BORDERTOPCOLOR,
        ATTR_HEX_INT
    },
    {
        L"border-top-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERTOPSTYLE,
        DISPID_A_BORDERTOPSTYLE,
        0, border_style_values
    },
    {
        L"border-top-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERTOPWIDTH,
        DISPID_A_BORDERTOPWIDTH
    },
    {
        L"border-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_BORDERWIDTH,
        DISPID_A_BORDERWIDTH
    },
    {
        L"bottom",
        DISPID_IHTMLCSSSTYLEDECLARATION_BOTTOM,
        STDPROPID_XOBJ_BOTTOM,
        ATTR_FIX_PX | ATTR_BUILTIN_NAME,
    },
    {
        L"box-sizing",
        DISPID_IHTMLCSSSTYLEDECLARATION_BOXSIZING,
        DISPID_A_BOXSIZING
    },
    {
        L"clear",
        DISPID_IHTMLCSSSTYLEDECLARATION_CLEAR,
        DISPID_A_CLEAR,
        ATTR_BUILTIN_NAME
    },
    {
        L"clip",
        DISPID_IHTMLCSSSTYLEDECLARATION_CLIP,
        DISPID_A_CLIP,
        ATTR_REMOVE_COMMA | ATTR_BUILTIN_NAME
    },
    {
        L"color",
        DISPID_IHTMLCSSSTYLEDECLARATION_COLOR,
        DISPID_A_COLOR,
        ATTR_HEX_INT | ATTR_BUILTIN_NAME
    },
    {
        L"column-count",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNCOUNT,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-fill",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNFILL,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-gap",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNGAP,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-rule",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNRULE,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-rule-color",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNRULECOLOR,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-rule-style",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNRULESTYLE,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-rule-width",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNRULEWIDTH,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-span",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNSPAN,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"column-width",
        DISPID_IHTMLCSSSTYLEDECLARATION2_COLUMNWIDTH,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10
    },
    {
        L"cursor",
        DISPID_IHTMLCSSSTYLEDECLARATION_CURSOR,
        DISPID_A_CURSOR,
        ATTR_BUILTIN_NAME
    },
    {
        L"direction",
        DISPID_IHTMLCSSSTYLEDECLARATION_DIRECTION,
        DISPID_A_DIRECTION,
        ATTR_BUILTIN_NAME
    },
    {
        L"display",
        DISPID_IHTMLCSSSTYLEDECLARATION_DISPLAY,
        DISPID_A_DISPLAY,
        ATTR_BUILTIN_NAME
    },
    {
        L"filter",
        DISPID_IHTMLCSSSTYLEDECLARATION_FILTER,
        DISPID_A_FILTER,
        ATTR_BUILTIN_NAME
    },
    {
        L"float",
        DISPID_IHTMLCSSSTYLEDECLARATION_CSSFLOAT,
        DISPID_A_FLOAT
    },
    {
        L"font-family",
        DISPID_IHTMLCSSSTYLEDECLARATION_FONTFAMILY,
        DISPID_A_FONTFACE
    },
    {
        L"font-size",
        DISPID_IHTMLCSSSTYLEDECLARATION_FONTSIZE,
        DISPID_A_FONTSIZE,
        ATTR_FIX_PX
    },
    {
        L"font-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_FONTSTYLE,
        DISPID_A_FONTSTYLE,
        0, font_style_values
    },
    {
        L"font-variant",
        DISPID_IHTMLCSSSTYLEDECLARATION_FONTVARIANT,
        DISPID_A_FONTVARIANT,
        0, font_variant_values
    },
    {
        L"font-weight",
        DISPID_IHTMLCSSSTYLEDECLARATION_FONTWEIGHT,
        DISPID_A_FONTWEIGHT,
        ATTR_STR_TO_INT, font_weight_values
    },
    {
        L"height",
        DISPID_IHTMLCSSSTYLEDECLARATION_HEIGHT,
        STDPROPID_XOBJ_HEIGHT,
        ATTR_FIX_PX | ATTR_BUILTIN_NAME
    },
    {
        L"left",
        DISPID_IHTMLCSSSTYLEDECLARATION_LEFT,
        STDPROPID_XOBJ_LEFT,
        ATTR_BUILTIN_NAME
    },
    {
        L"letter-spacing",
        DISPID_IHTMLCSSSTYLEDECLARATION_LETTERSPACING,
        DISPID_A_LETTERSPACING
    },
    {
        L"line-height",
        DISPID_IHTMLCSSSTYLEDECLARATION_LINEHEIGHT,
        DISPID_A_LINEHEIGHT
    },
    {
        L"list-style",
        DISPID_IHTMLCSSSTYLEDECLARATION_LISTSTYLE,
        DISPID_A_LISTSTYLE
    },
    {
        L"list-style-position",
        DISPID_IHTMLCSSSTYLEDECLARATION_LISTSTYLEPOSITION,
        DISPID_A_LISTSTYLEPOSITION
    },
    {
        L"list-style-type",
        DISPID_IHTMLCSSSTYLEDECLARATION_LISTSTYLETYPE,
        DISPID_A_LISTSTYLETYPE
    },
    {
        L"margin",
        DISPID_IHTMLCSSSTYLEDECLARATION_MARGIN,
        DISPID_A_MARGIN,
        ATTR_BUILTIN_NAME
    },
    {
        L"margin-bottom",
        DISPID_IHTMLCSSSTYLEDECLARATION_MARGINBOTTOM,
        DISPID_A_MARGINBOTTOM,
        ATTR_FIX_PX
    },
    {
        L"margin-left",
        DISPID_IHTMLCSSSTYLEDECLARATION_MARGINLEFT,
        DISPID_A_MARGINLEFT,
        ATTR_FIX_PX
    },
    {
        L"margin-right",
        DISPID_IHTMLCSSSTYLEDECLARATION_MARGINRIGHT,
        DISPID_A_MARGINRIGHT,
        ATTR_FIX_PX
    },
    {
        L"margin-top",
        DISPID_IHTMLCSSSTYLEDECLARATION_MARGINTOP,
        DISPID_A_MARGINTOP,
        ATTR_FIX_PX
    },
    {
        L"max-height",
        DISPID_IHTMLCSSSTYLEDECLARATION_MAXHEIGHT,
        DISPID_A_MAXHEIGHT,
        ATTR_FIX_PX
    },
    {
        L"max-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_MAXWIDTH,
        DISPID_A_MAXWIDTH,
        ATTR_FIX_PX
    },
    {
        L"min-height",
        DISPID_IHTMLCSSSTYLEDECLARATION_MINHEIGHT,
        DISPID_A_MINHEIGHT
    },
    {
        L"min-width",
        DISPID_IHTMLCSSSTYLEDECLARATION_MINWIDTH,
        DISPID_A_MINWIDTH,
        ATTR_FIX_PX
    },
    {
        L"opacity",
        DISPID_IHTMLCSSSTYLEDECLARATION_OPACITY,
        DISPID_UNKNOWN,
        ATTR_BUILTIN_NAME
    },
    {
        L"outline",
        DISPID_IHTMLCSSSTYLEDECLARATION_OUTLINE,
        DISPID_A_OUTLINE,
        ATTR_NO_NULL | ATTR_BUILTIN_NAME
    },
    {
        L"overflow",
        DISPID_IHTMLCSSSTYLEDECLARATION_OVERFLOW,
        DISPID_A_OVERFLOW,
        ATTR_BUILTIN_NAME,
        overflow_values
    },
    {
        L"overflow-x",
        DISPID_IHTMLCSSSTYLEDECLARATION_OVERFLOWX,
        DISPID_A_OVERFLOWX
    },
    {
        L"overflow-y",
        DISPID_IHTMLCSSSTYLEDECLARATION_OVERFLOWY,
        DISPID_A_OVERFLOWY
    },
    {
        L"padding",
        DISPID_IHTMLCSSSTYLEDECLARATION_PADDING,
        DISPID_A_PADDING,
        ATTR_BUILTIN_NAME
    },
    {
        L"padding-bottom",
        DISPID_IHTMLCSSSTYLEDECLARATION_PADDINGBOTTOM,
        DISPID_A_PADDINGBOTTOM,
        ATTR_FIX_PX
    },
    {
        L"padding-left",
        DISPID_IHTMLCSSSTYLEDECLARATION_PADDINGLEFT,
        DISPID_A_PADDINGLEFT,
        ATTR_FIX_PX
    },
    {
        L"padding-right",
        DISPID_IHTMLCSSSTYLEDECLARATION_PADDINGRIGHT,
        DISPID_A_PADDINGRIGHT,
        ATTR_FIX_PX
    },
    {
        L"padding-top",
        DISPID_IHTMLCSSSTYLEDECLARATION_PADDINGTOP,
        DISPID_A_PADDINGTOP,
        ATTR_FIX_PX
    },
    {
        L"page-break-after",
        DISPID_IHTMLCSSSTYLEDECLARATION_PAGEBREAKAFTER,
        DISPID_A_PAGEBREAKAFTER
    },
    {
        L"page-break-before",
        DISPID_IHTMLCSSSTYLEDECLARATION_PAGEBREAKBEFORE,
        DISPID_A_PAGEBREAKBEFORE
    },
    {
        L"perspective",
        DISPID_IHTMLCSSSTYLEDECLARATION2_PERSPECTIVE,
        DISPID_UNKNOWN,
        ATTR_BUILTIN_NAME
    },
    {
        L"position",
        DISPID_IHTMLCSSSTYLEDECLARATION_POSITION,
        DISPID_A_POSITION,
        ATTR_BUILTIN_NAME
    },
    {
        L"right",
        DISPID_IHTMLCSSSTYLEDECLARATION_RIGHT,
        STDPROPID_XOBJ_RIGHT,
        ATTR_BUILTIN_NAME
    },
    {
        L"table-layout",
        DISPID_IHTMLCSSSTYLEDECLARATION_TABLELAYOUT,
        DISPID_A_TABLELAYOUT
    },
    {
        L"text-align",
        DISPID_IHTMLCSSSTYLEDECLARATION_TEXTALIGN,
        STDPROPID_XOBJ_BLOCKALIGN
    },
    {
        L"text-decoration",
        DISPID_IHTMLCSSSTYLEDECLARATION_TEXTDECORATION,
        DISPID_A_TEXTDECORATION,
        0, text_decoration_values
    },
    {
        L"text-indent",
        DISPID_IHTMLCSSSTYLEDECLARATION_TEXTINDENT,
        DISPID_A_TEXTINDENT,
        ATTR_FIX_PX
    },
    {
        L"text-transform",
        DISPID_IHTMLCSSSTYLEDECLARATION_TEXTTRANSFORM,
        DISPID_A_TEXTTRANSFORM
    },
    {
        L"top",
        DISPID_IHTMLCSSSTYLEDECLARATION_TOP,
        STDPROPID_XOBJ_TOP,
        ATTR_BUILTIN_NAME
    },
    {
        L"transform",
        DISPID_IHTMLCSSSTYLEDECLARATION2_TRANSFORM,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10 | ATTR_BUILTIN_NAME
    },
    {
        L"transition",
        DISPID_IHTMLCSSSTYLEDECLARATION2_TRANSITION,
        DISPID_UNKNOWN,
        ATTR_COMPAT_IE10 | ATTR_BUILTIN_NAME
    },
    {
        L"vertical-align",
        DISPID_IHTMLCSSSTYLEDECLARATION_VERTICALALIGN,
        DISPID_A_VERTICALALIGN,
        ATTR_FIX_PX
    },
    {
        L"visibility",
        DISPID_IHTMLCSSSTYLEDECLARATION_VISIBILITY,
        DISPID_A_VISIBILITY,
        ATTR_BUILTIN_NAME
    },
    {
        L"white-space",
        DISPID_IHTMLCSSSTYLEDECLARATION_WHITESPACE,
        DISPID_A_WHITESPACE
    },
    {
        L"width",
        DISPID_IHTMLCSSSTYLEDECLARATION_WIDTH,
        STDPROPID_XOBJ_WIDTH,
        ATTR_FIX_PX | ATTR_BUILTIN_NAME
    },
    {
        L"word-spacing",
        DISPID_IHTMLCSSSTYLEDECLARATION_WORDSPACING,
        DISPID_A_WORDSPACING
    },
    {
        L"word-wrap",
        DISPID_IHTMLCSSSTYLEDECLARATION_WORDWRAP,
        DISPID_A_WORDWRAP
    },
    {
        L"z-index",
        DISPID_IHTMLCSSSTYLEDECLARATION_ZINDEX,
        DISPID_A_ZINDEX,
        ATTR_STR_TO_INT
    }
};

C_ASSERT(ARRAY_SIZE(style_tbl) == STYLEID_MAX_VALUE);

static const WCHAR *get_style_nsname(const style_tbl_entry_t *style_entry)
{
    if(style_entry->name[0] == '-')
        return style_entry->name + sizeof("-ms-")-1;
    return style_entry->name;
}

static const WCHAR *get_style_prop_nsname(const style_tbl_entry_t *style_entry, const WCHAR *orig_name)
{
    return style_entry ? get_style_nsname(style_entry) : orig_name;
}

static const style_tbl_entry_t *lookup_style_tbl(CSSStyle *style, const WCHAR *name, unsigned exclude_mask)
{
    int c, i, min = 0, max = ARRAY_SIZE(style_tbl)-1;

    while(min <= max) {
        i = (min+max)/2;

        c = wcscmp(style_tbl[i].name, name);
        if(!c) {
            if(style_tbl[i].flags & exclude_mask)
                return NULL;
            if((style_tbl[i].flags & ATTR_COMPAT_IE10) && dispex_compat_mode(&style->dispex) < COMPAT_MODE_IE10)
                return NULL;
            return style_tbl+i;
        }

        if(c > 0)
            max = i-1;
        else
            min = i+1;
    }

    return NULL;
}

static void fix_px_value(nsAString *nsstr)
{
    const WCHAR *val, *ptr;

    nsAString_GetData(nsstr, &val);
    ptr = val;

    while(*ptr) {
        while(*ptr && iswspace(*ptr))
            ptr++;
        if(!*ptr)
            break;

        while(*ptr && is_digit(*ptr))
            ptr++;

        if(!*ptr || iswspace(*ptr)) {
            LPWSTR ret, p;
            int len = lstrlenW(val)+1;

            ret = malloc((len + 2) * sizeof(WCHAR));
            memcpy(ret, val, (ptr-val)*sizeof(WCHAR));
            p = ret + (ptr-val);
            *p++ = 'p';
            *p++ = 'x';
            lstrcpyW(p, ptr);

            TRACE("fixed %s -> %s\n", debugstr_w(val), debugstr_w(ret));

            nsAString_SetData(nsstr, ret);
            free(ret);
            break;
        }

        while(*ptr && !iswspace(*ptr))
            ptr++;
    }
}

static LPWSTR fix_url_value(LPCWSTR val)
{
    WCHAR *ret, *ptr;

    static const WCHAR urlW[] = {'u','r','l','('};

    if(wcsncmp(val, urlW, ARRAY_SIZE(urlW)) || !wcschr(val, '\\'))
        return NULL;

    ret = wcsdup(val);

    for(ptr = ret; *ptr; ptr++) {
        if(*ptr == '\\')
            *ptr = '/';
    }

    return ret;
}

static HRESULT set_nsstyle_property(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, const nsAString *value)
{
    nsAString str_name, str_empty;
    nsresult nsres;

    nsAString_InitDepend(&str_name, get_style_nsname(&style_tbl[sid]));
    nsAString_InitDepend(&str_empty, L"");
    nsres = nsIDOMCSSStyleDeclaration_SetProperty(nsstyle, &str_name, value, &str_empty);
    nsAString_Finish(&str_name);
    nsAString_Finish(&str_empty);
    if(NS_FAILED(nsres))
        WARN("SetProperty failed: %08lx\n", nsres);
    return map_nsresult(nsres);
}

static HRESULT var_to_styleval(CSSStyle *style, VARIANT *v, const style_tbl_entry_t *entry, nsAString *nsstr)
{
    HRESULT hres;
    unsigned flags = entry && dispex_compat_mode(&style->dispex) < COMPAT_MODE_IE9
        ? entry->flags : 0;

    hres = variant_to_nsstr(v, !!(flags & ATTR_HEX_INT), nsstr);
    if(SUCCEEDED(hres) && (flags & ATTR_FIX_PX))
        fix_px_value(nsstr);
    return hres;
}

static inline HRESULT set_style_property(CSSStyle *style, styleid_t sid, const WCHAR *value)
{
    nsAString value_str;
    unsigned flags = 0;
    WCHAR *val = NULL;
    HRESULT hres;

    if(value && *value && dispex_compat_mode(&style->dispex) < COMPAT_MODE_IE9) {
        flags = style_tbl[sid].flags;

        if(style_tbl[sid].allowed_values) {
            const WCHAR **iter;
            for(iter = style_tbl[sid].allowed_values; *iter; iter++) {
                if(!wcsicmp(*iter, value))
                    break;
            }
            if(!*iter) {
                WARN("invalid value %s\n", debugstr_w(value));
                nsAString_InitDepend(&value_str, L"");
                set_nsstyle_property(style->nsstyle, sid, &value_str);
                nsAString_Finish(&value_str);
                return E_INVALIDARG;
            }
        }

        if(flags & ATTR_FIX_URL)
            val = fix_url_value(value);
    }

    nsAString_InitDepend(&value_str, val ? val : value);
    if(flags & ATTR_FIX_PX)
        fix_px_value(&value_str);
    hres = set_nsstyle_property(style->nsstyle, sid, &value_str);
    nsAString_Finish(&value_str);
    free(val);
    return hres;
}

static HRESULT set_style_property_var(CSSStyle *style, styleid_t sid, VARIANT *value)
{
    nsAString val;
    HRESULT hres;

    hres = var_to_styleval(style, value, &style_tbl[sid], &val);
    if(FAILED(hres))
        return hres;

    hres = set_nsstyle_property(style->nsstyle, sid, &val);
    nsAString_Finish(&val);
    return hres;
}

static HRESULT get_nsstyle_attr_nsval(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, nsAString *value)
{
    nsAString str_name;
    nsresult nsres;

    nsAString_InitDepend(&str_name, get_style_nsname(&style_tbl[sid]));
    nsres = nsIDOMCSSStyleDeclaration_GetPropertyValue(nsstyle, &str_name, value);
    nsAString_Finish(&str_name);
    if(NS_FAILED(nsres))
        WARN("GetPropertyValue failed: %08lx\n", nsres);
    return map_nsresult(nsres);
}

static HRESULT nsstyle_to_bstr(const WCHAR *val, DWORD flags, BSTR *p)
{
    BSTR ret;
    DWORD len;

    if(!*val) {
        *p = (flags & ATTR_NO_NULL) ? SysAllocStringLen(NULL, 0) : NULL;
        return S_OK;
    }

    ret = SysAllocString(val);
    if(!ret)
        return E_OUTOFMEMORY;

    len = SysStringLen(ret);

    if(flags & ATTR_REMOVE_COMMA) {
        DWORD new_len = len;
        WCHAR *ptr, *ptr2;

        for(ptr = ret; (ptr = wcschr(ptr, ',')); ptr++)
            new_len--;

        if(new_len != len) {
            BSTR new_ret;

            new_ret = SysAllocStringLen(NULL, new_len);
            if(!new_ret) {
                SysFreeString(ret);
                return E_OUTOFMEMORY;
            }

            for(ptr2 = new_ret, ptr = ret; *ptr; ptr++) {
                if(*ptr != ',')
                    *ptr2++ = *ptr;
            }

            SysFreeString(ret);
            ret = new_ret;
        }
    }

    *p = ret;
    return S_OK;
}

static HRESULT get_nsstyle_property(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, compat_mode_t compat_mode, BSTR *p)
{
    nsAString str_value;
    const PRUnichar *value;
    HRESULT hres;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    hres = nsstyle_to_bstr(value, compat_mode < COMPAT_MODE_IE9 ? style_tbl[sid].flags : 0, p);
    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_w(*p));
    return hres;
}

static HRESULT get_nsstyle_property_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, compat_mode_t compat_mode, VARIANT *p)
{
    unsigned flags = style_tbl[sid].flags;
    nsAString str_value;
    const PRUnichar *value;
    BOOL set = FALSE;
    HRESULT hres = S_OK;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);

    if((flags & ATTR_STR_TO_INT) && (*value || compat_mode < COMPAT_MODE_IE9)) {
        const PRUnichar *ptr = value;
        BOOL neg = FALSE;
        INT i = 0;

        if(*ptr == '-') {
            neg = TRUE;
            ptr++;
        }

        while(is_digit(*ptr))
            i = i*10 + (*ptr++ - '0');

        if(!*ptr) {
            V_VT(p) = VT_I4;
            V_I4(p) = neg ? -i : i;
            set = TRUE;
        }
    }

    if(!set) {
        BSTR str;

        hres = nsstyle_to_bstr(value, compat_mode < COMPAT_MODE_IE9 ? flags : 0, &str);
        if(SUCCEEDED(hres)) {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = str;
        }
    }

    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_variant(p));
    return S_OK;
}

HRESULT get_style_property(CSSStyle *style, styleid_t sid, BSTR *p)
{
    return get_nsstyle_property(style->nsstyle, sid, dispex_compat_mode(&style->dispex), p);
}

HRESULT get_style_property_var(CSSStyle *style, styleid_t sid, VARIANT *v)
{
    return get_nsstyle_property_var(style->nsstyle, sid, dispex_compat_mode(&style->dispex), v);
}

static HRESULT check_style_attr_value(HTMLStyle *This, styleid_t sid, LPCWSTR exval, VARIANT_BOOL *p)
{
    nsAString str_value;
    const PRUnichar *value;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(This->css_style.nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    *p = variant_bool(!wcscmp(value, exval));
    nsAString_Finish(&str_value);

    TRACE("%s -> %x\n", debugstr_w(style_tbl[sid].name), *p);
    return S_OK;
}

static inline HRESULT set_style_pos(HTMLStyle *This, styleid_t sid, float value)
{
    WCHAR szValue[25];

    value = floor(value);

    swprintf(szValue, ARRAY_SIZE(szValue), L"%.0fpx", value);

    return set_style_property(&This->css_style, sid, szValue);
}

static HRESULT set_style_pxattr(HTMLStyle *style, styleid_t sid, LONG value)
{
    WCHAR value_str[16];

    swprintf(value_str, ARRAY_SIZE(value_str), L"%dpx", value);

    return set_style_property(&style->css_style, sid, value_str);
}

static HRESULT get_nsstyle_pos(HTMLStyle *This, styleid_t sid, float *p)
{
    nsAString str_value;
    HRESULT hres;

    TRACE("%p %d %p\n", This, sid, p);

    *p = 0.0f;

    nsAString_Init(&str_value, NULL);

    hres = get_nsstyle_attr_nsval(This->css_style.nsstyle, sid, &str_value);
    if(hres == S_OK)
    {
        WCHAR *ptr;
        const PRUnichar *value;

        nsAString_GetData(&str_value, &value);
        if(value)
        {
            *p = wcstol(value, &ptr, 10);

            if(*ptr && wcscmp(ptr, L"px"))
            {
                nsAString_Finish(&str_value);
                FIXME("only px values are currently supported\n");
                hres = E_FAIL;
            }
        }
    }

    TRACE("ret %f\n", *p);

    nsAString_Finish(&str_value);
    return hres;
}

static HRESULT get_nsstyle_pixel_val(HTMLStyle *This, styleid_t sid, LONG *p)
{
    nsAString str_value;
    HRESULT hres;

    if(!p)
        return E_POINTER;

    nsAString_Init(&str_value, NULL);

    hres = get_nsstyle_attr_nsval(This->css_style.nsstyle, sid, &str_value);
    if(hres == S_OK) {
        WCHAR *ptr = NULL;
        const PRUnichar *value;

        nsAString_GetData(&str_value, &value);
        if(value) {
            *p = wcstol(value, &ptr, 10);

            if(*ptr == '.') {
                /* Skip all digits. We have tests showing that we should not round the value. */
                while(is_digit(*++ptr));
            }
        }

        if(!ptr || (*ptr && wcscmp(ptr, L"px")))
            *p = 0;
    }

    nsAString_Finish(&str_value);
    return hres;
}

static BOOL is_valid_border_style(BSTR v)
{
    return !v || wcsicmp(v, L"none")   == 0 || wcsicmp(v, L"dotted") == 0 ||
        wcsicmp(v, L"dashed") == 0 || wcsicmp(v, L"solid")  == 0 ||
        wcsicmp(v, L"double") == 0 || wcsicmp(v, L"groove") == 0 ||
        wcsicmp(v, L"ridge")  == 0 || wcsicmp(v, L"inset")  == 0 ||
        wcsicmp(v, L"outset") == 0;
}

static inline HTMLStyle *impl_from_IHTMLStyle(IHTMLStyle *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle, IHTMLStyle, impl_from_IHTMLStyle(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle_put_fontFamily(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_FONT_FAMILY, v);
}

static HRESULT WINAPI HTMLStyle_get_fontFamily(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLStyle_put_fontStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_FONT_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_fontStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_fontVariant(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_FONT_VARIANT, v);
}

static HRESULT WINAPI HTMLStyle_get_fontVariant(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_property(&This->css_style, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontWeight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_FONT_WEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_fontWeight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_FONT_WEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontSize(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_FONT_SIZE, &v);
}

static HRESULT WINAPI HTMLStyle_get_fontSize(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_FONT_SIZE, p);
}

static HRESULT WINAPI HTMLStyle_put_font(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_font(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_color(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_color(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_background(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BACKGROUND, v);
}

static HRESULT WINAPI HTMLStyle_get_background(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BACKGROUND, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BACKGROUND_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BACKGROUND_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BACKGROUND_IMAGE, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundRepeat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BACKGROUND_REPEAT , v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundRepeat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BACKGROUND_REPEAT, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundAttachment(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BACKGROUND_ATTACHMENT, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundAttachment(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BACKGROUND_ATTACHMENT, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BACKGROUND_POSITION, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundPosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BACKGROUND_POSITION, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionX(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return IHTMLCSSStyleDeclaration_put_backgroundPositionX(&This->css_style.IHTMLCSSStyleDeclaration_iface, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionX(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLCSSStyleDeclaration_get_backgroundPositionX(&This->css_style.IHTMLCSSStyleDeclaration_iface, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionY(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return IHTMLCSSStyleDeclaration_put_backgroundPositionY(&This->css_style.IHTMLCSSStyleDeclaration_iface, v);
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionY(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLCSSStyleDeclaration_get_backgroundPositionY(&This->css_style.IHTMLCSSStyleDeclaration_iface, p);
}

static HRESULT WINAPI HTMLStyle_put_wordSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_WORD_SPACING, &v);
}

static HRESULT WINAPI HTMLStyle_get_wordSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(&This->css_style, STYLEID_WORD_SPACING, p);
}

static HRESULT WINAPI HTMLStyle_put_letterSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_LETTER_SPACING, &v);
}

static HRESULT WINAPI HTMLStyle_get_letterSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(&This->css_style, STYLEID_LETTER_SPACING, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecoration(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_TEXT_DECORATION , v);
}

static HRESULT WINAPI HTMLStyle_get_textDecoration(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(&This->css_style, STYLEID_TEXT_DECORATION, v ? L"none" : L"");
}

static HRESULT WINAPI HTMLStyle_get_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, L"none", p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(&This->css_style, STYLEID_TEXT_DECORATION, v ? L"underline" : L"");
}

static HRESULT WINAPI HTMLStyle_get_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, L"underline", p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(&This->css_style, STYLEID_TEXT_DECORATION, v ? L"overline" : L"");
}

static HRESULT WINAPI HTMLStyle_get_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, L"overline", p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(&This->css_style, STYLEID_TEXT_DECORATION, v ? L"line-through" : L"");
}

static HRESULT WINAPI HTMLStyle_get_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, L"line-through", p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_property(&This->css_style, STYLEID_TEXT_DECORATION, v ? L"blink" : L"");
}

static HRESULT WINAPI HTMLStyle_get_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, L"blink", p);
}

static HRESULT WINAPI HTMLStyle_put_verticalAlign(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_VERTICAL_ALIGN, &v);
}

static HRESULT WINAPI HTMLStyle_get_verticalAlign(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_VERTICAL_ALIGN, p);
}

static HRESULT WINAPI HTMLStyle_put_textTransform(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_TEXT_TRANSFORM, v);
}

static HRESULT WINAPI HTMLStyle_get_textTransform(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_TEXT_TRANSFORM, p);
}

static HRESULT WINAPI HTMLStyle_put_textAlign(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_TEXT_ALIGN, v);
}

static HRESULT WINAPI HTMLStyle_get_textAlign(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLStyle_put_textIndent(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_TEXT_INDENT, &v);
}

static HRESULT WINAPI HTMLStyle_get_textIndent(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_TEXT_INDENT, p);
}

static HRESULT WINAPI HTMLStyle_put_lineHeight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_LINE_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_lineHeight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_LINE_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_marginTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MARGIN_TOP, &v);
}

static HRESULT WINAPI HTMLStyle_get_marginTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_MARGIN_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_marginRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MARGIN_RIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_marginRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(&This->css_style, STYLEID_MARGIN_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_marginBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MARGIN_BOTTOM, &v);
}

static HRESULT WINAPI HTMLStyle_get_marginBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_MARGIN_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_marginLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MARGIN_LEFT, &v);
}

static HRESULT WINAPI HTMLStyle_put_margin(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_MARGIN, v);
}

static HRESULT WINAPI HTMLStyle_get_margin(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_MARGIN, p);
}

static HRESULT WINAPI HTMLStyle_get_marginLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(&This->css_style, STYLEID_MARGIN_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_PADDING_TOP, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_PADDING_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_PADDING_RIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_PADDING_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_PADDING_BOTTOM, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_PADDING_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_paddingLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_PADDING_LEFT, &v);
}

static HRESULT WINAPI HTMLStyle_get_paddingLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_PADDING_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_padding(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_PADDING, v);
}

static HRESULT WINAPI HTMLStyle_get_padding(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_PADDING, p);
}

static HRESULT WINAPI HTMLStyle_put_border(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BORDER, v);
}

static HRESULT WINAPI HTMLStyle_get_border(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BORDER, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTop(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_borderTop(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_RIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_borderRight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottom(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_BOTTOM, v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottom(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeft(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BORDER_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeft(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BORDER_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_borderColor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BORDER_COLOR, v);
}

static HRESULT WINAPI HTMLStyle_get_borderColor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BORDER_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_TOP_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderTopColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BORDER_TOP_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_RIGHT_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderRightColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BORDER_RIGHT_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_BOTTOM_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BORDER_BOTTOM_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_LEFT_COLOR, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BORDER_LEFT_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderWidth(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_borderWidth(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_TOP_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderTopWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BORDER_TOP_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_RIGHT_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderRightWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BORDER_RIGHT_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_BOTTOM_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(&This->css_style, STYLEID_BORDER_BOTTOM_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BORDER_LEFT_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(&This->css_style, STYLEID_BORDER_LEFT_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres = S_OK;
    BSTR pstyle;
    int i=0;
    int last = 0;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    while(v[i] && hres == S_OK)
    {
        if(v[i] == (WCHAR)' ')
        {
            pstyle = SysAllocStringLen(&v[last], (i-last));
            if( !(is_valid_border_style(pstyle) || wcsicmp(L"window-inset", pstyle) == 0))
            {
                TRACE("1. Invalid style (%s)\n", debugstr_w(pstyle));
                hres = E_INVALIDARG;
            }
            SysFreeString(pstyle);
            last = i+1;
        }
        i++;
    }

    if(hres == S_OK)
    {
        pstyle = SysAllocStringLen(&v[last], i-last);
        if( !(is_valid_border_style(pstyle) || wcsicmp(L"window-inset", pstyle) == 0))
        {
            TRACE("2. Invalid style (%s)\n", debugstr_w(pstyle));
            hres = E_INVALIDARG;
        }
        SysFreeString(pstyle);
    }

    if(hres == S_OK)
        hres = set_style_property(&This->css_style, STYLEID_BORDER_STYLE, v);

    return hres;
}

static HRESULT WINAPI HTMLStyle_get_borderStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_TOP_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderTopStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_RIGHT_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderRightStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_BOTTOM_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(&This->css_style, STYLEID_BORDER_LEFT_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(&This->css_style, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_width(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle_get_width(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_height(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle_get_height(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_styleFloat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_FLOAT, v);
}

static HRESULT WINAPI HTMLStyle_get_styleFloat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_FLOAT, p);
}

static HRESULT WINAPI HTMLStyle_put_clear(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_CLEAR, v);
}

static HRESULT WINAPI HTMLStyle_get_clear(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_CLEAR, p);
}

static HRESULT WINAPI HTMLStyle_put_display(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_DISPLAY, v);
}

static HRESULT WINAPI HTMLStyle_get_display(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLStyle_put_visibility(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_VISIBILITY, v);
}

static HRESULT WINAPI HTMLStyle_get_visibility(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_VISIBILITY, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleType(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_LISTSTYLETYPE, v);
}

static HRESULT WINAPI HTMLStyle_get_listStyleType(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_LISTSTYLETYPE, p);
}

static HRESULT WINAPI HTMLStyle_put_listStylePosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_LISTSTYLEPOSITION, v);
}

static HRESULT WINAPI HTMLStyle_get_listStylePosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_LISTSTYLEPOSITION, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStyleImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_listStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_LIST_STYLE, v);
}

static HRESULT WINAPI HTMLStyle_get_listStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_LIST_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_whiteSpace(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_WHITE_SPACE, v);
}

static HRESULT WINAPI HTMLStyle_get_whiteSpace(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_WHITE_SPACE, p);
}

static HRESULT WINAPI HTMLStyle_put_top(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_TOP, &v);
}

static HRESULT WINAPI HTMLStyle_get_top(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_left(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_LEFT, &v);
}

static HRESULT WINAPI HTMLStyle_get_left(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_get_position(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyle2_get_position(&This->IHTMLStyle2_iface, p);
}

static HRESULT WINAPI HTMLStyle_put_zIndex(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_Z_INDEX, &v);
}

static HRESULT WINAPI HTMLStyle_get_zIndex(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_Z_INDEX, p);
}

static HRESULT WINAPI HTMLStyle_put_overflow(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_OVERFLOW, v);
}


static HRESULT WINAPI HTMLStyle_get_overflow(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_property(&This->css_style, STYLEID_OVERFLOW, p);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakBefore(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_PAGE_BREAK_BEFORE, v);
}

static HRESULT WINAPI HTMLStyle_get_pageBreakBefore(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_PAGE_BREAK_BEFORE, p);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakAfter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_PAGE_BREAK_AFTER, v);
}

static HRESULT WINAPI HTMLStyle_get_pageBreakAfter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_PAGE_BREAK_AFTER, p);
}

static HRESULT WINAPI HTMLStyle_put_cssText(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLCSSStyleDeclaration_put_cssText(&This->css_style.IHTMLCSSStyleDeclaration_iface, v);
}

static HRESULT WINAPI HTMLStyle_get_cssText(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLCSSStyleDeclaration_get_cssText(&This->css_style.IHTMLCSSStyleDeclaration_iface, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelTop(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return set_style_pxattr(This, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelTop(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelLeft(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return set_style_pxattr(This, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelLeft(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelWidth(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->()\n", This);

    return set_style_pxattr(This, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelWidth(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelHeight(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%ld)\n", This, v);

    return set_style_pxattr(This, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelHeight(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_posTop(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_posTop(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_posLeft(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_posLeft(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_posWidth(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_posWidth(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_WIDTH, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_posHeight(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_posHeight(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_HEIGHT, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_cursor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_CURSOR, v);
}

static HRESULT WINAPI HTMLStyle_get_cursor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLStyle_put_clip(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_CLIP, v);
}

static HRESULT WINAPI HTMLStyle_get_clip(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_CLIP, p);
}

static void set_opacity(HTMLStyle *This, const WCHAR *val)
{
    nsAString name_str, val_str, empty_str;
    nsresult nsres;

    TRACE("%s\n", debugstr_w(val));

    nsAString_InitDepend(&name_str, L"opacity");
    nsAString_InitDepend(&val_str, val);
    nsAString_InitDepend(&empty_str, L"");

    nsres = nsIDOMCSSStyleDeclaration_SetProperty(This->css_style.nsstyle, &name_str, &val_str, &empty_str);
    if(NS_FAILED(nsres))
        ERR("SetProperty failed: %08lx\n", nsres);

    nsAString_Finish(&name_str);
    nsAString_Finish(&val_str);
    nsAString_Finish(&empty_str);
}

static void update_filter(HTMLStyle *This)
{
    const WCHAR *ptr, *ptr2;

    static const WCHAR alphaW[] = {'a','l','p','h','a'};

    if(dispex_compat_mode(&This->css_style.dispex) >= COMPAT_MODE_IE10)
        return;

    ptr = This->elem->filter;
    TRACE("%s\n", debugstr_w(ptr));
    if(!ptr) {
        set_opacity(This, L"");
        return;
    }

    while(1) {
        while(iswspace(*ptr))
            ptr++;
        if(!*ptr)
            break;

        ptr2 = ptr;
        while(iswalnum(*ptr))
            ptr++;
        if(ptr == ptr2) {
            WARN("unexpected char '%c'\n", *ptr);
            break;
        }
        if(*ptr != '(') {
            WARN("expected '('\n");
            continue;
        }

        if(ptr2 + ARRAY_SIZE(alphaW) == ptr && !memcmp(ptr2, alphaW, sizeof(alphaW))) {
            static const WCHAR opacityW[] = {'o','p','a','c','i','t','y','='};

            ptr++;
            do {
                while(iswspace(*ptr))
                    ptr++;

                ptr2 = ptr;
                while(*ptr && *ptr != ',' && *ptr != ')')
                    ptr++;
                if(!*ptr) {
                    WARN("unexpected end of string\n");
                    break;
                }

                if(ptr-ptr2 > ARRAY_SIZE(opacityW) && !memcmp(ptr2, opacityW, sizeof(opacityW))) {
                    float fval = 0.0f, e = 0.1f;
                    WCHAR buf[32];

                    ptr2 += ARRAY_SIZE(opacityW);

                    while(is_digit(*ptr2))
                        fval = fval*10.0f + (float)(*ptr2++ - '0');

                    if(*ptr2 == '.') {
                        while(is_digit(*++ptr2)) {
                            fval += e * (float)(*ptr2++ - '0');
                            e *= 0.1f;
                        }
                    }

                    swprintf(buf, ARRAY_SIZE(buf), L"%f", fval * 0.01f);
                    set_opacity(This, buf);
                }else {
                    FIXME("unknown param %s\n", debugstr_wn(ptr2, ptr-ptr2));
                }

                if(*ptr == ',')
                    ptr++;
            }while(*ptr != ')');
        }else {
            FIXME("unknown filter %s\n", debugstr_wn(ptr2, ptr-ptr2));
            ptr = wcschr(ptr, ')');
            if(!ptr)
                break;
            ptr++;
        }
    }
}

static HRESULT WINAPI HTMLStyle_put_filter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR *new_filter = NULL;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->elem) {
        FIXME("Element already destroyed\n");
        return E_UNEXPECTED;
    }

    if(v) {
        new_filter = wcsdup(v);
        if(!new_filter)
            return E_OUTOFMEMORY;
    }

    free(This->elem->filter);
    This->elem->filter = new_filter;

    update_filter(This);
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_filter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        FIXME("Element already destroyed\n");
        return E_UNEXPECTED;
    }

    if(This->elem->filter) {
        *p = SysAllocString(This->elem->filter);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_setAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        VARIANT AttributeValue, LONG lFlags)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s %s %08lx)\n", This, debugstr_w(strAttributeName),
          debugstr_variant(&AttributeValue), lFlags);

    if(!strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, &strAttributeName, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    if(hres == S_OK)
    {
        VARIANT ret;
        DISPID dispidNamed = DISPID_PROPERTYPUT;
        DISPPARAMS params;

        params.cArgs = 1;
        params.rgvarg = &AttributeValue;
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidNamed;

        hres = HTMLStyle_Invoke(iface, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
    }
    else
    {
        FIXME("Custom attributes not supported.\n");
    }

    TRACE("ret: %08lx\n", hres);

    return hres;
}

static HRESULT WINAPI HTMLStyle_getAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        LONG lFlags, VARIANT *AttributeValue)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s v%p %08lx)\n", This, debugstr_w(strAttributeName),
          AttributeValue, lFlags);

    if(!AttributeValue || !strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, &strAttributeName, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    if(hres == S_OK)
    {
        DISPPARAMS params = {NULL, NULL, 0, 0 };

        hres = HTMLStyle_Invoke(iface, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &params, AttributeValue, NULL, NULL);
    }
    else
    {
        FIXME("Custom attributes not supported.\n");
    }

    return hres;
}

static HRESULT WINAPI HTMLStyle_removeAttribute(IHTMLStyle *iface, BSTR strAttributeName,
                                                LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    const style_tbl_entry_t *style_entry;
    nsAString name_str, ret_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %08lx %p)\n", This, debugstr_w(strAttributeName), lFlags, pfSuccess);

    style_entry = lookup_style_tbl(&This->css_style, strAttributeName, 0);
    if(!style_entry) {
        DWORD fdex = (lFlags & 1) ? fdexNameCaseSensitive : fdexNameCaseInsensitive;
        compat_mode_t compat_mode = dispex_compat_mode(&This->css_style.dispex);
        DISPID dispid;
        unsigned i;

        if(compat_mode < COMPAT_MODE_IE9)
            hres = IWineJSDispatchHost_GetDispID(&This->css_style.dispex.IWineJSDispatchHost_iface, strAttributeName, fdex, &dispid);
        else
            hres = dispex_get_chain_builtin_id(&This->css_style.dispex, strAttributeName, fdex, &dispid);
        if(hres != S_OK) {
            *pfSuccess = VARIANT_FALSE;
            return S_OK;
        }

        for(i=0; i < ARRAY_SIZE(style_tbl); i++) {
            if(dispid == (compat_mode >= COMPAT_MODE_IE9
                          ? style_tbl[i].dispid : style_tbl[i].compat_dispid))
                break;
        }

        if(i == ARRAY_SIZE(style_tbl))
            return remove_attribute(&This->css_style.dispex, dispid, pfSuccess);
        style_entry = style_tbl+i;
    }

    /* filter property is a special case */
    if(style_entry->compat_dispid == DISPID_IHTMLSTYLE_FILTER) {
        if(!This->elem)
            return E_UNEXPECTED;
        *pfSuccess = variant_bool(This->elem->filter && *This->elem->filter);
        free(This->elem->filter);
        This->elem->filter = NULL;
        update_filter(This);
        return S_OK;
    }

    nsAString_InitDepend(&name_str, get_style_nsname(style_entry));
    nsAString_Init(&ret_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_RemoveProperty(This->css_style.nsstyle, &name_str, &ret_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *ret;
        nsAString_GetData(&ret_str, &ret);
        *pfSuccess = variant_bool(*ret);
    }else {
        WARN("RemoveProperty failed: %08lx\n", nsres);
    }
    nsAString_Finish(&name_str);
    nsAString_Finish(&ret_str);
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLStyle_toString(IHTMLStyle *iface, BSTR *String)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, String);

    return dispex_to_string(&This->css_style.dispex, String);
}

static const IHTMLStyleVtbl HTMLStyleVtbl = {
    HTMLStyle_QueryInterface,
    HTMLStyle_AddRef,
    HTMLStyle_Release,
    HTMLStyle_GetTypeInfoCount,
    HTMLStyle_GetTypeInfo,
    HTMLStyle_GetIDsOfNames,
    HTMLStyle_Invoke,
    HTMLStyle_put_fontFamily,
    HTMLStyle_get_fontFamily,
    HTMLStyle_put_fontStyle,
    HTMLStyle_get_fontStyle,
    HTMLStyle_put_fontVariant,
    HTMLStyle_get_fontVariant,
    HTMLStyle_put_fontWeight,
    HTMLStyle_get_fontWeight,
    HTMLStyle_put_fontSize,
    HTMLStyle_get_fontSize,
    HTMLStyle_put_font,
    HTMLStyle_get_font,
    HTMLStyle_put_color,
    HTMLStyle_get_color,
    HTMLStyle_put_background,
    HTMLStyle_get_background,
    HTMLStyle_put_backgroundColor,
    HTMLStyle_get_backgroundColor,
    HTMLStyle_put_backgroundImage,
    HTMLStyle_get_backgroundImage,
    HTMLStyle_put_backgroundRepeat,
    HTMLStyle_get_backgroundRepeat,
    HTMLStyle_put_backgroundAttachment,
    HTMLStyle_get_backgroundAttachment,
    HTMLStyle_put_backgroundPosition,
    HTMLStyle_get_backgroundPosition,
    HTMLStyle_put_backgroundPositionX,
    HTMLStyle_get_backgroundPositionX,
    HTMLStyle_put_backgroundPositionY,
    HTMLStyle_get_backgroundPositionY,
    HTMLStyle_put_wordSpacing,
    HTMLStyle_get_wordSpacing,
    HTMLStyle_put_letterSpacing,
    HTMLStyle_get_letterSpacing,
    HTMLStyle_put_textDecoration,
    HTMLStyle_get_textDecoration,
    HTMLStyle_put_textDecorationNone,
    HTMLStyle_get_textDecorationNone,
    HTMLStyle_put_textDecorationUnderline,
    HTMLStyle_get_textDecorationUnderline,
    HTMLStyle_put_textDecorationOverline,
    HTMLStyle_get_textDecorationOverline,
    HTMLStyle_put_textDecorationLineThrough,
    HTMLStyle_get_textDecorationLineThrough,
    HTMLStyle_put_textDecorationBlink,
    HTMLStyle_get_textDecorationBlink,
    HTMLStyle_put_verticalAlign,
    HTMLStyle_get_verticalAlign,
    HTMLStyle_put_textTransform,
    HTMLStyle_get_textTransform,
    HTMLStyle_put_textAlign,
    HTMLStyle_get_textAlign,
    HTMLStyle_put_textIndent,
    HTMLStyle_get_textIndent,
    HTMLStyle_put_lineHeight,
    HTMLStyle_get_lineHeight,
    HTMLStyle_put_marginTop,
    HTMLStyle_get_marginTop,
    HTMLStyle_put_marginRight,
    HTMLStyle_get_marginRight,
    HTMLStyle_put_marginBottom,
    HTMLStyle_get_marginBottom,
    HTMLStyle_put_marginLeft,
    HTMLStyle_get_marginLeft,
    HTMLStyle_put_margin,
    HTMLStyle_get_margin,
    HTMLStyle_put_paddingTop,
    HTMLStyle_get_paddingTop,
    HTMLStyle_put_paddingRight,
    HTMLStyle_get_paddingRight,
    HTMLStyle_put_paddingBottom,
    HTMLStyle_get_paddingBottom,
    HTMLStyle_put_paddingLeft,
    HTMLStyle_get_paddingLeft,
    HTMLStyle_put_padding,
    HTMLStyle_get_padding,
    HTMLStyle_put_border,
    HTMLStyle_get_border,
    HTMLStyle_put_borderTop,
    HTMLStyle_get_borderTop,
    HTMLStyle_put_borderRight,
    HTMLStyle_get_borderRight,
    HTMLStyle_put_borderBottom,
    HTMLStyle_get_borderBottom,
    HTMLStyle_put_borderLeft,
    HTMLStyle_get_borderLeft,
    HTMLStyle_put_borderColor,
    HTMLStyle_get_borderColor,
    HTMLStyle_put_borderTopColor,
    HTMLStyle_get_borderTopColor,
    HTMLStyle_put_borderRightColor,
    HTMLStyle_get_borderRightColor,
    HTMLStyle_put_borderBottomColor,
    HTMLStyle_get_borderBottomColor,
    HTMLStyle_put_borderLeftColor,
    HTMLStyle_get_borderLeftColor,
    HTMLStyle_put_borderWidth,
    HTMLStyle_get_borderWidth,
    HTMLStyle_put_borderTopWidth,
    HTMLStyle_get_borderTopWidth,
    HTMLStyle_put_borderRightWidth,
    HTMLStyle_get_borderRightWidth,
    HTMLStyle_put_borderBottomWidth,
    HTMLStyle_get_borderBottomWidth,
    HTMLStyle_put_borderLeftWidth,
    HTMLStyle_get_borderLeftWidth,
    HTMLStyle_put_borderStyle,
    HTMLStyle_get_borderStyle,
    HTMLStyle_put_borderTopStyle,
    HTMLStyle_get_borderTopStyle,
    HTMLStyle_put_borderRightStyle,
    HTMLStyle_get_borderRightStyle,
    HTMLStyle_put_borderBottomStyle,
    HTMLStyle_get_borderBottomStyle,
    HTMLStyle_put_borderLeftStyle,
    HTMLStyle_get_borderLeftStyle,
    HTMLStyle_put_width,
    HTMLStyle_get_width,
    HTMLStyle_put_height,
    HTMLStyle_get_height,
    HTMLStyle_put_styleFloat,
    HTMLStyle_get_styleFloat,
    HTMLStyle_put_clear,
    HTMLStyle_get_clear,
    HTMLStyle_put_display,
    HTMLStyle_get_display,
    HTMLStyle_put_visibility,
    HTMLStyle_get_visibility,
    HTMLStyle_put_listStyleType,
    HTMLStyle_get_listStyleType,
    HTMLStyle_put_listStylePosition,
    HTMLStyle_get_listStylePosition,
    HTMLStyle_put_listStyleImage,
    HTMLStyle_get_listStyleImage,
    HTMLStyle_put_listStyle,
    HTMLStyle_get_listStyle,
    HTMLStyle_put_whiteSpace,
    HTMLStyle_get_whiteSpace,
    HTMLStyle_put_top,
    HTMLStyle_get_top,
    HTMLStyle_put_left,
    HTMLStyle_get_left,
    HTMLStyle_get_position,
    HTMLStyle_put_zIndex,
    HTMLStyle_get_zIndex,
    HTMLStyle_put_overflow,
    HTMLStyle_get_overflow,
    HTMLStyle_put_pageBreakBefore,
    HTMLStyle_get_pageBreakBefore,
    HTMLStyle_put_pageBreakAfter,
    HTMLStyle_get_pageBreakAfter,
    HTMLStyle_put_cssText,
    HTMLStyle_get_cssText,
    HTMLStyle_put_pixelTop,
    HTMLStyle_get_pixelTop,
    HTMLStyle_put_pixelLeft,
    HTMLStyle_get_pixelLeft,
    HTMLStyle_put_pixelWidth,
    HTMLStyle_get_pixelWidth,
    HTMLStyle_put_pixelHeight,
    HTMLStyle_get_pixelHeight,
    HTMLStyle_put_posTop,
    HTMLStyle_get_posTop,
    HTMLStyle_put_posLeft,
    HTMLStyle_get_posLeft,
    HTMLStyle_put_posWidth,
    HTMLStyle_get_posWidth,
    HTMLStyle_put_posHeight,
    HTMLStyle_get_posHeight,
    HTMLStyle_put_cursor,
    HTMLStyle_get_cursor,
    HTMLStyle_put_clip,
    HTMLStyle_get_clip,
    HTMLStyle_put_filter,
    HTMLStyle_get_filter,
    HTMLStyle_setAttribute,
    HTMLStyle_getAttribute,
    HTMLStyle_removeAttribute,
    HTMLStyle_toString
};

static inline HTMLStyle *impl_from_IHTMLStyle2(IHTMLStyle2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle2, IHTMLStyle2, impl_from_IHTMLStyle2(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle2_put_tableLayout(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_TABLE_LAYOUT, v);
}

static HRESULT WINAPI HTMLStyle2_get_tableLayout(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_TABLE_LAYOUT, p);
}

static HRESULT WINAPI HTMLStyle2_put_borderCollapse(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BORDER_COLLAPSE, v);
}

static HRESULT WINAPI HTMLStyle2_get_borderCollapse(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BORDER_COLLAPSE, p);
}

static HRESULT WINAPI HTMLStyle2_put_direction(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_DIRECTION, v);
}

static HRESULT WINAPI HTMLStyle2_get_direction(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_DIRECTION, p);
}

static HRESULT WINAPI HTMLStyle2_put_behavior(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle2_get_behavior(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_setExpression(IHTMLStyle2 *iface, BSTR propname, BSTR expression, BSTR language)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(propname), debugstr_w(expression), debugstr_w(language));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_getExpression(IHTMLStyle2 *iface, BSTR propname, VARIANT *expression)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), expression);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_removeExpression(IHTMLStyle2 *iface, BSTR propname, VARIANT_BOOL *pfSuccess)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), pfSuccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_position(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_POSITION, v);
}

static HRESULT WINAPI HTMLStyle2_get_position(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_POSITION, p);
}

static HRESULT WINAPI HTMLStyle2_put_unicodeBidi(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_unicodeBidi(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_bottom(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_BOTTOM, &v);
}

static HRESULT WINAPI HTMLStyle2_get_bottom(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle2_put_right(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_RIGHT, &v);
}

static HRESULT WINAPI HTMLStyle2_get_right(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle2_put_pixelBottom(IHTMLStyle2 *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_pixelBottom(IHTMLStyle2 *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_pixelRight(IHTMLStyle2 *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_pixelRight(IHTMLStyle2 *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_posBottom(IHTMLStyle2 *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_posBottom(IHTMLStyle2 *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_posRight(IHTMLStyle2 *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%f)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_posRight(IHTMLStyle2 *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_imeMode(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_imeMode(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyAlign(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyAlign(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyPosition(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyPosition(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_rubyOverhang(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_rubyOverhang(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridChar(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridChar(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridLine(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridLine(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridMode(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridMode(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGridType(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGridType(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_layoutGrid(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_layoutGrid(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_wordBreak(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_wordBreak(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_lineBreak(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_lineBreak(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textJustify(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textJustify(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textJustifyTrim(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textJustifyTrim(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textKashida(IHTMLStyle2 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textKashida(IHTMLStyle2 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_textAutospace(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_textAutospace(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_put_overflowX(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_OVERFLOW_X, v);
}

static HRESULT WINAPI HTMLStyle2_get_overflowX(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_OVERFLOW_X, p);
}

static HRESULT WINAPI HTMLStyle2_put_overflowY(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_OVERFLOW_Y, v);
}

static HRESULT WINAPI HTMLStyle2_get_overflowY(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_OVERFLOW_Y, p);
}

static HRESULT WINAPI HTMLStyle2_put_accelerator(IHTMLStyle2 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle2_get_accelerator(IHTMLStyle2 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle2(iface);
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

static inline HTMLStyle *impl_from_IHTMLStyle3(IHTMLStyle3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle3_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle3, IHTMLStyle3, impl_from_IHTMLStyle3(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle3_put_layoutFlow(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_layoutFlow(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_zoom(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return IHTMLCSSStyleDeclaration_put_zoom(&This->css_style.IHTMLCSSStyleDeclaration_iface, v);
}

static HRESULT WINAPI HTMLStyle3_get_zoom(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLCSSStyleDeclaration_get_zoom(&This->css_style.IHTMLCSSStyleDeclaration_iface, p);
}

static HRESULT WINAPI HTMLStyle3_put_wordWrap(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_WORD_WRAP, v);
}

static HRESULT WINAPI HTMLStyle3_get_wordWrap(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_WORD_WRAP, p);
}

static HRESULT WINAPI HTMLStyle3_put_textUnderlinePosition(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textUnderlinePosition(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarBaseColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarBaseColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarFaceColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarFaceColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbar3dLightColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbar3dLightColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarShadowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarShadowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarHighlightColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarHighlightColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarDarkShadowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarDarkShadowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarArrowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarArrowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarTrackColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return S_OK;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarTrackColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_writingMode(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_writingMode(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_textAlignLast(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textAlignLast(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_textKashidaSpace(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textKashidaSpace(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle3Vtbl HTMLStyle3Vtbl = {
    HTMLStyle3_QueryInterface,
    HTMLStyle3_AddRef,
    HTMLStyle3_Release,
    HTMLStyle3_GetTypeInfoCount,
    HTMLStyle3_GetTypeInfo,
    HTMLStyle3_GetIDsOfNames,
    HTMLStyle3_Invoke,
    HTMLStyle3_put_layoutFlow,
    HTMLStyle3_get_layoutFlow,
    HTMLStyle3_put_zoom,
    HTMLStyle3_get_zoom,
    HTMLStyle3_put_wordWrap,
    HTMLStyle3_get_wordWrap,
    HTMLStyle3_put_textUnderlinePosition,
    HTMLStyle3_get_textUnderlinePosition,
    HTMLStyle3_put_scrollbarBaseColor,
    HTMLStyle3_get_scrollbarBaseColor,
    HTMLStyle3_put_scrollbarFaceColor,
    HTMLStyle3_get_scrollbarFaceColor,
    HTMLStyle3_put_scrollbar3dLightColor,
    HTMLStyle3_get_scrollbar3dLightColor,
    HTMLStyle3_put_scrollbarShadowColor,
    HTMLStyle3_get_scrollbarShadowColor,
    HTMLStyle3_put_scrollbarHighlightColor,
    HTMLStyle3_get_scrollbarHighlightColor,
    HTMLStyle3_put_scrollbarDarkShadowColor,
    HTMLStyle3_get_scrollbarDarkShadowColor,
    HTMLStyle3_put_scrollbarArrowColor,
    HTMLStyle3_get_scrollbarArrowColor,
    HTMLStyle3_put_scrollbarTrackColor,
    HTMLStyle3_get_scrollbarTrackColor,
    HTMLStyle3_put_writingMode,
    HTMLStyle3_get_writingMode,
    HTMLStyle3_put_textAlignLast,
    HTMLStyle3_get_textAlignLast,
    HTMLStyle3_put_textKashidaSpace,
    HTMLStyle3_get_textKashidaSpace
};

/*
 * IHTMLStyle4 Interface
 */
static inline HTMLStyle *impl_from_IHTMLStyle4(IHTMLStyle4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle4_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle4, IHTMLStyle4, impl_from_IHTMLStyle4(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle4_put_textOverflow(IHTMLStyle4 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle4_get_textOverflow(IHTMLStyle4 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle4_put_minHeight(IHTMLStyle4 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MIN_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle4_get_minHeight(IHTMLStyle4 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_MIN_HEIGHT, p);
}

static const IHTMLStyle4Vtbl HTMLStyle4Vtbl = {
    HTMLStyle4_QueryInterface,
    HTMLStyle4_AddRef,
    HTMLStyle4_Release,
    HTMLStyle4_GetTypeInfoCount,
    HTMLStyle4_GetTypeInfo,
    HTMLStyle4_GetIDsOfNames,
    HTMLStyle4_Invoke,
    HTMLStyle4_put_textOverflow,
    HTMLStyle4_get_textOverflow,
    HTMLStyle4_put_minHeight,
    HTMLStyle4_get_minHeight
};

static inline HTMLStyle *impl_from_IHTMLStyle5(IHTMLStyle5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle5_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle5, IHTMLStyle5, impl_from_IHTMLStyle5(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle5_put_msInterpolationMode(IHTMLStyle5 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_get_msInterpolationMode(IHTMLStyle5 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_put_maxHeight(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MAX_HEIGHT, &v);
}

static HRESULT WINAPI HTMLStyle5_get_maxHeight(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(p));

    return get_style_property_var(&This->css_style, STYLEID_MAX_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle5_put_minWidth(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MIN_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle5_get_minWidth(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_MIN_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle5_put_maxWidth(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_style_property_var(&This->css_style, STYLEID_MAX_WIDTH, &v);
}

static HRESULT WINAPI HTMLStyle5_get_maxWidth(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property_var(&This->css_style, STYLEID_MAX_WIDTH, p);
}

static const IHTMLStyle5Vtbl HTMLStyle5Vtbl = {
    HTMLStyle5_QueryInterface,
    HTMLStyle5_AddRef,
    HTMLStyle5_Release,
    HTMLStyle5_GetTypeInfoCount,
    HTMLStyle5_GetTypeInfo,
    HTMLStyle5_GetIDsOfNames,
    HTMLStyle5_Invoke,
    HTMLStyle5_put_msInterpolationMode,
    HTMLStyle5_get_msInterpolationMode,
    HTMLStyle5_put_maxHeight,
    HTMLStyle5_get_maxHeight,
    HTMLStyle5_put_minWidth,
    HTMLStyle5_get_minWidth,
    HTMLStyle5_put_maxWidth,
    HTMLStyle5_get_maxWidth
};

static inline HTMLStyle *impl_from_IHTMLStyle6(IHTMLStyle6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle6_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle6, IHTMLStyle6, impl_from_IHTMLStyle6(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle6_put_content(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_content(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_contentSide(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_contentSide(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_counterIncrement(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_counterIncrement(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_counterReset(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_counterReset(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outline(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_OUTLINE, v);
}

static HRESULT WINAPI HTMLStyle6_get_outline(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_OUTLINE, p);
}

static HRESULT WINAPI HTMLStyle6_put_outlineWidth(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineWidth(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outlineStyle(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineStyle(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outlineColor(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineColor(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_boxSizing(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BOX_SIZING, v);
}

static HRESULT WINAPI HTMLStyle6_get_boxSizing(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BOX_SIZING, p);
}

static HRESULT WINAPI HTMLStyle6_put_borderSpacing(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(&This->css_style, STYLEID_BORDER_SPACING, v);
}

static HRESULT WINAPI HTMLStyle6_get_borderSpacing(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(&This->css_style, STYLEID_BORDER_SPACING, p);
}

static HRESULT WINAPI HTMLStyle6_put_orphans(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_orphans(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_windows(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_windows(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_pageBreakInside(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_pageBreakInside(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_emptyCells(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_emptyCells(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_msBlockProgression(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_msBlockProgression(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_quotes(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_quotes(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle6Vtbl HTMLStyle6Vtbl = {
    HTMLStyle6_QueryInterface,
    HTMLStyle6_AddRef,
    HTMLStyle6_Release,
    HTMLStyle6_GetTypeInfoCount,
    HTMLStyle6_GetTypeInfo,
    HTMLStyle6_GetIDsOfNames,
    HTMLStyle6_Invoke,
    HTMLStyle6_put_content,
    HTMLStyle6_get_content,
    HTMLStyle6_put_contentSide,
    HTMLStyle6_get_contentSide,
    HTMLStyle6_put_counterIncrement,
    HTMLStyle6_get_counterIncrement,
    HTMLStyle6_put_counterReset,
    HTMLStyle6_get_counterReset,
    HTMLStyle6_put_outline,
    HTMLStyle6_get_outline,
    HTMLStyle6_put_outlineWidth,
    HTMLStyle6_get_outlineWidth,
    HTMLStyle6_put_outlineStyle,
    HTMLStyle6_get_outlineStyle,
    HTMLStyle6_put_outlineColor,
    HTMLStyle6_get_outlineColor,
    HTMLStyle6_put_boxSizing,
    HTMLStyle6_get_boxSizing,
    HTMLStyle6_put_borderSpacing,
    HTMLStyle6_get_borderSpacing,
    HTMLStyle6_put_orphans,
    HTMLStyle6_get_orphans,
    HTMLStyle6_put_windows,
    HTMLStyle6_get_windows,
    HTMLStyle6_put_pageBreakInside,
    HTMLStyle6_get_pageBreakInside,
    HTMLStyle6_put_emptyCells,
    HTMLStyle6_get_emptyCells,
    HTMLStyle6_put_msBlockProgression,
    HTMLStyle6_get_msBlockProgression,
    HTMLStyle6_put_quotes,
    HTMLStyle6_get_quotes
};

static inline HTMLStyle *impl_from_IWineCSSProperties(IWineCSSProperties *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IWineCSSProperties_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLStyle_CSSProperties, IWineCSSProperties, impl_from_IWineCSSProperties(iface)->css_style.dispex)

static HRESULT WINAPI HTMLStyle_CSSProperties_getAttribute(IWineCSSProperties *iface, BSTR name, LONG flags, VARIANT *p)
{
    HTMLStyle *This = impl_from_IWineCSSProperties(iface);
    return HTMLStyle_getAttribute(&This->IHTMLStyle_iface, name, flags, p);
}

static HRESULT WINAPI HTMLStyle_CSSProperties_setAttribute(IWineCSSProperties *iface, BSTR name, VARIANT value, LONG flags)
{
    HTMLStyle *This = impl_from_IWineCSSProperties(iface);
    return HTMLStyle_setAttribute(&This->IHTMLStyle_iface, name, value, flags);
}

static HRESULT WINAPI HTMLStyle_CSSProperties_removeAttribute(IWineCSSProperties *iface, BSTR name, LONG flags, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IWineCSSProperties(iface);
    return HTMLStyle_removeAttribute(&This->IHTMLStyle_iface, name, flags, p);
}

static const IWineCSSPropertiesVtbl HTMLStyle_CSSPropertiesVtbl = {
    HTMLStyle_CSSProperties_QueryInterface,
    HTMLStyle_CSSProperties_AddRef,
    HTMLStyle_CSSProperties_Release,
    HTMLStyle_CSSProperties_GetTypeInfoCount,
    HTMLStyle_CSSProperties_GetTypeInfo,
    HTMLStyle_CSSProperties_GetIDsOfNames,
    HTMLStyle_CSSProperties_Invoke,
    HTMLStyle_CSSProperties_setAttribute,
    HTMLStyle_CSSProperties_getAttribute,
    HTMLStyle_CSSProperties_removeAttribute
};

static inline CSSStyle *impl_from_IHTMLCSSStyleDeclaration(IHTMLCSSStyleDeclaration *iface)
{
    return CONTAINING_RECORD(iface, CSSStyle, IHTMLCSSStyleDeclaration_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLCSSStyleDeclaration, IHTMLCSSStyleDeclaration,
                      impl_from_IHTMLCSSStyleDeclaration(iface)->dispex)

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_length(IHTMLCSSStyleDeclaration *iface, LONG *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_parentRule(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_getPropertyValue(IHTMLCSSStyleDeclaration *iface, BSTR name, BSTR *value)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    const style_tbl_entry_t *style_entry;
    nsAString name_str, value_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), value);

    style_entry = lookup_style_tbl(This, name, 0);
    nsAString_InitDepend(&name_str, get_style_prop_nsname(style_entry, name));
    nsAString_InitDepend(&value_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_GetPropertyValue(This->nsstyle, &name_str, &value_str);
    nsAString_Finish(&name_str);
    return return_nsstr(nsres, &value_str, value);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_getPropertyPriority(IHTMLCSSStyleDeclaration *iface, BSTR bstrPropertyName, BSTR *pbstrPropertyPriority)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrPropertyName), pbstrPropertyPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_removeProperty(IHTMLCSSStyleDeclaration *iface, BSTR bstrPropertyName, BSTR *pbstrPropertyValue)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    const style_tbl_entry_t *style_entry;
    nsAString name_str, ret_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrPropertyName), pbstrPropertyValue);

    style_entry = lookup_style_tbl(This, bstrPropertyName, 0);
    nsAString_InitDepend(&name_str, get_style_prop_nsname(style_entry, bstrPropertyName));
    nsAString_Init(&ret_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_RemoveProperty(This->nsstyle, &name_str, &ret_str);
    nsAString_Finish(&name_str);
    return return_nsstr(nsres, &ret_str, pbstrPropertyValue);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_setProperty(IHTMLCSSStyleDeclaration *iface, BSTR name, VARIANT *value, VARIANT *priority)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString priority_str, name_str, value_str;
    const style_tbl_entry_t *style_entry;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s)\n", This, debugstr_w(name), debugstr_variant(value), debugstr_variant(priority));

    style_entry = lookup_style_tbl(This, name, 0);
    hres = var_to_styleval(This, value, style_entry, &value_str);
    if(FAILED(hres))
        return hres;

    if(priority) {
        if(V_VT(priority) != VT_BSTR) {
            WARN("invalid priority type %s\n", debugstr_variant(priority));
            nsAString_Finish(&value_str);
            return S_OK;
        }
        nsAString_InitDepend(&priority_str, V_BSTR(priority));
    }else {
        nsAString_InitDepend(&priority_str, NULL);
    }

    nsAString_InitDepend(&name_str, get_style_prop_nsname(style_entry, name));
    nsres = nsIDOMCSSStyleDeclaration_SetProperty(This->nsstyle, &name_str, &value_str, &priority_str);
    nsAString_Finish(&name_str);
    nsAString_Finish(&value_str);
    nsAString_Finish(&priority_str);
    if(NS_FAILED(nsres))
        WARN("SetProperty failed: %08lx\n", nsres);
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_item(IHTMLCSSStyleDeclaration *iface, LONG index, BSTR *pbstrPropertyName)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, pbstrPropertyName);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontFamily(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FONT_FAMILY, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontFamily(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FONT_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontVariant(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FONT_VARIANT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontVariant(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontWeight(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FONT_WEIGHT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontWeight(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FONT_WEIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontSize(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_FONT_SIZE, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontSize(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_FONT_SIZE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_font(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_font(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_color(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_color(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_background(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_background(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BACKGROUND_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BACKGROUND_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundImage(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND_IMAGE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundImage(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundRepeat(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND_REPEAT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundRepeat(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND_REPEAT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundAttachment(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND_ATTACHMENT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundAttachment(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND_ATTACHMENT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundPosition(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND_POSITION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundPosition(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND_POSITION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundPositionX(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString pos_str, val_str;
    const WCHAR *val;
    WCHAR *pos_val;
    DWORD val_len;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var_to_styleval(This, &v, &style_tbl[STYLEID_BACKGROUND_POSITION_X], &val_str);
    if(FAILED(hres))
        return hres;

    nsAString_GetData(&val_str, &val);
    val_len = val ? lstrlenW(val) : 0;

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *posy;
        DWORD posy_len;

        nsAString_GetData(&pos_str, &pos);
        posy = wcschr(pos, ' ');
        if(!posy) {
            TRACE("no space in %s\n", debugstr_w(pos));
            posy = L" 0px";
        }

        posy_len = lstrlenW(posy);
        pos_val = malloc((val_len + posy_len + 1) * sizeof(WCHAR));
        if(pos_val) {
            if(val_len)
                memcpy(pos_val, val, val_len*sizeof(WCHAR));
            if(posy_len)
                memcpy(pos_val+val_len, posy, posy_len*sizeof(WCHAR));
            pos_val[val_len+posy_len] = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }
    nsAString_Finish(&pos_str);
    nsAString_Finish(&val_str);
    if(FAILED(hres))
        return hres;

    TRACE("setting position to %s\n", debugstr_w(pos_val));
    hres = set_style_property(This, STYLEID_BACKGROUND_POSITION, pos_val);
    free(pos_val);
    return hres;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundPositionX(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString pos_str;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *space;

        nsAString_GetData(&pos_str, &pos);
        space = wcschr(pos, ' ');
        if(!space) {
            WARN("no space in %s\n", debugstr_w(pos));
            space = pos + lstrlenW(pos);
        }

        if(space != pos) {
            ret = SysAllocStringLen(pos, space-pos);
            if(!ret)
                hres = E_OUTOFMEMORY;
        }else {
            ret = NULL;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("returning %s\n", debugstr_w(ret));
    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundPositionY(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString pos_str, val_str;
    const WCHAR *val;
    WCHAR *pos_val;
    DWORD val_len;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var_to_styleval(This, &v, &style_tbl[STYLEID_BACKGROUND_POSITION], &val_str);
    if(FAILED(hres))
        return hres;

    nsAString_GetData(&val_str, &val);
    val_len = val ? lstrlenW(val) : 0;

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *space;
        DWORD posx_len;

        nsAString_GetData(&pos_str, &pos);
        space = wcschr(pos, ' ');
        if(space) {
            space++;
        }else {
            TRACE("no space in %s\n", debugstr_w(pos));
            pos = L"0px ";
            space = pos + ARRAY_SIZE(L"0px ")-1;
        }

        posx_len = space-pos;

        pos_val = malloc((posx_len + val_len + 1) * sizeof(WCHAR));
        if(pos_val) {
            memcpy(pos_val, pos, posx_len*sizeof(WCHAR));
            if(val_len)
                memcpy(pos_val+posx_len, val, val_len*sizeof(WCHAR));
            pos_val[posx_len+val_len] = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }
    nsAString_Finish(&pos_str);
    nsAString_Finish(&val_str);
    if(FAILED(hres))
        return hres;

    TRACE("setting position to %s\n", debugstr_w(pos_val));
    hres = set_style_property(This, STYLEID_BACKGROUND_POSITION, pos_val);
    free(pos_val);
    return hres;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundPositionY(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString pos_str;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *posy;

        nsAString_GetData(&pos_str, &pos);
        posy = wcschr(pos, ' ');
        if(posy) {
            ret = SysAllocString(posy+1);
            if(!ret)
                hres = E_OUTOFMEMORY;
        }else {
            ret = NULL;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("returning %s\n", debugstr_w(ret));
    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_wordSpacing(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_WORD_SPACING, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_wordSpacing(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_WORD_SPACING, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_letterSpacing(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_LETTER_SPACING, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_letterSpacing(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_LETTER_SPACING, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textDecoration(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_TEXT_DECORATION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textDecoration(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_verticalAlign(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_VERTICAL_ALIGN, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_verticalAlign(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_VERTICAL_ALIGN, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textTransform(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_TEXT_TRANSFORM, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textTransform(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_TEXT_TRANSFORM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textAlign(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_TEXT_ALIGN, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textAlign(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textIndent(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_TEXT_INDENT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textIndent(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_TEXT_INDENT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_lineHeight(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_LINE_HEIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_lineHeight(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_LINE_HEIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_marginTop(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MARGIN_TOP, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_marginTop(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MARGIN_TOP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_marginRight(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MARGIN_RIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_marginRight(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MARGIN_RIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_marginBottom(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MARGIN_BOTTOM, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_marginBottom(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MARGIN_BOTTOM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_marginLeft(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MARGIN_LEFT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_marginLeft(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MARGIN_LEFT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_margin(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_MARGIN, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_margin(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_MARGIN, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_paddingTop(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_PADDING_TOP, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_paddingTop(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_PADDING_TOP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_paddingRight(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_PADDING_RIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_paddingRight(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_PADDING_RIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_paddingBottom(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_PADDING_BOTTOM, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_paddingBottom(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_PADDING_BOTTOM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_paddingLeft(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_PADDING_LEFT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_paddingLeft(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_PADDING_LEFT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_padding(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_PADDING, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_padding(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_PADDING, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_border(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_border(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderTop(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_TOP, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderTop(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_TOP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderRight(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_RIGHT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderRight(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_RIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderBottom(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_BOTTOM, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderBottom(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_BOTTOM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderLeft(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_LEFT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderLeft(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_LEFT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderColor(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_COLOR, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderColor(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderTopColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_TOP_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderTopColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_TOP_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderRightColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_RIGHT_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderRightColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_RIGHT_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderBottomColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_BOTTOM_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderBottomColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_BOTTOM_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderLeftColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_LEFT_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderLeftColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_LEFT_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderWidth(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_WIDTH, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderWidth(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderTopWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_TOP_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderTopWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_TOP_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderRightWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_RIGHT_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderRightWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_RIGHT_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderBottomWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_BOTTOM_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderBottomWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_BOTTOM_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderLeftWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BORDER_LEFT_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderLeftWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BORDER_LEFT_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderTopStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_TOP_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderTopStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderRightStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_RIGHT_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderRightStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderBottomStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_BOTTOM_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderBottomStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderLeftStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_LEFT_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderLeftStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_width(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_width(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_height(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_HEIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_height(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_styleFloat(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FLOAT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_styleFloat(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FLOAT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clear(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_CLEAR, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clear(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_CLEAR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_display(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_DISPLAY, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_display(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_visibility(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_VISIBILITY, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_visibility(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_VISIBILITY, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_listStyleType(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_listStyleType(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_listStylePosition(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_listStylePosition(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_listStyleImage(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_listStyleImage(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_listStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_LIST_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_listStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_LIST_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_whiteSpace(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_WHITE_SPACE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_whiteSpace(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_WHITE_SPACE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_top(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_TOP, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_top(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_left(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_LEFT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_left(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_zIndex(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_Z_INDEX, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_zIndex(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_Z_INDEX, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_overflow(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_OVERFLOW, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_overflow(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_OVERFLOW, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_pageBreakBefore(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_PAGE_BREAK_BEFORE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_pageBreakBefore(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_PAGE_BREAK_BEFORE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_pageBreakAfter(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_PAGE_BREAK_AFTER, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_pageBreakAfter(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_PAGE_BREAK_AFTER, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_cssText(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMCSSStyleDeclaration_SetCssText(This->nsstyle, &text_str);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        FIXME("SetCssStyle failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_cssText(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    /* NOTE: Quicks mode should use different formatting (uppercase, no ';' at the end of rule). */
    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_GetCssText(This->nsstyle, &text_str);
    return return_nsstr(nsres, &text_str, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_cursor(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_CURSOR, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_cursor(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clip(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_CLIP, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clip(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_CLIP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_filter(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FILTER, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_filter(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FILTER, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_tableLayout(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_TABLE_LAYOUT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_tableLayout(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_TABLE_LAYOUT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderCollapse(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_COLLAPSE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderCollapse(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_COLLAPSE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_direction(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_DIRECTION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_direction(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_DIRECTION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_behavior(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_behavior(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_position(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_POSITION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_position(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_POSITION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_unicodeBidi(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_unicodeBidi(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_bottom(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_BOTTOM, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_bottom(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_BOTTOM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_right(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_RIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_right(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_RIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_imeMode(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_imeMode(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_rubyAlign(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_rubyAlign(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_rubyPosition(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_rubyPosition(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_rubyOverhang(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_rubyOverhang(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_layoutGridChar(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_layoutGridChar(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_layoutGridLine(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_layoutGridLine(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_layoutGridMode(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_layoutGridMode(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_layoutGridType(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_layoutGridType(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_layoutGrid(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_layoutGrid(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textAutospace(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textAutospace(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_wordBreak(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_wordBreak(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_lineBreak(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_lineBreak(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textJustify(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textJustify(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textJustifyTrim(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textJustifyTrim(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textKashida(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textKashida(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_overflowX(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_OVERFLOW_X, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_overflowX(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_OVERFLOW_X, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_overflowY(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_OVERFLOW_Y, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_overflowY(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_OVERFLOW_Y, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_accelerator(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_accelerator(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_layoutFlow(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_layoutFlow(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_zoom(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    VARIANT *var;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    /* zoom property is IE CSS extension that is mostly used as a hack to workaround IE bugs.
     * The value is set to 1 then. We can safely ignore setting zoom to 1. */
    if(V_VT(&v) != VT_I4 || V_I4(&v) != 1)
        WARN("stub for %s\n", debugstr_variant(&v));

    hres = dispex_get_dprop_ref(&This->dispex, L"zoom", TRUE, &var);
    if(FAILED(hres))
        return hres;

    return VariantChangeType(var, &v, 0, VT_BSTR);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_zoom(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    VARIANT *var;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = dispex_get_dprop_ref(&This->dispex, L"zoom", FALSE, &var);
    if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = NULL;
        return S_OK;
    }
    if(FAILED(hres))
        return hres;

    return VariantCopy(p, var);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_wordWrap(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_WORD_WRAP, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_wordWrap(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_WORD_WRAP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textUnderlinePosition(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textUnderlinePosition(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarBaseColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarBaseColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarFaceColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarFaceColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbar3dLightColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbar3dLightColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarShadowColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarShadowColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarHighlightColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarHighlightColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarDarkShadowColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarDarkShadowColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarArrowColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarArrowColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_scrollbarTrackColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_scrollbarTrackColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_writingMode(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_writingMode(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textAlignLast(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textAlignLast(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textKashidaSpace(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textKashidaSpace(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textOverflow(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textOverflow(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_minHeight(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MIN_HEIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_minHeight(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MIN_HEIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_msInterpolationMode(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_msInterpolationMode(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_maxHeight(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MAX_HEIGHT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_maxHeight(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MAX_HEIGHT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_minWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MIN_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_minWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MIN_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_maxWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_MAX_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_maxWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_MAX_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_content(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_content(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_captionSide(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_captionSide(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_counterIncrement(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_counterIncrement(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_counterReset(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_counterReset(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_outline(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_OUTLINE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_outline(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_OUTLINE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_outlineWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_outlineWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_outlineStyle(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_outlineStyle(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_outlineColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_outlineColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_boxSizing(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BOX_SIZING, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_boxSizing(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BOX_SIZING, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderSpacing(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BORDER_SPACING, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderSpacing(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BORDER_SPACING, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_orphans(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_orphans(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_widows(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_widows(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_pageBreakInside(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_pageBreakInside(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_emptyCells(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_emptyCells(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_msBlockProgression(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_msBlockProgression(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_quotes(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_quotes(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_alignmentBaseline(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_alignmentBaseline(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_baselineShift(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_baselineShift(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_dominantBaseline(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_dominantBaseline(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontSizeAdjust(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontSizeAdjust(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fontStretch(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fontStretch(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_opacity(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_OPACITY, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_opacity(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_OPACITY, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clipPath(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clipPath(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clipRule(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clipRule(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fill(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fill(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fillOpacity(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fillOpacity(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_fillRule(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_fillRule(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_kerning(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_kerning(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_marker(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_marker(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_markerEnd(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_markerEnd(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_markerMid(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_markerMid(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_markerStart(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_markerStart(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_mask(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_mask(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_pointerEvents(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_pointerEvents(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_stopColor(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_stopColor(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_stopOpacity(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_stopOpacity(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_stroke(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_stroke(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeDasharray(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeDasharray(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeDashoffset(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeDashoffset(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeLinecap(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeLinecap(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeLinejoin(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeLinejoin(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeMiterlimit(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeMiterlimit(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeOpacity(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeOpacity(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_strokeWidth(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_strokeWidth(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_textAnchor(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_textAnchor(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_glyphOrientationHorizontal(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_glyphOrientationHorizontal(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_glyphOrientationVertical(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_glyphOrientationVertical(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderRadius(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderRadius(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderTopLeftRadius(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderTopLeftRadius(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderTopRightRadius(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderTopRightRadius(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderBottomRightRadius(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderBottomRightRadius(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_borderBottomLeftRadius(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_borderBottomLeftRadius(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clipTop(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clipTop(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clipRight(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clipRight(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clipBottom(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_clipLeft(IHTMLCSSStyleDeclaration *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_clipLeft(IHTMLCSSStyleDeclaration *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_cssFloat(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_FLOAT, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_cssFloat(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_FLOAT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundClip(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND_CLIP, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundClip(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND_CLIP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundOrigin(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundOrigin(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_backgroundSize(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_BACKGROUND_SIZE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_backgroundSize(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_BACKGROUND_SIZE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_boxShadow(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_boxShadow(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_msTransform(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_MSTRANSFORM, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_msTransform(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_MSTRANSFORM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_put_msTransformOrigin(IHTMLCSSStyleDeclaration *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration_get_msTransformOrigin(IHTMLCSSStyleDeclaration *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCSSStyleDeclarationVtbl HTMLCSSStyleDeclarationVtbl = {
    HTMLCSSStyleDeclaration_QueryInterface,
    HTMLCSSStyleDeclaration_AddRef,
    HTMLCSSStyleDeclaration_Release,
    HTMLCSSStyleDeclaration_GetTypeInfoCount,
    HTMLCSSStyleDeclaration_GetTypeInfo,
    HTMLCSSStyleDeclaration_GetIDsOfNames,
    HTMLCSSStyleDeclaration_Invoke,
    HTMLCSSStyleDeclaration_get_length,
    HTMLCSSStyleDeclaration_get_parentRule,
    HTMLCSSStyleDeclaration_getPropertyValue,
    HTMLCSSStyleDeclaration_getPropertyPriority,
    HTMLCSSStyleDeclaration_removeProperty,
    HTMLCSSStyleDeclaration_setProperty,
    HTMLCSSStyleDeclaration_item,
    HTMLCSSStyleDeclaration_put_fontFamily,
    HTMLCSSStyleDeclaration_get_fontFamily,
    HTMLCSSStyleDeclaration_put_fontStyle,
    HTMLCSSStyleDeclaration_get_fontStyle,
    HTMLCSSStyleDeclaration_put_fontVariant,
    HTMLCSSStyleDeclaration_get_fontVariant,
    HTMLCSSStyleDeclaration_put_fontWeight,
    HTMLCSSStyleDeclaration_get_fontWeight,
    HTMLCSSStyleDeclaration_put_fontSize,
    HTMLCSSStyleDeclaration_get_fontSize,
    HTMLCSSStyleDeclaration_put_font,
    HTMLCSSStyleDeclaration_get_font,
    HTMLCSSStyleDeclaration_put_color,
    HTMLCSSStyleDeclaration_get_color,
    HTMLCSSStyleDeclaration_put_background,
    HTMLCSSStyleDeclaration_get_background,
    HTMLCSSStyleDeclaration_put_backgroundColor,
    HTMLCSSStyleDeclaration_get_backgroundColor,
    HTMLCSSStyleDeclaration_put_backgroundImage,
    HTMLCSSStyleDeclaration_get_backgroundImage,
    HTMLCSSStyleDeclaration_put_backgroundRepeat,
    HTMLCSSStyleDeclaration_get_backgroundRepeat,
    HTMLCSSStyleDeclaration_put_backgroundAttachment,
    HTMLCSSStyleDeclaration_get_backgroundAttachment,
    HTMLCSSStyleDeclaration_put_backgroundPosition,
    HTMLCSSStyleDeclaration_get_backgroundPosition,
    HTMLCSSStyleDeclaration_put_backgroundPositionX,
    HTMLCSSStyleDeclaration_get_backgroundPositionX,
    HTMLCSSStyleDeclaration_put_backgroundPositionY,
    HTMLCSSStyleDeclaration_get_backgroundPositionY,
    HTMLCSSStyleDeclaration_put_wordSpacing,
    HTMLCSSStyleDeclaration_get_wordSpacing,
    HTMLCSSStyleDeclaration_put_letterSpacing,
    HTMLCSSStyleDeclaration_get_letterSpacing,
    HTMLCSSStyleDeclaration_put_textDecoration,
    HTMLCSSStyleDeclaration_get_textDecoration,
    HTMLCSSStyleDeclaration_put_verticalAlign,
    HTMLCSSStyleDeclaration_get_verticalAlign,
    HTMLCSSStyleDeclaration_put_textTransform,
    HTMLCSSStyleDeclaration_get_textTransform,
    HTMLCSSStyleDeclaration_put_textAlign,
    HTMLCSSStyleDeclaration_get_textAlign,
    HTMLCSSStyleDeclaration_put_textIndent,
    HTMLCSSStyleDeclaration_get_textIndent,
    HTMLCSSStyleDeclaration_put_lineHeight,
    HTMLCSSStyleDeclaration_get_lineHeight,
    HTMLCSSStyleDeclaration_put_marginTop,
    HTMLCSSStyleDeclaration_get_marginTop,
    HTMLCSSStyleDeclaration_put_marginRight,
    HTMLCSSStyleDeclaration_get_marginRight,
    HTMLCSSStyleDeclaration_put_marginBottom,
    HTMLCSSStyleDeclaration_get_marginBottom,
    HTMLCSSStyleDeclaration_put_marginLeft,
    HTMLCSSStyleDeclaration_get_marginLeft,
    HTMLCSSStyleDeclaration_put_margin,
    HTMLCSSStyleDeclaration_get_margin,
    HTMLCSSStyleDeclaration_put_paddingTop,
    HTMLCSSStyleDeclaration_get_paddingTop,
    HTMLCSSStyleDeclaration_put_paddingRight,
    HTMLCSSStyleDeclaration_get_paddingRight,
    HTMLCSSStyleDeclaration_put_paddingBottom,
    HTMLCSSStyleDeclaration_get_paddingBottom,
    HTMLCSSStyleDeclaration_put_paddingLeft,
    HTMLCSSStyleDeclaration_get_paddingLeft,
    HTMLCSSStyleDeclaration_put_padding,
    HTMLCSSStyleDeclaration_get_padding,
    HTMLCSSStyleDeclaration_put_border,
    HTMLCSSStyleDeclaration_get_border,
    HTMLCSSStyleDeclaration_put_borderTop,
    HTMLCSSStyleDeclaration_get_borderTop,
    HTMLCSSStyleDeclaration_put_borderRight,
    HTMLCSSStyleDeclaration_get_borderRight,
    HTMLCSSStyleDeclaration_put_borderBottom,
    HTMLCSSStyleDeclaration_get_borderBottom,
    HTMLCSSStyleDeclaration_put_borderLeft,
    HTMLCSSStyleDeclaration_get_borderLeft,
    HTMLCSSStyleDeclaration_put_borderColor,
    HTMLCSSStyleDeclaration_get_borderColor,
    HTMLCSSStyleDeclaration_put_borderTopColor,
    HTMLCSSStyleDeclaration_get_borderTopColor,
    HTMLCSSStyleDeclaration_put_borderRightColor,
    HTMLCSSStyleDeclaration_get_borderRightColor,
    HTMLCSSStyleDeclaration_put_borderBottomColor,
    HTMLCSSStyleDeclaration_get_borderBottomColor,
    HTMLCSSStyleDeclaration_put_borderLeftColor,
    HTMLCSSStyleDeclaration_get_borderLeftColor,
    HTMLCSSStyleDeclaration_put_borderWidth,
    HTMLCSSStyleDeclaration_get_borderWidth,
    HTMLCSSStyleDeclaration_put_borderTopWidth,
    HTMLCSSStyleDeclaration_get_borderTopWidth,
    HTMLCSSStyleDeclaration_put_borderRightWidth,
    HTMLCSSStyleDeclaration_get_borderRightWidth,
    HTMLCSSStyleDeclaration_put_borderBottomWidth,
    HTMLCSSStyleDeclaration_get_borderBottomWidth,
    HTMLCSSStyleDeclaration_put_borderLeftWidth,
    HTMLCSSStyleDeclaration_get_borderLeftWidth,
    HTMLCSSStyleDeclaration_put_borderStyle,
    HTMLCSSStyleDeclaration_get_borderStyle,
    HTMLCSSStyleDeclaration_put_borderTopStyle,
    HTMLCSSStyleDeclaration_get_borderTopStyle,
    HTMLCSSStyleDeclaration_put_borderRightStyle,
    HTMLCSSStyleDeclaration_get_borderRightStyle,
    HTMLCSSStyleDeclaration_put_borderBottomStyle,
    HTMLCSSStyleDeclaration_get_borderBottomStyle,
    HTMLCSSStyleDeclaration_put_borderLeftStyle,
    HTMLCSSStyleDeclaration_get_borderLeftStyle,
    HTMLCSSStyleDeclaration_put_width,
    HTMLCSSStyleDeclaration_get_width,
    HTMLCSSStyleDeclaration_put_height,
    HTMLCSSStyleDeclaration_get_height,
    HTMLCSSStyleDeclaration_put_styleFloat,
    HTMLCSSStyleDeclaration_get_styleFloat,
    HTMLCSSStyleDeclaration_put_clear,
    HTMLCSSStyleDeclaration_get_clear,
    HTMLCSSStyleDeclaration_put_display,
    HTMLCSSStyleDeclaration_get_display,
    HTMLCSSStyleDeclaration_put_visibility,
    HTMLCSSStyleDeclaration_get_visibility,
    HTMLCSSStyleDeclaration_put_listStyleType,
    HTMLCSSStyleDeclaration_get_listStyleType,
    HTMLCSSStyleDeclaration_put_listStylePosition,
    HTMLCSSStyleDeclaration_get_listStylePosition,
    HTMLCSSStyleDeclaration_put_listStyleImage,
    HTMLCSSStyleDeclaration_get_listStyleImage,
    HTMLCSSStyleDeclaration_put_listStyle,
    HTMLCSSStyleDeclaration_get_listStyle,
    HTMLCSSStyleDeclaration_put_whiteSpace,
    HTMLCSSStyleDeclaration_get_whiteSpace,
    HTMLCSSStyleDeclaration_put_top,
    HTMLCSSStyleDeclaration_get_top,
    HTMLCSSStyleDeclaration_put_left,
    HTMLCSSStyleDeclaration_get_left,
    HTMLCSSStyleDeclaration_put_zIndex,
    HTMLCSSStyleDeclaration_get_zIndex,
    HTMLCSSStyleDeclaration_put_overflow,
    HTMLCSSStyleDeclaration_get_overflow,
    HTMLCSSStyleDeclaration_put_pageBreakBefore,
    HTMLCSSStyleDeclaration_get_pageBreakBefore,
    HTMLCSSStyleDeclaration_put_pageBreakAfter,
    HTMLCSSStyleDeclaration_get_pageBreakAfter,
    HTMLCSSStyleDeclaration_put_cssText,
    HTMLCSSStyleDeclaration_get_cssText,
    HTMLCSSStyleDeclaration_put_cursor,
    HTMLCSSStyleDeclaration_get_cursor,
    HTMLCSSStyleDeclaration_put_clip,
    HTMLCSSStyleDeclaration_get_clip,
    HTMLCSSStyleDeclaration_put_filter,
    HTMLCSSStyleDeclaration_get_filter,
    HTMLCSSStyleDeclaration_put_tableLayout,
    HTMLCSSStyleDeclaration_get_tableLayout,
    HTMLCSSStyleDeclaration_put_borderCollapse,
    HTMLCSSStyleDeclaration_get_borderCollapse,
    HTMLCSSStyleDeclaration_put_direction,
    HTMLCSSStyleDeclaration_get_direction,
    HTMLCSSStyleDeclaration_put_behavior,
    HTMLCSSStyleDeclaration_get_behavior,
    HTMLCSSStyleDeclaration_put_position,
    HTMLCSSStyleDeclaration_get_position,
    HTMLCSSStyleDeclaration_put_unicodeBidi,
    HTMLCSSStyleDeclaration_get_unicodeBidi,
    HTMLCSSStyleDeclaration_put_bottom,
    HTMLCSSStyleDeclaration_get_bottom,
    HTMLCSSStyleDeclaration_put_right,
    HTMLCSSStyleDeclaration_get_right,
    HTMLCSSStyleDeclaration_put_imeMode,
    HTMLCSSStyleDeclaration_get_imeMode,
    HTMLCSSStyleDeclaration_put_rubyAlign,
    HTMLCSSStyleDeclaration_get_rubyAlign,
    HTMLCSSStyleDeclaration_put_rubyPosition,
    HTMLCSSStyleDeclaration_get_rubyPosition,
    HTMLCSSStyleDeclaration_put_rubyOverhang,
    HTMLCSSStyleDeclaration_get_rubyOverhang,
    HTMLCSSStyleDeclaration_put_layoutGridChar,
    HTMLCSSStyleDeclaration_get_layoutGridChar,
    HTMLCSSStyleDeclaration_put_layoutGridLine,
    HTMLCSSStyleDeclaration_get_layoutGridLine,
    HTMLCSSStyleDeclaration_put_layoutGridMode,
    HTMLCSSStyleDeclaration_get_layoutGridMode,
    HTMLCSSStyleDeclaration_put_layoutGridType,
    HTMLCSSStyleDeclaration_get_layoutGridType,
    HTMLCSSStyleDeclaration_put_layoutGrid,
    HTMLCSSStyleDeclaration_get_layoutGrid,
    HTMLCSSStyleDeclaration_put_textAutospace,
    HTMLCSSStyleDeclaration_get_textAutospace,
    HTMLCSSStyleDeclaration_put_wordBreak,
    HTMLCSSStyleDeclaration_get_wordBreak,
    HTMLCSSStyleDeclaration_put_lineBreak,
    HTMLCSSStyleDeclaration_get_lineBreak,
    HTMLCSSStyleDeclaration_put_textJustify,
    HTMLCSSStyleDeclaration_get_textJustify,
    HTMLCSSStyleDeclaration_put_textJustifyTrim,
    HTMLCSSStyleDeclaration_get_textJustifyTrim,
    HTMLCSSStyleDeclaration_put_textKashida,
    HTMLCSSStyleDeclaration_get_textKashida,
    HTMLCSSStyleDeclaration_put_overflowX,
    HTMLCSSStyleDeclaration_get_overflowX,
    HTMLCSSStyleDeclaration_put_overflowY,
    HTMLCSSStyleDeclaration_get_overflowY,
    HTMLCSSStyleDeclaration_put_accelerator,
    HTMLCSSStyleDeclaration_get_accelerator,
    HTMLCSSStyleDeclaration_put_layoutFlow,
    HTMLCSSStyleDeclaration_get_layoutFlow,
    HTMLCSSStyleDeclaration_put_zoom,
    HTMLCSSStyleDeclaration_get_zoom,
    HTMLCSSStyleDeclaration_put_wordWrap,
    HTMLCSSStyleDeclaration_get_wordWrap,
    HTMLCSSStyleDeclaration_put_textUnderlinePosition,
    HTMLCSSStyleDeclaration_get_textUnderlinePosition,
    HTMLCSSStyleDeclaration_put_scrollbarBaseColor,
    HTMLCSSStyleDeclaration_get_scrollbarBaseColor,
    HTMLCSSStyleDeclaration_put_scrollbarFaceColor,
    HTMLCSSStyleDeclaration_get_scrollbarFaceColor,
    HTMLCSSStyleDeclaration_put_scrollbar3dLightColor,
    HTMLCSSStyleDeclaration_get_scrollbar3dLightColor,
    HTMLCSSStyleDeclaration_put_scrollbarShadowColor,
    HTMLCSSStyleDeclaration_get_scrollbarShadowColor,
    HTMLCSSStyleDeclaration_put_scrollbarHighlightColor,
    HTMLCSSStyleDeclaration_get_scrollbarHighlightColor,
    HTMLCSSStyleDeclaration_put_scrollbarDarkShadowColor,
    HTMLCSSStyleDeclaration_get_scrollbarDarkShadowColor,
    HTMLCSSStyleDeclaration_put_scrollbarArrowColor,
    HTMLCSSStyleDeclaration_get_scrollbarArrowColor,
    HTMLCSSStyleDeclaration_put_scrollbarTrackColor,
    HTMLCSSStyleDeclaration_get_scrollbarTrackColor,
    HTMLCSSStyleDeclaration_put_writingMode,
    HTMLCSSStyleDeclaration_get_writingMode,
    HTMLCSSStyleDeclaration_put_textAlignLast,
    HTMLCSSStyleDeclaration_get_textAlignLast,
    HTMLCSSStyleDeclaration_put_textKashidaSpace,
    HTMLCSSStyleDeclaration_get_textKashidaSpace,
    HTMLCSSStyleDeclaration_put_textOverflow,
    HTMLCSSStyleDeclaration_get_textOverflow,
    HTMLCSSStyleDeclaration_put_minHeight,
    HTMLCSSStyleDeclaration_get_minHeight,
    HTMLCSSStyleDeclaration_put_msInterpolationMode,
    HTMLCSSStyleDeclaration_get_msInterpolationMode,
    HTMLCSSStyleDeclaration_put_maxHeight,
    HTMLCSSStyleDeclaration_get_maxHeight,
    HTMLCSSStyleDeclaration_put_minWidth,
    HTMLCSSStyleDeclaration_get_minWidth,
    HTMLCSSStyleDeclaration_put_maxWidth,
    HTMLCSSStyleDeclaration_get_maxWidth,
    HTMLCSSStyleDeclaration_put_content,
    HTMLCSSStyleDeclaration_get_content,
    HTMLCSSStyleDeclaration_put_captionSide,
    HTMLCSSStyleDeclaration_get_captionSide,
    HTMLCSSStyleDeclaration_put_counterIncrement,
    HTMLCSSStyleDeclaration_get_counterIncrement,
    HTMLCSSStyleDeclaration_put_counterReset,
    HTMLCSSStyleDeclaration_get_counterReset,
    HTMLCSSStyleDeclaration_put_outline,
    HTMLCSSStyleDeclaration_get_outline,
    HTMLCSSStyleDeclaration_put_outlineWidth,
    HTMLCSSStyleDeclaration_get_outlineWidth,
    HTMLCSSStyleDeclaration_put_outlineStyle,
    HTMLCSSStyleDeclaration_get_outlineStyle,
    HTMLCSSStyleDeclaration_put_outlineColor,
    HTMLCSSStyleDeclaration_get_outlineColor,
    HTMLCSSStyleDeclaration_put_boxSizing,
    HTMLCSSStyleDeclaration_get_boxSizing,
    HTMLCSSStyleDeclaration_put_borderSpacing,
    HTMLCSSStyleDeclaration_get_borderSpacing,
    HTMLCSSStyleDeclaration_put_orphans,
    HTMLCSSStyleDeclaration_get_orphans,
    HTMLCSSStyleDeclaration_put_widows,
    HTMLCSSStyleDeclaration_get_widows,
    HTMLCSSStyleDeclaration_put_pageBreakInside,
    HTMLCSSStyleDeclaration_get_pageBreakInside,
    HTMLCSSStyleDeclaration_put_emptyCells,
    HTMLCSSStyleDeclaration_get_emptyCells,
    HTMLCSSStyleDeclaration_put_msBlockProgression,
    HTMLCSSStyleDeclaration_get_msBlockProgression,
    HTMLCSSStyleDeclaration_put_quotes,
    HTMLCSSStyleDeclaration_get_quotes,
    HTMLCSSStyleDeclaration_put_alignmentBaseline,
    HTMLCSSStyleDeclaration_get_alignmentBaseline,
    HTMLCSSStyleDeclaration_put_baselineShift,
    HTMLCSSStyleDeclaration_get_baselineShift,
    HTMLCSSStyleDeclaration_put_dominantBaseline,
    HTMLCSSStyleDeclaration_get_dominantBaseline,
    HTMLCSSStyleDeclaration_put_fontSizeAdjust,
    HTMLCSSStyleDeclaration_get_fontSizeAdjust,
    HTMLCSSStyleDeclaration_put_fontStretch,
    HTMLCSSStyleDeclaration_get_fontStretch,
    HTMLCSSStyleDeclaration_put_opacity,
    HTMLCSSStyleDeclaration_get_opacity,
    HTMLCSSStyleDeclaration_put_clipPath,
    HTMLCSSStyleDeclaration_get_clipPath,
    HTMLCSSStyleDeclaration_put_clipRule,
    HTMLCSSStyleDeclaration_get_clipRule,
    HTMLCSSStyleDeclaration_put_fill,
    HTMLCSSStyleDeclaration_get_fill,
    HTMLCSSStyleDeclaration_put_fillOpacity,
    HTMLCSSStyleDeclaration_get_fillOpacity,
    HTMLCSSStyleDeclaration_put_fillRule,
    HTMLCSSStyleDeclaration_get_fillRule,
    HTMLCSSStyleDeclaration_put_kerning,
    HTMLCSSStyleDeclaration_get_kerning,
    HTMLCSSStyleDeclaration_put_marker,
    HTMLCSSStyleDeclaration_get_marker,
    HTMLCSSStyleDeclaration_put_markerEnd,
    HTMLCSSStyleDeclaration_get_markerEnd,
    HTMLCSSStyleDeclaration_put_markerMid,
    HTMLCSSStyleDeclaration_get_markerMid,
    HTMLCSSStyleDeclaration_put_markerStart,
    HTMLCSSStyleDeclaration_get_markerStart,
    HTMLCSSStyleDeclaration_put_mask,
    HTMLCSSStyleDeclaration_get_mask,
    HTMLCSSStyleDeclaration_put_pointerEvents,
    HTMLCSSStyleDeclaration_get_pointerEvents,
    HTMLCSSStyleDeclaration_put_stopColor,
    HTMLCSSStyleDeclaration_get_stopColor,
    HTMLCSSStyleDeclaration_put_stopOpacity,
    HTMLCSSStyleDeclaration_get_stopOpacity,
    HTMLCSSStyleDeclaration_put_stroke,
    HTMLCSSStyleDeclaration_get_stroke,
    HTMLCSSStyleDeclaration_put_strokeDasharray,
    HTMLCSSStyleDeclaration_get_strokeDasharray,
    HTMLCSSStyleDeclaration_put_strokeDashoffset,
    HTMLCSSStyleDeclaration_get_strokeDashoffset,
    HTMLCSSStyleDeclaration_put_strokeLinecap,
    HTMLCSSStyleDeclaration_get_strokeLinecap,
    HTMLCSSStyleDeclaration_put_strokeLinejoin,
    HTMLCSSStyleDeclaration_get_strokeLinejoin,
    HTMLCSSStyleDeclaration_put_strokeMiterlimit,
    HTMLCSSStyleDeclaration_get_strokeMiterlimit,
    HTMLCSSStyleDeclaration_put_strokeOpacity,
    HTMLCSSStyleDeclaration_get_strokeOpacity,
    HTMLCSSStyleDeclaration_put_strokeWidth,
    HTMLCSSStyleDeclaration_get_strokeWidth,
    HTMLCSSStyleDeclaration_put_textAnchor,
    HTMLCSSStyleDeclaration_get_textAnchor,
    HTMLCSSStyleDeclaration_put_glyphOrientationHorizontal,
    HTMLCSSStyleDeclaration_get_glyphOrientationHorizontal,
    HTMLCSSStyleDeclaration_put_glyphOrientationVertical,
    HTMLCSSStyleDeclaration_get_glyphOrientationVertical,
    HTMLCSSStyleDeclaration_put_borderRadius,
    HTMLCSSStyleDeclaration_get_borderRadius,
    HTMLCSSStyleDeclaration_put_borderTopLeftRadius,
    HTMLCSSStyleDeclaration_get_borderTopLeftRadius,
    HTMLCSSStyleDeclaration_put_borderTopRightRadius,
    HTMLCSSStyleDeclaration_get_borderTopRightRadius,
    HTMLCSSStyleDeclaration_put_borderBottomRightRadius,
    HTMLCSSStyleDeclaration_get_borderBottomRightRadius,
    HTMLCSSStyleDeclaration_put_borderBottomLeftRadius,
    HTMLCSSStyleDeclaration_get_borderBottomLeftRadius,
    HTMLCSSStyleDeclaration_put_clipTop,
    HTMLCSSStyleDeclaration_get_clipTop,
    HTMLCSSStyleDeclaration_put_clipRight,
    HTMLCSSStyleDeclaration_get_clipRight,
    HTMLCSSStyleDeclaration_get_clipBottom,
    HTMLCSSStyleDeclaration_put_clipLeft,
    HTMLCSSStyleDeclaration_get_clipLeft,
    HTMLCSSStyleDeclaration_put_cssFloat,
    HTMLCSSStyleDeclaration_get_cssFloat,
    HTMLCSSStyleDeclaration_put_backgroundClip,
    HTMLCSSStyleDeclaration_get_backgroundClip,
    HTMLCSSStyleDeclaration_put_backgroundOrigin,
    HTMLCSSStyleDeclaration_get_backgroundOrigin,
    HTMLCSSStyleDeclaration_put_backgroundSize,
    HTMLCSSStyleDeclaration_get_backgroundSize,
    HTMLCSSStyleDeclaration_put_boxShadow,
    HTMLCSSStyleDeclaration_get_boxShadow,
    HTMLCSSStyleDeclaration_put_msTransform,
    HTMLCSSStyleDeclaration_get_msTransform,
    HTMLCSSStyleDeclaration_put_msTransformOrigin,
    HTMLCSSStyleDeclaration_get_msTransformOrigin
};

static inline CSSStyle *impl_from_IHTMLCSSStyleDeclaration2(IHTMLCSSStyleDeclaration2 *iface)
{
    return CONTAINING_RECORD(iface, CSSStyle, IHTMLCSSStyleDeclaration2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLCSSStyleDeclaration2, IHTMLCSSStyleDeclaration2,
                      impl_from_IHTMLCSSStyleDeclaration2(iface)->dispex)

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollChaining(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollChaining(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZooming(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZooming(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomSnapType(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomSnapType(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollRails(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollRails(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomChaining(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomChaining(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollSnapType(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollSnapType(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomLimit(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomLimit(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomSnap(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomSnap(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomSnapPoints(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomSnapPoints(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomLimitMin(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomLimitMin(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msContentZoomLimitMax(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msContentZoomLimitMax(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollSnapX(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollSnapX(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollSnapY(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollSnapY(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollSnapPointsX(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollSnapPointsX(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollSnapPointsY(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollSnapPointsY(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridColumn(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridColumn(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridColumnAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridColumnAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridColumns(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridColumns(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridColumnSpan(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridColumnSpan(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridRow(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridRow(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridRowAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridRowAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridRows(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridRows(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msGridRowSpan(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msGridRowSpan(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msWrapThrough(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msWrapThrough(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msWrapMargin(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msWrapMargin(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msWrapFlow(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msWrapFlow(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationName(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationName(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationDirection(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationDirection(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationPlayState(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationPlayState(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationIterationCount(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationIterationCount(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimation(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimation(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msAnimationFillMode(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msAnimationFillMode(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_colorInterpolationFilters(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_colorInterpolationFilters(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnCount(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_COLUMN_COUNT, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnCount(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property_var(This, STYLEID_COLUMN_COUNT, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnWidth(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_COLUMN_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnWidth(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property_var(This, STYLEID_COLUMN_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnGap(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_COLUMN_GAP, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnGap(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_COLUMN_GAP, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnFill(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_COLUMN_FILL, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnFill(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property(This, STYLEID_COLUMN_FILL, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnSpan(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_COLUMN_SPAN, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnSpan(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property(This, STYLEID_COLUMN_SPAN, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columns(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columns(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnRule(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_COLUMN_RULE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnRule(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property(This, STYLEID_COLUMN_RULE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnRuleColor(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_COLUMN_RULE_COLOR, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnRuleColor(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property_var(This, STYLEID_COLUMN_RULE_COLOR, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnRuleStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_COLUMN_RULE_STYLE, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnRuleStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property(This, STYLEID_COLUMN_RULE_STYLE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_columnRuleWidth(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_COLUMN_RULE_WIDTH, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_columnRuleWidth(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    WARN("(%p)->(%p) semi-stub\n", This, p);
    return get_style_property_var(This, STYLEID_COLUMN_RULE_WIDTH, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_breakBefore(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_breakBefore(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_breakAfter(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_breakAfter(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_breakInside(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_breakInside(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_floodColor(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_floodColor(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_floodOpacity(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_floodOpacity(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_lightingColor(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_lightingColor(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollLimitXMin(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollLimitXMin(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollLimitYMin(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollLimitYMin(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollLimitXMax(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollLimitXMax(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollLimitYMax(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollLimitYMax(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollLimit(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollLimit(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_textShadow(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_textShadow(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlowFrom(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlowFrom(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlowInto(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlowInto(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msHyphens(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msHyphens(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msHyphenateLimitZone(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msHyphenateLimitZone(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msHyphenateLimitChars(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msHyphenateLimitChars(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msHyphenateLimitLines(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msHyphenateLimitLines(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msHighContrastAdjust(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msHighContrastAdjust(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_enableBackground(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_enableBackground(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFontFeatureSettings(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFontFeatureSettings(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msUserSelect(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msUserSelect(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msOverflowStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msOverflowStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTransformStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTransformStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msBackfaceVisibility(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msBackfaceVisibility(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msPerspective(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msPerspective(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msPerspectiveOrigin(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msPerspectiveOrigin(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTransitionProperty(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTransitionProperty(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTransitionDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTransitionDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTransitionTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTransitionTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTransitionDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTransitionDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTransition(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_property(This, STYLEID_MSTRANSITION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTransition(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_property(This, STYLEID_MSTRANSITION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTouchAction(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTouchAction(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msScrollTranslation(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msScrollTranslation(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlex(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlex(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexPositive(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexPositive(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexNegative(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexNegative(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexPreferredSize(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexPreferredSize(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexFlow(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexFlow(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexDirection(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexDirection(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexWrap(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexWrap(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexItemAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexItemAlign(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexPack(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexPack(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexLinePack(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexLinePack(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msFlexOrder(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msFlexOrder(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_msTouchSelect(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_msTouchSelect(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transform(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_TRANSFORM, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transform(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_TRANSFORM, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transformOrigin(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transformOrigin(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transformStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transformStyle(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_backfaceVisibility(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_backfaceVisibility(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_perspective(IHTMLCSSStyleDeclaration2 *iface, VARIANT v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));
    return set_style_property_var(This, STYLEID_PERSPECTIVE, &v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_perspective(IHTMLCSSStyleDeclaration2 *iface, VARIANT *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property_var(This, STYLEID_PERSPECTIVE, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_perspectiveOrigin(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_perspectiveOrigin(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transitionProperty(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transitionProperty(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transitionDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transitionDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transitionTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transitionTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transitionDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transitionDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_transition(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_TRANSITION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_transition(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_TRANSITION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_fontFeatureSettings(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_fontFeatureSettings(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationName(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_ANIMATION_NAME, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationName(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_ANIMATION_NAME, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationDuration(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationTimingFunction(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationDelay(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationDirection(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationDirection(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationPlayState(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationPlayState(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationIterationCount(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationIterationCount(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animation(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_property(This, STYLEID_ANIMATION, v);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animation(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_property(This, STYLEID_ANIMATION, p);
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_put_animationFillMode(IHTMLCSSStyleDeclaration2 *iface, BSTR v)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCSSStyleDeclaration2_get_animationFillMode(IHTMLCSSStyleDeclaration2 *iface, BSTR *p)
{
    CSSStyle *This = impl_from_IHTMLCSSStyleDeclaration2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCSSStyleDeclaration2Vtbl HTMLCSSStyleDeclaration2Vtbl = {
    HTMLCSSStyleDeclaration2_QueryInterface,
    HTMLCSSStyleDeclaration2_AddRef,
    HTMLCSSStyleDeclaration2_Release,
    HTMLCSSStyleDeclaration2_GetTypeInfoCount,
    HTMLCSSStyleDeclaration2_GetTypeInfo,
    HTMLCSSStyleDeclaration2_GetIDsOfNames,
    HTMLCSSStyleDeclaration2_Invoke,
    HTMLCSSStyleDeclaration2_put_msScrollChaining,
    HTMLCSSStyleDeclaration2_get_msScrollChaining,
    HTMLCSSStyleDeclaration2_put_msContentZooming,
    HTMLCSSStyleDeclaration2_get_msContentZooming,
    HTMLCSSStyleDeclaration2_put_msContentZoomSnapType,
    HTMLCSSStyleDeclaration2_get_msContentZoomSnapType,
    HTMLCSSStyleDeclaration2_put_msScrollRails,
    HTMLCSSStyleDeclaration2_get_msScrollRails,
    HTMLCSSStyleDeclaration2_put_msContentZoomChaining,
    HTMLCSSStyleDeclaration2_get_msContentZoomChaining,
    HTMLCSSStyleDeclaration2_put_msScrollSnapType,
    HTMLCSSStyleDeclaration2_get_msScrollSnapType,
    HTMLCSSStyleDeclaration2_put_msContentZoomLimit,
    HTMLCSSStyleDeclaration2_get_msContentZoomLimit,
    HTMLCSSStyleDeclaration2_put_msContentZoomSnap,
    HTMLCSSStyleDeclaration2_get_msContentZoomSnap,
    HTMLCSSStyleDeclaration2_put_msContentZoomSnapPoints,
    HTMLCSSStyleDeclaration2_get_msContentZoomSnapPoints,
    HTMLCSSStyleDeclaration2_put_msContentZoomLimitMin,
    HTMLCSSStyleDeclaration2_get_msContentZoomLimitMin,
    HTMLCSSStyleDeclaration2_put_msContentZoomLimitMax,
    HTMLCSSStyleDeclaration2_get_msContentZoomLimitMax,
    HTMLCSSStyleDeclaration2_put_msScrollSnapX,
    HTMLCSSStyleDeclaration2_get_msScrollSnapX,
    HTMLCSSStyleDeclaration2_put_msScrollSnapY,
    HTMLCSSStyleDeclaration2_get_msScrollSnapY,
    HTMLCSSStyleDeclaration2_put_msScrollSnapPointsX,
    HTMLCSSStyleDeclaration2_get_msScrollSnapPointsX,
    HTMLCSSStyleDeclaration2_put_msScrollSnapPointsY,
    HTMLCSSStyleDeclaration2_get_msScrollSnapPointsY,
    HTMLCSSStyleDeclaration2_put_msGridColumn,
    HTMLCSSStyleDeclaration2_get_msGridColumn,
    HTMLCSSStyleDeclaration2_put_msGridColumnAlign,
    HTMLCSSStyleDeclaration2_get_msGridColumnAlign,
    HTMLCSSStyleDeclaration2_put_msGridColumns,
    HTMLCSSStyleDeclaration2_get_msGridColumns,
    HTMLCSSStyleDeclaration2_put_msGridColumnSpan,
    HTMLCSSStyleDeclaration2_get_msGridColumnSpan,
    HTMLCSSStyleDeclaration2_put_msGridRow,
    HTMLCSSStyleDeclaration2_get_msGridRow,
    HTMLCSSStyleDeclaration2_put_msGridRowAlign,
    HTMLCSSStyleDeclaration2_get_msGridRowAlign,
    HTMLCSSStyleDeclaration2_put_msGridRows,
    HTMLCSSStyleDeclaration2_get_msGridRows,
    HTMLCSSStyleDeclaration2_put_msGridRowSpan,
    HTMLCSSStyleDeclaration2_get_msGridRowSpan,
    HTMLCSSStyleDeclaration2_put_msWrapThrough,
    HTMLCSSStyleDeclaration2_get_msWrapThrough,
    HTMLCSSStyleDeclaration2_put_msWrapMargin,
    HTMLCSSStyleDeclaration2_get_msWrapMargin,
    HTMLCSSStyleDeclaration2_put_msWrapFlow,
    HTMLCSSStyleDeclaration2_get_msWrapFlow,
    HTMLCSSStyleDeclaration2_put_msAnimationName,
    HTMLCSSStyleDeclaration2_get_msAnimationName,
    HTMLCSSStyleDeclaration2_put_msAnimationDuration,
    HTMLCSSStyleDeclaration2_get_msAnimationDuration,
    HTMLCSSStyleDeclaration2_put_msAnimationTimingFunction,
    HTMLCSSStyleDeclaration2_get_msAnimationTimingFunction,
    HTMLCSSStyleDeclaration2_put_msAnimationDelay,
    HTMLCSSStyleDeclaration2_get_msAnimationDelay,
    HTMLCSSStyleDeclaration2_put_msAnimationDirection,
    HTMLCSSStyleDeclaration2_get_msAnimationDirection,
    HTMLCSSStyleDeclaration2_put_msAnimationPlayState,
    HTMLCSSStyleDeclaration2_get_msAnimationPlayState,
    HTMLCSSStyleDeclaration2_put_msAnimationIterationCount,
    HTMLCSSStyleDeclaration2_get_msAnimationIterationCount,
    HTMLCSSStyleDeclaration2_put_msAnimation,
    HTMLCSSStyleDeclaration2_get_msAnimation,
    HTMLCSSStyleDeclaration2_put_msAnimationFillMode,
    HTMLCSSStyleDeclaration2_get_msAnimationFillMode,
    HTMLCSSStyleDeclaration2_put_colorInterpolationFilters,
    HTMLCSSStyleDeclaration2_get_colorInterpolationFilters,
    HTMLCSSStyleDeclaration2_put_columnCount,
    HTMLCSSStyleDeclaration2_get_columnCount,
    HTMLCSSStyleDeclaration2_put_columnWidth,
    HTMLCSSStyleDeclaration2_get_columnWidth,
    HTMLCSSStyleDeclaration2_put_columnGap,
    HTMLCSSStyleDeclaration2_get_columnGap,
    HTMLCSSStyleDeclaration2_put_columnFill,
    HTMLCSSStyleDeclaration2_get_columnFill,
    HTMLCSSStyleDeclaration2_put_columnSpan,
    HTMLCSSStyleDeclaration2_get_columnSpan,
    HTMLCSSStyleDeclaration2_put_columns,
    HTMLCSSStyleDeclaration2_get_columns,
    HTMLCSSStyleDeclaration2_put_columnRule,
    HTMLCSSStyleDeclaration2_get_columnRule,
    HTMLCSSStyleDeclaration2_put_columnRuleColor,
    HTMLCSSStyleDeclaration2_get_columnRuleColor,
    HTMLCSSStyleDeclaration2_put_columnRuleStyle,
    HTMLCSSStyleDeclaration2_get_columnRuleStyle,
    HTMLCSSStyleDeclaration2_put_columnRuleWidth,
    HTMLCSSStyleDeclaration2_get_columnRuleWidth,
    HTMLCSSStyleDeclaration2_put_breakBefore,
    HTMLCSSStyleDeclaration2_get_breakBefore,
    HTMLCSSStyleDeclaration2_put_breakAfter,
    HTMLCSSStyleDeclaration2_get_breakAfter,
    HTMLCSSStyleDeclaration2_put_breakInside,
    HTMLCSSStyleDeclaration2_get_breakInside,
    HTMLCSSStyleDeclaration2_put_floodColor,
    HTMLCSSStyleDeclaration2_get_floodColor,
    HTMLCSSStyleDeclaration2_put_floodOpacity,
    HTMLCSSStyleDeclaration2_get_floodOpacity,
    HTMLCSSStyleDeclaration2_put_lightingColor,
    HTMLCSSStyleDeclaration2_get_lightingColor,
    HTMLCSSStyleDeclaration2_put_msScrollLimitXMin,
    HTMLCSSStyleDeclaration2_get_msScrollLimitXMin,
    HTMLCSSStyleDeclaration2_put_msScrollLimitYMin,
    HTMLCSSStyleDeclaration2_get_msScrollLimitYMin,
    HTMLCSSStyleDeclaration2_put_msScrollLimitXMax,
    HTMLCSSStyleDeclaration2_get_msScrollLimitXMax,
    HTMLCSSStyleDeclaration2_put_msScrollLimitYMax,
    HTMLCSSStyleDeclaration2_get_msScrollLimitYMax,
    HTMLCSSStyleDeclaration2_put_msScrollLimit,
    HTMLCSSStyleDeclaration2_get_msScrollLimit,
    HTMLCSSStyleDeclaration2_put_textShadow,
    HTMLCSSStyleDeclaration2_get_textShadow,
    HTMLCSSStyleDeclaration2_put_msFlowFrom,
    HTMLCSSStyleDeclaration2_get_msFlowFrom,
    HTMLCSSStyleDeclaration2_put_msFlowInto,
    HTMLCSSStyleDeclaration2_get_msFlowInto,
    HTMLCSSStyleDeclaration2_put_msHyphens,
    HTMLCSSStyleDeclaration2_get_msHyphens,
    HTMLCSSStyleDeclaration2_put_msHyphenateLimitZone,
    HTMLCSSStyleDeclaration2_get_msHyphenateLimitZone,
    HTMLCSSStyleDeclaration2_put_msHyphenateLimitChars,
    HTMLCSSStyleDeclaration2_get_msHyphenateLimitChars,
    HTMLCSSStyleDeclaration2_put_msHyphenateLimitLines,
    HTMLCSSStyleDeclaration2_get_msHyphenateLimitLines,
    HTMLCSSStyleDeclaration2_put_msHighContrastAdjust,
    HTMLCSSStyleDeclaration2_get_msHighContrastAdjust,
    HTMLCSSStyleDeclaration2_put_enableBackground,
    HTMLCSSStyleDeclaration2_get_enableBackground,
    HTMLCSSStyleDeclaration2_put_msFontFeatureSettings,
    HTMLCSSStyleDeclaration2_get_msFontFeatureSettings,
    HTMLCSSStyleDeclaration2_put_msUserSelect,
    HTMLCSSStyleDeclaration2_get_msUserSelect,
    HTMLCSSStyleDeclaration2_put_msOverflowStyle,
    HTMLCSSStyleDeclaration2_get_msOverflowStyle,
    HTMLCSSStyleDeclaration2_put_msTransformStyle,
    HTMLCSSStyleDeclaration2_get_msTransformStyle,
    HTMLCSSStyleDeclaration2_put_msBackfaceVisibility,
    HTMLCSSStyleDeclaration2_get_msBackfaceVisibility,
    HTMLCSSStyleDeclaration2_put_msPerspective,
    HTMLCSSStyleDeclaration2_get_msPerspective,
    HTMLCSSStyleDeclaration2_put_msPerspectiveOrigin,
    HTMLCSSStyleDeclaration2_get_msPerspectiveOrigin,
    HTMLCSSStyleDeclaration2_put_msTransitionProperty,
    HTMLCSSStyleDeclaration2_get_msTransitionProperty,
    HTMLCSSStyleDeclaration2_put_msTransitionDuration,
    HTMLCSSStyleDeclaration2_get_msTransitionDuration,
    HTMLCSSStyleDeclaration2_put_msTransitionTimingFunction,
    HTMLCSSStyleDeclaration2_get_msTransitionTimingFunction,
    HTMLCSSStyleDeclaration2_put_msTransitionDelay,
    HTMLCSSStyleDeclaration2_get_msTransitionDelay,
    HTMLCSSStyleDeclaration2_put_msTransition,
    HTMLCSSStyleDeclaration2_get_msTransition,
    HTMLCSSStyleDeclaration2_put_msTouchAction,
    HTMLCSSStyleDeclaration2_get_msTouchAction,
    HTMLCSSStyleDeclaration2_put_msScrollTranslation,
    HTMLCSSStyleDeclaration2_get_msScrollTranslation,
    HTMLCSSStyleDeclaration2_put_msFlex,
    HTMLCSSStyleDeclaration2_get_msFlex,
    HTMLCSSStyleDeclaration2_put_msFlexPositive,
    HTMLCSSStyleDeclaration2_get_msFlexPositive,
    HTMLCSSStyleDeclaration2_put_msFlexNegative,
    HTMLCSSStyleDeclaration2_get_msFlexNegative,
    HTMLCSSStyleDeclaration2_put_msFlexPreferredSize,
    HTMLCSSStyleDeclaration2_get_msFlexPreferredSize,
    HTMLCSSStyleDeclaration2_put_msFlexFlow,
    HTMLCSSStyleDeclaration2_get_msFlexFlow,
    HTMLCSSStyleDeclaration2_put_msFlexDirection,
    HTMLCSSStyleDeclaration2_get_msFlexDirection,
    HTMLCSSStyleDeclaration2_put_msFlexWrap,
    HTMLCSSStyleDeclaration2_get_msFlexWrap,
    HTMLCSSStyleDeclaration2_put_msFlexAlign,
    HTMLCSSStyleDeclaration2_get_msFlexAlign,
    HTMLCSSStyleDeclaration2_put_msFlexItemAlign,
    HTMLCSSStyleDeclaration2_get_msFlexItemAlign,
    HTMLCSSStyleDeclaration2_put_msFlexPack,
    HTMLCSSStyleDeclaration2_get_msFlexPack,
    HTMLCSSStyleDeclaration2_put_msFlexLinePack,
    HTMLCSSStyleDeclaration2_get_msFlexLinePack,
    HTMLCSSStyleDeclaration2_put_msFlexOrder,
    HTMLCSSStyleDeclaration2_get_msFlexOrder,
    HTMLCSSStyleDeclaration2_put_msTouchSelect,
    HTMLCSSStyleDeclaration2_get_msTouchSelect,
    HTMLCSSStyleDeclaration2_put_transform,
    HTMLCSSStyleDeclaration2_get_transform,
    HTMLCSSStyleDeclaration2_put_transformOrigin,
    HTMLCSSStyleDeclaration2_get_transformOrigin,
    HTMLCSSStyleDeclaration2_put_transformStyle,
    HTMLCSSStyleDeclaration2_get_transformStyle,
    HTMLCSSStyleDeclaration2_put_backfaceVisibility,
    HTMLCSSStyleDeclaration2_get_backfaceVisibility,
    HTMLCSSStyleDeclaration2_put_perspective,
    HTMLCSSStyleDeclaration2_get_perspective,
    HTMLCSSStyleDeclaration2_put_perspectiveOrigin,
    HTMLCSSStyleDeclaration2_get_perspectiveOrigin,
    HTMLCSSStyleDeclaration2_put_transitionProperty,
    HTMLCSSStyleDeclaration2_get_transitionProperty,
    HTMLCSSStyleDeclaration2_put_transitionDuration,
    HTMLCSSStyleDeclaration2_get_transitionDuration,
    HTMLCSSStyleDeclaration2_put_transitionTimingFunction,
    HTMLCSSStyleDeclaration2_get_transitionTimingFunction,
    HTMLCSSStyleDeclaration2_put_transitionDelay,
    HTMLCSSStyleDeclaration2_get_transitionDelay,
    HTMLCSSStyleDeclaration2_put_transition,
    HTMLCSSStyleDeclaration2_get_transition,
    HTMLCSSStyleDeclaration2_put_fontFeatureSettings,
    HTMLCSSStyleDeclaration2_get_fontFeatureSettings,
    HTMLCSSStyleDeclaration2_put_animationName,
    HTMLCSSStyleDeclaration2_get_animationName,
    HTMLCSSStyleDeclaration2_put_animationDuration,
    HTMLCSSStyleDeclaration2_get_animationDuration,
    HTMLCSSStyleDeclaration2_put_animationTimingFunction,
    HTMLCSSStyleDeclaration2_get_animationTimingFunction,
    HTMLCSSStyleDeclaration2_put_animationDelay,
    HTMLCSSStyleDeclaration2_get_animationDelay,
    HTMLCSSStyleDeclaration2_put_animationDirection,
    HTMLCSSStyleDeclaration2_get_animationDirection,
    HTMLCSSStyleDeclaration2_put_animationPlayState,
    HTMLCSSStyleDeclaration2_get_animationPlayState,
    HTMLCSSStyleDeclaration2_put_animationIterationCount,
    HTMLCSSStyleDeclaration2_get_animationIterationCount,
    HTMLCSSStyleDeclaration2_put_animation,
    HTMLCSSStyleDeclaration2_get_animation,
    HTMLCSSStyleDeclaration2_put_animationFillMode,
    HTMLCSSStyleDeclaration2_get_animationFillMode
};

static inline CSSStyle *impl_from_DispatchEx(DispatchEx *dispex)
{
    return CONTAINING_RECORD(dispex, CSSStyle, dispex);
}

void *CSSStyle_query_interface(DispatchEx *dispex, REFIID riid)
{
    CSSStyle *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLCSSStyleDeclaration, riid))
        return &This->IHTMLCSSStyleDeclaration_iface;
    if(IsEqualGUID(&IID_IHTMLCSSStyleDeclaration2, riid))
        return &This->IHTMLCSSStyleDeclaration2_iface;

    return NULL;
}

void CSSStyle_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    CSSStyle *This = impl_from_DispatchEx(dispex);
    if(This->nsstyle)
        note_cc_edge((nsISupports*)This->nsstyle, "nsstyle", cb);
}

void CSSStyle_unlink(DispatchEx *dispex)
{
    CSSStyle *This = impl_from_DispatchEx(dispex);
    unlink_ref(&This->nsstyle);
}

void CSSStyle_destructor(DispatchEx *dispex)
{
    CSSStyle *This = impl_from_DispatchEx(dispex);
    free(This);
}

HRESULT CSSStyle_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *dispid)
{
    CSSStyle *This = impl_from_DispatchEx(dispex);
    const style_tbl_entry_t *style_entry;

    style_entry = lookup_style_tbl(This, name, ATTR_BUILTIN_NAME);
    if(style_entry) {
        DISPID id = dispex_compat_mode(dispex) >= COMPAT_MODE_IE9
            ? style_entry->dispid : style_entry->compat_dispid;
        if(id == DISPID_UNKNOWN)
            return DISP_E_UNKNOWNNAME;

        *dispid = id;
        return S_OK;
    }

    return DISP_E_UNKNOWNNAME;
}

static inline HTMLStyle *HTMLStyle_from_DispatchEx(DispatchEx *dispex)
{
    return CONTAINING_RECORD(dispex, HTMLStyle, css_style.dispex);
}

static void *HTMLStyle_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLStyle *This = HTMLStyle_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLStyle, riid))
        return &This->IHTMLStyle_iface;
    if(IsEqualGUID(&IID_IHTMLStyle2, riid))
        return &This->IHTMLStyle2_iface;
    if(IsEqualGUID(&IID_IHTMLStyle3, riid))
        return &This->IHTMLStyle3_iface;
    if(IsEqualGUID(&IID_IHTMLStyle4, riid))
        return &This->IHTMLStyle4_iface;
    if(IsEqualGUID(&IID_IHTMLStyle5, riid))
        return &This->IHTMLStyle5_iface;
    if(IsEqualGUID(&IID_IHTMLStyle6, riid))
        return &This->IHTMLStyle6_iface;
    if(IsEqualGUID(&IID_IWineCSSProperties, riid))
        return &This->IWineCSSProperties_iface;
    return CSSStyle_query_interface(&This->css_style.dispex, riid);
}

static void HTMLStyle_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLStyle *This = HTMLStyle_from_DispatchEx(dispex);
    CSSStyle_traverse(&This->css_style.dispex, cb);

    if(This->elem)
        note_cc_edge((nsISupports*)&This->elem->node.IHTMLDOMNode_iface, "elem", cb);
}

static void HTMLStyle_unlink(DispatchEx *dispex)
{
    HTMLStyle *This = HTMLStyle_from_DispatchEx(dispex);
    CSSStyle_unlink(&This->css_style.dispex);

    if(This->elem) {
        HTMLElement *elem = This->elem;
        This->elem = NULL;
        IHTMLDOMNode_Release(&elem->node.IHTMLDOMNode_iface);
    }
}

void MSCSSProperties_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t styledecl_ie11_hooks[] = {
        {DISPID_IHTMLCSSSTYLEDECLARATION_BEHAVIOR},

        /* IE10 and below */
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPTOP},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPRIGHT},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPBOTTOM},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPLEFT},
        {DISPID_UNKNOWN}
    };
    const dispex_hook_t *const styledecl_hooks = styledecl_ie11_hooks + 1;
    if(mode >= COMPAT_MODE_IE9) {
        dispex_info_add_interface(info, IHTMLCSSStyleDeclaration_tid, mode >= COMPAT_MODE_IE11 ? styledecl_ie11_hooks : styledecl_hooks);
        dispex_info_add_interface(info, IWineCSSProperties_tid, NULL);
    }
    if(mode >= COMPAT_MODE_IE10)
        dispex_info_add_interface(info, IHTMLCSSStyleDeclaration2_tid, NULL);
}

dispex_static_data_t MSCSSProperties_dispex = {
    .id           = PROT_MSCSSProperties,
    .prototype_id = PROT_CSSStyleDeclaration,
    .init_info    = MSCSSProperties_init_dispex_info,
};

static void MSStyleCSSProperties_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t style_ie9_hooks[] = {
        {DISPID_IHTMLSTYLE_TOSTRING},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t style2_ie11_hooks[] = {
        {DISPID_IHTMLSTYLE2_BEHAVIOR},

        /* IE9+ */
        {DISPID_IHTMLSTYLE2_SETEXPRESSION},
        {DISPID_IHTMLSTYLE2_GETEXPRESSION},
        {DISPID_IHTMLSTYLE2_REMOVEEXPRESSION},
        {DISPID_UNKNOWN}
    };
    const dispex_hook_t *const style2_ie9_hooks = style2_ie11_hooks + 1;

    MSCSSProperties_init_dispex_info(info, mode);
    dispex_info_add_interface(info, IHTMLStyle2_tid, mode >= COMPAT_MODE_IE11 ? style2_ie11_hooks :
                                                     mode >= COMPAT_MODE_IE9  ? style2_ie9_hooks  : NULL);
    dispex_info_add_interface(info, IHTMLStyle_tid, mode >= COMPAT_MODE_IE9 ? style_ie9_hooks : NULL);
}

static const dispex_static_data_vtbl_t MSStyleCSSProperties_dispex_vtbl = {
    CSSSTYLE_DISPEX_VTBL_ENTRIES,
    .query_interface   = HTMLStyle_query_interface,
    .traverse          = HTMLStyle_traverse,
    .unlink            = HTMLStyle_unlink
};

static const tid_t MSStyleCSSProperties_iface_tids[] = {
    IHTMLStyle6_tid,
    IHTMLStyle5_tid,
    IHTMLStyle4_tid,
    IHTMLStyle3_tid,
    0
};
dispex_static_data_t MSStyleCSSProperties_dispex = {
    .id           = PROT_MSStyleCSSProperties,
    .prototype_id = PROT_MSCSSProperties,
    .vtbl         = &MSStyleCSSProperties_dispex_vtbl,
    .disp_tid     = DispHTMLStyle_tid,
    .iface_tids   = MSStyleCSSProperties_iface_tids,
    .init_info    = MSStyleCSSProperties_init_dispex_info,
};

static HRESULT get_style_from_elem(HTMLElement *elem, nsIDOMCSSStyleDeclaration **ret)
{
    nsIDOMElementCSSInlineStyle *nselemstyle;
    nsIDOMSVGElement *svg_element;
    nsresult nsres;

    if(!elem->dom_element) {
        FIXME("comment element\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMElement_QueryInterface(elem->dom_element, &IID_nsIDOMElementCSSInlineStyle,
            (void**)&nselemstyle);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIDOMElementCSSInlineStyle_GetStyle(nselemstyle, ret);
        nsIDOMElementCSSInlineStyle_Release(nselemstyle);
        if(NS_FAILED(nsres)) {
            ERR("GetStyle failed: %08lx\n", nsres);
            return E_FAIL;
        }
        return S_OK;
    }

    nsres = nsIDOMElement_QueryInterface(elem->dom_element, &IID_nsIDOMSVGElement, (void**)&svg_element);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIDOMSVGElement_GetStyle(svg_element, ret);
        nsIDOMSVGElement_Release(svg_element);
        if(NS_FAILED(nsres)) {
            ERR("GetStyle failed: %08lx\n", nsres);
            return E_FAIL;
        }
        return S_OK;
    }

    FIXME("Unsupported element type\n");
    return E_NOTIMPL;
}

void init_css_style(CSSStyle *style, nsIDOMCSSStyleDeclaration *nsstyle, dispex_static_data_t *dispex_info, DispatchEx *owner)
{
    style->IHTMLCSSStyleDeclaration_iface.lpVtbl = &HTMLCSSStyleDeclarationVtbl;
    style->IHTMLCSSStyleDeclaration2_iface.lpVtbl = &HTMLCSSStyleDeclaration2Vtbl;
    style->nsstyle = nsstyle;
    nsIDOMCSSStyleDeclaration_AddRef(nsstyle);

    init_dispatch_with_owner(&style->dispex, dispex_info, owner);
}

HRESULT HTMLStyle_Create(HTMLElement *elem, HTMLStyle **ret)
{
    nsIDOMCSSStyleDeclaration *nsstyle;
    HTMLStyle *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &nsstyle);
    if(FAILED(hres))
        return hres;

    style = calloc(1, sizeof(HTMLStyle));
    if(!style) {
        nsIDOMCSSStyleDeclaration_Release(nsstyle);
        return E_OUTOFMEMORY;
    }

    style->IHTMLStyle_iface.lpVtbl = &HTMLStyleVtbl;
    style->IHTMLStyle2_iface.lpVtbl = &HTMLStyle2Vtbl;
    style->IHTMLStyle3_iface.lpVtbl = &HTMLStyle3Vtbl;
    style->IHTMLStyle4_iface.lpVtbl = &HTMLStyle4Vtbl;
    style->IHTMLStyle5_iface.lpVtbl = &HTMLStyle5Vtbl;
    style->IHTMLStyle6_iface.lpVtbl = &HTMLStyle6Vtbl;
    style->IWineCSSProperties_iface.lpVtbl = &HTMLStyle_CSSPropertiesVtbl;

    style->elem = elem;
    IHTMLDOMNode_AddRef(&elem->node.IHTMLDOMNode_iface);

    init_css_style(&style->css_style, nsstyle, &MSStyleCSSProperties_dispex, &elem->node.event_target.dispex);
    nsIDOMCSSStyleDeclaration_Release(nsstyle);

    *ret = style;
    return S_OK;
}

static void CSSStyleDeclaration_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t styledecl_hooks[] = {
        {DISPID_IHTMLCSSSTYLEDECLARATION_FILTER},

        /* IE10+ */
        {DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDPOSITIONX},
        {DISPID_IHTMLCSSSTYLEDECLARATION_BACKGROUNDPOSITIONY},
        {DISPID_IHTMLCSSSTYLEDECLARATION_STYLEFLOAT},
        {DISPID_IHTMLCSSSTYLEDECLARATION_BEHAVIOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_IMEMODE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LAYOUTGRIDCHAR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LAYOUTGRIDLINE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LAYOUTGRIDMODE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LAYOUTGRIDTYPE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LAYOUTGRID},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LINEBREAK},
        {DISPID_IHTMLCSSSTYLEDECLARATION_TEXTJUSTIFYTRIM},
        {DISPID_IHTMLCSSSTYLEDECLARATION_TEXTKASHIDA},
        {DISPID_IHTMLCSSSTYLEDECLARATION_TEXTAUTOSPACE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_ACCELERATOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_LAYOUTFLOW},
        {DISPID_IHTMLCSSSTYLEDECLARATION_ZOOM},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARBASECOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARFACECOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBAR3DLIGHTCOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARSHADOWCOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARHIGHLIGHTCOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARDARKSHADOWCOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARARROWCOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_SCROLLBARTRACKCOLOR},
        {DISPID_IHTMLCSSSTYLEDECLARATION_WRITINGMODE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_TEXTKASHIDASPACE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_MSINTERPOLATIONMODE},
        {DISPID_IHTMLCSSSTYLEDECLARATION_MSBLOCKPROGRESSION},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPTOP},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPRIGHT},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPBOTTOM},
        {DISPID_IHTMLCSSSTYLEDECLARATION_CLIPLEFT},
        {DISPID_UNKNOWN}
    };
    const dispex_hook_t *const styledecl_ie10_hooks = styledecl_hooks + 1;

    dispex_info_add_interface(info, IHTMLCSSStyleDeclaration_tid, mode >= COMPAT_MODE_IE10 ? styledecl_ie10_hooks : styledecl_hooks);
    if(mode >= COMPAT_MODE_IE10)
        dispex_info_add_interface(info, IHTMLCSSStyleDeclaration2_tid, NULL);
}

static const dispex_static_data_vtbl_t CSSStyleDeclaration_dispex_vtbl = {
    CSSSTYLE_DISPEX_VTBL_ENTRIES,
    .query_interface   = CSSStyle_query_interface,
    .traverse          = CSSStyle_traverse,
    .unlink            = CSSStyle_unlink
};

dispex_static_data_t CSSStyleDeclaration_dispex = {
    .id        = PROT_CSSStyleDeclaration,
    .vtbl      = &CSSStyleDeclaration_dispex_vtbl,
    .disp_tid  = DispHTMLW3CComputedStyle_tid,
    .init_info = CSSStyleDeclaration_init_dispex_info,
};

HRESULT create_computed_style(nsIDOMCSSStyleDeclaration *nsstyle, DispatchEx *owner, IHTMLCSSStyleDeclaration **p)
{
    CSSStyle *style;

    if(!(style = calloc(1, sizeof(*style))))
        return E_OUTOFMEMORY;

    init_css_style(style, nsstyle, &CSSStyleDeclaration_dispex, owner);
    *p = &style->IHTMLCSSStyleDeclaration_iface;
    return S_OK;
}

HRESULT get_elem_style(HTMLElement *elem, styleid_t styleid, BSTR *ret)
{
    nsIDOMCSSStyleDeclaration *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &style);
    if(FAILED(hres))
        return hres;

    hres = get_nsstyle_property(style, styleid, COMPAT_MODE_IE11, ret);
    nsIDOMCSSStyleDeclaration_Release(style);
    return hres;
}

HRESULT set_elem_style(HTMLElement *elem, styleid_t styleid, const WCHAR *val)
{
    nsIDOMCSSStyleDeclaration *style;
    nsAString value_str;
    HRESULT hres;

    hres = get_style_from_elem(elem, &style);
    if(FAILED(hres))
        return hres;

    nsAString_InitDepend(&value_str, val);
    hres = set_nsstyle_property(style, styleid, &value_str);
    nsAString_Finish(&value_str);
    nsIDOMCSSStyleDeclaration_Release(style);
    return hres;
}
