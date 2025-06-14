/*
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
 * Copyright (C) 2008 by Reece H. Dunn
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
 *
 * TODO
 *  Styles
 *  - BS_NOTIFY: is it complete?
 *  - BS_RIGHTBUTTON: same as BS_LEFTTEXT
 *
 *  Messages
 *  - WM_CHAR: Checks a (manual or automatic) check box on '+' or '=', clears it on '-' key.
 *  - WM_SETFOCUS: For (manual or automatic) radio buttons, send the parent window BN_CLICKED
 *  - WM_NCCREATE: Turns any BS_OWNERDRAW button into a BS_PUSHBUTTON button.
 *  - WM_SYSKEYUP
 *
 *  Notifications
 *  - BCN_HOTITEMCHANGE
 *  - BN_DISABLE
 *  - BN_PUSHED/BN_HILITE
 *  + BN_KILLFOCUS: is it OK?
 *  - BN_PAINT
 *  + BN_SETFOCUS: is it OK?
 *  - BN_UNPUSHED/BN_UNHILITE
 *
 *  Structures/Macros/Definitions
 *  - NMBCHOTITEM
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define OEMRESOURCE

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/debug.h"
#include "wine/heap.h"

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(button);

/* undocumented flags */
#define BUTTON_NSTATES         0x0F
#define BUTTON_BTNPRESSED      0x40
#define BUTTON_UNKNOWN2        0x20
#define BUTTON_UNKNOWN3        0x10
#ifdef __REACTOS__
#define BUTTON_BMCLICK         0x100 // ReactOS Need to up to wine!
#endif

#define BUTTON_NOTIFY_PARENT(hWnd, code) \
    do { /* Notify parent which has created this button control */ \
        TRACE("notification " #code " sent to hwnd=%p\n", GetParent(hWnd)); \
        SendMessageW(GetParent(hWnd), WM_COMMAND, \
                     MAKEWPARAM(GetWindowLongPtrW((hWnd),GWLP_ID), (code)), \
                     (LPARAM)(hWnd)); \
    } while(0)

typedef struct _BUTTON_INFO
{
    HWND             hwnd;
    HWND             parent;
    LONG             style;
    LONG             state;
    HFONT            font;
    WCHAR           *note;
    INT              note_length;
    DWORD            image_type; /* IMAGE_BITMAP or IMAGE_ICON */
    BUTTON_IMAGELIST imagelist;
    UINT             split_style;
    HIMAGELIST       glyph;      /* this is a font character code when split_style doesn't have BCSS_IMAGE */
    SIZE             glyph_size;
    RECT             text_margin;
    union
    {
        HICON   icon;
        HBITMAP bitmap;
        HANDLE  image;
    } u;

#ifdef __REACTOS__
    DWORD ui_state;
#endif
} BUTTON_INFO;

static UINT BUTTON_CalcLayoutRects( const BUTTON_INFO *infoPtr, HDC hdc, RECT *labelRc, RECT *imageRc, RECT *textRc );
static void PB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void CB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void GB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void UB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void OB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void SB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void BUTTON_CheckAutoRadioButton( HWND hwnd );
static void get_split_button_rects(const BUTTON_INFO*, const RECT*, RECT*, RECT*);
static BOOL notify_split_button_dropdown(const BUTTON_INFO*, const POINT*, HWND);
static void draw_split_button_dropdown_glyph(const BUTTON_INFO*, HDC, RECT*);

#define MAX_BTN_TYPE  16

static const WORD maxCheckState[MAX_BTN_TYPE] =
{
    BST_UNCHECKED,      /* BS_PUSHBUTTON */
    BST_UNCHECKED,      /* BS_DEFPUSHBUTTON */
    BST_CHECKED,        /* BS_CHECKBOX */
    BST_CHECKED,        /* BS_AUTOCHECKBOX */
    BST_CHECKED,        /* BS_RADIOBUTTON */
    BST_INDETERMINATE,  /* BS_3STATE */
    BST_INDETERMINATE,  /* BS_AUTO3STATE */
    BST_UNCHECKED,      /* BS_GROUPBOX */
    BST_UNCHECKED,      /* BS_USERBUTTON */
    BST_CHECKED,        /* BS_AUTORADIOBUTTON */
    BST_UNCHECKED,      /* BS_PUSHBOX */
    BST_UNCHECKED,      /* BS_OWNERDRAW */
    BST_UNCHECKED,      /* BS_SPLITBUTTON */
    BST_UNCHECKED,      /* BS_DEFSPLITBUTTON */
    BST_UNCHECKED,      /* BS_COMMANDLINK */
    BST_UNCHECKED       /* BS_DEFCOMMANDLINK */
};

/* Generic draw states, use get_draw_state() to get specific state for button type */
enum draw_state
{
    STATE_NORMAL,
    STATE_DISABLED,
    STATE_HOT,
    STATE_PRESSED,
    STATE_DEFAULTED,
    DRAW_STATE_COUNT
};

typedef void (*pfPaint)( const BUTTON_INFO *infoPtr, HDC hdc, UINT action );

static const pfPaint btnPaintFunc[MAX_BTN_TYPE] =
{
    PB_Paint,    /* BS_PUSHBUTTON */
    PB_Paint,    /* BS_DEFPUSHBUTTON */
    CB_Paint,    /* BS_CHECKBOX */
    CB_Paint,    /* BS_AUTOCHECKBOX */
    CB_Paint,    /* BS_RADIOBUTTON */
    CB_Paint,    /* BS_3STATE */
    CB_Paint,    /* BS_AUTO3STATE */
    GB_Paint,    /* BS_GROUPBOX */
    UB_Paint,    /* BS_USERBUTTON */
    CB_Paint,    /* BS_AUTORADIOBUTTON */
    NULL,        /* BS_PUSHBOX */
    OB_Paint,    /* BS_OWNERDRAW */
    SB_Paint,    /* BS_SPLITBUTTON */
    SB_Paint,    /* BS_DEFSPLITBUTTON */
    PB_Paint,    /* BS_COMMANDLINK */
    PB_Paint     /* BS_DEFCOMMANDLINK */
};


#ifdef __REACTOS__ /* r73885 */
typedef void (*pfThemedPaint)( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);

static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);
static void CB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);
static void GB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);
static void SB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);

#else
typedef void (*pfThemedPaint)( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);

static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void CB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void GB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void SB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);

#endif

static const pfThemedPaint btnThemedPaintFunc[MAX_BTN_TYPE] =
{
    PB_ThemedPaint, /* BS_PUSHBUTTON */
    PB_ThemedPaint, /* BS_DEFPUSHBUTTON */
    CB_ThemedPaint, /* BS_CHECKBOX */
    CB_ThemedPaint, /* BS_AUTOCHECKBOX */
    CB_ThemedPaint, /* BS_RADIOBUTTON */
    CB_ThemedPaint, /* BS_3STATE */
    CB_ThemedPaint, /* BS_AUTO3STATE */
    GB_ThemedPaint, /* BS_GROUPBOX */
    NULL,           /* BS_USERBUTTON */
    CB_ThemedPaint, /* BS_AUTORADIOBUTTON */
    NULL,           /* BS_PUSHBOX */
    NULL,           /* BS_OWNERDRAW */
    SB_ThemedPaint, /* BS_SPLITBUTTON */
    SB_ThemedPaint, /* BS_DEFSPLITBUTTON */
    NULL,           /* BS_COMMANDLINK */
    NULL,           /* BS_DEFCOMMANDLINK */
};

typedef BOOL (*pfGetIdealSize)(BUTTON_INFO *infoPtr, SIZE *size);

static BOOL PB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL CB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL GB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL SB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);

static const pfGetIdealSize btnGetIdealSizeFunc[MAX_BTN_TYPE] = {
    PB_GetIdealSize, /* BS_PUSHBUTTON */
    PB_GetIdealSize, /* BS_DEFPUSHBUTTON */
    CB_GetIdealSize, /* BS_CHECKBOX */
    CB_GetIdealSize, /* BS_AUTOCHECKBOX */
    CB_GetIdealSize, /* BS_RADIOBUTTON */
    GB_GetIdealSize, /* BS_3STATE */
    GB_GetIdealSize, /* BS_AUTO3STATE */
    GB_GetIdealSize, /* BS_GROUPBOX */
    PB_GetIdealSize, /* BS_USERBUTTON */
    CB_GetIdealSize, /* BS_AUTORADIOBUTTON */
    GB_GetIdealSize, /* BS_PUSHBOX */
    GB_GetIdealSize, /* BS_OWNERDRAW */
    SB_GetIdealSize, /* BS_SPLITBUTTON */
    SB_GetIdealSize, /* BS_DEFSPLITBUTTON */
    /* GetIdealSize() for following types are unimplemented, use BS_PUSHBUTTON's for now */
    PB_GetIdealSize, /* BS_COMMANDLINK */
    PB_GetIdealSize  /* BS_DEFCOMMANDLINK */
};

static inline UINT get_button_type( LONG window_style )
{
    return (window_style & BS_TYPEMASK);
}

static inline BOOL button_centers_text( LONG window_style )
{
    /* Push button's text is centered by default, same for split buttons */
    UINT type = get_button_type(window_style);
    return type <= BS_DEFPUSHBUTTON || type == BS_SPLITBUTTON || type == BS_DEFSPLITBUTTON;
}

#ifndef __REACTOS__
/* paint a button of any type */
static inline void paint_button( BUTTON_INFO *infoPtr, LONG style, UINT action )
{
    if (btnPaintFunc[style] && IsWindowVisible(infoPtr->hwnd))
    {
        HDC hdc = GetDC( infoPtr->hwnd );
        btnPaintFunc[style]( infoPtr, hdc, action );
        ReleaseDC( infoPtr->hwnd, hdc );
    }
}
#endif

/* retrieve the button text; returned buffer must be freed by caller */
static inline WCHAR *get_button_text( const BUTTON_INFO *infoPtr )
{
    INT len = GetWindowTextLengthW( infoPtr->hwnd );
    WCHAR *buffer = heap_alloc( (len + 1) * sizeof(WCHAR) );
    if (buffer)
        GetWindowTextW( infoPtr->hwnd, buffer, len + 1 );
    return buffer;
}

/* get the default glyph size for split buttons */
static LONG get_default_glyph_size(const BUTTON_INFO *infoPtr)
{
    if (infoPtr->split_style & BCSS_IMAGE)
    {
        /* Size it to fit, including the left and right edges */
        int w, h;
        if (!ImageList_GetIconSize(infoPtr->glyph, &w, &h)) w = 0;
        return w + GetSystemMetrics(SM_CXEDGE) * 2;
    }

    /* The glyph size relies on the default menu font's cell height */
    return GetSystemMetrics(SM_CYMENUCHECK);
}

static void init_custom_draw(NMCUSTOMDRAW *nmcd, const BUTTON_INFO *infoPtr, HDC hdc, const RECT *rc)
{
    nmcd->hdr.hwndFrom = infoPtr->hwnd;
    nmcd->hdr.idFrom   = GetWindowLongPtrW(infoPtr->hwnd, GWLP_ID);
    nmcd->hdr.code     = NM_CUSTOMDRAW;
    nmcd->hdc          = hdc;
    nmcd->rc           = *rc;
    nmcd->dwDrawStage  = CDDS_PREERASE;
    nmcd->dwItemSpec   = 0;
    nmcd->lItemlParam  = 0;
    nmcd->uItemState   = IsWindowEnabled(infoPtr->hwnd) ? 0 : CDIS_DISABLED;
    if (infoPtr->state & BST_PUSHED)  nmcd->uItemState |= CDIS_SELECTED;
    if (infoPtr->state & BST_FOCUS)   nmcd->uItemState |= CDIS_FOCUS;
    if (infoPtr->state & BST_HOT)     nmcd->uItemState |= CDIS_HOT;
    if (infoPtr->state & BST_INDETERMINATE)
        nmcd->uItemState |= CDIS_INDETERMINATE;

    /* Windows doesn't seem to send CDIS_CHECKED (it fails the tests) */
    /* CDIS_SHOWKEYBOARDCUES is misleading, as the meaning is reversed */
    /* FIXME: Handle it properly when we support keyboard cues? */
}

HRGN set_control_clipping( HDC hdc, const RECT *rect )
{
    RECT rc = *rect;
    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );

    if (GetClipRgn( hdc, hrgn ) != 1)
    {
        DeleteObject( hrgn );
        hrgn = 0;
    }
    DPtoLP( hdc, (POINT *)&rc, 2 );
    if (GetLayout( hdc ) & LAYOUT_RTL)  /* compensate for the shifting done by IntersectClipRect */
    {
        rc.left++;
        rc.right++;
    }
    IntersectClipRect( hdc, rc.left, rc.top, rc.right, rc.bottom );
    return hrgn;
}

static WCHAR *heap_strndupW(const WCHAR *src, size_t length)
{
    size_t size = (length + 1) * sizeof(WCHAR);
    WCHAR *dst = heap_alloc(size);
    if (dst) memcpy(dst, src, size);
    return dst;
}

/**********************************************************************
 * Convert button styles to flags used by DrawText.
 */
