/* File: button.c -- Button type widgets
 *
 * Copyright (C) 1993 Johannes Ruscheinski
 * Copyright (C) 1993 David Metcalfe
 * Copyright (C) 1994 Alexandre Julliard
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
 * NOTES
 *
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Oct. 3, 2004, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
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
#include "comctl32.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(button);

/* GetWindowLong offsets for window extra information */
#define STATE_GWL_OFFSET  0
#define HFONT_GWL_OFFSET  (sizeof(LONG))
#define HIMAGE_GWL_OFFSET (HFONT_GWL_OFFSET+sizeof(HFONT))
#define UISTATE_GWL_OFFSET (HIMAGE_GWL_OFFSET+sizeof(HFONT))
#define NB_EXTRA_BYTES    (UISTATE_GWL_OFFSET+sizeof(LONG))

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

static UINT BUTTON_CalcLabelRect( HWND hwnd, HDC hdc, RECT *rc );
static void PB_Paint( HWND hwnd, HDC hDC, UINT action );
static void CB_Paint( HWND hwnd, HDC hDC, UINT action );
static void GB_Paint( HWND hwnd, HDC hDC, UINT action );
static void UB_Paint( HWND hwnd, HDC hDC, UINT action );
static void OB_Paint( HWND hwnd, HDC hDC, UINT action );
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
    BST_UNCHECKED       /* BS_OWNERDRAW */
};

typedef void (*pfPaint)( HWND hwnd, HDC hdc, UINT action );

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
    OB_Paint     /* BS_OWNERDRAW */
};

/* The original code from user32 was kept in order to make it easier to bring changes from user32 */
#ifdef _USER32_
/*********************************************************************
 * button class descriptor
 */
static const WCHAR buttonW[] = {'B','u','t','t','o','n',0};
const struct builtin_class_descr BUTTON_builtin_class =
{
    buttonW,             /* name */
    CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC, /* style  */
#ifdef __REACTOS__
    ButtonWndProcA,      /* procA */
    ButtonWndProcW,      /* procW */
#else
    WINPROC_BUTTON,      /* proc */
#endif
    NB_EXTRA_BYTES,      /* extra */
    IDC_ARROW,           /* cursor */
    0                    /* brush */
};


static inline LONG get_button_state( HWND hwnd )
{
    return GetWindowLongPtrW( hwnd, STATE_GWL_OFFSET );
}

static inline void set_button_state( HWND hwnd, LONG state )
{
    SetWindowLongPtrW( hwnd, STATE_GWL_OFFSET, state );
}

#ifdef __REACTOS__

static __inline void set_ui_state( HWND hwnd, LONG flags )
{
    SetWindowLongPtrW( hwnd, UISTATE_GWL_OFFSET, flags );
}

static __inline LONG get_ui_state( HWND hwnd )
{
    return GetWindowLongPtrW( hwnd, UISTATE_GWL_OFFSET );
}

#endif /* __REACTOS__ */

static inline HFONT get_button_font( HWND hwnd )
{
    return (HFONT)GetWindowLongPtrW( hwnd, HFONT_GWL_OFFSET );
}

static inline void set_button_font( HWND hwnd, HFONT font )
{
    SetWindowLongPtrW( hwnd, HFONT_GWL_OFFSET, (LONG_PTR)font );
}

static inline UINT get_button_type( LONG window_style )
{
    return (window_style & BS_TYPEMASK);
}

/* paint a button of any type */
static inline void paint_button( HWND hwnd, LONG style, UINT action )
{
    if (btnPaintFunc[style] && IsWindowVisible(hwnd))
    {
        HDC hdc = GetDC( hwnd );
        btnPaintFunc[style]( hwnd, hdc, action );
        ReleaseDC( hwnd, hdc );
    }
}

#else

#define NtUserAlterWindowStyle SetWindowLongPtrW

