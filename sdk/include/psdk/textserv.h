/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef _TEXTSERV_H
#define _TEXTSERV_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __REACTOS__
#include <wine/asm.h>
#endif

#ifdef __cplusplus
#define THISCALLMETHOD_(type,method)  virtual type __thiscall method
#else
#define THISCALLMETHOD_(type,method)  type (__thiscall *method)
#endif

EXTERN_C const IID IID_ITextServices;
EXTERN_C const IID IID_ITextHost;
EXTERN_C const IID IID_ITextHost2;

/*****************************************************************************
 * ITextServices interface
 */
#define INTERFACE ITextServices
DECLARE_INTERFACE_(ITextServices,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void** ppvObject) PURE;

    STDMETHOD_(ULONG,AddRef)(THIS) PURE;

    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** ITextServices methods ***/

    THISCALLMETHOD_(HRESULT,TxSendMessage)( THIS_
        UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* plresult) PURE;

    THISCALLMETHOD_(HRESULT,TxDraw)( THIS_
        DWORD dwDrawAspect,
        LONG lindex,
        void* pvAspect,
        DVTARGETDEVICE* ptd,
        HDC hdcDraw,
        HDC hicTargetDev,
        LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds,
        LPRECT lprcUpdate,
        BOOL (CALLBACK * pfnContinue)(DWORD),
        DWORD dwContinue,
        LONG lViewId) PURE;

    THISCALLMETHOD_(HRESULT,TxGetHScroll)( THIS_
        LONG* plMin,
        LONG* plMax,
        LONG* plPos,
        LONG* plPage,
        BOOL* pfEnabled) PURE;

    THISCALLMETHOD_(HRESULT,TxGetVScroll)( THIS_
        LONG* plMin,
        LONG* plMax,
        LONG* plPos,
        LONG* plPage,
        BOOL* pfEnabled) PURE;

    THISCALLMETHOD_(HRESULT,OnTxSetCursor)( THIS_
        DWORD dwDrawAspect,
        LONG lindex,
        void* pvAspect,
        DVTARGETDEVICE* ptd,
        HDC hdcDraw,
        HDC hicTargetDev,
        LPCRECT lprcClient,
        INT x,
        INT y) PURE;

    THISCALLMETHOD_(HRESULT,TxQueryHitPoint)( THIS_
        DWORD dwDrawAspect,
        LONG lindex,
        void* pvAspect,
        DVTARGETDEVICE* ptd,
        HDC hdcDraw,
        HDC hicTargetDev,
        LPCRECT lprcClient,
        INT x,
        INT y,
        DWORD* pHitResult) PURE;

    THISCALLMETHOD_(HRESULT,OnTxInPlaceActivate)( THIS_
        LPCRECT prcClient) PURE;

    THISCALLMETHOD_(HRESULT,OnTxInPlaceDeactivate)( THIS ) PURE;

    THISCALLMETHOD_(HRESULT,OnTxUIActivate)( THIS ) PURE;

    THISCALLMETHOD_(HRESULT,OnTxUIDeactivate)( THIS ) PURE;

    THISCALLMETHOD_(HRESULT,TxGetText)( THIS_
        BSTR* pbstrText) PURE;

    THISCALLMETHOD_(HRESULT,TxSetText)( THIS_
        LPCWSTR pszText) PURE;

    THISCALLMETHOD_(HRESULT,TxGetCurTargetX)( THIS_
        LONG* x) PURE;

    THISCALLMETHOD_(HRESULT,TxGetBaseLinePos)( THIS_
        LONG* x) PURE;

    THISCALLMETHOD_(HRESULT,TxGetNaturalSize)( THIS_
        DWORD dwAspect,
        HDC hdcDraw,
        HDC hicTargetDev,
        DVTARGETDEVICE* ptd,
        DWORD dwMode,
        const SIZEL* psizelExtent,
        LONG* pwidth,
        LONG* pheight) PURE;

    THISCALLMETHOD_(HRESULT,TxGetDropTarget)( THIS_
        IDropTarget** ppDropTarget) PURE;

    THISCALLMETHOD_(HRESULT,OnTxPropertyBitsChange)( THIS_
        DWORD dwMask,
        DWORD dwBits) PURE;

    THISCALLMETHOD_(HRESULT,TxGetCachedSize)( THIS_
        DWORD* pdwWidth,
        DWORD* pdwHeight) PURE;

};
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ITextServices_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITextServices_AddRef(p) (p)->lpVtbl->AddRef(p)
#define ITextServices_Release(p) (p)->lpVtbl->Release(p)
#endif

typedef enum _TXTBACKSTYLE {
    TXTBACK_TRANSPARENT = 0,
    TXTBACK_OPAQUE
} TXTBACKSTYLE;

