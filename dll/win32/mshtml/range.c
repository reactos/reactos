/*
 * Copyright 2006-2007 Jacek Caban for CodeWeavers
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
#include "mshtmcid.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    DispatchEx dispex;
    IHTMLTxtRange     IHTMLTxtRange_iface;
    IOleCommandTarget IOleCommandTarget_iface;

    nsIDOMRange *nsrange;
    HTMLDocumentNode *doc;

    struct list entry;
} HTMLTxtRange;

typedef struct {
    DispatchEx dispex;
    IHTMLDOMRange IHTMLDOMRange_iface;

    nsIDOMRange *nsrange;
} HTMLDOMRange;

typedef struct {
    WCHAR *buf;
    DWORD len;
    DWORD size;
} wstrbuf_t;

typedef struct {
    UINT16 type;
    nsIDOMNode *node;
    UINT32 off;
} rangepoint_t;

typedef enum {
    RU_UNKNOWN,
    RU_CHAR,
    RU_WORD,
    RU_SENTENCE,
    RU_TEXTEDIT
} range_unit_t;

static HTMLTxtRange *get_range_object(HTMLDocumentNode *doc, IHTMLTxtRange *iface)
{
    HTMLTxtRange *iter;

    LIST_FOR_EACH_ENTRY(iter, &doc->range_list, HTMLTxtRange, entry) {
        if(&iter->IHTMLTxtRange_iface == iface)
            return iter;
    }

    ERR("Could not find range in document\n");
    return NULL;
}

static range_unit_t string_to_unit(LPCWSTR str)
{
    if(!wcsicmp(str, L"character"))  return RU_CHAR;
    if(!wcsicmp(str, L"word"))       return RU_WORD;
    if(!wcsicmp(str, L"sentence"))   return RU_SENTENCE;
    if(!wcsicmp(str, L"textedit"))   return RU_TEXTEDIT;

    return RU_UNKNOWN;
}

static int string_to_nscmptype(LPCWSTR str)
{
    if(!wcsicmp(str, L"StartToEnd"))  return NS_START_TO_END;
    if(!wcsicmp(str, L"StartToStart"))  return NS_START_TO_START;
    if(!wcsicmp(str, L"EndToStart"))  return NS_END_TO_START;
    if(!wcsicmp(str, L"EndToEnd"))  return NS_END_TO_END;

    return -1;
}

static UINT16 get_node_type(nsIDOMNode *node)
{
    UINT16 type = 0;

    if(node)
        nsIDOMNode_GetNodeType(node, &type);

    return type;
}

static void get_text_node_data(nsIDOMNode *node, nsAString *nsstr, const PRUnichar **str)
{
    nsIDOMText *nstext;
    nsresult nsres;

    nsres = nsIDOMNode_QueryInterface(node, &IID_nsIDOMText, (void**)&nstext);
    assert(nsres == NS_OK);

    nsAString_Init(nsstr, NULL);
    nsres = nsIDOMText_GetData(nstext, nsstr);
    nsIDOMText_Release(nstext);
    if(NS_FAILED(nsres))
        ERR("GetData failed: %08lx\n", nsres);

    nsAString_GetData(nsstr, str);
}

static nsIDOMNode *get_child_node(nsIDOMNode *node, UINT32 off)
{
    nsIDOMNodeList *node_list;
    nsIDOMNode *ret = NULL;

    nsIDOMNode_GetChildNodes(node, &node_list);
    nsIDOMNodeList_Item(node_list, off, &ret);
    nsIDOMNodeList_Release(node_list);

    return ret;
}

/* This is very inefficient, but there is no faster way to compute index in
 * child node list using public API. Gecko has internal nsINode::IndexOf
 * function that we could consider exporting and use instead. */
static int get_child_index(nsIDOMNode *parent, nsIDOMNode *child)
{
    nsIDOMNodeList *node_list;
    nsIDOMNode *node;
    int ret = 0;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(parent, &node_list);
    assert(nsres == NS_OK);

    while(1) {
        nsres = nsIDOMNodeList_Item(node_list, ret, &node);
        assert(nsres == NS_OK && node);
        if(node == child) {
            nsIDOMNode_Release(node);
            break;
        }
        nsIDOMNode_Release(node);
        ret++;
    }

    nsIDOMNodeList_Release(node_list);
    return ret;
}

static void init_rangepoint_no_addref(rangepoint_t *rangepoint, nsIDOMNode *node, UINT32 off)
{
    rangepoint->type = get_node_type(node);
    rangepoint->node = node;
    rangepoint->off = off;
}

static void init_rangepoint(rangepoint_t *rangepoint, nsIDOMNode *node, UINT32 off)
{
    nsIDOMNode_AddRef(node);
    init_rangepoint_no_addref(rangepoint, node, off);
}

static inline void free_rangepoint(rangepoint_t *rangepoint)
{
    nsIDOMNode_Release(rangepoint->node);
}

static inline BOOL rangepoint_cmp(const rangepoint_t *point1, const rangepoint_t *point2)
{
    return point1->node == point2->node && point1->off == point2->off;
}

static BOOL rangepoint_next_node(rangepoint_t *iter)
{
    nsIDOMNode *node;
    UINT32 off;
    nsresult nsres;

    /* Try to move to the child node. */
    node = get_child_node(iter->node, iter->off);
    if(node) {
        free_rangepoint(iter);
        init_rangepoint_no_addref(iter, node, 0);
        return TRUE;
    }

    /* There are no more children in the node. Move to parent. */
    nsres = nsIDOMNode_GetParentNode(iter->node, &node);
    assert(nsres == NS_OK);
    if(!node)
        return FALSE;

    off = get_child_index(node, iter->node)+1;
    free_rangepoint(iter);
    init_rangepoint_no_addref(iter, node, off);
    return TRUE;
}

static UINT32 get_child_count(nsIDOMNode *node)
{
    nsIDOMNodeList *node_list;
    UINT32 ret;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(node, &node_list);
    assert(nsres == NS_OK);

    nsres = nsIDOMNodeList_GetLength(node_list, &ret);
    nsIDOMNodeList_Release(node_list);
    assert(nsres == NS_OK);

    return ret;
}

static UINT32 get_text_length(nsIDOMNode *node)
{
    nsIDOMText *nstext;
    UINT32 ret;
    nsresult nsres;

    nsres = nsIDOMNode_QueryInterface(node, &IID_nsIDOMText, (void**)&nstext);
    assert(nsres == NS_OK);

    nsres = nsIDOMText_GetLength(nstext, &ret);
    nsIDOMText_Release(nstext);
    assert(nsres == NS_OK);

    return ret;
}

static BOOL rangepoint_prev_node(rangepoint_t *iter)
{
    nsIDOMNode *node;
    UINT32 off;
    nsresult nsres;

    /* Try to move to the child node. */
    if(iter->off) {
        node = get_child_node(iter->node, iter->off-1);
        assert(node != NULL);

        off = get_node_type(node) == TEXT_NODE ? get_text_length(node) : get_child_count(node);
        free_rangepoint(iter);
        init_rangepoint_no_addref(iter, node, off);
        return TRUE;
    }

    /* There are no more children in the node. Move to parent. */
    nsres = nsIDOMNode_GetParentNode(iter->node, &node);
    assert(nsres == NS_OK);
    if(!node)
        return FALSE;

    off = get_child_index(node, iter->node);
    free_rangepoint(iter);
    init_rangepoint_no_addref(iter, node, off);
    return TRUE;
}

