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
    EVENTID_BLUR,
    EVENTID_CHANGE,
    EVENTID_CLICK,
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
    EVENTID_SELECTSTART,
    EVENTID_LAST
} eventid_t;

eventid_t str_to_eid(LPCWSTR);
void check_event_attr(HTMLDocument*,nsIDOMElement*);
void release_event_target(event_target_t*);
void fire_event(HTMLDocument*,eventid_t,nsIDOMNode*);
HRESULT set_event_handler(event_target_t**,HTMLDocument*,eventid_t,VARIANT*);
HRESULT get_event_handler(event_target_t**,eventid_t,VARIANT*);

static inline HRESULT set_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return set_event_handler(&node->event_target, node->doc, eid, var);
}

static inline HRESULT get_node_event(HTMLDOMNode *node, eventid_t eid, VARIANT *var)
{
    return get_event_handler(&node->event_target, eid, var);
}

static inline HRESULT set_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return set_event_handler(&doc->event_target, doc, eid, var);
}

static inline HRESULT get_doc_event(HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    return get_event_handler(&doc->event_target, eid, var);
}
