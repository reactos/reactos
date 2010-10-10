/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/crt/misc/i386/prolog.s
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:     Wine Development Team
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

/* GLOBALS *******************************************************************/

.globl __EH_prolog

// Copied from Wine.
__EH_prolog:
    pushl    $-1
    pushl    %eax
    pushl    %fs:0
    movl     %esp, %fs:0
    movl     12(%esp), %eax
    movl     %ebp, 12(%esp)
    leal     12(%esp), %ebp
    pushl    %eax
    ret
