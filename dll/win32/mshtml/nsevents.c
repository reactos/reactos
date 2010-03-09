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
#include "mshtmcid.h"
#include "shlguid.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    const nsIDOMEventListenerVtbl      *lpDOMEventListenerVtbl;
    nsDocumentEventListener *This;
} nsEventListener;

struct nsDocumentEventListener {
    nsEventListener blur_listener;
    nsEventListener focus_listener;
    nsEventListener keypress_listener;
    nsEventListener load_listener;
    nsEventListener htmlevent_listener;

    LONG ref;

    HTMLDocumentNode *doc;
};

static LONG release_listener(nsDocumentEventListener *This)
{
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

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
    nsDocumentEventListener *This = NSEVENTLIST_THIS(iface)->This;
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static nsrefcnt NSAPI nsDOMEventListener_Release(nsIDOMEventListener *iface)
{
    nsDocumentEventListener *This = NSEVENTLIST_THIS(iface)->This;

    return release_listener(This);
}

static BOOL is_doc_child_focus(HTMLDocumentObj *doc)
{
    HWND hwnd;

    for(hwnd = GetFocus(); hwnd && hwnd != doc->hwnd; hwnd = GetParent(hwnd));

    return hwnd != NULL;
}

static nsresult NSAPI handle_blur(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    HTMLDocumentNode *doc = NSEVENTLIST_THIS(iface)->This->doc;
    HTMLDocumentObj *doc_obj;

    TRACE("(%p)\n", doc);

    if(!doc || !doc->basedoc.doc_obj)
        return NS_ERROR_FAILURE;
    doc_obj = doc->basedoc.doc_obj;

    if(!doc_obj->nscontainer->reset_focus && doc_obj->focus && !is_doc_child_focus(doc_obj)) {
        doc_obj->focus = FALSE;
        notif_focus(doc_obj);
    }

    return NS_OK;
}

static nsresult NSAPI handle_focus(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    HTMLDocumentNode *doc = NSEVENTLIST_THIS(iface)->This->doc;
    HTMLDocumentObj *doc_obj;

    TRACE("(%p)\n", doc);

    if(!doc)
        return NS_ERROR_FAILURE;
    doc_obj = doc->basedoc.doc_obj;

    if(!doc_obj->nscontainer->reset_focus && !doc_obj->focus) {
        doc_obj->focus = TRUE;
        notif_focus(doc_obj);
    }

    return NS_OK;
}

static nsresult NSAPI handle_keypress(nsIDOMEventListener *iface,
        nsIDOMEvent *event)
{
    HTMLDocumentNode *doc = NSEVENTLIST_THIS(iface)->This->doc;
    HTMLDocumentObj *doc_obj;

    if(!doc)
        return NS_ERROR_FAILURE;
    doc_obj = doc->basedoc.doc_obj;

    TRACE("(%p)->(%p)\n", doc, event);

    update_doc(&doc_obj->basedoc, UPDATE_UI);
    if(doc_obj->usermode == EDITMODE)
        handle_edit_event(&doc_obj->basedoc, event);

    return NS_OK;
}

static void handle_docobj_load(HTMLDocumentObj *doc)
{
    IOleCommandTarget *olecmd = NULL;
    HRESULT hres;

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(SUCCEEDED(hres)) {
        if(doc->download_state) {
            VARIANT state, progress;

            V_VT(&progress) = VT_I4;
            V_I4(&progress) = 0;
            IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETPROGRESSPOS,
                    OLECMDEXECOPT_DONTPROMPTUSER, &progress, NULL);

            V_VT(&state) = VT_I4;
            V_I4(&state) = 0;
            IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_SETDOWNLOADSTATE,
                    OLECMDEXECOPT_DONTPROMPTUSER, &state, NULL);
        }

        IOleCommandTarget_Exec(olecmd, &CGID_ShellDocView, 103, 0, NULL, NULL);
        IOleCommandTarget_Exec(olecmd, &CGID_MSHTML, IDM_PARSECOMPLETE, 0, NULL, NULL);
        IOleCommandTarget_Exec(olecmd, NULL, OLECMDID_HTTPEQUIV_DONE, 0, NULL, NULL);

        IOleCommandTarget_Release(olecmd);
    }
    doc->download_state = 0;
}

