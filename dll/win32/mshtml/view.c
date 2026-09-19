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
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define TIMER_ID 0x1000

static ATOM serverwnd_class = 0;

typedef struct {
    HTMLDocumentObj *doc;
    WNDPROC proc;
} tooltip_data;

static void paint_document(HTMLDocumentObj *This)
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;

    if(This->window && This->window->base.inner_window && !This->window->base.inner_window->first_paint_time)
        This->window->base.inner_window->first_paint_time = get_time_stamp();

    GetClientRect(This->hwnd, &rect);

    hdc = BeginPaint(This->hwnd, &ps);

    if(!(This->hostinfo.dwFlags & (DOCHOSTUIFLAG_NO3DOUTERBORDER|DOCHOSTUIFLAG_NO3DBORDER)))
        DrawEdge(hdc, &rect, EDGE_SUNKEN, BF_RECT|BF_ADJUST);

    EndPaint(This->hwnd, &ps);
}

static void activate_gecko(GeckoBrowser *This)
{
    TRACE("(%p) %p\n", This, This->window);

    SetParent(This->hwnd, This->doc->hwnd);
    ShowWindow(This->hwnd, SW_SHOW);

    nsIBaseWindow_SetVisibility(This->window, TRUE);
    nsIBaseWindow_SetEnabled(This->window, TRUE);
}

void update_doc(HTMLDocumentObj *This, DWORD flags)
{
    if(!This->update && This->hwnd)
        SetTimer(This->hwnd, TIMER_ID, 100, NULL);

    This->update |= flags;
}

void update_title(HTMLDocumentObj *This)
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

        V_VT(&title) = VT_BSTR;
        V_BSTR(&title) = SysAllocString(L"");
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETTITLE, OLECMDEXECOPT_DONTPROMPTUSER,
                               &title, NULL);
        SysFreeString(V_BSTR(&title));

        IOleCommandTarget_Release(olecmd);
    }
}

static LRESULT on_timer(HTMLDocumentObj *This)
{
    TRACE("(%p) %lx\n", This, This->update);

    KillTimer(This->hwnd, TIMER_ID);

    if(!This->update)
        return 0;
    IUnknown_AddRef(This->outer_unk);

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

    IUnknown_Release(This->outer_unk);
    return 0;
}

void notif_focus(HTMLDocumentObj *This)
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
    HTMLDocumentObj *This;

    if(msg == WM_CREATE) {
        This = *(HTMLDocumentObj**)lParam;
        SetPropW(hwnd, L"THIS", This);
    }else {
        This = GetPropW(hwnd, L"THIS");
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
    case WM_SETFOCUS:
        TRACE("(%p) WM_SETFOCUS\n", This);
        nsIWebBrowserFocus_Activate(This->nscontainer->focus);
        break;
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
        L"Internet Explorer_Server",
        NULL,
    };
    wndclass.hInstance = hInst;
    serverwnd_class = RegisterClassExW(&wndclass);
}

static HRESULT activate_window(HTMLDocumentObj *This)
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
        WARN("CanInPlaceActivate returned: %08lx\n", hres);
        return FAILED(hres) ? hres : E_FAIL;
    }

    frameinfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    hres = IOleInPlaceSite_GetWindowContext(This->ipsite, &pIPFrame, &This->ip_window,
            &posrect, &cliprect, &frameinfo);
    if(FAILED(hres)) {
        WARN("GetWindowContext failed: %08lx\n", hres);
        return hres;
    }

    TRACE("got window context: %p %p %s %s {%d %x %p %p %d}\n",
            pIPFrame, This->ip_window, wine_dbgstr_rect(&posrect), wine_dbgstr_rect(&cliprect),
            frameinfo.cb, frameinfo.fMDIApp, frameinfo.hwndFrame, frameinfo.haccel, frameinfo.cAccelEntries);

    hres = IOleInPlaceSite_GetWindow(This->ipsite, &parent_hwnd);
    if(FAILED(hres)) {
        WARN("GetWindow failed: %08lx\n", hres);
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
        CreateWindowExW(0, L"Internet Explorer_Server", NULL,
                WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                posrect.left, posrect.top, posrect.right-posrect.left, posrect.bottom-posrect.top,
                parent_hwnd, NULL, hInst, This);

        TRACE("Created window %p\n", This->hwnd);

        SetWindowPos(This->hwnd, NULL, 0, 0, 0, 0,
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        RedrawWindow(This->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);
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
        WARN("OnInPlaceActivate failed: %08lx\n", hres);
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

    if(!This->request_uiactivate) {
        hres = IOleInPlaceSite_QueryInterface(This->ipsite, &IID_IOleInPlaceSiteEx, (void**)&ipsiteex);
        if(SUCCEEDED(hres)) {
            IOleInPlaceSiteEx_RequestUIActivate(ipsiteex);
            IOleInPlaceSiteEx_Release(ipsiteex);
        }
    }

    This->window_active = TRUE;

    return S_OK;
}

static LRESULT WINAPI tooltips_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    tooltip_data *data = GetPropW(hwnd, L"tooltip_data");

    TRACE("%d %p\n", msg, data);

    if(msg == TTM_WINDOWFROMPOINT) {
        RECT rect;
        POINT *pt = (POINT*)lParam;

        TRACE("TTM_WINDOWFROMPOINT (%ld,%ld)\n", pt->x, pt->y);

        GetWindowRect(data->doc->hwnd, &rect);

        if(rect.left <= pt->x && pt->x <= rect.right
           && rect.top <= pt->y && pt->y <= rect.bottom)
            return (LPARAM)data->doc->hwnd;
    }

    return CallWindowProcW(data->proc, hwnd, msg, wParam, lParam);
}

