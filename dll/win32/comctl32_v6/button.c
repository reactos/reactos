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
 *  - BCM_GETIDEALSIZE
 *  - BCM_GETIMAGELIST
 *  - BCM_GETTEXTMARGIN
 *  - BCM_SETIMAGELIST
 *  - BCM_SETTEXTMARGIN
 *
 *  Notifications
 *  - BCN_HOTITEMCHANGE
 *  - BN_DISABLE
 *  - BN_PUSHED/BN_HILITE
 *  + BN_KILLFOCUS: is it OK?
 *  - BN_PAINT
 *  + BN_SETFOCUS: is it OK?
 *  - BN_UNPUSHED/BN_UNHILITE
 *  - NM_CUSTOMDRAW
 *
 *  Structures/Macros/Definitions
 *  - BUTTON_IMAGELIST
 *  - NMBCHOTITEM
 *  - Button_GetIdealSize
 *  - Button_GetImageList
 *  - Button_GetTextMargin
 *  - Button_SetImageList
 *  - Button_SetTextMargin
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
    HWND        hwnd;
    HWND        parent;
    LONG        state;
    HFONT       font;
    WCHAR      *note;
    INT         note_length;
    union
    {
        HICON   icon;
        HBITMAP bitmap;
        HANDLE  image;
    } u;

#ifdef __REACTOS__
    DWORD ui_state;
    RECT rcTextMargin;
    BUTTON_IMAGELIST imlData;
#endif
} BUTTON_INFO;

static UINT BUTTON_CalcLabelRect( const BUTTON_INFO *infoPtr, HDC hdc, RECT *rc );
static void PB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void CB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void GB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void UB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void OB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void BUTTON_CheckAutoRadioButton( HWND hwnd );

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

/* These are indices into a states array to determine the theme state for a given theme part. */
typedef enum
{
    STATE_NORMAL,
    STATE_DISABLED,
    STATE_HOT,
    STATE_PRESSED,
    STATE_DEFAULTED
} ButtonState;

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
    PB_Paint,    /* BS_SPLITBUTTON */
    PB_Paint,    /* BS_DEFSPLITBUTTON */
    PB_Paint,    /* BS_COMMANDLINK */
    PB_Paint     /* BS_DEFCOMMANDLINK */
};


#ifdef __REACTOS__ /* r73885 */
typedef void (*pfThemedPaint)( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);

static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);
static void CB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);
static void GB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused, LPARAM prfFlag);

#else
typedef void (*pfThemedPaint)( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused);

static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused);
static void CB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused);
static void GB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, ButtonState drawState, UINT dtflags, BOOL focused);

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
    NULL,           /* BS_SPLITBUTTON */
    NULL,           /* BS_DEFSPLITBUTTON */
    NULL,           /* BS_COMMANDLINK */
    NULL,           /* BS_DEFCOMMANDLINK */
};

static inline UINT get_button_type( LONG window_style )
{
    return (window_style & BS_TYPEMASK);
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
            /* Pushbutton's text is centered by default */
            if (get_button_type(style) <= BS_DEFPUSHBUTTON) dtStyle |= DT_CENTER;
            /* all other flavours have left aligned text */
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
    else
        /* GroupBox's text is always single line and is top aligned. */
        dtStyle |= DT_SINGLELINE;

    return dtStyle;
}


