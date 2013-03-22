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

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <config.h>

#define COBJMACROS

//#include <stdarg.h>

#ifdef HAVE_LIBXML2
//# include <libxml/parser.h>
//# include <libxml/xmlerror.h>
# include <libxml/HTMLtree.h>
# ifdef SONAME_LIBXSLT
#  ifdef HAVE_LIBXSLT_PATTERN_H
#   include <libxslt/pattern.h>
#  endif
#  ifdef HAVE_LIBXSLT_TRANSFORM_H
#   include <libxslt/transform.h>
#  endif
#  include <libxslt/xsltutils.h>
#  include <libxslt/xsltInternals.h>
# endif
#endif

#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
//#include "winnls.h"
#include <ole2.h>
#include <msxml6.h>

#include "msxml_private.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#ifdef SONAME_LIBXSLT
extern void* libxslt_handle;
# define MAKE_FUNCPTR(f) extern typeof(f) * p##f
MAKE_FUNCPTR(xsltApplyStylesheet);
MAKE_FUNCPTR(xsltCleanupGlobals);
MAKE_FUNCPTR(xsltFreeStylesheet);
MAKE_FUNCPTR(xsltParseStylesheetDoc);
# undef MAKE_FUNCPTR
#endif

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
    TRACE("(%p)->(%d)\n", This, ref );
    return ref;
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    SupportErrorInfo *This = impl_from_ISupportErrorInfo(iface);
    enum tid_t const *tid;

    TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));

    tid = This->iids;
    while (*tid)
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

    This = heap_alloc(sizeof(*This));
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

    hr = node_get_prefix(This, &prefix);
    if (hr == S_OK)
    {
        static const WCHAR colW = ':';
        WCHAR *ptr;

        /* +1 for ':' */
        ptr = *name = SysAllocStringLen(NULL, SysStringLen(base) + SysStringLen(prefix) + 1);
        memcpy(ptr, prefix, SysStringByteLen(prefix));
        ptr += SysStringLen(prefix);
        memcpy(ptr++, &colW, sizeof(WCHAR));
        memcpy(ptr, base, SysStringByteLen(base));

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
    heap_free(str);
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
        heap_free(str);
        return E_OUTOFMEMORY;
    }

    xmlNodeSetContent(This->node, escaped);

    heap_free(str);
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
    int ret = *(LONG *)&node->_private;
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

        /* unlink from current parent first */
        if(node_obj->parent)
        {
            hr = IXMLDOMNode_removeChild(node_obj->parent, node_obj->iface, NULL);
            if (hr == S_OK) xmldoc_remove_orphan(node_obj->node->doc, node_obj->node);
        }

        doc = node_obj->node->doc;

        /* refs count including subtree */
        if (doc != before_node_obj->node->doc)
            refcount = xmlnode_get_inst_cnt(node_obj);

        if (refcount) xmldoc_add_refs(before_node_obj->node->doc, refcount);
        xmlAddPrevSibling(before_node_obj->node, node_obj->node);
        if (refcount) xmldoc_release_refs(doc, refcount);
        node_obj->parent = This->parent;
    }
    else
    {
        /* unlink from current parent first */
        if(node_obj->parent)
        {
            hr = IXMLDOMNode_removeChild(node_obj->parent, node_obj->iface, NULL);
            if (hr == S_OK) xmldoc_remove_orphan(node_obj->node->doc, node_obj->node);
        }
        doc = node_obj->node->doc;

        if (doc != This->node->doc)
            refcount = xmlnode_get_inst_cnt(node_obj);

        if (refcount) xmldoc_add_refs(This->node->doc, refcount);
        /* xmlAddChild doesn't unlink node from previous parent */
        xmlUnlinkNode(node_obj->node);
        xmlAddChild(This->node, node_obj->node);
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

static inline xmlChar* trim_whitespace(xmlChar* str)
{
    xmlChar* ret = str;
    int len;

    if (!str)
        return NULL;

    while (*ret && isspace(*ret))
        ++ret;
    len = xmlStrlen(ret);
    if (len)
        while (isspace(ret[len-1])) --len;

    ret = xmlStrndup(ret, len);
    xmlFree(str);
    return ret;
}

static xmlChar* do_get_text(xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar* str;
    BOOL preserving = is_preserving_whitespace(node);

    if (!node->children)
    {
        str = xmlNodeGetContent(node);
    }
    else
    {
        xmlElementType prev_type = XML_TEXT_NODE;
        xmlChar* tmp;
        str = xmlStrdup(BAD_CAST "");
        for (child = node->children; child != NULL; child = child->next)
        {
            switch (child->type)
            {
            case XML_ELEMENT_NODE:
                tmp = do_get_text(child);
                break;
            case XML_TEXT_NODE:
            case XML_CDATA_SECTION_NODE:
            case XML_ENTITY_REF_NODE:
            case XML_ENTITY_NODE:
                tmp = xmlNodeGetContent(child);
                break;
            default:
                tmp = NULL;
                break;
            }

            if (tmp)
            {
                if (*tmp)
                {
                    if (prev_type == XML_ELEMENT_NODE && child->type == XML_ELEMENT_NODE)
                        str = xmlStrcat(str, BAD_CAST " ");
                    str = xmlStrcat(str, tmp);
                    prev_type = child->type;
                }
                xmlFree(tmp);
            }
        }
    }

    switch (node->type)
    {
    case XML_ELEMENT_NODE:
    case XML_TEXT_NODE:
    case XML_ENTITY_REF_NODE:
    case XML_ENTITY_NODE:
    case XML_DOCUMENT_NODE:
    case XML_DOCUMENT_FRAG_NODE:
        if (!preserving)
            str = trim_whitespace(str);
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

    if (!text) return E_INVALIDARG;

    content = do_get_text(This->node);
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
    heap_free(str);

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

static void htmldtd_dumpcontent(xmlOutputBufferPtr buf, xmlDocPtr doc)
{
    xmlDtdPtr cur = doc->intSubset;

    xmlOutputBufferWriteString(buf, "<!DOCTYPE ");
    xmlOutputBufferWriteString(buf, (const char *)cur->name);
    if (cur->ExternalID)
    {
        xmlOutputBufferWriteString(buf, " PUBLIC ");
        xmlBufferWriteQuotedString(buf->buffer, cur->ExternalID);
        if (cur->SystemID)
        {
            xmlOutputBufferWriteString(buf, " ");
            xmlBufferWriteQuotedString(buf->buffer, cur->SystemID);
	}
    }
    else if (cur->SystemID)
    {
        xmlOutputBufferWriteString(buf, " SYSTEM ");
        xmlBufferWriteQuotedString(buf->buffer, cur->SystemID);
    }
    xmlOutputBufferWriteString(buf, ">\n");
}

static void htmldoc_dumpcontent(xmlOutputBufferPtr buf, xmlDocPtr doc)
{
    xmlElementType type;

    /* force HTML output */
    type = doc->type;
    doc->type = XML_HTML_DOCUMENT_NODE;
    if (doc->intSubset)
        htmldtd_dumpcontent(buf, doc);
    if (doc->children)
    {
        xmlNodePtr cur = doc->children;

        while (cur)
        {
            htmlNodeDumpFormatOutput(buf, doc, cur, NULL, 1);
            cur = cur->next;
        }

    }
    doc->type = type;
}

HRESULT node_transform_node(const xmlnode *This, IXMLDOMNode *stylesheet, BSTR *p)
{
#ifdef SONAME_LIBXSLT
    xsltStylesheetPtr xsltSS;
    xmlnode *sheet;

    if (!libxslt_handle) return E_NOTIMPL;
    if (!stylesheet || !p) return E_INVALIDARG;

    *p = NULL;

    sheet = get_node_obj(stylesheet);
    if(!sheet) return E_FAIL;

    xsltSS = pxsltParseStylesheetDoc(sheet->node->doc);
    if(xsltSS)
    {
        xmlDocPtr result = pxsltApplyStylesheet(xsltSS, This->node->doc, NULL);
        if(result)
        {
            const xmlChar *content;

            if(result->type == XML_HTML_DOCUMENT_NODE)
            {
                xmlOutputBufferPtr output = xmlAllocOutputBuffer(NULL);
                if (output)
                {
                    htmldoc_dumpcontent(output, result->doc);
                    content = xmlBufferContent(output->buffer);
                    *p = bstr_from_xmlChar(content);
                    xmlOutputBufferClose(output);
                }
            }
            else
            {
                xmlBufferPtr buf = xmlBufferCreate();
                if (buf)
                {
                    int size = xmlNodeDump(buf, NULL, (xmlNodePtr)result, 0, 0);
                    if(size > 0)
                    {
                        content = xmlBufferContent(buf);
                        *p = bstr_from_xmlChar(content);
                    }
                    xmlBufferFree(buf);
                }
            }
            xmlFreeDoc(result);
        }
        /* libxslt "helpfully" frees the XML document the stylesheet was
           generated from, too */
        xsltSS->doc = NULL;
        pxsltFreeStylesheet(xsltSS);
    }

    if(!*p) *p = SysAllocStringLen(NULL, 0);

    return S_OK;
#else
    FIXME("libxslt headers were not found at compile time\n");
    return E_NOTIMPL;
#endif
}

HRESULT node_select_nodes(const xmlnode *This, BSTR query, IXMLDOMNodeList **nodes)
{
    xmlChar* str;
    HRESULT hr;

    if (!query || !nodes) return E_INVALIDARG;

    str = xmlchar_from_wchar(query);
    hr = create_selection(This->node, str, nodes);
    heap_free(str);

    return hr;
}

HRESULT node_select_singlenode(const xmlnode *This, BSTR query, IXMLDOMNode **node)
{
    IXMLDOMNodeList *list;
    HRESULT hr;

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

    *name = bstr_from_xmlChar(This->node->name);
    if (!*name) return E_OUTOFMEMORY;

    TRACE("returning %s\n", debugstr_w(*name));

    return S_OK;
}

/* _private field holds a number of COM instances spawned from this libxml2 node */
static void xmlnode_add_ref(xmlNodePtr node)
{
    if (node->type == XML_DOCUMENT_NODE) return;
    InterlockedIncrement((LONG*)&node->_private);
}

static void xmlnode_release(xmlNodePtr node)
{
    if (node->type == XML_DOCUMENT_NODE) return;
    InterlockedDecrement((LONG*)&node->_private);
}

void destroy_xmlnode(xmlnode *This)
{
    if(This->node)
    {
        xmlnode_release(This->node);
        xmldoc_release(This->node->doc);
    }
    release_dispex(&This->dispex);
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
        heap_free(This);
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
    unknode *This = unknode_from_IXMLDOMNode( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMNode_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI unknode_GetIDsOfNames(
    IXMLDOMNode *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    unknode *This = unknode_from_IXMLDOMNode( iface );

    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
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
    unknode *This = unknode_from_IXMLDOMNode( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMNode_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IXMLDOMNode_iface, dispIdMember, wFlags, pDispParams,
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

    *domNodeType = This->node.node->type;
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
        pUnk = create_attribute( node );
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
        pUnk = create_doc_type( node );
        break;
    default: {
        unknode *new_node;

        FIXME("only creating basic node for type %d\n", node->type);

        new_node = heap_alloc(sizeof(unknode));
        if(!new_node)
            return NULL;

        new_node->IXMLDOMNode_iface.lpVtbl = &unknode_vtbl;
        new_node->ref = 1;
        init_xmlnode(&new_node->node, node, &new_node->IXMLDOMNode_iface, NULL);
        pUnk = (IUnknown*)&new_node->IXMLDOMNode_iface;
    }
    }

    hr = IUnknown_QueryInterface(pUnk, &IID_IXMLDOMNode, (LPVOID*)&ret);
    IUnknown_Release(pUnk);
    if(FAILED(hr)) return NULL;
    return ret;
}
#endif
