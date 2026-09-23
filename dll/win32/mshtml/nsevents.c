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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmcid.h"
#include "shlguid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlscript.h"
#include "htmlevent.h"
#include "binding.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef nsresult (*handler_t)(HTMLDocumentNode*,nsIDOMEvent*);

typedef struct {
    nsIDOMEventListener nsIDOMEventListener_iface;
    nsDocumentEventListener *This;
    handler_t handler;
} nsEventListener;

static nsresult handle_blur(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_focus(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_keypress(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_pageshow(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_pagehide(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_load(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_beforeunload(HTMLDocumentNode*,nsIDOMEvent*);
static nsresult handle_unload(HTMLDocumentNode*,nsIDOMEvent*);

enum doc_event_listener_flags {
    BUBBLES  = 0x0001,
    OVERRIDE = 0x0002,
};

static const struct {
    eventid_t id;
    enum doc_event_listener_flags flags;
    handler_t handler;
} doc_event_listeners[] = {
    { EVENTID_BLUR,             0,                  handle_blur },
    { EVENTID_FOCUS,            0,                  handle_focus },
    { EVENTID_KEYPRESS,         BUBBLES,            handle_keypress },
    { EVENTID_PAGESHOW,         OVERRIDE,           handle_pageshow },
    { EVENTID_PAGEHIDE,         OVERRIDE,           handle_pagehide },
    { EVENTID_LOAD,             OVERRIDE,           handle_load },
    { EVENTID_BEFOREUNLOAD,     OVERRIDE,           handle_beforeunload },
    { EVENTID_UNLOAD,           OVERRIDE,           handle_unload },
};

struct nsDocumentEventListener {
    nsEventListener listener[ARRAY_SIZE(doc_event_listeners)];
    nsEventListener htmlevent_listener;

    LONG ref;

    HTMLDocumentNode *doc;
};

static LONG release_listener(nsDocumentEventListener *This)
{
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static inline nsEventListener *impl_from_nsIDOMEventListener(nsIDOMEventListener *iface)
{
    return CONTAINING_RECORD(iface, nsEventListener, nsIDOMEventListener_iface);
}

static nsresult NSAPI nsDOMEventListener_QueryInterface(nsIDOMEventListener *iface,
        nsIIDRef riid, void **result)
{
    nsEventListener *This = impl_from_nsIDOMEventListener(iface);

    *result = NULL;

    if(IsEqualGUID(&IID_nsISupports, riid)) {
        TRACE("(%p)->(IID_nsISupports, %p)\n", This, result);
        *result = &This->nsIDOMEventListener_iface;
    }else if(IsEqualGUID(&IID_nsIDOMEventListener, riid)) {
        TRACE("(%p)->(IID_nsIDOMEventListener %p)\n", This, result);
        *result = &This->nsIDOMEventListener_iface;
    }

    if(*result) {
        nsIDOMEventListener_AddRef(&This->nsIDOMEventListener_iface);
        return NS_OK;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), result);
    return NS_NOINTERFACE;
}

static nsrefcnt NSAPI nsDOMEventListener_AddRef(nsIDOMEventListener *iface)
{
    nsEventListener *This = impl_from_nsIDOMEventListener(iface);
    LONG ref = InterlockedIncrement(&This->This->ref);

    TRACE("(%p) ref=%ld\n", This->This, ref);

    return ref;
}

static nsrefcnt NSAPI nsDOMEventListener_Release(nsIDOMEventListener *iface)
{
    nsEventListener *This = impl_from_nsIDOMEventListener(iface);

    return release_listener(This->This);
}

static nsresult NSAPI nsDOMEventListener_HandleEvent(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    nsEventListener *This = impl_from_nsIDOMEventListener(iface);
    HTMLDocumentNode *doc = This->This->doc;
    HTMLInnerWindow *window;
    nsresult nsres;

    if(!doc) {
        WARN("NULL doc!\n");
        return NS_ERROR_FAILURE;
    }

    /* Hold a ref to the window, as some apps load another document during handlers */
    window = doc->window;
    if(window)
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    block_task_processing();
    nsres = This->handler(doc, event);
    unblock_task_processing();

    if(window)
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);

    return nsres;
}

static const nsIDOMEventListenerVtbl nsDOMEventListenerVtbl = {
    nsDOMEventListener_QueryInterface,
    nsDOMEventListener_AddRef,
    nsDOMEventListener_Release,
    nsDOMEventListener_HandleEvent
};

static BOOL is_doc_child_focus(GeckoBrowser *nscontainer)
{
    HWND hwnd;

    for(hwnd = GetFocus(); hwnd && hwnd != nscontainer->hwnd; hwnd = GetParent(hwnd));

    return hwnd != NULL;
}

static nsresult handle_blur(HTMLDocumentNode *doc, nsIDOMEvent *event)
{
    HTMLDocumentObj *doc_obj;

    TRACE("(%p)\n", doc);

    if(!doc->doc_obj)
        return NS_ERROR_FAILURE;
    doc_obj = doc->doc_obj;

    if(doc_obj->focus && !is_doc_child_focus(doc_obj->nscontainer)) {
        doc_obj->focus = FALSE;
        notif_focus(doc_obj);
    }

    return NS_OK;
}

static nsresult handle_focus(HTMLDocumentNode *doc, nsIDOMEvent *event)
{
    HTMLDocumentObj *doc_obj;

    TRACE("(%p)\n", doc);

    if(!doc->doc_obj)
        return NS_ERROR_FAILURE;
    doc_obj = doc->doc_obj;

    if(!doc_obj->focus) {
        doc_obj->focus = TRUE;
        notif_focus(doc_obj);
    }

    return NS_OK;
}

static nsresult handle_keypress(HTMLDocumentNode *doc, nsIDOMEvent *event)
{
    if(!doc->browser || !doc->browser->doc)
        return NS_ERROR_FAILURE;

    TRACE("(%p)->(%p)\n", doc, event);

    update_doc(doc->browser->doc, UPDATE_UI);
    if(doc->browser->usermode == EDITMODE)
        handle_edit_event(doc, event);

    return NS_OK;
}

static nsresult handle_pageshow(HTMLDocumentNode *doc, nsIDOMEvent *nsevent)
{
    HTMLInnerWindow *window;
    DOMEvent *event;
    HRESULT hres;

    if(!(window = doc->window) || !doc->dom_document || doc->document_mode < COMPAT_MODE_IE11)
        return NS_OK;

    hres = create_document_event(doc, EVENTID_PAGESHOW, &event);
    if(SUCCEEDED(hres)) {
        dispatch_event(&window->event_target, event);
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }

    return NS_OK;
}

static nsresult handle_pagehide(HTMLDocumentNode *doc, nsIDOMEvent *nsevent)
{
    HTMLInnerWindow *window;
    DOMEvent *event;
    HRESULT hres;

    if(!(window = doc->window) || !doc->dom_document || doc->document_mode < COMPAT_MODE_IE11 || doc->unload_sent)
        return NS_OK;

    hres = create_document_event(doc, EVENTID_PAGEHIDE, &event);
    if(SUCCEEDED(hres)) {
        dispatch_event(&window->event_target, event);
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }

    return NS_OK;
}

static void handle_docobj_load(HTMLDocumentObj *doc)
{
    IOleCommandTarget *olecmd = NULL;
    HRESULT hres;

    unlink_ref(&doc->nscontainer->editor_controller);

    if(doc->nscontainer->usermode == EDITMODE)
        setup_editor_controller(doc->nscontainer);

    if(doc->client) {
        hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
        if(FAILED(hres))
            olecmd = NULL;
    }

    if(doc->download_state) {
        if(olecmd) {
            VARIANT progress;

            V_VT(&progress) = VT_I4;
            V_I4(&progress) = 0;
            IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSPOS,
                    OLECMDEXECOPT_DONTPROMPTUSER, &progress, NULL);
        }

        set_download_state(doc, 0);
    }

    if(olecmd) {
        IOleCommandTarget_Exec(olecmd, &CGID_ShellDocView, 103, 0, NULL, NULL);
        IOleCommandTarget_Exec(olecmd, &CGID_MSHTML, IDM_PARSECOMPLETE, 0, NULL, NULL);
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_HTTPEQUIV_DONE, 0, NULL, NULL);

        IOleCommandTarget_Release(olecmd);
    }
}

static nsresult handle_load(HTMLDocumentNode *doc, nsIDOMEvent *event)
{
    HTMLDocumentObj *doc_obj = NULL;
    DOMEvent *load_event;
    HRESULT hres;

    TRACE("(%p)\n", doc);

    if(!doc->window || !doc->window->base.outer_window)
        return NS_ERROR_FAILURE;
    if(doc->doc_obj && doc->doc_obj->doc_node == doc) {
        doc_obj = doc->doc_obj;
        IUnknown_AddRef(doc_obj->outer_unk);
    }
    connect_scripts(doc->window);

    if(doc_obj)
        handle_docobj_load(doc_obj);

    doc->window->dom_complete_time = get_time_stamp();
    if(doc->window->base.outer_window)
        set_ready_state(doc->window->base.outer_window, READYSTATE_COMPLETE);

    if(doc_obj) {
        if(doc_obj->view_sink)
            IAdviseSink_OnViewChange(doc_obj->view_sink, DVASPECT_CONTENT, -1);

        set_statustext(doc_obj, IDS_STATUS_DONE, NULL);

        update_title(doc_obj);

        if(doc_obj->doc_object_service && doc->window->base.outer_window && !(doc->window->base.outer_window->load_flags & BINDING_REFRESH))
            IDocObjectService_FireDocumentComplete(doc_obj->doc_object_service,
                    &doc->window->base.outer_window->base.IHTMLWindow2_iface, 0);

        IUnknown_Release(doc_obj->outer_unk);
    }

    doc->window->load_event_start_time = get_time_stamp();

    if(doc->dom_document) {
        hres = create_document_event(doc, EVENTID_LOAD, &load_event);
        if(SUCCEEDED(hres)) {
            dispatch_event(&doc->node.event_target, load_event);
            IDOMEvent_Release(&load_event->IDOMEvent_iface);
        }
    }else {
        WARN("no dom_document\n");
    }

    hres = create_event_from_nsevent(event, doc->script_global, dispex_compat_mode(&doc->node.event_target.dispex), &load_event);
    if(SUCCEEDED(hres)) {
        dispatch_event(&doc->window->event_target, load_event);
        IDOMEvent_Release(&load_event->IDOMEvent_iface);
    }

    doc->window->load_event_end_time = get_time_stamp();
    return NS_OK;
}

static nsresult handle_beforeunload(HTMLDocumentNode *doc, nsIDOMEvent *nsevent)
{
    HTMLInnerWindow *window;
    DOMEvent *event;
    HRESULT hres;

    if(!(window = doc->window) || doc->unload_sent)
        return NS_OK;

    /* Gecko dispatches this to the document, but IE dispatches it to the window */
    hres = create_event_from_nsevent(nsevent, doc->script_global, dispex_compat_mode(&doc->node.event_target.dispex), &event);
    if(SUCCEEDED(hres)) {
        dispatch_event(&window->event_target, event);
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }

    return NS_OK;
}

static nsresult handle_unload(HTMLDocumentNode *doc, nsIDOMEvent *nsevent)
{
    HTMLInnerWindow *window, *pending_window;
    DOMEvent *event;
    HRESULT hres;

    if(!(window = doc->window) || doc->unload_sent)
        return NS_OK;
    doc->unload_sent = TRUE;

    pending_window = window->base.outer_window->pending_window;
    if(pending_window)
        pending_window->unload_event_start_time = get_time_stamp();

    hres = create_event_from_nsevent(nsevent, doc->script_global, dispex_compat_mode(&doc->node.event_target.dispex), &event);
    if(SUCCEEDED(hres)) {
        dispatch_event(&window->event_target, event);
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }

    if(pending_window)
        pending_window->unload_event_end_time = get_time_stamp();

    return NS_OK;
}

static nsresult handle_htmlevent(HTMLDocumentNode *doc, nsIDOMEvent *nsevent)
{
    nsIDOMEventTarget *event_target;
    EventTarget *target;
    nsIDOMNode *nsnode;
    HTMLDOMNode *node = NULL;
    DOMEvent *event;
    nsresult nsres;
    HRESULT hres;

    TRACE("%p\n", doc);

    nsres = nsIDOMEvent_GetTarget(nsevent, &event_target);
    if(NS_FAILED(nsres) || !event_target) {
        ERR("GetEventTarget failed: %08lx\n", nsres);
        return NS_OK;
    }

    nsres = nsIDOMEventTarget_QueryInterface(event_target, &IID_nsIDOMNode, (void**)&nsnode);
    nsIDOMEventTarget_Release(event_target);
    if(NS_FAILED(nsres)) {
        if(!doc->window)
            return NS_OK;
        target = &doc->window->event_target;
        IHTMLWindow2_AddRef(&doc->window->base.IHTMLWindow2_iface);
    }else {
        hres = get_node(nsnode, TRUE, &node);
        nsIDOMNode_Release(nsnode);
        if(FAILED(hres) || !node->doc->script_global)
            return NS_OK;
        target = &node->event_target;
    }

    hres = create_event_from_nsevent(nsevent, doc->script_global, dispex_compat_mode(&target->dispex), &event);
    if(FAILED(hres)) {
        IEventTarget_Release(&target->IEventTarget_iface);
        return NS_OK;
    }

    /* If we fine need for more special cases here, we may consider handling it in a more generic way. */
    if((!node || doc == node->doc) && (event->event_id == EVENTID_FOCUS || event->event_id == EVENTID_BLUR)) {
        DOMEvent *focus_event;

        hres = create_document_event(doc, event->event_id == EVENTID_FOCUS ? EVENTID_FOCUSIN : EVENTID_FOCUSOUT, &focus_event);
        if(SUCCEEDED(hres)) {
            dispatch_event(target, focus_event);
            IDOMEvent_Release(&focus_event->IDOMEvent_iface);
        }
    }

    dispatch_event(target, event);

    IDOMEvent_Release(&event->IDOMEvent_iface);
    IEventTarget_Release(&target->IEventTarget_iface);
    return NS_OK;
}

static void init_event(nsIDOMEventTarget *target, const PRUnichar *type,
        nsIDOMEventListener *listener, BOOL capture)
{
    nsAString type_str;
    nsresult nsres;

    nsAString_InitDepend(&type_str, type);
    nsres = nsIDOMEventTarget_AddEventListener(target, &type_str, listener, capture, FALSE, 1);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres))
        ERR("AddEventTarget failed: %08lx\n", nsres);

}

static void init_listener(nsEventListener *This, nsDocumentEventListener *listener, handler_t handler)
{
    This->nsIDOMEventListener_iface.lpVtbl = &nsDOMEventListenerVtbl;
    This->handler = handler;
    This->This = listener;
}

static nsIDOMEventTarget *get_default_document_target(HTMLDocumentNode *doc)
{
    nsIDOMEventTarget *target;
    nsISupports *target_iface;
    nsresult nsres;

    target_iface = doc->window && doc->window->base.outer_window ? (nsISupports*)doc->window->base.outer_window->nswindow : (nsISupports*)doc->dom_document;
    nsres = nsISupports_QueryInterface(target_iface, &IID_nsIDOMEventTarget, (void**)&target);
    return NS_SUCCEEDED(nsres) ? target : NULL;
}

void add_nsevent_listener(HTMLDocumentNode *doc, nsIDOMNode *nsnode, LPCWSTR type)
{
    nsIDOMEventTarget *target;
    nsresult nsres;

    if(nsnode) {
        nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMEventTarget, (void**)&target);
        assert(nsres == NS_OK);
    }else {
        target = get_default_document_target(doc);
        if(!target)
            return;
    }

    init_event(target, type, &doc->nsevent_listener->htmlevent_listener.nsIDOMEventListener_iface,
            TRUE);
    nsIDOMEventTarget_Release(target);
}