#ifdef __REACTOS__
BOOL BUTTON_PaintWithTheme(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hParamDC, LPARAM prfFlag)
{
    DWORD dwStyle;
    DWORD dwStyleEx;
    DWORD type;
    UINT dtFlags;
    ButtonState drawState;
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

    if(dwStyle & WS_DISABLED)
        drawState = STATE_DISABLED;
    else if(infoPtr->state & BST_PUSHED) 
        drawState = STATE_PRESSED;
    else if ((dwStyle & BS_PUSHLIKE) && (infoPtr->state & (BST_CHECKED|BST_INDETERMINATE))) 
        drawState = STATE_PRESSED;
    else if(infoPtr->state & BST_HOT) 
        drawState = STATE_HOT;
    else if((infoPtr->state & BST_FOCUS) || (dwStyle & BS_DEFPUSHBUTTON))
        drawState = STATE_DEFAULTED;
    else
        drawState = STATE_NORMAL;

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

    TextSize.cy += infoPtr->rcTextMargin.top + infoPtr->rcTextMargin.bottom;
    TextSize.cx += infoPtr->rcTextMargin.left + infoPtr->rcTextMargin.right;

    if (infoPtr->imlData.himl && ImageList_GetIconSize(infoPtr->imlData.himl, &ImageSize.cx, &ImageSize.cy))
    {
        ImageSize.cx += infoPtr->imlData.margin.left + infoPtr->imlData.margin.right;
        ImageSize.cy += infoPtr->imlData.margin.top + infoPtr->imlData.margin.bottom;
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
        infoPtr = heap_alloc_zero( sizeof(*infoPtr) );
        SetWindowLongPtrW( hWnd, 0, (LONG_PTR)infoPtr );
        infoPtr->hwnd = hWnd;
#ifdef __REACTOS__
        SetRect(&infoPtr->rcTextMargin, 1,1,1,1);
#endif
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);

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
            ButtonState drawState;
            UINT dtflags;

            if (IsWindowEnabled( hWnd ))
            {
                if (infoPtr->state & BST_PUSHED) drawState = STATE_PRESSED;
                else if (infoPtr->state & BST_HOT) drawState = STATE_HOT;
                else if (infoPtr->state & BST_FOCUS) drawState = STATE_DEFAULTED;
                else drawState = STATE_NORMAL;
            }
            else
                drawState = STATE_DISABLED;

            dtflags = BUTTON_BStoDT(style, GetWindowLongW(hWnd, GWL_EXSTYLE));
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
        SetCapture( hWnd );
        SetFocus( hWnd );
        infoPtr->state |= BUTTON_BTNPRESSED;
        SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
        break;

    case WM_KEYUP:
	if (wParam != VK_SPACE)
	    break;
	/* fall through */
    case WM_LBUTTONUP:
        state = infoPtr->state;
        if (!(state & BUTTON_BTNPRESSED)) break;
        infoPtr->state &= BUTTON_NSTATES;
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

#ifdef __REACTOS__
    case BCM_GETTEXTMARGIN:
    {
        RECT* prc = (RECT*)lParam;
        if (!prc)
            return FALSE;
        *prc = infoPtr->rcTextMargin;
        return TRUE;
    }
    case BCM_SETTEXTMARGIN:
    {
        RECT* prc = (RECT*)lParam;
        if (!prc)
            return FALSE;
        infoPtr->rcTextMargin = *prc;
        return TRUE;
    }
    case BCM_SETIMAGELIST:
    {
        BUTTON_IMAGELIST * pimldata = (BUTTON_IMAGELIST *)lParam;
        if (!pimldata || !pimldata->himl)
            return FALSE;
        infoPtr->imlData = *pimldata;
        return TRUE;
    }
    case BCM_GETIMAGELIST:
    {
        BUTTON_IMAGELIST * pimldata = (BUTTON_IMAGELIST *)lParam;
        if (!pimldata)
            return FALSE;
        *pimldata = infoPtr->imlData;
        return TRUE;
    }
    case BCM_GETIDEALSIZE:
    {
        HTHEME theme = GetWindowTheme(hWnd);
        BOOL ret = FALSE;
        SIZE* pSize = (SIZE*)lParam;

        if (!pSize)
        {
            return FALSE;
        }

        if (btn_type == BS_PUSHBUTTON || 
            btn_type == BS_DEFPUSHBUTTON ||
            btn_type == BS_USERBUTTON)
        {
            ret = BUTTON_GetIdealSize(infoPtr, theme, pSize);
        }

        if (!ret)
        {
            GetClientRect(hWnd, &rect);
            pSize->cx = rect.right;
            pSize->cy = rect.bottom;
        }

        return TRUE;
    }
#endif

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
            BUTTON_CalcLabelRect(infoPtr, hdc, &rc);
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
#ifndef __REACTOS__
        paint_button( infoPtr, btn_type, ODA_FOCUS );
#endif

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
        /* Check that image format matches button style */
        switch (style & (BS_BITMAP|BS_ICON))
        {
        case BS_BITMAP:
            if (wParam != IMAGE_BITMAP) return 0;
            break;
        case BS_ICON:
            if (wParam != IMAGE_ICON) return 0;
            break;
        default:
            return 0;
        }
        oldHbitmap = infoPtr->u.image;
        infoPtr->u.image = (HANDLE)lParam;
	InvalidateRect( hWnd, NULL, FALSE );
	return (LRESULT)oldHbitmap;

    case BM_GETIMAGE:
        return (LRESULT)infoPtr->u.image;

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

    case WM_NCHITTEST:
        if(btn_type == BS_GROUPBOX) return HTTRANSPARENT;
        /* fall through */
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/**********************************************************************
 *       BUTTON_CalcLabelRect
 *
 *   Calculates label's rectangle depending on button style.
 *
 * Returns flags to be passed to DrawText.
 * Calculated rectangle doesn't take into account button state
 * (pushed, etc.). If there is nothing to draw (no text/image) output
 * rectangle is empty, and return value is (UINT)-1.
 */
static UINT BUTTON_CalcLabelRect(const BUTTON_INFO *infoPtr, HDC hdc, RECT *rc)
{
   LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
   LONG ex_style = GetWindowLongW( infoPtr->hwnd, GWL_EXSTYLE );
   WCHAR *text;
   ICONINFO    iconInfo;
   BITMAP      bm;
   UINT        dtStyle = BUTTON_BStoDT( style, ex_style );
   RECT        r = *rc;
   INT         n;
#ifdef __REACTOS__
    BOOL bHasIml = BUTTON_DrawIml(hdc, &infoPtr->imlData, &r, TRUE, 0);
#endif

   /* Calculate label rectangle according to label type */
   switch (style & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
      {
          HFONT hFont, hPrevFont = 0;

          if (!(text = get_button_text( infoPtr ))) goto empty_rect;
          if (!text[0])
          {
              heap_free( text );
              goto empty_rect;
          }

          if ((hFont = infoPtr->font)) hPrevFont = SelectObject( hdc, hFont );
#ifdef __REACTOS__
          DrawTextW(hdc, text, -1, &r, ((dtStyle | DT_CALCRECT) & ~(DT_VCENTER | DT_BOTTOM)));
#else
          DrawTextW(hdc, text, -1, &r, dtStyle | DT_CALCRECT);
#endif
          if (hPrevFont) SelectObject( hdc, hPrevFont );
          heap_free( text );
#ifdef __REACTOS__
          if (infoPtr->ui_state & UISF_HIDEACCEL)
              dtStyle |= DT_HIDEPREFIX;
#endif
          break;
      }

      case BS_ICON:
         if (!GetIconInfo(infoPtr->u.icon, &iconInfo))
            goto empty_rect;

         GetObjectW (iconInfo.hbmColor, sizeof(BITMAP), &bm);

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;

         DeleteObject(iconInfo.hbmColor);
         DeleteObject(iconInfo.hbmMask);
         break;

      case BS_BITMAP:
         if (!GetObjectW( infoPtr->u.bitmap, sizeof(BITMAP), &bm))
            goto empty_rect;

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;
         break;

      default:
      empty_rect:
#ifdef __REACTOS__
         if (bHasIml)
             break;
#endif
         rc->right = r.left;
         rc->bottom = r.top;
         return (UINT)-1;
   }

#ifdef __REACTOS__
   if (bHasIml)
   {
     if (infoPtr->imlData.uAlign == BUTTON_IMAGELIST_ALIGN_LEFT)
         r.left = infoPtr->imlData.margin.left;
     else if (infoPtr->imlData.uAlign == BUTTON_IMAGELIST_ALIGN_RIGHT)
         r.right = infoPtr->imlData.margin.right;
     else if (infoPtr->imlData.uAlign == BUTTON_IMAGELIST_ALIGN_TOP)
         r.top = infoPtr->imlData.margin.top;
     else if (infoPtr->imlData.uAlign == BUTTON_IMAGELIST_ALIGN_BOTTOM)
         r.bottom = infoPtr->imlData.margin.bottom;
   }
#endif

   /* Position label inside bounding rectangle according to
    * alignment flags. (calculated rect is always left-top aligned).
    * If label is aligned to any side - shift label in opposite
    * direction to leave extra space for focus rectangle.
    */
   switch (dtStyle & (DT_CENTER|DT_RIGHT))
   {
      case DT_LEFT:    r.left++;  r.right++;  break;
      case DT_CENTER:  n = r.right - r.left;
                       r.left   = rc->left + ((rc->right - rc->left) - n) / 2;
                       r.right  = r.left + n; break;
      case DT_RIGHT:   n = r.right - r.left;
                       r.right  = rc->right - 1;
                       r.left   = r.right - n;
                       break;
   }

   switch (dtStyle & (DT_VCENTER|DT_BOTTOM))
   {
      case DT_TOP:     r.top++;  r.bottom++;  break;
      case DT_VCENTER: n = r.bottom - r.top;
#ifdef __REACTOS__
                       r.top    = rc->top + ((rc->bottom - 1 - rc->top) - n) / 2;
#else
                       r.top    = rc->top + ((rc->bottom - rc->top) - n) / 2;
#endif
                       r.bottom = r.top + n;  break;
      case DT_BOTTOM:  n = r.bottom - r.top;
                       r.bottom = rc->bottom - 1;
                       r.top    = r.bottom - n;
                       break;
   }

   *rc = r;
   return dtStyle;
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
 */
static void BUTTON_DrawLabel(const BUTTON_INFO *infoPtr, HDC hdc, UINT dtFlags, const RECT *rc)
{
   DRAWSTATEPROC lpOutputProc = NULL;
   LPARAM lp;
   WPARAM wp = 0;
   HBRUSH hbr = 0;
   UINT flags = IsWindowEnabled(infoPtr->hwnd) ? DSS_NORMAL : DSS_DISABLED;
   LONG state = infoPtr->state;
   LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
   WCHAR *text = NULL;

   /* FIXME: To draw disabled label in Win31 look-and-feel, we probably
    * must use DSS_MONO flag and COLOR_GRAYTEXT brush (or maybe DSS_UNION).
    * I don't have Win31 on hand to verify that, so I leave it as is.
    */

#ifdef __REACTOS__
    RECT rcText = *rc;
    BUTTON_DrawIml(hdc, &infoPtr->imlData, &rcText, FALSE, 0);
#endif

   if ((style & BS_PUSHLIKE) && (state & BST_INDETERMINATE))
   {
      hbr = GetSysColorBrush(COLOR_GRAYTEXT);
      flags |= DSS_MONO;
   }

   switch (style & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
         /* DST_COMPLEX -- is 0 */
         lpOutputProc = BUTTON_DrawTextCallback;
         if (!(text = get_button_text( infoPtr ))) return;
         lp = (LPARAM)text;
         wp = dtFlags;
#ifdef __REACTOS__
         if (dtFlags & DT_HIDEPREFIX)
             flags |= DSS_HIDEPREFIX;
#endif
         break;

      case BS_ICON:
         flags |= DST_ICON;
         lp = (LPARAM)infoPtr->u.icon;
         break;

      case BS_BITMAP:
         flags |= DST_BITMAP;
         lp = (LPARAM)infoPtr->u.bitmap;
         break;

      default:
         return;
   }

#ifdef __REACTOS__
   DrawStateW(hdc, hbr, lpOutputProc, lp, wp, rcText.left, rcText.top,
              rcText.right - rcText.left, rcText.bottom - rcText.top, flags);
#else
   DrawStateW(hdc, hbr, lpOutputProc, lp, wp, rc->left, rc->top,
              rc->right - rc->left, rc->bottom - rc->top, flags);
#endif
   heap_free( text );
}

/**********************************************************************
 *       Push Button Functions
 */
static void PB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT     rc, r;
    UINT     dtFlags, uState;
    HPEN     hOldPen, hpen;
    HBRUSH   hOldBrush;
    INT      oldBkMode;
    COLORREF oldTxtColor;
    HFONT hFont;
    LONG state = infoPtr->state;
    LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
    BOOL pushedState = (state & BST_PUSHED);
    HWND parent;
    HRGN hrgn;
#ifdef __REACTOS__
    DWORD cdrf;
#endif

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

#ifdef __REACTOS__
    cdrf = BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_PREERASE, &rc);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;
#endif

    if (get_button_type(style) == BS_DEFPUSHBUTTON)
    {
        if (action != ODA_FOCUS)
            Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
	InflateRect( &rc, -1, -1 );
    }

    /* completely skip the drawing if only focus has changed */
    if (action == ODA_FOCUS) goto draw_focus;

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

#ifdef __REACTOS__
    if (cdrf == CDRF_NOTIFYPOSTERASE)
        BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_POSTERASE, &rc);

    cdrf = BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_PREPAINT, &rc);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;
