/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "commctrl.h"
#include "ole2.h"
#include "resource.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define TIMER_ID 0x1000

static const WCHAR wszInternetExplorer_Server[] =
    {'I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','_','S','e','r','v','e','r',0};

static const WCHAR wszTooltipData[] = {'t','o','o','l','t','i','p','_','d','a','t','a',0};

static ATOM serverwnd_class = 0;

typedef struct {
    HTMLDocument *doc;
    WNDPROC proc;
} tooltip_data;

static void paint_document(HTMLDocument *This)
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;

    GetClientRect(This->hwnd, &rect);

    hdc = BeginPaint(This->hwnd, &ps);

    if(!(This->hostinfo.dwFlags & (DOCHOSTUIFLAG_NO3DOUTERBORDER|DOCHOSTUIFLAG_NO3DBORDER)))
        DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT|BF_ADJUST);

    if(!This->nscontainer) {
        WCHAR wszHTMLDisabled[100];
        HFONT font;

        LoadStringW(hInst, IDS_HTMLDISABLED, wszHTMLDisabled, sizeof(wszHTMLDisabled)/sizeof(WCHAR));

        font = CreateFontA(25,0,0,0,400,0,0,0,ANSI_CHARSET,0,0,DEFAULT_QUALITY,DEFAULT_PITCH,NULL);

        SelectObject(hdc, font);
        SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW));

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        DrawTextW(hdc, wszHTMLDisabled,-1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

        DeleteObject(font);
    }

    EndPaint(This->hwnd, &ps);
}

static void activate_gecko(NSContainer *This)
{
    TRACE("(%p) %p\n", This, This->window);

    SetParent(This->hwnd, This->doc->hwnd);
    ShowWindow(This->hwnd, SW_SHOW);

    nsIBaseWindow_SetVisibility(This->window, TRUE);
    nsIBaseWindow_SetEnabled(This->window, TRUE);
    nsIWebBrowserFocus_Activate(This->focus);
}

void update_doc(HTMLDocument *This, DWORD flags)
{
    if(!This->update && This->hwnd)
        SetTimer(This->hwnd, TIMER_ID, 100, NULL);

    This->update |= flags;
}

void update_title(HTMLDocument *This)
{
    IOleCommandTarget *olecmd;
    HRESULT hres;

    if(!(This->update & UPDATE_TITLE))
        return;

    This->update &= ~UPDATE_TITLE;

    if(!This->client)
        return;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(SUCCEEDED(hres)) {
        VARIANT title;
        WCHAR empty[] = {0};

        V_VT(&title) = VT_BSTR;
        V_BSTR(&title) = SysAllocString(empty);
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETTITLE, OLECMDEXECOPT_DONTPROMPTUSER,
                               &title, NULL);
        SysFreeString(V_BSTR(&title));

        IOleCommandTarget_Release(olecmd);
    }
}

static LRESULT on_timer(HTMLDocument *This)
{
    TRACE("(%p) %x\n", This, This->update);

    KillTimer(This->hwnd, TIMER_ID);

    if(!This->update)
        return 0;

    if(This->update & UPDATE_UI) {
        if(This->hostui)
            IDocHostUIHandler_UpdateUI(This->hostui);

        if(This->client) {
            IOleCommandTarget *cmdtrg;
            HRESULT hres;

            hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget,
                                                 (void**)&cmdtrg);
            if(SUCCEEDED(hres)) {
                IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_UPDATECOMMANDS,
                                       OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
                IOleCommandTarget_Release(cmdtrg);
            }
        }
    }

    update_title(This);
    This->update = 0;
    return 0;
}

void notif_focus(HTMLDocument *This)
{
    IOleControlSite *site;
    HRESULT hres;

    if(!This->client)
        return;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IOleControlSite, (void**)&site);
    if(FAILED(hres))
        return;

    IOleControlSite_OnFocus(site, This->focus);
    IOleControlSite_Release(site);
}

