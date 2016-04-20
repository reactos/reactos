/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/sdk/crt/except/amd64/seh.s
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

#define DISPOSITION_DISMISS         0
#define DISPOSITION_CONTINUE_SEARCH 1
#define DISPOSITION_COLLIDED_UNWIND 3

/* GLOBALS *******************************************************************/

PUBLIC _global_unwind2
PUBLIC _local_unwind
PUBLIC _local_unwind2
PUBLIC _abnormal_termination
PUBLIC _except_handler2
PUBLIC _except_handler3

/* CODE **********************************************************************/
.code64

FUNC _unwind_handler
    .endprolog
    ret
ENDFUNC

FUNC _global_unwind2
    .endprolog
    ret
ENDFUNC

FUNC _abnormal_termination
    .endprolog
    ret
ENDFUNC

FUNC _local_unwind
    .endprolog
    ret
ENDFUNC

FUNC _local_unwind2
    .endprolog
    ret
ENDFUNC

FUNC _except_handler2
    .endprolog
    ret
ENDFUNC

FUNC _except_handler3
    .endprolog
    ret
ENDFUNC

END
