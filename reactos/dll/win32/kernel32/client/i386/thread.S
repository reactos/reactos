/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       lib/kernel32/thread/i386/thread.S
 * PURPOSE:    Thread Start Thunks
 * PROGRAMMER: Alex Ionescu (alex@relsoft.net)
 */

#include <asm.inc>
#include <ks386.inc>

.code

EXTERN _BaseThreadStartup@8:PROC
EXTERN _BaseProcessStartup@4:PROC

PUBLIC _BaseThreadStartupThunk@0
_BaseThreadStartupThunk@0:
    
    /* Start out fresh */
    xor ebp, ebp
    
    push ebx                  /* lpParameter */
    push eax                  /* lpStartAddress */
    push 0                    /* Return EIP */
    jmp _BaseThreadStartup@8
END

PUBLIC _BaseProcessStartThunk@0
_BaseProcessStartThunk@0:
    /* Start out fresh */
    xor ebp, ebp
    
    push eax                  /* lpStartAddress */
    push 0                    /* Return EIP */
    jmp _BaseProcessStartup@4

END

PUBLIC _BaseFiberStartup@0
_BaseFiberStartup@0:
    /* Get the fiber data */
    mov eax, fs:[TEB_FIBER_DATA]

    push dword ptr [eax+FIBER_CONTEXT_EBX] /* Parameter */
    push dword ptr [eax+FIBER_CONTEXT_EAX] /* Start Address */
    call _BaseThreadStartup@8
END

/* EOF */