static inline void _SetButtonData(HWND hwnd, PBUTTON_DATA data)
{
    SetWindowLongPtrW( hwnd, 0, (LONG)data );
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

BOOL BUTTON_PaintWithTheme(HTHEME theme, HWND hwnd, HDC hParamDC, LPARAM prfFlag);
WCHAR *get_button_text( HWND hwnd );

static inline LONG_PTR get_button_image(HWND hwnd)
{
    return _GetButtonData(hwnd)->image;
}

static inline LONG_PTR set_button_image(HWND hwnd, LONG_PTR image)
{
    PBUTTON_DATA data = _GetButtonData(hwnd);
    LONG_PTR ret = data->image;
    data->image = image;
    return ret;
}

static inline LONG get_button_state( HWND hwnd )
{
    return _GetButtonData(hwnd)->state;
}

static inline void set_button_state( HWND hwnd, LONG state )
{
    _GetButtonData(hwnd)->state = state;
}

static __inline void set_ui_state( HWND hwnd, LONG flags )
{
    _GetButtonData(hwnd)->ui_state = flags;
}

static __inline LONG get_ui_state( HWND hwnd )
{
    return _GetButtonData(hwnd)->ui_state;
}

static inline HFONT get_button_font( HWND hwnd )
{
    return (HFONT)_GetButtonData(hwnd)->font;
}

static inline void set_button_font( HWND hwnd, HFONT font )
{
    _GetButtonData(hwnd)->font = font;
}

static inline UINT get_button_type( LONG window_style )
{
    return (window_style & BS_TYPEMASK);
}

/* paint a button of any type */
static inline void paint_button( HWND hwnd, LONG style, UINT action )
{
    HTHEME theme = GetWindowTheme(hwnd);
    RECT rc;
    HDC hdc = GetDC( hwnd );
    /* GetDC appears to give a dc with a clip rect that includes the whoe parent, not sure if it is correct or not. */
    GetClientRect(hwnd, &rc);
    IntersectClipRect (hdc, rc.left, rc. top, rc.right, rc.bottom);
    if (theme && BUTTON_PaintWithTheme(theme, hwnd, hdc, 0))
    {
        ReleaseDC( hwnd, hdc );
        return;
    }
    if (btnPaintFunc[style] && IsWindowVisible(hwnd))
    {
        btnPaintFunc[style]( hwnd, hdc, action );
    }
    ReleaseDC( hwnd, hdc );
}

BOOL BUTTON_GetIdealSize(HTHEME theme, HWND hwnd, SIZE* psize)
{
    PBUTTON_DATA pdata;
    HDC hdc;
    WCHAR *text;
    HFONT hFont = 0, hPrevFont = 0;
    SIZE TextSize, ImageSize, ButtonSize;
    BOOL ret = FALSE;
    LOGFONTW logfont = {0};

    pdata = _GetButtonData(hwnd);
    text = get_button_text( hwnd );
    hdc = GetDC(hwnd);
    if (!pdata || !text || !hdc || !text[0])
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
        if (pdata->font)
            hPrevFont = SelectObject( hdc, pdata->font );
    }

    GetTextExtentPoint32W(hdc, text, wcslen(text), &TextSize);

    if (logfont.lfHeight == -1 && logfont.lfWidth == 0 && wcscmp(logfont.lfFaceName, L"Arial") == 0 && wcsicmp(text, L"Start") == 0)
    {
        TextSize.cx = 5;
        TextSize.cy = 4;
    }

    if (hPrevFont)
        SelectObject( hdc, hPrevFont );

    TextSize.cy += pdata->rcTextMargin.top + pdata->rcTextMargin.bottom;
    TextSize.cx += pdata->rcTextMargin.left + pdata->rcTextMargin.right;

    if (pdata->imlData.himl && ImageList_GetIconSize(pdata->imlData.himl, &ImageSize.cx, &ImageSize.cy))
    {
        ImageSize.cx += pdata->imlData.margin.left + pdata->imlData.margin.right;
        ImageSize.cy += pdata->imlData.margin.top + pdata->imlData.margin.bottom;
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
        ReleaseDC(hwnd, hdc);

    return ret;
}

#endif


/* retrieve the button text; returned buffer must be freed by caller */
inline WCHAR *get_button_text( HWND hwnd )
{
    INT len = 512;
    WCHAR *buffer = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
    if (buffer) InternalGetWindowText( hwnd, buffer, len + 1 );
    return buffer;
}

#ifdef __REACTOS__
/* Retrieve the UI state for the control */
static BOOL button_update_uistate(HWND hwnd, BOOL unicode)
{
    LONG flags, prevflags;

    if (unicode)
        flags = DefWindowProcW(hwnd, WM_QUERYUISTATE, 0, 0);
    else
        flags = DefWindowProcA(hwnd, WM_QUERYUISTATE, 0, 0);

    prevflags = get_ui_state(hwnd);

    if (prevflags != flags)
    {
        set_ui_state(hwnd, flags);
        return TRUE;
    }

    return FALSE;
}
#endif

/***********************************************************************
 *           ButtonWndProc_common
 */
LRESULT WINAPI ButtonWndProc_common(HWND hWnd, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    RECT rect;
    POINT pt;
    LONG style = GetWindowLongPtrW( hWnd, GWL_STYLE );
    UINT btn_type = get_button_type( style );
    LONG state;
    HANDLE oldHbitmap;
#if defined(__REACTOS__) && defined(_USER32_)
    PWND pWnd;

    pWnd = ValidateHwnd(hWnd);
    if (pWnd)
    {
       if (!pWnd->fnid)
       {
          NtUserSetWindowFNID(hWnd, FNID_BUTTON);
       }
       else
       {
          if (pWnd->fnid != FNID_BUTTON)
          {
             ERR("Wrong window class for Button! fnId 0x%x\n",pWnd->fnid);
             return 0;
          }
       }
    }
    else
       return 0;
#else
    if (!IsWindow( hWnd )) return 0;
#endif

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

#ifndef _USER32_
    switch (uMsg)
    {
        case WM_NCCREATE:
        {
            PBUTTON_DATA data = HeapAlloc( GetProcessHeap(), 0, sizeof(BUTTON_DATA) );
            if (!data)
            {
                ERR("Failed to alloc internal button data\n");
                return -1;
            }

            memset(data, 0, sizeof(BUTTON_DATA));
            SetRect(&data->rcTextMargin, 1,1,1,1);

            _SetButtonData(hWnd, data);
            break;
        }
        case WM_NCDESTROY:
        {
            PBUTTON_DATA data = _GetButtonData(hWnd);
            if (!data)
            {
                ERR("No data");
                return 0;
            }
            HeapFree( GetProcessHeap(), 0, data );
            _SetButtonData(hWnd, NULL);
        }
        case WM_CREATE:
            OpenThemeData(hWnd, WC_BUTTONW);
            break;
        case WM_DESTROY:
            CloseThemeData (GetWindowTheme(hWnd));
            break;
        case WM_THEMECHANGED:
            CloseThemeData (GetWindowTheme(hWnd));
            OpenThemeData(hWnd, WC_BUTTONW);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        case WM_MOUSELEAVE:
        {
            state = get_button_state( hWnd );
            if (state & BST_HOT)
            {
                NMBCHOTITEM nmhotitem;

                state &= ~BST_HOT;
                set_button_state(hWnd, state);

                nmhotitem.hdr.hwndFrom = hWnd;
                nmhotitem.hdr.idFrom   = GetWindowLongPtrW (hWnd, GWLP_ID);
                nmhotitem.hdr.code     = BCN_HOTITEMCHANGE;
                nmhotitem.dwFlags      = HICF_LEAVING;
                SendMessageW(GetParent(hWnd), WM_NOTIFY, nmhotitem.hdr.idFrom, (LPARAM)&nmhotitem);

                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT mouse_event;
            state = get_button_state( hWnd );
            if ((state & BST_HOT) == 0)
            {
                NMBCHOTITEM nmhotitem;

                state |= BST_HOT;
                set_button_state(hWnd, state);

                nmhotitem.hdr.hwndFrom = hWnd;
                nmhotitem.hdr.idFrom   = GetWindowLongPtrW (hWnd, GWLP_ID);
                nmhotitem.hdr.code     = BCN_HOTITEMCHANGE;
                nmhotitem.dwFlags      = HICF_ENTERING;
                SendMessageW(GetParent(hWnd), WM_NOTIFY, nmhotitem.hdr.idFrom, (LPARAM)&nmhotitem);

                InvalidateRect(hWnd, NULL, TRUE);
            }

            mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
            mouse_event.dwFlags = TME_QUERY;
            if(!TrackMouseEvent(&mouse_event) || !(mouse_event.dwFlags&TME_LEAVE))
            {
                mouse_event.dwFlags = TME_LEAVE;
                mouse_event.hwndTrack = hWnd;
                mouse_event.dwHoverTime = 1;
                TrackMouseEvent(&mouse_event);
            }
            break;
        }
        case BCM_GETTEXTMARGIN:
        {
            RECT* prc = (RECT*)lParam;
            PBUTTON_DATA data = _GetButtonData(hWnd);
            if (!prc || !data)
                return FALSE;
            *prc = data->rcTextMargin;
            return TRUE;
        }
        case BCM_SETTEXTMARGIN:
        {
            RECT* prc = (RECT*)lParam;
            PBUTTON_DATA data = _GetButtonData(hWnd);
            if (!prc || !data)
                return FALSE;
            data->rcTextMargin = *prc;
            return TRUE;
        }
        case BCM_SETIMAGELIST:
        {
            BUTTON_IMAGELIST * pimldata = (BUTTON_IMAGELIST *)lParam;
            PBUTTON_DATA data = _GetButtonData(hWnd);
            if (!data || !pimldata || !pimldata->himl)
                return FALSE;
            data->imlData = *pimldata;
            return TRUE;
        }
        case BCM_GETIMAGELIST:
        {
            BUTTON_IMAGELIST * pimldata = (BUTTON_IMAGELIST *)lParam;
            PBUTTON_DATA data = _GetButtonData(hWnd);
            if (!data|| !pimldata)
                return FALSE;
            *pimldata = data->imlData;
            return TRUE;
        }
        case BCM_GETIDEALSIZE:
        {
            HTHEME theme = GetWindowTheme(hWnd);
            BOOL ret = FALSE;
            SIZE* pSize = (SIZE*)lParam;

            if (btn_type == BS_PUSHBUTTON || 
                btn_type == BS_DEFPUSHBUTTON ||
                btn_type == BS_USERBUTTON)
            {
                ret = BUTTON_GetIdealSize(theme, hWnd, pSize);
            }

            if (!ret)
            {
                GetClientRect(hWnd, &rect);
                pSize->cx = rect.right;
                pSize->cy = rect.bottom;
            }

            return TRUE;
        }
    }

    if (!_GetButtonData(hWnd))
    {
        ERR("no data!\n");
        return unicode ? DefWindowProcW(hWnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }

#endif

    switch (uMsg)
    {
    case WM_GETDLGCODE:
        switch(btn_type)
        {
        case BS_USERBUTTON:
        case BS_PUSHBUTTON:      return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
        case BS_DEFPUSHBUTTON:   return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON: return DLGC_BUTTON | DLGC_RADIOBUTTON;
        case BS_GROUPBOX:        return DLGC_STATIC;
        default:                 return DLGC_BUTTON;
        }

    case WM_ENABLE:
        paint_button( hWnd, btn_type, ODA_DRAWENTIRE );
        break;

    case WM_CREATE:
        if (btn_type >= MAX_BTN_TYPE)
            return -1; /* abort */

        /* XP turns a BS_USERBUTTON into BS_PUSHBUTTON */
        if (btn_type == BS_USERBUTTON )
        {
            style = (style & ~BS_TYPEMASK) | BS_PUSHBUTTON;
#ifdef __REACTOS__
            NtUserAlterWindowStyle(hWnd, GWL_STYLE, style );
#else
            WIN_SetStyle( hWnd, style, BS_TYPEMASK & ~style );
#endif
        }
        set_button_state( hWnd, BST_UNCHECKED );
#ifdef __REACTOS__
        button_update_uistate( hWnd, unicode );
#endif
        return 0;

#if defined(__REACTOS__) && defined(_USER32_)
    case WM_NCDESTROY:
        NtUserSetWindowFNID(hWnd, FNID_DESTROY);
    case WM_DESTROY:
        break;
#endif
    case WM_ERASEBKGND:
        if (btn_type == BS_OWNERDRAW)
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            HBRUSH hBrush;
            HWND parent = GetParent(hWnd);
            if (!parent) parent = hWnd;
#if defined(__REACTOS__) && defined(_USER32_)
            hBrush = GetControlColor( parent, hWnd, hdc, WM_CTLCOLORBTN);
#else
            hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hdc, (LPARAM)hWnd);
            if (!hBrush) /* did the app forget to call defwindowproc ? */
                hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORBTN,
                                                (WPARAM)hdc, (LPARAM)hWnd);
#endif
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, hBrush);
        }
        return 1;

    case WM_PRINTCLIENT:
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = wParam ? (HDC)wParam : BeginPaint( hWnd, &ps );
#ifndef _USER32_
        HTHEME theme = GetWindowTheme(hWnd);
        if (theme && BUTTON_PaintWithTheme(theme, hWnd, hdc, uMsg == WM_PRINTCLIENT ? lParam : 0))
        {
            if ( !wParam ) EndPaint( hWnd, &ps );
            return 0;
        }
#endif
        if (btnPaintFunc[btn_type])
        {
            int nOldMode = SetBkMode( hdc, OPAQUE );
            (btnPaintFunc[btn_type])( hWnd, hdc, ODA_DRAWENTIRE );
            SetBkMode(hdc, nOldMode); /*  reset painting mode */
        }
        if ( !wParam ) EndPaint( hWnd, &ps );
        break;
    }

    case WM_KEYDOWN:
	if (wParam == VK_SPACE)
	{
	    SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
            set_button_state( hWnd, get_button_state( hWnd ) | BUTTON_BTNPRESSED );
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
        set_button_state( hWnd, get_button_state( hWnd ) | BUTTON_BTNPRESSED );
        SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
        break;

    case WM_KEYUP:
	if (wParam != VK_SPACE)
	    break;
	/* fall through */
    case WM_LBUTTONUP:
#ifdef _REACTOS_
        BOOL TellParent = FALSE; //// ReactOS see note below.
#endif
        state = get_button_state( hWnd );
        if (!(state & BUTTON_BTNPRESSED)) break;
        state &= BUTTON_NSTATES;
        set_button_state( hWnd, state );
        if (!(state & BST_PUSHED))
        {
            ReleaseCapture();
            break;
        }
        SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        GetClientRect( hWnd, &rect );
	if (uMsg == WM_KEYUP || PtInRect( &rect, pt ))
        {
            state = get_button_state( hWnd );
            switch(btn_type)
            {
            case BS_AUTOCHECKBOX:
                SendMessageW( hWnd, BM_SETCHECK, !(state & BST_CHECKED), 0 );
                break;
            case BS_AUTORADIOBUTTON:
                SendMessageW( hWnd, BM_SETCHECK, TRUE, 0 );
                break;
            case BS_AUTO3STATE:
                SendMessageW( hWnd, BM_SETCHECK,
                                (state & BST_INDETERMINATE) ? 0 : ((state & 3) + 1), 0 );
                break;
            }
#ifdef _REACTOS_
            TellParent = TRUE; // <---- Fix CORE-10194, Notify parent after capture is released.
#else
            ReleaseCapture();
            BUTTON_NOTIFY_PARENT(hWnd, BN_CLICKED);
#endif
        }
#ifdef _REACTOS_
        ReleaseCapture();
        if (TellParent) BUTTON_NOTIFY_PARENT(hWnd, BN_CLICKED);
#else
        else
        {
            ReleaseCapture();
        }
#endif
        break;

    case WM_CAPTURECHANGED:
        TRACE("WM_CAPTURECHANGED %p\n", hWnd);
        if (hWnd == (HWND)lParam) break;
        state = get_button_state( hWnd );
        if (state & BUTTON_BTNPRESSED)
        {
            state &= BUTTON_NSTATES;
            set_button_state( hWnd, state );
            if (state & BST_PUSHED) SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        }
        break;

    case WM_MOUSEMOVE:
        if ((wParam & MK_LBUTTON) && GetCapture() == hWnd)
        {
            GetClientRect( hWnd, &rect );
            SendMessageW( hWnd, BM_SETSTATE, PtInRect(&rect, pt), 0 );
        }
        break;

    case WM_SETTEXT:
    {
        /* Clear an old text here as Windows does */
//
// ReactOS Note :
// wine Bug: http://bugs.winehq.org/show_bug.cgi?id=25790
// Patch: http://source.winehq.org/patches/data/70889
// By: Alexander LAW, Replicate Windows behavior of WM_SETTEXT handler regarding WM_CTLCOLOR*
//
#ifdef __REACTOS__
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
                            btn_type == BS_PUSHLIKE ||
                            btn_type == BS_USERBUTTON ||
                            btn_type == BS_OWNERDRAW) ?
                            WM_CTLCOLORBTN : WM_CTLCOLORSTATIC;

            if (!parent) parent = hWnd;
#if defined(__REACTOS__) && defined(_USER32_)
            hbrush = GetControlColor(parent, hWnd, hdc, message);
#else
            hbrush = (HBRUSH)SendMessageW(parent, message,
                                          (WPARAM)hdc, (LPARAM)hWnd);
            if (!hbrush) /* did the app forget to call DefWindowProc ? */
                hbrush = (HBRUSH)DefWindowProcW(parent, message,
                                                (WPARAM)hdc, (LPARAM)hWnd);
#endif

            GetClientRect(hWnd, &client);
            rc = client;
            /* FIXME: check other BS_* handlers */
            if (btn_type == BS_GROUPBOX)
                InflateRect(&rc, -7, 1); /* GB_Paint does this */
            BUTTON_CalcLabelRect(hWnd, hdc, &rc);
            /* Clip by client rect bounds */
            if (rc.right > client.right) rc.right = client.right;
            if (rc.bottom > client.bottom) rc.bottom = client.bottom;
            FillRect(hdc, &rc, hbrush);
            ReleaseDC(hWnd, hdc);
        }

        if (unicode) DefWindowProcW( hWnd, WM_SETTEXT, wParam, lParam );
        else DefWindowProcA( hWnd, WM_SETTEXT, wParam, lParam );
        if (btn_type == BS_GROUPBOX) /* Yes, only for BS_GROUPBOX */
            InvalidateRect( hWnd, NULL, TRUE );
        else
            paint_button( hWnd, btn_type, ODA_DRAWENTIRE );
        return 1; /* success. FIXME: check text length */
    }

    case WM_SETFONT:
        set_button_font( hWnd, (HFONT)wParam );
        if (lParam) InvalidateRect(hWnd, NULL, TRUE);
        break;

    case WM_GETFONT:
        return (LRESULT)get_button_font( hWnd );

    case WM_SETFOCUS:
        TRACE("WM_SETFOCUS %p\n",hWnd);
        set_button_state( hWnd, get_button_state(hWnd) | BST_FOCUS );
