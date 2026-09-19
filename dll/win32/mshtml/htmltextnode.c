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
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "htmlevent.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct CharacterData {
    IWineHTMLCharacterData IWineHTMLCharacterData_iface;
    HTMLDOMNode *node;
    nsIDOMCharacterData *nschardata;
};

struct HTMLDOMTextNode {
    HTMLDOMNode node;
    IHTMLDOMTextNode IHTMLDOMTextNode_iface;
    IHTMLDOMTextNode2 IHTMLDOMTextNode2_iface;
    struct CharacterData character_data;

    nsIDOMText *nstext;
};

struct HTMLCommentElement {
    HTMLElement element;
    IHTMLCommentElement IHTMLCommentElement_iface;
    IHTMLCommentElement2 IHTMLCommentElement2_iface;
    struct CharacterData character_data;
};

static inline struct CharacterData *impl_from_IWineHTMLCharacterData(IWineHTMLCharacterData *iface)
{
    return CONTAINING_RECORD(iface, struct CharacterData, IWineHTMLCharacterData_iface);
}

DISPEX_IDISPATCH_IMPL(CharacterData, IWineHTMLCharacterData,
                      impl_from_IWineHTMLCharacterData(iface)->node->event_target.dispex)

static HRESULT WINAPI CharacterData_put_data(IWineHTMLCharacterData *iface, BSTR v)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nschardata) {
        FIXME("legacy doctype comment\n");
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMCharacterData_SetData(This->nschardata, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI CharacterData_get_data(IWineHTMLCharacterData *iface, BSTR *p)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);
    const PRUnichar *str, *end;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nschardata) {
        nsAString_Init(&nsstr, NULL);
        hres = nsnode_to_nsstring(This->node->nsnode, &nsstr);
        if(FAILED(hres))
            return hres;

        nsAString_GetData(&nsstr, &str);
        end = str + wcslen(str);
        if(*str == '<') {
            end -= (end[-1] == '>');
            str++;
            str += (*str == '!');
        }

        if(!(*p = SysAllocStringLen(str, end - str)))
            hres = E_OUTOFMEMORY;
        nsAString_Finish(&nsstr);
        return hres;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMCharacterData_GetData(This->nschardata, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI CharacterData_get_length(IWineHTMLCharacterData *iface, LONG *p)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);
    UINT32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nschardata) {
        FIXME("legacy doctype comment\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMCharacterData_GetLength(This->nschardata, &length);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    *p = length;
    return S_OK;
}

static HRESULT WINAPI CharacterData_substringData(IWineHTMLCharacterData *iface, LONG offset, LONG count, BSTR *string)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);

    FIXME("(%p)->(%ld %ld %p)\n", This, offset, count, string);

    return E_NOTIMPL;
}

static HRESULT WINAPI CharacterData_appendData(IWineHTMLCharacterData *iface, BSTR string)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(string));

    if(!This->nschardata) {
        FIXME("legacy doctype comment\n");
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, string);
    nsres = nsIDOMCharacterData_AppendData(This->nschardata, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    return S_OK;
}

static HRESULT WINAPI CharacterData_insertData(IWineHTMLCharacterData *iface, LONG offset, BSTR string)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);

    FIXME("(%p)->(%ld %s)\n", This, offset, debugstr_w(string));

    return E_NOTIMPL;
}

static HRESULT WINAPI CharacterData_deleteData(IWineHTMLCharacterData *iface, LONG offset, LONG count)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);

    FIXME("(%p)->(%ld %ld)\n", This, offset, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI CharacterData_replaceData(IWineHTMLCharacterData *iface, LONG offset, LONG count, BSTR string)
{
    struct CharacterData *This = impl_from_IWineHTMLCharacterData(iface);

    FIXME("(%p)->(%ld %ld %s)\n", This, offset, count, debugstr_w(string));

    return E_NOTIMPL;
}

static const IWineHTMLCharacterDataVtbl CharacterDataVtbl = {
    CharacterData_QueryInterface,
    CharacterData_AddRef,
    CharacterData_Release,
    CharacterData_GetTypeInfoCount,
    CharacterData_GetTypeInfo,
    CharacterData_GetIDsOfNames,
    CharacterData_Invoke,
    CharacterData_put_data,
    CharacterData_get_data,
    CharacterData_get_length,
    CharacterData_substringData,
    CharacterData_appendData,
    CharacterData_insertData,
    CharacterData_deleteData,
    CharacterData_replaceData
};

static void CharacterData_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    dispex_info_add_interface(info, IWineHTMLCharacterData_tid, NULL);
}

