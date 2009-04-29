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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "mshtml_private.h"
#include "htmlevent.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct event_target_t {
    IDispatch *event_table[EVENTID_LAST];
};

static const WCHAR blurW[] = {'b','l','u','r',0};
static const WCHAR onblurW[] = {'o','n','b','l','u','r',0};

static const WCHAR changeW[] = {'c','h','a','n','g','e',0};
static const WCHAR onchangeW[] = {'o','n','c','h','a','n','g','e',0};

static const WCHAR clickW[] = {'c','l','i','c','k',0};
static const WCHAR onclickW[] = {'o','n','c','l','i','c','k',0};

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

static const WCHAR selectstartW[] = {'s','e','l','e','c','t','s','t','a','r','t',0};
static const WCHAR onselectstartW[] = {'o','n','s','e','l','e','c','t','s','t','a','r','t',0};

typedef struct {
    LPCWSTR name;
    LPCWSTR attr_name;
    DWORD flags;
} event_info_t;

#define EVENT_DEFAULTLISTENER    0x0001
#define EVENT_BUBBLE             0x0002

static const event_info_t event_info[] = {
    {blurW,         onblurW,         EVENT_DEFAULTLISTENER},
    {changeW,       onchangeW,       EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {clickW,        onclickW,        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {dragW,         ondragW,         0},
    {dragstartW,    ondragstartW,    0},
    {focusW,        onfocusW,        EVENT_DEFAULTLISTENER},
    {keydownW,      onkeydownW,      EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {keyupW,        onkeyupW,        EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {loadW,         onloadW,         0},
    {mousedownW,    onmousedownW,    EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseoutW,     onmouseoutW,     EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseoverW,    onmouseoverW,    EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {mouseupW,      onmouseupW,      EVENT_DEFAULTLISTENER|EVENT_BUBBLE},
    {pasteW,        onpasteW,        0},
    {selectstartW,  onselectstartW,  0}
};

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

typedef struct {
    DispatchEx dispex;
    const IHTMLEventObjVtbl  *lpIHTMLEventObjVtbl;

    LONG ref;

    HTMLDOMNode *target;
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

    if(!ref)
        heap_free(This);

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
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_ctrlKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_shiftKey(IHTMLEventObj *iface, VARIANT_BOOL *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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
    return E_NOTIMPL;
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
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_fromElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_toElement(IHTMLEventObj *iface, IHTMLElement **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_button(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_type(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_qualifier(IHTMLEventObj *iface, BSTR *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_reason(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_x(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_y(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_clientX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_clientY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_offsetX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_offsetY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_screenX(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_screenY(IHTMLEventObj *iface, LONG *p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLEventObj_get_srcFilter(IHTMLEventObj *iface, IDispatch **p)
{
    HTMLEventObj *This = HTMLEVENTOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

static IHTMLEventObj *create_event(HTMLDOMNode *target)
{
    HTMLEventObj *ret;

    ret = heap_alloc(sizeof(*ret));
    ret->lpIHTMLEventObjVtbl = &HTMLEventObjVtbl;
    ret->ref = 1;
    ret->target = target;
    IHTMLDOMNode_AddRef(HTMLDOMNODE(target));

    init_dispex(&ret->dispex, (IUnknown*)HTMLEVENTOBJ(ret), &HTMLEventObj_dispex);

    return HTMLEVENTOBJ(ret);
}

void fire_event(HTMLDocument *doc, eventid_t eid, nsIDOMNode *target)
{
    IHTMLEventObj *prev_event, *event_obj = NULL;
    nsIDOMNode *parent, *nsnode;
    HTMLDOMNode *node;
    PRUint16 node_type;

    nsIDOMNode_GetNodeType(target, &node_type);
    if(node_type != ELEMENT_NODE) {
        FIXME("node type %d node supported\n", node_type);
        return;
    }

    prev_event = doc->window->event;
    nsnode = target;
    nsIDOMNode_AddRef(nsnode);

    while(1) {
        node = get_node(doc, nsnode, FALSE);

        if(node && node->event_target && node->event_target->event_table[eid]) {
            if(!event_obj)
                event_obj = doc->window->event = create_event(get_node(doc, target, TRUE));

            TRACE("%s >>>\n", debugstr_w(event_info[eid].name));
            call_disp_func(doc, node->event_target->event_table[eid], (IDispatch*)HTMLDOMNODE(node));
            TRACE("%s <<<\n", debugstr_w(event_info[eid].name));
        }

        if(!(event_info[eid].flags & EVENT_BUBBLE))
            break;

        nsIDOMNode_GetParentNode(nsnode, &parent);
        nsIDOMNode_Release(nsnode);
        nsnode = parent;
        if(!nsnode)
            break;

        nsIDOMNode_GetNodeType(nsnode, &node_type);
        if(node_type != ELEMENT_NODE)
            break;
    }

    if(nsnode)
        nsIDOMNode_Release(nsnode);

    if((event_info[eid].flags & EVENT_BUBBLE) && doc->event_target && doc->event_target->event_table[eid]) {
        if(!event_obj)
            event_obj = doc->window->event = create_event(get_node(doc, target, TRUE));

        TRACE("doc %s >>>\n", debugstr_w(event_info[eid].name));
        call_disp_func(doc, doc->event_target->event_table[eid], (IDispatch*)HTMLDOC(doc));
        TRACE("doc %s <<<\n", debugstr_w(event_info[eid].name));
    }

    if(event_obj) {
        IHTMLEventObj_Release(event_obj);
        doc->window->event = prev_event;
    }
}

static HRESULT set_event_handler_disp(event_target_t **event_target, HTMLDocument *doc, eventid_t eid, IDispatch *disp)
{
    if(!*event_target)
        *event_target = heap_alloc_zero(sizeof(event_target_t));
    else if((*event_target)->event_table[eid])
        IDispatch_Release((*event_target)->event_table[eid]);

    (*event_target)->event_table[eid] = disp;
    if(!disp)
        return S_OK;
    IDispatch_AddRef(disp);

    if(doc->nscontainer && (event_info[eid].flags & EVENT_DEFAULTLISTENER)) {
        if(!doc->nscontainer->event_vector) {
            doc->nscontainer->event_vector = heap_alloc_zero(EVENTID_LAST*sizeof(BOOL));
            if(!doc->nscontainer->event_vector)
                return E_OUTOFMEMORY;
        }

        if(!doc->nscontainer->event_vector[eid]) {
            doc->nscontainer->event_vector[eid] = TRUE;
            add_nsevent_listener(doc->nscontainer, event_info[eid].name);
        }
    }

    return S_OK;
}

HRESULT set_event_handler(event_target_t **event_target, HTMLDocument *doc, eventid_t eid, VARIANT *var)
{
    switch(V_VT(var)) {
    case VT_NULL:
        if(*event_target && (*event_target)->event_table[eid]) {
            IDispatch_Release((*event_target)->event_table[eid]);
            (*event_target)->event_table[eid] = NULL;
        }
        break;

    case VT_DISPATCH:
        return set_event_handler_disp(event_target, doc, eid, V_DISPATCH(var));

    default:
        FIXME("not supported vt=%d\n", V_VT(var));
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT get_event_handler(event_target_t **event_target, eventid_t eid, VARIANT *var)
{
    if(*event_target && (*event_target)->event_table[eid]) {
        V_VT(var) = VT_DISPATCH;
        V_DISPATCH(var) = (*event_target)->event_table[eid];
        IDispatch_AddRef(V_DISPATCH(var));
    }else {
        V_VT(var) = VT_NULL;
    }

    return S_OK;
}

void check_event_attr(HTMLDocument *doc, nsIDOMElement *nselem)
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

            disp = script_parse_event(doc, attr_value);
            if(disp) {
                node = get_node(doc, (nsIDOMNode*)nselem, TRUE);
                set_event_handler_disp(&node->event_target, node->doc, i, disp);
                IDispatch_Release(disp);
            }
        }
    }

    nsAString_Finish(&attr_value_str);
    nsAString_Finish(&attr_name_str);
}

void release_event_target(event_target_t *event_target)
{
    int i;

    for(i=0; i < EVENTID_LAST; i++) {
        if(event_target->event_table[i])
            IDispatch_Release(event_target->event_table[i]);
    }

    heap_free(event_target);
}
