/* $Id: button.c,v 1.5 2003/07/11 17:08:44 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Button control
 * FILE:             lib/user32/controls/static.c
 * PROGRAMER:        Richard Campbell lib/user32/controls/button.c
 * REVISION HISTORY: 2003/05/28 Created
 * NOTES:            Adapted from Wine
 */

#include "windows.h"
#include "user32/regcontrol.h"
#include "user32.h"

#define STATE_GWL_OFFSET  0
#define HFONT_GWL_OFFSET  (sizeof(LONG))
#define HIMAGE_GWL_OFFSET (2*sizeof(LONG))
#define NB_EXTRA_BYTES    (3*sizeof(LONG))

#define BS_FLAT              0x00008000L

#define BUTTON_UNCHECKED       0x00
#define BUTTON_CHECKED         0x01
#define BUTTON_3STATE          0x02
#define BUTTON_HIGHLIGHTED     0x04
#define BUTTON_HASFOCUS        0x08
#define BUTTON_NSTATES         0x0F

#define BUTTON_BTNPRESSED      0x40
#define BUTTON_UNKNOWN2        0x20
#define BUTTON_UNKNOWN3        0x10

static UINT BUTTON_CalcLabelRect( HWND hwnd, HDC hdc, RECT *rc );
static void PB_Paint( HWND hwnd, HDC hDC, UINT action );
static void CB_Paint( HWND hwnd, HDC hDC, UINT action );
static void GB_Paint( HWND hwnd, HDC hDC, UINT action );
static void UB_Paint( HWND hwnd, HDC hDC, UINT action );
static void OB_Paint( HWND hwnd, HDC hDC, UINT action );
static void BUTTON_CheckAutoRadioButton( HWND hwnd );
static LRESULT WINAPI ButtonWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ButtonWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

#define MAX_BTN_TYPE  12

static const WORD maxCheckState[MAX_BTN_TYPE] =
{
    BUTTON_UNCHECKED,   /* BS_PUSHBUTTON */
    BUTTON_UNCHECKED,   /* BS_DEFPUSHBUTTON */
    BUTTON_CHECKED,     /* BS_CHECKBOX */
    BUTTON_CHECKED,     /* BS_AUTOCHECKBOX */
    BUTTON_CHECKED,     /* BS_RADIOBUTTON */
    BUTTON_3STATE,      /* BS_3STATE */
    BUTTON_3STATE,      /* BS_AUTO3STATE */
    BUTTON_UNCHECKED,   /* BS_GROUPBOX */
    BUTTON_UNCHECKED,   /* BS_USERBUTTON */
    BUTTON_CHECKED,     /* BS_AUTORADIOBUTTON */
    BUTTON_UNCHECKED,   /* Not defined */
    BUTTON_UNCHECKED    /* BS_OWNERDRAW */
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
    NULL,        /* Not defined */
    OB_Paint     /* BS_OWNERDRAW */
};

static HBITMAP hbitmapCheckBoxes = 0;
static WORD checkBoxWidth = 0, checkBoxHeight = 0;


/*********************************************************************
 * button class descriptor
 */
const struct builtin_class_descr BUTTON_builtin_class =
{
    "Button",            /* name */
    CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC, /* style  */
    (WNDPROC) ButtonWndProcA,      /* procA */
    (WNDPROC) ButtonWndProcW,      /* procW */
    NB_EXTRA_BYTES,      /* extra */
    (LPCSTR) IDC_ARROW,          /* cursor */
    0                    /* brush */
};

HPEN STDCALL GetSysColorPen (int nIndex);

inline static LONG get_button_state( HWND hwnd )
{
    return GetWindowLongA( hwnd, STATE_GWL_OFFSET );
}

inline static void set_button_state( HWND hwnd, LONG state )
{
    SetWindowLongA( hwnd, STATE_GWL_OFFSET, state );
}

inline static HFONT get_button_font( HWND hwnd )
{
    return (HFONT)GetWindowLongA( hwnd, HFONT_GWL_OFFSET );
}

