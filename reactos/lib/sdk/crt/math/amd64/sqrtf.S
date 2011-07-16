/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of tan
 * FILE:              lib/sdk/crt/math/amd64/sqrtf.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

/* CODE **********************************************************************/
.code64

PUBLIC sqrtf
sqrtf:
    sqrtss	xmm0, xmm0
    ret

END
