/*
 *    Node implementation
 *
 * Copyright 2005 Mike McCormack
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

#define COBJMACROS

#include <stdarg.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/HTMLtree.h>
#include <libxslt/pattern.h>
#include <libxslt/transform.h>
#include <libxslt/imports.h>
#include <libxslt/variables.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/documents.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static const IID IID_xmlnode = {0x4f2f4ba2,0xb822,0x11df,{0x8b,0x8a,0x68,0x50,0xdf,0xd7,0x20,0x85}};

xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type )
{
    xmlnode *This;

    if ( !iface )
        return NULL;
    This = get_node_obj( iface );
    if ( !This || !This->node )
        return NULL;
    if ( type && This->node->type != type )
        return NULL;
    return This->node;
}

BOOL node_query_interface(xmlnode *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_xmlnode, riid)) {
        TRACE("(%p)->(IID_xmlnode %p)\n", This, ppv);
        *ppv = This;
        return TRUE;
    }

    return dispex_query_interface(&This->dispex, riid, ppv);
}

/* common ISupportErrorInfo implementation */
typedef struct {
   ISupportErrorInfo ISupportErrorInfo_iface;
   LONG ref;

   const tid_t* iids;
} SupportErrorInfo;

static inline SupportErrorInfo *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return CONTAINING_RECORD(iface, SupportErrorInfo, ISupportErrorInfo_iface);
}

static HRESULT WINAPI SupportErrorInfo_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void **obj)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISupportErrorInfo)) {
        *obj = iface;
        ISupportErrorInfo_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI SupportErrorInfo_AddRef(ISupportErrorInfo *iface)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p, refcount %ld.\n", iface, ref );
    return ref;
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    enum tid_t const *tid;

    TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));

    tid = This->iids;
    while (*tid != NULL_tid)
    {
        if (IsEqualGUID(riid, get_riid_from_tid(*tid)))
            return S_OK;
        tid++;
    }

    return S_FALSE;
}

static const struct ISupportErrorInfoVtbl SupportErrorInfoVtbl = {
    SupportErrorInfo_QueryInterface,
    SupportErrorInfo_AddRef,
    SupportErrorInfo_Release,
    SupportErrorInfo_InterfaceSupportsErrorInfo
};

HRESULT node_create_supporterrorinfo(enum tid_t const *iids, void **obj)
{
    SupportErrorInfo *This;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->ISupportErrorInfo_iface.lpVtbl = &SupportErrorInfoVtbl;
    This->ref = 1;
    This->iids = iids;

    *obj = &This->ISupportErrorInfo_iface;

    return S_OK;
}

xmlnode *get_node_obj(IXMLDOMNode *node)
{
    xmlnode *obj = NULL;
    HRESULT hres;

    hres = IXMLDOMNode_QueryInterface(node, &IID_xmlnode, (void**)&obj);
    if (!obj) WARN("node is not our IXMLDOMNode implementation\n");
    return SUCCEEDED(hres) ? obj : NULL;
}

HRESULT node_get_nodeName(xmlnode *This, BSTR *name)
{
    BSTR prefix, base;
    HRESULT hr;

    if (!name)
        return E_INVALIDARG;

    hr = node_get_base_name(This, &base);
    if (hr != S_OK) return hr;

    if (!base[0] && xmldoc_version(This->node->doc) != MSXML6)
    {
        SysFreeString(base);
        *name = SysAllocString(L"xmlns");
        return S_OK;
    }

    hr = node_get_prefix(This, &prefix);
    if (hr == S_OK)
    {
        static const WCHAR colW = ':';
        WCHAR *ptr;

        /* +1 for ':' */
        ptr = *name = SysAllocStringLen(NULL, SysStringLen(base) + SysStringLen(prefix) + 1);
        if (SysStringByteLen(prefix))
        {
            memcpy(ptr, prefix, SysStringByteLen(prefix));
            ptr += SysStringLen(prefix);
        }
        if (SysStringByteLen(base))
        {
            if (SysStringByteLen(prefix))
                memcpy(ptr++, &colW, sizeof(WCHAR));
            memcpy(ptr, base, SysStringByteLen(base));
        }

        SysFreeString(base);
        SysFreeString(prefix);
    }
    else
        *name = base;

    return S_OK;
}

HRESULT node_get_content(xmlnode *This, VARIANT *value)
{
    xmlChar *content;

    if(!value)
        return E_INVALIDARG;

    content = xmlNodeGetContent(This->node);
    V_VT(value) = VT_BSTR;
    V_BSTR(value) = bstr_from_xmlChar( content );
    xmlFree(content);

    TRACE("%p returned %s\n", This, debugstr_w(V_BSTR(value)));
    return S_OK;
}

HRESULT node_set_content(xmlnode *This, LPCWSTR value)
{
    xmlChar *str;

    TRACE("(%p)->(%s)\n", This, debugstr_w(value));
    str = xmlchar_from_wchar(value);
    if(!str)
        return E_OUTOFMEMORY;

    xmlNodeSetContent(This->node, str);
    free(str);
    return S_OK;
}

static HRESULT node_set_content_escaped(xmlnode *This, LPCWSTR value)
{
    xmlChar *str, *escaped;

    TRACE("(%p)->(%s)\n", This, debugstr_w(value));
    str = xmlchar_from_wchar(value);
    if(!str)
        return E_OUTOFMEMORY;

    escaped = xmlEncodeSpecialChars(NULL, str);
    if(!escaped)
    {
        free(str);
        return E_OUTOFMEMORY;
    }

    xmlNodeSetContent(This->node, escaped);

    free(str);
    xmlFree(escaped);

    return S_OK;
}

HRESULT node_put_value(xmlnode *This, VARIANT *value)
{
    HRESULT hr;

    if (V_VT(value) != VT_BSTR)
    {
        VARIANT string_value;

        VariantInit(&string_value);
        hr = VariantChangeType(&string_value, value, 0, VT_BSTR);
        if(FAILED(hr)) {
            WARN("Couldn't convert to VT_BSTR\n");
            return hr;
        }

        hr = node_set_content(This, V_BSTR(&string_value));
        VariantClear(&string_value);
    }
    else
        hr = node_set_content(This, V_BSTR(value));

    return hr;
}

HRESULT node_put_value_escaped(xmlnode *This, VARIANT *value)
{
    HRESULT hr;

    if (V_VT(value) != VT_BSTR)
    {
       VARIANT string_value;

        VariantInit(&string_value);
        hr = VariantChangeType(&string_value, value, 0, VT_BSTR);
        if(FAILED(hr)) {
            WARN("Couldn't convert to VT_BSTR\n");
            return hr;
        }

        hr = node_set_content_escaped(This, V_BSTR(&string_value));
        VariantClear(&string_value);
    }
    else
        hr = node_set_content_escaped(This, V_BSTR(value));

    return hr;
}

