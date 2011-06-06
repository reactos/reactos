/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _chkstk and _alloca_probe
 * FILE:              lib/sdk/crt/math/amd64/chkstk_asm.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

EXTERN MsgUnimplemented:BYTE

FUNC __chkstk
    .endprolog
    UNIMPLEMENTED chkstk
    ret
ENDFUNC __chkstk

FUNC __alloca_probe
    .endprolog
    UNIMPLEMENTED alloca_probe
    ret
ENDFUNC __alloca_probe

END
/* EOF */
