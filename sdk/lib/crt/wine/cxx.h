/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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

#include "cppexcept.h"

#ifdef _MSC_VER
#define __ASM_VTABLE(name,funcs)
#else
#ifdef _WIN64

#define VTABLE_ADD_FUNC(name) "\t.quad " THISCALL_NAME(name) "\n"

#define __ASM_VTABLE(name,funcs) \
    __asm__(".data\n" \
            "\t.balign 8\n" \
            "\t.quad " __ASM_NAME(#name "_rtti") "\n" \
            "\t.globl " __ASM_NAME(#name "_vtable") "\n" \
            __ASM_NAME(#name "_vtable") ":\n" \
            funcs "\n\t.text")

#else

#define VTABLE_ADD_FUNC(name) "\t.long " THISCALL_NAME(name) "\n"

#define __ASM_VTABLE(name,funcs) \
    __asm__(".data\n" \
            "\t.balign 4\n" \
            "\t.long " __ASM_NAME(#name "_rtti") "\n" \
            "\t.globl " __ASM_NAME(#name "_vtable") "\n" \
            __ASM_NAME(#name "_vtable") ":\n" \
            funcs "\n\t.text")

#endif /* _WIN64 */
#endif // _MSC_VER

#ifndef __x86_64__

#define DEFINE_RTTI_BASE(name, base_classes_no, mangled_name) \
    static type_info name ## _type_info = { \
        &type_info_vtable, \
        NULL, \
        mangled_name \
    }; \
\
static const rtti_base_descriptor name ## _rtti_base_descriptor = { \
    &name ##_type_info, \
    base_classes_no, \
    { 0, -1, 0}, \
    64 \
};

#define DEFINE_RTTI_DATA(name, off, base_classes_no, cl1, cl2, cl3, cl4, cl5, cl6, cl7, cl8, cl9, mangled_name) \
    DEFINE_RTTI_BASE(name, base_classes_no, mangled_name) \
\
static const rtti_base_array name ## _rtti_base_array = { \
    { \
        &name ## _rtti_base_descriptor, \
        cl1, \
        cl2, \
        cl3, \
        cl4, \
        cl5, \
        cl6, \
        cl7, \
        cl8, \
        cl9, \
    } \
}; \
\
static const rtti_object_hierarchy name ## _hierarchy = { \
    0, \
    0, \
    base_classes_no+1, \
    &name ## _rtti_base_array \
}; \
\
const rtti_object_locator name ## _rtti = { \
    0, \
    off, \
    0, \
    &name ## _type_info, \
    &name ## _hierarchy \
};

#else

#define __DEFINE_RTTI_BASE(name, base_classes_no, mangled_name) \
    static type_info name ## _type_info = { \
        &type_info_vtable, \
        NULL, \
        mangled_name \
    }; \
\
static rtti_base_descriptor name ## _rtti_base_descriptor = { \
    0xdeadbeef, \
    base_classes_no, \
    { 0, -1, 0}, \
    64 \
};

#define DEFINE_RTTI_BASE(name, base_classes_no, mangled_name) \
    __DEFINE_RTTI_BASE(name, base_classes_no, mangled_name) \
    \
    static void init_ ## name ## _rtti(char *base) \
    { \
        name ## _rtti_base_descriptor.type_descriptor = (char*)&name ## _type_info - base; \
    }

#define DEFINE_RTTI_DATA(name, off, base_classes_no, cl1, cl2, cl3, cl4, cl5, cl6, cl7, cl8, cl9, mangled_name) \
    __DEFINE_RTTI_BASE(name, base_classes_no, mangled_name) \
\
static rtti_base_array name ## _rtti_base_array = { \
    { \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
    } \
}; \
\
static rtti_object_hierarchy name ## _hierarchy = { \
    0, \
    0, \
    base_classes_no+1, \
    0xdeadbeef \
}; \
\
rtti_object_locator name ## _rtti = { \
    1, \
    off, \
    0, \
    0xdeadbeef, \
    0xdeadbeef, \
    0xdeadbeef \
};\
\
static void init_ ## name ## _rtti(char *base) \
{ \
    name ## _rtti_base_descriptor.type_descriptor = (char*)&name ## _type_info - base; \
    name ## _rtti_base_array.bases[0] = (char*)&name ## _rtti_base_descriptor - base; \
    name ## _rtti_base_array.bases[1] = (char*)cl1 - base; \
    name ## _rtti_base_array.bases[2] = (char*)cl2 - base; \
    name ## _rtti_base_array.bases[3] = (char*)cl3 - base; \
    name ## _rtti_base_array.bases[4] = (char*)cl4 - base; \
    name ## _rtti_base_array.bases[5] = (char*)cl5 - base; \
    name ## _rtti_base_array.bases[6] = (char*)cl6 - base; \
    name ## _rtti_base_array.bases[7] = (char*)cl7 - base; \
    name ## _rtti_base_array.bases[8] = (char*)cl8 - base; \
    name ## _rtti_base_array.bases[9] = (char*)cl9 - base; \
    name ## _hierarchy.base_classes = (char*)&name ## _rtti_base_array - base; \
    name ## _rtti.type_descriptor = (char*)&name ## _type_info - base; \
    name ## _rtti.type_hierarchy = (char*)&name ## _hierarchy - base; \
    name ## _rtti.object_locator = (char*)&name ## _rtti - base; \
}