inline static void set_button_font( HWND hwnd, HFONT font )
{
    SetWindowLongA( hwnd, HFONT_GWL_OFFSET, (LONG)font );
}

inline static UINT get_button_type( LONG window_style )
{
    return (window_style & 0x0f);
}

/* paint a button of any type */
inline static void paint_button( HWND hwnd, LONG style, UINT action )
{
    if (btnPaintFunc[style] && IsWindowVisible(hwnd))
    {
        HDC hdc = GetDC( hwnd );
        btnPaintFunc[style]( hwnd, hdc, action );
        ReleaseDC( hwnd, hdc );
    }
}

/* retrieve the button text; returned buffer must be freed by caller */
inline static WCHAR *get_button_text( HWND hwnd )
{
    INT len = GetWindowTextLengthW( hwnd );
    WCHAR *buffer = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
    if (buffer) GetWindowTextW( hwnd, buffer, len + 1 );
    return buffer;
}

/***********************************************************************
 *           ButtonWndProc_common
 */
static LRESULT WINAPI ButtonWndProc_common(HWND hWnd, UINT uMsg,
                                           WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    RECT rect;
    POINT pt;
    LONG style = GetWindowLongA( hWnd, GWL_STYLE );
    UINT btn_type = get_button_type( style );
    LONG state;
    HANDLE oldHbitmap;

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    switch (uMsg)
    {
    case WM_GETDLGCODE:
        switch(btn_type)
        {
        case BS_PUSHBUTTON:      return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
        case BS_DEFPUSHBUTTON:   return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON: return DLGC_BUTTON | DLGC_RADIOBUTTON;
        default:                 return DLGC_BUTTON;
        }

    case WM_ENABLE:
        paint_button( hWnd, btn_type, ODA_DRAWENTIRE );
        break;

    case WM_CREATE:
        if (!hbitmapCheckBoxes)
        {
            BITMAP bmp;
            hbitmapCheckBoxes = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CHECKBOXES));
            GetObjectW( hbitmapCheckBoxes, sizeof(bmp), &bmp );
            checkBoxWidth  = bmp.bmWidth / 4;
            checkBoxHeight = bmp.bmHeight / 3;
        }
        if (btn_type >= MAX_BTN_TYPE)
            return -1; /* abort */
        set_button_state( hWnd, BUTTON_UNCHECKED );
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        if (btnPaintFunc[btn_type])
        {
            PAINTSTRUCT ps;
            HDC hdc = wParam ? (HDC)wParam : BeginPaint( hWnd, &ps );
            int nOldMode = SetBkMode( hdc, OPAQUE );
            (btnPaintFunc[btn_type])( hWnd, hdc, ODA_DRAWENTIRE );
            SetBkMode(hdc, nOldMode); /*  reset painting mode */
            if( !wParam ) EndPaint( hWnd, &ps );
        }
        break;

    case WM_KEYDOWN:
	if (wParam == VK_SPACE)
	{
	    SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
            set_button_state( hWnd, get_button_state( hWnd ) | BUTTON_BTNPRESSED );
	}
	break;

    case WM_LBUTTONDBLCLK:
        if(style & BS_NOTIFY ||
           btn_type == BS_RADIOBUTTON ||
           btn_type == BS_USERBUTTON ||
           btn_type == BS_OWNERDRAW)
        {
            SendMessageW( GetParent(hWnd), WM_COMMAND,
                          MAKEWPARAM( GetWindowLongA(hWnd,GWL_ID), BN_DOUBLECLICKED ),
                          (LPARAM)hWnd);
            break;
        }
        /* fall through */
    case WM_LBUTTONDOWN:
        SetCapture( hWnd );
        SetFocus( hWnd );
        SendMessageW( hWnd, BM_SETSTATE, TRUE, 0 );
        set_button_state( hWnd, get_button_state( hWnd ) | BUTTON_BTNPRESSED );
        break;

    case WM_KEYUP:
	if (wParam != VK_SPACE)
	    break;
	/* fall through */
    case WM_LBUTTONUP:
        state = get_button_state( hWnd );
        if (!(state & BUTTON_BTNPRESSED)) break;
        state &= BUTTON_NSTATES;
        set_button_state( hWnd, state );
        if (!(state & BUTTON_HIGHLIGHTED))
        {
            ReleaseCapture();
            break;
        }
        SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
        ReleaseCapture();
        GetClientRect( hWnd, &rect );
	if (uMsg == WM_KEYUP || PtInRect( &rect, pt ))
        {
            state = get_button_state( hWnd );
            switch(btn_type)
            {
            case BS_AUTOCHECKBOX:
                SendMessageW( hWnd, BM_SETCHECK, !(state & BUTTON_CHECKED), 0 );
                break;
            case BS_AUTORADIOBUTTON:
                SendMessageW( hWnd, BM_SETCHECK, TRUE, 0 );
                break;
            case BS_AUTO3STATE:
                SendMessageW( hWnd, BM_SETCHECK,
                                (state & BUTTON_3STATE) ? 0 : ((state & 3) + 1), 0 );
                break;
            }
            SendMessageW( GetParent(hWnd), WM_COMMAND,
                          MAKEWPARAM( GetWindowLongA(hWnd,GWL_ID), BN_CLICKED ), (LPARAM)hWnd);
        }
        break;

    case WM_CAPTURECHANGED:
        state = get_button_state( hWnd );
        if (state & BUTTON_BTNPRESSED)
        {
            state &= BUTTON_NSTATES;
            set_button_state( hWnd, state );
            if (state & BUTTON_HIGHLIGHTED) SendMessageW( hWnd, BM_SETSTATE, FALSE, 0 );
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
        HDC hdc = GetDC(hWnd);
        HBRUSH hbrush;
        RECT client, rc;

        hbrush = (HBRUSH)SendMessageW(GetParent(hWnd), WM_CTLCOLORSTATIC,
				      (WPARAM)hdc, (LPARAM)hWnd);
        if (!hbrush) /* did the app forget to call DefWindowProc ? */
            hbrush = (HBRUSH)DefWindowProcW(GetParent(hWnd), WM_CTLCOLORSTATIC,
					    (WPARAM)hdc, (LPARAM)hWnd);

        GetClientRect(hWnd, &client);
        rc = client;
        BUTTON_CalcLabelRect(hWnd, hdc, &rc);
        /* Clip by client rect bounds */
        if (rc.right > client.right) rc.right = client.right;
        if (rc.bottom > client.bottom) rc.bottom = client.bottom;
        FillRect(hdc, &rc, hbrush);
        ReleaseDC(hWnd, hdc);

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
        if (lParam) paint_button( hWnd, btn_type, ODA_DRAWENTIRE );
        break;

    case WM_GETFONT:
        return (LRESULT)get_button_font( hWnd );

    case WM_SETFOCUS:
        if ((btn_type == BS_RADIOBUTTON || btn_type == BS_AUTORADIOBUTTON) && (GetCapture() != hWnd) &&
            !(SendMessageW(hWnd, BM_GETCHECK, 0, 0) & BST_CHECKED))
	{
            /* The notification is sent when the button (BS_AUTORADIOBUTTON)
               is unchecked and the focus was not given by a mouse click. */
            if (btn_type == BS_AUTORADIOBUTTON)
                SendMessageW( hWnd, BM_SETCHECK, BUTTON_CHECKED, 0 );
            SendMessageW( GetParent(hWnd), WM_COMMAND,
                          MAKEWPARAM( GetWindowLongA(hWnd,GWL_ID), BN_CLICKED ), (LPARAM)hWnd);
        }
        set_button_state( hWnd, get_button_state(hWnd) | BUTTON_HASFOCUS );
        paint_button( hWnd, btn_type, ODA_FOCUS );
        break;

    case WM_KILLFOCUS:
        set_button_state( hWnd, get_button_state(hWnd) & ~BUTTON_HASFOCUS );
	paint_button( hWnd, btn_type, ODA_FOCUS );
	InvalidateRect( hWnd, NULL, TRUE );
        break;

    case WM_SYSCOLORCHANGE:
        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case BM_SETSTYLE:
        if ((wParam & 0x0f) >= MAX_BTN_TYPE) break;
        btn_type = wParam & 0x0f;
        style = (style & ~0x0f) | btn_type;
        SetWindowLongA( hWnd, GWL_STYLE, style );

        /* Only redraw if lParam flag is set.*/
        if (lParam)
           paint_button( hWnd, btn_type, ODA_DRAWENTIRE );

        break;

    case BM_CLICK:
	SendMessageW( hWnd, WM_LBUTTONDOWN, 0, 0 );
	SendMessageW( hWnd, WM_LBUTTONUP, 0, 0 );
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
        oldHbitmap = (HBITMAP)SetWindowLongA( hWnd, HIMAGE_GWL_OFFSET, lParam );
	InvalidateRect( hWnd, NULL, FALSE );
	return (LRESULT)oldHbitmap;

    case BM_GETIMAGE:
        return GetWindowLongA( hWnd, HIMAGE_GWL_OFFSET );

    case BM_GETCHECK:
        return get_button_state( hWnd ) & 3;

    case BM_SETCHECK:
        if (wParam > maxCheckState[btn_type]) wParam = maxCheckState[btn_type];
        state = get_button_state( hWnd );
        if ((btn_type == BS_RADIOBUTTON) || (btn_type == BS_AUTORADIOBUTTON))
        {
            if (wParam) style |= WS_TABSTOP;
            else style &= ~WS_TABSTOP;
            SetWindowLongA( hWnd, GWL_STYLE, style );
        }
        if (((WPARAM) state & 3) != wParam)
        {
            set_button_state( hWnd, (state & ~3) | wParam );
            paint_button( hWnd, btn_type, ODA_SELECT );
        }
        if ((btn_type == BS_AUTORADIOBUTTON) && (wParam == BUTTON_CHECKED) && (style & WS_CHILD))
            BUTTON_CheckAutoRadioButton( hWnd );
        break;

    case BM_GETSTATE:
        return get_button_state( hWnd );

    case BM_SETSTATE:
        state = get_button_state( hWnd );
        if (wParam)
        {
            if (state & BUTTON_HIGHLIGHTED) break;
            set_button_state( hWnd, state | BUTTON_HIGHLIGHTED );
        }
        else
        {
            if (!(state & BUTTON_HIGHLIGHTED)) break;
            set_button_state( hWnd, state & ~BUTTON_HIGHLIGHTED );
        }
        paint_button( hWnd, btn_type, ODA_SELECT );
        break;

    case WM_NCHITTEST:
        if(btn_type == BS_GROUPBOX) return HTTRANSPARENT;
        /* fall through */
    default:
        return unicode ? DefWindowProcW(hWnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/***********************************************************************
 *           ButtonWndProcW
 * The button window procedure. This is just a wrapper which locks
 * the passed HWND and calls the real window procedure (with a WND*
 * pointer pointing to the locked windowstructure).
 */
static LRESULT WINAPI ButtonWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow( hWnd )) return 0;
    return ButtonWndProc_common( hWnd, uMsg, wParam, lParam, TRUE );
}