static HRESULT get_node(
    xmlnode *This,
    const char *name,
    xmlNodePtr node,
    IXMLDOMNode **out )
{
    TRACE("(%p)->(%s %p %p)\n", This, name, node, out );

    if ( !out )
        return E_INVALIDARG;

    /* if we don't have a doc, use our parent. */
    if(node && !node->doc && node->parent)
        node->doc = node->parent->doc;

    *out = create_node( node );
    if (!*out)
        return S_FALSE;
    return S_OK;
}

HRESULT node_get_parent(xmlnode *This, IXMLDOMNode **parent)
{
    return get_node( This, "parent", This->node->parent, parent );
}

HRESULT node_get_child_nodes(xmlnode *This, IXMLDOMNodeList **ret)
{
    if(!ret)
        return E_INVALIDARG;

    *ret = create_children_nodelist(This->node);
    if(!*ret)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT node_get_first_child(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "firstChild", This->node->children, ret);
}

HRESULT node_get_last_child(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "lastChild", This->node->last, ret);
}

HRESULT node_get_previous_sibling(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "previous", This->node->prev, ret);
}

HRESULT node_get_next_sibling(xmlnode *This, IXMLDOMNode **ret)
{
    return get_node(This, "next", This->node->next, ret);
}

static int node_get_inst_cnt(xmlNodePtr node)
{
    int ret = *(LONG *)&node->_private & NODE_PRIV_REFCOUNT_MASK;
    xmlNodePtr child;

    /* add attribute counts */
    if (node->type == XML_ELEMENT_NODE)
    {
        xmlAttrPtr prop = node->properties;

        while (prop)
        {
            ret += node_get_inst_cnt((xmlNodePtr)prop);
            prop = prop->next;
        }
    }

    /* add children counts */
    child = node->children;
    while (child)
    {
        ret += node_get_inst_cnt(child);
        child = child->next;
    }

    return ret;
}

int xmlnode_get_inst_cnt(xmlnode *node)
{
    return node_get_inst_cnt(node->node);
}

/* _private field holds a number of COM instances spawned from this libxml2 node
 * most significant bits are used to store information about ignorable whitespace nodes */
void xmlnode_add_ref(xmlNodePtr node)
{
    if (node->type == XML_DOCUMENT_NODE) return;
    InterlockedIncrement((LONG*)&node->_private);
}

void xmlnode_release(xmlNodePtr node)
{
    if (node->type == XML_DOCUMENT_NODE) return;
    InterlockedDecrement((LONG*)&node->_private);
}

HRESULT node_insert_before(xmlnode *This, IXMLDOMNode *new_child, const VARIANT *ref_child,
        IXMLDOMNode **ret)
{
    IXMLDOMNode *before = NULL;
    xmlnode *node_obj;
    int refcount = 0;
    xmlDocPtr doc;
    HRESULT hr;

    if(!new_child)
        return E_INVALIDARG;

    node_obj = get_node_obj(new_child);
    if(!node_obj) return E_FAIL;

    switch(V_VT(ref_child))
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        if (V_UNKNOWN(ref_child))
        {
            hr = IUnknown_QueryInterface(V_UNKNOWN(ref_child), &IID_IXMLDOMNode, (void**)&before);
            if(FAILED(hr)) return hr;
        }
        break;

    default:
        FIXME("refChild var type %x\n", V_VT(ref_child));
        return E_FAIL;
    }

    TRACE("new child %p, This->node %p\n", node_obj->node, This->node);

    if(!node_obj->node->parent)
        if(xmldoc_remove_orphan(node_obj->node->doc, node_obj->node) != S_OK)
            WARN("%p is not an orphan of %p\n", node_obj->node, node_obj->node->doc);

    refcount = xmlnode_get_inst_cnt(node_obj);

    if(before)
    {
        xmlnode *before_node_obj = get_node_obj(before);
        IXMLDOMNode_Release(before);
        if(!before_node_obj) return E_FAIL;
    }

    /* unlink from current parent first */
    if(node_obj->parent)
    {
        hr = IXMLDOMNode_removeChild(node_obj->parent, node_obj->iface, NULL);
        if (hr == S_OK) xmldoc_remove_orphan(node_obj->node->doc, node_obj->node);
    }
    doc = node_obj->node->doc;

    if(before)
    {
        xmlNodePtr new_node;
        xmlnode *before_node_obj = get_node_obj(before);

        /* refs count including subtree */
        if (doc != before_node_obj->node->doc)
            refcount = xmlnode_get_inst_cnt(node_obj);

        if (refcount) xmldoc_add_refs(before_node_obj->node->doc, refcount);
        new_node = xmlAddPrevSibling(before_node_obj->node, node_obj->node);
        if (new_node != node_obj->node)
        {
            if (refcount != 1)
                FIXME("referenced xmlNode was freed, expect crashes\n");
            xmlnode_add_ref(new_node);
            node_obj->node = new_node;
        }
        if (refcount) xmldoc_release_refs(doc, refcount);
        node_obj->parent = This->parent;
    }
    else
    {
        xmlNodePtr new_node;

        if (doc != This->node->doc)
            refcount = xmlnode_get_inst_cnt(node_obj);

        if (refcount) xmldoc_add_refs(This->node->doc, refcount);
        /* xmlAddChild doesn't unlink node from previous parent */
        xmlUnlinkNode(node_obj->node);
        new_node = xmlAddChild(This->node, node_obj->node);
        if (new_node != node_obj->node)
        {
            if (refcount != 1)
                FIXME("referenced xmlNode was freed, expect crashes\n");
            xmlnode_add_ref(new_node);
            node_obj->node = new_node;
        }
        if (refcount) xmldoc_release_refs(doc, refcount);
        node_obj->parent = This->iface;
    }

    if(ret)
    {
        IXMLDOMNode_AddRef(new_child);
        *ret = new_child;
    }

    TRACE("ret S_OK\n");
    return S_OK;
}

HRESULT node_replace_child(xmlnode *This, IXMLDOMNode *newChild, IXMLDOMNode *oldChild,
        IXMLDOMNode **ret)
{
    xmlnode *old_child, *new_child;
    xmlDocPtr leaving_doc;
    xmlNode *my_ancestor;
    int refcount = 0;

    /* Do not believe any documentation telling that newChild == NULL
       means removal. It does certainly *not* apply to msxml3! */
    if(!newChild || !oldChild)
        return E_INVALIDARG;

    if(ret)
        *ret = NULL;

    old_child = get_node_obj(oldChild);
    if(!old_child) return E_FAIL;

    if(old_child->node->parent != This->node)
    {
        WARN("childNode %p is not a child of %p\n", oldChild, This);
        return E_INVALIDARG;
    }

    new_child = get_node_obj(newChild);
    if(!new_child) return E_FAIL;

    my_ancestor = This->node;
    while(my_ancestor)
    {
        if(my_ancestor == new_child->node)
        {
            WARN("tried to create loop\n");
            return E_FAIL;
        }
        my_ancestor = my_ancestor->parent;
    }

    if(!new_child->node->parent)
        if(xmldoc_remove_orphan(new_child->node->doc, new_child->node) != S_OK)
            WARN("%p is not an orphan of %p\n", new_child->node, new_child->node->doc);

    leaving_doc = new_child->node->doc;

    if (leaving_doc != old_child->node->doc)
        refcount = xmlnode_get_inst_cnt(new_child);

    if (refcount) xmldoc_add_refs(old_child->node->doc, refcount);
    xmlReplaceNode(old_child->node, new_child->node);
    if (refcount) xmldoc_release_refs(leaving_doc, refcount);
    new_child->parent = old_child->parent;
    old_child->parent = NULL;

    xmldoc_add_orphan(old_child->node->doc, old_child->node);

    if(ret)
    {
        IXMLDOMNode_AddRef(oldChild);
        *ret = oldChild;
    }

    return S_OK;
}