#ifndef _USER32_
        if (btn_type != BS_OWNERDRAW)
            InvalidateRect(hWnd, NULL, FALSE);
        else
#endif
        paint_button( hWnd, btn_type, ODA_FOCUS );
        if (style & BS_NOTIFY)
            BUTTON_NOTIFY_PARENT(hWnd, BN_SETFOCUS);
        break;

    case WM_KILLFOCUS:
        TRACE("WM_KILLFOCUS %p\n",hWnd);
        state = get_button_state( hWnd );
        set_button_state( hWnd, state & ~BST_FOCUS );
#ifdef _USER32_
	paint_button( hWnd, btn_type, ODA_FOCUS );
#endif

        if ((state & BUTTON_BTNPRESSED) && GetCapture() == hWnd)
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
#ifdef __REACTOS__
        NtUserAlterWindowStyle(hWnd, GWL_STYLE, style);
#else
        WIN_SetStyle( hWnd, style, BS_TYPEMASK & ~style );
#endif

        /* Only redraw if lParam flag is set.*/
        if (lParam)
            InvalidateRect( hWnd, NULL, TRUE );

        break;

    case BM_CLICK:
#ifdef __REACTOS__
        state = get_button_state(hWnd);
        if (state & BUTTON_BMCLICK)
           break;
        set_button_state(hWnd, state | BUTTON_BMCLICK); // Tracked in STATE_GWL_OFFSET.