dispex_static_data_t CharacterData_dispex = {
    .id           = PROT_CharacterData,
    .prototype_id = PROT_Node,
    .init_info    = CharacterData_init_dispex_info,
};

static void init_char_data(HTMLDOMNode *node, nsIDOMCharacterData *nschardata, struct CharacterData *ret)
{
    /* nschardata shares reference with nsnode */
    ret->IWineHTMLCharacterData_iface.lpVtbl = &CharacterDataVtbl;
    ret->nschardata = nschardata;
    ret->node = node;
}

static inline HTMLDOMTextNode *impl_from_IHTMLDOMTextNode(IHTMLDOMTextNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, IHTMLDOMTextNode_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMTextNode, IHTMLDOMTextNode,
                      impl_from_IHTMLDOMTextNode(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDOMTextNode_put_data(IHTMLDOMTextNode *iface, BSTR v)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return CharacterData_put_data(&This->character_data.IWineHTMLCharacterData_iface, v);
}

static HRESULT WINAPI HTMLDOMTextNode_get_data(IHTMLDOMTextNode *iface, BSTR *p)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return CharacterData_get_data(&This->character_data.IWineHTMLCharacterData_iface, p);
}

static HRESULT WINAPI HTMLDOMTextNode_toString(IHTMLDOMTextNode *iface, BSTR *String)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);

    TRACE("(%p)->(%p)\n", This, String);

    if(!String)
        return E_INVALIDARG;

    return IHTMLDOMTextNode_get_data(&This->IHTMLDOMTextNode_iface, String);
}

static HRESULT WINAPI HTMLDOMTextNode_get_length(IHTMLDOMTextNode *iface, LONG *p)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    return CharacterData_get_length(&This->character_data.IWineHTMLCharacterData_iface, p);
}

static HRESULT WINAPI HTMLDOMTextNode_splitText(IHTMLDOMTextNode *iface, LONG offset, IHTMLDOMNode **pRetNode)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode(iface);
    HTMLDOMNode *node;
    nsIDOMText *text;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%ld %p)\n", This, offset, pRetNode);

    nsres = nsIDOMText_SplitText(This->nstext, offset, &text);
    if(NS_FAILED(nsres)) {
        ERR("SplitText failed: %lx08x\n", nsres);
        return E_FAIL;
    }

    if(!text) {
        *pRetNode = NULL;
        return S_OK;
    }

    hres = get_node((nsIDOMNode*)text, TRUE, &node);
    nsIDOMText_Release(text);
    if(FAILED(hres))
        return hres;

    *pRetNode = &node->IHTMLDOMNode_iface;
    return S_OK;
}

static const IHTMLDOMTextNodeVtbl HTMLDOMTextNodeVtbl = {
    HTMLDOMTextNode_QueryInterface,
    HTMLDOMTextNode_AddRef,
    HTMLDOMTextNode_Release,
    HTMLDOMTextNode_GetTypeInfoCount,
    HTMLDOMTextNode_GetTypeInfo,
    HTMLDOMTextNode_GetIDsOfNames,
    HTMLDOMTextNode_Invoke,
    HTMLDOMTextNode_put_data,
    HTMLDOMTextNode_get_data,
    HTMLDOMTextNode_toString,
    HTMLDOMTextNode_get_length,
    HTMLDOMTextNode_splitText
};