static void get_start_point(HTMLTxtRange *This, rangepoint_t *ret)
{
    nsIDOMNode *node;
    LONG off;

    nsIDOMRange_GetStartContainer(This->nsrange, &node);
    nsIDOMRange_GetStartOffset(This->nsrange, &off);

    init_rangepoint_no_addref(ret, node, off);
}

static void get_end_point(HTMLTxtRange *This, rangepoint_t *ret)
{
    nsIDOMNode *node;
    LONG off;

    nsIDOMRange_GetEndContainer(This->nsrange, &node);
    nsIDOMRange_GetEndOffset(This->nsrange, &off);

    init_rangepoint_no_addref(ret, node, off);
}

static void set_start_point(HTMLTxtRange *This, const rangepoint_t *start)
{
    nsresult nsres = nsIDOMRange_SetStart(This->nsrange, start->node, start->off);
    if(NS_FAILED(nsres))
        ERR("failed: %08lx\n", nsres);
}

static void set_end_point(HTMLTxtRange *This, const rangepoint_t *end)
{
    nsresult nsres = nsIDOMRange_SetEnd(This->nsrange, end->node, end->off);
    if(NS_FAILED(nsres))
        ERR("failed: %08lx\n", nsres);
}

static BOOL is_elem_tag(nsIDOMNode *node, LPCWSTR istag)
{
    nsIDOMElement *elem;
    nsAString tag_str;
    const PRUnichar *tag;
    BOOL ret = FALSE;
    nsresult nsres;

    nsres = nsIDOMNode_QueryInterface(node, &IID_nsIDOMElement, (void**)&elem);
    if(NS_FAILED(nsres))
        return FALSE;

    nsAString_Init(&tag_str, NULL);
    nsIDOMElement_GetTagName(elem, &tag_str);
    nsIDOMElement_Release(elem);
    nsAString_GetData(&tag_str, &tag);

    ret = !wcsicmp(tag, istag);

    nsAString_Finish(&tag_str);

    return ret;
}

static inline BOOL wstrbuf_init(wstrbuf_t *buf)
{
    buf->len = 0;
    buf->size = 16;
    buf->buf = malloc(buf->size * sizeof(WCHAR));
    if (!buf->buf) return FALSE;
    *buf->buf = 0;
    return TRUE;
}

static inline void wstrbuf_finish(wstrbuf_t *buf)
{
    free(buf->buf);
}

static void wstrbuf_append_len(wstrbuf_t *buf, LPCWSTR str, int len)
{
    if(buf->len+len >= buf->size) {
        buf->size = 2*buf->size+len;
        buf->buf = realloc(buf->buf, buf->size * sizeof(WCHAR));
    }

    memcpy(buf->buf+buf->len, str, len*sizeof(WCHAR));
    buf->len += len;
    buf->buf[buf->len] = 0;
}

static void wstrbuf_append_nodetxt(wstrbuf_t *buf, LPCWSTR str, int len)
{
    const WCHAR *s = str;
    WCHAR *d;

    TRACE("%s\n", debugstr_wn(str, len));

    if(buf->len+len >= buf->size) {
        buf->size = 2*buf->size+len;
        buf->buf = realloc(buf->buf, buf->size * sizeof(WCHAR));
    }

    if(buf->len && iswspace(buf->buf[buf->len-1])) {
        while(s < str+len && iswspace(*s))
            s++;
    }

    d = buf->buf+buf->len;
    while(s < str+len) {
        if(iswspace(*s)) {
            *d++ = ' ';
            s++;
            while(s < str+len && iswspace(*s))
                s++;
        }else {
            *d++ = *s++;
        }
    }

    buf->len = d - buf->buf;
    *d = 0;
}

static void wstrbuf_append_node(wstrbuf_t *buf, nsIDOMNode *node, BOOL ignore_text)
{

    switch(get_node_type(node)) {
    case TEXT_NODE: {
        nsIDOMText *nstext;
        nsAString data_str;
        const PRUnichar *data;

        if(ignore_text)
            break;

        nsIDOMNode_QueryInterface(node, &IID_nsIDOMText, (void**)&nstext);

        nsAString_Init(&data_str, NULL);
        nsIDOMText_GetData(nstext, &data_str);
        nsAString_GetData(&data_str, &data);
        wstrbuf_append_nodetxt(buf, data, lstrlenW(data));
        nsAString_Finish(&data_str);

        nsIDOMText_Release(nstext);

        break;
    }
    case ELEMENT_NODE:
        if(is_elem_tag(node, L"br")) {
            static const WCHAR endlW[] = {'\r','\n'};
            wstrbuf_append_len(buf, endlW, 2);
        }else if(is_elem_tag(node, L"hr")) {
            static const WCHAR endl2W[] = {'\r','\n','\r','\n'};
            wstrbuf_append_len(buf, endl2W, 4);
        }
    }
}

static void wstrbuf_append_node_rec(wstrbuf_t *buf, nsIDOMNode *node)
{
    nsIDOMNode *iter, *tmp;

    wstrbuf_append_node(buf, node, FALSE);

    nsIDOMNode_GetFirstChild(node, &iter);
    while(iter) {
        wstrbuf_append_node_rec(buf, iter);
        nsIDOMNode_GetNextSibling(iter, &tmp);
        nsIDOMNode_Release(iter);
        iter = tmp;
    }
}

static void range_to_string(HTMLTxtRange *This, wstrbuf_t *buf)
{
    rangepoint_t end_pos, iter;
    cpp_bool collapsed;

    nsIDOMRange_GetCollapsed(This->nsrange, &collapsed);
    if(collapsed) {
        wstrbuf_finish(buf);
        buf->buf = NULL;
        buf->size = 0;
        return;
    }

    get_end_point(This, &end_pos);
    get_start_point(This, &iter);

    do {
        if(iter.type == TEXT_NODE) {
            const PRUnichar *str;
            nsAString nsstr;

            get_text_node_data(iter.node, &nsstr, &str);

            if(iter.node == end_pos.node) {
                wstrbuf_append_nodetxt(buf, str+iter.off, end_pos.off-iter.off);
                nsAString_Finish(&nsstr);
                break;
            }

            wstrbuf_append_nodetxt(buf, str+iter.off, lstrlenW(str+iter.off));
            nsAString_Finish(&nsstr);
        }else {
            nsIDOMNode *node;

            node = get_child_node(iter.node, iter.off);
            if(node) {
                wstrbuf_append_node(buf, node, TRUE);
                nsIDOMNode_Release(node);
            }
        }

        if(!rangepoint_next_node(&iter)) {
            ERR("End of document?\n");
            break;
        }
    }while(!rangepoint_cmp(&iter, &end_pos));

    free_rangepoint(&iter);
    free_rangepoint(&end_pos);

    if(buf->len) {
        WCHAR *p;

        for(p = buf->buf+buf->len-1; p >= buf->buf && iswspace(*p); p--);

        p = wcschr(p, '\r');
        if(p)
            *p = 0;
    }
}

