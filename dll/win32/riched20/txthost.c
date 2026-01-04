/*
 * RichEdit - ITextHost implementation for windowed richedit controls
 *
 * Copyright 2009 by Dylan Smith
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

#define COBJMACROS

#include "editor.h"
#include "ole2.h"
#include "richole.h"
#include "imm.h"
#include "textserv.h"
#include "wine/debug.h"
#include "editstr.h"
#include "rtf.h"
#include "undocuser.h"
#include "riched20.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

struct host
{
    ITextHost2 ITextHost_iface;
    LONG ref;
    ITextServices *text_srv;
    HWND window, parent;
    unsigned int emulate_10 : 1;
    unsigned int dialog_mode : 1;
    unsigned int want_return : 1;
    unsigned int sel_bar : 1;
    unsigned int client_edge : 1;
    unsigned int use_set_rect : 1;
    unsigned int use_back_colour : 1;
    unsigned int defer_release : 1;
    PARAFORMAT2 para_fmt;
    DWORD props, scrollbars, event_mask;
    RECT client_rect, set_rect;
    COLORREF back_colour;
    WCHAR password_char;
    unsigned int notify_level;
};

static const ITextHost2Vtbl textHostVtbl;

static BOOL listbox_registered;
static BOOL combobox_registered;

static void host_init_props( struct host *host )
{
    DWORD style;

    style = GetWindowLongW( host->window, GWL_STYLE );

    /* text services assumes the scrollbars are originally not shown, so hide them.
       However with ES_DISABLENOSCROLL it'll immediately show them, so don't bother */
    if (!(style & ES_DISABLENOSCROLL)) ShowScrollBar( host->window, SB_BOTH, FALSE );

    host->scrollbars = style & (WS_VSCROLL | WS_HSCROLL | ES_AUTOVSCROLL |
                                ES_AUTOHSCROLL | ES_DISABLENOSCROLL);
    if (style & WS_VSCROLL) host->scrollbars |= ES_AUTOVSCROLL;
    if ((style & WS_HSCROLL) && !host->emulate_10) host->scrollbars |= ES_AUTOHSCROLL;

    host->props = TXTBIT_RICHTEXT | TXTBIT_ALLOWBEEP;
    if (style & ES_MULTILINE)     host->props |= TXTBIT_MULTILINE;
    if (style & ES_READONLY)      host->props |= TXTBIT_READONLY;
    if (style & ES_PASSWORD)      host->props |= TXTBIT_USEPASSWORD;
    if (!(style & ES_NOHIDESEL))  host->props |= TXTBIT_HIDESELECTION;
    if (style & ES_SAVESEL)       host->props |= TXTBIT_SAVESELECTION;
    if (style & ES_VERTICAL)      host->props |= TXTBIT_VERTICAL;
    if (style & ES_NOOLEDRAGDROP) host->props |= TXTBIT_DISABLEDRAG;

    if (!(host->scrollbars & ES_AUTOHSCROLL)) host->props |= TXTBIT_WORDWRAP;

    host->sel_bar     = !!(style & ES_SELECTIONBAR);
    host->want_return = !!(style & ES_WANTRETURN);

    style = GetWindowLongW( host->window, GWL_EXSTYLE );
    host->client_edge = !!(style & WS_EX_CLIENTEDGE);
}

struct host *host_create( HWND hwnd, CREATESTRUCTW *cs, BOOL emulate_10 )
{
    struct host *texthost;

    texthost = malloc( sizeof(*texthost) );
    if (!texthost) return NULL;

    texthost->ITextHost_iface.lpVtbl = &textHostVtbl;
    texthost->ref = 1;
    texthost->text_srv = NULL;
    texthost->window = hwnd;
    texthost->parent = cs->hwndParent;
    texthost->emulate_10 = emulate_10;
    texthost->dialog_mode = 0;
    memset( &texthost->para_fmt, 0, sizeof(texthost->para_fmt) );
    texthost->para_fmt.cbSize = sizeof(texthost->para_fmt);
    texthost->para_fmt.dwMask = PFM_ALIGNMENT;
    texthost->para_fmt.wAlignment = PFA_LEFT;
    if (cs->style & ES_RIGHT)
        texthost->para_fmt.wAlignment = PFA_RIGHT;
    if (cs->style & ES_CENTER)
        texthost->para_fmt.wAlignment = PFA_CENTER;
    host_init_props( texthost );
    texthost->event_mask = 0;
    texthost->use_set_rect = 0;
    SetRectEmpty( &texthost->set_rect );
    GetClientRect( hwnd, &texthost->client_rect );
    texthost->use_back_colour = 0;
    texthost->password_char = (texthost->props & TXTBIT_USEPASSWORD) ? '*' : 0;
    texthost->defer_release = 0;
    texthost->notify_level = 0;

    return texthost;
}

static inline struct host *impl_from_ITextHost( ITextHost2 *iface )
{
    return CONTAINING_RECORD( iface, struct host, ITextHost_iface );
}

