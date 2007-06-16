/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/atan.S
 * PROGRAMER:         Magnus Olsen (greatlord@greatlord.com)
*/
 
.globl _atan
 
.intel_syntax noprefix

_atan:
        fld qword ptr [esp+4]
        fld1
        fpatan
        ret
