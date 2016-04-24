/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CRT
 * FILE:            lib/sdk/crt/except/i386/prolog.s
 * PURPOSE:         SEH Support for the CRT
 * PROGRAMMERS:     Wine Development Team
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

/* FUNCTIONS *****************************************************************/
.code

PUBLIC __EH_prolog
// Copied from Wine.
__EH_prolog:
    push -1
    push eax
    push fs:0
    mov fs:0, esp
    mov eax, [esp + 12]
    mov [esp + 12], ebp
    lea ebp, [esp + 12]
    push eax
    ret

END