static UINT BUTTON_BStoDT( DWORD style, DWORD ex_style )
{
    UINT dtStyle = DT_NOCLIP;  /* We use SelectClipRgn to limit output */

    /* "Convert" pushlike buttons to pushbuttons */
    if (style & BS_PUSHLIKE)
        style &= ~BS_TYPEMASK;

    if (!(style & BS_MULTILINE))
        dtStyle |= DT_SINGLELINE;
    else
        dtStyle |= DT_WORDBREAK;

    switch (style & BS_CENTER)
    {
        case BS_LEFT:   /* DT_LEFT is 0 */    break;
        case BS_RIGHT:  dtStyle |= DT_RIGHT;  break;
        case BS_CENTER: dtStyle |= DT_CENTER; break;
        default:
            if (button_centers_text(style)) dtStyle |= DT_CENTER;
    }

    if (ex_style & WS_EX_RIGHT) dtStyle = DT_RIGHT | (dtStyle & ~(DT_LEFT | DT_CENTER));

    /* DrawText ignores vertical alignment for multiline text,
     * but we use these flags to align label manually.
     */
    if (get_button_type(style) != BS_GROUPBOX)
    {
        switch (style & BS_VCENTER)
        {
            case BS_TOP:     /* DT_TOP is 0 */      break;
            case BS_BOTTOM:  dtStyle |= DT_BOTTOM;  break;
            case BS_VCENTER: /* fall through */
            default:         dtStyle |= DT_VCENTER; break;
        }
    }

    return dtStyle;
}


#ifdef __REACTOS__
static int get_draw_state(const BUTTON_INFO *infoPtr);
BOOL BUTTON_PaintWithTheme(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hParamDC, LPARAM prfFlag)
{
    DWORD dwStyle;
    DWORD dwStyleEx;
    DWORD type;
    UINT dtFlags;
    int drawState;
    pfThemedPaint paint;

    /* Don't draw with themes on a button with BS_ICON or BS_BITMAP */
    if (infoPtr->u.image != 0)
        return FALSE;

    dwStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    type = get_button_type(dwStyle);

    if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON && (dwStyle & BS_PUSHLIKE))
        type = BS_PUSHBUTTON;

    paint = btnThemedPaintFunc[type];
    if (!paint)
        return FALSE;

    dwStyleEx = GetWindowLongW(infoPtr->hwnd, GWL_EXSTYLE);
    dtFlags = BUTTON_BStoDT(dwStyle, dwStyleEx);

    drawState = get_draw_state(infoPtr);

    if (paint == PB_ThemedPaint || paint == CB_ThemedPaint)
    {
        HDC hdc;
        HBITMAP hbmp;
        RECT rc;

        GetClientRect(infoPtr->hwnd, &rc);
        hdc = CreateCompatibleDC(hParamDC);
        hbmp = CreateCompatibleBitmap(hParamDC, rc.right, rc.bottom);
        if (hdc && hbmp)
        {
            SelectObject(hdc, hbmp);

            paint(theme, infoPtr, hdc, drawState, dtFlags, infoPtr->state & BST_FOCUS, prfFlag);

            BitBlt(hParamDC, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
            DeleteObject(hbmp);
            DeleteDC(hdc);
            return TRUE;
        }
        else
        {
            ERR("Failed to create DC and bitmap for double buffering\n");
            if (hbmp)
                DeleteObject(hbmp);
            if (hdc)
                DeleteDC(hdc);
        }
    }

    paint(theme, infoPtr, hParamDC, drawState, dtFlags, infoPtr->state & BST_FOCUS, prfFlag);
    return TRUE;
}

/* paint a button of any type */
static inline void paint_button( BUTTON_INFO *infoPtr, LONG style, UINT action )
{
    HTHEME theme = GetWindowTheme(infoPtr->hwnd);
    RECT rc;
    HDC hdc = GetDC( infoPtr->hwnd );
    /* GetDC appears to give a dc with a clip rect that includes the whoe parent, not sure if it is correct or not. */
    GetClientRect(infoPtr->hwnd, &rc);
    IntersectClipRect (hdc, rc.left, rc. top, rc.right, rc.bottom);
    if (theme && BUTTON_PaintWithTheme(theme, infoPtr, hdc, 0))
    {
        ReleaseDC( infoPtr->hwnd, hdc );
        return;
    }
    if (btnPaintFunc[style] && IsWindowVisible(infoPtr->hwnd))
    {
        btnPaintFunc[style]( infoPtr, hdc, action );
    }
    ReleaseDC( infoPtr->hwnd, hdc );
}

BOOL BUTTON_GetIdealSize(BUTTON_INFO *infoPtr, HTHEME theme, SIZE* psize)
{
    HDC hdc;
    WCHAR *text;
    HFONT hFont = 0, hPrevFont = 0;
    SIZE TextSize, ImageSize, ButtonSize;
    BOOL ret = FALSE;
    LOGFONTW logfont = {0};

    text = get_button_text(infoPtr);
    hdc = GetDC(infoPtr->hwnd);
    if (!text || !hdc || !text[0])
        goto cleanup;

    /* FIXME : Should use GetThemeTextExtent but unfortunately uses DrawTextW which is broken */
    if (theme)
    {
        HRESULT hr = GetThemeFont(theme, hdc, BP_PUSHBUTTON, PBS_NORMAL, TMT_FONT, &logfont);
        if(SUCCEEDED(hr))
        {
            hFont = CreateFontIndirectW(&logfont);
            if(hFont)
                hPrevFont = SelectObject( hdc, hFont );
        }
    }
    else
    {
        if (infoPtr->font)
            hPrevFont = SelectObject( hdc, infoPtr->font );
    }

    GetTextExtentPoint32W(hdc, text, wcslen(text), &TextSize);

    if (logfont.lfHeight == -1 && logfont.lfWidth == 0 && wcscmp(logfont.lfFaceName, L"Arial") == 0 && wcsicmp(text, L"Start") == 0)
    {
        TextSize.cx = 5;
        TextSize.cy = 4;
    }

    if (hPrevFont)
        SelectObject( hdc, hPrevFont );

    TextSize.cy += infoPtr->text_margin.top + infoPtr->text_margin.bottom;
    TextSize.cx += infoPtr->text_margin.left + infoPtr->text_margin.right;

    if (infoPtr->imagelist.himl && ImageList_GetIconSize(infoPtr->imagelist.himl, &ImageSize.cx, &ImageSize.cy))
    {
        ImageSize.cx += infoPtr->imagelist.margin.left + infoPtr->imagelist.margin.right;
        ImageSize.cy += infoPtr->imagelist.margin.top + infoPtr->imagelist.margin.bottom;
    }
    else
    {
        ImageSize.cx = ImageSize.cy = 0;
    }

    if (theme)
    {
        RECT rcContents = {0};
        RECT rcButtonExtent = {0};
        rcContents.right = ImageSize.cx + TextSize.cx;
        rcContents.bottom = max(ImageSize.cy, TextSize.cy);
        GetThemeBackgroundExtent(theme, hdc, BP_PUSHBUTTON, PBS_NORMAL, &rcContents, &rcButtonExtent);
        ButtonSize.cx = rcButtonExtent.right - rcButtonExtent.left;
        ButtonSize.cy = rcButtonExtent.bottom - rcButtonExtent.top;
    }
    else
    {
        ButtonSize.cx = ImageSize.cx + TextSize.cx + 5;
        ButtonSize.cy = max(ImageSize.cy, TextSize.cy  + 7);
    }

    *psize = ButtonSize;
    ret = TRUE;

cleanup:
    if (hFont)
        DeleteObject(hFont);
    if (text) 
        HeapFree( GetProcessHeap(), 0, text );
    if (hdc)
        ReleaseDC(infoPtr->hwnd, hdc);

    return ret;
}

BOOL BUTTON_DrawIml(HDC hDC, const BUTTON_IMAGELIST *pimlData, RECT *prc, BOOL bOnlyCalc, int index)
{
    SIZE ImageSize;
    int left, top, count;

    if (!pimlData->himl)
        return FALSE;

    if (!ImageList_GetIconSize(pimlData->himl, &ImageSize.cx, &ImageSize.cy))
        return FALSE;

    if (pimlData->uAlign == BUTTON_IMAGELIST_ALIGN_LEFT)
    {
        left = prc->left + pimlData->margin.left;
        top = prc->top + (prc->bottom - prc->top - ImageSize.cy) / 2;
        prc->left = left + pimlData->margin.right + ImageSize.cx;
    }
    else if (pimlData->uAlign == BUTTON_IMAGELIST_ALIGN_RIGHT)
    {
        left = prc->right - pimlData->margin.right - ImageSize.cx;
        top = prc->top + (prc->bottom - prc->top - ImageSize.cy) / 2;
        prc->right = left - pimlData->margin.left;
    }
    else if (pimlData->uAlign == BUTTON_IMAGELIST_ALIGN_TOP)
    {
        left = prc->left + (prc->right - prc->left - ImageSize.cx) / 2;
        top = prc->top + pimlData->margin.top;
        prc->top = top + ImageSize.cy + pimlData->margin.bottom;
    }
    else if (pimlData->uAlign == BUTTON_IMAGELIST_ALIGN_BOTTOM)
    {
        left = prc->left + (prc->right - prc->left - ImageSize.cx) / 2;
        top = prc->bottom - pimlData->margin.bottom - ImageSize.cy;
        prc->bottom = top - pimlData->margin.top;
    }
    else if (pimlData->uAlign == BUTTON_IMAGELIST_ALIGN_CENTER)
    {
        left = prc->left + (prc->right - prc->left - ImageSize.cx) / 2;
        top = prc->top + (prc->bottom - prc->top - ImageSize.cy) / 2;
    }

    if (bOnlyCalc)
        return TRUE;

    count = ImageList_GetImageCount(pimlData->himl);
    
    if (count == 1)
        index = 0;
    else if (index >= count)
        return TRUE;

    ImageList_Draw(pimlData->himl, index, hDC, left, top, 0);

    return TRUE;
}

DWORD BUTTON_SendCustomDraw(const BUTTON_INFO *infoPtr, HDC hDC, DWORD dwDrawStage, RECT* prc)
{
    NMCUSTOMDRAW nmcs;

    nmcs.hdr.hwndFrom = infoPtr->hwnd;
    nmcs.hdr.idFrom   = GetWindowLongPtrW (infoPtr->hwnd, GWLP_ID);
    nmcs.hdr.code     = NM_CUSTOMDRAW ;
    nmcs.dwDrawStage  = dwDrawStage;
    nmcs.hdc          = hDC;
    nmcs.rc           = *prc;
    nmcs.dwItemSpec   = 0;
    nmcs.uItemState   = 0;
    nmcs.lItemlParam  = 0;
    if(!IsWindowEnabled(infoPtr->hwnd))
        nmcs.uItemState |= CDIS_DISABLED;
    if (infoPtr->state & (BST_CHECKED | BST_INDETERMINATE))
        nmcs.uItemState |= CDIS_CHECKED;
    if (infoPtr->state & BST_FOCUS)
        nmcs.uItemState |= CDIS_FOCUS;
    if (infoPtr->state & BST_PUSHED)
        nmcs.uItemState |= CDIS_SELECTED;
    if (!(infoPtr->ui_state & UISF_HIDEACCEL))
        nmcs.uItemState |= CDIS_SHOWKEYBOARDCUES;

    return SendMessageW(GetParent(infoPtr->hwnd), WM_NOTIFY, nmcs.hdr.idFrom, (LPARAM)&nmcs);
}

/* Retrieve the UI state for the control */
static BOOL button_update_uistate(BUTTON_INFO *infoPtr)
{
    LONG flags = DefWindowProcW(infoPtr->hwnd, WM_QUERYUISTATE, 0, 0);

    if (infoPtr->ui_state != flags)
    {
        infoPtr->ui_state = flags;
        return TRUE;
    }

    return FALSE;
}
#endif

static int get_draw_state(const BUTTON_INFO *infoPtr)
{
    static const int pb_states[DRAW_STATE_COUNT] = { PBS_NORMAL, PBS_DISABLED, PBS_HOT, PBS_PRESSED, PBS_DEFAULTED };
    static const int cb_states[3][DRAW_STATE_COUNT] =
    {
        { CBS_UNCHECKEDNORMAL, CBS_UNCHECKEDDISABLED, CBS_UNCHECKEDHOT, CBS_UNCHECKEDPRESSED, CBS_UNCHECKEDNORMAL },
        { CBS_CHECKEDNORMAL, CBS_CHECKEDDISABLED, CBS_CHECKEDHOT, CBS_CHECKEDPRESSED, CBS_CHECKEDNORMAL },
        { CBS_MIXEDNORMAL, CBS_MIXEDDISABLED, CBS_MIXEDHOT, CBS_MIXEDPRESSED, CBS_MIXEDNORMAL }
    };
    static const int rb_states[2][DRAW_STATE_COUNT] =
    {
        { RBS_UNCHECKEDNORMAL, RBS_UNCHECKEDDISABLED, RBS_UNCHECKEDHOT, RBS_UNCHECKEDPRESSED, RBS_UNCHECKEDNORMAL },
        { RBS_CHECKEDNORMAL, RBS_CHECKEDDISABLED, RBS_CHECKEDHOT, RBS_CHECKEDPRESSED, RBS_CHECKEDNORMAL }
    };
    static const int gb_states[DRAW_STATE_COUNT] = { GBS_NORMAL, GBS_DISABLED, GBS_NORMAL, GBS_NORMAL, GBS_NORMAL };
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    UINT type = get_button_type(style);
    int check_state = infoPtr->state & 3;
    enum draw_state state;

    if (!IsWindowEnabled(infoPtr->hwnd))
        state = STATE_DISABLED;
    else if (infoPtr->state & BST_PUSHED)
        state = STATE_PRESSED;
    else if (infoPtr->state & BST_HOT)
        state = STATE_HOT;
    else if (infoPtr->state & BST_FOCUS)
        state = STATE_DEFAULTED;
    else
        state = STATE_NORMAL;

    switch (type)
    {
    case BS_PUSHBUTTON:
    case BS_DEFPUSHBUTTON:
    case BS_USERBUTTON:
    case BS_SPLITBUTTON:
    case BS_DEFSPLITBUTTON:
    case BS_COMMANDLINK:
    case BS_DEFCOMMANDLINK:
        return pb_states[state];
    case BS_CHECKBOX:
    case BS_AUTOCHECKBOX:
        return cb_states[check_state][state];
    case BS_RADIOBUTTON:
    case BS_3STATE:
    case BS_AUTO3STATE:
    case BS_AUTORADIOBUTTON:
        return rb_states[check_state][state];
    case BS_GROUPBOX:
        return gb_states[state];
    default:
        WARN("Unsupported button type 0x%08x\n", type);
        return PBS_NORMAL;
    }
}