HRESULT get_node_text(HTMLDOMNode *node, BSTR *ret)
{
    wstrbuf_t buf;
    HRESULT hres = S_OK;

    if (!wstrbuf_init(&buf))
        return E_OUTOFMEMORY;
    wstrbuf_append_node_rec(&buf, node->nsnode);
    if(buf.buf && *buf.buf) {
        *ret = SysAllocString(buf.buf);
        if(!*ret)
            hres = E_OUTOFMEMORY;
    } else {
        *ret = NULL;
    }
    wstrbuf_finish(&buf);

    if(SUCCEEDED(hres))
        TRACE("ret %s\n", debugstr_w(*ret));
    return hres;
}

static WCHAR move_next_char(rangepoint_t *iter)
{
    rangepoint_t last_space;
    nsIDOMNode *node;
    WCHAR cspace = 0;
    const WCHAR *p;

    do {
        switch(iter->type) {
        case TEXT_NODE: {
            const PRUnichar *str;
            nsAString nsstr;
            WCHAR c;

            get_text_node_data(iter->node, &nsstr, &str);
            p = str+iter->off;
            if(!*p) {
                nsAString_Finish(&nsstr);
                break;
            }

            c = *p;
            if(iswspace(c)) {
                while(iswspace(*p))
                    p++;

                if(cspace)
                    free_rangepoint(&last_space);
                else
                    cspace = ' ';

                iter->off = p-str;
                c = *p;
                nsAString_Finish(&nsstr);
                if(!c) { /* continue to skip spaces */
                    init_rangepoint(&last_space, iter->node, iter->off);
                    break;
                }

                return cspace;
            }else {
                nsAString_Finish(&nsstr);
            }

            /* If we have a non-space char and we're skipping spaces, stop and return the last found space. */
            if(cspace) {
                free_rangepoint(iter);
                *iter = last_space;
                return cspace;
            }

            iter->off++;
            return c;
        }
        case ELEMENT_NODE:
            node = get_child_node(iter->node, iter->off);
            if(!node)
                break;

            if(is_elem_tag(node, L"br")) {
                if(cspace) {
                    nsIDOMNode_Release(node);
                    free_rangepoint(iter);
                    *iter = last_space;
                    return cspace;
                }

                cspace = '\n';
                init_rangepoint(&last_space, iter->node, iter->off+1);
            }else if(is_elem_tag(node, L"hr")) {
                nsIDOMNode_Release(node);
                if(cspace) {
                    free_rangepoint(iter);
                    *iter = last_space;
                    return cspace;
                }

                iter->off++;
                return '\n';
            }

            nsIDOMNode_Release(node);
        }
    }while(rangepoint_next_node(iter));

    if(cspace)
        free_rangepoint(&last_space);

    return cspace;
}

static WCHAR move_prev_char(rangepoint_t *iter)
{
    rangepoint_t last_space;
    nsIDOMNode *node;
    WCHAR cspace = 0;
    const WCHAR *p;

    do {
        switch(iter->type) {
        case TEXT_NODE: {
            const PRUnichar *str;
            nsAString nsstr;
            WCHAR c;

            if(!iter->off)
                break;

            get_text_node_data(iter->node, &nsstr, &str);

            p = str+iter->off-1;
            c = *p;

            if(iswspace(c)) {
                while(p > str && iswspace(*(p-1)))
                    p--;

                if(cspace)
                    free_rangepoint(&last_space);
                else
                    cspace = ' ';

                iter->off = p-str;
                nsAString_Finish(&nsstr);
                if(p == str) { /* continue to skip spaces */
                    init_rangepoint(&last_space, iter->node, iter->off);
                    break;
                }

                return cspace;
            }else {
                nsAString_Finish(&nsstr);
            }

            /* If we have a non-space char and we're skipping spaces, stop and return the last found space. */
            if(cspace) {
                free_rangepoint(iter);
                *iter = last_space;
                return cspace;
            }

            iter->off--;
            return c;
        }
        case ELEMENT_NODE:
            if(!iter->off)
                break;

            node = get_child_node(iter->node, iter->off-1);
            if(!node)
                break;

            if(is_elem_tag(node, L"br")) {
                if(cspace)
                    free_rangepoint(&last_space);
                cspace = '\n';
                init_rangepoint(&last_space, iter->node, iter->off-1);
            }else if(is_elem_tag(node, L"hr")) {
                nsIDOMNode_Release(node);
                if(cspace) {
                    free_rangepoint(iter);
                    *iter = last_space;
                    return cspace;
                }

                iter->off--;
                return '\n';
            }

            nsIDOMNode_Release(node);
        }
    }while(rangepoint_prev_node(iter));

    if(cspace) {
        free_rangepoint(iter);
        *iter = last_space;
        return cspace;
    }

    return 0;
}

static LONG move_by_chars(rangepoint_t *iter, LONG cnt)
{
    LONG ret = 0;

    if(cnt >= 0) {
        while(ret < cnt && move_next_char(iter))
            ret++;
    }else {
        while(ret > cnt && move_prev_char(iter))
            ret--;
    }

    return ret;
}

static LONG find_prev_space(rangepoint_t *iter, BOOL first_space)
{
    rangepoint_t prev;
    WCHAR c;

    init_rangepoint(&prev, iter->node, iter->off);
    c = move_prev_char(&prev);
    if(!c || (first_space && iswspace(c))) {
        free_rangepoint(&prev);
        return FALSE;
    }

    do {
        free_rangepoint(iter);
        init_rangepoint(iter, prev.node, prev.off);
        c = move_prev_char(&prev);
    }while(c && !iswspace(c));

    free_rangepoint(&prev);
    return TRUE;
}

static BOOL find_word_end(rangepoint_t *iter, BOOL is_collapsed)
{
    rangepoint_t prev_iter;
    WCHAR c;
    BOOL ret = FALSE;

    if(!is_collapsed) {
        init_rangepoint(&prev_iter, iter->node, iter->off);
        c = move_prev_char(&prev_iter);
        free_rangepoint(&prev_iter);
        if(iswspace(c))
            return FALSE;
    }

    do {
        init_rangepoint(&prev_iter, iter->node, iter->off);
        c = move_next_char(iter);
        if(c == '\n') {
            free_rangepoint(iter);
            *iter = prev_iter;
            return ret;
        }
        if(!c) {
            if(!ret)
                ret = !rangepoint_cmp(iter, &prev_iter);
        }else {
            ret = TRUE;
        }
        free_rangepoint(&prev_iter);
    }while(c && !iswspace(c));

    return ret;
}

static LONG move_by_words(rangepoint_t *iter, LONG cnt)
{
    LONG ret = 0;

    if(cnt >= 0) {
        WCHAR c;

        while(ret < cnt && (c = move_next_char(iter))) {
            if(iswspace(c))
                ret++;
        }
    }else {
        while(ret > cnt && find_prev_space(iter, FALSE))
            ret--;
    }

    return ret;
}