#endif

#define DEFINE_RTTI_DATA0(name, off, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, mangled_name)
#define DEFINE_RTTI_DATA1(name, off, cl1, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 1, cl1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, mangled_name)
#define DEFINE_RTTI_DATA2(name, off, cl1, cl2, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 2, cl1, cl2, NULL, NULL, NULL, NULL, NULL, NULL, NULL, mangled_name)
#define DEFINE_RTTI_DATA3(name, off, cl1, cl2, cl3, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 3, cl1, cl2, cl3, NULL, NULL, NULL, NULL, NULL, NULL, mangled_name)
#define DEFINE_RTTI_DATA4(name, off, cl1, cl2, cl3, cl4, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 4, cl1, cl2, cl3, cl4, NULL, NULL, NULL, NULL, NULL, mangled_name)
#define DEFINE_RTTI_DATA8(name, off, cl1, cl2, cl3, cl4, cl5, cl6, cl7, cl8, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 8, cl1, cl2, cl3, cl4, cl5, cl6, cl7, cl8, NULL, mangled_name)
#define DEFINE_RTTI_DATA9(name, off, cl1, cl2, cl3, cl4, cl5, cl6, cl7, cl8, cl9, mangled_name) \
    DEFINE_RTTI_DATA(name, off, 9, cl1, cl2, cl3, cl4, cl5, cl6, cl7, cl8, cl9, mangled_name)

#ifndef __x86_64__

typedef struct _rtti_base_descriptor
{
    const type_info *type_descriptor;
    int num_base_classes;
    this_ptr_offsets offsets;    /* offsets for computing the this pointer */
    unsigned int attributes;
} rtti_base_descriptor;

typedef struct _rtti_base_array
{
    const rtti_base_descriptor *bases[10]; /* First element is the class itself */
} rtti_base_array;

typedef struct _rtti_object_hierarchy
{
    unsigned int signature;
    unsigned int attributes;
    int array_len; /* Size of the array pointed to by 'base_classes' */
    const rtti_base_array *base_classes;
} rtti_object_hierarchy;

typedef struct _rtti_object_locator
{
    unsigned int signature;
    int base_class_offset;
    unsigned int flags;
    const type_info *type_descriptor;
    const rtti_object_hierarchy *type_hierarchy;
} rtti_object_locator;

#else

typedef struct
{
    unsigned int type_descriptor;
    int num_base_classes;
    this_ptr_offsets offsets;    /* offsets for computing the this pointer */
    unsigned int attributes;
} rtti_base_descriptor;

typedef struct
{
    unsigned int bases[10]; /* First element is the class itself */
} rtti_base_array;

typedef struct
{
    unsigned int signature;
    unsigned int attributes;
    int array_len; /* Size of the array pointed to by 'base_classes' */
    unsigned int base_classes;
} rtti_object_hierarchy;

typedef struct
{
    unsigned int signature;
    int base_class_offset;
    unsigned int flags;
    unsigned int type_descriptor;
    unsigned int type_hierarchy;
    unsigned int object_locator;
} rtti_object_locator;

#endif

#ifdef __ASM_USE_THISCALL_WRAPPER

#define CALL_VTBL_FUNC(this, off, ret, type, args) ((ret (WINAPI*)type)&vtbl_wrapper_##off)args

extern void *vtbl_wrapper_0;
extern void *vtbl_wrapper_4;
extern void *vtbl_wrapper_8;
extern void *vtbl_wrapper_12;
extern void *vtbl_wrapper_16;
extern void *vtbl_wrapper_20;
extern void *vtbl_wrapper_24;
extern void *vtbl_wrapper_28;
extern void *vtbl_wrapper_32;
extern void *vtbl_wrapper_36;
extern void *vtbl_wrapper_40;
extern void *vtbl_wrapper_44;
extern void *vtbl_wrapper_48;

#else

#define CALL_VTBL_FUNC(this, off, ret, type, args) ((ret (__thiscall***)type)this)[0][off/4]args

#endif

exception* __thiscall exception_ctor(exception*, const char**);

extern const vtable_ptr type_info_vtable;

#ifdef __REACTOS__
void * __thiscall type_info_vector_dtor(type_info * _this, unsigned int flags);
#endif

#define CREATE_TYPE_INFO_VTABLE \
DEFINE_THISCALL_WRAPPER(type_info_vector_dtor,8) \
void * __thiscall type_info_vector_dtor(type_info * _this, unsigned int flags) \
{ \
    if (flags & 2) \
    { \
        /* we have an array, with the number of elements stored before the first object */ \
        INT_PTR i, *ptr = (INT_PTR *)_this - 1; \
\
        for (i = *ptr - 1; i >= 0; i--) free(_this[i].name); \
        free(ptr); \
    } \
    else \
    { \
        free(_this->name); \
        if (flags & 1) free(_this); \
    } \
    return _this; \
} \
\
DEFINE_RTTI_DATA0( type_info, 0, ".?AVtype_info@@" ) \
\
__ASM_BLOCK_BEGIN(type_info_vtables) \
    __ASM_VTABLE(type_info, \
            VTABLE_ADD_FUNC(type_info_vector_dtor)); \
__ASM_BLOCK_END
