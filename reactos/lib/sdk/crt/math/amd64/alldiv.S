/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of alldiv
 * FILE:              lib/sdk/crt/math/amd64/alldiv.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/amd64/asm.h>
#include <ndk/amd64/asmmacro.S>

.intel_syntax noprefix

/* DATA *********************************************************************/

 .globl __fltused
__fltused:
        .long 0x9875

/* FUNCTIONS ****************************************************************/

.proc alldiv
    UNIMPLEMENTED alldiv
    ret

.endproc
