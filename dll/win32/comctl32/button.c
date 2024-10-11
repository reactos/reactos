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

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(button);

/* undocumented flags */
#define BUTTON_NSTATES         0x0F
#define BUTTON_BTNPRESSED      0x40
#define BUTTON_UNKNOWN2        0x20
#define BUTTON_UNKNOWN3        0x10

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
    HANDLE           image; /* Original handle set with BM_SETIMAGE and returned with BM_GETIMAGE. */
    union
    {
        HICON   icon;
        HBITMAP bitmap;
        HANDLE  image; /* Duplicated handle used for drawing. */
    } u;
} BUTTON_INFO;

static UINT BUTTON_CalcLayoutRects( const BUTTON_INFO *infoPtr, HDC hdc, RECT *labelRc, RECT *imageRc, RECT *textRc );
static void PB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void CB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void GB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void UB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void OB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void SB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
static void CL_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action );
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
    CL_Paint,    /* BS_COMMANDLINK */
    CL_Paint     /* BS_DEFCOMMANDLINK */
};

typedef void (*pfThemedPaint)( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);

static void PB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void CB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void GB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void SB_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);
static void CL_ThemedPaint( HTHEME theme, const BUTTON_INFO *infoPtr, HDC hdc, int drawState, UINT dtflags, BOOL focused);

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
    CL_ThemedPaint, /* BS_COMMANDLINK */
    CL_ThemedPaint  /* BS_DEFCOMMANDLINK */
};

typedef BOOL (*pfGetIdealSize)(BUTTON_INFO *infoPtr, SIZE *size);

static BOOL PB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL CB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL GB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL SB_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);
static BOOL CL_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size);

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
    CL_GetIdealSize, /* BS_COMMANDLINK */
    CL_GetIdealSize  /* BS_DEFCOMMANDLINK */
};

/* Fixed margin for command links, regardless of DPI (based on tests done on Windows) */
enum { command_link_margin = 6 };

/* The width and height for the default command link glyph (when there's no image) */
enum { command_link_defglyph_size = 17 };

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