static HRESULT WINAPI ITextHostImpl_QueryInterface( ITextHost2 *iface, REFIID iid, void **obj )
{
    struct host *host = impl_from_ITextHost( iface );

    if (IsEqualIID( iid, &IID_IUnknown ) || IsEqualIID( iid, &IID_ITextHost ) || IsEqualIID( iid, &IID_ITextHost2 ))
    {
        *obj = &host->ITextHost_iface;
        ITextHost_AddRef( (ITextHost *)*obj );
        return S_OK;
    }

    FIXME( "Unknown interface: %s\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI ITextHostImpl_AddRef( ITextHost2 *iface )
{
    struct host *host = impl_from_ITextHost( iface );
    ULONG ref = InterlockedIncrement( &host->ref );
    return ref;
}

static ULONG WINAPI ITextHostImpl_Release( ITextHost2 *iface )
{
    struct host *host = impl_from_ITextHost( iface );
    ULONG ref = InterlockedDecrement( &host->ref );

    if (!ref)
    {
        SetWindowLongPtrW( host->window, 0, 0 );
        ITextServices_Release( host->text_srv );
        free( host );
    }
    return ref;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetDC,4)
HDC __thiscall ITextHostImpl_TxGetDC( ITextHost2 *iface )
{
    struct host *host = impl_from_ITextHost( iface );
    return GetDC( host->window );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxReleaseDC,8)
INT __thiscall ITextHostImpl_TxReleaseDC( ITextHost2 *iface, HDC hdc )
{
    struct host *host = impl_from_ITextHost( iface );
    return ReleaseDC( host->window, hdc );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowScrollBar,12)
BOOL __thiscall ITextHostImpl_TxShowScrollBar( ITextHost2 *iface, INT bar, BOOL show )
{
    struct host *host = impl_from_ITextHost( iface );
    return ShowScrollBar( host->window, bar, show );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxEnableScrollBar,12)
BOOL __thiscall ITextHostImpl_TxEnableScrollBar( ITextHost2 *iface, INT bar, INT arrows )
{
    struct host *host = impl_from_ITextHost( iface );
    return EnableScrollBar( host->window, bar, arrows );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollRange,20)
BOOL __thiscall ITextHostImpl_TxSetScrollRange( ITextHost2 *iface, INT bar, LONG min_pos, INT max_pos, BOOL redraw )
{
    struct host *host = impl_from_ITextHost( iface );
    SCROLLINFO info = { .cbSize = sizeof(info), .fMask = SIF_PAGE | SIF_RANGE };

    if (bar != SB_HORZ && bar != SB_VERT)
    {
        FIXME( "Unexpected bar %d\n", bar );
        return FALSE;
    }

    if (host->scrollbars & ES_DISABLENOSCROLL) info.fMask |= SIF_DISABLENOSCROLL;

    if (host->text_srv) /* This can be called during text services creation */
    {
        if (bar == SB_HORZ) ITextServices_TxGetHScroll( host->text_srv, NULL, NULL, NULL, (LONG *)&info.nPage, NULL );
        else ITextServices_TxGetVScroll( host->text_srv, NULL, NULL, NULL, (LONG *)&info.nPage, NULL );
    }

    info.nMin = min_pos;
    info.nMax = max_pos;
    return SetScrollInfo( host->window, bar, &info, redraw );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetScrollPos,16)
BOOL __thiscall ITextHostImpl_TxSetScrollPos( ITextHost2 *iface, INT bar, INT pos, BOOL redraw )
{
    struct host *host = impl_from_ITextHost( iface );
    DWORD style = GetWindowLongW( host->window, GWL_STYLE );
    DWORD mask = (bar == SB_HORZ) ? WS_HSCROLL : WS_VSCROLL;
    BOOL show = TRUE, shown = style & mask;

    if (bar != SB_HORZ && bar != SB_VERT)
    {
        FIXME( "Unexpected bar %d\n", bar );
        return FALSE;
    }

    /* If the application has adjusted the scrollbar's visibility it is reset */
    if (!(host->scrollbars & ES_DISABLENOSCROLL))
    {
        if (bar == SB_HORZ) ITextServices_TxGetHScroll( host->text_srv, NULL, NULL, NULL, NULL, &show );
        else ITextServices_TxGetVScroll( host->text_srv, NULL, NULL, NULL, NULL, &show );
    }

    if (!show ^ !shown) ShowScrollBar( host->window, bar, show );
    return SetScrollPos( host->window, bar, pos, redraw ) != 0;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxInvalidateRect,12)
void __thiscall ITextHostImpl_TxInvalidateRect( ITextHost2 *iface, const RECT *rect, BOOL mode )
{
    struct host *host = impl_from_ITextHost( iface );
    InvalidateRect( host->window, rect, mode );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxViewChange,8)
void __thiscall ITextHostImpl_TxViewChange( ITextHost2 *iface, BOOL update )
{
    struct host *host = impl_from_ITextHost( iface );
    if (update) UpdateWindow( host->window );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxCreateCaret,16)
BOOL __thiscall ITextHostImpl_TxCreateCaret( ITextHost2 *iface, HBITMAP bitmap, INT width, INT height )
{
    struct host *host = impl_from_ITextHost( iface );
    return CreateCaret( host->window, bitmap, width, height );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowCaret,8)
BOOL __thiscall ITextHostImpl_TxShowCaret( ITextHost2 *iface, BOOL show )
{
    struct host *host = impl_from_ITextHost( iface );
    if (show) return ShowCaret( host->window );
    else return HideCaret( host->window );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCaretPos,12)
BOOL __thiscall ITextHostImpl_TxSetCaretPos( ITextHost2 *iface, INT x, INT y )
{
    return SetCaretPos(x, y);
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetTimer,12)
BOOL __thiscall ITextHostImpl_TxSetTimer( ITextHost2 *iface, UINT id, UINT timeout )
{
    struct host *host = impl_from_ITextHost( iface );
    return SetTimer( host->window, id, timeout, NULL ) != 0;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxKillTimer,8)
void __thiscall ITextHostImpl_TxKillTimer( ITextHost2 *iface, UINT id )
{
    struct host *host = impl_from_ITextHost( iface );
    KillTimer( host->window, id );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScrollWindowEx,32)
void __thiscall ITextHostImpl_TxScrollWindowEx( ITextHost2 *iface, INT dx, INT dy, const RECT *scroll,
                                                                const RECT *clip, HRGN update_rgn, RECT *update_rect,
                                                                UINT flags )
{
    struct host *host = impl_from_ITextHost( iface );
    ScrollWindowEx( host->window, dx, dy, scroll, clip, update_rgn, update_rect, flags );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCapture,8)
void __thiscall ITextHostImpl_TxSetCapture( ITextHost2 *iface, BOOL capture )
{
    struct host *host = impl_from_ITextHost( iface );
    if (capture) SetCapture( host->window );
    else ReleaseCapture();
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetFocus,4)
void __thiscall ITextHostImpl_TxSetFocus( ITextHost2 *iface )
{
    struct host *host = impl_from_ITextHost( iface );
    SetFocus( host->window );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCursor,12)
void __thiscall ITextHostImpl_TxSetCursor( ITextHost2 *iface, HCURSOR cursor, BOOL text )
{
    SetCursor( cursor );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxScreenToClient,8)
BOOL __thiscall ITextHostImpl_TxScreenToClient( ITextHost2 *iface, POINT *pt )
{
    struct host *host = impl_from_ITextHost( iface );
    return ScreenToClient( host->window, pt );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxClientToScreen,8)
BOOL __thiscall ITextHostImpl_TxClientToScreen( ITextHost2 *iface, POINT *pt )
{
    struct host *host = impl_from_ITextHost( iface );
    return ClientToScreen( host->window, pt );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxActivate,8)
HRESULT __thiscall ITextHostImpl_TxActivate( ITextHost2 *iface, LONG *old_state )
{
    struct host *host = impl_from_ITextHost( iface );
    *old_state = HandleToLong( SetActiveWindow( host->window ) );
    return *old_state ? S_OK : E_FAIL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxDeactivate,8)
HRESULT __thiscall ITextHostImpl_TxDeactivate( ITextHost2 *iface, LONG new_state )
{
    HWND ret = SetActiveWindow( LongToHandle( new_state ) );
    return ret ? S_OK : E_FAIL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetClientRect,8)
HRESULT __thiscall ITextHostImpl_TxGetClientRect( ITextHost2 *iface, RECT *rect )
{
    struct host *host = impl_from_ITextHost( iface );

    if (!host->use_set_rect)
    {
        *rect = host->client_rect;
        if (host->client_edge) rect->top += 1;
        InflateRect( rect, -1, 0 );
    }
    else *rect = host->set_rect;

    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetViewInset,8)
HRESULT __thiscall ITextHostImpl_TxGetViewInset( ITextHost2 *iface, RECT *rect )
{
    SetRectEmpty( rect );
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetCharFormat,8)
HRESULT __thiscall ITextHostImpl_TxGetCharFormat( ITextHost2 *iface, const CHARFORMATW **ppCF )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetParaFormat,8)
HRESULT __thiscall ITextHostImpl_TxGetParaFormat( ITextHost2 *iface, const PARAFORMAT **fmt )
{
    struct host *host = impl_from_ITextHost( iface );
    *fmt = (const PARAFORMAT *)&host->para_fmt;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSysColor,8)
COLORREF __thiscall ITextHostImpl_TxGetSysColor( ITextHost2 *iface, int index )
{
    struct host *host = impl_from_ITextHost( iface );

    if (index == COLOR_WINDOW && host->use_back_colour) return host->back_colour;
    return GetSysColor( index );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetBackStyle,8)
HRESULT __thiscall ITextHostImpl_TxGetBackStyle( ITextHost2 *iface, TXTBACKSTYLE *style )
{
    *style = TXTBACK_OPAQUE;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetMaxLength,8)
HRESULT __thiscall ITextHostImpl_TxGetMaxLength( ITextHost2 *iface, DWORD *length )
{
    *length = INFINITE;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetScrollBars,8)
HRESULT __thiscall ITextHostImpl_TxGetScrollBars( ITextHost2 *iface, DWORD *scrollbars )
{
    struct host *host = impl_from_ITextHost( iface );

    *scrollbars = host->scrollbars;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPasswordChar,8)
HRESULT __thiscall ITextHostImpl_TxGetPasswordChar( ITextHost2 *iface, WCHAR *c )
{
    struct host *host = impl_from_ITextHost( iface );

    *c = host->password_char;
    return *c ? S_OK : S_FALSE;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetAcceleratorPos,8)
HRESULT __thiscall ITextHostImpl_TxGetAcceleratorPos( ITextHost2 *iface, LONG *pos )
{
    *pos = -1;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetExtent,8)
HRESULT __thiscall ITextHostImpl_TxGetExtent( ITextHost2 *iface, SIZEL *extent )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxCharFormatChange,8)
HRESULT __thiscall ITextHostImpl_OnTxCharFormatChange( ITextHost2 *iface, const CHARFORMATW *pcf )
{
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_OnTxParaFormatChange,8)
HRESULT __thiscall ITextHostImpl_OnTxParaFormatChange( ITextHost2 *iface, const PARAFORMAT *ppf )
{
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPropertyBits,12)
HRESULT __thiscall ITextHostImpl_TxGetPropertyBits( ITextHost2 *iface, DWORD mask, DWORD *bits )
{
    struct host *host = impl_from_ITextHost( iface );

    *bits = host->props & mask;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxNotify,12)
HRESULT __thiscall ITextHostImpl_TxNotify( ITextHost2 *iface, DWORD iNotify, void *pv )
{
    struct host *host = impl_from_ITextHost( iface );
    UINT id;

    if (!host->parent) return S_OK;

    id = GetWindowLongW( host->window, GWLP_ID );

    switch (iNotify)
    {
        case EN_DROPFILES:
        case EN_LINK:
        case EN_OLEOPFAILED:
        case EN_PROTECTED:
        case EN_REQUESTRESIZE:
        case EN_SAVECLIPBOARD:
        case EN_SELCHANGE:
        case EN_STOPNOUNDO:
        {
            /* FIXME: Verify this assumption that pv starts with NMHDR. */
            NMHDR *info = pv;
            if (!info)
                return E_FAIL;

            info->hwndFrom = host->window;
            info->idFrom = id;
            info->code = iNotify;
            SendMessageW( host->parent, WM_NOTIFY, id, (LPARAM)info );
            break;
        }

        case EN_UPDATE:
            /* Only sent when the window is visible. */
            if (!IsWindowVisible( host->window ))
                break;
            /* Fall through */
        case EN_CHANGE:
        case EN_ERRSPACE:
        case EN_HSCROLL:
        case EN_KILLFOCUS:
        case EN_MAXTEXT:
        case EN_SETFOCUS:
        case EN_VSCROLL:
            SendMessageW( host->parent, WM_COMMAND, MAKEWPARAM( id, iNotify ), (LPARAM)host->window );
            break;

        case EN_MSGFILTER:
            FIXME("EN_MSGFILTER is documented as not being sent to TxNotify\n");
            /* fall through */
        default:
            return E_FAIL;
    }
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmGetContext,4)
HIMC __thiscall ITextHostImpl_TxImmGetContext( ITextHost2 *iface )
{
    struct host *host = impl_from_ITextHost( iface );
    return ImmGetContext( host->window );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxImmReleaseContext,8)
