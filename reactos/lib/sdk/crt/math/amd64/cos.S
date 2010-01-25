/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of cos
 * FILE:              lib/sdk/crt/math/amd64/cos.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/amd64/asm.h>
#include <ndk/amd64/asmmacro.S>

.intel_syntax noprefix


.proc cos
    UNIMPLEMENTED cos
    ret

.endproc
