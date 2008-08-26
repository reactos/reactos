/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       lib/kernel32/thread/i386/thread.S
 * PURPOSE:    Thread Start Thunks
 * PROGRAMMER: Alex Ionescu (alex@relsoft.net)
 */

.globl _BaseThreadStartupThunk
.globl _BaseProcessStartThunk
.intel_syntax noprefix

_BaseThreadStartupThunk:
    
    /* Start out fresh */
    xor rbp, rbp
    
    push rbx                  /* lpParameter */
    push rax                  /* lpStartAddress */
    push 0                    /* Return RIP */
    jmp _BaseThreadStartup

_BaseProcessStartThunk:

    /* Start out fresh */
    xor rbp, rbp
    
    push rax                  /* lpStartAddress */
    push 0                    /* Return RIP */
    jmp _BaseProcessStartup

/* EOF */
