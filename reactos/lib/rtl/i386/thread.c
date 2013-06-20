/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Rtl user thread functions
 * FILE:              lib/rtl/thread.c
 * PROGRAMERS:
 *                    Alex Ionescu (alex@relsoft.net)
 *                    Eric Kohl
 *                    KJK::Hyperion
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include "i386/ketypes.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *******************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlInitializeContext(IN HANDLE ProcessHandle,
                     OUT PCONTEXT ThreadContext,
                     IN PVOID ThreadStartParam  OPTIONAL,
                     IN PTHREAD_START_ROUTINE ThreadStartAddress,
                     IN PINITIAL_TEB InitialTeb)
{
    DPRINT("RtlInitializeContext: (hProcess: %p, ThreadContext: %p, Teb: %p\n",
            ProcessHandle, ThreadContext, InitialTeb);

    /*
     * Set the Initial Registers
     * This is based on NT's default values -- crazy apps might expect this...
     */
    ThreadContext->Ebp = 0;
    ThreadContext->Eax = 0;
    ThreadContext->Ebx = 1;
    ThreadContext->Ecx = 2;
    ThreadContext->Edx = 3;
    ThreadContext->Esi = 4;
    ThreadContext->Edi = 5;

    /* Set the Selectors */
    ThreadContext->SegGs = 0;
    ThreadContext->SegFs = KGDT_R3_TEB;
    ThreadContext->SegEs = KGDT_R3_DATA;
    ThreadContext->SegDs = KGDT_R3_DATA;
    ThreadContext->SegSs = KGDT_R3_DATA;
    ThreadContext->SegCs = KGDT_R3_CODE;

    /* Enable Interrupts */
    ThreadContext->EFlags = EFLAGS_INTERRUPT_MASK;

    /* Settings passed */
    ThreadContext->Eip = (ULONG)ThreadStartAddress;
    ThreadContext->Esp = (ULONG)InitialTeb;

    /* Only the basic Context is initialized */
    ThreadContext->ContextFlags = CONTEXT_CONTROL |
                                  CONTEXT_INTEGER |
                                  CONTEXT_SEGMENTS;

    /* Set up ESP to the right value */
    ThreadContext->Esp -= sizeof(PVOID);
    ZwWriteVirtualMemory(ProcessHandle,
                         (PVOID)ThreadContext->Esp,
                         (PVOID)&ThreadStartParam,
                         sizeof(PVOID),
                         NULL);

    /* Push it down one more notch for RETEIP */
    ThreadContext->Esp -= sizeof(PVOID);
}

/* EOF */
