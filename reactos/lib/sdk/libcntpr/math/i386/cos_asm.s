/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/cos.S
 * PROGRAMER:         Magnus Olsen (greatlord@greatlord.com)
*/

.globl _cos

.intel_syntax noprefix

_cos:
        fld qword ptr [esp+4]
        fcos
        ret