static void detach_nslistener(HTMLDocumentNode *doc, const WCHAR *type, nsEventListener *listener, cpp_bool is_capture)
{
    nsIDOMEventTarget *target;
    nsAString type_str;
    nsresult nsres;

    target = get_default_document_target(doc);
    if(!target)
        return;

    nsAString_InitDepend(&type_str, type);
    nsres = nsIDOMEventTarget_RemoveEventListener(target, &type_str,
            &listener->nsIDOMEventListener_iface, is_capture);
    nsAString_Finish(&type_str);
    nsIDOMEventTarget_Release(target);
    if(NS_FAILED(nsres))
        ERR("RemoveEventTarget failed: %08lx\n", nsres);
}

void detach_nsevent(HTMLDocumentNode *doc, const WCHAR *type)
{
    detach_nslistener(doc, type, &doc->nsevent_listener->htmlevent_listener, TRUE);
}

void release_nsevents(HTMLDocumentNode *doc)
{
    nsDocumentEventListener *listener = doc->nsevent_listener;
    unsigned i;

    TRACE("%p %p\n", doc, doc->nsevent_listener);

    if(!listener)
        return;

    for(i = 0; i < ARRAY_SIZE(doc_event_listeners); i++)
        detach_nslistener(doc, get_event_name(doc_event_listeners[i].id), &listener->listener[i], !(doc_event_listeners[i].flags & BUBBLES));

    listener->doc = NULL;
    release_listener(listener);
    doc->nsevent_listener = NULL;
}

void init_nsevents(HTMLDocumentNode *doc)
{
    nsDocumentEventListener *listener;
    nsIDOMEventTarget *target;
    unsigned i;

    listener = malloc(sizeof(nsDocumentEventListener));
    if(!listener)
        return;

    TRACE("%p %p\n", doc, listener);

    listener->ref = 1;
    listener->doc = doc;

    for(i = 0; i < ARRAY_SIZE(doc_event_listeners); i++)
        init_listener(&listener->listener[i], listener, doc_event_listeners[i].handler);
    init_listener(&listener->htmlevent_listener, listener, handle_htmlevent);

    doc->nsevent_listener = listener;

    target = get_default_document_target(doc);
    if(!target)
        return;

    for(i = 0; i < ARRAY_SIZE(doc_event_listeners); i++) {
        init_event(target, get_event_name(doc_event_listeners[i].id), &listener->listener[i].nsIDOMEventListener_iface,
                   !(doc_event_listeners[i].flags & BUBBLES));

        if(doc_event_listeners[i].flags & OVERRIDE)
            doc->event_vector[doc_event_listeners[i].id] = TRUE;
    }

    nsIDOMEventTarget_Release(target);
}