void __thiscall ITextHostImpl_TxImmReleaseContext( ITextHost2 *iface, HIMC context )
{
    struct host *host = impl_from_ITextHost( iface );
    ImmReleaseContext( host->window, context );
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetSelectionBarWidth,8)
HRESULT __thiscall ITextHostImpl_TxGetSelectionBarWidth( ITextHost2 *iface, LONG *width )
{
    struct host *host = impl_from_ITextHost( iface );

    *width = host->sel_bar ? 225 : 0; /* in HIMETRIC */
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxIsDoubleClickPending,4)
BOOL __thiscall ITextHostImpl_TxIsDoubleClickPending( ITextHost2 *iface )
{
    return FALSE;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetWindow,8)
HRESULT __thiscall ITextHostImpl_TxGetWindow( ITextHost2 *iface, HWND *hwnd )
{
    struct host *host = impl_from_ITextHost( iface );
    *hwnd = host->window;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetForegroundWindow,4)
HRESULT __thiscall ITextHostImpl_TxSetForegroundWindow( ITextHost2 *iface )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetPalette,4)
HPALETTE __thiscall ITextHostImpl_TxGetPalette( ITextHost2 *iface )
{
    return NULL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetEastAsianFlags,8)
HRESULT __thiscall ITextHostImpl_TxGetEastAsianFlags( ITextHost2 *iface, LONG *flags )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxSetCursor2,12)
HCURSOR __thiscall ITextHostImpl_TxSetCursor2( ITextHost2 *iface, HCURSOR cursor, BOOL text )
{
    return NULL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxFreeTextServicesNotification,4)
void __thiscall ITextHostImpl_TxFreeTextServicesNotification( ITextHost2 *iface )
{
    return;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetEditStyle,12)