static LRESULT CALLBACK BUTTON_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BUTTON_INFO *infoPtr = (BUTTON_INFO *)GetWindowLongPtrW(hWnd, 0);
    RECT rect;
    POINT pt;
    LONG style = GetWindowLongW( hWnd, GWL_STYLE );
    UINT btn_type = get_button_type( style );
    LONG state, new_state;
    HANDLE oldHbitmap;
    HTHEME theme;

    if (!IsWindow( hWnd )) return 0;

    if (!infoPtr && (uMsg != WM_NCCREATE))
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    switch (uMsg)
    {
    case WM_GETDLGCODE:
        switch(btn_type)
        {
        case BS_COMMANDLINK:
        case BS_USERBUTTON:
        case BS_PUSHBUTTON:      return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
        case BS_DEFCOMMANDLINK:
        case BS_DEFPUSHBUTTON:   return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON: return DLGC_BUTTON | DLGC_RADIOBUTTON;
        case BS_GROUPBOX:        return DLGC_STATIC;
        case BS_SPLITBUTTON:     return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON | DLGC_WANTARROWS;
        case BS_DEFSPLITBUTTON:  return DLGC_BUTTON | DLGC_DEFPUSHBUTTON | DLGC_WANTARROWS;
        default:                 return DLGC_BUTTON;
        }

    case WM_ENABLE:
#ifndef __REACTOS__
        theme = GetWindowTheme( hWnd );
        if (theme)
            RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW );
        else
#endif
            paint_button( infoPtr, btn_type, ODA_DRAWENTIRE );
        break;

    case WM_NCCREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;

        infoPtr = heap_alloc_zero( sizeof(*infoPtr) );
        SetWindowLongPtrW( hWnd, 0, (LONG_PTR)infoPtr );
        infoPtr->hwnd = hWnd;
        infoPtr->parent = cs->hwndParent;
        infoPtr->style = cs->style;
        infoPtr->split_style = BCSS_STRETCH;
        infoPtr->glyph = (HIMAGELIST)0x36;  /* Marlett down arrow char code */
        infoPtr->glyph_size.cx = get_default_glyph_size(infoPtr);
#ifdef __REACTOS__
        SetRect(&infoPtr->text_margin, 1,1,1,1);
#endif
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    case WM_NCDESTROY:
        SetWindowLongPtrW( hWnd, 0, 0 );
        heap_free(infoPtr->note);
        heap_free(infoPtr);
        break;

    case WM_CREATE:
        if (btn_type >= MAX_BTN_TYPE)
            return -1; /* abort */

        /* XP turns a BS_USERBUTTON into BS_PUSHBUTTON */
        if (btn_type == BS_USERBUTTON )
        {
            style = (style & ~BS_TYPEMASK) | BS_PUSHBUTTON;
            SetWindowLongW( hWnd, GWL_STYLE, style );
        }
        infoPtr->state = BST_UNCHECKED;
        OpenThemeData( hWnd, WC_BUTTONW );
        return 0;

    case WM_DESTROY:
        theme = GetWindowTheme( hWnd );
        CloseThemeData( theme );
        break;

    case WM_THEMECHANGED:
        theme = GetWindowTheme( hWnd );
        CloseThemeData( theme );
        OpenThemeData( hWnd, WC_BUTTONW );
#ifdef __REACTOS__
        InvalidateRect(hWnd, NULL, TRUE);
#endif
        break;

    case WM_ERASEBKGND:
        if (btn_type == BS_OWNERDRAW)
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            HBRUSH hBrush;
            HWND parent = GetParent(hWnd);
            if (!parent) parent = hWnd;
            hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hdc, (LPARAM)hWnd);
            if (!hBrush) /* did the app forget to call defwindowproc ? */
                hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORBTN,
                                                (WPARAM)hdc, (LPARAM)hWnd);
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, hBrush);
        }
        return 1;

    case WM_PRINTCLIENT:
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc;

        theme = GetWindowTheme( hWnd );
        hdc = wParam ? (HDC)wParam : BeginPaint( hWnd, &ps );

#ifdef __REACTOS__
        if (theme && BUTTON_PaintWithTheme(theme, infoPtr, hdc, uMsg == WM_PRINTCLIENT ? lParam : 0))
        {
            if ( !wParam ) EndPaint( hWnd, &ps );
            return 0;
        }
#else
        if (theme && btnThemedPaintFunc[btn_type])
        {
            int drawState = get_draw_state(infoPtr);
            UINT dtflags = BUTTON_BStoDT(style, GetWindowLongW(hWnd, GWL_EXSTYLE));

            btnThemedPaintFunc[btn_type](theme, infoPtr, hdc, drawState, dtflags, infoPtr->state & BST_FOCUS);
        }
#endif
        else if (btnPaintFunc[btn_type])
        {
            int nOldMode = SetBkMode( hdc, OPAQUE );
            btnPaintFunc[btn_type]( infoPtr, hdc, ODA_DRAWENTIRE );
            SetBkMode(hdc, nOldMode); /*  reset painting mode */
        }

        if ( !wParam ) EndPaint( hWnd, &ps );
        break;
    }

    case WM_KEYDOWN:
	if (wParam == VK_SPACE)
	{
	    SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
            infoPtr->state |= BUTTON_BTNPRESSED;
            SetCapture( hWnd );
	}
        else if (wParam == VK_UP || wParam == VK_DOWN)
        {
            /* Up and down arrows work on every button, and even with BCSS_NOSPLIT */
            notify_split_button_dropdown(infoPtr, NULL, hWnd);
        }
	break;

    case WM_LBUTTONDBLCLK:
        if(style & BS_NOTIFY ||
           btn_type == BS_RADIOBUTTON ||
           btn_type == BS_USERBUTTON ||
           btn_type == BS_OWNERDRAW)
        {
            BUTTON_NOTIFY_PARENT(hWnd, BN_DOUBLECLICKED);
            break;
        }
        /* fall through */
    case WM_LBUTTONDOWN:
        SetFocus( hWnd );

        if ((btn_type == BS_SPLITBUTTON || btn_type == BS_DEFSPLITBUTTON) &&
            !(infoPtr->split_style & BCSS_NOSPLIT) &&
            notify_split_button_dropdown(infoPtr, &pt, hWnd))
            break;

        SetCapture( hWnd );
        infoPtr->state |= BUTTON_BTNPRESSED;
        SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
        break;

    case WM_KEYUP:
	if (wParam != VK_SPACE)
	    break;
	/* fall through */
    case WM_LBUTTONUP:
        state = infoPtr->state;
        if (state & BST_DROPDOWNPUSHED)
            SendMessageW(hWnd, BCM_SETDROPDOWNSTATE, FALSE, 0);
        if (!(state & BUTTON_BTNPRESSED)) break;
        infoPtr->state &= BUTTON_NSTATES | BST_HOT;
        if (!(state & BST_PUSHED))
        {
            ReleaseCapture();
            break;
        }
        SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        GetClientRect( hWnd, &rect );
	if (uMsg == WM_KEYUP || PtInRect( &rect, pt ))
        {
            switch(btn_type)
            {
            case BS_AUTOCHECKBOX:
                SendMessageW( hWnd, BM_SETCHECK, !(infoPtr->state & BST_CHECKED), 0 );
                break;
            case BS_AUTORADIOBUTTON:
#ifdef __REACTOS__
                BUTTON_CheckAutoRadioButton( hWnd );
#else
                SendMessageW( hWnd, BM_SETCHECK, TRUE, 0 );
#endif
                break;
            case BS_AUTO3STATE:
                SendMessageW( hWnd, BM_SETCHECK, (infoPtr->state & BST_INDETERMINATE) ? 0 :
                    ((infoPtr->state & 3) + 1), 0 );
                break;
            }
#ifdef __REACTOS__
            // Fix CORE-10194, Notify parent after capture is released.
            ReleaseCapture();
            BUTTON_NOTIFY_PARENT(hWnd, BN_CLICKED);
#else
            BUTTON_NOTIFY_PARENT(hWnd, BN_CLICKED);
            ReleaseCapture();
#endif
        }
        else
        {
            ReleaseCapture();
        }

        break;

    case WM_CAPTURECHANGED:
        TRACE("WM_CAPTURECHANGED %p\n", hWnd);
        if (hWnd == (HWND)lParam) break;
        if (infoPtr->state & BUTTON_BTNPRESSED)
        {
            infoPtr->state &= BUTTON_NSTATES;
            if (infoPtr->state & BST_PUSHED)
                SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        }
        break;

    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT mouse_event;
        mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
        mouse_event.dwFlags = TME_QUERY;

#ifdef __REACTOS__
        if ((infoPtr->state & BST_HOT) == 0)
        {
            NMBCHOTITEM nmhotitem;

            infoPtr->state |= BST_HOT;

            nmhotitem.hdr.hwndFrom = hWnd;
            nmhotitem.hdr.idFrom   = GetWindowLongPtrW (hWnd, GWLP_ID);
            nmhotitem.hdr.code     = BCN_HOTITEMCHANGE;
            nmhotitem.dwFlags      = HICF_ENTERING;
            SendMessageW(GetParent(hWnd), WM_NOTIFY, nmhotitem.hdr.idFrom, (LPARAM)&nmhotitem);

            theme = GetWindowTheme( hWnd );
            if (theme)
                InvalidateRect(hWnd, NULL, TRUE);
        }

        if(!TrackMouseEvent(&mouse_event) || !(mouse_event.dwFlags&TME_LEAVE))
        {
            mouse_event.dwFlags = TME_LEAVE;
            mouse_event.hwndTrack = hWnd;
            mouse_event.dwHoverTime = 1;
            TrackMouseEvent(&mouse_event);
        }
#else

        if (!TrackMouseEvent(&mouse_event) || !(mouse_event.dwFlags & (TME_HOVER | TME_LEAVE)))
        {
            mouse_event.dwFlags = TME_HOVER | TME_LEAVE;
            mouse_event.hwndTrack = hWnd;
            mouse_event.dwHoverTime = 1;
            TrackMouseEvent(&mouse_event);
        }
#endif

        if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
        {
            GetClientRect( hWnd, &rect );
            SendMessageW( hWnd, BM_SETSTATE, PtInRect(&rect, pt), 0 );
        }
        break;
    }

#ifndef __REACTOS__
    case WM_MOUSEHOVER:
    {
        infoPtr->state |= BST_HOT;
        InvalidateRect( hWnd, NULL, FALSE );
        break;
    }
