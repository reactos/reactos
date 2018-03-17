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

#include <assert.h>

#include "jscript.h"

#include "wine/debug.h"

/*
 * This is the length of a string that is considered to be long enough to be
 * worth the rope to avoid copy.
 * This probably could be tuned, but keep it low for a while to better test rope's code.
 */
#define JSSTR_SHORT_STRING_LENGTH 8

/*
 * This is the max rope depth. While faster to allocate, ropes may become slow at access.
 */
#define JSSTR_MAX_ROPE_DEPTH 100

const char *debugstr_jsstr(jsstr_t *str)
{
    return jsstr_is_inline(str) ? debugstr_wn(jsstr_as_inline(str)->buf, jsstr_length(str))
        : jsstr_is_heap(str) ? debugstr_wn(jsstr_as_heap(str)->buf, jsstr_length(str))
        : wine_dbg_sprintf("%s...", debugstr_jsstr(jsstr_as_rope(str)->left));
}

void jsstr_free(jsstr_t *str)
{
    switch(jsstr_tag(str)) {
    case JSSTR_HEAP:
        heap_free(jsstr_as_heap(str)->buf);
        break;
    case JSSTR_ROPE: {
        jsstr_rope_t *rope = jsstr_as_rope(str);
        jsstr_release(rope->left);
        jsstr_release(rope->right);
        break;
    }
    case JSSTR_INLINE:
        break;
    }

    heap_free(str);
}

static inline void jsstr_init(jsstr_t *str, unsigned len, jsstr_tag_t tag)
{
    str->length_flags = len << JSSTR_LENGTH_SHIFT | tag;
    str->ref = 1;
}

jsstr_t *jsstr_alloc_buf(unsigned len, WCHAR **buf)
{
    jsstr_inline_t *ret;

    if(len > JSSTR_MAX_LENGTH)
        return NULL;

    ret = heap_alloc(FIELD_OFFSET(jsstr_inline_t, buf[len+1]));
    if(!ret)
        return NULL;

    jsstr_init(&ret->str, len, JSSTR_INLINE);
    ret->buf[len] = 0;
    *buf = ret->buf;
    return &ret->str;
}

jsstr_t *jsstr_alloc_len(const WCHAR *buf, unsigned len)
{
    jsstr_t *ret;
    WCHAR *ptr;

    ret = jsstr_alloc_buf(len, &ptr);
    if(ret)
        memcpy(ptr, buf, len*sizeof(WCHAR));

    return ret;
}

static void jsstr_rope_extract(jsstr_rope_t *str, unsigned off, unsigned len, WCHAR *buf)
{
    unsigned left_len = jsstr_length(str->left);

    if(left_len <= off) {
        jsstr_extract(str->right, off-left_len, len, buf);
    }else if(left_len >= len+off) {
        jsstr_extract(str->left, off, len, buf);
    }else {
        left_len -= off;
        jsstr_extract(str->left, off, left_len, buf);
        jsstr_extract(str->right, 0, len-left_len, buf+left_len);
    }
}

void jsstr_extract(jsstr_t *str, unsigned off, unsigned len, WCHAR *buf)
{
    switch(jsstr_tag(str)) {
    case JSSTR_INLINE:
        memcpy(buf, jsstr_as_inline(str)->buf+off, len*sizeof(WCHAR));
        return;
    case JSSTR_HEAP:
        memcpy(buf, jsstr_as_heap(str)->buf+off, len*sizeof(WCHAR));
        return;
    case JSSTR_ROPE:
        jsstr_rope_extract(jsstr_as_rope(str), off, len, buf);
        return;
    }
}

static int jsstr_cmp_str(jsstr_t *jsstr, const WCHAR *str, unsigned len)
{
    int ret;

    switch(jsstr_tag(jsstr)) {
    case JSSTR_INLINE:
        ret = memcmp(jsstr_as_inline(jsstr)->buf, str, len*sizeof(WCHAR));
        return ret || jsstr_length(jsstr) == len ? ret : 1;
    case JSSTR_HEAP:
        ret = memcmp(jsstr_as_heap(jsstr)->buf, str, len*sizeof(WCHAR));
        return ret || jsstr_length(jsstr) == len ? ret : 1;
    case JSSTR_ROPE: {
        jsstr_rope_t *rope = jsstr_as_rope(jsstr);
        unsigned left_len = jsstr_length(rope->left);

        ret = jsstr_cmp_str(rope->left, str, min(len, left_len));
        if(ret || len <= left_len)
            return ret;
        return jsstr_cmp_str(rope->right, str+left_len, len-left_len);
    }
    }

    assert(0);
    return 0;
}

#define TMP_BUF_SIZE 256