HRESULT __thiscall ITextHostImpl_TxGetEditStyle( ITextHost2 *iface, DWORD item, DWORD *data )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetWindowStyles,12)
HRESULT __thiscall ITextHostImpl_TxGetWindowStyles( ITextHost2 *iface, DWORD *style, DWORD *ex_style )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxShowDropCaret,16)
HRESULT __thiscall ITextHostImpl_TxShowDropCaret( ITextHost2 *iface, BOOL show, HDC hdc, const RECT *rect )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxDestroyCaret,4)
HRESULT __thiscall ITextHostImpl_TxDestroyCaret( ITextHost2 *iface )
{
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(ITextHostImpl_TxGetHorzExtent,8)
HRESULT __thiscall ITextHostImpl_TxGetHorzExtent( ITextHost2 *iface, LONG *horz_extent )
{
    return E_NOTIMPL;
}


#ifdef __ASM_USE_THISCALL_WRAPPER

#define STDCALL(func) (void *) __stdcall_ ## func
#ifdef _MSC_VER
#define DEFINE_STDCALL_WRAPPER(num,func,args) \
    __declspec(naked) HRESULT __stdcall_##func(void) \
    { \
        __asm pop eax \
        __asm pop ecx \
        __asm push eax \
        __asm mov eax, [ecx] \
        __asm jmp dword ptr [eax + 4*num] \
    }
#else /* _MSC_VER */
#define DEFINE_STDCALL_WRAPPER(num,func,args) \
   extern HRESULT __stdcall_ ## func(void); \
   __ASM_GLOBAL_FUNC(__stdcall_ ## func, \
                   "popl %eax\n\t" \
                   "popl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "movl (%ecx), %eax\n\t" \
                   "jmp *(4*(" #num "))(%eax)" )
#endif /* _MSC_VER */

DEFINE_STDCALL_WRAPPER(3,ITextHostImpl_TxGetDC,4)
DEFINE_STDCALL_WRAPPER(4,ITextHostImpl_TxReleaseDC,8)
DEFINE_STDCALL_WRAPPER(5,ITextHostImpl_TxShowScrollBar,12)
DEFINE_STDCALL_WRAPPER(6,ITextHostImpl_TxEnableScrollBar,12)
DEFINE_STDCALL_WRAPPER(7,ITextHostImpl_TxSetScrollRange,20)
DEFINE_STDCALL_WRAPPER(8,ITextHostImpl_TxSetScrollPos,16)
DEFINE_STDCALL_WRAPPER(9,ITextHostImpl_TxInvalidateRect,12)
DEFINE_STDCALL_WRAPPER(10,ITextHostImpl_TxViewChange,8)
DEFINE_STDCALL_WRAPPER(11,ITextHostImpl_TxCreateCaret,16)
DEFINE_STDCALL_WRAPPER(12,ITextHostImpl_TxShowCaret,8)
DEFINE_STDCALL_WRAPPER(13,ITextHostImpl_TxSetCaretPos,12)
DEFINE_STDCALL_WRAPPER(14,ITextHostImpl_TxSetTimer,12)
DEFINE_STDCALL_WRAPPER(15,ITextHostImpl_TxKillTimer,8)
DEFINE_STDCALL_WRAPPER(16,ITextHostImpl_TxScrollWindowEx,32)
DEFINE_STDCALL_WRAPPER(17,ITextHostImpl_TxSetCapture,8)
DEFINE_STDCALL_WRAPPER(18,ITextHostImpl_TxSetFocus,4)
DEFINE_STDCALL_WRAPPER(19,ITextHostImpl_TxSetCursor,12)
DEFINE_STDCALL_WRAPPER(20,ITextHostImpl_TxScreenToClient,8)
DEFINE_STDCALL_WRAPPER(21,ITextHostImpl_TxClientToScreen,8)
DEFINE_STDCALL_WRAPPER(22,ITextHostImpl_TxActivate,8)
DEFINE_STDCALL_WRAPPER(23,ITextHostImpl_TxDeactivate,8)
DEFINE_STDCALL_WRAPPER(24,ITextHostImpl_TxGetClientRect,8)
DEFINE_STDCALL_WRAPPER(25,ITextHostImpl_TxGetViewInset,8)
DEFINE_STDCALL_WRAPPER(26,ITextHostImpl_TxGetCharFormat,8)
DEFINE_STDCALL_WRAPPER(27,ITextHostImpl_TxGetParaFormat,8)
DEFINE_STDCALL_WRAPPER(28,ITextHostImpl_TxGetSysColor,8)
DEFINE_STDCALL_WRAPPER(29,ITextHostImpl_TxGetBackStyle,8)
DEFINE_STDCALL_WRAPPER(30,ITextHostImpl_TxGetMaxLength,8)
DEFINE_STDCALL_WRAPPER(31,ITextHostImpl_TxGetScrollBars,8)
DEFINE_STDCALL_WRAPPER(32,ITextHostImpl_TxGetPasswordChar,8)
DEFINE_STDCALL_WRAPPER(33,ITextHostImpl_TxGetAcceleratorPos,8)
DEFINE_STDCALL_WRAPPER(34,ITextHostImpl_TxGetExtent,8)
DEFINE_STDCALL_WRAPPER(35,ITextHostImpl_OnTxCharFormatChange,8)
DEFINE_STDCALL_WRAPPER(36,ITextHostImpl_OnTxParaFormatChange,8)
DEFINE_STDCALL_WRAPPER(37,ITextHostImpl_TxGetPropertyBits,12)
DEFINE_STDCALL_WRAPPER(38,ITextHostImpl_TxNotify,12)
DEFINE_STDCALL_WRAPPER(39,ITextHostImpl_TxImmGetContext,4)
DEFINE_STDCALL_WRAPPER(40,ITextHostImpl_TxImmReleaseContext,8)
DEFINE_STDCALL_WRAPPER(41,ITextHostImpl_TxGetSelectionBarWidth,8)
/* ITextHost2 */
DEFINE_STDCALL_WRAPPER(42,ITextHostImpl_TxIsDoubleClickPending,4)
DEFINE_STDCALL_WRAPPER(43,ITextHostImpl_TxGetWindow,8)
DEFINE_STDCALL_WRAPPER(44,ITextHostImpl_TxSetForegroundWindow,4)
DEFINE_STDCALL_WRAPPER(45,ITextHostImpl_TxGetPalette,4)
DEFINE_STDCALL_WRAPPER(46,ITextHostImpl_TxGetEastAsianFlags,8)
DEFINE_STDCALL_WRAPPER(47,ITextHostImpl_TxSetCursor2,12)
DEFINE_STDCALL_WRAPPER(48,ITextHostImpl_TxFreeTextServicesNotification,4)
DEFINE_STDCALL_WRAPPER(49,ITextHostImpl_TxGetEditStyle,12)
DEFINE_STDCALL_WRAPPER(50,ITextHostImpl_TxGetWindowStyles,12)
DEFINE_STDCALL_WRAPPER(51,ITextHostImpl_TxShowDropCaret,16)
DEFINE_STDCALL_WRAPPER(52,ITextHostImpl_TxDestroyCaret,4)
DEFINE_STDCALL_WRAPPER(53,ITextHostImpl_TxGetHorzExtent,8)

const ITextHost2Vtbl text_host2_stdcall_vtbl =
{
    NULL,
    NULL,
    NULL,
    STDCALL(ITextHostImpl_TxGetDC),
    STDCALL(ITextHostImpl_TxReleaseDC),
    STDCALL(ITextHostImpl_TxShowScrollBar),
    STDCALL(ITextHostImpl_TxEnableScrollBar),
    STDCALL(ITextHostImpl_TxSetScrollRange),
    STDCALL(ITextHostImpl_TxSetScrollPos),
    STDCALL(ITextHostImpl_TxInvalidateRect),
    STDCALL(ITextHostImpl_TxViewChange),
    STDCALL(ITextHostImpl_TxCreateCaret),
    STDCALL(ITextHostImpl_TxShowCaret),
    STDCALL(ITextHostImpl_TxSetCaretPos),
    STDCALL(ITextHostImpl_TxSetTimer),
    STDCALL(ITextHostImpl_TxKillTimer),
    STDCALL(ITextHostImpl_TxScrollWindowEx),
    STDCALL(ITextHostImpl_TxSetCapture),
    STDCALL(ITextHostImpl_TxSetFocus),
    STDCALL(ITextHostImpl_TxSetCursor),
    STDCALL(ITextHostImpl_TxScreenToClient),
    STDCALL(ITextHostImpl_TxClientToScreen),
    STDCALL(ITextHostImpl_TxActivate),
    STDCALL(ITextHostImpl_TxDeactivate),
    STDCALL(ITextHostImpl_TxGetClientRect),
    STDCALL(ITextHostImpl_TxGetViewInset),
    STDCALL(ITextHostImpl_TxGetCharFormat),
    STDCALL(ITextHostImpl_TxGetParaFormat),
    STDCALL(ITextHostImpl_TxGetSysColor),
    STDCALL(ITextHostImpl_TxGetBackStyle),
    STDCALL(ITextHostImpl_TxGetMaxLength),
    STDCALL(ITextHostImpl_TxGetScrollBars),
    STDCALL(ITextHostImpl_TxGetPasswordChar),
    STDCALL(ITextHostImpl_TxGetAcceleratorPos),
    STDCALL(ITextHostImpl_TxGetExtent),
    STDCALL(ITextHostImpl_OnTxCharFormatChange),
    STDCALL(ITextHostImpl_OnTxParaFormatChange),
    STDCALL(ITextHostImpl_TxGetPropertyBits),
    STDCALL(ITextHostImpl_TxNotify),
    STDCALL(ITextHostImpl_TxImmGetContext),
    STDCALL(ITextHostImpl_TxImmReleaseContext),
    STDCALL(ITextHostImpl_TxGetSelectionBarWidth),
/* ITextHost2 */
    STDCALL(ITextHostImpl_TxIsDoubleClickPending),
    STDCALL(ITextHostImpl_TxGetWindow),
    STDCALL(ITextHostImpl_TxSetForegroundWindow),
    STDCALL(ITextHostImpl_TxGetPalette),
    STDCALL(ITextHostImpl_TxGetEastAsianFlags),
    STDCALL(ITextHostImpl_TxSetCursor2),
    STDCALL(ITextHostImpl_TxFreeTextServicesNotification),
    STDCALL(ITextHostImpl_TxGetEditStyle),
    STDCALL(ITextHostImpl_TxGetWindowStyles),
    STDCALL(ITextHostImpl_TxShowDropCaret),
    STDCALL(ITextHostImpl_TxDestroyCaret),
    STDCALL(ITextHostImpl_TxGetHorzExtent)
};

#endif /* __ASM_USE_THISCALL_WRAPPER */

static const ITextHost2Vtbl textHostVtbl =
{
    ITextHostImpl_QueryInterface,
    ITextHostImpl_AddRef,
    ITextHostImpl_Release,
    THISCALL(ITextHostImpl_TxGetDC),
    THISCALL(ITextHostImpl_TxReleaseDC),
    THISCALL(ITextHostImpl_TxShowScrollBar),
    THISCALL(ITextHostImpl_TxEnableScrollBar),
    THISCALL(ITextHostImpl_TxSetScrollRange),
    THISCALL(ITextHostImpl_TxSetScrollPos),
    THISCALL(ITextHostImpl_TxInvalidateRect),
    THISCALL(ITextHostImpl_TxViewChange),
    THISCALL(ITextHostImpl_TxCreateCaret),
    THISCALL(ITextHostImpl_TxShowCaret),
    THISCALL(ITextHostImpl_TxSetCaretPos),
    THISCALL(ITextHostImpl_TxSetTimer),
    THISCALL(ITextHostImpl_TxKillTimer),
    THISCALL(ITextHostImpl_TxScrollWindowEx),
    THISCALL(ITextHostImpl_TxSetCapture),
    THISCALL(ITextHostImpl_TxSetFocus),
    THISCALL(ITextHostImpl_TxSetCursor),
    THISCALL(ITextHostImpl_TxScreenToClient),
    THISCALL(ITextHostImpl_TxClientToScreen),
    THISCALL(ITextHostImpl_TxActivate),
    THISCALL(ITextHostImpl_TxDeactivate),
    THISCALL(ITextHostImpl_TxGetClientRect),
    THISCALL(ITextHostImpl_TxGetViewInset),
    THISCALL(ITextHostImpl_TxGetCharFormat),
    THISCALL(ITextHostImpl_TxGetParaFormat),
    THISCALL(ITextHostImpl_TxGetSysColor),
    THISCALL(ITextHostImpl_TxGetBackStyle),
    THISCALL(ITextHostImpl_TxGetMaxLength),
    THISCALL(ITextHostImpl_TxGetScrollBars),
    THISCALL(ITextHostImpl_TxGetPasswordChar),
    THISCALL(ITextHostImpl_TxGetAcceleratorPos),
    THISCALL(ITextHostImpl_TxGetExtent),
    THISCALL(ITextHostImpl_OnTxCharFormatChange),
    THISCALL(ITextHostImpl_OnTxParaFormatChange),
    THISCALL(ITextHostImpl_TxGetPropertyBits),
    THISCALL(ITextHostImpl_TxNotify),
    THISCALL(ITextHostImpl_TxImmGetContext),
    THISCALL(ITextHostImpl_TxImmReleaseContext),
    THISCALL(ITextHostImpl_TxGetSelectionBarWidth),
/* ITextHost2 */
    THISCALL(ITextHostImpl_TxIsDoubleClickPending),
    THISCALL(ITextHostImpl_TxGetWindow),
    THISCALL(ITextHostImpl_TxSetForegroundWindow),
    THISCALL(ITextHostImpl_TxGetPalette),
    THISCALL(ITextHostImpl_TxGetEastAsianFlags),
    THISCALL(ITextHostImpl_TxSetCursor2),
    THISCALL(ITextHostImpl_TxFreeTextServicesNotification),
    THISCALL(ITextHostImpl_TxGetEditStyle),
    THISCALL(ITextHostImpl_TxGetWindowStyles),
    THISCALL(ITextHostImpl_TxShowDropCaret),
    THISCALL(ITextHostImpl_TxDestroyCaret),
    THISCALL(ITextHostImpl_TxGetHorzExtent)
};

static const char * const edit_messages[] =
{
    "EM_GETSEL",           "EM_SETSEL",           "EM_GETRECT",             "EM_SETRECT",
    "EM_SETRECTNP",        "EM_SCROLL",           "EM_LINESCROLL",          "EM_SCROLLCARET",
    "EM_GETMODIFY",        "EM_SETMODIFY",        "EM_GETLINECOUNT",        "EM_LINEINDEX",
    "EM_SETHANDLE",        "EM_GETHANDLE",        "EM_GETTHUMB",            "EM_UNKNOWN_BF",
    "EM_UNKNOWN_C0",       "EM_LINELENGTH",       "EM_REPLACESEL",          "EM_UNKNOWN_C3",
    "EM_GETLINE",          "EM_LIMITTEXT",        "EM_CANUNDO",             "EM_UNDO",
    "EM_FMTLINES",         "EM_LINEFROMCHAR",     "EM_UNKNOWN_CA",          "EM_SETTABSTOPS",
    "EM_SETPASSWORDCHAR",  "EM_EMPTYUNDOBUFFER",  "EM_GETFIRSTVISIBLELINE", "EM_SETREADONLY",
    "EM_SETWORDBREAKPROC", "EM_GETWORDBREAKPROC", "EM_GETPASSWORDCHAR",     "EM_SETMARGINS",
    "EM_GETMARGINS",       "EM_GETLIMITTEXT",     "EM_POSFROMCHAR",         "EM_CHARFROMPOS",
    "EM_SETIMESTATUS",     "EM_GETIMESTATUS"
};

static const char * const richedit_messages[] =
{
    "EM_CANPASTE",         "EM_DISPLAYBAND",      "EM_EXGETSEL",            "EM_EXLIMITTEXT",
    "EM_EXLINEFROMCHAR",   "EM_EXSETSEL",         "EM_FINDTEXT",            "EM_FORMATRANGE",
    "EM_GETCHARFORMAT",    "EM_GETEVENTMASK",     "EM_GETOLEINTERFACE",     "EM_GETPARAFORMAT",
    "EM_GETSELTEXT",       "EM_HIDESELECTION",    "EM_PASTESPECIAL",        "EM_REQUESTRESIZE",
    "EM_SELECTIONTYPE",    "EM_SETBKGNDCOLOR",    "EM_SETCHARFORMAT",       "EM_SETEVENTMASK",
    "EM_SETOLECALLBACK",   "EM_SETPARAFORMAT",    "EM_SETTARGETDEVICE",     "EM_STREAMIN",
    "EM_STREAMOUT",        "EM_GETTEXTRANGE",     "EM_FINDWORDBREAK",       "EM_SETOPTIONS",
    "EM_GETOPTIONS",       "EM_FINDTEXTEX",       "EM_GETWORDBREAKPROCEX",  "EM_SETWORDBREAKPROCEX",
    "EM_SETUNDOLIMIT",     "EM_UNKNOWN_USER_83",  "EM_REDO",                "EM_CANREDO",
    "EM_GETUNDONAME",      "EM_GETREDONAME",      "EM_STOPGROUPTYPING",     "EM_SETTEXTMODE",
    "EM_GETTEXTMODE",      "EM_AUTOURLDETECT",    "EM_GETAUTOURLDETECT",    "EM_SETPALETTE",
    "EM_GETTEXTEX",        "EM_GETTEXTLENGTHEX",  "EM_SHOWSCROLLBAR",       "EM_SETTEXTEX",
    "EM_UNKNOWN_USER_98",  "EM_UNKNOWN_USER_99",  "EM_SETPUNCTUATION",      "EM_GETPUNCTUATION",
    "EM_SETWORDWRAPMODE",  "EM_GETWORDWRAPMODE",  "EM_SETIMECOLOR",         "EM_GETIMECOLOR",
    "EM_SETIMEOPTIONS",    "EM_GETIMEOPTIONS",    "EM_CONVPOSITION",        "EM_UNKNOWN_USER_109",
    "EM_UNKNOWN_USER_110", "EM_UNKNOWN_USER_111", "EM_UNKNOWN_USER_112",    "EM_UNKNOWN_USER_113",
    "EM_UNKNOWN_USER_114", "EM_UNKNOWN_USER_115", "EM_UNKNOWN_USER_116",    "EM_UNKNOWN_USER_117",
    "EM_UNKNOWN_USER_118", "EM_UNKNOWN_USER_119", "EM_SETLANGOPTIONS",      "EM_GETLANGOPTIONS",
    "EM_GETIMECOMPMODE",   "EM_FINDTEXTW",        "EM_FINDTEXTEXW",         "EM_RECONVERSION",
    "EM_SETIMEMODEBIAS",   "EM_GETIMEMODEBIAS"
};

static const char *get_msg_name( UINT msg )
{
    if (msg >= EM_GETSEL && msg <= EM_CHARFROMPOS)
        return edit_messages[msg - EM_GETSEL];
    if (msg >= EM_CANPASTE && msg <= EM_GETIMEMODEBIAS)
        return richedit_messages[msg - EM_CANPASTE];
    return "";
}

static BOOL create_windowed_editor( HWND hwnd, CREATESTRUCTW *create, BOOL emulate_10 )
{
    struct host *host = host_create( hwnd, create, emulate_10 );
    IUnknown *unk;
    HRESULT hr;

    if (!host) return FALSE;

    hr = create_text_services( NULL, (ITextHost *)&host->ITextHost_iface, &unk, emulate_10 );
    if (FAILED( hr ))
    {
        ITextHost2_Release( &host->ITextHost_iface );
        return FALSE;
    }
    IUnknown_QueryInterface( unk, &IID_ITextServices, (void **)&host->text_srv );
    IUnknown_Release( unk );

    SetWindowLongPtrW( hwnd, 0, (LONG_PTR)host );

    return TRUE;
}

static HRESULT get_lineA( ITextServices *text_srv, WPARAM wparam, LPARAM lparam, LRESULT *res )
{
    LRESULT len = USHRT_MAX;
    WORD sizeA;
    HRESULT hr;
    WCHAR *buf;

    *res = 0;
    sizeA = *(WORD *)lparam;
    *(WORD *)lparam = 0;
    if (!sizeA) return S_OK;
    buf = malloc( len * sizeof(WCHAR) );
    if (!buf) return E_OUTOFMEMORY;
    *(WORD *)buf = len;
    hr = ITextServices_TxSendMessage( text_srv, EM_GETLINE, wparam, (LPARAM)buf, &len );
    if (hr == S_OK && len)
    {
        len = WideCharToMultiByte( CP_ACP, 0, buf, len, (char *)lparam, sizeA, NULL, NULL );
        if (!len && GetLastError() == ERROR_INSUFFICIENT_BUFFER) len = sizeA;
        if (len < sizeA) ((char *)lparam)[len] = '\0';
        *res = len;
    }
    free( buf );
    return hr;
}

static HRESULT get_text_rangeA( struct host *host, TEXTRANGEA *rangeA, LRESULT *res )
{
    TEXTRANGEW range;
    HRESULT hr;
    unsigned int count;
    LRESULT len;

    *res = 0;
    if (rangeA->chrg.cpMin < 0) return S_OK;
    ITextServices_TxSendMessage( host->text_srv, WM_GETTEXTLENGTH, 0, 0, &len );
    range.chrg = rangeA->chrg;
    if ((range.chrg.cpMin == 0 && range.chrg.cpMax == -1) || range.chrg.cpMax > len)
        range.chrg.cpMax = len;
    if (range.chrg.cpMin >= range.chrg.cpMax) return S_OK;
    count = range.chrg.cpMax - range.chrg.cpMin + 1;
    range.lpstrText = malloc( count * sizeof(WCHAR) );
    if (!range.lpstrText) return E_OUTOFMEMORY;
    hr = ITextServices_TxSendMessage( host->text_srv, EM_GETTEXTRANGE, 0, (LPARAM)&range, &len );
    if (hr == S_OK && len)
    {
        if (!host->emulate_10) count = INT_MAX;
        len = WideCharToMultiByte( CP_ACP, 0, range.lpstrText, -1, rangeA->lpstrText, count, NULL, NULL );
        if (!host->emulate_10) *res = len - 1;
        else
        {
            *res = count - 1;
            rangeA->lpstrText[*res] = '\0';
        }
    }
    free( range.lpstrText );
    return hr;
}

static HRESULT set_options( struct host *host, DWORD op, DWORD value, LRESULT *res )
{
    DWORD style, old_options, new_options, change, props_mask = 0;
    DWORD mask = ECO_AUTOWORDSELECTION | ECO_AUTOVSCROLL | ECO_AUTOHSCROLL | ECO_NOHIDESEL | ECO_READONLY |
        ECO_WANTRETURN | ECO_SAVESEL | ECO_SELECTIONBAR | ECO_VERTICAL;

    new_options = old_options = SendMessageW( host->window, EM_GETOPTIONS, 0, 0 );

    switch (op)
    {
    case ECOOP_SET:
        new_options = value;
        break;
    case ECOOP_OR:
        new_options |= value;
        break;
    case ECOOP_AND:
        new_options &= value;
        break;
    case ECOOP_XOR:
        new_options ^= value;
    }
    new_options &= mask;

    change = (new_options ^ old_options);

    if (change & ECO_AUTOWORDSELECTION)
    {
        host->props ^= TXTBIT_AUTOWORDSEL;
        props_mask |= TXTBIT_AUTOWORDSEL;
    }
    if (change & ECO_AUTOVSCROLL)
    {
        host->scrollbars ^= WS_VSCROLL;
        props_mask |= TXTBIT_SCROLLBARCHANGE;
    }
    if (change & ECO_AUTOHSCROLL)
    {
        host->scrollbars ^= WS_HSCROLL;
        props_mask |= TXTBIT_SCROLLBARCHANGE;
    }
    if (change & ECO_NOHIDESEL)
    {
        host->props ^= TXTBIT_HIDESELECTION;
        props_mask |= TXTBIT_HIDESELECTION;
    }
    if (change & ECO_READONLY)
    {
        host->props ^= TXTBIT_READONLY;
        props_mask |= TXTBIT_READONLY;
    }
    if (change & ECO_SAVESEL)
    {
        host->props ^= TXTBIT_SAVESELECTION;
        props_mask |= TXTBIT_SAVESELECTION;
    }
    if (change & ECO_SELECTIONBAR)
    {
        host->sel_bar ^= 1;
        props_mask |= TXTBIT_SELBARCHANGE;
        if (host->use_set_rect)
        {
            int width = SELECTIONBAR_WIDTH;
            host->set_rect.left += host->sel_bar ? width : -width;
            props_mask |= TXTBIT_CLIENTRECTCHANGE;
        }
    }
    if (change & ECO_VERTICAL)
    {
        host->props ^= TXTBIT_VERTICAL;
        props_mask |= TXTBIT_VERTICAL;
    }
    if (change & ECO_WANTRETURN) host->want_return ^= 1;

    if (props_mask)
        ITextServices_OnTxPropertyBitsChange( host->text_srv, props_mask, host->props & props_mask );

    *res = new_options;

    mask &= ~ECO_AUTOWORDSELECTION; /* doesn't correspond to a window style */
    style = GetWindowLongW( host->window, GWL_STYLE );
    style = (style & ~mask) | (*res & mask);
    SetWindowLongW( host->window, GWL_STYLE, style );
    return S_OK;
}

/* handle dialog mode VK_RETURN. Returns TRUE if message has been processed */
static BOOL handle_dialog_enter( struct host *host )
{
    BOOL ctrl_is_down = GetKeyState( VK_CONTROL ) & 0x8000;

    if (ctrl_is_down) return TRUE;

    if (host->want_return) return FALSE;

    if (host->parent)
    {
        DWORD id = SendMessageW( host->parent, DM_GETDEFID, 0, 0 );
        if (HIWORD( id ) == DC_HASDEFID)
        {
            HWND ctrl = GetDlgItem( host->parent, LOWORD( id ));
            if (ctrl)
            {
                SendMessageW( host->parent, WM_NEXTDLGCTL, (WPARAM)ctrl, TRUE );
                PostMessageW( ctrl, WM_KEYDOWN, VK_RETURN, 0 );
            }
        }
    }
    return TRUE;
}

static LRESULT send_msg_filter( struct host *host, UINT msg, WPARAM *wparam, LPARAM *lparam )
{
    MSGFILTER msgf;
    LRESULT res;

    if (!host->parent) return 0;
    msgf.nmhdr.hwndFrom = host->window;
    msgf.nmhdr.idFrom = GetWindowLongW( host->window, GWLP_ID );
    msgf.nmhdr.code = EN_MSGFILTER;
    msgf.msg = msg;
    msgf.wParam = *wparam;
    msgf.lParam = *lparam;
    if ((res = SendMessageW( host->parent, WM_NOTIFY, msgf.nmhdr.idFrom, (LPARAM)&msgf )))
        return res;
    *wparam = msgf.wParam;
    *lparam = msgf.lParam;

    return 0;
}

static LRESULT RichEditWndProc_common( HWND hwnd, UINT msg, WPARAM wparam,
                                       LPARAM lparam, BOOL unicode )
{
    struct host *host;
    HRESULT hr = S_OK;
    LRESULT res = 0;

    TRACE( "enter hwnd %p msg %04x (%s) %Ix %Ix, unicode %d\n",
           hwnd, msg, get_msg_name(msg), wparam, lparam, unicode );

    host = (struct host *)GetWindowLongPtrW( hwnd, 0 );
    if (!host)
    {
        if (msg == WM_NCCREATE)
        {
            CREATESTRUCTW *pcs = (CREATESTRUCTW *)lparam;

            TRACE( "WM_NCCREATE: hwnd %p style 0x%08lx\n", hwnd, pcs->style );
            return create_windowed_editor( hwnd, pcs, FALSE );
        }
        else return DefWindowProcW( hwnd, msg, wparam, lparam );
    }

    if ((((host->event_mask & ENM_KEYEVENTS) && msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ||
         ((host->event_mask & ENM_MOUSEEVENTS) && msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ||
         ((host->event_mask & ENM_SCROLLEVENTS) && msg >= WM_HSCROLL && msg <= WM_VSCROLL)))
    {
        host->notify_level++;
        res = send_msg_filter( host, msg, &wparam, &lparam );
        if (!--host->notify_level && host->defer_release)
        {
            TRACE( "exit (filtered deferred release) hwnd %p msg %04x (%s) %Ix %Ix -> 0\n",
                   hwnd, msg, get_msg_name(msg), wparam, lparam );
            ITextHost2_Release( &host->ITextHost_iface );
            return 0;
        }

        if (res)
        {
            TRACE( "exit (filtered %Iu) hwnd %p msg %04x (%s) %Ix %Ix -> 0\n",
                   res, hwnd, msg, get_msg_name(msg), wparam, lparam );
            return 0;
        }
    }

    switch (msg)
    {
    case WM_CHAR:
    {
        WCHAR wc = wparam;

        if (!unicode) MultiByteToWideChar( CP_ACP, 0, (char *)&wparam, 1, &wc, 1 );
        if (wparam == '\r' && host->dialog_mode && host->emulate_10 && handle_dialog_enter( host )) break;
        hr = ITextServices_TxSendMessage( host->text_srv, msg, wc, lparam, &res );
        break;
    }

    case WM_CREATE:
    {
        CREATESTRUCTW *createW = (CREATESTRUCTW *)lparam;
        CREATESTRUCTA *createA = (CREATESTRUCTA *)lparam;
        void *text;
        WCHAR *textW = NULL;
        LONG codepage = unicode ? CP_UNICODE : CP_ACP;
        int len;
        LRESULT evmask;

        ITextServices_OnTxInPlaceActivate( host->text_srv, NULL );

        if (lparam)
        {
            text = unicode ? (void *)createW->lpszName : (void *)createA->lpszName;
            textW = ME_ToUnicode( codepage, text, &len );
        }
        ITextServices_TxSendMessage( host->text_srv, EM_GETEVENTMASK, 0, 0, &evmask );
        ITextServices_TxSendMessage( host->text_srv, EM_SETEVENTMASK, 0, evmask & ~ENM_CHANGE, &evmask );
        ITextServices_TxSetText( host->text_srv, textW );
        ITextServices_TxSendMessage( host->text_srv, EM_SETEVENTMASK, 0, evmask, NULL );
        if (lparam) ME_EndToUnicode( codepage, textW );
        break;
    }
    case WM_DESTROY:
        if (!host->notify_level) ITextHost2_Release( &host->ITextHost_iface );
        else host->defer_release = 1;
        return 0;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wparam;
        RECT rc;
        HBRUSH brush;

        if (GetUpdateRect( hwnd, &rc, TRUE ))
        {
            brush = CreateSolidBrush( ITextHost_TxGetSysColor( &host->ITextHost_iface, COLOR_WINDOW ) );
            FillRect( hdc, &rc, brush );
            DeleteObject( brush );
        }
        return 1;
    }
    case EM_FINDTEXT:
    {
        FINDTEXTW *params = (FINDTEXTW *)lparam;
        FINDTEXTW new_params;
        int len;

        if (!unicode)
        {
            new_params.chrg = params->chrg;
            new_params.lpstrText = ME_ToUnicode( CP_ACP, (char *)params->lpstrText, &len );
            params = &new_params;
        }
        hr = ITextServices_TxSendMessage( host->text_srv, EM_FINDTEXTW, wparam, (LPARAM)params, &res );
        if (!unicode) ME_EndToUnicode( CP_ACP, (WCHAR *)new_params.lpstrText );
        break;
    }
    case EM_FINDTEXTEX:
    {
        FINDTEXTEXA *paramsA = (FINDTEXTEXA *)lparam;
        FINDTEXTEXW *params = (FINDTEXTEXW *)lparam;
        FINDTEXTEXW new_params;
        int len;

        if (!unicode)
        {
            new_params.chrg = params->chrg;
            new_params.lpstrText = ME_ToUnicode( CP_ACP, (char *)params->lpstrText, &len );
            params = &new_params;
        }
        hr = ITextServices_TxSendMessage( host->text_srv, EM_FINDTEXTEXW, wparam, (LPARAM)params, &res );
        if (!unicode)
        {
            ME_EndToUnicode( CP_ACP, (WCHAR *)new_params.lpstrText );
            paramsA->chrgText = params->chrgText;
        }
        break;
    }
    case WM_GETDLGCODE:
        if (lparam) host->dialog_mode = TRUE;

        res = DLGC_WANTCHARS | DLGC_WANTTAB | DLGC_WANTARROWS;
        if (host->props & TXTBIT_MULTILINE) res |= DLGC_WANTMESSAGE;
        if (!(host->props & TXTBIT_SAVESELECTION)) res |= DLGC_HASSETSEL;
        break;

    case EM_GETLINE:
        if (unicode) hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, lparam, &res );
        else hr = get_lineA( host->text_srv, wparam, lparam, &res );
        break;

    case EM_GETPASSWORDCHAR:
        ITextHost_TxGetPasswordChar( &host->ITextHost_iface, (WCHAR *)&res );
        break;

    case EM_GETRECT:
        hr = ITextHost_TxGetClientRect( &host->ITextHost_iface, (RECT *)lparam );
        break;

    case EM_GETSELTEXT:
    {
        TEXTRANGEA range;

        if (unicode) hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, lparam, &res );
        else
        {
            ITextServices_TxSendMessage( host->text_srv, EM_EXGETSEL, 0, (LPARAM)&range.chrg, &res );
            range.lpstrText = (char *)lparam;
            range.lpstrText[0] = '\0';
            hr = get_text_rangeA( host, &range, &res );
        }
        break;
    }
    case EM_GETOPTIONS:
        if (host->props & TXTBIT_READONLY) res |= ECO_READONLY;
        if (!(host->props & TXTBIT_HIDESELECTION)) res |= ECO_NOHIDESEL;
        if (host->props & TXTBIT_SAVESELECTION) res |= ECO_SAVESEL;
        if (host->props & TXTBIT_AUTOWORDSEL) res |= ECO_AUTOWORDSELECTION;
        if (host->props & TXTBIT_VERTICAL) res |= ECO_VERTICAL;
        if (host->scrollbars & ES_AUTOHSCROLL) res |= ECO_AUTOHSCROLL;
        if (host->scrollbars & ES_AUTOVSCROLL) res |= ECO_AUTOVSCROLL;
        if (host->want_return) res |= ECO_WANTRETURN;
        if (host->sel_bar) res |= ECO_SELECTIONBAR;
        break;

    case WM_GETTEXT:
    {
        GETTEXTEX params;

        params.cb = wparam * (unicode ? sizeof(WCHAR) : sizeof(CHAR));
        params.flags = GT_USECRLF;
        params.codepage = unicode ? CP_UNICODE : CP_ACP;
        params.lpDefaultChar = NULL;
        params.lpUsedDefChar = NULL;
        hr = ITextServices_TxSendMessage( host->text_srv, EM_GETTEXTEX, (WPARAM)&params, lparam, &res );
        break;
    }
    case WM_GETTEXTLENGTH:
    {
        GETTEXTLENGTHEX params;

        params.flags = GTL_CLOSE | (host->emulate_10 ? 0 : GTL_USECRLF) | GTL_NUMCHARS;
        params.codepage = unicode ? CP_UNICODE : CP_ACP;
        hr = ITextServices_TxSendMessage( host->text_srv, EM_GETTEXTLENGTHEX, (WPARAM)&params, 0, &res );
        break;
    }
    case EM_GETTEXTRANGE:
        if (unicode) hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, lparam, &res );
        else hr = get_text_rangeA( host, (TEXTRANGEA *)lparam, &res );
        break;

    case WM_KEYDOWN:
        switch (LOWORD( wparam ))
        {
        case VK_ESCAPE:
            if (host->dialog_mode && host->parent)
                PostMessageW( host->parent, WM_CLOSE, 0, 0 );
            break;
        case VK_TAB:
            if (host->dialog_mode && host->parent)
                SendMessageW( host->parent, WM_NEXTDLGCTL, GetKeyState( VK_SHIFT ) & 0x8000, 0 );
            break;
        case VK_RETURN:
            if (host->dialog_mode && !host->emulate_10 && handle_dialog_enter( host )) break;
            /* fall through */
        default:
            hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, lparam, &res );
        }
        break;

    case WM_PAINT:
    case WM_PRINTCLIENT:
    {
        HDC hdc;
        RECT rc, client, update;
        PAINTSTRUCT ps;
        HBRUSH brush, old_brush;
        LONG view_id;

        ITextHost_TxGetClientRect( &host->ITextHost_iface, &client );

        if (msg == WM_PAINT)
        {
            /* TODO retrieve if the text document is frozen */
            hdc = BeginPaint( hwnd, &ps );
            update = ps.rcPaint;
            view_id = TXTVIEW_ACTIVE;
        }
        else
        {
            hdc = (HDC)wparam;
            update = client;
            view_id = TXTVIEW_INACTIVE;
        }

        brush = CreateSolidBrush( ITextHost_TxGetSysColor( &host->ITextHost_iface, COLOR_WINDOW ) );
        old_brush = SelectObject( hdc, brush );

        /* Erase area outside of the formatting rectangle */
        if (update.top < client.top)
        {
            rc = update;
            rc.bottom = client.top;
            PatBlt( hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            update.top = client.top;
        }
        if (update.bottom > client.bottom)
        {
            rc = update;
            rc.top = client.bottom;
            PatBlt( hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            update.bottom = client.bottom;
        }
        if (update.left < client.left)
        {
            rc = update;
            rc.right = client.left;
            PatBlt( hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            update.left = client.left;
        }
        if (update.right > client.right)
        {
            rc = update;
            rc.left = client.right;
            PatBlt( hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY );
            update.right = client.right;
        }

        ITextServices_TxDraw( host->text_srv, DVASPECT_CONTENT, 0, NULL, NULL, hdc, NULL, NULL, NULL,
                              &update, NULL, 0, view_id );
        SelectObject( hdc, old_brush );
        DeleteObject( brush );
        if (msg == WM_PAINT) EndPaint( hwnd, &ps );
        return 0;
    }
    case EM_REPLACESEL:
    {
        int len;
        LONG codepage = unicode ? CP_UNICODE : CP_ACP;
        WCHAR *text = ME_ToUnicode( codepage, (void *)lparam, &len );

        hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, (LPARAM)text, &res );
        ME_EndToUnicode( codepage, text );
        res = len;
        break;
    }
    case EM_SETBKGNDCOLOR:
        res = ITextHost_TxGetSysColor( &host->ITextHost_iface, COLOR_WINDOW );
        host->use_back_colour = !wparam;
        if (host->use_back_colour) host->back_colour = lparam;
        InvalidateRect( hwnd, NULL, TRUE );
        break;

    case WM_SETCURSOR:
    {
        POINT pos;
        RECT rect;

        if (hwnd != (HWND)wparam) break;
        GetCursorPos( &pos );
        ScreenToClient( hwnd, &pos );
        ITextHost_TxGetClientRect( &host->ITextHost_iface, &rect );
        if (PtInRect( &rect, pos ))
            ITextServices_OnTxSetCursor( host->text_srv, DVASPECT_CONTENT, 0, NULL, NULL, NULL, NULL, NULL, pos.x, pos.y );
        else ITextHost_TxSetCursor( &host->ITextHost_iface, LoadCursorW( NULL, MAKEINTRESOURCEW( IDC_ARROW ) ), FALSE );
        break;
    }
    case EM_SETEVENTMASK:
        host->event_mask = lparam;
        hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, lparam, &res );
        break;

    case EM_SETOPTIONS:
        hr = set_options( host, wparam, lparam, &res );
        break;

    case EM_SETPASSWORDCHAR:
        if (wparam == host->password_char) break;
        host->password_char = wparam;
        if (wparam) host->props |= TXTBIT_USEPASSWORD;
        else host->props &= ~TXTBIT_USEPASSWORD;
        ITextServices_OnTxPropertyBitsChange( host->text_srv, TXTBIT_USEPASSWORD, host->props & TXTBIT_USEPASSWORD );
        break;

    case EM_SETREADONLY:
    {
        DWORD op = wparam ? ECOOP_OR : ECOOP_AND;
        DWORD mask = wparam ? ECO_READONLY : ~ECO_READONLY;

        SendMessageW( hwnd, EM_SETOPTIONS, op, mask );
        return 1;
    }
    case EM_SETRECT:
    case EM_SETRECTNP:
    {
        RECT *rc = (RECT *)lparam;

        if (!rc) host->use_set_rect = 0;
        else
        {
            if (wparam >= 2) break;
            host->set_rect = *rc;
            if (host->client_edge)
            {
                InflateRect( &host->set_rect, 1, 0 );
                host->set_rect.top -= 1;
            }
            if (!wparam) IntersectRect( &host->set_rect, &host->set_rect, &host->client_rect );
            host->use_set_rect = 1;
        }
        ITextServices_OnTxPropertyBitsChange( host->text_srv, TXTBIT_CLIENTRECTCHANGE, 0 );
        break;
    }
    case WM_SETTEXT:
    {
        char *textA = (char *)lparam;
        WCHAR *text = (WCHAR *)lparam;
        int len;

        if (!unicode && textA && strncmp( textA, "{\\rtf", 5 ) && strncmp( textA, "{\\urtf", 6 ))
            text = ME_ToUnicode( CP_ACP, textA, &len );
        hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, (LPARAM)text, &res );
        if (text != (WCHAR *)lparam) ME_EndToUnicode( CP_ACP, text );
        break;
    }
    case EM_SHOWSCROLLBAR:
    {
        DWORD mask = 0, new;

        if (wparam == SB_HORZ) mask = WS_HSCROLL;
        else if (wparam == SB_VERT) mask = WS_VSCROLL;
        else if (wparam == SB_BOTH) mask = WS_HSCROLL | WS_VSCROLL;

        if (mask)
        {
            new = lparam ? mask : 0;
            if ((host->scrollbars & mask) != new)
            {
                host->scrollbars &= ~mask;
                host->scrollbars |= new;
                ITextServices_OnTxPropertyBitsChange( host->text_srv, TXTBIT_SCROLLBARCHANGE, 0 );
            }
        }

        res = 0;
        break;
    }
    case WM_WINDOWPOSCHANGED:
    {
        RECT client;
        WINDOWPOS *winpos = (WINDOWPOS *)lparam;

        hr = S_FALSE; /* call defwndproc */
        if (winpos->flags & SWP_NOCLIENTSIZE) break;
        GetClientRect( hwnd, &client );
        if (host->use_set_rect)
        {
            host->set_rect.right += client.right - host->client_rect.right;
            host->set_rect.bottom += client.bottom - host->client_rect.bottom;
        }
        host->client_rect = client;
        ITextServices_OnTxPropertyBitsChange( host->text_srv, TXTBIT_CLIENTRECTCHANGE, 0 );
        break;
    }
    default:
        hr = ITextServices_TxSendMessage( host->text_srv, msg, wparam, lparam, &res );
    }

    if (hr == S_FALSE)
        res = DefWindowProcW( hwnd, msg, wparam, lparam );

    TRACE( "exit hwnd %p msg %04x (%s) %Ix %Ix, unicode %d -> %Iu\n",
           hwnd, msg, get_msg_name(msg), wparam, lparam, unicode, res );

    return res;
}

