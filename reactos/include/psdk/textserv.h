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

DEFINE_GUID(IID_ITextServices,0x8d33f740,0xcf58,0x11ce,0xa8,0x9d,0x00,0xaa,0x00,0x6c,0xad,0xc5);
DEFINE_GUID(IID_ITextHost,    0xc5bdd8d0,0xd26e,0x11ce,0xa8,0x9e,0x00,0xaa,0x00,0x6c,0xad,0xc5);
DEFINE_GUID(IID_ITextHost2,   0xc5bdd8d0,0xd26e,0x11ce,0xa8,0x9e,0x00,0xaa,0x00,0x6c,0xad,0xc5);

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

    STDMETHOD(TxSendMessage)( THIS_
        UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* plresult) PURE;

    STDMETHOD(TxDraw)( THIS_
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

    STDMETHOD(TxGetHScroll)( THIS_
        LONG* plMin,
        LONG* plMax,
        LONG* plPos,
        LONG* plPage,
        BOOL* pfEnabled) PURE;

    STDMETHOD(TxGetVScroll)( THIS_
        LONG* plMin,
        LONG* plMax,
        LONG* plPos,
        LONG* plPage,
        BOOL* pfEnabled) PURE;

    STDMETHOD(OnTxSetCursor)( THIS_
        DWORD dwDrawAspect,
        LONG lindex,
        void* pvAspect,
        DVTARGETDEVICE* ptd,
        HDC hdcDraw,
        HDC hicTargetDev,
        LPCRECT lprcClient,
        INT x,
        INT y) PURE;

    STDMETHOD(TxQueryHitPoint)( THIS_
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

    STDMETHOD(OnTxInplaceActivate)( THIS_
        LPCRECT prcClient) PURE;

    STDMETHOD(OnTxInplaceDeactivate)( THIS ) PURE;

    STDMETHOD(OnTxUIActivate)( THIS ) PURE;

    STDMETHOD(OnTxUIDeactivate)( THIS ) PURE;

    STDMETHOD(TxGetText)( THIS_
        BSTR* pbstrText) PURE;

    STDMETHOD(TxSetText)( THIS_
        LPCWSTR pszText) PURE;

    STDMETHOD(TxGetCurrentTargetX)( THIS_
        LONG* x) PURE;

    STDMETHOD(TxGetBaseLinePos)( THIS_
        LONG* x) PURE;

    STDMETHOD(TxGetNaturalSize)( THIS_
        DWORD dwAspect,
        HDC hdcDraw,
        HDC hicTargetDev,
        DVTARGETDEVICE* ptd,
        DWORD dwMode,
        const SIZEL* psizelExtent,
        LONG* pwidth,
        LONG* pheight) PURE;

    STDMETHOD(TxGetDropTarget)( THIS_
        IDropTarget** ppDropTarget) PURE;

    STDMETHOD(OnTxPropertyBitsChange)( THIS_
        DWORD dwMask,
        DWORD dwBits) PURE;

    STDMETHOD(TxGetCachedSize)( THIS_
        DWORD* pdwWidth,
        DWORD* pdwHeight) PURE;

};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ITextServices_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITextServices_AddRef(p) (p)->lpVtbl->AddRef(p)
#define ITextServices_Release(p) (p)->lpVtbl->Release(p)
#endif

#undef INTERFACE

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
    TXTVIEW_INACTIVE = 1
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
    STDMETHOD_(HDC,TxGetDC)( THIS
        ) PURE;

    STDMETHOD_(INT,TxReleaseDC)( THIS_
        HDC hdc) PURE;

    STDMETHOD_(BOOL,TxShowScrollBar)( THIS_
        INT fnBar,
        BOOL fShow) PURE;

    STDMETHOD_(BOOL,TxEnableScrollBar)( THIS_
        INT fuSBFlags,
        INT fuArrowflags) PURE;

    STDMETHOD_(BOOL,TxSetScrollRange)( THIS_
        INT fnBar,
        LONG nMinPos,
        INT nMaxPos,
        BOOL fRedraw) PURE;

    STDMETHOD_(BOOL,TxSetScrollPos)( THIS_
        INT fnBar,
        INT nPos,
        BOOL fRedraw) PURE;

    STDMETHOD_(void,TxInvalidateRect)( THIS_
        LPCRECT prc,
        BOOL fMode) PURE;

    STDMETHOD_(void,TxViewChange)( THIS_
        BOOL fUpdate) PURE;

    STDMETHOD_(BOOL,TxCreateCaret)( THIS_
        HBITMAP hbmp,
        INT xWidth,
        INT yHeight) PURE;

    STDMETHOD_(BOOL,TxShowCaret)( THIS_
        BOOL fShow) PURE;

    STDMETHOD_(BOOL,TxSetCaretPos)( THIS_
        INT x,
        INT y) PURE;

    STDMETHOD_(BOOL,TxSetTimer)( THIS_
        UINT idTimer,
        UINT uTimeout) PURE;

    STDMETHOD_(void,TxKillTimer)( THIS_
        UINT idTimer) PURE;

    STDMETHOD_(void,TxScrollWindowEx)( THIS_
        INT dx,
        INT dy,
        LPCRECT lprcScroll,
        LPCRECT lprcClip,
        HRGN hRgnUpdate,
        LPRECT lprcUpdate,
        UINT fuScroll) PURE;

    STDMETHOD_(void,TxSetCapture)( THIS_
        BOOL fCapture) PURE;

    STDMETHOD_(void,TxSetFocus)( THIS
        ) PURE;

    STDMETHOD_(void,TxSetCursor)( THIS_
        HCURSOR hcur,
        BOOL fText) PURE;

    STDMETHOD_(BOOL,TxScreenToClient)( THIS_
        LPPOINT lppt) PURE;

    STDMETHOD_(BOOL,TxClientToScreen)( THIS_
        LPPOINT lppt) PURE;

    STDMETHOD(TxActivate)( THIS_
        LONG* plOldState) PURE;

    STDMETHOD(TxDeactivate)( THIS_
        LONG lNewState) PURE;

    STDMETHOD(TxGetClientRect)( THIS_
        LPRECT prc) PURE;

    STDMETHOD(TxGetViewInset)( THIS_
        LPRECT prc) PURE;

    STDMETHOD(TxGetCharFormat)( THIS_
        const CHARFORMATW** ppCF) PURE;

    STDMETHOD(TxGetParaFormat)( THIS_
        const PARAFORMAT** ppPF) PURE;

    STDMETHOD_(COLORREF,TxGetSysColor)( THIS_
        int nIndex) PURE;

    STDMETHOD(TxGetBackStyle)( THIS_
        TXTBACKSTYLE* pStyle) PURE;

    STDMETHOD(TxGetMaxLength)( THIS_
        DWORD* plength) PURE;

    STDMETHOD(TxGetScrollBars)( THIS_
        DWORD* pdwScrollBar) PURE;

    STDMETHOD(TxGetPasswordChar)( THIS_
        WCHAR* pch) PURE;

    STDMETHOD(TxGetAcceleratorPos)( THIS_
        LONG* pch) PURE;

    STDMETHOD(TxGetExtent)( THIS_
        LPSIZEL lpExtent) PURE;

    STDMETHOD(OnTxCharFormatChange)( THIS_
        const CHARFORMATW* pcf) PURE;

    STDMETHOD(OnTxParaFormatChange)( THIS_
        const PARAFORMAT* ppf) PURE;

    STDMETHOD(TxGetPropertyBits)( THIS_
        DWORD dwMask,
        DWORD* pdwBits) PURE;

    STDMETHOD(TxNotify)( THIS_
        DWORD iNotify,
        void* pv) PURE;

    STDMETHOD_(HIMC,TxImmGetContext)( THIS
        ) PURE;

    STDMETHOD_(void,TxImmReleaseContext)( THIS_
        HIMC himc) PURE;

    STDMETHOD(TxGetSelectionBarWidth)( THIS_
        LONG* lSelBarWidth) PURE;

};

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define ITextHost_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define ITextHost_AddRef(p) (p)->lpVtbl->AddRef(p)
#define ITextHost_Release(p) (p)->lpVtbl->Release(p)
/*** ITextHost methods ***/
#define ITextHost_TxGetDC(p) (p)->lpVtbl->TxGetDC(p)
#define ITextHost_TxReleaseDC(p,a) (p)->lpVtbl->TxReleaseDC(p,a)
#define ITextHost_TxShowScrollBar(p,a,b) (p)->lpVtbl->TxShowScrollBar(p,a,b)
#define ITextHost_TxEnableScrollBar(p,a,b) (p)->lpVtbl->TxEnableScrollBar(p,a,b)
#define ITextHost_TxSetScrollRange(p,a,b,c,d) (p)->lpVtbl->TxSetScrollRange(p,a,b,c,d)
#define ITextHost_TxSetScrollPos(p,a,b,c) (p)->lpVtbl->TxSetScrollPos(p,a,b,c)
#define ITextHost_TxInvalidateRect(p,a,b) (p)->lpVtbl->TxInvalidateRect(p,a,b)
#define ITextHost_TxViewChange(p,a) (p)->lpVtbl->TxViewChange(p,a)
#define ITextHost_TxCreateCaret(p,a,b,c) (p)->lpVtbl->TxCreateCaret(p,a,b,c)
#define ITextHost_TxShowCaret(p,a) (p)->lpVtbl->TxShowCaret(p,a)
#define ITextHost_TxSetCarentPos(p,a,b) (p)->lpVtbl->TxSetCarentPos(p,a,b)
#define ITextHost_TxSetTimer(p,a,b) (p)->lpVtbl->TxSetTimer(p,a,b)
#define ITextHost_TxKillTimer(p,a) (p)->lpVtbl->TxKillTimer(p,a)
#define ITextHost_TxScrollWindowEx(p,a,b,c,d,e,f,g) (p)->lpVtbl->TxScrollWindowEx(p,a,b,c,d,e,f,g)
#define ITextHost_TxSetCapture(p,a) (p)->lpVtbl->TxSetCapture(p,a)
#define ITextHost_TxSetFocus(p) (p)->lpVtbl->TxSetFocus(p)
#define ITextHost_TxSetCursor(p,a,b) (p)->lpVtbl->TxSetCursor(p,a,b)
#define ITextHost_TxScreenToClient(p,a) (p)->lpVtbl->TxScreenToClient(p,a)
#define ITextHost_TxClientToScreen(p,a) (p)->lpVtbl->TxClientToScreen(p,a)
#define ITextHost_TxActivate(p,a) (p)->lpVtbl->TxActivate(p,a)
#define ITextHost_TxDeactivate(p,a) (p)->lpVtbl->TxDeactivate(p,a)
#define ITextHost_TxGetClientRect(p,a) (p)->lpVtbl->TxGetClientRect(p,a)
#define ITextHost_TxGetViewInset(p,a) (p)->lpVtbl->TxGetViewInset(p,a)
#define ITextHost_TxGetCharFormat(p,a) (p)->lpVtbl->TxGetCharFormat(p,a)
#define ITextHost_TxGetParaFormat(p,a) (p)->lpVtbl->TxGetParaFormat(p,a)
#define ITextHost_TxGetSysColor(p,a) (p)->lpVtbl->TxGetSysColor(p,a)
#define ITextHost_TxGetBackStyle(p,a) (p)->lpVtbl->TxGetBackStyle(p,a)
#define ITextHost_TxGetMaxLength(p,a) (p)->lpVtbl->TxGetMaxLength(p,a)
#define ITextHost_TxGetScrollBars(p,a) (p)->lpVtbl->TxGetScrollBars(p,a)
#define ITextHost_TxGetPasswordChar(p,a) (p)->lpVtbl->TxGetPasswordChar(p,a)
#define ITextHost_TxGetAcceleratorPos(p,a) (p)->lpVtbl->TxGetAcceleratorPos(p,a)
#define ITextHost_TxGetExtent(p,a) (p)->lpVtbl->TxGetExtent(p,a)
#define ITextHost_OnTxCharFormatChange(p,a) (p)->lpVtbl->OnTxCharFormatChange(p,a)
#define ITextHost_OnTxParaFormatChange(p,a) (p)->lpVtbl->OnTxParaFormatChange(p,a)
#define ITextHost_TxGetPropertyBits(p,a,b) (p)->lpVtbl->TxGetPropertyBits(p,a,b)
#define ITextHost_TxNotify(p,a,b) (p)->lpVtbl->TxNotify(p,a,b)
#define ITextHost_TxImmGetContext(p) (p)->lpVtbl->TxImmGetContext(p)
#define ITextHost_TxImmReleaseContext(p,a) (p)->lpVtbl->TxImmReleaseContext(p,a)
#define ITextHost_TxGetSelectionBarWidth(p,a) (p)->lpVtbl->TxGetSelectionBarWidth(p,a)
#endif

#undef INTERFACE

HRESULT WINAPI CreateTextServices(IUnknown*,ITextHost*,IUnknown**);

typedef HRESULT (WINAPI *PCreateTextServices)(IUnknown*,ITextHost*,IUnknown**);

#ifdef __cplusplus
}
#endif

#endif /* _TEXTSERV_H */
