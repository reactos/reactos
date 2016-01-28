/*
 * COPYRIGHT:  See COPYING in the top level directory
 * PROJECT:    ReactOS system libraries
 * FILE:       dll/win32/kernel32/client/amd64/thread.S
 * PURPOSE:    Thread Start Thunks
 * PROGRAMMER: Alex Ionescu (alex@relsoft.net)
 */

#include <asm.inc>
.code64

EXTERN BaseThreadStartup:PROC
EXTERN BaseProcessStartup:PROC

PUBLIC BaseThreadStartupThunk
PUBLIC BaseProcessStartThunk

BaseThreadStartupThunk:

    /* Start out fresh */
    xor rbp, rbp

    push rbx                  /* lpParameter */
    push rax                  /* lpStartAddress */
    push 0                    /* Return RIP */
    jmp BaseThreadStartup

BaseProcessStartThunk:

    /* Start out fresh */
    xor rbp, rbp

    push rax                  /* lpStartAddress */
    push 0                    /* Return RIP */
    jmp BaseProcessStartup

END
/* EOF */