static inline HTMLDOMTextNode *impl_from_IHTMLDOMTextNode2(IHTMLDOMTextNode2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, IHTMLDOMTextNode2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMTextNode2, IHTMLDOMTextNode2,
                      impl_from_IHTMLDOMTextNode2(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDOMTextNode2_substringData(IHTMLDOMTextNode2 *iface, LONG offset, LONG count, BSTR *string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return CharacterData_substringData(&This->character_data.IWineHTMLCharacterData_iface, offset, count, string);
}

static HRESULT WINAPI HTMLDOMTextNode2_appendData(IHTMLDOMTextNode2 *iface, BSTR string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return CharacterData_appendData(&This->character_data.IWineHTMLCharacterData_iface, string);
}

static HRESULT WINAPI HTMLDOMTextNode2_insertData(IHTMLDOMTextNode2 *iface, LONG offset, BSTR string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return CharacterData_insertData(&This->character_data.IWineHTMLCharacterData_iface, offset, string);
}

static HRESULT WINAPI HTMLDOMTextNode2_deleteData(IHTMLDOMTextNode2 *iface, LONG offset, LONG count)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return CharacterData_deleteData(&This->character_data.IWineHTMLCharacterData_iface, offset, count);
}

static HRESULT WINAPI HTMLDOMTextNode2_replaceData(IHTMLDOMTextNode2 *iface, LONG offset, LONG count, BSTR string)
{
    HTMLDOMTextNode *This = impl_from_IHTMLDOMTextNode2(iface);
    return CharacterData_replaceData(&This->character_data.IWineHTMLCharacterData_iface, offset, count, string);
}

static const IHTMLDOMTextNode2Vtbl HTMLDOMTextNode2Vtbl = {
    HTMLDOMTextNode2_QueryInterface,
    HTMLDOMTextNode2_AddRef,
    HTMLDOMTextNode2_Release,
    HTMLDOMTextNode2_GetTypeInfoCount,
    HTMLDOMTextNode2_GetTypeInfo,
    HTMLDOMTextNode2_GetIDsOfNames,
    HTMLDOMTextNode2_Invoke,
    HTMLDOMTextNode2_substringData,
    HTMLDOMTextNode2_appendData,
    HTMLDOMTextNode2_insertData,
    HTMLDOMTextNode2_deleteData,
    HTMLDOMTextNode2_replaceData
};

static inline HTMLDOMTextNode *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, node);
}

static HRESULT HTMLDOMTextNode_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDOMTextNode *This = impl_from_HTMLDOMNode(iface);

    return HTMLDOMTextNode_Create(This->node.doc, nsnode, ret);
}

static inline HTMLDOMTextNode *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMTextNode, node.event_target.dispex);
}

static void *HTMLDOMTextNode_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLDOMTextNode *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLDOMTextNode, riid))
        return &This->IHTMLDOMTextNode_iface;
    if(IsEqualGUID(&IID_IHTMLDOMTextNode2, riid))
        return &This->IHTMLDOMTextNode2_iface;
    if(IsEqualGUID(&IID_IWineHTMLCharacterData, riid))
        return &This->character_data.IWineHTMLCharacterData_iface;

    return HTMLDOMNode_query_interface(&This->node.event_target.dispex, riid);
}

static const cpc_entry_t HTMLDOMTextNode_cpc[] = {{NULL}};

static const NodeImplVtbl HTMLDOMTextNodeImplVtbl = {
    .cpc_entries           = HTMLDOMTextNode_cpc,
    .clone                 = HTMLDOMTextNode_clone
};

static const dispex_static_data_vtbl_t Text_dispex_vtbl = {
    .query_interface = HTMLDOMTextNode_query_interface,
    .destructor      = HTMLDOMNode_destructor,
    .traverse        = HTMLDOMNode_traverse,
    .unlink          = HTMLDOMNode_unlink
};

static void Text_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t textnode_ie9_hooks[] = {
        {DISPID_IHTMLDOMTEXTNODE_TOSTRING},
        {DISPID_UNKNOWN}
    };
    HTMLDOMNode_init_dispex_info(info, mode);
    if(mode >= COMPAT_MODE_IE9)
        CharacterData_init_dispex_info(info, mode);
    else
        dispex_info_add_interface(info, IHTMLDOMTextNode2_tid, NULL);
    dispex_info_add_interface(info, IHTMLDOMTextNode_tid, mode >= COMPAT_MODE_IE9 ? textnode_ie9_hooks : NULL);
}

static const tid_t Text_iface_tids[] = {
    IHTMLDOMNode_tid,
    0
};
dispex_static_data_t Text_dispex = {
    .id           = PROT_Text,
    .prototype_id = PROT_CharacterData,
    .vtbl         = &Text_dispex_vtbl,
    .disp_tid     = DispHTMLDOMTextNode_tid,
    .iface_tids   = Text_iface_tids,
    .init_info    = Text_init_dispex_info,
};

