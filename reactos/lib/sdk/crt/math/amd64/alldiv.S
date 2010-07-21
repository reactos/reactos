/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of alldiv
 * FILE:              lib/sdk/crt/math/amd64/alldiv.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <reactos/asm.h>
#include <ndk/amd64/asm.h>

/* DATA *********************************************************************/

PUBLIC _fltused
_fltused:
        .long 0x9875

/* FUNCTIONS ****************************************************************/

.code64

.proc alldiv
    UNIMPLEMENTED alldiv
    ret

.endp alldiv
