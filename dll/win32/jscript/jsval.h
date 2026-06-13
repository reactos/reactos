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

#ifndef JSVAL_H
#define JSVAL_H

#include "jsstr.h"

/*
 * jsval_t structure is used to represent JavaScript dynamically-typed values.
 * It's a (type,value) pair, usually represented as a structure of enum (type)
 * and union (value of given type). For both memory and speed performance, we
 * use tricks allowing storing both values as a struct with size equal to
 * size of double (that is 64-bit) on 32-bit systems. For that, we use the fact
 * that NaN value representation has 52 (almost) free bits.
 */

#ifdef __i386__
#define JSVAL_DOUBLE_LAYOUT_PTR32
#endif

#ifdef JSVAL_DOUBLE_LAYOUT_PTR32
/* NaN exponent, quiet bit 0x80000 and our 0x10000 marker */
#define JSV_VAL(x) (0x7ff90000|x)
#else
#define JSV_VAL(x) x
#endif

typedef enum {
    JSV_UNDEFINED = JSV_VAL(1),
    JSV_NULL      = JSV_VAL(2),
    JSV_OBJECT    = JSV_VAL(3),
    JSV_STRING    = JSV_VAL(4),
    JSV_NUMBER    = JSV_VAL(5),
    JSV_BOOL      = JSV_VAL(6),
    JSV_VARIANT   = JSV_VAL(7)
} jsval_type_t;

struct _jsval_t {
#ifdef JSVAL_DOUBLE_LAYOUT_PTR32
    union {
        double n;
        struct {
            union {
                IDispatch *obj;
                jsstr_t *str;
                BOOL b;
                VARIANT *v;
                UINT_PTR as_uintptr;
            } u;
            jsval_type_t tag;
        } s;
    } u;
#else
    jsval_type_t type;
    union {
        IDispatch *obj;
        jsstr_t *str;
        double n;
        BOOL b;
        VARIANT *v;
    } u;
#endif
};

#ifdef JSVAL_DOUBLE_LAYOUT_PTR32

C_ASSERT(sizeof(jsval_t) == sizeof(double));

#define __JSVAL_TYPE(x) ((x).u.s.tag)
#define __JSVAL_BOOL(x) ((x).u.s.u.b)
#define __JSVAL_STR(x)  ((x).u.s.u.str)
#define __JSVAL_OBJ(x)  ((x).u.s.u.obj)
#define __JSVAL_VAR(x)  ((x).u.s.u.v)

#else

#define __JSVAL_TYPE(x) ((x).type)
#define __JSVAL_BOOL(x) ((x).u.b)
#define __JSVAL_STR(x)  ((x).u.str)
#define __JSVAL_OBJ(x)  ((x).u.obj)
#define __JSVAL_VAR(x)  ((x).u.v)

#endif

static inline jsval_t jsval_bool(BOOL b)
{
    jsval_t ret;
    __JSVAL_TYPE(ret) = JSV_BOOL;
    __JSVAL_BOOL(ret) = b;
    return ret;
}

static inline jsval_t jsval_string(jsstr_t *str)
{
    jsval_t ret;
    __JSVAL_TYPE(ret) = JSV_STRING;
    __JSVAL_STR(ret) = str;
    return ret;
}

static inline jsval_t jsval_disp(IDispatch *obj)
{
    jsval_t ret;
    __JSVAL_TYPE(ret) = JSV_OBJECT;
    __JSVAL_OBJ(ret) = obj;
    return ret;
}

static inline jsval_t jsval_obj(jsdisp_t *obj)
{
    return jsval_disp(to_disp(obj));
}

static inline jsval_t jsval_null(void)
{
    jsval_t ret;
    __JSVAL_TYPE(ret) = JSV_NULL;
    __JSVAL_BOOL(ret) = FALSE;
    return ret;
}

static inline jsval_t jsval_null_disp(void)
{
    jsval_t ret;
    __JSVAL_TYPE(ret) = JSV_NULL;
    __JSVAL_BOOL(ret) = TRUE;
    return ret;
}

static inline jsval_t jsval_undefined(void)
{
    jsval_t ret;
    __JSVAL_TYPE(ret) = JSV_UNDEFINED;
    return ret;
}

static inline jsval_t jsval_number(double n)
{
    jsval_t ret;
#ifdef JSVAL_DOUBLE_LAYOUT_PTR32
    ret.u.n = n;
    /* normalize NaN value */
    if((ret.u.s.tag & 0x7ff00000) == 0x7ff00000) {
        /* isinf */
        if(ret.u.s.tag & 0xfffff) {
            ret.u.s.tag = 0x7ff80000;
            ret.u.s.u.as_uintptr = ~0;
        }else if(ret.u.s.u.as_uintptr) {
            ret.u.s.tag = 0x7ff80000;
        }
    }
#else
    ret.type = JSV_NUMBER;
    ret.u.n = n;
#endif
    return ret;
}

static inline BOOL is_object_instance(jsval_t v)
{
    return __JSVAL_TYPE(v) == JSV_OBJECT;
}

static inline BOOL is_undefined(jsval_t v)
{
    return __JSVAL_TYPE(v) == JSV_UNDEFINED;
}

static inline BOOL is_null(jsval_t v)
{
    return __JSVAL_TYPE(v) == JSV_NULL;
}

static inline BOOL is_null_disp(jsval_t v)
{
    return is_null(v) && __JSVAL_BOOL(v);
}

static inline BOOL is_string(jsval_t v)
{
    return __JSVAL_TYPE(v) == JSV_STRING;
}

static inline BOOL is_number(jsval_t v)
{
#ifdef JSVAL_DOUBLE_LAYOUT_PTR32
    return (v.u.s.tag & 0x7ff10000) != 0x7ff10000;
#else
    return v.type == JSV_NUMBER;
#endif
}

static inline BOOL is_variant(jsval_t v)
{
    return __JSVAL_TYPE(v) == JSV_VARIANT;
}

static inline BOOL is_bool(jsval_t v)
{
    return __JSVAL_TYPE(v) == JSV_BOOL;
}

static inline jsval_type_t jsval_type(jsval_t v)
{
#ifdef JSVAL_DOUBLE_LAYOUT_PTR32
    return is_number(v) ? JSV_NUMBER : v.u.s.tag;
#else
    return v.type;
#endif
}

static inline IDispatch *get_object(jsval_t v)
{
    return __JSVAL_OBJ(v);
}

static inline double get_number(jsval_t v)
{
    return v.u.n;
}

static inline jsstr_t *get_string(jsval_t v)
{
    return __JSVAL_STR(v);
}

static inline VARIANT *get_variant(jsval_t v)
{
    return __JSVAL_VAR(v);
}

static inline BOOL get_bool(jsval_t v)
{
    return __JSVAL_BOOL(v);
}

HRESULT variant_to_jsval(script_ctx_t*,VARIANT*,jsval_t*);
HRESULT jsval_to_variant(jsval_t,VARIANT*);
void jsval_release(jsval_t);
HRESULT jsval_copy(jsval_t,jsval_t*);

#endif