HRESULT HTMLDOMTextNode_Create(HTMLDocumentNode *doc, nsIDOMNode *nsnode, HTMLDOMNode **node)
{
    HTMLDOMTextNode *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->node.vtbl = &HTMLDOMTextNodeImplVtbl;
    ret->IHTMLDOMTextNode_iface.lpVtbl = &HTMLDOMTextNodeVtbl;
    ret->IHTMLDOMTextNode2_iface.lpVtbl = &HTMLDOMTextNode2Vtbl;

    HTMLDOMNode_Init(doc, &ret->node, nsnode, &Text_dispex);

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMText, (void**)&ret->nstext);
    assert(nsres == NS_OK && (nsIDOMNode*)ret->nstext == ret->node.nsnode);

    /* Share reference with nsnode */
    nsIDOMNode_Release(ret->node.nsnode);

    init_char_data(&ret->node, (nsIDOMCharacterData*)ret->nstext, &ret->character_data);

    *node = &ret->node;
    return S_OK;
}

static inline HTMLCommentElement *comment_from_IHTMLCommentElement(IHTMLCommentElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLCommentElement, IHTMLCommentElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLCommentElement, IHTMLCommentElement,
                      comment_from_IHTMLCommentElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLCommentElement_put_text(IHTMLCommentElement *iface, BSTR v)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCommentElement_get_text(IHTMLCommentElement *iface, BSTR *p)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement_get_outerHTML(&This->element.IHTMLElement_iface, p);
}

static HRESULT WINAPI HTMLCommentElement_put_atomic(IHTMLCommentElement *iface, LONG v)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCommentElement_get_atomic(IHTMLCommentElement *iface, LONG *p)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCommentElementVtbl HTMLCommentElementVtbl = {
    HTMLCommentElement_QueryInterface,
    HTMLCommentElement_AddRef,
    HTMLCommentElement_Release,
    HTMLCommentElement_GetTypeInfoCount,
    HTMLCommentElement_GetTypeInfo,
    HTMLCommentElement_GetIDsOfNames,
    HTMLCommentElement_Invoke,
    HTMLCommentElement_put_text,
    HTMLCommentElement_get_text,
    HTMLCommentElement_put_atomic,
    HTMLCommentElement_get_atomic
};

static inline HTMLCommentElement *comment_from_IHTMLCommentElement2(IHTMLCommentElement2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCommentElement, IHTMLCommentElement2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLCommentElement2, IHTMLCommentElement2,
                      comment_from_IHTMLCommentElement2(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLCommentElement2_put_data(IHTMLCommentElement2 *iface, BSTR v)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_put_data(&This->character_data.IWineHTMLCharacterData_iface, v);
}

static HRESULT WINAPI HTMLCommentElement2_get_data(IHTMLCommentElement2 *iface, BSTR *p)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_get_data(&This->character_data.IWineHTMLCharacterData_iface, p);
}

static HRESULT WINAPI HTMLCommentElement2_get_length(IHTMLCommentElement2 *iface, LONG *p)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_get_length(&This->character_data.IWineHTMLCharacterData_iface, p);
}

static HRESULT WINAPI HTMLCommentElement2_substringData(IHTMLCommentElement2 *iface, LONG offset, LONG count, BSTR *string)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_substringData(&This->character_data.IWineHTMLCharacterData_iface, offset, count, string);
}

static HRESULT WINAPI HTMLCommentElement2_appendData(IHTMLCommentElement2 *iface, BSTR string)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_appendData(&This->character_data.IWineHTMLCharacterData_iface, string);
}

static HRESULT WINAPI HTMLCommentElement2_insertData(IHTMLCommentElement2 *iface, LONG offset, BSTR string)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_insertData(&This->character_data.IWineHTMLCharacterData_iface, offset, string);
}

static HRESULT WINAPI HTMLCommentElement2_deleteData(IHTMLCommentElement2 *iface, LONG offset, LONG count)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_deleteData(&This->character_data.IWineHTMLCharacterData_iface, offset, count);
}