#endif
	SendMessageW( hWnd, WM_LBUTTONDOWN, 0, 0 );
	SendMessageW( hWnd, WM_LBUTTONUP, 0, 0 );
#ifdef __REACTOS__
        state = get_button_state(hWnd);
        if (!(state & BUTTON_BMCLICK)) break;
        state &= ~BUTTON_BMCLICK;
        set_button_state(hWnd, state);
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
#ifdef _USER32_
        oldHbitmap = (HBITMAP)SetWindowLongPtrW( hWnd, HIMAGE_GWL_OFFSET, lParam );
#else
        oldHbitmap = (HBITMAP)set_button_image(hWnd, lParam );
#endif
	InvalidateRect( hWnd, NULL, FALSE );
	return (LRESULT)oldHbitmap;

    case BM_GETIMAGE:
#ifdef _USER32_
        return GetWindowLongPtrW( hWnd, HIMAGE_GWL_OFFSET );
#else
        return get_button_image(hWnd);
#endif

    case BM_GETCHECK:
        return get_button_state( hWnd ) & 3;

    case BM_SETCHECK:
        if (wParam > maxCheckState[btn_type]) wParam = maxCheckState[btn_type];
        state = get_button_state( hWnd );
        if ((btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON))
        {
#ifdef __REACTOS__
            if (wParam) style |= WS_TABSTOP;
            else style &= ~WS_TABSTOP;
            NtUserAlterWindowStyle(hWnd, GWL_STYLE, style);
#else
            if (wParam) WIN_SetStyle( hWnd, WS_TABSTOP, 0 );
            else WIN_SetStyle( hWnd, 0, WS_TABSTOP );
#endif
        }
        if ((state & 3) != wParam)
        {
            set_button_state( hWnd, (state & ~3) | wParam );
#ifdef _USER32
            paint_button( hWnd, btn_type, ODA_SELECT );
#else
            InvalidateRect(hWnd, NULL, FALSE);
#endif
        }
        if ((btn_type == BS_AUTORADIOBUTTON) && (wParam == BST_CHECKED) && (style & WS_CHILD))
            BUTTON_CheckAutoRadioButton( hWnd );
        break;

    case BM_GETSTATE:
        return get_button_state( hWnd );

    case BM_SETSTATE:
        state = get_button_state( hWnd );
        if (wParam)
            set_button_state( hWnd, state | BST_PUSHED );
        else
            set_button_state( hWnd, state & ~BST_PUSHED );