enum TXTHITRESULT {
    TXTHITRESULT_NOHIT = 0,
    TXTHITRESULT_TRANSPARENT = 1,
    TXTHITRESULT_CLOSE = 2,
    TXTHITRESULT_HIT = 3
};

enum TXTNATURALSIZE {
    TXTNS_FITTOCONTENT = 1,
    TXTNS_ROUNDTOLINE = 2
};

enum TXTVIEW {
    TXTVIEW_ACTIVE = 0,
    TXTVIEW_INACTIVE = -1
};

#define TXTBIT_RICHTEXT         0x000001
#define TXTBIT_MULTILINE        0x000002
#define TXTBIT_READONLY         0x000004
#define TXTBIT_SHOWACCELERATOR  0x000008
#define TXTBIT_USEPASSWORD      0x000010
#define TXTBIT_HIDESELECTION    0x000020
#define TXTBIT_SAVESELECTION    0x000040
#define TXTBIT_AUTOWORDSEL      0x000080
#define TXTBIT_VERTICAL         0x000100
#define TXTBIT_SELBARCHANGE     0x000200
#define TXTBIT_WORDWRAP         0x000400
#define TXTBIT_ALLOWBEEP        0x000800
#define TXTBIT_DISABLEDRAG      0x001000
#define TXTBIT_VIEWINSETCHANGE  0x002000
#define TXTBIT_BACKSTYLECHANGE  0x004000
#define TXTBIT_MAXLENGTHCHANGE  0x008000
#define TXTBIT_SCROLLBARCHANGE  0x010000
#define TXTBIT_CHARFORMATCHANGE 0x020000
#define TXTBIT_PARAFORMATCHANGE 0x040000
#define TXTBIT_EXTENTCHANGE     0x080000
#define TXTBIT_CLIENTRECTCHANGE 0x100000
#define TXTBIT_USECURRENTBKG    0x200000

/*****************************************************************************
 * ITextHost interface
 */