static void create_tooltips_window(HTMLDocumentObj *This)
{
    tooltip_data *data = malloc(sizeof(*data));

    This->tooltips_hwnd = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, TTS_NOPREFIX | WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, This->hwnd, NULL, hInst, NULL);

    data->doc = This;
    data->proc = (WNDPROC)GetWindowLongPtrW(This->tooltips_hwnd, GWLP_WNDPROC);

    SetPropW(This->tooltips_hwnd, L"tooltip_data", data);

    SetWindowLongPtrW(This->tooltips_hwnd, GWLP_WNDPROC, (LONG_PTR)tooltips_proc);

    SetWindowPos(This->tooltips_hwnd, HWND_TOPMOST,0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

}

void show_tooltip(HTMLDocumentObj *This, DWORD x, DWORD y, LPCWSTR text)
{
    TTTOOLINFOW toolinfo = {
        sizeof(TTTOOLINFOW), 0, This->hwnd, 0xdeadbeef,
        {x>2 ? x-2 : 0, y>0 ? y-2 : 0, x+2, y+2}, /* FIXME */
        NULL, (LPWSTR)text, 0};
    MSG msg = {This->hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(x,y), 0, {x,y}};

    TRACE("(%p)->(%ld %ld %s)\n", This, x, y, debugstr_w(text));

    if(!This->tooltips_hwnd)
        create_tooltips_window(This);

    SendMessageW(This->tooltips_hwnd, TTM_ADDTOOLW, 0, (LPARAM)&toolinfo);
    SendMessageW(This->tooltips_hwnd, TTM_ACTIVATE, TRUE, 0);
    SendMessageW(This->tooltips_hwnd, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

void hide_tooltip(HTMLDocumentObj *This)
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

    if(act_obj && !html_documentW[0])
        LoadStringW(hInst, IDS_HTMLDOCUMENT, html_documentW, ARRAY_SIZE(html_documentW));

    return IOleInPlaceUIWindow_SetActiveObject(window, act_obj, act_obj ? html_documentW : NULL);
}

static unsigned get_window_list_num(HTMLInnerWindow *window)
{
    HTMLOuterWindow *child;
    unsigned ret = 1;

    LIST_FOR_EACH_ENTRY(child, &window->children, HTMLOuterWindow, sibling_entry)
        ret += get_window_list_num(child->base.inner_window);
    return ret;
}

static HTMLInnerWindow **get_window_list(HTMLInnerWindow *window, HTMLInnerWindow **output)
{
    HTMLOuterWindow *child;

    *output++ = window;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    LIST_FOR_EACH_ENTRY(child, &window->children, HTMLOuterWindow, sibling_entry)
        output = get_window_list(child->base.inner_window, output);
    return output;
}

static void send_unload_events(HTMLDocumentObj *doc)
{
    HTMLInnerWindow **windows, *window;
    DOMEvent *event;
    unsigned i, num;
    HRESULT hres;

    if(!doc->window || !doc->doc_node->content_ready)
        return;
    window = doc->window->base.inner_window;

    /* Grab list of all windows ahead, and keep refs,
       since it can be detached from under our feet. */
    num = get_window_list_num(window);
    if(!(windows = malloc(num * sizeof(*windows))))
        return;
    get_window_list(window, windows);

    for(i = 0; i < num; i++) {
        window = windows[i];

        if(window->doc && !window->doc->unload_sent) {
            window->doc->unload_sent = TRUE;

            /* Native sends pagehide events prior to unload on the same window
               before it moves on to the next window, so they're interleaved. */
            if(window->doc->document_mode >= COMPAT_MODE_IE11) {
                hres = create_document_event(window->doc, EVENTID_PAGEHIDE, &event);
                if(SUCCEEDED(hres)) {
                    dispatch_event(&window->event_target, event);
                    IDOMEvent_Release(&event->IDOMEvent_iface);
                }
            }

            hres = create_document_event(window->doc, EVENTID_UNLOAD, &event);
            if(SUCCEEDED(hres)) {
                dispatch_event(&window->event_target, event);
                IDOMEvent_Release(&event->IDOMEvent_iface);
            }
        }

        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }

    free(windows);
}

/**********************************************************
 * IOleDocumentView implementation
 */

static inline HTMLDocumentObj *impl_from_IOleDocumentView(IOleDocumentView *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleDocumentView_iface);
}