HRESULT node_remove_child(xmlnode *This, IXMLDOMNode* child, IXMLDOMNode** oldChild)
{
    xmlnode *child_node;

    if(!child) return E_INVALIDARG;

    if(oldChild)
        *oldChild = NULL;

    child_node = get_node_obj(child);
    if(!child_node) return E_FAIL;

    if(child_node->node->parent != This->node)
    {
        WARN("childNode %p is not a child of %p\n", child, This);
        return E_INVALIDARG;
    }

    xmlUnlinkNode(child_node->node);
    child_node->parent = NULL;
    xmldoc_add_orphan(child_node->node->doc, child_node->node);

    if(oldChild)
    {
        IXMLDOMNode_AddRef(child);
        *oldChild = child;
    }

    return S_OK;
}

HRESULT node_append_child(xmlnode *This, IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    DOMNodeType type;
    VARIANT var;
    HRESULT hr;

    if (!child)
        return E_INVALIDARG;

    hr = IXMLDOMNode_get_nodeType(child, &type);
    if(FAILED(hr) || type == NODE_ATTRIBUTE) {
        if (outChild) *outChild = NULL;
        return E_FAIL;
    }

    VariantInit(&var);
    return IXMLDOMNode_insertBefore(This->iface, child, var, outChild);
}

HRESULT node_has_childnodes(const xmlnode *This, VARIANT_BOOL *ret)
{
    if (!ret) return E_INVALIDARG;

    if (!This->node->children)
    {
        *ret = VARIANT_FALSE;
        return S_FALSE;
    }

    *ret = VARIANT_TRUE;
    return S_OK;
}

HRESULT node_get_owner_doc(const xmlnode *This, IXMLDOMDocument **doc)
{
    if(!doc)
        return E_INVALIDARG;
    return get_domdoc_from_xmldoc(This->node->doc, (IXMLDOMDocument3**)doc);
}

HRESULT node_clone(xmlnode *This, VARIANT_BOOL deep, IXMLDOMNode **cloneNode)
{
    IXMLDOMNode *node;
    xmlNodePtr clone;

    if(!cloneNode) return E_INVALIDARG;

    clone = xmlCopyNode(This->node, deep ? 1 : 2);
    if (clone)
    {
        xmlSetTreeDoc(clone, This->node->doc);
        xmldoc_add_orphan(clone->doc, clone);

        node = create_node(clone);
        if (!node)
        {
            ERR("Copy failed\n");
            xmldoc_remove_orphan(clone->doc, clone);
            xmlFreeNode(clone);
            return E_FAIL;
        }

        *cloneNode = node;
    }
    else
    {
        ERR("Copy failed\n");
        return E_FAIL;
    }

    return S_OK;
}

static xmlChar* do_get_text(xmlNodePtr node, BOOL trim, DWORD *first, DWORD *last, BOOL *trail_ig_ws)
{
    xmlNodePtr child;
    xmlChar* str;
    BOOL preserving = is_preserving_whitespace(node);

    *first = -1;
    *last = 0;

    if (!node->children)
    {
        str = xmlNodeGetContent(node);
        *trail_ig_ws = *(DWORD*)&node->_private & NODE_PRIV_CHILD_IGNORABLE_WS;
    }
    else
    {
        BOOL ig_ws = FALSE;
        xmlChar* tmp;
        DWORD pos = 0;
        str = xmlStrdup(BAD_CAST "");

        if (node->type != XML_DOCUMENT_NODE)
            ig_ws = *(DWORD*)&node->_private & NODE_PRIV_CHILD_IGNORABLE_WS;
        *trail_ig_ws = FALSE;

        for (child = node->children; child != NULL; child = child->next)
        {
            switch (child->type)
            {
            case XML_ELEMENT_NODE: {
                DWORD node_first, node_last;

                tmp = do_get_text(child, FALSE, &node_first, &node_last, trail_ig_ws);

                if (node_first!=-1 && pos+node_first<*first)
                    *first = pos+node_first;
                if (node_last && pos+node_last>*last)
                    *last = pos+node_last;
                break;
            }
            case XML_TEXT_NODE:
                tmp = xmlNodeGetContent(child);
                if (!preserving && tmp[0])
                {
                    xmlChar *beg;

                    for (beg = tmp; *beg; beg++)
                        if (!isspace(*beg)) break;

                    if (!*beg)
                    {
                        ig_ws = TRUE;
                        xmlFree(tmp);
                        tmp = NULL;
                    }
                }
                break;
            case XML_CDATA_SECTION_NODE:
            case XML_ENTITY_REF_NODE:
            case XML_ENTITY_NODE:
                tmp = xmlNodeGetContent(child);
                break;
            default:
                tmp = NULL;
                break;
            }

            if ((tmp && *tmp) || child->type==XML_CDATA_SECTION_NODE)
            {
                if (ig_ws && str[0])
                {
                    str = xmlStrcat(str, BAD_CAST " ");
                    pos++;
                }
                if (tmp && *tmp) str = xmlStrcat(str, tmp);
                if (child->type==XML_CDATA_SECTION_NODE && pos<*first)
                    *first = pos;
                if (tmp && *tmp) pos += xmlStrlen(tmp);
                if (child->type==XML_CDATA_SECTION_NODE && pos>*last)
                    *last = pos;
                ig_ws = FALSE;
            }
            if (tmp) xmlFree(tmp);

            if (!ig_ws)
            {
                ig_ws = *(DWORD*)&child->_private & NODE_PRIV_TRAILING_IGNORABLE_WS;
            }
            if (!ig_ws)
                ig_ws = *trail_ig_ws;
            *trail_ig_ws = FALSE;
        }

        *trail_ig_ws = ig_ws;
    }

    switch (node->type)
    {
    case XML_ELEMENT_NODE:
    case XML_TEXT_NODE:
    case XML_ENTITY_REF_NODE:
    case XML_ENTITY_NODE:
    case XML_DOCUMENT_NODE:
    case XML_DOCUMENT_FRAG_NODE:
        if (trim && !preserving)
        {
            xmlChar* ret;
            int len;

            if (!str)
                break;

            for (ret = str; *ret && isspace(*ret) && (*first)--; ret++)
                if (*last) (*last)--;
            for (len = xmlStrlen(ret)-1; len >= 0 && len >= *last; len--)
                if(!isspace(ret[len])) break;

            ret = xmlStrndup(ret, len+1);
            xmlFree(str);
            str = ret;
            break;
        }
        break;
    default:
        break;
    }

    return str;
}