#define INTERFACE ITextHost
DECLARE_INTERFACE_(ITextHost,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_
        REFIID riid,
        void** ppvObject) PURE;

    STDMETHOD_(ULONG,AddRef)(THIS) PURE;

    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /*** ITextHost methods ***/
    THISCALLMETHOD_(HDC,TxGetDC)( THIS
        ) PURE;

    THISCALLMETHOD_(INT,TxReleaseDC)( THIS_
        HDC hdc) PURE;

    THISCALLMETHOD_(BOOL,TxShowScrollBar)( THIS_
        INT fnBar,
        BOOL fShow) PURE;

    THISCALLMETHOD_(BOOL,TxEnableScrollBar)( THIS_
        INT fuSBFlags,
        INT fuArrowflags) PURE;

    THISCALLMETHOD_(BOOL,TxSetScrollRange)( THIS_
        INT fnBar,
        LONG nMinPos,
        INT nMaxPos,
        BOOL fRedraw) PURE;

    THISCALLMETHOD_(BOOL,TxSetScrollPos)( THIS_
        INT fnBar,
        INT nPos,
        BOOL fRedraw) PURE;

    THISCALLMETHOD_(void,TxInvalidateRect)( THIS_
        LPCRECT prc,
        BOOL fMode) PURE;

    THISCALLMETHOD_(void,TxViewChange)( THIS_
        BOOL fUpdate) PURE;

    THISCALLMETHOD_(BOOL,TxCreateCaret)( THIS_
        HBITMAP hbmp,
        INT xWidth,
        INT yHeight) PURE;

    THISCALLMETHOD_(BOOL,TxShowCaret)( THIS_
        BOOL fShow) PURE;

    THISCALLMETHOD_(BOOL,TxSetCaretPos)( THIS_
        INT x,
        INT y) PURE;

    THISCALLMETHOD_(BOOL,TxSetTimer)( THIS_
        UINT idTimer,
        UINT uTimeout) PURE;

    THISCALLMETHOD_(void,TxKillTimer)( THIS_
        UINT idTimer) PURE;

    THISCALLMETHOD_(void,TxScrollWindowEx)( THIS_
        INT dx,
        INT dy,
        LPCRECT lprcScroll,
        LPCRECT lprcClip,
        HRGN hRgnUpdate,
        LPRECT lprcUpdate,
        UINT fuScroll) PURE;

    THISCALLMETHOD_(void,TxSetCapture)( THIS_
        BOOL fCapture) PURE;

    THISCALLMETHOD_(void,TxSetFocus)( THIS
        ) PURE;

    THISCALLMETHOD_(void,TxSetCursor)( THIS_
        HCURSOR hcur,
        BOOL fText) PURE;

    THISCALLMETHOD_(BOOL,TxScreenToClient)( THIS_
        LPPOINT lppt) PURE;

    THISCALLMETHOD_(BOOL,TxClientToScreen)( THIS_
        LPPOINT lppt) PURE;

    THISCALLMETHOD_(HRESULT,TxActivate)( THIS_
        LONG* plOldState) PURE;

    THISCALLMETHOD_(HRESULT,TxDeactivate)( THIS_
        LONG lNewState) PURE;

    THISCALLMETHOD_(HRESULT,TxGetClientRect)( THIS_
        LPRECT prc) PURE;

    THISCALLMETHOD_(HRESULT,TxGetViewInset)( THIS_
        LPRECT prc) PURE;

    THISCALLMETHOD_(HRESULT,TxGetCharFormat)( THIS_
        const CHARFORMATW** ppCF) PURE;

    THISCALLMETHOD_(HRESULT,TxGetParaFormat)( THIS_
        const PARAFORMAT** ppPF) PURE;

    THISCALLMETHOD_(COLORREF,TxGetSysColor)( THIS_
        int nIndex) PURE;

    THISCALLMETHOD_(HRESULT,TxGetBackStyle)( THIS_
        TXTBACKSTYLE* pStyle) PURE;

    THISCALLMETHOD_(HRESULT,TxGetMaxLength)( THIS_
        DWORD* plength) PURE;

    THISCALLMETHOD_(HRESULT,TxGetScrollBars)( THIS_
        DWORD* pdwScrollBar) PURE;

    THISCALLMETHOD_(HRESULT,TxGetPasswordChar)( THIS_
        WCHAR* pch) PURE;

    THISCALLMETHOD_(HRESULT,TxGetAcceleratorPos)( THIS_
        LONG* pch) PURE;

    THISCALLMETHOD_(HRESULT,TxGetExtent)( THIS_
        LPSIZEL lpExtent) PURE;

    THISCALLMETHOD_(HRESULT,OnTxCharFormatChange)( THIS_
        const CHARFORMATW* pcf) PURE;

    THISCALLMETHOD_(HRESULT,OnTxParaFormatChange)( THIS_
        const PARAFORMAT* ppf) PURE;

    THISCALLMETHOD_(HRESULT,TxGetPropertyBits)( THIS_
        DWORD dwMask,
        DWORD* pdwBits) PURE;

    THISCALLMETHOD_(HRESULT,TxNotify)( THIS_
        DWORD iNotify,
        void* pv) PURE;

    THISCALLMETHOD_(HIMC,TxImmGetContext)( THIS
        ) PURE;

    THISCALLMETHOD_(void,TxImmReleaseContext)( THIS_
        HIMC himc) PURE;

    THISCALLMETHOD_(HRESULT,TxGetSelectionBarWidth)( THIS_
        LONG* lSelBarWidth) PURE;

};
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ITextHost_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITextHost_AddRef(p) (p)->lpVtbl->AddRef(p)
#define ITextHost_Release(p) (p)->lpVtbl->Release(p)
#endif

/*****************************************************************************
 * ITextHost2 interface
 */