#endif

    /* draw button label */
    r = rc;
    dtFlags = BUTTON_CalcLabelRect(infoPtr, hDC, &r);

    if (dtFlags == (UINT)-1L)
       goto cleanup;

    if (pushedState)
       OffsetRect(&r, 1, 1);

    oldTxtColor = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );

    BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &r);

    SetTextColor( hDC, oldTxtColor );

#ifdef __REACTOS__
    if (cdrf == CDRF_NOTIFYPOSTPAINT)
        BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_POSTPAINT, &rc);
#endif

draw_focus:
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
    RECT rbox, rtext, client;
    HBRUSH hBrush;
    int delta, text_offset, checkBoxWidth, checkBoxHeight;
    UINT dtFlags;
    HFONT hFont;
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
    rbox = rtext = client;

    checkBoxWidth  = 12 * GetDeviceCaps( hDC, LOGPIXELSX ) / 96 + 1;
    checkBoxHeight = 12 * GetDeviceCaps( hDC, LOGPIXELSY ) / 96 + 1;

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
        rtext.right -= checkBoxWidth + text_offset;
        rbox.left = rbox.right - checkBoxWidth;
    }
    else
    {
        rtext.left += checkBoxWidth + text_offset;
        rbox.right = checkBoxWidth;
    }

    /* Since WM_ERASEBKGND does nothing, first prepare background */
    if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
    if (action == ODA_DRAWENTIRE) FillRect( hDC, &client, hBrush );

    /* Draw label */
    client = rtext;
    dtFlags = BUTTON_CalcLabelRect(infoPtr, hDC, &rtext);

    /* Only adjust rbox when rtext is valid */
    if (dtFlags != (UINT)-1L)
    {
	rbox.top = rtext.top;
	rbox.bottom = rtext.bottom;
    }

    /* Draw the check-box bitmap */
    if (action == ODA_DRAWENTIRE || action == ODA_SELECT)
    {
	UINT flags;

	if ((get_button_type(style) == BS_RADIOBUTTON) ||
	    (get_button_type(style) == BS_AUTORADIOBUTTON)) flags = DFCS_BUTTONRADIO;
	else if (state & BST_INDETERMINATE) flags = DFCS_BUTTON3STATE;
	else flags = DFCS_BUTTONCHECK;

	if (state & (BST_CHECKED | BST_INDETERMINATE)) flags |= DFCS_CHECKED;
	if (state & BST_PUSHED) flags |= DFCS_PUSHED;

	if (style & WS_DISABLED) flags |= DFCS_INACTIVE;

	/* rbox must have the correct height */
	delta = rbox.bottom - rbox.top - checkBoxHeight;

	if (style & BS_TOP) {
	    if (delta > 0) {
		rbox.bottom = rbox.top + checkBoxHeight;
	    } else {
		rbox.top -= -delta/2 + 1;
		rbox.bottom = rbox.top + checkBoxHeight;
	    }
	} else if (style & BS_BOTTOM) {
	    if (delta > 0) {
		rbox.top = rbox.bottom - checkBoxHeight;
	    } else {
		rbox.bottom += -delta/2 + 1;
		rbox.top = rbox.bottom - checkBoxHeight;
	    }
	} else { /* Default */
	    if (delta > 0) {
		int ofs = (delta / 2);
		rbox.bottom -= ofs + 1;
		rbox.top = rbox.bottom - checkBoxHeight;
	    } else if (delta < 0) {
		int ofs = (-delta / 2);
		rbox.top -= ofs + 1;
		rbox.bottom = rbox.top + checkBoxHeight;
	    }
	}

	DrawFrameControl( hDC, &rbox, DFC_BUTTON, flags );
    }

    if (dtFlags == (UINT)-1L) /* Noting to draw */
	return;

    if (action == ODA_DRAWENTIRE)
        BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &rtext);

    /* ... and focus */
    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
	rtext.left--;
	rtext.right++;
	IntersectRect(&rtext, &rtext, &client);
	DrawFocusRect( hDC, &rtext );
    }
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
    RECT rc, rcFrame;
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
    GetClientRect( infoPtr->hwnd, &rc);
    rcFrame = rc;
    hrgn = set_control_clipping( hDC, &rc );

    GetTextMetricsW (hDC, &tm);
    rcFrame.top += (tm.tmHeight / 2) - 1;
    DrawEdge (hDC, &rcFrame, EDGE_ETCHED, BF_RECT | ((style & BS_FLAT) ? BF_FLAT : 0));

    InflateRect(&rc, -7, 1);
    dtFlags = BUTTON_CalcLabelRect(infoPtr, hDC, &rc);

    if (dtFlags != (UINT)-1)
    {
        /* Because buttons have CS_PARENTDC class style, there is a chance
         * that label will be drawn out of client rect.
         * But Windows doesn't clip label's rect, so do I.
         */

        /* There is 1-pixel margin at the left, right, and bottom */
        rc.left--; rc.right++; rc.bottom++;
        FillRect(hDC, &rc, hbr);
        rc.left++; rc.right--; rc.bottom--;

        BUTTON_DrawLabel(infoPtr, hDC, dtFlags, &rc);
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
    HFONT hFont;
    LONG state = infoPtr->state;
    HWND parent;

    GetClientRect( infoPtr->hwnd, &rc);

    if ((hFont = infoPtr->font)) SelectObject( hDC, hFont );

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);

    FillRect( hDC, &rc, hBrush );
    if (action == ODA_FOCUS || (state & BST_FOCUS))
        DrawFocusRect( hDC, &rc );

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

