/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/fabs.S
 * PROGRAMER:         Magnus Olsen (greatlord@greatlord.com)
*/

.globl _fabs
.intel_syntax noprefix
_fabs:

        fld qword ptr [esp+8]
        fabs
        ret
