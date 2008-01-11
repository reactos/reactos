/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "hhctrl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

#define BLOCK_SIZE 0x1000

typedef enum {
    INSERT_NEXT,
    INSERT_CHILD
} insert_type_t;

static void free_content_item(ContentItem *item)
{
    ContentItem *next;

    while(item) {
        next = item->next;

        free_content_item(item->child);

        hhctrl_free(item->name);
        hhctrl_free(item->local);
        hhctrl_free(item->merge.chm_file);
        hhctrl_free(item->merge.chm_index);

        item = next;
    }
}

typedef struct {
    char *buf;
    int size;
    int len;
} strbuf_t;

static void strbuf_init(strbuf_t *buf)
{
    buf->size = 8;
    buf->len = 0;
    buf->buf = hhctrl_alloc(buf->size);
}

static void strbuf_zero(strbuf_t *buf)
{
    buf->len = 0;
}

static void strbuf_free(strbuf_t *buf)
{
    hhctrl_free(buf->buf);
}

static void strbuf_append(strbuf_t *buf, const char *data, int len)
{
    if(buf->len+len > buf->size) {
        buf->size = buf->len+len;
        buf->buf = hhctrl_realloc(buf->buf, buf->size);
    }

    memcpy(buf->buf+buf->len, data, len);
    buf->len += len;
}

typedef struct {
    IStream *str;
    char buf[BLOCK_SIZE];
    ULONG size;
    ULONG p;
} stream_t;

static void stream_init(stream_t *stream, IStream *str)
{
    memset(stream, 0, sizeof(stream_t));
    stream->str = str;
}

static BOOL stream_chr(stream_t *stream, strbuf_t *buf, char c)
{
    BOOL b = TRUE;
    ULONG i;

    while(b) {
        for(i=stream->p; i<stream->size; i++) {
            if(stream->buf[i] == c) {
                b = FALSE;
                break;
            }
        }

        if(buf && i > stream->p)
            strbuf_append(buf, stream->buf+stream->p, i-stream->p);
        stream->p = i;

        if(stream->p == stream->size) {
            stream->p = 0;
            IStream_Read(stream->str, stream->buf, sizeof(stream->buf), &stream->size);
            if(!stream->size)
                break;
        }
    }

    return stream->size != 0;
}

static void get_node_name(strbuf_t *node, strbuf_t *name)
{
    const char *ptr = node->buf+1;

    strbuf_zero(name);

    while(*ptr != '>' && !isspace(*ptr))
        ptr++;

    strbuf_append(name, node->buf+1, ptr-node->buf-1);
    strbuf_append(name, "", 1);
}

static BOOL next_node(stream_t *stream, strbuf_t *buf)
{
    if(!stream_chr(stream, NULL, '<'))
        return FALSE;

    if(!stream_chr(stream, buf, '>'))
        return FALSE;

    strbuf_append(buf, ">", 2);

    return TRUE;
}

static const char *get_attr(const char *node, const char *name, int *len)
{
    const char *ptr, *ptr2;
    char name_buf[32];
    int nlen;

    nlen = strlen(name);
    memcpy(name_buf, name, nlen);
    name_buf[nlen++] = '=';
    name_buf[nlen++] = '\"';
    name_buf[nlen] = 0;

    ptr = strstr(node, name_buf);
    if(!ptr) {
        WARN("name not found\n");
        return NULL;
    }

    ptr += nlen;
    ptr2 = strchr(ptr, '\"');
    if(!ptr2)
        return NULL;

    *len = ptr2-ptr;
    return ptr;
}

