/*
 * Copyright 2008-2009 Jacek Caban for CodeWeavers
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
#include "mshtmdid.h"
#include "wininet.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "htmlscript.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef enum {
    LISTENER_TYPE_CAPTURE,
    LISTENER_TYPE_BUBBLE,
    LISTENER_TYPE_ONEVENT,
    LISTENER_TYPE_ATTACHED
} listener_type_t;

typedef struct {
    struct list entry;
    listener_type_t type;
    IDispatch *function;
} event_listener_t;

typedef struct {
    struct wine_rb_entry entry;
    struct list listeners;
    WCHAR type[1];
} listener_container_t;

typedef enum {
    DISPATCH_BOTH,
    DISPATCH_STANDARD,
    DISPATCH_LEGACY
} dispatch_mode_t;

/* Keep inherited event types after the inheritor (e.g. DragEvent->MouseEvent->UIEvent) */
typedef enum {
    EVENT_TYPE_EVENT,
    EVENT_TYPE_CUSTOM,
    EVENT_TYPE_DRAG,
    EVENT_TYPE_KEYBOARD,
    EVENT_TYPE_MOUSE,
    EVENT_TYPE_FOCUS,
    EVENT_TYPE_UIEVENT,
    EVENT_TYPE_PAGETRANSITION,
    EVENT_TYPE_MESSAGE,
    EVENT_TYPE_PROGRESS,
    EVENT_TYPE_STORAGE,
    EVENT_TYPE_CLIPBOARD
} event_type_t;

static const WCHAR *event_types[] = {
    L"Event",
    L"CustomEvent",
    L"Event", /* FIXME */
    L"KeyboardEvent",
    L"MouseEvent",
    L"Event", /* FIXME */
    L"UIEvent",
    L"PageTransitionEvent",
    L"MessageEvent",
    L"ProgressEvent",
    L"StorageEvent",
    L"Event"  /* FIXME */
};

typedef struct {
    const WCHAR *name;
    event_type_t type;
    DISPID dispid;
    DWORD flags;
} event_info_t;

/* Use Gecko default listener (it's registered on window object for DOM nodes). */
#define EVENT_DEFAULTLISTENER    0x0001
/* Register Gecko listener on target itself (unlike EVENT_DEFAULTLISTENER). */
#define EVENT_BIND_TO_TARGET     0x0002
/* Event bubbles by default (unless explicitly specified otherwise). */
#define EVENT_BUBBLES            0x0004
/* Event is cancelable by default (unless explicitly specified otherwise). */
#define EVENT_CANCELABLE         0x0008
/* Event may have default handler (so we always have to register Gecko listener). */
#define EVENT_HASDEFAULTHANDLERS 0x0020
/* Ecent is not supported properly, print FIXME message when it's used. */
#define EVENT_FIXME              0x0040

/* mouse event flags for fromElement and toElement implementation */
#define EVENT_MOUSE_TO_RELATED   0x0100
#define EVENT_MOUSE_FROM_RELATED 0x0200

/* Keep these sorted case sensitively */
static const event_info_t event_info[] = {
    {L"DOMContentLoaded",  EVENT_TYPE_EVENT,     0,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {L"abort",             EVENT_TYPE_EVENT,     DISPID_EVMETH_ONABORT,
        EVENT_BIND_TO_TARGET},
    {L"afterprint",        EVENT_TYPE_EVENT,     DISPID_EVMETH_ONAFTERPRINT,
        EVENT_DEFAULTLISTENER},
    {L"animationend",      EVENT_TYPE_EVENT,     DISPID_EVPROP_ONANIMATIONEND,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"animationstart",    EVENT_TYPE_EVENT,     DISPID_EVPROP_ONANIMATIONSTART,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"beforeactivate",    EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREACTIVATE,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"beforeprint",       EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREPRINT,
        EVENT_DEFAULTLISTENER},
    {L"beforeunload",      EVENT_TYPE_EVENT,     DISPID_EVMETH_ONBEFOREUNLOAD,
        EVENT_DEFAULTLISTENER | EVENT_CANCELABLE },
    {L"blur",              EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONBLUR,
        EVENT_DEFAULTLISTENER},
    {L"change",            EVENT_TYPE_EVENT,     DISPID_EVMETH_ONCHANGE,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"click",             EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONCLICK,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {L"contextmenu",       EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONCONTEXTMENU,
        EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"dataavailable",     EVENT_TYPE_EVENT,     DISPID_EVMETH_ONDATAAVAILABLE,
        EVENT_FIXME | EVENT_BUBBLES},
    {L"dblclick",          EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONDBLCLICK,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"drag",              EVENT_TYPE_DRAG,      DISPID_EVMETH_ONDRAG,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"dragstart",         EVENT_TYPE_DRAG,      DISPID_EVMETH_ONDRAGSTART,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"error",             EVENT_TYPE_EVENT,     DISPID_EVMETH_ONERROR,
        EVENT_BIND_TO_TARGET},
    {L"focus",             EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUS,
        EVENT_DEFAULTLISTENER},
    {L"focusin",           EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUSIN,
        EVENT_BUBBLES},
    {L"focusout",          EVENT_TYPE_FOCUS,     DISPID_EVMETH_ONFOCUSOUT,
        EVENT_BUBBLES},
    {L"help",              EVENT_TYPE_EVENT,     DISPID_EVMETH_ONHELP,
        EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"input",             EVENT_TYPE_EVENT,     DISPID_UNKNOWN,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES},
    {L"keydown",           EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYDOWN,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE },
    {L"keypress",          EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYPRESS,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"keyup",             EVENT_TYPE_KEYBOARD,  DISPID_EVMETH_ONKEYUP,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"load",              EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONLOAD,
        EVENT_BIND_TO_TARGET},
    {L"loadend",           EVENT_TYPE_PROGRESS,  DISPID_EVPROP_LOADEND,
        EVENT_BIND_TO_TARGET},
    {L"loadstart",         EVENT_TYPE_PROGRESS,  DISPID_EVPROP_LOADSTART,
        EVENT_BIND_TO_TARGET},
    {L"message",           EVENT_TYPE_MESSAGE,   DISPID_EVMETH_ONMESSAGE,
        0},
    {L"mousedown",         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEDOWN,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"mousemove",         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEMOVE,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE | EVENT_MOUSE_FROM_RELATED},
    {L"mouseout",          EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEOUT,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE | EVENT_MOUSE_TO_RELATED},
    {L"mouseover",         EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEOVER,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE | EVENT_MOUSE_FROM_RELATED},
    {L"mouseup",           EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEUP,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"mousewheel",        EVENT_TYPE_MOUSE,     DISPID_EVMETH_ONMOUSEWHEEL,
        EVENT_FIXME},
    {L"msthumbnailclick",  EVENT_TYPE_MOUSE,     DISPID_EVPROP_ONMSTHUMBNAILCLICK,
        EVENT_FIXME},
    {L"pagehide",          EVENT_TYPE_PAGETRANSITION, DISPID_EVPROP_ONPAGEHIDE,
        0},
    {L"pageshow",          EVENT_TYPE_PAGETRANSITION, DISPID_EVPROP_ONPAGESHOW,
        0},
    {L"paste",             EVENT_TYPE_CLIPBOARD, DISPID_EVMETH_ONPASTE,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"progress",          EVENT_TYPE_PROGRESS,  DISPID_EVPROP_PROGRESS,
        EVENT_BIND_TO_TARGET},
    {L"readystatechange",  EVENT_TYPE_EVENT,     DISPID_EVMETH_ONREADYSTATECHANGE,
        0},
    {L"resize",            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONRESIZE,
        EVENT_DEFAULTLISTENER},
    {L"scroll",            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONSCROLL,
        EVENT_DEFAULTLISTENER | EVENT_BUBBLES /* FIXME: not for elements */},
    {L"selectionchange",   EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSELECTIONCHANGE,
        EVENT_FIXME},
    {L"selectstart",       EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSELECTSTART,
        EVENT_FIXME | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"storage",           EVENT_TYPE_STORAGE,   DISPID_EVMETH_ONSTORAGE,
        0},
    {L"storagecommit",     EVENT_TYPE_STORAGE,   DISPID_EVMETH_ONSTORAGECOMMIT,
        0},
    {L"submit",            EVENT_TYPE_EVENT,     DISPID_EVMETH_ONSUBMIT,
        EVENT_DEFAULTLISTENER | EVENT_HASDEFAULTHANDLERS | EVENT_BUBBLES | EVENT_CANCELABLE},
    {L"timeout",           EVENT_TYPE_PROGRESS,  DISPID_EVPROP_TIMEOUT,
        EVENT_BIND_TO_TARGET},
    {L"unload",            EVENT_TYPE_UIEVENT,   DISPID_EVMETH_ONUNLOAD,
        EVENT_BIND_TO_TARGET},
    {L"visibilitychange",  EVENT_TYPE_EVENT,     DISPID_EVPROP_VISIBILITYCHANGE,
        EVENT_FIXME | EVENT_BUBBLES},

    /* EVENTID_LAST special entry */
    {NULL,                 EVENT_TYPE_EVENT,     0, 0}
};

C_ASSERT(ARRAY_SIZE(event_info) - 1 == EVENTID_LAST);

static eventid_t str_to_eid(const WCHAR *str)
{
    unsigned i, a = 0, b = ARRAY_SIZE(event_info) - 1;
    int c;

    while(a < b) {
        i = (a + b) / 2;
        if(!(c = wcscmp(event_info[i].name, str)))
            return i;
        if(c > 0) b = i;
        else      a = i + 1;
    }

    return EVENTID_LAST;
}

static eventid_t attr_to_eid(const WCHAR *str)
{
    unsigned i, a = 0, b = ARRAY_SIZE(event_info) - 1;
    int c;

    if((str[0] != 'o' && str[0] != 'O') || (str[1] != 'n' && str[1] != 'N'))
        return EVENTID_LAST;

    while(a < b) {
        i = (a + b) / 2;
        if(!(c = wcscmp(event_info[i].name, str+2)))
            return event_info[i].dispid ? i : EVENTID_LAST;
        if(c > 0) b = i;
        else      a = i + 1;
    }

    return EVENTID_LAST;
}

const WCHAR *get_event_name(eventid_t eid)
{
    return event_info[eid].name;
}

static listener_container_t *get_listener_container(EventTarget *event_target, const WCHAR *type, BOOL alloc)
{
    const event_target_vtbl_t *vtbl;
    listener_container_t *container;
    struct wine_rb_entry *entry;
    size_t type_len;
    eventid_t eid;

    entry = wine_rb_get(&event_target->handler_map, type);
    if(entry)
        return WINE_RB_ENTRY_VALUE(entry, listener_container_t, entry);
    if(!alloc)
        return NULL;

    eid = str_to_eid(type);
    if(event_info[eid].flags & EVENT_FIXME)
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    type_len = lstrlenW(type);
    container = malloc(FIELD_OFFSET(listener_container_t, type[type_len+1]));
    if(!container)
        return NULL;
    memcpy(container->type, type, (type_len + 1) * sizeof(WCHAR));
    list_init(&container->listeners);
    vtbl = dispex_get_vtbl(&event_target->dispex);
    if (!vtbl->bind_event)
        FIXME("Unsupported event binding on target %p\n", event_target);
    else if(eid != EVENTID_LAST)
        vtbl->bind_event(&event_target->dispex, eid);

    wine_rb_put(&event_target->handler_map, container->type, &container->entry);
    return container;
}

static void remove_event_listener(EventTarget *event_target, const WCHAR *type_name, listener_type_t type, IDispatch *function)
{
    listener_container_t *container;
    event_listener_t *listener;

    container = get_listener_container(event_target, type_name, FALSE);
    if(!container)
        return;

    LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
        if(listener->function == function && listener->type == type) {
            IDispatch_Release(listener->function);
            list_remove(&listener->entry);
            free(listener);
            break;
        }
    }
}

static IEventTarget *get_event_target_iface(EventTarget *event_target)
{
    const dispex_static_data_vtbl_t *vtbl = dispex_get_vtbl(&event_target->dispex);
    IEventTarget *ret;

    if(vtbl->get_outer_iface) {
        IWineJSDispatchHost *disp = vtbl->get_outer_iface(&event_target->dispex);
        IWineJSDispatchHost_QueryInterface(disp, &IID_IEventTarget, (void**)&ret);
        IWineJSDispatchHost_Release(disp);
    }else {
        ret = &event_target->IEventTarget_iface;
        IEventTarget_AddRef(ret);
    }
    return ret;
}

static HRESULT get_gecko_target(IEventTarget*,nsIDOMEventTarget**);

typedef struct {
    DOMEvent event;
    IDOMUIEvent IDOMUIEvent_iface;
    nsIDOMUIEvent *nsevent;
} DOMUIEvent;

static DOMUIEvent *DOMUIEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMUIEvent, event);
}

typedef struct {
    DOMEvent event;
    IDOMStorageEvent IDOMStorageEvent_iface;
    BSTR key;
    BSTR old_value;
    BSTR new_value;
    BSTR url;
} DOMStorageEvent;

static DOMStorageEvent *DOMStorageEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMStorageEvent, event);
}

typedef struct {
    DispatchEx dispex;
    IHTMLEventObj IHTMLEventObj_iface;
    IHTMLEventObj2 IHTMLEventObj2_iface;
    IHTMLEventObj3 IHTMLEventObj3_iface;
    IHTMLEventObj4 IHTMLEventObj4_iface;
    IHTMLEventObj5 IHTMLEventObj5_iface;
    IHTMLEventObj6 IHTMLEventObj6_iface;

    DOMEvent *event;
    VARIANT return_value;
} HTMLEventObj;

static inline HTMLEventObj *impl_from_IHTMLEventObj(IHTMLEventObj *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLEventObj, IHTMLEventObj, impl_from_IHTMLEventObj(iface)->dispex)

static HRESULT WINAPI HTMLEventObj_get_srcElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event) {
        *p = NULL;
        return S_OK;
    }

    return IDOMEvent_get_srcElement(&This->event->IDOMEvent_iface, p);
}

static HRESULT WINAPI HTMLEventObj_get_altKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMKeyboardEvent *keyboard_event;
    IDOMMouseEvent *mouse_event;
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_altKey(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMKeyboardEvent, (void**)&keyboard_event))) {
        HRESULT hres = IDOMKeyboardEvent_get_altKey(keyboard_event, p);
        IDOMKeyboardEvent_Release(keyboard_event);
        return hres;
    }

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_ctrlKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMKeyboardEvent *keyboard_event;
    IDOMMouseEvent *mouse_event;
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_ctrlKey(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMKeyboardEvent, (void**)&keyboard_event))) {
        HRESULT hres = IDOMKeyboardEvent_get_ctrlKey(keyboard_event, p);
        IDOMKeyboardEvent_Release(keyboard_event);
        return hres;
    }

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_shiftKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMKeyboardEvent *keyboard_event;
    IDOMMouseEvent *mouse_event;
    cpp_bool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_shiftKey(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMKeyboardEvent, (void**)&keyboard_event))) {
        HRESULT hres = IDOMKeyboardEvent_get_shiftKey(keyboard_event, p);
        IDOMKeyboardEvent_Release(keyboard_event);
        return hres;
    }

    *p = variant_bool(ret);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_returnValue(IHTMLEventObj *iface, VARIANT v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BOOL) {
        FIXME("unsupported value %s\n", debugstr_variant(&v));
        return DISP_E_BADVARTYPE;
    }

    This->return_value = v;
    if(!V_BOOL(&v) && This->event)
        IDOMEvent_preventDefault(&This->event->IDOMEvent_iface);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_returnValue(IHTMLEventObj *iface, VARIANT *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_EMPTY;
    return VariantCopy(p, &This->return_value);
}

static HRESULT WINAPI HTMLEventObj_put_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%x)\n", This, v);

    if(This->event)
        IDOMEvent_put_cancelBubble(&This->event->IDOMEvent_iface, v);
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IDOMEvent_get_cancelBubble(&This->event->IDOMEvent_iface, p);
}

