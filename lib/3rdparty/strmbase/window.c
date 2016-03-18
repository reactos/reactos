/*
 * Generic Implementation of strmbase window classes
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

static inline BaseControlWindow *impl_from_IVideoWindow( IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, BaseControlWindow, IVideoWindow_iface);
}

static inline BaseControlWindow *impl_from_BaseWindow(BaseWindow *iface)
{
    return CONTAINING_RECORD(iface, BaseControlWindow, baseWindow);
}

static LRESULT CALLBACK WndProcW(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BaseWindow* This = (BaseWindow*)GetWindowLongPtrW(hwnd, 0);

    if (!This)
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    if (This->pFuncsTable->pfnOnReceiveMessage)
        return This->pFuncsTable->pfnOnReceiveMessage(This, hwnd, uMsg, wParam, lParam);
    else
        return BaseWindowImpl_OnReceiveMessage(This, hwnd, uMsg, wParam, lParam);
}

LRESULT WINAPI BaseWindowImpl_OnReceiveMessage(BaseWindow *This, HWND hwnd, INT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (This->pFuncsTable->pfnPossiblyEatMessage && This->pFuncsTable->pfnPossiblyEatMessage(This, uMsg, wParam, lParam))
        return 0;

    switch (uMsg)
    {
        case WM_SIZE:
            if (This->pFuncsTable->pfnOnSize)
                return This->pFuncsTable->pfnOnSize(This, LOWORD(lParam), HIWORD(lParam));
            else
                return BaseWindowImpl_OnSize(This, LOWORD(lParam), HIWORD(lParam));
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL WINAPI BaseWindowImpl_OnSize(BaseWindow *This, LONG Width, LONG Height)
{
    This->Width = Width;
    This->Height = Height;
    return TRUE;
}

HRESULT WINAPI BaseWindow_Init(BaseWindow *pBaseWindow, const BaseWindowFuncTable* pFuncsTable)
{
    if (!pFuncsTable)
        return E_INVALIDARG;

    ZeroMemory(pBaseWindow,sizeof(BaseWindow));
    pBaseWindow->pFuncsTable = pFuncsTable;

    return S_OK;
}

HRESULT WINAPI BaseWindow_Destroy(BaseWindow *This)
{
    if (This->hWnd)
        BaseWindowImpl_DoneWithWindow(This);

    HeapFree(GetProcessHeap(), 0, This);
    return S_OK;
}

HRESULT WINAPI BaseWindowImpl_PrepareWindow(BaseWindow *This)
{
    WNDCLASSW winclass;
    static const WCHAR windownameW[] = { 'A','c','t','i','v','e','M','o','v','i','e',' ','W','i','n','d','o','w',0 };

    This->pClassName = This->pFuncsTable->pfnGetClassWindowStyles(This, &This->ClassStyles, &This->WindowStyles, &This->WindowStylesEx);

    winclass.style = This->ClassStyles;
    winclass.lpfnWndProc = WndProcW;
    winclass.cbClsExtra = 0;
    winclass.cbWndExtra = sizeof(BaseWindow*);
    winclass.hInstance = This->hInstance;
    winclass.hIcon = NULL;
    winclass.hCursor = NULL;
    winclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    winclass.lpszMenuName = NULL;
    winclass.lpszClassName = This->pClassName;
    if (!RegisterClassW(&winclass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR("Unable to register window class: %u\n", GetLastError());
        return E_FAIL;
    }

    This->hWnd = CreateWindowExW(This->WindowStylesEx,
                                 This->pClassName, windownameW,
                                 This->WindowStyles,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 CW_USEDEFAULT, NULL, NULL, This->hInstance,
                                 NULL);

    if (!This->hWnd)
    {
        ERR("Unable to create window\n");
        return E_FAIL;
    }

    SetWindowLongPtrW(This->hWnd, 0, (LONG_PTR)This);

    This->hDC = GetDC(This->hWnd);

    return S_OK;
}

HRESULT WINAPI BaseWindowImpl_DoneWithWindow(BaseWindow *This)
{
    if (!This->hWnd)
        return S_OK;

    if (This->hDC)
        ReleaseDC(This->hWnd, This->hDC);
    This->hDC = NULL;

    DestroyWindow(This->hWnd);
    This->hWnd = NULL;

    return S_OK;
}

RECT WINAPI BaseWindowImpl_GetDefaultRect(BaseWindow *This)
{
    static RECT defRect = {0, 0, 800, 600};
    return defRect;
}

BOOL WINAPI BaseControlWindowImpl_PossiblyEatMessage(BaseWindow *This, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BaseControlWindow* pControlWindow = impl_from_BaseWindow(This);

    if (pControlWindow->hwndDrain)
    {
        switch(uMsg)
        {
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEACTIVATE:
            case WM_MOUSEMOVE:
            case WM_NCLBUTTONDBLCLK:
            case WM_NCLBUTTONDOWN:
            case WM_NCLBUTTONUP:
            case WM_NCMBUTTONDBLCLK:
            case WM_NCMBUTTONDOWN:
            case WM_NCMBUTTONUP:
            case WM_NCMOUSEMOVE:
            case WM_NCRBUTTONDBLCLK:
            case WM_NCRBUTTONDOWN:
            case WM_NCRBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                PostMessageW(pControlWindow->hwndDrain, uMsg, wParam, lParam);
                return TRUE;
            default:
                break;
        }
    }
    return FALSE;
}

HRESULT WINAPI BaseControlWindow_Init(BaseControlWindow *pControlWindow, const IVideoWindowVtbl *lpVtbl, BaseFilter *owner, CRITICAL_SECTION *lock, BasePin* pPin,const BaseWindowFuncTable *pFuncsTable)
{
    HRESULT hr;

    hr = BaseWindow_Init(&pControlWindow->baseWindow, pFuncsTable);
    if (SUCCEEDED(hr))
    {
        BaseDispatch_Init(&pControlWindow->baseDispatch, &IID_IVideoWindow);
        pControlWindow->IVideoWindow_iface.lpVtbl = lpVtbl;
        pControlWindow->AutoShow = TRUE;
        pControlWindow->hwndDrain = NULL;
        pControlWindow->hwndOwner = NULL;
        pControlWindow->pFilter = owner;
        pControlWindow->pInterfaceLock = lock;
        pControlWindow->pPin = pPin;
    }
    return hr;
}

HRESULT WINAPI BaseControlWindow_Destroy(BaseControlWindow *pControlWindow)
{
    BaseWindowImpl_DoneWithWindow(&pControlWindow->baseWindow);
    return BaseDispatch_Destroy(&pControlWindow->baseDispatch);
}

HRESULT WINAPI BaseControlWindowImpl_GetTypeInfoCount(IVideoWindow *iface, UINT *pctinfo)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    return BaseDispatchImpl_GetTypeInfoCount(&This->baseDispatch, pctinfo);
}

HRESULT WINAPI BaseControlWindowImpl_GetTypeInfo(IVideoWindow *iface, UINT iTInfo, LCID lcid, ITypeInfo**ppTInfo)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    return BaseDispatchImpl_GetTypeInfo(&This->baseDispatch, &IID_NULL, iTInfo, lcid, ppTInfo);
}

HRESULT WINAPI BaseControlWindowImpl_GetIDsOfNames(IVideoWindow *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
 {
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    return BaseDispatchImpl_GetIDsOfNames(&This->baseDispatch, riid, rgszNames, cNames, lcid, rgDispId);
}

HRESULT WINAPI BaseControlWindowImpl_Invoke(IVideoWindow *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    ITypeInfo *pTypeInfo;
    HRESULT hr;

    hr = BaseDispatchImpl_GetTypeInfo(&This->baseDispatch, riid, 1, lcid, &pTypeInfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(pTypeInfo, &This->IVideoWindow_iface, dispIdMember, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
        ITypeInfo_Release(pTypeInfo);
    }

    return hr;
}

HRESULT WINAPI BaseControlWindowImpl_put_Caption(IVideoWindow *iface, BSTR strCaption)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    if (!SetWindowTextW(This->baseWindow.hWnd, strCaption))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Caption(IVideoWindow *iface, BSTR *strCaption)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    GetWindowTextW(This->baseWindow.hWnd, (LPWSTR)strCaption, 100);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    LONG old;

    old = GetWindowLongW(This->baseWindow.hWnd, GWL_STYLE);

    TRACE("(%p/%p)->(%x -> %x)\n", This, iface, old, WindowStyle);

    if (WindowStyle & (WS_DISABLED|WS_HSCROLL|WS_ICONIC|WS_MAXIMIZE|WS_MINIMIZE|WS_VSCROLL))
        return E_INVALIDARG;

    SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, WindowStyle);
    SetWindowPos(This->baseWindow.hWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOZORDER);
    This->baseWindow.WindowStyles = WindowStyle;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_WindowStyle(IVideoWindow *iface, LONG *WindowStyle)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    *WindowStyle = This->baseWindow.WindowStyles;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowStyleEx);

    if (!SetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE, WindowStyleEx))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    *WindowStyleEx = GetWindowLongW(This->baseWindow.hWnd, GWL_EXSTYLE);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_AutoShow(IVideoWindow *iface, LONG AutoShow)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, AutoShow);

    This->AutoShow = AutoShow;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_AutoShow(IVideoWindow *iface, LONG *AutoShow)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    *AutoShow = This->AutoShow;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_WindowState(IVideoWindow *iface, LONG WindowState)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, WindowState);
    ShowWindow(This->baseWindow.hWnd, WindowState);
    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_WindowState(IVideoWindow *iface, LONG *WindowState)
{
    WINDOWPLACEMENT place;
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    place.length = sizeof(place);
    GetWindowPlacement(This->baseWindow.hWnd, &place);
    TRACE("(%p/%p)->(%p)\n", This, iface, WindowState);
    *WindowState = place.showCmd;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, BackgroundPalette);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_BackgroundPalette(IVideoWindow *iface, LONG *pBackgroundPalette)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pBackgroundPalette);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Visible(IVideoWindow *iface, LONG Visible)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Visible);

    ShowWindow(This->baseWindow.hWnd, Visible ? SW_SHOW : SW_HIDE);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Visible(IVideoWindow *iface, LONG *pVisible)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    *pVisible = IsWindowVisible(This->baseWindow.hWnd);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Left(IVideoWindow *iface, LONG Left)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%d)\n", This, iface, Left);

    GetWindowRect(This->baseWindow.hWnd, &WindowPos);
    if (!SetWindowPos(This->baseWindow.hWnd, NULL, Left, WindowPos.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Left(IVideoWindow *iface, LONG *pLeft)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    *pLeft = WindowPos.left;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Width(IVideoWindow *iface, LONG Width)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Width);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, 0, 0, Width, This->baseWindow.Height, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->baseWindow.Width = Width;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Width(IVideoWindow *iface, LONG *pWidth)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    *pWidth = This->baseWindow.Width;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Top(IVideoWindow *iface, LONG Top)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%d)\n", This, iface, Top);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, WindowPos.left, Top, 0, 0, SWP_NOZORDER|SWP_NOSIZE))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Top(IVideoWindow *iface, LONG *pTop)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    *pTop = WindowPos.top;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Height(IVideoWindow *iface, LONG Height)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, Height);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, 0, 0, This->baseWindow.Width, Height, SWP_NOZORDER|SWP_NOMOVE))
        return E_FAIL;

    This->baseWindow.Height = Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Height(IVideoWindow *iface, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    *pHeight = This->baseWindow.Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_Owner(IVideoWindow *iface, OAHWND Owner)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Owner);

    This->hwndOwner = (HWND)Owner;
    SetParent(This->baseWindow.hWnd, This->hwndOwner);
    if (This->baseWindow.WindowStyles & WS_CHILD)
    {
        LONG old = GetWindowLongW(This->baseWindow.hWnd, GWL_STYLE);
        if (old != This->baseWindow.WindowStyles)
        {
            SetWindowLongW(This->baseWindow.hWnd, GWL_STYLE, This->baseWindow.WindowStyles);
            SetWindowPos(This->baseWindow.hWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOZORDER);
        }
    }

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_Owner(IVideoWindow *iface, OAHWND *Owner)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    *(HWND*)Owner = This->hwndOwner;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_MessageDrain(IVideoWindow *iface, OAHWND Drain)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%08x)\n", This, iface, (DWORD) Drain);

    This->hwndDrain = (HWND)Drain;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    *Drain = (OAHWND)This->hwndDrain;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_BorderColor(IVideoWindow *iface, LONG *Color)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, Color);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_put_BorderColor(IVideoWindow *iface, LONG Color)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, Color);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode)
{
    TRACE("(%p)->(%p)\n", iface, FullScreenMode);

    return E_NOTIMPL;
}

HRESULT WINAPI BaseControlWindowImpl_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode)
{
    TRACE("(%p)->(%d)\n", iface, FullScreenMode);
    return E_NOTIMPL;
}

HRESULT WINAPI BaseControlWindowImpl_SetWindowForeground(IVideoWindow *iface, LONG Focus)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    BOOL ret;
    IPin* pPin;
    HRESULT hr;

    TRACE("(%p/%p)->(%d)\n", This, iface, Focus);

    if ((Focus != FALSE) && (Focus != TRUE))
        return E_INVALIDARG;

    hr = IPin_ConnectedTo(&This->pPin->IPin_iface, &pPin);
    if ((hr != S_OK) || !pPin)
        return VFW_E_NOT_CONNECTED;

    if (Focus)
        ret = SetForegroundWindow(This->baseWindow.hWnd);
    else
        ret = SetWindowPos(This->baseWindow.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

    if (!ret)
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top, LONG Width, LONG Height)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%d, %d, %d, %d)\n", This, iface, Left, Top, Width, Height);

    if (!SetWindowPos(This->baseWindow.hWnd, NULL, Left, Top, Width, Height, SWP_NOZORDER))
        return E_FAIL;

    This->baseWindow.Width = Width;
    This->baseWindow.Height = Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT WindowPos;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);
    GetWindowRect(This->baseWindow.hWnd, &WindowPos);

    *pLeft = WindowPos.left;
    *pTop = WindowPos.top;
    *pWidth = This->baseWindow.Width;
    *pHeight = This->baseWindow.Height;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_NotifyOwnerMessage(IVideoWindow *iface, OAHWND hwnd, LONG uMsg, LONG_PTR wParam, LONG_PTR lParam)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    TRACE("(%p/%p)->(%08lx, %d, %08lx, %08lx)\n", This, iface, hwnd, uMsg, wParam, lParam);

    if (!PostMessageW(This->baseWindow.hWnd, uMsg, wParam, lParam))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT defaultRect;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);
    defaultRect = This->baseWindow.pFuncsTable->pfnGetDefaultRect(&This->baseWindow);

    *pWidth = defaultRect.right - defaultRect.left;
    *pHeight = defaultRect.bottom - defaultRect.top;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);
    RECT defaultRect;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);
    defaultRect = This->baseWindow.pFuncsTable->pfnGetDefaultRect(&This->baseWindow);

    *pWidth = defaultRect.right - defaultRect.left;
    *pHeight = defaultRect.bottom - defaultRect.top;

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p, %p, %p, %p): stub !!!\n", This, iface, pLeft, pTop, pWidth, pHeight);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_HideCursor(IVideoWindow *iface, LONG HideCursor)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%d): stub !!!\n", This, iface, HideCursor);

    return S_OK;
}

HRESULT WINAPI BaseControlWindowImpl_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden)
{
    BaseControlWindow*  This = impl_from_IVideoWindow(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, CursorHidden);

    return S_OK;
}