/***********************************************************************
 *           ButtonWndProcA
 */
static LRESULT WINAPI ButtonWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow( hWnd )) return 0;
    return ButtonWndProc_common( hWnd, uMsg, wParam, lParam, FALSE );
}


/**********************************************************************
 * Convert button styles to flags used by DrawText.
 * TODO: handle WS_EX_RIGHT extended style.
 */
static UINT BUTTON_BStoDT(DWORD style)
{
   UINT dtStyle = DT_NOCLIP;  /* We use SelectClipRgn to limit output */

   /* "Convert" pushlike buttons to pushbuttons */
   if (style & BS_PUSHLIKE)
      style &= ~0x0F;

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

   /* DrawText ignores vertical alignment for multiline text,
    * but we use these flags to align label manualy.
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
   LONG style = GetWindowLongA( hwnd, GWL_STYLE );
   WCHAR *text;
   ICONINFO    iconInfo;
   BITMAP      bm;
   UINT        dtStyle = BUTTON_BStoDT(style);
   RECT        r = *rc;
   INT         n;

   /* Calculate label rectangle according to label type */
   switch (style & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
          if (!(text = get_button_text( hwnd ))) goto empty_rect;
          if (!text[0])
          {
              HeapFree( GetProcessHeap(), 0, text );
              goto empty_rect;
          }
          DrawTextW(hdc, text, -1, &r, dtStyle | DT_CALCRECT);
          HeapFree( GetProcessHeap(), 0, text );
          break;

      case BS_ICON:
         if (!GetIconInfo((HICON)GetWindowLongA( hwnd, HIMAGE_GWL_OFFSET ), &iconInfo))
            goto empty_rect;

         GetObjectW (iconInfo.hbmColor, sizeof(BITMAP), &bm);

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;

         DeleteObject(iconInfo.hbmColor);
         DeleteObject(iconInfo.hbmMask);
         break;

      case BS_BITMAP:
         if (!GetObjectW( (HANDLE)GetWindowLongA( hwnd, HIMAGE_GWL_OFFSET ), sizeof(BITMAP), &bm))
            goto empty_rect;

         r.right  = r.left + bm.bmWidth;
         r.bottom = r.top  + bm.bmHeight;
         break;

      default:
      empty_rect:
         r.right = r.left;
         r.bottom = r.top;
         return (UINT)(LONG)-1;
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
   RECT rc;
   rc.left = 0;
   rc.top = 0;
   rc.right = cx;
   rc.bottom = cy;

   DrawTextW(hdc, (LPCWSTR)lp, -1, &rc, (UINT)wp);
   return TRUE;
}


/**********************************************************************
 *       BUTTON_DrawLabel
 *
 *   Common function for drawing button label.
 */
static void BUTTON_DrawLabel(HWND hwnd, HDC hdc, UINT dtFlags, RECT *rc)
{
   DRAWSTATEPROC lpOutputProc = NULL;
   LPARAM lp;
   WPARAM wp = 0;
   HBRUSH hbr = 0;
   UINT flags = IsWindowEnabled(hwnd) ? DSS_NORMAL : DSS_DISABLED;
   LONG state = get_button_state( hwnd );
   LONG style = GetWindowLongA( hwnd, GWL_STYLE );
   WCHAR *text = NULL;

   /* FIXME: To draw disabled label in Win31 look-and-feel, we probably
    * must use DSS_MONO flag and COLOR_GRAYTEXT brush (or maybe DSS_UNION).
    * I don't have Win31 on hand to verify that, so I leave it as is.
    */

   if ((style & BS_PUSHLIKE) && (state & BUTTON_3STATE))
   {
      hbr = GetSysColorBrush(COLOR_GRAYTEXT);
      flags |= DSS_MONO;
   }

   switch (style & (BS_ICON|BS_BITMAP))
   {
      case BS_TEXT:
         /* DST_COMPLEX -- is 0 */
         lpOutputProc = BUTTON_DrawTextCallback;
         if (!(text = get_button_text( hwnd ))) return;
         lp = (LPARAM)text;
         wp = (WPARAM)dtFlags;
         break;

      case BS_ICON:
         flags |= DST_ICON;
         lp = GetWindowLongA( hwnd, HIMAGE_GWL_OFFSET );
         break;

      case BS_BITMAP:
         flags |= DST_BITMAP;
         lp = GetWindowLongA( hwnd, HIMAGE_GWL_OFFSET );
         break;

      default:
         return;
   }

   DrawStateW(hdc, hbr, lpOutputProc, lp, wp, rc->left, rc->top,
              rc->right - rc->left, rc->bottom - rc->top, flags);
   if (text) HeapFree( GetProcessHeap(), 0, text );
}

/**********************************************************************
 *       Push Button Functions
 */
static void PB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    RECT     rc, focus_rect, r;
    UINT     dtFlags;
    HRGN     hRgn;
    HPEN     hOldPen;
    HBRUSH   hOldBrush;
    INT      oldBkMode;
    COLORREF oldTxtColor;
    HFONT hFont;
    LONG state = get_button_state( hwnd );
    LONG style = GetWindowLongA( hwnd, GWL_STYLE );
    BOOL pushedState = (state & BUTTON_HIGHLIGHTED);
    UINT uState;

    GetClientRect( hwnd, &rc );

    /* Send WM_CTLCOLOR to allow changing the font (the colors are fixed) */
    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );
    SendMessageW( GetParent(hwnd), WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)hwnd );
    hOldPen = (HPEN)SelectObject(hDC, GetSysColorPen(COLOR_WINDOWFRAME));
    hOldBrush =(HBRUSH)SelectObject(hDC,GetSysColorBrush(COLOR_BTNFACE));
    oldBkMode = SetBkMode(hDC, TRANSPARENT);

    if (get_button_type(style) == BS_DEFPUSHBUTTON)
    {
        Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
	InflateRect( &rc, -1, -1 );
    }

    uState = DFCS_BUTTONPUSH | DFCS_ADJUSTRECT;

        if (style & BS_FLAT)
            uState |= DFCS_MONO;
        else if (pushedState)
	{
	    if (get_button_type(style) == BS_DEFPUSHBUTTON )
	        uState |= DFCS_FLAT;
	    else
	        uState |= DFCS_PUSHED;
	}

        if (state & (BUTTON_CHECKED | BUTTON_3STATE))
            uState |= DFCS_CHECKED;

	DrawFrameControl( hDC, &rc, DFC_BUTTON, uState );

	focus_rect = rc;

    /* draw button label */
    r = rc;
    dtFlags = BUTTON_CalcLabelRect(hwnd, hDC, &r);

    if (dtFlags == (UINT)-1L)
       goto cleanup;

    if (pushedState)
       OffsetRect(&r, 1, 1);

    hRgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
    SelectClipRgn(hDC, hRgn);

    oldTxtColor = SetTextColor( hDC, GetSysColor(COLOR_BTNTEXT) );

    BUTTON_DrawLabel(hwnd, hDC, dtFlags, &r);

    SetTextColor( hDC, oldTxtColor );
    SelectClipRgn(hDC, 0);
    DeleteObject(hRgn);

    if (state & BUTTON_HASFOCUS)
    {
        InflateRect( &focus_rect, -1, -1 );
        IntersectRect(&focus_rect, &focus_rect, &rc);
        DrawFocusRect( hDC, &focus_rect );
    }

 cleanup:
    SelectObject( hDC, hOldPen );
    SelectObject( hDC, hOldBrush );
    SetBkMode(hDC, oldBkMode);
}

