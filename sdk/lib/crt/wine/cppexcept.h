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

void WINAPI _CxxThrowException(exception*,const cxx_exception_type*);
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
#define DEFINE_EXCEPTION_TYPE_INFO(type, base_no, cl1, cl2)  \
\
static const cxx_type_info type ## _cxx_type_info = { \
    0, \
    & type ##_type_info, \
    { 0, -1, 0 }, \
    sizeof(type), \
    (cxx_copy_ctor)THISCALL(MSVCRT_ ## type ##_copy_ctor) \
}; \
\
static const cxx_type_info_table type ## _type_info_table = { \
    base_no+1, \
    { \
        & type ## _cxx_type_info, \
        cl1, \
        cl2 \
    } \
}; \
\
static const cxx_exception_type type ## _exception_type = { \
    0, \
    (cxx_copy_ctor)THISCALL(MSVCRT_ ## type ## _dtor), \
    NULL, \
    & type ## _type_info_table \
};

#else

#define DEFINE_EXCEPTION_TYPE_INFO(type, base_no, cl1, cl2)  \
\
static cxx_type_info type ## _cxx_type_info = { \
    0, \
    0xdeadbeef, \
    { 0, -1, 0 }, \
    sizeof(type), \
    0xdeadbeef \
}; \
\
static cxx_type_info_table type ## _type_info_table = { \
    base_no+1, \
    { \
        0xdeadbeef, \
        0xdeadbeef, \
        0xdeadbeef  \
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
    type ## _cxx_type_info.type_info  = (char *)&type ## _type_info - base; \
    type ## _cxx_type_info.copy_ctor  = (char *)MSVCRT_ ## type ## _copy_ctor - base; \
    type ## _type_info_table.info[0]   = (char *)&type ## _cxx_type_info - base; \
    type ## _type_info_table.info[1]   = (char *)cl1 - base; \
    type ## _type_info_table.info[2]   = (char *)cl2 - base; \
    type ## _exception_type.destructor      = (char *)MSVCRT_ ## type ## _dtor - base; \
    type ## _exception_type.type_info_table = (char *)&type ## _type_info_table - base; \
}
#endif

#endif /* __MSVCRT_CPPEXCEPT_H */
