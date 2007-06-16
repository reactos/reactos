/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/sqrt.S
 * PROGRAMER:         Magnus Olsen (greatlord@greatlord.com)
 */

 
.globl _sqrt
 
.intel_syntax noprefix

_sqrt:
        fld     qword ptr [esp+4]
        fsqrt
        ret
