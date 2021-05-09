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

#define CXX_FRAME_MAGIC_VC6 0x19930520
#define CXX_FRAME_MAGIC_VC7 0x19930521
#define CXX_FRAME_MAGIC_VC8 0x19930522
#define CXX_EXCEPTION       0xe06d7363

/* Macros to define assembler functions somewhat portably */

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10

typedef void (*vtable_ptr)();

/* type_info object, see cpp.c for inplementation */
typedef struct __type_info
{
  const vtable_ptr *vtable;
  char              *name;        /* Unmangled name, allocated lazily */
  char               mangled[32]; /* Variable length, but we declare it large enough for static RTTI */
} type_info;

/* exception object */
typedef struct __exception
{
  const vtable_ptr *vtable;
  char             *name;    /* Name of this exception, always a new copy for each object */
  int               do_free; /* Whether to free 'name' in our dtor */
} exception;

/* the exception frame used by CxxFrameHandler */
typedef struct __cxx_exception_frame
{
    EXCEPTION_REGISTRATION_RECORD  frame;    /* the standard exception frame */
    int                            trylevel;
    DWORD                          ebp;
} cxx_exception_frame;

/* info about a single catch {} block */
typedef struct __catchblock_info
{
    UINT             flags;         /* flags (see below) */
    const type_info *type_info;     /* C++ type caught by this block */
    int              offset;        /* stack offset to copy exception object to */
    void           (*handler)(void);/* catch block handler code */
} catchblock_info;
#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

/* info about a single try {} block */
typedef struct __tryblock_info
{
    int                    start_level;      /* start trylevel of that block */
    int                    end_level;        /* end trylevel of that block */
    int                    catch_level;      /* initial trylevel of the catch block */
    int                    catchblock_count; /* count of catch blocks in array */
    const catchblock_info *catchblock;       /* array of catch blocks */
} tryblock_info;

/* info about the unwind handler for a given trylevel */
typedef struct __unwind_info
{
    int    prev;          /* prev trylevel unwind handler, to run after this one */
    void (*handler)(void);/* unwind handler */
} unwind_info;

/* descriptor of all try blocks of a given function */
typedef struct __cxx_function_descr
{
    UINT                 magic;          /* must be CXX_FRAME_MAGIC */
    UINT                 unwind_count;   /* number of unwind handlers */
    const unwind_info   *unwind_table;   /* array of unwind handlers */
    UINT                 tryblock_count; /* number of try blocks */
    const tryblock_info *tryblock;       /* array of try blocks */
    UINT                 ipmap_count;
    const void          *ipmap;
    const void          *expect_list;    /* expected exceptions list when magic >= VC7 */
    UINT                 flags;          /* flags when magic >= VC8 */
} cxx_function_descr;

#define FUNC_DESCR_SYNCHRONOUS  1        /* synchronous exceptions only (built with /EHs) */

typedef void (*cxx_copy_ctor)(void);

/* offsets for computing the this pointer */
typedef struct
{
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
} this_ptr_offsets;

/* complete information about a C++ type */
typedef struct __cxx_type_info
{
    UINT             flags;        /* flags (see CLASS_* flags below) */
    const type_info *type_info;    /* C++ type info */
    this_ptr_offsets offsets;      /* offsets for computing the this pointer */
    unsigned int     size;         /* object size */
    cxx_copy_ctor    copy_ctor;    /* copy constructor */
} cxx_type_info;
#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

/* table of C++ types that apply for a given object */
typedef struct __cxx_type_info_table
{
    UINT                 count;     /* number of types */
    const cxx_type_info *info[3];   /* variable length, we declare it large enough for static RTTI */
} cxx_type_info_table;

typedef DWORD (*cxx_exc_custom_handler)( PEXCEPTION_RECORD, cxx_exception_frame*,
                                         PCONTEXT, EXCEPTION_REGISTRATION_RECORD**,
                                         const cxx_function_descr*, int nested_trylevel,
                                         EXCEPTION_REGISTRATION_RECORD *nested_frame, DWORD unknown3 );

/* type information for an exception object */
typedef struct __cxx_exception_type
{
    UINT                       flags;            /* TYPE_FLAG flags */
    void                     (*destructor)(void);/* exception object destructor */
    cxx_exc_custom_handler     custom_handler;   /* custom handler for this exception */
    const cxx_type_info_table *type_info_table;  /* list of types for this exception object */
} cxx_exception_type;

void WINAPI _CxxThrowException(exception*,const cxx_exception_type*);
int CDECL _XcptFilter(NTSTATUS, PEXCEPTION_POINTERS);
int CDECL __CppXcptFilter(NTSTATUS, PEXCEPTION_POINTERS);

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

#endif /* __MSVCRT_CPPEXCEPT_H */
