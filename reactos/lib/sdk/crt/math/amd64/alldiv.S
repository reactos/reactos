/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of alldiv
 * FILE:              lib/sdk/crt/math/amd64/alldiv.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* DATA **********************************************************************/
.code64

PUBLIC _fltused
_fltused:
        .long HEX(9875)

/* CODE **********************************************************************/
.code64

FUNC alldiv
    .endprolog
    UNIMPLEMENTED alldiv
    ret

ENDFUNC alldiv

END