static HRESULT WINAPI HTMLEventObj_get_fromElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_fromElement(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_toElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_toElement(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_keyCode(IHTMLEventObj *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_keyCode(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMKeyboardEvent *keyboard_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMKeyboardEvent, (void**)&keyboard_event))) {
        HRESULT hres = IDOMKeyboardEvent_get_keyCode(keyboard_event, p);
        IDOMKeyboardEvent_Release(keyboard_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_button(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;
    USHORT button = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_button(mouse_event, &button);
        IDOMMouseEvent_Release(mouse_event);
        if(FAILED(hres))
            return hres;
    }

    *p = button;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_type(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event) {
        *p = NULL;
        return S_OK;
    }

    return IDOMEvent_get_type(&This->event->IDOMEvent_iface, p);
}

static HRESULT WINAPI HTMLEventObj_get_qualifier(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_reason(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_x(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG x = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMUIEvent *ui_event;
        nsresult nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event);

        if(NS_SUCCEEDED(nsres)) {
            /* NOTE: pageX is not exactly right here. */
            nsres = nsIDOMUIEvent_GetPageX(ui_event, &x);
            assert(nsres == NS_OK);
            nsIDOMUIEvent_Release(ui_event);
        }
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_y(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    LONG y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event) {
        nsIDOMUIEvent *ui_event;
        nsresult nsres = nsIDOMEvent_QueryInterface(This->event->nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event);

        if(NS_SUCCEEDED(nsres)) {
            /* NOTE: pageY is not exactly right here. */
            nsres = nsIDOMUIEvent_GetPageY(ui_event, &y);
            assert(nsres == NS_OK);
            nsIDOMUIEvent_Release(ui_event);
        }
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_clientX(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_clientY(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_offsetX(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_offsetY(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_screenX(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);
    IDOMMouseEvent *mouse_event;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->event && SUCCEEDED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMouseEvent, (void**)&mouse_event))) {
        HRESULT hres = IDOMMouseEvent_get_screenY(mouse_event, p);
        IDOMMouseEvent_Release(mouse_event);
        return hres;
    }

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_srcFilter(IHTMLEventObj *iface, IDispatch **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static const IHTMLEventObjVtbl HTMLEventObjVtbl = {
    HTMLEventObj_QueryInterface,
    HTMLEventObj_AddRef,
    HTMLEventObj_Release,
    HTMLEventObj_GetTypeInfoCount,
    HTMLEventObj_GetTypeInfo,
    HTMLEventObj_GetIDsOfNames,
    HTMLEventObj_Invoke,
    HTMLEventObj_get_srcElement,
    HTMLEventObj_get_altKey,
    HTMLEventObj_get_ctrlKey,
    HTMLEventObj_get_shiftKey,
    HTMLEventObj_put_returnValue,
    HTMLEventObj_get_returnValue,
    HTMLEventObj_put_cancelBubble,
    HTMLEventObj_get_cancelBubble,
    HTMLEventObj_get_fromElement,
    HTMLEventObj_get_toElement,
    HTMLEventObj_put_keyCode,
    HTMLEventObj_get_keyCode,
    HTMLEventObj_get_button,
    HTMLEventObj_get_type,
    HTMLEventObj_get_qualifier,
    HTMLEventObj_get_reason,
    HTMLEventObj_get_x,
    HTMLEventObj_get_y,
    HTMLEventObj_get_clientX,
    HTMLEventObj_get_clientY,
    HTMLEventObj_get_offsetX,
    HTMLEventObj_get_offsetY,
    HTMLEventObj_get_screenX,
    HTMLEventObj_get_screenY,
    HTMLEventObj_get_srcFilter
};

static inline HTMLEventObj *unsafe_impl_from_IHTMLEventObj(IHTMLEventObj *iface)
{
    return iface->lpVtbl == &HTMLEventObjVtbl ? impl_from_IHTMLEventObj(iface) : NULL;
}

static inline HTMLEventObj *impl_from_IHTMLEventObj2(IHTMLEventObj2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLEventObj2, IHTMLEventObj2, impl_from_IHTMLEventObj2(iface)->dispex)

static HRESULT WINAPI HTMLEventObj2_setAttribute(IHTMLEventObj2 *iface, BSTR strAttributeName, VARIANT AttributeValue, LONG lFlags)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s %s %08lx)\n", This, debugstr_w(strAttributeName), debugstr_variant(&AttributeValue), lFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_getAttribute(IHTMLEventObj2 *iface, BSTR strAttributeName, LONG lFlags, VARIANT *AttributeValue)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s %08lx %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_removeAttribute(IHTMLEventObj2 *iface, BSTR strAttributeName, LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s %lx %p)\n", This, debugstr_w(strAttributeName), lFlags, pfSuccess);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_propertyName(IHTMLEventObj2 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_propertyName(IHTMLEventObj2 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_bookmarks(IHTMLEventObj2 *iface, IHTMLBookmarkCollection *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_bookmarks(IHTMLEventObj2 *iface, IHTMLBookmarkCollection **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_recordset(IHTMLEventObj2 *iface, IDispatch *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_recordset(IHTMLEventObj2 *iface, IDispatch **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_dataFld(IHTMLEventObj2 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_dataFld(IHTMLEventObj2 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_boundElements(IHTMLEventObj2 *iface, IHTMLElementCollection *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_boundElements(IHTMLEventObj2 *iface, IHTMLElementCollection **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_repeat(IHTMLEventObj2 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_repeat(IHTMLEventObj2 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_srcUrn(IHTMLEventObj2 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_srcUrn(IHTMLEventObj2 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_put_srcElement(IHTMLEventObj2 *iface, IHTMLElement *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_srcElement(IHTMLEventObj2 *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_srcElement(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_altKey(IHTMLEventObj2 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_altKey(IHTMLEventObj2 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_altKey(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_ctrlKey(IHTMLEventObj2 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_ctrlKey(IHTMLEventObj2 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_ctrlKey(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_shiftKey(IHTMLEventObj2 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_shiftKey(IHTMLEventObj2 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_shiftKey(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_fromElement(IHTMLEventObj2 *iface, IHTMLElement *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_fromElement(IHTMLEventObj2 *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_fromElement(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_toElement(IHTMLEventObj2 *iface, IHTMLElement *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_toElement(IHTMLEventObj2 *iface, IHTMLElement **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_toElement(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_button(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_button(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_button(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_type(IHTMLEventObj2 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_type(IHTMLEventObj2 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_type(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_qualifier(IHTMLEventObj2 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_qualifier(IHTMLEventObj2 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_qualifier(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_reason(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_reason(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_reason(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_x(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_x(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_x(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_y(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_y(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_y(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_clientX(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_clientX(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_clientX(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_clientY(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_clientY(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_clientY(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_offsetX(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_offsetX(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_offsetX(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_offsetY(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_offsetY(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_offsetY(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_screenX(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_screenX(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_screenX(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_screenY(IHTMLEventObj2 *iface, LONG v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%ld)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_screenY(IHTMLEventObj2 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_screenY(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_put_srcFilter(IHTMLEventObj2 *iface, IDispatch *v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj2_get_srcFilter(IHTMLEventObj2 *iface, IDispatch **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);
    return HTMLEventObj_get_srcFilter(&This->IHTMLEventObj_iface, p);
}

static HRESULT WINAPI HTMLEventObj2_get_dataTransfer(IHTMLEventObj2 *iface, IHTMLDataTransfer **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj2(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IHTMLEventObj2Vtbl HTMLEventObj2Vtbl = {
    HTMLEventObj2_QueryInterface,
    HTMLEventObj2_AddRef,
    HTMLEventObj2_Release,
    HTMLEventObj2_GetTypeInfoCount,
    HTMLEventObj2_GetTypeInfo,
    HTMLEventObj2_GetIDsOfNames,
    HTMLEventObj2_Invoke,
    HTMLEventObj2_setAttribute,
    HTMLEventObj2_getAttribute,
    HTMLEventObj2_removeAttribute,
    HTMLEventObj2_put_propertyName,
    HTMLEventObj2_get_propertyName,
    HTMLEventObj2_put_bookmarks,
    HTMLEventObj2_get_bookmarks,
    HTMLEventObj2_put_recordset,
    HTMLEventObj2_get_recordset,
    HTMLEventObj2_put_dataFld,
    HTMLEventObj2_get_dataFld,
    HTMLEventObj2_put_boundElements,
    HTMLEventObj2_get_boundElements,
    HTMLEventObj2_put_repeat,
    HTMLEventObj2_get_repeat,
    HTMLEventObj2_put_srcUrn,
    HTMLEventObj2_get_srcUrn,
    HTMLEventObj2_put_srcElement,
    HTMLEventObj2_get_srcElement,
    HTMLEventObj2_put_altKey,
    HTMLEventObj2_get_altKey,
    HTMLEventObj2_put_ctrlKey,
    HTMLEventObj2_get_ctrlKey,
    HTMLEventObj2_put_shiftKey,
    HTMLEventObj2_get_shiftKey,
    HTMLEventObj2_put_fromElement,
    HTMLEventObj2_get_fromElement,
    HTMLEventObj2_put_toElement,
    HTMLEventObj2_get_toElement,
    HTMLEventObj2_put_button,
    HTMLEventObj2_get_button,
    HTMLEventObj2_put_type,
    HTMLEventObj2_get_type,
    HTMLEventObj2_put_qualifier,
    HTMLEventObj2_get_qualifier,
    HTMLEventObj2_put_reason,
    HTMLEventObj2_get_reason,
    HTMLEventObj2_put_x,
    HTMLEventObj2_get_x,
    HTMLEventObj2_put_y,
    HTMLEventObj2_get_y,
    HTMLEventObj2_put_clientX,
    HTMLEventObj2_get_clientX,
    HTMLEventObj2_put_clientY,
    HTMLEventObj2_get_clientY,
    HTMLEventObj2_put_offsetX,
    HTMLEventObj2_get_offsetX,
    HTMLEventObj2_put_offsetY,
    HTMLEventObj2_get_offsetY,
    HTMLEventObj2_put_screenX,
    HTMLEventObj2_get_screenX,
    HTMLEventObj2_put_screenY,
    HTMLEventObj2_get_screenY,
    HTMLEventObj2_put_srcFilter,
    HTMLEventObj2_get_srcFilter,
    HTMLEventObj2_get_dataTransfer
};

static inline HTMLEventObj *impl_from_IHTMLEventObj3(IHTMLEventObj3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj3_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLEventObj3, IHTMLEventObj3, impl_from_IHTMLEventObj3(iface)->dispex)

static HRESULT WINAPI HTMLEventObj3_get_contentOverflow(IHTMLEventObj3 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_put_shiftLeft(IHTMLEventObj3 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_shiftLeft(IHTMLEventObj3 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_put_altLeft(IHTMLEventObj3 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_altLeft(IHTMLEventObj3 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_put_ctrlLeft(IHTMLEventObj3 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_ctrlLeft(IHTMLEventObj3 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_imeCompositionChange(IHTMLEventObj3 *iface, LONG_PTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_imeNotifyCommand(IHTMLEventObj3 *iface, LONG_PTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_imeNotifyData(IHTMLEventObj3 *iface, LONG_PTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_imeRequest(IHTMLEventObj3 *iface, LONG_PTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_imeRequestData(IHTMLEventObj3 *iface, LONG_PTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_keyboardLayout(IHTMLEventObj3 *iface, LONG_PTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_behaviorCookie(IHTMLEventObj3 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_behaviorPart(IHTMLEventObj3 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj3_get_nextPage(IHTMLEventObj3 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj3(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IHTMLEventObj3Vtbl HTMLEventObj3Vtbl = {
    HTMLEventObj3_QueryInterface,
    HTMLEventObj3_AddRef,
    HTMLEventObj3_Release,
    HTMLEventObj3_GetTypeInfoCount,
    HTMLEventObj3_GetTypeInfo,
    HTMLEventObj3_GetIDsOfNames,
    HTMLEventObj3_Invoke,
    HTMLEventObj3_get_contentOverflow,
    HTMLEventObj3_put_shiftLeft,
    HTMLEventObj3_get_shiftLeft,
    HTMLEventObj3_put_altLeft,
    HTMLEventObj3_get_altLeft,
    HTMLEventObj3_put_ctrlLeft,
    HTMLEventObj3_get_ctrlLeft,
    HTMLEventObj3_get_imeCompositionChange,
    HTMLEventObj3_get_imeNotifyCommand,
    HTMLEventObj3_get_imeNotifyData,
    HTMLEventObj3_get_imeRequest,
    HTMLEventObj3_get_imeRequestData,
    HTMLEventObj3_get_keyboardLayout,
    HTMLEventObj3_get_behaviorCookie,
    HTMLEventObj3_get_behaviorPart,
    HTMLEventObj3_get_nextPage
};

static inline HTMLEventObj *impl_from_IHTMLEventObj4(IHTMLEventObj4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj4_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLEventObj4, IHTMLEventObj4, impl_from_IHTMLEventObj4(iface)->dispex)

static HRESULT WINAPI HTMLEventObj4_get_wheelDelta(IHTMLEventObj4 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj4(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IHTMLEventObj4Vtbl HTMLEventObj4Vtbl = {
    HTMLEventObj4_QueryInterface,
    HTMLEventObj4_AddRef,
    HTMLEventObj4_Release,
    HTMLEventObj4_GetTypeInfoCount,
    HTMLEventObj4_GetTypeInfo,
    HTMLEventObj4_GetIDsOfNames,
    HTMLEventObj4_Invoke,
    HTMLEventObj4_get_wheelDelta
};

static inline HTMLEventObj *impl_from_IHTMLEventObj5(IHTMLEventObj5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj5_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLEventObj5, IHTMLEventObj5, impl_from_IHTMLEventObj5(iface)->dispex)

static HRESULT WINAPI HTMLEventObj5_put_url(IHTMLEventObj5 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);
    IDOMStorageEvent *storage_event;
    DOMStorageEvent *p;
    BSTR url;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!v)
        return E_POINTER;

    if(!This->event || FAILED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMStorageEvent, (void**)&storage_event)))
        return DISP_E_MEMBERNOTFOUND;
    IDOMStorageEvent_Release(storage_event);

    if(!(url = SysAllocString(v)))
        return E_OUTOFMEMORY;

    p = DOMStorageEvent_from_DOMEvent(This->event);
    SysFreeString(p->url);
    p->url = url;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj5_get_url(IHTMLEventObj5 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);
    IDOMStorageEvent *storage_event;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event || FAILED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMStorageEvent, (void**)&storage_event))) {
        *p = NULL;
        return S_OK;
    }

    hres = IDOMStorageEvent_get_url(storage_event, p);
    IDOMStorageEvent_Release(storage_event);
    return hres;
}

static HRESULT WINAPI HTMLEventObj5_put_data(IHTMLEventObj5 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj5_get_data(IHTMLEventObj5 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);
    IDOMMessageEvent *message_event;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event || FAILED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMessageEvent, (void**)&message_event))) {
        *p = NULL;
        return S_OK;
    }

    hres = IDOMMessageEvent_get_data(message_event, p);
    IDOMMessageEvent_Release(message_event);
    return hres;
}

static HRESULT WINAPI HTMLEventObj5_get_source(IHTMLEventObj5 *iface, IDispatch **p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);
    IDOMMessageEvent *message_event;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event || FAILED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMessageEvent, (void**)&message_event))) {
        *p = NULL;
        return S_OK;
    }

    hres = IDOMMessageEvent_get_source(message_event, (IHTMLWindow2**)p);
    IDOMMessageEvent_Release(message_event);
    return hres;
}

static HRESULT WINAPI HTMLEventObj5_put_origin(IHTMLEventObj5 *iface, BSTR v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_w(v));

    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT WINAPI HTMLEventObj5_get_origin(IHTMLEventObj5 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);
    IDOMMessageEvent *message_event;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->event || FAILED(IDOMEvent_QueryInterface(&This->event->IDOMEvent_iface, &IID_IDOMMessageEvent, (void**)&message_event))) {
        *p = NULL;
        return S_OK;
    }

    hres = IDOMMessageEvent_get_origin(message_event, p);
    IDOMMessageEvent_Release(message_event);
    return hres;
}

static HRESULT WINAPI HTMLEventObj5_put_issession(IHTMLEventObj5 *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);

    FIXME("(%p)->(%x)\n", This, v);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj5_get_issession(IHTMLEventObj5 *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj5(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IHTMLEventObj5Vtbl HTMLEventObj5Vtbl = {
    HTMLEventObj5_QueryInterface,
    HTMLEventObj5_AddRef,
    HTMLEventObj5_Release,
    HTMLEventObj5_GetTypeInfoCount,
    HTMLEventObj5_GetTypeInfo,
    HTMLEventObj5_GetIDsOfNames,
    HTMLEventObj5_Invoke,
    HTMLEventObj5_put_url,
    HTMLEventObj5_get_url,
    HTMLEventObj5_put_data,
    HTMLEventObj5_get_data,
    HTMLEventObj5_get_source,
    HTMLEventObj5_put_origin,
    HTMLEventObj5_get_origin,
    HTMLEventObj5_put_issession,
    HTMLEventObj5_get_issession
};

static inline HTMLEventObj *impl_from_IHTMLEventObj6(IHTMLEventObj6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, IHTMLEventObj6_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLEventObj6, IHTMLEventObj6, impl_from_IHTMLEventObj6(iface)->dispex)

static HRESULT WINAPI HTMLEventObj6_get_actionURL(IHTMLEventObj6 *iface, BSTR *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj6(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj6_get_buttonID(IHTMLEventObj6 *iface, LONG *p)
{
    HTMLEventObj *This = impl_from_IHTMLEventObj6(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IHTMLEventObj6Vtbl HTMLEventObj6Vtbl = {
    HTMLEventObj6_QueryInterface,
    HTMLEventObj6_AddRef,
    HTMLEventObj6_Release,
    HTMLEventObj6_GetTypeInfoCount,
    HTMLEventObj6_GetTypeInfo,
    HTMLEventObj6_GetIDsOfNames,
    HTMLEventObj6_Invoke,
    HTMLEventObj6_get_actionURL,
    HTMLEventObj6_get_buttonID
};

static inline HTMLEventObj *HTMLEventObj_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLEventObj, dispex);
}

static void *HTMLEventObj_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLEventObj *This = HTMLEventObj_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLEventObj, riid))
        return &This->IHTMLEventObj_iface;
    if(IsEqualGUID(&IID_IHTMLEventObj2, riid))
        return &This->IHTMLEventObj2_iface;
    if(IsEqualGUID(&IID_IHTMLEventObj3, riid))
        return &This->IHTMLEventObj3_iface;
    if(IsEqualGUID(&IID_IHTMLEventObj4, riid))
        return &This->IHTMLEventObj4_iface;
    if(IsEqualGUID(&IID_IHTMLEventObj5, riid))
        return &This->IHTMLEventObj5_iface;
    if(IsEqualGUID(&IID_IHTMLEventObj6, riid))
        return &This->IHTMLEventObj6_iface;

    return NULL;
}

static void HTMLEventObj_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLEventObj *This = HTMLEventObj_from_DispatchEx(dispex);
    if(This->event)
        note_cc_edge((nsISupports*)&This->event->IDOMEvent_iface, "event", cb);
}

static void HTMLEventObj_unlink(DispatchEx *dispex)
{
    HTMLEventObj *This = HTMLEventObj_from_DispatchEx(dispex);
    if(This->event) {
        DOMEvent *event = This->event;
        This->event = NULL;
        IDOMEvent_Release(&event->IDOMEvent_iface);
    }
}

static void HTMLEventObj_destructor(DispatchEx *dispex)
{
    HTMLEventObj *This = HTMLEventObj_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLEventObj_dispex_vtbl = {
    .query_interface  = HTMLEventObj_query_interface,
    .destructor       = HTMLEventObj_destructor,
    .traverse         = HTMLEventObj_traverse,
    .unlink           = HTMLEventObj_unlink
};

static const tid_t MSEventObj_iface_tids[] = {
    IHTMLEventObj5_tid,
    IHTMLEventObj_tid,
    0
};

dispex_static_data_t MSEventObj_dispex = {
    .id         = PROT_MSEventObj,
    .vtbl       = &HTMLEventObj_dispex_vtbl,
    .disp_tid   = DispCEventObj_tid,
    .iface_tids = MSEventObj_iface_tids,
};

static HTMLEventObj *alloc_event_obj(DOMEvent *event, HTMLInnerWindow *script_global)
{
    HTMLEventObj *event_obj;

    event_obj = calloc(1, sizeof(*event_obj));
    if(!event_obj)
        return NULL;

    event_obj->IHTMLEventObj_iface.lpVtbl = &HTMLEventObjVtbl;
    event_obj->IHTMLEventObj2_iface.lpVtbl = &HTMLEventObj2Vtbl;
    event_obj->IHTMLEventObj3_iface.lpVtbl = &HTMLEventObj3Vtbl;
    event_obj->IHTMLEventObj4_iface.lpVtbl = &HTMLEventObj4Vtbl;
    event_obj->IHTMLEventObj5_iface.lpVtbl = &HTMLEventObj5Vtbl;
    event_obj->IHTMLEventObj6_iface.lpVtbl = &HTMLEventObj6Vtbl;
    event_obj->event = event;
    if(event) {
        IDOMEvent_AddRef(&event->IDOMEvent_iface);
        init_dispatch_with_owner(&event_obj->dispex, &MSEventObj_dispex, &event->dispex);
    }else {
        init_dispatch(&event_obj->dispex, &MSEventObj_dispex, script_global,
                      dispex_compat_mode(&script_global->event_target.dispex));
    }
    return event_obj;
}

HRESULT create_event_obj(DOMEvent *event, HTMLDocumentNode *doc, IHTMLEventObj **ret)
{
    HTMLEventObj *event_obj;

    event_obj = alloc_event_obj(event, doc->script_global);
    if(!event_obj)
        return E_OUTOFMEMORY;

    *ret = &event_obj->IHTMLEventObj_iface;
    return S_OK;
}

static inline DOMEvent *impl_from_IDOMEvent(IDOMEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, IDOMEvent_iface);
}

static const IDOMEventVtbl DOMEventVtbl;

static inline DOMEvent *unsafe_impl_from_IDOMEvent(IDOMEvent *iface)
{
    return iface && iface->lpVtbl == &DOMEventVtbl ? impl_from_IDOMEvent(iface) : NULL;
}

DISPEX_IDISPATCH_IMPL(DOMEvent, IDOMEvent, impl_from_IDOMEvent(iface)->dispex)

static HRESULT WINAPI DOMEvent_get_bubbles(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->bubbles);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_cancelable(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->cancelable);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_currentTarget(IDOMEvent *iface, IEventTarget **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->current_target)
        *p = get_event_target_iface(This->current_target);
    else
        *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_defaultPrevented(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->prevent_default);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_eventPhase(IDOMEvent *iface, USHORT *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->phase;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_target(IDOMEvent *iface, IEventTarget **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->target)
        *p = get_event_target_iface(This->target);
    else
        *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_timeStamp(IDOMEvent *iface, ULONGLONG *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->time_stamp;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_type(IDOMEvent *iface, BSTR *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->type) {
        *p = SysAllocString(This->type);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }
    return S_OK;
}

#ifdef __i386__
#define nsIDOMEvent_InitEvent(_this,type,bubbles,cancelable) \
    ((void (WINAPI*)(void*,nsIDOMEvent*,const nsAString*,cpp_bool,cpp_bool)) \
     &call_thiscall_func)((_this)->lpVtbl->InitEvent,_this,type,bubbles,cancelable)

#endif

static HRESULT WINAPI DOMEvent_initEvent(IDOMEvent *iface, BSTR type, VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);
    nsAString nsstr;

    TRACE("(%p)->(%s %x %x)\n", This, debugstr_w(type), can_bubble, cancelable);

    if(This->target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    free(This->type);
    This->type = wcsdup(type);
    if(!This->type)
        return E_OUTOFMEMORY;
    This->event_id = str_to_eid(type);

    This->bubbles = !!can_bubble;
    This->cancelable = !!cancelable;

    nsAString_InitDepend(&nsstr, type);
    nsIDOMEvent_InitEvent(This->nsevent, &nsstr, This->bubbles, This->cancelable);
    nsAString_Finish(&nsstr);

    return S_OK;
}

static HRESULT WINAPI DOMEvent_preventDefault(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    if(This->current_target && This->cancelable) {
        This->prevent_default = TRUE;
        nsIDOMEvent_PreventDefault(This->nsevent);
    }
    return S_OK;
}

static HRESULT WINAPI DOMEvent_stopPropagation(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    This->stop_propagation = TRUE;
    return S_OK;
}

static HRESULT WINAPI DOMEvent_stopImmediatePropagation(IDOMEvent *iface)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)\n", This);

    This->stop_immediate_propagation = This->stop_propagation = TRUE;
    nsIDOMEvent_StopImmediatePropagation(This->nsevent);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_isTrusted(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->trusted);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_put_cancelBubble(IDOMEvent *iface, VARIANT_BOOL v)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%x)\n", This, v);

    if(This->phase < 2)
        return S_OK;

    /* stop_immediate_propagation is not able to be interrupted, but native has a weird behavior. */
    This->stop_propagation = (v != VARIANT_FALSE);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_cancelBubble(IDOMEvent *iface, VARIANT_BOOL *p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->stop_propagation);
    return S_OK;
}

static HRESULT WINAPI DOMEvent_get_srcElement(IDOMEvent *iface, IHTMLElement **p)
{
    DOMEvent *This = impl_from_IDOMEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->target)
        IWineJSDispatchHost_QueryInterface(&This->target->dispex.IWineJSDispatchHost_iface, &IID_IHTMLElement, (void**)p);
    else
        *p = NULL;
    return S_OK;
}

static const IDOMEventVtbl DOMEventVtbl = {
    DOMEvent_QueryInterface,
    DOMEvent_AddRef,
    DOMEvent_Release,
    DOMEvent_GetTypeInfoCount,
    DOMEvent_GetTypeInfo,
    DOMEvent_GetIDsOfNames,
    DOMEvent_Invoke,
    DOMEvent_get_bubbles,
    DOMEvent_get_cancelable,
    DOMEvent_get_currentTarget,
    DOMEvent_get_defaultPrevented,
    DOMEvent_get_eventPhase,
    DOMEvent_get_target,
    DOMEvent_get_timeStamp,
    DOMEvent_get_type,
    DOMEvent_initEvent,
    DOMEvent_preventDefault,
    DOMEvent_stopPropagation,
    DOMEvent_stopImmediatePropagation,
    DOMEvent_get_isTrusted,
    DOMEvent_put_cancelBubble,
    DOMEvent_get_cancelBubble,
    DOMEvent_get_srcElement
};

static inline DOMEvent *DOMEvent_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, DOMEvent, dispex);
}

static void *DOMEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMEvent *This = DOMEvent_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IDOMEvent, riid))
        return &This->IDOMEvent_iface;

    return NULL;
}

static void DOMEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMEvent *This = DOMEvent_from_DispatchEx(dispex);
    if(This->target)
        note_cc_edge((nsISupports*)&This->target->IEventTarget_iface, "target", cb);
    if(This->nsevent)
        note_cc_edge((nsISupports*)This->nsevent, "nsevent", cb);
}

static void DOMEvent_unlink(DispatchEx *dispex)
{
    DOMEvent *This = DOMEvent_from_DispatchEx(dispex);
    if(This->target) {
        EventTarget *target = This->target;
        This->target = NULL;
        IEventTarget_Release(&target->IEventTarget_iface);
    }
    unlink_ref(&This->nsevent);
}

static void DOMEvent_destructor(DispatchEx *dispex)
{
    DOMEvent *This = DOMEvent_from_DispatchEx(dispex);
    free(This->type);
    free(This);
}

static inline DOMUIEvent *impl_from_IDOMUIEvent(IDOMUIEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMUIEvent, IDOMUIEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMUIEvent, IDOMUIEvent, impl_from_IDOMUIEvent(iface)->event.dispex)

static HRESULT WINAPI DOMUIEvent_get_view(IDOMUIEvent *iface, IHTMLWindow2 **p)
{
    DOMUIEvent *This = impl_from_IDOMUIEvent(iface);
    mozIDOMWindowProxy *moz_window;
    HTMLOuterWindow *view = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMUIEvent_GetView(This->nsevent, &moz_window);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(moz_window) {
        view = mozwindow_to_window(moz_window);
        mozIDOMWindowProxy_Release(moz_window);
    }
    if(view)
        IHTMLWindow2_AddRef((*p = &view->base.IHTMLWindow2_iface));
    else
        *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMUIEvent_get_detail(IDOMUIEvent *iface, LONG *p)
{
    DOMUIEvent *This = impl_from_IDOMUIEvent(iface);
    LONG detail;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMUIEvent_GetDetail(This->nsevent, &detail);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = detail;
    return S_OK;
}

static HRESULT WINAPI DOMUIEvent_initUIEvent(IDOMUIEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
        VARIANT_BOOL cancelable, IHTMLWindow2 *view, LONG detail)
{
    DOMUIEvent *This = impl_from_IDOMUIEvent(iface);
    nsAString type_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %p %lx)\n", This, debugstr_w(type), can_bubble, cancelable, view, detail);

    if(This->event.target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    if(view)
        FIXME("view argument is not supported\n");

    hres = IDOMEvent_initEvent(&This->event.IDOMEvent_iface, type, can_bubble, cancelable);
    if(FAILED(hres))
        return hres;

    nsAString_InitDepend(&type_str, type);
    nsres = nsIDOMUIEvent_InitUIEvent(This->nsevent, &type_str, !!can_bubble, !!cancelable,
                                      NULL /* FIXME */, detail);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres)) {
        FIXME("InitUIEvent failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static const IDOMUIEventVtbl DOMUIEventVtbl = {
    DOMUIEvent_QueryInterface,
    DOMUIEvent_AddRef,
    DOMUIEvent_Release,
    DOMUIEvent_GetTypeInfoCount,
    DOMUIEvent_GetTypeInfo,
    DOMUIEvent_GetIDsOfNames,
    DOMUIEvent_Invoke,
    DOMUIEvent_get_view,
    DOMUIEvent_get_detail,
    DOMUIEvent_initUIEvent
};

static void *DOMUIEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMUIEvent *This = DOMUIEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMUIEvent, riid))
        return &This->IDOMUIEvent_iface;
    return DOMEvent_query_interface(&This->event.dispex, riid);
}

static void DOMUIEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMUIEvent *This = DOMUIEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_traverse(&This->event.dispex, cb);
    if(This->nsevent)
        note_cc_edge((nsISupports*)This->nsevent, "UIEvent.nsevent", cb);
}

static void DOMUIEvent_unlink(DispatchEx *dispex)
{
    DOMUIEvent *This = DOMUIEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_unlink(&This->event.dispex);
    unlink_ref(&This->nsevent);
}

typedef struct {
    DOMUIEvent ui_event;
    IDOMMouseEvent IDOMMouseEvent_iface;
    nsIDOMMouseEvent *nsevent;
} DOMMouseEvent;

static inline DOMMouseEvent *impl_from_IDOMMouseEvent(IDOMMouseEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMMouseEvent, IDOMMouseEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMMouseEvent, IDOMMouseEvent, impl_from_IDOMMouseEvent(iface)->ui_event.event.dispex)

static HRESULT WINAPI DOMMouseEvent_get_screenX(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    LONG screen_x;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetScreenX(This->nsevent, &screen_x);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = screen_x;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_screenY(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    LONG screen_y;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetScreenY(This->nsevent, &screen_y);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = screen_y;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_clientX(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    LONG client_x;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetClientX(This->nsevent, &client_x);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = client_x;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_clientY(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    LONG client_y;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetClientY(This->nsevent, &client_y);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = client_y;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_ctrlKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetCtrlKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_shiftKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetShiftKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_altKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetAltKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_metaKey(IDOMMouseEvent *iface, VARIANT_BOOL *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetMetaKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_button(IDOMMouseEvent *iface, USHORT *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    INT16 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetButton(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_relatedTarget(IDOMMouseEvent *iface, IEventTarget **p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    nsIDOMEventTarget *related_target;
    nsIDOMNode *target_node;
    HTMLDOMNode *node;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetRelatedTarget(This->nsevent, &related_target);
    if(NS_FAILED(nsres))
        return E_FAIL;

    if(!related_target) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMEventTarget_QueryInterface(related_target, &IID_nsIDOMNode, (void**)&target_node);
    nsIDOMEventTarget_Release(related_target);
    if(NS_FAILED(nsres)) {
        FIXME("Only node targets supported\n");
        return E_NOTIMPL;
    }

    hres = get_node(target_node, TRUE, &node);
    nsIDOMNode_Release(target_node);
    if(FAILED(hres))
        return hres;

    *p = &node->event_target.IEventTarget_iface;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_initMouseEvent(IDOMMouseEvent *iface, BSTR type,
        VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable, IHTMLWindow2 *view, LONG detail,
        LONG screen_x, LONG screen_y, LONG client_x, LONG client_y, VARIANT_BOOL ctrl_key,
        VARIANT_BOOL alt_key, VARIANT_BOOL shift_key, VARIANT_BOOL meta_key, USHORT button,
        IEventTarget *related_target)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    nsIDOMEventTarget *nstarget = NULL;
    nsAString type_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %p %ld %ld %ld %ld %ld %x %x %x %x %u %p)\n", This, debugstr_w(type),
          can_bubble, cancelable, view, detail, screen_x, screen_y, client_x, client_y,
          ctrl_key, alt_key, shift_key, meta_key, button, related_target);

    if(This->ui_event.event.target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    if(view)
        FIXME("view argument is not supported\n");

    if(related_target) {
        hres = get_gecko_target(related_target, &nstarget);
        if(FAILED(hres))
            return hres;
    }

    hres = IDOMEvent_initEvent(&This->ui_event.event.IDOMEvent_iface, type, can_bubble, cancelable);
    if(SUCCEEDED(hres)) {
        nsAString_InitDepend(&type_str, type);
        nsres = nsIDOMMouseEvent_InitMouseEvent(This->nsevent, &type_str, can_bubble, cancelable,
                                                NULL /* FIXME */, detail, screen_x, screen_y,
                                                client_x, client_y, !!ctrl_key, !!alt_key, !!shift_key,
                                                !!meta_key, button, nstarget);
        nsAString_Finish(&type_str);
        if(NS_FAILED(nsres)) {
            FIXME("InitMouseEvent failed: %08lx\n", nsres);
            return E_FAIL;
        }
    }

    if(nstarget)
        nsIDOMEventTarget_Release(nstarget);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_getModifierState(IDOMMouseEvent *iface, BSTR key,
        VARIANT_BOOL *activated)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(key), activated);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_buttons(IDOMMouseEvent *iface, USHORT *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    UINT16 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetButtons(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_fromElement(IDOMMouseEvent *iface, IHTMLElement **p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    eventid_t event_id = This->ui_event.event.event_id;
    IEventTarget  *related_target = NULL;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    if(event_info[event_id].flags & EVENT_MOUSE_FROM_RELATED)
        hres = IDOMMouseEvent_get_relatedTarget(&This->IDOMMouseEvent_iface, &related_target);
    else if(event_info[event_id].flags & EVENT_MOUSE_TO_RELATED)
        hres = IDOMEvent_get_target(&This->ui_event.event.IDOMEvent_iface, &related_target);
    if(FAILED(hres))
        return hres;

    if(!related_target) {
        *p = NULL;
        return S_OK;
    }

    IEventTarget_QueryInterface(related_target, &IID_IHTMLElement, (void**)p);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_toElement(IDOMMouseEvent *iface, IHTMLElement **p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    eventid_t event_id = This->ui_event.event.event_id;
    IEventTarget  *related_target = NULL;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    if(event_info[event_id].flags & EVENT_MOUSE_TO_RELATED)
        hres = IDOMMouseEvent_get_relatedTarget(&This->IDOMMouseEvent_iface, &related_target);
    else if(event_info[event_id].flags & EVENT_MOUSE_FROM_RELATED)
        hres = IDOMEvent_get_target(&This->ui_event.event.IDOMEvent_iface, &related_target);
    if(FAILED(hres))
        return hres;

    if(!related_target) {
        *p = NULL;
        return S_OK;
    }

    IEventTarget_QueryInterface(related_target, &IID_IHTMLElement, (void**)p);
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_x(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_y(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMMouseEvent_get_offsetX(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);

    FIXME("(%p)->(%p) returning 0\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_offsetY(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);

    FIXME("(%p)->(%p) returning 0\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_pageX(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    LONG r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetPageX(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_pageY(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    LONG r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetPageY(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_layerX(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    nsresult nsres;
    LONG r;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetLayerX(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_layerY(IDOMMouseEvent *iface, LONG *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    nsresult nsres;
    LONG r;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetLayerY(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMMouseEvent_get_which(IDOMMouseEvent *iface, USHORT *p)
{
    DOMMouseEvent *This = impl_from_IDOMMouseEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMMouseEvent_GetWhich(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static const IDOMMouseEventVtbl DOMMouseEventVtbl = {
    DOMMouseEvent_QueryInterface,
    DOMMouseEvent_AddRef,
    DOMMouseEvent_Release,
    DOMMouseEvent_GetTypeInfoCount,
    DOMMouseEvent_GetTypeInfo,
    DOMMouseEvent_GetIDsOfNames,
    DOMMouseEvent_Invoke,
    DOMMouseEvent_get_screenX,
    DOMMouseEvent_get_screenY,
    DOMMouseEvent_get_clientX,
    DOMMouseEvent_get_clientY,
    DOMMouseEvent_get_ctrlKey,
    DOMMouseEvent_get_shiftKey,
    DOMMouseEvent_get_altKey,
    DOMMouseEvent_get_metaKey,
    DOMMouseEvent_get_button,
    DOMMouseEvent_get_relatedTarget,
    DOMMouseEvent_initMouseEvent,
    DOMMouseEvent_getModifierState,
    DOMMouseEvent_get_buttons,
    DOMMouseEvent_get_fromElement,
    DOMMouseEvent_get_toElement,
    DOMMouseEvent_get_x,
    DOMMouseEvent_get_y,
    DOMMouseEvent_get_offsetX,
    DOMMouseEvent_get_offsetY,
    DOMMouseEvent_get_pageX,
    DOMMouseEvent_get_pageY,
    DOMMouseEvent_get_layerX,
    DOMMouseEvent_get_layerY,
    DOMMouseEvent_get_which
};

static DOMMouseEvent *DOMMouseEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMMouseEvent, ui_event.event);
}

static void *DOMMouseEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMMouseEvent *This = DOMMouseEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMMouseEvent, riid))
        return &This->IDOMMouseEvent_iface;
    if(IsEqualGUID(&IID_IDOMUIEvent, riid))
        return &This->ui_event.IDOMUIEvent_iface;
    return DOMEvent_query_interface(&This->ui_event.event.dispex, riid);
}

static void DOMMouseEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMMouseEvent *This = DOMMouseEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMUIEvent_traverse(&This->ui_event.event.dispex, cb);
    if(This->nsevent)
        note_cc_edge((nsISupports*)This->nsevent, "MouseEvent.nsevent", cb);
}

static void DOMMouseEvent_unlink(DispatchEx *dispex)
{
    DOMMouseEvent *This = DOMMouseEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMUIEvent_unlink(&This->ui_event.event.dispex);
    unlink_ref(&This->nsevent);
}

typedef struct {
    DOMUIEvent ui_event;
    IDOMKeyboardEvent IDOMKeyboardEvent_iface;
    nsIDOMKeyEvent *nsevent;
} DOMKeyboardEvent;

static inline DOMKeyboardEvent *impl_from_IDOMKeyboardEvent(IDOMKeyboardEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMKeyboardEvent, IDOMKeyboardEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMKeyboardEvent, IDOMKeyboardEvent,
                      impl_from_IDOMKeyboardEvent(iface)->ui_event.event.dispex)

static HRESULT WINAPI DOMKeyboardEvent_get_key(IDOMKeyboardEvent *iface, BSTR *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    nsAString key_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);


    nsAString_Init(&key_str, NULL);
    nsres = nsIDOMKeyEvent_GetKey(This->nsevent, &key_str);
    return return_nsstr(nsres, &key_str, p);
}

static HRESULT WINAPI DOMKeyboardEvent_get_location(IDOMKeyboardEvent *iface, ULONG *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetLocation(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_ctrlKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetCtrlKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_shiftKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetShiftKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_altKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetAltKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_metaKey(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetMetaKey(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_repeat(IDOMKeyboardEvent *iface, VARIANT_BOOL *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    cpp_bool r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetRepeat(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = variant_bool(r);
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_getModifierState(IDOMKeyboardEvent *iface, BSTR key,
        VARIANT_BOOL *state)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(key), state);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMKeyboardEvent_initKeyboardEvent(IDOMKeyboardEvent *iface, BSTR type,
        VARIANT_BOOL can_bubble, VARIANT_BOOL cancelable, IHTMLWindow2 *view, BSTR key,
        ULONG location, BSTR modifiers_list, VARIANT_BOOL repeat, BSTR locale)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%s %x %x %p %s %lu %s %x %s)\n", This, debugstr_w(type), can_bubble,
          cancelable, view, debugstr_w(key), location, debugstr_w(modifiers_list),
          repeat, debugstr_w(locale));
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMKeyboardEvent_get_keyCode(IDOMKeyboardEvent *iface, LONG *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetKeyCode(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_charCode(IDOMKeyboardEvent *iface, LONG *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetKeyCode(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_which(IDOMKeyboardEvent *iface, LONG *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    UINT32 r;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMKeyEvent_GetWhich(This->nsevent, &r);
    if(NS_FAILED(nsres))
        return E_FAIL;

    *p = r;
    return S_OK;
}

static HRESULT WINAPI DOMKeyboardEvent_get_char(IDOMKeyboardEvent *iface, VARIANT *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMKeyboardEvent_get_locale(IDOMKeyboardEvent *iface, BSTR *p)
{
    DOMKeyboardEvent *This = impl_from_IDOMKeyboardEvent(iface);

    FIXME("(%p)->(%p) semi-stub\n", This, p);

    *p = SysAllocString(L"");
    return *p ? S_OK : E_OUTOFMEMORY;
}

static const IDOMKeyboardEventVtbl DOMKeyboardEventVtbl = {
    DOMKeyboardEvent_QueryInterface,
    DOMKeyboardEvent_AddRef,
    DOMKeyboardEvent_Release,
    DOMKeyboardEvent_GetTypeInfoCount,
    DOMKeyboardEvent_GetTypeInfo,
    DOMKeyboardEvent_GetIDsOfNames,
    DOMKeyboardEvent_Invoke,
    DOMKeyboardEvent_get_key,
    DOMKeyboardEvent_get_location,
    DOMKeyboardEvent_get_ctrlKey,
    DOMKeyboardEvent_get_shiftKey,
    DOMKeyboardEvent_get_altKey,
    DOMKeyboardEvent_get_metaKey,
    DOMKeyboardEvent_get_repeat,
    DOMKeyboardEvent_getModifierState,
    DOMKeyboardEvent_initKeyboardEvent,
    DOMKeyboardEvent_get_keyCode,
    DOMKeyboardEvent_get_charCode,
    DOMKeyboardEvent_get_which,
    DOMKeyboardEvent_get_char,
    DOMKeyboardEvent_get_locale
};

static DOMKeyboardEvent *DOMKeyboardEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMKeyboardEvent, ui_event.event);
}

static void *DOMKeyboardEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMKeyboardEvent *This = DOMKeyboardEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMKeyboardEvent, riid))
        return &This->IDOMKeyboardEvent_iface;
    if(IsEqualGUID(&IID_IDOMUIEvent, riid))
        return &This->ui_event.IDOMUIEvent_iface;
    return DOMEvent_query_interface(&This->ui_event.event.dispex, riid);
}

static void DOMKeyboardEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMKeyboardEvent *This = DOMKeyboardEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMUIEvent_traverse(&This->ui_event.event.dispex, cb);
    if(This->nsevent)
        note_cc_edge((nsISupports*)This->nsevent, "KeyboardEvent.nsevent", cb);
}

static void DOMKeyboardEvent_unlink(DispatchEx *dispex)
{
    DOMKeyboardEvent *This = DOMKeyboardEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMUIEvent_unlink(&This->ui_event.event.dispex);
    unlink_ref(&This->nsevent);
}

typedef struct {
    DOMEvent event;
    IWinePageTransitionEvent IWinePageTransitionEvent_iface;
} DOMPageTransitionEvent;

static inline DOMPageTransitionEvent *impl_from_IWinePageTransitionEvent(IWinePageTransitionEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMPageTransitionEvent, IWinePageTransitionEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMPageTransitionEvent, IWinePageTransitionEvent,
                      impl_from_IWinePageTransitionEvent(iface)->event.dispex)

static HRESULT WINAPI DOMPageTransitionEvent_get_persisted(IWinePageTransitionEvent *iface, VARIANT_BOOL *p)
{
    DOMPageTransitionEvent *This = impl_from_IWinePageTransitionEvent(iface);

    FIXME("(%p)->(%p): always returning FALSE\n", This, p);

    *p = VARIANT_FALSE;
    return S_OK;
}

static const IWinePageTransitionEventVtbl DOMPageTransitionEventVtbl = {
    DOMPageTransitionEvent_QueryInterface,
    DOMPageTransitionEvent_AddRef,
    DOMPageTransitionEvent_Release,
    DOMPageTransitionEvent_GetTypeInfoCount,
    DOMPageTransitionEvent_GetTypeInfo,
    DOMPageTransitionEvent_GetIDsOfNames,
    DOMPageTransitionEvent_Invoke,
    DOMPageTransitionEvent_get_persisted
};

static DOMPageTransitionEvent *DOMPageTransitionEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMPageTransitionEvent, event);
}

static void *DOMPageTransitionEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMPageTransitionEvent *page_transition_event = DOMPageTransitionEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IWinePageTransitionEvent, riid))
        return &page_transition_event->IWinePageTransitionEvent_iface;
    return DOMEvent_query_interface(&page_transition_event->event.dispex, riid);
}

typedef struct {
    DOMEvent event;
    IDOMCustomEvent IDOMCustomEvent_iface;
    VARIANT detail;
} DOMCustomEvent;

static inline DOMCustomEvent *impl_from_IDOMCustomEvent(IDOMCustomEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMCustomEvent, IDOMCustomEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMCustomEvent, IDOMCustomEvent, impl_from_IDOMCustomEvent(iface)->event.dispex)

static HRESULT WINAPI DOMCustomEvent_get_detail(IDOMCustomEvent *iface, VARIANT *p)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_EMPTY;
    return VariantCopy(p, &This->detail);
}

static HRESULT WINAPI DOMCustomEvent_initCustomEvent(IDOMCustomEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
                                                     VARIANT_BOOL cancelable, VARIANT *detail)
{
    DOMCustomEvent *This = impl_from_IDOMCustomEvent(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %s)\n", This, debugstr_w(type), can_bubble, cancelable, debugstr_variant(detail));

    hres = IDOMEvent_initEvent(&This->event.IDOMEvent_iface, type, can_bubble, cancelable);
    if(FAILED(hres))
        return hres;

    return VariantCopy(&This->detail, detail);
}

static const IDOMCustomEventVtbl DOMCustomEventVtbl = {
    DOMCustomEvent_QueryInterface,
    DOMCustomEvent_AddRef,
    DOMCustomEvent_Release,
    DOMCustomEvent_GetTypeInfoCount,
    DOMCustomEvent_GetTypeInfo,
    DOMCustomEvent_GetIDsOfNames,
    DOMCustomEvent_Invoke,
    DOMCustomEvent_get_detail,
    DOMCustomEvent_initCustomEvent
};

static DOMCustomEvent *DOMCustomEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMCustomEvent, event);
}

static void *DOMCustomEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMCustomEvent *custom_event = DOMCustomEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMCustomEvent, riid))
        return &custom_event->IDOMCustomEvent_iface;
    return DOMEvent_query_interface(&custom_event->event.dispex, riid);
}

static void DOMCustomEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMCustomEvent *custom_event = DOMCustomEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_traverse(&custom_event->event.dispex, cb);
    traverse_variant(&custom_event->detail, "detail", cb);
}

static void DOMCustomEvent_unlink(DispatchEx *dispex)
{
    DOMCustomEvent *custom_event = DOMCustomEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_unlink(&custom_event->event.dispex);
    unlink_variant(&custom_event->detail);
}

static void DOMCustomEvent_destructor(DispatchEx *dispex)
{
    DOMCustomEvent *custom_event = DOMCustomEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    VariantClear(&custom_event->detail);
    DOMEvent_destructor(dispex);
}

typedef struct {
    DOMEvent event;
    IDOMMessageEvent IDOMMessageEvent_iface;
    IHTMLWindow2 *source;
    BSTR origin;
    VARIANT data;
} DOMMessageEvent;

static inline DOMMessageEvent *impl_from_IDOMMessageEvent(IDOMMessageEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMMessageEvent, IDOMMessageEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMMessageEvent, IDOMMessageEvent, impl_from_IDOMMessageEvent(iface)->event.dispex)

static HRESULT WINAPI DOMMessageEvent_get_data(IDOMMessageEvent *iface, BSTR *p)
{
    DOMMessageEvent *This = impl_from_IDOMMessageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(V_VT(&This->data) == VT_EMPTY) {
        *p = SysAllocString(L"");
        return S_OK;
    }

    if(V_VT(&This->data) != VT_BSTR) {
        FIXME("non-string data\n");
        return E_NOTIMPL;
    }

    return (*p = SysAllocString(V_BSTR(&This->data))) ? S_OK : E_OUTOFMEMORY;
}

static HRESULT DOMMessageEvent_get_data_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    DOMMessageEvent *This = CONTAINING_RECORD(dispex, DOMMessageEvent, event.dispex);

    if(!(flags & DISPATCH_PROPERTYGET) || !res)
        return S_FALSE;

    TRACE("(%p)->(%p)\n", This, res);

    V_VT(res) = VT_EMPTY;
    return VariantCopy(res, &This->data);
}

static HRESULT WINAPI DOMMessageEvent_get_origin(IDOMMessageEvent *iface, BSTR *p)
{
    DOMMessageEvent *This = impl_from_IDOMMessageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->origin)
        return (*p = SysAllocStringLen(This->origin, SysStringLen(This->origin))) ? S_OK : E_OUTOFMEMORY;

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMMessageEvent_get_source(IDOMMessageEvent *iface, IHTMLWindow2 **p)
{
    DOMMessageEvent *This = impl_from_IDOMMessageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if((*p = This->source))
        IHTMLWindow2_AddRef(This->source);
    return S_OK;
}

static HRESULT WINAPI DOMMessageEvent_initMessageEvent(IDOMMessageEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
                                                       VARIANT_BOOL cancelable, BSTR data, BSTR origin,
                                                       BSTR last_event_id, IHTMLWindow2 *source)
{
    DOMMessageEvent *This = impl_from_IDOMMessageEvent(iface);
    BSTR new_origin = NULL;
    BSTR new_data = NULL;
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %s %s %s %p)\n", This, debugstr_w(type), can_bubble, cancelable,
          debugstr_w(data), debugstr_w(origin), debugstr_w(last_event_id), source);

    if(This->event.target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    if((data && !(new_data = SysAllocString(data))) ||
       (origin && !(new_origin = SysAllocString(origin)))) {
        hres = E_OUTOFMEMORY;
        goto fail;
    }

    hres = IDOMEvent_initEvent(&This->event.IDOMEvent_iface, type, can_bubble, cancelable);
    if(FAILED(hres))
        goto fail;

    if(new_data) {
        VariantClear(&This->data);
        V_VT(&This->data) = VT_BSTR;
        V_BSTR(&This->data) = new_data;
    }
    if(new_origin) {
        SysFreeString(This->origin);
        This->origin = new_origin;
    }
    if(This->source)
        IHTMLWindow2_Release(This->source);
    This->source = source;
    if(source)
        IHTMLWindow2_AddRef(source);
    return S_OK;

fail:
    SysFreeString(new_origin);
    SysFreeString(new_data);
    return hres;
}

static const IDOMMessageEventVtbl DOMMessageEventVtbl = {
    DOMMessageEvent_QueryInterface,
    DOMMessageEvent_AddRef,
    DOMMessageEvent_Release,
    DOMMessageEvent_GetTypeInfoCount,
    DOMMessageEvent_GetTypeInfo,
    DOMMessageEvent_GetIDsOfNames,
    DOMMessageEvent_Invoke,
    DOMMessageEvent_get_data,
    DOMMessageEvent_get_origin,
    DOMMessageEvent_get_source,
    DOMMessageEvent_initMessageEvent
};

static DOMMessageEvent *DOMMessageEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMMessageEvent, event);
}

static void *DOMMessageEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMMessageEvent *message_event = DOMMessageEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMMessageEvent, riid))
        return &message_event->IDOMMessageEvent_iface;
    return DOMEvent_query_interface(&message_event->event.dispex, riid);
}

static void DOMMessageEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMMessageEvent *message_event = DOMMessageEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_traverse(&message_event->event.dispex, cb);
    traverse_variant(&message_event->data, "data", cb);
    if(message_event->source)
        note_cc_edge((nsISupports*)message_event->source, "MessageEvent.source", cb);
}

static void DOMMessageEvent_unlink(DispatchEx *dispex)
{
    DOMMessageEvent *message_event = DOMMessageEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_unlink(&message_event->event.dispex);
    unlink_variant(&message_event->data);
    unlink_ref(&message_event->source);
}

static void DOMMessageEvent_destructor(DispatchEx *dispex)
{
    DOMMessageEvent *message_event = DOMMessageEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    SysFreeString(message_event->origin);
    VariantClear(&message_event->data);
    DOMEvent_destructor(dispex);
}

static void DOMMessageEvent_init_dispex_info(dispex_data_t *info, compat_mode_t compat_mode)
{
    static const dispex_hook_t hooks[] = {
        {DISPID_IDOMMESSAGEEVENT_DATA, DOMMessageEvent_get_data_hook},
        {DISPID_UNKNOWN}
    };
    dispex_info_add_interface(info, IDOMMessageEvent_tid, compat_mode >= COMPAT_MODE_IE10 ? hooks : NULL);
}

typedef struct {
    DOMEvent event;
    IDOMProgressEvent IDOMProgressEvent_iface;
    nsIDOMProgressEvent *nsevent;
    BOOL manual_init;
} DOMProgressEvent;

static inline DOMProgressEvent *impl_from_IDOMProgressEvent(IDOMProgressEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMProgressEvent, IDOMProgressEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMProgressEvent, IDOMProgressEvent, impl_from_IDOMProgressEvent(iface)->event.dispex)

static HRESULT WINAPI DOMProgressEvent_get_lengthComputable(IDOMProgressEvent *iface, VARIANT_BOOL *p)
{
    DOMProgressEvent *This = impl_from_IDOMProgressEvent(iface);
    nsresult nsres;
    cpp_bool b;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMProgressEvent_GetLengthComputable(This->nsevent, &b);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    *p = b ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI DOMProgressEvent_get_loaded(IDOMProgressEvent *iface, ULONGLONG *p)
{
    DOMProgressEvent *This = impl_from_IDOMProgressEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return map_nsresult(nsIDOMProgressEvent_GetLoaded(This->nsevent, p));
}

static HRESULT WINAPI DOMProgressEvent_get_total(IDOMProgressEvent *iface, ULONGLONG *p)
{
    DOMProgressEvent *This = impl_from_IDOMProgressEvent(iface);
    cpp_bool b;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->manual_init && (NS_FAILED(nsIDOMProgressEvent_GetLengthComputable(This->nsevent, &b)) || !b)) {
        *p = ~0;
        return S_OK;
    }

    return map_nsresult(nsIDOMProgressEvent_GetTotal(This->nsevent, p));
}

static HRESULT WINAPI DOMProgressEvent_initProgressEvent(IDOMProgressEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
                                                         VARIANT_BOOL cancelable, VARIANT_BOOL lengthComputable,
                                                         ULONGLONG loaded, ULONGLONG total)
{
    DOMProgressEvent *This = impl_from_IDOMProgressEvent(iface);
    nsAString type_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %x %x %x %s %s)\n", This, debugstr_w(type), can_bubble, cancelable, lengthComputable,
          wine_dbgstr_longlong(loaded), wine_dbgstr_longlong(total));

    if(This->event.target) {
        TRACE("called on already dispatched event\n");
        return S_OK;
    }

    hres = IDOMEvent_initEvent(&This->event.IDOMEvent_iface, type, can_bubble, cancelable);
    if(SUCCEEDED(hres)) {
        nsAString_InitDepend(&type_str, type);
        nsres = nsIDOMProgressEvent_InitProgressEvent(This->nsevent, &type_str, !!can_bubble, !!cancelable,
                                                      !!lengthComputable, loaded, total);
        nsAString_Finish(&type_str);
        if(NS_FAILED(nsres))
            return map_nsresult(nsres);
        This->manual_init = TRUE;
    }

    return hres;
}

static const IDOMProgressEventVtbl DOMProgressEventVtbl = {
    DOMProgressEvent_QueryInterface,
    DOMProgressEvent_AddRef,
    DOMProgressEvent_Release,
    DOMProgressEvent_GetTypeInfoCount,
    DOMProgressEvent_GetTypeInfo,
    DOMProgressEvent_GetIDsOfNames,
    DOMProgressEvent_Invoke,
    DOMProgressEvent_get_lengthComputable,
    DOMProgressEvent_get_loaded,
    DOMProgressEvent_get_total,
    DOMProgressEvent_initProgressEvent
};

static DOMProgressEvent *DOMProgressEvent_from_DOMEvent(DOMEvent *event)
{
    return CONTAINING_RECORD(event, DOMProgressEvent, event);
}

static void *DOMProgressEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMProgressEvent *This = DOMProgressEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMProgressEvent, riid))
        return &This->IDOMProgressEvent_iface;
    return DOMEvent_query_interface(&This->event.dispex, riid);
}

static void DOMProgressEvent_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    DOMProgressEvent *This = DOMProgressEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_traverse(&This->event.dispex, cb);
    if(This->nsevent)
        note_cc_edge((nsISupports*)This->nsevent, "ProgressEvent.nsevent", cb);
}

static void DOMProgressEvent_unlink(DispatchEx *dispex)
{
    DOMProgressEvent *This = DOMProgressEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    DOMEvent_unlink(&This->event.dispex);
    unlink_ref(&This->nsevent);
}

static inline DOMStorageEvent *impl_from_IDOMStorageEvent(IDOMStorageEvent *iface)
{
    return CONTAINING_RECORD(iface, DOMStorageEvent, IDOMStorageEvent_iface);
}

DISPEX_IDISPATCH_IMPL(DOMStorageEvent, IDOMStorageEvent, impl_from_IDOMStorageEvent(iface)->event.dispex)

static HRESULT WINAPI DOMStorageEvent_get_key(IDOMStorageEvent *iface, BSTR *p)
{
    DOMStorageEvent *This = impl_from_IDOMStorageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->key)
        return (*p = SysAllocStringLen(This->key, SysStringLen(This->key))) ? S_OK : E_OUTOFMEMORY;
    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMStorageEvent_get_oldValue(IDOMStorageEvent *iface, BSTR *p)
{
    DOMStorageEvent *This = impl_from_IDOMStorageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->old_value)
        return (*p = SysAllocStringLen(This->old_value, SysStringLen(This->old_value))) ? S_OK : E_OUTOFMEMORY;
    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMStorageEvent_get_newValue(IDOMStorageEvent *iface, BSTR *p)
{
    DOMStorageEvent *This = impl_from_IDOMStorageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->new_value)
        return (*p = SysAllocStringLen(This->new_value, SysStringLen(This->new_value))) ? S_OK : E_OUTOFMEMORY;
    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMStorageEvent_get_url(IDOMStorageEvent *iface, BSTR *p)
{
    DOMStorageEvent *This = impl_from_IDOMStorageEvent(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->url)
        return (*p = SysAllocStringLen(This->url, SysStringLen(This->url))) ? S_OK : E_OUTOFMEMORY;
    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI DOMStorageEvent_get_storageArea(IDOMStorageEvent *iface, IHTMLStorage **p)
{
    DOMStorageEvent *This = impl_from_IDOMStorageEvent(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI DOMStorageEvent_initStorageEvent(IDOMStorageEvent *iface, BSTR type, VARIANT_BOOL can_bubble,
                                                       VARIANT_BOOL cancelable, BSTR keyArg, BSTR oldValueArg,
                                                       BSTR newValueArg, BSTR urlArg, IHTMLStorage *storageAreaArg)
{
    DOMStorageEvent *This = impl_from_IDOMStorageEvent(iface);
    FIXME("(%p)->(%s %x %x %s %s %s %s %p)\n", This, debugstr_w(type), can_bubble, cancelable,
          debugstr_w(keyArg), debugstr_w(oldValueArg), debugstr_w(newValueArg), debugstr_w(urlArg), storageAreaArg);
    return E_NOTIMPL;
}

static const IDOMStorageEventVtbl DOMStorageEventVtbl = {
    DOMStorageEvent_QueryInterface,
    DOMStorageEvent_AddRef,
    DOMStorageEvent_Release,
    DOMStorageEvent_GetTypeInfoCount,
    DOMStorageEvent_GetTypeInfo,
    DOMStorageEvent_GetIDsOfNames,
    DOMStorageEvent_Invoke,
    DOMStorageEvent_get_key,
    DOMStorageEvent_get_oldValue,
    DOMStorageEvent_get_newValue,
    DOMStorageEvent_get_url,
    DOMStorageEvent_get_storageArea,
    DOMStorageEvent_initStorageEvent
};

static void *DOMStorageEvent_query_interface(DispatchEx *dispex, REFIID riid)
{
    DOMStorageEvent *storage_event = DOMStorageEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));

    if(IsEqualGUID(&IID_IDOMStorageEvent, riid))
        return &storage_event->IDOMStorageEvent_iface;
    return DOMEvent_query_interface(&storage_event->event.dispex, riid);
}

static void DOMStorageEvent_destructor(DispatchEx *dispex)
{
    DOMStorageEvent *storage_event = DOMStorageEvent_from_DOMEvent(DOMEvent_from_DispatchEx(dispex));
    SysFreeString(storage_event->key);
    SysFreeString(storage_event->old_value);
    SysFreeString(storage_event->new_value);
    SysFreeString(storage_event->url);
    DOMEvent_destructor(dispex);
}

static const dispex_static_data_vtbl_t DOMEvent_dispex_vtbl = {
    .query_interface  = DOMEvent_query_interface,
    .destructor       = DOMEvent_destructor,
    .traverse         = DOMEvent_traverse,
    .unlink           = DOMEvent_unlink
};

static const tid_t Event_iface_tids[] = {
    IDOMEvent_tid,
    0
};

dispex_static_data_t Event_dispex = {
    .id         = PROT_Event,
    .vtbl       = &DOMEvent_dispex_vtbl,
    .disp_tid   = DispDOMEvent_tid,
    .iface_tids = Event_iface_tids,
};

static const dispex_static_data_vtbl_t DOMUIEvent_dispex_vtbl = {
    .query_interface  = DOMUIEvent_query_interface,
    .destructor       = DOMEvent_destructor,
    .traverse         = DOMUIEvent_traverse,
    .unlink           = DOMUIEvent_unlink
};

static const tid_t UIEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMUIEvent_tid,
    0
};

dispex_static_data_t UIEvent_dispex = {
    .id           = PROT_UIEvent,
    .prototype_id = PROT_Event,
    .vtbl         = &DOMUIEvent_dispex_vtbl,
    .disp_tid     = DispDOMUIEvent_tid,
    .iface_tids   = UIEvent_iface_tids,
};

static const dispex_static_data_vtbl_t DOMMouseEvent_dispex_vtbl = {
    .query_interface  = DOMMouseEvent_query_interface,
    .destructor       = DOMEvent_destructor,
    .traverse         = DOMMouseEvent_traverse,
    .unlink           = DOMMouseEvent_unlink
};

static const tid_t MouseEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMUIEvent_tid,
    IDOMMouseEvent_tid,
    0
};

dispex_static_data_t MouseEvent_dispex = {
    .id           = PROT_MouseEvent,
    .prototype_id = PROT_UIEvent,
    .vtbl         = &DOMMouseEvent_dispex_vtbl,
    .disp_tid     = DispDOMMouseEvent_tid,
    .iface_tids   = MouseEvent_iface_tids,
};

static const dispex_static_data_vtbl_t DOMKeyboardEvent_dispex_vtbl = {
    .query_interface  = DOMKeyboardEvent_query_interface,
    .destructor       = DOMEvent_destructor,
    .traverse         = DOMKeyboardEvent_traverse,
    .unlink           = DOMKeyboardEvent_unlink
};

static void KeyboardEvent_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t hooks[] = {
        {DISPID_IDOMKEYBOARDEVENT_IE9_CHAR, NULL, L"char"},
        {DISPID_UNKNOWN}
    };
    dispex_info_add_interface(info, IDOMKeyboardEvent_tid, hooks);
}

static const tid_t KeyboardEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMUIEvent_tid,
    0
};

dispex_static_data_t KeyboardEvent_dispex = {
    .id           = PROT_KeyboardEvent,
    .prototype_id = PROT_UIEvent,
    .vtbl         = &DOMKeyboardEvent_dispex_vtbl,
    .disp_tid     = DispDOMKeyboardEvent_tid,
    .iface_tids   = KeyboardEvent_iface_tids,
    .init_info    = KeyboardEvent_init_dispex_info,
};

static void DOMPageTransitionEvent_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    dispex_info_add_interface(info, IWinePageTransitionEvent_tid, NULL);
}

static const dispex_static_data_vtbl_t DOMPageTransitionEvent_dispex_vtbl = {
    .query_interface  = DOMPageTransitionEvent_query_interface,
    .destructor       = DOMEvent_destructor,
    .traverse         = DOMEvent_traverse,
    .unlink           = DOMEvent_unlink
};

dispex_static_data_t PageTransitionEvent_dispex = {
    .id              = PROT_PageTransitionEvent,
    .prototype_id    = PROT_Event,
    .vtbl            = &DOMPageTransitionEvent_dispex_vtbl,
    .disp_tid        = DispDOMEvent_tid,
    .iface_tids      = Event_iface_tids,
    .init_info       = DOMPageTransitionEvent_init_dispex_info,
    .min_compat_mode = COMPAT_MODE_IE11,
};

static const dispex_static_data_vtbl_t DOMCustomEvent_dispex_vtbl = {
    .query_interface  = DOMCustomEvent_query_interface,
    .destructor       = DOMCustomEvent_destructor,
    .traverse         = DOMCustomEvent_traverse,
    .unlink           = DOMCustomEvent_unlink
};

static const tid_t CustomEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMCustomEvent_tid,
    0
};

dispex_static_data_t CustomEvent_dispex = {
    .id           = PROT_CustomEvent,
    .prototype_id = PROT_Event,
    .vtbl         = &DOMCustomEvent_dispex_vtbl,
    .disp_tid     = DispDOMCustomEvent_tid,
    .iface_tids   = CustomEvent_iface_tids,
};

static const dispex_static_data_vtbl_t DOMMessageEvent_dispex_vtbl = {
    .query_interface  = DOMMessageEvent_query_interface,
    .destructor       = DOMMessageEvent_destructor,
    .traverse         = DOMMessageEvent_traverse,
    .unlink           = DOMMessageEvent_unlink
};

dispex_static_data_t MessageEvent_dispex = {
    .id           = PROT_MessageEvent,
    .prototype_id = PROT_Event,
    .vtbl         = &DOMMessageEvent_dispex_vtbl,
    .disp_tid     = DispDOMMessageEvent_tid,
    .iface_tids   = Event_iface_tids,
    .init_info    = DOMMessageEvent_init_dispex_info,
};

static const dispex_static_data_vtbl_t DOMProgressEvent_dispex_vtbl = {
    .query_interface  = DOMProgressEvent_query_interface,
    .destructor       = DOMEvent_destructor,
    .traverse         = DOMProgressEvent_traverse,
    .unlink           = DOMProgressEvent_unlink
};

static const tid_t ProgressEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMProgressEvent_tid,
    0
};

dispex_static_data_t ProgressEvent_dispex = {
    .id              = PROT_ProgressEvent,
    .prototype_id    = PROT_Event,
    .vtbl            = &DOMProgressEvent_dispex_vtbl,
    .disp_tid        = DispDOMProgressEvent_tid,
    .iface_tids      = ProgressEvent_iface_tids,
    .min_compat_mode = COMPAT_MODE_IE10,
};

static const dispex_static_data_vtbl_t DOMStorageEvent_dispex_vtbl = {
    .query_interface  = DOMStorageEvent_query_interface,
    .destructor       = DOMStorageEvent_destructor,
    .traverse         = DOMEvent_traverse,
    .unlink           = DOMEvent_unlink
};

static const tid_t StorageEvent_iface_tids[] = {
    IDOMEvent_tid,
    IDOMStorageEvent_tid,
    0
};

dispex_static_data_t StorageEvent_dispex = {
    .id           = PROT_StorageEvent,
    .prototype_id = PROT_Event,
    .vtbl         = &DOMStorageEvent_dispex_vtbl,
    .disp_tid     = DispDOMStorageEvent_tid,
    .iface_tids   = StorageEvent_iface_tids,
};

static void *event_ctor(unsigned size, dispex_static_data_t *dispex_data, nsIDOMEvent *nsevent, eventid_t event_id,
                        HTMLInnerWindow *script_global, compat_mode_t compat_mode)
{
    DOMEvent *event = calloc(1, size);

    if(!event)
        return NULL;
    event->IDOMEvent_iface.lpVtbl = &DOMEventVtbl;
    event->event_id = event_id;
    if(event_id != EVENTID_LAST) {
        event->type = wcsdup(event_info[event_id].name);
        if(!event->type) {
            free(event);
            return NULL;
        }
        event->bubbles = (event_info[event_id].flags & EVENT_BUBBLES) != 0;
        event->cancelable = (event_info[event_id].flags & EVENT_CANCELABLE) != 0;
    }
    nsIDOMEvent_AddRef(event->nsevent = nsevent);

    event->time_stamp = get_time_stamp();

    init_dispatch(&event->dispex, dispex_data, script_global, compat_mode);
    return event;
}

static void fill_parent_ui_event(nsIDOMEvent *nsevent, DOMUIEvent *ui_event)
{
    ui_event->IDOMUIEvent_iface.lpVtbl = &DOMUIEventVtbl;
    nsIDOMEvent_QueryInterface(nsevent, &IID_nsIDOMUIEvent, (void**)&ui_event->nsevent);
}

static DOMEvent *generic_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                    compat_mode_t compat_mode)
{
    return event_ctor(sizeof(DOMEvent), &Event_dispex, nsevent, event_id, script_global, compat_mode);
}

static DOMEvent *ui_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                               compat_mode_t compat_mode)
{
    DOMUIEvent *ui_event = event_ctor(sizeof(DOMUIEvent), &UIEvent_dispex, nsevent, event_id, script_global, compat_mode);
    if(!ui_event) return NULL;
    ui_event->IDOMUIEvent_iface.lpVtbl = &DOMUIEventVtbl;
    ui_event->nsevent = iface;
    return &ui_event->event;
}

static DOMEvent *mouse_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                  compat_mode_t compat_mode)
{
    DOMMouseEvent *mouse_event = event_ctor(sizeof(DOMMouseEvent), &MouseEvent_dispex, nsevent, event_id, script_global, compat_mode);
    if(!mouse_event) return NULL;
    mouse_event->IDOMMouseEvent_iface.lpVtbl = &DOMMouseEventVtbl;
    mouse_event->nsevent = iface;
    fill_parent_ui_event(nsevent, &mouse_event->ui_event);
    return &mouse_event->ui_event.event;
}

static DOMEvent *keyboard_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                     compat_mode_t compat_mode)
{
    DOMKeyboardEvent *keyboard_event = event_ctor(sizeof(DOMKeyboardEvent), &KeyboardEvent_dispex, nsevent, event_id, script_global,
                                                  compat_mode);
    if(!keyboard_event) return NULL;
    keyboard_event->IDOMKeyboardEvent_iface.lpVtbl = &DOMKeyboardEventVtbl;
    keyboard_event->nsevent = iface;
    fill_parent_ui_event(nsevent, &keyboard_event->ui_event);
    return &keyboard_event->ui_event.event;
}

static DOMEvent *page_transition_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                            compat_mode_t compat_mode)
{
    DOMPageTransitionEvent *page_transition_event = event_ctor(sizeof(DOMCustomEvent), &PageTransitionEvent_dispex, nsevent, event_id,
                                                               script_global, compat_mode);
    if(!page_transition_event) return NULL;
    page_transition_event->IWinePageTransitionEvent_iface.lpVtbl = &DOMPageTransitionEventVtbl;
    return &page_transition_event->event;
}

static DOMEvent *custom_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                   compat_mode_t compat_mode)
{
    DOMCustomEvent *custom_event = event_ctor(sizeof(DOMCustomEvent), &CustomEvent_dispex, nsevent, event_id, script_global,
                                              compat_mode);
    if(!custom_event) return NULL;
    custom_event->IDOMCustomEvent_iface.lpVtbl = &DOMCustomEventVtbl;
    nsIDOMCustomEvent_Release(iface);
    return &custom_event->event;
}

static DOMEvent *progress_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                     compat_mode_t compat_mode)
{
    DOMProgressEvent *progress_event;

    if(!(progress_event = event_ctor(sizeof(DOMProgressEvent), &ProgressEvent_dispex, nsevent, event_id, script_global, compat_mode)))
        return NULL;
    progress_event->IDOMProgressEvent_iface.lpVtbl = &DOMProgressEventVtbl;
    progress_event->nsevent = iface;
    return &progress_event->event;
}

static DOMEvent *message_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                    compat_mode_t compat_mode)
{
    DOMMessageEvent *message_event = event_ctor(sizeof(DOMMessageEvent), &MessageEvent_dispex, nsevent, event_id, script_global,
                                                compat_mode);
    if(!message_event) return NULL;
    message_event->IDOMMessageEvent_iface.lpVtbl = &DOMMessageEventVtbl;
    return &message_event->event;
}

static DOMEvent *storage_event_ctor(void *iface, nsIDOMEvent *nsevent, eventid_t event_id, HTMLInnerWindow *script_global,
                                    compat_mode_t compat_mode)
{
    DOMStorageEvent *storage_event = event_ctor(sizeof(DOMStorageEvent), &StorageEvent_dispex, nsevent, event_id, script_global,
                                                compat_mode);
    if(!storage_event) return NULL;
    storage_event->IDOMStorageEvent_iface.lpVtbl = &DOMStorageEventVtbl;
    return &storage_event->event;
}

static const struct {
    REFIID iid;
    DOMEvent *(*ctor)(void *iface, nsIDOMEvent *nsevent, eventid_t, HTMLInnerWindow *sceript_global, compat_mode_t compat_mode);
    compat_mode_t min_compat_mode;
} event_types_ctor_table[] = {
    [EVENT_TYPE_EVENT]          = { NULL,                         generic_event_ctor },
    [EVENT_TYPE_UIEVENT]        = { &IID_nsIDOMUIEvent,           ui_event_ctor },
    [EVENT_TYPE_MOUSE]          = { &IID_nsIDOMMouseEvent,        mouse_event_ctor },
    [EVENT_TYPE_KEYBOARD]       = { &IID_nsIDOMKeyEvent,          keyboard_event_ctor },
    [EVENT_TYPE_CLIPBOARD]      = { NULL,                         generic_event_ctor },
    [EVENT_TYPE_FOCUS]          = { NULL,                         generic_event_ctor },
    [EVENT_TYPE_DRAG]           = { NULL,                         generic_event_ctor },
    [EVENT_TYPE_PAGETRANSITION] = { NULL,                         page_transition_event_ctor, COMPAT_MODE_IE11 },
    [EVENT_TYPE_CUSTOM]         = { &IID_nsIDOMCustomEvent,       custom_event_ctor },
    [EVENT_TYPE_PROGRESS]       = { &IID_nsIDOMProgressEvent,     progress_event_ctor, COMPAT_MODE_IE10 },
    [EVENT_TYPE_MESSAGE]        = { NULL,                         message_event_ctor },
    [EVENT_TYPE_STORAGE]        = { NULL,                         storage_event_ctor },
};

static DOMEvent *alloc_event(nsIDOMEvent *nsevent, HTMLInnerWindow *script_global, compat_mode_t compat_mode, event_type_t event_type,
        eventid_t event_id)
{
    void *iface = NULL;
    DOMEvent *event;

    if(compat_mode < event_types_ctor_table[event_type].min_compat_mode)
        event_type = EVENT_TYPE_EVENT;

    if(event_types_ctor_table[event_type].iid)
        nsIDOMEvent_QueryInterface(nsevent, event_types_ctor_table[event_type].iid, &iface);

    /* Transfer the iface ownership to the ctor on success */
    if(!(event = event_types_ctor_table[event_type].ctor(iface, nsevent, event_id, script_global, compat_mode)) && iface)
        nsISupports_Release(iface);
    return event;
}

HRESULT create_event_from_nsevent(nsIDOMEvent *nsevent, HTMLInnerWindow *script_global, compat_mode_t compat_mode, DOMEvent **ret_event)
{
    event_type_t event_type = EVENT_TYPE_EVENT;
    eventid_t event_id = EVENTID_LAST;
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;
    unsigned i;

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMEvent_GetType(nsevent, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const WCHAR *type;
        nsAString_GetData(&nsstr, &type);
        event_id = str_to_eid(type);
        if(event_id == EVENTID_LAST)
            FIXME("unknown event type %s\n", debugstr_w(type));
    }else {
        ERR("GetType failed: %08lx\n", nsres);
    }
    nsAString_Finish(&nsstr);

    for(i = 0; i < ARRAY_SIZE(event_types_ctor_table); i++) {
        void *iface;
        if(event_types_ctor_table[i].iid &&
           nsIDOMEvent_QueryInterface(nsevent, event_types_ctor_table[i].iid, &iface) == NS_OK) {
            nsISupports_Release(iface);
            event_type = i;
            break;
        }
    }

    event = alloc_event(nsevent, script_global, compat_mode, event_type, event_id);
    if(!event)
        return E_OUTOFMEMORY;

    event->trusted = TRUE;
    *ret_event = event;
    return S_OK;
}

HRESULT create_document_event_str(HTMLDocumentNode *doc, const WCHAR *type, IDOMEvent **ret_event)
{
    event_type_t event_type = EVENT_TYPE_EVENT;
    nsIDOMEvent *nsevent;
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;
    unsigned i;

    nsAString_InitDepend(&nsstr, type);
    nsres = nsIDOMDocument_CreateEvent(doc->dom_document, &nsstr, &nsevent);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        FIXME("CreateEvent(%s) failed: %08lx\n", debugstr_w(type), nsres);
        return E_FAIL;
    }

    for(i = 0; i < ARRAY_SIZE(event_types); i++) {
        if(!wcsicmp(type, event_types[i])) {
            event_type = i;
            break;
        }
    }

    event = alloc_event(nsevent, doc->script_global, dispex_compat_mode(&doc->node.event_target.dispex), event_type, EVENTID_LAST);
    nsIDOMEvent_Release(nsevent);
    if(!event)
        return E_OUTOFMEMORY;

    *ret_event = &event->IDOMEvent_iface;
    return S_OK;
}

HRESULT create_document_event(HTMLDocumentNode *doc, eventid_t event_id, DOMEvent **ret_event)
{
    nsIDOMEvent *nsevent;
    DOMEvent *event;
    nsAString nsstr;
    nsresult nsres;

    nsAString_InitDepend(&nsstr, event_types[event_info[event_id].type]);
    nsres = nsIDOMDocument_CreateEvent(doc->dom_document, &nsstr, &nsevent);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        FIXME("CreateEvent(%s) failed: %08lx\n", debugstr_w(event_types[event_info[event_id].type]), nsres);
        return E_FAIL;
    }

    event = alloc_event(nsevent, doc->script_global, doc->document_mode, event_info[event_id].type, event_id);
    nsIDOMEvent_Release(nsevent);
    if(!event)
        return E_OUTOFMEMORY;

    event->event_id = event_id;
    event->trusted = TRUE;
    *ret_event = event;
    return S_OK;
}

HRESULT create_message_event(HTMLDocumentNode *doc, IHTMLWindow2 *source, VARIANT *data, DOMEvent **ret)
{
    URL_COMPONENTSW url = { sizeof(url) };
    DOMMessageEvent *message_event;
    IHTMLLocation *location;
    DOMEvent *event;
    HRESULT hres;
    BSTR origin;

    hres = IHTMLWindow2_get_location(source, &location);
    if(FAILED(hres))
        return hres;

    hres = IHTMLLocation_get_href(location, &origin);
    IHTMLLocation_Release(location);
    if(FAILED(hres))
        return hres;

    url.dwUrlPathLength = 1;
    if(origin && InternetCrackUrlW(origin, 0, 0, &url)) {
        BSTR tmp = SysAllocStringLen(origin, url.lpszUrlPath - origin);
        SysFreeString(origin);
        if(!tmp)
            return E_OUTOFMEMORY;
        origin = tmp;
    }

    hres = create_document_event(doc, EVENTID_MESSAGE, &event);
    if(FAILED(hres)) {
        SysFreeString(origin);
        return hres;
    }
    message_event = DOMMessageEvent_from_DOMEvent(event);
    message_event->origin = origin;

    V_VT(&message_event->data) = VT_EMPTY;
    hres = VariantCopy(&message_event->data, data);
    if(FAILED(hres)) {
        IDOMEvent_Release(&event->IDOMEvent_iface);
        return hres;
    }

    message_event->source = source;
    IHTMLWindow2_AddRef(message_event->source);

    *ret = event;
    return S_OK;
}

HRESULT create_storage_event(HTMLDocumentNode *doc, BSTR key, BSTR old_value, BSTR new_value,
        const WCHAR *url, BOOL commit, DOMEvent **ret)
{
    DOMStorageEvent *storage_event;
    DOMEvent *event;
    HRESULT hres;

    hres = create_document_event(doc, commit ? EVENTID_STORAGECOMMIT : EVENTID_STORAGE, &event);
    if(FAILED(hres))
        return hres;
    storage_event = DOMStorageEvent_from_DOMEvent(event);

    if(!commit) {
        if((key       && !(storage_event->key =       SysAllocString(key))) ||
           (old_value && !(storage_event->old_value = SysAllocString(old_value))) ||
           (new_value && !(storage_event->new_value = SysAllocString(new_value)))) {
            IDOMEvent_Release(&event->IDOMEvent_iface);
            return E_OUTOFMEMORY;
        }
    }

    if(url && !(storage_event->url = SysAllocString(url))) {
        IDOMEvent_Release(&event->IDOMEvent_iface);
        return E_OUTOFMEMORY;
    }

    *ret = event;
    return S_OK;
}

HRESULT call_disp_func(IDispatch *disp, DISPPARAMS *dp, VARIANT *retv)
{
    IDispatchEx *dispex;
    EXCEPINFO ei;
    HRESULT hres;

    memset(&ei, 0, sizeof(ei));

    hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, 0, GetUserDefaultLCID(), DISPATCH_METHOD, dp, retv, &ei, NULL);
        IDispatchEx_Release(dispex);
    }else {
        TRACE("Could not get IDispatchEx interface: %08lx\n", hres);
        hres = IDispatch_Invoke(disp, 0, &IID_NULL, GetUserDefaultLCID(), DISPATCH_METHOD,
                dp, retv, &ei, NULL);
    }

    return hres;
}

static HRESULT call_cp_func(IDispatch *disp, DISPID dispid, IHTMLEventObj *event_obj, VARIANT *retv)
{
    DISPPARAMS dp = {NULL,NULL,0,0};
    VARIANT event_arg;
    UINT argerr;
    EXCEPINFO ei;

    TRACE("%p,%ld,%p,%p\n", disp, dispid, event_obj, retv);

    if(event_obj) {
        V_VT(&event_arg) = VT_DISPATCH;
        V_DISPATCH(&event_arg) = (IDispatch*)event_obj;
        dp.rgvarg = &event_arg;
        dp.cArgs = 1;
    }

    memset(&ei, 0, sizeof(ei));
    return IDispatch_Invoke(disp, dispid, &IID_NULL, 0, DISPATCH_METHOD, &dp, retv, &ei, &argerr);
}

static BOOL use_event_quirks(EventTarget *event_target)
{
    return dispex_compat_mode(&event_target->dispex) < COMPAT_MODE_IE9;
}

static BOOL is_cp_event(cp_static_data_t *data, DISPID dispid)
{
    int min, max, i;
    HRESULT hres;

    if(!data || dispid == DISPID_UNKNOWN)
        return FALSE;

    if(!data->ids) {
        hres = get_dispids(data->tid, &data->id_cnt, &data->ids);
        if(FAILED(hres))
            return FALSE;
    }

    min = 0;
    max = data->id_cnt-1;
    while(min <= max) {
        i = (min+max)/2;
        if(data->ids[i] == dispid)
            return TRUE;

        if(data->ids[i] < dispid)
            min = i+1;
        else
            max = i-1;
    }

    return FALSE;
}

static void call_event_handlers(EventTarget *event_target, DOMEvent *event, dispatch_mode_t dispatch_mode)
{
    const listener_container_t *container = get_listener_container(event_target, event->type, FALSE);
    const event_target_vtbl_t *vtbl = dispex_get_vtbl(&event_target->dispex);
    event_listener_t *listener, listeners_buf[8], *listeners = listeners_buf;
    unsigned listeners_cnt, listeners_size;
    ConnectionPointContainer *cp_container = NULL;
    BOOL skip_onevent_listener = FALSE;
    VARIANT v;
    HRESULT hres;

    assert(!event->current_target);
    event->current_target = event_target;

    if(container && !list_empty(&container->listeners) && event->phase != DEP_CAPTURING_PHASE) {
        listener = LIST_ENTRY(list_tail(&container->listeners), event_listener_t, entry);
        if(listener && listener->function && listener->type == LISTENER_TYPE_ONEVENT
                && use_event_quirks(event_target)) {
            DISPID named_arg = DISPID_THIS;
            VARIANTARG arg;
            DISPPARAMS dp = {&arg, &named_arg, 1, 1};

            skip_onevent_listener = TRUE;

            V_VT(&arg) = VT_DISPATCH;
            V_DISPATCH(&arg) = (IDispatch*)dispex_outer_iface(&event_target->dispex);
            V_VT(&v) = VT_EMPTY;

            TRACE("%p %s >>>\n", event_target, debugstr_w(event->type));
            hres = call_disp_func(listener->function, &dp, &v);
            IDispatch_Release(V_DISPATCH(&arg));

            if(hres == S_OK) {
                TRACE("%p %s <<< %s\n", event_target, debugstr_w(event->type), debugstr_variant(&v));

                if(event->cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%p %s <<< %08lx\n", event_target, debugstr_w(event->type), hres);
            }
        }
    }

    listeners_cnt = 0;
    listeners_size = ARRAY_SIZE(listeners_buf);

    if(container) {
        LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
            if(!listener->function)
                continue;
            switch(listener->type) {
            case LISTENER_TYPE_ONEVENT:
                if(skip_onevent_listener || event->phase == DEP_CAPTURING_PHASE)
                    continue;
                break;
            case LISTENER_TYPE_CAPTURE:
                if(event->phase == DEP_BUBBLING_PHASE || dispatch_mode == DISPATCH_LEGACY)
                    continue;
                break;
            case LISTENER_TYPE_BUBBLE:
                if(event->phase == DEP_CAPTURING_PHASE || dispatch_mode == DISPATCH_LEGACY)
                    continue;
                break;
            case LISTENER_TYPE_ATTACHED:
                if(event->phase == DEP_CAPTURING_PHASE || dispatch_mode == DISPATCH_STANDARD)
                    continue;
                break;
            }

            if(listeners_cnt == listeners_size) {
                event_listener_t *new_listeners;
                if(listeners == listeners_buf) {
                    new_listeners = malloc(listeners_size * 2 * sizeof(*new_listeners));
                    if(!new_listeners)
                        break;
                    memcpy(new_listeners, listeners, listeners_cnt * sizeof(*listeners));
                }else {
                    new_listeners = realloc(listeners, listeners_size * 2 * sizeof(*new_listeners));
                }
                listeners = new_listeners;
                listeners_size *= 2;
            }

            listeners[listeners_cnt].type = listener->type;
            IDispatch_AddRef(listeners[listeners_cnt].function = listener->function);
            listeners_cnt++;
        }
    }

    for(listener = listeners; listener < listeners + listeners_cnt; listener++) {
        if(listener->type != LISTENER_TYPE_ATTACHED) {
            DISPID named_arg = DISPID_THIS;
            VARIANTARG args[2];
            DISPPARAMS dp = {args, &named_arg, 2, 1};

            V_VT(args) = VT_DISPATCH;
            V_DISPATCH(args) = (IDispatch *)dispex_outer_iface(&event_target->dispex);

            V_VT(args+1) = VT_DISPATCH;
            V_DISPATCH(args+1) = dispatch_mode == DISPATCH_LEGACY
                ? (IDispatch*)event->event_obj : (IDispatch*)&event->IDOMEvent_iface;
            V_VT(&v) = VT_EMPTY;

            TRACE("%p %s >>>\n", event_target, debugstr_w(event->type));
            hres = call_disp_func(listener->function, &dp, &v);
            IDispatch_Release(V_DISPATCH(args));

            if(hres == S_OK) {
                TRACE("%p %s <<< %s\n", event_target, debugstr_w(event->type),
                      debugstr_variant(&v));

                if(event->cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%p %s <<< %08lx\n", event_target, debugstr_w(event->type), hres);
            }
        }else {
            VARIANTARG arg;
            DISPPARAMS dp = {&arg, NULL, 1, 0};

            V_VT(&arg) = VT_DISPATCH;
            V_DISPATCH(&arg) = (IDispatch*)event->event_obj;
            V_VT(&v) = VT_EMPTY;

            TRACE("%p %s attached >>>\n", event_target, debugstr_w(event->type));
            hres = call_disp_func(listener->function, &dp, &v);
            if(hres == S_OK) {
                TRACE("%p %s attached <<<\n", event_target, debugstr_w(event->type));

                if(event->cancelable) {
                    if(V_VT(&v) == VT_BOOL) {
                        if(!V_BOOL(&v))
                            IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                    }else if(V_VT(&v) != VT_EMPTY) {
                        FIXME("unhandled result %s\n", debugstr_variant(&v));
                    }
                }
                VariantClear(&v);
            }else {
                WARN("%p %s attached <<< %08lx\n", event_target, debugstr_w(event->type), hres);
            }
        }

        if(event->stop_immediate_propagation)
            break;
    }

    for(listener = listeners; listener < listeners + listeners_cnt; listener++)
        IDispatch_Release(listener->function);
    if(listeners != listeners_buf)
        free(listeners);

    if(event->phase != DEP_CAPTURING_PHASE && event_info[event->event_id].dispid && vtbl->get_cp_container)
        cp_container = vtbl->get_cp_container(&event_target->dispex);
    if(cp_container) {
        if(cp_container->cps) {
            ConnectionPoint *cp;
            unsigned i, j;

            for(j=0; cp_container->cp_entries[j].riid; j++) {
                cp = cp_container->cps + j;
                if(!cp->sinks_size || !is_cp_event(cp->data, event_info[event->event_id].dispid))
                    continue;

                for(i=0; i < cp->sinks_size; i++) {
                    if(!cp->sinks[i].disp)
                        continue;

                    V_VT(&v) = VT_EMPTY;

                    TRACE("%p cp %s [%u] >>>\n", event_target, debugstr_w(event->type), i);
                    hres = call_cp_func(cp->sinks[i].disp, event_info[event->event_id].dispid,
                            cp->data->pass_event_arg ? event->event_obj : NULL, &v);
                    if(hres == S_OK) {
                        TRACE("%p cp %s [%u] <<<\n", event_target, debugstr_w(event->type), i);

                        if(event->cancelable) {
                            if(V_VT(&v) == VT_BOOL) {
                                if(!V_BOOL(&v))
                                    IDOMEvent_preventDefault(&event->IDOMEvent_iface);
                            }else if(V_VT(&v) != VT_EMPTY) {
                                FIXME("unhandled result %s\n", debugstr_variant(&v));
                            }
                        }
                        VariantClear(&v);
                    }else {
                        WARN("%p cp %s [%u] <<< %08lx\n", event_target, debugstr_w(event->type), i, hres);
                    }
                }
            }
        }
        IConnectionPointContainer_Release(&cp_container->IConnectionPointContainer_iface);
    }

    event->current_target = NULL;
}

static HRESULT dispatch_event_object(EventTarget *event_target, DOMEvent *event,
                                     dispatch_mode_t dispatch_mode, VARIANT_BOOL *r)
{
    EventTarget *target_chain_buf[8], **target_chain = target_chain_buf;
    unsigned chain_cnt, chain_buf_size, i;
    const event_target_vtbl_t *vtbl, *target_vtbl;
    HTMLEventObj *event_obj_ref = NULL;
    IHTMLEventObj *prev_event = NULL;
    EventTarget *iter;
    HRESULT hres;

    TRACE("(%p) %s\n", event_target, debugstr_w(event->type));

    if(!event->type) {
        FIXME("Uninitialized event.\n");
        return E_FAIL;
    }

    if(event->current_target) {
        FIXME("event is being dispatched.\n");
        return E_FAIL;
    }

    iter = event_target;
    IEventTarget_AddRef(&event_target->IEventTarget_iface);

    chain_cnt = 0;
    chain_buf_size = ARRAY_SIZE(target_chain_buf);

    do {
        if(chain_cnt == chain_buf_size) {
            EventTarget **new_chain;
            if(target_chain == target_chain_buf) {
                new_chain = malloc(chain_buf_size * 2 * sizeof(*new_chain));
                if(!new_chain)
                    break;
                memcpy(new_chain, target_chain, chain_buf_size * sizeof(*new_chain));
            }else {
                new_chain = realloc(target_chain, chain_buf_size * 2 * sizeof(*new_chain));
                if(!new_chain)
                    break;
            }
            chain_buf_size *= 2;
            target_chain = new_chain;
        }

        target_chain[chain_cnt++] = iter;

        if(!(vtbl = dispex_get_vtbl(&iter->dispex))->get_parent_event_target)
            break;
        iter = vtbl->get_parent_event_target(&iter->dispex);
    } while(iter);

    if(!event->event_obj && !event->no_event_obj) {
        event_obj_ref = alloc_event_obj(event, NULL);
        if(event_obj_ref)
            event->event_obj = &event_obj_ref->IHTMLEventObj_iface;
    }

    target_vtbl = dispex_get_vtbl(&event_target->dispex);
    if(target_vtbl->set_current_event)
        prev_event = target_vtbl->set_current_event(&event_target->dispex, event->event_obj);

    if(event->target)
        IEventTarget_Release(&event->target->IEventTarget_iface);
    event->target = event_target;
    IEventTarget_AddRef(&event_target->IEventTarget_iface);

    event->phase = DEP_CAPTURING_PHASE;

    if(event_info[event->event_id].flags & EVENT_HASDEFAULTHANDLERS) {
        for(i = 0; i < chain_cnt; i++) {
            vtbl = dispex_get_vtbl(&target_chain[i]->dispex);
            if(!vtbl->pre_handle_event)
                continue;
            hres = vtbl->pre_handle_event(&target_chain[i]->dispex, event);
            if(FAILED(hres) || event->stop_propagation)
                break;
        }
    }

    i = chain_cnt-1;
    while(!event->stop_propagation && i)
        call_event_handlers(target_chain[i--], event, dispatch_mode);

    if(!event->stop_propagation) {
        event->phase = DEP_AT_TARGET;
        call_event_handlers(target_chain[0], event, dispatch_mode);
    }

    if(event->bubbles) {
        event->phase = DEP_BUBBLING_PHASE;
        for(i = 1; !event->stop_propagation && i < chain_cnt; i++)
            call_event_handlers(target_chain[i], event, dispatch_mode);
    }

    if(event->stop_propagation)
        nsIDOMEvent_StopPropagation(event->nsevent);

    if(r)
        *r = variant_bool(!event->prevent_default);

    if(target_vtbl->set_current_event) {
        IHTMLEventObj *prev = target_vtbl->set_current_event(&event_target->dispex, prev_event);
        if(prev)
            IHTMLEventObj_Release(prev);
    }

    if(prev_event)
        IHTMLEventObj_Release(prev_event);

    if(event_info[event->event_id].flags & EVENT_HASDEFAULTHANDLERS) {
        BOOL prevent_default = event->prevent_default;
        for(i = 0; !prevent_default && i < chain_cnt; i++) {
            vtbl = dispex_get_vtbl(&target_chain[i]->dispex);
            if(!vtbl->handle_event)
                continue;
            hres = vtbl->handle_event(&target_chain[i]->dispex, event, &prevent_default);
            if(FAILED(hres) || event->stop_propagation)
                break;
            if(prevent_default)
                nsIDOMEvent_PreventDefault(event->nsevent);
        }
    }

    event->prevent_default = FALSE;
    if(event_obj_ref) {
        event->event_obj = NULL;
        IHTMLEventObj_Release(&event_obj_ref->IHTMLEventObj_iface);
    }

    for(i = 0; i < chain_cnt; i++)
        IEventTarget_Release(&target_chain[i]->IEventTarget_iface);
    if(target_chain != target_chain_buf)
        free(target_chain);

    return S_OK;
}

void dispatch_event(EventTarget *event_target, DOMEvent *event)
{
    dispatch_event_object(event_target, event, DISPATCH_BOTH, NULL);

    /*
     * We may have registered multiple Gecko listeners for the same event type,
     * but we already dispatched event to all relevant targets. Stop event
     * propagation here to avoid events being dispatched multiple times.
     */
    if(event_info[event->event_id].flags & EVENT_BIND_TO_TARGET)
        nsIDOMEvent_StopPropagation(event->nsevent);
}

HRESULT fire_event(HTMLDOMNode *node, const WCHAR *event_name, VARIANT *event_var, VARIANT_BOOL *cancelled)
{
    HTMLEventObj *event_obj = NULL;
    eventid_t eid;
    HRESULT hres = S_OK;

    eid = attr_to_eid(event_name);
    if(eid == EVENTID_LAST) {
        WARN("unknown event %s\n", debugstr_w(event_name));
        return E_INVALIDARG;
    }

    if(event_var && V_VT(event_var) != VT_EMPTY && V_VT(event_var) != VT_ERROR) {
        if(V_VT(event_var) != VT_DISPATCH) {
            FIXME("event_var %s not supported\n", debugstr_variant(event_var));
            return E_NOTIMPL;
        }

        if(V_DISPATCH(event_var)) {
            IHTMLEventObj *event_iface;

            hres = IDispatch_QueryInterface(V_DISPATCH(event_var), &IID_IHTMLEventObj, (void**)&event_iface);
            if(FAILED(hres)) {
                FIXME("No IHTMLEventObj iface\n");
                return hres;
            }

            event_obj = unsafe_impl_from_IHTMLEventObj(event_iface);
            if(!event_obj) {
                ERR("Not our IHTMLEventObj?\n");
                IHTMLEventObj_Release(event_iface);
                return E_FAIL;
            }
        }
    }

    if(!event_obj) {
        event_obj = alloc_event_obj(NULL, node->doc->script_global);
        if(!event_obj)
            return E_OUTOFMEMORY;
    }

    if(!event_obj->event)
        hres = create_document_event(node->doc, eid, &event_obj->event);

    if(SUCCEEDED(hres)) {
        event_obj->event->event_obj = &event_obj->IHTMLEventObj_iface;
        dispatch_event_object(&node->event_target, event_obj->event, DISPATCH_LEGACY, NULL);
        event_obj->event->event_obj = NULL;
    }

    IHTMLEventObj_Release(&event_obj->IHTMLEventObj_iface);
    if(FAILED(hres))
        return hres;

    *cancelled = VARIANT_TRUE; /* FIXME */
    return S_OK;
}

HRESULT ensure_doc_nsevent_handler(HTMLDocumentNode *doc, nsIDOMNode *nsnode, eventid_t eid)
{
    TRACE("%s\n", debugstr_w(event_info[eid].name));

    if(!doc->dom_document)
        return S_OK;

    switch(eid) {
    case EVENTID_FOCUSIN:
        doc->event_vector[eid] = TRUE;
        eid = EVENTID_FOCUS;
        break;
    case EVENTID_FOCUSOUT:
        doc->event_vector[eid] = TRUE;
        eid = EVENTID_BLUR;
        break;
    default:
        break;
    }

    if(event_info[eid].flags & EVENT_DEFAULTLISTENER) {
        nsnode = NULL;
    }else if(!(event_info[eid].flags & EVENT_BIND_TO_TARGET)) {
        return S_OK;
    }

    if(!nsnode || nsnode == doc->node.nsnode) {
        if(doc->event_vector[eid])
            return S_OK;
        doc->event_vector[eid] = TRUE;
    }

    add_nsevent_listener(doc, nsnode, event_info[eid].name);
    return S_OK;
}

void detach_events(HTMLDocumentNode *doc)
{
    if(doc->event_vector) {
        int i;

        for(i=0; i < EVENTID_LAST; i++) {
            if(doc->event_vector[i]) {
                detach_nsevent(doc, event_info[i].name);
                doc->event_vector[i] = FALSE;
            }
        }
    }

    release_nsevents(doc);
}

static HRESULT get_event_dispex_ref(EventTarget *event_target, eventid_t eid, BOOL alloc, VARIANT **ret)
{
    WCHAR buf[64];
    buf[0] = 'o';
    buf[1] = 'n';
    lstrcpyW(buf+2, event_info[eid].name);
    return dispex_get_dprop_ref(&event_target->dispex, buf, alloc, ret);
}

static event_listener_t *get_onevent_listener(EventTarget *event_target, eventid_t eid, BOOL alloc)
{
    listener_container_t *container;
    event_listener_t *listener;

    container = get_listener_container(event_target, event_info[eid].name, alloc);
    if(!container)
        return NULL;

    LIST_FOR_EACH_ENTRY_REV(listener, &container->listeners, event_listener_t, entry) {
        if(listener->type == LISTENER_TYPE_ONEVENT)
            return listener;
    }

    if(!alloc)
        return NULL;

    listener = malloc(sizeof(*listener));
    if(!listener)
        return NULL;

    listener->type = LISTENER_TYPE_ONEVENT;
    listener->function = NULL;
    list_add_tail(&container->listeners, &listener->entry);
    return listener;
}

static void remove_event_handler(EventTarget *event_target, eventid_t eid)
{
    event_listener_t *listener;
    VARIANT *store;
    HRESULT hres;

    hres = get_event_dispex_ref(event_target, eid, FALSE, &store);
    if(SUCCEEDED(hres))
        VariantClear(store);

    listener = get_onevent_listener(event_target, eid, FALSE);
    if(listener && listener->function) {
        IDispatch_Release(listener->function);
        listener->function = NULL;
    }
}

static HRESULT set_event_handler_disp(EventTarget *event_target, eventid_t eid, IDispatch *disp)
{
    event_listener_t *listener;

    if(event_info[eid].flags & EVENT_FIXME)
        FIXME("unimplemented event %s\n", debugstr_w(event_info[eid].name));

    remove_event_handler(event_target, eid);
    if(!disp)
        return S_OK;

    listener = get_onevent_listener(event_target, eid, TRUE);
    if(!listener)
        return E_OUTOFMEMORY;

    if(listener->function)
        IDispatch_Release(listener->function);

    IDispatch_AddRef(listener->function = disp);
    return S_OK;
}

HRESULT set_event_handler(EventTarget *event_target, eventid_t eid, VARIANT *var)
{
    switch(V_VT(var)) {
    case VT_EMPTY:
        if(use_event_quirks(event_target)) {
            WARN("attempt to set to VT_EMPTY in quirks mode\n");
            return E_NOTIMPL;
        }
        /* fall through */
    case VT_NULL:
        remove_event_handler(event_target, eid);
        return S_OK;

    case VT_DISPATCH:
        return set_event_handler_disp(event_target, eid, V_DISPATCH(var));

    case VT_BSTR: {
        VARIANT *v;
        HRESULT hres;

        if(!use_event_quirks(event_target))
            FIXME("Setting to string %s not supported\n", debugstr_w(V_BSTR(var)));

        /*
         * Setting event handler to string is a rare case and we don't want to
         * complicate nor increase memory of listener_container_t for that. Instead,
         * we store the value in DispatchEx, which can already handle custom
         * properties.
         */
        remove_event_handler(event_target, eid);

        hres = get_event_dispex_ref(event_target, eid, TRUE, &v);
        if(FAILED(hres))
            return hres;

        V_BSTR(v) = SysAllocString(V_BSTR(var));
        if(!V_BSTR(v))
            return E_OUTOFMEMORY;
        V_VT(v) = VT_BSTR;
        return S_OK;
    }

    default:
        FIXME("not handler %s\n", debugstr_variant(var));
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT get_event_handler(EventTarget *event_target, eventid_t eid, VARIANT *var)
{
    event_listener_t *listener;
    VARIANT *v;
    HRESULT hres;

    hres = get_event_dispex_ref(event_target, eid, FALSE, &v);
    if(SUCCEEDED(hres) && V_VT(v) != VT_EMPTY) {
        V_VT(var) = VT_EMPTY;
        return VariantCopy(var, v);
    }

    listener = get_onevent_listener(event_target, eid, FALSE);
    if(listener && listener->function) {
        V_VT(var) = VT_DISPATCH;
        V_DISPATCH(var) = listener->function;
        IDispatch_AddRef(V_DISPATCH(var));
    }else {
        V_VT(var) = VT_NULL;
    }

    return S_OK;
}

HRESULT attach_event(EventTarget *event_target, BSTR name, IDispatch *disp, VARIANT_BOOL *res)
{
    listener_container_t *container;
    event_listener_t *listener;
    eventid_t eid;

    if(!disp) {
        *res = VARIANT_FALSE;
        return S_OK;
    }

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        *res = VARIANT_TRUE;
        return S_OK;
    }

    container = get_listener_container(event_target, event_info[eid].name, TRUE);
    if(!container)
        return E_OUTOFMEMORY;

    listener = malloc(sizeof(*listener));
    if(!listener)
        return E_OUTOFMEMORY;

    listener->type = LISTENER_TYPE_ATTACHED;
    IDispatch_AddRef(listener->function = disp);
    if(use_event_quirks(event_target))
        list_add_head(&container->listeners, &listener->entry);
    else
        list_add_tail(&container->listeners, &listener->entry);

    *res = VARIANT_TRUE;
    return S_OK;
}

HRESULT detach_event(EventTarget *event_target, BSTR name, IDispatch *disp)
{
    eventid_t eid;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        return S_OK;
    }

    remove_event_listener(event_target, event_info[eid].name, LISTENER_TYPE_ATTACHED, disp);
    return S_OK;
}

void bind_target_event(HTMLDocumentNode *doc, EventTarget *event_target, const WCHAR *event, IDispatch *disp)
{
    eventid_t eid;

    TRACE("(%p %p %s %p)\n", doc, event_target, debugstr_w(event), disp);

    eid = attr_to_eid(event);
    if(eid == EVENTID_LAST) {
        WARN("Unsupported event %s\n", debugstr_w(event));
        return;
    }

    set_event_handler_disp(event_target, eid, disp);
}

void update_doc_cp_events(HTMLDocumentNode *doc, cp_static_data_t *cp)
{
    int i;

    for(i=0; i < EVENTID_LAST; i++) {
        if((event_info[i].flags & EVENT_DEFAULTLISTENER) && is_cp_event(cp, event_info[i].dispid))
            ensure_doc_nsevent_handler(doc, NULL, i);
    }
}

void check_event_attr(HTMLDocumentNode *doc, nsIDOMElement *nselem)
{
    nsIDOMMozNamedAttrMap *attr_map;
    const PRUnichar *name, *value;
    nsAString name_str, value_str;
    HTMLDOMNode *node = NULL;
    cpp_bool has_attrs;
    nsIDOMAttr *attr;
    IDispatch *disp;
    UINT32 length, i;
    eventid_t eid;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMElement_HasAttributes(nselem, &has_attrs);
    if(NS_FAILED(nsres) || !has_attrs)
        return;

    nsres = nsIDOMElement_GetAttributes(nselem, &attr_map);
    if(NS_FAILED(nsres))
        return;

    nsres = nsIDOMMozNamedAttrMap_GetLength(attr_map, &length);
    assert(nsres == NS_OK);

    nsAString_Init(&name_str, NULL);
    nsAString_Init(&value_str, NULL);

    for(i = 0; i < length; i++) {
        nsres = nsIDOMMozNamedAttrMap_Item(attr_map, i, &attr);
        if(NS_FAILED(nsres))
            continue;

        nsres = nsIDOMAttr_GetName(attr, &name_str);
        if(NS_FAILED(nsres)) {
            nsIDOMAttr_Release(attr);
            continue;
        }

        nsAString_GetData(&name_str, &name);
        eid = attr_to_eid(name);
        if(eid == EVENTID_LAST) {
            nsIDOMAttr_Release(attr);
            continue;
        }

        nsres = nsIDOMAttr_GetValue(attr, &value_str);
        nsIDOMAttr_Release(attr);
        if(NS_FAILED(nsres))
            continue;

        nsAString_GetData(&value_str, &value);
        if(!*value)
            continue;

        TRACE("%p.%s = %s\n", nselem, debugstr_w(name), debugstr_w(value));

        disp = script_parse_event(doc->window, value);
        if(!disp)
            continue;

        if(!node) {
            hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
            if(FAILED(hres)) {
                IDispatch_Release(disp);
                break;
            }
        }

        set_event_handler_disp(get_node_event_prop_target(node, eid), eid, disp);
        IDispatch_Release(disp);
    }

    if(node)
        node_release(node);
    nsAString_Finish(&name_str);
    nsAString_Finish(&value_str);
    nsIDOMMozNamedAttrMap_Release(attr_map);
}

HRESULT doc_init_events(HTMLDocumentNode *doc)
{
    unsigned i;
    HRESULT hres;

    doc->event_vector = calloc(EVENTID_LAST, sizeof(BOOL));
    if(!doc->event_vector)
        return E_OUTOFMEMORY;

    init_nsevents(doc);

    for(i=0; i < EVENTID_LAST; i++) {
        if(event_info[i].flags & EVENT_HASDEFAULTHANDLERS) {
            hres = ensure_doc_nsevent_handler(doc, NULL, i);
            if(FAILED(hres))
                return hres;
        }
    }

    return S_OK;
}

static inline EventTarget *impl_from_IEventTarget(IEventTarget *iface)
{
    return CONTAINING_RECORD(iface, EventTarget, IEventTarget_iface);
}

DISPEX_IDISPATCH_IMPL(EventTarget, IEventTarget, impl_from_IEventTarget(iface)->dispex)

static HRESULT WINAPI EventTarget_addEventListener(IEventTarget *iface, BSTR type,
                                                   IDispatch *function, VARIANT_BOOL capture)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    listener_type_t listener_type = capture ? LISTENER_TYPE_CAPTURE : LISTENER_TYPE_BUBBLE;
    listener_container_t *container;
    event_listener_t *listener;

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(type), function, capture);

    if(!function)
        return S_OK;

    container = get_listener_container(This, type, TRUE);
    if(!container)
        return E_OUTOFMEMORY;

    /* check for duplicates */
    LIST_FOR_EACH_ENTRY(listener, &container->listeners, event_listener_t, entry) {
        if(listener->type == listener_type && listener->function == function)
            return S_OK;
    }

    listener = malloc(sizeof(*listener));
    if(!listener)
        return E_OUTOFMEMORY;

    listener->type = listener_type;
    IDispatch_AddRef(listener->function = function);
    list_add_tail(&container->listeners, &listener->entry);
    return S_OK;
}

static HRESULT WINAPI EventTarget_removeEventListener(IEventTarget *iface, BSTR type,
                                                      IDispatch *listener, VARIANT_BOOL capture)
{
    EventTarget *This = impl_from_IEventTarget(iface);

    TRACE("(%p)->(%s %p %x)\n", This, debugstr_w(type), listener, capture);

    remove_event_listener(This, type, capture ? LISTENER_TYPE_CAPTURE : LISTENER_TYPE_BUBBLE, listener);
    return S_OK;
}

static HRESULT WINAPI EventTarget_dispatchEvent(IEventTarget *iface, IDOMEvent *event_iface, VARIANT_BOOL *result)
{
    EventTarget *This = impl_from_IEventTarget(iface);
    DOMEvent *event = unsafe_impl_from_IDOMEvent(event_iface);

    TRACE("(%p)->(%p %p)\n", This, event, result);

    if(!event) {
        WARN("Invalid event\n");
        return E_INVALIDARG;
    }

    return dispatch_event_object(This, event, DISPATCH_STANDARD, result);
}

static HRESULT IEventTarget_addEventListener_hook(DispatchEx *dispex, WORD flags,
        DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    /* If only two arguments were given, implicitly set capture to false */
    if((flags & DISPATCH_METHOD) && dp->cArgs == 2 && !dp->cNamedArgs) {
        VARIANT args[3];
        DISPPARAMS new_dp = {args, NULL, 3, 0};
        V_VT(args) = VT_BOOL;
        V_BOOL(args) = VARIANT_FALSE;
        args[1] = dp->rgvarg[0];
        args[2] = dp->rgvarg[1];

        TRACE("implicit capture\n");

        return dispex_call_builtin(dispex, DISPID_IEVENTTARGET_ADDEVENTLISTENER, &new_dp, res, ei, caller);
    }

    return S_FALSE; /* fallback to default */
}

static HRESULT IEventTarget_removeEventListener_hook(DispatchEx *dispex, WORD flags,
        DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    /* If only two arguments were given, implicitly set capture to false */
    if((flags & DISPATCH_METHOD) && dp->cArgs == 2 && !dp->cNamedArgs) {
        VARIANT args[3];
        DISPPARAMS new_dp = {args, NULL, 3, 0};
        V_VT(args) = VT_BOOL;
        V_BOOL(args) = VARIANT_FALSE;
        args[1] = dp->rgvarg[0];
        args[2] = dp->rgvarg[1];

        TRACE("implicit capture\n");

        return dispex_call_builtin(dispex, DISPID_IEVENTTARGET_REMOVEEVENTLISTENER, &new_dp, res, ei, caller);
    }

    return S_FALSE; /* fallback to default */
}

static const IEventTargetVtbl EventTargetVtbl = {
    EventTarget_QueryInterface,
    EventTarget_AddRef,
    EventTarget_Release,
    EventTarget_GetTypeInfoCount,
    EventTarget_GetTypeInfo,
    EventTarget_GetIDsOfNames,
    EventTarget_Invoke,
    EventTarget_addEventListener,
    EventTarget_removeEventListener,
    EventTarget_dispatchEvent
};

static EventTarget *unsafe_impl_from_IEventTarget(IEventTarget *iface)
{
    return iface && iface->lpVtbl == &EventTargetVtbl ? impl_from_IEventTarget(iface) : NULL;
}

static HRESULT get_gecko_target(IEventTarget *target, nsIDOMEventTarget **ret)
{
    EventTarget *event_target = unsafe_impl_from_IEventTarget(target);
    const event_target_vtbl_t *vtbl;
    nsresult nsres;

    if(!event_target) {
        WARN("Not our IEventTarget implementation\n");
        return E_INVALIDARG;
    }

    vtbl = (const event_target_vtbl_t*)dispex_get_vtbl(&event_target->dispex);
    nsres = nsISupports_QueryInterface(vtbl->get_gecko_target(&event_target->dispex),
                                       &IID_nsIDOMEventTarget, (void**)ret);
    assert(nsres == NS_OK);
    return S_OK;
}

void *EventTarget_query_interface(EventTarget *event_target, REFIID riid)
{
    if(IsEqualGUID(riid, &IID_IEventTarget)) {
        if(use_event_quirks(event_target)) {
            WARN("IEventTarget queried, but not supported by in document mode\n");
            return NULL;
        }
        return &event_target->IEventTarget_iface;
    }

    return NULL;
}

void EventTarget_init_dispex_info(dispex_data_t *dispex_info, compat_mode_t compat_mode)
{
    static const dispex_hook_t IEventTarget_hooks[] = {
        {DISPID_IEVENTTARGET_ADDEVENTLISTENER, IEventTarget_addEventListener_hook},
        {DISPID_IEVENTTARGET_REMOVEEVENTLISTENER, IEventTarget_removeEventListener_hook},
        {DISPID_UNKNOWN}
    };

    if(compat_mode >= COMPAT_MODE_IE9)
        dispex_info_add_interface(dispex_info, IEventTarget_tid, IEventTarget_hooks);
}

static int event_id_cmp(const void *key, const struct wine_rb_entry *entry)
{
    return wcscmp(key, WINE_RB_ENTRY_VALUE(entry, listener_container_t, entry)->type);
}

void init_event_target(EventTarget *event_target, dispex_static_data_t *dispex_data, HTMLInnerWindow *script_global)
{
    compat_mode_t compat_mode = script_global && script_global->doc ? script_global->doc->document_mode : COMPAT_MODE_NONE;
    init_dispatch(&event_target->dispex, dispex_data, script_global, compat_mode);
    event_target->IEventTarget_iface.lpVtbl = &EventTargetVtbl;
    wine_rb_init(&event_target->handler_map, event_id_cmp);
}

void traverse_event_target(EventTarget *event_target, nsCycleCollectionTraversalCallback *cb)
{
    listener_container_t *iter;
    event_listener_t *listener;

    RB_FOR_EACH_ENTRY(iter, &event_target->handler_map, listener_container_t, entry)
        LIST_FOR_EACH_ENTRY(listener, &iter->listeners, event_listener_t, entry)
            if(listener->function)
                note_cc_edge((nsISupports*)listener->function, "EventTarget.listener", cb);
}

void release_event_target(EventTarget *event_target)
{
    listener_container_t *iter, *iter2;

    WINE_RB_FOR_EACH_ENTRY_DESTRUCTOR(iter, iter2, &event_target->handler_map, listener_container_t, entry) {
        while(!list_empty(&iter->listeners)) {
            event_listener_t *listener = LIST_ENTRY(list_head(&iter->listeners), event_listener_t, entry);
            list_remove(&listener->entry);
            if(listener->function)
                IDispatch_Release(listener->function);
            free(listener);
        }
        free(iter);
    }
    rb_destroy(&event_target->handler_map, NULL, NULL);
}