static inline HTMLTxtRange *impl_from_IHTMLTxtRange(IHTMLTxtRange *iface)
{
    return CONTAINING_RECORD(iface, HTMLTxtRange, IHTMLTxtRange_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLTxtRange, IHTMLTxtRange, impl_from_IHTMLTxtRange(iface)->dispex)

static HRESULT WINAPI HTMLTxtRange_get_htmlText(IHTMLTxtRange *iface, BSTR *p)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(This->nsrange) {
        nsIDOMDocumentFragment *fragment;
        nsresult nsres;

        nsres = nsIDOMRange_CloneContents(This->nsrange, &fragment);
        if(NS_SUCCEEDED(nsres)) {
            const PRUnichar *nstext;
            nsAString nsstr;

            nsAString_Init(&nsstr, NULL);
            nsnode_to_nsstring((nsIDOMNode*)fragment, &nsstr);
            nsIDOMDocumentFragment_Release(fragment);

            nsAString_GetData(&nsstr, &nstext);
            *p = SysAllocString(nstext);

            nsAString_Finish(&nsstr);
        }
    }

    if(!*p) {
        *p = SysAllocString(L"");
    }

    TRACE("return %s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_put_text(IHTMLTxtRange *iface, BSTR v)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    nsIDOMText *text_node;
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->doc)
        return MSHTML_E_NODOC;

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMDocument_CreateTextNode(This->doc->dom_document, &text_str, &text_node);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08lx\n", nsres);
        return S_OK;
    }
    nsres = nsIDOMRange_DeleteContents(This->nsrange);
    if(NS_FAILED(nsres))
        ERR("DeleteContents failed: %08lx\n", nsres);

    nsres = nsIDOMRange_InsertNode(This->nsrange, (nsIDOMNode*)text_node);
    if(NS_FAILED(nsres))
        ERR("InsertNode failed: %08lx\n", nsres);

    nsres = nsIDOMRange_SetEndAfter(This->nsrange, (nsIDOMNode*)text_node);
    if(NS_FAILED(nsres))
        ERR("SetEndAfter failed: %08lx\n", nsres);

    nsIDOMText_Release(text_node);
    return IHTMLTxtRange_collapse(&This->IHTMLTxtRange_iface, VARIANT_FALSE);
}

static HRESULT WINAPI HTMLTxtRange_get_text(IHTMLTxtRange *iface, BSTR *p)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    wstrbuf_t buf;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    if(!This->nsrange)
        return S_OK;

    if (!wstrbuf_init(&buf))
        return E_OUTOFMEMORY;
    range_to_string(This, &buf);
    if (buf.buf)
        *p = SysAllocString(buf.buf);
    wstrbuf_finish(&buf);

    TRACE("ret %s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_parentElement(IHTMLTxtRange *iface, IHTMLElement **parent)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    nsIDOMNode *nsnode, *tmp;
    HTMLDOMNode *node;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, parent);

    nsIDOMRange_GetCommonAncestorContainer(This->nsrange, &nsnode);
    while(nsnode && get_node_type(nsnode) != ELEMENT_NODE) {
        nsIDOMNode_GetParentNode(nsnode, &tmp);
        nsIDOMNode_Release(nsnode);
        nsnode = tmp;
    }

    if(!nsnode) {
        *parent = NULL;
        return S_OK;
    }

    hres = get_node(nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)parent);
    node_release(node);
    return hres;
}

static HRESULT WINAPI HTMLTxtRange_duplicate(IHTMLTxtRange *iface, IHTMLTxtRange **Duplicate)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    nsIDOMRange *nsrange = NULL;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, Duplicate);

    nsIDOMRange_CloneRange(This->nsrange, &nsrange);
    hres = HTMLTxtRange_Create(This->doc, nsrange, Duplicate);
    nsIDOMRange_Release(nsrange);

    return hres;
}