#endif

    case WM_MOUSELEAVE:
    {
#ifdef __REACTOS__
        if (infoPtr->state & BST_HOT)
        {
            NMBCHOTITEM nmhotitem;

            infoPtr->state &= ~BST_HOT;

            nmhotitem.hdr.hwndFrom = hWnd;
            nmhotitem.hdr.idFrom   = GetWindowLongPtrW (hWnd, GWLP_ID);
            nmhotitem.hdr.code     = BCN_HOTITEMCHANGE;
            nmhotitem.dwFlags      = HICF_LEAVING;
            SendMessageW(GetParent(hWnd), WM_NOTIFY, nmhotitem.hdr.idFrom, (LPARAM)&nmhotitem);

            theme = GetWindowTheme( hWnd );
            if (theme)
                InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
#else
        infoPtr->state &= ~BST_HOT;
        InvalidateRect( hWnd, NULL, FALSE );
        break;
#endif
    }

    case WM_SETTEXT:
    {
        /* Clear an old text here as Windows does */
#ifdef __REACTOS__
//
// ReactOS Note :
// wine Bug: http://bugs.winehq.org/show_bug.cgi?id=25790
// Patch: http://source.winehq.org/patches/data/70889
// By: Alexander LAW, Replicate Windows behavior of WM_SETTEXT handler regarding WM_CTLCOLOR*
//
        if (style & WS_VISIBLE)
#else
        if (IsWindowVisible(hWnd))
#endif
        {
            HDC hdc = GetDC(hWnd);
            HBRUSH hbrush;
            RECT client, rc;
            HWND parent = GetParent(hWnd);
            UINT message = (btn_type == BS_PUSHBUTTON ||
                            btn_type == BS_DEFPUSHBUTTON ||
                            btn_type == BS_USERBUTTON ||
                            btn_type == BS_OWNERDRAW) ?
                            WM_CTLCOLORBTN : WM_CTLCOLORSTATIC;

            if (!parent) parent = hWnd;
            hbrush = (HBRUSH)SendMessageW(parent, message,
                                          (WPARAM)hdc, (LPARAM)hWnd);
            if (!hbrush) /* did the app forget to call DefWindowProc ? */
                hbrush = (HBRUSH)DefWindowProcW(parent, message,
                                                (WPARAM)hdc, (LPARAM)hWnd);

            GetClientRect(hWnd, &client);
            rc = client;
            /* FIXME: check other BS_* handlers */
            if (btn_type == BS_GROUPBOX)
                InflateRect(&rc, -7, 1); /* GB_Paint does this */
            BUTTON_CalcLayoutRects(infoPtr, hdc, &rc, NULL, NULL);
            /* Clip by client rect bounds */
            if (rc.right > client.right) rc.right = client.right;
            if (rc.bottom > client.bottom) rc.bottom = client.bottom;
            FillRect(hdc, &rc, hbrush);
            ReleaseDC(hWnd, hdc);
        }

        DefWindowProcW( hWnd, WM_SETTEXT, wParam, lParam );
        if (btn_type == BS_GROUPBOX) /* Yes, only for BS_GROUPBOX */
            InvalidateRect( hWnd, NULL, TRUE );
        else
            paint_button( infoPtr, btn_type, ODA_DRAWENTIRE );
        return 1; /* success. FIXME: check text length */
    }

    case BCM_SETNOTE:
    {
        WCHAR *note = (WCHAR *)lParam;
        if (btn_type != BS_COMMANDLINK && btn_type != BS_DEFCOMMANDLINK)
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            return FALSE;
        }

        heap_free(infoPtr->note);
        if (note)
        {
            infoPtr->note_length = lstrlenW(note);
            infoPtr->note = heap_strndupW(note, infoPtr->note_length);
        }

        if (!note || !infoPtr->note)
        {
            infoPtr->note_length = 0;
            infoPtr->note = heap_alloc_zero(sizeof(WCHAR));
        }

        SetLastError(NO_ERROR);
        return TRUE;
    }

    case BCM_GETNOTE:
    {
        DWORD *size = (DWORD *)wParam;
        WCHAR *buffer = (WCHAR *)lParam;
        INT length = 0;

        if (btn_type != BS_COMMANDLINK && btn_type != BS_DEFCOMMANDLINK)
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            return FALSE;
        }

        if (!buffer || !size || !infoPtr->note)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (*size > 0)
        {
            length = min(*size - 1, infoPtr->note_length);
            memcpy(buffer, infoPtr->note, length * sizeof(WCHAR));
            buffer[length] = '\0';
        }

        if (*size < infoPtr->note_length + 1)
        {
            *size = infoPtr->note_length + 1;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        else
        {
            SetLastError(NO_ERROR);
            return TRUE;
        }
    }

    case BCM_GETNOTELENGTH:
    {
        if (btn_type != BS_COMMANDLINK && btn_type != BS_DEFCOMMANDLINK)
        {
            SetLastError(ERROR_NOT_SUPPORTED);
            return 0;
        }

        return infoPtr->note_length;
    }

    case WM_SETFONT:
        infoPtr->font = (HFONT)wParam;
        if (lParam) InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_GETFONT:
        return (LRESULT)infoPtr->font;

    case WM_SETFOCUS:
        TRACE("WM_SETFOCUS %p\n",hWnd);
        infoPtr->state |= BST_FOCUS;
#ifdef __REACTOS__
        if (btn_type != BS_OWNERDRAW)
            InvalidateRect(hWnd, NULL, FALSE);
        else
#endif
        paint_button( infoPtr, btn_type, ODA_FOCUS );
        if (style & BS_NOTIFY)
            BUTTON_NOTIFY_PARENT(hWnd, BN_SETFOCUS);
#ifdef __REACTOS__
        if (((btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON)) &&
            !(infoPtr->state & BST_CHECKED))
        {
            BUTTON_NOTIFY_PARENT(hWnd, BN_CLICKED);
        }
#endif
        break;

    case WM_KILLFOCUS:
        TRACE("WM_KILLFOCUS %p\n",hWnd);
        infoPtr->state &= ~BST_FOCUS;

        if ((infoPtr->state & BUTTON_BTNPRESSED) && GetCapture() == hWnd)
            ReleaseCapture();
        if (style & BS_NOTIFY)
            BUTTON_NOTIFY_PARENT(hWnd, BN_KILLFOCUS);

        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case WM_SYSCOLORCHANGE:
        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case BM_SETSTYLE:
        btn_type = wParam & BS_TYPEMASK;
        style = (style & ~BS_TYPEMASK) | btn_type;
        SetWindowLongW( hWnd, GWL_STYLE, style );

        /* Only redraw if lParam flag is set.*/
        if (lParam)
            InvalidateRect( hWnd, NULL, TRUE );

        break;

    case BM_CLICK:
#ifdef __REACTOS__
    /* Fix for core CORE-6024 */
    if (infoPtr->state & BUTTON_BMCLICK)
       break;
    infoPtr->state |= BUTTON_BMCLICK;
#endif
	SendMessageW( hWnd, WM_LBUTTONDOWN, 0, 0 );
	SendMessageW( hWnd, WM_LBUTTONUP, 0, 0 );
#ifdef __REACTOS__
    infoPtr->state &= ~BUTTON_BMCLICK;
#endif
	break;

    case BM_SETIMAGE:
        infoPtr->image_type = (DWORD)wParam;
        oldHbitmap = infoPtr->u.image;
        infoPtr->u.image = (HANDLE)lParam;
	InvalidateRect( hWnd, NULL, FALSE );
	return (LRESULT)oldHbitmap;

    case BM_GETIMAGE:
        return (LRESULT)infoPtr->u.image;

    case BCM_SETIMAGELIST:
    {
        BUTTON_IMAGELIST *imagelist = (BUTTON_IMAGELIST *)lParam;

        if (!imagelist) return FALSE;

        infoPtr->imagelist = *imagelist;
        return TRUE;
    }

    case BCM_GETIMAGELIST:
    {
        BUTTON_IMAGELIST *imagelist = (BUTTON_IMAGELIST *)lParam;

        if (!imagelist) return FALSE;

        *imagelist = infoPtr->imagelist;
        return TRUE;
    }

    case BCM_SETSPLITINFO:
    {
        BUTTON_SPLITINFO *info = (BUTTON_SPLITINFO*)lParam;

        if (!info) return TRUE;

        if (info->mask & (BCSIF_GLYPH | BCSIF_IMAGE))
        {
            infoPtr->split_style &= ~BCSS_IMAGE;
            if (!(info->mask & BCSIF_GLYPH))
                infoPtr->split_style |= BCSS_IMAGE;
            infoPtr->glyph = info->himlGlyph;
            infoPtr->glyph_size.cx = infoPtr->glyph_size.cy = 0;
        }

        if (info->mask & BCSIF_STYLE)
            infoPtr->split_style = info->uSplitStyle;
        if (info->mask & BCSIF_SIZE)
            infoPtr->glyph_size = info->size;

        /* Calculate fitting value for cx if invalid (cy is untouched) */
        if (infoPtr->glyph_size.cx <= 0)
            infoPtr->glyph_size.cx = get_default_glyph_size(infoPtr);

        /* Windows doesn't invalidate or redraw it, so we don't, either */
        return TRUE;
    }

    case BCM_GETSPLITINFO:
    {
        BUTTON_SPLITINFO *info = (BUTTON_SPLITINFO*)lParam;

        if (!info) return FALSE;

        if (info->mask & BCSIF_STYLE)
            info->uSplitStyle = infoPtr->split_style;
        if (info->mask & (BCSIF_GLYPH | BCSIF_IMAGE))
            info->himlGlyph = infoPtr->glyph;
        if (info->mask & BCSIF_SIZE)
            info->size = infoPtr->glyph_size;

        return TRUE;
    }

    case BM_GETCHECK:
        return infoPtr->state & 3;

    case BM_SETCHECK:
        if (wParam > maxCheckState[btn_type]) wParam = maxCheckState[btn_type];
        if ((btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON))
        {
            style = wParam ? style | WS_TABSTOP : style & ~WS_TABSTOP;
            SetWindowLongW( hWnd, GWL_STYLE, style );
        }
        if ((infoPtr->state & 3) != wParam)
        {
            infoPtr->state = (infoPtr->state & ~3) | wParam;
            InvalidateRect( hWnd, NULL, FALSE );
        }
#ifndef __REACTOS__
        if ((btn_type == BS_AUTORADIOBUTTON) && (wParam == BST_CHECKED) && (style & WS_CHILD))
            BUTTON_CheckAutoRadioButton( hWnd );
#endif
        break;

    case BM_GETSTATE:
        return infoPtr->state;

    case BM_SETSTATE:
        state = infoPtr->state;
        new_state = wParam ? BST_PUSHED : 0;

        if ((state ^ new_state) & BST_PUSHED)
        {
            if (wParam)
                state |= BST_PUSHED;
            else
                state &= ~BST_PUSHED;

            if (btn_type == BS_USERBUTTON)
                BUTTON_NOTIFY_PARENT( hWnd, (state & BST_PUSHED) ? BN_HILITE : BN_UNHILITE );
            infoPtr->state = state;

            InvalidateRect( hWnd, NULL, FALSE );
        }
        break;

#ifdef __REACTOS__
    case WM_UPDATEUISTATE:
        DefWindowProcW(hWnd, uMsg, wParam, lParam);

        if (button_update_uistate(infoPtr))
            paint_button( infoPtr, btn_type, ODA_DRAWENTIRE );
        break;
#endif

    case BCM_SETDROPDOWNSTATE:
        new_state = wParam ? BST_DROPDOWNPUSHED : 0;

        if ((infoPtr->state ^ new_state) & BST_DROPDOWNPUSHED)
        {
            infoPtr->state &= ~BST_DROPDOWNPUSHED;
            infoPtr->state |= new_state;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;

    case BCM_SETTEXTMARGIN:
    {
        RECT *text_margin = (RECT *)lParam;

        if (!text_margin) return FALSE;

        infoPtr->text_margin = *text_margin;
        return TRUE;
    }

    case BCM_GETTEXTMARGIN:
    {
        RECT *text_margin = (RECT *)lParam;

        if (!text_margin) return FALSE;

        *text_margin = infoPtr->text_margin;
        return TRUE;
    }

    case BCM_GETIDEALSIZE:
    {
        SIZE *size = (SIZE *)lParam;

        if (!size) return FALSE;

        return btnGetIdealSizeFunc[btn_type](infoPtr, size);
    }

    case WM_NCHITTEST:
        if(btn_type == BS_GROUPBOX) return HTTRANSPARENT;
        /* fall through */
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/* If maxWidth is zero, rectangle width is unlimited */
static RECT BUTTON_GetTextRect(const BUTTON_INFO *infoPtr, HDC hdc, const WCHAR *text, LONG maxWidth)
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLongW(infoPtr->hwnd, GWL_EXSTYLE);
    UINT dtStyle = BUTTON_BStoDT(style, exStyle);
    HFONT hPrevFont;
    RECT rect = {0};

    rect.right = maxWidth;
    hPrevFont = SelectObject(hdc, infoPtr->font);
    /* Calculate height without DT_VCENTER and DT_BOTTOM to get the correct height */
    DrawTextW(hdc, text, -1, &rect, (dtStyle & ~(DT_VCENTER | DT_BOTTOM)) | DT_CALCRECT);
    if (hPrevFont) SelectObject(hdc, hPrevFont);

    return rect;
}

static BOOL show_image_only(const BUTTON_INFO *infoPtr)
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    return (style & (BS_ICON | BS_BITMAP)) && (infoPtr->u.image || infoPtr->imagelist.himl);
}

static BOOL show_image_and_text(const BUTTON_INFO *infoPtr)
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    UINT type = get_button_type(style);
    return !(style & (BS_ICON | BS_BITMAP))
           && ((infoPtr->u.image
                && (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON || type == BS_USERBUTTON || type == BS_SPLITBUTTON
                    || type == BS_DEFSPLITBUTTON || type == BS_COMMANDLINK || type == BS_DEFCOMMANDLINK))
               || (infoPtr->imagelist.himl && type != BS_GROUPBOX));
}

static BOOL show_image(const BUTTON_INFO *infoPtr)
{
    return show_image_only(infoPtr) || show_image_and_text(infoPtr);
}

/* Get a bounding rectangle that is large enough to contain a image and a text side by side.
 * Note: (left,top) of the result rectangle may not be (0,0), offset it by yourself if needed */
static RECT BUTTON_GetBoundingLabelRect(LONG style, const RECT *textRect, const RECT *imageRect)
{
    RECT labelRect;
    RECT rect = *imageRect;
    INT textWidth = textRect->right - textRect->left;
    INT textHeight = textRect->bottom - textRect->top;
    INT imageWidth = imageRect->right - imageRect->left;
    INT imageHeight = imageRect->bottom - imageRect->top;

    if ((style & BS_CENTER) == BS_RIGHT)
        OffsetRect(&rect, textWidth, 0);
    else if ((style & BS_CENTER) == BS_LEFT)
        OffsetRect(&rect, -imageWidth, 0);
    else if ((style & BS_VCENTER) == BS_BOTTOM)
        OffsetRect(&rect, 0, textHeight);
    else if ((style & BS_VCENTER) == BS_TOP)
        OffsetRect(&rect, 0, -imageHeight);
    else
        OffsetRect(&rect, -imageWidth, 0);

    UnionRect(&labelRect, textRect, &rect);
    return labelRect;
}