#ifdef __REACTOS__ /* r73885 */
static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void PB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused)
#endif
{
    static const int states[] = { PBS_NORMAL, PBS_DISABLED, PBS_HOT, PBS_PRESSED, PBS_DEFAULTED };

    RECT bgRect, textRect;
    HFONT font = infoPtr->font;
    HFONT hPrevFont = font ? SelectObject(hDC, font) : NULL;
    int state = states[ drawState ];
    WCHAR *text = get_button_text(infoPtr);
#ifdef __REACTOS__
    HWND parent;
    DWORD cdrf;

    GetClientRect(infoPtr->hwnd, &bgRect);
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &bgRect, &textRect);

    if (prfFlag == 0)
    {
        if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
            DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    }

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    SendMessageW( parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd );

    cdrf = BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_PREERASE, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

    DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &bgRect, NULL);

    if (cdrf == CDRF_NOTIFYPOSTERASE)
        BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_POSTERASE, &bgRect);

    cdrf = BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_PREPAINT, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

    BUTTON_DrawIml(hDC, &infoPtr->imlData, &textRect, FALSE, drawState);
#else
    GetClientRect(infoPtr->hwnd, &bgRect);
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &bgRect, &textRect);

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);

    DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &bgRect, NULL);
#endif

    if (text)
    {
        DrawThemeText(theme, hDC, BP_PUSHBUTTON, state, text, lstrlenW(text), dtFlags, 0, &textRect);
        heap_free(text);
#ifdef __REACTOS__
        text = NULL;
#endif
    }

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