static LRESULT WINAPI serverwnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HTMLDocument *This;

    static const WCHAR wszTHIS[] = {'T','H','I','S',0};

    if(msg == WM_CREATE) {
        This = *(HTMLDocument**)lParam;
        SetPropW(hwnd, wszTHIS, This);
    }else {
        This = GetPropW(hwnd, wszTHIS);
    }

    switch(msg) {
    case WM_CREATE:
        This->hwnd = hwnd;
        break;
    case WM_PAINT:
        paint_document(This);
        break;
    case WM_SIZE:
        TRACE("(%p)->(WM_SIZE)\n", This);
        if(This->nscontainer) {
            INT ew=0, eh=0;

            if(!(This->hostinfo.dwFlags & (DOCHOSTUIFLAG_NO3DOUTERBORDER|DOCHOSTUIFLAG_NO3DBORDER))) {
                ew = GetSystemMetrics(SM_CXEDGE);
                eh = GetSystemMetrics(SM_CYEDGE);
            }

            SetWindowPos(This->nscontainer->hwnd, NULL, ew, eh,
                         LOWORD(lParam) - 2*ew, HIWORD(lParam) - 2*eh,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    case WM_TIMER:
        return on_timer(This);
    case WM_MOUSEACTIVATE:
        return MA_ACTIVATE;
    }
        
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void register_serverwnd_class(void)
{
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        CS_DBLCLKS,
        serverwnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszInternetExplorer_Server,
        NULL,
    };
    wndclass.hInstance = hInst;
    serverwnd_class = RegisterClassExW(&wndclass);
}

static HRESULT activate_window(HTMLDocument *This)
{
    IOleInPlaceFrame *pIPFrame;
    IOleCommandTarget *cmdtrg;
    IOleInPlaceSiteEx *ipsiteex;
    RECT posrect, cliprect;
    OLEINPLACEFRAMEINFO frameinfo;
    HWND parent_hwnd;
    HRESULT hres;

    if(!serverwnd_class)
        register_serverwnd_class();

    hres = IOleInPlaceSite_CanInPlaceActivate(This->ipsite);
    if(hres != S_OK) {
        WARN("CanInPlaceActivate returned: %08x\n", hres);
        return FAILED(hres) ? hres : E_FAIL;
    }

    hres = IOleInPlaceSite_GetWindowContext(This->ipsite, &pIPFrame, &This->ip_window,
            &posrect, &cliprect, &frameinfo);
    if(FAILED(hres)) {
        WARN("GetWindowContext failed: %08x\n", hres);
        return hres;
    }

    TRACE("got window context: %p %p {%d %d %d %d} {%d %d %d %d} {%d %x %p %p %d}\n",
            pIPFrame, This->ip_window, posrect.left, posrect.top, posrect.right, posrect.bottom,
            cliprect.left, cliprect.top, cliprect.right, cliprect.bottom,
            frameinfo.cb, frameinfo.fMDIApp, frameinfo.hwndFrame, frameinfo.haccel, frameinfo.cAccelEntries);

    hres = IOleInPlaceSite_GetWindow(This->ipsite, &parent_hwnd);
    if(FAILED(hres)) {
        WARN("GetWindow failed: %08x\n", hres);
        return hres;
    }

    TRACE("got parent window %p\n", parent_hwnd);

    if(This->hwnd) {
        if(GetParent(This->hwnd) != parent_hwnd)
            SetParent(This->hwnd, parent_hwnd);
        SetWindowPos(This->hwnd, HWND_TOP,
                posrect.left, posrect.top, posrect.right-posrect.left, posrect.bottom-posrect.top,
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }else {
        CreateWindowExW(0, wszInternetExplorer_Server, NULL,
                WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                posrect.left, posrect.top, posrect.right-posrect.left, posrect.bottom-posrect.top,
                parent_hwnd, NULL, hInst, This);

        TRACE("Created window %p\n", This->hwnd);

        SetWindowPos(This->hwnd, NULL, 0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        RedrawWindow(This->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);

        /* NOTE:
         * Windows implementation calls:
         * RegisterWindowMessage("MSWHEEL_ROLLMSG");
         */
        SetTimer(This->hwnd, TIMER_ID, 100, NULL);
    }

    if(This->nscontainer)
        activate_gecko(This->nscontainer);

    This->in_place_active = TRUE;
    hres = IOleInPlaceSite_QueryInterface(This->ipsite, &IID_IOleInPlaceSiteEx, (void**)&ipsiteex);
    if(SUCCEEDED(hres)) {
        BOOL redraw = FALSE;

        hres = IOleInPlaceSiteEx_OnInPlaceActivateEx(ipsiteex, &redraw, 0);
        IOleInPlaceSiteEx_Release(ipsiteex);
        if(redraw)
            FIXME("unsupported redraw\n");
    }else{
        hres = IOleInPlaceSite_OnInPlaceActivate(This->ipsite);
    }
    if(FAILED(hres)) {
        WARN("OnInPlaceActivate failed: %08x\n", hres);
        This->in_place_active = FALSE;
        return hres;
    }

    hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT var;

        IOleInPlaceFrame_SetStatusText(pIPFrame, NULL);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSMAX,
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSPOS, 
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);

        IOleCommandTarget_Release(cmdtrg);
    }

    if(This->frame)
        IOleInPlaceFrame_Release(This->frame);
    This->frame = pIPFrame;

    This->window_active = TRUE;

    return S_OK;
}

static LRESULT WINAPI tooltips_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    tooltip_data *data = GetPropW(hwnd, wszTooltipData);

    TRACE("%d %p\n", msg, data);

    if(msg == TTM_WINDOWFROMPOINT) {
        RECT rect;
        POINT *pt = (POINT*)lParam;

        TRACE("TTM_WINDOWFROMPOINT (%d,%d)\n", pt->x, pt->y);

        GetWindowRect(data->doc->hwnd, &rect);

        if(rect.left <= pt->x && pt->x <= rect.right
           && rect.top <= pt->y && pt->y <= rect.bottom)
            return (LPARAM)data->doc->hwnd;
    }

    return CallWindowProcW(data->proc, hwnd, msg, wParam, lParam);
}

