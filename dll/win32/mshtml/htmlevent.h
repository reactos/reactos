/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

typedef enum {
    EVENTID_INVALID_ID = -1,
    EVENTID_DOMCONTENTLOADED,
    EVENTID_ABORT,
    EVENTID_AFTERPRINT,
    EVENTID_ANIMATIONEND,
    EVENTID_ANIMATIONSTART,
    EVENTID_BEFOREACTIVATE,
    EVENTID_BEFOREPRINT,
    EVENTID_BEFOREUNLOAD,
    EVENTID_BLUR,
    EVENTID_CHANGE,
    EVENTID_CLICK,
    EVENTID_CONTEXTMENU,
    EVENTID_DATAAVAILABLE,
    EVENTID_DBLCLICK,
    EVENTID_DRAG,
    EVENTID_DRAGSTART,
    EVENTID_ERROR,
    EVENTID_FOCUS,
    EVENTID_FOCUSIN,
    EVENTID_FOCUSOUT,
    EVENTID_HELP,
    EVENTID_INPUT,
    EVENTID_KEYDOWN,
    EVENTID_KEYPRESS,
    EVENTID_KEYUP,
    EVENTID_LOAD,
    EVENTID_LOADEND,
    EVENTID_LOADSTART,
    EVENTID_MESSAGE,
    EVENTID_MOUSEDOWN,
    EVENTID_MOUSEMOVE,
    EVENTID_MOUSEOUT,
    EVENTID_MOUSEOVER,
    EVENTID_MOUSEUP,
    EVENTID_MOUSEWHEEL,
    EVENTID_MSTHUMBNAILCLICK,
    EVENTID_PAGEHIDE,
    EVENTID_PAGESHOW,
    EVENTID_PASTE,
    EVENTID_PROGRESS,
    EVENTID_READYSTATECHANGE,
    EVENTID_RESIZE,
    EVENTID_SCROLL,
    EVENTID_SELECTIONCHANGE,
    EVENTID_SELECTSTART,
    EVENTID_STORAGE,
    EVENTID_STORAGECOMMIT,
    EVENTID_SUBMIT,
    EVENTID_TIMEOUT,
    EVENTID_UNLOAD,
    EVENTID_VISIBILITYCHANGE,
    EVENTID_LAST
} eventid_t;

typedef struct DOMEvent {
    DispatchEx dispex;
    IDOMEvent IDOMEvent_iface;

    nsIDOMEvent *nsevent;

    eventid_t event_id;
    WCHAR *type;
    EventTarget *target;
    EventTarget *current_target;
    ULONGLONG time_stamp;
    unsigned bubbles : 1;
    unsigned cancelable : 1;
    unsigned prevent_default : 1;
    unsigned stop_propagation : 1;
    unsigned stop_immediate_propagation : 1;
    unsigned trusted : 1;
    unsigned no_event_obj : 1;
    DOM_EVENT_PHASE phase;

    IHTMLEventObj *event_obj;
} DOMEvent;

const WCHAR *get_event_name(eventid_t);
void check_event_attr(HTMLDocumentNode*,nsIDOMElement*);
void traverse_event_target(EventTarget*,nsCycleCollectionTraversalCallback*);
void release_event_target(EventTarget*);
HRESULT set_event_handler(EventTarget*,eventid_t,VARIANT*);
HRESULT get_event_handler(EventTarget*,eventid_t,VARIANT*);
HRESULT attach_event(EventTarget*,BSTR,IDispatch*,VARIANT_BOOL*);
HRESULT detach_event(EventTarget*,BSTR,IDispatch*);
HRESULT fire_event(HTMLDOMNode*,const WCHAR*,VARIANT*,VARIANT_BOOL*);
void update_doc_cp_events(HTMLDocumentNode*,cp_static_data_t*);
HRESULT doc_init_events(HTMLDocumentNode*);
void detach_events(HTMLDocumentNode *doc);
HRESULT create_event_obj(DOMEvent*,HTMLDocumentNode*,IHTMLEventObj**);
void bind_target_event(HTMLDocumentNode*,EventTarget*,const WCHAR*,IDispatch*);
HRESULT ensure_doc_nsevent_handler(HTMLDocumentNode*,nsIDOMNode*,eventid_t);