#ifdef __REACTOS__
    if (cdrf == CDRF_NOTIFYPOSTPAINT)
        BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_POSTPAINT, &bgRect);
cleanup:
    if (text) heap_free(text);
#endif
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

#ifdef __REACTOS__ /* r73885 */
static void CB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void CB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused)
#endif
{
    static const int cb_states[3][5] =
    {
        { CBS_UNCHECKEDNORMAL, CBS_UNCHECKEDDISABLED, CBS_UNCHECKEDHOT, CBS_UNCHECKEDPRESSED, CBS_UNCHECKEDNORMAL },
        { CBS_CHECKEDNORMAL, CBS_CHECKEDDISABLED, CBS_CHECKEDHOT, CBS_CHECKEDPRESSED, CBS_CHECKEDNORMAL },
        { CBS_MIXEDNORMAL, CBS_MIXEDDISABLED, CBS_MIXEDHOT, CBS_MIXEDPRESSED, CBS_MIXEDNORMAL }
    };

    static const int rb_states[2][5] =
    {
        { RBS_UNCHECKEDNORMAL, RBS_UNCHECKEDDISABLED, RBS_UNCHECKEDHOT, RBS_UNCHECKEDPRESSED, RBS_UNCHECKEDNORMAL },
        { RBS_CHECKEDNORMAL, RBS_CHECKEDDISABLED, RBS_CHECKEDHOT, RBS_CHECKEDPRESSED, RBS_CHECKEDNORMAL }
    };

    SIZE sz;
    RECT bgRect, textRect;
    HFONT font, hPrevFont = NULL;
    int checkState = infoPtr->state & 3;
    DWORD dwStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    UINT btn_type = get_button_type( dwStyle );
    int part = (btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON) ? BP_RADIOBUTTON : BP_CHECKBOX;
    int state = (part == BP_CHECKBOX)
              ? cb_states[ checkState ][ drawState ]
              : rb_states[ checkState ][ drawState ];
    WCHAR *text = get_button_text(infoPtr);
    LOGFONTW lf;
    BOOL created_font = FALSE;
#ifdef __REACTOS__
    HWND parent;
    HBRUSH hBrush;
    DWORD cdrf;
#endif

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
#ifdef __REACTOS__ /* r73885 */
        font = infoPtr->font;
#else
        font = (HFONT)SendMessageW(infoPtr->hwnd, WM_GETFONT, 0, 0);
#endif
        hPrevFont = SelectObject(hDC, font);
    }

    if (FAILED(GetThemePartSize(theme, hDC, part, state, NULL, TS_DRAW, &sz)))
        sz.cx = sz.cy = 13;

    GetClientRect(infoPtr->hwnd, &bgRect);

