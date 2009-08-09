/*
 * Copyright 2007-2008 Jacek Caban for CodeWeavers
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define NSEVENTLIST_THIS(iface) DEFINE_THIS(nsEventListener, DOMEventListener, iface)

static nsresult NSAPI nsDOMEventListener_QueryInterface(nsIDOMEventListener *iface,
                                                        nsIIDRef riid, nsQIResult result)
{
    nsEventListener *This = NSEVENTLIST_THIS(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = NSEVENTLIST(This);
    }else if(IsEqualGUID(&IID_nsIDOMEventListener, riid)) {
        TRACE("(%p)->(IID_nsIDOMEventListener %p)\n", This, result);
        *result = NSEVENTLIST(This);
    }

    if(*result) {
        nsIWebBrowserChrome_AddRef(NSEVENTLIST(This));
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsDOMEventListener_AddRef(nsIDOMEventListener *iface)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;
    return nsIWebBrowserChrome_AddRef(NSWBCHROME(This));
}

static nsrefcnt NSAPI nsDOMEventListener_Release(nsIDOMEventListener *iface)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;
    return nsIWebBrowserChrome_Release(NSWBCHROME(This));
}

static BOOL is_doc_child_focus(NSContainer *This)
{
    HWND hwnd;

    if(!This->doc)
        return FALSE;

    for(hwnd = GetFocus(); hwnd && hwnd != This->doc->hwnd; hwnd = GetParent(hwnd));

    return hwnd == This->doc->hwnd;
}

static nsresult NSAPI handle_blur(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;

    TRACE("(%p)\n", This);

    if(!This->reset_focus && This->doc && This->doc->focus && !is_doc_child_focus(This)) {
        This->doc->focus = FALSE;
        notif_focus(This->doc);
    }

    return NS_OK;
}

static nsresult NSAPI handle_focus(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;

    TRACE("(%p)\n", This);

    if(!This->reset_focus && This->doc && !This->doc->focus) {
        This->doc->focus = TRUE;
        notif_focus(This->doc);
    }

    return NS_OK;
}

static nsresult NSAPI handle_keypress(nsIDOMEventListener *iface,
        nsIDOMEvent *event)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;

    TRACE("(%p)->(%p)\n", This, event);

    update_doc(This->doc, UPDATE_UI);
    if(This->doc->usermode == EDITMODE)
        handle_edit_event(This->doc, event);

    return NS_OK;
}

static nsresult NSAPI handle_load(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;
    nsIDOMHTMLElement *nsbody = NULL;
    task_t *task;

    TRACE("(%p)\n", This);

    if(!This->doc)
        return NS_OK;

    update_nsdocument(This->doc);
    connect_scripts(This->doc);

    if(This->editor_controller) {
        nsIController_Release(This->editor_controller);
        This->editor_controller = NULL;
    }

    if(This->doc->usermode == EDITMODE)
        handle_edit_load(This->doc);

    task = heap_alloc(sizeof(task_t));

    task->doc = This->doc;
    task->task_id = TASK_PARSECOMPLETE;
    task->next = NULL;

    /*
     * This should be done in the worker thread that parses HTML,
     * but we don't have such thread (Gecko parses HTML for us).
     */
    push_task(task);

    if(!This->doc->nsdoc) {
        ERR("NULL nsdoc\n");
        return NS_ERROR_FAILURE;
    }

    nsIDOMHTMLDocument_GetBody(This->doc->nsdoc, &nsbody);
    if(nsbody) {
        fire_event(This->doc, EVENTID_LOAD, (nsIDOMNode*)nsbody);
        nsIDOMHTMLElement_Release(nsbody);
    }

    return NS_OK;
}

static nsresult NSAPI handle_htmlevent(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    NSContainer *This = NSEVENTLIST_THIS(iface)->This;
    const PRUnichar *type;
    nsIDOMEventTarget *event_target;
    nsIDOMNode *nsnode;
    nsAString type_str;
    eventid_t eid;
    nsresult nsres;

    nsAString_Init(&type_str, NULL);
    nsIDOMEvent_GetType(event, &type_str);
    nsAString_GetData(&type_str, &type);
    eid = str_to_eid(type);
    nsAString_Finish(&type_str);

    nsres = nsIDOMEvent_GetTarget(event, &event_target);
    if(NS_FAILED(nsres) || !event_target) {
        ERR("GetEventTarget failed: %08x\n", nsres);
        return NS_OK;
    }

    nsres = nsIDOMEventTarget_QueryInterface(event_target, &IID_nsIDOMNode, (void**)&nsnode);
    nsIDOMEventTarget_Release(event_target);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNode: %08x\n", nsres);
        return NS_OK;
    }

    fire_event(This->doc, eid, nsnode);

    nsIDOMNode_Release(nsnode);

    return NS_OK;
}