/* Position a rectangle inside a bounding rectangle according to button alignment flags */
static void BUTTON_PositionRect(LONG style, const RECT *outerRect, RECT *innerRect, const RECT *margin)
{
    INT width = innerRect->right - innerRect->left;
    INT height = innerRect->bottom - innerRect->top;

    if ((style & WS_EX_RIGHT) && !(style & BS_CENTER)) style |= BS_CENTER;

    if (!(style & BS_CENTER))
    {
        if (button_centers_text(style))
            style |= BS_CENTER;
        else
            style |= BS_LEFT;
    }

    if (!(style & BS_VCENTER))
    {
        /* Group box's text is top aligned by default */
        if (get_button_type(style) == BS_GROUPBOX)
            style |= BS_TOP;
    }

    switch (style & BS_CENTER)
    {
    case BS_CENTER:
        innerRect->left = outerRect->left + (outerRect->right - outerRect->left - width) / 2;
        innerRect->right = innerRect->left + width;
        break;
    case BS_RIGHT:
        innerRect->right = outerRect->right - margin->right;
        innerRect->left = innerRect->right - width;
        break;
    case BS_LEFT:
    default:
        innerRect->left = outerRect->left + margin->left;
        innerRect->right = innerRect->left + width;
        break;
    }

    switch (style & BS_VCENTER)
    {
    case BS_TOP:
        innerRect->top = outerRect->top + margin->top;
        innerRect->bottom = innerRect->top + height;
        break;
    case BS_BOTTOM:
        innerRect->bottom = outerRect->bottom - margin->bottom;
        innerRect->top = innerRect->bottom - height;
        break;
    case BS_VCENTER:
    default:
        innerRect->top = outerRect->top + (outerRect->bottom - outerRect->top - height) / 2;
        innerRect->bottom = innerRect->top + height;
        break;
    }
}

/* Convert imagelist align style to button align style */
static UINT BUTTON_ILStoBS(UINT align)
{
    switch (align)
    {
    case BUTTON_IMAGELIST_ALIGN_TOP:
        return BS_CENTER | BS_TOP;
    case BUTTON_IMAGELIST_ALIGN_BOTTOM:
        return BS_CENTER | BS_BOTTOM;
    case BUTTON_IMAGELIST_ALIGN_CENTER:
        return BS_CENTER | BS_VCENTER;
    case BUTTON_IMAGELIST_ALIGN_RIGHT:
        return BS_RIGHT | BS_VCENTER;
    case BUTTON_IMAGELIST_ALIGN_LEFT:
    default:
        return BS_LEFT | BS_VCENTER;
    }
}

static SIZE BUTTON_GetImageSize(const BUTTON_INFO *infoPtr)
{
    ICONINFO iconInfo;
    BITMAP bm = {0};
    SIZE size = {0};

    /* ImageList has priority over image */
    if (infoPtr->imagelist.himl)
        ImageList_GetIconSize(infoPtr->imagelist.himl, &size.cx, &size.cy);
    else if (infoPtr->u.image)
    {
        if (infoPtr->image_type == IMAGE_ICON)
        {
            GetIconInfo(infoPtr->u.icon, &iconInfo);
            GetObjectW(iconInfo.hbmColor, sizeof(bm), &bm);
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
        }
        else if (infoPtr->image_type == IMAGE_BITMAP)
            GetObjectW(infoPtr->u.bitmap, sizeof(bm), &bm);

        size.cx = bm.bmWidth;
        size.cy = bm.bmHeight;
    }

    return size;
}

static const RECT *BUTTON_GetTextMargin(const BUTTON_INFO *infoPtr)
{
    static const RECT oneMargin = {1, 1, 1, 1};

    /* Use text margin only when showing both image and text, and image is not imagelist */
    if (show_image_and_text(infoPtr) && !infoPtr->imagelist.himl)
        return &infoPtr->text_margin;
    else
        return &oneMargin;
}

static void BUTTON_GetClientRectSize(BUTTON_INFO *infoPtr, SIZE *size)
{
    RECT rect;
    GetClientRect(infoPtr->hwnd, &rect);
    size->cx = rect.right - rect.left;
    size->cy = rect.bottom - rect.top;
}

static void BUTTON_GetTextIdealSize(BUTTON_INFO *infoPtr, LONG maxWidth, SIZE *size)
{
    WCHAR *text = get_button_text(infoPtr);
    HDC hdc;
    RECT rect;
    const RECT *margin = BUTTON_GetTextMargin(infoPtr);

    if (maxWidth != 0)
    {
        maxWidth -= margin->right + margin->right;
        if (maxWidth <= 0) maxWidth = 1;
    }

    hdc = GetDC(infoPtr->hwnd);
    rect = BUTTON_GetTextRect(infoPtr, hdc, text, maxWidth);
    ReleaseDC(infoPtr->hwnd, hdc);
    heap_free(text);

    size->cx = rect.right - rect.left + margin->left + margin->right;
    size->cy = rect.bottom - rect.top + margin->top + margin->bottom;
}

static void BUTTON_GetLabelIdealSize(BUTTON_INFO *infoPtr, LONG maxWidth, SIZE *size)
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    SIZE imageSize;
    SIZE textSize;
    BOOL horizontal;

    imageSize = BUTTON_GetImageSize(infoPtr);
    if (infoPtr->imagelist.himl)
    {
        imageSize.cx += infoPtr->imagelist.margin.left + infoPtr->imagelist.margin.right;
        imageSize.cy += infoPtr->imagelist.margin.top + infoPtr->imagelist.margin.bottom;
        if (infoPtr->imagelist.uAlign == BUTTON_IMAGELIST_ALIGN_TOP
            || infoPtr->imagelist.uAlign == BUTTON_IMAGELIST_ALIGN_BOTTOM)
            horizontal = FALSE;
        else
            horizontal = TRUE;
    }
    else
    {
        /* horizontal alignment flags has priority over vertical ones if both are specified */
        if (!(style & (BS_CENTER | BS_VCENTER)) || ((style & BS_CENTER) && (style & BS_CENTER) != BS_CENTER)
            || !(style & BS_VCENTER) || (style & BS_VCENTER) == BS_VCENTER)
            horizontal = TRUE;
        else
            horizontal = FALSE;
    }

    if (horizontal)
    {
        if (maxWidth != 0)
        {
            maxWidth -= imageSize.cx;
            if (maxWidth <= 0) maxWidth = 1;
        }
        BUTTON_GetTextIdealSize(infoPtr, maxWidth, &textSize);
        size->cx = textSize.cx + imageSize.cx;
        size->cy = max(textSize.cy, imageSize.cy);
    }
    else
    {
        BUTTON_GetTextIdealSize(infoPtr, maxWidth, &textSize);
        size->cx = max(textSize.cx, imageSize.cx);
        size->cy = textSize.cy + imageSize.cy;
    }
}

static BOOL GB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size)
{
    BUTTON_GetClientRectSize(infoPtr, size);
    return TRUE;
}

static BOOL CB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size)
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    HDC hdc;
    HFONT hfont;
    SIZE labelSize;
    INT textOffset;
    double scaleX;
    double scaleY;
    LONG checkboxWidth, checkboxHeight;
    LONG maxWidth = 0;

    if (SendMessageW(infoPtr->hwnd, WM_GETTEXTLENGTH, 0, 0) == 0)
    {
        BUTTON_GetClientRectSize(infoPtr, size);
        return TRUE;
    }

    hdc = GetDC(infoPtr->hwnd);
    scaleX = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0;
    scaleY = GetDeviceCaps(hdc, LOGPIXELSY) / 96.0;
    if ((hfont = infoPtr->font)) SelectObject(hdc, hfont);
    GetCharWidthW(hdc, '0', '0', &textOffset);
    textOffset /= 2;
    ReleaseDC(infoPtr->hwnd, hdc);

    checkboxWidth = 12 * scaleX + 1;
    checkboxHeight = 12 * scaleY + 1;
    if (size->cx)
    {
        maxWidth = size->cx - checkboxWidth - textOffset;
        if (maxWidth <= 0) maxWidth = 1;
    }

    /* Checkbox doesn't support both image(but not image list) and text */
    if (!(style & (BS_ICON | BS_BITMAP)) && infoPtr->u.image)
        BUTTON_GetTextIdealSize(infoPtr, maxWidth, &labelSize);
    else
        BUTTON_GetLabelIdealSize(infoPtr, maxWidth, &labelSize);

    size->cx = labelSize.cx + checkboxWidth + textOffset;
    size->cy = max(labelSize.cy, checkboxHeight);

    return TRUE;
}

static BOOL PB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size)
{
#ifdef __REACTOS__
    HTHEME theme = GetWindowTheme(infoPtr->hwnd);
    BOOL ret = BUTTON_GetIdealSize(infoPtr, theme, size);
    if (!ret)
    {
        RECT rect;
        GetClientRect(infoPtr->hwnd, &rect);
        size->cx = rect.right;
        size->cy = rect.bottom;
    }

    return TRUE;
#else
    SIZE labelSize;

    if (SendMessageW(infoPtr->hwnd, WM_GETTEXTLENGTH, 0, 0) == 0)
        BUTTON_GetClientRectSize(infoPtr, size);
    else
    {
        /* Ideal size include text size even if image only flags(BS_ICON, BS_BITMAP) are specified */
        BUTTON_GetLabelIdealSize(infoPtr, size->cx, &labelSize);

        size->cx = labelSize.cx;
        size->cy = labelSize.cy;
    }
    return TRUE;
#endif
}

static BOOL SB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size)
{
    LONG extra_width = infoPtr->glyph_size.cx * 2 + GetSystemMetrics(SM_CXEDGE);
    SIZE label_size;

    if (SendMessageW(infoPtr->hwnd, WM_GETTEXTLENGTH, 0, 0) == 0)
    {
        BUTTON_GetClientRectSize(infoPtr, size);
        size->cx = max(size->cx, extra_width);
    }
    else
    {
        BUTTON_GetLabelIdealSize(infoPtr, size->cx, &label_size);
        size->cx = label_size.cx + ((size->cx == 0) ? extra_width : 0);
        size->cy = label_size.cy;
    }
    return TRUE;
}

/**********************************************************************
 *       BUTTON_CalcLayoutRects
 *
 *   Calculates the rectangles of the button label(image and text) and its parts depending on a button's style.
 *
 * Returns flags to be passed to DrawText.
 * Calculated rectangle doesn't take into account button state
 * (pushed, etc.). If there is nothing to draw (no text/image) output
 * rectangle is empty, and return value is (UINT)-1.
 *
 * PARAMS:
 * infoPtr [I]   Button pointer
 * hdc     [I]   Handle to device context to draw to
 * labelRc [I/O] Input the rect the label to be positioned in, and output the label rect
 * imageRc [O]   Optional, output the image rect
 * textRc  [O]   Optional, output the text rect
 */