HRESULT node_get_text(const xmlnode *This, BSTR *text)
{
    BSTR str = NULL;
    xmlChar *content;
    DWORD first, last;
    BOOL tmp;

    if (!text) return E_INVALIDARG;

    content = do_get_text(This->node, TRUE, &first, &last, &tmp);
    if (content)
    {
        str = bstr_from_xmlChar(content);
        xmlFree(content);
    }

    /* Always return a string. */
    if (!str) str = SysAllocStringLen( NULL, 0 );

    TRACE("%p %s\n", This, debugstr_w(str) );
    *text = str;
 
    return S_OK;
}

HRESULT node_put_text(xmlnode *This, BSTR text)
{
    xmlChar *str, *str2;

    TRACE("(%p)->(%s)\n", This, debugstr_w(text));

    str = xmlchar_from_wchar(text);

    /* Escape the string. */
    str2 = xmlEncodeEntitiesReentrant(This->node->doc, str);
    free(str);

    xmlNodeSetContent(This->node, str2);
    xmlFree(str2);

    return S_OK;
}

BSTR EnsureCorrectEOL(BSTR sInput)
{
    int nNum = 0;
    BSTR sNew;
    int nLen;
    int i;

    nLen = SysStringLen(sInput);
    /* Count line endings */
    for(i=0; i < nLen; i++)
    {
        if(sInput[i] == '\n')
            nNum++;
    }

    TRACE("len=%d, num=%d\n", nLen, nNum);

    /* Add linefeed as needed */
    if(nNum > 0)
    {
        int nPlace = 0;
        sNew = SysAllocStringLen(NULL, nLen + nNum);
        for(i=0; i < nLen; i++)
        {
            if(sInput[i] == '\n')
            {
                sNew[i+nPlace] = '\r';
                nPlace++;
            }
            sNew[i+nPlace] = sInput[i];
        }

        SysFreeString(sInput);
    }
    else
    {
        sNew = sInput;
    }

    TRACE("len %d\n", SysStringLen(sNew));

    return sNew;
}

/*
 * We are trying to replicate the same behaviour as msxml by converting
 * line endings to \r\n and using indents as \t. The problem is that msxml
 * only formats nodes that have a line ending. Using libxml we cannot
 * reproduce behaviour exactly.
 *
 */
HRESULT node_get_xml(xmlnode *This, BOOL ensure_eol, BSTR *ret)
{
    xmlBufferPtr xml_buf;
    xmlNodePtr xmldecl;
    int size;

    if(!ret)
        return E_INVALIDARG;

    *ret = NULL;

    xml_buf = xmlBufferCreate();
    if(!xml_buf)
        return E_OUTOFMEMORY;

    xmldecl = xmldoc_unlink_xmldecl( This->node->doc );

    size = xmlNodeDump(xml_buf, This->node->doc, This->node, 0, 1);
    if(size > 0) {
        const xmlChar *buf_content;
        BSTR content;

        /* Attribute Nodes return a space in front of their name */
        buf_content = xmlBufferContent(xml_buf);

        content = bstr_from_xmlChar(buf_content + (buf_content[0] == ' ' ? 1 : 0));
        if(ensure_eol)
            content = EnsureCorrectEOL(content);

        *ret = content;
    }else {
        *ret = SysAllocStringLen(NULL, 0);
    }

    xmlBufferFree(xml_buf);
    xmldoc_link_xmldecl( This->node->doc, xmldecl );
    return *ret ? S_OK : E_OUTOFMEMORY;
}

/* duplicates xmlBufferWriteQuotedString() logic */
static void xml_write_quotedstring(xmlOutputBufferPtr buf, const xmlChar *string)
{
    const xmlChar *cur, *base;

    if (xmlStrchr(string, '\"'))
    {
        if (xmlStrchr(string, '\''))
        {
            xmlOutputBufferWrite(buf, 1, "\"");
            base = cur = string;

            while (*cur)
            {
                if (*cur == '"')
                {
                    if (base != cur)
                        xmlOutputBufferWrite(buf, cur-base, (const char*)base);
                    xmlOutputBufferWrite(buf, 6, "&quot;");
                    cur++;
                    base = cur;
                }
                else
                    cur++;
            }
            if (base != cur)
                xmlOutputBufferWrite(buf, cur-base, (const char*)base);
            xmlOutputBufferWrite(buf, 1, "\"");
        }
        else
        {
            xmlOutputBufferWrite(buf, 1, "\'");
            xmlOutputBufferWriteString(buf, (const char*)string);
            xmlOutputBufferWrite(buf, 1, "\'");
        }
    }
    else
    {
        xmlOutputBufferWrite(buf, 1, "\"");
        xmlOutputBufferWriteString(buf, (const char*)string);
        xmlOutputBufferWrite(buf, 1, "\"");
    }
}

static int XMLCALL transform_to_stream_write(void *context, const char *buffer, int len)
{
    DWORD written;
    HRESULT hr = ISequentialStream_Write((ISequentialStream *)context, buffer, len, &written);
    return hr == S_OK ? written : -1;
}

/* Output for method "text" */
static void transform_write_text(xmlDocPtr result, xsltStylesheetPtr style, xmlOutputBufferPtr output)
{
    xmlNodePtr cur = result->children;
    while (cur)
    {
        if (cur->type == XML_TEXT_NODE)
            xmlOutputBufferWriteString(output, (const char*)cur->content);

        /* skip to next node */
        if (cur->children)
        {
            if ((cur->children->type != XML_ENTITY_DECL) &&
                (cur->children->type != XML_ENTITY_REF_NODE) &&
                (cur->children->type != XML_ENTITY_NODE))
            {
                cur = cur->children;
                continue;
            }
        }

        if (cur->next) {
            cur = cur->next;
            continue;
        }

        do
        {
            cur = cur->parent;
            if (cur == NULL)
                break;
            if (cur == (xmlNodePtr) style->doc) {
                cur = NULL;
                break;
            }
            if (cur->next) {
                cur = cur->next;
                break;
            }
        } while (cur);
    }
}

#undef XSLT_GET_IMPORT_PTR
#define XSLT_GET_IMPORT_PTR(res, style, name) {          \
    xsltStylesheetPtr st = style;                        \
    res = NULL;                                          \
    while (st != NULL) {                                 \
        if (st->name != NULL) { res = st->name; break; } \
        st = xsltNextImport(st);                         \
    }}

#undef XSLT_GET_IMPORT_INT
#define XSLT_GET_IMPORT_INT(res, style, name) {         \
    xsltStylesheetPtr st = style;                       \
    res = -1;                                           \
    while (st != NULL) {                                \
        if (st->name != -1) { res = st->name; break; }  \
        st = xsltNextImport(st);                        \
    }}

