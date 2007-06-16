/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/trig_asm.s
 * PROGRAMER:         Aleksey Bragin (aleksey reactos org)
 *
 */
.globl _atan
.globl _cos
.globl _sin
 
.intel_syntax noprefix

/* FUNCTIONS ***************************************************************/

_atan:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]
        fld1
        fpatan
        pop     ebp
        ret

_cos:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]
        fcos
        pop     ebp
        ret

_sin:
        push    ebp
        mov     ebp,esp
        fld     qword ptr [ebp+8]
        fsin
        pop     ebp
        ret

_tan:
        push    ebp
        mov     ebp,esp
        sub     esp,4
        fld     qword ptr [ebp+8]
        fptan
        fstp    dword ptr [ebp-4]
        mov     esp,ebp
        pop     ebp
        ret

