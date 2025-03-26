/*
 * msvcrt C++ exception handling
 *
 * Copyright 2002 Alexandre Julliard
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

#ifndef __MSVCRT_CPPEXCEPT_H
#define __MSVCRT_CPPEXCEPT_H

#include "wine/asm.h"

#define CXX_FRAME_MAGIC_VC6 0x19930520
#define CXX_FRAME_MAGIC_VC7 0x19930521
#define CXX_FRAME_MAGIC_VC8 0x19930522
#define CXX_EXCEPTION       0xe06d7363

#define FUNC_DESCR_SYNCHRONOUS  1 /* synchronous exceptions only (built with /EHs and /EHsc) */
#define FUNC_DESCR_NOEXCEPT     4 /* noexcept function */

typedef void (*vtable_ptr)(void);

/* type_info object, see cpp.c for implementation */
typedef struct __type_info
{
  const vtable_ptr *vtable;
  char              *name;        /* Unmangled name, allocated lazily */
  char               mangled[64]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

/* exception object */
typedef struct __exception
{
  const vtable_ptr *vtable;
  char             *name;    /* Name of this exception, always a new copy for each object */
  BOOL              do_free; /* Whether to free 'name' in our dtor */
} exception;

typedef void (*cxx_copy_ctor)(void);

/* offsets for computing the this pointer */
typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

/* complete information about a C++ type */
#ifndef __x86_64__
typedef struct __cxx_type_info
{
    UINT             flags;        /* flags (see CLASS_* flags below) */
    const type_info *type_info;    /* C++ type info */
    this_ptr_offsets offsets;      /* offsets for computing the this pointer */
    unsigned int     size;         /* object size */
    cxx_copy_ctor    copy_ctor;    /* copy constructor */
} cxx_type_info;
#else
typedef struct __cxx_type_info
{
    UINT flags;
    unsigned int type_info;
    this_ptr_offsets offsets;
    unsigned int size;
    unsigned int copy_ctor;
} cxx_type_info;
#endif

#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

/* table of C++ types that apply for a given object */
#ifndef __x86_64__
typedef struct __cxx_type_info_table
{
    UINT                 count;     /* number of types */
    const cxx_type_info *info[3];   /* variable length, we declare it large enough for static RTTI */
} cxx_type_info_table;
#else
typedef struct __cxx_type_info_table
{
    UINT count;
    unsigned int info[3];
} cxx_type_info_table;
#endif

struct __cxx_exception_frame;
struct __cxx_function_descr;

typedef DWORD (*cxx_exc_custom_handler)( PEXCEPTION_RECORD, struct __cxx_exception_frame*,
                                         PCONTEXT, EXCEPTION_REGISTRATION_RECORD**,
                                         const struct __cxx_function_descr*, int nested_trylevel,
                                         EXCEPTION_REGISTRATION_RECORD *nested_frame, DWORD unknown3 );

/* type information for an exception object */
#ifndef __x86_64__
typedef struct __cxx_exception_type
{
    UINT                       flags;            /* TYPE_FLAG flags */
    void                     (*destructor)(void);/* exception object destructor */
    cxx_exc_custom_handler     custom_handler;   /* custom handler for this exception */
    const cxx_type_info_table *type_info_table;  /* list of types for this exception object */
} cxx_exception_type;
#else
typedef struct
{
    UINT flags;
    unsigned int destructor;
    unsigned int custom_handler;
    unsigned int type_info_table;
} cxx_exception_type;
#endif

void WINAPI _CxxThrowException(void*,const cxx_exception_type*);
int CDECL _XcptFilter(NTSTATUS, PEXCEPTION_POINTERS);

static inline const char *dbgstr_type_info( const type_info *info )
{
    if (!info) return "{}";
    return wine_dbg_sprintf( "{vtable=%p name=%s (%s)}",
                             info->vtable, info->mangled, info->name ? info->name : "" );
}

/* compute the this pointer for a base class of a given type */
static inline void *get_this_pointer( const this_ptr_offsets *off, void *object )
{
    if (!object) return NULL;

    if (off->vbase_descr >= 0)
    {
        int *offset_ptr;

        /* move this ptr to vbase descriptor */
        object = (char *)object + off->vbase_descr;
        /* and fetch additional offset from vbase descriptor */
        offset_ptr = (int *)(*(char **)object + off->vbase_offset);
        object = (char *)object + *offset_ptr;
    }

    object = (char *)object + off->this_offset;
    return object;
}

#ifndef __x86_64__
#define DEFINE_CXX_TYPE_INFO(type) \
static const cxx_type_info type ## _cxx_type_info = { \
    0, \
    & type ##_type_info, \
    { 0, -1, 0 }, \
    sizeof(type), \
    (cxx_copy_ctor)THISCALL(type ##_copy_ctor) \
};

#define DEFINE_CXX_EXCEPTION(type, base_no, cl1, cl2, dtor)  \
static const cxx_type_info_table type ## _cxx_type_table = { \
    base_no+1, \
    { \
        & type ## _cxx_type_info, \
        cl1, \
        cl2, \
    } \
}; \
\
static const cxx_exception_type type ## _exception_type = { \
    0, \
    (cxx_copy_ctor)THISCALL(dtor), \
    NULL, \
    & type ## _cxx_type_table \
};

#else

