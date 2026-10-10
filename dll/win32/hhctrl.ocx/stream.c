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

#include "hhctrl.h"
#include "stream.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

void strbuf_init(strbuf_t *buf)
{
    buf->size = 8;
    buf->len = 0;
    buf->buf = malloc(buf->size);
}

void strbuf_zero(strbuf_t *buf)
{
    buf->len = 0;
}

void strbuf_free(strbuf_t *buf)
{
    free(buf->buf);
}

static void strbuf_append(strbuf_t *buf, const char *data, int len)
{
    if(buf->len+len > buf->size) {
        buf->size = buf->len+len;
        buf->buf = realloc(buf->buf, buf->size);
    }

    memcpy(buf->buf+buf->len, data, len);
    buf->len += len;
}

void stream_init(stream_t *stream, IStream *str)
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

void get_node_name(strbuf_t *node, strbuf_t *name)
{
    const char *ptr = node->buf+1;

    strbuf_zero(name);

    while(*ptr != '>' && !isspace(*ptr))
        ptr++;

    strbuf_append(name, node->buf+1, ptr-node->buf-1);
    strbuf_append(name, "", 1);
}

/* Return the stream content up to the next HTML tag.
 *
 * Note: the first returned character is the end of the last tag (>).
 */
BOOL next_content(stream_t *stream, strbuf_t *buf)
{
    if(!stream_chr(stream, buf, '<'))
        return FALSE;

    return TRUE;
}

static BOOL find_node_end(stream_t *stream, strbuf_t *buf)
{
    int tag_count = 0, b = buf->len;
    char *p;

    while(1)
    {
        if(!stream_chr(stream, buf, '>'))
            return FALSE;
        if(buf->len == 0)
            break;
        p = &buf->buf[b];
        while((p = memchr(p+1, '"', buf->len-(p+1-buf->buf))) != NULL)
            tag_count++;
        b = buf->len;
        if(tag_count % 2 != 0)
        {
            if(!stream_chr(stream, buf, '"'))
                return FALSE;
            tag_count++;
        }
        else
            break;
    }
    return TRUE;
}

BOOL next_node(stream_t *stream, strbuf_t *buf)
{
    /* find the beginning of the next node */
    if(!stream_chr(stream, NULL, '<'))
        return FALSE;

    /* read out the data of the next node */
    if(!find_node_end(stream, buf))
        return FALSE;

    strbuf_append(buf, ">", 2);

    return TRUE;
}

/*
 * Find the value of a named HTML attribute.
 *
 * Note: Attribute names are case insensitive, so it is necessary to
 * put both the node text and the attribute name in the same case
 * before attempting a string search.
 */
const char *get_attr(const char *node, const char *name, int *len)
{
    const char *ptr, *ptr2;
    int name_len, node_len;
    char name_buf[32];
    char *node_buf;
    int i;

    /* Create a lower case copy of the node */
    node_len = strlen(node)+1;
    node_buf = malloc(node_len * sizeof(char));
    if(!node_buf)
        return NULL;
    memcpy(node_buf, node, node_len);
    for(i=0;i<node_len;i++)
        node_buf[i] = tolower(node_buf[i]);
    /* Create a lower case copy of the attribute name (search string) */
    name_len = strlen(name);
    memcpy(name_buf, name, name_len);
    for(i=0;i<name_len;i++)
        name_buf[i] = tolower(name_buf[i]);
    name_buf[name_len++] = '=';
    name_buf[name_len++] = '\"';
    name_buf[name_len] = 0;

    ptr = strstr(node_buf, name_buf);
    if(!ptr) {
        WARN("name not found\n");
        free(node_buf);
        return NULL;
    }

    ptr += name_len;
    ptr2 = strchr(ptr, '\"');
    if(!ptr2)
    {
        free(node_buf);
        return NULL;
    }

    *len = ptr2-ptr;
    /* Return the pointer offset within the original string */
    ptr = node+(ptr-node_buf);

    free(node_buf);
    return ptr;
}