static UINT BUTTON_CalcLayoutRects(const BUTTON_INFO *infoPtr, HDC hdc, RECT *labelRc, RECT *imageRc, RECT *textRc)
{
   LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
   LONG ex_style = GetWindowLongW( infoPtr->hwnd, GWL_EXSTYLE );
   LONG split_style = infoPtr->imagelist.himl ? BUTTON_ILStoBS(infoPtr->imagelist.uAlign) : style;
   WCHAR *text = get_button_text(infoPtr);
   SIZE imageSize = BUTTON_GetImageSize(infoPtr);
   UINT dtStyle = BUTTON_BStoDT(style, ex_style);
   RECT labelRect, imageRect, imageRectWithMargin, textRect;
   LONG imageMarginWidth, imageMarginHeight;
   const RECT *textMargin = BUTTON_GetTextMargin(infoPtr);
   RECT emptyMargin = {0};
   LONG maxTextWidth;

#ifdef __REACTOS__ // FIXME: Is this the correct location?
   if (infoPtr->ui_state & UISF_HIDEACCEL)
      dtStyle |= DT_HIDEPREFIX;
#endif

   /* Calculate label rectangle according to label type */
   if ((imageSize.cx == 0 && imageSize.cy == 0) && (text == NULL || text[0] == '\0'))
   {
       SetRectEmpty(labelRc);
       SetRectEmpty(imageRc);
       SetRectEmpty(textRc);
       heap_free(text);
       return (UINT)-1;
   }

   SetRect(&imageRect, 0, 0, imageSize.cx, imageSize.cy);
   imageRectWithMargin = imageRect;
   if (infoPtr->imagelist.himl)
   {
       imageRectWithMargin.top -= infoPtr->imagelist.margin.top;
       imageRectWithMargin.bottom += infoPtr->imagelist.margin.bottom;
       imageRectWithMargin.left -= infoPtr->imagelist.margin.left;
       imageRectWithMargin.right += infoPtr->imagelist.margin.right;
   }

   /* Show image only */
   if (show_image_only(infoPtr))
   {
       BUTTON_PositionRect(style, labelRc, &imageRect,
                           infoPtr->imagelist.himl ? &infoPtr->imagelist.margin : &emptyMargin);
       labelRect = imageRect;
       SetRectEmpty(&textRect);
   }
   else
   {
       /* Get text rect */
       maxTextWidth = labelRc->right - labelRc->left;
       textRect = BUTTON_GetTextRect(infoPtr, hdc, text, maxTextWidth);

       /* Show image and text */
       if (show_image_and_text(infoPtr))
       {
           RECT boundingLabelRect, boundingImageRect, boundingTextRect;

           /* Get label rect */
           /* Image list may have different alignment than the button, use the whole rect for label in this case */
           if (infoPtr->imagelist.himl)
               labelRect = *labelRc;
           else
           {
               /* Get a label bounding rectangle to position the label in the user specified label rectangle because
                * text and image need to align together. */
               boundingLabelRect = BUTTON_GetBoundingLabelRect(split_style, &textRect, &imageRectWithMargin);
               BUTTON_PositionRect(split_style, labelRc, &boundingLabelRect, &emptyMargin);
               labelRect = boundingLabelRect;
           }

           /* When imagelist has center align, use the whole rect for imagelist and text */
           if(infoPtr->imagelist.himl && infoPtr->imagelist.uAlign == BUTTON_IMAGELIST_ALIGN_CENTER)
           {
               boundingImageRect = labelRect;
               boundingTextRect = labelRect;
               BUTTON_PositionRect(split_style, &boundingImageRect, &imageRect,
                                   infoPtr->imagelist.himl ? &infoPtr->imagelist.margin : &emptyMargin);
               /* Text doesn't use imagelist align */
               BUTTON_PositionRect(style, &boundingTextRect, &textRect, textMargin);
           }
           else
           {
               /* Get image rect */
               /* Split the label rect to two halves as two bounding rectangles for image and text */
               boundingImageRect = labelRect;
               imageMarginWidth = imageRectWithMargin.right - imageRectWithMargin.left;
               imageMarginHeight = imageRectWithMargin.bottom - imageRectWithMargin.top;
               if ((split_style & BS_CENTER) == BS_RIGHT)
                   boundingImageRect.left = boundingImageRect.right - imageMarginWidth;
               else if ((split_style & BS_CENTER) == BS_LEFT)
                   boundingImageRect.right = boundingImageRect.left + imageMarginWidth;
               else if ((split_style & BS_VCENTER) == BS_BOTTOM)
                   boundingImageRect.top = boundingImageRect.bottom - imageMarginHeight;
               else if ((split_style & BS_VCENTER) == BS_TOP)
                   boundingImageRect.bottom = boundingImageRect.top + imageMarginHeight;
               else
                   boundingImageRect.right = boundingImageRect.left + imageMarginWidth;
               BUTTON_PositionRect(split_style, &boundingImageRect, &imageRect,
                                   infoPtr->imagelist.himl ? &infoPtr->imagelist.margin : &emptyMargin);

               /* Get text rect */
               SubtractRect(&boundingTextRect, &labelRect, &boundingImageRect);
               /* Text doesn't use imagelist align */
               BUTTON_PositionRect(style, &boundingTextRect, &textRect, textMargin);
           }
       }
       /* Show text only */
       else
       {
           if (get_button_type(style) != BS_GROUPBOX)
               BUTTON_PositionRect(style, labelRc, &textRect, textMargin);
           else
               /* GroupBox is always top aligned */
               BUTTON_PositionRect((style & ~BS_VCENTER) | BS_TOP, labelRc, &textRect, textMargin);
#ifdef __REACTOS__
           /* Fix button text misalignment */
           textRect.top -= 1;
           textRect.bottom -= 1;
#endif
           labelRect = textRect;
           SetRectEmpty(&imageRect);
       }
   }
   heap_free(text);

   CopyRect(labelRc, &labelRect);
   CopyRect(imageRc, &imageRect);
   CopyRect(textRc, &textRect);

   return dtStyle;
}


/**********************************************************************
 *       BUTTON_DrawImage
 *
 *   Draw the button's image into the specified rectangle.
 */
static void BUTTON_DrawImage(const BUTTON_INFO *infoPtr, HDC hdc, HBRUSH hbr, UINT flags, const RECT *rect)
{
    if (infoPtr->imagelist.himl)
    {
        int i = (ImageList_GetImageCount(infoPtr->imagelist.himl) == 1) ? 0 : get_draw_state(infoPtr) - 1;

        ImageList_Draw(infoPtr->imagelist.himl, i, hdc, rect->left, rect->top, ILD_NORMAL);
    }
    else
    {
        switch (infoPtr->image_type)
        {
        case IMAGE_ICON:
            flags |= DST_ICON;
            break;
        case IMAGE_BITMAP:
            flags |= DST_BITMAP;
            break;
        default:
            return;
        }

        DrawStateW(hdc, hbr, NULL, (LPARAM)infoPtr->u.image, 0, rect->left, rect->top,
                   rect->right - rect->left, rect->bottom - rect->top, flags);
    }
}


/**********************************************************************
 *       BUTTON_DrawTextCallback
 *
 *   Callback function used by DrawStateW function.
 */
static BOOL CALLBACK BUTTON_DrawTextCallback(HDC hdc, LPARAM lp, WPARAM wp, int cx, int cy)
{
   RECT rc;

   SetRect(&rc, 0, 0, cx, cy);
   DrawTextW(hdc, (LPCWSTR)lp, -1, &rc, (UINT)wp);
   return TRUE;
}

/**********************************************************************
 *       BUTTON_DrawLabel
 *
 *   Common function for drawing button label.
 *
 * FIXME:
 *      1. When BS_SINGLELINE is specified and text contains '\t', '\n' or '\r' in the middle, they are rendered as
 *         squares now whereas they should be ignored.
 *      2. When BS_MULTILINE is specified and text contains space in the middle, the space mistakenly be rendered as newline.
 */
static void BUTTON_DrawLabel(const BUTTON_INFO *infoPtr, HDC hdc, UINT dtFlags, const RECT *imageRect,
                             const RECT *textRect)
{
   HBRUSH hbr = 0;
   UINT flags = IsWindowEnabled(infoPtr->hwnd) ? DSS_NORMAL : DSS_DISABLED;
   LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
   WCHAR *text;

   /* FIXME: To draw disabled label in Win31 look-and-feel, we probably
    * must use DSS_MONO flag and COLOR_GRAYTEXT brush (or maybe DSS_UNION).
    * I don't have Win31 on hand to verify that, so I leave it as is.
    */

   if ((style & BS_PUSHLIKE) && (infoPtr->state & BST_INDETERMINATE))
   {
      hbr = GetSysColorBrush(COLOR_GRAYTEXT);
      flags |= DSS_MONO;
   }

   if (show_image(infoPtr)) BUTTON_DrawImage(infoPtr, hdc, hbr, flags, imageRect);
   if (show_image_only(infoPtr)) return;

   /* DST_COMPLEX -- is 0 */
   if (!(text = get_button_text(infoPtr))) return;
#ifdef __REACTOS__
   if (dtFlags & DT_HIDEPREFIX)
       flags |= DSS_HIDEPREFIX;
#endif
   DrawStateW(hdc, hbr, BUTTON_DrawTextCallback, (LPARAM)text, dtFlags, textRect->left, textRect->top,
              textRect->right - textRect->left, textRect->bottom - textRect->top, flags);
   heap_free(text);
}

/**********************************************************************
 *       Push Button Functions
 */
static void PB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT     rc, labelRect, imageRect, textRect;
    UINT     dtFlags, uState;
    HPEN     hOldPen, hpen;
    HBRUSH   hOldBrush;
    INT      oldBkMode;
    COLORREF oldTxtColor;
    LRESULT  cdrf;
    HFONT hFont;
    NMCUSTOMDRAW nmcd;
    LONG state = infoPtr->state;
    LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
    BOOL pushedState = (state & BST_PUSHED);
    HWND parent;
    HRGN hrgn;

    GetClientRect( infoPtr->hwnd, &rc );

    /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if ((hFont = infoPtr->font)) SelectObject( hDC, hFont );
    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    SendMessageW( parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd );

    hrgn = set_control_clipping( hDC, &rc );

    hpen = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    hOldPen = SelectObject(hDC, hpen);
    hOldBrush = SelectObject(hDC,GetSysColorBrush(COLOR_BTNFACE));
    oldBkMode = SetBkMode(hDC, TRANSPARENT);

    init_custom_draw(&nmcd, infoPtr, hDC, &rc);

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (get_button_type(style) == BS_DEFPUSHBUTTON)
    {
        if (action != ODA_FOCUS)
            Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
	InflateRect( &rc, -1, -1 );
    }

    /* Skip the frame drawing if only focus has changed */
    if (action != ODA_FOCUS)
    {
        uState = DFCS_BUTTONPUSH;

        if (style & BS_FLAT)
            uState |= DFCS_MONO;
        else if (pushedState)
        {
            if (get_button_type(style) == BS_DEFPUSHBUTTON )
                uState |= DFCS_FLAT;
            else
                uState |= DFCS_PUSHED;
        }

        if (state & (BST_CHECKED | BST_INDETERMINATE))
            uState |= DFCS_CHECKED;

        DrawFrameControl( hDC, &rc, DFC_BUTTON, uState );
    }

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (!(cdrf & CDRF_DOERASE) && action != ODA_FOCUS)
    {
        /* draw button label */
        labelRect = rc;
        /* Shrink label rect at all sides by 2 so that the content won't touch the surrounding frame */
        InflateRect(&labelRect, -2, -2);
        dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &labelRect, &imageRect, &textRect);

        if (dtFlags != (UINT)-1L)
        {
            if (pushedState) OffsetRect(&labelRect, 1, 1);

            oldTxtColor = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );

            BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &imageRect, &textRect);

            SetTextColor( hDC, oldTxtColor );
        }
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if ((cdrf & CDRF_SKIPPOSTPAINT) || dtFlags == (UINT)-1L) goto cleanup;

    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
#ifdef __REACTOS__
        if (!(infoPtr->ui_state & UISF_HIDEFOCUS))
        {
#endif
            InflateRect( &rc, -2, -2 );
            DrawFocusRect( hDC, &rc );
#ifdef __REACTOS__
        }
#endif
    }

 cleanup:
    SelectObject( hDC, hOldPen );
    SelectObject( hDC, hOldBrush );
    SetBkMode(hDC, oldBkMode);
    SelectClipRgn( hDC, hrgn );
    if (hrgn) DeleteObject( hrgn );
    DeleteObject( hpen );
}

/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT rbox, labelRect, imageRect, textRect, client;
    HBRUSH hBrush;
    int delta, text_offset, checkBoxWidth, checkBoxHeight;
    UINT dtFlags;
    LRESULT cdrf;
    HFONT hFont;
    NMCUSTOMDRAW nmcd;
    LONG state = infoPtr->state;
    LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
    LONG ex_style = GetWindowLongW( infoPtr->hwnd, GWL_EXSTYLE );
    HWND parent;
    HRGN hrgn;

    if (style & BS_PUSHLIKE)
    {
        PB_Paint( infoPtr, hDC, action );
	return;
    }

    GetClientRect(infoPtr->hwnd, &client);
    rbox = labelRect = client;

    checkBoxWidth  = 12 * GetDpiForWindow( infoPtr->hwnd ) / 96 + 1;
    checkBoxHeight = 12 * GetDpiForWindow( infoPtr->hwnd ) / 96 + 1;

    if ((hFont = infoPtr->font)) SelectObject( hDC, hFont );
    GetCharWidthW( hDC, '0', '0', &text_offset );
    text_offset /= 2;

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    hrgn = set_control_clipping( hDC, &client );

    if (style & BS_LEFTTEXT || ex_style & WS_EX_RIGHT)
    {
        labelRect.right -= checkBoxWidth + text_offset;
        rbox.left = rbox.right - checkBoxWidth;
    }
    else
    {
        labelRect.left += checkBoxWidth + text_offset;
        rbox.right = checkBoxWidth;
    }

    init_custom_draw(&nmcd, infoPtr, hDC, &client);

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    /* Since WM_ERASEBKGND does nothing, first prepare background */
    if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
    if (action == ODA_DRAWENTIRE) FillRect( hDC, &client, hBrush );
    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Draw label */
    client = labelRect;
    dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &labelRect, &imageRect, &textRect);

    /* Only adjust rbox when rtext is valid */
    if (dtFlags != (UINT)-1L)
    {
        rbox.top = labelRect.top;
        rbox.bottom = labelRect.bottom;
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    /* Draw the check-box bitmap */
    if (!(cdrf & CDRF_DOERASE))
    {
        if (action == ODA_DRAWENTIRE || action == ODA_SELECT)
        {
            UINT flags;

            if ((get_button_type(style) == BS_RADIOBUTTON) ||
                (get_button_type(style) == BS_AUTORADIOBUTTON)) flags = DFCS_BUTTONRADIO;
            else if (state & BST_INDETERMINATE) flags = DFCS_BUTTON3STATE;
            else flags = DFCS_BUTTONCHECK;

            if (state & (BST_CHECKED | BST_INDETERMINATE)) flags |= DFCS_CHECKED;
            if (state & BST_PUSHED)  flags |= DFCS_PUSHED;
            if (style & WS_DISABLED) flags |= DFCS_INACTIVE;

            /* rbox must have the correct height */
            delta = rbox.bottom - rbox.top - checkBoxHeight;

            if ((style & BS_VCENTER) == BS_TOP)
            {
                if (delta > 0)
                    rbox.bottom = rbox.top + checkBoxHeight;
                else
                {
                    rbox.top -= -delta / 2 + 1;
                    rbox.bottom = rbox.top + checkBoxHeight;
                }
            }
            else if ((style & BS_VCENTER) == BS_BOTTOM)
            {
                if (delta > 0)
                    rbox.top = rbox.bottom - checkBoxHeight;
                else
                {
                    rbox.bottom += -delta / 2 + 1;
                    rbox.top = rbox.bottom - checkBoxHeight;
                }
            }
            else  /* Default */
            {
                if (delta > 0)
                {
                    int ofs = delta / 2;
                    rbox.bottom -= ofs + 1;
                    rbox.top = rbox.bottom - checkBoxHeight;
                }
                else if (delta < 0)
                {
                    int ofs = -delta / 2;
                    rbox.top -= ofs + 1;
                    rbox.bottom = rbox.top + checkBoxHeight;
                }
            }

            DrawFrameControl(hDC, &rbox, DFC_BUTTON, flags);
        }

        if (dtFlags != (UINT)-1L) /* Something to draw */
            if (action == ODA_DRAWENTIRE) BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &imageRect, &textRect);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if ((cdrf & CDRF_SKIPPOSTPAINT) || dtFlags == (UINT)-1L) goto cleanup;

    /* ... and focus */
    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
        labelRect.left--;
        labelRect.right++;
        IntersectRect(&labelRect, &labelRect, &client);
        DrawFocusRect(hDC, &labelRect);
    }