#undef NSEVENTLIST_THIS

#define EVENTLISTENER_VTBL(handler) \
    { \
        nsDOMEventListener_QueryInterface, \
        nsDOMEventListener_AddRef, \
        nsDOMEventListener_Release, \
        handler, \
    }

static const nsIDOMEventListenerVtbl blur_vtbl =      EVENTLISTENER_VTBL(handle_blur);
static const nsIDOMEventListenerVtbl focus_vtbl =     EVENTLISTENER_VTBL(handle_focus);
static const nsIDOMEventListenerVtbl keypress_vtbl =  EVENTLISTENER_VTBL(handle_keypress);
static const nsIDOMEventListenerVtbl load_vtbl =      EVENTLISTENER_VTBL(handle_load);
static const nsIDOMEventListenerVtbl htmlevent_vtbl = EVENTLISTENER_VTBL(handle_htmlevent);

static void init_event(nsIDOMEventTarget *target, const PRUnichar *type,
        nsIDOMEventListener *listener, BOOL capture)
{
    nsAString type_str;
    nsresult nsres;

    nsAString_Init(&type_str, type);
    nsres = nsIDOMEventTarget_AddEventListener(target, &type_str, listener, capture);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres))
        ERR("AddEventTarget failed: %08x\n", nsres);

}

static void init_listener(nsEventListener *This, NSContainer *container,
        const nsIDOMEventListenerVtbl *vtbl)
{
    This->lpDOMEventListenerVtbl = vtbl;
    This->This = container;
}

void add_nsevent_listener(NSContainer *container, LPCWSTR type)
{
    nsIDOMWindow *dom_window;
    nsIDOMEventTarget *target;
    nsresult nsres;

    nsres = nsIWebBrowser_GetContentDOMWindow(container->webbrowser, &dom_window);
    if(NS_FAILED(nsres)) {
        ERR("GetContentDOMWindow failed: %08x\n", nsres);
        return;
    }

    nsres = nsIDOMWindow_QueryInterface(dom_window, &IID_nsIDOMEventTarget, (void**)&target);
    nsIDOMWindow_Release(dom_window);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMEventTarget interface: %08x\n", nsres);
        return;
    }

    init_event(target, type, NSEVENTLIST(&container->htmlevent_listener), TRUE);
    nsIDOMEventTarget_Release(target);
}

void init_nsevents(NSContainer *This)
{
    nsIDOMWindow *dom_window;
    nsIDOMEventTarget *target;
    nsresult nsres;

    static const PRUnichar wsz_blur[]      = {'b','l','u','r',0};
    static const PRUnichar wsz_focus[]     = {'f','o','c','u','s',0};
    static const PRUnichar wsz_keypress[]  = {'k','e','y','p','r','e','s','s',0};
    static const PRUnichar wsz_load[]      = {'l','o','a','d',0};

    init_listener(&This->blur_listener,        This, &blur_vtbl);
    init_listener(&This->focus_listener,       This, &focus_vtbl);
    init_listener(&This->keypress_listener,    This, &keypress_vtbl);
    init_listener(&This->load_listener,        This, &load_vtbl);
    init_listener(&This->htmlevent_listener,   This, &htmlevent_vtbl);

    nsres = nsIWebBrowser_GetContentDOMWindow(This->webbrowser, &dom_window);
    if(NS_FAILED(nsres)) {
        ERR("GetContentDOMWindow failed: %08x\n", nsres);
        return;
    }

    nsres = nsIDOMWindow_QueryInterface(dom_window, &IID_nsIDOMEventTarget, (void**)&target);
    nsIDOMWindow_Release(dom_window);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMEventTarget interface: %08x\n", nsres);
        return;
    }

    init_event(target, wsz_blur,       NSEVENTLIST(&This->blur_listener),        TRUE);
    init_event(target, wsz_focus,      NSEVENTLIST(&This->focus_listener),       TRUE);
    init_event(target, wsz_keypress,   NSEVENTLIST(&This->keypress_listener),    FALSE);
    init_event(target, wsz_load,       NSEVENTLIST(&This->load_listener),        TRUE);

    nsIDOMEventTarget_Release(target);
}
