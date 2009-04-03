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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __MSVCRT_CPPEXCEPT_H
#define __MSVCRT_CPPEXCEPT_H

#include <pseh/pseh2.h>

/* Macros to define assembler functions somewhat portably */

#define __ASM_FUNC(name) ".def " __ASM_NAME(name) "; .scl 2; .type 32; .endef"
#define __ASM_NAME(name) "_" name

#ifdef __GNUC__
# define __ASM_GLOBAL_FUNC(name,code) \
      __asm__( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code );
#else  /* __GNUC__ */
# define __ASM_GLOBAL_FUNC(name,code) \
      void __asm_dummy_##name(void) { \
          asm( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code ); \
      }
#endif  /* __GNUC__ */

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10

static inline EXCEPTION_REGISTRATION_RECORD *__wine_push_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
    frame->Next = (struct _EXCEPTION_REGISTRATION_RECORD *)__readfsdword(0);
	__writefsdword(0, (unsigned long)frame);
    return frame->Next;
}

static inline EXCEPTION_REGISTRATION_RECORD *__wine_pop_frame( EXCEPTION_REGISTRATION_RECORD *frame )
{
	__writefsdword(0, (unsigned long)frame->Next);
    return frame->Next;
}

#define __TRY _SEH2_TRY
#define __EXCEPT(func) _SEH2_EXCEPT(func(_SEH2_GetExceptionInformation()))
#define __EXCEPT_PAGE_FAULT _SEH2_EXCEPT(_SEH2_GetExceptionCode() == STATUS_ACCESS_VIOLATION)
#define __EXCEPT_ALL _SEH2_EXCEPT(_SEH_EXECUTE_HANDLER)
#define __ENDTRY _SEH2_END
#define __FINALLY(func) _SEH2_FINALLY { func(!_SEH2_AbnormalTermination()); }

#define CXX_FRAME_MAGIC    0x19930520
#define CXX_EXCEPTION      0xe06d7363

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
    UINT       flags;         /* flags (see below) */
    type_info *type_info;     /* C++ type caught by this block */
    int        offset;        /* stack offset to copy exception object to */
    void     (*handler)();    /* catch block handler code */
} catchblock_info;
#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

/* info about a single try {} block */
typedef struct __tryblock_info
{
    int              start_level;      /* start trylevel of that block */
    int              end_level;        /* end trylevel of that block */
    int              catch_level;      /* initial trylevel of the catch block */
    int              catchblock_count; /* count of catch blocks in array */
    catchblock_info *catchblock;       /* array of catch blocks */
} tryblock_info;

/* info about the unwind handler for a given trylevel */
typedef struct __unwind_info
{
    int    prev;          /* prev trylevel unwind handler, to run after this one */
    void (*handler)();    /* unwind handler */
} unwind_info;

/* descriptor of all try blocks of a given function */
typedef struct __cxx_function_descr
{
    UINT           magic;          /* must be CXX_FRAME_MAGIC */
    UINT           unwind_count;   /* number of unwind handlers */
    unwind_info   *unwind_table;   /* array of unwind handlers */
    UINT           tryblock_count; /* number of try blocks */
    tryblock_info *tryblock;       /* array of try blocks */
    UINT           unknown[3];
} cxx_function_descr;

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
    void                     (*destructor)();    /* exception object destructor */
    cxx_exc_custom_handler     custom_handler;   /* custom handler for this exception */
    const cxx_type_info_table *type_info_table;  /* list of types for this exception object */
} cxx_exception_type;

void _CxxThrowException(exception*,const cxx_exception_type*);

static inline const char *dbgstr_type_info( const type_info *info )
{
    if (!info) return "{}";
    return "{}";/*sprintf( "{vtable=%p name=%s (%s)}",
                             info->vtable, info->mangled, info->name ? info->name : "" );*/
}

/* compute the this pointer for a base class of a given type */
static inline void *get_this_pointer( const this_ptr_offsets *off, void *object )
{
    void *this_ptr;
    int *offset_ptr;

    if (!object) return NULL;
    this_ptr = (char *)object + off->this_offset;
    if (off->vbase_descr >= 0)
    {
        /* move this ptr to vbase descriptor */
        this_ptr = (char *)this_ptr + off->vbase_descr;
        /* and fetch additional offset from vbase descriptor */
        offset_ptr = (int *)(*(char **)this_ptr + off->vbase_offset);
        this_ptr = (char *)this_ptr + *offset_ptr;
    }
    return this_ptr;
}

#endif /* __MSVCRT_CPPEXCEPT_H */