static void transform_write_xmldecl(xmlDocPtr result, xsltStylesheetPtr style, BOOL omit_encoding, xmlOutputBufferPtr output)
{
    int omit_xmldecl, standalone;

    XSLT_GET_IMPORT_INT(omit_xmldecl, style, omitXmlDeclaration);
    if (omit_xmldecl == 1) return;

    XSLT_GET_IMPORT_INT(standalone, style, standalone);

    xmlOutputBufferWriteString(output, "<?xml version=");
    if (result->version)
    {
        xmlOutputBufferWriteString(output, "\"");
        xmlOutputBufferWriteString(output, (const char *)result->version);
        xmlOutputBufferWriteString(output, "\"");
    }
    else
        xmlOutputBufferWriteString(output, "\"1.0\"");

    if (!omit_encoding)
    {
        const xmlChar *encoding;

        /* default encoding is UTF-16 */
        XSLT_GET_IMPORT_PTR(encoding, style, encoding);
        xmlOutputBufferWriteString(output, " encoding=");
        xmlOutputBufferWriteString(output, "\"");
        xmlOutputBufferWriteString(output, encoding ? (const char *)encoding : "UTF-16");
        xmlOutputBufferWriteString(output, "\"");
    }

    /* standalone attribute */
    if (standalone != -1)
        xmlOutputBufferWriteString(output, standalone == 0 ? " standalone=\"no\"" : " standalone=\"yes\"");

    xmlOutputBufferWriteString(output, "?>");
}

static void htmldtd_dumpcontent(xmlOutputBufferPtr buf, xmlDocPtr doc)
{
    xmlDtdPtr cur = doc->intSubset;

    xmlOutputBufferWriteString(buf, "<!DOCTYPE ");
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    if (cur->ExternalID)
    {
        xmlOutputBufferWriteString(buf, " PUBLIC ");
        xml_write_quotedstring(buf, cur->ExternalID);
        if (cur->SystemID)
        {
            xmlOutputBufferWriteString(buf, " ");
            xml_write_quotedstring(buf, cur->SystemID);
        }
    }
    else if (cur->SystemID)
    {
        xmlOutputBufferWriteString(buf, " SYSTEM ");
        xml_write_quotedstring(buf, cur->SystemID);
    }
    xmlOutputBufferWriteString(buf, ">\n");
}

/* Duplicates htmlDocContentDumpFormatOutput() the way we need it - doesn't add trailing newline. */
static void htmldoc_dumpcontent(xmlOutputBufferPtr buf, xmlDocPtr doc, const char *encoding, int format)
{
    xmlElementType type;

    /* force HTML output */
    type = doc->type;
    doc->type = XML_HTML_DOCUMENT_NODE;
    if (doc->intSubset)
        htmldtd_dumpcontent(buf, doc);
    if (doc->children) {
        xmlNodePtr cur = doc->children;
        while (cur) {
            htmlNodeDumpFormatOutput(buf, doc, cur, encoding, format);
            cur = cur->next;
        }
    }
    doc->type = type;
}

static inline BOOL transform_is_empty_resultdoc(xmlDocPtr result)
{
    return !result->children || ((result->children->type == XML_DTD_NODE) && !result->children->next);
}

static inline BOOL transform_is_valid_method(xsltStylesheetPtr style)
{
    return !style->methodURI || !(style->method && xmlStrEqual(style->method, (const xmlChar *)"xhtml"));
}

/* Helper to write transformation result to specified output buffer. */
static HRESULT node_transform_write(xsltStylesheetPtr style, xmlDocPtr result, BOOL omit_encoding, const char *encoding, xmlOutputBufferPtr output)
{
    const xmlChar *method;
    int indent;

    if (!transform_is_valid_method(style))
    {
        ERR("unknown output method\n");
        return E_FAIL;
    }

    XSLT_GET_IMPORT_PTR(method, style, method)
    XSLT_GET_IMPORT_INT(indent, style, indent);

    if (!method && (result->type == XML_HTML_DOCUMENT_NODE))
        method = (const xmlChar *) "html";

    if (method && xmlStrEqual(method, (const xmlChar *)"html"))
    {
        htmlSetMetaEncoding(result, (const xmlChar *)encoding);
        if (indent == -1)
            indent = 1;
        htmldoc_dumpcontent(output, result, encoding, indent);
    }
    else if (method && xmlStrEqual(method, (const xmlChar *)"xhtml"))
    {
        htmlSetMetaEncoding(result, (const xmlChar *) encoding);
        htmlDocContentDumpOutput(output, result, encoding);
    }
    else if (method && xmlStrEqual(method, (const xmlChar *)"text"))
        transform_write_text(result, style, output);
    else
    {
        transform_write_xmldecl(result, style, omit_encoding, output);

        if (result->children)
        {
            xmlNodePtr child = result->children;

            while (child)
            {
                xmlNodeDumpOutput(output, result, child, 0, indent == 1, encoding);
                if (indent && ((child->type == XML_DTD_NODE) || ((child->type == XML_COMMENT_NODE) && child->next)))
                    xmlOutputBufferWriteString(output, "\r\n");
                child = child->next;
            }
        }
    }

    xmlOutputBufferFlush(output);
    return S_OK;
}

/* For BSTR output is always UTF-16, without 'encoding' attribute */
static HRESULT node_transform_write_to_bstr(xsltStylesheetPtr style, xmlDocPtr result, BSTR *str)
{
    HRESULT hr = S_OK;

    if (transform_is_empty_resultdoc(result))
        *str = SysAllocStringLen(NULL, 0);
    else
    {
        xmlOutputBufferPtr output = xmlAllocOutputBuffer(xmlFindCharEncodingHandler("UTF-16"));
        const xmlChar *content;
        size_t len;

        *str = NULL;
        if (!output)
            return E_OUTOFMEMORY;

        hr = node_transform_write(style, result, TRUE, "UTF-16", output);
        content = xmlBufContent(output->conv);
        len = xmlBufUse(output->conv);
        /* UTF-16 encoder places UTF-16 bom, we don't need it for BSTR */
        content += sizeof(WCHAR);
        *str = SysAllocStringLen((WCHAR*)content, len/sizeof(WCHAR) - 1);
        xmlOutputBufferClose(output);
    }

    return *str ? hr : E_OUTOFMEMORY;
}

static HRESULT node_transform_write_to_stream(xsltStylesheetPtr style, xmlDocPtr result, ISequentialStream *stream)
{
    static const xmlChar *utf16 = (const xmlChar*)"UTF-16";
    xmlOutputBufferPtr output;
    const xmlChar *encoding;
    HRESULT hr;

    if (transform_is_empty_resultdoc(result))
    {
        WARN("empty result document\n");
        return S_OK;
    }

    if (style->methodURI && (!style->method || !xmlStrEqual(style->method, (const xmlChar *) "xhtml")))
    {
        ERR("unknown output method\n");
        return E_FAIL;
    }

    /* default encoding is UTF-16 */
    XSLT_GET_IMPORT_PTR(encoding, style, encoding);
    if (!encoding)
        encoding = utf16;

    output = xmlOutputBufferCreateIO(transform_to_stream_write, NULL, stream, xmlFindCharEncodingHandler((const char*)encoding));
    if (!output)
        return E_OUTOFMEMORY;

    hr = node_transform_write(style, result, FALSE, (const char*)encoding, output);
    xmlOutputBufferClose(output);
    return hr;
}