#ifdef _USER32_
        paint_button( hWnd, btn_type, ODA_SELECT );
#else
        InvalidateRect(hWnd, NULL, FALSE);
#endif
        break;

#ifdef __REACTOS__
    case WM_UPDATEUISTATE:
        if (unicode)
            DefWindowProcW(hWnd, uMsg, wParam, lParam);
        else
            DefWindowProcA(hWnd, uMsg, wParam, lParam);

        if (button_update_uistate(hWnd, unicode))
            paint_button( hWnd, btn_type, ODA_DRAWENTIRE );
        break;
#endif

    case WM_NCHITTEST:
        if(btn_type == BS_GROUPBOX) return HTTRANSPARENT;
        /* fall through */
    default:
        return unicode ? DefWindowProcW(hWnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

#ifdef __REACTOS__

/***********************************************************************
 *           ButtonWndProcW
 * The button window procedure. This is just a wrapper which locks
 * the passed HWND and calls the real window procedure (with a WND*
 * pointer pointing to the locked windowstructure).
 */
LRESULT WINAPI ButtonWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!IsWindow(hWnd)) return 0;
    return ButtonWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}

/***********************************************************************
 *           ButtonWndProcA
 */
LRESULT WINAPI ButtonWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!IsWindow(hWnd)) return 0;
    return ButtonWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