static void parse_obj_node_param(ContentItem *item, ContentItem *hhc_root, const char *text)
{
    const char *ptr;
    LPWSTR *param, merge;
    int len, wlen;

    ptr = get_attr(text, "name", &len);
    if(!ptr) {
        WARN("name attr not found\n");
        return;
    }

    if(!strncasecmp("name", ptr, len)) {
        param = &item->name;
    }else if(!strncasecmp("merge", ptr, len)) {
        param = &merge;
    }else if(!strncasecmp("local", ptr, len)) {
        param = &item->local;
    }else {
        WARN("unhandled param %s\n", debugstr_an(ptr, len));
        return;
    }

    ptr = get_attr(text, "value", &len);
    if(!ptr) {
        WARN("value attr not found\n");
        return;
    }

    wlen = MultiByteToWideChar(CP_ACP, 0, ptr, len, NULL, 0);
    *param = hhctrl_alloc((wlen+1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ptr, len, *param, wlen);
    (*param)[wlen] = 0;

    if(param == &merge) {
        SetChmPath(&item->merge, hhc_root->merge.chm_file, merge);
        hhctrl_free(merge);
    }
}

static ContentItem *parse_hhc(HHInfo*,IStream*,ContentItem*,insert_type_t*);

static ContentItem *insert_item(ContentItem *item, ContentItem *new_item, insert_type_t insert_type)
{
    if(!item)
        return new_item;

    if(!new_item)
        return item;

    switch(insert_type) {
    case INSERT_NEXT:
        item->next = new_item;
        return new_item;
    case INSERT_CHILD:
        if(item->child) {
            ContentItem *iter = item->child;
            while(iter->next)
                iter = iter->next;
            iter->next = new_item;
        }else {
            item->child = new_item;
        }
        return item;
    }

    return NULL;
}

static ContentItem *parse_sitemap_object(HHInfo *info, stream_t *stream, ContentItem *hhc_root,
        insert_type_t *insert_type)
{
    strbuf_t node, node_name;
    ContentItem *item;

    *insert_type = INSERT_NEXT;

    strbuf_init(&node);
    strbuf_init(&node_name);

    item = hhctrl_alloc_zero(sizeof(ContentItem));

    while(next_node(stream, &node)) {
        get_node_name(&node, &node_name);

        TRACE("%s\n", node.buf);

        if(!strcasecmp(node_name.buf, "/object"))
            break;
        if(!strcasecmp(node_name.buf, "param"))
            parse_obj_node_param(item, hhc_root, node.buf);

        strbuf_zero(&node);
    }

    strbuf_free(&node);
    strbuf_free(&node_name);

    if(item->merge.chm_index) {
        IStream *merge_stream;

        merge_stream = GetChmStream(info->pCHMInfo, item->merge.chm_file, &item->merge);
        if(merge_stream) {
            item->child = parse_hhc(info, merge_stream, hhc_root, insert_type);
            IStream_Release(merge_stream);
        }else {
            WARN("Could not get %s::%s stream\n", debugstr_w(item->merge.chm_file),
                 debugstr_w(item->merge.chm_file));

            if(!item->name) {
                free_content_item(item);
                item = NULL;
            }
        }

    }

    return item;
}

static ContentItem *parse_ul(HHInfo *info, stream_t *stream, ContentItem *hhc_root)
{
    strbuf_t node, node_name;
    ContentItem *ret = NULL, *prev = NULL, *new_item = NULL;
    insert_type_t it;

    strbuf_init(&node);
    strbuf_init(&node_name);

    while(next_node(stream, &node)) {
        get_node_name(&node, &node_name);

        TRACE("%s\n", node.buf);

        if(!strcasecmp(node_name.buf, "object")) {
            const char *ptr;
            int len;

            static const char sz_text_sitemap[] = "text/sitemap";

            ptr = get_attr(node.buf, "type", &len);

            if(ptr && len == sizeof(sz_text_sitemap)-1
               && !memcmp(ptr, sz_text_sitemap, len)) {
                new_item = parse_sitemap_object(info, stream, hhc_root, &it);
                prev = insert_item(prev, new_item, it);
                if(!ret)
                    ret = prev;
            }
        }else if(!strcasecmp(node_name.buf, "ul")) {
            new_item = parse_ul(info, stream, hhc_root);
            insert_item(prev, new_item, INSERT_CHILD);
        }else if(!strcasecmp(node_name.buf, "/ul")) {
            break;
        }

        strbuf_zero(&node);
    }

    strbuf_free(&node);
    strbuf_free(&node_name);

    return ret;
}

static ContentItem *parse_hhc(HHInfo *info, IStream *str, ContentItem *hhc_root,
        insert_type_t *insert_type)
{
    stream_t stream;
    strbuf_t node, node_name;
    ContentItem *ret = NULL, *prev = NULL;

    *insert_type = INSERT_NEXT;

    strbuf_init(&node);
    strbuf_init(&node_name);

    stream_init(&stream, str);

    while(next_node(&stream, &node)) {
        get_node_name(&node, &node_name);

        TRACE("%s\n", node.buf);

        if(!strcasecmp(node_name.buf, "ul")) {
            ContentItem *item = parse_ul(info, &stream, hhc_root);
            prev = insert_item(prev, item, INSERT_CHILD);
            if(!ret)
                ret = prev;
            *insert_type = INSERT_CHILD;
        }

        strbuf_zero(&node);
    }

    strbuf_free(&node);
    strbuf_free(&node_name);

    return ret;
}

static void insert_content_item(HWND hwnd, ContentItem *parent, ContentItem *item)
{
    TVINSERTSTRUCTW tvis;

    memset(&tvis, 0, sizeof(tvis));
    tvis.u.item.mask = TVIF_TEXT|TVIF_PARAM;
    tvis.u.item.cchTextMax = strlenW(item->name)+1;
    tvis.u.item.pszText = item->name;
    tvis.u.item.lParam = (LPARAM)item;
    tvis.hParent = parent ? parent->id : 0;
    tvis.hInsertAfter = TVI_LAST;

    item->id = (HTREEITEM)SendMessageW(hwnd, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
}

static void fill_content_tree(HWND hwnd, ContentItem *parent, ContentItem *item)
{
    while(item) {
        if(item->name) {
            insert_content_item(hwnd, parent, item);
            fill_content_tree(hwnd, item, item->child);
        }else {
            fill_content_tree(hwnd, parent, item->child);
        }
        item = item->next;
    }
}

static void set_item_parents(ContentItem *parent, ContentItem *item)
{
    while(item) {
        item->parent = parent;
        set_item_parents(item, item->child);
        item = item->next;
    }
}

void InitContent(HHInfo *info)
{
    IStream *stream;
    insert_type_t insert_type;

    info->content = hhctrl_alloc_zero(sizeof(ContentItem));
    SetChmPath(&info->content->merge, info->pCHMInfo->szFile, info->WinType.pszToc);

    stream = GetChmStream(info->pCHMInfo, info->pCHMInfo->szFile, &info->content->merge);
    if(!stream) {
        TRACE("Could not get content stream\n");
        return;
    }

    info->content->child = parse_hhc(info, stream, info->content, &insert_type);
    IStream_Release(stream);

    set_item_parents(NULL, info->content);
    fill_content_tree(info->tabs[TAB_CONTENTS].hwnd, NULL, info->content);
}

void ReleaseContent(HHInfo *info)
{
    free_content_item(info->content);
}
