/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/tan.S
 * PROGRAMER:         Magnus Olsen (magnus@greatlord.com)
 */
.globl _tan
.intel_syntax noprefix
_tan:
        sub     esp,4
        fld     qword ptr [ebp+8]
        fptan
        fstp    dword ptr [ebp+4]
        add     esp,4;
        ret
