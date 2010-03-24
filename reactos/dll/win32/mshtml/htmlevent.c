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

#include "mshtml_private.h"
#include "htmlevent.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    IDispatch *handler_prop;
    DWORD handler_cnt;
    IDispatch *handlers[0];
} handler_vector_t;

struct event_target_t {
    DWORD node_handlers_mask;
    handler_vector_t *event_table[EVENTID_LAST];
};

static const WCHAR beforeunloadW[] = {'b','e','f','o','r','e','u','n','l','o','a','d',0};
static const WCHAR onbeforeunloadW[] = {'o','n','b','e','f','o','r','e','u','n','l','o','a','d',0};

static const WCHAR blurW[] = {'b','l','u','r',0};
static const WCHAR onblurW[] = {'o','n','b','l','u','r',0};

static const WCHAR changeW[] = {'c','h','a','n','g','e',0};
static const WCHAR onchangeW[] = {'o','n','c','h','a','n','g','e',0};

static const WCHAR clickW[] = {'c','l','i','c','k',0};
static const WCHAR onclickW[] = {'o','n','c','l','i','c','k',0};

static const WCHAR dblclickW[] = {'d','b','l','c','l','i','c','k',0};
static const WCHAR ondblclickW[] = {'o','n','d','b','l','c','l','i','c','k',0};

static const WCHAR dragW[] = {'d','r','a','g',0};
static const WCHAR ondragW[] = {'o','n','d','r','a','g',0};

static const WCHAR dragstartW[] = {'d','r','a','g','s','t','a','r','t',0};
static const WCHAR ondragstartW[] = {'o','n','d','r','a','g','s','t','a','r','t',0};

static const WCHAR focusW[] = {'f','o','c','u','s',0};
static const WCHAR onfocusW[] = {'o','n','f','o','c','u','s',0};

static const WCHAR keydownW[] = {'k','e','y','d','o','w','n',0};
static const WCHAR onkeydownW[] = {'o','n','k','e','y','d','o','w','n',0};

static const WCHAR keyupW[] = {'k','e','y','u','p',0};
static const WCHAR onkeyupW[] = {'o','n','k','e','y','u','p',0};

static const WCHAR loadW[] = {'l','o','a','d',0};
static const WCHAR onloadW[] = {'o','n','l','o','a','d',0};

static const WCHAR mousedownW[] = {'m','o','u','s','e','d','o','w','n',0};
static const WCHAR onmousedownW[] = {'o','n','m','o','u','s','e','d','o','w','n',0};

static const WCHAR mouseoutW[] = {'m','o','u','s','e','o','u','t',0};
static const WCHAR onmouseoutW[] = {'o','n','m','o','u','s','e','o','u','t',0};

static const WCHAR mouseoverW[] = {'m','o','u','s','e','o','v','e','r',0};
static const WCHAR onmouseoverW[] = {'o','n','m','o','u','s','e','o','v','e','r',0};

static const WCHAR mouseupW[] = {'m','o','u','s','e','u','p',0};
static const WCHAR onmouseupW[] = {'o','n','m','o','u','s','e','u','p',0};

static const WCHAR pasteW[] = {'p','a','s','t','e',0};
static const WCHAR onpasteW[] = {'o','n','p','a','s','t','e',0};

static const WCHAR readystatechangeW[] = {'r','e','a','d','y','s','t','a','t','e','c','h','a','n','g','e',0};
static const WCHAR onreadystatechangeW[] = {'o','n','r','e','a','d','y','s','t','a','t','e','c','h','a','n','g','e',0};

static const WCHAR resizeW[] = {'r','e','s','i','z','e',0};
static const WCHAR onresizeW[] = {'o','n','r','e','s','i','z','e',0};

static const WCHAR selectstartW[] = {'s','e','l','e','c','t','s','t','a','r','t',0};
static const WCHAR onselectstartW[] = {'o','n','s','e','l','e','c','t','s','t','a','r','t',0};

static const WCHAR HTMLEventsW[] = {'H','T','M','L','E','v','e','n','t','s',0};
static const WCHAR KeyboardEventW[] = {'K','e','y','b','o','a','r','d','E','v','e','n','t',0};
static const WCHAR MouseEventW[] = {'M','o','u','s','e','E','v','e','n','t',0};