/* retrieve the button text; returned buffer must be freed by caller */
static inline WCHAR *get_button_text( const BUTTON_INFO *infoPtr )
{
    INT len = GetWindowTextLengthW( infoPtr->hwnd );
    WCHAR *buffer = Alloc( (len + 1) * sizeof(WCHAR) );
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

static BOOL is_themed_paint_supported(HTHEME theme, UINT btn_type)
{
    if (!theme || !btnThemedPaintFunc[btn_type])
        return FALSE;

    if (btn_type == BS_COMMANDLINK || btn_type == BS_DEFCOMMANDLINK)
    {
        if (!IsThemePartDefined(theme, BP_COMMANDLINK, 0))
            return FALSE;
    }

    return TRUE;
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
    WCHAR *dst = Alloc(size);
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

static int get_draw_state(const BUTTON_INFO *infoPtr)
{
    static const int pb_states[DRAW_STATE_COUNT] = { PBS_NORMAL, PBS_DISABLED, PBS_HOT, PBS_PRESSED, PBS_DEFAULTED };
    static const int cb_states[3][DRAW_STATE_COUNT] =
    {
        { CBS_UNCHECKEDNORMAL, CBS_UNCHECKEDDISABLED, CBS_UNCHECKEDHOT, CBS_UNCHECKEDPRESSED, CBS_UNCHECKEDNORMAL },
        { CBS_CHECKEDNORMAL, CBS_CHECKEDDISABLED, CBS_CHECKEDHOT, CBS_CHECKEDPRESSED, CBS_CHECKEDNORMAL },
        { CBS_MIXEDNORMAL, CBS_MIXEDDISABLED, CBS_MIXEDHOT, CBS_MIXEDPRESSED, CBS_MIXEDNORMAL }
    };
    static const int pushlike_cb_states[3][DRAW_STATE_COUNT] =
    {
        { PBS_NORMAL, PBS_DISABLED, PBS_HOT, PBS_PRESSED, PBS_NORMAL },
        { PBS_PRESSED, PBS_PRESSED, PBS_PRESSED, PBS_PRESSED, PBS_PRESSED },
        { PBS_NORMAL, PBS_DISABLED, PBS_HOT, PBS_PRESSED, PBS_NORMAL }
    };
    static const int rb_states[2][DRAW_STATE_COUNT] =
    {
        { RBS_UNCHECKEDNORMAL, RBS_UNCHECKEDDISABLED, RBS_UNCHECKEDHOT, RBS_UNCHECKEDPRESSED, RBS_UNCHECKEDNORMAL },
        { RBS_CHECKEDNORMAL, RBS_CHECKEDDISABLED, RBS_CHECKEDHOT, RBS_CHECKEDPRESSED, RBS_CHECKEDNORMAL }
    };
    static const int pushlike_rb_states[2][DRAW_STATE_COUNT] =
    {
        { PBS_NORMAL, PBS_DISABLED, PBS_HOT, PBS_PRESSED, PBS_NORMAL },
        { PBS_PRESSED, PBS_PRESSED, PBS_PRESSED, PBS_PRESSED, PBS_PRESSED }
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
    else if (infoPtr->state & BST_FOCUS || type == BS_DEFPUSHBUTTON || type == BS_DEFSPLITBUTTON
             || (type == BS_DEFCOMMANDLINK && !(style & BS_PUSHLIKE)))
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
    case BS_3STATE:
    case BS_AUTO3STATE:
        return style & BS_PUSHLIKE ? pushlike_cb_states[check_state][state]
                                   : cb_states[check_state][state];
    case BS_RADIOBUTTON:
    case BS_AUTORADIOBUTTON:
        return style & BS_PUSHLIKE ? pushlike_rb_states[check_state][state]
                                   : rb_states[check_state][state];
    case BS_GROUPBOX:
        return style & BS_PUSHLIKE ? pb_states[state] : gb_states[state];
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
        theme = GetWindowTheme( hWnd );
        if (theme)
            RedrawWindow( hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW );
        else
            paint_button( infoPtr, btn_type, ODA_DRAWENTIRE );
        break;

    case WM_NCCREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;

        infoPtr = Alloc( sizeof(*infoPtr) );
        SetWindowLongPtrW( hWnd, 0, (LONG_PTR)infoPtr );
        infoPtr->hwnd = hWnd;
        infoPtr->parent = cs->hwndParent;
        infoPtr->style = cs->style;
        infoPtr->split_style = BCSS_STRETCH;
        infoPtr->glyph = (HIMAGELIST)0x36;  /* Marlett down arrow char code */
        infoPtr->glyph_size.cx = get_default_glyph_size(infoPtr);
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    case WM_NCDESTROY:
        SetWindowLongPtrW( hWnd, 0, 0 );
        if (infoPtr->image_type == IMAGE_BITMAP)
            DeleteObject(infoPtr->u.bitmap);
        else if (infoPtr->image_type == IMAGE_ICON)
            DestroyIcon(infoPtr->u.icon);
        Free(infoPtr->note);
        Free(infoPtr);
        break;

    case WM_CREATE:
    {
        HWND parent;

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

        parent = GetParent( hWnd );
        if (parent)
            EnableThemeDialogTexture( parent, ETDT_ENABLE );
        return 0;
    }

    case WM_DESTROY:
        theme = GetWindowTheme( hWnd );
        CloseThemeData( theme );
        break;

    case WM_THEMECHANGED:
        theme = GetWindowTheme( hWnd );
        CloseThemeData( theme );
        OpenThemeData( hWnd, WC_BUTTONW );
        InvalidateRect( hWnd, NULL, TRUE );
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

        if (is_themed_paint_supported(theme, btn_type))
        {
            int drawState = get_draw_state(infoPtr);
            UINT dtflags = BUTTON_BStoDT(style, GetWindowLongW(hWnd, GWL_EXSTYLE));

            btnThemedPaintFunc[btn_type](theme, infoPtr, hdc, drawState, dtflags, infoPtr->state & BST_FOCUS);
        }
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
                SendMessageW( hWnd, BM_SETCHECK, TRUE, 0 );
                break;
            case BS_AUTO3STATE:
                SendMessageW( hWnd, BM_SETCHECK, (infoPtr->state & BST_INDETERMINATE) ? 0 :
                    ((infoPtr->state & 3) + 1), 0 );
                break;
            }
            ReleaseCapture();
            BUTTON_NOTIFY_PARENT(hWnd, BN_CLICKED);
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
        if (!TrackMouseEvent(&mouse_event) || !(mouse_event.dwFlags & (TME_HOVER | TME_LEAVE)))
        {
            mouse_event.dwFlags = TME_HOVER | TME_LEAVE;
            mouse_event.hwndTrack = hWnd;
            mouse_event.dwHoverTime = 1;
            TrackMouseEvent(&mouse_event);
        }

        if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
        {
            GetClientRect( hWnd, &rect );
            SendMessageW( hWnd, BM_SETSTATE, PtInRect(&rect, pt), 0 );
        }
        break;
    }

    case WM_MOUSEHOVER:
    {
        infoPtr->state |= BST_HOT;
        InvalidateRect( hWnd, NULL, FALSE );
        break;
    }

    case WM_MOUSELEAVE:
    {
        infoPtr->state &= ~BST_HOT;
        InvalidateRect( hWnd, NULL, FALSE );
        break;
    }

    case WM_SETTEXT:
    {
        /* Clear an old text here as Windows does */
        if (IsWindowVisible(hWnd))
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
        else if (GetWindowTheme( hWnd ))
            RedrawWindow( hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
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

        Free(infoPtr->note);
        if (note)
        {
            infoPtr->note_length = lstrlenW(note);
            infoPtr->note = heap_strndupW(note, infoPtr->note_length);
        }

        if (!note || !infoPtr->note)
        {
            infoPtr->note_length = 0;
            infoPtr->note = Alloc(sizeof(WCHAR));
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

        if (btn_type == BS_OWNERDRAW)
            paint_button( infoPtr, btn_type, ODA_FOCUS );
        else
            InvalidateRect(hWnd, NULL, FALSE);

        if (style & BS_NOTIFY)
            BUTTON_NOTIFY_PARENT(hWnd, BN_SETFOCUS);
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
    {
        DWORD new_btn_type;

        new_btn_type= wParam & BS_TYPEMASK;
        if (btn_type >= BS_SPLITBUTTON && new_btn_type <= BS_DEFPUSHBUTTON)
            new_btn_type = (btn_type & ~BS_DEFPUSHBUTTON) | new_btn_type;

        style = (style & ~BS_TYPEMASK) | new_btn_type;
        SetWindowLongW( hWnd, GWL_STYLE, style );

        /* Only redraw if lParam flag is set.*/
        if (lParam)
            InvalidateRect( hWnd, NULL, TRUE );

        break;
    }
    case BM_CLICK:
	SendMessageW( hWnd, WM_LBUTTONDOWN, 0, 0 );
	SendMessageW( hWnd, WM_LBUTTONUP, 0, 0 );
	break;

    case BM_SETIMAGE:
        infoPtr->image_type = (DWORD)wParam;
        oldHbitmap = infoPtr->image;
        infoPtr->u.image = CopyImage((HANDLE)lParam, infoPtr->image_type, 0, 0, 0);
        infoPtr->image = (HANDLE)lParam;
        InvalidateRect( hWnd, NULL, FALSE );
        return (LRESULT)oldHbitmap;

    case BM_GETIMAGE:
        return (LRESULT)infoPtr->image;

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
        if ((btn_type == BS_AUTORADIOBUTTON) && (wParam == BST_CHECKED) && (style & WS_CHILD))
            BUTTON_CheckAutoRadioButton( hWnd );
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

    if ((style & BS_PUSHLIKE) && !(style & BS_CENTER)) style |= BS_CENTER;

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
        /* The left and right margins are added to the inner rectangle to get a new rectangle. Then
         * the new rectangle is adjusted to be in the horizontal center */
        innerRect->left = outerRect->left + (outerRect->right - outerRect->left - width
                                             + margin->left - margin->right) / 2;
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
        /* The top and bottom margins are added to the inner rectangle to get a new rectangle. Then
         * the new rectangle is adjusted to be in the vertical center */
        innerRect->top = outerRect->top + (outerRect->bottom - outerRect->top - height
                                           + margin->top - margin->bottom) / 2;
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
    {
        int scx, scy;
        ImageList_GetIconSize(infoPtr->imagelist.himl, &scx, &scy);
        size.cx = scx;
        size.cy = scy;
    }
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
    Free(text);

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

static BOOL CL_GetIdealSize(BUTTON_INFO *infoPtr, SIZE *size)
{
    HTHEME theme = GetWindowTheme(infoPtr->hwnd);
    HDC hdc = GetDC(infoPtr->hwnd);
    LONG w, text_w = 0, text_h = 0;
    UINT flags = DT_TOP | DT_LEFT;
    HFONT font, old_font = NULL;
    RECT text_bound = { 0 };
    SIZE img_size;
    RECT margin;
    WCHAR *text;

    /* Get the image size */
    if (infoPtr->u.image || infoPtr->imagelist.himl)
        img_size = BUTTON_GetImageSize(infoPtr);
    else
    {
        if (theme)
            GetThemePartSize(theme, NULL, BP_COMMANDLINKGLYPH, CMDLS_NORMAL, NULL, TS_DRAW, &img_size);
        else
            img_size.cx = img_size.cy = command_link_defglyph_size;
    }

    /* Get the content margins */
    if (theme)
    {
        RECT r = { 0, 0, 0xffff, 0xffff };
        GetThemeBackgroundContentRect(theme, hdc, BP_COMMANDLINK, CMDLS_NORMAL, &r, &margin);
        margin.left  -= r.left;
        margin.top   -= r.top;
        margin.right  = r.right  - margin.right;
        margin.bottom = r.bottom - margin.bottom;
    }
    else
    {
        margin.left = margin.right = command_link_margin;
        margin.top = margin.bottom = command_link_margin;
    }

    /* Account for the border margins and the margin between image and text */
    w = margin.left + margin.right + (img_size.cx ? (img_size.cx + command_link_margin) : 0);

    /* If a rectangle with a specific width was requested, bound the text to it */
    if (size->cx > w)
    {
        text_bound.right = size->cx - w;
        flags |= DT_WORDBREAK;
    }

    if (theme)
    {
        if (infoPtr->font) old_font = SelectObject(hdc, infoPtr->font);

        /* Find the text's rect */
        if ((text = get_button_text(infoPtr)))
        {
            RECT r;
            GetThemeTextExtent(theme, hdc, BP_COMMANDLINK, CMDLS_NORMAL,
                               text, -1, flags, &text_bound, &r);
            Free(text);
            text_w = r.right - r.left;
            text_h = r.bottom - r.top;
        }

        /* Find the note's rect */
        if (infoPtr->note)
        {
            DTTOPTS opts;

            opts.dwSize = sizeof(opts);
            opts.dwFlags = DTT_FONTPROP | DTT_CALCRECT;
            opts.iFontPropId = TMT_BODYFONT;
            DrawThemeTextEx(theme, hdc, BP_COMMANDLINK, CMDLS_NORMAL,
                            infoPtr->note, infoPtr->note_length,
                            flags | DT_NOPREFIX | DT_CALCRECT, &text_bound, &opts);
            text_w = max(text_w, text_bound.right - text_bound.left);
            text_h += text_bound.bottom - text_bound.top;
        }
    }
    else
    {
        NONCLIENTMETRICSW ncm;

        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        {
            LONG note_weight = ncm.lfMessageFont.lfWeight;

            /* Find the text's rect */
            ncm.lfMessageFont.lfWeight = FW_BOLD;
            if ((font = CreateFontIndirectW(&ncm.lfMessageFont)))
            {
                if ((text = get_button_text(infoPtr)))
                {
                    RECT r = text_bound;
                    old_font = SelectObject(hdc, font);
                    DrawTextW(hdc, text, -1, &r, flags | DT_CALCRECT);
                    Free(text);

                    text_w = r.right - r.left;
                    text_h = r.bottom - r.top;
                }
                DeleteObject(font);
            }

            /* Find the note's rect */
            ncm.lfMessageFont.lfWeight = note_weight;
            if (infoPtr->note && (font = CreateFontIndirectW(&ncm.lfMessageFont)))
            {
                HFONT tmp = SelectObject(hdc, font);
                if (!old_font) old_font = tmp;

                DrawTextW(hdc, infoPtr->note, infoPtr->note_length, &text_bound,
                          flags | DT_NOPREFIX | DT_CALCRECT);
                DeleteObject(font);

                text_w = max(text_w, text_bound.right - text_bound.left);
                text_h += text_bound.bottom - text_bound.top + 2;
            }
        }
    }
    w += text_w;

    size->cx = min(size->cx, w);
    size->cy = max(text_h, img_size.cy) + margin.top + margin.bottom;

    if (old_font) SelectObject(hdc, old_font);
    ReleaseDC(infoPtr->hwnd, hdc);
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
   WCHAR *text = get_button_text(infoPtr);
   SIZE imageSize = BUTTON_GetImageSize(infoPtr);
   RECT labelRect, imageRect, imageRectWithMargin, textRect;
   LONG imageMarginWidth, imageMarginHeight;
   const RECT *textMargin = BUTTON_GetTextMargin(infoPtr);
   LONG style, ex_style, split_style;
   RECT emptyMargin = {0};
   LONG maxTextWidth;
   UINT dtStyle;

   /* Calculate label rectangle according to label type */
   if ((imageSize.cx == 0 && imageSize.cy == 0) && (text == NULL || text[0] == '\0'))
   {
       SetRectEmpty(labelRc);
       SetRectEmpty(imageRc);
       SetRectEmpty(textRc);
       Free(text);
       return (UINT)-1;
   }

   style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
   ex_style = GetWindowLongW(infoPtr->hwnd, GWL_EXSTYLE);
   /* Add BS_RIGHT directly. When both WS_EX_RIGHT and BS_LEFT are present, it becomes BS_CENTER */
   if (ex_style & WS_EX_RIGHT)
       style |= BS_RIGHT;
   split_style = infoPtr->imagelist.himl ? BUTTON_ILStoBS(infoPtr->imagelist.uAlign) : style;
   dtStyle = BUTTON_BStoDT(style, ex_style);

   /* Group boxes are top aligned unless BS_PUSHLIKE is set and it's not themed */
   if (get_button_type(style) == BS_GROUPBOX
       && (!(style & BS_PUSHLIKE) || GetWindowTheme(infoPtr->hwnd)))
       style &= ~BS_VCENTER | BS_TOP;

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
           BUTTON_PositionRect(style, labelRc, &textRect, textMargin);
           labelRect = textRect;
           SetRectEmpty(&imageRect);
       }
   }
   Free(text);

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
   DrawStateW(hdc, hbr, BUTTON_DrawTextCallback, (LPARAM)text, dtFlags, textRect->left, textRect->top,
              textRect->right - textRect->left, textRect->bottom - textRect->top, flags);
   Free(text);
}

static void BUTTON_DrawThemedLabel(const BUTTON_INFO *info, HDC hdc, UINT text_flags,
                                   const RECT *image_rect, const RECT *text_rect, HTHEME theme,
                                   int part, int state)
{
    HBRUSH brush = NULL;
    UINT image_flags;
    WCHAR *text;

    if (show_image(info))
    {
        image_flags = IsWindowEnabled(info->hwnd) ? DSS_NORMAL : DSS_DISABLED;

        if ((GetWindowLongW(info->hwnd, GWL_STYLE) & BS_PUSHLIKE)
            && (info->state & BST_INDETERMINATE))
        {
            brush = GetSysColorBrush(COLOR_GRAYTEXT);
            image_flags |= DSS_MONO;
        }

        BUTTON_DrawImage(info, hdc, brush, image_flags, image_rect);
    }

   if (show_image_only(info))
       return;

   if (!(text = get_button_text(info)))
       return;

   DrawThemeText(theme, hdc, part, state, text, lstrlenW(text), text_flags, 0, text_rect);
   Free(text);
}

/**********************************************************************
 *       Push Button Functions
 */
static void PB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT     rc, labelRect, imageRect, textRect;
    UINT     dtFlags = (UINT)-1, uState;
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
        InflateRect( &rc, -2, -2 );
        DrawFocusRect( hDC, &rc );
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

/* Get adjusted check box or radio box rectangle */
static RECT get_box_rect(LONG style, LONG ex_style, const RECT *content_rect,
                         const RECT *label_rect, BOOL has_label, SIZE box_size)
{
    RECT rect;
    int delta;

    rect = *content_rect;

    if (style & BS_LEFTTEXT || ex_style & WS_EX_RIGHT)
        rect.left = rect.right - box_size.cx;
    else
        rect.right = rect.left + box_size.cx;

    /* Adjust box when label is valid */
    if (has_label)
    {
        rect.top = label_rect->top;
        rect.bottom = label_rect->bottom;
    }

    /* Box must have the correct height */
    delta = rect.bottom - rect.top - box_size.cy;
    if ((style & BS_VCENTER) == BS_TOP)
    {
        if (delta <= 0)
            rect.top -= -delta / 2 + 1;

        rect.bottom = rect.top + box_size.cy;
    }
    else if ((style & BS_VCENTER) == BS_BOTTOM)
    {
        if (delta <= 0)
            rect.bottom += -delta / 2 + 1;

        rect.top = rect.bottom - box_size.cy;
    }
    else
    {
        if (delta > 0)
        {
            rect.bottom -= delta / 2 + 1;
            rect.top = rect.bottom - box_size.cy;
        }
        else if (delta < 0)
        {
            rect.top -= -delta / 2 + 1;
            rect.bottom = rect.top + box_size.cy;
        }
    }

    return rect;
}

static void CB_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    RECT rbox, labelRect, oldLabelRect, imageRect, textRect, client;
    HBRUSH hBrush;
    int text_offset;
    UINT dtFlags;
    LRESULT cdrf;
    HFONT hFont;
    NMCUSTOMDRAW nmcd;
    LONG state = infoPtr->state;
    LONG style = GetWindowLongW( infoPtr->hwnd, GWL_STYLE );
    LONG ex_style = GetWindowLongW( infoPtr->hwnd, GWL_EXSTYLE );
    SIZE box_size;
    HWND parent;
    HRGN hrgn;

    if (style & BS_PUSHLIKE)
    {
        PB_Paint( infoPtr, hDC, action );
	return;
    }

    GetClientRect(infoPtr->hwnd, &client);
    labelRect = client;

    box_size.cx = 12 * GetDpiForWindow(infoPtr->hwnd) / 96 + 1;
    box_size.cy = box_size.cx;

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
        labelRect.right -= box_size.cx + text_offset;
    else
        labelRect.left += box_size.cx + text_offset;

    oldLabelRect = labelRect;
    dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &labelRect, &imageRect, &textRect);
    rbox = get_box_rect(style, ex_style, &client, &labelRect, dtFlags != (UINT)-1L, box_size);

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
        IntersectRect(&labelRect, &labelRect, &oldLabelRect);
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
    start = sibling = GetNextDlgGroupItem( parent, hwnd, TRUE );
    do
    {
        if (!sibling) break;
        if ((hwnd != sibling) &&
            ((GetWindowLongW( sibling, GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON))
            SendMessageW( sibling, BM_SETCHECK, BST_UNCHECKED, 0 );
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
                             L"Marlett" };
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
 *       Command Link Functions
 */
static void CL_Paint( const BUTTON_INFO *infoPtr, HDC hDC, UINT action )
{
    LONG style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    LONG state = infoPtr->state;

    RECT rc, content_rect;
    NMCUSTOMDRAW nmcd;
    HPEN pen, old_pen;
    HBRUSH old_brush;
    INT old_bk_mode;
    LRESULT cdrf;
    HWND parent;
    HRGN hrgn;

    GetClientRect(infoPtr->hwnd, &rc);

    /* Command Links are not affected by the button's font, and are based
       on the default message font. Furthermore, they are not affected by
       any of the alignment styles (and always align with the top-left). */
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
    content_rect = rc;

    if (get_button_type(style) == BS_DEFCOMMANDLINK)
    {
        if (action != ODA_FOCUS)
            Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
        InflateRect(&rc, -1, -1);
    }

    /* Skip the frame drawing if only focus has changed */
    if (action != ODA_FOCUS)
    {
        if (!(state & (BST_HOT | BST_PUSHED | BST_CHECKED | BST_INDETERMINATE)))
            FillRect(hDC, &rc, GetSysColorBrush(COLOR_BTNFACE));
        else
        {
            UINT flags = DFCS_BUTTONPUSH;

            if (style & BS_FLAT) flags |= DFCS_MONO;
            else if (state & BST_PUSHED) flags |= DFCS_PUSHED;

            if (state & (BST_CHECKED | BST_INDETERMINATE))
                flags |= DFCS_CHECKED;
            DrawFrameControl(hDC, &rc, DFC_BUTTON, flags);
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

    if (!(cdrf & CDRF_DOERASE) && action != ODA_FOCUS)
    {
        UINT flags = IsWindowEnabled(infoPtr->hwnd) ? DSS_NORMAL : DSS_DISABLED;
        COLORREF old_color = SetTextColor(hDC, GetSysColor(flags == DSS_NORMAL ?
                                                           COLOR_BTNTEXT : COLOR_GRAYTEXT));
        HIMAGELIST defimg = NULL;
        NONCLIENTMETRICSW ncm;
        UINT txt_h = 0;
        SIZE img_size;

        /* Command Links ignore the margins of the image list or its alignment */
        if (infoPtr->u.image || infoPtr->imagelist.himl)
            img_size = BUTTON_GetImageSize(infoPtr);
        else
        {
            img_size.cx = img_size.cy = command_link_defglyph_size;
            defimg = ImageList_LoadImageW(COMCTL32_hModule, (LPCWSTR)MAKEINTRESOURCE(IDB_CMDLINK),
                                          img_size.cx, 3, CLR_NONE, IMAGE_BITMAP, LR_CREATEDIBSECTION);
        }

        /* Shrink rect by the command link margin, except on bottom (just the frame) */
        InflateRect(&content_rect, -command_link_margin, -command_link_margin);
        content_rect.bottom += command_link_margin - 2;

        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        {
            LONG note_weight = ncm.lfMessageFont.lfWeight;
            RECT r = content_rect;
            WCHAR *text;
            HFONT font;

            if (img_size.cx) r.left += img_size.cx + command_link_margin;

            /* Draw the text */
            ncm.lfMessageFont.lfWeight = FW_BOLD;
            if ((font = CreateFontIndirectW(&ncm.lfMessageFont)))
            {
                if ((text = get_button_text(infoPtr)))
                {
                    SelectObject(hDC, font);
                    txt_h = DrawTextW(hDC, text, -1, &r,
                                      DT_TOP | DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
                    Free(text);
                }
                DeleteObject(font);
            }

            /* Draw the note */
            ncm.lfMessageFont.lfWeight = note_weight;
            if (infoPtr->note && (font = CreateFontIndirectW(&ncm.lfMessageFont)))
            {
                r.top += txt_h + 2;
                SelectObject(hDC, font);
                DrawTextW(hDC, infoPtr->note, infoPtr->note_length, &r,
                          DT_TOP | DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
                DeleteObject(font);
            }
        }

        /* Position the image at the vertical center of the drawn text (not note) */
        txt_h = min(txt_h, content_rect.bottom - content_rect.top);
        if (img_size.cy < txt_h) content_rect.top += (txt_h - img_size.cy) / 2;

        content_rect.right = content_rect.left + img_size.cx;
        content_rect.bottom = content_rect.top + img_size.cy;

        if (defimg)
        {
            int i = 0;
            if (flags == DSS_DISABLED) i = 2;
            else if (state & BST_HOT)  i = 1;

            ImageList_Draw(defimg, i, hDC, content_rect.left, content_rect.top, ILD_NORMAL);
            ImageList_Destroy(defimg);
        }
        else
            BUTTON_DrawImage(infoPtr, hDC, NULL, flags, &content_rect);

        SetTextColor(hDC, old_color);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if (cdrf & CDRF_SKIPPOSTPAINT) goto cleanup;

    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
        InflateRect(&rc, -2, -2);
        DrawFocusRect(hDC, &rc);
    }

cleanup:
    SelectObject(hDC, old_pen);
    SelectObject(hDC, old_brush);
    SetBkMode(hDC, old_bk_mode);
    SelectClipRgn(hDC, hrgn);
    if (hrgn) DeleteObject(hrgn);
    DeleteObject(pen);
}


/**********************************************************************
 *       Themed Paint Functions
 */
static void PB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
{
    RECT bgRect, labelRect, imageRect, textRect, focusRect;
    NMCUSTOMDRAW nmcd;
    HBRUSH brush;
    LRESULT cdrf;
    HWND parent;

    if (infoPtr->font) SelectObject(hDC, infoPtr->font);

    GetClientRect(infoPtr->hwnd, &bgRect);
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &bgRect, &labelRect);
    focusRect = labelRect;

    init_custom_draw(&nmcd, infoPtr, hDC, &bgRect);

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) return;

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
    {
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
        /* Tests show that the brush from WM_CTLCOLORBTN is used for filling background after a
         * DrawThemeParentBackground() call */
        brush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
        FillRect(hDC, &bgRect, brush ? brush : GetSysColorBrush(COLOR_BTNFACE));
    }
    DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &bgRect, NULL);

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) return;

    if (!(cdrf & CDRF_DOERASE))
    {
        dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &labelRect, &imageRect, &textRect);
        if (dtFlags != (UINT)-1L)
            BUTTON_DrawThemedLabel(infoPtr, hDC, dtFlags, &imageRect, &textRect, theme,
                                   BP_PUSHBUTTON, state);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if (cdrf & CDRF_SKIPPOSTPAINT) return;

    if (focused) DrawFocusRect(hDC, &focusRect);
}

static void CB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
{
    RECT client_rect, content_rect, old_label_rect, label_rect, box_rect, image_rect, text_rect;
    HFONT font, hPrevFont = NULL;
    DWORD dwStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    LONG ex_style = GetWindowLongW(infoPtr->hwnd, GWL_EXSTYLE);
    UINT btn_type = get_button_type( dwStyle );
    int part = (btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON) ? BP_RADIOBUTTON : BP_CHECKBOX;
    NMCUSTOMDRAW nmcd;
    HBRUSH brush;
    LRESULT cdrf;
    LOGFONTW lf;
    HWND parent;
    BOOL created_font = FALSE;
    int text_offset;
    SIZE box_size;
    HRGN region;
    HRESULT hr;

    if (dwStyle & BS_PUSHLIKE)
    {
        PB_ThemedPaint(theme, infoPtr, hDC, state, dtFlags, focused);
        return;
    }

    hr = GetThemeFont(theme, hDC, part, state, TMT_FONT, &lf);
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
        if (infoPtr->font) SelectObject(hDC, infoPtr->font);
    }

    GetClientRect(infoPtr->hwnd, &client_rect);
    GetThemeBackgroundContentRect(theme, hDC, part, state, &client_rect, &content_rect);
    region = set_control_clipping(hDC, &client_rect);

    if (FAILED(GetThemePartSize(theme, hDC, part, state, &content_rect, TS_DRAW, &box_size)))
    {
        box_size.cx = 12 * GetDpiForWindow(infoPtr->hwnd) / 96 + 1;
        box_size.cy = box_size.cx;
    }

    GetCharWidthW(hDC, '0', '0', &text_offset);
    text_offset /= 2;

    label_rect = content_rect;
    if (dwStyle & BS_LEFTTEXT || ex_style & WS_EX_RIGHT)
        label_rect.right -= box_size.cx + text_offset;
    else
        label_rect.left += box_size.cx + text_offset;

    old_label_rect = label_rect;
    dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &label_rect, &image_rect, &text_rect);
    box_rect = get_box_rect(dwStyle, ex_style, &content_rect, &label_rect, dtFlags != (UINT)-1L,
                            box_size);

    init_custom_draw(&nmcd, infoPtr, hDC, &client_rect);

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    /* Tests show that the brush from WM_CTLCOLORSTATIC is used for filling background after a
     * DrawThemeParentBackground() call */
    brush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);
    FillRect(hDC, &client_rect, brush ? brush : GetSysColorBrush(COLOR_BTNFACE));

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) goto cleanup;

    /* Draw label */
    if (!(cdrf & CDRF_DOERASE))
    {
        DrawThemeBackground(theme, hDC, part, state, &box_rect, NULL);
        if (dtFlags != (UINT)-1L)
            BUTTON_DrawThemedLabel(infoPtr, hDC, dtFlags, &image_rect, &text_rect, theme, part, state);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if ((cdrf & CDRF_SKIPPOSTPAINT) || dtFlags == (UINT)-1L) goto cleanup;

    if (focused)
    {
        label_rect.left--;
        label_rect.right++;
        IntersectRect(&label_rect, &label_rect, &old_label_rect);
        DrawFocusRect(hDC, &label_rect);
    }

cleanup:
    SelectClipRgn(hDC, region);
    if (region) DeleteObject(region);
    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

static void GB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
{
    RECT clientRect, contentRect, labelRect, imageRect, textRect, bgRect;
    HRGN region, textRegion = NULL;
    LOGFONTW lf;
    HFONT font, hPrevFont = NULL;
    BOOL created_font = FALSE;
    TEXTMETRICW textMetric;
    HBRUSH brush;
    HWND parent;
    HRESULT hr;
    LONG style;
    int part;

    /* DrawThemeParentBackground() is used for filling content background. The brush from
     * WM_CTLCOLORSTATIC is used for filling text background */
    parent = GetParent(infoPtr->hwnd);
    if (!parent)
        parent = infoPtr->hwnd;
    brush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)infoPtr->hwnd);

    hr = GetThemeFont(theme, hDC, BP_GROUPBOX, state, TMT_FONT, &lf);
    if (SUCCEEDED(hr)) {
        font = CreateFontIndirectW(&lf);
        if (!font)
            TRACE("Failed to create font\n");
        else {
            hPrevFont = SelectObject(hDC, font);
            created_font = TRUE;
        }
    } else {
        if (infoPtr->font)
            SelectObject(hDC, infoPtr->font);
    }

    GetClientRect(infoPtr->hwnd, &clientRect);
    region = set_control_clipping(hDC, &clientRect);

    bgRect = clientRect;
    GetTextMetricsW(hDC, &textMetric);
    bgRect.top += (textMetric.tmHeight / 2) - 1;

    labelRect = clientRect;
    InflateRect(&labelRect, -7, 1);
    dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &labelRect, &imageRect, &textRect);
    if (dtFlags != (UINT)-1 && !show_image_only(infoPtr))
    {
        textRegion = CreateRectRgnIndirect(&textRect);
        ExtSelectClipRgn(hDC, textRegion, RGN_DIFF);
    }

    style = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    if (style & BS_PUSHLIKE)
    {
        part = BP_PUSHBUTTON;
    }
    else
    {
        part = BP_GROUPBOX;
        GetThemeBackgroundContentRect(theme, hDC, part, state, &bgRect, &contentRect);
        ExcludeClipRect(hDC, contentRect.left, contentRect.top, contentRect.right, contentRect.bottom);
    }
    if (IsThemeBackgroundPartiallyTransparent(theme, part, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    DrawThemeBackground(theme, hDC, part, state, &bgRect, NULL);

    if (dtFlags != (UINT)-1)
    {
        if (textRegion)
        {
            SelectClipRgn(hDC, textRegion);
            DeleteObject(textRegion);
        }
        FillRect(hDC, &textRect, brush ? brush : GetSysColorBrush(COLOR_BTNFACE));
        BUTTON_DrawThemedLabel(infoPtr, hDC, dtFlags, &imageRect, &textRect, theme, part, state);
    }

    SelectClipRgn(hDC, region);
    if (region) DeleteObject(region);
    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

static void SB_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
{
    RECT rc, content_rect, push_rect, dropdown_rect, focus_rect, label_rect, image_rect, text_rect;
    NMCUSTOMDRAW nmcd;
    LRESULT cdrf;
    HWND parent;

    if (infoPtr->font) SelectObject(hDC, infoPtr->font);

    GetClientRect(infoPtr->hwnd, &rc);
    init_custom_draw(&nmcd, infoPtr, hDC, &rc);

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) return;

    if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);

    /* The zone outside the content is ignored for the dropdown (draws over) */
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &rc, &content_rect);
    get_split_button_rects(infoPtr, &rc, &push_rect, &dropdown_rect);

    if (infoPtr->split_style & BCSS_NOSPLIT)
    {
        push_rect = rc;
        DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &rc, NULL);
        GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &push_rect, &focus_rect);
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
        focus_rect = content_rect;
    }

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) return;

    if (!(cdrf & CDRF_DOERASE))
    {
        COLORREF old_color, color;
        INT old_bk_mode;

        label_rect = content_rect;
        dtFlags = BUTTON_CalcLayoutRects(infoPtr, hDC, &label_rect, &image_rect, &text_rect);
        if (dtFlags != (UINT)-1L)
            BUTTON_DrawThemedLabel(infoPtr, hDC, dtFlags, &image_rect, &text_rect, theme,
                                   BP_PUSHBUTTON, state);

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
    if (cdrf & CDRF_SKIPPOSTPAINT) return;

    if (focused) DrawFocusRect(hDC, &focus_rect);
}

