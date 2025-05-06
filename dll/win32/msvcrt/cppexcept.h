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

#include <fpieee.h>
#include "cxx.h"

#define CXX_FRAME_MAGIC_VC6 0x19930520
#define CXX_FRAME_MAGIC_VC7 0x19930521
#define CXX_FRAME_MAGIC_VC8 0x19930522
#define CXX_EXCEPTION       0xe06d7363

typedef struct
{
    UINT ip;
    int  state;
} ipmap_info;

#ifndef RTTI_USE_RVA

#define CXX_EXCEPTION_PARAMS 3

/* info about a single catch {} block */
typedef struct
{
    UINT             flags;         /* flags (see below) */
    const type_info *type_info;     /* C++ type caught by this block */
    int              offset;        /* stack offset to copy exception object to */
    void *         (*handler)(void);/* catch block handler code */
} catchblock_info;

/* info about a single try {} block */
typedef struct
{
    int                    start_level;      /* start trylevel of that block */
    int                    end_level;        /* end trylevel of that block */
    int                    catch_level;      /* initial trylevel of the catch block */
    unsigned int           catchblock_count; /* count of catch blocks in array */
    const catchblock_info *catchblock;       /* array of catch blocks */
} tryblock_info;

/* info about the unwind handler for a given trylevel */
typedef struct
{
    int      prev;          /* prev trylevel unwind handler, to run after this one */
    void * (*handler)(void);/* unwind handler */
} unwind_info;

/* descriptor of all try blocks of a given function */
typedef struct
{
    UINT                 magic : 29;     /* must be CXX_FRAME_MAGIC */
    UINT                 bbt_flags : 3;
    UINT                 unwind_count;   /* number of unwind handlers */
    const unwind_info   *unwind_table;   /* array of unwind handlers */
    UINT                 tryblock_count; /* number of try blocks */
    const tryblock_info *tryblock;       /* array of try blocks */
    UINT                 ipmap_count;
    const ipmap_info    *ipmap;
    const void          *expect_list;    /* expected exceptions list when magic >= VC7 */
    UINT                 flags;          /* flags when magic >= VC8 */
} cxx_function_descr;

#else  /* RTTI_USE_RVA */

#define CXX_EXCEPTION_PARAMS 4

typedef struct
{
    UINT flags;
    UINT type_info;
    int  offset;
    UINT handler;
#ifdef _WIN64
    UINT frame;
#endif
} catchblock_info;

typedef struct
{
    int  start_level;
    int  end_level;
    int  catch_level;
    UINT catchblock_count;
    UINT catchblock;
} tryblock_info;

typedef struct
{
    int  prev;
    UINT handler;
} unwind_info;

typedef struct
{
    UINT magic : 29;
    UINT bbt_flags : 3;
    UINT unwind_count;
    UINT unwind_table;
    UINT tryblock_count;
    UINT tryblock;
    UINT ipmap_count;
    UINT ipmap;
    int  unwind_help;
    UINT expect_list;
    UINT flags;
} cxx_function_descr;

#endif  /* RTTI_USE_RVA */

#define FUNC_DESCR_SYNCHRONOUS  1 /* synchronous exceptions only (built with /EHs and /EHsc) */
#define FUNC_DESCR_NOEXCEPT     4 /* noexcept function */

#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

#define TYPE_FLAG_CONST      1
#define TYPE_FLAG_VOLATILE   2
#define TYPE_FLAG_REFERENCE  8

void WINAPI DECLSPEC_NORETURN _CxxThrowException(void*,const cxx_exception_type*);

static inline BOOL is_cxx_exception( EXCEPTION_RECORD *rec )
{
    if (rec->ExceptionCode != CXX_EXCEPTION) return FALSE;
    if (rec->NumberParameters != CXX_EXCEPTION_PARAMS) return FALSE;
    return (rec->ExceptionInformation[0] >= CXX_FRAME_MAGIC_VC6 &&
            rec->ExceptionInformation[0] <= CXX_FRAME_MAGIC_VC8);
}

typedef struct
{
    EXCEPTION_RECORD *rec;
    LONG *ref; /* not binary compatible with native msvcr100 */
} exception_ptr;

void throw_exception(const char*);
void exception_ptr_from_record(exception_ptr*,EXCEPTION_RECORD*);

void __cdecl __ExceptionPtrCreate(exception_ptr*);
void __cdecl __ExceptionPtrDestroy(exception_ptr*);
void __cdecl __ExceptionPtrRethrow(const exception_ptr*);

BOOL __cdecl __uncaught_exception(void);

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