cleanup:
    SelectClipRgn( hDC, hrgn );
    if (hrgn) DeleteObject( hrgn );
}


/**********************************************************************
 *       BUTTON_CheckAutoRadioButton
 *
 * hwnd is checked, uncheck every other auto radio button in group
 */
static void BUTTON_CheckAutoRadioButton( HWND hwnd )
{
    HWND parent, sibling, start;

    parent = GetParent(hwnd);
    /* make sure that starting control is not disabled or invisible */
#ifdef __REACTOS__
    start = sibling = hwnd;
#else
    start = sibling = GetNextDlgGroupItem( parent, hwnd, TRUE );
#endif
    do
    {
        if (!sibling) break;
#ifdef __REACTOS__
        if ((SendMessageW(sibling, WM_GETDLGCODE, 0, 0) & (DLGC_BUTTON | DLGC_RADIOBUTTON)) == (DLGC_BUTTON | DLGC_RADIOBUTTON))
            SendMessageW( sibling, BM_SETCHECK, sibling == hwnd ? BST_CHECKED : BST_UNCHECKED, 0 );
#else
        if ((hwnd != sibling) &&
            ((GetWindowLongW( sibling, GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON))
            SendMessageW( sibling, BM_SETCHECK, BST_UNCHECKED, 0 );
#endif
        sibling = GetNextDlgGroupItem( parent, sibling, FALSE );
    } while (sibling != start);
}


/**********************************************************************
 *       Group Box Functions
 */

static void GB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT labelRect, imageRect, textRect, rcFrame;
    HBRUSH hbr;
    HFONT hFont;
    UINT dtFlags;
    TEXTMETRICW tm;
    LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
    HWND parent;
    HRGN hrgn;

    if ((hFont = infoPtr->font)) SelectObject( hDC, hFont );
    /* GroupBox acts like static control, so it sends CTLCOLORSTATIC */
    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    hbr = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    if (!hbr) /* did the app forget to call defwindowproc ? */
        hbr = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    GetClientRect(infoPtr->hwnd, &labelRect);
    rcFrame = labelRect;
    hrgn = set_control_clipping(hDC, &labelRect);

    GetTextMetricsW (hDC, &tm);
    rcFrame.top += (tm.tmHeight / 2) - 1;
    DrawEdge (hDC, &rcFrame, EDGE_ETCHED, BF_RECT | ((style & BS_FLAT) ? BF_FLAT : 0));

    InflateRect(&labelRect, -7, 1);
    dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &labelRect, &imageRect, &textRect);

    if (dtFlags != (UINT)-1)
    {
        /* Because buttons have CS_PARENTDC class style, there is a chance
         * that label will be drawn out of client rect.
         * But Windows doesn't clip label's rect, so do I.
         */

        /* There is 1-pixel margin at the left, right, and bottom */
        labelRect.left--;
        labelRect.right++;
        labelRect.bottom++;
        FillRect(hDC, &labelRect, hbr);
        labelRect.left++;
        labelRect.right--;
        labelRect.bottom--;

        BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &imageRect, &textRect);
    }
    SelectClipRgn( hDC, hrgn );
    if (hrgn) DeleteObject( hrgn );
}


/**********************************************************************
 *       User Button Functions
 */

static void UB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT rc;
    HBRUSH hBrush;
    LRESULT cdrf;
    HFONT hFont;
    NMCUSTOMDRAW nmcd;
    LONG state = infoPtr->state;
    HWND parent;

    GetClientRect( infoPtr->hwnd, &rc);

    if ((hFont = infoPtr->font)) SelectObject( hDC, hFont );

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);

    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
        init_custom_draw(&nmcd, infoPtr, hDC, &rc);

        /* Send erase notifications */
        cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
        if (cdrf & CDRF_SKIPDEFAULT) goto notify;
    }

    FillRect( hDC, &rc, hBrush );
    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
        if (cdrf & CDRF_NOTIFYPOSTERASE)
        {
            nmcd.dwDrawStage = CDDS_POSTERASE;
            SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
        }

        /* Send paint notifications */
        nmcd.dwDrawStage = CDDS_PREPAINT;
        cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
        if (cdrf & CDRF_SKIPDEFAULT) goto notify;
        if (cdrf & CDRF_NOTIFYPOSTPAINT)
        {
            nmcd.dwDrawStage = CDDS_POSTPAINT;
            SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
        }

        if (!(cdrf & CDRF_SKIPPOSTPAINT))
            DrawFocusRect( hDC, &rc );
    }

notify:
    switch (action)
    {
    case ODA_FOCUS:
        BUTTON_NOTIFY_PARENT( infoPtr->hwnd, (state & BST_FOCUS) ? BN_SETFOCUS : BN_KILLFOCUS );
        break;

    case ODA_SELECT:
        BUTTON_NOTIFY_PARENT( infoPtr->hwnd, (state & BST_PUSHED) ? BN_HILITE : BN_UNHILITE );
        break;

    default:
        break;
    }
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    LONG state = infoPtr->state;
    DRAWITEMSTRUCT dis;
    LONG_PTR id = GetWindowLongPtrW( infoPtr->hwnd, GWLP_ID );
    HWND parent;
    HFONT hFont;
    HRGN hrgn;

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = id;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = ((state & BST_FOCUS) ? ODS_FOCUS : 0) |
                     ((state & BST_PUSHED) ? ODS_SELECTED : 0) |
                     (IsWindowEnabled(infoPtr->hwnd) ? 0: ODS_DISABLED);
    dis.hwndItem   = infoPtr->hwnd;
    dis.hDC        = hDC;
    dis.itemData   = 0;
    GetClientRect( infoPtr->hwnd, &dis.rcItem );

    if ((hFont = infoPtr->font)) SelectObject( hDC, hFont );
    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    SendMessageW( parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd );

    hrgn = set_control_clipping( hDC, &dis.rcItem );

    SendMessageW( GetParent(infoPtr->hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
    SelectClipRgn( hDC, hrgn );
    if (hrgn) DeleteObject( hrgn );
}


/**********************************************************************
 *       Split Button Functions
 */
static void SB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    LONG state = infoPtr->state;
    UINT dtFlags = (UINT)-1L;

    RECT rc, push_rect, dropdown_rect;
    NMCUSTOMDRAW nmcd;
    HPEN pen, old_pen;
    HBRUSH old_brush;
    INT old_bk_mode;
    LRESULT cdrf;
    HWND parent;
    HRGN hrgn;

    GetClientRect(infoPtr->hwnd, &rc);

    /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if (infoPtr->font) SelectObject(hDC, infoPtr->font);
    if (!(parent = GetParent(infoPtr->hwnd))) parent = infoPtr->hwnd;
    SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);

    hrgn = set_control_clipping(hDC, &rc);

    pen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_WINDOWFRAME));
    old_pen = SelectObject(hDC, pen);
    old_brush = SelectObject(hDC, GetSysColorBrush(COLOR_BTNFACE));
    old_bk_mode = SetBkMode(hDC, TRANSPARENT);

    init_custom_draw(&nmcd, infoPtr, hDC, &rc);

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (get_button_type(style) == BS_DEFSPLITBUTTON)
    {
        if (action != ODA_FOCUS)
            Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        InflateRect(&rc, -1, -1);
        /* The split will now be off by 1 pixel, but
           that's exactly what Windows does as well */
    }

    get_split_button_rects(infoPtr, &rc, &push_rect, &dropdown_rect);
    if (infoPtr->split_style & BCSS_NOSPLIT)
        push_rect = rc;

    /* Skip the frame drawing if only focus has changed */
    if (action != ODA_FOCUS)
    {
        UINT flags = DFCS_BUTTONPUSH;

        if (style & BS_FLAT) flags |= DFCS_MONO;
        else if (state & BST_PUSHED)
            flags |= (get_button_type(style) == BS_DEFSPLITBUTTON)
                     ? DFCS_FLAT : DFCS_PUSHED;

        if (state & (BST_CHECKED | BST_INDETERMINATE))
            flags |= DFCS_CHECKED;

        if (infoPtr->split_style & BCSS_NOSPLIT)
            DrawFrameControl(hDC, &push_rect, DFC_BUTTON, flags);
        else
        {
            UINT dropdown_flags = flags & ~DFCS_CHECKED;

            if (state & BST_DROPDOWNPUSHED)
                dropdown_flags = (dropdown_flags & ~DFCS_FLAT) | DFCS_PUSHED;

            /* Adjust for shadow and draw order so it looks properly */
            if (infoPtr->split_style & BCSS_ALIGNLEFT)
            {
                dropdown_rect.right++;
                DrawFrameControl(hDC, &dropdown_rect, DFC_BUTTON, dropdown_flags);
                dropdown_rect.right--;
                DrawFrameControl(hDC, &push_rect, DFC_BUTTON, flags);
            }
            else
            {
                push_rect.right++;
                DrawFrameControl(hDC, &push_rect, DFC_BUTTON, flags);
                push_rect.right--;
                DrawFrameControl(hDC, &dropdown_rect, DFC_BUTTON, dropdown_flags);
            }
        }
    }

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    /* Shrink push button rect so that the content won't touch the surrounding frame */
    InflateRect(&push_rect, -2, -2);

    if (!(cdrf & CDRF_DOERASE) && action != ODA_FOCUS)
    {
        COLORREF old_color = SetTextColor(hDC, GetSysColor(COLOR_BTNTEXT));
        RECT label_rect = push_rect, image_rect, text_rect;

        dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &label_rect, &image_rect, &text_rect);

        if (dtFlags != (UINT)-1L)
            BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &image_rect, &text_rect);

        draw_split_button_dropdown_glyph(infoPtr, hDC, &dropdown_rect);
        SetTextColor(hDC, old_color);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if ((cdrf & CDRF_SKIPPOSTPAINT) || dtFlags == (UINT)-1L) goto cleanup;

    if (action == ODA_FOCUS || (state & BST_FOCUS))
        DrawFocusRect(hDC, &push_rect);

cleanup:
    SelectObject(hDC, old_pen);
    SelectObject(hDC, old_brush);
    SetBkMode(hDC, old_bk_mode);
    SelectClipRgn(hDC, hrgn);
    if (hrgn) DeleteObject(hrgn);
    DeleteObject(pen);
}

/* Given the full button rect of the split button, retrieve the push part and the dropdown part */
static inline void get_split_button_rects(const BUTTON_INFO *infoPtr, const RECT *button_rect,
                                          RECT *push_rect, RECT *dropdown_rect)
{
    *push_rect = *dropdown_rect = *button_rect;

    /* The dropdown takes priority if the client rect is too small, it will only have a dropdown */
    if (infoPtr->split_style & BCSS_ALIGNLEFT)
    {
        dropdown_rect->right = min(button_rect->left + infoPtr->glyph_size.cx, button_rect->right);
        push_rect->left = dropdown_rect->right;
    }
    else
    {
        dropdown_rect->left = max(button_rect->right - infoPtr->glyph_size.cx, button_rect->left);
        push_rect->right = dropdown_rect->left;
    }
}

/* Notify the parent if the point is within the dropdown and return TRUE (always notify if NULL) */
static BOOL notify_split_button_dropdown(const BUTTON_INFO *infoPtr, const POINT *pt, HWND hwnd)
{
    NMBCDROPDOWN nmbcd;

    GetClientRect(hwnd, &nmbcd.rcButton);
    if (pt)
    {
        RECT push_rect, dropdown_rect;

        get_split_button_rects(infoPtr, &nmbcd.rcButton, &push_rect, &dropdown_rect);
        if (!PtInRect(&dropdown_rect, *pt))
            return FALSE;

        /* If it's already down (set manually via BCM_SETDROPDOWNSTATE), fake the notify */
        if (infoPtr->state & BST_DROPDOWNPUSHED)
            return TRUE;
    }
    SendMessageW(hwnd, BCM_SETDROPDOWNSTATE, TRUE, 0);

    nmbcd.hdr.hwndFrom = hwnd;
    nmbcd.hdr.idFrom   = GetWindowLongPtrW(hwnd, GWLP_ID);
    nmbcd.hdr.code     = BCN_DROPDOWN;
    SendMessageW(GetParent(hwnd), WM_NOTIFY, nmbcd.hdr.idFrom, (LPARAM)&nmbcd);

    SendMessageW(hwnd, BCM_SETDROPDOWNSTATE, FALSE, 0);
    return TRUE;
}