static LRESULT WINAPI RichEditWndProcW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    BOOL unicode = TRUE;

    /* Under Win9x RichEdit20W returns ANSI strings, see the tests. */
    if (msg == WM_GETTEXT && (GetVersion() & 0x80000000))
        unicode = FALSE;

    return RichEditWndProc_common( hwnd, msg, wparam, lparam, unicode );
}

static LRESULT WINAPI RichEditWndProcA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return RichEditWndProc_common( hwnd, msg, wparam, lparam, FALSE );
}

/******************************************************************
 *        RichEditANSIWndProc (RICHED20.10)
 */
LRESULT WINAPI RichEditANSIWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return RichEditWndProcA( hwnd, msg, wparam, lparam );
}

/******************************************************************
 *        RichEdit10ANSIWndProc (RICHED20.9)
 */
LRESULT WINAPI RichEdit10ANSIWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_NCCREATE && !GetWindowLongPtrW( hwnd, 0 ))
    {
        CREATESTRUCTW *pcs = (CREATESTRUCTW *)lparam;

        TRACE( "WM_NCCREATE: hwnd %p style 0x%08lx\n", hwnd, pcs->style );
        return create_windowed_editor( hwnd, pcs, TRUE );
    }
    return RichEditANSIWndProc( hwnd, msg, wparam, lparam );
}

