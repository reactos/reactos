/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/crt/misc/i386/seh.S
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>
.intel_syntax noprefix

#define DISPOSITION_DISMISS         0
#define DISPOSITION_CONTINUE_SEARCH 1
#define DISPOSITION_COLLIDED_UNWIND 3

/* GLOBALS *******************************************************************/

.globl __global_unwind2
.globl __local_unwind2
.globl __abnormal_termination
.globl __except_handler2
.globl __except_handler3

/* FUNCTIONS *****************************************************************/

.func unwind_handler
_unwind_handler:
    ret
.endfunc

.func _global_unwind2
__global_unwind2:
    ret
.endfunc

.func _abnormal_termination
__abnormal_termination:
    ret
.endfunc

.func _local_unwind2
__local_unwind2:
    ret
.endfunc

.func _except_handler2
__except_handler2:
    ret
.endfunc

.func _except_handler3
__except_handler3:
    ret
.endfunc