static HRESULT WINAPI HTMLTxtRange_inRange(IHTMLTxtRange *iface, IHTMLTxtRange *Range,
        VARIANT_BOOL *InRange)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    HTMLTxtRange *src_range;
    short nsret = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, Range, InRange);

    *InRange = VARIANT_FALSE;

    src_range = get_range_object(This->doc, Range);
    if(!src_range)
        return E_FAIL;

    nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_START_TO_START,
            src_range->nsrange, &nsret);
    if(NS_SUCCEEDED(nsres) && nsret <= 0) {
        nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_END_TO_END,
                src_range->nsrange, &nsret);
        if(NS_SUCCEEDED(nsres) && nsret >= 0)
            *InRange = VARIANT_TRUE;
    }

    if(NS_FAILED(nsres))
        ERR("CompareBoundaryPoints failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_isEqual(IHTMLTxtRange *iface, IHTMLTxtRange *Range,
        VARIANT_BOOL *IsEqual)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    HTMLTxtRange *src_range;
    short nsret = 0;
    nsresult nsres;

    TRACE("(%p)->(%p %p)\n", This, Range, IsEqual);

    *IsEqual = VARIANT_FALSE;

    src_range = get_range_object(This->doc, Range);
    if(!src_range)
        return E_FAIL;

    nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_START_TO_START,
            src_range->nsrange, &nsret);
    if(NS_SUCCEEDED(nsres) && !nsret) {
        nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, NS_END_TO_END,
                src_range->nsrange, &nsret);
        if(NS_SUCCEEDED(nsres) && !nsret)
            *IsEqual = VARIANT_TRUE;
    }

    if(NS_FAILED(nsres))
        ERR("CompareBoundaryPoints failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_scrollIntoView(IHTMLTxtRange *iface, VARIANT_BOOL fStart)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%x)\n", This, fStart);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_collapse(IHTMLTxtRange *iface, VARIANT_BOOL Start)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);

    TRACE("(%p)->(%x)\n", This, Start);

    nsIDOMRange_Collapse(This->nsrange, Start != VARIANT_FALSE);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_expand(IHTMLTxtRange *iface, BSTR Unit, VARIANT_BOOL *Success)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(Unit), Success);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    *Success = VARIANT_FALSE;

    switch(unit) {
    case RU_WORD: {
        rangepoint_t end, start;
        cpp_bool is_collapsed;

        get_start_point(This, &start);
        get_end_point(This, &end);

        nsIDOMRange_GetCollapsed(This->nsrange, &is_collapsed);

        if(find_word_end(&end, is_collapsed)) {
            set_end_point(This, &end);
            *Success = VARIANT_TRUE;
        }

        if(find_prev_space(&start, TRUE)) {
            set_start_point(This, &start);
            *Success = VARIANT_TRUE;
        }

        free_rangepoint(&end);
        free_rangepoint(&start);
        break;
    }

    case RU_TEXTEDIT: {
        nsIDOMHTMLElement *nsbody = NULL;
        nsresult nsres;

        if(!This->doc->html_document) {
            FIXME("Not implemented for XML document\n");
            return E_NOTIMPL;
        }

        nsres = nsIDOMHTMLDocument_GetBody(This->doc->html_document, &nsbody);
        if(NS_FAILED(nsres) || !nsbody) {
            ERR("Could not get body: %08lx\n", nsres);
            break;
        }

        nsres = nsIDOMRange_SelectNodeContents(This->nsrange, (nsIDOMNode*)nsbody);
        nsIDOMHTMLElement_Release(nsbody);
        if(NS_FAILED(nsres)) {
            ERR("Collapse failed: %08lx\n", nsres);
            break;
        }

        *Success = VARIANT_TRUE;
        break;
    }

    default:
        FIXME("Unimplemented unit %s\n", debugstr_w(Unit));
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_move(IHTMLTxtRange *iface, BSTR Unit,
        LONG Count, LONG *ActualCount)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(Unit), Count, ActualCount);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    if(!Count) {
        *ActualCount = 0;
        return IHTMLTxtRange_collapse(&This->IHTMLTxtRange_iface, VARIANT_TRUE);
    }

    switch(unit) {
    case RU_CHAR: {
        rangepoint_t start;

        get_start_point(This, &start);

        *ActualCount = move_by_chars(&start, Count);

        set_start_point(This, &start);
        IHTMLTxtRange_collapse(&This->IHTMLTxtRange_iface, VARIANT_TRUE);
        free_rangepoint(&start);
        break;
    }

    case RU_WORD: {
        rangepoint_t start;

        get_start_point(This, &start);

        *ActualCount = move_by_words(&start, Count);

        set_start_point(This, &start);
        IHTMLTxtRange_collapse(&This->IHTMLTxtRange_iface, VARIANT_TRUE);
        free_rangepoint(&start);
        break;
    }

    default:
        FIXME("unimplemented unit %s\n", debugstr_w(Unit));
    }

    TRACE("ret %ld\n", *ActualCount);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_moveStart(IHTMLTxtRange *iface, BSTR Unit,
        LONG Count, LONG *ActualCount)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(Unit), Count, ActualCount);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    if(!Count) {
        *ActualCount = 0;
        return S_OK;
    }

    switch(unit) {
    case RU_CHAR: {
        rangepoint_t start;

        get_start_point(This, &start);

        *ActualCount = move_by_chars(&start, Count);

        set_start_point(This, &start);
        free_rangepoint(&start);
        break;
    }

    default:
        FIXME("unimplemented unit %s\n", debugstr_w(Unit));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_moveEnd(IHTMLTxtRange *iface, BSTR Unit,
        LONG Count, LONG *ActualCount)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    range_unit_t unit;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(Unit), Count, ActualCount);

    unit = string_to_unit(Unit);
    if(unit == RU_UNKNOWN)
        return E_INVALIDARG;

    if(!Count) {
        *ActualCount = 0;
        return S_OK;
    }

    switch(unit) {
    case RU_CHAR: {
        rangepoint_t end;

        get_end_point(This, &end);

        *ActualCount = move_by_chars(&end, Count);

        set_end_point(This, &end);
        free_rangepoint(&end);
        break;
    }

    default:
        FIXME("unimplemented unit %s\n", debugstr_w(Unit));
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_select(IHTMLTxtRange *iface)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    nsISelection *nsselection;
    nsresult nsres;

    TRACE("(%p)\n", This);

    if(!This->doc->window) {
        FIXME("no window\n");
        return E_FAIL;
    }

    nsres = nsIDOMWindow_GetSelection(This->doc->window->dom_window, &nsselection);
    if(NS_FAILED(nsres)) {
        ERR("GetSelection failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsISelection_RemoveAllRanges(nsselection);
    nsISelection_AddRange(nsselection, This->nsrange);
    nsISelection_Release(nsselection);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_pasteHTML(IHTMLTxtRange *iface, BSTR html)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    nsIDOMDocumentFragment *doc_frag;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(html));

    nsres = nsIDOMRange_Collapse(This->nsrange, TRUE);
    assert(nsres == NS_OK);

    nsAString_InitDepend(&nsstr, html);
    nsres = nsIDOMRange_CreateContextualFragment(This->nsrange, &nsstr, &doc_frag);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("CreateContextualFragment failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMRange_InsertNode(This->nsrange, (nsIDOMNode*)doc_frag);
    nsIDOMDocumentFragment_Release(doc_frag);
    if(NS_FAILED(nsres)) {
        ERR("InsertNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMRange_Collapse(This->nsrange, FALSE);
    assert(nsres == NS_OK);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_moveToElementText(IHTMLTxtRange *iface, IHTMLElement *element)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    HTMLElement *elem;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, element);

    elem = unsafe_impl_from_IHTMLElement(element);
    if(!elem)
        return E_INVALIDARG;

    nsres = nsIDOMRange_SelectNodeContents(This->nsrange, elem->node.nsnode);
    if(NS_FAILED(nsres)) {
        ERR("SelectNodeContents failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_setEndPoint(IHTMLTxtRange *iface, BSTR how,
        IHTMLTxtRange *SourceRange)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    HTMLTxtRange *src_range;
    nsIDOMNode *ref_node;
    LONG ref_offset;
    BOOL set_start;
    int how_type;
    INT16 cmp;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(how), SourceRange);

    how_type = string_to_nscmptype(how);
    if(how_type == -1)
        return E_INVALIDARG;

    src_range = get_range_object(This->doc, SourceRange);
    if(!src_range)
        return E_FAIL;

    switch(how_type) {
    case NS_START_TO_START:
    case NS_END_TO_START:
        nsres = nsIDOMRange_GetStartContainer(src_range->nsrange, &ref_node);
        assert(nsres == NS_OK);

        nsres = nsIDOMRange_GetStartOffset(src_range->nsrange, &ref_offset);
        assert(nsres == NS_OK);

        set_start = how_type == NS_START_TO_START;
        break;
    case NS_END_TO_END:
    case NS_START_TO_END:
        nsres = nsIDOMRange_GetEndContainer(src_range->nsrange, &ref_node);
        assert(nsres == NS_OK);

        nsres = nsIDOMRange_GetEndOffset(src_range->nsrange, &ref_offset);
        assert(nsres == NS_OK);

        set_start = how_type == NS_START_TO_END;
        break;
    DEFAULT_UNREACHABLE;
    }

    nsres = nsIDOMRange_ComparePoint(This->nsrange, ref_node, ref_offset, &cmp);
    assert(nsres == NS_OK);

    if(set_start) {
        if(cmp <= 0) {
            nsres = nsIDOMRange_SetStart(This->nsrange, ref_node, ref_offset);
        }else {
            nsres = nsIDOMRange_Collapse(This->nsrange, FALSE);
            assert(nsres == NS_OK);

            nsres = nsIDOMRange_SetEnd(This->nsrange, ref_node, ref_offset);
        }
    }else {
        if(cmp >= 0) {
            nsres = nsIDOMRange_SetEnd(This->nsrange, ref_node, ref_offset);
        }else {
            nsres = nsIDOMRange_Collapse(This->nsrange, TRUE);
            assert(nsres == NS_OK);

            nsres = nsIDOMRange_SetStart(This->nsrange, ref_node, ref_offset);
        }
    }
    assert(nsres == NS_OK);

    nsIDOMNode_Release(ref_node);
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_compareEndPoints(IHTMLTxtRange *iface, BSTR how,
        IHTMLTxtRange *SourceRange, LONG *ret)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    HTMLTxtRange *src_range;
    short nsret = 0;
    int nscmpt;
    nsresult nsres;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(how), SourceRange, ret);

    nscmpt = string_to_nscmptype(how);
    if(nscmpt == -1)
        return E_INVALIDARG;

    src_range = get_range_object(This->doc, SourceRange);
    if(!src_range)
        return E_FAIL;

    nsres = nsIDOMRange_CompareBoundaryPoints(This->nsrange, nscmpt, src_range->nsrange, &nsret);
    if(NS_FAILED(nsres))
        ERR("CompareBoundaryPoints failed: %08lx\n", nsres);

    *ret = nsret;
    return S_OK;
}

static HRESULT WINAPI HTMLTxtRange_findText(IHTMLTxtRange *iface, BSTR String,
        LONG count, LONG Flags, VARIANT_BOOL *Success)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %ld %08lx %p)\n", This, debugstr_w(String), count, Flags, Success);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_moveToPoint(IHTMLTxtRange *iface, LONG x, LONG y)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%ld %ld)\n", This, x, y);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_getBookmark(IHTMLTxtRange *iface, BSTR *Bookmark)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%p)\n", This, Bookmark);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_moveToBookmark(IHTMLTxtRange *iface, BSTR Bookmark,
        VARIANT_BOOL *Success)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(Bookmark), Success);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandSupported(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandEnabled(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandState(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandIndeterm(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandText(IHTMLTxtRange *iface, BSTR cmdID,
        BSTR *pcmdText)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pcmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_queryCommandValue(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT *pcmdValue)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pcmdValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_execCommand(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %x v %p)\n", This, debugstr_w(cmdID), showUI, pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTxtRange_execCommandShowHelp(IHTMLTxtRange *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLTxtRange *This = impl_from_IHTMLTxtRange(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static const IHTMLTxtRangeVtbl HTMLTxtRangeVtbl = {
    HTMLTxtRange_QueryInterface,
    HTMLTxtRange_AddRef,
    HTMLTxtRange_Release,
    HTMLTxtRange_GetTypeInfoCount,
    HTMLTxtRange_GetTypeInfo,
    HTMLTxtRange_GetIDsOfNames,
    HTMLTxtRange_Invoke,
    HTMLTxtRange_get_htmlText,
    HTMLTxtRange_put_text,
    HTMLTxtRange_get_text,
    HTMLTxtRange_parentElement,
    HTMLTxtRange_duplicate,
    HTMLTxtRange_inRange,
    HTMLTxtRange_isEqual,
    HTMLTxtRange_scrollIntoView,
    HTMLTxtRange_collapse,
    HTMLTxtRange_expand,
    HTMLTxtRange_move,
    HTMLTxtRange_moveStart,
    HTMLTxtRange_moveEnd,
    HTMLTxtRange_select,
    HTMLTxtRange_pasteHTML,
    HTMLTxtRange_moveToElementText,
    HTMLTxtRange_setEndPoint,
    HTMLTxtRange_compareEndPoints,
    HTMLTxtRange_findText,
    HTMLTxtRange_moveToPoint,
    HTMLTxtRange_getBookmark,
    HTMLTxtRange_moveToBookmark,
    HTMLTxtRange_queryCommandSupported,
    HTMLTxtRange_queryCommandEnabled,
    HTMLTxtRange_queryCommandState,
    HTMLTxtRange_queryCommandIndeterm,
    HTMLTxtRange_queryCommandText,
    HTMLTxtRange_queryCommandValue,
    HTMLTxtRange_execCommand,
    HTMLTxtRange_execCommandShowHelp
};

static inline HTMLTxtRange *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, HTMLTxtRange, IOleCommandTarget_iface);
}

static HRESULT WINAPI RangeCommandTarget_QueryInterface(IOleCommandTarget *iface, REFIID riid, void **ppv)
{
    HTMLTxtRange *This = impl_from_IOleCommandTarget(iface);
    return IHTMLTxtRange_QueryInterface(&This->IHTMLTxtRange_iface, riid, ppv);
}

static ULONG WINAPI RangeCommandTarget_AddRef(IOleCommandTarget *iface)
{
    HTMLTxtRange *This = impl_from_IOleCommandTarget(iface);
    return IHTMLTxtRange_AddRef(&This->IHTMLTxtRange_iface);
}

static ULONG WINAPI RangeCommandTarget_Release(IOleCommandTarget *iface)
{
    HTMLTxtRange *This = impl_from_IOleCommandTarget(iface);
    return IHTMLTxtRange_Release(&This->IHTMLTxtRange_iface);
}

static HRESULT WINAPI RangeCommandTarget_QueryStatus(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    HTMLTxtRange *This = impl_from_IOleCommandTarget(iface);
    FIXME("(%p)->(%s %ld %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

static HRESULT exec_indent(HTMLTxtRange *This, VARIANT *in, VARIANT *out)
{
    nsIDOMElement *blockquote_elem, *p_elem;
    nsIDOMDocumentFragment *fragment;
    nsIDOMNode *tmp;

    TRACE("(%p)->(%p %p)\n", This, in, out);

    if(!This->doc->dom_document) {
        WARN("NULL dom_document\n");
        return E_NOTIMPL;
    }

    create_nselem(This->doc, L"BLOCKQUOTE", &blockquote_elem);
    create_nselem(This->doc, L"P", &p_elem);

    nsIDOMRange_ExtractContents(This->nsrange, &fragment);
    nsIDOMElement_AppendChild(p_elem, (nsIDOMNode*)fragment, &tmp);
    nsIDOMDocumentFragment_Release(fragment);
    nsIDOMNode_Release(tmp);

    nsIDOMElement_AppendChild(blockquote_elem, (nsIDOMNode*)p_elem, &tmp);
    nsIDOMElement_Release(p_elem);
    nsIDOMNode_Release(tmp);

    nsIDOMRange_InsertNode(This->nsrange, (nsIDOMNode*)blockquote_elem);
    nsIDOMElement_Release(blockquote_elem);

    return S_OK;
}

static HRESULT WINAPI RangeCommandTarget_Exec(IOleCommandTarget *iface, const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    HTMLTxtRange *This = impl_from_IOleCommandTarget(iface);

    TRACE("(%p)->(%s %ld %lx %p %p)\n", This, debugstr_guid(pguidCmdGroup), nCmdID,
          nCmdexecopt, pvaIn, pvaOut);

    if(pguidCmdGroup && IsEqualGUID(&CGID_MSHTML, pguidCmdGroup)) {
        switch(nCmdID) {
        case IDM_INDENT:
            return exec_indent(This, pvaIn, pvaOut);
        default:
            FIXME("Unsupported cmdid %ld of CGID_MSHTML\n", nCmdID);
        }
    }else {
        FIXME("Unsupported cmd %ld of group %s\n", nCmdID, debugstr_guid(pguidCmdGroup));
    }

    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    RangeCommandTarget_QueryInterface,
    RangeCommandTarget_AddRef,
    RangeCommandTarget_Release,
    RangeCommandTarget_QueryStatus,
    RangeCommandTarget_Exec
};

static inline HTMLTxtRange *HTMLTxtRange_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLTxtRange, dispex);
}

static void *HTMLTxtRange_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLTxtRange *This = HTMLTxtRange_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLTxtRange, riid))
        return &This->IHTMLTxtRange_iface;
    if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        return &This->IOleCommandTarget_iface;

    return NULL;
}

