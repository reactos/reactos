/* $Id: cpp.c,v 1.1 2003/04/30 22:07:30 gvg Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS C runtime
 * FILE:        cpp.c
 * PURPOSE:     C++ runtime
 * PROGRAMMERS: Ge van Geldorp (ge@gse.nl)
 * NOTES:       Copied from Wine and slightly adapter for ReactOS
 */

#include <windows.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/stddef.h>

#define CXX_FRAME_MAGIC    0x19930520
#define CXX_EXCEPTION      0xe06d7363

typedef void (*v_table_ptr)();

typedef struct __type_info
{
  v_table_ptr *vtable;
  void *data;
  char name[1];
} type_info;

/* the exception frame used by CxxFrameHandler */
typedef struct __cxx_exception_frame
{
    EXCEPTION_REGISTRATION frame;    /* the standard exception frame */
    int                    trylevel;
    DWORD                  ebp;
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

/* complete information about a C++ type */
typedef struct __cxx_type_info
{
    UINT        flags;         /* flags (see CLASS_* flags below) */
    type_info  *type_info;     /* C++ type info */
    int         this_offset;   /* offset of base class this pointer from start of object */
    int         vbase_descr;   /* offset of virtual base class descriptor */
    int         vbase_offset;  /* offset of this pointer offset in virtual base class descriptor */
    size_t      size;          /* object size */
    void      (*copy_ctor)();  /* copy constructor */
} cxx_type_info;
#define CLASS_IS_SIMPLE_TYPE          1
#define CLASS_HAS_VIRTUAL_BASE_CLASS  4

/* table of C++ types that apply for a given object */
typedef struct __cxx_type_info_table
{
    UINT           count;     /* number of types */
    cxx_type_info *info[1];   /* array of types */
} cxx_type_info_table;

typedef DWORD (*cxx_exc_custom_handler)( PEXCEPTION_RECORD, cxx_exception_frame*,
                                         PCONTEXT, PEXCEPTION_REGISTRATION*,
                                         cxx_function_descr*, int nested_trylevel,
                                         PEXCEPTION_REGISTRATION nested_frame,
                                         DWORD unknown3 );

/* type information for an exception object */
typedef struct __cxx_exception_type
{
    UINT                   flags;            /* TYPE_FLAG flags */
    void                 (*destructor)();    /* exception object destructor */
    cxx_exc_custom_handler custom_handler;   /* custom handler for this exception */
    cxx_type_info_table   *type_info_table;  /* list of types for this exception object */
} cxx_exception_type;

/* ??1type_info@@UAE@XZ (MSVCRT.@) */
void MSVCRT_type_info_dtor(type_info * _this)
{
  if (_this->data)
    {
    free(_this->data);
    }
}

static DWORD cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                                PCONTEXT exc_context, PEXCEPTION_REGISTRATION* dispatch,
                                cxx_function_descr *descr, PEXCEPTION_REGISTRATION nested_frame,
                                int nested_trylevel, CONTEXT_X86 *context );

/* call a function with a given ebp */
inline static void *call_ebp_func( void *func, void *ebp )
{
    void *ret;
    __asm__ __volatile__ ("pushl %%ebp; movl %2,%%ebp; call *%%eax; popl %%ebp" \
                          : "=a" (ret) : "0" (func), "g" (ebp) : "ecx", "edx", "memory" );
    return ret;
}

/* call a copy constructor */
inline static void call_copy_ctor( void *func, void *this, void *src, int has_vbase )
{
#if 0
    TRACE( "calling copy ctor %p object %p src %p\n", func, this, src );
#endif
    if (has_vbase)
        /* in that case copy ctor takes an extra bool indicating whether to copy the base class */
        __asm__ __volatile__("pushl $1; pushl %2; call *%0"
                             : : "r" (func), "c" (this), "g" (src) : "eax", "edx", "memory" );
    else
        __asm__ __volatile__("pushl %2; call *%0"
                             : : "r" (func), "c" (this), "g" (src) : "eax", "edx", "memory" );
}

/* call the destructor of the exception object */
inline static void call_dtor( void *func, void *object )
{
    __asm__ __volatile__("call *%0" : : "r" (func), "c" (object) : "eax", "edx", "memory" );
}


static void dump_type( cxx_type_info *type )
{
#if 0
    DPRINTF( "flags %x type %p", type->flags, type->type_info );
    if (type->type_info) DPRINTF( " (%p %s)", type->type_info->data, type->type_info->name );
    DPRINTF( " offset %d vbase %d,%d size %d copy ctor %p\n", type->this_offset,
             type->vbase_descr, type->vbase_offset, type->size, type->copy_ctor );
#endif
}