void dispatch_event(EventTarget*,DOMEvent*);

HRESULT create_document_event(HTMLDocumentNode*,eventid_t,DOMEvent**);
HRESULT create_document_event_str(HTMLDocumentNode*,const WCHAR*,IDOMEvent**);
HRESULT create_event_from_nsevent(nsIDOMEvent*,HTMLInnerWindow*,compat_mode_t,DOMEvent**);
HRESULT create_message_event(HTMLDocumentNode*,IHTMLWindow2*,VARIANT*,DOMEvent**);
HRESULT create_storage_event(HTMLDocumentNode*,BSTR,BSTR,BSTR,const WCHAR*,BOOL,DOMEvent**);

void init_nsevents(HTMLDocumentNode*);
void release_nsevents(HTMLDocumentNode*);
void add_nsevent_listener(HTMLDocumentNode*,nsIDOMNode*,LPCWSTR);
void detach_nsevent(HTMLDocumentNode*,const WCHAR*);

/* We extend dispex vtbl for EventTarget functions to avoid separated vtbl. */
typedef struct {
    dispex_static_data_vtbl_t dispex_vtbl;
    nsISupports *(*get_gecko_target)(DispatchEx*);
    void (*bind_event)(DispatchEx*,eventid_t);
    EventTarget *(*get_parent_event_target)(DispatchEx*);
    HRESULT (*pre_handle_event)(DispatchEx*,DOMEvent*);
    HRESULT (*handle_event)(DispatchEx*,DOMEvent*,BOOL*);
    ConnectionPointContainer *(*get_cp_container)(DispatchEx*);
    IHTMLEventObj *(*set_current_event)(DispatchEx*,IHTMLEventObj*);
} event_target_vtbl_t;

IHTMLEventObj *default_set_current_event(HTMLInnerWindow*,IHTMLEventObj*);

nsISupports *HTMLElement_get_gecko_target(DispatchEx*);
void HTMLElement_bind_event(DispatchEx*,eventid_t);
EventTarget *HTMLElement_get_parent_event_target(DispatchEx*);
HRESULT HTMLElement_handle_event(DispatchEx*,DOMEvent*,BOOL*);
ConnectionPointContainer *HTMLElement_get_cp_container(DispatchEx*);
IHTMLEventObj *HTMLElement_set_current_event(DispatchEx*,IHTMLEventObj*);

#define HTMLELEMENT_DISPEX_VTBL_ENTRIES                 \
    .populate_props      = HTMLElement_populate_props

#define HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES                       \
    .get_gecko_target        = HTMLElement_get_gecko_target,        \
    .bind_event              = HTMLElement_bind_event,              \
    .get_parent_event_target = HTMLElement_get_parent_event_target, \
    .get_cp_container        = HTMLElement_get_cp_container,        \
    .set_current_event       = HTMLElement_set_current_event

static inline EventTarget *get_node_event_prop_target(HTMLDOMNode *node, eventid_t eid)
{
    return node->vtbl->get_event_prop_target ? node->vtbl->get_event_prop_target(node, eid) : &node->event_target;
}

static inline HRESULT set_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return set_event_handler(get_node_event_prop_target(node, eid), eid, var);
}

static inline HRESULT get_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return get_event_handler(get_node_event_prop_target(node, eid), eid, var);
}

static inline HRESULT set_doc_event(HTMLDocumentNode *doc, eventid_t eid, VARIANT *var)
{
    return set_event_handler(&doc->node.event_target, eid, var);
}

static inline HRESULT get_doc_event(HTMLDocumentNode *doc, eventid_t eid, VARIANT *var)
{
    return get_event_handler(&doc->node.event_target, eid, var);
}