static void create_tooltips_window(HTMLDocument *This)
{
    tooltip_data *data = heap_alloc(sizeof(*data));

    This->tooltips_hwnd = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, TTS_NOPREFIX | WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, This->hwnd, NULL, hInst, NULL);

    data->doc = This;
    data->proc = (WNDPROC)GetWindowLongPtrW(This->tooltips_hwnd, GWLP_WNDPROC);

    SetPropW(This->tooltips_hwnd, wszTooltipData, data);

    SetWindowLongPtrW(This->tooltips_hwnd, GWLP_WNDPROC, (LONG_PTR)tooltips_proc);

    SetWindowPos(This->tooltips_hwnd, HWND_TOPMOST,0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

}

void show_tooltip(HTMLDocument *This, DWORD x, DWORD y, LPCWSTR text)
{
    TTTOOLINFOW toolinfo = {
        sizeof(TTTOOLINFOW), 0, This->hwnd, 0xdeadbeef,
        {x>2 ? x-2 : 0, y>0 ? y-2 : 0, x+2, y+2}, /* FIXME */
        NULL, (LPWSTR)text, 0};
    MSG msg = {This->hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x,y), 0, {x,y}};

    TRACE("(%p)->(%d %d %s)\n", This, x, y, debugstr_w(text));

    if(!This->tooltips_hwnd)
        create_tooltips_window(This);

    SendMessageW(This->tooltips_hwnd, TTM_ADDTOOLW, 0, (LPARAM)&toolinfo);
    SendMessageW(This->tooltips_hwnd, TTM_ACTIVATE, TRUE, 0);
    SendMessageW(This->tooltips_hwnd, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

void hide_tooltip(HTMLDocument *This)
{
    TTTOOLINFOW toolinfo = {
        sizeof(TTTOOLINFOW), 0, This->hwnd, 0xdeadbeef,
        {0,0,0,0}, NULL, NULL, 0};

    TRACE("(%p)\n", This);

    SendMessageW(This->tooltips_hwnd, TTM_DELTOOLW, 0, (LPARAM)&toolinfo);
    SendMessageW(This->tooltips_hwnd, TTM_ACTIVATE, FALSE, 0);
}

HRESULT call_set_active_object(IOleInPlaceUIWindow *window, IOleInPlaceActiveObject *act_obj)
{
    static WCHAR html_documentW[30];

    if(act_obj && !html_documentW[0]) {
        LoadStringW(hInst, IDS_HTMLDOCUMENT, html_documentW,
                    sizeof(html_documentW)/sizeof(WCHAR));
    }

    return IOleInPlaceFrame_SetActiveObject(window, act_obj, act_obj ? html_documentW : NULL);
}

/**********************************************************
 * IOleDocumentView implementation
 */

#define DOCVIEW_THIS(iface) DEFINE_THIS(HTMLDocument, OleDocumentView, iface)

static HRESULT WINAPI OleDocumentView_QueryInterface(IOleDocumentView *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleDocumentView_AddRef(IOleDocumentView *iface)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleDocumentView_Release(IOleDocumentView *iface)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleDocumentView_SetInPlaceSite(IOleDocumentView *iface, IOleInPlaceSite *pIPSite)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    TRACE("(%p)->(%p)\n", This, pIPSite);

    if(pIPSite)
        IOleInPlaceSite_AddRef(pIPSite);

    if(This->ipsite)
        IOleInPlaceSite_Release(This->ipsite);

    This->ipsite = pIPSite;
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetInPlaceSite(IOleDocumentView *iface, IOleInPlaceSite **ppIPSite)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    TRACE("(%p)->(%p)\n", This, ppIPSite);

    if(!ppIPSite)
        return E_INVALIDARG;

    if(This->ipsite)
        IOleInPlaceSite_AddRef(This->ipsite);

    *ppIPSite = This->ipsite;
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetDocument(IOleDocumentView *iface, IUnknown **ppunk)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    TRACE("(%p)->(%p)\n", This, ppunk);

    if(!ppunk)
        return E_INVALIDARG;

    IHTMLDocument2_AddRef(HTMLDOC(This));
    *ppunk = (IUnknown*)HTMLDOC(This);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SetRect(IOleDocumentView *iface, LPRECT prcView)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    RECT rect;

    TRACE("(%p)->(%p)\n", This, prcView);

    if(!prcView)
        return E_INVALIDARG;

    if(This->hwnd) {
        GetClientRect(This->hwnd, &rect);
        if(memcmp(prcView, &rect, sizeof(RECT))) {
            InvalidateRect(This->hwnd,NULL,TRUE);
            SetWindowPos(This->hwnd, NULL, prcView->left, prcView->top, prcView->right,
                    prcView->bottom, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetRect(IOleDocumentView *iface, LPRECT prcView)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);

    TRACE("(%p)->(%p)\n", This, prcView);

    if(!prcView)
        return E_INVALIDARG;

    GetClientRect(This->hwnd, prcView);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SetRectComplex(IOleDocumentView *iface, LPRECT prcView,
                        LPRECT prcHScroll, LPRECT prcVScroll, LPRECT prcSizeBox)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    FIXME("(%p)->(%p %p %p %p)\n", This, prcView, prcHScroll, prcVScroll, prcSizeBox);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_Show(IOleDocumentView *iface, BOOL fShow)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%x)\n", This, fShow);

    if(fShow) {
        if(!This->ui_active) {
            hres = activate_window(This);
            if(FAILED(hres))
                return hres;
        }
        update_doc(This, UPDATE_UI);
        ShowWindow(This->hwnd, SW_SHOW);
    }else {
        ShowWindow(This->hwnd, SW_HIDE);
        if(This->ip_window) {
            IOleInPlaceUIWindow_Release(This->ip_window);
            This->ip_window = NULL;
        }
    }

    return S_OK;
}

static HRESULT WINAPI OleDocumentView_UIActivate(IOleDocumentView *iface, BOOL fUIActivate)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%x)\n", This, fUIActivate);

    if(!This->ipsite) {
        FIXME("This->ipsite = NULL\n");
        return E_FAIL;
    }

    if(fUIActivate) {
        RECT rcBorderWidths;

        if(This->ui_active)
            return S_OK;

        if(!This->window_active) {
            hres = activate_window(This);
            if(FAILED(hres))
                return hres;
        }

        This->focus = TRUE;
        if(This->nscontainer)
            nsIWebBrowserFocus_Activate(This->nscontainer->focus);
        notif_focus(This);

        update_doc(This, UPDATE_UI);

        hres = IOleInPlaceSite_OnUIActivate(This->ipsite);
        if(SUCCEEDED(hres)) {
            call_set_active_object((IOleInPlaceUIWindow*)This->frame, ACTOBJ(This));
        }else {
            FIXME("OnUIActivate failed: %08x\n", hres);
            IOleInPlaceFrame_Release(This->frame);
            This->frame = NULL;
            This->ui_active = FALSE;
            return hres;
        }

        if(This->hostui) {
            hres = IDocHostUIHandler_ShowUI(This->hostui,
                    This->usermode == EDITMODE ? DOCHOSTUITYPE_AUTHOR : DOCHOSTUITYPE_BROWSE,
                    ACTOBJ(This), CMDTARGET(This), This->frame, This->ip_window);
            if(FAILED(hres))
                IDocHostUIHandler_HideUI(This->hostui);
        }

        if(This->ip_window)
            call_set_active_object(This->ip_window, ACTOBJ(This));

        memset(&rcBorderWidths, 0, sizeof(rcBorderWidths));
        IOleInPlaceFrame_SetBorderSpace(This->frame, &rcBorderWidths);

        This->ui_active = TRUE;
    }else {
        if(This->ui_active) {
            This->ui_active = FALSE;
            if(This->ip_window)
                call_set_active_object(This->ip_window, NULL);
            if(This->frame)
                call_set_active_object((IOleInPlaceUIWindow*)This->frame, NULL);
            if(This->hostui)
                IDocHostUIHandler_HideUI(This->hostui);
            if(This->ipsite)
                IOleInPlaceSite_OnUIDeactivate(This->ipsite, FALSE);
        }
    }
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_Open(IOleDocumentView *iface)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_CloseView(IOleDocumentView *iface, DWORD dwReserved)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    TRACE("(%p)->(%x)\n", This, dwReserved);

    if(dwReserved)
        WARN("dwReserved = %d\n", dwReserved);

    /* NOTE:
     * Windows implementation calls QueryInterface(IID_IOleCommandTarget),
     * QueryInterface(IID_IOleControlSite) and KillTimer
     */

    IOleDocumentView_Show(iface, FALSE);

    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SaveViewState(IOleDocumentView *iface, LPSTREAM pstm)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstm);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_ApplyViewState(IOleDocumentView *iface, LPSTREAM pstm)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pstm);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_Clone(IOleDocumentView *iface, IOleInPlaceSite *pIPSiteNew,
                                        IOleDocumentView **ppViewNew)
{
    HTMLDocument *This = DOCVIEW_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pIPSiteNew, ppViewNew);
    return E_NOTIMPL;
}

