/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/sin.S
 * PROGRAMER:         Magnus Olsen (greatlord@greatlord.com)
 */
 
.globl _sin
.intel_syntax noprefix
_sin:
        fld     qword ptr [esp+4]
        fsin
        ret
