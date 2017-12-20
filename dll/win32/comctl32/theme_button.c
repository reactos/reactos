/*
 * Theming - Button control
 *
 * Copyright (c) 2008 by Reece H. Dunn
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
 */

#include "comctl32.h"
WINE_DEFAULT_DEBUG_CHANNEL(theme_button);

#define BUTTON_TYPE 0x0f /* bit mask for the available button types */

/* These are indices into a states array to determine the theme state for a given theme part. */
typedef enum
{
	STATE_NORMAL,
	STATE_HOT,
	STATE_PRESSED,
	STATE_DISABLED,
	STATE_DEFAULTED
} ButtonState;

#ifdef __REACTOS__ /* r73885 */
typedef void (*pfThemedPaint)(HTHEME theme, HWND hwnd, HDC hdc, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag);
#else
typedef void (*pfThemedPaint)(HTHEME theme, HWND hwnd, HDC hdc, ButtonState drawState, UINT dtFlags, BOOL focused);
#endif

#ifdef __REACTOS__ /* r73885 & r73907 */
static inline LONG get_button_state( HWND hwnd )
{
    return _GetButtonData(hwnd)->state;
}

static inline HFONT get_button_font( HWND hwnd )
{
    return (HFONT)_GetButtonData(hwnd)->font;
}

static inline LONG_PTR get_button_image(HWND hwnd)
{
    return _GetButtonData(hwnd)->image;
}

BOOL BUTTON_DrawIml(HDC hdc, BUTTON_IMAGELIST *pimlData, RECT *prc, BOOL bOnlyCalc, int index);
DWORD BUTTON_SendCustomDraw(HWND hwnd, HDC hDC, DWORD dwDrawStage, RECT* prc);
#endif

static UINT get_drawtext_flags(DWORD style, DWORD ex_style)
{
    UINT flags = 0;

    if (style & BS_PUSHLIKE)
        style &= ~BUTTON_TYPE;

    if (!(style & BS_MULTILINE))
        flags |= DT_SINGLELINE;
    else
        flags |= DT_WORDBREAK;

    switch (style & BS_CENTER)
    {
    case BS_LEFT:   flags |= DT_LEFT;   break;
    case BS_RIGHT:  flags |= DT_RIGHT;  break;
    case BS_CENTER: flags |= DT_CENTER; break;
    default:
        flags |= ((style & BUTTON_TYPE) <= BS_DEFPUSHBUTTON)
               ? DT_CENTER : DT_LEFT;
    }

    if (ex_style & WS_EX_RIGHT)
        flags = DT_RIGHT | (flags & ~(DT_LEFT | DT_CENTER));

    if ((style & BUTTON_TYPE) != BS_GROUPBOX)
    {
        switch (style & BS_VCENTER)
        {
        case BS_TOP:     flags |= DT_TOP;     break;
        case BS_BOTTOM:  flags |= DT_BOTTOM;  break;
        case BS_VCENTER: /* fall through */
        default:         flags |= DT_VCENTER; break;
        }
    }
    else
        /* GroupBox's text is always single line and is top aligned. */
        flags |= DT_SINGLELINE | DT_TOP;

    return flags;
}

static inline WCHAR *get_button_text(HWND hwnd)
{
    INT len = 512;
    WCHAR *text;
    text = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (text) InternalGetWindowText(hwnd, text, len + 1);
    return text;
}