#ifdef __REACTOS__
    if (prfFlag == 0)
    {
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    }

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC,
                                 (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC,
                                        (WPARAM)hDC, (LPARAM)infoPtr->hwnd );
    FillRect( hDC, &bgRect, hBrush );

    cdrf = BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_PREERASE, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;
#endif

    GetThemeBackgroundContentRect(theme, hDC, part, state, &bgRect, &textRect);

    if (dtFlags & DT_SINGLELINE) /* Center the checkbox / radio button to the text. */
        bgRect.top = bgRect.top + (textRect.bottom - textRect.top - sz.cy) / 2;

    /* adjust for the check/radio marker */
    bgRect.bottom = bgRect.top + sz.cy;
    bgRect.right = bgRect.left + sz.cx;
    textRect.left = bgRect.right + 6;

#ifdef __REACTOS__
    DrawThemeBackground(theme, hDC, part, state, &bgRect, NULL);

    if (cdrf == CDRF_NOTIFYPOSTERASE)
        BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_POSTERASE, &bgRect);

    cdrf = BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_PREPAINT, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

#else
    DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    DrawThemeBackground(theme, hDC, part, state, &bgRect, NULL);
#endif
    if (text)
    {
        DrawThemeText(theme, hDC, part, state, text, lstrlenW(text), dtFlags, 0, &textRect);

        if (focused)
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

#ifdef __REACTOS__
    if (cdrf == CDRF_NOTIFYPOSTPAINT)
        BUTTON_SendCustomDraw(infoPtr, hDC, CDDS_POSTPAINT, &bgRect);
cleanup:
    if (text) heap_free(text);
#endif
    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

#ifdef __REACTOS__ /* r73885 */
static void GB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void GB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused)
#endif
{
    static const int states[] = { GBS_NORMAL, GBS_DISABLED, GBS_NORMAL, GBS_NORMAL, GBS_NORMAL };

    RECT bgRect, textRect, contentRect;
    int state = states[ drawState ];
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