static HRESULT WINAPI OleDocumentView_QueryInterface(IOleDocumentView *iface, REFIID riid, void **ppvObject)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObject);
}

static ULONG WINAPI OleDocumentView_AddRef(IOleDocumentView *iface)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI OleDocumentView_Release(IOleDocumentView *iface)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI OleDocumentView_SetInPlaceSite(IOleDocumentView *iface, IOleInPlaceSite *pIPSite)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    TRACE("(%p)->(%p)\n", This, pIPSite);

    if(pIPSite)
        IOleInPlaceSite_AddRef(pIPSite);

    if(This->ipsite)
        IOleInPlaceSite_Release(This->ipsite);

    This->ipsite = pIPSite;
    This->request_uiactivate = TRUE;
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetInPlaceSite(IOleDocumentView *iface, IOleInPlaceSite **ppIPSite)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
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
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    TRACE("(%p)->(%p)\n", This, ppunk);

    if(!ppunk)
        return E_INVALIDARG;

    *ppunk = (IUnknown*)&This->IHTMLDocument2_iface;
    IUnknown_AddRef(*ppunk);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SetRect(IOleDocumentView *iface, LPRECT prcView)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    RECT rect;

    TRACE("(%p)->(%p)\n", This, prcView);

    if(!prcView)
        return E_INVALIDARG;

    if(This->hwnd) {
        GetClientRect(This->hwnd, &rect);
        if(!EqualRect(prcView, &rect)) {
            InvalidateRect(This->hwnd, NULL, TRUE);
            SetWindowPos(This->hwnd, NULL, prcView->left, prcView->top, prcView->right - prcView->left,
                    prcView->bottom - prcView->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_GetRect(IOleDocumentView *iface, LPRECT prcView)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);

    TRACE("(%p)->(%p)\n", This, prcView);

    if(!prcView)
        return E_INVALIDARG;

    GetClientRect(This->hwnd, prcView);
    MapWindowPoints(This->hwnd, GetParent(This->hwnd), (POINT*)prcView, 2);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SetRectComplex(IOleDocumentView *iface, LPRECT prcView,
                        LPRECT prcHScroll, LPRECT prcVScroll, LPRECT prcSizeBox)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    FIXME("(%p)->(%p %p %p %p)\n", This, prcView, prcHScroll, prcVScroll, prcSizeBox);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_Show(IOleDocumentView *iface, BOOL fShow)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
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

        if(This->in_place_active)
            IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->IOleInPlaceObjectWindowless_iface);

        unlink_ref(&This->ip_window);
    }

    return S_OK;
}