static void dump_exception_type( cxx_exception_type *type )
{
#if 0
    int i;

    DPRINTF( "exception type:\n" );
    DPRINTF( "flags %x destr %p handler %p type info %p\n",
             type->flags, type->destructor, type->custom_handler, type->type_info_table );
    for (i = 0; i < type->type_info_table->count; i++)
    {
        DPRINTF( "    %d: ", i );
        dump_type( type->type_info_table->info[i] );
    }
#endif
}

static void dump_function_descr( cxx_function_descr *descr, cxx_exception_type *info )
{
#if 0
    int i, j;

    DPRINTF( "function descr:\n" );
    DPRINTF( "magic %x\n", descr->magic );
    DPRINTF( "unwind table: %p %d\n", descr->unwind_table, descr->unwind_count );
    for (i = 0; i < descr->unwind_count; i++)
    {
        DPRINTF( "    %d: prev %d func %p\n", i,
                 descr->unwind_table[i].prev, descr->unwind_table[i].handler );
    }
    DPRINTF( "try table: %p %d\n", descr->tryblock, descr->tryblock_count );
    for (i = 0; i < descr->tryblock_count; i++)
    {
        DPRINTF( "    %d: start %d end %d catchlevel %d catch %p %d\n", i,
                 descr->tryblock[i].start_level, descr->tryblock[i].end_level,
                 descr->tryblock[i].catch_level, descr->tryblock[i].catchblock,
                 descr->tryblock[i].catchblock_count );
        for (j = 0; j < descr->tryblock[i].catchblock_count; j++)
        {
            catchblock_info *ptr = &descr->tryblock[i].catchblock[j];
            DPRINTF( "        %d: flags %x offset %d handler %p type %p",
                     j, ptr->flags, ptr->offset, ptr->handler, ptr->type_info );
            if (ptr->type_info) DPRINTF( " (%p %s)", ptr->type_info->data, ptr->type_info->name );
            DPRINTF( "\n" );
        }
    }
#endif
}

/* compute the this pointer for a base class of a given type */
static void *get_this_pointer( cxx_type_info *type, void *object )
{
    void *this_ptr;
    int *offset_ptr;

    if (!object) return NULL;
    this_ptr = (char *)object + type->this_offset;
    if (type->vbase_descr >= 0)
    {
        /* move this ptr to vbase descriptor */
        this_ptr = (char *)this_ptr + type->vbase_descr;
        /* and fetch additional offset from vbase descriptor */
        offset_ptr = (int *)(*(char **)this_ptr + type->vbase_offset);
        this_ptr = (char *)this_ptr + *offset_ptr;
    }
    return this_ptr;
}

/* check if the exception type is caught by a given catch block, and return the type that matched */
static cxx_type_info *find_caught_type( cxx_exception_type *exc_type, catchblock_info *catchblock )
{
    UINT i;

    for (i = 0; i < exc_type->type_info_table->count; i++)
    {
        cxx_type_info *type = exc_type->type_info_table->info[i];

        if (!catchblock->type_info) return type;   /* catch(...) matches any type */
        if (catchblock->type_info != type->type_info)
        {
            if (strcmp( catchblock->type_info->name, type->type_info->name )) continue;
        }
        /* type is the same, now check the flags */
        if ((exc_type->flags & TYPE_FLAG_CONST) &&
            !(catchblock->flags & TYPE_FLAG_CONST)) continue;
        if ((exc_type->flags & TYPE_FLAG_VOLATILE) &&
            !(catchblock->flags & TYPE_FLAG_VOLATILE)) continue;
        return type;  /* it matched */
    }
    return NULL;
}


/* copy the exception object where the catch block wants it */
static void copy_exception( void *object, cxx_exception_frame *frame,
                            catchblock_info *catchblock, cxx_type_info *type )
{
    void **dest_ptr;

    if (!catchblock->type_info || !catchblock->type_info->name[0]) return;
    if (!catchblock->offset) return;
    dest_ptr = (void **)((char *)&frame->ebp + catchblock->offset);

    if (catchblock->flags & TYPE_FLAG_REFERENCE)
    {
        *dest_ptr = get_this_pointer( type, object );
    }
    else if (type->flags & CLASS_IS_SIMPLE_TYPE)
    {
        memmove( dest_ptr, object, type->size );
        /* if it is a pointer, adjust it */
        if (type->size == sizeof(void *)) *dest_ptr = get_this_pointer( type, *dest_ptr );
    }
    else  /* copy the object */
    {
        if (type->copy_ctor)
            call_copy_ctor( type->copy_ctor, dest_ptr, get_this_pointer(type,object),
                            (type->flags & CLASS_HAS_VIRTUAL_BASE_CLASS) );
        else
            memmove( dest_ptr, get_this_pointer(type,object), type->size );
    }
}

