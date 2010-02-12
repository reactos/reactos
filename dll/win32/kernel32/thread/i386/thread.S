/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       lib/kernel32/thread/i386/thread.S
 * PURPOSE:    Thread Start Thunks
 * PROGRAMMER: Alex Ionescu (alex@relsoft.net)
 */

.globl _BaseThreadStartupThunk@0
.globl _BaseProcessStartThunk@0
.intel_syntax noprefix

_BaseThreadStartupThunk@0:
    
    /* Start out fresh */
    xor ebp, ebp
    
    push ebx                  /* lpParameter */
    push eax                  /* lpStartAddress */
    push 0                    /* Return EIP */
    jmp _BaseThreadStartup@8

_BaseProcessStartThunk@0:

    /* Start out fresh */
    xor ebp, ebp
    
    push eax                  /* lpStartAddress */
    push 0                    /* Return EIP */
    jmp _BaseProcessStartup@4

/* EOF */
