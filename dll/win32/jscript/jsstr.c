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

#include "jscript.h"

#include <wine/debug.h>

const char *debugstr_jsstr(jsstr_t *str)
{
    return debugstr_wn(str->str, jsstr_length(str));
}

jsstr_t *jsstr_alloc_buf(unsigned len)
{
    jsstr_t *ret;

    if(len > JSSTR_MAX_LENGTH)
        return NULL;

    ret = heap_alloc(FIELD_OFFSET(jsstr_t, str[len+1]));
    if(!ret)
        return NULL;

    ret->length_flags = len << JSSTR_LENGTH_SHIFT;
    ret->ref = 1;
    ret->str[len] = 0;
    return ret;
}

jsstr_t *jsstr_alloc_len(const WCHAR *buf, unsigned len)
{
    jsstr_t *ret;

    ret = jsstr_alloc_buf(len);
    if(ret)
        memcpy(ret->str, buf, len*sizeof(WCHAR));

    return ret;
}

int jsstr_cmp(jsstr_t *str1, jsstr_t *str2)
{
    int len1 = jsstr_length(str1);
    int len2 = jsstr_length(str2);
    int ret;

    ret = memcmp(str1->str, str2->str, min(len1, len2)*sizeof(WCHAR));
    if(!ret)
        ret = len1 - len2;

    return ret;
}

jsstr_t *jsstr_concat(jsstr_t *str1, jsstr_t *str2)
{
    unsigned len1, len2;
    jsstr_t *ret;

    len1 = jsstr_length(str1);
    if(!len1)
        return jsstr_addref(str2);

    len2 = jsstr_length(str2);
    if(!len2)
        return jsstr_addref(str1);

    ret = jsstr_alloc_buf(len1+len2);
    if(!ret)
        return NULL;

    jsstr_flush(str1, ret->str);
    jsstr_flush(str2, ret->str+len1);
    return ret;
}

static jsstr_t *empty_str, *nan_str, *undefined_str;

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

BOOL init_strings(void)
{
    static const WCHAR NaNW[] = { 'N','a','N',0 };
    static const WCHAR undefinedW[] = {'u','n','d','e','f','i','n','e','d',0};

    if(!(empty_str = jsstr_alloc_buf(0)))
        return FALSE;
    if(!(nan_str = jsstr_alloc(NaNW)))
        return FALSE;
    if(!(undefined_str = jsstr_alloc(undefinedW)))
        return FALSE;
    return TRUE;
}

void free_strings(void)
{
    jsstr_release(empty_str);
    jsstr_release(nan_str);
    jsstr_release(undefined_str);
}