static void HTMLTxtRange_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTxtRange *This = HTMLTxtRange_from_DispatchEx(dispex);
    if(This->nsrange)
        note_cc_edge((nsISupports*)This->nsrange, "nsrange", cb);
}

static void HTMLTxtRange_unlink(DispatchEx *dispex)
{
    HTMLTxtRange *This = HTMLTxtRange_from_DispatchEx(dispex);
    unlink_ref(&This->nsrange);
    if(This->doc) {
        This->doc = NULL;
        list_remove(&This->entry);
    }
}

static void HTMLTxtRange_destructor(DispatchEx *dispex)
{
    HTMLTxtRange *This = HTMLTxtRange_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLTxtRange_dispex_vtbl = {
    .query_interface  = HTMLTxtRange_query_interface,
    .destructor       = HTMLTxtRange_destructor,
    .traverse         = HTMLTxtRange_traverse,
    .unlink           = HTMLTxtRange_unlink
};

static const tid_t TextRange_iface_tids[] = {
    IHTMLTxtRange_tid,
    0
};
dispex_static_data_t TextRange_dispex = {
    .id         = PROT_TextRange,
    .vtbl       = &HTMLTxtRange_dispex_vtbl,
    .disp_tid   = IHTMLTxtRange_tid,
    .iface_tids = TextRange_iface_tids,
};

HRESULT HTMLTxtRange_Create(HTMLDocumentNode *doc, nsIDOMRange *nsrange, IHTMLTxtRange **p)
{
    HTMLTxtRange *ret;

    ret = malloc(sizeof(HTMLTxtRange));
    if(!ret)
        return E_OUTOFMEMORY;

    init_dispatch(&ret->dispex, &TextRange_dispex, doc->script_global,
                  dispex_compat_mode(&doc->node.event_target.dispex));

    ret->IHTMLTxtRange_iface.lpVtbl = &HTMLTxtRangeVtbl;
    ret->IOleCommandTarget_iface.lpVtbl = &OleCommandTargetVtbl;

    if(nsrange)
        nsIDOMRange_AddRef(nsrange);
    ret->nsrange = nsrange;

    ret->doc = doc;
    list_add_head(&doc->range_list, &ret->entry);

    *p = &ret->IHTMLTxtRange_iface;
    return S_OK;
}

static inline HTMLDOMRange *impl_from_IHTMLDOMRange(IHTMLDOMRange *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMRange, IHTMLDOMRange_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMRange, IHTMLDOMRange, impl_from_IHTMLDOMRange(iface)->dispex)

static HRESULT WINAPI HTMLDOMRange_get_startContainer(IHTMLDOMRange *iface, IHTMLDOMNode **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_get_startOffset(IHTMLDOMRange *iface, LONG *p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_get_endContainer(IHTMLDOMRange *iface, IHTMLDOMNode **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_get_endOffset(IHTMLDOMRange *iface, LONG *p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_get_collapsed(IHTMLDOMRange *iface, VARIANT_BOOL *p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_get_commonAncestorContainer(IHTMLDOMRange *iface, IHTMLDOMNode **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_setStart(IHTMLDOMRange *iface, IDispatch *node, LONG offset)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p, %ld)\n", This, node, offset);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_setEnd(IHTMLDOMRange *iface, IDispatch *node, LONG offset)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p, %ld)\n", This, node, offset);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_setStartBefore(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_setStartAfter(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_setEndBefore(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_setEndAfter(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_collapse(IHTMLDOMRange *iface, VARIANT_BOOL tostart)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%x)\n", This, tostart);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_selectNode(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_selectNodeContents(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_compareBoundaryPoints(IHTMLDOMRange *iface, short how,
    IDispatch *src_range, LONG *result)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%x, %p, %p)\n", This, how, src_range, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_deleteContents(IHTMLDOMRange *iface)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_extractContents(IHTMLDOMRange *iface, IDispatch **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_cloneContents(IHTMLDOMRange *iface, IDispatch **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_insertNode(IHTMLDOMRange *iface, IDispatch *node)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_surroundContents(IHTMLDOMRange *iface, IDispatch *parent)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, parent);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_cloneRange(IHTMLDOMRange *iface, IHTMLDOMRange **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_toString(IHTMLDOMRange *iface, BSTR *p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_detach(IHTMLDOMRange *iface)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_getClientRects(IHTMLDOMRange *iface, IHTMLRectCollection **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMRange_getBoundingClientRect(IHTMLDOMRange *iface, IHTMLRect **p)
{
    HTMLDOMRange *This = impl_from_IHTMLDOMRange(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLDOMRangeVtbl HTMLDOMRangeVtbl = {
    HTMLDOMRange_QueryInterface,
    HTMLDOMRange_AddRef,
    HTMLDOMRange_Release,
    HTMLDOMRange_GetTypeInfoCount,
    HTMLDOMRange_GetTypeInfo,
    HTMLDOMRange_GetIDsOfNames,
    HTMLDOMRange_Invoke,
    HTMLDOMRange_get_startContainer,
    HTMLDOMRange_get_startOffset,
    HTMLDOMRange_get_endContainer,
    HTMLDOMRange_get_endOffset,
    HTMLDOMRange_get_collapsed,
    HTMLDOMRange_get_commonAncestorContainer,
    HTMLDOMRange_setStart,
    HTMLDOMRange_setEnd,
    HTMLDOMRange_setStartBefore,
    HTMLDOMRange_setStartAfter,
    HTMLDOMRange_setEndBefore,
    HTMLDOMRange_setEndAfter,
    HTMLDOMRange_collapse,
    HTMLDOMRange_selectNode,
    HTMLDOMRange_selectNodeContents,
    HTMLDOMRange_compareBoundaryPoints,
    HTMLDOMRange_deleteContents,
    HTMLDOMRange_extractContents,
    HTMLDOMRange_cloneContents,
    HTMLDOMRange_insertNode,
    HTMLDOMRange_surroundContents,
    HTMLDOMRange_cloneRange,
    HTMLDOMRange_toString,
    HTMLDOMRange_detach,
    HTMLDOMRange_getClientRects,
    HTMLDOMRange_getBoundingClientRect,
};

static inline HTMLDOMRange *HTMLDOMRange_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMRange, dispex);
}

static void *HTMLDOMRange_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLDOMRange *This = HTMLDOMRange_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IUnknown, riid))
        return &This->IHTMLDOMRange_iface;
    if(IsEqualGUID(&IID_IHTMLDOMRange, riid))
        return &This->IHTMLDOMRange_iface;

    return NULL;
}

static void HTMLDOMRange_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLDOMRange *This = HTMLDOMRange_from_DispatchEx(dispex);
    if(This->nsrange)
        note_cc_edge((nsISupports*)This->nsrange, "nsrange", cb);
}

static void HTMLDOMRange_unlink(DispatchEx *dispex)
{
    HTMLDOMRange *This = HTMLDOMRange_from_DispatchEx(dispex);
    unlink_ref(&This->nsrange);
}

static void HTMLDOMRange_destructor(DispatchEx *dispex)
{
    HTMLDOMRange *This = HTMLDOMRange_from_DispatchEx(dispex);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLDOMRange_dispex_vtbl = {
    .query_interface  = HTMLDOMRange_query_interface,
    .destructor       = HTMLDOMRange_destructor,
    .traverse         = HTMLDOMRange_traverse,
    .unlink           = HTMLDOMRange_unlink
};

static const tid_t Range_iface_tids[] = {
    IHTMLDOMRange_tid,
    0
};

dispex_static_data_t Range_dispex = {
    .id         = PROT_Range,
    .vtbl       = &HTMLDOMRange_dispex_vtbl,
    .disp_tid   = DispHTMLDOMRange_tid,
    .iface_tids = Range_iface_tids,
};

HRESULT create_dom_range(nsIDOMRange *nsrange, HTMLDocumentNode *doc, IHTMLDOMRange **p)
{
    HTMLDOMRange *ret;

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    init_dispatch(&ret->dispex, &Range_dispex, doc->script_global,
                  dispex_compat_mode(&doc->node.event_target.dispex));

    ret->IHTMLDOMRange_iface.lpVtbl = &HTMLDOMRangeVtbl;

    if(nsrange)
        nsIDOMRange_AddRef(nsrange);
    ret->nsrange = nsrange;

    *p = &ret->IHTMLDOMRange_iface;
    return S_OK;
}

void detach_ranges(HTMLDocumentNode *This)
{
    HTMLTxtRange *iter, *next;

    LIST_FOR_EACH_ENTRY_SAFE(iter, next, &This->range_list, HTMLTxtRange, entry) {
        iter->doc = NULL;
        list_remove(&iter->entry);
    }
}

typedef struct {
    IMarkupPointer2 IMarkupPointer2_iface;
    LONG ref;
} MarkupPointer;

static inline MarkupPointer *impl_from_IMarkupPointer2(IMarkupPointer2 *iface)
{
    return CONTAINING_RECORD(iface, MarkupPointer, IMarkupPointer2_iface);
}

static HRESULT WINAPI MarkupPointer2_QueryInterface(IMarkupPointer2 *iface, REFIID riid, void **ppv)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IMarkupPointer2_iface;
    }else if(IsEqualGUID(&IID_IMarkupPointer, riid)) {
        *ppv = &This->IMarkupPointer2_iface;
    }else if(IsEqualGUID(&IID_IMarkupPointer2, riid)) {
        *ppv = &This->IMarkupPointer2_iface;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MarkupPointer2_AddRef(IMarkupPointer2 *iface)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI MarkupPointer2_Release(IMarkupPointer2 *iface)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI MarkupPointer2_OwningDoc(IMarkupPointer2 *iface, IHTMLDocument2 **p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_Gravity(IMarkupPointer2 *iface, POINTER_GRAVITY *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_SetGravity(IMarkupPointer2 *iface, POINTER_GRAVITY gravity)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%u)\n", This, gravity);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_Cling(IMarkupPointer2 *iface, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_SetCling(IMarkupPointer2 *iface, BOOL cling)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%x)\n", This, cling);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_Unposition(IMarkupPointer2 *iface)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsPositioned(IMarkupPointer2 *iface, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_GetContainer(IMarkupPointer2 *iface, IMarkupContainer **p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveAdjacentToElement(IMarkupPointer2 *iface, IHTMLElement *element, ELEMENT_ADJACENCY adj)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %u)\n", This, element, adj);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveToPointer(IMarkupPointer2 *iface, IMarkupPointer *pointer)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, pointer);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveToContainer(IMarkupPointer2 *iface, IMarkupContainer *container, BOOL at_start)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %x)\n", This, container, at_start);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_Left(IMarkupPointer2 *iface, BOOL move, MARKUP_CONTEXT_TYPE *context,
                                          IHTMLElement **element, LONG *len, OLECHAR *text)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%x %p %p %p %p)\n", This, move, context, element, len, text);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_Right(IMarkupPointer2 *iface, BOOL move, MARKUP_CONTEXT_TYPE *context,
                                           IHTMLElement **element, LONG *len, OLECHAR *text)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%x %p %p %p %p)\n", This, move, context, element, len, text);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_CurrentScope(IMarkupPointer2 *iface, IHTMLElement **p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsLeftOf(IMarkupPointer2 *iface, IMarkupPointer *that_pointer, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %p)\n", This, that_pointer, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsLeftOfOrEqualTo(IMarkupPointer2 *iface, IMarkupPointer *that_pointer, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %p)\n", This, that_pointer, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsRightOf(IMarkupPointer2 *iface, IMarkupPointer *that_pointer, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %p)\n", This, that_pointer, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsRightOfOrEqualTo(IMarkupPointer2 *iface, IMarkupPointer *that_pointer, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %p)\n", This, that_pointer, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsEqualTo(IMarkupPointer2 *iface, IMarkupPointer *that_pointer, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %p)\n", This, that_pointer, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveUnit(IMarkupPointer2 *iface, MOVEUNIT_ACTION action)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%u)\n", This, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_FindText(IMarkupPointer2 *iface, OLECHAR *text, DWORD flags,
                                              IMarkupPointer *end_match, IMarkupPointer *end_search)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%s %lx %p %p)\n", This, debugstr_w(text), flags, end_match, end_search);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsAtWordBreak(IMarkupPointer2 *iface, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_GetMarkupPosition(IMarkupPointer2 *iface, LONG *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveToMarkupPosition(IMarkupPointer2 *iface, IMarkupContainer *container, LONG mp)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %ld)\n", This, container, mp);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveUnitBounded(IMarkupPointer2 *iface, MOVEUNIT_ACTION action, IMarkupPointer *boundary)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%u %p)\n", This, action, boundary);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_IsInsideURL(IMarkupPointer2 *iface, IMarkupPointer *right, BOOL *p)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %p)\n", This, right, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupPointer2_MoveToContent(IMarkupPointer2 *iface, IHTMLElement *element, BOOL at_start)
{
    MarkupPointer *This = impl_from_IMarkupPointer2(iface);
    FIXME("(%p)->(%p %x)\n", This, element, at_start);
    return E_NOTIMPL;
}