static void PB_draw(HTHEME theme, HWND hwnd, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag)
{
    static const int states[] = { PBS_NORMAL, PBS_HOT, PBS_PRESSED, PBS_DISABLED, PBS_DEFAULTED };

    RECT bgRect, textRect;
    HFONT font = get_button_font(hwnd);
    HFONT hPrevFont = font ? SelectObject(hDC, font) : NULL;
    int state = states[ drawState ];
    WCHAR *text;
    PBUTTON_DATA pdata = _GetButtonData(hwnd);
    HWND parent;
    DWORD cdrf;

    GetClientRect(hwnd, &bgRect);
    GetThemeBackgroundContentRect(theme, hDC, BP_PUSHBUTTON, state, &bgRect, &textRect);

    if (prfFlag == 0)
    {
        if (IsThemeBackgroundPartiallyTransparent(theme, BP_PUSHBUTTON, state))
            DrawThemeParentBackground(hwnd, hDC, NULL);
    }

    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
    SendMessageW( parent, WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)hwnd );

    cdrf = BUTTON_SendCustomDraw(hwnd, hDC, CDDS_PREERASE, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

    DrawThemeBackground(theme, hDC, BP_PUSHBUTTON, state, &bgRect, NULL);

    if (cdrf == CDRF_NOTIFYPOSTERASE)
        BUTTON_SendCustomDraw(hwnd, hDC, CDDS_POSTERASE, &bgRect);

    cdrf = BUTTON_SendCustomDraw(hwnd, hDC, CDDS_PREPAINT, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

    BUTTON_DrawIml(hDC, &pdata->imlData, &textRect, FALSE, drawState);

    text = get_button_text(hwnd);
    if (text)
    {
        DrawThemeText(theme, hDC, BP_PUSHBUTTON, state, text, lstrlenW(text), dtFlags, 0, &textRect);
        HeapFree(GetProcessHeap(), 0, text);
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

    if (cdrf == CDRF_NOTIFYPOSTPAINT)
        BUTTON_SendCustomDraw(hwnd, hDC, CDDS_POSTPAINT, &bgRect);

cleanup:
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

static void CB_draw(HTHEME theme, HWND hwnd, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag)
{
    static const int cb_states[3][5] =
    {
        { CBS_UNCHECKEDNORMAL, CBS_UNCHECKEDHOT, CBS_UNCHECKEDPRESSED, CBS_UNCHECKEDDISABLED, CBS_UNCHECKEDNORMAL },
        { CBS_CHECKEDNORMAL, CBS_CHECKEDHOT, CBS_CHECKEDPRESSED, CBS_CHECKEDDISABLED, CBS_CHECKEDNORMAL },
        { CBS_MIXEDNORMAL, CBS_MIXEDHOT, CBS_MIXEDPRESSED, CBS_MIXEDDISABLED, CBS_MIXEDNORMAL }
    };

    static const int rb_states[2][5] =
    {
        { RBS_UNCHECKEDNORMAL, RBS_UNCHECKEDHOT, RBS_UNCHECKEDPRESSED, RBS_UNCHECKEDDISABLED, RBS_UNCHECKEDNORMAL },
        { RBS_CHECKEDNORMAL, RBS_CHECKEDHOT, RBS_CHECKEDPRESSED, RBS_CHECKEDDISABLED, RBS_CHECKEDNORMAL }
    };

    SIZE sz;
    RECT bgRect, textRect;
    HFONT font, hPrevFont = NULL;
    LRESULT checkState = get_button_state(hwnd) & 3;
    DWORD dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    int part = ((dwStyle & BUTTON_TYPE) == BS_RADIOBUTTON) || ((dwStyle & BUTTON_TYPE) == BS_AUTORADIOBUTTON)
             ? BP_RADIOBUTTON
             : BP_CHECKBOX;
    int state = (part == BP_CHECKBOX)
              ? cb_states[ checkState ][ drawState ]
              : rb_states[ checkState ][ drawState ];
    WCHAR *text;
    LOGFONTW lf;
    BOOL created_font = FALSE;
    HWND parent;
    HBRUSH hBrush;
    DWORD cdrf;

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
        font = get_button_font(hwnd);
        hPrevFont = SelectObject(hDC, font);
    }

    if (FAILED(GetThemePartSize(theme, hDC, part, state, NULL, TS_DRAW, &sz)))
        sz.cx = sz.cy = 13;

    GetClientRect(hwnd, &bgRect);

    if (prfFlag == 0)
    {
        DrawThemeParentBackground(hwnd, hDC, NULL);
    }

    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC,
                                 (WPARAM)hDC, (LPARAM)hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC,
                                        (WPARAM)hDC, (LPARAM)hwnd );
    FillRect( hDC, &bgRect, hBrush );

    cdrf = BUTTON_SendCustomDraw(hwnd, hDC, CDDS_PREERASE, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

    GetThemeBackgroundContentRect(theme, hDC, part, state, &bgRect, &textRect);

    if (dtFlags & DT_SINGLELINE) /* Center the checkbox / radio button to the text. */
        bgRect.top = bgRect.top + (textRect.bottom - textRect.top - sz.cy) / 2;

    /* adjust for the check/radio marker */
    bgRect.bottom = bgRect.top + sz.cy;
    bgRect.right = bgRect.left + sz.cx;
    textRect.left = bgRect.right + 6;

    DrawThemeBackground(theme, hDC, part, state, &bgRect, NULL);

    if (cdrf == CDRF_NOTIFYPOSTERASE)
        BUTTON_SendCustomDraw(hwnd, hDC, CDDS_POSTERASE, &bgRect);

    cdrf = BUTTON_SendCustomDraw(hwnd, hDC, CDDS_PREPAINT, &bgRect);
    if (cdrf == CDRF_SKIPDEFAULT)
        goto cleanup;

    text = get_button_text(hwnd);
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

        HeapFree(GetProcessHeap(), 0, text);
    }

    if (cdrf == CDRF_NOTIFYPOSTPAINT)
        BUTTON_SendCustomDraw(hwnd, hDC, CDDS_POSTPAINT, &bgRect);

cleanup:
    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

#ifdef __REACTOS__ /* r73885 */
static void GB_draw(HTHEME theme, HWND hwnd, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused, LPARAM prfFlag)
#else
static void GB_draw(HTHEME theme, HWND hwnd, HDC hDC, ButtonState drawState, UINT dtFlags, BOOL focused)
#endif
{
    static const int states[] = { GBS_NORMAL, GBS_NORMAL, GBS_NORMAL, GBS_DISABLED, GBS_NORMAL };

    RECT bgRect, textRect, contentRect;
    int state = states[ drawState ];
    WCHAR *text = get_button_text(hwnd);
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
        font = get_button_font(hwnd);
#else
        font = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
#endif
        hPrevFont = SelectObject(hDC, font);
    }

    GetClientRect(hwnd, &bgRect);
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
    {
        if (IsThemeBackgroundPartiallyTransparent(theme, BP_GROUPBOX, state))
            DrawThemeParentBackground(hwnd, hDC, NULL);
    }
#else
    if (IsThemeBackgroundPartiallyTransparent(theme, BP_GROUPBOX, state))
        DrawThemeParentBackground(hwnd, hDC, NULL);
#endif

#ifdef __REACTOS__ /* r74406 */
    parent = GetParent(hwnd);
    if (!parent) parent = hwnd;
    hBrush = (HBRUSH)SendMessageW(parent, WM_CTLCOLORSTATIC,
                                  (WPARAM)hDC, (LPARAM)hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(parent, WM_CTLCOLORSTATIC,
                                       (WPARAM)hDC, (LPARAM)hwnd );
    GetClientRect(hwnd, &clientRect);
    FillRect( hDC, &clientRect, hBrush );
#endif

    DrawThemeBackground(theme, hDC, BP_GROUPBOX, state, &bgRect, NULL);

    SelectClipRgn(hDC, NULL);

    if (text)
    {
        InflateRect(&textRect, -2, 0);
        DrawThemeText(theme, hDC, BP_GROUPBOX, state, text, lstrlenW(text), 0, 0, &textRect);
        HeapFree(GetProcessHeap(), 0, text);
    }

    if (created_font) DeleteObject(font);
    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

static const pfThemedPaint btnThemedPaintFunc[BUTTON_TYPE + 1] =
{
    PB_draw, /* BS_PUSHBUTTON */
    PB_draw, /* BS_DEFPUSHBUTTON */
    CB_draw, /* BS_CHECKBOX */
    CB_draw, /* BS_AUTOCHECKBOX */
    CB_draw, /* BS_RADIOBUTTON */
    CB_draw, /* BS_3STATE */
    CB_draw, /* BS_AUTO3STATE */
    GB_draw, /* BS_GROUPBOX */
    NULL, /* BS_USERBUTTON */
    CB_draw, /* BS_AUTORADIOBUTTON */
    NULL, /* Not defined */
    NULL, /* BS_OWNERDRAW */
    NULL, /* Not defined */
    NULL, /* Not defined */
    NULL, /* Not defined */
    NULL, /* Not defined */
};

BOOL BUTTON_PaintWithTheme(HTHEME theme, HWND hwnd, HDC hParamDC, LPARAM prfFlag)
{
    DWORD dwStyle;
    DWORD dwStyleEx;
    DWORD type;
    UINT dtFlags;
    int state;
    ButtonState drawState;
    pfThemedPaint paint;

    /* Don't draw with themes on a button with BS_ICON or BS_BITMAP */
    if (get_button_image(hwnd) != 0)
        return FALSE;

    dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    type = dwStyle & BUTTON_TYPE;

    if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON && (dwStyle & BS_PUSHLIKE))
        type = BS_PUSHBUTTON;

    paint = btnThemedPaintFunc[type];
    if (!paint)
        return FALSE;

    dwStyleEx = GetWindowLongW(hwnd, GWL_EXSTYLE);
    dtFlags = get_drawtext_flags(dwStyle, dwStyleEx);
    state = get_button_state(hwnd);

    if(dwStyle & WS_DISABLED)
        drawState = STATE_DISABLED;
    else if(state & BST_PUSHED) 
        drawState = STATE_PRESSED;
    else if ((dwStyle & BS_PUSHLIKE) && (state & (BST_CHECKED|BST_INDETERMINATE))) 
        drawState = STATE_PRESSED;
    else if(state & BST_HOT) 
        drawState = STATE_HOT;
    else if((state & BST_FOCUS) || (dwStyle & BS_DEFPUSHBUTTON))
        drawState = STATE_DEFAULTED;
    else
        drawState = STATE_NORMAL;

    if (paint == PB_draw || paint == CB_draw)
    {
        HDC hdc;
        HBITMAP hbmp;
        RECT rc;

        GetClientRect(hwnd, &rc);
        hdc = CreateCompatibleDC(hParamDC);
        hbmp = CreateCompatibleBitmap(hParamDC, rc.right, rc.bottom);
        if (hdc && hbmp)
        {
            SelectObject(hdc, hbmp);

            paint(theme, hwnd, hdc, drawState, dtFlags, state & BST_FOCUS, prfFlag);

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

    paint(theme, hwnd, hParamDC, drawState, dtFlags, state & BST_FOCUS, prfFlag);
    return TRUE;
}
