/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/sdk/crt/except/powerpc/seh.s
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:     arty
 */

/* INCLUDES ******************************************************************/

#include <ndk/asm.h>

#define DISPOSITION_DISMISS         0
#define DISPOSITION_CONTINUE_SEARCH 1
#define DISPOSITION_COLLIDED_UNWIND 3

/* GLOBALS *******************************************************************/

.globl _global_unwind2
.globl _local_unwind2
.globl _abnormal_termination
.globl _except_handler2
.globl _except_handler3

/* FUNCTIONS *****************************************************************/

unwind_handler:
        blr

_global_unwind2:
        blr

_local_unwind2:
        blr

_except_handler2:
        blr

_except_handler3:
        blr

//
//
// REMOVE ME REMOVE ME REMOVE ME REMOVE ME REMOVE ME REMOVE ME REMOVE ME
// sorry
//
//
.globl RtlpGetStackLimits
RtlpGetStackLimits:
        stwu 1,16(1)
        mflr 0
        
        stw 0,4(1)
        stw 3,8(1)
        stw 4,12(1)
        
        /* Get the current thread */
        lwz 3,KPCR_CURRENT_THREAD(13)

        /* Get the stack limits */
        lwz 4,KTHREAD_STACK_LIMIT(3)
        lwz 5,KTHREAD_INITIAL_STACK(3)
        subi 5,5,SIZEOF_FX_SAVE_AREA

        /* Return them */
        lwz 3,8(1)
        stw 4,0(3)

        lwz 3,12(1)
        stw 5,0(3)

        addi 1,1,16
        
        /* return */
        blr