static HRESULT WINAPI HTMLCommentElement2_replaceData(IHTMLCommentElement2 *iface, LONG offset, LONG count, BSTR string)
{
    HTMLCommentElement *This = comment_from_IHTMLCommentElement2(iface);
    return CharacterData_replaceData(&This->character_data.IWineHTMLCharacterData_iface, offset, count, string);
}

static const IHTMLCommentElement2Vtbl HTMLCommentElement2Vtbl = {
    HTMLCommentElement2_QueryInterface,
    HTMLCommentElement2_AddRef,
    HTMLCommentElement2_Release,
    HTMLCommentElement2_GetTypeInfoCount,
    HTMLCommentElement2_GetTypeInfo,
    HTMLCommentElement2_GetIDsOfNames,
    HTMLCommentElement2_Invoke,
    HTMLCommentElement2_put_data,
    HTMLCommentElement2_get_data,
    HTMLCommentElement2_get_length,
    HTMLCommentElement2_substringData,
    HTMLCommentElement2_appendData,
    HTMLCommentElement2_insertData,
    HTMLCommentElement2_deleteData,
    HTMLCommentElement2_replaceData
};

static inline HTMLCommentElement *comment_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLCommentElement, element.node);
}

static HRESULT HTMLCommentElement_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLCommentElement *This = comment_from_HTMLDOMNode(iface);
    HTMLElement *new_elem;
    HRESULT hres;

    hres = HTMLCommentElement_Create(This->element.node.doc, nsnode, &new_elem);
    if(FAILED(hres))
        return hres;

    *ret = &new_elem->node;
    return S_OK;
}

static inline HTMLCommentElement *comment_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLCommentElement, element.node.event_target.dispex);
}

static void *HTMLCommentElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLCommentElement *This = comment_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLCommentElement, riid))
        return &This->IHTMLCommentElement_iface;
    if(IsEqualGUID(&IID_IHTMLCommentElement2, riid))
        return &This->IHTMLCommentElement2_iface;
    if(IsEqualGUID(&IID_IWineHTMLCharacterData, riid))
        return &This->character_data.IWineHTMLCharacterData_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLCommentElementImplVtbl = {
    .clsid                 = &CLSID_HTMLCommentElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLCommentElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLCommentElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLCommentElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static void Comment_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t ie9_hooks[] = {
        {DISPID_IHTMLCOMMENTELEMENT_ATOMIC},
        {DISPID_UNKNOWN}
    };

    if(mode >= COMPAT_MODE_IE9)
        HTMLDOMNode_init_dispex_info(info, mode);
    else
        HTMLElement_init_dispex_info(info, mode);
    CharacterData_init_dispex_info(info, mode);

    dispex_info_add_interface(info, IHTMLCommentElement_tid, mode >= COMPAT_MODE_IE9 ? ie9_hooks : NULL);
}

dispex_static_data_t Comment_dispex = {
    .id           = PROT_Comment,
    .prototype_id = PROT_CharacterData,
    .vtbl         = &HTMLCommentElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLCommentElement_tid,
    .init_info    = Comment_init_dispex_info,
};

HRESULT HTMLCommentElement_Create(HTMLDocumentNode *doc, nsIDOMNode *nsnode, HTMLElement **elem)
{
    nsIDOMCharacterData *nschardata;
    HTMLCommentElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->element.node.vtbl = &HTMLCommentElementImplVtbl;
    ret->IHTMLCommentElement_iface.lpVtbl = &HTMLCommentElementVtbl;
    ret->IHTMLCommentElement2_iface.lpVtbl = &HTMLCommentElement2Vtbl;

    HTMLElement_Init(&ret->element, doc, NULL, &Comment_dispex);
    HTMLDOMNode_Init(doc, &ret->element.node, nsnode, &Comment_dispex);

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMCharacterData, (void**)&nschardata);
    if(nsres != NS_OK)
        nschardata = NULL;
    else {
        assert((nsIDOMNode*)nschardata == nsnode);

        /* Share reference with nsnode */
        nsIDOMCharacterData_Release(nschardata);
    }
    init_char_data(&ret->element.node, nschardata, &ret->character_data);

    *elem = &ret->element;
    return S_OK;
}