#define DEFINE_CXX_TYPE_INFO(type) \
static cxx_type_info type ## _cxx_type_info = { \
    0, \
    0xdeadbeef, \
    { 0, -1, 0 }, \
    sizeof(type), \
    0xdeadbeef \
}; \
\
static void init_ ## type ## _cxx_type_info(char *base) \
{ \
    type ## _cxx_type_info.type_info  = (char *)&type ## _type_info - base; \
    type ## _cxx_type_info.copy_ctor  = (char *)type ## _copy_ctor - base; \
}

#define DEFINE_CXX_EXCEPTION(type, base_no, cl1, cl2, dtor)  \
static cxx_type_info_table type ## _cxx_type_table = { \
    base_no+1, \
    { \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef, \
    } \
}; \
\
static cxx_exception_type type ##_exception_type = { \
    0, \
    0xdeadbeef, \
    0, \
    0xdeadbeef \
}; \
\
static void init_ ## type ## _cxx(char *base) \
{ \
    init_ ## type ## _cxx_type_info(base); \
    type ## _cxx_type_table.info[0]   = (char *)&type ## _cxx_type_info - base; \
    type ## _cxx_type_table.info[1]   = (char *)cl1 - base; \
    type ## _cxx_type_table.info[2]   = (char *)cl2 - base; \
    type ## _exception_type.destructor      = (char *)dtor - base; \
    type ## _exception_type.type_info_table = (char *)&type ## _cxx_type_table - base; \
}

#endif

#define DEFINE_CXX_DATA(type, base_no, cl1, cl2, dtor) \
DEFINE_CXX_TYPE_INFO(type) \
DEFINE_CXX_EXCEPTION(type, base_no, cl1, cl2, dtor)

#define DEFINE_CXX_EXCEPTION0(name, dtor) \
    DEFINE_CXX_EXCEPTION(name, 0, NULL, NULL, dtor)

#define DEFINE_CXX_DATA0(name, dtor) \
    DEFINE_CXX_DATA(name, 0, NULL, NULL, dtor)
#define DEFINE_CXX_DATA1(name, cl1, dtor) \
    DEFINE_CXX_DATA(name, 1, cl1, NULL, dtor)
#define DEFINE_CXX_DATA2(name, cl1, cl2, dtor) \
    DEFINE_CXX_DATA(name, 2, cl1, cl2, dtor)

#if _MSVCR_VER >= 80
#define EXCEPTION_MANGLED_NAME ".?AVexception@std@@"
#else
#define EXCEPTION_MANGLED_NAME ".?AVexception@@"
#endif

#define CREATE_EXCEPTION_OBJECT(exception_name) \
static exception* __exception_ctor(exception *this, const char *str, const vtable_ptr *vtbl) \
{ \
    if (str) \
    { \
        unsigned int len = strlen(str) + 1; \
        this->name = malloc(len); \
        memcpy(this->name, str, len); \
        this->do_free = TRUE; \
    } \
    else \
    { \
        this->name = NULL; \
        this->do_free = FALSE; \
    } \
    this->vtable = vtbl; \
    return this; \
} \
\
static exception* __exception_copy_ctor(exception *this, const exception *rhs, const vtable_ptr *vtbl) \
{ \
    if (rhs->do_free) \
    { \
        __exception_ctor(this, rhs->name, vtbl); \
    } \
    else \
    { \
        *this = *rhs; \
        this->vtable = vtbl; \
    } \
    return this; \
} \
extern const vtable_ptr exception_name ## _vtable; \
exception* __thiscall exception_name ## _copy_ctor(exception *this, const exception *rhs); \
DEFINE_THISCALL_WRAPPER(exception_name ## _copy_ctor,8) \
exception* __thiscall exception_name ## _copy_ctor(exception *this, const exception *rhs) \
{ \
    return __exception_copy_ctor(this, rhs, & exception_name ## _vtable); \
} \
\
void __thiscall exception_name ## _dtor(exception *this); \
DEFINE_THISCALL_WRAPPER(exception_name ## _dtor,4) \
void __thiscall exception_name ## _dtor(exception *this) \
{ \
    if (this->do_free) free(this->name); \
} \
\
void* __thiscall exception_name ## _vector_dtor(exception *this, unsigned int flags); \
DEFINE_THISCALL_WRAPPER(exception_name ## _vector_dtor,8) \
void* __thiscall exception_name ## _vector_dtor(exception *this, unsigned int flags) \
{ \
    if (flags & 2) \
    { \
        INT_PTR i, *ptr = (INT_PTR *)this - 1; \
\
        for (i = *ptr - 1; i >= 0; i--) exception_name ## _dtor(this + i); \
        operator_delete(ptr); \
    } \
    else \
    { \
        exception_name ## _dtor(this); \
        if (flags & 1) operator_delete(this); \
    } \
    return this; \
} \
\
const char* __thiscall exception_name ## _what(exception *this); \
DEFINE_THISCALL_WRAPPER(exception_name ## _what,4) \
const char* __thiscall exception_name ## _what(exception *this) \
{ \
    return this->name ? this->name : "Unknown exception"; \
} \
\
__ASM_BLOCK_BEGIN(exception_name ## _vtables) \
__ASM_VTABLE(exception_name, \
        VTABLE_ADD_FUNC(exception_name ## _vector_dtor) \
        VTABLE_ADD_FUNC(exception_name ## _what)); \
__ASM_BLOCK_END \
\
DEFINE_RTTI_DATA0(exception_name, 0, EXCEPTION_MANGLED_NAME) \
DEFINE_CXX_TYPE_INFO(exception_name)

#endif /* __MSVCRT_CPPEXCEPT_H */
