/*
 * RichEdit - functions and interfaces around CreateTextServices for txtsrv.c
 *
 * Copyright 2005, 2006, Maarten Lankhorst
 *
 * RichEdit - ITextHost implementation for windowed richedit controls for txthost.c
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


/* Forward definitions from txtsrv.c to make MSVC compile in ReactOS. */

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxSendMessage( ITextServices *iface, UINT msg, WPARAM wparam,
                                                            LPARAM lparam, LRESULT *result );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxDraw( ITextServices *iface, DWORD aspect, LONG index, void *aspect_info,
                                                     DVTARGETDEVICE *td, HDC draw, HDC target,
                                                     const RECTL *bounds, const RECTL *mf_bounds, RECT *update,
                                                     BOOL (CALLBACK *continue_fn)(DWORD), DWORD continue_param,
                                                     LONG view_id );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetHScroll( ITextServices *iface, LONG *min_pos, LONG *max_pos, LONG *pos,
                                                           LONG *page, BOOL *enabled );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetVScroll( ITextServices *iface, LONG *min_pos, LONG *max_pos, LONG *pos,
                                                           LONG *page, BOOL *enabled );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxSetCursor( ITextServices *iface, DWORD aspect, LONG index,
                                                            void *aspect_info, DVTARGETDEVICE *td, HDC draw,
                                                            HDC target, const RECT *client, INT x, INT y );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxQueryHitPoint(ITextServices *iface, DWORD dwDrawAspect, LONG lindex,
                                                             void *pvAspect, DVTARGETDEVICE *ptd, HDC hdcDraw,
                                                             HDC hicTargetDev, LPCRECT lprcClient, INT x, INT y,
                                                             DWORD *pHitResult);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxInPlaceActivate( ITextServices *iface, const RECT *client );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxInPlaceDeactivate(ITextServices *iface);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxUIActivate(ITextServices *iface);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxUIDeactivate(ITextServices *iface);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetText( ITextServices *iface, BSTR *text );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxSetText( ITextServices *iface, const WCHAR *text );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetCurTargetX(ITextServices *iface, LONG *x);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetBaseLinePos(ITextServices *iface, LONG *x);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetNaturalSize( ITextServices *iface, DWORD aspect, HDC draw,
                                                               HDC target, DVTARGETDEVICE *td, DWORD mode,
                                                               const SIZEL *extent, LONG *width, LONG *height );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetDropTarget(ITextServices *iface, IDropTarget **ppDropTarget);

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_OnTxPropertyBitsChange( ITextServices *iface, DWORD mask, DWORD bits );

DECLSPEC_HIDDEN HRESULT __thiscall fnTextSrv_TxGetCachedSize(ITextServices *iface, DWORD *pdwWidth, DWORD *pdwHeight);


/* Forward definitions from txthost.c to make MSVC compile in ReactOS. */


DECLSPEC_HIDDEN HDC __thiscall ITextHostImpl_TxGetDC( ITextHost2 *iface );

DECLSPEC_HIDDEN INT __thiscall ITextHostImpl_TxReleaseDC( ITextHost2 *iface, HDC hdc );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxShowScrollBar( ITextHost2 *iface, INT bar, BOOL show );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxEnableScrollBar( ITextHost2 *iface, INT bar, INT arrows );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxSetScrollRange( ITextHost2 *iface, INT bar, LONG min_pos, INT max_pos, BOOL redraw );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxSetScrollPos( ITextHost2 *iface, INT bar, INT pos, BOOL redraw );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxInvalidateRect( ITextHost2 *iface, const RECT *rect, BOOL mode );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxViewChange( ITextHost2 *iface, BOOL update );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxCreateCaret( ITextHost2 *iface, HBITMAP bitmap, INT width, INT height );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxShowCaret( ITextHost2 *iface, BOOL show );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxSetCaretPos( ITextHost2 *iface, INT x, INT y );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxSetTimer( ITextHost2 *iface, UINT id, UINT timeout );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxKillTimer( ITextHost2 *iface, UINT id );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxScrollWindowEx( ITextHost2 *iface, INT dx, INT dy, const RECT *scroll,
                                                                const RECT *clip, HRGN update_rgn, RECT *update_rect,
                                                                UINT flags );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxSetCapture( ITextHost2 *iface, BOOL capture );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxSetFocus( ITextHost2 *iface );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxSetCursor( ITextHost2 *iface, HCURSOR cursor, BOOL text );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxScreenToClient( ITextHost2 *iface, POINT *pt );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxClientToScreen( ITextHost2 *iface, POINT *pt );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxActivate( ITextHost2 *iface, LONG *old_state );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxDeactivate( ITextHost2 *iface, LONG new_state );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetClientRect( ITextHost2 *iface, RECT *rect );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetViewInset( ITextHost2 *iface, RECT *rect );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetCharFormat( ITextHost2 *iface, const CHARFORMATW **ppCF );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetParaFormat( ITextHost2 *iface, const PARAFORMAT **fmt );

DECLSPEC_HIDDEN COLORREF __thiscall ITextHostImpl_TxGetSysColor( ITextHost2 *iface, int index );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetBackStyle( ITextHost2 *iface, TXTBACKSTYLE *style );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetMaxLength( ITextHost2 *iface, DWORD *length );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetScrollBars( ITextHost2 *iface, DWORD *scrollbars );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetPasswordChar( ITextHost2 *iface, WCHAR *c );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetAcceleratorPos( ITextHost2 *iface, LONG *pos );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetExtent( ITextHost2 *iface, SIZEL *extent );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_OnTxCharFormatChange( ITextHost2 *iface, const CHARFORMATW *pcf );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_OnTxParaFormatChange( ITextHost2 *iface, const PARAFORMAT *ppf );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetPropertyBits( ITextHost2 *iface, DWORD mask, DWORD *bits );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxNotify( ITextHost2 *iface, DWORD iNotify, void *pv );

DECLSPEC_HIDDEN HIMC __thiscall ITextHostImpl_TxImmGetContext( ITextHost2 *iface );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxImmReleaseContext( ITextHost2 *iface, HIMC context );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetSelectionBarWidth( ITextHost2 *iface, LONG *width );

DECLSPEC_HIDDEN BOOL __thiscall ITextHostImpl_TxIsDoubleClickPending( ITextHost2 *iface );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetWindow( ITextHost2 *iface, HWND *hwnd );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxSetForegroundWindow( ITextHost2 *iface );

DECLSPEC_HIDDEN HPALETTE __thiscall ITextHostImpl_TxGetPalette( ITextHost2 *iface );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetEastAsianFlags( ITextHost2 *iface, LONG *flags );

DECLSPEC_HIDDEN HCURSOR __thiscall ITextHostImpl_TxSetCursor2( ITextHost2 *iface, HCURSOR cursor, BOOL text );

DECLSPEC_HIDDEN void __thiscall ITextHostImpl_TxFreeTextServicesNotification( ITextHost2 *iface );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetEditStyle( ITextHost2 *iface, DWORD item, DWORD *data );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetWindowStyles( ITextHost2 *iface, DWORD *style, DWORD *ex_style );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxShowDropCaret( ITextHost2 *iface, BOOL show, HDC hdc, const RECT *rect );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxDestroyCaret( ITextHost2 *iface );

DECLSPEC_HIDDEN HRESULT __thiscall ITextHostImpl_TxGetHorzExtent( ITextHost2 *iface, LONG *horz_extent );