/* unwind the local function up to a given trylevel */
static void cxx_local_unwind( cxx_exception_frame* frame, cxx_function_descr *descr, int last_level)
{
    void (*handler)();
    int trylevel = frame->trylevel;

    while (trylevel != last_level)
    {
        if (trylevel < 0 || trylevel >= descr->unwind_count)
        {
            OutputDebugString( "invalid trylevel\n" );
            _exit(1);
        }
        handler = descr->unwind_table[trylevel].handler;
        if (handler)
        {
#if 0
            TRACE( "calling unwind handler %p trylevel %d last %d ebp %p\n",
                   handler, trylevel, last_level, &frame->ebp );
#endif
            call_ebp_func( handler, &frame->ebp );
        }
        trylevel = descr->unwind_table[trylevel].prev;
    }
    frame->trylevel = last_level;
}

/* exception frame for nested exceptions in catch block */
struct catch_func_nested_frame
{
    EXCEPTION_REGISTRATION frame;     /* standard exception frame */
    EXCEPTION_RECORD      *prev_rec;  /* previous record to restore in thread data */
    cxx_exception_frame   *cxx_frame; /* frame of parent exception */
    cxx_function_descr    *descr;     /* descriptor of parent exception */
    int                    trylevel;  /* current try level */
};

/* handler for exceptions happening while calling a catch function */
static EXCEPTION_DISPOSITION catch_function_nested_handler(PEXCEPTION_RECORD rec,
                                                           PEXCEPTION_REGISTRATION frame,
                                                           PCONTEXT context,
                                                           PVOID DispatcherContext)
{
    struct catch_func_nested_frame *nested_frame = (struct catch_func_nested_frame *)frame;

    if (rec->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        GetThreadData()->exc_record = nested_frame->prev_rec;
        return ExceptionContinueSearch;
    }
    else
    {
#if 0
        TRACE( "got nested exception in catch function\n" );
#endif
        return cxx_frame_handler( rec, nested_frame->cxx_frame, context,
                                  NULL, nested_frame->descr, &nested_frame->frame,
                                  nested_frame->trylevel, context );
    }
}

static inline PEXCEPTION_REGISTRATION __wine_push_frame( PEXCEPTION_REGISTRATION frame )
{
#if defined(__GNUC__) && defined(__i386__)
    PEXCEPTION_REGISTRATION prev;
    __asm__ __volatile__(".byte 0x64\n\tmovl (0),%0"
                         "\n\tmovl %0,(%1)"
                         "\n\t.byte 0x64\n\tmovl %1,(0)"
                         : "=&r" (prev) : "r" (frame) : "memory" );
    return prev;
#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    frame->prev = (void *)teb->ExceptionList;
    teb->ExceptionList = (void *)frame;
    return frame->prev;
#endif
}

static inline PEXCEPTION_REGISTRATION __wine_pop_frame( PEXCEPTION_REGISTRATION frame )
{
#if defined(__GNUC__) && defined(__i386__)
    __asm__ __volatile__(".byte 0x64\n\tmovl %0,(0)"
                         : : "r" (frame->prev) : "memory" );
    return frame->prev;

#else
    NT_TIB *teb = (NT_TIB *)NtCurrentTeb();
    teb->ExceptionList = (void *)frame->prev;
    return frame->prev;
#endif
}

