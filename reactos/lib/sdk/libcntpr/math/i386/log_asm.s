/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Run-Time Library
 * FILE:              lib/rtl/i386/log.S
 * PROGRAMER:         Magnus Olsen (greatlord@greatlord.com)
 */
.globl _log
.intel_syntax noprefix

_log:
        fld     qword ptr [esp+4]
        fldln2
        fxch    st(1)
        fyl2x
        ret
