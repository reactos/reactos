/*
 * RichEdit - functions and interfaces around CreateTextServices
 *
 * Copyright 2005, 2006, Maarten Lankhorst
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
#include "oleauto.h"
#include "richole.h"
#include "tom.h"
#include "imm.h"
#include "textserv.h"
#include "wine/debug.h"
#include "editstr.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

static inline struct text_services *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD( iface, struct text_services, IUnknown_inner );
}

static HRESULT WINAPI ITextServicesImpl_QueryInterface( IUnknown *iface, REFIID iid, void **obj )
{
    struct text_services *services = impl_from_IUnknown( iface );

    TRACE( "(%p)->(%s, %p)\n", iface, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown )) *obj = &services->IUnknown_inner;
    else if (IsEqualIID( iid, &IID_ITextServices )) *obj = &services->ITextServices_iface;
    else if (IsEqualIID( iid, &IID_IRichEditOle )) *obj= &services->IRichEditOle_iface;
    else if (IsEqualIID( iid, &IID_IDispatch ) ||
             IsEqualIID( iid, &IID_ITextDocument ) ||
             IsEqualIID( iid, &IID_ITextDocument2Old )) *obj = &services->ITextDocument2Old_iface;
    else
    {
        *obj = NULL;
        FIXME( "Unknown interface: %s\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*obj );
    return S_OK;
}

static ULONG WINAPI ITextServicesImpl_AddRef(IUnknown *iface)
{
    struct text_services *services = impl_from_IUnknown( iface );
    LONG ref = InterlockedIncrement( &services->ref );

    TRACE( "(%p) ref = %d\n", services, ref );

    return ref;
}

static ULONG WINAPI ITextServicesImpl_Release(IUnknown *iface)
{
    struct text_services *services = impl_from_IUnknown( iface );
    LONG ref = InterlockedDecrement( &services->ref );

    TRACE( "(%p) ref = %d\n", services, ref );

    if (!ref)
    {
        richole_release_children( services );
        ME_DestroyEditor( services->editor );
        CoTaskMemFree( services );
    }
    return ref;
}

static const IUnknownVtbl textservices_inner_vtbl =
{
   ITextServicesImpl_QueryInterface,
   ITextServicesImpl_AddRef,
   ITextServicesImpl_Release
};

static inline struct text_services *impl_from_ITextServices( ITextServices *iface )
{
    return CONTAINING_RECORD( iface, struct text_services, ITextServices_iface );
}

static HRESULT WINAPI fnTextSrv_QueryInterface( ITextServices *iface, REFIID iid, void **obj )
{
    struct text_services *services = impl_from_ITextServices( iface );
    return IUnknown_QueryInterface( services->outer_unk, iid, obj );
}

static ULONG WINAPI fnTextSrv_AddRef(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );
    return IUnknown_AddRef( services->outer_unk );
}

static ULONG WINAPI fnTextSrv_Release(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );
    return IUnknown_Release( services->outer_unk );
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSendMessage,20)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxSendMessage( ITextServices *iface, UINT msg, WPARAM wparam,
                                                            LPARAM lparam, LRESULT *result )
{
    struct text_services *services = impl_from_ITextServices( iface );
    HRESULT hr;
    LRESULT res;

    res = editor_handle_message( services->editor, msg, wparam, lparam, &hr );
    if (result) *result = res;
    return hr;
}

static HRESULT update_client_rect( struct text_services *services, const RECT *client )
{
    RECT rect;
    HRESULT hr;

    if (!client)
    {
        if (!services->editor->in_place_active) return E_INVALIDARG;
        hr = ITextHost_TxGetClientRect( services->editor->texthost, &rect );
        if (FAILED( hr )) return hr;
    }
    else rect = *client;

    rect.left += services->editor->selofs;

    if (EqualRect( &rect, &services->editor->rcFormat )) return S_FALSE;
    services->editor->rcFormat = rect;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxDraw,52)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxDraw( ITextServices *iface, DWORD aspect, LONG index, void *aspect_info,
                                                     DVTARGETDEVICE *td, HDC draw, HDC target,
                                                     const RECTL *bounds, const RECTL *mf_bounds, RECT *update,
                                                     BOOL (CALLBACK *continue_fn)(DWORD), DWORD continue_param,
                                                     LONG view_id )
{
    struct text_services *services = impl_from_ITextServices( iface );
    HRESULT hr;
    HDC dc = draw;
    BOOL rewrap = FALSE;

    TRACE( "%p: aspect %d, %d, %p, %p, draw %p, target %p, bounds %s, mf_bounds %s, update %s, %p, %d, view %d\n",
           services, aspect, index, aspect_info, td, draw, target, wine_dbgstr_rect( (RECT *)bounds ),
           wine_dbgstr_rect( (RECT *)mf_bounds ), wine_dbgstr_rect( update ), continue_fn, continue_param, view_id );

    if (aspect != DVASPECT_CONTENT || aspect_info || td || target || mf_bounds || continue_fn )
        FIXME( "Many arguments are ignored\n" );

    hr = update_client_rect( services, (RECT *)bounds );
    if (FAILED( hr )) return hr;
    if (hr == S_OK) rewrap = TRUE;

    if (!dc && services->editor->in_place_active)
        dc = ITextHost_TxGetDC( services->editor->texthost );
    if (!dc) return E_FAIL;

    if (rewrap)
    {
        editor_mark_rewrap_all( services->editor );
        wrap_marked_paras_dc( services->editor, dc, FALSE );
    }

    if (!services->editor->bEmulateVersion10 || services->editor->nEventMask & ENM_UPDATE)
        ITextHost_TxNotify( services->editor->texthost, EN_UPDATE, NULL );

    editor_draw( services->editor, dc, update );

    if (!draw) ITextHost_TxReleaseDC( services->editor->texthost, dc );
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetHScroll,24)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetHScroll( ITextServices *iface, LONG *min_pos, LONG *max_pos, LONG *pos,
                                                           LONG *page, BOOL *enabled )
{
    struct text_services *services = impl_from_ITextServices( iface );

    if (min_pos) *min_pos = services->editor->horz_si.nMin;
    if (max_pos) *max_pos = services->editor->horz_si.nMax;
    if (pos) *pos = services->editor->horz_si.nPos;
    if (page) *page = services->editor->horz_si.nPage;
    if (enabled) *enabled = services->editor->horz_sb_enabled;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetVScroll,24)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetVScroll( ITextServices *iface, LONG *min_pos, LONG *max_pos, LONG *pos,
                                                           LONG *page, BOOL *enabled )
{
    struct text_services *services = impl_from_ITextServices( iface );

    if (min_pos) *min_pos = services->editor->vert_si.nMin;
    if (max_pos) *max_pos = services->editor->vert_si.nMax;
    if (pos) *pos = services->editor->vert_si.nPos;
    if (page) *page = services->editor->vert_si.nPage;
    if (enabled) *enabled = services->editor->vert_sb_enabled;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxSetCursor,40)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxSetCursor( ITextServices *iface, DWORD aspect, LONG index,
                                                            void *aspect_info, DVTARGETDEVICE *td, HDC draw,
                                                            HDC target, const RECT *client, INT x, INT y )
{
    struct text_services *services = impl_from_ITextServices( iface );

    TRACE( "%p: %d, %d, %p, %p, draw %p target %p client %s pos (%d, %d)\n", services, aspect, index, aspect_info, td, draw,
           target, wine_dbgstr_rect( client ), x, y );

    if (aspect != DVASPECT_CONTENT || index || aspect_info || td || draw || target || client)
        FIXME( "Ignoring most params\n" );

    link_notify( services->editor, WM_SETCURSOR, 0, MAKELPARAM( x, y ) );
    editor_set_cursor( services->editor, x, y );
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxQueryHitPoint,44)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxQueryHitPoint(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                                             void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw,
                                                             HDC hicTargetDev, LPCRECT lprcClient, INT x, INT y,
                                                             DWORD *pHitResult)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInPlaceActivate,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxInPlaceActivate( ITextServices *iface, const RECT *client )
{
    struct text_services *services = impl_from_ITextServices( iface );
    HRESULT hr;
    BOOL old_active = services->editor->in_place_active;

    TRACE( "%p: %s\n", services, wine_dbgstr_rect( client ) );

    services->editor->in_place_active = TRUE;
    hr = update_client_rect( services, client );
    if (FAILED( hr ))
    {
        services->editor->in_place_active = old_active;
        return hr;
    }
    ME_RewrapRepaint( services->editor );
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxInPlaceDeactivate,4)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxInPlaceDeactivate(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );

    TRACE( "%p\n", services );
    services->editor->in_place_active = FALSE;
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIActivate,4)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxUIActivate(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxUIDeactivate,4)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxUIDeactivate(ITextServices *iface)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetText,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetText( ITextServices *iface, BSTR *text )
{
    struct text_services *services = impl_from_ITextServices( iface );
    int length;

    length = ME_GetTextLength( services->editor );
    if (length)
    {
        ME_Cursor start;
        BSTR bstr;
        bstr = SysAllocStringByteLen( NULL, length * sizeof(WCHAR) );
        if (bstr == NULL) return E_OUTOFMEMORY;

        cursor_from_char_ofs( services->editor, 0, &start );
        ME_GetTextW( services->editor, bstr, length, &start, INT_MAX, FALSE, FALSE );
        *text = bstr;
    }
    else *text = NULL;

    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxSetText,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxSetText( ITextServices *iface, const WCHAR *text )
{
    struct text_services *services = impl_from_ITextServices( iface );
    ME_Cursor cursor;

    ME_SetCursorToStart( services->editor, &cursor );
    ME_InternalDeleteText( services->editor, &cursor, ME_GetTextLength( services->editor ), FALSE );
    if (text) ME_InsertTextFromCursor( services->editor, 0, text, -1, services->editor->pBuffer->pDefaultStyle );
    set_selection_cursors( services->editor, 0, 0);
    services->editor->nModifyStep = 0;
    OleFlushClipboard();
    ME_EmptyUndoStack( services->editor );
    ME_UpdateRepaint( services->editor, FALSE );

    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCurTargetX,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetCurTargetX(ITextServices *iface, LONG *x)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetBaseLinePos,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetBaseLinePos(ITextServices *iface, LONG *x)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetNaturalSize,36)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetNaturalSize( ITextServices *iface, DWORD aspect, HDC draw,
                                                               HDC target, DVTARGETDEVICE *td, DWORD mode,
                                                               const SIZEL *extent, LONG *width, LONG *height )
{
    struct text_services *services = impl_from_ITextServices( iface );
    RECT rect;
    HDC dc = draw;
    BOOL rewrap = FALSE;
    HRESULT hr;

    TRACE( "%p: aspect %d, draw %p, target %p, td %p, mode %08x, extent %s, *width %d, *height %d\n", services,
           aspect, draw, target, td, mode, wine_dbgstr_point( (POINT *)extent ), *width, *height );

    if (aspect != DVASPECT_CONTENT || target || td || mode != TXTNS_FITTOCONTENT )
        FIXME( "Many arguments are ignored\n" );

    SetRect( &rect, 0, 0, *width, *height );

    hr = update_client_rect( services, &rect );
    if (FAILED( hr )) return hr;
    if (hr == S_OK) rewrap = TRUE;

    if (!dc && services->editor->in_place_active)
        dc = ITextHost_TxGetDC( services->editor->texthost );
    if (!dc) return E_FAIL;

    if (rewrap)
    {
        editor_mark_rewrap_all( services->editor );
        wrap_marked_paras_dc( services->editor, dc, FALSE );
    }

    *width = services->editor->nTotalWidth;
    *height = services->editor->nTotalLength;

    if (!draw) ITextHost_TxReleaseDC( services->editor->texthost, dc );
    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetDropTarget,8)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetDropTarget(ITextServices *iface, IDropTarget **ppDropTarget)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_OnTxPropertyBitsChange,12)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxPropertyBitsChange( ITextServices *iface, DWORD mask, DWORD bits )
{
    struct text_services *services = impl_from_ITextServices( iface );
    DWORD scrollbars;
    HRESULT hr;
    BOOL repaint = FALSE;

    TRACE( "%p, mask %08x, bits %08x\n", services, mask, bits );

    services->editor->props = (services->editor->props & ~mask) | (bits & mask);
    if (mask & (TXTBIT_WORDWRAP | TXTBIT_MULTILINE))
        services->editor->bWordWrap = (services->editor->props & TXTBIT_WORDWRAP) && (services->editor->props & TXTBIT_MULTILINE);

    if (mask & TXTBIT_SCROLLBARCHANGE)
    {
        hr = ITextHost_TxGetScrollBars( services->editor->texthost, &scrollbars );
        if (SUCCEEDED( hr ))
        {
            if ((services->editor->scrollbars ^ scrollbars) & WS_HSCROLL)
                ITextHost_TxShowScrollBar( services->editor->texthost, SB_HORZ, (scrollbars & WS_HSCROLL) &&
                                           services->editor->nTotalWidth > services->editor->sizeWindow.cx );
            if ((services->editor->scrollbars ^ scrollbars) & WS_VSCROLL)
                ITextHost_TxShowScrollBar( services->editor->texthost, SB_VERT, (scrollbars & WS_VSCROLL) &&
                                           services->editor->nTotalLength > services->editor->sizeWindow.cy );
            services->editor->scrollbars = scrollbars;
        }
    }

    if ((mask & TXTBIT_HIDESELECTION) && !services->editor->bHaveFocus) ME_InvalidateSelection( services->editor );

    if (mask & TXTBIT_SELBARCHANGE)
    {
        LONG width;

        hr = ITextHost_TxGetSelectionBarWidth( services->editor->texthost, &width );
        if (hr == S_OK)
        {
            ITextHost_TxInvalidateRect( services->editor->texthost, &services->editor->rcFormat, TRUE );
            services->editor->rcFormat.left -= services->editor->selofs;
            services->editor->selofs = width ? SELECTIONBAR_WIDTH : 0; /* FIXME: convert from HIMETRIC */
            services->editor->rcFormat.left += services->editor->selofs;
            repaint = TRUE;
        }
    }

    if (mask & TXTBIT_CLIENTRECTCHANGE)
    {
        hr = update_client_rect( services, NULL );
        if (SUCCEEDED( hr )) repaint = TRUE;
    }

    if (mask & TXTBIT_USEPASSWORD)
    {
        if (bits & TXTBIT_USEPASSWORD) ITextHost_TxGetPasswordChar( services->editor->texthost, &services->editor->password_char );
        else services->editor->password_char = 0;
        repaint = TRUE;
    }

    if (repaint) ME_RewrapRepaint( services->editor );

    return S_OK;
}