#endif /* __REACTOS__ */

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
static UINT BUTTON_CalcLabelRect(HWND hwnd, HDC hdc, RECT *rc)
{
   LONG style = GetWindowLongPtrW( hwnd, GWL_STYLE );
   LONG ex_style = GetWindowLongPtrW( hwnd, GWL_EXSTYLE );
   WCHAR *text;
   ICONINFO    iconInfo;
   BITMAP      bm;
   UINT        dtStyle = BUTTON_BStoDT( style, ex_style );
   RECT        r = *rc;
   INT         n;

   /* Calculate label rectangle according to label type */
   switch (style & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
      {
          HFONT hFont, hPrevFont = 0;

          if (!(text = get_button_text( hwnd ))) goto empty_rect;
          if (!text[0])
          {
              HeapFree( GetProcessHeap(), 0, text );
              goto empty_rect;
          }

          if ((hFont = get_button_font( hwnd ))) hPrevFont = SelectObject( hdc, hFont );
          DrawTextW(hdc, text, -1, &r, dtStyle | DT_CALCRECT);
          if (hPrevFont) SelectObject( hdc, hPrevFont );
          HeapFree( GetProcessHeap(), 0, text );
#ifdef __REACTOS__
          if (get_ui_state(hwnd) & UISF_HIDEACCEL)
              dtStyle |= DT_HIDEPREFIX;
#endif
          break;
      }

      case BS_ICON:
#ifdef _USER32_
         if (!GetIconInfo((HICON)GetWindowLongPtrW( hwnd, HIMAGE_GWL_OFFSET ), &iconInfo))
#else
         if (!GetIconInfo((HICON)get_button_image(hwnd), &iconInfo))
#endif
            goto empty_rect;

         GetObjectW (iconInfo.hbmColor, sizeof(BITMAP), &bm);

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;

         DeleteObject(iconInfo.hbmColor);
         DeleteObject(iconInfo.hbmMask);
         break;

      case BS_BITMAP:
#ifdef _USER32_
         if (!GetObjectW( (HANDLE)GetWindowLongPtrW( hwnd, HIMAGE_GWL_OFFSET ), sizeof(BITMAP), &bm))
#else
         if (!GetObjectW( (HANDLE)get_button_image(hwnd), sizeof(BITMAP), &bm))
#endif
            goto empty_rect;

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;
         break;

      default:
      empty_rect:
         rc->right = r.left;
         rc->bottom = r.top;
         return (UINT)-1;
   }

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
                       r.top    = rc->top + ((rc->bottom - rc->top) - n) / 2;
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
#ifdef _USER32_
   RECT rc;

   SetRect(&rc, 0, 0, cx, cy);
   DrawTextW(hdc, (LPCWSTR)lp, -1, &rc, (UINT)wp);
   return TRUE;
#else
    HWND hwnd = (HWND)lp;
    RECT rc;
    PBUTTON_DATA pdata = _GetButtonData(hwnd);
    SIZE ImageSize;
    WCHAR *text = NULL;

    if (!(text = get_button_text( hwnd ))) return TRUE;

    SetRect(&rc, 0, 0, cx, cy);

    if (pdata->imlData.himl && ImageList_GetIconSize(pdata->imlData.himl, &ImageSize.cx, &ImageSize.cy))
    {
        int left = pdata->imlData.margin.left;
        int top = (cy - ImageSize.cy) / 2;
        rc.left += pdata->imlData.margin.left + pdata->imlData.margin.right + ImageSize.cy;
        ImageList_Draw(pdata->imlData.himl, 0, hdc, left, top, 0);
    }

    DrawTextW(hdc, text, -1, &rc, (UINT)wp);
    return TRUE;
#endif
}


/**********************************************************************
 *       BUTTON_DrawLabel
 *
 *   Common function for drawing button label.
 */
static void BUTTON_DrawLabel(HWND hwnd, HDC hdc, UINT dtFlags, const RECT *rc)
{
   DRAWSTATEPROC lpOutputProc = NULL;
   LPARAM lp;
   WPARAM wp = 0;
   HBRUSH hbr = 0;
   UINT flags = IsWindowEnabled(hwnd) ? DSS_NORMAL : DSS_DISABLED;
   LONG state = get_button_state( hwnd );
   LONG style = GetWindowLongPtrW( hwnd, GWL_STYLE );
   WCHAR *text = NULL;

   /* FIXME: To draw disabled label in Win31 look-and-feel, we probably
    * must use DSS_MONO flag and COLOR_GRAYTEXT brush (or maybe DSS_UNION).
    * I don't have Win31 on hand to verify that, so I leave it as is.
    */

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
#ifdef _USER32_
         if (!(text = get_button_text( hwnd ))) return;
         lp = (LPARAM)text;
#else
         lp = (LPARAM)hwnd;
#endif

         wp = (WPARAM)dtFlags;

#ifdef __REACTOS__
         if (dtFlags & DT_HIDEPREFIX)
             flags |= DSS_HIDEPREFIX;
#endif
         break;

      case BS_ICON:
         flags |= DST_ICON;
#ifdef _USER32_
         lp = GetWindowLongPtrW( hwnd, HIMAGE_GWL_OFFSET );
#else
         lp = get_button_image(hwnd);
#endif
         break;

      case BS_BITMAP:
         flags |= DST_BITMAP;
#ifdef _USER32_
         lp = GetWindowLongPtrW( hwnd, HIMAGE_GWL_OFFSET );
#else
         lp = get_button_image(hwnd);
#endif
         break;

      default:
         return;
   }

   DrawStateW(hdc, hbr, lpOutputProc, lp, wp, rc->left, rc->top,
              rc->right - rc->left, rc->bottom - rc->top, flags);
   HeapFree( GetProcessHeap(), 0, text );
}

