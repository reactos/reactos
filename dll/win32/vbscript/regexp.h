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

/*
 * Code in this file is based on files:
 * js/src/jsregexp.h
 * js/src/jsregexp.c
 * from Mozilla project, released under LGPL 2.1 or later.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 */

#pragma once

#define REG_FOLD      0x01      /* fold uppercase to lowercase */
#define REG_GLOB      0x02      /* global exec, creates array of matches */
#define REG_MULTILINE 0x04      /* treat ^ and $ as begin and end of line */
#define REG_STICKY    0x08      /* only match starting at lastIndex */

typedef struct RECapture {
    ptrdiff_t index;            /* start of contents, -1 for empty  */
    size_t length;              /* length of capture */
} RECapture;

typedef struct match_state_t {
    const WCHAR *cp;
    DWORD match_len;

    DWORD paren_count;
    RECapture parens[1];
} match_state_t;

typedef BYTE jsbytecode;

typedef struct regexp_t {
    WORD                flags;         /* flags, see jsapi.h's REG_* defines */
    size_t              parenCount;    /* number of parenthesized submatches */
    size_t              classCount;    /* count [...] bitmaps */
    struct RECharSet    *classList;    /* list of [...] bitmaps */
    const WCHAR         *source;       /* locked source string, sans // */
    DWORD               source_len;
    jsbytecode          program[1];    /* regular expression bytecode */
} regexp_t;

regexp_t* regexp_new(void*, heap_pool_t*, const WCHAR*, DWORD, WORD, BOOL) DECLSPEC_HIDDEN;
void regexp_destroy(regexp_t*) DECLSPEC_HIDDEN;
HRESULT regexp_execute(regexp_t*, void*, heap_pool_t*, const WCHAR*,
        DWORD, match_state_t*) DECLSPEC_HIDDEN;
HRESULT regexp_set_flags(regexp_t**, void*, heap_pool_t*, WORD) DECLSPEC_HIDDEN;

static inline match_state_t* alloc_match_state(regexp_t *regexp,
        heap_pool_t *pool, const WCHAR *pos)
{
    size_t size = offsetof(match_state_t, parens) + regexp->parenCount*sizeof(RECapture);
    match_state_t *ret;

    ret = pool ? heap_pool_alloc(pool, size) : heap_alloc(size);
    if(!ret)
        return NULL;

    ret->cp = pos;
    return ret;
}
