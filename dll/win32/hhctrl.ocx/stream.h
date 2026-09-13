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

#ifndef HHCTRL_STREAM_H
#define HHCTRL_STREAM_H

#define BLOCK_BITS 12
#define BLOCK_SIZE (1 << BLOCK_BITS)
#define BLOCK_MASK (BLOCK_SIZE-1)

typedef struct {
    char *buf;
    int size;
    int len;
} strbuf_t;

typedef struct {
    IStream *str;
    char buf[BLOCK_SIZE];
    ULONG size;
    ULONG p;
} stream_t;

void strbuf_init(strbuf_t *buf);
void strbuf_zero(strbuf_t *buf);
void strbuf_free(strbuf_t *buf);
void stream_init(stream_t *stream, IStream *str);
void get_node_name(strbuf_t *node, strbuf_t *name);
BOOL next_content(stream_t *stream, strbuf_t *buf);
BOOL next_node(stream_t *stream, strbuf_t *buf);
const char *get_attr(const char *node, const char *name, int *len);

#endif
