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
    EVENTID_BEFOREUNLOAD,
    EVENTID_BLUR,
    EVENTID_CHANGE,
    EVENTID_CLICK,
    EVENTID_DBLCLICK,
    EVENTID_DRAG,
    EVENTID_DRAGSTART,
    EVENTID_FOCUS,
    EVENTID_KEYDOWN,
    EVENTID_KEYUP,
    EVENTID_LOAD,
    EVENTID_MOUSEDOWN,
    EVENTID_MOUSEOUT,
    EVENTID_MOUSEOVER,
    EVENTID_MOUSEUP,
    EVENTID_PASTE,
    EVENTID_READYSTATECHANGE,
    EVENTID_RESIZE,
    EVENTID_SELECTSTART,
    EVENTID_LAST
} eventid_t;

eventid_t str_to_eid(LPCWSTR);
void check_event_attr(HTMLDocumentNode*,nsIDOMElement*);
void release_event_target(event_target_t*);
void fire_event(HTMLDocumentNode*,eventid_t,nsIDOMNode*,nsIDOMEvent*);
HRESULT set_event_handler(event_target_t**,HTMLDocumentNode*,eventid_t,VARIANT*);
HRESULT get_event_handler(event_target_t**,eventid_t,VARIANT*);
HRESULT attach_event(event_target_t**,HTMLDocument*,BSTR,IDispatch*,VARIANT_BOOL*);
HRESULT dispatch_event(HTMLDOMNode*,const WCHAR*,VARIANT*,VARIANT_BOOL*);
HRESULT call_event(HTMLDOMNode*,eventid_t);
void update_cp_events(HTMLWindow*,cp_static_data_t*);

static inline event_target_t **get_node_event_target(HTMLDOMNode *node)
{
    return node->vtbl->get_event_target ? node->vtbl->get_event_target(node) : &node->event_target;
}

static inline HRESULT set_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return set_event_handler(get_node_event_target(node), node->doc, eid, var);
}

static inline HRESULT get_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return get_event_handler(get_node_event_target(node), eid, var);
}

static inline HRESULT set_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return set_node_event(&doc->doc_node->node, eid, var);
}

static inline HRESULT get_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return get_node_event(&doc->doc_node->node, eid, var);
}
