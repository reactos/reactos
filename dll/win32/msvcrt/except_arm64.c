/*
 * msvcrt C++ exception handling
 *
 * Copyright 2011 Alexandre Julliard
 * Copyright 2013 André Hentschel
 * Copyright 2017 Martin Storsjo
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

#ifdef __aarch64__

#include <setjmp.h>
#include <stdarg.h>
#include <fpieee.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "excpt.h"
#include "wine/debug.h"

#include "cppexcept.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);


extern void *call_exc_handler( void *handler, ULONG_PTR frame, UINT flags, BYTE *nonvol_regs );
__ASM_GLOBAL_FUNC( call_exc_handler,
                   "stp x29, x30, [sp, #-96]!\n\t"
                   ".seh_save_fplr_x 96\n\t"
                   "stp x19, x20, [sp, #16]\n\t"
                   ".seh_save_regp x19, 16\n\t"
                   "stp x21, x22, [sp, #32]\n\t"
                   ".seh_save_regp x21, 32\n\t"
                   "stp x23, x24, [sp, #48]\n\t"
                   ".seh_save_regp x23, 48\n\t"
                   "stp x25, x26, [sp, #64]\n\t"
                   ".seh_save_regp x25, 64\n\t"
                   "stp x27, x28, [sp, #80]\n\t"
                   ".seh_save_regp x27, 80\n\t"
                   "str x1, [sp, #-16]!\n\t"
                   ".seh_stackalloc 16\n\t"
                   ".seh_endprologue\n\t"
                   "ldp x19, x20, [x3, #0]\n\t" /* nonvolatile regs */
                   "ldp x21, x22, [x3, #16]\n\t"
                   "ldp x23, x24, [x3, #32]\n\t"
                   "ldp x25, x26, [x3, #48]\n\t"
                   "ldp x27, x28, [x3, #64]\n\t"
                   "ldr x29, [x3, #80]\n\t"
                   "blr x0\n\t"
                   "add sp, sp, 16\n\t"
                   "ldp x19, x20, [sp, #16]\n\t"
                   "ldp x21, x22, [sp, #32]\n\t"
                   "ldp x23, x24, [sp, #48]\n\t"
                   "ldp x25, x26, [sp, #64]\n\t"
                   "ldp x27, x28, [sp, #80]\n\t"
                   "ldp x29, x30, [sp], #96\n\t"
                   "ret" )


/*******************************************************************
 *		call_catch_handler
 */
void *call_catch_handler( EXCEPTION_RECORD *rec )
{
    ULONG_PTR frame = rec->ExceptionInformation[1];
    void *handler = (void *)rec->ExceptionInformation[5];
    BYTE *nonvol_regs = (BYTE *)rec->ExceptionInformation[10];

    TRACE( "calling %p frame %Ix\n", handler, frame );
    return call_exc_handler( handler, frame, 0x100, nonvol_regs );
}


/*******************************************************************
 *		call_unwind_handler
 */
void *call_unwind_handler( void *handler, ULONG_PTR frame, DISPATCHER_CONTEXT *dispatch )
{
    TRACE( "calling %p frame %Ix\n", handler, frame );
    return call_exc_handler( handler, frame, 0x100, dispatch->NonVolatileRegisters );
}


/*******************************************************************
 *		get_exception_pc
 */
ULONG_PTR get_exception_pc( DISPATCHER_CONTEXT *dispatch )
{
    ULONG_PTR pc = dispatch->ControlPc;
    if (dispatch->ControlPcIsUnwound) pc -= 4;
    return pc;
}


/*******************************************************************
 *		_setjmp (MSVCRT.@)
 */
__ASM_GLOBAL_FUNC( _setjmp, "b _setjmpex" );


/*********************************************************************
 *              handle_fpieee_flt
 */
int handle_fpieee_flt( __msvcrt_ulong exception_code, EXCEPTION_POINTERS *ep,
                       int (__cdecl *handler)(_FPIEEE_RECORD*) )
{
    FIXME("(%lx %p %p)\n", exception_code, ep, handler);
    return EXCEPTION_CONTINUE_SEARCH;
}

#endif  /* __aarch64__ */