struct import_buffer
{
    char *data;
    int cur;
    int len;
};

static int XMLCALL import_loader_io_read(void *context, char *out, int len)
{
    struct import_buffer *buffer = (struct import_buffer *)context;

    TRACE("%p, %p, %d\n", context, out, len);

    if (buffer->cur == buffer->len)
        return 0;

    len = min(len, buffer->len - buffer->cur);
    memcpy(out, &buffer->data[buffer->cur], len);
    buffer->cur += len;

    TRACE("read %d\n", len);

    return len;
}

static int XMLCALL import_loader_io_close(void * context)
{
    struct import_buffer *buffer = (struct import_buffer *)context;

    TRACE("%p\n", context);

    free(buffer->data);
    free(buffer);
    return 0;
}

static HRESULT import_loader_onDataAvailable(void *ctxt, char *ptr, DWORD len)
{
    xmlParserInputPtr *input = (xmlParserInputPtr *)ctxt;
    xmlParserInputBufferPtr inputbuffer;
    struct import_buffer *buffer;

    buffer = malloc(sizeof(*buffer));

    buffer->data = malloc(len);
    memcpy(buffer->data, ptr, len);
    buffer->cur = 0;
    buffer->len = len;

    inputbuffer = xmlParserInputBufferCreateIO(import_loader_io_read, import_loader_io_close, buffer,
            XML_CHAR_ENCODING_NONE);
    *input = xmlNewIOInputStream(NULL, inputbuffer, XML_CHAR_ENCODING_NONE);
    if (!*input)
        xmlFreeParserInputBuffer(inputbuffer);

    return *input ? S_OK : E_FAIL;
}

static HRESULT xslt_doc_get_uri(const xmlChar *uri, void *_ctxt, xsltLoadType type, IUri **doc_uri)
{
    xsltStylesheetPtr style = (xsltStylesheetPtr)_ctxt;
    IUri *href_uri;
    HRESULT hr;
    BSTR uriW;

    *doc_uri = NULL;

    uriW = bstr_from_xmlChar(uri);
    hr = CreateUri(uriW, Uri_CREATE_ALLOW_RELATIVE | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &href_uri);
    SysFreeString(uriW);
    if (FAILED(hr))
    {
        WARN("Failed to create href uri, %#lx.\n", hr);
        return hr;
    }

    if (type == XSLT_LOAD_STYLESHEET && style->doc && style->doc->name)
    {
        IUri *base_uri;
        BSTR baseuriW;

        baseuriW = bstr_from_xmlChar((xmlChar *)style->doc->name);
        hr = CreateUri(baseuriW, Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, &base_uri);
        SysFreeString(baseuriW);
        if (FAILED(hr))
        {
            WARN("Failed to create base uri, %#lx.\n", hr);
            return hr;
        }

        hr = CoInternetCombineIUri(base_uri, href_uri, 0, doc_uri, 0);
        IUri_Release(base_uri);
        if (FAILED(hr))
            WARN("Failed to combine uris, hr %#lx.\n", hr);
    }
    else
    {
        *doc_uri = href_uri;
        IUri_AddRef(*doc_uri);
    }

    IUri_Release(href_uri);

    return hr;
}

xmlDocPtr xslt_doc_default_loader(const xmlChar *uri, xmlDictPtr dict, int options,
    void *_ctxt, xsltLoadType type)
{
    IUri *import_uri = NULL;
    xmlParserInputPtr input;
    xmlParserCtxtPtr pctxt;
    xmlDocPtr doc = NULL;
    IMoniker *moniker;
    HRESULT hr;
    bsc_t *bsc;
    BSTR uriW;

    TRACE("%s, %p, %#x, %p, %d\n", debugstr_a((const char *)uri), dict, options, _ctxt, type);

    pctxt = xmlNewParserCtxt();
    if (!pctxt)
        return NULL;

    if (dict && pctxt->dict)
    {
        xmlDictFree(pctxt->dict);
        pctxt->dict = NULL;
    }

    if (dict)
    {
        pctxt->dict = dict;
        xmlDictReference(pctxt->dict);
    }

    xmlCtxtUseOptions(pctxt, options);

    hr = xslt_doc_get_uri(uri, _ctxt, type, &import_uri);
    if (FAILED(hr))
        goto failed;

    hr = CreateURLMonikerEx2(NULL, import_uri, &moniker, 0);
    if (FAILED(hr))
        goto failed;

    hr = bind_url(moniker, import_loader_onDataAvailable, &input, &bsc);
    IMoniker_Release(moniker);
    if (FAILED(hr))
        goto failed;

    if (FAILED(detach_bsc(bsc)))
        goto failed;

    if (!input)
        goto failed;

    inputPush(pctxt, input);
    xmlParseDocument(pctxt);

    if (pctxt->wellFormed)
    {
        doc = pctxt->myDoc;
        /* Set imported uri, to give nested imports a chance. */
        if (IUri_GetPropertyBSTR(import_uri, Uri_PROPERTY_ABSOLUTE_URI, &uriW, 0) == S_OK)
        {
            doc->name = (char *)xmlchar_from_wcharn(uriW, SysStringLen(uriW), TRUE);
            SysFreeString(uriW);
        }
    }
    else
    {
        doc = NULL;
        xmlFreeDoc(pctxt->myDoc);
        pctxt->myDoc = NULL;
    }

failed:
    xmlFreeParserCtxt(pctxt);
    if (import_uri)
        IUri_Release(import_uri);

    return doc;
}

HRESULT node_transform_node_params(const xmlnode *This, IXMLDOMNode *stylesheet, BSTR *p,
    ISequentialStream *stream, const struct xslprocessor_params *params)
{
    xsltStylesheetPtr xsltSS;
    xmlDocPtr sheet_doc;
    HRESULT hr = S_OK;
    xmlnode *sheet;

    if (!stylesheet || (!p && !stream)) return E_INVALIDARG;

    if (p) *p = NULL;

    sheet = get_node_obj(stylesheet);
    if(!sheet) return E_FAIL;

    sheet_doc = xmlCopyDoc(sheet->node->doc, 1);
    xsltSS = xsltParseStylesheetDoc(sheet_doc);
    if (xsltSS)
    {
        const char **xslparams = NULL;
        xmlDocPtr result;
        unsigned int i;

        /* convert our parameter list to libxml2 format */
        if (params && params->count)
        {
            struct xslprocessor_par *par;

            i = 0;
            xslparams = malloc((params->count * 2 + 1) * sizeof(char*));
            LIST_FOR_EACH_ENTRY(par, &params->list, struct xslprocessor_par, entry)
            {
                xslparams[i++] = (char*)xmlchar_from_wchar(par->name);
                xslparams[i++] = (char*)xmlchar_from_wchar(par->value);
            }
            xslparams[i] = NULL;
        }

        if (xslparams)
        {
            xsltTransformContextPtr ctxt = xsltNewTransformContext(xsltSS, This->node->doc);

            /* push parameters to user context */
            xsltQuoteUserParams(ctxt, xslparams);
            result = xsltApplyStylesheetUser(xsltSS, This->node->doc, NULL, NULL, NULL, ctxt);
            xsltFreeTransformContext(ctxt);

            for (i = 0; i < params->count*2; i++)
                free((char*)xslparams[i]);
            free(xslparams);
        }
        else
            result = xsltApplyStylesheet(xsltSS, This->node->doc, NULL);

        if (result)
        {
            if (stream)
                hr = node_transform_write_to_stream(xsltSS, result, stream);
            else
                hr = node_transform_write_to_bstr(xsltSS, result, p);
            xmlFreeDoc(result);
        }

        xsltFreeStylesheet(xsltSS);
    }
    else
        xmlFreeDoc(sheet_doc);

    if (p && !*p) *p = SysAllocStringLen(NULL, 0);

    return hr;
}