static LRESULT WINAPI REComboWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    /* FIXME: Not implemented */
    TRACE( "hwnd %p msg %04x (%s) %08Ix %08Ix\n",
           hwnd, msg, get_msg_name( msg ), wparam, lparam );
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static LRESULT WINAPI REListWndProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    /* FIXME: Not implemented */
    TRACE( "hwnd %p msg %04x (%s) %08Ix %08Ix\n",
           hwnd, msg, get_msg_name( msg ), wparam, lparam );
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

/******************************************************************
 *        REExtendedRegisterClass (RICHED20.8)
 *
 * FIXME undocumented
 * Need to check for errors and implement controls and callbacks
 */
LRESULT WINAPI REExtendedRegisterClass( void )
{
    WNDCLASSW wc;
    UINT result;

    FIXME( "semi stub\n" );
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;

    if (!listbox_registered)
    {
        wc.style = CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS;
        wc.lpfnWndProc = REListWndProc;
        wc.lpszClassName = L"REListBox20W";
        if (RegisterClassW( &wc )) listbox_registered = TRUE;
    }

    if (!combobox_registered)
    {
        wc.style = CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
        wc.lpfnWndProc = REComboWndProc;
        wc.lpszClassName = L"REComboBox20W";
        if (RegisterClassW( &wc )) combobox_registered = TRUE;
    }

    result = 0;
    if (listbox_registered) result += 1;
    if (combobox_registered) result += 2;

    return result;
}