/* Draw the split button dropdown glyph or image */
static void draw_split_button_dropdown_glyph(const BUTTON_INFO *infoPtr, HDC hdc, RECT *rect)
{
    if (infoPtr->split_style & BCSS_IMAGE)
    {
        int w, h;

        /* When the glyph is an image list, Windows is very buggy with BCSS_STRETCH,
           positions it weirdly and doesn't even stretch it, but instead extends the
           image, leaking into other images in the list (or black if none). Instead,
           we'll ignore this and just position it at center as without BCSS_STRETCH. */
        if (!ImageList_GetIconSize(infoPtr->glyph, &w, &h)) return;

        ImageList_Draw(infoPtr->glyph,
                       (ImageList_GetImageCount(infoPtr->glyph) == 1) ? 0 : get_draw_state(infoPtr) - 1,
                       hdc, rect->left + (rect->right  - rect->left - w) / 2,
                            rect->top  + (rect->bottom - rect->top  - h) / 2, ILD_NORMAL);
    }
    else if (infoPtr->glyph_size.cy >= 0)
    {
        /* infoPtr->glyph is a character code from Marlett */
        HFONT font, old_font;
        LOGFONTW logfont = { 0, 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0,
                             { 'M','a','r','l','e','t','t',0 } };
        if (infoPtr->glyph_size.cy)
        {
            /* BCSS_STRETCH preserves aspect ratio, uses minimum as size */
            if (infoPtr->split_style & BCSS_STRETCH)
                logfont.lfHeight = min(infoPtr->glyph_size.cx, infoPtr->glyph_size.cy);
            else
            {
                logfont.lfWidth  = infoPtr->glyph_size.cx;
                logfont.lfHeight = infoPtr->glyph_size.cy;
            }
        }
        else logfont.lfHeight = infoPtr->glyph_size.cx;

        if ((font = CreateFontIndirectW(&logfont)))
        {
            old_font = SelectObject(hdc, font);
            DrawTextW(hdc, (const WCHAR*)&infoPtr->glyph, 1, rect,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX);
            SelectObject(hdc, old_font);
            DeleteObject(font);
        }
    }
}


/**********************************************************************
 *       Themed Paint Functions
 */
#ifdef __REACTOS__ /* r73885 */
static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void PB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
#endif
{
    RECT bgRect, textRect;
    HFONT font = infoPtr->font;
    HFONT hPrevFont = font ? SelectObject(hDC, font) : NULL;
    NMCUSTOMDRAW nmcd;
    LRESULT cdrf;
    HWND parent;
    WCHAR *text;

    GetClientRect(infoPtr->hwnd, &bgRect);
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &bgRect, &textRect);
    init_custom_draw(&nmcd, infoPtr, hDC, &bgRect);

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &bgRect, NULL);

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (!(cdrf & CDRF_DOERASE) && (text = get_button_text(infoPtr)))
    {
        DrawThemeText(theme, hDC, BP_PUSHBUTTON, state, text, lstrlenW(text), dtFlags, 0, &textRect);
        heap_free(text);
#ifdef __REACTOS__
        text = NULL;
#endif
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if (cdrf & CDRF_SKIPPOSTPAINT) goto cleanup;

    if (focused)
    {
        MARGINS margins;
        RECT focusRect = bgRect;

        GetThemeMargins(theme, hDC, BP_PUSHBUTTON, state, TMT_CONTENTMARGINS, NULL, &margins);

        focusRect.left += margins.cxLeftWidth;
        focusRect.top += margins.cyTopHeight;
        focusRect.right -= margins.cxRightWidth;
        focusRect.bottom -= margins.cyBottomHeight;

        DrawFocusRect( hDC, &focusRect );
    }

cleanup:
#ifdef __REACTOS__
    if (text) heap_free(text);
#endif
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

#ifdef __REACTOS__ /* r73885 */
static void CB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void CB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
#endif
{
    SIZE sz;
    RECT bgRect, textRect;
    HFONT font, hPrevFont = NULL;
    DWORD dwStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    UINT btn_type = get_button_type( dwStyle );
    int part = (btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON) ? BP_RADIOBUTTON : BP_CHECKBOX;
    NMCUSTOMDRAW nmcd;
    LRESULT cdrf;
    LOGFONTW lf;
    HWND parent;
    WCHAR *text;
    BOOL created_font = FALSE;

    HRESULT hr = GetThemeFont(theme, hDC, part, state, TMT_FONT, &lf);
    if (SUCCEEDED(hr)) {
        font = CreateFontIndirectW(&lf);
        if (!font)
            TRACE("Failed to create font\n");
        else {
            TRACE("font = %s\n", debugstr_w(lf.lfFaceName));
            hPrevFont = SelectObject(hDC, font);
            created_font = TRUE;
        }
    } else {
        font = (HFONT)SendMessageW(infoPtr->hwnd, WM_GETFONT, 0, 0);
        hPrevFont = SelectObject(hDC, font);
    }

    if (FAILED(GetThemePartSize(theme, hDC, part, state, NULL, TS_DRAW, &sz)))
        sz.cx = sz.cy = 13;

    GetClientRect(infoPtr->hwnd, &bgRect);

    GetThemeBackgroundContentRect(theme, hDC, part, state, &bgRect, &textRect);
    init_custom_draw(&nmcd, infoPtr, hDC, &bgRect);

    if (dtFlags & DT_SINGLELINE) /* Center the checkbox / radio button to the text. */
        bgRect.top = bgRect.top + (textRect.bottom - textRect.top - sz.cy) / 2;

    /* adjust for the check/radio marker */
    bgRect.bottom = bgRect.top + sz.cy;
    bgRect.right = bgRect.left + sz.cx;
    textRect.left = bgRect.right + 6;

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    DrawThemeBackground(theme, hDC, part, state, &bgRect, NULL);

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    text = get_button_text(infoPtr);
    if (!(cdrf & CDRF_DOERASE) && text)
        DrawThemeText(theme, hDC, part, state, text, lstrlenW(text), dtFlags, 0, &textRect);

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    if (text)
    {
        if (!(cdrf & CDRF_SKIPPOSTPAINT) && focused)
        {
            RECT focusRect;

            focusRect = textRect;

            DrawTextW(hDC, text, lstrlenW(text), &focusRect, dtFlags | DT_CALCRECT);

            if (focusRect.right < textRect.right) focusRect.right++;
            focusRect.bottom = textRect.bottom;

            DrawFocusRect( hDC, &focusRect );
        }

        heap_free(text);
#ifdef __REACTOS__
        text = NULL;
#endif
    }

cleanup:
#ifdef __REACTOS__
    if (text) heap_free(text);
#endif
    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

#ifdef __REACTOS__ /* r73885 */
static void GB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void GB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
#endif
{
    RECT bgRect, textRect, contentRect;
    WCHAR *text = get_button_text(infoPtr);
    LOGFONTW lf;
    HFONT font, hPrevFont = NULL;
    BOOL created_font = FALSE;
#ifdef __REACTOS__ /* r74406 */
    HWND parent;
    HBRUSH hBrush;
    RECT clientRect;
#endif

    HRESULT hr = GetThemeFont(theme, hDC, BP_GROUPBOX, state, TMT_FONT, &lf);
    if (SUCCEEDED(hr)) {
        font = CreateFontIndirectW(&lf);
        if (!font)
            TRACE("Failed to create font\n");
        else {
            hPrevFont = SelectObject(hDC, font);
            created_font = TRUE;
        }
    } else {
#ifdef __REACTOS__ /* r73885 */
        font = infoPtr->font;
#else
        font = (HFONT)SendMessageW(infoPtr->hwnd, WM_GETFONT, 0, 0);
#endif
        hPrevFont = SelectObject(hDC, font);
    }

    GetClientRect(infoPtr->hwnd, &bgRect);
    textRect = bgRect;

    if (text)
    {
        SIZE textExtent;
        GetTextExtentPoint32W(hDC, text, lstrlenW(text), &textExtent);
        bgRect.top += (textExtent.cy / 2);
        textRect.left += 10;
        textRect.bottom = textRect.top + textExtent.cy;
        textRect.right = textRect.left + textExtent.cx + 4;

        ExcludeClipRect(hDC, textRect.left, textRect.top, textRect.right, textRect.bottom);
    }

    GetThemeBackgroundContentRect(theme, hDC, BP_GROUPBOX, state, &bgRect, &contentRect);
    ExcludeClipRect(hDC, contentRect.left, contentRect.top, contentRect.right, contentRect.bottom);

#ifdef __REACTOS__ /* r73885 & r74149 */
    if (prfFlag == 0)
#endif
    if (IsThemeBackgroundPartiallyTransparent(theme, BP_GROUPBOX, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);

#ifdef __REACTOS__ /* r74406 */
    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC,
                                  (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC,
                                       (WPARAM)hDC, (LPARAM)infoPtr->hwnd );
    GetClientRect(infoPtr->hwnd, &clientRect);
    FillRect( hDC, &clientRect, hBrush );
#endif

    DrawThemeBackground(theme, hDC, BP_GROUPBOX, state, &bgRect, NULL);

    SelectClipRgn(hDC, NULL);

    if (text)
    {
        InflateRect(&textRect, -2, 0);
        DrawThemeText(theme, hDC, BP_GROUPBOX, state, text, lstrlenW(text), 0, 0, &textRect);
        heap_free(text);
    }

    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

#ifdef __REACTOS__
static void SB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void SB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
#endif
{
    HFONT old_font = infoPtr->font ? SelectObject(hDC, infoPtr->font) : NULL;
    RECT rc, content_rect, push_rect, dropdown_rect;
    NMCUSTOMDRAW nmcd;
    LRESULT cdrf;
    HWND parent;

    GetClientRect(infoPtr->hwnd, &rc);
    init_custom_draw(&nmcd, infoPtr, hDC, &rc);

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);

    /* The zone outside the content is ignored for the dropdown (draws over) */
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &rc, &content_rect);
    get_split_button_rects(infoPtr, &rc, &push_rect, &dropdown_rect);

    if (infoPtr->split_style & BCSS_NOSPLIT)
    {
        push_rect = rc;
        DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &rc, NULL);
    }
    else
    {
        RECT r = { dropdown_rect.left, content_rect.top, dropdown_rect.right, content_rect.bottom };
        UINT edge = (infoPtr->split_style & BCSS_ALIGNLEFT) ? BF_RIGHT : BF_LEFT;
        const RECT *clip = NULL;

        /* If only the dropdown is pressed, we need to draw it separately */
        if (state != PBS_PRESSED && (infoPtr->state & BST_DROPDOWNPUSHED))
        {
            DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, PBS_PRESSED, &rc, &dropdown_rect);
            clip = &push_rect;
        }
        DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &rc, clip);

        /* Draw the separator */
        DrawThemeEdge(theme, hDC, BP_PUSHBUTTON, state, &r, EDGE_ETCHED, edge, NULL);

        /* The content rect should be the content area of the push button */
        GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &push_rect, &content_rect);
    }

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    if (!(cdrf & CDRF_DOERASE))
    {
        COLORREF old_color, color;
        INT old_bk_mode;
        WCHAR *text;

        if ((text = get_button_text(infoPtr)))
        {
            DrawThemeText(theme, hDC, BP_PUSHBUTTON, state, text, lstrlenW(text), dtFlags, 0, &content_rect);
            heap_free(text);
        }

        GetThemeColor(theme, BP_PUSHBUTTON, state, TMT_TEXTCOLOR, &color);
        old_bk_mode = SetBkMode(hDC, TRANSPARENT);
        old_color = SetTextColor(hDC, color);

        draw_split_button_dropdown_glyph(infoPtr, hDC, &dropdown_rect);

        SetTextColor(hDC, old_color);
        SetBkMode(hDC, old_bk_mode);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if (cdrf & CDRF_SKIPPOSTPAINT) goto cleanup;

    if (focused)
    {
        MARGINS margins;

        GetThemeMargins(theme, hDC, BP_PUSHBUTTON, state, TMT_CONTENTMARGINS, NULL, &margins);

        push_rect.left += margins.cxLeftWidth;
        push_rect.top += margins.cyTopHeight;
        push_rect.right -= margins.cxRightWidth;
        push_rect.bottom -= margins.cyBottomHeight;
        DrawFocusRect(hDC, &push_rect);
    }

cleanup:
    if (old_font) SelectObject(hDC, old_font);
}

void BUTTON_Register(void)
{
    WNDCLASSW wndClass;

    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC;
    wndClass.lpfnWndProc = BUTTON_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(BUTTON_INFO *);
    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = WC_BUTTONW;
    RegisterClassW(&wndClass);
}


#ifdef __REACTOS__
void BUTTON_Unregister(void)
{
    UnregisterClassW(WC_BUTTONW, NULL);
}
#endif