static void CL_ThemedPaint(HTHEME theme, const BUTTON_INFO *infoPtr, HDC hDC, int state, UINT dtFlags, BOOL focused)
{
    NMCUSTOMDRAW nmcd;
    LRESULT cdrf;
    HWND parent;
    int part;
    RECT rc;

    if (infoPtr->font) SelectObject(hDC, infoPtr->font);

    GetClientRect(infoPtr->hwnd, &rc);
    init_custom_draw(&nmcd, infoPtr, hDC, &rc);

    parent = GetParent(infoPtr->hwnd);
    if (!parent) parent = infoPtr->hwnd;

    /* Send erase notifications */
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) return;

    part = GetWindowLongW(infoPtr->hwnd, GWL_STYLE) & BS_PUSHLIKE ? BP_PUSHBUTTON : BP_COMMANDLINK;
    if (IsThemeBackgroundPartiallyTransparent(theme, part, state))
        DrawThemeParentBackground(infoPtr->hwnd, hDC, NULL);
    DrawThemeBackground(theme, hDC, part, state, &rc, NULL);

    if (cdrf & CDRF_NOTIFYPOSTERASE)
    {
        nmcd.dwDrawStage = CDDS_POSTERASE;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }

    /* Send paint notifications */
    nmcd.dwDrawStage = CDDS_PREPAINT;
    cdrf = SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    if (cdrf & CDRF_SKIPDEFAULT) return;

    if (!(cdrf & CDRF_DOERASE))
    {
        RECT r, img_rect;
        UINT txt_h = 0;
        SIZE img_size;
        WCHAR *text;

        GetThemeBackgroundContentRect(theme, hDC, part, state, &rc, &r);

        /* The text alignment and styles are fixed and don't depend on button styles */
        dtFlags = DT_TOP | DT_LEFT | DT_WORDBREAK;

        /* Command Links ignore the margins of the image list or its alignment */
        if (infoPtr->u.image || infoPtr->imagelist.himl)
            img_size = BUTTON_GetImageSize(infoPtr);
        else
            GetThemePartSize(theme, NULL, BP_COMMANDLINKGLYPH, state, NULL, TS_DRAW, &img_size);

        img_rect = r;
        if (img_size.cx) r.left += img_size.cx + command_link_margin;

        /* Draw the text */
        if ((text = get_button_text(infoPtr)))
        {
            UINT len = lstrlenW(text);
            RECT text_rect;

            GetThemeTextExtent(theme, hDC, part, state, text, len, dtFlags | DT_END_ELLIPSIS, &r,
                               &text_rect);
            DrawThemeText(theme, hDC, part, state, text, len, dtFlags | DT_END_ELLIPSIS, 0, &r);

            txt_h = text_rect.bottom - text_rect.top;
            Free(text);
        }

        /* Draw the note */
        if (infoPtr->note)
        {
            DTTOPTS opts;

            r.top += txt_h;
            opts.dwSize = sizeof(opts);
            opts.dwFlags = DTT_FONTPROP;
            opts.iFontPropId = TMT_BODYFONT;
            DrawThemeTextEx(theme, hDC, part, state, infoPtr->note, infoPtr->note_length,
                            dtFlags | DT_NOPREFIX, &r, &opts);
        }

        /* Position the image at the vertical center of the drawn text (not note) */
        txt_h = min(txt_h, img_rect.bottom - img_rect.top);
        if (img_size.cy < txt_h) img_rect.top += (txt_h - img_size.cy) / 2;

        img_rect.right = img_rect.left + img_size.cx;
        img_rect.bottom = img_rect.top + img_size.cy;

        if (infoPtr->u.image || infoPtr->imagelist.himl)
            BUTTON_DrawImage(infoPtr, hDC, NULL,
                             (state == CMDLS_DISABLED) ? DSS_DISABLED : DSS_NORMAL,
                             &img_rect);
        else
            DrawThemeBackground(theme, hDC, BP_COMMANDLINKGLYPH, state, &img_rect, NULL);
    }

    if (cdrf & CDRF_NOTIFYPOSTPAINT)
    {
        nmcd.dwDrawStage = CDDS_POSTPAINT;
        SendMessageW(parent, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);
    }
    if (cdrf & CDRF_SKIPPOSTPAINT) return;

    if (focused)
    {
        MARGINS margins;

        /* The focus rect has margins of a push button rather than command link... */
        GetThemeMargins(theme, hDC, BP_PUSHBUTTON, state, TMT_CONTENTMARGINS, NULL, &margins);

        rc.left += margins.cxLeftWidth;
        rc.top += margins.cyTopHeight;
        rc.right -= margins.cxRightWidth;
        rc.bottom -= margins.cyBottomHeight;
        DrawFocusRect(hDC, &rc);
    }
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
