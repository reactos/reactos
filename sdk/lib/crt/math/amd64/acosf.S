/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of acosf
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

PUBLIC acosf
acosf:
    UNIMPLEMENTED acos
    ret

END
