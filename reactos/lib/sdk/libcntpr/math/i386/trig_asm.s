/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/trig_asm.s
 * PROGRAMER:         Aleksey Bragin (aleksey reactos org)
*/

.globl _atan
.globl _cos
.globl _sin
.globl _tan

.intel_syntax noprefix

_atan:
        fld qword ptr [esp+8]
        fld1
        fpatan
        ret

_cos:
        fld qword ptr [esp+8]
        fcos
        ret

_sin:
        fld qword ptr [esp+8]
        fsin
        ret

_tan:
        fld     qword ptr [esp+8]
        fptan
        fstp    dword ptr [esp-4]
        ret