/**********************************************************************
 *       Check Box & Radio Button Functions
 */

static void CB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    RECT rbox, rtext, client;
    HBRUSH hBrush;
    int delta;
    UINT dtFlags;
    HRGN hRgn;
    HFONT hFont;
    LONG state = get_button_state( hwnd );
    LONG style = GetWindowLongA( hwnd, GWL_STYLE );

    if (style & BS_PUSHLIKE)
    {
        PB_Paint( hwnd, hDC, action );
	return;
    }

    GetClientRect(hwnd, &client);
    rbox = rtext = client;

    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );

    hBrush = (HBRUSH)SendMessageW(GetParent(hwnd), WM_CTLCOLORSTATIC,
				  (WPARAM)hDC, (LPARAM)hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(GetParent(hwnd), WM_CTLCOLORSTATIC,
					(WPARAM)hDC, (LPARAM)hwnd );

    if (style & BS_LEFTTEXT)
    {
	/* magic +4 is what CTL3D expects */

        rtext.right -= checkBoxWidth + 4;
        rbox.left = rbox.right - checkBoxWidth;
    }
    else
    {
        rtext.left += checkBoxWidth + 4;
        rbox.right = checkBoxWidth;
    }
 
    /* Since WM_ERASEBKGND does nothing, first prepare background */
    if (action == ODA_SELECT) FillRect( hDC, &rbox, hBrush );
    if (action == ODA_DRAWENTIRE) FillRect( hDC, &client, hBrush );

    /* Draw label */
    client = rtext;
    dtFlags = BUTTON_CalcLabelRect(hwnd, hDC, &rtext);
    
    rbox.top = rtext.top;
    rbox.bottom = rtext.bottom;
    /* Draw the check-box bitmap */
    if (action == ODA_DRAWENTIRE || action == ODA_SELECT)
    {
            UINT flags;

            if ((get_button_type(style) == BS_RADIOBUTTON) ||
                (get_button_type(style) == BS_AUTORADIOBUTTON)) flags = DFCS_BUTTONRADIO;
            else if (state & BUTTON_3STATE) flags = DFCS_BUTTON3STATE;
	    else flags = DFCS_BUTTONCHECK;

            if (state & (BUTTON_CHECKED | BUTTON_3STATE)) flags |= DFCS_CHECKED;
	    if (state & BUTTON_HIGHLIGHTED) flags |= DFCS_PUSHED;

	    if (style & WS_DISABLED) flags |= DFCS_INACTIVE;

	    /* rbox must have the correct height */
	    delta = rbox.bottom - rbox.top - checkBoxHeight;
	    
	    if (style & BS_TOP) {
	      if (delta > 0) {
		rbox.bottom = rbox.top + checkBoxHeight;
	      } else {
		rbox.top -= -delta/2 + 1;
		rbox.bottom += rbox.top + checkBoxHeight;
	      }
	    } else if (style & BS_BOTTOM) {
	      if (delta > 0) {
		rbox.top = rbox.bottom - checkBoxHeight;
	      } else {
		rbox.bottom += -delta/2 + 1;
		rbox.top = rbox.bottom -= checkBoxHeight;
	      }
	    } else { /* Default */
	      if (delta > 0)
		{
		  int ofs = (delta / 2);
		  rbox.bottom -= ofs + 1;
		  rbox.top = rbox.bottom - checkBoxHeight;
		}
	      else if (delta < 0)
		{
		  int ofs = (-delta / 2);
		  rbox.top -= ofs + 1;
		  rbox.bottom = rbox.top + checkBoxHeight;
		}
	    }

	    DrawFrameControl( hDC, &rbox, DFC_BUTTON, flags );
        }

    if (dtFlags == (UINT)-1L) /* Noting to draw */
	return;
    hRgn = CreateRectRgn(client.left, client.top, client.right, client.bottom);
    SelectClipRgn(hDC, hRgn);
    DeleteObject(hRgn);

    if (action == ODA_DRAWENTIRE)
	BUTTON_DrawLabel(hwnd, hDC, dtFlags, &rtext);

    /* ... and focus */
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (state & BUTTON_HASFOCUS)))
    {
	rtext.left--;
	rtext.right++;
	IntersectRect(&rtext, &rtext, &client);
	DrawFocusRect( hDC, &rtext );
    }
    SelectClipRgn(hDC, 0);
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
            ((GetWindowLongA( sibling, GWL_STYLE) & 0x0f) == BS_AUTORADIOBUTTON))
            SendMessageW( sibling, BM_SETCHECK, BUTTON_UNCHECKED, 0 );
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
    LONG style = GetWindowLongA( hwnd, GWL_STYLE );

    if (action != ODA_DRAWENTIRE) return;

    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );
    /* GroupBox acts like static control, so it sends CTLCOLORSTATIC */
    hbr = (HBRUSH)SendMessageW(GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hDC, (LPARAM)hwnd);
    if (!hbr) /* did the app forget to call defwindowproc ? */
        hbr = (HBRUSH)DefWindowProcW(GetParent(hwnd), WM_CTLCOLORSTATIC,
				     (WPARAM)hDC, (LPARAM)hwnd);

    GetClientRect( hwnd, &rc);

	rcFrame = rc;

	GetTextMetricsW (hDC, &tm);
	rcFrame.top += (tm.tmHeight / 2) - 1;
	DrawEdge (hDC, &rcFrame, EDGE_ETCHED, BF_RECT | ((style & BS_FLAT) ? BF_FLAT : 0));
    

    InflateRect(&rc, -7, 1);
    dtFlags = BUTTON_CalcLabelRect(hwnd, hDC, &rc);

    if (dtFlags == (UINT)-1L)
       return;

    /* Because buttons have CS_PARENTDC class style, there is a chance
     * that label will be drawn out of client rect.
     * But Windows doesn't clip label's rect, so do I.
     */

    /* There is 1-pixel marging at the left, right, and bottom */
    rc.left--; rc.right++; rc.bottom++;
    FillRect(hDC, &rc, hbr);
    rc.left++; rc.right--; rc.bottom--;

    BUTTON_DrawLabel(hwnd, hDC, dtFlags, &rc);
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

    if (action == ODA_SELECT) return;

    GetClientRect( hwnd, &rc);

    if ((hFont = get_button_font( hwnd ))) SelectObject( hDC, hFont );

    hBrush = (HBRUSH)SendMessageW(GetParent(hwnd), WM_CTLCOLORBTN, (WPARAM)hDC, (LPARAM)hwnd);
    if (!hBrush) /* did the app forget to call defwindowproc ? */
        hBrush = (HBRUSH)DefWindowProcW(GetParent(hwnd), WM_CTLCOLORBTN,
					(WPARAM)hDC, (LPARAM)hwnd);

    FillRect( hDC, &rc, hBrush );
    if ((action == ODA_FOCUS) ||
        ((action == ODA_DRAWENTIRE) && (state & BUTTON_HASFOCUS)))
        DrawFocusRect( hDC, &rc );
}