static int ropes_cmp(jsstr_rope_t *left, jsstr_rope_t *right)
{
    WCHAR left_buf[TMP_BUF_SIZE], right_buf[TMP_BUF_SIZE];
    unsigned left_len = jsstr_length(&left->str);
    unsigned right_len = jsstr_length(&right->str);
    unsigned cmp_off = 0, cmp_size;
    int ret;

    /* FIXME: We can avoid temporary buffers here. */
    while(cmp_off < min(left_len, right_len)) {
        cmp_size = min(left_len, right_len) - cmp_off;
        if(cmp_size > TMP_BUF_SIZE)
            cmp_size = TMP_BUF_SIZE;

        jsstr_rope_extract(left, cmp_off, cmp_size, left_buf);
        jsstr_rope_extract(right, cmp_off, cmp_size, right_buf);
        ret = memcmp(left_buf, right_buf, cmp_size);
        if(ret)
            return ret;

        cmp_off += cmp_size;
    }

    return left_len - right_len;
}

static inline const WCHAR *jsstr_try_flat(jsstr_t *str)
{
    return jsstr_is_inline(str) ? jsstr_as_inline(str)->buf
        : jsstr_is_heap(str) ? jsstr_as_heap(str)->buf
        : NULL;
}

int jsstr_cmp(jsstr_t *str1, jsstr_t *str2)
{
    unsigned len1 = jsstr_length(str1);
    unsigned len2 = jsstr_length(str2);
    const WCHAR *str;
    int ret;

    str = jsstr_try_flat(str2);
    if(str) {
        ret = jsstr_cmp_str(str1, str, min(len1, len2));
        return ret || len1 == len2 ? ret : -1;
    }

    str = jsstr_try_flat(str1);
    if(str) {
        ret = jsstr_cmp_str(str2, str, min(len1, len2));
        return ret || len1 == len2 ? -ret : 1;
    }

    return ropes_cmp(jsstr_as_rope(str1), jsstr_as_rope(str2));
}

jsstr_t *jsstr_concat(jsstr_t *str1, jsstr_t *str2)
{
    unsigned len1, len2;
    jsstr_t *ret;
    WCHAR *ptr;

    len1 = jsstr_length(str1);
    if(!len1)
        return jsstr_addref(str2);

    len2 = jsstr_length(str2);
    if(!len2)
        return jsstr_addref(str1);

    if(len1 + len2 >= JSSTR_SHORT_STRING_LENGTH) {
        unsigned depth, depth2;
        jsstr_rope_t *rope;

        depth = jsstr_is_rope(str1) ? jsstr_as_rope(str1)->depth : 0;
        depth2 = jsstr_is_rope(str2) ? jsstr_as_rope(str2)->depth : 0;
        if(depth2 > depth)
            depth = depth2;

        if(depth++ < JSSTR_MAX_ROPE_DEPTH) {
            if(len1+len2 > JSSTR_MAX_LENGTH)
                return NULL;

            rope = heap_alloc(sizeof(*rope));
            if(!rope)
                return NULL;

            jsstr_init(&rope->str, len1+len2, JSSTR_ROPE);
            rope->left = jsstr_addref(str1);
            rope->right = jsstr_addref(str2);
            rope->depth = depth;
            return &rope->str;
        }
    }

    ret = jsstr_alloc_buf(len1+len2, &ptr);
    if(!ret)
        return NULL;

    jsstr_flush(str1, ptr);
    jsstr_flush(str2, ptr+len1);
    return ret;

}

C_ASSERT(sizeof(jsstr_heap_t) <= sizeof(jsstr_rope_t));

const WCHAR *jsstr_rope_flatten(jsstr_rope_t *str)
{
    WCHAR *buf;

    buf = heap_alloc((jsstr_length(&str->str)+1) * sizeof(WCHAR));
    if(!buf)
        return NULL;

    jsstr_flush(str->left, buf);
    jsstr_flush(str->right, buf+jsstr_length(str->left));
    buf[jsstr_length(&str->str)] = 0;

    /* Trasform to heap string */
    jsstr_release(str->left);
    jsstr_release(str->right);
    str->str.length_flags |= JSSTR_FLAG_FLAT;
    return jsstr_as_heap(&str->str)->buf = buf;
}

static jsstr_t *empty_str, *nan_str, *undefined_str, *null_bstr_str;

jsstr_t *jsstr_nan(void)
{
    return jsstr_addref(nan_str);
}

jsstr_t *jsstr_empty(void)
{
    return jsstr_addref(empty_str);
}

jsstr_t *jsstr_undefined(void)
{
    return jsstr_addref(undefined_str);
}

jsstr_t *jsstr_null_bstr(void)
{
    return jsstr_addref(null_bstr_str);
}

BOOL is_null_bstr(jsstr_t *str)
{
    return str == null_bstr_str;
}

BOOL init_strings(void)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};
    WCHAR *ptr;

    if(!(empty_str = jsstr_alloc_buf(0, &ptr)))
        return FALSE;
    if(!(nan_str = jsstr_alloc(NaNW)))
        return FALSE;
    if(!(undefined_str = jsstr_alloc(undefinedW)))
        return FALSE;
    if(!(null_bstr_str = jsstr_alloc_buf(0, &ptr)))
        return FALSE;
     return TRUE;
}

void free_strings(void)
{
    if(empty_str)
        jsstr_release(empty_str);
    if(nan_str)
        jsstr_release(nan_str);
    if(undefined_str)
        jsstr_release(undefined_str);
    if(null_bstr_str)
        jsstr_release(null_bstr_str);
}
