/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

/*
 * jsstr_t is a common header for all string representations. The exact layout of the string
 * representation may be:
 *
 * - inline string - string bytes directly follow string headers.
 * - heap string - a structure containing a pointer to buffer on the heap.
 * - roper string - a product of concatenation of two strings. Instead of copying whole
 *   buffers, we may store just references to concatenated strings.
 *
 * String layout may change over life time of the string. Currently possible transformation
 * is when a rope string becomes a heap stream. That happens when we need a real, linear
 * zero-terminated buffer (a flat buffer). At this point the type of the string is changed
 * and the new buffer is stored in the string, so that subsequent operations requiring
 * a flat string won't need to flatten it again.
 *
 * In the future more layouts and transformations may be added.
 */
struct _jsstr_t {
    unsigned length_flags;
    unsigned ref;
};

#define JSSTR_LENGTH_SHIFT 4
#define JSSTR_MAX_LENGTH ((1 << (32-JSSTR_LENGTH_SHIFT))-1)
#define JSSTR_FLAGS_MASK ((1 << JSSTR_LENGTH_SHIFT)-1)

#define JSSTR_FLAG_LBIT     1
#define JSSTR_FLAG_FLAT     2
#define JSSTR_FLAG_TAG_MASK 3

typedef enum {
    JSSTR_INLINE = JSSTR_FLAG_FLAT,
    JSSTR_HEAP   = JSSTR_FLAG_FLAT|JSSTR_FLAG_LBIT,
    JSSTR_ROPE   = JSSTR_FLAG_LBIT
} jsstr_tag_t;

static inline unsigned jsstr_length(jsstr_t *str)
{
    return str->length_flags >> JSSTR_LENGTH_SHIFT;
}

static inline jsstr_tag_t jsstr_tag(jsstr_t *str)
{
    return str->length_flags & JSSTR_FLAG_TAG_MASK;
}

static inline BOOL jsstr_is_inline(jsstr_t *str)
{
    return jsstr_tag(str) == JSSTR_INLINE;
}

static inline BOOL jsstr_is_heap(jsstr_t *str)
{
    return jsstr_tag(str) == JSSTR_HEAP;
}

static inline BOOL jsstr_is_rope(jsstr_t *str)
{
    return jsstr_tag(str) == JSSTR_ROPE;
}

typedef struct {
    jsstr_t str;
    WCHAR buf[];
} jsstr_inline_t;

typedef struct {
    jsstr_t str;
    WCHAR *buf;
} jsstr_heap_t;

typedef struct {
    jsstr_t str;
    jsstr_t *left;
    jsstr_t *right;
    unsigned depth;
} jsstr_rope_t;

jsstr_t *jsstr_alloc_len(const WCHAR*,unsigned);
jsstr_t *jsstr_alloc_buf(unsigned,WCHAR**);

static inline jsstr_t *jsstr_alloc(const WCHAR *str)
{
    return jsstr_alloc_len(str, lstrlenW(str));
}

void jsstr_free(jsstr_t*);

static inline void jsstr_release(jsstr_t *str)
{
    if(!--str->ref)
        jsstr_free(str);
}

static inline jsstr_t *jsstr_addref(jsstr_t *str)
{
    str->ref++;
    return str;
}

static inline jsstr_inline_t *jsstr_as_inline(jsstr_t *str)
{
    return CONTAINING_RECORD(str, jsstr_inline_t, str);
}

static inline jsstr_heap_t *jsstr_as_heap(jsstr_t *str)
{
    return CONTAINING_RECORD(str, jsstr_heap_t, str);
}

static inline jsstr_rope_t *jsstr_as_rope(jsstr_t *str)
{
    return CONTAINING_RECORD(str, jsstr_rope_t, str);
}

const WCHAR *jsstr_rope_flatten(jsstr_rope_t*);

static inline const WCHAR *jsstr_flatten(jsstr_t *str)
{
    return jsstr_is_inline(str) ? jsstr_as_inline(str)->buf
        : jsstr_is_heap(str) ? jsstr_as_heap(str)->buf
        : jsstr_rope_flatten(jsstr_as_rope(str));
}

void jsstr_extract(jsstr_t*,unsigned,unsigned,WCHAR*);

static inline unsigned jsstr_flush(jsstr_t *str, WCHAR *buf)
{
    unsigned len = jsstr_length(str);
    if(jsstr_is_inline(str)) {
        memcpy(buf, jsstr_as_inline(str)->buf, len*sizeof(WCHAR));
    }else if(jsstr_is_heap(str)) {
        memcpy(buf, jsstr_as_heap(str)->buf, len*sizeof(WCHAR));
    }else {
        jsstr_rope_t *rope = jsstr_as_rope(str);
        jsstr_flush(rope->left, buf);
        jsstr_flush(rope->right, buf+jsstr_length(rope->left));
    }
    return len;
}

static inline jsstr_t *jsstr_substr(jsstr_t *str, unsigned off, unsigned len)
{
    jsstr_t *ret;
    WCHAR *ptr;

    ret = jsstr_alloc_buf(len, &ptr);
    if(ret)
        jsstr_extract(str, off, len, ptr);
    return ret;
}

int jsstr_cmp(jsstr_t*,jsstr_t*);

static inline BOOL jsstr_eq(jsstr_t *left, jsstr_t *right)
{
    return jsstr_length(left) == jsstr_length(right) && !jsstr_cmp(left, right);
}

jsstr_t *jsstr_concat(jsstr_t*,jsstr_t*);

jsstr_t *jsstr_nan(void);
jsstr_t *jsstr_empty(void);
jsstr_t *jsstr_undefined(void);

jsstr_t *jsstr_null_bstr(void);
HRESULT jsstr_to_bstr(jsstr_t *str, BSTR *r);

BOOL init_strings(void);
void free_strings(void);

const char *debugstr_jsstr(jsstr_t*);