/**********************************************************************
 *       Ownerdrawn Button Functions
 */

static void OB_Paint( HWND hwnd, HDC hDC, UINT action )
{
    LONG state = get_button_state( hwnd );
    DRAWITEMSTRUCT dis;
    HRGN clipRegion;
    RECT clipRect;
    UINT id = GetWindowLongA( hwnd, GWL_ID );

    dis.CtlType    = ODT_BUTTON;
    dis.CtlID      = id;
    dis.itemID     = 0;
    dis.itemAction = action;
    dis.itemState  = ((state & BUTTON_HASFOCUS) ? ODS_FOCUS : 0) |
                     ((state & BUTTON_HIGHLIGHTED) ? ODS_SELECTED : 0) |
                     (IsWindowEnabled(hwnd) ? 0: ODS_DISABLED);
    dis.hwndItem   = hwnd;
    dis.hDC        = hDC;
    dis.itemData   = 0;
    GetClientRect( hwnd, &dis.rcItem );

    clipRegion = CreateRectRgnIndirect(&dis.rcItem);
    if (GetClipRgn(hDC, clipRegion) != 1)
    {
	DeleteObject(clipRegion);
	clipRegion=NULL;
    }
    clipRect = dis.rcItem;
    DPtoLP(hDC, (LPPOINT) &clipRect, 2);
    IntersectClipRect(hDC, clipRect.left,  clipRect.top, clipRect.right, clipRect.bottom);

    SetBkColor( hDC, GetSysColor( COLOR_BTNFACE ) );
    SendMessageW( GetParent(hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
    SelectClipRgn(hDC, clipRegion);
}