/* find and call the appropriate catch block for an exception */
/* returns the address to continue execution to after the catch block was called */
inline static void *call_catch_block( PEXCEPTION_RECORD rec, cxx_exception_frame *frame,
                                      cxx_function_descr *descr, int nested_trylevel,
                                      cxx_exception_type *info )
{
    int i, j;
    void *addr, *object = (void *)rec->ExceptionInformation[1];
    struct catch_func_nested_frame nested_frame;
    int trylevel = frame->trylevel;
    PTHREADDATA thread_data = GetThreadData();

    for (i = 0; i < descr->tryblock_count; i++)
    {
        tryblock_info *tryblock = &descr->tryblock[i];

        if (trylevel < tryblock->start_level) continue;
        if (trylevel > tryblock->end_level) continue;

        /* got a try block */
        for (j = 0; j < tryblock->catchblock_count; j++)
        {
            catchblock_info *catchblock = &tryblock->catchblock[j];
            cxx_type_info *type = find_caught_type( info, catchblock );
            if (!type) continue;

#if 0
            TRACE( "matched type %p in tryblock %d catchblock %d\n", type, i, j );
#endif

            /* copy the exception to its destination on the stack */
            copy_exception( object, frame, catchblock, type );

            /* unwind the stack */
            RtlUnwind( &(frame->frame), 0, rec, 0 );
            cxx_local_unwind( frame, descr, tryblock->start_level );
            frame->trylevel = tryblock->end_level + 1;

            /* call the catch block */
#if 0
            TRACE( "calling catch block %p for type %p addr %p ebp %p\n",
                   catchblock, type, catchblock->handler, &frame->ebp );
#endif

            /* setup an exception block for nested exceptions */

            nested_frame.frame.handler = catch_function_nested_handler;
            nested_frame.prev_rec  = thread_data->exc_record;
            nested_frame.cxx_frame = frame;
            nested_frame.descr     = descr;
            nested_frame.trylevel  = nested_trylevel + 1;

            __wine_push_frame( &nested_frame.frame );
            thread_data->exc_record = rec;
            addr = call_ebp_func( catchblock->handler, &frame->ebp );
            thread_data->exc_record = nested_frame.prev_rec;
            __wine_pop_frame( &nested_frame.frame );

            if (info->destructor) call_dtor( info->destructor, object );
#if 0
            TRACE( "done, continuing at %p\n", addr );
#endif
            return addr;
        }
    }
    return NULL;
}


/*********************************************************************
 *		cxx_frame_handler
 *
 * Implementation of __CxxFrameHandler.
 */
static DWORD cxx_frame_handler( PEXCEPTION_RECORD rec, cxx_exception_frame* frame,
                                PCONTEXT exc_context, PEXCEPTION_REGISTRATION* dispatch,
                                cxx_function_descr *descr, PEXCEPTION_REGISTRATION nested_frame,
                                int nested_trylevel, CONTEXT_X86 *context )
{
    cxx_exception_type *exc_type;
    void *next_ip;

    if (descr->magic != CXX_FRAME_MAGIC)
    {
        OutputDebugString( "invalid frame magic\n" );
        return ExceptionContinueSearch;
    }
    if (rec->ExceptionFlags & (EH_UNWINDING|EH_EXIT_UNWIND))
    {
        if (descr->unwind_count && !nested_trylevel) cxx_local_unwind( frame, descr, -1 );
        return ExceptionContinueSearch;
    }
    if (!descr->tryblock_count) return ExceptionContinueSearch;

    exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];
    if (rec->ExceptionCode == CXX_EXCEPTION &&
        rec->ExceptionInformation[0] > CXX_FRAME_MAGIC &&
        exc_type->custom_handler)
    {
        return exc_type->custom_handler( rec, frame, exc_context, dispatch,
                                         descr, nested_trylevel, nested_frame, 0 );
    }

    if (!exc_type)  /* nested exception, fetch info from original exception */
    {
        rec = GetThreadData()->exc_record;
        exc_type = (cxx_exception_type *)rec->ExceptionInformation[2];
    }

#if 0
    if (TRACE_ON(seh))
    {
        TRACE("handling C++ exception rec %p frame %p trylevel %d descr %p nested_frame %p\n",
              rec, frame, frame->trylevel, descr, nested_frame );
        dump_exception_type( exc_type );
        dump_function_descr( descr, exc_type );
    }
#endif

    next_ip = call_catch_block( rec, frame, descr, frame->trylevel, exc_type );

    if (!next_ip) return ExceptionContinueSearch;
    rec->ExceptionFlags &= ~EH_NONCONTINUABLE;
    context->Eip = (DWORD)next_ip;
    context->Ebp = (DWORD)&frame->ebp;
    context->Esp = ((DWORD*)frame)[-1];
    return ExceptionContinueExecution;
}


/*********************************************************************
 *		__CxxFrameHandler (MSVCRT.@)
 */
void __CxxFrameHandler( PEXCEPTION_RECORD rec, PEXCEPTION_REGISTRATION frame,
                        PCONTEXT exc_context, PEXCEPTION_REGISTRATION* dispatch,
                        CONTEXT_X86 *context )
{
    cxx_function_descr *descr = (cxx_function_descr *)context->Eax;
    context->Eax = cxx_frame_handler( rec, (cxx_exception_frame *)frame,
                                      exc_context, dispatch, descr, NULL, 0, context );
}

/*********************************************************************
 *		_CxxThrowException (MSVCRT.@)
 */
void _CxxThrowException( void *object, cxx_exception_type *type )
{
    DWORD args[3];

    args[0] = CXX_FRAME_MAGIC;
    args[1] = (DWORD)object;
    args[2] = (DWORD)type;
    RaiseException( CXX_EXCEPTION, EH_NONCONTINUABLE, 3, args );
}