enum {
    EVENTT_NONE,
    EVENTT_HTML,
    EVENTT_KEY,
    EVENTT_MOUSE
};

static const WCHAR *event_types[] = {
    NULL,
    HTMLEventsW,
    KeyboardEventW,
    MouseEventW
};

typedef struct {
    LPCWSTR name;
    LPCWSTR attr_name;
    DWORD type;
    DISPID dispid;
    DWORD flags;
} event_info_t;

#define EVENT_DEFAULTLISTENER    0x0001
#define EVENT_BUBBLE             0x0002
#define EVENT_FORWARDBODY        0x0004
#define EVENT_NODEHANDLER        0x0008

static const event_info_t event_info[] = {
    {beforeunloadW,      onbeforeunloadW,      EVENTT_NONE,   DISPID_EVMETH_ONBEFOREUNLOAD,
        EVENT_DEFAULTLISTENER|EVENT_FORWARDBODY},
    {blurW,              onblurW,              EVENTT_HTML,   DISPID_EVMETH_ONBLUR,
        EVENT_DEFAULTLISTENER},
    {changeW,            onchangeW,            EVENTT_HTML,   DISPID_EVMETH_ONCHANGE,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {clickW,             onclickW,             EVENTT_MOUSE,  DISPID_EVMETH_ONCLICK,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {dblclickW,          ondblclickW,          EVENTT_MOUSE,  DISPID_EVMETH_ONDBLCLICK,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {dragW,              ondragW,              EVENTT_MOUSE,  DISPID_EVMETH_ONDRAG,
        0},
    {dragstartW,         ondragstartW,         EVENTT_MOUSE,  DISPID_EVMETH_ONDRAGSTART,
        0},
    {focusW,             onfocusW,             EVENTT_HTML,   DISPID_EVMETH_ONFOCUS,
        EVENT_DEFAULTLISTENER},
    {keydownW,           onkeydownW,           EVENTT_KEY,    DISPID_EVMETH_ONKEYDOWN,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {keyupW,             onkeyupW,             EVENTT_KEY,    DISPID_EVMETH_ONKEYUP,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {loadW,              onloadW,              EVENTT_HTML,   DISPID_EVMETH_ONLOAD,
        EVENT_NODEHANDLER},
    {mousedownW,         onmousedownW,         EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEDOWN,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseoutW,          onmouseoutW,          EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEOUT,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseoverW,         onmouseoverW,         EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEOVER,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseupW,           onmouseupW,           EVENTT_MOUSE,  DISPID_EVMETH_ONMOUSEUP,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {pasteW,             onpasteW,             EVENTT_NONE,   DISPID_EVMETH_ONPASTE,
        0},
    {readystatechangeW,  onreadystatechangeW,  EVENTT_NONE,   DISPID_EVMETH_ONREADYSTATECHANGE,
        0},
    {resizeW,            onresizeW,            EVENTT_NONE,   DISPID_EVMETH_ONRESIZE,
        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {selectstartW,       onselectstartW,       EVENTT_MOUSE,  DISPID_EVMETH_ONSELECTSTART,
        0}
};

static const eventid_t node_handled_list[] = { EVENTID_LOAD };

eventid_t str_to_eid(LPCWSTR str)
{
    int i;

    for(i=0; i < sizeof(event_info)/sizeof(event_info[0]); i++) {
        if(!strcmpW(event_info[i].name, str))
            return i;
    }

    ERR("unknown type %s\n", debugstr_w(str));
    return EVENTID_LAST;
}

static eventid_t attr_to_eid(LPCWSTR str)
{
    int i;

    for(i=0; i < sizeof(event_info)/sizeof(event_info[0]); i++) {
        if(!strcmpW(event_info[i].attr_name, str))
            return i;
    }

    return EVENTID_LAST;
}

static DWORD get_node_handler_mask(eventid_t eid)
{
    DWORD i;

    for(i=0; i<sizeof(node_handled_list)/sizeof(*node_handled_list); i++) {
        if(node_handled_list[i] == eid)
            return 1 << i;
    }

    ERR("Invalid eid %d\n", eid);
    return ~0;
}

typedef struct {
    DispatchEx dispex;
    const IHTMLEventObjVtbl  *lpIHTMLEventObjVtbl;

    LONG ref;

    HTMLDOMNode *target;
    const event_info_t *type;
    nsIDOMEvent *nsevent;
} HTMLEventObj;

#define HTMLEVENTOBJ(x) ((IHTMLEventObj*) &(x)->lpIHTMLEventObjVtbl)

#define HTMLEVENTOBJ_THIS(iface) DEFINE_THIS(HTMLEventObj, IHTMLEventObj, iface)

static HRESULT WINAPI HTMLEventObj_QueryInterface(IHTMLEventObj *iface, REFIID riid, void **ppv)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLEVENTOBJ(This);
    }else if(IsEqualGUID(&IID_IHTMLEventObj, riid)) {
        TRACE("(%p)->(IID_IHTMLEventObj %p)\n", This, ppv);
        *ppv = HTMLEVENTOBJ(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLEventObj_AddRef(IHTMLEventObj *iface)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLEventObj_Release(IHTMLEventObj *iface)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsevent)
            nsIDOMEvent_Release(This->nsevent);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLEventObj_GetTypeInfoCount(IHTMLEventObj *iface, UINT *pctinfo)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_GetTypeInfo(IHTMLEventObj *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_GetIDsOfNames(IHTMLEventObj *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_Invoke(IHTMLEventObj *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_srcElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(This->target), &IID_IHTMLElement, (void**)p);
}

static HRESULT WINAPI HTMLEventObj_get_altKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRBool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetAltKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetAltKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_ctrlKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRBool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetCtrlKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetCtrlKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_shiftKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRBool ret = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetShiftKey(key_event, &ret);
            nsIDOMKeyEvent_Release(key_event);
        }else {
            nsIDOMMouseEvent *mouse_event;

            nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
            if(NS_SUCCEEDED(nsres)) {
                nsIDOMMouseEvent_GetShiftKey(mouse_event, &ret);
                nsIDOMMouseEvent_Release(mouse_event);
            }
        }
    }

    *p = ret ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_returnValue(IHTMLEventObj *iface, VARIANT v)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_returnValue(IHTMLEventObj *iface, VARIANT *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_EMPTY;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL v)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_cancelBubble(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_fromElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_toElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_put_keyCode(IHTMLEventObj *iface, LONG v)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_keyCode(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRUint32 key_code = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMKeyEvent_GetKeyCode(key_event, &key_code);
            nsIDOMKeyEvent_Release(key_event);
        }
    }

    *p = key_code;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_button(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRUint16 button = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetButton(mouse_event, &button);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = button;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_type(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(This->type->name);
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLEventObj_get_qualifier(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_reason(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_x(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = -1;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_y(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = -1;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRInt32 x = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetClientX(mouse_event, &x);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_clientY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRInt32 y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetClientY(mouse_event, &y);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_offsetY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRInt32 x = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetScreenX(mouse_event, &x);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = x;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_screenY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    PRInt32 y = 0;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsevent) {
        nsIDOMMouseEvent *mouse_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(This->nsevent, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMMouseEvent_GetScreenY(mouse_event, &y);
            nsIDOMMouseEvent_Release(mouse_event);
        }
    }

    *p = y;
    return S_OK;
}

static HRESULT WINAPI HTMLEventObj_get_srcFilter(IHTMLEventObj *iface, IDispatch **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);

    FIXME("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

#undef HTMLEVENTOBJ_THIS

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

static const tid_t HTMLEventObj_iface_tids[] = {
    IHTMLEventObj_tid,
    0
};

static dispex_static_data_t HTMLEventObj_dispex = {
    NULL,
    DispCEventObj_tid,
    NULL,
    HTMLEventObj_iface_tids
};

static IHTMLEventObj *create_event(HTMLDOMNode *target, eventid_t eid, nsIDOMEvent *nsevent)
{
    HTMLEventObj *ret;

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return NULL;

    ret->lpIHTMLEventObjVtbl = &HTMLEventObjVtbl;
    ret->ref = 1;
    ret->type = event_info+eid;

    ret->nsevent = nsevent;
    if(nsevent) {
        nsIDOMEvent_AddRef(nsevent);
    }else if(event_types[event_info[eid].type]) {
        nsIDOMDocumentEvent *doc_event;
        nsresult nsres;

        nsres = nsIDOMHTMLDocument_QueryInterface(target->doc->nsdoc, &IID_nsIDOMDocumentEvent,
                 (void**)&doc_event);
        if(NS_SUCCEEDED(nsres)) {
            nsAString type_str;

            nsAString_InitDepend(&type_str, event_types[event_info[eid].type]);
            nsres = nsIDOMDocumentEvent_CreateEvent(doc_event, &type_str, &ret->nsevent);
            nsAString_Finish(&type_str);
            nsIDOMDocumentEvent_Release(doc_event);
        }
        if(NS_FAILED(nsres)) {
            ERR("Could not create event: %08x\n", nsres);
            IHTMLEventObj_Release(HTMLEVENTOBJ(ret));
            return NULL;
        }
    }

    ret->target = target;
    IHTMLDOMNode_AddRef(HTMLDOMNODE(target));

    init_dispex(&ret->dispex, (IUnknown*)HTMLEVENTOBJ(ret), &HTMLEventObj_dispex);

    return HTMLEVENTOBJ(ret);
}

static HRESULT call_cp_func(IDispatch *disp, DISPID dispid)
{
    DISPPARAMS dp = {NULL,NULL,0,0};
    ULONG argerr;
    EXCEPINFO ei;
    VARIANT vres;
    HRESULT hres;

    V_VT(&vres) = VT_EMPTY;
    memset(&ei, 0, sizeof(ei));
    hres = IDispatch_Invoke(disp, dispid, &IID_NULL, 0, DISPATCH_METHOD, &dp, &vres, &ei, &argerr);
    if(SUCCEEDED(hres) && V_VT(&vres) != VT_EMPTY) {
        FIXME("handle result %s\n", debugstr_variant(&vres));
        VariantClear(&vres);
    }

    return hres;
}

static BOOL is_cp_event(cp_static_data_t *data, DISPID dispid)
{
    int min, max, i;
    HRESULT hres;

    if(!data)
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

static void call_event_handlers(HTMLDocumentNode *doc, IHTMLEventObj *event_obj, event_target_t *event_target,
        ConnectionPointContainer *cp_container, eventid_t eid, IDispatch *this_obj)
{
    handler_vector_t *handler_vector = NULL;
    int i;
    HRESULT hres;

    if(event_target)
        handler_vector = event_target->event_table[eid];

    if(handler_vector && handler_vector->handler_prop) {
        DISPID named_arg = DISPID_THIS;
        VARIANTARG arg;
        DISPPARAMS dp = {&arg, &named_arg, 1, 1};

        V_VT(&arg) = VT_DISPATCH;
        V_DISPATCH(&arg) = this_obj;

        TRACE("%s >>>\n", debugstr_w(event_info[eid].name));
        hres = call_disp_func(handler_vector->handler_prop, &dp);
        if(hres == S_OK)
            TRACE("%s <<<\n", debugstr_w(event_info[eid].name));
        else
            WARN("%s <<< %08x\n", debugstr_w(event_info[eid].name), hres);
    }

    if(handler_vector && handler_vector->handler_cnt) {
        VARIANTARG arg;
        DISPPARAMS dp = {&arg, NULL, 1, 0};

        V_VT(&arg) = VT_DISPATCH;
        V_DISPATCH(&arg) = (IDispatch*)event_obj;

        i = handler_vector->handler_cnt;
        while(i--) {
            if(handler_vector->handlers[i]) {
                TRACE("%s [%d] >>>\n", debugstr_w(event_info[eid].name), i);
                hres = call_disp_func(handler_vector->handlers[i], &dp);
                if(hres == S_OK)
                    TRACE("%s [%d] <<<\n", debugstr_w(event_info[eid].name), i);
                else
                    WARN("%s [%d] <<< %08x\n", debugstr_w(event_info[eid].name), i, hres);
            }
        }
    }

    if(cp_container) {
        ConnectionPoint *cp;

        if(cp_container->forward_container)
            cp_container = cp_container->forward_container;

        for(cp = cp_container->cp_list; cp; cp = cp->next) {
            if(cp->sinks_size && is_cp_event(cp->data, event_info[eid].dispid)) {
                for(i=0; i < cp->sinks_size; i++) {
                    if(!cp->sinks[i].disp)
                        continue;

                    TRACE("cp %s [%d] >>>\n", debugstr_w(event_info[eid].name), i);
                    hres = call_cp_func(cp->sinks[i].disp, event_info[eid].dispid);
                    if(hres == S_OK)
                        TRACE("cp %s [%d] <<<\n", debugstr_w(event_info[eid].name), i);
                    else
                        WARN("cp %s [%d] <<< %08x\n", debugstr_w(event_info[eid].name), i, hres);
                }
            }
        }
    }
}

void fire_event(HTMLDocumentNode *doc, eventid_t eid, BOOL set_event, nsIDOMNode *target, nsIDOMEvent *nsevent)
{
    IHTMLEventObj *prev_event, *event_obj = NULL;
    nsIDOMNode *parent, *nsnode;
    HTMLDOMNode *node;
    PRUint16 node_type;

    TRACE("(%p) %s\n", doc, debugstr_w(event_info[eid].name));

    prev_event = doc->basedoc.window->event;
    if(set_event)
        event_obj = create_event(get_node(doc, target, TRUE), eid, nsevent);
    doc->basedoc.window->event = event_obj;

    nsIDOMNode_GetNodeType(target, &node_type);
    nsnode = target;
    nsIDOMNode_AddRef(nsnode);

    switch(node_type) {
    case ELEMENT_NODE:
        do {
            node = get_node(doc, nsnode, FALSE);
            if(node)
                call_event_handlers(doc, event_obj, *get_node_event_target(node), node->cp_container, eid,
                        (IDispatch*)HTMLDOMNODE(node));

            if(!(event_info[eid].flags & EVENT_BUBBLE))
                break;

            nsIDOMNode_GetParentNode(nsnode, &parent);
            nsIDOMNode_Release(nsnode);
            nsnode = parent;
            if(!nsnode)
                break;

            nsIDOMNode_GetNodeType(nsnode, &node_type);
        }while(node_type == ELEMENT_NODE);

        if(!(event_info[eid].flags & EVENT_BUBBLE))
            break;

    case DOCUMENT_NODE:
        if(event_info[eid].flags & EVENT_FORWARDBODY) {
            nsIDOMHTMLElement *nsbody;
            nsresult nsres;

            nsres = nsIDOMHTMLDocument_GetBody(doc->nsdoc, &nsbody);
            if(NS_SUCCEEDED(nsres) && nsbody) {
                node = get_node(doc, (nsIDOMNode*)nsbody, FALSE);
                if(node)
                    call_event_handlers(doc, event_obj, *get_node_event_target(node), node->cp_container,
                            eid, (IDispatch*)HTMLDOMNODE(node));
                nsIDOMHTMLElement_Release(nsbody);
            }else {
                ERR("Could not get body: %08x\n", nsres);
            }
        }

        call_event_handlers(doc, event_obj, doc->node.event_target, &doc->basedoc.cp_container, eid,
                (IDispatch*)HTMLDOC(&doc->basedoc));
        break;

    default:
        FIXME("unimplemented node type %d\n", node_type);
    }

    if(nsnode)
        nsIDOMNode_Release(nsnode);

    if(event_obj)
        IHTMLEventObj_Release(event_obj);
    doc->basedoc.window->event = prev_event;
}

HRESULT dispatch_event(HTMLDOMNode *node, const WCHAR *event_name, VARIANT *event_obj, VARIANT_BOOL *cancelled)
{
    eventid_t eid;

    eid = attr_to_eid(event_name);
    if(eid == EVENTID_LAST) {
        WARN("unknown event %s\n", debugstr_w(event_name));
        return E_INVALIDARG;
    }

    if(event_obj && V_VT(event_obj) != VT_EMPTY && V_VT(event_obj) != VT_ERROR)
        FIXME("event_obj not implemented\n");

    if(!(event_info[eid].flags & EVENT_DEFAULTLISTENER)) {
        FIXME("not EVENT_DEFAULTEVENTHANDLER\n");
        return E_NOTIMPL;
    }

    fire_event(node->doc, eid, TRUE, node->nsnode, NULL);

    *cancelled = VARIANT_TRUE; /* FIXME */
    return S_OK;
}

HRESULT call_event(HTMLDOMNode *node, eventid_t eid)
{
    HRESULT hres;

    if(node->vtbl->call_event) {
        BOOL handled = FALSE;

        hres = node->vtbl->call_event(node, eid, &handled);
        if(handled)
            return hres;
    }

    fire_event(node->doc, eid, TRUE, node->nsnode, NULL);
    return S_OK;
}

static inline event_target_t *get_event_target(event_target_t **event_target_ptr)
{
    if(!*event_target_ptr)
        *event_target_ptr = heap_alloc_zero(sizeof(event_target_t));
    return *event_target_ptr;
}

static BOOL alloc_handler_vector(event_target_t *event_target, eventid_t eid, int cnt)
{
    handler_vector_t *new_vector, *handler_vector = event_target->event_table[eid];

    if(handler_vector) {
        if(cnt <= handler_vector->handler_cnt)
            return TRUE;

        new_vector = heap_realloc_zero(handler_vector, sizeof(handler_vector_t) + sizeof(IDispatch*)*cnt);
    }else {
        new_vector = heap_alloc_zero(sizeof(handler_vector_t) + sizeof(IDispatch*)*cnt);
    }

    if(!new_vector)
        return FALSE;

    new_vector->handler_cnt = cnt;
    event_target->event_table[eid] = new_vector;
    return TRUE;
}

static HRESULT ensure_nsevent_handler(HTMLDocumentNode *doc, event_target_t *event_target, nsIDOMNode *nsnode, eventid_t eid)
{
    if(!doc->nsdoc)
        return S_OK;

    if(event_info[eid].flags & EVENT_NODEHANDLER) {
        DWORD mask;

        mask = get_node_handler_mask(eid);
        if(event_target->node_handlers_mask & mask)
            return S_OK;

        add_nsevent_listener(doc, nsnode, event_info[eid].name);
        event_target->node_handlers_mask |= mask;
        return S_OK;
    }

    if(!(event_info[eid].flags & EVENT_DEFAULTLISTENER))
        return S_OK;

    if(!doc->event_vector) {
        doc->event_vector = heap_alloc_zero(EVENTID_LAST*sizeof(BOOL));
        if(!doc->event_vector)
            return E_OUTOFMEMORY;
    }

    if(!doc->event_vector[eid]) {
        doc->event_vector[eid] = TRUE;
        add_nsevent_listener(doc, NULL, event_info[eid].name);
    }

    return S_OK;
}

static HRESULT remove_event_handler(event_target_t **event_target, eventid_t eid)
{
    if(*event_target && (*event_target)->event_table[eid]->handler_prop) {
        IDispatch_Release((*event_target)->event_table[eid]->handler_prop);
        (*event_target)->event_table[eid]->handler_prop = NULL;
    }

    return S_OK;
}

static HRESULT set_event_handler_disp(event_target_t **event_target_ptr, nsIDOMNode *nsnode, HTMLDocumentNode *doc,
        eventid_t eid, IDispatch *disp)
{
    event_target_t *event_target;

    if(!disp)
        return remove_event_handler(event_target_ptr, eid);

    event_target = get_event_target(event_target_ptr);
    if(!event_target)
        return E_OUTOFMEMORY;

    if(!alloc_handler_vector(event_target, eid, 0))
        return E_OUTOFMEMORY;

    if(event_target->event_table[eid]->handler_prop)
        IDispatch_Release(event_target->event_table[eid]->handler_prop);

    event_target->event_table[eid]->handler_prop = disp;
    IDispatch_AddRef(disp);

    return ensure_nsevent_handler(doc, event_target, nsnode, eid);
}

HRESULT set_event_handler(event_target_t **event_target, nsIDOMNode *nsnode, HTMLDocumentNode *doc, eventid_t eid, VARIANT *var)
{
    switch(V_VT(var)) {
    case VT_NULL:
        return remove_event_handler(event_target, eid);

    case VT_DISPATCH:
        return set_event_handler_disp(event_target, nsnode, doc, eid, V_DISPATCH(var));

    default:
        FIXME("not supported vt=%d\n", V_VT(var));
    case VT_EMPTY:
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT get_event_handler(event_target_t **event_target, eventid_t eid, VARIANT *var)
{
    if(*event_target && (*event_target)->event_table[eid] && (*event_target)->event_table[eid]->handler_prop) {
        V_VT(var) = VT_DISPATCH;
        V_DISPATCH(var) = (*event_target)->event_table[eid]->handler_prop;
        IDispatch_AddRef(V_DISPATCH(var));
    }else {
        V_VT(var) = VT_NULL;
    }

    return S_OK;
}

HRESULT attach_event(event_target_t **event_target_ptr, nsIDOMNode *nsnode, HTMLDocument *doc, BSTR name,
        IDispatch *disp, VARIANT_BOOL *res)
{
    event_target_t *event_target;
    eventid_t eid;
    DWORD i = 0;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        *res = VARIANT_TRUE;
        return S_OK;
    }

    event_target = get_event_target(event_target_ptr);
    if(!event_target)
        return E_OUTOFMEMORY;

    if(event_target->event_table[eid]) {
        while(i < event_target->event_table[eid]->handler_cnt && event_target->event_table[eid]->handlers[i])
            i++;
        if(i == event_target->event_table[eid]->handler_cnt && !alloc_handler_vector(event_target, eid, i+1))
            return E_OUTOFMEMORY;
    }else if(!alloc_handler_vector(event_target, eid, i+1)) {
        return E_OUTOFMEMORY;
    }

    IDispatch_AddRef(disp);
    event_target->event_table[eid]->handlers[i] = disp;

    *res = VARIANT_TRUE;
    return ensure_nsevent_handler(doc->doc_node, event_target, nsnode, eid);
}

HRESULT detach_event(event_target_t *event_target, HTMLDocument *doc, BSTR name, IDispatch *disp)
{
    eventid_t eid;
    DWORD i = 0;

    if(!event_target)
        return S_OK;

    eid = attr_to_eid(name);
    if(eid == EVENTID_LAST) {
        WARN("Unknown event\n");
        return S_OK;
    }

    if(!event_target->event_table[eid])
        return S_OK;

    while(i < event_target->event_table[eid]->handler_cnt) {
        if(event_target->event_table[eid]->handlers[i] == disp) {
            IDispatch_Release(event_target->event_table[eid]->handlers[i]);
            event_target->event_table[eid]->handlers[i] = NULL;
        }
        i++;
    }

    return S_OK;
}

void update_cp_events(HTMLWindow *window, event_target_t **event_target_ptr, cp_static_data_t *cp, nsIDOMNode *nsnode)
{
    event_target_t *event_target;
    int i;

    event_target = get_event_target(event_target_ptr);
    if(!event_target)
        return; /* FIXME */

    for(i=0; i < EVENTID_LAST; i++) {
        if((event_info[i].flags & EVENT_DEFAULTLISTENER) && is_cp_event(cp, event_info[i].dispid))
            ensure_nsevent_handler(window->doc, event_target, nsnode, i);
    }
}

void check_event_attr(HTMLDocumentNode *doc, nsIDOMElement *nselem)
{
    const PRUnichar *attr_value;
    nsAString attr_name_str, attr_value_str;
    IDispatch *disp;
    HTMLDOMNode *node;
    int i;
    nsresult nsres;

    nsAString_Init(&attr_value_str, NULL);
    nsAString_Init(&attr_name_str, NULL);

    for(i=0; i < EVENTID_LAST; i++) {
        nsAString_SetData(&attr_name_str, event_info[i].attr_name);
        nsres = nsIDOMElement_GetAttribute(nselem, &attr_name_str, &attr_value_str);
        if(NS_SUCCEEDED(nsres)) {
            nsAString_GetData(&attr_value_str, &attr_value);
            if(!*attr_value)
                continue;

            TRACE("%p.%s = %s\n", nselem, debugstr_w(event_info[i].attr_name), debugstr_w(attr_value));

            disp = script_parse_event(doc->basedoc.window, attr_value);
            if(disp) {
                node = get_node(doc, (nsIDOMNode*)nselem, TRUE);
                set_event_handler_disp(get_node_event_target(node), node->nsnode, node->doc, i, disp);
                IDispatch_Release(disp);
            }
        }
    }

    nsAString_Finish(&attr_value_str);
    nsAString_Finish(&attr_name_str);
}

void release_event_target(event_target_t *event_target)
{
    int i, j;

    for(i=0; i < EVENTID_LAST; i++) {
        if(event_target->event_table[i]) {
            if(event_target->event_table[i]->handler_prop)
                IDispatch_Release(event_target->event_table[i]->handler_prop);
            for(j=0; j < event_target->event_table[i]->handler_cnt; j++)
                IDispatch_Release(event_target->event_table[i]->handlers[j]);
        }
    }

    heap_free(event_target);
}