/**********************************************************************
 *       Push Button Functions
 */
static void PB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    RECT     rc, r;
    UINT     dtFlags, uState;
    HPEN     hOldPen;
    HBRUSH   hOldBrush;
    INT      oldBkMode;
    COLORREF oldTxtColor;
    HFONT hFont;
    LONG state = get_button_state( hwnd );
    LONG style = GetWindowLongPtrW( hwnd, GWL_STYLE );
    BOOL pushedState = (state & BST_PUSHED);
    HWND parent;
    HRGN hrgn;

    GetClientRect( hwnd, &rc );

    /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );
    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
#if defined(__REACTOS__) && defined(_USER32_)
    GetControlColor( parent, hwnd, hDC, WM_CTLCOLORBTN);
#else
    SendMessageW( parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)hwnd );
#endif

    hrgn = set_control_clipping( hDC, &rc );
#ifdef __REACTOS__
    hOldPen = SelectObject(hDC, GetStockObject(DC_PEN));
    SetDCPenColor(hDC, GetSysColor(COLOR_WINDOWFRAME));
#else
    hOldPen = SelectObject(hDC, SYSCOLOR_GetPen(COLOR_WINDOWFRAME));
#endif
    hOldBrush = SelectObject(hDC,GetSysColorBrush(COLOR_BTNFACE));
    oldBkMode = SetBkMode(hDC, TRANSPARENT);

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

    /* draw button label */
    r = rc;
    dtFlags = BUTTON_CalcLabelRect(hwnd, hDC, &r);

    if (dtFlags == (UINT)-1L)
       goto cleanup;

    if (pushedState)
       OffsetRect(&r, 1, 1);

    oldTxtColor = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );

    BUTTON_DrawLabel(hwnd, hDC, dtFlags, &r);

    SetTextColor( hDC, oldTxtColor );

draw_focus:
    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
#ifdef __REACTOS__
        if (!(get_ui_state(hwnd) & UISF_HIDEFOCUS))
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
}

/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    RECT rbox, rtext, client;
    HBRUSH hBrush;
    int delta, text_offset, checkBoxWidth, checkBoxHeight;
    UINT dtFlags;
    HFONT hFont;
    LONG state = get_button_state( hwnd );
    LONG style = GetWindowLongPtrW( hwnd, GWL_STYLE );
    LONG ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );
    HWND parent;
    HRGN hrgn;

    if (style & BS_PUSHLIKE)
    {
        PB_Paint( hwnd, hDC, action );
	return;
    }

    GetClientRect(hwnd, &client);
    rbox = rtext = client;

    checkBoxWidth  = 12 * GetDeviceCaps( hDC, LOGPIXELSX ) / 96 + 1;
    checkBoxHeight = 12 * GetDeviceCaps( hDC, LOGPIXELSY ) / 96 + 1;

    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );
    GetCharWidthW( hDC, '0', '0', &text_offset );
    text_offset /= 2;

    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
#if defined(__REACTOS__) && defined(_USER32_)
    hBrush = GetControlColor(parent, hwnd, hDC, WM_CTLCOLORSTATIC);
#else
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC,
				  (WPARAM)hDC, (LPARAM)hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC,
					(WPARAM)hDC, (LPARAM)hwnd );
#endif
    hrgn = set_control_clipping( hDC, &client );

    if (style & BS_LEFTTEXT || ex_style & WS_EX_RIGHT)
    {
	/* magic +4 is what CTL3D expects */

        rtext.right -= checkBoxWidth + text_offset;;
        rbox.left = rbox.right - checkBoxWidth;
    }
    else
    {
        rtext.left += checkBoxWidth + text_offset;;
        rbox.right = checkBoxWidth;
    }

    /* Since WM_ERASEBKGND does nothing, first prepare background */
    if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
    if (action == ODA_DRAWENTIRE) FillRect( hDC, &client, hBrush );

    /* Draw label */
    client = rtext;
    dtFlags = BUTTON_CalcLabelRect(hwnd, hDC, &rtext);
    
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
	BUTTON_DrawLabel(hwnd, hDC, dtFlags, &rtext);

    /* ... and focus */
    if (action == ODA_FOCUS || (state & BST_FOCUS))
    {
#ifdef __REACTOS__
        if (!(get_ui_state(hwnd) & UISF_HIDEFOCUS))
        {
#endif
            rtext.left--;
            rtext.right++;
            IntersectRect(&rtext, &rtext, &client);
            DrawFocusRect( hDC, &rtext );
#ifdef __REACTOS__
        }
#endif
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
    start = sibling = GetNextDlgGroupItem( parent, hwnd, TRUE );
    do
    {
        if (!sibling) break;
        if ((hwnd != sibling) &&
            ((GetWindowLongPtrW( sibling, GWL_STYLE) & BS_TYPEMASK) == BS_AUTORADIOBUTTON))
            SendMessageW( sibling, BM_SETCHECK, BST_UNCHECKED, 0 );
        sibling = GetNextDlgGroupItem( parent, sibling, FALSE );
    } while (sibling != start);
}