static const IMarkupPointer2Vtbl MarkupPointer2Vtbl = {
    MarkupPointer2_QueryInterface,
    MarkupPointer2_AddRef,
    MarkupPointer2_Release,
    MarkupPointer2_OwningDoc,
    MarkupPointer2_Gravity,
    MarkupPointer2_SetGravity,
    MarkupPointer2_Cling,
    MarkupPointer2_SetCling,
    MarkupPointer2_Unposition,
    MarkupPointer2_IsPositioned,
    MarkupPointer2_GetContainer,
    MarkupPointer2_MoveAdjacentToElement,
    MarkupPointer2_MoveToPointer,
    MarkupPointer2_MoveToContainer,
    MarkupPointer2_Left,
    MarkupPointer2_Right,
    MarkupPointer2_CurrentScope,
    MarkupPointer2_IsLeftOf,
    MarkupPointer2_IsLeftOfOrEqualTo,
    MarkupPointer2_IsRightOf,
    MarkupPointer2_IsRightOfOrEqualTo,
    MarkupPointer2_IsEqualTo,
    MarkupPointer2_MoveUnit,
    MarkupPointer2_FindText,
    MarkupPointer2_IsAtWordBreak,
    MarkupPointer2_GetMarkupPosition,
    MarkupPointer2_MoveToMarkupPosition,
    MarkupPointer2_MoveUnitBounded,
    MarkupPointer2_IsInsideURL,
    MarkupPointer2_MoveToContent
};

HRESULT create_markup_pointer(IMarkupPointer **ret)
{
    MarkupPointer *markup_pointer;

    if(!(markup_pointer = malloc(sizeof(*markup_pointer))))
        return E_OUTOFMEMORY;

    markup_pointer->IMarkupPointer2_iface.lpVtbl = &MarkupPointer2Vtbl;
    markup_pointer->ref = 1;

    *ret = (IMarkupPointer*)&markup_pointer->IMarkupPointer2_iface;
    return S_OK;
}