#ifdef __ASM_USE_THISCALL_WRAPPER
extern void call_copy_ctor( void *func, void *this, void *src, int has_vbase );
extern void call_dtor( void *func, void *this );
#else
static inline void call_copy_ctor( void *func, void *this, void *src, int has_vbase )
{
    if (has_vbase)
        ((void (__thiscall*)(void*, void*, BOOL))func)(this, src, 1);
    else
        ((void (__thiscall*)(void*, void*))func)(this, src);
}
static inline void call_dtor( void *func, void *this )
{
    ((void (__thiscall*)(void*))func)( this );
}
#endif

/* check if the exception type is caught by a given catch block, and return the type that matched */
static inline const cxx_type_info *find_caught_type( cxx_exception_type *exc_type, uintptr_t base,
                                                     const type_info *catch_ti, UINT catch_flags )
{
    const cxx_type_info_table *type_info_table = rtti_rva( exc_type->type_info_table, base );
    UINT i;

    for (i = 0; i < type_info_table->count; i++)
    {
        const cxx_type_info *type = rtti_rva( type_info_table->info[i], base );
        const type_info *ti = rtti_rva( type->type_info, base );

        if (!catch_ti) return type;   /* catch(...) matches any type */
        if (catch_ti != ti)
        {
            if (strcmp( catch_ti->mangled, ti->mangled )) continue;
        }
        /* type is the same, now check the flags */
        if ((exc_type->flags & TYPE_FLAG_CONST) &&
            !(catch_flags & TYPE_FLAG_CONST)) continue;
        if ((exc_type->flags & TYPE_FLAG_VOLATILE) &&
            !(catch_flags & TYPE_FLAG_VOLATILE)) continue;
        return type;  /* it matched */
    }
    return NULL;
}

/* copy the exception object where the catch block wants it */
static inline void copy_exception( void *object, void **dest, UINT catch_flags,
                                   const cxx_type_info *type, uintptr_t base )
{
    if (catch_flags & TYPE_FLAG_REFERENCE)
    {
        *dest = get_this_pointer( &type->offsets, object );
    }
    else if (type->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memmove( dest, object, type->size );
        /* if it is a pointer, adjust it */
        if (type->size == sizeof(void*)) *dest = get_this_pointer( &type->offsets, *dest );
    }
    else  /* copy the object */
    {
        if (type->copy_ctor)
            call_copy_ctor( rtti_rva( type->copy_ctor, base ), dest,
                            get_this_pointer( &type->offsets, object ),
                            (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS) );
        else
            memmove( dest, get_this_pointer( &type->offsets, object ), type->size );
    }
}

#define TRACE_EXCEPTION_TYPE(type,base) do { \
    const cxx_type_info_table *table = rtti_rva( type->type_info_table, base ); \
    unsigned int i; \
    TRACE( "flags %x destr %p handler %p type info %p\n", \
           type->flags, rtti_rva( type->destructor, base ), \
           type->custom_handler ? rtti_rva( type->custom_handler, base ) : NULL, table ); \
    for (i = 0; i < table->count; i++) \
    { \
        const cxx_type_info *type = rtti_rva( table->info[i], base ); \
        const type_info *info = rtti_rva( type->type_info, base ); \
        TRACE( "    %d: flags %x type %p %s offsets %d,%d,%d size %d copy ctor %p\n", \
               i, type->flags, info, dbgstr_type_info( info ), \
               type->offsets.this_offset, type->offsets.vbase_descr, type->offsets.vbase_offset, \
               type->size, rtti_rva( type->copy_ctor, base )); \
    } \
} while(0)

extern void dump_function_descr( const cxx_function_descr *descr, uintptr_t base );
extern void *find_catch_handler( void *object, uintptr_t frame, uintptr_t exc_base,
                                 const tryblock_info *tryblock,
                                 cxx_exception_type *exc_type, uintptr_t image_base );
extern int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                              int (__cdecl *handler)(_FPIEEE_RECORD*) );
#ifndef __i386__
extern void *call_catch_handler( EXCEPTION_RECORD *rec );
extern void *call_unwind_handler( void *func, uintptr_t frame, DISPATCHER_CONTEXT *dispatch );
extern ULONG_PTR get_exception_pc( DISPATCHER_CONTEXT *dispatch );
#endif

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
DEFINE_THISCALL_WRAPPER(exception_name ## _copy_ctor,8) \
exception* __thiscall exception_name ## _copy_ctor(exception *this, const exception *rhs) \
{ \
    return __exception_copy_ctor(this, rhs, & exception_name ## _vtable); \
} \
\
DEFINE_THISCALL_WRAPPER(exception_name ## _dtor,4) \
void __thiscall exception_name ## _dtor(exception *this) \
{ \
    if (this->do_free) free(this->name); \
} \
\
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
DEFINE_RTTI_DATA0(exception_name, 0, EXCEPTION_MANGLED_NAME)

#endif /* __MSVCRT_CPPEXCEPT_H */