#undef DOCVIEW_THIS

static const IOleDocumentViewVtbl OleDocumentViewVtbl = {
    OleDocumentView_QueryInterface,
    OleDocumentView_AddRef,
    OleDocumentView_Release,
    OleDocumentView_SetInPlaceSite,
    OleDocumentView_GetInPlaceSite,
    OleDocumentView_GetDocument,
    OleDocumentView_SetRect,
    OleDocumentView_GetRect,
    OleDocumentView_SetRectComplex,
    OleDocumentView_Show,
    OleDocumentView_UIActivate,
    OleDocumentView_Open,
    OleDocumentView_CloseView,
    OleDocumentView_SaveViewState,
    OleDocumentView_ApplyViewState,
    OleDocumentView_Clone
};

/**********************************************************
 * IViewObject implementation
 */

#define VIEWOBJ_THIS(iface) DEFINE_THIS(HTMLDocument, ViewObject2, iface)

static HRESULT WINAPI ViewObject_QueryInterface(IViewObject2 *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI ViewObject_AddRef(IViewObject2 *iface)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI ViewObject_Release(IViewObject2 *iface)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI ViewObject_Draw(IViewObject2 *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect,
        DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds, BOOL (CALLBACK *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p %p %p %p %p %p %ld)\n", This, dwDrawAspect, lindex, pvAspect,
            ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetColorSet(IViewObject2 *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect,
        DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p %p %p)\n", This, dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, ppColorSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Freeze(IViewObject2 *iface, DWORD dwDrawAspect, LONG lindex,
        void *pvAspect, DWORD *pdwFreeze)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p)\n", This, dwDrawAspect, lindex, pvAspect, pdwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Unfreeze(IViewObject2 *iface, DWORD dwFreeze)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d)\n", This, dwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_SetAdvise(IViewObject2 *iface, DWORD aspects, DWORD advf, IAdviseSink *pAdvSink)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, aspects, advf, pAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetAdvise(IViewObject2 *iface, DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pAspects, pAdvf, ppAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetExtent(IViewObject2 *iface, DWORD dwDrawAspect, LONG lindex,
                                DVTARGETDEVICE* ptd, LPSIZEL lpsizel)
{
    HTMLDocument *This = VIEWOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p %p)\n", This, dwDrawAspect, lindex, ptd, lpsizel);
    return E_NOTIMPL;
}

#undef VIEWOBJ_THIS

static const IViewObject2Vtbl ViewObjectVtbl = {
    ViewObject_QueryInterface,
    ViewObject_AddRef,
    ViewObject_Release,
    ViewObject_Draw,
    ViewObject_GetColorSet,
    ViewObject_Freeze,
    ViewObject_Unfreeze,
    ViewObject_SetAdvise,
    ViewObject_GetAdvise,
    ViewObject_GetExtent
};

void HTMLDocument_View_Init(HTMLDocument *This)
{
    This->lpOleDocumentViewVtbl = &OleDocumentViewVtbl;
    This->lpViewObject2Vtbl = &ViewObjectVtbl;

    This->ipsite = NULL;
    This->frame = NULL;
    This->ip_window = NULL;
    This->hwnd = NULL;
    This->tooltips_hwnd = NULL;

    This->in_place_active = FALSE;
    This->ui_active = FALSE;
    This->window_active = FALSE;
    This->focus = FALSE;

    This->update = 0;
}