static nsresult NSAPI handle_load(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    HTMLDocumentNode *doc = NSEVENTLIST_THIS(iface)->This->doc;
    HTMLDocumentObj *doc_obj;
    nsIDOMHTMLElement *nsbody = NULL;

    TRACE("(%p)\n", doc);

    if(!doc)
        return NS_ERROR_FAILURE;
    doc_obj = doc->basedoc.doc_obj;

    connect_scripts(doc->basedoc.window);

    if(doc_obj->nscontainer->editor_controller) {
        nsIController_Release(doc_obj->nscontainer->editor_controller);
        doc_obj->nscontainer->editor_controller = NULL;
    }

    if(doc_obj->usermode == EDITMODE)
        handle_edit_load(&doc_obj->basedoc);

    if(doc == doc_obj->basedoc.doc_node)
        handle_docobj_load(doc_obj);

    set_ready_state(doc->basedoc.window, READYSTATE_COMPLETE);

    if(doc == doc_obj->basedoc.doc_node) {
        if(doc_obj->view_sink)
            IAdviseSink_OnViewChange(doc_obj->view_sink, DVASPECT_CONTENT, -1);

        if(doc_obj->frame) {
            static const WCHAR wszDone[] = {'D','o','n','e',0};
            IOleInPlaceFrame_SetStatusText(doc_obj->frame, wszDone);
        }

        update_title(doc_obj);
    }

    if(!doc->nsdoc) {
        ERR("NULL nsdoc\n");
        return NS_ERROR_FAILURE;
    }

    nsIDOMHTMLDocument_GetBody(doc->nsdoc, &nsbody);
    if(nsbody) {
        fire_event(doc, EVENTID_LOAD, (nsIDOMNode*)nsbody, event);
        nsIDOMHTMLElement_Release(nsbody);
    }

    return NS_OK;
}

static nsresult NSAPI handle_htmlevent(nsIDOMEventListener *iface, nsIDOMEvent *event)
{
    HTMLDocumentNode *doc = NSEVENTLIST_THIS(iface)->This->doc;
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

    fire_event(doc, eid, nsnode, event);

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

    nsAString_InitDepend(&type_str, type);
    nsres = nsIDOMEventTarget_AddEventListener(target, &type_str, listener, capture);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres))
        ERR("AddEventTarget failed: %08x\n", nsres);

}

static void init_listener(nsEventListener *This, nsDocumentEventListener *listener,
        const nsIDOMEventListenerVtbl *vtbl)
{
    This->lpDOMEventListenerVtbl = vtbl;
    This->This = listener;
}

void add_nsevent_listener(HTMLDocumentNode *doc, LPCWSTR type)
{
    nsIDOMEventTarget *target;
    nsresult nsres;

    nsres = nsIDOMWindow_QueryInterface(doc->basedoc.window->nswindow, &IID_nsIDOMEventTarget, (void**)&target);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMEventTarget interface: %08x\n", nsres);
        return;
    }

    init_event(target, type, NSEVENTLIST(&doc->nsevent_listener->htmlevent_listener), TRUE);
    nsIDOMEventTarget_Release(target);
}

void release_nsevents(HTMLDocumentNode *doc)
{
    if(doc->nsevent_listener) {
        doc->nsevent_listener->doc = NULL;
        release_listener(doc->nsevent_listener);
        doc->nsevent_listener = NULL;
    }
}

void init_nsevents(HTMLDocumentNode *doc)
{
    nsDocumentEventListener *listener;
    nsIDOMEventTarget *target;
    nsresult nsres;

    static const PRUnichar wsz_blur[]      = {'b','l','u','r',0};
    static const PRUnichar wsz_focus[]     = {'f','o','c','u','s',0};
    static const PRUnichar wsz_keypress[]  = {'k','e','y','p','r','e','s','s',0};
    static const PRUnichar wsz_load[]      = {'l','o','a','d',0};

    listener = heap_alloc(sizeof(nsDocumentEventListener));
    if(!listener)
        return;

    listener->ref = 1;
    listener->doc = doc;

    init_listener(&listener->blur_listener,        listener, &blur_vtbl);
    init_listener(&listener->focus_listener,       listener, &focus_vtbl);
    init_listener(&listener->keypress_listener,    listener, &keypress_vtbl);
    init_listener(&listener->load_listener,        listener, &load_vtbl);
    init_listener(&listener->htmlevent_listener,   listener, &htmlevent_vtbl);

    doc->nsevent_listener = listener;

    nsres = nsIDOMWindow_QueryInterface(doc->basedoc.window->nswindow, &IID_nsIDOMEventTarget, (void**)&target);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMEventTarget interface: %08x\n", nsres);
        return;
    }

    init_event(target, wsz_blur,       NSEVENTLIST(&listener->blur_listener),        TRUE);
    init_event(target, wsz_focus,      NSEVENTLIST(&listener->focus_listener),       TRUE);
    init_event(target, wsz_keypress,   NSEVENTLIST(&listener->keypress_listener),    FALSE);
    init_event(target, wsz_load,       NSEVENTLIST(&listener->load_listener),        TRUE);

    nsIDOMEventTarget_Release(target);
}
