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

#ifdef __REACTOS__
#pragma once
#endif // __REACTOS__

typedef enum {
    EVENTID_ABORT,
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
    EVENTID_HELP,
    EVENTID_KEYDOWN,
    EVENTID_KEYPRESS,
    EVENTID_KEYUP,
    EVENTID_LOAD,
    EVENTID_MOUSEDOWN,
    EVENTID_MOUSEMOVE,
    EVENTID_MOUSEOUT,
    EVENTID_MOUSEOVER,
    EVENTID_MOUSEUP,
    EVENTID_MOUSEWHEEL,
    EVENTID_PASTE,
    EVENTID_READYSTATECHANGE,
    EVENTID_RESIZE,
    EVENTID_SCROLL,
    EVENTID_SELECTSTART,
    EVENTID_SUBMIT,
    EVENTID_LAST
} eventid_t;

eventid_t str_to_eid(LPCWSTR) DECLSPEC_HIDDEN;
void check_event_attr(HTMLDocumentNode*,nsIDOMHTMLElement*) DECLSPEC_HIDDEN;
void release_event_target(event_target_t*) DECLSPEC_HIDDEN;
void fire_event(HTMLDocumentNode*,eventid_t,BOOL,nsIDOMNode*,nsIDOMEvent*,IDispatch*) DECLSPEC_HIDDEN;
HRESULT set_event_handler(EventTarget*,eventid_t,VARIANT*) DECLSPEC_HIDDEN;
HRESULT get_event_handler(EventTarget*,eventid_t,VARIANT*) DECLSPEC_HIDDEN;
HRESULT attach_event(EventTarget*,BSTR,IDispatch*,VARIANT_BOOL*) DECLSPEC_HIDDEN;
HRESULT detach_event(EventTarget*,BSTR,IDispatch*) DECLSPEC_HIDDEN;
HRESULT dispatch_event(HTMLDOMNode*,const WCHAR*,VARIANT*,VARIANT_BOOL*) DECLSPEC_HIDDEN;
HRESULT call_fire_event(HTMLDOMNode*,eventid_t) DECLSPEC_HIDDEN;
void update_doc_cp_events(HTMLDocumentNode*,cp_static_data_t*) DECLSPEC_HIDDEN;
HRESULT doc_init_events(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void detach_events(HTMLDocumentNode *doc) DECLSPEC_HIDDEN;
HRESULT create_event_obj(IHTMLEventObj**) DECLSPEC_HIDDEN;
void bind_target_event(HTMLDocumentNode*,EventTarget*,const WCHAR*,IDispatch*) DECLSPEC_HIDDEN;
HRESULT ensure_doc_nsevent_handler(HTMLDocumentNode*,eventid_t) DECLSPEC_HIDDEN;

typedef struct HTMLEventObj HTMLEventObj;
void call_event_handlers(HTMLDocumentNode*,HTMLEventObj*,EventTarget*,ConnectionPointContainer*,eventid_t,IDispatch*);

void init_nsevents(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void release_nsevents(HTMLDocumentNode*) DECLSPEC_HIDDEN;
void add_nsevent_listener(HTMLDocumentNode*,nsIDOMNode*,LPCWSTR) DECLSPEC_HIDDEN;
void detach_nsevent(HTMLDocumentNode*,const WCHAR*) DECLSPEC_HIDDEN;

static inline HRESULT set_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return set_event_handler(&node->event_target, eid, var);
}

static inline HRESULT get_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return get_event_handler(&node->event_target, eid, var);
}

static inline HRESULT set_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return set_node_event(&doc->doc_node->node, eid, var);
}

static inline HRESULT get_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return get_node_event(&doc->doc_node->node, eid, var);
}
