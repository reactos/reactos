/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/brkpoint.c
 * PURPOSE:         Handles breakpoints
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL 
DbgBreakPoint(VOID)
{
#if defined(__GNUC__)
   __asm__("int $3\n\t");
#elif defined(_MSC_VER)
   __asm int 3;
#else
#error Unknown compiler for inline assembler
#endif
}

/*
 * @implemented
 */
#if defined(__GNUC__)
 __asm__(".globl _DbgBreakPointNoBugCheck@0\n\t"
         "_DbgBreakPointNoBugCheck@0:\n\t"
         "int $3\n\t"
         "ret\n\t");
#endif

/*
 * @implemented
 */
VOID STDCALL 
DbgBreakPointWithStatus(ULONG Status)
{
#if defined(__GNUC__)
   __asm__("mov %0, %%eax\n\t"
           "int $3\n\t"
           ::"m"(Status));
#elif defined(_MSC_VER)
   __asm mov eax, Status
   __asm int 3;
#else
#error Unknown compiler for inline assembler
#endif
}
