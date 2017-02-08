/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of fabs
 * FILE:              lib/sdk/crt/math/amd64/fabs.S
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

/* CODE **********************************************************************/
.code64

PUBLIC _fabs
_fabs:
    UNIMPLEMENTED fabs
    ret

#ifdef _USE_ML
ALIAS <fabs> = <_fabs>
#endif

END