static BOOL register_classes( HINSTANCE instance )
{
    WNDCLASSW wcW;
    WNDCLASSA wcA;

    wcW.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wcW.lpfnWndProc = RichEditWndProcW;
    wcW.cbClsExtra = 0;
    wcW.cbWndExtra = sizeof(struct host *);
    wcW.hInstance = NULL; /* hInstance would register DLL-local class */
    wcW.hIcon = NULL;
    wcW.hCursor = LoadCursorW( NULL, (LPWSTR)IDC_IBEAM );
    wcW.hbrBackground = GetStockObject( NULL_BRUSH );
    wcW.lpszMenuName = NULL;

    if (!(GetVersion() & 0x80000000))
    {
        wcW.lpszClassName = RICHEDIT_CLASS20W;
        if (!RegisterClassW( &wcW )) return FALSE;
        wcW.lpszClassName = MSFTEDIT_CLASS;
        if (!RegisterClassW( &wcW )) return FALSE;
    }
    else
    {
        /* WNDCLASSA/W have the same layout */
        wcW.lpszClassName = (LPCWSTR)"RichEdit20W";
        if (!RegisterClassA( (WNDCLASSA *)&wcW )) return FALSE;
        wcW.lpszClassName = (LPCWSTR)"RichEdit50W";
        if (!RegisterClassA( (WNDCLASSA *)&wcW )) return FALSE;
    }

    wcA.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wcA.lpfnWndProc = RichEditWndProcA;
    wcA.cbClsExtra = 0;
    wcA.cbWndExtra = sizeof(struct host *);
    wcA.hInstance = NULL; /* hInstance would register DLL-local class */
    wcA.hIcon = NULL;
    wcA.hCursor = LoadCursorW( NULL, (LPWSTR)IDC_IBEAM );
    wcA.hbrBackground = GetStockObject(NULL_BRUSH);
    wcA.lpszMenuName = NULL;
    wcA.lpszClassName = RICHEDIT_CLASS20A;
    if (!RegisterClassA( &wcA )) return FALSE;
    wcA.lpszClassName = "RichEdit50A";
    if (!RegisterClassA( &wcA )) return FALSE;

    return TRUE;
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        dll_instance = instance;
        DisableThreadLibraryCalls( instance );
        if (!register_classes( instance )) return FALSE;
        LookupInit();
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        UnregisterClassW( RICHEDIT_CLASS20W, 0 );
        UnregisterClassW( MSFTEDIT_CLASS, 0 );
        UnregisterClassA( RICHEDIT_CLASS20A, 0 );
        UnregisterClassA( "RichEdit50A", 0 );
        if (listbox_registered) UnregisterClassW( L"REListBox20W", 0 );
        if (combobox_registered) UnregisterClassW( L"REComboBox20W", 0 );
        LookupCleanup();
        release_typelib();
        break;
    }
    return TRUE;
}