HRESULT node_transform_node(const xmlnode *node, IXMLDOMNode *stylesheet, BSTR *p)
{
    return node_transform_node_params(node, stylesheet, p, NULL, NULL);
}

HRESULT node_select_nodes(const xmlnode *This, BSTR query, IXMLDOMNodeList **nodes)
{
    xmlChar* str;
    HRESULT hr;

    if (!query || !nodes) return E_INVALIDARG;

    str = xmlchar_from_wchar(query);
    hr = create_selection(This->node, str, nodes);
    free(str);

    return hr;
}

HRESULT node_select_singlenode(const xmlnode *This, BSTR query, IXMLDOMNode **node)
{
    IXMLDOMNodeList *list;
    HRESULT hr;

    if (node)
        *node = NULL;

    hr = node_select_nodes(This, query, &list);
    if (hr == S_OK)
    {
        hr = IXMLDOMNodeList_nextNode(list, node);
        IXMLDOMNodeList_Release(list);
    }
    return hr;
}

HRESULT node_get_namespaceURI(xmlnode *This, BSTR *namespaceURI)
{
    xmlNsPtr ns = This->node->ns;

    if(!namespaceURI)
        return E_INVALIDARG;

    *namespaceURI = NULL;

    if (ns && ns->href)
        *namespaceURI = bstr_from_xmlChar(ns->href);

    TRACE("uri: %s\n", debugstr_w(*namespaceURI));

    return *namespaceURI ? S_OK : S_FALSE;
}

HRESULT node_get_prefix(xmlnode *This, BSTR *prefix)
{
    xmlNsPtr ns = This->node->ns;

    if (!prefix) return E_INVALIDARG;

    *prefix = NULL;

    if (ns && ns->prefix)
        *prefix = bstr_from_xmlChar(ns->prefix);

    TRACE("prefix: %s\n", debugstr_w(*prefix));

    return *prefix ? S_OK : S_FALSE;
}

HRESULT node_get_base_name(xmlnode *This, BSTR *name)
{
    if (!name) return E_INVALIDARG;

    if (xmldoc_version(This->node->doc) != MSXML6 &&
        xmlStrEqual(This->node->name, BAD_CAST "xmlns"))
        *name = SysAllocString(L"");
    else
        *name = bstr_from_xmlChar(This->node->name);
    if (!*name) return E_OUTOFMEMORY;

    TRACE("returning %s\n", debugstr_w(*name));

    return S_OK;
}

void destroy_xmlnode(xmlnode *This)
{
    if(This->node)
    {
        xmlnode_release(This->node);
        xmldoc_release(This->node->doc);
    }
}

void init_xmlnode(xmlnode *This, xmlNodePtr node, IXMLDOMNode *node_iface, dispex_static_data_t *dispex_data)
{
    if(node)
    {
        xmlnode_add_ref(node);
        xmldoc_add_ref(node->doc);
    }

    This->node = node;
    This->iface = node_iface;
    This->parent = NULL;

    init_dispex(&This->dispex, (IUnknown*)This->iface, dispex_data);
}

typedef struct {
    xmlnode node;
    IXMLDOMNode IXMLDOMNode_iface;
    LONG ref;
} unknode;

static inline unknode *unknode_from_IXMLDOMNode(IXMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, unknode, IXMLDOMNode_iface);
}

static HRESULT WINAPI unknode_QueryInterface(
    IXMLDOMNode *iface,
    REFIID riid,
    void** ppvObject )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown)) {
        *ppvObject = iface;
    }else if (IsEqualGUID( riid, &IID_IDispatch) ||
              IsEqualGUID( riid, &IID_IXMLDOMNode)) {
        *ppvObject = &This->IXMLDOMNode_iface;
    }else if(node_query_interface(&This->node, riid, ppvObject)) {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }else  {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);
    return S_OK;
}

static ULONG WINAPI unknode_AddRef(
    IXMLDOMNode *iface )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI unknode_Release(
    IXMLDOMNode *iface )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    LONG ref;

    ref = InterlockedDecrement( &This->ref );
    if(!ref) {
        destroy_xmlnode(&This->node);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI unknode_GetTypeInfoCount(
    IXMLDOMNode *iface,
    UINT* pctinfo )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI unknode_GetTypeInfo(
    IXMLDOMNode *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IXMLDOMNode_tid, ppTInfo);
}

static HRESULT WINAPI unknode_GetIDsOfNames(
    IXMLDOMNode *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMNode_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI unknode_Invoke(
    IXMLDOMNode *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMNode_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI unknode_get_nodeName(
    IXMLDOMNode *iface,
    BSTR* p )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI unknode_get_nodeValue(
    IXMLDOMNode *iface,
    VARIANT* value)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, value);

    if(!value)
        return E_INVALIDARG;

    V_VT(value) = VT_NULL;
    return S_FALSE;
}

static HRESULT WINAPI unknode_put_nodeValue(
    IXMLDOMNode *iface,
    VARIANT value)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(v%d)\n", This, V_VT(&value));
    return E_FAIL;
}

static HRESULT WINAPI unknode_get_nodeType(
    IXMLDOMNode *iface,
    DOMNodeType* domNodeType )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, domNodeType);

    switch (This->node.node->type)
    {
    case XML_ELEMENT_NODE:
    case XML_ATTRIBUTE_NODE:
    case XML_TEXT_NODE:
    case XML_CDATA_SECTION_NODE:
    case XML_ENTITY_REF_NODE:
    case XML_ENTITY_NODE:
    case XML_PI_NODE:
    case XML_COMMENT_NODE:
    case XML_DOCUMENT_NODE:
    case XML_DOCUMENT_TYPE_NODE:
    case XML_DOCUMENT_FRAG_NODE:
    case XML_NOTATION_NODE:
        /* we only care about this set of types, libxml2 type values are
           exactly what we need */
        *domNodeType = (DOMNodeType)This->node.node->type;
        break;
    default:
        *domNodeType = NODE_INVALID;
        break;
    }

    return S_OK;
}