#define INTERFACE ITextHost2
DECLARE_INTERFACE_(ITextHost2,ITextHost)
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)( THIS_ REFIID riid, void** ppvObject ) PURE;
    STDMETHOD_(ULONG,AddRef)( THIS ) PURE;
    STDMETHOD_(ULONG,Release)( THIS ) PURE;
    /*** ITextHost methods ***/
    THISCALLMETHOD_(HDC,TxGetDC)( THIS ) PURE;
    THISCALLMETHOD_(INT,TxReleaseDC)( THIS_ HDC hdc ) PURE;
    THISCALLMETHOD_(BOOL,TxShowScrollBar)( THIS_ INT fnBar, BOOL fShow ) PURE;
    THISCALLMETHOD_(BOOL,TxEnableScrollBar)( THIS_ INT fuSBFlags, INT fuArrowflags ) PURE;
    THISCALLMETHOD_(BOOL,TxSetScrollRange)( THIS_ INT fnBar, LONG nMinPos, INT nMaxPos, BOOL fRedraw ) PURE;
    THISCALLMETHOD_(BOOL,TxSetScrollPos)( THIS_ INT fnBar, INT nPos, BOOL fRedraw ) PURE;
    THISCALLMETHOD_(void,TxInvalidateRect)( THIS_ LPCRECT prc, BOOL fMode ) PURE;
    THISCALLMETHOD_(void,TxViewChange)( THIS_ BOOL fUpdate ) PURE;
    THISCALLMETHOD_(BOOL,TxCreateCaret)( THIS_ HBITMAP hbmp, INT xWidth, INT yHeight ) PURE;
    THISCALLMETHOD_(BOOL,TxShowCaret)( THIS_ BOOL fShow ) PURE;
    THISCALLMETHOD_(BOOL,TxSetCaretPos)( THIS_ INT x, INT y ) PURE;
    THISCALLMETHOD_(BOOL,TxSetTimer)( THIS_ UINT idTimer, UINT uTimeout ) PURE;
    THISCALLMETHOD_(void,TxKillTimer)( THIS_ UINT idTimer ) PURE;
    THISCALLMETHOD_(void,TxScrollWindowEx)( THIS_ INT dx, INT dy, LPCRECT lprcScroll, LPCRECT lprcClip,
                                            HRGN hRgnUpdate, LPRECT lprcUpdate, UINT fuScroll ) PURE;
    THISCALLMETHOD_(void,TxSetCapture)( THIS_ BOOL fCapture ) PURE;
    THISCALLMETHOD_(void,TxSetFocus)( THIS ) PURE;
    THISCALLMETHOD_(void,TxSetCursor)( THIS_ HCURSOR hcur, BOOL fText ) PURE;
    THISCALLMETHOD_(BOOL,TxScreenToClient)( THIS_ LPPOINT lppt ) PURE;
    THISCALLMETHOD_(BOOL,TxClientToScreen)( THIS_ LPPOINT lppt ) PURE;
    THISCALLMETHOD_(HRESULT,TxActivate)( THIS_ LONG* plOldState ) PURE;
    THISCALLMETHOD_(HRESULT,TxDeactivate)( THIS_ LONG lNewState ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetClientRect)( THIS_ LPRECT prc ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetViewInset)( THIS_ LPRECT prc ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetCharFormat)( THIS_ const CHARFORMATW** ppCF ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetParaFormat)( THIS_ const PARAFORMAT** ppPF ) PURE;
    THISCALLMETHOD_(COLORREF,TxGetSysColor)( THIS_ int nIndex ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetBackStyle)( THIS_ TXTBACKSTYLE* pStyle ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetMaxLength)( THIS_ DWORD* plength ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetScrollBars)( THIS_ DWORD* pdwScrollBar ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetPasswordChar)( THIS_ WCHAR* pch ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetAcceleratorPos)( THIS_ LONG* pch ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetExtent)( THIS_ LPSIZEL lpExtent ) PURE;
    THISCALLMETHOD_(HRESULT,OnTxCharFormatChange)( THIS_ const CHARFORMATW* pcf ) PURE;
    THISCALLMETHOD_(HRESULT,OnTxParaFormatChange)( THIS_ const PARAFORMAT* ppf ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetPropertyBits)( THIS_ DWORD dwMask, DWORD* pdwBits ) PURE;
    THISCALLMETHOD_(HRESULT,TxNotify)( THIS_ DWORD iNotify, void* pv ) PURE;
    THISCALLMETHOD_(HIMC,TxImmGetContext)( THIS ) PURE;
    THISCALLMETHOD_(void,TxImmReleaseContext)( THIS_ HIMC himc ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetSelectionBarWidth)( THIS_ LONG* lSelBarWidth ) PURE;
    /* ITextHost2 methods */
    THISCALLMETHOD_(BOOL,TxIsDoubleClickPending)( THIS ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetWindow)( THIS_ HWND *hwnd ) PURE;
    THISCALLMETHOD_(HRESULT,TxSetForegroundWindow)( THIS ) PURE;
    THISCALLMETHOD_(HPALETTE,TxGetPalette)( THIS ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetEastAsianFlags)( THIS_ LONG *flags ) PURE;
    THISCALLMETHOD_(HCURSOR,TxSetCursor2)( THIS_ HCURSOR cursor, BOOL text ) PURE;
    THISCALLMETHOD_(void,TxFreeTextServicesNotification)( THIS ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetEditStyle)( THIS_ DWORD item, DWORD *data ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetWindowStyles)( THIS_ DWORD *style, DWORD *ex_style ) PURE;
    THISCALLMETHOD_(HRESULT,TxShowDropCaret)( THIS_ BOOL show, HDC hdc, const RECT *rect ) PURE;
    THISCALLMETHOD_(HRESULT,TxDestroyCaret)( THIS ) PURE;
    THISCALLMETHOD_(HRESULT,TxGetHorzExtent)( THIS_ LONG *horz_extent ) PURE;
};
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ITextHost2_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITextHost2_AddRef(p) (p)->lpVtbl->AddRef(p)
#define ITextHost2_Release(p) (p)->lpVtbl->Release(p)
#endif

HRESULT WINAPI CreateTextServices(IUnknown*,ITextHost*,IUnknown**);

typedef HRESULT (WINAPI *PCreateTextServices)(IUnknown*,ITextHost*,IUnknown**);

#ifdef __cplusplus
}
#endif

#endif /* _TEXTSERV_H */
