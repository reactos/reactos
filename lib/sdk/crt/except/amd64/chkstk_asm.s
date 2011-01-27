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

PUBLIC MsgUnimplemented
MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"


FUNC _chkstk
    .endprolog
    UNIMPLEMENTED chkstk
    ret
ENDFUNC _chkstk

FUNC _alloca_probe
    .endprolog
    UNIMPLEMENTED alloca_probe
    ret
ENDFUNC _alloca_probe

END
/* EOF */