static HRESULT WINAPI OleDocumentView_UIActivate(IOleDocumentView *iface, BOOL fUIActivate)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    HRESULT hres;

    TRACE("(%p)->(%x)\n", This, fUIActivate);

    if(!This->ipsite) {
        IOleClientSite *cs = This->client;
        IOleInPlaceSite *ips;

        if(!cs) {
            WARN("this->ipsite = NULL\n");
            return E_UNEXPECTED;
        }

        hres = IOleClientSite_QueryInterface(cs, &IID_IOleInPlaceSiteWindowless, (void**)&ips);
        if(SUCCEEDED(hres))
            This->ipsite = ips;
        else {
            hres = IOleClientSite_QueryInterface(cs, &IID_IOleInPlaceSiteEx, (void**)&ips);
            if(SUCCEEDED(hres))
                This->ipsite = ips;
            else {
                hres = IOleClientSite_QueryInterface(cs, &IID_IOleInPlaceSite, (void**)&ips);
                if(SUCCEEDED(hres))
                    This->ipsite = ips;
                else {
                    WARN("this->ipsite = NULL\n");
                    return E_NOINTERFACE;
                }
            }
        }

        IOleInPlaceSite_AddRef(This->ipsite);
        This->request_uiactivate = FALSE;
        HTMLDocument_LockContainer(This, TRUE);
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
            call_set_active_object((IOleInPlaceUIWindow*)This->frame, &This->IOleInPlaceActiveObject_iface);
        }else {
            FIXME("OnUIActivate failed: %08lx\n", hres);
            IOleInPlaceFrame_Release(This->frame);
            This->frame = NULL;
            This->ui_active = FALSE;
            return hres;
        }

        if(This->hostui) {
            hres = IDocHostUIHandler_ShowUI(This->hostui,
                    This->nscontainer->usermode == EDITMODE ? DOCHOSTUITYPE_AUTHOR : DOCHOSTUITYPE_BROWSE,
                    &This->IOleInPlaceActiveObject_iface, &This->IOleCommandTarget_iface,
                    This->frame, This->ip_window);
            if(FAILED(hres))
                IDocHostUIHandler_HideUI(This->hostui);
        }

        if(This->ip_window)
            call_set_active_object(This->ip_window, &This->IOleInPlaceActiveObject_iface);

        SetRectEmpty(&rcBorderWidths);
        IOleInPlaceFrame_SetBorderSpace(This->frame, &rcBorderWidths);

        This->ui_active = TRUE;
    }else {
        This->focus = FALSE;
        nsIWebBrowserFocus_Deactivate(This->nscontainer->focus);
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
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_CloseView(IOleDocumentView *iface, DWORD dwReserved)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    TRACE("(%p)->(%lx)\n", This, dwReserved);

    if(dwReserved)
        WARN("dwReserved = %ld\n", dwReserved);

    send_unload_events(This);
    IOleDocumentView_Show(iface, FALSE);
    return S_OK;
}

static HRESULT WINAPI OleDocumentView_SaveViewState(IOleDocumentView *iface, IStream *pstm)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    FIXME("(%p)->(%p)\n", This, pstm);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_ApplyViewState(IOleDocumentView *iface, IStream *pstm)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    FIXME("(%p)->(%p)\n", This, pstm);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocumentView_Clone(IOleDocumentView *iface, IOleInPlaceSite *pIPSiteNew,
                                        IOleDocumentView **ppViewNew)
{
    HTMLDocumentObj *This = impl_from_IOleDocumentView(iface);
    FIXME("(%p)->(%p %p)\n", This, pIPSiteNew, ppViewNew);
    return E_NOTIMPL;
}

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

static inline HTMLDocumentObj *impl_from_IViewObjectEx(IViewObjectEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IViewObjectEx_iface);
}

static HRESULT WINAPI ViewObject_QueryInterface(IViewObjectEx *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI ViewObject_AddRef(IViewObjectEx *iface)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI ViewObject_Release(IViewObjectEx *iface)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI ViewObject_Draw(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect,
        DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw, LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds, BOOL (CALLBACK *pfnContinue)(ULONG_PTR dwContinue), ULONG_PTR dwContinue)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p %p %p %p %Id)\n", This, dwDrawAspect, lindex, pvAspect,
            ptd, hdcTargetDev, hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetColorSet(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex, void *pvAspect,
        DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, ppColorSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Freeze(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex,
        void *pvAspect, DWORD *pdwFreeze)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %ld %p %p)\n", This, dwDrawAspect, lindex, pvAspect, pdwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_Unfreeze(IViewObjectEx *iface, DWORD dwFreeze)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld)\n", This, dwFreeze);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_SetAdvise(IViewObjectEx *iface, DWORD aspects, DWORD advf, IAdviseSink *pAdvSink)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);

    TRACE("(%p)->(%ld %ld %p)\n", This, aspects, advf, pAdvSink);

    if(aspects != DVASPECT_CONTENT || advf != ADVF_PRIMEFIRST)
        FIXME("unsupported arguments\n");

    if(This->view_sink)
        IAdviseSink_Release(This->view_sink);
    if(pAdvSink)
        IAdviseSink_AddRef(pAdvSink);

    This->view_sink = pAdvSink;
    return S_OK;
}