DEFINE_THISCALL_WRAPPER(fnTextSrv_TxGetCachedSize,12)
DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetCachedSize(ITextServices *iface, DWORD *pdwWidth, DWORD *pdwHeight)
{
    struct text_services *services = impl_from_ITextServices( iface );

    FIXME( "%p: STUB\n", services );
    return E_NOTIMPL;
}

#ifdef __ASM_USE_THISCALL_WRAPPER

#define STDCALL(func) (void *) __stdcall_ ## func
#ifdef _MSC_VER
#define DEFINE_STDCALL_WRAPPER(num,func) \
    __declspec(naked) HRESULT __stdcall_##func(void) \
    { \
        __asm pop eax \
        __asm pop ecx \
        __asm push eax \
        __asm mov eax, [ecx] \
        __asm jmp dword ptr [eax + 4*num] \
    }
#else /* _MSC_VER */
#define DEFINE_STDCALL_WRAPPER(num,func) \
   extern HRESULT __stdcall_ ## func(void); \
   __ASM_GLOBAL_FUNC(__stdcall_ ## func, \
                   "popl %eax\n\t" \
                   "popl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "movl (%ecx), %eax\n\t" \
                   "jmp *(4*(" #num "))(%eax)" )
#endif /* _MSC_VER */

DEFINE_STDCALL_WRAPPER(3, ITextServices_TxSendMessage)
DEFINE_STDCALL_WRAPPER(4, ITextServices_TxDraw)
DEFINE_STDCALL_WRAPPER(5, ITextServices_TxGetHScroll)
DEFINE_STDCALL_WRAPPER(6, ITextServices_TxGetVScroll)
DEFINE_STDCALL_WRAPPER(7, ITextServices_OnTxSetCursor)
DEFINE_STDCALL_WRAPPER(8, ITextServices_TxQueryHitPoint)
DEFINE_STDCALL_WRAPPER(9, ITextServices_OnTxInPlaceActivate)
DEFINE_STDCALL_WRAPPER(10, ITextServices_OnTxInPlaceDeactivate)
DEFINE_STDCALL_WRAPPER(11, ITextServices_OnTxUIActivate)
DEFINE_STDCALL_WRAPPER(12, ITextServices_OnTxUIDeactivate)
DEFINE_STDCALL_WRAPPER(13, ITextServices_TxGetText)
DEFINE_STDCALL_WRAPPER(14, ITextServices_TxSetText)
DEFINE_STDCALL_WRAPPER(15, ITextServices_TxGetCurTargetX)
DEFINE_STDCALL_WRAPPER(16, ITextServices_TxGetBaseLinePos)
DEFINE_STDCALL_WRAPPER(17, ITextServices_TxGetNaturalSize)
DEFINE_STDCALL_WRAPPER(18, ITextServices_TxGetDropTarget)
DEFINE_STDCALL_WRAPPER(19, ITextServices_OnTxPropertyBitsChange)
DEFINE_STDCALL_WRAPPER(20, ITextServices_TxGetCachedSize)

const ITextServicesVtbl text_services_stdcall_vtbl =
{
    NULL,
    NULL,
    NULL,
    STDCALL(ITextServices_TxSendMessage),
    STDCALL(ITextServices_TxDraw),
    STDCALL(ITextServices_TxGetHScroll),
    STDCALL(ITextServices_TxGetVScroll),
    STDCALL(ITextServices_OnTxSetCursor),
    STDCALL(ITextServices_TxQueryHitPoint),
    STDCALL(ITextServices_OnTxInPlaceActivate),
    STDCALL(ITextServices_OnTxInPlaceDeactivate),
    STDCALL(ITextServices_OnTxUIActivate),
    STDCALL(ITextServices_OnTxUIDeactivate),
    STDCALL(ITextServices_TxGetText),
    STDCALL(ITextServices_TxSetText),
    STDCALL(ITextServices_TxGetCurTargetX),
    STDCALL(ITextServices_TxGetBaseLinePos),
    STDCALL(ITextServices_TxGetNaturalSize),
    STDCALL(ITextServices_TxGetDropTarget),
    STDCALL(ITextServices_OnTxPropertyBitsChange),
    STDCALL(ITextServices_TxGetCachedSize),
};

#endif /* __ASM_USE_THISCALL_WRAPPER */

static const ITextServicesVtbl textservices_vtbl =
{
    fnTextSrv_QueryInterface,
    fnTextSrv_AddRef,
    fnTextSrv_Release,
    THISCALL(fnTextSrv_TxSendMessage),
    THISCALL(fnTextSrv_TxDraw),
    THISCALL(fnTextSrv_TxGetHScroll),
    THISCALL(fnTextSrv_TxGetVScroll),
    THISCALL(fnTextSrv_OnTxSetCursor),
    THISCALL(fnTextSrv_TxQueryHitPoint),
    THISCALL(fnTextSrv_OnTxInPlaceActivate),
    THISCALL(fnTextSrv_OnTxInPlaceDeactivate),
    THISCALL(fnTextSrv_OnTxUIActivate),
    THISCALL(fnTextSrv_OnTxUIDeactivate),
    THISCALL(fnTextSrv_TxGetText),
    THISCALL(fnTextSrv_TxSetText),
    THISCALL(fnTextSrv_TxGetCurTargetX),
    THISCALL(fnTextSrv_TxGetBaseLinePos),
    THISCALL(fnTextSrv_TxGetNaturalSize),
    THISCALL(fnTextSrv_TxGetDropTarget),
    THISCALL(fnTextSrv_OnTxPropertyBitsChange),
    THISCALL(fnTextSrv_TxGetCachedSize)
};

HRESULT create_text_services( IUnknown *outer, ITextHost *text_host, IUnknown **unk, BOOL emulate_10 )
{
    struct text_services *services;

    TRACE( "%p %p --> %p\n", outer, text_host, unk );
    if (text_host == NULL) return E_POINTER;

    services = CoTaskMemAlloc( sizeof(*services) );
    if (services == NULL) return E_OUTOFMEMORY;
    services->ref = 1;
    services->IUnknown_inner.lpVtbl = &textservices_inner_vtbl;
    services->ITextServices_iface.lpVtbl = &textservices_vtbl;
    services->IRichEditOle_iface.lpVtbl = &re_ole_vtbl;
    services->ITextDocument2Old_iface.lpVtbl = &text_doc2old_vtbl;
    services->editor = ME_MakeEditor( text_host, emulate_10 );
    services->editor->richole = &services->IRichEditOle_iface;

    if (outer) services->outer_unk = outer;
    else services->outer_unk = &services->IUnknown_inner;

    services->text_selection = NULL;
    list_init( &services->rangelist );
    list_init( &services->clientsites );

    *unk = &services->IUnknown_inner;
    return S_OK;
}

/******************************************************************
 *        CreateTextServices (RICHED20.4)
 */
HRESULT WINAPI CreateTextServices( IUnknown *outer, ITextHost *text_host, IUnknown **unk )
{
    return create_text_services( outer, text_host, unk, FALSE );
}