static HRESULT WINAPI unknode_get_parentNode(
    IXMLDOMNode *iface,
    IXMLDOMNode** parent )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p)\n", This, parent);
    if (!parent) return E_INVALIDARG;
    *parent = NULL;
    return S_FALSE;
}

static HRESULT WINAPI unknode_get_childNodes(
    IXMLDOMNode *iface,
    IXMLDOMNodeList** outList)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI unknode_get_firstChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_lastChild(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_previousSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_nextSibling(
    IXMLDOMNode *iface,
    IXMLDOMNode** domNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI unknode_get_attributes(
    IXMLDOMNode *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI unknode_insertBefore(
    IXMLDOMNode *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p x%d %p)\n", This, newNode, V_VT(&refChild), outOldNode);

    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI unknode_replaceChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p %p %p)\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI unknode_removeChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_remove_child(&This->node, domNode, oldNode);
}

static HRESULT WINAPI unknode_appendChild(
    IXMLDOMNode *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_append_child(&This->node, newNode, outNewNode);
}

static HRESULT WINAPI unknode_hasChildNodes(
    IXMLDOMNode *iface,
    VARIANT_BOOL* pbool)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_has_childnodes(&This->node, pbool);
}

static HRESULT WINAPI unknode_get_ownerDocument(
    IXMLDOMNode *iface,
    IXMLDOMDocument** domDocument)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_get_owner_doc(&This->node, domDocument);
}

static HRESULT WINAPI unknode_cloneNode(
    IXMLDOMNode *iface,
    VARIANT_BOOL pbool, IXMLDOMNode** outNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_clone(&This->node, pbool, outNode );
}

static HRESULT WINAPI unknode_get_nodeTypeString(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI unknode_get_text(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI unknode_put_text(
    IXMLDOMNode *iface,
    BSTR p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_put_text(&This->node, p);
}

static HRESULT WINAPI unknode_get_specified(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isSpecified)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI unknode_get_definition(
    IXMLDOMNode *iface,
    IXMLDOMNode** definitionNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI unknode_get_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT* var1)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p)\n", This, var1);
    return return_null_var(var1);
}

static HRESULT WINAPI unknode_put_nodeTypedValue(
    IXMLDOMNode *iface,
    VARIANT typedValue)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI unknode_get_dataType(
    IXMLDOMNode *iface,
    VARIANT* var1)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    TRACE("(%p)->(%p)\n", This, var1);
    return return_null_var(var1);
}

static HRESULT WINAPI unknode_put_dataType(
    IXMLDOMNode *iface,
    BSTR p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI unknode_get_xml(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    FIXME("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI unknode_transformNode(
    IXMLDOMNode *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_transform_node(&This->node, domNode, p);
}

static HRESULT WINAPI unknode_selectNodes(
    IXMLDOMNode *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI unknode_selectSingleNode(
    IXMLDOMNode *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI unknode_get_parsed(
    IXMLDOMNode *iface,
    VARIANT_BOOL* isParsed)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI unknode_get_namespaceURI(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI unknode_get_prefix(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_get_prefix(&This->node, p);
}

static HRESULT WINAPI unknode_get_baseName(
    IXMLDOMNode *iface,
    BSTR* p)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    return node_get_base_name(&This->node, p);
}

static HRESULT WINAPI unknode_transformNodeToObject(
    IXMLDOMNode *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    unknode *This = unknode_from_IXMLDOMNode( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static const struct IXMLDOMNodeVtbl unknode_vtbl =
{
    unknode_QueryInterface,
    unknode_AddRef,
    unknode_Release,
    unknode_GetTypeInfoCount,
    unknode_GetTypeInfo,
    unknode_GetIDsOfNames,
    unknode_Invoke,
    unknode_get_nodeName,
    unknode_get_nodeValue,
    unknode_put_nodeValue,
    unknode_get_nodeType,
    unknode_get_parentNode,
    unknode_get_childNodes,
    unknode_get_firstChild,
    unknode_get_lastChild,
    unknode_get_previousSibling,
    unknode_get_nextSibling,
    unknode_get_attributes,
    unknode_insertBefore,
    unknode_replaceChild,
    unknode_removeChild,
    unknode_appendChild,
    unknode_hasChildNodes,
    unknode_get_ownerDocument,
    unknode_cloneNode,
    unknode_get_nodeTypeString,
    unknode_get_text,
    unknode_put_text,
    unknode_get_specified,
    unknode_get_definition,
    unknode_get_nodeTypedValue,
    unknode_put_nodeTypedValue,
    unknode_get_dataType,
    unknode_put_dataType,
    unknode_get_xml,
    unknode_transformNode,
    unknode_selectNodes,
    unknode_selectSingleNode,
    unknode_get_parsed,
    unknode_get_namespaceURI,
    unknode_get_prefix,
    unknode_get_baseName,
    unknode_transformNodeToObject
};

IXMLDOMNode *create_node( xmlNodePtr node )
{
    IUnknown *pUnk;
    IXMLDOMNode *ret;
    HRESULT hr;

    if ( !node )
        return NULL;

    TRACE("type %d\n", node->type);
    switch(node->type)
    {
    case XML_ELEMENT_NODE:
        pUnk = create_element( node );
        break;
    case XML_ATTRIBUTE_NODE:
        pUnk = create_attribute( node, FALSE );
        break;
    case XML_TEXT_NODE:
        pUnk = create_text( node );
        break;
    case XML_CDATA_SECTION_NODE:
        pUnk = create_cdata( node );
        break;
    case XML_ENTITY_REF_NODE:
        pUnk = create_doc_entity_ref( node );
        break;
    case XML_PI_NODE:
        pUnk = create_pi( node );
        break;
    case XML_COMMENT_NODE:
        pUnk = create_comment( node );
        break;
    case XML_DOCUMENT_NODE:
        pUnk = create_domdoc( node );
        break;
    case XML_DOCUMENT_FRAG_NODE:
        pUnk = create_doc_fragment( node );
        break;
    case XML_DTD_NODE:
    case XML_DOCUMENT_TYPE_NODE:
        pUnk = create_doc_type( node );
        break;
    case XML_ENTITY_NODE:
    case XML_NOTATION_NODE: {
        unknode *new_node;

        FIXME("only creating basic node for type %d\n", node->type);

        new_node = malloc(sizeof(unknode));
        if(!new_node)
            return NULL;

        new_node->IXMLDOMNode_iface.lpVtbl = &unknode_vtbl;
        new_node->ref = 1;
        init_xmlnode(&new_node->node, node, &new_node->IXMLDOMNode_iface, NULL);
        pUnk = (IUnknown*)&new_node->IXMLDOMNode_iface;
        break;
    }
    default:
        ERR("Called for unsupported node type %d\n", node->type);
        return NULL;
    }

    hr = IUnknown_QueryInterface(pUnk, &IID_IXMLDOMNode, (LPVOID*)&ret);
    IUnknown_Release(pUnk);
    if(FAILED(hr)) return NULL;
    return ret;
}
