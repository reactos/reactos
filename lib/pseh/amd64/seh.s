/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/pseh/amd64/seh.S
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#define DISPOSITION_DISMISS         0
#define DISPOSITION_CONTINUE_SEARCH 1
#define DISPOSITION_COLLIDED_UNWIND 3

#define EXCEPTION_EXIT_UNWIND 4
#define EXCEPTION_UNWINDING 2


EXTERN RtlUnwind:PROC

/* GLOBALS *******************************************************************/

PUBLIC _global_unwind2
PUBLIC _local_unwind2
PUBLIC _abnormal_termination
PUBLIC _except_handler2
PUBLIC _except_handler3

/* FUNCTIONS *****************************************************************/

.code
_unwind_handler:
    ret

_global_unwind2:
    ret

_abnormal_termination:
    ret

_local_unwind2:
    ret

_except_handler2:
    ret

_except_handler3:
    ret

END
