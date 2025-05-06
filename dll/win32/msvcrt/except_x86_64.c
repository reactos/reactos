/*
 * msvcrt C++ exception handling
 *
 * Copyright 2011 Alexandre Julliard
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

#if defined(__x86_64__) && !defined(__arm64ec__)

#include <stdarg.h>
#include <fpieee.h>
#define longjmp ms_longjmp  /* avoid prototype mismatch */
#include <setjmp.h>
#undef longjmp

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/exception.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);

extern void *call_exc_handler( void *handler, ULONG_PTR frame, UINT flags );
__ASM_GLOBAL_FUNC( call_exc_handler,
                   "subq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x28\n\t")
                   __ASM_SEH(".seh_stackalloc 0x28\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "movq %rcx, 0x0(%rsp)\n\t"
                   "movl %r8d, 0x8(%rsp)\n\t"
                   "movq %rdx, 0x10(%rsp)\n\t"
                   "callq *%rcx\n\t"
                   "addq $0x28,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -0x28\n\t")
                   "ret" )


/*******************************************************************
 *		call_catch_handler
 */
void *call_catch_handler( EXCEPTION_RECORD *rec )
{
    ULONG_PTR frame = rec->ExceptionInformation[1];
    void *handler = (void *)rec->ExceptionInformation[5];

    TRACE( "calling %p frame %Ix\n", handler, frame );
    return call_exc_handler( handler, frame, 0x100 );
}


/*******************************************************************
 *		call_unwind_handler
 */
void *call_unwind_handler( void *handler, ULONG_PTR frame, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "calling %p frame %Ix\n", handler, frame );
    return call_exc_handler( handler, frame, 0x100 );
}


/*******************************************************************
 *		get_exception_pc
 */
ULONG_PTR get_exception_pc( DISPATCHER_CONTEXT *dispatch )
{
    return dispatch->ControlPc;
}


/*******************************************************************
 *		longjmp (MSVCRT.@)
 */
#ifndef __WINE_PE_BUILD
void __cdecl longjmp( _JUMP_BUFFER *jmp, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;
    if (jmp->Frame)
    {
        rec.ExceptionCode = STATUS_LONGJUMP;
        rec.ExceptionFlags = 0;
        rec.ExceptionRecord = NULL;
        rec.ExceptionAddress = NULL;
        rec.NumberParameters = 1;
        rec.ExceptionInformation[0] = (DWORD_PTR)jmp;
        RtlUnwind( (void *)jmp->Frame, (void *)jmp->Rip, &rec, IntToPtr(retval) );
    }
    __wine_longjmp( (__wine_jmp_buf *)jmp, retval );
}
#endif

/*******************************************************************
 *		_local_unwind (MSVCRT.@)
 */
void __cdecl _local_unwind( void *frame, void *target )
{
    RtlUnwind( frame, target, NULL, 0 );
}

/*********************************************************************
 *              handle_fpieee_flt
 */
int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FIXME("(%lx %p %p) opcode: %#I64x\n", exception_code, ep, handler,
            *(ULONG64*)ep->ContextRecord->Rip);
    return EXCEPTION_CONTINUE_SEARCH;
}

#if _MSVCR_VER>=110 && _MSVCR_VER<=120
/*********************************************************************
 *  __crtCapturePreviousContext (MSVCR110.@)
 */
void __cdecl __crtCapturePreviousContext( CONTEXT *ctx )
{
    UNWIND_HISTORY_TABLE table;
    RUNTIME_FUNCTION *func;
    PEXCEPTION_ROUTINE handler;
    ULONG_PTR frame, base;
    void *data;
    ULONG i;

    RtlCaptureContext( ctx );
    for (i = 0; i < 2; i++)
    {
        if (!(func = RtlLookupFunctionEntry( ctx->Rip, &base, &table ))) break;
        if (RtlVirtualUnwind2( UNW_FLAG_NHANDLER, base, ctx->Rip, func, ctx, NULL,
                               &data, &frame, NULL, NULL, NULL, &handler, 0 )) break;
        if (!ctx->Rip) break;
        if (!frame) break;
    }
}
#endif

#endif  /* __x86_64__ */