/**********************************************************************
 *       Group Box Functions
 */

static void GB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    RECT rc, rcFrame;
    HBRUSH hbr;
    HFONT hFont;
    UINT dtFlags;
    TEXTMETRICW tm;
    LONG style = GetWindowLongPtrW( hwnd, GWL_STYLE );
    HWND parent;
    HRGN hrgn;

    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );
    /* GroupBox acts like static control, so it sends CTLCOLORSTATIC */
    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
#if defined(__REACTOS__) && defined(_USER32_)
    hbr = GetControlColor(parent, hwnd, hDC, WM_CTLCOLORSTATIC);
#else
    hbr = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)hwnd);
    if (!hbr) /* did the app forget to call defwindowproc ? */
        hbr = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC,
				     (WPARAM)hDC, (LPARAM)hwnd);
#endif
    GetClientRect( hwnd, &rc);
    rcFrame = rc;
    hrgn = set_control_clipping( hDC, &rc );

    GetTextMetricsW (hDC, &tm);
    rcFrame.top += (tm.tmHeight / 2) - 1;
    DrawEdge (hDC, &rcFrame, EDGE_ETCHED, BF_RECT | ((style & BS_FLAT) ? BF_FLAT : 0));

    InflateRect(&rc, -7, 1);
    dtFlags = BUTTON_CalcLabelRect(hwnd, hDC, &rc);

    if (dtFlags != (UINT)-1L)
    {
        /* Because buttons have CS_PARENTDC class style, there is a chance
         * that label will be drawn out of client rect.
         * But Windows doesn't clip label's rect, so do I.
         */

        /* There is 1-pixel margin at the left, right, and bottom */
        rc.left--; rc.right++; rc.bottom++;
        FillRect(hDC, &rc, hbr);
        rc.left++; rc.right--; rc.bottom--;

        BUTTON_DrawLabel(hwnd, hDC, dtFlags, &rc);
    }
    SelectClipRgn( hDC, hrgn );
    if (hrgn) DeleteObject( hrgn );
}


/**********************************************************************
 *       User Button Functions
 */

static void UB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    RECT rc;
    HBRUSH hBrush;
    HFONT hFont;
    LONG state = get_button_state( hwnd );
    HWND parent;

    GetClientRect( hwnd, &rc);

    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );

    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
#if defined(__REACTOS__) && defined(_USER32_)
    hBrush = GetControlColor( parent, hwnd, hDC, WM_CTLCOLORBTN);
#else
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORBTN,
					(WPARAM)hDC, (LPARAM)hwnd);
#endif

    FillRect( hDC, &rc, hBrush );
    if (action == ODA_FOCUS || (state & BST_FOCUS))
#ifdef __REACTOS__
    {
        if (!(get_ui_state(hwnd) & UISF_HIDEFOCUS))
#endif
            DrawFocusRect( hDC, &rc );
#ifdef __REACTOS__
    }
#endif

    switch (action)
    {
    case ODA_FOCUS:
        BUTTON_NOTIFY_PARENT( hwnd, (state & BST_FOCUS) ? BN_SETFOCUS : BN_KILLFOCUS );
        break;

    case ODA_SELECT:
        BUTTON_NOTIFY_PARENT( hwnd, (state & BST_PUSHED) ? BN_HILITE : BN_UNHILITE );
        break;

    default:
        BUTTON_NOTIFY_PARENT( hwnd, BN_PAINT );
        break;
    }
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    LONG state = get_button_state( hwnd );
    DRAWITEMSTRUCT dis;
    LONG_PTR id = GetWindowLongPtrW( hwnd, GWLP_ID );
    HWND parent;
    HFONT hFont, hPrevFont = 0;
    HRGN hrgn;

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = id;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = ((state & BST_FOCUS) ? ODS_FOCUS : 0) |
                     ((state & BST_PUSHED) ? ODS_SELECTED : 0) |
                     (IsWindowEnabled(hwnd) ? 0: ODS_DISABLED);
    dis.hwndItem   = hwnd;
    dis.hDC        = hDC;
    dis.itemData   = 0;
    GetClientRect( hwnd, &dis.rcItem );

    if ((hFont = get_button_font( hwnd ))) hPrevFont = SelectObject( hDC, hFont );
    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
#if defined(__REACTOS__) && defined(_USER32_)
    GetControlColor( parent, hwnd, hDC, WM_CTLCOLORBTN);
#else
    SendMessageW( parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)hwnd );
#endif

    hrgn = set_control_clipping( hDC, &dis.rcItem );

    SendMessageW( GetParent(hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
    if (hPrevFont) SelectObject(hDC, hPrevFont);
    SelectClipRgn( hDC, hrgn );
    if (hrgn) DeleteObject( hrgn );
}

#ifndef _USER32_
void BUTTON_Register()
{
    WNDCLASSW wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC;
    wndClass.lpfnWndProc   = ButtonWndProcW;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PBUTTON_DATA);
    wndClass.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = WC_BUTTONW;

    RegisterClassW(&wndClass);
}

void BUTTON_Unregister()
{
    UnregisterClassW(WC_BUTTONW, NULL);
}
#endif