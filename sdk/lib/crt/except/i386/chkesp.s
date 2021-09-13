/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS C run time library
 * PURPOSE:           Stack checker
 * PROGRAMMERS:       Jérôme Gardou
 */

#include <asm.inc>
#include <ks386.inc>

/* Code is taken from wine 1.3.33,
 * Copyright Jon Griffiths and Alexandre Julliard
 */
EXTERN __chkesp_failed:PROC

PUBLIC __chkesp
.code
__chkesp:
    jnz .test_failed
    ret

.test_failed:
    push  ebp
    mov ebp, esp
    sub esp, 12
    push eax
    push ecx
    push edx
    call __chkesp_failed
    pop edx
    pop ecx
    pop eax
    leave
    ret

END