static HRESULT WINAPI ViewObject_GetAdvise(IViewObjectEx *iface, DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pAspects, pAdvf, ppAdvSink);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetExtent(IViewObjectEx *iface, DWORD dwDrawAspect, LONG lindex,
                                DVTARGETDEVICE* ptd, LPSIZEL lpsizel)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %ld %p %p)\n", This, dwDrawAspect, lindex, ptd, lpsizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetRect(IViewObjectEx *iface, DWORD dwAspect, LPRECTL pRect)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwAspect, pRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetViewStatus(IViewObjectEx *iface, DWORD *pdwStatus)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_QueryHitPoint(IViewObjectEx* iface, DWORD dwAspect,
        LPCRECT pRectBounds, POINT ptlLoc, LONG lCloseHint, DWORD *pHitResult)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %p (%ld %ld) %ld %p)\n", This, dwAspect, pRectBounds, ptlLoc.x,
         ptlLoc.y, lCloseHint, pHitResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_QueryHitRect(IViewObjectEx *iface, DWORD dwAspect,
        LPCRECT pRectBounds, LPCRECT pRectLoc, LONG lCloseHint, DWORD *pHitResult)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %p %p %ld %p)\n", This, dwAspect, pRectBounds, pRectLoc, lCloseHint, pHitResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObject_GetNaturalExtent(IViewObjectEx *iface, DWORD dwAspect, LONG lindex,
        DVTARGETDEVICE *ptd, HDC hicTargetDev, DVEXTENTINFO *pExtentInfo, LPSIZEL pSizel)
{
    HTMLDocumentObj *This = impl_from_IViewObjectEx(iface);
    FIXME("(%p)->(%ld %ld %p %p %p %p\n", This, dwAspect,lindex, ptd,
            hicTargetDev, pExtentInfo, pSizel);
    return E_NOTIMPL;
}

static const IViewObjectExVtbl ViewObjectVtbl = {
    ViewObject_QueryInterface,
    ViewObject_AddRef,
    ViewObject_Release,
    ViewObject_Draw,
    ViewObject_GetColorSet,
    ViewObject_Freeze,
    ViewObject_Unfreeze,
    ViewObject_SetAdvise,
    ViewObject_GetAdvise,
    ViewObject_GetExtent,
    ViewObject_GetRect,
    ViewObject_GetViewStatus,
    ViewObject_QueryHitPoint,
    ViewObject_QueryHitRect,
    ViewObject_GetNaturalExtent
};

static inline HTMLDocumentObj *impl_from_IWindowForBindingUI(IWindowForBindingUI *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IWindowForBindingUI_iface);
}

static HRESULT WINAPI WindowForBindingUI_QueryInterface(IWindowForBindingUI *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_IWindowForBindingUI(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWindowForBindingUI_iface;
    }else if(IsEqualGUID(&IID_IWindowForBindingUI, riid)) {
        TRACE("(%p)->(IID_IWindowForBindingUI %p)\n", This, ppv);
        *ppv = &This->IWindowForBindingUI_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WindowForBindingUI_AddRef(IWindowForBindingUI *iface)
{
    HTMLDocumentObj *This = impl_from_IWindowForBindingUI(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI WindowForBindingUI_Release(IWindowForBindingUI *iface)
{
    HTMLDocumentObj *This = impl_from_IWindowForBindingUI(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI WindowForBindingUI_GetWindow(IWindowForBindingUI *iface, REFGUID rguidReason, HWND *phwnd)
{
    HTMLDocumentObj *This = impl_from_IWindowForBindingUI(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(rguidReason), phwnd);

    *phwnd = This->hwnd;
    return S_OK;
}

static const IWindowForBindingUIVtbl WindowForBindingUIVtbl = {
    WindowForBindingUI_QueryInterface,
    WindowForBindingUI_AddRef,
    WindowForBindingUI_Release,
    WindowForBindingUI_GetWindow
};

void HTMLDocument_View_Init(HTMLDocumentObj *doc)
{
    doc->IOleDocumentView_iface.lpVtbl = &OleDocumentViewVtbl;
    doc->IViewObjectEx_iface.lpVtbl = &ViewObjectVtbl;
    doc->IWindowForBindingUI_iface.lpVtbl = &WindowForBindingUIVtbl;
}
